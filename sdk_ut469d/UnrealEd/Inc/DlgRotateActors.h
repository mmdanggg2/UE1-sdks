/*=============================================================================
	RotateActors : Random rotate selected actors
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Buggie

    Work-in-progress todo's:

=============================================================================*/

class WDlgRotateActors : public WDialogTool
{
	DECLARE_WINDOWCLASS(WDlgRotateActors,WDialogTool,UnrealEd)

	// Variables.
	WButton OKButton, CloseButton;
	WEdit PitchMin;
	WEdit PitchMax;
	WEdit YawMin;
	WEdit YawMax;
	WEdit RollMin;
	WEdit RollMax;

	// Constructor.
	WDlgRotateActors( UObject* InContext, WWindow* InOwnerWindow )
	:	WDialogTool		( TEXT("RotateActors"), IDDIALOG_ROTATE_ACTORS, InOwnerWindow )
	,	OKButton		( this, IDOK,			FDelegate(this,(TDelegate)&WDlgRotateActors::OnOK) )
	,	CloseButton		( this, IDPB_CLOSE,		FDelegate(this,(TDelegate)&WDlgRotateActors::OnClose) )
	,	PitchMin		( this, IDEC_PITCH_MIN )
	,	PitchMax		( this, IDEC_PITCH_MAX )
	,	YawMin			( this, IDEC_YAW_MIN )
	,	YawMax			( this, IDEC_YAW_MAX )
	,	RollMin			( this, IDEC_ROLL_MIN )
	,	RollMax			( this, IDEC_ROLL_MAX )
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgRotateActors::OnInitDialog);
		WDialogTool::OnInitDialog();

		PitchMin.SetText(TEXT("0"));
		YawMin.SetText(TEXT("0"));
		RollMin.SetText(TEXT("0"));
		PitchMax.SetText(TEXT("65535"));
		YawMax.SetText(TEXT("65535"));
		RollMax.SetText(TEXT("65535"));

		unguard;
	}
	virtual void DoModeless()
	{
		guard(WDlgRotateActors::DoModeless);
		AddWindow(this, TEXT("WDlgRotateActors"));
		hWnd = CreateDialogParamA( hInstance, MAKEINTRESOURCEA(IDDIALOG_ROTATE_ACTORS), OwnerWindow?OwnerWindow->hWnd:NULL, (DLGPROC)StaticDlgProc, (LPARAM)this);
		if( !hWnd )
			appGetLastError();
		Show(1);
		unguard;
	}
	void OnClose()
	{
		guard(WDlgRotateActors::OnClose);
		Show(0);
		unguard;
	}
	void OnOK()
	{
		guard(WDlgRotateActors::OnOK);

		INT iPitchMin = appAtoi(*PitchMin.GetText());
		INT iPitchMax = appAtoi(*PitchMax.GetText());
		INT iYawMin = appAtoi(*YawMin.GetText());
		INT iYawMax = appAtoi(*YawMax.GetText());
		INT iRollMin = appAtoi(*RollMin.GetText());
		INT iRollMax = appAtoi(*RollMax.GetText());

		GEditor->Trans->Begin( TEXT("Range rotate actors") );
		GEditor->Level->Modify();

		for (INT i = 0; i < GEditor->Level->Actors.Num(); i++)
		{
			AActor* Actor = GEditor->Level->Actors(i);
			if( Actor && Actor->bSelected )
			{
				Actor->Modify();
				// order is irrelevant since appFrand() is equally distributed between 0 and 1.
				Actor->Rotation += FRotator(
					iPitchMin + (iPitchMax - iPitchMin)*appFrand(),
					iYawMin + (iYawMax - iYawMin)*appFrand(),
					iRollMin + (iRollMax - iRollMin)*appFrand()
				);
			}
		}

		GEditor->Trans->End();

		GEditor->NoteSelectionChange( GEditor->Level );
		GEditor->RedrawLevel( GEditor->Level );

		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
