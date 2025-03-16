/*=============================================================================
	OpenGL_Texture.cpp
	
	Texture format handling

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

#include "OpenGLDrv.h"
#include "UTGLROpenGL.h"

#include "OpenGL_TextureFormat.h"

#define GL_COLOR_INDEX 0x1900


/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

static bool bTextureFormatSetup = false;
static FTextureFormatInfo Formats[256];
static TArray<BYTE> Compose;

static void InitFormat( BYTE Format, GLenum InternalFormat, GLenum SourceFormat=0, GLenum Type=0);
static void InitSharedExp();

static bool FASTCALL ConvertP8_RGBA8( FTextureUploadState& State);
static bool FASTCALL ConvertP8_RGB9_E5( FTextureUploadState& State);
static bool FASTCALL ConvertBGRA7_BGRA8( FTextureUploadState& State);
static bool FASTCALL DispatchConvertP8_RGBA8( FTextureUploadState& State);

static bool PaletteHasAlpha( FColor* Palette);

/*-----------------------------------------------------------------------------
	Texture support query.
-----------------------------------------------------------------------------*/

UBOOL UOpenGLRenderDevice::SupportsTextureFormat( ETextureFormat Format)
{
	// P8 is always supported
	if ( Format == TEXF_P8 )
		return true;

	// Compressed formats require SupportsTC flag
	const FTextureFormatInfo& Desc = FTextureFormatInfo::Get(Format);
	if ( Desc.Compressed && !SupportsTC )
		return false;

	// We don't "support" textures that require software conversion
	return Desc.Supported != 0;
}


/*-----------------------------------------------------------------------------
	Texture format control.
-----------------------------------------------------------------------------*/

void UOpenGLRenderDevice::UpdateTextureFormat( UBOOL& FlushTextures)
{
	guard(UOpenGLRenderDevice::UpdateTextureFormat);

	if ( UseHDTextures != (UBOOL)SupportsTC )
	{
		bTextureFormatSetup = false;
		FlushTextures = true;
	}

	if ( AlwaysMipmap != PL_AlwaysMipmap )
		FlushTextures = true;

	if ( !bTextureFormatSetup )
	{
		bTextureFormatSetup = true;
		FTextureFormatInfo::Init(!!UseHDTextures);
	}

	// Keep a compose buffer just large enough to update fractal textures
	constexpr INT ResetComposeSize = 256 * 256 * sizeof(DWORD);
	if ( Compose.Num() > ResetComposeSize )
	{
		Compose.Empty(); // Empty prevents memory copy
		Compose.SetSize(ResetComposeSize);
	}

	unguard;
}

/*-----------------------------------------------------------------------------
	FTextureFormatInfo implementation.
-----------------------------------------------------------------------------*/

void FTextureFormatInfo::Init( bool bUseTC)
{
	appMemzero(&Formats, sizeof(Formats));

	// Palette (default behaviour is to convert to RGBA8)
	{
		InitFormat(     TEXF_P8,          GL_COLOR_INDEX8_EXT,              GL_COLOR_INDEX,     GL_UNSIGNED_BYTE);
	}

	// BGRA7 Light/fog maps
	{
		InitFormat(     TEXF_BGRA8_LM,    GL_RGBA8,                         GL_BGRA,            GL_UNSIGNED_BYTE);
	}

	// BGR10A2 Light/fog maps
	{
		InitFormat(     TEXF_RGB10A2_LM,  GL_RGB10_A2,                      GL_RGBA,            GL_UNSIGNED_INT_2_10_10_10_REV);
	}

	// Standard RGBA - BGRA
	{
		InitFormat(     TEXF_RGBA8_,      GL_RGBA8,                         GL_RGBA,            GL_UNSIGNED_BYTE);
		InitFormat(     TEXF_BGRA8,       GL_RGBA8,                         GL_BGRA,            GL_UNSIGNED_BYTE);
		InitFormat(     TEXF_RGBA16,      GL_RGBA16,                        GL_RGBA,            GL_UNSIGNED_SHORT);
		InitFormat(     TEXF_RGB10A2,     GL_RGB10_A2,                      GL_RGBA,            GL_UNSIGNED_INT_2_10_10_10_REV);
	}

	// Shared exponent RGB
	if ( FOpenGLBase::SupportsRGB9_E5 )
	{
		InitFormat(     TEXF_RGB9E5,      GL_RGB9_E5_EXT,                   GL_RGB,             GL_UNSIGNED_INT_5_9_9_9_REV_EXT);
		InitSharedExp();
	}

	// R8 (requires GL 3.0)
	if ( (FOpenGLBase::MajorVersion >= 3) && FOpenGLBase::SupportsTextureSwizzle )
		InitFormat(     TEXF_R8,          GL_R8,                            GL_RED,             GL_UNSIGNED_BYTE);

	// S3TC
	if ( FOpenGLBase::SupportsS3TC )
	{
		InitFormat(     TEXF_BC1,         /*GL_COMPRESSED_RGB_S3TC_DXT1_EXT*/GL_COMPRESSED_RGBA_S3TC_DXT1_EXT); //DieHard's HD textures need this
		InitFormat(     TEXF_BC1_PA,      GL_COMPRESSED_RGBA_S3TC_DXT1_EXT);
		InitFormat(     TEXF_BC2,         GL_COMPRESSED_RGBA_S3TC_DXT3_EXT);
		InitFormat(     TEXF_BC3,         GL_COMPRESSED_RGBA_S3TC_DXT5_EXT);
	}
	// RGTC
	if ( FOpenGLBase::SupportsRGTC )
	{
		InitFormat(     TEXF_BC4,         GL_COMPRESSED_RED_RGTC1);
		InitFormat(     TEXF_BC4_S,       GL_COMPRESSED_SIGNED_RED_RGTC1);
		InitFormat(     TEXF_BC5,         GL_COMPRESSED_RG_RGTC2);
		InitFormat(     TEXF_BC5_S,       GL_COMPRESSED_SIGNED_RG_RGTC2);
	}
	// BPTC
	if ( FOpenGLBase::SupportsBPTC )
	{
		InitFormat(     TEXF_BC6H,        GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB);
		InitFormat(     TEXF_BC6H_S,      GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB);
		InitFormat(     TEXF_BC7,         GL_COMPRESSED_RGBA_BPTC_UNORM_ARB);
	}
}

