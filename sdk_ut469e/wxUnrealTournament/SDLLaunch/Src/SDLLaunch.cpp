/*=============================================================================
	Launch.cpp: Game launcher.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Daniel Vogel (based on XLaunch).

=============================================================================*/

//#define WX

#ifdef WX

//#define WX_FIRST

#ifdef WX_FIRST
#include "wx.h"

#ifdef __INTEL__
#undef __INTEL__
#endif

#ifdef DECLARE_CLASS
#undef DECLARE_CLASS
#endif

#ifdef DECLARE_ABSTRACT_CLASS
#undef DECLARE_ABSTRACT_CLASS
#endif

#ifdef IMPLEMENT_CLASS
#undef IMPLEMENT_CLASS
#endif

#endif // WX_FIRST

#endif // WX

// System includes
#include "SDLLaunchPrivate.h"

#ifndef FORCE_XOPENGLDRV
#define FORCE_XOPENGLDRV 1
#endif

/*-----------------------------------------------------------------------------
	Global variables.
-----------------------------------------------------------------------------*/

TCHAR GPackageInternal[64]=TEXT("SDLLaunch");
extern "C" {const TCHAR* GPackage=GPackageInternal;}

// Memory allocator.
#include "FMallocImpl.h"
FMallocImpl Malloc;

// Log file.
#include "FOutputDeviceFile.h"
FOutputDeviceFile Log;

// Error handler.
#include "FOutputDeviceSDLError.h"
FOutputDeviceSDLError Error;

// Feedback.
#include "FFeedbackContextSDL.h"
FFeedbackContextSDL Warn;

// File manager.
#include "FFileManagerLinux.h"
FFileManagerLinux FileManager;

// Config.
#include "FConfigCacheIni.h"

#if __STATIC_LINK

// Extra stuff for static links for Engine.
#include "UnEngineNative.h"
#include "UnCon.h"
#include "UnRender.h"
#include "UnNet.h"

// Fire static stuff.
#include "UnFractal.h"

// IpDrv static stuff.
#include "UnIpDrv.h"
#include "UnTcpNetDriver.h"
#include "UnIpDrvCommandlets.h"
#include "UnIpDrvNative.h"
#include "HTTPDownload.h"

// UWeb static stuff.
#include "UWeb.h"
#include "UWebNative.h"

// SDLDrv static stuff.
#include "SDLDrv.h"

// Render static stuff.
#include "Render.h"
#include "UnRenderNative.h"

#include "ALAudio.h"
#include "Cluster.h"

#include "udemoprivate.h"
#include "udemoNative.h"

#if defined(__EMSCRIPTEN__)
	#if FORCE_XOPENGLDRV
		#include "XOpenGLDrv.h"
	#else
		#include "OpenGLDrv.h"
		#include "UTGLROpenGL.h"
	#endif
#else
	#include "XOpenGLDrv.h"
	#include "OpenGLDrv.h"
	#include "UTGLROpenGL.h"
#endif

#if APPLE
# include "FruCoReDrv.h"
#endif

#include "EditorPrivate.h"
#include "UnEditorNative.h"

//#include "NullRender.h"

#endif

// SDL
#include <SDL2/SDL.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#ifdef MACOSX
#include <mach-o/dyld.h>
#endif

#ifdef WX

#include <semaphore.h>

#ifndef WX_FIRST
#undef DECLARE_ABSTRACT_CLASS
#undef DECLARE_CLASS
#undef IMPLEMENT_CLASS
#include "wx.h"
#endif // WX_FIRST
#include "WindowRes.h"

#include "WxStuff.h"

LogWindow LogWin;

// Used to check if the game is already running
sem_t* RunningSemaphore = NULL;

#endif // WX

//SplashScreen
SDL_Renderer*	SplashRenderer = NULL;
SDL_Surface*    SplashImage    = NULL;
SDL_Texture*    SplashTexture  = NULL;
SDL_Window*		SplashWindow   = NULL;

