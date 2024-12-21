/*=============================================================================
	SDL2Launch.cpp: Game launcher.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Daniel Vogel (based on XLaunch).
	* Updated to SDL2 with some improvements by Smirftsch
=============================================================================*/

// System includes
#include "SDL2LaunchPrivate.h"
#include "FConfigCacheIni.h"

/*-----------------------------------------------------------------------------
	Global variables.
-----------------------------------------------------------------------------*/

#if _MSC_VER
	//#include <time.h>

	extern "C" {HINSTANCE hInstance;}

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
#else
    TCHAR GPackageInternal[64]=TEXT("SDL2Launch");
    extern "C" {const TCHAR* GPackage=GPackageInternal;}


	#include <sys/time.h>

	// Memory allocator.
	#include "FMallocAnsi.h"
	FMallocAnsi Malloc;

	// Log file.
	#include "FOutputDeviceFile.h"
	FOutputDeviceFile Log;

	// Error handler.
	#include "FOutputDeviceAnsiError.h"
	FOutputDeviceAnsiError Error;

	// Feedback.
	#include "FFeedbackContextAnsi.h"
	FFeedbackContextAnsi Warn;

	// File manager.
	#include "FFileManagerLinux.h"
	FFileManagerLinux FileManager;
#endif

// Config.
//#include "FConfigCacheIni.h"

#if __STATIC_LINK

  // Extra stuff for static links for Engine.
  #include "UnFont.h"
  #include "UnEngineNative.h"
  #include "UnRender.h"
  #include "UnRenderNative.h"
  #include "UnNet.h"

  // UPak static stuff.
  #include "UPak.h"
  #include "UnUPakNative.h"

  // PathLogic static stuff.
  #include "PathLogicClasses.h"
  #include "UnPathLogicNative.h"

  // NullNetDriver static stuff.
  // #include "NullNetDriver.h"

  // NullRender static stuff.
  // #include "NullRender.h"
  // #include "NullRenderNative.h"

  // Fire static stuff.
  #include "UnFractal.h"
//  #include "FireClasses.h"
  #include "UnFluidSurface.h"
  #include "UnProcMesh.h"
  #include "UnFireNative.h"

  // IpDrv static stuff.
  #include "UnIpDrv.h"
  #include "UnTcpNetDriver.h"
  #include "UnIpDrvCommandlets.h"
  #include "UnIpDrvNative.h"
  #include "HTTPDownload.h"

  // Editor static stuff.
  #include "EditorPrivate.h"
  #include "UnEditorNative.h"

  // Emitter static stuff.
  #include "EmitterPrivate.h"
  #include "UnEmitterNative.h"

  // UWeb static stuff.
  #include "UWebAdminPrivate.h"
  #include "UnUWebAdminNative.h"

  // SDL2Drv static stuff.
  #include "SDLDrv.h"

  // Rendering/Audio
  #include "ALAudio.h"
  #include "XOpenGLDrv.h"
  #include "OpenGLDrv.h"
  #include "OpenGL.h"

#if MACOSX
# include "FruCoRe.h"
#endif

  #include "PhysXPhysics.h"
  #include "UnPhysXPhysicsNative.h"

#endif

// SDL
#include "SDL.h"

//SplashScreen
SDL_Renderer*	SplashRenderer;
SDL_Surface*	    SplashImage;
SDL_Texture*	    SplashTexture;
SDL_Window*		SplashWindow;

/*-----------------------------------------------------------------------------
	Initialization
-----------------------------------------------------------------------------*/

