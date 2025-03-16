#/*=============================================================================
	CustomizeContextMenus : Hide specific items in context menus
	Copyright 2024, OldUnreal. All Rights Reserved.

	Revision history:
		* Created by Buggie
=============================================================================*/

class WCustomizeContextMenus : public WWindow
{
	DECLARE_WINDOWCLASS(WCustomizeContextMenus, WWindow, UnrealEd)

	enum { MENU_ID_OFFSET = 100000000 };
	enum { MENU_COUNT = 3 };

	// Variables.
	WCheckListBox* MenuItemList{};
	TArray<INT> MenuItemIds;
	WButton* EnableAllButton{};

	// Constructor.
	WCustomizeContextMenus(FName InPersistentName, WWindow* InOwnerWindow)
		: WWindow(InPersistentName, InOwnerWindow)
	{
	}

	// WWindow Interface
	void OpenWindow()
	{
		guard(WCustomizeContextMenus::OpenWindow);
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
		SetText(TEXT("Configure Context Menus"));
		unguard;
	}

	void OnClose()
	{
		guard(WCustomizeContextMenus::OnClose);
		Show(0);
		unguard;
	}

	void OnDestroy()
	{
		guard(WCustomizeContextMenus::OnDestroy);
		delete MenuItemList;
		delete EnableAllButton;
		WWindow::OnDestroy();
		unguard;
	}
	
	void OnCreate()
	{
		guard(WCustomizeContextMenus::OnCreate);
		WWindow::OnCreate();

		MenuItemList = new WCheckListBox(this, IDLB_MENU_ITEMS);
		MenuItemList->ItemModifiedDelegate = FDelegateInt(this, (TDelegateInt)&WCustomizeContextMenus::UpdateMenuItem);
		MenuItemList->OpenWindow(1, 0, 0);

		EnableAllButton = new WButton(this, IDBM_ENABLE_ALL, FDelegate(this, (TDelegate)&WCustomizeContextMenus::EnableAll));
		EnableAllButton->OpenWindow(1, 0, 0, 0, 0, TEXT("Enable All"));
		
		unguard;
	}
	
	void OnSize(DWORD Flags, INT NewX, INT NewY)
	{
		guard(WCustomizeContextMenus::OnSize);
		WWindow::OnSize(Flags, NewX, NewY);
		PositionChildControls();
		InvalidateRect(hWnd, NULL, FALSE);
		unguard;
	}
	
	void OnShowWindow(UBOOL bShow) 
	{
		guard(WCustomizeContextMenus::OnShowWindow);
		WWindow::OnShowWindow(bShow);
		if (bShow)
			RefreshMenuItemList();
		unguard;
	}

	void PositionChildControls()
	{
		guard(WCustomizeContextMenus::PositionChildControls);
		if (!MenuItemList || !EnableAllButton)
			return;

		FRect CR = GetClientRect();

		::MoveWindow(MenuItemList->hWnd,
			MulDiv(9, DPIX, 96),
			MulDiv(9, DPIY, 96),
			CR.Width() - MulDiv(18, DPIX, 96),
			CR.Height() - MulDiv(44, DPIY, 96),
			1);

		::MoveWindow(EnableAllButton->hWnd,
			CR.Width() / 2 - MulDiv(50, DPIX, 96),
			CR.Height() - MulDiv(27, DPIY, 96),
			MulDiv(100, DPIX, 96),
			MulDiv(19, DPIY, 96),
			1);

		::InvalidateRect(hWnd, NULL, 1);
		unguard;
	}

	void OnPaint()
	{
		guard(WCustomizeContextMenus::OnPaint);
		PAINTSTRUCT PS;
		HDC hDC = BeginPaint(*this, &PS);
		FillRect(hDC, GetClientRect(), (HBRUSH)(COLOR_BTNFACE + 1));
		EndPaint(*this, &PS);
		unguard;
	}

	void UpdateMenuItem(INT Item)
	{
		guard(WCustomizeContextMenus::UpdateMenuItem);
		if (!MenuItemList || Item < 0 || Item > MenuItemIds.Num())
			return;

		INT ActionId = MenuItemIds(Item) % MENU_ID_OFFSET;
		if (ActionId == 0)
		{
			MenuItemList->SetItemData(Item, BST_CHECKED);
			InvalidateRect(hWnd, NULL, FALSE);
			return;
		}
		SaveConfig();
		unguard;
	}

