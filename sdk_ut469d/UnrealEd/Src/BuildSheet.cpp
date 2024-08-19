/*=============================================================================
	TBuildSheet : Property sheet for map rebuild options/stats
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:
	- remove the OK and Cancel buttons off the bottom of the window and resize
	  the window accordingly

=============================================================================*/

#include "BuildSheet.h"
#include "res/resource.h"
#include <commctrl.h>

#include "..\..\Editor\Src\EditorPrivate.h"
EDITOR_API extern class UEditorEngine* GEditor;

INT_PTR APIENTRY OptionsProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR APIENTRY StatsProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

extern TBuildSheet* GBuildSheet;

HWND GhwndBSPages[eBS_MAX];
static INT ActivePage = 0;

TBuildSheet::TBuildSheet()
{
	m_bShow = FALSE;
	m_hwndSheet = NULL;
}

TBuildSheet::~TBuildSheet()
{
}

void TBuildSheet::OpenWindow( HINSTANCE hInst, HWND hWndOwner )
{
	// Options page
	//
	m_pages[eBS_OPTIONS].dwSize = sizeof(PROPSHEETPAGE);
	m_pages[eBS_OPTIONS].dwFlags = PSP_USETITLE;
	m_pages[eBS_OPTIONS].hInstance = hInst;
	m_pages[eBS_OPTIONS].pszTemplate = MAKEINTRESOURCE(IDPP_BUILD_OPTIONS);
	m_pages[eBS_OPTIONS].pszIcon = NULL;
	m_pages[eBS_OPTIONS].pfnDlgProc = OptionsProc;
	m_pages[eBS_OPTIONS].pszTitle = TEXT("Options");
	m_pages[eBS_OPTIONS].lParam = 0;
	GhwndBSPages[eBS_OPTIONS] = NULL;

	// Stats page
	//
	m_pages[eBS_STATS].dwSize = sizeof(PROPSHEETPAGE);
	m_pages[eBS_STATS].dwFlags = PSP_USETITLE;
	m_pages[eBS_STATS].hInstance = hInst;
	m_pages[eBS_STATS].pszTemplate = MAKEINTRESOURCE(IDPP_BUILD_STATS);
	m_pages[eBS_STATS].pszIcon = NULL;
	m_pages[eBS_STATS].pfnDlgProc = StatsProc;
	m_pages[eBS_STATS].pszTitle = TEXT("Stats");
	m_pages[eBS_STATS].lParam = 0;
	GhwndBSPages[eBS_STATS] = NULL;

	// Property sheet
	//
	m_psh.dwSize = sizeof(PROPSHEETHEADER);
	m_psh.dwFlags = PSH_PROPSHEETPAGE | PSH_MODELESS | PSH_NOAPPLYNOW;
	m_psh.hwndParent = hWndOwner;
	m_psh.hInstance = hInst;
	m_psh.pszIcon = NULL;
	m_psh.pszCaption = TEXT("Build");
	m_psh.nPages = eBS_MAX;
	m_psh.ppsp = (LPCPROPSHEETPAGE)&m_pages;

	m_hwndSheet = (HWND)PropertySheet( &m_psh );
	m_bShow = TRUE;

	// Customize the property sheet by deleting the OK button and changing the "Cancel" button to "Hide".
	DestroyWindow( GetDlgItem( m_hwndSheet, IDOK ) );
	SendMessageW( GetDlgItem( m_hwndSheet, IDCANCEL ), WM_SETTEXT, 0, (LPARAM)(LPCTSTR)TEXT("&Hide"));
}

void TBuildSheet::Show( BOOL bShow )
{
	if( !m_hwndSheet || m_bShow == bShow )
	{
		if (bShow && m_hwndSheet)
			BringWindowToTop(m_hwndSheet);
		return;
	}

	ShowWindow(m_hwndSheet, bShow ? SW_SHOW : SW_HIDE);

	// stijn: workaround for weird WIN32 bug
	if (bShow)
	{
		INT Tmp = ActivePage;

		for (INT i = 0; i < eBS_MAX; ++i)
			if (GhwndBSPages[i])
				PropSheet_SetCurSel(m_hwndSheet, GhwndBSPages[i], i);
				
		if (Tmp >= 0 && Tmp < eBS_MAX)
			PropSheet_SetCurSel(m_hwndSheet, GhwndBSPages[Tmp], Tmp);
	}	

	m_bShow = bShow;

	if (bShow)
		BringWindowToTop(m_hwndSheet);
}

