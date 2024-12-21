/*=============================================================================
	USetupDefinitionWindows.cpp: Unreal Windows installer/filer code.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Tim Sweeney.
=============================================================================*/

#pragma warning( disable : 4201 )
#define STRICT
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <io.h>
#include <direct.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include "..\..\External\dxsdk\Include\DSetup.h"
#include "Res\resource.h"
#include "SetupPrivate.h"
#include "USetupDefinitionWindows.h"
#include "Window.h"

//!!duplicated
static void regSet( HKEY Base, FString Dir, FString Key, FString Value )
{
	guard(regSet);
	HKEY hKey = NULL;
	check(RegCreateKeyW( Base, *Dir, &hKey )==ERROR_SUCCESS);
	check(RegSetValueExW( hKey, *Key, 0, REG_SZ, (BYTE*)*Value, (Value.Len()+1)*sizeof(TCHAR) )==ERROR_SUCCESS);
	check(RegCloseKey( hKey )==ERROR_SUCCESS);
	unguard;
}
static HKEY regGetRootKey( FString Root )
{
	guard(regGetRootKey);
	HKEY Key
	=	Root==TEXT("HKEY_CLASSES_ROOT")	 ? HKEY_CLASSES_ROOT
	:	Root==TEXT("HKEY_CURRENT_USER")	 ? HKEY_CURRENT_USER
	:	Root==TEXT("HKEY_LOCAL_MACHINE") ? HKEY_LOCAL_MACHINE
	:	NULL;
	if( !Key )
		appErrorf( TEXT("Invalid root registry key %ls"), *Root );
	return Key;
	unguard;
}
static UBOOL regGet( HKEY Base, FString Dir, FString Key, FString& Str )
{
	guard(regGetFString);
	HKEY hKey = NULL;
	if( RegOpenKeyW( Base, *Dir, &hKey )==ERROR_SUCCESS )
	{
		TCHAR Buffer[4096]=TEXT("");
		DWORD Type=REG_SZ, BufferSize=sizeof(Buffer);
		if
		(	RegQueryValueExW( hKey, *Key, 0, &Type, (BYTE*)Buffer, &BufferSize )==ERROR_SUCCESS
		&&	Type==REG_SZ )
		{
			Str = Buffer;
			return 1;
		}
	}
	Str = TEXT("");
	return 0;
	unguard;
}

// HRESULT checking.
#define verifyHRESULT(fn) {HRESULT hRes=fn; if( hRes!=S_OK ) appErrorf( TEXT(#fn) TEXT(" failed (%08X)"), hRes );}
#define verifyHRESULTSlow(fn) if(fn){}

/*-----------------------------------------------------------------------------
	USetupShortcut.
-----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(USetupShortcut);

/*-----------------------------------------------------------------------------
	USetupGroupWindows.
-----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(USetupGroupWindows);

/*-----------------------------------------------------------------------------
	USetupDefinitionWindows.
-----------------------------------------------------------------------------*/

