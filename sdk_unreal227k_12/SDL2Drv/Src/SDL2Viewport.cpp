/*=============================================================================
	USDL2Viewport.cpp: USDL2Viewport code.
	Copyright 2002 Epic Games, Inc. All Rights Reserved.

        SDL website: http://www.libsdl.org/

Revision history:
	* Created by Ryan C. Gordon, based on WinDrv.
      This is an updated rewrite of the original SDLDrv.
	* Updated to SDL2 with some improvements by Smirftsch www.oldunreal.com
=============================================================================*/

#include <stdlib.h>

#ifdef __LINUX__
#include <unistd.h>
#elif _MSC_VER
#include <windows.h>
#endif

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#include "SDL2Drv.h"

/*-----------------------------------------------------------------------------
	Class implementation.
-----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(USDL2Viewport);

/*-----------------------------------------------------------------------------
	USDL2Viewport Init/Exit.
-----------------------------------------------------------------------------*/

static UBOOL FullscreenOnly = 0;   // !!! FIXME: UE2 compat hack.
static SDL_Cursor* StandardCursors[7] = {nullptr};
static const SDL_Cursor* ActiveCursor = nullptr;

//
// Constructor.
//
USDL2Viewport::USDL2Viewport()
    :	UViewport()
{
    guard(USDL2Viewport::USDL2Viewport);
    debugf(NAME_Init,TEXT("SDL2: Setting up viewport"));

    FullscreenOnly = 0;
    LostFullscreen = LostGrab = 0;
    LastUpdateTime = appSeconds();
    ColorBytes = INDEX_NONE;
	MouseEnterX = MouseEnterY = -1;
	MouseLeaveX = MouseLeaveY = -1;

    // Zero out maps.
    for (INT i=0; i<SDL_NUM_SCANCODES; i++)
        KeysymMap[i] = IK_None;

    // Remap important keys.

    // TTY Functions.
    KeysymMap[SDL_SCANCODE_BACKSPACE]   = IK_Backspace;
    KeysymMap[SDL_SCANCODE_TAB]		    = IK_Tab;
    KeysymMap[SDL_SCANCODE_RETURN]	    = IK_Enter;
    KeysymMap[SDL_SCANCODE_PAUSE]	    = IK_Pause;
    KeysymMap[SDL_SCANCODE_ESCAPE]	    = IK_Escape;
#if MACOSX || MACOSXPPC
    KeysymMap[SDL_SCANCODE_DELETE]      = IK_Backspace; // OSX seems to have some weird mapping here- Smirftsch
#else
    KeysymMap[SDL_SCANCODE_DELETE]	= SDL_SCANCODE_BACKSPACE;
#endif
    KeysymMap[SDL_SCANCODE_INSERT]      = IK_Insert;

    // Modifiers.
    KeysymMap[SDL_SCANCODE_LSHIFT]	= IK_LShift;
    KeysymMap[SDL_SCANCODE_RSHIFT]	= IK_RShift;
    KeysymMap[SDL_SCANCODE_LCTRL]	= IK_LControl;
    KeysymMap[SDL_SCANCODE_RCTRL]	= IK_RControl;
    KeysymMap[SDL_SCANCODE_LGUI]	= IK_F24;
    KeysymMap[SDL_SCANCODE_RGUI]	= IK_F24;
    KeysymMap[SDL_SCANCODE_LALT]	= IK_Alt;
    KeysymMap[SDL_SCANCODE_RALT]	= IK_Alt;

    // Special remaps.
    KeysymMap[SDL_SCANCODE_GRAVE]           = IK_Tilde;
    KeysymMap[241]                          = IK_Tilde; // Spanish Ñ key
    KeysymMap[SDL_SCANCODE_APOSTROPHE]      = IK_SingleQuote;
    KeysymMap[SDL_SCANCODE_SEMICOLON]       = IK_Semicolon;
    KeysymMap[SDL_SCANCODE_COMMA]           = IK_Comma;
    KeysymMap[SDL_SCANCODE_PERIOD]          = IK_Period;
    KeysymMap[SDL_SCANCODE_SLASH]           = IK_Slash;
    KeysymMap[SDL_SCANCODE_BACKSLASH]       = IK_Backslash;
    KeysymMap[SDL_SCANCODE_LEFTBRACKET]     = IK_LeftBracket;
    KeysymMap[SDL_SCANCODE_RIGHTBRACKET]    = IK_RightBracket;

    // Misc function keys.
    KeysymMap[SDL_SCANCODE_F1]	= IK_F1;
    KeysymMap[SDL_SCANCODE_F2]	= IK_F2;
    KeysymMap[SDL_SCANCODE_F3]	= IK_F3;
    KeysymMap[SDL_SCANCODE_F4]	= IK_F4;
    KeysymMap[SDL_SCANCODE_F5]	= IK_F5;
    KeysymMap[SDL_SCANCODE_F6]	= IK_F6;
    KeysymMap[SDL_SCANCODE_F7]	= IK_F7;
    KeysymMap[SDL_SCANCODE_F8]	= IK_F8;
    KeysymMap[SDL_SCANCODE_F9]	= IK_F9;
    KeysymMap[SDL_SCANCODE_F10]	= IK_F10;
    KeysymMap[SDL_SCANCODE_F11]	= IK_F11;
    KeysymMap[SDL_SCANCODE_F12]	= IK_F12;
    KeysymMap[SDL_SCANCODE_F13]	= IK_F13;
    KeysymMap[SDL_SCANCODE_F14]	= IK_F14;
    KeysymMap[SDL_SCANCODE_F15]	= IK_F15;

    // Cursor control and motion.
    KeysymMap[SDL_SCANCODE_HOME]	= IK_Home;
    KeysymMap[SDL_SCANCODE_LEFT]	= IK_Left;
    KeysymMap[SDL_SCANCODE_UP]		= IK_Up;
    KeysymMap[SDL_SCANCODE_RIGHT]	= IK_Right;
    KeysymMap[SDL_SCANCODE_DOWN]	= IK_Down;
    KeysymMap[SDL_SCANCODE_PAGEUP]	= IK_PageUp;
    KeysymMap[SDL_SCANCODE_PAGEDOWN]= IK_PageDown;
    KeysymMap[SDL_SCANCODE_END]	    = IK_End;

    // Keypad functions and numbers.
    KeysymMap[SDL_SCANCODE_KP_ENTER]	= IK_Enter;
    KeysymMap[SDL_SCANCODE_KP_0]		= IK_NumPad0;
    KeysymMap[SDL_SCANCODE_KP_1]		= IK_NumPad1;
    KeysymMap[SDL_SCANCODE_KP_2]		= IK_NumPad2;
    KeysymMap[SDL_SCANCODE_KP_3]		= IK_NumPad3;
    KeysymMap[SDL_SCANCODE_KP_4]		= IK_NumPad4;
    KeysymMap[SDL_SCANCODE_KP_5]		= IK_NumPad5;
    KeysymMap[SDL_SCANCODE_KP_6]		= IK_NumPad6;
    KeysymMap[SDL_SCANCODE_KP_7]		= IK_NumPad7;
    KeysymMap[SDL_SCANCODE_KP_8]		= IK_NumPad8;
    KeysymMap[SDL_SCANCODE_KP_9]		= IK_NumPad9;
    KeysymMap[SDL_SCANCODE_KP_MULTIPLY]	= IK_GreyStar;
    KeysymMap[SDL_SCANCODE_KP_PLUS]		= IK_GreyPlus;
    KeysymMap[SDL_SCANCODE_KP_EQUALS]	= IK_Separator;
    KeysymMap[SDL_SCANCODE_KP_MINUS]	= IK_GreyMinus;
    KeysymMap[SDL_SCANCODE_KP_PERIOD]	= IK_NumPadPeriod;
    KeysymMap[SDL_SCANCODE_KP_DIVIDE]	= IK_GreySlash;

    // Other
    KeysymMap[SDL_SCANCODE_MINUS]			= IK_Minus;
    KeysymMap[SDL_SCANCODE_EQUALS]			= IK_Equals;
    KeysymMap[SDL_SCANCODE_NUMLOCKCLEAR]	= IK_NumLock;
    KeysymMap[SDL_SCANCODE_CAPSLOCK]		= IK_CapsLock;
    KeysymMap[SDL_SCANCODE_SCROLLLOCK]		= IK_ScrollLock;

    // Main
    KeysymMap[SDL_SCANCODE_1]				= IK_1;
    KeysymMap[SDL_SCANCODE_2]				= IK_2;
    KeysymMap[SDL_SCANCODE_3]				= IK_3;
    KeysymMap[SDL_SCANCODE_4]				= IK_4;
    KeysymMap[SDL_SCANCODE_5]				= IK_5;
    KeysymMap[SDL_SCANCODE_6]				= IK_6;
    KeysymMap[SDL_SCANCODE_7]				= IK_7;
    KeysymMap[SDL_SCANCODE_8]				= IK_8;
    KeysymMap[SDL_SCANCODE_9]				= IK_9;
    KeysymMap[SDL_SCANCODE_0]				= IK_0;

    KeysymMap[SDL_SCANCODE_A]				= IK_A;
    KeysymMap[SDL_SCANCODE_B]				= IK_B;
    KeysymMap[SDL_SCANCODE_C]				= IK_C;
    KeysymMap[SDL_SCANCODE_D]				= IK_D;
    KeysymMap[SDL_SCANCODE_E]				= IK_E;
    KeysymMap[SDL_SCANCODE_F]				= IK_F;
    KeysymMap[SDL_SCANCODE_G]				= IK_G;
    KeysymMap[SDL_SCANCODE_H]				= IK_H;
    KeysymMap[SDL_SCANCODE_I]				= IK_I;
    KeysymMap[SDL_SCANCODE_J]				= IK_J;
    KeysymMap[SDL_SCANCODE_K]				= IK_K;
    KeysymMap[SDL_SCANCODE_L]				= IK_L;
    KeysymMap[SDL_SCANCODE_M]				= IK_M;
    KeysymMap[SDL_SCANCODE_N]				= IK_N;
    KeysymMap[SDL_SCANCODE_O]				= IK_O;
    KeysymMap[SDL_SCANCODE_P]				= IK_P;
    KeysymMap[SDL_SCANCODE_Q]				= IK_Q;
    KeysymMap[SDL_SCANCODE_R]				= IK_R;
    KeysymMap[SDL_SCANCODE_S]				= IK_S;
    KeysymMap[SDL_SCANCODE_T]				= IK_T;
    KeysymMap[SDL_SCANCODE_U]				= IK_U;
    KeysymMap[SDL_SCANCODE_V]				= IK_V;
    KeysymMap[SDL_SCANCODE_W]				= IK_W;
    KeysymMap[SDL_SCANCODE_X]				= IK_X;
    KeysymMap[SDL_SCANCODE_Y]				= IK_Y;
    KeysymMap[SDL_SCANCODE_Z]				= IK_Z;
    KeysymMap[SDL_SCANCODE_SPACE]			= IK_Space;
    KeysymMap[SDL_SCANCODE_COMMA]			= IK_Comma;


#if MACOSX || MACOSXPPC // !!! FIXME: Arguably, a bug in SDL... - Yet? Check later, Smirftsch
    KeysymMap[SDL_SCANCODE_PRINTSCREEN]	    = IK_F13;
    KeysymMap[SDL_SCANCODE_INSERT]	        = IK_Help;
    // "Insert" (insert on PC, help on some Mac keyboards (but does send code 73, not 117))
#endif

    KeyRepeatKey		= IK_None;
    LastJoyHat			= IK_None;
    ConsoleKey          = IK_None;
    UWindowKey          = IK_None;
    Window              = NULL;
    bTyping             = 0;

	StandardCursors[0] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	StandardCursors[1] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
	StandardCursors[2] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
	StandardCursors[3] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
	StandardCursors[4] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
	StandardCursors[5] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
	StandardCursors[6] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);

	if (StandardCursors[0])
	{
		ActiveCursor = StandardCursors[0];
		SDL_SetCursor(StandardCursors[0]);
		bWindowsMouseAvailable=1;
	}

    debugf(NAME_Init, TEXT("SDL2: Created and initialized a new viewport.") );
    unguard;
}

