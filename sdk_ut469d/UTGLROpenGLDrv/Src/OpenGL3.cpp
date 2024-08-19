/*=============================================================================
	OpenGL3.cpp

	Core 3.3 profile context handling.

	Notes:
	- VAOs are not shareable

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

#include "OpenGLDrv.h"
#include "UTGLROpenGL.h"
#include "FOpenGL3.h"
#include "OpenGL_ShaderWriter.h"



/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

// Declare GL3 procs and extensions
#define GLEntry(type, name) type FOpenGL3::name = nullptr;
#define GLExt(name,ext) UBOOL FOpenGL3::Supports##name = 0;
GL_ENTRYPOINTS_3(GLEntry)
GL_EXTENSIONS_3(GLExt)
#undef GLEntry
#undef GLExt

TMapExt<FProgramID,FOpenGL3::FProgramData> FOpenGL3::Programs;
TMapExt<FProgramID,GLuint> FOpenGL3::VertexShaders;
TMapExt<FProgramID,GLuint> FOpenGL3::GeometryShaders;
TMapExt<FProgramID,GLuint> FOpenGL3::FragmentShaders;

bool UOpenGLRenderDevice::InitGL3()
{
	return FOpenGL3::Init();
}

UBOOL UOpenGLRenderDevice::SetGL3( void* Window)
{
	guard(UOpenGLRenderDevice::SetGL3);

	if ( !GL )
		GL = new FOpenGL3(Window);
	else if ( !GL->Context )
		GL->Context = FOpenGLBase::CreateContext(Window);

	// Set GL3 permanent state.
	if ( GL )
	{
		GL->RenDev = this;
		GL->Reset();
		FOpenGLBase::ActiveInstance = GL;
	}
	return GL && GL->Context;

	unguard;
}

bool FOpenGL3::Init()
{
	guard(FOpenGL3::Init);

	static bool AttemptedInit = false;
	static bool SuccessInit = false;
	if ( AttemptedInit )
		return SuccessInit;

	// Evaluate required extensions
	#define GLExt(name,ext) Supports##name = SupportsExtension(ext);
	GL_EXTENSIONS_3(GLExt)
	#undef GLExt

	if ( !SupportsSamplerObjects || !SupportsVBO || !SupportsVAO )
		return false;

	// Load required procs
	#define GLEntry(type,name) name = (type)GetGLProc(#name);
	GL_ENTRYPOINTS_3(GLEntry)
	#undef GLEntry

	// Verify procs
	bool Success = true;
	#define GLEntry(type,name) \
	if ( name == nullptr ) \
	{ \
		debugf( NAME_Init, LocalizeError("MissingFunc"), appFromAnsi(#name)); \
		Success = false; \
	}
	GL_ENTRYPOINTS_3(GLEntry)
	#undef GLEntry

	// Enable/disable rendering features on success.
	if ( Success )
	{
		INT MaxUniformBlockSize = Min(FOpenGLBase::MaxUniformBlockSize, 65536) & (~15);
		UOpenGLRenderDevice::TexturePool.InitUniformQueue(MaxUniformBlockSize / 16);
		UOpenGLRenderDevice::TexturePool.UseArrayTextures = 1;

		SupportsClipCullDistance = true;
	}

	SuccessInit = Success;
	return Success;

	unguard;
}


/*-----------------------------------------------------------------------------
	Per-context.
-----------------------------------------------------------------------------*/

FOpenGL3::FOpenGL3( void* InWindow)
	: FOpenGLBase(InWindow)
	, ActiveVAO(0)
	, ProgramID(0)
{
	// Register existing UBO's to this OpenGL context
	if ( RenDev->bufferId_GlobalRenderUBO )
		glBindBufferBase(GL_UNIFORM_BUFFER, FGlobalRender_UBO::UniformIndex, RenDev->bufferId_GlobalRenderUBO);
	if ( RenDev->bufferId_StaticBspUBO )
		glBindBufferBase(GL_UNIFORM_BUFFER, FStaticBsp_UBO::UniformIndex, RenDev->bufferId_StaticBspUBO);
	if ( RenDev->bufferId_TextureParamsUBO )
		glBindBufferBase(GL_UNIFORM_BUFFER, FTextureParams_UBO::UniformIndex, RenDev->bufferId_TextureParamsUBO);
	if ( RenDev->bufferId_csTextureScaleUBO )
		glBindBufferBase(GL_UNIFORM_BUFFER, FComplexSurfaceScale_UBO::UniformIndex, RenDev->bufferId_csTextureScaleUBO);
}

FOpenGL3::~FOpenGL3()
{
	FlushPrograms();

	// Safely delete VAOs
	for ( TMap<DWORD,GLuint>::TIterator It(VAOs); It; ++It)
		if ( It.Value() && glIsVertexArray(It.Value()) )
			glDeleteVertexArrays(1, &It.Value());
	VAOs.Empty();
}

