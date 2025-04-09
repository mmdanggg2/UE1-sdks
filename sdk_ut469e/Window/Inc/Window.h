/*=============================================================================
	Window.h: GUI window management code.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#if defined(_MSC_VER) && _MSC_VER==1200
#define VK_VOLUME_MUTE                  0xAD
#define VK_VOLUME_DOWN                  0xAE
#define VK_VOLUME_UP                    0xAF
#define WM_INPUT                        0x00FF
#define WM_MOUSEWHEEL                   0x020A
#define WM_DWMCOMPOSITIONCHANGED        0x031E
#define WM_DWMNCRENDERINGCHANGED        0x031F
#define WM_DWMCOLORIZATIONCOLORCHANGED  0x0320
#define WM_DWMWINDOWMAXIMIZEDCHANGE     0x0321
#endif
#include <Windowsx.h>

inline const TCHAR* GetWindowMessageString(UINT Message)
{
	guard(GetWindowMessageString);

#define HANDLE_CASE_(type) case type: return TEXT(#type)
	switch (Message)
	{
		HANDLE_CASE_(WM_NULL);
		HANDLE_CASE_(WM_CREATE);
		HANDLE_CASE_(WM_NCCREATE);
		HANDLE_CASE_(WM_INITDIALOG);
		HANDLE_CASE_(WM_GETMINMAXINFO);
		HANDLE_CASE_(WM_CHAR);
		HANDLE_CASE_(WM_KEYDOWN);
		HANDLE_CASE_(WM_LBUTTONDOWN);
		HANDLE_CASE_(WM_RBUTTONDOWN);
		HANDLE_CASE_(WM_ACTIVATEAPP);
		HANDLE_CASE_(WM_NCACTIVATE);
		HANDLE_CASE_(WM_DESTROY);
		HANDLE_CASE_(WM_DRAWITEM);
		HANDLE_CASE_(WM_MEASUREITEM);
		HANDLE_CASE_(WM_CLOSE);
		HANDLE_CASE_(WM_PAINT);
		HANDLE_CASE_(WM_TIMER);
		HANDLE_CASE_(WM_ENTERIDLE);
		HANDLE_CASE_(WM_SETFOCUS);
		HANDLE_CASE_(WM_ACTIVATE);
		HANDLE_CASE_(WM_KILLFOCUS);
		HANDLE_CASE_(WM_SIZE);
		HANDLE_CASE_(WM_PASTE);
		HANDLE_CASE_(WM_SHOWWINDOW);
		HANDLE_CASE_(WM_COPYDATA);
		HANDLE_CASE_(WM_CAPTURECHANGED);
		HANDLE_CASE_(WM_MDIACTIVATE);
		HANDLE_CASE_(WM_MOUSEMOVE);
		HANDLE_CASE_(WM_LBUTTONUP);
		HANDLE_CASE_(WM_RBUTTONUP);
		HANDLE_CASE_(WM_CUT);
		HANDLE_CASE_(WM_COPY);
		HANDLE_CASE_(WM_UNDO);
		HANDLE_CASE_(WM_SETCURSOR);
		HANDLE_CASE_(WM_COMMAND);
		HANDLE_CASE_(WM_HSCROLL);
		HANDLE_CASE_(WM_VSCROLL);
		HANDLE_CASE_(WM_INPUT);
		HANDLE_CASE_(WM_MOUSEWHEEL);
		HANDLE_CASE_(WM_HELP);
		HANDLE_CASE_(WM_MOVE);
		HANDLE_CASE_(EM_GETPASSWORDCHAR);
		HANDLE_CASE_(WM_TIMECHANGE);
		HANDLE_CASE_(WM_SETFONT);
		HANDLE_CASE_(WM_NCCALCSIZE);
		HANDLE_CASE_(WM_SETTEXT);
		HANDLE_CASE_(WM_PARENTNOTIFY);
		HANDLE_CASE_(WM_WINDOWPOSCHANGING);
		HANDLE_CASE_(WM_WINDOWPOSCHANGED);
		HANDLE_CASE_(WM_GETICON);
		HANDLE_CASE_(WM_NCPAINT);
		HANDLE_CASE_(WM_ERASEBKGND);
		HANDLE_CASE_(WM_IME_SETCONTEXT);
		HANDLE_CASE_(WM_IME_NOTIFY);
		HANDLE_CASE_(WM_DISPLAYCHANGE);
		HANDLE_CASE_(WM_SETREDRAW);
		HANDLE_CASE_(WM_STYLECHANGING);
		HANDLE_CASE_(WM_STYLECHANGED);
		HANDLE_CASE_(WM_SYSKEYDOWN);
		HANDLE_CASE_(WM_CANCELMODE);
		HANDLE_CASE_(WM_SYSCHAR);
		HANDLE_CASE_(WM_SYSCOMMAND);
		HANDLE_CASE_(WM_ENTERMENULOOP);
		HANDLE_CASE_(WM_EXITMENULOOP);
		HANDLE_CASE_(WM_INITMENU);
		HANDLE_CASE_(WM_MENUCHAR);
		HANDLE_CASE_(LB_ADDSTRING);
		HANDLE_CASE_(LB_GETCOUNT);
		HANDLE_CASE_(LB_GETITEMHEIGHT);
		HANDLE_CASE_(LB_GETITEMDATA);
		HANDLE_CASE_(VK_VOLUME_MUTE);
		HANDLE_CASE_(VK_VOLUME_DOWN);
		HANDLE_CASE_(VK_VOLUME_UP);
		HANDLE_CASE_(VK_NUMLOCK);
		HANDLE_CASE_(VK_SCROLL);
		HANDLE_CASE_(WM_DWMCOMPOSITIONCHANGED);
		HANDLE_CASE_(WM_DWMNCRENDERINGCHANGED);
		HANDLE_CASE_(WM_DWMCOLORIZATIONCOLORCHANGED);
		HANDLE_CASE_(WM_DWMWINDOWMAXIMIZEDCHANGE);
		HANDLE_CASE_(WM_NCDESTROY);
		HANDLE_CASE_(WM_KEYUP);
		HANDLE_CASE_(WM_NCHITTEST);
		HANDLE_CASE_(WM_SYSKEYUP);
		HANDLE_CASE_(WM_MENUSELECT);
		HANDLE_CASE_(LB_GETITEMRECT);
		HANDLE_CASE_(LB_FINDSTRING);
		HANDLE_CASE_(LB_RESETCONTENT);
		HANDLE_CASE_(VK_LSHIFT);
		HANDLE_CASE_(VK_RSHIFT);
		HANDLE_CASE_(VK_LCONTROL);
		HANDLE_CASE_(VK_RCONTROL);
		HANDLE_CASE_(VK_LMENU);
		HANDLE_CASE_(VK_RMENU);
		HANDLE_CASE_(VK_INSERT);
		HANDLE_CASE_(LB_ITEMFROMPOINT);
		HANDLE_CASE_(WM_CTLCOLORLISTBOX);
		HANDLE_CASE_(LB_GETCARETINDEX);
	default:
	{
		static TCHAR CustomMsg[32];
		appSprintf(CustomMsg, TEXT("Unknown(0x%x)"), Message);
		return CustomMsg;
	}
	}
#undef HANDLE_CASE_
	unguard;
}

#pragma once
#include "..\Src\Res\WindowRes.h"
#include "UnMsg.h"
#include <Windows.h>
#include <Windowsx.h>

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (push,OBJECT_ALIGNMENT)
#endif

/*-----------------------------------------------------------------------------
	Defines.
-----------------------------------------------------------------------------*/

#ifndef WINDOW_API
#define WINDOW_API DLL_IMPORT
#endif

#define WIN_OBJ 0
#define UNSpecCtr TEXT("Null category")

/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

// Classes.
class WWindow;
class WControl;
class WWizardDialog;
class WWizardPage;
class WDragInterceptor;

// Global functions.
WINDOW_API void InitWindowing();
WINDOW_API HBITMAP LoadFileToBitmap( const TCHAR* Filename, INT& SizeX, INT& SizeY );

// Global variables.
extern WINDOW_API HBRUSH hBrushWhite;
extern WINDOW_API HBRUSH hBrushOffWhite;
extern WINDOW_API HBRUSH hBrushHeadline;
extern WINDOW_API HBRUSH hBrushBlack;
extern WINDOW_API HBRUSH hBrushStipple;
extern WINDOW_API HBRUSH hBrushCurrent;
extern WINDOW_API HBRUSH hBrushDark;
extern WINDOW_API HBRUSH hBrushGrey;
extern WINDOW_API HFONT  hFontText;
extern WINDOW_API HFONT  hFontUrl;
extern WINDOW_API HFONT  hFontHeadline;
extern WINDOW_API class  WLog* GLogWindow;
extern WINDOW_API HINSTANCE hInstanceWindow;
extern WINDOW_API UBOOL GNotify;
extern WINDOW_API UINT WindowMessageOpen;
extern WINDOW_API UINT WindowMessageMouseWheel;
extern WINDOW_API NOTIFYICONDATA NID;
extern WINDOW_API HACCEL hAccel;      // handle to accelerator table
extern WINDOW_API TArray<ACCEL> Accel;
extern WINDOW_API NOTIFYICONDATAA NIDA;
extern WINDOW_API class UClass* GMetaClass;

extern WINDOW_API void (*GIdleTick)(void);

/*-----------------------------------------------------------------------------
	Window class definition macros.
-----------------------------------------------------------------------------*/

inline void MakeWindowClassName( TCHAR* Result, const TCHAR* Base )
{
	guard(MakeWindowClassName);
	appSprintf( Result, TEXT("%lsUnreal%ls"), appPackage(), Base ); // stijn: mem safety NOT OK
	unguard;
}

