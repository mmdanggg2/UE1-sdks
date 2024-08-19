/*=============================================================================
	FFileManagerWindows.h: Unreal Windows OS based file manager.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
		* HomeDir override code by Fernando Velazquez (Higor)
=============================================================================*/

#include "FFileManagerGeneric.h"

/*-----------------------------------------------------------------------------
	Important notes.
-----------------------------------------------------------------------------*/

/** LONG PATH AWARE:

	As of Windows 10 build 1607 we have a new 'Long Path Aware' functionality that
	allows the operating system to lift the MAX_PATH limitation.

	The following key must exist and be set to 1:
 HKLM\SYSTEM\CurrentControlSet\Control\FileSystem LongPathsEnabled (Type: REG_DWORD)
	
	The application manifest must contain:
<application xmlns="urn:schemas-microsoft-com:asm.v3">
	<windowsSettings xmlns:ws2="http://schemas.microsoft.com/SMI/2016/WindowsSettings">
		<ws2:longPathAware>true</ws2:longPathAware>
	</windowsSettings>
</application>


These are the directory management functions that no longer have MAX_PATH restrictions if you opt-in to long path behavior:
	CreateDirectoryW, CreateDirectoryExW, GetCurrentDirectoryW, RemoveDirectoryW, SetCurrentDirectoryW.

These are the file management functions that no longer have MAX_PATH restrictions if you opt-in to long path behavior:
	CopyFileW, CopyFile2, CopyFileExW, CreateFileW, CreateFile2, CreateHardLinkW, CreateSymbolicLinkW, DeleteFileW,
	FindFirstFileW, FindFirstFileExW, FindNextFileW, GetFileAttributesW, GetFileAttributesExW, SetFileAttributesW,
	GetFullPathNameW, GetLongPathNameW, MoveFileW, MoveFileExW, MoveFileWithProgressW, ReplaceFileW, SearchPathW,
	FindFirstFileNameW, FindNextFileNameW, FindFirstStreamW, FindNextStreamW, GetCompressedFileSizeW, GetFinalPathNameByHandleW.
*/



/*-----------------------------------------------------------------------------
	File Manager.
-----------------------------------------------------------------------------*/

