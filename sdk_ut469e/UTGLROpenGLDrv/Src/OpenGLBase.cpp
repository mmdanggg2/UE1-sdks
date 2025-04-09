/*=============================================================================
	OpenGLBase.cpp: OpenGL commons.

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

#include "OpenGLDrv.h"
#include "UTGLROpenGL.h"

#include "OpenGL_TextureFormat.h"

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
GLint FOpenGLBase::MajorVersion                 = 0;
GLint FOpenGLBase::MinorVersion                 = 0;
GLint FOpenGLBase::MaxTextureSize               = 0;
GLint FOpenGLBase::MaxArrayTextureLayers        = 0;
GLint FOpenGLBase::MaxTextureImageUnits         = 0;
GLint FOpenGLBase::MaxAnisotropy                = 0;
GLint FOpenGLBase::MaxFramebufferSamples        = 0;
GLint FOpenGLBase::MaxUniformBlockSize          = 0;
GLint FOpenGLBase::MaxVertexUniformBlocks       = 0;
GLint FOpenGLBase::MaxFragmentUniformBlocks     = 0;
GLint FOpenGLBase::UniformBufferOffsetAlignment = 0;
GLint FOpenGLBase::MaxTextureBufferSize         = 0;
FString FOpenGLBase::Vendor;
FString FOpenGLBase::Renderer;
FString FOpenGLBase::Version;
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
	Internal Function declarations.
-----------------------------------------------------------------------------*/

//
// SetTextureStorage
//
static void (*InternalSetTextureStorage)( FOpenGLTexture&, const FTextureFormatInfo&) = nullptr;
static void InternalSetTextureStorage_Mutable( FOpenGLTexture&, const FTextureFormatInfo&);
static void InternalSetTextureStorage_Inmutable( FOpenGLTexture&, const FTextureFormatInfo&);
static void InternalSetTextureStorage_Inmutable_DSA( FOpenGLTexture&, const FTextureFormatInfo&);

//
// SetTextureData
//
static void (*InternalSetTextureData)( FOpenGLTexture&, const FTextureFormatInfo&, GLint, GLint, GLint, GLint, void*) = nullptr;
static void (*InternalSetCompressedTextureData)( FOpenGLTexture&, const FTextureFormatInfo&, GLint, GLint, GLint, GLint, void*, GLsizei) = nullptr;
static void InternalSetTextureData_Base( FOpenGLTexture&, const FTextureFormatInfo&, GLint, GLint, GLint, GLint, void*);
static void InternalSetTextureData_DSA( FOpenGLTexture&, const FTextureFormatInfo&, GLint, GLint, GLint, GLint, void*);
static void InternalSetCompressedTextureData_Base( FOpenGLTexture&, const FTextureFormatInfo&, GLint, GLint, GLint, GLint, void*, GLsizei);
static void InternalSetCompressedTextureData_DSA( FOpenGLTexture&, const FTextureFormatInfo&, GLint, GLint, GLint, GLint, void*, GLsizei);

//
// SetTextureFilters
//
static void (*InternalSetTextureFilters)( FOpenGLTexture&, bool Nearest) = nullptr;
static void InternalSetTextureFilters_Base( FOpenGLTexture&, bool);
static void InternalSetTextureFilters_DSA( FOpenGLTexture&, bool);

void FOpenGLBase::SetCurrentInstance( FOpenGLBase* Instance)
{
	ActiveInstance = Instance;
}

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

	static void* APIENTRY glMapNamedBufferRange( GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access)
	{
		FOpenGLBase::glBindBuffer(GL_ARRAY_BUFFER, buffer);
		void* Result = FOpenGLBase::glMapBufferRange(GL_ARRAY_BUFFER, offset, length, access);
		FOpenGLBase::glBindBuffer(GL_ARRAY_BUFFER, 0); // TODO: Track binding?
		return Result;
	}

	static GLboolean APIENTRY glUnmapNamedBuffer(GLuint buffer)
	{
		FOpenGLBase::glBindBuffer(GL_ARRAY_BUFFER, buffer);
		GLboolean Result = FOpenGLBase::glUnmapBuffer(GL_ARRAY_BUFFER);
		FOpenGLBase::glBindBuffer(GL_ARRAY_BUFFER, 0); // TODO: Track binding?
		return Result;
	}
};


