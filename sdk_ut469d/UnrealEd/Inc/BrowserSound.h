/*=============================================================================
	BrowserSound : Browser window for sound effects
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>

extern FString GLastDir[eLASTDIR_MAX];

// --------------------------------------------------------------
//
// IMPORT SOUND Dialog
//
// --------------------------------------------------------------

class WDlgImportSound : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgImportSound,WDialog,UnrealEd)

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
	WDlgImportSound( UObject* InContext, WBrowser* InOwnerWindow )
		: WDialog(TEXT("Import Sound"), IDDIALOG_IMPORT_SOUND, InOwnerWindow)
		, OkButton(this, IDOK, FDelegate(this, (TDelegate)&WDlgImportSound::OnOk))
		, OkAllButton(this, IDPB_OKALL, FDelegate(this, (TDelegate)&WDlgImportSound::OnOkAll))
		, SkipButton(this, IDPB_SKIP, FDelegate(this, (TDelegate)&WDlgImportSound::OnSkip))
		, CancelButton(this, IDCANCEL, FDelegate(this, (TDelegate)&WDlgImportSound::EndDialogFalse))
		, FilenameStatic(this, IDSC_FILENAME)
		, PackageEdit(this, IDEC_PACKAGE)
		, GroupEdit(this, IDEC_GROUP)
		, NameEdit(this, IDEC_NAME)
		, paFilenames(NULL)
		, bOKToAll(FALSE)
		, iCurrentFilename(0)
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgImportSound::OnInitDialog);
		WDialog::OnInitDialog();

		PackageEdit.Init(*defPackage, &GroupEdit);
		GroupEdit.SetText( *defGroup );
		::SetFocus( NameEdit.hWnd );

		bOKToAll = FALSE;
		iCurrentFilename = -1;
		SetNextFilename();

		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgImportSound::OnDestroy);
		WDialog::OnDestroy();
		unguard;
	}
	virtual int DoModal( FString _defPackage, FString _defGroup, TArray<FString>* _paFilenames)
	{
		guard(WDlgImportSound::DoModal);

		defPackage = _defPackage;
		defGroup = _defGroup;
		paFilenames = _paFilenames;

		return WDialog::DoModal( hInstance );
		unguard;
	}
	void OnOk()
	{
		guard(WDlgImportSound::OnOk);
		if( GetDataFromUser() )
		{
			ImportFile( (*paFilenames)(iCurrentFilename) );
			SetNextFilename();
		}
		unguard;
	}
	void OnOkAll()
	{
		guard(WDlgImportSound::OnOkAll);
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
		guard(WDlgImportSound::OnSkip);
		if( GetDataFromUser() )
			SetNextFilename();
		unguard;
	}
	void ImportTexture( void )
	{
		guard(WDlgImportSound::ImportTexture);
		unguard;
	}
	void RefreshName( void )
	{
		guard(WDlgImportSound::RefreshName);
		FilenameStatic.SetText( *(*paFilenames)(iCurrentFilename) );

		FString Name = GetFilenameOnly( (*paFilenames)(iCurrentFilename) );
		NameEdit.SetText( *Name );
		unguard;
	}
	BOOL GetDataFromUser( void )
	{
		guard(WDlgImportSound::GetDataFromUser);
		Package = PackageEdit.GetText();
		Group = GroupEdit.GetText();
		Name = NameEdit.GetText();

		if( !Package.Len()
				|| !Name.Len() )
		{
			appMsgf( TEXT("Invalid input.") );
			return FALSE;
		}

		return TRUE;
		unguard;
	}
	void SetNextFilename( void )
	{
		guard(WDlgImportSound::SetNextFilename);
		iCurrentFilename++;
		if( iCurrentFilename == paFilenames->Num() ) {
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
		guard(WDlgImportSound::ImportFile);
		if (Group.Len())
			GEditor->Exec(*FString::Printf(TEXT("AUDIO IMPORT FILE=\"%ls\" NAME=\"%ls\" PACKAGE=\"%ls\" GROUP=\"%ls\""),
				*(*paFilenames)(iCurrentFilename), *Name, *Package, *Group));
		else
			GEditor->Exec(*FString::Printf(TEXT("AUDIO IMPORT FILE=\"%ls\" NAME=\"%ls\" PACKAGE=\"%ls\""),
				*(*paFilenames)(iCurrentFilename), *Name, *Package));
		unguard;
	}
};

// --------------------------------------------------------------
//
// WBROWSERSOUND
//
// --------------------------------------------------------------

#define ID_BS_TOOLBAR	29020
TBBUTTON tbBSButtons[] = {
	{ 0, IDMN_MB_DOCK, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 1, ID_FileOpen, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 2, ID_FileSave, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 3, IDMN_LOAD_ENTIRE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 4, IDMN_SB_PLAY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 5, IDMN_SB_STOP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
};
struct {
	TCHAR ToolTip[64];
	int ID;
} ToolTips_BS[] = {
	TEXT("Toggle Dock Status"), IDMN_MB_DOCK,
	TEXT("Open Package"), ID_FileOpen,
	TEXT("Save Package"), ID_FileSave,
	TEXT("Load Entire Package"), IDMN_LOAD_ENTIRE,
	TEXT("Play"), IDMN_SB_PLAY,
	TEXT("Stop"), IDMN_SB_STOP,
	NULL, 0
};

class WBrowserSound : public WBrowser
{
	DECLARE_WINDOWCLASS(WBrowserSound,WBrowser,Window)

	WComboBox *pComboPackage, *pComboGroup;
	WListBox *pListSounds;
	WCheckBox *pCheckGroupAll;
	HWND hWndToolBar;
	WToolTip *ToolTipCtrl;
	MRUList* mrulist;
	TArray<USound*> SndList;
	HBITMAP ToolbarImage{};

	// Structors.
	WBrowserSound( FName InPersistentName, WWindow* InOwnerWindow, HWND InEditorFrame )
		: WBrowser(InPersistentName, InOwnerWindow, InEditorFrame)
		, hWndToolBar(NULL)
		, ToolTipCtrl(NULL)
		, mrulist(NULL)
	{
		pComboPackage = pComboGroup = NULL;
		pListSounds = NULL;
		pCheckGroupAll = NULL;
		MenuID = IDMENU_BrowserSound;
		BrowserID = eBROWSER_SOUND;
		Description = TEXT("Sounds");
	}

	inline FString GetDisplayName(USound* S)
	{
		if (!S->Data.Num())
			S->Data.Load();
		return FString::Printf(TEXT("%ls [%ls - %.2f s - %.1f kb]"), *FObjectName(S), *S->FileType, S->GetDuration(), (S->Data.Num() / 1024.f));
	}

	// WBrowser interface.
	void OpenWindow( UBOOL bChild )
	{
		guard(WBrowserSound::OpenWindow);
		WBrowser::OpenWindow( bChild );
		SetCaption();
		unguard;
	}
	void OnCreate()
	{
		guard(WBrowserSound::OnCreate);
		WBrowser::OnCreate();

		SetRedraw(false);

		SetMenu( hWnd, AddHotkeys(LoadMenuW(hInstance, MAKEINTRESOURCEW(IDMENU_BrowserSound))) );

		// PACKAGES
		//
		pComboPackage = new WComboBox( this, IDCB_PACKAGE );
		pComboPackage->OpenWindow( 1, 1 );
		pComboPackage->SelectionChangeDelegate = FDelegate(this, (TDelegate)&WBrowserSound::OnComboPackageSelChange);

		// GROUP
		//
		pComboGroup = new WComboBox( this, IDCB_GROUP );
		pComboGroup->OpenWindow( 1, 1 );
		pComboGroup->SelectionChangeDelegate = FDelegate(this, (TDelegate)&WBrowserSound::OnComboGroupSelChange);

		// SOUND LIST
		//
		pListSounds = new WListBox( this, IDLB_SOUNDS );
		pListSounds->OpenWindow( 1, 0, 0, 0, 1 );
		pListSounds->DoubleClickDelegate = FDelegate(this, (TDelegate)&WBrowserSound::OnListSoundsDblClick);
		pListSounds->CommandDelegate = FDelegateInt(this, (TDelegateInt)&WBrowserSound::OnCommand);

		// CHECK BOXES
		//
		pCheckGroupAll = new WCheckBox( this, IDCK_GRP_ALL, FDelegate(this, (TDelegate)&WBrowserSound::OnGroupAllClick) );
		pCheckGroupAll->OpenWindow( 1, 0, 0, 1, 1, TEXT("All Groups"), 1, 0, BS_PUSHLIKE );

		ToolbarImage = reinterpret_cast<HBITMAP>(LoadImage(hInstance,
			MAKEINTRESOURCE(IDB_BrowserSound_TOOLBAR),
			IMAGE_BITMAP, 0, 0, 0));
		check(ToolbarImage);
		ScaleImageAndReplace(ToolbarImage);

		hWndToolBar = CreateToolbarEx( 
			hWnd, WS_CHILD | WS_VISIBLE | CCS_ADJUSTABLE,
			IDB_BrowserSound_TOOLBAR,
			5,
			NULL,
			reinterpret_cast<PTRINT>(ToolbarImage),
			(LPCTBBUTTON)&tbBSButtons,
			8,
			MulDiv(16, DPIX, 96), MulDiv(16, DPIY, 96),
			MulDiv(16, DPIX, 96), MulDiv(16, DPIY, 96),
			sizeof(TBBUTTON));
		check(hWndToolBar);

		ToolTipCtrl = new WToolTip(this);
		ToolTipCtrl->OpenWindow();
		for( int tooltip = 0 ; ToolTips_BS[tooltip].ID > 0 ; tooltip++ )
		{
			// Figure out the rectangle for the toolbar button.
			int index = SendMessageW( hWndToolBar, TB_COMMANDTOINDEX, ToolTips_BS[tooltip].ID, 0 );
			RECT rect;
			SendMessageW( hWndToolBar, TB_GETITEMRECT, index, (LPARAM)&rect);

			ToolTipCtrl->AddTool( hWndToolBar, ToolTips_BS[tooltip].ToolTip, tooltip, &rect );
		}

		mrulist = new MRUList( *PersistentName );
		mrulist->ReadINI();
		if( GBrowserMaster->GetCurrent()==BrowserID )
			mrulist->AddToMenu( hWnd, GetMenu( IsDocked() ? OwnerWindow->hWnd : hWnd ) );

		RefreshAll();
		PositionChildControls();

		SetRedraw(true);

		unguard;
	}
	void OnDestroy()
	{
		guard(WBrowserSound::OnDestroy);

		mrulist->WriteINI();
		delete mrulist;

		delete pComboPackage;
		delete pComboGroup;
		delete pListSounds;
		delete pCheckGroupAll;

		::DestroyWindow( hWndToolBar );
		delete ToolTipCtrl;

		if (ToolbarImage)
			DeleteObject(ToolbarImage);

		WBrowser::OnDestroy();
		unguard;
	}
	virtual void UpdateMenu()
	{
		guard(WBrowserSound::UpdateMenu);

		HMENU menu = IsDocked() ? GetMenu( OwnerWindow->hWnd ) : GetMenu( hWnd );
		CheckMenuItem( menu, IDMN_MB_DOCK, MF_BYCOMMAND | (IsDocked() ? MF_CHECKED : MF_UNCHECKED) );

		if( mrulist
				&& GBrowserMaster->GetCurrent()==BrowserID )
			mrulist->AddToMenu( hWnd, GetMenu( IsDocked() ? OwnerWindow->hWnd : hWnd ) );

		unguard;
	}
	inline USound* GetSound()
	{
		FString Name = pListSounds->GetString(pListSounds->GetCurrent());
		Name = Name.Left(Name.InStr(TEXT(" [")));

		USound* Snd = NULL;
		for (INT i = 0; i < SndList.Num(); ++i)
		{
			Snd = SndList(i);
			if (Name == *FObjectName(Snd))
				break;
		}
		return Snd;
	}
	inline void RunCompress(const TCHAR* Cmd)
	{
		USound* S = GetSound();
		if (S)
		{
			GEditor->Exec(*FString::Printf(Cmd, *FObjectPathName(S)));
			RefreshSoundList();
			pListSounds->SetCurrent(pListSounds->FindStringExact(*GetDisplayName(S)), 1);
		}
	}
	inline void BatchCompress(const TCHAR* Cmd)
	{
		for (INT i = 0; i < SndList.Num(); ++i)
			GEditor->Exec(*FString::Printf(Cmd, *FObjectPathName(SndList(i))));
		RefreshSoundList();
	}
	void OnCommand( INT Command )
	{
		guard(WBrowserSound::OnCommand);
		switch( Command )
		{
			case ID_EditDelete:
			case IDMN_SB_DELETE:
			{
				USound* S = GetSound();
				if (!S)
					break;
				FStringOutputDevice GetPropResult = FStringOutputDevice();
			    GEditor->Get( TEXT("Obj"), *FString::Printf(TEXT("DELETE CLASS=SOUND OBJECT=\"%ls\""), *FObjectPathName(S)), GetPropResult);

				if( !GetPropResult.Len() )
					RefreshSoundList();
				else
					appMsgf( TEXT("Can't delete sound - Error: %s"), *GetPropResult );
			}
			break;

#if ENGINE_VERSION==227
			case IDMN_SB_RENAME:
			{
				// Add rename here...
				USound* S = GetSound();
				if (S)
				{
					WDlgRename dlg(NULL, this);
					if (dlg.DoModal(*FObjectName(S)))
					{
						// ?
					}
					GBrowserMaster->RefreshAll();
					pComboPackage->SetCurrent(pComboPackage->FindStringExact(*FObjectName(FindOuter<UPackage>(S, 1))));
					RefreshGroups();
					RefreshSoundList();
				}
			}
			break;
#endif

			case IDMN_SB_EXPORT_WAV:
			{
				USound* Snd = GetSound();
				FString File;
				if (GetSaveNameWithDialog(
					*FString::Printf(TEXT("%ls.%ls"), *FObjectName(Snd), *FString(*Snd->FileType).Locs()),
					*GLastDir[eLASTDIR_WAV],
#if ENGINE_VERSION==227
					TEXT("Audio Files (*.wav, *.ogg)\0*.wav;*.ogg\0All Files\0*.*\0\0"),
					TEXT("*.wav;*.ogg"),
#else
					TEXT("WAV Files (*.wav)\0*.wav\0All Files\0*.*\0\0"),
					TEXT("wav"),
#endif
					TEXT("Export Sound"),
					File
				))
				{
					FString Package = pComboPackage->GetString( pComboPackage->GetCurrent() );
					INT Exported = GEditor->Exec(*FString::Printf(
						TEXT("OBJ EXPORT TYPE=SOUND PACKAGE=\"%ls\" NAME=\"%ls\" FILE=\"%ls\""),
						*Package, *FObjectName(Snd), *File));
					appMsgf(TEXT("Successfully exported %d of 1 sounds"), Exported);
					GLastDir[eLASTDIR_WAV] = appFilePathName(*File);
				}
				GFileManager->SetDefaultDirectory(appBaseDir());
			}
			break;

			case IDMN_SB_EXPORT_WAV_ALL:
			{
				FString CurrentPackage = pComboPackage->GetString(pComboPackage->GetCurrent());
				FString OutDir = FString::Printf(TEXT("..%ls%ls%lsSounds"), PATH_SEPARATOR, *CurrentPackage, PATH_SEPARATOR);
				GFileManager->MakeDirectory(*OutDir, 1);
				GFileManager->SetDefaultDirectory(*OutDir);

				INT Exported = 0, Expected = 0;
				UPackage* Pck = FindObject<UPackage>(NULL, *CurrentPackage);
				for (TObjectIterator<USound> It; It; ++It)
				{
					USound* A = *It;
					if (A->IsIn(Pck))
					{
						UPackage* G = FindOuter<UPackage>(A);
						const TCHAR* GroupDir = *FObjectName(A->GetOuter());
						FString OutName, Name = FObjectName(A);
						if (G != Pck)
						{
							GFileManager->MakeDirectory(GroupDir, 0);
							OutName = GroupDir;
							OutName += PATH_SEPARATOR;
						}
						OutName += *Name;
						OutName += '.';
						OutName += *FString(*A->FileType).Locs();

						debugf(TEXT("Exporting: %ls"), *OutName);
						Exported += GEditor->Exec(*FString::Printf(TEXT("OBJ EXPORT TYPE=SOUND PACKAGE=\"%ls\" NAME=\"%ls\" FILE=\"%ls\""), *CurrentPackage, *Name, *OutName));
						Expected ++;
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
				GEditor->Exec( *(FString::Printf(TEXT("OBJ LOAD FILE=\"%ls\""), *Filename )) );

				FString Package = appFileBaseName(*Filename);
				Package = Package.Left( Package.InStr( TEXT(".")) );

				GBrowserMaster->RefreshAll();
				pComboPackage->SetCurrent( pComboPackage->FindStringExact( *Package ) );
				RefreshGroups();
				RefreshSoundList();
			}
			break;

			case IDMN_SB_IMPORT_WAV:
			{
				TArray<FString> Files;
				if (OpenFilesWithDialog(
					*GLastDir[eLASTDIR_WAV],
#if ENGINE_VERSION==227
					TEXT("Audio Files (*.wav, *.ogg)\0*.wav;*.ogg\0All Files\0*.*\0\0"),
					TEXT("*.wav;*.ogg"),
#else
					TEXT("WAV Files (*.wav)\0*.wav\0All Files\0*.*\0\0"),
					TEXT("wav"),
#endif
					TEXT("Import Sounds"),
					Files
				))
				{
					FString Package = pComboPackage->GetString( pComboPackage->GetCurrent() );
					FString Group = pComboGroup->GetString( pComboGroup->GetCurrent() );
	
					if (Files.Num() > 0)
						GLastDir[eLASTDIR_WAV] = appFilePathName(*Files(0));

					WDlgImportSound l_dlg( NULL, this );
					l_dlg.DoModal( Package, Group, &Files );

					GBrowserMaster->RefreshAll();
					pComboPackage->SetCurrent( pComboPackage->FindStringExact( *l_dlg.Package) );
					RefreshGroups();
					pComboGroup->SetCurrent( pComboGroup->FindStringExact( *l_dlg.Group) );
					RefreshSoundList();
				}

				GFileManager->SetDefaultDirectory(appBaseDir());
			}
			break;

			case IDPB_EDIT:
			case ID_PlayPauseTrack:
			case IDMN_SB_PLAY:
				OnPlay();
				break;

			case ID_StopTrack:
			case IDMN_SB_STOP:
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
				FString Package = pComboPackage->GetString(pComboPackage->GetCurrent());
				if (GetSaveNameWithDialog(
					*FString::Printf(TEXT("%ls.uax"), *Package),
					*GLastDir[eLASTDIR_UAX],
					TEXT("Sound Packages (*.uax)\0*.uax\0All Files\0*.*\0\0"),
					TEXT("uax"),
					TEXT("Save Sound Package"),
					File			
				) && AllowOverwrite(*File))
				{
					GEditor->Exec(*FString::Printf(
						TEXT("OBJ SAVEPACKAGE PACKAGE=\"%ls\" FILE=\"%ls\""),
						*Package, *File));

					mrulist->AddItem( *File );
					mrulist->WriteINI();
					GConfig->Flush(0);
					if( GBrowserMaster->GetCurrent()==BrowserID )
						mrulist->AddToMenu( hWnd, GetMenu( IsDocked() ? OwnerWindow->hWnd : hWnd ) );
					GLastDir[eLASTDIR_UAX] = appFilePathName(*File);
				}

				GFileManager->SetDefaultDirectory(appBaseDir());
			}
			break;

			case ID_FileOpen:
			{
				TArray<FString> Files;
				if (OpenFilesWithDialog(
					*GLastDir[eLASTDIR_UAX],
					TEXT("Sound Packages (*.uax)\0*.uax\0All Files\0*.*\0\0"),
					TEXT("uax"),
					TEXT("Open Sound Package"),
					Files			
				))
				{
					if( Files.Num() > 0 )
					{
						GLastDir[eLASTDIR_UAX] = appFilePathName(*Files(0));
						SavePkgName = appFileBaseName(*(Files(0)));
						SavePkgName = SavePkgName.Left( SavePkgName.InStr(TEXT(".")) );
					}

					GWarn->BeginSlowTask( TEXT(""), 1, 0 );

					for( int x = 0 ; x < Files.Num() ; x++ )
					{
						GWarn->StatusUpdatef( x, Files.Num(), TEXT("Loading %ls"), *(Files(x)) );
						GEditor->Exec(*FString::Printf(TEXT("OBJ LOAD FILE=\"%ls\""), *Files(x)));

						mrulist->AddItem( *(Files(x)) );
						mrulist->WriteINI();
						GConfig->Flush(0);
						if( GBrowserMaster->GetCurrent()==BrowserID )
							mrulist->AddToMenu( hWnd, GetMenu( IsDocked() ? OwnerWindow->hWnd : hWnd ) );
					}

					GWarn->EndSlowTask();

					GBrowserMaster->RefreshAll();
					pComboPackage->SetCurrent( pComboPackage->FindStringExact( *SavePkgName ) );
					RefreshGroups();
					RefreshSoundList();
				}

				GFileManager->SetDefaultDirectory(appBaseDir());
			}
			break;

#if ENGINE_VERSION==227
			case ID_COMPRESSTOOGG_VLOW:
				RunCompress(TEXT("AUDIO COMPRESS NAME=\"%ls\" QUALITY=0"));
				break;
			case ID_COMPRESSTOOGG_LOW:
				RunCompress(TEXT("AUDIO COMPRESS NAME=\"%ls\" QUALITY=3"));
				break;
			case ID_COMPRESSTOOGG_MEDIUM:
				RunCompress(TEXT("AUDIO COMPRESS NAME=\"%ls\" QUALITY=7"));
				break;
			case ID_COMPRESSTOOGG_HIGH:
				RunCompress(TEXT("AUDIO COMPRESS NAME=\"%ls\" QUALITY=11"));
				break;
			case ID_DECOMPRESSTOWAV:
				RunCompress(TEXT("AUDIO DECOMPRESS NAME=\"%ls\""));
				break;

			case ID_COMPRESSALLTOOGG_VERYLOWQUALITY:
				BatchCompress(TEXT("AUDIO COMPRESS NAME=\"%ls\" QUALITY=0"));
				break;
			case ID_COMPRESSALLTOOGG_LOWQUALITY:
				BatchCompress(TEXT("AUDIO COMPRESS NAME=\"%ls\" QUALITY=3"));
				break;
			case ID_COMPRESSALLTOOGG_MEDIUMQUALITY:
				BatchCompress(TEXT("AUDIO COMPRESS NAME=\"%ls\" QUALITY=7"));
				break;
			case ID_COMPRESSALLTOOGG_HIGHQUALITY:
				BatchCompress(TEXT("AUDIO COMPRESS NAME=\"%ls\" QUALITY=11"));
				break;
#endif

			default:
				WBrowser::OnCommand(Command);
				break;
		}
		unguard;
	}
	virtual void RefreshAll()
	{
		guard(WBrowserSound::RefreshAll);
		RefreshPackages();
		RefreshGroups();
		RefreshSoundList();
		if( GBrowserMaster->GetCurrent()==BrowserID )
			mrulist->AddToMenu( hWnd, GetMenu( IsDocked() ? OwnerWindow->hWnd : hWnd ) );
		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WBrowserSound::OnSize);
		WBrowser::OnSize(Flags, NewX, NewY);
		PositionChildControls();
		InvalidateRect( hWnd, NULL, FALSE );
		UpdateMenu();
		unguard;
	}
	
	void RefreshPackages( void )
	{
		guard(WBrowserSound::RefreshPackages);

		SetRedraw(false);

		// PACKAGES
		//
		pComboPackage->Empty();

		FStringOutputDevice GetPropResult = FStringOutputDevice();
		GEditor->Get( TEXT("OBJ"), TEXT("PACKAGES CLASS=Sound"), GetPropResult );

		TArray<FString> StringArray;
		ParseStringToArray( TEXT(","), *GetPropResult, &StringArray );

		for( int x = 0 ; x < StringArray.Num() ; x++ )
			pComboPackage->AddString( *(StringArray(x)) );

		pComboPackage->SetCurrent( 0 );

		SetRedraw(true);
		unguard;
	}
	void RefreshGroups( void )
	{
		guard(WBrowserSound::RefreshGroups);

		FString Package = pComboPackage->GetString( pComboPackage->GetCurrent() );
		UPackage* Pck = FindObject<UPackage>(NULL, *Package);

		// GROUPS
		//
		pComboGroup->Empty();

		TArray<FString> StringArray;
		for (TObjectIterator<USound> It; It; ++It)
		{
			UObject* Outer = It->GetOuter();
			if (Outer == Pck)
				StringArray.AddUniqueItem(TEXT("None"));
			else if (Outer && Outer->GetOuter() == Pck)
				StringArray.AddUniqueItem(Outer->GetName());
		}

		for (int x = 0; x < StringArray.Num(); x++)
			pComboGroup->AddString(*(StringArray(x)));

		pComboGroup->SetCurrent(0);

		unguard;
	}
	inline void ListSound(USound* S)
	{
		SndList.AddItem(S);
		pListSounds->AddString(*GetDisplayName(S));
	}
	void RefreshSoundList( void )
	{
		guard(WBrowserSound::RefreshSoundList);

		FString Package = pComboPackage->GetString(pComboPackage->GetCurrent());
		FString Group = pComboGroup->GetString(pComboGroup->GetCurrent());
		FString Selected = pListSounds->GetString(pListSounds->GetCurrent());

		// SOUNDS
		//
		pListSounds->Empty();

		SndList.Empty();
		UPackage* Pck = FindObject<UPackage>(NULL, *Package);

		if (pCheckGroupAll->IsChecked())
		{
			for (TObjectIterator<USound> It; It; ++It)
			{
				if (It->IsIn(Pck))
					ListSound(*It);
			}
		}
		else
		{
			for (TObjectIterator<USound> It; It; ++It)
			{
				if (!It->IsIn(Pck))
					continue;
				UPackage* P = FindOuter<UPackage>(*It);
				if (P == Pck ? Group == TEXT("None") : Group == *FObjectName(P))
					ListSound(*It);
			}
		}

		INT Current = pListSounds->FindStringExact(*Selected);
		if (Current >= 0)
			pListSounds->SetCurrent(Current, 1);

		unguard;
	}
	// Moves the child windows around so that they best match the window size.
	//
	void PositionChildControls( void )
	{
		guard(WBrowserSound::PositionChildControls);

		if( !pComboPackage 
			|| !pComboGroup
			|| !pListSounds
			|| !pCheckGroupAll ) return;

		FRect CR = GetClientRect();
		::MoveWindow(hWndToolBar, 4, 0, 7 * MulDiv(16, DPIX, 96), 7 * MulDiv(16, DPIY, 96), 1);
		RECT R;
		::GetClientRect( hWndToolBar, &R );
		::MoveWindow( GetDlgItem( hWnd, ID_BS_TOOLBAR ), 0, 0, 2000, R.bottom, TRUE );

		float Top = (R.bottom - R.top) + 8;

		::MoveWindow( pComboPackage->hWnd, 4, Top, CR.Width() - 8, 20, 1 );
		Top += MulDiv(22, DPIY, 96);
		::MoveWindow( pCheckGroupAll->hWnd, 4, Top, MulDiv(75, DPIY, 96), MulDiv(20, DPIY, 96), 1 );
		::MoveWindow( pComboGroup->hWnd, 8 + MulDiv(75, DPIY, 96), Top, CR.Width() - 12 - MulDiv(75, DPIY, 96), 20, 1 );
		Top += MulDiv(26, DPIY, 96);
		::MoveWindow( pListSounds->hWnd, 4, Top, CR.Width() - 8, CR.Height() - Top - 4, 1 );

		unguard;
	}

	// Notification delegates for child controls.
	//
	void OnComboPackageSelChange()
	{
		guard(WBrowserSound::OnComboPackageSelChange);
		RefreshGroups();
		RefreshSoundList();
		unguard;
	}
	void OnComboGroupSelChange()
	{
		guard(WBrowserSound::OnComboGroupSelChange);
		RefreshSoundList();
		unguard;
	}
	void OnListSoundsDblClick()
	{
		guard(WBrowserSound::OnListSoundsDblClick);
		OnPlay();
		unguard;
	}
	void OnPlay()
	{
		guard(WBrowserSound::OnPlay);
		GEditor->Exec(*FString::Printf(TEXT("AUDIO PLAY NAME=\"%ls\""), *FObjectPathName(GetSound())));
		unguard;
	}
	void OnStop()
	{
		guard(WBrowserSound::OnStop);
		GEditor->Exec( TEXT("AUDIO PLAY NAME=None") );
		unguard;
	}
	void OnGroupAllClick()
	{
		guard(WBrowserSound::OnGroupAllClick);
		EnableWindow( pComboGroup->hWnd, !pCheckGroupAll->IsChecked() );
		RefreshSoundList();
		unguard;
	}
	virtual FString GetCurrentPathName( void )
	{
		guard(WBrowserSound::GetCurrentPathName);
		return FObjectPathName(GetSound());
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
