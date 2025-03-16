/*=============================================================================
	UnDemoRec.h: Unreal demo recording.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Jack Porter.
=============================================================================*/

class UDemoRecDriver;
class UDemoRecConnection;

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack(push,OBJECT_ALIGNMENT)
#endif

/*-----------------------------------------------------------------------------
	UDemoRecConnection.
-----------------------------------------------------------------------------*/

//
// Simulated network connection for recording and playing back game sessions.
//
class ENGINE_API UDemoRecConnection : public UNetConnection
{
	DECLARE_CLASS(UDemoRecConnection,UNetConnection,CLASS_Config|CLASS_Transient,Engine)
	NO_DEFAULT_CONSTRUCTOR(UDemoRecConnection)

	// Constructors.
	void StaticConstructor();
	UDemoRecConnection( UNetDriver* InDriver, const FURL& InURL );

	// UNetConnection interface.
	FString LowLevelGetRemoteAddress();
	FString LowLevelDescribe();
	void LowLevelSend( void* Data, INT Count );
	INT IsNetReady( UBOOL Saturate );
	void FlushNet();
	void HandleClientPlayer( APlayerPawn* Pawn );

	// UDemoRecConnection functions.
	UDemoRecDriver* GetDriver();
};

/*-----------------------------------------------------------------------------
	UDemoRecDriver.
-----------------------------------------------------------------------------*/

//
// Simulated network driver for recording and playing back game sessions.
//
class ENGINE_API UDemoRecDriver : public UNetDriver
{
	DECLARE_CLASS(UDemoRecDriver,UNetDriver,CLASS_Transient|CLASS_Config,Engine)

	// Variables.
	FString			DemoFilename;
	FStringNoInit	DemoSpectatorClass;
	FArchive*		FileAr;
	UBOOL			ClientThirdPerson;
	UBOOL			TimeBased;
	UBOOL			NoFrameCap;
	UBOOL			DemoEnded;
	UBOOL			Loop;
	INT				FrameNum;
	FURL            LoopURL;
#if OLDUNREAL_DEMO_COMPAT
	UBOOL			BackwardCompatibility;
#endif

	// Constructors.
	void StaticConstructor();
	UDemoRecDriver();

	// UNetDriver interface.
	void LowLevelDestroy();
	FString LowLevelGetNetworkNumber();
	UBOOL InitConnect( FNetworkNotify* InNotify, FURL& ConnectURL, FString& Error );
	UBOOL InitListen( FNetworkNotify* InNotify, FURL& ConnectURL, FString& Error );
	void TickDispatch( FLOAT DeltaTime );

	// FExec interface.
	INT Exec( const TCHAR* Cmd, FOutputDevice& Ar=*GLog );

	// UDemoRecDriver functions.
	UBOOL InitBase( UBOOL Connect, FNetworkNotify* InNotify, FURL& ConnectURL, FString& Error );
	ULevel* GetLevel();
	void SpawnDemoRecSpectator( UNetConnection* Connection );
};

/*-----------------------------------------------------------------------------
	UPackageMapDemo - v469 backwards compatible demo recording support
-----------------------------------------------------------------------------*/
#if OLDUNREAL_DEMO_COMPAT
class ENGINE_API UPackageMapDemo : public UPackageMapLevel
{
	DECLARE_CLASS(UPackageMapDemo, UPackageMapLevel, CLASS_Transient, Engine);

	INT LookupDemoGeneration(FPackageInfo& PackageInfo);
	void Compute();

	UBOOL DowngradePackages()
	{
		return false;
	}

	UPackageMapDemo()
	{}
	UPackageMapDemo(UNetConnection* InConnection)
		: UPackageMapLevel(InConnection)
	{}

};
#endif

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (pop)
#endif

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