static void InitSplash( const TCHAR* Filename )
{
    guard(InitSplash);

	//SDL_Surface * image = SDL_LoadBMP("../Help/<Filename>"); //FIXME add support for Filename!!!
	SplashImage = SDL_LoadBMP("../Help/Logo.bmp");
	if (SplashImage == NULL)
	{
		SDL_ShowSimpleMessageBox(0, "SplashImage init error", SDL_GetError(), SplashWindow);
		return;
	}

	TCHAR WindowName[256];
	// Set viewport window's name to show resolution.
	appSprintf( WindowName, TEXT("Unreal is starting...") );
	SplashWindow = SDL_CreateWindow(appToAnsi(WindowName),
                          SDL_WINDOWPOS_CENTERED,
                          SDL_WINDOWPOS_CENTERED,
                          SplashImage->w, SplashImage->h,
                          SDL_WINDOW_BORDERLESS);

	if (SplashWindow == NULL)
	{
        SDL_ShowSimpleMessageBox(0, "SplashWindow init error", SDL_GetError(), SplashWindow);
		return;
	}

	SplashRenderer = SDL_CreateRenderer(SplashWindow, -1, 0);
	if (SplashRenderer == NULL)
	{
		SDL_ShowSimpleMessageBox(0, "SplashRenderer init error", SDL_GetError(), SplashWindow);
		return;
	}

	SplashTexture = SDL_CreateTextureFromSurface(SplashRenderer, SplashImage);
	if (SplashTexture == NULL)
	{
		SDL_ShowSimpleMessageBox(0, "SplashTexture init error", SDL_GetError(), SplashWindow);
		return;
	}
    SDL_RenderClear(SplashRenderer);
    SDL_RenderCopy(SplashRenderer, SplashTexture, NULL, NULL);
    SDL_RenderPresent(SplashRenderer);

    unguard;
}

static void ExitSplash()
{
	guard(ExitSplash);

	//if (SplashTexture)
	//	SDL_DestroyTexture(SplashTexture);
	if (SplashImage)
        SDL_FreeSurface(SplashImage);
	// stijn: you can't destroy this! it will get reused by the main window
	//if (SplashRenderer)
    //    SDL_DestroyRenderer(SplashRenderer);
	if (SplashWindow)
		SDL_DestroyWindow(SplashWindow);

    unguard;
}

