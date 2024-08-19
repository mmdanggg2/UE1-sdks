/*=============================================================================
	Window.cpp: GUI window management code.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#pragma warning(push)
// UT compiles with a non-default 4-byte struct alignment. This generates a few C4121 warnings when parsing winnt.h
// Luckily, we don't use the affected data structures so it is safe to ignore this warning
// --stijn
#pragma warning(disable: 4121) /* 'JOBOBJECT_IO_RATE_CONTROL_INFORMATION_NATIVE_V2': alignment of a member was sensitive to packing */
#pragma warning(disable: 4091)
#define STRICT
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#pragma warning(pop)
#include "Core.h"
#include "Window.h"
#include "AkelEdit.h"
#include "UnPolyFlag.h"

/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

WNDPROC WTabControl::SuperProc = NULL;
WNDPROC WLabel::SuperProc = NULL;
WNDPROC WCustomLabel::SuperProc = NULL;
//WNDPROC WListView::SuperProc = NULL;
WNDPROC WEdit::SuperProc = NULL;
WNDPROC WRichEdit::SuperProc = NULL;
WNDPROC WListBox::SuperProc = NULL;
WNDPROC WCheckListBox::SuperProc = NULL;
WNDPROC WTrackBar::SuperProc = NULL;
WNDPROC WProgressBar::SuperProc = NULL;
WNDPROC WComboBox::SuperProc = NULL;
WNDPROC WMRUComboBox::SuperProc = NULL;
WNDPROC WPackageComboBox::SuperProc = NULL;
WNDPROC WButton::SuperProc = NULL;
WNDPROC WToolTip::SuperProc = NULL;
WNDPROC WCoolButton::SuperProc = NULL;
WNDPROC WUrlButton::SuperProc = NULL;
WNDPROC WCheckBox::SuperProc = NULL;
WNDPROC WVScrollBar::SuperProc = NULL;
WNDPROC WTreeView::SuperProc = NULL;
INT WWindow::ModalCount=0;
INT WWindow::DPIX = 0;
INT WWindow::DPIY = 0;
UBOOL WWindow::AppActive = 0;
TArray<WWindow*> WWindow::_Windows;
TArray<WWindow*> WWindow::_DeleteWindows;
TArray<FTreeItem*> FTreeItem::DeleteItems;
TArray<WProperties*> WProperties::PropertiesWindows;
COLORREF FPropertyItem::acrCustClr[16];
COLORREF FHeaderItem::acrCustClr[16];
WINDOW_API WLog* GLogWindow=NULL;
WINDOW_API HBRUSH hBrushBlack;
WINDOW_API HBRUSH hBrushWhite;
WINDOW_API HBRUSH hBrushOffWhite;
WINDOW_API HBRUSH hBrushHeadline;
WINDOW_API HBRUSH hBrushStipple;
WINDOW_API HBRUSH hBrushCurrent;
WINDOW_API HBRUSH hBrushDark;
WINDOW_API HBRUSH hBrushGrey;
WINDOW_API HFONT hFontUrl;
WINDOW_API HFONT hFontText;
WINDOW_API HFONT hFontHeadline;
WINDOW_API HINSTANCE hInstanceWindow;
WINDOW_API UBOOL GNotify=0;
WCoolButton* WCoolButton::GlobalCoolButton=NULL;
WINDOW_API UINT WindowMessageOpen;
WINDOW_API UINT WindowMessageMouseWheel;
WINDOW_API NOTIFYICONDATA NID;
WINDOW_API NOTIFYICONDATAA NIDA;
WINDOW_API HACCEL hAccel = NULL;
WINDOW_API TArray<ACCEL> Accel;

WINDOW_API void (*GIdleTick)(void) = NULL;

IMPLEMENT_PACKAGE(Window)

/*-----------------------------------------------------------------------------
	Window manager.
-----------------------------------------------------------------------------*/

W_IMPLEMENT_CLASS(WWindow)
W_IMPLEMENT_CLASS(WControl)
W_IMPLEMENT_CLASS(WTabControl)
W_IMPLEMENT_CLASS(WLabel)
W_IMPLEMENT_CLASS(WCustomLabel)
//W_IMPLEMENT_CLASS(WListView)
W_IMPLEMENT_CLASS(WButton)
W_IMPLEMENT_CLASS(WToolTip)
W_IMPLEMENT_CLASS(WCoolButton)
W_IMPLEMENT_CLASS(WUrlButton)
W_IMPLEMENT_CLASS(WComboBox)
W_IMPLEMENT_CLASS(WMRUComboBox)
W_IMPLEMENT_CLASS(WPackageComboBox)
W_IMPLEMENT_CLASS(WEdit)
W_IMPLEMENT_CLASS(WRichEdit)
W_IMPLEMENT_CLASS(WTerminalBase)
W_IMPLEMENT_CLASS(WTerminal)
W_IMPLEMENT_CLASS(WLog)
W_IMPLEMENT_CLASS(WViewText)
W_IMPLEMENT_CLASS(WDialog)
W_IMPLEMENT_CLASS(WPasswordDialog)
W_IMPLEMENT_CLASS(WTextScrollerDialog)
W_IMPLEMENT_CLASS(WTrackBar)
W_IMPLEMENT_CLASS(WProgressBar)
W_IMPLEMENT_CLASS(WListBox)
W_IMPLEMENT_CLASS(WItemBox)
W_IMPLEMENT_CLASS(WCheckListBox)
W_IMPLEMENT_CLASS(WPropertiesBase)
W_IMPLEMENT_CLASS(WDragInterceptor)
W_IMPLEMENT_CLASS(WProperties)
W_IMPLEMENT_CLASS(WObjectProperties)
W_IMPLEMENT_CLASS(WClassProperties)
W_IMPLEMENT_CLASS(WConfigProperties)
W_IMPLEMENT_CLASS(WWizardPage)
W_IMPLEMENT_CLASS(WWizardDialog)
W_IMPLEMENT_CLASS(WEditTerminal)
W_IMPLEMENT_CLASS(WCheckBox)
W_IMPLEMENT_CLASS(WVScrollBar)
W_IMPLEMENT_CLASS(WTreeView)

