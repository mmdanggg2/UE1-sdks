/*=============================================================================
	SurfPropSheet.h : Surface Properties
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include <windows.h>
#include <commctrl.h>

enum eSPS {
	eSPS_FLAGS1		= 0,
	//eSPS_FLAGS2		= 1,
	eSPS_ALIGNMENT	= 1,
	eSPS_STATS		= 2,
	eSPS_MAX		= 3
};

class TSurfPropSheet
{
public:
	TSurfPropSheet();
	~TSurfPropSheet();

	void OpenWindow( HINSTANCE hInst, HWND hWndOwner );
	void Show( BOOL bShow );
	void GetDataFromSurfs1();
	void RefreshStats();

	PROPSHEETPAGE m_pages[eSPS_MAX]{};
    PROPSHEETHEADER m_psh{};
	HWND m_hwndSheet;
	BOOL m_bShow;
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
