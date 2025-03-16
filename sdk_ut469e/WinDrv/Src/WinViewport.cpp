/*=============================================================================
	UnWnCam.cpp: UWindowsViewport code.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#include "WinDrv.h"
#include "netfw.h"

#define DD_OTHERLOCKFLAGS 0 /*DDLOCK_NOSYSLOCK*/ /*0x00000800L*/
#define WM_MOUSEWHEEL 0x020A
#define RI_MOUSE_BUTTONS 0x03FF
#define CLIP_CURSOR_SAFE_GAP 4

#ifndef CURSOR_SUPPRESSED
	#define CURSOR_SUPPRESSED 0x00000002
#endif

/*-----------------------------------------------------------------------------
	Class implementation.
-----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(UWindowsViewport);

/*-----------------------------------------------------------------------------
	UWindowsViewport Init/Exit.
-----------------------------------------------------------------------------*/

void UWindowsViewport::StaticConstructor()
{
	new(GetClass(), TEXT("ClickDetectionMovementThreshold"), RF_Public)UIntProperty(CPP_PROPERTY(ClickDetectionMovementThreshold), TEXT("Input"), CPF_Config);
	new(GetClass(), TEXT("ClickDetectionTimeThreshold"), RF_Public)UFloatProperty(CPP_PROPERTY(ClickDetectionTimeThreshold), TEXT("Input"), CPF_Config);
	ClickDetectionMovementThreshold = 15;
	ClickDetectionTimeThreshold = 0.25f;
}

//
// Constructor.
//
UWindowsViewport::UWindowsViewport()
:	UViewport()
,	Status( WIN_ViewportOpening )
{
	guard(UWindowsViewport::UWindowsViewport);
	Window = new WWindowsViewportWindow( this );

	// Set color bytes based on screen resolution.
	HWND hwndDesktop = GetDesktopWindow();
	HDC  hdcDesktop  = GetDC(hwndDesktop);
	switch( GetDeviceCaps( hdcDesktop, BITSPIXEL ) )
	{
		case 8:
			ColorBytes  = 2;
			break;
		case 16:
			ColorBytes  = 2;
			Caps       |= CC_RGB565;
			break;
		case 24:
			ColorBytes  = 4;
			break;
		case 32: 
			ColorBytes  = 4;
			break;
		default: 
			ColorBytes  = 2; 
			Caps       |= CC_RGB565;
			break;
	}
	DesktopColorBytes = ColorBytes;

	// Init other stuff.
	ReleaseDC( hwndDesktop, hdcDesktop );
	SavedCursor.x = -1;
	CapturingMouse = FALSE;

	StandardCursors[0] = LoadCursorW(NULL, IDC_ARROW);
	StandardCursors[1] = LoadCursorW(NULL, IDC_SIZEALL);
	StandardCursors[2] = LoadCursorW(NULL, IDC_SIZENESW);
	StandardCursors[3] = LoadCursorW(NULL, IDC_SIZENS);
	StandardCursors[4] = LoadCursorW(NULL, IDC_SIZENWSE);
	StandardCursors[5] = LoadCursorW(NULL, IDC_SIZEWE);
	StandardCursors[6] = LoadCursorW(NULL, IDC_WAIT);

	RepaintPending = TRUE;

	unguard;
}

//
// Destroy.
//
void UWindowsViewport::Destroy()
{
	guard(UWindowsViewport::Destroy);
	Super::Destroy();
	if( BlitFlags & BLIT_Temporary )
		appFree( ScreenPointer );
	Window->MaybeDestroy();
	delete Window;
	Window = nullptr;
	unguard;
}

//
// Error shutdown.
//
void UWindowsViewport::ShutdownAfterError()
{
	if( ddBackBuffer )
	{
		ddBackBuffer->Release();
		ddBackBuffer = NULL;
	}
	if( ddFrontBuffer )
	{
		ddFrontBuffer->Release();
		ddFrontBuffer = NULL;
	}
	if( Window->hWnd )
	{
		// stijn: added force minimize here because DestroyWindow does not immediately 
		// destroy the window. Instead, it posts a WM_DESTROY message to the game's 
		// message queue. The window remains visible until the WM_DESTROY message is 
		// dispatched.
		ShowWindow(Window->hWnd, SW_FORCEMINIMIZE);
		DestroyWindow(Window->hWnd);
	}
	Super::ShutdownAfterError();
}

/*-----------------------------------------------------------------------------
	Command line.
-----------------------------------------------------------------------------*/

static INT CommonResList[][2] = {
	{320, 240},
	{384, 288},
	{400, 300},
	{512, 384},
	{640, 360},
	{640, 480},
	{800, 600},
	{854, 480},
	{960, 540},
	{960, 720},
	{1024, 576},
	{1024, 768},
	{1138, 640},
	{1152, 864},
	{1280, 720},
	{1280, 800},
	{1280, 960},
	{1280, 1024},
	{1344, 750},
	{1366, 768},
	{1400, 1050},
	{1440, 1080},
	{1600, 900},
	{1600, 1200},
	{1600, 1280},
	{1680, 1050},
	{1792, 1344},
	{1800, 1400},
	{1856, 1392},
	{1920, 1080},
	{1920, 1200},
	{1920, 1280},
	{1920, 1440},
	{2048, 1152},
	{2048, 1280},
	{2048, 1536},
	{2560, 1440},
	{2560, 1600},
	{2560, 2048},
	{2576, 1450},
	{2800, 2100},
	{2880, 1620},
	{2880, 1800},
	{3200, 1800},
	{3200, 2048},
	{3200, 2400},
	{3840, 2160},
	{3840, 2400},
	{4096, 2304},
	{4480, 2520},
	{5120, 2880},
	{6016, 3384},
	{8192, 4608},
	{15360, 8640},
};

static QSORT_RETURN CDECL CompareRes(const FPlane* A, const FPlane* B) {
	return (QSORT_RETURN) (((A->X - B->X) != 0.0f) ? (A->X - B->X) : (A->Y - B->Y));
}

FString GetWinResList(UWindowsViewport* Viewport)
{
	guard(GetWinResList);
	INT MaxW = Max(CommonResList[0][0], GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CXVIRTUALSCREEN));
	INT MaxH = Max(CommonResList[0][1], GetSystemMetrics(SM_CYSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN));
	TArray<FPlane> ResList;
	ResList.Empty(ARRAY_COUNT(CommonResList));

	for (INT i = 0; i < ARRAY_COUNT(CommonResList); i++)
		if (CommonResList[i][0] <= MaxW && CommonResList[i][1] <= MaxH)
			ResList.AddItem(FPlane(CommonResList[i][0], CommonResList[i][1], 0, 0));

	if (Viewport->RenDev)
	{
		FStringOutputDevice StrOut;
		if (Viewport->RenDev->Exec(TEXT("GetRes"), StrOut))
		{
			FString RendResList = StrOut;
			while (RendResList.Len() > 2)
			{
				INT W = appAtoi(*RendResList);
				INT Pos = RendResList.InStr(TEXT("x"));
				INT H = appAtoi(*RendResList.Mid(Pos + 1));
				if (0 < W && W <= MaxW && 0 < H && H <= MaxH)
					ResList.AddUniqueItem(FPlane(W, H, 0, 0));
				Pos = RendResList.InStr(TEXT(" "));
				if (Pos < 0)
					break;
				RendResList = RendResList.Mid(Pos + 1);
			}
		}
	}
	
	if (Viewport->GetOuterUWindowsClient())
		for (INT i = 0; i < Viewport->GetOuterUWindowsClient()->DisplayModes.Num(); i++)
		{
			DDSURFACEDESC2& Desc = Viewport->GetOuterUWindowsClient()->DisplayModes(i);
			if (Desc.ddpfPixelFormat.dwRGBBitCount == Viewport->ColorBytes * 8 &&
				0 < Desc.dwWidth && Desc.dwWidth <= DWORD(MaxW) && 0 < Desc.dwHeight && Desc.dwHeight <= DWORD(MaxH))
				ResList.AddUniqueItem(FPlane(Desc.dwWidth, Desc.dwHeight, 0, 0));
		}

	appQsort(&ResList(0), ResList.Num(), sizeof(FPlane), (QSORT_COMPARE)CompareRes);
	FString Str;
	for (INT i = 0; i < ResList.Num(); i++) {
		Str += FString::Printf(TEXT("%ix%i "), (INT)ResList(i).X, (INT)ResList(i).Y);
	}
	return Str.LeftChop(1);
	unguard;
}

//
// Command line.
//
UBOOL UWindowsViewport::Exec( const TCHAR* Cmd, FOutputDevice& Ar )
{
	guard(UWindowsViewport::Exec);

	if (!IsFullscreen() && ParseCommand(&Cmd, TEXT("GetRes")))
	{
		Ar.Log(GetWinResList(this));
		return 1;
	}
	if( UViewport::Exec( Cmd, Ar ) )
	{
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("EndFullscreen")) )
	{
		EndFullscreen();
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("ToggleFullscreen")) )
	{
		if (!GIsEditor)
			ToggleFullscreen();
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("GetCurrentRes")) )
	{
		Ar.Logf( TEXT("%ix%i"), SizeX, SizeY );
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("GetCurrentColorDepth")) )
	{
		Ar.Logf( TEXT("%i"), (ColorBytes?ColorBytes:2)*8 );
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("GetColorDepths")) )
	{
		Ar.Log( TEXT("32") );
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("GetCurrentRenderDevice")) )
	{
		Ar.Log( *FObjectPathName(RenDev->GetClass()) );
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("GetRes")) )
	{
		if (BlitFlags & BLIT_DirectDraw)
		{
			// DirectDraw modes.
			FString Result;
			for (INT i = 0; i < GetOuterUWindowsClient()->DisplayModes.Num(); i++)
			{
				DDSURFACEDESC2& Desc = GetOuterUWindowsClient()->DisplayModes(i);
				if (Desc.ddpfPixelFormat.dwRGBBitCount == ColorBytes * 8)
				{
					Result += FString::Printf(TEXT("%ix%i "), Desc.dwWidth, Desc.dwHeight);
				}
			}
			if( Result.Right(1)==TEXT(" ") )
				Result = Result.LeftChop(1);
			Ar.Log( *Result );
		}
		else if( BlitFlags & BLIT_DibSection )
		{
			// Windowed mode.
			Ar.Log(GetWinResList(this));
		}
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("GetScreenMode")) )
	{
		if (IsFullscreen())
			Ar.Log(TEXT("Fullscreen"));
		else if (IsBorderless())
			Ar.Log(TEXT("Borderless"));
		else
			Ar.Log(TEXT("Windowed"));
 		return 1;
 	}
	else if (ParseCommand(&Cmd, TEXT("SetScreenMode")))
	{
#define MODE_UNSET 0
#define MODE_WINDOWED 1
#define MODE_BORDERLESS 2
#define MODE_FULLSCREEN 3
		INT Mode = MODE_UNSET;
		if (appStricmp(Cmd, TEXT("Fullscreen")) == 0)
			Mode = MODE_FULLSCREEN;
		else if (appStricmp(Cmd, TEXT("Windowed")) == 0)
			Mode = MODE_WINDOWED;
		else if (appStricmp(Cmd, TEXT("Borderless")) == 0)
			Mode = MODE_BORDERLESS;
		if (Mode != MODE_UNSET)
		{
			if (IsFullscreen() != (Mode == MODE_FULLSCREEN))
			{
				if (Mode == MODE_FULLSCREEN)
					ToggleFullscreen();
				else
					EndFullscreen();
			}
			if (Mode == MODE_BORDERLESS || (Mode == MODE_WINDOWED && IsBorderless()))
			{
				INT Width = SizeX;
				INT Height = SizeY;
				GetBorderlessSize(Width, Height);
				if (Mode == MODE_WINDOWED)
				{
					Width -= 100;
					Height -= 100;
				}

				HoldCount++;
				RenDev->SetRes(Width, Height, ColorBytes, IsFullscreen());
				HoldCount--;
			}
			GetOuterUWindowsClient()->StartupFullscreen = Mode == MODE_FULLSCREEN;
			GetOuterUWindowsClient()->StartupBorderless = Mode == MODE_BORDERLESS;
			GetOuterUWindowsClient()->SaveConfig();
		}
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("SetRes")) )
	{
		INT X=appAtoi(Cmd);
		const TCHAR* CmdTemp = appStrchr(Cmd,'x') ? appStrchr(Cmd,'x')+1 : appStrchr(Cmd,'X') ? appStrchr(Cmd,'X')+1 : TEXT("");
		INT Y=appAtoi(CmdTemp);
		Cmd = CmdTemp;
		CmdTemp = appStrchr(Cmd,'x') ? appStrchr(Cmd,'x')+1 : appStrchr(Cmd,'X') ? appStrchr(Cmd,'X')+1 : TEXT("");
		INT C=appAtoi(CmdTemp);
		INT NewColorBytes = C ? C/8 : ColorBytes;
		if( X && Y )
		{
			HoldCount++;
			UBOOL Result = RenDev->SetRes( X, Y, NewColorBytes, IsFullscreen() );
			HoldCount--;
			if( !Result )
				EndFullscreen();
		}
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("Preferences")) )
	{
		UWindowsClient* Client = GetOuterUWindowsClient();
		Client->ConfigReturnFullscreen = 0;
		if( BlitFlags & BLIT_Fullscreen )
		{
			EndFullscreen();
			Client->ConfigReturnFullscreen = 1;
		}
		if( !Client->ConfigProperties )
		{
			Client->ConfigProperties = new WConfigProperties( TEXT("Preferences"), LocalizeGeneral(TEXT("AdvancedOptionsTitle"), TEXT("Window")) );
			Client->ConfigProperties->SetNotifyHook( Client );
			Client->ConfigProperties->OpenWindow( Window->hWnd );
			Client->ConfigProperties->ForceRefresh();
		}
		GetOuterUWindowsClient()->ConfigProperties->Show(1);
		SetFocus( *GetOuterUWindowsClient()->ConfigProperties );
		return 1;
	}
	else if (ParseCommand(&Cmd, TEXT("SAVESCREENPOS")))
	{
		if (!GIsEditor && !IsFullscreen() && !ParentWindow && Window && Window->hWnd)
		{
			FRect wr = Window->GetWindowRect();
			UWindowsClient* C = GetOuterUWindowsClient();
			if (C && wr.Min.X != -32000 && wr.Min.Y != -32000 && 
				wr.Min.X != wr.Max.X && wr.Min.Y != wr.Max.Y &&
				((C->WinPosX != wr.Min.X) || (C->WinPosY != wr.Min.Y)))
			{
				C->WinPosX = wr.Min.X;
				C->WinPosY = wr.Min.Y;
				C->SaveConfig();
			}
		}
		return 1;
	}
	else return 0;
	unguard;
}