//
// Destroy.
//
void USDL2Viewport::Destroy()
{
    guard(USDL2Viewport::Destroy);
    Super::Destroy();

    if( BlitFlags & BLIT_Temporary )
        appFree( ScreenPointer );
    if (ScreenSurface)
        SDL_FreeSurface(ScreenSurface);
    if (Renderer)
        SDL_DestroyRenderer(Renderer);
    if (Window)
        SDL_DestroyWindow(Window);

	for (int i = 0; i < ARRAY_COUNT(StandardCursors); ++i)
	{
		if (StandardCursors[i])
		{
			SDL_FreeCursor(StandardCursors[i]);
			StandardCursors[i]=nullptr;
		}
	}

    SDL_Quit();

    unguard;
}

//
// Error shutdown.
//
void USDL2Viewport::ShutdownAfterError()
{
    if (ScreenSurface)
        SDL_FreeSurface(ScreenSurface);
    if (Renderer)
        SDL_DestroyRenderer(Renderer);
    if (Window)
        SDL_DestroyWindow(Window);
    SDL_Quit();
    Super::ShutdownAfterError();
}

void USDL2Viewport::GetMouseState(INT* MouseX, INT* MouseY)
{
	if (MouseIsGrabbed)
		SDL_GetRelativeMouseState(MouseX, MouseY);
	else
		SDL_GetMouseState(MouseX, MouseY);
}

void USDL2Viewport::UpdateMouseGrabState(const UBOOL ShouldGrab)
{
	bool OldMouseIsGrabbed = MouseIsGrabbed;
	MouseIsGrabbed = ShouldGrab;

	if (OldMouseIsGrabbed != MouseIsGrabbed)
	{
		// going back to not grabbed
		if (!MouseIsGrabbed)
		{
			GetMouseState(&MouseEnterX, &MouseEnterY);
			if (ActiveCursor)
				SDL_ShowCursor(SDL_ENABLE);
			WindowsMouseX = Clamp<FLOAT>(MouseEnterX * MouseScaleX, 0.f, SizeX);
			WindowsMouseY = Clamp<FLOAT>(MouseEnterY * MouseScaleY, 0.f, SizeY);
		}
		// going to grabbed
		else
		{
			GetMouseState(&MouseLeaveX, &MouseLeaveY);
			if (ActiveCursor)
				SDL_ShowCursor(SDL_DISABLE);
		}
	}

#ifndef __EMSCRIPTEN__  // we grab this elsewhere for Emscripten.
	SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, 0);
	SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_SCALING, 0);
	SDL_SetRelativeMouseMode(MouseIsGrabbed ? SDL_TRUE : SDL_FALSE);
	SDL_SetWindowGrab(Window, MouseIsGrabbed ? SDL_TRUE : SDL_FALSE);
#endif
}

/*-----------------------------------------------------------------------------
	Command line.
-----------------------------------------------------------------------------*/

