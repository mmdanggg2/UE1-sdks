/*=============================================================================
	UnNullDrivers.h: Unreal engine private header file.
=============================================================================*/
#pragma once

class UNullRenderDevice : public URenderDevice
{
	DECLARE_CLASS(UNullRenderDevice, URenderDevice, CLASS_Transient, Engine);
	NO_DEFAULT_CONSTRUCTOR(UNullRenderDevice);

	// URenderDevice interface.
	UBOOL Init(UViewport* InViewport, INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen);
	UBOOL SetRes(INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen);
	void Exit() {}
	void Flush(UBOOL AllowPrecache) {}
	UBOOL Exec(const TCHAR* Cmd, FOutputDevice& Ar) { return 0; }
	void Lock(FPlane FlashScale, FPlane FlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* HitData, INT* HitSize);
	void Unlock(UBOOL Blit) {}
	void DrawComplexSurface(FSceneNode* Frame, FSurfaceInfo& Surface, FSurfaceFacet& Facet) {}
	void DrawGouraudPolygon(FSceneNode* Frame, FTextureInfo& Info, FTransTexture** Pts, int NumPts, DWORD PolyFlags, FSpanBuffer* Span) {}
	void DrawTile(FSceneNode* Frame, FTextureInfo& Info, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, class FSpanBuffer* Span, FLOAT Z, FPlane Color, FPlane Fog, DWORD PolyFlags) {}
	void Draw3DLine(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector OrigP, FVector OrigQ) {}
	void Draw2DClippedLine(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2) {}
	void Draw2DLine(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2) {};
	void Draw2DPoint(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FLOAT X1, FLOAT Y1, FLOAT X2, FLOAT Y2, FLOAT Z) {};
	void ClearZ(FSceneNode* Frame) {};
	void PushHit(const BYTE* Data, INT Count) {};
	void PopHit(INT Count, UBOOL bForce) {};
	void GetStats(TCHAR* Result) {};
	void ReadPixels(FColor* Pixels, UBOOL bGammaCorrectOutput) {};
	void DrawGouraudPolyList(FSceneNode* Frame, FTextureInfo& Info, FTransTexture* Pts, int NumPts, DWORD PolyFlags, FSpanBuffer* Span = NULL) {};

private:
	unsigned long long XFrameCounter = 0;
	DOUBLE LastFrameLog = 0.0;
	UViewport* Viewport;
};

/*-----------------------------------------------------------------------------
	UNullConnection.
-----------------------------------------------------------------------------*/

//
// Windows socket class.
//
class UNullConnection : public UNetConnection
{
	DECLARE_CLASS(UNullConnection, UNetConnection, CLASS_Transient, Engine);
	NO_DEFAULT_CONSTRUCTOR(UNullConnection);

	void LowLevelSend(void* Data, INT Count) {}
	FString LowLevelGetRemoteAddress(UBOOL bIncludePort) { return TEXT("!!"); }
	FString LowLevelDescribe() { return TEXT("!!"); }
};

/*-----------------------------------------------------------------------------
	UNullNetDriver.
-----------------------------------------------------------------------------*/

class UNullNetDriver : public UNetDriver
{
	DECLARE_CLASS(UNullNetDriver, UNetDriver, CLASS_Transient, Engine);
	NO_DEFAULT_CONSTRUCTOR(UNullNetDriver);

	// UNetDriver interface.
	UBOOL InitConnect(FNetworkNotify* InNotify, FURL& ConnectURL, FString& Error) { return 1; }
	UBOOL InitListen(FNetworkNotify* InNotify, FURL& LocalURL, FString& Error) { return 1; }
	void TickDispatch(FLOAT DeltaTime) {}
	FString LowLevelGetNetworkNumber() { return TEXT("!!"); }
	void LowLevelDestroy() {}
};

//
// Null implementation of the client.
//
class UNullClient : public UClient, public FNotifyHook
{
	DECLARE_CLASS(UNullClient, UClient, CLASS_Transient, Engine);
	NO_DEFAULT_CONSTRUCTOR(UNullClient);

	// UObject interface.
	void ShutdownAfterError();

	// UClient interface.
	void ShowViewportWindows(DWORD ShowFlags, INT DoShow) {}
	void EnableViewportWindows(DWORD ShowFlags, INT DoEnable) {}
	void MakeCurrent(UViewport* InViewport) {}
	UViewport* NewViewport(const FName Name);
	void Tick() {}
};

class UNullViewport : public UViewport
{
	DECLARE_CLASS(UNullViewport, UViewport, CLASS_Transient, Engine);
	DECLARE_WITHIN(UNullClient);
	NO_DEFAULT_CONSTRUCTOR(UNullViewport);

	// UViewport interface.
	UBOOL Lock(FPlane FlashScale, FPlane FlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* HitData, INT* HitSize) { return TRUE; }
	UBOOL ResizeViewport(DWORD BlitFlags, INT NewX, INT NewY, INT NewColorBytes) { return TRUE; }
	UBOOL IsFullscreen() { return FALSE; }
	void SetModeCursor() {}
	void UpdateWindowFrame() {}
	void UpdateInput(UBOOL Reset) {}
	void Repaint(UBOOL Blit) {}
	void Unlock(UBOOL Blit) {}
	void OpenWindow(void* ParentWindow, UBOOL Temporary, INT NewX, INT NewY, INT OpenX, INT OpenY, const TCHAR* ForcedRenDevClass) {}
	void CloseWindow() {}
	void* GetWindow() { return nullptr; }
	void SetMouseCapture(UBOOL Capture, UBOOL Clip, UBOOL FocusOnly) {}
};