#if UNREAL_TOURNAMENT_OLDUNREAL
static INT PrevBuildOpt = (2 << 16) | (70 << 8) | 15;
#endif

static INT* GetSavedBuildOpt()
{
	return 
#if ENGINE_VERSION==227
		if (GEditor && GEditor->Level) ? &GEditor->Level->GetLevelInfo()->EdBuildOpt :
#endif
			&PrevBuildOpt;
}

void TBuildSheet::Build()
{
	FString Cmd;
	UBOOL bVisibleOnly = SendMessageA(::GetDlgItem( GhwndBSPages[eBS_OPTIONS], IDCK_ONLY_VISIBLE), BM_GETCHECK, 0, 0 ) == BST_CHECKED;

	INT* EdBuildOpt = GetSavedBuildOpt();	
	INT Cuts = *EdBuildOpt & 0xFF;
	INT Portals = (*EdBuildOpt >> 8) & 0xFF;	
	INT Opt = (*EdBuildOpt >> 16) & 0x03;
	
	GEditor->Exec( TEXT("HIDELOG") );

	// GEOMETRY
	if( SendMessageA( ::GetDlgItem( GhwndBSPages[eBS_OPTIONS], IDCK_GEOMETRY), BM_GETCHECK, 0, 0 ) == BST_CHECKED )
	{
		FString Cmd = FString::Printf(TEXT("MAP REBUILD VISIBLEONLY=%d"), bVisibleOnly );
		GEditor->Exec( *Cmd );
	}

	// BSP
	if( SendMessageA( ::GetDlgItem( GhwndBSPages[eBS_OPTIONS], IDCK_BSP), BM_GETCHECK, 0, 0 ) == BST_CHECKED )
	{
		Cmd = TEXT("BSP REBUILD");
		if ((SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_LAME), BM_GETCHECK, 0, 0) == BST_CHECKED))
		{
			Cmd += TEXT(" LAME");
			Opt = 0;
		}
		if ((SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_GOOD), BM_GETCHECK, 0, 0) == BST_CHECKED))
		{
			Cmd += TEXT(" GOOD");
			Opt = 1;
		}
		if ((SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_OPTIMAL), BM_GETCHECK, 0, 0) == BST_CHECKED))
		{
			Cmd += TEXT(" OPTIMAL");
			Opt = 2;
		}
		if( (SendMessageA( ::GetDlgItem( GhwndBSPages[eBS_OPTIONS], IDCK_OPT_GEOM), BM_GETCHECK, 0, 0 ) == BST_CHECKED ) )
			Cmd += TEXT(" OPTGEOM");
		if( (SendMessageA( ::GetDlgItem( GhwndBSPages[eBS_OPTIONS], IDCK_BUILD_VIS_ZONES), BM_GETCHECK, 0, 0 ) == BST_CHECKED ) )
			Cmd += TEXT(" ZONES");
		Cmd += FString::Printf(TEXT(" BALANCE=%d"), SendMessageA( GetDlgItem( GhwndBSPages[eBS_OPTIONS], IDSL_BALANCE), TBM_GETPOS, 0, 0 ) );
		Cmd += FString::Printf(TEXT(" PORTALBIAS=%d"), SendMessageA( GetDlgItem( GhwndBSPages[eBS_OPTIONS], IDSL_PORTALBIAS), TBM_GETPOS, 0, 0 ) );

		*EdBuildOpt = Cuts | (Portals << 8) | (Opt << 16);	
		GEditor->Exec( *Cmd );
	}

	// LIGHTING
	if( SendMessageA( ::GetDlgItem( GhwndBSPages[eBS_OPTIONS], IDCK_LIGHTING), BM_GETCHECK, 0, 0 ) == BST_CHECKED )
	{
		UBOOL bSelected = SendMessageA(::GetDlgItem( GhwndBSPages[eBS_OPTIONS], IDCK_SEL_LIGHTS_ONLY), BM_GETCHECK, 0, 0 ) == BST_CHECKED;
		GEditor->Exec( *(FString::Printf(TEXT("LIGHT APPLY SELECTED=%d VISIBLEONLY=%d"), bSelected, bVisibleOnly) ) );
	}

	// PATHS
	if( SendMessageA( ::GetDlgItem( GhwndBSPages[eBS_OPTIONS], IDCK_PATH_DEFINE), BM_GETCHECK, 0, 0 ) == BST_CHECKED )
		GEditor->Exec( TEXT("PATHS DEFINE") );

	RefreshStats();
}

