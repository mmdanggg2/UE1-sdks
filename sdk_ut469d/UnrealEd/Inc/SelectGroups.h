#/*=============================================================================
	SelectGroups : Edit the groups for the selected actors
	Copyright 2020, OldUnreal. All Rights Reserved.

	Revision history:
		* Created by Stijn Volckaert
=============================================================================*/

class WSelectGroups : public WWindow
{
	DECLARE_WINDOWCLASS(WSelectGroups, WWindow, UnrealEd)

	// Variables.
	WCheckListBox* GroupList{};
	WListBox* ActorList{};
	WLabel* GroupLabel{};
	WLabel* ActorLabel{};
	WButton* RefreshButton{};
	WButton* ClearSelectionButton{};

	// Constructor.
	WSelectGroups(FName InPersistentName, WWindow* InOwnerWindow)
		: WWindow(InPersistentName, InOwnerWindow)
	{
	}

	// WWindow Interface
	void OpenWindow()
	{
		MdiChild = 0;

		PerformCreateWindowEx
		(
			0,
			NULL,
			WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
			0,
			0,
			MulDiv(640, DPIX, 96),
			MulDiv(480, DPIY, 96),
			OwnerWindow ? OwnerWindow->hWnd : NULL,
			NULL,
			hInstance
		);
		SendMessage(*this, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(0, 0));
		SetText(TEXT("Select Groups"));
	}

	void OnClose()
	{
		Show(0);
	}

	void OnDestroy()
	{
		delete ActorList;
		delete GroupList;
		delete ActorLabel;
		delete GroupLabel;
		delete RefreshButton;
		delete ClearSelectionButton;
		WWindow::OnDestroy();
	}
	
	void OnCreate()
	{
		guard(WSelectGroups::OnCreate);
		WWindow::OnCreate();

		ActorLabel = new WLabel(this);
		ActorLabel->OpenWindow(1, 0);
		ActorLabel->SetText(TEXT("Selected Actors"));

		GroupLabel = new WLabel(this);
		GroupLabel->OpenWindow(1, 0);
		GroupLabel->SetText(TEXT("Assigned Groups"));

		ActorList = new WListBox(this, IDLB_ACTORS);
		ActorList->OpenWindow(1, 0, 0, 0, 1);

		GroupList = new WCheckListBox(this, IDLB_GROUPS, 0, 1);
		GroupList->ItemModifiedDelegate = FDelegateInt(this, (TDelegateInt)&WSelectGroups::UpdateGroup);
		GroupList->OpenWindow(1, 0, 0, 1);

		RefreshButton = new WButton(this, IDBM_REFRESH_GROUPS, FDelegate(this, (TDelegate)&WSelectGroups::RefreshGroupsList));
		RefreshButton->OpenWindow(1, 0, 0, 0, 0, TEXT("Refresh selection"));

		ClearSelectionButton = new WButton(this, IDBM_CLEAR_SELECTION, FDelegate(this, (TDelegate)&WSelectGroups::ClearSelection));
		ClearSelectionButton->OpenWindow(1, 0, 0, 0, 0, TEXT("Clear selection"));
		
		unguard;
	}
	
	void OnSize(DWORD Flags, INT NewX, INT NewY)
	{
		guard(WSelectGroups::OnSize);
		WWindow::OnSize(Flags, NewX, NewY);
		PositionChildControls();
		InvalidateRect(hWnd, NULL, FALSE);
		unguard;
	}
	
	void OnShowWindow(UBOOL bShow) 
	{
		guard(WSelectGroups::OnShowWindow);
		WWindow::OnShowWindow(bShow);
		if (bShow)
			RefreshGroupsList();
		unguard;
	}

	void PositionChildControls()
	{
		if (!ActorList || !GroupList || !ActorLabel || !GroupLabel)
			return;

		FRect CR = GetClientRect();

		::MoveWindow(GroupLabel->hWnd,
			MulDiv(9, DPIX, 96),
			MulDiv(9, DPIY, 96),
			MulDiv(100, DPIX, 96),
			MulDiv(16, DPIY, 96),
			1);

		::MoveWindow(ActorLabel->hWnd,
			CR.Width() / 2 + MulDiv(9, DPIX, 96),
			MulDiv(9, DPIY, 96),
			MulDiv(100, DPIX, 96),
			MulDiv(16, DPIY, 96),
			1);

		::MoveWindow(ActorList->hWnd,
			CR.Width() / 2 + MulDiv(9, DPIX, 96),
			MulDiv(30, DPIY, 96),
			CR.Width() / 2 - MulDiv(18, DPIX, 96),
			CR.Height() - MulDiv(66, DPIY, 96),
			1);

		::MoveWindow(GroupList->hWnd,
			MulDiv(9, DPIX, 96),
			MulDiv(30, DPIY, 96),
			CR.Width() / 2 - MulDiv(18, DPIX, 96),
			CR.Height() - MulDiv(66, DPIY, 96),
			1);

		::MoveWindow(RefreshButton->hWnd,
			CR.Width() / 2 + MulDiv(9, DPIX, 96),
			CR.Height() - MulDiv(27, DPIY, 96),
			MulDiv(100, DPIX, 96),
			MulDiv(19, DPIY, 96),
			1);

		::MoveWindow(ClearSelectionButton->hWnd,
			CR.Width() / 2 - MulDiv(109, DPIX, 96),
			CR.Height() - MulDiv(27, DPIY, 96),
			MulDiv(100, DPIX, 96),
			MulDiv(19, DPIY, 96),
			1);

		::InvalidateRect(hWnd, NULL, 1);
	}

