/*=============================================================================
	WinClient.cpp: UWindowsClient code.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
		* HIDInput implementation by Smirftsch
=============================================================================*/

#include "WinDrv.h"

/*-----------------------------------------------------------------------------
	Class implementation.
-----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(UWindowsClient);

/*-----------------------------------------------------------------------------
	UWindowsClient implementation.
-----------------------------------------------------------------------------*/

//
// UWindowsClient constructor.
//
UWindowsClient::UWindowsClient()
{
	guard(UWindowsClient::UWindowsClient);

	// Init hotkey atoms.
	hkAltEsc	= GlobalAddAtom( TEXT("UnrealAltEsc")  );
	hkAltTab	= GlobalAddAtom( TEXT("UnrealAltTab")  );
	hkCtrlEsc	= GlobalAddAtom( TEXT("UnrealCtrlEsc") );
	hkCtrlTab	= GlobalAddAtom( TEXT("UnrealCtrlTab") );

	unguard;
}

//
// Static init.
//
void UWindowsClient::StaticConstructor()
{
	guard(UWindowsClient::StaticConstructor);

	new(GetClass(),TEXT("InhibitWindowsHotkeys"),		RF_Public)UBoolProperty(CPP_PROPERTY(InhibitWindowsHotkeys),	TEXT("Display"),  CPF_Config );
	new(GetClass(),TEXT("UseDirectDraw"),				RF_Public)UBoolProperty(CPP_PROPERTY(UseDirectDraw),			TEXT("Display"),  CPF_Config );
	new(GetClass(),TEXT("UseDirectDrawBorderlessFullscreen"), RF_Public)UBoolProperty(CPP_PROPERTY(UseDirectDrawBorderlessFullscreen), TEXT("Display"),  CPF_Config );
	new(GetClass(),TEXT("UseDirectInput"),				RF_Public)UBoolProperty(CPP_PROPERTY(UseDirectInput),			TEXT("Display"),  CPF_Config );
	new(GetClass(),TEXT("UseRawHIDInput"),				RF_Public)UBoolProperty(CPP_PROPERTY(UseRawHIDInput),			TEXT("Display"),  CPF_Config );
	new(GetClass(),TEXT("EnableEditorRawHIDInput"),		RF_Public)UBoolProperty(CPP_PROPERTY(EnableEditorRawHIDInput),	TEXT("Display"),  CPF_Config );
	new(GetClass(),TEXT("UseLegacyCursorInput"),		RF_Public)UBoolProperty(CPP_PROPERTY(UseLegacyCursorInput),		TEXT("Display"),  CPF_Config );
	new(GetClass(),TEXT("NoEnhancedPointerPrecision"),	RF_Public)UBoolProperty(CPP_PROPERTY(NoEnhancedPointerPrecision),TEXT("Display"),  CPF_Config );
	new(GetClass(),TEXT("UseJoystick"),					RF_Public)UBoolProperty(CPP_PROPERTY(UseJoystick),				TEXT("Display"),  CPF_Config );
	new(GetClass(),TEXT("StartupFullscreen"),			RF_Public)UBoolProperty(CPP_PROPERTY(StartupFullscreen),		TEXT("Display"),  CPF_Config );
	new(GetClass(),TEXT("StartupBorderless"),			RF_Public)UBoolProperty(CPP_PROPERTY(StartupBorderless),		TEXT("Display"),  CPF_Config );
	new(GetClass(),TEXT("DeadZoneXYZ"),					RF_Public)UBoolProperty(CPP_PROPERTY(DeadZoneXYZ),				TEXT("Joystick"), CPF_Config );
	new(GetClass(),TEXT("DeadZoneRUV"),					RF_Public)UBoolProperty(CPP_PROPERTY(DeadZoneRUV),				TEXT("Joystick"), CPF_Config );
	new(GetClass(),TEXT("InvertVertical"),				RF_Public)UBoolProperty(CPP_PROPERTY(InvertVertical),			TEXT("Joystick"), CPF_Config );
	new(GetClass(),TEXT("ScaleXYZ"),					RF_Public)UFloatProperty(CPP_PROPERTY(ScaleXYZ),				TEXT("Joystick"), CPF_Config );
	new(GetClass(),TEXT("ScaleRUV"),					RF_Public)UFloatProperty(CPP_PROPERTY(ScaleRUV),				TEXT("Joystick"), CPF_Config );
	new(GetClass(),TEXT("WinPosX"),						RF_Public)UIntProperty(CPP_PROPERTY(WinPosX),					TEXT("Display"), CPF_Config );
	new(GetClass(),TEXT("WinPosY"),						RF_Public)UIntProperty(CPP_PROPERTY(WinPosY),					TEXT("Display"), CPF_Config );

	WinPosX = INDEX_NONE;
	WinPosY = INDEX_NONE;
	UseLegacyCursorInput = TRUE;
	
	if (!GIsEditor)
		NoEnhancedPointerPrecision = TRUE;

	unguard;
}

