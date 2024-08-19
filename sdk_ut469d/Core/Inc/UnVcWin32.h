/*=============================================================================
	UnVcWin32.h: Unreal definitions for Visual C++ SP2 running under Win32.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/*----------------------------------------------------------------------------
	Platform compiler definitions.
----------------------------------------------------------------------------*/

#define __WIN32__				1
#define __INTEL__				1
#define __INTEL_BYTE_ORDER__	1
#define __UNREAL_X86__			1

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
#undef CDECL

// Make sure HANDLE is defined.
#ifndef _WINDOWS_
	#define HANDLE void*
	#define HINSTANCE void*
#endif

#ifdef __UNREAL_X86__
# ifndef USES_SSE_INTRINSICS
#  define USES_SSE_INTRINSICS 1
# endif
# include <xmmintrin.h>
# include <emmintrin.h>
# include <intrin.h>
#endif

// Sizes.
enum {DEFAULT_ALIGNMENT = 16}; // Default boundary to align memory allocations on.
enum {CACHE_LINE_SIZE   = 32}; // Cache line size.

// Function type macros.
#ifdef __STATIC_LINK
#define DLL_IMPORT
#define DLL_EXPORT
#define DLL_EXPORT_CLASS
#else
#define DLL_IMPORT	__declspec(dllimport)	/* Import function from DLL */
#define DLL_EXPORT  __declspec(dllexport)	/* Export function to DLL */
#define DLL_EXPORT_CLASS	__declspec(dllexport)	/* Export class to DLL */
#endif

#define VARARGS     __cdecl					/* Functions with variable arguments */
#define CDECL	    __cdecl					/* Standard C function */
#define STDCALL		__stdcall				/* Standard calling convention */
#define FASTCALL	__fastcall
#define FORCEINLINE __forceinline			/* Force code to be inline */
#define ZEROARRAY                           /* Zero-length arrays in structs */

// Variable arguments.

#include <stdarg.h>
#include <stdio.h>

#pragma warning(disable:4996)
#define GET_VARARGS(msg,len,fmt)	\
{	\
	va_list ArgPtr;	\
	va_start( ArgPtr, fmt );	\
	(void) _vsnwprintf(msg, len, fmt, ArgPtr); \
	va_end( ArgPtr );	\
}
#define GET_VARARGS_RESULT(msg,len,fmt,result)	\
{	\
	va_list ArgPtr;	\
	va_start( ArgPtr, fmt );	\
	result = _vsnwprintf(msg, len, fmt, ArgPtr); \
	va_end( ArgPtr );	\
}

// New VARARGS system:
// va_start >> GET_VARARGS_LENGTH >> allocate length+1 sized buffer >> PRINT_VARARGS_LENGTH >> va_end
#define GET_VARARGS_LENGTH(length,fmt,arglist)	\
{	\
	va_list ArgListCopy;	\
	va_copy(ArgListCopy,arglist);	\
	length = _vscwprintf(fmt,ArgListCopy);	\
	va_end(ArgListCopy);	\
}
#define PRINT_VARARGS_LENGTH(buffer,length,fmt,arglist)	_vsnwprintf((TCHAR*)buffer,length+1,fmt,arglist)

// Compiler name.
#ifdef _DEBUG
	#define COMPILER "Compiled with Visual C++ Debug"
#else
	#define COMPILER "Compiled with Visual C++"
#endif

// Unsigned base types.
typedef unsigned char		BYTE;		// 8-bit  unsigned.
typedef unsigned short		_WORD;		// 16-bit unsigned.
typedef unsigned long		DWORD;		// 32-bit unsigned.
typedef unsigned __int64	QWORD;		// 64-bit unsigned.

// Signed base types.
typedef	signed char			SBYTE;		// 8-bit  signed.
typedef signed short		SWORD;		// 16-bit signed.
typedef signed int  		INT;		// 32-bit signed.
typedef signed __int64		SQWORD;		// 64-bit signed.

