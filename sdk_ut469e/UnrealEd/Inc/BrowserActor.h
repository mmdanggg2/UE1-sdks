/*=============================================================================
	BrowserActor : Browser window for actor classes
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>

// --------------------------------------------------------------
//
// NEW CLASS Dialog
//
// --------------------------------------------------------------

class WDlgNewClass : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgNewClass,WDialog,UnrealEd)

	// Variables.
	WButton OkButton;
	WButton CancelButton;
	WLabel ParentLabel;
	WPackageComboBox PackageEdit;
	WEdit NameEdit;

	FString ParentClass, Package, Name;
	UClass* ReplacedClass;

	// Constructor.
	WDlgNewClass( UObject* InContext, WWindow* InOwnerWindow )
	:	WDialog			( TEXT("New Class"), IDDIALOG_NEW_CLASS, InOwnerWindow )
	,	OkButton		( this, IDOK,			FDelegate(this,(TDelegate)&WDlgNewClass::OnOk) )
	,	CancelButton	( this, IDCANCEL,		FDelegate(this,(TDelegate)&WDlgNewClass::EndDialogFalse) )
	,	ParentLabel		( this, IDSC_PARENT )
	,	PackageEdit		( this, IDEC_PACKAGE )
	,	NameEdit		( this, IDEC_NAME )
	,	ReplacedClass	( NULL )
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgNewClass::OnInitDialog);
		WDialog::OnInitDialog();

		// Make sure this class has a script.
		FStringOutputDevice GetPropResult = FStringOutputDevice();
	    GEditor->Get(TEXT("SCRIPTPOS"), *ParentClass, GetPropResult);
		if( !GetPropResult.Len() )
		{
			appMsgf( *(FString::Printf(TEXT("Subclassing of '%ls' is not allowed."), *ParentClass)) );
			EndDialog(TRUE);
		}

		// stijn: the ParentClass might be a pathname so parse the basename here
		FString BaseName = *ParentClass.Mid(ParentClass.InStr(TEXT("."), 1) + 1);
		ParentLabel.SetText( *BaseName );
		PackageEdit.Init(TEXT("MyLevel"));
		NameEdit.SetText( *(FString::Printf(TEXT("My%ls"), *BaseName )));
		::SetFocus( PackageEdit.hWnd );
		ReplacedClass = NULL;

		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgNewClass::OnDestroy);
		WDialog::OnDestroy();
		unguard;
	}
	virtual int DoModal( FString _ParentClass )
	{
		guard(WDlgNewClass::DoModal);

		ParentClass = _ParentClass;

		return WDialog::DoModal( hInstance );
		unguard;
	}
	void OnOk()
	{
		guard(WDlgNewClass::OnOk);
		if( GetDataFromUser() )
		{
			// Check if class already exists.
			//
			ReplacedClass = NULL;
			debugf(TEXT("Checking for duplicate classname: %ls"), *Name);
			UClass* Class = FindObject<UClass>(ANY_PACKAGE, *Name, 1);
			if (Class)
			{
				debugf(TEXT("Found duplicate classname: %ls"), *FObjectFullName(Class));
				if( Package != FObjectName(Class->GetOuter()) )
				{
					appMsgf(TEXT("Can't create duplicate of %ls with different package name."), *FObjectFullName(Class));
					return;
				}
				if( !GWarn->YesNof(TEXT("An Actor class named '%ls' already exists! Do you want to replace it?"), *FObjectFullName(Class)))
					return;
				ReplacedClass = Class;
			}

			EndDialog(TRUE);
		}
		unguard;
	}
	BOOL GetDataFromUser( void )
	{
		guard(WDlgNewClass::GetDataFromUser);
		Package = PackageEdit.GetText();
		Name = NameEdit.GetText();

		if( !Package.Len()
				|| !Name.Len() )
		{
			appMsgf( TEXT("Invalid input.") );
			return FALSE;
		}
		else
		{
			// Validate class and package names
			for (INT Pos = 0, N = Package.Len(); Pos < N; Pos++)
				if (!appIsAlnum((*Package)[Pos]) && (*Package)[Pos] != '_')
				{
					appMsgf( TEXT("Invalid character in package name '%lc'"), (*Package)[Pos]);
					return FALSE;
				}
			if (appIsDigit((*Name)[0]))
			{
				appMsgf( TEXT("A class name cannot start with a digit."));
				return FALSE;
			}
			for (INT Pos = 0, N = Name.Len(); Pos < N; Pos++)
				if (!appIsAlnum((*Name)[Pos]) && (*Name)[Pos] != '_')
				{
					appMsgf( TEXT("Invalid character in class name '%lc'"), (*Name)[Pos]);
					return FALSE;
				}

			return TRUE;
		}
		unguard;
	}
};

// --------------------------------------------------------------
//
// FIND CLASS Dialog
//
// --------------------------------------------------------------

class WDlgFindClass : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgFindClass, WDialog, UnrealEd)

	// Variables.
	WButton OkButton;
	WButton CancelButton;
	WEdit NameEdit;

	FString Name;

	// Constructor.
	WDlgFindClass(UObject* InContext, WWindow* InOwnerWindow)
		: WDialog(TEXT("Find Class"), IDDIALOG_FIND_CLASS, InOwnerWindow)
		, OkButton(this, IDOK, FDelegate(this, (TDelegate)&WDlgFindClass::OnOk))
		, CancelButton(this, IDCANCEL, FDelegate(this, (TDelegate)&WDialog::EndDialogFalse))
		, NameEdit(this, IDEC_NAME)
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgFindClass::OnInitDialog);
		WDialog::OnInitDialog();
		::SetFocus(NameEdit.hWnd);
		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgFindClass::OnDestroy);
		WDialog::OnDestroy();
		unguard;
	}
	virtual int DoModal()
	{
		guard(WDlgFindClass::DoModal);
		return WDialog::DoModal(hInstance);
		unguard;
	}
	void OnOk()
	{
		guard(WDlgFindClass::OnOk);
		if (GetDataFromUser())
		{
			// Check if class exists.
			//
			debugf(TEXT("Checking for classname: %ls"), *Name);
			UClass* Class = FindObject<UClass>(ANY_PACKAGE, *Name, 1);
			if (Class)
			{
				debugf(TEXT("Found classname: %ls"), *FObjectFullName(Class));
				GEditor->Exec(*(FString::Printf(TEXT("SETCURRENTCLASS CLASS=\"%ls\""), *FObjectPathName(Class))));
			}
			else return appMsgf(TEXT("Class %ls not found!"), *Name);
			EndDialog(TRUE);
		}
		unguard;
	}
	BOOL GetDataFromUser(void)
	{
		guard(WDlgFindClass::GetDataFromUser);
		Name = NameEdit.GetText();
		if (!Name.Len())
		{
			appMsgf(TEXT("Invalid input."));
			return FALSE;
		}
		return TRUE;
		unguard;
	}
};

// --------------------------------------------------------------
//
// WBrowserActor
//
// --------------------------------------------------------------

enum EExportType
{
	ET_ALL,
	ET_CHANGED,
	ET_SELECTED,
};

#define ID_BA_TOOLBAR	29030
TBBUTTON tbBAButtons[] = {
	{ 0, IDMN_MB_DOCK, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 1, ID_FileOpen, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 2, ID_FileSave, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 3, ID_FileNew, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 4, IDMN_AB_EDIT_SCRIPT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 5, ID_EditDelete, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 6, IDMN_AB_DEF_PROP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
};
struct {
	TCHAR ToolTip[64];
	int ID;
} ToolTips_BA[] = {
	TEXT("Toggle Dock Status"), IDMN_MB_DOCK,
	TEXT("Open Package"), ID_FileOpen,
	TEXT("Save Selected Packages"), ID_FileSave,
	TEXT("New Script"), ID_FileNew,
	TEXT("Edit Script"), IDMN_AB_EDIT_SCRIPT,
	TEXT("Remove Class"), ID_EditDelete,
	TEXT("Edit Default Properties"), IDMN_AB_DEF_PROP,
	NULL, 0
};

class WBrowserActor : public WBrowser
{
	DECLARE_WINDOWCLASS(WBrowserActor,WBrowser,Window)

	WTreeView* pTreeView;
	WCheckBox* pObjectsCheck;
	WCheckListBox* pPackagesList{};
	HTREEITEM htiRoot, htiLastSel;
	HWND hWndToolBar{};
	WToolTip* ToolTipCtrl{};
	HBITMAP ToolbarImage{};

	UBOOL bShowPackages{};

	// Structors.
	WBrowserActor( FName InPersistentName, WWindow* InOwnerWindow, HWND InEditorFrame )
	:	WBrowser( InPersistentName, InOwnerWindow, InEditorFrame )
	{
		pTreeView = NULL;
		pObjectsCheck = NULL;
		htiRoot = htiLastSel = NULL;
		MenuID = IDMENU_BrowserActor;
		BrowserID = eBROWSER_ACTOR;
		Description = TEXT("Actor Classes");
		mrulist = NULL;
	}

	// WBrowser interface.
	virtual void SetCaption( FString* Tail = NULL )
	{
		guard(WBrowserActor::SetCaption);

		FString Extra;
		if( GEditor->CurrentClass )
			Extra = FObjectPathName(GEditor->CurrentClass);

		WBrowser::SetCaption( &Extra );
		unguard;
	}
	void OnCreate()
	{
		guard(WBrowserActor::OnCreate);
		WBrowser::OnCreate();

		SetRedraw(false);

		SetMenu( hWnd, AddHotkeys(LoadMenuW(hInstance, MAKEINTRESOURCEW(IDMENU_BrowserActor))) );
		pObjectsCheck = new WCheckBox( this, IDCK_OBJECTS );
		pObjectsCheck->ClickDelegate = FDelegate(this, (TDelegate)&WBrowserActor::OnObjectsClick);
		pObjectsCheck->OpenWindow( 1, 0, 0, 1, 1, TEXT("Actor classes only") );
		SendMessageW( pObjectsCheck->hWnd, BM_SETCHECK, BST_CHECKED, 0 );
		
		pTreeView = new WTreeView( this, IDTV_TREEVIEW );
		pTreeView->OpenWindow( 1, 1, 0, 0, 1 );
		pTreeView->SelChangedDelegate = FDelegate(this, (TDelegate)&WBrowserActor::OnTreeViewSelChanged);
		pTreeView->ItemExpandingDelegate = FDelegate(this, (TDelegate)&WBrowserActor::OnTreeViewItemExpanding);
		pTreeView->DblClkDelegate = FDelegate(this, (TDelegate)&WBrowserActor::OnTreeViewDblClk);
		
		pPackagesList = new WCheckListBox( this, IDLB_PACKAGES );
		pPackagesList->OpenWindow( 1, 0, 0, 1 );
		pPackagesList->DoubleClickDelegate = FDelegate(this, (TDelegate)&WBrowserActor::OnPackagesListDblClk);

		if(!GConfig->GetInt( *PersistentName, TEXT("ShowPackages"), bShowPackages, GUEDIni ))		bShowPackages = 1;

		ToolbarImage = reinterpret_cast<HBITMAP>(LoadImage(hInstance,
			MAKEINTRESOURCE(IDB_BrowserActor_TOOLBAR),
			IMAGE_BITMAP, 0, 0, 0));
		check(ToolbarImage);
		ScaleImageAndReplace(ToolbarImage);

		// stijn: it would be nice if we could use CreateWindow instead, but WINXP wants CreateToolbarEx instead...
		hWndToolBar = CreateToolbarEx(
			hWnd, WS_CHILD | WS_BORDER | WS_VISIBLE | CCS_ADJUSTABLE,
			IDB_BrowserActor_TOOLBAR,
			7,
			NULL,
			reinterpret_cast<PTRINT>(ToolbarImage),
			(LPCTBBUTTON)&tbBAButtons,
			9,
			MulDiv(16, DPIX, 96),MulDiv(16, DPIY, 96),
			MulDiv(16, DPIX, 96), MulDiv(16, DPIY, 96),
			sizeof(TBBUTTON));
		check(hWndToolBar);

		mrulist = new MRUList(*PersistentName);
		mrulist->ReadINI();

		ToolTipCtrl = new WToolTip(this);
		ToolTipCtrl->OpenWindow();
		for( int tooltip = 0 ; ToolTips_BA[tooltip].ID > 0 ; tooltip++ )
		{
			// Figure out the rectangle for the toolbar button.
			int index = SendMessageW( hWndToolBar, TB_COMMANDTOINDEX, ToolTips_BA[tooltip].ID, 0 );
			RECT rect;
			SendMessageW( hWndToolBar, TB_GETITEMRECT, index, (LPARAM)&rect);

			ToolTipCtrl->AddTool( hWndToolBar, ToolTips_BA[tooltip].ToolTip, tooltip, &rect );
		}

		PositionChildControls();
		RefreshPackages();
		RefreshActorList();
		SendMessageW( pTreeView->hWnd, TVM_EXPAND, TVE_EXPAND, (LPARAM)htiRoot );
		UpdateMenu();

		SetRedraw(true);

		unguard;
	}
	virtual void UpdateMenu()
	{
		guard(WBrowserActor::UpdateMenu);

		HMENU menu = GetMyMenu();
		if (!menu)
			return;

		CheckMenuItem( menu, IDMN_AB_SHOWPACKAGES, MF_BYCOMMAND | (bShowPackages ? MF_CHECKED : MF_UNCHECKED) );

		unguard;
	}
	void RefreshPackages(void)
	{
		guard(WBrowserActor::RefreshPackages);

		SetRedraw(false);

		// PACKAGES
		//
		FStringOutputDevice GetPropResult = FStringOutputDevice();
		GEditor->Get(TEXT("OBJ"), TEXT("PACKAGES CLASS=Class"), GetPropResult);

		TArray<FString> Selected;
		for( int x = 0 ; x < pPackagesList->GetCount() ; x++ )
			if( pPackagesList->GetItemData(x) )
				Selected.AddItem(pPackagesList->GetString( x ));

		TArray<FString> Previous;
		for( int x = 0 ; x < pPackagesList->GetCount() ; x++ )
			Previous.AddItem(pPackagesList->GetString( x ));

		TArray<FString> PkgArray;
		ParseStringToArray( TEXT(","), *GetPropResult, &PkgArray );

		FString Current = pPackagesList->GetString(pPackagesList->GetCurrent());
		FString Top = pPackagesList->GetString(pPackagesList->GetTop());

		pPackagesList->Empty();

		for( int x = 0 ; x < PkgArray.Num() ; x++ )
			pPackagesList->AddString(*PkgArray(x));

		if (pPackagesList->FindItem(TEXT("MyLevel")) == -1)
			pPackagesList->AddString(TEXT("MyLevel"));

		for( int x = 0 ; x < Selected.Num() ; x++ )
		{
			int i = pPackagesList->FindItem(*Selected(x));
			if (i != -1)
				pPackagesList->SetItemData(i, 1);
		}

		if (pPackagesList->GetCount() > Previous.Num())
		{
			int i;
			for (TObjectIterator<UClass> It; It; ++It)
				if ((It->ClassFlags & CLASS_EditorExpanded) && !Previous.FindItem(*It->GetOuterUPackage()->GetFName(), i))
					It->ClassFlags &= ~CLASS_EditorExpanded;
		}

		pPackagesList->SetCurrent(pPackagesList->FindItem(*Current), false);
		pPackagesList->SetTop(pPackagesList->FindItem(*Top));

		SetRedraw(true);

		unguard;
	}
	void OnDestroy()
	{
		guard(WBrowserActor::OnDestroy);

		delete pTreeView;
		delete pObjectsCheck;
		delete pPackagesList;

		GConfig->SetInt( *PersistentName, TEXT("ShowPackages"), bShowPackages, GUEDIni );

		WBrowser::OnDestroy();
		unguard;
	}
	UClass* GetRootClass(void)
	{
		guard(WBrowserActor::GetRootClass);
		return pObjectsCheck->IsChecked() ? AActor::StaticClass() : UObject::StaticClass();
		unguard;
	}
	void RemoveClass(UClass* Class)
	{
		guard(WBrowserActor::FindClass);
		HTREEITEM NewItem = FindClass(Class);
		if (NewItem)
			SendMessage(pTreeView->hWnd, TVM_DELETEITEM, 0, (LPARAM)NewItem);
		unguard;
	}
	HTREEITEM FindClass(UClass* Class, BOOL bWithExpand = TRUE)
	{
		guard(WBrowserActor::FindClass);
		UClass* OuterMost = GetRootClass();
		if (!Class->IsChildOf(OuterMost))
			return NULL;
		if (!(OuterMost->ClassFlags & CLASS_EditorExpanded))
		{
			if (!bWithExpand)
				return NULL;
			SendMessage(pTreeView->hWnd, TVM_EXPAND, TVE_EXPAND, (LPARAM)htiRoot);
		}
		FString PathTo = TEXT("");
		UBOOL bDummy = 0;
		HTREEITEM htiTravel = htiRoot;

		while (OuterMost != Class && OuterMost)
		{
			UClass* Find = Class;
			while (Find && Find->GetSuperClass() != OuterMost)
				Find = Find->GetSuperClass();
			if (!Find)
				return NULL;
			if (Find != Class && !(Find->ClassFlags & CLASS_EditorExpanded))
			{
				if (!bWithExpand)
					return NULL;
				HTREEITEM NewItem = pTreeView->FindItem(pTreeView->hWnd, htiTravel, *GetClassDisplayName(Find, bDummy));
				if (NewItem)
				{
					htiTravel = NewItem;
					SendMessage(pTreeView->hWnd, TVM_EXPAND, TVE_EXPAND, (LPARAM)NewItem);
				}
				PathTo += FObjectName(Find);
				PathTo += ' ';
			}
			OuterMost = Find;
		}

		if (OuterMost)
		{
			debugf(TEXT("%ls found in: %ls"), *FObjectFullName(OuterMost), PathTo.Len() > 0 ? *PathTo : TEXT("Root"));
			return pTreeView->FindItem(pTreeView->hWnd, htiTravel, *GetClassDisplayName(OuterMost, bDummy));
		}
		debugf(TEXT("Class not found"));
		return NULL;
		unguard;
	}
	void OnCommand( INT Command )
	{
		guard(WBrowserActor::OnCommand);
		switch( Command )
		{
			case WM_TREEVIEW_RIGHT_CLICK:
			{
				POINT ptScreen;
				::GetCursorPos( &ptScreen );
				HWND Target = WindowFromPoint(ptScreen);
				if (Target == pTreeView->hWnd)
				{
					// Select the tree item underneath the mouse cursor.
					TVHITTESTINFO tvhti;
					
					tvhti.pt = ptScreen;
					::ScreenToClient( pTreeView->hWnd, &tvhti.pt );

					SendMessageW( pTreeView->hWnd, TVM_HITTEST, 0, (LPARAM)&tvhti);

					if( tvhti.hItem )
						SendMessageW( pTreeView->hWnd, TVM_SELECTITEM, TVGN_CARET, (LPARAM)(HTREEITEM)tvhti.hItem);

					// Show a context menu for the currently selected item.
					HMENU OuterMenu = AddHotkeys(LoadMenuW(hInstance, MAKEINTRESOURCEW(IDMENU_BrowserActor_Context)));
					HMENU menu = GetSubMenu(OuterMenu, 0);
					TrackPopupMenu( menu,
						TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
						ptScreen.x, ptScreen.y, 0,
						hWnd, NULL);
					DestroyMenu(OuterMenu);
				}
				else if (Target == pPackagesList->hWnd)
				{
					INT Selected = pPackagesList->GetCurrent();
					if (Selected >= 0)
					{
						HMENU OuterMenu = AddHotkeys(LoadMenuW(hInstance, MAKEINTRESOURCEW(IDMENU_BrowserActor_Package)));
						HMENU menu = GetSubMenu(OuterMenu, 0);
						TrackPopupMenu( menu,
							TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
							ptScreen.x, ptScreen.y, 0,
							hWnd, NULL);
						DestroyMenu(OuterMenu);
					}
				}
			}
			break;

			case IDMN_AB_EXPORT_ALL:
			{
				if( ::MessageBox( hWnd, TEXT("This option will export all classes to text .uc files which can later be rebuilt. Do you want to do this?"), TEXT("Export classes to *.uc files"), MB_YESNO) == IDYES)
				{
					ExportScripts(ET_ALL);
				}
			}
			break;

			case IDMN_AB_EXPORT:
			{
				if( ::MessageBox( hWnd, TEXT("This option will export all modified classes to text .uc files which can later be rebuilt. Do you want to do this?"), TEXT("Export classes to *.uc files"), MB_YESNO) == IDYES)
				{
					ExportScripts(ET_CHANGED);
				}
			}
			break;

			case IDMN_AB_EXPORT_Selected:
			{
				if (::MessageBox(hWnd, TEXT("This option will export all selected packages to text .uc files which can later be rebuilt. Do you want to do this?"), TEXT("Export packages to *.uc files"), MB_YESNO) == IDYES)
				{
					ExportScripts(ET_SELECTED);
				}
			}
			break;

			case IDMN_LOAD_ENTIRE:
			{
				FString Pkg;
				for (int x = 0; x < pPackagesList->GetCount(); x++)
				{
					if (pPackagesList->GetItemData(x))
					{
						Pkg = *(pPackagesList->GetString(x));
						UObject::LoadPackage(NULL, *Pkg, LOAD_NoWarn);
					}
				}
				GBrowserMaster->RefreshAll();
			}
			break;

			case IDMN_AB_SHOWPACKAGES:
			{
				bShowPackages = !bShowPackages;
				PositionChildControls();
				UpdateMenu();
			}
			break;

			case IDMN_AB_LOAD_FILE_INTO:
			{
				INT Selected = pPackagesList->GetCurrent();
				if (Selected > 0)
				{
					FString Package = pPackagesList->GetString(Selected);
					
					if (!GLastDir[eLASTDIR_CLS].Len())
						GLastDir[eLASTDIR_CLS] = TEXT("..\\System");
					TArray<FString> Files;
					if (OpenFilesWithDialog(
						*GLastDir[eLASTDIR_CLS],
						TEXT("All Packages (*.u, *.uax, *.umx, *.utx)\0*.u;*.uax;*.umx;*.utx\0Class Packages (*.u)\0*.u\0Sound Packages (*.uax)\0*.uax\0Music Packages (*.umx)\0*.umx\0Texture Packages (*.utx)\0*.utx\0All Files\0*.*\0\0"),
						TEXT("u"),						
						*FString::Printf(TEXT("Load File Into %ls Package"), *Package),
						Files) && Files.Num() > 0)
					{
						TArray<FString> Previous;
						for (TObjectIterator<UClass> It; It; ++It)
							if ((It->ClassFlags & CLASS_EditorExpanded))
								Previous.AddItem(FObjectFullName(*It));
						
						for( int x = 0 ; x < Files.Num() ; x++ )
							GEditor->Exec(*FString::Printf(TEXT("OBJ LOAD FILE=\"%ls\" PACKAGE=\"%ls\""), *(Files(x)), *Package));

						int i;
						for (TObjectIterator<UClass> It; It; ++It)
							if ((It->ClassFlags & CLASS_EditorExpanded) && !Previous.FindItem(FObjectFullName(*It), i))
								It->ClassFlags &= ~CLASS_EditorExpanded;

						GLastDir[eLASTDIR_CLS] = appFilePathName(*(Files(0)));

						GBrowserMaster->RefreshAll();
						RefreshPackages();
					}

					GFileManager->SetDefaultDirectory(appBaseDir());
				}
			}
			break;

			case IDMN_AB_SAVE_PACKAGE_AS:
			{
				INT Selected = pPackagesList->GetCurrent();
				if (Selected > 0)
				{
					FString Package = pPackagesList->GetString(Selected);
					SavePackageAs(*Package, *Package);
				}
			}
			break;

			case IDMN_AB_OPEN_ALL_CLASSES:
			{
				OnPackagesListDblClk();
			}
			break;

			case IDMN_AB_DUMP_INT:
			{
				INT Selected = pPackagesList->GetCurrent();
				if (Selected > 0)
				{
					FString Package = pPackagesList->GetString(Selected);
					if (Package == TEXT("MyLevel"))
						Package = ((WWindow *)GLevelFrame)->GetText(); // hack for get filename without know layout of class WLevelFrame
					INT MessageID = ::MessageBox(hWnd, *FString::Printf(TEXT("Dump .int file for %ls? This will overwrite the current .int file of that package."),
						*Package), TEXT("UnrealEd"), MB_YESNO);
					if (MessageID == IDYES)
					{
						FString EmulatedCmdLine = FString::Printf(TEXT("%ls"), *Package);
						UClass* Class = UDumpIntCommandlet::StaticClass();
						GIsCommandlet = 1;
						UCommandlet* Commandlet = ConstructObject<UCommandlet>( Class );
						Commandlet->InitExecution();
						Commandlet->ParseParms(*EmulatedCmdLine);
						Commandlet->Main(*EmulatedCmdLine);
						GIsCommandlet = 0;
						GIsRequestingExit = 0; // prevent exit from editor
					}
				}
			}
			break;

			case ID_FileNew:
			{
				WDlgNewClass l_dlg( NULL, this );
				if( l_dlg.DoModal( GEditor->CurrentClass ? *FObjectPathName(GEditor->CurrentClass) : TEXT("Engine.Actor") ) )
				{
					if (l_dlg.ReplacedClass)
					{
						GCodeFrame->RemoveClass(FObjectPathName(l_dlg.ReplacedClass));
						RemoveClass(l_dlg.ReplacedClass);
					}

					// Create new class.
					//
					GEditor->Exec( *(FString::Printf( TEXT("CLASS NEW NAME=\"%ls\" PACKAGE=\"%ls\" PARENT=\"%ls\""),
						*l_dlg.Name, *l_dlg.Package, *l_dlg.ParentClass)) );
					FString FullName = FString::Printf(TEXT("%ls.%ls"), *l_dlg.Package, *l_dlg.Name);
					GEditor->Exec( *(FString::Printf(TEXT("SETCURRENTCLASS CLASS=\"%ls\""), *FullName)) );

					if (GEditor->CurrentClass)
					{
						// Create standard header for the new class.
						//
						const TCHAR* NewLine = TEXT("\x0d\x0a");
						FString S = FString::Printf(
							TEXT("//=============================================================================%ls// %ls.%ls//=============================================================================%lsclass %ls expands %ls;%ls"),
							NewLine,
							*l_dlg.Name, NewLine,
							NewLine,
							*l_dlg.Name, *l_dlg.ParentClass.Mid(l_dlg.ParentClass.InStr(TEXT(".")) + 1), NewLine);
						GEditor->Set(TEXT("SCRIPT"), *FullName, *S);

						// compile new class
						GEditor->MakeScripts(GWarn, FALSE, FALSE, GEditor->CurrentClass);

						UBOOL bDummy = 0;
						if (!(GEditor->CurrentClass->GetSuperClass()->ClassFlags & CLASS_EditorExpanded))
							GEditor->CurrentClass->GetSuperClass()->ClassFlags |= CLASS_EditorExpanded;
						else if (!pTreeView->AddItem(*GetClassDisplayName(GEditor->CurrentClass, bDummy), htiLastSel, FALSE))
							OnTreeViewItemExpanding();
						RefreshActorList();
						SendMessage(pTreeView->hWnd, TVM_EXPAND, TVE_EXPAND, (LPARAM)(HTREEITEM)htiLastSel);
						HTREEITEM NewItem = pTreeView->FindItem(pTreeView->hWnd, htiLastSel, *GetClassDisplayName(GEditor->CurrentClass, bDummy));
						if (NewItem)
							SendMessage(pTreeView->hWnd, TVM_SELECTITEM, TVGN_CARET, (LPARAM)NewItem);
						else debugf(TEXT("Item not found!"));

						// Open an editing window.
						//
						GCodeFrame->AddClass( GEditor->CurrentClass );
						RefreshPackages();
					}
				}
			}
			break;

			case IDMN_AB_SELECT_OFCLASS:
			case IDMN_AB_SELECT_OFSUBCLASS:
			{
				if (!GEditor->CurrentClass)
					break;
				GEditor->Exec(*FString::Printf(TEXT("ACTOR SELECT OF%lsCLASS CLASS=%ls"), Command == IDMN_AB_SELECT_OFSUBCLASS ? TEXT("SUB") : TEXT(""), *FObjectPathName(GEditor->CurrentClass)));
			}
			break;

			case ID_EditDelete:
			{
				if (!GEditor->CurrentClass)
					break;
				UClass* Obj = GEditor->CurrentClass;
				GEditor->CurrentClass = NULL;
				FString CurName = FObjectPathName(Obj);
				FString Reason = FString::Printf(TEXT("Remove Class: %ls"), *CurName);
				TArray<UObject*> Res;
				FCheckObjRefArc Ref(&Res, Obj, false);
				if (Ref.HasReferences())
				{
					Res.Empty();

					// The undo buffer might still have a reference to this object.
					// We should clear the buffer and run the garbage collector to 
					// ensure the full and safe deletion of this actor.
					GEditor->Trans->Reset(*Reason);
					GEditor->PostDeleteActors();
					GEditor->Cleanse(FALSE, *Reason); // call GC

					Ref = FCheckObjRefArc(&Res, Obj, false);
				}

				if (Ref.HasReferences())
				{
					GEditor->CurrentClass = Obj;
					FString RefStr = FObjectFullName(Res(0));
					for (INT i = 1; i < Min<INT>(4, Res.Num()); i++)
					{
						RefStr += FString::Printf(TEXT(", %ls"), *FObjectFullName(Res(i)));
						if (i == 3 && Res.Num() > 4)
							RefStr += TEXT(", etc...");
					}
					appMsgf(TEXT("Class %ls is still being referenced by: %ls"), *FObjectName(Obj), *RefStr);
				}
				else
				{
					GEditor->Exec(TEXT("SETCURRENTCLASS Class=Engine.Light"));

					// Try to cleanly update the actor list.  If this fails, just reload it from scratch...
					if (!SendMessage(pTreeView->hWnd, TVM_DELETEITEM, 0, (LPARAM)htiLastSel))
						RefreshActorList();
					GCodeFrame->RemoveClass(CurName);
					Obj->RemoveFromRoot();
					Obj->ClearFlags(RF_Native | RF_Standalone); // Makes sure we can GC the object
					GEditor->Cleanse(FALSE, *Reason); // call GC
					debugf(TEXT("Removed: %ls"), *CurName);
				}
			}
			break;

			case ID_EditRename:
			{
				UClass* Cls = GEditor->CurrentClass;
				if (!Cls)
					break;
				if (Cls->GetFlags() & RF_Native)
					appMsgf(TEXT("Cannot rename native class %ls"), *FObjectPathName(Cls));
				else
				{
					WDlgRename dlg(NULL, this);
					if (dlg.DoModal(*FObjectName(Cls)))
					{
						Cls->Rename(*dlg.NewName);
						GCodeFrame->RefreshScripts(TRUE);
					}
					GBrowserMaster->RefreshAll();
					RefreshActorList();
				}
			}
			break;

			case IDMN_AB_DEF_PROP:
				GEditor->Exec( *(FString::Printf(TEXT("HOOK CLASSPROPERTIES CLASS=\"%ls\""), *FObjectPathName(GEditor->CurrentClass))) );
				break;

			case IDMN_AB_DEF_PROPCOPY:
				if (GEditor->CurrentClass)
					CopyDefPropsToClipboard(GEditor->CurrentClass);
				break;

			case IDMN_AB_DEF_PROPPAST:
				if (GEditor->CurrentClass)
				{
					INT MessageID = ::MessageBox(hWnd, *FString::Printf(TEXT("Paste the defaultproperties from clipboard:\r\nonly into %ls ? (Yes)\r\n")
						TEXT("Or paste them in all child classes as well? (No)"),
						*FObjectFullName(GEditor->CurrentClass)), TEXT("UnrealEd"), MB_YESNOCANCEL);
					if (MessageID == IDYES)
						CopyDefPropsFromClipboard(GEditor->CurrentClass);
					else if (MessageID == IDNO)
						for (TObjectIterator<UClass> It; It; ++It)
							if (It->IsChildOf(GEditor->CurrentClass))
								CopyDefPropsFromClipboard(*It);
				}
				break;

			case IDMN_AB_SAVE_CONFIG:
				if (GEditor->CurrentClass)
				{
					INT MessageID = ::MessageBox(hWnd, *FString::Printf(TEXT("Save config for %ls? This will overwrite the current config of that class."),
						*FObjectFullName(GEditor->CurrentClass)), TEXT("UnrealEd"), MB_YESNO);
					if (MessageID == IDYES)
						GEditor->CurrentClass->GetDefaultObject()->SaveConfig();
				}
				break;			

			case ID_FileOpenProtected:
			case ID_FileOpen:
			{
				TArray<FString> Files;
				if (OpenFilesWithDialog(
					TEXT("..\\System"),
					TEXT("Class Packages (*.u)\0*.u\0All Files\0*.*\0\0"),
					TEXT("u"),						
					TEXT("Open Class Package"),
					Files))					
				{
					const TCHAR* Protected = Command == ID_FileOpenProtected ? TEXT(" PROTECTED") : TEXT("");
					for( int x = 0 ; x < Files.Num() ; x++ )
					{
						GEditor->Exec(*FString::Printf(TEXT("CLASS LOAD%ls FILE=\"%ls\""), Protected, *Files(x)));

						mrulist->AddItem(*(Files(x)));
						mrulist->WriteINI();
						GConfig->Flush(0);
						UpdateMenu();
					}

					GBrowserMaster->RefreshAll();
					RefreshPackages();
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
				GEditor->Exec(*FString::Printf(TEXT("CLASS LOAD FILE=\"%ls\""), *Filename));

				GBrowserMaster->RefreshAll();
				RefreshPackages();
			}
			break;

			case ID_FileSave:
			{
				INT iSelected = 0;
				for (INT x = 0; x < pPackagesList->GetCount(); x++)
					if (pPackagesList->GetItemData(x))
						iSelected++;
				if (iSelected == 0) {
					appMsgf(TEXT("Nothing to save. You need to select at least one package from the package list first. %ls"),
						bShowPackages ? TEXT("") : TEXT("\n\nUse the 'View -> Show Packages' menu to display the package list."));
					break;
				}

				GCodeFrame->Save(); // save current script if any

				INT iFailed = 0;
				FString Pkg, Failed;
				GWarn->BeginSlowTask( TEXT("Saving Packages"), 1, 0 );
				for( int x = 0 ; x < pPackagesList->GetCount() ; x++ )
				{
					// stijn: for some reason, the item data is set to 0 when the package is selected...
					if( pPackagesList->GetItemData(x) )
					{
						Pkg = *(pPackagesList->GetString( x ));
						if (GRecoveryMode)
							SavePackageAs(Pkg, Pkg);
						else if(!GEditor->Exec(*(FString::Printf(TEXT("OBJ SAVEPACKAGE PACKAGE=\"%ls\" FILE=\"%ls.u\""), *Pkg, *Pkg))))
						{
							Failed += Pkg + TEXT(", ");
							iFailed++;
						}
						else
						{
							FString File = FString::Printf(TEXT("%ls%ls.u"), appBaseDir(), *Pkg);
							mrulist->AddItem(*File);
							mrulist->WriteINI();
							GConfig->Flush(0);
							UpdateMenu();
						}
					}
				}
				GWarn->EndSlowTask();
				CheckSavePackage(iFailed, *Failed.Mid(0, Failed.Len() - 2));
			}
			break;

			case IDMN_AB_FileMyLevelSaveAs:
			{
				FString Name = appFileBaseName(*((WWindow*)GLevelFrame)->GetText());
				if( Name.InStr( TEXT(".") ) != -1 )
					Name = Name.Left( Name.InStr( TEXT("."), true ) );
				SavePackageAs(TEXT("MyLevel"), Name);
			}
			break;

			case IDMN_AB_EDIT_SCRIPT:
			{
				GCodeFrame->AddClass( GEditor->CurrentClass );
			}
			break;

			case ID_EditFind:
			{
				PostMessage(hWnd, WM_COMMAND, IDMN_AB_FIND_CLASS, 0);
			}
			break;
			case IDMN_AB_FIND_CLASS:
			{
				WDlgFindClass l_dlg(NULL, this);
				if (l_dlg.DoModal() == IDOK)
					SendMessage(hWnd, WM_COMMAND, IDMN_AB_SELECT_CLASS, 0);
			}
			break;
			case IDMN_AB_SELECT_CLASS:
			{
				SelectClass(GEditor->CurrentClass);
			}
			break;

			case IDPB_EDIT:
			{
				OnTreeViewDblClk();
			}
			break;

			default:
				WBrowser::OnCommand(Command);
				break;
		}
		unguard;
	}
	void SavePackageAs(FString Package, FString Name)
	{
		GCodeFrame->Save(); // save current script if any
					
		if (!GLastDir[eLASTDIR_CLS].Len())
			GLastDir[eLASTDIR_CLS] = TEXT("..\\System");
		FString File;
		if (GetSaveNameWithDialog(
			*FString::Printf(TEXT("%ls.u"), *Name),
			*GLastDir[eLASTDIR_CLS],
			TEXT("Class Packages (*.u)\0*.u\0All Files\0*.*\0\0"),
			TEXT("u"),
			*FString::Printf(TEXT("Save %ls Package"), *Package),
			File			
		) && AllowOverwrite(*File)){
			CheckSavePackage(!GEditor->Exec(*FString::Printf(
				TEXT("OBJ SAVEPACKAGE PACKAGE=\"%ls\" FILE=\"%ls\""), *Package, *File)), Package);

			GLastDir[eLASTDIR_CLS] = appFilePathName(*File);
		}

		GFileManager->SetDefaultDirectory(appBaseDir());
	}
	void SelectClass(UClass* Class)
	{
		guard(WBrowserActor::SelectClass);
		if (Class)
		{
			if (pObjectsCheck->IsChecked() && !Class->IsChildOf(AActor::StaticClass()))
			{
				SendMessage(pObjectsCheck->hWnd, WM_LBUTTONDOWN, 0, 0);
				SendMessage(pObjectsCheck->hWnd, WM_LBUTTONUP, 0, 0);
			}

			HTREEITEM NewItem = FindClass(Class);
			if (NewItem)
			{
				TreeView_SelectItem(pTreeView->hWnd, NewItem);
				TreeView_SelectSetFirstVisible(pTreeView->hWnd, NewItem);
				SetFocus(pTreeView->hWnd);
			}
		}
		unguard;
	}
	void ExportScripts(EExportType Type)
	{
		guard(WBrowserActor::ExportScripts);

		GCodeFrame->Save(); // save current script if any

		GWarn->BeginSlowTask( TEXT("Exporting scripts"), 0, 0 );
		TArray<UObject*> Pkgs;
		if (Type == ET_SELECTED)
			for (int x = 0; x < pPackagesList->GetCount(); x++)
			{
				if (pPackagesList->GetItemData(x))
				{
					UPackage* Pkg = FindObject<UPackage>(NULL, *pPackagesList->GetString(x));
					if (Pkg)
						Pkgs.AddItem(Pkg);
				}
			}		
		TArray<UObject*> CreatedDirs;
		for (TObjectIterator<UClass> It; It; ++It)
		{
			if (!It->ScriptText)
				continue;
			if (Type == ET_SELECTED && Pkgs.FindItemIndex(It->GetOuter()) < 0)
				continue;
			if (Type == ET_CHANGED && !(It->GetFlags() & RF_SourceModified))
				continue;

			FString Path = FString::Printf(TEXT("..") PATH_SEPARATOR TEXT("%ls") PATH_SEPARATOR TEXT("Classes"), It->GetOuter()->GetName());

			if (CreatedDirs.FindItemIndex(It->GetOuter()) < 0)
			{
				CreatedDirs.AddItem(It->GetOuter());
				GFileManager->MakeDirectory(*Path, TRUE);
			}			
			
			Path *= FString::Printf(TEXT("%ls.uc"), It->GetName());
			
			Spew(*It, Path);
		}
		GWarn->EndSlowTask();
		unguard;
	}
	void Spew(UClass* Cls, FString Filename)
	{
		guard(WBrowserActor::Spew);
		debugf(NAME_Log, TEXT("Spewing: %ls"), *Filename);
		FString Script = Cls->ScriptText->Text;
		INT Start = 0, End = Script.Len();
		while (Start < End && (Script[Start] == '\r' || Script[Start] == '\n' || Script[Start] == ' '))
			Start++;
		while (End > Start && (Script[End - 1] == '\r' || Script[End - 1] == '\n' || Script[End - 1] == ' ') )
			End--;
		if (Start != 0 || End != Script.Len())
			Script = Script.Mid(Start, End);
		Script += TEXT("\r\n\r\n");
		Script += GCodeFrame->ExportDefPropsToFString(Cls);
		Script += TEXT("\r\n");
		appSaveStringToFile(Script, *Filename);
		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WBrowserActor::OnSize);
		WBrowser::OnSize(Flags, NewX, NewY);
		PositionChildControls();
		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void PositionChildControls( void )
	{
		guard(WBrowserActor::PositionChildControls);

		if( !pTreeView
				|| !pObjectsCheck
				|| !pPackagesList ) return;

		FRect CR = GetClientRect();

		::MoveWindow(hWndToolBar, 4, 0, 8 * MulDiv(16, DPIX, 96), 8 * MulDiv(16, DPIY, 96), 1);

		RECT R;
		::GetClientRect( hWndToolBar, &R );
		float Fraction = (CR.Width() - 8) / 10.0f;
		float Top = 4 + R.bottom;

		
		::MoveWindow( pObjectsCheck->hWnd, 4, Top, Min((float)MulDiv(150, DPIX, 96), Fraction * 10), MulDiv(20, DPIY, 96), 1 );		
		Top += MulDiv(20, DPIY, 96);
		if( bShowPackages )
		{
			::MoveWindow( pTreeView->hWnd, 4, Top, CR.Width() - 8, ((CR.Height() / 3) * 2) - Top, 1 );	
			Top += ((CR.Height() / 3) * 2) - Top;
			::MoveWindow( pPackagesList->hWnd, 4, Top, CR.Width() - 8, (CR.Height() / 3) - 4, 1);
			debugf(TEXT("%1.2f %1.2f %1.2f %1.2f"),
				(float)4,
				(float)(Top),
				(float)(CR.Width() - 8),
				(float)((CR.Height() / 3) - 4 ));
		}
		else
		{
			::MoveWindow( pTreeView->hWnd, 4, Top, CR.Width() - 8, CR.Height() - Top - 4, 1 );
			::MoveWindow( pPackagesList->hWnd, 0, 0, 0, 0, 1);
		}

		::InvalidateRect( hWnd, NULL, 1);

		unguard;
	}
	virtual void RefreshAll()
	{
		guard(WBrowserActor::RefreshAll);
		SetRedraw(FALSE);
		RefreshPackages();
		RefreshActorList();
		UpdateMenu();
		SetRedraw(TRUE);
		unguard;
	}
	void RefreshActorList(void)
	{
		guard(WBrowserActor::RefreshActorList);
		UClass* FirstVisible = NULL;
		HTREEITEM htiFirstVisible = TreeView_GetFirstVisible(pTreeView->hWnd);
		if (htiFirstVisible)
		{
			TCHAR chText[1024] = TEXT("\0");
			TVITEM tvi;
			appMemzero( &tvi, sizeof(tvi));
			tvi.hItem = htiFirstVisible;
			tvi.mask = TVIF_TEXT;
			tvi.pszText = chText;
			tvi.cchTextMax = sizeof(chText);

			if (SendMessageW(pTreeView->hWnd, TVM_GETITEM, 0, (LPARAM)& tvi))
				FirstVisible = FindClassFromDisplayName(tvi.pszText);
		}
		pTreeView->SetRedraw(0);
		pTreeView->Empty();
		htiRoot = pTreeView->AddItem(pObjectsCheck->IsChecked() ? TEXT("Actor*") : TEXT("Object*"), NULL, TRUE);
		htiLastSel = htiFirstVisible = NULL;
		UClass* RootClass = GetRootClass();
		RootClass->ClassFlags |= CLASS_EditorExpanded;

		TArray<UClass*> Collapse;

		for (TObjectIterator<UClass> It; It; ++It)
			if (It->ClassFlags & CLASS_EditorExpanded)
			{	
				if (!It->IsChildOf(RootClass))
					It->ClassFlags &= ~CLASS_EditorExpanded;
				else
					for (UClass* Parent = *It; Parent && Parent != RootClass; Parent = Parent->GetSuperClass())
					{
						if (!(Parent->ClassFlags & CLASS_EditorExpanded))
							Collapse.AddItem(Parent);
						Parent->ClassFlags |= CLASS_EditorExpanded;
					}
			}
		
		ReopenClass(RootClass, htiRoot, FirstVisible, &htiFirstVisible);

		for (INT i = 0; i < Collapse.Num(); i++)
		{
			HTREEITEM hti = FindClass(Collapse(i), FALSE);
			if (hti)
			{
				Collapse(i)->ClassFlags &= ~CLASS_EditorExpanded;
				SendMessage(pTreeView->hWnd, TVM_EXPAND, TVE_COLLAPSE, (LPARAM)hti);
			}
		}

		if (htiLastSel)
			TreeView_SelectItem(pTreeView->hWnd, htiLastSel);
		if (htiFirstVisible)
			TreeView_SelectSetFirstVisible(pTreeView->hWnd, htiFirstVisible);
		pTreeView->SetRedraw(1);
		unguard;
	}
	void ReopenClass(UClass* Class, HTREEITEM hti, UClass* FirstVisible, HTREEITEM* htiFirstVisible)
	{
		guard(WBrowserActor::ReopenClass);
		FString DisplayName;
		for (TObjectIterator<UClass> It; It; ++It)
		{
			UClass* C = *It;
			if (C->GetSuperClass() == Class)
			{
				UBOOL bChildren = 0;
				DisplayName = GetClassDisplayName(C, bChildren);
				HTREEITEM htiChild = pTreeView->AddItem(*DisplayName, hti, bChildren);
				if (C->ClassFlags & CLASS_EditorExpanded)
				{
					if (bChildren)
						ReopenClass(C, htiChild, FirstVisible, htiFirstVisible);
					else
						C->ClassFlags &= ~CLASS_EditorExpanded; // Reset flag.
				}
				if (C == GEditor->CurrentClass)
					htiLastSel = htiChild;
				if (C == FirstVisible)
					*htiFirstVisible = htiChild;
			}
		}
		SendMessage(pTreeView->hWnd, TVM_EXPAND, TVE_EXPAND, (LPARAM)hti);
		unguard;
	}
	void AddChildren(const TCHAR* pParentName, HTREEITEM hti, UBOOL Expand)
	{
		guard(WBrowserActor::AddChildren);
		UClass* ParentClass = FindClassFromDisplayName(pParentName);
		if (!ParentClass)
		{
			pTreeView->AddItem(TEXT("None"), hti, FALSE);
			return;
		}
		ParentClass->ClassFlags |= CLASS_EditorExpanded;

		FString DisplayName;
		for (TObjectIterator<UClass> It; It; ++It)
		{
			UClass* C = *It;
			if (C->GetSuperClass() == ParentClass)
			{
				UBOOL bChildren = 0;
				DisplayName = GetClassDisplayName(C, bChildren);
				pTreeView->AddItem(*DisplayName, hti, bChildren);
			}
		}
		unguard;
	}
	void OnTreeViewSelChanged( void )
	{
		guard(WBrowserActor::OnTreeViewSelChanged);
		TCHAR chText[1024] = TEXT("\0");
		TVITEM tvi;

		appMemzero( &tvi, sizeof(tvi));
		tvi.hItem = TreeView_GetSelection(pTreeView->hWnd);
		if (!tvi.hItem)
			return;
		htiLastSel = tvi.hItem;
		tvi.mask = TVIF_TEXT;
		tvi.pszText = chText;
		tvi.cchTextMax = sizeof(chText);

		if (SendMessageW(pTreeView->hWnd, TVM_GETITEM, 0, (LPARAM)& tvi))
		{
			UClass* CurClass = FindClassFromDisplayName(tvi.pszText);
			FString Cmd = FString::Printf(TEXT("SETCURRENTCLASS CLASS=\"%ls\""),
				(CurClass ? *FObjectPathName(CurClass) : TEXT("Engine.Light")));
			GEditor->Exec(*Cmd);
		}
		SetCaption();
		unguard;
	}
	void OnTreeViewItemExpanding( void )
	{
		guard(WBrowserActor::OnTreeViewItemExpanding);
		NMTREEVIEW* pnmtv = (LPNMTREEVIEW)pTreeView->LastlParam;
		TCHAR chText[1024] = TEXT("\0");

		TVITEM tvi;

		appMemzero( &tvi, sizeof(tvi));
		tvi.hItem = pnmtv->itemNew.hItem;
		tvi.mask = TVIF_TEXT;
		tvi.pszText = chText;
		tvi.cchTextMax = sizeof(chText);

		if (pnmtv->action == TVE_COLLAPSE)
		{
			if (SendMessageW(pTreeView->hWnd, TVM_GETITEM, 0, (LPARAM)& tvi))
			{
				UClass* Parent = FindClassFromDisplayName(tvi.pszText);	
				if (Parent)
					Parent->ClassFlags &= ~CLASS_EditorExpanded;
			}
			return;
		}

		// If this item already has children loaded, bail...
		if( SendMessageW( pTreeView->hWnd, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)pnmtv->itemNew.hItem ) )
			return;

		if (SendMessageW(pTreeView->hWnd, TVM_GETITEM, 0, (LPARAM)& tvi))
			AddChildren(tvi.pszText, pnmtv->itemNew.hItem, 0);
		unguard;
	}
	void OnPackagesListDblClk( void )
	{
		guard(WBrowserActor::OnPackagesListDblClk);
		FString Current = pPackagesList->GetString(pPackagesList->GetCurrent());
		GCodeFrame->AddAllClasses(Current);
		unguard;
	}
	void OnTreeViewDblClk( void )
	{
		guard(WBrowserActor::OnTreeViewDblClk);
		GCodeFrame->AddClass( GEditor->CurrentClass );
		PostMessage(NULL, WM_SETFOCUS_DELAYED, (WPARAM)GCodeFrame->Edit.hWnd, 0);
		unguard;
	}
	void OnObjectsClick()
	{
		guard(WBrowserActor::OnObjectsClick);
		RefreshActorList();
		SetFocus(pTreeView->hWnd);
		unguard;
	}
	UClass* FindClassFromDisplayName(const TCHAR* N) const
	{
		guard(WBrowserActor::FindClassFromDisplayName);
		if (!appStricmp(N, TEXT("None")))
			return NULL;

		FString FullName = N;

		// Remove the non-placeable marker
		const INT StarPos = FullName.InStr(TEXT("*"));
		if (StarPos != -1)
			FullName = FullName.Left(StarPos) + FullName.Mid(StarPos + 1);

		// Parse the package name
		const INT OpenParenPos = FullName.InStr(TEXT("("));
		const INT CloseParenPos = FullName.InStr(TEXT(")"));
		if (OpenParenPos != -1 && CloseParenPos != -1 && CloseParenPos > OpenParenPos)
		{
			FullName = (FullName.Mid(OpenParenPos + 1, CloseParenPos - OpenParenPos - 1) += '.') += *FullName.Left(OpenParenPos);
			
			// strip trailing spaces (if any)
			while (FullName.Right(1) == TEXT(" "))
				FullName = FullName.Left(FullName.Len() - 1);
			return FindObject<UClass>(NULL, *FullName, 1);
		}

		return FindObject<UClass>(ANY_PACKAGE, *FullName, 1);
		unguard;
	}
	FString GetClassDisplayName(UClass* Class, UBOOL& bShowChildren) const
	{
		guard(WBrowserActor::GetClassDisplayName);
		// Test for parent classes.
		for (TObjectIterator<UClass> It; It; ++It)
		{
			if (It->GetSuperClass() == Class)
			{
				bShowChildren = 1;
				break;
			}
		}
		FString DisplayName = FObjectName(Class);

		if (Class->ClassFlags & (CLASS_Abstract | CLASS_NoUserCreate | CLASS_Transient))
			DisplayName += TEXT("*"); // Not placeable in editor.

		UClass* TestClass = FindObject<UClass>(ANY_PACKAGE, *FObjectName(Class), 1); // Test for inconclusive result
		if (TestClass != Class) // Duplicate class with same name found, append package name!
		{
			DisplayName += TEXT(" (");
			DisplayName += FObjectName(Class->GetOuter());
			DisplayName += ')';
		}
		
		return DisplayName;
		unguard;
	}
	void CopyDefPropsToClipboard(UClass* Class)
	{
		guard(WBrowserActor::CopyDefPropsToClipboard);
		appClipboardCopy(*GCodeFrame->ExportDefPropsToFString(Class));
		unguard;
	}
	void CopyDefPropsFromClipboard(UClass* Class)
	{
		guard(WBrowserActor::CopyDefPropsFromClipboard);
		FString Str(appClipboardPaste());
		GCodeFrame->ImportDefPropsFromFString(Class, Str);
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