//
// DirectDraw driver enumeration callback.
//
BOOL CALLBACK UWindowsClient::EnumDirectDrawsCallback(GUID* Guid, char* Description, char* Name, void* Context, HMONITOR Monitor)
{
	FGuid NoGuid(0, 0, 0, 0);
	new(*static_cast<TArray<FDeviceInfo>*>(Context))FDeviceInfo(Guid ? *Guid : *(GUID*)& NoGuid, Description ? appFromAnsi(Description) : TEXT(""), Name ? appFromAnsi(Name) : TEXT(""));
	debugf(NAME_Init, TEXT("   %s (%s)"), Name ? appFromAnsi(Name) : TEXT("None"), Description ? appFromAnsi(Description) : TEXT("None"));
	return 1;
}

HRESULT CALLBACK UWindowsClient::EnumModesCallback(DDSURFACEDESC2* Desc, void* Context)
{
	static_cast<TArray<DDSURFACEDESC2>*>(Context)->AddItem(*Desc);
	return DDENUMRET_OK;
}

//
// Initialize the platform-specific viewport manager subsystem.
// Must be called after the Unreal object manager has been initialized.
// Must be called before any viewports are created.
//
void UWindowsClient::Init( UEngine* InEngine )
{
	guard(UWindowsClient::UWindowsClient);

	// Init base.
	UClient::Init( InEngine );

	// Register window class.
	IMPLEMENT_WINDOWCLASS(WWindowsViewportWindow,GIsEditor ? (CS_DBLCLKS|CS_OWNDC) : (CS_OWNDC));

	// Create a working DC compatible with the screen, for CreateDibSection.
	hMemScreenDC = CreateCompatibleDC( NULL );
	if( !hMemScreenDC )
		appErrorf( TEXT("CreateCompatibleDC failed: %s"), appGetSystemErrorMessage() );

	// Get mouse info.
	if (!NoEnhancedPointerPrecision)
	{
		SystemParametersInfoW(SPI_GETMOUSE, 0, NormalMouseInfo, 0);
		debugf(NAME_Init, TEXT("Enhanced Pointer Precision Enabled - Mouse info: %i %i %i"), NormalMouseInfo[0], NormalMouseInfo[1], NormalMouseInfo[2]);
		CaptureMouseInfo[0] = 0;     // First threshold.
		CaptureMouseInfo[1] = 0;     // Second threshold.
		CaptureMouseInfo[2] = 65536; // Speed.
	}
	else
	{
		debugf(NAME_Init, TEXT("Enhanced Pointer Precision Disabled"));
	}

	// Init DirectDraw.
	for( ; ; )
	{
		// Skip out.
		if
		(	!UseDirectDraw
		||	ParseParam(appCmdLine(),TEXT("noddraw"))
		||	appStricmp(GConfig->GetStr(TEXT("Engine.Engine"),TEXT("GameRenderDevice")),TEXT("GlideDrv.GlideRenderDevice"))==0
		||	appStricmp(GConfig->GetStr(TEXT("Engine.Engine"),TEXT("GameRenderDevice")),TEXT("D3DDrv.D3DRenderDevice"))==0 )
		{
			break;
		}
		debugf( NAME_Init, TEXT("Initializing DirectDraw") );

		// Load DirectDraw DLL.
		HINSTANCE Instance = LoadLibraryW(TEXT("ddraw.dll"));
		if( Instance == NULL )
		{
			debugf( NAME_Init, TEXT("DirectDraw not installed") );
			break;
		}
		ddCreateFunc = (DD_CREATE_FUNC)GetProcAddress( Instance, "DirectDrawCreateEx" );
		ddEnumFunc   = (DD_ENUM_FUNC)GetProcAddress( Instance, "DirectDrawEnumerateExA" );
		if( !ddCreateFunc || !ddEnumFunc )
		{
			debugf( NAME_Init, TEXT("DirectDraw GetProcAddress failed") );
			break;
		}

		// Enumerate DirectDraw devices.
		guard(EnumDirectDraws);
		DirectDraws.Empty();
		debugf(NAME_Init, TEXT("DirectDraw drivers detected:"));
		ddEnumFunc(EnumDirectDrawsCallback, &DirectDraws, 0/*!!flags for multimonitor?*/);
		if (!DirectDraws.Num())
		{
			debugf(NAME_Init, TEXT("No DirectDraw drivers found"));
			break;
		}
		unguard;

		// Find best DirectDraw driver.
		FDeviceInfo Best = DirectDraws(0);
		guard(FindBestDriver);
		for (TArray<FDeviceInfo>::TIterator It(DirectDraws); It; ++It)
			if (It->Description.InStr(TEXT("Primary")) != -1)
				Best = *It;
		unguard;

		// Create DirectDraw.
		guard(CreateDirectDraw);
		HRESULT Result;
		if (FAILED(Result = ddCreateFunc(&Best.Guid, &dd, IID_IDirectDraw7, NULL)))
		{
			debugf(NAME_Init, TEXT("DirectDraw created failed: %s"), ddError(Result));
			break;
		}
		unguard;		

		// Get list of display modes.
		guard(EnumDisplayModes);
		DisplayModes.Empty();
		// stijn: 224/436 tested specific DDSURFACEDESCs here but this no longer works with ddraw7
		dd->EnumDisplayModes(0, NULL, &DisplayModes, EnumModesCallback);
		debugf(NAME_Init, TEXT("DirectDraw: Enumerated Display Modes:"));
		for (TArray<DDSURFACEDESC2>::TIterator It(DisplayModes); It; ++It)
			debugf(NAME_Init, TEXT("DirectDraw: Mode %dx%dx%d"), It->dwWidth, It->dwHeight, It->ddpfPixelFormat.dwRGBBitCount);
		unguard;

		debugf(NAME_Init, TEXT("DirectDraw initialized successfully"));

		// Successfuly initialized DirectDraw.
		break;
	}

	// Input System
	if (ParseParam(appCmdLine(), TEXT("safe")))
	{
		if (UseRawHIDInput)
		{
			UsingRawHIDInput = 0;
			debugf(NAME_Init, TEXT("Disabling RawHIDInput for safe mode"));
		}
		if (UseDirectInput)
		{
			UsingDirectInput = 0;
			debugf(NAME_Init, TEXT("Disabling DirectInput for safe mode"));
		}
	}
	else if (UseDirectInput)
	{
		if (GIsEditor)
		{
			debugf(NAME_Init, TEXT("DirectInput is not available in UnrealEd. Switching to RawHIDInput for this session."));
			UsingDirectInput = 0;
			UsingRawHIDInput = 1;
		}
		else if (UseRawHIDInput)
		{
			debugf(NAME_Init, TEXT("RawHIDInput is enabled, disabling DirectInput"));
			UsingDirectInput = 0;
		}
		else
		{
			UsingDirectInput = 1;
		}
	}
	else if (UseRawHIDInput)
	{
		UsingRawHIDInput = 1;
	}

	if (GIsEditor && UsingRawHIDInput && !EnableEditorRawHIDInput)
	{
		debugf(NAME_Init, TEXT("RawHIDInput support is disabled in UnrealEd. Set EnableEditorRawHIDInput to true to enable support."));
		UsingRawHIDInput = 0;
	}

	if (UsingRawHIDInput)
	{
		debugf(NAME_Init, TEXT("Initializing Mouse Input Handler: Raw Input"));
		MouseInputHandler = new FWindowsRawInputHandler;
	}
	else if (UsingDirectInput)
	{
		debugf(NAME_Init, TEXT("Initializing Mouse Input Handler: DirectInput"));
		MouseInputHandler = new FWindowsDirectInputHandler;
	}
	else
	{
		debugf(NAME_Init, TEXT("Initializing Mouse Input Handler: Win32 Cursor Input (Legacy: %ls)"), UseLegacyCursorInput ? TEXT("TRUE") : TEXT("FALSE"));
		if (UseLegacyCursorInput)
			MouseInputHandler = new FWindowsLegacyWin32InputHandler;
		else
			MouseInputHandler = new FWindowsWin32InputHandler;
	}

	if (!MouseInputHandler->SetupInput())
	{
		debugf(NAME_Init, TEXT("Mouse Input Handler initialization failed. Falling back to Win32 Cursor Input (Legacy: %ls)"), UseLegacyCursorInput ? TEXT("TRUE") : TEXT("FALSE"));
		delete MouseInputHandler;
		if (UseLegacyCursorInput)
			MouseInputHandler = new FWindowsLegacyWin32InputHandler;
		else
			MouseInputHandler = new FWindowsWin32InputHandler;
	}

	// Fix up the environment variables for 3dfx.
	_putenv( "SST_RGAMMA=" );
	_putenv( "SST_GGAMMA=" );
	_putenv( "SST_BGAMMA=" );
	_putenv( "FX_GLIDE_NO_SPLASH=1" );

	// Note configuration.
	PostEditChange();

	// Default res option.
	if( ParseParam(appCmdLine(),TEXT("defaultres")) )
	{
		WindowedViewportX  = FullscreenViewportX  = 1024;
		WindowedViewportY  = FullscreenViewportY  = 768;
	}

	// Success.
	debugf( NAME_Init, TEXT("Client initialized") );
	unguard;
}
 