	void OnPaint()
	{
		guard(WSelectGroups::OnPaint);
		PAINTSTRUCT PS;
		HDC hDC = BeginPaint(*this, &PS);
		FillRect(hDC, GetClientRect(), (HBRUSH)(COLOR_BTNFACE + 1));
		EndPaint(*this, &PS);
		unguard;
	}

	void UpdateGroup(INT Item)
	{
		if (!GroupList || !ActorList)
			return;

		FString ModifiedGroup = GroupList->GetString(Item);
		INT UpdatedState = reinterpret_cast<INT>(GroupList->GetItemData(Item));

		//debugf(TEXT("Modified group %ls modified state %d"), *ModifiedGroup, UpdatedState);

		FString Errors;

		if (GEditor && GEditor->Level)
		{
			for (int i = 0; i < GEditor->Level->Actors.Num(); i++)
			{
				AActor* pActor = GEditor->Level->Actors(i);
				if (pActor && pActor->bSelected)
				{
					//debugf(TEXT("Actor %ls current groups %ls"), *FObjectFullName(pActor), *pActor->Group);
					TArray<FString> Groups;
					BOOL Modified = 0;
					if (pActor->Group != NAME_None)
					{
						for (TStringIterator It(*pActor->Group, ','); It.IsValid; It.Next())
						{
							new(Groups) FString(*It.Value);
							//debugf(TEXT("Parsed %ls"), *It.Value);
						}
					}

					if (UpdatedState == 0)
					{
						Modified = Groups.RemoveItem(ModifiedGroup) > 0;
					}
					else
					{
						INT OriginalCount = Groups.Num();
						Modified = Groups.AddUniqueItem(ModifiedGroup) == OriginalCount;
					}

					if (Modified)
					{
						FString NewGroupName;
						for (INT i = 0; i < Groups.Num(); ++i)
						{
							if (i > 0)
								NewGroupName += TEXT(",");
							NewGroupName += Groups(i);
						}

						if (NewGroupName.Len() < NAME_SIZE)
						{
							pActor->Modify();
							pActor->Group = NewGroupName.Len() > 0 ? *NewGroupName : TEXT("None");
							//debugf(TEXT("Actor %ls new groups %ls"), *FObjectFullName(pActor), *NewGroupName);
						}
						else
						{
							Errors += FString::Printf(TEXT("Could not add Actor %ls to group %ls (group list overflow)\n"), *FObjectName(pActor), *ModifiedGroup);
						}
					}
				}
			}
		}

		if (Errors.Len() > 0)
		{
			debugf(TEXT("The following error(s) occured when performing this action:\n\n%ls"), *Errors);
			appMsgf(TEXT("The following error(s) occured when performing this action:\n\n%ls"), *Errors);
			RefreshGroupsList();
		}
	}

	void ClearSelection()
	{
		if (GEditor && GEditor->Level)
		{
			for (int i = 0; i < GEditor->Level->Actors.Num(); i++)
			{
				AActor* pActor = GEditor->Level->Actors(i);
				if (pActor)
					pActor->bSelected = false;
			}
		}
		RefreshGroupsList();		
	}
	
	void RefreshGroupsList(void)
	{
		guard(WSelectGroups::RefreshActorList);

		if (!ActorList || !GroupList)
			return;

		SetRedraw(false);
		
		ActorList->Empty();
		GroupList->Empty();	

		// Group name => selected count
		TMap<FString, INT> Groups;
		INT SelectedActors = 0;
		
		if (GEditor	&& GEditor->Level)
		{
			for (int i = 0; i < GEditor->Level->Actors.Num(); i++)
			{
				AActor* pActor = GEditor->Level->Actors(i);
				if (pActor)
				{
					if (pActor->bSelected)
					{
						ActorList->AddString(*FObjectName(pActor));
						SelectedActors++;
					}

					if (pActor->Group != NAME_None)
					{
						for (TStringIterator It(*pActor->Group, ','); It.IsValid; It.Next())
						{
							FString& GroupName = It.Value;
							INT* SelectedCount = Groups.Find(GroupName);

							if (pActor->bSelected)
							{
								if (SelectedCount)
									*SelectedCount = *SelectedCount + 1;
								else
									Groups.Set(*GroupName, 1);
							}
							else
							{
								if (!SelectedCount)
									Groups.Set(*GroupName, 0);
							}
						}
					}
				}				
			}
		}

		ActorList->SetCurrent(0, 1);

		for (TMap<FString, INT>::TIterator It(Groups); It; ++It)
		{
			INT Index = GroupList->AddString(*It.Key());
			if (It.Value() == SelectedActors)
				GroupList->SetItemData(Index, 1);
			else if (It.Value() == 0)
				GroupList->SetItemData(Index, 0);
			else
				GroupList->SetItemData(Index, 2);
		}

		if (GBrowserGroup)
			GBrowserGroup->RefreshGroupList();

		SetRedraw(true);

		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