static void InitSplash(const TCHAR *fname)
{
	guard(InitSplash);

	SplashImage = SDL_LoadBMP(appToAnsi(fname));
	if (SplashImage == NULL)
	{
		SDL_ShowSimpleMessageBox(0, "SplashImage init error", SDL_GetError(), SplashWindow);
		return;
	}


	TCHAR WindowName[256];
	// Set viewport window's name to show resolution.
	appSnprintf( WindowName, ARRAY_COUNT(WindowName), TEXT("Unreal Tournament is starting...") );
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

	// stijn: destroying the texture or renderer causes other SDL windows to render only black pixels
	// idk why...

//	if (SplashTexture)
//		SDL_DestroyTexture(SplashTexture);
	if (SplashImage)
		SDL_FreeSurface(SplashImage);
//	if (SplashRenderer)
//		SDL_DestroyRenderer(SplashRenderer);
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
		TEXT("GlideDrv.GlideRenderDevice"),
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

		if( appStricmp(GConfig->GetStr(TEXT("Engine.Engine"),TEXT("ViewportManager"),TEXT("System")),TEXT("WinDrv.WindowsClient"))==0 )
		{
			debugf(TEXT("Your ini had WinDrv...Forcing use of SDLDrv instead."));
			GConfig->SetString(TEXT("Engine.Engine"), TEXT("ViewportManager"), TEXT("SDLDrv.SDLClient"));
		}

		for (INT j = 0; j < ARRAY_COUNT(IniVideoRendererKeys); ++j)
		{
			for (INT i = 0; i < ARRAY_COUNT(LegacyOrWindowsVideoRenderers); ++i)
			{
				if (appStricmp(GConfig->GetStr(TEXT("Engine.Engine"), IniVideoRendererKeys[j], TEXT("System")), LegacyOrWindowsVideoRenderers[i]) == 0)
				{
					debugf(TEXT("Engine.Engine.%s was %s... Forcing use of OpenGLDrv instead."),
						   IniVideoRendererKeys[j],
						   GConfig->GetStr(TEXT("Engine.Engine"), IniVideoRendererKeys[j], TEXT("System")));
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
			if (appStricmp(GConfig->GetStr(TEXT("Engine.Engine"), TEXT("AudioDevice"), TEXT("System")), LegacyOrWindowsAudioRenderers[i]) == 0)
			{
				debugf(TEXT("Engine.Engine.AudioDevice was %s... Forcing use of ALAudio instead."),
					   GConfig->GetStr(TEXT("Engine.Engine"), TEXT("AudioDevice"), TEXT("System")));
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
	DOUBLE LoadTime = appSecondsNew();

	// Set exec hook.
	GExec = NULL;

#ifdef WX
	static FExecHook GLocalHook;
	GLocalHook.Engine = NULL;
	GExec = &GLocalHook;

	LogWin.LogWin = new wxLogWindow(NULL,
		*FString::Printf( LocalizeGeneral("LogWindow",TEXT("Window")), LocalizeGeneral("Product",TEXT("Core")) ),
		LogWin.ShowLog, false);
	LogWin.MyFramePos = new FramePos(TEXT("GameLog"), LogWin.LogWin->GetFrame());
	GLocalHook.LogWin = &LogWin;
#endif // WX

	// Update first-run.
	INT FirstRun=0;
	if (FirstRun<ENGINE_VERSION)
		FirstRun = ENGINE_VERSION;
	GConfig->SetInt( TEXT("FirstRun"), TEXT("FirstRun"), FirstRun );

	FixIni();  // check for legacy and Windows-specific INI entries...

	// Create the global engine object.
	UClass* EngineClass;
	EngineClass = UObject::StaticLoadClass(
		UGameEngine::StaticClass(), NULL, 
		TEXT("ini:Engine.Engine.GameEngine"), 
		NULL, LOAD_NoFail, NULL 
	);
	UEngine* Engine = ConstructObject<UEngine>( EngineClass );
	Engine->Init();

	debugf( TEXT("Startup time: %f seconds."), appSecondsNew()-LoadTime );

#ifdef WX
	GLocalHook.Engine = Engine;

	if (!GCurrentViewport && Engine->Client && Engine->Client->Viewports.Num())
	{
		GCurrentViewport = Engine->Client->Viewports(0);
	}
#endif // WX

	return Engine;
	unguard;
}

/*-----------------------------------------------------------------------------
	Main Loop
-----------------------------------------------------------------------------*/

//
// Exit wound.
// 
int CleanUpOnExit(int ErrorLevel)
{
#ifdef WX
	if (RunningSemaphore)
	{
		sem_close(RunningSemaphore);
		sem_unlink("UnrealTournamentRunningSemaphore");
	}
#endif // WX

	// Clean up our mess.
	GIsRunning = 0;
	/*
	if( GDisplay )
		XCloseDisplay(GDisplay);
	*/
	GFileManager->Delete(TEXT("Running.ini"),0,0);
	debugf( NAME_Title, LocalizeGeneral("Exit") );

	appPreExit();
	GIsGuarded = 0;

	// Shutdown.
	appExit();
	GIsStarted = 0;

	// Better safe than sorry.
	SDL_Quit( );

	#ifdef __EMSCRIPTEN__
	emscripten_cancel_main_loop();  // this should "kill" the app.
	emscripten_force_exit(ErrorLevel);  // this should "kill" the app.
	#endif

	_exit(ErrorLevel);
}

// just in case.  :)  --ryan.
static void sdl_atexit_handler(void)
{
	static bool already_called = false;

	if (!already_called)
	{
		already_called = true;
		SDL_Quit();
	}
}

struct MainLoopArgs
{
	DOUBLE OldTime;
	DOUBLE SecondStartTime;
	INT TickCount;
	UEngine *Engine;
};

static bool MainLoopIteration(MainLoopArgs *args)
{
	guard(MainLoopIteration);

	if ( !GIsRunning || GIsRequestingExit )
	{
		GIsRunning = 0;
		return false;
	}

	try 
	{
		DOUBLE &OldTime = args->OldTime;
		DOUBLE &SecondStartTime = args->SecondStartTime;
		INT &TickCount = args->TickCount;
		UEngine *Engine = args->Engine;

		// Update the world.
		guard(UpdateWorld);
		DOUBLE NewTime = appSecondsNew();
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
	}
	catch (...)
	{
		raise(SIGUSR1);
	}

	return true;
	unguard;
}

#ifdef __EMSCRIPTEN__
static void EmscriptenMainLoopIteration(void *args)
{
	if (!MainLoopIteration((MainLoopArgs *) args))
	{
		CleanUpOnExit(0);
		emscripten_cancel_main_loop();  // this should "kill" the app.
	}
}
#endif

//
// game message loop.
//
static void MainLoop( UEngine* Engine )
{
	guard(MainLoop);
	check(Engine);

#ifndef WX
	// Loop while running.
	GIsRunning = 1;
	MainLoopArgs *args = new MainLoopArgs;
	args->OldTime = appSecondsNew();
	args->SecondStartTime = args->OldTime;
	args->TickCount = 0;
	args->Engine = Engine;

	#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop_arg(EmscriptenMainLoopIteration, args, 0, 1);
	#else
	while (MainLoopIteration(args))
	{
		// Enforce optional maximum tick rate.
		guard(EnforceTickRate);
		const FLOAT MaxTickRate = args->Engine->GetMaxTickRate();
		if( MaxTickRate>0.0 )
		{
			DOUBLE Delta = (1.0/MaxTickRate) - (appSecondsNew()-args->OldTime);
			appSleepLong( Max(0.0,Delta) );
		}
		unguard;
	}
	delete args;
	CleanUpOnExit(0);
	#endif
#endif // WX

	unguard;
}

/*-----------------------------------------------------------------------------
	Main.
-----------------------------------------------------------------------------*/

//
// Simple copy.
// 

void SimpleCopy(TCHAR* fromfile, TCHAR* tofile)
{
	INT c;
	FILE* from;
	FILE* to;
	from = fopen(TCHAR_TO_ANSI(fromfile), "r");
	if (from == NULL)
		return;
	to = fopen(TCHAR_TO_ANSI(tofile), "w");
	if (to == NULL)
	{
		printf("Can't open or create %s", TCHAR_TO_ANSI(tofile));
		return;
	}
	while ((c = getc(from)) != EOF)
		putc(c, to);
	fclose(from);
	fclose(to);
}

void SDL_Init_()
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) == -1)
	{
		const TCHAR *err = appFromAnsi(SDL_GetError());
		appErrorf(TEXT("Couldn't initialize SDL: %s\n"), err);
		appExit();
	}

	atexit(sdl_atexit_handler);
}

#ifdef WX

#include "WxMain.h"

UEngine* wxEngine = NULL;

#endif // WX

int main_(int argc, char* argv[])
{
#if __STATIC_LINK
	// Clean lookups.
	for (INT k=0; k<ARRAY_COUNT(GNativeLookupFuncs); k++)
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

	UEngine* Engine = NULL;

	guard(main); 
	try 
	{ 
		GIsStarted		= 1;

		// Set module name.
		// vogel: FIXME: strip directories from argv[0]
		appStrncpy( GModule, TEXT("UnrealTournament"), ARRAY_COUNT(GModule));

		// Set the package name.
		appStrncpy( const_cast<TCHAR*>(GPackage), appPackage(), 64);	

		// Get the command line.
		FString CmdLine;
		for( INT i=1; i<argc; i++ )
		{
			if( i>1 )
				CmdLine += TEXT(" ");
			CmdLine += ANSI_TO_TCHAR(argv[i]); 
		}

		// Init core.
		GIsClient		= 1; 
		GIsGuarded		= 1;
		appInit( TEXT("UnrealTournament"), *CmdLine, &Malloc, &Log, &Error, &Warn, &FileManager, FConfigCacheIni::Factory, 1 );

#ifndef WX
		SDL_Init_();
#endif // WX
		// Init static classes.
#if __STATIC_LINK
		AUTO_INITIALIZE_REGISTRANTS_ENGINE;
		AUTO_INITIALIZE_REGISTRANTS_EDITOR;
		AUTO_INITIALIZE_REGISTRANTS_SDLDRV;
		AUTO_INITIALIZE_REGISTRANTS_ALAUDIO;
		UClusterAudioSubsystem::StaticClass();
#if defined(__EMSCRIPTEN__)
#if FORCE_XOPENGLDRV
		AUTO_INITIALIZE_REGISTRANTS_XOPENGLDRV;
#else
		AUTO_INITIALIZE_REGISTRANTS_OPENGLDRV;
#endif
#else
		AUTO_INITIALIZE_REGISTRANTS_OPENGLDRV;
		AUTO_INITIALIZE_REGISTRANTS_XOPENGLDRV;
#endif
		//AUTO_INITIALIZE_REGISTRANTS_NULLRENDER;
		AUTO_INITIALIZE_REGISTRANTS_FIRE;
		AUTO_INITIALIZE_REGISTRANTS_IPDRV;
		AUTO_INITIALIZE_REGISTRANTS_UWEB;
		AUTO_INITIALIZE_REGISTRANTS_RENDER;
		AUTO_INITIALIZE_REGISTRANTS_UDEMO;

		InitUdemo();

//!!! FIXME
		char dummyref = 0;
		extern BYTE GLoadedSDLDrv; snprintf(&dummyref, 1, "%d", (int) GLoadedSDLDrv);
#if defined(__EMSCRIPTEN__)
#if FORCE_XOPENGLDRV
		extern BYTE GLoadedXOpenGLDrv; snprintf(&dummyref, 1, "%d", (int) GLoadedXOpenGLDrv);
#else
		extern BYTE GLoadedOpenGLDrv; snprintf(&dummyref, 1, "%d", (int) GLoadedOpenGLDrv);
#endif
#else
		extern BYTE GLoadedOpenGLDrv; snprintf(&dummyref, 1, "%d", (int) GLoadedOpenGLDrv);
		extern BYTE GLoadedXOpenGLDrv; snprintf(&dummyref, 1, "%d", (int) GLoadedXOpenGLDrv);
#endif
//extern BYTE GLoadedNullRender; snprintf(&dummyref, 1, "%d", (int) GLoadedNullRender);
		extern BYTE GLoadedALAudio; snprintf(&dummyref, 1, "%d", (int) GLoadedALAudio);
#endif

		// Init mode.
		GIsServer		= 1;
		GIsClient		= !ParseParam(appCmdLine(), TEXT("SERVER"));
		GIsEditor		= 0;
		GIsScriptable	= 1;
		GLazyLoad		= !GIsClient || ParseParam(appCmdLine(), TEXT("LAZY"));

#ifdef WX
		// Check if we're already running
		RunningSemaphore = sem_open("UnrealTournamentRunningSemaphore", O_CREAT | O_EXCL);
		UBOOL AlreadyRunning = RunningSemaphore == SEM_FAILED && errno == EEXIST;

		// First-run menu.
		INT FirstRun=0;
		GConfig->GetInt( TEXT("FirstRun"), TEXT("FirstRun"), FirstRun );
		if( ParseParam(appCmdLine(),TEXT("FirstRun")) )
			FirstRun=0;

		if( !GIsEditor && GIsClient )
		{
			WConfigWizard D;
			wxWizardPage* Page = NULL;
			if( ParseParam(appCmdLine(),TEXT("safe")) || appStrfind(appCmdLine(),TEXT("readini")) )
			{
//				Page = new WConfigPageSafeMode(&D, nullptr);
//				D.Title=LocalizeGeneral(TEXT("SafeMode"),TEXT("Startup"));
			}
			else if( FirstRun<ENGINE_VERSION || ParseParam(appCmdLine(),TEXT("firsttime")) )
			{
				Page = new WConfigPageRenderer(&D, nullptr);
				D.Title=LocalizeGeneral(TEXT("FirstTime"),TEXT("Startup"));
			}
			else if( ParseParam(appCmdLine(),TEXT("changevideo")) )
			{
				Page = new WConfigPageRenderer(&D, nullptr);
				D.Title=LocalizeGeneral(TEXT("Video"),TEXT("Startup"));
			}
			else if (ParseParam(appCmdLine(), TEXT("changesound")))
			{
				Page = new WConfigPageSound(&D, nullptr);
				D.Title = LocalizeGeneral(TEXT("Audio"), TEXT("Startup"));
			}
			else if( !AlreadyRunning && GFileManager->FileSize(TEXT("Running.ini"))>=0 )
			{
//				Page = new WConfigPageSafeMode(&D, nullptr);
//				D.Title = LocalizeGeneral(TEXT("RecoveryMode"),TEXT("Startup"));
			}
			if( Page )
			{
				//	ExitSplash();
				if (!D.RunWizard( Page ))
					return 0;
			}
		}

		LogWin.AuxOut = GLog;
		GLog = &LogWin;

		// Figure out whether to show log or splash screen.
		UBOOL ShowLog = ParseParam(*CmdLine, TEXT("LOG"));
		LogWin.ShowLog = ShowLog;

		const TCHAR* ConsoleLog = TEXT("ConsoleLog");
#else // WX
		// Init windowing.
		// InitWindowing();

		UBOOL ShowLog = FALSE;
		const TCHAR* ConsoleLog = TEXT("LOG");
#endif // WX

		FString Filename = FString::Printf(TEXT("../Help/Splash%i.bmp"),((int)time(NULL)) % 5);
		if( GFileManager->FileSize(*Filename)<0 )
			Filename = FString(TEXT("../Help")) * TEXT("Logo.bmp");
		if( GFileManager->FileSize(*Filename)<0 )
			Filename = TEXT("../Help/Logo.bmp");

		// Init splash screen.
		if (!ShowLog && GFileManager->FileSize(*Filename) > 0)
			InitSplash(*Filename );
	
		// Init console log.
		if (ParseParam(*CmdLine, ConsoleLog))
		{
			Warn.AuxOut	= GLog;
			GLog		= &Warn;
		}

		// Init engine.
		Engine = InitEngine();
		if( Engine )
		{
			debugf( NAME_Title, LocalizeGeneral("Run") );

			// Remove splash screen.
			ExitSplash();

			// Optionally Exec and exec file.
			FString Temp;
			if( Parse(*CmdLine, TEXT("EXEC="), Temp) )
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
		Error.HandleError();
	}

#ifndef WX
	if (Engine)
	{
		// Start main engine loop.
		debugf( TEXT("Entering main loop.") );
		if ( !GIsRequestingExit )
			MainLoop( Engine );
	}
#else // WX
	wxEngine = Engine;
#endif // WX

	unguard;

	return -1;
}

#ifdef WX

int wxUnrealTournament::OnRun()
{
	int ret = main_(argc, argv);

	if (ret != -1)
		return ret;

	// Start main engine loop.
	debugf( TEXT("Entering main loop.") );
	if ( !GIsRequestingExit )
	{
		// Loop while running.
		GIsRunning = 1;
		Args = new MainLoopArgs;
		Args->OldTime = appSecondsNew();
		Args->SecondStartTime = Args->OldTime;
		Args->TickCount = 0;
		Args->Engine = wxEngine;

		// Tick the game engine whenever possible
		Connect(wxID_ANY, wxEVT_IDLE, wxIdleEventHandler(wxUnrealTournament::Tick));
	}

	return wxApp::OnRun();
}

wxIMPLEMENT_CLASS(wxUnrealTournament, wxApp);
wxIMPLEMENT_APP_NO_MAIN(wxUnrealTournament);
wxIMPLEMENT_WX_THEME_SUPPORT;

#endif // WX

//
// Entry point.
//
int main( int argc, char* argv[] )
{
	Malloc.Init();
	GMalloc = &Malloc;

	if (!appChdirSystem())
		wprintf(TEXT("WARNING: Could not chdir into the game's System folder\n"));

#ifdef WX
	//
	// stijn: SDL needs to initialize _BEFORE_ wxWidgets. Otherwise, everything
	// will blow up.
	//
	SDL_Init_();

	// Start wxWindow
	wxInitAllImageHandlers();
	wxEntryStart(argc, argv);
	wxTheApp->CallOnInit();
	wxTheApp->OnRun();
	wxEntryCleanup();
#else // WX
	main_(argc, argv);
#endif // WX

	return 0;
}

