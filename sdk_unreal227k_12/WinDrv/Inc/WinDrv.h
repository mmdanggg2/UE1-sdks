/*=============================================================================
	WinDrvPrivate.cpp: Unreal Windows viewport and platform driver.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Tim Sweeney.
=============================================================================*/

#ifndef _INC_WINDRV
#define _INC_WINDRV

/*----------------------------------------------------------------------------
	API.
----------------------------------------------------------------------------*/

#ifndef WINDRV_API
	#define WINDRV_API DLL_IMPORT
#endif

/*----------------------------------------------------------------------------
	Dependencies.
----------------------------------------------------------------------------*/

// Windows includes.
#pragma warning(push)
#pragma warning(disable: 4121) /* 'JOBOBJECT_IO_RATE_CONTROL_INFORMATION_NATIVE_V2': alignment of a member was sensitive to packing */
#pragma warning(disable: 4091) 
//#define STRICT
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#if __STATIC_LINK
#include <joystickapi.h>
#endif
#pragma warning(pop)
#define DIRECTINPUT_VERSION 0x0800
#include <ddraw.h>
#include <dinput.h>
#include "Res\WinDrvRes.h"

// Unreal includes.
#include "Engine.h"
#include "Window.h"
#include "UnRender.h"

/*-----------------------------------------------------------------------------
	Declarations.
-----------------------------------------------------------------------------*/

// Classes.
class UWindowsViewport;
class UWindowsClient;

// Global functions.
WINDRV_API const TCHAR* ddError( HRESULT Result );
WINDRV_API const TCHAR* diError( HRESULT Result );

/*-----------------------------------------------------------------------------
	UWindowsInputHandler.
-----------------------------------------------------------------------------*/

class WINDRV_API FWindowsMouseInputHandler
{
public:
	// Constructors/Destructors
	virtual ~FWindowsMouseInputHandler() = default;

	//
	// The ProcessInputEvent and PollInputs functions below just update
	// the FWindowsMouseInputHandler state. We don't apply said state to
	// the game until we call ProcessInputUpdates.
	//
	void ProcessInputUpdates(UWindowsViewport* Viewport);

	//
	// Common Win32 mouse message handler
	//
	LRESULT ProcessCommonEvent(UWindowsViewport* Viewport, UINT iMessage, WPARAM wParam, LPARAM lParam);

	// Input API-specific functionality.
	virtual UBOOL SetupInput() { return TRUE; }
	virtual void ShutdownInput() {}
	virtual void RegisterViewport(UWindowsViewport* Viewport, UBOOL StartCaptured) {}
	virtual LRESULT ProcessInputEvent(UWindowsViewport* Viewport, UINT iMessage, WPARAM wParam, LPARAM lParam) { return 0; }
	virtual void PollInputs(UWindowsViewport* Viewport) {}
	virtual void AcquireMouse(UWindowsViewport* Viewport) {}
	virtual void ReleaseMouse(UWindowsViewport* Viewport) {}
	virtual void ResetMouseState();

	//
	// Relative mouse motion APIs (DirectInput and Raw Input) don't activate until
	// we capture the mouse. Capturing often starts in response to a mouse click
	// delivered through a Window Message. This means all APIs must process
	// click messages.
	//
	// This helper function maps click message codes onto ButtonStates. We use this
	// to write the LastClickLocation into the ButtonState.
	//
	virtual INT GetButtonStateIndex(UINT WindowMessage, WPARAM wParam);

	struct ButtonState
	{
		ButtonState() = default;

		ButtonState(DWORD SelectionMask, DWORD CompareMask, EInputKey EventKey, EMouseButtons ButtonFlag, UBOOL _EnablesDrag)
			: ButtonStateSelectionMask(SelectionMask)
			, ButtonStateCompareMask(CompareMask)
			, InputEventKey(EventKey)
			, ViewportButtonFlag(ButtonFlag)
			, EnablesDrag(_EnablesDrag)
		{}

		//
		// The button is held down if (RawButtonData & ButtonStateSelectionMask) == ButtonStateCompareMask
		//
		DWORD ButtonStateSelectionMask{};
		DWORD ButtonStateCompareMask{};

		//
		// InputKey to pass to CauseInputEvent if the button state changes
		//
		EInputKey InputEventKey{};

		//
		// Flag to pass to Click/MouseDelta (in UED)
		//
		EMouseButtons ViewportButtonFlag{};

		//
		// Where was the cursor when we clicked?
		//
		POINT LastClickLocation{};
		UBOOL HaveLastClickLocation{};

		//
		// Set to true if clicking this button should enable dragging
		//
		UBOOL EnablesDrag{};

		//
		// Cached state
		//
		UBOOL IsDown{};
		UBOOL WasDown{};
		LONG CumulativeMovementSinceLastClick{};
		FTime LastClickTime{};

	} ButtonStates[10];
	INT NumButtons{};
	UBOOL ButtonsClicked{};