static inline void FixIni()
{
	const TCHAR* IniVideoRendererKeys[] =
	{
		TEXT("RenderDevice"),
		TEXT("GameRenderDevice"),
		TEXT("WindowRenderDevice")
	};

	const TCHAR* LegacyOrWindowsVideoRenderers[] =
	{
        TEXT("D3DDrv.D3DRenderDevice"),
		TEXT("D3D8Drv.D3D8RenderDevice"),
		TEXT("D3D9Drv.D3D9RenderDevice"),
		TEXT("D3D10Drv.D3D10RenderDevice"),
		TEXT("D3D11Drv.D3D11RenderDevice"),
//		TEXT("GlideDrv.GlideRenderDevice"),
		TEXT("SDLGLDrv.SDLGLRenderDevice"),
		TEXT("SoftDrv.SoftwareRenderDevice"),
		TEXT("SDLSoftDrv.SDLSoftwareRenderDevice")
	};

	const TCHAR* LegacyOrWindowsAudioRenderers[] =
	{
		TEXT("Audio.GenericAudioSubsystem"),
		TEXT("Galaxy.GalaxyAudioSubsystem"),
	};

    // A default config? Force it from WinDrv to SDLDrv... --ryan.
    //  Also clean up legacy Loki interfaces...
	if( !ParseParam(appCmdLine(),TEXT("NoForceSDLDrv")) )
    {
#ifdef __EMSCRIPTEN__
# if FORCE_XOPENGLDRV
		GConfig->SetString(TEXT("Engine.Engine"), TEXT("GameRenderDevice"), TEXT("XOpenGLDrv.XOpenGLRenderDevice"));
		GConfig->SetString(TEXT("Engine.Engine"), TEXT("WindowRenderDevice"), TEXT("XOpenGLDrv.XOpenGLRenderDevice"));
		GConfig->SetString(TEXT("Engine.Engine"), TEXT("RenderDevice"), TEXT("XOpenGLDrv.XOpenGLRenderDevice"));
# else
		GConfig->SetString(TEXT("Engine.Engine"), TEXT("GameRenderDevice"), TEXT("OpenGLDrv.OpenGLRenderDevice"));
		GConfig->SetString(TEXT("Engine.Engine"), TEXT("WindowRenderDevice"), TEXT("OpenGLDrv.OpenGLRenderDevice"));
		GConfig->SetString(TEXT("Engine.Engine"), TEXT("RenderDevice"), TEXT("OpenGLDrv.OpenGLRenderDevice"));
# endif
#else

	    if( appStricmp(*GConfig->GetStr(TEXT("Engine.Engine"),TEXT("ViewportManager"),TEXT("System")),TEXT("WinDrv.WindowsClient"))==0 )
        {
            debugf(TEXT("Your ini had WinDrv...Forcing use of SDLDrv instead."));
			GConfig->SetString(TEXT("Engine.Engine"), TEXT("ViewportManager"), TEXT("SDLDrv.SDLClient"));
        }

		for (INT j = 0; j < ARRAY_COUNT(IniVideoRendererKeys); ++j)
		{
			for (INT i = 0; i < ARRAY_COUNT(LegacyOrWindowsVideoRenderers); ++i)
			{
				if (appStricmp(*GConfig->GetStr(TEXT("Engine.Engine"), IniVideoRendererKeys[j], TEXT("System")), LegacyOrWindowsVideoRenderers[i]) == 0)
				{
					debugf(TEXT("Engine.Engine.%ls was %ls... Forcing use of OpenGLDrv instead."),
						   IniVideoRendererKeys[j],
						   *GConfig->GetStr(TEXT("Engine.Engine"), IniVideoRendererKeys[j], TEXT("System")));
					GConfig->SetString(TEXT("Engine.Engine"), IniVideoRendererKeys[j], TEXT("OpenGLDrv.OpenGLRenderDevice"));
					break;
				}
			}
		}
#endif
    }

	if( !ParseParam(appCmdLine(),TEXT("NoForceALAudio")) )
    {
		for (INT i = 0; i < ARRAY_COUNT(LegacyOrWindowsAudioRenderers); ++i)
		{
			if (appStricmp(*GConfig->GetStr(TEXT("Engine.Engine"), TEXT("AudioDevice"), TEXT("System")), LegacyOrWindowsAudioRenderers[i]) == 0)
			{
				debugf(TEXT("Engine.Engine.AudioDevice was %ls... Forcing use of ALAudio instead."),
					   *GConfig->GetStr(TEXT("Engine.Engine"), TEXT("AudioDevice"), TEXT("System")));
				GConfig->SetString(TEXT("Engine.Engine"), TEXT("AudioDevice"), TEXT("ALAudio.ALAudioSubsystem"));
				break;
			}
		}
    }
}

//
// Creates a UEngine object.
//
static UEngine* InitEngine()
{
	guard(InitEngine);
	FTime LoadTime = appSeconds();

	// Set exec hook.
	GExec = NULL;

	// First-run menu.
	INT FirstRun=0;
	GConfig->GetInt( TEXT("FirstRun"), TEXT("FirstRun"), FirstRun );
	if( ParseParam(appCmdLine(),TEXT("FirstRun")) )
		FirstRun=0;

	FixIni();

	if( FirstRun<227 )
	{
		// Migrate savegames.
		TArray<FString> Saves = GFileManager->FindFiles( TEXT("..\\Save\\*.usa"), 1, 0 );
		for( TArray<FString>::TIterator It(Saves); It; ++It )
		{
			INT Pos = appAtoi(**It+4);
			FString Section = TEXT("UnrealShare.UnrealSlotMenu");
			FString Key     = FString::Printf(TEXT("SlotNames[%i]"),Pos);
			if( appStricmp(*GConfig->GetStr(*Section,*Key,TEXT("user")),TEXT(""))==0 )
				GConfig->SetString(*Section,*Key,TEXT("Saved game"),TEXT("user"));
		}
	}

	// Update first-run.
	if( FirstRun<ENGINE_VERSION )
		FirstRun = ENGINE_VERSION;
	GConfig->SetInt( TEXT("FirstRun"), TEXT("FirstRun"), FirstRun );

	// Create the global engine object.
	UClass* EngineClass;
	EngineClass = UObject::StaticLoadClass(
		UGameEngine::StaticClass(), NULL,
		TEXT("ini:Engine.Engine.GameEngine"),
		NULL, LOAD_NoFail, NULL
	);
	UEngine* Engine = ConstructObject<UEngine>( EngineClass );
	Engine->Init();

	debugf( TEXT("Startup time: %f seconds."), appSeconds()-LoadTime );

	return Engine;
	unguard;
}

