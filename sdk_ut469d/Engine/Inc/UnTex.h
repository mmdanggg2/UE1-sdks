/*=============================================================================
	UnTex.h: Unreal texture related classes.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack(push,OBJECT_ALIGNMENT)
#endif

/*-----------------------------------------------------------------------------
	Constants.
-----------------------------------------------------------------------------*/

enum {NUM_PAL_COLORS=256};	// Number of colors in a standard palette.

// Constants.
enum
{
	EHiColor565_R = 0xf800,
	EHiColor565_G = 0x07e0,
	EHiColor565_B = 0x001f,

	EHiColor555_R = 0x7c00,
	EHiColor555_G = 0x03e0,
	EHiColor555_B = 0x001f,

	ETrueColor_R  = 0x00ff0000,
	ETrueColor_G  = 0x0000ff00,
	ETrueColor_B  = 0x000000ff,
};

/*-----------------------------------------------------------------------------
	UPalette.
-----------------------------------------------------------------------------*/

extern ENGINE_API FPlane FGetHSV( BYTE H, BYTE S, BYTE V );

//
// A palette object.  Holds NUM_PAL_COLORS unique FColor values, 
// forming a 256-color palette which can be referenced by textures.
//
class ENGINE_API UPalette : public UObject
{
	DECLARE_CLASS(UPalette,UObject,CLASS_SafeReplace,Engine)

	// Variables.
	TArray<FColor> Colors;

	// Constructors.
	UPalette();
	UPalette( const TArray<FColor>& InColorMap);

	// UObject interface.
	void Serialize( FArchive& Ar );

	// UPalette interface.
	BYTE BestMatch( FColor Color, INT First );
	UPalette* ReplaceWithExisting();
	void FixPalette();
	UBOOL Matches( UPalette* Other);
};

/*-----------------------------------------------------------------------------
	UTexture and FTextureInfo.
-----------------------------------------------------------------------------*/

// Texture level-of-detail sets.
enum ELODSet
{
	LODSET_None  = 0,  // No level of detail mipmap tossing.
	LODSET_World = 1,  // World level-of-detail set.
	LODSET_Skin  = 2,  // Skin level-of-detail set.
	LODSET_MAX   = 8,  // Maximum.
};

enum {MAX_TEXTURE_LOD=4};

//
// Base mipmap.
//
struct ENGINE_API FMipmapBase
{
public:
	BYTE*			DataPtr;		// Pointer to data, valid only when locked.
	INT				USize,  VSize;	// Power of two tile dimensions.
	BYTE			UBits,  VBits;	// Power of two tile bits.

	FMipmapBase( BYTE InUBits, BYTE InVBits )
	:	DataPtr		(0)
	,	USize		(1<<InUBits)
	,	VSize		(1<<InVBits)
	,	UBits		(InUBits)
	,	VBits		(InVBits)
	{}
	FMipmapBase()
	:	DataPtr		(0)
	,	USize		(0)
	,	VSize		(0)
	,	UBits		(0)
	,	VBits		(0)
	{}
};

//
// Texture mipmap.
//
struct ENGINE_API FMipmap : public FMipmapBase
{
public:
	TLazyArray<BYTE> DataArray; // Data.
	FMipmap()
	{}
	FMipmap( BYTE InUBits, BYTE InVBits )
	:	FMipmapBase( InUBits, InVBits )
	,	DataArray( USize * VSize )
	{}
	FMipmap( BYTE InUBits, BYTE InVBits, INT InSize )
	:	FMipmapBase( InUBits, InVBits )
	,	DataArray( InSize )
	{}
	void Clear()
	{
		guard(FMipmap::Clear);
		appMemzero( &DataArray(0), DataArray.Num() );
		unguard;
	}
	friend FArchive& operator<<( FArchive& Ar, FMipmap& M )
	{
		guard(FMipmap<<);
		Ar << M.DataArray;
		Ar << M.USize << M.VSize << M.UBits << M.VBits;
		return Ar;
		unguard;
	}
	void LoadMip()
	{
		DataArray.Load();
		DataPtr = DataArray.Num() > 0 ? &DataArray(0) : nullptr;
	}
	void UnloadMip()
	{
		DataArray.Unload();
		DataPtr = nullptr;
	}
};