	// Track cursor and mouse wheel status
	POINT NewCursorPos{};
	POINT OldCursorPos{};
	UBOOL CursorMoved{};
	UBOOL RelativeMouseMotion{};

	LONG NewWheelPos{};
	LONG OldWheelPos{};
	UBOOL WheelMoved{};
};

class WINDRV_API FWindowsWin32InputHandler : public FWindowsMouseInputHandler
{
public:
	// InputHandler interface.
	UBOOL SetupInput();
	LRESULT ProcessInputEvent(UWindowsViewport* Viewport, UINT iMessage, WPARAM wParam, LPARAM lParam);
	void PollInputs(UWindowsViewport* Viewport);

	// Win32-API specific.
	void WarpMouseIfNecessary(UWindowsViewport* Viewport, UBOOL Force = TRUE);
	void AcquireMouse(UWindowsViewport* Viewport);
	void ReleaseMouse(UWindowsViewport* Viewport);

	//
	// stijn: set to true if the captured windows cursor has approached
	// the border of the window and we've issued a cursor reset via SetCursorPos
	//
	UBOOL MouseResetPending{};

	//
	// stijn: position we've moved the cursor to using SetCursorPos.
	// The first relative coordinates we calculate after the reset should be relative to
	// the mouse reset target coordinates
	//
	POINT MouseResetTarget{};

	//
	// stijn: Cursor position after the previous tick
	//
	POINT SavedLockedCursor{};

	//
	// stijn: Are we clipping the cursor so it stays within the viewport boundaries?
	//
	UBOOL bClippingEnabled{};
};

class WINDRV_API FWindowsLegacyWin32InputHandler : public FWindowsMouseInputHandler
{
public:
	// InputHandler interface.
	UBOOL SetupInput();
	LRESULT ProcessInputEvent(UWindowsViewport* Viewport, UINT iMessage, WPARAM wParam, LPARAM lParam);
	void PollInputs(UWindowsViewport* Viewport);
	void AcquireMouse(UWindowsViewport* Viewport);
	void ReleaseMouse(UWindowsViewport* Viewport);

	// Win32-API specific.
	void WarpMouse(UWindowsViewport* Viewport, BOOL FlushPendingInput);

	//
	// stijn: kernel tick count at the time of the reset. Whenever we reset the mouse, we should discard all
	// subsequent messages that have a time stamp lower than the reset time!
	//
	DWORD MouseResetTime{};

	//
	// stijn: position we've moved the cursor to using SetCursorPos.
	// The first relative coordinates we calculate after the reset should be relative to
	// the mouse reset target coordinates
	//
	POINT MouseResetTarget{};
};

class WINDRV_API FWindowsDirectInputHandler : public FWindowsMouseInputHandler
{
public:
	// DirectInput Constants.
	typedef HRESULT(WINAPI* DI_CREATE_FUNC)(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter);

	// InputHandler interface.
	UBOOL SetupInput();
	void ShutdownInput();
	void RegisterViewport(UWindowsViewport* Viewport, UBOOL StartCaptured);
	LRESULT ProcessInputEvent(UWindowsViewport* Viewport, UINT iMessage, WPARAM wParam, LPARAM lParam);
	void PollInputs(UWindowsViewport* Viewport);
	void AcquireMouse(UWindowsViewport* Viewport);
	void ReleaseMouse(UWindowsViewport* Viewport);

	static IDirectInput8* di;
	static DI_CREATE_FUNC	diCreateFunc;
	IDirectInputDevice8* diMouse{};
	DIDEVICEOBJECTDATA* diMouseBuffer{};
	DIMOUSESTATE2			diMouseState{};
	UBOOL					diAcquiredMouse{};
	POINT					SavedCursor{};
};

class WINDRV_API FWindowsRawInputHandler : public FWindowsMouseInputHandler
{
public:
	// InputHandler interface.
	UBOOL SetupInput();
	void ShutdownInput();
	void RegisterViewport(UWindowsViewport* Viewport, UBOOL StartCaptured);
	LRESULT ProcessInputEvent(UWindowsViewport* Viewport, UINT iMessage, WPARAM wParam, LPARAM lParam);
	void PollInputs(UWindowsViewport* Viewport);
	void AcquireMouse(UWindowsViewport* Viewport);
	void ReleaseMouse(UWindowsViewport* Viewport);

	LONG  MouseButtons{};
	SWORD MouseButtonData{};
protected:
	enum { NUM_RAW_DEVICES = 1 };
	RAWINPUTDEVICE RawInputDevices[NUM_RAW_DEVICES]{};
};

/*-----------------------------------------------------------------------------
	UWindowsClient.
-----------------------------------------------------------------------------*/

