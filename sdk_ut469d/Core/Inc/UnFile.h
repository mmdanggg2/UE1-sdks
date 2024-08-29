/*=============================================================================
	UnFile.h: General-purpose file utilities.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include <math.h>

/*-----------------------------------------------------------------------------
	Global variables.
-----------------------------------------------------------------------------*/

// Global variables.
CORE_API extern DWORD GCRCTable[];

/*----------------------------------------------------------------------------
	Byte order conversion.
----------------------------------------------------------------------------*/

// Bitfields.
#ifndef NEXT_BITFIELD
	#if __INTEL_BYTE_ORDER__
		#define NEXT_BITFIELD(b) ((b)<<1)
		#define FIRST_BITFIELD   (1)
	#else
		#define NEXT_BITFIELD(b) ((b)>>1)
		#define FIRST_BITFIELD   (0x80000000)
	#endif
#endif

#if __INTEL_BYTE_ORDER__
	#define INTEL_ORDER16(x)   (x)
	#define INTEL_ORDER32(x)   (x)
	#define INTEL_ORDER64(x)   (x)
#else

    // These macros are not safe to use unless data is UNSIGNED!
	#define INTEL_ORDER16_unsigned(x)   ((((x)>>8)&0xff)+ (((x)<<8)&0xff00))
	#define INTEL_ORDER32_unsigned(x)   (((x)>>24) + (((x)>>8)&0xff00) + (((x)<<8)&0xff0000) + ((x)<<24))

    static inline _WORD INTEL_ORDER16(_WORD val)
    {
        return(INTEL_ORDER16_unsigned(val));
    }

    static inline SWORD INTEL_ORDER16(SWORD val)
    {
        _WORD uval = *((_WORD *) &val);
        uval = INTEL_ORDER16(uval);
        return( *((SWORD *) &uval) );
    }

    static inline DWORD INTEL_ORDER32(DWORD val)
    {
        return(INTEL_ORDER32_unsigned(val));
    }

    static inline INT INTEL_ORDER32(INT val)
    {
        DWORD uval = *((DWORD *) &val);
        uval = INTEL_ORDER32(uval);
        return( *((INT *) &uval) );
    }

	static inline QWORD INTEL_ORDER64(QWORD x)
	{
		/* Separate into high and low 32-bit values and swap them */
		DWORD l = (DWORD) (x & 0xFFFFFFFF);
		DWORD h = (DWORD) ((x >> 32) & 0xFFFFFFFF);
	    return( (((QWORD) (INTEL_ORDER32(l))) << 32) |
		         ((QWORD) (INTEL_ORDER32(h))) );
	}
#endif

/*-----------------------------------------------------------------------------
	Stats.
-----------------------------------------------------------------------------*/

#if STATS
	#define STAT(x) x
#else
	#define STAT(x)
#endif

/*-----------------------------------------------------------------------------
	Global init and exit.
-----------------------------------------------------------------------------*/

CORE_API void appInit( const TCHAR* InPackage, const TCHAR* InCmdLine, FMalloc* InMalloc, FOutputDevice* InLog, FOutputDeviceError* InError, FFeedbackContext* InWarn, FFileManager* InFileManager, FConfigCache*(*ConfigFactory)(), UBOOL RequireConfig );
CORE_API FString appPlatformBuildCmdLine(INT StartArgNum, void* OrigArgv = nullptr, INT OrigArgc = 0, FString* FirstToken = nullptr);
CORE_API void appPlatformPreInit();
CORE_API void appPreExit();
CORE_API void appExit();

/*-----------------------------------------------------------------------------
	Logging and critical errors.
-----------------------------------------------------------------------------*/

CORE_API void appRequestExit( UBOOL Force );

CORE_API void VARARGS appFailAssert( const ANSICHAR* Expr, const ANSICHAR* File, INT Line );
CORE_API void VARARGS appUnwindf( const TCHAR* Fmt, ... );
CORE_API INT appGetSystemErrorCode();
CORE_API const TCHAR* appGetSystemErrorMessage( INT Error=0 );
CORE_API const void appDebugMessagef( const TCHAR* Fmt, ... );
CORE_API const void appMsgf( const TCHAR* Fmt, ... );
CORE_API const void appGetLastError( void );

#define debugf				GLog->Logf
#define appErrorf			GError->Logf
#define warnf				GWarn->Logf

#if DO_GUARD_SLOW
	#define debugfSlow		GLog->Logf
	#define appErrorfSlow	GError->Logf
#else
	#define debugfSlow		GNull->Logf
	#define appErrorfSlow	GNull->Logf
#endif

/*-----------------------------------------------------------------------------
	Misc.
-----------------------------------------------------------------------------*/

CORE_API void* appGetDllHandle( const TCHAR* DllName );
CORE_API void appFreeDllHandle( void* DllHandle );
CORE_API void* appGetDllExport( void* DllHandle, const TCHAR* ExportName );
CORE_API void appLaunchURL( const TCHAR* URL, const TCHAR* Parms=NULL, FString* Error=NULL );
CORE_API void* appCreateProc( const TCHAR* URL, const TCHAR* Parms , UBOOL bRealTime );
CORE_API UBOOL appGetProcReturnCode( void* ProcHandle, INT* ReturnCode );
CORE_API void appEnableFastMath( UBOOL Enable );
CORE_API class FGuid appCreateGuid();
CORE_API void appCreateTempFilename( const TCHAR* Path, TCHAR* Result256 );
CORE_API void appCleanFileCache();
CORE_API UBOOL appFindPackageFile( const TCHAR* In, const FGuid* Guid, TCHAR* Out );
CORE_API UBOOL appFindPackageFileExt(const TCHAR* In, const FGuid* Guid, FString& Out, DWORD LoadFlags);

