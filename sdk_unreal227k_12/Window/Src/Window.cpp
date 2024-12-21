/*=============================================================================
	Window.cpp: GUI window management code.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#pragma warning( disable : 4201 )
#define STRICT
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include "Core.h"
#include "Window.h"

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
WNDPROC WDblCheckListBox::SuperProc = NULL;
WNDPROC WTrackBar::SuperProc = NULL;
WNDPROC WProgressBar::SuperProc = NULL;
WNDPROC WComboBox::SuperProc = NULL;
WNDPROC WButton::SuperProc = NULL;
//WNDPROC WToolTip::SuperProc = NULL;
WNDPROC WCoolButton::SuperProc = NULL;
WNDPROC WUrlButton::SuperProc = NULL;
WNDPROC WCheckBox::SuperProc = NULL;
WNDPROC WVScrollBar::SuperProc = NULL;
WNDPROC WTreeView::SuperProc = NULL;
INT WWindow::ModalCount=0;
INT WWindow::DPIX = 0;
INT WWindow::DPIY = 0;
TArray<WWindow*> WWindow::_Windows;
TArray<WWindow*> WWindow::_DeleteWindows;
TArray<FTreeItem*> FTreeItem::DeleteItems;
TArray<WProperties*> WProperties::PropertiesWindows;
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
WINDOW_API HACCEL hAccel;
#if UNICODE
WINDOW_API NOTIFYICONDATAA NIDA;
WINDOW_API BOOL (WINAPI* Shell_NotifyIconWX)( DWORD dwMessage, PNOTIFYICONDATAW pnid )=NULL;
WINDOW_API BOOL (WINAPI* SHGetSpecialFolderPathWX)( HWND hwndOwner, LPTSTR lpszPath, INT nFolder, BOOL fCreate );
#endif

IMPLEMENT_PACKAGE(Window)

/*-----------------------------------------------------------------------------
	Window manager.
-----------------------------------------------------------------------------*/

W_IMPLEMENT_CLASS(WWindow)
W_IMPLEMENT_CLASS(WCustomWindowBase)
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
W_IMPLEMENT_CLASS(WEdit)
W_IMPLEMENT_CLASS(WRichEdit)
W_IMPLEMENT_CLASS(WTerminalBase)
W_IMPLEMENT_CLASS(WTerminal)
W_IMPLEMENT_CLASS(WLog)
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
W_IMPLEMENT_CLASS(WCheckBox)
W_IMPLEMENT_CLASS(WVScrollBar)
W_IMPLEMENT_CLASS(WTreeView)
W_IMPLEMENT_CLASS(WDblCheckListBox)

// Constructor.
UWindowManager::UWindowManager()
{
	guard(UWindowManager::UWindowManager);

	// Init common controls.
	InitCommonControls();

	// Get addresses of procedures that don't exist in Windows 95.
#if UNICODE
	HMODULE hModShell32 = GetModuleHandle( TEXT("SHELL32.DLL") );
	*(FARPROC*)&Shell_NotifyIconWX       = GetProcAddress( hModShell32, "Shell_NotifyIconW" );
	*(FARPROC*)&SHGetSpecialFolderPathWX = GetProcAddress( hModShell32, "SHGetSpecialFolderPathW" );
#endif

	// Save instance.
	hInstanceWindow = hInstance;

	LoadLibraryW(TEXT("RICHED32.DLL"));

	// Implement window classes.
	IMPLEMENT_WINDOWSUBCLASS(WListBox,TEXT("LISTBOX"));
	IMPLEMENT_WINDOWSUBCLASS(WItemBox,TEXT("LISTBOX"));
	IMPLEMENT_WINDOWSUBCLASS(WCheckListBox,TEXT("LISTBOX"));
	IMPLEMENT_WINDOWSUBCLASS(WDblCheckListBox, TEXT("LISTBOX"));
	IMPLEMENT_WINDOWSUBCLASS(WTabControl,WC_TABCONTROL);
	IMPLEMENT_WINDOWSUBCLASS(WLabel,TEXT("STATIC"));
	IMPLEMENT_WINDOWSUBCLASS(WCustomLabel,TEXT("STATIC"));
//		IMPLEMENT_WINDOWSUBCLASS(WListView,TEXT("SysListView32"));
	IMPLEMENT_WINDOWSUBCLASS(WEdit,TEXT("EDIT"));
	IMPLEMENT_WINDOWSUBCLASS(WRichEdit,TEXT("RICHEDIT"));
	IMPLEMENT_WINDOWSUBCLASS(WComboBox,TEXT("COMBOBOX"));
	IMPLEMENT_WINDOWSUBCLASS(WButton,TEXT("BUTTON"));
//		IMPLEMENT_WINDOWSUBCLASS(WToolTip,TOOLTIPS_CLASS);
	IMPLEMENT_WINDOWSUBCLASS(WCoolButton,TEXT("BUTTON"));
	IMPLEMENT_WINDOWSUBCLASS(WUrlButton,TEXT("BUTTON"));
	IMPLEMENT_WINDOWSUBCLASS(WCheckBox,TEXT("BUTTON"));
	IMPLEMENT_WINDOWSUBCLASS(WVScrollBar,TEXT("SCROLLBAR"));
	IMPLEMENT_WINDOWSUBCLASS(WTreeView,WC_TREEVIEW);
	IMPLEMENT_WINDOWSUBCLASS(WTrackBar,TRACKBAR_CLASS);
	IMPLEMENT_WINDOWSUBCLASS(WProgressBar,PROGRESS_CLASS);
	IMPLEMENT_WINDOWCLASS(WTerminal,CS_DBLCLKS);
	IMPLEMENT_WINDOWCLASS(WLog,CS_DBLCLKS);
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
	IMPLEMENT_WINDOWCLASS(WCustomWindowBase, 0);
	//WC_HEADER (InitCommonControls)
	//WC_TABCONTROL (InitCommonControls)
	//TOOLTIPS_CLASS (InitCommonControls)
	//TRACKBAR_CLASS (InitCommonControls)
	//UPDOWN_CLASS (InitCommonControls)
	//STATUSCLASSNAME (InitCommonControls)
	//TOOLBARCLASSNAME (InitCommonControls)
	//"RichEdit" (RICHED32.DLL)

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

	// Create fonts.
	HDC hDC       = GetDC( NULL );
	WWindow::DPIX = ::GetDeviceCaps(hDC, LOGPIXELSX);
	WWindow::DPIY = ::GetDeviceCaps(hDC, LOGPIXELSY);
	
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
		FTreeItem::DeleteItems(i)->Serialize(Ar);
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
	WProperties::PropertiesWindows.Empty();
	FTreeItem::DeleteItems.Empty();
	unguard;
}