const FTextureFormatInfo& FTextureFormatInfo::Get( GLubyte Format)
{
	return Formats[Format];
}

static void InitFormat( BYTE Format, GLenum InternalFormat, GLenum SourceFormat, GLenum Type)
{
	FTextureFormatInfo& F = Formats[Format];

	F.Type            = Type;
	F.InternalFormat  = InternalFormat;
	F.SourceFormat    = SourceFormat ? SourceFormat : InternalFormat;
	F.BlockWidth      = (GLubyte)FTextureBlockWidth(Format);
	F.BlockHeight     = (GLubyte)FTextureBlockHeight(Format);
	F.BlockBytes      = (GLubyte)FTextureBlockBytes(Format);
	F.Compressed      = FIsCompressedFormat(Format) > 0;
	F.Supported       = InternalFormat != 0;
}

f_TextureConvert FTextureUploadState::SelectHardwareConversion()
{
	GL_DEV_CHECK(Level == 0);

	// Experimental, only run this for bRealtime textures
	if ( (Format == TEXF_P8) && Info->bRealtime && Info->Palette && !(PolyFlags & PF_Masked) && FOpenGLBase::Compute.Programs.PaletteTextureUpdate )
	{
		Format = TEXF_RGBA8_;
		return &DispatchConvertP8_RGBA8;
	}

	return nullptr;
}

f_TextureConvert FTextureUploadState::SelectSoftwareConversion()
{
	GL_DEV_CHECK(Level == 0);

	// This P8 texture won't ever change
	if ( !Info->bRealtime && (InitialFormat == TEXF_P8) )
	{
		// Choose RGB9_E5 if this resembles a decal texture
		if ( FOpenGLBase::SupportsRGB9_E5 && !(PolyFlags & PF_Masked) )
		{
			// See if top left pixel in base mip is grey and check if there's no alpha channel
			FMipmapBase* BaseMip = Info->Mips[CurrentMip];
			if ( BaseMip->DataPtr )
			{
				FColor TopLeft = Info->Palette[*BaseMip->DataPtr];
				if	(	(TopLeft.R == 127 || TopLeft.R == 128) 
					&&	(TopLeft.G == 127 || TopLeft.G == 128)
					&&	(TopLeft.B == 127 || TopLeft.B == 128)
					&&	!PaletteHasAlpha(Info->Palette) )
				{
					Format = TEXF_RGB9E5;
					return &ConvertP8_RGB9_E5;
				}
			}
		}

		// Choose R8 if this is a pure grayscale image (using swizzle)
		if ( FOpenGLBase::SupportsTextureSwizzle )
		{
			if ( PaletteGrayscaleFont(Info->Palette) )
			{
				Format = TEXF_R8;
				Swizzle = FOpenGLSwizzle( SWZ_RED, SWZ_RED, SWZ_RED, SWZ_RED);
				return nullptr;
			}
		}
	}

	// Choose RGBA8
	if ( InitialFormat == TEXF_P8 )
	{
		Format = TEXF_RGBA8_;
		return &ConvertP8_RGBA8;
	}

	// Choose RGBA8 for lightmaps, no conversion needed
	if ( InitialFormat == TEXF_BGRA8_LM )
	{
		if ( FOpenGLBase::SupportsTextureSwizzle )
		{
			Format = TEXF_RGBA8_;
			Swizzle = FOpenGLSwizzle(SWZ_BLUE, SWZ_GREEN, SWZ_RED, SWZ_ALPHA);
		}
		else
			Format = TEXF_BGRA8;
		return nullptr;
	}

	return nullptr;
}

