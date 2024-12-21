/*=============================================================================
	RenderPrivate.h: Rendering package private header.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#pragma once

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
#if ASM3DNOW
# include "Amd3d.h"
#endif

/*------------------------------------------------------------------------------------
	Render package private.
------------------------------------------------------------------------------------*/

#include "UnSpan.h"
#if LIGHTMAPATLAS
#include "UnAtlas.h"
#endif // LIGHTMAPATLAS

#define MAKELABEL(A,B,C,D) A##B##C##D

struct FBspDrawList
{
	INT 			iNode;
	INT				iSurf;
	INT				iZone;
	INT				Key;
	DWORD			PolyFlags, ExPolyFlags;
	FSpanBuffer		Span;
	AZoneInfo*		Zone;
	FBspDrawList*	Next;
	FBspDrawList*	SurfNext;
	FActorLink*		Volumetrics;
	FSavedPoly*		Polys;
	FActorLink*		SurfLights;
	struct FBspLightMap* LightData;

	FBspDrawList()
		: iNode(0)
		, iSurf(0)
		, iZone(0)
		, Key(0)
		, PolyFlags(PF_None)
		, ExPolyFlags(EPF_None)
		, Zone(nullptr)
		, Next(nullptr)
		, SurfNext(nullptr)
		, Volumetrics(nullptr)
		, Polys(nullptr)
		, SurfLights(nullptr)
		, LightData(nullptr)
	{}
};

//
// BSP draw list pointer for sorting.
//
struct FBspDrawListPtr
{
	FBspDrawList *Ptr;

	friend INT Compare( const FBspDrawListPtr& A, const FBspDrawListPtr& B )
	{
		return A.Ptr->Key - B.Ptr->Key;
	}
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
	,	Next		( InNext )
	,	Volumetric	( Other.Volumetric )
	{}
};

/*------------------------------------------------------------------------------------
	Dynamic Bsp contents.
------------------------------------------------------------------------------------*/

struct FDynamicItem
{
	// Variables.
	FDynamicItem*	FilterNext;
	FLOAT			Z;

	// Functions.
	FDynamicItem()
		: FilterNext(NULL), Z(0.f)
	{}
	FDynamicItem( INT iNode );
	virtual void Filter( UViewport* Viewport, FSceneNode* Frame, INT iNode, INT Outside ) {}
	virtual void PreRender( UViewport* Viewport, FSceneNode* Frame, FSpanBuffer* SpanBuffer, INT iNode, FVolActorLink* Volumetrics ) {}
};

struct FDynamicSprite : public FDynamicItem
{
	// Variables.
	struct FActorTreeItem* TreeItem;
	FSpanBuffer*		SpanBuffer;
	FDynamicSprite*		RenderNext;
	FTransform			ProxyVerts[4];
	AActor*				Actor;
	INT					X1, Y1;
	INT					X2, Y2;
	FLOAT 				ScreenX, ScreenY;
	FLOAT				Persp;
	FActorLink*			Volumetrics;
	EActorRenderPass	RenderPass;
	INT					iDrawLeaf;

	// Functions.
	FDynamicSprite(AActor* InActor, FActorTreeItem* Item = nullptr)
		: FDynamicItem(0)
		, TreeItem(Item)
		, SpanBuffer(NULL)
		, RenderNext(NULL)
		, Actor(InActor)
		, Volumetrics(NULL)
		, iDrawLeaf(InActor->Region.iLeaf)
	{}
	FDynamicSprite(AActor* InActor, ENoInit)
		: FDynamicItem()
		, TreeItem(NULL)
		, SpanBuffer(NULL)
		, Actor(InActor)
		, Volumetrics(NULL)
		, iDrawLeaf(InActor->Region.iLeaf)
	{}
	FDynamicSprite()
		: FDynamicItem()
		, TreeItem(NULL)
		, SpanBuffer(NULL)
		, Actor(NULL)
		, Volumetrics(NULL)
		, iDrawLeaf(INDEX_NONE)
	{}
	void FilterBSP(FSceneNode* Frame);
	virtual UBOOL Setup( FSceneNode* Frame );
	virtual void OnDraw(FSceneNode* Frame);

	inline void InitRenderPass(FSceneNode* Frame)
	{
		RenderPass = Actor->GetRenderPass();

		// Check if should fade in/out of existance.
		if (RenderPass != RENDPASS_Third && Actor->ShouldDistanceFade(Frame->Coords.Origin, NULL))
			RenderPass = RENDPASS_Third;
	}

protected:
	void InnerFilterBSP(FSceneNode* Frame);
};

struct FTerrainZoneFilter
{
	UBOOL bIsVisible;

	FTerrainZoneFilter()
		: bIsVisible(FALSE)
	{}
};
struct FZonedSprite : public FDynamicSprite
{
	FTerrainZoneFilter* VisFilter;
	const INT iZoneNumber;

