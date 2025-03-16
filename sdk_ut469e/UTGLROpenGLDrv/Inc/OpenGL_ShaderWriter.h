/*=============================================================================
	OpenGL_ShaderWriter.h
	
	GLSL shader writers.

	Texture Samplers:
	- Texture0
	- Texture1
	- Texture2
	- Texture3
	- Texture4

	Vertex shader Inputs:
	-  0: InVertex
	-  1: InColor0
	-  2: InColor1
	-  3: InTexCoords0
	-  4: InTexCoords1
	-  5: InTexCoords2
	-  6: InNormal
	-  7: InTextureLayers
	-  8: InLightInfo
	-  9: InVertexParam0 (Complex:InTexturePan)
	- 10: InVertexParam1 (Complex:ScaleUniformIndex)

	Fragment shader Inputs:
	- FragColor0
	- FragColor1
	- FragTexCoords0
	- FragTexCoords1
	- FragTexCoords2
	- FragTexCoords3
	- FragTexCoords4
	- FragZoneID
	- FragDistance

	Final outputs:
	- FinalColor

	// IMPORTANT: See if ARB_conservative_depth negatively impacts NoNearZ

	// TODO: Uniform should contain USize/1, VSize/1, ?, BindlessID
	// TODO: Each TexIndex comes with a Scale modifier to apply on top of the Uniform stuff

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

//
// Scoped block
// Adds a newline at the end if a block of text was written
//
struct FScopedTextBlock
{
	FCharWriter& Writer;
	INT InitialSize;

	FScopedTextBlock() = delete;
	FScopedTextBlock(FCharWriter& InWriter)
		: Writer(InWriter)
		, InitialSize(InWriter.Data.Num())
	{}

	~FScopedTextBlock()
	{
		EndBlock();
	}

	void EndBlock()
	{
		if ( Writer.Data.Num() != InitialSize )
		{
			InitialSize = Writer.Data.Num();
			Writer << END_LINE;
		}
	}
};

//
// GLSL shader writer base class
//
class FShaderWriter
{
public:
	FProgramID Flags;
	FCharWriter Out;

	FShaderWriter()
	{}

	virtual ~FShaderWriter() {}

	void Create( FProgramID NewFlags)
	{
		Flags = NewFlags;

		Setup();

		FScopedTextBlock TextBlock(Out);
		WriteHeader(Out);   TextBlock.EndBlock();
		WriteInputs(Out);   TextBlock.EndBlock();
		WriteOutputs(Out);  TextBlock.EndBlock();
		WriteProgram(Out);  TextBlock.EndBlock();
	}

	static void WriteVersion( FCharWriter& Out);
	static bool IsES();
	static const ANSICHAR* xyzw(INT Coord);

protected:
	virtual const ANSICHAR* ShaderName() {	return "Generic Shader"; }

	virtual void Setup() {}
	virtual void WriteHeader( FCharWriter& Out);
	virtual void WriteExtensions( FCharWriter& Out);
	virtual void WriteInputs( FCharWriter& Out)  {}
	virtual void WriteOutputs( FCharWriter& Out) {}
	virtual void WriteProgram( FCharWriter& Out) {}

	bool UseSingleClipPlane() const;
	virtual bool UseMultiSampling() const        { return false; }
	virtual bool UseLodBias() const              { return false; }
};


//
// GLSL vertex shader writer
//
class FVertexShaderWriter : public FShaderWriter
{
	bool UseTextureUVPan; // Texture UVs need to be scaled, Detail and Macro depend on Base

protected:
	const ANSICHAR* ShaderName() {	return "Vertex Shader [v2f]"; }

	void Setup();
	void WriteInputs( FCharWriter& Out);
	void WriteExtensions( FCharWriter& Out);
	void WriteOutputs( FCharWriter& Out);
	void WriteProgram( FCharWriter& Out);
};


//
// GLSL fragment shader writer
//
class FFragmentShaderWriter : public FShaderWriter
{
public:
	~FFragmentShaderWriter() {}

protected:
	const ANSICHAR* ShaderName() {	return "Fragment Shader"; }

	void Setup() {}
	void WriteExtensions( FCharWriter& Out);
	void WriteInputs( FCharWriter& Out);
	void WriteOutputs( FCharWriter& Out);
	void WriteProgram( FCharWriter& Out);

	bool UseLodBias() const                      { return true; }

	// Helpers
	void WriteBaseTexture( FCharWriter& Out);
	void WriteDetailTexture( FCharWriter& Out);
	void WriteMacroTexture( FCharWriter& Out);
	void WriteLightMap( FCharWriter& Out);
	void WriteFogMap( FCharWriter& Out);
};


//
// FBO blit
//
class FShaderWriterFBOblit : public FShaderWriter
{
protected:
	FCharWriter CompositionWriter;

	bool UseMultiSampling() const override;

	const ANSICHAR* samplerType() const;
	const ANSICHAR* textureSize( const ANSICHAR* SamplerName);
};

class FVertexShaderWriterFBOblit : public FShaderWriterFBOblit
{
protected:
	const ANSICHAR* ShaderName() {  return "Vertex Shader [FBO blit]"; }

	void WriteInputs( FCharWriter& Out);
	void WriteOutputs( FCharWriter& Out);
	void WriteProgram( FCharWriter& Out);
};

class FFragmentShaderWriterFBOblit : public FShaderWriterFBOblit
{
protected:
	const ANSICHAR* ShaderName() {  return "Fragment Shader [FBO blit]"; }

	void WriteInputs( FCharWriter& Out);
	void WriteOutputs( FCharWriter& Out);
	void WriteProgram( FCharWriter& Out);
};


/*-----------------------------------------------------------------------------
	GLSL globals.
-----------------------------------------------------------------------------*/

