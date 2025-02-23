/*=============================================================================
	BrowserGroup : Browser window for actor groups
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>

extern void ParseStringToArray( const TCHAR* pchDelim, FString String, TArray<FString>* _pArray);

// --------------------------------------------------------------
//
// NEW/RENAME GROUP Dialog
//
// --------------------------------------------------------------

class WDlgGroup : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgGroup,WDialog,UnrealEd)

	// Variables.
	WButton OkButton;
	WButton CancelButton;
	WEdit NameEdit;

	FString defName, Name;
	UBOOL bNew;

	// Constructor.
	WDlgGroup( UObject* InContext, WBrowser* InOwnerWindow )
	:	WDialog			( TEXT("Group"), IDDIALOG_GROUP, InOwnerWindow )
	,	OkButton		( this, IDOK,			FDelegate(this,(TDelegate)&WDlgGroup::OnOk) )
		, CancelButton(this, IDCANCEL, FDelegate(this, (TDelegate)&WDlgGroup::EndDialogFalse))
	,	NameEdit		( this, IDEC_NAME )
		, bNew(FALSE)
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgGroup::OnInitDialog);
		WDialog::OnInitDialog();

		NameEdit.SetText( *defName );
		::SetFocus( NameEdit.hWnd );

		if( bNew )
			SetText(TEXT("New Group"));
		else
			SetText(TEXT("Rename Group"));

		NameEdit.SetSelection(0, -1);

		unguard;
	}
	virtual int DoModal( UBOOL InbNew, FString _defName )
	{
		guard(WDlgGroup::DoModal);

		bNew = InbNew;
		defName = _defName;

		return WDialog::DoModal( hInstance );
		unguard;
	}
	void OnOk()
	{
		guard(WDlgGroup::OnOk);
		Name = NameEdit.GetText();
		EndDialog(TRUE);
		unguard;
	}
};

// --------------------------------------------------------------
//
// WBrowserGroup
//
// --------------------------------------------------------------

#define ID_BG_TOOLBAR	29050
TBBUTTON tbBGButtons[] = {
	{ 0, IDMN_MB_DOCK, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 1, IDMN_GB_NEW_GROUP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 2, IDMN_GB_DELETE_GROUP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 3, IDMN_GB_ADD_TO_GROUP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 4, IDMN_GB_DELETE_FROM_GROUP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 5, IDMN_GB_REFRESH, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 6, IDMN_GB_SELECT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 7, IDMN_GB_DESELECT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
};
struct {
	TCHAR ToolTip[64];
	int ID;
} ToolTips_BG[] = {
	TEXT("Toggle Dock Status"), IDMN_MB_DOCK,
	TEXT("New Group"), IDMN_GB_NEW_GROUP,
	TEXT("Delete"), IDMN_GB_DELETE_GROUP,
	TEXT("Add Selected Actors to Group(s)"), IDMN_GB_ADD_TO_GROUP,
	TEXT("Delete Selected Actors from Group(s)"), IDMN_GB_DELETE_FROM_GROUP,
	TEXT("Refresh Group List"), IDMN_GB_REFRESH,
	TEXT("Select Actors in Group(s)"), IDMN_GB_SELECT,
	TEXT("Deselect Actors in Group(s)"), IDMN_GB_DESELECT,
	NULL, 0
};

struct FGroupActors
{
	TSingleMap<AActor*> Actors;
	INT VisFlags,Tag;
	FString GroupName;

	FGroupActors(const TCHAR* GN)
		: GroupName(GN), VisFlags(0), Tag(INDEX_NONE)
	{}
	FGroupActors(const FString& GN)
		: GroupName(GN), VisFlags(0), Tag(INDEX_NONE)
	{}
};
struct TStringIterator
{
	const TCHAR* OrgStr;
	TCHAR Separator;
	UBOOL IsValid;

	FString Value;

	TStringIterator(const TCHAR* InStr, TCHAR Sep)
		: OrgStr(InStr), Separator(Sep), IsValid(1)
	{
		Next();
	}
	inline void Next()
	{
		guardSlow(TStringIterator::Next);
		if (!*OrgStr)
			IsValid = 0;
		else
		{
			while (*OrgStr == ' ')
				++OrgStr;
			const TCHAR* StartStr = OrgStr;
			while (*OrgStr && *OrgStr != Separator)
				++OrgStr;

			Value = FString(StartStr, OrgStr);
			if (*OrgStr)
				++OrgStr;
		}
		unguardSlow;
	}
};

class WBrowserGroup : public WBrowser , public FActorGroupCallback
{
	DECLARE_WINDOWCLASS(WBrowserGroup,WBrowser,Window)

	WDblCheckListBox*pListGroups;
	HWND hWndToolBar;
	WToolTip *ToolTipCtrl;
	UBOOL bIsDirty;
	HBITMAP ToolbarImage{};

	TArray<FGroupActors> GroupList;
	INT _Tag;

	// Structors.
	WBrowserGroup( FName InPersistentName, WWindow* InOwnerWindow, HWND InEditorFrame )
	:	WBrowser( InPersistentName, InOwnerWindow, InEditorFrame ), bIsDirty(0), _Tag(0)
	{
		pListGroups = NULL;
		MenuID = IDMENU_BrowserGroup;
		BrowserID = eBROWSER_GROUP;
		Description = TEXT("Groups");
		GEdCallback = this;
	}

	// WBrowser interface.
	void OpenWindow( UBOOL bChild )
	{
		guard(WBrowserGroup::OpenWindow);
		WBrowser::OpenWindow( bChild );
		SetCaption();
		unguard;
	}
	virtual void UpdateMenu()
	{
		guard(WBrowserGroup::UpdateMenu);
		HMENU menu = IsDocked() ? GetMenu( OwnerWindow->hWnd ) : GetMenu( hWnd );
		CheckMenuItem( menu, IDMN_MB_DOCK, MF_BYCOMMAND | (IsDocked() ? MF_CHECKED : MF_UNCHECKED) );
		unguard;
	}
	void OnCreate()
	{
		guard(WBrowserGroup::OnCreate);
		WBrowser::OnCreate();

		SetMenu( hWnd, LoadMenuW(hInstance, MAKEINTRESOURCEW(IDMENU_BrowserGroup)) );
		
		// GROUP LIST
		//
		pListGroups = new WDblCheckListBox( this, IDLB_GROUPS );
		pListGroups->OpenWindow( 1, 0, 1, 1 );

		ToolbarImage = reinterpret_cast<HBITMAP>(LoadImage(hInstance,
					MAKEINTRESOURCE(IDB_BrowserGroup_TOOLBAR),
					IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS));
				check(ToolbarImage);
				ScaleImageAndReplace(ToolbarImage);

		hWndToolBar = CreateToolbarEx(
			hWnd, WS_CHILD | WS_BORDER | WS_VISIBLE | CCS_ADJUSTABLE,
			IDB_BrowserGroup_TOOLBAR,
			8,
			NULL,
			reinterpret_cast<PTRINT>(ToolbarImage),
			(LPCTBBUTTON)&tbBGButtons,
			11,
			MulDiv(16, DPIX, 96), MulDiv(16, DPIY, 96),
			MulDiv(16, DPIX, 96), MulDiv(16, DPIY, 96),
			sizeof(TBBUTTON));
		check(hWndToolBar);

		ToolTipCtrl = new WToolTip(this);
		ToolTipCtrl->OpenWindow();
		for( int tooltip = 0 ; ToolTips_BG[tooltip].ID > 0 ; tooltip++ )
		{
			// Figure out the rectangle for the toolbar button.
			int index = SendMessageW( hWndToolBar, TB_COMMANDTOINDEX, ToolTips_BG[tooltip].ID, 0 );
			RECT rect;
			SendMessageW( hWndToolBar, TB_GETITEMRECT, index, (LPARAM)&rect);

			ToolTipCtrl->AddTool( hWndToolBar, ToolTips_BG[tooltip].ToolTip, tooltip, &rect );
		}

		RefreshActorList();
		RefreshGroupList();
		PositionChildControls();

		unguard;
	}
	virtual void RefreshAll()
	{
		guard(WBrowserGroup::RefreshAll);
		GetFromVisibleGroups();
		RefreshGroupList();
		unguard;
	}
	void OnTimer()
	{
		if (bIsDirty)
			RefreshGroupList();
	}
	void RefreshDelayed()
	{
		if (!bIsDirty)
		{
			bIsDirty = 1;
			SetTimer(hWnd, 0, 10, NULL);
		}
	}
	void OnDestroy()
	{
		guard(WBrowserGroup::OnDestroy);

		delete pListGroups;

		::DestroyWindow( hWndToolBar );
		delete ToolTipCtrl;
		if (ToolbarImage)
			DeleteObject(ToolbarImage);

		WBrowser::OnDestroy();

		if (GEdCallback == this)
			GEdCallback = NULL;
		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WBrowserGroup::OnSize);
		WBrowser::OnSize(Flags, NewX, NewY);
		PositionChildControls();
		InvalidateRect( hWnd, NULL, FALSE );
		UpdateMenu();
		unguard;
	}
	// Updates the check status of all the groups, based on the contents of the VisibleGroups
	// variable in the LevelInfo.
	void GetFromVisibleGroups()
	{
		guard(WBrowserGroup::GetFromVisibleGroups);
		ALevelInfo* Level = GEditor->Level->GetLevelInfo();

		RefreshActorList();
		INT i;

		// Default all to hidden.
		for (i = (GroupList.Num() - 1); i >= 0; --i)
			GroupList(i).VisFlags = 1;

		for (TStringIterator It(*Level->VisibleGroups, ','); It.IsValid; It.Next())
		{
			for (i = (GroupList.Num() - 1); i >= 0; --i)
			{
				if (GroupList(i).GroupName == It.Value)
				{
					GroupList(i).VisFlags &= ~1;
					break;
				}
			}
		}
		for (TStringIterator It(*Level->LockedGroups, ','); It.IsValid; It.Next())
		{
			for (i = (GroupList.Num() - 1); i >= 0; --i)
			{
				if (GroupList(i).GroupName == It.Value)
				{
					GroupList(i).VisFlags |= 2;
					break;
				}
			}
		}
		unguard;
	}
	// Updates the VisibleGroups variable in the LevelInfo
	void SendToVisibleGroups()
	{
		guard(WBrowserGroup::SendToVisibleGroups);

		FString VisG, LockG;

		for (INT i = (GroupList.Num() - 1); i >= 0; --i)
		{
			//debugf(TEXT("Groups.Num %i %ls"),Groups.Num(),Groups(x));
			FGroupActors& G = GroupList(i);

			if (!(G.VisFlags & 1))
			{
				if (VisG.Len())
					VisG += FString(TEXT(",")) + G.GroupName;
				else VisG = G.GroupName;
			}
			if ((G.VisFlags & 2))
			{
				if (LockG.Len())
					LockG += FString(TEXT(",")) + G.GroupName;
				else LockG = G.GroupName;
			}
		}
		ALevelInfo* Level = GEditor->Level->GetLevelInfo();
		Level->VisibleGroups = VisG;
		Level->LockedGroups = LockG;

		GEditor->NoteSelectionChange( GEditor->Level );
		unguard;
	}
	inline UBOOL ActorImmune(AActor* Other)
	{
		return (Other->GetClass() == ACamera::StaticClass() || Other == GEditor->Level->Brush() || Other->GetClass()->GetDefaultActor()->bHiddenEd);
	}
	inline void UpdateActorVis(AActor* Other, FGroupActors* Group = NULL)
	{
		UBOOL bHidden = 0;
		UBOOL bLocked = 0;
		if (Other->Group != NAME_None)
		{
			if (Group && !appStrfind(*Other->Group, TEXT(",")))
			{
				if (Group->VisFlags != INDEX_NONE)
				{
					if (Group->VisFlags & 1)
						bHidden = 1;
					else if (Group->VisFlags & 2)
						bLocked = 1;
				}
			}
			else
			{
				for (INT i = (GroupList.Num() - 1); i >= 0; --i)
				{
					if (GroupList(i).VisFlags != INDEX_NONE && GroupList(i).Actors.Find(Other))
					{
						if (GroupList(i).VisFlags & 1)
							bHidden = 1;
						else if (GroupList(i).VisFlags & 2)
							bLocked = 1;
					}
				}
			}
		}
		if (UBOOL(Other->bHiddenEd) != bHidden || UBOOL(Other->bEdLocked) != bLocked)
		{
			Other->Modify();
			Other->bHiddenEd = bHidden;
			Other->bEdLocked = bLocked;
			if (bHidden || bLocked)
				Other->bSelected = 0;
		}
	}
	void NotifyLevelActor(AActor* Other, UEngine::EActorAction Action)
	{
		if (ActorImmune(Other))
			return;

		INT i;
		if (Action == UEngine::EActorAction::NOTE_ActorDestroyed)
		{
			UBOOL bDirty = FALSE;
			for (i = (GroupList.Num()-1); i >= 0; --i)
			{
				if (GroupList(i).Actors.Remove(Other) && GroupList(i).Actors.Num() == 0)
				{
					bDirty = TRUE;
					GroupList.Remove(i);
				}
			}
			if(bDirty)
				RefreshDelayed();
		}
		else
		{
			UBOOL bDirty = FALSE;
			if (Other->Group == NAME_None)
			{
				for (i = (GroupList.Num() - 1); i >= 0; --i)
				{
					if (GroupList(i).Actors.Remove(Other))
					{
						if (GroupList(i).Actors.Num() == 0)
							GroupList.Remove(i);
						bDirty = TRUE;
						break;
					}
				}
			}
			else
			{
				++_Tag;
				FGroupActors* G;
				for (TStringIterator It(*Other->Group, ','); It.IsValid; It.Next())
				{
					G = FindByGroupName(*It.Value);
					if (!G)
						G = new(GroupList) FGroupActors(It.Value);
					if(G->Actors.Set(Other))
						bDirty = TRUE;
					G->Tag = _Tag;
				}
				for (i = (GroupList.Num() - 1); i >= 0; --i)
				{
					if (GroupList(i).Tag != _Tag && GroupList(i).Actors.Remove(Other))
					{
						if(GroupList(i).Actors.Num() == 0)
							GroupList.Remove(i);
						bDirty = TRUE;
					}
				}
			}
			if (bDirty)
			{
				UpdateActorVis(Other);
				RefreshDelayed();
			}
		}
	}
	void RefreshActorList()
	{
		guard(WBrowserGroup::RefreshActorList);
		INT i;
		FGroupActors* G;

		// Reset list.
		for (i = (GroupList.Num() - 1); i >= 0; --i)
			GroupList(i).Actors.Empty();

		// Loop through all the actors in the world and put together a list of unique group names.
		// Actors can belong to multiple groups by seperating the group names with commas ("group1,group2")
		for (i = 0; i < GEditor->Level->Actors.Num(); i++)
		{
			AActor* pActor = GEditor->Level->Actors(i);
			if (pActor && !ActorImmune(pActor))
			{
				G = NULL;
				if (pActor->Group != NAME_None)
				{
					for (TStringIterator It(*pActor->Group, ','); It.IsValid; It.Next())
					{
						G = FindByGroupName(*It.Value);
						if (!G)
							G = new(GroupList) FGroupActors(It.Value);
						G->Actors.Set(pActor);
					}
				}
				UpdateActorVis(pActor, G);
			}
		}

		// Remove empty groups.
		for (i = (GroupList.Num() - 1); i >= 0; --i)
			if (!GroupList(i).Actors.Num())
				GroupList.Remove(i);
		unguard;
	}
	void RefreshGroupList()
	{
		guard(WBrowserGroup::RefreshGroupList);

		// Add the list of unique group names to the group listbox
		pListGroups->Empty();

		//debugf(TEXT("RefreshGroupList Groups.Num %i"), GroupList.Num());
		for (INT i = (GroupList.Num() - 1); i >= 0; --i)
		{
			//debugf(TEXT("Groups %i %ls"), i, *GroupList(i).GroupName);
			FGroupActors& G = GroupList(i);
			pListGroups->AddString(*G.GroupName);
			pListGroups->SetItemData(pListGroups->FindStringExact(*G.GroupName), G.VisFlags);
		}
		SendToVisibleGroups();

		pListGroups->SetCurrent( 0, 1 );
		bIsDirty = 0;
		unguard;
	}
	// Moves the child windows around so that they best match the window size.
	//
	void PositionChildControls( void )
	{
		guard(WBrowserGroup::PositionChildControls);

		if( !pListGroups ) return;

		FRect CR;
		CR = GetClientRect();
		RECT R;
		::GetClientRect( hWndToolBar, &R );

		::MoveWindow( pListGroups->hWnd, 4, R.bottom + 4, CR.Width() - 8, CR.Height() - 4 - R.bottom, 1 );

		unguard;
	}
	// Loops through all actors in the world and updates their visibility based on which groups are selected.
	void UpdateVisibility()
	{
		guard(WBrowserGroup::UpdateVisibility);
		UBOOL bDirty = 0;
		INT NewFlags;
		for (INT i = (GroupList.Num() - 1); i >= 0; --i)
		{
			FGroupActors& G = GroupList(i);
			int Index = pListGroups->FindStringExact(*G.GroupName);
			if (Index != LB_ERR)
			{
				NewFlags = (int)pListGroups->GetItemData(Index);
				if (NewFlags != G.VisFlags)
				{
					G.VisFlags = NewFlags;
					bDirty = 1;
					for (TSingleMap<AActor*>::TIterator It(G.Actors); It; ++It)
						UpdateActorVis(It.Key(), &G);
				}
			}
		}
		if (bDirty)
		{
			SendToVisibleGroups();
			PostMessageW(GhwndEditorFrame, WM_COMMAND, WM_REDRAWALLVIEWPORTS, 0);
		}
		unguard;
	}
	void OnCommand( INT Command )
	{
		guard(WBrowserGroup::OnCommand);
		switch( Command ) {

			case IDMN_GB_NEW_GROUP:
				NewGroup();
				break;

			case IDMN_GB_DELETE_GROUP:
				DeleteGroup();
				break;

			case IDMN_GB_ADD_TO_GROUP:
				OnAddToGroup();
				break;

			case IDMN_GB_DELETE_FROM_GROUP:
				DeleteFromGroup();
				break;

			case IDMN_GB_RENAME_GROUP:
				RenameGroup();
				break;

			case IDMN_GB_REFRESH:
				OnRefreshGroups();
				break;

			case IDMN_GB_SELECT:
				SelectActorsInGroups(1);
				break;

			case IDMN_GB_DESELECT:
				SelectActorsInGroups(0);
				break;

			case WM_WCLB_UPDATE_VISIBILITY:
				UpdateVisibility();
				break;

			default:
				WBrowser::OnCommand(Command);
				break;
		}
		unguard;
	}
	void SelectActorsInGroups( UBOOL Select )
	{
		guard(WBrowserGroup::SelectActorsInGroups);

		int SelCount = pListGroups->GetSelectedCount();
		if( SelCount == LB_ERR )	return;
		int* Buffer = new int[SelCount];
		pListGroups->GetSelectedItems(SelCount, Buffer);

		INT s;
		for (s = 0; s < SelCount; s++)
		{
			FGroupActors* G = FindByGroupName(*pListGroups->GetString(Buffer[s]));
			if (G)
			{
				for (TSingleMap<AActor*>::TIterator It(G->Actors); It; ++It)
					It.Key()->bSelected = Select;
			}
		}
		delete [] Buffer;
		PostMessageW( GhwndEditorFrame, WM_COMMAND, WM_REDRAWALLVIEWPORTS, 0 );

		unguard;
	}
	void NewGroup()
	{
		guard(WBrowserGroup::NewGroup);

		if( !HasActorsSelected() )
		{
			appMsgf(TEXT("You must have some actors selected to create a new group."));
			return;
		}

		// Generate a suggested group name to use as a default.
		int x = 1;
		FString DefaultName;
		while(1)
		{
			DefaultName = *(FString::Printf(TEXT("Group%d"), x) );
			if (!FindByGroupName(*DefaultName))
				break;
			x++;
		}

		WDlgGroup dlg( NULL, this );
		if( dlg.DoModal( 1, DefaultName ) )
		{
			FGroupActors* G = FindByGroupName(*dlg.Name);
			if (G)
				AddToGroup(*G);
			else
			{
				G = new (GroupList) FGroupActors(dlg.Name);
				AddToGroup(*G);
				RefreshGroupList();
			}
		}

		unguard;
	}
	void DeleteGroup()
	{
		guard(WBrowserGroup::DeleteGroup);
		int SelCount = pListGroups->GetSelectedCount();
		if( SelCount == LB_ERR )	return;

		INT s, i;
		int* Buffer = new int[SelCount];
		pListGroups->GetSelectedItems(SelCount, Buffer);
		for (s = 0; s < SelCount; s++)
		{
			FString DeletedGroup = pListGroups->GetString(Buffer[s]);
			for (i = (GroupList.Num() - 1); i >= 0; --i)
			{
				FGroupActors& G = GroupList(i);
				if (G.GroupName == DeletedGroup)
				{
					G.VisFlags = INDEX_NONE; // Pending delete.
					for (TSingleMap<AActor*>::TIterator It(G.Actors); It; ++It)
					{
						AActor* A = It.Key();
						if (A->Group != NAME_None)
						{
							FString NewGroups;
							for (TStringIterator SIt(*A->Group, ','); SIt.IsValid; SIt.Next())
							{
								if (SIt.Value != G.GroupName)
								{
									if (NewGroups.Len())
										NewGroups = NewGroups + TEXT(",") + SIt.Value;
									else NewGroups = SIt.Value;
								}
							}
							A->Modify();
							if (NewGroups.Len())
								A->Group = *NewGroups;
							else A->Group = NAME_None;
						}
						UpdateActorVis(A);
					}
					GroupList.Remove(i);
					break;
				}
			}
		}
		delete [] Buffer;

		GEditor->NoteSelectionChange( GEditor->Level );
		RefreshGroupList();
		unguard;
	}
	void AddToGroup(FGroupActors& G)
	{
		guard(WBrowserGroup::AddToGroup);
		UBOOL bFound;
		for( int i = 0 ; i < GEditor->Level->Actors.Num() ; i++ )
		{
			AActor* pActor = GEditor->Level->Actors(i);
			if (pActor && !ActorImmune(pActor) && pActor->bSelected)
			{
				// Make sure this actor is not already in this group.  If so, don't add it again.
				bFound = FALSE;
				if (pActor->Group != NAME_None)
				{
					for (TStringIterator It(*pActor->Group, ','); It.IsValid; It.Next())
					{
						if (It.Value == G.GroupName)
						{
							bFound = TRUE;
							break;
						}
					}
				}
				if (!bFound)
				{
					// Add the group to the actors group list
					FString NewGroupName = pActor->Group == NAME_None ? G.GroupName : (FString(*pActor->Group) + TEXT(",") + G.GroupName);
					pActor->Modify();
					pActor->Group = *NewGroupName;
				}
				G.Actors.Set(pActor);
				UpdateActorVis(pActor, &G);
			}
		}

		GEditor->NoteSelectionChange( GEditor->Level );
		unguard;
	}
	void DeleteFromGroup()
	{
		guard(WBrowserGroup::DeleteFromGroup);
		int SelCount = pListGroups->GetSelectedCount();
		if( SelCount == LB_ERR )	return;

		INT s, i;
		int* Buffer = new int[SelCount];
		pListGroups->GetSelectedItems(SelCount, Buffer);
		for (s = 0; s < SelCount; s++)
		{
			FString DeletedGroup = pListGroups->GetString(Buffer[s]);

			for (i = (GroupList.Num() - 1); i >= 0; --i)
			{
				FGroupActors& G = GroupList(i);
				if (G.GroupName == DeletedGroup)
				{
					for (TSingleMap<AActor*>::TIterator It(G.Actors); It; ++It)
					{
						AActor* A = It.Key();
						if (A->Group != NAME_None && A->bSelected)
						{
							It.RemoveCurrent();
							FString NewGroups;
							for (TStringIterator SIt(*A->Group, ','); SIt.IsValid; SIt.Next())
							{
								if (SIt.Value != G.GroupName)
								{
									if (NewGroups.Len())
										NewGroups = NewGroups + TEXT(",") + SIt.Value;
									else NewGroups = SIt.Value;
								}
							}
							A->Modify();
							if (NewGroups.Len())
								A->Group = *NewGroups;
							else A->Group = NAME_None;
							UpdateActorVis(A);
						}
					}
					break;
				}
			}
		}

		GEditor->NoteSelectionChange( GEditor->Level );
		RefreshGroupList();
		unguard;
	}
	void RenameGroup()
	{
		guard(WBrowserGroup::RenameGroup);

		WDlgGroup dlg( NULL, this );
		int SelCount = pListGroups->GetSelectedCount();
		if( SelCount == LB_ERR )	return;
		int* Buffer = new int[SelCount];
		pListGroups->GetSelectedItems(SelCount, Buffer);
		for( int s = 0 ; s < SelCount ; s++ )
		{
			FString Src = pListGroups->GetString(Buffer[s]);
			if( dlg.DoModal( 0, Src ) )
				SwapGroupNames( Src, dlg.Name );
		}
		delete [] Buffer;

		RefreshGroupList();
		GEditor->NoteSelectionChange( GEditor->Level );

		unguard;
	}
	void SwapGroupNames( FString Src, FString Dst )
	{
		guard(WBrowserGroup::SwapGroupNames);

		if (Src == Dst || !Dst.Len()) return;
		
		INT i;
		FGroupActors* SrcG = FindByGroupName(*Src), * DestG = FindByGroupName(*Dst);

		if (!SrcG || SrcG == DestG)
			return;
		if (!DestG)
		{
			DestG = new (GroupList) FGroupActors(Dst);

			// Refind src group as pointer may have changed.
			SrcG = FindByGroupName(*Src);
		}

		SrcG->VisFlags = INDEX_NONE;
		for (TSingleMap<AActor*>::TIterator It(SrcG->Actors); It; ++It)
		{
			AActor* A = It.Key();
			if (A->Group != NAME_None)
			{
				FString NewGroups;
				for (TStringIterator SIt(*A->Group, ','); SIt.IsValid; SIt.Next())
				{
					if (SIt.Value != Src && SIt.Value != Dst)
					{
						if (NewGroups.Len())
							NewGroups = NewGroups + TEXT(",") + SIt.Value;
						else NewGroups = SIt.Value;
					}
				}
				if (NewGroups.Len())
					NewGroups = NewGroups + TEXT(",") + Dst;
				else NewGroups = Dst;

				A->Modify();
				A->Group = *NewGroups;

				DestG->Actors.Set(A);
				UpdateActorVis(A, DestG);
			}
		}

		// Delete group.
		i = FindByGroupNameIndex(*Src);
		if (i != INDEX_NONE)
			GroupList.Remove(i);
		unguard;
	}
	
	inline UBOOL HasActorsSelected()
	{
		guard(WBrowserGroup::HasActorsSelected);
		for (int i = 0; i < GEditor->Level->Actors.Num(); i++)
		{
			AActor* pActor = GEditor->Level->Actors(i);
			if (pActor && !ActorImmune(pActor) && pActor->bSelected)
				return TRUE;
		}
		return FALSE;
		unguard;
	}
	inline FGroupActors* FindByGroupName(const TCHAR* Name)
	{
		guardSlow(FindByGroupName);
		for (INT i = (GroupList.Num() - 1); i >= 0; --i)
		{
			if (GroupList(i).GroupName == Name)
				return &GroupList(i);
		}
		return NULL;
		unguardSlow;
	}
	inline INT FindByGroupNameIndex(const TCHAR* Name)
	{
		for (INT i = (GroupList.Num() - 1); i >= 0; --i)
		{
			if (GroupList(i).GroupName == Name)
				return i;
		}
		return INDEX_NONE;
	}

	// Notification delegates for child controls.
	//
	void OnNewGroup()
	{
		guard(WBrowserGroup::OnNewGroupClick);
		NewGroup();
		unguard;
	}
	void OnDeleteGroup()
	{
		guard(WBrowserGroup::OnDeleteGroupClick);
		DeleteGroup();
		unguard;
	}
	void OnAddToGroup()
	{
		guard(WBrowserGroup::OnAddToGroupClick);
		int SelCount = pListGroups->GetSelectedCount();
		if( SelCount == LB_ERR )	return;
		int* Buffer = new int[SelCount];
		pListGroups->GetSelectedItems(SelCount, Buffer);
		for (int s = 0; s < SelCount; s++)
		{
			FString GN = pListGroups->GetString(Buffer[s]);
			for (INT i = (GroupList.Num() - 1); i >= 0; --i)
			{
				FGroupActors& G = GroupList(i);
				if (G.GroupName == GN)
				{
					AddToGroup(G);
					break;
				}
			}
		}
		delete [] Buffer;
		unguard;
	}
	void OnDeleteFromGroup()
	{
		guard(WBrowserGroup::OnDeleteFromGroupClick);
		DeleteFromGroup();
		unguard;
	}
	void OnRefreshGroups()
	{
		guard(WBrowserGroup::OnRefreshGroupsClick);
		RefreshActorList();
		RefreshGroupList();
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
