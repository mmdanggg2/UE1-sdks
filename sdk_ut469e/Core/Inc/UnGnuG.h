/*=============================================================================
	UnGnuG.h: Unreal definitions for Gnu G++. Unfinished. Unsupported.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	* Fast appRound, appFloor on GNUC/x86 added by Michael Vance.
=============================================================================*/

/*----------------------------------------------------------------------------
	Platform compiler definitions.
----------------------------------------------------------------------------*/

#ifdef __LINUX_X86__
	#define __UNIX__  1
	#define __LINUX__ 1
	#define __INTEL__ 1
	#define __INTEL_BYTE_ORDER__ 1
	#define __UNREAL_X86__ 1
	#undef ASM
	#define COMPILER "Compiled with Clang (" __VERSION__ ")"
#elif __LINUX_ARM__
    #define __UNIX__ 1
    #define __LINUX__ 1
    #define __INTEL__ 1
    #define __INTEL_BYTE_ORDER__ 1
    #undef ASM
    #define COMPILER "Compiled with GNU g++ (" __VERSION__ ")"
#elif MACOSX
	#define __UNIX__  1
	#define __INTEL__ 1
	#define __INTEL_BYTE_ORDER__ 1
# if !__APPLE_ARM64__
	#define __UNREAL_X86__ 1
# endif
        #undef ASM
	#define COMPILER "Compiled with macOS Clang (" __VERSION__ ")"
    #if __LITTLE_ENDIAN__
    #define WCHAR_T_ENCODING_STRING "UCS-4LE"
    #else
    #define WCHAR_T_ENCODING_STRING "UCS-4BE"
    #endif
#elif __EMSCRIPTEN__
	#define __UNIX__ 1
	#define __LINUX__ 1
	#define __INTEL__ 1
	#define __INTEL_BYTE_ORDER__ 1
	#undef ASM
	#define COMPILER "Compiled with Emscripten (" __VERSION__ ")"
#else
	#error Unsupported platform.
#endif

#ifndef WCHAR_T_ENCODING_STRING
#define WCHAR_T_ENCODING_STRING "WCHAR_T"
#endif

// higher-level parts of UE1 depend on SDL on Unix, but Emscripten uses it
//  for core pieces like timing, too. Eventually, we might flip the switch
//  and use it on all Unix platforms (or...all platforms?!).
#if !MOD_BUILD
# if SDL2BUILD
#  include <SDL2/SDL.h>
# else
#  include <SDL3/SDL.h>
# endif
#endif

#include <string.h>

#include <stdlib.h>  // alloca
#include <cstdarg>
#include <errno.h>

#include <cwchar>
#include <math.h>

/*----------------------------------------------------------------------------
	Platform specifics types and defines.
----------------------------------------------------------------------------*/

// Undo any Windows defines.
#undef BYTE
#undef WORD
#undef DWORD
#undef INT
#undef FLOAT
#undef MAXBYTE
#undef MAXWORD
#undef MAXDWORD
#undef MAXINT
#undef VOID
#undef CDECL

// Make sure HANDLE is defined.
#define HANDLE DWORD
#define HINSTANCE DWORD
#define FALSE 0
#define TRUE 1

// Sizes.
enum {DEFAULT_ALIGNMENT = 16}; // Default boundary to align memory allocations on.
enum {CACHE_LINE_SIZE   = 32}; // Cache line size.

#define GCC_PACK(n) __attribute__((aligned(n)))
#define GCC_REALLY_PACK(n) __attribute__((packed,aligned(n)))
#define GCC_ALIGN(n) __attribute__((aligned(n)))
#define GCC_MOVE_ALIGN(n)

// Function type macros.
#define __forceinline inline __attribute__((always_inline))
#define DLL_IMPORT
#define DLL_EXPORT extern "C"
#define DLL_EXPORT_CLASS
#define VARARGS
#define CDECL
#define STDCALL
#if __LINUX_X86__ && !BUILD_64
# define FASTCALL __attribute__((fastcall)) // stijn: deprecated i386 calling convention
#else
# define FASTCALL
#endif
#define FORCEINLINE __forceinline
#define ZEROARRAY 0 /* Zero-length arrays in structs */
#define __cdecl

