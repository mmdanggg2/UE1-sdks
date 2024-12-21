/*=============================================================================
	EditorPrivate.h: Unreal editor public header file.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/*----------------------------------------------------------------------------
	API.
----------------------------------------------------------------------------*/
#ifndef EDITOR_API
	#define EDITOR_API DLL_IMPORT
#endif

#if !defined(GET_CLASS_TAG)
#define GET_CLASS_TAG(obj) (EName)obj->GetClass()->GetFName().GetIndex()
#endif

/*-----------------------------------------------------------------------------
	Advance editor private definitions.
-----------------------------------------------------------------------------*/

//
// Quality level for rebuilding Bsp.
//
enum EBspOptimization
{
	BSP_Lame,
	BSP_Good,
	BSP_Optimal
};

//
// Things to set in mapSetBrush.
//
enum EMapSetBrushFlags
{
	MSB_BrushColor	= 1,			// Set brush color.
	MSB_Group		= 2,			// Set group.
	MSB_PolyFlags	= 4,			// Set poly flags.
	MSB_CSGOper		= 8				// Set CSG operation.
};

//
// Possible positions of a child Bsp node relative to its parent (for BspAddToNode).
//
enum ENodePlace
{
	NODE_Back		= 0, // Node is in back of parent              -> Bsp[iParent].iBack.
	NODE_Front		= 1, // Node is in front of parent             -> Bsp[iParent].iFront.
	NODE_Plane		= 2, // Node is coplanar with parent           -> Bsp[iParent].iPlane.
	NODE_Root		= 3, // Node is the Bsp root and has no parent -> Bsp[0].
};

//
// Bsp poly alignment types for polyTexAlign.
//
enum ETexAlign
{
	TEXALIGN_Default		= 0,	// No special alignment (just derive from UV vectors).
	TEXALIGN_Floor			= 1,	// Regular floor (U,V not necessarily axis-aligned).
	TEXALIGN_WallDir		= 2,	// Grade (approximate floor), U,V X-Y axis aligned.
	TEXALIGN_WallPan		= 3,	// Align as wall (V vertical, U horizontal).
	TEXALIGN_OneTile		= 4,	// Align one tile.
	TEXALIGN_WallColumn		= 5,	// Align as wall on column.
	TEXALIGN_WallX			= 6,	// Align to wall around X axis
	TEXALIGN_WallY			= 7,	// Align to wall around Y axis
	TEXALIGN_Clamp			= 8,	// Set alignment for Clamp_to_Edge Textures
};

// Byte describing effects for a mesh triangle.
enum EJSMeshTriType
{
	// Triangle types. Mutually exclusive.
	MTT_Normal				= 0,	// Normal one-sided.
	MTT_NormalTwoSided      = 1,    // Normal but two-sided.
	MTT_Translucent			= 2,	// Translucent two-sided.
	MTT_Masked				= 3,	// Masked two-sided.
	MTT_Modulate			= 4,	// Modulation blended two-sided.
	MTT_AlphaBlend			= 5,	// Alphablend two-sided.
	MTT_Placeholder			= 8,	// Placeholder triangle for positioning weapon. Invisible.
	// Bit flags.
	MTT_Unlit				= 16,	// Full brightness, no lighting.
	MTT_Flat				= 32,	// Flat surface, don't do bMeshCurvy thing.
	MTT_Environment			= 64,	// Environment mapped.
	MTT_NoSmooth			= 128,	// No bilinear filtering on this poly's texture.
};

/*-----------------------------------------------------------------------------
	Editor public includes.
-----------------------------------------------------------------------------*/

#include "Editor.h"

inline DWORD MttToPolyFlags(BYTE MTT)
{
	DWORD PolyFlags = PF_None;
	switch (MTT & 15)
	{
	case MTT_Normal:
		break;
	case MTT_NormalTwoSided:
		PolyFlags = PF_TwoSided;
		break;
	case MTT_Modulate:
		PolyFlags = (PF_TwoSided | PF_Modulated);
		break;
	case MTT_Translucent:
		PolyFlags = (PF_TwoSided | PF_Translucent);
		break;
	case MTT_Masked:
		PolyFlags = (PF_TwoSided | PF_Masked);
		break;
	case MTT_Placeholder:
		PolyFlags = (PF_TwoSided | PF_Invisible);
		break;
	case MTT_AlphaBlend:
		PolyFlags = (PF_TwoSided | PF_AlphaBlend);
		break;
	}

	// Handle effects.
	if (MTT & MTT_Unlit)		PolyFlags |= PF_Unlit;
	if (MTT & MTT_Flat)			PolyFlags |= PF_Flat;
	if (MTT & MTT_Environment)	PolyFlags |= PF_Environment;
	if (MTT & MTT_NoSmooth)		PolyFlags |= PF_NoSmooth;

	return PolyFlags;
}