inline void FShaderWriter::WriteVersion( FCharWriter& Out)
{
	const ANSICHAR* VersionString = "";
	switch (UOpenGLRenderDevice::GetContextType())
	{
	case (CONTEXTTYPE_GL3):
		VersionString = "330 core";
		break;
	case (CONTEXTTYPE_GL4):
		VersionString = "450 core";
		break;
	case (CONTEXTTYPE_GLES):
		VersionString = "310 es";
		break;
	}
	Out << "#version " << VersionString << END_LINE;
	Out << END_LINE;
}

static const char* xyzw_array[4] = { "x", "y", "z", "w" };
inline const ANSICHAR* FShaderWriter::xyzw( INT Coord)
{
	return xyzw_array[Coord];
}

inline bool FShaderWriter::IsES()
{
	return UOpenGLRenderDevice::GetContextType() == CONTEXTTYPE_GLES;
}


/*-----------------------------------------------------------------------------
	GLSL traits.
-----------------------------------------------------------------------------*/

//
// Single non-batched uniform clip plane
//
inline bool FShaderWriter::UseSingleClipPlane() const
{
	return	FOpenGLBase::SupportsClipCullDistance 
		&&	(	UOpenGLRenderDevice::GetContextType() == CONTEXTTYPE_GL3	
			||	UOpenGLRenderDevice::GetContextType() == CONTEXTTYPE_GLES	)
		&&	FGL::VertexProgram::UsesClipPlane(Flags.GetVertexType());
}


/*-----------------------------------------------------------------------------
	GLSL utils.
-----------------------------------------------------------------------------*/

static const ANSICHAR* GLSL_FUNC_MAX3 = 
R"(// Returns maximum of first 3 components
float max3( vec3 v)
{
    return max(max(v.x, v.y), v.z);
}
float max3( vec4 v)
{
    return max(max(v.x, v.y), v.z);
}
)";


static const ANSICHAR* GLSL_FUNC_SQUARE_F =
R"(// Returns square of argument
float square_f( float f)
{
    return f*f;
}
)";

static const ANSICHAR* GLSL_FUNC_TEXTURE_MULTISAMPLE =
R"(// Performs texture multisampling operation
vec4 textureMultisample( sampler2DMS Sampler, ivec2 Coords)
{
    vec4 color = vec4(0.0);
    for ( int i=0; i<SampleCount; i++)
        color += texelFetch(Sampler, Coords, i);
    color /= float(SampleCount);
    return color;
}
)";


/*-----------------------------------------------------------------------------
	GLSL shader writer.
-----------------------------------------------------------------------------*/

inline void FShaderWriter::WriteExtensions( FCharWriter& Out)
{
	if ( IsES() )
	{
		if ( FOpenGLBase::SupportsTBO )
		{
			Out << "#ifdef GL_OES_texture_buffer"                END_LINE;
			Out << " #extension GL_OES_texture_buffer : require" END_LINE;
			Out << "#else"                                       END_LINE;
			Out << " #extension GL_EXT_texture_buffer : require" END_LINE;
			Out << "#endif"                                      END_LINE;
		}
	}
}

inline void FShaderWriter::WriteHeader( FCharWriter& Out)
{
	check(FOpenGLBase::ActiveInstance->RenDev);

	Out << "/*=============================================================================" END_LINE
	       "        " << ShaderName() <<                                                     END_LINE
	       "    Dynamically generated shader for OpenGLDrv."                                 END_LINE
	       "    See OpenGL_ShaderWriter.h for more information"                              END_LINE
		                                                                                     END_LINE
	       "    This shader was generated with the following features:"                      END_LINE;
	for ( INT i=0; i<8; i++)
		if ( Flags & (1 << i) )
			Out << "    * Texture" << i <<                                                   END_LINE;
	if ( Flags & FGL::Program::Color0 )
		Out <<     "    * Primary Color"                                                     END_LINE;
	if ( Flags & FGL::Program::Color1 )
		Out <<     "    * Secondary Color"                                                   END_LINE;
	if ( Flags & FGL::Program::ColorGlobal )
		Out <<     "    * Global Color"                                                      END_LINE;
	if ( Flags & FGL::Program::AlphaTest )
		Out <<     "    * Alpha Test"                                                        END_LINE;
	if ( Flags & FGL::Program::AlphaHack )
		Out <<     "    * Translucent to premultiplied alpha hack"                           END_LINE;
	if ( Flags & FGL::Program::NoNearZ )
		Out <<     "    * Near Z hack"                                                       END_LINE;
	Out << "=============================================================================*/" END_LINE;

	WriteVersion(Out);

	{
		FScopedTextBlock TextBlock(Out);
		WriteExtensions(Out);
	}

	// Precision statements for GLES
	if ( IsES() )
	{
		Out << "precision highp float;" END_LINE;
		Out << "precision highp sampler2DArray;" END_LINE;
		if ( FOpenGLBase::SupportsTBO )
			Out << "precision highp samplerBuffer;" END_LINE;
		Out << END_LINE;
	}

	// Definitions
	Out << "#define DETAILMAX " << FOpenGLBase::ActiveInstance->RenDev->DetailMax << END_LINE;
	Out << "#define TEXINFOMAX " << UOpenGLRenderDevice::TexturePool.UniformQueue.Size() << END_LINE;
	Out << "#define CSSCALEMAX " << FComplexSurfaceScale_UBO::ArraySize() << END_LINE;
	Out << END_LINE;

	// Uniforms
	if ( Flags & FGL::Program::AlphaTest )
		Out << "uniform float AlphaTest;"     END_LINE;
	if ( Flags & FGL::Program::ColorGlobal )
		Out << "uniform vec4 ColorGlobal;"    END_LINE;
	if ( UseSingleClipPlane() )
		Out << "uniform vec4 ClipPlane;"      END_LINE;
	if ( UseMultiSampling() )
		Out << "uniform int SampleCount;"     END_LINE;
	if ( UseLodBias() )
		Out << "uniform float LodBias;"       END_LINE;

	Out << END_LINE;

	// Uniform blocks
	Out << FGlobalRender_UBO::GLSL_Struct();
	Out << FStaticBsp_UBO::GLSL_Struct();
	Out << FTextureParams_UBO::GLSL_Struct();
	Out << FComplexSurfaceScale_UBO::GLSL_Struct();
}


