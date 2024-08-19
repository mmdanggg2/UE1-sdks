/*=============================================================================
	FOutputDeviceAnsiError.h: Ansi stdout error output device.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

//
// ANSI stdout output device.
//
class FOutputDeviceAnsiError : public FOutputDeviceError
{
	INT ErrorPos;
	EName ErrorType;
	FString ErrorHist;
	void LocalPrint( const TCHAR* Str )
	{
		wprintf(TEXT("%ls"),Str);
	}
public:
	FOutputDeviceAnsiError()
	: ErrorPos(0)
	, ErrorType(NAME_None)
	{}
	void Serialize( const TCHAR* Msg, enum EName Event )
	{
#if defined(_DEBUG)
		// Just display info and break the debugger.
  		debugf( NAME_Critical, TEXT("appError called while debugging:") );
		debugf( NAME_Critical, Msg );
		UObject::StaticShutdownAfterError();
  		debugf( NAME_Critical, TEXT("Breaking debugger") );
		// stijn: the pointer needs to be volatile, otherwise the compiler can
		// remove the null deref even at low optimization levels.
		*(volatile BYTE*)NULL=0;
#else
		if( !GIsCriticalError )
		{
			// First appError.
			GIsCriticalError = 1;
			ErrorType        = Event;
			debugf( NAME_Critical, TEXT("appError called:") );
			debugf( NAME_Critical, Msg );

			// Shut down.
			UObject::StaticShutdownAfterError();
			ErrorHist = Msg;
			ErrorHist += TEXT("\r\n\r\n");
			ErrorPos = ErrorHist.Len();
			if( GIsGuarded )
			{
				ErrorHist += LocalizeError("History",TEXT("Core"));
				ErrorHist += TEXT(": ");
#if !defined(_MSC_VER)
				FString GuardBackTrace;
				UnGuardBlockTLS::GetBackTrace(GuardBackTrace);
				ErrorHist += GuardBackTrace;
#endif
				if (ErrorHist.Len() + 1 < ARRAY_COUNT(GErrorHist))
					appStrcpy(GErrorHist, *ErrorHist); // stijn: mem safety ok
				else
					GErrorHist[0] = 0;
			}
			else
			{
				HandleError();
			}
		}
		else debugf( NAME_Critical, TEXT("Error reentered: %s"), Msg );

		// Propagate the error or exit.
		if( GIsGuarded )
		{
//			appThrowf( TEXT("Throwing C++ exception for error logging") );
			throw (1);
		}
		else
			appRequestExit( 1 );
#endif
	}
	void HandleError()
	{
		try
		{
			GIsGuarded       = 0;
			GIsRunning       = 0;
			GIsCriticalError = 1;
			GLogHook         = NULL;
			UObject::StaticShutdownAfterError();
			if (GErrorHist[0])
				LocalPrint(GErrorHist);
			else
				LocalPrint( *ErrorHist );
			LocalPrint( TEXT("\n\nExiting due to error\n") );
		}
		catch( ... )
		{}
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
