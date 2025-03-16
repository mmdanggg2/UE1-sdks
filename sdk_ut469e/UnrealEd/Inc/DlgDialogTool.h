/*=============================================================================
	DialogTool : Base for dialog tool, contain some common code
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Buggie

    Work-in-progress todo's:

=============================================================================*/

class WDialogTool : public WDialog
{
	DECLARE_WINDOWCLASS(WDialogTool, WDialog, UnrealEd)

	// Constructor.
	WDialogTool(FName InPersistentName, INT InDialogId, WWindow* InOwnerWindow = NULL)
	:	WDialog (InPersistentName, InDialogId, InOwnerWindow)
	{
	}

	void OnCommand(INT Command)
	{
		guard(WDialogTool::OnCommand);
		OnCommandStatic(Command);
		unguard;
	}	
	void OnDestroy()
	{
		guard(WDialogTool::OnDestroy);
		WDialog::OnDestroy();
		::DestroyWindow(hWnd);
		unguard;
	}

	static void OnCommandStatic(INT Command)
	{
		guard(WDialogTool::OnCommandStatic);

		HWND hwndFocus = ::GetFocus();
		if (hwndFocus)
		{
			TArray<TCHAR> Buffer(100);
			if (::GetClassName(hwndFocus, &Buffer(0), Buffer.Num() - 1) && FString(TEXT("Edit")) == &Buffer(0))
				return;
		}

		switch(Command)
		{
			case ID_EditUndo:
				GEditor->Exec(TEXT("TRANSACTION UNDO"));
				break;

			case ID_EditRedo:
				GEditor->Exec(TEXT("TRANSACTION REDO"));
				break;
		}
		unguard;
	}
};
