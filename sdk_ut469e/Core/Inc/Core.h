/*=============================================================================
	Core.h: Unreal core public header file.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#ifndef _INC_CORE
#define _INC_CORE

#ifdef __EMSCRIPTEN__
#define _exit(x) exit(x)
#endif

/*----------------------------------------------------------------------------
	Low level includes.
----------------------------------------------------------------------------*/

// API definition.
#ifndef CORE_API
#define CORE_API DLL_IMPORT
#define CORE_API_EXPORT
#define CORE_API_IMPORT CORE_API
#else
#define CORE_API_EXPORT CORE_API
#define CORE_API_IMPORT
#endif

// Build options.
#include "UnBuild.h"
#include <utility>

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack(push,OBJECT_ALIGNMENT)
#endif

// workaround for gcc 2.95.3 issue... --ryan.
#if (__GNUC__ < 3)
#define BUGGYINLINE
#endif

#ifndef BUGGYINLINE
#define BUGGYINLINE inline
#endif

// Time.
#define FIXTIME 4294967296.f
#define FIXTIMELONG 4294967296.0
class FTime
{
#if __GNUG__
#define TIMETYP long long
#else
#define TIMETYP __int64
#endif
public:
			FTime()						{ v = 0; }
			FTime(float f)				{ v = (TIMETYP)(f * FIXTIME); }
			FTime(double d)				{ v = (TIMETYP)(d * FIXTIME); }
	float   GetFloat()					{ return v / FIXTIME; }
	double	GetDouble()					{ return v / FIXTIMELONG; }
	FTime   operator+  (float f) const	{ return FTime(v + (TIMETYP)(f * FIXTIME)); }
	float   operator-  (FTime t) const	{ return (v - t.v) / FIXTIME; }
	FTime   operator*  (float f) const	{ return FTime(v * f); }
	FTime   operator/  (float f) const	{ return FTime(v / f); }
	FTime&	operator+= (float f)		{ v = v + (TIMETYP)(f * FIXTIME); return *this; }
	FTime&	operator*= (float f)		{ v = (TIMETYP)(v * f); return *this; }
	FTime&	operator/= (float f)		{ v = (TIMETYP)(v / f); return *this; }
	int     operator== (FTime t)		{ return v == t.v; }
	int     operator!= (FTime t)		{ return v != t.v; }
	int     operator>  (FTime t)		{ return v > t.v; }
	FTime&	operator=  (const FTime& t) { v = t.v; return *this; }
private:
	FTime(TIMETYP i) { v = i; }
	TIMETYP v;
};

/* stijn: for testing only
class FTime
{
public:
	FTime() = default;
			//FTime()					{ v = 0; }
			FTime(float f)				{ v = f; }
			FTime(double d)				{ v = d; }
	float   GetFloat()					{ return static_cast<float>(v); }
	double	GetDouble()					{ return v; }
	FTime   operator+  (float f) const	{ return FTime(v+f); }
	float   operator-  (FTime t) const	{ return static_cast<float>(v - t.v); }
	FTime   operator*  (float f) const	{ return FTime(v * f); }
	FTime   operator/  (float f) const	{ return FTime(v / f); }
	FTime&	operator+= (float f)		{ v = v + f; return *this; }
	FTime&	operator*= (float f)		{ v = v * f; return *this; }
	FTime&	operator/= (float f)		{ v = v / f; return *this; }
	int     operator== (FTime t)		{ return v == t.v; }
	int     operator!= (FTime t)		{ return v != t.v; }
	int     operator>  (FTime t)		{ return v > t.v; }
	FTime&	operator=  (const FTime& t) { v = t.v; return *this; }
private:
	double v{};
};*/

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (pop)
#endif


// Compiler specific include.
#include <string.h>
#if _MSC_VER
#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
	#include <windows.h>
	#include "UnVcWin32.h"	
#elif __GNUG__	
	#include "UnGnuG.h"
#else
	#error Unknown Compiler
#endif

// Ensure certain flags are defined
#ifndef BUILD_64
# define BUILD_64 0
#endif
#ifndef USES_SSE_INTRINSICS
# define USES_SSE_INTRINSICS 0
#endif

// If no asm, redefine __asm to cause compile-time error.
#if !ASM && !__GNUG__
	#define __asm ERROR_ASM_NOT_ALLOWED
#endif

// OS specific include.
#if __UNIX__
	#include <pthread.h>
	#include "UnUnix.h"
	#include <signal.h>
#endif