USetupDefinitionWindows::USetupDefinitionWindows()
: hWndManager( NULL )
{}
UBOOL USetupDefinitionWindows::GetRegisteredProductFolder( FString Product, FString& Folder )
{
	guard(USetupDefinitionWindows::GetRegisteredProduct);
	return regGet( HKEY_LOCAL_MACHINE, US + TEXT("Software\\Unreal Technology\\Installed Apps\\") + Product, TEXT("Folder"), Folder )!=0;
	unguard;
}
void USetupDefinitionWindows::SetupFormatStrings()
{
	guard(USetupDefinitionWindows::SetupFormatStrings);
	USetupDefinition::SetupFormatStrings();

	// Get system directory.
	TCHAR TSysDir[256]=TEXT(""), TWinDir[256]=TEXT("");
	GetSystemDirectory( TSysDir, ARRAY_COUNT(TSysDir) );
	GetWindowsDirectory( TWinDir, ARRAY_COUNT(TWinDir) );
	WinSysPath = TSysDir;
	WinPath    = TWinDir;
	GConfig->SetString( TEXT("Setup"), TEXT("WinPath"), *WinPath, *ConfigFile );
	GConfig->SetString( TEXT("Setup"), TEXT("WinSysPath"), *WinSysPath, *ConfigFile );

	// Per user folders.
	TCHAR Temp[MAX_PATH]=TEXT("");
	SHGetSpecialFolderPathW( (HWND)hWndManager, Temp, CSIDL_DESKTOP,  0 );
	DesktopPath = Temp;
	GConfig->SetString( TEXT("Setup"), TEXT("DesktopPath"), *DesktopPath, *ConfigFile );

	SHGetSpecialFolderPathW( (HWND)hWndManager, Temp, CSIDL_PROGRAMS,  0 );
	ProgramsPath = Temp;
	GConfig->SetString( TEXT("Setup"), TEXT("ProgramsPath"), *ProgramsPath, *ConfigFile );

	SHGetSpecialFolderPathW( (HWND)hWndManager, Temp, CSIDL_FAVORITES,  0 );
	FavoritesPath = Temp;
	GConfig->SetString( TEXT("Setup"), TEXT("FavoritesPath"), *FavoritesPath, *ConfigFile );

	SHGetSpecialFolderPathW( (HWND)hWndManager, Temp, CSIDL_STARTUP,  0 );
	StartupPath = Temp;
	GConfig->SetString( TEXT("Setup"), TEXT("StartupPath"), *StartupPath, *ConfigFile );

	// Common folders.
	SHGetSpecialFolderPathW( (HWND)hWndManager, Temp, CSIDL_COMMON_PROGRAMS,  0 );
	CommonProgramsPath = *Temp ? Temp : *ProgramsPath;
	GConfig->SetString( TEXT("Setup"), TEXT("CommonProgramsPath"), *CommonProgramsPath, *ConfigFile );

	SHGetSpecialFolderPathW( (HWND)hWndManager, Temp, CSIDL_COMMON_FAVORITES,  0 );
	CommonFavoritesPath = *Temp ? Temp : *FavoritesPath;
	GConfig->SetString( TEXT("Setup"), TEXT("CommonFavoritesPath"), *CommonFavoritesPath, *ConfigFile );

	SHGetSpecialFolderPathW( (HWND)hWndManager, Temp, CSIDL_COMMON_STARTUP ,  0 );
	CommonStartupPath = *Temp ? Temp : *StartupPath;
	GConfig->SetString( TEXT("Setup"), TEXT("CommonStartupPath"), *CommonStartupPath, *ConfigFile );

	unguard;
}
void USetupDefinitionWindows::ProcessExtra( FString Key, FString Value, UBOOL Selected, FInstallPoll* Poll )
{
	guard(USetupDefinitionWindows::ProcessExtra);
	if( Selected && Key==TEXT("WinRegistry") )
	{
		// Create registry items.
		INT Pos=Value.InStr(TEXT("="));
		check(Pos>=0);
		FString RegKey   = Value.Left(Pos);
		FString RegValue = Value.Mid(Pos+1);

		// Update uninstallation log.
		UninstallLogAdd( TEXT("WinRegistry"), *RegKey, 0, 1 );

		// Get root key.
		Pos               = RegKey.InStr(TEXT("\\"));
		check(Pos>=0);
		FString Root      = Pos>=0 ? RegKey.Left(Pos)  : RegKey;
		RegKey            = Pos>=0 ? RegKey.Mid(Pos+1) : TEXT("");
		HKEY hKey          = regGetRootKey( Root );
		Pos               = RegKey.InStrRight(TEXT("\\"));
		check(Pos>=0);
		regSet( hKey, *RegKey.Left(Pos), *RegKey.Mid(Pos+1), *RegValue );
	}
	else if( Selected && Key==TEXT("Shortcut") )
	{
		USetupShortcut* Shortcut = new(GetOuter(),*Value)USetupShortcut;

		// Get icon.
		INT Pos=Shortcut->Icon.InStr(TEXT(",")), IconIndex=0;
		if( Pos>=0 )
		{
			IconIndex = appAtoi( *(Shortcut->Icon.Mid(Pos+1)) );
			Shortcut->Icon = Shortcut->Icon.Left( Pos );
		}

		// Expand parameters.
		Shortcut->Template         = Format( Shortcut->Template        , *Value );
		Shortcut->Command          = Format( Shortcut->Command         , *Value );
		Shortcut->Parameters       = Format( Shortcut->Parameters      , *Value );
		Shortcut->Icon             = Format( Shortcut->Icon            , *Value );
		Shortcut->WorkingDirectory = Format( Shortcut->WorkingDirectory, *Value );
		GFileManager->MakeDirectory( *BasePath(Shortcut->Template), 1 );

		// Update uninstallation log.
		UninstallLogAdd( TEXT("Shortcut"), *Shortcut->Template, 0, 1 );

		// Make Windows shortcut.
		IShellLink* psl=NULL;
		verify(SUCCEEDED(CoCreateInstance( CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&psl )));
		check(psl->SetPath( *Shortcut->Command )==NOERROR);
		check(psl->SetWorkingDirectory( *Shortcut->WorkingDirectory )==NOERROR);
		check(psl->SetArguments( *Shortcut->Parameters )==NOERROR);
		if (Shortcut->Icon.Len())
			check(psl->SetIconLocation( *Shortcut->Icon, IconIndex )==NOERROR);
		IPersistFile* ppf=NULL;
		verify(SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (void**)&ppf)));
		verifyHRESULTSlow(ppf->Save(reinterpret_cast<LPCOLESTR>(appToUnicode(*Shortcut->Template)),TRUE));
		ppf->Release();
		psl->Release();
	}
	else Super::ProcessExtra( Key, Value, Selected, Poll );
	unguard;
}
USetupDefinitionWindows* GSetup=NULL;
FInstallPoll* GPoll=NULL;
DWORD WINAPI DirectXSetupCallbackFunction( DWORD Reason, DWORD MsgType, char* szMessage, char* szName, void* pInfo )
{
	guard(DirectXSetupCallbackFunction);
	if( MsgType!=0 )
		return MessageBoxA((HWND)GSetup->hWndManager,szMessage,"DirectX Update Caption",MsgType);
	if( szMessage )
		GPoll->Poll(appFromAnsi(szMessage),0,1,0,0);
	return 0;
	unguard;
}
void USetupDefinitionWindows::ProcessPostCopy( FString Key, FString Value, UBOOL Selected, FInstallPoll* Poll )
{
	guard(USetupDefinitionWindows::ProcessPostCopy);
	USetupDefinition::ProcessPostCopy( Key, Value, Selected, Poll );
	if( Selected && Key==TEXT("DirectXHook") )
	{
		// Load DirectX library.
		FString File    = Format(Value);
		HMODULE hMod    = LoadLibraryW( *File );
		UBOOL   Success = 0;
		if( hMod )
		{
			INT(WINAPI*DirectXSetupSetCallbackA)(DSETUP_CALLBACK) = (INT(WINAPI*)(DSETUP_CALLBACK))GetProcAddress( hMod, "DirectXSetupSetCallback" );
			INT(WINAPI*DirectXSetupA)( HWND hWnd, LPSTR lpszRootPath, DWORD dwFlags ) = (INT(WINAPI*)(HWND,LPSTR,DWORD))GetProcAddress( hMod, "DirectXSetupA" );
			if( DirectXSetupA && DirectXSetupSetCallbackA )
			{
				GSetup = this;
				GPoll = Poll;
				DirectXSetupSetCallbackA( DirectXSetupCallbackFunction );
				INT Result = DirectXSetupA( (HWND)hWndManager, const_cast<ANSICHAR*>(appToAnsi(*BasePath(File))), DSETUP_DIRECTX );
				if( Result==DSETUPERR_SUCCESS_RESTART )
					MustReboot = 1;
				if( Result>=0 )
					Success = 1;
			}
		}
	}
	unguard;
}
void USetupDefinitionWindows::ProcessUninstallRemove( FString Key, FString Value, FInstallPoll* Poll )
{
	guard(USetupDefinitionWindows::ProcessUninstallRemove);
	Poll->Poll(*Value,0,1,UninstallCount++,UninstallTotal);
	if( Key==TEXT("Shortcut") && UpdateRefCount(*Key,*Value,-1)==0 )
	{
		GFileManager->Delete( appFromAnsi(appToAnsi(*Value)),1 );
		RemoveEmptyDirectory( *BasePath(Value) );
	}
	else if( Key==TEXT("WinRegistry") && UpdateRefCount(*Key,*Value,-1)==0 )
	{
		// Get root key.
		FString RegKey    = Value;
		INT Pos           = RegKey.InStr(TEXT("\\"));
		check(Pos>=0);
		FString Root      = Pos>=0 ? RegKey.Left(Pos)  : RegKey;
		RegKey            = Pos>=0 ? RegKey.Mid(Pos+1) : TEXT("");
		HKEY hKey          = regGetRootKey( Root );
		Pos               = RegKey.InStrRight(TEXT("\\"));
		check(Pos>=0);
		FString Path      = RegKey.Left(Pos);
		FString Name      = RegKey.Mid(Pos+1);
		HKEY ThisKey=NULL;
		RegOpenKeyExW( hKey, *Path, 0, KEY_ALL_ACCESS, &ThisKey );
		if( ThisKey )
		{
			RegDeleteValueW( ThisKey, *Name );
			RegCloseKey( ThisKey );
		}
		while( Path!=TEXT("") )
		{
			RegOpenKeyExW( hKey, *Path, 0, KEY_ALL_ACCESS, &ThisKey );
			if( !ThisKey )
				break;
			DWORD SubKeyCount=0;
			RegQueryInfoKeyW( ThisKey, NULL, NULL, NULL, &SubKeyCount, NULL, NULL, NULL, NULL, NULL, NULL, NULL );
			RegCloseKey( ThisKey );
			if( SubKeyCount )
				break;
			FString SubKeyName;
			while( Path.Len() && Path.Right(1)!=PATH_SEPARATOR )
			{
				SubKeyName = Path.Right(1) + SubKeyName;
				Path = Path.LeftChop( 1 );
			}
			if( Path.Right(1)==PATH_SEPARATOR )
				Path = Path.LeftChop( 1 );
			HKEY SuperKey = hKey;
			if( Path!=TEXT("") )
				RegOpenKeyExW( hKey, *Path, 0, KEY_ALL_ACCESS, &SuperKey );
			if( !SuperKey )
				break;
			RegDeleteKeyW( SuperKey, *SubKeyName );
			RegCloseKey( SuperKey );
		}
	}
	else Super::ProcessUninstallRemove( Key, Value, Poll );
	unguard;
}
void USetupDefinitionWindows::PreExit()
{
	guard(USetupDefinitionWindows::PreExit);
	if( MustReboot )
	{
		HANDLE hToken=NULL;
		TOKEN_PRIVILEGES tkp;
		OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken );
		LookupPrivilegeValueW( NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid );
		tkp.PrivilegeCount = 1;
		tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		AdjustTokenPrivileges( hToken, FALSE, &tkp, 0, NULL, 0 );
		ExitWindowsEx( EWX_REBOOT, 0 );
	}
	unguard;
}
void USetupDefinitionWindows::PerformUninstallCopy()
{
	guard(USetupDefinitionWindows::PerformUninstallCopy);
	TCHAR TempPath[MAX_PATH]=TEXT("");
	GetTempPathW( ARRAY_COUNT(TempPath), TempPath );
 	TMultiMap<FString,FString>* Map = GConfig->GetSectionPrivate( *Product, 0, 1, SETUP_INI );
	if( !Map )
		appErrorf(TEXT("Uninstaller couldn't find product: %ls"), *Product );
	check(Map);
	for( TMultiMap<FString,FString>::TIterator It(*Map); It; ++It )
	{
		if( It.Key()==TEXT("Backup") )
		{
			FString Src = Format(It.Value());
			if( !LocateSourceFile( Src ) )
				appErrorf(TEXT("Uninstaller failed to find file %ls (%ls)"), *It.Value(), *Src );
			FString Dest = FString(TempPath)*BaseFilename(It.Value());
			if( !GFileManager->Copy( *Dest, *Src, 1, 1, 0, NULL ) )
				appErrorf(TEXT("Uninstaller failed to copy file %ls (%ls) to %ls"), *It.Value(), *Src, *Dest );
		}
	}
	FString Launch = FString(TempPath)*TEXT("Setup.exe");
	HINSTANCE Code = ShellExecuteW(NULL, TEXT("open"), *Launch, *(US + TEXT("reallyuninstall \"") + Product + TEXT("\" Path=\"") + appBaseDir() + TEXT("\"")), TempPath, SW_SHOWNORMAL);

	if (*reinterpret_cast<INT*>(&Code) <= 32)
		appErrorf(TEXT("Uninstaller failed to launch %ls"), *Launch );
	unguard;
}
void USetupDefinitionWindows::CreateRootGroup()
{
	guard(USetupDefinitionWindows::CreateRootGroup);
	USetupGroup::Manager = this;
	RootGroup = new(GetOuter(), TEXT("Setup"))USetupGroupWindows;
	unguard;
}
IMPLEMENT_CLASS(USetupDefinitionWindows);

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