	FZonedSprite(AActor* InActor, INT inZone, FActorTreeItem* Item = nullptr)
		: FDynamicSprite(InActor, Item)
		, VisFilter(nullptr)
		, iZoneNumber(inZone)
	{}
	void OnDraw(FSceneNode* Frame)
	{
		guardSlow(FZonedSprite::OnDraw);
		if (!VisFilter || VisFilter->bIsVisible)
			FDynamicSprite::OnDraw(Frame);
		unguardSlow;
	}
	void FilterBSP(FSceneNode* Frame);
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
	struct FLightInfo* Actor;
	BYTE			IsVol;
	BYTE			HitLeaf;

	// Functions.
	FDynamicLight(FLightInfo* A, FDynamicLight* N)
		: Actor(A)
	{
		FilterNext = N;
	}
	FDynamicLight(INT iNode, FLightInfo* Actor, BYTE Vol, BYTE InHitLeaf); //FDynamicLight( INT iNode, AActor* Actor, BYTE IsVol, UBOOL InHitLeaf );
	void Filter( UViewport* Viewport, FSceneNode* Frame, INT iNode, INT Outside );
};

/*------------------------------------------------------------------------------------
	Globals.
------------------------------------------------------------------------------------*/

extern FLightManagerBase* GLightManager;
extern FMemStack GDynMem, GSceneMem;
extern FVector GBaseColour;
extern INT GVisibleLightFlags;
extern FLOAT GLightSpecular;
extern FLOAT GLightSuperSpecular;
extern FLOAT GBaseSpecularity;

/*------------------------------------------------------------------------------------
	Debugging stats.
------------------------------------------------------------------------------------*/

//
// General-purpose statistics:
//
#if STATS
struct FRenderStatsX
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
extern FRenderStatsX GStat;
#endif

/*------------------------------------------------------------------------------------
	Random numbers.
------------------------------------------------------------------------------------*/

// Random number subsystem.
// Tracks a list of set random numbers.
class FGlobalRandomsBase
{
public:
	// Functions.

	virtual void Init() _VF_BASE; // Initialize subsystem.
	virtual void Exit() _VF_BASE; // Shut down subsystem.
	virtual void Tick(FLOAT TimeSeconds) _VF_BASE; // Mark one unit of passing time.

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
#if MACOSXPPC
inline FLOAT DivSqrtApprox(FLOAT F) { return __frsqrte(F); }
inline FLOAT DivApprox    (FLOAT F) { return __fres(F); }
inline FLOAT SqrtApprox   (FLOAT F) { return __fres(__frsqrte(F)); }

#elif ASM
#define POWER_ASM(ManTbl,ExpTbl)\
	__asm\
	{\
		/* Here we use the identity sqrt(a*b) = sqrt(a)*sqrt(b) to perform\
		** an approximate floating point square root by using a lookup table\
		** for the mantissa (a) and the exponent (b), taking advantage of the\
		** ieee floating point format.\
		*/\
		__asm mov  eax,[F]									/* get float as int                   */\
		__asm shr  eax,(32-APPROX_EXP_BITS)-APPROX_MAN_BITS	/* want APPROX_MAN_BITS mantissa bits */\
		__asm mov  ebx,[F]									/* get float as int                   */\
		__asm shr  ebx,32-APPROX_EXP_BITS					/* want APPROX_EXP_BITS exponent bits */\
		__asm and  eax,(1<<APPROX_MAN_BITS)-1				/* keep lowest 9 mantissa bits        */\
		__asm fld  DWORD PTR ManTbl[eax*4]					/* get mantissa lookup                */\
		__asm fmul DWORD PTR ExpTbl[ebx*4]					/* multiply by exponent lookup        */\
		__asm fstp [F]										/* store result                       */\
	}\
	return F;
//
// Fast floating point power routines.
// Pretty accurate to the first 10 bits.
// About 12 cycles on the Pentium.
//
inline FLOAT DivSqrtApprox(FLOAT F) {POWER_ASM(DivSqrtManTbl,DivSqrtExpTbl);}
inline FLOAT DivApprox    (FLOAT F) {POWER_ASM(DivManTbl,    DivExpTbl    );}
inline FLOAT SqrtApprox   (FLOAT F)
{
	__asm
	{
		mov  eax,[F]                        // get float as int.
		shr  eax,(23 - APPROX_MAN_BITS) - 2 // shift away unused low mantissa.
		mov  ebx,[F]						// get float as int.
		and  eax, ((1 << (APPROX_MAN_BITS+1) )-1) << 2 // 2 to avoid "[eax*4]".
		and  ebx, 0x7F000000				// 7 bit exp., wipe low bit+sign.
		shr  ebx, 1							// exponent/2.
		mov  eax,DWORD PTR SqrtManTbl [eax]	// index hi bit is exp. low bit.
		add  eax,ebx						// recombine with exponent.
		mov  [F],eax						// store.
	}
	return F;								// compiles to fld [F].
}
#else
inline FLOAT DivSqrtApprox(FLOAT F) {return appFastInvSqrtN0(F);}
inline FLOAT DivApprox    (FLOAT F) {return 1.0/F;}
inline FLOAT SqrtApprox   (FLOAT F) {return appSqrt(F);}
#endif

struct FRenderToTexState
{
	FRenderToTexState* PrevState;
	FRenderToTexture* Tex;
	INT OrgX, OrgY;
	BYTE* OrgPtr;