#if 1  // !!! FIXME: flip to zero before shipping.
	#define STUBBED(x) do { \
		static bool seen_this = false; \
		if (!seen_this) { \
			seen_this = true; \
			fprintf(stderr, "STUBBED: %s at %s (%s:%d)\n", x, __FUNCTION__, \
			        __FILE__, __LINE__); \
		} \
	} while (0)
#endif

#ifndef STUBBED
	#define STUBBED(x)
#endif

#define FORCE_USCRIPT_PADDING_BYTE3(x) BYTE force_uscript_padding_byte##x
#define FORCE_USCRIPT_PADDING_BYTE2(x) FORCE_USCRIPT_PADDING_BYTE3(x)
#define FORCE_USCRIPT_PADDING_BYTE FORCE_USCRIPT_PADDING_BYTE2(__LINE__)

#if BUILD_64
#define FORCE_64BIT_PADDING_DWORD3(x) DWORD force_64bit_padding_dword##x
#define FORCE_64BIT_PADDING_DWORD2(x) FORCE_64BIT_PADDING_DWORD3(x)
#define FORCE_64BIT_PADDING_DWORD FORCE_64BIT_PADDING_DWORD2(__LINE__)
#define FORCE_64BIT_PADDING_BYTE3(x) BYTE force_64bit_padding_byte##x
#define FORCE_64BIT_PADDING_BYTE2(x) FORCE_64BIT_PADDING_BYTE3(x)
#define FORCE_64BIT_PADDING_BYTE FORCE_64BIT_PADDING_BYTE2(__LINE__)
#else
#define FORCE_64BIT_PADDING_DWORD
#define FORCE_64BIT_PADDING_BYTE 
#endif

// Global constants.
enum {MAXBYTE		= 0xff       };
enum {MAXWORD		= 0xffffU    };
enum {MAXDWORD		= 0xffffffffU};
enum {MAXSBYTE		= 0x7f       };
enum {MAXSWORD		= 0x7fff     };
enum {MAXINT		= 0x7fffffff };
enum {INDEX_NONE	= -1         };
enum {UNICODE_BOM   = 0xfeff     };
#define UTF16LE_BOM "\xff\xfe"
#define UTF8_BOM	"\xef\xbb\xbf"
enum ENoInit {E_NoInit = 0};

#define HACKFLAGS_NoNearZ		0x1
#define HACKFLAGS_PostRender	0x2

// Unicode or single byte character set mappings.

// stijn: UEngine uses up to 3 different string representations, depending on the target platform and build configuration.
// In UNICODE builds, we use TCHAR strings for almost everything. The one notable exception is when we need to serialize
// FStrings before storing them on disk or sending them over the network. In this case, we use ANSICHAR or UNICHAR strings.
//
// ANSI/ANSICHAR strings:
// > 1-byte encoding
// > Windows-125x code pages
// > Used for serialization of pure ansi strings
//
// TCHAR strings:
// > Equivalent to ANSI strings if we build without unicode support. If we build WITH unicode support, then these are wide-character strings.
// The exact representation is platform-specific. Windows uses 2 bytes per char and UTF-16LE encoding.
// Linux usually does 4 bytes per char and UTF-32 encoding.
// > ISO 10646 aka UCS code pages
// > Used for most internal string operations and for storage in RAM
//
// UNICHAR strings:
// > Equivalent to ANSI strings if we build without unicode support. If we build WITH unicode support, these are UTF-16LE strings.
// > ISO 10646 aka UCS code pages
// > Used for serialization of strings that cannot be represented in pure ANSI
//
#ifndef WIN32
	#include <iconv.h>
	CORE_API extern "C" iconv_t IconvAnsiToTchar;
	CORE_API extern "C" iconv_t IconvAnsiToUnichar;
	CORE_API extern "C" iconv_t IconvUnicharToAnsi;
	CORE_API extern "C" iconv_t IconvUnicharToTchar;
	CORE_API extern "C" iconv_t IconvTcharToAnsi;
	CORE_API extern "C" iconv_t IconvTcharToUnichar;
    CORE_API extern "C" iconv_t IconvTcharToUtf8;
    CORE_API extern "C" iconv_t IconvUtf8ToTchar;
#endif

#ifndef _TCHAR_DEFINED
#define _TCHAR_DEFINED 1
typedef UNICHAR  TCHAR;
#endif
typedef UNICHARU TCHARU;
#undef TEXT
#define TEXT(s) L##s
#undef US
#define US FString(L"")

