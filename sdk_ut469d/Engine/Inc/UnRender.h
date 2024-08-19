/*=============================================================================
	UnRender.h: Rendering functions and structures
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#ifndef _UNRENDER_H_
#define _UNRENDER_H_

/*------------------------------------------------------------------------------------
	Forward declarations.
------------------------------------------------------------------------------------*/

class  URenderDevice;
class  FSpan;
class  FSpanBuffer;
class  FRasterPoly;
struct FTransTexture;
struct FScreenBounds;
struct FSurfaceInfo;
struct FSurfaceFacet;
struct FSceneNode;
struct FDynamicItem;
struct FDynamicSprite;
struct FBspDrawList;
struct FSavedPoly;
struct FStaticBspInfoBase;

/*------------------------------------------------------------------------------------
	Includes.
------------------------------------------------------------------------------------*/

#include "UnRenDev.h"

/*------------------------------------------------------------------------------------
	Defines.
------------------------------------------------------------------------------------*/

#define ORTHO_LOW_DETAIL 40000.0f

/*------------------------------------------------------------------------------------
	FSceneNode.
------------------------------------------------------------------------------------*/

//
// A scene frame is a temporary object representing a portion of
// the view of the world to render.
//
class FSpanBuffer;
struct FBspDrawList;
struct FDynamicSprite;
struct ENGINE_API FSceneNode
{
	// Variables.
	UViewport*		Viewport;		// Viewport the scene frame is attached to.
	ULevel*			Level;			// Level this scene is being rendered from.
	FSceneNode*		Parent;			// Frame from whence this was created, NULL=top level.
	FSceneNode*		Sibling;		// Next sibling scene frame.
	FSceneNode*		Child;			// Next child scene frame.
	INT				iSurf;			// Surface seen through (Parent,iSurface pair is unique).
	INT				ZoneNumber;		// Inital rendering zone of viewport in destination level (NOT the zone of the viewpoint!)
	INT				Recursion;		// Recursion depth, 0 if initial.
	FLOAT			Mirror;			// Mirror value, 1.f or -1.f.
	FPlane			NearClip;		// Near-clipping plane in screenspace.
	FCoords			Coords;			// Transform coordinate system.
	FCoords			Uncoords;		// Inverse coordinate system.
	FSpanBuffer*	Span;			// Initial span buffer for the scene.
	FBspDrawList*	Draw[3];		// Draw lists (portals, occluding, non-occluding).
	FDynamicSprite* Sprite;			// Sprites to draw.
	INT				X, Y;			// Frame size.
	INT				XB, YB;			// Offset of top left active viewport.

	// Precomputes.
	FLOAT			FX, FY;			// Floating point X,Y.
	FLOAT			FX15, FY15;		// (Floating point SXR + 1.0001f)/2.f.
	FLOAT			FX2, FY2;		// Floating point SXR / 2.f.
	FLOAT			Zoom;			// Zoom value, based on OrthoZoom and size.
	FVector			Proj;      		// Projection vector.
	FVector			RProj;			// Reverse projection vector.
	FLOAT			PrjXM, PrjYM;	// Minus clipping numbers.
	FLOAT			PrjXP, PrjYP;	// Plus clipping numbers.
	FVector			ViewSides [4];	// 4 unit vectors indicating view frustrum extent lines.
	FPlane			ViewPlanes[4];	// 4 planes indicating view frustrum extent planes.

	// Functions.
	BYTE* Screen( INT X, INT Y ) {return Viewport->ScreenPointer + (X+XB+(Y+YB)*Viewport->Stride)*Viewport->ColorBytes;}

	// Old variants accessing Viewport and RenDev.
	void ComputeRenderSize();
	void ComputeRenderCoords( FVector& Location, FRotator& Rotation );

	// New variants, which won't access Viewport.
	void ComputeRenderCoords(FVector& Location, FRotator& Rotation, INT RendMap, FLOAT FovAngle, FLOAT OrthoZoom);
	void ComputeRenderSize(FLOAT FovAngle, FLOAT OrthoZoom);
};