inline BYTE PolyFlagsToMtt(DWORD PolyFlags)
{
	BYTE Result = 0;

	// Main flags
	if (PolyFlags & PF_Invisible)
		Result = MTT_Placeholder;
	else if (PolyFlags & PF_Masked)
		Result = MTT_Masked;
	else if (PolyFlags & PF_Translucent)
		Result = MTT_Translucent;
	else if (PolyFlags & PF_Modulated)
		Result = MTT_Modulate;
	else if (PolyFlags & PF_TwoSided)
		Result = MTT_NormalTwoSided;
	else Result = MTT_Normal;

	// FX
	if (PolyFlags & PF_Unlit)
		Result |= MTT_Unlit;
	if (PolyFlags & PF_Flat)
		Result |= MTT_Flat;
	if (PolyFlags & PF_Environment)
		Result |= MTT_Environment;
	if (PolyFlags & PF_NoSmooth)
		Result |= MTT_NoSmooth;
	return Result;
}

/*-----------------------------------------------------------------------------
	Editor private includes.
-----------------------------------------------------------------------------*/

#include "FFileManagerArc.h"
#include "UnEdTran.h"
#include "UnTopics.h"
#include "UnStaticMeshTools.h"

EDITOR_API extern class FGlobalTopicTable GTopics;
EDITOR_API extern TArray<UObject*> ForcedTexList;

/*-----------------------------------------------------------------------------
	Editor build config
-----------------------------------------------------------------------------*/
#if BUILD_64
# define DEFAULT_UED_RENDERER_PATHNAME TEXT("OpenGLDrv.OpenGLRenderDevice")
# define DEFAULT_UED_RENDERER_SHORTNAME TEXT("OpenGL Rendering")
#else
# define DEFAULT_UED_RENDERER_PATHNAME TEXT("SoftDrv.SoftwareRenderDevice")
# define DEFAULT_UED_RENDERER_SHORTNAME TEXT("Software Rendering")
#endif

/*-----------------------------------------------------------------------------
	Factories.
-----------------------------------------------------------------------------*/

class EDITOR_API ULevelFactoryNew : public UFactory
{
	DECLARE_CLASS(ULevelFactoryNew,UFactory,0,Editor)
	FStringNoInit LevelTitle;
	FStringNoInit Author;
	BITFIELD CloseExistingWindows;
	ULevelFactoryNew();
	void StaticConstructor();
	void Serialize( FArchive& Ar );
	UObject* FactoryCreateNew( UClass* Class, UObject* InParent, FName Name, DWORD Flags, UObject* Context, FFeedbackContext* Warn );
};

class EDITOR_API UClassFactoryNew : public UFactory
{
	DECLARE_CLASS(UClassFactoryNew,UFactory,0,Editor)
	FName ClassName;
	UPackage* ClassPackage;
	UClass* Superclass;
	UClassFactoryNew();
	void StaticConstructor();
	void Serialize( FArchive& Ar );
	UObject* FactoryCreateNew( UClass* Class, UObject* InParent, FName Name, DWORD Flags, UObject* Context, FFeedbackContext* Warn );
};

class EDITOR_API UTextureFactoryNew : public UFactory
{
	DECLARE_CLASS(UTextureFactoryNew,UFactory,0,Editor)
	FName TextureName;
	UPackage* TexturePackage;
	UClass* TextureClass;
	INT USize;
	INT VSize;
	UTextureFactoryNew();
	void StaticConstructor();
	void Serialize( FArchive& Ar );
	UObject* FactoryCreateNew( UClass* Class, UObject* InParent, FName Name, DWORD Flags, UObject* Context, FFeedbackContext* Warn );
};

class EDITOR_API UClassFactoryUC : public UFactory
{
	DECLARE_CLASS(UClassFactoryUC,UFactory,0,Editor)
	UClassFactoryUC();
	void StaticConstructor();
	UObject* FactoryCreateText( UClass* Class, UObject* InParent, FName Name, DWORD Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn );
};

class EDITOR_API ULevelFactory : public UFactory
{
	DECLARE_CLASS(ULevelFactory,UFactory,0,Editor)
	ULevelFactory();
	void StaticConstructor();
	UObject* FactoryCreateText( UClass* Class, UObject* InParent, FName Name, DWORD Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn );
};

class EDITOR_API UPolysFactory : public UFactory
{
	DECLARE_CLASS(UPolysFactory,UFactory,0,Editor)
	UPolysFactory();
	void StaticConstructor();
	UObject* FactoryCreateText( UClass* Class, UObject* InParent, FName Name, DWORD Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn );
};