/*-----------------------------------------------------------------------------
	Clipboard.
-----------------------------------------------------------------------------*/

CORE_API void appClipboardCopy( const TCHAR* Str );
CORE_API FString appClipboardPaste();

/*-----------------------------------------------------------------------------
	Guard macros for call stack display.
-----------------------------------------------------------------------------*/

//
// guard/unguardf/unguard macros.
// For showing calling stack when errors occur in major functions.
// Meant to be enabled in release builds.
//
#ifndef _MSC_VER
#define MAX_RYANS_HACKY_GUARD_BLOCKS 8192

class FString;

// stijn: old version. Was not thread-safe.
class UnGuardBlock
{
public:
	static void GetBackTrace(FString& str);
private:
	static INT GuardIndex;
	static const TCHAR* GuardTexts[MAX_RYANS_HACKY_GUARD_BLOCKS];
};

// stijn: new version. thread-safe.
class UnGuardBlockTLSData
{
public:
	INT GuardIndex;
	const TCHAR* GuardTexts[MAX_RYANS_HACKY_GUARD_BLOCKS];
};

class UnGuardBlockTLS
{
public:
	UnGuardBlockTLS(const TCHAR* text)
	{
		if (++Data.GuardIndex < MAX_RYANS_HACKY_GUARD_BLOCKS)
			Data.GuardTexts[Data.GuardIndex] = text;
	}

	~UnGuardBlockTLS(void)
	{
		Data.GuardIndex--;
	}

	static const TCHAR* GetFuncName()
	{
		return (Data.GuardIndex >= 0 && Data.GuardIndex < MAX_RYANS_HACKY_GUARD_BLOCKS) ?
			Data.GuardTexts[Data.GuardIndex] : TEXT("");
	}

	static void GetBackTrace(FString& str);

private:
	static thread_local UnGuardBlockTLSData Data;
};

#endif

#if !DO_GUARD
# define guard(func)			{static const TCHAR __FUNC_NAME__[]=TEXT(#func);
# define unguard				}
# define unguardf(msg)		    }
# define guard_nofunc			{
# define unguardf_nofunc(msg)	}
#elif WIN32
# define guard(func)			{static const TCHAR __FUNC_NAME__[]=TEXT(#func); try{
# define unguard				}catch(TCHAR*Err){throw Err;}catch(...){appUnwindf(TEXT("%s"),__FUNC_NAME__); throw;}}
# define unguardf(msg)		    }catch(TCHAR*Err){throw Err;}catch(...){appUnwindf(TEXT("%s"),__FUNC_NAME__); appUnwindf msg; throw;}}
# define guard_nofunc			{try{
# define unguardf_nofunc(msg)	}catch(TCHAR*Err){throw Err;}catch(...){appUnwindf msg; throw;}}
#else
// stijn: C++ catch blocks cannot catch segfaults on nix. Ryan's guard blocks 
// are probably the cleanest and most portable way to create a backtrace...
# define guard(func)			{UnGuardBlockTLS __FUNC_NAME__(TEXT(#func)); try{
# define unguard				}catch(TCHAR*Err){throw Err;}catch(...){appUnwindf(TEXT("%s"),UnGuardBlockTLS::GetFuncName()); throw;}}
# define unguardf(msg)		    }catch(TCHAR*Err){throw Err;}catch(...){appUnwindf(TEXT("%s"),UnGuardBlockTLS::GetFuncName()); appUnwindf msg; throw;}}
# define guard_nofunc			{try{
# define unguardf_nofunc(msg)	}catch(TCHAR*Err){throw Err;}catch(...){appUnwindf msg; throw;}}
#endif

//
// guardSlow/unguardfSlow/unguardSlow macros.
// For showing calling stack when errors occur in performance-critical functions.
// Meant to be disabled in release builds.
//
#if defined(_DEBUG) || !DO_GUARD || !DO_GUARD_SLOW
	#define guardSlow(func)		{
	#define unguardfSlow(msg)	}
	#define unguardSlow			}
	#define unguardfSlow(msg)	}
#else
	#define guardSlow(func)		guard(func)
	#define unguardSlow			unguard
	#define unguardfSlow(msg)	unguardf(msg)
#endif

//
// For throwing string-exceptions which safely propagate through guard/unguard.
//
CORE_API void VARARGS appThrowf( const TCHAR* Fmt, ... );

/*-----------------------------------------------------------------------------
	Check macros for assertions.
-----------------------------------------------------------------------------*/

