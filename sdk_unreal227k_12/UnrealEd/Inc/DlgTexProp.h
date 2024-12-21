/*=============================================================================
	TexProp : Properties of a texture
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>
#include "UnFont.h"

int GLastViewportNum = -1;

class WDlgTexProp : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgTexProp,WDialog,UnrealEd)

	// Variables.
	WButton ClearButton;

	FString ViewportName;
	UViewport *pViewport;
	UTexture *pTexture;
	UFont* pFont;
	WObjectProperties* pProps;
	UObject* pObject;
	INT OldU, OldV, MaxSize;
	POINT l_point;
	WCheckBox* pStretchCheck;
	WCheckBox* pShowBgCheck;

	// Constructor.
	WDlgTexProp( UObject* InContext, WWindow* InOwnerWindow, UObject* InTexture )
	:	WDialog		( TEXT("Texture Properties"), IDDIALOG_TEX_PROP, InOwnerWindow )
	,	ClearButton	( this, IDPB_CLEAR, FDelegate(this,(TDelegate)&WDlgTexProp::OnClear) )
	,	pStretchCheck(nullptr)
	{
		ViewportName = FString::Printf( TEXT("TextureProp%d"), ++GLastViewportNum );
		pTexture = Cast<UTexture>(InTexture);
		pFont = Cast<UFont>(InTexture);
		pObject = InTexture;
		pViewport = NULL;
		pProps = new WObjectProperties(NAME_None, CPF_Edit, TEXT(""), this, pTexture != NULL);
		pProps->ShowTreeLines = 1;
	}

	inline void UpdateCaption()
	{
		FString Caption;
		if (pFont)
			Caption = FString::Printf(TEXT("%ls Properties - %ls"),
				pFont->GetClass()->GetName(),
				pFont->GetPathName());
		else
			Caption = FString::Printf(TEXT("%ls Properties - %ls (%dx%d)"),
				pTexture->GetClass()->GetName(),
				pTexture->GetPathName(),
				pTexture->USize,
				pTexture->VSize);
		SetText(*Caption);
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgTexProp::OnInitDialog);
		WDialog::OnInitDialog();

		pProps->OpenChildWindow( IDSC_PROPS );
		pProps->Root.SetObjects( (UObject**)&pObject, 1 );

		MaxSize = MulDiv(256, DPIY, 96);

		// Create the texture viewport
		//
		pViewport = GEditor->Client->NewViewport( *ViewportName );
		GEditor->Level->SpawnViewActor( pViewport );
		pViewport->Input->Init( pViewport );
		check(pViewport->Actor);
		pViewport->Actor->ShowFlags = SHOW_StandardView | SHOW_NoButtons | SHOW_ChildWindow | SHOW_RealTime;
		pViewport->Actor->RendMap   = REN_TexView;
		pViewport->Group = NAME_None;
		pViewport->MiscRes = pObject;
		pViewport->Actor->bAdmin = TRUE; // Alias for show background.

		RECT l_rc;
		::GetWindowRect( GetDlgItem( hWnd, IDSC_TEXTURE ), &l_rc );
		l_point.x = l_rc.left;		l_point.y = l_rc.top;
		::ScreenToClient( hWnd, &l_point );
		UBOOL bStartStreched = (pTexture && pTexture->GetClass() != UTexture::StaticClass() && pTexture->USize == 1 && pTexture->VSize == 1); // Constant color of sort, start stretched.

		if (pTexture && !bStartStreched)
		{
			OldU = Min(MaxSize, pTexture->USize);
			OldV = Min(MaxSize, pTexture->VSize);
		}
		else
		{
			OldU = OldV = MaxSize;
		}
		FString TexRend;
		GConfig->GetString(TEXT("Texture Browser"), TEXT("Device"), TexRend, GUEdIni);
		pViewport->OpenWindow(hWnd, 0, OldU, OldV, l_point.x, l_point.y, TexRend.Len() ? *TexRend : nullptr);

		UpdateCaption();
		INT xPos = MulDiv(94, DPIX, 96);

		if (pTexture)
		{
			SetTimer(hWnd, 0, 30, NULL);

			pStretchCheck = new WCheckBox(this, IDCK_OBJECTS);
			pStretchCheck->ClickDelegate = FDelegate(this, (TDelegate)&WDlgTexProp::OnStretchClick);
			pStretchCheck->OpenWindow(1, 0, 0, 1, 1, TEXT("Stretch To Fit"));
			SendMessageW(pStretchCheck->hWnd, BM_SETCHECK, bStartStreched ? BST_CHECKED : BST_UNCHECKED, 0);
			::MoveWindow(pStretchCheck->hWnd, xPos, l_point.x + MaxSize + MulDiv(26, DPIY, 96), MulDiv(96, DPIX, 96), MulDiv(20, DPIY, 96), 1);
			xPos += MulDiv(96 + 8, DPIX, 96);
		}
		pShowBgCheck = new WCheckBox(this, IDCK_OBJECTS);
		pShowBgCheck->ClickDelegate = FDelegate(this, (TDelegate)&WDlgTexProp::OnShowBgClick);
		pShowBgCheck->OpenWindow(1, 0, 0, 1, 1, TEXT("Show background"));
		SendMessageW(pShowBgCheck->hWnd, BM_SETCHECK, BST_CHECKED, 0);
		::MoveWindow(pShowBgCheck->hWnd, xPos, l_point.x + MaxSize + MulDiv(26, DPIY, 96), MulDiv(120, DPIX, 96), MulDiv(20, DPIY, 96), 1);

		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgTexProp::OnDestroy);
		WDialog::OnDestroy();
		delete pProps;
		::DestroyWindow( hWnd );

		TCHAR l_ch[256];
		appSprintf( l_ch, TEXT("CAMERA CLOSE NAME=%ls"), *ViewportName );
	    GEditor->Exec( l_ch );
		delete pViewport;
		if (pStretchCheck)
			delete pStretchCheck;
		delete pShowBgCheck;

		unguard;
	}
	void OnStretchClick()
	{
		guard(WDlgTexProp::OnStretchClick);
		if (pStretchCheck->IsChecked())
		{
			if (OldV < MaxSize || OldV < MaxSize)
				ResizeViewport();
		}
		else if (OldV < MaxSize || OldV < MaxSize)
			ResizeViewport();
		unguard;
	}
	void OnShowBgClick()
	{
		guard(WDlgTexProp::OnStretchClick);
		pViewport->Actor->bAdmin = pShowBgCheck->IsChecked();
		unguard;
	}
	virtual void DoModeless()
	{
		guard(WDlgTexProp::DoModeless);
		_Windows.AddItem( this );
		hWnd = CreateDialogParamA( hInstance, MAKEINTRESOURCEA(IDDIALOG_TEX_PROP), OwnerWindow?OwnerWindow->hWnd:NULL, (DLGPROC)StaticDlgProc, (LPARAM)this);
		if( !hWnd )
			appGetLastError();
		Show(1);
		unguard;
	}
	void OnClear()
	{
		guard(WDlgTexProp::OnClear);
		if (pTexture)
			GEditor->Exec( *(FString::Printf( TEXT("TEXTURE CLEAR NAME=%ls"), pTexture->GetPathName() )) );
		unguard;
	}
	void ResizeViewport()
	{
		INT XS = OldU;
		INT YS = OldV;
		if (pStretchCheck && pStretchCheck->IsChecked())
			XS = YS = MaxSize;
		pViewport->OpenWindow(hWnd, 0, XS, YS, l_point.x, l_point.y);
	}
	void OnTimer()
	{
		if (pTexture)
		{
			INT NewU = Min(MaxSize, pTexture->USize);
			INT NewV = Min(MaxSize, pTexture->VSize);

			if (NewU != OldU || NewV != OldV)
			{
				OldU = NewU;
				OldV = NewV;
				ResizeViewport();
				UpdateCaption();
			}
		}
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