// stijn: only to be used if we _REALLY_ do not want to use iconv for proper conversion
inline TCHAR    RawFromAnsi     (ANSICHAR In) { return (BYTE)In;                        }
inline TCHAR    RawFromUnicode  (UNICHAR In ) { return In;                              }
inline ANSICHAR RawToAnsi       (TCHAR In   ) { return (_WORD)In<0x100 ? In : MAXSBYTE; }
inline UNICHAR  RawToUnicode    (TCHAR In   ) { return In;                              }

// Mirrored in UScript in the Engine.StatLogFile class
enum FileEncoding
{
	ENCODING_ANSI,
	ENCODING_UTF16LE,
	ENCODING_UTF16LE_BOM,
	ENCODING_UTF8,
	ENCODING_UTF8_BOM,
	ENCODING_AUTO
};

// String Conversion
enum StringEncoding
{
	STRENCODING_ANSI,
	STRENCODING_UTF8,
	STRENCODING_WCHAR
};

/*----------------------------------------------------------------------------
	Forward declarations.
----------------------------------------------------------------------------*/

// Objects.
class	UObject;
class		UExporter;
class		UFactory;
class		UField;
class			UConst;
class			UEnum;
class			UProperty;
class				UByteProperty;
class				UIntProperty;
class				UBoolProperty;
class				UFloatProperty;
class				UObjectProperty;
class					UClassProperty;
class				UNameProperty;
class				UStructProperty;
class               UStrProperty;
class               UArrayProperty;
class			UStruct;
class				UFunction;
class				UState;
class					UClass;
class		ULinker;
class			ULinkerLoad;
class			ULinkerSave;
class		UPackage;
class		USubsystem;
class			USystem;
class		UTextBuffer;
class       URenderDevice;
class		UPackageMap;
class       UViewport;
class       UClient;

// Structs.
class FName;
class FArchive;
class FCompactIndex;
class FExec;
class FGuid;
class FMemCache;
class FMemStack;
class FPackageInfo;
class FTransactionBase;
class FUnknown;
class FRepLink;
class FArray;
class FLazyLoader;
class FString;
class FStringRaw;
class FMalloc;

// Threading support
class FSynchronizationFactory;
class FThreadFactory;

// Templates.
template<class T> class TArray;
template<class T> class TTransArray;
template<class T> class TLazyArray;
template<class TK, class TI> class TMap;
template<class TK, class TI> class TMultiMap;

#ifdef UTPG_MD5
class UMD5Table;
class FMD5Record;
#endif

// Globals.
CORE_API extern class FOutputDevice* GNull;

// EName definition.
#include "UnNames.h"

/*-----------------------------------------------------------------------------
	Abstract interfaces.
-----------------------------------------------------------------------------*/

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack(push,OBJECT_ALIGNMENT)
#endif

#undef FORMAT_STRING
#if _MSC_VER >= 1400
# include <sal.h>
# if _MSC_VER > 1400
#  define FORMAT_STRING(p) _Printf_format_string_ p
# else
#  define FORMAT_STRING(p) __format_string p
# endif /* FORMAT_STRING */
#else
# define FORMAT_STRING(p) p
#endif /* _MSC_VER */

// An output device.
class CORE_API FOutputDevice
{
public:
	// FOutputDevice interface.
	virtual void Serialize( const TCHAR* V, EName Event )=0;

	// Simple text printing.
	void Log( const TCHAR* S );
	void Log( enum EName Type, const TCHAR* S );
	void Log( const FString& S );
	void Log( enum EName Type, const FString& S );
	void Logf( FORMAT_STRING(const TCHAR* Fmt), ... );
	void Logf( enum EName Type, FORMAT_STRING(const TCHAR* Fmt), ... );
};

// Error device.
class CORE_API FOutputDeviceError : public FOutputDevice
{
public:
	virtual void HandleError()=0;
};

// Buggie: Convert assert error into exception, for current scope.
class CORE_API FScopeErrorThrowError
{
public:
	FScopeErrorThrowError(DWORD LoadFlags = 0);
	void EndTrack();
	~FScopeErrorThrowError();
private:
	FScopeErrorThrowError(FScopeErrorThrowError* Other) = delete;
	FScopeErrorThrowError& operator=(FScopeErrorThrowError& Other) = delete;
	FOutputDeviceError* OldErr;
};

