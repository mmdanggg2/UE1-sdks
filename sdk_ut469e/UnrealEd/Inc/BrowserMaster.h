/*=============================================================================
	BrowserMaster : Master window where all "docked" browser reside
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>

// --------------------------------------------------------------
//
// WBrowserMaster
//
// --------------------------------------------------------------

class WBrowserMaster : public WBrowser
{
	DECLARE_WINDOWCLASS(WBrowserMaster,WBrowser,Window)

	WBrowser** Browsers[eBROWSER_MAX];
	WTabControl* BrowserTabs;
	int CurrentBrowser;

	// Structors.
	WBrowserMaster( FName InPersistentName, WWindow* InOwnerWindow )
		: WBrowser(InPersistentName, InOwnerWindow, NULL)
	{
		for (int x = 0; x < eBROWSER_MAX; x++)
			Browsers[x] = NULL;
		CurrentBrowser = -1;
		BrowserTabs = NULL;
		Description = TEXT("Browsers");
	}

	// WBrowser interface.
	void ShowBrowser( int InBrowser )
	{
		guard(WBrowserMaster::ShowBrowser);

		check(Browsers[InBrowser]);
		check(*Browsers[InBrowser]);

		CurrentBrowser = InBrowser;

		(*Browsers[InBrowser])->Show(1);
		::BringWindowToTop( (*Browsers[InBrowser])->hWnd );

		if( (*Browsers[InBrowser])->IsDocked() )
		{
			UpdateBrowserPositions();
			RefreshBrowserTabs( InBrowser );
			(*Browsers[InBrowser])->SetCaption();
			Show(1);
			::BringWindowToTop(hWnd);
		}

		unguard;
	}
	void UpdateBrowserPositions()
	{
		guard(WBrowserMaster::UpdateBrowserPositions);

		RECT rect;
		::GetClientRect( hWnd, &rect );

		for( int x = 0 ; x < eBROWSER_MAX ; x++ )
		{
			// If the browser is docked, we need to fit it inside of our client area.
			if( Browsers[x] && *Browsers[x] && (*Browsers[x])->IsDocked() )
				::MoveWindow( (*Browsers[x])->hWnd, 4, 32, rect.right - 8 , rect.bottom - 36, 1);
		}
		unguard;
	}
	void OnCreate()
	{
		guard(WBrowserMaster::OnCreate);
		WBrowser::OnCreate();

		BrowserTabs = new WTabControl( this, IDCB_BROWSER );
		BrowserTabs->OpenWindow( 1 );
		BrowserTabs->SelectionChangeDelegate = FDelegate(this, (TDelegate)&WBrowserMaster::OnBrowserTabSelChange);

		PositionChildControls();

		unguard;
	}
	void OnDestroy()
	{
		guard(WBrowserMaster::OnDestroy);
		WBrowser::OnDestroy();
		delete BrowserTabs;
		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WBrowserMaster::OnSize);
		WBrowser::OnSize(Flags, NewX, NewY);
		PositionChildControls();
		UpdateBrowserPositions();
		InvalidateRect( hWnd, NULL, 1 );
		unguard;
	}
	void PositionChildControls( void )
	{
		guard(WBrowserMaster::PositionChildControls);

		if( !BrowserTabs ) return;

		RECT rect;
		::GetClientRect( *this, &rect );
		::MoveWindow( BrowserTabs->hWnd, 0, 0, rect.right, rect.bottom, 1 );

		::InvalidateRect( hWnd, NULL, 1);

		unguard;
	}
	virtual bool ShouldBlockAccelerators()
	{
		for (int x = 0; x < eBROWSER_MAX; x++)
			if (Browsers[x] && *Browsers[x] && (*Browsers[x])->ShouldBlockAccelerators())
				return true;
		return false;
	}
	// Check to see how many browsers are docked and create buttons for them.
	void RefreshBrowserTabs( int InBrowser )
	{
		guard(WBrowserMaster::RefreshBrowserTabs);

		SetRedraw(false);

		BrowserTabs->Empty();

		for( int x = 0 ; x < eBROWSER_MAX ; x++ )
			if( Browsers[x] && *Browsers[x] && (*Browsers[x])->IsDocked() )
				BrowserTabs->AddTab( *(*Browsers[x])->Description, (*Browsers[x])->BrowserID );

		SetRedraw(true);

		if( !BrowserTabs->GetCount() )
		{
			HMENU CurrentMenu = GetMenu( hWnd );
			DestroyMenu( CurrentMenu );
			SetMenu( hWnd, NULL );
			Show(0);
			BringWindowToTop(OwnerWindow->hWnd); // GEditorFrame
		}
		else
		{
			if( InBrowser != -1 )
			{
				if( Browsers[InBrowser] && *Browsers[InBrowser] && (*Browsers[InBrowser])->IsDocked() )
					BrowserTabs->SetCurrent( BrowserTabs->GetIndexFromlParam( (*Browsers[InBrowser])->BrowserID ) );

				HMENU CurrentMenu = GetMenu( hWnd );
				DestroyMenu( CurrentMenu );
				SetMenu( hWnd, AddHotkeys(LoadMenuW(hInstance, MAKEINTRESOURCEW((*Browsers[InBrowser])->MenuID))) );
				(*Browsers[InBrowser])->UpdateMenu();
			}
			else
			{
				BrowserTabs->SetCurrent(0);
				ShowBrowser(BrowserTabs->GetlParam(0));
			}
		}

		unguard;
	}
	virtual void RefreshAll()
	{
		guard(WBrowserMaster::RefreshAll);
		SetRedraw(FALSE);
		for( int x = 0 ; x < eBROWSER_MAX ; x++ )
			if( Browsers[x] && *Browsers[x] )
				(*Browsers[x])->RefreshAll();
		SetRedraw(TRUE);
		unguard;
	}
	int FindBrowserIdxFromName( FString InDesc )
	{
		guard(WBrowserMaster::FindBrowserIdxFromName);
		for( int x = 0 ; x < eBROWSER_MAX ; x++ )
			if( Browsers[x] && *Browsers[x] && (*Browsers[x])->Description == InDesc )
				return x;

		return 0;
		unguard;
	}
	void OnCommand( INT Command )
	{
		guard(WBrowserMaster::OnCommand);
		// If we don't want to deal with this message, pass it to the currently active browser.
		if( CurrentBrowser > -1 && Browsers[CurrentBrowser] && (*Browsers[CurrentBrowser])->IsDocked() )
			SendMessageW( (*Browsers[CurrentBrowser])->hWnd, WM_COMMAND, Command, 0);
		else
			WBrowser::OnCommand(Command);
		unguard;
	}
	int GetCurrent()
	{
		guard(WBrowserMaster::GetCurrent);
		return CurrentBrowser;
		unguard;
	}
	void OnBrowserTabSelChange()
	{
		guard(WBrowserMaster::OnBrowserTabSelChange);
		ShowBrowser( BrowserTabs->GetlParam( BrowserTabs->GetCurrent() ) );
		unguard;
	}
	FString GetTextureBrowserRenderDevice()
	{
		guard(WBrowserMaster::GetTextureBrowserRenderDevice);
		if (!Browsers[eBROWSER_TEXTURE])
			return DEFAULT_UED_RENDERER_PATHNAME;
		FString Device;
		GConfig->GetString(*(*Browsers[eBROWSER_TEXTURE])->PersistentName, TEXT("Device"), Device, GUEDIni);
		return Device;
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
