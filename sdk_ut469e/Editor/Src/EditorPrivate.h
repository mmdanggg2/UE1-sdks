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


// Byte describing effects for a mesh triangle.
enum EJSMeshTriType
{
	// Triangle types. Mutually exclusive.
	MTT_Normal				= 0,	// Normal one-sided.
	MTT_NormalTwoSided      = 1,    // Normal but two-sided.
	MTT_Translucent			= 2,	// Translucent two-sided.
	MTT_Masked				= 3,	// Masked two-sided.
	MTT_Modulate			= 4,	// Modulation blended two-sided.
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

/*-----------------------------------------------------------------------------
	Editor private includes.
-----------------------------------------------------------------------------*/

#include "UnEdTran.h"
#include "UnTopics.h"
#include "FFileManagerArc.h"
#include "UnPixelReader.h"

EDITOR_API extern class FGlobalTopicTable GTopics;
EDITOR_API extern TArray<UTexture*> ForcedTexList;
EDITOR_API extern TMap<UClass*,TArray<FName>> GClassDependencyList;

EDITOR_API FMovingBrushTrackerBase* GNewEditorBrushTracker(ULevel* Level);

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

class EDITOR_API UTextureFactory : public UFactory
{
	DECLARE_CLASS(UTextureFactory, UFactory, 0, Editor)
	UTextureFactory();
	void StaticConstructor();
	UObject* FactoryCreateBinary(UClass* Class, UObject* InParent, FName Name, DWORD Flags, UObject* Context, const TCHAR* Type, const BYTE*& Buffer, const BYTE* BufferEnd, FFeedbackContext* Warn);

	UBOOL   DoMips;
	FString Format;
	static DWORD NewTexturePolyFlags;
	static FName NewPaletteName;

	UBOOL ImportBMP( FImageImporter& Info);
	UBOOL ImportPCX( FImageImporter& Info);
	UBOOL ImportDDS( FImageImporter& Info);
};

class EDITOR_API UFontFactory : public UTextureFactory
{
	DECLARE_CLASS(UFontFactory, UTextureFactory, 0, Editor)
	UFontFactory();
	void StaticConstructor();
	UObject* FactoryCreateBinary(UClass* Class, UObject* InParent, FName Name, DWORD Flags, UObject* Context, const TCHAR* Type, const BYTE*& Buffer, const BYTE* BufferEnd, FFeedbackContext* Warn);
};

//
// Iterates over all glyphs, checking how many pages of the specified Width and Height we're going to need
// This also returns the minimum number of characters per page.
// NOTE: If we need multiple pages, then MinCharactersPerPage does not consider the last page.
//
inline INT CalculateFontPageCount
(
	TArray<FGlyphInfo>& Glyphs,
	INT MaxPageWidth,
	INT MaxPageHeight,
	INT XPad,
	INT YPad,
	INT& MinCharactersPerPage
)
{
	//debugf(NAME_DevGraphics, TEXT("Calculating Font Page Count - %dx%d"), MaxPageWidth, MaxPageHeight);
	INT PageCount = 1, OrigMinCharactersPerPage = 1, Iteration = -1;
	MinCharactersPerPage = 1 << appCeilLogTwo(Glyphs.Num());
	
	// might need multiple iterations to reach a fix point
	while (OrigMinCharactersPerPage != MinCharactersPerPage)
	{
		INT CharactersOnThisPage = 0;
		INT RowWidth = 0, RowHeight = 0;
		INT PageHeight = 0;
		INT Rows = 0;		
		OrigMinCharactersPerPage = MinCharactersPerPage;
		PageCount = 1;
		Iteration++;
		for (INT i = 0; i < Glyphs.Num(); ++i)
		{
			FGlyphInfo& Info = Glyphs(i);

			if (Info.CellWidth + 2 * XPad >= MaxPageWidth || 
				Info.CellHeight + 2 * YPad >= MaxPageHeight)
				return -1;

			if (PageHeight + Info.CellHeight + 2 * YPad >= MaxPageHeight ||
				CharactersOnThisPage >= MinCharactersPerPage)
			{
				//debugf(NAME_DevGraphics, TEXT("Glyph %d - Height %d too high for page %d"), i, Info.BlitHeight, PageCount - 1);
				// Advance to the next page
				//debugf(NAME_DevGraphics, TEXT("Iteration %d - Page %d - Rows %d - Characters on page %d"), 
				//	Iteration, PageCount - 1, Rows, CharactersOnThisPage);
				
				if (CharactersOnThisPage < MinCharactersPerPage)
				{
					MinCharactersPerPage = CharactersOnThisPage;
					break;
				}
				CharactersOnThisPage = 0;
				PageHeight = 0;
				RowWidth = 0;
				RowHeight = 0;
				Rows = 0;
				PageCount++;
				i--;
				continue;
			}

			RowWidth += XPad * 2 + Info.CellWidth;

			if (RowWidth >= MaxPageWidth)
			{
				//debugf(NAME_DevGraphics, TEXT("Glyph %d - Width %d too wide for page %d"), i, Info.BlitWidth, PageCount - 1);
				// Advance to the next row
				PageHeight += RowHeight;
				RowWidth = 0;
				RowHeight = 0;
				i--;
				Rows++;
				continue;
			}

			// If it fits, it sits
			RowHeight = Max(RowHeight, YPad * 2 + Info.CellHeight);
			CharactersOnThisPage++;
		}
	}

	return PageCount;
}

// stijn: I copied notes from UDN to explain what the various parameters do
class EDITOR_API UTrueTypeFontFactory : public UTextureFactory
{
	DECLARE_CLASS(UTrueTypeFontFactory, UTextureFactory, 0, Editor)

