/*=============================================================================
	BrowserTexture : Browser window for textures
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:
	- needs ability to export to BMP format

=============================================================================*/

#include <stdio.h>

EDITOR_API extern INT GLastScroll;

extern void Query( ULevel* Level, const TCHAR* Item, FString* pOutput, UPackage* InPkg );

// --------------------------------------------------------------
//
// NEW TEXTURE Dialog
//
// --------------------------------------------------------------

class WDlgNewTexture : public WDialog
{
DECLARE_WINDOWCLASS(WDlgNewTexture,WDialog,UnrealEd)

	// Variables.
	WButton OkButton;
	WButton CancelButton;
	WPackageComboBox PackageEdit;
	WComboBox GroupEdit;
	WEdit NameEdit;
	WComboBox ClassCombo;
	WComboBox WidthCombo;
	WComboBox HeightCombo;

	FString defPackage, defGroup;
	TArray<FString>* paFilenames;

	FString Package, Group, Name;

	// Constructor.
	WDlgNewTexture( UObject* InContext, WBrowser* InOwnerWindow )
		: WDialog(TEXT("New Texture"), IDDIALOG_NEW_TEXTURE, InOwnerWindow)
		  , OkButton(this, IDOK, FDelegate(this, (TDelegate)&WDlgNewTexture::OnOk))
		  , CancelButton(this, IDCANCEL, FDelegate(this, (TDelegate)&WDlgNewTexture::EndDialogFalse))
		  , PackageEdit(this, IDEC_PACKAGE)
		  , GroupEdit(this, IDEC_GROUP)
		  , NameEdit(this, IDEC_NAME)
		  , ClassCombo(this, IDCB_CLASS)
		  , WidthCombo(this, IDCB_WIDTH)
		  , HeightCombo(this, IDCB_HEIGHT)
		  , paFilenames(NULL)
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgNewTexture::OnInitDialog);
		WDialog::OnInitDialog();

		SetRedraw(false);

		PackageEdit.Init(*defPackage, &GroupEdit);
		GroupEdit.SetText( *defGroup );
		::SetFocus( NameEdit.hWnd );

		SetupSizeCombo(&WidthCombo, 14, 8); // 14 steps - up to 8192, Item 8 = 256
		SetupSizeCombo(&HeightCombo, 14, 8);

		ClassCombo.SelectionChangeDelegate = FDelegate(this,(TDelegate)& WDlgNewTexture::ClassChange);

		FString Classes;

		Query( GEditor->Level, TEXT("GETCHILDREN CLASS=TEXTURE CONCRETE=1"), &Classes);

		TArray<FString> Array;
		ParseStringToArray( TEXT(","), Classes, &Array );

		for( int x = 0 ; x < Array.Num() ; x++ )
		{
			ClassCombo.AddString( *(Array(x)) );
		}
		ClassCombo.SetCurrent(0);

		PackageEdit.SetText( *defPackage );
		GroupEdit.SetText( *defGroup );

		SetRedraw(true);

		unguard;
	}
	void SetupSizeCombo(WComboBox* Combo, INT ItemsCount, INT Current)
	{
		guard(WDlgNewTexture::SetupSizeCombo);
		Combo->Empty();
		INT Size = 1;
		for (INT i = 0; i < ItemsCount; i++)
		{
			Combo->AddString(*FString::Printf(TEXT("%d"), Size));
			Size *= 2;
		}
		Combo->SetCurrent(Min(ItemsCount - 1, Current));
		unguard;
	}
	void ClassChange()
	{
		guard(WDlgNewTexture::ClassChange);
		FString CurrentClass = ClassCombo.GetString(ClassCombo.GetCurrent()); // 14 steps - up to 8192, 9 - up to 256
		INT MaxSteps = CurrentClass == TEXT("Engine.Texture") || CurrentClass == TEXT("Engine.ScriptedTexture") ? 14 : 9;
		SetupSizeCombo(&WidthCombo, MaxSteps, WidthCombo.GetCurrent());
		SetupSizeCombo(&HeightCombo, MaxSteps, HeightCombo.GetCurrent());
		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgNewTexture::OnDestroy);
		WDialog::OnDestroy();
		unguard;
	}
	virtual int DoModal( FString _defPackage, FString _defGroup)
	{
		guard(WDlgNewTexture::DoModal);
		defPackage = _defPackage;
		defGroup = _defGroup;

		return WDialog::DoModal( hInstance );
		unguard;
	}
	void OnOk()
	{
		guard(WDlgNewTexture::OnOk);
		if( GetDataFromUser() )
		{
			GEditor->Exec( *(FString::Printf( TEXT("TEXTURE NEW NAME=\"%ls\" CLASS=\"%ls\" GROUP=\"%ls\" USIZE=%ls VSIZE=%ls PACKAGE=\"%ls\""),
			                                  *NameEdit.GetText(), *ClassCombo.GetString( ClassCombo.GetCurrent() ), *GroupEdit.GetText(),
			                                  *WidthCombo.GetString( WidthCombo.GetCurrent() ), *HeightCombo.GetString( HeightCombo.GetCurrent() ),
			                                  *PackageEdit.GetText() )));
			EndDialog(TRUE);
		}
		unguard;
	}
	BOOL GetDataFromUser( void )
	{
		guard(WDlgNewTexture::GetDataFromUser);
		Package = PackageEdit.GetText();
		Group = GroupEdit.GetText();
		Name = NameEdit.GetText();

		if( !Package.Len()
			|| !Name.Len() )
		{
			appMsgf( TEXT("Invalid input.") );
			return FALSE;
		}

		return TRUE;
		unguard;
	}
};

// --------------------------------------------------------------
//
// IMPORT TEXTURE Dialog
//
// --------------------------------------------------------------

class WDlgImportTexture : public WDialog
{
DECLARE_WINDOWCLASS(WDlgImportTexture,WDialog,UnrealEd)

	// Variables.
	WButton OkButton;
	WButton OkAllButton;
	WButton SkipButton;
	WButton CancelButton;
	WLabel FilenameStatic;
	WPackageComboBox PackageEdit;
	WComboBox GroupEdit;
	WEdit NameEdit;
	WCheckBox CheckMasked;
#if ENGINE_VERSION==227
	WCheckBox CheckAlphaBlend;
#endif
	WCheckBox CheckMipMap;
	WComboBox CompressionFormat;

	FString defPackage, defGroup;
	TArray<FString>* paFilenames{};

	FString Package, Group, Name;
	BOOL bOKToAll{};
	int iCurrentFilename{};
	int CompressionDXT{};

	// Constructor.
	WDlgImportTexture( UObject* InContext, WBrowser* InOwnerWindow )
		:	WDialog			( TEXT("Import Texture"), IDDIALOG_IMPORT_TEXTURE, InOwnerWindow )
	    ,	OkButton		( this, IDOK,			FDelegate(this,(TDelegate)&WDlgImportTexture::OnOk) )
	    ,	OkAllButton		( this, IDPB_OKALL,		FDelegate(this,(TDelegate)&WDlgImportTexture::OnOkAll) )
	    ,	SkipButton		( this, IDPB_SKIP,		FDelegate(this,(TDelegate)&WDlgImportTexture::OnSkip) )
	    ,	CancelButton	( this, IDCANCEL,		FDelegate(this,(TDelegate)&WDlgImportTexture::EndDialogFalse) )
	    ,	PackageEdit		( this, IDEC_PACKAGE )
	    ,	GroupEdit		( this, IDEC_GROUP )
	    ,	NameEdit		( this, IDEC_NAME )
	    ,	FilenameStatic	( this, IDSC_FILENAME )
	    ,	CheckMasked		( this, IDCK_MASKED )
#if ENGINE_VERSION==227
		,	CheckAlphaBlend	( this, IDCK_ALPHABLEND)
#endif
	    ,	CheckMipMap		( this, IDCK_MIPMAP )
	    ,	CompressionFormat ( this, IDCB_COMPRESS)
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgImportTexture::OnInitDialog);
		WDialog::OnInitDialog();

		SetRedraw(false);

		PackageEdit.Init(*defPackage, &GroupEdit);
		GroupEdit.SetText(*defGroup);
		::SetFocus(NameEdit.hWnd);

		bOKToAll = FALSE;
		iCurrentFilename = -1;
		SetNextFilename();
		CheckMipMap.SetCheck(BST_CHECKED);
		CompressionFormat.AddString(TEXT("None"));
		CompressionFormat.AddString(TEXT("Palette (256 colors)"));
		CompressionFormat.AddString(TEXT("BC1/DXT1 (24bpp)"));
		CompressionFormat.AddString(TEXT("BC2/DXT3 (32bpp)"));
		CompressionFormat.AddString(TEXT("BC3/DXT5 (32bpp)"));
#if ENGINE_VERSION==227
		CompressionFormat.AddString(TEXT("BC4  (32bpp)"));
		CompressionFormat.AddString(TEXT("BC5  (32bpp)"));
		//CompressionFormat.AddString(TEXT("BC6H (32bpp)"));
