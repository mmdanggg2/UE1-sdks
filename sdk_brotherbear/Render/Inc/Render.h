/*=============================================================================
	RenderPrivate.h: Rendering package private header.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

// WARNING: This file is copied from UT v469 sdk as a base
// The members of these classes may not match up with the actual game unless otherwise stated
// On the URender class especially, members have been removed without much care to match 
// the size of object the game expects - mmdanggg2

#ifndef _RENDER_H_
#define _RENDER_H_

/*----------------------------------------------------------------------------
	API.
----------------------------------------------------------------------------*/

#ifndef RENDER_API
	#define RENDER_API DLL_IMPORT
#endif

/*------------------------------------------------------------------------------------
	Dependencies.
------------------------------------------------------------------------------------*/

#include "Engine.h"
#include "UnRender.h"

/*------------------------------------------------------------------------------------
	Render package private.
------------------------------------------------------------------------------------*/

#include "UnSpan.h"

#define MAKELABEL(A,B,C,D) A##B##C##D

// Operate using FPlane instead of FVector if compiler is using SIMD instructions.
#if USES_SSE_INTRINSICS
# define USE_SIMD_MATH 1
#else
// TODO: ARM
# define USE_SIMD_MATH 0
#endif


struct FBspDrawList
{
	INT 			iNode;
	INT				iSurf;
	INT				iZone;
	INT				Key;
	DWORD			PolyFlags;
	FSpanBuffer		Span;
	AZoneInfo*		Zone;
	FBspDrawList*	Next;
	FBspDrawList*	SurfNext;
	FActorLink*		Volumetrics;
	FSavedPoly*		Polys;
	FActorLink*		SurfLights;
};

//
// Class encapsulating the dynamic lighting subsystem.
//
class RENDER_API FLightManagerBase
{
public:
	virtual void Init()=0;
	virtual void Exit()=0;
	virtual DWORD SetupForActor( FSceneNode* Frame, AActor* Actor, struct FVolActorLink* LeafLights, FActorLink* Volumetrics )=0;
	virtual void SetupForSurf( FSceneNode* Frame, FCoords& FacetCoords, FBspDrawList* Draw, FTextureInfo*& LightMap, FTextureInfo*& FogMap, UBOOL Merged )=0;
	virtual void FinishSurf()=0;
	virtual void FinishActor()=0;
	virtual FPlane Light( FTransSample& Point, DWORD ExtraFlags )=0;
	virtual FPlane Fog( FTransSample& Point, DWORD ExtraFlags )=0;

	static UBOOL UsingAmbientlessLightmaps;
	// TODO: LightMap format here too?
};

/*------------------------------------------------------------------------------------
	Links.
------------------------------------------------------------------------------------*/

//
// Linked list of actors with volumetric flag.
//
struct FVolActorLink
{
	// Variables.
	FVector			Location;
	AActor*			Actor;
	FVolActorLink*	Next;
	UBOOL			Volumetric;

	// Functions.
	FVolActorLink( FCoords& Coords, AActor* InActor, FVolActorLink* InNext, UBOOL InVolumetric )
	:	Location	( InActor->Location.TransformPointBy( Coords ) )
	,	Actor		( InActor )
	,	Next		( InNext )
	,	Volumetric	( InVolumetric )
	{}
	FVolActorLink( FVolActorLink& Other, FVolActorLink* InNext )
	:	Location	( Other.Location )
	,	Actor		( Other.Actor )
	,	Volumetric	( Other.Volumetric )
	,	Next		( InNext )
	{}
};
extern AActor* Consider[120];
extern INT NumConsider;

/*------------------------------------------------------------------------------------
	Dynamic Bsp contents.
------------------------------------------------------------------------------------*/

struct FDynamicItem
{
	// Variables.
	FDynamicItem*	FilterNext;
	FLOAT			Z;
	FLOAT mysteryVar1;

