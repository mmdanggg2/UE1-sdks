/*=============================================================================
	UnRenDev.h: 3D rendering device class.

	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
	Compiled with Visual C++ 4.0. Best viewed with Tabs=4.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

/*------------------------------------------------------------------------------------
	URenderDevice.
------------------------------------------------------------------------------------*/

// Flags for locking a rendering device.
enum ELockRenderFlags
{
	LOCKR_ClearScreen	= 1,
	LOCKR_LightDiminish = 2,
};

//
// A low-level 3D rendering device.
//
class ENGINE_API URenderDevice : public USubsystem
{
	DECLARE_ABSTRACT_CLASS(URenderDevice,USubsystem,CLASS_Config)

	// Variables.
	UViewport*		Viewport;
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
	BITFIELD		Pad1[8];
	DWORD			Pad0[8];

	// Constructors.
	void StaticConstructor();

	// URenderDevice low-level functions that drivers must implement.
	virtual UBOOL Init( UViewport* InViewport, INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen )=0;
	virtual UBOOL SetRes( INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen )=0;
	virtual void Exit()=0;
	virtual void Flush()=0;
	virtual UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar )=0;
	virtual void Lock( FPlane FlashScale, FPlane FlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* HitData, INT* HitSize )=0;
	virtual void Unlock( UBOOL Blit )=0;
	virtual void DrawComplexSurface( FSceneNode* Frame, FSurfaceInfo& Surface, FSurfaceFacet& Facet )=0;
	virtual void DrawGouraudPolygon( FSceneNode* Frame, FTextureInfo& Info, FTransTexture** Pts, int NumPts, DWORD PolyFlags, FSpanBuffer* Span )=0;
	virtual void DrawTile( FSceneNode* Frame, FTextureInfo& Info, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, class FSpanBuffer* Span, FLOAT Z, FPlane Color, FPlane Fog, DWORD PolyFlags )=0;
	virtual void Draw2DLine( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2 )=0;
	virtual void Draw2DPoint( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FLOAT X1, FLOAT Y1, FLOAT X2, FLOAT Y2, FLOAT Z )=0;
	virtual void ClearZ( FSceneNode* Frame )=0;
	virtual void PushHit( const BYTE* Data, INT Count )=0;
	virtual void PopHit( INT Count, UBOOL bForce )=0;
	virtual void GetStats( TCHAR* Result )=0;
	virtual void ReadPixels( FColor* Pixels )=0;
	virtual void EndFlash() {}
};

/*------------------------------------------------------------------------------------
	The End.
------------------------------------------------------------------------------------*/
