/*=============================================================================
	AddSpecial : Add special brushes
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>

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

		SetRedraw(false);

		PrefabCombo.AddString(TEXT("Invisible Collision Hull"));
		PrefabCombo.AddString(TEXT("Masked Decoration"));
		PrefabCombo.AddString(TEXT("Masked Wall"));
		PrefabCombo.AddString(TEXT("Regular Brush"));
		PrefabCombo.AddString(TEXT("Semisolid Pillar"));
		PrefabCombo.AddString(TEXT("Transparent Window"));
		PrefabCombo.AddString(TEXT("Water"));
		PrefabCombo.AddString(TEXT("Zone Portal"));
		PrefabCombo.SelectionChangeDelegate = FDelegate(this, (TDelegate)&WDlgAddSpecial::OnComboPrefabsSelChange);

		PrefabCombo.SetCurrent( 3 );
		OnComboPrefabsSelChange();

		SetRedraw(true);

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
		AddWindow(this, TEXT("WDlgAddSpecial"));
		hWnd = CreateDialogParamA( hInstance, MAKEINTRESOURCEA(IDDIALOG_ADD_SPECIAL), OwnerWindow?OwnerWindow->hWnd:NULL, (DLGPROC)StaticDlgProc, (LPARAM)this);
		if( !hWnd )
			appGetLastError();
		Show( TRUE );
		unguard;
	}
	void OnOK()
	{
		guard(WDlgAddSpecial::OnCompileAll);

		int Flags = 0;

		if( SendMessageW( ::GetDlgItem( hWnd, IDCK_MASKED), BM_GETCHECK, 0, 0) == BST_CHECKED )
			Flags |= PF_Masked;
		if( SendMessageW( ::GetDlgItem( hWnd, IDCK_TRANSPARENT), BM_GETCHECK, 0, 0) == BST_CHECKED )
			Flags |= PF_Translucent;
		if (SendMessageW(::GetDlgItem(hWnd, IDCK_TRANSLUCENT), BM_GETCHECK, 0, 0) == BST_CHECKED)
			Flags |= PF_Translucent;
#if ENGINE_VERSION==227
		if (SendMessageW(::GetDlgItem(hWnd, IDCK_ALPHABLEND), BM_GETCHECK, 0, 0) == BST_CHECKED)
			Flags |= PF_AlphaBlend;
#endif
		if (SendMessageW(::GetDlgItem(hWnd, IDCK_ENVIRONMENT), BM_GETCHECK, 0, 0) == BST_CHECKED)
			Flags |= PF_Environment;
		if (SendMessageW(::GetDlgItem(hWnd, IDCK_MODULATED), BM_GETCHECK, 0, 0) == BST_CHECKED)
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

		GEditor->Exec( *(FString::Printf(TEXT("BRUSH ADD FLAGS=%d"), Flags)));
		unguard;
	}
	void OnClose()
	{
		Show(0);
	}
	void OnComboPrefabsSelChange()
	{
		switch( PrefabCombo.GetCurrent() )
		{
			case 0:	// Invisible Collision Hull
				SendMessageW( ::GetDlgItem( hWnd, IDCK_MASKED), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_TRANSPARENT), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_ZONE_PORTAL), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_INVIS), BM_SETCHECK, BST_CHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_TWO_SIDED), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDRB_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDRB_SEMI_SOLID), BM_SETCHECK, BST_CHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDRB_NON_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				break;

			case 1:	// Masked Decoration
				SendMessageW( ::GetDlgItem( hWnd, IDCK_MASKED), BM_SETCHECK, BST_CHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_TRANSPARENT), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_ZONE_PORTAL), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_INVIS), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_TWO_SIDED), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDRB_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDRB_SEMI_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDRB_NON_SOLID), BM_SETCHECK, BST_CHECKED, 0);
				break;

			case 2:	// Masked Wall
				SendMessageW( ::GetDlgItem( hWnd, IDCK_MASKED), BM_SETCHECK, BST_CHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_TRANSPARENT), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_ZONE_PORTAL), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_INVIS), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_TWO_SIDED), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDRB_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDRB_SEMI_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDRB_NON_SOLID), BM_SETCHECK, BST_CHECKED, 0);
				break;

			case 3:	// Regular Brush
				SendMessageW( ::GetDlgItem( hWnd, IDCK_MASKED), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_TRANSPARENT), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_ZONE_PORTAL), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_INVIS), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_TWO_SIDED), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDRB_SOLID), BM_SETCHECK, BST_CHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDRB_SEMI_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDRB_NON_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				break;

			case 4:	// Semisolid Pillar
				SendMessageW( ::GetDlgItem( hWnd, IDCK_MASKED), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_TRANSPARENT), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_ZONE_PORTAL), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_INVIS), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_TWO_SIDED), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDRB_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDRB_SEMI_SOLID), BM_SETCHECK, BST_CHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDRB_NON_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				break;

			case 5:	// Transparent Window
				SendMessageW( ::GetDlgItem( hWnd, IDCK_MASKED), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_TRANSPARENT), BM_SETCHECK, BST_CHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_ZONE_PORTAL), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_INVIS), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_TWO_SIDED), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDRB_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDRB_SEMI_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDRB_NON_SOLID), BM_SETCHECK, BST_CHECKED, 0);
				break;

			case 6:	// Water
				SendMessageW( ::GetDlgItem( hWnd, IDCK_MASKED), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_TRANSPARENT), BM_SETCHECK, BST_CHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_ZONE_PORTAL), BM_SETCHECK, BST_CHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_INVIS), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_TWO_SIDED), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDRB_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDRB_SEMI_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDRB_NON_SOLID), BM_SETCHECK, BST_CHECKED, 0);
				break;

			case 7:	// Zone Portal
				SendMessageW( ::GetDlgItem( hWnd, IDCK_MASKED), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_TRANSPARENT), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_ZONE_PORTAL), BM_SETCHECK, BST_CHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_INVIS), BM_SETCHECK, BST_CHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDCK_TWO_SIDED), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDRB_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDRB_SEMI_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageW( ::GetDlgItem( hWnd, IDRB_NON_SOLID), BM_SETCHECK, BST_CHECKED, 0);
				break;
		}
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