//
// Small OpenGL validation layer
// todo: move to own cpp file
//
namespace FGLValidation
{
	static PFNGLACTIVETEXTUREPROC          glActiveTexture_ORIGINAL          = nullptr;
	static PFNGLBINDTEXTUREPROC            glBindTexture_ORIGINAL            = nullptr;
	static PFNGLTEXSUBIMAGE3DPROC          glTexSubImage3D_ORIGINAL          = nullptr;
	static PFNGLTEXTURESUBIMAGE3DPROC      glTextureSubImage3D_ORIGINAL      = nullptr;
	static PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus_ORIGINAL = nullptr;
	static PFNGLBUFFERSTORAGEPROC          glBufferStorage_ORIGINAL          = nullptr;
	static PFNGLNAMEDBUFFERSTORAGEPROC     glNamedBufferStorage_ORIGINAL     = nullptr;

	static GLenum ActiveTextureUnit = GL_TEXTURE0;

#define CASE_RETURN(casestring) case casestring: return TEXT(#casestring)
	static const TCHAR* ErrorToString_Framebuffer( GLenum Error)
	{
		switch (Error)
		{
			// No error
			CASE_RETURN(GL_FRAMEBUFFER_COMPLETE);
			// is returned if the specified framebuffer is the default read or draw framebuffer, but the default framebuffer does not exist.
			CASE_RETURN(GL_FRAMEBUFFER_UNDEFINED);
			// is returned if any of the framebuffer attachment points are framebuffer incomplete.
			CASE_RETURN(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
			// is returned if the framebuffer does not have at least one image attached to it.
			CASE_RETURN(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);
			// is returned if the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for any color attachment point(s) named by GL_DRAW_BUFFERi.
			CASE_RETURN(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER);
			// is returned if GL_READ_BUFFER is not GL_NONE and the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for the color attachment point named by GL_READ_BUFFER.
			CASE_RETURN(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER);
			// is returned if the combination of internal formats of the attached images violates an implementation-dependent set of restrictions.
			CASE_RETURN(GL_FRAMEBUFFER_UNSUPPORTED);
			// is returned
			// - if the value of GL_RENDERBUFFER_SAMPLES is not the same for all attached renderbuffers
			// - if the value of GL_TEXTURE_SAMPLES is the not same for all attached textures
			// - if the attached images are a mix of renderbuffers and textures, the value of GL_RENDERBUFFER_SAMPLES does not match the value of GL_TEXTURE_SAMPLES.
			// is also returned
			// - if the value of GL_TEXTURE_FIXED_SAMPLE_LOCATIONS is not the same for all attached textures
			// - if the attached images are a mix of renderbuffers and textures, the value of GL_TEXTURE_FIXED_SAMPLE_LOCATIONS is not GL_TRUE for all attached textures.
			CASE_RETURN(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE);
			// is returned if any framebuffer attachment is layered, and any populated attachment is not layered, or if all populated color attachments are not from textures of the same target. 
			CASE_RETURN(GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS);
		default:
			return TEXT("UNKNOWN");
		}
	}

	static void HandlePreviousError()
	{
		const TCHAR* ErrorText = FOpenGLBase::GetGLError();
		if ( ErrorText )
			debugf(NAME_DevGraphics, TEXT("Unhandled GL error: %s"), ErrorText);
	}

	static void APIENTRY glActiveTexture_VALIDATION( GLenum TextureUnit)
	{
		HandlePreviousError();
		GL_DEV_CHECK(TextureUnit >= GL_TEXTURE0 && TextureUnit <= GL_TEXTURE31);
		GLint Unit = TextureUnit - GL_TEXTURE0;
		if ( Unit >= FOpenGLBase::MaxTextureImageUnits )
			debugf(NAME_DevGraphics, TEXT("glActiveTexture(GL_TEXTURE%i) error: Context supports up to %i TIU's"), Unit, FOpenGLBase::MaxTextureImageUnits);
		glActiveTexture_ORIGINAL(TextureUnit);
		const TCHAR* ErrorText = FOpenGLBase::GetGLError();
		if ( ErrorText )
			debugf(NAME_DevGraphics, TEXT("glActiveTexture(GL_TEXTURE%i) error: %s"), Unit, ErrorText);
		ActiveTextureUnit = Unit;
	}

	static void APIENTRY glBindTexture_VALIDATION( GLenum Target, GLuint Texture)
	{
		HandlePreviousError();
		GL_DEV_CHECK(Target != 0);
		glBindTexture_ORIGINAL(Target, Texture);
		const TCHAR* ErrorText = FOpenGLBase::GetGLError();
		if ( ErrorText )
			debugf(NAME_DevGraphics, TEXT("glBindTexture(%s,%i) [Unit=%i] error: %s"), FOpenGLBase::GetTextureTargetString(Target), Texture, ActiveTextureUnit, ErrorText);
	}

	static void APIENTRY glTexSubImage3D_VALIDATION( GLenum Target, GLint Level, GLint XOffset, GLint YOffset, GLint ZOffset, GLsizei Width, GLsizei Height, GLsizei Depth, GLenum Format, GLenum Type, const void *Pixels)
	{
		HandlePreviousError();
		GL_DEV_CHECK(Target != 0);
		GL_DEV_CHECK(Level >= 0);
		GL_DEV_CHECK(Width > 0);
		GL_DEV_CHECK(Height > 0);
		GL_DEV_CHECK(Depth > 0);
		GL_DEV_CHECK(Pixels != 0);
		glTexSubImage3D_ORIGINAL(Target, Level, XOffset, YOffset, ZOffset, Width, Height, Depth, Format, Type, Pixels);
		const TCHAR* ErrorText = FOpenGLBase::GetGLError();
		if ( ErrorText )
			debugf(NAME_DevGraphics, TEXT("glTexSubImage3D(%s,%i,...) [Unit=%i] error: %s"), FOpenGLBase::GetTextureTargetString(Target), Level, ActiveTextureUnit, ErrorText);
	}

	static void APIENTRY glTextureSubImage3D_VALIDATION( GLuint Texture, GLint Level, GLint XOffset, GLint YOffset, GLint ZOffset, GLsizei Width, GLsizei Height, GLsizei Depth, GLenum Format, GLenum Type, const void *Pixels)
	{
		HandlePreviousError();
		GL_DEV_CHECK(Texture != 0);
		GL_DEV_CHECK(Level >= 0);
		GL_DEV_CHECK(Width > 0);
		GL_DEV_CHECK(Height > 0);
		GL_DEV_CHECK(Depth > 0);
		GL_DEV_CHECK(Pixels != 0);
		glTextureSubImage3D_ORIGINAL(Texture, Level, XOffset, YOffset, ZOffset, Width, Height, Depth, Format, Type, Pixels);
		const TCHAR* ErrorText = FOpenGLBase::GetGLError();
		if ( ErrorText )
			debugf(NAME_DevGraphics, TEXT("glTextureSubImage3D(%i,%i,...) [Unit=%i] error: %s"), Texture, Level, ActiveTextureUnit, ErrorText);
	}

	static GLenum APIENTRY glCheckFramebufferStatus_VALIDATION( GLenum Target)
	{
		HandlePreviousError();
		if ( (Target != GL_DRAW_FRAMEBUFFER) && (Target != GL_READ_FRAMEBUFFER) && (Target != GL_FRAMEBUFFER) )
		{
			debugf(NAME_DevGraphics, TEXT("glCheckFramebufferStatus(%i) error: Invalid Target parameter"), Target);
			return 0;
		}
		GLenum Result = glCheckFramebufferStatus_ORIGINAL(Target);
		const TCHAR* ErrorText = FOpenGLBase::GetGLError();
		if ( ErrorText )
			debugf(NAME_DevGraphics, TEXT("glCheckFramebufferStatus(%i) error: %s"), Target, ErrorText);
		if ( Result != GL_FRAMEBUFFER_COMPLETE )
			debugf(NAME_DevGraphics, TEXT("glCheckFramebufferStatus(%i) error: %s"), Target, ErrorToString_Framebuffer(Result));
		return Result;
	}

	static void APIENTRY glBufferStorage_VALIDATION( GLenum Target, GLsizeiptr Size, const void *Data, GLbitfield Flags)
	{
		HandlePreviousError();
		GL_DEV_CHECK(Size > 0);
		glBufferStorage_ORIGINAL(Target, Size, Data, Flags);
		const TCHAR* ErrorText = FOpenGLBase::GetGLError();
		if ( ErrorText )
			debugf(NAME_DevGraphics, TEXT("glBufferStorage(%i, %i, Data, Flags) error: %s"), Target, (INT)Size, ErrorText);
	}

	static void APIENTRY glNamedBufferStorage_VALIDATION( GLuint Buffer, GLsizeiptr Size, const void *Data, GLbitfield Flags)
	{
		HandlePreviousError();
		GL_DEV_CHECK(Buffer != 0);
		GL_DEV_CHECK(Size > 0);
		glNamedBufferStorage_ORIGINAL(Buffer, Size, Data, Flags);
		const TCHAR* ErrorText = FOpenGLBase::GetGLError();
		if ( ErrorText )
			debugf(NAME_DevGraphics, TEXT("glNamedBufferStorage(%i, %i, Data, Flags) error: %s"), Buffer, (INT)Size, ErrorText);
	}

	void Install()
	{
		if ( GL_DEV == 0 )
			return;

#define INSTALL_FUNC(func)                          \
	do                                              \
	{                                               \
		func##_ORIGINAL = FOpenGLBase::func;        \
		if ( func##_ORIGINAL )                      \
			FOpenGLBase::func = &func##_VALIDATION; \
	} while (0)

		INSTALL_FUNC(glActiveTexture);
		INSTALL_FUNC(glBindTexture);
		INSTALL_FUNC(glTexSubImage3D);
		INSTALL_FUNC(glTextureSubImage3D);
		INSTALL_FUNC(glCheckFramebufferStatus);
		INSTALL_FUNC(glBufferStorage);
		INSTALL_FUNC(glNamedBufferStorage);
#undef INSTALL_FUNC

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
	if ( !glMapNamedBufferRange )         *(void**)&glMapNamedBufferRange = (void*)FGLCompat::glMapNamedBufferRange;
	if ( !glUnmapNamedBuffer )            *(void**)&glUnmapNamedBuffer = (void*)FGLCompat::glUnmapNamedBuffer;
	if ( !glBindImageTexture )            *(void**)&glBindImageTexture = GetGLProc("glBindImageTextureEXT"); // GL_EXT_shader_image_load_store
	if ( !glMemoryBarrier )               *(void**)&glMemoryBarrier = GetGLProc("glMemoryBarrierEXT"); // GL_EXT_shader_image_load_store
	if ( !glTexBuffer )                   *(void**)&glTexBuffer = GetGLProc("glTexBufferOES"); // GL_OES_texture_buffer
	if ( !glTexBuffer )                   *(void**)&glTexBuffer = GetGLProc("glTexBufferEXT"); // GL_EXT_texture_buffer

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

	// Get OpenGL version as reported by the context
	glGetIntegerv(GL_MAJOR_VERSION, &MajorVersion);
	glGetIntegerv(GL_MINOR_VERSION, &MinorVersion);
	if ( !MajorVersion )
		glGetError(); // GL < 3.0, absorb error

	// Enumerate all extensions
	Extensions.Empty();
	GLint NumExtensions = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &NumExtensions);
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

	// Get driver strings
	if ( !Initialized )
	{
		Vendor   = (const ANSICHAR*)glGetString(GL_VENDOR);
		Renderer = (const ANSICHAR*)glGetString(GL_RENDERER);
		Version  = (const ANSICHAR*)glGetString(GL_VERSION);
		debugf( NAME_Init, TEXT("GL_VENDOR     : %s"), *Vendor);
		debugf( NAME_Init, TEXT("GL_RENDERER   : %s"), *Renderer);
		debugf( NAME_Init, TEXT("GL_VERSION    : %s"), *Version);
	}

	// Ugly way of detecting the GL ES renderers
	bool IsGLES = Version.InStr(TEXT("OpenGL ES")) != INDEX_NONE;
	bool IsGLES3 = Version.InStr(TEXT("OpenGL ES 3")) != INDEX_NONE;

	//
	// Evaluate what functionality needs to be manually enabled
	//
	/* GLES does not advertise ARB_vertex_buffer_object */
	if ( !SupportsVBO ) SupportsVBO = (glBindBuffer != nullptr);
	/* GLES does not advertise ARB_uniform_buffer_object */
	if ( !SupportsUBO ) SupportsUBO = (glBindBufferRange != nullptr); 
	/* GLES does not advertise ARB_vertex_array_object, GL may advertise APPLE_vertex_array_object */
	if ( !SupportsVAO ) SupportsVAO = (glBindVertexArray != nullptr);
	/* GLES does not advertise ARB_sampler_objects */
	if ( !SupportsSamplerObjects ) SupportsSamplerObjects = (glBindSampler != nullptr);
	/* GLES does not advertise ARB_framebuffer_object */
	if ( !SupportsFramebuffer ) SupportsFramebuffer = (glBindFramebuffer != nullptr);
	/* GLES does not advertise EXT_framebuffer_blit */
	if ( !SupportsFramebufferBlit ) SupportsFramebufferBlit = (glBlitFramebuffer != nullptr);
	/* GLES does not advertise ARB_framebuffer_multisample */
	if ( !SupportsFramebufferMultisampling ) SupportsFramebufferMultisampling = (glRenderbufferStorageMultisample != nullptr);
	/* GL 4.6 Core extension */
	if ( !SupportsAnisotropy ) SupportsAnisotropy = !!SupportsExtension(TEXT("GL_ARB_texture_filter_anisotropic"));
	/* GLES does not advertise ARB_texture_storage */
	if ( !SupportsTextureStorage ) SupportsTextureStorage = (glTexStorage2D) && (glTexStorage3D); 
	/* GLES does not advertise ARB_texture_storage_multisample */
	if ( !SupportsTextureStorageMultisample ) SupportsTextureStorageMultisample = (glTexStorage2DMultisample) && (glTexStorage3DMultisample); 
	/* GLES uses a per-component limited swizzle, not advertised since ES 3.0 */
	if ( !SupportsTextureSwizzle ) SupportsTextureSwizzle = IsGLES3 || SupportsExtension(TEXT("GL_EXT_texture_swizzle")); 
	/* GLES does not advertise ARB_invalidate_subdata */
	if ( !SupportsDataInvalidation ) SupportsDataInvalidation = (glInvalidateFramebuffer && glInvalidateTexImage); 
	/* GLES (research this!) */
	if ( !SupportsClipControl ) SupportsClipControl = (glClipControl != nullptr);
	/* GLES uses GL_OES_texture_buffer, GL may advertise GL_EXT_texture_buffer_object (do not check proc, RPI4 has bug)*/
	if ( !SupportsTBO ) SupportsTBO = SupportsExtension(TEXT("GL_OES_texture_buffer")) || SupportsExtension(TEXT("GL_EXT_texture_buffer"));
	/* GLES does not advertise ARB_compute_shader */
	if ( !SupportsComputeShader ) SupportsComputeShader = (glDispatchCompute != nullptr);
	/* GLES does not advertise ARB_shader_image_load_store */
	if ( !SupportsImageLoadStore ) SupportsImageLoadStore = (glBindImageTexture != nullptr);
	/* GL Core 3.0 and higher does not advertise gl_ClipDistance */
	if ( !SupportsClipCullDistance ) SupportsClipCullDistance = !IsGLES && (MajorVersion >= 3);

	//
	// Quirks
	//
	/* GLES Intel shader compiler has a bug with gl_ClipDistance */
	if ( IsGLES3 && SupportsClipCullDistance && (Vendor == TEXT("Intel")) )
	{
		debugf(NAME_Init, TEXT("OpenGL: Disabling GL_EXT_clip_cull_distance"));
		SupportsClipCullDistance = false;
	}

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
		glGetIntegerv( GL_MAX_FRAGMENT_UNIFORM_BLOCKS      , &MaxFragmentUniformBlocks);
		glGetIntegerv( GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT  , &UniformBufferOffsetAlignment);
	}
	if ( SupportsTBO )
		glGetIntegerv( GL_MAX_TEXTURE_BUFFER_SIZE          , &MaxTextureBufferSize);

	if ( MaxTextureSize != 0 )
	{
		if ( !Initialized )
		{
			debugf( NAME_Init, TEXT("OpenGL: Texture capabilities: [%s]"),
				*	FeatureInt(MaxTextureSize,TEXT("MaxSize"),
					FeatureInt(MaxArrayTextureLayers,TEXT("MaxLayers"),
					FeatureInt(MaxAnisotropy,TEXT("MaxAnisotropy"),
					Feature(SupportsDataInvalidation,TEXT("Invalidation"),
					Feature(SupportsTextureStorage,TEXT("Inmutable Storage"),
					Feature(SupportsTextureSwizzle,TEXT("Swizzle")))
				)))));

			if ( SupportsFramebuffer )
			debugf( NAME_Init, TEXT("OpenGL: Framebuffer capabilities: [%s]"),
				*	Feature(SupportsFramebufferBlit,TEXT("Blit"),
					Feature(SupportsFramebufferMultisampling,TEXT("Multisample"),
					Feature(SupportsDataInvalidation,TEXT("Invalidation"))
				)));

			if ( SupportsUBO)
				debugf( NAME_Init, TEXT("OpenGL: Uniform capabilities: [BlockSize=%i] [Vert=%i] [Frag=%i] [Align=%i]"),
				MaxUniformBlockSize, MaxVertexUniformBlocks, MaxFragmentUniformBlocks, UniformBufferOffsetAlignment);

			if ( SupportsComputeShader )
				debugf( NAME_Init, TEXT("OpenGL: Compute Shader"));

			Initialized = true;
		}
	}
	else
		Initialized = false;
	// Call glGetError in case we have a missing instruction?
}

void FOpenGLBase::InitInterfaces()
{
	// Select Texture Storage Mode
	InternalSetTextureStorage = SupportsDirectStateAccess ? InternalSetTextureStorage_Inmutable_DSA :
	                            SupportsTextureStorage    ? InternalSetTextureStorage_Inmutable     :
	                                                        InternalSetTextureStorage_Mutable;

	// Select Texture Data modes
	InternalSetTextureData = SupportsDirectStateAccess ? InternalSetTextureData_DSA :
	                                                     InternalSetTextureData_Base;
	InternalSetCompressedTextureData = SupportsDirectStateAccess ? InternalSetCompressedTextureData_DSA :
	                                                               InternalSetCompressedTextureData_Base;

	// Select Texture Filters Mode
	InternalSetTextureFilters = SupportsDirectStateAccess ? InternalSetTextureFilters_DSA :
	                                                        InternalSetTextureFilters_Base;
}

static void APIENTRY InternalDebugCallback
(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam
)
{
	FString Msg(message);
	debugf(NAME_DevGraphics, *Msg);
}

void FOpenGLBase::InstallDebugCallback()
{
	if ( !FOpenGLBase::glDebugMessageCallback )
	{
		debugf(NAME_Init, TEXT("Unable to install OpenGL debug callback"));
		return;
	}

	FOpenGLBase::glEnable(GL_DEBUG_OUTPUT);
	FOpenGLBase::glDebugMessageCallback(&InternalDebugCallback, nullptr);
	debugf(NAME_Init, TEXT("Installed OpenGL debug callback"));
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
	if ( (Brightness == 1.0f) && (GammaOffsetRed == 0.f) && (GammaOffsetBlue == 0.f) && (GammaOffsetGreen == 0.f) )
		UsingColorCorrection = 0;
	else
		UsingColorCorrection = 1;

	FLOAT GammaRed = (GammaOffsetRed + 2.0f) > 0.f ? 1 / (GammaOffsetRed + 1.0f) : 1.f;
	FLOAT GammaGreen = (GammaOffsetGreen + 2.0f) > 0.f ? 1 / (GammaOffsetGreen + 1.0f) : 1.f;
	FLOAT GammaBlue = (GammaOffsetBlue + 2.0f) > 0.f ? 1 / (GammaOffsetBlue + 1.0f) : 1.f;
	ColorCorrection = FPlane( GammaRed, GammaGreen, GammaBlue, Brightness);
}


/*-----------------------------------------------------------------------------
	Generic abstraction layer
-----------------------------------------------------------------------------*/

void FOpenGLBase::CreateInmutableBuffer( GLuint& Buffer, GLenum Target, GLsizeiptr Size, const void* Data, GLbitfield StorageFlags)
{
	GL_DEV_CHECK(!Buffer);

	if ( SupportsDirectStateAccess )
	{
		glCreateBuffers(1, &Buffer);
		glNamedBufferStorage(Buffer, Size, Data, StorageFlags);
	}
	else
	{
		glGenBuffers(1, &Buffer);
		glBindBuffer(Target, Buffer);
		glBufferStorage(Target, Size, Data, StorageFlags);
		glBindBuffer(Target, 0);
	}
}


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
	Texture.BaseLevel = 0;
	Texture.MaxLevel  = Levels-1;
	InternalSetTextureStorage(Texture, Format);
	Texture.Allocated = true;
}