void TBuildSheet::BuildBSP()
{
	FString Cmd;
	INT* EdBuildOpt = GetSavedBuildOpt();
	INT Cuts = *EdBuildOpt & 0xFF;
	INT Portals = (*EdBuildOpt >> 8) & 0xFF;
	INT Opt = (*EdBuildOpt >> 16) & 0x03;
	
	GEditor->Exec( TEXT("HIDELOG") );

	// BSP
	if (SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDCK_BSP), BM_GETCHECK, 0, 0) == BST_CHECKED)
	{
		Cmd = TEXT("BSP REBUILD");
		if ((SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_LAME), BM_GETCHECK, 0, 0) == BST_CHECKED))
		{
			Cmd += TEXT(" LAME");
			Opt = 0;
		}
		if ((SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_GOOD), BM_GETCHECK, 0, 0) == BST_CHECKED))
		{
			Cmd += TEXT(" GOOD");
			Opt = 1;
		}
		if ((SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_OPTIMAL), BM_GETCHECK, 0, 0) == BST_CHECKED))
		{
			Cmd += TEXT(" OPTIMAL");
			Opt = 2;
		}
		if ((SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDCK_OPT_GEOM), BM_GETCHECK, 0, 0) == BST_CHECKED))
			Cmd += TEXT(" OPTGEOM");
		if ((SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDCK_BUILD_VIS_ZONES), BM_GETCHECK, 0, 0) == BST_CHECKED))
			Cmd += TEXT(" ZONES");
		Cmd += FString::Printf(TEXT(" BALANCE=%d"), SendMessageA(GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDSL_BALANCE), TBM_GETPOS, 0, 0));
		Cmd += FString::Printf(TEXT(" PORTALBIAS=%d"), SendMessageA(GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDSL_PORTALBIAS), TBM_GETPOS, 0, 0));

		*EdBuildOpt = Cuts | (Portals << 8) | (Opt << 16);
		GEditor->Exec(*Cmd);
	}

	RefreshStats();
}

