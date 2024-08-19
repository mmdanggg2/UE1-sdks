/*=============================================================================
	BrowserMesh : Browser window for meshes
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>

#if ENGINE_VERSION==227
const TCHAR* USM_EXT = TEXT("usm");
const TCHAR* USM_DIR = TEXT("..\\Meshes");
const TCHAR* USM_FILTER = TEXT("Mesh Packages (*.usm)\0*.usm\0All Files\0*.*\0\0");
#else
const TCHAR* USM_EXT = TEXT("u");
const TCHAR* USM_DIR = TEXT("..\\System");
const TCHAR* USM_FILTER = TEXT("Mesh Packages (*.u)\0*.u\0All Files\0*.*\0\0");
#endif

EDITOR_API extern INT GLastScroll;

extern void Query( ULevel* Level, const TCHAR* Item, FString* pOutput, UPackage* InPkg );

// --------------------------------------------------------------
//
// WBrowserMesh
//
// --------------------------------------------------------------

#define ID_MESH_TOOLBAR	29050
TBBUTTON tbMESHButtons[] = {
	{ 0, IDMN_MB_DOCK, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 1, ID_FileOpen, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 2, ID_FileSave, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 3, IDMN_LOAD_ENTIRE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
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
	, { 10, IDPB_SHOWSKEL, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 11, IDPB_SHOWSKELNAM, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
#endif
};
struct {
	TCHAR ToolTip[64];
	int ID;
} ToolTips_MESH[] = {
	TEXT("Toggle Dock Status"), IDMN_MB_DOCK,
	TEXT("Open Package"), ID_FileOpen,
	TEXT("Save Package"), ID_FileSave,
	TEXT("Load Entire Package"), IDMN_LOAD_ENTIRE,
	TEXT("Play Animation"), IDPB_PLAY,
	TEXT("Stop Animation"), IDPB_STOP,
	TEXT("Next Frame in Animation"), IDPB_NEXT_FRAME,
	TEXT("Previous Frame in Animation"), IDPB_PREV_FRAME,
	TEXT("Edit Mesh Properties"), IDPB_EDIT,
#if ENGINE_VERSION==227
	TEXT("Add Mesh into Map"), IDPB_ADD_MESH,
	TEXT("Show/Hide Skeletal mesh skeleton"), IDPB_SHOWSKEL,
	TEXT("Show/Hide Skeletal mesh bone names"), IDPB_SHOWSKELNAM,
#endif
	NULL, 0
};

#define CPPOBJPROPERTY(obj,name) EC_CppProperty, (BYTE*)&((obj*)1)->name - (BYTE*)1
void InitMeshProperties()
{
	static bool bHasInitProps = false;
	if (bHasInitProps)
		return;
	bHasInitProps = true;
	UStruct* Vect = FindObjectChecked<UStruct>(NULL, TEXT("Core.Object.Vector"), 1);
	UStruct* Rot = FindObjectChecked<UStruct>(NULL, TEXT("Core.Object.Rotator"), 1);
	UStruct* Crds = FindObjectChecked<UStruct>(NULL, TEXT("Core.Object.Coords"), 1);
	UStruct* BX = FindObjectChecked<UStruct>(NULL, TEXT("Core.Object.BoundingBox"), 1);
	UStruct* SPX = FindObjectChecked<UStruct>(NULL, TEXT("Core.Object.Plane"), 1);
	new(UMesh::StaticClass(), TEXT("Scale"), 0)UStructProperty(CPPOBJPROPERTY(UMesh, Scale), TEXT("Mesh"), (CPF_Edit | CPF_Native), Vect);
	new(UMesh::StaticClass(), TEXT("Origin"), 0)UStructProperty(CPPOBJPROPERTY(UMesh, Origin), TEXT("Mesh"), (CPF_Edit | CPF_Native), Vect);
	new(UMesh::StaticClass(), TEXT("RotOrigin"), 0)UStructProperty(CPPOBJPROPERTY(UMesh, RotOrigin), TEXT("Mesh"), (CPF_Edit | CPF_Native), Rot);
	new(USkeletalMesh::StaticClass(), TEXT("WeaponAdjust"), 0)UStructProperty(CPPOBJPROPERTY(USkeletalMesh, WeaponAdjust), TEXT("SkeletalMesh"), (CPF_Edit | CPF_Native), Crds);
	new(USkeletalMesh::StaticClass(), TEXT("BoundingBox"), 0)UStructProperty(CPPOBJPROPERTY(USkeletalMesh, BoundingBox), TEXT("SkeletalMesh"), (CPF_Edit | CPF_Native), BX);
	new(USkeletalMesh::StaticClass(), TEXT("BoundingSphere"), 0)UStructProperty(CPPOBJPROPERTY(USkeletalMesh, BoundingSphere), TEXT("SkeletalMesh"), (CPF_Edit | CPF_Native), SPX);
}
#undef CPPOBJPROPERTY

class WBrowserMesh : public WBrowser, public WViewportWindowContainer
{
	DECLARE_WINDOWCLASS(WBrowserMesh,WBrowser,Window)

	WComboBox* pMeshCombo;
	WComboBox* pMeshPackCombo;
	WComboBox* pMeshComboGroup;
	WListBox* pAnimList;
	WCheckBox* pMeshCheck;
	WCheckBox* pSkeletalMeshCheck;
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

	UObject* GetParentOuter(UObject* O)
	{
		while (O->GetOuter())
			O = O->GetOuter();
		return O;
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
		SendMessageX(pStaticMeshCheck->hWnd, BM_SETCHECK, BST_CHECKED, 0);
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
		pAnimList->CommandDelegate = FDelegateInt(this, (TDelegateInt)&WBrowserMesh::OnCommand);

		ToolbarImage = reinterpret_cast<HBITMAP>(LoadImage(hInstance,
			MAKEINTRESOURCE(IDB_BrowserMesh_TOOLBAR),
			IMAGE_BITMAP, 0, 0, 0));
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
			13,
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

		if( GetCurrentMeshName().Len() )
			Caption += FString::Printf( TEXT(" - %ls"),	*GetCurrentMeshName() );

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
			PList.AddUniqueItem(GetParentOuter(M));
		}
		for (INT i = 0; i < PList.Num(); i++)
			pMeshPackCombo->AddString(*FObjectName(PList(i)));
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
			if (!IsValidMesh(M) || PName != *FObjectName(GetParentOuter(M)))
				continue;
			pMeshCombo->AddString(*FObjectName(M));
		}
		for (TObjectIterator<UMesh> It; It; ++It)
		{
			UMesh* M = *It;
			if (!IsValidSkeletalMesh(M) || PName != *FObjectName(GetParentOuter(M)))
				continue;
			pMeshCombo->AddString(*FObjectName(M));
		}
#if ENGINE_VERSION==227
		for (TObjectIterator<UMesh> It; It; ++It)
		{
			UMesh* M = *It;
			if (!IsValidStaticMesh(M) || PName != *FObjectName(GetParentOuter(M)))
				continue;
			pMeshCombo->AddString(*FObjectName(M));
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
		FString FullName = pMeshCombo->GetString( pMeshCombo->GetCurrent() );
		if (FullName.Len() > 0)
			FullName = pMeshPackCombo->GetString(pMeshPackCombo->GetCurrent()) + TEXT(".") + FullName;
		return FullName;
		unguard;
	}
	void RefreshAnimList()
	{
		guard(WBrowserMesh::RefreshAnimList);

		FString MeshName = GetCurrentMeshName();

		pAnimList->Empty();

		FStringOutputDevice GetPropResult = FStringOutputDevice();
		GEditor->Get( TEXT("MESH"), *(FString::Printf(TEXT("NUMANIMSEQS NAME=%ls"), *MeshName)), GetPropResult );
		int NumAnims = appAtoi( *GetPropResult );

		for( int anim = 0 ; anim < NumAnims ; anim++ )
		{
			FStringOutputDevice GetPropResult = FStringOutputDevice();
			GEditor->Get( TEXT("MESH"), *(FString::Printf(TEXT("ANIMSEQ NAME=%ls NUM=%d"), *MeshName, anim)), GetPropResult );

			int NumFrames = appAtoi( *(GetPropResult.Right(3)) );
			FString Name = GetPropResult.Left( GetPropResult.InStr(TEXT(" ")));

			pAnimList->AddString( *(FString::Printf(TEXT("%ls [ %d ]"), *Name, NumFrames )) );
		}

		pAnimList->SetCurrent(0, 1);
		pAnimList->AutoResizeColumns();

		unguard;
	}
	void RefreshViewport()
	{
		guard(WBrowserMesh::RefreshViewport);

		FString MeshName = pMeshCombo->GetString(pMeshCombo->GetCurrent());
		if( !pViewport )
		{
			FString Device;
			GConfig->GetString(*ConfigName, TEXT("Device"), Device, GUEDIni);
			CreateViewport(SHOW_StandardView | SHOW_NoButtons | SHOW_ChildWindow, REN_MeshView, 320, 200, -1, -1, *Device);
			check(pViewport);
			pViewport->MiscRes = FindObject<UMesh>(ANY_PACKAGE, *MeshName);
		}
		else
		{
			FStringOutputDevice GetPropResult = FStringOutputDevice();
			GEditor->Get( TEXT("MESH"), *(FString::Printf(TEXT("ANIMSEQ NAME=\"%ls\" NUM=%d"), *MeshName, pAnimList->GetCurrent())), GetPropResult );

			GEditor->Exec( *(FString::Printf(TEXT("CAMERA UPDATE NAME=MeshBrowser MESH=\"%ls\" FLAGS=%d REN=%d MISC1=%d MISC2=%d"),
				*MeshName,
				bPlaying
				? SHOW_StandardView | SHOW_NoButtons | SHOW_ChildWindow | SHOW_Backdrop | SHOW_RealTime
				: SHOW_StandardView | SHOW_NoButtons | SHOW_ChildWindow,
				REN_MeshView,
				appAtoi(*(GetPropResult.Right(7).Left(3))),
				pViewport->Actor->Misc2
				)));
		}		

		PositionChildControls();
		pViewport->Repaint(1);
		unguard;
	}
	void OnDestroy()
	{
		guard(WBrowserMesh::OnDestroy);		

		GEditor->Exec(TEXT("CAMERA CLOSE NAME=MeshViewer"));
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
		::MoveWindow((HWND)pViewport->GetWindow(), 4, Top, (CR.Width() - 8) & 0xFFFFFFFE, CR.Height() - Top, 1);
		pViewport->Repaint(1);

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
		UpdateRendererMenu(menu, FALSE);

		if (mrulist && GBrowserMaster->GetCurrent() == BrowserID)
			mrulist->AddToMenu(hWnd, GetMenu(IsDocked() ? OwnerWindow->hWnd : hWnd));

		unguard;
	}
	void OnCommand(INT Command)
	{
		guard(WBrowserMesh::OnCommand);
		switch (Command)
		{
			case ID_PlayPauseTrack:
			case IDPB_PLAY:
				OnPlay();
				break;
			case ID_StopTrack:
			case IDPB_STOP:
				OnStop();
				break;
			case IDPB_EDIT:
				InitMeshProperties();
				GEditor->Exec(*(FString::Printf(TEXT("HOOK MESHPROPERTIES MESH=\"%ls\""), *GetCurrentMeshName())));
				break;
			case IDPB_SHOWSKEL:
				OnShowSkele();
				break;
			case IDPB_SHOWSKELNAM:
				OnShowSkelNames();
				break;
			case ID_NextTrack:
			case IDPB_NEXT_FRAME:
				OnShowNext();
				break;
			case ID_PrevTrack:
			case IDPB_PREV_FRAME:
				OnShowPrev();
				break;		
			case IDMN_LOAD_ENTIRE:
			{
				FString Package = pMeshPackCombo->GetString(pMeshPackCombo->GetCurrent());
				UObject::LoadPackage(NULL, *Package, LOAD_NoWarn);
				GBrowserMaster->RefreshAll();
				break;
			}

			case IDPB_IMPORT:
			{
				TArray<FString> Files;
				if (OpenFilesWithDialog(
					*(GLastDir[eLASTDIR_USM]),
					TEXT("Import Types (*.obj)\0*.obj;\0All Files\0*.*\0\0"),
					TEXT("obj"),
					TEXT("Import Static Mesh (Wavefront .obj)"),
					Files))
				{
					FString Package = pMeshPackCombo->GetString(pMeshPackCombo->GetCurrent());
					FString Group = TEXT("None");// pMeshComboGroup->GetString( pMeshComboGroup->GetCurrent() );

					if (Files.Num() > 0)
						GLastDir[eLASTDIR_USM] = appFilePathName(*Files(0));

					WDlgStaticMeshImport l_dlg(NULL, this);
					l_dlg.DoModal(Package, Group, &Files);

					GBrowserMaster->RefreshAll();

					pMeshPackCombo->SetCurrent(pMeshPackCombo->FindStringExact(*l_dlg.Package));
					//pMeshComboGroup->SetCurrent( pMeshComboGroup->FindStringExact( *l_dlg.Group) );
					GBrowserMaster->RefreshAll();
				}
				GFileManager->SetDefaultDirectory(appBaseDir());
			}
			RefreshAll();
			break;

			case IDPB_EXPORT:
			{
				FString File;
				if (GetSaveNameWithDialog(
					*FObjectName(pViewport->Actor->Mesh),
					TEXT(""),
					TEXT("OBJ Files (*.obj)\0*.obj;\0All Files\0*.*\0\0"),
					TEXT("obj"),
					TEXT("Export to Wavefront (.obj)"),
					File))
				{
					INT Exported = GEditor->Exec(*FString::Printf(TEXT("OBJ EXPORTOBJ FILE=\"%ls\" ACTOR=%ls"), *File, *FObjectName(pViewport->Actor)));
					appMsgf(TEXT("Successfully exported %d of 1 meshes"), Exported);
				}
					
				GFileManager->SetDefaultDirectory(appBaseDir());
			}
			break;
			case IDPB_EXPORT_ALL:
			{
				FString File;
				if (GetSaveNameWithDialog(
					*FObjectName(pViewport->Actor->Mesh),
					TEXT(""),
					TEXT("OBJ Files (*.obj)\0*.obj;\0All Files\0*.*\0\0"),
					TEXT("obj"),
					TEXT("Export all animation frames to Wavefront (.obj)"),
					File))
				{
					INT Exported = GEditor->Exec(*FString::Printf(TEXT("OBJ EXPORTOBJ FILE=\"%ls\" ACTOR=%ls EXPORTALL"), *File, *FObjectName(pViewport->Actor)));
					appMsgf(TEXT("Successfully exported %d meshes"), Exported);
				}
				
				GFileManager->SetDefaultDirectory(appBaseDir());
			}
			break;

			case IDPM_EXPORT:
			{
				if (pViewport->Actor->Mesh->IsA(USkeletalMesh::StaticClass()))
				{
					appMsgf(TEXT("Can't export %ls as .3d file!"), *FObjectFullName(pViewport->Actor->Mesh));
					break;
				}

				FString File;
				if (GetSaveNameWithDialog(
					*FObjectName(pViewport->Actor->Mesh),
					TEXT(""),
					TEXT("3d Files (*.3d)\0*.3d;\0All Files\0*.*\0\0"),
					TEXT("3d"),
					TEXT("Export Unreal Mesh (.3d)"),
					File))
				{
					INT Exported = GEditor->Exec(*FString::Printf(TEXT("MESH EXPORT MESH=%ls FILENAME=\"%ls\""), *FObjectName(pViewport->Actor->Mesh), *File));
					appMsgf(TEXT("Successfully exported %d of 1 meshes"), Exported);
				}									
				GFileManager->SetDefaultDirectory(appBaseDir());
			}
			break;
			case IDPM_EXPORT_ALL:
			{
				FString CurrentPackage = pMeshPackCombo->GetString(pMeshPackCombo->GetCurrent());
				FString OutDir = FString::Printf(TEXT("..%ls%ls%lsModels"), PATH_SEPARATOR, *CurrentPackage, PATH_SEPARATOR);
				GFileManager->MakeDirectory(*OutDir, 1);
				GFileManager->SetDefaultDirectory(*OutDir);

				INT Exported = 0, Expected = 0;
				for (TObjectIterator<UMesh> It; It; ++It)
				{
					UMesh* M = *It;
					UClass* MeshClass = M->GetClass();
					if ((MeshClass == UMesh::StaticClass() || MeshClass == ULodMesh::StaticClass()) &&
						MeshClass != USkeletalMesh::StaticClass() &&
						(CurrentPackage == *FObjectName(M->GetOuter())))
					{
						debugf(TEXT("Exporting: %ls.3d"), *FObjectName(M));
						Exported += GEditor->Exec(*FString::Printf(TEXT("MESH EXPORT MESH=%ls FILENAME=\"%ls.3d\""), *FObjectName(M), *FObjectName(M)));
						Expected++;
					}
					else if (CurrentPackage == *FObjectName(M->GetOuter()))
						debugf(TEXT("Skipping %ls"), *FObjectFullName(M));
				}

				if (Expected > 0)
					appMsgf(TEXT("Successfully exported %d of %d meshes"), Exported, Expected);
					
				GFileManager->SetDefaultDirectory(appBaseDir());
			}
			break;
			case IDPSKM_EXPORT:
			{
				if (!pViewport->Actor->Mesh->IsA(USkeletalMesh::StaticClass()))
				{
					appMsgf(TEXT("Can't export %ls as .psk file!"), *FObjectFullName(pViewport->Actor->Mesh));
					break;
				}

				FString File;
				INT Exported = 0;
				if (GetSaveNameWithDialog(
					*FObjectName(pViewport->Actor->Mesh),
					TEXT(""),
					TEXT("PSK Files (*.psk)\0*.psk;\0All Files\0*.*\0\0"),
					TEXT("psk"),
					TEXT("Export Unreal SkeletalMesh (.psk)"),
					File))
				{
					if (UExporter::ExportToFile(pViewport->Actor->Mesh, NULL, *File, 0, 0))
					{
						Exported = 1;
						GLog->Logf(TEXT("Exported %ls to %ls"), *FObjectFullName(pViewport->Actor->Mesh), *File);
					}
					else
						GLog->Logf(TEXT("Can't export %ls to file %ls"), *FObjectFullName(pViewport->Actor->Mesh), *File);
				}

				appMsgf(TEXT("Successfully exported %d of 1 meshes"), Exported);
				GFileManager->SetDefaultDirectory(appBaseDir());
			}
			break;
			case IDPSKM_EXPORT_ALL:
			{
				FString CurrentPackage = pMeshPackCombo->GetString(pMeshPackCombo->GetCurrent());
				FString OutDir = FString::Printf(TEXT("..%ls%ls%lsModels"), PATH_SEPARATOR, *CurrentPackage, PATH_SEPARATOR);
				GFileManager->MakeDirectory(*OutDir, 1);
				GFileManager->SetDefaultDirectory(*OutDir);

				INT Exported = 0, Expected = 0;
				for (TObjectIterator<UObject> It; It; ++It)
				{
					if (It->IsA(USkeletalMesh::StaticClass()) && CurrentPackage == *FObjectName(It->GetOuter()))
					{
						FString PackageWithDot = CurrentPackage;
						PackageWithDot = PackageWithDot.Caps() + TEXT(".");
						FString WithGroup = FObjectPathName(*It);
						if (WithGroup.Left(PackageWithDot.Len()).Caps() == PackageWithDot)
							WithGroup = WithGroup.Mid(PackageWithDot.Len());
						else
							WithGroup = *FObjectName(*It);

						FString Filename = OutDir * WithGroup + TEXT(".") + TEXT("psk");
						Expected++;
						if (UExporter::ExportToFile(*It, NULL, *Filename, 0, 0))
						{
							Exported++;							
							GLog->Logf(TEXT("Exported %ls to %ls"), *FObjectFullName(*It), *Filename);
						}
						else
							GLog->Logf(TEXT("Can't export %ls to file %ls"), *FObjectFullName(*It), *Filename);
					}
				}

				if (Expected > 0)
					appMsgf(TEXT("Successfully exported %d of %d meshes"), Exported, Expected);
				GFileManager->SetDefaultDirectory(appBaseDir());
			}
#if ENGINE_VERSION==227
			case IDCK_SM_DELETE:
			{
				UMesh* Mesh = pViewport->Actor->Mesh;
				pViewport->Actor->Mesh = NULL;
				pViewport->MiscRes = NULL;
				pViewport->Actor->Mesh->RemoveFromRoot();
				FString Name = *FObjectName(Mesh);
				TArray<UObject*> Res;
				FCheckObjRefArc Ref(&Res, Mesh, false);
				if (Ref.HasReferences())
				{
					pViewport->Actor->Mesh = Mesh;
					pViewport->MiscRes = Mesh;
					FString RefStr(FString::Printf(TEXT("%ls"), *FObjectFullName(Res(0))));
					for (INT i = 1; i < Min<INT>(4, Res.Num()); i++)
					{
						RefStr += FString::Printf(TEXT(", %ls"), *FObjectFullName(Res(i)));
						if (i == 3 && Res.Num() > 4)
							RefStr += TEXT(", etc...");
					}
					appMsgf(TEXT("StaticMesh %ls is still being referenced by: %ls"), *FObjectName(Mesh), *RefStr);
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
				WDlgRename dlg(NULL, this);
				if (dlg.DoModal(*FObjectName(pViewport->Actor->Mesh)))
				{
					pViewport->Actor->Mesh->Rename(*dlg.NewName);

				}
				RefreshAll();
			}
			break;
#endif
			case ID_FileSave:
			{
				FString Package = pMeshPackCombo->GetString(pMeshPackCombo->GetCurrent());					
				FString File;
				if (GetSaveNameWithDialog(
					*FString::Printf(TEXT("%ls.%ls"), *Package, USM_EXT),
					USM_DIR,
					USM_FILTER,
					USM_EXT,
					TEXT("Save Mesh Package"),
					File) && AllowOverwrite(*File))
				{
					GEditor->Exec(*FString::Printf(TEXT("OBJ SAVEPACKAGE PACKAGE=\"%ls\" FILE=\"%ls\""), *Package, *File));
					mrulist->AddItem(*File);
					mrulist->WriteINI();
					GConfig->Flush(0);
					if (GBrowserMaster->GetCurrent() == BrowserID)
						mrulist->AddToMenu(hWnd, GetMenu(IsDocked() ? OwnerWindow->hWnd : hWnd));
				}

				GFileManager->SetDefaultDirectory(appBaseDir());
			}
			break;

			case ID_FileOpen:
			{
				TArray<FString> Files;
				if (OpenFilesWithDialog(
					USM_DIR,
					USM_FILTER,
					USM_EXT,
					TEXT("Open Mesh Package"),
					Files
				))
				{
					if (Files.Num() > 0)
					{
						GLastDir[eLASTDIR_USM] = appFilePathName(*Files(0));
						SavePkgName = appFileBaseName(*(Files(0)));
						SavePkgName = SavePkgName.Left(SavePkgName.InStr(TEXT(".")));
					}
					
					GWarn->BeginSlowTask(TEXT(""), 1, 0);

					for (int x = 0; x < Files.Num(); x++)
					{
						GWarn->StatusUpdatef(x, Files.Num(), TEXT("Loading %ls"), *(Files(x)));
						GEditor->Exec(*FString::Printf(TEXT("OBJ LOAD FILE=\"%ls\""), *(Files(x))));

						mrulist->AddItem(*(Files(x)));
						mrulist->WriteINI();
						GConfig->Flush(0);
						if (GBrowserMaster->GetCurrent() == BrowserID)
							mrulist->AddToMenu(hWnd, GetMenu(IsDocked() ? OwnerWindow->hWnd : hWnd));
					}


					GWarn->EndSlowTask();
					GBrowserMaster->RefreshAll();
					pMeshPackCombo->SetCurrent(pMeshCombo->FindStringExact(*SavePkgName));
					RefreshAll();
				}

				GFileManager->SetDefaultDirectory(appBaseDir());
			}
			break;
			case ID_EDIT_COPYIMPORTPROPERTIES:
			{
				GEditor->Exec(*FString::Printf(TEXT("MESH GETPROPERTIES MESH=%ls"), *FObjectPathName(pViewport->Actor->Mesh)));
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
				GEditor->Exec(*(FString::Printf(TEXT("OBJ LOAD FILE=\"%ls\""), *Filename)));
				GBrowserMaster->RefreshAll();
			}
			break;

			case IDMN_RD_SOFTWARE:
			case IDMN_RD_OPENGL:
			case IDMN_RD_D3D:
			case IDMN_RD_D3D9:
			case IDMN_RD_XOPENGL:
			case IDMN_RD_CUSTOM0:
			case IDMN_RD_CUSTOM1:
			case IDMN_RD_CUSTOM2:
			case IDMN_RD_CUSTOM3:
			case IDMN_RD_CUSTOM4:
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

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