// File manager.
class FArchiveFileReader : public FArchive
{
public:
	FArchiveFileReader( HANDLE InHandle, FOutputDevice* InError, INT InSize )
	:	Handle			( InHandle )
	,	Error			( InError )
	,	Size			( InSize )
	,	Pos				( 0 )
	,	BufferBase		( 0 )
	,	BufferCount		( 0 )
	,	Buffer			{0}
	{
		guard(FArchiveFileReader::FArchiveFileReader);
		ArIsLoading = ArIsPersistent = 1;
		unguard;
	}
	~FArchiveFileReader()
	{
		guard(FArchiveFileReader::~FArchiveFileReader);
		if( Handle )
			Close();
		unguard;
	}
	void Precache( INT HintCount )
	{
		guardSlow(FArchiveFileReader::Precache);
		checkSlow(Pos==BufferBase+BufferCount);
		BufferBase = Pos;
		BufferCount = Min( Min( HintCount, (INT)(ARRAY_COUNT(Buffer) - (Pos&(ARRAY_COUNT(Buffer)-1))) ), Size-Pos );
		INT Count=0;
		if( ReadFile( Handle, Buffer, BufferCount, (DWORD*)&Count, NULL ) && Count!=BufferCount )
		{
			ArIsError = 1;
			Error->Logf( TEXT("ReadFile failed: Count=%i BufferCount=%i Error=%s"), Count, BufferCount, appGetSystemErrorMessage() );
		}
		unguardSlow;
	}
	void Seek( INT InPos )
	{
		guard(FArchiveFileReader::Seek);
		check(InPos>=0);
		check(InPos<=Size);
		if( SetFilePointer( Handle, InPos, 0, FILE_BEGIN )==0xFFFFFFFF )
		{
			ArIsError = 1;
			Error->Logf( TEXT("SetFilePointer Failed %i/%i: %i %s"), InPos, Size, Pos, appGetSystemErrorMessage() );
		}
		Pos         = InPos;
		BufferBase  = Pos;
		BufferCount = 0;
		unguard;
	}
	INT Tell()
	{
		return Pos;
	}
	INT TotalSize()
	{
		return Size;
	}
	UBOOL Close()
	{
		guardSlow(FArchiveFileReader::Close);
		if( Handle )
			CloseHandle( Handle );
		Handle = NULL;
		return !ArIsError;
		unguardSlow;
	}
	void Serialize( void* V, INT Length )
	{
		guardSlow(FArchiveFileReader::Serialize);
		while( Length>0 )
		{
			INT Copy = Min( Length, BufferBase+BufferCount-Pos );
			if( Copy==0 )
			{
				if( Length >= ARRAY_COUNT(Buffer) )
				{
					INT Count=0;
					if( ReadFile( Handle, V, Length, (DWORD*)&Count, NULL ) && Count!=Length )
					{
						ArIsError = 1;
						Error->Logf( TEXT("ReadFile failed: Count=%i Length=%i Error=%s"), Count, Length, appGetSystemErrorMessage() );
					}
					Pos += Length;
					BufferBase += Length;
					return;
				}
				Precache( MAXINT );
				Copy = Min( Length, BufferBase+BufferCount-Pos );
				if( Copy<=0 )
				{
					ArIsError = 1;
					Error->Logf( TEXT("ReadFile beyond EOF %i+%i/%i"), Pos, Length, Size );
				}
				if( ArIsError )
					return;
			}
			memcpy( V, Buffer+Pos-BufferBase, Copy );
			Pos       += Copy;
			Length    -= Copy;
			V          = (BYTE*)V + Copy;
		}
		unguardSlow;
	}
	FString GetFullPathName()
	{
		return appFilePathForHandle(Handle);
	}
protected:
	HANDLE			Handle;
	FOutputDevice*	Error;
	INT				Size;
	INT				Pos;
	INT				BufferBase;
	INT				BufferCount;
	BYTE			Buffer[1024];
};
class FArchiveFileWriter : public FArchive
{
public:
	FArchiveFileWriter( HANDLE InHandle, FOutputDevice* InError, INT InPos )
	:	Handle		( InHandle )
	,	Error		( InError )
	,	Pos			( InPos )
	,	BufferCount	( 0 )
	,	Buffer		{0}
	{
		ArIsSaving = ArIsPersistent = 1;
	}
	~FArchiveFileWriter()
	{
		guard(FArchiveFileWriter::~FArchiveFileWriter);
		if( Handle )
			Close();
		Handle = NULL;
		unguard;
	}
	void Seek( INT InPos )
	{
		Flush();
		if( SetFilePointer( Handle, InPos, 0, FILE_BEGIN )==0xFFFFFFFF )
		{
			ArIsError = 1;
			Error->Logf( LocalizeError("SeekFailed",TEXT("Core")) );
		}
		Pos = InPos;
	}
	INT Tell()
	{
		return Pos;
	}
	UBOOL Close()
	{
		guardSlow(FArchiveFileWriter::Close);
		Flush();
		if( Handle && !CloseHandle(Handle) )
		{
			ArIsError = 1;
			Error->Logf( LocalizeError("WriteFailed",TEXT("Core")) );
		}
		else
		{
			Handle = NULL;
		}
		return !ArIsError;
		unguardSlow;
	}
	void Serialize( void* V, INT Length )
	{
		Pos += Length;
		INT Copy;
		while( Length > (Copy=ARRAY_COUNT(Buffer)-BufferCount) )
		{
			appMemcpy( Buffer+BufferCount, V, Copy );
			BufferCount += Copy;
			Length      -= Copy;
			V            = (BYTE*)V + Copy;
			Flush();
		}
		if( Length )
		{
			appMemcpy( Buffer+BufferCount, V, Length );
			BufferCount += Length;
		}
	}
	void Flush()
	{
		if( BufferCount )
		{
			INT Result=0;
			if( !WriteFile( Handle, Buffer, BufferCount, (DWORD*)&Result, NULL ) )
			{
				ArIsError = 1;
				Error->Logf( LocalizeError("WriteFailed",TEXT("Core")) );
			}
		}
		BufferCount = 0;
	}
	FString GetFullPathName()
	{
		return appFilePathForHandle(Handle);
	}
protected:
	HANDLE			Handle;
	FOutputDevice*	Error;
	INT				Pos;
	INT				BufferCount;
	BYTE			Buffer[4096];
};


