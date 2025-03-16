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

	enum {CONFIG_COUNT = 8};

	// Variables.
	WCheckBox* CfgCheck[CONFIG_COUNT];
	WButton OKButton, CancelButton;
	HBITMAP hbmCfg[CONFIG_COUNT];

	int ViewportConfig;

	// Constructor.
	WDlgViewportConfig( UObject* InContext, WWindow* InOwnerWindow )
		: WDialog(TEXT("Viewport Config"), IDDIALOG_VIEWPORT_CONFIG, InOwnerWindow)
		, CfgCheck()
		, OKButton(this, IDOK, FDelegate(this, (TDelegate)&WDlgViewportConfig::OnOK))
		, CancelButton(this, IDCANCEL, FDelegate(this, (TDelegate)&WDlgViewportConfig::EndDialogFalse))
		, hbmCfg()
		, ViewportConfig(0)
	{
		INT IDRBs[CONFIG_COUNT] = {IDRB_VCONFIG0, IDRB_VCONFIG1, IDRB_VCONFIG2, IDRB_VCONFIG3, IDRB_VCONFIG4, IDRB_VCONFIG5, IDRB_VCONFIG6, IDRB_VCONFIG7, };
		for (INT i = 0; i < CONFIG_COUNT; i++)
			CfgCheck[i] = new WCheckBox(this, IDRBs[i]);
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgViewportConfig::OnInitDialog);
		WDialog::OnInitDialog();

		// Set controls to initial values
		if (ViewportConfig >= 0 && ViewportConfig < CONFIG_COUNT)
			CfgCheck[ViewportConfig]->SetCheck(BST_CHECKED);

		INT IDBMs[CONFIG_COUNT] = {IDBM_VIEWPORT_CFG0, IDBM_VIEWPORT_CFG1, IDBM_VIEWPORT_CFG2, IDBM_VIEWPORT_CFG3, IDBM_VIEWPORT_CFG4, IDBM_VIEWPORT_CFG5, IDBM_VIEWPORT_CFG6, IDBM_VIEWPORT_CFG7, };
		for (INT i = 0; i < CONFIG_COUNT; i++)
		{
			hbmCfg[i] = (HBITMAP)LoadImageW( hInstance, MAKEINTRESOURCEW(IDBMs[i]), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS );
			check(hbmCfg[i]);
			ScaleImageAndReplace(hbmCfg[i]);
			SendMessageW(CfgCheck[i]->hWnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbmCfg[i]);
		}

		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgViewportConfig::OnDestroy);
		WDialog::OnDestroy();
		::DestroyWindow( hWnd );
		for (INT i = 0; i < CONFIG_COUNT; i++)
			DeleteObject(hbmCfg[i]);
		for (INT i = 0; i < CONFIG_COUNT; i++)
			delete CfgCheck[i];
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

		for (INT i = 0; i < CONFIG_COUNT; i++)
			if (CfgCheck[i]->IsChecked())
				ViewportConfig = i;

		EndDialog(1);
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
