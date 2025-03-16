/*=============================================================================
	FFileManagerLinux.h: Unreal Linux based file manager.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Brandon Reinhart
		* Major changes by Daniel Vogel
=============================================================================*/

#include <dirent.h>
#include <unistd.h>
#include "FFileManagerGeneric.h"
#include <sys/types.h>
#include <pwd.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <utime.h>
#include <fcntl.h>

/*-----------------------------------------------------------------------------
	File Manager.
-----------------------------------------------------------------------------*/

// File manager.
class FArchiveFileReader : public FArchive
{
public:
	FArchiveFileReader( FILE* InFile, FOutputDevice* InError, INT InSize )
	:	File			( InFile )
	,	Error			( InError )
	,	Size			( InSize )
	,	Pos			    ( 0 )
	,	BufferBase		( 0 )
	,	BufferCount		( 0 )
	{
		guard(FArchiveFileReader::FArchiveFileReader);
		fseek( File, 0, SEEK_SET );
		ArIsLoading = ArIsPersistent = 1;
		unguard;
	}
	~FArchiveFileReader()
	{
		guard(FArchiveFileReader::~FArchiveFileReader);
		if( File )
			Close();
		unguard;
	}
	void Precache( INT HintCount )
	{
		guardSlow(FArchiveFileReader::Precache);
		checkSlow(Pos==BufferBase+BufferCount);
		BufferBase = Pos;
		BufferCount = Min( Min( HintCount, (INT)(ARRAY_COUNT(Buffer) - (Pos&(ARRAY_COUNT(Buffer)-1))) ), Size-Pos );
		if( fread( Buffer, BufferCount, 1, File )!=1 && BufferCount!=0 )
		{
			ArIsError = 1;
			Error->Logf( TEXT("fread failed: BufferCount=%i Error=%i"), BufferCount, ferror(File) );
			return;
		}
		unguardSlow;
	}
	void Seek( INT InPos )
	{
		guard(FArchiveFileReader::Seek);
		check(InPos>=0);
		check(InPos<=Size);
		if( fseek(File,InPos,SEEK_SET) )
		{
			ArIsError = 1;
			Error->Logf( TEXT("seek Failed %i/%i: %i %i"), InPos, Size, Pos, ferror(File) );
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
		if( File )
			fclose( File );
		File = NULL;
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
					if( fread( V, Length, 1, File )!=1 )
					{
						ArIsError = 1;
						Error->Logf( TEXT("fread failed: Length=%i Error=%i"), Length, ferror(File) );
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
		return appFilePathForHandle(File);
	}
protected:
	FILE*		File;
	FOutputDevice*	Error;
	INT		    Size;
	INT		    Pos;
	INT		    BufferBase;
	INT		    BufferCount;
	BYTE		Buffer[1024];
};
class FArchiveFileWriter : public FArchive
{
public:
	FArchiveFileWriter( FILE* InFile, FOutputDevice* InError )
	:	File		( InFile )
	,	Error		( InError )
	,	Pos		    (0)
	,	BufferCount	(0)
	{
		ArIsSaving = ArIsPersistent = 1;
	}
	~FArchiveFileWriter()
	{
		guard(FArchiveFileWriter::~FArchiveFileWriter);
		if( File )
			Close();
		File = NULL;
		unguard;
	}
	void Seek( INT InPos )
	{
		Flush();
		if( fseek(File,InPos,SEEK_SET) )
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
		if( File && fclose( File ) )
		{
			ArIsError = 1;
			Error->Logf( LocalizeError("WriteFailed",TEXT("Core")) );
		}
		File = NULL;
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
		if( BufferCount && fwrite( Buffer, BufferCount, 1, File )!=1 )
		{
			ArIsError = 1;
			Error->Logf( LocalizeError("WriteFailed",TEXT("Core")) );
		}
		BufferCount=0;
	}
	FString GetFullPathName()
	{
		return appFilePathForHandle(File);
	}
protected:
	FILE*		File;
	FOutputDevice*	Error;
	INT		Pos;
	INT		BufferCount;
	BYTE		Buffer[4096];
};

class FFileManagerLinux : public FFileManagerGeneric
{
public:
	FArchive* CreateFileReader( const TCHAR* FakeFilename, DWORD Flags, FOutputDevice* Error )
	{
		guard(FFileManagerLinux::CreateFileReader);
		FString FullPath;
		if ((FakeFilename && *FakeFilename == L'/') || (Flags & FILEREAD_NoBaseDir))
			FullPath = FakeFilename;
		else
			FullPath = BaseDir + FakeFilename;		
		FILE* File = fopen(appToAnsi(*appUnixPath(FullPath)), "rb");
		if ( UseHomeDir && !(Flags & FILEREAD_NoBaseDir) && !File )
			File = fopen(appToAnsi(*appUnixPath(FakeFilename)), "rb");
		if ( !File )
		{
			if( Flags & FILEREAD_NoFail )
				appErrorf(TEXT("Failed to read file: %s"), *FullPath);
			return NULL;
		}
		fcntl( fileno(File), F_SETFD, FD_CLOEXEC );
		fseek( File, 0, SEEK_END );
		return new FArchiveFileReader(File,Error,ftell(File));
		unguard;
	}
	FArchive* CreateFileWriter( const TCHAR* FakeFilename, DWORD Flags, FOutputDevice* Error )
	{	
		guard(FFileManagerLinux::CreateFileWriter);
		FString FullPath;
		if (FakeFilename && *FakeFilename == L'/')
			FullPath = FakeFilename;
		else
			FullPath = BaseDir + FakeFilename;
		if( Flags & FILEWRITE_EvenIfReadOnly )
			chmod(appToAnsi(*appUnixPath(FullPath)), S_IREAD | S_IWRITE);
		if( (Flags & FILEWRITE_NoReplaceExisting) && FileSize(*FullPath)>=0 )
			return NULL;

        if (BaseDir.Len() > 0)  // might need to build dirs in $HOME...
        {
            const ANSICHAR* ansitmp = appToAnsi(*FullPath);
            ANSICHAR *ptr = const_cast<ANSICHAR*>(strchr(ansitmp, '/'));
            if (ptr != NULL)
            {
                for (ptr = const_cast<ANSICHAR*>(ansitmp); *ptr; ptr++)  // build each chunk.
                {
                    const ANSICHAR ch = *ptr;
                    if ((ch == '/') || (ch == '\\'))
                    {
                        *ptr = '\0';
                        // don't care if it fails, since fopen() will tell us.
                        mkdir(ansitmp, S_IRWXU);
                        *ptr = '/';  // always convert to Unix format.
                    }
                }
            }
        }

		// Higor:
		// Sometimes it's necessary to allow the file writer to touch the game directory's
		// files instead of writing to the Base dir, updating Cache files' last modified
		// date is one of those cases.
		if	(	UseHomeDir
			&&	FullPath != FakeFilename
			&&	(Flags & FILEWRITE_AllowBaseDir) 
			&&	(GetGlobalTime(*appUnixPath(FullPath)) == (SQWORD)-1)
			&&	(GetGlobalTime(*appUnixPath(FakeFilename)) != (SQWORD)-1) )
			FullPath = FakeFilename;

		const char* Mode = (Flags & FILEWRITE_Append) ? "ab" : "wb";
		FILE* File = fopen(appToAnsi(*appUnixPath(FullPath)),Mode);
		if( !File )
		{
			if( Flags & FILEWRITE_NoFail )
				appErrorf( TEXT("Failed to write: %s"), *FullPath );
			return NULL;
		}
		fcntl( fileno(File), F_SETFD, FD_CLOEXEC );
		if( Flags & FILEWRITE_Unbuffered )
			setvbuf( File, 0, _IONBF, 0 );
		return new FArchiveFileWriter(File,Error);

		unguard;
	}
	UBOOL Delete( const TCHAR* FakeFilename, UBOOL RequireExists=0, UBOOL EvenReadOnly=0 )
	{	       
		guard(FFileManagerLinux::Delete);
		FString FullPath = BaseDir + FakeFilename;
		if( EvenReadOnly )
			chmod(appToAnsi(*appUnixPath(FullPath)), S_IREAD | S_IWRITE);
		return unlink(appToAnsi(*appUnixPath(FullPath)))==0 || (errno==ENOENT && !RequireExists);
		unguard;
	}
	SQWORD GetGlobalTime( const TCHAR* Filename )
	{
		guard(FFileManagerLinux::GetGlobalTime);
		struct stat statbuf;
		if (stat(appToAnsi(Filename), &statbuf) == -1)
			return ~0;
		return static_cast<SQWORD>(statbuf.st_mtime);
		unguard;
	}
	UBOOL SetGlobalTime( const TCHAR* Filename )
	{
		guard(FFileManagerLinux::SetGlobalTime);
		struct stat statbuf;
		const char* FilenameA = appToAnsi(Filename);

		if (stat(FilenameA, &statbuf) == -1)
			return FALSE;

		struct utimbuf new_time;
		time_t t = time(NULL);
		new_time.actime = new_time.modtime = t;

		if (utime(FilenameA, &new_time) == 0)
			return TRUE;

		return FALSE;
		unguard;
	}
	UBOOL MakeDirectory( const TCHAR* FakePath, UBOOL Tree=0 )
	{
		guard(FFileManagerLinux::MakeDirectory);
		FString FullPath = BaseDir + FakePath;
		if( Tree )
			return FFileManagerGeneric::MakeDirectory( *FullPath, Tree );
		return mkdir(appToAnsi(*appUnixPath(FullPath)), S_IREAD | S_IWRITE | S_IEXEC)==0 || errno==EEXIST;
		unguard;
	}
	UBOOL DeleteDirectory( const TCHAR* FakePath, UBOOL RequireExists=0, UBOOL Tree=0 )
	{
		guard(FFileManagerLinux::DeleteDirectory);
		FString FullPath = BaseDir + FakePath;
		if( Tree )
			return FFileManagerGeneric::DeleteDirectory( *FullPath, RequireExists, Tree );
		return rmdir(appToAnsi(*appUnixPath(FullPath)))==0 || (errno==ENOENT && !RequireExists);
		unguard;
	}

	TArray<FString> FindFiles( const TCHAR* FakeFilename, UBOOL Files, UBOOL Directories )
	{
		guard(FFileManagerLinux::FindFiles);
		TArray<FString> Result;

		FString FullPath = BaseDir + FakeFilename;

		if ( UseHomeDir )
			SearchDirectory( Result, *FullPath );
		SearchDirectory( Result, FakeFilename );

		return Result;
		unguard;
	}
	UBOOL SetDefaultDirectory( const TCHAR* Filename )
	{
		guard(FFileManagerLinux::SetDefaultDirectory);
        if (*Filename == 0)
            return -1;
		return chdir(appToAnsi(*appUnixPath(Filename)))==0;
		unguard;
	}
	FString GetDefaultDirectory()
	{
		guard(FFileManagerLinux::GetDefaultDirectory);
		{
			ANSICHAR Buffer[PATH_MAX]="";
			if (getcwd( Buffer, ARRAY_COUNT(Buffer) ))
				return FString(Buffer);
			return TEXT("");
		}
		unguard;
	}

	void Init(UBOOL _UseHomeDir, const TCHAR* HomeDirOverride) 
	{
		FString Tmp;

		char *Home = getenv("HOME");
		if ( Home == NULL )
		{
			uid_t id = getuid();
			struct passwd *pwd;

			setpwent();
			while( (pwd = getpwent()) != NULL ) 
			{
				if( pwd->pw_uid == id ) 
				{
					Home = pwd->pw_dir;
					break;
				}
			}
			endpwent();
		}

		if (!_UseHomeDir)
		{
			wprintf(L"WARNING: Not using preference directory\n");
			return;
		}

		if (!HomeDirOverride || !*HomeDirOverride)
		{
			Tmp = FString(Home);
			if (Tmp.Right(1) != TEXT("/"))
				Tmp += TEXT("/");

			BaseDir = TEXT("");

#if MACOSX
			Tmp += TEXT("Library");
			mkdir(appToAnsi(*Tmp), S_IRWXU);
			Tmp += TEXT("/Application Support");
			mkdir(appToAnsi(*Tmp), S_IRWXU);
			Tmp += TEXT("/Unreal Tournament");
			mkdir(appToAnsi(*Tmp), S_IRWXU);
			Tmp += TEXT("/System");
			mkdir(appToAnsi(*Tmp), S_IRWXU);
#else
			Tmp += TEXT(".utpg");
			mkdir(appToAnsi(*Tmp), S_IRWXU);
			Tmp += TEXT("/System");
			mkdir(appToAnsi(*Tmp), S_IRWXU);
#endif

			Tmp += TEXT("/");
		}
		else
		{
			char TmpPath[PATH_MAX];
			if (*HomeDirOverride == L'~')
			{
				Tmp = FString(Home);
				Tmp += (HomeDirOverride + 1);
			}
			else
			{
				Tmp = HomeDirOverride;
			}

			if (realpath(appToAnsi(*Tmp), TmpPath))
				Tmp = FString(TmpPath);
		}

		// Whether to use HomeDir for writing or not.
		UseHomeDir = _UseHomeDir;

		// If not accessible don't use it. (user didn't call wrapper script e.g.)
		if ( access(appToAnsi(*Tmp), F_OK ) == -1 )
		{
			UseHomeDir = false;     
			wprintf(L"WARNING: preference directory not accessible: %ls\n", *Tmp);
		}

		if (UseHomeDir)
		{
			wprintf(L"WARNING: using preference directory: %ls\n", *Tmp);
			if (Tmp.Right(1) != PATH_SEPARATOR)
				Tmp += PATH_SEPARATOR;
			BaseDir = Tmp;
		}
		else
		{
			wprintf(L"WARNING: Not using preference directory\n");
		}
	}

	void SearchDirectory( TArray<FString> &Result, const TCHAR* Filename )
	{
		guard(FFileManagerLinux::SearchDirectory);
		DIR *Dirp;
		struct dirent* Direntp;
		UBOOL Match;
		FString Path = Filename, BaseName;

		// Convert MS "\" to Unix "/".
		for( INT i = 0; i != Path.Len(); i++ )
			if( Path[i] == TEXT('\\') )
				Path[i] = TEXT('/');

		BaseName = appFileBaseName(*Path);
		Path = appFilePathName(*Path);

		// Check for empty path.
		if (Path.Len() == 0)
			Path = TEXT("./");

		// Open directory, get first entry.
		Dirp = opendir( appToAnsi(*Path) );
		if (Dirp == NULL)
			return;
	
		Direntp = readdir( Dirp );

		// Check each entry.
		while( Direntp != NULL )
		{
			Match = false;

			FString Entry(Direntp->d_name);

			if( BaseName == TEXT("*") )
			{
				// Any filename.
				Match = true;
			}
			else if( BaseName == TEXT("*.*") )
			{
				// Any filename with a '.'.
				if( Entry.InStr(TEXT(".")) != -1 )
					Match = true;
			}
			else if( BaseName[0] == TEXT('*') )
			{
				// "*.ext" filename.
				FString Suffix = BaseName.Mid(1);
				if (Entry.Len() >= Suffix.Len()
					&& Entry.Right(Suffix.Len()) == Suffix)
					Match = true;
			}
			else if( BaseName.Right(1) == TEXT("*") )
			{
				// "name.*" filename.
				if (Entry.Len() >= BaseName.Len() - 1
					&& Entry.Left(BaseName.Len() - 1) == BaseName.Left(BaseName.Len() - 1))
					Match = true;
			}
			else if( BaseName.InStr(TEXT("*")) != -1 )
			{
				INT StarPos = BaseName.InStr(TEXT("*"));
				FString Prefix = BaseName.Left(StarPos);
				FString Suffix = BaseName.Mid(StarPos + 1);

				if (Entry.Len() >= Prefix.Len()
					&& Entry.Left(Prefix.Len()) == Prefix
					&& Entry.Len() >= Suffix.Len()
					&& Entry.Right(Suffix.Len()) == Suffix)
					Match = true;
			}
			else if (Entry == BaseName)
			{
				// Literal filename.
				Match = true;
			}

			// Does this entry match the Filename?
			if( Match )
			{
				// Yes, add the file name to Result.
				Result.AddItem(Entry);
			}
		
			// Get next entry.
			Direntp = readdir( Dirp );
		}
	
		// Close directory.
		closedir( Dirp );
		unguard;
	}


// vogel: CD path not implemented yet
	FString BaseDir;
	FString CDDir;

	UBOOL UseCD;
	UBOOL UseHomeDir;
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
