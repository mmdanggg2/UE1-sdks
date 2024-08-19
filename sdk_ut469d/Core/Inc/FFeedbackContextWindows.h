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

	INT LastTime;
	INT LastNumerator;
	INT LastDenominator;
	const TCHAR* LastFmt;

	// Constructor.
	FFeedbackContextWindows()
	: SlowTaskCount( 0 )
	, hWndProgressBar( 0 )
	, hWndProgressText( 0 )
	, hWndProgressDlg( 0 )
	, hWndCancelButton(0)
	, bCanceled(0)
	, LastTime(-1)
	, LastNumerator(-1)
	, LastDenominator(-1)
	, LastFmt(NULL)
	{}
	void Serialize( const TCHAR* V, EName Event )
	{
		guard(FFeedbackContextWindows::Serialize);
		if( Event==NAME_UserPrompt && (GIsClient || GIsEditor) )
			::MessageBox( NULL, V, LocalizeError("Warning",TEXT("Core")), MB_OK|MB_TASKMODAL );
		else
			debugf( NAME_Warning, TEXT("%s"), V );
		unguard;
	}
	UBOOL YesNof( const TCHAR* Fmt, ... )
	{
		va_list ArgPtr;
		va_start(ArgPtr, Fmt);
		FString Temp = FString::Printf(Fmt, ArgPtr);
		va_end(ArgPtr);

		guard(FFeedbackContextWindows::YesNof);
		if( GIsClient || GIsEditor )
			return( ::MessageBox( NULL, *Temp, LocalizeError("Question",TEXT("Core")), MB_YESNO|MB_TASKMODAL ) == IDYES);
		else
			return 0;
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
			::ShowWindow( hWndProgressDlg, SW_HIDE );
		unguard;
	}
	UBOOL VARARGS StatusUpdatef( INT Numerator, INT Denominator, const TCHAR* Fmt, ... )
	{
		guard(FFeedbackContextWindows::StatusUpdatef);
		va_list ArgPtr;
		va_start(ArgPtr, Fmt);

		INT CurTime = GetTickCount();

		if (CurTime - LastTime >= 100 || Fmt != LastFmt || LastDenominator != Denominator || Numerator < LastNumerator || (Numerator > LastNumerator && Numerator >= Denominator - 1))
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
	{}
	void OnCancelProgress()
	{
		bCanceled = 1;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
