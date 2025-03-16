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
	SetWindowPos(m_hwndSheet, 0, 160, 160, 0, 0, SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOOWNERZORDER);

	// Customize the property sheet by deleting the OK, Cancel button and resize form.
	DestroyWindow( GetDlgItem( m_hwndSheet, IDOK ) );
	HWND hwndCancel = GetDlgItem(m_hwndSheet, IDCANCEL);
	RECT rectCancel, rectSheet;
	if (GetWindowRect(hwndCancel, &rectCancel) && GetWindowRect(m_hwndSheet, &rectSheet))
		MoveWindow(m_hwndSheet, rectSheet.left, rectSheet.top, rectSheet.right - rectSheet.left, rectCancel.top - rectSheet.top, TRUE);
	DestroyWindow(hwndCancel);
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
	UBOOL bVisibleOnly = SendMessageW(::GetDlgItem( GhwndBSPages[eBS_OPTIONS], IDCK_ONLY_VISIBLE), BM_GETCHECK, 0, 0 ) == BST_CHECKED;
	UBOOL bRebuildMovers = SendMessageW( ::GetDlgItem( GhwndBSPages[eBS_OPTIONS], IDCK_REBUILD_MOVERS), BM_GETCHECK, 0, 0 ) == BST_CHECKED;

	INT* EdBuildOpt = GetSavedBuildOpt();	
	INT Cuts = *EdBuildOpt & 0xFF;
	INT Portals = (*EdBuildOpt >> 8) & 0xFF;	
	INT Opt = (*EdBuildOpt >> 16) & 0x03;

	DOUBLE Time = appSecondsNew();

	GWarn->BeginSlowTask(TEXT("Map Build"), 1, 0);
	
	GEditor->Exec( TEXT("HIDELOG") );

	UBOOL Cancelled = FALSE;

	// GEOMETRY
	if( SendMessageW( ::GetDlgItem( GhwndBSPages[eBS_OPTIONS], IDCK_GEOMETRY), BM_GETCHECK, 0, 0 ) == BST_CHECKED )
	{
		FString Cmd = FString::Printf(TEXT("MAP REBUILD VISIBLEONLY=%d REBUILDMOVERS=%d"), bVisibleOnly, bRebuildMovers );
		Cancelled = !GEditor->Exec( *Cmd );
	}

	if (!Cancelled)
	{
		// BSP
		if( SendMessageW( ::GetDlgItem( GhwndBSPages[eBS_OPTIONS], IDCK_BSP), BM_GETCHECK, 0, 0 ) == BST_CHECKED )
		{
			Cmd = TEXT("BSP REBUILD");
			if ((SendMessageW(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_LAME), BM_GETCHECK, 0, 0) == BST_CHECKED))
			{
				Cmd += TEXT(" LAME");
				Opt = 0;
			}
			if ((SendMessageW(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_GOOD), BM_GETCHECK, 0, 0) == BST_CHECKED))
			{
				Cmd += TEXT(" GOOD");
				Opt = 1;
			}
			if ((SendMessageW(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_OPTIMAL), BM_GETCHECK, 0, 0) == BST_CHECKED))
			{
				Cmd += TEXT(" OPTIMAL");
				Opt = 2;
			}
			if( (SendMessageW( ::GetDlgItem( GhwndBSPages[eBS_OPTIONS], IDCK_OPT_GEOM), BM_GETCHECK, 0, 0 ) == BST_CHECKED ) )
				Cmd += TEXT(" OPTGEOM");
			if( (SendMessageW( ::GetDlgItem( GhwndBSPages[eBS_OPTIONS], IDCK_BUILD_VIS_ZONES), BM_GETCHECK, 0, 0 ) == BST_CHECKED ) )
				Cmd += TEXT(" ZONES");
			Cmd += FString::Printf(TEXT(" BALANCE=%d"), SendMessageW( GetDlgItem( GhwndBSPages[eBS_OPTIONS], IDSL_BALANCE), TBM_GETPOS, 0, 0 ) );
			Cmd += FString::Printf(TEXT(" PORTALBIAS=%d"), SendMessageW( GetDlgItem( GhwndBSPages[eBS_OPTIONS], IDSL_PORTALBIAS), TBM_GETPOS, 0, 0 ) );

			*EdBuildOpt = Cuts | (Portals << 8) | (Opt << 16);	
			GEditor->Exec( *Cmd );
		}

		// LIGHTING
		if( SendMessageW( ::GetDlgItem( GhwndBSPages[eBS_OPTIONS], IDCK_LIGHTING), BM_GETCHECK, 0, 0 ) == BST_CHECKED )
		{
			UBOOL bSelected = SendMessageW(::GetDlgItem( GhwndBSPages[eBS_OPTIONS], IDCK_SEL_LIGHTS_ONLY), BM_GETCHECK, 0, 0 ) == BST_CHECKED;
			GEditor->Exec( *(FString::Printf(TEXT("LIGHT APPLY SELECTED=%d VISIBLEONLY=%d"), bSelected, bVisibleOnly) ) );
		}

		// PATHS
		if( SendMessageW( ::GetDlgItem( GhwndBSPages[eBS_OPTIONS], IDCK_PATH_DEFINE), BM_GETCHECK, 0, 0 ) == BST_CHECKED )
			GEditor->Exec( TEXT("PATHS DEFINE") );
	}

	RefreshStats();

	GWarn->EndSlowTask();

	Time = appSecondsNew() - Time;
	debugf(TEXT("Total build time: %f seconds"), Time);
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
	if (SendMessageW(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDCK_BSP), BM_GETCHECK, 0, 0) == BST_CHECKED)
	{
		Cmd = TEXT("BSP REBUILD");
		if ((SendMessageW(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_LAME), BM_GETCHECK, 0, 0) == BST_CHECKED))
		{
			Cmd += TEXT(" LAME");
			Opt = 0;
		}
		if ((SendMessageW(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_GOOD), BM_GETCHECK, 0, 0) == BST_CHECKED))
		{
			Cmd += TEXT(" GOOD");
			Opt = 1;
		}
		if ((SendMessageW(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_OPTIMAL), BM_GETCHECK, 0, 0) == BST_CHECKED))
		{
			Cmd += TEXT(" OPTIMAL");
			Opt = 2;
		}
		if ((SendMessageW(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDCK_OPT_GEOM), BM_GETCHECK, 0, 0) == BST_CHECKED))
			Cmd += TEXT(" OPTGEOM");
		if ((SendMessageW(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDCK_BUILD_VIS_ZONES), BM_GETCHECK, 0, 0) == BST_CHECKED))
			Cmd += TEXT(" ZONES");
		Cmd += FString::Printf(TEXT(" BALANCE=%d"), SendMessageW(GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDSL_BALANCE), TBM_GETPOS, 0, 0));
		Cmd += FString::Printf(TEXT(" PORTALBIAS=%d"), SendMessageW(GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDSL_PORTALBIAS), TBM_GETPOS, 0, 0));

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
	GetPropResult.Empty();	GEditor->Get( TEXT("MAP"), TEXT("~ZONES"), GetPropResult );
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
		SendMessageW( ::GetDlgItem( GhwndBSPages[eBS_STATS], IDSC_RATIO), WM_SETTEXT, 0, (LPARAM)TEXT("N/A") );
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

	SetOptions(Cuts, Portals);

	if (Opt == 0)
	{
		SendMessageW(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_LAME), BM_SETCHECK, BST_CHECKED, 0);
		SendMessageW(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_GOOD), BM_SETCHECK, BST_UNCHECKED, 0);
		SendMessageW(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_OPTIMAL), BM_SETCHECK, BST_UNCHECKED, 0);
	}
	else if (Opt == 1)
	{
		SendMessageW(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_LAME), BM_SETCHECK, BST_UNCHECKED, 0);
		SendMessageW(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_GOOD), BM_SETCHECK, BST_CHECKED, 0);
		SendMessageW(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_OPTIMAL), BM_SETCHECK, BST_UNCHECKED, 0);
	}
	else
	{
		SendMessageW(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_LAME), BM_SETCHECK, BST_UNCHECKED, 0);
		SendMessageW(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_GOOD), BM_SETCHECK, BST_UNCHECKED, 0);
		SendMessageW(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_OPTIMAL), BM_SETCHECK, BST_CHECKED, 0);
	}

	GEditor->Flush(0);
}