/*-----------------------------------------------------------------------------
	Window openining and closing.
-----------------------------------------------------------------------------*/

//
// A temporary RenderDevice to satisfy light rebuilding needs.
//
class UTemporaryRenderDevice : public URenderDeviceOldUnreal469
{
	DECLARE_CLASS(UTemporaryRenderDevice, URenderDeviceOldUnreal469, 0)

	// URenderDevice interface.
	UBOOL Init(UViewport* InViewport, INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen) { return 1; }
	UBOOL SetRes(INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen) { return 1; }
	void Exit() {}
	void Flush(UBOOL AllowPrecache) {}
	void Flush() {}
	UBOOL Exec(const TCHAR* Cmd, FOutputDevice& Ar) { return 0; }
	void Lock(FPlane FlashScale, FPlane FlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* HitData, INT* HitSize) {}
	void Unlock(UBOOL Blit) {}
	void DrawComplexSurface(FSceneNode* Frame, FSurfaceInfo& Surface, FSurfaceFacet& Facet) {}
	void DrawGouraudPolygon(FSceneNode* Frame, FTextureInfo& Info, FTransTexture** Pts, int NumPts, DWORD PolyFlags, FSpanBuffer* Span) {}
	void DrawTile(FSceneNode* Frame, FTextureInfo& Info, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, class FSpanBuffer* Span, FLOAT Z, FPlane Color, FPlane Fog, DWORD PolyFlags) {}
	void Draw3DLine(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector OrigP, FVector OrigQ) {}
	void Draw2DClippedLine(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2) {}
	void Draw2DLine(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2) {}
	void Draw2DPoint(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FLOAT X1, FLOAT Y1, FLOAT X2, FLOAT Y2, FLOAT Z) {}
	void ClearZ(FSceneNode* Frame) {}
	void PushHit(const BYTE* Data, INT Count) {}
	void PopHit(INT Count, UBOOL bForce) {}
	void GetStats(TCHAR* Result) {}
	void ReadPixels(FColor* Pixels) {}
	void EndFlash() {}
	void DrawStats(FSceneNode* Frame) {}
	void SetSceneNode(FSceneNode* Frame) {}
	void PrecacheTexture(FTextureInfo& Info, DWORD PolyFlags) {}

	DWORD BlitFlags(UBOOL Fullscreen) { return 0; }
};
IMPLEMENT_CLASS(UTemporaryRenderDevice);

//
// Open this viewport's window.
//
void UWindowsViewport::OpenWindow( void* InParentWindow, UBOOL IsTemporary, INT NewX, INT NewY, INT OpenX, INT OpenY, const TCHAR* ForcedRenDevClass )
{
	guard(UWindowsViewport::OpenWindow);
	check(Actor);
	check(!HoldCount);
	UBOOL DoRepaint=0, DoSetActive=0;
	UWindowsClient* C = GetOuterUWindowsClient();
	if( NewX!=INDEX_NONE )
		NewX = Align( NewX, 2 );

	// User window of launcher if no parent window was specified.
	if( !InParentWindow )
		Parse( appCmdLine(), TEXT("HWND="), reinterpret_cast<PTRINT&>(InParentWindow) );

	// Create frame buffer.
	if( IsTemporary )
	{
		// Create in-memory data.
		BlitFlags     = BLIT_Temporary;
		ColorBytes    = 2;
		SizeX         = NewX;
		SizeY         = NewY;
		ScreenPointer = (BYTE*)appMalloc( 2 * NewX * NewY, TEXT("TemporaryViewportData") );	
		Window->hWnd  = NULL;
		debugf( NAME_Log, TEXT("Opened temporary viewport") );
   	}
	else
	{
		// Figure out physical window size we must specify to get appropriate client area.
		FRect rTemp( 100, 100, (NewX!=INDEX_NONE?NewX:C->WindowedViewportX) + 100, (NewY!=INDEX_NONE?NewY:C->WindowedViewportY) + 100 );

		// Get style and proper rectangle.
		DWORD Style;
		if( InParentWindow && (Actor->ShowFlags & SHOW_ChildWindow) )
		{
			Style = WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS;
   			AdjustWindowRect( rTemp, Style, 0 );
		}
		else
		{
			Style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME;
   			AdjustWindowRect( rTemp, Style, (Actor->ShowFlags & SHOW_Menu) ? TRUE : FALSE );

			if (OpenX == -1)
			{
				OpenX = C->WinPosX;
				OpenY = C->WinPosY;
			}
		}

		// Set position and size.
		if( OpenX==-1 )
			OpenX = CW_USEDEFAULT;
		if( OpenY==-1 )
			OpenY = CW_USEDEFAULT;
		INT OpenXL = rTemp.Width();
		INT OpenYL = rTemp.Height();

		// Create or update the window.
		if( !Window->hWnd )
		{
			// Creating new viewport.
			ParentWindow = (HWND)InParentWindow;

			// Open the physical window.
			Window->PerformCreateWindowEx
			(
				WS_EX_APPWINDOW,
				TEXT(""),
				Style,
				OpenX, OpenY,
				OpenXL, OpenYL,
				ParentWindow,
				NULL,
				hInstance
			);

			// Set parent window.
			if( ParentWindow && (Actor->ShowFlags & SHOW_ChildWindow) )
			{
				// Force this to be a child.
				SetWindowLongPtrW(Window->hWnd, GWL_STYLE, WS_VISIBLE | WS_POPUP);
				SetWindowPos(Window->hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED); // Regarding MSDN: If you have changed certain window data using SetWindowLong, you must call SetWindowPos for the changes to take effect. Use the following combination for uFlags: SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED.
			}
			debugf( NAME_Log, TEXT("Opened viewport") );
			DoSetActive = DoRepaint = 1;

			GetOuterUWindowsClient()->MouseInputHandler->RegisterViewport(this, FALSE);
		}
		else
		{
			// Resizing existing viewport.
			//!!only needed for old vb code
			SetWindowPos( Window->hWnd, NULL, OpenX, OpenY, OpenXL, OpenYL, SWP_NOACTIVATE );
		}
		ShowWindow( Window->hWnd, SW_SHOWNOACTIVATE );
		if( DoRepaint )
			UpdateWindow( Window->hWnd );
	}

	DOUBLE Seconds = appSecondsNew();

	// Create rendering device.	
	if (!RenDev && (BlitFlags & BLIT_Temporary))
	{
		// stijn: called without TryRenderDevice because we don't need to call init or even give the rendev a viewport
		UClass* RenDevClass = LoadClass<URenderDeviceOldUnreal469>(NULL, TEXT("WinDrv.TemporaryRenderDevice"), NULL, LOAD_NoFail, NULL);
		RenDev = ConstructObject<URenderDeviceOldUnreal469>(RenDevClass, this);
		check(RenDev);
	}
	if (!RenDev && GIsEditor && ForcedRenDevClass && ForcedRenDevClass[0])
		TryRenderDevice(ForcedRenDevClass, NewX, NewY, INDEX_NONE, 0);
	if (!RenDev && !GIsEditor && !ParseParam(appCmdLine(), TEXT("nohard")))
		TryRenderDevice(TEXT("ini:Engine.Engine.GameRenderDevice"), NewX, NewY, INDEX_NONE, C->StartupFullscreen);
	if (!RenDev && !GIsEditor && GetOuterUWindowsClient()->StartupFullscreen)
		TryRenderDevice(TEXT("ini:Engine.Engine.WindowedRenderDevice"), NewX, NewY, INDEX_NONE, 1);
	if (!RenDev && !GIsEditor && GetOuterUWindowsClient()->StartupBorderless)
	{
		GetBorderlessSize(NewX, NewY);
		TryRenderDevice(TEXT("ini:Engine.Engine.WindowedRenderDevice"), NewX, NewY, INDEX_NONE, 0);
	}
	if (!RenDev)
		TryRenderDevice(TEXT("ini:Engine.Engine.WindowedRenderDevice"), NewX, NewY, INDEX_NONE, 0);
	check(RenDev);
	UpdateWindowFrame();
	if( DoRepaint )
		Repaint( 1 );
	if (DoSetActive && !GIsEditor)
	{
		if (!IsFullscreen() && !ParentWindow)
		{
			RECT desktop, winsz;
			GetWindowRect(GetDesktopWindow(), &desktop);
			GetWindowRect(Window->hWnd, &winsz);

			if (OpenX == CW_USEDEFAULT)
			{
				// Marco: center window to desktop on first boot.
				SetWindowPos(Window->hWnd, HWND_TOP, (desktop.right - (winsz.right - winsz.left)) >> 1, (desktop.bottom - (winsz.bottom - winsz.top)) >> 1, 0, 0, (SWP_NOACTIVATE | SWP_NOSIZE));
			}
			else
			{
				// Make sure it fits within screen bounds.
				int x = Clamp<int>(winsz.left, 0, desktop.right - (winsz.right - winsz.left));
				int y = Clamp<int>(winsz.top, 0, desktop.bottom - (winsz.bottom - winsz.top));
				if (x != winsz.left || y != winsz.top)
					SetWindowPos(Window->hWnd, HWND_TOP, x, y, 0, 0, (SWP_NOACTIVATE | SWP_NOSIZE));
			}
		}
		SetForegroundWindow(Window->hWnd);
	}

	UpdateMouseClippingRegion();

	debugf(TEXT("Render device created in %f seconds"), appSecondsNew() - Seconds);

	unguard;
}

//
// Close a viewport window.  Assumes that the viewport has been openened with
// OpenViewportWindow.  Does not affect the viewport's object, only the
// platform-specific information associated with it.
//
void UWindowsViewport::CloseWindow()
{
	guard(UWindowsViewport::CloseWindow);
	if( Window->hWnd && Status==WIN_ViewportNormal )
	{
		Status = WIN_ViewportClosing;
		DestroyWindow( Window->hWnd );
	}
	unguard;
}

/*-----------------------------------------------------------------------------
	UWindowsViewport operations.
-----------------------------------------------------------------------------*/

//
// Set window position according to menu's on-top setting:
//
void UWindowsViewport::SetTopness()
{
	guard(UWindowsViewport::SetTopness);

	//
	// stijn: It looks like UT originally had an editorcam/playercam menu with an "Always on top" setting
	// The menu is gone now, so the check below always failed and just set Topness to HWND_NOTTOPMOST.
	// This broke exclusive fullscreen support in OpenGLDrv/XOpenGLDrv
	//

	// HWND Topness = HWND_NOTOPMOST;
	// if( GetMenu(Window->hWnd) && GetMenuState(GetMenu(Window->hWnd),ID_ViewTop,MF_BYCOMMAND)&MF_CHECKED )
	//	Topness = HWND_TOPMOST;

	const HWND Topness = HWND_TOPMOST;
	SetWindowPos( Window->hWnd, Topness, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOACTIVATE );
	unguard;
}

//
// Repaint the viewport.
//
void UWindowsViewport::Repaint( UBOOL Blit )
{
	guard(UWindowsViewport::Repaint);
	RepaintPending = FALSE;
	GetOuterUWindowsClient()->Engine->Draw( this, Blit );	
	unguard;
}

//
// Return whether fullscreen.
//
UBOOL UWindowsViewport::IsFullscreen()
{
	guard(UWindowsViewport::IsFullscreen);
	return (BlitFlags & BLIT_Fullscreen)!=0;
	unguard;
}

//
// Return whether borderless.
//
UBOOL UWindowsViewport::IsBorderless()
{
	guard(UWindowsViewport::IsBorderless);
	if (IsFullscreen())
		return FALSE;
	INT Width = SizeX;
	INT Height = SizeY;
	GetBorderlessSize(Width, Height);
	return SizeX == Width && SizeY == Height;
	unguard;
}

