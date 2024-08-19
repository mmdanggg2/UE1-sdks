/*=============================================================================
	UnRenDev.h: 3D rendering device class.

	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
	Compiled with Visual C++ 4.0. Best viewed with Tabs=4.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#ifndef _UNRENDEV_H_
#define _UNRENDEV_H_

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack(push,OBJECT_ALIGNMENT)
#endif

/*------------------------------------------------------------------------------------
	URenderDevice.
------------------------------------------------------------------------------------*/

// Flags for locking a rendering device.
enum ELockRenderFlags
{
	LOCKR_ClearScreen	    = 1,
	LOCKR_LightDiminish     = 2,
};
enum EDescriptionFlags
{
	RDDESCF_Certified       = 1,
	RDDESCF_Incompatible    = 2,
	RDDESCF_LowDetailWorld  = 4,
	RDDESCF_LowDetailSkins  = 8,
	RDDESCF_LowDetailActors = 16,
};

//
// A low-level 3D rendering device.
//
class ENGINE_API URenderDevice : public USubsystem
{
	DECLARE_ABSTRACT_CLASS(URenderDevice,USubsystem,CLASS_Config,Engine)

	// Variables.
	BYTE			DecompFormat;
	INT				RecommendedLOD;
	UViewport*		Viewport;
	FString			Description;
	DWORD			DescFlags;
	BITFIELD		SpanBased;
	BITFIELD		FullscreenOnly;
	BITFIELD		SupportsFogMaps;
	BITFIELD		SupportsDistanceFog;
	BITFIELD		VolumetricLighting;
	BITFIELD		ShinySurfaces;
	BITFIELD		Coronas;
	BITFIELD		HighDetailActors;
	BITFIELD		SupportsTC;
	BITFIELD		PrecacheOnFlip;
	BITFIELD		SupportsLazyTextures;
	BITFIELD		PrefersDeferredLoad;
	BITFIELD		DetailTextures;
	BITFIELD		Pad1[8];
	DWORD			Pad0[8];

	// Constructors.
	void StaticConstructor();

	// URenderDevice low-level functions that drivers must implement.
	virtual void Placeholder() {}
	virtual UBOOL Init( UViewport* InViewport, INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen )=0;
	virtual UBOOL SetRes( INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen )=0;
	virtual void Exit()=0;
	virtual void Flush( UBOOL AllowPrecache )=0;
	virtual UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar );
	virtual void Lock( FPlane FlashScale, FPlane FlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* HitData, INT* HitSize )=0;
	virtual void Unlock( UBOOL Blit )=0;
	virtual void DrawComplexSurface( FSceneNode* Frame, FSurfaceInfo& Surface, FSurfaceFacet& Facet )=0;
	virtual void DrawGouraudPolygon( FSceneNode* Frame, FTextureInfo& Info, FTransTexture** Pts, int NumPts, DWORD PolyFlags, FSpanBuffer* Span )=0;
	virtual void DrawTile( FSceneNode* Frame, FTextureInfo& Info, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, class FSpanBuffer* Span, FLOAT Z, FPlane Color, FPlane Fog, DWORD PolyFlags )=0;
	virtual void Draw3DLine( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector OrigP, FVector OrigQ );
	virtual void Draw2DClippedLine( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2 );
	virtual void Draw2DLine( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2 )=0;
	virtual void Draw2DPoint( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FLOAT X1, FLOAT Y1, FLOAT X2, FLOAT Y2, FLOAT Z )=0;
	virtual void ClearZ( FSceneNode* Frame )=0;
	virtual void PushHit( const BYTE* Data, INT Count )=0;
	virtual void PopHit( INT Count, UBOOL bForce )=0;
	virtual void GetStats( TCHAR* Result )=0;
	virtual void ReadPixels( FColor* Pixels )=0;
	virtual void EndFlash() {}
	virtual void DrawStats( FSceneNode* Frame ) {}
	virtual void SetSceneNode( FSceneNode* Frame ) {}
	virtual void PrecacheTexture( FTextureInfo& Info, DWORD PolyFlags ) {}

	// Padding.
	virtual void vtblPad0() {}
	virtual void vtblPad1() {}
	virtual void vtblPad2() {}
	virtual void vtblPad3() {}
	virtual void vtblPad4() {}
	virtual void vtblPad5() {}
	virtual void vtblPad6() {}
	virtual void vtblPad7() {}
};