#endif
		CompressionFormat.AddString(TEXT("BC7  (32bpp)"));
		CompressionFormat.SelectionChangeDelegate = FDelegate(this, (TDelegate)&WDlgImportTexture::OnCompressionLevelChange);
		CompressionFormat.SetCurrent(0);
		OnCompressionLevelChange();

		SetRedraw(true);

		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgImportTexture::OnDestroy);
		WDialog::OnDestroy();
		unguard;
	}
	virtual int DoModal( FString _defPackage, FString _defGroup, TArray<FString>* _paFilenames)
	{
		guard(WDlgImportTexture::DoModal);
		defPackage = _defPackage;
		defGroup = _defGroup;
		paFilenames = _paFilenames;

		return WDialog::DoModal( hInstance );
		unguard;
	}
	void OnOk()
	{
		guard(WDlgImportTexture::OnOk);
		if( GetDataFromUser() )
		{
			ImportFile( (*paFilenames)(iCurrentFilename) );
			SetNextFilename();
		}
		unguard;
	}
	void OnOkAll()
	{
		guard(WDlgImportTexture::OnOkAll);
		if( GetDataFromUser() )
		{
			ImportFile( (*paFilenames)(iCurrentFilename) );
			bOKToAll = TRUE;
			SetNextFilename();
		}
		unguard;
	}
	void OnSkip()
	{
		guard(WDlgImportTexture::OnSkip);
		if( GetDataFromUser() )
			SetNextFilename();
		unguard;
	}
	void OnCompressionLevelChange(void)
	{
		guard(WDlgImportTexture::OnCompressionLevelChange);
		switch (CompressionFormat.GetCurrent())
		{
		case 0:
			CompressionDXT = 0;
			break;
		case 1:
			CompressionDXT = -1;
			break;
		case 2:
			CompressionDXT = 1;
			break;
		case 3:
			CompressionDXT = 2;
			break;
		case 4:
			CompressionDXT = 3;
			break;
#if ENGINE_VERSION==227
		case 5:
			CompressionDXT = 4;
			break;
		case 6:
			CompressionDXT = 5;
			break;
		case 7:
			CompressionDXT = 7;
			break;
		case 8:
			CompressionDXT = 7;
			break;
#else
		case 5:
			CompressionDXT = 7;
			break;
#endif
		}
		unguard;
	}
	void RefreshName( void )
	{
		guard(WDlgImportTexture::RefreshName);
		FilenameStatic.SetText( *(*paFilenames)(iCurrentFilename) );
		FString Name = GetFilenameOnly( (*paFilenames)(iCurrentFilename) );
		NameEdit.SetText( *Name );
		unguard;
	}
	BOOL GetDataFromUser( void )
	{
		guard(WDlgImportTexture::GetDataFromUser);
		Package = PackageEdit.GetText();
		Group = GroupEdit.GetText();
		Name = NameEdit.GetText();

		if( !Package.Len()
			|| !Name.Len() )
		{
			appMsgf( TEXT("Invalid input.") );
			return FALSE;
		}

		return TRUE;
		unguard;
	}
	void SetNextFilename( void )
	{
		guard(WDlgImportTexture::SetNextFilename);
		iCurrentFilename++;
		if( iCurrentFilename == paFilenames->Num() )
		{
			EndDialogTrue();
			return;
		}

		if( bOKToAll )
		{
			RefreshName();
			GetDataFromUser();
			ImportFile( (*paFilenames)(iCurrentFilename) );
			SetNextFilename();
			return;
		}

		RefreshName();
		unguard;
	}
	void ImportFile( FString Filename )
	{
		guard(WDlgImportTexture::ImportFile);
#if ENGINE_VERSION==227
		if (CheckAlphaBlend.IsChecked() && CheckMasked.IsChecked())
			debugf(TEXT("Masking has been disabled because AlphaBlending is enabled"));
		DWORD Flags = CheckAlphaBlend.IsChecked() ? PF_AlphaBlend : (CheckMasked.IsChecked() ? PF_Masked : 0);
#else
		DWORD Flags = (CheckMasked.IsChecked() ? PF_Masked : 0);
#endif
		FString Cmd = Group.Len() > 0
			              ? FString::Printf(TEXT("TEXTURE IMPORT FILE=\"%ls\" NAME=\"%ls\" PACKAGE=\"%ls\" GROUP=\"%ls\" MIPS=%d FLAGS=%d BTC=%d"), *Filename, *Name, *Package, *Group, CheckMipMap.IsChecked(), Flags, CompressionDXT)
			              : FString::Printf(TEXT("TEXTURE IMPORT FILE=\"%ls\" NAME=\"%ls\" PACKAGE=\"%ls\" MIPS=%d FLAGS=%d BTC=%d"), *Filename, *Name, *Package, CheckMipMap.IsChecked(), Flags, CompressionDXT);
		debugf(TEXT("%ls"), *Cmd);
		if (!GEditor->Exec(*Cmd))
			appMsgf(TEXT("Import of file %ls failed"), *Filename);
		GEditor->Flush(1);
		unguard;
	}
};


// --------------------------------------------------------------
//
// IMPORT COMPMIPS Dialog
//
// --------------------------------------------------------------

class WDlgImportCompMips : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgImportCompMips,WDialog,UnrealEd)

	// Variables.
	WButton OkButton;
	WButton CancelButton;
	WLabel TextureNameStatic;
	WLabel FilenameStatic;
	WComboBox CompressionFormat;

	struct FSupportedFormat
	{
		FString Name;
		FString Code;

		FSupportedFormat( const TCHAR* InName, const TCHAR* InCode)
			: Name(InName), Code(InCode)
		{}
	};
	TArray<FSupportedFormat> SupportedFormats;
	FString TextureName;
	FString Filename;
	FString SelectedFormat;

	// Constructor.
	WDlgImportCompMips( UObject* InContext, WBrowser* InOwnerWindow )
		:	WDialog          ( TEXT("Import Compressed Mips"), IDDIALOG_IMPORT_COMPMIPS, InOwnerWindow )
		,	OkButton         ( this, IDOK,			FDelegate(this,(TDelegate)&WDlgImportCompMips::EndDialogTrue) )
		,	CancelButton     ( this, IDCANCEL,		FDelegate(this,(TDelegate)&WDlgImportCompMips::EndDialogFalse) )
		,	TextureNameStatic( this, IDSC_TEXTURE)
		,	FilenameStatic   ( this, IDSC_FILENAME )
		,	CompressionFormat( this, IDCB_COMPRESS )
	{
		SupportedFormats.AddItem( FSupportedFormat(TEXT("Automatic (BC1/BC3)"), TEXT("")) );
		SupportedFormats.AddItem( FSupportedFormat(TEXT("BC1/DXT1 (S3TC)")    , TEXT("BC1")) );
		SupportedFormats.AddItem( FSupportedFormat(TEXT("BC2/DXT3 (S3TC)")    , TEXT("BC2")) );
		SupportedFormats.AddItem( FSupportedFormat(TEXT("BC3/DXT5 (S3TC)")    , TEXT("BC3")) );
		SupportedFormats.AddItem( FSupportedFormat(TEXT("BC7/DXT5 (BPTC)")    , TEXT("BC7")) );
		TextureName = *FObjectPathName(InContext);
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgImportCompMips::OnInitDialog);
		WDialog::OnInitDialog();

		SetRedraw(false);
	
		TextureNameStatic.SetText( *TextureName );
		FilenameStatic.SetText(*Filename);
		for ( INT i=0; i<SupportedFormats.Num(); i++)
			CompressionFormat.AddString( *SupportedFormats(i).Name );
		CompressionFormat.SelectionChangeDelegate = FDelegate(this,(TDelegate)&WDlgImportCompMips::OnChangedFormat);
		CompressionFormat.SetCurrent(0);

		SetRedraw(true);

		unguard;
	}
	void OnClose()
	{
		guard(WDlgImportCompMips::OnClose);
		Show(0);
		unguard;
	}
	void OnChangedFormat()
	{
		guard(WDlgImportCompMips::OnChangedFormat);
		debugf( TEXT("OnChangedFormat: %i/%i"), CompressionFormat.GetCurrent(), SupportedFormats.Num() );
		if ( SupportedFormats.IsValidIndex(CompressionFormat.GetCurrent()) )
			SelectedFormat = SupportedFormats(CompressionFormat.GetCurrent()).Code;
		unguard;
	}
	virtual int DoModal( FString InFilename)
	{
		guard(WDlgImportCompMips::DoModal);
		Filename = InFilename;

		return WDialog::DoModal( hInstance );
		unguard;
	}
};


// --------------------------------------------------------------
//
// WBrowserTexture
//
// --------------------------------------------------------------