//
// "check" expressions are only evaluated if enabled.
// "verify" expressions are always evaluated, but only cause an error if enabled.
//
#if DO_CHECK
	#define check(expr)  {if(!(expr)) appFailAssert( #expr, __FILE__, __LINE__ );}
	#define verify(expr) {if(!(expr)) appFailAssert( #expr, __FILE__, __LINE__ );}
	#define verifyError(expr) {if(!(expr)) {\
	    INT err = appGetSystemErrorCode();\
		appFailAssert( appToAnsi(*FString::Printf(TEXT("%s failed with error %d: %s"), TEXT(#expr), err, appGetSystemErrorMessage(err))), __FILE__, __LINE__ );\
	}}
#else
	#define check(expr) 0
	#define verify(expr) expr
	#define verifyError(expr) expr
#endif

//
// Check for development only.
//
#if DO_GUARD_SLOW
	#define checkSlow(expr)  {if(!(expr)) appFailAssert( #expr, __FILE__, __LINE__ );}
	#define verifySlow(expr) {if(!(expr)) appFailAssert( #expr, __FILE__, __LINE__ );}
#else
	#define checkSlow(expr)
	#define verifySlow(expr) if(expr){}
#endif

/*-----------------------------------------------------------------------------
	Timing macros.
-----------------------------------------------------------------------------*/

//
// Normal timing.
//

// stijn: someone originally gave this the same name as a standard C89/99/POSIX
// function.  This was a BAD idea and caused a _LOT_ of problems.
#define clockFast(Timer)   {Timer -= appCycles();}
#define unclockFast(Timer) {Timer += appCycles()-34;}

//
// Performance critical timing.
//
#if DO_CLOCK_SLOW
	#define clockSlow(Timer)   {Timer-=appCycles();}
	#define unclockSlow(Timer) {Timer+=appCycles();}
#else
	#define clockSlow(Timer)
	#define unclockSlow(Timer)
#endif

/*-----------------------------------------------------------------------------
	Text format.
-----------------------------------------------------------------------------*/

CORE_API FString appFormat( FString Src, const TMultiMap<FString,FString>& Map );

/*-----------------------------------------------------------------------------
	Localization.
-----------------------------------------------------------------------------*/

class CORE_API FLocalizeFileCache
{
public:
	FLocalizeFileCache();
	~FLocalizeFileCache();

	// Find an actual language file by package name, returns false if failed.
	static UBOOL FindLocaleFiles(TArray<FString>& Result, const TCHAR* PackageName, const TCHAR* LangExt = NULL);

	// Grab list of all language files.
	static void GetAllLocaleFiles(TArray<FString>& Result, const TCHAR* LangExt = NULL);

	// Find list of all language Core files.
	static void FindAllLanguages(TArray<FString>& Result);

	// Check if valid language.
	static UBOOL ValidLanguage(const TCHAR* LangExt);

	// Create new language file if doesn't exist, and return filepath of it.
	static FString CreateLangFile(const TCHAR* PackageName, const TCHAR* LangExt = NULL);

	// Lang path parsing.
	static FString GetLangPathWildcard(const TCHAR* InPath, const TCHAR* LangID); // Turn i.e: ../SystemLocalized/<lang>/*.<lang> -> ../SystemLocalized/int/*.int
	static FString GetLangPath(const TCHAR* InPath, const TCHAR* LangID, const TCHAR* Filename = NULL); // Turn i.e: ../SystemLocalized/<lang>/*.<lang> -> ../SystemLocalized/int/ (OR with Filename specified: ../SystemLocalized/int/Core.int)
};

CORE_API const TCHAR* Localize( const TCHAR* Section, const TCHAR* Key, const TCHAR* Package=GPackage, const TCHAR* LangExt=NULL, UBOOL Optional=0 );
CORE_API const TCHAR* LocalizeWarning(const TCHAR* Key, const TCHAR* Package = GPackage, const TCHAR* LangExt = NULL);
CORE_API const TCHAR* LocalizeError( const TCHAR* Key, const TCHAR* Package=GPackage, const TCHAR* LangExt=NULL );
CORE_API const TCHAR* LocalizeProgress( const TCHAR* Key, const TCHAR* Package=GPackage, const TCHAR* LangExt=NULL );
CORE_API const TCHAR* LocalizeQuery( const TCHAR* Key, const TCHAR* Package=GPackage, const TCHAR* LangExt=NULL );
CORE_API const TCHAR* LocalizeGeneral( const TCHAR* Key, const TCHAR* Package=GPackage, const TCHAR* LangExt=NULL );

CORE_API const TCHAR* Localize( const ANSICHAR* Section, const ANSICHAR* Key, const TCHAR* Package=GPackage, const TCHAR* LangExt=NULL, UBOOL Optional=0 );
CORE_API const TCHAR* LocalizeWarning(const ANSICHAR* Key, const TCHAR* Package = GPackage, const TCHAR* LangExt = NULL);
CORE_API const TCHAR* LocalizeError( const ANSICHAR* Key, const TCHAR* Package=GPackage, const TCHAR* LangExt=NULL );
CORE_API const TCHAR* LocalizeProgress( const ANSICHAR* Key, const TCHAR* Package=GPackage, const TCHAR* LangExt=NULL );
CORE_API const TCHAR* LocalizeQuery( const ANSICHAR* Key, const TCHAR* Package=GPackage, const TCHAR* LangExt=NULL );
CORE_API const TCHAR* LocalizeGeneral( const ANSICHAR* Key, const TCHAR* Package=GPackage, const TCHAR* LangExt=NULL );

/*-----------------------------------------------------------------------------
	File functions.
-----------------------------------------------------------------------------*/

// File utilities.
CORE_API const TCHAR* appFExt( const TCHAR* Filename );
CORE_API FString appFilePathName( const TCHAR* InName );
CORE_API FString appFileBaseName( const TCHAR* InName );
CORE_API UBOOL appUpdateFileModTime( TCHAR* Filename );
CORE_API FString appFilePathForHandle( void* FileHandle ); // stijn: FILE* on nix, HANDLE on Win
CORE_API FString appGetGMTRef();

CORE_API UBOOL appChdirSystem(); // stijn: chdir into the System directory

/*-----------------------------------------------------------------------------
	OS functions.
-----------------------------------------------------------------------------*/

CORE_API const TCHAR* appCmdLine();
CORE_API const TCHAR* appBaseDir();
CORE_API const TCHAR* appPackage();
CORE_API const TCHAR* appComputerName();
CORE_API const TCHAR* appUserName();

/*-----------------------------------------------------------------------------
	Timing functions.
-----------------------------------------------------------------------------*/

//
// ============================================================================
// stijn: IMPORTANT NOTES ON THE SOURCE OF TIMING INFORMATION:
// ============================================================================
//
// UT originally preferred RDTSC as the timing source on any x86 platform.
// This made sense from a performance perspective (because the TSC can be read 
// from user space), but it also caused a _LOT_ of problems because:
//
// 1) The TSC does not always tick at a constant speed (unless the CPU has a 
// constant-rate TSC). Dynamic frequency scaling (e.g., Intel SpeedStep) caused 
// the TSC (and, therefore, the game) to speed up and down.
//
// Current situation: Modern CPUs (Intel P4 and later or AMD Phenom and later) 
// generally have a constant-rate TSC. However, even a constant-rate TSC can 
// slow down, speed up, or pause unexpectedly because of interaction with 
// power-saving features. See the note on invariant TSCs down below.
// 
// 2) The TSC is not always synchronized between CPU cores. This could 
// cause the time to "drift" if the game's main thread got scheduled on a 
// different CPU core than the one it previously ran on. The most visible side 
// effects of an unsychronized TSC are warping and movement glitches.
//
// Current situation: Most modern CPUs and OSes try to keep the TSC synchronized 
// across cores. Intel does this even on multi-socket machines. AMD only does it 
// across cores on the same socket. Once again, some exceptions apply. Linux has 
// some heuristics to guess whether the TSC is synchronized (see 
// https://github.com/torvalds/linux/blob/master/arch/x86/kernel/tsc.c).
// Windows will prefer HPET as the underlying timing source for 
// QueryPerformanceCounter if synchronizing the TSC is not feasible.
//
// 3) The TSC can stop ticking if the CPU is in some power saving mode (e.g., 
// in ACPI P-, C-, and T-states). Most modern mid-range and high-end CPUs have 
// an "invariant TSC", which ticks at a constant rate even in ACPI P/C/T-states. 
// Contrary to regular constant-rate TSCs and  synchronized TSCs, the presence of 
// an invariant TSC is explicitly advertised through the CPUID functionality of 
// the CPU (bit 8 of register EDX of CPUID request 0x80000007 is set to 1 if an 
// invariant TSC is present).
//
// Current situation: Invariant TSCs are somewhat reliable. However, they can 
// still get reset after a CPU or CPU cores wakes up from some sleep state. Once 
// again, you can see the effects this has on the Linux kernel in 
// https://github.com/torvalds/linux/blob/master/arch/x86/kernel/tsc.c.
//
// ============================================================================
// TIMING SOURCES IN UT V469 AND LATER:
// ============================================================================
// 
// Because of the various problems with TSCs, UT v469 will prefer 
// QueryPerformanceCounter as the timing source on Windows, and 
// gettimeofday as the timing source on Linux and mac. Both APIs 
// will select an appropriate timing source based on which features the CPU and 
// OS support.
//
// On most Windows x86 systems, QueryPerformanceCounter will use the TSC as the
// primary timing source. However, QPC also takes corner cases such as waking
// up from a sleep state into account.
//
// On Linux x86 systems, gettimeofday uses whatever timing source has the 
// highest accuracy and precision. Technically, gettimeofday is a system call,
// and therefore incurs quite some overhead. However, any somewhat modern Linux
// system virtualizes gettimeofday through the VDSO to make it as fast as a 
// user-space function call.
//
// There is one corner case where we will use RDTSC directly on Windows: if QPC
// has a very low frequency (e.g., 10Mhz) _AND_ if the CPU supports invariant 
// TSC. 
//
// ============================================================================
// TIMING FUNCTIONS AND VARIABLES IN UT V469 AND LATER:
// ============================================================================
//
// UT still heavily relies on the assumption that appCycles is a low-cost 
// function. On x86 systems, we therefore still use RDTSC as the timing source
// for appCycles, even if the TSC is not stable.
// 
// We _STRONGLY_ discourage you from using appCycles/appCyclesLong as a 
// reliable timing source. If you absolutely need an ultra low-cost timing
// function, however, you can convert the return value of 
// appCycles/appCyclesLong to a timestamp by multiplying it with 
// GSecondsPerCycle/GSecondsPerCycleLong. 
//
// If you need a reliable timing source, then please use appSeconds or 
// appSecondsNew. The latter function has higher precision, but it is not 
// available in older versions of UT.
//

#if !DEFINED_appCycles
CORE_API DWORD appCycles();
CORE_API QWORD appCyclesLong();
#endif

CORE_API DWORD appGetTime( const TCHAR* Filename );

// stijn: returns true if both files exist and B was modified after A's last modification
CORE_API UBOOL appFileIsNewer(const TCHAR* FilenameA, const TCHAR* FilenameB);

// stijn: IMPORTANT NOTE ON STORAGE OF TIMING INFORMATION:
//
// UEngine originally used single-precision floats to store ALL timing 
// information. This was a _BAD_ design choice (though, understandable because 
// of the PSX2). All floating point numbers inevitably lose precision
// as they get bigger, but this happens so much more quickly for single-precision 
// floats that you will actually notice that the game glitches if your system has
// over 1 day of uptime.
//
// In UT v469 and beyond, we use double-precision floats for absolute time values
// and single-precision floats for time differences (aka deltatimes).
//
// For more background info, see:
// https://medium.com/@sidneyriffic/floating-point-numbers-arithmetic-85e543828d38
// https://randomascii.wordpress.com/2012/02/13/dont-store-that-in-a-float/
// 
// 
#if !DEFINED_appSeconds
CORE_API FTime appSeconds();
CORE_API DOUBLE appSecondsNew();
CORE_API DOUBLE appSecondsNewNoCheck();
CORE_API DOUBLE appSecondsCompat();
#endif

CORE_API void appSystemTime( INT& Year, INT& Month, INT& DayOfWeek, INT& Day, INT& Hour, INT& Min, INT& Sec, INT& MSec );
CORE_API const TCHAR* appTimestamp();
CORE_API FString appTimestampSafe();
CORE_API FTime appSecondsSlow();
CORE_API void appSleep( FLOAT Seconds );
CORE_API void appSleepLong (DOUBLE Seconds);

/*-----------------------------------------------------------------------------
	Integer based time operations.
-----------------------------------------------------------------------------*/

class FTimestamp;
class FDeltaTime;

// Represents a timestamp.
class FTimestamp
{
	friend class FDeltaTime;

public:
	FTimestamp()
		: Timestamp(0)
	{}

	static FTimestamp Current()
	{
		// Call into platform specific timer.
		return appPlatformTimer();
	}

	FTimestamp RelativeWait(FDeltaTime Delta);

	UBOOL operator==(const FTimestamp& T) const { return Timestamp == T.Timestamp; }
	UBOOL operator!=(const FTimestamp& T) const { return Timestamp != T.Timestamp; }

	//
	// The ordering operators relies on the assumption that the internal storage
	// should never overflow during program runtime (which might not be true).
	//
	UBOOL operator> (const FTimestamp& T) const { return Timestamp > T.Timestamp; }
	UBOOL operator< (const FTimestamp& T) const { return Timestamp < T.Timestamp; }
	UBOOL operator>=(const FTimestamp& T) const { return Timestamp >= T.Timestamp; }
	UBOOL operator<=(const FTimestamp& T) const { return Timestamp <= T.Timestamp; }

	FDeltaTime operator-(const FTimestamp& T) const;

private:
	FTimestamp(TIMESTAMPTYPE InTimestamp)
		: Timestamp(InTimestamp)
	{}
	TIMESTAMPTYPE Timestamp;
};

// Represents the difference between two timestamps.
class FDeltaTime
{
	FDeltaTime()
		: DeltaTime(0)
	{}

	friend class FTimeStamp;
	friend FDeltaTime FTimestamp::operator-(const FTimestamp& T) const;

public:
	enum ETimeType
	{
		TIME_NanoSeconds,
		TIME_MicroSeconds,
		TIME_MilliSeconds,
		TIME_Seconds,
		TIME_Minutes,
		TIME_Hours,
		TIME_Days
	};

	FDeltaTime(ETimeType Type, DOUBLE Delta)
	{
		switch (Type)
		{
		case TIME_Days:         DeltaTime = Delta / (GPlatformTimerSecondsPerIncrement * (1.0 / 60.0 / 60.0 / 24.0)); break;
		case TIME_Hours:        DeltaTime = Delta / (GPlatformTimerSecondsPerIncrement * (1.0 / 60.0 / 60.0)); break;
		case TIME_Minutes:      DeltaTime = Delta / (GPlatformTimerSecondsPerIncrement * (1.0 / 60.0)); break;
		case TIME_Seconds:      DeltaTime = Delta / (GPlatformTimerSecondsPerIncrement); break;
		case TIME_MilliSeconds: DeltaTime = Delta / (GPlatformTimerSecondsPerIncrement * 1000.0); break;
		case TIME_MicroSeconds: DeltaTime = Delta / (GPlatformTimerSecondsPerIncrement * 1000.0 * 1000.0); break;
		case TIME_NanoSeconds:  DeltaTime = Delta / (GPlatformTimerSecondsPerIncrement * 1000.0 * 1000.0 * 1000.0); break;

		default:
			//appErrorf( TEXT("Invalid ETimeDifference (Type=%i)"), Type );
			break;
		}
	}

	DOUBLE Days() { return DeltaTime * GPlatformTimerSecondsPerIncrement * (1.0 / 60.0 / 60.0 / 24.0); }
	DOUBLE Hours() { return DeltaTime * GPlatformTimerSecondsPerIncrement * (1.0 / 60.0 / 60.0); }
	DOUBLE Minutes() { return DeltaTime * GPlatformTimerSecondsPerIncrement * (1.0 / 60.0); }
	DOUBLE Seconds() { return DeltaTime * GPlatformTimerSecondsPerIncrement; }
	DOUBLE MilliSeconds() { return DeltaTime * GPlatformTimerSecondsPerIncrement * 1000.0; }
	DOUBLE MicroSeconds() { return DeltaTime * GPlatformTimerSecondsPerIncrement * 1000.0 * 1000.0; }
	DOUBLE NanoSeconds() { return DeltaTime * GPlatformTimerSecondsPerIncrement * 1000.0 * 1000.0 * 1000.0; } // 1 ns is about the resolution of the RDTSC for me.

private:
	FDeltaTime(DELTATIMETYPE InDeltaTime)
		: DeltaTime(InDeltaTime)
	{}

	DELTATIMETYPE DeltaTime;
};

inline FDeltaTime FTimestamp::operator-(const FTimestamp& T) const
{
	return FDeltaTime(DELTATIMETYPE(Timestamp) - DELTATIMETYPE(T.Timestamp));
}

inline FTimestamp FTimestamp::RelativeWait(FDeltaTime Delta)
{
	DOUBLE Seconds = Delta.Seconds();

	appPlatformSleep(Seconds);

	// Spin until we hit our wait target.
	for (FTimestamp Current = *this; ; Current = FTimestamp::Current())
		if ((Current - *this).Seconds() >= Seconds)
			return Current;
}

/*-----------------------------------------------------------------------------
	Character type functions.
-----------------------------------------------------------------------------*/

inline TCHAR appToUpper( TCHAR c )
{
	return (c<'a' || c>'z') ? (c) : (TCHAR)(c+'A'-'a');
}
inline TCHAR appToLower( TCHAR c )
{
	return (c<'A' || c>'Z') ? (c) : (TCHAR)(c+'a'-'A');
}
inline UBOOL appIsAlpha( TCHAR c )
{
	return (c>='a' && c<='z') || (c>='A' && c<='Z');
}
inline UBOOL appIsDigit( TCHAR c )
{
	return c>='0' && c<='9';
}
inline UBOOL appIsHexDigit(TCHAR c)
{
	return (c >= '0' && c <= '9') || (c>='a' && c<='f') || (c >= 'A' && c <= 'F');
}
inline UBOOL appIsAlnum( TCHAR c )
{
	return (c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9');
}

/*-----------------------------------------------------------------------------
	String functions.
-----------------------------------------------------------------------------*/

//
// stijn: as of v469, appToAnsi and appFromAnsi have no restrictions on the
// sizes of the strings they can output. appToUnicode and appFromUnicode _ARE_
// restricted to strings of 1024 characters. Source strings that are over 1024
// characters long will be truncated.
// 
CORE_API const ANSICHAR* appToAnsi( const TCHAR* Str );
CORE_API const UNICHAR* appToUnicode( const TCHAR* Str );
CORE_API const TCHAR* appFromAnsi( const ANSICHAR* Str );
CORE_API const TCHAR* appFromUnicode( const UNICHAR* Str );

//
// stijn: in-place conversion functions. Use these functions if you want to avoid
// reallocating/moving long strings after conversion. @MaxCount is always the size
// of the destination buffer in number of characters. Callers need to ensure that
// @MaxCount is sufficient to contain a NUL-terminator!
// 
// The size of the destination buffer in bytes is, therefore, always MaxCount *
// sizeof(type of output character).
//
// Note that the output string will always be null-terminated. These functions will
// write a truncated (and converted) input string into the destination buffer if the
// destination buffer is too small to fit the entire converted input string.
//

// Converts a wide-character input string (C++ wchar) to an ANSI (single character encoding) string
// Note that this function uses the system's ANSI code page to perform the conversion
CORE_API size_t appToAnsiInPlace(ANSICHAR* Dst, const TCHAR* Src, size_t MaxCount, const TCHAR** NextChar=nullptr);
// Converts an ANSI (single character encoding) input string to a wide-character string (C++ wchar)
// This also uses the system's ANSI code page to perform the conversion
CORE_API size_t appFromAnsiInPlace(TCHAR* Dst, const ANSICHAR* Src, size_t MaxCount);

// Converts C++ wchar to UTF-16LE
CORE_API size_t appToUnicodeInPlace(UNICHAR* Dst, const TCHAR* Src, size_t MaxCount, const TCHAR** NextChar=nullptr);
// Converts UTF-16LE to C++ wchar
CORE_API size_t appFromUnicodeInPlace(TCHAR* Dst, const UNICHAR* Src, size_t MaxCount);

// Converts C++ wchar to UTF-8
CORE_API size_t appToUtf8InPlace(ANSICHAR* Dst, const TCHAR* Src, size_t MaxCount, const TCHAR** NextChar=nullptr);
// Converts UTF-8 to C++ wchar
CORE_API size_t appFromUtf8InPlace(TCHAR* Dst, const ANSICHAR* Src, size_t MaxCount);

CORE_API UBOOL appIsPureAnsi( const TCHAR* Str );
CORE_API UBOOL appIsPureAnsiOld( const TCHAR* Str );

CORE_API TCHAR* appStrcpy( TCHAR* Dest, const TCHAR* Src );
CORE_API INT appStrcpy( const TCHAR* String );
CORE_API INT appStrlen( const TCHAR* String );
CORE_API TCHAR* appStrstr( const TCHAR* String, const TCHAR* Find );
CORE_API TCHAR* appStrchr( const TCHAR* String, INT c );
CORE_API TCHAR* appStrcat( TCHAR* Dest, const TCHAR* Src );
CORE_API INT appStrcmp( const TCHAR* String1, const TCHAR* String2 );
CORE_API INT appStricmp( const TCHAR* String1, const TCHAR* String2 );
CORE_API INT appStrncmp( const TCHAR* String1, const TCHAR* String2, INT Count );
CORE_API FString& appStaticFString();
CORE_API TCHAR* appStaticString1024();
CORE_API ANSICHAR* appAnsiStaticString1024();

// stijn: added for v469. Dynamically allocates a string of the requested size
// in a string memory pool. The caller should _NOT_ free the string.
CORE_API ANSICHAR* appAnsiStaticString(size_t Len);
CORE_API TCHAR* appStaticString(size_t Len);

CORE_API const TCHAR* appSpc( int Num );
CORE_API TCHAR* appStrncpy( TCHAR* Dest, const TCHAR* Src, int Max);
CORE_API TCHAR* appStrncat( TCHAR* Dest, const TCHAR* Src, int Max);
CORE_API TCHAR* appStrupr( TCHAR* String );
CORE_API const TCHAR* appStrfind(const TCHAR* Str, const TCHAR* Find);
CORE_API DWORD appStrCrc( const TCHAR* Data );
CORE_API DWORD appStrCrcCaps( const TCHAR* Data );
CORE_API INT appAtoi( const TCHAR* Str );
CORE_API FLOAT appAtof( const TCHAR* Str );
CORE_API INT appStrtoi( const TCHAR* Start, TCHAR** End, INT Base );
CORE_API UBOOL appStrtoUBOOL( const TCHAR* Str );
CORE_API INT appStrnicmp( const TCHAR* A, const TCHAR* B, INT Count );
CORE_API INT appSprintf( TCHAR* Dest, const TCHAR* Fmt, ... ); // stijn: deprecated and dangerous. Use appSnprintf wherever possible!
CORE_API INT appSnprintf(TCHAR* Dest, INT Size, const TCHAR* Fmt, ...);
CORE_API void appTrimSpaces( ANSICHAR* String);
CORE_API TCHAR* appStrdup(const TCHAR* Src);

#if _MSC_VER && !_DEBUG
	CORE_API INT appGetVarArgs( TCHAR* Dest, INT Count, const TCHAR*& Fmt );
#else
	#include <stdio.h>
	#include <stdarg.h>
#endif

typedef int QSORT_RETURN;
typedef QSORT_RETURN(CDECL* QSORT_COMPARE)( const void* A, const void* B );
CORE_API void appQsort( void* Base, INT Num, INT Width, QSORT_COMPARE Compare );

//
// Case insensitive string hash function.
//
inline DWORD appStrihash( const TCHAR* Data )
{
	DWORD Hash=0;
	while( *Data )
	{
		TCHAR Ch = appToUpper(*Data++);
		BYTE  B  = (BYTE)Ch;
		Hash     = ((Hash >> 8) & 0x00FFFFFF) ^ GCRCTable[(Hash ^ B) & 0x000000FF];
		B        = (BYTE)(Ch>>8);
		Hash     = ((Hash >> 8) & 0x00FFFFFF) ^ GCRCTable[(Hash ^ B) & 0x000000FF];
	}
	return Hash;
}

inline DWORD appStrnihash(const TCHAR* Data, INT MaxCount)
{
	DWORD Hash = 0;
	INT i = 0;
	while (*Data && i++ < MaxCount)
	{
		TCHAR Ch = appToUpper(*Data++);
		BYTE  B = (BYTE)Ch;
		Hash = ((Hash >> 8) & 0x00FFFFFF) ^ GCRCTable[(Hash ^ B) & 0x000000FF];
		B = (BYTE)(Ch >> 8);
		Hash = ((Hash >> 8) & 0x00FFFFFF) ^ GCRCTable[(Hash ^ B) & 0x000000FF];
	}
	return Hash;
}

/*-----------------------------------------------------------------------------
	Parsing functions.
-----------------------------------------------------------------------------*/

CORE_API UBOOL ParseCommand( const TCHAR** Stream, const TCHAR* Match );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, class FName& Name );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, DWORD& Value );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, class FGuid& Guid );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, TCHAR* Value, INT MaxLen );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, BYTE& Value );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, SBYTE& Value );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, _WORD& Value );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, SWORD& Value );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, FLOAT& Value );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, INT& Value );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, FString& Value );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, QWORD& Value );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, SQWORD& Value );
CORE_API UBOOL ParseUBOOL( const TCHAR* Stream, const TCHAR* Match, UBOOL& OnOff );
CORE_API UBOOL ParseObject( const TCHAR* Stream, const TCHAR* Match, class UClass* Type, class UObject*& DestRes, class UObject* InParent );
CORE_API UBOOL ParseLine( const TCHAR** Stream, TCHAR* Result, INT MaxLen, UBOOL Exact=0 );
CORE_API UBOOL ParseLine( const TCHAR** Stream, FString& Resultd, UBOOL Exact=0 );
CORE_API UBOOL ParseToken( const TCHAR*& Str, TCHAR* Result, INT MaxLen, UBOOL UseEscape );
CORE_API UBOOL ParseToken( const TCHAR*& Str, FString& Arg, UBOOL UseEscape );
CORE_API FString ParseToken( const TCHAR*& Str, UBOOL UseEscape );
CORE_API void ParseNext( const TCHAR** Stream );
CORE_API UBOOL ParseParam( const TCHAR* Stream, const TCHAR* Param );
CORE_API UBOOL ParseDestructive( FString& Str, const TCHAR* Match, FString& Value );

