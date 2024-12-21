/*=============================================================================
	StaticMesh / Brush Windows

	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Smirftsch
=============================================================================*/

#include <stdio.h>

__declspec(dllimport) INT GLastScroll;

extern void Query(ULevel* Level, const TCHAR* Item, FString* pOutput, UPackage* InPkg);
extern void ParseStringToArray(const TCHAR* pchDelim, FString String, TArray<FString>* _pArray);

UBOOL bOk;

class WDlgConvertToStaticMesh : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgConvertToStaticMesh,WDialog,UnrealEd)

	// Variables.
	WButton OkButton;
	WButton CancelButton;
	WEdit PackageEdit;
	WEdit GroupEdit;
	WEdit NameEdit;
	FString Package, Name, Group, defPackage;
	TCHAR l_chCmd[255];
	char buffer[10];
	INT Cuts;

	// Constructor.
	WDlgConvertToStaticMesh( UObject* InContext, WWindow* InOwnerWindow )
	:	WDialog			( TEXT("Convert To StaticMesh"), IDDIALOG_CONVERT_STATICMESH, InOwnerWindow )
	,	OkButton		( this, IDOK,			FDelegate(this,(TDelegate)&WDlgConvertToStaticMesh::OnOk) )
	,	CancelButton	( this, IDCANCEL,		FDelegate(this,(TDelegate)&WDialog::EndDialogFalse) )
	,	PackageEdit		( this, IDEC_PACKAGE )
	,	GroupEdit		( this, IDEC_GROUP )
	,	NameEdit		( this, IDEC_NAME )
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgConvertToStaticMesh::OnInitDialog);
		WDialog::OnInitDialog();
		NameEdit.SetText( TEXT("") );
		GroupEdit.SetText( TEXT("") );
		PackageEdit.SetText( *defPackage );
		::SetFocus( NameEdit.hWnd );
		bOk=0;

		Cuts = 20;
		_itoa(Cuts, buffer, sizeof(buffer));
		SendMessageW(::GetDlgItem(this->hWnd, IDSL_SMBALANCE), TBM_SETRANGE, 1, MAKELONG(1, 30));
		SendMessageW(::GetDlgItem(this->hWnd, IDSL_SMBALANCE), TBM_SETTICFREQ, 10, 0);
		SendMessageW(::GetDlgItem(this->hWnd, IDSL_SMBALANCE), TBM_SETPOS, 1, Cuts);
		SendMessageW(::GetDlgItem(this->hWnd, IDSC_SMBALANCE), WM_SETTEXT, 0, (LPARAM)(LPCTSTR)buffer);

		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgConvertToStaticMesh::OnDestroy);
		WDialog::OnDestroy();
		unguard;
	}
	void OnClose()
	{
		guard(WDlgConvertToStaticMesh::OnClose);
		WDialog::EndDialogFalse();
		unguard;
	}
	virtual int DoModal(FString _defPackage)
	{
		guard(WDlgConvertToStaticMesh::DoModal);
		defPackage = _defPackage;
		return WDialog::DoModal( hInstance );
		unguard;
	}
	void OnOk()
	{
		guard(WDlgConvertToStaticMesh::OnOk);
		if( GetDataFromUser() )
		{
			Cuts = SendMessageW(::GetDlgItem(this->hWnd, IDSL_SMBALANCE), TBM_GETPOS, 0, 0);
			appSprintf( l_chCmd, TEXT("ACTOR CONVERT STATICMESH NAME=%ls PACKAGE=%ls CUTS=%i"),*Name,*Package,Cuts );
			GEditor->Exec( l_chCmd );
			bOk=1;
			EndDialog(TRUE);
		}
		unguard;
	}
	BOOL GetDataFromUser( void )
	{
		guard(WDlgConvertToStaticMesh::GetDataFromUser);
		Package = PackageEdit.GetText();
		Name = NameEdit.GetText();
		if( !Package.Len() || !Name.Len() )
		{
			appMsgf( TEXT("Invalid input.") );
			return FALSE;
		}
		return TRUE;
		unguard;
	}
	void OnCommand(INT Command)
	{
		//called when  Message==WM_COMMAND || Message==WM_HSCROLL
		Cuts = SendMessageW(::GetDlgItem(this->hWnd, IDSL_SMBALANCE), TBM_GETPOS, 0, 0);
		_itoa(Cuts, buffer, sizeof(buffer));
		SendMessageA(::GetDlgItem(this->hWnd, IDSC_SMBALANCE), WM_SETTEXT, 0, (LPARAM)(LPCTSTR)buffer);
	}
};

