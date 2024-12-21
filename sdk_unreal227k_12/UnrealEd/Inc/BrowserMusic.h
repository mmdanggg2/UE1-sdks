/*=============================================================================
	BrowserMusic : Browser window for music files
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>
extern FString GLastDir[eLASTDIR_MAX];

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
	WEdit PackageEdit;
	WEdit GroupEdit;
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
		, FilenameStatic	( this, IDSC_FILENAME )
		, PackageEdit		( this, IDEC_PACKAGE )
		, GroupEdit		( this, IDEC_GROUP )
		, NameEdit		( this, IDEC_NAME )
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

		PackageEdit.SetText( *defPackage );
		GroupEdit.SetText( *defGroup );
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
	virtual int DoModal( FString _defPackage, FString _defGroup, TArray<FString>* _paFilenames)
	{
		guard(WDlgImportMusic::DoModal);

		defPackage = _defPackage;
		defGroup = _defGroup;
		paFilenames = _paFilenames;

		return WDialog::DoModal( hInstance );
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

		FString Name = ((*paFilenames)(iCurrentFilename)).GetFilenameOnly();
		NameEdit.SetText( *Name );
		unguard;
	}
	BOOL GetDataFromUser( void )
	{
		guard(WDlgImportMusic::GetDataFromUser);
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
		guard(WDlgImportMusic::SetNextFilename);
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
		guard(WDlgImportMusic::ImportFile);
		TCHAR l_chCmd[512];

		//appMsgf(TEXT("%ls %ls %ls"),*(*paFilenames)(iCurrentFilename), *Name, *Package);

		if( Group.Len() )
			appSprintf( l_chCmd, TEXT("MUSIC IMPORT TYPE=MUSIC FILE=\"%ls\" NAME=\"%ls\" PACKAGE=\"%ls\" GROUP=\"%ls\""),
				*(*paFilenames)(iCurrentFilename), *Name, *Package, *Group );
		else
			appSprintf( l_chCmd, TEXT("MUSIC IMPORT TYPE=MUSIC FILE=\"%ls\" NAME=\"%ls\" PACKAGE=\"%ls\""),
				*(*paFilenames)(iCurrentFilename), *Name, *Package );
		GEditor->Exec( l_chCmd );
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
	, { 1, IDMN_MB_FileOpen, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 2, IDMN_MB_FileSave, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 3, IDMN_MB_IN_LOAD_ENTIRE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 4, IDMN_MB_PLAY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 5, IDMN_MB_STOP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
};
struct {
	TCHAR ToolTip[64];
	int ID;
} ToolTips_BM[] = {
	TEXT("Toggle Dock Status"), IDMN_MB_DOCK,
	TEXT("Open Package"), IDMN_MB_FileOpen,
	TEXT("Save Package"), IDMN_MB_FileSave,
	TEXT("Load Entire Package"), IDMN_MB_IN_LOAD_ENTIRE,
	TEXT("Play"), IDMN_MB_PLAY,
	TEXT("Stop"), IDMN_MB_STOP,
	NULL, 0
};

inline INT Compare(const UMusic* A, const UMusic* B)
{
	return appStricmp(A->GetName(), B->GetName());
}

class WBrowserMusic : public WBrowser
{
	DECLARE_WINDOWCLASS(WBrowserMusic,WBrowser,Window)

	WTrackBar* pSectionTrack;
	WComboBox *pComboPackage, *pComboGroup;
	WListBox *pListMusic;
	TArray<UMusic*> MusicList;
	WCheckBox *pCheckGroupAll;
	HWND hWndToolBar{};
	WToolTip *ToolTipCtrl{};
	MRUList* mrulist;

	HBITMAP ToolbarImage{};

	// Structors.
	WBrowserMusic( FName InPersistentName, WWindow* InOwnerWindow, HWND InEditorFrame )
	:	WBrowser( InPersistentName, InOwnerWindow, InEditorFrame )
	,	pSectionTrack(nullptr), pComboPackage(nullptr), pComboGroup(nullptr), pListMusic(nullptr), pCheckGroupAll(nullptr), mrulist(nullptr)
	{
		MenuID = IDMENU_BrowserMusic;
		BrowserID = eBROWSER_MUSIC;
		Description = TEXT("Music");
	}

	// WBrowser interface.
	void OpenWindow( UBOOL bChild )
	{
		guard(WBrowserMusic::OpenWindow);
		WBrowser::OpenWindow( bChild );
		SetCaption();
		unguard;
	}
	void OnCreate()
	{
		guard(WBrowserMusic::OnCreate);
		WBrowser::OnCreate();

		SetMenu( hWnd, LoadMenuW(hInstance, MAKEINTRESOURCEW(IDMENU_BrowserMusic)) );

		// Song section slider.
		pSectionTrack = new WTrackBar(this);
		pSectionTrack->OpenWindow(1);
		pSectionTrack->SetTicFreq(1);
		pSectionTrack->SetRange(0, 8);

		// PACKAGES
		//
		pComboPackage = new WComboBox( this, IDCB_PACKAGE );
		pComboPackage->OpenWindow( 1, 1 );
		pComboPackage->SelectionChangeDelegate = FDelegate(this, (TDelegate)&WBrowserMusic::OnComboPackageSelChange);

		// GROUP
		//
		pComboGroup = new WComboBox( this, IDCB_GROUP );
		pComboGroup->OpenWindow( 1, 1 );
		pComboGroup->SelectionChangeDelegate = FDelegate(this, (TDelegate)&WBrowserMusic::OnComboGroupSelChange);

		// MUSIC LIST
		//
		pListMusic = new WListBox( this, IDLB_MUSIC );
		pListMusic->OpenWindow( 1, 0, 0, 0 );
		pListMusic->DoubleClickDelegate = FDelegate(this, (TDelegate)&WBrowserMusic::OnListMusicDblClick);
		pListMusic->SelectionChangeDelegate = FDelegate(this, (TDelegate)&WBrowserMusic::OnListMusicClick);

		ToolbarImage = reinterpret_cast<HBITMAP>(LoadImage(hInstance,
			MAKEINTRESOURCE(IDB_BrowserMusic_TOOLBAR),
			IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS));
		check(ToolbarImage);
		ScaleImageAndReplace(ToolbarImage);


		// CHECK BOXES
		//
		pCheckGroupAll = new WCheckBox( this, IDCK_GRP_ALL, FDelegate(this, (TDelegate)&WBrowserMusic::OnGroupAllClick) );
		pCheckGroupAll->OpenWindow( 1, 0, 0, 1, 1, TEXT("All"), 1, 0, BS_PUSHLIKE );

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
		for (int tooltip = 0; ToolTips_BM[tooltip].ID > 0; tooltip++)
		{
			// Figure out the rectangle for the toolbar button.
			int index = SendMessageW( hWndToolBar, TB_COMMANDTOINDEX, ToolTips_BM[tooltip].ID, 0 );
			RECT rect;
			SendMessageW( hWndToolBar, TB_GETITEMRECT, index, (LPARAM)&rect);

			ToolTipCtrl->AddTool( hWndToolBar, ToolTips_BM[tooltip].ToolTip, tooltip, &rect );
		}

		mrulist = new MRUList( *PersistentName );
		mrulist->ReadINI();
		if( GBrowserMaster->GetCurrent()==BrowserID )
			mrulist->AddToMenu( hWnd, GetMenu( IsDocked() ? OwnerWindow->hWnd : hWnd ) );

		RefreshAll();
		PositionChildControls();

		unguard;
	}
	void OnDestroy()
	{
		guard(WBrowserMusic::OnDestroy);

		mrulist->WriteINI();
		delete mrulist;

		delete pComboPackage;
		delete pComboGroup;
		delete pListMusic;
		delete pCheckGroupAll;

		if (ToolbarImage)
			DeleteObject(ToolbarImage);

		::DestroyWindow( hWndToolBar );
		delete ToolTipCtrl;

		WBrowser::OnDestroy();
		unguard;
	}
	virtual void UpdateMenu()
	{
		guard(WBrowserMusic::UpdateMenu);

		HMENU menu = IsDocked() ? GetMenu( OwnerWindow->hWnd ) : GetMenu( hWnd );
		CheckMenuItem( menu, IDMN_MB_DOCK, MF_BYCOMMAND | (IsDocked() ? MF_CHECKED : MF_UNCHECKED) );

		if( mrulist
				&& GBrowserMaster->GetCurrent()==BrowserID )
			mrulist->AddToMenu( hWnd, GetMenu( IsDocked() ? OwnerWindow->hWnd : hWnd ) );

		unguard;
	}
	void OnCommand( INT Command )
	{
		guard(WBrowserMusic::OnCommand);
		switch( Command ) {

			case IDMN_MB_DELETE:
				{
					UMusic* Song = GetSelectedSong();
					if (Song)
					{
						FStringOutputDevice GetResult;
						TCHAR l_chCmd[256];

						appSprintf(l_chCmd, TEXT("DELETE CLASS=MUSIC OBJECT=\"%ls\""), Song->GetPathName());
						GEditor->Get(TEXT("Obj"), l_chCmd, GetResult);

						if (!GetResult.Len())
							RefreshMusicList();
						else
							appMsgf(TEXT("Can't delete music"));
					}
				}
				break;

			case IDMN_MB_RENAME:
				{
					UMusic* Song = GetSelectedSong();
					if (Song)
					{
						// Add rename here...
						WDlgRename dlg(Song, this);
						if (dlg.DoModal())
						{
							GBrowserMaster->RefreshAll();
							RefreshGroups();
							RefreshMusicList();
							SelectMusic(Song);
						}
					}
				}
				break;

			case IDMN_MB_EXPORT:
				{
					UMusic* Song = GetSelectedSong();
					if (Song)
					{
						OPENFILENAMEA ofn;
						char File[8192] = "\0";

						::sprintf_s(File, ARRAY_COUNT(File), "%ls.%ls", Song->GetName(), *Song->FileType);

						ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
						ofn.lStructSize = sizeof(OPENFILENAMEA);
						ofn.hwndOwner = hWnd;
						ofn.lpstrFile = File;
						ofn.nMaxFile = sizeof(File);
						ofn.lpstrFilter = "Music Files (*.mod, *.s3m, *.stm, *.it, *.xm, *.far, *.669, *.ogg)\0*.mod;*.s3m;*.stm;*.it;*.xm;*.far;*.669;*.ogg\0Amiga Modules (*,mod)\0*.mod\0Scream Tracker 3 (*.s3m)\0*.s3m\0Scream Tracker 2 (*.stm)\0*.stm\0Impulse Tracker (*.it)\0*.it\0Fasttracker 2\0*,xm\0Farandole (*.far)\0*.far\0ComposD (*.669)\0*.669\0OGG (*.ogg)\0*.ogg\0All Files\0*.*\0\0";
						ofn.lpstrDefExt = "*.mod;*.s3m;*.stm;*.it;*.xm;*.far;*.669;*.ogg";
						ofn.lpstrTitle = "Export Music (no conversion)";
						ofn.lpstrInitialDir = appToAnsi(*(GLastDir[eLASTDIR_MUS]));
						ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;

						// Display the Open dialog box.
						//
						if (GetSaveFileNameA(&ofn))
						{
							TCHAR l_chCmd[512];
							appSprintf(l_chCmd, TEXT("OBJ EXPORT TYPE=MUSIC PACKAGE=\"%ls\" NAME=\"%ls\" FILE=\"%ls\""),
								Song->TopOuter()->GetName(), Song->GetName(), appFromAnsi(File));
							GEditor->Exec(l_chCmd);

							FString S = appFromAnsi(File);
							GLastDir[eLASTDIR_MUS] = S.GetFilePath().LeftChop(1);
						}
						GFileManager->SetDefaultDirectory(appBaseDir());
					}
				}
				break;
				case IDMN_SB_EXPORT_MUS_ALL:
				{
					FString CurrentPackage = pComboPackage->GetString(pComboPackage->GetCurrent());
					UPackage* SongPackage = FindObject<UPackage>(NULL, *CurrentPackage, TRUE);
					if (!SongPackage)
						break;
					TCHAR TempStr[256];
					TCHAR TempName[256];

					// Make package directory.
					appStrcpy( TempStr, TEXT("..") PATH_SEPARATOR );
					appStrcat( TempStr, *CurrentPackage );
					GFileManager->MakeDirectory( TempStr, 0 );

					// Make package\Textures directory.
					appStrcat( TempStr, PATH_SEPARATOR TEXT("Music") );
					GFileManager->MakeDirectory( TempStr, 0 );
					GFileManager->SetDefaultDirectory(TempStr);

					for( TObjectIterator<UMusic> It; It; ++It )
					{
						if (It->IsIn(SongPackage))
						{
							UMusic* A = *It;
							appStrcpy(TempName, A->GetName());
							appSnprintf(TempName, 256, TEXT("%ls.%ls"), A->GetName(), *A->FileType);
							debugf(TEXT("Exporting: %ls"), TempName);
							TCHAR l_chCmd[512];
							appSprintf(l_chCmd, TEXT("OBJ EXPORT TYPE=MUSIC PACKAGE=\"%ls\" NAME=\"%ls\" FILE=\"%ls\""), *CurrentPackage, A->GetName(), TempName);
							GEditor->Exec( l_chCmd );
						}
					}
					GFileManager->SetDefaultDirectory(appBaseDir());
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

				FString Package = Filename.GetFilenameOnly();
				GBrowserMaster->RefreshAll();
				pComboPackage->SetCurrent( pComboPackage->FindStringExact( *Package ) );
				RefreshGroups();
				RefreshMusicList();
			}
			break;


			case IDMN_MB_IMPORT:
				{
					OPENFILENAMEA ofn;
					char File[8192] = "\0";

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(File);
					ofn.lpstrFilter = "Music Files (*.mod, *.s3m, *.stm, *.it, *.xm, *.far, *.669, *.ogg)\0*.mod;*.s3m;*.stm;*.it;*.xm;*.far;*.669;*.ogg\0Amiga Modules (*,mod)\0*.mod\0Scream Tracker 3 (*.s3m)\0*.s3m\0Scream Tracker 2 (*.stm)\0*.stm\0Impulse Tracker (*.it)\0*.it\0Fasttracker 2\0*,xm\0Farandole (*.far)\0*.far\0ComposD (*.669)\0*.669\0OGG (*.ogg)\0*.ogg\0All Files\0*.*\0\0";
					ofn.lpstrDefExt = "*.mod;*.s3m;*.stm;*.it;*.xm;*.far;*.669;*.ogg";
					ofn.lpstrTitle = "Import Music";
					ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_MUS]) );
					ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

					// Display the Open dialog box.
					//
					if( GetOpenFileNameA(&ofn) )
					{
						int iNULLs = FormatFilenames( File );
						FString Package = pComboPackage->GetString( pComboPackage->GetCurrent() );
						if (!Package.Len())
							Package = TEXT("MyMusic");

						FString Group = pComboGroup->GetString( pComboGroup->GetCurrent() );

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
							GLastDir[eLASTDIR_MUS] = StringArray(0).GetFilePath().LeftChop(1);
						else
							GLastDir[eLASTDIR_MUS] = StringArray(0);


						TArray<FString> FilenamesArray;

						for( int x = iStart ; x < StringArray.Num() ; x++ )
						{
							FString NewString;

							NewString = FString::Printf( TEXT("%ls%ls"), *Prefix, *(StringArray(x)) );
							new(FilenamesArray)FString( NewString );
						}
						WDlgImportMusic l_dlg( NULL, this );
						l_dlg.DoModal( Package, Group, &FilenamesArray );

						GBrowserMaster->RefreshAll();
						pComboPackage->SetCurrent( pComboPackage->FindStringExact( *l_dlg.Package) );
						RefreshGroups();
						pComboGroup->SetCurrent( pComboGroup->FindStringExact( *l_dlg.Group) );
						RefreshMusicList();
					}

					GFileManager->SetDefaultDirectory(appBaseDir());
				}
				break;

			case IDMN_MB_PLAY:
				OnPlay();
				break;

			case IDMN_MB_STOP:
				OnStop();
				break;

			case IDMN_MB_IN_LOAD_ENTIRE:
				{
					FString Package = pComboPackage->GetString( pComboPackage->GetCurrent() );
					UObject::LoadPackage(NULL,*Package,LOAD_NoWarn);
					RefreshGroups();
					RefreshMusicList();
				}
				break;

			case IDMN_MB_FileSave:
				{
					OPENFILENAMEA ofn;
					char File[8192] = "\0";
					FString Package = pComboPackage->GetString( pComboPackage->GetCurrent() );

					::sprintf_s( File,256, "%ls.umx", *Package );

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(char) * 8192;
					ofn.lpstrFilter = "Music Packages (*.umx)\0*.umx\0All Files\0*.*\0\0";
					ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_UMX]) );
					ofn.lpstrDefExt = "umx";
					ofn.lpstrTitle = "Save Music Package";
					ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;

					if( GetSaveFileNameA(&ofn) )
					{
						TCHAR l_chCmd[256];
						appSprintf( l_chCmd, TEXT("OBJ SAVEPACKAGE PACKAGE=\"%ls\" FILE=\"%ls\""),
							*Package, appFromAnsi( File ) );
						GEditor->Exec( l_chCmd );

						FString S = appFromAnsi( File );
						mrulist->AddItem( S );

						if( GBrowserMaster->GetCurrent()==BrowserID )
							mrulist->AddToMenu( hWnd, GetMenu( IsDocked() ? OwnerWindow->hWnd : hWnd ) );
						GLastDir[eLASTDIR_UMX] = S.GetFilePath().LeftChop(1);
					}

					GFileManager->SetDefaultDirectory(appBaseDir());
				}
				break;

			case IDMN_MB_FileOpen:
				{
					OPENFILENAMEA ofn;
					char File[8192] = "\0";

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(char) * 8192;
					ofn.lpstrFilter = "Music Packages (*.umx)\0*.umx\0All Files\0*.*\0\0";
					ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_UMX]) );
					ofn.lpstrDefExt = "umx";
					ofn.lpstrTitle = "Open Music Package";
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
							GLastDir[eLASTDIR_UMX] = StringArray(0).GetFilePath().LeftChop(1);
						else
							GLastDir[eLASTDIR_UMX] = StringArray(0);

						GWarn->BeginSlowTask( TEXT(""), 1, 0 );

						for( int x = iStart ; x < StringArray.Num() ; x++ )
						{
							GWarn->StatusUpdatef( x, StringArray.Num(), TEXT("Loading %ls"), *(StringArray(x)) );

							TCHAR l_chCmd[512];
							appSprintf( l_chCmd, TEXT("OBJ LOAD FILE=\"%ls%ls\""), *Prefix, *(StringArray(x)) );
							GEditor->Exec( l_chCmd );

							mrulist->AddItem( *(StringArray(x)) );
							if( GBrowserMaster->GetCurrent()==BrowserID )
								mrulist->AddToMenu( hWnd, GetMenu( IsDocked() ? OwnerWindow->hWnd : hWnd ) );
						}

						GWarn->EndSlowTask();

						GBrowserMaster->RefreshAll();
						pComboPackage->SetCurrent( pComboPackage->FindStringExact( *SavePkgName ) );
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
	virtual void RefreshAll()
	{
		guard(WBrowserMusic::RefreshAll);
		RefreshPackages();
		RefreshGroups();
		RefreshMusicList();
		if( GBrowserMaster->GetCurrent()==BrowserID )
			mrulist->AddToMenu( hWnd, GetMenu( IsDocked() ? OwnerWindow->hWnd : hWnd ) );
		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WBrowserMusic::OnSize);
		WBrowser::OnSize(Flags, NewX, NewY);
		PositionChildControls();
		InvalidateRect( hWnd, NULL, FALSE );
		UpdateMenu();
		unguard;
	}
	void RefreshPackages( void )
	{
		guard(WBrowserSound::RefreshPackages);

		// PACKAGES
		//
		pComboPackage->Empty();

		FStringOutputDevice GetResult;
		GEditor->Get( TEXT("OBJ"), TEXT("PACKAGES CLASS=MUSIC"), GetResult);

		TArray<FString> StringArray;
		ParseStringToArray( TEXT(","), *GetResult, &StringArray );

		for( int x = 0 ; x < StringArray.Num() ; x++ )
		{
			pComboPackage->AddString( *(StringArray(x)) );
		}

		pComboPackage->SetCurrent( 0 );
		unguard;
	}
	void RefreshGroups( void )
	{
		guard(WBrowserSound::RefreshGroups);

		FString Package = pComboPackage->GetString( pComboPackage->GetCurrent() );

		// GROUPS
		//
		pComboGroup->Empty();

		FStringOutputDevice GetResult;
		TCHAR l_ch[256];
		appSprintf( l_ch, TEXT("GROUPS CLASS=MUSIC PACKAGE=\"%ls\""), *Package );
		GEditor->Get( TEXT("OBJ"), l_ch, GetResult);

		TArray<FString> StringArray;
		ParseStringToArray( TEXT(","), *GetResult, &StringArray );

		for( int x = 0 ; x < StringArray.Num() ; x++ )
		{
			pComboGroup->AddString( *(StringArray(x)) );
		}

		pComboGroup->SetCurrent( 0 );

		unguard;
	}
	void RefreshMusicList( void )
	{
		guard(WBrowserMusic::RefreshMusicList);

		FString Package = pComboPackage->GetString( pComboPackage->GetCurrent() );
		FString Group = pComboGroup->GetString( pComboGroup->GetCurrent() );

		UPackage* PackageObj = FindObject<UPackage>(NULL, *Package, TRUE);
		UPackage* GroupObj = Group.Len() ? FindObject<UPackage>(PackageObj, *Group, TRUE) : nullptr;
		if (!GroupObj || pCheckGroupAll->IsChecked())
			GroupObj = PackageObj;

		// MUSIC
		//
		pListMusic->Empty();
		MusicList.Empty();

		if (PackageObj)
		{
			for (TObjectIterator<UMusic> It; It; ++It)
			{
				if (It->IsIn(GroupObj))
					MusicList.AddItem(*It);
			}

			if (MusicList.Num())
			{
				Sort(MusicList);
				TCHAR* InfoStr = appStaticString1024();
				for (INT i = 0; i < MusicList.Num(); ++i)
				{
					UMusic* M = MusicList(i);
					M->Data.Load();
					appSnprintf(InfoStr, 1024, TEXT("%ls [%ls - %i kb]"), M->GetName(), *M->FileType, (M->Data.Num() >> 10));
					pListMusic->AddString(InfoStr);
				}
			}
		}
		pListMusic->SetCurrent( 0, 1 );
		unguard;
	}
	// Moves the child windows around so that they best match the window size.
	//
	void PositionChildControls( void )
	{
		guard(WBrowserMusic::PositionChildControls);

		if( !pComboPackage
			|| !pComboGroup
			|| !pListMusic
			|| !pCheckGroupAll
			|| !pSectionTrack) return;

		FRect CR = GetClientRect();
		::MoveWindow(hWndToolBar, 4, 0, 7 * MulDiv(16, DPIX, 96), 7 * MulDiv(16, DPIY, 96), 1);
		RECT R;
		::GetClientRect( hWndToolBar, &R );
		::MoveWindow( GetDlgItem( hWnd, ID_BM_TOOLBAR ), 0, 0, 2000, R.bottom, TRUE );

		int Top = (R.bottom - R.top) + 8;
		::MoveWindow( pSectionTrack->hWnd, 4, Top, CR.Width() - 8, 18, 1);
		Top += 20;
		::MoveWindow( pComboPackage->hWnd, 4, Top, CR.Width() - 8, 20, 1 );
		Top += 22;
		::MoveWindow( pCheckGroupAll->hWnd, 4, Top, 32, 20, 1 );
		::MoveWindow( pComboGroup->hWnd, 4 + 32, Top, CR.Width() - 8 - 32, 20, 1 );
		Top += 26;
		::MoveWindow( pListMusic->hWnd, 4, Top, CR.Width() - 8, CR.Height() - Top - 4, 1 );

		unguard;
	}

	// Notification delegates for child controls.
	//
	void OnComboPackageSelChange()
	{
		guard(WBrowserSound::OnComboPackageSelChange);
		RefreshGroups();
		RefreshMusicList();
		unguard;
	}
	void OnComboGroupSelChange()
	{
		guard(WBrowserSound::OnComboGroupSelChange);
		RefreshMusicList();
		unguard;
	}
	void OnListMusicDblClick()
	{
		guard(WBrowserMusic::OnListMusicDblClick);
		OnPlay();
		unguard;
	}
	void OnListMusicClick()
	{
		guard(WBrowserMusic::OnListMusicClick);
		UEditorEngine::CurrentMusic = GetSelectedSong();
		unguard;
	}
	void OnPlay()
	{
		guard(WBrowserMusic::OnPlay);
		UMusic* Song = GetSelectedSong();
		if (Song)
		{
			TCHAR l_chCmd[256];
			appSprintf(l_chCmd, TEXT("MUSIC PLAY NAME=\"%ls\" SECTION=%i"), Song->GetPathName(), (INT)pSectionTrack->GetPos());
			GEditor->Exec(l_chCmd);
		}
		unguard;
	}
	void OnStop()
	{
		guard(WBrowserMusic::OnStop);
		GEditor->Exec( TEXT("MUSIC PLAY NAME=None") );
		unguard;
	}
	void OnGroupAllClick()
	{
		guard(WBrowserMusic::OnGroupAllClick);
		EnableWindow( pComboGroup->hWnd, !pCheckGroupAll->IsChecked() );
		RefreshMusicList();
		unguard;
	}
	virtual FString GetCurrentPathName( void )
	{
		guardSlow(WBrowserMusic::GetCurrentPathName);
		UMusic* Song = GetSelectedSong();
		if (Song)
			return FString(Song->GetPathName());
		return FString();
		unguardSlow;
	}

	inline UMusic* GetSelectedSong() const
	{
		return MusicList(pListMusic->GetCurrent());
	}
	inline void SelectMusic(UMusic* Song)
	{
		INT Index = MusicList.FindItemIndex(Song);
		if (Index >= 0)
			pListMusic->SetCurrent(Index, TRUE);
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
