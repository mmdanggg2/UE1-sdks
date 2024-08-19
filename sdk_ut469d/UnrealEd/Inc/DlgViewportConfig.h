/*=============================================================================
	ViewportConfig : Options for configuring viewport layouts and options
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>

class WDlgViewportConfig : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgViewportConfig,WDialog,UnrealEd)

	// TODO: Buggie: rewrite all to arrays

	// Variables.
	WCheckBox Cfg0Check, Cfg1Check, Cfg2Check, Cfg3Check, Cfg4Check, Cfg5Check;
	WButton OKButton, CancelButton;
	HBITMAP hbmCfg0, hbmCfg1, hbmCfg2, hbmCfg3, hbmCfg4, hbmCfg5;

	int ViewportConfig;

	// Constructor.
	WDlgViewportConfig( UObject* InContext, WWindow* InOwnerWindow )
		: WDialog(TEXT("Viewport Config"), IDDIALOG_VIEWPORT_CONFIG, InOwnerWindow)
		, Cfg0Check(this, IDRB_VCONFIG0)
		, Cfg1Check(this, IDRB_VCONFIG1)
		, Cfg2Check(this, IDRB_VCONFIG2)
		, Cfg3Check(this, IDRB_VCONFIG3)
		, Cfg4Check(this, IDRB_VCONFIG4)
		, Cfg5Check(this, IDRB_VCONFIG5)
		, OKButton(this, IDOK, FDelegate(this, (TDelegate)&WDlgViewportConfig::OnOK))
		, CancelButton(this, IDCANCEL, FDelegate(this, (TDelegate)&WDlgViewportConfig::EndDialogFalse))
		, hbmCfg0(NULL)
		, hbmCfg1(NULL)
		, hbmCfg2(NULL)
		, hbmCfg3(NULL)
		, hbmCfg4(NULL)
		, hbmCfg5(NULL)
		, ViewportConfig(0)
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgViewportConfig::OnInitDialog);
		WDialog::OnInitDialog();

		// Set controls to initial values
		switch(ViewportConfig)
		{
			case 0:		Cfg0Check.SetCheck(BST_CHECKED);	break;
			case 1:		Cfg1Check.SetCheck(BST_CHECKED);	break;
			case 2:		Cfg2Check.SetCheck(BST_CHECKED);	break;
			case 3:		Cfg3Check.SetCheck(BST_CHECKED);	break;
			case 4:		Cfg4Check.SetCheck(BST_CHECKED);	break;
			case 5:		Cfg5Check.SetCheck(BST_CHECKED);	break;
		}

		hbmCfg0 = (HBITMAP)LoadImageA( hInstance, MAKEINTRESOURCEA(IDBM_VIEWPORT_CFG0), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS );	check(hbmCfg0);
		hbmCfg1 = (HBITMAP)LoadImageA( hInstance, MAKEINTRESOURCEA(IDBM_VIEWPORT_CFG1), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS );	check(hbmCfg1);
		hbmCfg2 = (HBITMAP)LoadImageA( hInstance, MAKEINTRESOURCEA(IDBM_VIEWPORT_CFG2), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS );	check(hbmCfg2);
		hbmCfg3 = (HBITMAP)LoadImageA( hInstance, MAKEINTRESOURCEA(IDBM_VIEWPORT_CFG3), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS );	check(hbmCfg3);
		hbmCfg4 = (HBITMAP)LoadImageA( hInstance, MAKEINTRESOURCEA(IDBM_VIEWPORT_CFG4), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS );	check(hbmCfg4);
		hbmCfg5 = (HBITMAP)LoadImageA( hInstance, MAKEINTRESOURCEA(IDBM_VIEWPORT_CFG5), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS );	check(hbmCfg5);

		ScaleImageAndReplace(hbmCfg0);
		ScaleImageAndReplace(hbmCfg1);
		ScaleImageAndReplace(hbmCfg2);
		ScaleImageAndReplace(hbmCfg3);
		ScaleImageAndReplace(hbmCfg4);
		ScaleImageAndReplace(hbmCfg5);
		
		SendMessageW( Cfg0Check.hWnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbmCfg0);
		SendMessageW( Cfg1Check.hWnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbmCfg1);
		SendMessageW( Cfg2Check.hWnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbmCfg2);
		SendMessageW( Cfg3Check.hWnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbmCfg3);
		SendMessageW( Cfg4Check.hWnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbmCfg4);
		SendMessageW( Cfg5Check.hWnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbmCfg5);

		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgViewportConfig::OnDestroy);
		WDialog::OnDestroy();
		::DestroyWindow( hWnd );
		DeleteObject(hbmCfg0);
		DeleteObject(hbmCfg1);
		DeleteObject(hbmCfg2);
		DeleteObject(hbmCfg3);
		DeleteObject(hbmCfg4);
		unguard;
	}
	virtual int DoModal( int _Config )
	{
		guard(WDlgViewportConfig::DoModal);

		ViewportConfig = _Config;

		return WDialog::DoModal( hInstance );
		unguard;
	}
	void OnOK()
	{
		guard(WDlgViewportConfig::OnOK);

		ViewportConfig = 0;

		if( Cfg0Check.IsChecked() ) ViewportConfig = 0;
		else if( Cfg1Check.IsChecked() ) ViewportConfig = 1;
		else if( Cfg2Check.IsChecked() ) ViewportConfig = 2;
		else if( Cfg3Check.IsChecked() ) ViewportConfig = 3;
		else if( Cfg4Check.IsChecked() ) ViewportConfig = 4;
		else if( Cfg5Check.IsChecked() ) ViewportConfig = 5;

		EndDialog(1);
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
