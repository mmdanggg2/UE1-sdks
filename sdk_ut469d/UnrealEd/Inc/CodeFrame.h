/*=============================================================================
	CodeFrame : This window is where all UnrealScript editing takes place
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#pragma once

#include "AkelEdit.h"
#include "StrFunc.h"
#include "StackFunc.h"
#include "RegExpFunc.h"

//Include AEC functions
#define AEC_FUNCTIONS
#include "AkelEdit.h"

//Include string functions
#define WideCharUpper
#define WideCharLower
#define xmemset
#define xmemcpy
#define xstrlenW
#define xstrcpyW
#define xstrcmpnW
#define xstrcmpinW
#define xatoiW
#define hex2decW
#include "StrFunc.h"

//Include stack functions
#define StackInsertBefore
#define StackDelete
#define StackJoin
#include "StackFunc.h"

#define RE_FUNCTIONS
#include "RegExpFunc.h"

enum EFRMode
{
	FR_Backward,
	FR_Forward,
	FR_Selection,
	FR_Everywhere
};

class WDlgFindReplace : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgFindReplace,WDialog,UnrealEd)

	// Variables.
	WButton FindButton;
	WButton FindNextButton;
	WButton FindPrevButton;
	WButton ReplaceButton;
	WButton ReplaceAllButton;
	WButton CancelButton;
	WButton CloseButton;
	WMRUComboBox FindCombo;
	WMRUComboBox ReplaceCombo;
	WCheckBox MatchCaseCheck;
	WCheckBox WholeWordCheck;
	WCheckBox RegExpCheck;
	WCheckBox DotMatchNLCheck;

	FString GSearchText;
	FString GReplaceText;
	DWORD GSearchFlags = 0;

	HWND EditHwnd;

	// Constructor.
	WDlgFindReplace( UObject* InContext, WWindow* InOwnerWindow )
		: WDialog(TEXT("Find/Replace"), IDDIALOG_FINDREPLACE, InOwnerWindow)
		, FindButton(this, IDPB_FIND, FDelegate(this, (TDelegate)&WDlgFindReplace::OnFind))
		, FindNextButton(this, IDPB_FIND_NEXT, FDelegate(this, (TDelegate)&WDlgFindReplace::OnFindNext))
		, FindPrevButton(this, IDPB_FIND_PREV, FDelegate(this, (TDelegate)&WDlgFindReplace::OnFindPrev))
		, ReplaceButton(this, IDPB_REPLACE, FDelegate(this, (TDelegate)&WDlgFindReplace::OnReplace))
		, ReplaceAllButton(this, IDPB_REPLACE_ALL, FDelegate(this, (TDelegate)&WDlgFindReplace::OnReplaceAll))
		, CloseButton(this, IDPB_CLOSE, FDelegate(this, (TDelegate)&WDlgFindReplace::OnClose))
		, FindCombo(this, TEXT("History"), TEXT("Find%d"), 16, IDCB_FIND)
		, ReplaceCombo(this, TEXT("History"), TEXT("Replace%d"), 16, IDCB_REPLACE)
		, MatchCaseCheck(this, IDCK_MATCH_CASE)
		, WholeWordCheck(this, IDCK_WHOLE_WORD)
		, RegExpCheck(this, IDCK_REG_EXP, FDelegate(this, (TDelegate)&WDlgFindReplace::OnRegExpChanged))
		, DotMatchNLCheck(this, IDCK_DOT_MATCH_NL)
		, EditHwnd(NULL)
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgFindReplace::OnInitDialog);
		WDialog::OnInitDialog();

		MatchCaseCheck.SetCheck( GSearchFlags & AEFR_MATCHCASE ? BST_CHECKED : BST_UNCHECKED );
		WholeWordCheck.SetCheck( GSearchFlags & AEFR_WHOLEWORD ? BST_CHECKED : BST_UNCHECKED );
		RegExpCheck.SetCheck( GSearchFlags & AEFR_REGEXP ? BST_CHECKED : BST_UNCHECKED );
		DotMatchNLCheck.SetCheck( !(GSearchFlags & AEFR_REGEXPNONEWLINEDOT) ? BST_CHECKED : BST_UNCHECKED );

		FindCombo.ReadMRU();
		ReplaceCombo.ReadMRU();

		OnRegExpChanged();

		unguard;
	}
	void FillFromSelection()
	{
		guard(WDlgFindReplace::FillFromSelection);
		// If there is text selected, pull it in and make it the default text to search for.
		CHARRANGE range;
		SendMessageW( EditHwnd, EM_EXGETSEL, 0, (LPARAM)&range );

		if( range.cpMax - range.cpMin )
		{
			TCHAR Text[256] = TEXT("");
			if (range.cpMax - range.cpMin > 250 || range.cpMax == -1)
				range.cpMax = range.cpMin + 250;

			TEXTRANGEA txtrange;
			txtrange.chrg = range;
			txtrange.lpstrText = (CHAR*)&Text;
			SendMessageW( EditHwnd, EM_GETTEXTRANGE, 0, (LPARAM)&txtrange );

			FindCombo.SetText( Text );
		}
		unguard;
	}
	void OnShowWindow(UBOOL bShow)
	{
		guard(WDlgFindReplace::OnShowWindow);
		WDialog::OnShowWindow(bShow);
		if (bShow)
		{
			FillFromSelection();

			::SetFocus( FindCombo.hWnd );
		}
		unguard;
	}
	void UpdateHistory( FString Find, FString Replace )
	{
		FindCombo.AddMRU(Find);
		ReplaceCombo.AddMRU(Replace);
	}
	void OnDestroy()
	{
		guard(WDlgFindReplace::OnDestroy);
		WDialog::OnDestroy();
		::DestroyWindow( hWnd );
		unguard;
	}
	virtual void DoModeless()
	{
		guard(WDlgFindReplace::DoModeless);
		AddWindow(this, TEXT("WDlgFindReplace"));
		hWnd = CreateDialogParamA( hInstance, MAKEINTRESOURCEA(IDDIALOG_FINDREPLACE), OwnerWindow?OwnerWindow->hWnd:NULL, (DLGPROC)StaticDlgProc, (LPARAM)this);
		if( !hWnd )
			appGetLastError();
		Show( TRUE );
		unguard;
	}
	void GetUserInput()
	{
		guard(WDlgFindReplace::GetUserInput);
		GSearchText = FindCombo.GetText();
		GReplaceText = ReplaceCombo.GetText();
		GSearchFlags = 0;
		if (MatchCaseCheck.IsChecked())
			GSearchFlags |= AEFR_MATCHCASE;
		if (WholeWordCheck.IsChecked())
			GSearchFlags |= AEFR_WHOLEWORD;
		if (RegExpCheck.IsChecked())
			GSearchFlags |= AEFR_REGEXP | AEFR_REGEXPMINMATCH;
		if ((GSearchFlags & AEFR_REGEXP) && !DotMatchNLCheck.IsChecked())
			GSearchFlags |= AEFR_REGEXPNONEWLINEDOT;

		unguard;
	}
	void OnCommand( INT Command )
	{
		guard(WDlgFindReplace::OnCommand);
		switch( Command ) {
			case IDPB_CLOSE:
				OnClose();
				break;

			default:
				WDialog::OnCommand(Command);
				break;
		}
		unguard;
	}
	void OnRegExpChanged()
	{
		guard(WDlgFindReplace::OnRegExpChanged);
		EnableWindow(DotMatchNLCheck.hWnd, RegExpCheck.IsChecked());
		unguard;
	}
	void OnFind()
	{
		guard(WDlgFindReplace::OnFind);

		GetUserInput();

		CHARRANGE range;
		SendMessageW( EditHwnd, EM_EXGETSEL, 0, (LPARAM)&range );

		if (range.cpMin <= range.cpMax && range.cpMax - range.cpMin <= GSearchText.Len())
		{
			range.cpMin = 0;
			range.cpMax = -1;
		}
		while (true)
		{
			BOOL bInSelection = range.cpMax != -1;

			UBOOL Found = SearchText(bInSelection ? FR_Selection : FR_Everywhere);
			if (Found == TRUE)
				Show(0);
			else if (Found != FALSE)
				/* empty */;
			else if (bInSelection)
			{
				INT MessageID = ::MessageBox(hWnd, TEXT("The text was not found in the selection.\r\nSearch everywhere?"), TEXT("UnrealEd"), MB_YESNO);
				if (MessageID == IDYES)
				{
					range.cpMin = 0;
					range.cpMax = -1;
					continue;
				}
			}
			else
			{
				::MessageBox(m_bShow ? hWnd : EditHwnd, TEXT("Text not found"), TEXT("UnrealEd"), MB_OK);
				SetFocus(m_bShow ? FindCombo.hWnd : EditHwnd);
			}
			break;
		}

		UpdateHistory( GSearchText, GReplaceText );

		unguard;
	}
	void OnFindNext()
	{
		guard(WDlgFindReplace::OnFindNext);
		FindNext();
		unguard;
	}
	BOOL FindNext(BOOL bFromReplace = FALSE)
	{
		guard(WDlgFindReplace::FindNext);

		GetUserInput();

		BOOL bFound = SearchText(FR_Forward);
		if (bFound == FALSE && !bFromReplace)
		{
			::MessageBox(m_bShow ? hWnd : EditHwnd, TEXT("Text not found.\r\nThe end of the text has been reached."), TEXT("UnrealEd"), MB_OK);
			SetFocus(m_bShow ? FindCombo.hWnd : EditHwnd);
		}
		return bFound == TRUE;
		unguard;
	}
	void OnFindPrev()
	{
		guard(WDlgFindReplace::OnFindPrev);
		
		GetUserInput();

		if (SearchText(FR_Backward) == FALSE)
		{
			::MessageBox(m_bShow ? hWnd : EditHwnd, TEXT("Text not found.\r\nThe beginning of the text has been reached."), TEXT("UnrealEd"), MB_OK);
			SetFocus(m_bShow ? FindCombo.hWnd : EditHwnd);
		}
		unguard;
	}
	UBOOL SearchText(EFRMode Mode)
	{
		guard(WDlgFindReplace::SearchText);
		AEFINDTEXTW ft = {0};

		ft.dwFlags = GSearchFlags;
		if (Mode != FR_Backward)
			ft.dwFlags |= AEFR_DOWN;
		ft.pText = *GSearchText;
		ft.dwTextLen = (UINT_PTR)-1;
		ft.nNewLine = AELB_ASIS;
		WPARAM From, To;
		switch (Mode)
		{	
			case FR_Backward: From = AEGI_FIRSTCHAR; To = AEGI_FIRSTSELCHAR; break;
			default:
			case FR_Forward: From = AEGI_LASTSELCHAR; To = AEGI_LASTCHAR; break;
			case FR_Selection: From = AEGI_FIRSTSELCHAR; To = AEGI_LASTSELCHAR; break;
			case FR_Everywhere: From = AEGI_FIRSTCHAR; To = AEGI_LASTCHAR; break;
		}
		SendMessage(EditHwnd, AEM_GETINDEX, From, (LPARAM)&ft.crSearch.ciMin);
		SendMessage(EditHwnd, AEM_GETINDEX, To, (LPARAM)&ft.crSearch.ciMax);

		BOOL bFound = SendMessage(EditHwnd, AEM_FINDTEXTW, 0, (LPARAM)&ft);
		if (ft.nCompileErrorOffset)
		{
			Show(1);
			::MessageBox(m_bShow ? hWnd : EditHwnd, *(FString::Printf(TEXT("Failed compile REGEX '%ls', error on pos %d at '%ls'"), 
				ft.pText, ft.nCompileErrorOffset, &ft.pText[ft.nCompileErrorOffset - 1])), TEXT("UnrealEd"), MB_OK);
			SetFocus(FindCombo.hWnd);
			SendMessage(FindCombo.hWnd, CB_SETEDITSEL, 0, (ft.nCompileErrorOffset - 1) | (ft.nCompileErrorOffset << 16));
			return -1;
		}
		else if (bFound)
		{
			AESELECTION aes = {ft.crFound};
			SendMessage(EditHwnd, AEM_SETSEL, (WPARAM)NULL, (LPARAM)&aes);
			SendMessageW(EditHwnd, EM_SCROLLCARET, 0, 0);
		}

		return bFound;
		unguard;
	}
	void OnReplace()
	{
		guard(WDlgFindReplace::OnReplace);
		
		GetUserInput();

		if( GSearchText.Len() )
		{
			UpdateHistory( GSearchText, GReplaceText );

			volatile INT Start, End; // volatile for prevent compiler "optimize" out this variables
			SendMessage(EditHwnd, EM_GETSEL, (WPARAM)&Start, (LPARAM)&End);

			if (End > Start)
				SendMessage(EditHwnd, EM_SETSEL, Start, Start);

			if (FindNext())
			{
				ReplaceSelected();
				FindNext();
			}
		}

		unguard;
	}
	void ReplaceSelected()
	{
		guard(WDlgFindReplace::ReplaceSelected);
		
		if (!(GSearchFlags & AEFR_REGEXP))
			SendMessage(EditHwnd, EM_REPLACESELW, TRUE, (LPARAM)*GReplaceText);
		else
		{
			PATREPLACE pr = {0};

			SendMessage(EditHwnd, AEM_EXGETSEL, (WPARAM)&pr.ciStr, (LPARAM)&pr.ciMaxStr);
			pr.wpPat = *GSearchText;
			pr.wpMaxPat = pr.wpPat + lstrlenW(pr.wpPat);
			pr.wpRep = *GReplaceText;
			pr.wpMaxRep = pr.wpRep + lstrlenW(pr.wpRep);
			pr.dwOptions = RESE_ISMATCH;
			if (GSearchFlags & AEFR_WHOLEWORD)
				pr.dwOptions |= RESE_WHOLEWORD;
			if (GSearchFlags & AEFR_MATCHCASE)
				pr.dwOptions |= RESE_MATCHCASE;
			if (GSearchFlags & AEFR_REGEXPNONEWLINEDOT)
				pr.dwOptions |= RESE_NONEWLINEDOT;
			INT_PTR nResultTextLen = PatReplace(&pr);

			if (pr.nReplaceCount)
			{
				TArray<TCHAR> Buf;
				Buf.SetSize(nResultTextLen);
				pr.wszResult = &Buf(0);
				PatReplace(&pr);
				if (pr.wszResult)
					SendMessage(EditHwnd, EM_REPLACESELW, TRUE, (LPARAM)pr.wszResult);
			}
		}
		
		unguard;
	}
	void OnReplaceAll()
	{
		guard(WDlgFindReplace::OnReplaceAll);

		GetUserInput();

		if( GSearchText.Len() )
		{
			UpdateHistory( GSearchText, GReplaceText );

			SendMessage(EditHwnd, EM_SETSEL, 0, 0);

			INT Replaced = 0;
			while (FindNext(TRUE))
			{
				ReplaceSelected();
				Replaced++;
			}
			::MessageBox(m_bShow ? hWnd : EditHwnd, *(FString::Printf(TEXT("Changed %d occurrences"), Replaced)), TEXT("UnrealEd"), MB_OK);
		}
		
		unguard;
	}
	void OnClose()
	{
		guard(WCodeFrame::OnClose);
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

class SyntaxHighlighting
{
protected:
	HWND hWnd;
	AEHTHEME hTheme;

public:
	// Constructor.
	SyntaxHighlighting(HWND InhWnd)
		: hWnd(InhWnd)
	{
		hTheme = (AEHTHEME)SendMessage(hWnd, AEM_HLCREATETHEMEA, 0, (LPARAM)"UnrealScript");
		if (hTheme)
			Setup();
	}

protected:	
	void AddDelimiter(const TCHAR* Delimiter)
	{
		guard(SyntaxHighlighting::AddDelimiter);
		
		AEDELIMITEMW di = {0};
		
		di.nIndex = -1;
		di.pDelimiter = Delimiter;
		di.nDelimiterLen = lstrlenW(di.pDelimiter);
		di.dwFlags = AEHLF_MATCHCASE;
		di.dwFontStyle = AEHLS_NONE;
		di.crText = (DWORD)-1;
		di.crBk = (DWORD)-1;
		SendMessage(hWnd, AEM_HLADDDELIMITERW, (WPARAM)hTheme, (LPARAM)&di);

		unguard;
	}
	void AddKeyword(const TCHAR* Word)
	{
		guard(SyntaxHighlighting::AddWord);
		
		AEWORDITEMW wi = {0};
		
		wi.pWord = Word;
		wi.nWordLen = lstrlenW(wi.pWord);
		wi.dwFlags = 0;
		wi.dwFontStyle = AEHLS_NONE;
		wi.crText = RGB(0x00, 0xFF, 0xFF);
		wi.crBk = (DWORD)-1;
		SendMessage(hWnd, AEM_HLADDWORDW, (WPARAM)hTheme, (LPARAM)&wi);

		unguard;
	}
	void AddQuote(const TCHAR* Start, const TCHAR* End, COLORREF Color, const TCHAR Escape = '\0', DWORD Flags = 0, const TCHAR* Exclude = NULL)
	{
		guard(SyntaxHighlighting::AddQuote);
		
		AEQUOTEITEMW qi = {0};
		
		qi.nIndex = -1;
		qi.pQuoteStart = Start;
		qi.nQuoteStartLen = lstrlenW(qi.pQuoteStart);
		qi.pQuoteEnd = End;
		qi.nQuoteEndLen = lstrlenW(qi.pQuoteEnd);
		qi.chEscape = Escape;
		qi.pQuoteInclude = NULL;
		qi.nQuoteIncludeLen = 0;
		qi.pQuoteExclude = Exclude;
		qi.nQuoteExcludeLen = lstrlenW(qi.pQuoteExclude);
		qi.dwFlags = Flags;
		if (!(Flags & AEHLF_REGEXP))
			qi.dwFlags |= AEHLF_MATCHCASE | AEHLF_QUOTEEND_REQUIRED;
		if (qi.pQuoteExclude)
			qi.dwFlags |= AEHLF_QUOTEEXCLUDE;
		qi.dwFontStyle = AEHLS_NONE;
		qi.crText = Color;
		qi.crBk = (DWORD)-1;

		if (!SendMessage(hWnd, AEM_HLADDQUOTEW, (WPARAM)hTheme, (LPARAM)&qi) && qi.nCompileErrorOffset)
			debugf(TEXT("Failed compile quote REGEX pattern '%ls', error on pos %d at '%ls'"), 
				qi.pQuoteStart, qi.nCompileErrorOffset, &qi.pQuoteStart[qi.nCompileErrorOffset - 1]);

		unguard;
	}
	void AddFold()
	{
		guard(SyntaxHighlighting::AddFold);
		
		AEPOINT pointMin = {0};
		AEPOINT pointMax = {0};
		AEFOLD fold = {0};

		pointMin.nPointOffset=AEPTO_CALC;
		pointMax.nPointOffset=AEPTO_CALC;
		SendMessage(hWnd, AEM_EXGETSEL, (WPARAM)&pointMin.ciPoint, (LPARAM)&pointMax.ciPoint);
		
		fold.lpMinPoint = &pointMin;
		fold.lpMaxPoint = &pointMax;
		fold.dwFlags = 0;
		fold.dwFontStyle = AEHLS_NONE;
		fold.crText = RGB(0xFF, 0x00, 0x00);
		fold.crBk = (DWORD)-1;
		fold.dwUserData = 0;
		SendMessage(hWnd, AEM_ADDFOLD, 0, (LPARAM)&fold);

		unguard;
	}
	void AddMark(const TCHAR* Mark, COLORREF Color, DWORD Flags = 0)
	{
		guard(SyntaxHighlighting::AddMark);
		
		AEMARKTEXTITEMW mti = {0};
		mti.nIndex = -1;
		mti.pMarkText = Mark;
		mti.nMarkTextLen = lstrlenW(mti.pMarkText);
		mti.dwFlags = Flags;
		mti.dwFontStyle = AEHLS_NONE;
		mti.crText = Color;
		mti.crBk = (DWORD)-1;

		if (!SendMessage(hWnd, AEM_HLADDMARKTEXTW, (WPARAM)hTheme, (LPARAM)&mti) && mti.nCompileErrorOffset)
			debugf(TEXT("Failed compile mark REGEX pattern '%ls', error on pos %d at '%ls'"), 
				mti.pMarkText, mti.nCompileErrorOffset, &mti.pMarkText[mti.nCompileErrorOffset - 1]);

		unguard;
	}
	void Setup()
	{
		guard(SyntaxHighlighting::Setup);

		// exec directives
		AddQuote(TEXT("#"), NULL, RGB(0x80, 0x80, 0x80));
		// multiline-comments
		AddQuote(TEXT("/*"), TEXT("*/"), RGB(0x00, 0xFF, 0x0), '\0', AEHLF_QUOTEMULTILINE);
		//AddMark(TEXT("/\\*([\\s\\S]*)\\*/"), RGB(0x00, 0xFF, 0x0), AEHLF_REGEXP);
		// comments
		AddQuote(TEXT("//"), NULL, RGB(0x00, 0xFF, 0x0), ':');
		// strings
		AddQuote(TEXT("\""), TEXT("\""), RGB(0x00, 0xFF, 0x0), '\\');
		// names
		AddQuote(TEXT("'"), TEXT("'"), RGB(255, 169, 128), '\\');
		// labels
		AddMark(TEXT("^[ \t]*[a-zA-Z_][a-zA-Z_0-9]*(?<!\\bdefault):"), RGB(0xFF, 0xFF, 0x00), AEHLF_REGEXP);
		
		FString Delimiters = TEXT(" \t(){}.,:;!~|=-*+/<>&^%?[]@$");

		TCHAR Delimiter[2];
		Delimiter[1] = '\0';
		for( INT i = 0; i < Delimiters.Len(); i++ )
		{
			Delimiter[0] = Delimiters[i];
			AddDelimiter(Delimiter);
		}

		// Add the hardcoded names which are tagged for syntax highlighting.
		for( INT i = 0; i < FName::GetMaxNames(); i++ )
		{
			FNameEntry *Entry = FName::GetEntry(i);
			if( Entry && Entry->Flags & RF_HighlightedName )
				AddKeyword(Entry->Name);
		}

		SendMessage(hWnd, AEM_HLSETTHEME, (WPARAM)hTheme, TRUE);

		unguard;
	}
};

extern WBrowserMaster* GBrowserMaster;

#define ID_CF_TOOLBAR	29001
TBBUTTON tbCFButtons[] = {
	{ 0, IDMN_CLOSE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 1, IDMN_CF_COMPILE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 2, IDMN_CF_COMPILE_ALL, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 8, ID_FileSave, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 3, ID_BrowserActor, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 7, ID_ViewDefProp, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 4, ID_EditFind, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
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
	TEXT("Save Package of Current Script"), ID_FileSave,
	TEXT("Actor Class Browser"), ID_BrowserActor,
	TEXT("View Default Properties"), ID_ViewDefProp,
	TEXT("Find (Ctrl+F)"), ID_EditFind,
	TEXT("Find Previous (Shift+F3)"), IDMN_CF_FIND_PREV,
	TEXT("Find Next (F3)"), IDMN_CF_FIND_NEXT,
	NULL, 0
};

static HWND GhWndEdit = NULL;
struct ClassHolder
{
	UClass* Cls;
	HANDLE hUndoStack;
	~ClassHolder()
	{
		if (hUndoStack && GhWndEdit)
			SendMessage(GhWndEdit, AEM_EMPTYUNDOBUFFER, 0, (LPARAM)hUndoStack);
	}
};

// A code editing window.
class WCodeFrame : public WWindow
{
	DECLARE_WINDOWCLASS(WCodeFrame,WWindow,UnrealEd)

	// Variables.
	UClass* Class{};
	WRichEdit Edit;
	WListBox FilesList;
	BOOL bFirstTime;
	RECT rcStatus{};
	BOOL m_bStatusError{};
	FString m_StatusText;
	HWND hWndToolBar{};
	WToolTip* ToolTipCtrl{};
	WDlgFindReplace* DlgFindReplace{};
	WViewText* ViewDefProps{};
	HBITMAP ToolbarImage{};
	WDragInterceptor*	DragInterceptor{};
	INT DividerWidth = 200;

	WLabel ClosestThings;
	DWORD CodeHash;
	INT CheckedPos;

	TArray<ClassHolder> m_Classes;	// the list of classes we are editing scripts for
	UClass* pCurrentClass;

	// Constructor.
	WCodeFrame( FName InPersistentName, WWindow* InOwnerWindow )
	: WWindow( InPersistentName, InOwnerWindow )
	,	Edit		( this )
	,	FilesList	( this, IDLB_FILES )
	,	ClosestThings ( this )
	{
		pCurrentClass = NULL;
		bFirstTime = TRUE;
		rcStatus.top = rcStatus.bottom = rcStatus.left = rcStatus.right = 0;
		if( PersistentName != NAME_None )
			GConfig->GetInt( TEXT("WindowPositions"), *(FString(*PersistentName)+TEXT(".Split")), DividerWidth );
		ClampDividerWidth();
		CodeHash = -1;
		CheckedPos = -1;
	}

	void ClampDividerWidth()
	{
		DividerWidth = Clamp(DividerWidth, 100, 400);
	}

	// WWindow interface.
	void OnSetFocus( HWND hWndLoser )
	{
		guard(WCodeFrame::OnSetFocus);
		WWindow::OnSetFocus( hWndLoser );
		SetFocus( Edit );
		unguard;
	}
	virtual INT OnSetCursor()
	{
		guard(WCodeFrame::OnSetCursor);
		FPoint P = GetCursorPos();
		if (Abs(P.X - DividerWidth) <= 2)
		{
			SetCursor(LoadCursorW(hInstanceWindow,MAKEINTRESOURCEW(IDC_SplitWE)));
			if (GetAsyncKeyState(GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON) & 0x8000)
				BeginSplitterDrag();
			return 1;
		}
		SetCursor(LoadCursorW(NULL, IDC_ARROW));
		return 0;
		unguard;
	}
	void BeginSplitterDrag()
	{
		guard(WCodeFrame::BeginSplitterDrag);
		DragInterceptor            = new WDragInterceptor( this, FPoint(0,INDEX_NONE), GetClientRect(), FPoint(3,3) );	
		DragInterceptor->DragPos   = FPoint(FilesList.GetWindowRect().Width(),GetCursorPos().Y);
		DragInterceptor->DragClamp = FRect(GetClientRect().Inner(FPoint(100,0)));
		DragInterceptor->OpenWindow();
		unguard;
	}
	void OnFinishSplitterDrag( WDragInterceptor* Drag, UBOOL Success )
	{
		guard(WCodeFrame::OnFinishSplitterDrag);
		if( Success )
		{
			DividerWidth += Drag->DragPos.X - Drag->DragStart.X;
			ClampDividerWidth();
			if( PersistentName != NAME_None )
				GConfig->SetInt( TEXT("WindowPositions"), *(FString(*PersistentName)+TEXT(".Split")), DividerWidth );
			PositionChildControls();
			InvalidateRect( *this, NULL, 0 );
			UpdateWindow( *this );
		}
		DragInterceptor = NULL;
		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WCodeFrame::OnSize);
		WWindow::OnSize( Flags, NewX, NewY );
		PositionChildControls();
		unguard;
	}
	void PositionChildControls( void )
	{
		guard(WCodeFrame::PositionChildControls);

		if( !::IsWindow( GetDlgItem( hWnd, ID_CF_TOOLBAR )))	return;

		FRect CR = GetClientRect();
		::MoveWindow(hWndToolBar, 4, 0, 10 * MulDiv(16, DPIX, 96), 10 * MulDiv(16, DPIY, 96), 1);
		::MoveWindow(ClosestThings.hWnd, MulDiv(300, DPIX, 96), MulDiv(6, DPIY, 96), CR.Width() - MulDiv(350, DPIX, 96), MulDiv(20, DPIY, 96), 1);
		RECT R;
		::GetWindowRect( GetDlgItem( hWnd, ID_CF_TOOLBAR ), &R );
		::MoveWindow( GetDlgItem( hWnd, ID_CF_TOOLBAR ), 0, 0, CR.Max.X, R.bottom, TRUE );

		FilesList.MoveWindow( FRect(0,(R.bottom - R.top) - 1,DividerWidth,CR.Max.Y), TRUE );

		Edit.MoveWindow( FRect(DividerWidth,(R.bottom - R.top) - 1,CR.Max.X,CR.Max.Y - 20), TRUE );
		//warren Edit.ScrollCaret();

		rcStatus.left = DividerWidth;
		rcStatus.right = CR.Max.X;
		rcStatus.top = CR.Max.Y - 20;
		rcStatus.bottom = CR.Max.Y;

		::InvalidateRect( hWnd, NULL, TRUE );

		unguard;
	}
	void UpdateStatus( BOOL bError, FString Text )
	{
		guard(WCodeFrame::UpdateStatus);
		m_bStatusError = bError;
		m_StatusText = Text;
		::InvalidateRect( hWnd, NULL, TRUE );
		unguard;
	}
	void OnPaint()
	{
		guard(WCodeFrame::OnPaint);
		PAINTSTRUCT PS;
		HDC hDC = BeginPaint( *this, &PS );

		//
		// STATUS BAR
		//

		HPEN l_penError, l_penOK, l_penOld;
		HBRUSH l_brushError, l_brushOK, l_brushOld;

		::SetBkMode( hDC, TRANSPARENT );

		l_penError = ::CreatePen( PS_SOLID, 1, RGB(255, 0, 0) );
		l_penOK = ::CreatePen( PS_SOLID, 1, ::GetSysColor( COLOR_3DFACE ) );
		l_brushError = ::CreateSolidBrush( RGB(255, 0, 0) );
		l_brushOK = ::CreateSolidBrush( ::GetSysColor( COLOR_3DFACE ) );

		if( m_bStatusError )
		{
			l_penOld = (HPEN)::SelectObject( hDC, l_penError );
			l_brushOld = (HBRUSH)::SelectObject( hDC, l_brushError );
			::SetTextColor( hDC, RGB(255, 255, 255) );
		}
		else
		{
			l_penOld = (HPEN)::SelectObject( hDC, l_penOK );
			l_brushOld = (HBRUSH)::SelectObject( hDC, l_brushOK );
			::SetTextColor( hDC, ::GetSysColor( COLOR_BTNTEXT ) );
		}

		// Draw the background
		::Rectangle( hDC, rcStatus.left, rcStatus.top, rcStatus.right, rcStatus.bottom );

		AECHARINDEX ciCaret;
		SendMessage(Edit.hWnd, AEM_GETSEL, (WPARAM)&ciCaret, NULL);

		volatile INT Start, End; // volatile for prevent compiler "optimize" out this variables
		SendMessage(Edit.hWnd, EM_GETSEL, (WPARAM)&Start, (LPARAM)&End);

		AECHARINDEX LStart, LEnd;
		SendMessage(Edit.hWnd, AEM_EXGETSEL, (WPARAM)&LStart, (LPARAM)&LEnd);

		FString Pos = FString::Printf(TEXT("%d:%d"), ciCaret.nLine + 1, ciCaret.nCharInLine + 1);
		if (End > Start)
			Pos = FString::Printf(TEXT("%d:%d | %ls"), LEnd.nLine - LStart.nLine + 1, End - Start, *Pos);
			
		::DrawTextW( hDC, *Pos, Pos.Len(), &rcStatus, DT_RIGHT | DT_VCENTER | DT_SINGLELINE );

		UpdateClosestThings(LEnd.nLine);

		LONG Right = rcStatus.right;
		SIZE Size;
		if (GetTextExtentPoint32W(hDC, *Pos, Pos.Len(), &Size))
			rcStatus.right = Max(0L, rcStatus.right - Size.cx - MulDiv(3, DPIX, 96));

		// Draw the message
		::DrawTextW( hDC, *m_StatusText, m_StatusText.Len(), &rcStatus, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );
		
		rcStatus.right = Right;

		// Clean up
		::SetBkMode( hDC, OPAQUE );

		::SelectObject( hDC, l_penOld );
		::SelectObject( hDC, l_brushOld );

		EndPaint( *this, &PS );

		::DeleteObject( l_penError );
		::DeleteObject( l_penOK );
		::DeleteObject( l_brushError );
		::DeleteObject( l_brushOK );

		unguard;
	}
	void UpdateClosestThings(INT Pos)
	{
		guard(WCodeFrame::UpdateClosestThings);
		if (!pCurrentClass)
			return;
		DWORD Hash = appStrihash(*pCurrentClass->ScriptText->Text);
		if (CheckedPos == Pos && CodeHash == Hash)
			return;
		CheckedPos = Pos;
		CodeHash = Hash;

		FString State = GetThing(TEXT("\\sstate\\s+\\w+\\s*\\{")).
			Replace(TEXT("\r\n"), TEXT(" ")).Replace(TEXT("{"), TEXT(" "));
		FString Function = GetThing(TEXT("\\s(function|event)\\s+(\\w+\\s+)?\\w+\\s*\\(")).
			Replace(TEXT("\r\n"), TEXT(" ")).Replace(TEXT("("), TEXT(" "));

		ClosestThings.SetText(*FString::Printf(TEXT("%ls\t\t\t%ls"), *State, *Function));
		unguard;
	}
	FString GetThing(const TCHAR* Pattern)
	{
		guard(WCodeFrame::GetThing);
		FString Match;
		AEFINDTEXTW ft = { 0 };

		ft.dwFlags = AEFR_REGEXP;
		ft.pText = Pattern;
		ft.dwTextLen = (UINT_PTR)-1;
		ft.nNewLine = AELB_ASIS;
		SendMessage(Edit.hWnd, AEM_GETINDEX, AEGI_CARETCHAR, (LPARAM)&ft.crSearch.ciMax);
		ft.crSearch.ciMin = ft.crSearch.ciMax;

		const INT MAX_STEPS = 3;
		for (INT i = 0; i < MAX_STEPS; i++)
		{
			if (ft.crSearch.ciMin.nLine == 0)
				break;
			if (i == MAX_STEPS - 1)
				SendMessage(Edit.hWnd, AEM_GETINDEX, AEGI_FIRSTCHAR, (LPARAM)&ft.crSearch.ciMin);
			else
				SendMessage(Edit.hWnd, AEM_GETLINEINDEX, Max(0, ft.crSearch.ciMin.nLine - 500), (LPARAM)&ft.crSearch.ciMin);

			BOOL bFound = SendMessage(Edit.hWnd, AEM_FINDTEXTW, 0, (LPARAM)&ft);
			if (ft.nCompileErrorOffset)
			{
				debugf(TEXT("Failed compile GetThing REGEX pattern '%ls', error on pos %d at '%ls'"),
					ft.pText, ft.nCompileErrorOffset, &ft.pText[ft.nCompileErrorOffset - 1]);
				break;
			}
			else if (bFound)
			{
				AETEXTRANGEW tr = { 0 };
				tr.cr = ft.crFound;
				tr.nNewLine = AELB_ASOUTPUT;
				tr.bFillSpaces = FALSE;
				tr.dwBufferMax = SendMessage(Edit.hWnd, AEM_GETTEXTRANGEW, 0, (LPARAM)&tr);

				if (tr.dwBufferMax)
				{
					TArray<TCHAR> Buf(tr.dwBufferMax);
					tr.pBuffer = &Buf(0);
					SendMessage(Edit.hWnd, AEM_GETTEXTRANGEW, 0, (LPARAM)&tr);

					Match = &Buf(0);
				}
				break;
			}
		}
		return Match;
		unguard;
	}
	// Checks for script compile errors.
	//
	void ProcessResults(void)
	{
		guard(WCodeFrame::ProcessResults);
		FStringOutputDevice GetPropResult = FStringOutputDevice();
		GEditor->Get( TEXT("TEXT"), TEXT("RESULTS"), GetPropResult );

		FString S;

		S = GetPropResult;
		// Sometimes there's crap on the end of the message .. strip it off.
		if( S.InStr(TEXT("\x0d")) != -1 )
			S = S.Left( S.InStr(TEXT("\x0d")) );
		UpdateStatus(HighlightError(S), *S);

		UpdateDefProps();

		unguard;
	}
	// Highlights a compilation error by opening up that classes script and moving to the appropriate line.
	//
	BOOL HighlightError(FString S)
 	{
 		guard(WCodeFrame::HighlightError);
		if (appStrcmp( *(S.Left(9)), TEXT("Error in ")) || S.InStr(TEXT(":")) == -1)
			return FALSE;
		S = S.Mid(9);
		INT i = S.InStr(TEXT(", Line "));
		INT Line = 0;
		if (i != -1)
		{
			Line = appAtoi(*(S.Mid(i + 7)));	// Line number
			S = S.Left(i);						// Class name
		}
 
 		UClass* Class;
		if( ParseObject<UClass>( *(FString::Printf(TEXT("CLASS=%ls"), *S)), TEXT("CLASS="), Class, ANY_PACKAGE ) )
 		{
			AddClass(Class, 0, 0); // load code into editor
			Line--; // zero-based index
			AddClass(Class, SendMessageW(Edit.hWnd, EM_LINEINDEX, Line, 0), Line); // scroll to actual line
 		}
		return TRUE;
 		unguard;
 	}
	virtual LRESULT WndProc(UINT Message, WPARAM wParam, LPARAM lParam)
	{
		if (Message == WM_LBUTTONDBLCLK)
			HighlightError(m_StatusText);

		return WWindow::WndProc(Message, wParam, lParam);
	}
	void OnCommand( INT Command )
	{
		guard(WCodeFrame::OnCommand);
		if (Command == Edit.ControlId)
			::InvalidateRect( hWnd, NULL, TRUE );
		
		switch( Command ) {
			case WM_TREEVIEW_RIGHT_CLICK:
			{
				FString Name = FilesList.GetString( FilesList.GetCurrent() );
				GEditor->Exec( *(FString::Printf(TEXT("SETCURRENTCLASS CLASS=%ls"), *Name)) );
				GBrowserMaster->ShowBrowser( eBROWSER_ACTOR );
				SendMessage((*GBrowserMaster->Browsers[eBROWSER_ACTOR])->hWnd, WM_COMMAND, IDMN_AB_SELECT_CLASS, 0);
			}
			break;

			case ID_EditFind:
				if (DlgFindReplace->m_bShow)
					DlgFindReplace->FillFromSelection();
				DlgFindReplace->Show(1);
				break;

			case IDMN_CF_FIND_NEXT:
				if( DlgFindReplace->GSearchText.Len() )
					DlgFindReplace->OnFindNext();
				else
					DlgFindReplace->Show(1);
				break;

			case IDMN_CF_FIND_PREV:
				if( DlgFindReplace->GSearchText.Len() )
					DlgFindReplace->OnFindPrev();
				else
					DlgFindReplace->Show(1);
				break;

			case IDMN_CF_EXPORT_CHANGED:
				if (GBrowserMaster->Browsers[eBROWSER_ACTOR])
					SendMessage((*GBrowserMaster->Browsers[eBROWSER_ACTOR])->hWnd, WM_COMMAND, IDMN_AB_EXPORT, 0);
				SetFocus(Edit.hWnd);
				break;

			case IDMN_CF_EXPORT_ALL:
				if (GBrowserMaster->Browsers[eBROWSER_ACTOR])
					SendMessage((*GBrowserMaster->Browsers[eBROWSER_ACTOR])->hWnd, WM_COMMAND, IDMN_AB_EXPORT_ALL, 0);
				SetFocus(Edit.hWnd);
				break;

			case IDMN_CF_COMPILE:
				{
					GWarn->BeginSlowTask( TEXT("Compiling changed scripts"), 1, 0 );
					Save();
					GEditor->Exec( TEXT("SCRIPT MAKE") );
					GWarn->EndSlowTask();

					ProcessResults();
					SetFocus(Edit.hWnd);
				}
				break;

			case IDMN_CF_COMPILE_ALL:
				{
					if (::MessageBox(hWnd, TEXT("Do you really want to compile all scripts?"), TEXT("Compile all?"), MB_YESNO) == IDYES)
					{
						GWarn->BeginSlowTask(TEXT("Compiling all scripts"), 1, 0);
						Save();
						GEditor->Exec(TEXT("SCRIPT MAKE ALL"));
						GWarn->EndSlowTask();

						ProcessResults();
					}
					SetFocus(Edit.hWnd);
				}
				break;

			case IDMN_CLOSE_ALL:
				{
					FString Name = FilesList.GetString( FilesList.GetCurrent() );
					m_Classes.Empty();
					m_Classes.AddItem({FindObject<UClass>(ANY_PACKAGE, *Name, 1)});
				}
				/* no break; */

			case IDMN_CLOSE:
				{
					Save();

					// Find the currently selected class and remove it from the list.
					//
					FString Name = FilesList.GetString( FilesList.GetCurrent() );
					RemoveClass( Name );
				}
				break;

			case ID_BrowserActor:
			{
				GBrowserMaster->ShowBrowser( eBROWSER_ACTOR );
			}
			break;

			case ID_ViewDefProp:
				ViewDefProps->Show(1);
			break;

			case ID_FileSave:
			{
				if (HIWORD(GetKeyState(VK_CONTROL))) // hotkey supression
				{
					TCHAR Keys[] = {'S', 'L'};
					for (INT i = 0; i < ARRAY_COUNT(Keys); i++)
						if (HIWORD(GetKeyState(Keys[i])))
							Edit.IgnoreHotkeyDown = Keys[i];
				}
				if (pCurrentClass)
				{
					Save(); // save current script if any

					FString Pkg = *pCurrentClass->GetOuterUPackage()->GetFName();

					if (Pkg == "MyLevel")
						MapSave(this);
					else
					{
						if (!GLastDir[eLASTDIR_CLS].Len())
							GLastDir[eLASTDIR_CLS] = TEXT("..\\System");
						FString File = Pkg;
						File += TEXT(".u");

						if (!GRecoveryMode || (GetSaveNameWithDialog(
							*File,
							*GLastDir[eLASTDIR_CLS],
							TEXT("Class Packages (*.u)\0*.u\0All Files\0*.*\0\0"),
							TEXT("u"),
							*FString::Printf(TEXT("Save %ls Package"), *Pkg),
							File
						) && AllowOverwrite(*File)))
						{
							GEditor->Exec( *(FString::Printf(TEXT("OBJ SAVEPACKAGE PACKAGE=\"%ls\" FILE=\"%ls\""), *Pkg, *File )) );

							GLastDir[eLASTDIR_CLS] = appFilePathName(*File);
						}

						GFileManager->SetDefaultDirectory(appBaseDir());
					}

					SetFocus(Edit.hWnd);
				}
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

		SetMenu( hWnd, AddHotkeys(LoadMenuW(hInstance, MAKEINTRESOURCEW(IDMENU_CodeFrame))) );

		// Load windows last position.
		//
		int X, Y, W, H;

		if(!GConfig->GetInt( *PersistentName, TEXT("X"), X, GUEDIni ))	X = 0;
		if(!GConfig->GetInt( *PersistentName, TEXT("Y"), Y, GUEDIni ))	Y = 0;
		if(!GConfig->GetInt( *PersistentName, TEXT("W"), W, GUEDIni ))	W = 640;
		if(!GConfig->GetInt( *PersistentName, TEXT("H"), H, GUEDIni ))	H = 480;

		if( !W ) W = 320;
		if( !H ) H = 200;

		debugf(TEXT("WCodeFrame::OnCreate - Moving Window - X: %d, Y: %d, W: %d, H: %d"), X, Y, W, H);

		::MoveWindow( hWnd, X, Y, W, H, TRUE );

		// Set up the main edit control.
		//
		Edit.OpenWindow(1,0);
		GhWndEdit = Edit.hWnd;
		
		Edit.SetText(TEXT(""));

		AECOLORS aec;

		aec.dwFlags = AECLR_BASICTEXT | AECLR_BASICBK | AECLR_SELBK | AECLR_URLTEXT | AECLR_URLCURSORTEXT | AECLR_URLVISITTEXT;
		aec.crBasicText = RGB(255,255,255);
		aec.crBasicBk = RGB(0,0,64);
		aec.crSelBk = RGB(127,0,127);
		aec.crUrlText = RGB(255,0,255);
		aec.crUrlCursorText = RGB(255,127,255);
		aec.crUrlVisitText = RGB(127,127,255);
		SendMessage(Edit.hWnd, AEM_SETCOLORS, 0, (LPARAM)&aec);

		new SyntaxHighlighting(Edit.hWnd);

		Edit.SetReadOnly( TRUE );

		ToolbarImage = reinterpret_cast<HBITMAP>(LoadImage(hInstance,
			MAKEINTRESOURCE(IDB_CodeFrame_TOOLBAR),
			IMAGE_BITMAP, 0, 0, 0));
		check(ToolbarImage);
		ScaleImageAndReplace(ToolbarImage);

		hWndToolBar = CreateToolbarEx(
			hWnd, WS_CHILD | WS_VISIBLE | CCS_ADJUSTABLE,
			ID_CF_TOOLBAR,
			8,
			NULL,
			reinterpret_cast<PTRINT>(ToolbarImage),
			(LPCTBBUTTON)&tbCFButtons,
			ARRAY_COUNT(tbCFButtons),
			MulDiv(16, DPIX, 96), MulDiv(16, DPIY, 96),
			MulDiv(16, DPIX, 96), MulDiv(16, DPIY, 96),
			sizeof(TBBUTTON));

		if( !hWndToolBar )
			appMsgf( TEXT("Toolbar not created!") );

		ClosestThings.OpenWindow(TRUE, FALSE);
		SetParent(ClosestThings.hWnd, hWndToolBar);

		ToolTipCtrl = new WToolTip(this);
		ToolTipCtrl->OpenWindow();

		for( int tooltip = 0 ; ToolTips_CF[tooltip].ID > 0 ; tooltip++ )
		{
			// Figure out the rectangle for the toolbar button.
			int index = SendMessageW( hWndToolBar, TB_COMMANDTOINDEX, ToolTips_CF[tooltip].ID, 0 );
			RECT rect;
			SendMessageW( hWndToolBar, TB_GETITEMRECT, index, (LPARAM)&rect);

			ToolTipCtrl->AddTool( hWndToolBar, ToolTips_CF[tooltip].ToolTip, tooltip, &rect );
		}

		FilesList.OpenWindow( 1, 0, 0, 0, 1 );
		FilesList.DoubleClickDelegate = FDelegate(this, (TDelegate)&WCodeFrame::OnFilesListDblClick);

		UpdateStatus( FALSE, TEXT("Ready."));

		DlgFindReplace = new WDlgFindReplace( NULL, this );
		DlgFindReplace->EditHwnd = Edit.hWnd;
		DlgFindReplace->DoModeless();
		DlgFindReplace->Show(0);

		ViewDefProps = new WViewText( TEXT("ViewDefProps"), this );
		ViewDefProps->OpenWindow();
		HFONT Font = (HFONT)SendMessageW( Edit.hWnd, WM_GETFONT, 0, 0);
		if (Font)
			ViewDefProps->Display.SetFont(Font);
		ViewDefProps->Show(0);
		SetTimer( hWnd, ID_ViewDefProp, 1000, NULL);

		unguard;
	}
	void Save(void)
	{
		guard(WCodeFrame::Save);

		if( !pCurrentClass )	return;
		if( !m_Classes.Num() )	return;

		int iLength = Edit.GetLength();
		if (iLength > 0)
		{
			FString unibuf = Edit.GetText();

			TCHAR LastChar = unibuf[unibuf.Len() - 1];

			// The script compiler wants a newline at the end of the file...
			if (LastChar != '\r' && LastChar != '\n')
				unibuf += TEXT("\n");
			
			pCurrentClass->ScriptText->Text = unibuf;
			SendMessageW(Edit.hWnd, EM_GETSEL, (WPARAM) & (pCurrentClass->ScriptText->Pos), 0);
			pCurrentClass->ScriptText->Top = SendMessageW(Edit.hWnd, EM_GETFIRSTVISIBLELINE, 0, 0);
		}

		unguard;
	}
	void OnDestroy()
	{
		guard(WCodeFrame::OnDestroy);

		delete DlgFindReplace;

		Save();

		// Save Window position (base class doesn't always do this properly)
		// (Don't do this if the window is minimized.)
		//
		if( !::IsIconic( hWnd ) && !::IsZoomed( hWnd ) )
		{
			RECT R;
			::GetWindowRect(hWnd, &R);

			GConfig->SetInt( *PersistentName, TEXT("Active"), m_bShow, GUEDIni );
			GConfig->SetInt( *PersistentName, TEXT("X"), R.left, GUEDIni );
			GConfig->SetInt( *PersistentName, TEXT("Y"), R.top, GUEDIni );
			GConfig->SetInt( *PersistentName, TEXT("W"), R.right - R.left, GUEDIni );
			GConfig->SetInt( *PersistentName, TEXT("H"), R.bottom - R.top, GUEDIni );
		}

		::DestroyWindow( hWndToolBar );
		delete ToolTipCtrl;

		if (ToolbarImage)
			DeleteObject(ToolbarImage);

		WWindow::OnDestroy();
		unguard;
	}
	void OpenWindow( UBOOL bMdi=0, UBOOL AppWindow=0 )
	{
		guard(WCodeFrame::OpenWindow);
		MdiChild = bMdi;
		debugf(TEXT("WCodeFrame::OpenWindow - Creating Window - W: %d, H: %d"),
			MulDiv(384, DPIX, 96),
			MulDiv(512, DPIY, 96));
		PerformCreateWindowEx
		(
			WS_EX_WINDOWEDGE,
			TEXT("Script Editor"),
			WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_SIZEBOX | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			MulDiv(384, DPIX, 96),
			MulDiv(512, DPIY, 96),
			OwnerWindow->hWnd,
			NULL,
			hInstance
		);
		unguard;
	}
	void AddClass( UClass* pClass, int Pos = -1, int Top = -1, BOOL bSilent = FALSE )
	{
		guard(WCodeFrame::AddClass);
		debugf(TEXT("WCodeFrame::AddClass - %ls"), *FObjectFullName(pClass));
		if( !pClass ) return;

		// Make sure this class has a script.
		//
		FStringOutputDevice GetPropResult = FStringOutputDevice();
	    GEditor->Get(TEXT("SCRIPTPOS"), *FObjectPathName(pClass), GetPropResult);
		if( !GetPropResult.Len() )
		{
			if (!bSilent)
				appMsgf( *(FString::Printf(TEXT("'%ls' has no script to edit."), *FObjectPathName(pClass))) );
			return;
		}

		// Only add this class to the list if it's not already there.
		//
		if (FilesList.FindStringExact(*FObjectPathName(pClass)) == -1)
		{
			debugf(TEXT("WCodeFrame::AddClass - Adding class to list"));
			m_Classes.AddItem({pClass});
		}

		if (bSilent)
			return;

		RefreshScripts();
		
		SetClass( *FObjectPathName(pClass), Pos, Top );

		Show(1);
		::BringWindowToTop(hWnd);
		
		unguard;
	}
	void RemoveClass( FString Name )
	{
		guard(WCodeFrame::RemoveClass);

		// Remove the class from the internal list.
		//
		for( int x = 0 ; x < m_Classes.Num() ; x++ )
		{
			if( !appStrcmp( *FObjectPathName(m_Classes(x).Cls), *Name ) )
			{
				m_Classes.Remove(x);
				break;
			}
		}

		INT ID = FilesList.GetCurrent();
		RefreshScripts();
		if (FilesList.GetCount() == ID)
			ID--;
		FilesList.SetCurrent( ID, true );
		OnFilesListDblClick();

		if( !m_Classes.Num() )
			pCurrentClass = NULL;
		SetCaption();

		unguard;
	}
	void RefreshScripts(void)
	{
		guard(WCodeFrame::RefreshScripts);
		
		FilesList.SetRedraw(false);

		// LOADED SCRIPTS
		//
		FilesList.Empty();
		for( int x = 0 ; x < m_Classes.Num() ; x++ )
			FilesList.AddString( *FObjectPathName(m_Classes(x).Cls) );
			
		FilesList.SetRedraw(true);

		unguard;
	}
	void OnTimer()
	{
		guard(WCodeFrame::OnTimer);
		UpdateDefProps();
		unguard;
	}
	void UpdateDefProps()
	{
		guard(WCodeFrame::UpdateDefProps);
		if (!ViewDefProps)
			return;
		FString Caption;
		FString Text;
		if (pCurrentClass)
		{
			Caption = *pCurrentClass->GetFName();
			Text = ExportDefPropsToFString(pCurrentClass);
		}
		Caption = FString::Printf(TEXT("Default %ls properties"), *Caption);
		ViewDefProps->UpdateText(*Caption, *Text);
		unguard;
	}
	// Saves the current script and selects a new script.
	//
	void SetClass( FString Name, int Pos = -1, int Top = -1 )
	{
		guard(WCodeFrame::SetClass);

		// If there are no classes loaded, just empty the edit control.
		//
		if( !m_Classes.Num() )
		{
			Edit.SetReadOnly( TRUE );
			Edit.SetText(TEXT("No scripts loaded."));
			pCurrentClass = NULL;
			return;
		}

		// Locate the proper class pointer.
		//
		ClassHolder* NewClass = NULL;
		for (int x = 0; x < m_Classes.Num(); x++)
		{
			if (!appStrcmp(*FObjectPathName(m_Classes(x).Cls), *Name))
			{
				NewClass = &m_Classes(x);
				break;
			}
		}
		BOOL Same = NewClass && pCurrentClass == NewClass->Cls;

		// Save the settings/script for the current class before changing.
		//
		if( pCurrentClass && !bFirstTime && !Same )
		{
			Save();
			for (int x = 0; x < m_Classes.Num(); x++)
			{
				if (m_Classes(x).Cls == pCurrentClass)
				{
					m_Classes(x).hUndoStack = (HANDLE)SendMessage(Edit.hWnd, AEM_DETACHUNDO, 0, 0);
					break;
				}
			}
		}

		bFirstTime = FALSE;
		FilesList.SetCurrent( FilesList.FindString( *Name ), 1 );

		Edit.SetReadOnly( FALSE );

		pCurrentClass = NewClass->Cls;

		// Override whatever is in the class if we need to.
		//
		if( Pos > -1 )		pCurrentClass->ScriptText->Pos = Pos;
		if( Top > -1 )		pCurrentClass->ScriptText->Top = Top;

		if (!Same)
		{
			// Load current script into edit window.
			//
			SetCaption();

			FString FullCode = pCurrentClass->ScriptText->Text;

			// Set text into the RichEdit control
			Edit.SetRedraw(false);
			Edit.SetText(*FullCode);
			Edit.SetRedraw(true);
			if (NewClass->hUndoStack && SendMessage(Edit.hWnd, AEM_ATTACHUNDO, 0, (LPARAM)NewClass->hUndoStack))
				NewClass->hUndoStack = NULL;
		}
	
		ScrollToLine(pCurrentClass->ScriptText->Top, pCurrentClass->ScriptText->Pos);

		::SetFocus( Edit.hWnd );
		unguard;
	}
	void SetCaption()
	{
		if( pCurrentClass )
			SetText( *FObjectFullName(pCurrentClass) );
		else
			SetText( TEXT("") );
		
		UpdateDefProps();
	}
	void ScrollToLine( int Line, int Pos )
	{
		guard(WCodeFrame::ScrollToLine);

		// Stop the window from updating while scrolling to the requested line.  This makes
		// it go MUCH faster -- and it looks better.
		//
		Edit.SetRedraw(false);

		int LinePos = SendMessageW(Edit.hWnd, EM_LINEINDEX, Line, 0);
		SendMessageW(Edit.hWnd, EM_SETSEL, LinePos, LinePos);
		SendMessageW(Edit.hWnd, EM_SCROLLCARET, 0, 0);
		
		SCROLLINFO SI;
		SI.cbSize = sizeof(SI);
		SI.fMask  = SIF_POS;

		GetScrollInfo(Edit.hWnd, SB_VERT, &SI);

		SendMessageW(Edit.hWnd, EM_SETSEL, Pos, Pos); // can scroll to caret

		SetScrollInfo(Edit.hWnd, SB_VERT, &SI, false);

		int CurTop = SendMessageW(Edit.hWnd, EM_GETFIRSTVISIBLELINE, 0, 0);
		SendMessageW(Edit.hWnd, EM_LINESCROLL, 0, Line - CurTop);

		Edit.SetRedraw(true);
		unguard;
	}
	int OnSysCommand( INT Command )
	{
		guard(WCodeFrame::OnSysCommand);
		// Don't actually close the window when the user hits the "X" button.  Just hide it.
		if( Command == SC_CLOSE )
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
		FString Name = FilesList.GetString( FilesList.GetCurrent() );
		SetClass( Name );
		SetCaption();
		unguard;
	}
	// Written by Marco, copy object defaultproperties block into clipboard.
	FString ExportDefPropsToFString(UClass* Class)
	{
		guard(WCodeFrame::ExportDefPropsToFString);
		FStringOutputDevice Out;

		// stijn: let Core know which class the exports are relative to
		auto OldObject = GPropObject;
		GPropObject = Class;

		Out.Logf(TEXT("defaultproperties\r\n{\r\n"));
		BYTE* MainOffset = &Class->Defaults(0);
		BYTE* ParentOffset = (Class->GetSuperClass() ? &Class->GetSuperClass()->Defaults(0) : NULL);
		TArray<BYTE> ZeroBytes;
		for (TFieldIterator<UProperty> It(Class); It; ++It)
		{
			UProperty* P = *It;
			if ((P->PropertyFlags & CPF_Transient) || (P->PropertyFlags & CPF_Native))
				continue;

			// Get bytes to compare with.
			UClass* PropClass = Cast<UClass>(P->GetOuter());
			BYTE* CompBytes = NULL;
			bool bZeroBytes = false;
			if (PropClass == Class)
			{
				if (ZeroBytes.Num() < P->ElementSize)
					ZeroBytes.AddZeroed(P->ElementSize - ZeroBytes.Num());
				CompBytes = &ZeroBytes(0);
				bZeroBytes = true;
			}
			else if (PropClass)
				CompBytes = ParentOffset;

			if (P->ArrayDim > 1)
			{
				for (INT i = 0; i < P->ArrayDim; ++i)
				{
					INT Offset = P->Offset + (P->ElementSize * i);
					BYTE* ThisOffset = (CompBytes ? (bZeroBytes ? CompBytes : (CompBytes + Offset)) : NULL);
					if (CompBytes && P->Identical(MainOffset + Offset, ThisOffset))
						continue;
					FString Value;
#if ENGINE_VERSION==227
					P->ExportTextItem(Value, MainOffset + Offset, ThisOffset, PPF_Delimited | PPF_Compact);
#else
					UProperty::ExportTextItem(P, Value, MainOffset + Offset, ThisOffset, PPF_Delimited | PPF_Compact);
#endif
					Out.Logf(TEXT("\t%ls(%i)=%ls\r\n"), *FObjectName(P), i, *Value);
				}
			}
			else
			{
				INT Offset = P->Offset;
				BYTE* ThisOffset = (CompBytes ? (bZeroBytes ? CompBytes : (CompBytes + Offset)) : NULL);
				if (CompBytes && P->Identical(MainOffset + Offset, ThisOffset))
					continue;
				FString Value;
#if ENGINE_VERSION==227
				P->ExportTextItem(Value, MainOffset + Offset, CompBytes, PPF_Delimited | PPF_Compact);
#else
				UProperty::ExportTextItem(P, Value, MainOffset + Offset, CompBytes, PPF_Delimited | PPF_Compact);
#endif
				Out.Logf(TEXT("\t%ls=%ls\r\n"), *FObjectName(P), *Value);
			}
		}
		Out.Logf(TEXT("}"));

		GPropObject = OldObject;
		return *Out;
		unguard;
	}
	// Written by Marco, paste object defaultproperties block from clipboard.
	const TCHAR* GetNextWord(const TCHAR* S)
	{
		while (*S == ' ' || *S == '\n' || *S == '\r' || *S == '\t')
			++S;
		return S;
	}
	bool StringMatches(const TCHAR* S, const TCHAR* D)
	{
		guard(WBrowserActor::StringMatches);
		while (*D)
		{
			if (appToUpper(*S) != appToUpper(*D))
				return false;
			++S;
			++D;
		}
		return true;
		unguard;
	}
	void ImportDefPropsFromFString(UClass* Class, FString Str)
	{
		guard(WCodeFrame::ImportDefPropsFromFString);
		const TCHAR* S = *Str;
		BYTE* MainOffset = &Class->Defaults(0);

		// Remove 'defaultproperties' text if there is.
		S = GetNextWord(S);
		if (StringMatches(S, TEXT("defaultproperties")))
			S += 17;
		// Remove '{'
		S = GetNextWord(S);
		if (*S == '{')
			++S;

		// Process parms
		while (1)
		{
			S = GetNextWord(S);
			if (*S == '}' || !*S)
				break; // End of properties.

			// Get property name
			const TCHAR* Start = S;
			while (*S && *S != '(' && *S != '[' && *S != '=')
				++S;
			if (!*S)
				break;

			FString PropName(Start, S-Start);

			// Get array index
			FString ArIndex;
			if (*S == '(' || *S == '[') // Array index
			{
				++S;
				Start = S;
				while (*S && *S != ']' && *S != ')' && *S != '=')
					++S;
				if (!*S)
					break;
				ArIndex = FString(Start, S-Start);
			}

			// Get property value
			while (*S && *S != '=')
				++S;
			if (*S)
				++S;
			if (!*S)
				break;
			Start = S;
			while (*S && *S != '\n' && *S != '\r')
				++S;
			
			FString Value(Start, S-Start);

			// Assign property value.
			//debugf(TEXT("Set '%ls' = '%ls'"),*PropName,*Value);
			UProperty* Prop = FindField<UProperty>(Class, *PropName);
			if (Prop)
			{
				if (Prop->ArrayDim > 1 && ArIndex.Len())
				{
					INT Index = appAtoi(*ArIndex);
					if (Index >= 0 && Index < Prop->ArrayDim)
						Prop->ImportText(*Value, MainOffset + Prop->Offset + (Index * Prop->ElementSize), PPF_Delimited);
				}
				else if (Prop->ArrayDim == 1)
					Prop->ImportText(*Value, MainOffset + Prop->Offset, PPF_Delimited);
			}
		}
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