//
// Command line.
//
UBOOL USDL2Viewport::Exec( const TCHAR* Cmd, FOutputDevice& Ar )
{
    guard(USDL2Viewport::Exec);
    if( UViewport::Exec( Cmd, Ar ) )
    {
        return 1;
    }
    else if( ParseCommand(&Cmd,TEXT("ENDFULLSCREEN")) )
    {
        if( BlitFlags & BLIT_Fullscreen )
            EndFullscreen();
        return 1;
    }
    else if( ParseCommand(&Cmd,TEXT("TOGGLEFULLSCREEN")) )
    {
        ToggleFullscreen();
        return 1;
    }
    else if( ParseCommand(&Cmd,TEXT("GETCURRENTRES")) )
    {
        INT WindowX=0,WindowY=0;
        if( BlitFlags & BLIT_Fullscreen )
        {
            SDL_DisplayMode CurrentMode;
            SDL_GetWindowDisplayMode(Window,&CurrentMode);
            Ar.Logf( TEXT("%ix%i"),CurrentMode.w,CurrentMode.h );
        }
        else
        {
            SDL_GetWindowSize(Window, &WindowX,&WindowY);
            Ar.Logf( TEXT("%ix%i"),WindowX,WindowY );
        }
        return 1;
    }
    else if( ParseCommand(&Cmd,TEXT("GETCURRENTCOLORDEPTH")) )
    {
        Ar.Logf( TEXT("%i"), (ColorBytes?ColorBytes:2)*8 );
        return 1;
    }
    else if( ParseCommand(&Cmd,TEXT("GETCOLORDEPTHS")) )
    {
        Ar.Log( TEXT("16 32") );
        return 1;
    }
    else if( ParseCommand(&Cmd,TEXT("GETCURRENTRENDERDEVICE")) )
    {
        Ar.Log( RenDev->GetClass()->GetPathName() );
        return 1;
    }
    else if( ParseCommand(&Cmd,TEXT("SUPPORTEDRESOLUTION")) )
    {
        FString Str = TEXT("");
        // Available fullscreen video modes
        int display_count = 0, display_index = 0, i=0;
        SDL_DisplayMode mode = { SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0 };

        if ((display_count = SDL_GetNumDisplayModes(0)) < 1)
        {
            debugf(NAME_Init, TEXT("SDL2: No available fullscreen video modes"));
        }
        //debugf(TEXT("SDL_GetNumVideoDisplays returned: %i"), display_count);

        for(i = 0; i < display_count; i++)
        {
            mode.format = 0;
            mode.w = 0;
            mode.h = 0;
            mode.refresh_rate = 0;
            mode.driverdata = 0;
            if (SDL_GetDisplayMode(display_index, i, &mode) != 0)
                debugf(TEXT("SDL2: SDL_GetDisplayMode failed: %ls"), SDL_GetError());
            //debugf(TEXT("SDL_GetDisplayMode(0, 0, &mode):\t\t%i bpp\t%i x %i"), SDL_BITSPERPIXEL(mode.format), mode.w, mode.h);
            Str += FString::Printf(TEXT("%ix%i "), mode.w, mode.h);
        }

        // Send the resolution string to the engine.
        Ar.Logf(*Str.LeftChop(1));

        return 1;
    }

    else if( ParseCommand(&Cmd,TEXT("TEMPSETRES")) )
    {
        INT X=appAtoi(Cmd);
        const TCHAR* CmdTemp = appStrchr(Cmd,'x') ? appStrchr(Cmd,'x')+1 : appStrchr(Cmd,'X') ? appStrchr(Cmd,'X')+1 : TEXT("");
        INT Y=appAtoi(CmdTemp);
        Cmd = CmdTemp;
        CmdTemp = appStrchr(Cmd,'x') ? appStrchr(Cmd,'x')+1 : appStrchr(Cmd,'X') ? appStrchr(Cmd,'X')+1 : TEXT("");
        INT BPP=appAtoi(CmdTemp);
        Cmd = CmdTemp;
        UBOOL	Fullscreen = IsFullscreen() || FullscreenOnly;
        if(appStrchr(Cmd,'w') || appStrchr(Cmd,'W'))
            Fullscreen = 0;
        else if(appStrchr(Cmd,'f') || appStrchr(Cmd,'F'))
            Fullscreen = 1;
        if( X && Y )
        {
            INT ColorBytes = 0;
            switch( BPP )
            {
            case 16:
                ColorBytes = 2;
                break;
            case 24:
            case 32:
                ColorBytes = 4;
                break;
            }
            UBOOL Result = RenDev->SetRes( X, Y, ColorBytes, Fullscreen || FullscreenOnly);
            if( !Result )
                EndFullscreen();
        }
        return 1;
    }

    else if( ParseCommand(&Cmd,TEXT("SETRES")) )
    {
        INT X=appAtoi(Cmd);
        const TCHAR* CmdTemp = appStrchr(Cmd,'x') ? appStrchr(Cmd,'x')+1 : appStrchr(Cmd,'X') ? appStrchr(Cmd,'X')+1 : TEXT("");
        INT Y=appAtoi(CmdTemp);
        Cmd = CmdTemp;
        CmdTemp = appStrchr(Cmd,'x') ? appStrchr(Cmd,'x')+1 : appStrchr(Cmd,'X') ? appStrchr(Cmd,'X')+1 : TEXT("");
        INT BPP=appAtoi(CmdTemp);
        Cmd = CmdTemp;
        UBOOL	Fullscreen = IsFullscreen() || FullscreenOnly;

        if(appStrchr(Cmd,'w') || appStrchr(Cmd,'W'))
            Fullscreen = 0;
        else if(appStrchr(Cmd,'f') || appStrchr(Cmd,'F'))
            Fullscreen = 1;

        USDL2Client*	Client = GetOuterUSDL2Client();

        if (Fullscreen && Client->UseDesktopFullScreen) // can't change fullscreen resolution when UseDesktopFullScreen
        {
            debugf(TEXT("SDL2Drv: can't change fullscreen resolution when UseDesktopFullScreen"));
            return 0;
        }

        INT				SavedX,
                        SavedY;
        if(Fullscreen)
        {
            SavedX = Client->FullscreenViewportX;
            SavedY = Client->FullscreenViewportY;
        }
        else
        {
            SavedX = Client->WindowedViewportX;
            SavedY = Client->WindowedViewportY;
        }

        if( X && Y )
        {
            INT ColorBytes = 0;
            switch( BPP )
            {
            case 16:
                ColorBytes = 2;
                break;
            case 24:
            case 32:
                ColorBytes = 4;
                break;
            }
            UBOOL Result = RenDev->SetRes( X, Y, ColorBytes, Fullscreen /*|| FullscreenOnly*/ );
            if( !Result )
                EndFullscreen();

            SavedX = X;
            SavedY = Y;
        }

        if(Fullscreen)
        {
            Client->FullscreenViewportX = SavedX;
            Client->FullscreenViewportY = SavedY;
        }
        else
        {
            Client->WindowedViewportX = SavedX;
            Client->WindowedViewportY = SavedY;
        }
        Client->SaveConfig();

        return 1;
    }
#if 0  // !!! uh...?
    else if( ParseCommand(&Cmd,TEXT("PREFERENCES")) )
    {
        USDL2Client* Client = GetOuterUSDL2Client();
        Client->ConfigReturnFullscreen = 0;
        if( BlitFlags & BLIT_Fullscreen )
        {
            EndFullscreen();
            Client->ConfigReturnFullscreen = 1;
        }
        if( !Client->ConfigProperties )
        {
            Client->ConfigProperties = new WConfigProperties( TEXT("Preferences"), LocalizeGeneral("AdvancedOptionsTitle",TEXT("Window")) );
            Client->ConfigProperties->SetNotifyHook( Client );
            Client->ConfigProperties->OpenWindow( Window->hWnd );
            Client->ConfigProperties->ForceRefresh();
        }
        GetOuterUSDL2Client()->ConfigProperties->Show(1);
        SetFocus( *GetOuterUSDL2Client()->ConfigProperties );
        return 1;
    }
#endif

    // gam ---
    else if( ParseCommand(&Cmd,TEXT("SETMOUSE")) )
    {
        USDL2Client*	Client = GetOuterUSDL2Client();

        if (!Client)
            return 0;

        const TCHAR* CmdTemp = Cmd;
        INT X, Y;

        X = appAtoi(CmdTemp);

        while( appIsDigit( *CmdTemp) )
            CmdTemp++;

        while( isspace( *CmdTemp) )
            CmdTemp++;

        Y = appAtoi(CmdTemp);
        /*
        //SDL_WarpMouseGlobal available in 2.04+, not stable yet. Todo: SDLWindow for SDL_WarpMouseInWindow, but needed?
        if (!Client || (Client && Client->Viewports(0)->IsFullscreen()))
        	SDL_WarpMouseGlobal(X, Y);
        else
        */
        SDL_WarpMouseInWindow (Window,X,Y);

        return 1;
    }

    // vi represent!  --ryan.
    else if ((ParseCommand(&Cmd,TEXT(":wq"))) || (ParseCommand(&Cmd,TEXT(":q!"))))
    {
        GIsRequestingExit = 1;
        return 1;
    }
    else if( ParseCommand(&Cmd,TEXT("GetScreenMode")) )
    {
        if(IsFullscreen())
            Ar.Log( TEXT("Fullscreen"));
        else Ar.Log( TEXT("Windowed"));
        return 1;
    }
    // --- gam
    return 0;
    unguard;
}

/*-----------------------------------------------------------------------------
	Window openining and closing.
-----------------------------------------------------------------------------*/

//
// Open this viewport's window.
//
void USDL2Viewport::OpenWindow( void* InParentWindow, UBOOL IsTemporary, INT NewX, INT NewY, INT OpenX, INT OpenY, const TCHAR* ForcedRenDevClass )
{
    guard(USDL2Viewport::OpenWindow);

    check(Actor);
    USDL2Client* C = GetOuterUSDL2Client();

    debugf( TEXT("SDL2: Opening viewport.") );

    // Create or update the window.
    SizeX = C->FullscreenViewportX;
    SizeY = C->FullscreenViewportY;

    // Create rendering device.
	if( !RenDev && ForcedRenDevClass )
		TryRenderDevice( ForcedRenDevClass, NewX, NewY, ColorBytes, C->StartupFullscreen );
    if( !RenDev && !GIsEditor && !ParseParam(appCmdLine(),TEXT("nohard")) )
        TryRenderDevice( TEXT("ini:Engine.Engine.GameRenderDevice"), NewX, NewY, ColorBytes, C->StartupFullscreen );
    check(RenDev);
    UpdateWindowFrame();
    Repaint( 1 );

    unguard;
}

//
// Close a viewport window.  Assumes that the viewport has been opened with
// OpenViewportWindow.  Does not affect the viewport's object, only the
// platform-specific information associated with it.
//
void USDL2Viewport::CloseWindow()
{
    guard(USDL2Viewport::CloseWindow);
    if (!IsFullscreen())
    {
        USDL2Client* Client = GetOuterUSDL2Client();
        SDL_GetWindowPosition(Window,&Client->WindowPosX,&Client->WindowPosY); //Store window position.
        //debugf(TEXT("Client->WindowPos %i %i"),Client->WindowPosX,Client->WindowPosY);
        GConfig->SetInt(TEXT("SDL2Drv.SDL2Client"), TEXT("WindowPosX"), Client->WindowPosX, GIni );
        GConfig->SetInt(TEXT("SDL2Drv.SDL2Client"), TEXT("WindowPosY"), Client->WindowPosY, GIni );
    }
    unguard;
}

/*-----------------------------------------------------------------------------
	USDL2Viewport operations.
-----------------------------------------------------------------------------*/

