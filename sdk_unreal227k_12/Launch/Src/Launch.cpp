/*=============================================================================
	Launch.cpp: Game launcher.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Tim Sweeney.
=============================================================================*/

#include "LaunchPrivate.h"
#include "UnEngineWin.h"
#include "FConfigCacheIni.h"

#if __STATIC_LINK

// Extra stuff for static links for Engine.
#include "UnFont.h"
#include "UnEngineNative.h"
#include "UnRender.h"
#include "UnNet.h"

// UPak static stuff.
#include "UPak.h"
#include "UnUPakNative.h"

// Fire static stuff.
#include "FireClasses.h"
#include "UnFluidSurface.h"
#include "UnProcMesh.h"
#include "UnFractal.h"
#include "UnFireNative.h"

// IpDrv static stuff.
#include "UnIpDrv.h"
#include "UnTcpNetDriver.h"
#include "UnIpDrvCommandlets.h"
#include "UnIpDrvNative.h"
#include "HTTPDownload.h"

#include "EditorPrivate.h"
#include "UnEditorNative.h"

#include "EmitterPrivate.h"
#include "UnEmitterNative.h"

// UWeb static stuff.
#include "UWebAdminPrivate.h"
#include "UnUWebAdminNative.h"

// Renderers
#include "../../D3D9drv/src/D3D9Drv.h"
#include "../../D3D9drv/src/D3D9.h"
#include "SoftDrvPrivate.h"
#include "XOpenGLDrv.h"
#include "OpenGLDrv.h"

#include "UnRenderNative.h"

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shellapi.h>

// Windrv stuff
#include "WinDrv.h"
#endif

/*-----------------------------------------------------------------------------
	Global variables.
-----------------------------------------------------------------------------*/

// General.
extern "C" {HINSTANCE hInstance;}
TCHAR GPackageInternal[64] = TEXT("Launch");
extern "C" {const TCHAR* GPackage = GPackageInternal;}

// Memory allocator.
#include "FMallocAnsi.h"
FMallocAnsi Malloc;

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

/*-----------------------------------------------------------------------------
	WinMain.
-----------------------------------------------------------------------------*/