	// This is the name of the True Type font used to generate this font. 
	// 
	// Note from stijn: The Unix implementation of this factory will default 
	// to OpenSans if the requested font is not available.
	FStringNoInit	FontName;

	// The height of the characters for this font. Think of this as the True 
	// Type font size 
	INT				Height;

	// This is the size, in the horizontal and vertical dimensions respectively, 
	// of texture to put the font on. If the font doesn't need a texture as large 
	// as specified, it will use something smaller. 
	//
	// Note from stijn: In UT, this specifies the size of FontPage textures. We
	// can still create multiple pages if the glyphs won't fit onto one page
	INT				USize;
	INT				VSize;

	// These change the spacing of the characters in the texture but not when it 
	// is printed.
	INT				XPad;
	INT				YPad;

	// stijn: UT exclusive. Adjusts the position of glyphs within their glyph boxes
	// 
	// We currently need this because:
	// 1) The font rendering APIs we use (i.e., GetTextExtentPoint and/or DrawText)
	// generate slightly larger glyph box sizes than they did back in WinXP days
	// 2) Some mods (including our current implementation of UMenuLookAndFeel) have
	// hardcoded font heights
	INT				YAdjust;

	// Note from stijn: we no longer use this
	INT				CharactersPerPage;

	// This is used to set the boldness of the font in an odd sort of way. 
	// (500=normal, 600=bold, 700=very bold) 
	INT				Style;

	// 	This is never referenced in code and therefore doesn't do anything 
	FLOAT			Gamma;

	// Normally the font will import the standard 256 characters of the font. If 
	// you use this option only the characters that you pass in this string will 
	// be part of the font. For example if you wanted a font just for numbers you 
	// could pass Chars="1234567890". 
	FStringNoInit	Chars;

	// 	If this set to 1 (true) the font will be anti-aliased 
	UBOOL			AntiAlias;

	// If this set to 1 (true) the font will be italic 
	UBOOL			Italic;

	// If this set to 1 (true) the font will be underlined
	UBOOL			Underline;

	// Note from stijn: this is a UT exclusive. If this is set to 1 (true) the 
	// height will be multiplied by the desktop DPI scaling value
	UBOOL			DPIScaled;

	// Note from stijn: we no longer use this
	FString			List;

