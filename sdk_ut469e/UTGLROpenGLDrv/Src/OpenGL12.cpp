/*=============================================================================
	OpenGL1.cpp

	Compatibility profile context handling.

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

#include "OpenGLDrv.h"
#include "UTGLROpenGL.h"
#include "FOpenGL12.h"
#include "OpenGL_ARBProgram.h"

bool UOpenGLRenderDevice::InitGL1()
{
	return FOpenGL12::Init();
}

UBOOL UOpenGLRenderDevice::SetGL1( void* Window)
{
	guard(UOpenGLRenderDevice::SetGL1);

	if ( !GL )
		GL = new FOpenGL12(Window);
	else if ( !GL->Context )
		GL->Context = FOpenGLBase::CreateContext(Window);

	// Set GL1 permanent state.
	if ( GL )
	{
		GL->RenDev = this;
		GL->Reset();
		FOpenGLBase::ActiveInstance = GL;
	}
	return GL && GL->Context;

	unguard;
}


/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

// Declare GL12 procs and extensions
#define GLEntry(type, name) type FOpenGL12::name = nullptr;
#define GLExt(name,ext) UBOOL FOpenGL12::Supports##name = 0;
GL_ENTRYPOINTS_12(GLEntry)
GL_EXTENSIONS_12(GLExt)
#undef GLEntry
#undef GLExt


bool FOpenGL12::Init()
{
	guard(FOpenGL12::Init);

	static bool AttemptedInit = false;
	static bool SuccessInit = false;
	if ( AttemptedInit )
		return SuccessInit;

	// Evaluate required extensions
	#define GLExt(name,ext) Supports##name = SupportsExtension(ext);
	GL_EXTENSIONS_12(GLExt)
	#undef GLExt

	if ( !SupportsVertexProgram || !SupportsFragmentProgram || !SupportsMultiTexture || !SupportsSecondaryColor )
	{
		debugf( NAME_Warning, TEXT("Missing required OpenGL1/2 capabilities"));
		return false;
	}

	// Load required procs
	#define GLEntry(type,name) name = (type)FOpenGLBase::GetGLProc(#name);
	GL_ENTRYPOINTS_12(GLEntry)
	#undef GLEntry

	// Verify procs
	bool Success = true;
	#define GLEntry(type,name) \
	if ( name == nullptr ) \
	{ \
		debugf( NAME_Init, LocalizeError("MissingFunc"), appFromAnsi(#name), 0); \
		Success = false; \
	}
	GL_ENTRYPOINTS_12(GLEntry)
	#undef GLEntry

	// Enable/disable rendering features on success.
	if ( Success )
	{
		SupportsUBO = false;
		SupportsTBO = false;
	}

	SuccessInit = Success;
	return Success;

	unguard;
}


/*-----------------------------------------------------------------------------
	Per-context.
-----------------------------------------------------------------------------*/

FOpenGL12::FOpenGL12( void* InWindow)
	: FOpenGLBase(InWindow)
	, TextureEnableBits(0)
	, ClientTextureEnableBits(0)
	, ClientStateBits(0)
	, ActiveDrawBuffer(nullptr)
	, ProgramID(0)
	, ActiveVertexProgram(0)
	, ActiveFragmentProgram(0)
{
}

FOpenGL12::~FOpenGL12()
{
	FlushPrograms();
}

void FOpenGL12::Reset()
{
	FOpenGLBase::Reset();

	glShadeModel(GL_SMOOTH);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DITHER);
	TextureUnits.SetActive(0);
	SetEnabledClientTextures(0);
	SetEnabledClientStates(0);
	CurrentLODBias = 0;
	FlushPrograms();
	appMemzero( VertexEnvs, sizeof(VertexEnvs) + sizeof(FragmentEnvs) );

	// Tell context to use best quality for mipmap generation. (TODO: add to ES)
	if ( SupportsFramebuffer )
		glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	// Set modelview.
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glScalef(1.0f, -1.0f, -1.0f);
	GL_ERROR_ASSERT
}

