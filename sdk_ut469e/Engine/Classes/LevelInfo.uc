//=============================================================================
// LevelInfo contains information about the current level. There should
// be one per level and it should be actor 0. UnrealEd creates each level's
// LevelInfo automatically so you should never have to place one
// manually.
//
// The ZoneInfo properties in the LevelInfo are used to define
// the properties of all zones which don't themselves have ZoneInfo.
//=============================================================================
class LevelInfo extends ZoneInfo
	native
	nativereplication;

// Textures.
#exec Texture Import File=Textures\DefaultTexture.pcx

//-----------------------------------------------------------------------------
// Level time.

// Time passage.
var() float TimeDilation;          // Normally 1 - scales real time passage.

// Current time.
var           float	TimeSeconds;   // Time in seconds since level began play.
var transient int   Year;          // Year.
var transient int   Month;         // Month.
var transient int   Day;           // Day of month.
var transient int   DayOfWeek;     // Day of week.
var transient int   Hour;          // Hour.
var transient int   Minute;        // Minute.
var transient int   Second;        // Second.
var transient int   Millisecond;   // Millisecond.

//-----------------------------------------------------------------------------
// Text info about level.

var() localized string Title;
var()           string Author;		    // Who built it.
var() localized string IdealPlayerCount;// Ideal number of players for this level. I.E.: 6-8
var() int	RecommendedEnemies;			// number of enemy bots recommended (used by rated games)
var() int	RecommendedTeammates;		// number of friendly bots recommended (used by rated games)
var() localized string LevelEnterText;  // Message to tell players when they enter.
var()           string LocalizedPkg;    // Package to look in for localizations.
var             string Pauser;          // If paused, name of person pausing the game.
var levelsummary Summary;
var           string VisibleGroups;		    // List of the group names which were checked when the level was last saved

//-----------------------------------------------------------------------------
// Flags affecting the level.

var() bool           bLonePlayer;     // No multiplayer coordination, i.e. for entranceways.
var bool             bBegunPlay;      // Whether gameplay has begun.
var bool             bPlayersOnly;    // Only update players.
var bool             bHighDetailMode; // Client high-detail mode.
var bool			 bDropDetail;	  // frame rate is below DesiredFrameRate, so drop high detail actors
var bool			 bAggressiveLOD;  // frame rate is well below DesiredFrameRate, so make LOD more aggressive
var bool             bStartup;        // Starting gameplay.
var() bool			 bHumansOnly;	  // Only allow "human" player pawns in this level
var bool			 bNoCheating;
var bool			 bAllowFOV;
var config bool		 bLowRes;		  // optimize for low resolution (e.g. TV)

//-----------------------------------------------------------------------------
// Audio properties.

var(Audio) const music  Song;          // Default song for level.
var(Audio) const byte   SongSection;   // Default song order for level.
var(Audio) const byte   CdTrack;       // Default CD track for level.
var(Audio) float        PlayerDoppler; // Player doppler shift, 0=none, 1=full.

//-----------------------------------------------------------------------------
// Miscellaneous information.

var() float Brightness;
var() texture Screenshot;
var texture DefaultTexture;
var int HubStackLevel;
var transient enum ELevelAction
{
	LEVACT_None,
	LEVACT_Loading,
	LEVACT_Saving,
	LEVACT_Connecting,
	LEVACT_Precaching
} LevelAction;

//-----------------------------------------------------------------------------
// Renderer Management.
var() bool bNeverPrecache;

//-----------------------------------------------------------------------------
// Networking.

