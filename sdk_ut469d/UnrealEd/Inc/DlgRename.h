/*=============================================================================
	DlgRename : Accepts input of a new name for something
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>

class WDlgRename : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgRename,WDialog,UnrealEd)

	WButton OKButton, CancelButton;
	WEdit NameEdit;

	// Variables.
	FString OldName, NewName;

	// Constructor.
	WDlgRename( UObject* InContext, WWindow* InOwnerWindow )
	:	WDialog				( TEXT("Rename"), IDDIALOG_RENAME, InOwnerWindow )
	,	OKButton			( this, IDOK, FDelegate(this,(TDelegate)&WDlgRename::OnOK) )
	,	CancelButton		( this, IDCANCEL, FDelegate(this,(TDelegate)&WDlgRename::EndDialogFalse) )
	,	NameEdit			( this, IDEC_NAME )
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgRename::OnInitDialog);
		WDialog::OnInitDialog();
		NameEdit.SetText( *OldName );
		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgRename::OnDestroy);
		WDialog::OnDestroy();
		::DestroyWindow( hWnd );
		unguard;
	}
	virtual int DoModal( FString InOldName )
	{
		guard(WDlgRename::DoModal);

		NewName = OldName = InOldName;

		return WDialog::DoModal( hInstance );
		unguard;
	}

	void OnOK()
	{
		guard(WDlgRename::OnOK);
		NewName = NameEdit.GetText();
		EndDialogTrue();
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