void FOpenGL12::Lock()
{
	// Init buffers
	auto& DrawBuffer = RenDev->DrawBuffer;
	if ( !DrawBuffer.StreamBufferInit )
	{
		DrawBuffer.StreamBufferInit = 1;
		DrawBuffer.Complex       = new FGL::DrawBuffer::FComplexARB;
		DrawBuffer.ComplexStatic = new FGL::DrawBuffer::FComplexStaticARB;
		DrawBuffer.Gouraud       = new FGL::DrawBuffer::FGouraudARB;
		DrawBuffer.Quad          = new FGL::DrawBuffer::FQuadARB;
		DrawBuffer.Line          = new FGL::DrawBuffer::FLineARB;
		DrawBuffer.Fill          = new FGL::DrawBuffer::FFillARB;
		DrawBuffer.Decal         = new FGL::DrawBuffer::FDecalARB;
	}

	// Brightness, Gamma
	if ( UsingColorCorrection )
		SetFragmentShaderEnv<true>( 0, ColorCorrection);

	// LOD Bias status
	if ( LODBias != CurrentLODBias )
	{
		CurrentLODBias = LODBias;
		for ( INT i=TMU_SpecialMap; i>=0; i--)
		{
			glActiveTexture(GL_TEXTURE0 + i);
			glTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT, GL_TEXTURE_LOD_BIAS_EXT, LODBias);
		}
	}

	// Trilinear/Anisotropy filter status
	// Note: texture objects are shared between contexts so these trackers are global
	UOpenGLRenderDevice::TexturePool.SetTrilinearFiltering(UsingTrilinear!=0);
	UOpenGLRenderDevice::TexturePool.SetAnisotropicFiltering((GLfloat)UsingAnisotropy);

	// Handle shaders - Note: vertex shaders do not depend on Device settings
	DWORD FlushProgramID = 0;
	if ( RenDev->DetailMax != RenDev->PL_DetailMax )          FlushProgramID |= FGL::Program::Texture1;
	if ( GetShaderBrightness() != RenDev->PL_ShaderBrightness )     FlushProgramID |= FGL::Program::ColorCorrection;
	if ( FlushProgramID )
	{
		SetProgram(INDEX_NONE);
		for ( TMap<FProgramID,GLuint>::TIterator It(FragmentPrograms); It; ++It)
			if ( It.Key() & FlushProgramID )
			{
				glDeleteProgramsARB( 1, &It.Value());
				It.Value() = 0;
			}
	}

	// TODO: Evaluate removing this
	RenDev->SetDefaultProjectionState();
	GL_ERROR_ASSERT

}

void FOpenGL12::Unlock()
{
	SetProgram(INDEX_NONE);
}

//
// Decide what TexCoords are sent to the ARB vertex shader.
//
void FOpenGL12::SetEnabledClientTextures( BYTE NewEnableBits)
{
	BYTE EnableXor = ClientTextureEnableBits ^ NewEnableBits;
	if ( EnableXor != 0 )
	{
		INT ActiveUnit = 0;
		for ( INT i=7; i>=0; i--)
		{
			BYTE Bit = (1 << i);
			if ( EnableXor & Bit )
			{
				if ( ActiveUnit != i )
					glClientActiveTextureARB(GL_TEXTURE0 + i);
				if ( NewEnableBits & Bit )
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				else
					glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				ActiveUnit = i;
			}
		}
		if ( ActiveUnit != 0 )
			glClientActiveTextureARB(GL_TEXTURE0);
		ClientTextureEnableBits = NewEnableBits;
	}
}

//
// Decide what arrays are sent to the ARB vertex shader.
//
void FOpenGL12::SetEnabledClientStates( BYTE NewEnableBits)
{
	static const GLenum StateArray[] = 
	{
		GL_VERTEX_ARRAY_EXT,
		GL_NORMAL_ARRAY_EXT,
		GL_COLOR_ARRAY_EXT,
		GL_SECONDARY_COLOR_ARRAY_EXT
	};
	BYTE EnableXor = ClientStateBits ^ NewEnableBits;
	if ( EnableXor != 0 )
	{
		for ( INT i=0; i<ARRAY_COUNT(StateArray); i++)
		{
			BYTE Bit = (1 << i);
			if ( EnableXor & Bit )
			{
				if ( NewEnableBits & Bit )
					glEnableClientState(StateArray[i]);
				else
					glDisableClientState(StateArray[i]);
			}
		}
		ClientStateBits = NewEnableBits;
	}
}