/*------------------------------------------------------------------------------------
	Transformations.
------------------------------------------------------------------------------------*/

//
// Transformed vector with outcode info.
//
struct FOutVector
{
	FVector Point;
	BYTE    Flags;
};

//
// Transformed and projected vector.
//
struct FTransform : public FOutVector
{
	FLOAT ScreenX;
	FLOAT ScreenY;
	INT   IntY;
	FLOAT RZ;
	void Project( const FSceneNode* Frame )
	{
		RZ      = Frame->Proj.Z / Point.Z;
		ScreenX = Point.X * RZ + Frame->FX15;
		ScreenY = Point.Y * RZ + Frame->FY15;
		IntY    = appFloor( ScreenY );
	}
	void ComputeOutcode( const FSceneNode* Frame )
	{
		static FLOAT ClipXM, ClipXP, ClipYM, ClipYP;
		static const BYTE OutXMinTab [2] = { 0, FVF_OutXMin };
		static const BYTE OutXMaxTab [2] = { 0, FVF_OutXMax };
		static const BYTE OutYMinTab [2] = { 0, FVF_OutYMin };
		static const BYTE OutYMaxTab [2] = { 0, FVF_OutYMax };

		ClipXM = Frame->PrjXM * Point.Z + Point.X;
		ClipXP = Frame->PrjXP * Point.Z - Point.X;
		ClipYM = Frame->PrjYM * Point.Z + Point.Y;
		ClipYP = Frame->PrjYP * Point.Z - Point.Y;
		Flags  =
		(	OutXMinTab [ClipXM < 0.f]
		+	OutXMaxTab [ClipXP < 0.f]
		+	OutYMinTab [ClipYM < 0.f]
		+	OutYMaxTab [ClipYP < 0.f]);
	}
	FTransform operator+( const FTransform& V ) const
	{
		FTransform Temp;
		Temp.Point = Point + V.Point;
		return Temp;
	}
	FTransform operator-( const FTransform& V ) const
	{
		FTransform Temp;
		Temp.Point = Point - V.Point;
		return Temp;
	}
	FTransform operator*(FLOAT Scale ) const
	{
		FTransform Temp;
		Temp.Point = Point * Scale;
		return Temp;
	}
};

//
// Transformed sample point.
//
struct FTransSample : public FTransform
{
	FPlane Normal, Light, Fog;
	FTransSample operator+(const FTransSample& T) const
	{
		FTransSample Temp;
		Temp.Point  = Point  + T.Point;
		Temp.Normal = Normal + T.Normal;
		Temp.Light  = Light  + T.Light;
		Temp.Fog    = Fog    + T.Fog;
		return Temp;
	}
	FTransSample operator-(const FTransSample& T) const
	{
		FTransSample Temp;
		Temp.Point  = Point  - T.Point;
		Temp.Normal = Normal - T.Normal;
		Temp.Light  = Light  - T.Light;
		Temp.Fog    = Fog    - T.Fog;
		return Temp;
	}
	FTransSample operator*(FLOAT Scale) const
	{
		FTransSample Temp;
		Temp.Point  = Point * Scale;
		Temp.Normal = Normal * Scale;
		Temp.Light  = Light * Scale;
		Temp.Fog    = Fog * Scale;
		return Temp;
	}
};