//
// Return borderless viewport size (matching to whole of current screen).
//
void UWindowsViewport::GetBorderlessSize(INT& Width, INT& Height)
{
	guard(UWindowsViewport::GetBorderlessSize);
	HDC ScreenDC = GetDC(NULL);
	Width = GetDeviceCaps(ScreenDC, HORZRES);
	Height = GetDeviceCaps(ScreenDC, VERTRES);
	ReleaseDC(0, ScreenDC);
	unguard;
}

//
// Set the mouse cursor according to Unreal or UnrealEd's mode, or to
// an hourglass if a slow task is active.
//
void UWindowsViewport::SetModeCursor()
{
	guard(UWindowsViewport::SetModeCursor);
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
	if( GIsSlowTask )
	{
		SetCursor(LoadCursorW(NULL,IDC_WAIT));
		return;
	}
	HCURSOR hCursor;
	switch( GetOuterUWindowsClient()->Engine->edcamMode(this) )
	{
		case EM_ViewportZoom:	hCursor = LoadCursorW(hInstance, MAKEINTRESOURCEW(IDCURSOR_CameraZoom)); break;
		case EM_BrushRotate:	hCursor = LoadCursorW(hInstance, MAKEINTRESOURCEW(IDCURSOR_BrushRot)); break;
		case EM_BrushSheer:		hCursor = LoadCursorW(hInstance, MAKEINTRESOURCEW(IDCURSOR_BrushSheer)); break;
		case EM_BrushScale:		hCursor = LoadCursorW(hInstance, MAKEINTRESOURCEW(IDCURSOR_BrushScale)); break;
		case EM_BrushStretch:	hCursor = LoadCursorW(hInstance, MAKEINTRESOURCEW(IDCURSOR_BrushStretch)); break;
		case EM_BrushSnap:		hCursor = LoadCursorW(hInstance, MAKEINTRESOURCEW(IDCURSOR_BrushSnap)); break;
		case EM_TexturePan:		hCursor = LoadCursorW(hInstance, MAKEINTRESOURCEW(IDCURSOR_TexPan)); break;
		case EM_TextureRotate:	hCursor = LoadCursorW(hInstance, MAKEINTRESOURCEW(IDCURSOR_TexRot)); break;
		case EM_TextureScale:	hCursor = LoadCursorW(hInstance, MAKEINTRESOURCEW(IDCURSOR_TexScale)); break;
		case EM_None: 			hCursor = LoadCursorW(NULL,IDC_CROSS); break;
		case EM_ViewportMove: 	hCursor = LoadCursorW(NULL,IDC_CROSS); break;
		case EM_TexView:		hCursor = LoadCursorW(NULL,IDC_ARROW); break;
		case EM_TexBrowser:		hCursor = LoadCursorW(NULL,IDC_ARROW); break;
		case EM_MeshView:		hCursor = LoadCursorW(NULL,IDC_CROSS); break;
		case EM_VertexEdit: 	hCursor = LoadCursorW(hInstance, MAKEINTRESOURCEW(IDCURSOR_VertexEdit)); break;
		case EM_BrushClip:		hCursor = LoadCursorW(hInstance, MAKEINTRESOURCEW(IDCURSOR_BrushClip)); break;
		case EM_FaceDrag:		hCursor = LoadCursorW(hInstance, MAKEINTRESOURCEW(IDCURSOR_FaceDrag)); break;
		default: 				hCursor = LoadCursorW(NULL,IDC_ARROW); break;
	}
	check(hCursor);
	SetCursor( hCursor );
	unguard;
}

void UWindowsViewport::MoveWindow(int Left, int Top, int Width, int Height, UBOOL bRepaint)
{
	guard(UWindowsViewport::MoveWindow);
	if (Window)
		Window->MoveWindow(Left, Top, Width, Height, bRepaint);
	UpdateMouseClippingRegion();
	unguard;
}

void ApplySafeGap(RECT& TempRect)
{
	TempRect.left += CLIP_CURSOR_SAFE_GAP;
	TempRect.top += CLIP_CURSOR_SAFE_GAP;
	TempRect.right -= CLIP_CURSOR_SAFE_GAP;
	TempRect.bottom -= CLIP_CURSOR_SAFE_GAP;
}

void UWindowsViewport::UpdateMouseClippingRegion() const
{
	if (ClippingMouse)
	{
		RECT TempRect;
		::GetClientRect(Window->hWnd, &TempRect);
		ApplySafeGap(TempRect);
		MapWindowPoints(Window->hWnd, NULL, (POINT*)&TempRect, 2);
		ClipCursor(&TempRect);

		debugfSlow(NAME_DevInput, TEXT("Updated mouse clipping region - [Left: %d, Top: %d, Right: %d, Bottom: %d]"),
			TempRect.left, TempRect.top, TempRect.right, TempRect.bottom);
	}
}


//
// Update user viewport interface.
//
void UWindowsViewport::UpdateWindowFrame()
{
	guard(UWindowsViewport::UpdateWindowFrame);

	// If not a window, exit.
	if( HoldCount || !Window->hWnd || (BlitFlags&BLIT_Fullscreen) || (BlitFlags&BLIT_Temporary) )
		return;

	// Set viewport window's name to show resolution.
	FString WindowName;
	if( !GIsEditor || (Actor->ShowFlags&SHOW_PlayerCtrl) )
	{
		WindowName = LocalizeGeneral(TEXT("Product"), appPackage());
	}
	else switch( Actor->RendMap )
	{
		case REN_Wire:		WindowName = LocalizeGeneral(TEXT("ViewPersp")); break;
		case REN_OrthXY:	WindowName = LocalizeGeneral(TEXT("ViewXY")   ); break;
		case REN_OrthXZ:	WindowName = LocalizeGeneral(TEXT("ViewXZ")   ); break;
		case REN_OrthYZ:	WindowName = LocalizeGeneral(TEXT("ViewYZ")   ); break;
		default:			WindowName = LocalizeGeneral(TEXT("ViewOther")); break;
	}
	Window->SetText( *WindowName );

	// Update parent window.
	if( ParentWindow )
	{
		SendMessageW( ParentWindow, WM_CHAR, 0, 0 );
		PostMessageW( ParentWindow, WM_COMMAND, WM_VIEWPORT_UPDATEWINDOWFRAME, 0 );
	}

	unguard;
}

//
// Return the viewport's window.
//
void* UWindowsViewport::GetWindow()
{
	guard(UWindowsViewport::GetWindow);
	return Window->hWnd;
	unguard;
}

//
// Controls cursor visibility
//
void UWindowsViewport::SetCursorDisplay( UBOOL bShow)
{
	CURSORINFO Info;
	Info.cbSize = sizeof(Info);
	GetCursorInfo( &Info);

	// UT on a touchpad system?
	if ( Info.flags & CURSOR_SUPPRESSED )
		return;

	// Do not change cursor display if not needed
	if ( ((Info.flags & CURSOR_SHOWING) != 0) == (bShow != 0) )
		return;

	if ( bShow )
	{
		while ( ShowCursor(TRUE) < 0 );
	}
	else
	{
		while ( ShowCursor(FALSE) >= 0 );
	}
}

/*-----------------------------------------------------------------------------
	Input.
-----------------------------------------------------------------------------*/

// Stores the active modifier keys for each key code.
// This allows us to ignore IST_Release if we release the modifier keys before
// the modified key.
static BYTE IgnoreKeyTable[IK_MAX] = {};
static UBOOL bIgnoreInput = FALSE;

//
// Input event router.
//
UBOOL UWindowsViewport::CauseInputEvent( INT iKey, EInputAction Action, FLOAT Delta )
{
	guard(UWindowsViewport::CauseInputEvent);
	if (bIgnoreInput)
		return 0;

	// Route to engine if the key is valid; some keyboards produce key
	// codes that go beyond IK_MAX.
	if (iKey >= 0 && iKey < IK_MAX)
	{
		if (GIsEditor && (Action == IST_Press || Action == IST_Release))
		{
			// Here we prevent calling engine input bind commands if any modifiers pressed.
			// See https://github.com/OldUnreal/UnrealTournamentPatches/issues/1588
			// This also prevents bind commands from running when we press LMB or RMB.
			UBOOL bRunBindCommands = TRUE;
			
			if (iKey == IK_LeftMouse || iKey == IK_RightMouse || iKey == IK_MouseWheelUp || iKey == IK_MouseWheelDown)
			{
				bRunBindCommands = FALSE;
			}
			else if (Action == IST_Press)
			{
				const UBOOL ModifierActive = 
					(GetAsyncKeyState(VK_SHIFT) & 0x8000) |
					(GetAsyncKeyState(VK_CONTROL) & 0x8000) |
					(GetAsyncKeyState(VK_MENU) & 0x8000); // Alt

				IgnoreKeyTable[iKey] = ModifierActive; 

				if (ModifierActive)
					bRunBindCommands = FALSE;
			}
			else if (Action == IST_Release)
			{
				if (IgnoreKeyTable[iKey])
				{
					IgnoreKeyTable[iKey] = 0;
					bRunBindCommands = FALSE;
				}
			}

			if (!bRunBindCommands)
				return Input->PreProcess((EInputKey)iKey, Action, Delta);
		}
		return GetOuterUWindowsClient()->Engine->InputEvent( this, (EInputKey)iKey, Action, Delta );
	}
	return 0;

	unguard;
}

//
// If the cursor is currently being captured, stop capturing, clipping, and 
// hiding it, and move its position back to where it was when it was initially
// captured.
//
void UWindowsViewport::SetMouseCapture( UBOOL Capture, UBOOL Clip, UBOOL OnlyFocus )
{
	guard(UWindowsViewport::SetMouseCapture);

	bWindowsMouseAvailable = !Capture;

	// If only handling focus windows, exit out.
	if( OnlyFocus )
		if( Window->hWnd != GetFocus() )
			return;

	// If capturing, windows requires clipping in order to keep focus.
	Clip |= Capture;

	// Get window rectangle.
	RECT TempRect;
	::GetClientRect(Window->hWnd, &TempRect);
	ApplySafeGap(TempRect);
	MapWindowPoints(Window->hWnd, NULL, (POINT*)&TempRect, 2);

	// Handle capturing.
	if (Capture)
	{
		if (!CapturingMouse)
		{
			// Remember where we were when we started capturing
			GetCursorPos(&SavedCursor);

			// Bring window to foreground.
			SetForegroundWindow(Window->hWnd);

			// Start capturing cursor.
			SetCapture(Window->hWnd);
			SetCursorDisplay(0);			
		}

		GetOuterUWindowsClient()->MouseInputHandler->AcquireMouse(this);
		CapturingMouse = TRUE;
	}
	else
	{
		if (CapturingMouse)
		{
			// Restore position.
			SetCursorPos(SavedCursor.x, SavedCursor.y);
		}
		
		// Buggie: this hack ensures we can keep dragging the mouse in EM_TexView
		// viewports until we capture the mouse elsewhere. We need this for things
		// like fire textures, where we use the mouse to paint line sparks, for
		// example.
		const INT EM_TexView			= 19;	// Viewing textures.
		if ((!GIsEditor || GetOuterUWindowsClient()->Engine->edcamMode(this) != EM_TexView || CapturingMouse) && GetOuterUWindowsClient()->MouseInputHandler)
			GetOuterUWindowsClient()->MouseInputHandler->ReleaseMouse(this);

		CapturingMouse = FALSE;

		// Release captured cursor.
		SavedCursor.x = -1;
		ReleaseCapture();
		SetCursorDisplay(1);
	}

	// Handle clipping.
	if (DO_GUARD_SLOW)
	{
		if (Clip)
			debugfSlow(NAME_DevInput, TEXT("CaptureMouse set clipping region - [Left: %d, Top: %d, Right: %d, Bottom: %d]"),
				TempRect.left, TempRect.top, TempRect.right, TempRect.bottom);
		else
			debugfSlow(NAME_DevInput, TEXT("CaptureMouse disabled clipping region"));
	}

	ClipCursor(Clip ? &TempRect : NULL);
	ClippingMouse = Clip;

	unguard;
}

//
// Update input for viewport.
//
UBOOL UWindowsViewport::JoystickInputEvent( FLOAT Delta, EInputKey Key, FLOAT Scale, UBOOL DeadZone )
{
	guard(UWindowsViewport::JoystickInputEvent);
	Delta = (Delta-32768.0)/32768.0;
	if( DeadZone )
	{
		if( Delta > 0.2 )
			Delta = (Delta - 0.2) / 0.8;
		else if( Delta < -0.2 )
			Delta = (Delta + 0.2) / 0.8;
		else
			Delta = 0.0;
	}
	return CauseInputEvent( Key, IST_Axis, Scale * Delta );
	unguard;
}

