/*=============================================================================
	UnBuild.h: Unreal build settings.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
=============================================================================*/
#ifndef __UNBUILD_H__
#define __UNBUILD_H__
#ifndef _REALLY_WANT_DEBUG
#define _REALLY_WANT_DEBUG
#endif
#if defined(_DEBUG) && !defined(_REALLY_WANT_DEBUG)
#error "Your active configuration is set to DEBUG.  Click here for important information!"
//
// ** IMPORTANT INFORMATION **
//
// When running in DEBUG mode, all errors intentionally trigger a breakpoint. 
// Therefore, DEBUG  builds are only suitable for running from within the debugger.  
// If you run DEBUG builds outside the debugger, you will get a GPF when an error 
// occurs instead of a useful error message.  If this is what you want, 
// define _REALLY_WANT_DEBUG and recompile.
//
// This message is here because Visual C++ sets itself to DEBUG mode when you 
// first load Unreal.dsw, which is probably not what you want.
//
// Always do a full recompile of 100% of the C++ code (all DLL's and EXE's) 
// when switching between DEBUG and RELEASE builds.  The two builds generate 
// the same named EXE's and DLL's, but they are not compatible.
// 
#endif

/*-----------------------------------------------------------------------------
	Major build options.
-----------------------------------------------------------------------------*/

// Whether to turn off all checks.
#ifndef DO_CHECK
#define DO_CHECK 1
#endif

// Whether to track call-stack errors.
#ifndef DO_GUARD
# if __EMSCRIPTEN__
#  define DO_GUARD 0
# else
#  define DO_GUARD 1
# endif
#endif

// Whether to track call-stack errors in performance critical routines.
#ifndef DO_GUARD_SLOW
# ifdef _DEBUG
#  define DO_GUARD_SLOW 1
# else
#  define DO_GUARD_SLOW 0
# endif
#endif

// Whether to perform CPU-intensive timing of critical loops.
#ifndef DO_CLOCK_SLOW
#define DO_CLOCK_SLOW 0
#endif

//#define RENDER_BENCHMARK

// Whether to gather performance statistics.
#ifndef STATS
#ifdef RENDER_BENCHMARK
#define STATS 0
#else
#define STATS 1
#endif
#endif

// Whether to use Intel assembler code.
#ifndef ASM
#define ASM 1
#endif

/*-----------------------------------------------------------------------------
	Optional UTPG and OldUnreal features.
-----------------------------------------------------------------------------*/

//elmuerte: enable MD5 checking
#ifndef UTPG_MD5
#define UTPG_MD5 1
#endif

#ifdef WIN32
	//elmuerte: winxp sp2 firewall hack (backported from UT2004)
	#ifndef UTPG_WINXP_FIREWALL
	#define UTPG_WINXP_FIREWALL 1
	#endif
#endif

// stijn: Set to 1 to enable Higor's XC_Engine relevancy loop
// This improves server performance and saves bandwidth
#define OLDUNREAL_RELEVANCY_LOOP 1

// stijn: records demos in v436 format
#define OLDUNREAL_DEMO_COMPAT 1

// stijn: set to 1 to enable UScript function hooking/redirection support
#define OLDUNREAL_USCRIPT_HOOKING 1

// Higor: experimental property list cache system
#define OLDUNREAL_PROPERTY_TABLES 1

// stijn: Enables alphablending support code in the renderer
#define OLDUNREAL_ALPHABLEND 1

// stijn: Allows you to recompile the code for myleveled actors
#define OLDUNREAL_MYLEVEL_ACTOR_RECOMPILATION_SUPPORT 1

//
// stijn: We messed up and added 3 of Botpack.Kicker's _existing_
// variables to the replication block in the 469a patch. As a result,
// UT>=469a and UT<=468 disagree on how many replicatable fields
// Botpack.Kicker has. Normally, this is not a big deal, but in this
// particular case, the two engine versions will also use a different
// number of bits for the field replication index. This causes serious
// network compatibility problems.
//
// This hack restores network compatibility by not registering the 3
// variables in question as replicatable variables when we're
// communicating with an old peer.
//
#define OLDUNREAL_USE_KICKER_HACK 1

#define GUEDIni TEXT("UnrealEd.ini")

/*-----------------------------------------------------------------------------
	Code generation settings.
-----------------------------------------------------------------------------*/

#if _WIN64 == 1 || __LP64__ || __x86_64__ || __powerpc64__ || __aarch64__ || __ARM_ARCH_8__ || _M_X64
#define BUILD_64 1
#else
#define BUILD_64 0
#endif

// Boundary to align class properties on.
#define INT_ALIGNMENT 4

#if BUILD_64

// stijn: in 64-bit builds, we can always align everything at its natural
// boundaries
# define VECTOR_ALIGNMENT 16
# define OBJECT_ALIGNMENT 16 // needs to be >= VECTOR_ALIGNMENT
# define POINTER_ALIGNMENT 8
# define OLDUNREAL_BINARY_COMPAT 0

#elif defined(_MSC_VER)

// stijn: in the 32-bit Windows builds, we want to retain the 4-byte packing for
// binary compatibility with existing native mods
# define VECTOR_ALIGNMENT 4
# define OBJECT_ALIGNMENT 4
# define POINTER_ALIGNMENT 4
# define OLDUNREAL_BINARY_COMPAT 1

#else

// stijn: for all other targets, we can just align at natural boundaries
# define VECTOR_ALIGNMENT 16
# define OBJECT_ALIGNMENT 16
# define POINTER_ALIGNMENT 4
# define OLDUNREAL_BINARY_COMPAT 0

#endif

#endif // __UNBUILD_H__

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
