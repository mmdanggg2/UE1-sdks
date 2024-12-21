/*=============================================================================
	SDL2Drv.h: Simple Directmedia Layer cross-platform driver.
	Copyright 2002 Epic Games, Inc. All Rights Reserved.

        SDL website: http://www.libsdl.org/

Revision history:
	* Created by Ryan C. Gordon, based on WinDrv.
      This is an updated rewrite of the original SDLDrv.
	* Updated to SDL2 with some improvements by Smirftsch www.oldunreal.com
=============================================================================*/

#ifndef _INC_SDL2DRV
#define _INC_SDL2DRV

/*----------------------------------------------------------------------------
	API.
----------------------------------------------------------------------------*/

#ifndef SDL2DRV_API
	#define SDL2DRV_API DLL_IMPORT
#endif

#ifndef FALSE
#define FALSE               0
#endif

#ifndef TRUE
#define TRUE                1
#endif

/*----------------------------------------------------------------------------
	Dependencies.
----------------------------------------------------------------------------*/

#include "SDL.h"

// Unreal includes.
#include "Engine.h"
#include "UnRender.h"

/*-----------------------------------------------------------------------------
	Declarations.
-----------------------------------------------------------------------------*/

// Classes.
class USDL2Viewport;
class USDL2Client;

/*-----------------------------------------------------------------------------
	USDL2Client.
-----------------------------------------------------------------------------*/

//
// SDL implementation of the client.
//
class USDL2Client : public UClient, public FNotifyHook
{
	DECLARE_CLASS(USDL2Client,UClient,CLASS_Transient|CLASS_Config,SDL2Drv)

	// Configuration.
	UBOOL				UseJoystick;
	UBOOL				StartupFullscreen;
	INT                 JoystickNumber;
	INT                 JoystickHatNumber;
    INT                 WindowPosX;
    INT                 WindowPosY;
	FLOAT               ScaleJBX;
	FLOAT               ScaleJBY;
	UBOOL               IgnoreHat;
	UBOOL               IgnoreUngrabbedMouse;
	UBOOL               UseRawHIDInput; //dummy, not needed for functionality, but for UMenu.
	UBOOL				UseDesktopFullScreen;
	FStringNoInit       TextToSpeechFile;

	// Variables.
	UBOOL				InMenuLoop;
	UBOOL				ConfigReturnFullscreen;
	INT					NormalMouseInfo[3];
	INT					CaptureMouseInfo[3];

    static SDL_Joystick *Joystick;
    static int JoystickAxes;
    static int JoystickButtons;
    static int JoystickHats;
    static int JoystickBalls;

	// Constructors.
	USDL2Client();
	void StaticConstructor();

	// FNotifyHook interface.
	void NotifyDestroy( void* Src );

	// UObject interface.
	void Destroy();
	void PostEditChange();
	void ShutdownAfterError();

	// UClient interface.
	void Init( UEngine* InEngine );
	void ShowViewportWindows( DWORD ShowFlags, INT DoShow );
	void EnableViewportWindows( DWORD ShowFlags, INT DoEnable );
	UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar=*GLog );
	void Tick();
	void MakeCurrent( UViewport* InViewport );
	UViewport* GetLastCurrent();
	class UViewport* NewViewport( const FName Name );

	// DPI Awareness.
	INT GetDPIScaledX(INT X);
	INT GetDPIScaledY(INT Y);
};

/*-----------------------------------------------------------------------------
	USDL2Viewport.
-----------------------------------------------------------------------------*/

//
// An SDL viewport.
//
class USDL2Viewport : public UViewport
{
	DECLARE_CLASS(USDL2Viewport,UViewport,CLASS_Transient,SDL2Drv)
	DECLARE_WITHIN(USDL2Client)

	// Variables.
	DWORD				BlitFlags;

    UBOOL				LostGrab;
    UBOOL				LostFullscreen;

	// Where did we last enter/leave the window?
	INT                 MouseLeaveX;
	INT                 MouseLeaveY;
	INT                 MouseEnterX;
	INT                 MouseEnterY;

	// SDL Keysym to EInputKey map.
	BYTE				KeysymMap[512];

	// KeyRepeatKey.
	bool                KeyIsPressed[SDL_NUM_SCANCODES];
	const Uint8         *KeyboardState;
	bool                bWaitForKeyUp;
	Uint8               JoyState;
	UBOOL               bTyping;
	INT                 KeyRepeatKey;
    EInputKey           ConsoleKey;
	EInputKey           UWindowKey;
	EInputKey			LastKey;

	// Joystick Hat Hack
	EInputKey 			LastJoyHat;

	UBOOL				MouseIsGrabbed;
	INT					TextToSpeechObject;  // file descriptor for named pipe.
	DOUBLE              MouseScaleX;
	DOUBLE              MouseScaleY;

	//MainScreen
	SDL_Renderer*		Renderer;
	SDL_Surface*		ScreenSurface;
	SDL_Window*			Window;
	SDL_DisplayMode*	Mode;

	FTime LastUpdateTime;

	// Constructor.
	USDL2Viewport();

	// UObject interface.
	void Destroy();
	void ShutdownAfterError();

	// UViewport interface.
	UBOOL Lock( FPlane FlashScale, FPlane FlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* HitData, INT* HitSize);
	UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar );
	UBOOL ResizeViewport( DWORD BlitFlags, INT NewX=INDEX_NONE, INT NewY=INDEX_NONE, INT NewColorBytes=INDEX_NONE );
	UBOOL IsFullscreen();
	void Unlock( UBOOL Blit );
	void Repaint( UBOOL Blit );
	void SetModeCursor();
	void SetTitleBar();
	void UpdateWindowFrame();
	void OpenWindow( void* ParentWindow, UBOOL Temporary, INT NewX, INT NewY, INT OpenX, INT OpenY, const TCHAR* ForcedRenDevClass);
	void CloseWindow();
	void UpdateInput( UBOOL Reset );
	void* GetWindow();
	void SetMouseCapture( UBOOL Capture, UBOOL Clip, UBOOL FocusOnly );

	// USDL2Viewport interface.
	void UpdateMouseGrabState(const UBOOL ShouldGrab);
	void ToggleFullscreen();
	void EndFullscreen();
	void SetTopness();
	DWORD GetViewportButtonFlags( DWORD wParam );

	UBOOL CauseInputEvent( INT iKey, EInputAction Action, FLOAT Delta=0.0 );

	virtual void TryRenderDevice( const TCHAR* ClassName, INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen );
    void InitSplash( const TCHAR* Filename );
	void ExitSplash();

	TCHAR* GetLocalizedKeyName( EInputKey Key );
	void GetMouseState(INT* MouseX, INT* MouseY);
};


#define AUTO_INITIALIZE_REGISTRANTS_SDL2DRV \
	USDL2Viewport::StaticClass(); \
	USDL2Client::StaticClass();

#endif //_INC_SDL2DRV
/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

