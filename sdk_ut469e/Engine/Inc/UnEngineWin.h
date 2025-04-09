/*=============================================================================
	UnEngineWin.h: Unreal engine windows-specific code.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
	* Created by Tim Sweeney.
=============================================================================*/

/*-----------------------------------------------------------------------------
	Splash screen.
-----------------------------------------------------------------------------*/

//
// Splash screen, implemented with old-style Windows code so that it
// can be opened super-fast before initialization.
//
BOOL CALLBACK SplashDialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if( uMsg==WM_DESTROY )
		PostQuitMessage(0);
	return 0;
}
static HWND    hWndSplash = NULL;
static HBITMAP hBitmap    = NULL;
static INT     BitmapX    = 0;
static INT     BitmapY    = 0;
static DWORD   ThreadId   = 0;
static HANDLE  hThread    = 0;

// stijn: Does DPI scaling for an image that we want to use for STM_SETIMAGE
void ScaleImage(HBITMAP& Bitmap, INT& ImageWidth, INT& ImageHeight, HWND InHwnd)
{
	HDC hdc = GetDC(InHwnd);
	INT LogX = GetDeviceCaps(hdc, LOGPIXELSX);
	INT LogY = GetDeviceCaps(hdc, LOGPIXELSY);

	if (LogX != 96 || LogY != 96)
	{
		ImageWidth = MulDiv(ImageWidth, LogX, 96);
		ImageHeight = MulDiv(ImageHeight, LogY, 96);
		Bitmap = reinterpret_cast<HBITMAP>(CopyImage(Bitmap, IMAGE_BITMAP, ImageWidth, ImageHeight, LR_COPYDELETEORG));
	}

	ReleaseDC(InHwnd, hdc);
}

DWORD WINAPI ThreadProc( VOID* Parm )
{
	hWndSplash = CreateDialogW(hInstance,MAKEINTRESOURCEW(IDDIALOG_Splash), NULL, reinterpret_cast<DLGPROC>(SplashDialogProc));
	if( hWndSplash )
	{
		HWND hWndLogo = GetDlgItem(hWndSplash,IDC_Logo);
		if( hWndLogo )
		{
			ScaleImage(hBitmap, BitmapX, BitmapY, hWndLogo);
			SetWindowPos(hWndSplash,HWND_TOPMOST,(GetSystemMetrics(SM_CXSCREEN)-BitmapX)/2,(GetSystemMetrics(SM_CYSCREEN)-BitmapY)/2,BitmapX,BitmapY,SWP_SHOWWINDOW);
			SetWindowPos(hWndSplash,HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
			SendMessageW( hWndLogo, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap );
			UpdateWindow( hWndSplash );
			MSG Msg;
			while(GetMessageW(&Msg,NULL,0,0))
				DispatchMessageW(&Msg);
		}
	}
	return 0;
}
void InitSplash( const TCHAR* Filename )
{
	FWindowsBitmap Bitmap(1);
	if( Filename )
	{
		UBOOL Loaded = FALSE;
		HDC hdc = GetDC(NULL);
		INT LogX = GetDeviceCaps(hdc, LOGPIXELSX);
		INT LogY = GetDeviceCaps(hdc, LOGPIXELSY);
		ReleaseDC(NULL, hdc);

		FString SplashName;
		INT Scale = 1;
		for (INT i = LogX != 96 || LogY != 96 ? 0 : 2; i < 3; i++)
		{
			SplashName = Filename;
			Scale = 1.0f;
			if (SplashName.Right(4) == TEXT(".bmp") && i < 2)
			{
				SplashName = SplashName.Left(SplashName.Len() - 4) += (i == 0 ? TEXT("-x4.bmp") : TEXT("-x2.bmp"));
				Scale = (i == 0 ? 4 : 2);
			}
			Loaded = Bitmap.LoadFile(*SplashName);
			if (Loaded)
				break;
		}
		if (!Loaded)
		{
			if (GFileManager && GFileManager->FileSize(Filename) <= 0 && 
				GFileManager->FileSize(TEXT("..\\Textures\\Palettes.utx")) <= 0 &&
				GFileManager->FileSize(TEXT("..\\Maps\\Entry.unr")) <= 0)
				appErrorf(TEXT("Game data not found. Please re-apply the patch to an existing installation of the full game."));
			appErrorf(TEXT("Could not load splash logo: %ls"), Filename);
		}
		hBitmap = Bitmap.GetBitmapHandle();
		BitmapX = Bitmap.SizeX/Scale;
		BitmapY = Bitmap.SizeY/Scale;
	}
	hThread=CreateThread(NULL,0,&ThreadProc,NULL,0,&ThreadId);
}
void ExitSplash()
{
	if( ThreadId )
		PostThreadMessageW(ThreadId,WM_QUIT,0,0);
}

/*-----------------------------------------------------------------------------
	System Directories.
-----------------------------------------------------------------------------*/

TCHAR SysDir[MAX_PATH]=TEXT(""), WinDir[MAX_PATH]=TEXT(""), ThisFile[MAX_PATH]=TEXT("");
void InitSysDirs()
{
	GetSystemDirectory( SysDir, ARRAY_COUNT(SysDir) );
	GetWindowsDirectory( WinDir, ARRAY_COUNT(WinDir) );
	GetModuleFileName( NULL, ThisFile, ARRAY_COUNT(ThisFile) );
	if( appStrlen(ThisFile) > 4 && !appStricmp( &ThisFile[appStrlen(ThisFile) - 4], TEXT(".ICD") ) )
		appStrcpy( &ThisFile[appStrlen(ThisFile) - 4], TEXT(".EXE") ); // stijn: mem safety ok
}

/*-----------------------------------------------------------------------------
	Config wizard.
-----------------------------------------------------------------------------*/

class WConfigWizard : public WWizardDialog
{
	DECLARE_WINDOWCLASS(WConfigWizard,WWizardDialog,Startup)
	WLabel LogoStatic;
	FWindowsBitmap LogoBitmap;
	UBOOL Cancel;
	FString Title;
	WConfigWizard()
	: LogoStatic(this,IDC_Logo)
	, Cancel(0)
	{
		InitSysDirs();
	}
	void OnInitDialog()
	{
		guard(WStartupWizard::OnInitDialog);
		WWizardDialog::OnInitDialog();
		SendMessageW( *this, WM_SETICON, ICON_BIG, (WPARAM)LoadIconW(hInstance,MAKEINTRESOURCEW(IDICON_Mainframe)) );
		LogoBitmap.LoadFile( TEXT("..\\Help\\Logo.bmp") );
		SendMessageW( LogoStatic, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)LogoBitmap.GetBitmapHandle() );
		SetText( *Title );
		SetForegroundWindow( hWnd );
		unguard;
	}
};

