
enum
{
	IDM_NewSubLevel=10,
	IDM_DeleteSubLevel,
	IDM_EditSubLevel,
};

class WSubLevelWindowBase : public WWindow
{
	DECLARE_WINDOWCLASS(WSubLevelWindowBase, WWindow, Window)

	WSubLevelWindowBase()
		: WWindow()
	{}
};

class WSubLevelNew : public WSubLevelWindowBase
{
public:
	WLabel Prompt,PromtGroup;
	WCoolButton OkButton;
	WCoolButton CancelButton;
	WEdit EditField,EditFieldGroup;
	WCheckBox AdditiveCheck;

	void OnClickTrue();
	void OnClickFalse()
	{
		_CloseWindow();
	}

	WSubLevelNew(HWND Parent)
		: WSubLevelWindowBase()
		, Prompt(this), PromtGroup(this)
		, OkButton(this, 0, FDelegate(this, (TDelegate)&WSubLevelNew::OnClickTrue))
		, CancelButton(this, 0, FDelegate(this, (TDelegate)&WSubLevelNew::OnClickFalse))
		, EditField(this), EditFieldGroup(this)
		, AdditiveCheck(this)
	{
		PerformCreateWindowEx(0, TEXT("Create new Sub-Level..."), WS_SYSMENU, 150, 150, 250, 176, Parent, NULL, NULL);

		FRect AreaRec = GetClientRect();
		INT XSize = AreaRec.Width();
		INT YSize = AreaRec.Height();

		Prompt.OpenWindow(1, 0);
		Prompt.SetText(TEXT("UI Name:"));
		
		PromtGroup.OpenWindow(1, 0);
		PromtGroup.SetText(TEXT("UI Group name:"));

		EditField.OpenWindow(1, 0, 0);
		EditField.SetText(TEXT("MyLevel_1"));

		EditFieldGroup.OpenWindow(1, 0, 0);
		EditFieldGroup.SetText(TEXT(""));

		Prompt.MoveWindow(10, 5, XSize - 20, 16, 1);
		EditField.MoveWindow(25, 27, XSize - 50, 20, 1);
		PromtGroup.MoveWindow(10, 49, XSize - 20, 16, 1);
		EditFieldGroup.MoveWindow(25, 71, XSize - 50, 20, 1);

		AdditiveCheck.OpenWindow(1, 10, 93, XSize - 20, 20, TEXT("Additive level"));

		OkButton.OpenWindow(1, 80, YSize - 24, 50, 20, TEXT("OK"));
		CancelButton.OpenWindow(1, 145, YSize - 24, 50, 20, TEXT("Cancel"));
	}
};

class WSubLevelWindow : public WSubLevelWindowBase
{
public:
	static WSubLevelWindow* GSubLevelEditor;
	WTreeView LevelTree;
	//HMENU MainWinMenu;
	ULevel* SelectedLevelRow;
	FString SelectedRowName;
	ALevelInfo* MainLevelInfo;