class EDITOR_API UModelFactory : public UFactory
{
	DECLARE_CLASS(UModelFactory,UFactory,0,Editor)
	UModelFactory();
	void StaticConstructor();
	UObject* FactoryCreateText( UClass* Class, UObject* InParent, FName Name, DWORD Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn );
};

class EDITOR_API USoundFactory : public UFactory
{
	DECLARE_CLASS(USoundFactory,UFactory,0,Editor)
	INT OggQuality;
	USoundFactory();
	void StaticConstructor();
	UObject* FactoryCreateBinary( UClass* Class, UObject* InParent, FName Name, DWORD Flags, UObject* Context, const TCHAR* Type, const BYTE*& Buffer, const BYTE* BufferEnd, FFeedbackContext* Warn );
};

class EDITOR_API UMusicFactory : public UFactory
{
	DECLARE_CLASS(UMusicFactory,UFactory,0,Editor)
	UMusicFactory();
	void StaticConstructor();
	UObject* FactoryCreateBinary( UClass* Class, UObject* InParent, FName Name, DWORD Flags, UObject* Context, const TCHAR* Type, const BYTE*& Buffer, const BYTE* BufferEnd, FFeedbackContext* Warn );
};

class EDITOR_API UTextureFactory : public UFactory
{
	DECLARE_CLASS(UTextureFactory,UFactory,0,Editor)
	UTextureFactory();
	void StaticConstructor();
	UObject* FactoryCreateBinary( UClass* Class, UObject* InParent, FName Name, DWORD Flags, UObject* Context, const TCHAR* Type, const BYTE*& Buffer, const BYTE* BufferEnd, FFeedbackContext* Warn );
};

class EDITOR_API UTextureFactoryFX : public UFactory
{
	DECLARE_CLASS(UTextureFactoryFX, UFactory, 0, Editor);
	UTextureFactoryFX();
	void StaticConstructor();
	UObject* FactoryCreateText(UClass* Class, UObject* InParent, FName Name, DWORD Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn);
};

class EDITOR_API UFontFactory : public UTextureFactory
{
	DECLARE_CLASS(UFontFactory,UTextureFactory,0,Editor)
	FStringNoInit HDTexture;
	UFontFactory();
	void StaticConstructor();
	UObject* FactoryCreateBinary( UClass* Class, UObject* InParent, FName Name, DWORD Flags, UObject* Context, const TCHAR* Type, const BYTE*& Buffer, const BYTE* BufferEnd, FFeedbackContext* Warn );
};

class EDITOR_API UTrueTypeFontFactory : public UFontFactory
{
	DECLARE_CLASS(UTrueTypeFontFactory,UFontFactory,0,Editor)
	FStringNoInit	FontName;
	INT				Height;
	INT				USize;
	INT				VSize;
	INT				XPad;
	INT				YPad;
	INT				R,G,B;
	INT				CharactersPerPage;
	INT				Count;
	INT				Charset;
	FLOAT			Gamma;
	FStringNoInit	Chars;
	UBOOL			AntiAlias;
	UBOOL			UseGlyphs;
	FString			List;
	FString			Wildcard;
	FString			Path;

	UTrueTypeFontFactory();
	void StaticConstructor();
	UObject* FactoryCreateNew( UClass* Class, UObject* InParent, FName Name, DWORD Flags, UObject* Context, FFeedbackContext* Warn );
};

/*-----------------------------------------------------------------------------
	Exporters.
-----------------------------------------------------------------------------*/

class EDITOR_API UTextBufferExporterTXT : public UExporter
{
	DECLARE_CLASS(UTextBufferExporterTXT,UExporter,0,Editor)
	void StaticConstructor();
	UBOOL ExportText( UObject* Object, const TCHAR* Type, FOutputDevice& Out, FFeedbackContext* Warn );
};

class EDITOR_API USoundExporterWAV : public UExporter
{
	DECLARE_CLASS(USoundExporterWAV,UExporter,0,Editor)
	void StaticConstructor();
	UBOOL ExportBinary( UObject* Object, const TCHAR* Type, FArchive& Ar, FFeedbackContext* Warn );
};

class EDITOR_API UMusicExporterTracker : public UExporter
{
	DECLARE_CLASS(UMusicExporterTracker,UExporter,0,Editor)
	void StaticConstructor();
	UBOOL ExportBinary( UObject* Object, const TCHAR* Type, FArchive& Ar, FFeedbackContext* Warn );
};

