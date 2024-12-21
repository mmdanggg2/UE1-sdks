/*=============================================================================
	AddSpecial : Add special brushes
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>

enum EBrushMode
{
	BRUSH_Solid,
	BRUSH_SemiSolid,
	BRUSH_NonSolid,
};
static struct FBrushPresets
{
	DWORD Flags;
	EBrushMode Mode;
	const TCHAR* Name;
} BBuilderPresets[] = {
	{PF_Invisible, BRUSH_SemiSolid, TEXT("Invisible Collision Hull")},
	{PF_Masked, BRUSH_NonSolid, TEXT("Masked Decoration")},
	{PF_Masked, BRUSH_SemiSolid, TEXT("Masked Wall")},
	{0, BRUSH_Solid, TEXT("Regular Brush")},
	{0, BRUSH_SemiSolid, TEXT("Semisolid Pillar")},
	{PF_Translucent, BRUSH_NonSolid, TEXT("Transparent Window")},
	{(PF_Translucent | PF_Portal), BRUSH_NonSolid, TEXT("Water")},
	{(PF_Invisible | PF_Portal), BRUSH_NonSolid, TEXT("Zone Portal")},
	{PF_OccluderPoly, BRUSH_NonSolid, TEXT("Anti-Portal")},
};

class WDlgAddSpecial : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgAddSpecial,WDialog,UnrealEd)

	// Variables.
	WComboBox PrefabCombo;
	WButton OKButton;
	WButton CloseButton;

	// Constructor.
	WDlgAddSpecial( UObject* InContext, WWindow* InOwnerWindow )
	:	WDialog			( TEXT("Add Special"), IDDIALOG_ADD_SPECIAL, InOwnerWindow )
	,	PrefabCombo		( this, IDCB_PREFABS )
	,	OKButton		( this, IDOK, FDelegate(this,(TDelegate)&WDlgAddSpecial::OnOK) )
	,	CloseButton	( this, IDPB_CLOSE, FDelegate(this,(TDelegate)&WDlgAddSpecial::OnClose) )
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgAddSpecial::OnInitDialog);
		WDialog::OnInitDialog();

		for(INT i=0; i<ARRAY_COUNT(BBuilderPresets); ++i )
			PrefabCombo.AddString(BBuilderPresets[i].Name);
		PrefabCombo.SelectionChangeDelegate = FDelegate(this, (TDelegate)&WDlgAddSpecial::OnComboPrefabsSelChange);

		PrefabCombo.SetCurrent( 3 ); // Regular Brush
		OnComboPrefabsSelChange();

		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgAddSpecial::OnDestroy);
		WDialog::OnDestroy();
		::DestroyWindow( hWnd );
		unguard;
	}
	virtual void DoModeless()
	{
		guard(WDlgAddSpecial::DoModeless);
		_Windows.AddItem( this );
		hWnd = CreateDialogParamA( hInstance, MAKEINTRESOURCEA(IDDIALOG_ADD_SPECIAL), OwnerWindow?OwnerWindow->hWnd:NULL, (DLGPROC)StaticDlgProc, (LPARAM)this);
		if( !hWnd )
			appGetLastError();
		Show( TRUE );
		unguard;
	}
	void OnOK()
	{
		guard(WDlgAddSpecial::OnOK);

		DWORD Flags = 0;

		if( SendMessageW( ::GetDlgItem( hWnd, IDCK_MASKED), BM_GETCHECK, 0, 0) == BST_CHECKED )
			Flags |= PF_Masked;
		if (SendMessageW( ::GetDlgItem( hWnd, IDCK_TRANSLUCENT), BM_GETCHECK, 0, 0) == BST_CHECKED)
			Flags |= PF_Translucent;
		if (SendMessageW( ::GetDlgItem( hWnd, IDCK_ALPHABLEND), BM_GETCHECK, 0, 0) == BST_CHECKED)
			Flags |= PF_AlphaBlend;
		if (SendMessageW(::GetDlgItem(hWnd, IDCK_ENVIRONMENT), BM_GETCHECK, 0, 0) == BST_CHECKED)
			Flags |= PF_Environment;
		if (SendMessageW( ::GetDlgItem( hWnd, IDCK_MODULATED), BM_GETCHECK, 0, 0) == BST_CHECKED)
			Flags |= PF_Modulated;
		if( SendMessageW( ::GetDlgItem( hWnd, IDCK_ZONE_PORTAL), BM_GETCHECK, 0, 0) == BST_CHECKED )
			Flags |= PF_Portal;
		if( SendMessageW( ::GetDlgItem( hWnd, IDCK_INVIS), BM_GETCHECK, 0, 0) == BST_CHECKED )
			Flags |= PF_Invisible;
		if( SendMessageW( ::GetDlgItem( hWnd, IDCK_TWO_SIDED), BM_GETCHECK, 0, 0) == BST_CHECKED )
			Flags |= PF_TwoSided;
		if( SendMessageW( ::GetDlgItem( hWnd, IDRB_SEMI_SOLID), BM_GETCHECK, 0, 0) == BST_CHECKED )
			Flags |= PF_Semisolid;
		if( SendMessageW( ::GetDlgItem( hWnd, IDRB_NON_SOLID), BM_GETCHECK, 0, 0) == BST_CHECKED )
			Flags |= PF_NotSolid;
		if (SendMessageW(::GetDlgItem(hWnd, IDCK_INVISOCCLUDE), BM_GETCHECK, 0, 0) == BST_CHECKED)
			Flags |= PF_OccluderPoly;

		GEditor->Exec( *(FString::Printf(TEXT("BRUSH ADD FLAGS=%d"), Flags)));
		unguard;
	}
	void OnClose()
	{
		Show(0);
	}
	inline void SetFlags(DWORD Flags, EBrushMode BrushMode)
	{
		SendMessageW(::GetDlgItem(hWnd, IDCK_MASKED), BM_SETCHECK, ((Flags & PF_Masked) ? BST_CHECKED : BST_UNCHECKED), 0);
		SendMessageW(::GetDlgItem(hWnd, IDCK_TRANSLUCENT), BM_SETCHECK, ((Flags & PF_Translucent) ? BST_CHECKED : BST_UNCHECKED), 0);
		SendMessageW(::GetDlgItem(hWnd, IDCK_ZONE_PORTAL), BM_SETCHECK, ((Flags & PF_Portal) ? BST_CHECKED : BST_UNCHECKED), 0);
		SendMessageW(::GetDlgItem(hWnd, IDCK_INVIS), BM_SETCHECK, ((Flags & PF_Invisible) ? BST_CHECKED : BST_UNCHECKED), 0);
		SendMessageW(::GetDlgItem(hWnd, IDCK_TWO_SIDED), BM_SETCHECK, ((Flags & PF_TwoSided) ? BST_CHECKED : BST_UNCHECKED), 0);
		SendMessageW(::GetDlgItem(hWnd, IDCK_INVISOCCLUDE), BM_SETCHECK, ((Flags & PF_OccluderPoly) ? BST_CHECKED : BST_UNCHECKED), 0);

		SendMessageW(::GetDlgItem(hWnd, IDRB_SOLID), BM_SETCHECK, ((BrushMode == BRUSH_Solid) ? BST_CHECKED : BST_UNCHECKED), 0);
		SendMessageW(::GetDlgItem(hWnd, IDRB_SEMI_SOLID), BM_SETCHECK, ((BrushMode == BRUSH_SemiSolid) ? BST_CHECKED : BST_UNCHECKED), 0);
		SendMessageW(::GetDlgItem(hWnd, IDRB_NON_SOLID), BM_SETCHECK, ((BrushMode == BRUSH_NonSolid) ? BST_CHECKED : BST_UNCHECKED), 0);
	}
	void OnComboPrefabsSelChange()
	{
		INT i = PrefabCombo.GetCurrent();
		SetFlags(BBuilderPresets[i].Flags, BBuilderPresets[i].Mode);
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