void TBuildSheet::RefreshStats()
{
	// GEOMETRY
	FStringOutputDevice GetPropResult = FStringOutputDevice();

	GetPropResult.Empty();	GEditor->Get( TEXT("MAP"), TEXT("BRUSHES"), GetPropResult );
	SendMessageW( ::GetDlgItem( GhwndBSPages[eBS_STATS], IDSC_BRUSHES), WM_SETTEXT, 0, (LPARAM)*GetPropResult );
	GetPropResult.Empty();	GEditor->Get( TEXT("MAP"), TEXT("ZONES"), GetPropResult );
	SendMessageW( ::GetDlgItem( GhwndBSPages[eBS_STATS], IDSC_ZONES), WM_SETTEXT, 0, (LPARAM)*GetPropResult );

	// BSP
	int iPolys, iNodes;
	GetPropResult.Empty();	GEditor->Get( TEXT("BSP"), TEXT("POLYS"), GetPropResult );	iPolys = appAtoi( *GetPropResult );
	SendMessageW( ::GetDlgItem( GhwndBSPages[eBS_STATS], IDSC_POLYS), WM_SETTEXT, 0, (LPARAM)*GetPropResult );
	GetPropResult.Empty();	GEditor->Get( TEXT("BSP"), TEXT("NODES"), GetPropResult );	iNodes = appAtoi( *GetPropResult );
	SendMessageW( ::GetDlgItem( GhwndBSPages[eBS_STATS], IDSC_NODES), WM_SETTEXT, 0, (LPARAM)*GetPropResult );
	GetPropResult.Empty();	GEditor->Get( TEXT("BSP"), TEXT("MAXDEPTH"), GetPropResult );
	SendMessageW( ::GetDlgItem( GhwndBSPages[eBS_STATS], IDSC_MAX_DEPTH), WM_SETTEXT, 0, (LPARAM)*GetPropResult );
	GetPropResult.Empty();	GEditor->Get( TEXT("BSP"), TEXT("AVGDEPTH"), GetPropResult );
	SendMessageW( ::GetDlgItem( GhwndBSPages[eBS_STATS], IDSC_AVG_DEPTH), WM_SETTEXT, 0, (LPARAM)*GetPropResult );

	if(!iPolys)
		SendMessageA( ::GetDlgItem( GhwndBSPages[eBS_STATS], IDSC_RATIO), WM_SETTEXT, 0, (LPARAM)TEXT("N/A") );
	else
	{
		float fRatio = (iNodes / (float)iPolys);
		SendMessageW( ::GetDlgItem( GhwndBSPages[eBS_STATS], IDSC_RATIO), WM_SETTEXT, 0, (LPARAM) *FString::Printf(TEXT("%1.2f:1"), fRatio) );
	}

	GetPropResult.Empty();	GEditor->Get(TEXT("BSP"), TEXT("POINTS"), GetPropResult);
	SendMessageW(::GetDlgItem(GhwndBSPages[eBS_STATS], IDSC_POINTS), WM_SETTEXT, 0, (LPARAM)*GetPropResult);

	// LIGHTING
	GetPropResult.Empty();	GEditor->Get( TEXT("LIGHT"), TEXT("COUNT"), GetPropResult );
	SendMessageW( ::GetDlgItem( GhwndBSPages[eBS_STATS], IDSC_LIGHTS), WM_SETTEXT, 0, (LPARAM)*GetPropResult );
	
	// PATHS
	SendMessageW( ::GetDlgItem( GhwndBSPages[eBS_STATS], IDSC_PATHS), WM_SETTEXT, 0, (LPARAM) *FString::Printf(TEXT("%i"), GEditor->Level->ReachSpecs.Num()) );

	//Build options
	INT* EdBuildOpt = GetSavedBuildOpt();
	INT Cuts = *EdBuildOpt & 0xFF;
	INT Portals = (*EdBuildOpt >> 8) & 0xFF;
	INT Opt = (*EdBuildOpt >> 16) & 0x03;
	char buffer[10];
	
	::itoa(Cuts, buffer, sizeof(buffer));
	SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDSL_BALANCE), TBM_SETPOS, 1, Cuts);
	SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDSC_BALANCE), WM_SETTEXT, 0, (LPARAM)(LPCTSTR)buffer);

	::itoa(Portals, buffer, sizeof(buffer));
	SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDSL_PORTALBIAS), TBM_SETPOS, 1, Portals);
	SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDSC_PORTALBIAS), WM_SETTEXT, 0, (LPARAM)(LPCTSTR)buffer);

	if (Opt == 0)
	{
		SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_LAME), BM_SETCHECK, BST_CHECKED, 0);
		SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_GOOD), BM_SETCHECK, BST_UNCHECKED, 0);
		SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_OPTIMAL), BM_SETCHECK, BST_UNCHECKED, 0);
	}
	else if (Opt == 1)
	{
		SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_LAME), BM_SETCHECK, BST_UNCHECKED, 0);
		SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_GOOD), BM_SETCHECK, BST_CHECKED, 0);
		SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_OPTIMAL), BM_SETCHECK, BST_UNCHECKED, 0);
	}
	else
	{
		SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_LAME), BM_SETCHECK, BST_UNCHECKED, 0);
		SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_GOOD), BM_SETCHECK, BST_UNCHECKED, 0);
		SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_OPTIMAL), BM_SETCHECK, BST_CHECKED, 0);
	}

	GEditor->Flush(0);
}

// --------------------------------------------------------------
//
// OPTIONS
//
// --------------------------------------------------------------

static BOOL bFirstTimeBuildSheetOptionsProc = TRUE;

