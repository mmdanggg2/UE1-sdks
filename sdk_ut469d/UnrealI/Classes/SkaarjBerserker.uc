//=============================================================================
// SkaarjBerserker.
//=============================================================================
class SkaarjBerserker extends SkaarjWarrior;

#exec TEXTURE IMPORT NAME=Skaarjw2 FILE=MODELS\Skar1.PCX GROUP=Skins

function WhatToDoNext(name LikelyState, name LikelyLabel)
{
	local Pawn aPawn;

	if ( Health >0 && !bDeleteme)
	{
		aPawn = Level.PawnList;
		while ( aPawn != None )
		{
			if ( AttitudeToCreature(aPawn) < ATTITUDE_Ignore
					&& (aPawn.IsA('PlayerPawn') || (aPawn.IsA('ScriptedPawn') && aPawn!=self))
					&& (VSize(Location - aPawn.Location) < 500)
					&& CanSee(aPawn) )
			{
				if ( SetEnemy(aPawn) )
				{
					GotoState('Attacking');
					return;
				}
			}
			aPawn = aPawn.nextPawn;
		}
	}
	Super.WhatToDoNext(LikelyState, LikelyLabel);
}

function eAttitude AttitudeToCreature(Pawn Other)
{
	if ( Other==self)
		return ATTITUDE_Friendly;
	else if ( Other.IsA('Pupae') )
		return ATTITUDE_Ignore;
	else
		return ATTITUDE_Hate;
}

defaultproperties
{
     voicePitch=+00000.300000
     LungeDamage=40
     SpinDamage=40
     ClawDamage=20
	 CombatStyle=+00001.000000
     Aggressiveness=+00000.800000
     Health=320
     Skill=+00001.000000
	 DrawScale=+00001.200000
	 Fatness=150
     Skin=Skaarjw2
     CollisionHeight=+00056.000000
     Mass=+00180.000000
     Buoyancy=+00180.000000
     RotationRate=(Yaw=50000)
}