struct FDeviceInfo
{
	GUID				Guid;
	FString				Description;
	FString				Name;
	FDeviceInfo(GUID InGuid, const TCHAR* InDescription, const TCHAR* InName)
		: Guid(InGuid), Description(InDescription), Name(InName)
	{}
};

//
// Windows implementation of the client.
//
class DLL_EXPORT UWindowsClient : public UClient, public FNotifyHook
{
	DECLARE_CLASS(UWindowsClient, UClient, CLASS_Transient | CLASS_Config, WinDrv);

	// DirectDraw Constants.
	typedef HRESULT(WINAPI* DD_CREATE_FUNC)(GUID* lpGUID, void* lplpDD, REFIID iid, IUnknown* pUnkOuter);
	typedef HRESULT(WINAPI* DD_ENUM_FUNC)(LPDDENUMCALLBACKEXA lpCallback, LPVOID lpContext, DWORD dwFlags);

	// Configuration.
	BITFIELD			InhibitWindowsHotkeys;
	BITFIELD			UseDirectDraw;
	BITFIELD			UseDirectInput;
	BITFIELD			UseRawHIDInput;
	BITFIELD			EnableEditorRawHIDInput;
	BITFIELD			UseLegacyCursorInput; // Broken as of Win10 FCU
	BITFIELD			UseJoystick;
	BITFIELD			NoEnhancedPointerPrecision;
	BITFIELD			StartupFullscreen;
	BITFIELD			DeadZoneXYZ;
	BITFIELD			DeadZoneRUV;
	BITFIELD			InvertVertical;
	FLOAT				ScaleXYZ;
	FLOAT				ScaleRUV;
	INT					WinPosX, WinPosY;

	// Variables.
	HDC					hMemScreenDC;
	UBOOL				InMenuLoop;
	UBOOL				ConfigReturnFullscreen;
	INT					NormalMouseInfo[3];
	INT					CaptureMouseInfo[3];
	IDirectDraw7* dd;
	DD_CREATE_FUNC		ddCreateFunc;
	DD_ENUM_FUNC		ddEnumFunc;
	TArray<FDeviceInfo>	DirectDraws;
	TArray<DDPIXELFORMAT>	PixelFormats;
	TArray<DDPIXELFORMAT>	ZFormats;
	TArray<DDSURFACEDESC2>	DisplayModes;
	UBOOL				UsingRawHIDInput;
	UBOOL				UsingDirectInput;
	UBOOL				bInitialized;

	WConfigProperties* ConfigProperties;
	ATOM				hkAltEsc, hkAltTab, hkCtrlEsc, hkCtrlTab;
	JOYCAPSA			JoyCaps;

	FWindowsMouseInputHandler* MouseInputHandler;

	// Constructors.
	UWindowsClient();
	void StaticConstructor();

	// FNotifyHook interface.
	void NotifyDestroy(void* Src);

	// UObject interface.
	void Destroy();
	void PostEditChange();
	void ShutdownAfterError();

	// UClient interface.
	void Init(UEngine* InEngine);
	void ShowViewportWindows(DWORD ShowFlags, INT DoShow);
	void EnableViewportWindows(DWORD ShowFlags, INT DoEnable);
	UBOOL Exec(const TCHAR* Cmd, FOutputDevice& Ar = *GLog);
	void Tick();
	void MakeCurrent(UViewport* InViewport);
	class UViewport* NewViewport(const FName Name);

#if !OLDUNREAL_BINARY_COMPAT
	FString GetClipboardText();
	UBOOL SetClipboardText(FString& Str);
#endif

	// UWindowsClient interface.
	void ddEndMode();

	// Static functions.
	static BOOL CALLBACK EnumDirectDrawsCallback(GUID* Guid, char* Description, char* Name, void* Context, HMONITOR Monitor);
	static HRESULT CALLBACK EnumModesCallback(DDSURFACEDESC2* Desc, void* Context);
};

/*-----------------------------------------------------------------------------
	UWindowsViewport.
-----------------------------------------------------------------------------*/

//
// Viewport window status.
//
enum EWinViewportStatus
{
	WIN_ViewportOpening	= 0, // Viewport is opening and hWnd is still unknown.
	WIN_ViewportNormal	= 1, // Viewport is operating normally, hWnd is known.
	WIN_ViewportClosing	= 2, // Viewport is closing and CloseViewport has been called.
};

//
// A Windows viewport.
//
class DLL_EXPORT UWindowsViewport : public UViewport
{
	DECLARE_CLASS(UWindowsViewport, UViewport, CLASS_Transient, WinDrv);
	DECLARE_WITHIN(UWindowsClient);

