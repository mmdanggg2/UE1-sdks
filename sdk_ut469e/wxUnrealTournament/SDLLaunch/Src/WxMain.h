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
		FirstTimePrompt = new WTextLabel(this, IDC_Prompt, Localize(TEXT("IDDIALOG_ConfigPageFirstTime"), TEXT("IDC_Prompt"), TEXT("Startup")));
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
				if (GFileManager->FileSize(*(FString::Printf(TEXT("./%ls.so"), *Left))) <= 0)
					continue;
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

void wxUnrealTournament::Tick(wxIdleEvent& Event)
{
	if (MainLoopIteration(Args))
	{
		// Enforce optional maximum tick rate.
		guard(EnforceTickRate);
		const FLOAT MaxTickRate = Args->Engine->GetMaxTickRate();
		if( MaxTickRate>0.0 )
		{
			DOUBLE Delta = (1.0/MaxTickRate) - (appSecondsNew()-Args->OldTime);
			appSleepLong( Max(0.0,Delta) );
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