// Memory allocator.
class CORE_API FMalloc
{
public:
	virtual void* Malloc( size_t Count, const TCHAR* Tag )=0;
	virtual void* Realloc( void* Original, size_t Count, const TCHAR* Tag )=0;
	virtual void Free( void* Original )=0;
	virtual void DumpAllocs()=0;
	virtual void HeapCheck()=0;
	virtual void Init()=0;
	virtual void Exit()=0;
};

// Configuration database cache.
class FConfigCache
{
public:
	virtual UBOOL GetBool( const TCHAR* Section, const TCHAR* Key, UBOOL& Value, const TCHAR* Filename=NULL )=0;
	virtual UBOOL GetInt( const TCHAR* Section, const TCHAR* Key, INT& Value, const TCHAR* Filename=NULL )=0;
	virtual UBOOL GetFloat( const TCHAR* Section, const TCHAR* Key, FLOAT& Value, const TCHAR* Filename=NULL )=0;
	virtual UBOOL GetString( const TCHAR* Section, const TCHAR* Key, TCHAR* Value, INT Size, const TCHAR* Filename=NULL )=0;
	virtual UBOOL GetString( const TCHAR* Section, const TCHAR* Key, class FString& Str, const TCHAR* Filename=NULL )=0;
	virtual const TCHAR* GetStr( const TCHAR* Section, const TCHAR* Key, const TCHAR* Filename=NULL )=0;
	virtual UBOOL GetSection( const TCHAR* Section, TCHAR* Value, INT Size, const TCHAR* Filename=NULL )=0; // stijn: deprecated and dangerous. Use GetSectionSafe where available!
	virtual TMultiMap<FString,FString>* GetSectionPrivate( const TCHAR* Section, UBOOL Force, UBOOL Const, const TCHAR* Filename=NULL )=0;
	virtual void EmptySection( const TCHAR* Section, const TCHAR* Filename=NULL )=0;
	virtual void SetBool( const TCHAR* Section, const TCHAR* Key, UBOOL Value, const TCHAR* Filename=NULL )=0;
	virtual void SetInt( const TCHAR* Section, const TCHAR* Key, INT Value, const TCHAR* Filename=NULL )=0;
	virtual void SetFloat( const TCHAR* Section, const TCHAR* Key, FLOAT Value, const TCHAR* Filename=NULL )=0;
	virtual void SetString( const TCHAR* Section, const TCHAR* Key, const TCHAR* Value, const TCHAR* Filename=NULL )=0;
	virtual void Flush( UBOOL Read, const TCHAR* Filename=NULL )=0;
	virtual void Detach( const TCHAR* Filename )=0;
	virtual void Init( const TCHAR* InSystem, const TCHAR* InUser, UBOOL RequireConfig )=0;
	virtual void Exit()=0;
	virtual void Dump( FOutputDevice& Ar )=0;
	virtual ~FConfigCache() noexcept(false) {};
};

// Any object that is capable of taking commands.
class CORE_API FExec
{
public:
	virtual UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar )=0;
};

// Notification hook.
class CORE_API FNotifyHook
{
public:
	virtual void NotifyDestroy( void* Src ) {}
	virtual void NotifyPreChange( void* Src ) {}
	virtual void NotifyPostChange( void* Src ) {}
	virtual void NotifyExec( void* Src, const TCHAR* Cmd ) {}
};

// Interface for returning a context string.
class FContextSupplier
{
public:
	virtual FString GetContext(UBOOL bShort=FALSE)=0;
};

// A context for displaying modal warning messages.
enum EAnswer
{
	ANSWER_Cancel = -1,
	ANSWER_No = 0,
	ANSWER_Yes = 1,
};
class CORE_API FFeedbackContext : public FOutputDevice
{
public:
	virtual UBOOL YesNof( const TCHAR* Fmt, ... )=0;
	virtual EAnswer YesNoCancelf( const TCHAR* Fmt, ... )=0;
	virtual void BeginSlowTask( const TCHAR* Task, UBOOL StatusWindow, UBOOL Cancelable )=0;
	virtual void EndSlowTask()=0;
	virtual UBOOL VARARGS StatusUpdatef( INT Numerator, INT Denominator, const TCHAR* Fmt, ... )=0;
	virtual void SetContext( FContextSupplier* InSupplier )=0;
	virtual void ResetWarningCount()=0;
	virtual INT GetWarningCount()=0;
	virtual void MarkBeepTime()=0;
	virtual void MaybeBeep()=0;
};