/*-----------------------------------------------------------------------------
	Math functions.
-----------------------------------------------------------------------------*/

CORE_API DOUBLE appExp( DOUBLE Value );
CORE_API DOUBLE appLoge( DOUBLE Value );
CORE_API DOUBLE appFmod( DOUBLE A, DOUBLE B );
CORE_API DOUBLE appSin( DOUBLE Value );
CORE_API DOUBLE appCos( DOUBLE Value );
CORE_API DOUBLE appAcos( DOUBLE Value );
CORE_API DOUBLE appTan( DOUBLE Value );
CORE_API DOUBLE appAtan( DOUBLE Value );
CORE_API DOUBLE appAtan2( DOUBLE Y, DOUBLE X );
CORE_API DOUBLE appSinDeg(DOUBLE Value);
CORE_API DOUBLE appCosDeg(DOUBLE Value);
CORE_API DOUBLE appAcosDeg(DOUBLE Value);
CORE_API DOUBLE appTanDeg(DOUBLE Value);
CORE_API DOUBLE appAtanDeg(DOUBLE Value);
CORE_API DOUBLE appAtan2Deg(DOUBLE Y, DOUBLE X);
CORE_API DOUBLE appSqrt( DOUBLE Value );
CORE_API DOUBLE appPow( DOUBLE A, DOUBLE B );
CORE_API INT appRand();
CORE_API FLOAT appFrand();
CORE_API FLOAT appRandRange( FLOAT Min, FLOAT Max );
CORE_API INT appRandRange( INT Min, INT Max );