class FFileManagerWindows : public FFileManagerGeneric
{
public:
	FFileManagerWindows()
		: FFileManagerGeneric()
		, DirEntry(0)
	{
	}

	FArchive* CreateFileReader( const TCHAR* Filename, DWORD Flags, FOutputDevice* Error )
	{
		guard(FFileManagerWindows::CreateFileReader);
		FArchive* Result = nullptr;
		if ( !(Flags & FILEREAD_NoBaseDir) && TryBaseDir(Filename) )
			Result = InternalCreateFileReader( *InternalBaseDirFilename(Filename), Flags & ~FILEREAD_NoFail, Error);
		if ( !Result )
			Result = InternalCreateFileReader( Filename, Flags, Error);
		return Result;
		unguard;
	}
	FArchive* CreateFileWriter( const TCHAR* Filename, DWORD Flags, FOutputDevice* Error )
	{
		guard(FFileManagerWindows::CreateFileWriter);
		if ( TryBaseDir(Filename) )
		{
			FString BaseDirName = InternalBaseDirFilename(Filename);

			// Higor:
			// Sometimes it's necessary to allow the file writer to touch the game directory's
			// files instead of writing to the Base dir, updating Cache files' last modified
			// date is one of those cases.
			if	(	(Flags & FILEWRITE_AllowBaseDir) 
				&&	(InternalFileSize(*BaseDirName) < 0)
				&&	(InternalFileSize(Filename) >= 0) )
				return InternalCreateFileWriter( Filename, Flags, Error);
			else
				return InternalCreateFileWriter( *BaseDirName, Flags, Error);
		}
		return InternalCreateFileWriter( Filename, Flags, Error);
		unguard;
	}
	INT FileSize( const TCHAR* Filename )
	{
		guard(FFileManagerWindows::FileSize);
		INT Size = -1;
		if ( TryBaseDir(Filename) )
			Size = InternalFileSize( *InternalBaseDirFilename(Filename) );
		if ( Size == -1 )
			Size = InternalFileSize( Filename );
		return Size;
		unguard;
	}
	UBOOL Copy( const TCHAR* DestFile, const TCHAR* SrcFile, UBOOL ReplaceExisting, UBOOL EvenIfReadOnly, UBOOL Attributes, void (*Progress)(FLOAT Fraction) )
	{
		FString BaseSrcFile, BaseDestFile;
		if ( TryBaseDir(SrcFile) ) // Copy from BaseDir if found.
		{
			BaseSrcFile = InternalBaseDirFilename(SrcFile);
			if ( InternalFileSize(*BaseSrcFile) != -1 )
				SrcFile = *BaseSrcFile;
		}
		if ( TryBaseDir(DestFile) ) // Only copy into BaseDir, if specified
		{
			BaseDestFile = InternalBaseDirFilename(DestFile);
			DestFile = *BaseDestFile;
		}
		if( EvenIfReadOnly )
			SetFileAttributesW(DestFile, 0);
		UBOOL Result
		=	Progress
		?	FFileManagerGeneric::Copy( DestFile, SrcFile, ReplaceExisting, EvenIfReadOnly, Attributes, Progress )
		:	CopyFileW(SrcFile, DestFile, !ReplaceExisting)!=0;
		if( Result && !Attributes )
			SetFileAttributesW(DestFile, 0);
		return Result;
	}
	UBOOL Delete( const TCHAR* Filename, UBOOL RequireExists=0, UBOOL EvenReadOnly=0 )
	{
		guard(FFileManagerWindows::Delete);
		FString BaseFilename;
		if ( TryBaseDir(Filename) ) // Only delete from BaseDir, if specified
		{
			BaseFilename = InternalBaseDirFilename(Filename);
			Filename = *BaseFilename;
		}
		if( EvenReadOnly )
			SetFileAttributesW(Filename,FILE_ATTRIBUTE_NORMAL);
		return DeleteFile(Filename)!=0 || (!RequireExists && GetLastError()==ERROR_FILE_NOT_FOUND);
		unguard;
	}
	UBOOL Move( const TCHAR* DestFile, const TCHAR* SrcFile, UBOOL Replace=1, UBOOL EvenIfReadOnly=0, UBOOL Attributes=0 )
	{
		guard(FFileManagerWindows::Move);
		FString BaseSrcFile, BaseDestFile;

		if ( Replace )
			Delete( DestFile, 0, 1 );
		if ( TryBaseDir(SrcFile) ) // If we have BaseDir, but source is in install dir, do a copy operation instead
		{
			BaseSrcFile = InternalBaseDirFilename(SrcFile);
			if ( InternalFileSize(*BaseSrcFile) != -1 )
				SrcFile = *BaseSrcFile;
			else
				return Copy( DestFile, SrcFile, Replace, EvenIfReadOnly, Attributes, nullptr);
		}
		if ( TryBaseDir(DestFile) ) // Only move into BaseDir, if specified
		{
			BaseDestFile = InternalBaseDirFilename(DestFile);
			DestFile = *BaseDestFile;
		}
	
		INT Result =  MoveFileW(SrcFile,DestFile);
		if( !Result )
			debugf( NAME_Warning, TEXT("Error moving file '%s' to '%s' (%ls)"), SrcFile, DestFile, appGetSystemErrorMessage(appGetSystemErrorCode()) );
		return Result!=0;
		unguard;
	}
	SQWORD GetGlobalTime( const TCHAR* Filename )
	{
		guard(FFileManagerWindows::GetGlobalTime);
		FILETIME LastWriteTime;

		HANDLE Handle = CreateFileW
		(
			Filename, 
			GENERIC_READ,
			FILE_SHARE_READ,
			nullptr, 
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL, 
			NULL
		);

		if (Handle == INVALID_HANDLE_VALUE)
			return ~0;

		if (GetFileTime(Handle, nullptr, nullptr, &LastWriteTime))
		{
			CloseHandle(Handle);

			// Convert to Unix timestamp
			ULARGE_INTEGER Result;
			Result.LowPart = LastWriteTime.dwLowDateTime;
			Result.HighPart = LastWriteTime.dwHighDateTime;
			return static_cast<SQWORD>(Result.QuadPart / 10000000ULL - 11644473600ULL);
		}

		CloseHandle(Handle);
		return ~0;
		unguard;
	}
	UBOOL SetGlobalTime( const TCHAR* Filename )
	{
		guard(FFileManagerWindows::SetGlobalTime);
		FILETIME LastWriteTime;
		SYSTEMTIME CurrentTime;

		HANDLE Handle = CreateFileW
		(
			Filename,
			FILE_WRITE_ATTRIBUTES,
			FILE_SHARE_WRITE,
			nullptr,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		if (Handle == INVALID_HANDLE_VALUE)
			return FALSE;

		GetSystemTime(&CurrentTime);
		SystemTimeToFileTime(&CurrentTime, &LastWriteTime);

		if (SetFileTime(Handle, nullptr, nullptr, &LastWriteTime))
		{
			CloseHandle(Handle);
			return TRUE;
		}

		CloseHandle(Handle);
		return FALSE;
		unguard;
	}
	UBOOL MakeDirectory( const TCHAR* Path, UBOOL Tree=0 )
	{
		guard(FFileManagerWindows::MakeDirectory);
		UBOOL Result;
		FString BasePath;
		if ( !DirEntry && TryBaseDir(Path) )
		{
			BasePath = InternalBaseDirFilename(Path);
			Path = *BasePath;
		}
		DirEntry++;
		if( Tree )
			Result = FFileManagerGeneric::MakeDirectory( Path, Tree );
		else
			Result = CreateDirectoryW(Path,NULL)!=0 || GetLastError()==ERROR_ALREADY_EXISTS;
		DirEntry--;
		return Result;
		unguard;
	}
	UBOOL DeleteDirectory( const TCHAR* Path, UBOOL RequireExists=0, UBOOL Tree=0 )
	{
		guard(FFileManagerWindows::DeleteDirectory);
		UBOOL Result;
		FString BasePath;
		if ( !DirEntry && TryBaseDir(Path) )
		{
			BasePath = InternalBaseDirFilename(Path);
			Path = *BasePath;
		}
		DirEntry++;
		if( Tree )
			Result = FFileManagerGeneric::DeleteDirectory( Path, RequireExists, Tree );
		else
			Result = RemoveDirectoryW(Path)!=0 || (!RequireExists && GetLastError()==ERROR_FILE_NOT_FOUND);
		DirEntry--;
		return Result;
		unguard;
	}
	TArray<FString> FindFiles( const TCHAR* Filename, UBOOL Files, UBOOL Directories )
	{
		guard(FFileManagerWindows::FindFiles);
		TArray<FString> BaseDirResult;
		if ( TryBaseDir(Filename) )
			BaseDirResult = InternalFindFiles( *InternalBaseDirFilename(Filename), Files, Directories);
		TArray<FString> Result = InternalFindFiles( Filename, Files, Directories);

		// BaseDir list must override install dir list (case insensitive)
		for ( INT i=0; i<BaseDirResult.Num(); i++)
		{
			bool Found = false;
			for ( INT j=0; j<Result.Num(); j++)
				if ( !appStricmp(*BaseDirResult(i),*Result(j)) )
				{
					Found = true;
					break;
				}
			if ( !Found ) // TODO: Use move semantics
				Result.AddItem( BaseDirResult(i) );
		}
		return Result;
		unguard;
	}
	UBOOL SetDefaultDirectory( const TCHAR* Filename )
	{
		guard(FFileManagerWindows::SetDefaultDirectory);
		return SetCurrentDirectoryW(Filename)!=0;
		unguard;
	}
	FString GetDefaultDirectory()
	{
		guard(FFileManagerWindows::GetDefaultDirectory);
		FString Directory;
		INT BufferSize = (INT)GetCurrentDirectoryW( 0, nullptr);
		if ( BufferSize > 0 )
		{
			Directory.GetCharArray().SetSize(BufferSize);
			GetCurrentDirectoryW( BufferSize, (TCHAR*)Directory.GetCharArray().GetData() );
		}
		return Directory;
		unguard;
	}

	void Init( UBOOL UseHomeDir, const TCHAR* HomeDirOverride) 
	{
		guard(FFileManagerWindows::Init);
		if ( HomeDirOverride && *HomeDirOverride )
		{
			BaseDir = HomeDirOverride;
			if ( BaseDir.Right(1) != TEXT("\\") && BaseDir.Right(1) != TEXT("/") )
				BaseDir += TEXT("\\");
			// Convert BaseDir into absolute path and create it.
			MakeDirectory( *InternalBaseDirFilename( TEXT("")), true);
		}
		/*else
		{
			TCHAR LauncherName[MAX_PATH];
			if (GetModuleFileNameW(GetModuleHandle(NULL), LauncherName, ARRAY_COUNT(LauncherName)) != 0)
			{
				BaseDir = appFilePathName(LauncherName);
				BaseDir += TEXT("\\");
			}
		}*/
		unguard;
	}

protected:
	typedef BOOL (STDAPICALLTYPE *PathIsRelativeW_FUNC)( LPCWSTR);

	FArchive* InternalCreateFileReader( const TCHAR* Filename, DWORD Flags, FOutputDevice* Error )
	{
		guard(InternalCreateFileReader);
		DWORD  Access    = GENERIC_READ;
		DWORD  WinFlags  = FILE_SHARE_READ;
		DWORD  Create    = OPEN_EXISTING;
		HANDLE Handle	 = CreateFileW(Filename, Access, WinFlags, NULL, Create, FILE_ATTRIBUTE_NORMAL, NULL);
		if( Handle==INVALID_HANDLE_VALUE )
		{
			if( Flags & FILEREAD_NoFail )
				appErrorf( TEXT("Failed to read file: %s"), Filename );
			return NULL;
		}
		return new FArchiveFileReader(Handle,Error,GetFileSize(Handle,NULL));
		unguard;
	}

	FArchive* InternalCreateFileWriter( const TCHAR* Filename, DWORD Flags, FOutputDevice* Error )
	{
		if ( Flags & FILEWRITE_EvenIfReadOnly )
			SetFileAttributesW( Filename, 0);
		DWORD  Access    = GENERIC_WRITE;
		DWORD  WinFlags  = (Flags & FILEWRITE_AllowRead) ? FILE_SHARE_READ : 0;
		DWORD  Create    = (Flags & FILEWRITE_Append) ? OPEN_ALWAYS : (Flags & FILEWRITE_NoReplaceExisting) ? CREATE_NEW : CREATE_ALWAYS;
		HANDLE Handle    = CreateFileW( Filename, Access, WinFlags, NULL, Create, FILE_ATTRIBUTE_NORMAL, NULL );
		INT    Pos       = 0;
		if( Handle==INVALID_HANDLE_VALUE )
		{
			if( Flags & FILEWRITE_NoFail )
				appErrorf( TEXT("Failed to create file: %s"), Filename );
			return NULL;
		}
		if( Flags & FILEWRITE_Append )
			Pos = SetFilePointer( Handle, 0, 0, FILE_END );
		return new FArchiveFileWriter(Handle,Error,Pos);
	}

	INT InternalFileSize( const TCHAR* Filename )
	{
		HANDLE Handle = CreateFileW( Filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
		if( Handle==INVALID_HANDLE_VALUE )
			return -1;
		DWORD Result = GetFileSize( Handle, NULL );
		CloseHandle( Handle );
		return Result;
	}

	TArray<FString> InternalFindFiles( const TCHAR* Filename, UBOOL Files, UBOOL Directories )
	{
		TArray<FString> Result;
		HANDLE Handle=NULL;
		WIN32_FIND_DATAW Data;
		Handle=FindFirstFileW(Filename,&Data);
		if( Handle!=INVALID_HANDLE_VALUE )
			do
				if
					(	appStricmp(Data.cFileName,TEXT("."))
						&&	appStricmp(Data.cFileName,TEXT(".."))
						&&	((Data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)?Directories:Files) )
					new(Result)FString(Data.cFileName);
		while( FindNextFileW(Handle,&Data) );
		if( Handle )
			FindClose( Handle );
		return Result;
	}

	BOOL InternalIsRelativePath( const TCHAR* Path, bool bQueryingBaseDir=false)
	{
		guard(FFileManagerWindows::InternalIsRelativePath);
		static PathIsRelativeW_FUNC PathIsRelative = (PathIsRelativeW_FUNC)GetSymbol("PathIsRelativeW",TEXT("shlwapi.dll"));

		// Error, do not use BaseDir
		if ( !PathIsRelative ) 
		{
			BaseDir = TEXT("");
			return false;
		}

		BOOL Relative = PathIsRelative(Path);

		// Paths starting with "\" are drive-relative, and PathIsRelative returns false on these. 
		// Treat as relative if we have a BaseDir in a different drive.
		if ( (Path[0] == '\\') && (Path[1] != '\\') && !Relative )
		{
			// BaseDir starting with "\" is in same drive as default dir.
			if ( bQueryingBaseDir )
				Relative = true;
			// If BaseDir is a relative path, then it's on the same drive.
			else if ( InternalIsRelativePath( *BaseDir, true) )
				Relative = true;
			// If BaseDir is absolute path (contains drive???), relative if in same drive.
			else
				Relative = (appStrnicmp( *BaseDir, *GetDefaultDirectory(), 2) == 0);
		}
		return Relative;

		unguard;
	}

	FString InternalBaseDirFilename( const TCHAR* Filename)
	{
		guard(InternalBaseDirFilename);
		FString Result = ShrinkPath( BaseDir + Filename);
		// Drive relative path
		if ( (Result.Len() > 0) && (Result[0] == '\\') && (Result[1] != '\\') )
			return GetDefaultDirectory().Left(2) + Result;
		return Result;
		unguard;
	}

	//
	// Utils
	//

	// Do we want to try BaseDir first for this file/path?
	bool TryBaseDir( const TCHAR* Filename)
	{
		guard(TryBaseDir);
		if ( (BaseDir != TEXT("")) && InternalIsRelativePath(Filename) )
			return true;
		return false;
		unguard;
	}

	// Process "..\" sections, optionally remove Filename from path.
	static FString ShrinkPath( const FString& Path, bool RemoveFile=false)
	{
		guard(FFileManagerWindows::ShrinkPath);
		if ( !**Path )
			return TEXT("");

		TArray<FString> Segments;
		FString Buffer = Path;
		TCHAR* InputStart = (TCHAR*)*Buffer;
		for ( TCHAR* Input=InputStart; *Input; Input++)
			if ( (*Input == '\\') || (*Input == '/') )
			{
				*Input = '\0';
				Segments.AddItem( FString(InputStart) + TEXT("\\") );
				InputStart = Input + 1;
			}
		if ( *InputStart && !RemoveFile )
			Segments.AddItem(InputStart);

		// Does first segment indicate it's an absolute path?
		bool Absolute = Segments.Num() > 0 
			? (IsDrive(*Segments(0)) || Segments(0) == TEXT("\\"))
			: 0;

		// Eliminate pairs of [NON-BASE-SEGMENT]+[..]
		for ( INT i=0; i<Segments.Num(); i++)
			if ( (i > 0) && (Segments(i) == TEXT("..\\")) )
			{
				Segments.Remove(i--);
				if ( (Segments(i) != TEXT("..\\")) && (i>0 || !Absolute) ) 
					Segments.Remove(i--);
			}

		FString Result;
		for ( INT i=0; i<Segments.Num(); i++)
			Result += Segments(i);

		return Result;
		unguard;
	}

	static bool IsDrive( const TCHAR* Path)
	{
		return appIsAlpha(Path[0]) && (Path[1] == ':') && (!Path[2] || Path[2] == '\\');
	}

	// TODO: Move to Windows platform headers
	static FARPROC GetSymbol( const ANSICHAR* Symbol, const TCHAR* Module)
	{
		HMODULE Handle = GetModuleHandleW( Module );
		if ( !Handle )
			Handle = LoadLibraryW( Module);
		return Handle ? GetProcAddress( Handle, Symbol) : nullptr;
	}

protected:
	FString BaseDir;
	INT DirEntry;
};


/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