class WConfigPageFirstTime : public WWizardPage
{
	DECLARE_WINDOWCLASS(WConfigPageFirstTime,WWizardPage,Startup)
	WConfigWizard* Owner;
	WConfigPageFirstTime( WConfigWizard* InOwner )
	: WWizardPage( TEXT("ConfigPageFirstTime"), IDDIALOG_ConfigPageFirstTime, InOwner )
	, Owner(InOwner)
	{}
	const TCHAR* GetNextText()
	{
		return LocalizeGeneral(TEXT("Run"),TEXT("Startup"));
	}
	WWizardPage* GetNext()
	{
		Owner->EndDialog(1);
		return NULL;
	}	
};

class WConfigPageSafeOptions : public WWizardPage
{
	DECLARE_WINDOWCLASS(WConfigPageSafeOptions,WWizardPage,Startup)
	WConfigWizard* Owner;
	WButton NoSoundButton, No3DSoundButton, No3DVideoButton, WindowButton, ResButton, ResetConfigButton, NoProcessorButton, NoJoyButton;
	WConfigPageSafeOptions( WConfigWizard* InOwner )
	: WWizardPage		( TEXT("ConfigPageSafeOptions"), IDDIALOG_ConfigPageSafeOptions, InOwner )
	, Owner				(InOwner)
	, NoSoundButton		(this,IDC_NoSound)
	, No3DSoundButton	(this,IDC_No3DSound)
	, No3DVideoButton	(this,IDC_No3dVideo)
	, WindowButton		(this,IDC_Window)
	, ResButton			(this,IDC_Res)
	, ResetConfigButton	(this,IDC_ResetConfig)
	, NoProcessorButton	(this,IDC_NoProcessor)
	, NoJoyButton		(this,IDC_NoJoy)
	{}
	void OnInitDialog()
	{
		WWizardPage::OnInitDialog();
		SendMessageW( NoSoundButton,     BM_SETCHECK, 1, 0 );
		SendMessageW( No3DSoundButton,   BM_SETCHECK, 1, 0 );
		SendMessageW( No3DVideoButton,   BM_SETCHECK, 1, 0 );
		SendMessageW( WindowButton,      BM_SETCHECK, 1, 0 );
		SendMessageW( ResButton,         BM_SETCHECK, 1, 0 );
		SendMessageW( ResetConfigButton, BM_SETCHECK, 0, 0 );
		SendMessageW( NoProcessorButton, BM_SETCHECK, 1, 0 );
		SendMessageW( NoJoyButton,       BM_SETCHECK, 1, 0 );
	}
	const TCHAR* GetNextText()
	{
		return LocalizeGeneral(TEXT("Run"),TEXT("Startup"));
	}
	WWizardPage* GetNext()
	{
		FString CmdLine;
		if( SendMessageW(NoSoundButton,BM_GETCHECK,0,0)==BST_CHECKED )
			CmdLine+=TEXT(" -nosound");
		if( SendMessageW(No3DSoundButton,BM_GETCHECK,0,0)==BST_CHECKED )
			CmdLine+=TEXT(" -no3dsound");
		if( SendMessageW(No3DVideoButton,BM_GETCHECK,0,0)==BST_CHECKED )
			CmdLine+=TEXT(" -nohard");
		if( SendMessageW(WindowButton,BM_GETCHECK,0,0)==BST_CHECKED )
			CmdLine+=TEXT(" -nohard -noddraw");
		if( SendMessageW(ResButton,BM_GETCHECK,0,0)==BST_CHECKED )
			CmdLine+=TEXT(" -defaultres");
		if( SendMessageW(NoProcessorButton,BM_GETCHECK,0,0)==BST_CHECKED )
			CmdLine+=TEXT(" -nommx -nokni -nok6");
		if( SendMessageW(NoJoyButton,BM_GETCHECK,0,0)==BST_CHECKED )
			CmdLine+=TEXT(" -nojoy");
		if( SendMessageW(ResetConfigButton,BM_GETCHECK,0,0)==BST_CHECKED )
			GFileManager->Delete( *(FString(appPackage())+TEXT(".ini")) );
		ShellExecuteW(NULL, TEXT("open"), ThisFile, *CmdLine, appBaseDir(), SW_SHOWNORMAL );
		Owner->EndDialog(0);
		return NULL;
	}
};

class WConfigPageDetail : public WWizardPage
{
	DECLARE_WINDOWCLASS(WConfigPageDetail,WWizardPage,Startup)
	WConfigWizard* Owner;
	WEdit DetailEdit;
	WConfigPageDetail( WConfigWizard* InOwner )
	: WWizardPage( TEXT("ConfigPageDetail"), IDDIALOG_ConfigPageDetail, InOwner )
	, Owner(InOwner)
	, DetailEdit(this,IDC_DetailEdit)
	{}
	void OnInitDialog()
	{
		WWizardPage::OnInitDialog();
		FString Info;

		INT DescFlags=0;
		INT FirstRun=0;
		FString Driver = GConfig->GetStr(TEXT("Engine.Engine"),TEXT("GameRenderDevice"));
		GConfig->GetInt(*Driver,TEXT("DescFlags"),DescFlags);
		GConfig->GetInt(TEXT("FirstRun"), TEXT("FirstRun"), FirstRun);

		// Frame rate dependent LOD.
		if( Driver==TEXT("SoftDrv.SoftwareRenderDevice") || 280.0*1000.0*1000.0*GSecondsPerCycle>1.f )
		{
			GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("MinDesiredFrameRate"), TEXT("20") );
		}
		else if( Driver==TEXT("D3DDrv.D3DRenderDevice") )
		{
			GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("MinDesiredFrameRate"), TEXT("28") );
		}

		// Sound quality.
		Info = Info + LocalizeGeneral(TEXT("SoundHigh"),TEXT("Startup")) + TEXT("\r\n");

		// Skins.
		if( (GPhysicalMemory < 96*1024*1024) || (DescFlags&RDDESCF_LowDetailSkins) )
		{
			Info = Info + LocalizeGeneral(TEXT("SkinsLow"),TEXT("Startup")) + TEXT("\r\n");
			GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("SkinDetail"), TEXT("Medium") );
		}
		else
		{
			Info = Info + LocalizeGeneral(TEXT("SkinsHigh"),TEXT("Startup")) + TEXT("\r\n");
		}

		// World.
		if( (GPhysicalMemory < 64*1024*1024) || (DescFlags&RDDESCF_LowDetailWorld) )
		{
			Info = Info + LocalizeGeneral(TEXT("WorldLow"),TEXT("Startup")) + TEXT("\r\n");
			GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetail"), TEXT("Medium") );
		}
		else
		{
			Info = Info + LocalizeGeneral(TEXT("WorldHigh"),TEXT("Startup")) + TEXT("\r\n");
		}

		// Resolution.
		if( (!GIsMMX || !GIsPentiumPro) && Driver==TEXT("SoftDrv.SoftwareRenderDevice") )
		{
			Info = Info + LocalizeGeneral(TEXT("ResLow"),TEXT("Startup")) + TEXT("\r\n");
			GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("WindowedViewportX"),  TEXT("320") );
			GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("WindowedViewportY"),  TEXT("240") );
			GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("WindowedColorBits"),  TEXT("16") );
			GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("FullscreenViewportX"), TEXT("320") );
			GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("FullscreenViewportY"), TEXT("240") );
			GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("FullscreenColorBits"), TEXT("16") );
		}
		// We only want to set the resolution when we're installing the game for the first time
		else if (FirstRun == 0)
		{
			DWORD BestWidth = 800, BestHeight = 600;
			DEVMODE DevMode{};
			DevMode.dmSize = sizeof(DEVMODE);
			for (INT ModeNum = 0; EnumDisplaySettings(NULL, ModeNum, &DevMode) != 0; ModeNum++)
			{
				if (DevMode.dmPelsHeight > BestHeight && DevMode.dmPelsHeight < 2000 &&
					DevMode.dmPelsWidth > BestWidth && DevMode.dmPelsWidth < 2000 &&
					DevMode.dmBitsPerPel == 32)
				{
					BestHeight = DevMode.dmPelsHeight;
					BestWidth = DevMode.dmPelsWidth;
				}
			}

			GConfig->SetString(TEXT("WinDrv.WindowsClient"), TEXT("WindowedViewportX"), *FString::Printf(TEXT("%i"), BestWidth));
			GConfig->SetString(TEXT("WinDrv.WindowsClient"), TEXT("WindowedViewportY"), *FString::Printf(TEXT("%i"), BestHeight));
			GConfig->SetString(TEXT("WinDrv.WindowsClient"), TEXT("WindowedColorBits"), TEXT("32"));
			GConfig->SetString(TEXT("WinDrv.WindowsClient"), TEXT("FullscreenViewportX"), *FString::Printf(TEXT("%i"), BestWidth));
			GConfig->SetString(TEXT("WinDrv.WindowsClient"), TEXT("FullscreenViewportY"), *FString::Printf(TEXT("%i"), BestHeight));
			GConfig->SetString(TEXT("WinDrv.WindowsClient"), TEXT("FullscreenColorBits"), TEXT("32"));
			Info = Info + LocalizeGeneral(TEXT("ResHigh"), TEXT("Startup")) + TEXT("\r\n");			
		}
		DetailEdit.SetText(*Info);
	}
	WWizardPage* GetNext()
	{
		return new WConfigPageFirstTime(Owner);
	}
};

