/*=============================================================================
	Editor.h: Unreal editor public header file.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef _INC_EDITOR
#define _INC_EDITOR

/*-----------------------------------------------------------------------------
	Dependencies.
-----------------------------------------------------------------------------*/

#include "Engine.h"

#ifndef EDITOR_API
#define EDITOR_API DLL_IMPORT
#endif

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack(push,OBJECT_ALIGNMENT)
#endif

struct EDITOR_API FBuilderPoly
{
	TArray<INT> VertexIndices;
	INT Direction;
	FName ItemName;
	INT PolyFlags;
	FBuilderPoly()
	: VertexIndices(), Direction(0), ItemName(NAME_None), PolyFlags(0)
	{}
};

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (pop)
#endif

#include "EditorClasses.h"

/*-----------------------------------------------------------------------------
	Editor public.
-----------------------------------------------------------------------------*/

#define dED_MAX_VIEWPORTS	8

EDITOR_API extern FVector GBoxSelStart, GBoxSelEnd;
EDITOR_API extern UBOOL GbIsBoxSel;

//
// The editor object.
//
EDITOR_API extern class UEditorEngine* GEditor;

//
// Importing object properties.
//
EDITOR_API const TCHAR* ImportProperties
(
	UClass*				ObjectClass,
	BYTE*				Object,
	ULevel*				Level,
	const TCHAR*		Data,
	UObject*			InParent,
	FFeedbackContext*	Warn,
	UBOOL				FinalPass = TRUE
);

//
// Editor mode settings.
//
// These are also referenced by help files and by the editor client, so
// they shouldn't be changed.
//
enum EEditorMode
{
	EM_None 			= 0,	// Gameplay, editor disabled.
	EM_ViewportMove		= 1,	// Move viewport normally.
	EM_ViewportZoom		= 2,	// Move viewport with acceleration.
	EM_BrushRotate		= 5,	// Rotate brush.
	EM_BrushSheer		= 6,	// Sheer brush.
	EM_BrushScale		= 7,	// Scale brush.
	EM_BrushStretch		= 8,	// Stretch brush.
	EM_TexturePan		= 11,	// Pan textures.
	EM_TextureRotate	= 13,	// Rotate textures.
	EM_TextureScale		= 14,	// Scale textures.
	EM_BrushSnap		= 18,	// Brush snap-scale.
	EM_TexView			= 19,	// Viewing textures.
	EM_TexBrowser		= 20,	// Browsing textures.
	EM_MeshView			= 21,	// Viewing mesh.
	EM_MeshBrowser		= 22,	// Browsing mesh.
	EM_BrushClip		= 23,	// brush Clipping.
	EM_VertexEdit		= 24,	// Multiple Vertex Editing.
	EM_FaceDrag			= 25,	// Face Dragging.
};

//
// Editor callback codes.
//
enum EUnrealEdCallbacks
{
	EDC_None						= 0,
	EDC_Browse						= 1,
	EDC_UseCurrent					= 2,
	EDC_CurTexChange				= 10,
	EDC_SelPolyChange				= 20,
	EDC_SelChange					= 21,
	EDC_RtClickTexture				= 23,
	EDC_RtClickPoly					= 24,
	EDC_RtClickActor				= 25,
	EDC_RtClickWindow				= 26,
	EDC_RtClickWindowCanAdd			= 27,
	EDC_MapChange					= 42,
	EDC_ViewportUpdateWindowFrame	= 43,
	EDC_SurfProps					= 44,
	EDC_SaveMap						= 45,
	EDC_SaveMapAs					= 46,
	EDC_LoadMap						= 47,
	EDC_PlayMap						= 48,
	EDC_CamModeChange				= 49,
	EDC_RedrawAllViewports			= 50,
	EDC_BrushBuildersUpdated		= 51,
	EDC_TransChange					= 52,
	EDC_RefreshEditor				= 53,
	EDC_BoxSelectionStart			= 54,
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
	TEXALIGN_OneTileU		= 5,	// Align one tile U.
	TEXALIGN_OneTileV		= 6,	// Align one tile V.
	TEXALIGN_WallColumn		= 7,	// Align as wall on column.
	TEXALIGN_WallX			= 8,	// Align to wall around X axis
	TEXALIGN_WallY			= 9,	// Align to wall around Y axis
	TEXALIGN_Walls			= 10,	// Align to walls
	TEXALIGN_Auto			= 11,	// Auto align
	TEXALIGN_Clamp			= 12,	// Set alignment for Clamp_to_Edge Textures
};

