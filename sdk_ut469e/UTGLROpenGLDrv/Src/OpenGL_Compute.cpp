/*=============================================================================
	OpenGL_Compute.cpp: OpenGL compute pipelines.

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

#include "OpenGLDrv.h"
#include "UTGLROpenGL.h"
#include "OpenGL_ShaderWriter.h"

/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

FOpenGLBase::FCompute FOpenGLBase::Compute{};

static const ANSICHAR* GLSL_Version()
{
	switch (UOpenGLRenderDevice::GetContextType())
	{
	case (CONTEXTTYPE_GL3):
		return "#version 430 core";
	case (CONTEXTTYPE_GL4):
		return "#version 450 core";
	case (CONTEXTTYPE_GLES):
		return "#version 310 es";
	}
	return "";
}

static bool CreateAndCompileShader( const ANSICHAR* Src, GLuint& OutShader, GLuint& OutProgram)
{
	GLuint Shader = FOpenGLBase::CompileShader(GL_COMPUTE_SHADER, Src);
	if ( Shader )
	{
		GLuint Program = FOpenGLBase::glCreateProgram();
		if ( Program )
		{
			FOpenGLBase::glAttachShader(Program, Shader);
			FOpenGLBase::glLinkProgram(Program);
			FOpenGLBase::glDetachShader(Program, Shader);

			GLint Result = 0;
			FOpenGLBase::glGetProgramiv(Program, GL_LINK_STATUS, &Result);
			if ( Result )
			{
				OutShader = Shader;
				OutProgram = Program;
				return true;
			}
			else
			{
				FOpenGLBase::glGetProgramiv(Program, GL_INFO_LOG_LENGTH, &Result);
				TArray<ANSICHAR> AnsiText(Result);
				FOpenGLBase::glGetProgramInfoLog(Program, Result, &Result, &AnsiText(0));
				debugf(NAME_Init, TEXT("OpenGLDrv: %s"), appFromAnsi(&AnsiText(0)) );
				FOpenGLBase::glDeleteProgram(Program);
			}
		}
		else
			FOpenGLBase::glDeleteShader(Shader);
	}
	return false;
}

void FOpenGLBase::SetComputeQuadExpansion( GLuint InBuffer, GLuint OutBuffer)
{
	guard(FOpenGLBase::SetComputeQuadExpansion);

	BindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, InBuffer);
	BindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, OutBuffer);

	if ( Compute.Programs.QuadExpansion )
	{
		glUseProgram(Compute.Programs.QuadExpansion);
		return;
	}

	FCharWriter Out;

	// Write header
	Out <<
		"/*=============================================================================" END_LINE
		"        Compute Quad Expansion"                                                  END_LINE
		"    Dynamically generated shader for OpenGLDrv."                                 END_LINE
		"    See OpenGL_Compute.cpp for more information"                                 END_LINE
	    "=============================================================================*/" END_LINE;
	Out << END_LINE;

	// Write target GLSL version
	Out << GLSL_Version() << END_LINE;
	Out << END_LINE;

	// Precision statements for GLES
	if ( FShaderWriter::IsES() )
	{
		Out << "precision highp float;" END_LINE;
		Out << END_LINE;
	}

	// Definitions
	Out <<
R"(// Buffer format
struct QuadIn
{
	float X, Y, XL, YL;
	float U, V, UL, VL;
	float Z;
	float UVScale;
	uint PackedColor;
	uint PackedLayers;
};

struct QuadOut
{
	float X, Y, Z;
	uint PackedColor;
	float U, V;
	uint PackedLayers;
};

layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 6) readonly buffer qIn
{
	QuadIn In[];
}; 

layout(std430, binding = 7) writeonly buffer qOut
{
	QuadOut Out[];
}; 

)";

	// Program
	Out <<