	// This is a path and filename/wildcard that are combined to make a list of 
	// files that will be read and all the characters in the files will be put 
	// in the font. (for example: Path="." WildCard="MyChars.*") This is useful 
	// for Unicode so you do not have type in all the codes. Simply make a file 
	// that includes the Unicode characters you want (by pasting from the 
	// Character Map for example) and save it in "Unicode - Codepage 1200" 
	// format. This can be done in Dev Studio by pulling down the arrow next to 
	// the save button in the Save-As dialog. This does not affect the standard 
	// character set which is included by default. 
	FString			Wildcard;
	FString			Path;

	// This will append Unicode characters to the set of characters this font 
	// would normally create. Unicode characters are given in hex in comma 
	// separated lists of ranges. If you want on specific character you must give 
	// it as a range from itself to itself. For example: Chars="-" 
	// UnicodeRange="061f-061f,0621-063a,0640-0655"
	FString			UnicodeRange;

	// Needed to generate 227's Blue/Red/Green UWindowFonts
	BYTE			R, G, B;

	UTrueTypeFontFactory();
	void StaticConstructor();
	UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, DWORD Flags, UObject* Context, FFeedbackContext* Warn);
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
	UBOOL DidTop;
	INT RecursionDepth;
	TArray<UClass*> ClassesList;
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

class EDITOR_API USkeletalMeshExporterPSK : public UExporter
{
	DECLARE_CLASS(USkeletalMeshExporterPSK, UExporter, 0, Editor)

	// Constructor.
	USkeletalMeshExporterPSK() {}

	// UObject interface.
	void StaticConstructor();

	// UExporter interface.
	UBOOL ExportBinary(UObject* Object, const TCHAR* Type, FArchive& Ar, FFeedbackContext* Warn);
	DWORD VMaterialPolyFlags(DWORD PolyFlags);
};

class EDITOR_API USkeletalAnimExporterPSA : public UExporter
{
	DECLARE_CLASS(USkeletalAnimExporterPSA, UExporter, 0, Editor)

	UBOOL IncludeAnimSeq;

	// Constructor.
	USkeletalAnimExporterPSA()
	{
		IncludeAnimSeq = 0;
	}

	// UObject interface.
	void StaticConstructor();

	// UExporter interface.
	UBOOL ExportBinary(UObject* Object, const TCHAR* Type, FArchive& Ar, FFeedbackContext* Warn);

