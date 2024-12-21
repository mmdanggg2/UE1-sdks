/*=============================================================================
	BrowserMesh : Browser window for meshes
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>

EDITOR_API extern INT GLastScroll;

void MENU_AnimContextMenu(HWND Wnd, FName AnimName, UMesh* MeshType);

extern void Query( ULevel* Level, const TCHAR* Item, FString* pOutput, UPackage* InPkg );
extern void ParseStringToArray( const TCHAR* pchDelim, FString String, TArray<FString>* _pArray);

// --------------------------------------------------------------
//
// WBrowserMesh
//
// --------------------------------------------------------------

#define ID_MESH_TOOLBAR	29050
TBBUTTON tbMESHButtons[] = {
	{ 0, IDMN_MB_DOCK, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 1, IDPB_MB_FileOpen, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 2, IDPB_MB_FileSave, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 3, IDPB_MB_IN_LOAD_ENTIRE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 4, IDPB_PLAY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 5, IDPB_STOP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 6, IDPB_PREV_FRAME, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 7, IDPB_NEXT_FRAME, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 8, IDPB_EDIT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
#if ENGINE_VERSION==227
	, { 9, IDPB_ADD_MESH, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
#endif
	, { 10, IDPB_SHOWSKEL, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 11, IDPB_SHOWSKELNAM, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
};
struct {
	TCHAR ToolTip[64];
	int ID;
} ToolTips_MESH[] = {
	TEXT("Toggle Dock Status"), IDMN_MB_DOCK,
	TEXT("Open Package"), IDPB_MB_FileOpen,
	TEXT("Save Package"), IDPB_MB_FileSave,
	TEXT("Load Entire Package"), IDPB_MB_IN_LOAD_ENTIRE,
	TEXT("Play Animation"), IDPB_PLAY,
	TEXT("Stop Animation"), IDPB_STOP,
	TEXT("Next Frame in Animation"), IDPB_NEXT_FRAME,
	TEXT("Previous Frame in Animation"), IDPB_PREV_FRAME,
	TEXT("Edit Mesh Properties"), IDPB_EDIT,
#if ENGINE_VERSION==227
	TEXT("Add Mesh into Map"), IDPB_ADD_MESH,
#endif
	TEXT("Show/Hide Skeletal mesh skeleton"), IDPB_SHOWSKEL,
	TEXT("Show/Hide Skeletal mesh bone names"), IDPB_SHOWSKELNAM,
	NULL, 0
};

#if ENGINE_VERSION!=227
const TCHAR* GUEdIni = TEXT("UnrealEd.ini");
#endif

class WBrowserMesh : public WBrowser, public WViewportWindowContainer
{
	DECLARE_WINDOWCLASS(WBrowserMesh,WBrowser,Window)

	WComboBox* pMeshCombo;
	WComboBox* pMeshPackCombo;
	WComboBox* pMeshComboGroup;
	WListBox* pAnimList;
	WCheckBox* pMeshCheck;
	WCheckBox* pSkeletalMeshCheck;
	TArray<FName> AnimList;
#if ENGINE_VERSION==227
	WCheckBox* pStaticMeshCheck;
#endif
	HWND hWndToolBar{};
	WToolTip *ToolTipCtrl{};
	MRUList* mrulist;
	HBITMAP ToolbarImage{};

	UBOOL bPlaying;

	int NumAnims, NumFrames;

	// Structors.
	WBrowserMesh( FName InPersistentName, WWindow* InOwnerWindow, HWND InEditorFrame )
	:	WBrowser( InPersistentName, InOwnerWindow, InEditorFrame )
	,	WViewportWindowContainer(TEXT("MeshBrowser"), *InPersistentName)
	{
		pMeshCombo = NULL;
		pMeshPackCombo = NULL;
		pMeshComboGroup = NULL;
		pAnimList = NULL;
		pMeshCheck = NULL;
#if ENGINE_VERSION==227
		pStaticMeshCheck = NULL;
#endif
		pMeshCheck = NULL;
		bPlaying = FALSE;
		MenuID = IDMENU_BrowserMesh;
		BrowserID = eBROWSER_MESH;
		Description = TEXT("Meshes");
		mrulist = NULL;
	}

	UBOOL IsValidMesh(UMesh* M)
	{
		UBOOL ValidMesh = 0;

		if (pMeshCheck->IsChecked() && (M->GetClass() == UMesh::StaticClass() || M->GetClass() == ULodMesh::StaticClass()))
			ValidMesh = 1;
		return (ValidMesh);
	}
	UBOOL IsValidSkeletalMesh(UMesh* M)
	{
		UBOOL ValidSkeletalMesh = 0;

		if (pSkeletalMeshCheck->IsChecked() && M->GetClass() == USkeletalMesh::StaticClass())
			ValidSkeletalMesh = 1;
		return (ValidSkeletalMesh);
	}
#if ENGINE_VERSION==227
	UBOOL IsValidStaticMesh(UMesh* M)
	{
		UBOOL ValidStaticMesh = 0;

		if (pStaticMeshCheck->IsChecked() && M->GetClass() == UStaticMesh::StaticClass())
			ValidStaticMesh = 1;
		return (ValidStaticMesh);
	}
#endif

	// WBrowser interface.
	void OpenWindow( UBOOL bChild )
	{
		guard(WBrowserMesh::OpenWindow);
		WBrowser::OpenWindow( bChild );
		SetCaption();
		Show(1);
		unguard;
	}
	void OnCreate()
	{
		guard(WBrowserMesh::OnCreate);
		WBrowser::OnCreate();

		SetRedraw(false);

		ViewportOwnerWindow = hWnd;

		pMeshCheck = new WCheckBox(this, IDCK_MESH);
		pMeshCheck->ClickDelegate = FDelegate(this, (TDelegate)&WBrowserMesh::OnObjectsClick);
		pMeshCheck->OpenWindow(1, 0, 0, 1, 1, TEXT("Show VertexMeshes"));
		SendMessage(pMeshCheck->hWnd, BM_SETCHECK, BST_CHECKED, 0);

		pSkeletalMeshCheck = new WCheckBox(this, IDCK_SKELETALMESH);
		pSkeletalMeshCheck->ClickDelegate = FDelegate(this, (TDelegate)&WBrowserMesh::OnObjectsClick);
		pSkeletalMeshCheck->OpenWindow(1, 0, 0, 1, 1, TEXT("Show SkeletalMeshes"));
		SendMessage(pSkeletalMeshCheck->hWnd, BM_SETCHECK, BST_CHECKED, 0);

#if ENGINE_VERSION==227
		pStaticMeshCheck = new WCheckBox(this, IDCK_STATICMESH);
		pStaticMeshCheck->ClickDelegate = FDelegate(this, (TDelegate)&WBrowserMesh::OnObjectsClick);
		pStaticMeshCheck->OpenWindow(1, 0, 0, 1, 1, TEXT("Show StaticMeshes"));
		SendMessage(pStaticMeshCheck->hWnd, BM_SETCHECK, BST_CHECKED, 0);
#endif

		pMeshCombo = new WComboBox( this, IDCB_MESH );
		pMeshCombo->OpenWindow( 1, 1 );

		pMeshPackCombo = new WComboBox(this, IDCB_MESHPACK);
		pMeshPackCombo->OpenWindow(1, 1);

		pAnimList = new WListBox( this, IDLB_ANIMATIONS );
		pAnimList->OpenWindow( 1, 0, 0, 0, 0, LBS_MULTICOLUMN | WS_HSCROLL );
		SendMessageW( pAnimList->hWnd, LB_SETCOLUMNWIDTH, 96, 0 );
		pMeshCombo->SelectionChangeDelegate = FDelegate(this,(TDelegate)&WBrowserMesh::OnMeshSelectionChange);
		pMeshPackCombo->SelectionChangeDelegate = FDelegate(this, (TDelegate)&WBrowserMesh::OnMPackageSelectionChange);
		pAnimList->DoubleClickDelegate = FDelegate(this,(TDelegate)&WBrowserMesh::OnAnimDoubleClick);
		pAnimList->SelectionChangeDelegate = FDelegate(this,(TDelegate)&WBrowserMesh::OnAnimSelectionChange);
		pAnimList->RightClickDelegate = FDelegate(this, (TDelegate)&WBrowserMesh::OnAnimRightClick);

		ToolbarImage = reinterpret_cast<HBITMAP>(LoadImage(hInstance,
			MAKEINTRESOURCE(IDB_BrowserMesh_TOOLBAR),
			IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS));
		check(ToolbarImage);
		ScaleImageAndReplace(ToolbarImage);

		hWndToolBar = CreateToolbarEx( 
			hWnd, WS_CHILD | WS_VISIBLE | CCS_ADJUSTABLE,
			IDB_BrowserMesh_TOOLBAR,
			6,
			NULL,
			reinterpret_cast<PTRINT>(ToolbarImage),
			(LPCTBBUTTON)&tbMESHButtons,
#if ENGINE_VERSION==227
			17,
#else
			15,
#endif
			MulDiv(16, DPIX, 96), MulDiv(16, DPIY, 96),
			MulDiv(16, DPIX, 96), MulDiv(16, DPIY, 96),
			sizeof(TBBUTTON));
		check(hWndToolBar);

		ToolTipCtrl = new WToolTip(this);
		ToolTipCtrl->OpenWindow();
		for( int tooltip = 0 ; ToolTips_MESH[tooltip].ID > 0 ; tooltip++ )
		{
			// Figure out the rectangle for the toolbar button.
			int index = SendMessageW( hWndToolBar, TB_COMMANDTOINDEX, ToolTips_MESH[tooltip].ID, 0 );
			RECT rect;
			SendMessageW( hWndToolBar, TB_GETITEMRECT, index, (LPARAM)&rect);

			ToolTipCtrl->AddTool( hWndToolBar, ToolTips_MESH[tooltip].ToolTip, tooltip, &rect );
		}

		mrulist = new MRUList(*PersistentName);
		mrulist->ReadINI();
		if (GBrowserMaster->GetCurrent() == BrowserID)
			mrulist->AddToMenu(hWnd, GetMenu(IsDocked() ? OwnerWindow->hWnd : hWnd));
		
		RefreshAll();
		SetCaption();

		PositionChildControls();

		SetRedraw(true);
		unguard;
	}
	void SetCaption( void )
	{
		guard(WBrowserMesh::SetCaption);

		FString Caption = TEXT("Mesh Browser");
		FString MeshName = GetCurrentMeshName();

		if (MeshName.Len())
		{
			Caption += TEXT(" - ");
			Caption += MeshName;
		}

		SetText( *Caption );
		unguard;
	}
	virtual void RefreshAll()
	{
		guard(WBrowserMesh::RefreshAll);
		RefreshMeshList();
		RefreshAnimList();
		RefreshViewport();
		if (GBrowserMaster->GetCurrent() == BrowserID)
			mrulist->AddToMenu(hWnd, GetMenu(IsDocked() ? OwnerWindow->hWnd : hWnd));
		unguard;
	}
	void RefreshMeshList()
	{
		guard(WBrowserMesh::RefreshMeshList);

		SetRedraw(false);

		FString OldEntry;
		if (pMeshPackCombo->GetCount())
			OldEntry = pMeshPackCombo->GetString(pMeshPackCombo->GetCurrent());
		pMeshPackCombo->Empty();

		TArray<UObject*> PList;
		for (TObjectIterator<UMesh> It; It; ++It)
		{
			UMesh* M = *It;
			if (!IsValidMesh(M) && !IsValidSkeletalMesh(M)
#if ENGINE_VERSION==227
				&& !IsValidStaticMesh(M)
#endif
				)
				continue;
			PList.AddUniqueItem(M->TopOuter());
		}
		for (INT i = 0; i < PList.Num(); i++)
			pMeshPackCombo->AddString(PList(i)->GetName());
		INT UseEntry = INDEX_NONE;
		if (OldEntry.Len())
			UseEntry = pMeshPackCombo->FindString(*OldEntry);
		pMeshPackCombo->SetCurrent((UseEntry > 0) ? UseEntry : 0);
		ReloadMeshes(true);

		SetRedraw(true);

		unguard;
	}
	void ReloadMeshes(bool bFindOld = false)
	{
		guard(WBrowserMesh::ReloadMeshes);

		FString OldEntry;
		if (bFindOld && pMeshCombo->GetCount())
			OldEntry = pMeshCombo->GetString(pMeshCombo->GetCurrent());

		FString PName = pMeshPackCombo->GetString(pMeshPackCombo->GetCurrent());
		pMeshCombo->Empty();

		for (TObjectIterator<UMesh> It; It; ++It)
		{
			UMesh* M = *It;
			if (!IsValidMesh(M) || PName != M->TopOuter()->GetName())
				continue;
			pMeshCombo->AddString(M->GetName());
		}
		for (TObjectIterator<UMesh> It; It; ++It)
		{
			UMesh* M = *It;
			if (!IsValidSkeletalMesh(M) || PName != M->TopOuter()->GetName())
				continue;
			pMeshCombo->AddString(M->GetName());
		}
#if ENGINE_VERSION==227
		for (TObjectIterator<UMesh> It; It; ++It)
		{
			UMesh* M = *It;
			if (!IsValidStaticMesh(M) || PName != M->TopOuter()->GetName())
				continue;
			pMeshCombo->AddString(M->GetName());
		}
#endif
		if (bFindOld && OldEntry.Len())
		{
			INT UseEntry = pMeshCombo->FindString(*OldEntry);
			pMeshCombo->SetCurrent((UseEntry > 0) ? UseEntry : 0);
		}
		else pMeshCombo->SetCurrent(0);
		unguard;
	}
	FString GetCurrentMeshName()
	{
		guard(WBrowserMesh::GetCurrentMeshName);
		return pMeshCombo->GetString( pMeshCombo->GetCurrent() );
		unguard;
	}
	UMesh* GetMesh()
	{
		guard(WBrowserMesh::GetMesh);
		FString MeshName = pMeshPackCombo->GetString(pMeshPackCombo->GetCurrent()) + TEXT(".") + GetCurrentMeshName();
		return FindObject<UMesh>(NULL, *MeshName);
		unguard;
	}
	void RefreshAnimList()
	{
		guard(WBrowserMesh::RefreshAnimList);
		INT iFirstAnim = 0;

		pAnimList->Empty();
		AnimList.Empty();

		UMesh* Mesh = GetMesh();

		if (Mesh && !Mesh->IsA(UStaticMesh::StaticClass()))
		{
			if (Mesh->IsA(USkeletalMesh::StaticClass()))
			{
				UAnimation* Anim = ((USkeletalMesh*)Mesh)->DefaultAnimation;
				pAnimList->AddString(TEXT("Default"));
				AnimList.AddItem(NAME_None);

				if (Anim)
				{
					iFirstAnim = 1;
					for (INT i = 0; i < Anim->AnimSeqs.Num(); ++i)
					{
						const FMeshAnimSeq& Seq = Anim->AnimSeqs(i);
						pAnimList->AddString(*(FString::Printf(TEXT("%ls [ %d ]"), *Seq.Name, Seq.NumFrames)));
						AnimList.AddItem(Seq.Name);
					}
				}
			}
			else
			{
				for (INT i = 0; i < Mesh->AnimSeqs.Num(); ++i)
				{
					const FMeshAnimSeq& Seq = Mesh->AnimSeqs(i);
					pAnimList->AddString(*(FString::Printf(TEXT("%ls [ %d ]"), *Seq.Name, Seq.NumFrames)));
					AnimList.AddItem(Seq.Name);
				}
			}
		}

		pAnimList->SetCurrent(iFirstAnim, TRUE);
		pAnimList->AutoResizeColumns();
		unguard;
	}
	void RefreshViewport()
	{
		guard(WBrowserMesh::RefreshViewport);

		if( pMeshCombo->GetCurrent()==INDEX_NONE )
			return;

		FString MeshName = pMeshPackCombo->GetString(pMeshPackCombo->GetCurrent()) + TEXT(".") + pMeshCombo->GetString(pMeshCombo->GetCurrent());
		FName AnimSeq = NAME_None;
		if (AnimList.IsValidIndex(pAnimList->GetCurrent()))
			AnimSeq = AnimList(pAnimList->GetCurrent());

		if( !pViewport )
		{
			// Create the mesh viewport
			debugf(TEXT("Create the mesh viewport"));
			FString Device;
			GConfig->GetString(*ConfigName, TEXT("Device"), Device, GUEdIni);
			CreateViewport(SHOW_StandardView | SHOW_NoButtons | SHOW_ChildWindow, REN_MeshView, 320, 200, -1, -1, *Device);
			check(pViewport);
			pViewport->MiscRes = FindObject<UMesh>(ANY_PACKAGE, *MeshName);
			//pViewport = GEditor->Client->NewViewport( TEXT("MeshViewer") );
			pViewport->Group = NAME_None;
			pViewport->Actor->Misc1 = 0;
			pViewport->Actor->Misc2 = 0;
			pViewport->Actor->AnimSequence = AnimSeq;
		}
		else
		{
			GEditor->Exec( *(FString::Printf(TEXT("CAMERA UPDATE NAME=MeshBrowser MESH=\"%ls\" FLAGS=%d REN=%d ANIM=\"%ls\" MISC2=%d"),
				*MeshName,
				bPlaying
				? SHOW_StandardView | SHOW_NoButtons | SHOW_ChildWindow | SHOW_Backdrop | SHOW_RealTime
				: SHOW_StandardView | SHOW_NoButtons | SHOW_ChildWindow,
				REN_MeshView,
				*AnimSeq,
				pViewport->Actor->Misc2
				)));
		}
		UEditorEngine::CurrentMesh = Cast<UMesh>(pViewport->MiscRes);

		PositionChildControls();
		pViewport->RepaintPending = TRUE;
		unguard;
	}
	void OnDestroy()
	{
		guard(WBrowserMesh::OnDestroy);

		GEditor->Exec(TEXT("CAMERA CLOSE NAME=MeshBrowser"));
		delete pViewport;

		delete pMeshCombo;
		delete pMeshPackCombo;
		delete pAnimList;
		delete pMeshCheck;
		delete pSkeletalMeshCheck;
#if ENGINE_VERSION==227
		delete pStaticMeshCheck;
#endif

		::DestroyWindow(hWndToolBar);
		delete ToolTipCtrl;

		if (ToolbarImage)
			DeleteObject(ToolbarImage);

		mrulist->WriteINI();
		delete mrulist;

		WBrowser::OnDestroy();
		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WBrowserMesh::OnSize);
		WBrowser::OnSize(Flags, NewX, NewY);
		PositionChildControls();
		InvalidateRect( hWnd, NULL, FALSE );
		UpdateMenu();
		unguard;
	}
	void PositionChildControls()
	{
		guard(WBrowserMesh::PositionChildControls);
		if (!pMeshPackCombo
			|| !::IsWindow(pMeshPackCombo->hWnd)
			|| !pMeshCombo
			|| !::IsWindow(pMeshCombo->hWnd)
			|| !pAnimList || !::IsWindow(pAnimList->hWnd)
			|| !pViewport
			)
			return;

		SetRedraw(false);

		FRect CR;
		CR = GetClientRect();

		::MoveWindow(hWndToolBar, 4, 0, 4 * MulDiv(16, DPIX, 96), 4 * MulDiv(16, DPIY, 96), 1);
		RECT R;
		::GetClientRect(hWndToolBar, &R);		
		float Top = CR.Min.Y + R.bottom + 12;

		::MoveWindow(pMeshCheck->hWnd, 4, Top, MulDiv(140, DPIX, 96), MulDiv(20, DPIY, 96), 1);
		::MoveWindow(pSkeletalMeshCheck->hWnd, MulDiv(144, DPIX, 96), Top, MulDiv(140, DPIX, 96), MulDiv(20, DPIY, 96), 1);
#if ENGINE_VERSION==227
		::MoveWindow(pStaticMeshCheck->hWnd, MulDiv(284, DPIX, 96), Top, MulDiv(140, DPIX, 96), MulDiv(20, DPIY, 96), 1);
#endif
		Top += MulDiv(25, DPIY, 96);
		::MoveWindow(pMeshPackCombo->hWnd, 4, Top, CR.Width() - 8, MulDiv(20, DPIY, 96), 1);
		Top += MulDiv(20, DPIY, 96);
		::MoveWindow(pMeshCombo->hWnd, 4, Top, CR.Width() - 8, MulDiv(20, DPIY, 96), 1);
		Top += MulDiv(34, DPIY, 96);
		::MoveWindow(pAnimList->hWnd, 4, Top, CR.Width() - 8, MulDiv(96, DPIY, 96), 1);
		Top += MulDiv(96, DPIY, 96);
		::MoveWindow((HWND)pViewport->GetWindow(), 4, Top, CR.Width() - 8, CR.Height() - Top, 1);
		pViewport->RepaintPending = TRUE;

		// Refresh the display.
		SetRedraw(true);

		unguard;
	}
	void OnPlay()
	{
		guard(WBrowserMesh::OnPlay);
		bPlaying = 1;
		RefreshViewport();
		unguard;
	}
	void OnStop()
	{
		guard(WBrowserMesh::OnStop);
		bPlaying = 0;
		RefreshViewport();
		unguard;
	}
	void OnShowNext()
	{
		guard(WBrowserMesh::OnShowNext);
		if (bPlaying)
			OnStop();
		pViewport->Actor->Misc2 += 1;
		RefreshViewport();
		unguard;
	}
	void OnShowPrev()
	{
		guard(WBrowserMesh::OnShowPrev);
		if (bPlaying)
			OnStop();
		pViewport->Actor->Misc2 -= 1;
		RefreshViewport();
		unguard;
	}
	void OnShowSkele()
	{
		pViewport->ShowSkel = !pViewport->ShowSkel;
		RefreshViewport();
	}
	void OnShowSkelNames()
	{
		pViewport->ShowNames = !pViewport->ShowNames;
		RefreshViewport();
	}
	virtual void UpdateMenu()
	{
		guard(WBrowserMesh::UpdateMenu);

		HMENU menu = IsDocked() ? GetMenu(OwnerWindow->hWnd) : GetMenu(hWnd);
		CheckMenuItem(menu, IDMN_MB_DOCK, MF_BYCOMMAND | (IsDocked() ? MF_CHECKED : MF_UNCHECKED));
		UpdateRendererMenu(menu, TRUE);

		if (mrulist && GBrowserMaster->GetCurrent() == BrowserID)
			mrulist->AddToMenu(hWnd, GetMenu(IsDocked() ? OwnerWindow->hWnd : hWnd));

		unguard;
	}
	void OnCommand( INT Command )
	{
		guard(WBrowserMesh::OnCommand);
		TCHAR l_chCmd[512];
		switch( Command ) {

			case IDPB_PLAY:
				OnPlay();
				break;

			case IDPB_STOP:
				OnStop();
				break;
			case IDPB_EDIT:
				GEditor->Exec( *(FString::Printf(TEXT("HOOK MESHPROPERTIES MESH=\"%ls\""),*GetCurrentMeshName())) );
				break;
			case IDPB_SHOWSKEL:
				OnShowSkele();
				break;
			case IDPB_SHOWSKELNAM:
				OnShowSkelNames();
				break;
			case IDPB_NEXT_FRAME:
				{
				OnShowNext();
				}
				break;
			case IDPB_PREV_FRAME:
				{
				OnShowPrev();
				}
				break;
			case IDPB_ADD_MESH:
				{
				appSprintf( l_chCmd, TEXT("ACTOR ADD MESH NAME=%ls SNAP=1"), *GetCurrentMeshName() );
				GEditor->Exec( l_chCmd );
				}
				break;
			case IDPB_MB_IN_LOAD_ENTIRE:
				{
					FString Package = pMeshPackCombo->GetString( pMeshPackCombo->GetCurrent());
					UObject::LoadPackage(NULL,*Package,LOAD_NoWarn);
					GBrowserMaster->RefreshAll();
				}
				break;
			case IDPB_IMPORT:
				{
					OPENFILENAMEA ofn;
					char File[8192] = "\0";

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(char) * 8192;
					ofn.lpstrFilter = "Import Types (*.obj)\0*.obj;\0All Files\0*.*\0\0";
					ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_USM]) );
					ofn.lpstrDefExt = "obj";
					ofn.lpstrTitle = "Import Static Mesh (Wavefront .obj)";
					ofn.Flags = OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

					// Display the Open dialog box.
					if( GetOpenFileNameA(&ofn) )
					{
						int iNULLs = FormatFilenames( File );
						FString Package = pMeshPackCombo->GetString( pMeshPackCombo->GetCurrent() );
						FString Group = TEXT("None");// pMeshComboGroup->GetString( pMeshComboGroup->GetCurrent() );

						TArray<FString> StringArray;
						ParseStringToArray( TEXT("|"), appFromAnsi( File ), &StringArray );

						int iStart = 0;
						FString Prefix = TEXT("\0");

						if( iNULLs )
						{
							iStart = 1;
							Prefix = *(StringArray(0));
							Prefix += TEXT("\\");
						}

						if( StringArray.Num() == 1 )
							GLastDir[eLASTDIR_USM] = StringArray(0).GetFilePath().LeftChop(1);
						else
							GLastDir[eLASTDIR_USM] = StringArray(0);

						TArray<FString> FilenamesArray;

						for( int x = iStart ; x < StringArray.Num() ; x++ )
						{
							FString NewString;

							NewString = FString::Printf( TEXT("%ls%ls"), *Prefix, *(StringArray(x)) );
							new(FilenamesArray)FString( NewString );

							FString S = NewString;
						}

						WDlgStaticMeshImport l_dlg( NULL, this );
						l_dlg.DoModal( Package, Group, &FilenamesArray );

						GBrowserMaster->RefreshAll();

						pMeshPackCombo->SetCurrent( pMeshPackCombo->FindStringExact( *l_dlg.Package) );
						//pMeshComboGroup->SetCurrent( pMeshComboGroup->FindStringExact( *l_dlg.Group) );
						GBrowserMaster->RefreshAll();
					}
					GFileManager->SetDefaultDirectory(appBaseDir());
				}
				RefreshAll();
			break;
			case IDPB_EXPORT:
				{
				OPENFILENAMEA ofn;
					char File[8192] = "\0";
					FString MeshName = pViewport->Actor->GetName();
					FString Name = pViewport->Actor->Mesh->GetName();
					::sprintf_s( File,256, "%ls", *Name );

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(char) * 8192;
					ofn.lpstrFilter = "OBJ Files (*.obj)\0*.obj;\0All Files\0*.*\0\0";
					ofn.lpstrDefExt = "obj";
					ofn.lpstrTitle = "Export to Wavefront (.obj)";
					ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;

					// Display the Open dialog box.
					//
					if( GetSaveFileNameA(&ofn) )
					{
						appSprintf( l_chCmd, TEXT("OBJ EXPORTOBJ FILE=\"%ls\" ACTOR=%ls"), appFromAnsi( File ),*MeshName );
						GEditor->Exec( l_chCmd );
					}

					GFileManager->SetDefaultDirectory(appBaseDir());
				}
			break;
			case IDPB_EXPORT_ALL:
			{
				OPENFILENAMEA ofn;
				char File[8192] = "\0";
				FString MeshName = pViewport->Actor->GetName();
				FString Name = pViewport->Actor->Mesh->GetName();
				::sprintf_s(File, 256, "%ls", *Name);

				ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
				ofn.lStructSize = sizeof(OPENFILENAMEA);
				ofn.hwndOwner = hWnd;
				ofn.lpstrFile = File;
				ofn.nMaxFile = sizeof(char) * 8192;
				ofn.lpstrFilter = "OBJ Files (*.obj)\0*.obj;\0All Files\0*.*\0\0";
				ofn.lpstrDefExt = "obj";
				ofn.lpstrTitle = "Export all animation frames to Wavefront (.obj)";
				ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;

				// Display the Open dialog box.
				//
				if (GetSaveFileNameA(&ofn))
				{
					appSprintf(l_chCmd, TEXT("OBJ EXPORTOBJ FILE=\"%ls\" ACTOR=%ls EXPORTALL"), appFromAnsi(File), *MeshName);
					GEditor->Exec(l_chCmd);
				}

				GFileManager->SetDefaultDirectory(appBaseDir());
			}
			break;
			case IDPM_EXPORT:
				{
					if( pViewport->Actor->Mesh->IsA(UStaticMesh::StaticClass()) || pViewport->Actor->Mesh->IsA(USkeletalMesh::StaticClass()))
					{
						appMsgf(TEXT("Can't export %ls as .3d file!"),pViewport->Actor->Mesh->GetFullName());
						break;
					}

					OPENFILENAMEA ofn;
					char File[8192] = "\0";
					FString MeshName = pViewport->Actor->GetName();
					FString Name = pViewport->Actor->Mesh->GetName();
					::sprintf_s( File,256, "%ls", *Name );

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(char) * 8192;
					ofn.lpstrFilter = "3d Files (*.3d)\0*.3d;\0All Files\0*.*\0\0";
					ofn.lpstrDefExt = "3d";
					ofn.lpstrTitle = "Export Unreal Mesh (.3d)";
					ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;

					// Display the Open dialog box.
					//
					if( GetSaveFileNameA(&ofn) )
					{
						appSprintf( l_chCmd, TEXT("MESH EXPORT MESH=%ls FILENAME=\"%ls\""), *Name, appFromAnsi( File ) );
						GEditor->Exec( l_chCmd );
					}

					GFileManager->SetDefaultDirectory(appBaseDir());
				}
				break;
				case IDPM_EXPORT_ALL:
				{
					FString CurrentPackage = pMeshPackCombo->GetString(pMeshPackCombo->GetCurrent());
					TCHAR TempStr[256];

					// Make package directory.
					appStrcpy( TempStr, TEXT("..") PATH_SEPARATOR );
					appStrcat( TempStr, *CurrentPackage );
					GFileManager->MakeDirectory( TempStr, 0 );

					// Make package\Models directory.
					appStrcat( TempStr, PATH_SEPARATOR TEXT("Models") );
					GFileManager->MakeDirectory( TempStr, 0 );

					GFileManager->SetDefaultDirectory(TempStr);

					for( TObjectIterator<UMesh> It; It; ++It )
					{
						UMesh* M = *It;
						UClass* MeshClass=M->GetClass();
						if( (MeshClass == UMesh::StaticClass() || MeshClass == ULodMesh::StaticClass()) &&
							(MeshClass != UStaticMesh::StaticClass() && MeshClass!= USkeletalMesh::StaticClass()) &&
							(CurrentPackage == M->GetOuter()->GetName()))
						{
							//debugf(TEXT("Exporting: %ls"), M->GetFullName());
							//OPENFILENAMEA ofn;
							char File[8192] = "\0";
							FString Name = M->GetName();
							::sprintf_s( File,256, "%ls", *Name );
							debugf(TEXT("Exporting: %ls"),appFromAnsi(File));
							appSprintf( l_chCmd, TEXT("MESH EXPORT MESH=%ls FILENAME=\"%ls\""), *Name, appFromAnsi( File ) );
							GEditor->Exec( l_chCmd );
						}
						else if (CurrentPackage == M->GetOuter()->GetName())
							debugf(TEXT("Skipping %ls"), M->GetFullName());
					}
				GFileManager->SetDefaultDirectory(appBaseDir());
				}
				break;
				case IDPSKM_EXPORT:
				{
					if( !pViewport->Actor->Mesh->IsA(USkeletalMesh::StaticClass()))
					{
						appMsgf(TEXT("Can't export %ls as .psk file!"),pViewport->Actor->Mesh->GetFullName());
						break;
					}

					OPENFILENAMEA ofn;
					char File[8192] = "\0";
					FString MeshName = pViewport->Actor->GetName();
					FString Name = pViewport->Actor->Mesh->GetName();
					::sprintf_s( File,256, "%ls", *Name );

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(char) * 8192;
					ofn.lpstrFilter = "PSK Files (*.psk)\0*.psk;\0All Files\0*.*\0\0";
					ofn.lpstrDefExt = "psk";
					ofn.lpstrTitle = "Export Unreal SkeletalMesh (.psk)";
					ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;

					// Display the Open dialog box.
					//
					if( GetSaveFileNameA(&ofn) )
					{
						if( UExporter::ExportToFile(pViewport->Actor->Mesh, NULL, ANSI_TO_TCHAR(File), 0, 0) )
								GLog->Logf( TEXT("Exported %ls to %ls"), pViewport->Actor->Mesh->GetFullName(), ANSI_TO_TCHAR(File) );
						else
							GLog->Logf(TEXT("Can't export %ls to file %ls"),pViewport->Actor->Mesh->GetFullName(),ANSI_TO_TCHAR(File));
					}

					GFileManager->SetDefaultDirectory(appBaseDir());
				}
				break;
				case IDPSKM_EXPORT_ALL:
				{
					FString CurrentPackage = pMeshPackCombo->GetString(pMeshPackCombo->GetCurrent());
					FString Path=TEXT("..");
					Path+=PATH_SEPARATOR;
					Path+=CurrentPackage;

					// Make package directory.
					GFileManager->MakeDirectory( *Path, 0 );
					Path+=PATH_SEPARATOR;
					Path+=TEXT("Models");
					// Make package\Models directory.
					GFileManager->MakeDirectory( *Path, 0 );

					for( TObjectIterator<UObject> It; It; ++It )
					{
						if( It->IsA(USkeletalMesh::StaticClass()) && CurrentPackage == It->GetOuter()->GetName() )
						{
							FString PackageWithDot = CurrentPackage;
							PackageWithDot = PackageWithDot.Caps() + TEXT(".");
							FString WithGroup = It->GetPathName();
							if( WithGroup.Left(PackageWithDot.Len()).Caps() == PackageWithDot )
								WithGroup = WithGroup.Mid(PackageWithDot.Len());
							else
								WithGroup = It->GetName();

							FString Filename = Path * WithGroup + TEXT(".") + TEXT("psk");
							if( UExporter::ExportToFile(*It, NULL, *Filename, 0, 0) )
								GLog->Logf( TEXT("Exported %ls to %ls"), It->GetFullName(), *Filename );
							else
								GLog->Logf(TEXT("Can't export %ls to file %ls"),It->GetFullName(),*Filename);
						}
					}
					GFileManager->SetDefaultDirectory(appBaseDir());
				}
				case IDCK_SM_DELETE:
				{
					UMesh*	Mesh=pViewport->Actor->Mesh;
					pViewport->Actor->Mesh = NULL;
					pViewport->MiscRes = NULL;
					pViewport->Actor->Mesh->RemoveFromRoot();
					FString Name = Mesh->GetName();
					TArray<UObject*> Res;
					FCheckObjRefArc Ref(&Res,Mesh,false);
					if( Ref.HasReferences() )
					{
						pViewport->Actor->Mesh = Mesh;
						pViewport->MiscRes = Mesh;
						FString RefStr(FString::Printf(TEXT("%ls"),Res(0)->GetFullName()));
						for( INT i=1; i<Min<INT>(4,Res.Num()); i++ )
						{
							RefStr+=FString::Printf(TEXT(", %ls"),Res(i)->GetFullName());
							if( i==3 && Res.Num()>4 )
								RefStr+=TEXT(", etc...");
						}
						appMsgf( TEXT("StaticMesh %ls is still being referenced by: %ls"),Mesh->GetName(),*RefStr );
					}
					else
					{
						Mesh->ConditionalDestroy();
						delete Mesh;
					}
					RefreshAll();
				}
				break;
				case IDCK_SM_RENAME:
				{
					WDlgRename dlg(pViewport->Actor->Mesh, this );
					if( dlg.DoModal() )
						RefreshAll();
				}
				break;
				case IDPB_MB_FileSave:
				{

					OPENFILENAMEA ofn;
					char File[8192] = "\0";
					FString Name = pViewport->Actor->Mesh->GetName();
					FString Package = pMeshPackCombo->GetString( pMeshPackCombo->GetCurrent() );
					::sprintf_s( File,256, "%ls.usm", *Package );

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(char) * 8192;
					ofn.lpstrFilter = "Mesh Packages (*.usm)\0*.usm\0All Files\0*.*\0\0";
					ofn.lpstrInitialDir = "..\\Meshes";
					ofn.lpstrDefExt = "usm";
					ofn.lpstrTitle = "Save Mesh Package";
					ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;

					if( GetSaveFileNameA(&ofn) )
					{
						appSprintf( l_chCmd, TEXT("OBJ SAVEPACKAGE PACKAGE=\"%ls\" FILE=\"%ls\""),*Package, appFromAnsi( File ) );
						GEditor->Exec( l_chCmd );

						mrulist->AddItem( appFromAnsi( File ) );
						if( GBrowserMaster->GetCurrent()==BrowserID )
							mrulist->AddToMenu( hWnd, GetMenu( IsDocked() ? OwnerWindow->hWnd : hWnd ) );
					}

					GFileManager->SetDefaultDirectory(appBaseDir());
				}
				break;

			case IDPB_MB_FileOpen:
				{
					OPENFILENAMEA ofn;
					char File[8192] = "\0";

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(char) * 8192;
					ofn.lpstrFilter = "Mesh Packages (*.usm)\0*.usm\0All Files\0*.*\0\0";
					ofn.lpstrInitialDir = "..\\Meshes";
					ofn.lpstrDefExt = "usm";
					ofn.lpstrTitle = "Open Mesh Package";
					ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

					if( GetOpenFileNameA(&ofn) )
					{
						int iNULLs = FormatFilenames( File );

						TArray<FString> StringArray;
						ParseStringToArray( TEXT("|"), appFromAnsi( File ), &StringArray );

						int iStart = 0;
						FString Prefix = TEXT("\0");

						if( iNULLs )
						{
							iStart = 1;
							Prefix = *(StringArray(0));
							Prefix += TEXT("\\");
						}

						if( StringArray.Num() > 0 )
						{
							if (StringArray.Num() == 1)
								SavePkgName = StringArray(0).GetFilenameOnly();
							else
								SavePkgName = StringArray(1).GetFilenameOnly();
						}

						if( StringArray.Num() == 1 )
							GLastDir[eLASTDIR_USM] = StringArray(0).GetFilePath().LeftChop(1);
						else
							GLastDir[eLASTDIR_USM] = StringArray(0);

						GWarn->BeginSlowTask( TEXT(""), 1, 0 );

						for( int x = iStart ; x < StringArray.Num() ; x++ )
						{
							GWarn->StatusUpdatef( x, StringArray.Num(), TEXT("Loading %ls"), *(StringArray(x)) );

							appSprintf( l_chCmd, TEXT("OBJ LOAD FILE=\"%ls%ls\""), *Prefix, *(StringArray(x)) );
							GEditor->Exec( l_chCmd );

							mrulist->AddItem( *(StringArray(x)) );
							if( GBrowserMaster->GetCurrent()==BrowserID )
								mrulist->AddToMenu( hWnd, GetMenu( IsDocked() ? OwnerWindow->hWnd : hWnd ) );
						}


						GWarn->EndSlowTask();
						GBrowserMaster->RefreshAll();
						pMeshPackCombo->SetCurrent( pMeshCombo->FindStringExact( *SavePkgName ) );
						RefreshAll();
					}

					GFileManager->SetDefaultDirectory(appBaseDir());
				}
				break;
			case ID_EDIT_COPYIMPORTPROPERTIES:
				{
					GEditor->Exec(*FString::Printf(TEXT("MESH GETPROPERTIES MESH=%ls"), pViewport->Actor->Mesh->GetPathName()));
				}
				break;

			case IDMN_MRU1:
			case IDMN_MRU2:
			case IDMN_MRU3:
			case IDMN_MRU4:
			case IDMN_MRU5:
			case IDMN_MRU6:
			case IDMN_MRU7:
			case IDMN_MRU8:
			{
				FString Filename = mrulist->Items[Command - IDMN_MRU1];
				GEditor->Exec( *(FString::Printf(TEXT("OBJ LOAD FILE=\"%ls\""), *Filename )) );

				GBrowserMaster->RefreshAll();
			}
			break;

			case IDMN_RD_SOFTWARE:
			case IDMN_RD_OPENGL:
			case IDMN_RD_D3D:
			case IDMN_RD_D3D9:
			case IDMN_RD_XOPENGL:
			case IDMN_RD_ICBINDX11:
			case IDMN_RD_CUSTOMRENDER:
			{
				SwitchRenderer(Command, TRUE);
				check(pViewport);
				FlushRepaintViewport();
				PositionChildControls();
				UpdateMenu();
				break;
			}

			default:
				WBrowser::OnCommand(Command);
				break;
		}
		unguard;
	}

	// Notification delegates for child controls.
	//
	void OnMPackageSelectionChange()
	{
		guard(WBrowserMesh::OnMPackageSelectionChange);
		pViewport->Actor->Misc2 = 0;
		RefreshAll();
		unguard;
	}
	void OnMeshSelectionChange()
	{
		guard(WBrowserMesh::OnMeshSelectionChange);
		pViewport->Actor->Misc2 = 0;
		RefreshAnimList();
		RefreshViewport();
		SetCaption();
		unguard;
	}
	void OnAnimDoubleClick()
	{
		guard(WBrowserMesh::OnAnimDoubleClick);
		OnPlay();
		unguard;
	}
	void OnAnimSelectionChange()
	{
		guard(WBrowserMesh::OnAnimSelectionChange);
		pViewport->Actor->Misc2 = 0;
		RefreshViewport();
		unguard;
	}
	void OnAnimRightClick()
	{
		guard(WBrowserMesh::OnAnimRightClick);
		OnAnimSelectionChange();
		MENU_AnimContextMenu(hWnd, pViewport->Actor->AnimSequence, pViewport->Actor->Mesh);
		unguard;
	}
	void OnObjectsClick()
	{
		guard(WBrowserMesh::OnObjectsClick);
		pViewport->Actor->Misc2 = 0;
		RefreshAll();
		SetCaption();
		unguard;
	}
	virtual void OnVScroll(WPARAM wParam, LPARAM lParam)
	{
		if (lParam == WM_MOUSEWHEEL)
		{
			switch (LOWORD(wParam))
			{
				case SB_LINEUP:
					OnShowPrev();
					break;

				case SB_LINEDOWN:
					OnShowNext();
					break;
				default:
					break;
			}
		}
	}
};
WBrowserMesh* GBrowserMesh = NULL;

static FString GetExportName(UObject* Obj)
{
	if (!Obj)
		return TEXT("None");
	return FString::Printf(TEXT("%ls'%ls'"), Obj->GetClass()->GetName(), Obj->GetPathName());
}
void RC_AnimOpts(INT ID)
{
	FString ClipName;
	if (ID == 0)
		ClipName = *GBrowserMesh->pViewport->Actor->AnimSequence;
	else if (ID == 1)
		ClipName = GetExportName(GBrowserMesh->pViewport->Actor->Mesh);
	else ClipName = GetExportName(CastChecked<USkeletalMesh>(GBrowserMesh->pViewport->Actor->Mesh)->DefaultAnimation);
	appClipboardCopy(*ClipName);
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