	void SaveConfig()
	{
		guard(WCustomizeContextMenus::SaveConfig);
		FString MenuConfigs[MENU_COUNT];

		for (INT i = 0; i < MenuItemIds.Num(); i++)
		{
			INT ActionId = MenuItemIds(i) % MENU_ID_OFFSET;
			if (ActionId == 0 || MenuItemList->GetItemData(i) != BST_UNCHECKED)
				continue;
			INT MenuId = MenuItemIds(i) / MENU_ID_OFFSET;
			MenuConfigs[MenuId] += FString::Printf(TEXT(" %d"), ActionId);
		}
		
		for (INT i = 0; i < MENU_COUNT; i++)
			GConfig->SetString(TEXT("Options"), GetConfigName(i), *MenuConfigs[i], GUEDIni);
		unguard;
	}

	static const TCHAR* GetConfigName(INT MenuId)
	{
		guard(WCustomizeContextMenus::GetConfigName);
		constexpr const TCHAR* Names[] = {TEXT("HideActorContexMenuItems"), TEXT("HideSurfaceContexMenuItems"), TEXT("HideBackdropContexMenuItems")};
		return Names[MenuId];
		unguard;
	}

	static UBOOL ClearMenu(HMENU Menu)
	{
		guard(WCustomizeContextMenus::ClearMenu);
		TCHAR Buffer[1024] = {};
		FString StrBuff;
		UBOOL Good = FALSE;
		UBOOL PrevIsSeparator = TRUE;
		for (int i = 0; i < GetMenuItemCount(Menu); i++)
		{
			GetMenuStringW(Menu, i, (LPWSTR)&Buffer, 1000, MF_BYPOSITION);
			INT MenuItemId = GetMenuItemID(Menu, i);
			if (MenuItemId == 0) // Separator
			{
				if (PrevIsSeparator)
				{
					DeleteMenu(Menu, i, MF_BYPOSITION);
					i--;
					continue;
				}
			}
			else if (MenuItemId == INDEX_NONE) // Submenu
			{
				HMENU SubMenu = GetSubMenu(Menu, i);
				if (SubMenu != NULL && !ClearMenu(SubMenu))
				{
					DeleteMenu(Menu, i, MF_BYPOSITION);
					i--;
					continue;
				}
				Good = TRUE;
			}
			else
				Good = TRUE;
			PrevIsSeparator = MenuItemId == 0;
		}
		// Remove trailing separators.
		for (int i = GetMenuItemCount(Menu) - 1; i >= 0; i--)
			if (GetMenuItemID(Menu, i) == 0)
				DeleteMenu(Menu, i, MF_BYPOSITION);
			else
				break;
		return Good;
		unguard;
	}

	static HMENU AddCustomContextMenu(HMENU Menu)
	{
		guard(WCustomizeContextMenus::AddCustomContextMenu);
		MENUITEMINFO Info;
		TCHAR Buffer[1024];
		appMemzero(&Info, sizeof(Info));
		Info.cbSize     = sizeof(Info);
		Info.fMask      = MIIM_TYPE | MIIM_ID;
		Info.cch        = ARRAY_COUNT(Buffer);
		Info.dwTypeData = Buffer;
		if (!GetMenuItemInfoW(Menu, ID_BackdropPopupAddCustom1Here, FALSE, &Info))
			return Menu;
		INT Pos = INDEX_NONE;
		for (int i = GetMenuItemCount(Menu) - 1; i >= 0; i--)
			if (GetMenuItemID(Menu, i) == ID_BackdropPopupAddCustom1Here)
			{
				Pos = i;
				break;
			}
		if (Pos == INDEX_NONE)
			return Menu;
		Pos++;
		for (INT i = ARRAY_COUNT(CustomContextMenu) - 1; i > 0; i--)
		{
			Info.wID = ID_BackdropPopupAddCustom1Here + i;
			InsertMenuItemW(Menu, Pos, TRUE, &Info);
		}
		return Menu;
		unguard;
	}

