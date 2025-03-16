/*=============================================================================
	FConfigCacheIni.h: Unreal config file reading/writing.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

/*-----------------------------------------------------------------------------
	Config cache.
-----------------------------------------------------------------------------*/

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack(push,OBJECT_ALIGNMENT)
#endif

#pragma once

#ifdef _INC_EDITOR
extern BOOL GRecoveryMode;
#endif

// One section in a config file.
class FConfigSection : public TMultiMap<FString,FString>
{};

// One config file.
class FConfigFile : public TMap<FString,FConfigSection>
{
public:
	UBOOL Dirty, NoSave, Quotes;
	FConfigFile()
	: Dirty( 0 )
	, NoSave( 0 )
	, Quotes( 0 )
	{}
	void Read( const TCHAR* Filename )
	{
		guard(FConfigFile::Read);
		FString Text;
		if( appLoadFileToString( Text, Filename ) && Text.Len() >= 6 )
		{
			Empty();
			TCHAR* Ptr = const_cast<TCHAR*>( *Text );
			FConfigSection* CurrentSection = NULL;
			UBOOL Done = 0;
			while( !Done )
			{
				while( *Ptr=='\r' || *Ptr=='\n' )
					Ptr++;
				TCHAR* Start = Ptr;
				TCHAR* End;
				while( *Ptr && *Ptr!='\r' && *Ptr!='\n' )
					Ptr++;				
				if( *Ptr==0 )
					Done = 1;
				*Ptr++ = 0;
				if( *Start=='[' && *(End=Start+appStrlen(Start)-1)==']' )
				{
					*End = 0;
					CurrentSection = Find( ++Start );
					if( !CurrentSection )
						CurrentSection = &Set( Start, FConfigSection() );
				}
				else if( CurrentSection && *Start )
				{
					TCHAR* Value = appStrstr(Start,TEXT("="));
					if( Value )
					{
						*Value++ = 0;
						if ( *Value )
						{
							End = Value + appStrlen(Value) - 1;

							// strip trailing spaces.
							while ( *End == ' ' )
								*End-- = 0;

							// decode quotes if they're present.
							if( *Value=='\"' && *End=='\"' )
							{
								Value++;
								*End = 0;
							}
						}
						CurrentSection->Add( Start, Value );
					}
				}
			}
		}
		unguard;
	}
	UBOOL Write( const TCHAR* Filename )
	{
		guard(FConfigFile::Write);
#ifdef _INC_EDITOR
		if (GRecoveryMode)
			return 1;
#endif
		if( !Dirty || NoSave )
			return 1;
		Dirty = 0;
		FString Text;
		// check for saved values (can be broken by memory corruption)
		UBOOL Broken = FALSE;
		for( TIterator It(*this); It; ++It )
		{
			#if __UNIX__
				#define NEWLINE TEXT("\n")
			#else
				#define NEWLINE TEXT("\r\n")
			#endif

			const TCHAR* Section = *It.Key();
			Broken = Broken || appStrstr(Section, TEXT("\r")) || appStrstr(Section, TEXT("\n"));
			Text += '[';
			Text += Section;
			Text += TEXT("]") NEWLINE;
			for( FConfigSection::TIterator It2(It.Value()); It2; ++It2 )
			{
				const TCHAR* Key = *It2.Key();
				Broken = Broken || appStrstr(Key, TEXT("\r")) || appStrstr(Key, TEXT("\n")) || appStrstr(Key, TEXT("="));
				Text += Key;
				Text += '=';
				const TCHAR* Value = *It2.Value();
				UBOOL AddQuotes = Quotes;
				if (!AddQuotes && *Value != TEXT('\0'))
				{
					const TCHAR* End = Value + appStrlen(Value) - 1;
					AddQuotes = *End == ' ' || (*Value == '\"' && *End == '\"');
				}
				if (AddQuotes)
					Text += '"';
				Broken = Broken || appStrstr(Value, TEXT("\r")) || appStrstr(Value, TEXT("\n"));
				Text += Value;
				if (AddQuotes)
					Text += '"';
				Text += NEWLINE;
			}
			Text += NEWLINE;
		}
		// Never delete the contents of an INI file by save an empty line in it.
		if (Text.Len() == 0)
			return 1;
		if (Broken)
		{
			FString FilenameBroken(Filename);
			FilenameBroken += TEXT(".broken");
			debugf(TEXT("Failed to validate the configuration cache. Saving broken configuration file as %ls."), *FilenameBroken);
			return appSaveStringToFile(Text, *FilenameBroken);
		}
		return appSaveStringToFile( Text, Filename );
		unguard;
	}
};