//
// Importing & Exporting INT files (gam).
//
EDITOR_API UBOOL IntExport (UObject *Package, const TCHAR *IntName, UBOOL ExportFresh, UBOOL ExportInstances, UBOOL NoDelete=0);
EDITOR_API UBOOL IntExport (const TCHAR *PackageName, const TCHAR *IntName, UBOOL ExportFresh, UBOOL ExportInstances, UBOOL NoDelete=0);
EDITOR_API UBOOL IntMatchesPackage (UObject *Package, const TCHAR *IntName);
EDITOR_API UBOOL IntMatchesPackage (const TCHAR *PackageName, const TCHAR *IntName);
EDITOR_API UBOOL IntMatchesPackage (const TCHAR *PackageName);
EDITOR_API void IntGetNameFromPackageName (const FString &PackageName, FString &IntName);

struct FActorGroupCallback
{
	virtual void NotifyLevelActor(AActor* Other, EActorAction Action) {}
};
EDITOR_API extern FActorGroupCallback* GEdCallback;

/*-----------------------------------------------------------------------------
	FEditorHitObserver.
-----------------------------------------------------------------------------*/

//
// Hit observer for editor events.
//
class EDITOR_API FEditorHitObserver : public FHitObserver
{
	UBOOL bHandled = FALSE;
public:
	// FHitObserver interface.
	void Click( const FHitCause& Cause, const HHitProxy& Hit )
	{
		if     ( Hit.IsA(TEXT("HBspSurf"        )) ) Click( Cause, *(HBspSurf        *)&Hit );
		else if( Hit.IsA(TEXT("HActor"          )) ) Click( Cause, *(HActor          *)&Hit );
		else if( Hit.IsA(TEXT("HMoverSurf"      )) ) Click( Cause, *(HMoverSurf      *)&Hit );
		else if( Hit.IsA(TEXT("HBrushVertex"    )) ) Click( Cause, *(HBrushVertex    *)&Hit );
		else if( Hit.IsA(TEXT("HGlobalPivot"    )) ) Click( Cause, *(HGlobalPivot    *)&Hit );
		else if( Hit.IsA(TEXT("HBrowserTexture" )) ) Click( Cause, *(HBrowserTexture *)&Hit );
		else FHitObserver::Click( Cause, Hit );
	}

	// FEditorHitObserver interface.
	virtual void Click( const FHitCause& Cause, const struct HBspSurf&        Hit );
	virtual void Click( const FHitCause& Cause, const struct HActor&          Hit );
	virtual void Click( const FHitCause& Cause, const struct HMoverSurf&      Hit );
	virtual void Click( const FHitCause& Cause, const struct HBrushVertex&    Hit );
	virtual void Click( const FHitCause& Cause, const struct HGlobalPivot&    Hit );
	virtual void Click( const FHitCause& Cause, const struct HBrowserTexture& Hit );

	virtual UBOOL IsHandled() { return bHandled; }
	virtual void SetHandled(UBOOL InHandled) { bHandled = InHandled; }
};

/*-----------------------------------------------------------------------------
	Hit proxies.
-----------------------------------------------------------------------------*/

// Hit a texture view.
struct HTextureView : public HHitProxy
{
	DECLARE_HIT_PROXY(HTextureView,HHitProxy)
	UTexture* Texture;
	INT ViewX, ViewY;
	HTextureView( UTexture* InTexture, INT InX, INT InY ) : Texture(InTexture), ViewX(InX), ViewY(InY) {}
	void Click( const FHitCause& Cause );
};

// Hit a brush vertex.
struct HBrushVertex : public HHitProxy
{
	DECLARE_HIT_PROXY(HBrushVertex,HHitProxy)
	ABrush* Brush;
	FVector Location;
	HBrushVertex( ABrush* InBrush, FVector InLocation ) : Brush(InBrush), Location(InLocation) {}
};

// Hit a global pivot.
struct HGlobalPivot : public HHitProxy
{
	DECLARE_HIT_PROXY(HGlobalPivot,HHitProxy)
	FVector Location;
	HGlobalPivot( FVector InLocation ) : Location(InLocation) {}
};

