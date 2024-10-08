//=============================================================================
// TFemale2.
//=============================================================================
class TFemale2 extends TournamentFemale;

#exec MESH IMPORT MESH=SGirl ANIVFILE=MODELS\tfemale2_a.3D DATAFILE=MODELS\tfemale2_d.3D UNMIRROR=1 LODSTYLE=12
#exec MESH ORIGIN MESH=SGirl X=-30 Y=-115 Z=20 YAW=64 ROLL=-64

#exec MESH SEQUENCE MESH=SGirl SEQ=All       STARTFRAME=0   NUMFRAMES=681 
#exec MESH SEQUENCE MESH=SGirl SEQ=GutHit    STARTFRAME=0   NUMFRAMES=1				Group=TakeHit
#exec MESH SEQUENCE MESH=SGirl SEQ=AimDnLg   STARTFRAME=1   NUMFRAMES=1				Group=Waiting
#exec MESH SEQUENCE MESH=SGirl SEQ=AimDnSm   STARTFRAME=2   NUMFRAMES=1				Group=Waiting
#exec MESH SEQUENCE MESH=SGirl SEQ=AimUpLg   STARTFRAME=3   NUMFRAMES=1				Group=Waiting
#exec MESH SEQUENCE MESH=SGirl SEQ=AimUpSm   STARTFRAME=4   NUMFRAMES=1				Group=Waiting
#exec MESH SEQUENCE MESH=SGirl SEQ=Taunt1    STARTFRAME=5   NUMFRAMES=15 RATE=20	Group=Gesture
#exec MESH SEQUENCE MESH=SGirl SEQ=Breath1   STARTFRAME=20  NUMFRAMES=7  RATE=6		Group=Waiting
#exec MESH SEQUENCE MESH=SGirl SEQ=Breath2   STARTFRAME=27  NUMFRAMES=20 RATE=7		Group=Waiting
#exec MESH SEQUENCE MESH=SGirl SEQ=CockGun   STARTFRAME=47  NUMFRAMES=6  RATE=6		Group=Waiting
#exec MESH SEQUENCE MESH=SGirl SEQ=DuckWlkL  STARTFRAME=53  NUMFRAMES=15 RATE=15	Group=Ducking
#exec MESH SEQUENCE MESH=SGirl SEQ=DuckWlkS  STARTFRAME=68  NUMFRAMES=15 RATE=15	Group=Ducking
#exec MESH SEQUENCE MESH=SGirl SEQ=HeadHit   STARTFRAME=83  NUMFRAMES=1				Group=TakeHit
#exec MESH SEQUENCE MESH=SGirl SEQ=JumpLgFr  STARTFRAME=84  NUMFRAMES=1				Group=Jumping
#exec MESH SEQUENCE MESH=SGirl SEQ=JumpSmFr  STARTFRAME=85  NUMFRAMES=1				Group=Jumping
#exec MESH SEQUENCE MESH=SGirl SEQ=LandLgFr  STARTFRAME=86  NUMFRAMES=1				Group=Landing
#exec MESH SEQUENCE MESH=SGirl SEQ=LandSmFr  STARTFRAME=87  NUMFRAMES=1				Group=Landing
#exec MESH SEQUENCE MESH=SGirl SEQ=LeftHit   STARTFRAME=88  NUMFRAMES=1				Group=TakeHit
#exec MESH SEQUENCE MESH=SGirl SEQ=Look      STARTFRAME=89  NUMFRAMES=35 RATE=15    Group=Waiting 
#exec MESH SEQUENCE MESH=SGirl SEQ=RightHit  STARTFRAME=124 NUMFRAMES=1				Group=TakeHit
#exec MESH SEQUENCE MESH=SGirl SEQ=RunLg     STARTFRAME=125 NUMFRAMES=10 RATE=17
#exec MESH SEQUENCE MESH=SGirl SEQ=RunLgFr   STARTFRAME=135 NUMFRAMES=10 RATE=17    Group=MovingFire
#exec MESH SEQUENCE MESH=SGirl SEQ=RunSm     STARTFRAME=145 NUMFRAMES=10 RATE=17
#exec MESH SEQUENCE MESH=SGirl SEQ=RunSmFr   STARTFRAME=155 NUMFRAMES=10 RATE=17	Group=MovingFire
#exec MESH SEQUENCE MESH=SGirl SEQ=StillFrRp STARTFRAME=165 NUMFRAMES=10 RATE=15	Group=Waiting
#exec MESH SEQUENCE MESH=SGirl SEQ=StillLgFr STARTFRAME=175 NUMFRAMES=10 RATE=15	Group=Waiting
#exec MESH SEQUENCE MESH=SGirl SEQ=StillSmFr STARTFRAME=185 NUMFRAMES=8  RATE=15	Group=Waiting
#exec MESH SEQUENCE MESH=SGirl SEQ=SwimLg    STARTFRAME=193 NUMFRAMES=18 RATE=15
#exec MESH SEQUENCE MESH=SGirl SEQ=SwimSm    STARTFRAME=211 NUMFRAMES=18 RATE=15
#exec MESH SEQUENCE MESH=SGirl SEQ=TreadLg   STARTFRAME=229 NUMFRAMES=15 RATE=15	Group=Waiting
#exec MESH SEQUENCE MESH=SGirl SEQ=TreadSm   STARTFRAME=244 NUMFRAMES=15 RATE=15	Group=Waiting
#exec MESH SEQUENCE MESH=SGirl SEQ=Victory1  STARTFRAME=259 NUMFRAMES=10 RATE=6  	Group=Gesture
#exec MESH SEQUENCE MESH=SGirl SEQ=WalkLg    STARTFRAME=269 NUMFRAMES=15 RATE=18
#exec MESH SEQUENCE MESH=SGirl SEQ=WalkLgFr  STARTFRAME=284 NUMFRAMES=15 RATE=18	Group=MovingFire
#exec MESH SEQUENCE MESH=SGirl SEQ=WalkSm    STARTFRAME=299 NUMFRAMES=15 RATE=18
#exec MESH SEQUENCE MESH=SGirl SEQ=WalkSmFr  STARTFRAME=314 NUMFRAMES=15 RATE=18	Group=MovingFire
#exec MESH SEQUENCE MESH=SGirl SEQ=Wave      STARTFRAME=329 NUMFRAMES=15 RATE=15	Group=Gesture
#exec MESH SEQUENCE MESH=SGirl SEQ=Walk      STARTFRAME=344 NUMFRAMES=15 RATE=18
#exec MESH SEQUENCE MESH=SGirl SEQ=TurnLg    STARTFRAME=284 NUMFRAMES=2  RATE=15					// 2 frames of walklgfr
#exec MESH SEQUENCE MESH=SGirl SEQ=TurnSm    STARTFRAME=314 NUMFRAMES=2  RATE=15					// 2 frames of walksmfr
#exec MESH SEQUENCE MESH=SGirl SEQ=Breath1L  STARTFRAME=359 NUMFRAMES=7  RATE=6		Group=Waiting
#exec MESH SEQUENCE MESH=SGirl SEQ=Breath2L  STARTFRAME=366 NUMFRAMES=20 RATE=7		Group=Waiting
#exec MESH SEQUENCE MESH=SGirl SEQ=CockGunL  STARTFRAME=386 NUMFRAMES=6  RATE=6		Group=Waiting
#exec MESH SEQUENCE MESH=SGirl SEQ=LookL     STARTFRAME=392 NUMFRAMES=35 RATE=15    Group=Waiting   
#exec MESH SEQUENCE MESH=SGirl SEQ=WaveL     STARTFRAME=427 NUMFRAMES=15 RATE=15	Group=Gesture
#exec MESH SEQUENCE MESH=SGirl SEQ=Chat1     STARTFRAME=442 NUMFRAMES=13 RATE=6		Group=Waiting
#exec MESH SEQUENCE MESH=SGirl SEQ=Chat2     STARTFRAME=455 NUMFRAMES=10 RATE=6		Group=Waiting
#exec MESH SEQUENCE MESH=SGirl SEQ=Thrust    STARTFRAME=465 NUMFRAMES=15 RATE=20	Group=Gesture
#exec MESH SEQUENCE MESH=SGirl SEQ=DodgeB    STARTFRAME=480 NUMFRAMES=1				Group=Jumping  
#exec MESH SEQUENCE MESH=SGirl SEQ=DodgeF    STARTFRAME=481 NUMFRAMES=1				Group=Jumping
#exec MESH SEQUENCE MESH=SGirl SEQ=DodgeR    STARTFRAME=482 NUMFRAMES=1				Group=Jumping
#exec MESH SEQUENCE MESH=SGirl SEQ=DodgeL    STARTFRAME=483 NUMFRAMES=1				Group=Jumping
#exec MESH SEQUENCE MESH=SGirl SEQ=Fighter   STARTFRAME=175 NUMFRAMES=1								// first frame of stilllgfr
#exec MESH SEQUENCE MESH=SGirl SEQ=Flip      STARTFRAME=484 NUMFRAMES=31			Group=Jumping
#exec MESH SEQUENCE MESH=SGirl SEQ=Dead2     STARTFRAME=515 NUMFRAMES=18 RATE=10	Group=TakeHit
#exec MESH SEQUENCE MESH=SGirl SEQ=Dead3     STARTFRAME=533 NUMFRAMES=16 RATE=10	Group=TakeHit
#exec MESH SEQUENCE MESH=SGirl SEQ=Dead4     STARTFRAME=549 NUMFRAMES=13 RATE=10	Group=TakeHit
#exec MESH SEQUENCE MESH=SGirl SEQ=Dead6     STARTFRAME=562 NUMFRAMES=11 RATE=10