//
// Shut down the platform-specific viewport manager subsystem.
//
void UWindowsClient::Destroy()
{
	guard(UWindowsClient::Destroy);

	// Shut down DirectDraw.
	if( dd )
	{
		ddEndMode();
		HRESULT Result = dd->Release();
		if( Result != DD_OK )
			debugf( NAME_Exit, TEXT("DirectDraw Release failed: %s"), ddError(Result) );
		else
			debugf( NAME_Exit, TEXT("DirectDraw released") );
		dd = NULL;
	}

	if (MouseInputHandler)
		MouseInputHandler->ShutdownInput();
	delete MouseInputHandler;
	MouseInputHandler = nullptr;

	// Stop capture.
	SetCapture( NULL );
	ClipCursor( NULL );
	ShowCursor(TRUE);
	if (!NoEnhancedPointerPrecision)
		SystemParametersInfoW( SPI_SETMOUSE, 0, NormalMouseInfo, 0 );

	// Clean up Windows resources.
	if( !DeleteDC( hMemScreenDC ) )
		debugf( NAME_Exit, TEXT("DeleteDC failed %i"), GetLastError() );

	debugf( NAME_Exit, TEXT("Windows client shut down") );
	Super::Destroy();
	unguard;
}

//
// Failsafe routine to shut down viewport manager subsystem
// after an error has occured. Not guarded.
//
void UWindowsClient::ShutdownAfterError()
{
	debugf( NAME_Exit, TEXT("Executing UWindowsClient::ShutdownAfterError") );
	if (MouseInputHandler)
		MouseInputHandler->ShutdownInput();
	SetCapture( NULL );
	ClipCursor( NULL );
	if (!NoEnhancedPointerPrecision)
		SystemParametersInfoW( SPI_SETMOUSE, 0, NormalMouseInfo, 0 );
  	ShowCursor( TRUE );
	if( Engine && Engine->Audio )
	{
		Engine->Audio->ConditionalShutdownAfterError();
	}
	for( INT i=Viewports.Num()-1; i>=0; i-- )
	{
		UWindowsViewport* Viewport = (UWindowsViewport*)Viewports( i );
		Viewport->ConditionalShutdownAfterError();
	}
	ddEndMode();
	Super::ShutdownAfterError();
}

