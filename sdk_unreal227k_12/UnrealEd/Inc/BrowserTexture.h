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
extern void ParseStringToArray( const TCHAR* pchDelim, FString String, TArray<FString>* _pArray);
extern FString GLastDir[eLASTDIR_MAX];

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
	WComboBox PackageEdit;
	WComboBox GroupEdit;
	WEdit NameEdit;
	WComboBox ClassCombo;
	WComboBox WidthCombo;
	WComboBox HeightCombo;

	FString defPackage, defGroup;
	TArray<FString>* paFilenames;
	INT OldTexLimit;

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
		, OldTexLimit(-2)
	{
		PackageEdit.SelectionChangeDelegate = FDelegate(this, (TDelegate)&WDlgNewTexture::OnTexturePackageChange);
		ClassCombo.SelectionChangeDelegate = FDelegate(this, (TDelegate)&WDlgNewTexture::OnTextureClassChange);
	}

	void SetMaxTexRes(INT NewLimit)
	{
		guard(WDlgNewTexture::SetMaxTexRes);
		if (OldTexLimit == NewLimit)
			return;
		OldTexLimit = NewLimit;

		if (NewLimit == -1)
		{
			WidthCombo.SetEnable(FALSE);
			HeightCombo.SetEnable(FALSE);
			return;
		}
		WidthCombo.SetEnable(TRUE);
		HeightCombo.SetEnable(TRUE);
		if (NewLimit == 0)
			NewLimit = 1024;
		FString PrevWidth = WidthCombo.GetString(WidthCombo.GetCurrent());
		FString PrevHeight = HeightCombo.GetString(HeightCombo.GetCurrent());
		WidthCombo.Empty();
		HeightCombo.Empty();
		TCHAR TempStr[8];
		for (INT i = 2; i <= NewLimit; i *= 2)
		{
			appSnprintf(TempStr, 8, TEXT("%i"), i);
			WidthCombo.AddString(TempStr);
			HeightCombo.AddString(TempStr);
		}

		INT iPrev = WidthCombo.FindStringExact(*PrevWidth);
		if (iPrev == INDEX_NONE)
			iPrev = WidthCombo.GetCount() - 1;
		WidthCombo.SetCurrent(iPrev);

		iPrev = HeightCombo.FindStringExact(*PrevHeight);
		if (iPrev == INDEX_NONE)
			iPrev = HeightCombo.GetCount() - 1;
		HeightCombo.SetCurrent(iPrev);
		unguard;
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgNewTexture::OnInitDialog);
		WDialog::OnInitDialog();

		SetRedraw(false);

		PackageEdit.Empty();
		{
			TSingleMap<UObject*> Listed;
			Listed.Set(UObject::GetTransientPackage());
			for (TObjectIterator<UTexture> It; It; ++It)
			{
				if (!(It->GetFlags() & RF_Public))
					continue;
				UObject* Top = It->TopOuter();
				if (Listed.Set(Top))
					PackageEdit.AddString(Top->GetName());
			}
		}
		INT index = PackageEdit.FindStringExact(*defPackage);
		if (index >= 0)
			PackageEdit.SetCurrent(index);
		else PackageEdit.SetText( *defPackage );
		OnTexturePackageChange();
		GroupEdit.SetText( *defGroup );
		::SetFocus( NameEdit.hWnd );

		for (TObjectIterator<UClass> It; It; ++It)
		{
			if ((It->IsChildOf(UTexture::StaticClass()) || It->IsChildOf(UFont::StaticClass())) && !(It->ClassFlags & (CLASS_Abstract | CLASS_NoUserCreate | CLASS_Transient)))
			{
				ClassCombo.AddString(It->GetName());
			}
		}
		ClassCombo.SetCurrent(0);

		PackageEdit.SetText( *defPackage );
		GroupEdit.SetText( *defGroup );
		OnTextureClassChange();

		SetRedraw(true);

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
			TCHAR* CmdStr = appStaticString1024();
			appSnprintf(CmdStr, 1024, TEXT("TEXTURE NEW NAME=\"%ls\" CLASS=\"%ls\" GROUP=\"%ls\" PACKAGE=\"%ls\""), *NameEdit.GetText(), *ClassCombo.GetString(ClassCombo.GetCurrent()), *GroupEdit.GetText(), *PackageEdit.GetText());
			if (OldTexLimit >= 0)
			{
				TCHAR ResStr[128];
				appSnprintf(ResStr, 128, TEXT(" USIZE=%ls VSIZE=%ls"), *WidthCombo.GetString(WidthCombo.GetCurrent()), *HeightCombo.GetString(HeightCombo.GetCurrent()));
				appStrncat(CmdStr, ResStr, 1024);
			}
			else appStrncat(CmdStr, TEXT(" USIZE=4 VSIZE=4"), 1024);
			GEditor->Exec(CmdStr);
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

		if( !Package.Len() || !Name.Len() )
		{
			appMsgf( TEXT("Invalid input.") );
			return FALSE;
		}
		return TRUE;
		unguard;
	}
	void OnClose()
	{
		guard(WDlgNewTexture::OnClose);
		Show(0);
		unguard;
	}
	void OnTextureClassChange()
	{
		guard(WDlgNewTexture::OnTextureClassChange);
		FString TexClassName = ClassCombo.GetString(ClassCombo.GetCurrent());
		UClass* TexClass = FindObject<UClass>(ANY_PACKAGE, *TexClassName);
		if (TexClass && TexClass->IsChildOf(UTexture::StaticClass()))
			SetMaxTexRes(reinterpret_cast<UTexture*>(TexClass->GetDefaultObject())->MaxInitResolution);
		unguard;
	}
	void OnTexturePackageChange()
	{
		guard(WDlgNewTexture::OnTexturePackageChange);
		FString OldGroup = GroupEdit.GetText();
		FString PackageName = PackageEdit.GetString(PackageEdit.GetCurrent());
		UPackage* SelPackage = FindObject<UPackage>(NULL, *PackageName, TRUE);
		GroupEdit.Empty();
		if (SelPackage)
		{
			TSingleMap<UObject*> Listed;
			Listed.Set(UObject::GetTransientPackage());
			for (TObjectIterator<UTexture> It; It; ++It)
			{
				if (!(It->GetFlags() & RF_Public) || !It->IsIn(SelPackage))
					continue;
				UObject* NextOuter = It->GetOuter();
				while (NextOuter->GetOuter() && NextOuter->GetOuter() != SelPackage)
					NextOuter = NextOuter->GetOuter();
				if (NextOuter != SelPackage && NextOuter->GetClass() == UPackage::StaticClass() && Listed.Set(NextOuter))
					GroupEdit.AddString(NextOuter->GetName());
			}
		}
		GroupEdit.AddString(*FName(NAME_None));
		INT iCur = GroupEdit.FindStringExact(*OldGroup);
		if (iCur >= 0)
			GroupEdit.SetCurrent(iCur);
		else GroupEdit.SetText(*OldGroup);
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
	WEdit PackageEdit;
	WEdit GroupEdit;
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
	INT iCurrentFilename{};
	INT Compression = 3;

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
	,	CheckAlphaBlend	( this, IDCK_ALPHABLEND)
	,	CheckMipMap		( this, IDCK_MIPMAP )
	,	CompressionFormat		( this, IDCB_COMPRESS)
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgImportTexture::OnInitDialog);
		WDialog::OnInitDialog();

		SetRedraw(false);

		PackageEdit.SetText( *defPackage );
		GroupEdit.SetText( *defGroup );
		::SetFocus( NameEdit.hWnd );

		bOKToAll = FALSE;
		iCurrentFilename = -1;
		SetNextFilename();
		CheckMipMap.SetCheck( BST_CHECKED );
		CompressionFormat.AddString(TEXT("None"));
		CompressionFormat.AddString(TEXT("Monochrome"));
		CompressionFormat.AddString(TEXT("P8 Palette (256 colors)"));
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
		CompressionFormat.SetCurrent( 5 );
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
	void OnClose()
	{
		guard(WDlgImportTexture::OnClose);
		Show(0);
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
	void OnCompressionLevelChange( void )
	{
		guard(WDlgImportTexture::OnCompressionLevelChange);
		switch( CompressionFormat.GetCurrent() )
			{
			case 0:
				Compression = 0; //None
				break;
			case 1:
				Compression = -1; //Mono
				break;
			case 2:
				Compression = -2; //P8
				break;
			case 3:
				Compression = 1; //BC...
				break;
			case 4:
				Compression = 2;
				break;
			case 5:
				Compression = 3;
				break;
			case 6:
				Compression = 4;
				break;
			case 7:
				Compression = 5;
				break;
			case 8:
				Compression = 7;
				break;
			case 9:
				Compression = 7;
				break;

			default:
				Compression = 3;
			}
		unguard;
	}
	void RefreshName( void )
	{
		guard(WDlgImportTexture::RefreshName);
		const FString& FullName = (*paFilenames)(iCurrentFilename);
		FilenameStatic.SetText(*FullName);

		FString Name = FullName.GetFilenameOnly();
		NameEdit.SetText( *Name );

		const TCHAR* Ext = FullName.GetFileExtension();
		CompressionFormat.SetEnable(appStricmp(Ext, TEXT("dds")) != 0);
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
		TCHAR l_chCmd[512] = TEXT("");
		appSprintf(l_chCmd, TEXT("TEXTURE IMPORT FILE=\"%ls\" NAME=\"%ls\" PACKAGE=\"%ls\""),
			*(*paFilenames)(iCurrentFilename), *Name, *Package);
		if (Group.Len() && Group!=TEXT("None"))
		{
			appStrcat(l_chCmd, TEXT(" GROUP=\""));
			appStrcat(l_chCmd, *Group);
			appStrcat(l_chCmd, TEXT("\""));
		}
		DWORD ImportFlags = 0;
		if (CheckAlphaBlend.IsChecked())
		{
			ImportFlags = PF_AlphaBlend;
			if (CheckMasked.IsChecked())
				GWarn->Log(TEXT("Masked flag is ignored while AlphaBlend flag is checked."));
		}
		else if (CheckMasked.IsChecked())
			ImportFlags = PF_Masked;

		appSprintf(&l_chCmd[appStrlen(l_chCmd)], TEXT(" MIPS=%d FLAGS=%d"),
			CheckMipMap.IsChecked(), ImportFlags);

		appSprintf(&l_chCmd[appStrlen(l_chCmd)], TEXT(" BTC=%i"), Compression);

		GEditor->Exec( l_chCmd );
		GEditor->Flush(1);
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
	{ 0, IDMN_MB_DOCK, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 1, IDMN_TB_FileOpen, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 2, IDMN_TB_FileSave, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 3, IDMN_TB_IN_LOAD_ENTIRE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 4, IDMN_TB_PROPERTIES, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 5, IDMN_TB_PREV_GRP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 6, IDMN_TB_NEXT_GRP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 7, IDMN_TB_IN_USE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 8, IDMN_TB_REALTIME, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
};
struct {
	TCHAR ToolTip[64];
	int ID;
} ToolTips_BT[] = {
	TEXT("Toggle Dock Status"), IDMN_MB_DOCK,
	TEXT("Open Package"), IDMN_TB_FileOpen,
	TEXT("Save Package"), IDMN_TB_FileSave,
	TEXT("Load Entire Package"), IDMN_TB_IN_LOAD_ENTIRE,
	TEXT("Texture Properties"), IDMN_TB_PROPERTIES,
	TEXT("Previous Group"), IDMN_TB_PREV_GRP,
	TEXT("Next Group"), IDMN_TB_NEXT_GRP,
	TEXT("Show All Textures in use"), IDMN_TB_IN_USE,
	TEXT("Show Real-time preview"), IDMN_TB_REALTIME,
	NULL, 0
};

int CDECL ResNameCompare(const void *A, const void *B)
{
	return appStricmp((*(UObject **)A)->GetName(),(*(UObject **)B)->GetName());
}
class WBrowserTexture : public WBrowser, public WViewportWindowContainer
{
	DECLARE_WINDOWCLASS(WBrowserTexture,WBrowser,Window)

	TArray<WDlgTexProp> PropWindows;

	WComboBox *pComboPackage, *pComboGroup;
	WLabel *pLabelFilter;
	WEdit *pEditFilter;
	WVScrollBar* pScrollBar;
	HWND hWndToolBar{};
	WToolTip *ToolTipCtrl{};
	MRUList* mrulist;
	HBITMAP ToolbarImage{};
	FName NAME_FontGroupName;

	int iZoom, iScroll;

	UBOOL bListInUse, bHasAllGroups;

	inline bool HasValidGroup( UObject* O ) const
	{
		if( !O->GetOuter() || !O->GetOuter()->GetOuter() )
			return false;
		return true;
	}
	inline UObject* GetGroupObject(UObject* O) const
	{
		while (O->GetOuter() && O->GetOuter()->GetOuter())
			O = O->GetOuter();
		return O;
	}

	// Structors.
	WBrowserTexture( FName InPersistentName, WWindow* InOwnerWindow, HWND InEditorFrame )
	:	WBrowser( InPersistentName, InOwnerWindow, InEditorFrame )
	,	WViewportWindowContainer(TEXT("TextureBrowser"), *InPersistentName)
	{
		pComboPackage = pComboGroup = NULL;
		pLabelFilter = NULL;
		pEditFilter = NULL;
		pScrollBar = NULL;
		ToolTipCtrl = NULL;
		iZoom = 128;
		iScroll = 0;
		MenuID = IDMENU_BrowserTexture;
		BrowserID = eBROWSER_TEXTURE;
		Description = TEXT("Textures");
		NAME_FontGroupName = FName(TEXT("Fonts"), FNAME_Intrinsic);
		mrulist = NULL;
		bListInUse = FALSE;
		bHasAllGroups = FALSE;
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

		SetMenu( hWnd, LoadMenuW(hInstance, MAKEINTRESOURCEW(IDMENU_BrowserTexture)) );

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
		GConfig->GetString(*ConfigName, TEXT("Device"), Device, GUEdIni);
		CreateViewport(SHOW_StandardView | SHOW_NoButtons | SHOW_ChildWindow, REN_TexBrowser, 320, 200, -1, -1, *Device);
		check(pViewport && pViewport->Actor);
		if (!GConfig->GetInt(*PersistentName, TEXT("Zoom"), iZoom, GUEdIni))
			iZoom = 128;
		pViewport->Actor->Misc1 = iZoom;
		pViewport->Actor->Misc2 = iScroll;

		// Force a repaint as we only set the zoom and scroll after creating the viewport
		pViewport->Repaint(1);

		ToolbarImage = reinterpret_cast<HBITMAP>(LoadImage(hInstance,
			MAKEINTRESOURCE(IDB_BrowserTexture_TOOLBAR),
			IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS));
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

		UpdateIni();

		delete pComboPackage;
		delete pComboGroup;
		delete pLabelFilter;
		delete pEditFilter;
		delete pScrollBar;

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
	void UpdateIni()
	{
		GConfig->SetInt(*PersistentName, TEXT("Zoom"), iZoom, GUEdIni);
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
		CheckMenuItem( menu, IDMN_VAR_200, MF_BYCOMMAND | ((iZoom == 1200) ? MF_CHECKED : MF_UNCHECKED) );
		CheckMenuItem( menu, IDMN_VAR_100, MF_BYCOMMAND | ((iZoom == 1100) ? MF_CHECKED : MF_UNCHECKED) );
		CheckMenuItem( menu, IDMN_VAR_50, MF_BYCOMMAND | ((iZoom == 1050) ? MF_CHECKED : MF_UNCHECKED) );
		CheckMenuItem( menu, IDMN_VAR_25, MF_BYCOMMAND | ((iZoom == 1025) ? MF_CHECKED : MF_UNCHECKED) );
		CheckMenuItem( menu, IDMN_MB_DOCK, MF_BYCOMMAND | (IsDocked() ? MF_CHECKED : MF_UNCHECKED) );
		CheckMenuItem( menu, IDMN_TB_REALTIME, MF_BYCOMMAND | bRealTime ? MF_CHECKED : MF_UNCHECKED);
		UpdateRendererMenu(menu, TRUE);
		
		SendMessage(hWndToolBar, TB_SETSTATE , IDMN_TB_IN_USE, bListInUse ? TBSTATE_CHECKED | TBSTATE_ENABLED : TBSTATE_ENABLED);
		SendMessage(hWndToolBar, TB_SETSTATE , IDMN_TB_REALTIME, bRealTime ? TBSTATE_CHECKED | TBSTATE_ENABLED : TBSTATE_ENABLED);

		if( mrulist && GBrowserMaster->GetCurrent()==BrowserID )
			mrulist->AddToMenu( hWnd, GetMenu( IsDocked() ? OwnerWindow->hWnd : hWnd ) );
		unguard;
	}
	virtual bool ShouldBlockAccelerators()
	{
		return pEditFilter && pEditFilter->hWnd && GetFocus() == pEditFilter->hWnd;
	}
	inline FString GetCurrentGroupName() const
	{
		return (bHasAllGroups && pComboGroup->GetCurrent()==0) ? FString(TEXT("None")) : pComboGroup->GetString(pComboGroup->GetCurrent());
	}
	void OnCommand( INT Command )
	{
		guard(WBrowserTexture::OnCommand);
		switch( Command )
		{
			case IDMN_TB_NEW:
				{
					FString Package = pComboPackage->GetCurrent()>0 ? pComboPackage->GetString( pComboPackage->GetCurrent() ) : FString();
					FString Group = GetCurrentGroupName();

					WDlgNewTexture l_dlg( NULL, this );
					if( l_dlg.DoModal( Package, Group ) )
					{
						UObject* SelObj = GEditor->CurrentFont ? reinterpret_cast<UObject*>(GEditor->CurrentFont) : reinterpret_cast<UObject*>(GEditor->CurrentTexture);

						if (SelObj)
						{
							RefreshPackages();
							pComboPackage->SetCurrent(pComboPackage->FindStringExact(*l_dlg.Package));
							RefreshGroups();
							pComboGroup->SetCurrent(SelObj->IsA(UFont::StaticClass()) ? pComboGroup->FindStringExact(*NAME_FontGroupName) : pComboGroup->FindStringExact(*l_dlg.Group));
							RefreshTextureList();
							SetCaption();

							// Call up the properties on this new texture.
							WDlgTexProp* pDlgTexProp = new(PropWindows)WDlgTexProp(NULL, OwnerWindow, SelObj);
							pDlgTexProp->DoModeless();
							pDlgTexProp->pProps->SetNotifyHook(GEditor);
						}
					}
				}
				break;

			case IDMN_TB_PROPERTIES:
				{
					UObject* SelObj = GEditor->CurrentFont ? reinterpret_cast<UObject*>(GEditor->CurrentFont) : reinterpret_cast<UObject*>(GEditor->CurrentTexture);
					if( SelObj )
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

						WDlgTexProp* pDlgTexProp = new(PropWindows)WDlgTexProp(NULL, OwnerWindow, SelObj);
						pDlgTexProp->DoModeless();
						pDlgTexProp->pProps->SetNotifyHook( GEditor );
						RefreshAll();
					}
				}
				break;

			case IDMN_TB_SETCOMPRESS_BC1:
				{
					if (GEditor->CurrentTexture)
						GEditor->CurrentTexture->Compress(TEXF_BC1, (GEditor->CurrentTexture->Mips.Num() > 1));
				}
				break;

			case IDMN_TB_SETCOMPRESS_BC2:
				{
					if (GEditor->CurrentTexture)
						GEditor->CurrentTexture->Compress(TEXF_BC2, (GEditor->CurrentTexture->Mips.Num() > 1));
				}
				break;

			case IDMN_TB_SETCOMPRESS_BC3:
				{
					if (GEditor->CurrentTexture)
						GEditor->CurrentTexture->Compress(TEXF_BC3, (GEditor->CurrentTexture->Mips.Num()>1));
				}
				break;

			case IDMN_TB_SETCOMPRESS_BC4:
				{
					if (GEditor->CurrentTexture)
						GEditor->CurrentTexture->Compress(TEXF_BC4, (GEditor->CurrentTexture->Mips.Num() > 1));
				}
				break;

			case IDMN_TB_SETCOMPRESS_BC5:
				{
					if (GEditor->CurrentTexture)
						GEditor->CurrentTexture->Compress(TEXF_BC5, (GEditor->CurrentTexture->Mips.Num() > 1));
				}
				break;

			case IDMN_TB_SETCOMPRESS_BC7:
				{
					if (GEditor->CurrentTexture)
						GEditor->CurrentTexture->Compress(TEXF_BC7, (GEditor->CurrentTexture->Mips.Num() > 1));
				}
				break;

			case IDMN_TB_SETCOMPRESS_P8:
				{
					if (GEditor->CurrentTexture)
						GEditor->CurrentTexture->Compress(TEXF_P8, (GEditor->CurrentTexture->Mips.Num()>1));
				}
				break;

			case IDMN_TB_SETCOMPRESS_MONO:
				{
					if (GEditor->CurrentTexture)
						GEditor->CurrentTexture->Compress(TEXF_MONO, (GEditor->CurrentTexture->Mips.Num() > 1));
				}
				break;

			case IDMN_TB_SETCOMPRESS_DECOM: // FIXME - Crashes render driver
				{
					if (GEditor->CurrentTexture)
						GEditor->CurrentTexture->Compress(TEXF_RGBA8_, (GEditor->CurrentTexture->Mips.Num() > 1));
					/*{
						if (bCompressedFormat(GEditor->CurrentTexture->Format))
							GEditor->CurrentTexture->Decompress(TEXF_RGBA8_, 1);
						else GEditor->CurrentTexture->ConvertFormat(TEXF_RGBA8_, 1);
					}*/
				}
				break;

			case IDMN_TB_SETCOMPRESS_DECOMBGRA: 
				{
					if (GEditor->CurrentTexture)
						GEditor->CurrentTexture->Compress(TEXF_BGRA8, (GEditor->CurrentTexture->Mips.Num() > 1));
					/*{
						if (bCompressedFormat(GEditor->CurrentTexture->Format))
							GEditor->CurrentTexture->Decompress(TEXF_BGRA8, 1);
						else GEditor->CurrentTexture->ConvertFormat(TEXF_BGRA8, 1);
					}*/
				}
				break;

			case IDMN_TB_SETMIPS_ADD:
				{
					if (GEditor->CurrentTexture)
						GEditor->CurrentTexture->CreateMips(1, 1);
				}
				break;

			case IDMN_TB_SETMIPS_DEL:
				{
					if (GEditor->CurrentTexture)
						GEditor->CurrentTexture->CreateMips(0, 0);
				}
				break;

			case IDMN_TB_DELETE:
				{
					UObject* DelObj = GEditor->CurrentFont ? reinterpret_cast<UObject*>(GEditor->CurrentFont) : reinterpret_cast<UObject*>(GEditor->CurrentTexture);
					if( !DelObj)
						break;

					FStringOutputDevice GetResult;
					TCHAR l_chCmd[256];

					appSprintf(l_chCmd, TEXT("DELETE CLASS=%ls OBJECT=\"%ls\""), DelObj->GetClass()->GetName(), DelObj->GetPathName());
					GEditor->Get(TEXT("Obj"), l_chCmd, GetResult);

					if (!GetResult.Len())
					{
						GEditor->CurrentTexture = NULL;
						GEditor->CurrentFont = NULL;
						RefreshPackages();
						RefreshGroups();
						RefreshTextureList();
					}
					else
					{
						GEditor->CurrentTexture = Cast<UTexture>(DelObj);
						GEditor->CurrentFont = Cast<UFont>(DelObj);
						appMsgf(TEXT("Can't delete %ls:\n%ls"), DelObj->GetFullName(), *GetResult);
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
					UObject* RenObj = GEditor->CurrentFont ? reinterpret_cast<UObject*>(GEditor->CurrentFont) : reinterpret_cast<UObject*>(GEditor->CurrentTexture);
					if( !RenObj ) break;

					WDlgRename dlg(RenObj, this);
					if (dlg.DoModal())
					{
						RefreshPackages();
						RefreshGroups();
						RefreshTextureList();
					}
				}
				break;
			case IDMN_TB_REMOVE:
				{
					if( !GEditor->CurrentTexture ) break;
					EInputKey Key=IK_Q;
					GEditor->Key(pViewport,Key, 0);
				}
				break;
			case IDMN_TB_IN_LOAD_ENTIRE:
				if (pComboPackage->GetCurrent() > 0)
				{
					FString Package = pComboPackage->GetString(pComboPackage->GetCurrent());
					UObject::LoadPackage(NULL, *Package, LOAD_NoWarn);
					RefreshTextureList();
					RefreshGroups();
				}
				break;

			case IDMN_TB_ZOOM_32:
				{
					iZoom = 32;
					iScroll = 0;
					RefreshTextureList();
					UpdateIni();
				}
				break;

			case IDMN_TB_ZOOM_64:
				{
				iZoom = 64;
				iScroll = 0;
				RefreshTextureList();
				UpdateIni();
				}
				break;

			case IDMN_TB_ZOOM_128:
				{
				iZoom = 128;
				iScroll = 0;
				RefreshTextureList();
				UpdateIni();
				}
				break;

			case IDMN_TB_ZOOM_256:
				{
				iZoom = 256;
				iScroll = 0;
				RefreshTextureList();
				UpdateIni();
				}
				break;

			case IDMN_TB_ZOOM_512:
				{
					iZoom = 512;
					iScroll = 0;
					RefreshTextureList();
					UpdateIni();
				}
				break;

			case IDMN_TB_ZOOM_1024:
				{
					iZoom = 1024;
					iScroll = 0;
					RefreshTextureList();
					UpdateIni();
				}
				break;

			case IDMN_VAR_200:
				{
					iZoom = 1200;
					iScroll = 0;
					RefreshTextureList();
					UpdateIni();
				}
				break;

			case IDMN_VAR_100:
				{
					iZoom = 1100;
					iScroll = 0;
					RefreshTextureList();
					UpdateIni();
				}
				break;

			case IDMN_VAR_50:
				{
					iZoom = 1050;
					iScroll = 0;
					RefreshTextureList();
					UpdateIni();
				}
				break;

			case IDMN_VAR_25:
				{
					iZoom = 1025;
					iScroll = 0;
					RefreshTextureList();
					UpdateIni();
				}
				break;

			case IDMN_TB_FileOpen:
				{
					OPENFILENAMEA ofn;
					char File[8192] = "\0";

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(char) * 8192;
					ofn.lpstrFilter = "Texture Packages (*.utx)\0*.utx\0All Files\0*.*\0\0";
					ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_UTX]) );
					ofn.lpstrDefExt = "utx";
					ofn.lpstrTitle = "Open Texture Package";
					ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

					if( GetOpenFileNameA(&ofn) )
					{
						int iNULLs = FormatFilenames( File );

						TArray<FString> StringArray;
						ParseStringToArray( TEXT("|"), appFromAnsi( File ), &StringArray );

						int iStart = 0;
						FString Prefix = TEXT("\0");

						if( iNULLs )
						{
							iStart = 1;
							Prefix = *(StringArray(0));
							Prefix += TEXT("\\");
						}

						if( StringArray.Num() > 0 )
						{
							if (StringArray.Num() == 1)
								SavePkgName = StringArray(0).GetFilenameOnly();
							else
								SavePkgName = StringArray(1).GetFilenameOnly();
						}

						if( StringArray.Num() == 1 )
							GLastDir[eLASTDIR_UTX] = StringArray(0).GetFilePath().LeftChop(1);
						else
							GLastDir[eLASTDIR_UTX] = StringArray(0);

						GWarn->BeginSlowTask( TEXT(""), 1, 0 );

						for( int x = iStart ; x < StringArray.Num() ; x++ )
						{
							GWarn->StatusUpdatef( x, StringArray.Num(), TEXT("Loading %ls"), *(StringArray(x)) );

							TCHAR l_chCmd[512];
							appSprintf( l_chCmd, TEXT("OBJ LOAD FILE=\"%ls%ls\""), *Prefix, *(StringArray(x)) );
							GEditor->Exec( l_chCmd );

							mrulist->AddItem( *(StringArray(x)) );
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

			case IDMN_TB_FileSave:
				if (pComboPackage->GetCurrent() > 0)
				{
					OPENFILENAMEA ofn;
					char File[8192] = "\0";
					FString Package = pComboPackage->GetString( pComboPackage->GetCurrent() );

					::sprintf_s( File, ARRAY_COUNT(File), "%ls.utx", *Package );

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(char) * 8192;
					ofn.lpstrFilter = "Texture Packages (*.utx)\0*.utx\0All Files\0*.*\0\0";
					ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_UTX]) );
					ofn.lpstrDefExt = "utx";
					ofn.lpstrTitle = "Save Texture Package";
					ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;

					if( GetSaveFileNameA(&ofn) )
					{
						TCHAR l_chCmd[256];

						appSprintf( l_chCmd, TEXT("OBJ SAVEPACKAGE PACKAGE=\"%ls\" FILE=\"%ls\""),
							*Package, appFromAnsi( File ) );
						GEditor->Exec( l_chCmd );

						FString S = appFromAnsi( File );
						mrulist->AddItem( S );
						if( GBrowserMaster->GetCurrent()==BrowserID )
							mrulist->AddToMenu( hWnd, GetMenu( IsDocked() ? OwnerWindow->hWnd : hWnd ) );
						GLastDir[eLASTDIR_UTX] = S.GetFilePath().LeftChop(1);
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

				FString Package = Filename.GetFilenameOnly();
				GBrowserMaster->RefreshAll();
				RefreshPackages();
				pComboPackage->SetCurrent( pComboPackage->FindStringExact( *Package ) );
				RefreshGroups();
				RefreshTextureList();
			}
				break;

			case IDMN_TB_IMPORT_IMAGE:
				{
					OPENFILENAMEA ofn;
					char File[8192] = "\0";

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(char) * 8192;
					ofn.lpstrFilter = "Image Files (*.pcx,*.bmp,*.dds,*.png,*.fx)\0*.pcx;*.bmp;*.dds;*.png;*.fx;\0 PCX Files\0*.pcx;\0 24/32bpp BMP Files\0*.bmp;\0 DDS Files\0*.dds;\0 24/32bpp PNG Files\0*.png;\0 Unreal FX Files\0*.fx;\0All Files\0*.*\0\0";
					ofn.lpstrDefExt = "*.pcx;*.bmp;*.dds;*.png;*.fx;";
					ofn.lpstrTitle = "Import Textures";
					ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_PCX]) );
					ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

					// Display the Open dialog box.
					//
					if( GetOpenFileNameA(&ofn) )
					{
						int iNULLs = FormatFilenames( File );
						FString Package = (pComboPackage->GetCurrent() > 0) ? pComboPackage->GetString(pComboPackage->GetCurrent()) : FString();
						FString Group = GetCurrentGroupName();

						TArray<FString> StringArray;
						ParseStringToArray( TEXT("|"), appFromAnsi( File ), &StringArray );

						int iStart = 0;
						FString Prefix = TEXT("\0");

						if( iNULLs )
						{
							iStart = 1;
							Prefix = *(StringArray(0));
							Prefix += TEXT("\\");
						}

						if( StringArray.Num() == 1 )
							GLastDir[eLASTDIR_PCX] = StringArray(0).GetFilePath().LeftChop(1);
						else
							GLastDir[eLASTDIR_PCX] = StringArray(0);

						TArray<FString> FilenamesArray;

						for( int x = iStart ; x < StringArray.Num() ; x++ )
						{
							FString NewString;

							NewString = FString::Printf( TEXT("%ls%ls"), *Prefix, *(StringArray(x)) );
							new(FilenamesArray)FString( NewString );

							FString S = NewString;
						}

						WDlgImportTexture l_dlg( NULL, this );
						if (l_dlg.DoModal(Package, Group, &FilenamesArray))
						{
							// Flip to the texture/group that was used for importing
							GBrowserMaster->RefreshAll();
							pComboPackage->SetCurrent(pComboPackage->FindStringExact(*l_dlg.Package));
							RefreshGroups();
							pComboGroup->SetCurrent(pComboGroup->FindStringExact(*l_dlg.Group));
							RefreshTextureList();
						}
					}

					GFileManager->SetDefaultDirectory(appBaseDir());
				}
				break;

			case IDMN_TB_EXPORT_IMAGE:
			{
				OPENFILENAMEA ofn;
				char File[8192] = "\0";

				if (!GEditor->CurrentTexture->IsValid())
				{
					appMsgf(TEXT("No valid Texture selected!"));
					break;
				}

				FString Name = GEditor->CurrentTexture->GetName();

				::sprintf_s(File, 256, "%ls", *Name);

				ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
				ofn.lStructSize = sizeof(OPENFILENAMEA);
				ofn.hwndOwner = hWnd;
				ofn.lpstrFile = File;
				ofn.nMaxFile = sizeof(char) * 8192;
				ofn.lpstrFilter = "Image Files (*.bmp,*.pcx,*.png, *.dds, *.fx)\0*.bmp;*.pcx;*.png;\0 24/32bpp BMP Files\0*.bmp;\0 PCX Files\0*.pcx;\0 24/32bpp PNG Files\0*.png;\0 DDS Files\0*.dds;\0 Unreal FX Files\0*.fx;\0All Files\0*.*\0\0";
				ofn.lpstrDefExt = "*.bmp;*.pcx;*.png;*.dds;*.fx;";
				ofn.lpstrTitle = "Export Texture";
				ofn.lpstrInitialDir = appToAnsi(*(GLastDir[eLASTDIR_PCX]));
				ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;

				// Display the Open dialog box.
				//
				if (GetSaveFileNameA(&ofn))
				{
					TCHAR l_chCmd[512];

					appSprintf(l_chCmd, TEXT("OBJ EXPORT TYPE=TEXTURE NAME=\"%ls\" FILE=\"%ls\""), *Name, appFromAnsi(File));
					GEditor->Exec(l_chCmd);

					FString S = appFromAnsi(File);
					GLastDir[eLASTDIR_PCX] = S.GetFilePath().LeftChop(1);
				}

				GFileManager->SetDefaultDirectory(appBaseDir());
			}
			break;

			case IDMN_TB_EXPORT_PCX:
				{
					OPENFILENAMEA ofn;
					char File[8192] = "\0";

					if (!GEditor->CurrentTexture->IsValid())
					{
						appMsgf(TEXT("No valid Texture selected!"));
						break;
					}

					FString Name = GEditor->CurrentTexture->GetName();

					::sprintf_s( File, ARRAY_COUNT(File), "%ls", *Name );

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(char) * 8192;
					ofn.lpstrFilter = "PCX Files (*.pcx)\0*.pcx\0All Files\0*.*\0\0";
					ofn.lpstrDefExt = "pcx";
					ofn.lpstrTitle = "Export Texture";
					ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_PCX]) );
					ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;

					// Display the Open dialog box.
					//
					if( GetSaveFileNameA(&ofn) )
					{
						TCHAR l_chCmd[512];

						appSprintf( l_chCmd, TEXT("OBJ EXPORT TYPE=TEXTURE NAME=\"%ls\" FILE=\"%ls\""),
							*Name, appFromAnsi( File ) );
						GEditor->Exec( l_chCmd );

						FString S = appFromAnsi( File );
						GLastDir[eLASTDIR_PCX] = S.GetFilePath().LeftChop(1);
					}

					GFileManager->SetDefaultDirectory(appBaseDir());
				}
				break;

				case IDMN_TB_EXPORT_BMP:
				{
					OPENFILENAMEA ofn;
					char File[8192] = "\0";

					if (!GEditor->CurrentTexture->IsValid())
					{
						appMsgf(TEXT("No valid Texture selected!"));
						break;
					}

					FString Name = GEditor->CurrentTexture->GetName();

					::sprintf_s( File, 256, "%ls", *Name );

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(char) * 8192;
					ofn.lpstrFilter = "BMP Files (*.bmp)\0*.bmp\0All Files\0*.*\0\0";
					ofn.lpstrDefExt = "bmp";
					ofn.lpstrTitle = "Export Texture";
					ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_PCX]) );
					ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;

					// Display the Open dialog box.
					//
					if( GetSaveFileNameA(&ofn) )
					{
						TCHAR l_chCmd[512];

						appSprintf( l_chCmd, TEXT("OBJ EXPORT TYPE=TEXTURE NAME=\"%ls\" FILE=\"%ls\""),
							*Name, appFromAnsi( File ) );
						GEditor->Exec( l_chCmd );

						FString S = appFromAnsi( File );
						GLastDir[eLASTDIR_PCX] = S.GetFilePath().LeftChop(1);
					}

					GFileManager->SetDefaultDirectory(appBaseDir());
				}
				break;

				case IDMN_TB_EXPORT_PNG:
				{
					OPENFILENAMEA ofn;
					char File[8192] = "\0";

					if (!GEditor->CurrentTexture->IsValid())
					{
						appMsgf(TEXT("No valid Texture selected!"));
						break;
					}

					FString Name = GEditor->CurrentTexture->GetName();

					::sprintf_s(File, 256, "%ls", *Name);

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(char) * 8192;
					ofn.lpstrFilter = "PNG Files (*.png)\0*.png\0All Files\0*.*\0\0";
					ofn.lpstrDefExt = "png";
					ofn.lpstrTitle = "Export Texture";
					ofn.lpstrInitialDir = appToAnsi(*(GLastDir[eLASTDIR_PCX]));
					ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;

					// Display the Open dialog box.
					//
					if (GetSaveFileNameA(&ofn))
					{
						TCHAR l_chCmd[512];

						appSprintf(l_chCmd, TEXT("OBJ EXPORT TYPE=TEXTURE NAME=\"%ls\" FILE=\"%ls\""),
							*Name, appFromAnsi(File));
						GEditor->Exec(l_chCmd);

						FString S = appFromAnsi(File);
						GLastDir[eLASTDIR_PCX] = S.GetFilePath().LeftChop(1);
					}

					GFileManager->SetDefaultDirectory(appBaseDir());
				}
				break;

				case IDMN_TB_EXPORT_DDS:
				{
					OPENFILENAMEA ofn;
					char File[8192] = "\0";

					if (!GEditor->CurrentTexture->IsValid())
					{
						appMsgf(TEXT("No valid Texture selected!"));
						break;
					}

					FString Name = GEditor->CurrentTexture->GetName();

					::sprintf_s(File, 256, "%ls", *Name);

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(char) * 8192;
					ofn.lpstrFilter = "DDS Files (*.dds)\0*.dds\0All Files\0*.*\0\0";
					ofn.lpstrDefExt = "dds";
					ofn.lpstrTitle = "Export Texture";
					ofn.lpstrInitialDir = appToAnsi(*(GLastDir[eLASTDIR_PCX]));
					ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;

					// Display the Open dialog box.
					//
					if (GetSaveFileNameA(&ofn))
					{
						TCHAR l_chCmd[512];

						appSprintf(l_chCmd, TEXT("OBJ EXPORT TYPE=TEXTURE NAME=\"%ls\" FILE=\"%ls\""),
							*Name, appFromAnsi(File));
						GEditor->Exec(l_chCmd);

						FString S = appFromAnsi(File);
						GLastDir[eLASTDIR_PCX] = S.GetFilePath().LeftChop(1);
					}

					GFileManager->SetDefaultDirectory(appBaseDir());
				}
				break;

				case IDMN_TB_EXPORT_BMP_ALL:
				if (pComboPackage->GetCurrent() > 0)
				{
					FString CurrentPackage = pComboPackage->GetString(pComboPackage->GetCurrent());
					TCHAR TempStr[256];

					// Make package directory.
					appStrcpy( TempStr, TEXT("..") PATH_SEPARATOR );
					appStrcat( TempStr, *CurrentPackage );
					GFileManager->MakeDirectory( TempStr, 0 );

					// Make package\Textures directory.
					appStrcat( TempStr, PATH_SEPARATOR TEXT("Textures") );
					GFileManager->MakeDirectory( TempStr, 0 );
					GFileManager->SetDefaultDirectory(TempStr);

					for( TObjectIterator<UTexture> It; It; ++It )
						{
							UTexture* A = *It;
							if( A->IsA(UTexture::StaticClass()))
							{
								FString PathName=A->GetPathName();
								FString TempPackageName = PathName.Left(CurrentPackage.Len());
								if (TempPackageName == CurrentPackage)
								{
									const TCHAR* GroupDir = A->GetOuter()->GetName();
									TCHAR TempName[256]=TEXT("");
									FString Name = A->GetName();
									char File[8192] = "\0";
									if (appStrcmp(GroupDir,*TempPackageName) != 0)
									{
										GFileManager->MakeDirectory( GroupDir, 0 );
										appStrcat( TempName, GroupDir);
										appStrcat( TempName, PATH_SEPARATOR);
										appStrcat( TempName, *Name);
									}
									else appStrcat( TempName, *Name);
									appStrcat( TempName, TEXT(".bmp"));
									::sprintf_s( File,256, "%ls", TempName );
									*TempName=0;
									debugf(TEXT("Exporting: %ls"),appFromAnsi(File));
									TCHAR l_chCmd[512];
									appSprintf( l_chCmd, TEXT("OBJ EXPORT TYPE=TEXTURE NAME=\"%ls\" FILE=\"%ls\""), *Name, appFromAnsi( File ) );
									GEditor->Exec( l_chCmd );
								}
							}
							else if (CurrentPackage == A->GetOuter()->GetName())
								debugf(TEXT("Skipping %ls"), A->GetFullName());
						}
					GFileManager->SetDefaultDirectory(appBaseDir());
				}
				break;

				case IDMN_TB_EXPORT_PNG_ALL:
				if (pComboPackage->GetCurrent() > 0)
				{
					FString CurrentPackage = pComboPackage->GetString(pComboPackage->GetCurrent());
					TCHAR TempStr[256];

					// Make package directory.
					appStrcpy(TempStr, TEXT("..") PATH_SEPARATOR);
					appStrcat(TempStr, *CurrentPackage);
					GFileManager->MakeDirectory(TempStr, 0);

					// Make package\Textures directory.
					appStrcat(TempStr, PATH_SEPARATOR TEXT("Textures"));
					GFileManager->MakeDirectory(TempStr, 0);
					GFileManager->SetDefaultDirectory(TempStr);

					for (TObjectIterator<UTexture> It; It; ++It)
					{
						UTexture* A = *It;
						if (A->IsA(UTexture::StaticClass()))
						{
							FString PathName = A->GetPathName();
							FString TempPackageName = PathName.Left(CurrentPackage.Len());
							if (TempPackageName == CurrentPackage)
							{
								const TCHAR* GroupDir = A->GetOuter()->GetName();
								TCHAR TempName[256] = TEXT("");
								FString Name = A->GetName();
								char File[8192] = "\0";
								if (appStrcmp(GroupDir, *TempPackageName) != 0)
								{
									GFileManager->MakeDirectory(GroupDir, 0);
									appStrcat(TempName, GroupDir);
									appStrcat(TempName, PATH_SEPARATOR);
									appStrcat(TempName, *Name);
								}
								else appStrcat(TempName, *Name);
								appStrcat(TempName, TEXT(".png"));
								::sprintf_s(File, 256, "%ls", TempName);
								*TempName = 0;
								debugf(TEXT("Exporting: %ls"), appFromAnsi(File));
								TCHAR l_chCmd[512];
								appSprintf(l_chCmd, TEXT("OBJ EXPORT TYPE=TEXTURE NAME=\"%ls\" FILE=\"%ls\""), *Name, appFromAnsi(File));
								GEditor->Exec(l_chCmd);
							}
						}
						else if (CurrentPackage == A->GetOuter()->GetName())
							debugf(TEXT("Skipping %ls"), A->GetFullName());
					}
					GFileManager->SetDefaultDirectory(appBaseDir());
				}
				break;

				case IDMN_TB_EXPORT_DDS_ALL:
				if (pComboPackage->GetCurrent() > 0)
				{
					FString CurrentPackage = pComboPackage->GetString(pComboPackage->GetCurrent());
					TCHAR TempStr[256];

					// Make package directory.
					appStrcpy(TempStr, TEXT("..") PATH_SEPARATOR);
					appStrcat(TempStr, *CurrentPackage);
					GFileManager->MakeDirectory(TempStr, 0);

					// Make package\Textures directory.
					appStrcat(TempStr, PATH_SEPARATOR TEXT("Textures"));
					GFileManager->MakeDirectory(TempStr, 0);
					GFileManager->SetDefaultDirectory(TempStr);

					for (TObjectIterator<UTexture> It; It; ++It)
					{
						UTexture* A = *It;
						if (A->IsA(UTexture::StaticClass()))
						{
							FString PathName = A->GetPathName();
							FString TempPackageName = PathName.Left(CurrentPackage.Len());
							if (TempPackageName == CurrentPackage)
							{
								const TCHAR* GroupDir = A->GetOuter()->GetName();
								TCHAR TempName[256] = TEXT("");
								FString Name = A->GetName();
								char File[8192] = "\0";
								if (appStrcmp(GroupDir, *TempPackageName) != 0)
								{
									GFileManager->MakeDirectory(GroupDir, 0);
									appStrcat(TempName, GroupDir);
									appStrcat(TempName, PATH_SEPARATOR);
									appStrcat(TempName, *Name);
								}
								else appStrcat(TempName, *Name);
								appStrcat(TempName, TEXT(".dds"));
								::sprintf_s(File, 256, "%ls", TempName);
								*TempName = 0;
								debugf(TEXT("Exporting: %ls"), appFromAnsi(File));
								TCHAR l_chCmd[512];
								appSprintf(l_chCmd, TEXT("OBJ EXPORT TYPE=TEXTURE NAME=\"%ls\" FILE=\"%ls\""), *Name, appFromAnsi(File));
								GEditor->Exec(l_chCmd);
							}
						}
						else if (CurrentPackage == A->GetOuter()->GetName())
							debugf(TEXT("Skipping %ls"), A->GetFullName());
					}
					GFileManager->SetDefaultDirectory(appBaseDir());
				}
				break;

				case IDMN_TB_EXPORT_PCX_ALL:
				if (pComboPackage->GetCurrent() > 0)
				{
					FString CurrentPackage = pComboPackage->GetString(pComboPackage->GetCurrent());
					TCHAR TempStr[256];

					// Make package directory.
					appStrcpy( TempStr, TEXT("..") PATH_SEPARATOR );
					appStrcat( TempStr, *CurrentPackage );
					GFileManager->MakeDirectory( TempStr, 0 );

					// Make package\Textures directory.
					appStrcat( TempStr, PATH_SEPARATOR TEXT("Textures") );
					GFileManager->MakeDirectory( TempStr, 0 );
					GFileManager->SetDefaultDirectory(TempStr);

					for( TObjectIterator<UTexture> It; It; ++It )
						{
							UTexture* A = *It;
							if( A->IsA(UTexture::StaticClass()))
							{
								FString PathName=A->GetPathName();
								FString TempPackageName = PathName.Left(CurrentPackage.Len());

								//debugf(TEXT("Exporting: CurrentPackage: %ls -- A->GetOuter()->GetName(): %ls -- A->GetFullName(): %ls"),*PathName,A->GetFullName());
								//OPENFILENAMEA ofn;
								//if (CurrentPackage == A->GetOuter()->GetName())
								if (TempPackageName == CurrentPackage)
								{
									const TCHAR* GroupDir = A->GetOuter()->GetName();
									TCHAR TempName[256]=TEXT("");
									FString Name = A->GetName();
									char File[8192] = "\0";
									//appStrcpy( GroupDir, TEXT(".") PATH_SEPARATOR );
									//debugf(TEXT("GroupDir %ls, TempPackageName %ls"),GroupDir,*TempPackageName);
									if (appStrcmp(GroupDir,*TempPackageName) != 0)
									{
										GFileManager->MakeDirectory( GroupDir, 0 );
										appStrcat( TempName, GroupDir);
										appStrcat( TempName, PATH_SEPARATOR);
										appStrcat( TempName, *Name);
									}
									else appStrcat( TempName, *Name);
									if(bCompressedFormat(A->Format))
									{
										debugf(TEXT("converting compressed texture %ls to .bmp"),*Name);
										appStrcat( TempName, TEXT(".bmp"));
									}
									else appStrcat( TempName, TEXT(".pcx"));
									::sprintf_s( File,256, "%ls", TempName );
									*TempName=0;
									debugf(TEXT("Exporting: %ls"),appFromAnsi(File));
									TCHAR l_chCmd[512];
									appSprintf( l_chCmd, TEXT("OBJ EXPORT TYPE=TEXTURE NAME=\"%ls\" FILE=\"%ls\""), *Name, appFromAnsi( File ) );
									GEditor->Exec( l_chCmd );
								}
							}
							else if (CurrentPackage == A->GetOuter()->GetName())
								debugf(TEXT("Skipping %ls"), A->GetFullName());
						}
					GFileManager->SetDefaultDirectory(appBaseDir());
				}
				break;

			case IDMN_RD_SOFTWARE:
			case IDMN_RD_OPENGL:
			case IDMN_RD_D3D:
			case IDMN_RD_D3D9:
			case IDMN_RD_XOPENGL:
			case IDMN_RD_ICBINDX11:
			case IDMN_RD_CUSTOMRENDER:
				{
					SwitchRenderer(Command, TRUE);
					check(pViewport);
					FlushRepaintViewport();
					PositionChildControls();
					UpdateMenu();
				}
				break;

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
	void GetUsedTextures( TArray<UObject*>& TList )
	{
		// Declare FArchive finder
		class FTexFinder : public FArchive
		{
		public:
			TArray<UObject*>& Tex;
			FTexFinder(TArray<UObject*>& TL)
				: FArchive()
				, Tex(TL)
			{}
			FArchive& operator<<(class UObject*& Res)
			{
				if (Res && (Res->IsA(UTexture::StaticClass()) || Res->IsA(UFont::StaticClass())))
					Tex.AddUniqueItem((UTexture*)Res);
				return *this;
			}
		};

		UObject* MyLvl = GEditor->Level->GetOuter();
		FTexFinder TFinder(TList);
		for( FObjectIterator It(UObject::StaticClass()); It; ++It )
		{
			UObject* O = *It;
			if (O->IsIn(MyLvl))
				O->Serialize(TFinder);
		}
	}
	void RefreshPackages( void )
	{
		guard(WBrowserTexture::RefreshPackages);

		SetRedraw(false);

		int Current = pComboPackage->GetCurrent();
		Current = (Current != CB_ERR) ? Current : 1;
		FString Package = pComboPackage->GetString( pComboPackage->GetCurrent() );

		// PACKAGES
		//
		pComboPackage->Empty();

		TArray<UObject*> PgkList;
		if( bListInUse )
		{
			TArray<UObject*> UsedTex;
			GetUsedTextures(UsedTex);
			for( INT i=0; i<UsedTex.Num(); i++ )
				PgkList.AddUniqueItem(UsedTex(i)->TopOuter());
		}
		else
		{
			{
				for (TObjectIterator<UTexture> It; It; ++It)
					PgkList.AddUniqueItem(It->TopOuter());
			}
			{
				for (TObjectIterator<UFont> It; It; ++It)
					PgkList.AddUniqueItem(It->TopOuter());
			}
		}

		pComboPackage->AddString(TEXT("(All)"));
		for( int x = 0 ; x < PgkList.Num() ; x++ )
			pComboPackage->AddString( PgkList(x)->GetName() );

		if( Package.Len() )
		{
			Current = pComboPackage->FindString(*Package);
			pComboPackage->SetCurrent((Current != INDEX_NONE) ? Current : 1);
		}
		else pComboPackage->SetCurrent(Current);

		SetRedraw(true);

		unguard;
	}
	inline UBOOL TextureIsNotFont(UObject* Tex)
	{
		return (!Tex->GetOuter() || !Tex->GetOuter()->IsA(UFont::StaticClass()));
	}
	void RefreshGroups( void )
	{
		guard(WBrowserTexture::RefreshGroups);

		UBOOL bFilterPck = (pComboPackage->GetCurrent() != 0);
		int Current = pComboGroup->GetCurrent();
		Current = (Current != CB_ERR) ? Current : 0;

		// GROUPS
		//
		pComboGroup->Empty();

		TArray<FName> GrpList;
		TArray<FName> FontList;
		FName PckName(NAME_None);
		if (bFilterPck)
		{
			FString Package = pComboPackage->GetString(pComboPackage->GetCurrent());
			PckName = FName(*Package, FNAME_Find);
		}
		if( bListInUse )
		{
			UBOOL bListedFonts = FALSE;
			TArray<UObject*> UsedTex;
			GetUsedTextures(UsedTex);
			for( INT i=0; i<UsedTex.Num(); i++ )
			{
				if (bFilterPck && UsedTex(i)->TopOuter()->GetFName() != PckName)
					continue;
				if (UsedTex(i)->IsA(UFont::StaticClass()))
				{
					if (!bListedFonts)
					{
						GrpList.AddUniqueItem(NAME_FontGroupName);
						bListedFonts = TRUE;
					}
				}
				else if (HasValidGroup(UsedTex(i)))
				{
					if (TextureIsNotFont(UsedTex(i)))
						GrpList.AddUniqueItem(GetGroupObject(UsedTex(i))->GetFName());
					else FontList.AddUniqueItem(UsedTex(i)->GetOuter()->GetFName());
				}
				else GrpList.AddUniqueItem(NAME_None);
			}
		}
		else
		{
			for( TObjectIterator<UTexture> It; It; ++It )
			{
				if (bFilterPck && It->TopOuter()->GetFName() != PckName)
					continue;
				if (HasValidGroup(*It))
				{
					if (TextureIsNotFont(*It))
						GrpList.AddUniqueItem(GetGroupObject(*It)->GetFName());
					else FontList.AddUniqueItem(It->GetOuter()->GetFName());
				}
				else GrpList.AddUniqueItem(NAME_None);
			}
			for (TObjectIterator<UFont> It; It; ++It)
			{
				if (bFilterPck && It->TopOuter()->GetFName() != PckName)
					continue;
				GrpList.AddUniqueItem(NAME_FontGroupName);
				break;
			}
		}

		bHasAllGroups = (GrpList.Num() > 1);
		if (bHasAllGroups)
			pComboGroup->AddString(TEXT("(All)"));
		INT x;
		for (x = 0; x < GrpList.Num(); x++)
			pComboGroup->AddString( *GrpList(x) );
		if (FontList.Num())
		{
			TCHAR* Buffer = appStaticString1024();
			for (x = 0; x < FontList.Num(); x++)
			{
				appSnprintf(Buffer, 1024, TEXT("%ls [Font]"), *FontList(x));
				pComboGroup->AddString(Buffer);
			}
		}

		pComboGroup->SetCurrent(0);
		pComboGroup->SetCurrent(Current);

		unguard;
	}
	void ReDrawCamera()
	{
		BOOL bWasRealTime = (pViewport && pViewport->Actor && (pViewport->Actor->ShowFlags & SHOW_RealTime));
		static TCHAR l_chCmd[1024];
		appSprintf(l_chCmd, TEXT("CAMERA UPDATE FLAGS=%d MISC1=%d MISC2=%d REN=%d NAME=TextureBrowser"),
			SHOW_StandardView | SHOW_NoButtons | SHOW_ChildWindow | (bWasRealTime ? SHOW_RealTime : 0),
			iZoom,
			iScroll,
			REN_TexBrowser);
		GEditor->Exec(l_chCmd);
	}
	inline const TCHAR* GetNameUpper(UObject* InTex)
	{
		static TCHAR Buffer[NAME_SIZE];
		const TCHAR* S = InTex->GetName();
		TCHAR* D = Buffer;
		while (*S)
			*D++ = appToUpper(*S++);
		*D = 0;
		return Buffer;
	}
	void RefreshTextureList( void )
	{
		guard(WBrowserTexture::RefreshTextureList);

		FName PckName, GrpName;
		FString NameFilter = pEditFilter->GetText().Caps();
		UBOOL bFilterPck = (pComboPackage->GetCurrent() != 0);
		UBOOL bShowAllNow = bHasAllGroups ? (pComboGroup->GetCurrent() == 0) : FALSE;

		if (bFilterPck)
		{
			FString Result = pComboPackage->GetString(pComboPackage->GetCurrent());
			PckName = FName(*Result, FNAME_Find);
		}
		if (!bShowAllNow)
		{
			FString Result = pComboGroup->GetString(pComboGroup->GetCurrent());
			if (Result.Right(7) == TEXT(" [Font]"))
				Result = Result.Left(Result.Len() - 7);
			GrpName = FName(*Result, FNAME_Find);
		}

		ForcedTexList.Empty();
		if( bListInUse )
		{
			TArray<UObject*> TL;
			GetUsedTextures(TL);
			for( INT i=0; i<TL.Num(); i++ )
			{
				UObject* T = TL(i);
				if (bFilterPck && T->TopOuter()->GetFName() != PckName)
					continue;
				if (T->IsA(UFont::StaticClass()))
				{
					if ((bShowAllNow || GrpName == NAME_FontGroupName) && (!NameFilter.Len() || appStrstr(GetNameUpper(T), *NameFilter)))
						ForcedTexList.AddItem(T);
				}
				else if ((bShowAllNow ? TextureIsNotFont(T) : ((GrpName == NAME_None && !HasValidGroup(T)) || GrpName == GetGroupObject(T)->GetFName())) && (!NameFilter.Len() || appStrstr(GetNameUpper(T), *NameFilter)))
					ForcedTexList.AddItem(T);
			}
		}
		else
		{
			for( TObjectIterator<UTexture> It; It; ++It )
			{
				UTexture* T = *It;
				if (bFilterPck && T->TopOuter()->GetFName() != PckName)
					continue;
				if (bShowAllNow ? TextureIsNotFont(T) : ((GrpName == NAME_None && !HasValidGroup(T)) || GrpName == GetGroupObject(T)->GetFName()))
					if (!NameFilter.Len() || appStrstr(GetNameUpper(T), *NameFilter))
						ForcedTexList.AddItem(T);
			}
			for (TObjectIterator<UFont> It; It; ++It)
			{
				UFont* T = *It;
				if (bFilterPck && T->TopOuter()->GetFName() != PckName)
					continue;
				if ((bShowAllNow || GrpName == NAME_FontGroupName) && (!NameFilter.Len() || appStrstr(GetNameUpper(T), *NameFilter)))
					ForcedTexList.AddItem(T);
			}
		}
		if( ForcedTexList.Num() )
		{
			// Sort textures by name.
			appQsort( &ForcedTexList(0), ForcedTexList.Num(), sizeof(UObject *), ResNameCompare );
		}

		ReDrawCamera();

		RefreshScrollBar();
		UpdateMenu();

		unguard;
	}

	void RefreshScrollBar( void )
	{
		guard(WBrowserTexture::RefreshScrollBar);

		if( !pScrollBar ) return;

		// Set the scroll bar to have a valid range.
		//
		INT iOldScroll = iScroll;
		SCROLLINFO si;
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_DISABLENOSCROLL | SIF_RANGE | SIF_POS;
		si.nMin = 0;
		si.nMax = GLastScroll;
		si.nPos = Min(iScroll, GLastScroll);
		iScroll = SetScrollInfo( pScrollBar->hWnd, SB_CTL, &si, TRUE );
		if (iOldScroll != iScroll)
			ReDrawCamera();
		unguard;
	}

	// Moves the child windows around so that they best match the window size.
	//
	void PositionChildControls( void )
	{
		guard(WBrowserTexture::PositionChildControls);
		if( !pComboPackage
			|| !pComboGroup
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
		float Top = R.bottom + 4;

		::MoveWindow( pComboPackage->hWnd, 4, Top, Fraction * 10, 20, 1 );

		Top += MulDiv(24, DPIY, 96);
		::MoveWindow( pComboGroup->hWnd, 4, Top, Fraction * 10, 20, 1 );

		Top += MulDiv(24, DPIY, 96);
		::MoveWindow( (HWND)pViewport->GetWindow(), 4, Top, CR.Width() - MulDiv(20, DPIX, 96), CR.Height() - MulDiv(80, DPIY, 96) - R.bottom, 1 );
		pViewport->Repaint(1);
		::MoveWindow( pScrollBar->hWnd, CR.Width() - MulDiv(16, DPIX, 96), Top, MulDiv(16, DPIX, 96), CR.Height() - MulDiv(80, DPIY, 96) - R.bottom, 1 );

		Top += CR.Height() - MulDiv(80, DPIY, 96) - R.bottom + 4;
		::MoveWindow( pLabelFilter->hWnd, 4, Top + 2, MulDiv(48, DPIX, 96), MulDiv(20, DPIY, 96), 1 );
		::MoveWindow( pEditFilter->hWnd, 4 + MulDiv(48, DPIX, 96), Top, CR.Width() - MulDiv(48, DPIX, 96), MulDiv(20, DPIY, 96), 1 );
		unguard;
	}
	virtual void SetCaption( FString* Tail = NULL )
	{
		guard(WBrowserTexture::SetCaption);

		FString Extra;

		if( GEditor->CurrentTexture )
		{
			Extra = (FString::Printf(TEXT("%ls (%dx%d) %ls - %i Mip(s)"),
				GEditor->CurrentTexture->GetPathName(), GEditor->CurrentTexture->USize, GEditor->CurrentTexture->VSize, *FTextureFormatString(GEditor->CurrentTexture->Format), GEditor->CurrentTexture->Mips.Num()));
		}
		else if (GEditor->CurrentFont)
		{
			Extra = FString::Printf(TEXT("%ls"), GEditor->CurrentFont->GetFullName());
		}

		WBrowser::SetCaption( Extra );
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
		RefreshTextureList();
		unguard;
	}
	void OnInUseOnlyClick()
	{
		bListInUse = !bListInUse;
		RefreshAll();
	}
	void OnEditFilterChange()
	{
		guard(WBrowserTexture::OnEditFilterChange);
		RefreshTextureList();
		unguard;
	}
	virtual void OnVScroll( WPARAM wParam, LPARAM lParam )
	{
		if ((HWND)lParam == pScrollBar->hWnd || lParam == WM_MOUSEWHEEL)
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
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