R"(
void main()
{
	uint i = gl_GlobalInvocationID.x;
	uint j = i * 6u;

	float X1 = In[i].X;
	float X2 = X1 + In[i].XL;
	float Y1 = In[i].Y;
	float Y2 = Y1 + In[i].YL;

	float Scale = In[i].UVScale;
	float UMult = 1.0f / Scale; // Texture size applied in vertex shader
	float VMult = 1.0f / Scale; // Texture size applied in vertex shader
	float U1 =      (In[i].U)  * UMult;
	float U2 = U1 + (In[i].UL) * UMult;
	float V1 =      (In[i].V)  * VMult;
	float V2 = V1 + (In[i].VL) * VMult;
	float Z  = In[i].Z;

	uint PackedColor  = In[i].PackedColor;
	uint PackedLayers = In[i].PackedLayers;

	// qv1
	Out[j + 0u].X            = X1;
	Out[j + 0u].Y            = Y1;
	Out[j + 0u].Z            = Z;
	Out[j + 0u].PackedColor  = PackedColor;
	Out[j + 0u].U            = U1;
	Out[j + 0u].V            = V1;
	Out[j + 0u].PackedLayers = PackedLayers;

	// qv2
	Out[j + 1u].X            = X2;
	Out[j + 1u].Y            = Y1;
	Out[j + 1u].Z            = Z;
	Out[j + 1u].PackedColor  = PackedColor;
	Out[j + 1u].U            = U2;
	Out[j + 1u].V            = V1;
	Out[j + 1u].PackedLayers = PackedLayers;

	// qv3
	Out[j + 2u].X            = X2;
	Out[j + 2u].Y            = Y2;
	Out[j + 2u].Z            = Z;
	Out[j + 2u].PackedColor  = PackedColor;
	Out[j + 2u].U            = U2;
	Out[j + 2u].V            = V2;
	Out[j + 2u].PackedLayers = PackedLayers;

	// qv3
	Out[j + 3u].X            = X2;
	Out[j + 3u].Y            = Y2;
	Out[j + 3u].Z            = Z;
	Out[j + 3u].PackedColor  = PackedColor;
	Out[j + 3u].U            = U2;
	Out[j + 3u].V            = V2;
	Out[j + 3u].PackedLayers = PackedLayers;

	// qv4
	Out[j + 4u].X            = X1;
	Out[j + 4u].Y            = Y2;
	Out[j + 4u].Z            = Z;
	Out[j + 4u].PackedColor  = PackedColor;
	Out[j + 4u].U            = U1;
	Out[j + 4u].V            = V2;
	Out[j + 4u].PackedLayers = PackedLayers;

	// qv1
	Out[j + 5u].X            = X1;
	Out[j + 5u].Y            = Y1;
	Out[j + 5u].Z            = Z;
	Out[j + 5u].PackedColor  = PackedColor;
	Out[j + 5u].U            = U1;
	Out[j + 5u].V            = V1;
	Out[j + 5u].PackedLayers = PackedLayers;
}
)";

	verify(CreateAndCompileShader(*Out, Compute.Shaders.QuadExpansion, Compute.Programs.QuadExpansion));
	glUseProgram(Compute.Programs.QuadExpansion);

	unguard;
}

void FOpenGLBase::SetComputePaletteTextureUpdate(GLuint Image, GLuint Layer, GLuint TexBuffer, GLuint PalBuffer, GLsizei PalOffset, bool Masked)
{
	guard(FOpenGLBase::SetComputePaletteTextureUpdate);

	// TODO: LAYER-IZE?
	// TODO: TRACK STATE?
	glBindImageTexture(0, Image, 0, GL_FALSE, Layer, GL_WRITE_ONLY, GL_RGBA8); // TODO: STATE CHECK
	BindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, TexBuffer);
	BindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, PalBuffer);

	if ( Compute.Programs.PaletteTextureUpdate )
	{
		glUseProgram(Compute.Programs.PaletteTextureUpdate);
		glUniform1ui(1, PalOffset); // TODO: STATE CHECK
		return;
	}

	FCharWriter Out;

	// Write header
	Out <<
		"/*=============================================================================" END_LINE
		"        Compute Palettized Texture Update"                                       END_LINE
		"    Dynamically generated shader for OpenGLDrv."                                 END_LINE
		"    See OpenGL_Compute.cpp for more information"                                 END_LINE
		"=============================================================================*/" END_LINE;
	Out << END_LINE;

	// Write target GLSL version
	Out << GLSL_Version() << END_LINE;
	Out << END_LINE;

	// Precision statements for GLES
	if ( FShaderWriter::IsES() )
	{
		Out << "precision highp image2D;" END_LINE;
		Out << END_LINE;
	}

	// Definitions
	Out <<
R"(
layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Bindings
layout(binding = 0, rgba8) uniform writeonly image2D OutImage;

layout(std430, binding = 6) readonly buffer qTex
{
	uint Tex[];
}; 

layout(std430, binding = 7) readonly buffer qPal
{
	uint Pal[];
};

// Uniforms
layout(location = 1) uniform uint PaletteOffset;

)";

		// Program
	Out <<
R"(
void main()
{
	uint ix = gl_GlobalInvocationID.x;
	uint iy = gl_GlobalInvocationID.y;

	uvec2 Extent = uvec2(imageSize(OutImage));

	// Do not go out of bounds
	if ( ix >= Extent.x || iy >= Extent.y )
		return;

	vec4 UnpackedPixel = unpackUnorm4x8(Tex[(ix + iy * Extent.x) / 4u]) * 255.1;
	uint Pixels[4];
	Pixels[0] = uint(UnpackedPixel.x);
	Pixels[1] = uint(UnpackedPixel.y);
	Pixels[2] = uint(UnpackedPixel.z);
	Pixels[3] = uint(UnpackedPixel.w);

	uint iPixel = ix - (ix / 4u) * 4u;
	uint iPal = PaletteOffset / 4u + Pixels[iPixel];

	vec4 Color = unpackUnorm4x8(Pal[iPal]);
	imageStore(OutImage, ivec2(ix, iy), Color);
}
)";

	verify(CreateAndCompileShader(*Out, Compute.Shaders.PaletteTextureUpdate, Compute.Programs.PaletteTextureUpdate));
	glUseProgram(Compute.Programs.PaletteTextureUpdate);

	unguard;
}

void FOpenGLBase::ExitCompute()
{
	static constexpr INT Num = 2;

	GLuint* Programs = (GLuint*)&Compute.Programs;
	GLuint* Shaders = (GLuint*)&Compute.Shaders;

	for ( INT i=0; i<Num; i++)
		if ( Programs[i] )
		{
			glDeleteProgram(Programs[i]);
			Programs[i] = 0;
		}

	for ( INT i=0; i<Num; i++)
		if ( Shaders[i] )
		{
			glDeleteShader(Shaders[i]);
			Shaders[i] = 0;
		}
}