//
// Set window position according to menu's on-top setting:
//
void USDL2Viewport::SetTopness()
{
    guard(USDL2Viewport::SetTopness);
    /* no-op. */
    unguard;
}

//
// Repaint the viewport.
//
void USDL2Viewport::Repaint( UBOOL Blit )
{
    guard(USDL2Viewport::Repaint);
    GetOuterUSDL2Client()->Engine->Draw( this, Blit );
	RepaintPending = FALSE;
    unguard;
}

//
// Return whether fullscreen.
//
UBOOL USDL2Viewport::IsFullscreen()
{
    guard(USDL2Viewport::IsFullscreen);
    return (BlitFlags & BLIT_Fullscreen)!=0;
    unguard;
}

//
// Set the mouse cursor according to Unreal or UnrealEd's mode, or to
// an hourglass if a slow task is active.
//
void USDL2Viewport::SetModeCursor()
{
    guard(USDL2Viewport::SetModeCursor);

// !!! FIXME: What to do with this?
#if 0 // original WinDrv code follows:
    if( GIsSlowTask )
    {
        SetCursor(LoadCursorIdX(NULL,IDC_WAIT));
        return;
    }
    HCURSOR hCursor = NULL;
    switch( GetOuterUSDL2Client()->Engine->edcamMode(this) )
    {
    case EM_ViewportZoom:
        hCursor = LoadCursorIdX(hInstance,IDCURSOR_CameraZoom);
        break;
    case EM_ActorRotate:
        hCursor = LoadCursorIdX(hInstance,IDCURSOR_BrushRot);
        break;
    case EM_ActorScale:
        hCursor = LoadCursorIdX(hInstance,IDCURSOR_BrushScale);
        break;
    case EM_ActorSnapScale:
        hCursor = LoadCursorIdX(hInstance,IDCURSOR_BrushSnap);
        break;
    case EM_TexturePan:
        hCursor = LoadCursorIdX(hInstance,IDCURSOR_TexPan);
        break;
    case EM_TextureRotate:
        hCursor = LoadCursorIdX(hInstance,IDCURSOR_TexRot);
        break;
    case EM_TextureScale:
        hCursor = LoadCursorIdX(hInstance,IDCURSOR_TexScale);
        break;
    case EM_None:
        hCursor = LoadCursorIdX(NULL,IDC_CROSS);
        break;
    case EM_ViewportMove:
        hCursor = LoadCursorIdX(NULL,IDC_CROSS);
        break;
    case EM_TexView:
        hCursor = LoadCursorIdX(NULL,IDC_ARROW);
        break;
    case EM_TexBrowser:
        hCursor = LoadCursorIdX(NULL,IDC_ARROW);
        break;
    case EM_StaticMeshBrowser:
        hCursor = LoadCursorIdX(NULL,IDC_CROSS);
        break;
    case EM_MeshView:
        hCursor = LoadCursorIdX(NULL,IDC_CROSS);
        break;
    case EM_Animation:
        hCursor = LoadCursorIdX(NULL,IDC_CROSS);
        break;
    case EM_VertexEdit:
        hCursor = LoadCursorIdX(hInstance,IDCURSOR_VertexEdit);
        break;
    case EM_BrushClip:
        hCursor = LoadCursorIdX(hInstance,IDCURSOR_BrushClip);
        break;
    case EM_FaceDrag:
        hCursor = LoadCursorIdX(hInstance,IDCURSOR_FaceDrag);
        break;
    case EM_Polygon:
        hCursor = LoadCursorIdX(hInstance,IDCURSOR_BrushWarp);
        break;
    case EM_TerrainEdit:
    {
        switch( GetOuterUSDL2Client()->Engine->edcamTerrainBrush() )
        {
        case TB_VertexEdit:
            hCursor = LoadCursorIdX(hInstance,IDCURSOR_TB_VertexEdit);
            break;
        case TB_Paint:
            hCursor = LoadCursorIdX(hInstance,IDCURSOR_TB_Paint);
            break;
        case TB_Smooth:
            hCursor = LoadCursorIdX(hInstance,IDCURSOR_TB_Smooth);
            break;
        case TB_Noise:
            hCursor = LoadCursorIdX(hInstance,IDCURSOR_TB_Noise);
            break;
        case TB_Flatten:
            hCursor = LoadCursorIdX(hInstance,IDCURSOR_TB_Flatten);
            break;
        case TB_TexturePan:
            hCursor = LoadCursorIdX(hInstance,IDCURSOR_TexPan);
            break;
        case TB_TextureRotate:
            hCursor = LoadCursorIdX(hInstance,IDCURSOR_TexRot);
            break;
        case TB_TextureScale:
            hCursor = LoadCursorIdX(hInstance,IDCURSOR_TexScale);
            break;
        case TB_Select:
            hCursor = LoadCursorIdX(hInstance,IDCURSOR_TB_Selection);
            break;
        case TB_Visibility:
            hCursor = LoadCursorIdX(hInstance,IDCURSOR_TB_Visibility);
            break;
        case TB_Color:
            hCursor = LoadCursorIdX(hInstance,IDCURSOR_TB_Color);
            break;
        case TB_EdgeTurn:
            hCursor = LoadCursorIdX(hInstance,IDCURSOR_TB_EdgeTurn);
            break;
        default:
            hCursor = LoadCursorIdX(hInstance,IDCURSOR_TerrainEdit);
            break;
        }
    }
    break;
    case EM_PrefabBrowser:
        hCursor = LoadCursorIdX(NULL,IDC_CROSS);
        break;
    case EM_Matinee:
        hCursor = LoadCursorIdX(hInstance,IDCURSOR_Matinee);
        break;
    case EM_EyeDropper:
        hCursor = LoadCursorIdX(hInstance,IDCURSOR_EyeDropper);
        break;
    case EM_FindActor:
        hCursor = LoadCursorIdX(hInstance,IDCURSOR_FindActor);
        break;
    case EM_Geometry:
        hCursor = LoadCursorIdX(hInstance,IDCURSOR_Geometry);
        break;

    case EM_NewCameraMove:
        hCursor = LoadCursorIdX(NULL,IDC_CROSS);
        break;
    default:
        hCursor = LoadCursorIdX(NULL,IDC_ARROW);
        break;
    }
    check(hCursor);
    SetCursor( hCursor );
#endif

    unguard;
}

void USDL2Viewport::SetTitleBar()
{
    guard(USDL2Viewport::SetTitleBar);
    //debugf(TEXT("SDL2: SetTitleBar"));

    FString WindowName;
	// Set viewport window's name to show resolution.
	if( !GIsEditor || (Actor->ShowFlags&SHOW_PlayerCtrl) )
	{
		WindowName = LocalizeGeneral("Product",appPackage());
        WindowName += FString::Printf(TEXT(" - Version: %i Subversion: %i (Build date: %ls %ls)"), ENGINE_VERSION, ENGINE_SUBVERSION, appFromAnsi(__DATE__), appFromAnsi(__TIME__) );
	}
	else switch( Actor->RendMap )
	{
		case REN_Wire:		WindowName=LocalizeGeneral(TEXT("ViewPersp"),TEXT("SDL2Drv")); break;
		case REN_OrthXY:	WindowName=LocalizeGeneral(TEXT("ViewXY")   ,TEXT("SDL2Drv")); break;
		case REN_OrthXZ:	WindowName=LocalizeGeneral(TEXT("ViewXZ")   ,TEXT("SDL2Drv")); break;
		case REN_OrthYZ:	WindowName=LocalizeGeneral(TEXT("ViewYZ")   ,TEXT("SDL2Drv")); break;
		default:			WindowName=LocalizeGeneral(TEXT("ViewOther"),TEXT("SDL2Drv")); break;
	}

	SDL_SetWindowTitle(Window, appToAnsi(*WindowName));

#if !MACOSX
    static bool loaded_icon = false;
    if (!loaded_icon)
    {
        loaded_icon = true;
        SDL_Surface *icon = SDL_LoadBMP("../Help/UnrealIcon.bmp");
        if (!icon)
            debugf( TEXT("SDL2: Couldn't load icon file %ls"), appFromAnsi(SDL_GetError()) );

        if (icon != NULL)
            SDL_SetWindowIcon(Window, icon);
    }
#endif
    unguard;
}