	WSubLevelWindow()
		: WSubLevelWindowBase()
		, LevelTree(this)
	{
		guard(WSubLevelWindow::WSubLevelWindow);

		GSubLevelEditor = this;

		RECT desktop; // Get the size of screen to the variable desktop
		::GetWindowRect(GetDesktopWindow(), &desktop);

		PerformCreateWindowEx(WS_EX_CLIENTEDGE, TEXT("Sub-Level Editor"), WS_OVERLAPPEDWINDOW, desktop.right * 0.65, desktop.bottom * 0.08, desktop.right * 0.3, desktop.bottom * 0.88, NULL, NULL, NULL);

		LevelTree.OpenWindow(1, 1, 0, 0, 1);
		LevelTree.SelChangedDelegate = FDelegate(this, (TDelegate)&WSubLevelWindow::OnTreeViewSelChanged);
		LevelTree.DblClkDelegate = FDelegate(this, (TDelegate)&WSubLevelWindow::OnTreeViewDblClick);

		/*HMENU hSubMenu;
		MainWinMenu = CreateMenu();

		// Main (menu)
		hSubMenu = CreatePopupMenu();
		AppendMenuW(hSubMenu, MF_STRING, ID_REFRESH_VIEW, TEXT("&Refresh selection"));
		AppendMenuW(hSubMenu, MF_STRING, ID_CLOSE_MENU, TEXT("&Close"));
		AppendMenuW(MainWinMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, TEXT("&Main"));

		hKismetObjectsMenu = CreatePopupMenu();
		AppendMenuW(hKismetObjectsMenu, MF_STRING, ID_NEWKISMET, TEXT("&New object"));
		AppendMenuW(hKismetObjectsMenu, MF_MENUBARBREAK, 0, NULL);
		AppendMenuW(MainWinMenu, MF_STRING | MF_POPUP, (UINT)hKismetObjectsMenu, TEXT("&Kismet objects"));

		hKismetObjectDel = CreatePopupMenu();
		AppendMenuW(MainWinMenu, MF_STRING | MF_POPUP, (UINT)hKismetObjectDel, TEXT("&Delete kismet objects"));

		hKismetObjectSettings = CreatePopupMenu();
		AppendMenuW(MainWinMenu, MF_STRING | MF_POPUP, (UINT)hKismetObjectSettings, TEXT("&Settings"));
		AppendMenuW(hKismetObjectSettings, MF_STRING | (KSettings.bDoubleBuffer ? MF_CHECKED : MF_UNCHECKED), ID_DBLBUFFER, TEXT("Render with &double buffer"));

		SetMenu(hWnd, MainWinMenu);

		Preferences.OpenChildWindow(0);
		Preferences.NotifyHook = &Hook;
		LeftArea.OpenWindow(hWnd);*/

		// Init positioning.
		OnSize(0, 0, 0);

		FullRefresh();
		unguard;
	}
	void DoDestroy()
	{
		if (GSubLevelEditor == this)
			GSubLevelEditor = NULL;
		WWindow::DoDestroy();
	}
	void OnClose()
	{
		if (GSubLevelEditor == this)
			GSubLevelEditor = NULL;
		WWindow::OnClose();
	}
	void OnSize(DWORD Flags, INT NewX, INT NewY)
	{
		FRect WinRec = GetClientRect();
		LevelTree.MoveWindow(0, 0, WinRec.Width(), WinRec.Height(), 1);
	}
	void OnTreeViewSelChanged()
	{
		guard(WBrowserActor::OnTreeViewSelChanged);
		NMTREEVIEW* pnmtv = (LPNMTREEVIEW)LevelTree.LastlParam;
		TCHAR chText[1024] = TEXT("\0");
		TVITEM tvi;

		appMemzero(&tvi, sizeof(tvi));
		tvi.hItem = pnmtv->itemNew.hItem;
		tvi.mask = TVIF_TEXT;
		tvi.pszText = chText;
		tvi.cchTextMax = sizeof(chText);

		if (SendMessageW(LevelTree.hWnd, TVM_GETITEM, 0, (LPARAM)&tvi))
		{
			SelectedRowName = tvi.pszText;
			if (SelectedRowName.Right(1) == TEXT("*"))
				SelectedRowName = SelectedRowName.Left(SelectedRowName.Len() - 1);
			const TCHAR* Str = *SelectedRowName;
			SelectedLevelRow = NULL;
			if(!appStricmp(Str,TEXT("MyLevel")))
				SelectedLevelRow = MainLevelInfo->XLevel;
			else
			{
				for (ALevelInfo* L = MainLevelInfo->ChildLevel; L; L = L->ChildLevel)
				{
					if (!appStricmp(Str, L->XLevel->GetName()))
					{
						SelectedLevelRow = L->XLevel;
						break;
					}
				}
			}
		}
		unguard;
	}
	void OnTreeViewDblClick()
	{
		guard(WBrowserActor::OnTreeViewDblClick);
		OnTreeViewSelChanged();
		if (SelectedLevelRow)
		{
			GEditor->Exec(*(FString::Printf(TEXT("MAP EDITLEVEL NAME=\"%ls\""), SelectedLevelRow->GetName())));
			FullRefresh();
		}
		unguard;
	}