var enum ENetMode
{
	NM_Standalone,        		// Standalone game.
	NM_DedicatedServer,   		// Dedicated server, no local client.
	NM_ListenServer,      		// Listen server.
	NM_Client             		// Client only, no local server.
} NetMode;
var string ComputerName;  		// Machine's name according to the OS.
var string EngineVersion; 		// Engine version. 
var string MinNetVersion; 		// Min engine version that is net compatible.
//
// OldUnreal: Max supported ServerMove protocol
//
// Version 0:
// * Original v400-451b movement
//
// Version 1:
// * Used in 469 betas + 469a
// * Added missing replay of the current pending move after getting a ClientAdjustPosition call. This caused constant desync at 90+ fps
// * Added mergecount to movement packets. This allows the server to replay client movement with roughly the same steps the client used prior to merging
// * Send uncompressed acceleration components
// * Do not force client resync if the position error is small
// * Account for gamespeed when calculating resync interval
// * Use a minimum netspeed of 10000 when calculating resync interval
// * Smooth position adjustments for minor desync errors
// * Replicate pending move immediately if (a) updated accel is significantly different from accel in pending move, (b) player has just (alt)fired, jumped, or started a dodge.
// * Fixed dodgedir/dodgeclicktimer not resetting when receiving a position adjustment in the middle of an active dodge move
// * Implemented invisible collision fix for PHYS_Falling
// * Fixed APawn::physLedgeAdjust bugs that caused bots to fail at walking over certain ledges
// * APawn::stepUp no longer applies an absolute adjustment. This fixes ridiculous acceleration boosts at high fps
//
// Version 2:
// * Rewrote the invisible collision fix because it still failed due to substantial floating point rounding errors that occured on slopes that were far away from the center of the map
//
// Version 3:
// * Capped the duration of the initial boost in physFalling to 0.016 seconds
//
var int    ServerMoveVersion;	
var string EngineRevision;      // OldUnreal: Revision of this engine version.
var string EngineArchitecture;  // OldUnreal: CPU Architecture for this engine.

//-----------------------------------------------------------------------------
// Gameplay rules

var() class<gameinfo> DefaultGameType;
var GameInfo Game;

//-----------------------------------------------------------------------------
// Navigation point and Pawn lists (chained using nextNavigationPoint and nextPawn).

var const NavigationPoint NavigationPointList;
var const Pawn PawnList;

//-----------------------------------------------------------------------------
// Server related.

var string NextURL;
var bool bNextItems;
var float NextSwitchCountdown;

//-----------------------------------------------------------------------------
// Actor Performance Management

var int AIProfile[8]; // TEMP statistics
var float AvgAITime;	//moving average of Actor time

//-----------------------------------------------------------------------------
// Physics control

var() bool bCheckWalkSurfaces; // enable texture-specific physics code for Pawns.

//-----------------------------------------------------------------------------
// Spawn notification list
var SpawnNotify SpawnNotify;

//-----------------------------------------------------------------------------
// Functions.

//
// Return the URL of this level on the local machine.
//
native simulated function string GetLocalURL();

//
// Return the URL of this level, which may possibly
// exist on a remote machine.
//
native simulated function string GetAddressURL();

//
// Jump the server to a new level.
//
event ServerTravel( string URL, bool bItems )
{
	if( NextURL=="" )
	{
		SetTimer(0.0,False);
		bNextItems          = bItems;
		NextURL             = URL;
		if( Game!=None )
			Game.ProcessServerTravel( URL, bItems );
		else
			NextSwitchCountdown = 0;
	}
}

event BeginPlay()
{
	ServerMoveVersion = 3;
	SetupTimer();
}

function SetupTimer()
{
	if (NetMode == NM_DedicatedServer || NetMode == NM_ListenServer)
		SetTimer(TimeDilation,True);
}

event Timer()
{
	if (Pauser != "" && Game != None)
		Game.SentText = 0;
}

//-----------------------------------------------------------------------------
// Network replication.

replication
{
	reliable if( Role==ROLE_Authority )
		Pauser, TimeDilation, bNoCheating, bAllowFOV, ServerMoveVersion;
}

defaultproperties
{
     TimeDilation=+00001.000000
	 Brightness=1
     Title="Untitled"
     bHiddenEd=True
	 CdTrack=255
	 DefaultTexture=DefaultTexture
	 HubStackLevel=0
	 bHighDetailMode=True
	 PlayerDoppler=0
	 VisibleGroups="None"
}
