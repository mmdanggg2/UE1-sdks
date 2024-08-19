/*=============================================================================
	OpenGLBase.cpp: OpenGL commons.

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

#include "OpenGLDrv.h"
#include "UTGLROpenGL.h"

/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

// Declare common procs
#define GLEntry(type, name) type FOpenGLBase::name = nullptr;
GL_ENTRYPOINTS_COMMON(GLEntry)
GL_ENTRYPOINTS_COMMON_OPTIONAL(GLEntry)
#undef GLEntry

// Declare common Extensions
#define GLExt(name,ext) UBOOL FOpenGLBase::Supports##name = 0;
GL_EXTENSIONS_COMMON(GLExt)
#undef GLExt

FOpenGLBase* FOpenGLBase::ActiveInstance        = nullptr;
TArray<FOpenGLBase*> FOpenGLBase::Instances;
GLint FOpenGLBase::MaxTextureSize               = 0;
GLint FOpenGLBase::MaxArrayTextureLayers        = 0;
GLint FOpenGLBase::MaxTextureImageUnits         = 0;
GLint FOpenGLBase::MaxAnisotropy                = 0;
GLint FOpenGLBase::MaxFramebufferSamples        = 0;
GLint FOpenGLBase::MaxUniformBlockSize          = 0;
GLint FOpenGLBase::MaxVertexUniformBlocks       = 0;
GLint FOpenGLBase::MaxGeometryUniformBlocks     = 0;
GLint FOpenGLBase::MaxFragmentUniformBlocks     = 0;
GLint FOpenGLBase::UniformBufferOffsetAlignment = 0;
TArray<FString> FOpenGLBase::Extensions;

UBOOL FOpenGLBase::UsingTrilinear               = 0;
GLint FOpenGLBase::UsingAnisotropy              = 0;
UBOOL FOpenGLBase::UsingDervMapping             = 0;
UBOOL FOpenGLBase::UsingColorCorrection         = 0;
FPlane FOpenGLBase::ColorCorrection             = FPlane(0,0,0,0);
GLfloat FOpenGLBase::LODBias                    = 0.f;
GLfloat FOpenGLBase::CurrentLODBias             = 0.f;

void* FOpenGLBase::CurrentWindow                = nullptr;
void* FOpenGLBase::CurrentContext               = nullptr;

/*-----------------------------------------------------------------------------
	Compatibility.
-----------------------------------------------------------------------------*/

namespace FGLCompat
{
	static void APIENTRY glClearDepthf( GLfloat value)
	{
		FOpenGLBase::glClearDepth( (GLclampd)value );
	}

	static void APIENTRY glDepthRangef( GLfloat n, GLfloat f)
	{
		FOpenGLBase::glDepthRange( (GLdouble)n, (GLdouble)f);
	}

	static void APIENTRY glMultiDrawArrays( GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount)
	{
		for ( GLsizei i=0; i<primcount; i++)
			FOpenGLBase::glDrawArrays( mode, first[i], count[i]);
	}

	static void APIENTRY glMultiDrawElements( GLenum mode, const GLsizei *count, GLenum type, const void* const* indices, GLsizei primcount)
	{
		for ( GLsizei i=0; i<primcount; i++)
			FOpenGLBase::glDrawElements( mode, count[i], type, indices[i]);
	}
};


//
// Small OpenGL validation layer
//
namespace FGLValidation
{
	static PFNGLACTIVETEXTUREPROC glActiveTexture_ORIGINAL = nullptr;
	static PFNGLBINDTEXTUREPROC glBindTexture_ORIGINAL = nullptr;

	static GLenum ActiveTextureUnit = GL_TEXTURE0;

	static void HandlePreviousError()
	{
		const TCHAR* ErrorText = FOpenGLBase::GetGLError();
		if ( ErrorText )
			debugf( NAME_DevGraphics, TEXT("Unhandled GL error: %s"), ErrorText);
	}

