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
static INT ActivePage = eBS_MAX;

TBuildSheet::TBuildSheet()
{
	m_bShow = FALSE;
	m_hwndSheet = NULL;
}

TBuildSheet::~TBuildSheet()
{
	if (m_hWndTooltip)
		DestroyWindow(m_hWndTooltip);
}

static void AddTool(HWND tth, HWND InHwnd, const TCHAR* InToolTip, int InId)
{
	guard(AddTool);

	check(tth != NULL);
	check(InHwnd != NULL);
	check(::IsWindow(tth));

	TOOLINFOW ti = { 0 };
	FString TmpToolTip = InToolTip;
	ti.cbSize = TTTOOLINFO_V1_SIZE;
	ti.uFlags = TTF_SUBCLASS | TTF_TRANSPARENT;
	ti.hwnd = InHwnd;
	ti.hinst = hInstance;
	ti.uId = InId;
	ti.lpszText = &TmpToolTip[0];
	::GetClientRect(InHwnd, &ti.rect);

	verify(SendMessageW(tth, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti));
	unguard;
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
	SendMessageA( GetDlgItem( m_hwndSheet, IDCANCEL ), WM_SETTEXT, 0, (LPARAM)(LPCTSTR)"&Hide");
}

void TBuildSheet::Show( BOOL bShow )
{
	if( !m_hwndSheet || m_bShow == bShow ) { return; }

	if (!bShow)
		SetFocus(GhwndEditorFrame);

	ShowWindow( m_hwndSheet, bShow ? SW_SHOW : SW_HIDE );

	m_bShow = bShow;

	if (bShow)
	{
		// stijn: workaround for weird WIN32 bug
		if (ActivePage >= 0 && ActivePage < eBS_MAX)
			PropSheet_SetCurSel(m_hwndSheet, GhwndBSPages[ActivePage], ActivePage);
	}
}

void TBuildSheet::GetBuildOpts()
{
	BuildOptions.BuildFlags = 0;
	BuildOptions.OptFlags = (SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDCK_ENABLE_MULTITHREAD), BM_GETCHECK, 0, 0) == BST_CHECKED) ? OPTFLAGS_MultiThreading : 0;

	// GEOMETRY
	if (SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDCK_GEOMETRY), BM_GETCHECK, 0, 0) == BST_CHECKED)
		BuildOptions.BuildFlags |= BUILDFLAGS_Geometry;
	if(SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDCK_ONLY_VISIBLE), BM_GETCHECK, 0, 0) == BST_CHECKED)
		BuildOptions.OptFlags |= OPTFLAGS_GeometryVisOnly;
	if (SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDCK_POST_SEMISOLIDS), BM_GETCHECK, 0, 0) == BST_CHECKED)
		BuildOptions.OptFlags |= OPTFLAGS_PostBuildSemis;

	// BSP
	if (SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDCK_BSP), BM_GETCHECK, 0, 0) == BST_CHECKED)
		BuildOptions.BuildFlags |= BUILDFLAGS_BSP;
	if ((SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_LAME), BM_GETCHECK, 0, 0) == BST_CHECKED))
		BuildOptions.Opt = 0;
	else if ((SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_GOOD), BM_GETCHECK, 0, 0) == BST_CHECKED))
		BuildOptions.Opt = 1;
	else if ((SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_OPTIMAL), BM_GETCHECK, 0, 0) == BST_CHECKED))
		BuildOptions.Opt = 2;
	if ((SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDCK_OPT_GEOM), BM_GETCHECK, 0, 0) == BST_CHECKED))
		BuildOptions.OptFlags |= OPTFLAGS_OptimizeGeometry;
	if ((SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDCK_BUILD_VIS_ZONES), BM_GETCHECK, 0, 0) == BST_CHECKED))
		BuildOptions.OptFlags |= OPTFLAGS_VisZones;
	if ((SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDCK_OPT_LIGHT_GEOMETRY), BM_GETCHECK, 0, 0) == BST_CHECKED))
		BuildOptions.OptFlags |= OPTFLAGS_OptimizeLightGeo;
	if ((SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDCK_OPT_FIX_COLLISION), BM_GETCHECK, 0, 0) != BST_CHECKED))
		BuildOptions.OptFlags |= OPTFLAGS_NoFixCollision;
	if ((SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_KEEP), BM_GETCHECK, 0, 0) == BST_CHECKED))
		BuildOptions.ZoningMode = ZONEMODE_Keep;
	else if ((SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_MERGE), BM_GETCHECK, 0, 0) == BST_CHECKED))
		BuildOptions.ZoningMode = ZONEMODE_Merge;
	else if ((SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_DISCARD), BM_GETCHECK, 0, 0) == BST_CHECKED))
		BuildOptions.ZoningMode = ZONEMODE_Discard;
	BuildOptions.Cuts = SendMessageA(GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDSL_BALANCE), TBM_GETPOS, 0, 0);
	BuildOptions.Portals = SendMessageA(GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDSL_PORTALBIAS), TBM_GETPOS, 0, 0);

	// LIGHTING
	if (SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDCK_LIGHTING), BM_GETCHECK, 0, 0) == BST_CHECKED)
		BuildOptions.BuildFlags |= BUILDFLAGS_Lights;
	if(SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDCK_SEL_LIGHTS_ONLY), BM_GETCHECK, 0, 0) == BST_CHECKED)
		BuildOptions.OptFlags |= OPTFLAGS_SelectedLights;

	// PATHS
	if (SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDCK_PATH_DEFINE), BM_GETCHECK, 0, 0) == BST_CHECKED)
		BuildOptions.BuildFlags |= BUILDFLAGS_Paths;

	// Update stored settings.
	SaveSettings();
}