// Set of all cached config files.
class FConfigCacheIni : public FConfigCache, public TMap<FString,FConfigFile>
{
public:
	// Basic functions.
	FString SystemIni;
	FString UserIni;

	// cache last result for speed
	FString LastFilename;
	FString Filename;

	FConfigCacheIni()
	{}
	~FConfigCacheIni() noexcept(false)
	{
		guard(FConfigCacheIni::~FConfigCacheIni);
		Flush( 1 );
		unguard;
	}
	FString GetConfigFilename( const TCHAR* InFilename )
	{
		guard(FConfigCacheIni::GetConfigFilename);
		// If filename not specified, use default.
		if (!InFilename)
			return SystemIni;

		if (LastFilename == InFilename && Filename.Len() > 0)
			return Filename;
		LastFilename = InFilename;
		Filename = InFilename;

		// Add .ini extension.
		INT Len = Filename.Len();
		if (Len < 5 || (Filename[Len - 4] != '.' && Filename[Len - 5] != '.'))
			Filename += TEXT(".ini");

		// Automatically translate generic filenames.
		if (Filename == TEXT("User.ini"))
			Filename = UserIni;
		else if (Filename == TEXT("System.ini"))
			Filename = SystemIni;

		return Filename;
		unguard;
	}
	FConfigFile* Find( const TCHAR* InFilename, UBOOL CreateIfNotFound )
	{
		guard(FConfigCacheIni::Find);

		const FString Filename = GetConfigFilename(InFilename);

		// Get file.
		FConfigFile* Result = TMap<FString,FConfigFile>::Find( Filename );
		if( !Result && (CreateIfNotFound || GFileManager->FileSize(*Filename)>=0)  )
		{
			FConfigFile ConfigFile = FConfigFile();
			ConfigFile.Read(*Filename);
			Result = &Set(*Filename, ConfigFile);
		}
		return Result;

		unguard;
	}
	void Flush( UBOOL Read, const TCHAR* Filename=NULL )
	{
		guard(FConfigCacheIni::Flush);

#ifdef _INC_EDITOR
		if (GRecoveryMode)
			return;
#endif

		for( TIterator It(*this); It; ++It )
			if( !Filename || It.Key()==Filename )
				It.Value().Write( *It.Key() );
		if( Read )
		{
			if( Filename )
				Remove(Filename);
			else
				Empty();
		}
		unguard;
	}
	void UnloadFile( const TCHAR* Filename )
	{
		guard(FConfigCacheIni::UnloadFile);
		FConfigFile* File = Find( Filename, 1 );
		if( File )
			Remove( Filename );
		unguard;
	}
	void Detach( const TCHAR* Filename )
	{
		guard(FConfigCacheIni::Detach);
		FConfigFile* File = Find( Filename, 1 );
		if( File )
			File->NoSave = 1;
		unguard;
	}
	UBOOL GetString( const TCHAR* Section, const TCHAR* Key, TCHAR* Value, INT Size, const TCHAR* Filename )
	{
		guard(FConfigCacheIni::GetString);
		*Value = 0;
		FConfigFile* File = Find( Filename, 0 );
		if( !File )
			return 0;
		FConfigSection* Sec = File->Find( Section );
		if( !Sec )
			return 0;
		FString* PairString = Sec->Find( Key );
		if( !PairString )
			return 0;
		appStrncpy( Value, **PairString, Size );
		return 1;
		unguard;
	}
	UBOOL GetString(const TCHAR* Section, const TCHAR* Key, FString& Value, const TCHAR* Filename)
	{
		guard(FConfigCacheIni::GetString);
		Value = TEXT("");
		FConfigFile* File = Find(Filename, 0);
		if (!File)
			return 0;
		FConfigSection* Sec = File->Find(Section);
		if (!Sec)
			return 0;
		FString* PairString = Sec->Find(Key);
		if (!PairString)
			return 0;
		Value = *PairString;
		return 1;
		unguard;
	}
	UBOOL GetSection( const TCHAR* Section, TCHAR* Result, INT Size, const TCHAR* Filename )
	{
		guard(FConfigCacheIni::GetSection);
		*Result = 0;
		FConfigFile* File = Find( Filename, 0 );
		if( !File )
			return 0;
		FConfigSection* Sec = File->Find( Section );
		if( !Sec )
			return 0;
		TCHAR* End = Result;
		for( FConfigSection::TIterator It(*Sec); It && End-Result+appStrlen(*It.Key())+1<Size; ++It )
			End += appSprintf( End, TEXT("%s=%s"), *It.Key(), *It.Value() ) + 1; // stijn: mem safety NOT OK
		*End++ = 0;
		return 1;
		unguard;
	}
	UBOOL GetSectionSafe(const TCHAR* Section, TArray<FString>& Result, const TCHAR* Filename)
	{
		guard(FConfigCacheIni::GetSectionSafe);
		Result.Empty();
		FConfigFile* File = Find(Filename, 0);
		if (!File)
			return 0;
		FConfigSection* Sec = File->Find(Section);
		if (!Sec)
			return 0;
		for (FConfigSection::TIterator It(*Sec); It; ++It)
		{
			FString& Item = *new(Result) FString(*It.Key());
			Item += '=';
			Item += *It.Value();
		}
		return 1;
		unguard;
	}
	TMultiMap<FString,FString>* GetSectionPrivate( const TCHAR* Section, UBOOL Force, UBOOL Const, const TCHAR* Filename )
	{
		guard(FConfigCacheIni::GetSectionPrivate);
		FConfigFile* File = Find( Filename, Force );
		if( !File )
			return NULL;
		FConfigSection* Sec = File->Find( Section );
		if( !Sec && Force )
			Sec = &File->Set( Section, FConfigSection() );
		if( Sec && (Force || !Const) )
			File->Dirty = 1;
		return Sec;
		unguard;
	}
	UBOOL GetFileSections(TArray<FString>& Sections, const TCHAR* Filename = NULL)
	{
		guard(FConfigCacheIni::GetFileSections);
		Sections.Empty();
		FConfigFile* File = Find(Filename, 0);
		if (File)
		{
			for (FConfigFile::TIterator It(*File); It; ++It)
				new (Sections) FString(It.Key());
			return 1;
		}
		return 0;
		unguard;
	}
	void SetString( const TCHAR* Section, const TCHAR* Key, const TCHAR* Value, const TCHAR* Filename )
	{
		guard(FConfigCacheIni::SetString);
		FConfigFile* File = Find( Filename, 1 );
		FConfigSection* Sec  = File->Find( Section );
		if( !Sec )
			Sec = &File->Set( Section, FConfigSection() );
		FString* Str = Sec->Find( Key );
		if( !Str )
		{
			Sec->Add( Key, Value );
			File->Dirty = 1;
		}
		else if( appStricmp(**Str,Value)!=0 )
		{
			File->Dirty = (appStrcmp(**Str,Value)!=0);
			*Str = Value;
		}
		unguard;
	}
	void EmptySection( const TCHAR* Section, const TCHAR* Filename )
	{
		guard(FConfigCacheIni::EmptySection);
		FConfigFile* File = Find( Filename, 0 );
		if( File )
		{
			FConfigSection* Sec = File->Find( Section );
		// rjp --
			if( Sec )
			{
				if ( FConfigSection::TIterator(*Sec) )
					Sec->Empty();

				File->Remove(Section);
				File->Dirty = 1;
			}	// -- rjp
		}
		unguard;
	}
	void Init( const TCHAR* InSystem, const TCHAR* InUser, UBOOL RequireConfig )
	{
		guard(FConfigCacheIni::Init);
		SystemIni = InSystem;
		UserIni   = InUser;
		unguard;
	}
	void Exit()
	{
		guard(FConfigCacheIni::Exit);
		Flush( 1 );
		unguard;
	}
	void Dump( FOutputDevice& Ar )
	{
		guard(FConfigCacheIni::Dump);
		Ar.Log( TEXT("Files map:") );
		TMap<FString,FConfigFile>::Dump( Ar );
		unguard;
	}