	static void APIENTRY glActiveTexture_VALIDATION(GLenum TextureUnit)
	{
		HandlePreviousError();
		GL_DEV_CHECK(TextureUnit >= GL_TEXTURE0 && TextureUnit <= GL_TEXTURE31);
		GLint Unit = TextureUnit - GL_TEXTURE0;
		if ( Unit >= FOpenGLBase::MaxTextureImageUnits )
			debugf( NAME_DevGraphics, TEXT("glActiveTexture(GL_TEXTURE0 + %i) error: Context supports up to %i TIU's"), Unit, FOpenGLBase::MaxTextureImageUnits);
		glActiveTexture_ORIGINAL(TextureUnit);
		const TCHAR* ErrorText = FOpenGLBase::GetGLError();
		if ( ErrorText )
			debugf( NAME_DevGraphics, TEXT("glActiveTexture(GL_TEXTURE0 + %i) error: %s"), Unit, ErrorText);
		ActiveTextureUnit = Unit;
	}

	static void APIENTRY glBindTexture_VALIDATION(GLenum Target, GLuint Texture)
	{
		HandlePreviousError();
		GL_DEV_CHECK(Target != 0);
		glBindTexture_ORIGINAL(Target, Texture);
		const TCHAR* ErrorText = FOpenGLBase::GetGLError();
		if ( ErrorText )
			debugf( NAME_DevGraphics, TEXT("glBindTexture(%s,%i) [Unit=%i] error: %s"), FOpenGLBase::GetTextureTargetString(Target), Texture, ActiveTextureUnit, ErrorText);
	}

	void Install()
	{
		if ( GL_DEV == 0 )
			return;

		glActiveTexture_ORIGINAL = FOpenGLBase::glActiveTexture;
		glBindTexture_ORIGINAL = FOpenGLBase::glBindTexture;
		FOpenGLBase::glActiveTexture = &glActiveTexture_VALIDATION;
		FOpenGLBase::glBindTexture = &glBindTexture_VALIDATION;
	}
};

/*-----------------------------------------------------------------------------
	Utils.
-----------------------------------------------------------------------------*/

static FString Feature( UBOOL Supported, const TCHAR* Feature, FString FeatureList=FString())
{
	if ( Supported )
	{
		if ( FeatureList.Len() == 0 )
			return FString::Printf(TEXT("%s"), Feature);
		else
			return FString::Printf(TEXT("%s, %s"), Feature, *FeatureList);
	}
	return FeatureList;
}

static FString FeatureInt( INT Value, const TCHAR* Feature, FString FeatureList=FString())
{
	if ( Value > 0 )
	{
		if ( FeatureList.Len() == 0 )
			return FString::Printf(TEXT("%s=%i"), Feature, Value);
		else
			return FString::Printf(TEXT("%s=%i, %s"), Feature, Value, *FeatureList);
	}
	return FeatureList;
}


/*-----------------------------------------------------------------------------
	Initialization.
-----------------------------------------------------------------------------*/