	// Functions.
	FDynamicItem() {}
	FDynamicItem( INT iNode );
	virtual void Filter( UViewport* Viewport, FSceneNode* Frame, INT iNode, INT Outside ) {}
	virtual void PreRender( UViewport* Viewport, FSceneNode* Frame, FSpanBuffer* SpanBuffer, INT iNode, FVolActorLink* Volumetrics ) {}
};

struct FDynamicSprite : public FDynamicItem
{
	// Variables.
	FSpanBuffer*	SpanBuffer;
	FDynamicSprite*	RenderNext;
	FTransform		ProxyVerts[4];
	AActor*			Actor;
	INT				X1, Y1;
	INT				X2, Y2;
	FLOAT 			ScreenX, ScreenY;
	FLOAT			Persp;
	FActorLink*		Volumetrics;
	FVolActorLink*	LeafLights;

	// Functions.
	FDynamicSprite( FSceneNode* Frame, INT iNode, AActor* Actor );
	FDynamicSprite( INT iNode, AActor* InActor ) : FDynamicItem( iNode ), Actor( InActor ), SpanBuffer( NULL ), Volumetrics( NULL ), LeafLights( NULL ) {}
	FDynamicSprite( AActor* InActor ) : Actor( InActor ), SpanBuffer( NULL ), Volumetrics( NULL ), LeafLights( NULL ) {}
	UBOOL Setup( FSceneNode* Frame );
};

struct FDynamicChunk : public FDynamicItem
{
	// Variables.
	FRasterPoly*	Raster;
	FDynamicSprite* Sprite;

	// Functions.
	FDynamicChunk( INT iNode, FDynamicSprite* InSprite, FRasterPoly* InRaster );
	void Filter( UViewport* Viewport, FSceneNode* Frame, INT iNode, INT Outside );
};

struct FDynamicFinalChunk : public FDynamicItem
{
	// Variables.
	FRasterPoly*	Raster;
	FDynamicSprite* Sprite;

	// Functions.
	FDynamicFinalChunk( INT iNode, FDynamicSprite* InSprite, FRasterPoly* InRaster, INT IsBack );
	void PreRender( UViewport* Viewport,  FSceneNode* Frame, FSpanBuffer* SpanBuffer, INT iNode, FVolActorLink* Volumetrics );
};

struct FDynamicLight : public FDynamicItem
{
	// Variables.
	AActor*			Actor;
	UBOOL			IsVol;
	UBOOL			HitLeaf;

	// Functions.
	FDynamicLight( INT iNode, AActor* Actor, UBOOL IsVol, UBOOL InHitLeaf );
	void Filter( UViewport* Viewport, FSceneNode* Frame, INT iNode, INT Outside );
};

struct FDynamicSysChild : public FDynamicSprite
{
	// Functions.
	FDynamicSysChild( FSceneNode* Frame, INT iNode, AActor* Actor );
};

struct FDynamicSysParent : public FDynamicSprite
{
	// Variables.
	TArray<FDynamicSysChild*>	Children;

	// Functions.
	FDynamicSysParent( FSceneNode* Frame, INT iNode, AActor* Actor );
	UBOOL Setup( FSceneNode* Frame );
};

/*------------------------------------------------------------------------------------
	Globals.
------------------------------------------------------------------------------------*/

RENDER_API extern FLightManagerBase* GLightManager;
RENDER_API extern FMemStack GDynMem, GSceneMem;

/*------------------------------------------------------------------------------------
	Debugging stats.
------------------------------------------------------------------------------------*/

//
// General-purpose statistics:
//
#if STATS
struct FRenderStats
{
	// Misc.
	INT ExtraTime;

	// MeshStats.
	INT MeshTime;
	INT MeshGetFrameTime, MeshProcessTime, MeshLightSetupTime, MeshLightTime, MeshSubTime, MeshClipTime, MeshTmapTime;
	INT MeshCount, MeshPolyCount, MeshSubCount, MeshVertLightCount, MeshLightCount, MeshVtricCount;

	// ActorStats.

	// FilterStats.
	INT DynFilterTime, FilterTime, DynCount;