#if !DEFINED_appIsNan
CORE_API UBOOL appIsNan( DOUBLE Value );
#endif

#if !DEFINED_appRound
inline INT appRound(FLOAT Value)
{
	return (INT)floor(Value + 0.5f);
}
inline INT appRound(DOUBLE Value)
{
	return (INT)floor(Value + 0.5);
}
#endif

#if !DEFINED_appFloor
inline INT appFloor(FLOAT Value)
{
	return (INT)floor(Value);
}
inline INT appFloor(DOUBLE Value)
{
	return (INT)floor(Value);
}
#endif

#if !DEFINED_appCeil
CORE_API INT appCeil( FLOAT Value );
#endif

/*-----------------------------------------------------------------------------
	Array functions.
-----------------------------------------------------------------------------*/

// Core functions depending on TArray and FString.
CORE_API UBOOL appLoadFileToArray( TArray<BYTE>& Result, const TCHAR* Filename, FFileManager* FileManager=GFileManager );
CORE_API UBOOL appLoadFileToString( FString& Result, const TCHAR* Filename, FFileManager* FileManager=GFileManager );
CORE_API UBOOL appSaveArrayToFile( const TArray<BYTE>& Array, const TCHAR* Filename, FFileManager* FileManager=GFileManager );
CORE_API UBOOL appSaveStringToFile( const FString& String, const TCHAR* Filename, FFileManager* FileManager=GFileManager );