//
// Texture clearing flags.
//
enum ETextureClear
{
	TCLEAR_Temporal	= 1,	// Clear temporal texture effects.
	TCLEAR_Bitmap   = 2,    // Clear the immediate bitmap.
};

//
// A low-level bitmap.
//
class ENGINE_API UBitmap : public UObject
{
	DECLARE_ABSTRACT_CLASS(UBitmap,UObject,0,Engine)

	// General bitmap information.
	BYTE		Format GCC_PACK(INT_ALIGNMENT);				// ETextureFormat.
	UPalette*	Palette;			// Palette if 8-bit palettized.
	BYTE		UBits, VBits;		// # of bits in USize, i.e. 8 for 256.
	INT			USize, VSize;		// Size, must be power of 2.
	INT			UClamp, VClamp;		// Clamped width, must be <= size.
	FColor		MipZero;			// Overall average color of texture.
	FColor		MaxColor;			// Maximum color for normalization.

	//
	// stijn: LastUpdateTime is mirrored as int InternalTime[2] in UScript. This is fine
	// for 32-bit builds, where we force 4-byte packing, but it doesn't really work in
	// 64-bit builds, where we use natural alignment.
	//
	// In 64-bit builds, the compiler will align FTime structs to an 8-byte boundary, but
	// int[] arrays will be aligned on a 4-byte boundary. In the case of UBitmap, this
	// causes a UScript/C++ mismatch since the UProperty code will calculate the 
	// Engine.Bitmap.InternalTime offset as 116, while the C++ compiler will store
	// LastUpdateTime at offset 120.
	//
	// Fortunately, LastUpdateTime is the last variable in the UBitmap class, so the padding
	// at the end of the class will ensure that the UBitmap child classes (e.g., UTexture)
	// properly match their UScript counterparts.
	//
	// Debug output (64-bit):
	//
	// Linked property IntProperty Engine.Bitmap.InternalTime - offset 116 - size 8
	// Struct size alignment : 124 = > 128 (alignment 8)
	// # Linking Struct Class Engine.Texture
	// Inheritance super Class Engine.Bitmap
	// Linked property ObjectProperty Engine.Texture.BumpMap - offset 128 - size 8
	//
	// Compiler output (64-bit):
	//
	// (unsigned long)&((Engine.dll!UBitmap*)0)->LastUpdateTime = 120
	// (unsigned long)&((Engine.dll!UTexture*)0)->BumpMap = 128
	//
	// Debug output (32-bit):
	//
	// Linked property IntProperty Engine.Bitmap.InternalTime - offset 76 - size 8
	// # Linking Struct Class Engine.Texture
	// Inheritance super Class Engine.Bitmap
	// Linked property ObjectProperty Engine.Texture.BumpMap - offset 84 - size 4
	//
	// Compiler output (32-bit):
	//
	// (unsigned long)&((Engine.dll!UBitmap*)0)->LastUpdateTime = 76
	// (unsigned long)&((Engine.dll!UTexture*)0)->BumpMap = 84
	//
	FTime		LastUpdateTime;		// Last time texture was locked for rendering.

	// Static.
	static class UClient* __Client;

	// Constructor.
	UBitmap();

	// UBitmap interface.
	virtual void Lock( FTextureInfo& TextureInfo, FTime Time, INT LOD, URenderDevice* RenDev )=0;
	virtual void Unlock( FTextureInfo& TextureInfo )=0;
	virtual FMipmapBase* GetMip( INT i )=0;