	FRenderToTexState(FRenderToTexState* PState, FRenderToTexture& InTex)
		: PrevState(PState), Tex(&InTex)
	{
		UViewport* V = InTex.Viewport;
		OrgX = V->SizeX;
		OrgY = V->SizeY;
		OrgPtr = V->ScreenPointer;
		V->SizeX = InTex.ScreenX;
		V->SizeY = InTex.ScreenY;
		V->ScreenPointer = InTex.ScreenPtr;
	}
	~FRenderToTexState()
	{
		UViewport* V = Tex->Viewport;
		V->SizeX = OrgX;
		V->SizeY = OrgY;
		V->ScreenPointer = OrgPtr;
	}
};

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
				FSceneNode*		Frame,
				FTextureInfo&	Texture,
				FSpanBuffer*	Span,
				FTransTexture**	Pts,
				DWORD			PolyFlags,
				INT				SubCount,
				AActor*			Owner
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
	void InitLightEngine(UBOOL bExit);
	UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar=*GLog );
	void PreRender( FSceneNode* Frame );
	void PostRender( FSceneNode* Frame );
	void DrawWorld( FSceneNode* Frame );
	UBOOL Deproject( FSceneNode* Frame, INT ScreenX, INT ScreenY, FVector& V );
	UBOOL Project( FSceneNode* Frame, const FVector& V, FLOAT& ScreenX, FLOAT& ScreenY, FLOAT* Scale );
	void DrawActor( FSceneNode* Frame, AActor* Actor, FDynamicSprite* Sprite=NULL );
	void GetVisibleSurfs( UViewport* Viewport, TArray<INT>& iSurfs, FLOAT MaxDistance );
	void OccludeBsp( FSceneNode* Frame );
	void SetupDynamics( FSceneNode* Frame, AActor* Exclude );
	UBOOL BoundVisible( FSceneNode* Frame, FBox* Bound, FSpanBuffer* SpanBuffer, FScreenBounds& Results );
	void GlobalLighting( UBOOL Realtime, AActor* Owner, FLOAT& Brightness, FPlane& Color, UBOOL UseColor=0 );
	FSceneNode* CreateMasterFrame( UViewport* Viewport, FVector Location, FRotator Rotation, FScreenBounds* Bounds );
	FSceneNode* CreateChildFrame( FSceneNode* Frame, FSpanBuffer* Span, ULevel* Level, INT iSurf, INT iZone, FLOAT Mirror, const FPlane& NearClip, const FCoords& Coords, FScreenBounds* Bounds, INT iClipNode=INDEX_NONE );
	void FinishMasterFrame();
	void DrawCircle( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector& Location, FLOAT Radius, UBOOL bScaleRadiusByZoom = 0 );
	void DrawBox( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector Min, FVector Max );
	void DrawCylinder( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector& Location, FLOAT Radius, FLOAT Height );
	void DrawCube( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector Location, FVector Size, FRotator xRotation = FRotator(0,0,0));
	void DrawSphere( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector Location, FLOAT r, INT n);
	void Precache( UViewport* Viewport );
	void NoteDestroyed( AActor* Other );
	BYTE GetSurfacePts( FBspNode* Node, FTransform**& Result );
	UBOOL PushRenderToTexture(FRenderToTexture& Tex);
	void PopRenderToTexture(FRenderToTexture& Tex);

	// Projector drawing
	void DrawDecalMeshFace( AProjector* ProjActor, bool bDeTransform, struct FTransTexture** PtsTT, INT NumPts, FSceneNode* Frame, INT DrawCount );

	static inline DWORD GetRenderFlags(AProjector* ProjActor)
	{
		switch (ProjActor->ProjectStyle)
		{
		case STY_None:
			return PF_None | (ProjActor->ProjectTexture ? ProjActor->ProjectTexture->PolyFlags : 0);
		case STY_Normal:
			return PF_None;
		case STY_Masked:
			return PF_Masked;
		case STY_Translucent:
			return PF_Translucent;
		case STY_Modulated:
			return PF_Modulated;
		case STY_AlphaBlend:
			return PF_AlphaBlend;
		default:
			return PF_None;
		}
	}

	// Dynamics cache.
	FVolActorLink* FirstVolumetric;
	FIteratorActorList* VisiblePawns, * VisibleProjectiles;
	void SetMaxNodeCount(INT MaxNodes);

	// Dynamic lighting.
	enum {MAX_DYN_LIGHT_SURFS=65536};
	enum {MAX_DYN_LIGHT_LEAVES=32768};
	
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
	static INT				LightDelta;

	// Variables.
	UBOOL					Toggle;
	UBOOL					LeakCheck;
	UBOOL                   WireShow;
	UBOOL                   BlendShow;
	UBOOL                   BoneShow;
	UBOOL					bDrawSolids;
	FLOAT					GlobalMeshLOD;
	FLOAT					GlobalShapeLOD;
	FLOAT					GlobalShapeLODAdjust;
	INT                     ShapeLODMode;
	FLOAT                   ShapeLODFix;

	// Timing.
	FTime					LastEndTime;
	FTime					StartTime;
	FTime					EndTime;
	DWORD					NodesDraw;
	DWORD					PolysDraw;

	// Scene.
	FMemMark				SceneMark, MemMark, DynMark, VecMark;
	INT						SceneCount;
	FTextureInfo*			InvisTextureMask;

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
	UBOOL AnimStats;
	UBOOL VideoStats;

	// Stat line counter.
	INT   StatLine;

	// OccludeBsp dynamics.

	// stijn: the dynamics cache stores dynamic lights, sprites, etc. 
	static struct FDynamicsCache
	{
		FDynamicItem* Dynamics[2];
	}*DynamicsCache;

	// stijn: Render uses this "PostDynamics" cache to remember the iNode numbers we used while rendering
	// the current frame. At the end of the frame, Render uses the PostDynamics list to NULL out the head of
	// every dynamics chain
	static TArray<INT> PostDynamics;

	static struct FStampedPoint
	{
		FTransform* Point;
		DWORD		Stamp;
	}*PointCache;
	static FMemStack VectorMem;
	static DWORD Stamp;

	static inline FDynamicItem* Dynamic(INT iNode, INT i)
	{
		return DynamicsCache[iNode].Dynamics[i];
	}
	FDynamicSprite* PostDrawAttachment;

	struct FForcedSprites
	{
		FDynamicSprite* Sprite;
		FForcedSprites* Next;

		FForcedSprites(FDynamicSprite* S, FForcedSprites* N)
			: Sprite(S), Next(N)
		{}
	};
	static FForcedSprites* ForceRenderActors;

	static FTerrainZoneFilter* TerrainDrawList[FBspNode::MAX_ZONES]; // Used by OccludeBSP to mark which terrains should be drawn.
	static INT UsedTerrainLists[FBspNode::MAX_ZONES]; // Used for faster cleanup.
	static INT NumUsedTerrainLists;

	// Marco: Render to texture stack.
	FRenderToTexState* RenToTexStates;

	// stijn: adds a dynamicitem to the front of the list
	static inline void AddDynamic(const INT iNode, FDynamicItem* Item)
	{
		guardSlow(URender::AddDynamic);
		Item->FilterNext = DynamicsCache[iNode].Dynamics[0];
		DynamicsCache[iNode].Dynamics[0] = Item;
		unguardSlow;
	}
	static inline void AddDynamicZSorted(const INT iNode, const INT IsBack, const FLOAT Z, FDynamicItem* Item)
	{
		guardSlow(URender::AddDynamicZSorted);
		for (FDynamicItem** ExistingItem = &DynamicsCache[iNode].Dynamics[IsBack ? 1 : 0];; ExistingItem = &(*ExistingItem)->FilterNext)
		{
			if (!*ExistingItem || (*ExistingItem)->Z >= Z)
			{
				Item->FilterNext = *ExistingItem;
				*ExistingItem = Item;
				break;
			}
		}
		unguardfSlow((TEXT("(Node %i Back %i Z %f)"), iNode, IsBack, Z));
	}
	static inline FTerrainZoneFilter* AddTerrainSprite(const INT iZone)
	{
		guardSlow(URender::AddTerrainSprite);
		FTerrainZoneFilter* Result = TerrainDrawList[iZone];
		if (!Result)
		{
			UsedTerrainLists[NumUsedTerrainLists++] = iZone;
			TerrainDrawList[iZone] = Result = new(GDynMem)FTerrainZoneFilter();
		}
		return Result;
		unguardSlow;
	}

	// stijn: if we don't have dynamics in slot iNode yet, then remember that we need to clean up the slot at the end of the frame
	static inline void MarkNewNode(INT iNode)
	{
		guardSlow(URender::MarkNewNode);
		if (!DynamicsCache[iNode].Dynamics[0] && !DynamicsCache[iNode].Dynamics[1])
			PostDynamics.AddItem(iNode);
		unguardSlow;
	}

	// Implementation.
	void OccludeFrame( FSceneNode* Frame );
	void DrawFrame( FSceneNode* Frame );
	void LeafVolumetricLighting( FSceneNode* Frame, UModel* Model, INT iLeaf );
	INT ClipBspSurf( INT iNode, FTransform**& OutPts );
	void Serialize(FArchive& Ar);
	FTextureInfo* GetInvisiTexMask();
	FTextureInfo* GetFogSphere(FLOAT Density);
	FTextureInfo* GetBackgroundFogmap(FSceneNode* Frame);

