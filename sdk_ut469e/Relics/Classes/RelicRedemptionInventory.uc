class RelicRedemptionInventory expands RelicInventory;

#exec MESH IMPORT MESH=RelicRedemption ANIVFILE=MODELS\RelicRedemption_a.3d DATAFILE=MODELS\RelicRedemption_d.3d X=0 Y=0 Z=0 MLOD=0.5
#exec MESH ORIGIN MESH=RelicRedemption X=0 Y=0 Z=0
#exec MESH SEQUENCE MESH=RelicRedemption SEQ=All                      STARTFRAME=0 NUMFRAMES=1
#exec MESH SEQUENCE MESH=RelicRedemption SEQ=base                     STARTFRAME=0 NUMFRAMES=1
#exec MESHMAP NEW   MESHMAP=RelicRedemption MESH=RelicRedemption
#exec MESHMAP SCALE MESHMAP=RelicRedemption X=0.1 Y=0.1 Z=0.2

#exec TEXTURE IMPORT NAME=JRelicRedemption FILE=Textures\RelicRedemption.PCX GROUP=Skins

function inventory PrioritizeArmor( int Damage, name DamageType, vector HitLocation )
{
	local int ArmorDamage, PointNumber, PointCount;
	local Pawn Victim;
	local NavigationPoint NP;

	Victim = Pawn(Owner);
	if ( (Victim == None) || (Victim.Health - Damage > 0) )
		return Super.PrioritizeArmor(Damage, DamageType, HitLocation);

	// Redeem this poor soul.
	PointNumber = Rand(MyRelic.NumPoints);
	for (NP = Level.NavigationPointList; NP != None; NP = NP.NextNavigationPoint)
	{
		if (NP.IsA('PathNode'))
		{
			if (PointCount == PointNumber)
			{
				if ( (Victim.PlayerReplicationInfo.HasFlag != None)
					&& Victim.PlayerReplicationInfo.HasFlag.IsA('CTFFlag')  )
					Victim.PlayerReplicationInfo.HasFlag.Drop(vect(0,0,0));

				Spawn(class'RelicSpawnEffect', Victim,, Victim.Location, Victim.Rotation);

				Victim.SetLocation(NP.Location);
				if ( Victim.IsA('PlayerPawn') )
					PlayerPawn(Victim).SetFOVAngle(170);

				Victim.Health = 100;
				Victim.AddVelocity(vect(0,0,-1000));
			}
			PointCount++;
		}
	}

	// Move the relic.
	Victim.DeleteInventory(self);
	Destroy();
	NextArmor = None;
	Return self;
}

//
// Absorb damage.
//
function int ArmorAbsorbDamage(int Damage, name DamageType, vector HitLocation)
{
	if ( Pawn(Owner) != None )
		Pawn(Owner).Health = 100;
	return 0;
}

defaultproperties
{
	Physics=PHYS_Rotating
	PickupViewScale=0.6
	Skin=texture'JRelicRedemption'
	PickupViewMesh=mesh'RelicRedemption'
	PickupMessage="You picked up the Relic of Redemption!"
    CollisionRadius=22.000000
    CollisionHeight=40.000000
	Icon=texture'RelicIconRedemption'
}