#define ID_BT_TOOLBAR	29040
TBBUTTON tbBTButtons[] = {
	{ 0, IDMN_MB_DOCK, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
	{ 1, ID_FileOpen, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 2, ID_FileSave, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 3, IDMN_LOAD_ENTIRE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
	{ 4, IDPB_EDIT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
	{ 5, IDMN_TB_PREV_GRP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 6, IDMN_TB_NEXT_GRP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
	{ 7, IDMN_TB_IN_USE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
	{ 8, IDMN_TB_REALTIME, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
};
struct {
	TCHAR ToolTip[64];
	int ID;
} ToolTips_BT[] = {
	TEXT("Toggle Dock Status"), IDMN_MB_DOCK,
	TEXT("Open Package"), ID_FileOpen,
	TEXT("Save Package"), ID_FileSave,
	TEXT("Load Entire Package"), IDMN_LOAD_ENTIRE,
	TEXT("Texture Properties"), IDPB_EDIT,
	TEXT("Previous Group"), IDMN_TB_PREV_GRP,
	TEXT("Next Group"), IDMN_TB_NEXT_GRP,
	TEXT("Show All Textures in use"), IDMN_TB_IN_USE,
	TEXT("Show Real-time preview"), IDMN_TB_REALTIME,
	NULL, 0
};

int CDECL ResNameCompare2(const void* A, const void* B)
{
	return appStricmp(*FObjectName(*(UObject**)A), *FObjectName(*(UObject**)B));
}

class WBrowserTexture : public WBrowser, public WViewportWindowContainer
{
DECLARE_WINDOWCLASS(WBrowserTexture,WBrowser,Window)

	TArray<WDlgTexProp> PropWindows;

	WComboBox *pComboPackage, *pComboGroup;
	WCheckBox *pCheckGroupAll, *pInUseOnly;
	WLabel *pLabelFilter;
	WEdit *pEditFilter;
	WVScrollBar* pScrollBar;
	HWND hWndToolBar{};
	WToolTip *ToolTipCtrl{};
	MRUList* mrulist;
	HBITMAP ToolbarImage{};

	WTabControl* ModeTabs = NULL;

	enum
	{
		MODE_BY_PACKAGE,
		MODE_IN_USE,
		MODE_ALL,
		MODE_MRU,
		MODE_MAX
	};

	INT ModeTab = MODE_BY_PACKAGE;
	INT LastScroll[MODE_MAX]{};

	enum {MRU_MAX_TEXTURES = 64};

	TArray<UTexture*> MRU;

	int iZoom, iScroll;

	UBOOL bListInUse;

	inline UObject* GetParentOuter( UObject* O )
	{
		while( O->GetOuter() )
			O = O->GetOuter();
		return O;
	}
	inline bool HasValidGroup( UObject* O )
	{
		if( !O->GetOuter() || !O->GetOuter()->GetOuter() )
			return false;
		return true;
	}

	// Structors.
	WBrowserTexture( FName InPersistentName, WWindow* InOwnerWindow, HWND InEditorFrame )
	:	WBrowser( InPersistentName, InOwnerWindow, InEditorFrame )
	,	WViewportWindowContainer(TEXT("TextureBrowser"), *InPersistentName)
	{
		pComboPackage = pComboGroup = NULL;
		pCheckGroupAll = pInUseOnly = NULL;
		pLabelFilter = NULL;
		pEditFilter = NULL;
		pScrollBar = NULL;
		iZoom = 128;
		iScroll = 0;
		MenuID = IDMENU_BrowserTexture;
		BrowserID = eBROWSER_TEXTURE;
		Description = TEXT("Textures");
		mrulist = NULL;
		bListInUse = false;
	}

	// WBrowser interface.
	void OpenWindow( UBOOL bChild )
	{
		guard(WBrowserTexture::OpenWindow);
		WBrowser::OpenWindow( bChild );
		SetCaption();
		unguard;
	}
	void OnCreate()
	{
		guard(WBrowserTexture::OnCreate);
		WBrowser::OnCreate();

		SetRedraw(false);

		ViewportOwnerWindow = hWnd;

		SetMenu( hWnd, AddHotkeys(LoadMenuW(hInstance, MAKEINTRESOURCEW(IDMENU_BrowserTexture))) );

		ModeTabs = new WTabControl(this, IDCB_MODE_TABS);
		ModeTabs->OpenWindow(1); 
		// Tabs added in reversed order, last become at index 0, and render left-most
		ModeTabs->AddTab(TEXT("MRU"), MODE_MRU);
		ModeTabs->AddTab(TEXT("All"), MODE_ALL);
		ModeTabs->AddTab(TEXT("In Use"), MODE_IN_USE);
		ModeTabs->AddTab(TEXT("By Package"), MODE_BY_PACKAGE);
		ModeTabs->SetCurrent(MODE_BY_PACKAGE);
		ModeTabs->SelectionChangeDelegate = FDelegate(this, (TDelegate)&WBrowserTexture::OnModeTabSelChange);

		// PACKAGES
		//
		pComboPackage = new WComboBox( this, IDCB_PACKAGE );
		pComboPackage->OpenWindow( 1, 1 );
		pComboPackage->SelectionChangeDelegate = FDelegate(this, (TDelegate)&WBrowserTexture::OnComboPackageSelChange);

		// GROUP
		//
		pComboGroup = new WComboBox( this, IDCB_GROUP );
		pComboGroup->OpenWindow( 1, 1 );
		pComboGroup->SelectionChangeDelegate = FDelegate(this, (TDelegate)&WBrowserTexture::OnComboGroupSelChange);

		// CHECK BOXES
		//
		pCheckGroupAll = new WCheckBox( this, IDCK_GRP_ALL, FDelegate(this, (TDelegate)&WBrowserTexture::OnGroupAllClick) );
		pCheckGroupAll->OpenWindow( 1, 0, 0, 1, 1, TEXT("All Groups"), 1, 0, BS_PUSHLIKE );
		pInUseOnly = new WCheckBox(this, IDCK_IN_USE_ONLY, FDelegate(this, (TDelegate)&WBrowserTexture::OnInUseOnlyClick));
		pInUseOnly->OpenWindow(1, 0, 0, 1, 1, TEXT("In Use Only"), 1, 0, BS_PUSHLIKE);

		// LABELS
		//
		pLabelFilter = new WLabel( this, IDST_FILTER );
		pLabelFilter->OpenWindow( 1, 0 );
		pLabelFilter->SetText( TEXT("Filter : ") );
	
		// EDIT CONTROLS
		//
		pEditFilter = new WEdit( this, IDEC_FILTER );
		pEditFilter->OpenWindow( 1, 0, 0 );
		pEditFilter->SetText( TEXT("") );
		pEditFilter->ChangeDelegate = FDelegate(this, (TDelegate)&WBrowserTexture::OnEditFilterChange);

		// SCROLLBARS
		//
		pScrollBar = new WVScrollBar( this, IDSB_SCROLLBAR );
		pScrollBar->OpenWindow( 1, 0, 0, 320, 200 );

		// Create the texture browser viewport
		//
		FString Device;
		GConfig->GetString(*ConfigName, TEXT("Device"), Device, GUEDIni);
		CreateViewport(SHOW_StandardView | SHOW_NoButtons | SHOW_ChildWindow, REN_TexBrowser, 320, 200, -1, -1, *Device);
		check(pViewport && pViewport->Actor);
		if (!GConfig->GetInt(*PersistentName, TEXT("Zoom"), iZoom, GUEDIni))
			iZoom = 128;
		pViewport->Actor->Misc1 = iZoom;
		pViewport->Actor->Misc2 = iScroll;

		// Force a repaint as we only set the zoom and scroll after creating the viewport
		pViewport->Repaint(1);

		ToolbarImage = reinterpret_cast<HBITMAP>(LoadImage(hInstance,
		                                                   MAKEINTRESOURCE(IDB_BrowserTexture_TOOLBAR),
		                                                   IMAGE_BITMAP, 0, 0, 0));
		check(ToolbarImage);
		ScaleImageAndReplace(ToolbarImage);

		hWndToolBar = CreateToolbarEx( 
			hWnd, WS_CHILD | WS_VISIBLE | CCS_ADJUSTABLE,
			IDB_BrowserTexture_TOOLBAR,
			9,
			NULL,
			reinterpret_cast<PTRINT>(ToolbarImage),
			(LPCTBBUTTON)&tbBTButtons,
			14,
			MulDiv(16, DPIX, 96), MulDiv(16, DPIY, 96),
			MulDiv(16, DPIX, 96), MulDiv(16, DPIY, 96),
			sizeof(TBBUTTON));
		check(hWndToolBar);

		ToolTipCtrl = new WToolTip(this);
		ToolTipCtrl->OpenWindow();
		for( int tooltip = 0 ; ToolTips_BT[tooltip].ID > 0 ; tooltip++ )
		{
			// Figure out the rectangle for the toolbar button.
			int index = SendMessageW( hWndToolBar, TB_COMMANDTOINDEX, ToolTips_BT[tooltip].ID, 0 );
			RECT rect;
			SendMessageW( hWndToolBar, TB_GETITEMRECT, index, (LPARAM)&rect);

			ToolTipCtrl->AddTool( hWndToolBar, ToolTips_BT[tooltip].ToolTip, tooltip, &rect );
		}

		mrulist = new MRUList( *PersistentName );
		mrulist->ReadINI();
		if( GBrowserMaster->GetCurrent()==BrowserID )
			mrulist->AddToMenu( hWnd, GetMenu( IsDocked() ? OwnerWindow->hWnd : hWnd ) );

		PositionChildControls();
		RefreshPackages();
		RefreshGroups();
		RefreshTextureList();

		SetCaption();

		SetRedraw(true);
		
		unguard;
	}
	void OnDestroy()
	{
		guard(WBrowserTexture::OnDestroy);
		GConfig->SetInt( *PersistentName, TEXT("Zoom"), iZoom, GUEDIni );

		delete pComboPackage;
		delete pComboGroup;
		delete pCheckGroupAll;
		delete pInUseOnly;
		delete pLabelFilter;
		delete pEditFilter;
		delete pScrollBar;

		delete ModeTabs;

		::DestroyWindow( hWndToolBar );
		delete ToolTipCtrl;

		mrulist->WriteINI();
		delete mrulist;

		// Clean up all open texture property windows.
		for( int x = 0 ; x < PropWindows.Num() ; x++ )
		{
			::DestroyWindow( PropWindows(x).hWnd );
			delete PropWindows(x);
		}

		GEditor->Exec( TEXT("CAMERA CLOSE NAME=TextureBrowser") );
		delete pViewport;

		if (ToolbarImage)
			DeleteObject(ToolbarImage);

		WBrowser::OnDestroy();
		unguard;
	}
	virtual void UpdateMenu()
	{
		guard(WBrowserTexture::UpdateMenu);
		HMENU menu = GetMenu( IsDocked() ? OwnerWindow->hWnd : hWnd );

		BOOL bRealTime = (pViewport && pViewport->Actor && (pViewport->Actor->ShowFlags & SHOW_RealTime));

		CheckMenuItem( menu, IDMN_TB_IN_USE, MF_BYCOMMAND | (bListInUse ? MF_CHECKED : MF_UNCHECKED) );
		CheckMenuItem( menu, IDMN_TB_ZOOM_32, MF_BYCOMMAND | ((iZoom == 32) ? MF_CHECKED : MF_UNCHECKED) );
		CheckMenuItem( menu, IDMN_TB_ZOOM_64, MF_BYCOMMAND | ((iZoom == 64) ? MF_CHECKED : MF_UNCHECKED) );
		CheckMenuItem( menu, IDMN_TB_ZOOM_128, MF_BYCOMMAND | ((iZoom == 128) ? MF_CHECKED : MF_UNCHECKED) );
		CheckMenuItem( menu, IDMN_TB_ZOOM_256, MF_BYCOMMAND | ((iZoom == 256) ? MF_CHECKED : MF_UNCHECKED) );
		CheckMenuItem( menu, IDMN_TB_ZOOM_512, MF_BYCOMMAND | ((iZoom == 512) ? MF_CHECKED : MF_UNCHECKED) );
		CheckMenuItem( menu, IDMN_TB_ZOOM_1024, MF_BYCOMMAND | ((iZoom == 1024) ? MF_CHECKED : MF_UNCHECKED) );
		CheckMenuItem( menu, IDMN_VAR_200, MF_BYCOMMAND | ((iZoom == 1200) ? MF_CHECKED : MF_UNCHECKED) );
		CheckMenuItem( menu, IDMN_VAR_100, MF_BYCOMMAND | ((iZoom == 1100) ? MF_CHECKED : MF_UNCHECKED) );
		CheckMenuItem( menu, IDMN_VAR_50, MF_BYCOMMAND | ((iZoom == 1050) ? MF_CHECKED : MF_UNCHECKED) );
		CheckMenuItem( menu, IDMN_VAR_25, MF_BYCOMMAND | ((iZoom == 1025) ? MF_CHECKED : MF_UNCHECKED) );
		CheckMenuItem( menu, IDMN_MB_DOCK, MF_BYCOMMAND | (IsDocked() ? MF_CHECKED : MF_UNCHECKED) );
		CheckMenuItem( menu, IDMN_TB_REALTIME, MF_BYCOMMAND | bRealTime ? MF_CHECKED : MF_UNCHECKED);
		UpdateRendererMenu(menu, FALSE);
		
		SendMessage(hWndToolBar, TB_SETSTATE , IDMN_TB_IN_USE, bListInUse ? TBSTATE_CHECKED | TBSTATE_ENABLED : TBSTATE_ENABLED);
		SendMessage(hWndToolBar, TB_SETSTATE , IDMN_TB_REALTIME, bRealTime ? TBSTATE_CHECKED | TBSTATE_ENABLED : TBSTATE_ENABLED);

		if( mrulist 
			&& GBrowserMaster->GetCurrent()==BrowserID )
			mrulist->AddToMenu( hWnd, GetMenu( IsDocked() ? OwnerWindow->hWnd : hWnd ) );
		unguard;
	}
	virtual bool ShouldBlockAccelerators()
	{
		return pEditFilter && pEditFilter->hWnd && GetFocus() == pEditFilter->hWnd;
	}
	void OnCommand( INT Command )
	{
		guard(WBrowserTexture::OnCommand);
		switch( Command )
		{
		case ID_FileNew:
		{
			FString Package = pComboPackage->GetString( pComboPackage->GetCurrent() );
			FString Group = pComboGroup->GetString( pComboGroup->GetCurrent() );

			WDlgNewTexture l_dlg( NULL, this );
			if( l_dlg.DoModal( Package, Group ) )
			{
				RefreshPackages();
				pComboPackage->SetCurrent( pComboPackage->FindStringExact( *l_dlg.Package) );
				RefreshGroups();
				pComboGroup->SetCurrent( pComboGroup->FindStringExact( *l_dlg.Group) );
				RefreshTextureList();

				// Call up the properties on this new texture.
				WDlgTexProp* pDlgTexProp = new(PropWindows)WDlgTexProp(NULL, OwnerWindow, GEditor->CurrentTexture );
				pDlgTexProp->DoModeless();
				pDlgTexProp->pProps->SetNotifyHook( GEditor );
			}
		}
		break;

		case IDPB_EDIT:
		{
			if( GEditor->CurrentTexture )
			{
				// make sure to remove old unused PropWindows before creating any new.
				for( int x = 0 ; x < PropWindows.Num() ; x++ )
				{
					if (!PropWindows(x))
					{
						PropWindows.Remove(x);
						x--;
					}
				}
				WDlgTexProp* pDlgTexProp = new(PropWindows)WDlgTexProp(NULL, OwnerWindow, GEditor->CurrentTexture );
				pDlgTexProp->DoModeless();
				pDlgTexProp->pProps->SetNotifyHook( GEditor );
				RefreshAll();
			}
		}
		break;

		case IDMN_TB_SETCOMPRESS_BC1:
		case IDMN_TB_SETCOMPRESS_BC2:
		case IDMN_TB_SETCOMPRESS_BC3:
		case IDMN_TB_SETCOMPRESS_BC7:
		case IDMN_TB_SETCOMPRESS_P8:
		{
			const ETextureFormat CommandToFormat[5] = { TEXF_BC1, TEXF_BC2, TEXF_BC3, TEXF_BC7, TEXF_P8 };
			ETextureFormat ToFormat = CommandToFormat[Command - IDMN_TB_SETCOMPRESS_BC1];
			UTexture* Texture = GEditor->CurrentTexture;
			if (Texture)
			{
				if (Texture->CompFormat == ToFormat)
				{
					appMsgf(TEXT("Texture is already in the requested format"));
					break;
				}
				Texture->Compress(ToFormat, (Texture->Mips.Num() > 1));
				FlushRepaintViewport();
			}
		}
		break;

		case IDMN_TB_SETCOMPRESS_NONE:
		{
			UTexture* Texture = GEditor->CurrentTexture;
			if (Texture)
			{
				if (!Texture->bHasComp)
				{
					appMsgf(TEXT("Texture doesn't have CompMips"));
					break;
				}
				Texture->CompMips.Empty();
				Texture->bHasComp = false;
				Texture->CompFormat = TEXF_P8;
				FlushRepaintViewport();
			}
		}
		break;

		case IDMN_TB_CONVERT_P8:
		case IDMN_TB_CONVERT_RGB8:
		case IDMN_TB_CONVERT_BGRA8:
		case IDMN_TB_CONVERT_RGBA8:
		{
			const ETextureFormat CommandToFormat[4] = { TEXF_P8, TEXF_RGB8, TEXF_BGRA8, TEXF_RGBA8_ };
			ETextureFormat ToFormat = CommandToFormat[Command - IDMN_TB_CONVERT_P8];
			UTexture* Texture = GEditor->CurrentTexture;
			if (Texture)
			{
				if (Texture->Format == ToFormat)
				{
					appMsgf(TEXT("Texture is already in the requested format"));
					break;
				}

				if (!Texture->ConvertFormat(ToFormat, Texture->Mips.Num() > 1))
				{
					appMsgf(TEXT("Texture conversion failed"));
				}
				else
				{
					FlushRepaintViewport();
				}
			}
		}
		break;
			
		case IDMN_TB_SETMIPS_ADD:
		{
			if (GEditor->CurrentTexture)
				GEditor->CurrentTexture->CreateMips(1, 1
#if ENGINE_VERSION == 227
				, 0
#endif
				);
		}
		break;

		case IDMN_TB_SETMIPS_DEL:
		{
			if (GEditor->CurrentTexture)
				GEditor->CurrentTexture->CreateMips(0, 0
#if ENGINE_VERSION == 227
				, 0
#endif
				);
		}
		break;

#if UNREAL_TOURNAMENT_OLDUNREAL
		case IDMN_TB_IMPORT_COMPMIPS:
		{
			UTexture* Texture = GEditor->CurrentTexture;
			if ( Texture && !Texture->bParametric )
			{
				WDlgImportCompMips dlg( Texture, this);
				TArray<FString> Files;
				if (OpenFilesWithDialog(
					*(GLastDir[eLASTDIR_PCX]),
					TEXT("Image Files (*.pcx,*.bmp,*.dds,*.png)\0*.pcx;*.bmp;*.dds;*.png;\0 PCX Files\0*.pcx;\0 24/32bpp BMP Files\0*.bmp;\0 DDS Files\0*.dds;\0 24/32bpp PNG Files\0*.png;\0All Files\0*.*\0\0"),
					TEXT("*.pcx;*.bmp;*.dds;*.png;"),
					TEXT("Import CompMips"),
					Files,
					0
				) && (Files.Num() == 1) && dlg.DoModal(Files(0)) )
				{
					UPackage* Package = nullptr;
					for ( UObject* O=Texture->GetOuter(); O; O=O->GetOuter() )
						Package = Cast<UPackage>(O);

					if ( Package )
					{
						FString Cmd = FString::Printf(TEXT("TEXTURE MERGECOMPRESSED FILE=%s PACKAGE=%s NAME=%s FORMAT=%s"), *Files(0), Package->GetName(), Texture->GetName(), *dlg.SelectedFormat);
						if ( Cast<UPackage>(Texture->GetOuter()) != Package )
							Cmd += FString::Printf( TEXT(" GROUP=%s"),Texture->GetOuter()->GetName());
						GLog->Log(*Cmd);
						INT Imported = GEditor->Exec( *Cmd);
						if ( Imported )
						{
							appMsgf(TEXT("Successfully imported compressed image data to texture."));
							FlushRepaintViewport();
						}
					}
					GLastDir[eLASTDIR_PCX] = appFilePathName(*Files(0));
					RefreshTextureList();
				}

				GFileManager->SetDefaultDirectory(appBaseDir());
			}
		}
		break;
#endif
		case ID_EditDelete:
		case IDMN_TB_DELETE:
		{
			if (!GEditor->CurrentTexture)
				break;
			UTexture* Tex = GEditor->CurrentTexture;
			GEditor->CurrentTexture = NULL;
			FString Name = FObjectName(Tex);
			TArray<UObject*> Res;
			FCheckObjRefArc Ref(&Res, Tex, false);
			INT Idx = ForcedTexList.FindItemIndex(Tex);
			if (Idx != INDEX_NONE)
				ForcedTexList(Idx) = NULL;
			BOOL InMRU = RemoveMRU(Tex);

			if (Ref.HasReferences())
			{
				if (InMRU)
					AddMRU(Tex);
				if (Idx != INDEX_NONE)
					ForcedTexList(Idx) = Tex;
				GEditor->CurrentTexture = Tex;
				FString RefStr = *FObjectFullName(Res(0));
				for (INT i = 1; i < Min<INT>(4, Res.Num()); i++)
				{
					RefStr += TEXT(", ");
					RefStr += *FObjectFullName(Res(i));
					if (i == 3 && Res.Num() > 4)
						RefStr += TEXT(", etc...");
				}
				appMsgf(TEXT("Texture %ls is still being referenced by: %ls"), *FObjectName(Tex), *RefStr);
			}
			else
			{
				Tex->ConditionalDestroy();
				delete Tex;
				RefreshPackages();
				RefreshGroups();
				RefreshTextureList();
			}
		}
		break;

		case IDMN_TB_PREV_GRP:
		{
			int Sel = pComboGroup->GetCurrent();
			Sel--;
			if( Sel < 0 ) Sel = pComboGroup->GetCount() - 1;
			pComboGroup->SetCurrent(Sel);
			RefreshTextureList();
		}
		break;

		case IDMN_TB_NEXT_GRP:
		{
			int Sel = pComboGroup->GetCurrent();
			Sel++;
			if( Sel >= pComboGroup->GetCount() ) Sel = 0;
			pComboGroup->SetCurrent(Sel);
			RefreshTextureList();
		}
		break;

		case IDMN_TB_IN_USE:
		{
			OnInUseOnlyClick();
		}
		break;

		case IDMN_TB_REALTIME:
		{
			pViewport->Actor->ShowFlags ^= SHOW_RealTime;
			UpdateMenu();
		}
		break;

		case IDMN_TB_RENAME:
		{
			if( !GEditor->CurrentTexture ) break;

			WDlgRename dlg( NULL, this );
			if( dlg.DoModal(*FObjectName(GEditor->CurrentTexture) ) )
			{
				GEditor->CurrentTexture->Rename( *dlg.NewName );

			}
			RefreshTextureList();
		}
		break;

		case IDMN_TB_REMOVE:
		{
			if( !GEditor->CurrentTexture ) 
				break;
			GEditor->Key(pViewport, IK_Q
#if ENGINE_VERSION==227
				, 0
#endif
			);
		}
		break;

		case ID_SurfPopupSelectMatchingTexture:
		{
			guard(polySelectMatchingTexture);
			if (!GEditor->CurrentTexture || !GEditor->Level || !GEditor->Level->Model)
				break;
			DWORD SelectedIndex = GEditor->CurrentTexture->GetIndex();
			for (INT i = 0; i < GEditor->Level->Model->Surfs.Num(); i++)
			{
				FBspSurf *Poly = &GEditor->Level->Model->Surfs(i);
				if (Poly->Texture && Poly->Texture->GetIndex() == SelectedIndex && !(Poly->PolyFlags & PF_Selected))
				{
					GEditor->Level->Model->ModifySurf( i, 0 );
					Poly->PolyFlags |= PF_Selected;
				}
			};
			GEditor->RedrawLevel(GEditor->Level);
			unguard;
		}
		break;

		case IDMN_TB_CULL:
		{
			GEditor->Exec( TEXT("TEXTURE CULL") );
			appMsgf(TEXT("Texture cull complete. Check the log file for a detailed report."));
		}
		break;

		case IDMN_TB_REMIP_PACKAGE:
		{
			FString Package = pComboPackage->GetString( pComboPackage->GetCurrent() );
			FString Cmd = FString::Printf(TEXT("OBJ REMIP PACKAGE=%ls"), *Package);
			GEditor->Exec( *Cmd );
			appMsgf(TEXT("Mipmaps have been regenerated for all textures in package %ls.\nDon't forget to save the package."), *Package);
		}
		break;

		case IDMN_TB_REMIP_TEXTURE:
		{
			FString Texture = *FObjectPathName(GEditor->CurrentTexture);
			FString Cmd = FString::Printf(TEXT("TEXTURE REMIP NAME=%ls"), *Texture);
			GEditor->Exec( *Cmd );
			appMsgf(TEXT("Mipmaps have been regenerated for texture %ls.\nDon't forget to save the package."), *Texture);
		}
		break;

		case IDMN_LOAD_ENTIRE:
		{
			FString Package = pComboPackage->GetString( pComboPackage->GetCurrent() );
			UObject::LoadPackage(NULL,*Package,LOAD_NoWarn);
			GBrowserMaster->RefreshAll();
		}
		break;

		case IDMN_TB_ZOOM_32:
			iZoom = 32;
			RefreshTextureList(GLastScroll);
			break;

		case IDMN_TB_ZOOM_64:
			iZoom = 64;
			RefreshTextureList(GLastScroll);
			break;

		case IDMN_TB_ZOOM_128:
			iZoom = 128;
			RefreshTextureList(GLastScroll);
			break;

		case IDMN_TB_ZOOM_256:
			iZoom = 256;
			RefreshTextureList(GLastScroll);
			break;

		case IDMN_TB_ZOOM_512:
			iZoom = 512;
			RefreshTextureList(GLastScroll);
			break;

		case IDMN_TB_ZOOM_1024:
			iZoom = 1024;
			RefreshTextureList(GLastScroll);
			break;

		case IDMN_VAR_200:
			iZoom = 1200;
			RefreshTextureList(GLastScroll);
			break;

		case IDMN_VAR_100:
			iZoom = 1100;
			RefreshTextureList(GLastScroll);
			break;

		case IDMN_VAR_50:
			iZoom = 1050;
			RefreshTextureList(GLastScroll);
			break;

		case IDMN_VAR_25:
			iZoom = 1025;
			RefreshTextureList(GLastScroll);
			break;

		case ID_FileOpen:
		{
			TArray<FString> Files;
			if (OpenFilesWithDialog(
				*(GLastDir[eLASTDIR_UTX]),
				TEXT("Texture Packages (*.utx)\0*.utx\0All Files\0*.*\0\0"),
				TEXT("utx"),
				TEXT("Open Texture Package"),
				Files
			))
			{
				if( Files.Num() > 0 )
				{
					GLastDir[eLASTDIR_UTX] = appFilePathName(*Files(0));					
					SavePkgName = appFileBaseName(*(Files(0)));					
					SavePkgName = SavePkgName.Left( SavePkgName.InStr(TEXT(".")) );
				}

				GWarn->BeginSlowTask( TEXT(""), 1, 0 );

				for( int x = 0 ; x < Files.Num() ; x++ )
				{
					GWarn->StatusUpdatef( x, Files.Num(), TEXT("Loading %ls"), *(Files(x)) );
					GEditor->Exec(*FString::Printf(TEXT("OBJ LOAD FILE=\"%ls\""), *(Files(x))));

					mrulist->AddItem( *(Files(x)) );
					mrulist->WriteINI();
					GConfig->Flush(0);
					if( GBrowserMaster->GetCurrent()==BrowserID )
						mrulist->AddToMenu( hWnd, GetMenu( IsDocked() ? OwnerWindow->hWnd : hWnd ) );
				}

				GWarn->EndSlowTask();

				GBrowserMaster->RefreshAll();
				pComboPackage->SetCurrent( pComboPackage->FindStringExact( *SavePkgName ) );
				RefreshGroups();
				RefreshTextureList();
			}

			GFileManager->SetDefaultDirectory(appBaseDir());
		}
		break;

		case ID_FileSave:
		{
			FString File;
			FString Package = pComboPackage->GetString(pComboPackage->GetCurrent());

			if (GetSaveNameWithDialog(
				*FString::Printf(TEXT("%ls.utx"), *Package),
				*GLastDir[eLASTDIR_UTX],
				TEXT("Texture Packages (*.utx)\0*.utx\0All Files\0*.*\0\0"),
				TEXT("utx"),
				TEXT("Save Texture Package"),
				File			
			) && AllowOverwrite(*File)) 
			{
				GEditor->Exec(*FString::Printf(
					TEXT("OBJ SAVEPACKAGE PACKAGE=\"%ls\" FILE=\"%ls\""),
					*Package, *File));

				mrulist->AddItem( *File );
				mrulist->WriteINI();
				GConfig->Flush(0);
				if( GBrowserMaster->GetCurrent()==BrowserID )
					mrulist->AddToMenu( hWnd, GetMenu( IsDocked() ? OwnerWindow->hWnd : hWnd ) );
				GLastDir[eLASTDIR_UTX] = appFilePathName(*File);
			}

			GFileManager->SetDefaultDirectory(appBaseDir());
		}
		break;

		case IDMN_MRU1:
		case IDMN_MRU2:
		case IDMN_MRU3:
		case IDMN_MRU4:
		case IDMN_MRU5:
		case IDMN_MRU6:
		case IDMN_MRU7:
		case IDMN_MRU8:
		{
			FString Filename = mrulist->Items[Command - IDMN_MRU1];
			GEditor->Exec( *(FString::Printf(TEXT("OBJ LOAD FILE=\"%ls\""), *Filename )) );

			FString Package = appFileBaseName(*Filename);
			Package = Package.Left( Package.InStr( TEXT(".")) );

			GBrowserMaster->RefreshAll();
			RefreshPackages();
			pComboPackage->SetCurrent( pComboPackage->FindStringExact( *Package ) );
			RefreshGroups();
			RefreshTextureList();
		}
		break;

		case IDMN_TB_IMPORT_IMAGE:
		{
			TArray<FString> Files;

			if (OpenFilesWithDialog(
				*(GLastDir[eLASTDIR_PCX]),
				TEXT("Image Files (*.pcx,*.bmp,*.dds,*.png)\0*.pcx;*.bmp;*.dds;*.png;\0 PCX Files\0*.pcx;\0 24/32bpp BMP Files\0*.bmp;\0 DDS Files\0*.dds;\0 24/32bpp PNG Files\0*.png;\0All Files\0*.*\0\0"),
				TEXT("*.pcx;*.bmp;*.dds;*.png;"),
				TEXT("Import Textures"),
				Files
			))
			{
				FString Package = pComboPackage->GetString( pComboPackage->GetCurrent() );
				FString Group = pComboGroup->GetString( pComboGroup->GetCurrent() );

				if( Files.Num() > 0 )
					GLastDir[eLASTDIR_PCX] = appFilePathName(*(Files(0)));

				WDlgImportTexture l_dlg( NULL, this );
				l_dlg.DoModal( Package, Group, &Files );

				// Flip to the texture/group that was used for importing
				GBrowserMaster->RefreshAll();
				INT Pos = pComboPackage->FindStringExact(*l_dlg.Package);
				if (Pos >= 0)
					pComboPackage->SetCurrent(Pos);
				RefreshGroups();
				if (Pos >= 0)
				{
					Pos = pComboGroup->FindStringExact(*l_dlg.Group);
					if (Pos >= 0)
						pComboGroup->SetCurrent(Pos);
				}
				RefreshTextureList();
			}

			GFileManager->SetDefaultDirectory(appBaseDir());
		}
		break;

		case IDMN_TB_EXPORT_IMAGE:
		case IDMN_TB_EXPORT_PCX:
		case IDMN_TB_EXPORT_BMP:
		case IDMN_TB_EXPORT_PNG:
		{
			if (!GEditor->CurrentTexture || !GEditor->CurrentTexture->IsValid())
			{
				appMsgf(TEXT("No valid Texture selected!"));
				break;
			}

			const TCHAR* Filter, *Ext;
			if (Command == IDMN_TB_EXPORT_PCX)
			{
				Filter = TEXT("PCX Files (*.pcx)\0*.pcx\0All Files\0*.*\0\0");
				Ext = TEXT("pcx");
			}
			else if (Command == IDMN_TB_EXPORT_BMP)
			{
				Filter = TEXT("BMP Files (*.bmp)\0*.bmp\0All Files\0*.*\0\0");
				Ext = TEXT("bmp");
			}
			else if (Command == IDMN_TB_EXPORT_PNG)
			{
				Filter = TEXT("PNG Files (*.png)\0*.png\0All Files\0*.*\0\0");
				Ext = TEXT("png");
			}
			else
			{
				Filter = TEXT("Image Files (*.bmp,*.pcx,*.png)\0*.bmp;*.pcx;*.png;\0 24/32bpp BMP Files\0*.bmp;\0 PCX Files\0*.pcx;\0 24/32bpp PNG Files\0*.png;\0All Files\0*.*\0\0");
				Ext = TEXT("*.bmp;*.pcx;*.png;");
			}

			FString File;
			if (GetSaveNameWithDialog(
				*FObjectName(GEditor->CurrentTexture),
				*GLastDir[eLASTDIR_PCX], 
				Filter, 
				Ext, 
				TEXT("Export Texture"), 
				File))
			{
				INT Exported = GEditor->Exec(*FString::Printf(
					TEXT("OBJ EXPORT TYPE=TEXTURE NAME=\"%ls\" FILE=\"%ls\""), 
					*FObjectName(GEditor->CurrentTexture), *File));
				appMsgf(TEXT("Successfully exported %d of 1 textures"), Exported);

				GLastDir[eLASTDIR_PCX] = appFilePathName(*File);
			}			
			GFileManager->SetDefaultDirectory(appBaseDir());
		}
		break;

		case IDMN_TB_EXPORT_BMP_ALL:
		case IDMN_TB_EXPORT_PCX_ALL:
		case IDMN_TB_EXPORT_PNG_ALL:
		{
			FString CurrentPackage = pComboPackage->GetString(pComboPackage->GetCurrent());

			FString Ext;
			if (Command == IDMN_TB_EXPORT_PCX_ALL)
				Ext = TEXT("pcx");
			else if (Command == IDMN_TB_EXPORT_BMP_ALL)
				Ext = TEXT("bmp");
			else
				Ext = TEXT("png");

			FString OutDir = FString::Printf(TEXT("..%ls%ls%lsTextures"), PATH_SEPARATOR, *CurrentPackage, PATH_SEPARATOR);
			GFileManager->MakeDirectory( *OutDir, 1 );
			GFileManager->SetDefaultDirectory(*OutDir);

			INT Exported = 0, Expected = 0;
			for( TObjectIterator<UTexture> It; It; ++It )
			{
				UTexture* A = *It;
				if( A->IsA(UTexture::StaticClass()))
				{
					FString TempPackageName = FObjectPathName(A).Left(CurrentPackage.Len());
					if (TempPackageName == CurrentPackage)
					{
						const TCHAR* GroupDir = *FObjectName(A->GetOuter());
						FString OutName, Name = FObjectName(A);
						if (appStrcmp(GroupDir, *TempPackageName) != 0)
						{
							GFileManager->MakeDirectory(GroupDir, 0);
							OutName = GroupDir;
							OutName += PATH_SEPARATOR;
						}
						OutName += *Name;
						OutName += '.';
						OutName += *Ext;
						debugf(TEXT("Exporting: %ls"), *OutName);						
						Exported += GEditor->Exec(*FString::Printf(TEXT("OBJ EXPORT TYPE=TEXTURE NAME=\"%ls\" FILE=\"%ls\""), *Name, *OutName));
						Expected++;
					}
				}
				else if (CurrentPackage == *FObjectName(A->GetOuter()))
					debugf(TEXT("Skipping %ls"), *FObjectFullName(A));
			}
			if (Expected > 0)
				appMsgf(TEXT("Successfully exported %d of %d textures"), Exported, Expected);
			GFileManager->SetDefaultDirectory(appBaseDir());
		}
		break;

		case IDMN_RD_SOFTWARE:
		case IDMN_RD_OPENGL:
		case IDMN_RD_D3D:
		case IDMN_RD_D3D9:
		case IDMN_RD_XOPENGL:
		case IDMN_RD_CUSTOM0:
		case IDMN_RD_CUSTOM1:
		case IDMN_RD_CUSTOM2:
		case IDMN_RD_CUSTOM3:
		case IDMN_RD_CUSTOM4:
		{
			SwitchRenderer(Command, TRUE);
			check(pViewport);
			FlushRepaintViewport();
			PositionChildControls();
			UpdateMenu();
			break;
		}

		default:
			WBrowser::OnCommand(Command);
			break;
		}
		unguard;
	}	
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WBrowserTexture::OnSize);
		WBrowser::OnSize(Flags, NewX, NewY);
		PositionChildControls();
		InvalidateRect( hWnd, NULL, FALSE );
		RefreshScrollBar();
		UpdateMenu();
		unguard;
	}
	virtual void RefreshAll()
	{
		guard(WBrowserTexture::RefreshAll);
		RefreshPackages();
		RefreshGroups();
		RefreshTextureList();
		if( GBrowserMaster->GetCurrent()==BrowserID )
			mrulist->AddToMenu( hWnd, GetMenu( IsDocked() ? OwnerWindow->hWnd : hWnd ) );
		GEditor->Flush(1);
		unguard;
	}
	void GetUsedTextures( TArray<UTexture*>& TList )
	{
		// Declare FArchive finder
		class FTexFinder : public FArchive
		{
			public:
			TArray<UTexture*>& Tex;
			FTexFinder( TArray<UTexture*>& TL )
			 : FArchive()
			 , Tex(TL)
			{}
			FArchive& operator<<( class UObject*& Res )
			{
				if( Res && Res->IsA(UTexture::StaticClass()) )
					Tex.AddUniqueItem((UTexture*)Res);
				return *this;
			}
		};

		FName MyLvl(TEXT("MyLevel"));
		FTexFinder TFinder(TList);
		for( FObjectIterator It(UObject::StaticClass()); It; ++It )
		{
			UObject* O = *It;
			if( GetParentOuter(O)->GetFName()==MyLvl )
				O->Serialize(TFinder);
		}
	}
	void RefreshPackages( void )
	{
		guard(WBrowserTexture::RefreshPackages);
		if (ModeTab != MODE_BY_PACKAGE)
			return;

		SetRedraw(false);

		int Current = pComboPackage->GetCurrent();
		Current = (Current != CB_ERR) ? Current : 0;
		FString Package = pComboPackage->GetString(pComboPackage->GetCurrent());

		// PACKAGES
		//
		pComboPackage->Empty();

		TArray<UObject*> PgkList;
		if( bListInUse )
		{
			TArray<UTexture*> UsedTex;
			GetUsedTextures(UsedTex);
			for( INT i=0; i<UsedTex.Num(); i++ )
				PgkList.AddUniqueItem(GetParentOuter(UsedTex(i)));
		}
		else
		{
			for( TObjectIterator<UTexture> It; It; ++It )
				PgkList.AddUniqueItem(GetParentOuter(*It));
		}

		for( int x = 0 ; x < PgkList.Num() ; x++ )
			pComboPackage->AddString(*FObjectName(PgkList(x)) );

		pComboPackage->SetCurrent( 0 );
		pComboPackage->SetCurrent( Current );
		if( Package.Len() )
		{
			Current = pComboPackage->FindString(*Package);
			if( Current!=INDEX_NONE )
				pComboPackage->SetCurrent( Current );
		}

		SetRedraw(true);

		unguard;
	}
	void RefreshGroups(void)
	{
		guard(WBrowserTexture::RefreshGroups);
		if (ModeTab != MODE_BY_PACKAGE)
			return;

		FString Package = pComboPackage->GetString(pComboPackage->GetCurrent());
		int Current = pComboGroup->GetCurrent();
		Current = (Current != CB_ERR) ? Current : 0;

		// GROUPS
		//
		pComboGroup->Empty();

		TArray<UObject*> GrpList;
		FName PckName(*Package);
		if (bListInUse)
		{
			TArray<UTexture*> UsedTex;
			GetUsedTextures(UsedTex);
			for (INT i = 0; i < UsedTex.Num(); i++)
			{
				if (!(GetParentOuter(UsedTex(i))->GetFName() == PckName))
					continue;
				if (HasValidGroup(UsedTex(i)))
					GrpList.AddUniqueItem(UsedTex(i)->GetOuter());
				else GrpList.AddUniqueItem(NULL);
			}
		}
		else
		{
			for (TObjectIterator<UTexture> It; It; ++It)
			{
				if (!(GetParentOuter(*It)->GetFName() == PckName))
					continue;
				if (HasValidGroup(*It))
					GrpList.AddUniqueItem(It->GetOuter());
				else GrpList.AddUniqueItem(NULL);
			}
		}

		for (int x = 0; x < GrpList.Num(); x++)
			pComboGroup->AddString(*FObjectName(GrpList(x)));

		pComboGroup->SetCurrent(0);
		pComboGroup->SetCurrent(Current);

		unguard;
	}
	void RefreshTextureList(INT PrevLastScroll = 0)
	{
		guard(WBrowserTexture::RefreshTextureList);
		if (PrevLastScroll)
		{
			FLOAT PercentScroll = (FLOAT)iScroll/PrevLastScroll;
			RefreshTextureListInternal();
			iScroll = 0.5f + PercentScroll*GLastScroll;
		}
		RefreshTextureListInternal();
		RefreshScrollBar();
		UpdateMenu();
		unguard;
	}
	void RefreshTextureListInternal( void )
	{
		guard(WBrowserTexture::RefreshTextureListInternal);
		FString Package = pComboPackage->GetString( pComboPackage->GetCurrent() );
		FString Group = pComboGroup->GetString( pComboGroup->GetCurrent() );
		FString NameFilter = pEditFilter->GetText();
		FString NameFilterCaps = NameFilter.Caps();

		BOOL bShowAllNow = ModeTab != MODE_BY_PACKAGE || pCheckGroupAll->IsChecked();

		ForcedTexList.Empty();
		FName PckName(*Package);
		FName GrpName(*Group);
		if (ModeTab == MODE_MRU)
			for (INT i = 0; i < MRU.Num(); i++)
			{
				FString TexName = FObjectName(MRU(i));
				if (appStrstr(*(TexName.Caps()), *NameFilterCaps))
					ForcedTexList.AddItem(MRU(i));
			}
		else
		{
			if ((ModeTab == MODE_BY_PACKAGE && bListInUse) || ModeTab == MODE_IN_USE)
			{
				TArray<UTexture*> TL;
				GetUsedTextures(TL);
				for (INT i = 0; i < TL.Num(); i++)
				{
					UTexture* T = TL(i);
					FString TexName = FObjectName(T);
					if (ModeTab == MODE_BY_PACKAGE && !(GetParentOuter(T)->GetFName() == PckName))
						continue;
					if (bShowAllNow || (GrpName == NAME_None && !HasValidGroup(T)) || GrpName == T->GetOuter()->GetFName())
						if (appStrstr(*(TexName.Caps()), *NameFilterCaps))
							ForcedTexList.AddItem(T);
				}
			}
			else
			{
				for (TObjectIterator<UTexture> It; It; ++It)
				{
					UTexture* T = *It;
					if (ModeTab == MODE_BY_PACKAGE && !(GetParentOuter(T)->GetFName() == PckName))
						continue;
					FString TexName = FObjectName(T);
					if (bShowAllNow || (GrpName == NAME_None && !HasValidGroup(T)) || GrpName == T->GetOuter()->GetFName())
						if (appStrstr(*(TexName.Caps()), *NameFilterCaps))
							ForcedTexList.AddItem(T);
				}
			}
			if (ForcedTexList.Num())
			{
				// Sort textures by name.
				appQsort(&ForcedTexList(0), ForcedTexList.Num(), sizeof(UTexture*), ResNameCompare2);
			}
		}
		
		GEditor->Exec(*FString::Printf( 
			TEXT("CAMERA UPDATE FLAGS=%d MISC1=%d MISC2=%d REN=%d NAME=TextureBrowser PACKAGE=\"%ls\" GROUP=\"%ls\" NAMEFILTER=\"%ls\""),
			SHOW_StandardView | SHOW_NoButtons | SHOW_ChildWindow | (pViewport && pViewport->Actor ? (pViewport->Actor->ShowFlags & SHOW_RealTime) : 0),
			iZoom,
			iScroll,
			REN_TexBrowser,
			*Package,
			bShowAllNow ? TEXT("(All)") : *Group,
			*NameFilter));
		unguard;
	}
	void RefreshScrollBar( void )
	{
		guard(WBrowserTexture::RefreshScrollBar);
		if( !pScrollBar ) return;

		RECT rect;
		::GetClientRect( hWnd, &rect );

		// Set the scroll bar to have a valid range.
		//
		SCROLLINFO si;
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_DISABLENOSCROLL | SIF_RANGE | SIF_POS | SIF_PAGE;
		si.nPage = rect.bottom;
		si.nMin = 0;
		si.nMax = GLastScroll ? GLastScroll + rect.bottom : 0;
		si.nPos = iScroll;
		int OldScroll = iScroll;
		iScroll = SetScrollInfo( pScrollBar->hWnd, SB_CTL, &si, TRUE );
		if (iScroll != OldScroll)
			RefreshTextureListInternal();

		EnableWindow( pScrollBar->hWnd, GLastScroll != 0 );
		unguard;
	}
	void OnModeTabSelChange()
	{
		guard(WBrowserTexture::OnModeTabSelChange);
		LastScroll[ModeTab] = iScroll;
		ModeTab = !ModeTabs ? MODE_BY_PACKAGE : ModeTabs->GetCurrent();
		iScroll = LastScroll[ModeTab];

		PositionChildControls();
		RefreshTextureList();
		unguard;
	}

	// Moves the child windows around so that they best match the window size.
	//
	void PositionChildControls( void )
	{
		guard(WBrowserTexture::PositionChildControls);
		if( !pComboPackage
			|| !pComboGroup
			|| !pCheckGroupAll
			|| !pInUseOnly
			|| !pLabelFilter
			|| !pEditFilter
			|| !pScrollBar
			|| !pViewport )
			return;

		FRect CR = GetClientRect();
		::MoveWindow(hWndToolBar, 4, 0, 9 * MulDiv(16, DPIX, 96), 9 * MulDiv(16, DPIY, 96), 1);
		RECT R;
		::GetClientRect( hWndToolBar, &R );

		float Fraction, Fraction20;
		Fraction = (CR.Width() - 8) / 10.0f;
		Fraction20 = (CR.Width() - 8) / 20.0f;
		float Top = R.bottom + MulDiv(4, DPIY, 96);

		if (ModeTabs)
		{
			SetWindowPos(ModeTabs->hWnd, HWND_BOTTOM,  0, Top, CR.Width(), CR.Height() - Top, SWP_NOACTIVATE | SWP_FRAMECHANGED);

			Top += MulDiv(26, DPIY, 96);
		}

		float OldTop = Top;
		if (ModeTab != MODE_BY_PACKAGE)
			Top = -100000; // hack for prevent draw this items

		::MoveWindow( pComboPackage->hWnd, 4, Top, Fraction * 10, 20, 1 );

		Top += MulDiv(24, DPIY, 96);
		::MoveWindow( pCheckGroupAll->hWnd, 5, Top, MulDiv(75, DPIX, 96),	MulDiv(20, DPIY, 96), 1 );
		::MoveWindow( pInUseOnly->hWnd, 10 + MulDiv(75, DPIX, 96) , Top, MulDiv(75, DPIX, 96), MulDiv(20, DPIY, 96), 1);
		::MoveWindow( pComboGroup->hWnd, 15 + MulDiv(150, DPIX, 96), Top, CR.Max.X - 20 - MulDiv(150, DPIX, 96), 20, 1 );
		
		Top += MulDiv(24, DPIY, 96);

		if (ModeTab != MODE_BY_PACKAGE)
			Top = OldTop;

		float Height = CR.Height() - Top - MulDiv(28, DPIY, 96);
		::MoveWindow( (HWND)pViewport->GetWindow(), 4, Top, (CR.Width() - MulDiv(20, DPIX, 96)) & 0xFFFFFFFE, Height, 1 );
		pViewport->Repaint(1);
		::MoveWindow( pScrollBar->hWnd, CR.Width() - MulDiv(16, DPIX, 96), Top, MulDiv(16, DPIX, 96), Height, 1 );

		Top += Height + MulDiv(4, DPIY, 96);
		::MoveWindow( pLabelFilter->hWnd, 4, Top + 2, MulDiv(36, DPIX, 96), MulDiv(20, DPIY, 96), 1 );
		::MoveWindow( pEditFilter->hWnd, 4 + MulDiv(36, DPIX, 96), Top, CR.Width() - MulDiv(36, DPIX, 96) - 8, MulDiv(20, DPIY, 96), 1 );
		unguard;
	}
	virtual void SetCaption(FString* Tail = NULL)
	{
		guard(WBrowserTexture::SetCaption);

		FString Extra;
		UTexture* Texture = GEditor->CurrentTexture;
		if (Texture)
		{
			Extra = FString::Printf(TEXT("%ls (%dx%d)"),
				*FObjectName(Texture), Texture->USize, Texture->VSize);
			AddMRU(Texture);
		}

		WBrowser::SetCaption(&Extra);
		unguard;
	}

	// Notification delegates for child controls.
	//
	void OnComboPackageSelChange()
	{
		guard(WBrowserTexture::OnComboPackageSelChange);
		iScroll = 0;
		RefreshGroups();
		RefreshTextureList();
		unguard;
	}
	void OnComboGroupSelChange()
	{
		guard(WBrowserTexture::OnComboGroupSelChange);
		iScroll = 0;
		RefreshTextureList();
		unguard;
	}
	void OnGroupAllClick()
	{
		guard(WBrowserTexture::OnGroupAllClick);
		EnableWindow( pComboGroup->hWnd, !pCheckGroupAll->IsChecked() );
		iScroll = 0;
		RefreshTextureList();
		unguard;
	}
	void OnInUseOnlyClick()
	{
		guard(WBrowserTexture::OnInUseOnlyClick);
		bListInUse = !bListInUse;
		pInUseOnly->SetCheck(bListInUse);
		RefreshAll();
		unguard;
	}
	void OnEditFilterChange()
	{
		guard(WBrowserTexture::OnEditFilterChange);
		RefreshTextureList();
		unguard;
	}
	virtual void OnVScroll( WPARAM wParam, LPARAM lParam )
	{
		if( (HWND)lParam == pScrollBar->hWnd || lParam == WM_MOUSEWHEEL)
		{

			switch(LOWORD(wParam))
			{

			case SB_LINEUP:
				iScroll -= 64;
				iScroll = Max( iScroll, 0 );
				RefreshTextureList();
				break;

			case SB_LINEDOWN:
				iScroll += 64;
				iScroll = Min( iScroll, GLastScroll );
				RefreshTextureList();
				break;

			case SB_PAGEUP:
				iScroll -= 256;
				iScroll = Max( iScroll, 0 );
				RefreshTextureList();
				break;

			case SB_PAGEDOWN:
				iScroll += 256;
				iScroll = Min( iScroll, GLastScroll );
				RefreshTextureList();
				break;

			case SB_THUMBTRACK:
				// stijn: The wParam for SB_THUMBTRACK only carries 16 bits of scroll position data.
				// GetScrollInfo allows us to retrieve 32 bits, which is necessary if we're scrolling down a really long window
				SCROLLINFO si;
				ZeroMemory(&si, sizeof(si));
				si.cbSize = sizeof(si);
				si.fMask = SIF_TRACKPOS;
				if (GetScrollInfo(pScrollBar->hWnd, SB_CTL, &si))
					iScroll = si.nTrackPos;
				else				
					iScroll = HIWORD(wParam);
				RefreshTextureList();
				break;
			}
		}
	}
	BOOL RemoveMRU(UTexture* InTexture)
	{
		guard(WBrowserTexture::RemoveMRU);
		INT idx;
		if (!MRU.FindItem(InTexture, idx))
			return FALSE;
		MRU.Remove(idx);
		return TRUE;
		unguard;
	}
	void AddMRU(UTexture* InTexture)
	{
		guard(WBrowserTexture::AddMRU);
		if (MRU.Num() > 0 && MRU(0) == InTexture)
			return;
		INT idx;
		if (ModeTab == MODE_MRU && GetFocus() == pViewport->GetWindow() && MRU.FindItem(InTexture, idx))
			return;
		RemoveMRU(InTexture);

		MRU.Insert(0);
		MRU(0) = InTexture;

		while (MRU.Num() > MRU_MAX_TEXTURES)
			MRU.Pop();

		if (ModeTab == MODE_MRU)
			RefreshTextureList();

		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