	// RejectStats.

	// SpanStats.

	// ZoneStats.

	// OcclusionStats.
	INT OcclusionTime, ClipTime, RasterTime, SpanTime;
	INT NodesDone, NodesTotal;
	INT NumRasterPolys, NumRasterBoxReject;
	INT NumTransform, NumClip;
	INT BoxTime, BoxChecks, BoxBacks, BoxIn, BoxOutOfPyramid, BoxSpanOccluded;
	INT NumPoints;

	// IllumStats.
	INT IllumTime;

	// PolyVStats.
	INT PolyVTime;

	// Actor drawing stats:
	INT NumSprites;			// Number of sprites filtered.
	INT NumChunks;			// Number of final chunks filtered.
	INT NumFinalChunks;		// Number of final chunks.
	INT NumMovingLights;    // Number of moving lights.
	INT ChunksDrawn;		// Chunks drawn.

	// Texture subdivision stats
	INT DynLightActors;		// Number of actors shining dynamic light.

	// Span buffer:
	INT SpanTotalChurn;		// Total spans added.
	INT SpanRejig;			// Number of span index that had to be reallocated during merging.

	// Clipping:
	INT ClipAccept;			// Polygons accepted by clipper.
	INT ClipOutcodeReject;	// Polygons outcode-rejected by clipped.
	INT ClipNil;			// Polygons clipped into oblivion.

	// Memory:
	INT GMem;				// Bytes used in global memory pool.
	INT GDynMem;			// Bytes used in dynamics memory pool.

	// Zone rendering:
	INT CurZone;			// Current zone the player is in.
	INT NumZones;			// Total zones in world.
	INT VisibleZones;		// Zones actually processed.
	INT MaskRejectZones;	// Zones that were mask rejected.

	// Illumination cache:
	INT PalCycles;			// Time spent in palette regeneration.

	// Lighting:
	INT Lightage,LightMem,MeshPtsGen,MeshesGen,VolLightActors;

	// Textures:
	INT UniqueTextures,UniqueTextureMem,CodePatches;

	// Extra:
	INT Extra1,Extra2,Extra3,Extra4;

	// Decal stats
	INT DecalTime, DecalClipTime, DecalCount;

	// Routine timings:
	INT GetValidRangeCycles;
	INT BoxIsVisibleCycles;
	INT CopyFromRasterUpdateCycles;
	INT CopyFromRasterCycles;
	INT CopyIndexFromCycles;
	INT MergeWithCycles;
	INT CalcRectFromCycles;
	INT CalcLatticeFromCycles;
	INT GenerateCycles;
	INT CalcLatticeCycles;
	INT RasterSetupCycles;
	INT RasterGenerateCycles;
	INT TransformCycles;
	INT ClipCycles;
	INT AsmCycles;
	// EARI Stats.
	INT TotalEARITime;
	INT EARIActorDrawTime;
	INT TotalEARIActors;
	INT TotalEARISubActors;
	FName EARIActorNames[64];
	FName EARIActorTags[64];
	INT EARISubActors[64];
	INT EARITime[64];
	INT EARIDrawTime[64];
};
extern FRenderStats GStat;
#endif

/*------------------------------------------------------------------------------------
	Random numbers.
------------------------------------------------------------------------------------*/

// Random number subsystem.
// Tracks a list of set random numbers.
class FGlobalRandomsBase
{
public:
	friend class FLightManager;

	// Functions.
	virtual void Init()=0; // Initialize subsystem.
	virtual void Exit()=0; // Shut down subsystem.
	virtual void Tick(FLOAT TimeSeconds)=0; // Mark one unit of passing time.

	// Inlines.
	FLOAT RandomBase( int i ) {return RandomBases[i & RAND_MASK]; }
	FLOAT Random(     int i ) {return Randoms    [i & RAND_MASK]; }

protected:
	// Constants.
	enum {RAND_CYCLE = 16       }; // Number of ticks for a complete cycle of Randoms.
	enum {N_RANDS    = 256      }; // Number of random numbers tracked, guaranteed power of two.
	enum {RAND_MASK  = N_RANDS-1}; // Mask so that (i&RAND_MASK) is a valid index into Randoms.

