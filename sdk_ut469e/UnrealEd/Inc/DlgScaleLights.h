/*=============================================================================
	ScaleLights : Allows for the scaling of light values
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

class WDlgScaleLights : public WDialogTool
{
	DECLARE_WINDOWCLASS(WDlgScaleLights,WDialogTool,UnrealEd)

	// Variables.
	WCheckBox LiteralCheck;
	WButton OKButton, CloseButton;
	WEdit ValueEdit;

	// Constructor.
	WDlgScaleLights( UObject* InContext, WWindow* InOwnerWindow )
	:	WDialogTool		( TEXT("Scale Lights"), IDDIALOG_SCALE_LIGHTS, InOwnerWindow )
	,	LiteralCheck	( this, IDRB_LITERAL )
	,	OKButton		( this, IDOK,			FDelegate(this,(TDelegate)&WDlgScaleLights::OnOK) )
	,	CloseButton		( this, IDPB_CLOSE,		FDelegate(this,(TDelegate)&WDlgScaleLights::OnClose) )
	,	ValueEdit		( this, IDEC_VALUE )
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgScaleLights::OnInitDialog);
		WDialogTool::OnInitDialog();
		LiteralCheck.SetCheck( BST_CHECKED );
		unguard;
	}
	virtual void DoModeless(UBOOL bShow = TRUE)
	{
		guard(WDlgScaleLights::DoModeless);
		AddWindow(this, TEXT("WDlgScaleLights"));
		hWnd = CreateDialogParamW( hInstance, MAKEINTRESOURCEW(IDDIALOG_SCALE_LIGHTS), OwnerWindow?OwnerWindow->hWnd:NULL, (DLGPROC)StaticDlgProc, (LPARAM)this);
		if( !hWnd )
			appGetLastError();
		Show(bShow);
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
		guard(WDlgScaleLights::OnOK);
		UBOOL bLiteral = LiteralCheck.IsChecked();
		int Value = appAtoi( *(ValueEdit.GetText()) );

		GEditor->Trans->Begin(TEXT("Scale Lights"));

		// Loop through all selected actors and scale their light value by the specified amount.
		for (int i = 0; i < GEditor->Level->Actors.Num(); i++)
		{
			AActor* pActor = GEditor->Level->Actors(i);
			if (pActor && pActor->bSelected)
			{
				int iLightBrighness = pActor->LightBrightness;
				if (bLiteral)
				{
					iLightBrighness += Value;
				}
				else
				{
					Value = Clamp(Value, -100, 100);
					iLightBrighness += (int)((float)iLightBrighness * ((float)Value / 100.0f));

				}

				iLightBrighness = Clamp(iLightBrighness, 0, 255);
				pActor->Modify();
				pActor->LightBrightness = iLightBrighness;
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
