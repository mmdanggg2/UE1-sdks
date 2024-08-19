/*=============================================================================
	FFeedbackContextAnsi.h: Unreal Ansi user interface interaction.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

/*-----------------------------------------------------------------------------
	FFeedbackContextAnsi.
-----------------------------------------------------------------------------*/

//
// Feedback context.
//
class FFeedbackContextAnsi : public FFeedbackContext
{
public:
	// Variables.
	INT SlowTaskCount;
	INT WarningCount;
	FContextSupplier* Context;
	FOutputDevice* AuxOut;
	INT LogPrefix;

	// Local functions.
	void LocalPrint( const TCHAR* Str )
	{
		wprintf(TEXT("%ls"),Str);
	}

	// Constructor.
	FFeedbackContextAnsi()
	: SlowTaskCount( 0 )
	, WarningCount( 0 )
	, Context( NULL )
	, AuxOut( NULL )
	, LogPrefix( -1 )
	{}
	void Serialize( const TCHAR* V, EName Event )
	{
		guard(FFeedbackContextAnsi::Serialize);
		FString Temp;
		if( Event==NAME_Title )
		{
			return;
		}
		else if( Event==NAME_Heading )
		{
			Temp = TEXT("--------------------");
			Temp += V;
			Temp += TEXT("--------------------");
			V = *Temp;
		}
		else if( Event==NAME_SubHeading )
		{
			Temp = V;
			Temp += TEXT("...");
			V = *Temp;
		}
		else if( Event==NAME_Error || Event==NAME_Warning || Event==NAME_ExecWarning || Event==NAME_ScriptWarning )
		{
			if( Context )
			{
				Temp = *Context->GetContext();
				Temp += TEXT(" : ");
				Temp += *FName(Event);
				Temp += TEXT(", ");
				Temp += V;
				V = *Temp;
			}
			WarningCount++;
		}
		else if( Event==NAME_Progress )
		{
			Temp = V;
			V = *Temp;
			LocalPrint( V );
			LocalPrint( TEXT("\r") );
			fflush( stdout );
			return;
		}
	    //elmuerte: log prefix, if set append the event names
	    if (LogPrefix == -1) 
			LogPrefix = INT( ParseParam( appCmdLine(), TEXT("LOGPREFIX") ));
	    if (LogPrefix == 1) 
	    {
			Temp = FName::SafeString(Event);
			Temp += TEXT(": ");
			Temp += V;
			V = *Temp;
	    } //elmuerte: log prefix -- end
		LocalPrint( V );
		LocalPrint( TEXT("\n") );
		if( GLog != this )
			GLog->Serialize( V, Event );
		if( AuxOut )
			AuxOut->Serialize( V, Event );
		fflush( stdout );
		unguard;
	}
	UBOOL YesNof( const TCHAR* Fmt, ... )
	{
		va_list ArgPtr;
		va_start(ArgPtr, Fmt);
		FString Temp = FString::Printf(Fmt, ArgPtr);
		va_end(ArgPtr);

		guard(FFeedbackContextAnsi::YesNof);
		if( (GIsClient || GIsEditor) && !ParseParam(appCmdLine(),TEXT("Silent")) )//!!
		{
			LocalPrint(*Temp);
			LocalPrint( TEXT(" (Y/N): ") );
			fflush(stdout);
			fflush(stderr);
			INT Ch = getchar(), Dummy;
			// stijn: this is ridiculous, but necessary. If we type a Y or N char followed by an enter, the first getchar call will consume the Y/N, and the next getchar will consume the \n
			while ((Dummy = getchar()) != '\n' && Dummy != EOF)
				; 
			return (Ch=='Y' || Ch=='y');
		}
		else return 1;
		unguard;
	}
	void BeginSlowTask( const TCHAR* Task, UBOOL StatusWindow, UBOOL Cancelable )
	{
		guard(FFeedbackContextAnsi::BeginSlowTask);
		GIsSlowTask = ++SlowTaskCount>0;
		unguard;
	}
	void EndSlowTask()
	{
		guard(FFeedbackContextAnsi::EndSlowTask);
		check(SlowTaskCount>0);
		GIsSlowTask = --SlowTaskCount>0;
		unguard;
	}
	UBOOL VARARGS StatusUpdatef( INT Numerator, INT Denominator, const TCHAR* Fmt, ... )
	{
		guard(FFeedbackContextAnsi::StatusUpdatef);
		va_list ArgPtr;
		va_start(ArgPtr, Fmt);
		va_end(ArgPtr);
		
		return 1;
		unguard;
	}
	void SetContext( FContextSupplier* InSupplier )
	{
		guard(FFeedbackContextAnsi::SetContext);
		Context = InSupplier;
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
