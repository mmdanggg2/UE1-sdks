
/*=============================================================================
	Browser : Base class for browser windows
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>

static int CDECL ClassSortCompare( const void *elem1, const void *elem2 )
{
	return appStricmp((*(UClass**)elem1)->GetName(),(*(UClass**)elem2)->GetName());
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
			if( i>0 ) Work += TEXT(",");
			Work += FString::Printf( TEXT("%ls%ls.%ls"), 
				Children ? TEXT("C") : TEXT("_"),
				Results[i]->GetOuterUPackage()->GetName(),
				Results[i]->GetName() );
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
			if( i>0 ) Work += TEXT(",");
			Work += FString::Printf( TEXT("%ls.%ls"),
				Results[i]->GetOuterUPackage()->GetName(),
				Results[i]->GetName() );
		}

		*pOutput = Work;
	}
	unguard;
}
// Takes a delimited string and breaks it up into elements of a string array.
//
void ParseStringToArray( const TCHAR* pchDelim, FString String, TArray<FString>* _pArray)
{
	guard(ParseStringToArray);
	int i;
	FString S = String;

	i = S.InStr( pchDelim );

	while( i > 0 )
	{
		new(*_pArray)FString( S.Left(i) );
		S = S.Mid( i + 1, S.Len() );
		i = S.InStr( pchDelim );
	}

	new(*_pArray)FString( S );
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
		guard(WDlgExtrude::OnSetCursor);
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

		if (!GConfig->GetInt(*PersistentName, TEXT("X"), X, GUEdIni))	X = 0;
		if (!GConfig->GetInt(*PersistentName, TEXT("Y"), Y, GUEdIni))	Y = 0;
		if (!GConfig->GetInt(*PersistentName, TEXT("W"), W, GUEdIni))	W = 512;
		if (!GConfig->GetInt(*PersistentName, TEXT("H"), H, GUEdIni))	H = 384;

		if( !W ) W = 320;
		if( !H ) H = 200;

		FPoint desktop = GetDesktopResolution();
		X = Clamp<INT>(X, 0, desktop.X - W);
		Y = Clamp<INT>(Y, 0, desktop.Y - 80);

		::MoveWindow( hWnd, X, Y, W, H, TRUE );

		unguard;
	}
	virtual void SetCaption( FString Tail=TEXT("") )
	{
		guard(WBrowser::SetCaption);
		FString Caption;

		Caption = Description;

		if( Tail && Tail.Len() )
			Caption = *(FString::Printf(TEXT("%ls - %ls"), *Caption, *Tail ) );

		if( IsDocked() )
			OwnerWindow->SetText( *Caption );
		else
			SetText( *Caption );

		unguard;
	}
	virtual FString	GetCaption()
	{
		guard(WBrowser::GetCaption);
		return GetText();
		unguard;
	}
	virtual void UpdateMenu()
	{
		guard(WBrowser::UpdateMenu);
		unguard;
	}
	void OnDestroy()
	{
		guard(WBrowser::OnDestroy);

		// Save Window position (base class doesn't always do this properly)
		// (Don't do this if the window is minimized.)
		//
		if( !::IsIconic( hWnd ) && !::IsZoomed( hWnd ) )
		{
			RECT R;
			if(::GetWindowRect(hWnd, &R))
			{
				GConfig->SetInt(*PersistentName, TEXT("Active"), m_bShow, GUEdIni);
				GConfig->SetInt(*PersistentName, TEXT("Docked"), bDocked, GUEdIni);
				GConfig->SetInt(*PersistentName, TEXT("X"), R.left, GUEdIni);
				GConfig->SetInt(*PersistentName, TEXT("Y"), R.top, GUEdIni);
				GConfig->SetInt(*PersistentName, TEXT("W"), R.right - R.left, GUEdIni);
				GConfig->SetInt(*PersistentName, TEXT("H"), R.bottom - R.top, GUEdIni);
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
	// Searches a list of filenames and replaces all single NULL's with | characters.  This allows
	// the regular parse routine to work correctly.  The return value is the number of NULL's
	// that were replaced -- if this is greater than zero, you have multiple filenames.
	//
	int FormatFilenames( char* _pchFilenames )
	{
		guard(WBrowser::FormatFilenames);
		char *pch = _pchFilenames;
		int l_iNULLs = 0;

		while( true )
		{
			if( *pch == '\0' )
			{
				if( *(pch+1) == '\0') break;

				*pch = '|';
				l_iNULLs++;
			}
			pch++;
		}

		return l_iNULLs;
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

private:
	UBOOL bDocked;	// If TRUE, then this browser is docked inside the master browser window
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