class EDITOR_API UClassExporterH : public UExporter
{
	DECLARE_CLASS(UClassExporterH,UExporter,0,Editor)
	INT RecursionDepth;
	UBOOL bExportedHeader;
	TArray<UClass*> SortedClasses;
	void StaticConstructor();
	UBOOL ExportText( UObject* Object, const TCHAR* Type, FOutputDevice& Out, FFeedbackContext* Warn );
};

class EDITOR_API UClassExporterUC : public UExporter
{
	DECLARE_CLASS(UClassExporterUC,UExporter,0,Editor)
	void StaticConstructor();
	UBOOL ExportText( UObject* Object, const TCHAR* Type, FOutputDevice& Ar, FFeedbackContext* Warn );
};

class EDITOR_API UStrippedClassExporterUC : public UExporter
{
	DECLARE_CLASS(UStrippedClassExporterUC,UExporter,0,Editor)
	void StaticConstructor();
	UBOOL ExportText( UObject* Object, const TCHAR* Type, FOutputDevice& Ar, FFeedbackContext* Warn );
};

class EDITOR_API UPolysExporterT3D : public UExporter
{
	DECLARE_CLASS(UPolysExporterT3D,UExporter,0,Editor)
	void StaticConstructor();
	UBOOL ExportText( UObject* Object, const TCHAR* Type, FOutputDevice& Ar, FFeedbackContext* Warn );
};

class EDITOR_API UPolysExporterOBJ : public UExporter
{
	DECLARE_CLASS(UPolysExporterOBJ, UExporter, 0, Editor)
	void StaticConstructor();
	UBOOL ExportText(UObject* Object, const TCHAR* Type, FOutputDevice& Ar, FFeedbackContext* Warn);
};

class EDITOR_API UModelExporterT3D : public UExporter
{
	DECLARE_CLASS(UModelExporterT3D,UExporter,0,Editor)
	void StaticConstructor();
	UBOOL ExportText( UObject* Object, const TCHAR* Type, FOutputDevice& Ar, FFeedbackContext* Warn );
};

class EDITOR_API ULevelExporterT3D : public UExporter
{
	DECLARE_CLASS(ULevelExporterT3D,UExporter,0,Editor)
	void StaticConstructor();
	UBOOL ExportText( UObject* Object, const TCHAR* Type, FOutputDevice& Ar, FFeedbackContext* Warn );
};

class EDITOR_API UObjectExporterT3D : public UExporter
{
	DECLARE_CLASS(UObjectExporterT3D,UExporter,0,Editor)
	void StaticConstructor();
	UBOOL ExportText( UObject* Object, const TCHAR* Type, FOutputDevice& Ar, FFeedbackContext* Warn );
};

class EDITOR_API UTextureExporterPCX : public UExporter
{
	DECLARE_CLASS(UTextureExporterPCX,UExporter,0,Editor)
	void StaticConstructor();
	UBOOL ExportBinary( UObject* Object, const TCHAR* Type, FArchive& Ar, FFeedbackContext* Warn );
};

class EDITOR_API UTextureExporterBMP : public UExporter
{
	DECLARE_CLASS(UTextureExporterBMP,UExporter,0,Editor)
	void StaticConstructor();
	UBOOL ExportBinary( UObject* Object, const TCHAR* Type, FArchive& Ar, FFeedbackContext* Warn );
};

class EDITOR_API UTextureExporterPNG : public UExporter
{
	DECLARE_CLASS(UTextureExporterPNG, UExporter, 0, Editor)
	void StaticConstructor();
	UBOOL ExportBinary(UObject* Object, const TCHAR* Type, FArchive& Ar, FFeedbackContext* Warn);
};

class EDITOR_API UTextureExporterDDS : public UExporter
{
	DECLARE_CLASS(UTextureExporterDDS, UExporter, 0, Editor)
	void StaticConstructor();
	UBOOL ExportBinary(UObject* Object, const TCHAR* Type, FArchive& Ar, FFeedbackContext* Warn);
};

class EDITOR_API UTextureExporterFX : public UExporter
{
	DECLARE_CLASS(UTextureExporterFX, UExporter, 0, Editor);
	void StaticConstructor();
	UBOOL ExportText(UObject* Object, const TCHAR* Type, FOutputDevice& Ar, FFeedbackContext* Warn);
};

class EDITOR_API UMeshAnimExpTXT : public UExporter
{
	DECLARE_CLASS(UMeshAnimExpTXT,UExporter,0,Editor)

	// Constructor.
	UMeshAnimExpTXT(){}

	// UObject interface.
	void StaticConstructor();

	// UExporter interface.
	UBOOL ExportText( UObject* Object, const TCHAR* Type, FOutputDevice& Ar, FFeedbackContext* Warn );
};