/*-----------------------------------------------------------------------------
	Texture conversion utils.
-----------------------------------------------------------------------------*/

//
// EXT_texture_shared_exponent
//
namespace GLSharedExponent
{
	// Specification
	constexpr INT B = 15; //Exponent bias
	constexpr INT N = 9; //Mantissa bits
	constexpr INT Emax = 30; //Maximum allowed biased exponent value
	constexpr INT SharedExpMax = (2^N-1)/2^N * 2^(Emax-B);

	// Conversion status
	static UBOOL Initialized = 0;
	static FLOAT RcpEP[Emax+1];
	static DWORD ColorMap[NUM_PAL_COLORS];

	static void Initialize()
	{
		Initialized = 1;
		for ( INT i=0; i<Emax+1; i++)
			RcpEP[i] = 1.0f / appPow(2,i-(B+N));
	}

	static DWORD PackColor( FLOAT* RGB_F)
	{
		INT RGB_I[3];
		FLOAT RGB_MAX = Max( RGB_F[0], RGB_F[1], RGB_F[2]) + 0.001f;
		INT ExpShared = Max( -B-1, appFloor(log2(RGB_MAX))) + 1 + B; //Not refined!!!
		for ( INT i=0; i<3; i++)
			RGB_I[i] = appRound( RGB_F[i] * RcpEP[ExpShared]);

		return ((RGB_I[0] & 0b111111111)      )
			|  ((RGB_I[1] & 0b111111111) << 9 )
			|  ((RGB_I[2] & 0b111111111) << 18)
			|  ((ExpShared & 0b11111)    << 27);
	}

	static void GenerateColorMap( FColor* Palette)
	{
		FLOAT RGB_F[3];
		for ( INT i=0; i<NUM_PAL_COLORS; i++)
		{
			FColor& C = Palette[i];
			for ( INT j=0; j<3; j++)
			{
				BYTE& Component = (&C.R)[j];
				if ( Component == 127 || Component == 128 )
					RGB_F[j] = 127.5f / 255.f;
				else
					RGB_F[j] = (FLOAT)(INT)Component / 255.f; //Conversion to INT causes better codegen
			}
			ColorMap[i] = PackColor(RGB_F);
		}
	}
};

static inline void InitSharedExp()
{
	if ( !GLSharedExponent::Initialized )
		GLSharedExponent::Initialize();
}

//
// Expands compose buffer to needed size.
//
bool FASTCALL AllocateCompose( INT USize, INT VSize, BYTE Format)
{
	SQWORD NeededSize = FTextureBytes( Format, USize, VSize);

	// Compatible with TArray
	if ( (NeededSize > 0) && (NeededSize <= MAXINT) )
	{
		// We have the needed memory
		if ( Compose.GetData() && (Compose.Num() >= (INT)NeededSize) )
			return true;

		// Request memory
		Compose.SetSize( (INT)NeededSize );
		if ( Compose.GetData() )
			return true;
	}

//	appErrorf( TEXT("[GL EXT] Failed to allocate Compose memory %i (%i x %i)"), (INT)NeededSize, USize, VSize );
	return false;
}
#define CHECK_ALLOCATE_COMPOSE(usize,vsize,format) \
	if ( !AllocateCompose(usize, vsize, format) ) \
	{ 	return false; }

//
// Checks if any color in palette has alpha transparency.
//
static bool PaletteHasAlpha( FColor* Palette)
{
	for ( INT i=0; i<NUM_PAL_COLORS; i++)
		if ( Palette[i].A != 255 )
			return true;
	return false;
}

//
// Convert a 7 bit color to a 8 bit color
//
static inline DWORD XYZA7_to_XYZA8( DWORD Src)
{
	DWORD ColorDWORD = (Src & 0x7F7F7F7F) << 1;
	ColorDWORD |= (ColorDWORD & 0x80808080) >> 7;
	return ColorDWORD;
}


