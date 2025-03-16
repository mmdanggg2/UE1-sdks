/*=============================================================================
	UCC.cpp: Unreal command-line launcher.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Tim Sweeney.
=============================================================================*/

#if WIN32 && !__STATIC_LINK
	#pragma warning(push)
	#pragma warning(disable: 4121) /* 'JOBOBJECT_IO_RATE_CONTROL_INFORMATION_NATIVE_V2': alignment of a member was sensitive to packing */
	#include <windows.h>
	#pragma warning(pop)
#else
	#include <errno.h>
	#include <sys/stat.h>
#endif

#if WIN32
#include <io.h>
#include <fcntl.h>
#endif

#if !MACOSX
#include <malloc.h>
#endif

#include <stdio.h>
#include <time.h>

// Core and Engine
#include "Engine.h"

#if WIN32
#include "SETranslator.h"
#endif

#if __STATIC_LINK

// Extra stuff for static links for Engine.
#include "UnEngineNative.h"
#include "UnCon.h"
#include "UnRender.h"
#include "UnNet.h"

// Editor static stuff.
#include "EditorPrivate.h"
#include "UnEditorNative.h"

#undef _INC_EDITOR

// NullNetDriver static stuff
//#include "NullNetDriver.h"

// Fire static stuff.
#include "UnFractal.h"

// IpDrv static stuff.
#include "UnIpDrv.h"
#include "UnTcpNetDriver.h"
#include "UnIpDrvCommandlets.h"
#include "UnIpDrvNative.h"
#include "HTTPDownload.h"

// UWeb static stuff
#include "UWeb.h"
#include "UWebNative.h"

#include "udemoprivate.h"
#include "udemoNative.h"

#else

#include "FConfigCacheIni.h"

#endif

INT GFilesOpen, GFilesOpened;

/*-----------------------------------------------------------------------------
	Global variables.
-----------------------------------------------------------------------------*/

// General.
#if _MSC_VER
	extern "C" {HINSTANCE hInstance;}
#endif
TCHAR GPackageInternal[64] = TEXT("UCC");
extern "C" {const TCHAR* GPackage=GPackageInternal;}

// Log.
#include "FOutputDeviceFile.h"
FOutputDeviceFile Log;

// Error.
#include "FOutputDeviceAnsiError.h"
FOutputDeviceAnsiError Error;

// Feedback.
#include "FFeedbackContextAnsi.h"
FFeedbackContextAnsi Warn;

// File manager.
#if WIN32
	#include "FFileManagerWindows.h"
	FFileManagerWindows FileManager;
#else
	#include "FFileManagerLinux.h"
	FFileManagerLinux FileManager;
#endif

/*-----------------------------------------------------------------------------
	Main.
-----------------------------------------------------------------------------*/

// Unreal command-line applet executor.
FString RightPad( FString In, INT Count )
{
	while( In.Len()<Count )
		In += TEXT(" ");
	return In;
}
INT Compare( FString& A, FString& B )
{
	return appStricmp( *A, *B );
}
void ShowBanner( FOutputDevice& Warn )
{
	Warn.Logf( TEXT("=======================================") );
	Warn.Logf( TEXT("ucc.exe: UnrealOS execution environment") );
	Warn.Logf( TEXT("Copyright 1999 Epic Games Inc") );
	Warn.Logf( TEXT("=======================================") );
	Warn.Logf( TEXT("") );
}
int main( int argc, char* argv[] )
{
#if WIN32
	// stijn: This hack enables full buffering for stdout and stderr,
	// but only if stdout and stderr output to a terminal window
	DWORD Dummy;
	if (GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &Dummy) &&
		freopen("CONOUT$", "w", stdout))
		setvbuf(stdout, NULL, _IOFBF, 32 * 1024);
	if (GetConsoleMode(GetStdHandle(STD_ERROR_HANDLE), &Dummy) &&
		freopen("CONOUT$", "w", stderr))
		setvbuf(stderr, NULL, _IOFBF, 32 * 1024);

	// switch to a unicode code page in the console
	_setmode(_fileno(stdout), _O_U8TEXT);
	_setmode(_fileno(stderr), _O_U8TEXT);
#endif

#if !WIN32
appPlatformPreInit();
#endif

    if (!appChdirSystem())
		Warn.Logf(TEXT("Could not chdir into the System folder!"));
	