/*-----------------------------------------------------------------------------
	GLSL vertex shader writer.
-----------------------------------------------------------------------------*/

inline void FVertexShaderWriter::Setup()
{
	UseTextureUVPan = false;

	switch ( Flags.GetVertexType() )
	{
	case FGL::VertexProgram::Identity: // Pass coordinates without transformations
		return;
	case FGL::VertexProgram::ComplexSurfaceVBO:
	case FGL::VertexProgram::ComplexSurface:
		UseTextureUVPan   = true;
	default:
		break;
	}
}

inline void FVertexShaderWriter::WriteExtensions( FCharWriter& Out)
{
	FShaderWriter::WriteExtensions(Out);

	// In OpenGL ES we can't use gl_ClipDistance without this extension
	if ( IsES() )
	{
		if ( FOpenGLBase::SupportsClipCullDistance )
			Out << "#extension GL_EXT_clip_cull_distance : require" END_LINE;
	}
}

inline void FVertexShaderWriter::WriteInputs( FCharWriter& Out)
{
	DWORD VertexType = Flags.GetVertexType();

	Out << "// Inputs, generated by FVertexShaderWriter::WriteInputs" END_LINE;
	if ( Flags & FGL::Program::Texture0 )  Out << "uniform sampler2DArray Texture0;" END_LINE;
	if ( Flags & FGL::Program::Texture1 )  Out << "uniform sampler2DArray Texture1;" END_LINE;
	if ( Flags & FGL::Program::Texture2 )  Out << "uniform sampler2DArray Texture2;" END_LINE;
	if ( Flags & FGL::Program::Texture3 )  Out << "uniform sampler2DArray Texture3;" END_LINE;
	if ( Flags & FGL::Program::Texture4 )  Out << "uniform sampler2DArray Texture4;" END_LINE;
	if ( VertexType == FGL::VertexProgram::ComplexSurfaceVBO )
	{
		Out << "uniform samplerBuffer Texture8;" END_LINE;
	}

	Out <<         "layout (location=0) in vec3 InVertex;"          END_LINE;
	if ( Flags & FGL::Program::Color0 )
		Out <<     "layout (location=1) in vec4 InColor0;"          END_LINE;
	if ( Flags & FGL::Program::Color1 )
		Out <<     "layout (location=2) in vec4 InColor1;"          END_LINE;

	if ( VertexType == FGL::VertexProgram::ComplexSurface )
	{
			Out << "layout (location=3) in vec2 InTexCoords0;"      END_LINE;
		if ( Flags & FGL::Program::Texture3 ) // Light
			Out << "layout (location=4) in vec2 InTexCoords1;"      END_LINE;
		if ( Flags & FGL::Program::Texture4 ) // Fog
			Out << "layout (location=5) in vec2 InTexCoords2;"      END_LINE;
	}
	else if ( VertexType == FGL::VertexProgram::ComplexSurfaceVBO )
	{
		Out << "layout (location=3) in vec2 InTexCoords0;"          END_LINE;
		Out << "layout (location=4) in vec4 InTexCoords1;"          END_LINE;
		Out << "layout (location=5) in vec3 InTexCoords2;"          END_LINE;
	}
	else
	{
		if ( Flags & FGL::Program::Texture0 )
			Out << "layout (location=3) in vec2 InTexCoords0;"      END_LINE;
		if ( Flags & FGL::Program::Texture1 )
			Out << "layout (location=4) in vec2 InTexCoords1;"      END_LINE;
		if ( Flags & FGL::Program::Texture2 )
			Out << "layout (location=5) in vec2 InTexCoords2;"      END_LINE;
	}
	//Out << "layout (location=6) in vec3 InNormal;" END_LINE;

	if ( Flags & (FGL::Program::TextureUnlitMask)  )
		Out << "layout (location=7) in uvec4 InTextureLayers;"       END_LINE;

	if ( Flags & (FGL::Program::TextureLightMask) )
		Out <<     "layout (location=8) in uvec4 InLightInfo;"      END_LINE;

	if ( Flags & FGL::Program::VertexParam0 )
	{
		DWORD VertexType = Flags.GetVertexType();
		if ( (VertexType == FGL::VertexProgram::ComplexSurface) || (VertexType == FGL::VertexProgram::ComplexSurfaceVBO) )
			Out << "layout (location=9) in vec2 InTexturePan;"      END_LINE;
	}

	if ( UseTextureUVPan )
		Out << "layout (location=10) in uvec2 InUniformIndex;"      END_LINE;
}

inline void FVertexShaderWriter::WriteOutputs( FCharWriter& Out)
{
	Out << "// Outputs, generated by FVertexShaderWriter::WriteOutputs" END_LINE;

	if ( IsES() && FOpenGLBase::SupportsClipCullDistance )
		Out << "out float gl_ClipDistance[1];" END_LINE;

	if ( Flags & FGL::Program::Color0 )    Out << "out vec4 FragColor0;" END_LINE;
	if ( Flags & FGL::Program::Color1 )    Out << "out vec4 FragColor1;" END_LINE;
	if ( Flags & FGL::Program::Texture0 )  Out << "centroid out vec3 FragTexCoords0;" END_LINE;
	if ( Flags & FGL::Program::Texture1 )  Out << "out vec3 FragTexCoords1;" END_LINE;
	if ( Flags & FGL::Program::Texture2 )  Out << "out vec3 FragTexCoords2;" END_LINE;
	if ( Flags & FGL::Program::Texture3 )  Out << "out vec3 FragTexCoords3;" END_LINE;
	if ( Flags & FGL::Program::Texture4 )  Out << "out vec3 FragTexCoords4;" END_LINE;
	if ( Flags & FGL::Program::ZoneLight ) Out << "flat out int FragZoneID;" END_LINE;
	if ( Flags & FGL::Program::Texture1 )  Out << "out float FragDistance;" END_LINE;
}