	// Variables.
	static FLOAT RandomBases	[N_RANDS]; // Per-tick discontinuous random numbers.
	static FLOAT Randoms		[N_RANDS]; // Per-tick continuous random numbers.
};
extern FGlobalRandomsBase *GRandoms;

/*------------------------------------------------------------------------------------
	Fast approximate math code.
------------------------------------------------------------------------------------*/

#define APPROX_MAN_BITS 10		/* Number of bits of approximate square root mantissa, <=23 */
#define APPROX_EXP_BITS 9		/* Number of bits in IEEE exponent */

extern FLOAT SqrtManTbl[2<<APPROX_MAN_BITS];
extern FLOAT DivSqrtManTbl[1<<APPROX_MAN_BITS],DivManTbl[1<<APPROX_MAN_BITS];
extern FLOAT DivSqrtExpTbl[1<<APPROX_EXP_BITS],DivExpTbl[1<<APPROX_EXP_BITS];

//
// Macro to look up from a power table.
//
#if USES_SSE_INTRINSICS
	inline FLOAT DivSqrtApprox(FLOAT F) {return appFastInvSqrtSSE(F);}
#else
	inline FLOAT DivSqrtApprox(FLOAT F) {return 1.0/appSqrt(F);}
#endif
inline FLOAT DivApprox    (FLOAT F) {return 1.0/F;}
#if USES_SSE_INTRINSICS
	inline FLOAT SqrtApprox   (FLOAT F)
	{
		FLOAT Result;
		_mm_store_ss(&Result,_mm_sqrt_ss(_mm_load_ss(&F)));
		return Result;
	}
#else
	inline FLOAT SqrtApprox   (FLOAT F) {return appSqrt(F);}
#endif


/*------------------------------------------------------------------------------------
	URender.
------------------------------------------------------------------------------------*/

//
// Software rendering subsystem.
//
class RENDER_API URender : public URenderBase
{
	DECLARE_CLASS(URender,URenderBase,CLASS_Config,Render)

	// Friends.
	friend class  FGlobalSpanTextureMapper;
	friend struct FDynamicItem;
	friend struct FDynamicSprite;
	friend struct FDynamicChunk;
	friend struct FDynamicFinalChunk;
	friend struct FDynamicLight;
	friend class  FLightManager;
	friend void RenderSubsurface
	(
		UViewport*		Viewport,
		FSceneNode*	Frame,
		UTexture*		Texture,
		FSpanBuffer*	Span,
		FTransTexture*	Pts,
		DWORD			PolyFlags,
		INT				SubCount
	);

	// obsolete!!
	enum EDrawRaster
	{
		DRAWRASTER_Flat				= 0,	// Flat shaded
		DRAWRASTER_Normal			= 1,	// Normal texture mapped
		DRAWRASTER_Masked			= 2,	// Masked texture mapped
		DRAWRASTER_Blended			= 3,	// Blended texture mapped
		DRAWRASTER_Fire				= 4,	// Fire table texture mapped
		DRAWRASTER_MAX				= 5,	// First invalid entry
	};

	// Constructor.
	URender();
	void StaticConstructor();

	// UObject interface.
	void Destroy();