//
// Update input for this viewport.
//
void UWindowsViewport::UpdateInput( UBOOL Reset )
{
	guard(UWindowsViewport::UpdateInput);
	BYTE Processed[256];
	appMemset( Processed, 0, 256 );
	//debugf(TEXT("%i"),(INT)GTempDouble);

	// Joystick.
	UWindowsClient* Client = GetOuterUWindowsClient();
	if( Client->JoyCaps.wNumButtons )
	{
		JOYINFOEX JoyInfo;
		appMemzero( &JoyInfo, sizeof(JoyInfo) );
		JoyInfo.dwSize  = sizeof(JoyInfo);
		JoyInfo.dwFlags = JOY_RETURNBUTTONS | JOY_RETURNCENTERED | JOY_RETURNPOV | JOY_RETURNR | JOY_RETURNU | JOY_RETURNV | JOY_RETURNX | JOY_RETURNY | JOY_RETURNZ;
		MMRESULT Result = joyGetPosEx( JOYSTICKID1, &JoyInfo );
		if( Result==JOYERR_NOERROR )
		{ 
			// Pass buttons to app.
			for( INT Index=0; Index<16; Index++,JoyInfo.dwButtons/=2 )
			{
			   if( !Input->KeyDown(IK_Joy1+Index) && (JoyInfo.dwButtons & 1) )
				  CauseInputEvent( IK_Joy1+Index, IST_Press );
			   else if( Input->KeyDown(IK_Joy1+Index) && !(JoyInfo.dwButtons & 1) )
				  CauseInputEvent( IK_Joy1+Index, IST_Release );
				Processed[IK_Joy1+Index] = 1;
			}

			// Pass axes to app.
			JoystickInputEvent( JoyInfo.dwXpos, IK_JoyX, Client->ScaleXYZ, Client->DeadZoneXYZ );
			JoystickInputEvent( JoyInfo.dwYpos, IK_JoyY, Client->ScaleXYZ * (Client->InvertVertical ? 1.0 : -1.0), Client->DeadZoneXYZ );
			if( Client->JoyCaps.wCaps & JOYCAPS_HASZ )
				JoystickInputEvent( JoyInfo.dwZpos, IK_JoyZ, Client->ScaleXYZ, Client->DeadZoneXYZ );
			if( Client->JoyCaps.wCaps & JOYCAPS_HASR )
				JoystickInputEvent( JoyInfo.dwRpos, IK_JoyR, Client->ScaleRUV, Client->DeadZoneRUV );
			if( Client->JoyCaps.wCaps & JOYCAPS_HASU )
				JoystickInputEvent( JoyInfo.dwUpos, IK_JoyU, Client->ScaleRUV, Client->DeadZoneRUV );
			if( Client->JoyCaps.wCaps & JOYCAPS_HASV )
				JoystickInputEvent( JoyInfo.dwVpos, IK_JoyV, Client->ScaleRUV * (Client->InvertVertical ? 1.0 : -1.0), Client->DeadZoneRUV );
			if( Client->JoyCaps.wCaps & (JOYCAPS_POV4DIR|JOYCAPS_POVCTS) )
			{
				if( JoyInfo.dwPOV==JOY_POVFORWARD )
				{
					if( !Input->KeyDown(IK_JoyPovUp) )
						CauseInputEvent( IK_JoyPovUp, IST_Press );
					Processed[IK_JoyPovUp] = 1;
				}
				else if( JoyInfo.dwPOV==JOY_POVBACKWARD )
				{
					if( !Input->KeyDown(IK_JoyPovDown) )
						CauseInputEvent( IK_JoyPovDown, IST_Press );
					Processed[IK_JoyPovDown] = 1;
				}
				else if( JoyInfo.dwPOV==JOY_POVLEFT )
				{
					if( !Input->KeyDown(IK_JoyPovLeft) )
						CauseInputEvent( IK_JoyPovLeft, IST_Press );
					Processed[IK_JoyPovLeft] = 1;
				}
				else if( JoyInfo.dwPOV==JOY_POVRIGHT )
				{
					if( !Input->KeyDown(IK_JoyPovRight) )
						CauseInputEvent( IK_JoyPovRight, IST_Press );
					Processed[IK_JoyPovRight] = 1;
				}
			}
		}
	}

	// Process mouse input only for the focussed viewport
	if (GetFocus() == Window->hWnd)
	{
		FWindowsMouseInputHandler* InputHandler = GetOuterUWindowsClient()->MouseInputHandler;
		InputHandler->PollInputs(this);
		InputHandler->ProcessInputUpdates(this);
		if (Reset)
			InputHandler->ResetMouseState();
	}
	
	Processed[IK_LeftMouse] = 1;
	Processed[IK_RightMouse] = 1;
	Processed[IK_MiddleMouse] = 1;
	Processed[IK_MouseButton4] = 1;
	Processed[IK_MouseButton5] = 1;
	Processed[IK_MouseButton6] = 1;
	Processed[IK_MouseButton7] = 1;
	Processed[IK_MouseButton8] = 1;

	// Keyboard.
	Reset = Reset && GetFocus()==Window->hWnd;
	for( INT i=0; i<256; i++ )
	{
		if( !Processed[i] )
		{
			if( !Input->KeyDown(i) )
			{
				//old: if( Reset && (GetAsyncKeyState(i) & 0x8000) )
				if( Reset && (GetKeyState(i) & 0x8000) )
					CauseInputEvent( i, IST_Press );
			}
			else
			{
				//old: if( !(GetAsyncKeyState(i) & 0x8000) )
				if( !(GetKeyState(i) & 0x8000) )
					CauseInputEvent( i, IST_Release );
			}
		}
	}

	unguard;
}

/*-----------------------------------------------------------------------------
	Viewport WndProc.
-----------------------------------------------------------------------------*/