//
// Update user viewport interface.
//
void USDL2Viewport::UpdateWindowFrame()
{
    guard(USDL2Viewport::UpdateWindowFrame);

    //debugf(TEXT("SDL2: UpdateWindowFrame"));

    // If not a window, exit.
    if( !Window || (BlitFlags&BLIT_Fullscreen) || (BlitFlags&BLIT_Temporary) || !Actor)
        return;

    SetTitleBar();

    /* UED stuff. Maybe some day?
    // Set viewport window's name to show resolution.
    TCHAR WindowName[80];
    if( !GIsEditor || (Actor->ShowFlags&SHOW_PlayerCtrl) )
    {
    	appSprintf( WindowName, LocalizeGeneral("Product",appPackage()) );
    }
    else switch( Actor->RendMap )
    {
    	case REN_Wire:		appStrcpy(WindowName,LocalizeGeneral("ViewPersp")); break;
    	case REN_OrthXY:	appStrcpy(WindowName,LocalizeGeneral("ViewXY"   )); break;
    	case REN_OrthXZ:	appStrcpy(WindowName,LocalizeGeneral("ViewXZ"   )); break;
    	case REN_OrthYZ:	appStrcpy(WindowName,LocalizeGeneral("ViewYZ"   )); break;
    	default:			appStrcpy(WindowName,LocalizeGeneral("ViewOther")); break;
    }
    Window->SetText( WindowName );

    // Update parent window.
    if( ParentWindow )
    {
    	SendMessageX( ParentWindow, WM_CHAR, 0, 0 );
    	PostMessageX( ParentWindow, WM_COMMAND, WM_VIEWPORT_UPDATEWINDOWFRAME, 0 );
    }
    */

    unguard;
}

//
// Return the viewport's window.
//
void* USDL2Viewport::GetWindow()
{
    guard(USDL2Viewport::GetWindow);
    return Window;
    unguard;
}

/*-----------------------------------------------------------------------------
	Input.
-----------------------------------------------------------------------------*/

//
// Input event router.
//
UBOOL USDL2Viewport::CauseInputEvent( INT iKey, EInputAction Action, FLOAT Delta )
{
    guard(USDL2Viewport::CauseInputEvent);

    if( iKey>=0 && iKey<IK_MAX )
        return GetOuterUSDL2Client()->Engine->InputEvent( this, (EInputKey)iKey, Action, Delta );
    else
        return 0;
    unguard;
}

//
// If the cursor is currently being captured, stop capturing, clipping, and
// hiding it, and move its position back to where it was when it was initially
// captured.
//
void USDL2Viewport::SetMouseCapture( UBOOL Capture, UBOOL Clip, UBOOL OnlyFocus )
{
    guard(USDL2Viewport::SetMouseCapture);

    bWindowsMouseAvailable = !Capture;

    UpdateMouseGrabState(Capture);
    unguard;
}

int ucs2_char_code_from_utf8(char const *utf8_char_seq)
{
    unsigned char c0, c1, c2;
    c0 = *utf8_char_seq;

    if (c0 < 128)
        return c0;
    if (c0 < 224)
    {
        c1 = utf8_char_seq[1];
        if (128 <= c1 && c1 < 192)
            return (c0 & 31) << 6 | (c1 & 63);
        return -1;
    }
    if (c0 < 240)
    {
        c1 = utf8_char_seq[1];
        if (128 <= c1 && c1 < 192)
        {
            c2 = utf8_char_seq[2];
            if (128 <= c2 && c2 < 192)
                return (c0 & 15) << 12 | (c1 & 63) << 6 | (c2 & 63);
        }
        return -1;
    }
    return -1;
}

