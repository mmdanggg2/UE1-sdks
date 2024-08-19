/*=============================================================================
	UnEngine.h: Unreal engine definition.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack(push,OBJECT_ALIGNMENT)
#endif

/*-----------------------------------------------------------------------------
	Unreal engine.
-----------------------------------------------------------------------------*/

// stijn: struct passed to UEngine::CleanupDestroyed. CleanupDestroyed delays
// the effective C++ cleanup of actors until the weighted number of actors
// to be deleted exceeds 128
struct FLevelCleanup
{
	ULevel* Level;
	INT CleanupWeight;

	bool operator==(const struct FLevelCleanup& Other)
	{
		return Level == Other.Level;
	}
};

enum EActorAction
{
	NOTE_ActorSpawned,
	NOTE_ActorRedo,
	NOTE_ActorDestroyed,
	NOTE_ActorPropertiesChange,
};

class ENGINE_API UEngine : public USubsystem
{
	DECLARE_ABSTRACT_CLASS(UEngine,USubsystem,CLASS_Config|CLASS_Transient,Engine)

	// Subsystems.
	UClass*					GameRenderDeviceClass;
	UClass*					AudioDeviceClass;
	UClass*					ConsoleClass;
	UClass*					NetworkDriverClass;
	UClass*					LanguageClass;

	// Variables.
	class UPrimitive*		Cylinder;
	class UClient*			Client;
	class URenderBase*		Render;
	class UAudioSubsystemOldUnreal469*	Audio;
	INT						TickCycles, GameCycles, ClientCycles;
	INT						CacheSizeMegs;

	// Careful! This is a "bool" in uscript, so don't put another
	//  "bool" next to it, or the engine will want to pool them and you'll
	//  have serious problems!  --ryan.
	UBOOL				    UseSound;

	FLOAT					CurrentTickRate;
	INT						MinClientVersion; //elmuerte: used to set the minimal client version
	INT						MaxCacheItems;

	// OldUnreal/UTPG MD5 code
	FString					MD5ErrorMsg; // used for the corrupt file report
	UBOOL					MD5Enable;
	TArray<FString>			MD5Ignore;
	UBOOL					MD5AutoUpdate;
	FStringNoInit			MD5UpdateURL;
	INT						MD5Version;
	INT						MD5LastUpdateCheck;	

	// Constructors.
	UEngine();
	void StaticConstructor();

	// UObject interface.
	void Serialize( FArchive& Ar );
	void Destroy();

#if !DEMOVERSION
	virtual void vtPad1() {}
	virtual void vtPad2() {}
	virtual void vtPad3() {}
#endif

	// UEngine interface.
	virtual void Init();
	virtual void Exit();
	virtual UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Out=*GLog );
	virtual void Flush( UBOOL AllowPrecache );
	virtual UBOOL Key( UViewport* Viewport, EInputKey Key );
	virtual UBOOL InputEvent( UViewport* Viewport, EInputKey iKey, EInputAction State, FLOAT Delta=0.f );
	virtual void Tick( FLOAT DeltaSeconds )=0;
	virtual void Draw( UViewport* Viewport, UBOOL Blit=1, BYTE* HitData=NULL, INT* HitSize=NULL )=0;
	virtual void MouseDelta( UViewport* Viewport, DWORD Buttons, FLOAT DX, FLOAT DY )=0;
	virtual void MousePosition( UViewport* Viewport, DWORD Buttons, FLOAT X, FLOAT Y )=0;
	virtual void Click( UViewport* Viewport, DWORD Buttons, FLOAT X, FLOAT Y )=0;
	virtual void SetClientTravel( UPlayer* Viewport, const TCHAR* NextURL, UBOOL bItems, ETravelType TravelType )=0;
	virtual INT ChallengeResponse( INT Challenge );
	virtual FLOAT GetMaxTickRate();
	virtual void SetProgress( const TCHAR* Str1, const TCHAR* Str2, const TCHAR* Str3, const TCHAR* Str4, FLOAT Seconds );
	void InitAudio();

	virtual void edSetClickLocation( FVector& InLocation ) {};

	// Temporary!!
	virtual int edcamMode( UViewport* Viewport ) {return 0;}

	virtual void HandleDestruction( AActor* Actor);
	virtual UBOOL CleanupDestroyed( UBOOL bForce, TArray<FLevelCleanup>* Levels=nullptr );
#if !DEMOVERSION
	// stijn: reused vtPad6 for edNoteActor here
	virtual void edNoteActor(AActor* Other, EActorAction Action) {}
#endif

	// Higor: should we turn these into virtual functions or accessors?
	INT              GetVersion();
	FString          GetRevision();
	FString			 GetArchitecture();
	INT              GetMinNetVersion();
};

/*-----------------------------------------------------------------------------
	UServerCommandlet.
-----------------------------------------------------------------------------*/

class UServerCommandlet : public UCommandlet
{
	DECLARE_CLASS(UServerCommandlet,UCommandlet,CLASS_Transient,Engine);
	void StaticConstructor();
	INT Main( const TCHAR* Parms );
};

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (pop)
#endif

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