// Character types.
typedef char				ANSICHAR;	// An ANSI character.
typedef unsigned short      UNICHAR;	// A unicode character.
typedef unsigned char		ANSICHARU;	// An ANSI character.
typedef unsigned short      UNICHARU;	// A unicode character.

// Other base types.
typedef signed int			UBOOL;		// Boolean 0 (false) or 1 (true).
typedef float				FLOAT;		// 32-bit IEEE floating point.
typedef double				DOUBLE;		// 64-bit IEEE double.
typedef ULONG_PTR			SIZE_T;     // Corresponds to C SIZE_T.
typedef LONG_PTR			SSIZE_T;

// Bitfield type.
// stijn: WARNING: the property offset calculation in UnClass and UnProp assumes that bitfields are 32-bit integers.
typedef unsigned long       BITFIELD;	// For bitfields.

#ifdef _WIN64
typedef unsigned __int64 PTRINT;
#else
typedef unsigned long PTRINT;
#endif

#define NULL_PTR nullptr

// Unwanted VC++ level 4 warnings to disable.
#pragma warning(disable : 4091) /* 'typedef ': ignored on left of 'tagGPFIDL_FLAGS' when no variable is declared						*/
#pragma warning(disable : 4297) /* 'UObject::operator delete': function assumed not to throw an exception but does						*/
#pragma warning(disable : 4275) /* non dll-interface class used as base for dll-interface class											*/
#pragma warning(disable : 4456) /* declaration of 's' hides previous local declaration													*/
#pragma warning(disable : 4457) /* declaration of 'Ar' hides function parameter															*/
#pragma warning(disable : 4459) /* declaration of 'boxlist' hides global declaration													*/
#pragma warning(disable : 4200) /* Zero-length array item at end of structure, a VC-specific extension									*/
#pragma warning(disable : 4127) /* Conditional expression is constant																	*/
#pragma warning(disable : 4251) /* needs to have dll-interface to be used by clients of class 'ULinker'									*/
#pragma warning(disable : 4291) /* no matching operator delete found; memory will not be freed if initialization throws an exception	*/
#pragma warning(disable : 4244) /* conversion to float, possible loss of data															*/
#pragma warning(disable : 4100) /* unreferenced formal parameter																		*/
#pragma warning(disable : 4731) /* frame pointer register 'ebp' modified by inline assembly code										*/
#pragma warning(disable : 4458) /* declaration of 'Data' hides class member																*/
#pragma warning(disable : 4595) /* 'operator new': non-member operator new or delete functions may not be declared inline				*/
#pragma warning(disable : 4324) /* 'AZoneInfo': structure was padded due to alignment specifier (compiling source file UnPhysic.cpp)	*/
#pragma warning(disable : 4201) /* nonstandard extension used: nameless struct/union													*/

// If C++ exception handling is disabled, force guarding to be off.
#ifndef _CPPUNWIND
	#error "Bad VCC option: C++ exception handling must be enabled"
#endif

// Make sure characters are unsigned.
#ifdef _CHAR_UNSIGNED
	#error "Bad VC++ option: Characters must be signed"
#endif

// No asm if not compiling for x86.
#if !defined(__UNREAL_X86__) || BUILD_64
	#undef ASM
	#define ASM 0
#endif

// Strings.
#define LINE_TERMINATOR TEXT("\r\n")
#define LINE_TERMINATOR_ANSI "\r\n"
#define PATH_SEPARATOR TEXT("\\")

// DLL file extension.
#define DLLEXT TEXT(".dll")

// Pathnames.
#define PATH(s) s

// NULL.
#define NULL 0

// Package implementation.
#if !__STATIC_LINK
#define IMPLEMENT_PACKAGE_PLATFORM(pkgname) \
	extern "C" {HINSTANCE hInstance;} \
	INT DLL_EXPORT STDCALL DllMain( HINSTANCE hInInstance, DWORD Reason, void* Reserved ) \
	{ hInstance = hInInstance; return 1; }
