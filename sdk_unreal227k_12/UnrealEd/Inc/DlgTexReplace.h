/*=============================================================================
	TexReplace : Replace one texture with another in the level
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>

class WDlgTexReplace : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgTexReplace,WDialog,UnrealEd)

	// Variables.
	WButton Set1Button, Set2Button, ReplaceButton;
	WLabel TexName1Label, TexName2Label;

	UViewport *pViewport1, *pViewport2;
	UTexture *pTexture1, *pTexture2;

	// Constructor.
	WDlgTexReplace( UObject* InContext, WWindow* InOwnerWindow )
	:	WDialog		( TEXT("Replace Textures"), IDDIALOG_TEX_REPLACE, InOwnerWindow )
	,	Set1Button	( this, IDPB_SET1, FDelegate(this,(TDelegate)&WDlgTexReplace::OnSet1Button) )
	,	Set2Button	( this, IDPB_SET2, FDelegate(this,(TDelegate)&WDlgTexReplace::OnSet2Button) )
	,	ReplaceButton	( this, IDPB_REPLACE, FDelegate(this,(TDelegate)&WDlgTexReplace::OnReplaceButton) )
	,	TexName1Label	(this, IDSC_TEX_NAME1 )
	,	TexName2Label	(this, IDSC_TEX_NAME2 )
	{
		pViewport1 = pViewport2 = NULL;
		pTexture1 = pTexture2 = NULL;
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgTexReplace::OnInitDialog);
		WDialog::OnInitDialog();

		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgTexReplace::OnDestroy);
		WDialog::OnDestroy();

		::DestroyWindow( hWnd );

		GEditor->Exec( TEXT("CAMERA CLOSE NAME=TEXREPLACE1") );
		delete pViewport1;
		GEditor->Exec( TEXT("CAMERA CLOSE NAME=TEXREPLACE2") );
		delete pViewport2;

		unguard;
	}
	void OnClose()
	{
		Show(0);
	}
	virtual void DoModeless()
	{
		guard(WDlgTexReplace::DoModeless);
		_Windows.AddItem( this );
		hWnd = CreateDialogParamA( hInstance, MAKEINTRESOURCEA(IDDIALOG_TEX_REPLACE), OwnerWindow?OwnerWindow->hWnd:NULL, (DLGPROC)StaticDlgProc, (LPARAM)this);
		if( !hWnd )
			appGetLastError();
		Show(1);
		unguard;
	}
	void Refresh()
	{
		guard(WDlgTexReplace::Refresh);

		::LockWindowUpdate( hWnd );

		if (pTexture1)
		{
			GEditor->Exec(TEXT("CAMERA CLOSE NAME=TEXREPLACE1"));
			delete pViewport1;

			pViewport1 = GEditor->Client->NewViewport(TEXT("TEXREPLACE1"));
			GEditor->Level->SpawnViewActor(pViewport1);
			pViewport1->Input->Init(pViewport1);
			check(pViewport1->Actor);
			pViewport1->Actor->ShowFlags = SHOW_StandardView | SHOW_NoButtons | SHOW_ChildWindow | SHOW_RealTime;
			pViewport1->Actor->RendMap = REN_TexView;
			pViewport1->Group = NAME_None;
			pViewport1->MiscRes = pTexture1;

			RECT rect;
			if (::GetWindowRect(GetDlgItem(hWnd, IDSC_TEXTURE1), &rect))
			{
				int Width = min(rect.right - rect.left, pTexture1->USize),
					Height = min(rect.bottom - rect.top, pTexture1->VSize);
				::ScreenToClient(hWnd, &(*((POINT*)&rect)));
				pViewport1->OpenWindow(hWnd, 0, Width, Height, rect.left, rect.top);
			}
		}

		if( pTexture2 )
		{
			GEditor->Exec( TEXT("CAMERA CLOSE NAME=TEXREPLACE2") );
			delete pViewport2;

			pViewport2 = GEditor->Client->NewViewport( TEXT("TEXREPLACE2") );
			GEditor->Level->SpawnViewActor( pViewport2 );
			pViewport2->Input->Init( pViewport2 );
			check(pViewport2->Actor);
			pViewport2->Actor->ShowFlags = SHOW_StandardView | SHOW_NoButtons | SHOW_ChildWindow | SHOW_RealTime;
			pViewport2->Actor->RendMap   = REN_TexView;
			pViewport2->Group = NAME_None;
			pViewport2->MiscRes = pTexture2;

			RECT rect;
			if (::GetWindowRect(GetDlgItem(hWnd, IDSC_TEXTURE2), &rect))
			{
				int Width = min(rect.right - rect.left, pTexture2->USize),
					Height = min(rect.bottom - rect.top, pTexture2->VSize);
				::ScreenToClient(hWnd, &(*((POINT*)&rect)));
				pViewport2->OpenWindow(hWnd, 0, Width, Height, rect.left, rect.top);
			}
		}

		::LockWindowUpdate( NULL );

		unguard;
	}
	void OnSet1Button()
	{
		guard(WDlgTexReplace::OnSet1Button);
		pTexture1 = GEditor->CurrentTexture;
		FString Name = pTexture1->GetFullName();
		Name = Name.Right( Name.Len() - 8 );
		TexName1Label.SetText( *Name );
		Refresh();
		unguard;
	}
	void OnSet2Button()
	{
		guard(WDlgTexReplace::OnSet2Button);
		pTexture2 = GEditor->CurrentTexture;
		FString Name = pTexture2->GetFullName();
		Name = Name.Right( Name.Len() - 8 );
		TexName2Label.SetText( *Name );
		Refresh();
		unguard;
	}
	void OnReplaceButton()
	{
		guard(WDlgTexReplace::OnReplaceButton);

		GEditor->Trans->Begin( TEXT("Replace Textures") );

		for( TArray<AActor*>::TIterator ItA(GEditor->Level->Actors); ItA; ++ItA )
		{
			AActor* Actor = *ItA;
			if( Actor )
			{
				UModel* M = Actor->IsA(ALevelInfo::StaticClass()) ? GEditor->Level->Model : Actor->Brush;
				if( M )
				{
					M->Surfs.ModifyAllItems();
					for( TArray<FBspSurf>::TIterator ItS(M->Surfs); ItS; ++ItS )
						if( ItS->Texture==pTexture1 )
						{
							ItS->Texture = pTexture2;
						}
					if( M->Polys )
					{
						M->Polys->Element.ModifyAllItems();
						for( TArray<FPoly>::TIterator ItP(M->Polys->Element); ItP; ++ItP )
							if( ItP->Texture==pTexture1 )
							{
								ItP->Texture = pTexture2;
							}
					}
				}
			}
		}
		GEditor->Trans->End();

		GEditor->RedrawLevel(NULL);

		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