class WConfigPageMouseInput : public WWizardPage
{
	DECLARE_WINDOWCLASS(WConfigPageMouseInput,WWizardPage,Startup)
	WConfigWizard* Owner;
	WLabel MouseInputPrompt;
	WListBox MouseInputList;
	WConfigPageMouseInput( WConfigWizard* InOwner )
	: WWizardPage( TEXT("ConfigPageMouseInput"), IDDIALOG_ConfigPageMouseInput, InOwner )
	, Owner(InOwner)
	, MouseInputPrompt(this,IDC_MouseInputPrompt)
	, MouseInputList(this,IDC_MouseInputList)
	{}
	void OnInitDialog()
	{
		WWizardPage::OnInitDialog();
		MouseInputList.DoubleClickDelegate = FDelegate(Owner,(TDelegate)&WWizardDialog::OnNext);

		MouseInputList.AddString( Localize(TEXT("UMenuInputOptionsClientWindow"),TEXT("MouseInputDetail[0]"),TEXT("UMenu")) );
		MouseInputList.AddString( Localize(TEXT("UMenuInputOptionsClientWindow"),TEXT("MouseInputDetail[1]"),TEXT("UMenu")) );
		MouseInputList.AddString( Localize(TEXT("UMenuInputOptionsClientWindow"),TEXT("MouseInputDetail[2]"),TEXT("UMenu")) );

		FString ViewportManager;
		GConfig->GetString(TEXT("Engine.Engine"),TEXT("ViewportManager"), ViewportManager);
		UBOOL Value = 0;
		INT Current = 0;		
		if (GConfig->GetBool(*ViewportManager,TEXT("UseRawHIDInput"), Value) && Value)
			Current = 2;
		else if (GConfig->GetBool(*ViewportManager,TEXT("UseDirectInput"), Value) && Value)
			Current = 1;
		MouseInputList.SetCurrent(Current, TRUE);
	}
	WWizardPage* GetNext()
	{
		INT Current = MouseInputList.GetCurrent();
		if (Current >= 0)
		{
			FString ViewportManager;
			GConfig->GetString(TEXT("Engine.Engine"),TEXT("ViewportManager"), ViewportManager);
			
			GConfig->SetBool(*ViewportManager,TEXT("UseRawHIDInput"), Current == 2 ? TRUE : FALSE);
			GConfig->SetBool(*ViewportManager,TEXT("UseDirectInput"), Current == 1 ? TRUE : FALSE);
		}
		return new WConfigPageDetail(Owner);
	}
};

class WConfigPageDriver : public WWizardPage
{
	DECLARE_WINDOWCLASS(WConfigPageDriver,WWizardPage,Startup)
	WConfigWizard* Owner;
	WUrlButton WebButton;
	WLabel Card;
	WConfigPageDriver( WConfigWizard* InOwner )
	: WWizardPage( TEXT("ConfigPageDriver"), IDDIALOG_ConfigPageDriver, InOwner )
	, Owner(InOwner)
	, WebButton(this,LocalizeGeneral(TEXT("Direct3DWebPage"),TEXT("Startup")),IDC_WebButton)
	, Card(this,IDC_Card)
	{}
	void OnInitDialog()
	{
		WWizardPage::OnInitDialog();
		FString CardName=GConfig->GetStr(TEXT("D3DDrv.D3DRenderDevice"),TEXT("Description"));
		if( CardName!=TEXT("") )
			Card.SetText(*CardName);
	}
	WWizardPage* GetNext()
	{
		return new WConfigPageMouseInput(Owner);
	}	
};