/*------------------------------------------------------------------------------------
	OldUnreal Rendering Extensions.
------------------------------------------------------------------------------------*/
//
// This is a rendering interface that provides extended APIs to better leverage the 
// capabilities of modern GPUs. Future renderdevices should subclass this interface 
// rather than the regular URenderDevice.
//
// The idea is that we always provide a default implementation that implements the
// desired functionality on top of legacy rendering APIs. Currently, we only implement
// the DrawGouraudTriangles API.
//
class ENGINE_API URenderDeviceOldUnreal469 : public URenderDevice
{
	#define RENDER_DEVICE_OLDUNREAL 469

	DECLARE_ABSTRACT_CLASS(URenderDeviceOldUnreal469, URenderDevice, CLASS_Config, Engine)

	FLOAT Dot[32];

	//
	// OldUnreal's UT builds draw text with PF_Highlighted passed as a PolyFlag
	// to DrawTile. Old versions of UnrealEd used PF_Highlighted on surfaces to
	// indicate that they were selected in one of the editor viewports. PF_Highlighted
	// was then deprecated in favor of PF_Selected, but the PolyFlag was never officially
	// removed. We use it because:
	// 
	// 1) Most rendevs still support it
	// 2) It is the best PolyFlag for premultiplied alpha blending
	//
	// On renderers we do not maintain ourselves, we set NeedsMaskedFonts to true
	// because those renderers might not support PF_Highlighted. With NeedsMaskedFonts
	// set to true, we render fonts using PF_Masked instead of PF_Highlighted.
	// PF_Masked tiles are also rendered with premultiplied alpha blending, but texels
	// with alpha values below 0.5 are usually discarded. This makes our font glyphs
	// look rather bad as they now get lots of jagged edges.
	// 
	UBOOL NeedsMaskedFonts{};

	//
	// OldUnreal's UT builds can merge all lightmaps into a single lightmap atlas texture.
	// This massively improves performance in renderers that support large textures.
	// 
	UBOOL UseLightmapAtlas;
	UBOOL SupportsUpdateTextureRect;
	INT   MaxTextureSize;
	UBOOL SupportsStaticBsp;
	UBOOL UseAmbientlessLightmaps;
	INT   Padding[251];

	/**
	*	Extended APIs.
	* 
	*	Uses:
	*	- NeedsMaskedFonts
	*	- UseLightmapAtlas
	* 
	*	Required by all URenderDeviceOldUnreal469 implementations.
	*/

	// Triangle batching API. The default implementation does enviromapping, backface culling, clipping, and then renders using the old DrawGouraudPolygon interface
	virtual void DrawGouraudTriangles(const FSceneNode* Frame, const FTextureInfo& Info, FTransTexture* const Pts, INT NumPts, DWORD PolyFlags, DWORD DataFlags, FSpanBuffer* Span);

	// Renderer can draw or decode this texture format.
	virtual UBOOL SupportsTextureFormat( ETextureFormat Format);