// Class for handling undo/redo transactions among objects.
typedef void( *STRUCT_AR )( FArchive& Ar, void* TPtr );
typedef void( *STRUCT_DTOR )( void* TPtr );
class CORE_API FTransactionBase
{
public:
	virtual ~FTransactionBase() {}
	virtual void SaveObject( UObject* Object )=0;
	virtual void SaveArray( UObject* Object, FArray* Array, INT Index, INT Count, INT Oper, INT ElementSize, STRUCT_AR Serializer, STRUCT_DTOR Destructor )=0;
	virtual void Apply()=0;
};

// File manager.
enum EFileTimes
{
	FILETIME_Create      = 0,
	FILETIME_LastAccess  = 1,
	FILETIME_LastWrite   = 2,
};
enum EFileWrite
{
	FILEWRITE_NoFail            = 0x01,
	FILEWRITE_NoReplaceExisting = 0x02,
	FILEWRITE_EvenIfReadOnly    = 0x04,
	FILEWRITE_Unbuffered        = 0x08,
	FILEWRITE_Append			= 0x10,
	FILEWRITE_AllowRead         = 0x20,
	FILEWRITE_AllowBaseDir      = 0x40,
};
enum EFileRead
{
	FILEREAD_NoFail             = 0x01,
	FILEREAD_NoBaseDir			= 0x02, // Only read this file from the installation directory and not from the base dir
};
class CORE_API FFileManager
{
public:
	virtual FArchive* CreateFileReader( const TCHAR* Filename, DWORD ReadFlags=0, FOutputDevice* Error=GNull )=0;
	virtual FArchive* CreateFileWriter( const TCHAR* Filename, DWORD WriteFlags=0, FOutputDevice* Error=GNull )=0;
	virtual INT FileSize( const TCHAR* Filename )=0;
	virtual UBOOL Delete( const TCHAR* Filename, UBOOL RequireExists=0, UBOOL EvenReadOnly=0 )=0;
	virtual UBOOL Copy( const TCHAR* Dest, const TCHAR* Src, UBOOL Replace=1, UBOOL EvenIfReadOnly=0, UBOOL Attributes=0, void (*Progress)(FLOAT Fraction)=NULL )=0;
	virtual UBOOL Move( const TCHAR* Dest, const TCHAR* Src, UBOOL Replace=1, UBOOL EvenIfReadOnly=0, UBOOL Attributes=0 )=0;
	virtual SQWORD GetGlobalTime( const TCHAR* Filename )=0;
	virtual UBOOL SetGlobalTime( const TCHAR* Filename )=0;
	virtual UBOOL MakeDirectory( const TCHAR* Path, UBOOL Tree=0 )=0;
	virtual UBOOL DeleteDirectory( const TCHAR* Path, UBOOL RequireExists=0, UBOOL Tree=0 )=0;
	virtual TArray<FString> FindFiles( const TCHAR* Filename, UBOOL Files, UBOOL Directories )=0;
	virtual UBOOL SetDefaultDirectory( const TCHAR* Filename )=0;
	virtual FString GetDefaultDirectory()=0;
	virtual void Init( UBOOL UseHomeDir, const TCHAR* HomedirOverride=nullptr ) {}
};

class CORE_API FRTFConverter
{
public:
	static void ConvertTextStream(const TCHAR* Stream, FOutputDevice& Ar);
};

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (pop)
#endif

/*----------------------------------------------------------------------------
	Global variables.
----------------------------------------------------------------------------*/