#if __STATIC_LINK
		// Clean lookups.
	for (INT k=0; k<ARRAY_COUNT(GNativeLookupFuncs); k++)
	{
		GNativeLookupFuncs[k] = NULL;
	}

	INT k = 0;
	GNativeLookupFuncs[k++] = &FindCoreUObjectNative;
	GNativeLookupFuncs[k++] = &FindCoreUCommandletNative;
	GNativeLookupFuncs[k++] = &FindCoreURegistryNative;

	// Engine lookups.
	GNativeLookupFuncs[k++] = &FindEngineAActorNative;
	GNativeLookupFuncs[k++] = &FindEngineAPawnNative;
	GNativeLookupFuncs[k++] = &FindEngineAPlayerPawnNative;
	GNativeLookupFuncs[k++] = &FindEngineADecalNative;
	GNativeLookupFuncs[k++] = &FindEngineAStatLogNative;
	GNativeLookupFuncs[k++] = &FindEngineAStatLogFileNative;
	GNativeLookupFuncs[k++] = &FindEngineAZoneInfoNative;
	GNativeLookupFuncs[k++] = &FindEngineAWarpZoneInfoNative;
	GNativeLookupFuncs[k++] = &FindEngineALevelInfoNative;
	GNativeLookupFuncs[k++] = &FindEngineAGameInfoNative;
	GNativeLookupFuncs[k++] = &FindEngineANavigationPointNative;
	GNativeLookupFuncs[k++] = &FindEngineUCanvasNative;
	GNativeLookupFuncs[k++] = &FindEngineUConsoleNative;
	GNativeLookupFuncs[k++] = &FindEngineUScriptedTextureNative;

	GNativeLookupFuncs[k++] = &FindIpDrvAInternetLinkNative;
	GNativeLookupFuncs[k++] = &FindIpDrvAUdpLinkNative;
	GNativeLookupFuncs[k++] = &FindIpDrvATcpLinkNative;

	// UWeb lookups.
	GNativeLookupFuncs[k++] = &FindUWebUWebResponseNative;
	GNativeLookupFuncs[k++] = &FindUWebUWebRequestNative;

	// UDemo lookups.
	GNativeLookupFuncs[k++] = &FindudemoUUZHandlerNative;
	GNativeLookupFuncs[k++] = &FindudemoUudnativeNative;
	GNativeLookupFuncs[k++] = &FindudemoUDemoInterfaceNative;

	// Editor lookups.
	GNativeLookupFuncs[k++] = &FindEditorUBrushBuilderNative;
