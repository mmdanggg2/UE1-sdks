/*=============================================================================
	CodeFrame : This window is where all UnrealScript editing takes place
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

	Work-in-progress todo's:

=============================================================================*/

#pragma once

static TArray<FString> GFindHistory, GReplaceHistory;
static FString GSearchText;
static UBOOL GMatchCase = 0;
static UBOOL GWholeWord = 0;

class WDlgFindReplace : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgFindReplace, WDialog, UnrealEd)

	// Variables.
	WButton FindButton;
	WButton FindNextButton;
	WButton FindPrevButton;
	WButton ReplaceButton;
	WButton ReplaceAllButton;
	WButton CancelButton;
	WButton CloseButton;
	WComboBox FindCombo;
	WComboBox ReplaceCombo;
	WCheckBox MatchCaseCheck;
	WCheckBox WholeWordCheck;

	HWND EditHwnd;

	// Constructor.
	WDlgFindReplace(UObject* InContext, WWindow* InOwnerWindow)
		: WDialog(TEXT("Find/Replace"), IDDIALOG_FINDREPLACE, InOwnerWindow)
		, FindButton(this, IDPB_FIND, FDelegate(this, (TDelegate)&WDlgFindReplace::OnFind))
		, FindNextButton(this, IDPB_FIND_NEXT, FDelegate(this, (TDelegate)&WDlgFindReplace::OnFindNext))
		, FindPrevButton(this, IDPB_FIND_PREV, FDelegate(this, (TDelegate)&WDlgFindReplace::OnFindPrev))
		, ReplaceButton(this, IDPB_REPLACE, FDelegate(this, (TDelegate)&WDlgFindReplace::OnReplace))
		, ReplaceAllButton(this, IDPB_REPLACE_ALL, FDelegate(this, (TDelegate)&WDlgFindReplace::OnReplaceAll))
		, CloseButton(this, IDPB_CLOSE, FDelegate(this, (TDelegate)&WDlgFindReplace::OnCloseButton))
		, FindCombo(this, IDCB_FIND)
		, ReplaceCombo(this, IDCB_REPLACE)
		, MatchCaseCheck(this, IDCK_MATCH_CASE)
		, WholeWordCheck(this, IDCK_WHOLE_WORD)
		, EditHwnd(NULL)
	{
	}
	
	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgFindReplace::OnInitDialog);
		WDialog::OnInitDialog();

		::SetFocus(FindCombo.hWnd);
		RefreshHistory();

		// If there is text selected, pull it in and make it the default text to search for.
		CHARRANGE range;
		SendMessageW(EditHwnd, EM_EXGETSEL, 0, (LPARAM)&range);

		if (range.cpMax - range.cpMin)
		{
			char Text[256] = "";

			TEXTRANGEA txtrange;
			txtrange.chrg = range;
			txtrange.lpstrText = Text;
			SendMessageW(EditHwnd, EM_GETTEXTRANGE, 0, (LPARAM)&txtrange);

			FindCombo.SetText(appFromAnsi(Text));
		}

		MatchCaseCheck.SetCheck(GMatchCase ? BST_CHECKED : BST_UNCHECKED);
		WholeWordCheck.SetCheck(GWholeWord ? BST_CHECKED : BST_UNCHECKED);

		unguard;
	}
	void UpdateHistory(FString Find, FString Replace)
	{
		// FIND
		//
		if (Find.Len())
		{
			// Check if value is already in the list.  If not, add it.
			int x = 0;
			for (; x < GFindHistory.Num(); x++)
			{
				if (GFindHistory(x) == Find)
					break;
			}

			if (x == GFindHistory.Num())
				GFindHistory.AddItem(Find);
		}

		// REPLACE
		//
		if (Replace.Len())
		{
			int x = 0;
			for (; x < GReplaceHistory.Num(); x++)
			{
				if (GReplaceHistory(x) == Replace)
					break;
			}

			if (x == GReplaceHistory.Num())
				GReplaceHistory.AddItem(Replace);
		}
	}
	void RefreshHistory()
	{
		guard(WDlgFindReplace::RefreshHistory);

		FindCombo.Empty();
		for (int x = 0; x < GFindHistory.Num(); x++)
		{
			FindCombo.AddString(*(GFindHistory(x)));
		}

		ReplaceCombo.Empty();
		for (int x = 0; x < GReplaceHistory.Num(); x++)
		{
			ReplaceCombo.AddString(*(GReplaceHistory(x)));
		}

		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgFindReplace::OnDestroy);
		WDialog::OnDestroy();
		::DestroyWindow(hWnd);
		unguard;
	}
	virtual void DoModeless()
	{
		guard(WDlgFindReplace::DoModeless);
		_Windows.AddItem(this);
		hWnd = CreateDialogParamA(hInstance, MAKEINTRESOURCEA(IDDIALOG_FINDREPLACE), OwnerWindow ? OwnerWindow->hWnd : NULL, (DLGPROC)StaticDlgProc, (LPARAM)this);
		if (!hWnd)
			appGetLastError();
		Show(TRUE);
		unguard;
	}
	void GetUserInput()
	{
		guard(WDlgFindReplace::GetUserInput);
		GSearchText = FindCombo.GetText();
		GMatchCase = MatchCaseCheck.IsChecked();
		GWholeWord = WholeWordCheck.IsChecked();
		unguard;
	}
	void OnFind()
	{
		guard(WDlgFindReplace::OnFind);

		GetUserInput();

		CHARRANGE range;
		SendMessageW(EditHwnd, EM_EXGETSEL, 0, (LPARAM)&range);

		if (range.cpMin == range.cpMax)
			range.cpMax = -1;

		if (DoFindText(range.cpMin, range.cpMax, FR_DOWN, &range))
		{
			SendMessageW(EditHwnd, EM_EXSETSEL, 0, (LPARAM)&range);
			SendMessageW(EditHwnd, EM_SCROLLCARET, 0, 0);

			Show(0);
		}
		else
			appMsgf(TEXT("Text not found"));

		unguard;
	}
	void OnFindNext()
	{
		guard(WDlgFindReplace::OnFindNext);

		GetUserInput();

		CHARRANGE range;
		SendMessageW(EditHwnd, EM_EXGETSEL, 0, (LPARAM)&range);

		if (DoFindText(range.cpMax, -1, FR_DOWN, &range) || DoFindText(0, -1, FR_DOWN, &range))
		{
			SendMessageW(EditHwnd, EM_EXSETSEL, 0, (LPARAM)&range);
			SendMessageW(EditHwnd, EM_SCROLLCARET, 0, 0);
		}
		else 
			appMsgf(TEXT("Text not found"));

		unguard;
	}
	void OnFindPrev()
	{
		guard(WDlgFindReplace::OnFindPrev);

		CHARRANGE range;
		SendMessageW(EditHwnd, EM_EXGETSEL, 0, (LPARAM)&range);

		if (!DoFindText(range.cpMin, -1, 0, &range))
		{
			GETTEXTLENGTHEX lngth;
			lngth.flags = GTL_DEFAULT;
			lngth.codepage = 1200;
			INT len = SendMessageW(EditHwnd, EM_GETTEXTLENGTHEX, (WPARAM)&lngth, NULL);
			if (!DoFindText(len, -1, 0, &range))
			{
				appMsgf(TEXT("Text not found"));
				return;
			}
		}
		SendMessageW(EditHwnd, EM_EXSETSEL, 0, (LPARAM)&range);
		SendMessageW(EditHwnd, EM_SCROLLCARET, 0, 0);
		unguard;
	}
	UBOOL DoFindText(int Start, int End, int Flags, CHARRANGE* Range)
	{
		guard(WDlgFindReplace::DoFindText);

		Flags |= (GMatchCase ? FR_MATCHCASE : 0) | (GWholeWord ? FR_WHOLEWORD : 0);
		FINDTEXTEXW ft;
		ft.chrg.cpMin = Start;
		ft.chrg.cpMax = End;
		ft.lpstrText = *GSearchText;
		int Loc;
		check(::IsWindow(EditHwnd));
#if UNICODE
		if(Flags & FR_DOWN)
			Loc = SendMessageW(EditHwnd, EM_FINDTEXTEXW, Flags, (LPARAM)&ft);
		else
		{
			Flags |= FR_DOWN;

			// Marco: UGLY hack, why isn't FR_DOWN flag working???
			int BestMatch = INDEX_NONE;
			for (INT pass = (Start < 1000 ? 1 : 0); pass < 2; ++pass)
			{
				ft.chrg.cpMin = pass==0 ? (Start - 1000) : 0;
				ft.chrg.cpMax = Start;

				while (1)
				{
					Loc = SendMessageW(EditHwnd, EM_FINDTEXTEXW, Flags, (LPARAM)&ft);
					if (Loc == -1)
						break;
					BestMatch = Loc;
					ft.chrg.cpMin = Loc + GSearchText.Len();
				}
				if (BestMatch >= 0)
					break;
			}
			Loc = BestMatch;
		}
#else
		Loc = SendMessageW(EditHwnd, EM_FINDTEXTEX, Flags, (LPARAM)&ft);
#endif

		Range->cpMin = Loc;
		Range->cpMax = (Loc == -1) ? 0 : Loc + GSearchText.Len();

		return !(Range->cpMin == -1);
		unguard;
	}
	void OnReplace()
	{
		guard(WDlgFindReplace::OnReplace);

		/*
		FString ReplaceString = ReplaceCombo.GetText();
		GSearchText = FindCombo.GetText();

		if( GSearchText.Len() )
		{
			UpdateHistory( GSearchText, ReplaceString );

			CHARRANGE range;
			SendMessageX( EditHwnd, EM_EXGETSEL, 0, (LPARAM)&range );

			if( range.cpMin != range.cpMax )
			{
				FString OldText = GCodeFrame->Edit.GetText(1);
				FString NewText = OldText.Left(range.cpMin) + ReplaceString + OldText.Right( OldText.Len() - range.cpMax );
				GCodeFrame->Edit.SetText( *NewText );
			}

			OnFindNext();
		}
		*/

		unguard;
	}
	void OnReplaceAll()
	{
		guard(WDlgFindReplace::OnReplaceAll);
		unguard;
	}
	void OnCloseButton()
	{
		guard(WCodeFrame::OnCloseButton);
		Show(0);
		unguard;
	}
	/*
	void OnGoLineNUm()
	{
		guard(WDlgFindReplace::OnGoLineNUm);
		GCodeFrame->ScrollToLine( appAtoi( *(LineNumEdit.GetText()) ) );
		Show( FALSE );
		unguard;
	}
	*/
};

