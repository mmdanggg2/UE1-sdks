/*=============================================================================
	FOutputDeviceFile.h: ANSI file output device.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#include "FFileManagerUnicode.h"

#pragma once

//
// ANSI file output device.
//
class FOutputDeviceFile : public FOutputDevice
{
public:
	FString Buffer;

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
					if (LogAr && !LogAr->IsOpen())
					{
						FString Base = Filename;
						if (Base.Right(4) == TEXT(".log"))
							Base = Base.Left(Base.Len() - 4);
						FString Check;
						for (INT i = 2; i < 10; i++)
						{
							Check = FString::Printf(TEXT("%ls%d.log"), *Base, i);
							LogAr = new FArchiveUnicodeWriterHelper(*Check, FILEWRITE_AllowRead|FILEWRITE_Unbuffered|(Opened?FILEWRITE_Append:0));
							if (LogAr && LogAr->IsOpen())
							{
								Filename = Check;
								break;
							}
						}
					}
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
					// Special dump during error state.
					if (GIsCriticalError)
					{
						WriteStringSafe(FName::SafeString(Event));
						WriteStringSafe(TEXT(": "));
						WriteStringSafe(Data);
						WriteStringSafe(LINE_TERMINATOR);
					}
					else
					{
						Buffer.EmptyNoRealloc();
						Buffer += FName::SafeString(Event);
						Buffer += TEXT(": ");
						Buffer += Data;
						Buffer += LINE_TERMINATOR;
						LogAr->WriteString(*Buffer);
						if (Buffer.Len() > 64*1024)
							Buffer.Empty();
					}
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
	// Buggie: During error state we must be careful and not make new allocations.
	// Also we need feed LogAr with small portions of data, not more then 1024 symbols.
	// We can be slow here, that not really matter.
	// And we must flush after every write, because crash can happen in any moment.
	void WriteStringSafe(const TCHAR* Data)
	{
		if (!LogAr)
			return;
		const INT Len = appStrlen(Data);
		INT Pos = 0;
		while (LogAr && Pos < Len)
		{
			appStrncpy(LogBuf, &Data[Pos], ARRAY_COUNT(LogBuf));
			Pos += ARRAY_COUNT(LogBuf) - 1;
			if (LogAr)
				LogAr->WriteString(LogBuf);
			if (LogAr)
				LogAr->Flush();
		}
	}

	UBOOL Opened, Dead, ForceFlush;
	// Little less from FArchiveUnicodeWriterHelper::LogBuf for be sure
	TCHAR LogBuf[1001]{}; 
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