inline void FVertexShaderWriter::WriteProgram( FCharWriter& Out)
{
	Out << "// Program, generated by FVertexShaderWriter::WriteProgram" END_LINE;

	Out <<         "void main()"                        END_LINE
	               "{"                                  END_LINE;


	Out <<         "    // Transform to view space."    END_LINE;
	DWORD VertexType = Flags.GetVertexType();
	if ( VertexType == FGL::VertexProgram::Identity )
	{
		Out <<     "    gl_Position = vec4(InVertex, 1.0);"        END_LINE;
	}
	else if ( VertexType == FGL::VertexProgram::Tile )
	{
		Out <<     "    vec4 InVertex2 = vec4(InVertex * TileVector.xyz, 1.0);" END_LINE
		           "    InVertex2.xy *= mix(InVertex2.z, 1.0, TileVector.w);" END_LINE
		           "    gl_Position = ProjectionMatrix * InVertex2;" END_LINE;
	}
	else if ( VertexType == FGL::VertexProgram::ComplexSurfaceVBO )
	{
//		Out <<     "    vec3 T0 = InVertex - WorldCoordsOrigin;" END_LINE
//		           "    vec3 InVertex2 = vec3( dot(T0,WorldCoordsXAxis), dot(T0,WorldCoordsYAxis), dot(T0,WorldCoordsZAxis) );" END_LINE
//		           "    gl_Position = ProjectionMatrix * vec4(InVertex2, 1.0);" END_LINE;

		// ALT MODE USES TRANSFORMED POINTS
		Out <<     "    gl_Position = ProjectionMatrix * vec4(InVertex, 1.0);" END_LINE;
		if ( UseSingleClipPlane() )
			Out << "    gl_ClipDistance[0] = dot(InVertex.xyz, ClipPlane.xyz) - ClipPlane.w;" END_LINE;
	}
	else
	{
		Out <<     "    gl_Position = ProjectionMatrix * vec4(InVertex, 1.0);" END_LINE;
		if ( UseSingleClipPlane() )
			Out << "    gl_ClipDistance[0] = dot(InVertex.xyz, ClipPlane.xyz) - ClipPlane.w;" END_LINE;
	}
	Out << END_LINE;

	{
		FScopedTextBlock ColorBlock(Out);
		if ( Flags & FGL::Program::Color0 )
			Out <<     "    FragColor0 = InColor0;"         END_LINE;
		if ( Flags & FGL::Program::Color1 )
			Out <<     "    FragColor1 = InColor1;"         END_LINE;
	}

	if ( (VertexType == FGL::VertexProgram::ComplexSurface) || (VertexType == FGL::VertexProgram::ComplexSurfaceVBO) )
	{
		FScopedTextBlock CoordScaleBlock(Out);
		static_assert(1 & FGL::Program::TextureUnlitMask, "Ensure Texture programs start on the first bit!");
		BYTE CoordBits = Flags & FGL::Program::TextureUnlitMask;
		if ( CoordBits )
		{
			Out     << "    // Get texture UV coordinate scales for BSP surfaces"             END_LINE;
			Out     << "    vec4 Scales = csScale[InUniformIndex.x];"                         END_LINE;
			//vec2 CoordScaleN = vec2(textureSize(TextureN, 0).xy) * Scales.XN;
			for ( INT i=0; i<3; i++)
				if ( CoordBits & (1 << i) )
					Out << "    vec2 CoordScale" << i << " = vec2(textureSize(Texture" << i << ", 0).xy) * Scales." << xyzw(i) << ";" END_LINE;
		}
	}

	if ( VertexType == FGL::VertexProgram::ComplexSurface )
	{
		if ( Flags & (FGL::Program::Texture0|FGL::Program::Texture1|FGL::Program::Texture2) )
		{
			Out << "    vec2 BaseCoords = InTexCoords0;" END_LINE;
			if ( Flags & FGL::Program::Texture0 )
			{
				if ( Flags & FGL::Program::VertexParam0 )
					Out << "    FragTexCoords0.xy = (BaseCoords - InTexturePan) / CoordScale0;" END_LINE;
				else
					Out << "    FragTexCoords0.xy = BaseCoords / CoordScale0;" END_LINE;
			}
			if ( Flags & FGL::Program::Texture1 )
				Out << "    FragTexCoords1.xy = BaseCoords / CoordScale1;" END_LINE;
			if ( Flags & FGL::Program::Texture2 )
				Out << "    FragTexCoords2.xy = BaseCoords / CoordScale2;" END_LINE;
		}
		if ( Flags & FGL::Program::Texture3 )
			Out << "    FragTexCoords3.xy = InTexCoords1;"  END_LINE;
		if ( Flags & FGL::Program::Texture4 )
			Out << "    FragTexCoords4.xy = InTexCoords2;"  END_LINE;
		if ( Flags & FGL::Program::Texture1 )
			Out << "    FragDistance = gl_Position.z;"      END_LINE;
	}
	else if ( VertexType == FGL::VertexProgram::ComplexSurfaceVBO )
	{	// UNUSED, KEPT FOR FUTURE MODIFICATIONS
/*		if ( Flags & (FGL::Program::Texture0|FGL::Program::Texture1|FGL::Program::Texture2) )
		{
			Out << "    vec2 BaseCoords = InTexCoords0;"    END_LINE;
			Out << "    vec2 WavyPan    = WavyTime.xy * InTexCoords2.z;" END_LINE;
			Out << "    vec2 AutoUVPan  = ZoneAutoPan[InZoneLight.x] * InTexCoords2.xy;" END_LINE;
			if ( Flags & FGL::Program::Texture0 )
				Out << "    FragTexCoords0.xy = (BaseCoords + InTexturePan + WavyPan + AutoUVPan) / (Scales.x * textureSize(Texture0, 0).xy);" END_LINE;
			if ( Flags & FGL::Program::Texture1 )
				Out << "    FragTexCoords1.xy = BaseCoords / (Scales.y * textureSize(Texture1, 0).xy);" END_LINE;
			if ( Flags & FGL::Program::Texture2 )
				Out << "    FragTexCoords2.xy = BaseCoords / (Scales.z * textureSize(Texture2, 0).xy);" END_LINE;
		}
		if ( Flags & FGL::Program::Texture3 )
			Out << "    FragTexCoords3 = mix(InTexCoords1.xy, InTexCoords1.zw, float(InZoneLight.y));" END_LINE;
		if ( Flags & FGL::Program::Texture4 )
			Out << "    FragTexCoords4 = mix(InTexCoords1.xy, InTexCoords1.zw, float(InZoneLight.z));" END_LINE;
		if ( Flags & FGL::Program::Texture1 )
			Out << "    FragDistance = gl_Position.z;"      END_LINE;*/

		// Alt mode will sample from a buffer texture
		Out << "    vec2 BaseCoords = InTexCoords0;" END_LINE;

		if ( Flags & FGL::Program::Texture3 )
			Out << "    vec4 LightMapData = texelFetch(Texture8, int(InUniformIndex.y + 65536u * (InLightInfo.w & 1u)));" END_LINE;
		if ( Flags & FGL::Program::Texture4 )
			Out << "    vec4 FogMapData = texelFetch(Texture8, int(InUniformIndex.y + 65536u * (InLightInfo.w / 2u)));" END_LINE;

		if ( Flags & FGL::Program::Texture0 )
			Out << "    FragTexCoords0.xy = (BaseCoords - InTexturePan) / CoordScale0;" END_LINE;
		if ( Flags & FGL::Program::Texture1 )
			Out << "    FragTexCoords1.xy = (BaseCoords) / CoordScale1;" END_LINE;
		if ( Flags & FGL::Program::Texture2 )
			Out << "    FragTexCoords2.xy = (BaseCoords) / CoordScale2;" END_LINE;
		if ( Flags & FGL::Program::Texture3 )
			Out << "    FragTexCoords3.xy = (BaseCoords - LightMapData.xy + LightMapData.zw * 0.5) / (LightMapData.zw * vec2(textureSize(Texture3, 0).xy));" END_LINE;
		if ( Flags & FGL::Program::Texture4 )
			Out << "    FragTexCoords4.xy = (BaseCoords - FogMapData.xy + FogMapData.zw * 0.5) / (FogMapData.zw * vec2(textureSize(Texture4, 0).xy));" END_LINE;
		if ( Flags & FGL::Program::Texture1 )
			Out << "    FragDistance = gl_Position.z;"      END_LINE;
	}
	else if ( VertexType == FGL::VertexProgram::Tile )
	{
		if ( Flags & FGL::Program::Texture0 )
			Out << "    FragTexCoords0.xy = (InTexCoords0) / vec2(textureSize(Texture0, 0).xy);" END_LINE;
	}
	else
	{
		if ( Flags & FGL::Program::Texture0 ) // TODO: Pass TextureIndex for array layer!
			Out << "    FragTexCoords0 = vec3(InTexCoords0, 0.0);" END_LINE;
	}

	if ( Flags & FGL::Program::Texture0 )
		Out << "    FragTexCoords0.z = float(InTextureLayers.x);" END_LINE;
	if ( Flags & FGL::Program::Texture1 )
		Out << "    FragTexCoords1.z = float(InTextureLayers.y);" END_LINE;
	if ( Flags & FGL::Program::Texture2 )
		Out << "    FragTexCoords2.z = float(InTextureLayers.z);" END_LINE;
	if ( Flags & FGL::Program::Texture3 )
		Out << "    FragTexCoords3.z = float(InLightInfo.y);"  END_LINE;
	if ( Flags & FGL::Program::Texture4 )
		Out << "    FragTexCoords4.z = float(InLightInfo.z);"  END_LINE;

	if ( Flags & FGL::Program::ZoneLight )
		Out <<     "    FragZoneID = int(clamp(InLightInfo.x, 0u, 64u));" END_LINE;

	Out <<         "}"                                  END_LINE;
	Out << END_LINE;
}