	FQuat FetchOrientation(AnalogTrack& Track, INT Index);
	FVector FetchPosition(AnalogTrack& Track, INT Index);	
	FLOAT FetchTime(AnalogTrack& Track, INT Index);
};

/*-----------------------------------------------------------------------------
	Commandlets
-----------------------------------------------------------------------------*/

class EDITOR_API UBatchExportCommandlet : public UCommandlet
{
	DECLARE_CLASS(UBatchExportCommandlet, UCommandlet, CLASS_Transient, Editor);
	void StaticConstructor();
	INT Main(const TCHAR* Parms);
};

class EDITOR_API UConformCommandlet : public UCommandlet
{
	DECLARE_CLASS(UConformCommandlet, UCommandlet, CLASS_Transient, Editor);
	void StaticConstructor();
	INT Main(const TCHAR* Parms);
};

class EDITOR_API UCheckUnicodeCommandlet : public UCommandlet
{
	DECLARE_CLASS(UCheckUnicodeCommandlet, UCommandlet, CLASS_Transient, Editor);
	void StaticConstructor();
	INT Main(const TCHAR* Parms);
};

class EDITOR_API UPackageFlagCommandlet : public UCommandlet
{
	DECLARE_CLASS(UPackageFlagCommandlet, UCommandlet, CLASS_Transient, Editor);
	void StaticConstructor();
	INT Main(const TCHAR* Parms);
};

class EDITOR_API UDataRipCommandlet : public UCommandlet
{
	DECLARE_CLASS(UDataRipCommandlet, UCommandlet, CLASS_Transient, Editor);
	void StaticConstructor();
	INT Main(const TCHAR* Parms);
};

class EDITOR_API UDumpFontInfoCommandlet : public UCommandlet
{
	DECLARE_CLASS(UDumpFontInfoCommandlet, UCommandlet, CLASS_Transient, Editor)
	UDumpFontInfoCommandlet();
	void StaticConstructor();
	INT Main(const TCHAR* Parms);
};

class EDITOR_API UStripSourceCommandlet : public UCommandlet
{
	DECLARE_CLASS(UStripSourceCommandlet, UCommandlet, CLASS_Transient, Editor)
private:
	void StripText(FString& S);
public:
	void StaticConstructor();
	INT Main(const TCHAR* Parms);
};

class EDITOR_API UDumpIntCommandlet : public UCommandlet
{
	DECLARE_CLASS(UDumpIntCommandlet, UCommandlet, CLASS_Transient, Editor)
private:
	static FString GetMutDesc(UClass* C);
	static FString GetWepDesc(UClass* C);
	void GetExportedObjects(TMultiMap<FString, FString>* Sec, TUnorderedSet<UClass*>& ClassMap) const;
	UBOOL ValueIsModified(FString& Value, UClass* CurrentClass, UClass* BaseClass, UProperty* P, INT Index) const;
	static UBOOL ValueIsModifiedObj(FString& Value, UObject* Obj, UProperty* P, INT Index);
	UBOOL GetObjectPrefs(const TCHAR* S, FString& ObjName, FString& ObjClass, FString& ObjMeta) const;
	UBOOL HasObjectPrefsMatch(TMultiMap<FString, FString>* Sec, const TCHAR* ObjName, const TCHAR* ObjClass, const TCHAR* ObjMeta) const;
	UBOOL HasMatchingPreference(const TCHAR* Org, TMultiMap<FString, FString>* Sec) const;
public:
	void StaticConstructor();
	INT Main(const TCHAR* Parms);
};

struct KeyEntryType
{
	FString Key, Value;
	bool bFound;
};

struct ConfigEntryType
{
	FString GroupName;
	bool bGroupFound, bAlwaysKeep;
	TArray<KeyEntryType> Keys;
	void AddKey(const TCHAR* Key, const TCHAR* Val);
	bool CheckKey(const FString& InKey, const FString& InVal);
};

class EDITOR_API UListObjectsCommandlet : public UCommandlet
{
	DECLARE_CLASS(UListObjectsCommandlet, UCommandlet, CLASS_Transient, Editor)
	UListObjectsCommandlet();
	void StaticConstructor();
	INT Main(const TCHAR* Parms);
};

class EDITOR_API UMakeCommandlet : public UCommandlet
{
	DECLARE_CLASS(UMakeCommandlet, UCommandlet, CLASS_Transient, Editor);
	void StaticConstructor();
	INT Main(const TCHAR* Parms);
};

class EDITOR_API UMasterCommandlet : public UCommandlet
{
	DECLARE_CLASS(UMasterCommandlet, UCommandlet, CLASS_Transient, Editor);

	FString GConfigFile, GProduct, GRefPath, GMasterPath, GSrcPath, GArchive;
	FBufferArchive GArchiveData;
	FArchiveHeader GArc;