#if MACOSXPPC
	INT AltivecClipBspSurf( INT iNode, FTransform**& OutPts );
#endif

	virtual void DrawActorSprite( FSceneNode* Frame, FDynamicSprite* Sprite );
	virtual void DrawMesh(FSceneNode* Frame, FDynamicSprite* Sprite, const FCoords& Coords, DWORD ExtraFlags);
	virtual void DrawLodMesh(FSceneNode* Frame, FDynamicSprite* Sprite, const FCoords& Coords, DWORD ExtraFlags);
	virtual void DrawStaticMesh(FSceneNode* Frame, FDynamicSprite* Sprite, const FCoords& Coords, DWORD ExtraFlags);
	void DrawTerrain(FSceneNode* Frame, FDynamicSprite* Sprite, const FCoords& Coords);
	void DrawSpecialSprite(FSceneNode* Frame, FDynamicSprite* Sprite, DWORD ExtraFlags, UBOOL bRope);
	void ShowStat( FSceneNode* Frame, const TCHAR* Fmt, ... );
	void DrawStats( FSceneNode* Frame );
};
extern RENDER_API URender* GRender;

/*------------------------------------------------------------------------------------
	Radix float sorting for AlphaBlend Meshes.
------------------------------------------------------------------------------------*/

#define DELETEARRAY(x)	{ if (x != NULL) delete []x;	x = NULL; }
#define CHECKALLOC(x)	if(!x) return false;
#define CHECK_RESIZE(n)																			\
	if(n!=mPreviousSize)																		\
	{																							\
				if(n>mCurrentSize)	Resize(n);													\
		else						ResetIndices();												\
		mPreviousSize = n;																		\
	}

