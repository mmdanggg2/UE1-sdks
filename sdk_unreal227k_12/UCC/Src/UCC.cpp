/*=============================================================================
	UCC.cpp: Unreal command-line launcher.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
	* Created by Tim Sweeney.
=============================================================================*/

#if WIN32
	#include <windows.h>
	#include <io.h>    // _setmode
	#include <fcntl.h> // _O_U16TEXT
#else
	#include <errno.h>
	#include <sys/stat.h>
#endif

#if !MACOSX && !MACOSXPPC
#include <malloc.h>
#endif

#include <stdio.h>
#include <time.h>

// Core and Engine
#include "Engine.h"
#include "FConfigCacheIni.h"

#if __STATIC_LINK

// Extra stuff for static links for Engine.
#include "UnFont.h"
#include "UnEngineNative.h"
#include "Render.h"
#include "UnRender.h"
#include "UnRenderNative.h"
#include "UnNet.h"

// UPak static stuff.
#include "UPak.h"
#include "UnUPakNative.h"

// PathLogic static stuff.
#include "PathLogicClasses.h"
#include "UnPathLogicNative.h"

// Fire static stuff.
#include "UnFractal.h"
//#include "FireClasses.h"
#include "UnFluidSurface.h"
#include "UnProcMesh.h"
#include "UnFireNative.h"

// IpDrv static stuff.
#include "UnIpDrv.h"
#include "UnTcpNetDriver.h"
#include "UnIpDrvCommandlets.h"
#include "UnIpDrvNative.h"
#include "HTTPDownload.h"

#include "EditorPrivate.h"
#include "UnEditorNative.h"

#include "EmitterPrivate.h"
#include "UnEmitterNative.h"

// UWeb static stuff.
#include "UWebAdminPrivate.h"
#include "UnUWebAdminNative.h"

#include "PhysXPhysics.h"
#include "UnPhysXPhysicsNative.h"


#endif

INT GFilesOpen, GFilesOpened;

/*-----------------------------------------------------------------------------
	Global variables.
-----------------------------------------------------------------------------*/

// General.
extern "C" {HINSTANCE hInstance; }
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
#elif __PSX2_EE__
	#include "FFileManagerPSX2.h"
	FFileManagerPSX2 FileManager;
#elif __GNUG__
	#include "FFileManagerLinux.h"
	FFileManagerLinux FileManager;
#else
	#include "FFileManagerAnsi.h"
	FFileManagerAnsi FileManager;
#endif

#include "FMallocAnsi.h"
FMallocAnsi Malloc;

/*-----------------------------------------------------------------------------
	Main.
-----------------------------------------------------------------------------*/

// Unreal command-line applet executor.

INT Compare( FString& A, FString& B )
{
	return appStricmp( *A, *B );
}
static void ShowBanner( FOutputDevice& Warn )
{
	Warn.Logf(TEXT("======================================="));
	Warn.Logf(TEXT("UCC: UnrealOS execution environment"));
	Warn.Logf(TEXT("Copyright 1999 Epic Games Inc"));
	#ifdef __LINUX_X86__
	Warn.Logf( TEXT("Linux port by www.oldunreal.com") );
	#elif __LINUX_ARM__
	Warn.Logf( TEXT("Linux ARM port by www.oldunreal.com") );
	#elif MACOSX || MACOSXPPC
	Warn.Logf( TEXT("macOS port by www.oldunreal.com") );
	#endif
	Warn.Logf(TEXT("======================================="));
	Warn.Logf(TEXT(""));
}

