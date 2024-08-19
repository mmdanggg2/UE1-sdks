/*=============================================================================
	UnAudio.h: Unreal base audio.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
		* Wave modification code by Erik de Neve
=============================================================================*/

class USound;
class UMusic;
class UAudioSubsystem;

/*-----------------------------------------------------------------------------
	UAudioSubsystem.
-----------------------------------------------------------------------------*/

//
// UAudioSubsystem is the abstract base class of
// the game's audio subsystem.
//
class ENGINE_API UAudioSubsystem : public USubsystem
{
	DECLARE_ABSTRACT_CLASS(UAudioSubsystem,USubsystem,CLASS_Config,Engine)
	NO_DEFAULT_CONSTRUCTOR(UAudioSubsystem)

	// UAudioSubsystem interface.
	virtual UBOOL Init()=0;
	virtual void SetViewport( UViewport* Viewport )=0;
	virtual UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar=*GLog )=0;
	virtual void Update( FPointRegion Region, FCoords& Listener )=0;
	virtual void RegisterMusic( UMusic* Music )=0;
	virtual void RegisterSound( USound* Music )=0;
	virtual void UnregisterSound( USound* Sound )=0;
	virtual void UnregisterMusic( UMusic* Music )=0;
#undef PlaySound
	virtual UBOOL PlaySound( AActor* Actor, INT Id, USound* Sound, FVector Location, FLOAT Volume, FLOAT Radius, FLOAT Pitch )=0;
	virtual void NoteDestroy( AActor* Actor )=0;
	virtual UBOOL GetLowQualitySetting()=0;
	virtual UViewport* GetViewport()=0;
	virtual void RenderAudioGeometry( FSceneNode* Frame )=0;
	virtual void PostRender( FSceneNode* Frame )=0;
	virtual void CleanUp() {};
};

/*-----------------------------------------------------------------------------
	UAudioSubsystemOldUnreal469.
-----------------------------------------------------------------------------*/

// Bitmasks representing a SoundSlot derived from ESoundSlot.
enum ESoundSlotMask : DWORD
{
	SLOTMASK_None      = 1<<SLOT_None,
	SLOTMASK_Misc      = 1<<SLOT_Misc,
	SLOTMASK_Pain      = 1<<SLOT_Pain,
	SLOTMASK_Interact  = 1<<SLOT_Interact,
	SLOTMASK_Ambient   = 1<<SLOT_Ambient,
	SLOTMASK_Talk      = 1<<SLOT_Talk,
	SLOTMASK_Interface = 1<<SLOT_Interface,

	SLOTMASK_All       = 0xFFFFFFFF,
	SLOTMASK_Destroy   = SLOTMASK_Ambient,
};

enum EAudioCompatibilityFlags : DWORD
{
	AUDIOCOMPAT_None				  = 0x00,
	AUDIOCOMPAT_DisableSoundOcclusion = 0x01,		// The server/engine will do distance culling and volume dampening when the sound source is occluded. The audio sub does not have to worry about occlusion
};

//
// Wrapper around old integer Ids used for Sound playback.
//
union FSoundId
{
	INT Id;
	struct
	{
		BITFIELD bNoOverride:1;
		BITFIELD Slot:3;
		BITFIELD Index:27;
		BITFIELD bClient:1;      // Set when Id is generated in Client NetMode.
	};

	// Constructors.
	FSoundId()
	{}
	FSoundId( INT InId )
	: Id(InId)
	{}
	FSoundId( AActor* InActor, ESoundSlot InSlot, UBOOL bInNoOverride, UBOOL bInClient=0 )
	: Index(InActor->GetIndex())
	, Slot(InSlot)
	, bNoOverride(bInNoOverride)
	, bClient(bInClient)
	{}

	// Operators.
	UBOOL operator==( const FSoundId& Other ) const
	{
		// Ignore bNoOverride here, as it's only used to resolve (Slot,Index) pair conflicts.
		return Slot==Other.Slot && Index==Other.Index && bClient==Other.bClient;
	}
};

//
// This is the extended interface for OldUnreal patch 469. The primary reason
// why we added this is because the Audio device had no reliable way to
// determine the listener actor, nor that Actor's velocity. This caused
// problems with sound despatialization and made correct doppler effects
// impossible to produce.
//
class ENGINE_API UAudioSubsystemOldUnreal469 : public UAudioSubsystem
{
	DECLARE_ABSTRACT_CLASS(UAudioSubsystemOldUnreal469, UAudioSubsystem, CLASS_Config, Engine)
	NO_DEFAULT_CONSTRUCTOR(UAudioSubsystemOldUnreal469)