// Static Mesh Import Dialog
class WDlgStaticMeshImport : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgStaticMeshImport,WDialog,UnrealEd)

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
	WDlgStaticMeshImport( UObject* InContext, WBrowser* InOwnerWindow )
	:	WDialog			( TEXT("Static Mesh Import"), IDDIALOG_IMPORT_STATICMESH, InOwnerWindow )
	,	OkButton		( this, IDOK,			FDelegate(this,(TDelegate)&WDlgStaticMeshImport::OnOk) )
	,	OkAllButton		( this, IDPB_OKALL,		FDelegate(this,(TDelegate)&WDlgStaticMeshImport::OnOkAll) )
	,	SkipButton		( this, IDPB_SKIP,		FDelegate(this,(TDelegate)&WDlgStaticMeshImport::OnSkip) )
	,	CancelButton	( this, IDCANCEL,		FDelegate(this,(TDelegate)&WDialog::EndDialogFalse) )
	,	PackageEdit		( this, IDEC_PACKAGE )
	,	GroupEdit		( this, IDEC_GROUP )
	,	NameEdit		( this, IDEC_NAME )
	,	FilenameStatic	( this, IDSC_FILENAME )
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgStaticMeshImport::OnInitDialog);
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
		guard(WDlgStaticMeshImport::OnDestroy);
		WDialog::OnDestroy();
		unguard;
	}
	virtual int DoModal( FString _defPackage, FString _defGroup, TArray<FString>* _paFilenames)
	{
		guard(WDlgStaticMeshImport::DoModal);

		defPackage = _defPackage;
		defGroup = _defGroup;
		paFilenames = _paFilenames;

		// Sort the files so that uv2 map files always comes last.
		if (_paFilenames->Num() > 1)
		{
			TArray<FString> UV2Files;
			INT i, j;

			for (i = 0; i < _paFilenames->Num(); ++i)
			{
				if ((*_paFilenames)(i).Right(8) == TEXT("_uv2.obj"))
				{
					j = UV2Files.AddZeroed();
					UV2Files(j).ExchangeArray(&(*_paFilenames)(i));
					_paFilenames->Remove(i--);
				}
			}
			for (i = 0; i < UV2Files.Num(); ++i)
			{
				j = _paFilenames->AddZeroed();
				(*_paFilenames)(j).ExchangeArray(&UV2Files(i));
			}
		}

		return WDialog::DoModal( hInstance );
		unguard;
	}
	void OnOk()
	{
		guard(WDlgStaticMeshImport::OnOk);
		if( GetDataFromUser() )
		{
			ImportFile( (*paFilenames)(iCurrentFilename) );
			SetNextFilename();
		}
		unguard;
	}
	void OnOkAll()
	{
		guard(WDlgStaticMeshImport::OnOkAll);
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
		guard(WDlgStaticMeshImport::OnSkip);
		if( GetDataFromUser() )
			SetNextFilename();
		unguard;
	}
	void ImportTexture( void )
	{
		guard(WDlgStaticMeshImport::ImportTexture);
		unguard;
	}
	void RefreshName( void )
	{
		guard(WDlgStaticMeshImport::RefreshName);
		FilenameStatic.SetText( *(*paFilenames)(iCurrentFilename) );

		FString Name = ((*paFilenames)(iCurrentFilename)).GetFilenameOnly();
		if (Name.Right(4) == TEXT("_uv2")) // UV2 map file, strip the UV2 section of it automatically.
			Name = Name.LeftChop(4);
		NameEdit.SetText( *Name );
		unguard;
	}
	BOOL GetDataFromUser( void )
	{
		guard(WDlgStaticMeshImport::GetDataFromUser);
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
		guard(WDlgStaticMeshImport::SetNextFilename);
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
		guard(WDlgStaticMeshImport::ImportFile);
		TCHAR l_chCmd[512];

		if( Group.Len() )
			appSprintf( l_chCmd, TEXT("MESH MODELIMPORT STATICMESH MODELFILE=\"%ls\" NAME=\"%ls\" PACKAGE=\"%ls\" GROUP=\"%ls\""),*(*paFilenames)(iCurrentFilename), *Name, *Package, *Group);
		else
			appSprintf( l_chCmd, TEXT("MESH MODELIMPORT STATICMESH MODELFILE=\"%ls\" NAME=\"%ls\" PACKAGE=\"%ls\""),*(*paFilenames)(iCurrentFilename), *Name, *Package);

		GEditor->Exec( l_chCmd );
		unguard;
	}
};
/*
class WDlgStaticMeshImport : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgStaticMeshImport,WDialog,UnrealEd)

	// Variables.
	WButton OkButton;
	WButton CancelButton;
	WEdit PackageEdit;
	WEdit NameEdit;
	TCHAR l_chCmd[255];
	FString Filename,Package, Name;

	// Constructor.
	WDlgStaticMeshImport( UObject* InContext, WWindow* InOwnerWindow )
	:	WDialog			( TEXT("Static Mesh Import"), IDDIALOG_IMPORT_STATICMESH, InOwnerWindow )
	,	OkButton		( this, IDOK,			FDelegate(this,(TDelegate)&WDlgStaticMeshImport::OnOk) )
	,	CancelButton	( this, IDCANCEL,		FDelegate(this,(TDelegate)&WDialog::EndDialogFalse) )
	,	PackageEdit		( this, IDEC_PACKAGE )
	,	NameEdit		( this, IDEC_NAME )
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgStaticMeshImport::OnInitDialog);
		WDialog::OnInitDialog();
		NameEdit.SetText( TEXT("") );
		PackageEdit.SetText( TEXT("") );
		::SetFocus( NameEdit.hWnd );
		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgStaticMeshImport::OnDestroy);
		WDialog::OnDestroy();
		::DestroyWindow( hWnd );
		unguard;
	}
	virtual int DoModal( FString _Filename )
	{
		guard(WDlgStaticMeshImport::DoModal);

		Filename = _Filename;

		return WDialog::DoModal( hInstance );
		unguard;
	}
	void OnOk()
	{
		guard(WDlgBrushImport::OnOk);
		if( GetDataFromUser() )
		{
			appSprintf( l_chCmd, TEXT("MESH MODELIMPORT STATICMESH MODELFILE=\"%ls\" NAME=%ls PACKAGE=%ls"),*Filename, *Name, *Package);
			GEditor->Exec( l_chCmd );
			EndDialog(TRUE);
		}
		unguard;
	}
	BOOL GetDataFromUser( void )
	{
		guard(WDlgStaticMeshImport::GetDataFromUser);
		Package = PackageEdit.GetText();
		Name = NameEdit.GetText();
		if( !Package.Len() || !Name.Len() )
		{
			appMsgf( TEXT("Invalid input.") );
			return FALSE;
		}

		return TRUE;
		unguard;
	}
};
*/
