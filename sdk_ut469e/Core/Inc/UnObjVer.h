/*=============================================================================
	UnObjVer.h: Unreal object version.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/*-----------------------------------------------------------------------------
	Version coding.
-----------------------------------------------------------------------------*/

// Earliest engine build that is network compatible with this one.
#define ENGINE_MIN_NET_VERSION 432
// the minimum server version to send MD5 checksums to (DON'T CHANGE)
#define ENGINE_MIN_MD5_VERSION 455

// The server must request the MD5 checksum as of this version.
#define ENGINE_MIN_MD5_REQUEST_VERSION 469

#define MD5_MAGIC TEXT("http://www.epicgames.com")

//elmuerte: the minimum version of the server supported by the client
#define ENGINE_MIN_NET_SERVER_VERSION 432

// Engine build number, for displaying to end users.
// Epic versions: 400-436, UTPG versions: 440-468, OldUnreal versions: 469-...
#define ENGINE_VERSION 469

// Engine revision number
#define ENGINE_REVISION TEXT("e")

// Set to 1 for snapshots, 0 for releases
#define ENGINE_BUILD_SNAPSHOT 0

// Base protocol version to negotiate in network play.
#define ENGINE_NEGOTIATION_VERSION 400

// Prevents incorrect files from being loaded.
#define PACKAGE_FILE_TAG 0x9E2A83C1

// The current Unrealfile version.
#define PACKAGE_FILE_VERSION 69

#ifndef ENGINE_ARCHITECTURE
# if WIN32
#  if BUILD_64
#   define ENGINE_ARCHITECTURE "amd64"
#  else
#   define ENGINE_ARCHITECTURE "x86"
#  endif
# else
#  error "You need to define an ENGINE_ARCHITECTURE"
# endif
#endif

//
// Licensee Version Number
// by Paul Du Bois, Infinite Machine
//
// Uses the upper two words of the VersionNum DWORD to encode a licensee-specific
// version WORD.
//
#define PACKAGE_FILE_VERSION_LICENSEE	0x00

#define VERSION_ENGINE_ONLY				0x00

// The earliest file version which we can load with complete
// backwards compatibility. Must be at least PACKAGE_FILE_VERSION.
#define PACKAGE_MIN_VERSION 60

// Defines for easier building of multi game projects. --han
#define UNREAL_TOURNAMENT 1
#define UNREAL_TOURNAMENT_UTPG 1
#define UNREAL_TOURNAMENT_OLDUNREAL 1
#define SYSTEM_INI		TEXT("UnrealTournament")
#define APP_NAME		TEXT("UnrealTournament")
#define GAME_EXECUTABLE	TEXT("UnrealTournament.exe")
#define RUNNING_MUTEX(Path)	*(FString(TEXT("UnrealIsRunning-")) + appMD5String(FString(Path).Caps()))

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
