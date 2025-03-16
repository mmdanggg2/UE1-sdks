/*=============================================================================
	Launch.cpp: Game launcher.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Tim Sweeney.
=============================================================================*/

#include "LaunchPrivate.h"
#include "FConfigCacheIni.h"
#include "UnEngineWin.h"

/*-----------------------------------------------------------------------------
	Global variables.
-----------------------------------------------------------------------------*/

// General.
extern "C" {HINSTANCE hInstance;}
TCHAR GPackageInternal[64] = TEXT("Launch");
extern "C" {const TCHAR* GPackage=GPackageInternal;}

// GPU selection
extern "C" 
{
	DLL_EXPORT DWORD NvOptimusEnablement = 0x00000001;
	DLL_EXPORT INT AmdPowerXpressRequestHighPerformance = 1;
}

// Log file.
#include "FOutputDeviceFile.h"
FOutputDeviceFile Log;

// Error handler.
#include "FOutputDeviceWindowsError.h"
FOutputDeviceWindowsError Error;

// Feedback.
#include "FFeedbackContextWindows.h"
FFeedbackContextWindows Warn;

// File manager.
#include "FFileManagerWindows.h"
FFileManagerWindows FileManager;

// Config.
#include "FConfigCacheIni.h"

#if __STATIC_LINK

// Extra stuff for static links for Engine.
#include "UnEngineNative.h"
#include "UnCon.h"
#include "UnRender.h"
#include "UnNet.h"

// Editor static stuff.
#include "EditorPrivate.h"
#include "UnEditorNative.h"

// Fire static stuff.
#include "UnFractal.h"
#include "UnFireNative.h"

// IpDrv static stuff.
#include "UnIpDrv.h"
#include "UnTcpNetDriver.h"
#include "UnIpDrvCommandlets.h"
#include "UnIpDrvNative.h"
#include "HTTPDownload.h"

// UWeb static stuff.
#include "UWeb.h"
#include "UWebNative.h"

// Render static stuff.
#include "Render.h"
#include "UnRenderNative.h"

// Audio devices
//#include "ALAudio.h"
//#include "Cluster.h"
# if !BUILD_64
#  include "UnGalaxy.h"
# endif

// Renderers
//#include "D3D9Drv.h"
//#include "UTGLRD3D9.h"
#include "SoftDrvPrivate.h"
#include "XOpenGLDrv.h"
#include "OpenGLDrv.h"
#include "UTGLROpenGL.h"

// Windrv stuff
#include "WinDrv.h"

// UDemo stuff
#include "udemoprivate.h"
#include "udemoNative.h"

#endif

/*-----------------------------------------------------------------------------
	WinMain.
-----------------------------------------------------------------------------*/

