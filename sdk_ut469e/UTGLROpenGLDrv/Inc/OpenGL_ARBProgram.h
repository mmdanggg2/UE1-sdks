/*=============================================================================
	OpenGL_ARBProgram.h
	
	ARB shader writers.

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/


//
// ARB Program writer base class
//
class FProgramWriter
{
public:
	FProgramID Flags;
	FCharWriter Out;

	virtual ~FProgramWriter() {}

	void Create( FProgramID NewFlags)
	{
		Flags = NewFlags;

		Setup();
		WriteHeader(Out);
		WriteVariables(Out);
		WriteProgram(Out);
		WriteEnd(Out);
	}

protected:
	virtual void Setup() {}
	virtual void WriteHeader( FCharWriter& Out)=0;
	virtual void WriteVariables( FCharWriter& Out) {}
	virtual void WriteProgram( FCharWriter& Out) {}
	virtual void WriteEnd( FCharWriter& Out);
};

inline void FProgramWriter::WriteEnd(FCharWriter & Out)
{
	Out << "END" END_LINE;
}


//
// ARB vertex program writer
//
static constexpr DWORD OPT_TextureUVPan = FGL::Program::Texture0|FGL::Program::Texture1|FGL::Program::Texture2;

class FVertexProgramWriter : public FProgramWriter
{
protected:
	UBOOL UseVBOTranslation;
	UBOOL UseTextureUVPan; // Texture UVs need to be scaled
	INT TexCoordInput[TMU_SpecialMap];


	void Setup()
	{
		// Defaults
		UseVBOTranslation = false;
		UseTextureUVPan   = false;
		for ( INT i=0; i<ARRAY_COUNT(TexCoordInput); i++)
			TexCoordInput[i] = (Flags & (1 << i)) ? i : INDEX_NONE;

		switch ( Flags.GetVertexType() )
		{
			case FGL::VertexProgram::Identity: // Pass coordinates without transformations
				return;
			case FGL::VertexProgram::ComplexSurfaceVBO:
				UseVBOTranslation = true;
				TexCoordInput[TMU_LightMap] = INDEX_NONE; //HARDCODED
				TexCoordInput[TMU_FogMap]   = INDEX_NONE; //HARDCODED
			case FGL::VertexProgram::ComplexSurface:
				UseTextureUVPan   = true;
			default:
				break;
		}

		// Texture 1 and 2 also depend on input 0, HARDCODED
		if ( UseTextureUVPan )
		{
			for ( INT i=0; i<=2; i++)
				TexCoordInput[i] = INDEX_NONE;
			for ( INT i=3; i<ARRAY_COUNT(TexCoordInput); i++)
				if ( TexCoordInput[i] >= 0 )
					TexCoordInput[i] -= 2;
		}
	}

	void WriteHeader( FCharWriter& Out)
	{
		Out << "!!ARBvp1.0" END_LINE;
	}

	void WriteVariables( FCharWriter& Out)
	{
		if ( Flags.GetVertexType() == FGL::VertexProgram::Identity )
			return;

		// Position already transformed to CLIP coords
		if ( !UseVBOTranslation )
		{
			Out << "OPTION ARB_position_invariant;" END_LINE;
			Out << "ATTRIB iPos = vertex.position;" END_LINE;
		}

		// Texture coords need to be scaled for Base, Detail, Macro
		// NOTE: Light and Fog UV's are already scaled
		if ( UseTextureUVPan )
		{
			if ( Flags & (FGL::Program::Texture0|FGL::Program::Texture1) )
				Out << "ATTRIB texInfoBaseDetail = vertex.attrib[6];" END_LINE;
			if ( Flags & FGL::Program::Texture2 )
				Out << "ATTRIB texInfoMacro = vertex.attrib[7];" END_LINE;
		}

		if ( UseVBOTranslation )
		{
			Out << "PARAM mat[4] = { state.matrix.mvp };" END_LINE;
			Out << "TEMP iPos;" END_LINE;
		}

		if ( UseTextureUVPan )
		{
			Out << "TEMP t0;"           END_LINE
			    << "MOV t0, {0,0,0,1};" END_LINE;
		}

		if ( UseTextureUVPan && (Flags & OPT_TextureUVPan) )
			Out << "OUTPUT oTex_Pos = result.texcoord[" << TMU_Position << "];" END_LINE;
	}

	void WriteProgram( FCharWriter& Out)
	{
		if ( Flags & FGL::Program::Color0 )
			Out << "MOV result.color, vertex.color;" END_LINE;
		if ( Flags & FGL::Program::Color1 )
			Out << "MOV result.color.secondary, vertex.color.secondary;" END_LINE;

		if ( Flags.GetVertexType() == FGL::VertexProgram::Identity )
		{
			Out << "MOV result.position, vertex.position;" END_LINE;
			if ( Flags & FGL::Program::Texture0 )
				Out << "MOV result.texcoord[0], vertex.texcoord[0];" END_LINE;
			return;
		}

		if ( UseTextureUVPan )
		{
			// Base, Detail, Macro need to be transformed (using input 0)

			if ( Flags & FGL::Program::Texture0 )
			{
				Out << "MOV t0, vertex.texcoord[0];" END_LINE;

				if ( Flags.GetVertexType() == FGL::VertexProgram::ComplexSurfaceVBO )
				{
					Out << "ADD t0.xy, vertex.texcoord[0], vertex.texcoord[0].zwzw;" END_LINE; // Apply static panning
					Out << "MAD t0.xy, program.env[4].xyxy, vertex.texcoord[2].zzzz, t0;" END_LINE; // Apply wavy panning
					Out << "MAD t0.xy, program.env[5].xyxy, vertex.texcoord[2].xyxy, t0;" END_LINE; // Apply zone panning
				}
				else if ( Flags.GetVertexType() == FGL::VertexProgram::ComplexSurface )
					Out << "SUB t0.xy, vertex.texcoord[0], vertex.texcoord[0].zwzw;" END_LINE; // Apply full panning

				Out << "MUL t0.xy, t0, texInfoBaseDetail.xyxy;" END_LINE;
				Out << "MOV result.texcoord[" << TMU_Base       << "], t0;" END_LINE;
				// TODO: DERV MAPPING CONDITION
					Out << "MUL result.texcoord[" << TMU_SpecialMap << "], t0, program.env[7];" END_LINE;
			}

			// TODO: Pass UTexture::Scale to properly apply detail, macro bias of -0.5
			if ( Flags & FGL::Program::Texture1 )
				Out << "MUL result.texcoord[" << TMU_DetailTexture << "], vertex.texcoord[0], texInfoBaseDetail.zwzw;" END_LINE;
			if ( Flags & FGL::Program::Texture2 )
				Out << "MUL result.texcoord[" << TMU_MacroTexture  << "], vertex.texcoord[0], texInfoMacro.xyxy;" END_LINE;
		}

		// Light, Fog need to be transformed
		if ( Flags.GetVertexType() == FGL::VertexProgram::ComplexSurfaceVBO )
		{
			if ( Flags & FGL::Program::Texture3 )
			{
				Out << "MUL t0, program.env[6].xxxx, vertex.texcoord[1].xyxy;" END_LINE;
				Out << "MAD t0, program.env[6].yyyy, vertex.texcoord[1].zwzw, t0;" END_LINE;
				Out << "MOV result.texcoord[" << TMU_LightMap << "], t0;" END_LINE;
			}
			if ( Flags & FGL::Program::Texture4 )
			{
				Out << "MUL t0, program.env[6].zzzz, vertex.texcoord[1].xyxy;" END_LINE;
				Out << "MAD t0, program.env[6].wwww, vertex.texcoord[1].zwzw, t0;" END_LINE;
				Out << "MOV result.texcoord[" << TMU_FogMap << "], t0;" END_LINE;
			}
		}

		// Default inputs
		for ( INT i=0; i<TMU_SpecialMap; i++)
			if ( TexCoordInput[i] >= 0 )
				Out << "MOV result.texcoord[" << i << "], vertex.texcoord[" << TexCoordInput[i] << "];" END_LINE;
		// TODO: DERV MAP COORDS

		if ( UseVBOTranslation )
		{
			// Transform to UT screen space
			// TODO: Apply local transformation (Actor->Location) for VBO's
			Out << "MOV iPos, vertex.position;" END_LINE;
			Out << "ADD t0, iPos, -program.env[0];"       END_LINE //  Frame.Coords.Origin
			       "DP3 iPos.x, t0, program.env[1];"      END_LINE //  Frame.Coords.XAxis
			       "DP3 iPos.y, t0, program.env[2];"      END_LINE //  Frame.Coords.YAxis
			       "DP3 iPos.z, t0, program.env[3];"      END_LINE;//  Frame.Coords.ZAxis

			// Transform to CLIP space
			Out << "DP4 result.position.x, mat[0], iPos;" END_LINE
			       "DP4 result.position.y, mat[1], iPos;" END_LINE
			       "DP4 result.position.z, mat[2], iPos;" END_LINE
			       "DP4 result.position.w, mat[3], iPos;" END_LINE;
		}

		if ( UseTextureUVPan && (Flags & OPT_TextureUVPan) )
			Out << "MOV oTex_Pos, iPos;" END_LINE;

		if ( Flags & FGL::Program::LinearFog ) //RUNE
			Out << "MOV result.fogcoord.x, vertex.position.z;" END_LINE;
	}
};


//
// ARB fragment program writer
//
class FFragmentProgramWriter : public FProgramWriter
{
public:
	INT DetailPasses;
	UBOOL UsingAA;

protected:
	void Setup()
	{
		DetailPasses = Clamp( FOpenGLBase::ActiveInstance->RenDev->DetailMax, 1, 3);
		UsingAA = FOpenGLBase::ActiveInstance->RenDev->m_usingAA;
	}

	void WriteHeader( FCharWriter& Out)
	{
		Out << "!!ARBfp1.0" END_LINE;
	}

	// Notes:
	// GL_FRAGMENT_PROGRAM_ARB.GL_MAX_PROGRAM_ATTRIBS_ARB: safe minimum is 8
	// GL_FRAGMENT_PROGRAM_ARB.GL_MAX_PROGRAM_PARAMETERS_ARB: safe minimum is 26
	// GL_FRAGMENT_PROGRAM_ARB.GL_MAX_PROGRAM_TEMPORARIES_ARB: safe minimum is 16
	// GL_FRAGMENT_PROGRAM_ARB.GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB: safe minimum is 64
	void WriteVariables( FCharWriter& Out)
	{
		if ( Flags & FGL::Program::LinearFog )
			Out << "OPTION ARB_fog_linear;" END_LINE;

		// Parameters
		if ( Flags & FGL::Program::Color0 )
			Out << "ATTRIB iColor = fragment.color.primary;" END_LINE;
		if ( Flags & FGL::Program::Texture0 )
			Out << "ATTRIB iTC0 = fragment.texcoord[" << TMU_Base << "];" END_LINE;
		if ( Flags & FGL::Program::Texture1 )
			Out << "ATTRIB iTC_Detail = fragment.texcoord[" << TMU_DetailTexture << "];" END_LINE;
		if ( Flags & FGL::Program::Texture2 )
			Out << "ATTRIB iTC_Macro = fragment.texcoord[" << TMU_MacroTexture << "];" END_LINE;
		if ( Flags & FGL::Program::Texture3 )
			Out << "ATTRIB iTC_Light = fragment.texcoord[" << TMU_LightMap << "];" END_LINE;
		if ( Flags & FGL::Program::Texture4 )
			Out << "ATTRIB iTC_Fog = fragment.texcoord[" << TMU_FogMap << "];" END_LINE;
		if ( Flags & FGL::Program::Texture1 )
			Out << "ATTRIB iTC_Pos = fragment.texcoord[" << TMU_Position << "];" END_LINE;
		if ( Flags & FGL::Program::Texture0 ) // TODO: ANTIALIAS CONDITION
			Out << "ATTRIB iTC_Special = fragment.texcoord[" << TMU_SpecialMap << "];" END_LINE;

		if ( Flags & FGL::Program::ColorCorrection )
			Out << "PARAM gamma = program.env[0];" END_LINE;
		if ( Flags & FGL::Program::Texture1 )
			Out << "PARAM RNearZ = 0.002631578947;" END_LINE;
		if ( Flags & FGL::Program::AlphaHack )
			Out << "PARAM tP = { 1.3, 1.3, 1.3, 0};" END_LINE;
		if ( Flags & FGL::Program::Texture1 ) 
			Out << "PARAM DetailFade = { 0.5, 0.5, 0.5, 0.5};" END_LINE;
		if ( Flags & FGL::Program::Texture1 && DetailPasses > 1 )
			Out << "PARAM DetailScale = { 4.223, 4.223, 0, 1 };" END_LINE;

		// Locals
		Out << "TEMP t0, t1";
		if ( Flags & FGL::Program::Texture1 )
			Out << ", tD";
		if ( Flags & FGL::Program::Texture2 )
			Out << ", tMacro";
		if ( Flags & FGL::Program::Texture3 )
			Out << ", tLight";
		if ( Flags & FGL::Program::Texture4 )
			Out << ", tFog";
		if ( (Flags & FGL::Program::ColorCorrection) && (FOpenGLBase::GetShaderBrightness() != 1.0f) )
			Out << ", tBR";
		Out << ";" END_LINE;
	}

	void WriteProgram( FCharWriter& Out)
	{
		// Default color if no textures
		// NOTE: Detail, Macro, LightMap depend on Texture0 being present (FogMap can be drawn independently)
		if ( !(Flags & FGL::Program::TextureMask) )
			Out << "MOV t0, {1,1,1,1};" END_LINE;

		// Get textures
		if ( Flags & FGL::Program::Texture0 )
			Out << "TEX t0, iTC0, texture[" << TMU_Base << "], 2D;" END_LINE;
		if ( Flags & FGL::Program::Texture1 )
			Out << "TEX tD, iTC_Detail, texture[" << TMU_DetailTexture << "], 2D;" END_LINE;
		if ( Flags & FGL::Program::Texture2 )
			Out << "TEX tMacro, iTC_Macro, texture[" << TMU_MacroTexture << "], 2D;" END_LINE;
		if ( Flags & FGL::Program::Texture3 )
			Out << "TEX tLight, iTC_Light, texture[" << TMU_LightMap << "], 2D;" END_LINE;
		if ( Flags & FGL::Program::Texture4 )
			Out << "TEX tFog, iTC_Fog, texture[" << TMU_FogMap << "], 2D;" END_LINE;


		if ( Flags & FGL::Program::AlphaTest )
		{
			// See UOpenGLRenderDevice::InitDervTextureSafe
			if ( Flags & FGL::Program::DervMapped )
			{
				Out << "TEX t1, iTC_Special, texture[" << TMU_SpecialMap << "], 2D;" END_LINE
					"MUL t1.x, t1, 255;"                END_LINE
					"ADD t0.w, t0.w, -0.5;"             END_LINE
					"MAD t0.w, t0.w, t1.x, 0.5;"        END_LINE;
			}

			// Kill fragment if alpha is below alpha reject threshold
			// Masked: 0.5
			// Masked+Derv: 0.2
			// AlphaBlend: 0.01
			if ( Flags & FGL::Program::Texture0 )
			{
				Out << "SUB t1.xyzw, t0.w, program.env[1].z;" END_LINE
					"KIL t1;"                              END_LINE;
			}
		}

		// Alpha hack: transform RGB
		if ( (Flags & FGL::Program::Color0) && (Flags & FGL::Program::AlphaHack) )
		{
			Out << "MAX t1.w, t0.x, t0.y;"         END_LINE
				"MAX t1.w, t1.w, t0.z;"         END_LINE
				"MUL_SAT t1.w, t1.w, 1.25;"     END_LINE
				"MAX t1.z, iColor.x, iColor.y;" END_LINE
				"MAX t1.z, t1.z, iColor.z;"     END_LINE
				"POW t0.x, t0.x, tP.x;"         END_LINE
				"POW t0.y, t0.y, tP.y;"         END_LINE
				"POW t0.z, t0.z, tP.z;"         END_LINE
				"MUL t0, t0, tP;"               END_LINE;
		}

		// Apply simple color transformation
		if ( Flags & FGL::Program::Color0 )
			Out << "MUL t0, t0, iColor;" END_LINE;

		// Alpha hack: transform Alpha
		if ( (Flags & FGL::Program::Color0) && (Flags & FGL::Program::AlphaHack) )
			Out << "MUL_SAT t0.w, t1.w, t1.z;" END_LINE;

		// Apply light map
		if ( Flags & FGL::Program::Texture3 )
		{
			Out << "TEX tLight, iTC_Light, texture[" << TMU_LightMap << "], 2D;" END_LINE;
			if ( USES_AMBIENTLESS_LIGHTMAPS )
				Out << "ADD tLight, tLight, program.env[2];"                     END_LINE;
			Out << "MUL tLight, tLight, program.env[1].w;"                       END_LINE;
			Out << "MUL t0.rgb, tLight, t0;"                                     END_LINE;
		}

		// Apply detail texture
		if ( Flags & FGL::Program::Texture1 )
		{
			Out << "MUL_SAT t1.x, iTC_Pos.z, RNearZ;" END_LINE
				"LRP tD, t1.xxxx, DetailFade, tD;" END_LINE
				"MUL t0.rgb, t0, tD;" END_LINE
				"ADD t0.rgb, t0, t0;" END_LINE;

			if ( DetailPasses >= 2 )
			{
				Out << "MUL tD, iTC_Detail, DetailScale;" END_LINE
					"TEX tD, tD, texture[" << TMU_DetailTexture << "], 2D;" END_LINE
					"MUL_SAT t1.x, t1.x, DetailScale.x;" END_LINE
					"LRP tD, t1.xxxx, DetailFade, tD;" END_LINE
					"MUL t0.rgb, t0, tD;" END_LINE
					"ADD t0.rgb, t0, t0;" END_LINE;
			}

			if ( DetailPasses >= 3 ) // Higor: this is ridiculous
			{
				Out << "MUL tD, iTC_Detail, DetailScale;" END_LINE
					"MUL tD, iTC_Detail, DetailScale;" END_LINE
					"TEX tD, tD, texture[" << TMU_DetailTexture << "], 2D;" END_LINE
					"MUL_SAT t1.x, t1.x, DetailScale.x;" END_LINE
					"LRP tD, t1.xxxx, DetailFade, tD;" END_LINE
					"MUL t0.rgb, t0, tD;" END_LINE
					"ADD t0.rgb, t0, t0;" END_LINE;
			}
		}

		// Apply macro texture
		if ( Flags & FGL::Program::Texture2 )
		{
			// Do 2x scaling?
			Out << "MUL t0.rgb, t0, tMacro;" END_LINE;
		}

		// Apply fog map
		if ( Flags & FGL::Program::Texture4 )
		{
			if ( !(Flags & FGL::Program::Texture0) )
				Out << "ADD t0, tFog, tFog;" END_LINE;
			else
				Out << "ADD tFog, tFog, tFog;" END_LINE
				"SUB tFog.a, 1.0, tFog.a;" END_LINE
				"MAD t0.rgb, t0, tFog.aaaa, tFog;" END_LINE;
		}

		// Apply vertex fog
		if ( Flags & FGL::Program::Color1 )
			Out << "MAD t0.rgb, t0, iColor, fragment.color.secondary;" END_LINE;

		// Apply gamma
		if ( Flags & FGL::Program::ColorCorrection )
		{
			if ( FOpenGLBase::GetShaderBrightness() > 1.0f )
			{
				// Obtains a multiplier required to offset value according to the following
				// formula: ((1 - (2 * value - 1)^2) * 0.25)
				// It has the shape of a parabola with roots in 0,1 and maximum at f(x=0.5)=0.25
				Out << "MAX tBR.w, t0.x, t0.y;"        END_LINE
					"MAX tBR.w, t0.z, tBR.w;"       END_LINE // (Value)
					"MAD tBR.x, tBR.w, 2, -1;"      END_LINE // (2*Value - 1)
					"MAD tBR.x, tBR.x, -tBR.x, 1;"  END_LINE // (1 - ((2*Value - 1)^2))
					"MUL_SAT tBR.x, tBR.x, 0.25;"   END_LINE; // (1 - ((2*Value - 1)^2)) * 0.25 = X
				if ( Flags & FGL::Program::ColorCorrectionAlpha )
					Out << "MUL tBR.x, tBR.x, t0.a;"   END_LINE; //Downscale impact of brightness adjustment
				Out << "ADD tBR.z, -1.0, gamma.w;"     END_LINE
					"MUL tBR.x, tBR.z, tBR.x;"      END_LINE // (1 - ((2*Value - 1)^2)) * 0.25 * (Brightness-1)
					"ADD tBR.z, tBR.x, tBR.w;"      END_LINE // (Value + above) = (Value2)
					"RCP t1, tBR.w;"                END_LINE
					"MUL tBR.z, tBR.z, t1.x;"       END_LINE // (Value2) / Value
					"MUL_SAT t0.rgb, t0, tBR.zzzz;"     END_LINE;
			}
			else if ( FOpenGLBase::GetShaderBrightness() < 1.0f )
			{
				Out << "MUL t0.rgb, t0, gamma.wwww;"       END_LINE;
			}

			Out << "POW t0.r, t0.r, gamma.x;" END_LINE
				"POW t0.g, t0.g, gamma.y;" END_LINE
				"POW t0.b, t0.b, gamma.z;" END_LINE;
		}

		// Output color
		Out << "MOV result.color.rgba, t0;" END_LINE;

		// Output modified depth
		if ( Flags & FGL::Program::NoNearZ )
		{
			// Filter out fragments behind the camera
			Out << "SGE t1, fragment.position.zzzz, 0;" END_LINE
				"KIL t1;"                                                   END_LINE;

			Out << "SGE t1, fragment.position.zzzz, 0.6;"           END_LINE
				"MAD t0, fragment.position.zzzz, 0.1667, 0.5;"      END_LINE  // Transformed depth
				"LRP result.depth, t1, fragment.position.zzzz, t0;" END_LINE; // F=transformed depth if <1.2, normal depth if >= 1.2
//			Out << "MOV result.depth, fragment.position.z;" END_LINE;
		}
	}
};
