/*=============================================================================
	BrowserSound : Browser window for sound effects
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>
extern FString GLastDir[eLASTDIR_MAX];

void MENU_SoundContextMenu(HWND Wnd);

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
	WEdit PackageEdit;
	WEdit GroupEdit;
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
	void ImportSound( void )
	{
		guard(WDlgImportSound::ImportSound);
		unguard;
	}
	void RefreshName( void )
	{
		guard(WDlgImportSound::RefreshName);
		FilenameStatic.SetText( *(*paFilenames)(iCurrentFilename) );

		FString Name = ((*paFilenames)(iCurrentFilename)).GetFilenameOnly();
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
	, { 1, IDMN_SB_FileOpen, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 2, IDMN_SB_FileSave, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 3, IDMN_SB_IN_LOAD_ENTIRE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 4, IDMN_SB_PLAY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 5, IDMN_SB_STOP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
};
struct {
	TCHAR ToolTip[64];
	int ID;
} ToolTips_BS[] = {
	TEXT("Toggle Dock Status"), IDMN_MB_DOCK,
	TEXT("Open Package"), IDMN_SB_FileOpen,
	TEXT("Save Package"), IDMN_SB_FileSave,
	TEXT("Load Entire Package"), IDMN_SB_IN_LOAD_ENTIRE,
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
	HWND hWndToolBar{};
	WToolTip *ToolTipCtrl{};
	MRUList* mrulist;
	TArray<USound*> SndList;
	HBITMAP ToolbarImage{};

	// Structors.
	WBrowserSound( FName InPersistentName, WWindow* InOwnerWindow, HWND InEditorFrame )
	:	WBrowser( InPersistentName, InOwnerWindow, InEditorFrame )
	{
		pComboPackage = pComboGroup = NULL;
		pListSounds = NULL;
		pCheckGroupAll = NULL;
		MenuID = IDMENU_BrowserSound;
		BrowserID = eBROWSER_SOUND;
		Description = TEXT("Sounds");
		mrulist = NULL;
	}

	inline FString GetDisplayName(USound* S)
	{
		if(!S->Data.Num())
			S->Data.Load();
		return FString::Printf(TEXT("%ls [%ls - %.2f s - %.1f kb]"), S->GetName(), *S->FileType, S->GetDuration(), (S->Data.Num() / 1024.f));
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

		SetMenu( hWnd, LoadMenuW(hInstance, MAKEINTRESOURCEW(IDMENU_BrowserSound)) );

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
		pListSounds->SelectionChangeDelegate = FDelegate(this, (TDelegate)&WBrowserSound::OnListSoundsClick);
		pListSounds->RightClickDelegate = FDelegate(this, (TDelegate)&WBrowserSound::OnListSoundsRightClick);

		// CHECK BOXES
		//
		pCheckGroupAll = new WCheckBox( this, IDCK_GRP_ALL, FDelegate(this, (TDelegate)&WBrowserSound::OnGroupAllClick) );
		pCheckGroupAll->OpenWindow( 1, 0, 0, 1, 1, TEXT("All"), 1, 0, BS_PUSHLIKE );

		ToolbarImage = reinterpret_cast<HBITMAP>(LoadImage(hInstance,
			MAKEINTRESOURCE(IDB_BrowserSound_TOOLBAR),
			IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS));
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
			if (Name == Snd->GetName())
				break;
		}
		return Snd;
	}
	inline void RunCompress(const TCHAR* Cmd)
	{
		USound* S = GetSound();
		if (S)
		{
			GEditor->Exec(*FString::Printf(Cmd, S->GetPathName()));
			RefreshSoundList();
			pListSounds->SetCurrent(pListSounds->FindStringExact(*GetDisplayName(S)), 1);
		}
	}
	inline void BatchCompress(const TCHAR* Cmd)
	{
		for (INT i = 0; i < SndList.Num(); ++i)
			GEditor->Exec(*FString::Printf(Cmd, SndList(i)->GetPathName()));
		RefreshSoundList();
	}
	void OnCommand( INT Command )
	{
		guard(WBrowserSound::OnCommand);
		switch( Command ) {

			case IDMN_SB_DELETE:
				{
					USound* S = GetSound();
					if (S)
					{
						FStringOutputDevice GetResult;
						TCHAR l_chCmd[256];

						appSprintf(l_chCmd, TEXT("DELETE CLASS=SOUND OBJECT=\"%ls\""), S->GetPathName());
						GEditor->Get(TEXT("Obj"), l_chCmd, GetResult);

						if (!GetResult.Len())
							RefreshSoundList();
						else appMsgf(TEXT("Can't delete %ls:\n%ls"), S->GetFullName(), *GetResult);
					}
				}
				break;

			case IDMN_SB_RENAME:
				{
					// Add rename here...
					USound* S = GetSound();
					if (S)
					{
						WDlgRename dlg(S, this);
						if (dlg.DoModal())
						{
							GBrowserMaster->RefreshAll();
							pComboPackage->SetCurrent(pComboPackage->FindStringExact(FindOuter<UPackage>(S, 1)->GetName()));
							RefreshGroups();
							RefreshSoundList();
						}
					}
				}
				break;

			case IDMN_SB_EXPORT_WAV:
				{
					OPENFILENAMEA ofn;

					USound* Snd = GetSound();

					FString FileName = FString::Printf(TEXT("%ls.%ls"), Snd->GetName(), *FString(*Snd->FileType).Locs());

					char File[8192];
					::strcpy_s(File, appToAnsi(*FileName));

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(char) * 8192;
					ofn.lpstrFilter = "Audio Files (*.wav, *.ogg)\0*.wav;*.ogg\0All Files\0*.*\0\0";
					ofn.lpstrDefExt = "*.wav;*.ogg";
					ofn.lpstrTitle = "Export Sound";
					ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_WAV]) );
					ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;

					// Display the Open dialog box.
					//
					if( GetSaveFileNameA(&ofn) )
					{
						TCHAR l_chCmd[512];
						FString Package = pComboPackage->GetString( pComboPackage->GetCurrent() );

						appSprintf( l_chCmd, TEXT("OBJ EXPORT TYPE=SOUND PACKAGE=\"%ls\" NAME=\"%ls\" FILE=\"%ls\""),
							*Package, Snd->GetName(), appFromAnsi( File ) );
						GEditor->Exec( l_chCmd );

						FString S = appFromAnsi( File );
						GLastDir[eLASTDIR_WAV] = S.GetFilePath().LeftChop(1);
					}

					GFileManager->SetDefaultDirectory(appBaseDir());
				}
				break;
				case IDMN_SB_EXPORT_WAV_ALL:
				{
					FString CurrentPackage = pComboPackage->GetString(pComboPackage->GetCurrent());
					UPackage* Pck = FindObject<UPackage>(NULL, *CurrentPackage);
					TCHAR TempStr[256];

					// Make package directory.
					appStrcpy( TempStr, TEXT("..") PATH_SEPARATOR );
					appStrcat( TempStr, *CurrentPackage );
					GFileManager->MakeDirectory( TempStr, 0 );

					// Make package\Textures directory.
					appStrcat( TempStr, PATH_SEPARATOR TEXT("Sounds") );
					GFileManager->MakeDirectory( TempStr, 0 );
					GFileManager->SetDefaultDirectory(TempStr);

					for( TObjectIterator<USound> It; It; ++It )
					{
						USound* A = *It;
						if (A->IsIn(Pck))
						{
							UPackage* G = FindOuter<UPackage>(A);
							TCHAR TempName[512] = TEXT("");
							const TCHAR* Name = A->GetName();
							if (G!=Pck)
							{
								const TCHAR* GroupDir = G->GetName();
								GFileManager->MakeDirectory( GroupDir, 0 );
								appStrcat( TempName, GroupDir);
								appStrcat( TempName, PATH_SEPARATOR);
							}
							appStrcat(TempName, Name);
							appStrcat(TempName, TEXT("."));
							appStrcat(TempName, *FString(*A->FileType).Locs());
							debugf(TEXT("Exporting: %ls"), TempName);
							TCHAR l_chCmd[512];
							appSprintf(l_chCmd, TEXT("OBJ EXPORT TYPE=SOUND PACKAGE=\"%ls\" NAME=\"%ls\" FILE=\"%ls\""), *CurrentPackage, Name, TempName);
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
				RefreshSoundList();
			}
			break;

			case IDMN_SB_IMPORT_WAV:
				{
					OPENFILENAMEA ofn;
					char File[8192] = "\0";

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(char) * 8192;
					ofn.lpstrFilter = "Audio Files (*.wav, *.ogg)\0*.wav;*.ogg\0All Files\0*.*\0\0";
					ofn.lpstrDefExt = "*.wav;*.ogg";
					ofn.lpstrTitle = "Import Sounds";
					ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_WAV]) );
					ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

					// Display the Open dialog box.
					//
					if( GetOpenFileNameA(&ofn) )
					{
						int iNULLs = FormatFilenames( File );
						FString Package = pComboPackage->GetString( pComboPackage->GetCurrent() );
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
							GLastDir[eLASTDIR_WAV] = StringArray(0).GetFilePath().LeftChop(1);
						else
							GLastDir[eLASTDIR_WAV] = StringArray(0);

						TArray<FString> FilenamesArray;

						for( int x = iStart ; x < StringArray.Num() ; x++ )
						{
							FString NewString;

							NewString = FString::Printf( TEXT("%ls%ls"), *Prefix, *(StringArray(x)) );
							new(FilenamesArray)FString( NewString );
						}

						WDlgImportSound l_dlg( NULL, this );
						l_dlg.DoModal( Package, Group, &FilenamesArray );

						GBrowserMaster->RefreshAll();
						pComboPackage->SetCurrent( pComboPackage->FindStringExact( *l_dlg.Package) );
						RefreshGroups();
						pComboGroup->SetCurrent( pComboGroup->FindStringExact( *l_dlg.Group) );
						RefreshSoundList();
					}

					GFileManager->SetDefaultDirectory(appBaseDir());
				}
				break;

			case IDMN_SB_PLAY:
				OnPlay();
				break;

			case IDMN_SB_STOP:
				OnStop();
				break;

			case IDMN_SB_IN_LOAD_ENTIRE:
				{
					FString Package = pComboPackage->GetString( pComboPackage->GetCurrent() );
					UObject::LoadPackage(NULL,*Package,LOAD_NoWarn);
					RefreshGroups();
					RefreshSoundList();
				}
				break;

			case IDMN_SB_FileSave:
				{
					OPENFILENAMEA ofn;
					char File[8192] = "\0";
					FString Package = pComboPackage->GetString( pComboPackage->GetCurrent() );

					::sprintf_s( File, ARRAY_COUNT(File), "%ls.uax", *Package );

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(char) * 8192;
					ofn.lpstrFilter = "Sound Packages (*.uax)\0*.uax\0All Files\0*.*\0\0";
					ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_UAX]) );
					ofn.lpstrDefExt = "uax";
					ofn.lpstrTitle = "Save Sound Package";
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
						GLastDir[eLASTDIR_UAX] = S.GetFilePath().LeftChop(1);
					}

					GFileManager->SetDefaultDirectory(appBaseDir());
				}
				break;

			case IDMN_SB_FileOpen:
				{
					OPENFILENAMEA ofn;
					char File[8192] = "\0";

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(char) * 8192;
					ofn.lpstrFilter = "Sound Packages (*.uax)\0*.uax\0All Files\0*.*\0\0";
					ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_UAX]) );
					ofn.lpstrDefExt = "uax";
					ofn.lpstrTitle = "Open Sound Package";
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
							if( StringArray.Num() == 1 )
								SavePkgName = StringArray(0).GetFilenameOnly();
							else
								SavePkgName = StringArray(1).GetFilenameOnly();
						}

						if( StringArray.Num() == 1 )
							GLastDir[eLASTDIR_UAX] = StringArray(0).GetFilePath().LeftChop(1);
						else
							GLastDir[eLASTDIR_UAX] = StringArray(0);

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
						RefreshSoundList();
					}

					GFileManager->SetDefaultDirectory(appBaseDir());
				}
				break;

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

		// PACKAGES
		//
		pComboPackage->Empty();

		TSingleMap < UPackage*> List;
		for (TObjectIterator<USound> It; It; ++It)
		{
			UPackage* P = FindOuter<UPackage>(*It, 1);
			if (List.Set(P))
				pComboPackage->AddString(P->GetName());
		}

		pComboPackage->SetCurrent( 0 );
		unguard;
	}
	void RefreshGroups(void)
	{
		guard(WBrowserSound::RefreshGroups);

		FString Package = pComboPackage->GetString(pComboPackage->GetCurrent());

		// GROUPS
		//
		pComboGroup->Empty();

		UPackage* Pck = FindObject<UPackage>(NULL, *Package);

		TSingleMap < UPackage*> List;
		for (TObjectIterator<USound> It; It; ++It)
		{
			if (!It->IsIn(Pck))
				continue;
			UPackage* P = FindOuter<UPackage>(*It);
			if (List.Set(P))
				pComboGroup->AddString(P == Pck ? TEXT("None") : P->GetName());
		}

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

		FString Package = pComboPackage->GetString( pComboPackage->GetCurrent() );
		FString Group = pComboGroup->GetString( pComboGroup->GetCurrent() );

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
				if (P==Pck ? Group==TEXT("None") : Group == P->GetName())
					ListSound(*It);
			}
		}

		pListSounds->SetCurrent( 0, 1 );

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
		::MoveWindow( pCheckGroupAll->hWnd, 4, Top, MulDiv(32, DPIY, 96), MulDiv(20, DPIY, 96), 1 );
		::MoveWindow( pComboGroup->hWnd, 4 + MulDiv(32, DPIY, 96), Top, CR.Width() - 8 - MulDiv(32, DPIY, 96), 20, 1 );
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
	void OnListSoundsClick()
	{
		guard(WBrowserMusic::OnListSoundsClick);
		UEditorEngine::CurrentSound = GetSound();
		unguard;
	}
	void OnPlay()
	{
		guard(WBrowserSound::OnPlay);

		if (SndList.Num())
		{
			TCHAR l_chCmd[256];
			appSprintf(l_chCmd, TEXT("AUDIO PLAY NAME=\"%ls\""), GetSound()->GetPathName());
			GEditor->Exec(l_chCmd);
		}
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
	void OnListSoundsRightClick()
	{
		guard(WBrowserSound::OnListSoundsRightClick);
		MENU_SoundContextMenu(hWnd);
		unguard;
	}
	virtual FString GetCurrentPathName( void )
	{
		guard(WBrowserSound::GetCurrentPathName);
		return GetSound()->GetPathName();
		unguard;
	}
};
WBrowserSound* GBrowserSound = NULL;

void RC_SoundOpts(INT ID)
{
	if (ID == 4)
		appClipboardCopy(GBrowserSound->GetSound()->GetPathName());
	else
	{
		static INT Cmds[] = { IDMN_SB_PLAY , IDMN_SB_STOP, IDMN_SB_RENAME, IDMN_SB_EXPORT_WAV, 0, ID_COMPRESSTOOGG_VLOW, ID_COMPRESSTOOGG_LOW,
			ID_COMPRESSTOOGG_MEDIUM , ID_COMPRESSTOOGG_HIGH , ID_DECOMPRESSTOWAV , IDMN_SB_DELETE };
		GBrowserSound->OnCommand(Cmds[ID]);
	}
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