#define CREATE_HISTOGRAMS(type, buffer)															\
	/* Clear counters */																		\
	appMemzero(mHistogram, 256*4*sizeof(DWORD));												\
																								\
	/* Prepare for temporal coherence */														\
	type PrevVal = (type)buffer[mIndices[0]];													\
	bool AlreadySorted = true;	/* Optimism... */												\
	DWORD* Indices = mIndices;																	\
																								\
	/* Prepare to count */																		\
	BYTE* p = (BYTE*)input;																	\
	BYTE* pe = &p[nb*4];																		\
	DWORD* h0= &mHistogram[0];		/* Histogram for first pass (LSB)	*/						\
	DWORD* h1= &mHistogram[256];	/* Histogram for second pass		*/						\
	DWORD* h2= &mHistogram[512];	/* Histogram for third pass			*/						\
	DWORD* h3= &mHistogram[768];	/* Histogram for last pass (MSB)	*/						\
																								\
	while(p!=pe)																				\
	{																							\
		/* Read input buffer in previous sorted order */										\
		type Val = (type)buffer[*Indices++];													\
		/* Check whether already sorted or not */												\
		if(Val<PrevVal)	{ AlreadySorted = false; break; } /* Early out */						\
		/* Update for next iteration */															\
		PrevVal = Val;																			\
																								\
		/* Create histograms */																	\
		h0[*p++]++;	h1[*p++]++;	h2[*p++]++;	h3[*p++]++;											\
	}																							\
																								\
	/* If all input values are already sorted, we just have to return and leave the */			\
	/* previous list unchanged. That way the routine may take advantage of temporal */			\
	/* coherence, for example when used to sort transparent faces.					*/			\
	if(AlreadySorted)	{ mNbHits++; return *this;	}											\
																								\
	/* Else there has been an early out and we must finish computing the histograms */			\
	while(p!=pe)																				\
	{																							\
		/* Create histograms without the previous overhead */									\
		h0[*p++]++;	h1[*p++]++;	h2[*p++]++;	h3[*p++]++;											\
	}