/*-----------------------------------------------------------------------------
	GLSL fragment shader writer.
-----------------------------------------------------------------------------*/

inline void FFragmentShaderWriter::WriteExtensions(FCharWriter& Out)
{
	FShaderWriter::WriteExtensions(Out);

	if ( FOpenGLBase::SupportsShaderGroupVote )
		Out << "#extension GL_ARB_shader_group_vote : require" END_LINE;
}

inline void FFragmentShaderWriter::WriteInputs( FCharWriter& Out)
{
	Out << "// Inputs, generated by FFragmentShaderWriter::WriteInputs" END_LINE;

	if ( Flags & FGL::Program::Texture0 )  Out << "uniform sampler2DArray Texture0;" END_LINE;
	if ( Flags & FGL::Program::Texture1 )  Out << "uniform sampler2DArray Texture1;" END_LINE;
	if ( Flags & FGL::Program::Texture2 )  Out << "uniform sampler2DArray Texture2;" END_LINE;
	if ( Flags & FGL::Program::Texture3 )  Out << "uniform sampler2DArray Texture3;" END_LINE;
	if ( Flags & FGL::Program::Texture4 )  Out << "uniform sampler2DArray Texture4;" END_LINE;

	if ( Flags & FGL::Program::Color0 )    Out << "in vec4 FragColor0;" END_LINE;
	if ( Flags & FGL::Program::Color1 )    Out << "in vec4 FragColor1;" END_LINE;
	if ( Flags & FGL::Program::Texture0 )  Out << "centroid in vec3 FragTexCoords0;" END_LINE;
	if ( Flags & FGL::Program::Texture1 )  Out << "in vec3 FragTexCoords1;" END_LINE;
	if ( Flags & FGL::Program::Texture2 )  Out << "in vec3 FragTexCoords2;" END_LINE;
	if ( Flags & FGL::Program::Texture3 )  Out << "in vec3 FragTexCoords3;" END_LINE;
	if ( Flags & FGL::Program::Texture4 )  Out << "in vec3 FragTexCoords4;" END_LINE;
	if ( Flags & FGL::Program::ZoneLight ) Out << "flat in int FragZoneID;" END_LINE;
	if ( Flags & FGL::Program::Texture1 )  Out << "in float FragDistance;" END_LINE;

	Out << END_LINE;
}