//
// Bind the specified Textures
//
void FOpenGL12::SetTextures( FPendingTexture* Textures, BYTE NewTextureBits)
{
	guard(FOpenGL12::SetTextures);

	FOpenGLTexture* ChangedTextures[8];
	FGL::FTexturePool& TexturePool = UOpenGLRenderDevice::TexturePool;

	// Procedurally populate Texture array
	for ( BYTE i=0; i<8; i++)
	{
		if ( (NewTextureBits & (1 << i)) && TexturePool.Textures.IsValidIndex(Textures[i].PoolID) )
			ChangedTextures[i] = &TexturePool.Textures(Textures[i].PoolID);
		else
			ChangedTextures[i] = nullptr;
	}

	// Adjust DervMap parameters
	if ( ChangedTextures[0] && UsingDervMapping && (Textures[0].RelevantPolyFlags & PF_Masked) )
		SetVertexShaderEnv( 7, (FLOAT)ChangedTextures[0]->USize, (FLOAT)ChangedTextures[0]->VSize, 0, 1);

	// Verify that Textures have changed
	for ( BYTE i=0; i<8; i++)
		if ( ChangedTextures[i] )
		{
			bool NoSmooth     = (Textures[i].RelevantPolyFlags & PF_NoSmooth) != 0;
			bool ChangeSmooth = ChangedTextures[i]->MagNearest != NoSmooth; // TODO: Ensure that NoSmooth TextureViews never change this
			bool BindNew      = !TextureUnits.IsBound(*ChangedTextures[i], i);

			if ( ChangeSmooth || BindNew )
			{
				TextureUnits.SetActive(i);
				if ( BindNew )
				{
					TextureUnits.Bind(*ChangedTextures[i]);
				}
				if ( ChangeSmooth )
					SetTextureFilters(*ChangedTextures[i], NoSmooth);
			}
			else
				ChangedTextures[i] = nullptr; /* No changes in texture mapping unit*/
		}


	// TMU_Base changed, modify AlphaDerv mapping if needed (TODO: fix bad state control)
	GLfloat AlphaReject = FragmentEnvs[1].Z;
	if ( ChangedTextures[0] )
	{
		DWORD PolyFlags = Textures[0].RelevantPolyFlags;

		if ( PolyFlags & (PF_Highlighted|PF_AlphaBlend) )
			AlphaReject = 0.01f;
		else if ( PolyFlags & PF_Masked )
		{
			if ( UsingDervMapping )
			{
				FOpenGLTexture& DervMap = TexturePool.Textures(FGL::TEXPOOL_ID_DervMap);
				if ( DervMap.Texture && !TextureUnits.IsBound(DervMap, TMU_SpecialMap) )
				{
					TextureUnits.SetActiveNoCheck(TMU_SpecialMap);
					TextureUnits.Bind(DervMap);
				}
				AlphaReject = 0.2f;
			}
			else
				AlphaReject = 0.5f;
		}
	}
	SetFragmentShaderEnv( 1, 0, 0, AlphaReject, (RenDev->OneXBlending ? 1.0f : 2.0f) * 2.0f);
	TextureUnits.SetActive(0);

	unguard;
}


/*-----------------------------------------------------------------------------
	ARB Fragment Program and ARB Vertex program.
-----------------------------------------------------------------------------*/

