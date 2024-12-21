/*=============================================================================
	SDL2LaunchPrivate.h: Unreal launcher for X.
	Copyright 1999 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Daniel Vogel (based on XLaunch).
=============================================================================*/

#if _MSC_VER
	#pragma warning( disable : 4201 )
	#define STRICT
	#define SDLBUILD
	#include <windows.h>
	#include <commctrl.h>
	#include <shlobj.h>
	#include <malloc.h>
	#include <io.h>
	#include <direct.h>
	#include <errno.h>
	#include <stdio.h>
	#include <sys/stat.h>
	#include "Engine.h"
	#include "UnRender.h"
	#include "Res\LaunchRes.h"
	#include <process.h>     // needed for _beginthreadex()
#else
	#include <stdio.h>
	#include <stdlib.h>
	#include <errno.h>
	#include <sys/stat.h>
	#include <time.h>

	#if !MACOSX &&!MACOSXPPC
	#include <malloc.h>
	#endif

	#include <fcntl.h>
	#include "Engine.h"
	#include "Render.h"
	#include <pthread.h> // needed for pthread_create()
#endif

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