inline void FFragmentShaderWriter::WriteOutputs( FCharWriter& Out)
{
	Out << "// Outputs, generated by FFragmentShaderWriter::WriteOutputs" END_LINE;

	Out <<     "layout (location=0) out vec4 FinalColor;"     END_LINE;

	Out << END_LINE;
}

inline void FFragmentShaderWriter::WriteProgram( FCharWriter& Out)
{
	if ( Flags & (FGL::Program::AlphaHack|FGL::Program::ColorCorrection) )
		Out << GLSL_FUNC_MAX3;
	if ( Flags & FGL::Program::ColorCorrection )
		Out << GLSL_FUNC_SQUARE_F;

	FCharWriter OutBaseTexture;
	FCharWriter OutDetailTexture;
	FCharWriter OutMacroTexture;
	FCharWriter OutLightMap;
	FCharWriter OutFogMap;

	WriteBaseTexture(OutDetailTexture);
	WriteDetailTexture(OutDetailTexture);
	WriteMacroTexture(OutMacroTexture);
	WriteLightMap(OutLightMap);
	WriteFogMap(OutFogMap);



	Out << "// Program, generated by FFragmentShaderWriter::WriteProgram" END_LINE;

	Out <<         "void main()"                                        END_LINE
	               "{"                                                  END_LINE;

	Out << *OutBaseTexture << *OutDetailTexture << *OutMacroTexture << *OutLightMap << *OutFogMap;

	DWORD Flags2 = Flags.GetValue();

	// Initial color assignment
	Out <<         "    vec4 Color = ";
	if ( Flags2 & FGL::Program::Texture0 )
		Out <<                       "BaseColor";
	else if ( Flags2 & FGL::Program::Color0 )
	{
		Out <<                       "FragColor0";
		Flags2 &= ~FGL::Program::Color0;
	}
	else if ( Flags2 & FGL::Program::ColorGlobal )
	{
		Out <<                       "ColorGlobal";
		Flags2 &= ~FGL::Program::ColorGlobal;
	}
	else if ( Flags2 & FGL::Program::Texture3 )
	{
		Out <<                       "LightColor";
		Flags2 &= ~FGL::Program::Texture3;
	}
	else if ( Flags2 & FGL::Program::Texture4 )
	{
		Out <<                       "FogColor";
		Flags2 &= ~FGL::Program::Texture4;
	}
	else
		Out <<                       "vec4(1.0, 1.0, 1.0, 1.0)";
	Out << ";" END_LINE;


	if ( (Flags2 & FGL::Program::Color0) && (Flags2 & FGL::Program::AlphaHack) ) // Translucent to Alpha GUI
	{
		Out <<
R"(    // Alpha hack
    Color.w   = min( max3(Color) * 1.25, 1.0) * max3(FragColor0);
    Color.xyz = (pow(Color.xyz, vec3(1.3,1.3,1.3)) * 1.3) * FragColor0.xyz;

)";
	}
	else if ( Flags2 & FGL::Program::Color0 )
		Out <<     "    Color *= FragColor0;"                           END_LINE;

	if ( Flags2 & FGL::Program::ColorGlobal )
		Out <<     "    Color *= ColorGlobal;"                          END_LINE;

	Out << END_LINE;

	bool LockAlpha = (Flags2 & (FGL::Program::Texture1|FGL::Program::Texture2|FGL::Program::Texture3|FGL::Program::Texture4|FGL::Program::ColorCorrection|FGL::Program::Color1)) != 0;
	if ( LockAlpha )
	{
		Out <<     "    // Lock alpha" END_LINE;
		Out <<     "    vec4 Alpha = vec4(0,0,0,Color.w);"              END_LINE;
		Out << END_LINE;
	}



	if ( Flags2 & FGL::Program::Texture1 )
		Out <<     "    Color *= DetailColor;" END_LINE;
	if ( Flags2 & FGL::Program::Texture2 )
		Out <<     "    Color *= MacroColor;" END_LINE;
	if ( Flags2 & FGL::Program::Texture3 )
		Out <<     "    Color *= LightColor;" END_LINE;
	if ( Flags2 & FGL::Program::Texture4 )
		Out <<     "    Color = Color * (1.0 - FogColor.w) + FogColor;" END_LINE;
	if ( Flags2 & FGL::Program::Color1 )
		Out <<     "    Color = Color * (1.0 - FragColor1.w) + FragColor1; // Vertex Fog" END_LINE;

	if ( Flags2 & FGL::Program::ColorCorrection )
	{
		Out << END_LINE;
		if ( FOpenGLBase::GetShaderBrightness() > 1.0f )
		{
			// Obtains a multiplier required to offset value according to the following
			// formula: ((1 - (2 * value - 1)^2) * 0.25)
			// It has the shape of a parabola with roots in 0,1 and maximum at f(x=0.5)=0.25
			Out << "    // Apply brightness adjustment without saturating colors" END_LINE
			       "    float CCValue = max(max3(Color), 0.001);"                 END_LINE
			       "    float CC = (1.0 - square_f(2.0 * CCValue - 1.0)) * 0.25  * (ColorCorrection.w - 1.0);" END_LINE;
			if ( Flags2 & FGL::Program::ColorCorrectionAlpha )
				Out << "    CC *= Alpha.w;"                             END_LINE;
			Out << "    Color = clamp( Color * ((CCValue+CC) / CCValue), 0.0, 1.0);" END_LINE;
		}
		else if ( FOpenGLBase::GetShaderBrightness() < 1.0f )
		{
			Out << "    // Downscale brightness"                        END_LINE
			       "    Color *= ColorCorrection.w;"                    END_LINE;
		}
		Out <<     "    Color.xyz = pow(Color.xyz, ColorCorrection.xyz);" END_LINE;
	}
	Out << END_LINE;

	if ( LockAlpha )
		Out <<     "    FinalColor = Color * vec4(1,1,1,0) + Alpha;"    END_LINE;
	else
		Out <<     "    FinalColor = Color;" END_LINE;

	if ( Flags2 & FGL::Program::NoNearZ )
	{
		Out <<
R"(
    // Adjust Depth for weapon rendering, used to prevent drawing weapons on top of the HUD
    gl_FragDepth = max(gl_FragCoord.z , 0.6) - max( (0.6 - gl_FragCoord.z) / 6.0, 0.0);

)";
	}
	Out <<         "}" END_LINE END_LINE;
}