// Extended functions with forced encoding support.
CORE_API UBOOL appLoadFileToStringEx(FString& Result, const TCHAR* Filename, FFileManager* FileManager = GFileManager, FileEncoding Encoding = ENCODING_AUTO);
CORE_API UBOOL appSaveStringToFileEx(const FString& String, const TCHAR* Filename, FFileManager* FileManager = GFileManager, FileEncoding Encoding = ENCODING_AUTO);

/*-----------------------------------------------------------------------------
	Memory functions.
-----------------------------------------------------------------------------*/

CORE_API void* appMemmove( void* Dest, const void* Src, INT Count );
CORE_API INT appMemcmp( const void* Buf1, const void* Buf2, INT Count );
CORE_API UBOOL appMemIsZero( const void* V, int Count );
CORE_API DWORD appMemCrc( const void* Data, INT Length, DWORD CRC=0 );
CORE_API void appMemswap( void* Ptr1, void* Ptr2, DWORD Size );
CORE_API void appMemset( void* Dest, INT C, INT Count );

#ifndef DEFINED_appMemcpy
inline void appMemcpy( void* Dest, const void* Src, INT Count )
{
	memcpy(Dest, Src, Count);
}
#endif

#ifndef DEFINED_appMemzero
inline void appMemzero( void* Dest, INT Count )
{
	memset(Dest, 0, Count);
}
#endif