//
// Binds (and creates) ARB shaders
//
void FOpenGL12::SetProgram( FProgramID NewProgramID, void**)
{
	guard(FOpenGL12::SetShaderARB);

	// Adjust ProgramID
	if ( (NewProgramID != INDEX_NONE) && !UsingDervMapping )
		NewProgramID &= ~FGL::Program::DervMapped;

	if ( NewProgramID == ProgramID )
		return;

	// Disable and unbind ARB Programs
	if ( NewProgramID == INDEX_NONE )
	{
		glBindProgramARB(GL_VERTEX_PROGRAM_ARB, 0);
		glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
		glDisable(GL_VERTEX_PROGRAM_ARB);
		glDisable(GL_FRAGMENT_PROGRAM_ARB);
		ActiveVertexProgram = 0;
		ActiveFragmentProgram = 0;
		ProgramID = INDEX_NONE;
		return;
	}

	// Enable ARB Programs
	if ( ProgramID == INDEX_NONE )
	{
		glEnable(GL_VERTEX_PROGRAM_ARB);
		glEnable(GL_FRAGMENT_PROGRAM_ARB);
	}
	ProgramID = NewProgramID;

	// Handle ARB Vertex Program
	FProgramID VPID = NewProgramID.GetVertexProgramID();
	check((VPID.GetValue() & NewProgramID.GetValue()) == VPID.GetValue() );
	GLuint* VP = VertexPrograms.Find(VPID);
	if ( !VP )
		VP = &VertexPrograms.SetNoFind(VPID.GetValue(), 0);
	if ( !VP[0] )
	{
		// Create on demand
		FVertexProgramWriter Writer;
		Writer.Create(VPID);
		VP[0] = CompileProgram(GL_VERTEX_PROGRAM_ARB, *Writer.Out, Writer.Out.Length());
	}
	if ( ActiveVertexProgram != VP[0] )
	{
		ActiveVertexProgram = VP[0];
		glBindProgramARB(GL_VERTEX_PROGRAM_ARB, VP[0]);
	}

	// Handle ARB Fragment Program
	FProgramID FPID = NewProgramID.GetFragmentProgramID();
	GLuint* FP = FragmentPrograms.Find(FPID);
	if ( !FP )
		FP = &FragmentPrograms.SetNoFind(FPID.GetValue(), 0);
	if ( !FP[0] )
	{
		// Create on demand
		FFragmentProgramWriter Writer;
		Writer.Create(FPID);

		FP[0] = CompileProgram(GL_FRAGMENT_PROGRAM_ARB, *Writer.Out, Writer.Out.Length());
	}
	if ( ActiveFragmentProgram != FP[0] )
	{
		ActiveFragmentProgram = FP[0];
		glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, FP[0]);
	}
	
	unguard;
}


//
// Destroys all ARB programs
//
void FOpenGL12::FlushPrograms()
{
	guard(FOpenGL12::FlushPrograms);

	SetProgram(INDEX_NONE);

	TArray<GLuint> Programs;
	for ( TMap<FProgramID,GLuint>::TIterator It(VertexPrograms);   It; ++It) if (It.Value()) Programs.AddItem(It.Value());
	for ( TMap<FProgramID,GLuint>::TIterator It(FragmentPrograms); It; ++It) if (It.Value()) Programs.AddItem(It.Value());
	VertexPrograms.Empty();
	FragmentPrograms.Empty();
	if ( Programs.Num() )
		glDeleteProgramsARB( Programs.Num(), &Programs(0) );

	unguard;
}


//
// Parses an ARB program
//
GLuint FOpenGL12::CompileProgram( GLenum Target, const ANSICHAR* String, const GLuint Len)
{
	guard(FOpenGL12::CompileProgram);

//	debugf( appFromAnsi(String));

	GLuint Program, OldProgram;
	GLint iErrorPos = INDEX_NONE;

	glGetProgramivARB( Target, GL_PROGRAM_BINDING_ARB, (GLint*)&OldProgram);
	glGenProgramsARB( 1, &Program);
	glBindProgramARB( Target, Program);
	glProgramStringARB( Target, GL_PROGRAM_FORMAT_ASCII_ARB, Len, String);
	glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &iErrorPos);
	if ( iErrorPos != INDEX_NONE )
	{
		static bool PreviousError = false;
		if ( !PreviousError )
		{
			PreviousError = true;
			TCHAR Line[128];
			appFromAnsiInPlace( Line, String + Clamp<INT>(iErrorPos, 0, strlen(String) - 1), 127);
			debugf( (EName)UOpenGLRenderDevice::StaticClass()->GetFName().GetIndex(), TEXT("%s program error at: \"%s...\""),
				(Target == GL_FRAGMENT_PROGRAM_ARB) ? TEXT("Fragment") : TEXT("Vertex"),
				Line);
		}
		glDeleteProgramsARB( 1, &Program);
		Program = 0;
	}
	glBindProgramARB( Target, OldProgram);
	return Program;

	unguard;
}
