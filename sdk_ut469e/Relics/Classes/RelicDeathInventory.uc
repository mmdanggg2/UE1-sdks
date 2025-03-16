class RelicDeathInventory expands RelicInventory;

#exec MESH IMPORT MESH=RelicSkull ANIVFILE=MODELS\RelicSkull_a.3d DATAFILE=MODELS\RelicSkull_d.3d X=0 Y=0 Z=0 MLOD=0.5
#exec MESH ORIGIN MESH=RelicSkull X=0 Y=0 Z=0
#exec MESH SEQUENCE MESH=RelicSkull SEQ=All                      STARTFRAME=0 NUMFRAMES=30
#exec MESH SEQUENCE MESH=RelicSkull SEQ=bob                      STARTFRAME=0 NUMFRAMES=30
#exec MESHMAP NEW   MESHMAP=RelicSkull MESH=RelicSkull
#exec MESHMAP SCALE MESHMAP=RelicSkull X=0.1 Y=0.1 Z=0.2
#exec TEXTURE IMPORT NAME=JRelicSkull_01 FILE=Textures\RelicSkull.PCX GROUP=Skins //Material #1
#exec MESHMAP SETTEXTURE MESHMAP=RelicSkull NUM=1 TEXTURE=JRelicSkull_01

function PostBeginPlay()
{
	Super.PostBeginPlay();

	LoopAnim('Bob', 0.5);
}

function DropInventory()
{
	if ( Pawn(Owner).Health > 0 )
		Super.DropInventory();
	else
		Destroy();
}

function Destroyed()
{
	local Pawn Victim;
	local DeathWave DW;

	Victim = Pawn(Owner);

	if ( (Victim != None) && (Victim.Health <= 0) )
	{
	 	DW = Spawn(class'DeathWave', , , Victim.Location + vect(0,0,50), Victim.Rotation);
		DW.Instigator = Victim;
	}
	Super.Destroyed();
}

auto state Pickup
{
	function Landed(Vector HitNormal)
	{
		Super.Landed(HitNormal);
		LoopAnim('Bob', 0.5);
	}
}

defaultproperties
{
	Physics=PHYS_Rotating
	PickupViewScale=0.5
	Texture=texture'JDomN0'
	Mesh=mesh'RelicSkull'
	PickupViewMesh=mesh'RelicSkull'
	PickupMessage="You picked up the Relic of Vengeance!"
    CollisionRadius=22.000000
    CollisionHeight=55.000000
    bFixedRotationDir=True
    RotationRate=(Yaw=5000,Pitch=0,Roll=0)
    DesiredRotation=(Yaw=30000,Pitch=0,Roll=0)
	Icon=texture'RelicIconVengeance'
}