#endif

	INT ErrorLevel = 0;
	GIsStarted     = 1;
	try
	{
		GIsGuarded = 1;

#if !WIN32
		// Set module name.
		appStrncpy( GModule, TEXT("ucc"), ARRAY_COUNT(GModule));	
#endif
		
#if WIN32
		SetSETranslator();
#endif
		
		// Parse command line.
		FString Token, CmdLine = appPlatformBuildCmdLine(2, argv, argc, &Token);

		// Init engine core.
		appInit(APP_NAME, *CmdLine, nullptr, &Log, &Error, &Warn, &FileManager, FConfigCacheIni::Factory, 1 );

		// Get the ucc stuff going.	
		UObject::SetLanguage(TEXT("int"));		
		TArray<FRegistryObjectInfo> List;
		UObject::GetRegistryObjects( List, UClass::StaticClass(), UCommandlet::StaticClass(), 0 );
		GIsClient = GIsServer = GIsEditor = GIsScriptable = 1;
		GDoCompatibilityChecks  = !ParseParam(appCmdLine(), TEXT("bytehax")) && !ParseParam(appCmdLine(), TEXT("nocompat"));
		GFixCompatibilityIssues = ParseParam(appCmdLine(), TEXT("fixcompat"));
		GNoConstChecks			= ParseParam(appCmdLine(), TEXT("noconstchecks")) | ParseParam(appCmdLine(), TEXT("bytehax"));
		GLazyLoad = 0;
		UBOOL Help = 0;
		DWORD LoadFlags = LOAD_NoWarn | LOAD_Quiet;

		// stijn: for the static builds, we need to make sure that Core.Commandlet 
		// gets loaded from Core.u because the UScript class contains the 
		// definitions for the class properties. UCommandlet is one of the few 
		// classes that lacks a StaticConstructor that constructs the necessary
		// properties in C++ code.
		verify(UObject::StaticLoadClass(UCommandlet::StaticClass(), NULL, TEXT("Core.Commandlet"), NULL, LOAD_NoFail, NULL) == UCommandlet::StaticClass());

		// stijn: now that we've loaded Core.Commandlet, the engine knows about
		// a commandlet's properties and we can safely register other commandlets...
		GIsEditor = 0; // To enable loading localized text.
#if __STATIC_LINK
		AUTO_INITIALIZE_REGISTRANTS_ENGINE;
		//AUTO_INITIALIZE_REGISTRANTS_NULLNETDRIVER;
		AUTO_INITIALIZE_REGISTRANTS_FIRE;
		AUTO_INITIALIZE_REGISTRANTS_IPDRV;
		AUTO_INITIALIZE_REGISTRANTS_UWEB;
		AUTO_INITIALIZE_REGISTRANTS_EDITOR;
#endif
		GIsEditor = 1; // To enable loading localized text.

		if( Token==TEXT("") )
		{
			ShowBanner( Warn );
			Warn.Logf( TEXT("Use \"ucc help\" for help") );
		}
		else if( Token==TEXT("HELP") )
		{
			ShowBanner( Warn );			
			const TCHAR* Tmp = appCmdLine();
			GIsEditor = 0; // To enable loading localized text.
			if( !ParseToken( Tmp, Token, 0 ) )
			{
				INT i;
				Warn.Logf( TEXT("Usage:") );
				Warn.Logf( TEXT("   ucc <command> <parameters>") );
				Warn.Logf( TEXT("") );
				Warn.Logf( TEXT("Commands for \"ucc\":") );
				TArray<FString> Items;
				for( i=0; i<List.Num(); i++ )
				{
					UClass* Class = UObject::StaticLoadClass( UCommandlet::StaticClass(), NULL, *List(i).Object, NULL, LoadFlags, NULL );
					if( Class )
					{
						UCommandlet* Default = (UCommandlet*)Class->GetDefaultObject();
						FString HelpCmd = Default->HelpCmd;
						if (HelpCmd == TEXT(""))
						{
							HelpCmd = *FObjectFullName(Default);
							HelpCmd = HelpCmd.Left(HelpCmd.InStr(TEXT("Commandlet")));
						}
						new(Items)FString( FString(TEXT("   ucc ")) + RightPad(HelpCmd,21) + TEXT(" ") + Default->HelpOneLiner );
					}
				}
				new(Items)FString( TEXT("   ucc help <command>        Get help on a command") );
				Sort( Items );
				for( i=0; i<Items.Num(); i++ )
					Warn.Log( Items(i) );
			}
			else
			{
				Help = 1;
				goto Process;
			}
		}
		else
		{
			// Look it up.
			if( Token==TEXT("Make") )
				LoadFlags |= LOAD_DisallowFiles;
		Process:
			UClass* Class = UObject::StaticLoadClass( UCommandlet::StaticClass(), NULL, *Token, NULL, LoadFlags, NULL );
			if( !Class )
				Class = UObject::StaticLoadClass( UCommandlet::StaticClass(), NULL, *(Token+TEXT("Commandlet")), NULL, LoadFlags, NULL );
			if( !Class )
			{
				INT i;
				for( i=0; i<List.Num(); i++ )
				{
					FString Str = List(i).Object;
					while( Str.InStr(TEXT("."))>=0 )
						Str = Str.Mid(Str.InStr(TEXT("."))+1);
					if( Token==Str || Token+TEXT("Commandlet")==Str )
						break;
				}
				if( i<List.Num() )
					Class = UObject::StaticLoadClass( UCommandlet::StaticClass(), NULL, *List(i).Object, NULL, LoadFlags, NULL );
			}
			if( Class )
			{
				UCommandlet* Default = (UCommandlet*)Class->GetDefaultObject();
				if( Help )
				{
					// Get help on it.
					if( Default->HelpUsage!=TEXT("") )
					{
						Warn.Logf( TEXT("Usage:") );
						Warn.Logf( TEXT("   ucc %s"), *Default->HelpUsage );
					}
					if( Default->HelpParm[0]!=TEXT("") )
					{
						Warn.Logf( TEXT("") );
						Warn.Logf( TEXT("Parameters:") );
						for( INT i=0; i<ARRAY_COUNT(Default->HelpParm) && Default->HelpParm[i]!=TEXT(""); i++ )
							Warn.Logf( TEXT("   %s %s"), *RightPad(Default->HelpParm[i],16), *Default->HelpDesc[i] );
					}
					if( Default->HelpWebLink!=TEXT("") )
					{
						Warn.Logf( TEXT("") );
						Warn.Logf( TEXT("For more info, see") );
						Warn.Logf( TEXT("   %s"), *Default->HelpWebLink );
					}
				}
				else
				{
					// Run it.
					if( Default->LogToStdout )
					{
						Warn.AuxOut = GLog;
						GLog        = &Warn;
					}
					if( Default->ShowBanner )
					{
						ShowBanner( Warn );
					}
					debugf( TEXT("Executing %s"), *FObjectFullName(Class) );
					GIsClient = Default->IsClient;
					GIsServer = Default->IsServer;
					GIsEditor = Default->IsEditor;
					GLazyLoad = Default->LazyLoad;
					GIsCommandlet = 1 && !GIsClient && !GIsServer;
					UCommandlet* Commandlet = ConstructObject<UCommandlet>( Class );
					// --- stijn: *sigh* we need this to keep commandlets from getting garbage collected
					Commandlet->AddToRoot();
					//             --- stijn
					Commandlet->InitExecution();
					Commandlet->ParseParms( appCmdLine() );
					Commandlet->Main( appCmdLine() );
					if( Commandlet->ShowErrorCount )
						GWarn->Logf( TEXT("Success - 0 error(s), %i warnings"), Warn.WarningCount );
					if( Default->LogToStdout )
					{
						Warn.AuxOut = NULL;
						GLog        = &Log;
					}
				}
			}
			else
			{
				ShowBanner( Warn );
				Warn.Logf( TEXT("Commandlet %s not found"), *Token );
			}
		}
		appPreExit();
		GIsGuarded = 0;
	}
#if !WIN32
	catch (TCHAR* Err)
	{
		// Crashed.
		Error.Serialize(Err, NAME_Error);
		ErrorLevel = 1;
		GIsGuarded = 0;
		Error.HandleError();		
	}
#endif
	catch( ... )
	{
		// Crashed.
		ErrorLevel = 1;
		GIsGuarded = 0;
		Error.HandleError();
	}
	appExit();
	GIsStarted = 0;
	return ErrorLevel;
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
