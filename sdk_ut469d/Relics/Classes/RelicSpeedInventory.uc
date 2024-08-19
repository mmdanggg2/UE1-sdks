class RelicSpeedInventory expands RelicInventory;

#exec MESH IMPORT MESH=RelicHourglass ANIVFILE=MODELS\RelicHourglass_a.3d DATAFILE=MODELS\RelicHourglass_d.3d X=0 Y=0 Z=0 MLOD=0.5
#exec MESH ORIGIN MESH=RelicHourglass X=0 Y=0 Z=0
#exec MESH SEQUENCE MESH=RelicHourglass SEQ=All                      STARTFRAME=0 NUMFRAMES=1
#exec MESH SEQUENCE MESH=RelicHourglass SEQ=base                     STARTFRAME=0 NUMFRAMES=1
#exec MESHMAP NEW   MESHMAP=RelicHourglass MESH=RelicHourglass
#exec MESHMAP SCALE MESHMAP=RelicHourglass X=0.1 Y=0.1 Z=0.2
#exec TEXTURE IMPORT NAME=JRelicHourglass_01 FILE=Textures\RelicHourglass.PCX GROUP=Skins //Material #2
#exec MESHMAP SETTEXTURE MESHMAP=RelicHourglass NUM=1 TEXTURE=JRelicHourglass_01

#exec AUDIO IMPORT FILE="Sounds\SpeedWind.WAV" NAME="SpeedWind" GROUP="Relics"

function PickupFunction(Pawn Other)
{
	Super.PickupFunction(Other);

	ShellEffect = Spawn(ShellType, Owner,,Owner.Location, Owner.Rotation); 
}

state Activated
{
	function BeginState()
	{
		SetTimer(0.2, True);

		Super.BeginState();

		// Alter player's stats.
		Pawn(Owner).AirControl = 0.65;
		Pawn(Owner).JumpZ *= 1.1;
		Pawn(Owner).GroundSpeed *= 1.3;
		Pawn(Owner).WaterSpeed *= 1.3;
		Pawn(Owner).AirSpeed *= 1.3;
		Pawn(Owner).Acceleration *= 1.3;

		// Add wind blowing.
		Pawn(Owner).AmbientSound = sound'SpeedWind';
		Pawn(Owner).SoundRadius = 64;
	}

	function EndState()
	{
		local float SpeedScale;
		SetTimer(0.0, False);

		Super.EndState();

		if ( Level.Game.IsA('DeathMatchPlus') && DeathMatchPlus(Level.Game).bMegaSpeed )
			SpeedScale = 1.3;
		else
			SpeedScale = 1.0;

		// Restore player's stats.
		Pawn(Owner).AirControl = DeathMatchPlus(Level.Game).AirControl;
		Pawn(Owner).JumpZ = Pawn(Owner).Default.JumpZ * Level.Game.PlayerJumpZScaling();
		Pawn(Owner).GroundSpeed = Pawn(Owner).Default.GroundSpeed * SpeedScale;
		Pawn(Owner).WaterSpeed = Pawn(Owner).Default.WaterSpeed * SpeedScale;
		Pawn(Owner).AirSpeed = Pawn(Owner).Default.AirSpeed * SpeedScale;
		Pawn(Owner).Acceleration = Pawn(Owner).Default.Acceleration * SpeedScale;

		// Remove sound.
		Pawn(Owner).AmbientSound = None;
	}
}

defaultproperties
{
	ShellType=class'SpeedShell'
	Physics=PHYS_Rotating
	PickupViewScale=0.5
	PickupViewMesh=mesh'RelicHourglass'
	PickupMessage="You picked up the Relic of Speed!"
	Skin=texture'JRelicHourglass_01'
    CollisionRadius=22.000000
    CollisionHeight=40.000000
	Icon=texture'RelicIconSpeed'
}