//
// Main viewport window function.
//
LRESULT UWindowsViewport::ViewportWndProc( UINT iMessage, WPARAM wParam, LPARAM lParam )
{
	guard(UWindowsViewport::ViewportWndProc);
	UWindowsClient* Client = GetOuterUWindowsClient();
	if( HoldCount || Client->Viewports.FindItemIndex(this)==INDEX_NONE || !Actor )
		return DefWindowProcW( Window->hWnd, iMessage, wParam, lParam );

	// Updates.
	if( iMessage==WindowMessageMouseWheel )
	{
		iMessage = WM_MOUSEWHEEL;
		wParam   = MAKEWPARAM(0,wParam);
	}

	//debugf(TEXT("WinMsg: %ls (%08x)"), GetWindowMessageString(iMessage), iMessage);

	// Message handler.
	switch( iMessage )
	{
		case WM_CREATE:
		{
			guard(WM_CREATE);

         	// Set status.
			Status = WIN_ViewportNormal; 

			// Make this viewport current and update its title bar.
			GetOuterUClient()->MakeCurrent( this );

			// Disable IME (input method editor) for Japanese Windows
			ImmAssociateContext( Window->hWnd, NULL );

			return 0;
			unguard;
		}
		case WM_HOTKEY:
		{
			return 0;
		}
		case WM_CLOSE:
		{
			guard(WM_CLOSE);
			debugf(TEXT("UWindowsViewport::ViewportWndProc: Received WM_CLOSE"));
			if (!GIsEditor)
			{
				appRequestExit(0);
				return 0;
			}
			unguard;
		}
		case WM_DESTROY:
		{
			guard(WM_DESTROY);
			debugf(TEXT("UWindowsViewport::ViewportWndProc: Received WM_DESTROY"));

			// If there's an existing Viewport structure corresponding to
			// this window, deactivate it.
			if( BlitFlags & BLIT_Fullscreen )
				EndFullscreen();
			else Exec(TEXT("SAVESCREENPOS"));

			// Free DIB section stuff (if any).
			if( hBitmap )
				DeleteObject( hBitmap );

			// Restore focus to caller if desired.
			PTRINT ParentWindow=0;
			Parse( appCmdLine(), TEXT("HWND="), ParentWindow );
			if( ParentWindow )
			{
				::SetParent( Window->hWnd, NULL );
				SetFocus( (HWND)ParentWindow );
			}

			// Stop clipping.
			SetDrag( 0 );
			if( Status==WIN_ViewportNormal )
			{
				// Closed by user clicking on window's close button, so delete the viewport.
				Status = WIN_ViewportClosing; // Prevent recursion.
				if (GIsEditor)
					delete this;
				else
					appRequestExit(0);
			}
			debugf( NAME_Log, TEXT("Closed viewport") );
			return 0;
			unguard;
		}
		case WM_PAINT:
		{
			guard(WM_PAINT);
			if( BlitFlags & (BLIT_Fullscreen|BLIT_Direct3D|BLIT_HardwarePaint) )
			{
				if( BlitFlags & BLIT_HardwarePaint )
					Repaint( 1 );
				ValidateRect( Window->hWnd, NULL );
				UpdateMouseClippingRegion();
				return 0;
			}
			else if( IsWindowVisible(Window->hWnd) && SizeX && SizeY && hBitmap )
			{
				PAINTSTRUCT ps;
				BeginPaint( Window->hWnd, &ps );
				HDC hDC = GetDC( Window->hWnd );
				if( hDC == NULL )
					appErrorf( TEXT("GetDC failed: %ls"), appGetSystemErrorMessage() );
				if( SelectObject( Client->hMemScreenDC, hBitmap ) == NULL )
					appErrorf( TEXT("SelectObject failed: %ls"), appGetSystemErrorMessage() );
				// Make failure not fatal. This can be caused by active lock screen.
				// See https://github.com/OldUnreal/UnrealTournamentPatches/issues/1076
				if( BitBlt( hDC, 0, 0, SizeX, SizeY, Client->hMemScreenDC, 0, 0, SRCCOPY ) == NULL )
				{
					static DOUBLE LastTime = 0;
					if (appSecondsNew() - LastTime >= 60)
					{
						LastTime = appSecondsNew();
						debugf( TEXT("BitBlt failed at %d: %ls"), __LINE__, appGetSystemErrorMessage() );
					}
				}
				if( ReleaseDC( Window->hWnd, hDC ) == NULL )
					appErrorf( TEXT("ReleaseDC failed: %ls"), appGetSystemErrorMessage() );
				EndPaint( Window->hWnd, &ps );
				return 0;
			}
			else return 1;
			unguard;
		}
		case WM_COMMAND:
		{
			guard(WM_COMMAND);

			if (GIsEditor)
			{
				HWND hwndEditorFrame = GetAncestor(ParentWindow, GA_ROOT);
				PostMessageW(hwndEditorFrame, WM_COMMAND, wParam, lParam); // Pass through to Editor for accelerators.
				UpdateWindowFrame();			
			}

      		switch( LOWORD(wParam) )
			{
				case ID_MapDynLight:
				{
					Actor->RendMap=REN_DynLight;
					break;
				}
				case ID_MapPlainTex:
				{
					Actor->RendMap=REN_PlainTex;
					break;
				}
				case ID_MapWire:
				{
					Actor->RendMap=REN_Wire;
					break;
				}
				case ID_MapOverhead:
				{
					Actor->RendMap=REN_OrthXY;
					break;
				}
				case ID_MapXZ:
				{
					Actor->RendMap=REN_OrthXZ;
					break;
				}
				case ID_MapYZ:
				{
					Actor->RendMap=REN_OrthYZ;
					break;
				}
				case ID_MapPolys:
				{
					Actor->RendMap=REN_Polys;
					break;
				}
				case ID_MapPolyCuts:
				{
					Actor->RendMap=REN_PolyCuts;
					break;
				}
				case ID_MapZones:
				{
					Actor->RendMap=REN_Zones;
					break;
				}
				case ID_Win320:
				{
					RenDev->SetRes( 320, 240, ColorBytes, 0 );
					break;
				}
				case ID_Win400:
				{
					RenDev->SetRes( 400, 300, ColorBytes, 0 );
					break;
				}
				case ID_Win512:
				{
					RenDev->SetRes( 512, 384, ColorBytes, 0 );
					break;
				}
				case ID_Win640:
				{
					RenDev->SetRes( 640, 480, ColorBytes, 0 );
					break;
				}
				case ID_Win800:
				{
					RenDev->SetRes( 800, 600, ColorBytes, 0 );
					break;
				}
				case ID_Color16Bit:
				{
					RenDev->SetRes( SizeX, SizeY, 2, 0 );
					Repaint( 1 );
					break;
				}
				case ID_Color32Bit:
				{
					RenDev->SetRes( SizeX, SizeY, 4, 0 );
					Repaint( 1 );
					break;
				}
				case ID_ShowBackdrop:
				{
					Actor->ShowFlags ^= SHOW_Backdrop;
					break;
				}
				case ID_ActorsShow:
				{
					Actor->ShowFlags &= ~(SHOW_Actors | SHOW_ActorIcons | SHOW_ActorRadii);
					Actor->ShowFlags |= SHOW_Actors; 
					break;
				}
				case ID_ActorsIcons:
				{
					Actor->ShowFlags &= ~(SHOW_Actors | SHOW_ActorIcons | SHOW_ActorRadii); 
					Actor->ShowFlags |= SHOW_Actors | SHOW_ActorIcons;
					break;
				}
				case ID_ActorsRadii:
				{
					Actor->ShowFlags &= ~(SHOW_Actors | SHOW_ActorIcons | SHOW_ActorRadii); 
					Actor->ShowFlags |= SHOW_Actors | SHOW_ActorRadii;
					break;
				}
				case ID_ActorsHide:
				{
					Actor->ShowFlags &= ~(SHOW_Actors | SHOW_ActorIcons | SHOW_ActorRadii); 
					break;
				}
				case ID_ShowPaths:
				{
					Actor->ShowFlags ^= SHOW_Paths;
					break;
				}
				case ID_ShowCoords:
				{
					Actor->ShowFlags ^= SHOW_Coords;
					break;
				}
				case ID_ShowTextureGrid:
				{
					Actor->ShowFlags ^= SHOW_TextureGrid;
					break;
				}
				case ID_ShowBrush:
				{
					Actor->ShowFlags ^= SHOW_Brush;
					break;
				}
				case ID_ShowMovingBrushes:
				{
					Actor->ShowFlags ^= SHOW_MovingBrushes;
					break;
				}
				case ID_ShowMoverSurfaces:
				{
					Actor->ShowFlags ^= SHOW_MoverSurfaces;
					break;
				}
				case ID_ShowMoverPath:
				{
					Actor->ShowFlags ^= SHOW_MoverPath;
					break;
				}
				case ID_ShowEventLines:
				{
					Actor->ShowFlags ^= SHOW_EventLines;
					break;
				}
				case ID_OccludeLines:
				{
					Actor->ShowFlags ^= SHOW_OccludeLines;
					break;
				}
				case ID_ViewLog:
				{
					Exec( TEXT("SHOWLOG"), *this );
					break;
				}
				case ID_FileExit:
				{
					DestroyWindow( Window->hWnd );
					return 0;
				}
				case ID_ViewTop:
				{
					ToggleMenuItem(GetMenu(Window->hWnd),ID_ViewTop);
					SetTopness();
					break;
				}
				case ID_ViewAdvanced:
				{
					Exec( TEXT("Preferences"), *GLog );
					break;
				}
				default:
				{
					if (wParam >= ID_DDMode0 && wParam <= ID_DDMode9)
					{
						WPARAM Mode = 0;
						for (INT i = 0; i < Client->DisplayModes.Num(); ++i)
						{
							DDSURFACEDESC2& Desc = Client->DisplayModes(i);

							if (Desc.ddpfPixelFormat.dwRGBBitCount == ColorBytes * 8)
							{
								if (Mode++ == wParam - ID_DDMode0)
								{
									ResizeViewport(BLIT_Fullscreen | BLIT_DirectDraw,
										Desc.dwWidth, Desc.dwHeight);
								}
							}
						}
					}
					break;
				}
			}
			RepaintPending = TRUE;
			UpdateWindowFrame();
			return 0;
			unguard;
		}
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			guard(WM_KEYDOWN);

			// Get key code.
			EInputKey Key = (EInputKey)wParam;

			// Send key to input system.
			if( Key==IK_Enter && (GetKeyState(VK_MENU)&0x8000) )
			{
				ToggleFullscreen();
			}
			else if (Key == IK_F4 && (GetKeyState(VK_MENU) & 0x8000))
			{
				appRequestExit(0);
			}
			else if( CauseInputEvent( Key, IST_Press ) )
			{	
				// Redraw if the viewport won't be redrawn on timer.
				RepaintPending = TRUE;
			}

			// Set the cursor.
			if( GIsEditor )
				SetModeCursor();

			return 0;
			unguard;
		}
		case WM_KEYUP:
		case WM_SYSKEYUP:
		{
			guard(WM_KEYUP);

			// Send to the input system.
			EInputKey Key = (EInputKey)wParam;
			if( CauseInputEvent( Key, IST_Release ) )
			{	
				// Redraw if the viewport won't be redrawn on timer.
				RepaintPending = TRUE;
			}

			// Pass keystroke on to UnrealEd.
			if( ParentWindow && GIsEditor )
			{				
				if( Key == IK_F1 )
					PostMessageW( ParentWindow, iMessage, IK_F2, lParam );
				else if( Key!=IK_Tab && Key!=IK_Enter && Key!=IK_Alt )
					PostMessageW( ParentWindow, iMessage, wParam, lParam );
			}
			if( GIsEditor )
				SetModeCursor();
			return 0;
			unguard;
		}
		case WM_SYSCHAR:
		case WM_CHAR:
		{
			guard(WM_CHAR);
			EInputKey Key = (EInputKey)wParam;
			if( Key!=IK_Enter && Client->Engine->Key( this, Key ) )
			{
				// Redraw if needed.
				RepaintPending = TRUE;
				
				if( GIsEditor )
					SetModeCursor();
			}
			else if( iMessage == WM_SYSCHAR )
			{
				// Perform default processing.
				return DefWindowProcW( Window->hWnd, iMessage, wParam, lParam );
			}
			return 0;
			unguard;
		}
		case WM_ERASEBKGND:
		{
			// Prevent Windows from repainting client background in white.
			return 0;
		}
		case WM_SETCURSOR:
		{
			guard(WM_SETCURSOR);
			if( (LOWORD(lParam)==1) || GIsSlowTask )
			{
				// In client area or processing slow task.
				if( GIsEditor )
					SetModeCursor();
				return 0;
			}
			else
			{
				// Out of client area.
				return DefWindowProcW( Window->hWnd, iMessage, wParam, lParam );
			}
			unguard;
		}
		case WM_LBUTTONDBLCLK:
		{
			if( SizeX && SizeY && !(BlitFlags&BLIT_Fullscreen) )
			{
				Client->Engine->Click( this, MOUSE_LeftDouble, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) );
				RepaintPending = TRUE;
			}
			return 0;
		}
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_XBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		case WM_XBUTTONUP:
		case WM_MOUSEWHEEL:
		case WM_INPUT:
		case WM_MOUSEMOVE:
		{
			if (Client->InMenuLoop)
				return DefWindowProcW(Window->hWnd, iMessage, wParam, lParam);
			LRESULT Result = Client->MouseInputHandler->ProcessInputEvent(this, iMessage, wParam, lParam);
			Client->MouseInputHandler->ProcessInputUpdates(this);
			return Result;
		}
		
		case WM_MOUSEACTIVATE:
		{
			IgnoreMouseWheel=1; //elmuerte: EM_EXEC workaround
			// Activate this window and send the mouse-down message.
			return MA_ACTIVATE;
		}
		case WM_ACTIVATE:
		{
			guard(WM_ACTIVATE);

			// If window is becoming inactive, release the cursor.
			if (wParam == 0)
			{
				SetDrag(0);
				SetMouseCapture(FALSE, FALSE);
			}

			return 0;
			unguard;
		}
		case WM_ENTERMENULOOP:
		{
			guard(WM_ENTERMENULOOP);
			Client->InMenuLoop = 1;
			SetDrag( 0 );
			UpdateWindowFrame();
			return 0;
			unguard;
		}
		case WM_EXITMENULOOP:
		{
			guard(WM_EXITMENULOOP);
			Client->InMenuLoop = 0;
			return 0;
			unguard;
		}
		case WM_CANCELMODE:
		{
			guard(WM_CANCELMODE);
			SetDrag( 0 );
			return 0;
			unguard;
		}
		case WM_SIZE:
		{
			guard(WM_SIZE);
			INT NewX = LOWORD(lParam);
			INT NewY = HIWORD(lParam);
			if( BlitFlags & BLIT_Fullscreen )
			{
				// Window forced out of fullscreen.
				if( wParam==SIZE_RESTORED )
				{
					HoldCount++;
					Window->MoveWindow( SavedWindowRect, 1 );
					HoldCount--;
				}
				return 0;
			}
			else if( wParam==SIZE_RESTORED && DirectDrawMinimized )
			{
				DirectDrawMinimized = 0;
				ToggleFullscreen();
				return 0;
			}
			else
			{
				// stijn: DXGI (used by D3D10/D3D11) can get us here when pressing ALT+ENTER
				// We should ignore the resolution update because WinDrv thinks we're still
				// in windowed mode when processing this WM_SIZE message
				if ((GetKeyState(VK_MENU) & 0x8000) && (GetKeyState(VK_RETURN) & 0x8000))
					IgnoreResolutionConfigChange = TRUE;
				
				// Use resized window.				
				if( RenDev && (BlitFlags & (BLIT_OpenGL|BLIT_Direct3D)) )
				{
					RenDev->SetRes( NewX, NewY, ColorBytes, 0 );
				}
				else
				{
					ResizeViewport( BlitFlags|BLIT_NoWindowChange, NewX, NewY, ColorBytes );
				}
				if( GIsEditor )
					Repaint( 0 );
				
				IgnoreResolutionConfigChange = FALSE;
      			return 0;
        	}
			unguard;
		}
		case WM_KILLFOCUS:
		{
			guard(WM_KILLFOCUS);
			SetMouseCapture( 0, 0, 0 );
			SetDrag( 0 );
			Input->ResetInput();
			if( BlitFlags & BLIT_Fullscreen )
			{
				EndFullscreen();
				HoldCount++;
				ShowWindow( Window->hWnd, SW_SHOWMINNOACTIVE );
				HoldCount--;
				DirectDrawMinimized = 1;
			}

			GetOuterUClient()->MakeCurrent( NULL );
			return 0;
			unguard;
		}
		case WM_SETFOCUS:
		{
			guard(WM_SETFOCUS);

			Input->ResetInput();

			// Make this viewport current.
			GetOuterUClient()->MakeCurrent( this );
			if (GIsEditor)
				SetModeCursor();

			// Disable IME (input method editor) for Japanese Windows
			ImmAssociateContext( Window->hWnd, NULL );

            return 0;
			unguard;
		}
		case WM_SYSCOMMAND:
		{
			guard(WM_SYSCOMMAND);
			DWORD nID = wParam & 0xFFF0;
			if( nID==SC_SCREENSAVE || nID==SC_MONITORPOWER )
			{
				// Return 1 to prevent screen saver.
				if( nID==SC_SCREENSAVE )
					debugf( NAME_Log, TEXT("Received SC_SCREENSAVE") );
				else
					debugf( NAME_Log, TEXT("Received SC_MONITORPOWER") );
				return 0;
			}
			else if( nID==SC_MAXIMIZE )
			{
				// Maximize.
				ToggleFullscreen();
				return 0;
			}
			else if
			(	(BlitFlags & BLIT_Fullscreen)
			&&	(nID==SC_NEXTWINDOW || nID==SC_PREVWINDOW || nID==SC_TASKLIST || nID==SC_HOTKEY) )
			{
				// Don't allow window changes here.
				return 0;
			}
			else return DefWindowProcW(Window->hWnd,iMessage,wParam,lParam);
			unguard;
		}
		case WM_POWER:
		{
			guard(WM_POWER);
			if( wParam )
			{
				if( wParam == PWR_SUSPENDREQUEST )
				{
					debugf( NAME_Log, TEXT("Received WM_POWER suspend") );

					// Prevent powerdown if dedicated server or using joystick.
					if( 1 )
						return PWR_FAIL;
					else
						return PWR_OK;
				}
				else
				{
					debugf( NAME_Log, TEXT("Received WM_POWER") );
					return DefWindowProcW( Window->hWnd, iMessage, wParam, lParam );
				}
			}
			return 0;
			unguard;
		}
		case WM_DISPLAYCHANGE:
		{
			guard(WM_DISPLAYCHANGE);
			INT NewX = LOWORD(lParam);
			INT NewY = HIWORD(lParam);
			debugf(NAME_Log, TEXT("Viewport %ls: WM_DisplayChange %dx%dx%d"), GetName(), NewX, NewY, wParam);
			if (!(BlitFlags & BLIT_Fullscreen))
			{
				// Just notify viewport for make borderless <-> windowed transition if need.
				ResizeViewport(BlitFlags | BLIT_NoWindowChange);
				if (GIsEditor)
					Repaint(0);
        	}
			unguard;
			return 0;
		}
		case WM_WININICHANGE:
		{
			guard(WM_WININICHANGE);
			if (!DeleteDC(Client->hMemScreenDC))
				appErrorf(TEXT("DeleteDC failed: %ls"), appGetSystemErrorMessage());
			Client->hMemScreenDC = CreateCompatibleDC(NULL);
			return 0;
			unguard;
		}
		default:
		{
			guard(WM_UNKNOWN);
			return DefWindowProcW( Window->hWnd, iMessage, wParam, lParam );
			unguard;
		}
	}
	return 0;
	unguard;
}
W_IMPLEMENT_CLASS(WWindowsViewportWindow)

/*-----------------------------------------------------------------------------
	DirectDraw support.
-----------------------------------------------------------------------------*/