/*-----------------------------------------------------------------------------
	Main Loop
-----------------------------------------------------------------------------*/
//
// game message loop.
//
static void MainLoop( UEngine* Engine )
{
	guard(MainLoop);
	check(Engine);

	// Loop while running.
	GIsRunning = 1;
	FTime OldTime = appSeconds();
	FTime SecondStartTime = OldTime;
	INT TickCount = 0;
	STAT(QWORD LastCycles = appCycles());
	while( GIsRunning && !GIsRequestingExit )
	{
		// Update the world.
		guard(UpdateWorld);
		FTime NewTime = appSeconds();
		FLOAT  DeltaTime = NewTime - OldTime;
		Engine->Tick( DeltaTime );
		if( GWindowManager )
			GWindowManager->Tick( DeltaTime );
		OldTime = NewTime;
		TickCount++;
		if( OldTime > SecondStartTime + 1 )
		{
			Engine->CurrentTickRate = (FLOAT)TickCount / (OldTime - SecondStartTime);
			SecondStartTime = OldTime;
			TickCount = 0;
		}
		unguard;

		// Enforce optional maximum tick rate.
		guard(EnforceTickRate);
		FLOAT MaxTickRate = Engine->GetMaxTickRate();
		if( MaxTickRate>0.0 )
		{
			FLOAT Delta = (1.0/MaxTickRate) - (appSeconds()-OldTime);
			appSleep( Max(0.f,Delta) );
		}
		unguard;

#if STATS
		guard(TickStats);
		QWORD NewCycles = appCycles();
		GStatFPS.FPS.Time = NewCycles - LastCycles;
		++GStatFPS.FrameCount.Count;
		LastCycles = NewCycles;
		FStatGroup::Tick();
		unguard;
#endif
	}
	GIsRunning = 0;
	unguardf((TEXT("(ClientCam Pos=(%g,%g,%g) Rot=(%i,%i,%i))"), _ClientCameraLocation.X, _ClientCameraLocation.Y, _ClientCameraLocation.Z, _ClientCameraRotation.Pitch, _ClientCameraRotation.Yaw, _ClientCameraRotation.Roll));
}
/*-----------------------------------------------------------------------------
	Main.
-----------------------------------------------------------------------------*/

//
// Simple copy.
//

void SimpleCopy(char* fromfile, char* tofile)
{
	INT c;
	FILE* from;
	FILE* to;
	from = fopen(fromfile, "r");
	if (from == NULL)
		return;
	to = fopen(tofile, "w");
	if (to == NULL)
	{
		printf("Can't open or create %s", tofile);
		return;
	}
	while ((c = getc(from)) != EOF)
		putc(c, to);
	fclose(from);
	fclose(to);
}

//
// Exit wound.
//
int CleanUpOnExit(int ErrorLevel)
{
	// Clean up our mess.
	GIsRunning = 0;
	/*
	if( GDisplay )
		XCloseDisplay(GDisplay);
	*/
	GFileManager->Delete(TEXT("Running.ini"),0,0);
	debugf( NAME_Title, *LocalizeGeneral(TEXT("Exit")) );

	appPreExit();
	GIsGuarded = 0;

	// Shutdown.
	appExit();
	GIsStarted = 0;

	// Better safe than sorry.
	SDL_Quit( );

	_exit(ErrorLevel);
}

CORE_API void appArgv0(const char *argv0);