	// stijn: We do not intend to hide/override the inherited functions here and must make that explicit
	// with "using" directives.
	using UAudioSubsystem::PlaySound;
	using UAudioSubsystem::Update;
	using UAudioSubsystem::NoteDestroy;
	using UAudioSubsystem::SetViewport;

	// Legacy interface. The default implementations just forward to the extended interface.
	virtual void Update(FPointRegion ListenerRegion, FCoords& ListenerCoords);
	virtual UBOOL PlaySound(AActor* Actor, INT Id, USound* Sound, FVector Location, FLOAT Volume, FLOAT Radius, FLOAT Pitch);
	virtual void NoteDestroy(AActor* Actor);

	// ========================================================================
	// UAudioSubsytemOldUnreal469 interface.
	// ========================================================================
	// 
	// New PlaySound Parameters:
	// 
	// @Velocity: Velocity of the sound source
	// 
	// @PriorityMultiplier: Priority multiplier for this sound. The audio
	// device is expected to apply this to the full sound priority it
	// calculates based on sound properties such as the locations/velocities
	// of the sound source/listener, sound volume/radius, etc.
	// 
	// Galaxy Audio currently calculates sound priority as follows:
	//
	// if (!WorldSound)
	//     Priority = Volume * PriorityMultiplier;
	// else
	//     Priority = Volume * (1 - (SoundLocation-ListenerLocation).Size() / Radius) * PriorityMultiplier;
	//
	// @World: Is this a world sound or an interface sound? Interface sounds are expected to play without any spatialization effects
	// 
	virtual UBOOL PlaySound(AActor* Actor, INT Id, USound* Sound, FVector Location, FVector Velocity, FLOAT Volume, FLOAT Radius, FLOAT Pitch, FLOAT PriorityMultiplier, UBOOL World) = 0;
	//
	// New Update Parameters:
	// 
	// @TickType: Which type of Engine Tick are we performing? See struct
	// ELevelTick in Engine\Inc\UnLevel.h
	// 
	// @DeltaTime: How much time has passed since the last time we called
	// Audio->Update
	// 
	// @ListenerVelocity: Actual velocity of the sound listener. Audio
	// devices that support the new interface are expected to use this
	// velocity instead of trying to reconstruct the velocity using
	// PlayerCalcView-like heuristics
	// 
	// @ListenerActor: Actual sound listener. Same as above. Use this instead
	// of heuristics based on the Viewport->ViewTarget
	// 
	virtual void Update(ELevelTick TickType, FLOAT DeltaTime, FPointRegion ListenerRegion, FCoords& ListenerCoords, FVector ListenerVelocity, AActor* ListenerActor) = 0;
	//
	// New NoteDestroy Parameters:
	// @SlotStopMask: Which of the sounds this actor was playing should be
	// stopped? Use the SLOTMASK values in the ESoundSlotMask enum declared
	// above.
	// 
	virtual void NoteDestroy(AActor* Actor, DWORD SlotStopMask) = 0;
	//
	// New SetViewport Parameters:
	// @InCompatibilityFlags: Currently used to tell the audio sub whether
	// remotely played sounds are subject to server-side sound occlusion checks.
	// v469a servers leave occlusion checks up to the audio sub. v436-451 did not.
	// 
	virtual void SetViewport(UViewport* InViewport, DWORD InCompatibilityFlags) = 0;
	
	// New functions to stop specific sounds
	virtual UBOOL StopSound(AActor* Actor, USound* Sound) = 0;
	virtual void StopSoundId(INT Id) = 0;

	// The default implementation calls [Register|Unregister]Sound for all sound objects in the GObjObjects array
	virtual void RegisterAllSounds();	
	virtual void UnregisterAllSounds();	

	// The default implementation does nothing.
	virtual void RegisterAllMusic();
	virtual void UnregisterAllMusic();
	virtual void Exit();
};

/*-----------------------------------------------------------------------------
	UAudioSubsystemProxy.
-----------------------------------------------------------------------------*/

//
// We construct these proxy objects to support old audio devices that
// were never ported to the new OldUnreal 469 interface
//
class ENGINE_API UAudioSubsystemProxy : public UAudioSubsystemOldUnreal469
{
	DECLARE_CLASS(UAudioSubsystemProxy, UAudioSubsystemOldUnreal469, CLASS_Transient, Engine)
	NO_DEFAULT_CONSTRUCTOR(UAudioSubsystemProxy)

	// UObject interface
	UAudioSubsystemProxy(UAudioSubsystem* InOrig);
	void Destroy();
	void Serialize(FArchive& Ar);