	// Variables.
	class WWindowsViewportWindow* Window;
	EWinViewportStatus  Status;
	HBITMAP				hBitmap;
	HWND				ParentWindow;
	IDirectDrawSurface7* ddFrontBuffer;
	IDirectDrawSurface7* ddBackBuffer;
	DDSURFACEDESC2 		ddSurfaceDesc;
	INT					HoldCount;
	DWORD				BlitFlags;
	UBOOL				Borderless;
	UBOOL				DirectDrawMinimized;
	UUnrealCursor*		PrevCursor;
	HCURSOR				CustomCursorIc;

	// Info saved during captures and fullscreen sessions.
	POINT				SavedCursor;
	FRect				SavedWindowRect;
	INT					SavedColorBytes;
	INT					SavedCaps;
	INT					DesktopColorBytes;
	HCURSOR				StandardCursors[10];

	// Current capture/clipping status
	UBOOL				CapturingMouse;
	UBOOL				ClippingMouse;

	UBOOL				IgnoreMouseWheel; //elmuerte: EM_EXEC workaround
	UBOOL				IgnoreResolutionConfigChange; // stijn: set to true when we want to defer SizeX/SizeY updates while switching in and out of windowed mode with a DXGI renderer

	//
	// stijn: if the mouse moves less than ClickDetectionMovementThreshold px between a mouse button down and up event
	// AND if less than ClickDetectionTimeThreshold seconds pass between the down and up event,
	// then treat the up event as a mouse click
	//
	INT					ClickDetectionMovementThreshold; 
	FLOAT				ClickDetectionTimeThreshold;


	// Constructor.
	UWindowsViewport();
	void StaticConstructor();

	// UObject interface.
	void Destroy();
	void ShutdownAfterError();

	// UViewport interface.
	UBOOL Lock(FPlane FlashScale, FPlane FlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* HitData = NULL, INT* HitSize = 0);
	UBOOL Exec(const TCHAR* Cmd, FOutputDevice& Ar = *GLog);
	UBOOL ResizeViewport(DWORD BlitFlags, INT NewX = INDEX_NONE, INT NewY = INDEX_NONE, INT NewColorBytes = INDEX_NONE);
	UBOOL IsFullscreen();
	void Unlock(UBOOL Blit);
	void Repaint(UBOOL Blit);
	void SetModeCursor();
	void UpdateWindowFrame();
	void OpenWindow(void* ParentWindow, UBOOL Temporary, INT NewX, INT NewY, INT OpenX, INT OpenY, const TCHAR* ForcedDeviceClass = NULL);
	void CloseWindow();
	void* GetWindow();
	void UpdateInput(UBOOL Reset);
	void SetMouseCapture(UBOOL Capture, UBOOL Clip, UBOOL FocusOnly = 0);
	void UpdateMouseCursor(); // Update mouse pointer icon.

	// UWindowsViewport interface.
	LRESULT ViewportWndProc(UINT Message, WPARAM wParam, LPARAM lParam);
	void ToggleFullscreen();
	void EndFullscreen();
	void SetTopness();
	UBOOL CauseInputEvent(INT iKey, EInputAction Action, FLOAT Delta = 0.0);
	UBOOL JoystickInputEvent(FLOAT Delta, EInputKey Key, FLOAT Scale, UBOOL DeadZone);
	UBOOL ddSetMode(INT Width, INT Height, INT ColorBytes);
	void TryRenderDevice(const TCHAR* ClassName, INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen);
	void SetCursorDisplay(UBOOL bShow);

	// OldUnreal additions
	void InitializeDibSection(INT NewX, INT NewY, INT NewColorBytes);
	void MoveWindow(int Left, int Top, int Width, int Height, UBOOL bRepaint);
	void UpdateMouseClippingRegion() const;

	// Static functions.
	static LRESULT CALLBACK KeyboardProc(INT Code, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK SysMsgProc(INT nCode, WPARAM wParam, LPARAM lParam);

#ifdef UTPG_WINXP_FIREWALL
	static UBOOL					CoInitialized;
	virtual UBOOL FireWallHack(INT Cmd);
#endif
};

//
// A windows viewport window.
//
class DLL_EXPORT WWindowsViewportWindow : public WWindow
{
	W_DECLARE_CLASS(WWindowsViewportWindow,WWindow,CLASS_Transient)
	DECLARE_WINDOWCLASS(WWindowsViewportWindow,WWindow,Window)
	class UWindowsViewport* Viewport;
	WWindowsViewportWindow()
	{}
	WWindowsViewportWindow( class UWindowsViewport* InViewport )
	: Viewport( InViewport )
	{}
	LRESULT WndProc( UINT Message, WPARAM wParam, LPARAM lParam )
	{
		return Viewport->ViewportWndProc( Message, wParam, lParam );
	}
};

#if __STATIC_LINK
#define AUTO_INITIALIZE_REGISTRANTS_WINDRV \
	UWindowsClient::StaticClass();\
	UWindowsViewport::StaticClass();

#endif

#endif //_INC_WINDRV
/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
