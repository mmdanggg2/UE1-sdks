/*=============================================================================
	ScaleLights : Allows for the scaling of light values
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

class WDlgScaleLights : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgScaleLights,WDialog,UnrealEd)

	// Variables.
	WCheckBox LiteralCheck;
	WButton OKButton, CloseButton;
	WEdit ValueEdit;

	// Constructor.
	WDlgScaleLights( UObject* InContext, WWindow* InOwnerWindow )
	:	WDialog			( TEXT("Scale Lights"), IDDIALOG_SCALE_LIGHTS, InOwnerWindow )
	,	OKButton		( this, IDOK,			FDelegate(this,(TDelegate)&WDlgScaleLights::OnOK) )
	,	CloseButton		( this, IDPB_CLOSE,		FDelegate(this,(TDelegate)&WDlgScaleLights::OnClose) )
	,	LiteralCheck	( this, IDRB_LITERAL )
	,	ValueEdit		( this, IDEC_VALUE )
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgScaleLights::OnInitDialog);
		WDialog::OnInitDialog();
		LiteralCheck.SetCheck( BST_CHECKED );
		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgScaleLights::OnDestroy);
		WDialog::OnDestroy();
		::DestroyWindow( hWnd );
		unguard;
	}
	virtual void DoModeless()
	{
		guard(WDlgScaleLights::DoModeless);
		_Windows.AddItem( this );
		hWnd = CreateDialogParamA( hInstance, MAKEINTRESOURCEA(IDDIALOG_SCALE_LIGHTS), OwnerWindow?OwnerWindow->hWnd:NULL, (DLGPROC)StaticDlgProc, (LPARAM)this);
		if( !hWnd )
			appGetLastError();
		Show(1);
		unguard;
	}
	void OnClose()
	{
		guard(WDlgScaleLights::OnClose);
		Show(0);
		unguard;
	}
	void OnOK()
	{
		guard(WDlgScaleLights::OnTagEditChange);
		INT NumSelected = 0;
		for (FActorIterator It(GEditor->Level); It; ++It)
		{
			if (It->bSelected)
			{
				++NumSelected;
				break;
			}
		}
		if (!NumSelected)
		{
			appMsgf(TEXT("Can't scale lights, no actors selected!"));
			return;
		}

		UBOOL bLiteral = LiteralCheck.IsChecked();
		int Value = appAtoi( *(ValueEdit.GetText()) );
		FLOAT fValue;

		FString Task;
		if (bLiteral)
		{
			if (Value < 0)
				Task = FString::Printf(TEXT("%i"), Value);
			else Task = FString::Printf(TEXT("+%i"), Value);
		}
		else
		{
			fValue = Clamp<FLOAT>(Value, 0.f, 400.f) / 100.f;
			Task = FString::Printf(TEXT("X %i %%"), Clamp(Value, 0, 400));
		}

		if (GWarn->YesNof(TEXT("This will scale all selected actors LightBrightness %ls\nAre you sure you want to proceed?"), *Task))
		{
			FString Cmd;
			if (bLiteral)
				Cmd = FString::Printf(TEXT("LIGHT SCALE LITERAL VALUE=%i"), Value);
			else Cmd = FString::Printf(TEXT("LIGHT SCALE VALUE=%f"), fValue);
			GEditor->Exec(*Cmd);
		}
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
