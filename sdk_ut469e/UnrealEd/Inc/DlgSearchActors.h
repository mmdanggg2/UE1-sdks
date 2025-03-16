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
	WButton CloseButton, SelectButton;
	WListBox ActorList;
	WEdit NameEdit, EventEdit, TagEdit;
	WLabel FoundLabel;
	WCheckBox WholeWord;

	// Constructor.
	WDlgSearchActors( UObject* InContext, WWindow* InOwnerWindow )
	:	WDialog			( TEXT("Search for Actors"), IDDIALOG_SEARCH, InOwnerWindow )
	,	CloseButton		( this, IDPB_CLOSE,		FDelegate(this,(TDelegate)&WDlgSearchActors::OnClose) )
	,	SelectButton	( this, IDPB_SELECT,		FDelegate(this,(TDelegate)&WDlgSearchActors::OnSelect) )
	,	ActorList		( this, IDLB_NAMES )
	,	NameEdit		( this, IDEC_NAME )
	,	EventEdit		( this, IDEC_EVENT )
	,	TagEdit			( this, IDEC_TAG )
	,	FoundLabel		( this, IDSC_FOUND )
	,	WholeWord		( this, IDCK_WHOLE_WORD, FDelegate(this,(TDelegate)&WDlgSearchActors::RefreshActorList) )
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgSearchActors::OnInitDialog);
		WDialog::OnInitDialog();
		INT Stops[10] = {1500};
		SendMessageW( ActorList.hWnd, LB_SETTABSTOPS, 1, (LPARAM)&Stops); // hide away index of actor
		ActorList.m_bMultiSel = TRUE;
		ActorList.DoubleClickDelegate = FDelegate(this, (TDelegate)&WDlgSearchActors::OnActorListDblClick);
		NameEdit.ChangeDelegate = FDelegate(this, (TDelegate)&WDlgSearchActors::OnNameEditChange);
		EventEdit.ChangeDelegate = FDelegate(this, (TDelegate)&WDlgSearchActors::OnEventEditChange);
		TagEdit.ChangeDelegate = FDelegate(this, (TDelegate)&WDlgSearchActors::OnTagEditChange);
		RefreshActorList();
		unguard;
	}
	virtual void OnShowWindow( UBOOL bShow )
	{
		guard(WDlgSearchActors::OnShowWindow);
		WWindow::OnShowWindow( bShow );
		if (bShow)
			RefreshActorList();
		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgSearchActors::OnDestroy);
		WDialog::OnDestroy();
		::DestroyWindow( hWnd );
		unguard;
	}
	virtual void DoModeless(UBOOL bShow = TRUE)
	{
		guard(WDlgSearchActors::DoModeless);
		AddWindow(this, TEXT("WDlgSearchActors"));
		hWnd = CreateDialogParamW( hInstance, MAKEINTRESOURCEW(IDDIALOG_SEARCH), OwnerWindow?OwnerWindow->hWnd:NULL, (DLGPROC)StaticDlgProc, (LPARAM)this);
		if( !hWnd )
			appGetLastError();
		Show(bShow);
		unguard;
	}
	static BOOL WholeWordMatch(BOOL bWholeWord, const FString& CapitalizedFullString, const FString& CapitalizedSearchString)
	{
		if (bWholeWord)
			return CapitalizedFullString == CapitalizedSearchString;
		return CapitalizedFullString.InStr(CapitalizedSearchString) != -1;
	}
	void RefreshActorList( void )
	{
		guard(WDlgSearchActors::RefreshActorList);		
		ActorList.SetRedraw(false);
		ActorList.Empty();

		HWND hwndFocus = ::GetFocus();

		FString Name = NameEdit.GetText().Caps();
		FString Event = EventEdit.GetText().Caps();
		FString Tag = TagEdit.GetText().Caps();

		// strip trailing spaces (if any)
		while (Name.Right(1) == TEXT(" "))
			Name = Name.Left(Name.Len() - 1);
		while (Event.Right(1) == TEXT(" "))
			Event = Event.Left(Event.Len() - 1);
		while (Tag.Right(1) == TEXT(" "))
			Tag = Tag.Left(Tag.Len() - 1);

		const BOOL bWholeWord = WholeWord.IsChecked();

		INT Total = 0;

		if( GEditor && GEditor->Level )
		{
			FString Buffer;
			const INT NameLen = Name.Len(), EventLen = Event.Len(), TagLen = Tag.Len();
			TTransArray<AActor*> Actors = GEditor->Level->Actors;

			const TCHAR* CharEvent = *Event;
			const TCHAR* TriggerFields[] = {
				TEXT("OutEvents"), // Dispatcher, RoundRobin
				TEXT("Events"), // StochasticTrigger
				TEXT("CodeMasterTag"), // CodeTrigger
				TEXT("FatTag"), // FatnessTrigger
			};
			const TCHAR* MoverFields[] = {
				TEXT("BumpEvent"),
				TEXT("PlayerBumpEvent"),
			};
			const TCHAR* CharTag = *Tag;

			for (INT i = 0, im = Actors.Num(); i < im; ++i)
			{
				AActor* pActor = Actors(i);
				if (!pActor)
					continue;

				Total++;
				FString ActorName = FObjectName(pActor);
				if (NameLen && !WholeWordMatch(bWholeWord, ActorName.Caps(), Name))
					continue;

#define CheckField(CharField, CharValue) {\
	UField* Field = pActor->FindObjectField(CharField);\
	if (!Field)\
		continue;\
	\
	UProperty* Property = Cast<UProperty>(Field);\
	if (!Property)\
		continue;\
	\
	for (INT j = 0, jm = Property->ArrayDim; !Passed && j < jm; j++)\
	{\
		Buffer.Empty();\
		if (Property->ExportText(j, Buffer, (BYTE*)pActor, (BYTE*)pActor, 1) &&\
			WholeWordMatch(bWholeWord, Buffer.Caps(), CharValue))\
		{\
			Passed = TRUE;\
		}\
	}\
}

				if (EventLen)
				{
					FString ActorEvent = FString(*(pActor->Event)).Caps();
					if (!WholeWordMatch(bWholeWord, ActorEvent, Event))
					{
						UBOOL Passed = FALSE;
						ATeleporter* Tele;
						if (Cast<ATriggers>(pActor))
						{
							for (INT k = 0; !Passed && k < ARRAY_COUNT(TriggerFields); k++)
								CheckField(TriggerFields[k], CharEvent)
						}
						else if ((Tele = Cast<ATeleporter>(pActor)) != nullptr &&
							WholeWordMatch(bWholeWord, Tele->URL.Caps(), CharEvent))
						{
							Passed = TRUE;
						}
						else if (Cast<APawn>(pActor)) // FortStandard.DamageEvent[8]
							CheckField(TEXT("DamageEvent"), CharEvent)
						else if (Cast<AMover>(pActor))
						{
							for (INT k = 0; !Passed && k < ARRAY_COUNT(MoverFields); k++)
								CheckField(MoverFields[k], CharEvent)
							if (!Passed)
								CheckField(TEXT("Events"), CharEvent) // GradualMover, MixMover
						}
						if (!Passed)
							CheckField(TEXT("AttachTag"), CharEvent) // Actor, AttachMover, MixMover
						if (!Passed)
							continue;
					}
				}

				if (TagLen)
				{
					FString ActorTag = FString(*(pActor->Tag)).Caps();
					if (!WholeWordMatch(bWholeWord, ActorTag, Tag))
					{
						UBOOL Passed = FALSE;
						if (Cast<AMover>(pActor))
							CheckField(TEXT("Tags"), CharTag) // GradualMover, MixMover
						if (!Passed)
							continue;
					}
				}

				ActorList.AddString(*(FString::Printf(TEXT("%ls\t%d"), *ActorName, i)));
			}
		}

		FoundLabel.SetText(*(FString::Printf(TEXT("Matched %d actors out of %d."), ActorList.GetCount(), Total)));
		
		ActorList.SetRedraw(true);
		::SetFocus( hwndFocus );
		unguard;
	}
	void OnCommand( INT Command )
	{
		guard(WDlgSearchActors::OnCommand);
		switch( Command ) {
			case IDPB_EDIT:
				OnActorListDblClick();
				break;
			
			case IDPB_CLOSE:
				OnClose();
				break;

			case IDPB_SELECT:
				OnSelect();
				break;

			default:
				WDialog::OnCommand(Command);
				break;
		}
		unguard;
	}
	void OnClose()
	{
		guard(WDlgSearchActors::OnClose);
		Show(0);
		unguard;
	}
	void OnSelect()
	{
		guard(WDlgSearchActors::OnSelect);
		GEditor->SelectNone( GEditor->Level, 0 );
		INT SelectedCount = ActorList.GetSelectedCount();
		TArray<INT> Indicies(SelectedCount);
		if (SelectedCount)
			ActorList.GetSelectedItems(SelectedCount, (INT*)Indicies.GetData());
		AActor* Actor = NULL;
		for (INT i = (SelectedCount ? SelectedCount : ActorList.GetCount()) - 1; i >= 0; i--)
		{
			FString Item = ActorList.GetString(SelectedCount ? Indicies(i) : i);
			INT j = Item.Mid(Item.InStr(TEXT("\t")) + 1).Int();
			Actor = GEditor->Level->Actors(j);
			if (Actor)
			{
				Actor->Modify();
				Actor->bSelected = 1;
			}
		}
		GEditor->NoteSelectionChange( GEditor->Level );
		unguard;
	}
	void OnActorListDblClick()
	{
		guard(WDlgSearchActors::OnActorListDblClick);
		if (ActorList.GetCurrent() < 0)
			return;
		GEditor->SelectNone( GEditor->Level, 0 );
		FString Item = ActorList.GetString(ActorList.GetCurrent());
		GEditor->Exec( *(FString::Printf(TEXT("CAMERA ALIGN NAME=%ls"), *(Item.Left(Item.InStr(TEXT("\t"))) ) ) ) );
		GEditor->NoteSelectionChange( GEditor->Level );
		unguard;
	}
	void OnNameEditChange()
	{
		guard(WDlgSearchActors::OnNameEditChange);
		RefreshActorList();
		unguard;
	}
	void OnEventEditChange()
	{
		guard(WDlgSearchActors::OnEventEditChange);
		RefreshActorList();
		unguard;
	}
	void OnTagEditChange()
	{
		guard(WDlgSearchActors::OnTagEditChange);
		RefreshActorList();
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