#exec MESH SEQUENCE MESH=SGirl SEQ=DeathEnd  STARTFRAME=532 NUMFRAMES=1
#exec MESH SEQUENCE MESH=SGirl SEQ=DeathEnd2 STARTFRAME=561 NUMFRAMES=1
#exec MESH SEQUENCE MESH=SGirl SEQ=DeathEnd3 STARTFRAME=572 NUMFRAMES=1

#exec MESH SEQUENCE MESH=SGirl SEQ=Dead1     STARTFRAME=573 NUMFRAMES=23 RATE=12
#exec MESH SEQUENCE MESH=SGirl SEQ=Dead5     STARTFRAME=596 NUMFRAMES=13 RATE=12	
#exec MESH SEQUENCE MESH=SGirl SEQ=Dead9     STARTFRAME=609 NUMFRAMES=20 RATE=12
#exec MESH SEQUENCE MESH=SGirl SEQ=Dead9B    STARTFRAME=629 NUMFRAMES=11 RATE=12
#exec MESH SEQUENCE MESH=SGirl SEQ=Dead7     STARTFRAME=640 NUMFRAMES=11 RATE=12
#exec MESH SEQUENCE MESH=SGirl SEQ=BackRun   STARTFRAME=651 NUMFRAMES=10 RATE=17		Group=MovingFire
#exec MESH SEQUENCE MESH=SGirl SEQ=StrafeL   STARTFRAME=661 NUMFRAMES=10 RATE=17		Group=MovingFire
#exec MESH SEQUENCE MESH=SGirl SEQ=StrafeR   STARTFRAME=671 NUMFRAMES=10 RATE=17		Group=MovingFire