// Constructor.
UWindowManager::UWindowManager()
{
	guard(UWindowManager::UWindowManager);

	// Init common controls.		
	INITCOMMONCONTROLSEX iccx;
	iccx.dwSize = sizeof(INITCOMMONCONTROLSEX);
	iccx.dwICC = ICC_WIN95_CLASSES | ICC_USEREX_CLASSES | ICC_BAR_CLASSES | ICC_HOTKEY_CLASS | ICC_LISTVIEW_CLASSES;
	verify(InitCommonControlsEx(&iccx));

	// Save instance.
	hInstanceWindow = hInstance;

	//LoadLibraryX(TEXT("RICHED20.DLL"));
	//LoadLibrary(TEXT("Msftedit.dll"));
	if (GIsEditor)
	    verifyError(LoadLibrary(TEXT("AkelEdit.dll")))

	// Implement window classes.
	IMPLEMENT_WINDOWSUBCLASS(WListBox,TEXT("LISTBOX"));
	IMPLEMENT_WINDOWSUBCLASS(WItemBox,TEXT("LISTBOX"));
	IMPLEMENT_WINDOWSUBCLASS(WCheckListBox,TEXT("LISTBOX"));	
	IMPLEMENT_WINDOWSUBCLASS(WTabControl,WC_TABCONTROL);
	IMPLEMENT_WINDOWSUBCLASS(WLabel,TEXT("STATIC"));
	IMPLEMENT_WINDOWSUBCLASS(WCustomLabel,TEXT("STATIC"));
//		IMPLEMENT_WINDOWSUBCLASS(WListView,TEXT("SysListView32"));
	IMPLEMENT_WINDOWSUBCLASS(WEdit,TEXT("EDIT"));
	//IMPLEMENT_WINDOWSUBCLASS(WRichEdit, MSFTEDIT_CLASS); // stijn: needs winxp SP1
	//IMPLEMENT_WINDOWSUBCLASS(WRichEdit, AES_RICHEDIT20W);
	if (GIsEditor)
	    IMPLEMENT_WINDOWSUBCLASS(WRichEdit, AES_AKELEDITW);
	IMPLEMENT_WINDOWSUBCLASS(WComboBox,TEXT("COMBOBOX"));
	IMPLEMENT_WINDOWSUBCLASS(WMRUComboBox,TEXT("COMBOBOX"));
	IMPLEMENT_WINDOWSUBCLASS(WPackageComboBox,TEXT("COMBOBOX"));
	IMPLEMENT_WINDOWSUBCLASS(WEditTerminal,TEXT("EDIT"));
	IMPLEMENT_WINDOWSUBCLASS(WButton,TEXT("BUTTON"));
	IMPLEMENT_WINDOWSUBCLASS(WToolTip,TOOLTIPS_CLASS);
	IMPLEMENT_WINDOWSUBCLASS(WCoolButton,TEXT("BUTTON"));
	IMPLEMENT_WINDOWSUBCLASS(WUrlButton,TEXT("BUTTON"));
	IMPLEMENT_WINDOWSUBCLASS(WCheckBox,TEXT("BUTTON"));
	IMPLEMENT_WINDOWSUBCLASS(WVScrollBar,TEXT("SCROLLBAR"));
	IMPLEMENT_WINDOWSUBCLASS(WTreeView,WC_TREEVIEW);
	IMPLEMENT_WINDOWSUBCLASS(WTrackBar,TRACKBAR_CLASS);
	IMPLEMENT_WINDOWSUBCLASS(WProgressBar,PROGRESS_CLASS);
	IMPLEMENT_WINDOWCLASS(WTerminal,CS_DBLCLKS);
	IMPLEMENT_WINDOWCLASS(WLog,CS_DBLCLKS);
	IMPLEMENT_WINDOWCLASS(WViewText,CS_DBLCLKS);
	IMPLEMENT_WINDOWCLASS(WPasswordDialog,CS_DBLCLKS);
	IMPLEMENT_WINDOWCLASS(WTextScrollerDialog,CS_DBLCLKS);
	IMPLEMENT_WINDOWCLASS(WProperties,CS_DBLCLKS);
	IMPLEMENT_WINDOWCLASS(WObjectProperties,CS_DBLCLKS);
	IMPLEMENT_WINDOWCLASS(WConfigProperties,CS_DBLCLKS);
	IMPLEMENT_WINDOWCLASS(WClassProperties,CS_DBLCLKS);
	IMPLEMENT_WINDOWCLASS(WWizardDialog,0);
	IMPLEMENT_WINDOWCLASS(WWizardPage,0);
	IMPLEMENT_WINDOWCLASS(WDragInterceptor,CS_DBLCLKS);
	IMPLEMENT_WINDOWCLASS(WPictureButton,CS_DBLCLKS);
	//WC_HEADER (InitCommonControls)
	//WC_TABCONTROL (InitCommonControls)
	//TOOLTIPS_CLASS (InitCommonControls)
	//TRACKBAR_CLASS (InitCommonControls)
	//UPDOWN_CLASS (InitCommonControls)
	//STATUSCLASSNAME (InitCommonControls)
	//TOOLBARCLASSNAME (InitCommonControls)

	// Create brushes.
	hBrushBlack    = CreateSolidBrush( RGB(0,0,0) );
	hBrushWhite    = CreateSolidBrush( RGB(255,255,255) );
	hBrushOffWhite = CreateSolidBrush( RGB(224,224,224) );
	hBrushHeadline = CreateSolidBrush( RGB(200,200,200) );
	hBrushCurrent  = CreateSolidBrush( RGB(0,0,128) );
	hBrushDark     = CreateSolidBrush( RGB(64,64,64) );
	hBrushGrey     = CreateSolidBrush( RGB(128,128,128) );

	// Create stipple brush.
	WORD Pat[8];
	for( INT i = 0; i < 8; i++ )
		Pat[i] = (WORD)(0x5555 << (i & 1));
	HBITMAP Bitmap = CreateBitmap( 8, 8, 1, 1, &Pat );
	check(Bitmap);
	hBrushStipple = CreatePatternBrush(Bitmap);
	DeleteObject(Bitmap);

	// stijn: initialize DPI settings
	HDC hDC = ::GetDC(NULL);
	WWindow::DPIX = ::GetDeviceCaps(hDC, LOGPIXELSX);
	WWindow::DPIY = ::GetDeviceCaps(hDC, LOGPIXELSY);

	// Create fonts.
#ifndef JAPANESE
	hFontText     = CreateFont( -MulDiv(9/*PointSize*/,  GetDeviceCaps(hDC, LOGPIXELSY), 72), 0, 0, 0, 0, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Arial") );
	hFontUrl      = CreateFont( -MulDiv(9/*PointSize*/,  GetDeviceCaps(hDC, LOGPIXELSY), 72), 0, 0, 0, 0, 0, 1, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Arial") );
	hFontHeadline = CreateFont( -MulDiv(15/*PointSize*/, GetDeviceCaps(hDC, LOGPIXELSY), 72), 0, 0, FW_BOLD, 1, 1, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Arial") );
#else
	hFontText     = (HFONT)( GetStockObject( DEFAULT_GUI_FONT ) );
	hFontUrl      = (HFONT)( GetStockObject( DEFAULT_GUI_FONT ) );
	hFontHeadline = (HFONT)( GetStockObject( DEFAULT_GUI_FONT ) );
#endif
	ReleaseDC( NULL, hDC );

	// Custom window messages.
	WindowMessageOpen       = RegisterWindowMessageW( TEXT("UnrealOpen") );
	WindowMessageMouseWheel = RegisterWindowMessageW( TEXT("MSWHEEL_ROLLMSG") );

	unguard;
}

// FExec interface.
UBOOL UWindowManager::Exec( const TCHAR* Cmd, FOutputDevice& Ar )
{
	guard(UWindowManager::Exec);
	return 0;
	unguard;
}

// UObject interface.
void UWindowManager::Serialize( FArchive& Ar )
{
	guard(UWindowManager::Serialize);
	Super::Serialize( Ar );
	INT i=0;
	for( i=0; i<WWindow::_Windows.Num(); i++ )
		WWindow::_Windows(i)->Serialize( Ar );
	for( i=0; i<WWindow::_DeleteWindows.Num(); i++ )
		WWindow::_DeleteWindows(i)->Serialize( Ar );
	for (i = 0; i < FTreeItem::DeleteItems.Num(); i++)
		FTreeItem::DeleteItems(i)->Serialize( Ar );
	unguard;
}
void UWindowManager::Destroy()
{
	guard(UWindowManager::Destroy);
	Super::Destroy();
	check(GWindowManager==this);
	GWindowManager = NULL;
	if( !GIsCriticalError )
		Tick( 0.0 );
	WWindow::_Windows.Empty();
	WWindow::_DeleteWindows.Empty();
	FTreeItem::DeleteItems.Empty();
	WProperties::PropertiesWindows.Empty();
	unguard;
}

// USubsystem interface.
void UWindowManager::Tick( FLOAT DeltaTime )
{
	guard(UWindowManager::Tick);
	// not delete items in the background because Windows can make app foreground by complicate rules
	int i = 0;
	while( WWindow::_DeleteWindows.Num() > i )
	{
		WWindow* W = WWindow::_DeleteWindows( i );
		HWND Parent = GetParent(W->hWnd);
		if (Parent)
		{
			if (IsWindowVisible(Parent) && ++i) // skip this one
				continue;
			Parent = GetParent(Parent);
			if (Parent && IsWindowVisible(Parent) && ++i) // case for combo box with inner edit
				continue;
		}
		delete W;
		check(WWindow::_DeleteWindows.FindItemIndex(W)==INDEX_NONE);
	}
	while ( FTreeItem::DeleteItems.Num() )
	{
		FTreeItem* T = FTreeItem::DeleteItems( 0 );
		delete T;
		check(FTreeItem::DeleteItems.FindItemIndex(T) == INDEX_NONE);
	}
	unguard;
}
IMPLEMENT_CLASS(UWindowManager);

/*-----------------------------------------------------------------------------
	Functions.
-----------------------------------------------------------------------------*/

WINDOW_API HBITMAP LoadFileToBitmap( const TCHAR* Filename, INT& SizeX, INT& SizeY )
{
	guard(LoadFileToBitmap);
	TArray<BYTE> Bitmap;
	if( appLoadFileToArray(Bitmap,Filename) )
	{
		HDC hDC				 = GetDC(NULL);
		BITMAPFILEHEADER* BH = (BITMAPFILEHEADER*)&Bitmap(0);
		BITMAPINFO* BI		 = (BITMAPINFO*)(&Bitmap(0) + sizeof(BITMAPFILEHEADER));
		void* RawData		 = &Bitmap(0) + BH->bfOffBits;
		SizeX                = BI->bmiHeader.biWidth;
		SizeY                = BI->bmiHeader.biHeight;
		HBITMAP      hBitmap = CreateDIBitmap( hDC, &BI->bmiHeader, CBM_INIT, RawData, BI, DIB_RGB_COLORS );
		ReleaseDC( NULL, hDC );
		return hBitmap;
	}
	return NULL;
	unguard;
}

WINDOW_API void InitWindowing()
{
	guard(InitWindowing);
	GWindowManager = new UWindowManager;
	GWindowManager->AddToRoot();
	unguard;
}

TArray<UBoolProperty*> PolyFlagsProps;

LRESULT WItemBox::WndProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (Message == WM_KEYDOWN && (wParam == VK_LEFT || wParam == VK_RIGHT))
	{
		// Collapse/expand current item if expandable
		INT Current = GetCurrent();
		if (Current >= 0)
		{
			FTreeItem* Item = dynamic_cast<FTreeItem*>((FTreeItemBase*)GetItemData(Current));
			if (Item && Item->Expandable && Item->Expanded != (wParam == VK_RIGHT))
			{
				if (!Item->Expanded)
					Item->Expand();
				else
					Item->Collapse();
			}
		}
		return 1;
	}
	else if (Message == WM_ERASEBKGND)
	{ // Don't erase background.
		guard(WItemBox::WM_ERASEBKGND);
		FRect Rect = GetClientRect();
		if (Rect.Height() % itemHeight != 0 && (GetCount() - GetTop())*itemHeight < Rect.Height())
		{ // need erase background after last item
			Rect = Rect.Bottom(Rect.Height() - (GetCount() - GetTop())*itemHeight);
			FillRect((HDC)wParam, Rect, OnCtlColorBtn((HDC)wParam, hWnd));
		}
		unguard;
		return 0;
	}
	else return WControl::WndProc(Message, wParam, lParam);
}

void FPropertyItem::Expand()
{
	guard(FPropertyItem::Expand);
	UStructProperty* StructProperty;
	UArrayProperty* ArrayProperty;
	if (Property->ArrayDim > 1 && ArrayIndex == -1)
	{
		// Expand array.
		Sorted = 0;
		for (INT i = 0; i < Property->ArrayDim; i++)
			Children.AddItem(new FPropertyItem(OwnerProperties, this, Property, Name, i * Property->ElementSize, i));
	}
	else if ((ArrayProperty = Cast<UArrayProperty>(Property)) != NULL)
	{
		// Expand array.
		Sorted = 0;
		FArray* Array = GetArrayAddress();
		if (Array)
			for (INT i = 0; i < Array->Num(); i++)
				Children.AddItem(new FPropertyItem(OwnerProperties, this, ArrayProperty->Inner, Name, i * ArrayProperty->Inner->ElementSize, i));
	}
	else if ((StructProperty = Cast<UStructProperty>(Property)) != NULL)
	{
		// Expand struct.
		for (TFieldIterator<UProperty> It(StructProperty->Struct); It; ++It)
			if (AcceptFlags(It->PropertyFlags))
				Children.AddItem(new FPropertyItem(OwnerProperties, this, *It, It->GetFName(), It->Offset, -1));
	}
	else if ((Cast<UIntProperty>(Property) && appStricmp(*Property->GetFName(),TEXT("PolyFlags")) == 0))
	{
		if (!PolyFlagsProps.Num())
		{
			UBoolProperty* Prop;
#define MAKE_BOOL_PROPERTY(name)										\
			Prop = new(UWindowManager::StaticClass(), TEXT(#name), RF_Public)UBoolProperty(EC_CppProperty, 0, *Property->Category, CPF_Config); \
			Prop->BitMask = name;										\
			PolyFlagsProps.AddItem(Prop)

			MAKE_BOOL_PROPERTY(PF_Invisible);
			MAKE_BOOL_PROPERTY(PF_Masked);
			MAKE_BOOL_PROPERTY(PF_Translucent);
			MAKE_BOOL_PROPERTY(PF_NotSolid);
			MAKE_BOOL_PROPERTY(PF_Environment);
			MAKE_BOOL_PROPERTY(PF_Semisolid);
			MAKE_BOOL_PROPERTY(PF_Modulated);
			MAKE_BOOL_PROPERTY(PF_FakeBackdrop);
			MAKE_BOOL_PROPERTY(PF_TwoSided);
			MAKE_BOOL_PROPERTY(PF_AutoUPan);
			MAKE_BOOL_PROPERTY(PF_AutoVPan);
			MAKE_BOOL_PROPERTY(PF_NoSmooth);
			MAKE_BOOL_PROPERTY(PF_SpecialPoly);
			MAKE_BOOL_PROPERTY(PF_SmallWavy);
			MAKE_BOOL_PROPERTY(PF_Flat);
			MAKE_BOOL_PROPERTY(PF_LowShadowDetail);
			MAKE_BOOL_PROPERTY(PF_NoMerge);
			MAKE_BOOL_PROPERTY(PF_CloudWavy);
			MAKE_BOOL_PROPERTY(PF_DirtyShadows);
			MAKE_BOOL_PROPERTY(PF_BrightCorners);
			MAKE_BOOL_PROPERTY(PF_SpecialLit);
			MAKE_BOOL_PROPERTY(PF_Gouraud);
			MAKE_BOOL_PROPERTY(PF_Unlit);
			MAKE_BOOL_PROPERTY(PF_HighShadowDetail);
			MAKE_BOOL_PROPERTY(PF_Portal);
			MAKE_BOOL_PROPERTY(PF_Mirrored);
#undef MAKE_BOOL_PROPERTY
		}
		// emulate expand.
		for (int i = 0; i < PolyFlagsProps.Num(); i++)
			Children.AddItem(new(TEXT("FPropertyItem"))FPropertyItem(OwnerProperties, this, PolyFlagsProps(i), *PolyFlagsProps(i)->GetFName(), 0, -1));
	}
	else if (Cast<UObjectProperty>(Property) != NULL)
	{
		// Expand object properties.
		UObject** Object = (UObject * *)GetReadAddress(Property);
		if (Object)
			Children.AddItem(new FEditObjectItem(OwnerProperties, this, Object, Property));
	}
	FTreeItem::Expand();
	unguard;
}

/*-----------------------------------------------------------------------------
	WRichEdit
-----------------------------------------------------------------------------*/
LRESULT WRichEdit::WndProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (Message == WM_MEASUREITEM)
	{
		LPMEASUREITEMSTRUCT lpMeasureItem = (LPMEASUREITEMSTRUCT)lParam;
		if (lpMeasureItem->CtlID == ChoicesList.ControlId)
		{
			lpMeasureItem->itemHeight = (int)SendMessage(hWnd, AEM_GETCHARSIZE, AECS_HEIGHT, 0);
			return TRUE;
		}
	}
	else if (Message == WM_DRAWITEM)
	{
		LPDRAWITEMSTRUCT lpDrawItem = (LPDRAWITEMSTRUCT)lParam;
		if (lpDrawItem->CtlID == ChoicesList.ControlId)
		{
			HFONT hFont = (HFONT)SendMessage(hWnd, AEM_GETFONT, AEGF_CURRENT, 0);
			SelectObject(lpDrawItem->hDC, hFont);
			LPTSTR lpText = (LPTSTR)lpDrawItem->itemData;
			if (lpDrawItem->itemState & ODS_SELECTED)
			{
				SetBkColor(lpDrawItem->hDC, RGB(127, 0, 127));
				SetTextColor(lpDrawItem->hDC, RGB(255, 255, 255));
			}
			else
			{
				SetBkColor(lpDrawItem->hDC, RGB(255, 255, 192));
				SetTextColor(lpDrawItem->hDC, RGB(0, 0, 0));
			}
			ExtTextOut(lpDrawItem->hDC,
				lpDrawItem->rcItem.left + 2, lpDrawItem->rcItem.top,
				ETO_OPAQUE | ETO_CLIPPED, &lpDrawItem->rcItem,
				lpText, lstrlen(lpText), NULL);
			if (false && lpDrawItem->itemState & ODS_FOCUS)
				DrawFocusRect(lpDrawItem->hDC, &lpDrawItem->rcItem);
			return TRUE;
		}
	}
	else if (Message == WM_DELETEITEM)
	{
		LPDELETEITEMSTRUCT lpDeleteItem = (LPDELETEITEMSTRUCT)lParam;
		if (lpDeleteItem->CtlID == ChoicesList.ControlId)
		{
			// stijn: wrong. points to constant mem
			// delete (LPTSTR)lpDeleteItem->itemData;
			return TRUE;
		}
	}
	else if (Message == WM_COMPAREITEM && wParam == ChoicesList.ControlId)
	{
		COMPAREITEMSTRUCT* Cmp = (COMPAREITEMSTRUCT*)lParam;

		return _wcsicmp((TCHAR*)Cmp->itemData1, (TCHAR*)Cmp->itemData2);
	}

	if (ChoicesList.m_bShow)
	{
		if (Message == WM_CHAR && wParam == VK_RETURN)
		{
			if (ChoicesList.GetCurrent() == -1)
				ChoicesList.SetCurrent(0, FALSE);
			AutocompleteSelect();
			return 1;
		}
		if (Message == WM_CHAR && wParam == VK_ESCAPE)
		{
			AutocompleteHide();
			return 1;
		}
		INT ChoicesCount = ChoicesList.GetCount();
		if (ChoicesCount > 1 && (Message == WM_CHAR || Message == WM_KEYDOWN || Message == WM_KEYUP) && 
			(wParam == VK_UP || wParam == VK_DOWN || wParam == VK_PRIOR /*PageUp*/ || wParam == VK_NEXT /*PageDown*/))
		{
			INT ChoicesCurrent = ChoicesList.GetCurrent();
			if (Message == WM_KEYDOWN && wParam == VK_UP && ChoicesCurrent <= 0)
			{
				ChoicesList.SetCurrent(ChoicesCount - 1, FALSE);
				return 1;
			}
			if (Message == WM_KEYDOWN && wParam == VK_DOWN && ChoicesCurrent == ChoicesCount - 1)
			{
				ChoicesList.SetCurrent(0, FALSE);
				return 1;
			}
			return ChoicesList.WndProc(Message, wParam, lParam);
		}
	}
	if (Message == WM_KEYDOWN)
	{
		if (IgnoreHotkeyDown && wParam == IgnoreHotkeyDown)
		{	
			IgnoreHotkeyChar = IgnoreHotkeyDown;
			IgnoreHotkeyDown = 0;
			return 0;
		}
	}
	else if (Message == WM_CHAR)
	{
		if (IgnoreHotkeyChar)
		{
			IgnoreHotkeyChar = 0;
			return 0;
		}
		if (wParam == VK_SPACE && HIWORD(GetKeyState(VK_CONTROL)))
		{
			Autocomplete();
			return 1;
		}
		// Auto indentation fix by Buggie
		if (wParam == VK_RETURN)
		{
			LRESULT ret = WControl::WndProc(Message, wParam, lParam);
			// Get the line number of the current selection (if any) or the caret if nothing is selected
			LRESULT Line = WControl::WndProc(EM_LINEFROMCHAR, (WPARAM)-1, NULL);
			TCHAR Buf[4096];
			// From MSDN: For EM_GETLINE, lParam is A pointer to the buffer that receives a
			// copy of the line. Before sending the message, set the first word of this buffer to the
			// size, in TCHARs, of the buffer. For ANSI text, this is the number of bytes; for Unicode
			// text, this is the number of characters. The size in the first word is overwritten by the
			// copied line.     
			Buf[0] = ARRAY_COUNT(Buf) - 10;
			DWORD Len = WControl::WndProc(EM_GETLINEW, Line - 1, (LPARAM)&Buf);
			if (Len < ARRAY_COUNT(Buf) - 5)
				Buf[Len] = 0;
			Buf[ARRAY_COUNT(Buf) - 5] = 0;
			// Keep all whitespace at the beginning of the line
			TCHAR* Ch = &Buf[0];
			while (*Ch == VK_TAB || *Ch == VK_SPACE)
				Ch++;
			*Ch = 0;
			// If the line had whitespace at the beginning, replace the selection with that whitespace
			// If nothing was selected, the whitespace is inserted at the caret position
			if (Buf[0] != 0)
				WControl::WndProc(EM_REPLACESEL, TRUE, (LPARAM)&Buf);
			return ret;
		}
		if (wParam == VK_TAB)
		{
			INT Start = 0;
			INT End = 0;
			WControl::WndProc(EM_GETSEL, (WPARAM)&Start, (LPARAM)&End);
			BOOL bShift = HIWORD(GetKeyState(VK_SHIFT));
			if (bShift || Start != End)
			{
				LRESULT FirstLine = WControl::WndProc(EM_EXLINEFROMCHAR, 0, Start);
				LRESULT LastLine = WControl::WndProc(EM_EXLINEFROMCHAR, 0, End);
				LRESULT FirstChar = WControl::WndProc(EM_LINEINDEX, FirstLine, 0);
				LRESULT LastChar = WControl::WndProc(EM_LINEINDEX, LastLine, 0) +
					WControl::WndProc(EM_LINELENGTH, End, 0) - 1;
				/*LRESULT LastChar = End + Last;
				if (!Last && LastLine != WControl::WndProc(EM_EXLINEFROMCHAR, 0, End - 1))
					LastChar--;
				*/
				WControl::WndProc(EM_SETSEL, FirstChar, LastChar);
				TArray<TCHAR> Buf;
				Buf.SetSize(LastChar - FirstChar + 10);

				WControl::WndProc(EM_GETSELTEXT, 0, (LPARAM)&Buf(0));
				Buf.AddItem('\0');

				FString InText(&Buf(0)), OutText;
				//debugf(TEXT("RTF1: %d %d %d %ls"), InText.InStr(TEXT("\r\n")), InText.InStr(TEXT("\n")), InText.InStr(TEXT("\r")), *InText);

				const TCHAR* NewLine = TEXT("\r");

				UBOOL Changed = FALSE;
				if (!bShift)
				{ // add
					Start++;
					Changed = TRUE;
					while (true)
					{
						const INT LineIndex = InText.InStr(NewLine);
						OutText += TEXT("\t");
						End++;
						if (LineIndex >= 0)
						{
							OutText += InText.Left(LineIndex);
							OutText += NewLine;
							InText = InText.Mid(LineIndex + lstrlenW(NewLine));
						}
						else
						{
							OutText += InText;
							break;
						}
					}
				}
				else
				{
					// remove
					BOOL bFirstLine = TRUE;
					while (true)
					{
						const INT LineIndex = InText.InStr(NewLine);
						if (LineIndex != 0)
						{
							const INT LineLength = LineIndex >= 0 ? LineIndex : InText.Len();
							FString Line = InText.Left(LineLength);

							// Remove one tab (with up to 3 spaces before the tab) or up to 4 spaces without a tab
							INT Pos = 0;
							TCHAR SawChar;
							INT Removed = 0;
							while (Pos < LineLength && Removed < 4)
							{
								SawChar = Line[Pos++];
								if (SawChar == '\t')
								{
									Removed++;
									break;
								}
								if (SawChar != ' ')
									break;
								Removed++;
							}

							OutText += Line.Mid(Removed);
							if (Removed > 0)
							{
								if (bFirstLine)
									Start -= Removed;
								End -= Removed;
								Changed = TRUE;
							}

							if (LineIndex <= 0)
								break;
						}
						OutText += NewLine;
						InText = InText.Mid(LineIndex + lstrlenW(NewLine));
						bFirstLine = FALSE;
					}
					if (Start < static_cast<INT>(FirstChar))
						Start = static_cast<INT>(FirstChar);
					if (Start > End)
						End = Start;
				}

				if (Changed)
				{
					WControl::WndProc(EM_REPLACESELW, TRUE, (LPARAM)&OutText[0]);
				}

				//debugf(TEXT("DBG: %d %d %d"), FirstChar, Start, End);
				WControl::WndProc(EM_SETSEL, Start, End);

				Buf.Empty();
				return 1;
			}
		}
	}

	if (Message == WM_ERASEBKGND || Message == WM_PAINT)
	{
		HDC hDC = GetDC(hWnd);
		if (hDC)
		{
			DWORD dwFlags = AERC_NORIGHT;
			DWORD crDrawBoardBk = GetSysColor(COLOR_MENU);
			HBRUSH hBrushBoard = CreateSolidBrush(crDrawBoardBk);

			RECT rcDraw, rcBrush;
			SendMessage(*this, AEM_GETERASERECT, dwFlags, (LPARAM)&rcDraw);

			rcDraw.right = rcDraw.left;
			rcDraw.left -= LinesBarWidth;

			HFONT hFont = (HFONT)SendMessage(hWnd, AEM_GETFONT, AEGF_CURRENT, 0);
			HFONT hFontOld = (HFONT)SelectObject(hDC, hFont);

			SetBkMode(hDC, TRANSPARENT);
			DWORD crDrawBoardText = GetSysColor(COLOR_MENUTEXT);
			SetBkColor(hDC, crDrawBoardBk);
			SetTextColor(hDC, crDrawBoardText);

			//Draw lines
			AECHARINDEX ciFirstLine;
			AECHARINDEX ciLastLine;
			SendMessage(hWnd, AEM_GETINDEX, AEGI_FIRSTVISIBLELINE, (LPARAM)&ciFirstLine);
			SendMessage(hWnd, AEM_GETINDEX, AEGI_LASTVISIBLELINE, (LPARAM)&ciLastLine);

			int nCharHeight = (int)SendMessage(hWnd, AEM_GETCHARSIZE, AECS_HEIGHT, 0);

			rcDraw.top = SendMessage(hWnd, AEM_VPOSFROMLINE, AECT_CLIENT, ciFirstLine.nLine);
			if (rcDraw.top > 0)
			{
				rcBrush.left = 0;
				rcBrush.right = rcDraw.right;
				rcBrush.top = 0;
				rcBrush.bottom = rcDraw.top;
				FillRect(hDC, &rcBrush, hBrushBoard);
			}

			FString szLineNumber;
			SIZE sizeLineNumber;
			POINT ptTextOut;
			while (ciFirstLine.nLine <= ciLastLine.nLine && rcDraw.top < rcDraw.bottom)
			{
				if (!SendMessage(hWnd, AEM_ISLINECOLLAPSED, ciFirstLine.nLine, 0))
				{
					szLineNumber = FString::Printf(TEXT("%d"), ciFirstLine.nLine + 1);
					if (!GetTextExtentPoint32W(hDC, *szLineNumber, szLineNumber.Len(), &sizeLineNumber))
						break;
					ptTextOut.x = rcDraw.right - sizeLineNumber.cx - MulDiv(3, DPIX, 96);
					ptTextOut.y = rcDraw.top;

					//Erase space before number
					rcBrush.left = 0;
					rcBrush.right = ptTextOut.x;
					rcBrush.top = rcDraw.top;
					rcBrush.bottom = rcDraw.top + nCharHeight;
					FillRect(hDC, &rcBrush, hBrushBoard);

					//Draw number
					rcBrush.left = ptTextOut.x;
					rcBrush.right = ptTextOut.x + sizeLineNumber.cx;
					ExtTextOutW(hDC, ptTextOut.x, ptTextOut.y, ETO_OPAQUE, &rcBrush, *szLineNumber, szLineNumber.Len(), NULL);

					//Erase space after number
					rcBrush.left = ptTextOut.x + sizeLineNumber.cx;
					rcBrush.right = rcDraw.right;
					FillRect(hDC, &rcBrush, hBrushBoard);

					//Next line
					rcDraw.top += nCharHeight;
					ciFirstLine.nLine++;
				}
				else
				{
					if (!SendMessage(hWnd, AEM_GETINDEX, AEGI_NEXTUNCOLLAPSEDLINE, (LPARAM)&ciFirstLine))
						break;
				}
			}

			if (rcDraw.top < rcDraw.bottom)
			{
				rcBrush.left = 0;
				rcBrush.right = rcDraw.right;
				rcBrush.top = rcDraw.top;
				rcBrush.bottom = rcDraw.bottom;
				FillRect(hDC, &rcBrush, hBrushBoard);
			}

			if (hFontOld) SelectObject(hDC, hFontOld);

			DeleteObject(hBrushBoard);
			ReleaseDC(hWnd, hDC);
		}
	}

	return WControl::WndProc(Message, wParam, lParam);
}

void WRichEdit::Autocomplete()
{
	guard(WRichEdit::Autocomplete);
	FString BeginWord;
	AEFINDTEXTW ft = { 0 };

	ft.dwFlags = AEFR_REGEXP;
	ft.pText = TEXT("[a-zA-Z_0-9]*");
	ft.dwTextLen = (UINT_PTR)-1;
	ft.nNewLine = AELB_ASIS;
	SendMessage(*this, AEM_GETINDEX, AEGI_FIRSTCHAR, (LPARAM)&ft.crSearch.ciMin);
	SendMessage(*this, AEM_GETINDEX, AEGI_FIRSTSELCHAR, (LPARAM)&ft.crSearch.ciMax);

	BOOL bFound = SendMessage(*this, AEM_FINDTEXTW, 0, (LPARAM)&ft);
	if (ft.nCompileErrorOffset)
	{
		debugf(TEXT("Failed compile autocomplete REGEX pattern '%ls', error on pos %d at '%ls'"),
			ft.pText, ft.nCompileErrorOffset, &ft.pText[ft.nCompileErrorOffset - 1]);
	}
	else if (bFound)
	{
		AETEXTRANGEW tr = { 0 };
		tr.cr = ft.crFound;
		tr.nNewLine = AELB_ASOUTPUT;
		tr.bFillSpaces = FALSE;
		tr.dwBufferMax = SendMessage(*this, AEM_GETTEXTRANGEW, 0, (LPARAM)&tr);

		if (tr.dwBufferMax)
		{
			TArray<TCHAR> Buf(tr.dwBufferMax);
			tr.pBuffer = &Buf(0);
			SendMessage(*this, AEM_GETTEXTRANGEW, 0, (LPARAM)&tr);

			BeginWord = &Buf(0);
		}
	}

	ChoicesList.SetRedraw(false);
	ChoicesList.Empty();
	WordLen = BeginWord.Len();
	if (WordLen)
	{
		if (WordLen < NAME_SIZE)
			for (INT i = 0; i < FName::GetMaxNames(); i++)
			{
				FNameEntry* Entry = FName::GetEntry(i);
				if (Entry && Entry->Name && Entry->Name[0] &&
					(!WordLen || !_wcsnicmp(Entry->Name, *BeginWord, WordLen)))
					ChoicesList.AddString(Entry->Name);
			}
	}
	ChoicesList.SetRedraw(true);


	POINT ptClient = { 0 };
	SendMessage(*this, AEM_POSFROMCHAR, (WPARAM)&ptClient, (LPARAM)(WordLen ? &ft.crFound.ciMin : &ft.crSearch.ciMax));
	ptClient.x -= 2; // left padding

	int nCharHeight = (int)SendMessage(hWnd, AEM_GETCHARSIZE, AECS_HEIGHT, 0);
	ptClient.y += nCharHeight;

	int ItemHeight = (int)SendMessage(ChoicesList.hWnd, LB_GETITEMHEIGHT, 0, 0);

	if (ChoicesList.GetCount())
	{
		ChoicesList.SetCurrent(0, TRUE);
		const INT Width = 200;
		INT Height = ItemHeight * Min(ChoicesList.GetCount(), 30);

		RECT R;
		::GetWindowRect(*this, &R);
		INT MaxBottom = R.bottom - R.top - GetSystemMetrics(SM_CYHSCROLL) - 10;
		if (Height > MaxBottom / 2)
			Height = (int)(MaxBottom / 2 / ItemHeight) * ItemHeight;

		if (ptClient.y + Height > MaxBottom)
			ptClient.y -= Height + nCharHeight;

		INT MaxRight = R.right - R.left - GetSystemMetrics(SM_CXVSCROLL) - 10;
		if (ptClient.x + Width > MaxRight)
			ptClient.x = MaxRight - Width;

		ChoicesList.MoveWindow(ptClient.x, ptClient.y, Width, Height, TRUE);
		ChoicesList.Show(1);
	}
	else if (ChoicesList.m_bShow)
		ChoicesList.Show(0);
	unguard;
}

void WRichEdit::AutocompleteSelect()
{
	guard(WRichEdit::AutocompleteSelect);
	FString Item = (TCHAR*)ChoicesList.GetItemData(ChoicesList.GetCurrent());
	AutocompleteHide();

	AECHARRANGE aecr;
	SendMessage(*this, AEM_EXGETSEL, (WPARAM)&aecr.ciMin, (LPARAM)&aecr.ciMax);
	aecr.ciMin.nCharInLine -= WordLen;
	SendMessage(*this, AEM_EXSETSEL, (WPARAM)&aecr.ciMin, (LPARAM)&aecr.ciMax);

	SendMessage(*this, EM_REPLACESELW, TRUE, (LPARAM)*Item);
	unguard;
}

void WRichEdit::AutocompleteHide()
{
	guard(WRichEdit::AutocompleteHide);
	ChoicesList.Show(0);
	if (WordLen)
		ChoicesList.Empty();
	unguard;
}

void WRichEdit::OpenWindow(UBOOL Visible, UBOOL ReadOnly)
{
	guard(WRichEdit::OpenWindow);
	PerformCreateWindowEx
	(
		WS_EX_CLIENTEDGE,
		NULL,
		WS_CHILD | (Visible ? WS_VISIBLE : 0) | ES_MULTILINE | ES_NOHIDESEL | ES_SUNKEN | ES_SAVESEL | WS_HSCROLL | WS_VSCROLL | ES_AUTOHSCROLL | ES_GLOBALUNDO,
		0, 0,
		0, 0,
		*OwnerWindow,
		(HMENU)ControlId,
		hInstance
	);
	SendMessageW(*this, AEM_SETTABSTOP, 4, 0);

	HFONT hFont = CreateFont(-11, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, TEXT("Courier New"));
	if (!hFont)
		hFont = (HFONT)GetStockObject(ANSI_FIXED_FONT);
	SetFont(hFont);
	// stijn: This needs to be set explicitly folder older versions of Windows as they use a 32K character limit otherwise
	SetTextLimit(0x7FFFFFFE);
	// detect urls
	SendMessage(*this, AEM_EXSETOPTIONS, AECOOP_OR, AECOE_DETECTURL);
	SendMessage(*this, AEM_SETEVENTMASK, 0, AENM_LINK | AENM_SELCHANGE);
	SetClassLong(*this, GCL_STYLE, GetClassLong(*this, GCL_STYLE) | CS_DBLCLKS);

	RECT rcErase;
	RECT rcDraw;
	DWORD dwFlags = AERC_NORIGHT | AERC_NOBOTTOM;

	SendMessage(*this, AEM_GETERASERECT, dwFlags, (LPARAM)&rcErase);
	SendMessage(*this, AEM_GETRECT, dwFlags, (LPARAM)&rcDraw);

	rcDraw.left += LinesBarWidth + MulDiv(1, DPIX, 96);
	rcErase.left += LinesBarWidth;

	SendMessage(*this, AEM_SETERASERECT, dwFlags, (LPARAM)&rcErase);
	SendMessage(*this, AEM_SETRECT, dwFlags | AERC_UPDATE, (LPARAM)&rcDraw);

	ChoicesList.OpenWindow(FALSE, TRUE, FALSE, TRUE, TRUE);

	LONG lStyle = GetWindowLong(ChoicesList.hWnd, GWL_STYLE);
	lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
	SetWindowLong(ChoicesList.hWnd, GWL_STYLE, lStyle);

	LONG lExStyle = GetWindowLong(ChoicesList.hWnd, GWL_EXSTYLE);
	lExStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
	SetWindowLong(ChoicesList.hWnd, GWL_EXSTYLE, lExStyle);

	unguard;
}

UBOOL WRichEdit::InterceptControlCommand(UINT Message, WPARAM wParam, LPARAM lParam)
{
	guard(WRichEdit::InterceptControlCommand);
	if (Message == WM_NOTIFY && wParam == ControlId && ((NMHDR*)lParam)->code == AEN_SELCHANGED && ChoicesList.m_bShow)
	{
		Autocomplete();
	}
	if (HIWORD(wParam) == EN_CHANGE)
	{
		ChangeDelegate();
		return 1;
	}
	else if (Message == WM_NOTIFY && wParam == ControlId && ((NMHDR*)lParam)->code == AEN_LINK)
	{
		AENLINK* LinkInfo = (AENLINK*)lParam;
		if (LinkInfo->uMsg == WM_LBUTTONDBLCLK)
		{
			AETEXTRANGEW tr = { 0 };
			tr.cr = LinkInfo->crLink;
			tr.pBuffer = NULL;
			tr.nNewLine = AELB_ASIS;
			tr.bFillSpaces = FALSE;

			tr.dwBufferMax = SendMessage(*this, AEM_GETTEXTRANGEW, 0, (LPARAM)&tr);
			TArray<TCHAR> URL;
			URL.SetSize(tr.dwBufferMax);
			tr.pBuffer = &URL(0);

			SendMessage(*this, AEM_GETTEXTRANGEW, 0, (LPARAM)&tr);

			ShellExecuteW(*this, L"open", &URL(0), NULL, NULL, SW_SHOWNORMAL);

			LinkInfo->nVisitCount++;
		}
		return 1;
	}
	else return 0;
	unguard;
}

void WRichEdit::SetTextLimit(int _Limit)
{
	guard(WRichEdit::SetTextLimit);
	SendMessageW(*this, EM_EXLIMITTEXT, 0, _Limit);
	unguard;
}

void WRichEdit::SetReadOnly(UBOOL ReadOnly)
{
	guard(WRichEdit::SetReadOnly);
	check(hWnd);
	SendMessageW(*this, EM_SETREADONLY, ReadOnly, 0);
	unguard;
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
