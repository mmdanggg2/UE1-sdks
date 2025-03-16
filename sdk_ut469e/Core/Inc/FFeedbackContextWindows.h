/*=============================================================================
	FFeedbackContextWindows.h: Unreal Windows user interface interaction.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

/*-----------------------------------------------------------------------------
	FFeedbackContextWindows.
-----------------------------------------------------------------------------*/

//
// Feedback context.
//
class FFeedbackContextWindows : public FFeedbackContext
{
public:
	// Variables.
	INT SlowTaskCount;
	HWND hWndProgressBar, hWndProgressText, hWndProgressDlg, hWndCancelButton;
	UBOOL bCanceled;

	DOUBLE BeepStartTime;

	INT LastTime;
	INT LastNumerator;
	INT LastDenominator;
	const TCHAR* LastFmt;

	FContextSupplier* Context;
	INT WarningCount;

	// Constructor.
	FFeedbackContextWindows()
	: SlowTaskCount( 0 )
	, hWndProgressBar( 0 )
	, hWndProgressText( 0 )
	, hWndProgressDlg( 0 )
	, hWndCancelButton(0)
	, bCanceled(0)
	, BeepStartTime(0)
	, LastTime(-1)
	, LastNumerator(-1)
	, LastDenominator(-1)
	, LastFmt(NULL)
	, Context(NULL)
	, WarningCount(0)
	{}
	void Serialize( const TCHAR* V, EName Event )
	{
		guard(FFeedbackContextWindows::Serialize);
		if( Event==NAME_UserPrompt && (GIsClient || GIsEditor) )
		{
			MaybeBeep();
			::MessageBox( NULL, V, LocalizeError("Warning",TEXT("Core")), MB_OK|MB_TASKMODAL );
			MarkBeepTime();
		}
		else
		{
			FString Temp;
			EName OutName = NAME_Warning;
			if (Event == NAME_Error || Event == NAME_Warning || Event == NAME_ExecWarning || Event == NAME_ScriptWarning)
			{
				if(Context)
				{
					Temp = *Context->GetContext(TRUE);
					Temp += TEXT(" : ");
					Temp += V;
					V = *Temp;
				}
				OutName = Event;
				WarningCount++;
			}
			debugf(OutName, TEXT("%s"), V);
		}
		unguard;
	}
	UBOOL YesNof( const TCHAR* Fmt, ... )
	{
		va_list ArgPtr;
		va_start(ArgPtr, Fmt);
		FString Temp = FString::Printf(Fmt, ArgPtr);
		va_end(ArgPtr);

		return Ask(FALSE, Temp) == ANSWER_Yes;
	}
	EAnswer YesNoCancelf( const TCHAR* Fmt, ... )
	{
		va_list ArgPtr;
		va_start(ArgPtr, Fmt);
		FString Temp = FString::Printf(Fmt, ArgPtr);
		va_end(ArgPtr);

		return Ask(TRUE, Temp);
	}
	EAnswer Ask(UBOOL WithCancel, FString Temp)
	{
		guard(FFeedbackContextWindows::Ask);
		if( GIsClient || GIsEditor )
		{
			INT Result = ::MessageBox( NULL, *Temp, LocalizeError("Question",TEXT("Core")), (WithCancel ? MB_YESNOCANCEL : MB_YESNO) | MB_TASKMODAL );
			EAnswer ret = Result == IDYES ? ANSWER_Yes : (Result == IDNO || !WithCancel ? ANSWER_No : ANSWER_Cancel);
			return ret;
		}
		else
			return ANSWER_No;
		unguard;
	}
	void BeginSlowTask( const TCHAR* Task, UBOOL StatusWindow, UBOOL Cancelable )
	{
		guard(FFeedbackContextWindows::BeginSlowTask);
		::ShowWindow( hWndProgressDlg, SW_SHOW );
		if( hWndProgressBar && hWndProgressText )
		{
			SendMessageW( hWndProgressText, WM_SETTEXT, (WPARAM)0, (LPARAM)Task );
			SendMessageW( hWndProgressBar, PBM_SETRANGE, (WPARAM)0, MAKELPARAM(0, 100) );
			ShowWindow((HWND)hWndCancelButton, Cancelable ? SW_SHOW : SW_HIDE);

			UpdateWindow( hWndProgressDlg );
			UpdateWindow( hWndProgressText );
			UpdateWindow( hWndProgressText );

			{	// flush all messages
				MSG mfm_msg;
				while(::PeekMessage(&mfm_msg, hWndProgressDlg, 0, 0, PM_REMOVE))
				{
					TranslateMessage(&mfm_msg);
					DispatchMessage(&mfm_msg);
				}
			}
		}
		if (!GIsSlowTask)
			MarkBeepTime();
		bCanceled = 0;
		GIsSlowTask = ++SlowTaskCount>0;
		unguard;
	}
	void EndSlowTask()
	{
		guard(FFeedbackContextWindows::EndSlowTask);
		check(SlowTaskCount>0);
		bCanceled = 0;
		GIsSlowTask = --SlowTaskCount>0;
		if( !GIsSlowTask )
		{
			::ShowWindow( hWndProgressDlg, SW_HIDE );
			MaybeBeep();
		}
		unguard;
	}
	void MarkBeepTime()
	{
		guard(FFeedbackContextWindows::MarkBeepTime);
		BeepStartTime = appSecondsNew();
		unguard;
	}
	void MaybeBeep()
	{
		guard(FFeedbackContextWindows::MaybeBeep);
		if (!BeepStartTime)
			return;
		FLOAT NotifyActionEndAfterSeconds = 10.0f;
		GConfig->GetFloat(TEXT("Options"), TEXT("NotifyActionEndAfterSeconds"), NotifyActionEndAfterSeconds, GUEDIni);
		if (NotifyActionEndAfterSeconds > 0 && appSecondsNew() - BeepStartTime > NotifyActionEndAfterSeconds)
			MessageBeep(MB_OK);
		unguard;
	}
	UBOOL VARARGS StatusUpdatef( INT Numerator, INT Denominator, const TCHAR* Fmt, ... )
	{
		guard(FFeedbackContextWindows::StatusUpdatef);
		va_list ArgPtr;
		va_start(ArgPtr, Fmt);

		INT CurTime = GetTickCount();

		if (Abs(CurTime - LastTime) >= 100 || Fmt != LastFmt || LastDenominator != Denominator || Numerator < LastNumerator || (Numerator > LastNumerator && Numerator >= Denominator - 1))
		{
			LastTime = CurTime;
			LastNumerator = Numerator;
			LastDenominator = Denominator;
			LastFmt = Fmt;
			if( GIsSlowTask && hWndProgressBar && hWndProgressText )
			{
				FString Temp = FString::Printf(Fmt, ArgPtr);
				SendMessageW( hWndProgressText, WM_SETTEXT, (WPARAM)0, (LPARAM)*Temp );
				SendMessageW( hWndProgressBar, PBM_SETPOS, (WPARAM)(Denominator ? 100*Numerator/Denominator : 0), (LPARAM)0 );
				UpdateWindow( hWndProgressDlg );
				UpdateWindow( hWndProgressText );
				UpdateWindow( hWndProgressBar );

				{	// flush all messages
					MSG mfm_msg;
					while(::PeekMessage(&mfm_msg, hWndProgressDlg, 0, 0, PM_REMOVE)) {
						TranslateMessage(&mfm_msg);
						DispatchMessage(&mfm_msg);
					}
				}
			}
			else
			{
				// stijn: This is a hack to tell Windows that UEd is responsive
				MSG Msg;
				PeekMessage(&Msg, NULL, 0, 0, PM_NOREMOVE);
			}
		}

		va_end(ArgPtr);
		
		return !bCanceled;
		unguard;
	}
	void SetContext( FContextSupplier* InSupplier )
	{
		Context = InSupplier;
	}
	void OnCancelProgress()
	{
		bCanceled = 1;
	}
	void ResetWarningCount()
	{
		WarningCount = 0;
	}
	INT GetWarningCount()
	{
		return WarningCount;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
