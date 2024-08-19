/*=============================================================================
	Launch.cpp: Game launcher.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Daniel Vogel (based on XLaunch).

=============================================================================*/

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
#include "FMallocAnsi.h"
FMallocAnsi Malloc;

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

#include <semaphore.h>

#ifndef WX_FIRST
#include "wx.h"
#endif // WX_FIRST
#include "WindowRes.h"

#include "WxStuff.h"

LogWindow LogWin;

// Used to check if the game is already running
sem_t* RunningSemaphore = NULL;

// SplashScreen
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
//        SDL_DestroyRenderer(SplashRenderer);
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
static UEngine* InitEngine() // XXX
{
	guard(InitEngine);
	DOUBLE LoadTime = appSecondsNew();

	// Set exec hook.
	GExec = NULL;
	static FExecHook GLocalHook;
	GLocalHook.Engine = NULL;
	GExec = &GLocalHook;

	LogWin.LogWin = new wxLogWindow(NULL,
		*FString::Printf( LocalizeGeneral("LogWindow",TEXT("Window")), LocalizeGeneral("Product",TEXT("Core")) ),
		LogWin.ShowLog, false);
	LogWin.MyFramePos = new FramePos(TEXT("GameLog"), LogWin.LogWin->GetFrame());
	GLocalHook.LogWin = &LogWin;

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

	GLocalHook.Engine = Engine;

	if (!GCurrentViewport && Engine->Client && Engine->Client->Viewports.Num())
    {
        GCurrentViewport = Engine->Client->Viewports(0);
    }

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
	if (RunningSemaphore)
	{
		sem_close(RunningSemaphore);
		sem_unlink("UnrealTournamentRunningSemaphore");
	}

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

#define WINDOW_API

//
// Interface for accepting commands.
//
class WINDOW_API FCommandTarget
{
public:
	virtual void Unused() {}
};

//
// Delegate function pointers.
//
typedef void(FCommandTarget::*TDelegate)();
typedef void(FCommandTarget::*TDelegateInt)(INT);

//
// Simple bindings to an object and a member function of that object.
//
struct WINDOW_API FDelegate
{
	FCommandTarget* TargetObject;
	void (FCommandTarget::*TargetInvoke)();
	FDelegate( FCommandTarget* InTargetObject=NULL, TDelegate InTargetInvoke=NULL )
	: TargetObject( InTargetObject )
	, TargetInvoke( InTargetInvoke )
	{}
	virtual void operator()() { if( TargetObject ) (TargetObject->*TargetInvoke)(); }
};
struct WINDOW_API FDelegateInt
{
	FCommandTarget* TargetObject;
	void (FCommandTarget::*TargetInvoke)(int);
	FDelegateInt( FCommandTarget* InTargetObject=NULL, TDelegateInt InTargetInvoke=NULL )
	: TargetObject( InTargetObject )
	, TargetInvoke( InTargetInvoke )
	{}
	virtual void operator()( int I ) { if( TargetObject ) (TargetObject->*TargetInvoke)(I); }
};


// Forward declarations
class WConfigWizard;


class WRadioButton : public wxRadioButton
{
	DECLARE_CLASS(WRadioButton)
	DECLARE_EVENT_TABLE()

private:
	FDelegate OnClickDelegate;

public:
	WRadioButton(wxWindow* Parent, wxWindowID Id, FString Text, FDelegate InClicked=FDelegate(), UBOOL FirstOfGroup=FALSE)
		: wxRadioButton(Parent, Id, *Text, wxDefaultPosition, wxDefaultSize, FirstOfGroup ? wxRB_GROUP : 0)
	{
		OnClickDelegate = InClicked;
	}

	void OnClick(wxCommandEvent& Event)
	{
		OnClickDelegate();
	}
};
IMPLEMENT_CLASS(WRadioButton, wxRadioButton)

BEGIN_EVENT_TABLE(WRadioButton, wxRadioButton)
EVT_RADIOBUTTON(wxID_ANY, WRadioButton::OnClick)
END_EVENT_TABLE()


class WButton : public wxButton
{
	DECLARE_CLASS(WButton)
	DECLARE_EVENT_TABLE()

private:
	FDelegate OnClickDelegate;

public:
	WButton(wxWindow* Parent, wxWindowID Id, FString Text, FDelegate InClicked=FDelegate())
	: wxButton(Parent, Id, *Text)
	{
		OnClickDelegate = InClicked;
	}

	void OnClick(wxCommandEvent& Event)
	{
		OnClickDelegate();
	}

	void SetVisibleText(const TCHAR* Text)
	{
		SetLabel(Text ? Text : wxT(""));
		Show(Text != nullptr);
	}
};
IMPLEMENT_CLASS(WButton, wxButton);

BEGIN_EVENT_TABLE(WButton, wxButton)
EVT_BUTTON(wxID_ANY, WButton::OnClick)
END_EVENT_TABLE()

class WTextLabel : public wxStaticText
{
	DECLARE_CLASS(WTextLabel)

public:
	WTextLabel(wxWindow* Parent, wxWindowID Id, FString Text)
	: wxStaticText(Parent, Id, *Text)
	{
	}
};
IMPLEMENT_CLASS(WTextLabel, wxStaticText);

class WImageLabel : public wxStaticBitmap
{
	DECLARE_CLASS(WImageLabel)

public:
	WImageLabel(wxWindow* Parent, wxWindowID Id, FString BitmapFile)
	: wxStaticBitmap(Parent, Id, wxBitmap(*BitmapFile, wxBITMAP_TYPE_BMP))
	{
	}
};
IMPLEMENT_CLASS(WImageLabel, wxStaticBitmap);

class WListBox : public wxListBox
{
	DECLARE_CLASS(WListBox)
	DECLARE_EVENT_TABLE()

private:
	FDelegate OnClickDelegate;

public:
	WListBox(wxWindow* Parent, wxWindowID Id, long Style, FDelegate InClicked=FDelegate())
	: wxListBox(Parent, Id, wxDefaultPosition, wxDefaultSize, 0, nullptr, Style)
	{
		OnClickDelegate = InClicked;
	}

	void OnClick(wxCommandEvent& Event)
	{
		OnClickDelegate();
	}
};
IMPLEMENT_CLASS(WListBox, wxListBox);

BEGIN_EVENT_TABLE(WListBox, wxListBox)
EVT_LISTBOX(wxID_ANY, WListBox::OnClick)
END_EVENT_TABLE()

class WEdit : public wxTextCtrl
{
	DECLARE_CLASS(WEdit)

	// stijn: wxTE_RICH2
	WEdit(wxWindow* Parent, wxWindowID Id)
	: wxTextCtrl(Parent, Id, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY)
	{
	}

	//	void OpenWindow( UBOOL Visible, UBOOL Multiline, UBOOL ReadOnly, UBOOL HorizScroll = FALSE, UBOOL NoHideSel = FALSE )
};

IMPLEMENT_CLASS(WEdit, wxTextCtrl)

class WConfigPage : public wxWizardPage
{
	DECLARE_CLASS(WConfigPage)

public:
	WConfigWizard* Owner;
	WImageLabel* LogoStatic;
	wxWizardPage* PrevPage, *NextPage;
	wxBoxSizer* VertSizer;

	WConfigPage(WConfigWizard* InOwner, wxWizardPage* InPrev)
		: wxWizardPage((wxWizard*)InOwner)
		, Owner(InOwner)
		, PrevPage(InPrev)
	{
		NextPage = nullptr;
		VertSizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(VertSizer);

		LogoStatic = new WImageLabel(this, IDC_Logo, TEXT("../Help/Logo.bmp"));
		VertSizer->Add(LogoStatic, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 2);
		VertSizer->AddSpacer(1);
	}

	wxWizardPage* GetNext() const
	{
		return NextPage;
	}

	wxWizardPage* GetPrev() const
	{
		return PrevPage;
	}

	virtual void OnNext()
	{
	}
};
IMPLEMENT_CLASS(WConfigPage, wxWizardPage)

class WConfigPageFirstTime : public WConfigPage
{
	DECLARE_CLASS(WConfigPageFirstTime)

public:
	WTextLabel* FirstTimePrompt;

	WConfigPageFirstTime(WConfigWizard* InOwner, wxWizardPage* InPrev)
		: WConfigPage(InOwner, InPrev)
	{
		FirstTimePrompt = new WTextLabel(this, IDC_Prompt, Localize(TEXT("IDDIALOG_ConfigPageFirstTime"), TEXT("IDC_PromptSDL"), TEXT("Startup")));
		FirstTimePrompt->Wrap(640);
		// ugly hax to center the text vertically
		VertSizer->AddSpacer((((wxWizard*)Owner)->GetSize() - FirstTimePrompt->GetSize()).GetHeight()/2);
		VertSizer->Add(FirstTimePrompt, 0, wxALL, 10);
	}
};
IMPLEMENT_CLASS(WConfigPageFirstTime, WConfigPage)

class WConfigPageDetail : public WConfigPage
{
	DECLARE_CLASS(WConfigPageDetail)

public:
	WTextLabel* DetailPrompt, *DetailNote;
	WEdit* DetailEdit;

	WConfigPageDetail(WConfigWizard* InOwner, wxWizardPage* InPrev)
		: WConfigPage(InOwner, InPrev)
	{
		DetailPrompt = new WTextLabel(this, IDC_DetailPrompt, Localize(TEXT("IDDIALOG_ConfigPageDetail"), TEXT("IDC_DetailPrompt"), TEXT("Startup")));
		DetailPrompt->Wrap(640);
		VertSizer->Add(DetailPrompt, 0, wxALL, 10);

		DetailEdit = new WEdit(this, IDC_DetailEdit);
		VertSizer->Add(DetailEdit, 1, wxEXPAND | wxALL, 10);

		DetailNote = new WTextLabel(this, IDC_DetailNote, Localize(TEXT("IDDIALOG_ConfigPageDetail"), TEXT("IDC_DetailNote"), TEXT("Startup")));
		DetailNote->Wrap(640);
		VertSizer->Add(DetailNote, 0, wxALL, 10);

		FString Info;
		INT DescFlags=0;
		FString Driver = GConfig->GetStr(TEXT("Engine.Engine"),TEXT("GameRenderDevice"));
		GConfig->GetInt(*Driver,TEXT("DescFlags"),DescFlags);

		// Sound quality.
		if (!ParseParam(appCmdLine(), TEXT("changevideo")))
			Info = Info + LocalizeGeneral(TEXT("SoundHigh"),TEXT("Startup")) + TEXT("\r\n");

		// Skins.
		if (!ParseParam(appCmdLine(), TEXT("changesound")))
		{
			if( (DescFlags&RDDESCF_LowDetailSkins) )
			{
				Info = Info + LocalizeGeneral(TEXT("SkinsLow"),TEXT("Startup")) + TEXT("\r\n");
				GConfig->SetString( TEXT("SDLDrv.SDLClient"), TEXT("SkinDetail"), TEXT("Medium") );
			}
			else
			{
				Info = Info + LocalizeGeneral(TEXT("SkinsHigh"),TEXT("Startup")) + TEXT("\r\n");
			}

			// World.
			if( (DescFlags&RDDESCF_LowDetailWorld) )
			{
				Info = Info + LocalizeGeneral(TEXT("WorldLow"),TEXT("Startup")) + TEXT("\r\n");
				GConfig->SetString( TEXT("SDLDrv.SDLClient"), TEXT("TextureDetail"), TEXT("Medium") );
			}
			else
			{
				Info = Info + LocalizeGeneral(TEXT("WorldHigh"),TEXT("Startup")) + TEXT("\r\n");
			}

			// Resolution.
			if( (DescFlags&RDDESCF_LowDetailWorld) )
			{
				Info = Info + LocalizeGeneral(TEXT("ResLow"),TEXT("Startup")) + TEXT("\r\n");
				GConfig->SetString( TEXT("SDLDrv.SDLClient"), TEXT("WindowedViewportX"),  TEXT("512") );
				GConfig->SetString( TEXT("SDLDrv.SDLClient"), TEXT("WindowedViewportY"),  TEXT("384") );
				GConfig->SetString( TEXT("SDLDrv.SDLClient"), TEXT("WindowedColorBits"),  TEXT("16") );
				GConfig->SetString( TEXT("SDLDrv.SDLClient"), TEXT("FullscreenViewportX"), TEXT("512") );
				GConfig->SetString( TEXT("SDLDrv.SDLClient"), TEXT("FullscreenViewportY"), TEXT("384") );
				GConfig->SetString( TEXT("SDLDrv.SDLClient"), TEXT("FullscreenColorBits"), TEXT("16") );
			}
			else
			{
				Info = Info + LocalizeGeneral(TEXT("ResHigh"),TEXT("Startup")) + TEXT("\r\n");
			}
		}
		DetailEdit->Clear();
		DetailEdit->AppendText(*Info);

		NextPage = new WConfigPageFirstTime(Owner, this);
	}
};

IMPLEMENT_CLASS(WConfigPageDetail, WConfigPage)

class WConfigPageSound : public WConfigPage, public FCommandTarget
{
	DECLARE_CLASS(WConfigPageSound)

public:
	WListBox* SoundList;
	WRadioButton* ShowCompatible, *ShowAll;
	WTextLabel *SoundNote, *SoundPrompt;
	INT First;
	TArray<FRegistryObjectInfo> Classes;
	FString CurrentDriverCached;

	void RefreshList()
	{
		SoundList->Clear();
		INT All=ShowAll->GetValue(), BestPriority=0;
		FString Default;
		Classes.Empty();
		UObject::GetRegistryObjects( Classes, UClass::StaticClass(), UAudioSubsystem::StaticClass(), 0 );
		for( TArray<FRegistryObjectInfo>::TIterator It(Classes); It; ++It )
		{
			FString Path=It->Object, Left, Right, Temp;
			if( Path.Split(TEXT("."),&Left,&Right) )
			{
				INT DoShow=All, Priority=0;
				INT DescFlags=0;
				if( Path==TEXT("ALAudio.ALAudioSubsystem") ||
					Path==TEXT("Cluster.ClusterAudioSubsystem") )
					DoShow = Priority = 1;
				if( DoShow )
				{
					SoundList->Append( *(Temp=Localize(*Right,TEXT("ClassCaption"),*Left)) );
					if( Priority>=BestPriority )
						{Default=Temp; BestPriority=Priority;}
				}
			}
		}
		if( Default!=TEXT("") )
			SoundList->SetStringSelection(*Default);
		CurrentChange();
	}

	void CurrentChange()
	{
		SoundNote->SetLabel(Localize(TEXT("Descriptions"),*CurrentDriver(),TEXT("Startup"),NULL,1));
		SoundNote->Wrap(640);
		CurrentDriverCached=CurrentDriver();
	}

	void OnNext()
	{
		if( CurrentDriverCached!=TEXT("") )
			GConfig->SetString(TEXT("Engine.Engine"),TEXT("AudioDevice"),*CurrentDriverCached);
	}

	FString CurrentDriver()
	{
		INT Selection = SoundList->GetSelection();
		if( Selection != wxNOT_FOUND )
		{
			FString Name = SoundList->GetString(Selection).wc_str();
			for( TArray<FRegistryObjectInfo>::TIterator It(Classes); It; ++It )
			{
				FString Path=It->Object, Left, Right, Temp;
				if( Path.Split(TEXT("."),&Left,&Right) )
					if( Name==Localize(*Right,TEXT("ClassCaption"),*Left) )
						return Path;
			}
		}
		return TEXT("");
	}

	WConfigPageSound(WConfigWizard* InOwner, wxWizardPage* InPrev)
		: WConfigPage(InOwner, InPrev)
	{
		// Prompt
		SoundPrompt = new WTextLabel(this, IDC_SoundPrompt, Localize(TEXT("IDDIALOG_ConfigPageSound"), TEXT("IDC_SoundPrompt"), TEXT("Startup")));
		SoundPrompt->Wrap(640);
		VertSizer->Add(SoundPrompt, 1, wxLEFT | wxRIGHT, 10);

		// Renderer List
		SoundList = new WListBox(this, IDC_SoundList, wxLB_SINGLE | wxLB_NEEDED_SB, FDelegate(this, (TDelegate)&WConfigPageSound::CurrentChange));
		SoundList->SetMaxSize(wxSize(640,300));
		VertSizer->Add(SoundList, 4, wxEXPAND | wxALL, 10);

		// Combo
		wxBoxSizer* HorSizer = new wxBoxSizer(wxHORIZONTAL);
		VertSizer->Add(HorSizer, 0, wxEXPAND | wxALL, 10);
		ShowCompatible = new WRadioButton(this, IDC_Compatible, Localize(TEXT("IDDIALOG_ConfigPageSound"), TEXT("IDC_Compatible"), TEXT("Startup")), FDelegate(this, (TDelegate)&WConfigPageSound::RefreshList));
		ShowAll = new WRadioButton(this, IDC_All, Localize(TEXT("IDDIALOG_ConfigPageSound"), TEXT("IDC_All"), TEXT("Startup")), FDelegate(this, (TDelegate)&WConfigPageSound::RefreshList));
		HorSizer->Add(ShowCompatible, 1);
		HorSizer->Add(ShowAll, 1);

		// Note
		SoundNote = new WTextLabel(this, IDC_SoundNote, Localize(TEXT("Descriptions"),*CurrentDriver(),TEXT("Startup"),NULL,1));
		VertSizer->Add(SoundNote, 2, wxEXPAND | wxALL, 10);

		NextPage = new WConfigPageDetail(Owner, this);

		RefreshList();
	}
};
IMPLEMENT_CLASS(WConfigPageSound, wxWizardPage);


class WConfigPageRenderer : public WConfigPage, public FCommandTarget
{
	DECLARE_CLASS(WConfigPageRenderer)

public:
	WListBox* RenderList;
	WRadioButton* ShowCompatible, * ShowAll;
	WTextLabel *RenderNote, *RenderPrompt;
	INT First;
	TArray<FRegistryObjectInfo> Classes;
	FString CurrentDriverCached;

	void RefreshList()
	{
		RenderList->Clear();
		INT All=ShowAll->GetValue(), BestPriority=0;
		FString Default;
		Classes.Empty();
		UObject::GetRegistryObjects( Classes, UClass::StaticClass(), URenderDevice::StaticClass(), 0 );
		for( TArray<FRegistryObjectInfo>::TIterator It(Classes); It; ++It )
		{
			FString Path=It->Object, Left, Right, Temp;
			if( Path.Split(TEXT("."),&Left,&Right) )
			{
				INT DoShow=All, Priority=0;
				INT DescFlags=0;
				GConfig->GetInt(*Path,TEXT("DescFlags"),DescFlags);
				if( DescFlags & RDDESCF_Certified )
					DoShow = Priority = 2;
				else if( Path==TEXT("OpenGLDrv.OpenGLRenderDevice") ||
						 Path==TEXT("XOpenGLDrv.XOpenGLRenderDevice") )
					DoShow = Priority = 1;
				if( DoShow )
				{
					RenderList->Append( *(Temp=Localize(*Right,TEXT("ClassCaption"),*Left)) );
					if( Priority>=BestPriority )
						{Default=Temp; BestPriority=Priority;}
				}
			}
		}
		if( Default!=TEXT("") )
			RenderList->SetStringSelection(*Default);
		CurrentChange();
	}

	void CurrentChange()
	{
		RenderNote->SetLabel(Localize(TEXT("Descriptions"),*CurrentDriver(),TEXT("Startup"),NULL,1));
		RenderNote->Wrap(640);
		CurrentDriverCached=CurrentDriver();
	}

	void OnNext()
	{
		if( CurrentDriverCached!=TEXT("") )
			GConfig->SetString(TEXT("Engine.Engine"),TEXT("GameRenderDevice"),*CurrentDriverCached);
	}

	FString CurrentDriver()
	{
		INT Selection = RenderList->GetSelection();
		if( Selection != wxNOT_FOUND )
		{
			FString Name = RenderList->GetString(Selection).wc_str();
			for( TArray<FRegistryObjectInfo>::TIterator It(Classes); It; ++It )
			{
				FString Path=It->Object, Left, Right, Temp;
				if( Path.Split(TEXT("."),&Left,&Right) )
					if( Name==Localize(*Right,TEXT("ClassCaption"),*Left) )
						return Path;
			}
		}
		return TEXT("");
	}

	WConfigPageRenderer(WConfigWizard* InOwner, wxWizardPage* InPrev)
		: WConfigPage(InOwner, InPrev)
	{
		// Prompt
		RenderPrompt = new WTextLabel(this, IDC_RenderPrompt, Localize(TEXT("IDDIALOG_ConfigPageRenderer"), TEXT("IDC_RenderPrompt"), TEXT("Startup")));
		RenderPrompt->Wrap(640);
		VertSizer->Add(RenderPrompt, 1, wxLEFT | wxRIGHT, 10);

		// Renderer List
		RenderList = new WListBox(this, IDC_RenderList, wxLB_SINGLE | wxLB_NEEDED_SB, FDelegate(this, (TDelegate)&WConfigPageRenderer::CurrentChange));
		RenderList->SetMaxSize(wxSize(640,300));
		VertSizer->Add(RenderList, 4, wxEXPAND | wxALL, 10);

		// Combo
		wxBoxSizer* HorSizer = new wxBoxSizer(wxHORIZONTAL);
		VertSizer->Add(HorSizer, 0, wxEXPAND | wxALL, 10);
		ShowCompatible = new WRadioButton(this, IDC_Compatible, Localize(TEXT("IDDIALOG_ConfigPageRenderer"), TEXT("IDC_Compatible"), TEXT("Startup")), FDelegate(this, (TDelegate)&WConfigPageRenderer::RefreshList));
		ShowAll = new WRadioButton(this, IDC_All, Localize(TEXT("IDDIALOG_ConfigPageRenderer"), TEXT("IDC_All"), TEXT("Startup")), FDelegate(this, (TDelegate)&WConfigPageRenderer::RefreshList));
		HorSizer->Add(ShowCompatible, 1);
		HorSizer->Add(ShowAll, 1);

		// Note
		RenderNote = new WTextLabel(this, IDC_RenderNote, Localize(TEXT("Descriptions"),*CurrentDriver(),TEXT("Startup"),NULL,1));
		VertSizer->Add(RenderNote, 2, wxEXPAND | wxALL, 10);

		// if this is a firsttime run, show the sound page too
		if (!ParseParam(appCmdLine(), TEXT("changevideo")))
			NextPage = new WConfigPageSound(Owner, this);
		else
			NextPage = new WConfigPageDetail(Owner, this);

		RefreshList();
	}
};
IMPLEMENT_CLASS(WConfigPageRenderer, wxWizardPage);

class WConfigWizard : public wxWizard
{
	DECLARE_CLASS(WConfigWizard)
	DECLARE_EVENT_TABLE()

public:
	wxBoxSizer* BoxSizer;
	UBOOL Cancel;
	FString Title;
	WConfigWizard()
		: wxWizard(NULL, wxID_ANY)
		, BoxSizer(NULL)
		, Cancel(0)
	{
		SetIcon(wxIcon(wxT("../Help/Unreal.ico")));
	}

	bool RunWizard(wxWizardPage* Page)
	{
		SetTitle(*Title);
		GetPageAreaSizer()->Add(Page);
		return wxWizard::RunWizard(Page);
	}

	void OnNext(wxWizardEvent& Event)
	{
		WConfigPage* CurrentPage = (WConfigPage*)GetCurrentPage();
		if (CurrentPage)
			CurrentPage->OnNext();
	}
};
IMPLEMENT_CLASS(WConfigWizard, wxWizard)

BEGIN_EVENT_TABLE(WConfigWizard, wxWizard)
EVT_WIZARD_PAGE_CHANGING(wxID_ANY, WConfigWizard::OnNext)
END_EVENT_TABLE()

class wxUnrealTournament : public wxApp
{
	wxDECLARE_CLASS(wxUnrealTournament);

private:
	WConfigPageRenderer* Page;
	MainLoopArgs* Args;

public:
	bool OnInit();
	int OnRun();
	int OnExit();
	void Tick(wxIdleEvent& Event);
};

bool wxUnrealTournament::OnInit()
{
	Args = nullptr;
	Page = nullptr;
	return true;
}

int wxUnrealTournament::OnRun()
{
#if __STATIC_LINK
	// Clean lookups.
	for (INT k=0; k<ARRAY_COUNT(GNativeLookupFuncs); k++)
	{
		GNativeLookupFuncs[k] = NULL;
	}

	// Core lookups.
	GNativeLookupFuncs[0] = &FindCoreUObjectNative;
	GNativeLookupFuncs[1] = &FindCoreUCommandletNative;

	// Engine lookups.
	GNativeLookupFuncs[2] = &FindEngineAActorNative;
	GNativeLookupFuncs[3] = &FindEngineAPawnNative;
	GNativeLookupFuncs[4] = &FindEngineAPlayerPawnNative;
	GNativeLookupFuncs[5] = &FindEngineADecalNative;
	GNativeLookupFuncs[6] = &FindEngineAStatLogNative;
	GNativeLookupFuncs[7] = &FindEngineAStatLogFileNative;
	GNativeLookupFuncs[8] = &FindEngineAZoneInfoNative;
	GNativeLookupFuncs[9] = &FindEngineAWarpZoneInfoNative;
	GNativeLookupFuncs[10] = &FindEngineALevelInfoNative;
	GNativeLookupFuncs[11] = &FindEngineAGameInfoNative;
	GNativeLookupFuncs[12] = &FindEngineANavigationPointNative;
	GNativeLookupFuncs[13] = &FindEngineUCanvasNative;
	GNativeLookupFuncs[14] = &FindEngineUConsoleNative;
	GNativeLookupFuncs[15] = &FindEngineUScriptedTextureNative;

	GNativeLookupFuncs[16] = &FindIpDrvAInternetLinkNative;
	GNativeLookupFuncs[17] = &FindIpDrvAUdpLinkNative;
	GNativeLookupFuncs[18] = &FindIpDrvATcpLinkNative;

	// UWeb lookups.
	GNativeLookupFuncs[19] = &FindUWebUWebResponseNative;
	GNativeLookupFuncs[20] = &FindUWebUWebRequestNative;

	// udemo lookups.
	GNativeLookupFuncs[21] = &FindudemoUUZHandlerNative;
	GNativeLookupFuncs[22] = &FindudemoUudnativeNative;
	GNativeLookupFuncs[23] = &FindudemoUDemoInterfaceNative;
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

		// Init static classes.
#if __STATIC_LINK
		AUTO_INITIALIZE_REGISTRANTS_ENGINE;
		AUTO_INITIALIZE_REGISTRANTS_SDLDRV;
		AUTO_INITIALIZE_REGISTRANTS_ALAUDIO;
		AUTO_INITIALIZE_REGISTRANTS_CLUSTER;
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
//				D.Title=LocalizeGeneral(TEXT("RecoveryMode"),TEXT("Startup"));
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

		FString Filename = FString::Printf(TEXT("../Help/Splash%i.bmp"),((int)time(NULL)) % 5);
		if( GFileManager->FileSize(*Filename)<0 )
			Filename = FString(TEXT("../Help")) * TEXT("Logo.bmp");
		if( GFileManager->FileSize(*Filename)<0 )
			Filename = TEXT("../Help/Logo.bmp");

		// Init splash screen.
		if (!ShowLog && GFileManager->FileSize(*Filename) > 0)
			InitSplash(*Filename );

		// Init console log.
		if (ParseParam(*CmdLine, TEXT("ConsoleLog")))
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
    unguard;

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
		Args->Engine = Engine;

		// Tick the game engine whenever possible
		Connect(wxID_ANY, wxEVT_IDLE, wxIdleEventHandler(wxUnrealTournament::Tick));
	}

	return wxApp::OnRun();
}

