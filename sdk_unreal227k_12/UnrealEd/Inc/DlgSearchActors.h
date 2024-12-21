/*=============================================================================
	SearchActors : Searches for actors using various criteria
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

class WDlgSearchActors : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgSearchActors,WDialog,UnrealEd)

	// Variables.
	WButton CloseButton;
	WListBox ActorList;
	WEdit NameEdit, EventEdit, TagEdit, AnyEdit, PropNameEdit, PropValueEdit;
	WCheckBox ShowHidden;

	// Constructor.
	WDlgSearchActors( UObject* InContext, WWindow* InOwnerWindow )
	:	WDialog			( TEXT("Search for Actors"), IDDIALOG_SEARCH, InOwnerWindow )
	,	CloseButton		( this, IDPB_CLOSE,		FDelegate(this,(TDelegate)&WDlgSearchActors::OnClose) )
	,	NameEdit		( this, IDEC_NAME )
	,	EventEdit		( this, IDEC_EVENT )
	,	TagEdit			( this, IDEC_TAG )
	,	ActorList		( this, IDLB_NAMES )
	,	AnyEdit			( this, IDEC_ANYN )
	,	PropNameEdit	( this, IDEC_PROPNAME )
	,	PropValueEdit	( this, IDEC_PROPVALUE )
	,	ShowHidden		( this, IDC_SHOWHIDDEN )
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgSearchActors::OnInitDialog);
		WDialog::OnInitDialog();
		ActorList.DoubleClickDelegate = FDelegate(this, (TDelegate)&WDlgSearchActors::OnActorListDblClick);
		NameEdit.ChangeDelegate = FDelegate(this, (TDelegate)&WDlgSearchActors::OnEditChange);
		EventEdit.ChangeDelegate = FDelegate(this, (TDelegate)&WDlgSearchActors::OnEditChange);
		TagEdit.ChangeDelegate = FDelegate(this, (TDelegate)&WDlgSearchActors::OnEditChange);
		AnyEdit.ChangeDelegate = FDelegate(this, (TDelegate)&WDlgSearchActors::OnEditChange);
		//PropNameEdit.ChangeDelegate = FDelegate(this, (TDelegate)&WDlgSearchActors::OnEditChange);
		PropValueEdit.ChangeDelegate = FDelegate(this, (TDelegate)&WDlgSearchActors::OnEditChange);
		ShowHidden.ClickDelegate = FDelegate(this, (TDelegate)&WDlgSearchActors::OnEditChange);
		RefreshActorList();
		unguard;
	}
	virtual void OnShowWindow( UBOOL bShow )
	{
		guard(WDlgSearchActors::OnShowWindow);
		WWindow::OnShowWindow( bShow );
		RefreshActorList();
		unguard;
	}
	void OnClose()
	{
		guard(WDlgSearchActors::OnClose);
		Show(0);
		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgSearchActors::OnDestroy);
		WDialog::OnDestroy();
		::DestroyWindow( hWnd );
		unguard;
	}
	virtual void DoModeless()
	{
		guard(WDlgSearchActors::DoModeless);
		_Windows.AddItem( this );
		hWnd = CreateDialogParamA( hInstance, MAKEINTRESOURCEA(IDDIALOG_SEARCH), OwnerWindow?OwnerWindow->hWnd:NULL, (DLGPROC)StaticDlgProc, (LPARAM)this);
		if( !hWnd )
			appGetLastError();
		Show(1);
		unguard;
	}
	inline UBOOL NameMatch(const TCHAR* A, const TCHAR* B)
	{
		if (!*A)
			return TRUE;
		while (1)
		{
			while (*B && appToUpper(*B) != *A)
				++B;
			if (!*B)
				return FALSE;
			const TCHAR* tA = A + 1;
			const TCHAR* tB = B + 1;
			while (*tA && *tA == appToUpper(*tB))
			{
				++tA;
				++tB;
			}
			if (!*tA)
				return TRUE;
			++B;
		}
	}
	void RefreshActorList( void )
	{
		guard(WDlgSearchActors::RefreshActorList);
		ActorList.Empty();
		LockWindowUpdate( ActorList.hWnd );

		FString Name, Event, Tag, Any, PropValue;
		HWND hwndFocus = ::GetFocus();

		Name = NameEdit.GetText().Caps();
		Event = EventEdit.GetText().Caps();
		Tag = TagEdit.GetText().Caps();
		Any = AnyEdit.GetText().Caps();
		FName PropName = FName(*PropNameEdit.GetText(), FNAME_Find);
		if (PropName != NAME_None)
			PropValue = PropValueEdit.GetText().Caps();
		TMap<UClass*, UProperty*> PropMap;
		UProperty** FindProp;
		UProperty* P;
		UBOOL bShowHidden = ShowHidden.IsChecked();

		if( GEditor && GEditor->Level )
		{
			for( int i = 0 ; i < GEditor->Level->Actors.Num() ; i++ )
			{
				AActor* pActor = GEditor->Level->Actors(i);
				if( pActor && (bShowHidden || !pActor->bHiddenEd) )
				{
					if(!NameMatch(*Name, pActor->GetName()) || !NameMatch(*Event, *pActor->Event) || !NameMatch(*Tag, *pActor->Tag))
						continue;
					if (Any.Len())
					{
						UBOOL bFound = FALSE;
						UNameProperty* N;
						for (TFieldIterator<UNameProperty> It(pActor->GetClass()); (It && !bFound); ++It)
						{
							N = *It;
							FName* Data = reinterpret_cast<FName*>(reinterpret_cast<BYTE*>(pActor) + N->Offset);
							for (INT j = (N->ArrayDim - 1); j >= 0; --j)
							{
								if (Data[j] != NAME_None && NameMatch(*Any, *Data[j]))
								{
									bFound = TRUE;
									break;
								}
							}
						}
						if (!bFound)
							continue;
					}
					if (PropName != NAME_None)
					{
						FindProp = PropMap.Find(pActor->GetClass());
						if (FindProp)
							P = *FindProp;
						else
						{
							P = FindField<UProperty>(pActor->GetClass(), *PropName);
							PropMap.Set(pActor->GetClass(), P);
						}
						if (!P)
							continue;
						UBOOL bFound = FALSE;
						BYTE* Data = reinterpret_cast<BYTE*>(pActor) + P->Offset;
						for (INT j = (P->ArrayDim - 1); j >= 0; --j)
						{
							FString Value;
							P->ExportTextItem(Value, Data + (j * P->ElementSize), NULL, 0);
							if (NameMatch(*PropValue, *Value))
							{
								bFound = TRUE;
								break;
							}
						}
						if (!bFound)
							continue;
					}
					ActorList.AddString( pActor->GetName() );
				}
			}
		}

		LockWindowUpdate( NULL );
		::SetFocus( hwndFocus );
		unguard;
	}
	void OnActorListDblClick()
	{
		guard(WDlgSearchActors::OnActorListDblClick);
		GEditor->SelectNone( GEditor->Level, 0 );
		GEditor->Exec( *(FString::Printf(TEXT("CAMERA ALIGN NAME=%ls"), *(ActorList.GetString( ActorList.GetCurrent()) ) ) ) );
		GEditor->NoteSelectionChange( GEditor->Level );
		unguard;
	}
	void OnEditChange()
	{
		guard(WDlgSearchActors::OnEditChange);
		RefreshActorList();
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