class EDITOR_API UStaticMeshExpTXT : public UExporter
{
	DECLARE_CLASS(UStaticMeshExpTXT,UExporter,0,Editor)

	// Constructor.
	UStaticMeshExpTXT(){}

	// UObject interface.
	void StaticConstructor();

	// UExporter interface.
	UBOOL ExportText( UObject* Object, const TCHAR* Type, FOutputDevice& Ar, FFeedbackContext* Warn );
};

class EDITOR_API USkeletalMeshExpPSK : public UExporter
{
	DECLARE_CLASS(USkeletalMeshExpPSK,UExporter,0,Editor)

	// Constructor.
	USkeletalMeshExpPSK(){}

	// UObject interface.
	void StaticConstructor();

	// UExporter interface.
	UBOOL ExportBinary( UObject* Object, const TCHAR* Type, FArchive& Ar, FFeedbackContext* Warn );
};

class EDITOR_API USkeletalAnimExpPSA : public UExporter
{
	DECLARE_CLASS(USkeletalAnimExpPSA,UExporter,0,Editor)

	UBOOL IncludeAnimSeq;

	// Constructor.
	USkeletalAnimExpPSA()
	{
		IncludeAnimSeq = 0;
	}

	// UObject interface.
	void StaticConstructor();

	// UExporter interface.
	UBOOL ExportBinary( UObject* Object, const TCHAR* Type, FArchive& Ar, FFeedbackContext* Warn );
};

//#define OUTPUT_CP850_LATIN1_TABLE

class UFontExporter : public UExporter
{
	DECLARE_CLASS(UFontExporter,UExporter,0,Editor);

	// Static tables.
	static BYTE TTFNaPattern[16][16];
	static INT CodePage850_Latin1[256];
	static INT Latin1_CodePage850[256];

	// Constructors.
	UFontExporter();
	void StaticConstructor();

	// Helper.
	void CleanupLazyLoaders( UTexture* Texture );
	
	// Some heuristic code to detect whether the TrueTypeFontFactory was used for import.
	UBOOL HeuristicTTFCheck( UFont* Font );
	
	// Determines whether AntiAlias option was used during font import.
	UBOOL TTFAntiAliasCheck( UFont* Font );

#if defined(OUTPUT_CP850_LATIN1_TABLE)
	void BuildLatin1ToCodePage850Table();
#endif
	// Checks if Glyph data is all zero.
	UBOOL IsGlyphZero( FFontCharacter& Char );
	
	// Compare two Glyphs.
	UBOOL CompareGlyphs( FFontPage& Page, INT First, INT Second );
	
	// Draws borders around all glyphs.
	void DrawGlyphBorders( TArray<FFontCharacter>& Characters, UTexture* Target );
	
	// Draws Grid.
	void DrawGrid( INT GridUCount, INT GridVCount, INT TileU, INT TileV, UTexture* Target );
  
	UBOOL IsAreaEmpty( UTexture* Texture, INT StartU, INT StartV, INT USize, INT VSize );
	
	// Erases background and blits all glyphs.
	void BlitAllGlyphs( FFontPage& Page, UTexture* Target );
	
	// UExporter interface.
	UBOOL ExportBinary( UObject* Object, const TCHAR* Type, FArchive& Ar, FFeedbackContext* Warn );
};

/*-----------------------------------------------------------------------------
	Commandlets.
-----------------------------------------------------------------------------*/

class EDITOR_API UMakeCommandlet : public UCommandlet
{
	DECLARE_CLASS(UMakeCommandlet,UCommandlet,CLASS_Transient,Editor);
	void StaticConstructor();
	INT Main( const TCHAR* Parms );
	bool FindDependsOn( const TCHAR* FileName, TCHAR* Depancy );
	void BuildFile( const TCHAR* Pkg, UObject* PkgObject, TArray<FString>& FList, INT Index );
};

class EDITOR_API UStripSourceCommandlet : public UCommandlet
{
	DECLARE_CLASS(UStripSourceCommandlet,UCommandlet,CLASS_Transient,Editor);

private:
	void StripText( FString& S );
	
public:
	void StaticConstructor();
	INT Main( const TCHAR* Parms );
};