	// URenderBase interface.
	void Init( UEngine* InEngine );
	UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar=*GLog );
	void PreRender( FSceneNode* Frame );
	void PostRender( FSceneNode* Frame );
	void DrawWorld( FSceneNode* Frame );
	UBOOL Deproject( FSceneNode* Frame, INT ScreenX, INT ScreenY, FVector& V );
	UBOOL Project( FSceneNode* Frame, const FVector& V, FLOAT& ScreenX, FLOAT& ScreenY, FLOAT* Scale );
	void DrawActor( FSceneNode* Frame, AActor* Actor );
	void GetVisibleSurfs( UViewport* Viewport, TArray<INT>& iSurfs );
	void OccludeBsp( FSceneNode* Frame );
	void SetupDynamics( FSceneNode* Frame, AActor* Exclude );
	UBOOL BoundVisible( FSceneNode* Frame, const FCoords& Bound, FSpanBuffer* SpanBuffer, FScreenBounds& Results );
	void GlobalLighting( UBOOL Realtime, AActor* Owner, FLOAT& Brightness, FPlane& Color );
	FSceneNode* CreateMasterFrame( UViewport* Viewport, FVector Location, FRotator Rotation, FScreenBounds* Bounds );
	FSceneNode* CreateChildFrame( FSceneNode* Frame, FSpanBuffer* Span, ULevel* Level, INT iSurf, INT iZone, FLOAT Mirror, const FPlane& NearClip, const FCoords& Coords, FScreenBounds* Bounds );
	void FinishMasterFrame();
	void DrawCircle( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector& Location, FLOAT Radius, UBOOL bScaleRadiusByZoom = 0 );
	void DrawBox(FSceneNode* Frame, FPlane Color, DWORD LineFlags, const FBox& Min, const FCoords& Max);
	void DrawBox( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector Min, FVector Max );
	void DrawCylinder( FSceneNode* Frame, FPlane Color, DWORD LineFlags, const FCoords& Location, FLOAT Radius, FLOAT Height, FLOAT );
	void DrawSphere(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector Location, FLOAT r, INT n);
	void DrawInterpolationPathSegment(FSceneNode* param_1, AInterpolationPoint* param_2, AInterpolationPoint* param_3, const FPlane& param_4, INT param_5);
	void Precache( UViewport* Viewport );

	// Dynamics cache.
	void SetMaxNodeCount(INT MaxNodes);
	FVolActorLink* FirstVolumetric;

	// Scene frames.
	enum {MAX_FRAME_RECURSION=4};

	// Dynamic lighting.
	enum {MAX_DYN_LIGHT_SURFS=2048};
	enum {MAX_DYN_LIGHT_LEAVES=1024};

	// stijn: UT v468 and below had the same problem here as with the old PostDynamics cache.
	// Both DynLightSurfs and DynLightLeaves were originally statically allocated arrays.
	// They contained indices into the SurfLights/LeafLights arrays respectively.
	// SurfLights and LeafLights could grow dynamically and always had at least 
	// Level->Model->Surfs.Num() and Level->Model->Leaves.Num() items respectively.
	//
	// In levels such as AS-FoT-Chronoshift, this could lead to situations were 
	// SurfLights/LeafLights were bigger than DynLightSurfs/DynLightLeaves.
	// The code that wrote into DynLightSurfs/DynLightLeaves unfortunately assumed that
	// the array's sizes matched SurfLights/LeafLights and it did not do any bounds checking.
	//
	// In v469, I turned DynLightSurfs/DynLightLeaves into TArrays whose underlying allocations
	// always match the number of items in SurfLights/LeafLights.
	static TArray<INT> 		DynLightSurfs;
	static TArray<INT>		DynLightLeaves;
	static INT				MaxSurfLights;
	static INT				MaxLeafLights;
	static FActorLink**		SurfLights;
	static FVolActorLink**	LeafLights;

	// Variables.
	UBOOL					Toggle;
	UBOOL					LeakCheck;
	//UBOOL                   WireShow;
	//UBOOL                   BlendShow;
	//UBOOL                   BoneShow;
	FLOAT					GlobalMeshLOD;
	FLOAT					GlobalShapeLOD;
	FLOAT					GlobalShapeLODAdjust;
	INT                     ShapeLODMode;
	FLOAT                   ShapeLODFix;
#if !STATS && defined(LEGEND_RENDERING)
	INT						MeshPolyCount;