void FOpenGL3::Reset()
{
	FOpenGLBase::Reset();
}

void FOpenGL3::Lock()
{
	// Handle shaders
	UBOOL FlushShaders = 0;
	FlushShaders |= (RenDev->DetailMax != RenDev->PL_DetailMax);
	FlushShaders |= ((GetShaderBrightness() > 1.0) != (RenDev->PL_ShaderBrightness > 1.0) ) << 1;
	FlushShaders |= ((GetShaderBrightness() < 1.0) != (RenDev->PL_ShaderBrightness < 1.0) ) << 2;
	if ( FlushShaders )
		FlushPrograms();

	// Init buffers
	auto& DrawBuffer = RenDev->DrawBuffer;
	if ( !DrawBuffer.StreamBufferInit )
	{
		DrawBuffer.StreamBufferInit = 1;
		DrawBuffer.Complex = new FGL::DrawBuffer::FComplexGLSL3;
		DrawBuffer.Gouraud = new FGL::DrawBuffer::FGouraudGLSL3;
		DrawBuffer.Quad    = new FGL::DrawBuffer::FQuadGLSL3;
		DrawBuffer.Line    = new FGL::DrawBuffer::FLineGLSL3;
		DrawBuffer.Fill    = new FGL::DrawBuffer::FFillGLSL3;
		DrawBuffer.Decal   = new FGL::DrawBuffer::FDecalGLSL3;

		DrawBuffer.GeneralBuffer[0] = new FGL::VertexBuffer::CopyToVBO(64 * 1024 * 1024);
		DrawBuffer.Complex->Buffer = DrawBuffer.GeneralBuffer[0];
		DrawBuffer.Gouraud->Buffer = DrawBuffer.GeneralBuffer[0];
		DrawBuffer.Quad->Buffer = DrawBuffer.GeneralBuffer[0];
		DrawBuffer.Line->Buffer = DrawBuffer.GeneralBuffer[0];
		DrawBuffer.Fill->Buffer = DrawBuffer.GeneralBuffer[0]; // TODO: Remove
	}

	// Compute shader parameters
	FLOAT LightMapFactor = RenDev->OneXBlending ? 2.0f : 4.0f;

	// Buffer allocation
	if ( !RenDev->bufferId_GlobalRenderUBO )
	{
		RenDev->GlobalRenderData.ColorCorrection = ColorCorrection;
		RenDev->GlobalRenderData.LightMapFactor = LightMapFactor;
		CreateBuffer( RenDev->bufferId_GlobalRenderUBO, GL_UNIFORM_BUFFER, FGlobalRender_UBO::GLSL_Size(), nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase( GL_UNIFORM_BUFFER, FGlobalRender_UBO::UniformIndex, RenDev->bufferId_GlobalRenderUBO);
		RenDev->GlobalRenderData.BufferAll();
	}
	if ( !RenDev->bufferId_StaticBspUBO )
	{
		CreateBuffer( RenDev->bufferId_StaticBspUBO, GL_UNIFORM_BUFFER, FStaticBsp_UBO::GLSL_Size(), nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase( GL_UNIFORM_BUFFER, FStaticBsp_UBO::UniformIndex, RenDev->bufferId_StaticBspUBO);
		RenDev->StaticBspData.BufferWavyTime();
		RenDev->StaticBspData.BufferAmbientPlaneArray();
		RenDev->StaticBspData.BufferAutoPanArray();
	}

	if ( !RenDev->bufferId_TextureParamsUBO )
		RenDev->UpdateTextureParamsUBO();

	if ( !RenDev->bufferId_csTextureScaleUBO )
		RenDev->csUpdateTextureScaleUBO();

	bool UpdateGlobalRender = false;

	// Update color correction
	if ( ColorCorrection != RenDev->GlobalRenderData.ColorCorrection )
	{
		RenDev->GlobalRenderData.ColorCorrection = ColorCorrection;
		UpdateGlobalRender = true;
	}
	// Update OneXBlending state
	if ( LightMapFactor != RenDev->GlobalRenderData.LightMapFactor )
	{
		RenDev->GlobalRenderData.LightMapFactor = LightMapFactor;
		UpdateGlobalRender = true;
	}

	// UBO is global, but data is local.
	// When there's multiple viewports always update the UBO
	if ( UpdateGlobalRender || GIsEditor )
	{
		glBindBuffer(GL_UNIFORM_BUFFER, RenDev->bufferId_GlobalRenderUBO);
		RenDev->GlobalRenderData.BufferAll();
	}

	// Update LOD bias
	if ( LODBias != CurrentLODBias )
	{
		CurrentLODBias = LODBias;
		SetProgram(INDEX_NONE, nullptr);
		for ( TMapExt<FProgramID,FProgramData>::TIterator It(Programs); It; ++It)
			if ( It.Key().GetVertexType() != FGL::VertexProgram::Tile )
			{
				glUseProgram(It.Value().Program);
				SetUniform(It.Value().LodBias, LODBias);
			}
		glUseProgram(0);
	}
}

void FOpenGL3::Unlock()
{
	SetProgram(INDEX_NONE, nullptr);
}

//
// Bind the specified Textures
//
void FOpenGL3::SetTextures( FPendingTexture* Textures, BYTE NewTextureBits)
{
	guard(FOpenGL3::SetTextures);

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
//	if ( ChangedTextures[0] && UsingDervMapping && (Textures[0].RelevantPolyFlags & PF_Masked) )
//		SetVertexShaderEnv( 7, (FLOAT)ChangedTextures[0]->USize, (FLOAT)ChangedTextures[0]->VSize, 0, 1);

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

	TextureUnits.SetActive(0);

	unguard;
}


void FOpenGL3::SetProgram( FProgramID NewProgramID, void** UniformParams)
{
	guard(FOpenGL3::SetProgram);

	// Adjust ProgramID
	if ( (NewProgramID != INDEX_NONE) && !UsingDervMapping )
		NewProgramID &= ~FGL::Program::DervMapped;

	FProgramData* Program = Programs.Find(NewProgramID);
	if ( UniformParams )
		*((FProgramData**)UniformParams) = Program;

	if ( NewProgramID == ProgramID )
		return;

	// Disable and unbind shaders
	if ( NewProgramID == INDEX_NONE )
	{
		ProgramID = INDEX_NONE;
		glUseProgram(0);
		return;
	}

	bool SetUniforms = false;

	// Handle Program
	if ( !Program || !Program->Program )
	{
		FShaderWriter* VertexWriter   = nullptr;
		FShaderWriter* FragmentWriter = nullptr;
		FProgramID VertexProgramID;
		FProgramID FragmentProgramID;
		GLuint* VertexShader   = nullptr;
		GLuint* FragmentShader = nullptr;

		DWORD SpecialID = NewProgramID.GetSpecialProgram();
		switch ( SpecialID )
		{
		case FGL::SpecialProgram::None:
			VertexProgramID   = NewProgramID.GetVertexProgramID();
			FragmentProgramID = NewProgramID.GetFragmentProgramID();
			VertexWriter      = new FVertexShaderWriter;
			FragmentWriter    = new FFragmentShaderWriter;
			break;
		case FGL::SpecialProgram::FBO_Draw:
			VertexProgramID   = NewProgramID;
			FragmentProgramID = NewProgramID;
			VertexWriter      = new FVertexShaderWriterFBOblit;
			FragmentWriter    = new FFragmentShaderWriterFBOblit;
			break;
		default:
			return;
		}

		// Handle Vertex shader
		if	(	VertexProgramID != 0
			&&	(VertexShader=VertexShaders.Find(VertexProgramID)) == nullptr )
		{
			VertexWriter->Create(VertexProgramID);
			GLuint NewVertexShader = CompileShader(GL_VERTEX_SHADER, *VertexWriter->Out);
			if ( NewVertexShader )
				VertexShader = &VertexShaders.SetNoFind(VertexProgramID.GetValue(), NewVertexShader);
		}

		// Handle Fragment shader
		if	(	FragmentProgramID != 0
			&&	(FragmentShader=FragmentShaders.Find(FragmentProgramID)) == nullptr )
		{
			FragmentWriter->Create(FragmentProgramID);
			GLuint NewFragmentShader = CompileShader(GL_FRAGMENT_SHADER, *FragmentWriter->Out);
			if ( NewFragmentShader )
				FragmentShader = &FragmentShaders.SetNoFind(FragmentProgramID.GetValue(), NewFragmentShader);
		}

		if ( VertexWriter )   delete VertexWriter;
		if ( FragmentWriter ) delete FragmentWriter;

		// Handle program Linking
		if ( FragmentShader && *FragmentShader && VertexShader && *VertexShader )
		{
			FProgramData NewProgram;
			NewProgram.Program = glCreateProgram();
			glAttachShader(NewProgram.Program, *VertexShader);
			glAttachShader(NewProgram.Program, *FragmentShader);
			glLinkProgram(NewProgram.Program);

			GLint Result = 0;
			glGetProgramiv(NewProgram.Program, GL_LINK_STATUS, &Result);
			if ( !Result )
			{
				glGetProgramiv(NewProgram.Program, GL_INFO_LOG_LENGTH, &Result);
				TArray<ANSICHAR> AnsiText(Result);
				glGetProgramInfoLog(NewProgram.Program, Result, &Result, &AnsiText(0));
				debugf( NAME_Init, TEXT("OpenGLDrv: %s"), appFromAnsi(&AnsiText(0)) );
				glDeleteProgram(NewProgram.Program);
			}
			else
			{
				glDetachShader(NewProgram.Program, *VertexShader);
				glDetachShader(NewProgram.Program, *FragmentShader);
				Program = &Programs.SetNoFind( NewProgramID, NewProgram);
				if ( UniformParams )
					*((FProgramData**)UniformParams) = Program;
				SetUniforms = true;
			}
		}
		else
			return;
	}

	ProgramID = NewProgramID;

	glUseProgram(Program->Program);
	if ( SetUniforms )
	{
		bool IsDrawTile = ProgramID.GetVertexType() == FGL::VertexProgram::Tile;

		// Set all Sampler uniforms
		static ANSICHAR TextureUniformName[] = "TextureN";
		for ( INT i=0; i<8; i++)
			if ( ProgramID & (1<<i) )
			{
				TextureUniformName[7] = '0' + i;
				GLint TextureLoc = glGetUniformLocation(Program->Program, TextureUniformName);
				if ( TextureLoc != GL_INVALID_INDEX )
					glUniform1i(TextureLoc, i);
			}

		// Set uniform table with their respective defaults
		Program->AlphaTest.Location = glGetUniformLocation(Program->Program, "AlphaTest");
		SetUniform(Program->AlphaTest, 0.0f);

		Program->ColorGlobal.Location = glGetUniformLocation(Program->Program, "ColorGlobal");
		SetUniform(Program->ColorGlobal, FPlane(0,0,0,0));

		Program->ClipPlane.Location = glGetUniformLocation(Program->Program, "ClipPlane");
		SetUniform(Program->ClipPlane, FPlane(0,0,0,0));

		Program->SampleCount.Location = glGetUniformLocation(Program->Program, "SampleCount");
		SetUniform(Program->SampleCount, 0);

		Program->LodBias.Location = glGetUniformLocation(Program->Program, "LodBias");
		SetUniform(Program->LodBias, !IsDrawTile ? FOpenGLBase::LODBias : 0.0f);

		// Set uniform blocks
		GLuint BlockIndex;

		BlockIndex = glGetUniformBlockIndex(Program->Program, "GlobalRender");
		if ( BlockIndex != GL_INVALID_INDEX )
			glUniformBlockBinding(Program->Program, BlockIndex, FGlobalRender_UBO::UniformIndex);

		BlockIndex = glGetUniformBlockIndex(Program->Program, "StaticBsp");
		if ( BlockIndex != GL_INVALID_INDEX )
			glUniformBlockBinding(Program->Program, BlockIndex, FStaticBsp_UBO::UniformIndex);

		BlockIndex = glGetUniformBlockIndex(Program->Program, "TextureParams");
		if ( BlockIndex != GL_INVALID_INDEX )
			glUniformBlockBinding(Program->Program, BlockIndex, FTextureParams_UBO::UniformIndex);

		BlockIndex = glGetUniformBlockIndex(Program->Program, "ComplexSurfaceScale");
		if ( BlockIndex != GL_INVALID_INDEX )
			glUniformBlockBinding(Program->Program, BlockIndex, FComplexSurfaceScale_UBO::UniformIndex);
	}

	unguard;
}


void FOpenGL3::FlushPrograms()
{
	// Safely delete shaders
	for ( TMap<FProgramID,GLuint>::TIterator It(VertexShaders); It; ++It)
		if ( glIsShader(It.Value()) )
			glDeleteShader(It.Value());
	for ( TMap<FProgramID,GLuint>::TIterator It(GeometryShaders); It; ++It)
		if ( glIsShader(It.Value()) )
			glDeleteShader(It.Value());
	for ( TMap<FProgramID,GLuint>::TIterator It(FragmentShaders); It; ++It)
		if ( glIsShader(It.Value()) )
			glDeleteShader(It.Value());
	VertexShaders.Empty();
	GeometryShaders.Empty();
	FragmentShaders.Empty();

	// Safely delete programs
	for ( TMap<FProgramID,FProgramData>::TIterator It(Programs); It; ++It)
		if ( glIsProgram(It.Value().Program) )
			glDeleteProgram(It.Value().Program);
	Programs.Empty();
}


GLuint FOpenGL3::CompileShader( GLenum Target, const ANSICHAR* Source)
{
	guard(FOpenGL3::CompileShader);

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
		debugf( NAME_Init, TEXT("OpenGLDrv: %s"), appFromAnsi(&AnsiText(0)) );
		glDeleteShader(NewShader);
		NewShader = 0;
		appErrorf( TEXT("Unable to compile shader: \n%s"), appFromAnsi(Source) );
	}

	return NewShader;
	unguard;
}