	void FullRefresh()
	{
		SelectedLevelRow = NULL;
		ULevel* Main = GEditor->Level;
		ALevelInfo* ActiveLevel = Main->GetLevelInfo();
		ALevelInfo* Info = ActiveLevel;

		if (Info->ParentLevel && Info->ParentLevel != Info)
		{
			Info = Info->ParentLevel;
			Main = Info->XLevel;
		}

		MainLevelInfo = Info;
		LevelTree.Empty();
		if (!Info->ChildLevel)
		{
			LevelTree.AddItem(TEXT("MyLevel*"), NULL, FALSE);
			return;
		}
	
		HTREEITEM htiRoot = LevelTree.AddItem((ActiveLevel == MainLevelInfo ? TEXT("MyLevel*") : TEXT("MyLevel")), NULL, TRUE);
		
		ALevelInfo* L, * IL;
		TArray<FName> GroupList;
		INT j;
		for (L = MainLevelInfo->ChildLevel; L; L = L->ChildLevel)
		{
			if (L->Group != NAME_None)
				GroupList.AddUniqueItem(L->Group);
		}

		// First add all without groupname.
		for (L = MainLevelInfo->ChildLevel; L; L = L->ChildLevel)
		{
			if (L->Group == NAME_None)
			{
				FName N = L->XLevel->GetFName();
				j = GroupList.FindItemIndex(N);
				if (j >= 0)
				{
					GroupList.Remove(j);
					HTREEITEM inner = LevelTree.AddItem((ActiveLevel==L ? *(FString(*N)+TEXT("*")) : *N), htiRoot, TRUE);

					for (IL = MainLevelInfo->ChildLevel; IL; IL = IL->ChildLevel)
					{
						if (IL->Group == N)
						{
							LevelTree.AddItem((ActiveLevel == IL ? *(FString(IL->XLevel->GetName()) + TEXT("*")) : IL->XLevel->GetName()), inner, FALSE);
						}
					}
					SendMessageW(LevelTree.hWnd, TVM_EXPAND, TVE_EXPAND, (LPARAM)inner);
				}
				else LevelTree.AddItem((ActiveLevel == L ? *(FString(*N) + TEXT("*")) : *N), htiRoot, FALSE);
			}
		}

		// Then add all groups...
		for (j = 0; j < GroupList.Num(); ++j)
		{
			FName N = GroupList(j);
			HTREEITEM inner = LevelTree.AddItem(*N, htiRoot, TRUE);
			for (L = MainLevelInfo->ChildLevel; L; L = L->ChildLevel)
			{
				if (L->Group == N)
					LevelTree.AddItem((ActiveLevel == L ? *(FString(L->XLevel->GetName()) + TEXT("*")) : L->XLevel->GetName()), inner, FALSE);
			}
			SendMessageW(LevelTree.hWnd, TVM_EXPAND, TVE_EXPAND, (LPARAM)inner);
		}
		SendMessageW(LevelTree.hWnd, TVM_EXPAND, TVE_EXPAND, (LPARAM)htiRoot);
	}

