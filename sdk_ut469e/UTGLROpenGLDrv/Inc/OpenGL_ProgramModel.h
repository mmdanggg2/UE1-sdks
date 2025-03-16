/*=============================================================================
	OpenGL_ProgramModel.h

	Shader program ID descriptors and helpers.

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

/*-----------------------------------------------------------------------------
	Program descriptors.
-----------------------------------------------------------------------------*/

// Texture Mapping unit identification
enum ETextureMappingUnitIndex
{
	TMU_Base                =  0,
	TMU_DetailTexture,
	TMU_MacroTexture,
	TMU_LightMap,
	TMU_FogMap,
	TMU_SpecialMap,
	TMU_Position,

	TMU_LightMapParams      =  8,

	TMU_BASE_MAX            =  8,
	TMU_EXTENDED_MAX        = 16,

};

namespace FGL
{
	namespace ProgramBitNumber
	{
		enum EProgramBitNumber
		{
			Texture0        = 1,
			Texture1,
			Texture2,
			Texture3,
			Texture4,
			Texture5,
			Texture6,
			Texture7,

			Color0,
			Color1,
			ColorGlobal,
			ColorCorrection,
			ColorCorrectionAlpha,
			LinearFog,

			AlphaTest,
			AlphaHack,
			DervMapped,
			NoNearZ,
			ZoneLight,

			VertexParam0,

			MAX,
		};
	};

	namespace VertexProgram
	{
		enum EVertexProgramID
		{
			Identity,               // XYZ and UV unchanged
			Simple,                 // XYZ transformed with Projection matrix
			Tile,                   // XYZ transformed with tile parameters then Projection matrix
			ComplexSurface,         // XYZ transformed with Projection matrix, UV 0 panned, UV 0,1,2 scaled.
			ComplexSurfaceVBO,
//			ComplexSurfaceVBOActor,

			// Notes:
			// * ComplexSurface uses additional inputs in TexCoords0.zw for texture panning.

			MAX,
			BITMASK  = TBits<MAX-1>::BitMask,
			BITCOUNT = TBits<MAX-1>::BitCount,
		};

		static inline bool UsesClipPlane( DWORD VID)
		{
			return VID == ComplexSurface || VID == ComplexSurfaceVBO;
		}
	};

	namespace SpecialProgram
	{
		enum ESpecialProgramID
		{
			None,
			FBO_Draw,
			FBO_Compute,      // TODO: GL 4.5 mode with compute pipeline

			MAX,
			BITMASK  = TBits<MAX-1>::BitMask,
			BITCOUNT = TBits<MAX-1>::BitCount,
		};
	};

	namespace Program
	{
		enum EProgram
		{
			#define PROGRAM_BIT(OptName) OptName = 1 << (FGL::ProgramBitNumber::OptName-1)
			// Texture coords/attribs used in this shader
			PROGRAM_BIT(Texture0),
			PROGRAM_BIT(Texture1),
			PROGRAM_BIT(Texture2),
			PROGRAM_BIT(Texture3),
			PROGRAM_BIT(Texture4),
			// These bits are reserved for Texture mapping units
			TextureMask      = 0b11111111,
			TextureUnlitMask = 0b00000111,
			TextureLightMask = 0b00011000,

			PROGRAM_BIT(Color0),
			PROGRAM_BIT(Color1),
			PROGRAM_BIT(ColorGlobal),
			PROGRAM_BIT(ColorCorrection),
			PROGRAM_BIT(ColorCorrectionAlpha),
			PROGRAM_BIT(LinearFog),

			PROGRAM_BIT(AlphaTest),
			PROGRAM_BIT(AlphaHack),
			PROGRAM_BIT(DervMapped),
			PROGRAM_BIT(NoNearZ),
			PROGRAM_BIT(ZoneLight),

			PROGRAM_BIT(VertexParam0),

			// These bits are reserved for Vertex Program ID
			VertexIDMask    = FGL::VertexProgram::BITMASK << (FGL::ProgramBitNumber::MAX-1),

			// Special programs
			SpecialMask     = FGL::SpecialProgram::BITMASK << (FGL::ProgramBitNumber::MAX-1 + FGL::VertexProgram::BITCOUNT),

			// Shader stage bits
			VertexMask      = TextureMask | Color0 | Color1 | ColorGlobal | LinearFog | DervMapped | ZoneLight | VertexParam0 | VertexIDMask | SpecialMask,
			FragmentMask    = TextureMask | Color0 | Color1 | ColorGlobal | ColorCorrection | ColorCorrectionAlpha | LinearFog | AlphaTest | AlphaHack | DervMapped | NoNearZ | ZoneLight | SpecialMask,

			#undef PROGRAM_BIT
		};
	};

	static_assert( FGL::ProgramBitNumber::MAX-1 + FGL::VertexProgram::BITCOUNT + FGL::SpecialProgram::BITCOUNT < 32, "Insufficient program bits");
};


/*-----------------------------------------------------------------------------
	Program helpers.
-----------------------------------------------------------------------------*/

//
// Bitfield Program ID wrapper
//
class FProgramID : public TDataWrapperBitwise<DWORD>
{
public:
	typedef DWORD UINTTYPE;

	FProgramID();
	FProgramID( UINTTYPE InValue);

	UINTTYPE GetTextures() const;
	UINTTYPE GetVertexType() const;
	UINTTYPE GetSpecialProgram() const;

	FProgramID GetVertexProgramID() const;
	FProgramID GetFragmentProgramID() const;

	static FProgramID FromVertexType( FGL::VertexProgram::EVertexProgramID InVertexID);
	static FProgramID FromSpecial( FGL::SpecialProgram::ESpecialProgramID InSpecialID);

	friend DWORD GetTypeHash( FProgramID A);
};


/*-----------------------------------------------------------------------------
	FProgramID.
-----------------------------------------------------------------------------*/

inline FProgramID::FProgramID()
	: TDataWrapperBitwise(0)
{
}

inline FProgramID::FProgramID( UINTTYPE InValue)
	: TDataWrapperBitwise(InValue)
{
}

inline FProgramID::UINTTYPE FProgramID::GetTextures() const
{
	return (Value & FGL::Program::TextureMask);
}

inline FProgramID::UINTTYPE FProgramID::GetVertexType() const
{
	return (Value & FGL::Program::VertexIDMask) >> (FGL::ProgramBitNumber::MAX-1);
}

inline FProgramID::UINTTYPE FProgramID::GetSpecialProgram() const
{
	return (Value & FGL::Program::SpecialMask) >> (FGL::ProgramBitNumber::MAX-1 + FGL::VertexProgram::BITCOUNT);
}

inline FProgramID FProgramID::GetVertexProgramID() const
{
	return FProgramID(Value & FGL::Program::VertexMask);
}

inline FProgramID FProgramID::GetFragmentProgramID() const
{
	return FProgramID(Value & FGL::Program::FragmentMask);
}

inline FProgramID FProgramID::FromVertexType( FGL::VertexProgram::EVertexProgramID InVertexID)
{
	return FProgramID((UINTTYPE)InVertexID << (FGL::ProgramBitNumber::MAX-1));
}

inline FProgramID FProgramID::FromSpecial( FGL::SpecialProgram::ESpecialProgramID InSpecialID)
{
	return FProgramID((UINTTYPE)InSpecialID << (FGL::ProgramBitNumber::MAX-1 + FGL::VertexProgram::BITCOUNT));
}

inline DWORD GetTypeHash( const FProgramID A)
{
	return (DWORD)A.Value^((DWORD)(A.Value>>16))/*^((DWORD)(A.Value>>32))*/;
}