    // I don't think these are used anymore.  --ryan.
	//FTime GetLastUpdateTime() {FTime T; appMemcpy(&T,&__LastUpdateTime,sizeof(FTime)); return T;}
	//void SetLastUpdateTime(FTime T) {appMemcpy(&__LastUpdateTime,&T,sizeof(FTime));}
};

//
// A complex material texture.
//
class ENGINE_API UTexture : public UBitmap
{
	DECLARE_CLASS(UTexture,UBitmap,CLASS_SafeReplace,Engine)

	// Subtextures.
	UTexture*	BumpMap;			// Bump map to illuminate this texture with.
	UTexture*	DetailTexture;		// Detail texture to apply.
	UTexture*	MacroTexture;		// Macrotexture to apply, not currently used.

	// Surface properties.
	FLOAT		Diffuse;			// Diffuse lighting coefficient (0.f-1.f).
	FLOAT		Specular;			// Specular lighting coefficient (0.f-1.f).
	FLOAT		Alpha;				// Reflectivity (0.f-0.1f).
	FLOAT       Scale;              // Scaling relative to parent, 1.f=normal.
	FLOAT		Friction;			// Surface friction coefficient, 1.f=none, 0.95f=some.
	FLOAT		MipMult;			// Mipmap multiplier.

	// Sounds.
	USound*		FootstepSound;		// Footstep sound.
	USound*		HitSound;			// Sound when the texture is hit with a projectile.

	// Flags.
	DWORD		PolyFlags;			// Polygon flags to be applied to Bsp polys with texture (See PF_*).
	BITFIELD	bHighColorQuality:1; // High color quality hint.
	BITFIELD	bHighTextureQuality:1; // High color quality hint.
	BITFIELD	bRealtime:1;        // Texture changes in realtime.
	BITFIELD	bParametric:1;      // Texture data need not be stored.
	BITFIELD	bRealtimeChanged:1; // Changed since last render.
	BITFIELD    bHasComp:1;         // Compressed version included?
	
	BYTE        LODSet GCC_PACK(INT_ALIGNMENT); // Level of detail type.

	// Animation related.
	UTexture*	AnimNext;			// Next texture in looped animation sequence.
	UTexture*	AnimCur;			// Current animation frame.
	BYTE		PrimeCount;			// Priming total for algorithmic textures.
	BYTE		PrimeCurrent;		// Priming current for algorithmic textures.
	FLOAT		MinFrameRate;		// Minimum animation rate in fps.
	FLOAT		MaxFrameRate;		// Maximum animation rate in fps.
	FLOAT		Accumulator;		// Frame accumulator.

	// Table of mipmaps.
	TArray<FMipmap> Mips;			// Mipmaps in native format.
	TArray<FMipmap> CompMips;		// Mipmaps in requested format.
	BYTE            CompFormat;     // Decompressed texture format.
	FMipmap*		SourceMip;		// Original uncompressed texture in BGRA8 format (only valid after texture import!).

	// stijn: OldUnreal's XOpenGLDrv can achieve significantly better rendering
	// performance by storing bindless texture metadata directly in the UTexture.
	// In contexts where multiple render devices are sharing the same UTexture
	// (i.e. in UEd), this is not safe to use!
	void* TextureHandle;

	// Higor/stijn: How many times has the pixel data for this texture changed?
	// OldUnreal v469b uses this field to reliably track when a texture has
	// changed and when it should, therefore, be re-uploaded to the GPU.
	// OldUnreal v469a and earlier UT patches used the bRealtimeChanged
	// field instead. However, this had a fundamental drawback as the gets
	// reset to false whenever you call UTexture::Lock. Thus, if you lock a
	// texture at least once before your renderer inspects the bRealtimeChanged
	// field, that renderer will fail to see the texture has changed and it
	// will continue to use stale texture data.
	INT RealtimeChangeCount;

	// Constructor.
	UTexture();