#if WIN_OBJ
	#define DECLARE_WINDOWCLASS(cls,parentcls,pkg) \
	public: \
		void GetWindowClassName( TCHAR* Result ) {MakeWindowClassName(Result,TEXT(#cls));} \
		void Destroy() \
		{ \
			Super::Destroy(); \
			MaybeDestroy(); \
		} \
		virtual const TCHAR* GetPackageName() { return TEXT(#pkg); }
#else
	#define DECLARE_WINDOWCLASS(cls,parentcls,pkg) \
	public: \
		void GetWindowClassName( TCHAR* Result ) {MakeWindowClassName(Result,TEXT(#cls));} \
		~cls() {MaybeDestroy();} \
		virtual const TCHAR* GetPackageName() { return TEXT(#pkg); }
#endif

#define DECLARE_WINDOWSUBCLASS(cls,parentcls,pkg) \
	DECLARE_WINDOWCLASS(cls,parentcls,pkg) \
	static WNDPROC SuperProc;

#define IMPLEMENT_WINDOWCLASS(cls,clsf) \
	{TCHAR Temp[256]; MakeWindowClassName(Temp,TEXT(#cls)); cls::RegisterWindowClass( Temp, clsf );}

#define IMPLEMENT_WINDOWSUBCLASS(cls,wincls) \
	{TCHAR Temp[256]; MakeWindowClassName(Temp,TEXT(#cls)); cls::SuperProc = cls::RegisterWindowClass( Temp, wincls );}

#define FIRST_AUTO_CONTROL 8192

/*-----------------------------------------------------------------------------
	FRect.
-----------------------------------------------------------------------------*/

struct FPoint
{
	INT X, Y;
	FPoint() = default;

	FPoint( INT InX, INT InY )
	:	X( InX )
	,	Y( InY )
	{}
	static FPoint ZeroValue()
	{
		return FPoint(0,0);
	}
	static FPoint NoneValue()
	{
		return FPoint(INDEX_NONE,INDEX_NONE);
	}
	operator POINT*() const
	{
		return (POINT*)this;
	}
	const INT& operator()( INT i ) const
	{
		return (&X)[i];
	}
	INT& operator()( INT i )
	{
		return (&X)[i];
	}
	static INT Num()
	{
		return 2;
	}
	UBOOL operator==( const FPoint& Other ) const
	{
		return X==Other.X && Y==Other.Y;
	}
	UBOOL operator!=( const FPoint& Other ) const
	{
		return X!=Other.X || Y!=Other.Y;
	}
	FPoint& operator+=( const FPoint& Other )
	{
		X += Other.X;
		Y += Other.Y;
		return *this;
	}
	FPoint& operator-=( const FPoint& Other )
	{
		X -= Other.X;
		Y -= Other.Y;
		return *this;
	}
	FPoint operator+( const FPoint& Other ) const
	{
		return FPoint(*this) += Other;
	}
	FPoint operator-( const FPoint& Other ) const
	{
		return FPoint(*this) -= Other;
	}
};

struct FRect
{
	FPoint Min, Max;
	FRect() = default;

	FRect( INT X0, INT Y0, INT X1, INT Y1 )
	:	Min( X0, Y0 )
	,	Max( X1, Y1 )
	{}
	FRect( FPoint InMin, FPoint InMax )
	:	Min( InMin )
	,	Max( InMax )
	{}
	FRect( RECT R )
	:	Min( R.left, R.top )
	,	Max( R.right, R.bottom )
	{}
	operator RECT*() const
	{
		return (RECT*)this;
	}
	const FPoint& operator()( INT i ) const
	{
		return (&Min)[i];
	}
	FPoint& operator()( INT i )
	{
		return (&Min)[i];
	}
	static INT Num()
	{
		return 2;
	}
	UBOOL operator==( const FRect& Other ) const
	{
		return Min==Other.Min && Max==Other.Max;
	}
	UBOOL operator!=( const FRect& Other ) const
	{
		return Min!=Other.Min || Max!=Other.Max;
	}
	FRect Right( INT Width )
	{
		return FRect( ::Max(Min.X,Max.X-Width), Min.Y, Max.X, Max.Y );
	}
	FRect Bottom( INT Height )
	{
		return FRect( Min.X, ::Max(Min.Y,Max.Y-Height), Max.X, Max.Y );
	}
	FPoint Size()
	{
		return FPoint( Max.X-Min.X, Max.Y-Min.Y );
	}
	INT Width()
	{
		return Max.X-Min.X;
	}
	INT Height()
	{
		return Max.Y-Min.Y;
	}
	FRect& operator+=( const FPoint& P )
	{
		Min += P;
		Max += P;
		return *this;
	}
	FRect& operator-=( const FPoint& P )
	{
		Min -= P;
		Max -= P;
		return *this;
	}
	FRect operator+( const FPoint& P ) const
	{
		return FRect( Min+P, Max+P );
	}
	FRect operator-( const FPoint& P ) const
	{
		return FRect( Min-P, Max-P );
	}
	FRect operator+( const FRect& R ) const
	{
		return FRect( Min+R.Min, Max+R.Max );
	}
	FRect operator-( const FRect& R ) const
	{
		return FRect( Min-R.Min, Max-R.Max );
	}
	FRect Inner( FPoint P ) const
	{
		return FRect( Min+P, Max-P );
	}
	UBOOL Contains( FPoint P ) const
	{
		return P.X>=Min.X && P.X<Max.X && P.Y>=Min.Y && P.Y<Max.Y;
	}
	FRect& DPIScaled(INT DPIX, INT DPIY)
	{
		Min.X = MulDiv(Min.X, DPIX, 96);
		Min.Y = MulDiv(Min.Y, DPIY, 96);
		Max.X = MulDiv(Max.X, DPIX, 96);
		Max.Y = MulDiv(Max.Y, DPIY, 96);
		return *this;
	}
};

/*-----------------------------------------------------------------------------
	FControlSnoop.
-----------------------------------------------------------------------------*/

// For forwarding interaction with a control to an object.
class WINDOW_API FControlSnoop
{
public:
	// FControlSnoop interface.
	virtual void SnoopChar( WWindow* Src, INT Char ) {}
	virtual void SnoopKeyDown( WWindow* Src, INT Char ) {}
	virtual void SnoopLeftMouseDown( WWindow* Src, FPoint P ) {}
	virtual void SnoopRightMouseDown( WWindow* Src, FPoint P ) {}
};

/*-----------------------------------------------------------------------------
	FCommandTarget.
-----------------------------------------------------------------------------*/

//
// Interface for accepting commands.
//
class WINDOW_API FCommandTarget
{
public:
	virtual void Unused() {}
};

//
// Delegate function pointers.
//
typedef void(FCommandTarget::*TDelegate)();
typedef void(FCommandTarget::*TDelegateInt)(INT);

//
// Simple bindings to an object and a member function of that object.
//
struct WINDOW_API FDelegate
{
	FCommandTarget* TargetObject;
	void (FCommandTarget::*TargetInvoke)();
	FDelegate( FCommandTarget* InTargetObject=NULL, TDelegate InTargetInvoke=NULL )
	: TargetObject( InTargetObject )
	, TargetInvoke( InTargetInvoke )
	{}
	virtual void operator()() { if( TargetObject ) (TargetObject->*TargetInvoke)(); }
};
struct WINDOW_API FDelegateInt
{
	FCommandTarget* TargetObject;
	void (FCommandTarget::*TargetInvoke)(int);
	FDelegateInt( FCommandTarget* InTargetObject=NULL, TDelegateInt InTargetInvoke=NULL )
	: TargetObject( InTargetObject )
	, TargetInvoke( InTargetInvoke )
	{}
	virtual void operator()( int I ) { if( TargetObject ) (TargetObject->*TargetInvoke)(I); }
};

// Text formatting.
static inline FString LineFormat(const TCHAR* In)
{
	return FString(In).Replace(TEXT("\\n"), TEXT("\n"));
}

static inline FString LineFormat(FString In)
{
	return In.Replace(TEXT("\\n"), TEXT("\n"));
}

/*-----------------------------------------------------------------------------
	Menu helper functions.
-----------------------------------------------------------------------------*/

inline FString DescribeHotkey(ACCEL &Hotkey)
{
	guard(DescribeHotkey);
	FString Info;
	if (Hotkey.fVirt & FCONTROL)
		Info += TEXT("Ctrl+");
	if (Hotkey.fVirt & FALT)
		Info += TEXT("Alt+");
	if (Hotkey.fVirt & FSHIFT)
		Info += TEXT("Shift+");
	if (Hotkey.fVirt & FVIRTKEY)
	{
		TCHAR name[1024];
		UINT scanCode = MapVirtualKeyW(Hotkey.key, MAPVK_VK_TO_VSC);
		switch (Hotkey.key)
	    {
	        case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN:
	        case VK_RCONTROL: case VK_RMENU:
	        case VK_LWIN: case VK_RWIN: case VK_APPS:
	        case VK_PRIOR: case VK_NEXT:
	        case VK_END: case VK_HOME:
	        case VK_INSERT: case VK_DELETE:
	        case VK_DIVIDE:
	        case VK_NUMLOCK:
	            scanCode |= KF_EXTENDED;
		}
		int result = GetKeyNameTextW(scanCode << 16, name, ARRAY_COUNT(name));
		if (result <= 0)
			return TEXT("");
		name[ARRAY_COUNT(name) - 1] = '\0';
		Info += name;
	}
	else
		Info += (const TCHAR)Hotkey.key;
	return Info;
	unguard;
}

inline FString GetHotkey(INT MenuID)
{
	guard(GetHotkey);
	if (hAccel == NULL)
		return TEXT("");
	if (Accel.Num() == 0)
	{
		int Count = CopyAcceleratorTable(hAccel, NULL, 0);
		if (Count <= 0)
			return TEXT("");
		Accel.AddZeroed(Count);
		int Copied = CopyAcceleratorTable(hAccel, &Accel(0), Count);
		if (Copied < Count)
			Accel.SetSize(Copied);
	}
	for (int i = 0; i < Accel.Num(); i++)
		if (Accel(i).cmd == MenuID)
			return DescribeHotkey(Accel(i));
	return TEXT("");
	unguard;
}

inline HMENU AddHotkeys( HMENU hMenu )
{
	guard(AddHotkeys);
	for( INT i=GetMenuItemCount(hMenu)-1; i>=0; i-- )
	{
		MENUITEMINFO Info;
		TCHAR Buffer[1024];
		Buffer[0] = '\0';
		appMemzero( &Info, sizeof(Info) );
		Info.cbSize     = sizeof(Info);
		Info.fMask      = MIIM_TYPE | MIIM_ID | MIIM_SUBMENU;
		Info.cch        = ARRAY_COUNT(Buffer);
		Info.dwTypeData = Buffer;
		GetMenuItemInfo( hMenu, i, TRUE, &Info );
		const TCHAR* String = (const TCHAR*)Info.dwTypeData;
		if( String )
		{
			FString Hotkey = GetHotkey(Info.wID);
			FString Temp;
			if (Hotkey.Len())
				String = *(((Temp += String) += '\t') += Hotkey);
			const_cast<const TCHAR*&>(Info.dwTypeData) = String;
			SetMenuItemInfo( hMenu, i, TRUE, &Info );
		}
		if( Info.hSubMenu )
			AddHotkeys( Info.hSubMenu );
	}
	return hMenu;
	unguard;
}

inline void RemoveSubmenu(HMENU hMenu, UINT FirstChildItemId)
{
	guard(RemoveSubmenu);
	for (INT i = GetMenuItemCount(hMenu) - 1; i >= 0; i--)
	{
		MENUITEMINFO Info;
		appMemzero(&Info, sizeof(Info));
		Info.cbSize     = sizeof(Info);
		Info.fMask      = MIIM_ID | MIIM_SUBMENU;
		GetMenuItemInfo(hMenu, i, TRUE, &Info);
		if (Info.hSubMenu && GetMenuItemID(Info.hSubMenu, 0) == FirstChildItemId)
			DeleteMenu(hMenu, i, MF_BYPOSITION);
	}
	unguard;
}

//
// Load a menu and localize its text.
//
inline void LocalizeSubMenu( HMENU hMenu, const TCHAR* Name, const TCHAR* Package )
{
	guard(LocalizeSubMenu);
	for( INT i=GetMenuItemCount(hMenu)-1; i>=0; i-- )
	{
		MENUITEMINFO Info;
		TCHAR Buffer[1024];
		appMemzero( &Info, sizeof(Info) );
		Info.cbSize     = sizeof(Info);
		Info.fMask      = MIIM_TYPE | MIIM_ID | MIIM_SUBMENU;
		Info.cch        = ARRAY_COUNT(Buffer);
		Info.dwTypeData = Buffer;
		GetMenuItemInfo( hMenu, i, TRUE, &Info );
		const TCHAR* String = (const TCHAR*)Info.dwTypeData;
		if( String && String[0]=='I' && String[1]=='D' && String[2]=='_' )
		{
			const TCHAR* Localized = Localize( Name, String, Package );			
			FString Hotkey = GetHotkey(Info.wID);
			FString Temp;
			if (Hotkey)
				Localized = *(((Temp += Localized) += '\t') += Hotkey);
			const_cast<const TCHAR*&>(Info.dwTypeData) = Localized;
			SetMenuItemInfo( hMenu, i, TRUE, &Info );
		}
		if( Info.hSubMenu )
			LocalizeSubMenu( Info.hSubMenu, Name, Package );
	}
	unguard;
}
inline WINDOW_API HMENU LoadLocalizedMenu( HINSTANCE hInstance, INT Id, const TCHAR* Name, const TCHAR* Package=GPackage )
{
	guard(LoadLocalizedMenu);
	HMENU hMenu = LoadMenuW( hInstance, MAKEINTRESOURCEW(Id) );
	if( !hMenu )
		appErrorf( TEXT("Failed loading menu: %s %s"), Package, Name );
	LocalizeSubMenu( hMenu, Name, Package );
	return hMenu;
	unguard;
}

inline void MenuItemPrintf( HMENU hMenu, UINT ItemId, ... )
{
	guard(MenuItemPrintf);

	MENUITEMINFO Info;
	TCHAR Buffer[1024];
	appMemzero( &Info, sizeof(Info) );
	Info.cbSize     = sizeof(Info);
	Info.fMask      = MIIM_TYPE;
	Info.cch        = ARRAY_COUNT(Buffer);
	Info.dwTypeData = Buffer;
	GetMenuItemInfo( hMenu, ItemId, FALSE, &Info );
	const TCHAR* String = (const TCHAR*)Info.dwTypeData;
	if( !String ) String = TEXT("Failed load text");

	va_list ArgPtr;
	va_start(ArgPtr, ItemId);
	FString Temp = FString::Printf(String, ArgPtr);
	va_end(ArgPtr);
	
	const_cast<const TCHAR*&>(Info.dwTypeData) = *Temp;
	SetMenuItemInfo( hMenu, ItemId, FALSE, &Info );
	
	unguard;
}

inline void SetTextMenuItem(HMENU hMenu, UINT ItemId, FString Text)
{
	guard(SetTextMenuItem);

	MENUITEMINFO Info;
	TCHAR Buffer[1024];
	appMemzero( &Info, sizeof(Info) );
	Info.cbSize     = sizeof(Info);
	Info.fMask      = MIIM_TYPE;
	Info.cch        = ARRAY_COUNT(Buffer);
	Info.dwTypeData = Buffer;
	GetMenuItemInfo(hMenu, ItemId, FALSE, &Info);
	// preserve hotkey
	FString OldText = &Buffer[0];
	INT Pos = OldText.InStr(TEXT("\t"));
	if (Pos >= 0)
		Text += OldText.Mid(Pos);
	const_cast<const TCHAR*&>(Info.dwTypeData) = *Text;
	SetMenuItemInfo(hMenu, ItemId, FALSE, &Info);
	
	unguard;
}

//
// Toggle a menu item and return 0 if it's now off, 1 if it's now on.
//
inline WINDOW_API UBOOL ToggleMenuItem( HMENU hMenu, UINT Item )
{
	guard(ToggleMenuItem);
	if( GetMenuState(hMenu,Item,MF_BYCOMMAND)&MF_CHECKED )
	{
		// Now unchecked.
		CheckMenuItem( hMenu, Item, MF_UNCHECKED );
		return 0;
	}
	else
	{
		// Now checked.
		CheckMenuItem( hMenu, Item, MF_CHECKED );
		return 1;
	}
	unguard;
}

/*-----------------------------------------------------------------------------
	FWindowsBitmap.
-----------------------------------------------------------------------------*/

// A bitmap.
class FWindowsBitmap
{
public:
	INT SizeX, SizeY, Keep;
	FWindowsBitmap( UBOOL InKeep=0 )
	: SizeX( 0 )
	, SizeY( 0 )
	, Keep( InKeep )
	{}
	~FWindowsBitmap()
	{
		if( hBitmap && !Keep )
			DeleteObject( hBitmap );
	}
	UBOOL LoadFile( const TCHAR* Filename )
	{
		if( hBitmap )
			DeleteObject( hBitmap );
		hBitmap = LoadFileToBitmap( Filename, SizeX, SizeY );
		return hBitmap!=NULL;
	}
	HBITMAP GetBitmapHandle()
	{
		return hBitmap;
	}
private:
	HBITMAP hBitmap{NULL};
	void operator=( FWindowsBitmap& ) {}
};

/*-----------------------------------------------------------------------------
	WWindow.
-----------------------------------------------------------------------------*/

static HPEN penRaised = CreatePen( PS_SOLID, 1, RGB(159,159,159) ),
			penSunken = CreatePen( PS_SOLID, 1, RGB(106,106,106) );

#if WIN_OBJ
	#define W_DECLARE_ABSTRACT_CLASS(a,b,c) DECLARE_ABSTRACT_CLASS(a,b,c) 
	#define W_DECLARE_CLASS(a,b,c) DECLARE_CLASS(a,b,c) 
	#define W_IMPLEMENT_CLASS(a) IMPLEMENT_CLASS(a)
#else
	#define W_DECLARE_ABSTRACT_CLASS(a,b,c) public:
	#define W_DECLARE_CLASS(a,b,c) public:
	#define W_IMPLEMENT_CLASS(a)
#endif

typedef HRESULT(__stdcall *SetWindowThemeType)(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList);

// An operating system window.
class WINDOW_API WWindow : 
#if WIN_OBJ
public UObject, 
#endif
public FCommandTarget
{
	W_DECLARE_ABSTRACT_CLASS(WWindow,UObject,CLASS_Transient);

	// Variables.
	HWND					hWnd;
	FName					PersistentName;
	WORD					ControlId, TopControlId;
	BITFIELD				Destroyed:1;
	BITFIELD				MdiChild:1;
	WWindow*				OwnerWindow;
	FNotifyHook*			NotifyHook;
	FControlSnoop*			Snoop;
	TArray<class WControl*>	Controls;
	BOOL m_bShow;
	WPARAM LastwParam;	
	LPARAM LastlParam;

	// Static.
	static INT              ModalCount;
	static INT				DPIX;
	static INT				DPIY;
	static TArray<WWindow*> _Windows;
	static TArray<WWindow*> _DeleteWindows;
	static UBOOL			AppActive;

#if _DEBUG && 0
	FString OrigTag;
	INT OrigIndex;
#endif

	static void AddWindow(WWindow* Wnd, const TCHAR* Tag)
	{		
#if _DEBUG && 0
		Wnd->OrigTag = Tag;
		Wnd->OrigIndex =
#endif
			_Windows.AddItem(Wnd);
#if _DEBUG && 0
		appDebugMessagef(TEXT("Window %d - Registered Window %s"), Wnd->OrigIndex, Tag);
#endif
	}

	static void RemoveWindow(WWindow* Wnd)
	{
#if _DEBUG && 0
		INT Removed =
#endif
			_Windows.RemoveItem(Wnd);
#if _DEBUG && 0
		appDebugMessagef(TEXT("Window %d - Unregistered Window %s - %s"), Wnd->OrigIndex, *Wnd->OrigTag, Removed ? TEXT("Success") : TEXT("FAIL"));
#endif
	}

	// Use this procedure for modeless dialogs.
	static INT_PTR CALLBACK StaticDlgProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
	{
		try
		{
			INT i;
			for (i = 0; i < _Windows.Num(); i++)
				if (_Windows(i)->hWnd == hwndDlg)
					break;
			if (i == _Windows.Num() && uMsg == WM_INITDIALOG)
			{
				WWindow* WindowCreate = (WWindow*)lParam;
				check(WindowCreate);
				check(!WindowCreate->hWnd);
				WindowCreate->hWnd = hwndDlg;
				for (i = 0; i < _Windows.Num(); i++)
					if (_Windows(i) == WindowCreate)
						break;
				check(i < _Windows.Num());
			}
			if (i != _Windows.Num() && !GIsCriticalError)
			{
				_Windows(i)->WndProc(uMsg, wParam, lParam);
			}

			// Give up cycles.
			//
			::Sleep(0);
		}
		catch(...)
		{
			appUnwindf(TEXT("WWindow::StaticDlgProc (Message=%ls)"), GetWindowMessageString(uMsg));
		}

		return 0;		
	}
	static LRESULT APIENTRY StaticWndProc( HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam )
	{
		try
		{		
			INT i;
			for (i = 0; i < _Windows.Num(); i++)
				if (_Windows(i)->hWnd == hWnd)
					break;
			if (i == _Windows.Num() && (Message == WM_NCCREATE || Message == WM_INITDIALOG))
			{
				WWindow* WindowCreate
					= Message != WM_NCCREATE
					? (WWindow*)lParam
					: (GetWindowLongW(hWnd, GWL_EXSTYLE) & WS_EX_MDICHILD)
					? (WWindow*)((MDICREATESTRUCT*)((CREATESTRUCT*)lParam)->lpCreateParams)->lParam
					: (WWindow*)((CREATESTRUCT*)lParam)->lpCreateParams;
				check(WindowCreate);
				check(!WindowCreate->hWnd);
				WindowCreate->hWnd = hWnd;
				for (i = 0; i < _Windows.Num(); i++)
					if (_Windows(i) == WindowCreate)
						break;
				check(i < _Windows.Num());
			}
			if (i == _Windows.Num() || GIsCriticalError)
			{
				// Gets through before WM_NCCREATE: WM_GETMINMAXINFO
				return DefWindowProcW(hWnd, Message, wParam, lParam);
			}
			else
			{
				return _Windows(i)->WndProc(Message, wParam, lParam);
			}
		}
		catch(...)
		{
			appUnwindf(TEXT("WWindow::StaticWndProc (Message=%ls)"), GetWindowMessageString(Message));
		}

		return 0;
	}
	static WNDPROC RegisterWindowClass( const TCHAR* Name, DWORD Style )
	{
		guard(WWindow::RegisterWindowClass);
		WNDCLASSEXW Cls;
		appMemzero( &Cls, sizeof(Cls) );
		Cls.cbSize			= sizeof(Cls);
		Cls.style			= Style;
		Cls.lpfnWndProc		= StaticWndProc;
		Cls.hInstance		= hInstanceWindow;
		Cls.hIcon			= LoadIconW(hInstanceWindow,MAKEINTRESOURCEW((GIsEditor?IDICON_Editor:IDICON_Mainframe)));
		Cls.lpszClassName	= Name;
		Cls.hIconSm			= LoadIconW(hInstanceWindow,MAKEINTRESOURCEW((GIsEditor?IDICON_Editor:IDICON_Mainframe)));
		verify(RegisterClassExW( &Cls ));
		return NULL;
		unguard;
	}

	// Structors.
	WWindow( FName InPersistentName=NAME_None, WWindow* InOwnerWindow=NULL )
#if WIN_OBJ
	:	UObject				( EC_InPlaceConstructor, WWindow::StaticClass(), UObject::GetTransientPackage(), NAME_None, 0 )
	,	hWnd				( NULL )
#else
		: hWnd(NULL)
#endif
		, PersistentName(InPersistentName)
		, ControlId(0)
		, TopControlId(FIRST_AUTO_CONTROL)
		, Destroyed(0)
		, MdiChild(0)
		, OwnerWindow(InOwnerWindow)
		, NotifyHook(0)
		, Snoop(NULL)
		, m_bShow(0)
		, LastwParam(0)
		, LastlParam(0)
	{
	}
#if WIN_OBJ
	void Destroy()
	{
		guard(WWindow::Destroy);
		Super::Destroy();
		MaybeDestroy();
		m_bShow = FALSE;
		WWindow::_DeleteWindows.RemoveItem( this );
		unguard;
	}
#else
	virtual ~WWindow()
	{
		guard(WWindow::~WWindow);
		MaybeDestroy();
		WWindow::_DeleteWindows.RemoveItem( this );
		unguard;
	}
#endif

	// Accessors.
	FRect GetClientRect() const
	{
		RECT R;
		::GetClientRect( hWnd, &R );
		return FRect( R );
	}
	void MoveWindow( FRect R, UBOOL bRepaint )
	{
		::MoveWindow( hWnd, R.Min.X, R.Min.Y, R.Width(), R.Height(), bRepaint );
	}
	void MoveWindow( int Left, int Top, int Width, int Height, UBOOL bRepaint )
	{
		::MoveWindow( hWnd, Left, Top, Width, Height, bRepaint );
	}
	FRect GetWindowRect() const
	{
		RECT R = {};
		BOOL Success = ::GetWindowRect( hWnd, &R );
		return OwnerWindow && Success ? OwnerWindow->ScreenToClient(R) : FRect(R);
	}
	FPoint ClientToScreen( const FPoint& InP )
	{
		POINT P;
		P.x = InP.X;
		P.y = InP.Y;
		::ClientToScreen( hWnd, &P );
		return FPoint( P.x, P.y );
	}
	FPoint ScreenToClient( const FPoint& InP )
	{
		POINT P;
		P.x = InP.X;
		P.y = InP.Y;
		::ScreenToClient( hWnd, &P );
		return FPoint( P.x, P.y );
	}
	FRect ClientToScreen( const FRect& InR )
	{
		return FRect( ClientToScreen(InR.Min), ClientToScreen(InR.Max) );
	}
	FRect ScreenToClient( const FRect& InR )
	{
		return FRect( ScreenToClient(InR.Min), ScreenToClient(InR.Max) );
	}
	FPoint GetCursorPos()
	{
		FPoint Mouse;
		::GetCursorPos( Mouse );
		return ScreenToClient( Mouse );
	}
	void Show( UBOOL Show )
	{
		guard(WWindow::Show);
		m_bShow = Show;
		
		if (Show)
		{			
			if (::IsIconic(hWnd))
			{
				// if the window is minimized, just restore it
				ShowWindow(hWnd, SW_RESTORE);
			}
			else
			{
				// show the window
				ShowWindow(hWnd, SW_SHOW);
				// See OldUnreal/UnrealTournamentPatches/issues/3
				BringWindowToTop(hWnd);
			}
		}
		else
		{
			ShowWindow(hWnd, SW_HIDE);
		}

		unguard;
	}
	void UseClassicTheme(UBOOL Use)
	{
		guard(WWindow::UseClassicTheme);
		static HMODULE UxThemeDll = LoadLibrary(TEXT("uxtheme.dll"));
		static SetWindowThemeType SetWindowTheme = UxThemeDll ? (SetWindowThemeType)GetProcAddress(UxThemeDll, "SetWindowTheme") : NULL;
		if (SetWindowTheme)
			SetWindowTheme(hWnd, Use ? TEXT(" ") : NULL, Use ? TEXT(" ") : NULL);
		unguard;
	}

	// WWindow interface.
	virtual void Serialize( FArchive& Ar )
	{
		guard(WWindow::Serialize);
		//!!UObject interface.
		//!!Super::Serialize( Ar );
		Ar << PersistentName;
		unguard;
	}
	virtual const TCHAR* GetPackageName()
	{
		return TEXT("Window");
	}
	virtual void DoDestroy()
	{
		guard(WWindow::DoDestroy);
		if( NotifyHook )
			NotifyHook->NotifyDestroy( this );
		if( hWnd )
			DestroyWindow( *this );
		RemoveWindow(this);
		unguard;
	}
	virtual void GetWindowClassName( TCHAR* Result )=0;
	virtual LRESULT WndProc( UINT Message, WPARAM wParam, LPARAM lParam )
	{
		try
		{
			LastwParam = wParam;
			LastlParam = lParam;

			// Message snoop.
			if( Snoop )
			{
				guard(Snoop);
				if( Message==WM_CHAR )
					Snoop->SnoopChar( this, wParam );
				else if( Message==WM_KEYDOWN )
					Snoop->SnoopKeyDown( this, wParam );
				else if( Message==WM_LBUTTONDOWN )
					Snoop->SnoopLeftMouseDown( this, FPoint(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)) );
				else if( Message==WM_RBUTTONDOWN )
					Snoop->SnoopRightMouseDown( this, FPoint(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)) );
				unguard;
			}

			// Special multi-window activation handling.
			if( !MdiChild && !ModalCount )
			{
				guard(Special);				
				if( Message==WM_ACTIVATEAPP )
				{
					AppActive = wParam;
					SendMessageW( hWnd, WM_NCACTIVATE, wParam, 0 );
				}
				else if( Message==WM_NCACTIVATE && AppActive && !wParam )
				{
					return 1;
				}
				unguard;
			}

			// Message processing.
			if( Message==WM_DESTROY )
			{
				guard(WM_DESTROY);
				OnDestroy();
				unguard;
			}
			else if ( Message==WM_CTLCOLORBTN )
			{
				guard(WM_CTLCOLORBTN);
				return (LRESULT)OnCtlColorBtn((HDC)wParam, (HWND)lParam);
				unguard;
			}
			else if( Message==WM_DRAWITEM )
			{
				guard(WM_DRAWITEM);
				DRAWITEMSTRUCT* Info = (DRAWITEMSTRUCT*)lParam;
				for( INT i=0; i<Controls.Num(); i++ )
					if( ((WWindow*)Controls(i))->hWnd==Info->hwndItem )
						{((WWindow*)Controls(i))->OnDrawItem(Info); break;}
				return 1;
				unguard;
			}
			else if( Message==WM_MEASUREITEM )
			{
				guard(WM_MEASUREITEM);
				MEASUREITEMSTRUCT* Info = (MEASUREITEMSTRUCT*)lParam;
				for( INT i=0; i<Controls.Num(); i++ )
					if( ((WWindow*)Controls(i))->ControlId==Info->CtlID )
						{((WWindow*)Controls(i))->OnMeasureItem(Info); break;}
				return 1;
				unguard;
			}
			else if( Message==WM_CLOSE )
			{
				guard(WM_CLOSE);
				OnClose();
				unguard;
			}
			else if( Message==WM_CHAR )
			{
				guard(WM_CHAR);
				OnChar( wParam );
				unguard;
			}
			else if( Message==WM_KEYDOWN )
			{
				guard(WM_KEYDOWN);
				OnKeyDown( wParam );
				unguard;
			}
			else if( Message==WM_PAINT )
			{
				guard(WM_PAINT);
				OnPaint();
				unguard;
			}
			else if( Message==WM_CREATE )
			{
				guard(WM_CREATE);
				OnCreate();
				unguard;
			}
			else if( Message==WM_TIMER )
			{
				guard(WM_TIMER);
				OnTimer();
				unguard;
			}
			else if( Message==WM_INITDIALOG )
			{
				guard(WM_INITDIALOG);
				OnInitDialog();
				unguard;
			}
			else if( Message==WM_ENTERIDLE )
			{
				guard(WM_ENTERIDLE);
				OnEnterIdle();
				unguard;
			}
			else if( Message==WM_SETFOCUS )
			{
				guard(WM_SETFOCUS);
				OnSetFocus( (HWND)wParam );
				unguard;
			}
			else if( Message==WM_ACTIVATE )
			{
				guard(WM_ACTIVATE);
				OnActivate( LOWORD(wParam)!=0 );
				unguard;
			}
			else if( Message==WM_KILLFOCUS )
			{
				guard(WM_KILLFOCUS);
				OnKillFocus( (HWND)wParam );
				unguard;
			}
			else if( Message==WM_SIZE )
			{
				guard(WM_SIZE);
				if (GIsEditor && GetWindow(hWnd, GW_OWNER)) // we not main window
					UseClassicTheme(IsIconic(hWnd));
				OnSize( wParam, LOWORD(lParam), HIWORD(lParam) );
				SaveWindowPos();
				unguard;
			}
			else if( Message==WM_MOVE )
			{
				guard(WM_MOVE);
				SaveWindowPos();
				unguard;
			}
			else if( Message==WM_PASTE )
			{
				guard(WM_PASTE);
				OnPaste();
				unguard;
			}
			else if( Message==WM_SHOWWINDOW )
			{
				guard(WM_SHOWWINDOW);
				if ((lParam == SW_PARENTCLOSING || lParam == SW_PARENTOPENING) && IsIconic(hWnd))
					return 1;
				OnShowWindow( wParam );
				unguard;
			}
			else if( Message==WM_COPYDATA )
			{
				guard(WM_COPYDATA);
				OnCopyData( (HWND)wParam, (COPYDATASTRUCT*)lParam );
				unguard;
			}
			else if( Message==WM_CAPTURECHANGED )
			{
				guard(WM_CAPTURECHANGED);
				OnReleaseCapture();
				unguard;
			}
			else if( Message==WM_MDIACTIVATE )
			{
				guard(WM_MDIACTIVATE);
				OnMdiActivate( (HWND)lParam==hWnd );
				unguard;
			}
			else if( Message==WM_MOUSEMOVE )
			{
				guard(WM_MOUSEMOVE);
				OnMouseMove( wParam, FPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)) );
				unguard;
			}
			else if( Message==WM_LBUTTONDOWN )
			{
				guard(WM_LBUTTONDOWN);
				OnLeftButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
				unguard;
			}
			else if( Message==WM_RBUTTONDOWN )
			{
				guard(WM_RBUTTONDOWN);
				OnRightButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
				unguard;
			}
			else if( Message==WM_LBUTTONUP )
			{
				guard(WM_LBUTTONUP);
				OnLeftButtonUp();
				unguard;
			}
			else if( Message==WM_RBUTTONUP )
			{
				guard(WM_RBUTTONUP);
				OnRightButtonUp();
				unguard;
			}
			else if( Message==WM_CUT )
			{
				guard(WM_CUT);
				OnCut();
				unguard;
			}
			else if( Message==WM_COPY )
			{
				guard(WM_COPY);
				OnCopy();
				unguard;
			}
			else if( Message==WM_UNDO )
			{
				guard(WM_UNDO);
				OnUndo();
				unguard;
			}
			else if( Message==WM_SETCURSOR )
			{
				guard(WM_SETCURSOR);
				if( OnSetCursor() )
					return 1;
				unguard;
			}
			else if( Message==WM_NOTIFY )
			{
				guard(WM_NOTIFY);
				for( INT i=0; i<Controls.Num(); i++ )
					if(wParam==((WWindow*)Controls(i))->ControlId
							&& ((WWindow*)Controls(i))->InterceptControlCommand(Message,wParam,lParam) )
						return 1;
				OnCommand( wParam );
				unguard;
			}
			else if( Message==WM_VSCROLL )
			{
				guard(WM_VSCROLL);
				OnVScroll( wParam, lParam );
				unguard;
			}
			else if (Message == WM_MOUSEWHEEL)
			{
				//guard(WM_MOUSEWHEEL);
				SWORD zDelta = HIWORD(wParam);

				//if (GIsEditor)
				{
					if (zDelta < 0)
						OnVScroll(MAKEWPARAM(SB_LINEDOWN, 0), WM_MOUSEWHEEL);// Hack to make scrollbar working in 2DShapeEditor.
					else if (zDelta > 0)
						OnVScroll(MAKEWPARAM(SB_LINEUP, 0), WM_MOUSEWHEEL);
				}
				//unguard;
 			}
			else if( Message==WM_KEYUP)
			{
				guard(WM_KEYU);
				OnKeyUp( wParam, lParam );
				unguard;
			}
			else if( Message==WM_COMMAND || Message==WM_HSCROLL )
			{
				guard(WM_COMMAND);			
				for( INT i=0; i<Controls.Num(); i++ )
					if((HWND)lParam==((WWindow*)Controls(i))->hWnd
							&& ((WWindow*)Controls(i))->InterceptControlCommand(Message,wParam,lParam) )
						return 1;
				// stijn: we should only pass the low word here. The high word indicates
				// whether the command was generated through an accelerator table
				OnCommand( LOWORD(wParam) ); 
				// stijn: ugly hack here. Docking and undocking destroys the window.
				// Normally, we call the default proc after dispatching a message, but
				// in this case this is not safe since CallDefaultProc is a virtual func.
				// The CallDefaultProc dispatch will therefore dereference the this pointer
				// which becomes invalid after docking
				if (wParam == 40333) // IDMN_MB_DOCK - declared in UnrealEd resources
					return 1;
				unguard;
			}
			else if( Message==WM_SYSCOMMAND )
			{
				guard(WM_SYSCOMMAND);
				if( OnSysCommand( wParam ) )
					return 1;
				if (GIsEditor && wParam == SC_MINIMIZE && GetWindow(hWnd, GW_OWNER)) // we not main window
				{
					MINIMIZEDMETRICS prev;
					prev.cbSize = sizeof(prev);
					SystemParametersInfo(SPI_GETMINIMIZEDMETRICS, sizeof(prev), &prev, 0);
					MINIMIZEDMETRICS change = prev;
					change.iArrange = ARW_BOTTOMRIGHT | ARW_LEFT;
					SystemParametersInfo(SPI_SETMINIMIZEDMETRICS, sizeof(change), &change, 0);
					try {
						LRESULT ret = CallDefaultProc( Message, wParam, lParam );
						SystemParametersInfo(SPI_SETMINIMIZEDMETRICS, sizeof(prev), &prev, 0);
						return ret;
					}
					catch( ... )
					{ // for not spoil system settings
						SystemParametersInfo(SPI_SETMINIMIZEDMETRICS, sizeof(prev), &prev, 0);
						return 0;
					}
				}
				unguard;
			}
			return CallDefaultProc( Message, wParam, lParam );
		}
		catch( ... )
		{
			// This exception prevents the message from being routed to the default proc.
			return 0;
		}
	}
	virtual LRESULT CallDefaultProc( UINT Message, WPARAM wParam, LPARAM lParam )
	{
		if( MdiChild )
			return DefMDIChildProcW( hWnd, Message, wParam, lParam );
		else
			return DefWindowProcW( hWnd, Message, wParam, lParam );
	}
	virtual UBOOL InterceptControlCommand( UINT Message, WPARAM wParam, LPARAM lParam )
	{
		return 0;
	}
	virtual FString GetText()
	{
		guard(WWindow::GetText);
		//check(hWnd);
		INT Length = GetLength();
		if (Length <= 0)
			return TEXT("");
		// Do not pass empty string into constructor. There can be optimization about empty FStrings.
		FString Out(TEXT("T"), Length);
		SendMessage( *this, WM_GETTEXT, Length+1, (LPARAM)&Out[0] );
		return Out;
		unguard;
	}
	virtual void SetText( const TCHAR* Text )
	{
		guard(WWindow::SetText);
		check(hWnd);
		SendMessageW( *this, WM_SETTEXT, 0, (LPARAM)Text );
		unguard;
	}
	virtual INT GetLength()
	{
		guard(WWindow::GetLength);
		check(hWnd);
		INT Length = SendMessageW(*this, WM_GETTEXTLENGTH, 0, 0);
		if (Length == LB_ERR)
			Length = 0;
		return Length;
		unguard;
	}
	void SetNotifyHook( FNotifyHook* InNotifyHook )
	{
		guard(WWindow::SetNotifyHook);
		NotifyHook = InNotifyHook;
		unguard;
	}

	// WWindow notifications.
	virtual void OnCopyData( HWND hWndSender, COPYDATASTRUCT* CD )
	{}
	virtual void OnSetFocus( HWND hWndLosingFocus )
	{}
	virtual void OnKillFocus( HWND hWndGaininFocus )
	{}
	virtual void OnSize( DWORD Flags, INT NewX, INT NewY )
	{}
	virtual void OnCommand( INT Command )
	{}
	virtual int OnSysCommand( INT Command )
	{
		return 0;
	}
	virtual void OnActivate( UBOOL Active )
	{}
	virtual void OnChar( TCHAR Ch )
	{}
	virtual void OnKeyDown( TCHAR Ch )
	{}
	virtual void OnCut()
	{}
	virtual void OnCopy()
	{}
	virtual void OnPaste()
	{}
	virtual void OnShowWindow( UBOOL bShow )
	{}
	virtual void OnUndo()
	{}
	virtual void OnVScroll( WPARAM wParam, LPARAM lParam )
	{}	
	virtual void OnKeyUp( WPARAM wParam, LPARAM lParam )
	{}
	virtual void OnPaint()
	{}
	virtual void OnCreate()
	{}
	virtual HBRUSH OnCtlColorBtn( HDC wParam, HWND lParam )
	{
		return (HBRUSH)GetClassLongPtr(hWnd, GCLP_HBRBACKGROUND);
	}
	virtual void OnDrawItem( DRAWITEMSTRUCT* Info )
	{}
	virtual void OnMeasureItem( MEASUREITEMSTRUCT* Info )
	{}
	virtual void OnInitDialog()
	{}
	virtual void OnEnterIdle()
	{
		if (GIdleTick)
		{
			GIdleTick();
			PostMessage(hWnd, WM_NULL, 0, 0);
		}
	}
	virtual void OnMouseEnter()
	{}
	virtual void OnMouseLeave()
	{}
	virtual void OnMouseHover()
	{}
	virtual void OnTimer()
	{}
	virtual void OnReleaseCapture()
	{}
	virtual void OnMdiActivate( UBOOL Active )
	{}
	virtual void OnMouseMove( DWORD Flags, FPoint Location )
	{}
	virtual void OnLeftButtonDown(INT X, INT Y)
	{}
	virtual void OnRightButtonDown(INT X, INT Y)
	{}
	virtual void OnLeftButtonUp()
	{}
	virtual void OnRightButtonUp()
	{}
	virtual void OnFinishSplitterDrag( WDragInterceptor* Drag, UBOOL Success )
	{}
	virtual INT OnSetCursor()
	{
		guard(WWindow::OnSetCursor);
		return 0;
		unguard;
	}
	virtual void OnClose()
	{
		guard(WWindow::OnClose);
		if( MdiChild )
			SendMessage( OwnerWindow->hWnd, WM_MDIDESTROY, (WPARAM)hWnd, 0 );
		else
			DestroyWindow( *this );
		unguard;
	}
	virtual void OnDestroy()
	{
		guard(WWindow::OnDestroy);
		check(hWnd);
		SaveWindowPos();
		RemoveWindow(this);
		hWnd = NULL;
		unguard;
	}

	// WWindow functions.
	void SaveWindowPos()
	{
		guard(WWindow::SaveWindowPos);
		if( PersistentName!=NAME_None && PersistentName.IsValid() )
		{
			RECT RR;
			::GetWindowRect(hWnd, &RR);
			FRect R = FRect(RR);
			UBOOL Maximized = IsZoomed(hWnd);
			if (GetWindowLong(hWnd, GWL_STYLE) & WS_MAXIMIZEBOX)
				GConfig->SetBool( TEXT("WindowPositions"), *(FString(*PersistentName)+TEXT(".Maximized")), Maximized);
			if (!Maximized && !IsIconic(hWnd))
			{
				GConfig->SetString( TEXT("WindowPositions"), *PersistentName, *FString::Printf( TEXT("(X=%i,Y=%i,XL=%i,YL=%i)"), R.Min.X, R.Min.Y, R.Width(), R.Height() ) );
				GConfig->Flush(0);
			}
		}
		unguard;
	}
	// stijn: mark for deletion, but don't do the effective destruction right now because we might be in this WWindow's WndProc
	void DelayedDestroy()
	{
		_DeleteWindows.AddUniqueItem(this);
	}
	void MaybeDestroy()
	{
		guard(WWindow::MaybeDestroy);
		if( !Destroyed )
		{
			Destroyed=1;
			DoDestroy();
		}
		unguard;
	}
	void _CloseWindow()
	{
		guard(WWindow::_CloseWindow);
		check(hWnd);
		DestroyWindow( *this );
		unguard;
	}
	operator HWND() const
	{
		return hWnd;
	}
	void SetFont( HFONT hFont )
	{
		guard(WWindow::SetFont);
		SendMessageW( *this, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(0,0) );
		unguard;
	}
	void PerformCreateWindowEx( DWORD dwExStyle, LPCTSTR lpWindowName, DWORD dwStyle, INT x, INT y, INT nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance )
	{
		guard(PerformCreateWindowEx);
		check(hWnd==NULL);

		// Retrieve remembered position.
		FString Pos;
		if
		(	PersistentName!=NAME_None 
		&&	GConfig->GetString( TEXT("WindowPositions"), *PersistentName, Pos ) )
		{
			// Get saved position.
			Parse( *Pos, TEXT("X="), x );
			Parse( *Pos, TEXT("Y="), y );
			if( dwStyle & WS_SIZEBOX )
			{
				Parse( *Pos, TEXT("XL="), nWidth );
				Parse( *Pos, TEXT("YL="), nHeight );
				nWidth = Max(MulDiv(100, DPIX, 96), nWidth);
				nHeight = Max(MulDiv(80, DPIY, 96), nHeight);
			}

			// Count identical windows already opened.
			INT Count=0;
			for( INT i=0; i<_Windows.Num(); i++ )
			{
				Count += _Windows(i)->PersistentName==PersistentName;
			}
			if( Count )
			{
				// Move away.
				x += Count*16;
				y += Count*16;
			}

			// Clip size to screen.
			RECT Desktop;
			::GetWindowRect( GetDesktopWindow(), &Desktop );
			if (dwStyle & WS_SIZEBOX)
			{
				nWidth = Min((int)(Desktop.right - Desktop.left), nWidth);
				nHeight = Min((int)(Desktop.bottom - Desktop.top), nHeight);
			}
			if (x + nWidth  > Desktop.right  ) x = Desktop.right  - nWidth;
			if (y + nHeight > Desktop.bottom ) y = Desktop.bottom - nHeight;
			x = Max((int)(Desktop.left), x);
			y = Max((int)(Desktop.top), y);
		}
		if (PersistentName != NAME_None && (dwStyle & WS_MAXIMIZEBOX))
		{
			UBOOL Maximized = FALSE;
			GConfig->GetBool( TEXT("WindowPositions"), *(FString(*PersistentName)+TEXT(".Maximized")), Maximized);
			if (Maximized)
				dwStyle |= WS_MAXIMIZE;
		}

		// Create window.		
		TCHAR ClassName[256];
		GetWindowClassName( ClassName );
		AddWindow(this, ClassName);
		//hinstance must match window class hinstance!!
		HWND hWndCreated = CreateWindowEx(dwExStyle,ClassName,lpWindowName,dwStyle,x,y,nWidth,nHeight,hWndParent,hMenu,hInstanceWindow,this);
		if( !hWndCreated )
			appErrorf( TEXT("CreateWindowEx failed: %s"), appGetSystemErrorMessage() );
		check(hWndCreated);
		check(hWndCreated==hWnd);
		unguard;
	}
	void ScaleImageAndReplace(HBITMAP& hbm)
	{
		// stijn: do DPI scaling
		if (DPIX != 96 || DPIY != 96)
		{
			// fetch image dimensions
			BITMAP bm;
			GetObject(hbm, sizeof(bm), &bm);

			// scale up
			INT ScaledWidth = MulDiv(bm.bmWidth, DPIX, 96);
			INT ScaledHeight = MulDiv(bm.bmHeight, DPIY, 96);

			// copy and replace
			hbm = reinterpret_cast<HBITMAP>(CopyImage(hbm, IMAGE_BITMAP, ScaledWidth, ScaledHeight, LR_COPYDELETEORG));
		}
	}
	void SetRedraw( UBOOL Redraw )
	{
		guard(WWindow::SetRedraw);
		if (!m_bShow)
			return;
		SendMessageW( *this, WM_SETREDRAW, Redraw, 0 );
		if (Redraw)
			RedrawWindow(*this, NULL, NULL, RDW_UPDATENOW | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
		unguard;
	}
	// Used in the editor ... used to draw window edges in custom colors.
	virtual void MyDrawEdge( HDC hdc, LPRECT qrc, UBOOL bRaised )
	{
		guard(WWindow::MyDrawEdge);

		HDC	hDC = GetDC( hWnd );

		RECT rc = *qrc;
		rc.right -= 1;
		rc.bottom -= 1;

		HPEN penOld = (HPEN)SelectObject( hDC, (bRaised ? penRaised : penSunken ) );
		::MoveToEx( hDC, rc.left, rc.top, NULL );
		::LineTo( hDC, rc.right, rc.top );
		::MoveToEx( hDC, rc.left, rc.top, NULL );
		::LineTo( hDC, rc.left, rc.bottom);

		SelectObject( hDC, (bRaised ? penSunken : penRaised ) );
		::MoveToEx( hDC, rc.right, rc.bottom, NULL );
		::LineTo( hDC, rc.right, rc.top );
		::MoveToEx( hDC, rc.right, rc.bottom, NULL );
		::LineTo( hDC, rc.left, rc.bottom );
		SelectObject( hDC, penOld );

		::ReleaseDC( hWnd, hDC );

		unguard;
	}

	// Searches a list of filenames and replaces all single NULL's with | characters.  This allows
	// the regular parse routine to work correctly.  The return value is the number of NULL's
	// that were replaced -- if this is greater than zero, you have multiple filenames.
	//
	int FormatFilenames(TCHAR* _pchFilenames)
	{
		guard(WBrowser::FormatFilenames);
		TCHAR* pch = _pchFilenames;
		int l_iNULLs = 0;

		while (true)
		{
			if (*pch == L'\0')
			{
				if (*(pch + 1) == L'\0') break;

				*pch = L'|';
				l_iNULLs++;
			}
			pch++;
		}

		return l_iNULLs;
		unguard;
	}

	// Takes a delimited string and breaks it up into elements of a string array.
	void ParseStringToArray(const TCHAR* pchDelim, FString String, TArray<FString>* _pArray)
	{
		guard(ParseStringToArray);
		int i;
		FString S = String;

		i = S.InStr(pchDelim);

		while (i > 0)
		{
			new(*_pArray)FString(S.Left(i));
			S = S.Mid(i + 1, S.Len());
			i = S.InStr(pchDelim);
		}

		new(*_pArray)FString(S);
		unguard;
	}

	BOOL OpenFilesWithDialog(const TCHAR* InitialDir, const TCHAR* FileFilter, const TCHAR* FileExt, const TCHAR* FileDialogTitle, TArray<FString>& OutFilenames, BOOL MultiSelect = TRUE)
	{
		OPENFILENAMEW ofn;
		TCHAR Files[8192] = TEXT("\0");
		ZeroMemory(&ofn, sizeof(OPENFILENAMEW));
		ofn.lStructSize = sizeof(OPENFILENAMEW);
		ofn.hwndOwner = hWnd;
		ofn.lpstrFile = Files;
		ofn.nMaxFile = ARRAY_COUNT(Files);
		ofn.lpstrFilter = FileFilter;
		ofn.lpstrInitialDir = InitialDir;
		ofn.lpstrDefExt = FileExt;
		ofn.lpstrTitle = FileDialogTitle;
		ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_EXPLORER | (MultiSelect ? OFN_ALLOWMULTISELECT : 0);

		OutFilenames.Empty();
		if (GetOpenFileNameW(&ofn))
		{
			if (MultiSelect)
			{
				int iNULLs = FormatFilenames(Files);
				TArray<FString> StringArray;
				ParseStringToArray(TEXT("|"), Files, &StringArray);
				int iStart = 0;
				FString Prefix = TEXT("\0");

				if (iNULLs)
				{
					iStart = 1;
					Prefix = *(StringArray(0));
					Prefix += TEXT("\\");
				}

				for (INT i = iStart; i < StringArray.Num(); ++i)
				{
					OutFilenames.AddItem(Prefix + *StringArray(i));
				}
			}
			else
			{
				OutFilenames.AddItem(Files);
			}			
			return TRUE;
		}
		return FALSE;
	}

	BOOL GetSaveNameWithDialog(const TCHAR* InitialName, const TCHAR* InitialDir, const TCHAR* FileFilter, const TCHAR* FileExt, const TCHAR* FileDialogTitle, FString& OutFile)
	{
		OPENFILENAMEW ofn;
		TCHAR File[8192] = TEXT("\0");
		wcscpy_s(File, ARRAY_COUNT(File), InitialName);
		ZeroMemory(&ofn, sizeof(OPENFILENAMEW));
		ofn.lStructSize = sizeof(OPENFILENAMEW);
		ofn.hwndOwner = hWnd;
		ofn.lpstrFile = File;
		ofn.nMaxFile = ARRAY_COUNT(File);
		ofn.lpstrFilter = FileFilter;
		ofn.lpstrDefExt = FileExt;
		ofn.lpstrTitle = FileDialogTitle;
		ofn.lpstrInitialDir = InitialDir;
		ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;
		if (GetSaveFileNameW(&ofn))
		{
			OutFile = File;
			return TRUE;
		}
		return FALSE;
	}
};

/*-----------------------------------------------------------------------------
	WControl.
-----------------------------------------------------------------------------*/

// A control which exists inside an owner window.
class WINDOW_API WControl : public WWindow
{
	W_DECLARE_ABSTRACT_CLASS(WControl,WWindow,CLASS_Transient);

	// Variables.
	WNDPROC WindowDefWndProc{};

	// Structors.
	WControl() = default;

	WControl( WWindow* InOwnerWindow, INT InId, WNDPROC InSuperProc )
	: WWindow( NAME_None, InOwnerWindow )
	{
		check(OwnerWindow);
		WindowDefWndProc = InSuperProc;
		ControlId = InId ? InId : InOwnerWindow->TopControlId++;
		OwnerWindow->Controls.AddItem( this );
	}
#if WIN_OBJ
	void Destroy()
	{
		Super::Destroy();
		check(OwnerWindow);
		OwnerWindow->Controls.RemoveItem( this );
	}
#else
	~WControl()
	{
		// stijn: TODO/FIXME: This check should be removed once we figure out why OwnerWindow is ever null to begin with...
		if (OwnerWindow)
			OwnerWindow->Controls.RemoveItem( this );
	}
#endif

	// WWindow interface.
	virtual void OnDestroy()
	{
		guard(WControl::OnDestroy);
		if (OwnerWindow)
			OwnerWindow->Controls.RemoveItem( this );
		OwnerWindow = NULL;
		WWindow::OnDestroy();
		unguard;
	}
	virtual LRESULT CallDefaultProc( UINT Message, WPARAM wParam, LPARAM lParam )
	{
		if (WindowDefWndProc)
			return CallWindowProcW( WindowDefWndProc, hWnd, Message, wParam, lParam );
		return 0;
	}
	
	static WNDPROC RegisterWindowClass( const TCHAR* Name, const TCHAR* WinBaseClass )
	{
		guard(WControl::RegisterWindowClass);
		WNDPROC SuperProc=NULL;
		WNDCLASSEXW Cls;
		appMemzero( &Cls, sizeof(Cls) );
		Cls.cbSize        = sizeof(Cls);
		verifyError( GetClassInfoExW( NULL, WinBaseClass, &Cls ) );
		SuperProc         = Cls.lpfnWndProc;
		Cls.lpfnWndProc   = WWindow::StaticWndProc;
		Cls.lpszClassName = Name;
		Cls.hInstance     = hInstanceWindow;
		check(Cls.lpszMenuName==NULL);
		verify(RegisterClassExW( &Cls ));
		return SuperProc;
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	WTabControl.
-----------------------------------------------------------------------------*/

class WINDOW_API WTabControl : public WControl
{
	W_DECLARE_CLASS(WTabControl,WControl,CLASS_Transient)
	DECLARE_WINDOWSUBCLASS(WTabControl,WControl,Window)

	FDelegate SelectionChangeDelegate;

	// Constructor.
	WTabControl() = default;

	WTabControl( WWindow* InOwner, INT InId=0, WNDPROC InSuperProc=NULL )
	: WControl( InOwner, InId, InSuperProc?InSuperProc:SuperProc )
	{}

	// WWindow interface.
	void OpenWindow( UBOOL Visible, DWORD dwExtraStyle = 0 )
	{
		guard(WTabControl::OpenWindow);
		PerformCreateWindowEx
		(
			WS_EX_COMPOSITED,
            NULL,
            WS_CHILD | (Visible?WS_VISIBLE:0) | dwExtraStyle,
            0, 0,
			0, 0,
            *OwnerWindow,
            (HMENU)ControlId,
            hInstance
		);

		SendMessageW( *this, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(0,0) );
		unguard;
	}
	void AddTab( FString InText, int InlParam )
	{
		guard(WTabControl::AddTab);

		FString Text = InText;

		TCITEM tci;

		tci.mask = TCIF_TEXT | TCIF_PARAM;
		tci.pszText = &Text[0];
		tci.lParam = InlParam;

		SendMessageW( *this, TCM_INSERTITEM, 0, (LPARAM)(const LPTCITEM)&tci );

		unguard;
	}
	void Empty()
	{
		guard(WTabControl::Empty);
		SendMessageW( *this, TCM_DELETEALLITEMS, 0, 0 );
		unguard;
	}
	int GetCount()
	{
		guard(WTabControl::GetCount);
		return SendMessageW( *this, TCM_GETITEMCOUNT, 0, 0 );
		unguard;
	}
	int SetCurrent( int Index )
	{
		guard(WTabControl::SetCurrent);
		return SendMessageW( *this, TCM_SETCURSEL, Index, 0 );
		unguard;
	}
	int GetCurrent()
	{
		guard(WTabControl::GetCurrent);
		return SendMessageW( *this, TCM_GETCURSEL, 0, 0 );
		unguard;
	}
	int GetIndexFromlParam( int InlParam )
	{
		guard(WTabControl::GetIndexFromlParam);

		TCITEM tci;
		tci.mask = TCIF_PARAM;

		for( int x = 0 ; x < GetCount() ; x++ )
		{
			SendMessageW( *this, TCM_GETITEM, x, (LPARAM)(LPTCITEM)&tci );
			if( tci.lParam == InlParam )
				return x;
		}

		return -1;
		unguard;
	}
	FString GetString( int Index )
	{
		guard(WTabControl::GetString);

		TCHAR Buffer[256] = TEXT("");

		TCITEM tci;
		tci.mask = TCIF_TEXT;
		tci.pszText = Buffer;

		SendMessageW( *this, TCM_GETITEM, Index, (LPARAM)(LPTCITEM)&tci );

		return FString( Buffer );
		unguard;
	}
	int GetlParam( int Index )
	{
		guard(WTabControl::GetlParam);

		TCITEM tci;
		tci.mask = TCIF_PARAM;

		SendMessageW( *this, TCM_GETITEM, Index, (LPARAM)(LPTCITEM)&tci );

		return tci.lParam;
		unguard;
	}
	UBOOL InterceptControlCommand( UINT Message, WPARAM wParam, LPARAM lParam )
	{
		guard(WTabControl::InterceptControlCommand);

		LastwParam = wParam;
		LastlParam = lParam;

		NMHDR* pnmhdr = (LPNMHDR)lParam;

		if( pnmhdr->code == TCN_SELCHANGE)
			SelectionChangeDelegate();

		return 0;
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	WLabel.
-----------------------------------------------------------------------------*/

// A non-interactive label control.
class WINDOW_API WLabel : public WControl
{
	W_DECLARE_CLASS(WLabel,WControl,CLASS_Transient)
	DECLARE_WINDOWSUBCLASS(WLabel,WControl,Window)

	// Constructor.
	WLabel() = default;

	WLabel( WWindow* InOwner, INT InId=0, WNDPROC InSuperProc=NULL )
	: WControl( InOwner, InId, InSuperProc?InSuperProc:SuperProc )
	{}

	// WWindow interface.
	void OpenWindow( UBOOL Visible, UBOOL ClientEdge = 1, DWORD dwExtraStyle = 0 )
	{
		guard(WLabel::OpenWindow);
		PerformCreateWindowEx
		(
			(ClientEdge?WS_EX_CLIENTEDGE:0),
            NULL,
            WS_CHILD | (Visible?WS_VISIBLE:0) | dwExtraStyle,
            0, 0,
			0, 0,
            *OwnerWindow,
            (HMENU)ControlId,
            hInstance
		);

		SendMessageW( *this, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(0,0) );
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	WCustomLabel.
-----------------------------------------------------------------------------*/

class WINDOW_API WCustomLabel : public WControl
{
	W_DECLARE_CLASS(WCustomLabel,WControl,CLASS_Transient)
	DECLARE_WINDOWSUBCLASS(WCustomLabel,WControl,Window)

	// Constructor.
	WCustomLabel() = default;

	WCustomLabel( WWindow* InOwner, INT InId=0, WNDPROC InSuperProc=NULL )
	: WControl( InOwner, InId, InSuperProc?InSuperProc:SuperProc )
	{}

	// WWindow interface.
	void OpenWindow( UBOOL Visible, UBOOL ClientEdge = 1, DWORD dwExtraStyle = 0 )
	{
		guard(WCustomLabel::OpenWindow);
		PerformCreateWindowEx
		(
			(ClientEdge?WS_EX_CLIENTEDGE:0),
            NULL,
            WS_CHILD | (Visible?WS_VISIBLE:0) | dwExtraStyle,
            0, 0,
			0, 0,
            *OwnerWindow,
            (HMENU)ControlId,
            hInstance
		);

		SendMessageW( *this, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(0,0) );
		unguard;
	}
	void OnPaint()
	{
		guard(WCustomLabel::OnPaint);
		PAINTSTRUCT PS;
		HDC hDC = BeginPaint( *this, &PS );
		HBRUSH brushBack = CreateSolidBrush( RGB(128,128,128) );

		FRect Rect = GetClientRect();
		FillRect( hDC, Rect, brushBack );

		FString Caption = GetText();
		::SetBkMode( hDC, TRANSPARENT );
		::DrawTextW(hDC, *Caption, Caption.Len(), Rect, DT_CENTER | DT_SINGLELINE);

		EndPaint( *this, &PS );

		DeleteObject( brushBack );
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	WButton.
-----------------------------------------------------------------------------*/

// A button.
class WINDOW_API WButton : public WControl
{
	W_DECLARE_CLASS(WButton,WControl,CLASS_Transient);
	DECLARE_WINDOWSUBCLASS(WButton,WControl,Window)

	// Delegates.
	FDelegate ClickDelegate;
	FDelegate DoubleClickDelegate;
	FDelegate PushDelegate;
	FDelegate UnPushDelegate;
	FDelegate SetFocusDelegate;
	FDelegate KillFocusDelegate;
	HBITMAP hbm{};
	UBOOL bChecked{}, bOwnerDraw{};

	// Constructor.
	WButton() = default;

	WButton( WWindow* InOwner, INT InId=0, FDelegate InClicked=FDelegate(), WNDPROC InSuperProc=NULL )
	: WControl( InOwner, InId, InSuperProc?InSuperProc:SuperProc )
	, ClickDelegate( InClicked )
	{
		bChecked = 0;
	}

	// WWindow interface.
	void OpenWindow( UBOOL Visible, INT X, INT Y, INT XL, INT YL, const TCHAR* Text, UBOOL bBitmap = 0, DWORD dwExtraStyle = 0 )
	{
		guard(WButton::OpenWindow);
		PerformCreateWindowEx
		(
			0,
            NULL,
            WS_CHILD | BS_PUSHBUTTON | (bBitmap?BS_BITMAP:0) | dwExtraStyle,
            X, Y,
			XL, YL,
            *OwnerWindow,
            (HMENU)ControlId,
            hInstance
		);

		SendMessageW( *this, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(0,0) );
		SetText( Text );
		if( Visible )
			ShowWindow( *this, SW_SHOWNOACTIVATE );
		bOwnerDraw = (dwExtraStyle & BS_OWNERDRAW);

		HBRUSH hNew = CreateSolidBrush( RGB(128,128,128) );                                                                          
		HBRUSH hOld = (HBRUSH) SetClassLongPtr(hWnd, GCLP_HBRBACKGROUND, (LONG_PTR) hNew);                                           
		if (hOld)
			DeleteBrush(hOld);
		unguard;
	}
	void SetVisibleText( const TCHAR* Text )
	{
		guard(WButton::SetVisibleText);
		check(hWnd);
		if( Text )
			SetText( Text );
		Show( Text!=NULL );
		unguard;
	}
	void SetBitmap( HBITMAP Inhbm )
	{
		guard(WButton::SetBitmap);
		SendMessageW( hWnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)Inhbm);
		hbm = Inhbm;
		unguard;
	}
	virtual void Clicked(void)
	{
		guard(WButton::Clicked);
		SendMessageW( OwnerWindow->hWnd, WM_COMMAND, WM_PB_PUSH, ControlId );
		unguard;
	}
	void OnDrawItem( DRAWITEMSTRUCT* Item )
	{
		guard(WButton::OnDrawItem);
		RECT R=Item->rcItem;
		UBOOL bPressed = (Item->itemState&ODS_SELECTED)!=0 || bChecked;

		FillRect( Item->hDC, &Item->rcItem, hBrushGrey );

		MyDrawEdge( Item->hDC, &R, !bPressed );

		HDC hdcMem;
		HBITMAP hbmOld;

		hdcMem = CreateCompatibleDC(Item->hDC);
		hbmOld = (HBITMAP)SelectObject(hdcMem, hbm);
		BITMAP bitmap;
		GetObjectA( hbm, sizeof(BITMAP), (LPSTR)&bitmap );

		int Adjust = (bPressed) ? 2 : 0;

		BitBlt(Item->hDC,
			(R.right / 2) - (bitmap.bmWidth / 2) + Adjust, (R.bottom / 2) - (bitmap.bmHeight / 2) + Adjust,
			bitmap.bmWidth, bitmap.bmHeight,
			hdcMem,
			0, 0,
			SRCCOPY);

		// If this button is checked, draw a light border around in the inside to make it obvious.
		if( bPressed )
		{
			HPEN penOld, pen = CreatePen( PS_SOLID, 1, RGB(145,210,198) );

			RECT rc = R;
			rc.left++;	rc.top++;	rc.right--;		rc.bottom--;

			penOld = (HPEN)SelectObject( Item->hDC, pen );
			::MoveToEx( Item->hDC, rc.left, rc.top, NULL );
			::LineTo( Item->hDC, rc.right, rc.top );
			::LineTo( Item->hDC, rc.right, rc.bottom );
			::LineTo( Item->hDC, rc.left, rc.bottom );
			::LineTo( Item->hDC, rc.left, rc.top );
			SelectObject( Item->hDC, penOld );

			DeleteObject( pen );
		}

		// Clean up.
		//
		SelectObject(hdcMem, hbmOld);
		DeleteDC(hdcMem);

		unguard;
	}

	// WControl interface.
	UBOOL InterceptControlCommand( UINT Message, WPARAM wParam, LPARAM lParam )
	{
		guard(WButton::InterceptControlCommand);
		if     ( HIWORD(wParam)==BN_CLICKED   )
		{
			Clicked();
			ClickDelegate();
			// This notification returns 0 instead of 1 because the editor sometimes wants
			// to know about clicks without using a delegate (i.e. dynamically created buttons)
			return 0;
		}
		else if( HIWORD(wParam)==BN_DBLCLK    ) {DoubleClickDelegate(); return 1;}
		else if( HIWORD(wParam)==BN_PUSHED    ) {PushDelegate();        return 1;}
		else if( HIWORD(wParam)==BN_UNPUSHED  ) {UnPushDelegate();      return 1;}
		else if( HIWORD(wParam)==BN_SETFOCUS  ) {SetFocusDelegate();    return 1;}
		else if( HIWORD(wParam)==BN_KILLFOCUS ) {UnPushDelegate();      return 1;}
		else return 0;
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	WToolTip
-----------------------------------------------------------------------------*/

// Tooltip window - easy way to create tooltips for standard controls.
class WINDOW_API WToolTip : public WControl
{
	W_DECLARE_CLASS(WToolTip,WControl,CLASS_Transient);
	DECLARE_WINDOWSUBCLASS(WToolTip,WControl,Window)

	// Constructor.
	WToolTip() = default;

	WToolTip( WWindow* InOwner, INT InId=900, WNDPROC InSuperProc=NULL )
	: WControl( InOwner, InId, InSuperProc?InSuperProc:SuperProc )
	{
		check(OwnerWindow);
	}

	// WWindow interface.
	void OpenWindow()
	{
		guard(WToolTip::OpenWindow);
		hWnd = CreateWindow(TOOLTIPS_CLASS, NULL, TTS_ALWAYSTIP | WS_POPUP,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			OwnerWindow->hWnd, (HMENU)NULL, hInstance, NULL);
		check(hWnd);
		unguard;
	}
	void AddTool( HWND InHwnd, FString InToolTip, int InId, RECT* InRect = NULL )
	{
		guard(WToolTip::AddTool);

		check(::IsWindow( hWnd ));

		TOOLINFOW ti = {0};
		FString TmpToolTip = InToolTip;
		ti.cbSize = TTTOOLINFO_V1_SIZE;		
		ti.uFlags = TTF_SUBCLASS;
		ti.hwnd = InHwnd;
		ti.hinst = hInstance;
		ti.uId = InId;		
		ti.lpszText = &TmpToolTip[0];
		if (InRect)
			ti.rect = *InRect;
		else
			::GetClientRect(InHwnd, &ti.rect);

		verify(SendMessageW(hWnd, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)& ti));	 	
		unguard;
	}
	void UpdateToolTip(HWND InHwnd, FString InToolTip, int InId)
	{
		guard(WToolTip::UpdateToolTip);
		TOOLINFOW ti = {0};
		ti.lpszText = (LPWSTR)*InToolTip;
		ti.hinst = hInstance;
		ti.hwnd = InHwnd;
		ti.uId = InId;
		ti.cbSize = TTTOOLINFO_V1_SIZE;
		SendMessageW(hWnd, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	WPictureButton.
-----------------------------------------------------------------------------*/
class WPictureButton : public WWindow
{
	DECLARE_WINDOWCLASS(WPictureButton,WWindow,Window)

	HBITMAP hbmOn, hbmOff;
	RECT ClientPos, BmpOffPos, BmpOnPos;
	FString ToolTip;
	int ID;
	HWND hWndTT;
	UBOOL bOn, bHasBeenSetup;

	// Structors.
	WPictureButton( WWindow* InOwnerWindow )
		: WWindow(TEXT("PictureButton"), InOwnerWindow)
		, ClientPos()
		, BmpOffPos()
		, BmpOnPos()
		, ID(0)
	{
		hbmOn = hbmOff = NULL;
		hWndTT = NULL;
		bOn = bHasBeenSetup = 0;
	}

	// WWindow interface.
	void OpenWindow()
	{
		guard(WPictureButton::OpenWindow);
		MdiChild = 0;

		// HAVE to call "SetUp" before opening the window
		check(bHasBeenSetup);

		PerformCreateWindowEx
		(
			0,
			NULL,
			WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
			ClientPos.left,
			ClientPos.top,
			ClientPos.right,
			ClientPos.bottom,
			OwnerWindow ? OwnerWindow->hWnd : NULL,
			NULL,
			hInstance
		);
		SendMessageW( hWnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(0,0) );

		hWndTT = CreateWindowA(TOOLTIPS_CLASSA, NULL, TTS_ALWAYSTIP,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			NULL, (HMENU) NULL, hInstance, NULL);
		check(hWndTT);

		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WPictureButton::OnSize);
		WWindow::OnSize(Flags, NewX, NewY);

		RECT rect;
		::GetClientRect( hWnd, &rect );

		if( ToolTip.Len() )
		{
			TOOLINFO ti;
			FString TmpToolTip = ToolTip;
			ti.cbSize = TTTOOLINFO_V1_SIZE;
			ti.uFlags = TTF_SUBCLASS;
			ti.hwnd = hWnd;
			ti.hinst = hInstance;
			ti.uId = 900;
			ti.lpszText = &TmpToolTip[0];
			ti.rect = rect;

			SendMessageW(hWndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO)&ti);
		}

		unguard;
	}
	void OnDestroy()
	{
		guard(WPictureButton::OnDestroy);
		WWindow::OnDestroy();
		DestroyWindow( hWndTT );
		unguard;
	}
	void OnCreate()
	{
		guard(WPictureButton::OnCreate);
		WWindow::OnCreate();
		unguard;
	}
	void OnPaint()
	{
		guard(WPictureButton::OnPaint);
		PAINTSTRUCT PS;
		HDC hDC = BeginPaint( *this, &PS );
		HBRUSH brushBack = CreateSolidBrush( RGB(255,255,255) );

		RECT rc;
		::GetClientRect( hWnd, &rc );

		FillRect( hDC, &rc, brushBack );

		HDC hdcMem;
		HBITMAP hbmOld;

		hdcMem = CreateCompatibleDC(hDC);
		hbmOld = (HBITMAP)SelectObject(hdcMem, (bOn ? hbmOn : hbmOff));

		RECT BmpPos = (bOn ? BmpOnPos : BmpOffPos);

		BitBlt(hDC,
			(rc.right - BmpPos.right) / 2, (rc.bottom - BmpPos.bottom) / 2,
			BmpPos.right, BmpPos.bottom,
			hdcMem,
			BmpPos.left, BmpPos.top,
			SRCCOPY);

		// Clean up.
		//
		SelectObject(hdcMem, hbmOld);
		DeleteDC(hdcMem);
		DeleteObject( brushBack );

		EndPaint( *this, &PS );
		unguard;
	}
	void OnLeftButtonDown(INT X, INT Y)
	{
		guard(WPictureButton::OnLeftButtonDown);
		SendMessageW( OwnerWindow->hWnd, WM_COMMAND, WM_PB_PUSH, ID );
		unguard;
	}
	void SetUp( FString InToolTip, int InID, 
		int InClientLeft, int InClientTop, int InClientRight, int InClientBottom,
		HBITMAP InHbmOff, int InBmpOffLeft, int InBmpOffTop, int InBmpOffRight, int InBmpOffBottom,
		HBITMAP InHbmOn, int InBmpOnLeft, int InBmpOnTop, int InBmpOnRight, int InBmpOnBottom )
	{
		guard(WPictureButton::SetUp);

		bHasBeenSetup = 1;

		ToolTip = InToolTip;
		ID = InID;
		ClientPos.left = InClientLeft;
		ClientPos.top = InClientTop;
		ClientPos.right = InClientRight;
		ClientPos.bottom = InClientBottom;
		hbmOff = InHbmOff;
		BmpOffPos.left = InBmpOffLeft;
		BmpOffPos.top = InBmpOffTop;
		BmpOffPos.right = InBmpOffRight;
		BmpOffPos.bottom = InBmpOffBottom;
		hbmOn = InHbmOn;
		BmpOnPos.left = InBmpOnLeft;
		BmpOnPos.top = InBmpOnTop;
		BmpOnPos.right = InBmpOnRight;
		BmpOnPos.bottom = InBmpOnBottom;

		unguard;
	}
};

/*-----------------------------------------------------------------------------
	WCheckBox.
-----------------------------------------------------------------------------*/

// A checkbox.
class WINDOW_API WCheckBox : public WButton
{
	W_DECLARE_CLASS(WCheckBox,WButton,CLASS_Transient);
	DECLARE_WINDOWSUBCLASS(WCheckBox,WButton,Window)

	UBOOL bAutocheck{};

	// Constructor.
	WCheckBox()	= default;

	WCheckBox( WWindow* InOwner, INT InId=0, FDelegate InClicked=FDelegate() )
	: WButton( InOwner, InId, InClicked )
	{}

	// WWindow interface.
	void OpenWindow( UBOOL Visible, INT X, INT Y, INT XL, INT YL, const TCHAR* Text, UBOOL InbAutocheck = 1, UBOOL bBitmap = 0, DWORD dwExtraStyle = 0 )
	{
		guard(WCheckBox::OpenWindow);
		bAutocheck = InbAutocheck;
		PerformCreateWindowEx
		(
			0,
            NULL,
			WS_CHILD | BS_CHECKBOX | (bAutocheck?BS_AUTOCHECKBOX:0) | (bBitmap?BS_BITMAP:0) | dwExtraStyle,
            X, Y,
			XL, YL,
            *OwnerWindow,
            (HMENU)ControlId,
            hInstance
		);
		SetText( Text );
		SendMessageW( *this, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(0,0) );
		if( Visible )
			ShowWindow( *this, SW_SHOWNOACTIVATE );
		bOwnerDraw = (dwExtraStyle & BS_OWNERDRAW);

		unguard;
	}
	void OnCreate()
	{
		guard(WCheckBox::OnCreate);
		WButton::OnCreate();
		bOwnerDraw = 0;
		unguard;
	}
	virtual void OnRightButtonDown(INT X, INT Y)
	{
		guard(WCheckBox::OnRightButtonDown);
		// This is just a quick hack so the parent window can know when the user right clicks on
		// one of the buttons in the UnrealEd button bar.
		SendMessageW( ::GetParent( hWnd ), WM_COMMAND, WM_BB_RCLICK, ControlId );
		unguard;
	}
	BOOL IsChecked( void )
	{
		guard(WCheckBox::IsChecked);

		// Owner draw buttons require us to keep track of the checked status ourselves.
		if( bOwnerDraw )
			return bChecked;
		else
			return ( SendMessageW( *this, BM_GETCHECK, 0, 0 ) == BST_CHECKED );
		unguard;
	}
	void SetCheck( int iCheck )
	{
		guard(WCheckBox::SetCheck);
		SendMessageW( *this, BM_SETCHECK, (WPARAM)iCheck, 0 );
		bChecked = (iCheck == BST_CHECKED);
		unguard;
	}
	virtual void Clicked(void)
	{
		guard(WCheckBox::Clicked);
		if( bAutocheck )
			bChecked = !bChecked;
		else
			bChecked = FALSE;
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	WVScrollBar.
-----------------------------------------------------------------------------*/

// A vertical scrollbar.
class WINDOW_API WVScrollBar : public WControl
{
	W_DECLARE_CLASS(WVScrollBar,WControl,CLASS_Transient);
	DECLARE_WINDOWSUBCLASS(WVScrollBar,WControl,Window)

	// Constructor.
	WVScrollBar() = default;

	WVScrollBar( WWindow* InOwner, INT InId=0, WNDPROC InSuperProc=NULL  )
	: WControl( InOwner, InId, InSuperProc?InSuperProc:SuperProc )
	{}

	// WWindow interface.
	void OpenWindow( UBOOL Visible, INT X, INT Y, INT XL, INT YL )
	{
		guard(WVScrollBar::OpenWindow);
		PerformCreateWindowEx
		(
			0,
            NULL,
            WS_CHILD | SBS_RIGHTALIGN | SBS_VERT,
            X, Y,
			XL, YL,
            *OwnerWindow,
            (HMENU)ControlId,
            hInstance
		);
		SendMessageW( *this, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(0,0) );
		if( Visible )
			ShowWindow( *this, SW_SHOWNOACTIVATE );
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	WTreeView.
-----------------------------------------------------------------------------*/

// A tree control
class WINDOW_API WTreeView : public WControl
{
	W_DECLARE_CLASS(WTreeView,WControl,CLASS_Transient);
	DECLARE_WINDOWSUBCLASS(WTreeView,WControl,Window)

	// Delegates.
	FDelegate ItemExpandingDelegate;
	FDelegate SelChangedDelegate;
	FDelegate DblClkDelegate;

	// Constructor.
	WTreeView()	= default;

	WTreeView( WWindow* InOwner, INT InId=0, WNDPROC InSuperProc=NULL  )
	: WControl( InOwner, InId, InSuperProc?InSuperProc:SuperProc )
	{}

	// WWindow interface.
	void OpenWindow( UBOOL Visible, INT X, INT Y, INT XL, INT YL )
	{
		guard(WTreeView::OpenWindow);
		PerformCreateWindowEx
		(
			WS_EX_CLIENTEDGE,
            NULL,
            WS_CHILD | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_SHOWSELALWAYS | TVS_DISABLEDRAGDROP,
            X, Y,
			XL, YL,
            *OwnerWindow,
            (HMENU)ControlId,
            hInstance
		);
		SendMessageW( *this, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(0,0) );
		if( Visible )
			ShowWindow( *this, SW_SHOWNOACTIVATE );
		unguard;
	}
	void Empty( void )
	{
		guard(WTreeView::Empty);
		SendMessageW( *this, TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT );
		unguard;
	}
	HTREEITEM AddItem( const TCHAR* pName, HTREEITEM hti, BOOL bHasChildren )
	{
		guard(WTreeView::AddItem);

		TV_INSERTSTRUCT	InsertInfo;

		InsertInfo.hParent = hti;
		InsertInfo.hInsertAfter = TVI_SORT;

		InsertInfo.item.mask = TVIF_TEXT;
		InsertInfo.item.pszText = const_cast<TCHAR*>(pName); // stijn: for historic reasons, the constness needs to be stripped off, even though Windows will not modify the value

		if( bHasChildren )
		{
			InsertInfo.item.mask |= TVIF_CHILDREN;
			InsertInfo.item.cChildren = 1;
		}

		return (HTREEITEM)SendMessageW( *this, TVM_INSERTITEM, 0, (LPARAM) &InsertInfo );

		unguard;
	}
	HTREEITEM FindItem(HWND hWnd, HTREEITEM hItemParent, LPCWSTR FindName)
	{
		guard(WTreeView::FindItem);
		TVITEM Tvi;
		TCHAR ItemName[512];

		for (Tvi.hItem = TreeView_GetChild(hWnd, hItemParent); Tvi.hItem; Tvi.hItem = TreeView_GetNextSibling(hWnd, Tvi.hItem))
		{
			Tvi.mask = TVIF_TEXT | TVIF_CHILDREN;
			Tvi.pszText = ItemName;
			Tvi.cchTextMax = _countof(ItemName);
			if (TreeView_GetItem(hWnd, &Tvi))
			{
				if (appStricmp(Tvi.pszText, FindName) == 0)
					return Tvi.hItem;
				if (Tvi.cChildren)
				{
					HTREEITEM SearchChild = FindItem(hWnd, Tvi.hItem, FindName);
					if (SearchChild)
						return SearchChild;
				}
			}
		}
		return 0;
		unguard;
	}
	virtual void OnRightButtonDown(INT X, INT Y)
	{
		guard(WTreeView::OnRightButtonDown);
		PostMessageW( OwnerWindow->hWnd, WM_COMMAND, WM_TREEVIEW_RIGHT_CLICK, 0 );
		unguard;
	}
	UBOOL InterceptControlCommand( UINT Message, WPARAM wParam, LPARAM lParam )
	{
		guard(WTreeView::InterceptControlCommand);

		LastwParam = wParam;
		LastlParam = lParam;

		NMTREEVIEW* pntv = (LPNMTREEVIEW)lParam;

		if( pntv->hdr.code==TVN_SELCHANGEDA
				|| pntv->hdr.code==TVN_SELCHANGEDW )
			SelChangedDelegate();
		if( pntv->hdr.code==TVN_ITEMEXPANDINGA
				|| pntv->hdr.code==TVN_ITEMEXPANDINGW )
			ItemExpandingDelegate();
		if( pntv->hdr.code==NM_DBLCLK )
		{
			DblClkDelegate();
			return 1;
		}

		return 0;
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	WCoolButton.
-----------------------------------------------------------------------------*/

// Frame showing styles.
enum EFrameFlags
{
	CBFF_ShowOver	= 0x01,
	CBFF_ShowAway	= 0x02,
	CBFF_DimAway    = 0x04,
	CBFF_UrlStyle	= 0x08,
	CBFF_NoCenter   = 0x10
};

// A coolbar-style button.
class WINDOW_API WCoolButton : public WButton
{
	W_DECLARE_CLASS(WCoolButton,WButton,CLASS_Transient);
	DECLARE_WINDOWSUBCLASS(WCoolButton,WButton,Window)

	// Variables.
	static WCoolButton* GlobalCoolButton;
	HICON hIcon{};
	DWORD FrameFlags{};

	// Constructor.
	WCoolButton() = default;

	WCoolButton( WWindow* InOwner, INT InId=0, FDelegate InClicked=FDelegate(), DWORD InFlags=CBFF_ShowOver|CBFF_DimAway )
	: WButton( InOwner, InId, InClicked )
	, hIcon( NULL )
	, FrameFlags( InFlags )
	{}

	// WWindow interface.
	void OpenWindow( UBOOL Visible, INT X, INT Y, INT XL, INT YL, const TCHAR* Text )
	{
		guard(WCoolButton::OpenWindow);
#ifndef JAPANESE
		PerformCreateWindowEx
		(
			0,
            NULL,
            WS_CHILD | BS_OWNERDRAW,
            X, Y,
			XL, YL,
            *OwnerWindow,
            (HMENU)ControlId,
            hInstance
		);
#else
		PerformCreateWindowEx
		(
			0,
            TEXT("BUTTON"),
            WS_CHILD | BS_PUSHBUTTON | WS_VISIBLE,
            X, Y,
			XL, YL,
            *OwnerWindow,
            (HMENU)ControlId,
            hInstance
		);
#endif
		SetText( Text );			

		SendMessageW( *this, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(0,0) );
		if (Visible)
		{
			ShowWindow(*this, SW_SHOWNOACTIVATE);
			m_bShow = TRUE;
		}
		unguard;
	}
	void OnDestroy()
	{
		if( GlobalCoolButton==this )
			GlobalCoolButton=NULL;
		WButton::OnDestroy();
	}
#ifndef JAPANESE
	void OnCreate()
	{
		WButton::OnCreate();
		SetClassLongX( *this, GCL_STYLE, GetClassLongX(*this,GCL_STYLE) & ~CS_DBLCLKS);
	}

	// WCoolButton interface.
	void UpdateHighlight( UBOOL TurnOff )
	{
		guard(WCoolButton::UpdateHighlight);
		FPoint P;
		FRect R;
		::GetCursorPos((POINT*)&P);
		::GetWindowRect(*this,(RECT*)&R);
		UBOOL ShouldHighlight = (R.Contains(P) || GetCapture()==hWnd) && !TurnOff;
		if( (GlobalCoolButton==this) != (ShouldHighlight) )
		{
			if( GlobalCoolButton==this )
			{
				GlobalCoolButton = NULL;
				KillTimer( hWnd, 0 );
			}
			else
			{
				if( GlobalCoolButton )
					GlobalCoolButton->UpdateHighlight( 1 );
				GlobalCoolButton = this;
				SetTimer( hWnd, 0, 50, NULL );
			}
			InvalidateRect( *this, NULL, 1 );
			UpdateWindow( *this );
		}
		unguard;
	}
	void OnTimer()
	{
		guard(WCoolButton::OnTimer);
		UpdateHighlight(0);
		unguard;
	}
	INT OnSetCursor()
	{
		guard(WUrlButton::OnSetCursor);
		WButton::OnSetCursor();
		UpdateHighlight(0);
		if( FrameFlags & CBFF_UrlStyle )
			SetCursor(LoadCursorIdX(hInstanceWindow,IDC_Hand));
		return 1;
		unguard;
	}

	// WWindow interface.
	void OnDrawItem( DRAWITEMSTRUCT* Item )
	{
		guard(WCoolButton::OnDrawItem);
		RECT R=Item->rcItem;
		UBOOL Pressed = (Item->itemState&ODS_SELECTED)!=0;
		FillRect( Item->hDC, &Item->rcItem, (HBRUSH)(COLOR_BTNFACE+1));
		if( GlobalCoolButton==this )
		{
			if( FrameFlags & CBFF_ShowOver )
				DrawEdge( Item->hDC, &Item->rcItem, Pressed?EDGE_SUNKEN:EDGE_RAISED, BF_RECT );
		}
		else
		{
			if( FrameFlags & CBFF_DimAway )
				DrawEdge( Item->hDC, &Item->rcItem, Pressed?BDR_SUNKENINNER:BDR_RAISEDINNER, BF_RECT );
			else if( FrameFlags & CBFF_ShowAway )
				DrawEdge( Item->hDC, &Item->rcItem, Pressed?EDGE_SUNKEN:EDGE_RAISED, BF_RECT );
		}
		R.left += Pressed;
		R.right += Pressed;
		R.top += Pressed;
		R.bottom += Pressed;
		if( hIcon )
		{
			ICONINFO II;
			GetIconInfo( hIcon, &II );
			DrawIcon( Item->hDC, R.left, R.top + (R.bottom-R.top)/2-II.yHotspot, hIcon );
			R.left += II.xHotspot * 2;
		}
		FString Text = GetText();
		DWORD TextFlags
		=	DT_END_ELLIPSIS
		|	DT_VCENTER
		|	DT_SINGLELINE
		|	((FrameFlags & CBFF_NoCenter) ? 0 : DT_CENTER);
		HFONT OldFont;
		if( FrameFlags & CBFF_UrlStyle )
		{
			R.left += 8;
			SetTextColor( Item->hDC, RGB(0,0,255) );
			OldFont = SelectObject( Item->hDC, hFontUrl );
		}
		else
			OldFont = SelectObject( Item->hDC, hFontText );
		SetBkMode( Item->hDC, TRANSPARENT );
		DrawTextX( Item->hDC, *Text, Text.Len(), &R, TextFlags );
		SelectObject( Item->hDC, OldFont );
		unguard;
	}
#endif
};

/*-----------------------------------------------------------------------------
	WUrlButton.
-----------------------------------------------------------------------------*/

// A URL button.
class WINDOW_API WUrlButton : public WCoolButton
{
	W_DECLARE_CLASS(WUrlButton,WCoolButton,CLASS_Transient);
	DECLARE_WINDOWSUBCLASS(WUrlButton,WCoolButton,Window)

	// Variables.
	FString URL;

	// Constructor.
	WUrlButton() = default;

	WUrlButton( WWindow* InOwner, const TCHAR* InURL, INT InId=0 )
	: WCoolButton( InOwner, InId, FDelegate(this,(TDelegate)&WUrlButton::OnClick) )
	, URL( InURL )
	{
		FrameFlags = CBFF_ShowOver | CBFF_UrlStyle | CBFF_NoCenter;
	}

	// WUrlButton interface.
	void OnClick()
	{
		guard(WUrlButton::OnClick);
		ShellExecuteW( GetActiveWindow(), TEXT("open"), *URL, TEXT(""), TEXT(""), SW_SHOWNORMAL );
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	WComboBox.
-----------------------------------------------------------------------------*/

// A combo box control.
class WINDOW_API WComboBox : public WControl
{
	W_DECLARE_CLASS(WComboBox,WControl,CLASS_Transient);
	DECLARE_WINDOWSUBCLASS(WComboBox,WControl,Window)

	// Delegates.
	FDelegate DoubleClickDelegate;
	FDelegate DropDownDelegate;
	FDelegate CloseComboDelegate;
	FDelegate EditChangeDelegate;
	FDelegate EditUpdateDelegate;
	FDelegate SetFocusDelegate;
	FDelegate KillFocusDelegate;
	FDelegate SelectionChangeDelegate;
	FDelegate SelectionEndOkDelegate;
	FDelegate SelectionEndCancelDelegate;
 
	// Constructor.
	WComboBox()	= default;

	WComboBox( WWindow* InOwner, INT InId=0, WNDPROC InSuperProc=NULL )
	: WControl( InOwner, InId, InSuperProc?InSuperProc:SuperProc )
	{}

	// WWindow interface.
	void OpenWindow( UBOOL Visible, UBOOL Sort = FALSE, UINT _iListType = CBS_DROPDOWNLIST )
	{
		guard(WComboBox::OpenWindow);
		PerformCreateWindowEx
		(
			0,
            NULL,
            WS_CHILD | WS_VSCROLL | CBS_AUTOHSCROLL | _iListType | (Sort?CBS_SORT:0) | (Visible?WS_VISIBLE:0),
            0, 0,
			MulDiv(64, DPIX, 96), 
			MulDiv(384, DPIY, 96),
            *OwnerWindow,
            (HMENU)ControlId,
            hInstance
		);

		if (Visible)
			m_bShow = TRUE;

		SendMessageW( *this, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(0,0) );
		unguard;
	}
	virtual LRESULT WndProc( UINT Message, WPARAM wParam, LPARAM lParam )
	{
		if( Message==WM_KEYDOWN && (wParam==VK_UP || wParam==VK_DOWN) )
		{
			// Suppress arrow keys.
			if( Snoop )
				Snoop->SnoopKeyDown( this, wParam );
			return 1;
		}
		else return WControl::WndProc( Message, wParam, lParam );
	}

	// WControl interface.
	UBOOL InterceptControlCommand( UINT Message, WPARAM wParam, LPARAM lParam)
	{
		guard(WComboBox::InterceptControlCommand);

		// Do not delegate commands to controls we are about to delete since they might
		// hold and use invalid data such as stale pointers
		if (_DeleteWindows.FindItemIndex(this) != INDEX_NONE)
			return 0;

		if     ( HIWORD(wParam)==CBN_DBLCLK         ) {DoubleClickDelegate();        return 1;}
		else if( HIWORD(wParam)==CBN_DROPDOWN       )
		{
			INT ItemHeight      = SendMessage( *this, CB_GETITEMHEIGHT, 0, 0 );
			INT SelectionHeight = SendMessage( *this, CB_GETITEMHEIGHT, static_cast<WPARAM>(-1), 0 );
			FRect R = GetWindowRect();
			MoveWindow(R.Min.X, R.Min.Y, R.Max.X-R.Min.X, Min((GetCount()+1)*ItemHeight+SelectionHeight, MulDiv(384, DPIY, 96)), 0 );
			DropDownDelegate();
			return 1;
		}
		else if( HIWORD(wParam)==CBN_CLOSEUP        ) {CloseComboDelegate();         return 1;}
		else if( HIWORD(wParam)==CBN_EDITCHANGE     ) {EditChangeDelegate();         return 1;}
		else if( HIWORD(wParam)==CBN_EDITUPDATE     ) {EditUpdateDelegate();         return 1;}
		else if( HIWORD(wParam)==CBN_SETFOCUS       ) {SetFocusDelegate();           return 1;}
		else if( HIWORD(wParam)==CBN_KILLFOCUS      ) {KillFocusDelegate();          return 1;}
		else if( HIWORD(wParam)==CBN_SELCHANGE      ) {SelectionChangeDelegate();    return 1;}
		else if( HIWORD(wParam)==CBN_SELENDOK       ) {SelectionEndOkDelegate();     return 1;}
		else if( HIWORD(wParam)==CBN_SELENDCANCEL   ) {SelectionEndCancelDelegate(); return 1;}
		else return 0;
		unguard;
	}

	// WComboBox interface.
	virtual INT AddString( const TCHAR* Str )
	{
		guard(WComboBox::AddString);
		return SendMessageW( *this, CB_ADDSTRING, 0, (LPARAM)Str );
		unguard;
	}
	virtual INT DeleteString( INT Index )
	{
		guard(WComboBox::AddString);
		return SendMessageW( *this, CB_DELETESTRING, Index, 0 );
		unguard;
	}
	virtual FString GetString( INT Index )
	{
		guard(WComboBox::GetString);
		INT Length = SendMessageW( *this, CB_GETLBTEXTLEN, Index, 0 );
		if( Length==CB_ERR )
			return TEXT("");

		TCHAR* Text = (TCHAR*)malloc((Length+1)*sizeof(TCHAR));
		SendMessage( *this, CB_GETLBTEXT, Index, (LPARAM)Text );
		FString Out = Text;
		free(Text);
		return Out;
		unguard;
	}
	virtual INT GetCount()
	{
		guard(WComboBox::GetCount);
		return SendMessageW( *this, CB_GETCOUNT, 0, 0 );
		unguard;
	}
	virtual void SetCurrent( INT Index )
	{
		guard(WComboBox::SetCurrent);
		SendMessageW( *this, CB_SETCURSEL, Index, 0 );
		unguard;
	}
	virtual INT GetCurrent()
	{
		guard(WComboBox::GetCurrent);
		return SendMessageW( *this, CB_GETCURSEL, 0, 0 );
		unguard;
	}
	virtual INT FindString( const TCHAR* String )
	{
		guard(WComboBox::FindString);
		INT Index = SendMessageW( *this, CB_FINDSTRING, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(String) );
		return Index!=CB_ERR ? Index : -1;
		unguard;
	}
	virtual INT FindStringExact( const TCHAR* String )
	{
		guard(WComboBox::FindStringExact);
		INT Index = SendMessageW( *this, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(String) );
		return Index!=CB_ERR ? Index : -1;
		unguard;
	}
	void Empty()
	{
		guard(WComboBox::Empty);
		SendMessageW( *this, CB_RESETCONTENT, 0, 0 );
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	WMRUComboBox.
-----------------------------------------------------------------------------*/

// A combo box control with MRU list.
class WINDOW_API WMRUComboBox : public WComboBox
{
	W_DECLARE_CLASS(WMRUComboBox,WComboBox,CLASS_Transient);
	DECLARE_WINDOWSUBCLASS(WMRUComboBox,WComboBox,Window)

	INT MaxMRU;
	TArray<FString> MRU;
	FString MRUSection;
	FString MRUItem;
 
	// Constructor.
	WMRUComboBox()	= default;

	WMRUComboBox( WWindow* InOwner, FString InMRUSection, FString InMRUItem, INT InMaxMRU = 16, INT InId=0, WNDPROC InSuperProc=NULL )
	: WComboBox( InOwner, InId, InSuperProc?InSuperProc:SuperProc )
	, MRUSection( InMRUSection )
	, MRUItem( InMRUItem )
	, MaxMRU( InMaxMRU )
	{}	
	void ReadMRU()
	{
		guard(WMRUComboBox::ReadMRU);
		for( int mru = 0 ; mru < MaxMRU ; mru++ )
		{
			FString Item;
			GConfig->GetString( *MRUSection, *(FString::Printf(*MRUItem, mru)), Item, GUEDIni );
			if (Item.Len())
				AddMRU(Item, FALSE);
		}

		RefreshMRU();
		unguard;
	}
	void WriteMRU()
	{
		guard(WMRUComboBox::WriteMRU);
		for( int mru = 0 ; mru < MaxMRU ; mru++ )
			GConfig->SetString( *MRUSection, *(FString::Printf(*MRUItem, mru)), mru < MRU.Num() ? *MRU(mru) : TEXT(""), GUEDIni );
		unguard;
	}
	void AddMRU( FString InCommand, BOOL bNotify = TRUE )
	{
		guard(WMRUComboBox::AddMRU);

		if (!InCommand.Len())
			return;

		int cnt = 0;

		// If the command already exists, erase.
		for( int x = 0; x < MRU.Num() ; x++, cnt++ )
		{
			if (cnt != x)
				MRU(cnt) = MRU(x);
			if( InCommand == MRU(x) )
				cnt--;
		}

		// Otherwise, add it to end of the list.
		cnt++;
		if( cnt > MaxMRU )
		{
			for( int x = 0 ; x < MaxMRU - 1 ; x++ )
				MRU(x) = MRU(x + 1);
			cnt = MaxMRU;
		}

		if (cnt > MRU.Num())
			MRU.AddItem(InCommand);
		else
			MRU(cnt - 1) = InCommand;

		if (bNotify)
		{
			RefreshMRU();
			WriteMRU();
		}

		unguard;
	}
	void RefreshMRU()
	{
		guard(WMRUComboBox::RefreshMRU);

		FString Text = GetText();
		DWORD cursor = SendMessage(hWnd, CB_GETEDITSEL, NULL, NULL);

		// Update the contents of the combobox with the MRU list
		Empty();

		for( int x = MRU.Num() - 1 ; x >= 0 ; x-- )
			AddString( *MRU(x) );

		SetText(*Text);
		SendMessage(hWnd, CB_SETEDITSEL, NULL, cursor);

		unguard;
	}
};

/*-----------------------------------------------------------------------------
	WPackageComboBox.
-----------------------------------------------------------------------------*/

// A combo box control with packages list.
class WINDOW_API WPackageComboBox : public WComboBox
{
	W_DECLARE_CLASS(WPackageComboBox,WComboBox,CLASS_Transient);
	DECLARE_WINDOWSUBCLASS(WPackageComboBox,WComboBox,Window)

	WComboBox* GroupComboBox;
 
	// Constructor.
	WPackageComboBox()	= default;

	WPackageComboBox( WWindow* InOwner, INT InId=0, WNDPROC InSuperProc=NULL )
	: WComboBox( InOwner, InId, InSuperProc?InSuperProc:SuperProc )
	, GroupComboBox( NULL )
	{}
	void Init(const TCHAR* Text, WComboBox* InGroupComboBox = NULL)
	{
		guard(WPackageComboBox::Init);
		Empty();
		for (TObjectIterator<UPackage> It; It; ++It)
			if (*It != UObject::GetTransientPackage() && !It->GetOuter())
				AddString(It->GetName());
		SetText(Text);

		if (InGroupComboBox)
		{
			GroupComboBox = InGroupComboBox;
			EditChangeDelegate = SelectionChangeDelegate = FDelegate(this, (TDelegate)&WPackageComboBox::OnComboChange);
			OnComboChange();
		}
		unguard;
	}
	void OnComboChange()
	{
		guard(WPackageComboBox::OnComboChange);
		if (!GroupComboBox)
			return;
		FString OldText = GroupComboBox->GetText();
		GroupComboBox->Empty();
		INT Curr = GetCurrent();
		UPackage* Pkg = FindObject<UPackage>(NULL, Curr < 0 ? *GetText() : *GetString(Curr));
		if (Pkg)
		{
			INT PackagePrefixLen = FObjectPathName(Pkg).Len() + 1;
			FString GroupName;
			for (TObjectIterator<UPackage> It; It; ++It)
				if (It->IsIn(Pkg))
					GroupComboBox->AddString(*FObjectPathName(*It).Mid(PackagePrefixLen, MAXINT));
			GroupComboBox->AddString(TEXT("None"));
		}
		GroupComboBox->SetText(*OldText);
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	WEdit.
-----------------------------------------------------------------------------*/

// A single-line or multiline edit control.
class WINDOW_API WEdit : public WControl
{
	W_DECLARE_CLASS(WEdit,WControl,CLASS_Transient);
	DECLARE_WINDOWSUBCLASS(WEdit,WControl,Window)

	// Variables.
	FDelegate ChangeDelegate;
	INT FontSize = 0;
	HFONT LoadedFont = NULL;

	// Constructor.
	WEdit() = default;

	WEdit( WWindow* InOwner, INT InId=0, WNDPROC InSuperProc=NULL )
	: WControl( InOwner, InId, InSuperProc?InSuperProc:SuperProc )
	{}

	// WWindow interface.
	void OpenWindow( UBOOL Visible, UBOOL Multiline, UBOOL ReadOnly, UBOOL HorizScroll = FALSE, UBOOL NoHideSel = FALSE )
	{
		guard(WEdit::OpenWindow);
		PerformCreateWindowEx
		(
			WS_EX_CLIENTEDGE,
            NULL,
            WS_CHILD | (HorizScroll?WS_HSCROLL:0) | (Visible?WS_VISIBLE:0) | ES_LEFT | (Multiline?(ES_MULTILINE|WS_VSCROLL):0) | ES_AUTOVSCROLL | ES_AUTOHSCROLL | (ReadOnly?ES_READONLY:0) | (NoHideSel?ES_NOHIDESEL:0),
            0, 0,
			0, 0,
            *OwnerWindow,
            (HMENU)ControlId,
            hInstance
		);

		if (Visible)
			m_bShow = TRUE;

		SendMessageW( *this, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(0,0) );
		// Clear default limit of 30000/32768 bytes/symbols
		SetTextLimit(0x7FFFFFFE);
		unguard;
	}
	UBOOL InterceptControlCommand( UINT Message, WPARAM wParam, LPARAM lParam)
	{
		guard(WEdit::InterceptControlCommand);
		if( HIWORD(wParam)==EN_CHANGE )
		{
			ChangeDelegate();
			return 1;
		}
		else return 0;
		unguard;
	}

	// WEdit interface.
	void SetFontSize(INT CodeFontSize)
	{
		guard(WEdit::SetFontSize);

		CodeFontSize = Clamp(CodeFontSize, 10, 72);
		if (FontSize == CodeFontSize && LoadedFont)
		{
			SetFont(LoadedFont);
			return;
		}
	
		HFONT hFont = CreateFont(-CodeFontSize, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, TEXT("Courier New"));
		if (!hFont && FontSize == 0)
			hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		if (!hFont)
			return;
		if (LoadedFont)
			DeleteObject(LoadedFont);
		LoadedFont = hFont;
		SetFont(hFont);
		debugf(L"SetFontSize: %p, %d -> %d", hFont, FontSize, CodeFontSize);
		FontSize = CodeFontSize;

		unguard;
	}
	void SetTextLimit( int _Limit )
	{
		guard(WEdit::SetTextLimit);
		SendMessageW( *this, EM_SETLIMITTEXT, (WPARAM)_Limit, 0 );
		unguard;
	}
	UBOOL GetReadOnly()
	{
		guard(WEdit::GetReadOnly);
		check(hWnd);
		return (GetWindowLongW( *this, GWL_STYLE )&ES_READONLY)!=0;
		unguard;
	}
	void SetReadOnly( UBOOL ReadOnly )
	{
		guard(WEdit::SetReadOnly);
		check(hWnd);
		SendMessageW( *this, EM_SETREADONLY, ReadOnly, 0 );
		unguard;
	}
	INT GetLineCount()
	{
		guard(WEdit::GetLineCount);
		check(hWnd);
		return SendMessageW( *this, EM_GETLINECOUNT, 0, 0 );
		unguard;
	}
	INT GetLineIndex( INT Line )
	{
		guard(WEdit::GetLineIndex);
		check(hWnd);
		return SendMessageW( *this, EM_LINEINDEX, Line, 0 );
		unguard;
	}
	void GetSelection( INT& Start, INT& End )
	{
		guard(WEdit::GetSelection);
		check(hWnd);
		SendMessageW( *this, EM_GETSEL, (WPARAM)&Start, (LPARAM)&End );
		unguard;
	}
	void SetSelection( INT Start, INT End )
	{
		guard(WEdit::SetSelection);
		check(hWnd);
		SendMessageW( *this, EM_SETSEL, Start, End );
		unguard;
	}
	void SetSelectedText( const TCHAR* Text, BOOL bAllowUndo = TRUE )
	{
		guard(WEdit::SetSelectedText);
		check(hWnd);
		SendMessageW( *this, EM_REPLACESEL, bAllowUndo, reinterpret_cast<LPARAM>(Text) );
		unguard;
	}
	UBOOL GetModify()
	{
		guard(WEdit::GetModify);
		return SendMessageW( *this, EM_GETMODIFY, 0, 0 )!=0;
		unguard;
	}
	void SetModify( UBOOL Modified )
	{
		guard(WEdit::SetModify);
		SendMessageW( *this, EM_SETMODIFY, Modified, 0 );
		unguard;
	}
	void ScrollCaret()
	{
		guard(WEdit::ScrollCaret);
		SendMessageW( *this, EM_SCROLLCARET, 0, 0 );
		unguard;
	}
	virtual FString GetText()
	{
		guard(WEdit::GetText);

		FString Str;
		Str = WWindow::GetText();

		// If the string begins with an "=" sign, we will evaluate it as a mathematical expresssion
		if( Str.Left(1) == TEXT("=") )
			Str = FString::Printf(TEXT("%f"), GetFloat(Str) );

		return Str;

		unguard;
	}
	static float GetFloat(FString Str)
	{
		guard(WEdit::GetFloat);
		float Result;
		// If the string begins with an "=" sign, we will evaluate it as a mathematical expresssion
		if( Str.Left(1) == TEXT("=") )
		{
			Str = Str.Mid(1);	// Strip off the "=" sign

			if( !Eval( Str, &Result ) )
				Result = 0;
		}
		else
			Result = appAtof(*Str);
		return Result;
		unguard;
	}
	// Evaluate a numerical expression.
	// Returns 1 if ok, 0 if error.
	// Sets Result, or 0.0 if error.
	//
	// Operators and precedence: 1:+- 2:/% 3:* 4:^ 5:&|
	// Unary: -
	// Types: Numbers (0-9.), Hex ($0-$f)
	// Grouping: ( )
	//
	static UBOOL Eval( FString Str, float* pResult )
	{
		guard(WEdit::Eval);

		float Result;
		FString Orig = Str;

		// Check for a matching number of brackets right up front.
		int Brackets = 0;
		for( int x = 0 ; x < Str.Len() ; x++ )
		{
			if( Str.Mid(x,1) == TEXT("(") )
				Brackets++;
			else
				if( Str.Mid(x,1) == TEXT(")") )
					Brackets--;
		}
		if( Brackets != 0 )
		{
			debugf(NAME_FriendlyError, TEXT("Mismatched brackets (too many %ls): %ls"), 
				Brackets > 0 ? TEXT("opening") : TEXT("closing"), *Str);
			Result = 0;
		}
		else
			if( !SubEval( &Str, &Result, 0 ) )
			{
				debugf(NAME_FriendlyError, TEXT("Error in expression: %ls"), *Orig);
				Result = 0;
			}

		*pResult = Result;

		return 1;
		unguard;
	}
	static UBOOL SubEval( FString* pStr, float* pResult, int Prec )
	{
		guard(WEdit::SubEval);

		FString c;
		float V, W, N;

		V = W = N = 0.0f;

		c = GrabChar(pStr);

		if((c >= TEXT("0") && c <= TEXT("9")) || c == TEXT(".")) // Number
		{
			V = 0;
			while(c >= TEXT("0") && c <= TEXT("9"))
			{
				V = V * 10 + Val(c);
				c = GrabChar(pStr);
			}
			if(c == TEXT("."))
			{
				N = 0.1f;
				c = GrabChar(pStr);
				while(c >= TEXT("0") && c <= TEXT("9"))
				{
					V = V + N * Val(c);
					N = N / 10.0f;
					c = GrabChar(pStr);
				}
			}
		}
		else if( c == TEXT("(")) // Opening parenthesis
		{
			if( !SubEval(pStr, &V, 0) )
				return 0;
			c = GrabChar(pStr);
		}
		else if( c == TEXT("-") ) // Negation
		{
			if( !SubEval(pStr, &V, 1000) )
				return 0;
			V = -V;
			c = GrabChar(pStr);
		}
		else if( c == TEXT("+")) // Positive
		{
			if( !SubEval(pStr, &V, 1000) )
				return 0;
			c = GrabChar(pStr);
		}
		else if( c == TEXT("@") ) // Square root
		{
			if( !SubEval(pStr, &V, 1000) )
				return 0;
			if( V < 0 )
			{
				debugf(NAME_FriendlyError, TEXT("Can't take square root of negative number: %f"), V);
				return 0;
			}
			else
				V = appSqrt(V);
			c = GrabChar(pStr);
		}
		else // Error
		{
			debugf(NAME_FriendlyError, TEXT("No value recognized at char: %ls"), *c);
			return 0;
		}
PrecLoop:
		if( c == TEXT("") )
		{
			*pResult = V;
			return 1;
		}
		else if( c == TEXT(")") )
		{
			*pStr = TEXT(")") + *pStr;
			*pResult = V;
			return 1;
		}
		else if( c == TEXT("+") )
		{
			if( Prec > 1 )
			{
				*pResult = V;
				*pStr = c + *pStr;
				return 1;
			}
			else
				if( SubEval(pStr, &W, 2) )
				{
					V = V + W;
					c = GrabChar(pStr);
					goto PrecLoop;
				}
				else
					return 0;
		}
		else if( c == TEXT("-") )
		{
			if( Prec > 1 )
			{
				*pResult = V;
				*pStr = c + *pStr;
				return 1;
			}
			else
				if( SubEval(pStr, &W, 2) )
				{
					V = V - W;
					c = GrabChar(pStr);
					goto PrecLoop;
				}
				else
					return 0;
		}
		else if( c == TEXT("/") )
		{
			if( Prec > 2 )
			{
				*pResult = V;
				*pStr = c + *pStr;
				return 1;
			}
			else
				if( SubEval(pStr, &W, 3) )
				{
					if( W == 0 )
					{
						debugf(NAME_FriendlyError, TEXT("Division by zero isn't allowed"));
						return 0;
					}
					else
					{
						V = V / W;
						c = GrabChar(pStr);
						goto PrecLoop;
					}
				}
				else
					return 0;
		}
		else if( c == TEXT("%") )
		{
			if( Prec > 2 )
			{
				*pResult = V;
				*pStr = c + *pStr;
				return 1;
			}
			else 
				if( SubEval(pStr, &W, 3) )
				{
					if( W == 0 )
					{
						debugf(NAME_FriendlyError, TEXT("Modulo zero isn't allowed"));
						return 0;
					}
					else
					{
						V = (int)V % (int)W;
						c = GrabChar(pStr);
						goto PrecLoop;
					}
				}
				else
					return 0;
		}
		else if( c == TEXT("*") )
		{
			if( Prec > 3 )
			{
				*pResult = V;
				*pStr = c + *pStr;
				return 1;
			}
			else
				if( SubEval(pStr, &W, 4) )
				{
					V = V * W;
					c = GrabChar(pStr);
					goto PrecLoop;
				}
				else
					return 0;
		}
		else
		{
			debugf(NAME_FriendlyError, TEXT("Unrecognized Operator: %ls"), *c);
			return 0;
		}

		*pResult = V;
		return 1;
		unguard;
	}
	static FString GrabChar( FString* pStr )
	{
		guard(WEdit::GrabChar);

		FString GrabChar;
		if( pStr->Len() )
			do {
				GrabChar = pStr->Left(1);
				*pStr = pStr->Mid(1);
			} while( GrabChar == TEXT(" ") );
		else
			GrabChar = TEXT("");

		return GrabChar;
		unguard;
	}
	// Converts a string to it's numeric equivalent, ignoring whitespace.
	// "123  45" - becomes 12,345
	static FLOAT Val( FString Value )
	{
		FLOAT RetValue = 0;

		for( int x = 0 ; x < Value.Len() ; x++ )
		{
			FString Char = Value.Mid(x, 1);

			if( Char >= TEXT("0") && Char <= TEXT("9") )
			{
				RetValue *= 10;
				RetValue += appAtoi( *Char );
			}
			else 
				if( Char != TEXT(" ") )
					break;
		}

		return RetValue;
	}
};

/*-----------------------------------------------------------------------------
	WTerminal.
-----------------------------------------------------------------------------*/

// Base class of terminal edit windows.
class WINDOW_API WTerminalBase : public WWindow
{
	W_DECLARE_ABSTRACT_CLASS(WTerminalBase,WWindow,CLASS_Transient);
	DECLARE_WINDOWCLASS(WTerminalBase,WWindow,Window)

	// Constructor.
	WTerminalBase()	= default;

	WTerminalBase( FName InPersistentName, WWindow* InOwnerWindow )
	: WWindow( InPersistentName, InOwnerWindow )
	{}

	// WTerminalBase interface.
	virtual void TypeChar( TCHAR Ch )=0;
	virtual void Paste()=0;
};

// A terminal edit window.
class WINDOW_API WEditTerminal : public WEdit
{
	W_DECLARE_ABSTRACT_CLASS(WEditTerminal,WEdit,CLASS_Transient)
	DECLARE_WINDOWCLASS(WEditTerminal,WEdit,Window)

	// Variables.
	WTerminalBase* OwnerTerminal{};

	// Constructor.
	WEditTerminal( WTerminalBase* InOwner=NULL )
	: WEdit( InOwner )
	, OwnerTerminal( InOwner )
	{}

	// WWindow interface.
	void OnChar( TCHAR Ch )
	{
		if( Ch!=('C'-'@') )
		{
			OwnerTerminal->TypeChar( Ch );
			throw TEXT("NoRoute");
		}
	}
	void OnRightButtonDown(INT X, INT Y)
	{
		throw TEXT("NoRoute");
	}
	void OnPaste()
	{
		OwnerTerminal->Paste();
		throw TEXT("NoRoute");
	}
	void OnUndo()
	{
		throw TEXT("NoRoute");
	}
};

// A terminal window.
class WINDOW_API WTerminal : public WTerminalBase, public FOutputDevice
{
	W_DECLARE_CLASS(WTerminal,WTerminalBase,CLASS_Transient);
	DECLARE_WINDOWCLASS(WTerminal,WTerminalBase,Window)

	// Variables.
	WEditTerminal Display;
	FExec* Exec{};
	INT MaxLines{}, SlackLines{}, LineCount{};
	FString Typing;
	UBOOL Shown{};
	TArray<FString> Pending;
	INT PendingOffset{}, PendingPos{};
	CRITICAL_SECTION UsePendingBuffer;

	// Structors.
	WTerminal()	= default;

	WTerminal( FName InPersistentName, WWindow* InOwnerWindow )
	:	WTerminalBase	( InPersistentName, InOwnerWindow )
	,	Display			( this )
	,	Exec			( NULL )
	,	MaxLines		( GIsEditor ? 768 : 256 )
	,	SlackLines		( 64 )
	,	LineCount		( 0 )
	,	Shown			( 0 )
	{
		Typing = TEXT(">");
		InitializeCriticalSectionAndSpinCount(&UsePendingBuffer, 0x00000400);
	}

	// FOutputDevice interface.
	void Serialize( const TCHAR* Data, EName MsgType )
	{
		guard(WTerminal::Serialize);
		if( MsgType==NAME_Title )
		{
			SetText( Data );
			return;
		}
		else if( Shown )
		{
			EnterCriticalSection(&UsePendingBuffer); 
			if (Pending.Num() < MaxLines)
				Pending.AddItem(TEXT(""));

			if (PendingOffset == PendingPos)
				PostMessage(hWnd, WM_PAINT, 0, 0);

			INT Index = PendingPos++ % MaxLines;
			
			FString& Temp = Pending(Index);
			Temp = *FName(MsgType);
			Temp += TEXT(": ");
			Temp += Data;
			Temp += TEXT("\r\n");
			LeaveCriticalSection(&UsePendingBuffer);
		}
		unguard;
	}

	// WWindow interface.
	void DoDestroy()
	{
		guard(WTerminal::DoDestroy);
		DeleteCriticalSection(&UsePendingBuffer);
		WTerminalBase::DoDestroy();
		unguard;
	}
	void OnShowWindow( UBOOL bShow )
	{
		guard(WTerminal::OnShowWindow);
		Shown = bShow;
		unguard;
	}
	void OnCreate()
	{
		guard(WTerminal::OnCreate);
		WWindow::OnCreate();
		Display.OpenWindow( 1, 1, 1 );

		Display.SetFont( (HFONT)GetStockObject(DEFAULT_GUI_FONT) );
		Display.SetText( *Typing );
		unguard;
	}
	void OpenWindow( UBOOL bMdi=0, UBOOL AppWindow=0 )
	{
		guard(WTerminal::OpenWindow);
		MdiChild = bMdi;
		PerformCreateWindowEx
		(
			MdiChild
			?	(WS_EX_MDICHILD)
			:	(AppWindow?WS_EX_APPWINDOW:0),
			*FString::Printf( LocalizeGeneral(TEXT("LogWindow"), TEXT("Window")), GIsEditor ?
				*FString::Printf(LocalizeGeneral(TEXT("FrameWindow"), TEXT("UnrealEd")), LocalizeGeneral(TEXT("Product"), TEXT("Core"))) : 
				LocalizeGeneral(TEXT("Product"), TEXT("Core")) ),
			MdiChild
			?	(WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
			:	(WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_SIZEBOX | WS_MINIMIZEBOX | WS_MAXIMIZEBOX),
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
	void OnSetFocus( HWND hWndLoser )
	{
		guard(WTerminal::OnSetFocus);
		WWindow::OnSetFocus( hWndLoser );
		SetFocus( Display );
		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WTerminal::OnSize);
		WWindow::OnSize( Flags, NewX, NewY );
		Display.MoveWindow( FRect(0,0,NewX,NewY), TRUE );
		Display.ScrollCaret();
		unguard;
	}
	void OnPaint()
	{
		guard(WTerminal::OnPaint);
		if (PendingPos > PendingOffset)
		{
			INT AddLines = Min(MaxLines, PendingPos - PendingOffset);
			Display.SetRedraw(0);
			INT Start = 0, End = 0;
			Display.GetSelection(Start, End);
			if (LineCount == 0 || LineCount + AddLines > MaxLines + 1)
			{
				LineCount = Display.GetLineCount();
				if (LineCount + AddLines > MaxLines + 1)
				{
					INT NewLineCount = Max(LineCount - Max(SlackLines, AddLines), 1);
					INT Index = Display.GetLineIndex(LineCount - NewLineCount);
					Display.SetSelection(0, Index);
					Display.SetSelectedText(TEXT(""), FALSE);
					Start -= Index;
					End -= Index;
					LineCount = Display.GetLineCount();
				}
			}
			INT Length = Display.GetLength();
			FString Temp;
			EnterCriticalSection(&UsePendingBuffer);
			if (PendingPos - MaxLines > PendingOffset)
				PendingOffset = PendingPos - MaxLines;
			for (; PendingOffset < PendingPos; PendingOffset++)
			{
				INT Index = PendingOffset % MaxLines;
				Temp += Pending(Index);
			}
			LeaveCriticalSection(&UsePendingBuffer);
			Temp += *Typing;
			Display.SetSelection(Max(Length - Typing.Len(), 0), Length);
			Display.SetSelectedText(*Temp, FALSE);
			if (Start >= Length - Typing.Len())
			{
				INT Diff = Temp.Len() - Typing.Len();
				Start += Diff;
				End += Diff;
			}
			Display.SetSelection(Max(0, Start), Max(0, End));
			LineCount += AddLines;
			SendMessageW(Display.hWnd, WM_VSCROLL, SB_BOTTOM, NULL);
			Display.SetRedraw(1);
		}
		unguard;
	}

	// WTerminalBase interface.
	void Paste()
	{
		guard(WTerminal::Paste);
		FString Str = appClipboardPaste();
		for (INT i = 0; i < Str.Len(); i++)
			if( Str[i] < 32 || Str[i] >= 127 )
				Str[i] = 0;
		if (Str.Len())
			EditTyping(Str);
		unguard;
	}
	void TypeChar( TCHAR Ch )
	{
		guard(WTerminal::TypeChar);
		if (Ch == 27) // Escape
		{
			SelectTyping();
			Typing = TEXT(">");
			Display.SetSelectedText(*Typing);
		}
		else if (Ch == 22) // Ctrl+V
		{
			Paste();
		}
		else if (Ch == 13 && Typing.Len() > 1) // Enter
		{
			SelectTyping();
			FString Temp = &Typing[1];
			Typing = TEXT(">");
			Display.SetSelectedText(*Typing);
			
			if (Exec)
				if (!Exec->Exec(*Temp, *GLog))
					Log (LocalizeError("Exec", TEXT("Core")));
		}
		else if (((Ch == 8 || Ch == 127) && Typing.Len() > 1) || // Backspace or Delete
			(Ch >= 32 && Ch < 127)) // Text
		{
			FString Str;
			Str += Ch;
			EditTyping(Str);
		}
		unguard;
	}

	// WTerminal interface.
	void EditTyping(FString Add)
	{
		guard(WTerminal::AddTyping);
		if (!Add.Len())
			return;
		INT Length = Display.GetLength();
		INT Last = Typing.Len() - 1;
		INT Min = Length - Typing.Len() + 1;
		INT Start = 0, End = 0;
		Display.GetSelection(Start, End);
		Start -= Min;
		End -= Min;
		if (Start == -1 && End == -1) // cursor before prompt
			Start = End = 0;
		else if (End < 0)
			Start = End = Last;
		else if (Start < 0)
			Start = 0;
		if (Add.Len() == 1)
		{
			if (Add[0] == 8) // Backspace
			{
				Add = TEXT("");
				if (Start > 0)
					Start--;
			}
			else if (Add[0] == 127) // Delete
			{
				Add = TEXT("");
				if (End < Last)
					End++;
			}
		}
		Display.SetSelection(Min + Start, Min + End);
		if (Start == Last && End == Last)
			Typing += Add;
		else
			Typing = Typing.Left(Start + 1) + Add + Typing.Mid(End + 1);
		
		Display.SetSelectedText(*Add);
		unguard;
	}
	void SelectTyping()
	{
		guard(WTerminal::SelectTyping);
		INT Length = Display.GetLength();
		Display.SetSelection(Max(Length - Typing.Len(), 0), Length);
		unguard;
	}
	void SetExec( FExec* InExec )
	{
		Exec = InExec;
	}
};

/*-----------------------------------------------------------------------------
	WLog.
-----------------------------------------------------------------------------*/

// A log window.
static void GNotifyExit()
{
	if( GNotify )
		Shell_NotifyIconW(NIM_DELETE,&NID);
}
class WINDOW_API WLog : public WTerminal
{
	W_DECLARE_CLASS(WLog,WTerminal,CLASS_Transient);
	DECLARE_WINDOWCLASS(WLog,WTerminal,Window)

	// Variables.
	UINT NidMessage{};
	FArchive*& LogAr;
	FString LogFilename;

	// Functions.
	WLog()
	:LogAr(LogAr)
	{}
	WLog( const TCHAR* InLogFilename, FArchive*& InLogAr, FName InPersistentName, WWindow* InOwnerWindow=NULL )
	: WTerminal( InPersistentName, InOwnerWindow )
	, NidMessage( RegisterWindowMessageW( TEXT("UnrealNidMessage")) )
	, LogAr( InLogAr )
	, LogFilename( InLogFilename )
	{}
	void OnCreate()
	{
		guard(WLog::OnCreate);
		WWindow::OnCreate();
		Display.OpenWindow( 1, 1, 1 );		

		INT X, Y, W, H, FontSize;
		if (!GConfig->GetInt(*PersistentName, TEXT("X"), X, GUEDIni))	X = 0;
		if (!GConfig->GetInt(*PersistentName, TEXT("Y"), Y, GUEDIni))	Y = 0;
		if (!GConfig->GetInt(*PersistentName, TEXT("W"), W, GUEDIni))	W = 512;
		if (!GConfig->GetInt(*PersistentName, TEXT("H"), H, GUEDIni))	H = 384;
		if (!GConfig->GetInt(*PersistentName, TEXT("FontSize"), FontSize, GUEDIni))	FontSize = 12;

		if (!W) W = 512;
		if (!H) H = 384;
		
		LOGFONT lf{};
		lf.lfHeight = -MulDiv(FontSize, DPIY, 72);
		lf.lfCharSet = ANSI_CHARSET;
		lf.lfPitchAndFamily = FIXED_PITCH;
		HFONT hfont = CreateFontIndirect(&lf);
		Display.SetFont( /*(HFONT)GetStockObject(ANSI_FIXED_FONT) */ hfont);
		Display.SetText(*Typing);		

		::MoveWindow(hWnd, X, Y, W, H, TRUE);
		
		unguard;
	}
	void SetText( const TCHAR* Text )
	{
		guard(WLog::SetText);
		WWindow::SetText( Text );
		if( GNotify )
		{
			appStrncpy( NID.szTip, Text, ARRAY_COUNT(NID.szTip) );
			Shell_NotifyIconW(NIM_MODIFY,&NID);
		}
		unguard;
	}
	void OnShowWindow( UBOOL bShow )
	{
		guard(WLog::OnShowWindow);
		WTerminal::OnShowWindow( bShow );
		if( bShow )
		{
			// Load log file.
			if( LogAr )
			{
				delete LogAr;
				FString Text;
				if (appLoadFileToString(Text, *LogFilename))
				{
					INT CrCount = 0;
					INT Ofs;
					for (Ofs = Text.Len() - 1; Ofs > 0 && CrCount < MaxLines; Ofs--)
						CrCount += (Text[Ofs] == L'\n');
					while (Ofs < Text.Len() && Text[Ofs] == L'\n')
						Ofs++;
					Text += L'\n';
					Text += L'>';
					Display.SetText(&Text[Ofs]);
				}
				LogAr = GFileManager->CreateFileWriter( *LogFilename, FILEWRITE_AllowRead|FILEWRITE_Unbuffered|FILEWRITE_Append );
			}
			else if (GIsEditor)
			{
				MaxLines *= 4; // Buggie: Increase the scrollback buffer size if the log window is not backed by an on-disk log file
			}
			INT Length = Display.GetLength();
			Display.SetSelection( Length, Length );
			Display.ScrollCaret();
		}
		unguard;
	}
	void OpenWindow( UBOOL bShow, UBOOL bMdi )
	{
		guard(WLog::OpenWindow);

		WTerminal::OpenWindow( bMdi, 0 );
		Show( bShow );
		UpdateWindow( *this );
		GLogHook = this;

		// Show dedicated server in tray.
		if( !GIsClient && !GIsEditor )
		{
			NID.cbSize           = sizeof(NID);
			NID.hWnd             = hWnd;
			NID.uID              = 0;
			NID.uFlags           = NIF_ICON | NIF_TIP | NIF_MESSAGE;
			NID.uCallbackMessage = NidMessage;
			NID.hIcon            = LoadIconW(hInstanceWindow,MAKEINTRESOURCEW((GIsEditor?IDICON_Editor:IDICON_Mainframe)));
			NID.szTip[0]         = 0;
			Shell_NotifyIconW(NIM_ADD,&NID);
			GNotify = 1;
			atexit( GNotifyExit );
		}

		unguard;
	}
	void OnDestroy()
	{
		guard(WLog::OnDestroy);

		// Save Window position (base class doesn't always do this properly)
		// (Don't do this if the window is minimized.)
		//
		if (!::IsIconic(hWnd) && !::IsZoomed(hWnd))
		{
			RECT R;
			if (::GetWindowRect(hWnd, &R))
			{
				GConfig->SetInt(*PersistentName, TEXT("X"), R.left, GUEDIni);
				GConfig->SetInt(*PersistentName, TEXT("Y"), R.top, GUEDIni);
				GConfig->SetInt(*PersistentName, TEXT("W"), R.right - R.left, GUEDIni);
				GConfig->SetInt(*PersistentName, TEXT("H"), R.bottom - R.top, GUEDIni);
			}
		}

		GLogHook = NULL;
		WTerminal::OnDestroy();

		unguard;
	}
	void OnCopyData( HWND hWndSender, COPYDATASTRUCT* CD )
	{
		guard(WLog::OnCopyData);
		if( Exec )
		{
			debugf( TEXT("WM_COPYDATA: %s"), (TCHAR*)CD->lpData );
			Exec->Exec( TEXT("TakeFocus"), *GLogWindow );
			TCHAR NewURL[1024];
			if (ParseToken(*(const TCHAR * *)& CD->lpData, NewURL, ARRAY_COUNT(NewURL), 0)
				&& NewURL[0] != '-')
			{
				FString OpenURL = FString::Printf(TEXT("Open %s"), NewURL);
				Exec->Exec(*OpenURL, *GLogWindow);
			}
		}
		unguard;
	}
	void OnClose()
	{
		guard(WLog::OnClose);
		Show( 0 );
		throw TEXT("NoRoute");
		unguard;
	}
	void OnCommand( INT Command )
	{
		guard(WLog::OnCommand);
		if( Command==ID_LogFileExit || Command==ID_NotifyExit )
		{
			// Exit.
			debugf( TEXT("WLog::OnCommand %s"), Command==ID_LogFileExit ? TEXT("ID_LogFileExit") : TEXT("ID_NotifyExit") );
			appRequestExit( 0 );
		}
		else if( Command==ID_LogAdvancedOptions || Command==ID_NotifyAdvancedOptions )
		{
			// Advanced options.
			if( Exec )
				Exec->Exec( TEXT("PREFERENCES"), *GLogWindow );
		}
		else if( Command==ID_NotifyShowLog )
		{
			// Show log window.
			ShowWindow( hWnd, SW_SHOWNORMAL );
			SetForegroundWindow( hWnd );
		}
		unguard;
	}
	LRESULT WndProc( UINT Message, WPARAM wParam, LPARAM lParam )
	{
		guard(WLog::WndProc);
		if( Message==NidMessage )
		{
			if( lParam==WM_RBUTTONDOWN || lParam==WM_LBUTTONDOWN )
			{
				// Options.
				POINT P;
				::GetCursorPos( &P );
				HMENU hMenu = LoadLocalizedMenu( hInstanceWindow, IDMENU_NotifyIcon, TEXT("IDMENU_NotifyIcon"), TEXT("Window") );
				SetForegroundWindow( hWnd );
				TrackPopupMenu( GetSubMenu(hMenu,0), lParam==WM_LBUTTONDOWN ? TPM_LEFTBUTTON : TPM_RIGHTBUTTON, P.x, P.y, 0, hWnd, NULL );
				DestroyMenu(hMenu);
				PostMessageW( hWnd, WM_NULL, 0, 0 );
			}
			return 1;
		}
		else return WWindow::WndProc( Message, wParam, lParam );
		unguard;
	}
};

class WINDOW_API WViewText : public WWindow
{
	W_DECLARE_CLASS(WViewText,WWindow,CLASS_Transient);
	DECLARE_WINDOWCLASS(WViewText,WWindow,Window)

	// Variables.
	WEdit Display;

	// Structors.
	WViewText()	= default;

	WViewText( FName InPersistentName, WWindow* InOwnerWindow )
	:	WWindow			( InPersistentName, InOwnerWindow )
	,	Display			( this )
	{
	}
	
	void UpdateText(FString InCaption, FString InText)
	{
		if (!hWnd)
			return;
		if (GetText() != InCaption)
			SetText(*InCaption);
		if (Display.GetText() != InText)
		{
			INT Start, End;
			Display.GetSelection(Start, End);
			Display.SetText(*InText);
			Display.SetSelection(Start, End);
		}
	}

	void OpenEditWindow()
	{
		guard(WViewText::OpenEditWindow);
		Display.PerformCreateWindowEx
		(
			WS_EX_CLIENTEDGE,
            NULL,
            WS_CHILD | WS_VISIBLE | ES_LEFT | ES_MULTILINE | WS_VSCROLL | ES_AUTOVSCROLL | ES_READONLY | ES_NOHIDESEL,
            0, 0,
			0, 0,
            *this,
            (HMENU)Display.ControlId,
			hInstance
		);

		Display.m_bShow = TRUE;

		// Clear default limit of 30000/32768 bytes/symbols
		Display.SetTextLimit(0x7FFFFFFE);
		unguard;
	}

	// WWindow interface.
	void OnCreate()
	{
		guard(WViewText::OnCreate);
		WWindow::OnCreate();
		OpenEditWindow();
		Display.SetReadOnly(TRUE);
		SetFontSize();
		INT TabSize = 16; // 4 chars
		SendMessageW(Display.hWnd, EM_SETTABSTOPS, 1, (LPARAM)&TabSize);
		unguard;
	}
	void SetFontSize()
	{
		guard(WViewText::SetFontSize);
		INT CodeFontSize = 11*MulDiv(1, DPIY, 96);
		GConfig->GetInt(TEXT("Options"), TEXT("CodeFontSize"), CodeFontSize, GUEDIni);

		Display.SetFontSize(CodeFontSize);
		unguard;
	}
	void OnShowWindow( UBOOL bShow )
	{
		guard(WViewText::OnShowWindow);
		SetFontSize();
		unguard;
	}
	void OpenWindow( HWND hWndParent = NULL )
	{
		guard(WViewText::OpenWindow);
		PerformCreateWindowEx
		(
			WS_EX_TOOLWINDOW,
			TEXT(""),
			WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_SIZEBOX | WS_MAXIMIZEBOX,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			MulDiv(512, DPIX, 96),
			MulDiv(512, DPIY, 96),
			hWndParent ? hWndParent : OwnerWindow ? OwnerWindow->hWnd : NULL,
			NULL,
			hInstance
		);
		unguard;
	}
	void OnSetFocus( HWND hWndLoser )
	{
		guard(WViewText::OnSetFocus);
		WWindow::OnSetFocus( hWndLoser );
		SetFocus( Display );
		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WViewText::OnSize);
		WWindow::OnSize( Flags, NewX, NewY );
		Display.MoveWindow( FRect(0,0,NewX,NewY), TRUE );
		Display.ScrollCaret();
		unguard;
	}
	int OnSysCommand( INT Command )
	{
		guard(WViewText::OnSysCommand);
		// Don't actually close the window when the user hits the "X" button.  Just hide it.
		if( Command == SC_CLOSE )
		{
			Show(0);
			return 1;
		}

		return 0;
		unguard;
	}
	void OnCommand(INT Command)
	{
		guard(WViewText::OnCommand);
		switch (Command)
		{
			case IDPB_SELECT_ALL:
				Display.SetSelection(0, -1);
				break;
			default:
				WWindow::OnCommand(Command);
				break;
		}
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	WDialog.
-----------------------------------------------------------------------------*/

// A dialog window, always based on a Visual C++ dialog template.
class WINDOW_API WDialog : public WWindow
{
	W_DECLARE_ABSTRACT_CLASS(WDialog,WWindow,CLASS_Transient);

	BOOL bModal = FALSE;

	// Constructors.
	WDialog() = default;

	WDialog( FName InPersistentName, INT InDialogId, WWindow* InOwnerWindow=NULL )
	: WWindow( InPersistentName, InOwnerWindow )
	{
		ControlId = InDialogId;
	}

	// WDialog interface.
	LRESULT CallDefaultProc( UINT Message, WPARAM wParam, LPARAM lParam)
	{
		return 0;
	}
	virtual INT DoModal( HINSTANCE hInst=hInstanceWindow )
	{
		guard(WDialog::DoModal);
		check(hWnd==NULL);
		AddWindow(this, TEXT("WDialog"));
		ModalCount++;
		bModal = TRUE;
		INT Result = DialogBoxParamW(hInst/*!!*/,MAKEINTRESOURCEW(ControlId),OwnerWindow?OwnerWindow->hWnd:NULL,(DLGPROC)StaticWndProc,(LPARAM)this);
		bModal = FALSE;
		ModalCount--;
		return Result;
		unguard;
	}
	void OpenChildWindow( INT InControlId, UBOOL Visible )
	{
		guard(WDialog::OpenChildWindow);
		check(!hWnd);
		AddWindow(this, TEXT("WDialogChild"));
		HWND hWndParent = InControlId ? GetDlgItem(OwnerWindow->hWnd,InControlId) : OwnerWindow ? OwnerWindow->hWnd : NULL;
		HWND hWndCreated = CreateDialogParamW(hInstanceWindow/*!!*/,MAKEINTRESOURCEW(ControlId),hWndParent,(DLGPROC)StaticWndProc,(LPARAM)this);
		check(hWndCreated);
		check(hWndCreated==hWnd);
		Show( Visible );
		unguard;
	}
	static BOOL CALLBACK LocalizeTextEnumProc( HWND hInWmd, LPARAM lParam )
	{
		guard(WDialog::LocalizeTextEnumProc);
		FString String;
		TCHAR** Temp = (TCHAR**)lParam;
		TCHAR Ch[1024]=TEXT("");
		SendMessage( hInWmd, WM_GETTEXT, ARRAY_COUNT(Ch), (LPARAM)Ch );
		String = Ch;
		if( FString(String).Left(4)==TEXT("IDC_") )
			SendMessageW( hInWmd, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(*LineFormat(Localize(Temp[0],*String,Temp[1]))) );
		else if( String==TEXT("IDOK") )
			SendMessageW( hInWmd, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(*LineFormat(LocalizeGeneral(TEXT("OkButton"), TEXT("Window")))) );
		else if( String==TEXT("IDCANCEL") )
			SendMessageW( hInWmd, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(*LineFormat(LocalizeGeneral(TEXT("CancelButton"), TEXT("Window")))) );
		SendMessageW( hInWmd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(1,0) );
		return 1;
		unguard;
	}
	virtual void LocalizeText( const TCHAR* Section, const TCHAR* Package=GPackage )
	{
		guard(WDialog::LocalizeText);
		const TCHAR* Temp[4];
		Temp[0] = Section;
		Temp[1] = Package;
		Temp[3] = (TCHAR*)this;
		EnumChildWindows( *this, LocalizeTextEnumProc, (LPARAM)Temp );
		LocalizeTextEnumProc( hWnd, (LPARAM)Temp );
		unguard;
	}
	virtual void OnInitDialog()
	{
		guard(WDialog::OnInitDialog);
		WWindow::OnInitDialog();
		SendMessageW( hWnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(1,0) );
		INT i;
		for( i=0; i<Controls.Num(); i++ )
		{
			// Bind all child controls.
			WControl* Control = Controls(i);
			check(!Control->hWnd);
			Control->hWnd = GetDlgItem( *this, Control->ControlId );
			check(Control->hWnd);
			AddWindow(Control, TEXT("WDialogControl"));
			Control->WindowDefWndProc = (WNDPROC)GetWindowLongPtr( Control->hWnd, GWLP_WNDPROC );
			SetWindowLongPtr( Control->hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WWindow::StaticWndProc) );
			//warning: Don't set GWL_HINSTANCE, it screws up subclassed edit controls in Win95.
		}
		for( i=0; i<Controls.Num(); i++ )
		{
			// Send create to all controls.
			Controls(i)->OnCreate();
		}
		LocalizeText( *FString::Printf(TEXT("IDDIALOG_%s"), *PersistentName), GetPackageName() );
		unguard;
	}
	void EndDialog( INT Result )
	{
		::EndDialog( hWnd, Result );
	}
	void EndDialogTrue()
	{
		EndDialog( 1 );
	}
	void EndDialogFalse()
	{
		EndDialog( 0 );
	}	
	void OnClose()
	{
		if (!bModal)
			WWindow::OnClose();
	}
};

/*-----------------------------------------------------------------------------
	WPasswordDialog.
-----------------------------------------------------------------------------*/

// A password dialog box.
class WINDOW_API WPasswordDialog : public WDialog
{
	W_DECLARE_CLASS(WPasswordDialog,WDialog,CLASS_Transient);
	DECLARE_WINDOWCLASS(WPasswordDialog,WDialog,Window)

	// Controls.
	WCoolButton OkButton;
	WCoolButton CancelButton;
	WEdit Name;
	WEdit Password;
	WLabel Prompt;

	// Output.
	FString ResultName;
	FString ResultPassword;

	// Constructor.
	WPasswordDialog()
	: WDialog	  ( TEXT("PasswordDialog"), IDDIALOG_Password )
	, OkButton    ( this, IDOK,     FDelegate(this,(TDelegate)&WDialog::EndDialogTrue) )
	, CancelButton( this, IDCANCEL, FDelegate(this,(TDelegate)&WDialog::EndDialogFalse) )
	, Name		  ( this, IDEDIT_Name )
	, Password	  ( this, IDEDIT_Password )
	, Prompt      ( this, IDLABEL_Prompt )
	{}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WPasswordDialog::OnInitDialog);
		WDialog::OnInitDialog();
		SetText( LocalizeQuery("PassDlg",TEXT("Core")) );
		Prompt.SetText( LocalizeQuery("PassPrompt",TEXT("Core")) );
		Name.SetText( TEXT("") );
		Password.SetText( TEXT("") );
		SetFocus( Name );
		unguard;
	}
	void OnDestroy()
	{
		guard(WPasswordDialog::OnDestroy);
		ResultName     = Name.GetText();
		ResultPassword = Password.GetText();
		WDialog::OnDestroy();
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	WTextScrollerDialog.
-----------------------------------------------------------------------------*/

// A text scroller dialog box.
class WINDOW_API WTextScrollerDialog : public WDialog
{
	W_DECLARE_CLASS(WTextScrollerDialog,WDialog,CLASS_Transient);
	DECLARE_WINDOWCLASS(WTextScrollerDialog,WDialog,Window)

	// Controls.
	WEdit TextEdit;
	WCoolButton OkButton;
	FString Caption, Message;

	// Constructor.
	WTextScrollerDialog() = default;

	WTextScrollerDialog( const TCHAR* InCaption, const TCHAR* InMessage )
	: WDialog	( TEXT("TextScrollerDialog"), IDDIALOG_TextScroller )
	, TextEdit  ( this, IDEDIT_TextEdit )
	, OkButton  ( this, IDOK, FDelegate(this,(TDelegate)&WDialog::EndDialogTrue) )
	, Caption   ( InCaption )
	, Message   ( InMessage )
	{}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WTextScrollerDialog::OnInitDialog);
		WDialog::OnInitDialog();
		HDC hDC = GetDC(hWnd);
		ReleaseDC(hWnd,hDC);
		TextEdit.SetFont( hFontText );
		SetText( *Caption );
		TextEdit.SetText( *Message );
		SetFocus( OkButton );
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	WTrackBar.
-----------------------------------------------------------------------------*/

// A non-interactive label control.
class WINDOW_API WTrackBar : public WControl
{
	W_DECLARE_CLASS(WTrackBar,WControl,CLASS_Transient)
	DECLARE_WINDOWSUBCLASS(WTrackBar,WControl,Window)

	// Delegates.
	FDelegate ThumbTrackDelegate;
	FDelegate ThumbPositionDelegate;

	// Constructor.
	WTrackBar()	= default;

	WTrackBar( WWindow* InOwner, INT InId=0, WNDPROC InSuperProc=NULL )
	: WControl( InOwner, InId, InSuperProc?InSuperProc:SuperProc )
	{}

	// WWindow interface.
	void OpenWindow( UBOOL Visible )
	{
		guard(WTrackBar::OpenWindow);
		PerformCreateWindowEx
		(
			WS_EX_CLIENTEDGE,
            NULL,
            WS_CHILD | TBS_HORZ | TBS_AUTOTICKS | TBS_BOTTOM | (Visible?WS_VISIBLE:0),
            0, 0,
			0, 0,
            *OwnerWindow,
            (HMENU)ControlId,
            hInstance
		);

		if (Visible)
			m_bShow = TRUE;
		
		unguard;
	}

	// WControl interface.
	UBOOL InterceptControlCommand( UINT Message, WPARAM wParam, LPARAM lParam)
	{
		guard(WTrackBar::InterceptControlCommand);
		if     ( Message==WM_HSCROLL && LOWORD(wParam)==TB_THUMBTRACK ) {ThumbTrackDelegate();    return 1;}
		else if( Message==WM_HSCROLL                                  ) {ThumbPositionDelegate(); return 1;}
		else return 0;
		unguard;
	}

	// WTrackBar interface.
	void SetTicFreq( INT TicFreq )
	{
		guard(WTrackBar::SetTicFreq);
		SendMessageW( *this, TBM_SETTICFREQ, TicFreq, 0 );
		unguard;
	}
	void SetRange( INT Min, INT Max )
	{
		guard(WTrackBar::SetRange);
		SendMessageW( *this, TBM_SETRANGE, 1, MAKELONG(Min,Max) );
		unguard;
	}
	void SetPos( INT Pos )
	{
		guard(WTrackBar::SetPos);
		SendMessageW( *this, TBM_SETPOS, 1, Pos );
		unguard;
	}
	INT GetPos()
	{
		guard(WTrackBar::GetPos);
		return SendMessageW( *this, TBM_GETPOS, 0, 0 );
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	WTrackBar.
-----------------------------------------------------------------------------*/

// A non-interactive label control.
class WINDOW_API WProgressBar : public WControl
{
	W_DECLARE_CLASS(WProgressBar,WControl,CLASS_Transient);
	DECLARE_WINDOWSUBCLASS(WProgressBar,WControl,Window)

	// Variables.
	INT Percent{};

	// Constructor.
	WProgressBar() = default;

	WProgressBar( WWindow* InOwner, INT InId=0, WNDPROC InSuperProc=NULL )
	: WControl( InOwner, InId, InSuperProc?InSuperProc:SuperProc )
	, Percent( 0 )
	{}

	// WWindow interface.
	void OpenWindow( UBOOL Visible )
	{
		guard(WProgressBar::OpenWindow);
		PerformCreateWindowEx
		(
			WS_EX_CLIENTEDGE,
            NULL,
            WS_CHILD | (Visible?WS_VISIBLE:0),
            0, 0,
			0, 0,
            *OwnerWindow,
            (HMENU)ControlId,
            hInstance
		);
		SendMessageW( *this, PBM_SETRANGE, 0, 100 );
		unguard;
	}

	// WProgressBar interface.
	void SetProgress( INT InCurrent, INT InMax )
	{
		INT InPercent = (SQWORD)InCurrent*100/Max(InMax,1);
		if( InPercent!=Percent )
			SendMessageW( *this, PBM_SETPOS, InPercent, 0 );
		Percent = InPercent;
	}
};

/*-----------------------------------------------------------------------------
	WListBox.
-----------------------------------------------------------------------------*/

// A list box.
class WINDOW_API WListBox : public WControl
{
	W_DECLARE_CLASS(WListBox,WControl,CLASS_Transient);
	DECLARE_WINDOWSUBCLASS(WListBox,WControl,Window)

	// Delegates.
	FDelegate DoubleClickDelegate;
	FDelegate SelectionChangeDelegate;
	FDelegate SelectionCancelDelegate;
	FDelegate SetFocusDelegate;
	FDelegate KillFocusDelegate;
	FDelegateInt CommandDelegate;

	BOOL m_bMultiSel{};

	// Constructor.
	WListBox() = default;

	WListBox( WWindow* InOwner, INT InId=0, WNDPROC InSuperProc=NULL )
	: WControl( InOwner, InId, InSuperProc?InSuperProc:SuperProc )
	{
		check(OwnerWindow);
		m_bMultiSel = FALSE;
	}

	// WWindow interface.
	void OpenWindow( UBOOL Visible, UBOOL Integral, UBOOL MultiSel, UBOOL OwnerDrawVariable, UBOOL Sort = 0, DWORD dwExtraStyle = 0  )
	{
		guard(WListBox::OpenWindow);
		m_bMultiSel = MultiSel;
		// stijn: UE1 originally used the LBS_OWNERDRAWVARIABLE style for the OwnerDrawVariable==1 case.
		// Unfortunately, the LBS_OWNERDRAWVARIABLE style has been totally bugged since at least Windows 2000.
		// When using the mouse wheel to scroll in an LBS_OWNERDRAWVARIABLE listbox, the scrolling animation appears to happen in the opposite direction.
		// This is extremely confusing and annoying so I changed this to LBS_OWNERDRAWFIXED.
		// The only real side effect of this change is that we now have to provide a correct height for the list box items by responding to the WM_MEASUREITEM Windows message.
		PerformCreateWindowEx
		(
			WS_EX_CLIENTEDGE,
            NULL,
            WS_CHILD | WS_BORDER | WS_VSCROLL | WS_CLIPCHILDREN | LBS_NOTIFY | (Visible?WS_VISIBLE:0) | (Integral?0:LBS_NOINTEGRALHEIGHT) 
			| (MultiSel?(LBS_EXTENDEDSEL|LBS_MULTIPLESEL):0) | (OwnerDrawVariable?LBS_OWNERDRAWFIXED:0) | (Sort?LBS_SORT:0) | dwExtraStyle,
            0, 0,
			0, 0,
            *OwnerWindow,
            (HMENU)ControlId,
            hInstance
		);

		SendMessageW( *this, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(0,0) );
		unguard;
	}
	
	LRESULT WndProc( UINT Message, WPARAM wParam, LPARAM lParam )
	{
		guard(WListBox::WndProc);
		if ( m_bMultiSel && Message == WM_CHAR && wParam + 0x40 == 'A' && HIWORD(GetKeyState(VK_CONTROL)) )
		{
			SendMessageW( *this, LB_SELITEMRANGE, TRUE, MAKELPARAM(0, GetCount() - 1) );
			return 1;
		}
		else return WControl::WndProc( Message, wParam, lParam );
		unguard;
	}

	// WControl interface.
	UBOOL InterceptControlCommand( UINT Message, WPARAM wParam, LPARAM lParam)
	{
		guard(WListBox::InterceptControlCommand);
		if     ( HIWORD(wParam)==LBN_DBLCLK   ) {DoubleClickDelegate();     return 1;}
		else if( HIWORD(wParam)==LBN_SELCHANGE) {SelectionChangeDelegate(); return 1;}
		else if( HIWORD(wParam)==LBN_SELCANCEL) {SelectionCancelDelegate(); return 1;}
		else if( HIWORD(wParam)==LBN_SETFOCUS)  {SetFocusDelegate();        return 1;}
		else if( HIWORD(wParam)==LBN_KILLFOCUS) {KillFocusDelegate();       return 1;}		
		else return 0;
		unguard;
	}

	// WListBox interface.
	FString GetString( INT Index )
	{
		guard(WListBox::GetText);
		INT Length = SendMessageW(*this,LB_GETTEXTLEN,Index,0);
		if (Length == -1)
			return TEXT("");

		TArray<TCHAR> Buf(Length + 1);
		SendMessageW( *this, LB_GETTEXT, Index, (LPARAM)&Buf(0) );
		FString ret(&Buf(0));
		return ret;
		unguard;
	}
	void* GetItemData( INT Index )
	{
		guard(WListBox::GetItemData);
		return (void*)SendMessageW( *this, LB_GETITEMDATA, Index, 0 );
		unguard;
	}
	void SetItemData( INT Index, void* Value )
	{
		guard(WListBox::SetItemData);
		SendMessageW( *this, LB_SETITEMDATA, Index, (LPARAM)Value );
		unguard;
	}
	void SetItemData( INT Index, INT Value )
	{
		guard(WListBox::SetItemData);
		SendMessageW( *this, LB_SETITEMDATA, Index, (LPARAM)Value );
		unguard;
	}
	INT GetCurrent()
	{
		guard(WListBox::GetCurrent);
		return SendMessageW( *this, LB_GETCARETINDEX, 0, 0 );
		unguard;
	}
	void ClearSel()
	{
		guard(WListBox::ClearSel);
		SendMessageW( *this, LB_SETSEL, FALSE, -1 );
		unguard;
	}
	void SetCurrent( INT Index, UBOOL bScrollIntoView )
	{
		guard(WListBox::SetCurrent);
		if( m_bMultiSel )
		{
			ClearSel();
			SendMessageW( *this, LB_SETSEL, TRUE, Index );
			SendMessageW( *this, LB_SETCARETINDEX, Index, bScrollIntoView );
		}
		else
		{
			SendMessageW( *this, LB_SETCURSEL, Index, 0 );
			SendMessageW( *this, LB_SETCARETINDEX, Index, bScrollIntoView );
		}
		unguard;
	}
	INT GetTop()
	{
		guard(WListBox::GetTop);
		return SendMessageW( *this, LB_GETTOPINDEX, 0, 0 );
		unguard;
	}
	void SetTop( INT Index )
	{
		guard(WListBox::SetTop);
		SendMessageW( *this, LB_SETTOPINDEX, Index, 0 );
		unguard;
	}
	void DeleteString( INT Index )
	{
		guard(WListBox::DeleteItem);
		SendMessageW( *this, LB_DELETESTRING, Index, 0 );
		unguard;
	}
	INT GetCount()
	{
		guard(WListBox::GetCount);
		return SendMessageW( *this, LB_GETCOUNT, 0, 0 );
		unguard;
	}
	INT GetItemHeight( INT Index )
	{
		guard(WListBox::GetItemHeight);
		return SendMessageW( *this, LB_GETITEMHEIGHT, Index, 0 );
		unguard;
	}
	void SetItemHeight(INT Height)
	{
		SendMessageW(*this, LB_SETITEMHEIGHT, 0, Height);
	}
	INT ItemFromPoint( FPoint P )
	{
		guard(WListBox::ItemFromPoint);
		DWORD Result=SendMessageW( *this, LB_ITEMFROMPOINT, 0, MAKELPARAM(P.X,P.Y) );
		return HIWORD(Result) ? -1 : LOWORD(Result);
		unguard;
	}
	FRect GetItemRect( INT Index )
	{
		guard(WListBox::GetItemRect);
		RECT R; R.left=R.right=R.top=R.bottom=0;
		SendMessageW( *this, LB_GETITEMRECT, Index, (LPARAM)&R );
		return R;
		unguard;
	}
	void Empty()
	{
		guard(WListBox::Empty);
		SendMessageW( *this, LB_RESETCONTENT, 0, 0 );
		unguard;
	}
	UBOOL GetSelected( INT Index )
	{
		guard(WListBox::GetSelected);
		return SendMessageW( *this, LB_GETSEL, Index, 0 );
		unguard;
	}
	INT GetSelectedItems( INT Count, INT* Buffer )
	{
		guard(WListBox::GetSelectedItems);
		return SendMessageW( *this, LB_GETSELITEMS, (WPARAM)Count, (LPARAM)Buffer );
		unguard;
	}
	INT GetSelectedCount()
	{
		guard(WListBox::GetSelectedCount);
		return SendMessageW( *this, LB_GETSELCOUNT, 0, 0 );
		unguard;
	}
	void AutoResizeColumns()
	{
		guard(WListBox::AutoResizeColumns);
		INT ItemCount = GetCount();
		INT MaxColumnWidth = 0;
		HDC dc = GetDC(*this);
		for (INT i = 0; i < ItemCount; ++i)
		{
			FString Text = GetString(i);
			if (Text.Len() > 0)
			{
				SIZE Size;
				GetTextExtentPoint32W(dc, *Text, Text.Len(), &Size);
				if (Size.cx > MaxColumnWidth)
					MaxColumnWidth = Size.cx;
			}
		}
		if (MaxColumnWidth > 0)
			SendMessageW(*this, LB_SETCOLUMNWIDTH, (WPARAM)MaxColumnWidth, 0);
		unguard;
	}

	// Accessing as strings.
	INT AddString( const TCHAR* C )
	{
		guard(WListBox::AddString);
		return SendMessageW( *this, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(C) );		
		unguard;
	}
	void InsertString( INT Index, const TCHAR* C )
	{
		guard(WListBox::InsertString);
		SendMessageW( *this, LB_INSERTSTRING, Index, reinterpret_cast<LPARAM>(C) );
		unguard;
	}
	INT FindStringExact( const TCHAR* C )
	{
		guard(WListBox::FindStringExact);
		return SendMessageW( *this, LB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(C));
		unguard;
	}
	INT FindString( const TCHAR* C )
	{
		guard(WListBox::FindString);
		return SendMessageW( *this, LB_FINDSTRING, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(C));
		unguard;
	}
	INT FindStringChecked( const TCHAR* C )
	{
		guard(WListBox::FindStringChecked);
		INT Result = SendMessageW( *this, LB_FINDSTRING, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(C));
		check(Result!=LB_ERR);
		return Result;
		unguard;
	}
	void InsertStringAfter( const TCHAR* Existing, const TCHAR* New )
	{
		guard(WListBox::InsertStringAfter);
		InsertString( FindStringChecked(Existing)+1, New );
		unguard;
	}

	// Accessing as pointers.
	INT AddItem( const void* C )
	{
		guard(WListBox::AddItem);
		return SendMessageW( *this, LB_ADDSTRING, 0, (LPARAM)C );
		unguard;
	}
	void InsertItem( INT Index, const void* C )
	{
		guard(WListBox::AddInsert);
		SendMessageW( *this, LB_INSERTSTRING, Index, (LPARAM)C );
		unguard;
	}
	INT FindItem( const void* C )
	{
		guard(WListBox::FindItem);
		return SendMessageW( *this, LB_FINDSTRING, static_cast<WPARAM>(-1), (LPARAM)C );
		unguard;
	}
	INT FindItemChecked( const void* C )
	{
		guard(WListBox::FindItemChecked);
		INT Result = SendMessageW( *this, LB_FINDSTRING, static_cast<WPARAM>(-1), (LPARAM)C );
		check(Result!=LB_ERR);
		return Result;
		unguard;
	}
	void InsertItemAfter( const void* Existing, const void* New )
	{
		guard(WListBox::InsertItemAfter);
		InsertItem( FindItemChecked(Existing)+1, New );
		unguard;
	}
	virtual void OnCommand(INT Command)
	{
		CommandDelegate(Command);
		WWindow::OnCommand(Command);
	}
	virtual void OnRightButtonDown(INT X, INT Y)
	{
		guard(WListBox::OnRightButtonDown);
		FPoint P(X, Y);
		INT Index = ItemFromPoint(P);
		if (Index != -1)
		{
			SetCurrent(Index, false);
			PostMessageW( OwnerWindow->hWnd, WM_COMMAND, WM_TREEVIEW_RIGHT_CLICK, 0 );
		}
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	WCheckListBox.
-----------------------------------------------------------------------------*/

// A list box where each item has a checkbox beside it.
class WINDOW_API WCheckListBox : public WListBox
{
	W_DECLARE_CLASS(WCheckListBox,WListBox,CLASS_Transient);
	DECLARE_WINDOWSUBCLASS(WCheckListBox,WListBox,Window)

	HBITMAP hbmOff{}, hbmOn{}, hbmIndeterminate{};
	int bOn{};
	BOOL ThreeState{};

	FDelegateInt ItemModifiedDelegate;

	// Constructor.
	WCheckListBox()	= default;

	WCheckListBox( WWindow* InOwner, INT InId=0, WNDPROC InSuperProc=NULL, UBOOL _ThreeState=FALSE )
		: WListBox( InOwner, InId )
	{
		check(OwnerWindow);
		hbmOff = (HBITMAP)LoadImageW( hInstance, MAKEINTRESOURCEW(IDBM_CHECKBOX_OFF), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS );	check(hbmOff);
		hbmOn = (HBITMAP)LoadImageW( hInstance, MAKEINTRESOURCEW(IDBM_CHECKBOX_ON), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS );	check(hbmOn);		
		ScaleImageAndReplace(hbmOff);
		ScaleImageAndReplace(hbmOn);

		if (_ThreeState)
		{
			hbmIndeterminate = (HBITMAP)LoadImageW(hInstance, MAKEINTRESOURCEW(IDBM_CHECKBOX_INDETERMINATE), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);	check(hbmIndeterminate);
			ScaleImageAndReplace(hbmIndeterminate);
			ThreeState = _ThreeState;
		}
		bOn = 0;
	}

	// WWindow interface.
	void OpenWindow( UBOOL Visible, UBOOL Integral, UBOOL MultiSel, UBOOL Sort = FALSE, DWORD dwExtraStyle = 0 )
	{
		guard(WCheckListBox::OpenWindow);
		m_bMultiSel = MultiSel;
		PerformCreateWindowEx
		(
			WS_EX_CLIENTEDGE,
            NULL,
            WS_CHILD | WS_BORDER | WS_VSCROLL | WS_CLIPCHILDREN | LBS_NOTIFY | (Visible?WS_VISIBLE:0) | (Integral?0:LBS_NOINTEGRALHEIGHT) 
				| (MultiSel?(LBS_EXTENDEDSEL|LBS_MULTIPLESEL):0) | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | (Sort?LBS_SORT:0) | dwExtraStyle,
            0, 0,
			0, 0,
            *OwnerWindow,
            (HMENU)ControlId,
            hInstance
		);

		BITMAP bm;
		GetObject(hbmOn, sizeof(bm), &bm);
		SetItemHeight(bm.bmHeight + 6);

		SendMessageW( *this, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(0,0) );
		unguard;
	}
	void OnDestroy()
	{
		guard(WCheckListBox::OnDestroy);
		WListBox::OnDestroy();
		DeleteObject(hbmOff);
		DeleteObject(hbmOn);
		if (ThreeState)
			DeleteObject(hbmIndeterminate);
		unguard;
	}
	void OnDrawItem( DRAWITEMSTRUCT* Item )
	{
		guard(WCheckListBox::OnDrawItem);

		if(((LONG)(Item->itemID) >= 0)
			&& (Item->itemAction & (ODA_DRAWENTIRE | ODA_SELECT)))
		{
			BOOL fDisabled = !IsWindowEnabled(hWnd);

			COLORREF newTextColor = fDisabled ? RGB(0x80, 0x80, 0x80) : GetSysColor(COLOR_WINDOWTEXT);  // light gray
			COLORREF oldTextColor = SetTextColor(Item->hDC, newTextColor);

			COLORREF newBkColor = GetSysColor(COLOR_WINDOW);
			COLORREF oldBkColor = SetBkColor(Item->hDC, newBkColor);

			if (newTextColor == newBkColor)
				newTextColor = RGB(0xC0, 0xC0, 0xC0);   // dark gray

			if (!fDisabled && ((Item->itemState & ODS_SELECTED) != 0))
			{
				SetTextColor(Item->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
				SetBkColor(Item->hDC, GetSysColor(COLOR_HIGHLIGHT));
			}

			FString strText = GetString(Item->itemID);
			ExtTextOutW(Item->hDC, Item->rcItem.left + MulDiv(18, DPIX, 96),
				Item->rcItem.top + MulDiv(2, DPIY, 96),
				ETO_OPAQUE, &(Item->rcItem), *strText, strText.Len(), NULL);

			SetTextColor(Item->hDC, oldTextColor);
			SetBkColor(Item->hDC, oldBkColor);

			// BITMAP
			//
			HDC hdcMem = CreateCompatibleDC(Item->hDC);
			PTRINT OldState = reinterpret_cast<PTRINT>(GetItemData(Item->itemID));
			HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, OldState == 1 ? hbmOn : OldState == 0 ? hbmOff : hbmIndeterminate);

			BitBlt(Item->hDC,
				Item->rcItem.left + MulDiv(2, DPIX, 96), Item->rcItem.top + MulDiv(3, DPIY, 96),
				MulDiv(13, DPIX, 96), MulDiv(13, DPIY, 96),
				hdcMem,
				0, 0,
				SRCCOPY);

			SelectObject(hdcMem, hbmOld);
			DeleteDC(hdcMem);
		}

		if((Item->itemAction & ODA_FOCUS) != 0)
			DrawFocusRect(Item->hDC, &(Item->rcItem));
		unguard;
	}
	void OnLeftButtonDown(INT X, INT Y)
	{
		guard(WCheckListBox::OnLeftButtonDown);
		WListBox::OnLeftButtonDown(X, Y);

		FPoint pt;
		pt.X = X;
		pt.Y = Y;

		if( X <= MulDiv(16, DPIX, 96))
		{
			int Item = ItemFromPoint( pt );
			PTRINT State = reinterpret_cast<PTRINT>(GetItemData(Item));
			if (ThreeState && State == 2)
				State = 0;
			else
				State = (State + 1) % 2;			
			SetItemData( Item, State );
			ItemModifiedDelegate(Item);
		}
		
		check(OwnerWindow);
		PostMessageW(OwnerWindow->hWnd, WM_COMMAND, WM_WCLB_UPDATE_VISIBILITY, 0L);
		InvalidateRect(hWnd, NULL, FALSE);
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	FTreeItemBase.
-----------------------------------------------------------------------------*/

class WINDOW_API FTreeItemBase : public FCommandTarget, public FControlSnoop
{
public:
	virtual void Draw( HDC hDC )=0;
	virtual INT GetHeight()=0;
	virtual void SetSelected( UBOOL NewSelected )=0;
};

/*-----------------------------------------------------------------------------
	WItemBox.
-----------------------------------------------------------------------------*/

// A list box contaning list items.
class WINDOW_API WItemBox : public WListBox
{
	W_DECLARE_CLASS(WItemBox, WListBox, CLASS_Transient);
	DECLARE_WINDOWCLASS(WItemBox, WListBox, Window)

	INT itemHeight = MulDiv(16, DPIY, 96);

	// Constructors.
	WItemBox() = default;

	WItemBox(WWindow* InOwner, INT InId = 0)
		: WListBox(InOwner, InId)
	{
		check(OwnerWindow);
	}

	// WWindow interface.
	virtual LRESULT WndProc(UINT Message, WPARAM wParam, LPARAM lParam);
	void OnDrawItem(DRAWITEMSTRUCT* Info)
	{
		if (Info->itemData)
		{
			((FTreeItemBase*)Info->itemData)->SetSelected((Info->itemState & ODS_SELECTED) != 0);
			((FTreeItemBase*)Info->itemData)->Draw(Info->hDC);
		}
	}
	void OnMeasureItem(MEASUREITEMSTRUCT* Info)
	{
		if (Info->itemData)
			itemHeight = ((FTreeItemBase*)Info->itemData)->GetHeight();
		Info->itemHeight = itemHeight;
	}
	void OnVScroll(WPARAM wParam, LPARAM lParam)
	{
		InvalidateRect(hWnd, NULL, FALSE);
	}
	void OnSize(DWORD Flags, INT NewX, INT NewY)
	{
		InvalidateRect(hWnd, NULL, FALSE);
	}
};

/*-----------------------------------------------------------------------------
	WListView.
-----------------------------------------------------------------------------

// A list view.
class WINDOW_API WListView : public WControl
{
	W_DECLARE_CLASS(WListView,WControl,CLASS_Transient)
	DECLARE_WINDOWSUBCLASS(WListView,WControl,Window)

	FDelegate DblClkDelegate;
	FDelegate SelChangedDelegate;

	// Constructor.
	WListView()
	{}
	WListView( WWindow* InOwner, INT InId=0, WNDPROC InSuperProc=NULL )
	: WControl( InOwner, InId, InSuperProc?InSuperProc:SuperProc )
	{}


	void OpenWindow( UBOOL Visible, DWORD dwExtraStyle = 0 )
	{
		guard(WListView::OpenWindow);
		PerformCreateWindowEx
		(
			WS_EX_CLIENTEDGE,
			NULL,
			WS_CHILD | (Visible?WS_VISIBLE:0) | dwExtraStyle,
			0, 0,
			0, 0,
			*OwnerWindow,
			(HMENU)ControlId,
			hInstance
		);

		SendMessageX( *this, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(0,0) );
		unguard;
	}

	void Empty()
	{
		guard(WListView::Empty);
		SendMessageX(hWnd, LVM_DELETEALLITEMS, 0, 0L);
		unguard;
	}

	UBOOL InterceptControlCommand( UINT Message, WPARAM wParam, LPARAM lParam )
	{
		guard(WListView::InterceptControlCommand);

		LastwParam = wParam;
		LastlParam = lParam;

		NMTREEVIEW* pntv = (LPNMTREEVIEW)lParam;

		if( pntv->hdr.code==NM_DBLCLK )
		{
			DblClkDelegate();
			return 1;
		}
		if( pntv->hdr.code==LVN_ITEMCHANGED )
			SelChangedDelegate();

		return 0;
		unguard;
	}


	INT GetCurrent()
	{
		guard(WListView::GetCurrent);
		return SendMessageX( hWnd, LVM_GETNEXTITEM, -1, LVNI_ALL | LVNI_SELECTED | LVNI_FOCUSED );
		unguard;
	}
};
*/


/*-----------------------------------------------------------------------------
	WRichEdit.
-----------------------------------------------------------------------------*/

// A multiline edit control for edit unreal script code.
class WINDOW_API WRichEdit : public WControl
{
	W_DECLARE_CLASS(WRichEdit,WControl,CLASS_Transient);
	DECLARE_WINDOWSUBCLASS(WRichEdit,WControl,Window)

	// Variables.
	FDelegate ChangeDelegate;
	
	INT LinesBarWidth = MulDiv(45, DPIX, 96);

	WListBox ChoicesList;
	INT WordLen = -1;
	WPARAM IgnoreHotkeyDown = 0;
	WPARAM IgnoreHotkeyChar = 0;

	INT FontSize = 0;
	HFONT LoadedFont = NULL;
	UBOOL bReadOnly = FALSE;

	// Constructor.
	WRichEdit()	= default;

	WRichEdit( WWindow* InOwner, INT InId=0, WNDPROC InSuperProc=NULL )
	: WControl( InOwner, InId, InSuperProc?InSuperProc:SuperProc )
	, ChoicesList( WListBox(this))
	{
		ChoicesList.DoubleClickDelegate=FDelegate(this,(TDelegate)&WRichEdit::AutocompleteSelect);
	}
	
	virtual LRESULT WndProc(UINT Message, WPARAM wParam, LPARAM lParam);
	void Autocomplete();
	void AutocompleteSelect();	
	void AutocompleteHide();
	
	// WWindow interface.
	void OpenWindow(UBOOL Visible, UBOOL ReadOnly);
	UBOOL InterceptControlCommand(UINT Message, WPARAM wParam, LPARAM lParam);

	// WRichEdit interface.
	void SetTextLimit(int _Limit);
	void SetReadOnly(UBOOL ReadOnly);
	void SetFontSize(INT FontSize);
	FString GetStatus(INT* LastLine);
};

/*-----------------------------------------------------------------------------
	WPropertiesBase.
-----------------------------------------------------------------------------*/

class WINDOW_API WPropertiesBase : public WWindow, public FControlSnoop
{
	W_DECLARE_ABSTRACT_CLASS(WPropertiesBase,WWindow,CLASS_Transient);

	// Variables.
	UBOOL ShowTreeLines{};
	WItemBox List;
	class FTreeItem* FocusItem{};

	// Structors.
	WPropertiesBase() = default;

	WPropertiesBase( FName InPersistentName, WWindow* InOwnerWindow )
	:	WWindow			( InPersistentName, InOwnerWindow )
	,	List			( this )
	,	FocusItem		( NULL )
	,	ShowTreeLines	( 1 )
	{
		List.Snoop = this;
	}

	// WPropertiesBase interface.
	virtual FTreeItem* GetRoot()=0;
	virtual INT GetDividerWidth()=0;
	virtual void ResizeList()=0;
	virtual void SetItemFocus( UBOOL FocusCurrent )=0;
	virtual void ForceRefresh()=0;
	virtual void BeginSplitterDrag()=0;
	virtual class FTreeItem* GetListItem( INT i )
	{
		guard(WProperties::GetListItem);
		FTreeItem* Result = (FTreeItem*)List.GetItemData(i);
		check(Result);
		return Result;
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	WDragInterceptor.
-----------------------------------------------------------------------------*/

// Splitter drag handler.
class WINDOW_API WDragInterceptor : public WWindow
{
	W_DECLARE_CLASS(WDragInterceptor,WWindow,CLASS_Transient);
	DECLARE_WINDOWCLASS(WDragInterceptor,WWindow,Window)

	// Variables.
	FPoint		OldMouseLocation{};
	FPoint		DragIndices{};
	FPoint		DragPos{};
	FPoint		DragStart{};
	FPoint		DrawWidth{};
	FRect		DragClamp{};
	UBOOL		Success{};

	// Constructor.
	WDragInterceptor() = default;

	WDragInterceptor( WWindow* InOwner, FPoint InDragIndices, FRect InDragClamp, FPoint InDrawWidth )
	:	WWindow			( NAME_None, InOwner )
	,	DragIndices		( InDragIndices )
	,	DragPos			( FPoint::ZeroValue() )
	,	DragClamp		( InDragClamp )
	,	DrawWidth		( InDrawWidth )
	,	Success			( 1 )
	{}

	// Functions.
	virtual void OpenWindow()
	{
		guard(WDragInterceptor::OpenWindow);
		PerformCreateWindowEx( 0, NULL, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, *OwnerWindow, NULL, hInstance );
		OldMouseLocation = OwnerWindow->GetCursorPos();
		DragStart = DragPos;
		SetCapture( *this );
		SetFocus( *this );
		ClipCursor( ClientToScreen(DragClamp-DragPos+OwnerWindow->GetCursorPos()) );
		ToggleDraw( NULL );
		unguard;
	}
	virtual void ToggleDraw( HDC hInDC )
	{
		guard(WDragInterceptor::ToggleDraw);

		HDC hDC = hInDC ? hInDC : GetDC(*OwnerWindow);
		HBRUSH OldBrush = (HBRUSH)SelectObject( hDC, hBrushStipple );
		if( DragIndices.X!=INDEX_NONE )
			PatBlt( hDC, DragPos.X, 0, DrawWidth.X, OwnerWindow->GetClientRect().Height(), PATINVERT );
		if( DragIndices.Y!=INDEX_NONE )
			PatBlt( hDC, 0, DragPos.Y, OwnerWindow->GetClientRect().Width(), DrawWidth.Y, PATINVERT );
		if( !hInDC )
			ReleaseDC( hWnd, hDC );
		SelectObject( hDC, OldBrush );

		unguard;
	}
	void OnKeyDown( TCHAR Ch )
	{
		if( Ch==VK_ESCAPE )
		{
			Success = 0;
			ReleaseCapture();
		}
	}
	void OnMouseMove( DWORD Flags, FPoint MouseLocation )
	{
		guard(WDragInterceptor::OnMouseMove);
		ToggleDraw( NULL );
		for( INT i=0; i<FPoint::Num(); i++ )
			if( DragIndices(i)!=INDEX_NONE )
				DragPos(i) = Clamp( DragPos(i) + MouseLocation(i) - OldMouseLocation(i), DragClamp.Min(i), DragClamp.Max(i) );
		ToggleDraw( NULL );
		OldMouseLocation = MouseLocation;
		unguard;
	}
	void OnReleaseCapture()
	{
		guard(WDragInterceptor::OnReleaseCapture);
		ClipCursor( NULL );
		ToggleDraw( NULL );
		OwnerWindow->OnFinishSplitterDrag( this, Success );
		DestroyWindow( *this );
		unguard;
	}
	void OnLeftButtonUp()
	{
		guard(WDragInterceptor::OnLeftButtonUp);
		ReleaseCapture();
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	FTreeItem.
-----------------------------------------------------------------------------*/

// QSort comparator.
static INT Compare( const class FTreeItem* T1, const class FTreeItem* T2 );

// Base class of list items.
class WINDOW_API FTreeItem : public FTreeItemBase
{
public:
	// Variables.
	class WPropertiesBase*	OwnerProperties{};
	FTreeItem*				Parent{};
	UBOOL					Expandable{};
	UBOOL					Expanded{};
	UBOOL					Sorted{};
	UBOOL					Selected{};
	INT						ButtonWidth{};
	TArray<WCoolButton*>	Buttons;
	TArray<FTreeItem*>		Children;
	static TArray<FTreeItem*> DeleteItems;

	// Structors.
	FTreeItem()	= default;

	FTreeItem( WPropertiesBase* InOwnerProperties, FTreeItem* InParent, UBOOL InExpandable )
	:	OwnerProperties	( InOwnerProperties )
	,	Parent			( InParent )
	,	Expandable		( InExpandable )
	,	Sorted			( 1 )
{}
	virtual ~FTreeItem()
	{
		guard(FTreeItem::~FTreeItem);
		EmptyChildren();
		DeleteItems.RemoveItem(this);
		unguard;
	}

	// FTreeItem interface.
	virtual HBRUSH GetBackgroundBrush( UBOOL Selected )
	{
		guard(FPropertyItem::GetBackgroundBrush);
		return Selected ? hBrushCurrent : hBrushOffWhite;
		unguard;
	}
	virtual COLORREF GetTextColor( UBOOL Selected )
	{
		guard(FPropertyItem::GetTextColor);
		return Selected ? RGB(255,255,255) : RGB(0,0,0);
		unguard;
	}
	virtual void Serialize( FArchive& Ar )
	{
		guard(FTreeItem::Serialize);
		//!!Super::Serialize( Ar );
		for( INT i=0; i<Children.Num(); i++ )
			Children(i)->Serialize( Ar );
		unguard;
	}
	virtual INT OnSetCursor()
	{
		guard(FTreeItem::OnSetCursor);
		return 0;
		unguard;
	}
	void EmptyChildren()
	{
		guard(FTreeItem::EmptyChildren);
		for (INT i = 0; i < Children.Num(); i++)			
			Children(i)->DelayedDestroy();
		Children.Empty();
		unguard;
	}
	virtual void DelayedDestroy()
	{
		DeleteItems.AddUniqueItem(this);
	}
	virtual FRect GetRect()
	{
		guard(FTreeItem::GetRect);
		return OwnerProperties->List.GetItemRect(OwnerProperties->List.FindItemChecked(this));
		unguard;
	}
	virtual void Redraw()
	{
		guard(FTreeItem::Redraw);
		InvalidateRect( OwnerProperties->List, GetRect(), 0 );
		UpdateWindow( OwnerProperties->List );
		if( Parent!=OwnerProperties->GetRoot() )
			Parent->Redraw();
		unguard;
	}
	virtual void OnItemSetFocus()
	{
		guard(FTreeItem::OnItemSetFocus);
		if( Parent && Parent!=OwnerProperties->GetRoot() )
			Parent->OnItemSetFocus();
		unguard;
	}
	virtual void OnItemKillFocus( UBOOL Abort )
	{
		guard(FTreeItem::OnItemKillFocus);
		for (INT i = 0; i < Buttons.Num(); i++)
		{
			Buttons(i)->Show(0);
			Buttons(i)->DelayedDestroy();
		}
		Buttons.Empty();
		ButtonWidth = 0;
		Redraw();
		if( Parent && Parent!=OwnerProperties->GetRoot() )
			Parent->OnItemKillFocus( Abort );
		unguard;
	}
	virtual void AddButton( const TCHAR* Text, FDelegate Action )
	{
		guard(FTreeItem::AddButton);
		FRect Rect=GetRect();
		Rect.Max.X -= ButtonWidth;
		SIZE Size;
		HDC hDC = GetDC(*OwnerProperties);
		GetTextExtentPoint32W( hDC, Text, appStrlen(Text), &Size ); 
		ReleaseDC(*OwnerProperties,hDC);
		INT Width = Size.cx + 2;
		WCoolButton* Button = new WCoolButton(&OwnerProperties->List,0,Action);
		Buttons.AddItem( Button );
		Button->OpenWindow( 1, Rect.Max.X-Width, Rect.Min.Y, Width, Rect.Max.Y-Rect.Min.Y, Text );
		ButtonWidth += Width;
		unguard;
	}
	virtual void OnItemLeftMouseDown( FPoint P )
	{
		guard(FTreeItem::OnItemLeftMouseDown);
		if( Expandable && GetExpanderRect().Inner(FPoint(-1,-1)).Contains(P) )
			ToggleExpansion();
		unguard;
	}
	virtual void OnItemRightMouseDown( FPoint P )
	{
		guard(FTreeItem::OnItemRightMouseDown);
		unguard;
	}
	INT GetIndent()
	{
		guard(FTreeItem::GetIndent);
		INT Result=0;
		for( FTreeItem* Test=Parent; Test!=OwnerProperties->GetRoot(); Test=Test->Parent )
			Result++;
		return Result;
		unguard;
	}
	INT GetUnitIndentPixels()
	{
		return MulDiv(OwnerProperties->ShowTreeLines ? 12 : 8, WWindow::DPIX, 96);
	}
	virtual INT GetIndentPixels( UBOOL Text )
	{
		guard(FTreeItem::GetIndentPixels);
		return GetUnitIndentPixels()*GetIndent() + MulDiv((Text?OwnerProperties->ShowTreeLines?20:16:0), WWindow::DPIX, 96);
		unguard;
	}
	virtual FRect GetExpanderRect()
	{
		guard(FTreeItem::GetExpanderRect);
		return FRect( 
			GetIndentPixels(0) + MulDiv(4, WWindow::DPIX, 96),
			MulDiv(4, WWindow::DPIY, 96), 
			GetIndentPixels(0) + MulDiv(13, WWindow::DPIX, 96),
			MulDiv(13, WWindow::DPIY, 96) );
		unguard;
	}
	virtual UBOOL GetSelected()
	{
		guard(FTreeItem::GetSelected);
		return Selected;
		unguard;
	}
	void SetSelected( UBOOL InSelected )
	{
		guard(FTreeItem::SetSelected);
		Selected = InSelected;
		unguard;
	}
	virtual void DrawTreeLines( HDC hDC, FRect Rect )
	{
		guard(FTreeItem::Draw);
		SetBkMode( hDC, TRANSPARENT );
		SetTextColor( hDC, RGB(0,0,0) );
		if( OwnerProperties->ShowTreeLines )
		{
			FTreeItem* Prev = this;
			for( INT i=GetIndent(); i>=0; i-- )
			{
				check(Prev->Parent);
				UBOOL FromAbove = (Prev->Parent->Children.Last()!=Prev || Prev->Parent->Children.Last()==this) && (i!=0 || Prev->Parent->Children(0)!=this);
				UBOOL ToBelow   = (Prev->Parent->Children.Last()!=Prev);
				FPoint P( Rect.Min.X + GetUnitIndentPixels()*i, Rect.Min.Y );
				if( FromAbove || ToBelow )
					FillRect( hDC, 
						FRect(
							P+FPoint(MulDiv(8, WWindow::DPIX, 96),MulDiv(FromAbove?0:8, WWindow::DPIY, 96)),
							P+FPoint(MulDiv(9, WWindow::DPIX, 96),MulDiv(ToBelow?16:8, WWindow::DPIY, 96))), 
						hBrushStipple );
				if( i==GetIndent() )
					FillRect( hDC, 
						FRect(
							P+FPoint(MulDiv(8, WWindow::DPIX, 96),MulDiv(8, WWindow::DPIY, 96)),
							P+FPoint(MulDiv(20, WWindow::DPIX, 96),MulDiv(9,WWindow::DPIY, 96))), 
						hBrushStipple );
				Prev = Prev->Parent;
			}
			check(Prev->Parent==NULL);
		}
		if( Expandable )
		{
			FRect R = GetExpanderRect() + GetRect().Min;
			FillRect( hDC, R, hBrushGrey );
			FillRect( hDC, R.Inner(FPoint(MulDiv(1, WWindow::DPIX, 96), MulDiv(1, WWindow::DPIY, 96))), GetBackgroundBrush(0) );
			FillRect( hDC, R.Inner(FPoint(MulDiv(2, WWindow::DPIX, 96), MulDiv(4, WWindow::DPIY, 96))), hBrushBlack );
			if( !Expanded )
				FillRect( hDC, R.Inner(FPoint(MulDiv(4, WWindow::DPIX, 96), MulDiv(2, WWindow::DPIY, 96))), hBrushBlack );
		}
		unguard;
	}
	virtual void Collapse()
	{
		guard(FTreeItem::Collapse);
		OwnerProperties->SetItemFocus( 0 );
		INT Index = OwnerProperties->List.FindItemChecked( this );
		INT Count = OwnerProperties->List.GetCount();
		while( Index+1<Count )
		{
			FTreeItem* NextItem = OwnerProperties->GetListItem(Index+1);
			FTreeItem* Check;
			for( Check=NextItem->Parent; Check && Check!=this; Check=Check->Parent );
			if( !Check )
				break;
			NextItem->Expanded = 0;
			OwnerProperties->List.DeleteString( Index+1 );
			Count--;
		}
		Expanded=0;
		OwnerProperties->ResizeList();
		unguard;
	}
	virtual void Expand()
	{
		guard(FTreeItem::Expand);
		if( Sorted )
		{
			Sort( Children );
		}
		if( this==OwnerProperties->GetRoot() )
		{
			for( INT i=0; i<Children.Num(); i++ )
				OwnerProperties->List.AddItem( Children(i) );
		}
		else
		{
			for( INT i=Children.Num()-1; i>=0; i-- )
				OwnerProperties->List.InsertItemAfter( this, Children(i) );
		}
		OwnerProperties->SetItemFocus( 0 );
		OwnerProperties->ResizeList();
		Expanded=1;
		unguard;
	}
	virtual void ToggleExpansion()
	{
		guard(FTreeItem::ToggleExpansion);
		if( Expandable )
		{
			OwnerProperties->List.SetRedraw( 0 );
			if( Expanded )
				Collapse();
			else
				Expand();
			OwnerProperties->List.SetRedraw( 1 );
			UpdateWindow( OwnerProperties->List );
		}
		OwnerProperties->SetItemFocus( 1 );
		unguard;
	}
	virtual void OnItemDoubleClick()
	{
		guard(FTreeItem::OnItemDoubleClick);
		ToggleExpansion();
		unguard;
	}
	virtual BYTE* GetReadAddress(UProperty* Property, FTreeItem* Top, INT Index = INDEX_NONE)
	{
		guard(FTreeList::GetReadAddress);
		return Parent ? Parent->GetReadAddress(Property, Top, Index) : NULL;
		unguard;
	}
	BYTE* GetPropertyAddress(UProperty* Property, BYTE* BaseAddr)
	{
		guard(FTreeList::GetPropertyAddress);
		if (!BaseAddr)
			return NULL;
		UProperty* SupVal = Cast<UProperty>(Property->GetOuter());
		if (SupVal)
			BaseAddr = GetPropertyAddress(SupVal, BaseAddr);
		if (Property->IsA(UArrayProperty::StaticClass()))
			return (BYTE*)((FArray*)BaseAddr + Property->Offset)->GetData();
		return BaseAddr + Property->Offset;
		unguard;
	}
	virtual FArray* GetArrayAddress(INT Index = INDEX_NONE)
	{
		guard(FTreeList::GetArrayAddress);
		return NULL;
		unguard;
	}
	virtual void SetProperty( UProperty* Property, FTreeItem* Offset, const TCHAR* Value )
	{
		guard(FTreeList::SetProperty);
		if( Parent )
			Parent->SetProperty( Property, Offset, Value );
		unguard;
	}
	virtual void GetStates( TArray<FName>& States )
	{
		guard(FTreeList::GetStates);
		if( Parent ) Parent->GetStates( States );
		unguard;
	}
	virtual UBOOL AcceptFlags( DWORD InFlags )
	{
		guard(FTreeList::AcceptFlags);
		return Parent ? Parent->AcceptFlags( InFlags ) : 0;
		unguard;
	}
	virtual UProperty* GetParentProperty()
	{
		return Parent ? Parent->GetParentProperty() : NULL;
	}
	virtual QWORD GetId() const=0;
	virtual FString GetCaption() const=0;
	virtual FString GetDescription()
	{
		return FString();
	}
	virtual void OnItemHelp() {}
	virtual void SetFocusToItem() {}
	virtual void SetValue( const TCHAR* Value ) {}
	virtual void OnArrayEmpty() {}
	virtual INT GetItemCount() const
	{
		return Parent ? Parent->GetItemCount() : 1;
	}

	// FControlSnoop interface.
	void SnoopChar( WWindow* Src, INT Char )
	{
		guard(FTreeItem::SnoopChar);
		if( Char==13 && Expandable )
			ToggleExpansion();
		unguard;
	}
	void SnoopKeyDown( WWindow* Src, INT Char )
	{
		guard(FTreeItem::SnoopKeyDown);
		if( Char==VK_UP || Char==VK_DOWN )
			PostMessageW( OwnerProperties->List, WM_KEYDOWN, Char, 0 );
		if( Char==9 )
			PostMessageW( OwnerProperties->List, WM_KEYDOWN, (GetKeyState(16)&0x8000)?VK_UP:VK_DOWN, 0 );
		unguard;
	}
	virtual UObject* GetParentOuter()
	{
		guard(FTreeItem::GetParentOuter);
		return Parent ? Parent->GetParentOuter() : NULL;
		unguard;
	}
	virtual void SaveObjectConfig()
	{
		guard(FTreeItem::SaveObjectConfig);
		if (Parent) Parent->SaveObjectConfig();
		unguard;
	}
};

// QSort comparator.
static INT Compare( const class FTreeItem* T1, const class FTreeItem* T2 )
{
	return appStricmp( *T1->GetCaption(), *T2->GetCaption() );
}

// Property list item.
class WINDOW_API FPropertyItem : public FTreeItem
{
public:
	// Variables.
	UProperty*      Property;
	INT				Offset;
	INT				ArrayIndex;
	WEdit*			EditControl;
	WTrackBar*		TrackControl;
	WComboBox*		ComboControl;
	WLabel*			HolderControl;
	UBOOL			ComboChanged;
	FName			Name;
	static COLORREF acrCustClr[16];
	FString			Text;

	// Constructors.
	FPropertyItem( WPropertiesBase* InOwnerProperties, FTreeItem* InParent, UProperty* InProperty, FName InName, INT InOffset, INT InArrayIndex )
	:	FTreeItem	    ( InOwnerProperties, InParent, 0 )
	,	Property		( InProperty )
	,	Offset			( InOffset )
	,	ArrayIndex		( InArrayIndex )
	,	EditControl		( NULL )
	,	TrackControl	( NULL )
	,	ComboControl	( NULL )
	,	HolderControl	( NULL )
	,	ComboChanged	( 0 )
	,	Name			( InName )
	{
		guard(FPropertyItem::FPropertyItem);
		if ((Cast<UStructProperty>(InProperty)) || (Cast<UObjectProperty>(InProperty))
				|| (Cast<UArrayProperty>(InProperty) && GetArrayAddress())
				|| (InProperty->ArrayDim > 1 && InArrayIndex == -1)
				|| (Cast<UIntProperty>(InProperty) && appStricmp(*Property->GetFName(),TEXT("PolyFlags")) == 0))
			Expandable = 1;
		unguard;
		Text = TEXT("Hint:");
	}
	BYTE* GetReadAddress(UProperty* InProperty, FTreeItem* Top, INT Index = INDEX_NONE)
	{
		guard(FPropertyItem::GetReadAddress);
		if (!Parent)
			return NULL;
		BYTE* AdrV = Parent->GetReadAddress(InProperty, Top, Index);
		if (!AdrV)
			return NULL;
		AdrV += Offset;
		if (Property && Property->IsA(UArrayProperty::StaticClass()))
			return (BYTE*)((FArray*)AdrV)->GetData();
		return AdrV;
		unguard;
	}
	FArray* GetArrayAddress(INT Index = INDEX_NONE)
	{
		guard(FPropertyItem::GetArrayAddress);
		if (!Parent)
			return NULL;
		BYTE* AdrV = Parent->GetReadAddress(Property, this, Index);
		if (!AdrV)
			return NULL;
		AdrV += Offset;
		return (FArray*)AdrV;
		unguard;
	}
	UProperty* GetParentProperty()
	{
		if (Parent)
		{
			UProperty* P = Parent->GetParentProperty();
			return P ? P : Property;
		}
		return Property;
	}

	// FTreeItem interface.
	void Serialize( FArchive& Ar )
	{
		guard(FPropertyItem::Serialize);
		FTreeItem::Serialize( Ar );
		Ar << Property << Name;
		unguard;
	}
	QWORD GetId() const
	{
		guard(FPropertyItem::GetId);
		return Name.GetIndex() + ((QWORD)1<<32) + ((QWORD)(ArrayIndex + 1) << 47);
		unguard;
	}
	FString GetCaption() const
	{
		guard(FPropertyItem::GetCaption);
		return *Name;
		unguard;
	}
	virtual INT OnSetCursor()
	{
		guard(FPropertyItem::OnSetCursor);
		check(OwnerProperties);	// WDM
		FPoint P = OwnerProperties->GetCursorPos() - GetRect().Min;
		if( Abs(P.X-OwnerProperties->GetDividerWidth())<=2 )
		{
			SetCursor(LoadCursorW(hInstanceWindow,MAKEINTRESOURCEW(IDC_SplitWE)));
			return 1;
		}
		return 0;
		unguard;
	}
	virtual void OnItemLeftMouseDown( FPoint P )
	{
		guard(FPropertyItem::OnItemLeftMouseDown);
		P = OwnerProperties->GetCursorPos() - GetRect().Min;
		if( Abs(P.X-OwnerProperties->GetDividerWidth())<=2 )
		{
			OwnerProperties->BeginSplitterDrag();
			throw TEXT("NoRoute");
		}
		else FTreeItem::OnItemLeftMouseDown( P );
		unguard;
	}
	// stijn: deprecated and dangerous. DO NOT USE!!!
	void GetPropertyText(TCHAR* Str, BYTE* ReadValue )
	{
		guard(GetPropertyText);
		if( Cast<UClassProperty>(Property) && appStricmp(*Property->Category,TEXT("Drivers"))==0 )
		{
			// Class config.
			FString Path, Left, Right;
			GConfig->GetString( *FObjectPathName(Property->GetOwnerClass()), Property->GetName(), Path );
			if( Path.Split(TEXT("."),&Left,&Right) )
				appStrcpy( Str, Localize(*Right,TEXT("ClassCaption"),*Left) ); // stijn: mem safety NOT OK
			else
				appStrcpy( Str, Localize("Language","Language",TEXT("Core"),*Path) ); // stijn: mem safety NOT OK
		}
		else
		{
			// Regular property.
			Property->ExportText( 0, Str, ReadValue-Property->Offset, ReadValue-Property->Offset, PPF_Localized );
		}
		unguard;
	}
	void GetPropertyTextSafe(FString& Str, BYTE* ReadValue)
	{
		guard(GetPropertyTextSafe);
		if (Cast<UClassProperty>(Property) && appStricmp(*Property->Category, TEXT("Drivers")) == 0)
		{
			// Class config.
			FString Path, Left, Right;
			GConfig->GetString(*FObjectPathName(Property->GetOwnerClass()), Property->GetName(), Path);
			if (Path.Split(TEXT("."), &Left, &Right))
				Str = Localize(*Right, TEXT("ClassCaption"), *Left);
			else
				Str = Localize("Language", "Language", TEXT("Core"), *Path);
		}
		else
		{
			// Regular property.
			Property->ExportText(0, Str, ReadValue - Property->Offset, ReadValue - Property->Offset, PPF_Localized);
		}
		unguard;
	}
	void SetValue( const TCHAR* Value )
	{
		guard(FPropertyItem::SetValue);
		SetProperty( Property, this, Value );
		ReceiveFromControl();
		Redraw();
		unguard;
	}
	void Draw( HDC hDC )
	{
		guard(FPropertyItem::Draw);
		FRect Rect = GetRect();
		FString Str;

		// Draw background.
		FillRect( hDC, Rect, hBrushWhite ); 

		// Draw left background.
		FRect LeftRect=Rect;
		LeftRect.Max.X = OwnerProperties->GetDividerWidth();
		FillRect( hDC, LeftRect+FRect(0,1-GetSelected(),-1,0), GetBackgroundBrush(GetSelected()) );
		LeftRect.Min.X += GetIndentPixels(1);

		// Draw tree lines.
		DrawTreeLines( hDC, Rect );

		// Setup text.
		SetBkMode( hDC, TRANSPARENT );

		// Draw left text.
		if( ArrayIndex==-1 ) 
			Str = *Name ;
		else
			Str = FString::Printf( TEXT("[%i]"), ArrayIndex );

		SetTextColor( hDC, GetTextColor(GetSelected()) );
		DrawTextExW( hDC, const_cast<TCHAR*>(*Str), Str.Len(), LeftRect + FRect(0,1,0,0).DPIScaled(WWindow::DPIX, WWindow::DPIY), DT_END_ELLIPSIS | DT_LEFT | DT_SINGLELINE | DT_VCENTER, NULL );

		// Draw right background.
		FRect RightRect = Rect;
		RightRect.Min.X = OwnerProperties->GetDividerWidth();		
		FillRect( hDC, RightRect+FRect(0,1,0,0), GetBackgroundBrush(0) );

		// Draw right text.
		RightRect.Max.X -= ButtonWidth;
		BYTE* ReadValue = GetReadAddress(Property, this);
		SetTextColor( hDC, GetTextColor(0) );
		if( (Property->ArrayDim!=1 && ArrayIndex==-1) || Cast<UArrayProperty>(Property) )
		{
			// Array expander.
			FString Tmp = TEXT("...");
			DrawTextExW( hDC, const_cast<TCHAR*>(*Tmp), Tmp.Len(), RightRect + FRect(4,0,0,1).DPIScaled(WWindow::DPIX, WWindow::DPIY), DT_END_ELLIPSIS | DT_LEFT | DT_SINGLELINE | DT_VCENTER, NULL );
		}
		else if( ReadValue && Cast<UStructProperty>(Property) && Cast<UStructProperty>(Property)->Struct->GetFName()==NAME_Color )
		{
			// Color.
			FillRect( hDC, RightRect + FRect(4,4,-4,-4), hBrushBlack );
			DWORD D = (*(DWORD*)ReadValue); //!!
			DWORD TempColor = D & 0xffffff;
			HBRUSH hBrush = CreateSolidBrush(COLORREF(TempColor));
			FillRect( hDC, RightRect + FRect(5,5,-5,-5), hBrush );
			DeleteObject( hBrush );
		}
		else if( ReadValue )
		{
			// Text.
			Str = TEXT("");
			GetPropertyTextSafe( Str, ReadValue );
			DrawTextExW( hDC, const_cast<TCHAR*>(*Str), Str.Len(), RightRect + FRect(4,1,0,0).DPIScaled(WWindow::DPIX, WWindow::DPIY), DT_END_ELLIPSIS | DT_LEFT | DT_SINGLELINE | DT_VCENTER, NULL );
		}

 		unguard;
	}
	INT GetHeight()
	{
		guard(FPropertyItem::GetHeigth);
		return MulDiv(16, WWindow::DPIY, 96);
		unguard;
	}
	void SetFocusToItem()
	{
		guard(FPropertyItem::SetFocusToItem);
		if( EditControl )
			SetFocus( *EditControl );
		else if( TrackControl )
			SetFocus( *TrackControl );
		else if( ComboControl )
			SetFocus( *ComboControl );
		unguard;
	}
	void OnItemDoubleClick()
	{
		guard(FProperty::OnItemDoubleClick);
		if (!Property->IsA(UArrayProperty::StaticClass()))
			Advance();
		FTreeItem::OnItemDoubleClick();
		unguard;
	}
	void OnItemSetFocus()
	{
		guard(FPropertyItem::OnItemSetFocus);
		check(!EditControl);
		check(!TrackControl);
		FTreeItem::OnItemSetFocus();
		if
		(	(Property->ArrayDim==1 || ArrayIndex!=-1)
		&&	!(Property->PropertyFlags & CPF_EditConst) )
		{
			if( Property->IsA(UByteProperty::StaticClass()) && !Cast<UByteProperty>(Property)->Enum )
			{
				// Slider.
				FRect Rect = GetRect();
				Rect.Min.X = 28+OwnerProperties->GetDividerWidth();
				Rect.Min.Y--;
				TrackControl = new WTrackBar( &OwnerProperties->List );
				TrackControl->ThumbTrackDelegate    = FDelegate(this,(TDelegate)&FPropertyItem::OnTrackBarThumbTrack);
				TrackControl->ThumbPositionDelegate = FDelegate(this,(TDelegate)&FPropertyItem::OnTrackBarThumbPosition);
				TrackControl->OpenWindow( 0 );
				TrackControl->SetTicFreq( 32 );
				TrackControl->SetRange( 0, 255 );
				TrackControl->MoveWindow( Rect, 0 );
			}
			if
			(	(Property->IsA(UBoolProperty::StaticClass()))
			||	(Property->IsA(UByteProperty::StaticClass()) && Cast<UByteProperty>(Property)->Enum)
			||	(Property->IsA(UNameProperty::StaticClass()) && Name==NAME_InitialState)
			||  (Cast<UClassProperty>(Property) && appStricmp(*Property->Category,TEXT("Drivers"))==0) )
			{
				// Combo box.
				FRect Rect = GetRect() + FRect(0,0,-1,-1);
				Rect.Min.X = OwnerProperties->GetDividerWidth();

				HolderControl = new WLabel( &OwnerProperties->List );
				HolderControl->Snoop = this;
				HolderControl->OpenWindow( 0 );
				FRect HolderRect = Rect.Right(MulDiv(20, WWindow::DPIX, 96)) + FRect(0,0,0,1);
				HolderControl->MoveWindow( HolderRect, 0 );

				Rect = Rect + FRect(-2,-6,-2,0).DPIScaled(WWindow::DPIX, WWindow::DPIY);

				ComboControl = new WComboBox( HolderControl );
				ComboControl->Snoop = this;
				ComboControl->SelectionEndOkDelegate     = FDelegate(this,(TDelegate)&FPropertyItem::ComboSelectionEndOk);
				ComboControl->SelectionEndCancelDelegate = FDelegate(this,(TDelegate)&FPropertyItem::ComboSelectionEndCancel);
				ComboControl->OpenWindow( 0 );
				ComboControl->MoveWindow( Rect-HolderRect.Min, 1 );

				if( Property->IsA(UBoolProperty::StaticClass()) )
				{
					ComboControl->AddString( GFalse );
					ComboControl->AddString( GTrue );
				}
				else if( Property->IsA(UByteProperty::StaticClass()) )
				{
					for( INT i=0; i<Cast<UByteProperty>(Property)->Enum->Names.Num(); i++ )
						ComboControl->AddString( *Cast<UByteProperty>(Property)->Enum->Names(i) );
				}
				else if( Property->IsA(UNameProperty::StaticClass()) && Name==NAME_InitialState )
				{
					TArray<FName> States;
					GetStates( States );
					ComboControl->AddString( *FName(NAME_None) );
					for( INT i=0; i<States.Num(); i++ )
						ComboControl->AddString( *States(i) );
				}
				else if( Cast<UClassProperty>(Property) && appStricmp(*Property->Category,TEXT("Drivers"))==0 )
				{
					UClassProperty* ClassProp = CastChecked<UClassProperty>(Property);
					TArray<FRegistryObjectInfo> Classes;
					UObject::GetRegistryObjects( Classes, UClass::StaticClass(), ClassProp->MetaClass, 0 );
					for( INT i=0; i<Classes.Num(); i++ )
					{
						FString Path=Classes(i).Object, Left, Right;
						if( Path.Split(TEXT("."),&Left,&Right) )
							ComboControl->AddString( Localize(*Right,TEXT("ClassCaption"),*Left) );
						else
							ComboControl->AddString( Localize("Language","Language",TEXT("Core"),*Path) );
					}
					goto SkipTheRest;
				}
			}
			if( Property->IsA(UArrayProperty::StaticClass()) )
			{
				if( Expandable )
				{
					AddButton( LocalizeGeneral(TEXT("AddButton"), TEXT("Window")), FDelegate(this,(TDelegate)&FPropertyItem::OnArrayAdd) );
				}
				AddButton( LocalizeGeneral(TEXT("EmptyButton"), TEXT("Window")), FDelegate(this,(TDelegate)&FPropertyItem::OnArrayEmpty) );
			}
			if( Cast<UArrayProperty>(Property->GetOuter()) )
			{
				if( Parent->Expandable )
				{
					AddButton( LocalizeGeneral(TEXT("InsertButton"), TEXT("Window")), FDelegate(this,(TDelegate)&FPropertyItem::OnArrayInsert) );
					AddButton( LocalizeGeneral(TEXT("DeleteButton"), TEXT("Window")), FDelegate(this,(TDelegate)&FPropertyItem::OnArrayDelete) );
				}
			}
			if( Property->IsA(UStructProperty::StaticClass()) && appStricmp(Cast<UStructProperty>(Property)->Struct->GetName(),TEXT("Color"))==0 )
			{
				// Color.
				AddButton( LocalizeGeneral(TEXT("BrowseButton"), TEXT("Window")), FDelegate(this,(TDelegate)&FPropertyItem::OnChooseColorButton) );
			}
			else if( Property->IsA(UObjectProperty::StaticClass()) )
			{
				// Class.
				AddButton( LocalizeGeneral(TEXT("BrowseButton"), TEXT("Window")), FDelegate(this,(TDelegate)&FPropertyItem::OnBrowseButton) );
				AddButton( LocalizeGeneral(TEXT("UseButton"), TEXT("Window")), FDelegate(this,(TDelegate)&FPropertyItem::OnUseCurrentButton) );
				AddButton( LocalizeGeneral(TEXT("ClearButton"), TEXT("Window")), FDelegate(this,(TDelegate)&FPropertyItem::OnClearButton) );
			}
			if
			(	(Property->IsA(UFloatProperty ::StaticClass()))
			||	(Property->IsA(UIntProperty   ::StaticClass()))
			||	(Property->IsA(UNameProperty  ::StaticClass()) && Name!=NAME_InitialState)
			||	(Property->IsA(UStrProperty   ::StaticClass()))
			||	(Property->IsA(UObjectProperty::StaticClass()))
			||	(Property->IsA(UByteProperty  ::StaticClass()) && Cast<UByteProperty>(Property)->Enum==NULL) )
			{
				// Edit control.
				FRect Rect = GetRect();
				Rect.Min.X = 1+OwnerProperties->GetDividerWidth();
				Rect.Min.Y--;
				if( Property->IsA(UByteProperty::StaticClass()) )
					Rect.Max.X = Rect.Min.X + 28;
				else
					Rect.Max.X -= ButtonWidth;
				EditControl = new WEdit( &OwnerProperties->List );
				EditControl->Snoop = this;
				EditControl->OpenWindow( 0, 0, 0 );
				EditControl->MoveWindow( Rect+FRect(0,1,0,1), 0 );
			}
			SkipTheRest:
			ReceiveFromControl();
			Redraw();
			if( EditControl )
				EditControl->Show(1);
			if( TrackControl )
				TrackControl->Show(1);
			if( ComboControl )
				ComboControl->Show(1);
			if( HolderControl )
				HolderControl->Show(1);
			SetFocusToItem();
		}
		unguard;
	}
	void OnItemKillFocus(UBOOL Abort)
	{
		guard(FPropertyItem::OnKillFocus);
		if (!Abort)
			SendToControl();
		if (EditControl)
		{
			EditControl->Show(0);
			EditControl->DelayedDestroy();
		}
		EditControl = NULL;
		if (TrackControl)
		{
			TrackControl->Show(0);
			TrackControl->DelayedDestroy();
		}	
		TrackControl=NULL;
		if (ComboControl)
		{
			ComboControl->Show(0);
			ComboControl->DelayedDestroy();
		}
		ComboControl=NULL;
		if (HolderControl)
		{
			HolderControl->Show(0);
			HolderControl->DelayedDestroy();
		}
		HolderControl=NULL;
		FTreeItem::OnItemKillFocus( Abort );
		unguard;
	}
	void Expand();
	void Collapse()
	{
		guard(FPropertyItem::Collapse);
		FTreeItem::Collapse();
		EmptyChildren();
		unguard;
	}

	// FControlSnoop interface.
	void SnoopChar( WWindow* Src, INT Char )
	{
		guard(FPropertyItem::SnoopChar);
		if( Char==13 )
			Advance();
		else if( Char==27 )
			ReceiveFromControl();
		if (Char + 0x40 == 'C' && HIWORD(GetKeyState(VK_CONTROL)))
		{			
			BYTE* ReadValue = GetReadAddress(Property, this);
			if (ReadValue)
			{
				FString Str;
				GetPropertyTextSafe(Str, ReadValue);
				appClipboardCopy(*Str);
			}
		}
		FTreeItem::SnoopChar( Src, Char );
		unguard;
	}
	void ComboSelectionEndCancel()
	{
		guard(FPropertyItem::ComboSelectionEndCancel);
		ReceiveFromControl();
		unguard;
	}
	void ComboSelectionEndOk()
	{
		guard(FPropertyItem::ComboSelectionEndOk);
		ComboChanged=1;
		SendToControl();
		ReceiveFromControl();
		Redraw();
		unguard;
	}
	void OnTrackBarThumbTrack()
	{
		guard(FPropertyItem::OnTrackBarThumbTrack);
		if( TrackControl && EditControl )
		{
			EditControl->SetText(*FString::Printf(TEXT("%i"), TrackControl->GetPos()));
			EditControl->SetModify( 1 );
			UpdateWindow( *EditControl );
		}
		unguard;
	}
	void OnTrackBarThumbPosition()
	{
		guard(FPropertyItem::OnTrackBarThumbPosition);
		OnTrackBarThumbTrack();
		SendToControl();
		if( EditControl )
		{
			SetFocus( *EditControl );
			EditControl->SetSelection( 0, EditControl->GetText().Len() );
			Redraw();
		}
		unguard;
	}
	void OnChooseColorButton()
	{
		guard(FPropertyItem::OnChooseColorButton);
		BYTE* ReadValue = GetReadAddress(Property, this);
		CHOOSECOLORA cc;		
		appMemzero( &cc, sizeof(cc) );
		cc.lStructSize  = sizeof(cc);
		cc.hwndOwner    = OwnerProperties->List;
		cc.lpCustColors = (LPDWORD)acrCustClr;
		cc.rgbResult    = ReadValue ? *(DWORD*)ReadValue : 0;
		cc.Flags        = CC_FULLOPEN | CC_RGBINIT;
		if( ChooseColorA(&cc)==TRUE )
		{
			if (Parent->Children.Num() == 0)
				return;
			SetValue(*FString::Printf(TEXT("(R=%i,G=%i,B=%i)"), GetRValue(cc.rgbResult), GetGValue(cc.rgbResult), GetBValue(cc.rgbResult)));
			InvalidateRect( OwnerProperties->List, NULL, 0 );
			UpdateWindow( OwnerProperties->List );
		}
		Redraw();
		unguard;
	}
	void OnArrayAdd()
	{
		guard(FPropertyItem::OnArrayAdd);
		Collapse();
		for (INT j = GetItemCount() - 1; j >= 0; j--)
		{
			FArray* Addr = GetArrayAddress(j);
			if (Addr)
			{
				UArrayProperty* Array = CastChecked<UArrayProperty>(Property);
				Addr->AddZeroed(Array->Inner->ElementSize, 1);
			}
		}
		Expand();
		unguard;
	}
	void OnArrayEmpty()
	{
		guard(FPropertyItem::OnArrayEmpty);
		Collapse();
		for (INT j = GetItemCount() - 1; j >= 0; j--)
		{
			FArray* Addr = GetArrayAddress(j);
			if (Addr)
			{
				UArrayProperty* Array = CastChecked<UArrayProperty>(Property);
				for (INT i = 0; i < Addr->Num(); i++)
					Array->Inner->DestroyValue((BYTE*)Addr->GetData() + (i * Array->Inner->ElementSize));
				Addr->Empty(Array->Inner->ElementSize);
			}
		}
		Expand();
		if (Property->PropertyFlags & CPF_Config)
			SaveObjectConfig();
		unguard;
	}
	void OnArrayInsert()
	{
		guard(FPropertyItem::OnArrayInsert);
		if (!Parent)
			return;
		Parent->Collapse();
		UArrayProperty* Array = CastChecked<UArrayProperty>(Property->GetOuter());
		for (INT j = GetItemCount() - 1; j >= 0; j--)
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
		unguard;
	}
	void OnArrayDelete()
	{
		guard(FPropertyItem::OnArrayDelete);
		if (!Parent)
			return;
		if (Parent->Children.Num() == 1)
		{
			Parent->OnArrayEmpty();
			return;
		}
		UArrayProperty* Array = CastChecked<UArrayProperty>(Property->GetOuter());
		for (INT j = GetItemCount() - 1; j >= 0; j--)
		{
			FArray* Addr = Parent->GetArrayAddress(j);
			if (Addr)
			{
				INT Index = Offset / Array->Inner->ElementSize;
				Array->Inner->DestroyValue((BYTE*)Addr->GetData() + (Index * Array->Inner->ElementSize));
				Addr->Remove(Index, 1, Array->Inner->ElementSize);
			}
		}
		Parent->Collapse();
		Parent->Expand();
		if (CastChecked<UProperty>(Property->GetOuter())->PropertyFlags & CPF_Config)
			SaveObjectConfig();
		unguard;
	}
	void OnBrowseButton()
	{
		guard(FPropertyItem::OnBrowseButton);
		UObjectProperty* ReferenceProperty = CastChecked<UObjectProperty>(Property);		
		if( OwnerProperties->NotifyHook )
		{
			UClassProperty* ClassProperty = Cast<UClassProperty>(Property);
			GMetaClass = ClassProperty ? ClassProperty->MetaClass : NULL;
			OwnerProperties->NotifyHook->NotifyExec( OwnerProperties, *FString::Printf(TEXT("BROWSECLASS CLASS=%s"), *FObjectPathName(ReferenceProperty->PropertyClass)) );
		}
		Redraw();
		unguard;
	}
	void OnUseCurrentButton()
	{
		guard(FPropertyItem::OnUseCurrentButton);
		UObjectProperty* ReferenceProperty = CastChecked<UObjectProperty>(Property);
		if( OwnerProperties->NotifyHook )
			OwnerProperties->NotifyHook->NotifyExec( OwnerProperties, *FString::Printf(TEXT("USECURRENT CLASS=%s"), *FObjectPathName(ReferenceProperty->PropertyClass)) );
		Redraw();
		unguard;
	}
	void OnClearButton()
	{
		guard(FPropertyItem::OnClearButton);
		SetValue( TEXT("None") );
		Redraw();
		unguard;
	}

	// FPropertyItem interface.
	virtual void Advance()
	{
		guard(FPropertyItem::Advance);
		if( ComboControl && ComboControl->GetCount() )
		{
			ComboControl->SetCurrent( (ComboControl->GetCurrent()+1) % ComboControl->GetCount() );
			ComboChanged=1;
		}
		SendToControl();
		ReceiveFromControl();
		Redraw();
		unguard;
	}
	virtual void SendToControl()
	{
		guard(FPropertyItem::SendToControl);

		if (EditControl)
		{
			FString Str = EditControl->GetText();
			if (EditControl->GetModify())
				SetValue(*Str);
		}
		else if (ComboControl)
		{
			FString Str = ComboControl->GetString(ComboControl->GetCurrent());
			if (ComboChanged)
				SetValue(*Str);
			ComboChanged = 0;
		}
		unguard;
	}
	virtual void ReceiveFromControl()
	{
		guard(FPropertyItem::ReceiveFromControl);
		ComboChanged = 0;
		BYTE* ReadValue = GetReadAddress(Property, this);
		if (EditControl)
		{
			FString Str;
			if (ReadValue)
				GetPropertyTextSafe(Str, ReadValue);
			EditControl->SetText(*Str);
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
			else if (Property->IsA(UNameProperty::StaticClass()) && Name == NAME_InitialState)
			{
				INT Index = ReadValue ? ComboControl->FindString(**(FName*)ReadValue) : 0;
				ComboControl->SetCurrent(Index >= 0 ? Index : 0);
			}
			ComboChanged = 0;
		}
		unguard;
	}
};

// An abstract list header.
class WINDOW_API FHeaderItem : public FTreeItem
{
public:

	static COLORREF acrCustClr[16];

	// Constructors.
	FHeaderItem() = default;

	FHeaderItem( WPropertiesBase* InOwnerProperties, FTreeItem* InParent, UBOOL InExpandable )
	: FTreeItem( InOwnerProperties, InParent, InExpandable )
	{}

	// FTreeItem interface.
	void Draw( HDC hDC )
	{
		guard(FHeaderItem::Draw);
		FRect Rect = GetRect();

		// Draw background.
		FillRect( hDC, Rect, hBrushWhite ); 
		FillRect( hDC, Rect+FRect(0,1-GetSelected(),0,0), GetBackgroundBrush(GetSelected()) );

		// Draw tree.
		DrawTreeLines( hDC, Rect );

		// Prep text.
		SetTextColor( hDC, GetTextColor(GetSelected()) );
		SetBkMode( hDC, TRANSPARENT );

		// Draw name.
		FString C = GetCaption();

		if( !appStricmp( UObject::GetLanguage(), TEXT("jpt") ) )
			TextOutW( hDC, Rect.Min.X, Rect.Min.Y,  const_cast<TCHAR*>(*C), C.Len() );
		else
			DrawTextExW( hDC, const_cast<TCHAR*>(*C), C.Len(), FRect(Rect) + FRect(GetIndentPixels(1),1,-1,0), DT_END_ELLIPSIS | DT_LEFT | DT_SINGLELINE | DT_VCENTER, NULL );
		
 		unguard;
	}
	// Converts an RGB value to HSL format
	//
	// RGB : X = Red,	Y = Green,		Z = Blue
	// HSL : X = Hue,	Y = Saturation,	Z = Luminance
	//
	FVector ToHSL( FVector RGB )
	{
		guard(FHeaderItem::ToHSL);
		FVector HSL(0,0,0);

		unsigned char minval = Min(RGB.X, Min(RGB.Y, RGB.Z));
		unsigned char maxval = Max(RGB.X, Max(RGB.Y, RGB.Z));
		float mdiff  = float(maxval) - float(minval);
		float msum   = float(maxval) + float(minval);

		HSL.Z = msum / 510.0f;

		if (maxval == minval) 
		{
			HSL.Y = 0.0f;
			HSL.X = 0.0f; 
		}   
		else 
		{ 
			float rnorm = (maxval - RGB.X  ) / mdiff;      
			float gnorm = (maxval - RGB.Y) / mdiff;
			float bnorm = (maxval - RGB.Z ) / mdiff;   

			HSL.Y = (HSL.Z <= 0.5f) ? (mdiff / msum) : (mdiff / (510.0f - msum));

			if (RGB.X   == maxval) HSL.X = 60.0f * (6.0f + bnorm - gnorm);
			if (RGB.Y == maxval) HSL.X = 60.0f * (2.0f + rnorm - bnorm);
			if (RGB.Z  == maxval) HSL.X = 60.0f * (4.0f + gnorm - rnorm);
			if (HSL.X > 360.0f) HSL.X = HSL.X - 360.0f;
		}

		HSL.X = 255.0f * (HSL.X / 360.0f);
		HSL.Y *= 255.0f;
		HSL.Z *= 255.0f;

		return HSL;
		unguard;
	}
	void OnChooseHSLColorButton()
	{
		guard(FHeaderItem::OnChooseColorButton);

		// Get the current color in HSL format and convert it to RGB.
		FVector _RGB(0,0,0);

		// Open the color dialog.
		CHOOSECOLORA cc;		
		appMemzero( &cc, sizeof(cc) );
		cc.lStructSize  = sizeof(cc);
		cc.hwndOwner    = OwnerProperties->List;
		cc.lpCustColors = (LPDWORD)acrCustClr;
		cc.rgbResult    = RGB( _RGB.X, _RGB.Y, _RGB.Z );
		cc.Flags        = CC_FULLOPEN | CC_RGBINIT;
		if( ChooseColorA(&cc)==TRUE )
		{
			FVector HSL = ToHSL( FVector( GetRValue(cc.rgbResult), GetGValue(cc.rgbResult), GetBValue(cc.rgbResult) ));
			
			// Once the user chooses a color, loop through the child items of this header item, and set
			// the appropriate values.  We have to do this, since I don't see how to get access to the
			// actual actors themselves.  NOTE : You have to have this header item expanded before this will work.
			for( int x = 0 ; x < Children.Num() ; x++ )
			{
				FTreeItem* Item = Children(x);

				if( Item->GetCaption() == TEXT("LightHue") )
					Item->SetValue(*FString::Printf(TEXT("%d"), (int)HSL.X));
				if (Item->GetCaption() == TEXT("LightSaturation"))
					Item->SetValue(*FString::Printf(TEXT("%d"), (int)HSL.Z));
			}

			if (Children.Num() == 0)
				return;

			InvalidateRect( OwnerProperties->List, NULL, 0 );
			UpdateWindow( OwnerProperties->List );
		}
		Redraw();
		unguard;
	}
	void OnItemSetFocus()
	{
		guard(FHeaderItem::OnItemSetFocus);
		if( GetCaption() == TEXT("LightColor") && Children.Num() != 0)
			AddButton( TEXT("Color"), FDelegate(this,(TDelegate)&FHeaderItem::OnChooseHSLColorButton) );
		unguard;
	}
	virtual void OnItemKillFocus( UBOOL Abort )
	{
		guard(FHeaderItem::OnItemKillFocus);
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
	INT GetHeight()
	{
		guard(FHeaderItem::GetHeigth);
		return MulDiv(16, WWindow::DPIY, 96);
		unguard;
	}
};

// An category header list item.
class WINDOW_API FCategoryItem : public FHeaderItem
{
public:
	// Variables.
	FName Category;
	UClass* BaseClass;

	// Constructors.
	FCategoryItem( WPropertiesBase* InOwnerProperties, FTreeItem* InParent, UClass* InBaseClass, FName InCategory, UBOOL InExpandable )
	:	FHeaderItem( InOwnerProperties, InParent, InExpandable )
	,	Category    ( InCategory )
	,	BaseClass	( InBaseClass )
	{
		check(BaseClass);
	}

	// FTreeItem interface.
	void Serialize( FArchive& Ar )
	{
		guard(FCategoryItem::Serialize);
		FHeaderItem::Serialize( Ar );
		Ar << Category << BaseClass;
		unguard;
	}
	QWORD GetId() const
	{
		guard(FCategoryItem::GetId);
		return Category.GetIndex() + ((QWORD)2<<32);
		unguard;
	}
	virtual FString GetCaption() const
	{
		guard(FCategoryItem::GetText);
		return *Category;
		unguard;
	}
	void Expand()
	{
		guard(FCategoryItem::Expand);
		FName FindCatr = (Category == FName(UNSpecCtr) ? NAME_None : Category);
		for (TFieldIterator<UProperty> It(BaseClass); It; ++It)
			if (It->Category == FindCatr && AcceptFlags(It->PropertyFlags))
				Children.AddItem(new FPropertyItem(OwnerProperties, this, *It, It->GetFName(), It->Offset, -1));
		FTreeItem::Expand();
		unguard;
	}
	void Collapse()
	{
		guard(FCategoryItem::Collapse);
		FTreeItem::Collapse();
		EmptyChildren();
		unguard;
	}
};

// For hidden properties...
class WINDOW_API FHiddenCategoryItem : public FHeaderItem
{
public:
	// Variables.
	UClass* BaseClass;

	// Constructors.
	FHiddenCategoryItem(WPropertiesBase* InOwnerProperties, FTreeItem* InParent, UClass* InBaseClass)
		: FHeaderItem(InOwnerProperties, InParent, 1)
		, BaseClass(InBaseClass)
	{
		check(BaseClass);
	}

	// FTreeItem interface.
	void Serialize(FArchive& Ar)
	{
		guard(FHiddenCategoryItem::Serialize);
		FHeaderItem::Serialize(Ar);
		Ar << BaseClass;
		unguard;
	}
	QWORD GetId() const
	{
		guard(FHiddenCategoryItem::GetId);
		return BaseClass->GetIndex() + ((QWORD)5 << 32);
		unguard;
	}
	virtual FString GetCaption() const
	{
		guard(FHiddenCategoryItem::GetText);
		return BaseClass->GetName();
		unguard;
	}
	void Expand()
	{
		guard(FHiddenCategoryItem::Expand);
		for (TFieldIterator<UProperty> It(BaseClass); (It && It->GetOwnerClass() == BaseClass); ++It)
			if (!AcceptFlags(It->PropertyFlags) || It->Category == NAME_None)
				Children.AddItem(new FPropertyItem(OwnerProperties, this, *It, It->GetFName(), It->Offset, -1));
		FTreeItem::Expand();
		unguard;
	}
	void Collapse()
	{
		guard(FHiddenCategoryItem::Collapse);
		FTreeItem::Collapse();
		EmptyChildren();
		unguard;
	}
};

class WINDOW_API FHiddenCategoryList : public FHiddenCategoryItem
{
public:
	// Constructors.
	FHiddenCategoryList(WPropertiesBase* InOwnerProperties, FTreeItem* InParent, UClass* InBaseClass)
		: FHiddenCategoryItem(InOwnerProperties, InParent, InBaseClass)
	{}

	// FTreeItem interface.
	QWORD GetId() const
	{
		guard(FHiddenCategoryList::GetId);
		return BaseClass->GetIndex() + ((QWORD)6 << 32);
		unguard;
	}
	virtual FString GetCaption() const
	{
		guard(FHiddenCategoryItem::GetText);
		return TEXT("<Hidden Properties>");
		unguard;
	}
	COLORREF GetTextColor(UBOOL Selected)
	{
		guard(FHiddenCategoryItem::GetTextColor);
		return Selected ? RGB(255, 128, 128) : RGB(128, 0, 0);
		unguard;
	}
	void Expand()
	{
		guard(FHiddenCategoryItem::Expand);
		TArray<UClass*> CatList;
		for (TFieldIterator<UProperty> It(BaseClass); It; ++It)
			if (!AcceptFlags(It->PropertyFlags) || It->Category == NAME_None)
				CatList.AddUniqueItem(It->GetOwnerClass());
		for (INT i = 0; i < CatList.Num(); ++i)
			Children.AddItem(new FHiddenCategoryItem(OwnerProperties, this, CatList(i)));
		FTreeItem::Expand();
		unguard;
	}
};


/*-----------------------------------------------------------------------------
	WProperties.
-----------------------------------------------------------------------------*/

// General property editing control.
class WINDOW_API WProperties : public WPropertiesBase
{
	W_DECLARE_ABSTRACT_CLASS(WProperties,WPropertiesBase,CLASS_Transient);
	DECLARE_WINDOWCLASS(WProperties,WWindow,Window)

	// Metallicafan212:	This is just for ForceRefresh, to store the state of the window before clearing it
	struct FPropItem
	{
		QWORD ID;

		UBOOL bExpanded;

		UBOOL bTop;
		UBOOL bFocus;

		TMap<QWORD, FPropItem> Children;
	};

	// Variables.
	TArray<QWORD>		Remembered;
	QWORD				SavedTop{}, SavedCurrent{};
	WDragInterceptor*	DragInterceptor{};
	INT					DividerWidth{};
	static TArray<WProperties*> PropertiesWindows;
	UBOOL bAllowForceRefresh{};
	TMap<QWORD, FPropItem> RootMap;

	// Structors.
	WProperties() = default;

	WProperties( FName InPersistentName, WWindow* InOwnerWindow=NULL )
	:	WPropertiesBase	( InPersistentName, InOwnerWindow )
	,	DragInterceptor	( NULL )
	,	DividerWidth	( MulDiv(128, DPIX, 96) )
	{
		guard(WProperties::WProperties);
		if( PersistentName!=NAME_None )
			GConfig->GetInt( TEXT("WindowPositions"), *(FString(*PersistentName)+TEXT(".Split")), DividerWidth );
		PropertiesWindows.AddItem( this );
		List.DoubleClickDelegate     = FDelegate(this,(TDelegate)&WProperties::OnListDoubleClick);
		List.SelectionChangeDelegate = FDelegate(this,(TDelegate)&WProperties::OnListSelectionChange);
		bAllowForceRefresh = 1;
		unguard;
	}

	// WWindow interface.
	void Serialize( FArchive& Ar )
	{
		guard(WProperties::Serialize);
		WPropertiesBase::Serialize( Ar );
		GetRoot()->Serialize( Ar );
		unguard;
	}
	void DoDestroy()
	{
		guard(WProperties::DoDestroy);
		PropertiesWindows.RemoveItem( this );
		WWindow::DoDestroy();
		unguard;
	}
	INT OnSetCursor()
	{
		guard(WProperties::OnSetCursor);
		if( ::IsWindow(List.hWnd) && List.GetCount() )
		{
			FPoint P = GetCursorPos();
			INT Index = List.ItemFromPoint( P );
			return Index>0 ? GetListItem( Index )->OnSetCursor() : 0;
		}
		return 0;
		unguard;
	}
	void OnDestroy()
	{
		guard(WProperties::OnDestroy);
		WWindow::OnDestroy();
		_DeleteWindows.AddUniqueItem( this );
		unguard;
	}
	void OpenChildWindow( INT InControlId )
	{
		guard(WProperties::OpenChildWindow);
		HWND hWndParent = InControlId ? GetDlgItem(OwnerWindow->hWnd,InControlId) : OwnerWindow->hWnd;
		check(hWndParent);
		FRect R;
		::GetClientRect( hWndParent, R );
		PerformCreateWindowEx
		(
			WS_EX_CONTROLPARENT, NULL, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
			0, 0, R.Width(), R.Height(),
			hWndParent, NULL, hInstance
		);
		List.OpenWindow( 1, 0, 0, 1, 0, WS_TABSTOP );
		Show(1);
		unguard;
	}
	void OpenWindow( HWND hWndParent=NULL )
	{
		guard(WProperties::OpenWindow);
		PerformCreateWindowEx
		(
			WS_EX_TOOLWINDOW,
			*GetRoot()->GetCaption(),
			WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_SIZEBOX | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			MulDiv(512, DPIX, 96),
			MulDiv(512, DPIY, 96),
			hWndParent ? hWndParent : OwnerWindow ? OwnerWindow->hWnd : NULL,
			NULL,
			hInstance
		);
		List.OpenWindow( 1, 0, 0, 1 );
		unguard;
	}
	void OnActivate( UBOOL Active )
	{
		guard(WProperties::OnActivate);
		if( Active==1 )
		{
			SetFocus( List );
			if( !FocusItem )
				SetItemFocus( 1 );
		}
		else
		{
			SetItemFocus( 0 );
		}
		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WProperties::OnSize);
		WWindow::OnSize( Flags, NewX, NewY );
		if( List.hWnd )
		{
			SetItemFocus( 0 );
			InvalidateRect( List, NULL, FALSE );
			ResizeList();
			SetItemFocus( 1 );
		}
		unguard;
	}
	void OnPaint()
	{
		guard(WProperties::OnPaint);
		if( GetUpdateRect( *this, NULL, 0 ) )
		{
			PAINTSTRUCT PS;
			HDC   hDC      = BeginPaint( *this, &PS );
			FRect Rect     = GetClientRect();
			FRect ListRect = List.GetClientRect();
			Rect.Min.Y     = ListRect.Max.Y;
			FillRect( hDC, Rect, (HBRUSH)(COLOR_BTNFACE+1) ); 
			EndPaint( *this, &PS );
		}
		unguard;
	}

	// Delegates.
	void OnListDoubleClick()
	{
		if( FocusItem )
			FocusItem->OnItemDoubleClick();
	}
	void OnListSelectionChange()
	{
		SetItemFocus( 1 );
	}

	// FControlSnoop interface.
	void SnoopLeftMouseDown( WWindow* Src, FPoint P )
	{
		guard(WProperties::SnoopLeftMouseDown);
		if( Src==&List )
		{
			INT Index = List.ItemFromPoint( P );
			if( Index>=0 )
			{
				List.SetCurrent( Index, 0 );
				FTreeItem* Item = GetListItem( Index );
				if (Item && Item != (FTreeItem*)LB_ERR)
					Item->OnItemLeftMouseDown( P-Item->GetRect().Min );
			}
		}
		unguard;
	}
	void SnoopRightMouseDown( WWindow* Src, FPoint P )
	{
		guard(WProperties::SnoopRightMouseDown);
		if( Src==&List )
		{
			INT Index = List.ItemFromPoint( P );
			if( Index>=0 )
			{
				List.SetCurrent( Index, 0 );
				FTreeItem* Item = GetListItem( Index );
				if (Item && Item != (FTreeItem*)LB_ERR)
					Item->OnItemRightMouseDown( P-Item->GetRect().Min );
			}
		}
		unguard;
	}
	void SnoopChar( WWindow* Src, INT Char )
	{
		guard(WProperties::SnoopChar);
		if( FocusItem )
			FocusItem->SnoopChar( Src, Char );
		unguard;
	}
	void SnoopKeyDown( WWindow* Src, INT Char )
	{
		guard(WProperties::SnoopKeyDown);
		if( Char==9 )
			PostMessageW( List, WM_KEYDOWN, (GetKeyState(16)&0x8000)?VK_UP:VK_DOWN, 0 );
		WPropertiesBase::SnoopKeyDown( Src, Char );
		unguard;
	}

	// WPropertiesBase interface.
	INT GetDividerWidth()
	{
		guard(WProperties::GetDividerWidth);
		return DividerWidth;
		unguard;
	}

	// WProperties interface.
	virtual void BeginSplitterDrag()
	{
		guard(WProperties::BeginSplitterDrag);
		SetItemFocus( NULL );
		DragInterceptor            = new WDragInterceptor( this, FPoint(0,INDEX_NONE), GetClientRect(), FPoint(3,3) );	
		DragInterceptor->DragPos   = FPoint(GetDividerWidth(),GetCursorPos().Y);
		DragInterceptor->DragClamp = FRect(GetClientRect().Inner(FPoint(64,0)));
		DragInterceptor->OpenWindow();
		unguard;
	}
	void OnFinishSplitterDrag( WDragInterceptor* Drag, UBOOL Success )
	{
		guard(WProperties::OnFinishSplitterDrag);
		if( Success )
		{
			DividerWidth += Drag->DragPos.X - Drag->DragStart.X;
			if( PersistentName!=NAME_None )
				GConfig->SetInt( TEXT("WindowPositions"), *(FString(*PersistentName)+TEXT(".Split")), DividerWidth );
			InvalidateRect( *this, NULL, 0 );
			UpdateWindow( *this );
		}
		DragInterceptor = NULL;
		unguard;
	}
	virtual void SetValue( const TCHAR* Value )
	{
		guard(WProperties::SetValue);
		if( FocusItem )
			FocusItem->SetValue( Value );
		unguard;
	}
	virtual void SetItemFocus( UBOOL FocusCurrent )
	{
		guard(WProperties::SetItemFocus);
		if( FocusItem )
			FocusItem->OnItemKillFocus( 0 );
		FocusItem = NULL;
		if( FocusCurrent && List.GetCount()>0 )
			FocusItem = GetListItem( List.GetCurrent() );
		if( FocusItem )
			FocusItem->OnItemSetFocus();
		unguard;
	}
	virtual void ResizeList()
	{
		guard(WProperties::ResizeList);
		FRect ClientRect = GetClientRect();
		FRect R(0,0,0,4);//!!why?
		R = R.DPIScaled(WWindow::DPIX, WWindow::DPIY);
		for( INT i=List.GetCount()-1; i>=0; i-- )
			R.Max.Y += List.GetItemHeight( i );
		AdjustWindowRect( R, GetWindowLongW(List,GWL_STYLE), 0 );
		List.MoveWindow( FRect(0,0,ClientRect.Width(),Min(ClientRect.Height(),R.Height())), 1 );
		unguard;
	}

	void ProcessChildren(FTreeItem* Item, TMap<QWORD, FPropItem>& IArray, FTreeItem* Top, FTreeItem* Focus)
	{
		guard(WProperties::ProcessChildren);

		// Metallicafan212:	Process the children of this item
		for (INT i = 0; i < Item->Children.Num(); i++)
		{
			FTreeItem* itm	= Item->Children(i);

			FPropItem& PIt	= IArray.Set(itm->GetId(), FPropItem());

			PIt.ID			= itm->GetId();

			if (itm->Expanded)
			{
				PIt.bExpanded = 1;

				// Metallicafan212:	Now process this child's properties
				ProcessChildren(itm, PIt.Children, Top, Focus);
			}

			// Metallicafan212:	See if this item is the top or the item in focus
			if (itm == Top)
			{
				PIt.bTop = 1;
			}

			if (itm == Focus)
			{
				PIt.bFocus = 1;
			}
		}

		unguard;
	}

	// Metallicafan212:	Process and expand
	void ExpandChildren(FTreeItem* Item, TMap<QWORD, FPropItem>& IArray, FTreeItem** NewTop, FTreeItem** NewCur)
	{
		guard(WProperties::ExpandChildren);

		for (INT i = 0; i < Item->Children.Num(); i++)
		{
			FTreeItem* itm = Item->Children(i);

			// Metallicafan212:	Find it in the map
			FPropItem* PIt = IArray.Find(itm->GetId());

			if (PIt != nullptr)
			{
				// Metallicafan212:	See if it should become the new focus or new top
				if (PIt->bFocus && NewCur != nullptr)
				{
					*NewCur = itm;
				}

				if (PIt->bTop && NewTop != nullptr)
				{
					*NewTop = itm;
				}

				// Metallicafan212:	If this property was expanded, process its children
				if (PIt->bExpanded)
				{
					// Metallicafan212:	If this item isn't currently expanded, expand it now
					if (!itm->Expanded)
					{
						itm->Expand();
					}

					// Metallicafan212:	Now process it all
					ExpandChildren(itm, PIt->Children, NewTop, NewCur);
				}
			}
		}

		unguard;
	}

	// Metallicafan212:	This default implementation causes issues with the new expandable sub-object properties
	//					So, it needs a reimplementation using a tree-like structure to decend and mark things as expanded manually
	virtual void ForceRefresh()
	{
		guard(WProperties::ForceRefresh);

		if( !bAllowForceRefresh ) return;

		// Disable editing.
		SetItemFocus( 0 );

		// Metallicafan212:	See if we need to assemble a list
		FTreeItem* Root = GetRoot();

		// Metallicafan212:	Only process if the list has content
		if (List.GetCount())
		{
			RootMap.Empty();
			FTreeItem* OldTop = GetListItem(List.GetTop());
			FTreeItem* OldCur = List.GetSelected(List.GetCurrent()) ? GetListItem(List.GetCurrent()) : nullptr;

			// Metallicafan212:	Now get the (manual) tree representation of the properties
			ProcessChildren(Root, RootMap, OldTop, OldCur);
		}

		// Empty it and add root items.
		List.Empty();
		Root->EmptyChildren();
		Root->Expanded=0;
		Root->Expand();

		// Metallicafan212:	Now reprocess the children of the root
		FTreeItem* NewTop = nullptr;
		FTreeItem* NewCur = nullptr;

		ExpandChildren(Root, RootMap, &NewTop, &NewCur);

		// Adjust list size.
		ResizeList();

		// Metallicafan212:	Now set the top and current (if they were set)
		if (NewTop != nullptr)
		{
			INT NTop = List.FindItem(NewTop);

			if (NTop != INDEX_NONE)
			{
				List.SetTop(NTop);
			}
		}

		if (NewCur != nullptr)
		{
			INT NCur = List.FindItem(NewCur);

			if (NCur != INDEX_NONE)
			{
				List.SetCurrent(NCur, 0);
			}
		}

		unguard;
	}
};

/*-----------------------------------------------------------------------------
	FPropertyItemBase.
-----------------------------------------------------------------------------*/

class WINDOW_API FPropertyItemBase : public FHeaderItem
{
public:
	// Variables.
	FString Caption;
	DWORD FlagMask{};
	UClass* BaseClass{};

	// Structors.
	FPropertyItemBase()	= default;

	FPropertyItemBase( WPropertiesBase* InOwnerProperties, FTreeItem* InParent, DWORD InFlagMask, const TCHAR* InCaption )
	:	FHeaderItem	( InOwnerProperties, InParent, 1 )
	,	Caption		( InCaption )
	,	FlagMask	( InFlagMask )
	,	BaseClass	( NULL )
	{}

	// FTreeItem interface.
	void Serialize( FArchive& Ar )
	{
		guard(FPropertyItemBase::Serialize);
		FHeaderItem::Serialize( Ar );
		Ar << BaseClass;
		unguard;
	}

	// FHeaderItem interface.
	UBOOL AcceptFlags( DWORD InFlags )
	{
		guard(FPropertyItemBase::AcceptFlags);
		return (InFlags&FlagMask)==FlagMask;
		unguard;
	}
	void GetStates( TArray<FName>& States )
	{
		guard(FPropertyItemBase::GetStates);
		if( BaseClass )
			for( TFieldIterator<UState> StateIt(BaseClass); StateIt; ++StateIt )
				if( StateIt->StateFlags & STATE_Editable )
					States.AddUniqueItem( StateIt->GetFName() );
		unguard;
	}
	void Collapse()
	{
		guard(FPropertyItemBase::Collapse);
		FTreeItem::Collapse();
		EmptyChildren();
		unguard;
	}
	FString GetCaption() const
	{
		guard(FPropertyItemBase::GetCaption);
		return Caption;
		unguard;
	}
	QWORD GetId() const
	{
		guard(FPropertyItemBase::GetId);
		if (!BaseClass)
			return appStrihash(*GetCaption()) + ((QWORD)8<<32);
		return (QWORD)BaseClass + ((QWORD)7<<32);
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	WObjectProperties.
-----------------------------------------------------------------------------*/

// Object properties root.
class WINDOW_API FObjectsItem : public FPropertyItemBase
{
public:
	// Variables.
	UBOOL ByCategory{};
	TArray<UObject*> _Objects;
	TArray<INT> _ObjectIndexes;

	// Structors.
	FObjectsItem() = default;

	FObjectsItem( WPropertiesBase* InOwnerProperties, FTreeItem* InParent, DWORD InFlagMask, const TCHAR* InCaption, UBOOL InByCategory )
	:	FPropertyItemBase( InOwnerProperties, InParent, InFlagMask, InCaption )
	,	ByCategory( InByCategory )
	{}

	// FTreeItem interface.
	void Serialize( FArchive& Ar )
	{
		guard(FObjectsItem::Serialize);
		FPropertyItemBase::Serialize( Ar );
		Ar << _Objects;
		unguard;
	}
	BYTE* GetReadAddress(UProperty* InProperty, FTreeItem* Top, INT Index = INDEX_NONE)
	{
		guard(FObjectsItem::GetReadAddress);
		ValidateSelection();
		if (!_Objects.Num())
			return NULL;
		else if (_Objects.Num() == 1)
			return (BYTE*)_Objects(0);
		else if (Index >= 0)
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
		BYTE* Base0 = Top->GetReadAddress(InProperty, Top, 0);
		for (INT i = 1; i < _Objects.Num(); i++)
			if (!InProperty->Identical(Base0, Top->GetReadAddress(InProperty, Top, i)))
				return NULL;
		return Base;
		unguard;
	}
	void SetProperty(UProperty* Property, FTreeItem* Offset, const TCHAR* Value)
	{
		guard(FObjectsItem::SetProperty);
		ValidateSelection();
		if (OwnerProperties->NotifyHook)
			OwnerProperties->NotifyHook->NotifyPreChange(OwnerProperties);
		for (INT i = 0; i < _Objects.Num(); i++)
		{
			_Objects(i)->Modify();
			Property->ImportText(Value, Offset->GetReadAddress(Property, Offset, i), PPF_Localized);
			_Objects(i)->PostEditChange();
		}
		if (OwnerProperties->NotifyHook)
			OwnerProperties->NotifyHook->NotifyPostChange(OwnerProperties);
		unguard;
	}
	void Expand()
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
				if (AcceptFlags(It->PropertyFlags) && It->Category != NAME_None)
					Categories.AddUniqueItem(It->Category);
				else bHasHidden = 1;
			}
			for (INT i = 0; i < Categories.Num(); i++)
				Children.AddItem(new FCategoryItem(OwnerProperties, this, BaseClass, Categories(i), 1));
			if (bHasHidden)
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
	INT GetItemCount() const
	{
		return _Objects.Num();
	}

	// FHeaderItem interface.
	FString GetCaption() const
	{
		guard(FObjectsItem::GetCaption);

		if( Caption.Len() )
			return Caption;			
		else if( !BaseClass )
			return LocalizeGeneral(TEXT("PropNone"), TEXT("Window"));
		else if( _Objects.Num()==1 )
			return FString::Printf( LocalizeGeneral(TEXT("PropSingle"), TEXT("Window")), BaseClass->GetName() );
		else
			return FString::Printf( LocalizeGeneral(TEXT("PropMulti"), TEXT("Window")), BaseClass->GetName(), _Objects.Num() );

		unguard;
	}

	// FObjectsItem interface.
	virtual void SetObjects( UObject** InObjects, INT Count )
	{
		guard(FObjectsItem::SetObjects);

		// Disable editing, to prevent crash due to edit-in-progress after empty objects list.
		OwnerProperties->SetItemFocus( 0 );

		// Add objects and find lowest common base class.
		_Objects.Empty();
		_ObjectIndexes.Empty();
		BaseClass = NULL;
		for( INT i=0; i<Count; i++ )
		{
			if( InObjects[i] )
			{
				check(InObjects[i]->GetClass());
				if( BaseClass==NULL )	
					BaseClass=InObjects[i]->GetClass();
				while( !InObjects[i]->GetClass()->IsChildOf(BaseClass) )
					BaseClass = BaseClass->GetSuperClass();
				_Objects.AddItem( InObjects[i] );
				_ObjectIndexes.AddItem(InObjects[i]->GetIndex());
			}
		}

		// Automatically title the window.
		OwnerProperties->SetText( *GetCaption() );

		// Refresh all properties.
		if( Expanded || this==OwnerProperties->GetRoot() )
			OwnerProperties->ForceRefresh();

		unguard;
	}
	void ValidateSelection()
	{
		guard(FObjectsItem::ValidateSelection);
		bool bChanged = false;
		INT i = 0;
		for (i = 0; i < _Objects.Num(); i++)
		{
			if (!_Objects(i) || (i < _ObjectIndexes.Num() && _Objects(i) != UObject::GetIndexedObject(_ObjectIndexes(i))) || _Objects(i)->IsPendingKill())
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
	UObject* GetParentOuter()
	{
		guard(FObjectsItem::GetParentOuter);
		for (INT i = 0; i < _Objects.Num(); ++i)
			if (_Objects(i))
			{
				UObject* Outer = _Objects(i);
				while (Outer->GetOuter())
					Outer = Outer->GetOuter();
				return Outer;
			}
		return NULL;
		unguard;
	}
	void SaveObjectConfig() {}
};

// Multiple selection object properties.
#define WPROP_MENU_COPY 8000
class WINDOW_API WObjectProperties : public WProperties
{
	W_DECLARE_CLASS(WObjectProperties,WProperties,CLASS_Transient);
	DECLARE_WINDOWCLASS(WObjectProperties,WProperties,Window)

	// Variables.
	FObjectsItem Root;

	// Structors.
	WObjectProperties()	= default;

	WObjectProperties( FName InPersistentName, DWORD InFlagMask, const TCHAR* InCaption, WWindow* InOwnerWindow, UBOOL InByCategory )
	:	WProperties	( InPersistentName, InOwnerWindow )
	,	Root		( this, NULL, InFlagMask, InCaption, InByCategory )
	{}
	void OpenWindow( HWND hWndParent=NULL )
	{
		guard(WObjectProperties::OpenWindow);
		WProperties::OpenWindow(hWndParent);
		
		HMENU SysMenu = GetSystemMenu(hWnd, false);
		if (SysMenu)
			AppendMenu(SysMenu, MF_STRING, WPROP_MENU_COPY, TEXT("Copy Actor properties"));
		unguard;
	}

	// WPropertiesBase interface.
	FTreeItem* GetRoot()
	{
		guard(WObjectProperties::GetRoot);
		return &Root;
		unguard;
	}
	int OnSysCommand(INT Command)
	{
		if (Command == WPROP_MENU_COPY)
		{
			if (Root._Objects.Num())
			{
				for( FObjectIterator It; It; ++It )
					It->ClearFlags( RF_TagImp | RF_TagExp );
				
				FStringOutputDevice Ar;
				for (INT i = 0; i < Root._Objects.Num(); ++i)
				{
					UObject* Obj = Root._Objects(i);
					if (Obj)
					{
						Ar.Serialize(*(FString::Printf(TEXT("--- %ls %ls ---\r\n"), Obj->GetClass()->GetName(), *Obj->GetFName())), NAME_None);
						Ar.Serialize(*(FString::Printf(TEXT("  CurrentState=%ls\r\n"), 
							*((Obj->GetStateFrame() && Obj->GetStateFrame()->StateNode != Obj->GetClass()) ? Obj->GetStateFrame()->StateNode->GetFName() : NAME_None))), NAME_None);
						Obj->ExportProperties(Ar, Obj->GetClass(), (BYTE*)Obj, 0, Obj->GetClass(), &Obj->GetClass()->Defaults(0));
					}
				}
				appClipboardCopy( *Ar );

				for( FObjectIterator It; It; ++It )
					It->ClearFlags( RF_TagImp | RF_TagExp );
			}
			return 1;
		}
		return WProperties::OnSysCommand(Command);
	}
};

/*-----------------------------------------------------------------------------
	WClassProperties.
-----------------------------------------------------------------------------*/

// Object properties root.
class WINDOW_API FClassItem : public FPropertyItemBase
{
public:
	// Structors.
	FClassItem() = default;

	FClassItem( WPropertiesBase* InOwnerProperties, FTreeItem* InParent, DWORD InFlagMask, const TCHAR* InCaption, UClass* InBaseClass )
	:	FPropertyItemBase( InOwnerProperties, InParent, InFlagMask, InCaption )
	{
		BaseClass = InBaseClass;
	}

	// FTreeItem interface.
	BYTE* GetReadAddress(UProperty* InProperty, FTreeItem* Top, INT Index = INDEX_NONE)
	{
		guard(FClassItem::GetReadAddress);
		return &BaseClass->Defaults(0);
		unguard;
	}
	void SetProperty(UProperty* Property, FTreeItem* Offset, const TCHAR* Value)
	{
		guard(FClassItem::SetProperty);
		Property->ImportText(Value, Offset->GetReadAddress(Property, this), PPF_Localized);
		BaseClass->SetFlags(RF_SourceModified);
		unguard;
	}
	void Expand()
	{
		guard(FClassItem::Expand);
		TArray<FName> Categories;
		UBOOL bHasHidden = 0;
		for (TFieldIterator<UProperty> It(BaseClass); It; ++It)
		{
			if (AcceptFlags(It->PropertyFlags) && It->Category != NAME_None)
				Categories.AddUniqueItem(It->Category);
			else bHasHidden = 1;
		}
		for (INT i = 0; i < Categories.Num(); i++)
			Children.AddItem(new FCategoryItem(OwnerProperties, this, BaseClass, Categories(i), 1));
		if (bHasHidden)
			Children.AddItem(new FHiddenCategoryList(OwnerProperties, this, BaseClass));
		FTreeItem::Expand();
		unguard;
	}
	UObject* GetParentOuter()
	{
		guard(FClassItem::GetParentOuter);
		UObject* Outer = BaseClass;
		while (Outer->GetOuter())
			Outer = Outer->GetOuter();
		return Outer;
		unguard;
	}
	void SaveObjectConfig()
	{
		guard(FClassItem::SaveObjectConfig);
		BaseClass->SetFlags(RF_SourceModified);
		unguard;
	}
};

// Multiple selection object properties.
class WINDOW_API WClassProperties : public WProperties
{
	W_DECLARE_CLASS(WClassProperties,WProperties,CLASS_Transient);
	DECLARE_WINDOWCLASS(WClassProperties,WProperties,Window)

	// Variables.
	FClassItem Root;

	// Structors.
	WClassProperties()
	{}
	WClassProperties( FName InPersistentName, DWORD InFlagMask, const TCHAR* InCaption, UClass* InBaseClass )
	:	WProperties	( InPersistentName )
	,	Root		( this, NULL, InFlagMask, InCaption, InBaseClass )
	{}

	// WPropertiesBase interface.
	FTreeItem* GetRoot()
	{
		guard(WClassProperties::GetRoot);
		return &Root;
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	WConfigProperties.
-----------------------------------------------------------------------------*/

// Object configuration header.
class WINDOW_API FObjectConfigItem : public FPropertyItemBase
{
public:
	// Variables.
	FString  ClassName;
	FName    CategoryFilter;
	UClass*  Class{};
	UObject* OverrideRes{};
	UBOOL	 Failed{};
	UBOOL    Immediate;

	// Structors.
	FObjectConfigItem( WPropertiesBase* InOwnerProperties, FTreeItem* InParent, const TCHAR* InCaption, const TCHAR* InClass, UBOOL InImmediate, FName InCategoryFilter )
	:	FPropertyItemBase( InOwnerProperties, InParent, CPF_Config, InCaption )
	,	ClassName		( InClass )
	,	CategoryFilter	( InCategoryFilter )
	,	Immediate		( InImmediate )
	{}

	// FTreeItem interface.
	BYTE* GetReadAddress(UProperty* InProperty, FTreeItem* Top, INT Index = INDEX_NONE)
	{
		guard(FObjectConfigItem::GetReadAddress);
		return &Class->Defaults(0);
		unguard;
	}
	INT GetPropertyOffset(UProperty* Prop)
	{
		INT Res = 0;
		for (Prop = Prop; Prop; Prop = Cast<UProperty>(Prop->GetOuter()))
			Res += Prop->Offset;
		return Res;
	}
	void SetProperty(UProperty* Property, FTreeItem* Offset, const TCHAR* Value)
	{
		guard(FObjectConfigItem::SetProperty);
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
				if (Path.Split(TEXT("."), &Left, &Right))
					Text = Localize(*Right, TEXT("ClassCaption"), *Left);
				else
					Text = Localize(TEXT("Language"), TEXT("Language"), TEXT("Core"), *Path);

				if (appStricmp(*Text, Value) == 0)
					GConfig->SetString(*FObjectPathName(Property->GetOwnerClass()), Property->GetName(), *Classes(i).Object);
			}
		}
		else
		{
			Property->ImportText(Value, Offset->GetReadAddress(Property, this), PPF_Localized);
			if (Immediate)
			{
				guard(Immediate);
				UProperty* ParentProp = Offset->GetParentProperty();

				for (FObjectIterator It; It; ++It)
				{
					if (It->IsA(Class))
					{
						ParentProp->CopyCompleteValue(((BYTE*)* It) + ParentProp->Offset, GetReadAddress(ParentProp, this) + ParentProp->Offset);
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
		unguard;
	}
	void OnResetToDefaultsButton()
	{
		guard(FObjectConfigItem::OnResetToDefaultsButton);
		LazyLoadClass();
		if( Class )
		{
			UObject::ResetConfig( Class );
			InvalidateRect( OwnerProperties->List, NULL, 1 );
			UpdateWindow( OwnerProperties->List );
		}
		Redraw();
		unguard;
	}
	void OnItemSetFocus()
	{
		FPropertyItemBase::OnItemSetFocus();
		if (!Parent || !Parent->Buttons.Num())
			AddButton( LocalizeGeneral(TEXT("DefaultsButton"), TEXT("Window")), FDelegate(this,(TDelegate)&FObjectConfigItem::OnResetToDefaultsButton) );
	}
	void Expand()
	{
		guard(FObjectConfigItem::Expand);
		LazyLoadClass();
		if (Class)
		{
			if (Children.Num() == 0)
			{
				INT NumCategories = 0;
				TArray<FPreferencesInfo> NewPrefs;
				UObject::GetPreferences(NewPrefs, *Caption, 0);
				for (INT i = 0; i < NewPrefs.Num(); i++)
				{
					INT j;
					for (j = 0; j < Children.Num(); j++)
					{
						if (appStricmp(*Children(j)->GetCaption(), *NewPrefs(i).Caption) == 0)
							break;
					}
					if (j == Children.Num())
					{
						if (NewPrefs(i).Class != TEXT(""))
						{
							Children.AddItem(new FObjectConfigItem(OwnerProperties, this, *NewPrefs(i).Caption, *NewPrefs(i).Class, NewPrefs(i).Immediate, NewPrefs(i).Category));
							NumCategories++;
						}

						// STUFF NEEDS TO BE MOVED INSIDE CPP FILE TO BREAK CIRCULAR DEPENDENCY.
						//else
						//{
						//Children.AddItem( new FConfigItem( NewPrefs(i), OwnerProperties, this ) );
						//NumCategories++;
						//}
					}
				}
				Class->GetDefaultObject()->LoadConfig(1);//!!
				for (TFieldIterator<UProperty> It(Class); It; ++It)
				{
					if
						((AcceptFlags(It->PropertyFlags))
							&& (Class == It->GetOwnerClass() || !(It->PropertyFlags & CPF_GlobalConfig))
							&& (CategoryFilter == NAME_None || It->Category == CategoryFilter))
						Children.AddItem(new FPropertyItem(OwnerProperties, this, *It, It->GetFName(), It->Offset, -1));
				}
				// Sort categories and properties seperate, so categories stay on top.
				if (Sorted && Children.Num() > 0)
				{
					if (NumCategories > 1)
						Sort(&Children(0), NumCategories);
					if (NumCategories < Children.Num() - 1)
						Sort(&Children(NumCategories), Children.Num() - NumCategories);
				}

				UBOOL SavedSorted = 0;
				Exchange(SavedSorted, Sorted);
				FTreeItem::Expand();
				Exchange(SavedSorted, Sorted);
			}
			else
			{
				FTreeItem::Expand(); // Causes sort. Does it even matter here?
			}
		}
		else
		{
			Expandable = 0;
			Redraw();
		}
		unguard;
	}

	// FObjectConfigItem interface.
	void LazyLoadClass()
	{
		guard(FObjectConfigItem::LazyLoadClass);
		if( !Class && !Failed )
		{
			Class = UObject::StaticLoadClass( UObject::StaticClass(), NULL, *ClassName, NULL, LOAD_NoWarn, NULL );
			if( !Class )
			{
				Failed = 1;
				Caption = FString::Printf( LocalizeError("FailedConfigLoad",TEXT("Window")), *ClassName );
			}
		}
		unguard;
	}

	// FTreeItem interface.
	void Serialize( FArchive& Ar )
	{
		guard(FObjectConfigItem::Serialize);
		FPropertyItemBase::Serialize( Ar );
		Ar << CategoryFilter << Class;
		unguard;
	}
	UObject* GetParentOuter()
	{
		guard(FObjectConfigItem::GetParentOuter);
		UObject* Outer = Class;
		while (Outer->GetOuter())
			Outer = Outer->GetOuter();
		return Outer;
		unguard;
	}
	void SaveObjectConfig()
	{
		guard(FObjectConfigItem::SaveObjectConfig);
		Class->GetDefaultObject()->SaveConfig();
		unguard;
	}
};

// An configuration list item.
class WINDOW_API FConfigItem : public FHeaderItem
{
public:
	// Variables.
	FPreferencesInfo Prefs;

	// Constructors.
	FConfigItem() = default;

	FConfigItem( const FPreferencesInfo& InPrefs, WPropertiesBase* InOwnerProperties, FTreeItem* InParent )
	: FHeaderItem( InOwnerProperties, InParent, 1 )
	, Prefs( InPrefs )
	{}

	// FTreeItem interface.
	QWORD GetId() const
	{
		guard(FConfigItem::GetId);
		return appStrihash(*GetCaption()) + ((QWORD)3<<32);
		unguard;
	}
	virtual FString GetCaption() const
	{
		guard(FConfigItem::GetText);
		return Prefs.Caption;
		unguard;
	}
	void Expand()
	{
		guard(FConfigItem::Expand);
		TArray<FPreferencesInfo> NewPrefs;
		UObject::GetPreferences( NewPrefs, *Prefs.Caption, 0 );
		for( INT i=0; i<NewPrefs.Num(); i++ )
		{
			INT j;
			for( j=0; j<Children.Num(); j++ )
			{
				if( appStricmp( *Children(j)->GetCaption(), *NewPrefs(i).Caption )==0 )
					break;
			}
			if( j==Children.Num() )
			{
				if( NewPrefs(i).Class!=TEXT("") )
					Children.AddItem( new FObjectConfigItem( OwnerProperties, this, *NewPrefs(i).Caption, *NewPrefs(i).Class, NewPrefs(i).Immediate, NewPrefs(i).Category ) );
				else
					Children.AddItem( new FConfigItem( NewPrefs(i), OwnerProperties, this ) );
			}
		}
		FTreeItem::Expand();
		unguard;
	}
	void Collapse()
	{
		guard(FConfigItem::Collapse);
		FTreeItem::Collapse();
		EmptyChildren();
		unguard;
	}
};

// Configuration properties.
class WINDOW_API WConfigProperties : public WProperties
{
	W_DECLARE_CLASS(WConfigProperties,WProperties,CLASS_Transient);
	DECLARE_WINDOWCLASS(WConfigProperties,WProperties,Window)

	// Variables.
	FConfigItem Root;

	// Structors.
	WConfigProperties()	= default;

	WConfigProperties( FName InPersistentName, const TCHAR* InTitle )
	:	WProperties	( InPersistentName )
	,	Root		( FPreferencesInfo(), this, NULL )
	{
		Root.Prefs.Caption = InTitle;
	}

	// WPropertiesBase interface.
	FTreeItem* GetRoot()
	{
		guard(WConfigProperties::GetRoot);
		return &Root;
		unguard;
	}
};

// New class in 227j
class WINDOW_API FEditObjectItem : public FHeaderItem
{
public:
	// Variables.
	UObject** EditClassPtr;
	UObject* OldObjectPtr;
	UObjectProperty* Property;
	UClassProperty* ClassProperty;
	WComboBox* ComboControl;
	WLabel* HolderControl;

	// Constructors.
	FEditObjectItem(WPropertiesBase* InOwnerProperties, FTreeItem* InParent, UObject** InObject, UProperty* InProp)
		: FHeaderItem(InOwnerProperties, InParent, (*InObject != NULL && !Cast<UClassProperty>(InProp)))
		, EditClassPtr(InObject)
		, OldObjectPtr(*InObject)
		, Property(CastChecked<UObjectProperty>(InProp))
		, ClassProperty(Cast<UClassProperty>(InProp))
		, ComboControl(NULL)
		, HolderControl(NULL)
	{
		check(EditClassPtr);
	}

	void Draw(HDC hDC)
	{
		guard(FEditObjectItem::Draw);
		if (OldObjectPtr != *EditClassPtr)
		{
			OldObjectPtr = *EditClassPtr;
			Expandable = (!ClassProperty && *EditClassPtr != NULL);
			Collapse();
		}
		FHeaderItem::Draw(hDC);
		
		unguard;
	}

	BYTE* GetReadAddress(UProperty* InProperty, FTreeItem* Top, INT Index = INDEX_NONE)
	{
		guard(FEditObjectItem::GetReadAddress);
		return (BYTE*)* EditClassPtr;
		unguard;
	}

	// FTreeItem interface.
	void Serialize(FArchive& Ar)
	{
		guard(FEditObjectItem::Serialize);
		FHeaderItem::Serialize(Ar);
		Ar << *EditClassPtr;
		unguard;
	}
	QWORD GetId() const
	{
		guard(FEditObjectItem::GetId);
		return appStrihash(*GetCaption()) + ((QWORD)4 << 32);
		unguard;
	}
	virtual FString GetCaption() const
	{
		guard(FEditObjectItem::GetText);
		//if (ComboControl && !ClassProperty && (Property->PropertyFlags & CPF_EditInLine))
			//return FString::Printf(TEXT("New [%ls]"), *ComboControl->GetString(ComboControl->GetCurrent()));
		return FObjectFullName(*EditClassPtr);
		unguard;
	}
	void Expand()
	{
		guard(FEditObjectItem::Expand);
		if (*EditClassPtr && !ClassProperty)
		{
			// Expand to show categories.
			TArray<FName> Categories;
			UBOOL bHasHidden = 0;
			for (TFieldIterator<UProperty> It((*EditClassPtr)->GetClass()); It; ++It)
			{
				if (AcceptFlags(It->PropertyFlags) && It->Category != NAME_None)
					Categories.AddUniqueItem(It->Category);
				else bHasHidden = 1;
			}
			for (INT i = 0; i < Categories.Num(); i++)
				Children.AddItem(new FCategoryItem(OwnerProperties, this, (*EditClassPtr)->GetClass(), Categories(i), 1));
			if (bHasHidden)
				Children.AddItem(new FHiddenCategoryList(OwnerProperties, this, (*EditClassPtr)->GetClass()));
		}
		FTreeItem::Expand();
		unguard;
	}
	void Collapse()
	{
		guard(FEditObjectItem::Collapse);
		FTreeItem::Collapse();
		EmptyChildren();
		unguard;
	}
	void SetFocusToItem()
	{
		guard(FEditObjectItem::SetFocusToItem);
		if (ComboControl)
			SetFocus(*ComboControl);
		unguard;
	}
	void OnItemSetFocus()
	{
		guard(FEditObjectItem::OnItemSetFocus);
		FTreeItem::OnItemSetFocus();
		//BYTE bEditInLine = ((Property->PropertyFlags & CPF_EditInLine) != 0 && !ClassProperty);
		BYTE bEditConst = ((Property->PropertyFlags & CPF_EditConst) != 0);
		if (!bEditConst && ((!*EditClassPtr && GIsEditor) /*|| !bEditInLine*/))
		{
			//if (bEditInLine)
				//AddButton(TEXT("New"), FDelegate(this, (TDelegate)& FEditObjectItem::OnCreateNew));

			// Combo box.
			FRect Rect = GetRect() + FRect(0, 0, -1 - ButtonWidth, -1).DPIScaled(WWindow::DPIX, WWindow::DPIY);
			Rect.Min.X = OwnerProperties->GetDividerWidth();

			HolderControl = new WLabel(&OwnerProperties->List);
			HolderControl->Snoop = this;
			HolderControl->OpenWindow(0);
			FRect HolderRect = (Rect.Right(20) + FRect(0, 0, 0, 1)).DPIScaled(WWindow::DPIX, WWindow::DPIY);
			HolderControl->MoveWindow(HolderRect, 0);

			Rect = Rect + FRect(-2, -6, -2, 0).DPIScaled(WWindow::DPIX, WWindow::DPIY) - HolderRect.Min;

			ComboControl = new WComboBox(HolderControl);
			ComboControl->Snoop = this;
			ComboControl->SelectionEndOkDelegate = FDelegate(this, (TDelegate)& FEditObjectItem::ComboSelectionEnd);
			ComboControl->SelectionEndCancelDelegate = FDelegate(this, (TDelegate)& FEditObjectItem::ComboSelectionEndCancel);
			ComboControl->OpenWindow(0);
			ComboControl->MoveWindow(Rect, 1);

			// Find classes matching this property.
			UClass* ParentClass = (Property->PropertyClass ? Property->PropertyClass : UObject::StaticClass());
			/*
			if (bEditInLine)
			{
				for (TObjectIterator<UClass> It; It; ++It)
				{
					if (!(It->ClassFlags & (CLASS_Abstract | CLASS_NoUserCreate | CLASS_Transient)) && It->IsChildOf(ParentClass))
						ComboControl->AddString(*FObjectPathName(*It));
				}
				if (ComboControl->GetCount())
					ComboControl->SetCurrent(0);
			}
			else
			{*/
				ComboControl->AddString(TEXT("None"));
				if (ClassProperty)
				{
					ParentClass = ClassProperty->MetaClass;
					for (TObjectIterator<UClass> It; It; ++It)
					{
						if (!ParentClass || It->IsChildOf(ParentClass))
							ComboControl->AddString(*FObjectPathName(*It));
					}
				}
				else
				{
					for (FObjectIterator It; It; ++It)
					{
						if (It->GetClass()->IsChildOf(ParentClass))
							ComboControl->AddString(*FObjectPathName(*It));
					}
				}
				MatchComboSelection();
			//}

			Redraw();

			if (ComboControl)
				ComboControl->Show(1);
			if (HolderControl)
				HolderControl->Show(1);
			SetFocusToItem();
		}
		unguard;
	}
	void MatchComboSelection()
	{
		guard(FEditObjectItem::MatchComboSelection);
		ComboControl->SetCurrent(ComboControl->FindStringExact(*FObjectPathName(*EditClassPtr)));
		unguard;
	}
	void OnItemKillFocus(UBOOL Abort)
	{
		guard(FEditObjectItem::OnKillFocus);
		if (ComboControl)
		{
			ComboControl->Show(0);
			ComboControl->DelayedDestroy();
		}
		ComboControl = NULL;
		if (HolderControl)
		{
			HolderControl->Show(0);
			HolderControl->DelayedDestroy();
		}
		HolderControl = NULL;
		FTreeItem::OnItemKillFocus(Abort);
		unguard;
	}
	void ComboSelectionEnd()
	{
		guard(FEditObjectItem::ComboSelectionEnd);
		if (ClassProperty)
			ChangePtr(FindObject<UClass>(ANY_PACKAGE, *ComboControl->GetString(ComboControl->GetCurrent()), 1));
		else //if (!(Property->PropertyFlags & CPF_EditInLine))
			ChangePtr(FindObject<UObject>(ANY_PACKAGE, *ComboControl->GetString(ComboControl->GetCurrent())));
		Redraw();
		unguard;
	}
	void ComboSelectionEndCancel()
	{
		guard(FEditObjectItem::ComboSelectionEndCancel);
		if (/*!(Property->PropertyFlags & CPF_EditInLine) || */ClassProperty)
			MatchComboSelection();
		Redraw();
		unguard;
	}
	void OnCreateNew()
	{
		guard(FEditObjectItem::OnCreateNew);
		check(ComboControl);
		FString Value = ComboControl->GetString(ComboControl->GetCurrent());
		UClass* Class = FindObject<UClass>(NULL, *Value, 1);
		if (Class)
		{
			ChangePtr(ConstructObject<UObject>(Class, GetParentOuter()));
			if (ComboControl)
			{
				ComboControl->Show(0);
				ComboControl->DelayedDestroy();
			}
			ComboControl = NULL;
			if (HolderControl)
			{
				HolderControl->Show(0);
				HolderControl->DelayedDestroy();
			}
			HolderControl = NULL;
			Redraw();
		}
		unguard;
	}
	void SetProperty(UProperty* Property, FTreeItem* Offset, const TCHAR* Value)
	{
		guard(FEditObjectItem::SetProperty);
		if (*EditClassPtr)
		{
			if (OwnerProperties->NotifyHook)
				OwnerProperties->NotifyHook->NotifyPreChange(OwnerProperties);
			Property->ImportText(Value, Offset->GetReadAddress(Property, this), PPF_Localized);
			(*EditClassPtr)->PostEditChange();
			if (OwnerProperties->NotifyHook)
				OwnerProperties->NotifyHook->NotifyPostChange(OwnerProperties);
		}
		unguard;
	}
	void ChangePtr(UObject* NewObj)
	{
		guard(FEditObjectItem::ChangePtr);
		if (GIsEditor)
			Parent->SetProperty(Property, Parent, *FObjectPathName(NewObj)); // Allow undo.
		*EditClassPtr = NewObj;
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	WWizardPage.
-----------------------------------------------------------------------------*/

// A wizard page.
class WINDOW_API WWizardPage : public WDialog
{
	W_DECLARE_ABSTRACT_CLASS(WWizardPage,WDialog,CLASS_Transient);
	DECLARE_WINDOWCLASS(WWizardPage,WDialog,Window)

	// Variables.
	WWizardDialog* Owner{};

	// Constructor.
	WWizardPage() = default;

	WWizardPage( const TCHAR* PageName, INT ControlId, WWizardDialog* InOwner )
	: WDialog	    ( PageName, ControlId, (WWindow*)InOwner )
	, Owner         ( InOwner )
	{}

	// WWizardPage interface.
	virtual void OnCurrent()
	{}
	virtual WWizardPage* GetNext()
	{
		return NULL;
	}
	virtual const TCHAR* GetBackText()
	{
		return LocalizeGeneral(TEXT("BackButton"), TEXT("Window"));
	}
	virtual const TCHAR* GetNextText()
	{
		return LocalizeGeneral(TEXT("NextButton"), TEXT("Window"));
	}
	virtual const TCHAR* GetFinishText()
	{
		return NULL;
	}
	virtual const TCHAR* GetCancelText()
	{
		return LocalizeGeneral(TEXT("CancelButton"), TEXT("Window"));
	}
	virtual UBOOL GetShow()
	{
		return 1;
	}
	virtual void OnCancel()
	{
		guard(WWizardPage::OnCancel);
		((WDialog*)Owner)->EndDialog( 0 );
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	WWizardDialog.
-----------------------------------------------------------------------------*/

// The wizard frame dialog.
class WINDOW_API WWizardDialog : public WDialog
{
	W_DECLARE_CLASS(WWizardDialog,WDialog,CLASS_Transient);
	DECLARE_WINDOWCLASS(WWizardDialog,WDialog,Window)

	// Variables.
	WCoolButton BackButton;
	WCoolButton NextButton;
	WCoolButton FinishButton;
	WCoolButton CancelButton;
	WLabel PageHolder;
	TArray<WWizardPage*> Pages;
	WWizardPage* CurrentPage;

	// Constructor.
	WWizardDialog()
	: WDialog	    ( TEXT("WizardDialog"), IDDIALOG_WizardDialog )
	, BackButton    ( this, IDC_Back,   FDelegate(this,(TDelegate)&WWizardDialog::OnBack  ) )
	, NextButton    ( this, IDC_Next,   FDelegate(this,(TDelegate)&WWizardDialog::OnNext  ) )
	, FinishButton  ( this, IDC_Finish, FDelegate(this,(TDelegate)&WWizardDialog::OnFinish) )
	, CancelButton  ( this, IDCANCEL,   FDelegate(this,(TDelegate)&WWizardDialog::OnCancel) )
	, PageHolder    ( this, IDC_PageHolder )
	, CurrentPage   ( NULL )
	{}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WWizardDialog::OnInitDialog);
		WDialog::OnInitDialog();
		SendMessageW( *this, WM_SETICON, ICON_BIG, (WPARAM)LoadIconW(hInstance,MAKEINTRESOURCEW((GIsEditor?IDICON_Editor:IDICON_Mainframe))) );
		RefreshPage();
		unguard;
	}

	// WWizardDialog interface.
	virtual void Advance( WWizardPage* NewPage )
	{
		guard(WWizardDialog::Advanced);
		check(NewPage);
		Pages.AddItem( NewPage );
		if( hWnd )
			RefreshPage();
		if( !Pages.Last()->GetShow() )
			OnNext();
		unguard;
	}
	virtual void RefreshPage()
	{
		guard(WWizardDialog::RefreshPage);
		if( Pages.Num() )
		{
			WWizardPage* Page = Pages.Last();
			if( !Page->hWnd )
			{
				Page->OpenChildWindow( 0, 1 );
				// resize and move after, for make proper tab order
				FRect R = PageHolder.GetWindowRect();
				SetWindowPos(Page->hWnd, PageHolder.hWnd, R.Min.X, R.Min.Y, R.Width(), R.Height(), 0);
				PageHolder.Show(0);
			}
			NextButton  .SetVisibleText( Page->GetNextText  () );
			FinishButton.SetVisibleText( Page->GetFinishText() );
			BackButton  .SetVisibleText( Pages.Num()>1 ? Page->GetBackText() : NULL );
			CancelButton.SetVisibleText( Page->GetCancelText() );

			// set proper tab order
			SetWindowPos(NextButton.hWnd, PageHolder.hWnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			SetWindowPos(FinishButton.hWnd, NextButton.hWnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			SetWindowPos(BackButton.hWnd, FinishButton.hWnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			SetWindowPos(CancelButton.hWnd, BackButton.hWnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			
			if( Pages.Num()>1 )
				Pages(Pages.Num()-2)->Show(0);

			// move focus after Cancel button, by send TAB press.
			// This lead to select first usable control in page, or select Next button instead.
			if (GetFocus() != CancelButton.hWnd) // prevent send tab press few times
			{
				SetFocus(CancelButton.hWnd);
				if (GetFocus() == CancelButton.hWnd)
					PostMessageW(CancelButton.hWnd, WM_KEYDOWN, VK_TAB, 0);
			}
			
			Pages.Last()->OnCurrent();

			// activate focus
			SetFocus(GetFocus());
		}
		unguard;
	}
	virtual void OnCommand(INT Command)
	{
		guard(WWizardDialog::OnCommand);
		WPARAM wParam = LastwParam;	
		LPARAM lParam = LastlParam;
		WDialog::OnCommand(Command);
		// propagate events to page
		if (Pages.Num())
			SendMessageW(Pages.Last()->hWnd, WM_NOTIFY, wParam, lParam);
		unguard;
	}
	virtual void OnDestroy()
	{
		guard(WWizardDialog::OnDestroy);
		for( INT i=0; i<Pages.Num(); i++ )
			delete Pages(i);
		WDialog::OnDestroy();
		unguard;
	}
	virtual void OnBack()
	{
		guard(WWizardDialog::OnBack);
		if( Pages.Num()>1 )
		{
			Pages(Pages.Num()-2)->Show(1);
			delete Pages.Pop();
			RefreshPage();
			if( !Pages.Last()->GetShow() )
				OnBack();
		}
		unguard;
	}
	virtual void OnNext()
	{
		guard(WWizardDialog::OnNext);
		if( Pages.Num() && Pages.Last()->GetNextText() )
		{
			WWizardPage* GotNext = Pages.Last()->GetNext();
			if( GotNext )
				Advance( GotNext );
		}
		unguard;
	}
	virtual void OnFinish()
	{
		guard(WWizardDialog::OnFinish);
		EndDialog( 1 );
		unguard;
	}
	virtual void OnCancel()
	{
		guard(WWizardDialog::OnCancel);
		Pages.Last()->OnCancel();
		unguard;
	}
	void OnClose()
	{
		guard(WWizardDialog::OnClose);
		Pages.Last()->OnCancel();
		throw TEXT("NoRoute");
		unguard;
	}
};

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (pop)
#endif

class WINDOW_API UWindowManager : public USubsystem
{
	DECLARE_CLASS(UWindowManager, UObject, CLASS_Transient, Window);

	// Constructor.
	UWindowManager();

	// FExec interface.
	UBOOL Exec(const TCHAR* Cmd, FOutputDevice& Ar);

	// UObject interface.
	void Serialize(FArchive& Ar);
	void Destroy();

	// USubsystem interface.
	void Tick(FLOAT DeltaTime);
};

#if __STATIC_LINK
#define AUTO_INITIALIZE_REGISTRANTS_WINDOW \
	UWindowManager::StaticClass();
#endif

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