void wxUnrealTournament::Tick(wxIdleEvent& Event)
{
	if (MainLoopIteration(Args))
	{
		// Enforce optional maximum tick rate.
		guard(EnforceTickRate);
		const FLOAT MaxTickRate = Args->Engine->GetMaxTickRate();
		if( MaxTickRate>0.0 )
		{
			FLOAT Delta = (1.0/MaxTickRate) - (appSecondsNew()-Args->OldTime);
			appSleep( Max(0.f,Delta) );
		}
		unguard;

        // draw wx gui here if we have one
		Event.RequestMore();
	}
	else
	{
		Disconnect(wxEVT_IDLE, wxIdleEventHandler(wxUnrealTournament::Tick));
		wxTheApp->OnExit();
		wxTheApp->Exit();
	}
}

int wxUnrealTournament::OnExit()
{
	delete Args;
	GExec = NULL;
	CleanUpOnExit(0);
	return wxApp::OnExit();
}

wxIMPLEMENT_CLASS(wxUnrealTournament, wxApp);
wxIMPLEMENT_APP_NO_MAIN(wxUnrealTournament);
wxIMPLEMENT_WX_THEME_SUPPORT;

//
// Entry point.
//
int main( int argc, char* argv[] )
{
	Malloc.Init();
	GMalloc = &Malloc;

	appChdirSystem();

	//
	// stijn: SDL needs to initialize _BEFORE_ wxWidgets. Otherwise, everything
	// will blow up.
	//
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) == -1)
	{
		const TCHAR *err = appFromAnsi(SDL_GetError());
		appErrorf(TEXT("Couldn't initialize SDL: %s\n"), err);
		appExit();
	}

	atexit(sdl_atexit_handler);

	// Start wxWindow
	wxInitAllImageHandlers();
	wxEntryStart(argc, argv);
	wxTheApp->CallOnInit();
	wxTheApp->OnRun();
	wxEntryCleanup();

    return(0);
}