// Variable arguments.
extern "C" int appVswprintf(wchar_t* Out, size_t Count, const wchar_t* Fmt, va_list ArgPtr);
#define GET_VARARGS(msg,len,fmt)	\
{	\
	va_list ArgPtr;	\
	va_start( ArgPtr, fmt );	\
	(void) appVswprintf( msg, len, fmt, ArgPtr );	\
	va_end( ArgPtr );	\
}

#define GET_VARARGS_RESULT(msg,len,fmt,result)	\
{	\
	va_list ArgPtr;	\
	va_start( ArgPtr, fmt );	\
	result = appVswprintf( msg, len, fmt, ArgPtr );	\
	va_end( ArgPtr );	\
}

// Unsigned base types.
typedef bool BOOL;
typedef unsigned char		BYTE;		// 8-bit  unsigned.
typedef unsigned short		_WORD;		// 16-bit unsigned.
typedef unsigned int		DWORD;		// 32-bit unsigned.
#if BUILD_64
typedef unsigned long		QWORD;		// 64-bit unsigned.
#else
typedef unsigned long long	QWORD;		// 64-bit unsigned.
#endif

// Signed base types.
typedef	signed char			SBYTE;		// 8-bit  signed.
typedef signed short		SWORD;		// 16-bit signed.
typedef unsigned short		WORD;		// 16-bit signed.
typedef signed int			INT;		// 32-bit signed.
typedef unsigned int		UINT;		// 32-bit unsigned
typedef signed long long	SQWORD;		// 64-bit signed.

// Character types.
typedef char			    ANSICHAR;	// An ANSI character.
typedef unsigned char		ANSICHARU;	// An ANSI character.
typedef unsigned short		UNICHAR;	// A unicode character.
typedef unsigned short		UNICHARU;	// A unicode character.

#ifdef UNICODE
typedef wchar_t TCHAR;
#define _TCHAR_DEFINED 1
#else
typedef char TCHAR;
#define _TCHAR_DEFINED 1
#endif

// Other base types.
typedef signed int		UBOOL;			// Boolean 0 (false) or 1 (true).
typedef float			FLOAT;			// 32-bit IEEE floating point.
typedef double			DOUBLE;         // 64-bit IEEE double.
typedef unsigned long	SIZE_T;         // Corresponds to C SIZE_T.
typedef long long		SSIZE_T;

// !!! FIXME: This is currently true on all supported 64 and 32 bit platforms,
// !!! FIXME:  but it may not be universal. PTRINT should be an integer that
// !!! FIXME:  is the same size as a pointer, so casting can be (ahem) safely
// !!! FIXME:  done between them.  --ryan.
#ifdef BUILD_64
typedef unsigned long PTRINT;
typedef unsigned long INT_PTR; 
#else
typedef DWORD PTRINT;
typedef DWORD INT_PTR;
#endif

typedef int32_t LONG;

// Bitfield type.
typedef unsigned int		BITFIELD;	// For bitfields.

#define NULL_PTR nullptr
#ifndef INFINITE 
#define INFINITE 0xFFFFFFFF
#endif

// !!! FIXME: This should really just include a header.
#if ((!defined MACOSX) && (!defined __x86_64__)) && !defined(__aarch64__)
typedef unsigned int size_t;
#endif

// Make sure characters are unsigned.
#ifdef __CHAR_UNSIGNED__
	#error "Bad compiler option: Characters must be signed"
#endif

// Strings.
#if __UNIX__
#  define LINE_TERMINATOR TEXT("\n")
#  define LINE_TERMINATOR_ANSI "\n"
#  define PATH_SEPARATOR TEXT("/")
#  if MACOSX
#    define DLLEXT TEXT(".dylib")
#  else
#    define DLLEXT TEXT(".so")
#  endif
#else
#  define LINE_TERMINATOR TEXT("\r\n")
#  define LINE_TERMINATOR_ANSI "\r\n"
#  define PATH_SEPARATOR TEXT("\\")
#  define DLLEXT TEXT(".dll")
#endif

// NULL.
#undef NULL
#define NULL 0

// Package implementation.
#define IMPLEMENT_PACKAGE_PLATFORM(pkgname) \
	BYTE GLoaded##pkgname;

// Platform support options.
#define PLATFORM_NEEDS_ARRAY_NEW 1
#define FORCE_ANSI_LOG           0
#define FORCE_UTF8_LOG           1

