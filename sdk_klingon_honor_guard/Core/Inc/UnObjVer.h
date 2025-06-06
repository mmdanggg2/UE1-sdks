/*=============================================================================
	UnObjVer.h: Unreal object version.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/*-----------------------------------------------------------------------------
	Version coding.
-----------------------------------------------------------------------------*/

// Earliest engine build that is network compatible with this one.
#define ENGINE_MIN_NET_VERSION 219

// Engine build number, for displaying to end users.
#define ENGINE_VERSION 219

// Base protocol version to negotiate in network play.
#define ENGINE_NEGOTIATION_VERSION 219

// Prevents incorrect files from being loaded.
#define PACKAGE_FILE_TAG 0x9E2A83C1

// The current Unrealfile version.
#define PACKAGE_FILE_VERSION 68

// The earliest file version which we can load with complete
// backwards compatibility. Must be at least PACKAGE_FILE_VERSION.
#define PACKAGE_MIN_VERSION 60
// Defines for easier building of multi game projects. --han
#define KLINGON_HONOR_GUARD 1

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