int main( int argc, char* argv[] )
{
	// stijn: needs to be initialized early now because we use FStrings everywhere
	Malloc.Init();
	GMalloc = &Malloc;
	GIsUCC = TRUE;

#if WIN32
	::_setmode(_fileno(stdout), _O_U8TEXT); // set console mode to provide proper support for correct output of !=Latin fonts.
	::_setmode(_fileno(stderr), _O_U8TEXT);
#endif

	if (!appChdirSystem())
		Warn.Logf(TEXT("Could not chdir into the System folder!"));

#if __STATIC_LINK
	// Clean lookups.
	memset(GNativeLookupFuncs, 0, sizeof(NativeLookup) * ARRAY_COUNT(GNativeLookupFuncs));
	// Core lookups.
	INT i = 0;
	GNativeLookupFuncs[i++] = &FindCoreUObjectNative;
	GNativeLookupFuncs[i++] = &FindCoreUCommandletNative;
	GNativeLookupFuncs[i++] = &FindCoreULogHandlerNative;
	GNativeLookupFuncs[i++] = &FindCoreUScriptHookNative;
	GNativeLookupFuncs[i++] = &FindCoreULocaleNative;
	GNativeLookupFuncs[i++] = &FindNetSerializeNative;

	GNativeLookupFuncs[i++] = &FindEngineAActorNative;
	GNativeLookupFuncs[i++] = &FindEngineAPawnNative;
	GNativeLookupFuncs[i++] = &FindEngineAPlayerPawnNative;
	GNativeLookupFuncs[i++] = &FindEngineADecalNative;
	GNativeLookupFuncs[i++] = &FindEngineATimeDemoNative;
	GNativeLookupFuncs[i++] = &FindEngineAStatLogNative;
	GNativeLookupFuncs[i++] = &FindEngineAStatLogFileNative;
	GNativeLookupFuncs[i++] = &FindEngineAZoneInfoNative;
	GNativeLookupFuncs[i++] = &FindEngineAWarpZoneInfoNative;
	GNativeLookupFuncs[i++] = &FindEngineALevelInfoNative;
	GNativeLookupFuncs[i++] = &FindEngineAGameInfoNative;
	GNativeLookupFuncs[i++] = &FindEngineANavigationPointNative;
	GNativeLookupFuncs[i++] = &FindEngineUCanvasNative;
	GNativeLookupFuncs[i++] = &FindEngineUConsoleNative;
	GNativeLookupFuncs[i++] = &FindEngineUEngineNative;
	GNativeLookupFuncs[i++] = &FindEngineUScriptedTextureNative;
	GNativeLookupFuncs[i++] = &FindEngineUShadowBitMapNative;
	GNativeLookupFuncs[i++] = &FindEngineUIK_SolverBaseNative;
	GNativeLookupFuncs[i++] = &FindEngineAProjectorNative;
	GNativeLookupFuncs[i++] = &FindEngineUPX_PhysicsDataBaseNative;
	GNativeLookupFuncs[i++] = &FindEngineUPX_VehicleBaseNative;
	GNativeLookupFuncs[i++] = &FindEngineUPX_VehicleWheeledNative;
	GNativeLookupFuncs[i++] = &FindEngineUPXJ_BaseJointNative;
	GNativeLookupFuncs[i++] = &FindEngineUPhysicsAnimationNative;
	GNativeLookupFuncs[i++] = &FindEngineURenderIteratorNative;
	GNativeLookupFuncs[i++] = &FindEngineAVolumeNative;
	GNativeLookupFuncs[i++] = &FindEngineAInventoryNative;
	GNativeLookupFuncs[i++] = &FindEngineAProjectileNative;
	GNativeLookupFuncs[i++] = &FindEngineUClientPreloginSceneNative;
	GNativeLookupFuncs[i++] = &FindEngineUServerPreloginSceneNative;
	GNativeLookupFuncs[i++] = &FindEngineUAnimationNotifyNative;
	GNativeLookupFuncs[i++] = &FindEngineUIK_LipSyncNative;		

	GNativeLookupFuncs[i++] = &FindFireAProceduralMeshNative;
	GNativeLookupFuncs[i++] = &FindFireUPaletteModifierNative;
	GNativeLookupFuncs[i++] = &FindFireUConstantColorNative;
	GNativeLookupFuncs[i++] = &FindFireUTextureModifierBaseNative;
	GNativeLookupFuncs[i++] = &FindFireAFluidSurfaceInfoNative;

	GNativeLookupFuncs[i++] = &FindEmitterAXRopeDecoNative;
	GNativeLookupFuncs[i++] = &FindEmitterAXEmitterNative;
	GNativeLookupFuncs[i++] = &FindEmitterAXParticleEmitterNative;
	GNativeLookupFuncs[i++] = &FindEmitterAXWeatherEmitterNative;

	GNativeLookupFuncs[i++] = &FindUPakAPawnPathNodeIteratorNative;
	GNativeLookupFuncs[i++] = &FindUPakAPathNodeIteratorNative;

	GNativeLookupFuncs[i++] = &FindPathLogicUPathLogicManagerNative;

	GNativeLookupFuncs[i++] = &FindEditorUBrushBuilderNative;
	GNativeLookupFuncs[i++] = &FindEditorUEdGUI_CheckBoxNative;
	GNativeLookupFuncs[i++] = &FindEditorUEdGUI_ComponentNative;
	GNativeLookupFuncs[i++] = &FindEditorUEdGUI_BaseNative;
	GNativeLookupFuncs[i++] = &FindEditorUEdGUI_ComboBoxNative;
	GNativeLookupFuncs[i++] = &FindEditorUEdGUI_WindowFrameNative;

	GNativeLookupFuncs[i++] = &FindIpDrvAInternetLinkNative;
	GNativeLookupFuncs[i++] = &FindIpDrvAUdpLinkNative;
	GNativeLookupFuncs[i++] = &FindIpDrvATcpLinkNative;

	GNativeLookupFuncs[i++] = &FindUWebAdminUWebQueryNative;
#endif

#if !_MSC_VER && 0//!DO_RYANS_HACKY_GUARD_BLOCKS
		__Context::StaticInit();
	#endif

	INT ErrorLevel = 0;
	GIsStarted     = 1;
	TCHAR Language[256]=TEXT("");
	appStrcpy(Language, UObject::GetLanguage()); // store Language setting to restore when using "make"

#ifndef _DEBUG
	try
#endif
	{
		GIsGuarded = 1;

		#if !_MSC_VER
		{ // Set module name.
			//appStrcpy( GModule, "Unreal" );
			//strcpy( GModule, argv[0] );
			FString ModuleStr(FString::GetFilenameOnlyStr(ANSI_TO_TCHAR(argv[0]))); // Strip path and extension from module name.
			appStrncpy(GModule, *ModuleStr, ARRAY_COUNT(GModule));
		}
		#endif

		// Parse command line.
		#if WIN32
            TCHAR CmdLine[1024], *CmdLinePtr=CmdLine;
            *CmdLinePtr = 0;
			ANSICHAR* Ch = GetCommandLineA();
			while( *Ch && *Ch!=' ' )
				Ch++;
			while( *Ch==' ' )
				Ch++;
			while( *Ch )
				*CmdLinePtr++ = *Ch++;
			*CmdLinePtr++ = 0;
		#else
            char CmdLine[1024] ="";
            char *CmdLinePtr=CmdLine;
            *CmdLinePtr = 0;
            for( INT i=1; i<argc; i++ )
            {
                if ( (strlen(CmdLine) + strlen(argv[i])) > (sizeof(CmdLine) -1))
                    break;
                if( i>1 )
                    strcat( CmdLine, " " );
                strcat( CmdLine, argv[i] );
            }
            TCHAR* CommandLine = appStaticString1024();
            appStrcpy(CommandLine,ANSI_TO_TCHAR(CmdLine));
		#endif

		// Init engine core.
		#ifdef __LINUX__
		appInit( TEXT("UnrealLinux"), CommandLine, &Malloc, &Log, &Error, &Warn, &FileManager, FConfigCacheIni::Factory, 1 );
		#elif _WIN32
		appInit( TEXT("Unreal"), CmdLine, &Malloc, &Log, &Error, &Warn, &FileManager, FConfigCacheIni::Factory, 1 );
		GSys->MainWindow = GetConsoleWindow();
		#elif MACOSX || MACOSXPPC
		appInit( TEXT("UnrealOSX"), CommandLine, &Malloc, &Log, &Error, &Warn, &FileManager, FConfigCacheIni::Factory, 1 );
		#endif
	 
		// Set up meta package readers.
//		for( i=0; i<GSys->MetaPackages.Num(); i++ )
//		{
//			printf("%ls\n", *GSys->MetaPackages(i));
//			((FFileManagerPSX2*) GFileManager)->AddMetaArchive( *GSys->MetaPackages(i), &Error );
//		}

		// Get the ucc stuff going.
		//UObject::SetLanguage(TEXT("int"));
		FString Token = argc>1 ? appFromAnsi(argv[1]) : TEXT("");
		TArray<FRegistryObjectInfo> List;
		UObject::GetRegistryObjects( List, UClass::StaticClass(), UCommandlet::StaticClass(), 0 );
		GIsClient = GIsServer = GIsEditor = GIsScriptable = 1;
		GLazyLoad = 0;
		UBOOL Help = 0;
		DWORD LoadFlags = LOAD_NoWarn | LOAD_Quiet;

		// stijn: We need to do this _here_ and not further down because we need
		// to fully load the UCommandlet class and construct its properties
		// _before_ we load any other commandlets. If we don't do this in the
		// correct order, the commandlets won't have any of their properties
		// set.
		verify(UObject::StaticLoadClass(UCommandlet::StaticClass(), NULL, TEXT("Core.Commandlet"), NULL, LOAD_NoFail, NULL) == UCommandlet::StaticClass());

				// Init static classes.
#if __STATIC_LINK
		GIsEditor = 0; // To enable loading localized text.
		AUTO_INITIALIZE_REGISTRANTS_ENGINE;
		AUTO_INITIALIZE_REGISTRANTS_FIRE;
		AUTO_INITIALIZE_REGISTRANTS_EMITTER;
		AUTO_INITIALIZE_REGISTRANTS_UPAK;
		AUTO_INITIALIZE_REGISTRANTS_PATHLOGIC;
		AUTO_INITIALIZE_REGISTRANTS_EDITOR;
		AUTO_INITIALIZE_REGISTRANTS_IPDRV;
		AUTO_INITIALIZE_REGISTRANTS_UWEBADMIN;
		AUTO_INITIALIZE_REGISTRANTS_RENDER;
		AUTO_INITIALIZE_REGISTRANTS_PHYSXPHYSICS;

		// stijn: lld is a bit too eager at discarding GLoaded symbols.
		// This hack forces lld to keep said symbols around
		extern BYTE GLoadedRender;
		extern BYTE GLoadedPhysXPhysics;

		volatile BYTE Dummy;
		Dummy = GLoadedRender;
		Dummy = GLoadedPhysXPhysics;
#endif

		
		if( Token==TEXT("") )
		{
			ShowBanner( Warn );
        #ifdef _WIN32
			Warn.Logf( TEXT("Use \"ucc help\" for help") );
        #elif __LINUX__
            Warn.Logf( TEXT("Use \"UCCLinux help\" for help") );
        #else
            Warn.Logf( TEXT("Use \"UCC help\" for help") );
        #endif
		}
		else if( Token==TEXT("HELP") )
		{
			ShowBanner( Warn );
			GIsEditor = 0;
			const TCHAR* Tmp = appCmdLine();
			
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
							HelpCmd = Default->GetFullName();
							HelpCmd = HelpCmd.Left(HelpCmd.InStr(TEXT("Commandlet")));
						}
						new(Items)FString( FString(TEXT("   ucc ")) + HelpCmd.RightPad(21) + TEXT(" ") + Default->HelpOneLiner );
					}
				}
				new(Items)FString( TEXT("   ucc help <command>        Get help on a command") );
				Sort( &Items(0), Items.Num() );
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
						Warn.Logf( TEXT("   ucc %ls"), *Default->HelpUsage );
					}
					if( Default->HelpParm[0]!=TEXT("") )
					{
						Warn.Logf( TEXT("") );
						Warn.Logf( TEXT("Parameters:") );
						for( INT i=0; i<ARRAY_COUNT(Default->HelpParm) && Default->HelpParm[i]!=TEXT(""); i++ )
							Warn.Logf( TEXT("   %ls %ls"), *Default->HelpParm[i].RightPad(16), *Default->HelpDesc[i] );
					}
					if( Default->HelpWebLink!=TEXT("") )
					{
						Warn.Logf( TEXT("") );
						Warn.Logf( TEXT("For more info, see") );
						Warn.Logf( TEXT("   %ls"), *Default->HelpWebLink );
					}
				}
				else
				{
					// Some commandlets, such as "Make" require to be int.
					if (Default->ForceInt)
						UObject::SetLanguage(TEXT("int"));

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
					debugf( TEXT("Executing %ls"), Class->GetFullName() );
					GIsClient = Default->IsClient;
					GIsServer = Default->IsServer;
					GIsEditor = Default->IsEditor;
					GLazyLoad = Default->LazyLoad;

					if (GIsEditor)
					{
						// Load Exporters.
						TArray<FRegistryObjectInfo> ExporterList;
						UObject::GetRegistryObjects(ExporterList, UClass::StaticClass(), UExporter::StaticClass(), 0);
						for (INT i = 0; i<ExporterList.Num(); i++)
							UObject::StaticLoadClass(UExporter::StaticClass(), NULL, *ExporterList(i).Object, NULL, LoadFlags, NULL);

						// Load Factories.
						TArray<FRegistryObjectInfo> FactoryList;
						UObject::GetRegistryObjects(FactoryList, UClass::StaticClass(), UFactory::StaticClass(), 0);
						for (INT i = 0; i<FactoryList.Num(); i++)
							UObject::StaticLoadClass(UFactory::StaticClass(), NULL, *FactoryList(i).Object, NULL, LoadFlags, NULL);
					}

					UCommandlet* Commandlet = ConstructObject<UCommandlet>( Class );
					// --- stijn: *sigh* we need this to keep commandlets from getting garbage collected
					Commandlet->AddToRoot();
					//             --- stijn
					Commandlet->InitExecution();
					Commandlet->ParseParms( appCmdLine() );
					Commandlet->Main( appCmdLine() );
					if( Commandlet->ShowErrorCount )
						GWarn->Logf( TEXT("%i warning(s)"),Warn.WarningCount );
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
				Warn.Logf( TEXT("Commandlet %ls not found"), *Token );
			}
		}
		appPreExit();
		GIsGuarded = 0;
	}
#ifndef _DEBUG
    catch(TCHAR* Err)
	{
		// Crashed.
		Error.Serialize(Err, NAME_Error);
		ErrorLevel = 1;
		GIsGuarded = 0;
		Error.HandleError();
	}
	catch( ... )
	{
		// Crashed.
		ErrorLevel = 1;
		GIsGuarded = 0;
		Error.HandleError();
	}
#endif
	UObject::SetLanguage(Language);
	appExit();
	GIsStarted = 0;
	return ErrorLevel;
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
