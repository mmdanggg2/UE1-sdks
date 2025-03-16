/*=============================================================================
	OpenGlDrv.h: Unreal OpenGL support header.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
		* Multitexture and context support - Andy Hanson (hanson@3dfx.com) and
		  Jack Mathews (jack@3dfx.com)
		* Unified by Daniel Vogel
		
=============================================================================*/

/*-----------------------------------------------------------------------------
	Includes.
-----------------------------------------------------------------------------*/

#ifndef _INCL_OPENGLDRV_H_
#define _INCL_OPENGLDRV_H_

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN 1
#pragma warning(push)
#pragma warning(disable: 4121) /* 'JOBOBJECT_IO_RATE_CONTROL_INFORMATION_NATIVE_V2': alignment of a member was sensitive to packing */
#include <windows.h>
#pragma warning(pop)
#endif

#include "Core.h"

#ifndef WIN32
#include "SDL.h"
#endif

#include "Engine.h"
#include "UnRender.h"

#define GL_GLEXT_LEGACY 1
#ifdef __EMSCRIPTEN__
#include "../../regal/include/GL/Regal.h"
#else
#include "gl.h"
#include "glext.h"
#endif

// Hack so I don't have to move UOpenGLRenderDevice's interface into a header.  --ryan.
//#define AUTO_INITIALIZE_REGISTRANTS_OPENGLDRV UOpenGLRenderDevice::StaticClass();
extern "C" { void autoInitializeRegistrantsOpenGLDrv(void); }
#define AUTO_INITIALIZE_REGISTRANTS_OPENGLDRV autoInitializeRegistrantsOpenGLDrv();

#endif
/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
