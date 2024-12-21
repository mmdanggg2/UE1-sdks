/*=============================================================================
	SDL2Client.cpp: USDL2Client code.
	Copyright 2002 Epic Games, Inc. All Rights Reserved.

        SDL website: http://www.libsdl.org/

Revision history:
	* Created by Ryan C. Gordon, based on WinDrv.
      This is an updated rewrite of the original SDLDrv.
	* Updated to SDL2 with some improvements by Smirftsch www.oldunreal.com
=============================================================================*/

#include <stdlib.h>
#include "SDL2Drv.h"

/*-----------------------------------------------------------------------------
	Class implementation.
-----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(USDL2Client);

/*-----------------------------------------------------------------------------
	USDL2Client implementation.
-----------------------------------------------------------------------------*/

SDL_Joystick *USDL2Client::Joystick = NULL;
int USDL2Client::JoystickAxes = 0;
int USDL2Client::JoystickButtons = 0;
int USDL2Client::JoystickHats = 0;
int USDL2Client::JoystickBalls = 0;


//
// USDL2Client constructor.
//
USDL2Client::USDL2Client()
{
	guard(USDL2Client::USDL2Client);
	unguard;
}

//
// Static init.
//
void USDL2Client::StaticConstructor()
{
	guard(USDL2Client::StaticConstructor);
	new(GetClass(),TEXT("JoystickNumber"),       RF_Public)UIntProperty(CPP_PROPERTY(JoystickNumber),        TEXT("Joystick"), CPF_Config );
	new(GetClass(),TEXT("JoystickHatNumber"),    RF_Public)UIntProperty(CPP_PROPERTY(JoystickHatNumber),     TEXT("Joystick"), CPF_Config );
    new(GetClass(),TEXT("WindowPosX"),           RF_Public)UIntProperty(CPP_PROPERTY(WindowPosX),            TEXT("Display"),  CPF_Config );
    new(GetClass(),TEXT("WindowPosY"),           RF_Public)UIntProperty(CPP_PROPERTY(WindowPosY),            TEXT("Display"),  CPF_Config );
	new(GetClass(),TEXT("ScaleJBX"),             RF_Public)UFloatProperty(CPP_PROPERTY(ScaleJBX),            TEXT("Joystick"), CPF_Config );
	new(GetClass(),TEXT("ScaleJBY"),             RF_Public)UFloatProperty(CPP_PROPERTY(ScaleJBY),            TEXT("Joystick"), CPF_Config );
	new(GetClass(),TEXT("IgnoreHat"),            RF_Public)UBoolProperty(CPP_PROPERTY(IgnoreHat),            TEXT("Joystick"), CPF_Config );
	new(GetClass(),TEXT("IgnoreUngrabbedMouse"), RF_Public)UBoolProperty(CPP_PROPERTY(IgnoreUngrabbedMouse), TEXT("Display"),  CPF_Config );
	new(GetClass(),TEXT("UseJoystick"),          RF_Public)UBoolProperty(CPP_PROPERTY(UseJoystick),          TEXT("Display"),  CPF_Config );
	new(GetClass(),TEXT("StartupFullscreen"),    RF_Public)UBoolProperty(CPP_PROPERTY(StartupFullscreen),    TEXT("Display"),  CPF_Config );
	new(GetClass(),TEXT("UseDesktopFullScreen"), RF_Public)UBoolProperty(CPP_PROPERTY(UseDesktopFullScreen), TEXT("Display"),  CPF_Config );
	new(GetClass(),TEXT("UseRawHIDInput"),	     RF_Public)UBoolProperty(CPP_PROPERTY(UseRawHIDInput),	     TEXT("Display"),  CPF_Config );
	unguard;
}


// just in case.  :)  --ryan.
static void sdl_atexit_handler(void)
{
    static bool already_called = false;

    if (!already_called)
    {
        already_called = true;
        SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
    }
}



