/*=============================================================================
	BrowserActor : Browser window for actor classes
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>

bool bReplacement;

// --------------------------------------------------------------
//
// NEW CLASS Dialog
//
// --------------------------------------------------------------
static UClass* FindClassFromName( const TCHAR* N )
{
	if( !appStricmp(N,TEXT("None")) )
		return NULL;

	// Find the end offset
	const TCHAR* S = N;
	while( *S && *S!='*' && *S!=' ' )
		++S;

	FString SeekName(N, S);

	// Skip star
	if( *S=='*' )
		++S;

	if( !*S || S[1]!='(' ) // Contains no package name.
	{
		UClass* Res = FindObject<UClass>(ANY_PACKAGE, *SeekName, 1);
		return Res;
	}

	// Parse package name.
	S+=2;
	const TCHAR* NameOffset = S;
	while( *S && *S!=')' )
		++S;
	FString PckName(NameOffset, S);

	// Parse the class name.
	static TCHAR FullName[512];
	appSprintf(FullName, TEXT("%ls.%ls"), *PckName, *SeekName);

	return FindObject<UClass>(NULL, FullName, 1);
}
static UBOOL ContainsPlaceable(UClass* Class)
{
	if (Class->GetSuperClass() == UObject::StaticClass()) // Quick reject for Object classes list.
		return (Class == AActor::StaticClass());

	if ((Class->IsClassType(CLCAST_AActor) && Class->GetDefaultActor()->bIsMover) || Class->IsClassType(CLCAST_AProjectile) || Class->IsClassType(CLCAST_APlayerPawn))
		return FALSE;

	if (!(Class->ClassFlags & (CLASS_Transient | CLASS_Abstract | CLASS_NoUserCreate)))
		return TRUE;

	for (TObjectIterator<UClass> It; It; ++It)
	{
		if (It->GetSuperClass() == Class && ContainsPlaceable(*It))
			return TRUE;
	}
	return FALSE;
}
static const TCHAR* GetClassDisplayName( UClass* Class, UBOOL& bShowChildren, UBOOL bPlaceableOnly )
{
	// Test for parent classes.
	for( TObjectIterator<UClass> It; It; ++It )
	{
		if( It->GetSuperClass()==Class && (!bPlaceableOnly || ContainsPlaceable(*It)))
		{
			bShowChildren = 1;
			break;
		}
	}
	static TCHAR ResultName[700];

	appStrcpy(ResultName,Class->GetName());

	if (Class->ClassFlags & (CLASS_Abstract | CLASS_NoUserCreate | CLASS_Transient))
		appStrcat(ResultName,TEXT("*")); // Not placeable in editor.

	UClass* TestClass = FindObject<UClass>(ANY_PACKAGE,Class->GetName(),1); // Test for inconclusive result
	if( TestClass!=Class ) // Duplicate class with same name found, append package name!
	{
		appStrcat(ResultName,TEXT(" ("));
		appStrcat(ResultName,Class->GetOuter()->GetName());
		appStrcat(ResultName,TEXT(")"));
	}
	return ResultName;
}

inline void GetClassDesc(UClass* C, FString& Result)
{
	if (!C->ScriptText)
	{
		if (C->GetFlags() & RF_Native)
			Result = FString::Printf(TEXT("%ls\n(C++ object)"), C->GetFullName());
		else Result = FString::Printf(TEXT("%ls\n(no code)"), C->GetFullName());
		return;
	}

	Result.Empty();
	const TCHAR* S = *C->ScriptText->Text;
	while (*S)
	{
		// Ship whitespaces.
		while (*S == ' ' || *S == '\t' || *S == '\r' || *S == '\n')
			++S;

		if (*S != '/') // Begin actual code, end here.
			break;
		++S;
		if (*S == '/') // Comment until end of line.
		{
			++S;

			// Skip comment formatting block and whitespaces.
			while (*S == ' ' || *S == '\t' || *S == '=' || *S == '/')
				++S;

			// End if end of comment line.
			if (*S == '\r' || *S == '\n')
				continue;

			const TCHAR* Start = S;
			while (*S && *S != '\r' && *S != '\n')
				++S;

			// Has enough information to go with.
			if (Start <= (S + 1))
			{
				if (Result.Len())
					Result += FString(TEXT("\n")) + FString(Start, S);
				else Result = FString(Start, S);
			}
		}
		else if (*S == '*') // Comment block
		{
			++S;
			while (*S)
			{
				// Skip comment formatting block and whitespaces.
				while (*S == ' ' || *S == '\t' || *S == '=' || *S == '/' || *S == '\r' || *S == '\n')
					++S;

				// Reached end of comment block.
				if (*S == '*' && S[1] == '/')
					break;

				const TCHAR* Start = S;
				while (*S && *S != '\r' && *S != '\n')
					++S;

				// Has enough information to go with.
				if (Start <= (S + 1))
				{
					if (Result.Len())
						Result += FString(TEXT("\n")) + FString(Start, S);
					else Result = FString(Start, S);
				}
			}
		}
		else break;
	}

	if (Result.Len() < 3)
		Result = FString::Printf(TEXT("%ls\n(no comments)"), C->GetFullName());
}

class WDlgNewClass : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgNewClass,WDialog,UnrealEd)

	// Variables.
	WButton OkButton;
	WButton CancelButton;
	WLabel ParentLabel;
	WEdit PackageEdit;
	WEdit NameEdit;

	FString ParentClass, Package, Name;

	// Constructor.
	WDlgNewClass( UObject* InContext, WWindow* InOwnerWindow )
	:	WDialog			( TEXT("New Class"), IDDIALOG_NEW_CLASS, InOwnerWindow )
	,	OkButton		( this, IDOK,			FDelegate(this,(TDelegate)&WDlgNewClass::OnOk) )
	,	CancelButton	( this, IDCANCEL,		FDelegate(this,(TDelegate)&WDlgNewClass::EndDialogFalse) )
	,	ParentLabel		( this, IDSC_PARENT )
	,	PackageEdit		( this, IDEC_PACKAGE )
	,	NameEdit		( this, IDEC_NAME )
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgNewClass::OnInitDialog);

		// Make sure this class has a script.
		//
		FStringOutputDevice GetResult;
	    GEditor->Get(TEXT("SCRIPTPOS"), *ParentClass, GetResult);
		if( !GetResult.Len() )
		{
			appMsgf( *(FString::Printf(TEXT("Subclassing of '%ls' is not allowed."), *ParentClass)) );
			EndDialog(FALSE);
		}

		WDialog::OnInitDialog();

		ParentLabel.SetText( *ParentClass );
		PackageEdit.SetText( TEXT("MyPackage") );
		NameEdit.SetText( *(FString::Printf(TEXT("My%ls"), *ParentClass) ) );
		::SetFocus( PackageEdit.hWnd );
		bReplacement=false;

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
			bReplacement=false;
			debugf(TEXT("Checking for dublicate classname: %ls"),*Name);
			UClass* Class =  FindObject<UClass>(ANY_PACKAGE,*Name,1);
			if (Class)
			{
				debugf(TEXT("Found dublicate classname: %ls"),Class->GetFullName());
				if( Package!=Class->GetOuter()->GetName() )
				{
					appMsgf(TEXT("Can't create duplicate of %ls with different package name."),Class->GetFullName());
					return;
				}
				if( !GWarn->YesNof(TEXT("An Actor class named '%ls' already exists! Do you want to replace it?"), Class->GetFullName()))
					return;
				else bReplacement=true;
			}

			// Create new class.
			//
			GEditor->Exec( *(FString::Printf( TEXT("CLASS NEW NAME=\"%ls\" PACKAGE=\"%ls\" PARENT=\"%ls\""),
				*Name, *Package, *ParentClass)) );
			GEditor->Exec( *(FString::Printf(TEXT("SETCURRENTCLASS CLASS=\"%ls\""), *Name)) );

			// Create standard header for the new class.
			//
			FString S = FString::Printf(
				TEXT("//=============================================================================\r\n// %ls.\r\n//=============================================================================\r\nclass %ls extends %ls;\r\n"),
				*Name, *Name, *ParentClass);
			GEditor->Set(TEXT("SCRIPT"), *Name, *S);

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
			return TRUE;
		unguard;
	}
};

class WDlgFindClass : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgFindClass,WDialog,UnrealEd)

	// Variables.
	WButton OkButton;
	WButton CancelButton;
	WEdit NameEdit;

	FString Name;

	// Constructor.
	WDlgFindClass( UObject* InContext, WWindow* InOwnerWindow )
	:	WDialog			( TEXT("Find Class"),	IDDIALOG_FIND_CLASS, InOwnerWindow )
	,	OkButton		( this, IDOK,			FDelegate(this,(TDelegate)&WDlgFindClass::OnOk) )
	,	CancelButton	( this, IDCANCEL,		FDelegate(this,(TDelegate)&WDialog::EndDialogFalse) )
	,	NameEdit		( this, IDEC_NAME )
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgFindClass::OnInitDialog);
		WDialog::OnInitDialog();
		::SetFocus( NameEdit.hWnd );
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
		guard(WDlgNewClass::DoModal);
		return WDialog::DoModal( hInstance );
		unguard;
	}
	void OnOk()
	{
		guard(WDlgFindClass::OnOk);
		if( GetDataFromUser() )
		{
			// Check if class exists.
			//
			debugf(TEXT("Checking for classname: %ls"),*Name);
			UClass* Class =  FindObject<UClass>(ANY_PACKAGE,*Name,1);
			if (Class)
			{
				debugf(TEXT("Found classname: %ls"),Class->GetFullName());
				GEditor->Exec( *(FString::Printf(TEXT("SETCURRENTCLASS CLASS=\"%ls\""), *Name)) );
			}
			else appMsgf(TEXT("Class %ls not found!"),*Name);
			EndDialog(TRUE);
		}
		unguard;
	}
	BOOL GetDataFromUser( void )
	{
		guard(WDlgFindClass::GetDataFromUser);
		Name = NameEdit.GetText();
		if( !Name.Len() )
		{
			appMsgf( TEXT("Invalid input.") );
			return FALSE;
		}
		else
			return TRUE;
		unguard;
	}
};

// --------------------------------------------------------------
//
// WBrowserActor
//
// --------------------------------------------------------------

#define ID_BA_TOOLBAR	29030
TBBUTTON tbBAButtons[] = {
	{ 0, IDMN_MB_DOCK, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 1, IDMN_AB_FileOpen, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 2, IDMN_AB_FileSave, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 3, IDMN_AB_NEW_CLASS, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 4, IDMN_AB_EDIT_SCRIPT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 5, IDMN_AB_DELETE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 6, IDMN_AB_DEF_PROP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
};
struct {
	TCHAR ToolTip[64];
	int ID;
} ToolTips_BA[] = {
	TEXT("Toggle Dock Status"), IDMN_MB_DOCK,
	TEXT("Open Package"), IDMN_AB_FileOpen,
	TEXT("Save Selected Packages"), IDMN_AB_FileSave,
	TEXT("New Script"), IDMN_AB_NEW_CLASS,
	TEXT("Edit Script"), IDMN_AB_EDIT_SCRIPT,
	TEXT("Remove Class"), IDMN_AB_DELETE,
	TEXT("Edit Default Properties"), IDMN_AB_DEF_PROP,
	NULL, 0
};

class WBrowserActor : public WBrowser
{
	DECLARE_WINDOWCLASS(WBrowserActor,WBrowser,Window)

	WTreeView* pTreeView;
	WCheckBox* pObjectsCheck, *pPlaceableCheck;
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
		pPlaceableCheck = NULL;
		htiRoot = htiLastSel = NULL;
		MenuID = IDMENU_BrowserActor;
		BrowserID = eBROWSER_ACTOR;
		Description = TEXT("Actor Classes");
	}

	// WBrowser interface.
	void OpenWindow( UBOOL bChild )
	{
		guard(WBrowserActor::OpenWindow);
		WBrowser::OpenWindow( bChild );
		SetCaption();
		unguard;
	}
	virtual void SetCaption( FString* Tail = NULL )
	{
		guard(WBrowserActor::SetCaption);

		FString Extra;
		if( GEditor->CurrentClass )
		{
			Extra = GEditor->CurrentClass->GetPathName();
			//Extra = Extra.Right( Extra.Len() - 6 );	// remove "class" from the front of it
		}

		WBrowser::SetCaption( Extra );
		unguard;
	}
	void OnCreate()
	{
		guard(WBrowserActor::OnCreate);
		WBrowser::OnCreate();

		SetMenu( hWnd, LoadMenuW(hInstance, MAKEINTRESOURCEW(IDMENU_BrowserActor)) );
		pObjectsCheck = new WCheckBox( this, IDCK_OBJECTS );
		pObjectsCheck->ClickDelegate = FDelegate(this, (TDelegate)&WBrowserActor::OnObjectsClick);
		pObjectsCheck->OpenWindow( 1, 0, 0, 1, 1, TEXT("Actor classes only") );
		SendMessageW( pObjectsCheck->hWnd, BM_SETCHECK, BST_CHECKED, 0 );

		pPlaceableCheck = new WCheckBox(this, IDCK_OBJECTS);
		pPlaceableCheck->ClickDelegate = FDelegate(this, (TDelegate)&WBrowserActor::OnObjectsClick);
		pPlaceableCheck->OpenWindow(1, 0, 0, 1, 1, TEXT("Placeable classes only?"));
		INT bShowPlaceable = FALSE;
		GConfig->GetInt(*PersistentName, TEXT("ShowPlaceable"), bShowPlaceable, GUEdIni);
		SendMessageW(pPlaceableCheck->hWnd, BM_SETCHECK, (bShowPlaceable ? BST_CHECKED : BST_UNCHECKED), 0);
		
		pTreeView = new WTreeView( this, IDTV_TREEVIEW );
		pTreeView->OpenWindow( TRUE, 1, 0, 0, 1, TRUE );
		pTreeView->SelChangedDelegate = FDelegate(this, (TDelegate)&WBrowserActor::OnTreeViewSelChanged);
		pTreeView->ItemExpandingDelegate = FDelegate(this, (TDelegate)&WBrowserActor::OnTreeViewItemExpanding);
		pTreeView->DblClkDelegate = FDelegate(this, (TDelegate)&WBrowserActor::OnTreeViewDblClk);
		pTreeView->OnToolTipDelegate = FDelegate(this, (TDelegate)&WBrowserActor::OnGetToolTip);

		pPackagesList = new WCheckListBox( this, IDLB_PACKAGES );
		pPackagesList->OpenWindow( 1, 0, 0, 1 );

		if (!GConfig->GetInt(*PersistentName, TEXT("ShowPackages"), bShowPackages, GUEdIni))		bShowPackages = 1;
		UpdateMenu();

		ToolbarImage = reinterpret_cast<HBITMAP>(LoadImage(hInstance,
			MAKEINTRESOURCE(IDB_BrowserActor_TOOLBAR),
			IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS));
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

		unguard;
	}
	virtual void UpdateMenu()
	{
		guard(WBrowserActor::UpdateMenu);

		HMENU menu = IsDocked() ? GetMenu( OwnerWindow->hWnd ) : GetMenu( hWnd );

		CheckMenuItem( menu, IDMN_AB_SHOWPACKAGES, MF_BYCOMMAND | (bShowPackages ? MF_CHECKED : MF_UNCHECKED) );
		CheckMenuItem( menu, IDMN_MB_DOCK, MF_BYCOMMAND | (IsDocked() ? MF_CHECKED : MF_UNCHECKED) );

		unguard;
	}
	void RefreshPackages(void)
	{
		guard(WBrowserActor::RefreshPackages);

		// PACKAGES
		//
		FStringOutputDevice GetResult;
	    GEditor->Get(TEXT("OBJ"), TEXT("PACKAGES CLASS=Class"), GetResult);

		TArray<FString> PkgArray;
		ParseStringToArray( TEXT(","), GetResult, &PkgArray );

		pPackagesList->Empty();

		for( int x = 0 ; x < PkgArray.Num() ; x++ )
			pPackagesList->AddString( *(FString::Printf( TEXT("%ls"), *PkgArray(x))) );

		pPackagesList->AddString( *(FString::Printf( TEXT("MyLevel"))) );

		unguard;
	}
	void OnDestroy()
	{
		guard(WBrowserActor::OnDestroy);

		INT bShowPlaceable = pPlaceableCheck->IsChecked() != 0;
		GConfig->SetInt(*PersistentName, TEXT("ShowPlaceable"), bShowPlaceable, GUEdIni);

		delete pTreeView;
		delete pObjectsCheck;
		delete pPlaceableCheck;
		delete pPackagesList;

		::DestroyWindow( hWndToolBar );
		delete ToolTipCtrl;

		if (ToolbarImage)
			DeleteObject(ToolbarImage);

		GConfig->SetInt(*PersistentName, TEXT("ShowPackages"), bShowPackages, GUEdIni);

		WBrowser::OnDestroy();
		unguard;
	}
	void FindClass( UClass* Class )
	{
		UClass* OuterMost = (!pObjectsCheck->IsChecked() ? UObject::StaticClass() : AActor::StaticClass());
		RefreshActorList();
		FString PathTo = TEXT("");
		UBOOL bDummy = 0;

		while( OuterMost!=Class && OuterMost )
		{
			UClass* Find = Class;
			for( ; (Find && Find->GetSuperClass()!=OuterMost); Find=Find->GetSuperClass() )
			{}
			if (!Find)
			{
				OuterMost = NULL;
				break; // Might be an object while having Show actor classes only checked.
			}
			if (!Find->bEditorExpanded && Find != Class)
			{
				HTREEITEM NewItem = pTreeView->FindItem(pTreeView->hWnd, htiLastSel, GetClassDisplayName(Find, bDummy, FALSE));
				if (NewItem)
					SendMessageW(pTreeView->hWnd, TVM_EXPAND, TVE_EXPAND, (LPARAM)(HTREEITEM)NewItem);
				PathTo += FString::Printf(TEXT("%ls "), Find->GetName());
			}
			OuterMost = Find;
		}

		if (PathTo.Len() > 0)
		{

			//appMsgf(TEXT("%ls found in: %ls"), OuterMost->GetFullName(), *PathTo);
			debugf(TEXT("%ls found in: %ls"), OuterMost->GetFullName(), *PathTo);
			HTREEITEM NewItem = pTreeView->FindItem(pTreeView->hWnd, htiRoot, GetClassDisplayName(OuterMost, bDummy, FALSE));
			if (NewItem)
				SendMessageW(pTreeView->hWnd, TVM_SELECTITEM, TVGN_CARET, (LPARAM)NewItem);
			return;
		}
		else if (OuterMost)
		{
			//appMsgf(TEXT("%ls found in: Root"), OuterMost->GetFullName(), *PathTo);
			debugf(TEXT("%ls found in: Root"), OuterMost->GetFullName(), *PathTo);
			HTREEITEM NewItem = pTreeView->FindItem(pTreeView->hWnd, htiRoot, GetClassDisplayName(OuterMost, bDummy, FALSE));
			if (NewItem)
				SendMessageW(pTreeView->hWnd, TVM_SELECTITEM, TVGN_CARET, (LPARAM)NewItem);
		}
		else
		{
			debugf(TEXT("Class not found"));
			return;
		}
	}
	void OnCommand( INT Command )
	{
		guard(WBrowserActor::OnCommand);
		switch( Command )
		{
			case WM_TREEVIEW_RIGHT_CLICK:
				{
					// Select the tree item underneath the mouse cursor.
					TVHITTESTINFO tvhti;
					POINT ptScreen;
					::GetCursorPos( &ptScreen );
					tvhti.pt = ptScreen;
					::ScreenToClient( pTreeView->hWnd, &tvhti.pt );

					SendMessageW( pTreeView->hWnd, TVM_HITTEST, 0, (LPARAM)&tvhti);

					if( tvhti.hItem )
						SendMessageW( pTreeView->hWnd, TVM_SELECTITEM, TVGN_CARET, (LPARAM)(HTREEITEM)tvhti.hItem);

					// Show a context menu for the currently selected item.
					HMENU menu = GetSubMenu( LoadMenuW(hInstance, MAKEINTRESOURCEW(IDMENU_BrowserActor_Context)), 0 );
					TrackPopupMenu( menu,
						TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
						ptScreen.x, ptScreen.y, 0,
						hWnd, NULL);
				}
				break;

			case IDMN_AB_EXPORT_ALL:
				{
					if( ::MessageBox( hWnd, TEXT("This option will export all classes to text .uc files which can later be rebuilt. Do you want to do this?"), TEXT("Export classes to *.uc files"), MB_YESNO) == IDYES)
					{
						GEditor->Exec( TEXT("CLASS SPEW ALL") );
					}
				}
				break;

			case IDMN_AB_EXPORT:
				{
					if( ::MessageBox( hWnd, TEXT("This option will export all modified classes to text .uc files which can later be rebuilt. Do you want to do this?"), TEXT("Export classes to *.uc files"), MB_YESNO) == IDYES)
					{
						GEditor->Exec( TEXT("CLASS SPEW") );
					}
				}
				break;

			case IDMN_AB_EXPORT_Selected:
				if (::MessageBox(hWnd, TEXT("This option will export all selected packages to text .uc files which can later be rebuilt. Do you want to do this?"), TEXT("Export packages to *.uc files"), MB_YESNO) == IDYES)
				{
					FString Pkg;
					for( int x = 0 ; x < pPackagesList->GetCount() ; x++ )
					{
						if( pPackagesList->GetItemData(x) )
						{
							Pkg = *(pPackagesList->GetString( x ));
							GEditor->Exec( *(FString::Printf(TEXT("CLASS SPEW PACKAGE=\"%ls\""), *Pkg )) );
						}
					}
				}
				break;

			case IDMN_AB_SHOWPACKAGES:
				{
					bShowPackages = !bShowPackages;
					PositionChildControls();
					UpdateMenu();
				}
				break;

			case IDMN_AB_NEW_CLASS:
				{
					WDlgNewClass l_dlg( NULL, this );
					FString Class =  GEditor->CurrentClass ? GEditor->CurrentClass->GetName() : TEXT("Actor");
					if( l_dlg.DoModal( Class ) )
					{
						if (!bReplacement)
						{
							UBOOL bDummy=0;
							if( !GEditor->CurrentClass->GetSuperClass()->bEditorExpanded )
								AddChildren(GEditor->CurrentClass->GetSuperClass()->GetPathName(),htiLastSel, 0);
							else if( !pTreeView->AddItem(GetClassDisplayName(GEditor->CurrentClass,bDummy,FALSE),htiLastSel,FALSE) )
							{
								OnTreeViewItemExpanding();
								RefreshActorList();
							}
							SendMessageW( pTreeView->hWnd, TVM_EXPAND, TVE_EXPAND, (LPARAM)(HTREEITEM)htiLastSel );
							HTREEITEM NewItem = pTreeView->FindItem(pTreeView->hWnd, htiLastSel, GetClassDisplayName(GEditor->CurrentClass, bDummy, FALSE));
							//htiLastSel=TreeView_GetNextItem(pTreeView->hWnd,htiLastSel,TVGN_CHILD);
							if (NewItem)
								SendMessageW(pTreeView->hWnd, TVM_SELECTITEM, TVGN_CARET, (LPARAM)NewItem);
							else debugf(TEXT("Item not found!"));
							//SendMessageX( pTreeView->hWnd, TVM_SELECTITEM, TVGN_DROPHILITE, (LPARAM)(HTREEITEM)htiLastSel);
						}
						// Open an editing window.
						//
						GCodeFrame->AddClass( GEditor->CurrentClass );
						RefreshPackages();
					}
				}
				break;

			case IDMN_AB_DELETE:
				{
					if( !GEditor->CurrentClass )
						break;
					UClass* Obj = GEditor->CurrentClass;
					GEditor->CurrentClass = NULL;
					TArray<UObject*> Res;
					FCheckObjRefArc Ref(&Res,Obj,false);

					if( Ref.HasReferences() )
					{
						GEditor->CurrentClass = Obj;
						FString RefStr(FString::Printf(TEXT("%ls"),Res(0)->GetFullName()));
						for( INT i=1; i<Min<INT>(4,Res.Num()); i++ )
						{
							RefStr+=FString::Printf(TEXT(", %ls"),Res(i)->GetFullName());
							if( i==3 && Res.Num()>4 )
							RefStr+=TEXT(", etc...");
						}
						appMsgf( TEXT("Class %ls is still being referenced by: %ls"),Obj->GetName(),*RefStr );
					}
					else
					{
						FString CurName = Obj->GetName();
						GEditor->Exec( TEXT("SETCURRENTCLASS Class=Light") );

						// Try to cleanly update the actor list.  If this fails, just reload it from scratch...
						if( !SendMessageW( pTreeView->hWnd, TVM_DELETEITEM, 0, (LPARAM)htiLastSel ) )
							RefreshActorList();
						GCodeFrame->RemoveClass( CurName );
						Obj->RemoveFromRoot();
						delete Obj;
						debugf(TEXT("Removed: %ls"),*CurName);
					}
				}
				break;

			case IDMN_AB_DEF_PROP:
				if (GEditor->CurrentClass)
					GEditor->Exec( *(FString::Printf(TEXT("HOOK CLASSPROPERTIES CLASS=\"%ls\""), GEditor->CurrentClass->GetPathName())) );
				break;

			case IDMN_AB_DEF_PROPCOPY:
				if (GEditor->CurrentClass)
					GEditor->Exec(*(FString::Printf(TEXT("OBJ COPYPROPERTIES CLASS=\"%ls\""), GEditor->CurrentClass->GetPathName())));
				break;

			case IDMN_AB_DEF_PROPPAST:
				if (GEditor->CurrentClass)
				{
					if( !GWarn->YesNof(TEXT("Paste the defaultproperties from clipboard into %ls?"),GEditor->CurrentClass->GetFullName()))
						return;
					else GEditor->Exec(*(FString::Printf(TEXT("OBJ PASTEPROPERTIES CLASS=\"%ls\""), GEditor->CurrentClass->GetPathName())));
					RefreshPackages();
				}
				break;

			case IDMN_AB_FileOpen:
				{
					OPENFILENAMEA ofn;
					char File[8192] = "\0";

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(char) * 8192;
					ofn.lpstrFilter = "Class Packages (*.u)\0*.u\0All Files\0*.*\0\0";
					ofn.lpstrInitialDir = "..\\system";
					ofn.lpstrDefExt = "u";
					ofn.lpstrTitle = "Open Class Package";
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

						for( int x = iStart ; x < StringArray.Num() ; x++ )
						{
							GEditor->Exec(*FString::Printf(TEXT("CLASS LOAD FILE=\"%ls%ls\""), *Prefix, *(StringArray(x))));
						}

						GBrowserMaster->RefreshAll();
						RefreshPackages();
					}

					GFileManager->SetDefaultDirectory(appBaseDir());
					RefreshPackages();
				}
				break;

			case IDMN_AB_FileSave:
				{
					FString Pkg;

					GWarn->BeginSlowTask( TEXT("Saving Packages"), 1, 0 );

					for( int x = 0 ; x < pPackagesList->GetCount() ; x++ )
					{
						if( pPackagesList->GetItemData(x) )
						{
							Pkg = *(pPackagesList->GetString( x ));
							GEditor->Exec( *(FString::Printf(TEXT("OBJ SAVEPACKAGE PACKAGE=\"%ls\" FILE=\"%ls.u\""), *Pkg, *Pkg )) );
						}
					}

					GWarn->EndSlowTask();
				}
				break;

			case IDMN_AB_EDIT_SCRIPT:
				{
					GCodeFrame->AddClass( GEditor->CurrentClass );
				}
				break;
			case IDMN_AB_FIND_CLASS:
				{
					WDlgFindClass l_dlg(NULL,this);
					if( l_dlg.DoModal() == IDOK )
					{
						if( GEditor->CurrentClass )
							FindClass(GEditor->CurrentClass);
					}
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
		guard(WBrowserActor::OnSize);
		WBrowser::OnSize(Flags, NewX, NewY);
		PositionChildControls();
		InvalidateRect( hWnd, NULL, FALSE );
		UpdateMenu();
		unguard;
	}
	void PositionChildControls( void )
	{
		guard(WBrowserActor::PositionChildControls);

		if( !pTreeView || !pObjectsCheck || !pPackagesList ) return;

		FRect CR = GetClientRect();

		::MoveWindow(hWndToolBar, 4, 0, 8 * MulDiv(16, DPIX, 96), 8 * MulDiv(16, DPIY, 96), 1);

		RECT R;
		::GetClientRect( hWndToolBar, &R );
		float Fraction = (CR.Width() - 8) / 10.0f;
		float Top = 4 + R.bottom;

		
		::MoveWindow( pObjectsCheck->hWnd, 4, Top, Fraction * 10, MulDiv(20, DPIY, 96), 1 );
		Top += MulDiv(20, DPIY, 96);
		::MoveWindow( pPlaceableCheck->hWnd, 4, Top, Fraction * 10, MulDiv(20, DPIY, 96), 1);
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
		RefreshActorList();
		unguard;
	}
	void RefreshActorList( void )
	{
		guard(WBrowserActor::RefreshActorList);
		pTreeView->Empty();

		for( TObjectIterator<UClass> It; It; ++It )
			It->bEditorExpanded = 0; // Reset flag.

		if( pObjectsCheck->IsChecked() )
			htiRoot = pTreeView->AddItem( TEXT("Actor*"), NULL, TRUE );
		else
			htiRoot = pTreeView->AddItem( TEXT("Object*"), NULL, TRUE );

		htiLastSel = NULL;

		SendMessageW( pTreeView->hWnd, TVM_EXPAND, TVE_EXPAND, (LPARAM)htiRoot );
		unguard;
	}
	void AddChildren( const TCHAR* pParentName, HTREEITEM hti, UBOOL Expand )
	{
		guard(WBrowserActor::AddChildren);
		UClass* ParentClass = FindClassFromName(pParentName);
		if( !ParentClass )
		{
			pTreeView->AddItem( TEXT("None"), hti, FALSE );
			return;
		}
		UBOOL bPlaceableOnly = pPlaceableCheck->IsChecked() != 0;
		ParentClass->bEditorExpanded = 1;

		for( TObjectIterator<UClass> It; It; ++It )
		{
			UClass* C = *It;
			if( C->GetSuperClass()==ParentClass && (!bPlaceableOnly || ContainsPlaceable(C)))
			{
				UBOOL bChildren = 0;
				const TCHAR* Name = GetClassDisplayName(C, bChildren, bPlaceableOnly);
				pTreeView->AddItem( Name, hti, bChildren );
			}
		}
		unguard;
	}
	void OnTreeViewSelChanged( void )
	{
		guard(WBrowserActor::OnTreeViewSelChanged);
		NMTREEVIEW* pnmtv = (LPNMTREEVIEW)pTreeView->LastlParam;
		TCHAR chText[1024] = TEXT("\0");
		TVITEM tvi;

		appMemzero( &tvi, sizeof(tvi));
		htiLastSel = tvi.hItem = pnmtv->itemNew.hItem;
		tvi.mask = TVIF_TEXT;
		tvi.pszText = chText;
		tvi.cchTextMax = sizeof(chText);

		if( SendMessageW( pTreeView->hWnd, TVM_GETITEM, 0, (LPARAM)&tvi) )
		{
			UClass* CurClass = FindClassFromName(tvi.pszText);
			GEditor->Exec( *(FString::Printf(TEXT("SETCURRENTCLASS CLASS=\"%ls\""), (CurClass ? CurClass->GetPathName() : TEXT("Light")) )));
		}
		SetCaption();
		unguard;
	}
	void OnTreeViewItemExpanding( void )
	{
		guard(WBrowserActor::OnTreeViewItemExpanding);
		NMTREEVIEW* pnmtv = pTreeView->GetTreeViewParm();

		// If this item already has children loaded, bail...
		if (SendMessageW(pTreeView->hWnd, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)pnmtv->itemNew.hItem))
			return;

		const TCHAR* Value = pTreeView->GetItemValue(pnmtv->itemNew.hItem);
		if(Value)
			AddChildren(Value, pnmtv->itemNew.hItem, 0);
		unguard;
	}
	void OnTreeViewDblClk( void )
	{
		guard(WBrowserActor::OnTreeViewDblClk);
		GCodeFrame->AddClass( GEditor->CurrentClass );
		unguard;
	}
	void OnObjectsClick()
	{
		guard(WBrowserActor::OnObjectsClick);
		RefreshActorList();
		SendMessageW( pTreeView->hWnd, TVM_EXPAND, TVE_EXPAND, (LPARAM)htiRoot );
		unguard;
	}
	void OnGetToolTip()
	{
		guard(WBrowserActor::OnGetToolTip);
		LPNMTVGETINFOTIP pTip = pTreeView->GetToolTipParm();
		const TCHAR* ItemEntry = pTreeView->GetItemValue(pTip->hItem);
		if (ItemEntry)
		{
			UClass* CurClass = FindClassFromName(ItemEntry);
			if (CurClass)
			{
				FString Comments;
				GetClassDesc(CurClass, Comments);
				appStrncpy(pTip->pszText, *Comments, pTip->cchTextMax);
				pTreeView->bShowToolTip = TRUE;
			}
		}
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