void TBuildSheet::SaveSettings()
{
	guard(TBuildSheet::SaveSettings);
	GEditor->Level->GetLevelInfo()->EdBuildOpt = BuildOptions.Cuts | (BuildOptions.Portals << 8) | (BuildOptions.Opt << 16) | (INT((BuildOptions.OptFlags & OPTFLAGS_PostBuildSemis) != 0) << 18)
		| (INT((BuildOptions.OptFlags & OPTFLAGS_NoFixCollision) != 0) << 19) | (BuildOptions.ZoningMode << 27);
	if (UBOOL(GEditor->bMultiThreadBSPBuild) != UBOOL((BuildOptions.OptFlags & OPTFLAGS_MultiThreading) != 0))
	{
		GEditor->bMultiThreadBSPBuild = UBOOL((BuildOptions.OptFlags & OPTFLAGS_MultiThreading) != 0);
		GEditor->SaveConfig();
	}
	unguard;
}

void TBuildSheet::FBuildOptions::GetCmd_Geometry(TCHAR* Cmd)
{
	appSprintf(Cmd, TEXT("MAP REBUILD VISIBLEONLY=%d MULTITHREAD=%d POSTSEMISOLIDS=%d"), ((OptFlags & OPTFLAGS_GeometryVisOnly) != 0), ((OptFlags & OPTFLAGS_MultiThreading) != 0), ((OptFlags & OPTFLAGS_PostBuildSemis) != 0));

	if (OptFlags & OPTFLAGS_VisZones)
	{
		TCHAR Wk[64];
		appSprintf(Wk, TEXT(" LIGHTZ=%d ZONES=%d"), ((OptFlags & OPTFLAGS_OptimizeLightGeo) != 0), ZoningMode);
		appStrcat(Cmd, Wk);
	}
	if (OptFlags & OPTFLAGS_NoFixCollision)
		appStrcat(Cmd, TEXT(" NOFIXCOL"));
}
void TBuildSheet::FBuildOptions::GetCmd_BSP(TCHAR* Cmd)
{
	TCHAR Wk[64];

	appStrcpy(Cmd, TEXT("BSP REBUILD"));
	if (Opt == 0)
		appStrcat(Cmd, TEXT(" LAME"));
	else if (Opt == 1)
		appStrcat(Cmd, TEXT(" GOOD"));
	else appStrcat(Cmd, TEXT(" OPTIMAL"));

	if (OptFlags & OPTFLAGS_OptimizeGeometry)
		appStrcat(Cmd, TEXT(" OPTGEOM"));
	if (OptFlags & OPTFLAGS_VisZones)
	{
		appSprintf(Wk, TEXT(" LIGHTZ=%d ZONES=%d"), ((OptFlags & OPTFLAGS_OptimizeLightGeo) != 0), ZoningMode);
		appStrcat(Cmd, Wk);
	}
	appSprintf(Wk, TEXT(" BALANCE=%d"), Cuts);
	appStrcat(Cmd, Wk);
	appSprintf(Wk, TEXT(" PORTALBIAS=%d MULTITHREAD=%d"), Portals, ((OptFlags & OPTFLAGS_MultiThreading) != 0));
	appStrcat(Cmd, Wk);
	if (OptFlags & OPTFLAGS_NoFixCollision)
		appStrcat(Cmd, TEXT(" NOFIXCOL"));
}
void TBuildSheet::FBuildOptions::GetCmd_Lighting(TCHAR* Cmd)
{
	appSprintf(Cmd, TEXT("LIGHT APPLY SELECTED=%d VISIBLEONLY=%d LIGHTZ=%d ZONES=%d MULTITHREAD=%d"), ((OptFlags & OPTFLAGS_SelectedLights) != 0), ((OptFlags & OPTFLAGS_GeometryVisOnly) != 0), ((OptFlags & OPTFLAGS_OptimizeLightGeo) != 0), ZoningMode, ((OptFlags & OPTFLAGS_MultiThreading) != 0));
	if (OptFlags & OPTFLAGS_NoFixCollision)
		appStrcat(Cmd, TEXT(" NOFIXCOL"));
}
void TBuildSheet::FBuildOptions::GetCmd_Paths(TCHAR* Cmd)
{
	appStrcpy(Cmd, TEXT("PATHS DEFINE"));
}
void TBuildSheet::FBuildOptions::GetCmd_Full(TCHAR* Cmd, DWORD Flags)
{
	TCHAR Wk[64];

	appSprintf(Cmd, TEXT("BSP MULTI VISIBLEONLY=%d MULTITHREAD=%d"), ((OptFlags & OPTFLAGS_GeometryVisOnly) != 0), ((OptFlags & OPTFLAGS_MultiThreading) != 0));
	if (Flags & BUILDFLAGS_Geometry)
	{
		appStrcat(Cmd, TEXT(" GEOM"));
		if (OptFlags & OPTFLAGS_PostBuildSemis)
			appStrcat(Cmd, TEXT(" POSTSEMISOLIDS=1"));
	}

	UBOOL bBuiltLightZones = FALSE;
	if (Flags & BUILDFLAGS_BSP)
	{
		appSprintf(Wk, TEXT(" BSP=%d"), Opt);
		appStrcat(Cmd, Wk);

		if (OptFlags & OPTFLAGS_OptimizeGeometry)
			appStrcat(Cmd, TEXT(" OPTGEOM"));
		if (OptFlags & OPTFLAGS_VisZones)
		{
			bBuiltLightZones = TRUE;
			appSprintf(Wk, TEXT(" LIGHTZ=%d ZONES=%d"), ((OptFlags & OPTFLAGS_OptimizeLightGeo) != 0), ZoningMode);
			appStrcat(Cmd, Wk);
		}
		appSprintf(Wk, TEXT(" BALANCE=%d PORTALBIAS=%d"), Cuts, Portals);
		appStrcat(Cmd, Wk);
	}
	if (OptFlags & OPTFLAGS_NoFixCollision)
		appStrcat(Cmd, TEXT(" NOFIXCOL"));

	if (Flags & BUILDFLAGS_Lights)
	{
		if (!bBuiltLightZones)
		{
			appSprintf(Wk, TEXT(" LIGHTZ=%d ZONES=%d"), ((OptFlags & OPTFLAGS_OptimizeLightGeo) != 0), ZoningMode);
			appStrcat(Cmd, Wk);
		}
		appSprintf(Wk, TEXT(" LIGHTS SELECTED=%d"), ((OptFlags & OPTFLAGS_SelectedLights) != 0));
		appStrcat(Cmd, Wk);
	}

	if (Flags & BUILDFLAGS_Paths)
		appStrcat(Cmd, TEXT(" PATHS"));
}