//
// Set DirectDraw to a particular mode, with full error checking
// Returns 1 if success, 0 if failure.
//
UBOOL UWindowsViewport::ddSetMode( INT NewX, INT NewY, INT ColorBytes )
{
	guard(UWindowsViewport::ddSetMode);
	UWindowsClient* Client = GetOuterUWindowsClient();
	check(Client->dd);
	HRESULT	Result;

	// Set the display mode.
	debugf( NAME_Log, TEXT("Setting %ix%ix%i"), NewX, NewY, ColorBytes*8 );
	Result = Client->dd->SetDisplayMode( NewX, NewY, ColorBytes*8, 0, 0 );
	if( Result!=DD_OK )
	{
		debugf( NAME_Log, TEXT("DirectDraw Failed %ix%ix%i: %ls"), NewX, NewY, ColorBytes*8, ddError(Result) );
		Result = Client->dd->SetCooperativeLevel( NULL, DDSCL_NORMAL );
   		return 0;
	}

	// Create surfaces.
	DDSURFACEDESC2 SurfaceDesc{};
	SurfaceDesc.dwSize = sizeof(DDSURFACEDESC2);
	SurfaceDesc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
	SurfaceDesc.ddsCaps.dwCaps
		= DDSCAPS_PRIMARYSURFACE
		| DDSCAPS_FLIP
		| DDSCAPS_COMPLEX
		//|	(Client->SlowVideoBuffering ? DDSCAPS_SYSTEMMEMORY : DDSCAPS_VIDEOMEMORY); // stijn: SYSTEMMEMORY no longer works in Windows 2000 or beyond. LOL!
		| DDSCAPS_VIDEOMEMORY;
	

	// Create the best possible surface for rendering.
	const TCHAR* Descr=NULL;
	if( 1 )
	{
		// Try triple-buffered video memory surface.
		SurfaceDesc.dwBackBufferCount = 2;
		Result = Client->dd->CreateSurface( &SurfaceDesc, &ddFrontBuffer, NULL );
		Descr  = TEXT("Triple buffer");
	}
	if( Result != DD_OK )
   	{
		// Try to get a double buffered video memory surface.
		SurfaceDesc.dwBackBufferCount = 1; 
		Result = Client->dd->CreateSurface( &SurfaceDesc, &ddFrontBuffer, NULL );
		Descr  = TEXT("Double buffer");
    }
	if( Result != DD_OK )
	{
		// Settle for a main memory surface.
		SurfaceDesc.ddsCaps.dwCaps &= ~DDSCAPS_VIDEOMEMORY;
		Result = Client->dd->CreateSurface( &SurfaceDesc, &ddFrontBuffer, NULL );
		Descr  = TEXT("System memory");
    }
	if( Result != DD_OK )
	{
		debugf( NAME_Log, TEXT("DirectDraw, no available modes %ls"), ddError(Result) );
		Client->dd->RestoreDisplayMode();
		Client->dd->FlipToGDISurface();
	   	return 0;
	}
	debugf( NAME_Log, TEXT("DirectDraw: %ls, %ix%i, Stride=%i"), Descr, NewX, NewY, SurfaceDesc.lPitch );
	debugf( NAME_Log, TEXT("DirectDraw: Rate=%i"), SurfaceDesc.dwRefreshRate );

	// Clear the screen.
	DDBLTFX ddbltfx;
	ddbltfx.dwSize = sizeof( ddbltfx );
	ddbltfx.dwFillColor = 0;
	ddFrontBuffer->Blt( NULL, NULL, NULL, DDBLT_COLORFILL, &ddbltfx );

	// Get a pointer to the back buffer.
	DDSCAPS2 caps{};
	caps.dwCaps = DDSCAPS_BACKBUFFER;
	if( ddFrontBuffer->GetAttachedSurface( &caps, &ddBackBuffer )!=DD_OK )
	{
		debugf( NAME_Log, TEXT("DirectDraw GetAttachedSurface failed %ls"), ddError(Result) );
		ddFrontBuffer->Release();
		ddFrontBuffer = NULL;
		Client->dd->RestoreDisplayMode();
		Client->dd->FlipToGDISurface();
		return 0;
	}

	// Get pixel format.
	DDPIXELFORMAT PixelFormat;
	PixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	Result = ddFrontBuffer->GetPixelFormat( &PixelFormat );
	if( Result!=DD_OK )
	{
		debugf( TEXT("DirectDraw GetPixelFormat failed: %ls"), ddError(Result) );
		ddBackBuffer->Release();
		ddBackBuffer = NULL;
		ddFrontBuffer->Release();
		ddFrontBuffer = NULL;
		Client->dd->RestoreDisplayMode();
		Client->dd->FlipToGDISurface();
		return 0;
	}

	// See if we're in a 16-bit color mode.
	Caps &= ~CC_RGB565;
	if( ColorBytes==2 && PixelFormat.dwRBitMask==0xf800 ) 
		Caps |= CC_RGB565;

	// Flush the cache.
	GCache.Flush();

	// Success.
	return 1;
	unguard;
}

/*-----------------------------------------------------------------------------
	Lock and Unlock.
-----------------------------------------------------------------------------*/

//
// Lock the viewport window and set the approprite Screen and RealScreen fields
// of Viewport.  Returns 1 if locked successfully, 0 if failed.  Note that a
// lock failing is not a critical error; it's a sign that a DirectDraw mode
// has ended or the user has closed a viewport window.
//
UBOOL UWindowsViewport::Lock( FPlane FlashScale, FPlane FlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* HitData, INT* HitSize )
{
	guard(UWindowsViewport::LockWindow);
	UWindowsClient* Client = GetOuterUWindowsClient();
	clockFast(Client->DrawCycles);

	// Make sure window is lockable.
	if( (Window->hWnd && !IsWindow(Window->hWnd)) || HoldCount || !SizeX || !SizeY || !RenDev )
      	return 0;

	// Get info.
	Stride = SizeX;
	if( BlitFlags & BLIT_DirectDraw )
	{
		// Lock DirectDraw.
		check(!(BlitFlags&BLIT_DibSection));
		HRESULT Result;
  		if( ddFrontBuffer->IsLost() == DDERR_SURFACELOST )
		{
			Result = ddFrontBuffer->Restore();
   			if( Result != DD_OK )
			{
				debugf( NAME_Log, TEXT("DirectDraw Lock Restore failed %ls"), ddError(Result) );
				ResizeViewport( BLIT_DibSection );//!!failure of d3d?
				return 0;
			}
		}
		appMemzero( &ddSurfaceDesc, sizeof(ddSurfaceDesc) );
  		ddSurfaceDesc.dwSize = sizeof(ddSurfaceDesc);
		Result = ddBackBuffer->Lock( NULL, &ddSurfaceDesc, DDLOCK_WAIT|DD_OTHERLOCKFLAGS, NULL );
  		if( Result != DD_OK )
		{
			debugf( NAME_Log, TEXT("DirectDraw Lock failed: %ls"), ddError(Result) );
  			return 0;
		}
		if( ddSurfaceDesc.lPitch )
			Stride = ddSurfaceDesc.lPitch/ColorBytes;
		ScreenPointer = (BYTE*)ddSurfaceDesc.lpSurface;
		check(ScreenPointer);
	}
	else if( BlitFlags & BLIT_DibSection )
	{
		check(!(BlitFlags&BLIT_DirectDraw));
		check(ScreenPointer);
	}

	// Success here, so pass to superclass.
	unclockFast(Client->DrawCycles);
	return UViewport::Lock(FlashScale,FlashFog,ScreenClear,RenderLockFlags,HitData,HitSize);

	unguard;
}

//
// Unlock the viewport window.  If Blit=1, blits the viewport's frame buffer.
//
void UWindowsViewport::Unlock( UBOOL Blit )
{
	guard(UWindowsViewport::Unlock);
	UWindowsClient* Client = GetOuterUWindowsClient();
	check(!HoldCount);
	Client->DrawCycles=0;
	clockFast(Client->DrawCycles);
	UViewport::Unlock( Blit );
	if( BlitFlags & BLIT_DirectDraw )
	{
		// Handle DirectDraw.
		guard(UnlockDirectDraw);
		HRESULT Result;
		Result = ddBackBuffer->Unlock( NULL );
		if( Result ) 
		 	appErrorf( TEXT("DirectDraw Unlock: %ls"), ddError(Result) );
		if( Blit )
		{
			HRESULT Result = ddFrontBuffer->Flip( NULL, DDFLIP_WAIT );
			if( Result != DD_OK )
				appErrorf( TEXT("DirectDraw Flip failed: %ls"), ddError(Result) );
		}
		unguard;
	}
	else if( BlitFlags & BLIT_DibSection )
	{
		// Handle CreateDIBSection.
		if( Blit )
		{
			HDC hDC = GetDC( Window->hWnd );
			if( hDC == NULL )
				appErrorf( TEXT("GetDC failed: %ls"), appGetSystemErrorMessage() );
			if( SelectObject( Client->hMemScreenDC, hBitmap ) == NULL )
				appErrorf( TEXT("SelectObject failed: %ls"), appGetSystemErrorMessage() );
			// Make failure not fatal. This can be caused by active lock screen.
			// See https://github.com/OldUnreal/UnrealTournamentPatches/issues/1076
			if( BitBlt( hDC, 0, 0, SizeX, SizeY, Client->hMemScreenDC, 0, 0, SRCCOPY ) == NULL )
			{
				static DOUBLE LastTime = 0;
				if (appSecondsNew() - LastTime >= 60)
				{
					LastTime = appSecondsNew();
					debugf( TEXT("BitBlt failed at %d: %ls"), __LINE__, appGetSystemErrorMessage() );
				}
			}
			if( ReleaseDC( Window->hWnd, hDC ) == NULL )
				appErrorf( TEXT("ReleaseDC failed: %ls"), appGetSystemErrorMessage() );
		}
	}
	unclockFast(Client->DrawCycles);
	unguard;
}

/*-----------------------------------------------------------------------------
	Viewport modes.
-----------------------------------------------------------------------------*/

static void RecreateWindow(UWindowsViewport* Viewport)
{
	guard(RecreateWindow);
	if (GIsEditor || !Viewport->Window)
		return;
	HWND OldHwnd = Viewport->Window->hWnd;
	if (!OldHwnd)
		return;

	DWORD Style = GetWindowLongW(OldHwnd, GWL_STYLE);
	RECT Rect;
	if (!GetWindowRect(OldHwnd, &Rect))
	{
		Rect.left = Rect.top = 0;
		Rect.right = Viewport->SizeX;
		Rect.bottom = Viewport->SizeY;
	}
	Viewport->Window->hWnd = NULL;
	// Buggie: Prevent handle input again inside PerformCreateWindowEx.
	bIgnoreInput = TRUE;
	Viewport->Window->PerformCreateWindowEx
	(
		WS_EX_APPWINDOW,
		TEXT(""),
		Style,
		Rect.left, Rect.top,
		Rect.right - Rect.left, Rect.bottom - Rect.top,
		0,
		NULL,
		hInstance
	);
	bIgnoreInput = FALSE;
	if (Viewport->Window->hWnd && Viewport->Window->hWnd != OldHwnd)
	{
		DestroyWindow(OldHwnd);
		Viewport->GetOuterUWindowsClient()->MouseInputHandler->RegisterViewport(Viewport, FALSE);
	}
	else
		Viewport->Window->hWnd = OldHwnd;

	unguard;
}

//
// Try switching to a new rendering device.
//
void UWindowsViewport::TryRenderDevice( const TCHAR* ClassName, INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen )
{
	guard(UWindowsViewport::TryRenderDevice);

	// Metallicafan212:	If the cursor is captured, release it first.
	//					This fixes issues going to or from DX9 when using a DX11 renderer.
	UBOOL bCaptured = CapturingMouse;

	if (bCaptured)
	{
		SetMouseCapture(0, 0);
	}

	UClass* RenderClass = UObject::StaticLoadClass(URenderDevice::StaticClass(), NULL, ClassName, NULL, 0, NULL);

	// Shut down current rendering device.
	// stijn: We only do this if the new rendev is different
	if( RenDev && (!RenderClass || !RenDev->IsA(RenderClass)) )
	{
		RenDev->Exit();
		delete RenDev;
		RenDev = NULL;

		RecreateWindow(this);
	}

	// Use appropriate defaults.
	UWindowsClient* C = GetOuterUWindowsClient();
	if( NewX==INDEX_NONE )
		NewX = Fullscreen ? C->FullscreenViewportX : C->WindowedViewportX;
	if( NewY==INDEX_NONE )
		NewY = Fullscreen ? C->FullscreenViewportY : C->WindowedViewportY;
	if( NewColorBytes==INDEX_NONE )
		NewColorBytes = Fullscreen ? C->FullscreenColorBits/8 : ColorBytes;

	// Find device driver.	
	if( RenderClass )
	{
		HoldCount++;

		if (!RenDev)
		{
			URenderDevice* TempRenDev = ConstructObject<URenderDevice>(RenderClass, this);

			if (!TempRenDev)
			{
				RenDev = nullptr;
			}
			else
			{
				if (!Cast<URenderDeviceOldUnreal469>(TempRenDev))
				{
					RenDev = new URenderDeviceProxy(TempRenDev);
					debugf(NAME_Log, TEXT("%ls is a legacy render device."), *FObjectPathName(TempRenDev));
				}
				else
				{
					RenDev = Cast<URenderDeviceOldUnreal469>(TempRenDev);
				}
			}

			if (RenDev && RenDev->Init(this, NewX, NewY, NewColorBytes, Fullscreen))
			{
				if (GIsRunning)
					Actor->GetLevel()->DetailChange(RenDev->HighDetailActors);
			}
			else
			{
				debugf(NAME_Log, LocalizeError("Failed3D"));
				delete RenDev;
				RenDev = NULL;
			}
		}
		else
		{
			// stijn: We already have the correct renderer. just resize it then...
			if (RenDev->SetRes(NewX, NewY, NewColorBytes, Fullscreen))
			{
				if (GIsRunning)
					Actor->GetLevel()->DetailChange(RenDev->HighDetailActors);
			}
			else
			{
				debugf(NAME_Log, LocalizeError("Failed3D"));
				delete RenDev;
				RenDev = NULL;
			}
		}		
		
		HoldCount--;
	}

	GRenderDevice = RenDev;

	// Metallicafan212:	Resetup mouse capture, if it was captured (executing a bind for instance).
	if (bCaptured)
	{
		SetMouseCapture(1, 1);
	}

	unguard;
}