//
// Main entry point.
// This is an example of how to initialize and launch the engine.
//
INT WINAPI WinMain( HINSTANCE hInInstance, HINSTANCE hPrevInstance, char*, INT nCmdShow )
{
	appChdirSystem();

	// Remember instance.
	INT ErrorLevel = 0;
	GIsStarted     = 1;
	hInstance      = hInInstance;
	const TCHAR* CmdLine = GetCommandLine();
	// stijn: In the executables, GPackage actually points to a writable TCHAR array 
	// of 64 elements so the const_cast on the next line is fine!
	appStrncpy(const_cast<TCHAR*>(GPackage), appPackage(), 64);

	// See if this should be passed to another instances.
	if
	(	!appStrfind(CmdLine,TEXT("Server"))
	&&	!appStrfind(CmdLine,TEXT("NewWindow"))
	&&	!appStrfind(CmdLine,TEXT("changevideo"))
	&&  !appStrfind(CmdLine, TEXT("changesound"))
	&&	!appStrfind(CmdLine,TEXT("TestRenDev")) )
	{
		TCHAR ClassName[1024];
		MakeWindowClassName(ClassName,TEXT("WLog"));
		for( HWND hWnd=NULL; ; )
		{
			hWnd = FindWindowExW(hWnd,NULL,ClassName,NULL);
			if( !hWnd )
				break;
			if( GetPropW(hWnd,TEXT("IsBrowser")) )
			{
				while( *CmdLine && *CmdLine!=' ' )
					CmdLine++;
				if( *CmdLine==' ' )
					CmdLine++;
				COPYDATASTRUCT CD;
				PTRINT Result;
				CD.dwData = WindowMessageOpen;
				CD.cbData = (appStrlen(CmdLine)+1)*sizeof(TCHAR*);
				CD.lpData = const_cast<TCHAR*>( CmdLine );
				SendMessageTimeout( hWnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&CD, SMTO_ABORTIFHUNG|SMTO_BLOCK, 30000, &Result );
				GIsStarted = 0;
				return 0;
			}
		}
	}

#if __STATIC_LINK
	// Clean lookups.
	for (INT k = 0; k < ARRAY_COUNT(GNativeLookupFuncs); k++)
	{
		GNativeLookupFuncs[k] = NULL;
	}

	// Core lookups.
	INT k = 0;
	GNativeLookupFuncs[k++] = &FindCoreUObjectNative;
	GNativeLookupFuncs[k++] = &FindCoreUCommandletNative;
	GNativeLookupFuncs[k++] = &FindCoreURegistryNative;

	// Engine lookups.
	GNativeLookupFuncs[k++] = &FindEngineAActorNative;
	GNativeLookupFuncs[k++] = &FindEngineAPawnNative;
	GNativeLookupFuncs[k++] = &FindEngineAPlayerPawnNative;
	GNativeLookupFuncs[k++] = &FindEngineADecalNative;
	GNativeLookupFuncs[k++] = &FindEngineAStatLogNative;
	GNativeLookupFuncs[k++] = &FindEngineAStatLogFileNative;
	GNativeLookupFuncs[k++] = &FindEngineAZoneInfoNative;
	GNativeLookupFuncs[k++] = &FindEngineAWarpZoneInfoNative;
	GNativeLookupFuncs[k++] = &FindEngineALevelInfoNative;
	GNativeLookupFuncs[k++] = &FindEngineAGameInfoNative;
	GNativeLookupFuncs[k++] = &FindEngineANavigationPointNative;
	GNativeLookupFuncs[k++] = &FindEngineUCanvasNative;
	GNativeLookupFuncs[k++] = &FindEngineUConsoleNative;
	GNativeLookupFuncs[k++] = &FindEngineUScriptedTextureNative;

	GNativeLookupFuncs[k++] = &FindIpDrvAInternetLinkNative;
	GNativeLookupFuncs[k++] = &FindIpDrvAUdpLinkNative;
	GNativeLookupFuncs[k++] = &FindIpDrvATcpLinkNative;

	// UWeb lookups.
	GNativeLookupFuncs[k++] = &FindUWebUWebResponseNative;
	GNativeLookupFuncs[k++] = &FindUWebUWebRequestNative;

	// UDemo lookups.
	GNativeLookupFuncs[k++] = &FindudemoUUZHandlerNative;
	GNativeLookupFuncs[k++] = &FindudemoUudnativeNative;
	GNativeLookupFuncs[k++] = &FindudemoUDemoInterfaceNative;

	// Editor lookups.
	GNativeLookupFuncs[k++] = &FindEditorUBrushBuilderNative;
#endif

	// Begin guarded code.
#ifndef _DEBUG
	try
	{
#endif
		// Init core.
		GIsClient = GIsGuarded = 1;
		appInit( GPackage, *appPlatformBuildCmdLine(1), nullptr, &Log, &Error, &Warn, &FileManager, FConfigCacheIni::Factory, 1 );

#if __STATIC_LINK
		AUTO_INITIALIZE_REGISTRANTS_ENGINE;
		AUTO_INITIALIZE_REGISTRANTS_EDITOR;
		//AUTO_INITIALIZE_REGISTRANTS_ALAUDIO;
		//AUTO_INITIALIZE_REGISTRANTS_CLUSTER;
		//AUTO_INITIALIZE_REGISTRANTS_GALAXY;

		//AUTO_INITIALIZE_REGISTRANTS_D3D9DRV;
		//AUTO_INITIALIZE_REGISTRANTS_SOFTDRV;
		AUTO_INITIALIZE_REGISTRANTS_OPENGLDRV;
		AUTO_INITIALIZE_REGISTRANTS_XOPENGLDRV;

		AUTO_INITIALIZE_REGISTRANTS_WINDRV;
		AUTO_INITIALIZE_REGISTRANTS_WINDOW;
		
		AUTO_INITIALIZE_REGISTRANTS_FIRE;
		AUTO_INITIALIZE_REGISTRANTS_IPDRV;
		AUTO_INITIALIZE_REGISTRANTS_UWEB;
		AUTO_INITIALIZE_REGISTRANTS_RENDER;		
		AUTO_INITIALIZE_REGISTRANTS_UDEMO;

		InitUdemo();
#endif

		// Init mode.
		GIsServer     = 1;
		GIsClient     = !ParseParam(appCmdLine(),TEXT("SERVER"));
		GIsEditor     = 0;
		GIsScriptable = 1;
		GLazyLoad     = !GIsClient || ParseParam(appCmdLine(),TEXT("LAZY"));

		// Figure out whether to show log or splash screen.
		UBOOL ShowLog = ParseParam(CmdLine,TEXT("LOG"));
		INT RunCount = appAtoi(GConfig->GetStr(TEXT("Engine.Engine"),TEXT("RunCount")));
		FString Filename = FString::Printf(TEXT("..\\Help\\Splash%i.bmp"),RunCount%5);
		GConfig->SetString(TEXT("Engine.Engine"),TEXT("RunCount"),*FString::Printf(TEXT("%i"),RunCount+1));
		if( GFileManager->FileSize(*Filename)<0 )
			Filename = FString(TEXT("..\\Help")) * TEXT("Logo.bmp");
		if( GFileManager->FileSize(*Filename)<0 )
			Filename = TEXT("..\\Help\\Logo.bmp");
		// stijn: In the executables, GPackage actually points to a writable TCHAR array 
		// of 64 elements so the const_cast on the next line is fine!
		appStrncpy(const_cast<TCHAR*>(GPackage), appPackage(), 64);
		if( !ShowLog && !ParseParam(CmdLine,TEXT("server")) && !appStrfind(CmdLine,TEXT("TestRenDev")) )
			InitSplash( *Filename );

		// Init windowing.
		InitWindowing();

		// Create log window, but only show it if ShowLog.
		GLogWindow = new WLog( *Log.Filename, Log.LogAr->GetAr(), TEXT("GameLog") );
		GLogWindow->OpenWindow( ShowLog, 0 );
		GLogWindow->Log( NAME_Title, LocalizeGeneral(TEXT("Start")) );
		if( GIsClient )
			SetPropW( *GLogWindow, TEXT("IsBrowser"), (HANDLE)1 );

		// Init engine.
		UEngine* Engine = InitEngine();
		if( Engine )
		{
			GLogWindow->Log( NAME_Title, LocalizeGeneral(TEXT("Run")) );
			
			// Hide splash screen.
			ExitSplash();

#ifdef UTPG_WINXP_FIREWALL
			if( Engine->Client && Engine->Client->Viewports.Num() && Engine->Client->Viewports(0) )
			{
				if (!Engine->Client->Viewports(0)->FireWallHack(1))
				{
					GWarn->Logf(TEXT("Failed to authorize Windows XP SP2 Firewall.\nPlease check your firewall settings."));					
				}
			}
#endif

			// Optionally Exec an exec file
			FString Temp;
			if( Parse(CmdLine, TEXT("EXEC="), Temp) )
			{
				Temp = FString(TEXT("exec ")) + Temp;
				if( Engine->Client && Engine->Client->Viewports.Num() && Engine->Client->Viewports(0) )
					Engine->Client->Viewports(0)->Exec( *Temp, *GLogWindow );
			}

			// Start main engine loop, including the Windows message pump.
			if( !GIsRequestingExit )
				MainLoop( Engine );

			if (Engine->Client && Engine->Client->Viewports.Num() && Engine->Client->Viewports(0))
			{
				Engine->Client->Viewports(0)->Exec(TEXT("EndFullscreen"));
				Engine->Client->Viewports(0)->Exec(TEXT("SAVESCREENPOS"));
			}
		}

		// Clean shutdown.
		GFileManager->Delete(TEXT("Running.ini"),0,0);
		RemovePropW( *GLogWindow, TEXT("IsBrowser") );
		GLogWindow->Log( NAME_Title, LocalizeGeneral(TEXT("Exit")) );
		delete GLogWindow;
		appPreExit();
		GIsGuarded = 0;
#ifndef _DEBUG
	}
	catch( ... )
	{
		// Crashed.
		ErrorLevel = 1;
		Error.HandleError();
	}
#endif

	// Final shut down.
	appExit();
	GIsStarted = 0;
	return ErrorLevel;
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