// Hit a browser texture.
struct HBrowserTexture : public HHitProxy
{
	DECLARE_HIT_PROXY(HBrowserTexture,HHitProxy)
	UTexture* Texture;
	HBrowserTexture( UTexture* InTexture ) : Texture(InTexture) {}
};

// Hit the backdrop.
struct HBackdrop : public HHitProxy
{
	DECLARE_HIT_PROXY(HBackdrop,HHitProxy)
	FVector Location;
	HBackdrop( FVector InLocation ) : Location(InLocation) {}
	void Click( const FHitCause& Cause );
};

/*-----------------------------------------------------------------------------
	FScan.
-----------------------------------------------------------------------------*/

typedef void (*POLY_CALLBACK)( UModel* Model, INT iSurf );

/*-----------------------------------------------------------------------------
	FConstraints.
-----------------------------------------------------------------------------*/

//
// General purpose movement/rotation constraints.
//
class EDITOR_API FConstraints
{
public:
	// Functions.
	virtual void Snap( FVector& Point, FVector GridBase )=0;
	virtual void Snap( FRotator& Rotation )=0;
	virtual UBOOL Snap( ULevel* Level, FVector& Location, FVector GridBase, FRotator& Rotation )=0;
};

/*-----------------------------------------------------------------------------
	FConstraints.
-----------------------------------------------------------------------------*/

//
// General purpose movement/rotation constraints.
//
#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack(push,OBJECT_ALIGNMENT)
#endif
class EDITOR_API FEditorConstraints : public FConstraints
{
public:
	// Variables.
	BITFIELD	GridEnabled:1 GCC_PACK(INT_ALIGNMENT);		// Grid on/off.
	BITFIELD	SnapVertices:1;		// Snap to nearest vertex within SnapDist, if any.
	BITFIELD	AffectRegion:1;		// Affects other vertices within a specified range when dragging one
	BITFIELD	TextureLock:1;		// Prevents brushes from recomputing tex coords when a vertex is moved
	BITFIELD	SelectionLock:1;	// Locks selected actors so they cannot be deselected
	FLOAT		SnapDistance;		// Distance to check for snapping.
	FVector		GridSize;			// Movement grid.
	UBOOL		RotGridEnabled;		// Rotation grid on/off.
	FRotator	RotGridSize;		// Rotation grid.

	// Functions.
	virtual void Snap( FVector& Point, FVector GridBase );
	virtual void Snap( FRotator& Rotation );
	virtual UBOOL Snap( ULevel* Level, FVector& Location, FVector GridBase, FRotator& Rotation );
};
#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (pop)
#endif

/*-----------------------------------------------------------------------------
	Vertex Editing
-----------------------------------------------------------------------------*/
struct FPolyVertex
{
	FPolyVertex(INT i, INT j) : PolyIndex(i), VertexIndex(j) {};
	INT PolyIndex;
	INT VertexIndex;
	inline int operator==(const FPolyVertex& Other) const
	{
		return PolyIndex == Other.PolyIndex && VertexIndex == Other.VertexIndex;
	}
};

/*-----------------------------------------------------------------------------
	Actor Recompilation
-----------------------------------------------------------------------------*/

#if OLDUNREAL_MYLEVEL_ACTOR_RECOMPILATION_SUPPORT
struct FSavedActor
{
	//
	// Saved UObject properties
	//
	INT				SavedIndex;
	UObject*		SavedOuter;
	DWORD			SavedFlags;
	FName			SavedName;
	UClass*			SavedClass;
	INT				OriginalPropertiesSize;

	//
	// Saved AActor properties
	//
	AActor*			OriginalActor;
	AActor*			NewActor;
	INT				SavedActorIndex;
	FStringNoInit	SavedProperties;
};
#endif

/*-----------------------------------------------------------------------------
	UEditorEngine definition.
-----------------------------------------------------------------------------*/

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack(push,OBJECT_ALIGNMENT)
#endif
class EDITOR_API UEditorEngine : public UEngine, public FNotifyHook
{
	DECLARE_CLASS(UEditorEngine,UEngine,CLASS_Transient|CLASS_Config,Editor)

