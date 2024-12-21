/*=============================================================================
	SDL2DrvNative.h: Native function lookup table for static libraries.
	Copyright 2000 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Daniel Vogel (based on XDrvNative.h)
		* Updated to SDL2 with some improvements by Smirftsch www.oldunreal.com

=============================================================================*/

#ifndef SDL2DRVNATIVE_H
#define SDL2DRVNATIVE_H

#if __STATIC_LINK

/* No native execs. */

#define AUTO_INITIALIZE_REGISTRANTS_SDL2DRV \
	USDL2Client::StaticClass(); \
	USDL2Viewport::StaticClass();

#endif

#endif