bool FOpenGLBase::InitProcs( bool Force)
{
	static bool AttemptedInit = false;
	static bool InitializeSuccess = false;
	if ( AttemptedInit && !Force )
		return InitializeSuccess;
	AttemptedInit = true;

	// Load procs
	#define GLEntry(type,name) name = (type)GetGLProc(#name);
#if _WIN32
	GL_ENTRYPOINTS_DLL_WGL(GLEntry)
	GL_ENTRYPOINTS_WGL(GLEntry)
#endif
	GL_ENTRYPOINTS_COMMON(GLEntry)
	GL_ENTRYPOINTS_COMMON_OPTIONAL(GLEntry)
	#undef GLEntry

	// Add workarounds for missing procs (GL version too low)
	if ( !glActiveTexture )                glActiveTexture = glActiveTextureARB;
	if ( !glClearDepthf && glClearDepth )  glClearDepthf = FGLCompat::glClearDepthf;
	if ( !glDepthRangef && glDepthRange )  glDepthRangef = FGLCompat::glDepthRangef;
	if ( !glCompressedTexImage2D )         *(void**)&glCompressedTexImage2D = GetGLProc("glCompressedTexImage2DARB");
	if ( !glCompressedTexSubImage2D )      *(void**)&glCompressedTexSubImage2D = GetGLProc("glCompressedTexSubImage2DARB");
	if ( !glMultiDrawArrays )              glMultiDrawArrays = glMultiDrawArraysEXT;
	if ( !glMultiDrawArrays )              glMultiDrawArrays = FGLCompat::glMultiDrawArrays;
	if ( !glMultiDrawElements )            glMultiDrawElements = glMultiDrawElementsEXT;
	if ( !glMultiDrawElements )            glMultiDrawElements = FGLCompat::glMultiDrawElements;
	if ( !glBindBuffer )
	{
		*(void**)&glGenBuffers = GetGLProc("glGenBuffersARB");
		*(void**)&glDeleteBuffers = GetGLProc("glDeleteBuffersARB");
		*(void**)&glBindBuffer = GetGLProc("glBindBufferARB");
		*(void**)&glBufferData = GetGLProc("glBufferDataARB");
		*(void**)&glBufferSubData = GetGLProc("glBufferSubDataARB");
		*(void**)&glMapBuffer = GetGLProc("glMapBufferARB");
		*(void**)&glUnmapBuffer = GetGLProc("glUnmapBufferARB");
	}
	if ( !glClipControl )                  *(void**)&glClipControl = GetGLProc("glClipControlEXT"); //GLES
	if ( !glBindVertexArray )
	{
		*(void**)&glIsVertexArray = GetGLProc("glIsVertexArrayAPPLE"); // GL_APPLE_vertex_array_object
		*(void**)&glGenVertexArrays = GetGLProc("glGenVertexArraysAPPLE");
		*(void**)&glDeleteVertexArrays = GetGLProc("glDeleteVertexArraysAPPLE");
		*(void**)&glBindVertexArray = GetGLProc("glBindVertexArrayAPPLE");
	}
	if ( !glFlushMappedBufferRange )      *(void**)&glFlushMappedBufferRange = GetGLProc("glFlushMappedBufferRangeAPPLE"); // GL_APPLE_flush_buffer_range

	// Additional diagnostics
	FGLValidation::Install();

	// Verify procs
	bool Success = true;
	#define GLEntry(type,name) \
	if ( name == nullptr ) \
	{ \
		debugf( NAME_Init, LocalizeError("MissingFunc"), appFromAnsi(#name), 0); \
		Success = false; \
	}
	GL_ENTRYPOINTS_COMMON(GLEntry)
	#undef GLEntry


	InitializeSuccess = Success;
	return Success;
}

void FOpenGLBase::InitCapabilities( bool Force)
{
	static bool Initialized = false;
	if ( Initialized && !Force )
		return;

	// Enumerate all extensions
	Extensions.Empty();
	GLint NumExtensions = 0;
	glGetIntegerv( GL_NUM_EXTENSIONS, &NumExtensions);
	if ( NumExtensions > 0 )
	{
		// GL >= 3.0 extensions parser
		// Get extensions one by one
		PFNGLGETSTRINGIPROC glGetStringi = (PFNGLGETSTRINGIPROC)GetGLProc("glGetStringi");
		if ( glGetStringi == nullptr )
			appErrorf( TEXT("Cannot get glGetStringi proc") );
		for ( INT i=0; i<NumExtensions; i++)
			Extensions.AddItem( (const char*)glGetStringi( GL_EXTENSIONS, i) ); /* FString( const ANSICHAR* AnsiIn); */
	}
	else
	{
		// GL < 3.0 extensions parser
		// Absorb error from previous glGetIntegerv (if any)
		glGetError();

		// Get extensions in a single string
		const char* GLExt = (const char*)glGetString(GL_EXTENSIONS);
		if ( GLExt )
		{
			FString ExtensionsString(GLExt); /* FString( const ANSICHAR* AnsiIn); */
			FString Left, Right;
			const FString SplitToken = TEXT(" ");
			while ( ExtensionsString.Len() > 0 )
			{
				FString* NewExtension;
				if ( !ExtensionsString.Split( SplitToken, &Left, &Right) )
					NewExtension = &ExtensionsString;
				else
				{
					NewExtension = &Left;
					ExchangeArray( ExtensionsString.GetCharArray(), Right.GetCharArray()); // Higor: move Right ==> ExtensionsString
				}
				INT Index = Extensions.AddZeroed();
				ExchangeArray( Extensions(Index).GetCharArray(), NewExtension->GetCharArray()); // Higor: move NewExtension ==> Extensions(Index), empty NewExtension
			}
		}
	}

	// Evaluate necessary extensions
	#define GLExt(name,ext) Supports##name = SupportsExtension(ext);
	GL_EXTENSIONS_COMMON(GLExt)
	#undef GLExt

	//
	// Evaluate what functionality needs to be manually enabled
	//
	/* GLES does not advertise ARB_vertex_buffer_object */
	if ( !SupportsVBO ) SupportsVBO = (glBindBuffer != nullptr);
	/* GLES does not advertise ARB_uniform_buffer_object */
	if ( !SupportsUBO ) SupportsUBO = (glBindBufferRange != nullptr); 
	/* GLES does not advertise ARB_vertex_array_object, GL may advertise APPLE_vertex_array_object */
	if ( !SupportsVAO ) SupportsVAO = (glBindVertexArray != nullptr);
	/* GL 4.6 Core extension */
	if ( !SupportsAnisotropy ) SupportsAnisotropy = (UBOOL)SupportsExtension(TEXT("GL_ARB_texture_filter_anisotropic"));
	/* GLES does not advertise ARB_texture_storage */
	if ( !SupportsTextureStorage ) SupportsTextureStorage = (glTexStorage2D) && (glTexStorage3D); 
	/* GLES does not advertise ARB_texture_storage_multisample */
	if ( !SupportsTextureStorageMultisample ) SupportsTextureStorageMultisample = (glTexStorage2DMultisample) && (glTexStorage3DMultisample); 
	/* GLES uses a per-component limited swizzle */
	if ( !SupportsTextureSwizzle ) SupportsTextureSwizzle = (UBOOL)SupportsExtension(TEXT("GL_EXT_texture_swizzle")); 
	/* GLES does not advertise ARB_invalidate_subdata */
	if ( !SupportsDataInvalidation ) SupportsDataInvalidation = (glInvalidateFramebuffer && glInvalidateTexImage); 
	/* GLES (research this!) */
	if ( !SupportsClipControl ) SupportsClipControl = (glClipControl != nullptr);

	glGetIntegerv( GL_MAX_TEXTURE_SIZE                     , &MaxTextureSize);
	glGetIntegerv( GL_MAX_ARRAY_TEXTURE_LAYERS             , &MaxArrayTextureLayers);
	glGetIntegerv( GL_MAX_TEXTURE_IMAGE_UNITS_ARB          , &MaxTextureImageUnits);
	if ( SupportsAnisotropy )
		glGetIntegerv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT   , &MaxAnisotropy);
	if ( SupportsFramebufferMultisampling )
		glGetIntegerv( GL_MAX_SAMPLES_EXT                  , &MaxFramebufferSamples);
	if ( SupportsUBO )
	{
		// Note: UNIFORM_BLOCKS is guaranteed to be at least 12
		glGetIntegerv( GL_MAX_UNIFORM_BLOCK_SIZE           , &MaxUniformBlockSize);
		glGetIntegerv( GL_MAX_VERTEX_UNIFORM_BLOCKS        , &MaxVertexUniformBlocks);
		glGetIntegerv( GL_MAX_GEOMETRY_UNIFORM_BLOCKS      , &MaxGeometryUniformBlocks);
		glGetIntegerv( GL_MAX_FRAGMENT_UNIFORM_BLOCKS      , &MaxFragmentUniformBlocks);
		glGetIntegerv( GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT  , &UniformBufferOffsetAlignment);
	}

	if ( MaxTextureSize != 0 )
	{
		if ( !Initialized )
		{
			debugf( NAME_Init, TEXT("GL_VENDOR     : %s"), appFromAnsi((const ANSICHAR*)glGetString(GL_VENDOR)));
			debugf( NAME_Init, TEXT("GL_RENDERER   : %s"), appFromAnsi((const ANSICHAR*)glGetString(GL_RENDERER)));
			debugf( NAME_Init, TEXT("GL_VERSION    : %s"), appFromAnsi((const ANSICHAR*)glGetString(GL_VERSION)));

			debugf( NAME_Init, TEXT("OpenGL: GL_MAX_TEXTURE_IMAGE_UNITS_ARB = %i"), MaxTextureImageUnits );

			debugf( NAME_Init, TEXT("OpenGL: Texture capabilities: [%s]"),
				*	FeatureInt(MaxTextureSize,TEXT("MaxSize"),
					FeatureInt(MaxArrayTextureLayers,TEXT("MaxLayers"),
					FeatureInt(MaxAnisotropy,TEXT("MaxAnisotropy"),
					Feature(SupportsDataInvalidation,TEXT("Invalidation"),
					Feature(SupportsTextureStorage,TEXT("Inmutable Storage"))
				)))));

			if ( SupportsFramebuffer )
			debugf( NAME_Init, TEXT("OpenGL: Framebuffer capabilities: [%s]"),
				*	Feature(SupportsFramebufferBlit,TEXT("Blit"),
					Feature(SupportsFramebufferMultisampling,TEXT("Multisample"),
					Feature(SupportsDataInvalidation,TEXT("Invalidation"))
				)));

			if ( SupportsUBO)
				debugf( NAME_Init, TEXT("OpenGL: Uniform capabilities: [BlockSize=%i] [Vert=%i] [Geo=%i] [Frag=%i] [Align=%i]"),
				MaxUniformBlockSize, MaxVertexUniformBlocks, MaxGeometryUniformBlocks, MaxFragmentUniformBlocks, UniformBufferOffsetAlignment);
			Initialized = true;
		}
	}
	else
		Initialized = false;
	// Call glGetError in case we have a missing instruction?
}

FOpenGLBase::~FOpenGLBase()
{
	if ( Context )
	{
		glFinish();
		DeleteContext(Context);
		Context = nullptr;
	}

#if WIN32
	HDC DC = GetDC((HWND)Window);
	if ( DC )
		ReleaseDC((HWND)Window, DC);
#endif

	if ( ActiveInstance == this )
		ActiveInstance = nullptr;
	Instances.RemoveItem(this);
}


/*-----------------------------------------------------------------------------
	State Control.
-----------------------------------------------------------------------------*/

void FOpenGLBase::SetColorCorrection( FLOAT Brightness)
{
	UsingColorCorrection = 1;

	FLOAT Gamma = 1.05f - Brightness * 0.25f; // Higor: emulate gammaramp look
	ColorCorrection = FPlane( Gamma, Gamma, Gamma, Brightness);
}

void FOpenGLBase::SetColorCorrection( FLOAT Brightness, FLOAT GammaOffsetRed, FLOAT GammaOffsetBlue, FLOAT GammaOffsetGreen)
{
	UsingColorCorrection = 1;

	FLOAT GammaRed = (GammaOffsetRed + 2.0f) > 0.f ? 1 / (GammaOffsetRed + 1.0f) : 1.f;
	FLOAT GammaGreen = (GammaOffsetGreen + 2.0f) > 0.f ? 1 / (GammaOffsetGreen + 1.0f) : 1.f;
	FLOAT GammaBlue = (GammaOffsetBlue + 2.0f) > 0.f ? 1 / (GammaOffsetBlue + 1.0f) : 1.f;
	ColorCorrection = FPlane( GammaRed, GammaGreen, GammaBlue, Brightness);
}


/*-----------------------------------------------------------------------------
	Generic abstraction layer
-----------------------------------------------------------------------------*/


//
// Texture storage allocator
// NOTE: Does not validate levels!
//
void FOpenGLBase::SetTextureStorage( FOpenGLTexture& Texture, const FTextureFormatInfo& Format, GLsizei Width, GLsizei Height, GLsizei Depth, GLsizei Levels)
{
	// This render device treats all Texture storage as inmutable
	if ( Texture.Allocated )
		return;

	Texture.USize  = (GLushort)Width;
	Texture.VSize  = (GLushort)Height;
	Texture.WSize  = (GLushort)Max(Depth,1);
	Texture.Format = Format.InternalFormat;
	Texture.Compressed = Format.Compressed;
	if ( !FOpenGLBase::SupportsTextureStorage )
	{
		GLsizei USize = Texture.USize;
		GLsizei VSize = Texture.VSize;
		GLsizei WSize = Texture.WSize;
		for ( GLuint Level=0; Level<Levels; Level++)
		{
			USize = Max<GLsizei>(USize, 1);
			VSize = Max<GLsizei>(VSize, 1);
			WSize = Max<GLsizei>(WSize, 1);
			switch ( Texture.Target )
			{
			case (GL_TEXTURE_2D):
				if ( Format.Compressed )
					glCompressedTexImage2D(Texture.Target, Level, Format.InternalFormat, USize, VSize, 0, Format.GetTextureBytes(USize,VSize), nullptr);
				else
					glTexImage2D(Texture.Target, Level, Format.InternalFormat, USize, VSize, 0, Format.SourceFormat, Format.Type, nullptr);
				break;
			case (GL_TEXTURE_3D):
			case (GL_TEXTURE_2D_ARRAY):
				if ( Format.Compressed )
					glCompressedTexImage3D(Texture.Target, Level, Format.InternalFormat, USize, VSize, WSize, 0, Format.GetTextureBytes(USize,VSize), nullptr);
				else
					glTexImage3D(Texture.Target, Level, Format.InternalFormat, USize, VSize, WSize, 0, Format.SourceFormat, Format.Type, nullptr);
				break;
			default:
				return;
			}
			USize >>= 1;
			VSize >>= 1;
			if ( Texture.Target != GL_TEXTURE_2D_ARRAY )
				WSize >>= 1;
		}
	}
	else
	{
		// ARB_texture_storage
		switch ( Texture.Target )
		{
		case (GL_TEXTURE_2D):
		case (GL_TEXTURE_CUBE_MAP):
			FOpenGLBase::glTexStorage2D(Texture.Target, Levels, Format.InternalFormat, Texture.USize, Texture.VSize);
			break;
		case (GL_TEXTURE_3D):
		case (GL_TEXTURE_2D_ARRAY):
			FOpenGLBase::glTexStorage3D(Texture.Target, Levels, Format.InternalFormat, Texture.USize, Texture.VSize, Texture.WSize);
			break;
		default:
			return;
		}
		Texture.InmutableFormat = true;
	}
	Texture.Allocated = true;
	Texture.BaseLevel = 0;
	Texture.MaxLevel  = Levels-1;
	glTexParameteri(Texture.Target, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(Texture.Target, GL_TEXTURE_MAX_LEVEL , Levels-1); //AMD drivers require this
}

//
// Upload Texture data
//
void FOpenGLBase::SetTextureData( FOpenGLTexture& Texture, const FTextureFormatInfo& Format, GLint Level, GLint Width, GLint Height, GLint Layer, void* TextureData, GLsizei TextureBytes)
{
	if ( Format.Compressed )
	{
		if ( Texture.Target == GL_TEXTURE_2D_ARRAY )
			glCompressedTexSubImage3D(Texture.Target, Level, 0, 0, Layer, Width, Height, 1, Format.InternalFormat, TextureBytes, TextureData);
		else
			glCompressedTexSubImage2D(Texture.Target, Level, 0, 0, Width, Height, Format.InternalFormat, TextureBytes, TextureData);
	}
	else
	{
		if ( Texture.Target == GL_TEXTURE_2D_ARRAY )
			glTexSubImage3D(Texture.Target, Level, 0, 0, Layer, Width, Height, 1, Format.SourceFormat, Format.Type, TextureData);
		else
			glTexSubImage2D(Texture.Target, Level, 0, 0, Width, Height, Format.SourceFormat, Format.Type, TextureData);
	}
}

//
// Apply standard texture filters
//
void FOpenGLBase::SetTextureFilters( FOpenGLTexture& Texture, bool Nearest)
{
	Texture.MagNearest = Nearest;

	bool UseTrilinear = GetTrilinear() != 0;
	glTexParameteri(Texture.Target, GL_TEXTURE_MIN_FILTER, FGL::GetMinificationFilter(Nearest, UseTrilinear));
	glTexParameteri(Texture.Target, GL_TEXTURE_MAG_FILTER, FGL::GetMagnificationFilter(Nearest));
	if ( GetAnisotropy() )
	{
		FLOAT Anisotropy = Nearest ? 1.0f : (FLOAT)GetAnisotropy();
		glTexParameterf(Texture.Target, GL_TEXTURE_MAX_ANISOTROPY_EXT, Anisotropy );
	}
}

//
// Mipmap autogeneration
//
bool FOpenGLBase::GenerateTextureMipmaps( FOpenGLTexture& Texture)
{
	if ( glGenerateMipmap )
		glGenerateMipmap(Texture.Target);
	else
		return false;
	return true;
}

#define CASE(t) case t: return TEXT(#t)

const TCHAR* FOpenGLBase::GetTextureTargetString( GLenum Target)
{
	switch (Target)
	{
		CASE(GL_TEXTURE_1D);
		CASE(GL_TEXTURE_2D);
		CASE(GL_TEXTURE_3D);
		CASE(GL_TEXTURE_2D_ARRAY);
	}
	return TEXT("UNKNOWN");
}

const TCHAR* FOpenGLBase::GetGLError()
{
	static TCHAR UnknownError[64];

	GLenum Error = FOpenGLBase::glGetError();
	if ( Error != GL_NO_ERROR )
	{
		switch (Error)
		{
			case GL_INVALID_ENUM:                  return TEXT("GL_INVALID_ENUM");
			case GL_INVALID_VALUE:                 return TEXT("GL_INVALID_VALUE");
			case GL_INVALID_OPERATION:             return TEXT("GL_INVALID_OPERATION");
			case GL_STACK_OVERFLOW:                return TEXT("GL_STACK_OVERFLOW");
			case GL_STACK_UNDERFLOW:               return TEXT("GL_STACK_UNDERFLOW");
			case GL_OUT_OF_MEMORY:                 return TEXT("GL_OUT_OF_MEMORY");
			case GL_INVALID_FRAMEBUFFER_OPERATION: return TEXT("GL_INVALID_FRAMEBUFFER_OPERATION");
			default:
				appSprintf(UnknownError, TEXT("UNKNOWN 0x%08X"), Error);
				return UnknownError;
		}
	}
	return nullptr;
}



/*-----------------------------------------------------------------------------
	OpenGLBase Interface
-----------------------------------------------------------------------------*/

/*static GLenum GetTextureBindingParameter( GLenum Target)
{
	switch ( Target )
	{
	case GL_TEXTURE_2D:
		return GL_TEXTURE_BINDING_2D;
	case GL_TEXTURE_3D:
		return GL_TEXTURE_BINDING_3D;
	case GL_TEXTURE_2D_ARRAY:
		return GL_TEXTURE_BINDING_2D_ARRAY;
	default:
		return 0;
	}
}*/

void FOpenGLBase::Reset()
{
	guard(FOpenGLBase::Reset);

	// Set default state
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LEQUAL);
	glPolygonOffset(-1.0f, -1.0f);
	glBlendFunc(GL_ONE, GL_ZERO);
	glDisable(GL_BLEND);

	// Verify Texture bindings and reset texture unit.
	TextureUnits.UnbindAll();

	// Reset viewport parameters.
	ViewportInfo = FViewportInfo{0,0,0,0};

	unguard;
}


//
// Viewport size handling
//
void FOpenGLBase::SetViewport( GLint NewX, GLint NewY, GLsizei NewXL, GLsizei NewYL)
{
	if ( ViewportInfo.ForceSet || ViewportInfo.X != NewX || ViewportInfo.Y != NewY || ViewportInfo.XL != NewXL || ViewportInfo.YL != NewYL )
	{
		ViewportInfo.X  = NewX;
		ViewportInfo.Y  = NewY;
		ViewportInfo.XL = NewXL;
		ViewportInfo.YL = NewYL;
		glViewport( NewX, NewY, NewXL, NewYL);
		ViewportInfo.ForceSet = 0;
	}
}