INT_PTR APIENTRY OptionsProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	try
	{
		INT* EdBuildOpt = GetSavedBuildOpt();
		INT Cuts = *EdBuildOpt & 0xFF;
		INT Portals = (*EdBuildOpt >> 8) & 0xFF;
		INT Opt = (*EdBuildOpt >> 16) & 0x03;

		switch (message)
		{
		case WM_NOTIFY:

			switch (((NMHDR FAR*)lParam)->code)
			{
			case PSN_SETACTIVE:

				if (bFirstTimeBuildSheetOptionsProc)
				{
					bFirstTimeBuildSheetOptionsProc = FALSE;
					GhwndBSPages[eBS_OPTIONS] = hDlg;

					bFirstTimeBuildSheetOptionsProc = FALSE;
					char buffer[10];
					GhwndBSPages[eBS_OPTIONS] = hDlg;

					SendMessageA(GetDlgItem(hDlg, IDCK_GEOMETRY), BM_SETCHECK, BST_CHECKED, 0);
					SendMessageA(GetDlgItem(hDlg, IDCK_BSP), BM_SETCHECK, BST_CHECKED, 0);
					SendMessageA(GetDlgItem(hDlg, IDCK_LIGHTING), BM_SETCHECK, BST_CHECKED, 0);
					SendMessageA(GetDlgItem(hDlg, IDCK_PATH_DEFINE), BM_SETCHECK, BST_CHECKED, 0);

					if (Opt == 0)
						SendMessageA(GetDlgItem(hDlg, IDRB_LAME), BM_SETCHECK, BST_CHECKED, 0);
					else if (Opt == 1)
						SendMessageA(GetDlgItem(hDlg, IDRB_GOOD), BM_SETCHECK, BST_CHECKED, 0);
					else
						SendMessageA(GetDlgItem(hDlg, IDRB_OPTIMAL), BM_SETCHECK, BST_CHECKED, 0);
					SendMessageA(GetDlgItem(hDlg, IDCK_OPT_GEOM), BM_SETCHECK, BST_CHECKED, 0);
					SendMessageA(GetDlgItem(hDlg, IDCK_BUILD_VIS_ZONES), BM_SETCHECK, BST_CHECKED, 0);

					::itoa(Cuts, buffer, sizeof(buffer));
					SendMessageA(GetDlgItem(hDlg, IDSL_BALANCE), TBM_SETRANGE, 1, MAKELONG(0, 100));
					SendMessageA(GetDlgItem(hDlg, IDSL_BALANCE), TBM_SETTICFREQ, 10, 0);
					SendMessageA(GetDlgItem(hDlg, IDSL_BALANCE), TBM_SETPOS, 1, Cuts);
					SendMessageA(GetDlgItem(hDlg, IDSC_BALANCE), WM_SETTEXT, 0, (LPARAM)(LPCTSTR)buffer);

					::itoa(Portals, buffer, sizeof(buffer));
					SendMessageA(GetDlgItem(hDlg, IDSL_PORTALBIAS), TBM_SETRANGE, 1, MAKELONG(0, 100));
					SendMessageA(GetDlgItem(hDlg, IDSL_PORTALBIAS), TBM_SETTICFREQ, 5, 0);
					SendMessageA(GetDlgItem(hDlg, IDSL_PORTALBIAS), TBM_SETPOS, 1, Portals);
					SendMessageA(GetDlgItem(hDlg, IDSC_PORTALBIAS), WM_SETTEXT, 0, (LPARAM)(LPCTSTR)buffer);

				}
				ActivePage = eBS_OPTIONS;

				break;

			case PSN_QUERYCANCEL:
				GBuildSheet->Show(0);
				SetWindowLongPtr(GhwndBSPages[eBS_OPTIONS], DWLP_MSGRESULT, TRUE);
				// MSDN: If you have changed certain window data using SetWindowLong, you must call SetWindowPos for the changes to take effect. Use the following combination for uFlags: SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED.
				SetWindowPos(GhwndBSPages[eBS_OPTIONS], HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
				return TRUE;
			}
			break;

		case WM_HSCROLL:
		{
			char buffer[10];
			if ((HWND)lParam == GetDlgItem(hDlg, IDSL_BALANCE))
			{

				INT Cuts = SendMessageA(GetDlgItem(hDlg, IDSL_BALANCE), TBM_GETPOS, 0, 0);
				::itoa(Cuts, buffer, sizeof(buffer));
				*EdBuildOpt = Cuts | (Portals << 8) | (Opt << 16);
			
				SendMessageA(GetDlgItem(hDlg, IDSC_BALANCE), WM_SETTEXT, 0, (LPARAM)(LPCTSTR)buffer);
			}
			if ((HWND)lParam == GetDlgItem(hDlg, IDSL_PORTALBIAS))
			{
				INT Portals = SendMessageA(GetDlgItem(hDlg, IDSL_PORTALBIAS), TBM_GETPOS, 0, 0);
				::itoa(Portals, buffer, sizeof(buffer));
				*EdBuildOpt = Cuts | (Portals << 8) | (Opt << 16);
			
				SendMessageA(GetDlgItem(hDlg, IDSC_PORTALBIAS), WM_SETTEXT, 0, (LPARAM)(LPCTSTR)buffer);
			}
		}
		break;

		case WM_COMMAND:

			switch (HIWORD(wParam)) {

			case BN_CLICKED:

				switch (LOWORD(wParam)) {

				case IDCK_GEOMETRY:
				{
					BOOL bChecked = (SendMessageA(GetDlgItem(hDlg, IDCK_GEOMETRY), BM_GETCHECK, 0, 0) == BST_CHECKED);

					EnableWindow(GetDlgItem(hDlg, IDCK_ONLY_VISIBLE), bChecked);
				}
				break;

				case IDCK_BSP:
				{
					BOOL bChecked = (SendMessageA(GetDlgItem(hDlg, IDCK_BSP), BM_GETCHECK, 0, 0) == BST_CHECKED);

					EnableWindow(GetDlgItem(hDlg, IDSC_OPTIMIZATION), bChecked);
					EnableWindow(GetDlgItem(hDlg, IDRB_LAME), bChecked);
					EnableWindow(GetDlgItem(hDlg, IDRB_GOOD), bChecked);
					EnableWindow(GetDlgItem(hDlg, IDRB_OPTIMAL), bChecked);
					EnableWindow(GetDlgItem(hDlg, IDCK_OPT_GEOM), bChecked);
					EnableWindow(GetDlgItem(hDlg, IDCK_BUILD_VIS_ZONES), bChecked);
					EnableWindow(GetDlgItem(hDlg, IDSL_BALANCE), bChecked);
					EnableWindow(GetDlgItem(hDlg, IDSC_BSP_1), bChecked);
					EnableWindow(GetDlgItem(hDlg, IDSC_BSP_2), bChecked);
					EnableWindow(GetDlgItem(hDlg, IDSC_BALANCE), bChecked);
				}
				break;

				case IDCK_LIGHTING:
				{
					BOOL bChecked = (SendMessageA(GetDlgItem(hDlg, IDCK_LIGHTING), BM_GETCHECK, 0, 0) == BST_CHECKED);

					EnableWindow(GetDlgItem(hDlg, IDCK_SEL_LIGHTS_ONLY), bChecked);
					EnableWindow(GetDlgItem(hDlg, IDSC_RESOLUTION), bChecked);
				}
				break;

				case IDPB_BUILD:
					GBuildSheet->Build();
					break;

				case IDPB_BUILD_PATHS:
				{
					if (::MessageBox(hDlg, TEXT("This command will erase all existing pathnodes and attempt to create a pathnode network on its own.  Are you sure this is what you want to do?\n\nNOTE : This process can take a VERY long time."), TEXT("Build Paths"), MB_YESNO) == IDYES)
					{
						GEditor->Exec(TEXT("PATHS BUILD"));
					}
				}
				break;
				}
			}
			break;
		}
	}
	catch(...)
	{
		// stijn: eat errors here so we don't throw in a windowproc
	}

	return 0;
}