#endif

	// Timing.
	DOUBLE					LastEndTime;
	DOUBLE					StartTime;
	DOUBLE					EndTime;
	DWORD					NodesDraw;
	DWORD					PolysDraw;

	// Scene.
	FMemMark				SceneMark, MemMark, DynMark;
	INT						SceneCount;

	// Which stats to display.
	UBOOL NetStats;
	UBOOL FpsStats;
	UBOOL GlobalStats;
	UBOOL MeshStats;
	UBOOL ActorStats;
	UBOOL FilterStats;
	UBOOL RejectStats;
	UBOOL SpanStats;
	UBOOL ZoneStats;
	UBOOL LightStats;
	UBOOL OcclusionStats;
	UBOOL GameStats;
	UBOOL SoftStats;
	UBOOL CacheStats;
	UBOOL PolyVStats;
	UBOOL PolyCStats;
	UBOOL IllumStats;
	UBOOL HardwareStats;
	UBOOL EARIStats;
	UBOOL EARIDetails;
	UBOOL Extra8Stats;
	//UBOOL AnimStats;

	// Stat line counter.
	INT   StatLine;

	// Stats display offsets.
	INT   StatOffsetX;
	INT   StatOffsetY;

	// Stats drawing options.
	UBOOL StatDrawOutline;  // if true, draws a black outline around stats to improve font visibility
	UBOOL StatScaleFont;   // if true, the stat font will scale up on larger displays
	FLOAT StatScaleFactor; // -1 = scale dynamically, >1.0 = multiply font height by statscalefactor

	// Unlit behavior.
	UBOOL OldUnlitColors;		

	// OccludeBsp dynamics.
	
	// stijn: the dynamics cache stores dynamic lights, sprites, etc. 
	static struct FDynamicsCache
	{
		FDynamicItem* Dynamics[2];
	}* DynamicsCache;

	// stijn: Render uses this "PostDynamics" cache to remember the iNode numbers we used while rendering
	// the current frame. At the end of the frame, Render uses the PostDynamics list to NULL out the head of
	// every dynamics chain
	static TArray<INT> PostDynamics;

	static struct FStampedPoint
	{
		FTransform* Point;
		DWORD		Stamp;
	}* PointCache;
	static FMemStack VectorMem;
	static DWORD Stamp;
	

	FDynamicItem* Dynamic( INT iNode, INT i )
	{
		return DynamicsCache[iNode].Dynamics[i];
	}

	// stijn: adds a dynamicitem to the front of the list
	bool AddDynamic(INT iNode, INT i, FDynamicItem* Item)
	{
		Item->FilterNext = DynamicsCache[iNode].Dynamics[0];
		DynamicsCache[iNode].Dynamics[0] = Item;
		return true;
	}

	bool AddDynamicZSorted(INT iNode, INT IsBack, FLOAT Z, FDynamicItem* Item)
	{
		for (FDynamicItem** ExistingItem = &DynamicsCache[iNode].Dynamics[IsBack ? 1 : 0];; ExistingItem = &(*ExistingItem)->FilterNext)
		{
			if (!*ExistingItem || (*ExistingItem)->Z >= Z)
			{
				Item->FilterNext = *ExistingItem;
				*ExistingItem = Item;
				break;
			}
		}

		return true;
	}

	// Implementation.
	void OccludeFrame( FSceneNode* Frame );
	void DrawFrame( FSceneNode* Frame );
	void LeafVolumetricLighting( FSceneNode* Frame, UModel* Model, INT iLeaf );
	INT ClipBspSurf( INT iNode, FTransform**& OutPts );

	INT ClipTexPoints( FSceneNode* Frame, FTransTexture* InPts, FTransTexture* OutPts, INT Num0 );
	void DrawActorSprite( FSceneNode* Frame, FDynamicSprite* Sprite );
	void DrawMesh( FSceneNode* Frame, AActor* Owner, AActor* LightSink, FSpanBuffer* SpanBuffer, AZoneInfo* Zone, const FCoords& Coords, FVolActorLink* LeafLights, FActorLink* Volumetrics, DWORD PolyFlags );
	void DrawLodMesh( FSceneNode* Frame, AActor* Owner, AActor* LightSink, FSpanBuffer* SpanBuffer, AZoneInfo* Zone, const FCoords& Coords, FVolActorLink* LeafLights, FActorLink* Volumetrics, DWORD PolyFlags );
	void ShowStat ( FSceneNode* Frame, const TCHAR* Fmt, ... );
	void ShowStatCentered ( FSceneNode* Frame, const TCHAR* Fmt, ...);
	void DrawStats( FSceneNode* Frame );
	INT ClipDecal( FSceneNode* Frame, FDecal *Decal, UModel* Model, FBspSurf* Surf, FSavedPoly* Poly, FTransTexture**& DecalPoints, INT& NumPts, UBOOL DoFog );

	void DrawParticleSystem( FSceneNode*, FDynamicSprite*);