#define CHECK_PASS_VALIDITY(pass)																\
	/* Shortcut to current counters */															\
	DWORD* CurCount = &mHistogram[pass<<8];													\
																								\
	/* Reset flag. The sorting pass is supposed to be performed. (default) */					\
	bool PerformPass = true;																	\
																								\
	/* Check pass validity */																	\
																								\
	/* If all values have the same byte, sorting is useless. */									\
	/* It may happen when sorting bytes or words instead of dwords. */							\
	/* This routine actually sorts words faster than dwords, and bytes */						\
	/* faster than words. Standard running time (O(4*n))is reduced to O(2*n) */					\
	/* for words and O(n) for bytes. Running time for floats depends on actual values... */		\
																								\
	/* Get first byte */																		\
	BYTE UniqueVal = *(((BYTE*)input)+pass);													\
																								\
	/* Check that byte's counter */																\
	if(CurCount[UniqueVal]==nb)	PerformPass=false;

class RadixSort
{

	public:
	// Constructor/Destructor
	RadixSort();
	~RadixSort();
	// Sorting methods
	RadixSort&		RSort(const float* input, DWORD nb);

	//! Access to results. mIndices is a list of indices in sorted order, i.e. in the order you may further process your data
	inline	DWORD*			GetIndices()		const	{ return mIndices;		}

	//! mIndices2 gets trashed on calling the sort routine, but otherwise you can recycle it the way you want.
	inline	DWORD*			GetRecyclable()		const	{ return mIndices2;		}

	// Stats
			DWORD			GetUsedRam()		const;
	//! Returns the total number of calls to the radix sorter.
	inline	DWORD			GetNbTotalCalls()	const	{ return mTotalCalls;	}
	//! Returns the number of premature exits due to temporal coherence.
	inline	DWORD			GetNbHits()			const	{ return mNbHits;		}

	private:
#ifndef RADIX_LOCAL_RAM
			DWORD*			mHistogram;					//!< Counters for each byte
			DWORD*			mOffset;					//!< Offsets (nearly a cumulative distribution function)
#endif
			DWORD			mCurrentSize;				//!< Current size of the indices list
			DWORD			mPreviousSize;				//!< Size involved in previous call
			DWORD*			mIndices;					//!< Two lists, swapped each pass
			DWORD*			mIndices2;
	// Stats
			DWORD			mTotalCalls;
			DWORD			mNbHits;
	// Internal methods
			bool			Resize(DWORD nb);
			void			ResetIndices();
};

/*------------------------------------------------------------------------------------
	Subsystem definition
------------------------------------------------------------------------------------*/

// Function pointer types.
typedef void (*LIGHT_SPATIAL_FUNC)( FTextureInfo& Tex, FLightInfo* Info, BYTE* Src, BYTE* Dest );
typedef void (*LIGHT_VERTEX_SPATIAL_FUNC)(FLOAT& g, FLightInfo* Info);

// Information about one special lighting effect.
struct FLocalEffectEntry
{
	LIGHT_SPATIAL_FUNC	SpatialFxFunc;		// Function to perform spatial lighting
	INT					IsSpatialDynamic;	// Indicates whether light spatiality changes over time.
	INT					IsMergeDynamic;		// Indicates whether merge function changes over time.
};

// Information about a lightsource.

// Light classification.
enum ELightKind
{
	ALO_StaticLight		= 0,	// Actor is a non-moving, non-changing lightsource
	ALO_DynamicLight	= 1,	// Actor is a non-moving, changing lightsource
	ALO_MovingLight		= 2,	// Actor is a moving, changing lightsource
	ALO_NotLight		= 3,	// Not a surface light (probably volumetric only).
};

struct FLightInfo : public FLightVisibilityCache::FLightData
{
	friend struct FAtlasManager;

	// For all lights.
	ELightKind	Opt;					// Light type.
	FVector		Location;				// Transformed screenspace location of light.
	FLOAT		RRadiusMult;			// 16383.0f / (Radius * Radius).
	FLOAT		Brightness;				// Center brightness at this instance, 1.0f=max, 0.0f=none.
	FLOAT		Diffuse;				// BaseNormalDelta * RRadius.
	BYTE*		IlluminationMap;		// Temporary illumination map pointer.
	BYTE*		ShadowBits;				// Temporary shadow map.
	BYTE		bIsSpecialFX;			// For special effects useage (LE_StaticLight, LE_Spotlight ...).
	FVector		LightActorDir;			// For special effects useage.
	UINT		DrawFrame;				// Most recent drawn frame number.
	BYTE		bStaticLight;			// Light data will never change anymore.
	FVector		ActorDirection;			// Direction of the actor (only on special light effects).
	FSceneNode* OldFrame;

	// Clipping region.
	INT MinU, MaxU, MinV, MaxV;

	// For volumetric lights.
	BYTE		VolInside;				// Viewpoint is inside the sphere.
	BYTE		bSpotlightFog;			// Directional fog.
	BYTE		bVolumetricVisible;		// Volumetric is currently visible?

	// Information about the lighting effect.
	FLocalEffectEntry Effect;
	LIGHT_VERTEX_SPATIAL_FUNC VertEffect;