void TBuildSheet::SetOptions(INT Cuts, INT Portals)
{
	INT* EdBuildOpt = GetSavedBuildOpt();
	Cuts = Cuts & 0xFF;
	Portals = Portals & 0xFF;
	INT Opt = (*EdBuildOpt >> 16) & 0x03;
	TCHAR buffer[10];

	appSnprintf(buffer, ARRAY_COUNT(buffer), TEXT("%i"), Cuts);
	SendMessage(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDSL_BALANCE), TBM_SETPOS, 1, Cuts);
	SendMessage(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDSC_BALANCE), WM_SETTEXT, 0, (LPARAM)(LPCTSTR)buffer);

	appSnprintf(buffer, ARRAY_COUNT(buffer), TEXT("%i"), Portals);
	SendMessage(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDSL_PORTALBIAS), TBM_SETPOS, 1, Portals);
	SendMessage(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDSC_PORTALBIAS), WM_SETTEXT, 0, (LPARAM)(LPCTSTR)buffer);

	*EdBuildOpt = Cuts | (Portals << 8) | (Opt << 16);
}

// --------------------------------------------------------------
//
// OPTIONS
//
// --------------------------------------------------------------

struct LightMapScaleItem
{
	INT Default;
	INT LabelId;
	INT TrackbarId;
	HWND Trackbar;
};