void UWindowsClient::NotifyDestroy( void* Src )
{
	guard(UWindowsClient::NotifyDestroy);
	if( Src==ConfigProperties )
	{
		ConfigProperties = NULL;
		if( ConfigReturnFullscreen && Viewports.Num() )
			Viewports(0)->Exec( TEXT("ToggleFullscreen") );
	}
	unguard;
}

//
// Command line.
//
UBOOL UWindowsClient::Exec( const TCHAR* Cmd, FOutputDevice& Ar )
{
	guard(UWindowsClient::Exec);
	if( UClient::Exec( Cmd, Ar ) )
	{
		return 1;
	}
	return 0;
	unguard;
}

//
// Perform timer-tick processing on all visible viewports.  This causes
// all realtime viewports, and all non-realtime viewports which have been
// updated, to be blitted.
//
void UWindowsClient::Tick()
{
	guard(UWindowsClient::Tick);
	// Blit any viewports that need blitting.
  	for( INT i=0; i<Viewports.Num(); i++ )
	{
		UWindowsViewport* Viewport = CastChecked<UWindowsViewport>(Viewports(i));
		check(!Viewport->HoldCount);
		if( !IsWindow(Viewport->Window->hWnd) )
		{
			// Window was closed via close button.
			if (GIsEditor)
				delete Viewport;
			else
				appRequestExit(0);
			return;
		}
  		else if
		(	(Viewport->IsRealtime() || Viewport->RepaintPending)
		&&	Viewport->SizeX
		&&	Viewport->SizeY )
		{
			Viewport->Repaint(1);
		}
	}
	unguard;
}