	// Coloring.
	FColor*		Palette;				// Brightness scaler.
	FRedGreenBlue10Alpha2* PaletteHD;
	FColor*     VolPalette;             // Volumetric color scaler.
	FPlane OldLightColor, OldHDLightColor, OldVolColor;

	INT LightTag, FogTag;

	FLightInfo(AActor* LightActor)
		: DrawFrame(~GFrameNumber), bStaticLight(0),  bVolumetricVisible(1), Palette(NULL), PaletteHD(NULL), VolPalette(NULL), LightTag(0), FogTag(0)
	{
		STAT(GStatRender.LightMemActor.Value += sizeof(FLightInfo));
		Actor = LightActor;
	}
	~FLightInfo()
	{
		STAT(GStatRender.LightMemActor.Value -= sizeof(FLightInfo));
		if (Palette)
			delete[] Palette;
		if (PaletteHD)
			delete[] PaletteHD;
		if (VolPalette)
			delete[] VolPalette;
	}

	// Functions.
	void Update(FSceneNode* Frame);
	void ComputeFromActor( FTextureInfo* Map, FSceneNode* Frame );

	inline static FLightInfo* GetInfo(AActor* Actor)
	{
		FLightVisibilityCache* VisCache = Actor->LightDataPtr;
		if (!VisCache)
		{
			VisCache = new FLightVisibilityCache;
			Actor->LightDataPtr = VisCache;
		}
		if (!VisCache->LightInfo)
		{
			FLightInfo* LI = new FLightInfo(Actor);
			LI->Actor = Actor;
			VisCache->LightInfo = LI;
		}
		return (FLightInfo*)VisCache->LightInfo;
	}
	inline void CheckForUpdate(FSceneNode* Frame)
	{
		if (DrawFrame != GFrameNumber)
		{
			DrawFrame = GFrameNumber;
			Update(Frame);
		}
	}

private:
	FLightInfo() {}
};

struct FBspStaticLight
{
	AActor* Actor;
	BYTE* ShadowBits;
	BYTE* ShadowMap,* IllumMap;
	INT OmniIndex;

	FBspStaticLight* Next;

	FBspStaticLight(AActor* InActor, FBspStaticLight* N)
		: Actor(InActor), ShadowBits(nullptr), ShadowMap(nullptr), IllumMap(nullptr), OmniIndex(INDEX_NONE), Next(N)
	{
		STAT(GStatRender.LightMemBSP.Value += sizeof(FBspStaticLight));
	}
	~FBspStaticLight()
	{
		guardSlow(FBspStaticLight::~FBspStaticLight);
		STAT(GStatRender.LightMemBSP.Value -= sizeof(FBspStaticLight));
		if (ShadowMap)
			appFree(ShadowMap);
		if (IllumMap)
			appFree(IllumMap);
		unguardSlow;
	}
	inline UBOOL IsInvalidSource() const
	{
		return (Actor->IsDynamicLight() || Actor->bDeleteMe);
	}
};

struct FLightmapData
{
	DWORD* Data;
	DWORD U, V, NumRef;
	DWORD BuildU, BuildV, RowWidth;
	BYTE UBits, VBits;
	QWORD CacheID;
	BITFIELD bIsHDMap : 1;
	BITFIELD bIsDirty : 1;

	FLightmapData(BYTE UBit, BYTE VBit, QWORD Cache, UBOOL bHD);
	~FLightmapData();

	UBOOL FitsLightmap(FLightMapIndex* Index) const; // Check if still has space for more sub-lightmaps.
	FVector ReserveLightmap(FLightMapIndex* Index, BYTE** Offset);

	inline DWORD GetMemSize() const
	{
		return sizeof(FLightmapData) + (U * V * sizeof(DWORD));
	}
	inline void AddRef()
	{
		++NumRef;
	}
	inline void Release()
	{
		if (--NumRef == 0)
			delete this;
	}
};

struct FLightMapTexInfo : public FTextureInfo
{
	BYTE* StartOffset;
	FMipmapBase MipBase;

	FLightMapTexInfo(FLightmapData* Data, FLightMapIndex* Index, const FVector& BasePan);
	~FLightMapTexInfo();
};

struct FNodeLightMap
{
	UINT LastUpdateFrame;
	FLightMapTexInfo* LightMap;
	FLightmapData* LightData;
	DWORD* StaticLightMap;
	UBOOL bHadDynamicLights;

	FNodeLightMap()
		: LastUpdateFrame(~GFrameNumber), LightMap(nullptr), LightData(nullptr), StaticLightMap(nullptr), bHadDynamicLights(FALSE)
	{
	}
	~FNodeLightMap()
	{
		if (LightMap)
			delete LightMap;
		if (StaticLightMap)
			appFree(StaticLightMap);
		if (LightData)
			LightData->Release();
	}
};