//
// Entry point.
//
#if _MSC_VER
INT WINAPI WinMain( HINSTANCE hInInstance, HINSTANCE hPrevInstance, char*, INT nCmdShow )
{
	// Remember instance.
	INT ErrorLevel = 0;
	GIsStarted     = 1;
	hInstance      = hInInstance;
	const TCHAR* CmdLine = GetCommandLine();
	appStrcpy( GPackage, appPackage() );

	// Begin guarded code.
#ifndef _DEBUG
	try
	{
#endif
		// Init core.
		GIsClient = GIsGuarded = 1;
		appInit( GPackage, CmdLine, &Malloc, &Log, &Error, &Warn, &FileManager, FConfigCacheIni::Factory, 1 );

		// Init mode.
		GIsServer     = 1;
		GIsClient     = !ParseParam(appCmdLine(),TEXT("SERVER"));
		GIsEditor     = 0;
		GIsScriptable = 1;
		GLazyLoad     = !GIsClient || ParseParam(appCmdLine(),TEXT("LAZY"));

		// Init SDL. Need to init it very early, otherwise no splash.
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) == -1)
		{
			const TCHAR *err = appFromAnsi(SDL_GetError());
			appErrorf(TEXT("Couldn't initialize SDL: %ls\n"), err);
			appExit();
		}

		// Figure out whether to show log or splash screen.
		UBOOL ShowLog = ParseParam(CmdLine,TEXT("LOG"));
		INT RunCount = appAtoi(*GConfig->GetStr(TEXT("Engine.Engine"),TEXT("RunCount")));
		FString Filename = FString::Printf(TEXT("..\\Help\\Splash%i.bmp"),RunCount%5);
		GConfig->SetString(TEXT("Engine.Engine"),TEXT("RunCount"),*FString::Printf(TEXT("%i"),RunCount+1));
		Parse(CmdLine,TEXT("Splash="),Filename);
		if( GFileManager->FileSize(*Filename)<0 )
			Filename = FString(TEXT("..\\Help")) * TEXT("Logo.bmp");
		if( GFileManager->FileSize(*Filename)<0 )
			Filename = TEXT("..\\Help\\Logo.bmp");
		appStrcpy( GPackage, appPackage() );

		if( !ShowLog && !ParseParam(CmdLine,TEXT("server")) && !appStrfind(CmdLine,TEXT("TestRenDev")) )
			InitSplash( *Filename );

		/*
		// Init windowing.
		InitWindowing();

		// Create log window, but only show it if ShowLog.
		GLogWindow = new WLog( Log.Filename, Log.LogAr, TEXT("GameLog") );
		GLogWindow->OpenWindow( ShowLog, 0 );
		GLogWindow->Log( NAME_Title, LocalizeGeneral("Start") );
		if( GIsClient )
			SetPropX( *GLogWindow, TEXT("IsBrowser"), (HANDLE)1 );
		*/

		// Init engine.
		UEngine* Engine = InitEngine();
		if( Engine )
		{
			//GLogWindow->Log( NAME_Title, LocalizeGeneral("Run") );

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

			// Optionally Exec an exec file
			FString Temp;
			if( Parse(CmdLine, TEXT("EXEC="), Temp) )
			{
				Temp = FString(TEXT("exec ")) + Temp;
				if( Engine->Client && Engine->Client->Viewports.Num() && Engine->Client->Viewports(0) )
					Engine->Client->Viewports(0)->Exec( *Temp, *GLog );
			}

			// Start main engine loop, including the Windows message pump.
			if( !GIsRequestingExit )
				MainLoop( Engine );
		}

		// Clean shutdown.
		GFileManager->Delete(TEXT("Running.ini"),0,0);