inline void FFragmentShaderWriter::WriteBaseTexture(FCharWriter& Out)
{
	if ( !(Flags & FGL::Program::Texture0) )
		return;

	Out <<     "    // Base Texture" END_LINE;
	Out <<     "    vec4 BaseColor = texture(Texture0, FragTexCoords0, LodBias);" END_LINE;

	if ( Flags & FGL::Program::AlphaTest )
	{
		if ( Flags & FGL::Program::DervMapped )
		{
			// TODO: DERV MAP
		}

		Out << "    if ( BaseColor.w < AlphaTest )" END_LINE;
		Out << "        discard;" END_LINE;
	}

	Out << END_LINE;
}

inline void FFragmentShaderWriter::WriteDetailTexture( FCharWriter& Out)
{
	if ( !(Flags & FGL::Program::Texture1) )
		return;

	const char* IND = "    ";

	Out << IND << "vec4 DetailColor = vec4(1.0);" END_LINE;
	if ( FOpenGLBase::SupportsShaderGroupVote )
	{
		Out << IND << "if ( anyInvocationARB(FragDistance < 380.0) )" END_LINE;
		Out << IND << "{" END_LINE;
		IND = "        ";
	}

	Out << IND << "// DetailTexture (amplify by 1/512 for better neutral modulation)" END_LINE;
	Out << IND << "float DetailFade = min(FragDistance * 0.002631578947, 1.0); // Divide by 380" END_LINE;
	Out << IND << "DetailColor = mix(texture(Texture1, FragTexCoords1, LodBias)*2.0039, vec4(1,1,1,1), DetailFade); // Layer 1" END_LINE;

	if ( FOpenGLBase::ActiveInstance->RenDev->DetailMax <= 1 )
		goto END;

	Out << IND << "vec3 DetailCoords2 = FragTexCoords1 * vec3(4.223,4.223,1.0);" END_LINE;
	Out << IND << "DetailFade = min(DetailFade*4.223, 1.0);" END_LINE;
	Out << IND << "DetailColor *= mix(texture(Texture1, DetailCoords2, LodBias)*2.0039, vec4(1,1,1,1), DetailFade); // Layer 2" END_LINE;

	if ( FOpenGLBase::ActiveInstance->RenDev->DetailMax <= 2 )
		goto END;

	Out << IND << "vec3 DetailCoords3 = DetailCoords2 * vec3(4.223,4.223,1.0);" END_LINE;
	Out << IND << "DetailFade = min(DetailFade*4.223, 1.0);" END_LINE;
	Out << IND << "DetailColor *= mix(texture(Texture1, DetailCoords3, LodBias)*2.0039, vec4(1,1,1,1), DetailFade); // Layer 3" END_LINE;

	END:
	if ( FOpenGLBase::SupportsShaderGroupVote )
	{
		IND = "    ";
		Out << IND << "}" END_LINE;
	}

	Out << END_LINE;
}

inline void FFragmentShaderWriter::WriteMacroTexture(FCharWriter& Out)
{
	if ( !(Flags & FGL::Program::Texture2) )
		return;

	Out <<     "    // MacroTexture" END_LINE;
	Out <<     "    vec4 MacroColor = texture(Texture2, FragTexCoords2, LodBias);" END_LINE;

	Out << END_LINE;
}

inline void FFragmentShaderWriter::WriteLightMap( FCharWriter& Out)
{
	if ( !(Flags & FGL::Program::Texture3) )
		return;

	Out << "    // LightMap";
	Out << END_LINE;


	Out << "    vec4 LightColor = texture(Texture3, FragTexCoords3)";
	if ( Flags & FGL::Program::ZoneLight )
		Out << "+ ZoneAmbientPlane[FragZoneID]";
	Out << ";" END_LINE;

	Out << "    LightColor = min(LightColor,0.5) * LightMapFactor;" END_LINE;
	Out << END_LINE;
}

inline void FFragmentShaderWriter::WriteFogMap(FCharWriter& Out)
{
	if ( !(Flags & FGL::Program::Texture4) )
		return;

	Out <<     "    // FogMap" END_LINE;
	Out <<     "    vec4 FogColor = texture(Texture4, FragTexCoords4) * 2.0;" END_LINE;

	Out << END_LINE;
}


/*-----------------------------------------------------------------------------
	GLSL shader writer utils - framebuffer blit and resolve.
-----------------------------------------------------------------------------*/

inline bool FShaderWriterFBOblit::UseMultiSampling() const
{
	return false; // Not decided
}