static LightMapScaleItem LightMapScale[] = {
	{4, IDSC_LIGHT_HIGH, IDSL_LIGHT_HIGH},
	{5, IDSC_LIGHT_NORMAL, IDSL_LIGHT_NORMAL},
	{6, IDSC_LIGHT_LOW, IDSL_LIGHT_LOW},
	{7, IDSC_LIGHT_SUPERLOW, IDSL_LIGHT_SUPERLOW},
};

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
					TCHAR buffer[10];
					GhwndBSPages[eBS_OPTIONS] = hDlg;

					SendMessageW(GetDlgItem(hDlg, IDCK_GEOMETRY), BM_SETCHECK, BST_CHECKED, 0);
					SendMessageW(GetDlgItem(hDlg, IDCK_REBUILD_MOVERS), BM_SETCHECK, BST_CHECKED, 0);
					SendMessageW(GetDlgItem(hDlg, IDCK_BSP), BM_SETCHECK, BST_CHECKED, 0);
					SendMessageW(GetDlgItem(hDlg, IDCK_LIGHTING), BM_SETCHECK, BST_CHECKED, 0);
					SendMessageW(GetDlgItem(hDlg, IDCK_PATH_DEFINE), BM_SETCHECK, BST_CHECKED, 0);

					if (Opt == 0)
						SendMessageW(GetDlgItem(hDlg, IDRB_LAME), BM_SETCHECK, BST_CHECKED, 0);
					else if (Opt == 1)
						SendMessageW(GetDlgItem(hDlg, IDRB_GOOD), BM_SETCHECK, BST_CHECKED, 0);
					else
						SendMessageW(GetDlgItem(hDlg, IDRB_OPTIMAL), BM_SETCHECK, BST_CHECKED, 0);
					SendMessageW(GetDlgItem(hDlg, IDCK_OPT_GEOM), BM_SETCHECK, BST_CHECKED, 0);
					SendMessageW(GetDlgItem(hDlg, IDCK_BUILD_VIS_ZONES), BM_SETCHECK, BST_CHECKED, 0);

					HWND Trackbar;
					Trackbar = GetDlgItem(hDlg, IDSL_BALANCE);
					SendMessageW(Trackbar, TBM_SETRANGE, 1, MAKELONG(0, 100));
					SendMessageW(Trackbar, TBM_SETTICFREQ, 10, 0);
					SendMessageW(Trackbar, TBM_SETTIC, 0, 15);

					Trackbar = GetDlgItem(hDlg, IDSL_PORTALBIAS);
					SendMessageW(Trackbar, TBM_SETRANGE, 1, MAKELONG(0, 100));
					SendMessageW(Trackbar, TBM_SETTICFREQ, 10, 0);
					SendMessageW(Trackbar, TBM_SETTIC, 0, 70);

					GBuildSheet->SetOptions(Cuts, Portals);

					for (INT i = 0; i < ARRAY_COUNT(LightMapScale); i++)
					{
						INT Value = LightMapScale[i].Default;
						GConfig->GetInt(TEXT("Options"), *FString::Printf(TEXT("LightMapScale[%d]"), i), Value, GUEDIni);
						Value = Clamp(Value, 0, 16);

						appSnprintf(buffer, ARRAY_COUNT(buffer), TEXT("%i"), Value);
						Trackbar = GetDlgItem(hDlg, LightMapScale[i].TrackbarId);
						LightMapScale[i].Trackbar = Trackbar;
						SendMessageW(Trackbar, TBM_SETRANGE, 1, MAKELONG(0, 16));
						SendMessageW(Trackbar, TBM_SETTICFREQ, 16, 0); // no auto tics
						SendMessageW(Trackbar, TBM_SETTIC, 0, LightMapScale[i].Default);
						SendMessageW(Trackbar, TBM_SETPOS, 1, Value);
						SendMessageW(GetDlgItem(hDlg, LightMapScale[i].LabelId), WM_SETTEXT, 0, (LPARAM)(LPCTSTR)buffer);
					}

					UBOOL UniformLightMapScale = FALSE;
					GConfig->GetBool(TEXT("Options"), TEXT("UniformLightMapScale"), UniformLightMapScale, GUEDIni);
					SendMessageW(GetDlgItem(hDlg, IDCK_UNIFORM_SHADOWS), BM_SETCHECK, UniformLightMapScale ? BST_CHECKED : BST_UNCHECKED, 0);

					UBOOL GenerateVisNoReachList = TRUE;
					GConfig->GetBool(TEXT("Options"), TEXT("GenerateVisNoReachList"), GenerateVisNoReachList, GUEDIni);
					SendMessageW(GetDlgItem(hDlg, IDCK_GENERATE_VNR_LIST), BM_SETCHECK, GenerateVisNoReachList ? BST_CHECKED : BST_UNCHECKED, 0);
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
			TCHAR buffer[10];
			if ((HWND)lParam == GetDlgItem(hDlg, IDSL_BALANCE))
			{

				INT Cuts = SendMessageW((HWND)lParam, TBM_GETPOS, 0, 0);
				appSnprintf(buffer, ARRAY_COUNT(buffer), TEXT("%i"), Cuts);
				*EdBuildOpt = Cuts | (Portals << 8) | (Opt << 16);
			
				SendMessageW(GetDlgItem(hDlg, IDSC_BALANCE), WM_SETTEXT, 0, (LPARAM)(LPCTSTR)buffer);
			}
			else if ((HWND)lParam == GetDlgItem(hDlg, IDSL_PORTALBIAS))
			{
				INT Portals = SendMessageW((HWND)lParam, TBM_GETPOS, 0, 0);
				appSnprintf(buffer, ARRAY_COUNT(buffer), TEXT("%i"), Portals);
				*EdBuildOpt = Cuts | (Portals << 8) | (Opt << 16);
			
				SendMessageW(GetDlgItem(hDlg, IDSC_PORTALBIAS), WM_SETTEXT, 0, (LPARAM)(LPCTSTR)buffer);
			}
			else
				for (INT i = 0; i < ARRAY_COUNT(LightMapScale); i++)
					if ((HWND)lParam == LightMapScale[i].Trackbar)
					{
						INT Value = SendMessageW((HWND)lParam, TBM_GETPOS, 0, 0);
						Value = Clamp(Value, 0, 16);
						GConfig->SetInt(TEXT("Options"), *FString::Printf(TEXT("LightMapScale[%d]"), i), Value, GUEDIni);

						appSnprintf(buffer, ARRAY_COUNT(buffer), TEXT("%i"), Value);
						SendMessageW(GetDlgItem(hDlg, LightMapScale[i].LabelId), WM_SETTEXT, 0, (LPARAM)(LPCTSTR)buffer);
						break;
					}
		}
		break;

		case WM_COMMAND:

			switch (HIWORD(wParam)) {

			case BN_CLICKED:

				switch (LOWORD(wParam)) {

				case IDCK_GEOMETRY:
				{
					BOOL bChecked = (SendMessageW(GetDlgItem(hDlg, IDCK_GEOMETRY), BM_GETCHECK, 0, 0) == BST_CHECKED);

					EnableWindow(GetDlgItem(hDlg, IDCK_ONLY_VISIBLE), bChecked);
					EnableWindow(GetDlgItem(hDlg, IDCK_REBUILD_MOVERS), bChecked);
				}
				break;

				case IDCK_BSP:
				{
					BOOL bChecked = (SendMessageW(GetDlgItem(hDlg, IDCK_BSP), BM_GETCHECK, 0, 0) == BST_CHECKED);

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
					EnableWindow(GetDlgItem(hDlg, IDSL_PORTALBIAS), bChecked);
					EnableWindow(GetDlgItem(hDlg, IDSC_BSP_3), bChecked);
					EnableWindow(GetDlgItem(hDlg, IDSC_BSP_4), bChecked);
					EnableWindow(GetDlgItem(hDlg, IDSC_PORTALBIAS), bChecked);
				}
				break;

				case IDCK_LIGHTING:
				{
					BOOL bChecked = (SendMessageW(GetDlgItem(hDlg, IDCK_LIGHTING), BM_GETCHECK, 0, 0) == BST_CHECKED);

					EnableWindow(GetDlgItem(hDlg, IDCK_SEL_LIGHTS_ONLY), bChecked);
					EnableWindow(GetDlgItem(hDlg, IDSC_RESOLUTION), bChecked);
				}
				break;

				case IDCK_UNIFORM_SHADOWS:
				{
					BOOL bChecked = (SendMessageW(GetDlgItem(hDlg, IDCK_UNIFORM_SHADOWS), BM_GETCHECK, 0, 0) == BST_CHECKED);

					GConfig->SetBool(TEXT("Options"), TEXT("UniformLightMapScale"), bChecked, GUEDIni);
				}
				break;

				case IDCK_GENERATE_VNR_LIST:
				{
					BOOL bChecked = (SendMessageW(GetDlgItem(hDlg, IDCK_GENERATE_VNR_LIST), BM_GETCHECK, 0, 0) == BST_CHECKED);

					GConfig->SetBool(TEXT("Options"), TEXT("GenerateVisNoReachList"), bChecked, GUEDIni);
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
