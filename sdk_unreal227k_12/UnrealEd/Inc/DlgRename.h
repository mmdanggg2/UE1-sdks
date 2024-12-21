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
	WEdit NameEdit, GroupEdit, PackageEdit;

	// Variables.
	UObject* Object;

	// Constructor.
	WDlgRename( UObject* InObject, WWindow* InOwnerWindow )
	:	WDialog				( TEXT("Rename"), IDDIALOG_RENAME, InOwnerWindow )
	,	CancelButton		( this, IDCANCEL, FDelegate(this,(TDelegate)&WDialog::EndDialogFalse) )
	,	OKButton			( this, IDOK, FDelegate(this,(TDelegate)&WDlgRename::OnOK) )
	,	NameEdit			( this, IDEC_NAME )
	,	GroupEdit			( this, IDEC_GROUPNAME )
	,	PackageEdit			( this, IDEC_PACKAGENAME )
	,	Object				( InObject )
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgRename::OnInitDialog);
		WDialog::OnInitDialog();
		NameEdit.SetText(Object->GetName());
		UObject* Group = Object->GetOuter();
		if (!Group)
		{
			GroupEdit.SetText(TEXT(""));
			PackageEdit.SetText(TEXT(""));
		}
		else if (!Group->GetOuter())
		{
			GroupEdit.SetText(TEXT(""));
			PackageEdit.SetText(Group->GetName());
		}
		else
		{
			while (Group->GetOuter() && Group->GetOuter()->GetOuter())
				Group = Group->GetOuter();
			GroupEdit.SetText(Group->GetName());
			PackageEdit.SetText(Group->GetOuter()->GetName());
		}
		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgRename::OnDestroy);
		WDialog::OnDestroy();
		::DestroyWindow( hWnd );
		unguard;
	}
	virtual int DoModal()
	{
		guard(WDlgNewClass::DoModal);
		return WDialog::DoModal( hInstance );
		unguard;
	}

	void OnOK()
	{
		guard(WDlgRename::OnDestroy);
		FString ObjName = NameEdit.GetText();
		FString GName = GroupEdit.GetText();
		FString PckName = PackageEdit.GetText();

		if (!ObjName.Len() || ObjName == TEXT("None"))
			appMsgf(TEXT("Invalid object name!"));
		else if (!PckName.Len() || PckName == TEXT("None"))
			appMsgf(TEXT("Invalid package name!"));
		else
		{
			UPackage* Pck = UObject::CreatePackage(NULL, *PckName);
			if (GName.Len() && !(GName == TEXT("None")))
				Pck = UObject::CreatePackage(Pck, *GName);
			UObject* OldObj = UObject::StaticFindObject(UObject::StaticClass(), Pck, *ObjName, 0);
			if(OldObj)
				appMsgf(TEXT("New name conflicting with %ls"),OldObj->GetFullName());
			else
			{
				Object->Rename(*ObjName, Pck);
				EndDialogTrue();
			}
		}
		unguard;
	}
	void OnClose()
	{
		Show(0);
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