// --------------------------------------------------------------
//
// STATS
//
// --------------------------------------------------------------

static BOOL bFirstTimeBuildSheetStatsProc = TRUE;
INT_PTR APIENTRY StatsProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{	
	try
	{
		switch (message)
		{
		case WM_NOTIFY:

			switch (((NMHDR FAR*)lParam)->code)
			{
			case PSN_SETACTIVE:

				if (bFirstTimeBuildSheetStatsProc) {

					bFirstTimeBuildSheetStatsProc = FALSE;
					GhwndBSPages[eBS_STATS] = hDlg;
					GBuildSheet->RefreshStats();
				}
				ActivePage = eBS_STATS;
				break;


			case PSN_QUERYCANCEL:
				GBuildSheet->Show(0);
				SetWindowLongPtr(GhwndBSPages[eBS_STATS], DWLP_MSGRESULT, TRUE);
				// MSDN: If you have changed certain window data using SetWindowLong, you must call SetWindowPos for the changes to take effect. Use the following combination for uFlags: SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED.
				SetWindowPos(GhwndBSPages[eBS_STATS], HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
				return TRUE;
			}
			break;

		case WM_COMMAND:

			switch (HIWORD(wParam)) {

			case BN_CLICKED:

				switch (LOWORD(wParam)) {

				case IDPB_REFRESH:
					GBuildSheet->RefreshStats();
					break;

				case IDPB_BUILD:
					GBuildSheet->Build();
					break;
				}
			}
			break;
		}
	}
	catch (...)
	{
		// stijn: eat errors here so we don't throw in a windowproc
	}

	return 0;
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