extern void ParseStringToArray(const TCHAR* pchDelim, FString String, TArray<FString>* _pArray);
extern WBrowserMaster* GBrowserMaster;

#define ID_CF_TOOLBAR	29001
TBBUTTON tbCFButtons[] = {
	{ 0, IDMN_CLOSE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 1, IDMN_CF_COMPILE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 2, IDMN_CF_COMPILE_ALL, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 3, ID_BrowserActor, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 4, IDMN_CF_FIND, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 5, IDMN_CF_FIND_PREV, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 6, IDMN_CF_FIND_NEXT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
};
struct {
	TCHAR ToolTip[64];
	int ID;
} ToolTips_CF[] = {
	TEXT("Close Script"), IDMN_CLOSE,
	TEXT("Compile Changed Scripts"), IDMN_CF_COMPILE,
	TEXT("Compile ALL Scripts"), IDMN_CF_COMPILE_ALL,
	TEXT("Actor Class Browser"), ID_BrowserActor,
	TEXT("Find"), IDMN_CF_FIND,
	TEXT("Find Previous"), IDMN_CF_FIND_PREV,
	TEXT("Find Next"), IDMN_CF_FIND_NEXT,
	NULL, 0
};

struct FFileLineEntry
{
	FString File;
	INT Line;

	FFileLineEntry(const FString& F, INT l)
		: File(F), Line(l)
	{}
};
class WErrorCombo : public WComboBox
{
	HBRUSH hbrushRed,hbrushGreen;
	TArray<FFileLineEntry> ErList;

public:
	UBOOL bWasError;

	WErrorCombo(WWindow* InOwner, INT InId = 0, WNDPROC InSuperProc = NULL)
		: WComboBox(InOwner, InId, InSuperProc)
	{
		hbrushRed = CreateSolidBrush(RGB(255, 0, 0));
		hbrushGreen = CreateSolidBrush(RGB(0, 255, 0));
	}
	~WErrorCombo()
	{
		MaybeDestroy();
	}
	void DoDestroy()
	{
		WComboBox::DoDestroy();
		DeleteObject(hbrushRed);
		DeleteObject(hbrushGreen);
	}
	LRESULT CallDefaultProc(UINT Message, WPARAM wParam, LPARAM lParam)
	{
		switch (Message)
		{
		case WM_CTLCOLORBTN:
		case WM_CTLCOLORSTATIC:
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORMSGBOX:
		{
			HDC hdc = (HDC)wParam;
			SetTextColor(hdc, RGB(0, 0, 255)); //change text color
			SetBkMode(hdc, TRANSPARENT); //change text-background color, or set it to transparent
			return (INT_PTR)(bWasError ? hbrushRed : hbrushGreen);
		}
		default:
			return WComboBox::CallDefaultProc(Message, wParam, lParam);
		}
	}
	void Clear()
	{
		ErList.Empty();
		Empty();
	}
	void AddLine(const FString& Err, const FString& File, INT Line)
	{
		AddString(*Err);
		new (ErList) FFileLineEntry(File, Line);
	}
	const TCHAR* GetCurFile(INT& iLine)
	{
		INT ix = GetCurrent();
		if (ErList.IsValidIndex(ix))
		{
			iLine = ErList(ix).Line;
			return *ErList(ix).File;
		}
		return NULL;
	}
};

// A code editing window.
class WCodeFrame : public WWindow
{
	DECLARE_WINDOWCLASS(WCodeFrame, WWindow, UnrealEd)

	// Variables.
	UClass* Class {};
	WRichEdit Edit;
	WListBox FilesList;
	BOOL bFirstTime;
	RECT rcStatus{};
	HWND hWndToolBar{};
	WToolTip* ToolTipCtrl{};
	WDlgFindReplace* DlgFindReplace;
	HBITMAP ToolbarImage{};
	WErrorCombo StatusCombo;

	TArray<UClass*> m_Classes;	// the list of classes we are editing scripts for
	UClass* pCurrentClass;

	// Constructor.
	WCodeFrame(FName InPersistentName, WWindow* InOwnerWindow)
		: WWindow(InPersistentName, InOwnerWindow)
		, Edit(this)
		, FilesList(this, IDLB_FILES)
		, StatusCombo(this)
	{
		pCurrentClass = NULL;
		bFirstTime = TRUE;
		rcStatus.top = rcStatus.bottom = rcStatus.left = rcStatus.right = 0;
		StatusCombo.SelectionChangeDelegate = FDelegate(this, (TDelegate)&WCodeFrame::OnComboChange);
	}

	// WWindow interface.
	void OnSetFocus(HWND hWndLoser)
	{
		guard(WCodeFrame::OnSetFocus);
		WWindow::OnSetFocus(hWndLoser);
		SetFocus(Edit);
		unguard;
	}
	void OnSize(DWORD Flags, INT NewX, INT NewY)
	{
		guard(WCodeFrame::OnSize);
		WWindow::OnSize(Flags, NewX, NewY);
		PositionChildControls();
		unguard;
	}
	void PositionChildControls(void)
	{
		guard(WCodeFrame::PositionChildControls);

		if (!::IsWindow(GetDlgItem(hWnd, ID_CF_TOOLBAR)))	return;

		FRect CR = GetClientRect();
		::MoveWindow(hWndToolBar, 4, 0, 10 * MulDiv(16, DPIX, 96), 10 * MulDiv(16, DPIY, 96), 1);
		RECT R;
		::GetWindowRect(GetDlgItem(hWnd, ID_CF_TOOLBAR), &R);
		::MoveWindow(GetDlgItem(hWnd, ID_CF_TOOLBAR), 0, 0, CR.Max.X, R.bottom, TRUE);

		FilesList.MoveWindow(FRect(0, (R.bottom - R.top) - 1, 200, CR.Max.Y), TRUE);

		const INT StatusComboSize = MulDiv(20, DPIX, 96) + 1;
		Edit.MoveWindow(FRect(200, (R.bottom - R.top) - 1, CR.Max.X, CR.Max.Y - StatusComboSize), TRUE);

		//warren Edit.ScrollCaret();

		rcStatus.left = 200;
		rcStatus.right = CR.Max.X;
		rcStatus.top = CR.Max.Y - StatusComboSize;
		rcStatus.bottom = CR.Max.Y;
		StatusCombo.MoveWindow(rcStatus, TRUE);

		::InvalidateRect(hWnd, NULL, TRUE);

		unguard;
	}
	void OnPaint()
	{
		guard(WCodeFrame::OnPaint);
		unguard;
	}
	// Checks for script compile errors.
	//
	inline const TCHAR* GetErName(FName E) const
	{
		if (E == NAME_Error)
			return TEXT("Error");
		if (E == NAME_Critical)
			return TEXT("CriticalError");
		if (E == NAME_Warning)
			return TEXT("Warning");
		return TEXT("Info");
	}
	void ProcessResults(void)
	{
		guard(WCodeFrame::ProcessResults);

		StatusCombo.Clear();
		UBOOL bWasError = 0;
		INT i, n = 0;
		for (i = 0; i < GEditor->CompilerError.Num(); ++i)
		{
			FCompileError& E = GEditor->CompilerError(i);
			if (E.MsgType == NAME_Error || E.MsgType == NAME_Critical)
			{
				bWasError = 1;
				++n;
				StatusCombo.AddLine(FString::Printf(TEXT("Error in %ls (%i): %ls"), *E.File, E.Line, *E.ErrorMsg), E.File, E.Line);
			}
			if (E.MsgType == NAME_Log && (!bWasError || n > 1))
			{
				++n;
				StatusCombo.AddString(*E.ErrorMsg);
			}
		}
		StatusCombo.SetCurrent(n - 1);
		StatusCombo.bWasError = bWasError;

		if (bWasError)
		{
			for (i = 0; i < GEditor->CompilerError.Num(); ++i)
			{
				FCompileError& E = GEditor->CompilerError(i);
				if (E.MsgType == NAME_Error || E.MsgType == NAME_Critical)
				{
					HighlightError(*E.File, E.Line);
					break;
				}
			}
		}

		unguard;
	}
	void OnComboChange()
	{
		INT Line;
		const TCHAR* F = StatusCombo.GetCurFile(Line);
		if (F)
			HighlightError(F, Line);
	}

	// Highlights a compilation error by opening up that classes script and moving to the appropriate line.
	//
	void HighlightError(const TCHAR* Name, int Line)
	{
		guard(WCodeFrame::HighlightError);

		UClass* Class = FindObject<UClass>(NULL, Name, 1);
		if (Class && Class->ScriptText)
		{
			// Figure out where in the script the error line is, in chars.
			//
			const ANSICHAR* pch = appToAnsi(*(Class->ScriptText->Text));
			int iChar = 0, iLine = 1;

			while (*pch && iLine < Line)
			{
				if (*pch == '\n')
					iLine++;

				iChar++;
				pch++;
			}

			AddClass(Class, iChar, Line - 1);
		}
		unguard;
	}
	void OnCommand(INT Command)
	{
		guard(WCodeFrame::OnCommand);
		switch (Command) {

		case IDMN_CF_FIND:
			DlgFindReplace->Show(1);
			break;

		case IDMN_CF_FIND_NEXT:
			if (GSearchText.Len())
				DlgFindReplace->OnFindNext();
			else
				DlgFindReplace->Show(1);
			break;

		case IDMN_CF_FIND_PREV:
			if (GSearchText.Len())
				DlgFindReplace->OnFindPrev();
			else
				DlgFindReplace->Show(1);
			break;

		case IDMN_CF_EXPORT_CHANGED:
		{
			if (::MessageBox(hWnd, TEXT("This option will export all modified classes to text .uc files which can later be rebuilt. Do you want to do this?"), TEXT("Export classes to *.uc files"), MB_YESNO) == IDYES)
				GEditor->Exec(TEXT("CLASS SPEW"));
		}
		break;

		case IDMN_CF_EXPORT_ALL:
		{
			if (::MessageBox(hWnd, TEXT("This option will export all classes to text .uc files which can later be rebuilt. Do you want to do this?"), TEXT("Export classes to *.uc files"), MB_YESNO) == IDYES)
				GEditor->Exec(TEXT("CLASS SPEW ALL"));
		}
		break;

		case IDMN_CF_COMPILE:
		{
			GWarn->BeginSlowTask(TEXT("Compiling changed scripts"), 1, 0);
			Save();
			GEditor->Exec(TEXT("SCRIPT MAKE"));
			GWarn->EndSlowTask();

			ProcessResults();
		}
		break;

		case IDMN_CF_COMPILE_ALL:
		{
			TCHAR l_chMsg[256];
			appSprintf(l_chMsg, TEXT("Do you really want to compile all scripts?"));
			if (::MessageBox(hWnd, l_chMsg, TEXT("Compile all?"), MB_YESNO) == IDYES)
			{
				GWarn->BeginSlowTask(TEXT("Compiling all scripts"), 1, 0);
				Save();
				GEditor->Exec(TEXT("SCRIPT MAKE ALL"));
				GWarn->EndSlowTask();
				ProcessResults();
			}
		}
		break;

		case IDMN_CLOSE:
		{
			Save();

			// Find the currently selected class and remove it from the list.
			//
			FString Name = FilesList.GetString(FilesList.GetCurrent());
			RemoveClass(Name);
		}
		break;

		case ID_BrowserActor:
		{
			GBrowserMaster->ShowBrowser(eBROWSER_ACTOR);
		}
		break;

		default:
			WWindow::OnCommand(Command);
			break;
		}
		unguard;
	}
	void OnCreate()
	{
		guard(WCodeFrame::OnCreate);
		WWindow::OnCreate();

		SetMenu(hWnd, LoadMenuW(hInstance, MAKEINTRESOURCEW(IDMENU_CodeFrame)));

		// Load windows last position.
		//
		int X, Y, W, H;

		if (!GConfig->GetInt(*PersistentName, TEXT("X"), X, GUEdIni))	X = 0;
		if (!GConfig->GetInt(*PersistentName, TEXT("Y"), Y, GUEdIni))	Y = 0;
		if (!GConfig->GetInt(*PersistentName, TEXT("W"), W, GUEdIni))	W = 640;
		if (!GConfig->GetInt(*PersistentName, TEXT("H"), H, GUEdIni))	H = 480;

		if (!W) W = 320;
		if (!H) H = 200;

		::MoveWindow(hWnd, X, Y, W, H, TRUE);

		// Set up the main edit control.
		//
		Edit.OpenWindow(1, 0);
		UINT Tabs[16];
		for (INT i = 0; i < 16; i++)
			Tabs[i] = 4 * (i + 1);
		//Tabs[i]=5*4*(i+1);
		SendMessageW(Edit.hWnd, EM_SETTABSTOPS, 16, (LPARAM)Tabs);
		Edit.SetFont((HFONT)GetStockObject(ANSI_FIXED_FONT));
		SendMessageW( Edit.hWnd, EM_EXLIMITTEXT, 0, 262144 );
		Edit.SetText(TEXT(""));
		SendMessageW(Edit.hWnd, EM_SETTEXTMODE, 0, TM_RICHTEXT | TM_MULTILEVELUNDO);
		SendMessageW(Edit.hWnd, EM_SETBKGNDCOLOR, 0, (LPARAM)(COLORREF)RGB(0, 0, 64));

		Edit.SetReadOnly(TRUE);

		ToolbarImage = reinterpret_cast<HBITMAP>(LoadImage(hInstance,
			MAKEINTRESOURCE(IDB_CodeFrame_TOOLBAR),
			IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS));
		check(ToolbarImage);
		ScaleImageAndReplace(ToolbarImage);

		hWndToolBar = CreateToolbarEx(
			hWnd, WS_CHILD | WS_VISIBLE | CCS_ADJUSTABLE,
			ID_CF_TOOLBAR,
			8,
			NULL,
			reinterpret_cast<PTRINT>(ToolbarImage),
			(LPCTBBUTTON)&tbCFButtons,
			10,
			MulDiv(16, DPIX, 96), MulDiv(16, DPIY, 96),
			MulDiv(16, DPIX, 96), MulDiv(16, DPIY, 96),
			sizeof(TBBUTTON));

		if (!hWndToolBar)
			appMsgf(TEXT("Toolbar not created!"));


		ToolTipCtrl = new WToolTip(this);
		ToolTipCtrl->OpenWindow();

		for (int tooltip = 0; ToolTips_CF[tooltip].ID > 0; tooltip++)
		{
			// Figure out the rectangle for the toolbar button.
			int index = SendMessageW(hWndToolBar, TB_COMMANDTOINDEX, ToolTips_CF[tooltip].ID, 0);
			RECT rect;
			SendMessageW(hWndToolBar, TB_GETITEMRECT, index, (LPARAM)&rect);

			ToolTipCtrl->AddTool(hWndToolBar, ToolTips_CF[tooltip].ToolTip, tooltip, &rect);
		}

		FilesList.OpenWindow(1, 0, 0, 0, 1);
		FilesList.DoubleClickDelegate = FDelegate(this, (TDelegate)&WCodeFrame::OnFilesListDblClick);

		DlgFindReplace = new WDlgFindReplace(NULL, this);
		DlgFindReplace->EditHwnd = Edit.hWnd;
		DlgFindReplace->DoModeless();
		DlgFindReplace->Show(0);

		StatusCombo.OpenWindow(1, 0, CBS_DROPDOWNLIST);
		StatusCombo.AddString(TEXT("Ready."));
		StatusCombo.SetCurrent(0);
		StatusCombo.bWasError = 0;

		unguard;
	}
	DLL_EXPORT TCHAR* MY_ANSI_TO_TCHAR(char* str)
	{
		int iLength = winGetSizeUNICODE(str);
		TCHAR* pBuffer = new TCHAR[iLength];
		appStrcpy(pBuffer, TEXT(""));
		TCHAR* ret = winToUNICODE(pBuffer, str, iLength);
		return ret;
	}
	void Save(void)
	{
		guard(WCodeFrame::Save);

		if (!pCurrentClass || !pCurrentClass->ScriptText)	return;
		if (!m_Classes.Num())	return;

		int iLength = SendMessageA(Edit.hWnd, WM_GETTEXTLENGTH, 0, 0);
		if (iLength > 0)
		{
			char* pchBuffer = new char[iLength + 2];
			::strcpy_s(pchBuffer, iLength, "");
			Edit.StreamTextOut(pchBuffer, iLength);

			// The script compiler wants a newline at the end of the file...
			if (pchBuffer[iLength - 1] != '\n' &&
				pchBuffer[iLength - 1] != '\r')
			{
				strcat_s(pchBuffer, iLength + 2, "\n");
			}

			const TCHAR* unibuf = appFromAnsi(pchBuffer);
			delete[] pchBuffer;
			if (appStrCrc(*pCurrentClass->ScriptText->Text) != appStrCrc(unibuf))
			{
				pCurrentClass->MarkAsDirty();
				pCurrentClass->ScriptText->Text = unibuf;
			}
			SendMessageW(Edit.hWnd, EM_GETSEL, (WPARAM) & (pCurrentClass->ScriptText->Pos), 0);
			pCurrentClass->ScriptText->Top = SendMessageW(Edit.hWnd, EM_GETFIRSTVISIBLELINE, 0, 0);
		}

		unguard;
	}
	void OnDestroy()
	{
		guard(WCodeFrame::OnDestroy);

		//delete DlgFindReplace; <- Marco: commented out as this crashed upon exit.

		Save();

		// Save Window position (base class doesn't always do this properly)
		// (Don't do this if the window is minimized.)
		//
		if (!::IsIconic(hWnd) && !::IsZoomed(hWnd))
		{
			RECT R;
			::GetWindowRect(hWnd, &R);

			GConfig->SetInt(*PersistentName, TEXT("Active"), m_bShow, GUEdIni);
			GConfig->SetInt(*PersistentName, TEXT("X"), R.left, GUEdIni);
			GConfig->SetInt(*PersistentName, TEXT("Y"), R.top, GUEdIni);
			GConfig->SetInt(*PersistentName, TEXT("W"), R.right - R.left, GUEdIni);
			GConfig->SetInt(*PersistentName, TEXT("H"), R.bottom - R.top, GUEdIni);
		}

		::DestroyWindow(hWndToolBar);
		delete ToolTipCtrl;

		if (ToolbarImage)
			DeleteObject(ToolbarImage);

		WWindow::OnDestroy();
		unguard;
	}
	void OpenWindow(UBOOL bMdi = 0, UBOOL AppWindow = 0)
	{
		guard(WCodeFrame::OpenWindow);
		MdiChild = bMdi;
		debugf(TEXT("WCodeFrame::OpenWindow - Creating Window - W: %d, H: %d"),
			MulDiv(480, DPIX, 96),
			MulDiv(640, DPIY, 96));
		PerformCreateWindowEx
		(
			WS_EX_WINDOWEDGE,
			TEXT("Script Editor"),
			WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_SIZEBOX | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			MulDiv(480, DPIX, 96),
			MulDiv(640, DPIY, 96),
			OwnerWindow->hWnd,
			NULL,
			hInstance
		);
		unguard;
	}
	void AddClass(UClass* pClass, int Pos = -1, int Top = -1)
	{
		guard(WCodeFrame::AddClass);
		if (!pClass) return;

		// Make sure this class has a script.
		//
		FStringOutputDevice GetPropResult;
		GEditor->Get(TEXT("SCRIPTPOS"), pClass->GetPathName(), GetPropResult);
		/*if (!GetPropResult.Len())
		{
			appMsgf(TEXT("'%ls' has no script to edit."), pClass->GetPathName());
			return;
		}*/

		// Only add this class to the list if it's not already there.
		//

		if (FilesList.FindExactString(pClass->GetPathName()) == -1)
			m_Classes.AddItem(pClass);

		RefreshScripts();
		SetClass(pClass->GetPathName(), Pos, Top);

		Show(1);
		::BringWindowToTop(hWnd);
		unguard;
	}
	void RemoveClass(FString Name)
	{
		guard(WCodeFrame::RemoveClass);

		// Remove the class from the internal list.
		//
		for (int x = 0; x < m_Classes.Num(); x++)
		{
			if (!appStrcmp(m_Classes(x)->GetName(), *Name))
			{
				m_Classes.Remove(x);
				break;
			}
			if (!appStrcmp(m_Classes(x)->GetPathName(), *Name))
			{
				m_Classes.Remove(x);
				break;
			}
		}

		RefreshScripts();
		FilesList.SetCurrent(0, 1);
		OnFilesListDblClick();

		if (!m_Classes.Num())
			pCurrentClass = NULL;
		SetCaption();

		unguard;
	}
	void RefreshScripts(void)
	{
		guard(WCodeFrame::RefreshScripts);

		// LOADED SCRIPTS
		//
		FilesList.Empty();
		for (int x = 0; x < m_Classes.Num(); x++)
			FilesList.AddString(m_Classes(x)->GetPathName());

		unguard;
	}
	// Saves the current script and selects a new script.
	//
	void SetClass(FString Name, int Pos = -1, int Top = -1)
	{
		guard(WCodeFrame::SetClass);

		// If there are no classes loaded, just empty the edit control.
		//
		if (!m_Classes.Num())
		{
			Edit.SetReadOnly(TRUE);
			Edit.SetText(TEXT("No scripts loaded."));
			pCurrentClass = NULL;
			return;
		}

		// Save the settings/script for the current class before changing.
		//
		if (pCurrentClass && !bFirstTime)
		{
			Save();
		}

		bFirstTime = FALSE;

		FilesList.SetCurrent(FilesList.FindString(*Name), 1);

		Edit.SetReadOnly(FALSE);

		// Locate the proper class pointer.
		//
		for (int x = 0; x < m_Classes.Num(); x++)
			if (!appStrcmp(m_Classes(x)->GetPathName(), *Name))
			{
				pCurrentClass = m_Classes(x);
				break;
			}
		if (!pCurrentClass)
			for (int x = 0; x < m_Classes.Num(); x++)
				if (!appStrcmp(m_Classes(x)->GetName(), *Name))
				{
					pCurrentClass = m_Classes(x);
					break;
				}
		if (!pCurrentClass)
			return;

		// Override whatever is in the class if we need to.
		//
		if (pCurrentClass->ScriptText)
		{
			if (Pos > -1)		pCurrentClass->ScriptText->Pos = Pos;
			if (Top > -1)		pCurrentClass->ScriptText->Top = Top;
		}

		// Load current script into edit window.
		//
		SetCaption();

		// old code
		//Edit.SetText( *(pCurrentClass->ScriptText->Text) );

		// Get the script text in RTF format
		FStringOutputDevice GetPropResult = FStringOutputDevice();
		GEditor->Get(TEXT("RTF"), pCurrentClass->GetPathName(), GetPropResult);

		// Convert it to ANSI
		FString ScriptText = *GetPropResult;
		// stijn: Holy shit. This was a TCHAR_TO_ANSI before. This is totally unsafe because of two reasons:
		// 1) If we compile without unicode support, this effectively makes chScriptText and tchScriptText alias the same allocation.
		// This allocation gets freed below but the editor window still uses the memory block and does not free it until we save/exit.
		// 2) tchScriptText can be very long and can easily result in a very large (overflowing!) appAlloca
		char* AnsiScriptText = appToAnsi(*ScriptText);

		// Stream it into the RichEdit control
		LockWindowUpdate(Edit.hWnd);
		Edit.StreamTextIn(AnsiScriptText, static_cast<int>(strlen(AnsiScriptText)));
		LockWindowUpdate(NULL);

		SendMessageW(Edit.hWnd, EM_SETSEL, pCurrentClass->ScriptText ? pCurrentClass->ScriptText->Pos : 0, pCurrentClass->ScriptText ? pCurrentClass->ScriptText->Pos : 0);
		SendMessageW(Edit.hWnd, EM_SCROLLCARET, 0, 0);

		ScrollToLine(pCurrentClass->ScriptText ? pCurrentClass->ScriptText->Top : 0);

		::SetFocus(Edit.hWnd);
		unguard;
	}
	void SetCaption()
	{
		if (pCurrentClass)
			SetText(pCurrentClass->GetFullName());
		else
			SetText(TEXT(""));
	}
	void ScrollToLine(int Line)
	{
		guard(WCodeFrame::ScrollToLine);

		// Stop the window from updating while scrolling to the requested line.  This makes
		// it go MUCH faster -- and it looks better.
		//
		LockWindowUpdate(hWnd);

		int CurTop = SendMessageW(Edit.hWnd, EM_GETFIRSTVISIBLELINE, 0, 0);
		while (CurTop > Line)
		{
			SendMessageW(Edit.hWnd, EM_SCROLL, SB_LINEUP, 0);
			CurTop--;
		}
		while (CurTop < Line)
		{
			SendMessageW(Edit.hWnd, EM_SCROLL, SB_LINEDOWN, 0);
			CurTop++;
		}

		LockWindowUpdate(NULL);
		unguard;
	}
	int OnSysCommand(INT Command)
	{
		guard(WCodeFrame::OnSysCommand);
		// Don't actually close the window when the user hits the "X" button.  Just hide it.
		if (Command == SC_CLOSE)
		{
			Show(0);
			return 1;
		}

		return 0;
		unguard;
	}
	// Notification delegates for child controls.
	//
	void OnFilesListDblClick()
	{
		guard(WCodeFrame::OnFilesListDblClick);
		FString Name = FilesList.GetString(FilesList.GetCurrent());
		SetClass(Name);
		SetCaption();
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
