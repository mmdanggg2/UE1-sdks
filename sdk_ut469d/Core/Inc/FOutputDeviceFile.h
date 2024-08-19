/*=============================================================================
	FOutputDeviceFile.h: ANSI file output device.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#include "FFileManagerUnicode.h"

//
// ANSI file output device.
//
class FOutputDeviceFile : public FOutputDevice
{
public:
	FOutputDeviceFile()
	: LogAr( NULL )
	, Opened( 0 )
	, Dead( 0 )
	, ForceFlush( 0 )
	{
#if FORCE_UTF8_LOG
		Encoding = ENCODING_UTF8_BOM;
#elif FORCE_ANSI_LOG
		Encoding = ENCODING_ANSI;
#else
		Encoding = ENCODING_UTF16LE_BOM;
#endif		
	}
	~FOutputDeviceFile()
	{
		if( LogAr )
		{
			Logf( NAME_Log, TEXT("Log file closed, %s"), appTimestamp() );
			delete LogAr;
			LogAr = NULL;
		}
	}
	void Serialize( const TCHAR* Data, enum EName Event )
	{
		static UBOOL Entry=0;
		if( !GIsCriticalError || Entry )
		{
			if( !FName::SafeSuppressed(Event) )
			{
				if( !LogAr && !Dead )
				{
					// Make log filename.
					if( Filename.Len() == 0 )
					{
						FString TmpLogName;
						if (Parse(appCmdLine(), TEXT("LOG="), TmpLogName))
							Filename += TmpLogName;
						else if (Parse(appCmdLine(), TEXT("ABSLOG="), TmpLogName))
							Filename = TmpLogName;
						else
						{
							Filename += appPackage();
							Filename += TEXT(".log");
						}
					}

					if (ParseParam(appCmdLine(), TEXT("forceflush")) ||
						ParseParam(appCmdLine(), TEXT("forcelogflush")))
						ForceFlush = 1;

					// Open log file.
					LogAr = new FArchiveUnicodeWriterHelper(*Filename, FILEWRITE_AllowRead|FILEWRITE_Unbuffered|(Opened?FILEWRITE_Append:0));
					if( LogAr && LogAr->IsOpen() )
					{
						Opened = 1;
						LogAr->SetEncoding(Encoding, true);
						Logf( NAME_Log, TEXT("Log file open, %s"), appTimestamp() );
					}
					else Dead = 1;
				}
				if( LogAr && Event!=NAME_Title )
				{					
#if __EMSCRIPTEN__
# if UNICODE
					wprintf(TEXT("%ls: %ls\n"), FName::SafeString(Event), Data);
# else
                    printf("%s: %s\n", FName::SafeString(Event), Data);
# endif
#endif					
					LogAr->WriteString(FName::SafeString(Event));
					LogAr->WriteString(TEXT(": "));
					LogAr->WriteString(Data);
					LogAr->WriteString(LINE_TERMINATOR);
				}
				
				if( GLogHook )
					GLogHook->Serialize( Data, Event );

				if (ForceFlush && LogAr)
					LogAr->Flush();
			}
		}
		else
		{
			Entry=1;
			try
			{
				// Ignore errors to prevent infinite-recursive exception reporting.
				Serialize( Data, Event );
			}
			catch( ... )
			{}
			Entry=0;
		}
	}
	void Flush() { if (LogAr) LogAr->Flush(); }
	FArchiveUnicodeWriterHelper* LogAr;
	FString Filename;
	FileEncoding Encoding = ENCODING_ANSI;
	
private:
	UBOOL Opened, Dead, ForceFlush;	
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