//
// Update input for this viewport.
//
void USDL2Viewport::UpdateInput( UBOOL Reset )
{
    guard(USDL2Viewport::UpdateInput);

    FTime UpdateTime = appSeconds();
    FLOAT DeltaSeconds = UpdateTime - LastUpdateTime;
    LastUpdateTime = UpdateTime;

    USDL2Client* Client = GetOuterUSDL2Client();
    Client->InMenuLoop = ((Actor?Actor->bShowMenu:0) || (Client->Viewports(0)?Client->Viewports(0)->bShowWindowsMouse:0)); //...
	if( Client->Viewports.Num() && Client->Viewports(0)->Console )
    {
        ConsoleKey = (EInputKey)Client->Viewports(0)->Console->GetConsoleKey();
        UWindowKey = (EInputKey)Client->Viewports(0)->Console->GetWindowKey();
        bTyping = Client->Engine->TypingEvent(this);
    }


    if ((Client->UseJoystick) && (USDL2Client::Joystick != NULL))
    {
        SDL_JoystickUpdate();

        int i;
        int max = USDL2Client::JoystickButtons;
        for (i = 0; i < max; i++)
        {
            JoyState = SDL_JoystickGetButton(USDL2Client::Joystick, i);
            INT Button = IK_Joy1 + i;
            if( !Input->KeyDown( Button ) && (JoyState == SDL_PRESSED) )
                CauseInputEvent( Button, IST_Press );
            else if( Input->KeyDown( Button ) && (JoyState != SDL_PRESSED) )
                CauseInputEvent( Button, IST_Release );
        }

        static EInputKey axes[] = { IK_JoyX, IK_JoyY, IK_JoyZ, IK_JoyR, IK_JoyU, IK_JoyV }; //, IK_JoySlider1, IK_JoySlider2 };
        check(USDL2Client::JoystickAxes <= 8);
        for (i = 0; i < USDL2Client::JoystickAxes; i++)
        {
            float pos = SDL_JoystickGetAxis(USDL2Client::Joystick, i) / 65535.f;
            Input->DirectAxis(axes[i], pos, DeltaSeconds);
        }
    }

    // Keyboard.
    EInputKey Key 	    = IK_None;
    EInputKey TextKey   = IK_None;
    EInputKey MouseKey 	= IK_None;
    INT KeyValue = 0;

    // Mouse movement management.
    UBOOL MouseMoved = false;
    UBOOL UpdateViewport = false;
    UBOOL JoyHatMoved = false;
    INT DX = 0, DY = 0; // unscaled coords
	INT AbsX = 0, AbsY = 0; // unscaled coords
    EInputKey ThisJoyHat = IK_None;

    SDL_Event Event;
    while( SDL_PollEvent( &Event ) )
    {
        switch( Event.type )
        {
            case SDL_QUIT:
                GIsRequestingExit = 1;
                break;

            case SDL_KEYDOWN:

            if (bWaitForKeyUp)
                break;

            Key = (EInputKey)KeysymMap[Event.key.keysym.scancode];
            if (Key == IK_None)
                break;

            if ( ( Key == IK_G ) && (Event.key.keysym.mod & KMOD_CTRL) )
            {
                if (SDL_GetRelativeMouseMode())
                    SDL_SetRelativeMouseMode(SDL_FALSE);
                else SDL_SetRelativeMouseMode(SDL_TRUE);

                UpdateMouseGrabState(!MouseIsGrabbed);
                // Don't pass event to engine.
                break;
            }

            else if ( ( Key == IK_Enter) && (Event.key.keysym.mod & KMOD_ALT) )
            {
                ToggleFullscreen();
                bWaitForKeyUp = true;

                // Don't pass event to engine.
                break;
            }

            if ( Client->InMenuLoop || bTyping)
            {
                KeyIsPressed[Event.key.keysym.scancode] = true;
                CauseInputEvent( Key, IST_Press );
            }

            break;

            case SDL_KEYUP:

            bWaitForKeyUp = false;
            if ( Client->InMenuLoop || bTyping)
            {
                if (KeyIsPressed[Event.key.keysym.scancode])
                {
                    KeyIsPressed[Event.key.keysym.scancode] = false;
                    Key = (EInputKey)KeysymMap[Event.key.keysym.scancode]; // can't be IK_None here (see KeyIsPressed)
                    // Check the Keysym map.
                        CauseInputEvent( Key, IST_Release );
                }
            }

            break;

            case SDL_TEXTINPUT:
                // here we go, unicode textinput.
                KeyValue = ucs2_char_code_from_utf8(Event.text.text);
                TextKey = (EInputKey)KeyValue;
                if ( Client->InMenuLoop && (KeyValue > 0))
                {
                    Client->Engine->Key(this, (EInputKey)KeyValue, KeyValue);
                    TextKey = IK_None;
                }

                break;

            case SDL_MOUSEBUTTONDOWN:
                switch (Event.button.button)
                {
                    case SDL_BUTTON_LEFT:
                        MouseKey = IK_LeftMouse;
                        break;
                    case SDL_BUTTON_MIDDLE:
                        MouseKey = IK_MiddleMouse;
                        break;
                    case SDL_BUTTON_RIGHT:
                        MouseKey = IK_RightMouse;
                        break;
                    case SDL_BUTTON_X1:
                        MouseKey = IK_MouseButton4;
                        break;
                    case SDL_BUTTON_X2:
                        MouseKey = IK_MouseButton5;
                        break;
                }
                // Send to input system.
                CauseInputEvent( MouseKey, IST_Press );

                break;

            case SDL_MOUSEBUTTONUP:
                switch (Event.button.button)
                {
                    case SDL_BUTTON_LEFT:
                        MouseKey = IK_LeftMouse;
                        break;
                    case SDL_BUTTON_MIDDLE:
                        MouseKey = IK_MiddleMouse;
                        break;
                    case SDL_BUTTON_RIGHT:
                        MouseKey = IK_RightMouse;
                        break;
                    case SDL_BUTTON_X1:
                        MouseKey = IK_MouseButton4;
                        break;
                    case SDL_BUTTON_X2:
                        MouseKey = IK_MouseButton5;
                        break;
                }
                // Send to input system.
                CauseInputEvent( MouseKey, IST_Release );
                break;

            case SDL_MOUSEWHEEL:

                if (Event.wheel.y > 0)
                {
                    CauseInputEvent(IK_MouseWheelUp, IST_Press);
                    CauseInputEvent(IK_MouseWheelUp, IST_Release);
                }
                if (Event.wheel.y < 0)
                {
                    CauseInputEvent(IK_MouseWheelDown, IST_Press);
                    CauseInputEvent(IK_MouseWheelDown, IST_Release);
                }
                if (Event.wheel.x > 0)
                {
                    CauseInputEvent(IK_JoyPovLeft, IST_Press); //using JoyPov here since no defines for it specifically.
                    CauseInputEvent(IK_JoyPovLeft, IST_Release);
                }
                if (Event.wheel.x < 0)
                {
                    CauseInputEvent(IK_JoyPovRight, IST_Press);
                    CauseInputEvent(IK_JoyPovRight, IST_Release);
                }
                break;

            case SDL_MOUSEMOTION:
                if (Client->IgnoreUngrabbedMouse)
                    break;
                if (!IsFullscreen() && !MouseIsGrabbed)
                {
                    bWindowsMouseAvailable = TRUE;
                    break;
                }
                MouseMoved = true;
				AbsX = Event.motion.x;
				AbsY = Event.motion.y;
				DX += Event.motion.xrel;
				DY += Event.motion.yrel;

				if (!MouseIsGrabbed &&
					SelectedCursor >= 0 &&
					SelectedCursor <= ARRAY_COUNT(StandardCursors) &&
					StandardCursors[SelectedCursor])
				{
					SDL_SetCursor(StandardCursors[SelectedCursor]);
					ActiveCursor = StandardCursors[SelectedCursor];
				}
                break;

            case SDL_JOYBALLMOTION:
                if ( Event.jball.which != Client->JoystickNumber || !Client->UseJoystick )
                    break;
                MouseMoved = true;
                DX += (INT) (Event.jball.xrel * Client->ScaleJBX);
                DY += (INT) (Event.jball.yrel * Client->ScaleJBY);
                break;

            case SDL_JOYHATMOTION:
                if ( (Event.jhat.which != Client->JoystickNumber) ||
                        (Event.jhat.hat != Client->JoystickHatNumber) ||
                        (!Client->UseJoystick) ||
                        (Client->IgnoreHat) )
                {
                    break;
                }

                JoyHatMoved = true;

                switch ( Event.jhat.value )
                {
                case SDL_HAT_UP :
                    ThisJoyHat = IK_Joy13;
                    break;
                case SDL_HAT_DOWN :
                    ThisJoyHat = IK_Joy14;
                    break;
                case SDL_HAT_LEFT :
                    ThisJoyHat = IK_Joy15;
                    break;
                case SDL_HAT_RIGHT :
                    ThisJoyHat = IK_Joy16;
                    break;
                default :
                    ThisJoyHat = (EInputKey) 0;
                    break;
                }
            case SDL_WINDOWEVENT:
				if (Event.window.event == SDL_WINDOWEVENT_ENTER)
				{
					// resynchronize mouse position
                    SDL_GetMouseState(&MouseEnterX,&MouseEnterY);
					MouseMoved = true;
					AbsX = MouseEnterX;
					AbsY = MouseEnterY;
				}
				else if (Event.window.event == SDL_WINDOWEVENT_LEAVE)
				{
                    SDL_GetMouseState(&MouseLeaveX,&MouseLeaveY);
					MouseMoved = true;
					AbsX = MouseLeaveX;
					AbsY = MouseLeaveY;
				}
				else if (Event.window.event == SDL_WINDOWEVENT_RESIZED)
				{
					INT NewX = (IsFullscreen() ? 1 : MouseScaleX) * Event.window.data1;
					INT NewY = (IsFullscreen() ? 1 : MouseScaleY) * Event.window.data2;

					// stijn: macOS Retina support
					if (RenDev && (BlitFlags & BLIT_OpenGL))
					{
						INT WindowPointSizeX, WindowPointSizeY, WindowPixelSizeX, WindowPixelSizeY;
						SDL_GetWindowSize(Window, &WindowPointSizeX, &WindowPointSizeY);
						SDL_GL_GetDrawableSize(Window, &WindowPixelSizeX, &WindowPixelSizeY);

						if (WindowPixelSizeX > NewX || WindowPixelSizeY > NewY)
						{
							NewX = WindowPixelSizeX;
							NewY = WindowPixelSizeY;
						}
					}

					ResizeViewport(BlitFlags|BLIT_NoWindowChange, NewX, NewY, INDEX_NONE);

                    if (GIsEditor)
                        Repaint(0);
				}
                break;
            default:;
        }
    }

    // Deliver mouse behavior to the engine.
    if ( MouseMoved )
    {
        //bWindowsMouseAvailable = TRUE; //FALSE - well, Windoze mouse, but yeah, use desktop mousepointer. Edit: causes Mousepointer to disappear when moving while entering menu...
		if (!MouseIsGrabbed)
		{
			DX = AbsX * MouseScaleX - WindowsMouseX;
			DY = AbsY * MouseScaleY - WindowsMouseY;
		}

		// Send to input subsystem.
		if( DX )
			CauseInputEvent( IK_MouseX, IST_Axis, +DX );

		if( DY )
			CauseInputEvent( IK_MouseY, IST_Axis, -DY );

		if (!Client->InMenuLoop)
			Client->Engine->MouseDelta(this, 0, DX, DY);

		if (!IsRealtime())
		{
			if( Input->KeyDown(IK_Space) )
			{
				for( INT i=0; i<Client->Viewports.Num(); i++ )
					Client->Viewports(i)->Repaint( 1 );
			}
			else
			{
				Repaint( 1 );
			}
		}

		WindowsMouseX = AbsX * MouseScaleX;
		WindowsMouseY = AbsY * MouseScaleY;

		if (WindowsMouseX > SizeX)
			WindowsMouseX = SizeX;

		if (WindowsMouseX < 0)
			WindowsMouseX = 0;

		if (WindowsMouseY > SizeY)
			WindowsMouseY = SizeY;
    }
    else if( bWindowsMouseAvailable )
    {
        // If Mouse is not grabbed use OS mouse in menu instead to avoid unprecise offset.
		INT SavedCursorX = 0, SavedCursorY = 0;
        SDL_GetMouseState(&SavedCursorX, &SavedCursorY);
        WindowsMouseX = SavedCursorX * MouseScaleX;
        WindowsMouseY = SavedCursorY * MouseScaleY;
    }

    if ( (LastJoyHat != ThisJoyHat) && JoyHatMoved )
    {
        if (LastJoyHat)
            CauseInputEvent( LastJoyHat, IST_Release );
        if (ThisJoyHat)
            CauseInputEvent( ThisJoyHat, IST_Press );
        LastJoyHat = ThisJoyHat;
    }

    // Keyboard gaming input.
    KeyboardState = SDL_GetKeyboardState(NULL);
    if (!Client->InMenuLoop)
    {
        for (INT i=0; i<SDL_NUM_SCANCODES; i++)
        {
            Key = (EInputKey)KeysymMap[i];

            if (KeyIsPressed[i] && !KeyboardState[i])
                KeyIsPressed[i] = false;

            if (Key != IK_None && !bTyping)
            {
                if (KeyboardState[i] && !Reset)
                {
                    if ( (Key == UWindowKey) || (Key == ConsoleKey) || (Key == IK_Enter))
                    {
                        if (!KeyIsPressed[i])
                        {
                            CauseInputEvent( Key, IST_Press );
                            KeyIsPressed[i] = true;
                        }
                    }
                    else CauseInputEvent( Key, IST_Press );
                }
                else if (!KeyboardState[i])
                    CauseInputEvent( Key, IST_Release );
            }
        }
    }
    if (TextKey != IK_None)
        Client->Engine->Key(this, TextKey, KeyValue);
    unguard;
}



/*-----------------------------------------------------------------------------
	Lock and Unlock.
-----------------------------------------------------------------------------*/

//
// Lock the viewport window and set the approprite Screen and RealScreen fields
// of Viewport.  Returns 1 if locked successfully, 0 if failed.  Note that a
// lock failing is not a critical error; it's a sign that a DirectDraw mode
// has ended or the user has closed a viewport window.
//
UBOOL USDL2Viewport::Lock( FPlane FlashScale, FPlane FlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* HitData, INT* HitSize )
{
    guard(USDL2Viewport::LockWindow);
    USDL2Client* Client = GetOuterUSDL2Client();
    clockFast(Client->DrawCycles);

    // Success here, so pass to superclass.
    unclockFast(Client->DrawCycles);
    return UViewport::Lock(FlashScale,FlashFog,ScreenClear,RenderLockFlags,HitData,HitSize);

    unguard;
}