// Core globals.
CORE_API extern FMemStack				GMem;
CORE_API extern FOutputDevice*			GLog;
CORE_API extern FOutputDevice*			GNull;
CORE_API extern FOutputDevice*		    GThrow;
CORE_API extern FOutputDeviceError*		GError;
CORE_API extern FFeedbackContext*		GWarn;
CORE_API extern FConfigCache*			GConfig;
CORE_API extern FTransactionBase*		GUndo;
CORE_API extern FOutputDevice*			GLogHook;
CORE_API extern FExec*					GExec;
CORE_API extern FMalloc*				GMalloc;
CORE_API extern FFileManager*			GFileManager;
CORE_API extern USystem*				GSys;
CORE_API extern UProperty*				GProperty;
CORE_API extern BYTE*					GPropAddr;
CORE_API extern UObject*                GPropObject;
CORE_API extern USubsystem*				GWindowManager;
CORE_API extern TCHAR				    GErrorHist[4096];
CORE_API extern TCHAR                   GTrue[64], GFalse[64], GYes[64], GNo[64], GNone[64];
CORE_API extern TCHAR					GCdPath[];
CORE_API extern	FLOAT					GSecondsPerCycle;
CORE_API extern	DOUBLE					GSecondsPerCycleLong;
CORE_API extern DOUBLE					GPerformanceFrequency;
CORE_API extern	FTime					GTempTime;
CORE_API extern void					(*GTempFunc)(void*);
CORE_API extern SQWORD					GTicks;
CORE_API extern INT                     GScriptCycles;
CORE_API extern DWORD					GPageSize;
CORE_API extern DWORD					GProcessorCount;
CORE_API extern DWORD					GPhysicalMemory;
CORE_API extern DWORD                   GUglyHackFlags;
CORE_API extern UBOOL					GIsScriptable;
CORE_API extern UBOOL					GIsEditor;
CORE_API extern UBOOL					GIsClient;
CORE_API extern UBOOL					GIsServer;
CORE_API extern UBOOL					GIsCommandlet;
CORE_API extern UBOOL					GIsCriticalError;
CORE_API extern UBOOL					GIsBacktraceDumped;
CORE_API extern UBOOL					GIsStarted;
CORE_API extern UBOOL					GIsRunning;
CORE_API extern UBOOL					GIsSlowTask;
CORE_API extern UBOOL					GIsGuarded;
CORE_API extern UBOOL					GIsRequestingExit;
CORE_API extern UBOOL					GIsStrict;
CORE_API extern UBOOL                   GScriptEntryTag;
CORE_API extern UBOOL                   GLazyLoad;
CORE_API extern UBOOL					GUnicode;
CORE_API extern UBOOL					GUnicodeOS;
CORE_API extern UBOOL					GIsUTracing;
CORE_API extern UBOOL					GIsUProfiling;
CORE_API extern UBOOL					GIsDemoPlayback;
CORE_API extern UBOOL                   GIsNetPlay;
CORE_API extern UBOOL					GDoCompatibilityChecks;		// -nocompat or -bytehax
CORE_API extern UBOOL					GFixCompatibilityIssues;	// -fixcompat
CORE_API extern UBOOL					GNoConstChecks;				// -noconstchecks
CORE_API extern class FGlobalMath		GMath;
CORE_API extern	URenderDevice*			GRenderDevice;
CORE_API extern class FArchive*         GDummySave;
#if OLDUNREAL_BINARY_COMPAT
CORE_API extern DWORD					GCurrentViewport;
#else
CORE_API extern UViewport*				GCurrentViewport;
#endif
CORE_API extern UClient*                GClient;
CORE_API extern DWORD                   GRestrictSetPropertyFlags;

// Threading support
CORE_API extern FSynchronizationFactory* GSynchronizationFactory;
CORE_API extern FThreadFactory*			GThreadFactory;

CORE_API extern DWORD					GMessageTime;

#ifdef UTPG_MD5
CORE_API extern UMD5Table*				MD5Table;
#endif

// Per module globals.

// stijn: on Unix platforms, DLL_EXPORT is defined as extern "C"
#ifdef WIN32
extern "C"
#endif
DLL_EXPORT const TCHAR* GPackage;

// Normal includes.
#include "UnFile.h"			// Low level utility code.
#include "UnObjVer.h"		// Object version info.
#include "UnArc.h"			// Archive class.
#include "UnTemplate.h"     // Dynamic arrays.
#include "UnName.h"			// Global name subsystem.
#include "UnStack.h"		// Script stack definition.
#include "UnObjBas.h"		// Object base class.
#include "UnCoreNet.h"		// Core networking.
#include "UnCorObj.h"		// Core object class definitions.
#include "UnClass.h"		// Class definition.
#include "UnType.h"			// Base property type.
#include "UnScript.h"		// Script class.
#include "UFactory.h"		// Factory definition.
#include "UExporter.h"		// Exporter definition.
#include "UnCache.h"		// Cache based memory management.
#include "UnMem.h"			// Stack based memory management.
#include "UnCId.h"          // Cache ID's.
#include "UnBits.h"         // Bitstream archiver.
#include "UnMath.h"         // Vector math functions.
#include "UnThreadingBase.h"	// Threading support.

// Very basic abstract debugger class.
class UDebugger //DEBUGGER
{
public:
	virtual void DebugInfo( UObject* Debugee, FFrame* Stack, FString InfoType, int LineNumber, int InputPos )=0;
	virtual void NotifyAccessedNone()=0;
};

#ifdef UTPG_MD5
#include "UnMD5.h"      // MD5 table
#endif

#if __STATIC_LINK
#include "UnCoreNative.h"
#endif

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
#endif