	// UObject interface.
	void Serialize( FArchive& Ar );
	const TCHAR* Import( const TCHAR* Buffer, const TCHAR* BufferEnd, const TCHAR* FileType );
	void Export( FArchive& Ar, const TCHAR* FileType, INT Indent );
	void PostLoad();
	void Destroy();

	// UBitmap interface.
	DWORD GetColorsIndex()
	{
		return Palette->GetIndex();
	}
	FColor* GetColors()
	{
		return Palette ? &Palette->Colors(0) : NULL;
	}
	INT GetNumMips()
	{
		return Mips.Num();
	}
	FMipmapBase* GetMip( INT i )
	{
		return &Mips(i);
	}
	void Lock( FTextureInfo& TextureInfo, FTime Time, INT LOD, URenderDevice* RenDev );
	void Unlock( FTextureInfo& TextureInfo );

	// UTexture interface.
	virtual void Clear( DWORD ClearFlags );
	virtual void Init( INT InUSize, INT InVSize );
	virtual void Tick( FLOAT DeltaSeconds );
	virtual void ConstantTimeTick();
	virtual void MousePosition( DWORD Buttons, FLOAT X, FLOAT Y ) {}
	virtual void Click( DWORD Buttons, FLOAT X, FLOAT Y ) {}
	virtual void Update( FTime Time );
	virtual void Prime();

	// UTexture functions.
	void BuildRemapIndex( UBOOL Masked );
	void CreateMips( UBOOL FullMips, UBOOL Downsample );
	void CreateColorRange();
	UBOOL Compress( ETextureFormat Format, UBOOL Mipmap );
	UBOOL Decompress( ETextureFormat Format );
	INT DefaultLOD();

	// UTexture accessors.
	UTexture* Get( FTime Time )
	{
		Update( Time );
		return AnimCur ? AnimCur : this;
	}

	UBOOL CreateSourceMip();
	UBOOL ConvertFormat(ETextureFormat Dest, BOOL bMipmap);

	// Run Heckbert Quantize to turn an RGBA8 texture into P8.
	// Result array must be NUM_PAL_COLORS wide.
	static INT Palettize(const FMipmap* Src, BYTE* Dest, FColor* Result);

	//
	// Run ISPCTextureCompressor to compress to BC1/2/3/7 format
	// SourceFormat - Format of source texture
	// Src - Source mipmap
	// DestFormat - Destination format
	// DestSize - Number of bits returned.
	// Result - Result bits.
	// 
	static BYTE* CompressMip(ETextureFormat SourceFormat, const FMipmap* Src, ETextureFormat DestFormat, INT& DestSize);
	//
	// Decompress from BC1/2/3/7 to RGBA8
	// 
	static TArray<BYTE> DecompressMip(ETextureFormat SourceFormat, const FMipmap* Src, ETextureFormat DestFormat);

	//
	// Converts a base mip in textureformat @Src into a set of mips in textureformat @Dest.
	// We use the RGBA8 format as an intermediate format for all conversions where neither @Src nor @Dest are RGBA8.
	// 
	static TArray<FMipmap>* ConvertMip(ETextureFormat Src, ETextureFormat Dest, FMipmap* Mip, UBOOL bMipmap, UObject* Obj, UPalette*& Palette, BOOL Masked);
};

//
// Information about a locked texture. Used for ease of rendering.
//
enum {MAX_MIPS=12};
struct ENGINE_API FTextureInfo
{
	friend class UBitmap;
	friend class UTexture;

