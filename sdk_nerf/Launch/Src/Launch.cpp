/*=============================================================================
	Launch.cpp: Game launcher.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Tim Sweeney.
=============================================================================*/

#include "LaunchPrivate.h"
#include "UnEngineWin.h"

/*-----------------------------------------------------------------------------
	Global variables.
-----------------------------------------------------------------------------*/

// General.
extern "C" {HINSTANCE hInstance;}
extern "C" {TCHAR GPackage[64]=TEXT("Launch");}

// Memory allocator.
#ifdef _DEBUG
#include "FMallocDebug.h"
FMallocDebug Malloc;
#else
#include "FMallocWindows.h"
FMallocWindows Malloc;
#endif

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

/*-----------------------------------------------------------------------------
	WinMain.
-----------------------------------------------------------------------------*/

//
// Main entry point.
// This is an example of how to initialize and launch the engine.
//
INT WINAPI WinMain( HINSTANCE hInInstance, HINSTANCE hPrevInstance, char*, INT nCmdShow )
{
	// Remember instance.
	GIsStarted     = 1;
	INT ErrorLevel = 0;
	hInstance      = hInInstance;
	UBOOL bPlayCredits = 0;
	UBOOL bRanAsClient = 0;
	UBOOL bMatriculate = 0;
	const TCHAR* CmdLine = GetCommandLine();
	appStrcpy( GPackage, appPackage() );

	// See if this should be passed to another instances.
	if( !appStrfind(CmdLine,TEXT("Server")) && !appStrfind(CmdLine,TEXT("NewWindow")) && !appStrfind(CmdLine,TEXT("TestRenDev")) )
	{
		TCHAR ClassName[256];
		MakeWindowClassName(ClassName,TEXT("WLog"));
		for( HWND hWnd=NULL; ; )
		{
			hWnd = TCHAR_CALL_OS(FindWindowExW(hWnd,NULL,ClassName,NULL),FindWindowExA(hWnd,NULL,TCHAR_TO_ANSI(ClassName),NULL));
			if( !hWnd )
				break;
			if( GetPropX(hWnd,TEXT("IsBrowser")) )
			{
				while( *CmdLine && *CmdLine!=' ' )
					CmdLine++;
				if( *CmdLine==' ' )
					CmdLine++;
				COPYDATASTRUCT CD;
				DWORD Result;
				CD.dwData = WindowMessageOpen;
				CD.cbData = (appStrlen(CmdLine)+1)*sizeof(TCHAR*);
				CD.lpData = const_cast<TCHAR*>( CmdLine );
				SendMessageTimeout( hWnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&CD, SMTO_ABORTIFHUNG|SMTO_BLOCK, 30000, &Result );
				GIsStarted = 0;
				return 0;
			}
		}
	}

	// Begin guarded code.
#ifndef _DEBUG
	try
	{
#endif
		// Init core.
		GIsClient = GIsGuarded = 1;
		appInit( GPackage, CmdLine, &Malloc, &Log, &Error, &Warn, &FileManager, FConfigCacheIni::Factory, 1 );
		if( ParseParam(appCmdLine(),TEXT("MAKE")) )//oldver
			appErrorf( TEXT("'Unreal -make' is obsolete, use 'ucc make' now") );

		// Init mode.
		GIsServer     = 1;
		GIsClient     = !ParseParam(appCmdLine(),TEXT("SERVER"));
		GIsEditor     = 0;
		GIsScriptable = 1;
		GLazyLoad     = !GIsClient || ParseParam(appCmdLine(),TEXT("LAZY"));

		// Figure out whether to show log or splash screen.
		UBOOL ShowLog = ParseParam(CmdLine,TEXT("LOG"));
		FString Filename = FString(TEXT("..\\Help")) * GPackage + TEXT("Logo.bmp");
		if( GFileManager->FileSize(*Filename)<0 )
			Filename = TEXT("..\\Help\\Logo.bmp");
		appStrcpy( GPackage, appPackage() );

		if ( !ShowLog && !ParseParam(CmdLine,TEXT("server"))  && !appStrfind(CmdLine,TEXT("TestRenDev")) )
			InitSplash( *Filename );

		// Init windowing.
		InitWindowing();

		// Create log window, but only show it if ShowLog.
		GLogWindow = new WLog( Log.Filename, Log.LogAr, TEXT("GameLog") );
		GLogWindow->OpenWindow( ShowLog, 0 );
		GLogWindow->Log( NAME_Title, LocalizeGeneral("Start") );
		if( GIsClient )
			SetPropX( *GLogWindow, TEXT("IsBrowser"), (HANDLE)1 );

		// Init engine.
		UEngine* Engine = InitEngine();

		if ( Engine )
		{
			GLogWindow->Log( NAME_Title, LocalizeGeneral("Run") );

			// Hide splash screen.
			ExitSplash();

			// Optionally Exec an exec file
			FString Temp;
			if( Parse(CmdLine, TEXT("EXEC="), Temp) )
			{
				Temp = FString(TEXT("exec ")) + Temp;
				if( Engine->Client && Engine->Client->Viewports.Num() && Engine->Client->Viewports(0) )
					Engine->Client->Viewports(0)->Exec( *Temp, *GLogWindow );
			}

			// Set config entries for VNerf Bink video playback.
			GConfig->SetBool( TEXT("NerfI.NerfIPlayer"), TEXT("bPlayCredits"), 0, TEXT("user") );
			GConfig->SetBool( TEXT("Engine.PlayerPawn"), TEXT("bMatriculate"), 0, TEXT("user") );

			// Start main engine loop, including the Windows message pump.
			if( !GIsRequestingExit )
				MainLoop( Engine );

			// Was game run as a client?
			bRanAsClient = GIsClient;

			// Get config entries for VNerf Bink video playback.
			GConfig->GetBool( TEXT("NerfI.NerfIPlayer"), TEXT("bPlayCredits"), bPlayCredits, TEXT("user") );
			GConfig->GetBool( TEXT("Engine.PlayerPawn"), TEXT("bMatriculate"), bMatriculate, TEXT("user") );
		}

		// Delete is-running semaphore file.
		GFileManager->Delete(TEXT("Running.ini"),0,0);

		// Clean shutdown.
		RemovePropX( *GLogWindow, TEXT("IsBrowser") );
		GLogWindow->Log( NAME_Title, LocalizeGeneral("Exit") );
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

	// Play Bik files.
	if ( ErrorLevel == 0 && bRanAsClient )
	{
		if ( bPlayCredits )
		{
			if ( bMatriculate )
			{
				WinExec( "VNerf w", SW_SHOWDEFAULT );
			}
			else
			{
				WinExec( "VNerf c", SW_SHOWDEFAULT );
			}
		}
		else if ( bMatriculate )
		{
			WinExec( "VNerf w", SW_SHOWDEFAULT );
		}
	}

	return ErrorLevel;
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/