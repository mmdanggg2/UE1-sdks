/*=============================================================================
	D3DDrv.cpp: Unreal Direct3D driver precompiled header generator.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by independent contractor who wishes to remanin anonymous.
		* Taken over by Tim Sweeney.
=============================================================================*/

// Windows includes.
#pragma warning(push)
#pragma warning(disable: 4121) /* 'JOBOBJECT_IO_RATE_CONTROL_INFORMATION_NATIVE_V2': alignment of a member was sensitive to packing */
#include <windows.h>
#pragma warning(pop)
#include <objbase.h>

// Unreal includes.
#include "Engine.h"
#include "UnRender.h"

// Direct3D includes.
#define DIRECT3D_VERSION 0x0700
#include "d3d.h"

/*-----------------------------------------------------------------------------
	End.
-----------------------------------------------------------------------------*/