//
// Create a new viewport.
//
UViewport* UWindowsClient::NewViewport( const FName Name )
{
	guard(UWindowsClient::NewViewport);
	return new( this, Name )UWindowsViewport();
	unguard;
}

//
// End the current DirectDraw mode.
//
void UWindowsClient::ddEndMode()
{
	guard(UWindowsClient::ddEndMode);
	if( dd )
	{
		// Return to normal cooperative level.
		debugf( NAME_Log, TEXT("DirectDraw End Mode") );
		HRESULT Result = dd->SetCooperativeLevel( NULL, DDSCL_NORMAL );
		if( Result!=DD_OK )
			debugf( NAME_Log, TEXT("DirectDraw SetCooperativeLevel: %s"), ddError(Result) );

		// Restore the display mode.
		Result = dd->RestoreDisplayMode();
		if( Result!=DD_OK )
			debugf( NAME_Log, TEXT("DirectDraw RestoreDisplayMode: %s"), ddError(Result) );

		// Flip to GDI surface (may return error code; this is ok).
		dd->FlipToGDISurface();
	}
	if( !GIsCriticalError )
	{
		// Flush the cache unless we're ending DirectDraw due to a crash.
		debugf( NAME_Log, TEXT("Flushing cache") );
		GCache.Flush();
	}
	unguard;
}

//
// Configuration change.
//
void UWindowsClient::PostEditChange()
{
	guard(UWindowsClient::PostEditChange);
	Super::PostEditChange();

	// Joystick.
	appMemzero( &JoyCaps, sizeof(JoyCaps) );
	if( UseJoystick && !ParseParam(appCmdLine(),TEXT("NoJoy")) && !GIsEditor )
	{
		INT nJoys = joyGetNumDevs();
		if( !nJoys )
			debugf( TEXT("No joystick detected") );
		if( joyGetDevCapsA(JOYSTICKID1,&JoyCaps,sizeof(JoyCaps))==JOYERR_NOERROR )
			debugf( TEXT("Detected joysticks: %i (%s)"), nJoys, JoyCaps.szOEMVxD ? appFromAnsi(JoyCaps.szOEMVxD) : TEXT("None") );
		else debugf( TEXT("joyGetDevCaps failed") );
	}

	unguard;
}