inline const ANSICHAR* FShaderWriterFBOblit::samplerType() const
{
	return UseMultiSampling() ? "sampler2DMS" : "sampler2D";
}

inline const ANSICHAR* FShaderWriterFBOblit::textureSize( const ANSICHAR* SamplerName)
{
	// textureSize(sampler, lod) for sampler2D
	// textureSize(sampler)      for sampler2DMS
	CompositionWriter.Reset();
	CompositionWriter << "textureSize(" << SamplerName;
	if ( !UseMultiSampling() )\
		CompositionWriter << ", 0";
	CompositionWriter << ")";
	return *CompositionWriter;
}

/*-----------------------------------------------------------------------------
	GLSL vertex shader writer - framebuffer blit and resolve.
-----------------------------------------------------------------------------*/

inline void FVertexShaderWriterFBOblit::WriteInputs( FCharWriter& Out)
{
	Out << "// Inputs, generated by FVertexShaderWriterFBOblit::WriteInputs" END_LINE;
	Out << "uniform " << samplerType() << " Texture0;"                       END_LINE;
	Out << "layout (location=3) in vec2 InTexCoords0;"                       END_LINE;
	Out << END_LINE;
}

inline void FVertexShaderWriterFBOblit::WriteOutputs( FCharWriter& Out)
{
	Out << "// Outputs, generated by FVertexShaderWriterFBOblit::WriteOutputs" END_LINE;
	Out << "out vec2 FragTexCoords0;"                                          END_LINE;
	Out << END_LINE;
}

inline void FVertexShaderWriterFBOblit::WriteProgram( FCharWriter& Out)
{
	Out <<     "// Program, generated by FVertexShaderWriterFBOblit::WriteProgram" END_LINE;
	Out <<     "void main()"                                                       END_LINE;
	Out <<     "{"                                                                 END_LINE;

	Out << R"(    // Generate X,Y coords (using GL_TRIANGLE_FAN order)
    const vec2 XY[4] = vec2[](
        vec2(-1, -1),
        vec2(+1, -1),
        vec2(+1, +1),
        vec2(-1, +1)
    );
)";

	Out <<     "    gl_Position = vec4(XY[gl_VertexID], 0.0, 1.0);"                END_LINE;
	Out <<     "    FragTexCoords0 = (gl_Position.xy + vec2(1.0)) * 0.5 * vec2(" << textureSize("Texture0") << ".xy);" END_LINE;
	Out <<     "}"                                                                 END_LINE;
	Out << END_LINE;
}


/*-----------------------------------------------------------------------------
	GLSL fragment shader writer - framebuffer blit and resolve.
-----------------------------------------------------------------------------*/

inline void FFragmentShaderWriterFBOblit::WriteInputs( FCharWriter& Out)
{
	const char* samplerType = UseMultiSampling() ? "sampler2DMS" : "sampler2D"; 

	Out << "// Inputs, generated by FFragmentShaderWriterFBOblit::WriteInputs" END_LINE;
	Out << "uniform " << samplerType << " Texture0;"                           END_LINE;
	Out << "in vec2 FragTexCoords0;"                                           END_LINE;
	Out << END_LINE;
}

inline void FFragmentShaderWriterFBOblit::WriteOutputs( FCharWriter& Out)
{
	Out << "// Outputs, generated by FFragmentShaderWriterFBOblit::WriteOutputs" END_LINE;
	Out << "layout (location=0) out vec4 FinalColor;"                            END_LINE;
	Out << END_LINE;
}

inline void FFragmentShaderWriterFBOblit::WriteProgram( FCharWriter& Out)
{
	Out << GLSL_FUNC_MAX3;
	Out << GLSL_FUNC_SQUARE_F;
	if ( UseMultiSampling() )
		Out << GLSL_FUNC_TEXTURE_MULTISAMPLE;

	Out <<     "// Program, generated by FFragmentShaderWriterFBOblit::WriteProgram"  END_LINE;
	Out <<     "void main()"                                                          END_LINE;
	Out <<     "{"                                                                    END_LINE;

	if ( !UseMultiSampling() )
	{
		Out << "    // Base Texture"                                                  END_LINE;
		Out << "    ivec2 Coords = ivec2(FragTexCoords0);" END_LINE;
		Out << "    vec4 Color = texelFetch(Texture0, Coords, 0);"                    END_LINE;
	}
	else
	{
		Out << "    // Multisampled Base Texture"                                     END_LINE;
		Out << "    ivec2 Coords = ivec2(FragTexCoords0);"    END_LINE;
		Out << "    vec4 Color = textureMultisample(Texture0, Coords);"               END_LINE;
	}
	Out <<     END_LINE;

	Out <<     "    // Apply color correction"                                        END_LINE;
	if ( FOpenGLBase::GetShaderBrightness() > 1.0f )
	{
		// Obtains a multiplier required to offset value according to the following
		// formula: ((1 - (2 * value - 1)^2) * 0.25)
		// It has the shape of a parabola with roots in 0,1 and maximum at f(x=0.5)=0.25
		Out << "    float CCValue = max(max3(Color), 0.001);"                         END_LINE;
		Out << "    float CC = (1.0 - square_f(2.0 * CCValue - 1.0)) * 0.25  * (ColorCorrection.w - 1.0);" END_LINE;
		Out << "    Color.xyz = clamp(Color * ((CCValue+CC) / CCValue), 0.0, 1.0).xyz;" END_LINE;
	}
	else if ( FOpenGLBase::GetShaderBrightness() < 1.0f )
	{
		Out << "    Color.xyz *= ColorCorrection.w;"                                  END_LINE;
	}
	Out <<     "    FinalColor.rgb = pow(Color.xyz, ColorCorrection.xyz);"            END_LINE;
	Out <<     "    FinalColor.a   = Color.a;"                                        END_LINE;
	Out <<     "}"                                                                    END_LINE;
	Out <<     END_LINE;

}