private:
	void ShowStatInternal(FSceneNode* Frame, UBOOL Center, const TCHAR* Str);
	FLOAT GetStatFontScale(UCanvas* Canvas);

	int padding[4];

};
extern RENDER_API URender* GRender;

// Function pointer types.
typedef void (*LIGHT_SPATIAL_FUNC)(FTextureInfo& Tex, class FLightInfo* Info, BYTE* Src, BYTE* Dest);

// Information about one special lighting effect.
struct FLocalEffectEntry
{
	LIGHT_SPATIAL_FUNC	SpatialFxFunc;		// Function to perform spatial lighting
	INT					IsSpatialDynamic;	// Indicates whether light spatiality changes over time.
	INT					IsMergeDynamic;		// Indicates whether merge function changes over time.
};

// Light classification.
enum ELightKind
{
	ALO_StaticLight = 0,	// Actor is a non-moving, non-changing lightsource
	ALO_DynamicLight = 1,	// Actor is a non-moving, changing lightsource
	ALO_MovingLight = 2,	// Actor is a moving, changing lightsource
	ALO_NotLight = 3,	// Not a surface light (probably volumetric only).
};

// Information about a lightsource.
class FLightInfo
{
public:
	// For all lights.
	AActor*		Actor;					// All actor drawing info.
	ELightKind	Opt;					// Light type.
	FVector		Location;				// Transformed screenspace location of light.
	FLOAT		Radius;					// Maximum effective radius.
	FLOAT		RRadius;				// 1.0f / Radius.
	FLOAT		RRadiusMult;			// 16383.0f / (Radius * Radius).
	FLOAT		Brightness;				// Center brightness at this instance, 1.0f=max, 0.0f=none.
	FLOAT		Diffuse;				// BaseNormalDelta * RRadius.
	BYTE*		IlluminationMap;		// Temporary illumination map pointer.
	BYTE*		ShadowBits;				// Temporary shadow map.
	UBOOL		IsVolumetric;			// Whether it's volumetric.

	// Clipping region.
	INT			MinU, MaxU, MinV, MaxV;

	// For volumetric lights.
	FLOAT		VolRadius;				// Volumetric radius.
	FLOAT		VolRadiusSquared;		// VolRadius*VolRadius.
	FLOAT		VolBrightness;			// Volumetric lighting brightness.
	FLOAT		LocationSizeSquared;	// Location.SizeSqurated().
	FLOAT		RVolRadius;				// 1/Volumetric radius.
	FLOAT		RVolRadiusSquared;		// 1/VolRadius*VolRadius.
	UBOOL       VolInside;               // Viewpoint is inside the sphere.

	// Information about the lighting effect.
	FLocalEffectEntry Effect;

	// Coloring.
	FPlane		FloatColor;				// Incident lighting color.
	FPlane		VolumetricColor;		// Volumetric lighting color.
	FColor*		Palette;				// Brightness scaler.
	FColor*		VolPalette;             // Volumetric color scaler.

	// Functions.
	void ComputeFromActor(FTextureInfo* Map, FSceneNode* Frame);
};

#endif

/*------------------------------------------------------------------------------------
	The End.
------------------------------------------------------------------------------------*/