class WConfigPageSound : public WWizardPage
{
	DECLARE_WINDOWCLASS(WConfigPageSound,WWizardPage,Startup)
	WConfigWizard * Owner;
	WListBox SoundList;
	WButton ShowCompatible;
	WLabel SoundNote;
	INT First;
	TArray<FRegistryObjectInfo> Classes;
	BOOL SoundOnly;
	WConfigPageSound(WConfigWizard* InOwner)
	: WWizardPage(TEXT("ConfigPageSound"), IDDIALOG_ConfigPageSound, InOwner)
	, Owner(InOwner)
	, SoundList(this,IDC_SoundList)
	, ShowCompatible(this,IDC_Compatible,FDelegate(this,(TDelegate)& WConfigPageSound::RefreshList))
	, SoundNote(this,IDC_SoundNote)
	, First(0)
	, SoundOnly(FALSE)
	{}
	void RefreshList()
	{
		SoundList.Empty();
		FString Default;
		Classes.Empty();
		UObject::GetRegistryObjects(Classes, UClass::StaticClass(), UAudioSubsystem::StaticClass(), 0);
		for (TArray<FRegistryObjectInfo>::TIterator It(Classes); It; ++It)
		{
			FString Path = It->Object, Left, Right, Temp;
			if (Path.Split(TEXT("."),&Left,&Right))
			{
				SoundList.AddString(*(Temp = Localize(*Right,TEXT("ClassCaption"),*Left)));

				if (Default == TEXT(""))
				Default = Temp;
			}
		}
		if (Default != TEXT(""))
			SoundList.SetCurrent(SoundList.FindStringChecked(*Default),1);
		CurrentChange();
	}
	void CurrentChange()
	{
		debugf(TEXT("CurrentDriver %ls"), *CurrentDriver());
		FString Path = CurrentDriver(), Left, Right, Descr;
		if (Path.Split(TEXT("."), &Left, &Right))
			Descr = Localize(*Right, TEXT("ClassDescription"), *Left, NULL, 1);
		if (Descr == TEXT(""))
			Descr = Localize(TEXT("Descriptions"), *CurrentDriver(), TEXT("Startup"), NULL, 1);
		SoundNote.SetText(*Descr);
	}
	void OnPaint()
	{
		if (!First++)
		{
			UpdateWindow(*this);
			GConfig->Flush(1);
			RefreshList();
		}
	}
	void OnCurrent()
	{
		guard(WConfigPageSound::OnCurrent);
		unguard;
	}
	void OnInitDialog()
	{
		WWizardPage::OnInitDialog();
		SendMessageW(ShowCompatible,BM_SETCHECK,BST_CHECKED,0);
		SoundList.SelectionChangeDelegate = FDelegate(this,(TDelegate)& WConfigPageSound::CurrentChange);
		SoundList.DoubleClickDelegate = FDelegate(Owner,(TDelegate)& WWizardDialog::OnNext);
		SoundList.AddString(LocalizeGeneral(TEXT("Detecting"),TEXT("Startup")));
	}
	FString CurrentDriver()
	{
		if (SoundList.GetCurrent() >= 0)
		{
			FString Name = SoundList.GetString(SoundList.GetCurrent());
			for (TArray<FRegistryObjectInfo>::TIterator It(Classes); It; ++It)
			{
				FString Path = It->Object, Left, Right, Temp;
				if (Path.Split(TEXT("."),&Left,&Right))
					if (Name == Localize(*Right,TEXT("ClassCaption"),*Left))
						return Path;
			}
		}
		return TEXT("");
	}
	WWizardPage* GetNext()
	{
		if (CurrentDriver() != TEXT(""))
			GConfig->SetString(TEXT("Engine.Engine"),TEXT("AudioDevice"),*CurrentDriver());
		if (SoundOnly)
			return new WConfigPageFirstTime(Owner);
		if (CurrentDriver() == TEXT("ALAudio.ALAudioSubsystem"))
			return new WConfigPageDriver(Owner);
		if (CurrentDriver() == TEXT("Galaxy.GalaxyAudioSubsystem"))
			return new WConfigPageDriver(Owner);
		if (CurrentDriver() == TEXT("Cluster.ClusterAudioSubsystem"))
			return new WConfigPageDriver(Owner);
		else
			return new WConfigPageDetail(Owner);
	}
};