//		GLogWindow->Log( NAME_Title, LocalizeGeneral("Exit") );
//		delete GLogWindow;

		//if(GIsClient)
		//	GRenderDevice->ResetGamma(); //Ensure GammaReset is called on Renderer before really exiting.

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
#else
int main( int argc, char* argv[] )
{
    // Memory allocator. stijn: needs to be initialized early now because we use FStrings everywhere
    Malloc.Init();
	GMalloc = &Malloc;

	if (!appChdirSystem())
		wprintf(TEXT("WARNING: Could not chdir into the game's System folder\n"));

	guard(main);
	UEngine* Engine = nullptr;
	try
	{
		GIsStarted		    = 1;

		{ // Set module name.
			//appStrcpy( GModule, "Unreal" );
			//strcpy( GModule, argv[0] );
			FString ModuleStr(FString::GetFilenameOnlyStr(ANSI_TO_TCHAR(argv[0]))); // Strip path and extension from module name.
			appStrncpy( GModule, *ModuleStr, ARRAY_COUNT(GModule));
		}

		// Get the command line.
		TCHAR* CommandLine = appStaticString1024();
		{
			char CmdLine[1024];
			CmdLine[0] = '\x0';
			for( INT i=1; i<argc; i++ )
			{
				if( i>1 )
					strcat( CmdLine, " " );
				strcat( CmdLine, "- " );
				strcat( CmdLine, argv[i] );
			}
			appStrcpy(CommandLine,ANSI_TO_TCHAR(CmdLine));
		}


		//wprintf(TEXT("CommandLine %ls %i,%i"),CommandLine,appStrlen(CommandLine), appAtoi(CommandLine));

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
		GNativeLookupFuncs[i++] = &FindEngineUPX_VehicleBaseNative;
		GNativeLookupFuncs[i++] = &FindEngineUPX_VehicleWheeledNative;
		GNativeLookupFuncs[i++] = &FindEngineUPXJ_BaseJointNative;
		GNativeLookupFuncs[i++] = &FindEngineUPhysicsAnimationNative;
		GNativeLookupFuncs[i++] = &FindEngineURenderIteratorNative;
		GNativeLookupFuncs[i++] = &FindEngineAVolumeNative;
		GNativeLookupFuncs[i++] = &FindEngineAInventoryNative;
		GNativeLookupFuncs[i++] = &FindEngineAProjectileNative;
		GNativeLookupFuncs[i++] = &FindEngineUClientPreloginSceneNative;
		GNativeLookupFuncs[i++] = &FindEngineUServerPreloginSceneNative;
		GNativeLookupFuncs[i++] = &FindEngineUAnimationNotifyNative;
		GNativeLookupFuncs[i++] = &FindEngineUIK_LipSyncNative;		

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

		// Init core.
		GIsClient		= 1;
		GIsGuarded		= 1;
#ifdef __LINUX__
		appInit( TEXT("UnrealLinux"), CommandLine, &Malloc, &Log, &Error, &Warn, &FileManager, FConfigCacheIni::Factory, 1 );
#elif MACOSX || MACOSXPPC
		appInit( TEXT("UnrealOSX"), CommandLine, &Malloc, &Log, &Error, &Warn, &FileManager, FConfigCacheIni::Factory, 1 );
#endif

#if __STATIC_LINK
		AUTO_INITIALIZE_REGISTRANTS_ENGINE;
		AUTO_INITIALIZE_REGISTRANTS_FIRE;
		AUTO_INITIALIZE_REGISTRANTS_EMITTER;
		AUTO_INITIALIZE_REGISTRANTS_UPAK;
		AUTO_INITIALIZE_REGISTRANTS_PATHLOGIC;
		AUTO_INITIALIZE_REGISTRANTS_EDITOR;
		AUTO_INITIALIZE_REGISTRANTS_IPDRV;
		AUTO_INITIALIZE_REGISTRANTS_UWEBADMIN;
		AUTO_INITIALIZE_REGISTRANTS_SDLDRV;
		AUTO_INITIALIZE_REGISTRANTS_ALAUDIO;
		AUTO_INITIALIZE_REGISTRANTS_OPENGLDRV;
		AUTO_INITIALIZE_REGISTRANTS_XOPENGLDRV;
#if MACOSX
		AUTO_INITIALIZE_REGISTRANTS_FRUCORE;
#endif
		AUTO_INITIALIZE_REGISTRANTS_RENDER;
		AUTO_INITIALIZE_REGISTRANTS_PHYSXPHYSICS;
//		AUTO_INITIALIZE_REGISTRANTS_NULLRENDER;

		// stijn: lld is a bit too eager at discarding GLoaded symbols.
		// This hack forces lld to keep said symbols around
		extern BYTE GLoadedSDLDrv;
		extern BYTE GLoadedXOpenGLDrv;
		extern BYTE GLoadedOpenGLDrv;
		extern BYTE GLoadedALAudio;
		extern BYTE GLoadedRender;
		extern BYTE GLoadedPhysXPhysics;
//		extern BYTE GLoadedNullRender;

		volatile BYTE Dummy;
		Dummy = GLoadedSDLDrv;
		Dummy = GLoadedXOpenGLDrv;
		Dummy = GLoadedOpenGLDrv;
		Dummy = GLoadedALAudio;
		Dummy = GLoadedRender;
		Dummy = GLoadedPhysXPhysics;
		//Dummy = GLoadedNullRender;
#endif

		// Init mode.
		GIsServer		= 1;
		GIsClient		= !ParseParam(appCmdLine(), TEXT("SERVER"));
		GIsEditor		= 0;
		GIsScriptable	= 1;
		GLazyLoad		= !GIsClient || ParseParam(appCmdLine(), TEXT("LAZY"));


		// Init SDL. Need to init it very early, otherwise no splash.
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) == -1)
		{
			const TCHAR *err = appFromAnsi(SDL_GetError());
			appErrorf(TEXT("Couldn't initialize SDL: %ls\n"), err);
			appExit();
		}

		// Init windowing.
		// InitWindowing();

		UBOOL ShowLog = ParseParam(appCmdLine(),TEXT("LOG"));
		UBOOL Splash = 0;
		INT RunCount = appAtoi(*GConfig->GetStr(TEXT("Engine.Engine"),TEXT("RunCount")));
		FString Filename = FString::Printf(TEXT("..\\Help\\Splash%i.bmp"),RunCount%5);
		GConfig->SetString(TEXT("Engine.Engine"),TEXT("RunCount"),*FString::Printf(TEXT("%i"),RunCount+1));
		Parse(appCmdLine(),TEXT("Splash="),Filename);
		if( GFileManager->FileSize(*Filename)<0 )
			Filename = FString(TEXT("..\\Help")) * TEXT("Logo.bmp");
		if( GFileManager->FileSize(*Filename)<0 )
			Filename = TEXT("..\\Help\\Logo.bmp");

		// Init splash screen.
		if( !ShowLog && !ParseParam(appCmdLine(),TEXT("server")) && !appStrfind(appCmdLine(),TEXT("TestRenDev")) )
		{
			InitSplash( *Filename );
			Splash=1;
		}
		else GWarn->Logf(TEXT("Splash screen disabled when using the parameters log, server and TestRenDev"));

		// Init console log.
		if (ShowLog)
		{
			Warn.AuxOut	= GLog;
			GLog		= &Warn;
		}

		// Init engine.
		Engine = InitEngine();
		if( Engine )
		{
			debugf( NAME_Title, *LocalizeGeneral(TEXT("Run")) );

			if (Splash)
				ExitSplash();

			// Optionally Exec and exec file.
			FString Temp;
			if( Parse(CommandLine, TEXT("EXEC="), Temp) )
			{
				Temp = FString(TEXT("exec ")) + Temp;
				if( Engine->Client && Engine->Client->Viewports.Num() && Engine->Client->Viewports(0) )
					Engine->Client->Viewports(0)->Exec( *Temp, *GLog );
			}
		}
	}
	catch (...)
	{
		// Chained abort.  Do cleanup.
		// raise(SIGUSR1);
		Error.HandleError();
	}

	if (Engine)
	{
		// Start main engine loop.
		debugf( TEXT("Entering main loop.") );
		if ( !GIsRequestingExit )
			MainLoop( Engine );

//		if( Engine->Client && Engine->Client->Viewports.Num() && Engine->Client->Viewports(0) )
//			Engine->Client->Viewports(0)->Destroy();
	}

    unguard;

	CleanUpOnExit(0);
    return(0);
}
#endif
