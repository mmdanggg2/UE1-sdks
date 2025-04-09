/*=============================================================================
	BrowserMusic : Browser window for music files
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>

// --------------------------------------------------------------
//
// IMPORT MUSIC Dialog
//
// --------------------------------------------------------------

class WDlgImportMusic : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgImportMusic,WDialog,UnrealEd)

	// Variables.
	WButton OkButton;
	WButton OkAllButton;
	WButton SkipButton;
	WButton CancelButton;
	WLabel FilenameStatic;
	WPackageComboBox PackageEdit;
	WComboBox GroupEdit;
	WEdit NameEdit;

	FString defPackage, defGroup;
	TArray<FString>* paFilenames;

	FString Package, Group, Name;
	BOOL bOKToAll;
	int iCurrentFilename;

	// Constructor.
	WDlgImportMusic( UObject* InContext, WBrowser* InOwnerWindow )
		: WDialog(TEXT("Import Music"), IDDIALOG_IMPORT_MUSIC, InOwnerWindow)
		, OkButton(this, IDOK, FDelegate(this, (TDelegate)&WDlgImportMusic::OnOk))
		, OkAllButton(this, IDPB_OKALL, FDelegate(this, (TDelegate)&WDlgImportMusic::OnOkAll))
		, SkipButton(this, IDPB_SKIP, FDelegate(this, (TDelegate)&WDlgImportMusic::OnSkip))
		, CancelButton(this, IDCANCEL, FDelegate(this, (TDelegate)&WDlgImportMusic::EndDialogFalse))
		, FilenameStatic(this, IDSC_FILENAME)
		, NameEdit(this, IDEC_NAME)
		, GroupEdit(this, IDEC_GROUP)
		, PackageEdit(this, IDEC_PACKAGE)
		, paFilenames(NULL)
		, bOKToAll(FALSE)
		, iCurrentFilename(0)
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgImportMusic::OnInitDialog);
		WDialog::OnInitDialog();

		PackageEdit.Init(!defPackage.Len() && paFilenames && paFilenames->Num() ? *GetFilenameOnly((*paFilenames)(0)) : *defPackage, &GroupEdit);
		GroupEdit.SetText(*defGroup);
		::SetFocus( NameEdit.hWnd );

		bOKToAll = FALSE;
		iCurrentFilename = -1;
		SetNextFilename();

		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgImportMusic::OnDestroy);
		WDialog::OnDestroy();
		unguard;
	}
	virtual int DoModal(FString _defPackage, FString _defGroup, TArray<FString>* _paFilenames)
	{
		guard(WDlgImportMusic::DoModal);

		defPackage = _defPackage;
		defGroup = _defGroup;
		paFilenames = _paFilenames;

		return WDialog::DoModal(hInstance);
		unguard;
	}
	void OnOk()
	{
		guard(WDlgImportMusic::OnOk);
		if( GetDataFromUser() )
		{
			ImportFile( (*paFilenames)(iCurrentFilename) );
			SetNextFilename();
		}
		unguard;
	}
	void OnOkAll()
	{
		guard(WDlgImportMusic::OnOkAll);
		if( GetDataFromUser() )
		{
			ImportFile( (*paFilenames)(iCurrentFilename) );
			bOKToAll = TRUE;
			SetNextFilename();
		}
		unguard;
	}
	void OnSkip()
	{
		guard(WDlgImportMusic::OnSkip);
		if( GetDataFromUser() )
			SetNextFilename();
		unguard;
	}
	void ImportTexture( void )
	{
		guard(WDlgImportMusic::ImportTexture);
		unguard;
	}
	void RefreshName( void )
	{
		guard(WDlgImportMusic::RefreshName);
		FilenameStatic.SetText( *(*paFilenames)(iCurrentFilename) );

		FString Name = GetFilenameOnly( (*paFilenames)(iCurrentFilename) );
		NameEdit.SetText( *Name );
		unguard;
	}
	BOOL GetDataFromUser( void )
	{
		guard(WDlgImportMusic::GetDataFromUser);
		Package = PackageEdit.GetText();
		Group = GroupEdit.GetText();
		Name = NameEdit.GetText();

		if (!Package.Len() || !Name.Len())
		{
			appMsgf( TEXT("Invalid input.") );
			return FALSE;
		}

		return TRUE;
		unguard;
	}
	void SetNextFilename( void )
	{
		guard(WDlgImportMusic::SetNextFilename);
		iCurrentFilename++;
		if( iCurrentFilename == paFilenames->Num() ) 
		{
			EndDialogTrue();
			return;
		}

		if( bOKToAll ) {
			RefreshName();
			GetDataFromUser();
			ImportFile( (*paFilenames)(iCurrentFilename) );
			SetNextFilename();
			return;
		};

		RefreshName();

		unguard;
	}
	void ImportFile( FString Filename )
	{
		guard(WDlgImportMusic::ImportFile);
		GEditor->Exec( 
			Group.Len() > 0
			? *FString::Printf(TEXT("MUSIC IMPORT TYPE=MUSIC FILE=\"%ls\" NAME=\"%ls\" PACKAGE=\"%ls\" GROUP=\"%ls\""),	*Filename, *Name, *Package, *Group)
			: *FString::Printf(TEXT("MUSIC IMPORT TYPE=MUSIC FILE=\"%ls\" NAME=\"%ls\" PACKAGE=\"%ls\""), *Filename, *Name, *Package));
		unguard;
	}
};

// --------------------------------------------------------------
//
// WBrowserMusic
//
// --------------------------------------------------------------

#define ID_BM_TOOLBAR	29030
TBBUTTON tbBMButtons[] = {
	{ 0, IDMN_MB_DOCK, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 1, ID_FileOpen, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 2, ID_FileSave, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 3, IDMN_LOAD_ENTIRE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 4, IDMN_MB_PLAY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 5, IDMN_MB_STOP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
};
struct {
	TCHAR ToolTip[64];
	int ID;
} ToolTips_BM[] = {
	TEXT("Toggle Dock Status"), IDMN_MB_DOCK,
	TEXT("Open Package"), ID_FileOpen,
	TEXT("Save Package"), ID_FileSave,
	TEXT("Load Entire Package"), IDMN_LOAD_ENTIRE,
	TEXT("Play"), IDMN_MB_PLAY,
	TEXT("Stop"), IDMN_MB_STOP,
	NULL, 0
};

class WBrowserMusic : public WBrowser
{
	DECLARE_WINDOWCLASS(WBrowserMusic,WBrowser,Window)

	WComboBox* pComboPackage, * pComboGroup;
	WListBox *pListMusic;
	WCheckBox* pCheckGroupAll;
	TArray<UMusic*> MusicList;

	// Structors.
	WBrowserMusic( FName InPersistentName, WWindow* InOwnerWindow, HWND InEditorFrame )
	:	WBrowser( InPersistentName, InOwnerWindow, InEditorFrame )
	{
		pComboPackage = pComboGroup = NULL;
		pListMusic = NULL;
		pCheckGroupAll = NULL;
		MenuID = IDMENU_BrowserMusic;
		BrowserID = eBROWSER_MUSIC;
		Description = TEXT("Music");
		mrulist = NULL;
	}

	// WBrowser interface.
	void OnCreate()
	{
		guard(WBrowserMusic::OnCreate);
		WBrowser::OnCreate();

		SetRedraw(false);

		SetMenu( hWnd, AddHotkeys(LoadMenuW(hInstance, MAKEINTRESOURCEW(IDMENU_BrowserMusic))) );

		// PACKAGES
		//
		pComboPackage = new WComboBox(this, IDCB_PACKAGE);
		pComboPackage->OpenWindow(1, 1);
		pComboPackage->SelectionChangeDelegate = FDelegate(this, (TDelegate)&WBrowserMusic::OnComboPackageSelChange);

		// GROUP
		//
		pComboGroup = new WComboBox(this, IDCB_GROUP);
		pComboGroup->OpenWindow(1, 1);
		pComboGroup->SelectionChangeDelegate = FDelegate(this, (TDelegate)&WBrowserMusic::OnComboGroupSelChange);

		// MUSIC LIST
		//
		pListMusic = new WListBox( this, IDLB_MUSIC );
		pListMusic->OpenWindow( 1, 0, 0, 0, 1 );
		pListMusic->DoubleClickDelegate = FDelegate(this, (TDelegate)&WBrowserMusic::OnListMusicDblClick);
		pListMusic->CommandDelegate = FDelegateInt(this, (TDelegateInt)&WBrowserMusic::OnCommand);

		ToolbarImage = reinterpret_cast<HBITMAP>(LoadImage(hInstance,
			MAKEINTRESOURCE(IDB_BrowserMusic_TOOLBAR),
			IMAGE_BITMAP, 0, 0, 0));
		check(ToolbarImage);
		ScaleImageAndReplace(ToolbarImage);

		// CHECK BOXES
		//
		pCheckGroupAll = new WCheckBox(this, IDCK_GRP_ALL, FDelegate(this, (TDelegate)&WBrowserMusic::OnGroupAllClick));
		pCheckGroupAll->OpenWindow(1, 0, 0, 1, 1, TEXT("All Groups"), 1, 0, BS_PUSHLIKE);

		hWndToolBar = CreateToolbarEx( 
			hWnd, WS_CHILD | WS_VISIBLE | CCS_ADJUSTABLE,
			IDB_BrowserMusic_TOOLBAR,
			5,
			NULL,
			reinterpret_cast<PTRINT>(ToolbarImage),
			(LPCTBBUTTON)&tbBMButtons,
			8,
			MulDiv(16, DPIX, 96), MulDiv(16, DPIY, 96),
			MulDiv(16, DPIX, 96), MulDiv(16, DPIY, 96),
			sizeof(TBBUTTON));
		check(hWndToolBar);

		ToolTipCtrl = new WToolTip(this);
		ToolTipCtrl->OpenWindow();
		for( int tooltip = 0 ; ToolTips_BM[tooltip].ID > 0 ; tooltip++ )
		{
			// Figure out the rectangle for the toolbar button.
			int index = SendMessageW( hWndToolBar, TB_COMMANDTOINDEX, ToolTips_BM[tooltip].ID, 0 );
			RECT rect;
			SendMessageW( hWndToolBar, TB_GETITEMRECT, index, (LPARAM)&rect);

			ToolTipCtrl->AddTool( hWndToolBar, ToolTips_BM[tooltip].ToolTip, tooltip, &rect );
		}

		mrulist = new MRUList( *PersistentName );
		mrulist->ReadINI();

		RefreshAll();
		PositionChildControls();

		SetRedraw(true);

		unguard;
	}
	void OnDestroy()
	{
		guard(WBrowserMusic::OnDestroy);

		delete pComboPackage;
		delete pComboGroup;
		delete pListMusic;
		delete pCheckGroupAll;

		WBrowser::OnDestroy();
		unguard;
	}
	void OnCommand( INT Command )
	{
		guard(WBrowserMusic::OnCommand);
		switch( Command )
		{

			case ID_EditDelete:
			{
				UMusic* M = GetMusic();
				if (!M)
					break;
				FStringOutputDevice GetPropResult = FStringOutputDevice();
			    GEditor->Get( TEXT("Obj"), *FString::Printf(TEXT("DELETE CLASS=MUSIC OBJECT=\"%ls\""), *FObjectPathName(M)), GetPropResult);

				if( !GetPropResult.Len() )
					RefreshMusicList();
				else
					appMsgf( TEXT("Can't delete music - Error: %ls"), *GetPropResult );
			}
			break;

			case ID_EditRename:
			{
				UMusic* M = GetMusic();
				if (!M)
					break;
				FString Name = *FObjectName(M);
				FString CurrentPackage = pComboPackage->GetString(pComboPackage->GetCurrent());
				WDlgRename dlg( NULL, this );
				if( dlg.DoModal( Name ) )
				{
					M->Rename(*dlg.NewName);
				}
				GBrowserMaster->RefreshAll();
				pComboPackage->SetCurrent(pComboPackage->FindStringExact(*CurrentPackage));
				RefreshGroups();
				RefreshMusicList();
			}
			break;

			case IDMN_MB_EXPORT:
			{
				FString File;
				UMusic* M = GetMusic();
				if (!M)
					break;
				if (GetSaveNameWithDialog(
					*FObjectName(M),
					*GLastDir[eLASTDIR_MUS],
					TEXT("Music Files (*.s3m, *.ogg)\0*.s3m;*.ogg\0All Files\0*.*\0\0"),
					TEXT("*.s3m;*.ogg"),
					TEXT("Export Music"),
					File
				))
				{
					INT Exported = GEditor->Exec(*FString::Printf(
						TEXT("OBJ EXPORT TYPE=MUSIC NAME=\"%ls\" FILE=\"%ls\""),
						*FObjectPathName(M), *File));
					appMsgf(TEXT("Successfully exported %d of 1 music files"), Exported);
				}

				GFileManager->SetDefaultDirectory(appBaseDir());
			}
			break;

			case IDMN_SB_EXPORT_MUS_ALL:
			{
				FString CurrentPackage = pComboPackage->GetString(pComboPackage->GetCurrent());
				FString OutDir = FString::Printf(TEXT("..%ls%ls%lsMusic"), PATH_SEPARATOR, *CurrentPackage, PATH_SEPARATOR);
				GFileManager->MakeDirectory(*OutDir, 1);
				GFileManager->SetDefaultDirectory(*OutDir);

				INT Exported = 0, Expected = 0;
				UPackage* Pck = FindObject<UPackage>(NULL, *CurrentPackage);
				for (TObjectIterator<UMusic> It; It; ++It)
				{
					UMusic* A = *It;
					if (A->IsIn(Pck))
					{
						UPackage* G = FindOuter<UPackage>(A);
						const TCHAR* GroupDir = *FObjectName(A->GetOuter());
						FString OutName, Name = FObjectName(A);
						A->Data.Load();
						bool IsOgg = IsOggFormat(&A->Data(0), A->Data.Num());
						const TCHAR* Ext = IsOgg ? TEXT(".ogg") : TEXT(".s3m");
						if (G != Pck)
						{
							GFileManager->MakeDirectory(GroupDir, 0);
							OutName = GroupDir;
							OutName += PATH_SEPARATOR;
						}
						OutName += *Name;
						OutName += '.';
						OutName += Ext;
						A->Data.Unload();

						debugf(TEXT("Exporting: %ls"), *OutName);
						Exported += GEditor->Exec(*FString::Printf(TEXT("OBJ EXPORT TYPE=MUSIC NAME=\"%ls\" FILE=\"%ls\""), *FObjectPathName(A), *OutName));
						Expected++;
					}
				}
				GFileManager->SetDefaultDirectory(appBaseDir());

				if (Expected > 0)
					appMsgf(TEXT("Successfully exported %d of %d sounds"), Exported, Expected);
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

				FString Package = Filename.Right(Filename.Len() - (Filename.InStr(TEXT("\\"), 1) + 1));
				Package = Package.Left(Package.InStr(TEXT(".")));

				GBrowserMaster->RefreshAll();
				pComboPackage->SetCurrent(pComboPackage->FindStringExact(*Package));
				RefreshGroups();
				RefreshMusicList();
			}
			break;

			case IDMN_MB_IMPORT:
			{
				TArray<FString> Files;
				if (OpenFilesWithDialog(
					*GLastDir[eLASTDIR_MUS],
					TEXT("Music Files (*.mod, *.s3m, *.it, *.xm, *.669, *.ogg)\0*.mod;*.s3m;*.it;*.xm;*.669;*.ogg\0Amiga Modules (*.mod)\0*.mod\0Scream Tracker 3 (*.s3m)\0*.s3m\0Impulse Tracker (*.it)\0*.it\0Fasttracker 2\0*.xm\0ComposD (*.669)\0*.669\0Ogg Vorbis (*.ogg)\0*.ogg\0All Files\0*.*\0\0"),
					TEXT("*.mod;*.s3m;*.it;*.xm;*.669;*.ogg"),
					TEXT("Import Music"),
					Files
				))
				{
					FString Package = pComboPackage->GetString(pComboPackage->GetCurrent());
					FString Group = pComboGroup->GetString(pComboGroup->GetCurrent());

					WDlgImportMusic l_dlg(NULL, this);
					l_dlg.DoModal(Package, Group, &Files);

					GBrowserMaster->RefreshAll();
					INT Pos = pComboPackage->FindStringExact(*l_dlg.Package);
					if (Pos >= 0)
						pComboPackage->SetCurrent(Pos);
					RefreshGroups();
					if (Pos >= 0)
					{
						Pos = pComboGroup->FindStringExact(*l_dlg.Group);
						if (Pos >= 0)
							pComboGroup->SetCurrent(Pos);
					}
					RefreshMusicList();
					if (Files.Num() > 0)
						GLastDir[eLASTDIR_MUS] = appFilePathName(*Files(0));
				}

				GFileManager->SetDefaultDirectory(appBaseDir());
			}
			break;

			case IDPB_EDIT:
			case ID_PlayPauseTrack:
			case IDMN_MB_PLAY:
				OnPlay();
				break;

			case ID_StopTrack:
			case IDMN_MB_STOP:
				OnStop();
				break;

			case IDMN_LOAD_ENTIRE:
			{
				FString Package = pComboPackage->GetString(pComboPackage->GetCurrent());
				UObject::LoadPackage(NULL, *Package, LOAD_NoWarn);
				GBrowserMaster->RefreshAll();
			}
			break;

			case ID_FileSave:
			{
				FString File;
				FString Name = pComboPackage->GetString(pComboPackage->GetCurrent());
				if (GetSaveNameWithDialog(
					*Name,
					*GLastDir[eLASTDIR_UMX],
					TEXT("Music Packages (*.umx)\0*.umx\0All Files\0*.*\0\0"),
					TEXT("umx"),
					TEXT("Save Music Package"),
					File
				) && AllowOverwrite(*File))
				{
					CheckSavePackage(!GEditor->Exec(*FString::Printf(
						TEXT("OBJ SAVEPACKAGE PACKAGE=\"%ls\" FILE=\"%ls\""),
						*Name, *File)), Name);
				
					mrulist->AddItem( *File );
					mrulist->WriteINI();
					GConfig->Flush(0);
					UpdateMenu();
					GLastDir[eLASTDIR_UMX] = appFilePathName(*File);
				}

				GFileManager->SetDefaultDirectory(appBaseDir());
			}
			break;

			case ID_FileOpen:
			{
				TArray<FString> Files;
				if (OpenFilesWithDialog(
					*GLastDir[eLASTDIR_UMX],
					TEXT("Music Packages (*.umx)\0*.umx\0All Files\0*.*\0\0"),
					TEXT("umx"),
					TEXT("Open Music Package"),
					Files
				))
				{
					if (Files.Num() > 0)
					{
						GLastDir[eLASTDIR_UMX] = appFilePathName(*Files(0));
						SavePkgName = appFileBaseName(*(Files(0)));
						SavePkgName = SavePkgName.Left(SavePkgName.InStr(TEXT(".")));
					}

					GWarn->BeginSlowTask(TEXT(""), 1, 0);

					for( int x = 0 ; x < Files.Num() ; x++ )
					{
						GWarn->StatusUpdatef( x, Files.Num(), TEXT("Loading %ls"), *(Files(x)) );
						GEditor->Exec(*FString::Printf(TEXT("OBJ LOAD FILE=\"%ls\""), *(Files(x))));

						mrulist->AddItem( *(Files(x)) );
						mrulist->WriteINI();
						GConfig->Flush(0);
						UpdateMenu();
					}

					GWarn->EndSlowTask();

					GBrowserMaster->RefreshAll();

					pComboPackage->SetCurrent(pComboPackage->FindStringExact(*SavePkgName));
					RefreshGroups();
					RefreshMusicList();
				}

				GFileManager->SetDefaultDirectory(appBaseDir());
			}
			break;

			default:
				WBrowser::OnCommand(Command);
				break;
		}
		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WBrowserMusic::OnSize);
		WBrowser::OnSize(Flags, NewX, NewY);
		PositionChildControls();
		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	virtual void RefreshAll()
	{
		guard(WBrowserMusic::RefreshAll);
		SetRedraw(FALSE);
		RefreshPackages();
		RefreshGroups();
		RefreshMusicList();
		UpdateMenu();
		SetRedraw(TRUE);
		unguard;
	}
	void RefreshPackages(void)
	{
		guard(WBrowserMusic::RefreshPackages);
		WBrowser::RefreshPackages(pComboPackage, TEXT("Music"));
		unguard;
	}
	void RefreshGroups(void)
	{
		guard(WBrowserMusic::RefreshGroups);
		WBrowser::RefreshGroups<UMusic>(pComboPackage, pComboGroup);
		unguard;
	}
	inline FString GetDisplayName(UMusic* M)
	{
		if (!M->Data.Num())
			M->Data.Load();
		return FString::Printf(TEXT("%ls [%ls - %.1f kb]"), *FObjectName(M), *M->FileType, (M->Data.Num() / 1024.f));
	}
	// Called in WBrowser::RefreshItemList
	inline void ListItem(UMusic* M)
	{
		MusicList.AddItem(M);
		pListMusic->AddString(*GetDisplayName(M));
	}
	void RefreshMusicList(void)
	{
		guard(WBrowserMusic::RefreshMusicList);
		WBrowser::RefreshItemList<UMusic, WBrowserMusic>(pComboPackage, pComboGroup, pCheckGroupAll, pListMusic, MusicList);
		unguard;
	}
	// Moves the child windows around so that they best match the window size.
	//
	void PositionChildControls( void )
	{
		guard(WBrowserMusic::PositionChildControls);

		if (!pComboPackage
			|| !pComboGroup
			|| !pListMusic
			|| !pCheckGroupAll) return;

		FRect CR = GetClientRect();
		::MoveWindow(hWndToolBar, 4, 0, 7 * MulDiv(16, DPIX, 96), 7 * MulDiv(16, DPIY, 96), 1);
		RECT R;
		::GetClientRect(hWndToolBar, &R);
		::MoveWindow(GetDlgItem(hWnd, ID_BM_TOOLBAR), 0, 0, 2000, R.bottom, TRUE);

		float Top = (R.bottom - R.top) + 8;

		::MoveWindow(pComboPackage->hWnd, 4, Top, CR.Width() - 8, 20, 1);
		Top += MulDiv(22, DPIY, 96);
		::MoveWindow(pCheckGroupAll->hWnd, 4, Top, MulDiv(75, DPIY, 96), MulDiv(20, DPIY, 96), 1);
		::MoveWindow(pComboGroup->hWnd, 8 + MulDiv(75, DPIY, 96), Top, CR.Width() - 12 - MulDiv(75, DPIY, 96), 20, 1);
		Top += MulDiv(26, DPIY, 96);
		::MoveWindow(pListMusic->hWnd, 4, Top, CR.Width() - 8, CR.Height() - Top - 4, 1);

		unguard;
	}
	void OnPlay()
	{
		guard(WBrowserMusic::OnPlay);
		UMusic* M = GetMusic();
		if (!M)
			return;		
		GEditor->Exec(*FString::Printf(TEXT("MUSIC PLAY NAME=\"%ls\""), *FObjectPathName(M)));
		unguard;
	}
	void OnStop()
	{
		guard(WBrowserMusic::OnStop);
		GEditor->Exec( TEXT("MUSIC PLAY NAME=None") );
		unguard;
	}

	// Notification delegates for child controls.
	//
	void OnComboPackageSelChange()
	{
		guard(WBrowserMusic::OnComboPackageSelChange);
		RefreshGroups();
		RefreshMusicList();
		unguard;
	}
	void OnComboGroupSelChange()
	{
		guard(WBrowserMusic::OnComboGroupSelChange);
		RefreshMusicList();
		unguard;
	}
	void OnListMusicDblClick()
	{
		guard(WBrowserMusic::OnListMusicDblClick);
		OnPlay();
		unguard;
	}
	void OnGroupAllClick()
	{
		guard(WBrowserMusic::OnGroupAllClick);
		EnableWindow(pComboGroup->hWnd, !pCheckGroupAll->IsChecked());
		RefreshMusicList();
		unguard;
	}
	inline UMusic* GetMusic()
	{
		FString Name = pListMusic->GetString(pListMusic->GetCurrent());
		Name = Name.Left(Name.InStr(TEXT(" ["), TRUE));
		
		UMusic* Music = NULL;
		for (INT i = 0; i < MusicList.Num(); ++i)
		{
			Music = MusicList(i);
			if (Name == *FObjectName(Music))
				break;
		}
		return Music;
	}
	bool IsOggFormat(BYTE* Data, int DataLen)
	{
		const BYTE OGGHeaderID[] = { 0x4F,0x67,0x67,0x53,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
		if (DataLen < sizeof(OGGHeaderID))
			return 0;
		for (INT i = 0; i < ARRAY_COUNT(OGGHeaderID); ++i)
			if (OGGHeaderID[i] != Data[i])
				return 0;
		return 1;
	}
	virtual FString GetCurrentPathName(void)
	{
		guard(WBrowserMusic::GetCurrentPathName);
		return FObjectPathName(GetMusic());
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