	// UAudioSubsystem interface
	UBOOL Init();
	void SetViewport(UViewport* Viewport);
	UBOOL Exec(const TCHAR* Cmd, FOutputDevice& Ar = *GLog);
	void Update(FPointRegion Region, FCoords& Listener);
	void RegisterMusic(UMusic* Music);
	void RegisterSound(USound* Music);
	void UnregisterSound(USound* Sound);
	void UnregisterMusic(UMusic* Music);
	UBOOL PlaySound(AActor* Actor, INT Id, USound* Sound, FVector Location, FLOAT Volume, FLOAT Radius, FLOAT Pitch);
	void NoteDestroy(AActor* Actor);
	UBOOL GetLowQualitySetting();
	UViewport* GetViewport();
	void RenderAudioGeometry(FSceneNode* Frame);
	void PostRender(FSceneNode* Frame);
	void CleanUp();

	// Extended interface
	UBOOL PlaySound(AActor* Actor, INT Id, USound* Sound, FVector Location, FVector Velocity, FLOAT Volume, FLOAT Radius, FLOAT Pitch, FLOAT Priority, UBOOL World);
	void Update(ELevelTick TickType, FLOAT DeltaTime, FPointRegion ListenerRegion, FCoords& ListenerCoords, FVector ListenerVelocity, AActor* ListenerActor);
	void NoteDestroy(AActor* Actor, DWORD SlotStopMask);
	UBOOL StopSound(AActor* Actor, USound* Sound);
	void StopSoundId(INT Id);
	void SetViewport(UViewport* InViewport, DWORD InCompatibilityFlags);

	UAudioSubsystem* Orig{};
};

/*-----------------------------------------------------------------------------
	USound.
-----------------------------------------------------------------------------*/

//
// Sound data.
//
class ENGINE_API FSoundData : public TLazyArray<BYTE>
{
public:
	USound* Owner;
	void Load();
	FLOAT GetPeriod();
	FSoundData( USound* InOwner )
	: Owner( InOwner )
	{}
};

//
// A sound effect.
//
class ENGINE_API USound : public UObject
{
	DECLARE_CLASS(USound,UObject,CLASS_SafeReplace,Engine)

	// Variables.
	FSoundData	Data;
	FName		FileType;
	INT			OriginalSize;
	FLOAT       Duration;
	void*		Handle;
	static UAudioSubsystem* Audio;
	static DWORD AudioCompatibilityFlags;

	// Constructor.
	USound()
	: Data( this )
	{
		Duration = -1.f;
	}

	// Duration.
	FLOAT GetDuration()
	{
		if ( Duration < 0.f )
			Duration = Data.GetPeriod();
		return Duration;
	};

	void PS2Convert();

	// UObject interface.
	void Serialize( FArchive& Ar );
	void Destroy();
	void PostLoad();
};

/*-----------------------------------------------------------------------------
	UMusic.
-----------------------------------------------------------------------------*/

//
// A song.
//
class ENGINE_API UMusic : public UObject
{
	DECLARE_CLASS(UMusic,UObject,CLASS_SafeReplace,Engine)

	// Variables.
	TLazyArray<BYTE>	Data;
	FName				FileType;
	INT					OriginalSize;
	void*				Handle;
	static UAudioSubsystem* Audio;

	// Constructor.
	UMusic()
	{}

	// UObject implementation.
	void Serialize( FArchive& Ar );
	void Destroy();
	void PostLoad();
};

/*-----------------------------------------------------------------------------
	FWaveModInfo. 
-----------------------------------------------------------------------------*/

//  Macros to convert 4 bytes to a Riff-style ID DWORD.
//  Todo: make these endian independent !!!

#define MAKEFOURCC(ch0, ch1, ch2, ch3)\
    ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |\
    ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))

#define mmioFOURCC(ch0, ch1, ch2, ch3)\
    MAKEFOURCC(ch0, ch1, ch2, ch3)

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (push,2)
#endif

// Main Riff-Wave header.
struct FRiffWaveHeader
{ 
	DWORD	rID;			// Contains 'RIFF'
	DWORD	ChunkLen;		// Remaining length of the entire riff chunk (= file).
	DWORD	wID;			// Form type. Contains 'WAVE' for .wav files.
};

// General chunk header format.
struct FRiffChunkOld
{
	DWORD	ChunkID;		  // General data chunk ID like 'data', or 'fmt ' 
	DWORD	ChunkLen;		  // Length of the rest of this chunk in bytes.
};