class EDITOR_API UDumpIntCommandlet : public UCommandlet
{
	DECLARE_CLASS(UDumpIntCommandlet,UCommandlet,CLASS_Transient,Editor);
		
private:
	const TCHAR* GetMutDesc( UClass* C );
	const TCHAR* GetWepDesc( UClass* C );
	void GetExportedObjects(TMultiMap<FString, FString>* Sec, TSingleMap<UClass*>& ClassMap);
	UBOOL ValueIsModified(FString& Value, UClass* CurrentClass, UClass* BaseClass, UProperty* P, INT Index) const;
	UBOOL ValueIsModifiedObj(FString& Value, UObject* Obj, UProperty* P, INT Index) const;
	UBOOL GetObjectPrefs(const TCHAR* S, FString& ObjName, FString& ObjClass, FString& ObjMeta);
	UBOOL HasObjectPrefsMatch(TMultiMap<FString, FString>* Sec, const TCHAR* ObjName, const TCHAR* ObjClass, const TCHAR* ObjMeta);
	UBOOL HasMatchingPreference(const TCHAR* Org, TMultiMap<FString, FString>* Sec);

public:
	void StaticConstructor();
	INT Main(const TCHAR* Parms);
};

class EDITOR_API URipAndTearCommandlet : public UCommandlet
{
	DECLARE_CLASS(URipAndTearCommandlet,UCommandlet,CLASS_Transient,Editor);
	URipAndTearCommandlet();
	void StaticConstructor();
	UBOOL IsRes( UObject* Obj );
	//ripandtear <inpkg> <mappkg> <respkg>
	INT Main( const TCHAR* Parms );
};

class EDITOR_API UDumpMeshInfoCommandlet : public UCommandlet
{
	DECLARE_CLASS(UDumpMeshInfoCommandlet, UCommandlet, CLASS_Transient, Editor);
	UDumpMeshInfoCommandlet();
	void StaticConstructor();
	INT Main(const TCHAR* Parms);
};

class EDITOR_API USaveEmbeddedCommandlet : public UCommandlet
{
	DECLARE_CLASS(USaveEmbeddedCommandlet,UCommandlet,CLASS_Transient,Editor);
	USaveEmbeddedCommandlet();
	void StaticConstructor();
	INT Main( const TCHAR* Parms );
};

class EDITOR_API UMasterCommandlet : public UCommandlet
{
	DECLARE_CLASS(UMasterCommandlet,UCommandlet,CLASS_Transient,Editor);

	// Variables.
	FString GConfigFile, GProduct, GRefPath, GMasterPath, GSrcPath, GArchive;
	FBufferArchive GArchiveData;
	FArchiveHeader GArc;

	// Archive management.
	void LocalCopyFile( const TCHAR* Dest, const TCHAR* Src, DWORD Flags );

	// File diffing.
	struct FLink
	{
		INT Offset;
		FLink* Next;
		FLink( INT InOffset, FLink* InNext )
		: Offset( InOffset ), Next( InNext )
		{}
	};
	enum {ARRAY_SIZE=65536*64};
	enum {MIN_RUNLENGTH=10};
	INT ArrayCrc( const TArray<BYTE>& T, INT Offset );
	void Decompress( TArray<BYTE>& New, TArray<BYTE>& Delta, TArray<BYTE> Old );
	UBOOL DeltaCode( const TCHAR* RefFilename, const TCHAR* MasterFilename, const TCHAR* SrcFilename );

	// Process a group in advance.
	void UpdateGroup( FString MasterPath, const TCHAR* File, const TCHAR* Group, TMultiMap<FString,FString>& Map );

	// Copy a group.
	void CopyGroup( FString MasterPath, const TCHAR* File, const TCHAR* Group, TMultiMap<FString,FString>& Map );

	// Recursively process all groups.
	void ProcessGroup( FString MasterPath, const TCHAR* File, const TCHAR* Group, void(UMasterCommandlet::*Process)( FString MasterPath, const TCHAR* File, const TCHAR* Group, TMultiMap<FString,FString>& Map ) );
	
	void StaticConstructor();
	INT Main( const TCHAR* Parms );
};

class EDITOR_API UUpdateUModCommandlet : public UCommandlet
{
	DECLARE_CLASS(UUpdateUModCommandlet,UCommandlet,CLASS_Transient,Editor);
	INT Main( const TCHAR* Parms );
};

class EDITOR_API UChecksumPackageCommandlet : public UCommandlet
{
	DECLARE_CLASS(UChecksumPackageCommandlet,UCommandlet,CLASS_Transient,Editor);
	INT Main( const TCHAR* Parms );
};

class EDITOR_API UFontPageDiffCommandlet : public UCommandlet
{
	DECLARE_CLASS(UFontPageDiffCommandlet, UCommandlet, CLASS_Transient, Editor);
	UFontPageDiffCommandlet();
	void StaticConstructor();
	INT Main( const TCHAR* Parms );
};