#else
#define IMPLEMENT_PACKAGE_PLATFORM(pkgname)
#endif

// Platform support options.
#define PLATFORM_NEEDS_ARRAY_NEW	1
#define FORCE_UTF8_LOG				1

// OS unicode function calling.
CORE_API ANSICHAR* winToANSI( ANSICHAR* ACh, const TCHAR* InUCh, INT Count );
CORE_API INT winGetSizeANSI( const TCHAR* InUCh );
CORE_API TCHAR* winToUNICODE( TCHAR* Ch, const ANSICHAR* InUCh, INT Count );
CORE_API INT winGetSizeUNICODE( const ANSICHAR* InACh );
#define TCHAR_CALL_OS(funcW,funcA) (GUnicodeOS ? (funcW) : (funcA))

// stijn: old string conversion macros. These are unsafe to use because they can easily overflow the stack
#define TCHAR_TO_ANSI(str) winToANSI((ANSICHAR*)appAlloca(winGetSizeANSI(str)),str,winGetSizeANSI(str))
#define TCHAR_TO_OEM(str) winToOEM((ANSICHAR*)appAlloca(winGetSizeANSI(str)),str,winGetSizeANSI(str))
#define ANSI_TO_TCHAR(str) winToUNICODE((TCHAR*)appAlloca(winGetSizeUNICODE(str)*sizeof(TCHAR)),str,winGetSizeUNICODE(str))

// Bitfield alignment.
#define GCC_PACK(n)
#define GCC_REALLY_PACK(n)
#define GCC_ALIGN(n)

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
	extern CORE_API UBOOL GTimestamp;			// stijn: is the TSC usable on this system?
	extern CORE_API UBOOL GTimestampPreferred;	// stijn: is TSC the preferred timing source?
#if WIN32
	extern CORE_API UBOOL GTGTPreferred;		// stijn: prefer timeGetTime? Takes precendence over GTimestampPreferred but is only available through the -TGT command line option
#endif
	extern CORE_API UBOOL GIsInvariantTSC;
}

/*----------------------------------------------------------------------------
	Functions.
----------------------------------------------------------------------------*/

#if __UNREAL_X86__
# include "UnX86.h"
#endif

//
// CPU cycles, related to GSecondsPerCycle.
//
#define DEFINED_appCycles 1
inline DWORD appCycles()
{	
#ifdef __UNREAL_X86__
	return static_cast<DWORD>(__rdtsc());
#else
	return GetTickCount();
#endif
}
inline SQWORD appCyclesLong()
{
#ifdef __UNREAL_X86__
	return __rdtsc();
#else
	return GetTickCount64();
#endif
}
extern CORE_API FLOAT GSecondsPerCycle;
extern CORE_API DOUBLE GSecondsPerCycleLong;


CORE_API FTime appSeconds();
CORE_API DOUBLE appSecondsNew(); // uses RDTSC is GTimestampPreferred is 1. Otherwise QPC
CORE_API DOUBLE appSecondsNewNoCheck(); // always uses QPC

#if __UNREAL_X86__ && ASM
#define DEFINED_appDebugBreak
inline void appDebugBreak()
{
	__asm
	{
		int 3
	}
}
#endif


extern "C" void* __cdecl _alloca(size_t);
#define appAlloca(size) _alloca((size+7)&~7)

/*----------------------------------------------------------------------------
	Timing.
----------------------------------------------------------------------------*/
#define GPlatformTimerSecondsPerIncrement GSecondsPerCycle
#define appPlatformTimer appCyclesLong
#define appPlatformSleep appSleepLong
#define TIMESTAMPTYPE QWORD
#define DELTATIMETYPE SQWORD

/*----------------------------------------------------------------------------
	The End.
----------------------------------------------------------------------------*/