//
// Initialize the platform-specific viewport manager subsystem.
// Must be called after the Unreal object manager has been initialized.
// Must be called before any viewports are created.
//
void USDL2Client::Init( UEngine* InEngine )
{
	guard(USDL2Client::USDL2Client);

	debugf(TEXT("SDL2: Initializing"));

	// Init base.
	UClient::Init( InEngine );

	atexit(sdl_atexit_handler);

	// Note configuration.
	PostEditChange();

	// Default res option.
	if( ParseParam(appCmdLine(),TEXT("defaultres")) )
	{
	    // gam ---
		WindowedViewportX  = FullscreenViewportX  /*= MenuViewportX*/   = 640;
		WindowedViewportY  = FullscreenViewportY  /*= MenuViewportY*/   = 480;
		// --- gam
	}

	// Joystick.
    int joystickCount = SDL_NumJoysticks();
    JoystickButtons = 0;
    JoystickAxes = 0;
    debugf( NAME_Init, TEXT("SDL2: Detected %d joysticks"), joystickCount);
    if (joystickCount > 0)
    {
    	if ( JoystickNumber >= joystickCount )
    	{
    	    debugf( NAME_Init, TEXT("SDL2: JoystickNumber exceeds the number of detected joysticks, setting to 0."));
            JoystickNumber = 0;
    	}
		INT i;
		const TCHAR *joyName=TEXT("SDL2: Unknown Joystick");
		for(i = 0; i < joystickCount; i++)
		{
			if (JoystickNumber == i)
			{
				SDL_Joystick *joystick = SDL_JoystickOpen(i);
				joyName = appFromAnsi(SDL_JoystickName(joystick));
				break;
			}
		}

    	debugf( NAME_Init, TEXT("SDL2: Joystick [%i] : %ls") ,
				JoystickNumber , joyName);

    	Joystick = SDL_JoystickOpen( JoystickNumber );
		if ( Joystick == NULL )
		{
		    debugf( NAME_Init, TEXT("SDL2: Couldn't open joystick [%ls]"), joyName );
			UseJoystick = false;
  		}
   	    else
   		{
   			JoystickButtons = SDL_JoystickNumButtons( Joystick );
   		    debugf( NAME_Init, TEXT("SDL2: Joystick has %i buttons"), JoystickButtons );

   			JoystickHats = SDL_JoystickNumHats( Joystick );
   		    debugf( NAME_Init, TEXT("SDL2: Joystick has %i hats"), JoystickHats );

   			JoystickBalls = SDL_JoystickNumBalls( Joystick );
   		    debugf( NAME_Init, TEXT("SDL2: Joystick has %i balls"), JoystickBalls );

            if ((JoystickHatNumber < 0) || (JoystickHatNumber >= JoystickHats))
            {
    	        debugf( NAME_Init, TEXT("SDL2: JoystickHatNumber exceeds the number of detected hats, setting to 0."));
                JoystickHatNumber = 0;
            }

            if (JoystickButtons > 16)
                JoystickButtons = 16;

            if ((JoystickButtons > 12) && (JoystickHats > 0) && (!IgnoreHat))
                JoystickButtons = 12;  /* joy13 is first hat "button" */

            if (JoystickButtons != SDL_JoystickNumButtons(Joystick))
            {
                debugf( NAME_Init, TEXT("SDL2: Too many joystick buttons; clamped to %d."), JoystickButtons);
                if ((JoystickHats > 0) && (!IgnoreHat))
                    debugf( NAME_Init, TEXT("SDL2: (Disable hat switches with \"IgnoreHat=True\" to raise this.)"));
            }

   			JoystickAxes    = SDL_JoystickNumAxes( Joystick );
   			debugf( NAME_Init, TEXT("SDL2: Joystick has %i axes"   ), JoystickAxes );

            // x + y + z + r + u + v + 2 "sliders" == 8 axes.
            if (JoystickAxes > 8)
            {
                debugf( NAME_Init, TEXT("SDL2: Too many joystick axes; clamped to 8."));
                JoystickAxes = 8;
            }
   	    }
	}
    SDL_StartTextInput();

	// Success.
	debugf( NAME_Init, TEXT("SDL2: Client initialized.") );

	unguard;
}


//
// Shut down the platform-specific viewport manager subsystem.
//
void USDL2Client::Destroy()
{
	guard(USDL2Client::Destroy);

	// Make sure to shut down Viewports first.
	for( INT i=0; i<Viewports.Num(); i++ )
		Viewports(i)->ConditionalDestroy();

	// Shut down GRenDev.
	//GRenderDevice->Exit();

	SDL_StopTextInput();

	if ( Joystick != NULL )
		SDL_JoystickClose( Joystick );

    SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);

	debugf( NAME_Exit, TEXT("SDL2: client shut down") );
	Super::Destroy();
	unguard;
}
//
// Failsafe routine to shut down viewport manager subsystem
// after an error has occured. Not guarded.
//
void USDL2Client::ShutdownAfterError()
{
	debugf( NAME_Exit, TEXT("SDL2: Executing USDL2Client::ShutdownAfterError") );
    SDL_Quit();

	if (Engine && Engine->Audio)
	{
		Engine->Audio->ConditionalShutdownAfterError();
	}

	for (INT i = Viewports.Num() - 1; i >= 0; i--)
	{
		USDL2Viewport *Viewport = (USDL2Viewport *) Viewports(i);
		Viewport->ConditionalShutdownAfterError();
	}

	Super::ShutdownAfterError();
}

void USDL2Client::NotifyDestroy( void* Src )
{
	guard(USDL2Client::NotifyDestroy);
	unguard;
}