class EDITOR_API UFontCompressCommandlet : public UCommandlet
{
	DECLARE_CLASS(UFontCompressCommandlet, UCommandlet, CLASS_Transient, Editor);
	UFontCompressCommandlet();
	void StaticConstructor();
	INT Main(const TCHAR* Parms);

private:
	INT GetMaxColors(UTexture* T);
	FColor GetMonoColor(UTexture* Tex, UPalette* P);
	void SetMonoColor(UTexture* Tex, BYTE NewIndex);
	void CompressTextures(UObject* Pck);
};

class EDITOR_API UDumpTextureInfoCommandlet : public UCommandlet
{
	DECLARE_CLASS(UDumpTextureInfoCommandlet,UCommandlet,CLASS_Transient,Editor);
	UDumpTextureInfoCommandlet();
	void StaticConstructor();
	INT Main( const TCHAR* Parms );
};

class EDITOR_API UBatchExportCommandlet : public UCommandlet
{
	DECLARE_CLASS(UBatchExportCommandlet,UCommandlet,CLASS_Transient,Editor);
	void StaticConstructor();
	INT Main( const TCHAR* Parms );
};

class EDITOR_API UMusicPackagesCommandlet : public UCommandlet
{
	DECLARE_CLASS(UMusicPackagesCommandlet,UCommandlet,CLASS_Transient|CLASS_Config,Editor);

	TArray<FName> Extensions;
	FString       OutputPath;

	UMusicPackagesCommandlet();
	void StaticConstructor();
	INT Main( const TCHAR* Parms );
};

class EDITOR_API URebuildImportsCommandlet : public UCommandlet
{
	DECLARE_CLASS(URebuildImportsCommandlet,UCommandlet,CLASS_Transient|CLASS_Config,Editor);

	// File extension
	FString TextureFileExtension;
	FString SoundFileExtension;

	// Indention settings for output.
	INT FileIndentation;
	INT NameIndentation;
	INT GroupIndentation;
	INT FlagsIndentation;
	INT LODSetIndentation;
	INT MipsIndentation;

	URebuildImportsCommandlet();
	void StaticConstructor();

	// Checks if Object or it's parents are inside List.
	UBOOL IsInList( UObject* Object, TArray<UObject*> List );

	// Entry point.
	INT Main( const TCHAR* Parms );

	// Generate a single texture import line.
	void RebuildTextureImport( UObject* Package,  UTexture* Texture, FOutputDevice& Ar, TArray<FString>& UnhandledProperties, UBOOL UpkgFormat );

	// Generate a single sound import line.
	void RebuildSoundImport( UObject* Package, USound* Sound, FOutputDevice& Ar );

	// Generate a single mesh import line.
	void RebuildMeshImport( UObject* Package,  UMesh* Mesh, FOutputDevice& Ar );

	// Generate a single font import line.
	void RebuildFontImport( UObject* Package,  UFont* Font, FOutputDevice& Ar, UBOOL UpkgFormat );

	// Some heuristic code to detect whether the TrueTypeFontFactory was used for import.
	UBOOL HeuristicTTFCheck( UFont* Font );

	// Determines whether AntiAlias option was used during font import.
	UBOOL TTFAntiAliasCheck( UFont* Font );
};

class EDITOR_API UTextureMergerCommandlet : public UCommandlet
{
	DECLARE_CLASS(UTextureMergerCommandlet, UCommandlet, CLASS_Transient, Editor);

	UTextureMergerCommandlet();

	struct TextureInformation
	{
		FString PackageName;
		FString GroupName;
		FString FolderName;
		FString TextureName;
	};

	void StaticConstructor();
	INT Main(const TCHAR* Parms);
	void MergePackageTextures(UPackage *Pckg);
	void MergeSingleTexture(TextureInformation TextureInfo, UTexture* Texture);
};

class EDITOR_API UFullBatchExportCommandlet : public UCommandlet
{
	DECLARE_CLASS(UFullBatchExportCommandlet,UCommandlet,CLASS_Transient|CLASS_Config,Editor);

	// Configuration.
	FName DefaultFontExtension;
	FName DefaultMusicExtension;
	FName DefaultSoundExtension;
	FName DefaultTextureExtension;

	UFullBatchExportCommandlet();
	void StaticConstructor();

	// Recursive creates needed directory structure.
	FString CreateTree( UPackage* Pkg, const TCHAR* OutPath, const TCHAR* Dir, UObject* Obj, UBOOL MakeDirectory );

	// Entry point.
	INT Main( const TCHAR* Parms );

	// Some heuristic code to detect whether the TrueTypeFontFactory was used for import
	UBOOL HeuristicTTFCheck( UFont* Font );
	// Determines whether AntiAlias option was used during font import.
	UBOOL TTFAntiAliasCheck( UFont* Font );
};