/*-----------------------------------------------------------------------------
	Texture converters.

	Must be mip/level independent, as in all steps must return the same
	conversion path.
-----------------------------------------------------------------------------*/

static bool FASTCALL ConvertP8_RGBA8( FTextureUploadState& State)
{
	GL_DEV_CHECK(State.Format == TEXF_RGBA8_);

	FMipmapBase& Mip = *State.Info->Mips[State.CurrentMip];

	// Converts to RGBA8
	FColor FirstColor = State.Info->Palette[0];
	bool UseMaskedStorage = FGL::FCacheID(State.Info->CacheID).IsMaskedUTexture();
	if ( UseMaskedStorage )
		State.Info->Palette[0] = FColor(0,0,0,0);

	CHECK_ALLOCATE_COMPOSE( Mip.USize, Mip.VSize, TEXF_RGBA8_);
	BYTE* MapPtr = State.Data;
	BYTE* MapEnd = MapPtr + (PTRINT)(Mip.USize * Mip.VSize);

	State.Data     = &Compose(0);
	DWORD* DestPtr = (DWORD*)State.Data;

	while ( MapPtr < MapEnd )
		*DestPtr++ = GET_COLOR_DWORD(State.Info->Palette[*MapPtr++]);

	if ( UseMaskedStorage )
		State.Info->Palette[0] = FirstColor;

	return true;
}

static bool FASTCALL ConvertP8_RGB9_E5( FTextureUploadState& State)
{
	GL_DEV_CHECK(State.Format == TEXF_RGB9E5);

	if ( State.Level == 0 )
		GLSharedExponent::GenerateColorMap(State.Info->Palette);

	FMipmapBase& Mip = *State.Info->Mips[State.CurrentMip];

	CHECK_ALLOCATE_COMPOSE( Mip.USize, Mip.VSize, TEXF_RGB9E5);
	BYTE* MapPtr = State.Data;
	BYTE* MapEnd = MapPtr + (PTRINT)(Mip.USize * Mip.VSize);

	State.Data     = &Compose(0);
	DWORD* DestPtr = (DWORD*)State.Data;

	while ( MapPtr < MapEnd )
		*DestPtr++ = GLSharedExponent::ColorMap[*MapPtr++];

	return true;

}

static bool FASTCALL ConvertBGRA7_BGRA8( FTextureUploadState& State)
{
	GL_DEV_CHECK(State.Format == TEXF_BGRA8);

	FMipmapBase& Mip = *State.Info->Mips[State.CurrentMip];

	CHECK_ALLOCATE_COMPOSE( Mip.USize, Mip.VSize, TEXF_BGRA8);
	DWORD* SourcePtr = (DWORD*)State.Data;
	DWORD* SourceEnd = SourcePtr + (PTRINT)(Mip.USize*Mip.VSize);
	
	State.Data     = &Compose(0);
	DWORD* DestPtr = (DWORD*)State.Data;

	while ( SourcePtr < SourceEnd )
		*DestPtr++ = XYZA7_to_XYZA8(*SourcePtr++);

	return true;
}

static bool FASTCALL DispatchConvertP8_RGBA8( FTextureUploadState& State)
{
	GL_DEV_CHECK(State.Format == TEXF_RGBA8_);

	FMipmapBase& Mip = *State.Info->Mips[State.CurrentMip];

	// Upload raw texture data
	GLuint Buffer = 0;
	GLsizei Size = Mip.USize * Mip.VSize;
	FOpenGLBase::CreateBuffer(Buffer, GL_SHADER_STORAGE_BUFFER, Size, Mip.DataPtr, GL_STREAM_DRAW);

	auto& PaletteRemap = UOpenGLRenderDevice::PalettePool.GetRemap(State.Info->PaletteCacheID, State.Info->Palette);

	auto* GL = FOpenGLBase::ActiveInstance;
	GL->SetComputePaletteTextureUpdate(State.Texture->Texture, State.Layer, Buffer, PaletteRemap.SSBO, PaletteRemap.Offset, false);

	// The texture write is not coherent, which is great because we DON'T want to wait
	// on it from the rendering commands, this gives the driver the liberty to reorder
	// the compute pipeline executions as it sees fit
	GLuint Workgroups_X = Mip.USize / 8;
	GLuint Workgroups_Y = Mip.VSize / 8;
	GLuint Workgroups_Z = 1;
	FOpenGLBase::glDispatchCompute(Workgroups_X, Workgroups_Y, Workgroups_Z);

	// The buffer is being destroyed so this binding point needs to be cleared
	GL->BindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, 0);

	FOpenGLBase::glDeleteBuffers(1, &Buffer);

	return true;
}
