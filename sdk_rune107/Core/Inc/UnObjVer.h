/*=============================================================================
	UnObjVer.h: Unreal object version.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/*-----------------------------------------------------------------------------
	Version coding.
-----------------------------------------------------------------------------*/

// Unreal engine build number, used to be ENGINE_VERSION, used for legacy unreal code
#define UNREAL_VERSION  405

// Earliest engine build that is network compatible with this one.
#define ENGINE_MIN_NET_VERSION 106

// Engine build number, for displaying to end users.
#define ENGINE_VERSION 107

// Base protocol version to negotiate in network play.
#define ENGINE_NEGOTIATION_VERSION 100

// Prevents incorrect files from being loaded.
#define PACKAGE_FILE_TAG 0x9E2A83C1

// The current Unrealfile version.
#define PACKAGE_FILE_VERSION 69

// The earliest file version which we can load with complete
// backwards compatibility. Must be at least PACKAGE_FILE_VERSION.
#define PACKAGE_MIN_VERSION 60

/*	Revision log

	ver		date		unreal version		desc
	---		--------	--------------		-------------------------------
	100		10-10-00	405b				Gold
	101		10-24-00	405b				First Patch (US)
	102		11-02-00	405b				Foreign Patches / 2nd run of US retail
	103		12-21-00	405b				Unreleased
	104		01-08-01	405b				OEM DX8
	105										Russian/Patch consolidation
	106		03-12-01	405b				Add-on / Patch
	107		04-20-01	405b				Foreign Patch / Additional fixes
*/
// RUNE: Game version, Obsolete (now use ENGINE_VERSION)
#define GAME_VERSION 004

#define RUNE 1

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