//
// C style memory allocation stubs.
//
#if defined(NO_APP_MALLOC) || defined(UTGLR_NO_APP_MALLOC) || defined(_DEBUG)
#include <new>
#define appMalloc(size, tag) malloc(size)
#define appFree free
#define appRealloc(data, size, tag) realloc(data, size)
#else
#define appMalloc     GMalloc->Malloc
#define appFree       GMalloc->Free
#define appRealloc    GMalloc->Realloc
#endif

#ifdef _MSC_VER  // turn off "operator new may not be declared inline"
#pragma warning( push )
#pragma warning( disable : 4595 )
#endif

//
// C++ style memory allocation.
//
#if !defined(_DEBUG) && !defined(NO_APP_MALLOC)
__forceinline void* operator new( size_t Size, const TCHAR* Tag )
{
	guardSlow(new);
	return appMalloc( Size, Tag );
	unguardSlow;
}
__forceinline void* operator new( size_t Size )
{
	guardSlow(new);
	return appMalloc( Size, TEXT("new") );
	unguardSlow;
}
__forceinline void operator delete( void* Ptr ) throw()
{
	guardSlow(delete);
	appFree( Ptr );
	unguardSlow;
}
#endif

#if !defined(_DEBUG) && !defined(NO_APP_MALLOC)
# if PLATFORM_NEEDS_ARRAY_NEW
__forceinline void* operator new[]( size_t Size, const TCHAR* Tag )
{
	guardSlow(new);
	return appMalloc( Size, Tag );
	unguardSlow;
}
__forceinline void* operator new[]( size_t Size )
{
	guardSlow(new);
	return appMalloc( Size, TEXT("new") );
	unguardSlow;
}
__forceinline void operator delete[]( void* Ptr ) throw()
{
	guardSlow(delete);
	appFree( Ptr );
	unguardSlow;
}
# endif
#endif