//
// Command line.
//
UBOOL USDL2Client::Exec( const TCHAR* Cmd, FOutputDevice& Ar )
{
	guard(USDL2Client::Exec);
	if( UClient::Exec( Cmd, Ar ) )
	{
		return 1;
	}
	/*
    if( ParseCommand(&Cmd,TEXT("DEBUGCRASH")))
	{
        if(GWarn->YesNof(TEXT("Really want to DebugCrash?")))
        {
            appErrorf(TEXT("SDL2: Crash on demand"));
        }
		return 1;
	}
	*/
	return 0;
	unguardf((TEXT("Crashed by purpose")));
}

//
// Perform timer-tick processing on all visible viewports.  This causes
// all realtime viewports, and all non-realtime viewports which have been
// updated, to be blitted.
//
void USDL2Client::Tick()
{
	guard(USDL2Client::Tick);

	// Blit any viewports that need blitting.
  	for( INT i=0; i<Viewports.Num(); i++ )
	{
		USDL2Viewport* Viewport = CastChecked<USDL2Viewport>(Viewports(i));

		//SDL_SetWindowSize(Viewport->Window,Viewport->SizeX ,Viewport->SizeY);

		if (!Viewport->IsFullscreen())
		{
			// Check if Window is resized and adjust Viewport accordingly.

			// stijn: this is point size. We need pixel size :D
			// SDL_GetWindowSize(Viewport->Window, &WindowX,&WindowY);
			
			INT WindowPixelSizeX, WindowPixelSizeY;
			SDL_GL_GetDrawableSize(Viewport->Window, &WindowPixelSizeX, &WindowPixelSizeY);
			if (WindowPixelSizeX !=Viewport->SizeX || WindowPixelSizeY != Viewport->SizeY)
			{
				Viewport->SizeX=WindowPixelSizeX;
				Viewport->SizeY=WindowPixelSizeY;
			}
		}

		if
		(	(Viewport->IsRealtime() || Viewport->RepaintPending)
		&&	Viewport->SizeX
		&&	Viewport->SizeY )
		{
			Viewport->Repaint( 1 );
		}

	}

	unguard;
}

//
// Create a new viewport.
//
UViewport* USDL2Client::NewViewport( const FName Name )
{
	guard(USDL2Client::NewViewport);
	return new( this, Name )USDL2Viewport();
	unguard;
}

//
// Configuration change.
//
void USDL2Client::PostEditChange()
{
	guard(USDL2Client::PostEditChange);
	Super::PostEditChange();
	unguard;
}

//
// Enable or disable all viewport windows that have ShowFlags set (or all if ShowFlags=0).
//
void USDL2Client::EnableViewportWindows( DWORD ShowFlags, int DoEnable )
{
	guard(USDL2Client::EnableViewportWindows);
    /* meaningless in terms of SDL. */
	unguard;
}

//
// Show or hide all viewport windows that have ShowFlags set (or all if ShowFlags=0).
//
void USDL2Client::ShowViewportWindows( DWORD ShowFlags, int DoShow )
{
	guard(USDL2Client::ShowViewportWindows);
    /* meaningless in terms of SDL. */
	unguard;
}

//
// Make this viewport the current one.
// If Viewport=0, makes no viewport the current one.
//
void USDL2Client::MakeCurrent( UViewport* InViewport )
{
	guard(UWindowsViewport::MakeCurrent);
	for( INT i=0; i<Viewports.Num(); i++ )
	{
		UViewport* OldViewport = Viewports(i);
		if( OldViewport->Current && OldViewport!=InViewport )
		{
			OldViewport->Current = 0;
			OldViewport->UpdateWindowFrame();
		}
	}
	if( InViewport )
	{
//		LastCurrent = InViewport;
		InViewport->Current = 1;
		InViewport->UpdateWindowFrame();
	}
	unguard;
}

//
// DPI Awareness.
//
INT USDL2Client::GetDPIScaledX(INT X)
{
	const FLOAT DefaultDPI =
#if __APPLE__
		72.f;
#else
	    96.f;
#endif

    FLOAT DesktopDPI;
	if (SDL_GetDisplayDPI(0, NULL, &DesktopDPI, NULL) == 0)
		return appCeil(static_cast<FLOAT>(X) * (DesktopDPI / DefaultDPI));
	return X;
}

INT USDL2Client::GetDPIScaledY(INT Y)
{
	const FLOAT DefaultDPI =
#if __APPLE__
		72.f;
#else
	    96.f;
#endif

    FLOAT DesktopDPI;
	if (SDL_GetDisplayDPI(0, NULL, &DesktopDPI, NULL) == 0)
		return appCeil(static_cast<FLOAT>(Y) * (DesktopDPI / DefaultDPI));
	return Y;
}

/*-----------------------------------------------------------------------------
    That's all, folks.
-----------------------------------------------------------------------------*/

