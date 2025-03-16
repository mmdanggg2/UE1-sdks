/*=============================================================================
	TSurfPropSheet : Property sheet for manipulating poly properties
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include "SurfPropSheet.h"
#include "res/resource.h"
#include <commctrl.h>

#include "Engine.h"
#include "Window.h"

#include "..\..\Editor\Src\EditorPrivate.h"
EDITOR_API extern class UEditorEngine* GEditor;

#include "DlgDialogTool.h"

INT_PTR APIENTRY Flags1Proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR APIENTRY AlignmentProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR APIENTRY SPStatsProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void GetDataFromSurfs1( HWND hDlg, ULevel* Level );

extern TSurfPropSheet* GSurfPropSheet;

HWND GhwndSPPages[eSPS_MAX];
static INT ActivePage = 0;

void UpdateChildControls()
{
	FStringOutputDevice GetPropResult = FStringOutputDevice();
	GetPropResult.Empty();
	if( GEditor )
		GEditor->Get( TEXT("POLYS"), TEXT("NUMSELECTED"), GetPropResult );
	int NumSelected = appAtoi(*GetPropResult);

	// Disable all controls if no surfaces are selected.
	HWND hwndChild;
	for( int x = 0 ; x < eSPS_MAX ; x++ )
	{
		hwndChild = GetWindow( GhwndSPPages[x], GW_CHILD );
		while( hwndChild )
		{
			EnableWindow( hwndChild, NumSelected );
			hwndChild = GetWindow( hwndChild, GW_HWNDNEXT );
		}
	}

	// Change caption to show how many surfaces are selected.
	GetPropResult.Empty();
	if( GEditor )
		GEditor->Get( TEXT("POLYS"), TEXT("~TEXTURENAME"), GetPropResult );
	FString Caption;
	Caption = FString::Printf(TEXT("%d Surface%ls%ls%ls"), NumSelected, NumSelected == 1 ? TEXT("") : TEXT("s"), 
		GetPropResult.Len() ? TEXT(" : ") : TEXT(""), *GetPropResult );
	SendMessageW( GSurfPropSheet->m_hwndSheet, WM_SETTEXT, 0, (LPARAM)*Caption );
}

TSurfPropSheet::TSurfPropSheet()
{
	m_bShow = FALSE;
	m_hwndSheet = NULL;
}

TSurfPropSheet::~TSurfPropSheet()
{
}

void TSurfPropSheet::OpenWindow( HINSTANCE hInst, HWND hWndOwner )
{
	// Flags1 page
	//
	m_pages[eSPS_FLAGS1].dwSize = sizeof(PROPSHEETPAGE);
	m_pages[eSPS_FLAGS1].dwFlags = PSP_USETITLE;
	m_pages[eSPS_FLAGS1].hInstance = hInst;
	m_pages[eSPS_FLAGS1].pszTemplate = MAKEINTRESOURCE(IDPP_SP_FLAGS1);
	m_pages[eSPS_FLAGS1].pszIcon = NULL;
	m_pages[eSPS_FLAGS1].pfnDlgProc = Flags1Proc;
	m_pages[eSPS_FLAGS1].pszTitle = TEXT("Flags");
	m_pages[eSPS_FLAGS1].lParam = 0;
	GhwndSPPages[eSPS_FLAGS1] = NULL;

	// Alignment page
	//
	m_pages[eSPS_ALIGNMENT].dwSize = sizeof(PROPSHEETPAGE);
	m_pages[eSPS_ALIGNMENT].dwFlags = PSP_USETITLE;
	m_pages[eSPS_ALIGNMENT].hInstance = hInst;
	m_pages[eSPS_ALIGNMENT].pszTemplate = MAKEINTRESOURCE(IDPP_SP_ALIGNMENT);
	m_pages[eSPS_ALIGNMENT].pszIcon = NULL;
	m_pages[eSPS_ALIGNMENT].pfnDlgProc = AlignmentProc;
	m_pages[eSPS_ALIGNMENT].pszTitle = TEXT("Alignment");
	m_pages[eSPS_ALIGNMENT].lParam = 0;
	GhwndSPPages[eSPS_ALIGNMENT] = NULL;

	// Stats page
	//
	m_pages[eSPS_STATS].dwSize = sizeof(PROPSHEETPAGE);
	m_pages[eSPS_STATS].dwFlags = PSP_USETITLE;
	m_pages[eSPS_STATS].hInstance = hInst;
	m_pages[eSPS_STATS].pszTemplate = MAKEINTRESOURCE(IDPP_SP_STATS);
	m_pages[eSPS_STATS].pszIcon = NULL;
	m_pages[eSPS_STATS].pfnDlgProc = SPStatsProc;
	m_pages[eSPS_STATS].pszTitle = TEXT("Stats");
	m_pages[eSPS_STATS].lParam = 0;
	GhwndSPPages[eSPS_STATS] = NULL;

	// Property sheet
	//
	m_psh.dwSize = sizeof(PROPSHEETHEADER);
	m_psh.dwFlags = PSH_PROPSHEETPAGE | PSH_MODELESS | PSH_NOAPPLYNOW;
	m_psh.hwndParent = hWndOwner;
	m_psh.hInstance = hInst;
	m_psh.pszIcon = NULL;
	m_psh.pszCaption = TEXT("Surface Properties");
	m_psh.nPages = eSPS_MAX;
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

	// remove question mark button in the header
	LONG lExStyle = GetWindowLong(m_hwndSheet, GWL_EXSTYLE);
	lExStyle &= ~WS_EX_CONTEXTHELP;
	SetWindowLong(m_hwndSheet, GWL_EXSTYLE, lExStyle);
}

void TSurfPropSheet::Show( BOOL bShow )
{
	if( !m_hwndSheet || m_bShow == bShow )
	{
		if (bShow)
			BringWindowToTop(m_hwndSheet);
		return;
	}

	ShowWindow( m_hwndSheet, bShow ? SW_SHOW : SW_HIDE );

	if( bShow )
	{
		::GetDataFromSurfs1( GhwndSPPages[eSPS_FLAGS1], GEditor->Level );
		RefreshStats();

		// stijn: workaround for weird WIN32 bug
		INT Tmp = ActivePage;

		for (INT i = 0; i < eSPS_MAX; ++i)
			if (GhwndSPPages[i])
				PropSheet_SetCurSel(m_hwndSheet, GhwndSPPages[i], i);

		if (Tmp >= 0 && Tmp < eSPS_MAX)
			PropSheet_SetCurSel(m_hwndSheet, GhwndSPPages[Tmp], Tmp);
	}

	m_bShow = bShow;

	if (bShow)
		BringWindowToTop(m_hwndSheet);
}

void TSurfPropSheet::GetDataFromSurfs1()
{
	::GetDataFromSurfs1( GhwndSPPages[eSPS_FLAGS1], GEditor->Level );
}

struct StatItem
{
	INT Id;
	const TCHAR* Name;
};

void TSurfPropSheet::RefreshStats()
{
	FStringOutputDevice GetPropResult = FStringOutputDevice();

	StatItem Items[] = {
		// Lighting
		{IDSC_STATIC_LIGHTS, TEXT("STATICLIGHTS")},
		{IDSC_MESHELS, TEXT("~MESHELS")},
		{IDSC_MESH_SIZE, TEXT("~MESHSIZE")},
		// Surface Stats
		{IDSC_TEXTURENAME, TEXT("~TEXTURENAME")},
		{IDSC_USCALE, TEXT("~USCALE")},
		{IDSC_VSCALE, TEXT("~VSCALE")},
		{IDSC_UPAN, TEXT("~UPAN")},
		{IDSC_VPAN, TEXT("~VPAN")},
		{IDSC_SKEW, TEXT("~SKEW")},
		{IDSC_ANGLE, TEXT("~ANGLE")},
	};

	for (INT i = 0; i < ARRAY_COUNT(Items); i++)
	{
		GetPropResult.Empty();	GEditor->Get(TEXT("POLYS"), Items[i].Name, GetPropResult);
		//	debugf(TEXT("%ls %ls"), Items[i].Name, *GetPropResult);
		SendMessageW(::GetDlgItem(GhwndSPPages[eSPS_STATS], Items[i].Id), WM_SETTEXT, 0, (LPARAM)*GetPropResult);
	}
	
	UpdateChildControls();
}

// --------------------------------------------------------------
//
// FLAGS1
//
// --------------------------------------------------------------

#if ENGINE_VERSION==227
#define dNUM_FLAGS1	27
#else
#define dNUM_FLAGS1	26
#endif

struct {
	int Flag;		// Unreal's bit flag
	int ID;			// Windows control ID
	int Count;		// Temp var
} GPolyflags1[] = {
	PF_Invisible,			IDCK_INVISIBLE,			0,
	PF_Masked,				IDCK_MASKED,			0,
	PF_Translucent,			IDCK_TRANSLUCENT,		0,
	PF_Flat,				IDCK_FLAT,				0,
	PF_Modulated,			IDCK_MODULATED,			0,
	PF_FakeBackdrop,		IDCK_FAKEBACKDROP,		0,
	PF_TwoSided,			IDCK_2SIDED,			0,
	PF_AutoUPan,			IDCK_UPAN,				0,
	PF_AutoVPan,			IDCK_VPAN,				0,
	PF_NoSmooth,			IDCK_NOSMOOTH,			0,
#if ENGINE_VERSION==227
	PF_HeightMap,			IDCK_HEIGHTMAP,			0,
#else
	PF_SpecialPoly,			IDCK_SPECIALPOLY,		0,
#endif
	PF_SmallWavy,			IDCK_SMALLWAVY,			0,
	PF_LowShadowDetail,		IDCK_LOWSHADOWDETAIL,	0,
	PF_DirtyShadows,		IDCK_DIRTYSHADOWS,		0,
	PF_BrightCorners,		IDCK_BRIGHTCORNERS,		0,
	PF_SpecialLit,			IDCK_SPECIALLIT,		0,
	PF_NoBoundRejection,	IDCK_NOBOUNDREJECTION,	0,
	PF_Unlit,				IDCK_UNLIT,				0,
	PF_HighShadowDetail,	IDCK_HISHADOWDETAIL,	0,
	PF_Portal,				IDCK_PORTAL,			0,
	PF_Mirrored,			IDCK_MIRROR,			0,
	PF_Environment,			IDCK_ENVIRONMENT,		0,
#if ENGINE_VERSION==227
	PF_AlphaBlend,			IDCK_ALPHABLEND,		0,
#endif
	PF_NotSolid,			IDCK_NOTSOLID,			0,
	PF_Semisolid,			IDCK_SEMISOLID,			0,
	PF_NoMerge,				IDCK_NOMERGE,			0,
	PF_CloudWavy,			IDCK_CLOUDWAVY,			0,
};

void GetDataFromSurfs1( HWND hDlg, ULevel* Level )
{
	if( !hDlg ) return;

	int TotalSurfs = 0, x;

	// Init counts.
	//
	for( x = 0 ; x < dNUM_FLAGS1 ; x++ )
	{
		GPolyflags1[x].Count = 0;
	}

	// Check to see which flags are used on all selected surfaces.
	//
	for( INT i=0; i<Level->Model->Surfs.Num(); i++ )
	{
		FBspSurf *Poly = &Level->Model->Surfs(i);
		if( Poly->PolyFlags & PF_Selected )
		{
			for( x = 0 ; x < dNUM_FLAGS1 ; x++ )
			{
				if( Poly->PolyFlags & GPolyflags1[x].Flag )
				{
					GPolyflags1[x].Count++;
				}
			}

			TotalSurfs++;
		}
	}

	// Update checkboxes on dialog to match selections.
	//
	for( x = 0 ; x < dNUM_FLAGS1 ; x++ )
	{
		SendMessageW( GetDlgItem( hDlg, GPolyflags1[x].ID ), BM_SETCHECK, BST_UNCHECKED, 0 );

		if( TotalSurfs > 0
				&& GPolyflags1[x].Count > 0 )
		{
			if( GPolyflags1[x].Count == TotalSurfs )
				SendMessageW( GetDlgItem( hDlg, GPolyflags1[x].ID ), BM_SETCHECK, BST_CHECKED, 0 );
			else
				SendMessageW( GetDlgItem( hDlg, GPolyflags1[x].ID ), BM_SETCHECK, BST_INDETERMINATE, 0 );
		}
	}
}

void SendDataToSurfs1( HWND hDlg, ULevel* Level )
{    
	int OnFlags, OffFlags;

	OnFlags = OffFlags = 0;

	for( int x = 0 ; x < dNUM_FLAGS1 ; x++ )
	{
		if( SendMessageW( GetDlgItem( hDlg, GPolyflags1[x].ID ), BM_GETCHECK, 0, 0 ) == BST_CHECKED )
			OnFlags += GPolyflags1[x].Flag;
		if( SendMessageW( GetDlgItem( hDlg, GPolyflags1[x].ID ), BM_GETCHECK, 0, 0 ) == BST_UNCHECKED )
			OffFlags += GPolyflags1[x].Flag;
	}

    GEditor->Exec( *(FString::Printf(TEXT("POLY SET SETFLAGS=%d CLEARFLAGS=%d"), OnFlags, OffFlags)) );
}

static BOOL bFirstTimeSurfPropSheetFlags1Proc = TRUE;
INT_PTR APIENTRY Flags1Proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{	
	try
	{
		switch (message)
		{
		case WM_NOTIFY:

			switch (((NMHDR FAR*)lParam)->code)
			{
			case PSN_SETACTIVE:

				if (bFirstTimeSurfPropSheetFlags1Proc)
				{
					bFirstTimeSurfPropSheetFlags1Proc = FALSE;
					GhwndSPPages[eSPS_FLAGS1] = hDlg;
					UpdateChildControls();
				}
				ActivePage = eSPS_FLAGS1;
				return TRUE;

			case PSN_QUERYCANCEL:
				GSurfPropSheet->Show(0);
				SetWindowLongPtr(GhwndSPPages[eSPS_FLAGS1], DWLP_MSGRESULT, TRUE);
				// MSDN: If you have changed certain window data using SetWindowLong, you must call SetWindowPos for the changes to take effect. Use the following combination for uFlags: SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED.
				SetWindowPos(GhwndSPPages[eSPS_FLAGS1], HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
				return TRUE;
			}
			break;

		case WM_COMMAND:

			switch (HIWORD(wParam)) {

			case BN_CLICKED:

				switch (LOWORD(wParam)) {

					// MISC
					//
				case IDCANCEL:
					GSurfPropSheet->Show(FALSE);
					SendDataToSurfs1(hDlg, GEditor->Level);
					break;

				case IDCK_INVISIBLE:
				case IDCK_MASKED:
				case IDCK_TRANSLUCENT:
				case IDCK_FLAT:
				case IDCK_MODULATED:
				case IDCK_FAKEBACKDROP:
				case IDCK_2SIDED:
				case IDCK_UPAN:
				case IDCK_VPAN:
				case IDCK_NOSMOOTH:
#if ENGINE_VERSION==227
				case IDCK_HEIGHTMAP:
#else
				case IDCK_SPECIALPOLY:
#endif					
				case IDCK_SMALLWAVY:
				case IDCK_LOWSHADOWDETAIL:
				case IDCK_DIRTYSHADOWS:
				case IDCK_BRIGHTCORNERS:
				case IDCK_SPECIALLIT:
				case IDCK_NOBOUNDREJECTION:
				case IDCK_UNLIT:
				case IDCK_HISHADOWDETAIL:
				case IDCK_PORTAL:
				case IDCK_MIRROR:
				case IDCK_ENVIRONMENT:
#if ENGINE_VERSION==227
				case IDCK_ALPHABLEND:
#endif
				case IDCK_NOTSOLID:
				case IDCK_SEMISOLID:
				case IDCK_NOMERGE:
				case IDCK_CLOUDWAVY:

					// Don't allow the user to select the BST_INDETERMINATE mode for the check boxes.  The
					// editor uses that to show conflicts.
					//
					if (SendMessageW(GetDlgItem(hDlg, LOWORD(wParam)), BM_GETCHECK, 0, 0) == BST_CHECKED)
						SendMessageW(GetDlgItem(hDlg, LOWORD(wParam)), BM_SETCHECK, BST_CHECKED, 0);
					else
						SendMessageW(GetDlgItem(hDlg, LOWORD(wParam)), BM_SETCHECK, BST_UNCHECKED, 0);

					SendDataToSurfs1(hDlg, GEditor->Level);
					break;

				}
				break;
				
			default:
				WDialogTool::OnCommandStatic(LOWORD(wParam));
				break;
			}
			break;
		}
	}
	catch (...)
	{
		// stijn: eat errors here so we don't throw in a windowproc
	}

	return (FALSE);
}

// --------------------------------------------------------------
//
// ALIGNMENT
//
// --------------------------------------------------------------

static BOOL bFirstTimeSurfPropSheetAlignmentProc = TRUE;
INT_PTR APIENTRY AlignmentProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	try
	{
		TCHAR l_chCmd[256];

		switch (message)
		{
		case WM_NOTIFY:

			switch (((NMHDR FAR*)lParam)->code)
			{
			case PSN_SETACTIVE:

				if (bFirstTimeSurfPropSheetAlignmentProc) {

					bFirstTimeSurfPropSheetAlignmentProc = FALSE;

					// Load up initial values.
					//
					SendMessageW(GetDlgItem(hDlg, IDCB_SIMPLE_SCALE), CB_ADDSTRING, 0, (LPARAM)TEXT("0.0625"));
					SendMessageW(GetDlgItem(hDlg, IDCB_SIMPLE_SCALE), CB_ADDSTRING, 0, (LPARAM)TEXT("0.125"));
					SendMessageW(GetDlgItem(hDlg, IDCB_SIMPLE_SCALE), CB_ADDSTRING, 0, (LPARAM)TEXT("0.25"));
					SendMessageW(GetDlgItem(hDlg, IDCB_SIMPLE_SCALE), CB_ADDSTRING, 0, (LPARAM)TEXT("0.5"));
					SendMessageW(GetDlgItem(hDlg, IDCB_SIMPLE_SCALE), CB_ADDSTRING, 0, (LPARAM)TEXT("1.0"));
					SendMessageW(GetDlgItem(hDlg, IDCB_SIMPLE_SCALE), CB_ADDSTRING, 0, (LPARAM)TEXT("2.0"));
					SendMessageW(GetDlgItem(hDlg, IDCB_SIMPLE_SCALE), CB_ADDSTRING, 0, (LPARAM)TEXT("4.0"));
					SendMessageW(GetDlgItem(hDlg, IDCB_SIMPLE_SCALE), CB_ADDSTRING, 0, (LPARAM)TEXT("8.0"));
					SendMessageW(GetDlgItem(hDlg, IDCB_SIMPLE_SCALE), CB_ADDSTRING, 0, (LPARAM)TEXT("16.0"));

					GhwndSPPages[eSPS_ALIGNMENT] = hDlg;
					UpdateChildControls();
				}
				ActivePage = eSPS_ALIGNMENT;
				return true;

			case PSN_QUERYCANCEL:
				GSurfPropSheet->Show(0);
				SetWindowLongPtr(GhwndSPPages[eSPS_ALIGNMENT], DWLP_MSGRESULT, TRUE);
				// MSDN: If you have changed certain window data using SetWindowLong, you must call SetWindowPos for the changes to take effect. Use the following combination for uFlags: SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED.
				SetWindowPos(GhwndSPPages[eSPS_ALIGNMENT], HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
				return TRUE;
			}
			break;

		case WM_COMMAND:

			switch (HIWORD(wParam)) {

			case BN_CLICKED:
			{
				FLOAT Angle = 0;
				UBOOL bPressedShift = GetAsyncKeyState(VK_SHIFT) & 0x8000;
				FLOAT l_fMod = bPressedShift ? -1 : 1;
				const TCHAR* Mode = NULL;

				switch (LOWORD(wParam)) {

					// MISC
					//
				case IDCANCEL:
					GSurfPropSheet->Show(FALSE);
					break;

				case IDPB_SKEW_APPLY:
				{
					TCHAR l_chU[20], l_chV[20];
					float l_fU, l_fV;
					float l_fMod = GetAsyncKeyState(VK_SHIFT) & 0x8000 ? -1 : 1;

					GetWindowTextW(GetDlgItem(hDlg, IDEC_SKEW_UV), l_chU, ARRAY_COUNT(l_chU));
					GetWindowTextW(GetDlgItem(hDlg, IDEC_SKEW_VU), l_chV, ARRAY_COUNT(l_chV));

					l_fU = WEdit::GetFloat(FString(l_chU));
					l_fV = WEdit::GetFloat(FString(l_chV));

					appSnprintf(l_chCmd, ARRAY_COUNT(l_chCmd), TEXT("POLY TEXSCALE RELATIVE UV=%f VU=%f"), l_fU*l_fMod, l_fV*l_fMod);

					GEditor->Exec(l_chCmd);
				}
				break;

					// SCALING
					//
				case IDPB_SCALE_APPLY:
				case IDPB_SCALE_APPLY2:
				{
					TCHAR l_chU[20], l_chV[20];
					float l_fU, l_fV;

					GetWindowTextW(GetDlgItem(hDlg, LOWORD(wParam) == IDPB_SCALE_APPLY2 ? IDCB_SIMPLE_SCALE : IDEC_SCALE_U), l_chU, ARRAY_COUNT(l_chU));
					GetWindowTextW(GetDlgItem(hDlg, LOWORD(wParam) == IDPB_SCALE_APPLY2 ? IDCB_SIMPLE_SCALE : IDEC_SCALE_V), l_chV, ARRAY_COUNT(l_chV));

					l_fU = WEdit::GetFloat(FString(l_chU));
					l_fV = WEdit::GetFloat(FString(l_chV));

					if (!l_fU || !l_fV) { break; }

					l_fU = 1.0f / l_fU;
					l_fV = 1.0f / l_fV;

					if (SendMessageW(GetDlgItem(hDlg, IDCK_RELATIVE), BM_GETCHECK, 0, 0) == BST_CHECKED)
						appSnprintf(l_chCmd, ARRAY_COUNT(l_chCmd), TEXT("POLY TEXSCALE RELATIVE UU=%f VV=%f"), l_fU, l_fV);
					else
						appSnprintf(l_chCmd, ARRAY_COUNT(l_chCmd), TEXT("POLY TEXSCALE UU=%f VV=%f"), l_fU, l_fV);

					GEditor->Exec(l_chCmd);
				}
				break;

				// ROTATIONS
				//
				case IDPB_ROT_CUST_APPLY:
					TCHAR ch_rot[20];
					DOUBLE rot;

					GetWindowTextW(GetDlgItem(hDlg, IDPB_ROT_CUST), ch_rot, ARRAY_COUNT(ch_rot));

					rot =  WEdit::GetFloat(FString(ch_rot));

					if (!rot)
						break;
					debugf(TEXT("Rot %f"), rot);
										if (Angle == 0) { Angle = rot*PI/180; } // no break;
				case IDPB_ROT_1:		if (Angle == 0) { Angle = PI/180; } // no break;
				case IDPB_ROT_11_25:	if (Angle == 0) { Angle = PI/16; } // no break;
				case IDPB_ROT_22_5:		if (Angle == 0) { Angle = PI/8; } // no break;
				case IDPB_ROT_45:		if (Angle == 0) { Angle = PI/4; } // no break;
				case IDPB_ROT_90:		if (Angle == 0) { Angle = PI/2; } // no break;
				case IDPB_ROT_180:		if (Angle == 0) { Angle = PI; } // no break;
					GEditor->Exec(*FString::Printf(TEXT("POLY TEXROT ANGLE=%f"), Angle*l_fMod));
					break;

				case IDPB_ROT_D:
					GEditor->Exec(TEXT("POLY TEXMULT UU=1 VV=1 UV=-1 VU=1")); //Small Diagonal
					break;

				case IDPB_ROT_MD:
					GEditor->Exec(TEXT("POLY TEXMULT UU=0.5 VV=0.5 UV=-0.5 VU=0.5")); //Big Diagonal
					break;

				case IDPB_ROT_FLIP_U:
					GEditor->Exec(TEXT("POLY TEXMULT UU=-1 VV=1"));
					break;

				case IDPB_ROT_FLIP_V:
					GEditor->Exec(TEXT("POLY TEXMULT UU=1 VV=-1"));
					break;

					// PANNING
					//
				case IDPB_PAN_U_1:
					appSnprintf(l_chCmd, ARRAY_COUNT(l_chCmd), TEXT("POLY TEXPAN U=%f"), 1 * l_fMod);
					GEditor->Exec(l_chCmd);
					break;

				case IDPB_PAN_U_4:
					appSnprintf(l_chCmd, ARRAY_COUNT(l_chCmd), TEXT("POLY TEXPAN U=%f"), 4 * l_fMod);
					GEditor->Exec(l_chCmd);
					break;

				case IDPB_PAN_U_16:
					appSnprintf(l_chCmd, ARRAY_COUNT(l_chCmd), TEXT("POLY TEXPAN U=%f"), 16 * l_fMod);
					GEditor->Exec(l_chCmd);
					break;

				case IDPB_PAN_U_64:
					appSnprintf(l_chCmd, ARRAY_COUNT(l_chCmd), TEXT("POLY TEXPAN U=%f"), 64 * l_fMod);
					GEditor->Exec(l_chCmd);
					break;

				case IDPB_PAN_V_1:
					appSnprintf(l_chCmd, ARRAY_COUNT(l_chCmd), TEXT("POLY TEXPAN V=%f"), 1 * l_fMod);
					GEditor->Exec(l_chCmd);
					break;

				case IDPB_PAN_V_4:
					appSnprintf(l_chCmd, ARRAY_COUNT(l_chCmd), TEXT("POLY TEXPAN V=%f"), 4 * l_fMod);
					GEditor->Exec(l_chCmd);
					break;

				case IDPB_PAN_V_16:
					appSnprintf(l_chCmd, ARRAY_COUNT(l_chCmd), TEXT("POLY TEXPAN V=%f"), 16 * l_fMod);
					GEditor->Exec(l_chCmd);
					break;

				case IDPB_PAN_V_64:
					appSnprintf(l_chCmd, ARRAY_COUNT(l_chCmd), TEXT("POLY TEXPAN V=%f"), 64 * l_fMod);
					GEditor->Exec(l_chCmd);
					break;

				case IDPB_PAN_APPLY:
				{
					TCHAR l_chU[20], l_chV[20];
					float l_fU, l_fV;

					GetWindowTextW(GetDlgItem(hDlg, IDEC_PAN_U), l_chU, ARRAY_COUNT(l_chU));
					GetWindowTextW(GetDlgItem(hDlg, IDEC_PAN_V), l_chV, ARRAY_COUNT(l_chV));

					l_fU =  WEdit::GetFloat(FString(l_chU));
					l_fV =  WEdit::GetFloat(FString(l_chV));

					if (SendMessageW(GetDlgItem(hDlg, IDCK_PAN_RELATIVE), BM_GETCHECK, 0, 0) == BST_CHECKED)
						appSnprintf(l_chCmd, ARRAY_COUNT(l_chCmd), TEXT("POLY TEXPAN U=%f V=%f"), l_fU, l_fV);
					else
						appSnprintf(l_chCmd, ARRAY_COUNT(l_chCmd), TEXT("POLY TEXPAN RESET U=%f V=%f"), l_fU, l_fV);

					GEditor->Exec(l_chCmd);
				}
				break;

					// ALIGNMENT
					//
				case ID_SurfPopupAlignFloor:			if (!Mode) Mode = TEXT("FLOOR");
				case ID_SurfPopupAlignWallDirection:	if (!Mode) Mode = TEXT("WALLDIR");
				case ID_SurfPopupAlignWallPanning:		if (!Mode) Mode = TEXT("WALLPAN");
				case ID_SurfPopupUnalign:				if (!Mode) Mode = TEXT("DEFAULT");
				case ID_SurfPopupAlignClamp:			if (!Mode) Mode = TEXT("CLAMP");
				case ID_SurfPopupAlignWallX:			if (!Mode) Mode = TEXT("WALLX");
				case ID_SurfPopupAlignWallY:			if (!Mode) Mode = TEXT("WALLY");
				case ID_SurfPopupAlignWalls:			if (!Mode) Mode = TEXT("WALLS");
				case ID_SurfPopupAlignAuto:				if (!Mode) Mode = TEXT("AUTO");
				case ID_SurfPopupAlignWallColumn:		if (!Mode) Mode = TEXT("WALLCOLUMN");
				case ID_SurfPopupAlignOneTile:			if (!Mode) Mode = TEXT("ONETILE");
				case ID_SurfPopupAlignOneTileU:			if (!Mode) Mode = TEXT("ONETILE U");
				case ID_SurfPopupAlignOneTileV:			if (!Mode) Mode = TEXT("ONETILE V");
					
					GEditor->Exec(*FString::Printf(TEXT("POLY TEXALIGN %ls %ls"), Mode, bPressedShift ? TEXT("PRESERVESCALE") : TEXT("")));
					break;

				}
				GSurfPropSheet->RefreshStats();
			}
			break;

			default:
				WDialogTool::OnCommandStatic(LOWORD(wParam));
				break;
			}
			break;
		}
	}
	catch (...)
	{
		// stijn: eat errors here so we don't throw in a windowproc
	}

	return (FALSE);
}

// --------------------------------------------------------------
//
// STATS
//
// --------------------------------------------------------------

static BOOL bFirstTimeSurfPropSheetSPStatsProc = TRUE;
INT_PTR APIENTRY SPStatsProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	try
	{
		switch (message)
		{
		case WM_NOTIFY:

			switch (((NMHDR FAR*)lParam)->code)
			{
			case PSN_SETACTIVE:

				if (bFirstTimeSurfPropSheetSPStatsProc) {

					bFirstTimeSurfPropSheetSPStatsProc = FALSE;
					GhwndSPPages[eSPS_STATS] = hDlg;
					GSurfPropSheet->RefreshStats();
				}
				ActivePage = eSPS_STATS;
				return TRUE;

			case PSN_QUERYCANCEL:
				GSurfPropSheet->Show(0);
				SetWindowLongPtr(GhwndSPPages[eSPS_STATS], DWLP_MSGRESULT, TRUE);
				// MSDN: If you have changed certain window data using SetWindowLong, you must call SetWindowPos for the changes to take effect. Use the following combination for uFlags: SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED.
				SetWindowPos(GhwndSPPages[eSPS_STATS], HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
				return TRUE;
			}
			break;

		case WM_COMMAND:
			WDialogTool::OnCommandStatic(LOWORD(wParam));
			break;
		}
	}
	catch (...)
	{
		// stijn: eat errors here so we don't throw in a windowproc
	}

	return (FALSE);
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