	/**
	*	Partial Texture update API.
	* 
	*	Requires:
	*	- SupportsUpdateTextureRect
	* 
	*	Uses:
	*	- MaxTextureSize
	* 
	*	Designed to improve interaction with the Lightmap Atlas
	*	When enabled, the Lightmap Atlas will also use fogmap and dynamic light atlases.
	*/
	virtual void UpdateTextureRect( FTextureInfo& Info, INT U, INT V, INT UL, INT VL) {};

#ifdef PROTOTYPE_STATIC_GEOMETRY_BUFFERING
	/**
	*	Static BSP API.
	* 
	*	Requires:
	*	- SupportsStaticBsp
	* 
	*	Uses:
	*	- UsesAmbientlessLightmaps
	* 
	*	If the Render Device calls StaticBspInfo's ComputeStaticGeometry during SetStaticBsp
	*	then the Render Interface will issue DrawStaticBspNode, DrawStaticBspSurf calls
	*	instead DrawComplexSurface for rendering static BSP geometry.
	* 
	*	When SupportsAmbientlessLightmaps is enabled, the render interface will not apply
	*	the ZoneInfo's AmbientColor to it, instead the render device must apply it on
	*	the fragment shader.
	*	This ensures the lightmap atlas is used on surfaces contained by multiple zones.
	*/
	virtual void SetStaticBsp( FStaticBspInfoBase& StaticBspInfo) {};
	virtual void DrawStaticBspNode( INT iNode, FSurfaceInfo& Surface) {}; // Subject to changes
	virtual void DrawStaticBspSurf( INT iSurf, FSurfaceInfo& Surface) {}; // Subject to changes
#endif

private:

	// Used internally in the triangle batching API.
	INT Clip			(const FSceneNode* Frame, FTransTexture** Dest, FTransTexture** Src, INT SrcNum);
	void EnviroMap		(const FSceneNode* Frame, FTransTexture& P, FLOAT UScale, FLOAT VScale);
	void PushTriangle	(const FSceneNode* Frame, const FTextureInfo& Texture, FTransTexture** Pts, DWORD PolyFlags, class FSpanBuffer* Span);
};

/*-----------------------------------------------------------------------------
	URenderDeviceProxy.
-----------------------------------------------------------------------------*/

//
// We construct these proxy objects to support old render devices that
// were never ported to the new OldUnreal 469 interface
//
class ENGINE_API URenderDeviceProxy : public URenderDeviceOldUnreal469
{
	DECLARE_CLASS(URenderDeviceProxy, URenderDeviceOldUnreal469, CLASS_Transient, Engine)
	NO_DEFAULT_CONSTRUCTOR(URenderDeviceProxy)		

	// UObject interface
	URenderDeviceProxy(URenderDevice* InOrig);
	void Destroy();
	void Serialize(FArchive& Ar);

	// URenderDevice interface
	void Placeholder();
	UBOOL Init(UViewport* InViewport, INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen);
	UBOOL SetRes(INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen);
	void Exit();
	void Flush(UBOOL AllowPrecache);
	UBOOL Exec(const TCHAR* Cmd, FOutputDevice& Ar);
	void Lock(FPlane FlashScale, FPlane FlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* HitData, INT* HitSize);
	void Unlock(UBOOL Blit);
	void DrawComplexSurface(FSceneNode* Frame, FSurfaceInfo& Surface, FSurfaceFacet& Facet);
	void DrawGouraudPolygon(FSceneNode* Frame, FTextureInfo& Info, FTransTexture** Pts, int NumPts, DWORD PolyFlags, FSpanBuffer* Span) ;
	void DrawTile(FSceneNode* Frame, FTextureInfo& Info, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, class FSpanBuffer* Span, FLOAT Z, FPlane Color, FPlane Fog, DWORD PolyFlags);
	void Draw3DLine(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector OrigP, FVector OrigQ);
	void Draw2DClippedLine(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2);
	void Draw2DLine(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2);
	void Draw2DPoint(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FLOAT X1, FLOAT Y1, FLOAT X2, FLOAT Y2, FLOAT Z);
	void ClearZ(FSceneNode* Frame);
	void PushHit(const BYTE* Data, INT Count);
	void PopHit(INT Count, UBOOL bForce);
	void GetStats(TCHAR* Result);
	void ReadPixels(FColor* Pixels);
	void EndFlash();
	void DrawStats(FSceneNode* Frame);
	void SetSceneNode(FSceneNode* Frame);
	void PrecacheTexture(FTextureInfo& Info, DWORD PolyFlags);	
	UBOOL SupportsTextureFormat( ETextureFormat Format);

	URenderDevice* Orig{};
};

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (pop)
#endif

#endif
/*------------------------------------------------------------------------------------
	The End.
------------------------------------------------------------------------------------*/