	// Derived functions.
	const TCHAR* GetStr( const TCHAR* Section, const TCHAR* Key, const TCHAR* Filename )
	{
		guard(FConfigCacheIni::GetStr);
		TCHAR* Result = appStaticString1024();
		GetString( Section, Key, Result, 1024, Filename );
		return Result;
		unguard;
	}
	UBOOL GetInt
	(
		const TCHAR*	Section,
		const TCHAR*	Key,
		INT&			Value,
		const TCHAR*	Filename
	)
	{
		guard(FConfigCacheIni::GetInt);
		TCHAR Text[80]; 
		if( GetString( Section, Key, Text, ARRAY_COUNT(Text), Filename ) )
		{
			Value = appAtoi(Text);
			return 1;
		}
		return 0;
		unguard;
	}
	UBOOL GetFloat
	(
		const TCHAR*	Section,
		const TCHAR*	Key,
		FLOAT&			Value,
		const TCHAR*	Filename
	)
	{
		guard(FConfigCacheIni::GetFloat);
		TCHAR Text[80]; 
		if( GetString( Section, Key, Text, ARRAY_COUNT(Text), Filename ) )
		{
			Value = appAtof(Text);
			return 1;
		}
		return 0;
		unguard;
	}
	UBOOL GetBool
	(
		const TCHAR*	Section,
		const TCHAR*	Key,
		UBOOL&			Value,
		const TCHAR*	Filename
	)
	{
		guard(FConfigCacheIni::GetBool);
		TCHAR Text[80]; 
		if( GetString( Section, Key, Text, ARRAY_COUNT(Text), Filename ) )
		{
			if( appStricmp(Text,TEXT("True"))==0 )
			{
				Value = 1;
			}
			else
			{
				Value = appAtoi(Text)==1;
			}
			return 1;
		}
		return 0;
		unguard;
	}
	void SetInt
	(
		const TCHAR* Section,
		const TCHAR* Key,
		INT			 Value,
		const TCHAR* Filename
	)
	{
		guard(FConfigCacheIni::SetInt);
		TCHAR Text[30];
		appSnprintf( Text, ARRAY_COUNT(Text), TEXT("%i"), Value );
		SetString( Section, Key, Text, Filename );
		unguard;
	}
	void SetFloat
	(
		const TCHAR*	Section,
		const TCHAR*	Key,
		FLOAT			Value,
		const TCHAR*	Filename
	)
	{
		guard(FConfigCacheIni::SetFloat);
		TCHAR Text[30];
		appSnprintf( Text, ARRAY_COUNT(Text), TEXT("%f"), Value );
		SetString( Section, Key, Text, Filename );
		unguard;
	}
	void SetBool
	(
		const TCHAR* Section,
		const TCHAR* Key,
		UBOOL		 Value,
		const TCHAR* Filename
	)
	{
		guard(FConfigCacheIni::SetBool);
		SetString( Section, Key, Value ? TEXT("True") : TEXT("False"), Filename );
		unguard;
	}

	// Static allocator.
	static FConfigCache* Factory()
	{
		return new FConfigCacheIni();
	}
};

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (pop)
#endif

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
