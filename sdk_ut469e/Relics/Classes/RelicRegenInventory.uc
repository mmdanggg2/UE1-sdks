class RelicRegenInventory expands RelicInventory;

#exec MESH IMPORT MESH=RelicRegen ANIVFILE=MODELS\RelicRegen_a.3d DATAFILE=MODELS\RelicRegen_d.3d X=0 Y=0 Z=0 MLOD=0.5
#exec MESH ORIGIN MESH=RelicRegen X=0 Y=0 Z=0
#exec MESH SEQUENCE MESH=RelicRegen SEQ=All                      STARTFRAME=0 NUMFRAMES=1
#exec MESH SEQUENCE MESH=RelicRegen SEQ=base                     STARTFRAME=0 NUMFRAMES=1
#exec MESHMAP NEW   MESHMAP=RelicRegen MESH=RelicRegen
#exec MESHMAP SCALE MESHMAP=RelicRegen X=0.1 Y=0.1 Z=0.2
#exec TEXTURE IMPORT NAME=JRelicRegen_01 FILE=Textures\RelicRegen.PCX GROUP=Skins //Material #1
#exec MESHMAP SETTEXTURE MESHMAP=RelicRegen NUM=1 TEXTURE=JRelicRegen_01

#exec AUDIO IMPORT FILE="Sounds\RegenPulse.WAV" NAME="RegenHiss" GROUP="Relics"

var vector InstFog;
var float InstFlash;

state Activated
{
	function Timer()
	{
		if ( (Owner != None) && Owner.bIsPawn )
		{
			if ( Pawn(Owner).Health < 150 )
			{
				Pawn(Owner).Health = FMin(150, Pawn(Owner).Health + 10);
				FlashShell(0.3);
				if ( Owner.IsA('PlayerPawn') )
				{
					PlayerPawn(Owner).ClientPlaySound(sound'RegenHiss', False);
					PlayerPawn(Owner).ClientInstantFlash(InstFlash, InstFog);
				}
			}
		}

		Super.Timer();
	}

	function BeginState()
	{
		Super.BeginState();
		SetTimer(2.0, True);
	}

	function EndState()
	{
		SetTimer(0.0, False);
		Super.EndState();
	}
}

defaultproperties
{
	Physics=PHYS_Rotating
	PickupViewScale=0.5
	Texture=texture'JRelicRegen_01'
	Skin=texture'JRelicRegen_01'
	PickupMessage="You picked up the Relic of Regeneration!"
	InstFlash=-0.4
    InstFog=(X=475.00000,Y=325.00000,Z=145.00000)
	PickupViewMesh=mesh'RelicRegen'
    CollisionRadius=22.000000
    CollisionHeight=40.000000
	ShellSkin=texture'RelicBlue'
    LightHue=170
    LightSaturation=0
	Icon=texture'RelicIconRegen'
}