	// Objects.
	ULevel*					 Level;
	UModel*					 TempModel;
	UTexture*				 CurrentTexture;
	UClass*					 CurrentClass;
	class UTransactor*		 Trans;
	class UTextBuffer*		 Results;
	// stijn: the following properties are mirrored as an array of 8 pointers in
	// uscript.  Since we only really have 4 pointers + 1 int here, we need to
	// insert 12 bytes of additional padding in 32 bit and 28 bytes of
	// additional padding in 64 bit
	class WObjectProperties* ActorProperties;
	class WObjectProperties* LevelProperties;
	class WConfigProperties* Preferences;
	class WProperties*       UseDest;
	INT                      AutosaveCounter;	
	INT						 Pad[3];
	FORCE_64BIT_PADDING_DWORD;
	FORCE_64BIT_PADDING_DWORD;
	FORCE_64BIT_PADDING_DWORD;
	FORCE_64BIT_PADDING_DWORD;

	// Graphics.
	UTexture *MenuUp, *MenuDn;
	UTexture *CollOn, *CollOff;
	UTexture *PlyrOn, *PlyrOff;
	UTexture *LiteOn, *LiteOff;
	UTexture *Bad;
	UTexture *Bkgnd, *BkgndHi;

	// Toggles.
	BITFIELD				FastRebuild   :1;
	BITFIELD				Bootstrapping :1;
	BITFIELD				DeletingActors:1;

	// Variables.
	INT						AutoSaveIndex;
	INT						AutoSaveCount;
	INT						Mode;
	DWORD					ClickFlags;
	FLOAT					MovementSpeed;
	UObject*				ParentContext;
	FVector					ClickLocation;
	FPlane					ClickPlane;

	// Tools.
	TArray<UObject*>		Tools;
	UClass*					BrowseClass;

	// Constraints.
	FEditorConstraints		Constraints;

	// Advanced.
	FLOAT FovAngle;
	BITFIELD GodMode:1;
	BITFIELD AutoSave:1;
	BITFIELD DynamicActorRecompilation:1;
	BITFIELD WarnForActorRecompilation:1;
	BYTE AutosaveTimeMinutes GCC_PACK(INT_ALIGNMENT);
	FStringNoInit GameCommandLine;
	TArray<FString> EditPackages;

	// Color preferences.
	FColor
		C_WorldBox,
		C_GroundPlane,
		C_GroundHighlight,
		C_BrushWire,
		C_Pivot,
		C_Select,
		C_Current,
		C_AddWire,
		C_SubtractWire,
		C_GreyWire,
		C_BrushVertex,
		C_BrushSnap,
		C_Invalid,
		C_ActorWire,
		C_ActorHiWire,
		C_Black,
		C_White,
		C_Mask,
		C_SemiSolidWire,
		C_NonSolidWire,
		C_WireBackground,
		C_WireGridAxis,
		C_ActorArrow,
		C_ScaleBox,
		C_ScaleBoxHi,
		C_ZoneWire,
		C_Mover,
		C_OrthoBackground;

	// OldUnreal scaled fonts
	UFont* ScaledSmallFont;
	UFont* ScaledMedFont;
	UFont* ScaledBigFont;
	UFont* ScaledLargeFont;

	// Vertex Editing - This is mirrored as an array of 7 pointers.
	// Since TArrays contain 1 pointer and 2 ints, we need to insert
	// some extra padding behind them in the 64-bit build
	AActor* VertexEditActor{};
	TArray<FPolyVertex> VertexEditList;
	FORCE_64BIT_PADDING_DWORD;
	FORCE_64BIT_PADDING_DWORD;
	TArray<FVertexHit> VertexHitList;
	FORCE_64BIT_PADDING_DWORD;
	FORCE_64BIT_PADDING_DWORD;

	// Brush builders
	TArray<UBrushBuilder*> BrushBuilders;

	// Warning status
	INT ActorRecompilationWarningStatus;

	// Constructor.
	void StaticConstructor();
	UEditorEngine();

	// UObject interface.
	void Destroy();
	void Serialize( FArchive& Ar );

	// FNotify interface.
	void NotifyDestroy( void* Src );
	void NotifyPreChange( void* Src );
	void NotifyPostChange( void* Src );
	void NotifyExec( void* Src, const TCHAR* Cmd );