//
// Main entry point.
// This is an example of how to initialize and launch the engine.
//
INT WINAPI WinMain( HINSTANCE hInInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, INT nCmdShow )
{
	GMalloc = &Malloc;
	GMalloc->Init();

	// Remember instance.
	INT ErrorLevel = 0;
	GIsStarted     = 1;
	hInstance      = hInInstance;

	const TCHAR* CmdLine = GetCommandLine();
	appStrncpy(const_cast<TCHAR*>(GPackage), appPackage(), 64);

	// See if this should be passed to another instances.
	if
	(	!appStrfind(CmdLine,TEXT("Server"))
	&&	!appStrfind(CmdLine,TEXT("NewWindow"))
	&&	!appStrfind(CmdLine,TEXT("changevideo"))
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
	memset(GNativeLookupFuncs, 0, sizeof(NativeLookup) * ARRAY_COUNT(GNativeLookupFuncs));
	// Core lookups.
	INT i = 0;
	GNativeLookupFuncs[i++] = &FindCoreUObjectNative;
	GNativeLookupFuncs[i++] = &FindCoreUCommandletNative;
	GNativeLookupFuncs[i++] = &FindCoreULogHandlerNative;
	GNativeLookupFuncs[i++] = &FindCoreUScriptHookNative;
	GNativeLookupFuncs[i++] = &FindCoreULocaleNative;
	GNativeLookupFuncs[i++] = &FindNetSerializeNative;

	GNativeLookupFuncs[i++] = &FindEngineAActorNative;
	GNativeLookupFuncs[i++] = &FindEngineAPawnNative;
	GNativeLookupFuncs[i++] = &FindEngineAPlayerPawnNative;
	GNativeLookupFuncs[i++] = &FindEngineADecalNative;
	GNativeLookupFuncs[i++] = &FindEngineATimeDemoNative;
	GNativeLookupFuncs[i++] = &FindEngineAStatLogNative;
	GNativeLookupFuncs[i++] = &FindEngineAStatLogFileNative;
	GNativeLookupFuncs[i++] = &FindEngineAZoneInfoNative;
	GNativeLookupFuncs[i++] = &FindEngineAWarpZoneInfoNative;
	GNativeLookupFuncs[i++] = &FindEngineALevelInfoNative;
	GNativeLookupFuncs[i++] = &FindEngineAGameInfoNative;
	GNativeLookupFuncs[i++] = &FindEngineANavigationPointNative;
	GNativeLookupFuncs[i++] = &FindEngineUCanvasNative;
	GNativeLookupFuncs[i++] = &FindEngineUConsoleNative;
	GNativeLookupFuncs[i++] = &FindEngineUEngineNative;
	GNativeLookupFuncs[i++] = &FindEngineUScriptedTextureNative;
	GNativeLookupFuncs[i++] = &FindEngineUShadowBitMapNative;
	GNativeLookupFuncs[i++] = &FindEngineUIK_SolverBaseNative;
	GNativeLookupFuncs[i++] = &FindEngineAProjectorNative;
	GNativeLookupFuncs[i++] = &FindEngineUPX_PhysicsDataBaseNative;
	GNativeLookupFuncs[i++] = &FindEngineUPXJ_BaseJointNative;
	GNativeLookupFuncs[i++] = &FindEngineUPhysicsAnimationNative;
	GNativeLookupFuncs[i++] = &FindEngineURenderIteratorNative;
	GNativeLookupFuncs[i++] = &FindEngineAVolumeNative;
	GNativeLookupFuncs[i++] = &FindEngineAInventoryNative;
	GNativeLookupFuncs[i++] = &FindEngineAProjectileNative;
	GNativeLookupFuncs[i++] = &FindEngineUClientPreloginSceneNative;
	GNativeLookupFuncs[i++] = &FindEngineUServerPreloginSceneNative;

	GNativeLookupFuncs[i++] = &FindFireAProceduralMeshNative;
	GNativeLookupFuncs[i++] = &FindFireUPaletteModifierNative;
	GNativeLookupFuncs[i++] = &FindFireUConstantColorNative;
	GNativeLookupFuncs[i++] = &FindFireUTextureModifierBaseNative;
	GNativeLookupFuncs[i++] = &FindFireAFluidSurfaceInfoNative;

	GNativeLookupFuncs[i++] = &FindEmitterAXRopeDecoNative;
	GNativeLookupFuncs[i++] = &FindEmitterAXEmitterNative;
	GNativeLookupFuncs[i++] = &FindEmitterAXParticleEmitterNative;
	GNativeLookupFuncs[i++] = &FindEmitterAXWeatherEmitterNative;

	GNativeLookupFuncs[i++] = &FindUPakAPawnPathNodeIteratorNative;
	GNativeLookupFuncs[i++] = &FindUPakAPathNodeIteratorNative;

	GNativeLookupFuncs[i++] = &FindPathLogicUPathLogicManagerNative;

	GNativeLookupFuncs[i++] = &FindEditorUBrushBuilderNative;
	GNativeLookupFuncs[i++] = &FindEditorUEdGUI_CheckBoxNative;
	GNativeLookupFuncs[i++] = &FindEditorUEdGUI_ComponentNative;
	GNativeLookupFuncs[i++] = &FindEditorUEdGUI_BaseNative;
	GNativeLookupFuncs[i++] = &FindEditorUEdGUI_ComboBoxNative;
	GNativeLookupFuncs[i++] = &FindEditorUEdGUI_WindowFrameNative;

	GNativeLookupFuncs[i++] = &FindIpDrvAInternetLinkNative;
	GNativeLookupFuncs[i++] = &FindIpDrvAUdpLinkNative;
	GNativeLookupFuncs[i++] = &FindIpDrvATcpLinkNative;

	GNativeLookupFuncs[i++] = &FindUWebAdminUWebQueryNative;

#endif

	// Begin guarded code.
#if !defined(_DEBUG) && DO_GUARD
	try
	{
#endif
		// Init core.
		GIsClient = GIsGuarded = 1;
		appInit( GPackage, CmdLine, &Malloc, &Log, &Error, &Warn, &FileManager, FConfigCacheIni::Factory, 1 );

#if __STATIC_LINK
		AUTO_INITIALIZE_REGISTRANTS_ENGINE;
		AUTO_INITIALIZE_REGISTRANTS_EDITOR;
		//AUTO_INITIALIZE_REGISTRANTS_ALAUDIO;
		//AUTO_INITIALIZE_REGISTRANTS_CLUSTER;
		//AUTO_INITIALIZE_REGISTRANTS_GALAXY;

		AUTO_INITIALIZE_REGISTRANTS_D3D9DRV;
		//AUTO_INITIALIZE_REGISTRANTS_SOFTDRV;
		//AUTO_INITIALIZE_REGISTRANTS_OPENGLDRV;
		//AUTO_INITIALIZE_REGISTRANTS_XOPENGLDRV;

		AUTO_INITIALIZE_REGISTRANTS_WINDRV;
		AUTO_INITIALIZE_REGISTRANTS_WINDOW;
		AUTO_INITIALIZE_REGISTRANTS_EMITTER;

		AUTO_INITIALIZE_REGISTRANTS_FIRE;
		AUTO_INITIALIZE_REGISTRANTS_IPDRV;
		AUTO_INITIALIZE_REGISTRANTS_RENDER;
#endif

		// Init mode.
		GIsServer     = 1;
		GIsClient     = !ParseParam(appCmdLine(),TEXT("SERVER"));
		GIsEditor     = 0;
		GIsScriptable = 1;
		GLazyLoad     = !GIsClient || ParseParam(appCmdLine(),TEXT("LAZY"));

		// Figure out whether to show log or splash screen.
		UBOOL ShowLog = ParseParam(CmdLine,TEXT("LOG"));
		INT RunCount = appAtoi(*GConfig->GetStr(TEXT("Engine.Engine"),TEXT("RunCount")));
		FString Filename = FString::Printf(TEXT("..\\Help\\Splash%i.bmp"),RunCount%5);
		GConfig->SetString(TEXT("Engine.Engine"),TEXT("RunCount"),*FString::Printf(TEXT("%i"),RunCount+1));
		Parse(CmdLine,TEXT("Splash="),Filename);
		if( GFileManager->FileSize(*Filename)<0 )
			Filename = TEXT("..\\Help\\Logo.bmp");

		appStrncpy(const_cast<TCHAR*>(GPackage), appPackage(), 64);

		if( !ShowLog && !ParseParam(CmdLine,TEXT("server")) && !appStrfind(CmdLine,TEXT("TestRenDev")) )
			InitSplash( *Filename );

		// Init windowing.
		InitWindowing();

		// Create log window, but only show it if ShowLog.
		GLogWindow = new WLog( *Log.Filename, Log.LogAr->GetAr(), TEXT("GameLog") );
		GLogWindow->OpenWindow( ShowLog, 0 );
		GLogWindow->Log( NAME_Title, LocalizeGeneral("Start") );
		if( GIsClient )
			SetPropW( *GLogWindow, TEXT("IsBrowser"), (HANDLE)1 );

		// Init engine.
		UEngine* Engine = InitEngine();
		if( Engine )
		{
			GLogWindow->Log( NAME_Title, LocalizeGeneral("Run") );

			// Hide splash screen.
			ExitSplash();

#ifdef UTPG_WINXP_FIREWALL
			if( Engine->Client && Engine->Client->Viewports.Num() && Engine->Client->Viewports(0) )
			{
				if (!Engine->Client->Viewports(0)->FireWallHack(1))
				{
					debugf(TEXT("Failed to authorize Windows Firewall.\nPlease check your firewall settings."));
				}
			}
#endif

			// Save reference to main window HWND handle so that message boxes can modal themselves.
			if (Engine->Client && Engine->Client->Viewports.Num() && Engine->Client->Viewports(0))
				GSys->MainWindow = Engine->Client->Viewports(0)->GetWindow();

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
				Engine->Client->Viewports(0)->Exec(TEXT("SAVESCREENPOS"));
		}

		// Clean shutdown.
		GFileManager->Delete(GRunningFile);
		RemovePropW( *GLogWindow, TEXT("IsBrowser") );
		GLogWindow->Log( NAME_Title, LocalizeGeneral("Exit") );
		delete GLogWindow;

		//if(GIsClient)
		//	GRenderDevice->ResetGamma(); //Ensure GammaReset is called on Renderer before really exiting.

		appPreExit();
		GIsGuarded = 0;
#if !defined(_DEBUG) && DO_GUARD
	}
	catch (TCHAR* Err)
	{
		// Crashed.
		Error.Serialize(Err, NAME_Error);
		ErrorLevel = 1;
		GIsGuarded = 0;
		Error.HandleError();
	}
	catch( ... )
	{
		// Crashed.
		ErrorLevel = 1;
		GIsGuarded = 0;
		Error.HandleError();
	}
#endif

	UBOOL bAutoReboot = (GIsCriticalError && GIsClient && GSys && GSys->CrashAutoRestart && GFrameNumber>120);

	// Final shut down.
	appExit();
	GIsStarted = 0;

	if (bAutoReboot)
	{
		PlaySound((LPCTSTR)SND_ALIAS_SYSTEMEXCLAMATION, NULL, SND_ALIAS_ID);
		USystem::appRelaunch(TEXT(""));
	}
	return ErrorLevel;
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