	static HMENU ReduceMenu(INT MenuId, HMENU Menu)
	{
		guard(WCustomizeContextMenus::ReduceMenu);
		if (MenuId >= 0 && MenuId < MENU_COUNT)
		{
			FString HiddenItems;
			GConfig->GetString(TEXT("Options"), GetConfigName(MenuId), HiddenItems, GUEDIni);
			const TCHAR* Str = *HiddenItems;
			FString Token;
			while ((Token = ParseToken(Str, FALSE)).Len() > 0)
			{
				INT ActionId = Token.Int();
				if (ActionId > 0)
					DeleteMenu(Menu, ActionId, MF_BYCOMMAND);
			}
		}
		ClearMenu(Menu);
		return Menu;
		unguard;
	}

	void EnableAll()
	{
		guard(WCustomizeContextMenus::EnableAll);
		for (INT i = 0; i < MENU_COUNT; i++)
			GConfig->SetString(TEXT("Options"), GetConfigName(i), TEXT(""), GUEDIni);
		RefreshMenuItemList();
		unguard;
	}
	
	void AddMenu(INT MenuId, HMENU Menu, FString Indent)
	{
		guard(WCustomizeContextMenus::AddMenu);
		TCHAR Buffer[1024] = {};
		FString StrBuff;
		for (int i = 0, n = GetMenuItemCount(Menu); i < n; i++)
		{
			GetMenuStringW(Menu, i, (LPWSTR)&Buffer, 1000, MF_BYPOSITION);
			INT MenuItemId = GetMenuItemID(Menu, i);
			if (MenuItemId != 0 || Buffer[0])
			{
				StrBuff = Buffer;
				MenuItemList->AddString(*(Indent + StrBuff.Replace(TEXT("&"), TEXT(""))));
				MenuItemIds.AddItem(MenuId + Max(0, MenuItemId));
			}
			if (MenuItemId == INDEX_NONE)
			{
				HMENU SubMenu = GetSubMenu(Menu, i);
				if (SubMenu != NULL)
					AddMenu(MenuId, SubMenu, Indent + TEXT("    "));
			}
		}
		unguard;
	}
	void RefreshMenuItemList()
	{
		guard(WCustomizeContextMenus::RefreshMenuItemList);

		if (!MenuItemList)
			return;

		SetRedraw(false);

		if (MenuItemList->GetCount() == 0)
		{
			MenuItemList->Empty();
			MenuItemIds.Empty();

			INT MenuIds[] = {IDMENU_ActorPopup, IDMENU_SurfPopup, IDMENU_BackdropPopup};
			const TCHAR* MenuNames[] = {TEXT("Actor Context Menu"), TEXT("Surface Context Menu"), TEXT("Backdrop Context Menu")};

			for (INT i = 0; i < MENU_COUNT; i++)
			{
				INT MenuId = i*MENU_ID_OFFSET;
				MenuItemList->AddString(*FString::Printf(TEXT("---------------------------------------- %ls ----------------------------------------"), MenuNames[i]));
				MenuItemIds.AddItem(MenuId);

				HMENU OuterMenu = LoadMenuW(hInstance, MAKEINTRESOURCEW(MenuIds[i]));
				HMENU Menu = GetSubMenu(OuterMenu, 0);
				AddMenu(MenuId, AddCustomContextMenu(Menu), TEXT(""));
				DestroyMenu(OuterMenu);
			}
		}

		for (INT i = 0; i < MenuItemIds.Num(); i++)
			MenuItemList->SetItemData(i, BST_CHECKED);

		FString HiddenItems;
		FString Token;
		for (INT i = 0; i < MENU_COUNT; i++)
		{
			HiddenItems.EmptyNoRealloc();
			GConfig->GetString(TEXT("Options"), GetConfigName(i), HiddenItems, GUEDIni);
			const TCHAR* Str = *HiddenItems;
			while ((Token = ParseToken(Str, FALSE)).Len() > 0)
			{
				INT ActionId = Token.Int();
				if (ActionId > 0)
				{
					ActionId += i*MENU_ID_OFFSET;
					INT Index = MenuItemIds.FindItemIndex(ActionId);
					if (Index != INDEX_NONE)
						MenuItemList->SetItemData(Index, BST_UNCHECKED);
				}
			}
		}

		SetRedraw(true);
		::InvalidateRect(MenuItemList->hWnd, NULL, 1);
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