//
// If in fullscreen mode, end it and return to Windows.
//
void UWindowsViewport::EndFullscreen()
{
	guard(UWindowsViewport::EndFullscreen);
	UWindowsClient* Client = GetOuterUWindowsClient();
	debugf(TEXT("EndFullscreen"));
	if( RenDev && RenDev->FullscreenOnly )
	{
		// This device doesn't support fullscreen, so use a window-capable rendering device.
		TryRenderDevice( TEXT("ini:Engine.Engine.WindowedRenderDevice"), INDEX_NONE, INDEX_NONE, INDEX_NONE, 0 );
		check(RenDev);
	}
	else if( RenDev && (BlitFlags & (BLIT_Direct3D|BLIT_OpenGL)))
	{
		RenDev->SetRes( Client->WindowedViewportX, Client->WindowedViewportY, ColorBytes, 0 );
	}
	else
	{
		ResizeViewport( BLIT_DibSection );
	}
	UpdateWindowFrame();
	UpdateMouseClippingRegion();
	unguard;
}

//
// Toggle fullscreen.
//
void UWindowsViewport::ToggleFullscreen()
{
	guard(UWindowsViewport::ToggleFullscreen);
	if (BlitFlags & BLIT_Fullscreen)
	{
		EndFullscreen();
	}
	else if (!(Actor->ShowFlags & SHOW_ChildWindow))
	{
		debugf(TEXT("AttemptFullscreen"));
		TryRenderDevice(TEXT("ini:Engine.Engine.GameRenderDevice"), INDEX_NONE, INDEX_NONE, INDEX_NONE, 1);
		if (!RenDev)
			TryRenderDevice(TEXT("ini:Engine.Engine.WindowedRenderDevice"), INDEX_NONE, INDEX_NONE, INDEX_NONE, 1);
		if (!RenDev)
			TryRenderDevice(TEXT("ini:Engine.Engine.WindowedRenderDevice"), INDEX_NONE, INDEX_NONE, INDEX_NONE, 0);
	}
	UpdateMouseClippingRegion();
	unguard;
}

//
// Initialize the Dib section for SoftDrv
//
void UWindowsViewport::InitializeDibSection(INT NewX, INT NewY, INT NewColorBytes)
{
	if (hBitmap)
		DeleteObject(hBitmap);
	hBitmap = NULL;
	
	// Create DIB section.
	struct { BITMAPINFOHEADER Header; RGBQUAD Colors[256]; } Bitmap;

	// Init BitmapHeader for DIB.
	appMemzero( &Bitmap, sizeof(Bitmap) );
	Bitmap.Header.biSize			= sizeof(BITMAPINFOHEADER);
	Bitmap.Header.biWidth			= NewX;
	Bitmap.Header.biHeight			= -NewY;
	Bitmap.Header.biPlanes			= 1;
	Bitmap.Header.biBitCount		= NewColorBytes * 8;
	Bitmap.Header.biSizeImage		= NewX * NewY * NewColorBytes;

	// Handle color depth.
	if( NewColorBytes==2 )
	{
		// 16-bit color (565).
		Bitmap.Header.biCompression = BI_BITFIELDS;
		*(DWORD *)&Bitmap.Colors[0] = (Caps & CC_RGB565) ? 0xF800 : 0x7C00;
		*(DWORD *)&Bitmap.Colors[1] = (Caps & CC_RGB565) ? 0x07E0 : 0x03E0;
		*(DWORD *)&Bitmap.Colors[2] = (Caps & CC_RGB565) ? 0x001F : 0x001F;
	}
	else if( NewColorBytes==3 || NewColorBytes==4 )
	{
		// 24-bit or 32-bit color.
		Bitmap.Header.biCompression = BI_RGB;
		*(DWORD *)&Bitmap.Colors[0] = 0;
	}
	else appErrorf( TEXT("Invalid DibSection color depth %i"), NewColorBytes );

	// Create DIB section.
	HDC TempDC = GetDC(0);
	check(TempDC);
	hBitmap = CreateDIBSection( TempDC, (BITMAPINFO*)&Bitmap.Header, DIB_RGB_COLORS, (void**)&ScreenPointer, NULL, 0 );
	ReleaseDC( 0, TempDC );
	if( !hBitmap )
		appErrorf( LocalizeError("OutOfMemory",TEXT("Core")) );
	check(ScreenPointer);
}

VOID CALLBACK EmitDisplayChange( 
    HWND hwnd,        // handle to window for timer messages 
    UINT message,     // WM_TIMER message 
    UINT_PTR idTimer, // timer identifier 
    DWORD dwTime)     // current system time 
{
	KillTimer(hwnd, idTimer);
	SendMessage(hwnd, WM_DISPLAYCHANGE, (WPARAM)0, (LPARAM)0);
}

