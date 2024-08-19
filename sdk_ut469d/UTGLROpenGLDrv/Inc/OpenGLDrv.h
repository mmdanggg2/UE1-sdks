/*=============================================================================
	OpenGLDrv.h: Unreal OpenGL support header.
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

#include "Engine.h"
#include "UnRender.h"

#ifdef WIN32
# undef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN 1
#else
# include <SDL2/SDL.h>
#endif

#undef GL_VERSION_1_1
#include "glcorearb.h"
#include "glext.h"

#include <stdlib.h>


/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
