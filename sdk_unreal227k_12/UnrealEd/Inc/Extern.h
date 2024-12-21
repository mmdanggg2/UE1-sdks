/*=============================================================================
	Extern.h: External declarations that we need to communicate with editor.dll
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall
=============================================================================*/

#if __STATIC_LINK
extern void __stdcall NE_EdInit(HWND hInWndMain, HWND hInWndCallback);
extern FStringOutputDevice* GetPropResult;
#else
DLL_IMPORT void __stdcall NE_EdInit(HWND hInWndMain, HWND hInWndCallback);
DLL_IMPORT FStringOutputDevice* GetPropResult;
#endif
CORE_API extern UViewport* GCurrentViewport;

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
