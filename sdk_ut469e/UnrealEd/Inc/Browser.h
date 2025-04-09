/*=============================================================================
	Browser : Base class for browser windows
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>

extern FString GLastDir[eLASTDIR_MAX];

static int CDECL ClassSortCompare( const void *elem1, const void *elem2 )
{
	return appStricmp(*FObjectName(*(UClass**)elem1),*FObjectName(*(UClass**)elem2));
}
void Query( ULevel* Level, const TCHAR* Item, FString* pOutput, UPackage* InPkg = NULL)
{
	guard(Query);
	enum	{MAX_RESULTS=1024};
	int		NumResults = 0;
	UClass	*Results[MAX_RESULTS];
	FString Work;

	if( ParseCommand(&Item,TEXT("QUERY")) )
	{
		UClass *Parent = NULL;
		ParseObject<UClass>(Item,TEXT("PARENT="),Parent, InPkg ? InPkg : ANY_PACKAGE);

		// Make a list of all child classes.
		for( TObjectIterator<UClass> It; It && NumResults<MAX_RESULTS; ++It )
			if( It->GetSuperClass()==Parent )
				Results[NumResults++] = *It;

		// Return the results.
		for( INT i=0; i<NumResults; i++ )
		{
			// See if this item has children.
			INT Children = 0;
			for( TObjectIterator<UClass> It; It; ++It )
				if( It->GetSuperClass()==Results[i] )
					Children++;

			// Add to result string.
			if( i>0 ) Work += ',';
			Work += Children ? 'C' : '_';
			Work += *FObjectName(Results[i]->GetOuterUPackage());
			Work += '.';
			Work += *FObjectName(Results[i]);
		}

		*pOutput = Work;
	}
	if( ParseCommand(&Item,TEXT("GETCHILDREN")) )
	{
		UClass *Parent = NULL;
		ParseObject<UClass>(Item,TEXT("CLASS="),Parent, InPkg ? InPkg : ANY_PACKAGE);
		UBOOL Concrete=0; ParseUBOOL( Item, TEXT("CONCRETE="), Concrete );

		// Make a list of all child classes.
		for( TObjectIterator<UClass> It; It && NumResults<MAX_RESULTS; ++It )
			if( It->IsChildOf(Parent) && (!Concrete || !(It->ClassFlags & CLASS_Abstract)) )
				Results[NumResults++] = *It;

		// Sort them by name.
		appQsort( Results, NumResults, sizeof(UClass*), ClassSortCompare );

		// Return the results.
		for( int i=0; i<NumResults; i++ )
		{
			if( i>0 ) Work += ',';
			Work += *FObjectName(Results[i]->GetOuterUPackage());
			Work += '.';
			Work += *FObjectName(Results[i]);
		}

		*pOutput = Work;
	}
	unguard;
}

// --------------------------------------------------------------
//
// WBrowser
//
// --------------------------------------------------------------

class WBrowser : public WWindow
{
	DECLARE_WINDOWCLASS(WBrowser,WWindow,Window)

	FString SavePkgName, Description, DefCaption;
	int MenuID, BrowserID;
	HWND hwndEditorFrame;
	HMENU hmenu;
	MRUList* mrulist{};
	HWND hWndToolBar{};
	WToolTip* ToolTipCtrl{};
	HBITMAP ToolbarImage{};

	// Structors.
	WBrowser( FName InPersistentName, WWindow* InOwnerWindow, HWND InEditorFrame )
		: WWindow(InPersistentName, InOwnerWindow)
		, hmenu(NULL)
	{
		check(InOwnerWindow);
		bDocked = 0;
		MenuID = 0;
		BrowserID = -1;
		hwndEditorFrame = InEditorFrame;
		Description = TEXT("Browser");
	}

	// WWindow interface.
	void OpenWindow( UBOOL bChild )
	{
		guard(WBrowser::OpenWindow);
		MdiChild = 0;

		PerformCreateWindowEx
		(
			0,
			NULL,
			(bChild ? WS_CHILD  : WS_OVERLAPPEDWINDOW) | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
			0,
			0,
			MulDiv(320, DPIX, 96),
			MulDiv(200, DPIY, 96),
			OwnerWindow ? OwnerWindow->hWnd : NULL,
			NULL,
			hInstance
		);
		bDocked = bChild;
		Show(0);
		SetCaption();
		unguard;
	}
	int OnSysCommand( INT Command )
	{
		guard(WBrowser::OnSysCommand);
		if( Command == SC_CLOSE )
		{
			Show(0);
			return 1;
		}

		return 0;
		unguard;
	}
	INT OnSetCursor()
	{
		guard(WBrowser::OnSetCursor);
		WWindow::OnSetCursor();
		SetCursor(LoadCursorW(NULL, IDC_ARROW));
		return 0;
		unguard;
	}
	void OnCreate()
	{
		guard(WBrowser::OnCreate);
		WWindow::OnCreate();

		// Load windows last position.
		//
		int X, Y, W, H;

		if(!GConfig->GetInt( *PersistentName, TEXT("X"), X, GUEDIni ))	
			X = 0;
		if(!GConfig->GetInt( *PersistentName, TEXT("Y"), Y, GUEDIni ))	
			Y = 0;
		if(!GConfig->GetInt( *PersistentName, TEXT("W"), W, GUEDIni ))
			W = 512;
		if(!GConfig->GetInt( *PersistentName, TEXT("H"), H, GUEDIni ))
			H = 384;

		if( !W ) W = 320;
		if( !H ) H = 200;

		::MoveWindow( hWnd, X, Y, W, H, TRUE );		

		unguard;
	}
	virtual void SetCaption( FString* Tail = NULL )
	{
		guard(WBrowser::SetCaption);
		FString Caption;

		Caption = Description;

		if( Tail && Tail->Len() )
		{
			Caption += TEXT(" - ");
			Caption += **Tail;
		}

		if ( !IsDocked() )
			SetText( *Caption );
		else if ( IsCurrent() )
			OwnerWindow->SetText( *Caption );

		unguard;
	}
	virtual FString	GetCaption()
	{
		guard(WBrowser::GetCaption);
		return GetText();
		unguard;
	}
	// Get owned menu (if any), and apply to it MRU (if any) and dock state. Return NULL if currently no menu.
	HMENU GetMyMenu()
	{
		HMENU menu = NULL;
		// Docked browsers use Master one menu. Don't alter menu which not belong to this browser.
		if (!IsDocked() || IsCurrent())
		{
			menu = GetMenu(IsDocked() ? OwnerWindow->hWnd : hWnd);
			CheckMenuItem(menu, IDMN_MB_DOCK, MF_BYCOMMAND | (IsDocked() ? MF_CHECKED : MF_UNCHECKED));
			if (mrulist)
				mrulist->AddToMenu(hWnd, menu);
		}
		return menu;
	}
	virtual void UpdateMenu()
	{
		guard(WBrowser::UpdateMenu);
		GetMyMenu();
		unguard;
	}
	void OnDestroy()
	{
		guard(WBrowser::OnDestroy);

		if (hWndToolBar)
			::DestroyWindow(hWndToolBar);
		delete ToolTipCtrl;

		if (ToolbarImage)
			DeleteObject(ToolbarImage);

		if (mrulist)
			mrulist->WriteINI();
		delete mrulist;

		// Save Window position (base class doesn't always do this properly)
		// (Don't do this if the window is minimized.)
		//
		if( !::IsIconic( hWnd ) && !::IsZoomed( hWnd ) )
		{
			RECT R;
			if (::GetWindowRect(hWnd, &R))
			{
				GConfig->SetInt(*PersistentName, TEXT("Active"), m_bShow, GUEDIni);
				GConfig->SetInt(*PersistentName, TEXT("Docked"), bDocked, GUEDIni);
				GConfig->SetInt(*PersistentName, TEXT("X"), R.left, GUEDIni);
				GConfig->SetInt(*PersistentName, TEXT("Y"), R.top, GUEDIni);
				GConfig->SetInt(*PersistentName, TEXT("W"), R.right - R.left, GUEDIni);
				GConfig->SetInt(*PersistentName, TEXT("H"), R.bottom - R.top, GUEDIni);
			}
		}

		WWindow::OnDestroy();
		unguard;
	}
	void OnPaint()
	{
		guard(WBrowser::OnPaint);
		PAINTSTRUCT PS;
		HDC hDC = BeginPaint( *this, &PS );
		FillRect( hDC, GetClientRect(), (HBRUSH)(COLOR_BTNFACE+1) );
		EndPaint( *this, &PS );
		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WBrowser::OnSize);
		WWindow::OnSize(Flags, NewX, NewY);
		PositionChildControls();
		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	virtual void PositionChildControls( void )
	{
		guard(WBrowser::PositionChildControls);
		unguard;
	}	
	virtual FString GetCurrentPathName( void )
	{
		guard(WBrowser::GetCurrentPathName);
		return TEXT("");
		unguard;
	}
	virtual void RefreshAll()
	{
		guard(WBrowser::RefreshAll);
		unguard;
	}
	inline void RefreshPackages(WComboBox* pComboPackage, const TCHAR* Type)
	{
		guard(WBrowser::RefreshPackages);

		SetRedraw(false);

		INT Current = pComboPackage->GetCurrent();
		Current = (Current != CB_ERR) ? Current : 0;
		FString Package = pComboPackage->GetString(pComboPackage->GetCurrent());

		// PACKAGES
		//
		pComboPackage->Empty();

		FStringOutputDevice GetPropResult = FStringOutputDevice();
		GEditor->Get(TEXT("OBJ"), *(FString(TEXT("PACKAGES CLASS=")) += Type), GetPropResult);

		TArray<FString> StringArray;
		ParseStringToArray(TEXT(","), *GetPropResult, &StringArray);

		for (int x = 0; x < StringArray.Num(); x++)
			pComboPackage->AddString(*(StringArray(x)));

		pComboPackage->SetCurrent(0);
		pComboPackage->SetCurrent(Current);
		if (Package.Len())
		{
			Current = pComboPackage->FindString(*Package);
			if (Current != INDEX_NONE)
				pComboPackage->SetCurrent(Current);
		}

		SetRedraw(true);
		unguard;
	}
	template <class T>
	inline void RefreshGroups(WComboBox* pComboPackage, WComboBox* pComboGroup)
	{
		guard(WBrowser::RefreshGroups);

		FString Package = pComboPackage->GetString(pComboPackage->GetCurrent());
		UPackage* Pck = FindObject<UPackage>(NULL, *Package);
		int Current = pComboGroup->GetCurrent();
		Current = (Current != CB_ERR) ? Current : 0;
		FString Group = pComboGroup->GetString(pComboGroup->GetCurrent());

		// GROUPS
		//
		pComboGroup->Empty();

		TArray<FString> StringArray;
		if (Pck)
		{
			INT PackagePrefixLen = FObjectPathName(Pck).Len() + 1;
			FString GroupName;
			for (TObjectIterator<T> It; It; ++It)
			{
				if (!It->IsIn(Pck))
					continue;
				GroupName = FObjectPathName(It->GetOuter());
				GroupName.MidInline(PackagePrefixLen, MAXINT, EAllowShrinking::No);
				if (GroupName.Len() == 0)
					StringArray.AddUniqueItem(TEXT("None"));
				else
					StringArray.AddUniqueItem(*GroupName);
			}
		}

		for (int x = 0; x < StringArray.Num(); x++)
			pComboGroup->AddString(*(StringArray(x)));

		pComboGroup->SetCurrent(0);
		pComboGroup->SetCurrent(Current);
		if (Group.Len())
		{
			Current = pComboGroup->FindString(*Group);
			if (Current != INDEX_NONE)
				pComboGroup->SetCurrent(Current);
		}

		unguard;
	}
	template <class T, class V>
	inline void RefreshItemList(WComboBox* pComboPackage, WComboBox* pComboGroup, WCheckBox* pCheckGroupAll, WListBox* pListItems, TArray<T*>& ItemsList)
	{
		guard(WBrowser::RefreshItemList);

		FString Package = pComboPackage->GetString(pComboPackage->GetCurrent());
		FString Group = pComboGroup->GetString(pComboGroup->GetCurrent());
		FString Selected = pListItems->GetString(pListItems->GetCurrent());

		pListItems->Empty();

		ItemsList.Empty();
		UPackage* Pck = FindObject<UPackage>(NULL, *Package);

		if (Pck)
		{
			if (pCheckGroupAll->IsChecked())
			{
				for (TObjectIterator<T> It; It; ++It)
				{
					if (It->IsIn(Pck))
						((V*)this)->ListItem(*It);
				}
			}
			else
			{
				UPackage* GroupPck = Pck;
				if (Group != TEXT("None"))
					GroupPck = FindObject<UPackage>(Pck, *Group);
				for (TObjectIterator<T> It; It; ++It)
				{
					if (!It->IsIn(GroupPck))
						continue;
					UPackage* P = FindOuter<UPackage>(*It);
					if (P == GroupPck)
						((V*)this)->ListItem(*It);
				}
			}
		}

		INT Current = pListItems->FindStringExact(*Selected);
		if (Current >= 0)
			pListItems->SetCurrent(Current, 1);

		unguard;
	}
	virtual bool ShouldBlockAccelerators()
	{
		return false;
	}
	virtual LRESULT WndProc(UINT Message, WPARAM wParam, LPARAM lParam)
	{
		// From Accelerator
		if (Message == WM_COMMAND && HIWORD(wParam) == 1 && ShouldBlockAccelerators())
			return 0;

		return WWindow::WndProc(Message, wParam, lParam);
	}
	void OnCommand( INT Command )
	{
		guard(WBrowser::OnCommand);
		switch( Command ) {

			case IDMN_MB_DOCK:
			{
				bDocked = !bDocked;
				SendMessageW( hwndEditorFrame, WM_COMMAND, bDocked ? WM_BROWSER_DOCK : WM_BROWSER_UNDOCK, BrowserID );
			}
			break;

			default:
				WWindow::OnCommand(Command);
				break;
		}
		unguard;
	}
	inline UBOOL IsDocked() { return bDocked; }	
	UBOOL IsCurrent();
	INT CheckSavePackage(INT iFailed, FString Failed)
	{
		if (iFailed > 0)
			appMsgf(TEXT("Failed to save %d package%ls (%ls)"), iFailed, iFailed == 1 ? TEXT("") : TEXT("s"), *Failed);
		return iFailed;
	}

private:
	UBOOL bDocked;	// If TRUE, then this browser is docked inside the master browser window
};

// Takes a fully pathed filename, and just returns the name.
// i.e. "c:\test\file.txt" gets returned as "file".
//
FString GetFilenameOnly( FString Filename)
{
	guard(GetFilenameOnly);
	FString NewFilename = appFileBaseName(*Filename);

	if( NewFilename.InStr( TEXT(".") ) != -1 )
		NewFilename = NewFilename.Left( NewFilename.InStr( TEXT(".") ) );

	return NewFilename;
	unguard;
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