#ifdef _MSC_VER
#pragma warning( pop )
#endif

/*-----------------------------------------------------------------------------
	Math.
-----------------------------------------------------------------------------*/

CORE_API BYTE appCeilLogTwo( DWORD Arg );

#ifndef DEFINED_appFloorLogTwo
inline BYTE appFloorLogTwo(DWORD Arg)
{
	INT Bit = 32;
	while (--Bit >= 0)
		if (Arg & (1u << DWORD(Bit)))
			return Bit;

	// lg(0) is undefined, so we can't solve.                                                
	return 0;
}
#endif


/*-----------------------------------------------------------------------------
	MD5 functions.
-----------------------------------------------------------------------------*/

//
// MD5 Context.
//
struct FMD5Context
{
	DWORD state[4];
	DWORD count[2];
	BYTE buffer[64];
};

//
// MD5 functions.
//!!it would be cool if these were implemented as subclasses of
// FArchive.
//
CORE_API void appMD5Init( FMD5Context* context );
CORE_API void appMD5Update( FMD5Context* context, BYTE* input, INT inputLen );
CORE_API void appMD5Final( BYTE* digest, FMD5Context* context );
CORE_API void appMD5Transform( DWORD* state, BYTE* block );
CORE_API void appMD5Encode( BYTE* output, DWORD* input, INT len );
CORE_API void appMD5Decode( DWORD* output, BYTE* input, INT len );
CORE_API FString appMD5String( FString InString );

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