//
// Upload Texture data
//
void FOpenGLBase::SetTextureData( FOpenGLTexture& Texture, const FTextureFormatInfo& Format, GLint Level, GLint Width, GLint Height, GLint Layer, void* TextureData, GLsizei TextureBytes)
{
	if ( Format.Compressed )
		InternalSetCompressedTextureData(Texture, Format, Level, Width, Height, Layer, TextureData, TextureBytes);
	else
		InternalSetTextureData(Texture, Format, Level, Width, Height, Layer, TextureData);
}

//
// Apply standard texture filters
//
void FOpenGLBase::SetTextureFilters( FOpenGLTexture& Texture, bool Nearest)
{
	Texture.MagNearest = Nearest;
	InternalSetTextureFilters(Texture, Nearest);
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

//
// Shader compiler
//
GLuint FOpenGLBase::CompileShader( GLenum Target, const ANSICHAR* Source)
{
	guard(FOpenGLBase::CompileShader);

	GLuint NewShader = glCreateShader(Target);
	glShaderSource(NewShader, 1, &Source, nullptr);
	glCompileShader(NewShader);

	GLint Result;
	glGetShaderiv(NewShader, GL_COMPILE_STATUS, &Result);
	if ( !Result )
	{
		glGetShaderiv(NewShader, GL_INFO_LOG_LENGTH, &Result);
		TArray<ANSICHAR> AnsiText(Result);
		glGetShaderInfoLog(NewShader, Result, &Result, &AnsiText(0));
		glDeleteShader(NewShader);
		NewShader = 0;
		appErrorf( TEXT("Unable to compile shader: %s\n%s"), appFromAnsi(&AnsiText(0)), appFromAnsi(Source) );
	}

	return NewShader;
	unguard;
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

INT FOpenGLBase::FlushGLErrors()
{
	INT ErrorCount = 0;

	const TCHAR* Error;
	while ( (Error=GetGLError()) != nullptr )
	{
		ErrorCount++;
		if ( ErrorCount >= 100 )
			appErrorf(TEXT("glGetError infinite loop: %s"), Error);
	}
	return ErrorCount;
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

void FOpenGLBase::BindBufferBase( GLenum Target, GLuint Index, GLuint Buffer)
{
	GL_DEV_CHECK(Index < ARRAY_COUNT(IndexedBufferTargets));
	if ( IndexedBufferTargets[Index] != Buffer )
	{
		IndexedBufferTargets[Index] = Buffer;
		glBindBufferBase(Target, Index, Buffer);
	}
}

/*-----------------------------------------------------------------------------
	Internal Texture Storage implementations.
-----------------------------------------------------------------------------*/

static void InternalSetTextureStorage_Mutable( FOpenGLTexture& Texture, const FTextureFormatInfo& Format)
{
	GLsizei USize = Texture.USize;
	GLsizei VSize = Texture.VSize;
	GLsizei WSize = Texture.WSize;
	for ( GLuint Level=0; Level<=Texture.MaxLevel; Level++)
	{
		USize = Max<GLsizei>(USize, 1);
		VSize = Max<GLsizei>(VSize, 1);
		WSize = Max<GLsizei>(WSize, 1);
		switch ( Texture.Target )
		{
		case (GL_TEXTURE_2D):
			if ( Format.Compressed )
				FOpenGLBase::glCompressedTexImage2D(Texture.Target, Level, Format.InternalFormat, USize, VSize, 0, Format.GetTextureBytes(USize,VSize), nullptr);
			else
				FOpenGLBase::glTexImage2D(Texture.Target, Level, Format.InternalFormat, USize, VSize, 0, Format.SourceFormat, Format.Type, nullptr);
			break;
		case (GL_TEXTURE_3D):
		case (GL_TEXTURE_2D_ARRAY):
			if ( Format.Compressed )
				FOpenGLBase::glCompressedTexImage3D(Texture.Target, Level, Format.InternalFormat, USize, VSize, WSize, 0, Format.GetTextureBytes(USize,VSize), nullptr);
			else
				FOpenGLBase::glTexImage3D(Texture.Target, Level, Format.InternalFormat, USize, VSize, WSize, 0, Format.SourceFormat, Format.Type, nullptr);
			break;
		default:
			return;
		}
		USize >>= 1;
		VSize >>= 1;
		if ( Texture.Target != GL_TEXTURE_2D_ARRAY )
			WSize >>= 1;
	}
	FOpenGLBase::glTexParameteri(Texture.Target, GL_TEXTURE_BASE_LEVEL, 0);
	FOpenGLBase::glTexParameteri(Texture.Target, GL_TEXTURE_MAX_LEVEL , Texture.MaxLevel); //AMD drivers require this
}

static void InternalSetTextureStorage_Inmutable( FOpenGLTexture& Texture, const FTextureFormatInfo& Format)
{
	switch ( Texture.Target )
	{
	case (GL_TEXTURE_2D):
	case (GL_TEXTURE_CUBE_MAP):
		FOpenGLBase::glTexStorage2D(Texture.Target, Texture.MaxLevel+1, Format.InternalFormat, Texture.USize, Texture.VSize);
		break;
	case (GL_TEXTURE_3D):
	case (GL_TEXTURE_2D_ARRAY):
		FOpenGLBase::glTexStorage3D(Texture.Target, Texture.MaxLevel+1, Format.InternalFormat, Texture.USize, Texture.VSize, Texture.WSize);
		break;
	default:
		return;
	}
	FOpenGLBase::glTexParameteri(Texture.Target, GL_TEXTURE_BASE_LEVEL, 0);
	FOpenGLBase::glTexParameteri(Texture.Target, GL_TEXTURE_MAX_LEVEL , Texture.MaxLevel); //AMD drivers require this
	Texture.InmutableFormat = true;
}

static void InternalSetTextureStorage_Inmutable_DSA( FOpenGLTexture& Texture, const FTextureFormatInfo& Format)
{
	switch ( Texture.Target )
	{
	case (GL_TEXTURE_2D):
	case (GL_TEXTURE_CUBE_MAP):
		FOpenGLBase::glTextureStorage2D(Texture.Texture, Texture.MaxLevel+1, Format.InternalFormat, Texture.USize, Texture.VSize);
		break;
	case (GL_TEXTURE_3D):
	case (GL_TEXTURE_2D_ARRAY):
		FOpenGLBase::glTextureStorage3D(Texture.Texture, Texture.MaxLevel+1, Format.InternalFormat, Texture.USize, Texture.VSize, Texture.WSize);
		break;
	default:
		return;
	}
	FOpenGLBase::glTextureParameteri(Texture.Texture, GL_TEXTURE_BASE_LEVEL, 0);
	FOpenGLBase::glTextureParameteri(Texture.Texture, GL_TEXTURE_MAX_LEVEL , Texture.MaxLevel); //AMD drivers require this
	Texture.InmutableFormat = true;
}

/*-----------------------------------------------------------------------------
	Internal Texture Data implementations.
-----------------------------------------------------------------------------*/

static void InternalSetTextureData_Base( FOpenGLTexture& Texture, const FTextureFormatInfo& Format, GLint Level, GLint Width, GLint Height, GLint Layer, void* TextureData)
{
	if ( Texture.Target == GL_TEXTURE_2D_ARRAY )
		FOpenGLBase::glTexSubImage3D(Texture.Target, Level, 0, 0, Layer, Width, Height, 1, Format.SourceFormat, Format.Type, TextureData);
	else
		FOpenGLBase::glTexSubImage2D(Texture.Target, Level, 0, 0, Width, Height, Format.SourceFormat, Format.Type, TextureData);
}

static void InternalSetTextureData_DSA( FOpenGLTexture& Texture, const FTextureFormatInfo& Format, GLint Level, GLint Width, GLint Height, GLint Layer, void* TextureData)
{
	if ( Texture.Target == GL_TEXTURE_2D_ARRAY )
		FOpenGLBase::glTextureSubImage3D(Texture.Texture, Level, 0, 0, Layer, Width, Height, 1, Format.SourceFormat, Format.Type, TextureData);
	else
		FOpenGLBase::glTextureSubImage2D(Texture.Texture, Level, 0, 0, Width, Height, Format.SourceFormat, Format.Type, TextureData);
}

static void InternalSetCompressedTextureData_Base( FOpenGLTexture& Texture, const FTextureFormatInfo& Format, GLint Level, GLint Width, GLint Height, GLint Layer, void* TextureData, GLsizei TextureBytes)
{
	if ( Texture.Target == GL_TEXTURE_2D_ARRAY )
		FOpenGLBase::glCompressedTexSubImage3D(Texture.Target, Level, 0, 0, Layer, Width, Height, 1, Format.InternalFormat, TextureBytes, TextureData);
	else
		FOpenGLBase::glCompressedTexSubImage2D(Texture.Target, Level, 0, 0, Width, Height, Format.InternalFormat, TextureBytes, TextureData);
}

static void InternalSetCompressedTextureData_DSA( FOpenGLTexture& Texture, const FTextureFormatInfo& Format, GLint Level, GLint Width, GLint Height, GLint Layer, void* TextureData, GLsizei TextureBytes)
{
	if ( Texture.Target == GL_TEXTURE_2D_ARRAY )
		FOpenGLBase::glCompressedTextureSubImage3D(Texture.Texture, Level, 0, 0, Layer, Width, Height, 1, Format.InternalFormat, TextureBytes, TextureData);
	else
		FOpenGLBase::glCompressedTextureSubImage2D(Texture.Texture, Level, 0, 0, Width, Height, Format.InternalFormat, TextureBytes, TextureData);
}


/*-----------------------------------------------------------------------------
	Internal Texture Filter implementations.
-----------------------------------------------------------------------------*/

static void InternalSetTextureFilters_Base( FOpenGLTexture& Texture, bool Nearest)
{
	bool UseTrilinear = FOpenGLBase::GetTrilinear() != 0;
	FOpenGLBase::glTexParameteri(Texture.Target, GL_TEXTURE_MIN_FILTER, FGL::GetMinificationFilter(Nearest, UseTrilinear));
	FOpenGLBase::glTexParameteri(Texture.Target, GL_TEXTURE_MAG_FILTER, FGL::GetMagnificationFilter(Nearest));
	if ( FOpenGLBase::GetAnisotropy() )
	{
		FLOAT Anisotropy = Nearest ? 1.0f : (FLOAT)FOpenGLBase::GetAnisotropy();
		FOpenGLBase::glTexParameterf(Texture.Target, GL_TEXTURE_MAX_ANISOTROPY_EXT, Anisotropy);
	}
}

static void InternalSetTextureFilters_DSA( FOpenGLTexture& Texture, bool Nearest)
{
	bool UseTrilinear = FOpenGLBase::GetTrilinear() != 0;
	FOpenGLBase::glTextureParameteri(Texture.Texture, GL_TEXTURE_MIN_FILTER, FGL::GetMinificationFilter(Nearest, UseTrilinear));
	FOpenGLBase::glTextureParameteri(Texture.Texture, GL_TEXTURE_MAG_FILTER, FGL::GetMagnificationFilter(Nearest));
	if ( FOpenGLBase::GetAnisotropy() )
	{
		FLOAT Anisotropy = Nearest ? 1.0f : (FLOAT)FOpenGLBase::GetAnisotropy();
		FOpenGLBase::glTextureParameterf(Texture.Texture, GL_TEXTURE_MAX_ANISOTROPY_EXT, Anisotropy);
	}
}