void TBuildSheet::Build()
{
	TCHAR Cmd[256];

	GetBuildOpts();

	if (BuildOptions.BuildFlags)
	{
		BuildOptions.GetCmd_Full(Cmd, BuildOptions.BuildFlags);
		GEditor->Exec(Cmd);
	}
	RefreshStats();
}
void TBuildSheet::BuildStep(DWORD Flags)
{
	TCHAR Cmd[256];

	GetBuildOpts();

	if (Flags)
	{
		BuildOptions.GetCmd_Full(Cmd, Flags);
		GEditor->Exec(Cmd);
	}
	RefreshStats();
}

static LPARAM GetIntAnsi(const INT Value)
{
	static TCHAR UniStr[64];
	static ANSICHAR AnsStr[64];

	appSnprintf(UniStr, 64, TEXT("%i"), Value);
	appToAnsiInPlace(AnsStr, UniStr, 64);
	return reinterpret_cast<LPARAM>(AnsStr);
}

void TBuildSheet::RefreshStats()
{
	// GEOMETRY
	UEditorEngine::FBspStats Stats;
	GEditor->GetBSPStats(Stats);

	SendMessageA( ::GetDlgItem( GhwndBSPages[eBS_STATS], IDSC_BRUSHES), WM_SETTEXT, 0, GetIntAnsi(Stats.Brushes));
	SendMessageA( ::GetDlgItem( GhwndBSPages[eBS_STATS], IDSC_ZONES), WM_SETTEXT, 0, GetIntAnsi(Stats.Zones));

	// BSP
	SendMessageA( ::GetDlgItem( GhwndBSPages[eBS_STATS], IDSC_POLYS), WM_SETTEXT, 0, GetIntAnsi(Stats.Polys));
	SendMessageA( ::GetDlgItem( GhwndBSPages[eBS_STATS], IDSC_NODES), WM_SETTEXT, 0, GetIntAnsi(Stats.Nodes));
	SendMessageA( ::GetDlgItem( GhwndBSPages[eBS_STATS], IDSC_MAX_DEPTH), WM_SETTEXT, 0, GetIntAnsi(Stats.MaxDepth));
	SendMessageA( ::GetDlgItem( GhwndBSPages[eBS_STATS], IDSC_AVG_DEPTH), WM_SETTEXT, 0, GetIntAnsi(Stats.AvgDepth));

	if (!Stats.Polys)
		SendMessageA( ::GetDlgItem( GhwndBSPages[eBS_STATS], IDSC_RATIO), WM_SETTEXT, 0, (LPARAM)(LPCTSTR)"N/A" );
	else
	{
		const FLOAT fRatio = (Stats.Nodes / (FLOAT)Stats.Polys);
		FString RatioStr = FString::Printf(TEXT("%1.2f:1"), fRatio);
		SendMessageA(::GetDlgItem(GhwndBSPages[eBS_STATS], IDSC_RATIO), WM_SETTEXT, 0, (LPARAM)(LPCTSTR)appToAnsi(*RatioStr));
	}

	// LIGHTING
	SendMessageA( ::GetDlgItem( GhwndBSPages[eBS_STATS], IDSC_LIGHTS), WM_SETTEXT, 0, GetIntAnsi(Stats.LightCount));

	LoadSettings();

	GEditor->Flush(0);
}