#exec MESHMAP SCALE MESHMAP=SGirl X=0.0625 Y=0.0625 Z=0.125

#exec MESH NOTIFY MESH=SGirl SEQ=RunLG TIME=0.25 FUNCTION=PlayFootStep
#exec MESH NOTIFY MESH=SGirl SEQ=RunLG TIME=0.75 FUNCTION=PlayFootStep
#exec MESH NOTIFY MESH=SGirl SEQ=RunLGFR TIME=0.25 FUNCTION=PlayFootStep
#exec MESH NOTIFY MESH=SGirl SEQ=RunLGFR TIME=0.75 FUNCTION=PlayFootStep
#exec MESH NOTIFY MESH=SGirl SEQ=RunSM TIME=0.25 FUNCTION=PlayFootStep
#exec MESH NOTIFY MESH=SGirl SEQ=RunSM TIME=0.75 FUNCTION=PlayFootStep
#exec MESH NOTIFY MESH=SGirl SEQ=RunSMFR TIME=0.25 FUNCTION=PlayFootStep
#exec MESH NOTIFY MESH=SGirl SEQ=RunSMFR TIME=0.75 FUNCTION=PlayFootStep
#exec MESH NOTIFY MESH=SGirl SEQ=StrafeL TIME=0.25 FUNCTION=PlayFootStep
#exec MESH NOTIFY MESH=SGirl SEQ=StrafeL TIME=0.75 FUNCTION=PlayFootStep
#exec MESH NOTIFY MESH=SGirl SEQ=StrafeR TIME=0.25 FUNCTION=PlayFootStep
#exec MESH NOTIFY MESH=SGirl SEQ=StrafeR TIME=0.75 FUNCTION=PlayFootStep
#exec MESH NOTIFY MESH=SGirl SEQ=BackRun TIME=0.25 FUNCTION=PlayFootStep
#exec MESH NOTIFY MESH=SGirl SEQ=BackRun TIME=0.75 FUNCTION=PlayFootStep
#exec MESH NOTIFY MESH=SGirl SEQ=Dead2  TIME=0.66 FUNCTION=LandThump
#exec MESH NOTIFY MESH=SGirl SEQ=Dead3  TIME=0.38 FUNCTION=LandThump
#exec MESH NOTIFY MESH=SGirl SEQ=Dead4  TIME=0.46 FUNCTION=LandThump
#exec MESH NOTIFY MESH=SGirl SEQ=Dead6  TIME=0.54 FUNCTION=LandThump
#exec MESH NOTIFY MESH=SGirl SEQ=Dead1  TIME=0.69 FUNCTION=LandThump
#exec MESH NOTIFY MESH=SGirl SEQ=Dead5  TIME=0.61 FUNCTION=LandThump
#exec MESH NOTIFY MESH=SGirl SEQ=Dead9b TIME=0.45 FUNCTION=LandThump
#exec MESH NOTIFY MESH=SGirl SEQ=Dead7  TIME=0.54 FUNCTION=LandThump

defaultproperties
{
	Handedness=-1.000000
	Mesh=Mesh'Botpack.SGirl'
	SelectionMesh="Botpack.SelectionFemale2"
	SpecialMesh="Botpack.TrophyFemale2"
	CarcassType=TFemale2Carcass
	Menuname="Female Soldier"
	VoiceType="BotPack.VoiceFemaleTwo"
	TeamSkin1=0
	TeamSkin2=1
	FixedSkin=2
	FaceSkin=3
	DefaultSkinName="SGirlSkins.army"
	DefaultPackage="SGirlSkins."
}