// OS unicode function calling.
CORE_API ANSICHAR* nixToANSI( ANSICHAR* ACh, const TCHAR* InUCh, INT Count );
CORE_API INT nixGetSizeANSI( const TCHAR* InUCh );
CORE_API TCHAR* nixToUNICODE( TCHAR* Ch, const ANSICHAR* InUCh, INT Count );
CORE_API INT nixGetSizeUNICODE( const ANSICHAR* InACh );
#define TCHAR_CALL_OS(funcW,funcA) (GUnicodeOS ? (funcW) : (funcA))
#define TCHAR_TO_ANSI(str) nixToANSI((ANSICHAR*)appAlloca(nixGetSizeANSI(str)),str,nixGetSizeANSI(str))
#define TCHAR_TO_OEM(str) nixToOEM((ANSICHAR*)appAlloca(nixGetSizeANSI(str)),str,nixGetSizeANSI(str))
#define ANSI_TO_TCHAR(str) nixToUNICODE((TCHAR*)appAlloca(nixGetSizeUNICODE(str)*sizeof(TCHAR)),str,nixGetSizeUNICODE(str))

// !! Fixme: This is a workaround.
#define GCC_OPT_INLINE

// Memory
#define appAlloca(size) alloca((size+7)&~7)

#if __UNREAL_X86__
# include "UnX86.h"
#else
# define USES_SSE_INTRINSICS 0
#endif


#if __ARM_NEON__ || __ARM_FP
#define DEFINED_appRound 1
inline INT appRound(FLOAT F)
{
	return (INT)round(F);
}
inline INT appRound(DOUBLE D)
{
	return (INT)round(D);
}
#endif

//
// Test for not-a-number
//
#define DEFINED_appIsNan 1
inline UBOOL appIsNan(FLOAT F)
{
	return isnan(F);
}
inline UBOOL appIsNan(DOUBLE D)
{
	return isnan(D);
}

//
// CPU cycles, related to GSecondsPerCycle.
//
#define DEFINED_appCycles 1
inline DWORD appCycles()
{
#if __UNREAL_X86__
	return static_cast<DWORD>(__rdtsc());
#elif !MOD_BUILD
	return SDL_GetPerformanceCounter();
#else
	return 0;
#endif
}
inline SQWORD appCyclesLong()
{
#if __UNREAL_X86__
	return __rdtsc();
#elif !MOD_BUILD
	return SDL_GetPerformanceCounter();
#else
	return 0;
#endif
}

//
// Memory zero.
//
#define DEFINED_appMemzero 1
inline void appMemzero( void* Dest, INT Count )
{
	memset( Dest, 0, Count );
}

#ifndef DEFINED_appDebugBreak
void appDebugBreak();
#endif

/*----------------------------------------------------------------------------
	Timing.
----------------------------------------------------------------------------*/
#define GPlatformTimerSecondsPerIncrement 1.0
#define appPlatformTimer appSecondsNew
#define appPlatformSleep appSleepLong
#define TIMESTAMPTYPE DOUBLE
#define DELTATIMETYPE DOUBLE

/*----------------------------------------------------------------------------
	Globals.
----------------------------------------------------------------------------*/

// System identification.
extern "C"
{
	extern HINSTANCE      hInstance;
	extern CORE_API UBOOL GIsMMX;
	extern CORE_API UBOOL GIsSSE;
	extern CORE_API UBOOL GIsSSE2;
	extern CORE_API UBOOL GIsSSE3;
	extern CORE_API UBOOL GIsSSSE3;
	extern CORE_API UBOOL GIsSSE4_1;
	extern CORE_API UBOOL GIsSSE4_2;
	extern CORE_API UBOOL GIsAVX;
	extern CORE_API UBOOL GIsAVX2;
	extern CORE_API UBOOL GIsPentiumPro;
	extern CORE_API UBOOL GIsKatmai;
	extern CORE_API UBOOL GIsK6;
	extern CORE_API UBOOL GIs3DNow;
	extern CORE_API UBOOL GIsAltivec;  // PowerPC vectorizing instruction set.
	extern CORE_API UBOOL GTimestamp; // stijn: is the TSC usable on this system?
	extern CORE_API UBOOL GTimestampPreferred; // stijn: is TSC the preferred timing source?
	extern CORE_API UBOOL GIsInvariantTSC;
}

// Module name
extern TCHAR GModule[32];

/*----------------------------------------------------------------------------
	The End.
----------------------------------------------------------------------------*/