class EDITOR_API UProdigiosumInParvoCommandlet : public UCommandlet
{
	DECLARE_CLASS(UProdigiosumInParvoCommandlet,UCommandlet,CLASS_Transient, Editor);
	UProdigiosumInParvoCommandlet();
	void StaticConstructor();
	INT ExtractMips( TArray<FMipmap>& Mips, BYTE MipFormat, UPalette* Palette, FString BasePathName, FString OutFormat );
	INT Main( const TCHAR* Parms );
};

class EDITOR_API UBatchMeshExportCommandlet : public UCommandlet
{
	DECLARE_CLASS(UBatchMeshExportCommandlet,UCommandlet,CLASS_Transient|CLASS_Config,Editor);

	FName StudioTextureExtension;
	UBOOL SkipStudioTextureMaps;

	UBatchMeshExportCommandlet();
	void StaticConstructor();
	INT Main( const TCHAR* Parms );
};

class EDITOR_API UReduceTexturesCommandlet : public UCommandlet
{
	DECLARE_CLASS(UReduceTexturesCommandlet,UCommandlet,CLASS_Transient,Editor);
	UReduceTexturesCommandlet();
	void StaticConstructor();
	INT Main( const TCHAR* Parms );
};

class EDITOR_API UAudioPackageCommandlet : public UCommandlet
{
	DECLARE_CLASS(UAudioPackageCommandlet,UCommandlet,CLASS_Transient|CLASS_Config,Editor);

	TArray<FName> Extensions;
	FString       OutputPath;
	
	UAudioPackageCommandlet();
	void StaticConstructor();
	INT ImportDirectory( FString Directory, UPackage* Package, UPackage* Group );
	INT Main( const TCHAR* Parms );
};

class EDITOR_API UPS2ConvertCommandlet : public UCommandlet
{
	DECLARE_CLASS(UPS2ConvertCommandlet,UCommandlet,CLASS_Transient,Editor);
	void StaticConstructor();
	INT Main( const TCHAR* Parms );
};

class EDITOR_API UListObjectsCommandlet : public UCommandlet
{
	DECLARE_CLASS(UListObjectsCommandlet,UCommandlet,CLASS_Transient,Editor);
	UListObjectsCommandlet();
	void StaticConstructor();
	INT Main( const TCHAR* Parms );
};

class EDITOR_API UConformCommandlet : public UCommandlet
{
	DECLARE_CLASS(UConformCommandlet,UCommandlet,CLASS_Transient,Editor);
	void StaticConstructor();
	INT Main( const TCHAR* Parms );
};

class EDITOR_API UCheckUnicodeCommandlet : public UCommandlet
{
	DECLARE_CLASS(UCheckUnicodeCommandlet,UCommandlet,CLASS_Transient,Editor);
	void StaticConstructor();
	INT Main( const TCHAR* Parms );
};

class EDITOR_API UPackageFlagCommandlet : public UCommandlet
{
	DECLARE_CLASS(UPackageFlagCommandlet,UCommandlet,CLASS_Transient,Editor);
	void StaticConstructor();
	INT Main( const TCHAR* Parms );
};

class EDITOR_API UDataRipCommandlet : public UCommandlet
{
	DECLARE_CLASS(UDataRipCommandlet,UCommandlet,CLASS_Transient,Editor);
	void StaticConstructor();
	INT Main( const TCHAR* Parms );
};

/*-----------------------------------------------------------------------------
	Miscellaneous.
-----------------------------------------------------------------------------*/

#if EVOLUTE_VISIBILITY
class EDITOR_API UBitArray : public UObject
{
	DECLARE_CLASS(UBitArray,UObject,0,Editor)
	NO_DEFAULT_CONSTRUCTOR(UBitArray)

	// Variables.
	TArray<DWORD> Data;
	DWORD NumBits;

	// Constructor.
	UBitArray( DWORD InNumBits );

	// UObject interface.
	void Serialize( FArchive& Ar );

	// UBitArray interface.
	UBOOL Get( DWORD i );
	void Set( DWORD i, UBOOL Value );
};

//
// An nxn symmetric bit array.
//
class EDITOR_API UBitMatrix : public UBitArray
{
	DECLARE_CLASS(UBitMatrix,UBitArray,0,Editor)
	NO_DEFAULT_CONSTRUCTOR(UBitMatrix)

	// Variables.
	DWORD Side;

	// Constructor.
	UBitMatrix( DWORD InSide );

	// UObject interface.
	void Serialize( FArchive& Ar );

	// UBitMatrix interface.
	UBOOL Get( DWORD i, DWORD j );
	void Set( DWORD i, DWORD j, UBOOL Value );
};
#endif

// Editor Ini file.
static TCHAR GUEdIni[256];

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