	// UEngine interface.
	void Init();
	void InitEditor();
	UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar=*GLog );
	UBOOL HookExec( const TCHAR* Cmd, FOutputDevice& Ar=*GLog );
	int Key( UViewport* Viewport, EInputKey Key );
	void Tick( FLOAT DeltaSeconds );
	void Draw( UViewport* Viewport, UBOOL Blit=1, BYTE* HitData=NULL, INT* HitSize=NULL );
	void MouseDelta( UViewport* Viewport, DWORD Buttons, FLOAT DX, FLOAT DY );
	void MousePosition( UViewport* Viewport, DWORD Buttons, FLOAT X, FLOAT Y );
	void Click( UViewport* Viewport, DWORD Buttons, FLOAT X, FLOAT Y );
	void SetClientTravel( UPlayer* Viewport, const TCHAR* NextURL, UBOOL bItems, ETravelType TravelType ) {}
	void UpdateVertices(UBOOL WithTransform = FALSE, UBOOL FinalUpdate = FALSE);
	void HandleDestruction( AActor* Actor);

	// Vertex editing
	void InvalidateVertexCacheForPoly(ABrush* InBrush, INT PolyIndex);
	void GrabVertex(ULevel* Level);
	void ReleaseVertex(ULevel* Level);
	void MoveVertex(ULevel* Level, FVector Delta, UBOOL Constrained);
	bool RecomputePoly(FPoly* Poly);	
	bool RecomputePoly(AActor* InBrush, INT PolyIndex);

	virtual void edSetClickLocation( FVector& InLocation );
	virtual void edNoteActor(AActor* Other, EActorAction Action);

	// General functions.
	virtual UBOOL SafeExec( const TCHAR* Cmd, FOutputDevice& Ar=*GLog );
	void ExecMacro( const TCHAR* Filename, FOutputDevice& Ar );
	virtual void Cleanse( UBOOL Redraw, const TCHAR* TransReset );
	virtual void FinishAllSnaps( ULevel* Level );
	virtual void RedrawLevel( ULevel* Level );
	void FlushViewports(ULevel* Level);
	virtual void ResetSound();
	virtual AActor* AddActor( ULevel* Level, UClass* Class, const FVector& V, const FVector& Normal );
	virtual void NoteSelectionChange( ULevel* Level );
	virtual void NoteActorMovement( ULevel* Level );
	virtual void SetPivot( FVector NewPivot, UBOOL SnapPivotToGrid, UBOOL MoveActors );
	virtual void ResetPivot();
	virtual void UpdatePropertiesWindows();
	virtual UTransactor* CreateTrans();
	void CleanupMapGarbage(ULevel* Level);

	// Editor mode virtuals from UnEdCam.cpp.
	virtual void edcamSetMode( INT Mode );
	virtual int edcamMode( UViewport* Viewport );

	// Editor CSG virtuals from UnEdCsg.cpp.
	virtual void csgPrepMovingBrush( ABrush* Actor );
	virtual void csgCopyBrush( ABrush* Dest, ABrush* Src, DWORD PolyFlags, DWORD ResFlags, UBOOL IsMovingBrush );
	virtual ABrush*	csgAddOperation( ABrush* Actor, ULevel* Level, DWORD PolyFlags, ECsgOper CSG );
	virtual void csgRebuild( ULevel* Level, UBOOL bVisibleOnly = 0 );
	virtual void csgRebuildExt( ULevel* Level, UBOOL bVisibleOnly = 0, UBOOL bOnlyAddSelection = 0 );
	virtual const TCHAR* csgGetName( ECsgOper CsgOper );
	static void FixBrushLinks( ABrush* Actor);

	// Editor EdPoly/BspSurf assocation virtuals from UnEdCsg.cpp.
	virtual INT polyFindMaster( UModel* Model, INT iSurf, FPoly& Poly );
	virtual void polyUpdateMaster( UModel* Model, INT iSurf, INT UpdateTexCoords, INT UpdateBase );

	// Bsp Poly search virtuals from UnEdCsg.cpp.
	virtual void polySetAndClearPolyFlags( UModel* Model, DWORD SetBits, DWORD ClearBits, INT SelectedOnly, INT UpdateMaster );

	// Selection.
	virtual void SelectNone( ULevel* Level, UBOOL Notify );

	// Bsp Poly selection virtuals from UnEdCsg.cpp.
	virtual void polyResetSelection( UModel* Model );
	virtual void polySelectAll ( UModel* Model );
	virtual void polySelectMatchingGroups( UModel* Model );
	virtual void polySelectMatchingItems( UModel* Model );
	virtual void polySelectCoplanars( UModel* Model, UBOOL OneStep = FALSE );
	virtual void polySelectAdjacents( UModel* Model, UBOOL OneStep = FALSE);
	virtual void polySelectAdjacentWalls( UModel* Model, UBOOL OneStep = FALSE);
	virtual void polySelectAdjacentFloors( UModel* Model, UBOOL OneStep = FALSE);
	virtual void polySelectAdjacentSlants( UModel* Model, UBOOL OneStep = FALSE);
	virtual void polySelectAdjacentFill( UModel* Model, UBOOL OneStep = FALSE);
	virtual void polySelectMatchingBrush( UModel* Model );
	virtual void polySelectMatchingTexture( UModel* Model );
	virtual void polySelectMatchingPolyFlags(UModel* Model);
	virtual void polySelectReverse( UModel* Model );
	virtual void polyMemorizeSet( UModel* Model );
	virtual void polyRememberSet( UModel* Model );
	virtual void polyXorSet( UModel* Model );
	virtual void polyUnionSet( UModel* Model );
	virtual void polyIntersectSet( UModel* Model );
	virtual void polySelectZone( UModel *Model );

	// Poly texturing virtuals from UnEdCsg.cpp.
	virtual void polyTexPan( UModel* Model, INT PanU, INT PanV, INT Absolute );
	virtual void polyTexScale( UModel* Model,FLOAT UU, FLOAT UV, FLOAT VU, FLOAT VV, UBOOL Absolute );
	virtual void polyTexAlign( UModel* Model, enum ETexAlign TexAlignType, DWORD Texels, UBOOL bPreserveScale = FALSE );

	// Poly transform virtuals
	virtual void polyTessellate(UModel* Model);
	virtual void polyJoin(UModel* Model, UBOOL ByActors);
	virtual void polyMerge(UModel* Model);

	// Map brush selection virtuals from UnEdCsg.cpp.
	virtual void mapSelectOperation( ULevel* Level, ECsgOper CSGOper );
	virtual void mapSelectFlags(ULevel* Level, DWORD Flags );
	virtual void mapSelectFirst( ULevel* Level );
	virtual void mapSelectLast( ULevel* Level );
	virtual void mapBrushGet( ULevel* Level );
	virtual void mapBrushPut( ULevel* Level );
	virtual void mapSendToFirst( ULevel* Level );
	virtual void mapSendToLast( ULevel* Level );
	virtual void mapMemorizePosition(ULevel* Level);
	virtual void mapSendToMemorizedPosition(ULevel* Level, UBOOL bBefore);
	virtual void mapSetBrush( ULevel* Level, enum EMapSetBrushFlags PropertiesMask, _WORD BrushColor, FName Group, DWORD SetPolyFlags, DWORD ClearPolyFlags, DWORD CSGOper );

	// Editor actor virtuals from UnEdAct.cpp.
	virtual void edactSelectAll( ULevel* Level );
	virtual void edactSelectMatching(ULevel* Level);
	virtual void edactSelectInside( ULevel* Level );
	virtual void edactSelectInvert( ULevel* Level );
	virtual void edactSelectOfClass( ULevel* Level, UClass* Class );
	virtual void edactSelectSubclassOf( ULevel* Level, UClass* Class );
	virtual void edactSelectDeleted( ULevel* Level );
	virtual void edactDeleteSelected( ULevel* Level );
	virtual void edactDuplicateSelected( ULevel* Level );
	virtual void edactDuplicateSelected( ULevel* Level, FVector Offset );
	virtual void edactCopySelected( ULevel* Level );
	virtual void edactPasteSelected( ULevel* Level );
	virtual void edactPasteSelectedPos( ULevel* Level );
	virtual void edactReplaceSelectedBrush( ULevel* Level );
	virtual void edactReplaceSelectedWithClass( ULevel* Level, UClass* Class, UBOOL bKeep );
	virtual void edactReplaceClassWithClass( ULevel* Level, UClass* Class, UClass* WithClass );
	virtual void edactAlignVertices( ULevel* Level );
	virtual void edactSnapToGrid(ULevel* Level);
	virtual void edactHideSelected( ULevel* Level );
	virtual void edactHideUnselected( ULevel* Level );
	virtual void edactHideInvert(ULevel* Level);
	virtual void edactUnHideAll( ULevel* Level );
	virtual void edactApplyTransform( ULevel* Level );
	virtual void edactApplyTransformToBrush( ABrush* InBrush );
	virtual void edactBoxSelect( UViewport* Viewport, ULevel* Level, FVector Start, FVector End );
	virtual void edactExportProperties( AActor* Actor, FOutputDevice& Out, BOOL AllProperties=FALSE );
	virtual void edactImportProperties( AActor* Actor, const TCHAR* Buffer );

	// helpers from UnEdAct.cpp.
	virtual void PreDeleteActors();
	virtual void PostDeleteActors();

	// Bsp virtuals from UnBsp.cpp.
	virtual void bspRepartition( UModel* Model, INT iNode, UBOOL Simple );
	virtual INT bspAddVector( UModel* Model, FVector* V, UBOOL Exact );
	virtual INT bspAddPoint( UModel* Model, FVector* V, UBOOL Exact );
	virtual INT bspNodeToFPoly( UModel* Model, INT iNode, FPoly* EdPoly );
	virtual void bspBuild( UModel* Model, enum EBspOptimization Opt, INT Balance, INT RebuildSimplePolys, INT iNode );
	virtual void bspRefresh( UModel* Model, UBOOL NoRemapSurfs );
	virtual void bspCleanup( UModel* Model );
	virtual void bspBuildBounds( UModel* Model );
	virtual void bspBuildFPolys( UModel* Model, UBOOL SurfLinks, INT iNode );
	virtual void bspMergeCoplanars( UModel* Model, UBOOL RemapLinks, UBOOL MergeDisparateTextures );
	virtual INT bspBrushCSG( ABrush* Actor, UModel* Model, DWORD PolyFlags, ECsgOper CSGOper, UBOOL RebuildBounds, UBOOL MergePolys = 1 );
	virtual void bspOptGeom( UModel* Model );
	virtual void bspValidateBrush( UModel* Brush, UBOOL ForceValidate, UBOOL DoStatusUpdate );
	virtual void bspUnlinkPolys( UModel* Brush );
	virtual INT	bspAddNode( UModel* Model, INT iParent, enum ENodePlace ENodePlace, DWORD NodeFlags, FPoly* EdPoly );

	// Shadow virtuals (UnShadow.cpp).
	virtual void shadowIlluminateBsp( ULevel* Level, UBOOL Selected, UBOOL bVisibleOnly );

	// Mesh functions (UnMeshEd.cpp).
	virtual void meshImport( const TCHAR* MeshName, UObject* InParent, const TCHAR* AnivFname, const TCHAR* DataFname, UBOOL Unmirror, UBOOL ZeroTex, INT UnMirrorTex, ULODProcessInfo* LODInfo);
	virtual void modelImport( const TCHAR* MeshName, UObject* InParent, const TCHAR* SkinFname, UBOOL Unmirror, UBOOL ZeroTex, INT UnMirrorTex, ULODProcessInfo* LODInfo );
	virtual void meshBuildBounds( UMesh* Mesh );
	virtual void modelBuildBounds( USkeletalMesh* Mesh );
	virtual void meshLODProcess( ULodMesh* Mesh, ULODProcessInfo* LODInfo);
	virtual void modelLODProcess( USkeletalMesh* Mesh, ULODProcessInfo* LODInfo, USkelImport* RawData );
	virtual void meshDropFrames( UMesh* Mesh, INT StartFrame, INT NumFrame );
	virtual void ExportMesh(UMesh* Mesh, const TCHAR* FileName);

	// Skeletal animation, digest and linkup functions (UnMeshEd.cpp).
	virtual void modelAssignWeaponBone( USkeletalMesh* Mesh, FName TempFname );
	virtual INT  animGetBoneIndex( UAnimation* Anim, FName TempFname );
	virtual void modelSetWeaponPosition( USkeletalMesh* Mesh, FCoords WeaponCoords );
	virtual void animationImport( const TCHAR* AnimName, UObject* InParent, const TCHAR* DataFname, UBOOL Unmirror, UBOOL ImportSeqs, FLOAT CompDefault ); 	
	virtual void digestMovementRepertoire( UAnimation* Anim );
	virtual void movementDigest( UAnimation* Anim, INT Index );

	// Visibility.
	virtual void TestVisibility( ULevel* Level, UModel* Model, int A, int B );

	// Scripts.
	virtual int MakeScripts( FFeedbackContext* Warn, UBOOL MakeAll, UBOOL Booting, UClass* SpecificClass = NULL );
	virtual int CheckScripts( FFeedbackContext* Warn, UClass* Class, FOutputDevice& Ar );
	virtual UBOOL SaveAndResetActor( AActor* Actor, INT ActorIndex, FSavedActor& Result );
	virtual UBOOL RestoreActorDefaults( FSavedActor& SavedActor );
	virtual UBOOL RestoreActorProperties( FSavedActor& SavedActor );

	// Topics.
	virtual void Get( const TCHAR* Topic, const TCHAR* Item, FOutputDevice& Ar );
	virtual void Set( const TCHAR* Topic, const TCHAR* Item, const TCHAR* Value );
	virtual void EdCallback( DWORD Code, UBOOL Send );

	// Far-plane Z clipping state control functions.
	virtual void SetZClipping();
	virtual void ResetZClipping();

	// Editor rendering functions.
	virtual void DrawFPoly( struct FSceneNode* Frame, FPoly *Poly, FPlane WireColor, DWORD LineFlags );
	virtual void DrawGridSection( struct FSceneNode* Frame, INT ViewportLocX, INT ViewportSXR, INT ViewportGridY, FVector* A, FVector* B, FLOAT* AX, FLOAT* BX, INT AlphaCase );
	virtual void DrawWireBackground( struct FSceneNode* Frame );
	virtual void DrawLevelBrushes( struct FSceneNode* Frame, UBOOL bStatic, UBOOL bDynamic, UBOOL bActive );
	virtual void DrawLevelBrush( struct FSceneNode* Frame, ABrush* Actor, UBOOL bStatic, UBOOL bDynamic, UBOOL bActive );
	virtual void DrawBoundingBox( struct FSceneNode* Frame, FBox* Bound, AActor* Actor );

	// OldUnreal Compatibility Checking	
	virtual UBOOL IsNetCompatibleMap (ULevel* Level, UBOOL bSilent);

	// Brush builders
	virtual void UpdateBrushBuilders();
};

