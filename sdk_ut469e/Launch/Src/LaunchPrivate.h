/*=============================================================================
	LaunchPrivate.h: Unreal launcher.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Tim Sweeney.
=============================================================================*/

#if !__STATIC_LINK
#define STRICT
#pragma warning(push)
#pragma warning(disable: 4121) /* 'JOBOBJECT_IO_RATE_CONTROL_INFORMATION_NATIVE_V2': alignment of a member was sensitive to packing */
#pragma warning(disable : 4091)
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#pragma warning(pop)
#endif
#include "Engine.h"
#include "UnRender.h"
#include <commctrl.h>
#include <commdlg.h>
#include <ShellAPI.h>
#include "Window.h"
#include "Res\LaunchRes.h"

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