//
// Transformed texture mapped point.
//
struct FTransTexture : public FTransSample
{
	FLOAT U, V;
	FTransTexture operator+(const FTransTexture& T) const
	{
		FTransTexture Temp;
		Temp.Point  = Point  + T.Point;
		Temp.Normal = Normal + T.Normal;
		Temp.Light  = Light  + T.Light;
		Temp.Fog    = Fog    + T.Fog;
		Temp.U = U + T.U;
		Temp.V = V + T.V;
		return Temp;
	}
	FTransTexture operator-(const FTransTexture& T) const
	{
		FTransTexture Temp;
		Temp.Point  = Point  - T.Point;
		Temp.Normal = Normal - T.Normal;
		Temp.Light  = Light  - T.Light;
		Temp.Fog    = Fog    - T.Fog;
		Temp.U = U - T.U;
		Temp.V = V - T.V;
		return Temp;
	}
	FTransTexture operator*(FLOAT Scale) const
	{
		FTransTexture Temp;
		Temp.Point  = Point * Scale;
		Temp.Normal = Normal * Scale;
		Temp.Light  = Light * Scale;
		Temp.Fog    = Fog * Scale;
		Temp.U = U * Scale;
		Temp.V = V * Scale;
		return Temp;
	}
};

/*------------------------------------------------------------------------------------
	FSurfaceInfo.
------------------------------------------------------------------------------------*/

//
// Description of a renderable surface.
//
struct FSurfaceInfo
{
	DWORD			PolyFlags;		// Surface flags.
	FColor			FlatColor;		// Flat-shaded color.
	ULevel*			Level;			// Level to render.
	FTextureInfo*	Texture;		// Regular texture mapping info, if any.
	FTextureInfo*	LightMap;		// Light map, if any.
	FTextureInfo*	MacroTexture;	// Macrotexture, if any.
	FTextureInfo*	DetailTexture;	// Detail map, if any.
	FTextureInfo*	FogMap;			// Fog map, if any.
	AZoneInfo*		Zone;			// Zone this surface corresponds to.
};

//
// A saved polygon.
//
struct FSavedPoly
{
	FSavedPoly* Next;
	INT			iNode;
	void*       User;
	INT         NumPts;
	FTransform* Pts[ZEROARRAY];
};

//
// Description of a surface facet, represented as either
// a convex polygon or a concave span buffer.
//
struct FSurfaceFacet
{
	FCoords			MapCoords;		// Mapping coordinates.
	FCoords			MapUncoords;	// Inverse mapping coordinates.
	FSpanBuffer*	Span;			// Span buffer, if rendering device wants it.
	FSavedPoly*		Polys;			// Polygon list.
};

/*------------------------------------------------------------------------------------
	FScreenBounds.
------------------------------------------------------------------------------------*/

//
// Screen extents of an axis-aligned bounding box.
//
struct ENGINE_API FScreenBounds
{
	FLOAT MinX, MinY;
	FLOAT MaxX, MaxY;
	FLOAT MinZ;
};

/*------------------------------------------------------------------------------------
	URenderBase.
------------------------------------------------------------------------------------*/

//
// Line drawing flags.
//
enum ELineFlags
{
	LINE_None,
	LINE_Transparent,
	LINE_DepthCued
};

//
// Pure virtual base class of the rendering subsytem.
//
class ENGINE_API URenderBase : public USubsystem
{
	DECLARE_ABSTRACT_CLASS(URenderBase,USubsystem,CLASS_Transient|CLASS_Config,Engine)
	NO_DEFAULT_CONSTRUCTOR(URenderBase);

	// Variables.
	UEngine* Engine;