void TBuildSheet::LoadSettings()
{
	guard(TBuildSheet::LoadSettings);
	//Build options
	INT Cuts = 15;
	INT Opt = 2;
	INT Portals = 70;
	INT ZoneMode = 0;
	UBOOL bPostSemiBuild = FALSE;
	UBOOL bNoFixCol = FALSE;
	char buffer[10];
	if (GEditor && GEditor->Level)
	{
		Cuts = Min(GEditor->Level->GetLevelInfo()->EdBuildOpt & 0x7F, 100);
		Portals = Min((GEditor->Level->GetLevelInfo()->EdBuildOpt >> 8) & 0x7F, 100);
		Opt = (GEditor->Level->GetLevelInfo()->EdBuildOpt >> 16) & 0x03;
		bPostSemiBuild = (GEditor->Level->GetLevelInfo()->EdBuildOpt >> 18) & 1;
		bNoFixCol = (GEditor->Level->GetLevelInfo()->EdBuildOpt >> 19) & 1;
		ZoneMode = (GEditor->Level->GetLevelInfo()->EdBuildOpt >> 27) & ZONEMODE_BitMask;
	}
	_itoa(Cuts, buffer, sizeof(buffer));
	SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDSL_BALANCE), TBM_SETPOS, 1, Cuts);
	SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDSC_BALANCE), WM_SETTEXT, 0, (LPARAM)(LPCTSTR)buffer);

	_itoa(Portals, buffer, sizeof(buffer));
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
	if (ZoneMode == ZONEMODE_Keep)
	{
		SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_KEEP), BM_SETCHECK, BST_CHECKED, 0);
		SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_MERGE), BM_SETCHECK, BST_UNCHECKED, 0);
		SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_DISCARD), BM_SETCHECK, BST_UNCHECKED, 0);
	}
	else if (ZoneMode == ZONEMODE_Merge)
	{
		SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_KEEP), BM_SETCHECK, BST_UNCHECKED, 0);
		SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_MERGE), BM_SETCHECK, BST_CHECKED, 0);
		SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_DISCARD), BM_SETCHECK, BST_UNCHECKED, 0);
	}
	else
	{
		SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_KEEP), BM_SETCHECK, BST_UNCHECKED, 0);
		SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_MERGE), BM_SETCHECK, BST_UNCHECKED, 0);
		SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDRB_DISCARD), BM_SETCHECK, BST_CHECKED, 0);
	}
	SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDCK_POST_SEMISOLIDS), BM_SETCHECK, bPostSemiBuild ? BST_CHECKED : BST_UNCHECKED, 0);
	SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDCK_OPT_FIX_COLLISION), BM_SETCHECK, bNoFixCol ? BST_UNCHECKED : BST_CHECKED, 0);
	SendMessageA(::GetDlgItem(GhwndBSPages[eBS_OPTIONS], IDCK_ENABLE_MULTITHREAD), BM_SETCHECK, (!GEditor || GEditor->bMultiThreadBSPBuild) ? BST_CHECKED : BST_UNCHECKED, 0);
	unguard;
}