// ChunkID: 'fmt ' ("WaveFormatEx" structure ) 
struct FFormatChunk
{
    _WORD   wFormatTag;        // Format type: 1 = PCM
    _WORD   nChannels;         // Number of channels (i.e. mono, stereo...).
    DWORD   nSamplesPerSec;    // Sample rate. 44100 or 22050 or 11025  Hz.
    DWORD   nAvgBytesPerSec;   // For buffer estimation  = sample rate * BlockAlign.
    _WORD   nBlockAlign;       // Block size of data = Channels times BYTES per sample.
    _WORD   wBitsPerSample;    // Number of bits per sample of mono data.
    _WORD   cbSize;            // The count in bytes of the size of extra information (after cbSize).
};

// ChunkID: 'smpl'
struct FSampleChunk
{
	DWORD   dwManufacturer;
	DWORD   dwProduct;
	DWORD   dwSamplePeriod;
	DWORD   dwMIDIUnityNote;
	DWORD   dwMIDIPitchFraction;
	DWORD	dwSMPTEFormat;		
	DWORD   dwSMPTEOffset;		//
	DWORD   cSampleLoops;		// Number of tSampleLoop structures following this chunk
	DWORD   cbSamplerData;		// 
};
 
struct FSampleLoop				// Immediately following cbSamplerData in the SMPL chunk.
{
	DWORD	dwIdentifier;		//
	DWORD	dwType;				//
	DWORD	dwStart;			// Startpoint of the loop in samples
	DWORD	dwEnd;				// Endpoint of the loop in samples
	DWORD	dwFraction;			// Fractional sample adjustment
	DWORD	dwPlayCount;		// Play count
};

// stijn: I am not proud of this but this is the cleanest solution I could think
// of. All of the structs above are aligned on 16-bit boundaries, which means
// that the DWORD pointers in the FWaveModInfo struct potentially point to
// unaligned memory. In optimized builds for ARM (an probably PPC), gcc assumes
// that all pointer derefs are aligned, so it generates optimized load/store
// instructions that trap on unaligned accesses. With this indirection, we tell
// the compiler that the DWORD pointers point to memory with 16-bit alignment so
// it should not generate instructions that cannot tolerate misalignment.
//
// NOTE: Unlike the original code, this solution _IS_ C compliant.

struct FUnalignedDWORD
{
	DWORD   dwUnaligned;
};

struct FUnalignedDWORDPtr
{
	FUnalignedDWORD* Ptr;

	DWORD operator*()
	{
		return Ptr->dwUnaligned;
	}

	void Set(DWORD In)
	{
		Ptr->dwUnaligned = In;
	}
	
	void operator=(DWORD* In)
	{
		Ptr = reinterpret_cast<FUnalignedDWORD*>(In);
	}
};

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (pop)
#endif

//
// Structure for in-memory interpretation and modification of WAVE sound structures.
//
class ENGINE_API FWaveModInfo
{
public:

	// Pointers to variables in the in-memory WAVE file.
	FUnalignedDWORDPtr pSamplesPerSec;
	FUnalignedDWORDPtr pAvgBytesPerSec;
	_WORD* pBlockAlign;
	_WORD* pBitsPerSample;
	_WORD* pChannels;

	DWORD  OldBitsPerSample;

	FUnalignedDWORDPtr pWaveDataSize;
	FUnalignedDWORDPtr pMasterSize;
	BYTE*  SampleDataStart;
	BYTE*  SampleDataEnd;
	DWORD  SampleDataSize;
	BYTE*  WaveDataEnd;

	INT	   SampleLoopsNum;
	FSampleLoop*  pSampleLoop;

	DWORD  NewDataSize;
	UBOOL  NoiseGate;

	// Constructor.
	FWaveModInfo()
	{
		NoiseGate   = false;
		SampleLoopsNum = 0;
	}
	
	// 16-bit padding.
	DWORD Pad16Bit( DWORD InDW )
	{
		return ((InDW + 1)& ~1);
	}

	// Read headers and load all info pointers in WaveModInfo. 
	// Returns 0 if invalid data encountered.
	// UBOOL ReadWaveInfo( TArray<BYTE>& WavData );
	UBOOL ReadWaveInfo( TArray<BYTE>& WavData );
	
	// Handle RESIZING and updating of all variables needed for the new size:
	// notably the (possibly multiple) loop structures.
	UBOOL UpdateWaveData( TArray<BYTE>& WavData);

	// Wave size and/or bitdepth reduction.
	void Reduce16to8();
	void HalveData();
	void HalveReduce16to8(); 

	// Filters.
	void NoiseGateFilter(); 
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
