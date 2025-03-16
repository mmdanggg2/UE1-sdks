class RelicDefenseInventory expands RelicInventory;

#exec MESH IMPORT MESH=RelicHelmet ANIVFILE=MODELS\RelicHelmet_a.3d DATAFILE=MODELS\RelicHelmet_d.3d X=0 Y=0 Z=0
#exec MESH ORIGIN MESH=RelicHelmet X=0 Y=0 Z=0
#exec MESH SEQUENCE MESH=RelicHelmet SEQ=All                      STARTFRAME=0 NUMFRAMES=1
#exec MESH SEQUENCE MESH=RelicHelmet SEQ=base                     STARTFRAME=0 NUMFRAMES=1
#exec MESHMAP NEW   MESHMAP=RelicHelmet MESH=RelicHelmet
#exec MESHMAP SCALE MESHMAP=RelicHelmet X=0.1 Y=0.1 Z=0.2
#exec TEXTURE IMPORT NAME=JRelicHelmet FILE=Textures\RelicHelmet.PCX GROUP=Skins //Material #1
#exec MESHMAP SETTEXTURE MESHMAP=RelicHelmet NUM=0 TEXTURE=JRelicHelmet

function bool HandlePickupQuery( inventory Item )
{
	local Inventory I;

	if (item.IsA('UT_Shieldbelt') ) 
		return true; //don't allow shieldbelt if have defense relic

	return Super.HandlePickupQuery(Item);
}

function PickupFunction(Pawn Other)
{
	local Inventory I;
	
	Super.PickupFunction(Other);

	// remove other armors
	for ( I=Owner.Inventory; I!=None; I=I.Inventory )
		if ( I.IsA('UT_Shieldbelt') )
			I.Destroy();
}

function ArmorImpactEffect(vector HitLocation)
{ 
	if ( Owner.IsA('PlayerPawn') )
	{
		PlayerPawn(Owner).ClientFlash(-0.05,vect(400,400,400));
	}
	Owner.PlaySound(DeActivateSound, SLOT_None, 2.7*Pawn(Owner).SoundDampening);
	FlashShell(0.4);
}

//
// Absorb damage.
//
function int ArmorAbsorbDamage(int Damage, name DamageType, vector HitLocation)
{
	ArmorImpactEffect(HitLocation);
	return 0.4 * Damage;
}

//
// Return armor value.
//
function int ArmorPriority(name DamageType)
{
	return 1;  // very low absorption priority (only if no other armor left
}

defaultproperties
{
	bIsAnArmor=true
	Physics=PHYS_Rotating
	PickupViewScale=0.6
	PickupViewMesh=mesh'RelicHelmet'
	Skin=texture'JRelicHelmet'
	PickupMessage="You picked up the Relic of Defense!"
	ShellSkin=texture'RelicGreen'
    CollisionRadius=22.000000
    CollisionHeight=40.000000
    LightHue=100
    LightSaturation=0
	LightBrightness=200
    bFixedRotationDir=True
    RotationRate=(Yaw=6000,Pitch=0,Roll=0)
    DesiredRotation=(Yaw=30000,Pitch=0,Roll=0)
	Icon=texture'RelicIconDefense'
}