//
// Unlock the viewport window.  If Blit=1, blits the viewport's frame buffer.
//
void USDL2Viewport::Unlock( UBOOL Blit )
{
    guard(USDL2Viewport::Unlock);
    USDL2Client* Client = GetOuterUSDL2Client();

    Client->DrawCycles = 0;
    UViewport::Unlock( Blit );

    unguard;
}

/*-----------------------------------------------------------------------------
	Viewport modes.
-----------------------------------------------------------------------------*/

//
// Try switching to a new rendering device.
//
void USDL2Viewport::TryRenderDevice( const TCHAR* ClassName, INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen )
{
    guard(USDL2Viewport::TryRenderDevice);

    // Shut down current rendering device.
    if( RenDev )
    {
        RenDev->Exit();
        delete RenDev;
        RenDev = NULL;
    }

    // Use appropriate defaults.
    USDL2Client* C = GetOuterUSDL2Client();
    if( NewX==INDEX_NONE )
        NewX = Fullscreen ? C->FullscreenViewportX : C->WindowedViewportX;
    if( NewY==INDEX_NONE )
        NewY = Fullscreen ? C->FullscreenViewportY : C->WindowedViewportY;
    if( NewColorBytes == INDEX_NONE )
        NewColorBytes = Fullscreen ? (C->FullscreenColorBits/8) :  (C->WindowedColorBits/8);

    // Find device driver.
    UClass* RenderClass = UObject::StaticLoadClass( URenderDevice::StaticClass(), NULL, ClassName, NULL, 0, NULL );
    if( RenderClass )
    {
        debugf( TEXT("SDL2: Loaded render device class.") );
        //HoldCount++;
        RenDev = ConstructObject<URenderDevice>( RenderClass, this );
        if( RenDev->Init( this, NewX, NewY, NewColorBytes, Fullscreen ) )
        {
            if( GIsRunning )
                Actor->GetLevel()->DetailChange( RenDev->HighDetailActors );
        }
        else
        {
            debugf( NAME_Log, *LocalizeError(TEXT("Failed3D")) );
            delete RenDev;
            RenDev = NULL;
        }
        //HoldCount--;
    }
    GRenderDevice = RenDev;
    unguard;
}


//
// If in fullscreen mode, end it and return to Windows.
//
void USDL2Viewport::EndFullscreen()
{
    guard(USDL2Viewport::EndFullscreen);
    //USDL2Client* Client = GetOuterUSDL2Client();
    debugf(NAME_Log, TEXT("SDL2: Ending fullscreen mode by request."));
    if( RenDev && RenDev->FullscreenOnly )
    {
        // This device doesn't support fullscreen, so use a window-capable rendering device.
        TryRenderDevice( TEXT("ini:Engine.Engine.WindowedRenderDevice"), INDEX_NONE, INDEX_NONE, ColorBytes, 0 );
        check(RenDev);
    }
    else if( RenDev && (BlitFlags & BLIT_OpenGL) )
    {
        RenDev->SetRes( INDEX_NONE, INDEX_NONE, ColorBytes, 0 );
    }
    else
    {
        ResizeViewport( BLIT_DibSection );
    }
    UpdateWindowFrame();

    if ( Input )
        Input->ResetInput();

    unguard;
}

//
// Toggle fullscreen.
//
void USDL2Viewport::ToggleFullscreen()
{
    guard(USDL2Viewport::ToggleFullscreen);
    debugf(NAME_Log, TEXT("SDL2: ToggleFullscreen"));
    if( BlitFlags & BLIT_Fullscreen )
    {
        EndFullscreen();
    }
    else if( !(Actor->ShowFlags & SHOW_ChildWindow) )
    {
        RenDev->SetRes( INDEX_NONE, INDEX_NONE, ColorBytes, 1 );
    }
    unguard;
}