	// Init/exit functions.
	virtual void Init( UEngine* InEngine ) {Engine=InEngine;}
	virtual UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar=*GLog )=0;

	// Prerender/postrender functions.
	virtual void PreRender( FSceneNode* Frame )=0;
	virtual void PostRender( FSceneNode* Frame )=0;

	// Scene frame management.
	virtual FSceneNode* CreateMasterFrame( UViewport* Viewport, FVector Location, FRotator Rotation, FScreenBounds* Bounds )=0;
	virtual FSceneNode* CreateChildFrame( FSceneNode* Parent, FSpanBuffer* Span, ULevel* Level, INT iSurf, INT iZone, FLOAT Mirror, const FPlane& NearClip, const FCoords& Coords, FScreenBounds* Bounds )=0;
	virtual void FinishMasterFrame()=0;

	// Major rendering functions.
	virtual void DrawWorld( FSceneNode* Frame )=0;
	virtual void DrawActor( FSceneNode* Frame, AActor* Actor )=0;

	// Other functions.
	virtual UBOOL Project( FSceneNode* Frame, const FVector &V, FLOAT &ScreenX, FLOAT &ScreenY, FLOAT* Scale )=0;
	virtual UBOOL Deproject( FSceneNode* Frame, INT ScreenX, INT ScreenY, FVector& V )=0;
	virtual UBOOL BoundVisible( FSceneNode* Frame, FBox* Bound, FSpanBuffer* SpanBuffer, FScreenBounds& Results )=0;
	virtual void GetVisibleSurfs( UViewport* Viewport, TArray<INT>& iSurfs )=0;
	virtual void GlobalLighting( UBOOL Realtime, AActor* Owner, FLOAT& Brightness, FPlane& Color )=0;
	virtual void Precache( UViewport* Viewport )=0;

	// High level primitive drawing.
	virtual void DrawCircle( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector& Location, FLOAT Radius, UBOOL bScaleRadiusByZoom = 0 )=0;
	virtual void DrawBox( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector Min, FVector Max )=0;
	virtual void DrawCylinder( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector& Location, FLOAT Radius, FLOAT Height )=0;
	virtual void DrawSphere(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector Location, FLOAT r, INT n)=0;
};

/*------------------------------------------------------------------------------------
	Static geometry streaming.
------------------------------------------------------------------------------------*/

//
// A static BSP vertex prepared for render buffering
//
struct FStaticBspVertex
{
	FVector Point;

	// Texture coordinates *always unscaled*
	FLOAT TextureU, TextureV;

	// Optional light and fog map coordinates
	FLOAT LightU, LightV;
	FLOAT AtlasU, AtlasV;
};

//
// A static BSP node prepared for render buffering
//
struct FStaticBspNode
{
	FPlane Plane;
	INT SurfaceIndex;

	// Vertex indices for per-node rendering mode
	// Vertices are presented as a N vertex polygon
	INT VertexStart;
	INT VertexCount;
};

//
// A static BSP surface prepared for render buffering
//
struct FStaticBspSurf
{
	UTexture* Texture;
	SWORD PanU, PanV;
	DWORD PolyFlags;
	INT   iLightMap;
	QWORD SortKey;

	BYTE ZoneNumber; // Surfaces are split if in more than one zone

	// Vertex indices for per-surface rendering mode
	// Vertices are presented as triangles (VertexCount is always multiple of 3)
	INT VertexStart;
	INT VertexCount;

	// Surface group index
	INT Group;

	bool IsTwoSided()
	{
		return (PolyFlags & (PF_TwoSided|PF_Portal)) != 0;
	}
};

//
// A group of static BSP surfs
//
struct FStaticBspSurfGroup
{
	// Surface indices in SurfList
	INT SurfStart;
	INT SurfCount;

	// Vertex indices for all surfaces in this group
	// Vertices are presented as triangles (VertexCount is always multiple of 3)
	INT VertexStart;
	INT VertexCount;
};

enum EStaticBspMode
{
	// Higor: experimental, subject to changes
	STATIC_BSP_None    = 0, // No static BSP.
	STATIC_BSP_PerNode = 1, // Draw/Compute Static Geometry using BSP node number.
	STATIC_BSP_PerSurf = 2, // Draw/Compute Static Geometry using BSP surf.
};

//
// Static BSP render parameters.
// To be used for streaming static geometry to the GPU
// Warning: Render may provide RenDev469 with a subclass of this object!
//
struct ENGINE_API FStaticBspInfoBase
{
	// Types
	typedef void (*SurfCreatedNotify_t)(FStaticBspSurf&);

	// Control
	ULevel*         Level;
	QWORD           GeometryID;
	UBOOL           SourceGeometryChanged;