	// Variables.
	UTexture*		Texture;				// Optional texture.
	QWORD			CacheID;				// Unique cache ID.
	QWORD			PaletteCacheID;			// Unique cache ID of palette.
	FVector			Pan;					// Panning value relative to texture planes.
	FColor*			MaxColor;				// Maximum color in texture and all its mipmaps.
	ETextureFormat	Format;					// Texture format.
	FLOAT			UScale;					// U Scaling.
	FLOAT			VScale;					// V Scaling.
	INT				USize;					// Base U size.
	INT				VSize;					// Base V size.
	INT				UClamp;					// U clamping value, or 0 if none.
	INT				VClamp;					// V clamping value, or 0 if none.
	INT				NumMips;				// Number of mipmaps.
	INT				LOD;					// Level of detail, 0=highest.
	FColor*			Palette;				// Palette colors.
	BITFIELD		bHighColorQuality:1;	// High color quality hint.
	BITFIELD		bHighTextureQuality:1;	// High color quality hint.
	BITFIELD		bRealtime:1;			// Texture changes in realtime.
	BITFIELD		bParametric:1;			// Texture data need not be stored.
	BITFIELD		bRealtimeChanged:1;		// Changed since last render.
	FMipmapBase*	Mips[MAX_MIPS];			// Array of NumMips of mipmaps.

	// Functions.
	void Load();
	void Unload();
	void CacheMaxColor();

	// Returns true if texture needs to be updated for a render device (updates the device's record using texture's RealtimeChangeCount)
	UBOOL NeedsRealtimeUpdate( INT& RealtimeChangeRecord)
	{
		if ( !Texture )
			return bRealtimeChanged;
		if ( Texture->RealtimeChangeCount == RealtimeChangeRecord )
			return false;
		RealtimeChangeRecord = Texture->RealtimeChangeCount;
		return true;
	}
};

/*-----------------------------------------------------------------------------
	UFont.
-----------------------------------------------------------------------------*/

// Font constants.
enum {NUM_FONT_PAGES=256};
enum {NUM_FONT_CHARS=256};

//
// Information about one font glyph which resides in a texture.
//
struct ENGINE_API FFontCharacter
{
	// Variables.
	INT StartU, StartV;
	INT USize, VSize;

	// Serializer.
	friend FArchive& operator<<( FArchive& Ar, FFontCharacter& Ch )
	{
		guard(FFontCharacter<<);
		return Ar << Ch.StartU << Ch.StartV << Ch.USize << Ch.VSize;
		unguard;
	}
};

//
// A font page.
//
struct ENGINE_API FFontPage
{
	// Variables.
	UTexture* Texture;
	TArray<FFontCharacter> Characters;

	// Serializer.
	friend FArchive& operator<<( FArchive& Ar, FFontPage& Ch )
	{
		guard(FFontPage<<);
		return Ar << Ch.Texture << Ch.Characters;
		unguard;
	}
};

//
// OldUnreal font scaling.
//
enum EFontFamily
{
	FF_Arial,
	FF_Times,
	FF_Courier,
	FF_Tahoma
};

enum EFontProperties
{
	FP_Bold			= 0x01,
	FP_Italic		= 0x02,
	FP_Underlined	= 0x04,
	FP_DPIScaled	= 0x10,
	FP_AntiAliased	= 0x20
};

struct ENGINE_API FDynamicFontInfo
{	
	FString			UniqueName;
	EFontFamily		Family;
	INT				Height;
	EFontProperties	Props;
	UFont*			Font;
};

//
// A font object, containing information about a set of glyphs.
// The glyph bitmaps are stored in the contained textures, while
// the font database only contains the coordinates of the individual
// glyph.
//
class ENGINE_API UFont : public UObject
{
	DECLARE_CLASS(UFont,UObject,0,Engine)

	// Variables.
	INT CharactersPerPage;
	TArray<FFontPage> Pages;
	TMap<_WORD,_WORD> CharRemap;
	UBOOL IsRemapped;

	// Constructors.
	UFont();

	// UObject interface.
	void Serialize( FArchive& Ar );

	// UFont interface
	TCHAR RemapChar(TCHAR ch)
	{
		_WORD *p;
		if( !IsRemapped )
			return ch;
		p = CharRemap.Find(static_cast<_WORD>(ch));
		return p ? *p : 32; // return space if not found.
	}
};

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (pop)
#endif

/*----------------------------------------------------------------------------
	The End.
----------------------------------------------------------------------------*/