//
// Enable or disable all viewport windows that have ShowFlags set (or all if ShowFlags=0).
//
void UWindowsClient::EnableViewportWindows( DWORD ShowFlags, int DoEnable )
{
	guard(UWindowsClient::EnableViewportWindows);
  	for( int i=0; i<Viewports.Num(); i++ )
	{
		UWindowsViewport* Viewport = (UWindowsViewport*)Viewports(i);
		if( (Viewport->Actor->ShowFlags & ShowFlags)==ShowFlags )
			EnableWindow( Viewport->Window->hWnd, DoEnable );
	}
	unguard;
}

//
// Show or hide all viewport windows that have ShowFlags set (or all if ShowFlags=0).
//
void UWindowsClient::ShowViewportWindows( DWORD ShowFlags, int DoShow )
{
	guard(UWindowsClient::ShowViewportWindows); 	
	for( int i=0; i<Viewports.Num(); i++ )
	{
		UWindowsViewport* Viewport = (UWindowsViewport*)Viewports(i);
		if( (Viewport->Actor->ShowFlags & ShowFlags)==ShowFlags )
			Viewport->Window->Show(DoShow);
	}
	unguard;
}

//
// Make this viewport the current one.
// If Viewport=0, makes no viewport the current one.
//
void UWindowsClient::MakeCurrent( UViewport* InViewport )
{
	guard(UWindowsViewport::MakeCurrent);
	for( INT i=0; i<Viewports.Num(); i++ )
	{
		UViewport* OldViewport = Viewports(i);
		if( OldViewport->Current && OldViewport!=InViewport )
		{
			OldViewport->Current = 0;
			OldViewport->UpdateWindowFrame();
		}
	}
	if( InViewport )
	{
		InViewport->Current = 1;
		InViewport->UpdateWindowFrame();
	}
	unguard;
}

#if !OLDUNREAL_BINARY_COMPAT
//
// Copy text to clipboard.
//
UBOOL UWindowsClient::SetClipboardText( FString& Str )
{
	guard(UWindowsClient::SetClipboardText);
	if( OpenClipboard(GetActiveWindow()) )
	{
		verify(EmptyClipboard());
		TCHAR* Data = (TCHAR*)GlobalAlloc( GMEM_DDESHARE, sizeof(TCHAR)*(Str.Len() + 1) );
		appStrncpy( Data, *Str, Str.Len() + 1 ); 
		UBOOL Result = SetClipboardData( CF_UNICODETEXT, Data )
			&& CloseClipboard();
		return Result;		
	}
	return FALSE;	
	unguard;
}

//
// Paste text from clipboard.
//
FString UWindowsClient::GetClipboardText()
{
	guard(UWindowsClient::GetClipboardText);
	FString Result;
	if( OpenClipboard(GetActiveWindow()) )
	{
		void* V;
		if ((V = GetClipboardData(CF_UNICODETEXT)) != nullptr)
		{
			TCHAR* pchData = (TCHAR*)GlobalLock(V);
			if (pchData)
			{
				Result = pchData;
				GlobalUnlock(V);
			}
		}
		else if ((V = GetClipboardData(CF_TEXT)) != nullptr)
		{
			ANSICHAR* pchData = (ANSICHAR*)GlobalLock(V);
			if (pchData)
			{
				Result = pchData;
				GlobalUnlock(V);
			}
		}
		verify(CloseClipboard());
	}
	else Result=TEXT("");
	return Result;
	unguard;
}
#endif

/*-----------------------------------------------------------------------------
	Getting error messages.
-----------------------------------------------------------------------------*/