	// Current frame parameters
	FColor          ZoneAmbientColor[FBspNode::MAX_ZONES];
	FLOAT           ZoneAutoPanU[FBspNode::MAX_ZONES];
	FLOAT           ZoneAutoPanV[FBspNode::MAX_ZONES];
	FLOAT           WavyPanU, WavyPanV;

	// Static geometry
	// Note: only NodeList is guaranteed to match the source UModel's Node array layout.
	EStaticBspMode              StaticGeometryMode;
	TArray<FStaticBspVertex>    VertList;
	TArray<FStaticBspNode>      NodeList;
	TArray<FStaticBspSurf>      SurfList;
	TArray<FStaticBspSurfGroup> SurfGroups;

	// Callbacks
	SurfCreatedNotify_t SurfCreatedNotify; // A new FStaticBspSurf has been created

	FStaticBspInfoBase() {};
	FStaticBspInfoBase( ULevel* InLevel);


	// FStaticBspInfoBase interface
	// To be used by supported Render Devices

	// Updates render parameters
	virtual void Update();

	// Static geometry generation
	virtual void ComputeStaticGeometry( EStaticBspMode ComputeMode);

	// Precalculates Light UVs
	virtual void ComputeLightUV();


	// Default notifies
	static void SurfCreatedNotifyDefault( FStaticBspSurf& NewSurf)
	{
		QWORD Key = GetTypeHash(NewSurf.Texture) & 0x00FFFFFF;
		if ( NewSurf.PolyFlags & PF_NoSmooth )      Key |= 0x01000000;
		if ( NewSurf.PolyFlags & PF_Masked )        Key |= 0x02000000;
		if ( NewSurf.PolyFlags & PF_Invisible )     Key |= 0x04000000;
		if ( NewSurf.PolyFlags & PF_Highlighted )   Key |= 0x08000000;
		if ( NewSurf.PolyFlags & PF_Modulated )     Key |= 0x10000000;
		if ( NewSurf.PolyFlags & PF_Translucent )   Key |= 0x20000000;
		if ( NewSurf.PolyFlags & PF_Occlude )       Key |= 0x40000000;
		Key |= ((QWORD)NewSurf.ZoneNumber << 32);
		NewSurf.SortKey = Key;
	}
};

/*-----------------------------------------------------------------------------
	Hit proxies.
-----------------------------------------------------------------------------*/

// Hit a Bsp surface.
struct ENGINE_API HBspSurf : public HHitProxy
{
	DECLARE_HIT_PROXY(HBspSurf,HHitProxy)
	INT iSurf;
	HBspSurf( INT iInSurf ) : iSurf( iInSurf ) {}
};

// Hit an actor.
struct ENGINE_API HActor : public HHitProxy
{
	DECLARE_HIT_PROXY(HActor,HHitProxy)
	AActor* Actor;
	HActor( AActor* InActor ) : Actor( InActor ) {}
};

// Hit ray descriptor.
struct ENGINE_API HCoords : public HHitProxy
{
	DECLARE_HIT_PROXY(HCoords,HHitProxy)
	FCoords Coords, Uncoords;
	FVector Direction;
	HCoords( FSceneNode* InFrame )
	:	Coords  ( InFrame->Coords   )
	,	Uncoords( InFrame->Uncoords )
	{
		FLOAT X = InFrame->Viewport->HitX+InFrame->Viewport->HitXL/2;
		FLOAT Y = InFrame->Viewport->HitY+InFrame->Viewport->HitYL/2;
		Direction
		=	InFrame->Coords.ZAxis
		+	InFrame->Coords.XAxis * (X - InFrame->FX2) * InFrame->RProj.Z
		+	InFrame->Coords.YAxis * (Y - InFrame->FY2) * InFrame->RProj.Z;
	}
};

#endif

/*------------------------------------------------------------------------------------
	The End.
------------------------------------------------------------------------------------*/