	void LocalCopyFile(const TCHAR* Dest, const TCHAR* Src, DWORD Flags);
	struct FLink
	{
		INT Offset;
		FLink* Next;
		FLink(INT InOffset, FLink* InNext)
			: Offset(InOffset), Next(InNext)
		{}
	};
	enum { ARRAY_SIZE = 65536 * 64 };
	enum { MIN_RUNLENGTH = 10 };
	INT ArrayCrc(const TArray<BYTE>& T, INT Offset);
	void Decompress(TArray<BYTE>& New, TArray<BYTE>& Delta, TArray<BYTE> Old);
	UBOOL DeltaCode(const TCHAR* RefFilename, const TCHAR* MasterFilename, const TCHAR* SrcFilename);
	void UpdateGroup(FString MasterPath, const TCHAR* File, const TCHAR* Group, TMultiMap<FString, FString>& Map);
	void CopyGroup(FString MasterPath, const TCHAR* File, const TCHAR* Group, TMultiMap<FString, FString>& Map);
	void ProcessGroup(FString MasterPath, const TCHAR* File, const TCHAR* Group, void(UMasterCommandlet::* Process)(FString MasterPath, const TCHAR* File, const TCHAR* Group, TMultiMap<FString, FString>& Map));
	void StaticConstructor();
	INT Main(const TCHAR* Parms);
};

class EDITOR_API UUpdateUModCommandlet : public UCommandlet
{
	DECLARE_CLASS(UUpdateUModCommandlet, UCommandlet, CLASS_Transient, Editor);
	INT Main(const TCHAR* Parms);
};

class EDITOR_API UChecksumPackageCommandlet : public UCommandlet
{
	DECLARE_CLASS(UChecksumPackageCommandlet, UCommandlet, CLASS_Transient, Editor);
	INT Main(const TCHAR* Parms);
};

class EDITOR_API UMD5Commandlet : public UCommandlet
{
	DECLARE_CLASS(UMD5Commandlet, UCommandlet, CLASS_Transient, Editor);
	UBOOL Silent;
	void StaticConstructor();
	void RemoveByFilename(FString FileName);
	void RemoveByGuid(FGuid Guid, INT Generation);
	BOOL AddMd5Record(FMD5Record& NewRecord);
	INT Main(const TCHAR* Parms);
};

class EDITOR_API UMergeDXTCommandlet : public UCommandlet
{
	DECLARE_CLASS(UMergeDXTCommandlet, UCommandlet, CLASS_Transient, Editor);
	void StaticConstructor();
	void Merge(FString Src, FString Old, FString Dest);
	INT Main(const TCHAR* Parms);
};

class EDITOR_API UPackageDumpCommandlet : public UCommandlet
{
	DECLARE_CLASS(UPackageDumpCommandlet, UCommandlet, CLASS_Transient, Editor);
	void StaticConstructor();
	INT Main(const TCHAR* Parms);
};

class EDITOR_API UExecCommandlet : public UCommandlet
{
	DECLARE_CLASS(UExecCommandlet, UCommandlet, CLASS_Transient, Editor);
	void StaticConstructor();
	INT Main(const TCHAR* Parms);
};

/*-----------------------------------------------------------------------------
	Visibility Helpers
-----------------------------------------------------------------------------*/

class EDITOR_API UBitArray : public UObject
{
	DECLARE_CLASS(UBitArray, UObject, 0, Editor)
	NO_DEFAULT_CONSTRUCTOR(UBitArray)

	// Variables.
	TArray<DWORD> Data;
	DWORD NumBits;

	// Constructor.
	UBitArray(DWORD InNumBits);

	// UObject interface.
	void Serialize(FArchive& Ar);

	// UBitArray interface.
	UBOOL Get(DWORD i);
	void Set(DWORD i, UBOOL Value);
};

//
// An nxn symmetric bit array.
//
class EDITOR_API UBitMatrix : public UBitArray
{
	DECLARE_CLASS(UBitMatrix, UBitArray, 0, Editor)
	NO_DEFAULT_CONSTRUCTOR(UBitMatrix)

	// Variables.
	DWORD Side;

	// Constructor.
	UBitMatrix(DWORD InSide);

	// UObject interface.
	void Serialize(FArchive& Ar);

	// UBitMatrix interface.
	UBOOL Get(DWORD i, DWORD j);
	void Set(DWORD i, DWORD j, UBOOL Value);
};

/*-----------------------------------------------------------------------------
	Parsing Helpers
-----------------------------------------------------------------------------*/

struct TStringIterator
{
	const TCHAR* OrgStr;
	TCHAR Separator;
	UBOOL IsValid;

	FString Value;

	TStringIterator(const TCHAR* InStr, TCHAR Sep)
		: OrgStr(InStr), Separator(Sep), IsValid(1)
	{
		Next();
	}
	inline void Next()
	{
		guardSlow(TStringIterator::Next);
		if (!*OrgStr)
			IsValid = 0;
		else
		{
			while (*OrgStr == ' ')
				++OrgStr;
			const TCHAR* StartStr = OrgStr;
			while (*OrgStr && *OrgStr != Separator)
				++OrgStr;

			Value = FString(StartStr, OrgStr - StartStr);
			if (*OrgStr)
				++OrgStr;
		}
		unguardSlow;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