// --------------------------------------------------------------
//
// OPTIONS
//
// --------------------------------------------------------------

static BOOL bFirstTimeBuildSheetOptionsProc = TRUE;

INT_PTR APIENTRY OptionsProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_NOTIFY:

			switch(((NMHDR FAR*)lParam)->code)
			{
				case PSN_SETACTIVE:

					if(bFirstTimeBuildSheetOptionsProc) {

						bFirstTimeBuildSheetOptionsProc = FALSE;
						GhwndBSPages[eBS_OPTIONS] = hDlg;

						SendMessageA( GetDlgItem( hDlg, IDCK_GEOMETRY), BM_SETCHECK, BST_CHECKED, 0 );
						SendMessageA( GetDlgItem( hDlg, IDCK_BSP), BM_SETCHECK, BST_CHECKED, 0 );
						SendMessageA( GetDlgItem( hDlg, IDCK_LIGHTING), BM_SETCHECK, BST_CHECKED, 0 );
						SendMessageA( GetDlgItem( hDlg, IDCK_PATH_DEFINE), BM_SETCHECK, BST_CHECKED, 0 );

						SendMessageA( GetDlgItem( hDlg, IDCK_OPT_GEOM), BM_SETCHECK, BST_CHECKED, 0 );
						SendMessageA( GetDlgItem( hDlg, IDCK_BUILD_VIS_ZONES), BM_SETCHECK, BST_CHECKED, 0 );
						SendMessageA( GetDlgItem( hDlg, IDCK_OPT_LIGHT_GEOMETRY), BM_SETCHECK, BST_CHECKED, 0);
						SendMessageA( GetDlgItem( hDlg, IDCK_OPT_FIX_COLLISION), BM_SETCHECK, BST_CHECKED, 0);

						SendMessageA( GetDlgItem( hDlg, IDSL_BALANCE), TBM_SETRANGE, 1, MAKELONG(0, 100) );
						SendMessageA( GetDlgItem( hDlg, IDSL_BALANCE), TBM_SETTICFREQ, 10, 0 );

						SendMessageA( GetDlgItem( hDlg, IDSL_PORTALBIAS), TBM_SETRANGE, 1, MAKELONG(0, 100) );
						SendMessageA( GetDlgItem( hDlg, IDSL_PORTALBIAS), TBM_SETTICFREQ, 5, 0 );
						GBuildSheet->LoadSettings();

						// Tooltip
						GBuildSheet->m_hWndTooltip = CreateWindow(TOOLTIPS_CLASS, NULL, TTS_ALWAYSTIP | WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hDlg, (HMENU)NULL, hInstance, NULL);
						check(GBuildSheet->m_hWndTooltip);
						SendMessageW(GBuildSheet->m_hWndTooltip, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(0, 0));

#define ADDTOOLTIP(tid,txt) AddTool(GBuildSheet->m_hWndTooltip, GetDlgItem(hDlg, tid), txt, tid)
						ADDTOOLTIP(IDRB_KEEP, TEXT("Keep all zones valid as found in level. It may each area behind smallest closed off BSP gaps create own zones and can easily make map run out of zones. This is pre-227 behaviour of the editor."));
						ADDTOOLTIP(IDRB_MERGE, TEXT("Only keep zones with a valid zoneinfo actor in them, rest of zones are merged into LevelInfo zone."));
						ADDTOOLTIP(IDRB_DISCARD, TEXT("Only keep zones with a valid zoneinfo actor in them, rest of zones are discarded as invalid zones. Game wont even render the discarded zones in-game!"));

						ADDTOOLTIP(IDCK_POST_SEMISOLIDS, TEXT("Force editor to add semisolid brushes last into geometry. This will make game effectively treat semisolids same way as movers. NOTE: Rebuilding BSP without Geometry will nullify this effect!"));
						ADDTOOLTIP(IDCK_OPT_LIGHT_GEOMETRY, TEXT("Filter out light sources that can't be visible to specific BSP leafs. Otherwise function like pre-227 and all leafs within radius are considiered visible."));
						ADDTOOLTIP(IDCK_OPT_FIX_COLLISION, TEXT("This will make BSP builder add extra nodes to correct collision of sharp corners that could turn into invisible walls. It's compatible with pre-227 but will increase nodecount of level."));
#undef ADDTOOLTIP
					}
					ActivePage = eBS_OPTIONS;
					break;

				case PSN_QUERYCANCEL:
					GBuildSheet->Show( 0 );
					SetWindowLongPtr(GhwndBSPages[eBS_OPTIONS], DWLP_MSGRESULT, TRUE);
					SetWindowPos(GhwndBSPages[eBS_OPTIONS], HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED); // Regarding MSDN: If you have changed certain window data using SetWindowLong, you must call SetWindowPos for the changes to take effect. Use the following combination for uFlags: SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED.

					return TRUE;
					break;
			}
			break;

		case WM_HSCROLL:
			{
				char buffer[10];
				if( (HWND)lParam == GetDlgItem( hDlg, IDSL_BALANCE) )
				{
					GBuildSheet->BuildOptions.Cuts = SendMessageA(GetDlgItem(hDlg, IDSL_BALANCE), TBM_GETPOS, 0, 0);
					_itoa(GBuildSheet->BuildOptions.Cuts, buffer, sizeof(buffer) );
					SendMessageA( GetDlgItem( hDlg, IDSC_BALANCE), WM_SETTEXT, 0, (LPARAM)(LPCTSTR)buffer );
					GBuildSheet->SaveSettings();
				}
				if( (HWND)lParam == GetDlgItem( hDlg, IDSL_PORTALBIAS) )
				{
					GBuildSheet->BuildOptions.Portals = SendMessageA(GetDlgItem(hDlg, IDSL_PORTALBIAS), TBM_GETPOS, 0, 0);
					_itoa(GBuildSheet->BuildOptions.Portals, buffer, sizeof(buffer) );
					SendMessageA( GetDlgItem( hDlg, IDSC_PORTALBIAS), WM_SETTEXT, 0, (LPARAM)(LPCTSTR)buffer );
					GBuildSheet->SaveSettings();
				}
			}
			break;

		case WM_COMMAND:

			switch(HIWORD(wParam)) {

				case BN_CLICKED:

					switch(LOWORD(wParam)) {

						case IDCK_GEOMETRY:
							{
								BOOL bChecked = (SendMessageA( GetDlgItem( hDlg, IDCK_GEOMETRY), BM_GETCHECK, 0, 0 ) == BST_CHECKED);

								EnableWindow( GetDlgItem( hDlg, IDCK_ONLY_VISIBLE), bChecked );
								EnableWindow( GetDlgItem( hDlg, IDCK_POST_SEMISOLIDS), bChecked);
							}
							break;

						case IDCK_BSP:
							{
								BOOL bChecked = (SendMessageA( GetDlgItem( hDlg, IDCK_BSP), BM_GETCHECK, 0, 0 ) == BST_CHECKED);

								EnableWindow( GetDlgItem( hDlg, IDSC_OPTIMIZATION), bChecked );
								EnableWindow( GetDlgItem( hDlg, IDRB_LAME), bChecked );
								EnableWindow( GetDlgItem( hDlg, IDRB_GOOD), bChecked );
								EnableWindow( GetDlgItem( hDlg, IDRB_OPTIMAL), bChecked );
								EnableWindow( GetDlgItem( hDlg, IDCK_OPT_GEOM), bChecked );
								EnableWindow( GetDlgItem( hDlg, IDCK_BUILD_VIS_ZONES), bChecked );
								EnableWindow( GetDlgItem( hDlg, IDCK_OPT_LIGHT_GEOMETRY), bChecked);
								EnableWindow( GetDlgItem( hDlg, IDCK_OPT_FIX_COLLISION), bChecked);
								EnableWindow( GetDlgItem( hDlg, IDSL_BALANCE), bChecked );
								EnableWindow( GetDlgItem( hDlg, IDSC_BSP_1), bChecked );
								EnableWindow( GetDlgItem( hDlg, IDSC_BSP_2), bChecked );
								EnableWindow( GetDlgItem( hDlg, IDSC_BALANCE), bChecked );
								EnableWindow( GetDlgItem( hDlg, IDRB_KEEP), bChecked );
								EnableWindow( GetDlgItem( hDlg, IDRB_MERGE), bChecked );
								EnableWindow( GetDlgItem( hDlg, IDRB_DISCARD), bChecked );
							}
							break;

						case IDCK_LIGHTING:
							{
								BOOL bChecked = (SendMessageA( GetDlgItem( hDlg, IDCK_LIGHTING), BM_GETCHECK, 0, 0 ) == BST_CHECKED);

								EnableWindow( GetDlgItem( hDlg, IDCK_SEL_LIGHTS_ONLY), bChecked );
								EnableWindow( GetDlgItem( hDlg, IDSC_RESOLUTION), bChecked );
							}
							break;

						case IDPB_BUILD:
							GBuildSheet->Build();
							break;

						case IDPB_BUILD_PATHS:
						{
							if( ::MessageBox( hDlg, TEXT("This command will erase all existing pathnodes and attempt to create a pathnode network on its own.  Are you sure this is what you want to do?\n\nNOTE : This process can take a VERY long time."), TEXT("Build Paths"), MB_YESNO) == IDYES )
							{
								GEditor->Exec( TEXT("PATHS BUILD") );
							}
							break;
						}
					}
			}
			break;
	}

	return (FALSE);
}