class FCheckObjRefArc : public FArchive
{
private:
	TArray<UObject*>* OutputAr;
	UObject* Seek;
	UObject* Context;
	bool bResult, bMayCleanUp;

	inline bool CheckResFor(const UObject* Checking)
	{
		if (Checking == Seek || Checking->GetClass() == UTextBuffer::StaticClass()
			|| appStricmp(Checking->GetClass()->GetName(), TEXT("TransBuffer")) == 0)
			return false;
		if (!Checking->GetOuter())
			return true;
		for (UObject* C = Checking->GetOuter(); C; C = C->GetOuter())
			if (C == Seek)
				return false;
		return true;
	}
public:

	FArchive& operator<<(class UObject*& Res)
	{
		if (Seek == Res)
		{
			bResult = true;
			if (OutputAr)
				OutputAr->AddUniqueItem(Context);
			if (bMayCleanUp)
				Res = NULL;
		}
		return *this;
	}
	FCheckObjRefArc(TArray<UObject*>* Out, UObject* FindRef, bool bCleanUp)
		: OutputAr(Out)
		, Seek(FindRef)
		, Context(NULL)
		, bResult(false)
		, bMayCleanUp(bCleanUp)
	{}
	bool HasReferences()
	{
		for (FObjectIterator It; It; ++It)
		{
			UObject* O = *It;
			if (CheckResFor(O))
			{
				Context = O;
				O->Serialize(*this);
			}
		}
		return bResult;
	}
};

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (pop)
#endif

/*-----------------------------------------------------------------------------
	Parameter parsing functions.
-----------------------------------------------------------------------------*/

EDITOR_API UBOOL GetFVECTOR( const TCHAR* Stream, const TCHAR* Match, FVector& Value );
EDITOR_API UBOOL GetFVECTOR( const TCHAR* Stream, FVector& Value );
EDITOR_API UBOOL GetFPLANE( const TCHAR* Stream, FPlane& Value );
EDITOR_API UBOOL GetFROTATOR( const TCHAR* Stream, const TCHAR* Match, FRotator& Rotation, int ScaleFactor );
EDITOR_API UBOOL GetFROTATOR( const TCHAR* Stream, FRotator& Rotation, int ScaleFactor );
EDITOR_API UBOOL GetBEGIN( const TCHAR** Stream, const TCHAR* Match );
EDITOR_API UBOOL GetEND( const TCHAR** Stream, const TCHAR* Match );
EDITOR_API TCHAR* SetFVECTOR( TCHAR* Dest, const FVector* Value );
EDITOR_API UBOOL GetFSCALE( const TCHAR* Stream, FScale& Scale );

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
#endif