// USubsystem interface.
void UWindowManager::Tick( FLOAT DeltaTime )
{
	guard(UWindowManager::Tick);
	while( WWindow::_DeleteWindows.Num() )
	{
		WWindow* W = WWindow::_DeleteWindows( 0 );
		delete W;
		check(WWindow::_DeleteWindows.FindItemIndex(W)==INDEX_NONE);
	}
	while (FTreeItem::DeleteItems.Num())
	{
		FTreeItem* T = FTreeItem::DeleteItems(0);
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
	HBITMAP Bitmap;
	TArray<BYTE> Data;

	if ( appLoadFileToArray( Data, Filename ) )
	{
		HDC hDC = GetDC(0);
		BITMAPFILEHEADER* Hdr = (BITMAPFILEHEADER*)&Data(0);
		BITMAPINFO* Info = (BITMAPINFO*)(&Data(0)+sizeof(BITMAPFILEHEADER));
		SizeX = Info->bmiHeader.biWidth;
		SizeY = Info->bmiHeader.biHeight;
		//void* Init = &Data(0)+sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+(1<<Info->bmiHeader.biBitCount)*sizeof(RGBQUAD);
		void* Init = &Data(0)+Hdr->bfOffBits;
		Bitmap = CreateDIBitmap(hDC,&Info->bmiHeader,CBM_INIT,Init,Info,DIB_RGB_COLORS);
		ReleaseDC(0,hDC);
	}
	else Bitmap = 0;

	return Bitmap;
	unguard;
}

class A;

void fcn( A* );

class A
{
public:
   virtual void f() = 0;
   A() { fcn( this ); }
};

class B : A
{
   void f() { }
};

void fcn( A* p )
{
   p->f();
}

WINDOW_API void InitWindowing()
{
	guard(InitWindowing);
	GWindowManager = new UWindowManager;
	GWindowManager->AddToRoot();
	unguard;
}

/*-----------------------------------------------------------------------------
	FHeaderItem.
-----------------------------------------------------------------------------*/

void FHeaderItem::Draw(HDC hDC)
{
	guard(FHeaderItem::Draw);
	FRect Rect = GetRect();

	// Draw background.
	FillRect(hDC, Rect, hBrushWhite);
	if (bDrawLightColor)
	{
		// Obtain light color.
		const FColor LightColor = GetLightColor(TRUE);

		DWORD ColorVal = (GET_COLOR_DWORD(LightColor) & 0x00FFFFFF); // Remove alpha channel.
		HBRUSH hBrush = CreateSolidBrush(COLORREF(ColorVal));
		FillRect(hDC, Rect + FRect(0, 1 - GetSelected(), 0, 0), hBrush);
		DeleteObject(hBrush);
	}
	else FillRect(hDC, Rect + FRect(0, 1 - GetSelected(), 0, 0), GetBackgroundBrush(GetSelected()));

	// Draw tree.
	DrawTreeLines(hDC, Rect);

	// Prep text.
	SetTextColor(hDC, GetTextColor(GetSelected()));
	SetBkMode(hDC, TRANSPARENT);

	// Draw name.
	FString C = GetCaption();

	if (!appStricmp(UObject::GetLanguage(), TEXT("jpt")))
		TextOutW(hDC, Rect.Min.X, Rect.Min.Y, const_cast<TCHAR*>(*C), C.Len());
	else
		DrawTextExW(hDC, const_cast<TCHAR*>(*C), C.Len(), FRect(Rect) + FRect(GetIndentPixels(1), 1, -1, 0), DT_END_ELLIPSIS | DT_LEFT | DT_SINGLELINE | DT_VCENTER, NULL);
	unguard;
}

void FHeaderItem::OnChooseHSLColorButton()
{
	guard(FHeaderItem::OnChooseColorButton);

	// Get the current color in HSL format and convert it to RGB.
	const FColor BaseRGB = GetLightColor(FALSE);

	// Open the color dialog.
	CHOOSECOLORA cc;
	static COLORREF acrCustClr[16];
	appMemzero(&cc, sizeof(cc));
	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = OwnerProperties->List;
	cc.lpCustColors = (LPDWORD)acrCustClr;
	cc.rgbResult = (GET_COLOR_DWORD(BaseRGB) & 0x00FFFFFF); // Remove alpha channel.
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;
	if (ChooseColorA(&cc) == TRUE)
	{
		const FVector HSL = FColor(GetRValue(cc.rgbResult), GetGValue(cc.rgbResult), GetBValue(cc.rgbResult)).GetHSL();
		TCHAR Buffer[16];

		// Once the user chooses a color, loop through the child items of this header item, and set
		// the appropriate values.  We have to do this, since I don't see how to get access to the
		// actual actors themselves.  NOTE : You have to have this header item expanded before this will work.
		for (int x = 0; x < Children.Num(); x++)
		{
			FTreeItem* Item = Children(x);

			if (Item->GetCaption() == TEXT("LightHue"))
			{
				appSnprintf(Buffer, ARRAY_COUNT(Buffer), TEXT("%i"), appRound(HSL.X * 255.f));
				Item->SetValue(Buffer);
			}
			else if (Item->GetCaption() == TEXT("LightSaturation"))
			{
				appSnprintf(Buffer, ARRAY_COUNT(Buffer), TEXT("%i"), appRound(HSL.Y * 255.f));
				Item->SetValue(Buffer);
			}
			else if (Item->GetCaption() == TEXT("LightBrightness"))
			{
				appSnprintf(Buffer, ARRAY_COUNT(Buffer), TEXT("%i"), appRound(HSL.Z * 255.f));
				Item->SetValue(Buffer);
			}
		}

		InvalidateRect(OwnerProperties->List, NULL, 0);
		UpdateWindow(OwnerProperties->List);
	}
	Redraw();
	unguard;
}

void FHeaderItem::OnItemSetFocus()
{
	guard(FHeaderItem::OnItemSetFocus);
	bDrawLightColor = (GetCaption() == TEXT("LightColor") && Children.Num() != 0);
	if (bDrawLightColor)
		AddButton(TEXT("Color"), FDelegate(this, (TDelegate)&FHeaderItem::OnChooseHSLColorButton));
	unguard;
}

void FHeaderItem::OnItemKillFocus(UBOOL Abort)
{
	guard(FHeaderItem::OnItemKillFocus);
	bDrawLightColor = FALSE;
	for (INT i = 0; i < Buttons.Num(); i++)
	{
		Buttons(i)->Show(0);
		Buttons(i)->DelayedDestroy();
	}
	Buttons.Empty();
	ButtonWidth = 0;
	Redraw();
	unguard;
}

FColor FHeaderItem::GetLightColor(UBOOL bFullBright)
{
	guard(FHeaderItem::GetLightColor);
	FColor LightColor(0, 0, 0, 0);
	UByteProperty* P = Cast<UByteProperty>(FindProperty(TEXT("LightHue")));
	if (P)
	{
		BYTE* Addr = GetReadAddress(P, this);
		if (Addr)
			LightColor.R = *(Addr + P->Offset);
		P = Cast<UByteProperty>(FindProperty(TEXT("LightSaturation")));
		if (P)
		{
			Addr = GetReadAddress(P, this);
			if (Addr)
				LightColor.G = *(Addr + P->Offset);
		}
		if (bFullBright)
			LightColor.B = 255;
		else
		{
			P = Cast<UByteProperty>(FindProperty(TEXT("LightBrightness")));
			if (P)
			{
				Addr = GetReadAddress(P, this);
				if (Addr)
					LightColor.B = *(Addr + P->Offset);
			}
		}
	}
	if (LightColor.IsZero())
		return LightColor;

	FPlane ColorPlane = FPlane::GetHSV(LightColor.R, LightColor.G, LightColor.B) * 1.217567f;
	return FColor(ColorPlane);
	unguard;
}

/*-----------------------------------------------------------------------------
	FPolyFlagItem.
-----------------------------------------------------------------------------*/

static const TCHAR* PolyFlagNames[] = { TEXT("Invisible"),TEXT("Masked"),TEXT("Translucent"),TEXT("NotSolid"),TEXT("Environment"),TEXT("SemiSolid"),TEXT("Modulated"),TEXT("FakeBackdrop"),TEXT("TwoSided"),
	TEXT("AutoUPan"),TEXT("AutoVPan"),TEXT("NoSmooth"),TEXT("SpecialPoly"),TEXT("SmallWavy"),TEXT("ForceViewZone"),TEXT("LowShadowDetail"),TEXT("NoMerge"),TEXT("AlphaBlend"),TEXT("DirtyShadows"),TEXT("BrightCorners"),
	TEXT("SpecialLit"),TEXT("NoBoundRejection"),TEXT("Unlit"),TEXT("HighShadowDetail"),NULL,NULL,TEXT("Portal"),TEXT("Mirrored"),NULL,TEXT("OccluderPoly") };

class FPolyFlagItem : public FPropertyItem
{
public:
	DWORD BitOffset;

	FPolyFlagItem(WPropertiesBase* InOwnerProperties, FTreeItem* InParent, UIntProperty* InProperty, FName InName, DWORD InBitOffset)
		: FPropertyItem(InOwnerProperties, InParent, InProperty, InName, 0, INDEX_NONE, FALSE)
		, BitOffset(InBitOffset)
	{
		bIsPolyFlags = FALSE;
		Expandable = FALSE;
		IsAButton = FALSE;
	}
	BYTE* GetReadAddress(UProperty* InProperty, FTreeItem* Top, INT Index)
	{
		guard(FPolyFlagItem::GetReadAddress);
		if (Index == INDEX_NONE)
		{
			BYTE* ResultAddr = NULL;
			DWORD Result = 0;
			const INT Num = GetItemCount();
			for (INT i = 0; i < Num; ++i)
			{
				BYTE* ReadValue = Parent->GetReadAddress(Property, this, i);
				if (ReadValue)
				{
					ResultAddr = ReadValue;
					if (*reinterpret_cast<DWORD*>(ReadValue) & BitOffset)
						Result |= 2;
					else Result |= 1;
				}
			}
			if (Result == 3)
				ResultAddr = NULL;
			return ResultAddr;
		}
		return Parent->GetReadAddress(InProperty, Top, Index);
		unguard;
	}
	DWORD GetCurrentValue()
	{
		guard(FPolyFlagItem::GetCurrentValue);
		DWORD Result = 0;
		DWORD* Current = reinterpret_cast<DWORD*>(GetReadAddress(Property, this, INDEX_NONE));
		if (Current)
			return ((*Current) & BitOffset) ? 1 : 0;
		return 2;
		unguard;
	}
	void GetPropertyText(FString& Str, BYTE* ReadValue, UBOOL bRealValue = 0)
	{
		guardSlow(FPolyFlagItem::GetPropertyText);
		Str = GetCurrentValue() ? GTrue : GFalse;
		unguardSlow;
	}
	void OnItemSetFocus()
	{
		guard(FPolyFlagItem::OnItemSetFocus);
		FTreeItem::OnItemSetFocus();
		if (!(Property->PropertyFlags & CPF_EditConst))
		{
			// Combo box.
			FRect Rect = GetRect() + FRect(0, 0, -1, -1);
			Rect.Min.X = OwnerProperties->GetDividerWidth();
			Rect.Max.X -= ButtonWidth;

			HolderControl = new WLabel(&OwnerProperties->List);
			HolderControl->Snoop = this;
			HolderControl->OpenWindow(0);
			FRect HolderRect = Rect.Right(20) + FRect(0, 0, 0, 1);
			HolderControl->MoveWindow(HolderRect, 0);

			Rect = Rect + FRect(-2, -6, -2, 0);

			ComboControl = new WComboBox(HolderControl);
			ComboControl->OpenWindow(FALSE);
			ComboControl->MoveWindow(Rect - HolderRect.Min, FALSE);

			ComboControl->Snoop = this;
			ComboControl->SelectionEndOkDelegate = FDelegate(this, (TDelegate)&FPropertyItem::ComboSelectionEndOk);
			ComboControl->SelectionEndCancelDelegate = FDelegate(this, (TDelegate)&FPropertyItem::ComboSelectionEndCancel);

			ComboControl->AddString(GFalse);
			ComboControl->AddString(GTrue);

			ReceiveFromControl();

			Redraw();
			ComboControl->Show(1);
			HolderControl->Show(1);
			SetFocusToItem();
		}
		else if (ArrayIndex == INDEX_NONE && !Property->IsA(UArrayProperty::StaticClass())) // Create a selectable textbox.
		{
			// Edit control.
			FRect Rect = GetRect();
			Rect.Min.X = 1 + OwnerProperties->GetDividerWidth();
			Rect.Min.Y--;
			Rect.Max.X -= ButtonWidth;
			EditControl = new WEdit(&OwnerProperties->List);
			EditControl->Snoop = this;
			EditControl->OpenWindow(0, 0, 1);
			EditControl->MoveWindow(Rect + FRect(0, 1, 0, 1), 0);

			ReceiveFromControl();

			Redraw();
			if (EditControl)
				EditControl->Show(1);
			SetFocusToItem();
		}
		NotifyEditProperty(Property, GetParentProperty(), GetArrayIndex());
		unguard;
	}
	void SetValue(const TCHAR* Value)
	{
		guard(FPolyFlagItem::SetValue);
		const INT Num = GetItemCount();
		const UBOOL bEnabled = (!appStricmp(Value, TEXT("True")) || !appStricmp(Value, GTrue) || !appStrcmp(Value, TEXT("1")));
		for (INT i = 0; i < Num; ++i)
		{
			BYTE* ReadValue = Parent->GetReadAddress(Property, this, i);
			if (ReadValue)
			{
				if (bEnabled)
					*reinterpret_cast<DWORD*>(ReadValue) |= BitOffset;
				else *reinterpret_cast<DWORD*>(ReadValue) &= ~BitOffset;
				NotifyEditProperty(Property, Property, i);
			}
		}
		ReceiveFromControl();
		Redraw();
		Parent->Redraw();
		unguard;
	}
	void ReceiveFromControl()
	{
		guard(FPolyFlagItem::ReceiveFromControl);
		ComboChanged = 0;
		if (EditControl)
		{
			FString Str;
			GetPropertyText(Str, NULL);
			EditControl->SetText(*Str);
			if (EditControl->hWnd)
				EditControl->SetSelection(0, Str.Len());
		}
		if (ComboControl)
		{
			DWORD Current = GetCurrentValue();
			ComboControl->SetCurrent((Current < 2) ? Current : INDEX_NONE);
			ComboChanged = 0;
		}
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	FPropertyItem.
-----------------------------------------------------------------------------*/

FPropertyItem::FPropertyItem(WPropertiesBase* InOwnerProperties, FTreeItem* InParent, UProperty* InProperty, FName InName, INT InOffset, INT InArrayIndex, UBOOL bIsMap)
	: FTreeItem(InOwnerProperties, InParent, 0)
	, Property(InProperty)
	, Name(InName)
	, Offset(InOffset)
	, ArrayIndex(InArrayIndex)
	, OldWidth(0)
	, EditControl(NULL)
	, KeyEditControl(NULL)
	, TrackControl(NULL)
	, ComboControl(NULL)
	, HolderControl(NULL)
	, ComboChanged(0)
	, bRadiiAng(0)
	, bIsMapValue(bIsMap)
	, bIsLightColor(FALSE)
	, bIsPolyFlags(FALSE)
{
	guard(FPropertyItem::FPropertyItem);
	if
		((Cast<UStructProperty>(InProperty)) || (InProperty->GetClass()==UObjectProperty::StaticClass()) || (InProperty->GetClass() == UClassProperty::StaticClass() && !(InProperty->PropertyFlags & CPF_EditConst))
			|| (Cast<UArrayProperty>(InProperty) && GetArrayAddress()) || InProperty->IsA(UMapProperty::StaticClass())
			|| (InProperty->ArrayDim > 1 && InArrayIndex == -1))
		Expandable = TRUE;

	if (Expandable && Cast<UClassProperty>(Property) && appStricmp(*Property->Category, TEXT("Drivers")) == 0)
		Expandable = FALSE;

	if (!Expandable)
	{
		if (InProperty->IsA(UByteProperty::StaticClass()) && !reinterpret_cast<UByteProperty*>(InProperty)->Enum && (!appStricmp(InProperty->GetName(), TEXT("LightHue")) || !appStricmp(InProperty->GetName(), TEXT("LightSaturation")))
			&& !appStricmp(InProperty->GetOwnerClass()->GetName(),TEXT("Actor")))
		{
			bIsLightColor = TRUE;
		}
		if (InProperty->IsA(UButtonProperty::StaticClass()))
		{
			IsAButton = TRUE;
			AddButton(*((UButtonProperty*)InProperty)->Caption, FDelegate(this, (TDelegate)&FPropertyItem::OnClearButton));
		}
		else if (InProperty->GetClass() == UIntProperty::StaticClass() && !appStricmp(*InName, TEXT("PolyFlags")))
		{
			bIsPolyFlags = TRUE;
			Expandable = TRUE;
		}
		else IsAButton = FALSE;
	}
	unguard;
}

void FPropertyItem::Expand()
{
	guard(FPropertyItem::Expand);
	if (Property->ArrayDim > 1 && ArrayIndex == -1)
	{
		// Expand array.
		Sorted = 0;
		for (INT i = 0; i < Property->ArrayDim; i++)
			Children.AddItem(new FPropertyItem(OwnerProperties, this, Property, Name, i * Property->ElementSize, i));
	}
	else if (Property->IsA(UArrayProperty::StaticClass()))
	{
		// Expand array.
		UArrayProperty* ArrayProperty = (UArrayProperty*)Property;
		Sorted = 0;
		FArray* Array = GetArrayAddress();
		if (Array)
			for (INT i = 0; i < Array->Num(); i++)
				Children.AddItem(new FPropertyItem(OwnerProperties, this, ArrayProperty->Inner, Name, i * ArrayProperty->Inner->ElementSize, i));
	}
	else if (Property->IsA(UMapProperty::StaticClass()))
	{
		// Expand array.
		UMapProperty* MapProperty = (UMapProperty*)Property;
		BYTE* AdrV;
		if (Parent && (AdrV = GetReadAddress(Property, this)) != NULL)
		{
			FScriptMapBase* FMap = (FScriptMapBase*)AdrV;
			FPropertyItem* Item;
			for (INT i = 0; i < FMap->Num(); i++)
			{
				Item = new FPropertyItem(OwnerProperties, this, MapProperty->Value, Name, 0, i, 1);
				Children.AddItem(Item);
				MapProperty->Key->ExportText(0, Item->Text, FMap->Pairs(i).Key, NULL, 0);
			}
		}
	}
	else if (Property->IsA(UStructProperty::StaticClass()))
	{
		UStructProperty* StructProperty = (UStructProperty*)Property;
		if (StructProperty->Struct->GetFName() == NAME_Rotator && !GSys->UseRegularAngles)
		{
			// Expand rotator with angle radiis.
			for (TFieldIterator<UProperty> It(StructProperty->Struct); It; ++It)
			{
				FPropertyItem* PropItem = new FPropertyItem(OwnerProperties, this, *It, It->GetFName(), It->Offset, -1);
				PropItem->bRadiiAng = 1;
				Children.AddItem(PropItem);
			}
		}
		else
		{
			// Expand struct.
			for (TFieldIterator<UProperty> It(StructProperty->Struct); It; ++It)
				if (AcceptFlags(It->PropertyFlags))
					Children.AddItem(new FPropertyItem(OwnerProperties, this, *It, It->GetFName(), It->Offset, -1));
		}
	}
	else if (Cast<UObjectProperty>(Property) != NULL)
	{
		// Expand object properties.
		UObject** Object = (UObject**)GetReadAddress(Property, this);
		if (Object)
			Children.AddItem(new FEditObjectItem(OwnerProperties, this, Object, Property));
	}
	else if (bIsPolyFlags)
	{
		UIntProperty* IntProp = CastChecked<UIntProperty>(Property);
		for (INT i = 0; i < ARRAY_COUNT(PolyFlagNames); ++i)
			if (PolyFlagNames[i])
				Children.AddItem(new FPolyFlagItem(OwnerProperties, this, IntProp, FName(PolyFlagNames[i]), (DWORD)(1 << i)));
	}
	FTreeItem::Expand();
	unguard;
}
BYTE* FPropertyItem::GetReadAddress(UProperty* InProperty, FTreeItem* Top, INT Index)
{
	guard(FPropertyItem::GetReadAddress);
	if (!Parent)
		return NULL;
	BYTE* AdrV = Parent->GetReadAddress(InProperty, Top, Index);
	if (!AdrV)
		return NULL;
	if (bIsMapValue)
	{
		FScriptMapBase* FMap = (FScriptMapBase*)AdrV;
		AdrV = FMap->Pairs(ArrayIndex).Value;
	}
	AdrV += Offset;
	if (Property && Property->IsA(UArrayProperty::StaticClass()))
		return (BYTE*)((FArray*)AdrV)->GetData();
	return AdrV;
	unguard;
}
UProperty* FPropertyItem::GetParentProperty()
{
	if (Parent)
	{
		UProperty* P = Parent->GetParentProperty();
		return P ? P : Property;
	}
	return Property;
}

void FPropertyItem::Serialize(FArchive& Ar)
{
	guard(FPropertyItem::Serialize);
	FTreeItem::Serialize(Ar);
	Ar << Property << Name;
	unguard;
}

void FPropertyItem::OnItemLeftMouseDown(FPoint P)
{
	guard(FPropertyItem::OnItemLeftMouseDown);
	P = OwnerProperties->GetCursorPos() - GetRect().Min;
	if (Abs(P.X - OwnerProperties->GetDividerWidth()) <= 2)
	{
		OwnerProperties->BeginSplitterDrag();
		throw TEXT("NoRoute");
	}
	else FTreeItem::OnItemLeftMouseDown(P);
	unguard;
}

void FPropertyItem::NotifyListResize()
{
	guard(FPropertyItem::NotifyListResize);
	if (IsAButton)
	{
		if (OldWidth!=INDEX_NONE && Buttons.Num())
		{
			OldWidth = INDEX_NONE;
			Buttons(0)->MoveWindow(0, 0, 0, 0, 1);
		}
	}
	unguard;
}

static FPropertyItem* PendingItem;
static void RC_CopyProperty(INT ID)
{
	if (ID == 0)
	{
		if (PendingItem->bIsMapValue)
			appClipboardCopy(*PendingItem->Text);
		else appClipboardCopy(*PendingItem->Name);
	}
	else if (ID == 3)
	{
		FString Result;
		if(PendingItem->CopyFullProperties(&Result))
			appClipboardCopy(*Result);
	}
	else if (!PendingItem->IsAButton)
	{
		BYTE* ReadValue = PendingItem->GetReadAddress(PendingItem->Property, PendingItem);
		if (ReadValue)
		{
			// Text.
			FString Res;
			PendingItem->GetPropertyText(Res, ReadValue, TRUE);
			if (ID == 1)
				appClipboardCopy(*Res);
			else
			{
				FString PropName;
				if (PendingItem->bIsMapValue)
					PropName = PendingItem->Text;
				else if (PendingItem->ArrayIndex == -1)
					PropName = *PendingItem->Name;
				else PropName = FString::Printf(TEXT("%ls[%i]"), *PendingItem->Name, PendingItem->ArrayIndex);
				appClipboardCopy(*(PropName + TEXT("=") + Res));
			}
		}
	}
}
void FPropertyItem::OnItemRightMouseDown(FPoint P)
{
	guard(FPropertyItem::OnItemRightMouseDown);
	static FPersistentRCMenu Menu;
	static FMenuItem* CopyFullItem;

	if (!Menu.bMenuCreated)
	{
		Menu.AddItem(TEXT("Copy name"), &RC_CopyProperty, 0);
		Menu.AddItem(TEXT("Copy value"), &RC_CopyProperty, 1);
		Menu.AddItem(TEXT("Copy name and value"), &RC_CopyProperty, 2);
		Menu.AddLineBreak();
		CopyFullItem = Menu.AddItem(TEXT("Copy object properties"), &RC_CopyProperty, 3);
	}
	CopyFullItem->SetDisabled(CopyFullProperties(NULL) == FALSE);
	PendingItem = this;
	Menu.OpenMenu(OwnerProperties->hWnd);
	unguard;
}
void FPropertyItem::GetPropertyText(FString& Str, BYTE* ReadValue, UBOOL bRealValue)
{
	guard(FPropertyItem::GetPropertyText);
	if (Cast<UClassProperty>(Property) && appStricmp(*Property->Category, TEXT("Drivers")) == 0)
	{
		// Class config.
		FString Path, Left, Right;
		GConfig->GetString(Property->GetOwnerClass()->GetPathName(), Property->GetName(), Path);
		if (Path.Divide(TEXT("."), Left, Right))
			Str = Localize(*Right, TEXT("ClassCaption"), *Left);
		else Str = Localize("Language", "Language", TEXT("Core"), *Path);
	}
	else
	{
		// Regular property.
		Property->ExportText(0, Str, ReadValue - Property->Offset, ReadValue - Property->Offset, (bRealValue ? PPF_Delimited : (bRadiiAng ? PPF_RotDegree : PPF_Localized)) | PPF_PropWindow);
	}
	unguard;
}
void FPropertyItem::SetValue(const TCHAR* Value)
{
	guard(FPropertyItem::SetValue);
	SetProperty(Property, this, Value, bRadiiAng ? PPF_RotDegree : 0);
	ReceiveFromControl();
	Redraw();

	if (bIsPolyFlags && Expanded && Children.Num())
	{
		for (INT i = 0; i < Children.Num(); ++i)
			Children(i)->Redraw();
	}
	unguard;
}
COLORREF FPropertyItem::GetTextColor(UBOOL Selected)
{
	guard(FPropertyItem::GetTextColor);
	if (Property->PropertyFlags & CPF_EditConst)
		return Selected ? RGB(255, 128, 128) : RGB(128, 0, 0);
	return Selected ? RGB(255, 255, 255) : RGB(0, 0, 0);
	unguard;
}
void FPropertyItem::Draw(HDC hDC)
{
	guard(FPropertyItem::Draw);

	FRect Rect = GetRect();
	TCHAR Str[4096];//!!

	// Draw background.
	FillRect(hDC, Rect, hBrushWhite);

	// Draw left background.
	FRect LeftRect = Rect;
	LeftRect.Max.X = OwnerProperties->GetDividerWidth();
	FillRect(hDC, LeftRect + FRect(0, 1 - GetSelected(), -1, 0), GetBackgroundBrush(GetSelected()));
	LeftRect.Min.X += GetIndentPixels(1);

	// Draw tree lines.
	DrawTreeLines(hDC, Rect);

	// Setup text.
	SetBkMode(hDC, TRANSPARENT);

	// Draw left text.
	SetTextColor(hDC, GetTextColor(GetSelected()));
	if (bIsMapValue)
		DrawTextExW(hDC, (LPWSTR)*Text, Text.Len(), LeftRect + FRect(0, 1, 0, 0), DT_END_ELLIPSIS | DT_LEFT | DT_SINGLELINE | DT_VCENTER, NULL);
	else
	{
		if (ArrayIndex == -1) appStrcpy(Str, *Name);
		else appSprintf(Str, TEXT("[%i]"), ArrayIndex);
		DrawTextExW(hDC, Str, appStrlen(Str), LeftRect + FRect(0, 1, 0, 0), DT_END_ELLIPSIS | DT_LEFT | DT_SINGLELINE | DT_VCENTER, NULL);
	}

	// Draw right background.
	FRect RightRect = Rect;
	RightRect.Min.X = OwnerProperties->GetDividerWidth();
	FillRect(hDC, RightRect + FRect(0, 1, 0, 0), GetBackgroundBrush(0));

	// Draw right text.
	if (!IsAButton)
	{
		RightRect.Max.X -= ButtonWidth;
		BYTE* ReadValue = GetReadAddress(Property, this);
		SetTextColor(hDC, GetTextColor(0));
		if ((Property->ArrayDim != 1 && ArrayIndex == -1) || Cast<UArrayProperty>(Property))
		{
			// Array expander.
			TCHAR* StrTmp = TEXT("...");
			DrawTextExW(hDC, StrTmp, appStrlen(StrTmp), RightRect + FRect(4, 0, 0, 1), DT_END_ELLIPSIS | DT_LEFT | DT_SINGLELINE | DT_VCENTER, NULL);
		}
		else if (ReadValue && Cast<UStructProperty>(Property) && Cast<UStructProperty>(Property)->Struct->GetFName() == NAME_Color)
		{
			// Color.
			FillRect(hDC, RightRect + FRect(4, 4, -4, -4), hBrushBlack);
			DWORD ColorVal = (*(DWORD*)ReadValue) & 0x00FFFFFF; // Remove alpha channel.
			HBRUSH hBrush = CreateSolidBrush(COLORREF(ColorVal));
			FillRect(hDC, RightRect + FRect(5, 5, -5, -5), hBrush);
			DeleteObject(hBrush);
		}
		else if (ReadValue)
		{
			// Text.
			FString Res;
			GetPropertyText(Res, ReadValue);
			appStrncpy(Str, *Res, 4095);
			DrawTextExW(hDC, Str, appStrlen(Str), RightRect + FRect(4, 1, 0, 0), DT_END_ELLIPSIS | DT_LEFT | DT_SINGLELINE | DT_VCENTER, NULL);
		}
	}
	else
	{
		INT olTag = RightRect.Max.X | (RightRect.Min.X << 8) | (RightRect.Min.Y << 16);
		if ((OldWidth != olTag) && Buttons.Num())
		{
			OldWidth = olTag;
			Buttons(0)->MoveWindow(RightRect, 1);
		}
	}

	unguard;
}
void FPropertyItem::SetFocusToItem()
{
	guard(FPropertyItem::SetFocusToItem);
	if(KeyEditControl)
		SetFocus(*KeyEditControl);
	else if (EditControl)
		SetFocus(*EditControl);
	else if (TrackControl)
		SetFocus(*TrackControl);
	else if (ComboControl)
		SetFocus(*ComboControl);
	unguard;
}
void FPropertyItem::OnItemDoubleClick()
{
	guard(FProperty::OnItemDoubleClick);
	if (!Property->IsA(UArrayProperty::StaticClass()))
		Advance(); // Cause for crash on expand...
	FTreeItem::OnItemDoubleClick();
	unguard;
}

inline INT Compare(const FName& A, const FName& B)
{
	return appStricmp(*A, *B);
}

void FPropertyItem::OnItemSetFocus()
{
	guard(FPropertyItem::OnItemSetFocus);
	check(!EditControl);
	check(!TrackControl);
	FTreeItem::OnItemSetFocus();
	IsFreeComboEdit = FALSE;
	if (Property->ArrayDim == 1 || ArrayIndex != -1)
	{
		if (!(Property->PropertyFlags & CPF_EditConst))
		{
			const UBOOL bIsDrivers = (Cast<UClassProperty>(Property) && !appStricmp(*Property->Category, TEXT("Drivers")));

			if (!bIsDrivers)
			{
				if (Property->IsA(UArrayProperty::StaticClass()) || Property->IsA(UMapProperty::StaticClass()))
				{
					if (Expandable)
						AddButton(*LocalizeGeneral(TEXT("AddButton"), TEXT("Window")), FDelegate(this, (TDelegate)&FPropertyItem::OnArrayAdd));
					AddButton(*LocalizeGeneral(TEXT("EmptyButton"), TEXT("Window")), FDelegate(this, (TDelegate)&FPropertyItem::OnArrayEmpty));
				}
				if (Cast<UArrayProperty>(Property->GetOuter()))
				{
					if (Parent->Expandable)
					{
						AddButton(*LocalizeGeneral(TEXT("InsertButton"), TEXT("Window")), FDelegate(this, (TDelegate)&FPropertyItem::OnArrayInsert));
						AddButton(*LocalizeGeneral(TEXT("DeleteButton"), TEXT("Window")), FDelegate(this, (TDelegate)&FPropertyItem::OnArrayDelete));
					}
				}
				else if (bIsMapValue && Parent->Expandable)
				{
					AddButton(*LocalizeGeneral(TEXT("EditButton"), TEXT("Window")), FDelegate(this, (TDelegate)&FPropertyItem::OnArrayInsert));
					AddButton(*LocalizeGeneral(TEXT("DeleteButton"), TEXT("Window")), FDelegate(this, (TDelegate)&FPropertyItem::OnArrayDelete));
				}

				if (Property->IsA(UStructProperty::StaticClass()) && appStricmp(Cast<UStructProperty>(Property)->Struct->GetName(), TEXT("Color")) == 0)
				{
					// Color.
					AddButton(*LocalizeGeneral(TEXT("BrowseButton"), TEXT("Window")), FDelegate(this, (TDelegate)&FPropertyItem::OnChooseColorButton));
				}
				else if (Property->IsA(UObjectProperty::StaticClass()))
				{
					// Class.
					AddButton(*LocalizeGeneral(TEXT("BrowseButton"), TEXT("Window")), FDelegate(this, (TDelegate)&FPropertyItem::OnBrowseButton));
					AddButton(*LocalizeGeneral(TEXT("UseButton"), TEXT("Window")), FDelegate(this, (TDelegate)&FPropertyItem::OnUseCurrentButton));
					AddButton(*LocalizeGeneral(TEXT("ClearButton"), TEXT("Window")), FDelegate(this, (TDelegate)&FPropertyItem::OnClearButton));
				}
			}

			if (Property->IsA(UByteProperty::StaticClass()) && !reinterpret_cast<UByteProperty*>(Property)->Enum)
			{
				// Slider.
				FRect Rect = GetRect();
				Rect.Min.X = 28 + OwnerProperties->GetDividerWidth();
				Rect.Min.Y--;
				Rect.Max.X -= ButtonWidth;
				TrackControl = new WTrackBar(&OwnerProperties->List);
				TrackControl->ThumbTrackDelegate = FDelegate(this, (TDelegate)&FPropertyItem::OnTrackBarThumbTrack);
				TrackControl->ThumbPositionDelegate = FDelegate(this, (TDelegate)&FPropertyItem::OnTrackBarThumbPosition);
				TrackControl->OpenWindow(0);
				TrackControl->SetTicFreq(32);
				TrackControl->SetRange(0, 255);
				TrackControl->MoveWindow(Rect, 0);
			}

			UBOOL bHideEditBox = FALSE;
			if
				((Property->IsA(UBoolProperty::StaticClass()))
					|| (Property->IsA(UByteProperty::StaticClass()) && reinterpret_cast<UByteProperty*>(Property)->Enum)
					|| (Property->IsA(UNameProperty::StaticClass()) && Name == NAME_InitialState)
					|| (Property->IsA(UNameProperty::StaticClass()) && !appStricmp(*Name, TEXT("Orders")) && !appStricmp(Property->GetOuter()->GetName(), TEXT("ScriptedPawn")))
					|| bIsDrivers)
			{
				bHideEditBox = TRUE;
				IsFreeComboEdit = (Property->IsA(UNameProperty::StaticClass()) && !appStricmp(*Name, TEXT("Orders")));

				// Combo box.
				FRect Rect = GetRect() + FRect(0, 0, -1, -1);
				Rect.Min.X = OwnerProperties->GetDividerWidth();
				Rect.Max.X -= ButtonWidth;

				HolderControl = new WLabel(&OwnerProperties->List);
				HolderControl->Snoop = this;
				HolderControl->OpenWindow(0);
				FRect HolderRect = Rect.Right(20) + FRect(0, 0, 0, 1);
				HolderControl->MoveWindow(HolderRect, 0);

				Rect = Rect + FRect(-2, -6, -2, 0);

				ComboControl = new WComboBox(HolderControl);
				ComboControl->OpenWindow(FALSE);
				ComboControl->MoveWindow(Rect - HolderRect.Min, FALSE);
				
				ComboControl->Snoop = this;
				ComboControl->SelectionEndOkDelegate = FDelegate(this, (TDelegate)&FPropertyItem::ComboSelectionEndOk);
				ComboControl->SelectionEndCancelDelegate = FDelegate(this, (TDelegate)&FPropertyItem::ComboSelectionEndCancel);

				if (Property->IsA(UBoolProperty::StaticClass()))
				{
					ComboControl->AddString(GFalse);
					ComboControl->AddString(GTrue);
				}
				else if (Property->IsA(UByteProperty::StaticClass()))
				{
					for (INT i = 0; i < Cast<UByteProperty>(Property)->Enum->Names.Num(); i++)
						ComboControl->AddString(*Cast<UByteProperty>(Property)->Enum->Names(i));
				}
				else if (Property->IsA(UNameProperty::StaticClass()) && Name == NAME_InitialState)
				{
					UClass* BaseClass = GetBaseClass();
					TArray<FName> States;
					if (BaseClass)
					{
						for (TFieldIterator<UState> StateIt(BaseClass); StateIt; ++StateIt)
							if (StateIt->StateFlags & STATE_Editable)
								States.AddUniqueItem(StateIt->GetFName());
					}
					Sort(States);
					ComboControl->AddString(*FName(NAME_None));
					for (INT i = 0; i < States.Num(); i++)
						ComboControl->AddString(*States(i));
				}
				else if (Property->IsA(UNameProperty::StaticClass()) && !appStricmp(*Name, TEXT("Orders")))
				{
					UClass* BaseClass = GetBaseClass();
					TArray<FName> States;
					if (BaseClass)
					{
						for (TFieldIterator<UState> StateIt(BaseClass); StateIt; ++StateIt)
							States.AddUniqueItem(StateIt->GetFName());
					}
					Sort(States);
					ComboControl->AddString(*FName(NAME_None));
					for (INT i = 0; i < States.Num(); i++)
						ComboControl->AddString(*States(i));
				}
				else if (bIsDrivers)
				{
					UClassProperty* ClassProp = CastChecked<UClassProperty>(Property);
					TArray<FRegistryObjectInfo> Classes;
					UObject::GetRegistryObjects(Classes, UClass::StaticClass(), ClassProp->MetaClass, 0);
					for (INT i = 0; i < Classes.Num(); i++)
					{
						FString Path = Classes(i).Object, Left, Right;
						if (Path.Divide(TEXT("."), Left, Right))
							ComboControl->AddString(*Localize(*Right, TEXT("ClassCaption"), *Left));
						else
							ComboControl->AddString(*Localize(TEXT("Language"), TEXT("Language"), TEXT("Core"), *Path));
					}
				}
			}

			if(!bHideEditBox && (Property->IsA(UFloatProperty::StaticClass())
					|| Property->IsA(UIntProperty::StaticClass())
					|| Property->IsA(UStrProperty::StaticClass())
					|| Property->IsA(UNameProperty::StaticClass())
					|| Property->IsA(UAnyProperty::StaticClass())
					|| Property->IsA(UObjectProperty::StaticClass())
					|| (Property->IsA(UByteProperty::StaticClass()) && reinterpret_cast<UByteProperty*>(Property)->Enum == NULL)))
			{
				// Edit control.
				FRect Rect = GetRect();
				Rect.Min.X = 1 + OwnerProperties->GetDividerWidth();
				Rect.Min.Y--;
				if (Property->IsA(UByteProperty::StaticClass()))
					Rect.Max.X = Rect.Min.X + 28;
				else
					Rect.Max.X -= ButtonWidth;
				EditControl = new WEdit(&OwnerProperties->List);
				EditControl->Snoop = this;
				EditControl->OpenWindow(0, 0, 0);
				EditControl->MoveWindow(Rect + FRect(0, 1, 0, 1), 0);
			}

			ReceiveFromControl();

#if 0 // Marco: Is this even needed anymore? Seams to cause a lot of issues with scrolling through properties with arrow keys.
			// FIXME: Add proper handling for UArrayProperty::StaticClass() EditControl. Moved here to avoid crashes with ReceiveFromControl... HACK!
			if (Property->IsA(UArrayProperty::StaticClass()) || Property->IsA(UMapProperty::StaticClass()))
			{
				// Edit control.
				EditControl = new WEdit(&OwnerProperties->List);
				EditControl->Snoop = this;
			}
#endif

			Redraw();
			if (EditControl)
				EditControl->Show(1);
			if (TrackControl)
				TrackControl->Show(1);
			if (ComboControl)
				ComboControl->Show(1);
			if (HolderControl)
				HolderControl->Show(1);
			SetFocusToItem();
		}
		else if (ArrayIndex==INDEX_NONE && !Property->IsA(UArrayProperty::StaticClass())) // Create a selectable textbox.
		{
			// Edit control.
			FRect Rect = GetRect();
			Rect.Min.X = 1 + OwnerProperties->GetDividerWidth();
			Rect.Min.Y--;
			Rect.Max.X -= ButtonWidth;
			EditControl = new WEdit(&OwnerProperties->List);
			EditControl->Snoop = this;
			EditControl->OpenWindow(0, 0, 1);
			EditControl->MoveWindow(Rect + FRect(0, 1, 0, 1), 0);

			ReceiveFromControl();

			Redraw();
			if (EditControl)
				EditControl->Show(1);
			SetFocusToItem();
		}
	}
	NotifyEditProperty(Property, GetParentProperty(), GetArrayIndex());
	unguard;
}
void FPropertyItem::OnItemKillFocus(UBOOL Abort)
{
	guard(FPropertyItem::OnItemKillFocus);
	if (!Abort)
		SendToControl();
	if (EditControl)
	{
		EditControl->Show(0);
		EditControl->DelayedDestroy();
		EditControl = NULL;
	}
	if (TrackControl)
	{
		TrackControl->Show(0);
		TrackControl->DelayedDestroy();
		TrackControl = NULL;
	}
	if (ComboControl)
	{
		ComboControl->Show(0);
		ComboControl->DelayedDestroy();
		ComboControl = NULL;
	}
	if (HolderControl)
	{
		HolderControl->Show(0);
		HolderControl->DelayedDestroy();
		HolderControl = NULL;
	}
	if (KeyEditControl)
	{
		KeyEditControl->Show(0);
		KeyEditControl->DelayedDestroy();
		KeyEditControl = NULL;
	}
	FTreeItem::OnItemKillFocus(Abort);
	unguard;
}
void FPropertyItem::Collapse()
{
	guard(WPropertyItem::Collapse);
	FTreeItem::Collapse();
	EmptyChildren();
	unguard;
}

// FControlSnoop interface.
void FPropertyItem::SnoopChar(WWindow* Src, INT Char)
{
	guard(FPropertyItem::SnoopChar);
	if (Char == 13)
	{
		if (KeyEditControl)
			SendToControl();
		else Advance();
	}
	else if (Char == 27)
		ReceiveFromControl();
	else if (ComboControl && IsFreeComboEdit)
	{
		IsFreeComboEdit = FALSE;
		FString CurrentText = ComboControl->GetText();

		ComboControl->Snoop = nullptr;
		delete ComboControl;
		ComboControl = NULL;
		if (HolderControl)
		{
			delete HolderControl;
			HolderControl = NULL;
		}

		// Edit control.
		FRect Rect = GetRect();
		Rect.Min.X = 1 + OwnerProperties->GetDividerWidth();
		Rect.Min.Y--;
		Rect.Max.X -= ButtonWidth;
		EditControl = new WEdit(&OwnerProperties->List);
		EditControl->OpenWindow(FALSE, FALSE, FALSE);
		EditControl->MoveWindow(Rect + FRect(0, 1, 0, 1), 0);

		EditControl->SetText(*CurrentText);
		EditControl->Show(1);
		SetFocus(*EditControl);
		EditControl->SetSelection(CurrentText.Len(), CurrentText.Len());
		SendMessageW(*EditControl, WM_CHAR, (WPARAM)Char, MAKELPARAM(0, 0));

		EditControl->Snoop = this;
	}
	FTreeItem::SnoopChar(Src, Char);
	unguard;
}
void FPropertyItem::ComboSelectionEndCancel()
{
	guard(FPropertyItem::ComboSelectionEndCancel);
	ReceiveFromControl();
	unguard;
}
void FPropertyItem::ComboSelectionEndOk()
{
	guard(FPropertyItem::ComboSelectionEndOk);
	ComboChanged = 1;
	SendToControl();
	ReceiveFromControl();
	Redraw();
	unguard;
}
void FPropertyItem::OnTrackBarThumbTrack()
{
	guard(FPropertyItem::OnTrackBarThumbTrack);
	if (TrackControl && EditControl)
	{
		TCHAR Tmp[256];
		appSprintf(Tmp, TEXT("%i"), TrackControl->GetPos());
		EditControl->SetText(Tmp);
		EditControl->SetModify(1);
		if (bIsLightColor)
			SetValue(Tmp);
		UpdateWindow(*EditControl);
	}
	unguard;
}
void FPropertyItem::OnTrackBarThumbPosition()
{
	guard(FPropertyItem::OnTrackBarThumbPosition);
	OnTrackBarThumbTrack();
	SendToControl();
	if (EditControl)
	{
		SetFocus(*EditControl);
		EditControl->SetSelection(0, EditControl->GetText().Len());
		Redraw();
	}
	unguard;
}
void FPropertyItem::OnChooseColorButton()
{
	guard(FPropertyItem::OnChooseColorButton);
	BYTE* ReadValue = GetReadAddress(Property, this);
	CHOOSECOLORA cc;
	static COLORREF acrCustClr[16];
	appMemzero(&cc, sizeof(cc));
	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = OwnerProperties->List;
	cc.lpCustColors = (LPDWORD)acrCustClr;
	cc.rgbResult = ReadValue ? ((*(DWORD*)ReadValue) & 0x00FFFFFF) : 0; // Remove alpha channel.
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;
	if (ChooseColorA(&cc) == TRUE)
	{
		TCHAR Str[256];
		appSprintf(Str, TEXT("(R=%i,G=%i,B=%i)"), GetRValue(cc.rgbResult), GetGValue(cc.rgbResult), GetBValue(cc.rgbResult));
		if (Parent->Children.Num() == 0)
			return;
		SetValue(Str);
		InvalidateRect(OwnerProperties->List, NULL, 0);
		UpdateWindow(OwnerProperties->List);
		Redraw();
	}
	unguard;
}
void FPropertyItem::OnArrayAdd()
{
	guard(FPropertyItem::OnArrayAdd);
	if (Property->IsA(UMapProperty::StaticClass()))
	{
		// Edit control.
		FRect Rect = GetRect();
		Rect.Min.X = 1;
		Rect.Min.Y--;
		Rect.Max.X = OwnerProperties->GetDividerWidth() - 1;
		KeyEditControl = new WEdit(&OwnerProperties->List);
		KeyEditControl->Snoop = this;
		KeyEditControl->OpenWindow(0, 0, 0);
		KeyEditControl->MoveWindow(Rect + FRect(0, 1, 0, 1), 0);
		KeyEditControl->SetText(TEXT(""));

		ReceiveFromControl();

		Redraw();
		if(KeyEditControl)
			KeyEditControl->Show(1);
		SetFocusToItem();
	}
	else
	{
		Collapse();
		for (INT j = (GetItemCount() - 1); j >= 0; --j)
		{
			FArray* Addr = GetArrayAddress(j);
			if (Addr)
			{
				UArrayProperty* Array = CastChecked<UArrayProperty>(Property);
				Addr->AddZeroed(Array->Inner->ElementSize, 1);
			}
		}
		Expand();
		NotePropertyChange(Property, this);
	}
	unguard;
}
void FPropertyItem::OnArrayEmpty()
{
	guard(FPropertyItem::OnArrayEmpty);
	Collapse();
	if (Property->IsA(UMapProperty::StaticClass()))
	{
		for (INT j = (GetItemCount() - 1); j >= 0; --j)
		{
			BYTE* AdrV = GetReadAddress(Property, this, j);
			if(AdrV)
				Property->DestroyValue(AdrV);
		}
	}
	else
	{
		UArrayProperty* Array = CastChecked<UArrayProperty>(Property);
		for (INT j = (GetItemCount() - 1); j >= 0; --j)
		{
			FArray* Addr = GetArrayAddress(j);
			if (Addr)
			{
				for (INT i = 0; i < Addr->Num(); i++)
					Array->Inner->DestroyValue((BYTE*)Addr->GetData() + (i * Array->Inner->ElementSize));
				Addr->Empty(Array->Inner->ElementSize);
			}
		}
	}
	Expand();
	if (Property->PropertyFlags & CPF_Config)
		SaveObjectConfig();
	NotePropertyChange(Property, this);
	unguard;
}
void FPropertyItem::OnArrayInsert()
{
	guard(FPropertyItem::OnArrayInsert);
	if (!Parent)
		return;
	if (bIsMapValue)
	{
		FString CurKey;
		BYTE* AdrV = Parent->GetReadAddress(Property, this);
		if (AdrV)
		{
			FScriptMapBase* SM = (FScriptMapBase*)AdrV;
			if (SM->Pairs.IsValidIndex(ArrayIndex))
			{
				UMapProperty* M = CastChecked<UMapProperty>(Property->GetOuter());
				M->Key->ExportTextItem(CurKey, SM->Pairs(ArrayIndex).Key, NULL, 0);
			}
		}

		// Edit control.
		FRect Rect = GetRect();
		Rect.Min.X = 1;
		Rect.Min.Y--;
		Rect.Max.X = OwnerProperties->GetDividerWidth() - 1;
		KeyEditControl = new WEdit(&OwnerProperties->List);
		KeyEditControl->Snoop = this;
		KeyEditControl->OpenWindow(0, 0, 0);
		KeyEditControl->MoveWindow(Rect + FRect(0, 1, 0, 1), 0);
		KeyEditControl->SetText(*CurKey);

		ReceiveFromControl();

		Redraw();
		if (KeyEditControl)
			KeyEditControl->Show(1);
		SetFocusToItem();
	}
	else
	{
		Parent->Collapse();
		UArrayProperty* Array = CastChecked<UArrayProperty>(Property->GetOuter());
		for (INT j = (GetItemCount() - 1); j >= 0; --j)
		{
			FArray* Addr = Parent->GetArrayAddress(j);
			if (Addr)
			{
				INT Index = Offset / Array->Inner->ElementSize;
				Addr->Insert(Index, 1, Array->Inner->ElementSize);
				appMemzero((BYTE*)Addr->GetData() + Index * Array->Inner->ElementSize, Array->Inner->ElementSize);
			}
		}
		Parent->Expand();
		if (Array->PropertyFlags & CPF_Config)
			SaveObjectConfig();
		NotePropertyChange(Property, this);
	}
	unguard;
}
void FPropertyItem::OnArrayDelete()
{
	guard(FPropertyItem::OnArrayDelete);
	if (!Parent)
		return;

	if (Parent->Children.Num() == 1)
	{
		Parent->OnArrayEmpty();
		return;
	}
	if (bIsMapValue)
	{
		UMapProperty* M = CastChecked<UMapProperty>(Property->GetOuter());
		for (INT j = (GetItemCount() - 1); j >= 0; --j)
		{
			BYTE* Addr = Parent->GetReadAddress(M, Parent, j);
			if (!Addr)
				continue;
			FScriptMapBase* SM = (FScriptMapBase*)Addr;
			if (SM->Pairs.IsValidIndex(ArrayIndex))
			{
				M->Key->DestroyValue(SM->Pairs(ArrayIndex).Key);
				M->Value->DestroyValue(SM->Pairs(ArrayIndex).Value);
				SM->Pairs.Remove(ArrayIndex);
				SM->Relax();
			}
		}
	}
	else
	{
		UArrayProperty* Array = CastChecked<UArrayProperty>(Property->GetOuter());
		for (INT j = (GetItemCount() - 1); j >= 0; --j)
		{
			FArray* Addr = Parent->GetArrayAddress(j);
			if (Addr)
			{
				INT Index = Offset / Array->Inner->ElementSize;
				Array->Inner->DestroyValue((BYTE*)Addr->GetData() + (Index * Array->Inner->ElementSize));
				Addr->Remove(Index, 1, Array->Inner->ElementSize);
			}
		}
	}
	Parent->Collapse();
	Parent->Expand();
	NotePropertyChange(Property, this);
	if (CastChecked<UProperty>(Property->GetOuter())->PropertyFlags & CPF_Config)
		SaveObjectConfig();
	unguard;
}
void FPropertyItem::OnBrowseButton()
{
	guard(FPropertyItem::OnBrowseButton);
	UObjectProperty* ReferenceProperty = CastChecked<UObjectProperty>(Property);
	TCHAR Temp[256];
	appSprintf(Temp, TEXT("BROWSECLASS CLASS=%ls"), ReferenceProperty->PropertyClass->GetName());
	if (OwnerProperties->NotifyHook)
		OwnerProperties->NotifyHook->NotifyExec(OwnerProperties, Temp);
	Redraw();
	unguard;
}
void FPropertyItem::OnUseCurrentButton()
{
	guard(FPropertyItem::OnUseCurrentButton);
	UObjectProperty* ReferenceProperty = CastChecked<UObjectProperty>(Property);
	TCHAR Temp[256];
	appSprintf(Temp, TEXT("USECURRENT CLASS=%ls"), ReferenceProperty->PropertyClass->GetName());
	if (OwnerProperties->NotifyHook)
		OwnerProperties->NotifyHook->NotifyExec(OwnerProperties, Temp);
	Redraw();
	unguard;
}
void FPropertyItem::OnClearButton()
{
	guard(FPropertyItem::OnClearButton);
	SetValue(TEXT("None"));
	unguard;
}
void FPropertyItem::Advance()
{
	guard(FPropertyItem::Advance);
	if (ComboControl && ComboControl->GetCurrent() >= 0)
	{
		ComboControl->SetCurrent((ComboControl->GetCurrent() + 1) % ComboControl->GetCount());
		ComboChanged = 1;
	}
	SendToControl();
	ReceiveFromControl();
	Redraw();
	unguard;
}
void FPropertyItem::SendToControl()
{
	guard(FPropertyItem::SendToControl);

	if (EditControl)
	{
		if (EditControl->GetModify())
			SetValue(*EditControl->GetText());
	}
	else if (ComboControl)
	{
		if (ComboChanged)
			SetValue(*ComboControl->GetString(ComboControl->GetCurrent()));
		ComboChanged = 0;
	}
	if (KeyEditControl)
	{
		if (bIsMapValue)
		{
			UMapProperty* M = CastChecked<UMapProperty>(Property->GetOuter());
			UProperty* K = M->Key;
			FString Str = KeyEditControl->GetText();
			BYTE* Data = new BYTE[K->ElementSize];
			appMemzero(Data, K->ElementSize);
			K->ImportText(*Str, Data, 0);
			DWORD iHash = K->GetMapHash(Data);

			for (INT j = (GetItemCount() - 1); j >= 0; --j)
			{
				BYTE* Addr = Parent->GetReadAddress(M, Parent, j);
				if (!Addr)
					continue;
				FScriptMapBase* SM = (FScriptMapBase*)Addr;
				
				FScriptMapBase::USPair* P = &SM->Pairs(0) - 1;
				INT i;
				for (i = SM->FindHash(iHash); i; i = P[i].HashNext)
					if (K->Identical(Data, P[i].Key))
						break;

				if (i)
				{
					if ((i - 1) != ArrayIndex)
					{
						// Overwrite excisting value.
						Exchange(P[i].Value, SM->Pairs(ArrayIndex).Value);
						M->Key->DestroyValue(SM->Pairs(ArrayIndex).Key);
						M->Value->DestroyValue(SM->Pairs(ArrayIndex).Value);
						SM->Pairs.Remove(ArrayIndex);
						SM->Relax();
					}
				}
				else
				{
					// Create new value.
					K->CopySingleValue(SM->Pairs(ArrayIndex).Key, Data);
					SM->Pairs(ArrayIndex).HashValue = iHash;
					SM->Rehash();
				}
			}

			K->DestroyValue(Data);
			delete[] Data;

			if (M->PropertyFlags & CPF_Config)
				SaveObjectConfig();
		}
		else
		{
			UMapProperty* M = CastChecked<UMapProperty>(Property);
			UProperty* K = M->Key;
			FString Str = KeyEditControl->GetText();
			BYTE* Data = new BYTE[K->ElementSize];
			appMemzero(Data, K->ElementSize);
			K->ImportText(*Str, Data, 0);
			DWORD iHash = K->GetMapHash(Data);

			for (INT j = (GetItemCount() - 1); j >= 0; --j)
			{
				BYTE* Addr = GetReadAddress(M, this, j);
				if (!Addr)
					continue;
				FScriptMapBase* SM = (FScriptMapBase*)Addr;

				FScriptMapBase::USPair* P = &SM->Pairs(0) - 1;
				INT i;
				for (i = SM->FindHash(iHash); i; i = P[i].HashNext)
					if (K->Identical(Data, P[i].Key))
						break;

				if (i)
				{
					// Value already found.
				}
				else
				{
					// Create new value.
					BYTE* NewKey = (BYTE*)appMalloc(K->ElementSize, TEXT("SKey"));
					appMemzero(NewKey, K->ElementSize);
					K->CopySingleValue(NewKey, Data);

					BYTE* NewValue = (BYTE*)appMalloc(M->Value->ElementSize, TEXT("SValue"));
					appMemzero(NewValue, M->Value->ElementSize);
					SM->Add(NewKey, NewValue, iHash);
				}
			}

			K->DestroyValue(Data);
			delete[] Data;

			if (M->PropertyFlags & CPF_Config)
				SaveObjectConfig();
		}
		delete KeyEditControl;
		KeyEditControl = NULL;

		if (bIsMapValue)
		{
			Parent->Collapse();
			Parent->Expand();
		}
		else
		{
			Collapse();
			Expand();
		}
		NotePropertyChange(Property, this);
	}
	unguard;
}
void FPropertyItem::ReceiveFromControl()
{
	guard(FPropertyItem::ReceiveFromControl);
	ComboChanged = 0;
	BYTE* ReadValue = GetReadAddress(Property, this);
	if (EditControl)
	{
		FString Str;
		if (ReadValue)
			GetPropertyText(Str, ReadValue);
		EditControl->SetText(*Str);
		if(EditControl->hWnd)
			EditControl->SetSelection(0, Str.Len());
	}
	if (TrackControl)
	{
		if (ReadValue)
			TrackControl->SetPos(*(BYTE*)ReadValue);
	}
	if (ComboControl)
	{
		UBoolProperty* BoolProperty;
		if ((BoolProperty = Cast<UBoolProperty>(Property)) != NULL)
		{
			ComboControl->SetCurrent(ReadValue ? (*(BITFIELD*)ReadValue & BoolProperty->BitMask) != 0 : -1);
		}
		else if (Property->IsA(UByteProperty::StaticClass()))
		{
			ComboControl->SetCurrent(ReadValue ? *(BYTE*)ReadValue : -1);
		}
		else if (Property->IsA(UNameProperty::StaticClass()))
		{
			INT Index = ReadValue ? ComboControl->FindString(**(FName*)ReadValue) : 0;
			if (IsFreeComboEdit && Index == INDEX_NONE)
			{
				ComboControl->AddString(**(FName*)ReadValue);
				Index = ComboControl->GetCount() - 1;
			}
			ComboControl->SetCurrent(Index >= 0 ? Index : 0);
		}
		ComboChanged = 0;
	}
	unguard;
}
FString FPropertyItem::GetDescription()
{
	return Property->Description;
}

/*-----------------------------------------------------------------------------
	FObjectsItem.
-----------------------------------------------------------------------------*/

void FObjectsItem::Serialize(FArchive& Ar)
{
	guard(FPropertyItemBase::Serialize);
	FPropertyItemBase::Serialize(Ar);
	Ar << _Objects;
	unguard;
}
BYTE* FObjectsItem::GetReadAddress(UProperty* InProperty, FTreeItem* Top, INT Index)
{
	guard(FObjectsItem::GetReadAddress);
	ValidateSelection();
	if (!_Objects.Num())
		return NULL;
	else if (_Objects.Num() == 1)
		return (BYTE*)_Objects(0);
	else if(Index>=0)
		return (BYTE*)_Objects(Index);

	BYTE* Base = (BYTE*)_Objects(0);
	UArrayProperty* Array = Cast<UArrayProperty>(InProperty);
	if (Array)
	{
		INT Num0 = Top->GetArrayAddress(0)->Num();
		for (INT i = 1; i < _Objects.Num(); i++)
			if (Num0 != Top->GetArrayAddress(i)->Num())
				return NULL;
	}
	else
	{
		BYTE* Base0 = Top->GetReadAddress(InProperty, Top, 0);
		for (INT i = 1; i < _Objects.Num(); i++)
			if (!InProperty->Identical(Base0, Top->GetReadAddress(InProperty, Top, i)))
				return NULL;
	}
	return Base;
	unguardf((TEXT("(%ls - %i)"), InProperty->GetPathName(), Index));
}
UProperty* FPropertyItemBase::FindProperty(const TCHAR* PropName) const
{
	guard(FPropertyItemBase::FindProperty);
	return BaseClass ? FindField<UProperty>(BaseClass, PropName) : nullptr;
	unguard;
}

static UBOOL bInnerCall = 0;
void FObjectsItem::SetProperty(UProperty* Property, FTreeItem* Offset, const TCHAR* Value, INT ExtraFlags)
{
	guard(FObjectsItem::SetProperty);
	if (bInnerCall)
		return;

	bInnerCall = 1;
	ValidateSelection();
	if (OwnerProperties->NotifyHook)
		OwnerProperties->NotifyHook->NotifyPreChange(OwnerProperties);
	UProperty* ParentProp = Offset->GetParentProperty();

	for (INT i = 0; i < _Objects.Num(); i++)
	{
		UProperty::_ImportObject = _Objects(i)->GetClass();
		Property->ImportText(Value, Offset->GetReadAddress(Property, Offset, i), (PPF_Localized | PPF_PropWindow) | ExtraFlags);
		UProperty::_ImportObject = NULL;
		_Objects(i)->onPropertyChange(Property, ParentProp);
	}
	if (OwnerProperties->NotifyHook)
		OwnerProperties->NotifyHook->NotifyPostChange(OwnerProperties);
	bInnerCall = 0;
	unguard;
}
void FObjectsItem::NotePropertyChange(UProperty* Property, FTreeItem* Offset)
{
	guard(FObjectsItem::NotePropertyChange);
	if (bInnerCall)
		return;

	bInnerCall = 1;
	ValidateSelection();
	if (OwnerProperties->NotifyHook)
		OwnerProperties->NotifyHook->NotifyPreChange(OwnerProperties);
	UProperty* ParentProp = Offset->GetParentProperty();

	for (INT i = 0; i < _Objects.Num(); i++)
		_Objects(i)->onPropertyChange(Property, ParentProp);
	if (OwnerProperties->NotifyHook)
		OwnerProperties->NotifyHook->NotifyPostChange(OwnerProperties);
	bInnerCall = 0;
	unguard;
}

void FClassItem::SetProperty(UProperty* Property, FTreeItem* Offset, const TCHAR* Value, INT ExtraFlags)
{
	guard(FClassItem::SetProperty);
	if (bInnerCall)
		return;

	bInnerCall = 1;
	UProperty* ParentProp = Offset->GetParentProperty();
	UProperty::_ImportObject = BaseClass;
	Property->ImportText(Value, Offset->GetReadAddress(Property, this), (PPF_Localized | PPF_PropWindow) | ExtraFlags);
	UProperty::_ImportObject = NULL;
	BaseClass->SetFlags(RF_SourceModified);
	UObject::eventStaticPropertyChange(BaseClass, Property->GetFName(), ParentProp ? ParentProp->GetFName() : Property->GetFName());
	BaseClass->MarkAsDirty();
	bInnerCall = 0;
	unguard;
}
void FClassItem::NotePropertyChange(UProperty* Property, FTreeItem* Offset)
{
	guard(FClassItem::NotePropertyChange);
	if (bInnerCall)
		return;

	bInnerCall = 1;
	UProperty* ParentProp = Offset->GetParentProperty();
	UObject::eventStaticPropertyChange(BaseClass, Property->GetFName(), ParentProp ? ParentProp->GetFName() : Property->GetFName());
	BaseClass->MarkAsDirty();
	bInnerCall = 0;
	unguard;
}

UProperty* FObjectConfigItem::FindProperty(const TCHAR* PropName) const
{
	guard(FObjectConfigItem::FindProperty);
	return Class ? FindField<UProperty>(Class, PropName) : nullptr;
	unguard;
}

void FObjectConfigItem::SetProperty(UProperty* Property, FTreeItem* Offset, const TCHAR* Value, INT ExtraFlags)
{
	guard(FObjectConfigItem::SetProperty);
	if (bInnerCall)
		return;

	bInnerCall = 1;
	check(Class);
	if (OwnerProperties->NotifyHook)
		OwnerProperties->NotifyHook->NotifyPreChange(OwnerProperties);
	if (Cast<UClassProperty>(Property) && appStricmp(*Property->Category, TEXT("Drivers")) == 0)
	{
		// Save it.
		UClassProperty* ClassProp = CastChecked<UClassProperty>(Property);
		TArray<FRegistryObjectInfo> Classes;
		UObject::GetRegistryObjects(Classes, UClass::StaticClass(), ClassProp->MetaClass, 0);
		for (INT i = 0; i < Classes.Num(); i++)
		{
			FString Path, Left, Right, Text;
			Path = *Classes(i).Object;
			if (Path.Divide(TEXT("."), Left, Right))
				Text = *Localize(*Right, TEXT("ClassCaption"), *Left);
			else
				Text = *Localize(TEXT("Language"), TEXT("Language"), TEXT("Core"), *Path);

			if (appStricmp(*Text, Value) == 0)
				GConfig->SetString(Property->GetOwnerClass()->GetPathName(), Property->GetName(), *Classes(i).Object);
		}
	}
	else
	{
		Property->ImportText(Value, Offset->GetReadAddress(Property, this), (PPF_Localized | PPF_PropWindow) | ExtraFlags);
		if (Immediate)
		{
			guard(Immediate);
			UProperty* ParentProp = Offset->GetParentProperty();

			for (FObjectIterator It; It; ++It)
			{
				if (It->IsA(Class))
				{
					ParentProp->CopyCompleteValue(((BYTE*)*It) + ParentProp->Offset, GetReadAddress(ParentProp, this) + ParentProp->Offset);
					It->PostEditChange();
				}
			}
			unguard;
		}
		SaveObjectConfig();
	}
	//else UObject::GlobalSetProperty(Value,Class,Property,GetPropertyOffset(Property),Immediate);

	if (OwnerProperties->NotifyHook)
		OwnerProperties->NotifyHook->NotifyPostChange(OwnerProperties);

	UProperty* ParentProp = Offset->GetParentProperty();
	UObject::eventStaticPropertyChange(Class, Property->GetFName(), ParentProp ? ParentProp->GetFName() : Property->GetFName());
	bInnerCall = 0;
	unguard;
}
void FObjectConfigItem::NotePropertyChange(UProperty* Property, FTreeItem* Offset)
{
	guard(FObjectConfigItem::NotePropertyChange);
	if (bInnerCall)
		return;

	bInnerCall = 1;
	UProperty* ParentProp = Offset->GetParentProperty();
	UObject::eventStaticPropertyChange(Class, Property->GetFName(), ParentProp ? ParentProp->GetFName() : Property->GetFName());
	bInnerCall = 0;
	unguard;
}

void FObjectsItem::SetObjects(UObject** InObjects, INT Count)
{
	guard(FObjectsItem::SetObjects);

	// Disable editing, to prevent crash due to edit-in-progress after empty objects list.
	OwnerProperties->SetItemFocus(0);

	// Add objects and find lowest common base class.
	_Objects.Empty();
	BaseClass = NULL;
	for (INT i = 0; i < Count; i++)
	{
		if (InObjects[i])
		{
			check(InObjects[i]->GetClass());
			if (!BaseClass)
				BaseClass = InObjects[i]->GetClass();
			while (!InObjects[i]->GetClass()->IsChildOf(BaseClass))
				BaseClass = BaseClass->GetSuperClass();
			_Objects.AddItem(InObjects[i]);
		}
	}

	// Automatically title the window.
	OwnerProperties->SetText(*GetCaption());

	// Refresh all properties.
	if (Expanded || this == OwnerProperties->GetRoot())
		OwnerProperties->ForceRefresh();

	unguard;
}
void FObjectsItem::Expand()
{
	guard(FObjectsItem::Expand);
	ValidateSelection();
	if (ByCategory)
	{
		// Expand to show categories.
		UBOOL bHasHidden = 0;
		TArray<FName> Categories;
		for (TFieldIterator<UProperty> It(BaseClass); It; ++It)
		{
			if (AcceptFlags(It->PropertyFlags) && It->Category!=NAME_None)
				Categories.AddUniqueItem(It->Category);
			else bHasHidden = 1;
		}
		for (INT i = 0; i < Categories.Num(); i++)
			Children.AddItem(new FCategoryItem(OwnerProperties, this, BaseClass, Categories(i), 1));
		if(bHasHidden)
			Children.AddItem(new FHiddenCategoryList(OwnerProperties, this, BaseClass));
	}
	else
	{
		// Expand to show individual items.
		for (TFieldIterator<UProperty> It(BaseClass); It; ++It)
			if (AcceptFlags(It->PropertyFlags) && It->GetOwnerClass() != UObject::StaticClass())//hack for ufactory display!!
				Children.AddItem(new FPropertyItem(OwnerProperties, this, *It, It->GetFName(), It->Offset, -1));
	}
	FTreeItem::Expand();
	unguard;
}

void FObjectsItem::ValidateSelection()
{
	guard(FObjectsItem::ValidateSelection);
	bool bChanged = false;
	INT i = 0;
	for (i = 0; i < _Objects.Num(); i++)
	{
		if (!_Objects(i) || _Objects(i)->IsPendingKill())
		{
			bChanged = true;
			_Objects.Remove(i, 1);
			i--;
		}
	}
	if (bChanged)
	{
		// Disable editing, to prevent crash due to edit-in-progress after empty objects list.
		OwnerProperties->SetItemFocus(0);

		// Find lowest common base class.
		BaseClass = NULL;
		for (i = 0; i < _Objects.Num(); i++)
		{
			if (!BaseClass)
				BaseClass = _Objects(i)->GetClass();
			while (!_Objects(i)->GetClass()->IsChildOf(BaseClass))
				BaseClass = BaseClass->GetSuperClass();
		}

		// Automatically title the window.
		OwnerProperties->SetText(*GetCaption());

		// Refresh all properties.
		if (Expanded || this == OwnerProperties->GetRoot())
			OwnerProperties->ForceRefresh();
	}
	unguard;
}

static void FillProperties(FString* Out, BYTE* Values, BYTE* Delta, UClass* Class, INT DeltaSize=INDEX_NONE)
{
	guard(FillProperties);
	FStringOutputDevice A;
	UProperty* P;
	UBOOL bSize;
	for (TFieldIterator<UProperty> It(Class); It; ++It)
	{
		P = *It;
		if (P->GetOuter() == UObject::StaticClass())
			break;
		if (P->GetSize() && !(P->PropertyFlags & (CPF_EditConst | CPF_Transient | CPF_Native)))
		{
			bSize = (DeltaSize != INDEX_NONE && P->Offset >= DeltaSize);
			if (P->ArrayDim > 1)
			{
				for (INT i = 0; i < P->ArrayDim; ++i)
				{
					FString Value;
					if (P->ExportText(i, Value, Values, bSize ? NULL : Delta, PPF_Delimited))
						A.Logf(TEXT("\t%ls[%i]=%ls\r\n"), P->GetName(), i, *Value);
				}
			}
			else
			{
				FString Value;
				if (P->ExportText(0, Value, Values, bSize ? NULL : Delta, PPF_Delimited))
					A.Logf(TEXT("\t%ls=%ls\r\n"), P->GetName(), *Value);
			}
		}
	}
	*Out = A;
	unguardf((TEXT("(%ls)"),Class->GetFullName()));
}
UBOOL FObjectsItem::CopyFullProperties(FString* Output)
{
	guard(FObjectsItem::CopyFullProperties);
	ValidateSelection();
	if (_Objects.Num() != 1)
		return FALSE;

	if(Output)
		FillProperties(Output, (BYTE*)_Objects(0), &_Objects(0)->GetClass()->Defaults(0), _Objects(0)->GetClass());
	return TRUE;
	unguard;
}
UBOOL FEditObjectItem::CopyFullProperties(FString* Output)
{
	guard(FEditObjectItem::CopyFullProperties);
	if (!*EditClassPtr)
		return FHeaderItem::CopyFullProperties(Output);

	if (Output)
		FillProperties(Output, (BYTE*)*EditClassPtr, &(*EditClassPtr)->GetClass()->Defaults(0), (*EditClassPtr)->GetClass());
	return TRUE;
	unguard;
}
UBOOL FClassItem::CopyFullProperties(FString* Output)
{
	guard(FClassItem::CopyFullProperties);
	if (Output)
		FillProperties(Output, &BaseClass->Defaults(0), &BaseClass->GetSuperClass()->Defaults(0), BaseClass, BaseClass->GetSuperClass()->Defaults.Num());
	return TRUE;
	unguard;
}

/*-----------------------------------------------------------------------------
	WTerminal
-----------------------------------------------------------------------------*/

FThreadLock WTerminal::TerminalMutex;

void WTerminal::Serialize(const TCHAR* Data, EName MsgType)
{
	guard(WTerminal::Serialize);
	FScopeThread<FThreadLock> Lock(TerminalMutex);
	if (MsgType == NAME_Title)
	{
		SetText(Data);
		return;
	}
	else if (Shown)
	{
		Display.SetRedraw(0);
		INT LineCount = Display.GetLineCount();
		if (LineCount > MaxLines)
		{
			INT NewLineCount = Max(LineCount - SlackLines, 0);
			INT Index = Display.GetLineIndex(LineCount - NewLineCount);
			Display.SetSelection(0, Index);
			Display.SetSelectedText(TEXT(""));
			INT Length = Display.GetLength();
			Display.SetSelection(Length, Length);
			Display.ScrollCaret();
		}
		TCHAR Temp[1024] = TEXT("");
		appStrncat(Temp, *FName(MsgType), ARRAY_COUNT(Temp));
		appStrncat(Temp, TEXT(": "), ARRAY_COUNT(Temp));
		appStrncat(Temp, (TCHAR*)Data, ARRAY_COUNT(Temp));
		appStrncat(Temp, TEXT("\r\n"), ARRAY_COUNT(Temp));
		Temp[ARRAY_COUNT(Temp) - 1] = 0;
		INT Length = Display.GetLength();
		Display.SetSelection(Length, Length);
		Display.SetRedraw(1);
		Display.SetSelectedText(Temp);
	}
	unguard;
}

// WWindow interface.
void WTerminal::OpenWindow(UBOOL bMdi, UBOOL AppWindow)
{
	guard(WTerminal::OpenWindow);
	MdiChild = bMdi;
	PerformCreateWindowEx
	(
		MdiChild
		? (WS_EX_MDICHILD)
		: (AppWindow ? WS_EX_APPWINDOW : 0),
		*FString::Printf(*LocalizeGeneral(TEXT("LogWindow"), TEXT("Window")), *LocalizeGeneral(TEXT("Product"), TEXT("Core"))),
		MdiChild
		? (WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
		: (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_SIZEBOX),
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		MulDiv(512, DPIX, 96),
		MulDiv(256, DPIY, 96),
		OwnerWindow ? OwnerWindow->hWnd : NULL,
		NULL,
		hInstance
	);
	unguard;
}

// WTerminal interface.
void WTerminal::OnCreate()
{
	guard(WTerminal::OnCreate);
	WWindow::OnCreate();
	Display.OpenWindow(1, 1, 1);

	Display.SetFont((HFONT)GetStockObject(DEFAULT_GUI_FONT));
	Display.SetText(TEXT(""));

	EditBox.OpenWindow(1, 0, CBS_DROPDOWN | CBS_AUTOHSCROLL, 0, 0, 64, 16);
	unguard;
}
void WTerminal::OnSize(DWORD Flags, INT NewX, INT NewY)
{
	guard(WTerminal::OnSize);
	WWindow::OnSize(Flags, NewX, NewY);
	const INT EditHeight = 24;
	EditBox.MoveWindow(FRect(0, NewY - EditHeight, NewX, NewY), TRUE);
	Display.MoveWindow(FRect(0, 0, NewX, NewY - EditHeight), TRUE);
	Display.ScrollCaret();
	unguard;
}

void WTerminal::HitEnter()
{
	guard(WTerminal::HitEnter);
	FString Str = EditBox.GetText();
	if (Str.Len())
	{
		INT count = EditBox.GetCount();
		if (count >= 8) // History size overflowed!
			EditBox.RemoveString(*EditBox.GetString(0));

		if (EditBox.FindString(*Str) == INDEX_NONE)
			EditBox.AddString(*Str);

		EditBox.SetText(TEXT(""));
		if (Exec)
		{
			if (!GIsEditor)
				GLog->Log(NAME_Cmd, *Str);
			if (!Exec->Exec(*Str, *GLog))
				Log(LocalizeError(TEXT("Exec"), TEXT("Core")));
		}
	}
	unguard;
}

/*-----------------------------------------------------------------------------
	WLog
-----------------------------------------------------------------------------*/

void WLog::OnCreate()
{
	guard(WLog::OnCreate);
	WWindow::OnCreate();
	Display.OpenWindow(1, 1, 1);
	LOGFONT lf{};
	lf.lfHeight = -MulDiv(12, DPIY, 72);
	lf.lfCharSet = ANSI_CHARSET;
	lf.lfPitchAndFamily = FIXED_PITCH;
	HFONT hfont = CreateFontIndirect(&lf);
	Display.SetFont( /*(HFONT)GetStockObject(ANSI_FIXED_FONT) */ hfont);
	Display.SetText(TEXT(""));
	EditBox.OpenWindow(1, 0, CBS_DROPDOWN | CBS_AUTOHSCROLL);
	EditBox.SetFont( /*(HFONT)GetStockObject(ANSI_FIXED_FONT) */ hfont);
	unguard;
}
void WLog::SetText(const TCHAR* Text)
{
	guard(WLog::SetText);
	WWindow::SetText(Text);
	if (GNotify)
	{
#if UNICODE
		if (GUnicode && !GUnicodeOS)
		{
			appMemcpy(NIDA.szTip, TCHAR_TO_ANSI(Text), Min<INT>(ARRAY_COUNT(NIDA.szTip), appStrlen(Text) + 1));
			NIDA.szTip[ARRAY_COUNT(NIDA.szTip) - 1] = 0;
			Shell_NotifyIconA(NIM_MODIFY, &NIDA);
		}
		else
#endif
		{
			appStrncpy(NID.szTip, Text, ARRAY_COUNT(NID.szTip));
#if UNICODE
			Shell_NotifyIconW(NIM_MODIFY, &NID);
#else
			Shell_NotifyIconA(NIM_MODIFY, &NID);
#endif
		}
	}
	unguard;
}
void WLog::OnShowWindow(UBOOL bShow)
{
	guard(WLog::OnShowWindow);
	WTerminal::OnShowWindow(bShow);
	if (bShow)
	{
		// Load log file.
		if (LogAr)
		{
			delete LogAr;
			{
				FString LogContents;
				appLoadFileToString(LogContents, *LogFilename);
				INT CrCount = 0;
				INT Ofs;
				for (Ofs = LogContents.Len() - 1; Ofs > 0 && CrCount < MaxLines; Ofs--)
					CrCount += (LogContents[Ofs] == '\n');
				while (Ofs < LogContents.Len() && LogContents[Ofs] == '\n')
					Ofs++;
				if (LogContents.Len())
					LogContents += TEXT("\n");
				Display.SetText(&LogContents[Ofs]);
			}
			LogAr = GFileManager->CreateFileWriter(*LogFilename, FILEWRITE_Unbuffered | FILEWRITE_Append);
		}
		INT Length = Display.GetLength();
		Display.SetSelection(Length, Length);
		Display.ScrollCaret();
	}
	unguard;
}

static void GNotifyExit()
{
	if (GNotify)
		Shell_NotifyIconW(NIM_DELETE, &NID);
}
void WLog::OpenWindow(UBOOL bShow, UBOOL bMdi)
{
	guard(WLog::OpenWindow);

	WTerminal::OpenWindow(bMdi, 0);
	Show(bShow);
	UpdateWindow(*this);
	GLogHook = this;

	// Show dedicated server in tray.
	if (!GIsClient && !GIsEditor)
	{
		HICON ServerIcon = NULL;
		{
			FString IcName;
			if (Parse(appCmdLine(), TEXT("ICON="), IcName))
			{
				ServerIcon = (HICON)LoadImage(NULL,*IcName,IMAGE_ICON,0,0,LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_SHARED);
				if (!ServerIcon)
					GWarn->Logf(TEXT("Couldn't find or load server icon '%ls'"), *IcName);
			}
		}
		NID.cbSize = sizeof(NID);
		NID.hWnd = hWnd;
		NID.uID = 0;
		NID.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP;
		NID.uCallbackMessage = NidMessage;
		NID.hIcon = ServerIcon ? ServerIcon : LoadIconW(hInstanceWindow, MAKEINTRESOURCEW(IDICON_Mainframe));
		NID.szTip[0] = 0;
		Shell_NotifyIconW(NIM_ADD, &NID);
		GNotify = 1;
		atexit(GNotifyExit);
	}

	unguard;
}
void WLog::OnDestroy()
{
	guard(WLog::OnDestroy);

	GLogHook = NULL;
	WTerminal::OnDestroy();

	unguard;
}
void WLog::OnCopyData(HWND hWndSender, COPYDATASTRUCT* CD)
{
	guard(OnCopyData);
	if (Exec)
	{
		debugf(TEXT("WM_COPYDATA: %ls"), (TCHAR*)CD->lpData);
		Exec->Exec(TEXT("TakeFocus"), *GLogWindow);
		TCHAR NewURL[1024];
		if (ParseToken(*(const TCHAR**)&CD->lpData, NewURL, ARRAY_COUNT(NewURL), 0)
			&& NewURL[0] != '-')
		{
			FString OpenURL = FString::Printf(TEXT("Open %ls"), NewURL);
			Exec->Exec(*OpenURL, *GLogWindow);
		}
	}
	unguard;
}
void WLog::OnClose()
{
	guard(WLog::OnClose);
	Show(0);
	throw TEXT("NoRoute");
	unguard;
}
void WLog::OnCommand(INT Command)
{
	guard(WLog::OnCommand);
	if (Command == ID_LogFileExit || Command == ID_NotifyExit)
	{
		// Exit.
		FString Msg = Command == ID_LogFileExit ? TEXT("ID_LogFileExit") : TEXT("ID_NotifyExit");
		appRequestExit(0, Msg);
	}
	else if (Command == ID_LogAdvancedOptions || Command == ID_NotifyAdvancedOptions)
	{
		// Advanced options.
		if (Exec)
			Exec->Exec(TEXT("PREFERENCES"), *GLogWindow);
	}
	else if (Command == ID_NotifyShowLog)
	{
		// Show log window.
		ShowWindow(hWnd, SW_SHOWNORMAL);
		SetForegroundWindow(hWnd);
	}
	unguard;
}
LRESULT WLog::WndProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	guard(WLog::WndProc);
	if (Message == NidMessage)
	{
		if (lParam == WM_RBUTTONDOWN || lParam == WM_LBUTTONDOWN)
		{
			// Options.
			POINT P;
			::GetCursorPos(&P);
			HMENU hMenu = LoadLocalizedMenu(hInstanceWindow, IDMENU_NotifyIcon, TEXT("IDMENU_NotifyIcon"), TEXT("Window"));
			SetForegroundWindow(hWnd);
			TrackPopupMenu(GetSubMenu(hMenu, 0), lParam == WM_LBUTTONDOWN ? TPM_LEFTBUTTON : TPM_RIGHTBUTTON, P.x, P.y, 0, hWnd, NULL);
			PostMessageW(hWnd, WM_NULL, 0, 0);
		}
		return 1;
	}
	else return WWindow::WndProc(Message, wParam, lParam);
	unguardf((TEXT("(Message=%ls,PersistentName=%ls)"), *PersistentName, GetWindowMessageString(Message)));
}

/*-----------------------------------------------------------------------------
	FCustomCursor.
-----------------------------------------------------------------------------*/

FCustomCursor::~FCustomCursor() noexcept(false)
{
	guard(FCustomCursor::~FCustomCursor);
	if (Cursor)
	{
		DestroyCursor(Cursor);
		Cursor = NULL;
	}
	unguard;
}

UBOOL FCustomCursor::CreateCursorRaw(BYTE* Data, INT DataSize)
{
	if (Cursor)
	{
		DestroyCursor(Cursor);
		Cursor = NULL;
	}
	Cursor = CreateIconFromResourceEx((PBYTE)Data, DataSize, FALSE, 0x00030000, 0, 0, LR_DEFAULTSIZE);
	if (!Cursor)
		GWarn->Logf(TEXT("Create cursor failed, error code %i"), GetLastError());
	return (Cursor != NULL);
}

#define MAKEFOURCC(ch0, ch1, ch2, ch3)\
    ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |\
    ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))

#define mmioFOURCC(ch0, ch1, ch2, ch3)\
    MAKEFOURCC(ch0, ch1, ch2, ch3)

struct FByteWriter : public FArchive
{
	TArray<BYTE> Buffer;

	FByteWriter()
	{
		ArIsSaving = 1;
	}
	void Serialize(void* V, INT Length)
	{
		INT i = Buffer.Add(Length);
		appMemcpy(&Buffer(i), V, Length);
	}
};
struct FRiffHeader
{
	DWORD ID;
	DWORD Size;
	DWORD Signature;
};
struct FSectionID
{
	DWORD ID;
	DWORD Size;
};
struct animHeader
{
	DWORD HeaderSize;
	DWORD NumFrames;
	DWORD NumSteps;
	DWORD Width;
	DWORD Height;
	DWORD BitCount;
	DWORD NumPlanes;
	DWORD DisplayRate;
	DWORD Flags;
};
struct iconHeader
{
	WORD Reserved;
	WORD fileType;
	WORD imgCount;
	BYTE width;
	BYTE height;
	BYTE numColors;
	BYTE imgReserved;
	WORD HotSpotX;
	WORD HotSpotY;
};
struct Win32BitmapInfoHeader
{
	DWORD size;
	DWORD width;
	DWORD height;
	WORD planes;
	WORD bit_count;
	DWORD compression;
	DWORD x_pels_per_meter;
	DWORD y_pels_per_meter;
	DWORD clr_important;
	DWORD clr_used;
	DWORD size_image;
};
UBOOL FCustomCursor::CreateCursorTex(FAnimCursorData* Data, INT NumFrames, INT AnimRate)
{
	guard(FCustomCursor::CreateCursorTex);
	FByteWriter Out;
	INT secList = 0;
	{
		FRiffHeader Hdr = { mmioFOURCC('R', 'I', 'F', 'F'), 0, mmioFOURCC('A', 'C', 'O', 'N') };
		Out.Serialize(&Hdr, sizeof(FRiffHeader));
	}
	/*{
		FRiffHeader Hdr = { mmioFOURCC('L', 'I', 'S', 'T'), sizeof(DWORD), mmioFOURCC('I', 'N', 'F', 'O') };
		Out.Serialize(&Hdr, sizeof(FRiffHeader));
	}*/
	{
		FSectionID Hdr = { mmioFOURCC('a', 'n', 'i', 'h'), sizeof(animHeader) };
		Out.Serialize(&Hdr, sizeof(FSectionID));
	}
	{
		animHeader A;
		A.HeaderSize = sizeof(animHeader);
		A.NumFrames = NumFrames;
		A.NumSteps = NumFrames;
		A.Width = 0;
		A.Height = 0;
		A.BitCount = 0;
		A.NumPlanes = 1;
		A.DisplayRate = AnimRate;
		A.Flags = 1;
		Out.Serialize(&A, sizeof(animHeader));
	}
	{
		secList = Out.Buffer.Num();
		FRiffHeader Hdr = { mmioFOURCC('L', 'I', 'S', 'T'), 0,mmioFOURCC('f', 'r', 'a', 'm') };
		Out.Serialize(&Hdr, sizeof(FRiffHeader));
	}

	for (INT i = 0; i < NumFrames; ++i)
	{
		INT HeaderOffset = Out.Buffer.Num();
		INT DataSizeOffset,iDataSizeStart;
		{
			FSectionID Hdr = { mmioFOURCC('i', 'c', 'o', 'n'), 0 };
			Out.Serialize(&Hdr, sizeof(FSectionID));
		}
		{
			// Write icon header
			iconHeader H;
			H.Reserved = 0;
			H.fileType = 2;
			H.imgCount = 1;
			H.width = (Data[i].UClip >= 255) ? 0 : Data[i].UClip;
			H.height = (Data[i].VClip >= 255) ? 0 : Data[i].VClip;
			H.numColors = 0;
			H.imgReserved = 0;
			H.HotSpotX = Data[i].HotSpotX;
			H.HotSpotY = Data[i].HotSpotY;
			Out.Serialize(&H, sizeof(iconHeader));
			DataSizeOffset = Out.Buffer.Num();
			DWORD dataSize = 0;
			DWORD curOffset = 22;
			Out << dataSize << curOffset;
			iDataSizeStart = Out.Buffer.Num();
		}
		{
			// Write BMP header
			Win32BitmapInfoHeader H;
			H.size = sizeof(Win32BitmapInfoHeader);
			check(H.size == 40);
			H.width = Data[i].UClip;
			H.height = Data[i].VClip << 1;
			H.planes = 1;
			H.bit_count = 8;
			H.compression = 0;
			H.x_pels_per_meter = 0;
			H.y_pels_per_meter = 0;
			H.clr_important = 0;
			H.clr_used = 0;
			H.size_image = 0;
			Out.Serialize(&H, sizeof(Win32BitmapInfoHeader));
		}
		{
			// Write palette
			INT j;
			FColor* Clr = Data[i].Palette;
			DWORD colcc = 0x0000;
			Out << colcc;
			for (j = 1; j < 256; ++j)
			{
				colcc = Clr[j].TrueColor();
				Out << colcc;
			}

			// Write pixels
			BYTE* px = Data[i].Data;
			INT xs = Data[i].UClip;
			INT ys = Data[i].VClip;
			INT rx = Data[i].USize;
			INT x, y;
			BYTE* yrow;

			for (y = (ys - 1); y >= 0; --y)
			{
				yrow = &px[y * rx];
				for(x=0; x<xs; ++x)
					Out << yrow[x];
			}

			// Write translucent mask.
			BYTE pix;
			INT z, mz;
			for (y = (ys - 1); y >= 0; --y)
			{
				yrow = &px[y * rx];
				for (x = 0; x < xs; x += 8)
				{
					pix = 0;
					mz = Min(8, xs - x);
					for (z = 0; z < mz; ++z)
						pix |= yrow[x + z] ? 0 : (128 >> z);
					Out << pix;
				}
			}
		}

		// Update icon size.
		FSectionID* Hdr = (FSectionID*)&Out.Buffer(HeaderOffset);
		Hdr->Size = Out.Buffer.Num() - (HeaderOffset + 8);
		DWORD* DSize = (DWORD*)&Out.Buffer(DataSizeOffset);
		*DSize = Out.Buffer.Num() - iDataSizeStart;
	}

	// Update size info on header.
	{
		FRiffHeader* Hdr = (FRiffHeader*)&Out.Buffer(0);
		Hdr->Size = Out.Buffer.Num() - 8;
	}
	// List size.
	{
		FRiffHeader* Hdr = (FRiffHeader*)&Out.Buffer(secList);
		Hdr->Size = Out.Buffer.Num() - (secList+8);
	}

	return CreateCursorRaw(&Out.Buffer(0), Out.Buffer.Num());
	unguard;
}

FCustomBitmap::~FCustomBitmap() noexcept(false)
{
	guard(FCustomBitmap::~FCustomBitmap);
	if (Bmp)
	{
		DeleteObject(Bmp);
		Bmp = NULL;
	}
	unguard;
}

inline DWORD FixColor(DWORD In)
{
	return ((In & 0xFF) << 16) | ((In & 0xFF0000) >> 16) | (In & 0xFF00) | 0xFF000000;
}

// Create icon from a P8 palette texture.
UBOOL FCustomBitmap::CreateIconTex(const FAnimIconData& Data, const FColor* MaskColor)
{
	guard(FCustomBitmap::CreateIconTex);
	if (Bmp)
	{
		DeleteObject(Bmp);
		Bmp = NULL;
	}

	DWORD* BitColors = new DWORD[Data.UClip * Data.VClip];
	{
		DWORD* DestColors = BitColors;
		const DWORD* SrcColors = (DWORD*)Data.Palette;
		BYTE* SrcData = Data.Data;

		INT x, BaseY;

		if (MaskColor)
		{
			BYTE Pix;
			DWORD MaskCol = *((DWORD*)MaskColor);
			for (INT y = 0; y < Data.VClip; ++y)
			{
				BaseY = y * Data.USize;
				for (x = 0; x < Data.UClip; ++x)
				{
					Pix = SrcData[BaseY + x];
					*DestColors++ = FixColor(Pix ? SrcColors[SrcData[BaseY + x]] : MaskCol);
				}
			}
		}
		else
		{
			for (INT y = 0; y < Data.VClip; ++y)
			{
				BaseY = y * Data.USize;
				for (x = 0; x < Data.UClip; ++x)
					*DestColors++ = FixColor(SrcColors[SrcData[BaseY + x]]);
			}
		}
	}
	Bmp = CreateBitmap(Data.UClip, Data.VClip,1,sizeof(FColor) * 8, BitColors);
	delete[] BitColors;

	return (Bmp != NULL);
	unguard;
}

/*-----------------------------------------------------------------------------
	FRightClickMenu.
-----------------------------------------------------------------------------*/

FRightClickMenu::~FRightClickMenu() noexcept(false)
{
	guard(FRightClickMenu::~FRightClickMenu);
	DeleteObject(hRightClickMenu);
	for (INT i = 0; i < AllGroups.Num(); ++i)
		DeleteObject(AllGroups(i));
	unguard;
}
FRightClickMenu::FRightClickMenu()
{
	hRightClickMenu = CreatePopupMenu();
}
INT FRightClickMenu::OpenMenu(HWND MenuHandle)
{
	POINT pt;
	GetCursorPos(&pt);
	BOOL Result = TrackPopupMenuEx(hRightClickMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_VERPOSANIMATION | TPM_RETURNCMD, pt.x, pt.y, MenuHandle, NULL);
	return (INT)Result;
}
void FRightClickMenu::AddItem(INT Id, const TCHAR* Text, UBOOL bDisabled, UBOOL bChecked)
{
	HMENU Cur = (PopupStack.Num() ? PopupStack.Last() : hRightClickMenu);
	if (!Text)
		AppendMenuW(Cur, MF_MENUBARBREAK, 0, NULL);
	else AppendMenuW(Cur, (MF_STRING | (bDisabled ? MF_DISABLED : 0) | (bChecked ? MF_CHECKED : 0)), Id, Text);
}
HMENU FRightClickMenu::AddPopup(const TCHAR* Text)
{
	HMENU Cur = GetCurrentStack();
	HMENU NewItem = CreatePopupMenu();
	AppendMenuW(Cur, MF_POPUP, (UINT_PTR)NewItem, Text);
	PopupStack.AddItem(NewItem);
	AllGroups.AddItem(NewItem);
	return NewItem;
}
void FRightClickMenu::EndPopup()
{
	PopupStack.Pop();
}
void FRightClickMenu::RemoveItem(INT Id, HMENU StackLevel)
{
	RemoveMenu(StackLevel ? StackLevel : hRightClickMenu, Id, MF_BYCOMMAND);
}
HMENU FRightClickMenu::GetCurrentStack()
{
	return (PopupStack.Num() ? PopupStack.Last() : hRightClickMenu);
}

FMenuItemBase::~FMenuItemBase() noexcept(false)
{
	guard(FMenuItemBase::~FMenuItemBase);
	if (GroupOwner)
	{
		if (!bIsHidden)
			MoveOffset(-1);
		if (GroupOwner->List == this)
			GroupOwner->List = Next;
		else
		{
			for (FMenuItemBase* L = GroupOwner->List; L; L = L->Next)
			{
				if (L->Next == this)
				{
					L->Next = Next;
					break;
				}
			}
		}
		Next = NULL;
	}
	unguard;
}
void FMenuItemBase::MoveOffset(INT Dir)
{
	for (FMenuItemBase* I = Next; I; I=I->Next)
		I->Offset += Dir;
}

FMenuItem::FMenuItem(FMenuGroup* O, FPersistentRCMenu* M, INT IntID, UBOOL Hidden, UBOOL Disable, UBOOL Check, MenuCallbackFunc C, INT vID, const TCHAR* Txt, const TCHAR* PostTxt)
	: FMenuItemBase(FALSE, O, M, Hidden), bIsDisabled(Disable), bIsChecked(Check), InternalID(IntID), CurText(Txt), OrgText(Txt), PostFixText(PostTxt), Callback(C), ID(vID)
{
	if (PostFixText.Len())
		CurText += PostFixText;
	if (!Hidden)
		AppendMenuW(O->MenuHandle, (MF_STRING | (Disable ? MF_DISABLED : 0) | (Check ? MF_CHECKED : 0)), IntID, *CurText);
}
FMenuItem::~FMenuItem() noexcept(false)
{
	guard(FMenuItem::~FMenuItem);
	if (GroupOwner)
		RemoveMenu(GroupOwner->MenuHandle, InternalID, MF_BYCOMMAND);
	unguard;
}
void FMenuItem::UpdateMenu()
{
	if (!bIsHidden)
	{
		MENUITEMINFO Info;
		Info.cbSize = sizeof(MENUITEMINFO);
		Info.fMask = MIIM_FTYPE | MIIM_STRING;
		Info.fType = MFT_STRING;
		Info.dwTypeData = (LPWSTR)*CurText;

		SetMenuItemInfoW(GroupOwner->MenuHandle, InternalID, FALSE, &Info);
	}
}
void FMenuItem::Serialize(const TCHAR* V, EName Event)
{
	if (Event == NAME_Heading)
	{
		if (PostFixText != V)
		{
			PostFixText = V;
			CurText = OrgText + PostFixText;
			UpdateMenu();
		}
	}
	else if (OrgText != V)
	{
		OrgText = V;
		CurText = OrgText + PostFixText;
		UpdateMenu();
	}
}
void FMenuItem::SetDisabled(UBOOL bDisabled)
{
	if (bIsDisabled != bDisabled)
	{
		bIsDisabled = bDisabled;
		if (!bIsHidden)
			EnableMenuItem(GroupOwner->MenuHandle, InternalID, MF_BYCOMMAND | (bIsDisabled ? MF_GRAYED : MF_ENABLED));
	}
}
void FMenuItem::SetChecked(UBOOL bChecked)
{
	if (bIsChecked != bChecked)
	{
		bIsChecked = bChecked;
		if (!bIsHidden)
			CheckMenuItem(GroupOwner->MenuHandle, InternalID, MF_BYCOMMAND | (bIsChecked ? MF_CHECKED : MF_UNCHECKED));
	}
}
void FMenuItem::SetHidden(UBOOL bHidden)
{
	if (bIsHidden != bHidden)
	{
		bIsHidden = bHidden;
		if (bHidden)
			RemoveMenu(GroupOwner->MenuHandle, Offset, MF_BYPOSITION);
		else InsertMenuW(GroupOwner->MenuHandle, Offset, (MF_BYPOSITION | MF_STRING | (bIsDisabled ? MF_DISABLED : 0) | (bIsChecked ? MF_CHECKED : 0)), InternalID, *CurText);
		MoveOffset(bHidden ? -1 : 1);
	}
}
void FMenuItem::Execute()
{
	if (Callback)
		(*Callback)(ID);
}

FMenuGroup::~FMenuGroup() noexcept(false)
{
	guard(FMenuGroup::~FMenuGroup);
	FMenuItemBase* N;
	for (FMenuItemBase* L = List; L; L = N)
	{
		N = L->Next;
		L->GroupOwner = NULL;
		delete L;
	}

	if (MenuHandle)
	{
		DeleteObject(MenuHandle);
		MenuHandle = NULL;
	}
	unguard;
}
FMenuGroup::FMenuGroup(FPersistentRCMenu* O)
	: FMenuItemBase(TRUE, NULL, O, FALSE), CurText(TEXT("")), List(NULL), Current(NULL), ItemOffset(0)
{
	MenuHandle = CreatePopupMenu();
}
FMenuGroup::FMenuGroup(FPersistentRCMenu* O, FMenuGroup* P, const TCHAR* Str, UBOOL Hidden)
	: FMenuItemBase(TRUE, P, O, Hidden), CurText(Str), List(NULL), Current(NULL), ItemOffset(0)
{
	MenuHandle = CreatePopupMenu();
	if (!bIsHidden)
		AppendMenuW(P->MenuHandle, MF_POPUP, (UINT_PTR)MenuHandle, Str);
}
void FMenuGroup::Serialize(const TCHAR* V, EName Event)
{
	if (!GroupOwner)
		return; // Top tree item cant be modified.
}
void FMenuGroup::SetHidden(UBOOL bHidden)
{
	guard(FMenuGroup::SetHidden);
	if (!GroupOwner)
		return; // Top tree item cant be modified.

	if (bIsHidden != bHidden)
	{
		bIsHidden = bHidden;
		if (bHidden)
			RemoveMenu(GroupOwner->MenuHandle, Offset, MF_BYPOSITION);
		else InsertMenuW(GroupOwner->MenuHandle, Offset, (MF_BYPOSITION | MF_POPUP), (UINT_PTR)MenuHandle, *CurText);
		MoveOffset(bHidden ? (-1) : 1);
	}
	unguard;
}
FMenuItem* FMenuGroup::AddItem(const TCHAR* Text, MenuCallbackFunc Callback, INT ID, UBOOL bDisabled, UBOOL bChecked, UBOOL bHidden, const TCHAR* PostText)
{
	guard(FMenuGroup::AddItem);
	FMenuItem* I = new FMenuItem(this, MenuOwner, (MenuOwner->AllItems.Num() + 1), bHidden, bDisabled, bChecked, Callback, ID, Text, PostText);
	MenuOwner->AllItems.AddItem(I);
	AddItem(I);
	return I;
	unguard;
}
void FMenuGroup::AddLineBreak()
{
	guard(FMenuGroup::AddLineBreak);
	AppendMenuW(MenuHandle, MF_MENUBARBREAK, 0, NULL);
	++ItemOffset;
	unguard;
}

FPersistentRCMenu::FPersistentRCMenu()
	: MainGroup(this)
{
	TopStack = &MainGroup;
}
FPersistentRCMenu::~FPersistentRCMenu() noexcept(false)
{}
FMenuItem* FPersistentRCMenu::AddItem(const TCHAR* Text, MenuCallbackFunc Callback, INT ID, UBOOL bDisabled, UBOOL bChecked, UBOOL bHidden, const TCHAR* PostText)
{
	bMenuCreated = 1;
	return TopStack->AddItem(Text, Callback, ID, bDisabled, bChecked, bHidden, PostText);
}
void FPersistentRCMenu::AddLineBreak()
{
	bMenuCreated = 1;
	TopStack->AddLineBreak();
}
FMenuGroup* FPersistentRCMenu::AddPopup(const TCHAR* Text, UBOOL bHidden)
{
	guard(FPersistentRCMenu::AddPopup);
	Stack.AddItem(TopStack);
	FMenuGroup* NewGroup = new FMenuGroup(this, TopStack, Text, bHidden);
	TopStack->AddItem(NewGroup);
	TopStack = NewGroup;
	return NewGroup;
	unguard;
}
void FPersistentRCMenu::EndPopup()
{
	guard(FPersistentRCMenu::EndPopup);
	if (Stack.Num())
		TopStack = Stack.Pop();
	else TopStack = &MainGroup;
	unguard;
}
INT FPersistentRCMenu::OpenMenu(HWND MainWnd)
{
	guard(FPersistentRCMenu::OpenMenu);
	POINT pt;
	GetCursorPos(&pt);
	BOOL Result = TrackPopupMenuEx(MainGroup.MenuHandle, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_VERPOSANIMATION | TPM_RETURNCMD, pt.x, pt.y, MainWnd, NULL);

	if (Result && AllItems.IsValidIndex(--Result))
	{
		AllItems(Result)->Execute();
		return AllItems(Result)->ID;
	}
	return INDEX_NONE;
	unguard;
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