// --------------------------------------------------------------
//
// STATS
//
// --------------------------------------------------------------

static BOOL bFirstTimeBuildSheetStatsProc = TRUE;
INT_PTR APIENTRY StatsProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{	
	switch (message)
	{
		case WM_NOTIFY:

			switch(((NMHDR FAR*)lParam)->code) 
			{
				case PSN_SETACTIVE:

					if(bFirstTimeBuildSheetStatsProc) {

						bFirstTimeBuildSheetStatsProc = FALSE;
						GhwndBSPages[eBS_STATS] = hDlg;
					}
					GBuildSheet->RefreshStats();
					ActivePage = eBS_STATS;
					break;

				case PSN_QUERYCANCEL:
					GBuildSheet->Show( 0 );
					SetWindowLongPtr(GhwndBSPages[eBS_STATS], DWLP_MSGRESULT, TRUE);
					SetWindowPos(GhwndBSPages[eBS_STATS], HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED); // Regarding MSDN: If you have changed certain window data using SetWindowLong, you must call SetWindowPos for the changes to take effect. Use the following combination for uFlags: SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED.
					return TRUE;
					break;
			}
			break;

		case WM_COMMAND:

			switch(HIWORD(wParam)) {

				case BN_CLICKED:

					switch(LOWORD(wParam)) {

						case IDPB_REFRESH:
							GBuildSheet->RefreshStats();
							break;

						case IDPB_BUILD:
							GBuildSheet->Build();
							break;

						case IDPB_CHECKBSP:
							GEditor->Exec(TEXT("BSP VERIFY"));
							break;

						case IDPB_CHECKBSPREC:
							if (::MessageBox(hDlg, TEXT("This command will rebuild level geometry one brush at the time and verify each step for BSP errors, do wish to continue?"), TEXT("Check for errors"), MB_YESNO) == IDYES)
							{
								GEditor->Exec(TEXT("BSP VERIFY RECURSIVE"));
							}
							break;

						case IDPB_CLEARDEBUG:
							GEditor->Exec(TEXT("CLEARDEBUG"));
							break;

						case IDPB_DRAW_COLHULLS:
							GEditor->Exec(TEXT("BSP SHOWHULLS"));
							break;
					}
			}
			break;
	}

	return (FALSE);
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