enum EPrevLightModeFlags : BYTE
{
	PLIGHTMODE_Fog = 1,
	PLIGHTMODE_Lit = 2,
};

struct FBspLightMap : public FBspRenderCache
{
	BITFIELD bDynamicSurf : 1, bHasSpatialFX : 1, bInitStaticLights : 1, bStaticLightDirty : 1, bPortalFog : 1;
	FCoords RMapCoords GCC_PACK(VECTOR_ALIGNMENT); // Untransformed map coords for light manager.
	FCoords MapUncoords;
	FVector VertexBase;
	FVector VertexDU;
	FVector VertexDV;
	FNodeLightMap* MainMapInfo;
	FNodeLightMap** ZoneLightInfo;
	FLightMapTexInfo* FogMapInfo;
	BYTE PrevLightMode;
	FBspStaticLight* StaticLights,* FullStaticLights;
	DWORD MemSpent;
	FLightmapData* FogData;
	BYTE ZoneNumber;

	UINT LastFogComputeFrame;
	UINT DynBSPUpdateTag;

	FBspLightMap(ULevel* L, INT iSurf, BYTE iZone)
		: bHasSpatialFX(FALSE), bInitStaticLights(FALSE), bStaticLightDirty(FALSE), bPortalFog(FALSE), MainMapInfo(nullptr), ZoneLightInfo(nullptr), FogMapInfo(nullptr)
		, PrevLightMode(0), StaticLights(nullptr), FullStaticLights(nullptr), MemSpent(0)
		, FogData(nullptr), ZoneNumber(iZone)
		, LastFogComputeFrame(~GFrameNumber), DynBSPUpdateTag(0)
	{
		STAT(MemSpent = sizeof(FBspLightMap));
		STAT(GStatRender.LightMemBSP.Value += sizeof(FBspLightMap));
		bDynamicSurf = L->BrushTracker ? L->BrushTracker->SurfIsDynamic(iSurf) : FALSE;
	}
	~FBspLightMap()
	{
		guardSlow(FBspLightMap::~FBspLightMap);
		STAT(GStatRender.LightMemBSP.Value -= MemSpent);
		if (ZoneLightInfo)
		{
			for (INT i = 0; i < FBspNode::MAX_ZONES; ++i)
				if (ZoneLightInfo[i])
					delete ZoneLightInfo[i];
			appFree(ZoneLightInfo);
		}
		else if (MainMapInfo)
			delete MainMapInfo;
		if (FogMapInfo)
			delete FogMapInfo;
		if (FogData)
			FogData->Release();
		if (StaticLights)
		{
			FBspStaticLight* N;
			for (FBspStaticLight* L = StaticLights; L; L = N)
			{
				N = L->Next;
				delete L;
			}
		}
		if (FullStaticLights)
		{
			FBspStaticLight* N;
			for (FBspStaticLight* L = FullStaticLights; L; L = N)
			{
				N = L->Next;
				delete L;
			}
		}
		unguardSlow;
	}
	inline void CalcSurfMap(UModel* Model, FBspSurf* Surf)
	{
		RMapCoords = FCoords
		(
			Model->Points(Surf->pBase),
			Model->Vectors(Surf->vTextureU),
			Model->Vectors(Surf->vTextureV),
			Model->Vectors(Surf->vNormal)
		);
		if (Surf->iLightMap != INDEX_NONE)
		{
			MapUncoords = RMapCoords.Inverse().Transpose();

			FLightMapIndex* Index = NULL;
			if (bDynamicSurf)
			{
				UModel* M = Surf->Actor->Brush;
				if (M && M->LightMap.IsValidIndex(Surf->iLightMap))
					Index = &M->LightMap(Surf->iLightMap);
			}
			else if (Model->LightMap.IsValidIndex(Surf->iLightMap))
				Index = &Model->LightMap(Surf->iLightMap);

			if(Index)
			{
				VertexBase = RMapCoords.Origin + MapUncoords.XAxis * Index->Pan.X + MapUncoords.YAxis * Index->Pan.Y;
				VertexDU = MapUncoords.XAxis * Index->UScale;
				VertexDV = MapUncoords.YAxis * Index->VScale;
			}
			else
			{
				VertexBase = RMapCoords.Origin;
				VertexDU = MapUncoords.XAxis;
				VertexDV = MapUncoords.YAxis;
			}
		}
		DynBSPUpdateTag = Surf->UpdateTag;
	}
	void AddStaticLight(AActor* Light, BYTE* Bits);
	void RemoveStaticLight(FBspStaticLight* L);
	void AddFullStaticLight(AActor* Light);
	void FlushStaticLights();
	FLightmapData* FindSharedLightmap(FLightMapIndex* Index, BYTE iZone, UBOOL bAllowHD);
};

/*------------------------------------------------------------------------------------
	The End.
------------------------------------------------------------------------------------*/