//
// Resize the viewport.
//
UBOOL UWindowsViewport::ResizeViewport( DWORD NewBlitFlags, INT InNewX, INT InNewY, INT InNewColorBytes )
{
	guard(UWindowsViewport::ResizeViewport);
	UWindowsClient* Client = GetOuterUWindowsClient();

	// Handle temporary viewports.
	if( BlitFlags & BLIT_Temporary )
		NewBlitFlags &= ~(BLIT_DirectDraw | BLIT_DibSection);

	// Handle DirectDraw not available.
	if ((NewBlitFlags & BLIT_DirectDraw) && !Client->dd)
	{
		if (NewBlitFlags & BLIT_Fullscreen)
			debugf(TEXT("Cannot enter fullscreen mode because DirectDraw is not available. Check if UseDirectDraw is set to true."));
		NewBlitFlags = (NewBlitFlags | BLIT_DibSection) & ~(BLIT_Fullscreen | BLIT_DirectDraw);
	}

	// If going windowed, but the rendering device is fullscreen-only, switch to the software renderer.
	if( RenDev && RenDev->FullscreenOnly && !(NewBlitFlags & BLIT_Fullscreen) )
	{
		guard(SoftwareBail);
		if( !(GetFlags() & RF_Destroyed) )
		{
			TryRenderDevice( TEXT("ini:Engine.Engine.WindowedRenderDevice"), INDEX_NONE, INDEX_NONE, InNewColorBytes, 0 );
			check(RenDev);
		}
		return 0;
		unguard;
	}

	// Remember viewport.
	UViewport* SavedViewport = NULL;
	if( Client->Engine->Audio && !GIsEditor && !(GetFlags() & RF_Destroyed) )
		SavedViewport = Client->Engine->Audio->GetViewport();

	// Accept default parameters.
	INT NewX          = InNewX         ==INDEX_NONE ? SizeX      : InNewX;
	INT NewY          = InNewY         ==INDEX_NONE ? SizeY      : InNewY;
	INT NewColorBytes = InNewColorBytes==INDEX_NONE ? ColorBytes : InNewColorBytes;

	// Shut down current frame.
	if( BlitFlags & BLIT_DirectDraw )
	{
		debugf( NAME_Log, TEXT("DirectDraw session ending") );
		if( SavedViewport )
			Client->Engine->Audio->SetViewport( NULL );
		check(ddBackBuffer);
		ddBackBuffer->Release();
		check(ddFrontBuffer);
		ddFrontBuffer->Release();
		if( !(NewBlitFlags & BLIT_DirectDraw) )
		{
			HoldCount++;
			Client->ddEndMode();
			HoldCount--;
		}
	}
	else if( BlitFlags & BLIT_DibSection )
	{
		if( hBitmap )
			DeleteObject( hBitmap );
		hBitmap = NULL;
	}

	// Get this window rect.
	FRect WindowRect = SavedWindowRect;
	if( Window->hWnd && !(BlitFlags & BLIT_Fullscreen) && !(NewBlitFlags&BLIT_NoWindowChange) )
		WindowRect = Window->GetWindowRect();

	// Default resolution handling.
	NewX = InNewX!=INDEX_NONE ? InNewX : (NewBlitFlags&BLIT_Fullscreen) ? Client->FullscreenViewportX : Client->WindowedViewportX;
	NewY = InNewX!=INDEX_NONE ? InNewY : (NewBlitFlags&BLIT_Fullscreen) ? Client->FullscreenViewportY : Client->WindowedViewportY;

	// Align NewX.
	check(NewX>=0);
	check(NewY>=0);
	NewX = Align(NewX,2);

	// If currently fullscreen, end it.
	if( BlitFlags & BLIT_Fullscreen )
	{
		// Saved parameters.
		SetFocus( Window->hWnd );
		if( InNewColorBytes==INDEX_NONE )
			NewColorBytes = SavedColorBytes;

		// Remember saved info.
		WindowRect          = SavedWindowRect;
		Caps                = SavedCaps;

		// Restore window topness.
		SetWindowPos( Window->hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOACTIVATE ); // stijn: when switching from fullscreen to windowed -> revoke TOPMOST state
		SetDrag( 0 );

		// Stop inhibiting windows keys.
		if (Client->InhibitWindowsHotkeys)
		{
			UnregisterHotKey(Window->hWnd, Client->hkAltEsc);
			UnregisterHotKey(Window->hWnd, Client->hkAltTab);
			UnregisterHotKey(Window->hWnd, Client->hkCtrlEsc);
			UnregisterHotKey(Window->hWnd, Client->hkCtrlTab);
		}
		//DWORD Old=0;
		//SystemParametersInfoX( SPI_SCREENSAVERRUNNING, 0, &Old, 0 );
	}

	// If transitioning into fullscreen.
	if( (NewBlitFlags & BLIT_Fullscreen) && !(BlitFlags & BLIT_Fullscreen) )
	{
		// Save window parameters.
		SavedWindowRect = WindowRect;
		SavedColorBytes	= ColorBytes;
		SavedCaps       = Caps;

		// Make "Advanced Options" not return fullscreen after this.
		if( Client->ConfigProperties )
		{
			Client->ConfigReturnFullscreen = 0;
			DestroyWindow( *Client->ConfigProperties );
		}

		// Turn off window border and menu.
		HoldCount++;
		SendMessageW( Window->hWnd, WM_SETREDRAW, 0, 0 );
		SetMenu( Window->hWnd, NULL );
		if( !GIsEditor )
		{			
			if (Client->UseDirectDraw && Client->UseDirectDrawBorderlessFullscreen)
			{
				HRESULT Result = Client->dd->SetCooperativeLevel( Window->hWnd, DDSCL_NORMAL );
				debugf( TEXT("DirectDraw SetCooperativeLevel: %ls"), ddError(Result) );
				SetWindowLongPtrW(Window->hWnd, GWL_STYLE, GetWindowLongW(Window->hWnd, GWL_STYLE) & ~(WS_POPUP));
			}
			SetWindowLongPtrW(Window->hWnd, GWL_STYLE, GetWindowLongW(Window->hWnd, GWL_STYLE) & ~(WS_CAPTION | WS_THICKFRAME));
			SetWindowPos(Window->hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED); // Regarding MSDN: If you have changed certain window data using SetWindowLong, you must call SetWindowPos for the changes to take effect. Use the following combination for uFlags: SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED.
		}
		SendMessageW( Window->hWnd, WM_SETREDRAW, 1, 0 );
		HoldCount--;
	}

	if (!GIsEditor && !(NewBlitFlags & BLIT_Fullscreen) && NewX != 0 && NewY != 0) // skip minimzed state
	{
		INT Width = SizeX;
		INT Height = SizeY;
		GetBorderlessSize(Width, Height);
		UBOOL NeedBorderless = NewX == Width && NewY == Height;
		UBOOL AlreadyBorderless = !(GetWindowLongW(Window->hWnd, GWL_STYLE) & WS_CAPTION);
		if (NeedBorderless)
			NewBlitFlags = NewBlitFlags | BLIT_NoWindowChange;
		HoldCount++;
		if (NeedBorderless != AlreadyBorderless)
		{
			if (NeedBorderless)
			{
				SetWindowLongPtrW(Window->hWnd, GWL_STYLE, (GetWindowLongW(Window->hWnd, GWL_STYLE) & ~(WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME)) | WS_POPUP);
				SetWindowPos(Window->hWnd, HWND_TOP, 0, 0, Width, Height, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
			}
			else 
			{
				SetWindowLongPtrW(Window->hWnd, GWL_STYLE, (GetWindowLongW(Window->hWnd, GWL_STYLE) & ~(WS_POPUP)) | (WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX));
				SetWindowPos(Window->hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED); // Regarding MSDN: If you have changed certain window data using SetWindowLong, you must call SetWindowPos for the changes to take effect. Use the following combination for uFlags: SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED.
			}
		}
		if (Client->UseDirectDraw && Client->UseDirectDrawBorderlessFullscreen)
		{
			if (NeedBorderless)
			{
				HRESULT Result = Client->dd->SetCooperativeLevel( Window->hWnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT );
				debugf( TEXT("DirectDraw SetCooperativeLevel: %ls"), ddError(Result) );
				if (Result == DDERR_EXCLUSIVEMODEALREADYSET) // try restart on fail
				{
					Result = Client->dd->SetCooperativeLevel( Window->hWnd, DDSCL_NORMAL );
					debugf( TEXT("DirectDraw SetCooperativeLevel: %ls"), ddError(Result) );
					Result = Client->dd->SetCooperativeLevel( Window->hWnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT );
					debugf( TEXT("DirectDraw SetCooperativeLevel: %ls"), ddError(Result) );
				}
				Result = Client->dd->SetDisplayMode(Width, Height, ColorBytes*8, 0, 0);
				debugf( TEXT("DirectDraw SetDisplayMode: %ls"), ddError(Result) );
				if (RenDev && (Width != NewX || Height != NewY))
					RenDev->SetRes(Width, Height, ColorBytes, 0);
			} else if (AlreadyBorderless) {
				HRESULT Result = Client->dd->SetCooperativeLevel( Window->hWnd, DDSCL_NORMAL );
				debugf( TEXT("DirectDraw SetCooperativeLevel: %ls"), ddError(Result) );
			}
		}
		HoldCount--;
	}

	// Handle display method.
	if( NewBlitFlags & BLIT_DirectDraw )
	{
		// Go into closest matching DirectDraw mode.
		INT BestMode=-1, BestDelta=MAXINT;
		for( INT i=0; i<Client->DisplayModes.Num(); i++ )
		{
			DDSURFACEDESC2& Desc = Client->DisplayModes(i);
			if (Desc.ddpfPixelFormat.dwRGBBitCount == ColorBytes * 8)
			{
				INT Delta = Abs<INT>(Desc.dwWidth - NewX) + Abs<INT>(Desc.dwHeight - NewY);
				if (Delta < BestDelta)
				{
					BestMode = i;
					BestDelta = Delta;
				}
			}
		}
		if( BestMode>=0 )
		{
			// Try to go into DirectDraw.
			DDSURFACEDESC2& BestDesc = Client->DisplayModes(BestMode);
			NewX = BestDesc.dwWidth;
			NewY = BestDesc.dwHeight;
			HoldCount++;
			if( !(BlitFlags & BLIT_DirectDraw) )
			{
				if( SavedViewport )
					Client->Engine->Audio->SetViewport( NULL );
				HRESULT Result = Client->dd->SetCooperativeLevel( Window->hWnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT );
				if( Result != DD_OK )
				{
					debugf( TEXT("DirectDraw SetCooperativeLevel failed: %ls"), ddError(Result) );
   					return 0;
				}
			}
			SetCursor( NULL );
			UBOOL Result = ddSetMode( NewX, NewY, NewColorBytes );
			SetForegroundWindow( Window->hWnd );
			HoldCount--;
			if( !Result )
			{
				// DirectDraw failed.
				HoldCount++;
				Client->dd->SetCooperativeLevel( NULL, DDSCL_NORMAL );
				Window->MoveWindow( SavedWindowRect, 1 );
				SetTopness();
				HoldCount--;
				debugf( LocalizeError("DDrawMode") );
				return 0;
			}
		}
		else
		{
			appErrorf( TEXT("Could not initialize renderer because no valid DirectDraw modes were found.") );
			return 0;
		}
	}
	else if( (NewBlitFlags&BLIT_DibSection) && NewX && NewY )
	{
		InitializeDibSection(NewX, NewY, NewColorBytes);
	}
	else if( !(NewBlitFlags & BLIT_Temporary) )
	{
		ScreenPointer = NULL;
	}

	// OpenGL handling.
	if( (NewBlitFlags & BLIT_Fullscreen) && (NewBlitFlags & BLIT_OpenGL) && !GIsEditor && RenDev )
	{
		// Turn off window border and menu.
		HoldCount++;
		SendMessageW( Window->hWnd, WM_SETREDRAW, 0, 0 );
		Window->MoveWindow( FRect(0,0,NewX,NewY), 0 );
		SendMessageW( Window->hWnd, WM_SETREDRAW, 1, 0 );
		HoldCount--;
	}

	// Set new info.
	DWORD OldBlitFlags = BlitFlags;
	// stijn: we need to retain the temporary flag, otherwise WinDrv will try to move the window and repaint it.
	if (OldBlitFlags & BLIT_Temporary)
		NewBlitFlags |= BLIT_Temporary;
	BlitFlags          = NewBlitFlags & ~BLIT_ParameterFlags;
	SizeX              = NewX;
	SizeY              = NewY;
	ColorBytes         = NewColorBytes ? NewColorBytes : ColorBytes;

	// If transitioning out of fullscreen.
	if( !(NewBlitFlags & BLIT_Fullscreen) && (OldBlitFlags & BLIT_Fullscreen) )
	{
		SetMouseCapture( 0, 0, 0 );

		// Emit WM_DISPLAYCHANGE few second after, for update borderless state on window, after all transitions ends.
		for (INT i = 1; i <= 3; i++)
			SetTimer(Window->hWnd, 42 + i, i*1000, &EmitDisplayChange);
	}

	// Handle type.
	if (NewBlitFlags & BLIT_Fullscreen)
	{
		// Handle fullscreen input.
		SetDrag( 1 );
		SetMouseCapture(1, 1, 0);
		if (Client->InhibitWindowsHotkeys)
		{
			RegisterHotKey(Window->hWnd, Client->hkAltEsc, MOD_ALT, VK_ESCAPE);
			RegisterHotKey(Window->hWnd, Client->hkAltTab, MOD_ALT, VK_TAB);
			RegisterHotKey(Window->hWnd, Client->hkCtrlEsc, MOD_CONTROL, VK_ESCAPE);
			RegisterHotKey(Window->hWnd, Client->hkCtrlTab, MOD_CONTROL, VK_TAB);
		}
		//DWORD Old=0;
		//SystemParametersInfoX( SPI_SCREENSAVERRUNNING, 1, &Old, 0 );

		SetTopness();
	}
	else if( !(NewBlitFlags & BLIT_Temporary) && !(NewBlitFlags & BLIT_NoWindowChange) )
	{
		// Going to a window.
		FRect ClientRect(0,0,NewX,NewY);
		AdjustWindowRect( ClientRect, GetWindowLongPtrW(Window->hWnd,GWL_STYLE), (Actor->ShowFlags & SHOW_Menu)!=0 );

		// Resize the window and repaint it.
		if( !(Actor->ShowFlags & SHOW_ChildWindow) )
		{
			HoldCount++;
			Window->MoveWindow(FRect(WindowRect.Min, WindowRect.Min + ClientRect.Size()), 1);
			const FRect EffectiveClientRect = Window->GetClientRect();
			SizeX = EffectiveClientRect.Max.X;
			SizeY = EffectiveClientRect.Max.Y;
			if (BlitFlags & BLIT_DibSection && (SizeX != NewX || SizeY != NewY))
				InitializeDibSection(SizeX, SizeY, NewColorBytes);			
			HoldCount--;
		}
		SetDrag( 0 );
	}

	// Update audio.
	if (SavedViewport && SavedViewport != Client->Engine->Audio->GetViewport())
		Client->Engine->Audio->SetViewport(SavedViewport);

	// Update the window.
	UpdateWindowFrame();

	// Save info.
	if( RenDev && !GIsEditor && !IgnoreResolutionConfigChange )
	{
		if( NewBlitFlags & BLIT_Fullscreen )
		{
			if( NewX && NewY )
			{
				Client->FullscreenViewportX  = NewX;
				Client->FullscreenViewportY  = NewY;
				Client->FullscreenColorBits  = NewColorBytes*8;
			}
		}
		else
		{
			if( NewX && NewY )
			{
				Client->WindowedViewportX  = NewX;
				Client->WindowedViewportY  = NewY;
				Client->WindowedColorBits  = NewColorBytes*8;
			}
		}
		Client->SaveConfig();
	}

	return 1;
	unguard;
}

#ifdef UTPG_WINXP_FIREWALL

// backport from UT2004

// This is an WinXP only firewall hack

UBOOL UWindowsViewport::FireWallHack(INT Cmd)
{
	guard(UWindowsViewport::FireWallHack);

	UBOOL Result=TRUE;

	HRESULT hr = S_OK;
    INetFwMgr* fwMgr = NULL;			// The Firewall Manager Object
	INetFwPolicy* fwPolicy = NULL;		// The Local policy
	INetFwProfile* fwProfile = NULL;	// The Local profile

	INT IgnoreSP2=0;
	GConfig->GetInt(TEXT("FireWall"),TEXT("IgnoreSP2"),IgnoreSP2);

	if (IgnoreSP2>0)
		return TRUE;

	// Initialize COM for speech recognition.
	if( !UWindowsViewport::CoInitialized )
	{
		CoInitialize( NULL );
		CoInitialized = 1;
	}

	// Create the firewall ComObject

	if (FAILED (hr = CoCreateInstance(__uuidof(NetFwMgr), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwMgr), (void **)&fwMgr) ) )
	{
		GWarn->Logf(TEXT("FIREWALL: Could not Create XP.SP2 FireWall Object"));
		return true;
	}

	// Get the Policy
	if ( SUCCEEDED( hr=fwMgr->get_LocalPolicy(&fwPolicy)) )	
	{
		// Get the Profile
		if ( SUCCEEDED(hr = fwPolicy->get_CurrentProfile(&fwProfile) ) )
		{
			// Check to see if it's enabled

			VARIANT_BOOL fwEnabled;
			if ( SUCCEEDED( hr=fwProfile->get_FirewallEnabled(&fwEnabled)) && (fwEnabled == VARIANT_TRUE) )	
			{
				INetFwAuthorizedApplications* fwApps = NULL;	// All Firewall Applications

				if ( SUCCEEDED(hr = fwProfile->get_AuthorizedApplications(&fwApps)) )
				{

					INetFwAuthorizedApplication* fwApp = NULL;		// A Single Firewall Application
					BSTR fwBstrProcessImageFileName = NULL;
					fwBstrProcessImageFileName = SysAllocString(*FString::Printf(TEXT("%ls%ls.exe"),appBaseDir(),appPackage()));
					if (SysStringLen(fwBstrProcessImageFileName) >0)
					{

						if (Cmd==0)	// Check to See if it already authorized
						{
							if ( SUCCEEDED(hr = fwApps->Item(fwBstrProcessImageFileName,&fwApp)) )
							{
								// If the application's status is undefined or disabled, popup the message

								if ( FAILED(hr = fwApp->get_Enabled(&fwEnabled)) || fwEnabled != VARIANT_TRUE )
								{
									GWarn->Logf(TEXT("FIREWALL: UT is not authorized for outside access"));
									Result = FALSE;
								}

								fwApp->Release();
							}
							else	
							{
								GWarn->Logf(TEXT("FIREWALL: UT status is undefined therefore not authorized for outside access"));
								Result = FALSE;
							}
						}
						else if (Cmd==1)	// Authorize this package
						{
							BSTR fwBstrProcessImageDesc = NULL;
							fwBstrProcessImageDesc = SysAllocString(*FString::Printf(TEXT("%ls"),appPackage()));
							if (SysStringLen(fwBstrProcessImageDesc) >0)
							{

								hr = CoCreateInstance(__uuidof(NetFwAuthorizedApplication),	NULL,CLSCTX_INPROC_SERVER,
														__uuidof(INetFwAuthorizedApplication),	(void**) &fwApp);
								
								if ( SUCCEEDED(hr) )	// Process Created
								{	

									// Set the process image file name.
									hr = fwApp->put_ProcessImageFileName(fwBstrProcessImageFileName);
									if ( SUCCEEDED(hr) )
									{

										// Set the process description

										hr = fwApp->put_Name(fwBstrProcessImageDesc);
										if ( SUCCEEDED(hr) )
										{

											hr = fwApps->Add(fwApp);
											if ( FAILED(hr) )
											{
												GWarn->Logf(TEXT("FIREWALL: Could not Authorize"));
												Result = FALSE;
											}
										}
										else
										{
											GWarn->Logf(TEXT("FIREWALL: Could not Set Process Name"));
											Result = FALSE;
										}
									}
									else
									{
										GWarn->Logf(TEXT("FIREWALL: Could not Set Process Filename"));
										Result = FALSE;
									}

									fwApp->Release();
								}
								else
								{
									GWarn->Logf(TEXT("FIREWALL: Could not create authorization item"));
									Result = FALSE;
								}

								SysFreeString(fwBstrProcessImageDesc);

							}
							else
							{
								GWarn->Logf(TEXT("FIREWALL: Out of Memory (2)"));
								Result = FALSE;
							}

						}

						SysFreeString(fwBstrProcessImageFileName);
					}
					else
						GWarn->Logf(TEXT("FIREWALL: Out of Memory"));

					fwApps->Release();
				}
				else 
					GWarn->Logf(TEXT("FIREWALL: Could not obtain Authorized Application List"));
			}
			else
				GWarn->Logf(TEXT("FIREWALL: XP.SP2 FireWall Not Enabled"));

			fwProfile->Release();
		}
		else
			GWarn->Logf(TEXT("FIREWALL: Could not Accquire Local Profile"));

		fwPolicy->Release();
	}
	else
		GWarn->Logf(TEXT("FIREWALL: Could not Accquire Local Policy"));

	fwMgr->Release();
	GWarn->Logf(TEXT("FIREWALL: Freed XP.SP2 Firewall Object"));

	return Result;

	unguard;

}

UBOOL					UWindowsViewport::CoInitialized			= 0;

#endif

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