class WConfigPageRenderer : public WWizardPage
{
	DECLARE_WINDOWCLASS(WConfigPageRenderer,WWizardPage,Startup)
	WConfigWizard* Owner;
	WListBox RenderList;
	WButton ShowCompatible, ShowAll;
	WLabel RenderNote;
	INT First;
	TArray<FRegistryObjectInfo> Classes;
	WConfigPageRenderer( WConfigWizard* InOwner )
	: WWizardPage( TEXT("ConfigPageRenderer"), IDDIALOG_ConfigPageRenderer, InOwner )
	, Owner(InOwner)
	, RenderList(this,IDC_RenderList)
	, ShowCompatible(this,IDC_Compatible,FDelegate(this,(TDelegate)&WConfigPageRenderer::RefreshList))
	, ShowAll(this,IDC_All,FDelegate(this,(TDelegate)&WConfigPageRenderer::RefreshList))
	, RenderNote(this,IDC_RenderNote)
	, First(0)
	{}
	void RefreshList()
	{
		INT FirstRun = 0;
		GConfig->GetInt(TEXT("FirstRun"), TEXT("FirstRun"), FirstRun);
		RenderList.Empty();
		INT All=(SendMessageW(ShowAll,BM_GETCHECK,0,0)==BST_CHECKED), BestPriority=0;
		FString Default;
		Classes.Empty();
		FString CurrentSetting;
		GConfig->GetString(TEXT("Engine.Engine"), TEXT("GameRenderDevice"), CurrentSetting);
		UObject::GetRegistryObjects( Classes, UClass::StaticClass(), URenderDevice::StaticClass(), 0 );
		for( TArray<FRegistryObjectInfo>::TIterator It(Classes); It; ++It )
		{
			FString Path=It->Object, Left, Right, Temp;
			if( Path.Split(TEXT("."),&Left,&Right) )
			{
				// Buggie: Remove outdated renderers from the list if we're not building for Windows XP
#if _MSC_VER >= 1920 && !defined(TEST_BUILD) // not use in test builds
				if (Path == TEXT("SoftDrv.SoftwareRenderDevice") || Path == TEXT("D3DDrv.D3DRenderDevice"))
					continue;
#endif

				INT DoShow = All, Priority = 0;
				INT DescFlags = 0;
				GConfig->GetInt(*Path, TEXT("DescFlags"), DescFlags);
				if
				(	It->Autodetect != TEXT("")
				&& (GFileManager->FileSize(*FString::Printf(TEXT("%s\\%s"), SysDir, *It->Autodetect)) >= 0
				||  GFileManager->FileSize(*FString::Printf(TEXT("%s\\%s"), WinDir, *It->Autodetect)) >= 0) )
					DoShow = Priority = 3000;
				else if (DescFlags & RDDESCF_Certified)
					DoShow = Priority = 2000;
				else if (Path == TEXT("SoftDrv.SoftwareRenderDevice"))
					DoShow = Priority = 1000;

				// Ordering score
				if (Path == TEXT("D3D12Drv.D3D12RenderDevice")) // Just for future
					Priority += 100;
				else if (Path == TEXT("VulkanDrv.VulkanRenderDevice"))
					Priority += 200;
				else if (Path == TEXT("D3D9Drv.D3D9RenderDevice"))
					Priority += 300;
				else if (Path == TEXT("XOpenGLDrv.XOpenGLRenderDevice"))
					Priority += 400;
				else if (Path == TEXT("OpenGLDrv.OpenGLRenderDevice"))
					Priority += 500;
				else if (Path == TEXT("ICBINDx11Drv.ICBINDx11RenderDevice"))
					Priority += 600;
				else if (Path == TEXT("D3D11Drv.D3D11RenderDevice"))
					Priority += 700;

				if (DoShow)
				{	
					// Keep current setup if not first run.
					if (Path == CurrentSetting && FirstRun == ENGINE_VERSION)
						Priority = 4000;
					RenderList.AddString( *(Temp=Localize(*Right,TEXT("ClassCaption"),*Left)) );
					if (Priority >= BestPriority)
					{
						Default = Temp; 
						BestPriority = Priority;
					}
				}
			}
		}
		if( Default!=TEXT("") )
			RenderList.SetCurrent(RenderList.FindStringChecked(*Default),1);
		CurrentChange();
	}
	void CurrentChange()
	{
		debugf(TEXT("CurrentDriver %ls"), *CurrentDriver());
		FString Path = CurrentDriver(), Left, Right, Descr;
		if (Path.Split(TEXT("."), &Left, &Right))
			Descr = Localize(*Right, TEXT("ClassDescription"), *Left, NULL, 1);
		if (Descr == TEXT(""))
			Descr = Localize(TEXT("Descriptions"), *CurrentDriver(), TEXT("Startup"), NULL, 1);
		RenderNote.SetText(*Descr);
	}
	void OnPaint()
	{
		if( !First++ )
		{
			UpdateWindow( *this );
			GConfig->Flush( 1 );
			if( !ParseParam(appCmdLine(),TEXT("nodetect")) )
			{
				GFileManager->Delete(TEXT("Detected.ini"));
				ShellExecuteW( NULL, TEXT("open"), ThisFile, TEXT("testrendev=D3DDrv.D3DRenderDevice log=Detected.log"), appBaseDir(), SW_SHOWNORMAL );
				for( INT MSec=80000; MSec>0 && GFileManager->FileSize(TEXT("Detected.ini"))<0; MSec-=100 )
					Sleep(100);
			}
			RefreshList();
		}
	}
	void OnCurrent()
	{
		guard(WConfigPageRenderer::OnCurrent);
		unguard;
	}
	void OnInitDialog()
	{
		WWizardPage::OnInitDialog();
		SendMessageW(ShowCompatible,BM_SETCHECK,BST_CHECKED,0);
		RenderList.SelectionChangeDelegate = FDelegate(this,(TDelegate)&WConfigPageRenderer::CurrentChange);
		RenderList.DoubleClickDelegate = FDelegate(Owner,(TDelegate)&WWizardDialog::OnNext);
		RenderList.AddString( LocalizeGeneral(TEXT("Detecting"),TEXT("Startup")) );
	}
	FString CurrentDriver()
	{
		if( RenderList.GetCurrent()>=0 )
		{
			FString Name = RenderList.GetString(RenderList.GetCurrent());
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
	WWizardPage* GetNext()
	{
		if( CurrentDriver()!=TEXT("") )
			GConfig->SetString(TEXT("Engine.Engine"),TEXT("GameRenderDevice"),*CurrentDriver());
		if( CurrentDriver()==TEXT("D3DDrv.D3DRenderDevice") )
			return new WConfigPageDriver(Owner);
		else
			return new WConfigPageMouseInput(Owner);
	}
};

class WConfigPageSafeMode : public WWizardPage
{
	DECLARE_WINDOWCLASS(WConfigPageSafeMode,WWizardPage,Startup)
	WConfigWizard* Owner;
	WCoolButton RunButton, VideoButton, SoundButton, SafeModeButton, WebButton;
	WConfigPageSafeMode( WConfigWizard* InOwner )
	: WWizardPage    ( TEXT("ConfigPageSafeMode"), IDDIALOG_ConfigPageSafeMode, InOwner )
	, RunButton      ( this, IDC_Run,      FDelegate(this,(TDelegate)&WConfigPageSafeMode::OnRun) )
	, VideoButton    ( this, IDC_Video,    FDelegate(this,(TDelegate)&WConfigPageSafeMode::OnVideo) )
	, SoundButton	 ( this, IDC_Audio,	  FDelegate(this, (TDelegate)&WConfigPageSafeMode::OnSound))
	, SafeModeButton ( this, IDC_SafeMode, FDelegate(this,(TDelegate)&WConfigPageSafeMode::OnSafeMode) )
	, WebButton      ( this, IDC_Web,      FDelegate(this,(TDelegate)&WConfigPageSafeMode::OnWeb) )
	, Owner          (InOwner)
	{}
	void OnRun()
	{
		Owner->EndDialog(1);
	}
	void OnVideo()
	{
		Owner->Advance( new WConfigPageRenderer(Owner) );
	}
	void OnSound()
	{
		Owner->Advance(new WConfigPageSound(Owner));
	}
	void OnSafeMode()
	{
		Owner->Advance( new WConfigPageSafeOptions(Owner) );
	}
	void OnWeb()
	{
		ShellExecuteW( *this, TEXT("open"), LocalizeGeneral(TEXT("WebPage"),TEXT("Startup")), TEXT(""), appBaseDir(), SW_SHOWNORMAL );
		Owner->EndDialog(0);
	}
	const TCHAR* GetNextText()
	{
		return NULL;
	}
};

/*-----------------------------------------------------------------------------
	Exec hook.
-----------------------------------------------------------------------------*/

// FExecHook.
class FExecHook : public FExec, public FNotifyHook
{
private:
	WConfigProperties* Preferences;
	void NotifyDestroy( void* Src )
	{
		if( Src==Preferences )
			Preferences = NULL;
	}
	UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar )
	{
		guard(FExecHook::Exec);
		if( ParseCommand(&Cmd,TEXT("ShowLog")) )
		{
			if( GLogWindow )
			{
				GLogWindow->Show(1);
				SetFocus( *GLogWindow );
				GLogWindow->Display.ScrollCaret();
			}
			return 1;
		}
		else if( ParseCommand(&Cmd,TEXT("TakeFocus")) )
		{
			TObjectIterator<UEngine> EngineIt;
			if
			(	EngineIt
			&&	EngineIt->Client
			&&	EngineIt->Client->Viewports.Num() )
				SetForegroundWindow( (HWND)EngineIt->Client->Viewports(0)->GetWindow() );
			return 1;
		}
		else if (ParseCommand(&Cmd, TEXT("EditActor")))
		{
			UClass* Class;
			FName ActorName;
			TObjectIterator<UEngine> EngineIt;

			AActor* Found = NULL;

			if( EngineIt && ParseObject<UClass>( Cmd, TEXT("Class="), Class, ANY_PACKAGE ) )
			{
				AActor* Player  = EngineIt->Client ? EngineIt->Client->Viewports(0)->Actor : NULL;
				FLOAT   MinDist = 999999.0;
				for( TObjectIterator<AActor> It; It; ++It )
				{
					FLOAT Dist = Player ? FDist(It->Location,Player->Location) : 0.0f;
					if
					(	(!Player || It->GetLevel()==Player->GetLevel())
					&&	(!It->bDeleteMe)
					&&	(It->IsA( Class) )
					&&	(Dist<MinDist) )
					{
						MinDist = Dist;
						Found   = *It;
					}
				}
			}
			else if( EngineIt && Parse( Cmd, TEXT("Name="), ActorName ) )
			{
				// look for actor by name
				for( TObjectIterator<AActor> It; It; ++It )
				{
					if( !It->bDeleteMe && It->GetName() == *ActorName )
					{
						Found = *It;
						break;
					}
				}
			}

			if( Found )
			{
				if (!GIsEditor && EngineIt && EngineIt->Client && EngineIt->Client->Viewports(0))
					EngineIt->Client->Viewports(0)->Exec(TEXT("EndFullscreen"));
				
				WObjectProperties* P = new WObjectProperties( TEXT("EditActor"), 0, TEXT(""), NULL, 1 );
				P->OpenWindow( (HWND)(EngineIt && EngineIt->Client && EngineIt->Client->Viewports(0) ? 
					EngineIt->Client->Viewports(0)->GetWindow() : (GLogWindow ? GLogWindow->hWnd : NULL)) );
				P->Root.SetObjects( (UObject**)&Found, 1 );
				P->Show(1);
			}
			else Ar.Logf( TEXT("Bad or missing class or name") );
			return 1;
		}
		else if( ParseCommand(&Cmd,TEXT("HideLog")) )
		{
			if( GLogWindow )
				GLogWindow->Show(0);
			return 1;
		}
		else if( ParseCommand(&Cmd,TEXT("Preferences")) && !GIsClient )
		{
			if( !Preferences )
			{
				Preferences = new WConfigProperties( TEXT("Preferences"), LocalizeGeneral(TEXT("AdvancedOptionsTitle"), TEXT("Window")) );
				Preferences->SetNotifyHook( this );
				Preferences->OpenWindow( GLogWindow ? GLogWindow->hWnd : NULL );
				Preferences->ForceRefresh();
			}
			Preferences->Show(1);
			SetFocus( *Preferences );
			return 1;
		}
		else if (ParseCommand(&Cmd, TEXT("GETSYSTEMINI")))
		{
			Ar.Logf(TEXT("%ls"), dynamic_cast<FConfigCacheIni*>(GConfig) ? *dynamic_cast<FConfigCacheIni*>(GConfig)->SystemIni : TEXT("UnrealTournament.ini"));
			return 1;
		}
		else if (ParseCommand(&Cmd, TEXT("GETUSERINI")))
		{
			Ar.Logf(TEXT("%ls"), dynamic_cast<FConfigCacheIni*>(GConfig) ? *dynamic_cast<FConfigCacheIni*>(GConfig)->UserIni : TEXT("User.ini"));
			return 1;
		}
		else if (ParseCommand(&Cmd, TEXT("RELAUNCHSUPPORT")))
		{
			Ar.Logf(TEXT("ENABLED"));
			return 1;
		}
		else if (ParseCommand(&Cmd, TEXT("LOGWINDOWSUPPORT")))
		{
			Ar.Logf(TEXT("ENABLED"));
			return 1;
		}
		else return 0;
		unguard;
	}
public:
	FExecHook()
	: Preferences( NULL )
	{}
};

/*-----------------------------------------------------------------------------
	Startup and shutdown.
-----------------------------------------------------------------------------*/

//
// Initialize.
//
#ifndef _EDITOR_
static FExecHook GLocalHook;
static UEngine* InitEngine()
{
	guard(InitEngine);
	DOUBLE LoadTime = appSecondsNew();

	// Set exec hook.
	GExec = &GLocalHook;

	// Create mutex so installer knows we're running.
	CreateMutexW( NULL, 0, RUNNING_MUTEX(appBaseDir()));
	UBOOL AlreadyRunning = (GetLastError()==ERROR_ALREADY_EXISTS);

	// First-run menu.
	INT FirstRun=0;
	GConfig->GetInt( TEXT("FirstRun"), TEXT("FirstRun"), FirstRun );
	if( ParseParam(appCmdLine(),TEXT("FirstRun")) )
		FirstRun=0;
	if( FirstRun<220 )
	{
		// Migrate savegames.
		TArray<FString> Saves = GFileManager->FindFiles( TEXT("..\\Save\\*.usa"), 1, 0 );
		for( TArray<FString>::TIterator It(Saves); It; ++It )
		{
			INT Pos = appAtoi(**It+4);
			FString Section = TEXT("UnrealShare.UnrealSlotMenu");
			FString Key     = FString::Printf(TEXT("SlotNames[%i]"),Pos);
			if( appStricmp(GConfig->GetStr(*Section,*Key,TEXT("user")),TEXT(""))==0 )
				GConfig->SetString(*Section,*Key,TEXT("Saved game"),TEXT("user"));
		}
	}

	// Commandline (for mplayer/heat)
	FString Command;
	if( Parse(appCmdLine(),TEXT("consolecommand="), Command) )
	{
		debugf(TEXT("Executing console command %s"),*Command);
		GExec->Exec( *Command, *GLog );
		return NULL;
	}

	// Test render device.
	FString Device;
	if( Parse(appCmdLine(),TEXT("testrendev="),Device) )
	{
		debugf(TEXT("Detecting %s"),*Device);
		try
		{
			UClass* Cls = LoadClass<URenderDevice>( NULL, *Device, NULL, 0, NULL );
			GConfig->SetInt(*Device,TEXT("DescFlags"),RDDESCF_Incompatible);
			GConfig->Flush(0);
			if( Cls )
			{
				URenderDevice* RenDev = ConstructObject<URenderDevice>(Cls);
				if( RenDev )
				{
					if( RenDev->Init(NULL,0,0,0,0) )
					{
						debugf(TEXT("Successfully detected %s"),*Device);
					}
					else
					{
						delete RenDev;
						RenDev = NULL;
					}
				}
			}
		} catch( ... ) {}
		FArchive* Ar = GFileManager->CreateFileWriter(TEXT("Detected.ini"),0);
		if( Ar )
			delete Ar;
		return NULL;
	}

	// Config UI.
	guard(ConfigUI);
	if( !GIsEditor && GIsClient )
	{
		WConfigWizard D;
		WWizardPage* Page = NULL;
		if( ParseParam(appCmdLine(),TEXT("safe")) || appStrfind(appCmdLine(),TEXT("readini")) )
			{Page = new WConfigPageSafeMode(&D); D.Title=LocalizeGeneral(TEXT("SafeMode"),TEXT("Startup"));}
		else if( FirstRun<ENGINE_VERSION )
			{Page = new WConfigPageRenderer(&D); D.Title=LocalizeGeneral(TEXT("FirstTime"),TEXT("Startup"));}
		else if( ParseParam(appCmdLine(),TEXT("changevideo")) )
			{Page = new WConfigPageRenderer(&D); D.Title=LocalizeGeneral(TEXT("Video"),TEXT("Startup"));}
		else if (ParseParam(appCmdLine(), TEXT("changesound")))
			{Page = new WConfigPageSound(&D); ((WConfigPageSound*)Page)->SoundOnly = TRUE;  D.Title = LocalizeGeneral(TEXT("Audio"), TEXT("Startup"));}
		else if( !AlreadyRunning && GFileManager->FileSize(TEXT("Running.ini"))>=0 )
			{Page = new WConfigPageSafeMode(&D); D.Title=LocalizeGeneral(TEXT("RecoveryMode"),TEXT("Startup"));}
		if( Page )
		{
			ExitSplash();
			D.Advance( Page );
			if( !D.DoModal() )
				return NULL;
			InitSplash(NULL);
		}
	}
	unguard;

	// Create is-running semaphore file.
	FArchive* Ar = GFileManager->CreateFileWriter(TEXT("Running.ini"),0);
	if( Ar )
		delete Ar;

	// Cd check.
	FString CdPath;
	GConfig->GetString( TEXT("Engine.Engine"), TEXT("CdPath"), CdPath );
	if
	(	CdPath!=TEXT("")
	&&	GFileManager->FileSize(TEXT("..\\Textures\\Palettes.utx"))<=0 )//oldver
	{
		FString Check = CdPath * TEXT("Textures\\Palettes.utx");
		while( !GIsEditor && GFileManager->FileSize(*Check)<=0 )
		{
			if( MessageBox
			(
				NULL,
				LocalizeGeneral(TEXT("InsertCdText"),TEXT("Window")),
				LocalizeGeneral(TEXT("InsertCdTitle"),TEXT("Window")),
				MB_TASKMODAL|MB_OKCANCEL
			)==IDCANCEL )
			{
				GIsCriticalError = 1;
				ExitProcess( 0 );
			}
		}
	}

	// Create the global engine object.
	UClass* EngineClass;
	if( !GIsEditor )
	{
		// Create game engine.
		EngineClass = UObject::StaticLoadClass( UGameEngine::StaticClass(), NULL, TEXT("ini:Engine.Engine.GameEngine"), NULL, LOAD_NoFail, NULL );
	}
	else
	{
		// Editor.
		EngineClass = UObject::StaticLoadClass( UEngine::StaticClass(), NULL, TEXT("ini:Engine.Engine.EditorEngine"), NULL, LOAD_NoFail, NULL );
	}
	UEngine* Engine = ConstructObject<UEngine>( EngineClass );
	Engine->Init();
	debugf( TEXT("Startup time: %f seconds"), appSecondsNew()-LoadTime );

	return Engine;
	unguard;
}

static BOOL GIgnoreErrors = FALSE;
BOOL GRecoveryMode = FALSE;

static BOOL IgnoreUEdError()
{
	if (!GIsEditor)
		return FALSE;
	GErrorHist[ARRAY_COUNT(GErrorHist) - 1] = 0;
	//debugf(TEXT("Try catch error"));
	int Answer = GIgnoreErrors ? IDIGNORE : MessageBox(NULL, GErrorHist, GConfig ? TEXT("Critical Error") : TEXT("Critical Error At Startup"), MB_ABORTRETRYIGNORE | MB_ICONERROR | MB_TASKMODAL);
	if (Answer != IDRETRY && Answer != IDIGNORE)
		return FALSE;
	GIsCriticalError = 0;
	if (Answer == IDIGNORE)
		GIgnoreErrors = TRUE;
	if (!GRecoveryMode)
	{
		GRecoveryMode = TRUE;
		GWarn->YesNof(TEXT("UnrealEd will now continue in Recovery Mode.\n\nPlease back up your unsaved work and restart UnrealEd as soon as possible.\n\nClick any option to continue."));
	}
	else
		GRecoveryMode = TRUE;
	return TRUE;
}

#define guardcatch try {
#define unguardcatch }\
catch (TCHAR* Err) { if (!IgnoreUEdError()) throw Err; }\
catch (int Err) { if (!IgnoreUEdError()) throw Err; }\
catch (...) { if (!IgnoreUEdError()) throw; }\

//
// Unreal's main message loop.  All windows in Unreal receive messages
// somewhere below this function on the stack.
//

static inline void DispatchMessages()
{
	// Handle all incoming messages.
	guard(MessagePump);
	MSG Msg;
	POINT mouse;
	while (PeekMessageW(&Msg, NULL, 0, 0, PM_REMOVE))
	{
		guardcatch;
		
		if (Msg.message == WM_SETFOCUS_DELAYED)
		{
			SetFocus((HWND)Msg.wParam);
			continue;
		}
		
		if (Msg.message == WM_QUIT)
		{
			debugf(TEXT("DispatchMessages: Received WM_QUIT"));
			GIsRequestingExit = 1;
		}

		if (GIsEditor)
		{
			HWND Curr = GetActiveWindow();
			if (Curr == NULL)
				Curr = Msg.hwnd;
			TranslateAccelerator(
				Curr,  // handle to receiving window
				hAccel,    // handle to active accelerator table
				&Msg);     // message data
			if (WC_DIALOG == MAKEINTATOM(GetClassLong(Curr, GCW_ATOM)) && IsDialogMessage(Curr, &Msg))
				continue;
		}

		guard(TranslateDispatchMessage);
		if (Msg.message == WM_MOUSEWHEEL && GetCursorPos(&mouse))
		{ // hack for send scroll message to window below cursor, not which has focus
			HWND Win = WindowFromPoint(mouse);
			if (Win)
			{
				DWORD Pid = 0;
				GetWindowThreadProcessId(Win, &Pid);
				if (Pid == GetCurrentProcessId())
					Msg.hwnd = Win;
			}
		}

		GMessageTime = Msg.time;

		TranslateMessage(&Msg);
		DispatchMessageW(&Msg);
		unguardf((TEXT("%08X %i"), (PTRINT)Msg.hwnd, Msg.message));
		
		unguardcatch;

		// stijn: I added this because modern versions of windows no longer propagate exceptions thrown in Window procs.
		// If you want an exception thrown in a window proc to propagate all the way to the main loop, then you need to
		// eat the exception in the window proc, and rethrow it here.
		// See https://docs.microsoft.com/en-us/previous-versions/windows/desktop/legacy/ms633573(v=vs.85) for details
		if (GIsCriticalError && !IgnoreUEdError())
			throw 1;
	}
	unguard;
}

static inline void AdjustThreadPriority(DWORD ThreadId, HANDLE hThread)
{
	// If editor thread doesn't have the focus, don't suck up too much CPU time.
	if (GIsEditor)
	{
		guard(ThrottleEditor);
		static UBOOL HadFocus = 1;
		UBOOL HasFocus = (GetWindowThreadProcessId(GetForegroundWindow(), NULL) == ThreadId);
		if (HadFocus && !HasFocus)
		{
			// Drop our priority to speed up whatever is in the foreground.
			SetThreadPriority(hThread, THREAD_PRIORITY_BELOW_NORMAL);
		}
		else if (HasFocus && !HadFocus)
		{
			// Boost our priority back to normal.
			SetThreadPriority(hThread, THREAD_PRIORITY_NORMAL);
		}
		if (!HasFocus)
		{
			// Surrender the rest of this timeslice.
			Sleep(0);
		}
		HadFocus = HasFocus;
		unguard;
	}
}

// Update the world.
static inline void TimestampTick(UEngine* Engine, QWORD& OldCycles, QWORD& SecondStartCycles, INT& TickCount)
{
	guardcatch;
	
	guard(UpdateWorld);
	QWORD NewCycles = appCyclesLong();
	DOUBLE DeltaTime = (NewCycles - OldCycles) * GSecondsPerCycleLong;
	// skip a tick if the time has just wrapped around
	if (DeltaTime > 0.0)
	{
		Engine->Tick(DeltaTime);
		if (GWindowManager)
			GWindowManager->Tick(DeltaTime);
	}
	OldCycles = NewCycles;
	TickCount++;
	if (OldCycles * GSecondsPerCycleLong > SecondStartCycles* GSecondsPerCycleLong + 1)
	{
		Engine->CurrentTickRate = TickCount / ((OldCycles * GSecondsPerCycleLong) - (SecondStartCycles * GSecondsPerCycleLong));
		SecondStartCycles = OldCycles;
		TickCount = 0;
	}
	unguard;

	// Enforce optional maximum tick rate.
#ifndef RENDER_BENCHMARK
	guard(EnforceTickRate);
	DOUBLE MaxTickRate = Engine->GetMaxTickRate();
	if (MaxTickRate > 0.0)
	{
		DOUBLE Delta = (1.0 / MaxTickRate) - (appCyclesLong() - OldCycles) * GSecondsPerCycleLong;
		appSleepLong(Delta);
	}
	unguard;
#endif
	
	unguardcatch;
}

static inline void FTimeTick(UEngine* Engine, DOUBLE& OldTime, DOUBLE& SecondStartTime, INT& TickCount)
{
	guardcatch;
	
	// Update the world.
	guard(UpdateWorld);
	DOUBLE NewTime = appSecondsNewNoCheck();
	DOUBLE DeltaTime = NewTime - OldTime;
	// skip a tick if the time has just wrapped around
	if (DeltaTime > 0.0)
	{
		Engine->Tick(DeltaTime);
		if (GWindowManager)
			GWindowManager->Tick(DeltaTime);
	}
	OldTime = NewTime;
	TickCount++;
	if (OldTime > SecondStartTime + 1)
	{
		Engine->CurrentTickRate = (FLOAT)TickCount / (OldTime - SecondStartTime);
		SecondStartTime = OldTime;
		TickCount = 0;
	}
	unguard;

	// Enforce optional maximum tick rate.
	guard(EnforceTickRate);
	DOUBLE MaxTickRate = Engine->GetMaxTickRate();
	if (MaxTickRate > 0.0)
	{
		DOUBLE Delta = (1.0 / MaxTickRate) - (appSecondsNewNoCheck() - OldTime);
		appSleepLong(Delta);
	}
	unguard;
	
	unguardcatch;
}

static inline void TGTTick(UEngine* Engine, DOUBLE& OldTime, DOUBLE& SecondStartTime, INT& TickCount)
{
	guardcatch;
	
	// Update the world.
	guard(UpdateWorld);
	DOUBLE NewTime = appSecondsCompat();
	DOUBLE DeltaTime = NewTime - OldTime;
	// skip a tick if the time has just wrapped around
	if (DeltaTime > 0.0)
	{
		Engine->Tick(DeltaTime);
		if (GWindowManager)
			GWindowManager->Tick(DeltaTime);
	}
	OldTime = NewTime;
	TickCount++;
	if (OldTime > SecondStartTime + 1)
	{
		Engine->CurrentTickRate = (FLOAT)TickCount / (OldTime - SecondStartTime);
		SecondStartTime = OldTime;
		TickCount = 0;
	}
	unguard;

	// Enforce optional maximum tick rate.
	guard(EnforceTickRate);
	DOUBLE MaxTickRate = Engine->GetMaxTickRate();
	if (MaxTickRate > 0.0)
	{
		DOUBLE Delta = (1.0 / MaxTickRate) - (appSecondsCompat() - OldTime);
		appSleepLong(Delta);
	}
	unguard;
	
	unguardcatch;
}

static struct
{
	UEngine* Engine = NULL;
	INT* TickCount = NULL;
	DOUBLE* OldTime = NULL;
	DOUBLE* SecondStartTime = NULL;
	QWORD* OldCycles = NULL;
	QWORD* SecondStartCycles = NULL;
} IdleTickData;

static void IdleTick(void)
{
	guard(IdleTick);

	if (!GIsRunning || GIsRequestingExit)
		return;

	if (GTGTPreferred)
		TGTTick(IdleTickData.Engine, *IdleTickData.OldTime, *IdleTickData.SecondStartTime, *IdleTickData.TickCount);
	else if (GTimestampPreferred)
		TimestampTick(IdleTickData.Engine, *IdleTickData.OldCycles, *IdleTickData.SecondStartCycles, *IdleTickData.TickCount);
	else
		FTimeTick(IdleTickData.Engine, *IdleTickData.OldTime, *IdleTickData.SecondStartTime, *IdleTickData.TickCount);

	unguard;
}

#if WIN32
#include "SETranslator.h"
#endif

static void MainLoop( UEngine* Engine )
{
	guard(MainLoop);
	check(Engine);

#if WIN32
	SetSETranslator();
#endif

	// Enter main loop.
	guard(EnterMainLoop);
	if( GLogWindow )
		GLogWindow->SetExec( Engine );
	unguard;

	IdleTickData.Engine = Engine;

	// Loop while running.
	GIsRunning = 1;
	DWORD ThreadId = GetCurrentThreadId();
	HANDLE hThread = GetCurrentThread();
	INT TickCount = 0;
	IdleTickData.TickCount = &TickCount;
	GIdleTick = &IdleTick;
	if (GTGTPreferred)
	{
		DOUBLE OldTime = appSecondsCompat();
		DOUBLE SecondStartTime = OldTime;
		IdleTickData.OldTime = &OldTime;
		IdleTickData.SecondStartTime = &SecondStartTime;
		while (GIsRunning && !GIsRequestingExit)
		{
			TGTTick(Engine, OldTime, SecondStartTime, TickCount);
			DispatchMessages();
			AdjustThreadPriority(ThreadId, hThread);
		}
	}
	else if (GTimestampPreferred)
	{
		QWORD OldCycles = appCyclesLong();
		QWORD SecondStartCycles = OldCycles;
		IdleTickData.OldCycles = &OldCycles;
		IdleTickData.SecondStartCycles = &SecondStartCycles;
		while (GIsRunning && !GIsRequestingExit)
		{
			TimestampTick(Engine, OldCycles, SecondStartCycles, TickCount);
			DispatchMessages();
			AdjustThreadPriority(ThreadId, hThread);
		}
	}
	else
	{
		DOUBLE OldTime = appSecondsNewNoCheck();
		DOUBLE SecondStartTime = OldTime;
		IdleTickData.OldTime = &OldTime;
		IdleTickData.SecondStartTime = &SecondStartTime;
		while (GIsRunning && !GIsRequestingExit)
		{
			FTimeTick(Engine, OldTime, SecondStartTime, TickCount);
			DispatchMessages();
			AdjustThreadPriority(ThreadId, hThread);
		}
	}
	GIsRunning = 0;

	// Exit main loop.
	guard(ExitMainLoop);
	if( GLogWindow )
		GLogWindow->SetExec( NULL );
	GExec = NULL;
	unguard;

	unguard;
}
#endif

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