//
// Resize the viewport.
//
UBOOL USDL2Viewport::ResizeViewport( DWORD NewBlitFlags, INT InNewX, INT InNewY, INT InNewColorBytes)
{
    guard(USDL2Viewport::ResizeViewport);
    USDL2Client* Client = GetOuterUSDL2Client();

    // Andrew says: I made this keep the BLIT_Temporary flag so the temporary screen buffer doesn't get leaked
    // during light rebuilds.

    // Remember viewport.
    UViewport* SavedViewport = NULL;
    if( Client->Engine->Audio && !GIsEditor && !(GetFlags() & RF_Destroyed) )
        SavedViewport = Client->Engine->Audio->GetViewport();

    UBOOL RequestFullScreen = (NewBlitFlags&BLIT_Fullscreen);
	const INT FullScreenFlags = SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP;
    UBOOL IsFullScreen = Window ? (SDL_GetWindowFlags(Window) & FullScreenFlags) : 0;

	UBOOL ShouldResize = TRUE;

	// Default resolution handling.
	INT NewX = InNewX != INDEX_NONE ? InNewX : RequestFullScreen ?
       Client->FullscreenViewportX : Client->WindowedViewportX;
	INT NewY = InNewY != INDEX_NONE ? InNewY : RequestFullScreen ?
       Client->FullscreenViewportY : Client->WindowedViewportY;

    //debugf(TEXT("SDL2: %ls Client->WindowedViewportX %i Client->WindowedViewportY %i"),Client->GetFullName(), Client->WindowedViewportX, Client->WindowedViewportY);
    //debugf(TEXT("SDL2: ResizeViewport call: SizeX %i, SizeY %i, InNewX %i, InNewY %i, IsFullScreen %i, RequestFullScreen %i, ColorBytes %i"),SizeX, SizeY, InNewX, InNewY, IsFullScreen, RequestFullScreen, ColorBytes);

    INT NewColorBytes = InNewColorBytes==INDEX_NONE ? ColorBytes : InNewColorBytes;

    if (Window)
    {
        if (SizeX == NewX && SizeY == NewY)
        {
            if ((RequestFullScreen && IsFullScreen) || (!RequestFullScreen &&!IsFullScreen))
            {
                debugf(TEXT("SDL2Drv: No need to resize viewport"));
                return 1;
            }
        }
        else if (Client->UseDesktopFullScreen && IsFullScreen && RequestFullScreen)
        {
            INT WindowPointSizeX, WindowPointSizeY;
            SDL_GetWindowSize(Window, &WindowPointSizeX, &WindowPointSizeY);
            Client->FullscreenViewportX = WindowPointSizeX;
            Client->FullscreenViewportY = WindowPointSizeY;
            Client->SaveConfig();
            debugf(TEXT("SDL2Drv: UseDesktopFullScreen is enabled for fake fullscreen that takes the size of the desktop (%ix%i) - change desktop resolution instead."),WindowPointSizeX,WindowPointSizeY);
            return 0;
        }
    }

    // Align NewX.
    check(NewX>=0);
    check(NewY>=0);
    NewX = Align(NewX,2);

    if( !(NewBlitFlags & BLIT_Temporary) )
    {
        ScreenPointer = NULL;
    }

    // Pull current driver string.
    FString CurrentDriver = RenDev->GetClass()->GetPathName();

    //FIXME! Currently OpenGL only.

    // Checking whether to use Glide specific settings or OpenGL.
    INT VideoFlags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE;
    if ( !appStrcmp( *CurrentDriver, TEXT("GlideDrv.GlideRenderDevice") ) )
    {
        char EnvString[]="SDL_VIDEO_X11_DGAMOUSE=0";
        // Don't use DGA in conjunction with Glide unless user overrides it.
        if ( getenv( "SDL_VIDEO_X11_DGAMOUSE" ) == NULL )
            putenv( EnvString );
        NewBlitFlags |= BLIT_Fullscreen;
        debugf( TEXT("SDL2: Using GlideDrv.GlideRenderDevice") );
    }
    else if( NewBlitFlags & BLIT_OpenGL )
    {
        VideoFlags |= SDL_WINDOW_OPENGL;
        debugf( TEXT("SDL2: Using OpenGL / OpenGLES") );

#if __APPLE__
		//
		// stijn: HACK: we need to force macs to use a 32-bit depth buffer, even
		// if the rendev requests a different depth buffer size.
		// Without this hack, Unreal@macOS turns into a Z-flickering fest.
		//
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 32);
#endif
    }
    if(RequestFullScreen)
    {
        if (Client->UseDesktopFullScreen)
        {
            VideoFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP|SDL_WINDOW_MAXIMIZED|SDL_WINDOW_ALWAYS_ON_TOP;
            debugf(TEXT("SDL2: Using desktop resolution for fullscreen"));
        }
        else
        {
            VideoFlags |= SDL_WINDOW_FULLSCREEN|SDL_WINDOW_MAXIMIZED|SDL_WINDOW_ALWAYS_ON_TOP; // the 2 additional flags apparently fix all bad behavior when changing fullscreen resolution.
            debugf(TEXT("SDL2: Changing desktop resolution for fullscreen (%ix%i)"),InNewX,InNewY);
        }
    }

    INT RefreshRate = 0, TempRefreshRate = 0;
    if (GConfig->GetInt(*CurrentDriver, TEXT("RefreshRate"), TempRefreshRate))
        RefreshRate = TempRefreshRate;

    if (RefreshRate)
        debugf(TEXT("SDL2: Requesting RefreshRate of %i"),RefreshRate);

    SDL_ShowCursor(SDL_ENABLE);
    if (!Window)
    {
        debugf(TEXT("SDL2: Creating new window with %ix%i"),NewX,NewY);
        Window =  SDL_CreateWindow(appToAnsi((*LocalizeGeneral(TEXT("Product"),appPackage()))), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, NewX, NewY, VideoFlags);
        if (Window == NULL)
        {
            appMsgf( TEXT("Couldn't create SDL2 Window (%ix%i): %ls\n"), NewX, NewY,appFromAnsi(SDL_GetError()) );
            return 0;
        }
        if (!IsFullScreen)
            SDL_SetWindowPosition(Window,Client->WindowPosX,Client->WindowPosY);
    }
    else
    {
        debugf(TEXT("SDL2: Changing window mode"));
        SDL_DisplayMode	Mode{};
        if (SDL_GetWindowDisplayMode(Window, &Mode) == -1)
			Mode.format = SDL_PIXELFORMAT_UNKNOWN;

		// stijn: check if we actually need to resize. Chances are we came here
		// after receiving an SDL_WINDOWEVENT_RESIZED
		if (IsFullScreen && RequestFullScreen && Mode.w == NewX && Mode.h == NewY && Mode.refresh_rate == RefreshRate)
		{
			ShouldResize = FALSE;
		}
		else if (!IsFullScreen && !RequestFullScreen)
		{
			INT WindowPixelSizeX, WindowPixelSizeY;
			SDL_GL_GetDrawableSize(Window, &WindowPixelSizeX, &WindowPixelSizeY);
			if (WindowPixelSizeX == NewX && WindowPixelSizeY == NewY)
				ShouldResize = FALSE;
		}

		Mode.w = NewX;
		Mode.h = NewY;
		Mode.refresh_rate = RefreshRate;

		if (RequestFullScreen)
		{
			// stijn: I don't think you really need to do this...
			SDL_SetWindowAlwaysOnTop(Window,SDL_TRUE); // but yes, since using SDL_WINDOW_ALWAYS_ON_TOP videoflag need to disable this for windowed ;)
			debugf( TEXT("SDL2: RequestFullScreen of (%ix%i)"), NewX,NewY );

			if (!IsFullScreen)
			{
				//Store window position.
				SDL_GetWindowPosition(Window,&Client->WindowPosX,&Client->WindowPosY);

				if (SDL_SetWindowFullscreen(Window, VideoFlags & FullScreenFlags))
					debugf( TEXT("SDL2: Couldn't switch into fullscreen mode (%ix%i): %ls\n"), NewX, NewY,appFromAnsi(SDL_GetError()) );
				else debugf( TEXT("SDL2: Switched into fullscreen mode (%ix%i)"), NewX, NewY);
			}

			if (ShouldResize && !Client->UseDesktopFullScreen)
			{
				if (SDL_SetWindowDisplayMode(Window, &Mode))
					debugf( TEXT("SDL2: Couldn't set resolution of (%ix%i): %ls\n"), NewX, NewY,appFromAnsi(SDL_GetError()) );
				else debugf( TEXT("SDL2: Set resolution of (%ix%i)"), NewX, NewY );

				SDL_SetWindowSize(Window, NewX, NewY);
			}
		}
		else // !RequestFullScreen
		{
			if (IsFullScreen)
			{
				debugf(TEXT("SDL2: Changing fullscreen to windowed mode"));
				if (SDL_SetWindowFullscreen(Window, 0))
					debugf( TEXT("SDL2: Couldn't switch into windowed mode of (%ix%i): %ls\n"), NewX, NewY,appFromAnsi(SDL_GetError()) );
				else
                {
                    SDL_SetWindowAlwaysOnTop(Window,SDL_FALSE);
                    debugf( TEXT("SDL2: Switched into windowed mode of (%ix%i): %ls\n"), NewX, NewY,appFromAnsi(SDL_GetError()) );
				}
				SDL_SetWindowSize(Window,NewX,NewY);
				//SDL_Delay(1);
			}

			if (ShouldResize)
			{
				SDL_RestoreWindow(Window); // without this it won't resize the  window frame accordingly and fails to actually resize in fullscreen.
				SDL_SetWindowSize(Window,NewX,NewY); //odd enough, have to set again, otherwise it will fail on NewX. Perhaps some bug with gnome, but won't do any harm elsewhere.
				SDL_SetWindowPosition(Window,Client->WindowPosX,Client->WindowPosY);
			}
		}
	}

    if (Window == NULL)
    {
        appMsgf( TEXT("SDL2: Couldn't create SDL Window (%ix%i): %ls\n"), NewX, NewY,appFromAnsi(SDL_GetError()) );
        return 0;
    }

	// Get effective window size and fullscreen mode. Might differ from what we requested
    IsFullScreen = SDL_GetWindowFlags(Window) & FullScreenFlags;
	INT WindowPointSizeX, WindowPointSizeY, WindowPixelSizeX, WindowPixelSizeY;
	SDL_GetWindowSize(Window, &WindowPointSizeX, &WindowPointSizeY);
	if (BlitFlags & BLIT_OpenGL)
	{
		SDL_GL_GetDrawableSize(Window, &WindowPixelSizeX, &WindowPixelSizeY);
	}
	else
	{
		WindowPixelSizeX = WindowPointSizeX;
		WindowPixelSizeY = WindowPointSizeY;
	}

	MouseScaleX = WindowPixelSizeX / WindowPointSizeX;
	MouseScaleY = WindowPixelSizeY / WindowPointSizeY;
	//debugf(TEXT("SDL2Drv: Window Point Size %dx%d - Window Pixel Size %dx%d - Mouse Scale %lf,%lf"), WindowPointSizeX, WindowPointSizeY, WindowPixelSizeX, WindowPixelSizeY, MouseScaleX, MouseScaleY);

	if (WindowPointSizeX != NewX || WindowPointSizeY != NewY)
	{
		debugf(TEXT("SDL2Drv WARNING: Requested %dx%d window but got %dx%d - this is probably a high-dpi scaled resolution"), NewX, NewY, WindowPointSizeX, WindowPointSizeY);
	}

	if (IsFullScreen && !Client->UseDesktopFullScreen)
    {
        INT Result=-1;
        SDL_DisplayMode	NewMode;
        Result=SDL_GetWindowDisplayMode(Window, &NewMode);
        if(Result != 0)
        {
            debugf( TEXT("SDL2: Couldn't get current display mode %ls\n"), appFromAnsi(SDL_GetError()) );
        }
        if (RefreshRate && RefreshRate != NewMode.refresh_rate)
        {
            debugf(TEXT("SDL2: Requested RefreshRate of %i but got %i"), RefreshRate, NewMode.refresh_rate);
            GConfig->SetInt(*CurrentDriver, TEXT("RefreshRate"), NewMode.refresh_rate);
        }
    }

    // Make this viewport current and update its title bar.
    GetOuterUClient()->MakeCurrent(this);

    // Set new info.
    ColorBytes         = NewColorBytes ? NewColorBytes : ColorBytes;
    BlitFlags          = NewBlitFlags & ~BLIT_ParameterFlags;

    // Save info.
    if (WindowPointSizeX > 0 &&
		WindowPointSizeY > 0 &&
		RenDev &&
		!GIsEditor)
	{
		if (IsFullScreen)
		{
			Client->FullscreenViewportX = WindowPointSizeX;
			Client->FullscreenViewportY = WindowPointSizeY;
			Client->FullscreenColorBits = ColorBytes*8;
			//debugf(TEXT("SDL2: FullscreenViewportX %i FullscreenViewportY %i"),Client->FullscreenViewportX,Client->FullscreenViewportY);
		}
		else
		{
			Client->WindowedViewportX = WindowPointSizeX;
			Client->WindowedViewportY = WindowPointSizeY;
			Client->WindowedColorBits = ColorBytes*8;
			//debugf(TEXT("SDL2: WindowedViewportX %i WindowedViewportY %i"),Client->WindowedViewportX, Client->WindowedViewportY);

		}
		Client->SaveConfig();
	}

	// This is the framebuffer size!
    SizeX = WindowPixelSizeX;
	SizeY = WindowPixelSizeY;

    // Mouse
    SetMouseCapture(Client->CaptureMouse,1,0);

    // Update audio.
    if( SavedViewport && SavedViewport!=Client->Engine->Audio->GetViewport() )
        Client->Engine->Audio->SetViewport( SavedViewport );

    // Update the window.
    UpdateWindowFrame();

    debugf(TEXT("SDL2Drv: Resizing viewport successful!"));

    return 1;
    unguard;
}

//
// Get localized keyname. Uhhhm.?
//
TCHAR * USDL2Viewport::GetLocalizedKeyName( EInputKey Key )
{
    return (TCHAR*)appFromAnsi(SDL_GetKeyName((SDL_Keycode)Key));
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