	void OnCommand(INT Command)
	{
		guard(WBrowserActor::OnCommand);
		switch (Command)
		{
		case WM_TREEVIEW_RIGHT_CLICK:
			{
				// Select the tree item underneath the mouse cursor.
				TVHITTESTINFO tvhti;
				POINT ptScreen;
				::GetCursorPos(&ptScreen);
				tvhti.pt = ptScreen;
				::ScreenToClient(LevelTree.hWnd, &tvhti.pt);

				SendMessageW(LevelTree.hWnd, TVM_HITTEST, 0, (LPARAM)&tvhti);

				if (tvhti.hItem)
					SendMessageW(LevelTree.hWnd, TVM_SELECTITEM, TVGN_CARET, (LPARAM)(HTREEITEM)tvhti.hItem);

				// Show a context menu for the currently selected item.
				HMENU hRightClickMenu = CreatePopupMenu();
				if (SelectedLevelRow)
				{
					AppendMenuW(hRightClickMenu, MF_STRING, IDM_EditSubLevel, TEXT("Edit this level"));
					AppendMenuW(hRightClickMenu, MF_MENUBARBREAK, 0, NULL);
				}
				AppendMenuW(hRightClickMenu, MF_STRING, IDM_NewSubLevel, TEXT("Create new level"));
				if (SelectedLevelRow && SelectedLevelRow != MainLevelInfo->XLevel)
				{
					AppendMenuW(hRightClickMenu, MF_MENUBARBREAK, 0, NULL);
					AppendMenuW(hRightClickMenu, MF_STRING, IDM_DeleteSubLevel, TEXT("Delete this level"));
				}

				BOOL Result = TrackPopupMenuEx((HMENU)hRightClickMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_VERPOSANIMATION | TPM_RETURNCMD, ptScreen.x, ptScreen.y, hWnd, NULL);
				DeleteObject(hRightClickMenu);

				if (Result == IDM_EditSubLevel)
				{
					//appMsgf(TEXT("Edit %ls"), SelectedLevelRow->GetFullName());
					GEditor->Exec(*(FString::Printf(TEXT("MAP EDITLEVEL NAME=\"%ls\""), SelectedLevelRow->GetName())));
					FullRefresh();
				}
				else if (Result == IDM_NewSubLevel)
				{
					WSubLevelNew* NL = new WSubLevelNew(hWnd);
					NL->Show(1);
				}
				else if (Result == IDM_DeleteSubLevel)
				{
					if (GWarn->YesNof(TEXT("This will permanently delete sub-level %ls\nYou can't undo this operation, continue?"), SelectedLevelRow->GetFullName()))
					{
						appMsgf(TEXT("Deleted %ls"), SelectedLevelRow->GetFullName());

						// Deselect level if were deleting active level.
						if (GEditor->Level == SelectedLevelRow)
							GEditor->Exec(*(FString::Printf(TEXT("MAP EDITLEVEL NAME=\"%ls\""), MainLevelInfo->XLevel->GetName())));

						// Simply unlink the level we want removed.
						ALevelInfo* SelInfo = SelectedLevelRow->GetLevelInfo();
						if (MainLevelInfo->ChildLevel == SelInfo)
						{
							MainLevelInfo->ChildLevel = SelInfo->ChildLevel;
							if (!MainLevelInfo->ChildLevel)
								MainLevelInfo->ParentLevel = NULL;
						}
						else
						{
							for (ALevelInfo* L = MainLevelInfo; L; L = L->ChildLevel)
							{
								if (L->ChildLevel == SelInfo)
								{
									L->ChildLevel = SelInfo->ChildLevel;
									break;
								}
							}
						}
						SelInfo->ChildLevel = NULL;
						FullRefresh();
					}
				}
			}
			break;
		default:
			WWindow::OnCommand(Command);
			break;
		}
		unguard;
	}

	void CreateNewSubLevel(const TCHAR* Name, const TCHAR* GName, UBOOL bAdditive)
	{
		ULevel* NL = new (MainLevelInfo->XLevel->GetOuter(), Name) ULevel(GEditor, bAdditive);
		ALevelInfo* NewInfo = NL->GetLevelInfo();
		NewInfo->Group = GName;

		MainLevelInfo->ParentLevel = MainLevelInfo;
		NewInfo->ParentLevel = MainLevelInfo;
		for (ALevelInfo* L = MainLevelInfo; L; L = L->ChildLevel)
		{
			if (!L->ChildLevel)
			{
				L->ChildLevel = NewInfo;
				break;
			}
		}
		NewInfo->ChildLevel = NULL;
		FullRefresh();
	}

	static void ShowMenu()
	{
		if (!GSubLevelEditor)
			GSubLevelEditor = new WSubLevelWindow();
		GSubLevelEditor->Show(1);
	}
};

WSubLevelWindow* WSubLevelWindow::GSubLevelEditor = NULL;

void WSubLevelNew::OnClickTrue()
{
	FString Name = EditField.GetText();
	FString GName = EditFieldGroup.GetText();
	BOOL bAdditive = AdditiveCheck.IsChecked();

	_CloseWindow();
	WSubLevelWindow::GSubLevelEditor->CreateNewSubLevel(*Name, *GName, bAdditive);
}
