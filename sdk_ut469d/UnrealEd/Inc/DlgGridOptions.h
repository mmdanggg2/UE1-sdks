/*=============================================================================
	BuildOptions : Full options for rebuilding maps
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:
	- add in some way to cancel rebuilds

=============================================================================*/

#include <stdio.h>

class WDlgCustomGrid : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgCustomGrid,WDialog,UnrealEd)

	WButton OKButton, CancelButton;
	WEdit ValueEdit;

	// Variables.
	INT Value;

	// Constructor.
	WDlgCustomGrid( UObject* InContext, WWindow* InOwnerWindow )
	:	WDialog				( TEXT("Custom Grid Value"), IDDIALOG_2DSE_CUSTOM_DETAIL, InOwnerWindow )
	,	CancelButton		( this, IDCANCEL, FDelegate(this,(TDelegate)&WDialog::EndDialogFalse) )
	,	OKButton			( this, IDOK, FDelegate(this,(TDelegate)&WDlgCustomGrid::OnOK) )
	,	ValueEdit			( this, IDEC_VALUE )
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgCustomGrid::OnInitDialog);
		WDialog::OnInitDialog();
		ValueEdit.SetText(TEXT("1"));
		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgCustomGrid::OnDestroy);
		WDialog::OnDestroy();
		::DestroyWindow( hWnd );
		unguard;
	}
	virtual int DoModal()
	{
		guard(WDlgCustomGrid::DoModal);
		return WDialog::DoModal( hInstance );
		unguard;
	}

	void OnOK()
	{
		guard(WDlgCustomGrid::OnOK);
		Value = appAtoi( *ValueEdit.GetText() );
		EndDialogTrue();
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
