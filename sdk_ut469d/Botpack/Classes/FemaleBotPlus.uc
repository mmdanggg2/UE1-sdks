//=============================================================================
// FemaleBotPlus.
//=============================================================================
class FemaleBotPlus extends HumanBotPlus
	abstract;

function PlayRightHit(float tweentime)
{
	if ( AnimSequence == 'RightHit' )
		TweenAnim('GutHit', tweentime);
	else
		TweenAnim('RightHit', tweentime);
}	

function PlayChallenge()
{
	TweenToWaiting(0.17);
}

function PlayVictoryDance()
{
	local float decision;

	decision = FRand();

	if ( decision < 0.25 )
		PlayAnim('Victory1',0.7, 0.2);
	else if ( decision < 0.5 )
		PlayAnim('Thrust',0.7, 0.2);
	else if ( decision < 0.75 )
		PlayAnim('Taunt1',0.7, 0.2);
	else
		TweenAnim('Taunt1', 0.2);
}

function PlayDying(name DamageType, vector HitLoc)
{
	local carcass carc;

	BaseEyeHeight = Default.BaseEyeHeight;
	PlayDyingSound();
			
	if ( DamageType == 'Suicided' )
	{
		PlayAnim('Dead3',, 0.1);
		return;
	}

	// check for head hit
	if ( (DamageType == 'Decapitated') && !Level.Game.bVeryLowGore )
	{
		PlayDecap();
		return;
	}

	if ( FRand() < 0.15 )
	{
		PlayAnim('Dead7',,0.1);
		return;
	}

	// check for big hit
	if ( (Velocity.Z > 250) && (FRand() < 0.75) )
	{
		if ( (HitLoc.Z < Location.Z) && !Level.Game.bVeryLowGore && (FRand() < 0.6) )
		{
			PlayAnim('Dead5',,0.05);
			if ( Level.NetMode != NM_Client )
			{
				carc = Spawn(class 'UT_FemaleFoot',,, Location - CollisionHeight * vect(0,0,0.5));
				if (carc != None)
				{
					carc.Initfor(self);
					carc.Velocity = Velocity + VSize(Velocity) * VRand();
					carc.Velocity.Z = FMax(carc.Velocity.Z, Velocity.Z);
				}
			}
		}
		else
			PlayAnim('Dead2',, 0.1);
		return;
	}

	// check for repeater death
	if ( (Health > -10) && ((DamageType == 'shot') || (DamageType == 'zapped')) )
	{
		PlayAnim('Dead9',, 0.1);
		return;
	}
		
	if ( (HitLoc.Z - Location.Z > 0.7 * CollisionHeight) && !Level.Game.bVeryLowGore )
	{
		if ( FRand() < 0.5 )
			PlayDecap();
		else
			PlayAnim('Dead3',, 0.1);
		return;
	}
	
	//then hit in front or back	
	if ( FRand() < 0.5 ) 
		PlayAnim('Dead4',, 0.1);
	else
		PlayAnim('Dead1',, 0.1);
}

function PlayDecap()
{
	local carcass carc;

	PlayAnim('Dead6',, 0.1);
	if ( Level.NetMode != NM_Client )
	{
		carc = Spawn(class 'UT_HeadFemale',,, Location + CollisionHeight * vect(0,0,0.8), Rotation + rot(3000,0,16384) );
		if (carc != None)
		{
			carc.Initfor(self);
			carc.Velocity = Velocity + VSize(Velocity) * VRand();
			carc.Velocity.Z = FMax(carc.Velocity.Z, Velocity.Z);
		}
	}
}
	
defaultproperties
{
     drown=mdrown2fem
     breathagain=Botpack.FemaleSounds.hgasp3
     HitSound3=Botpack.FemaleSounds.linjur4
     HitSound4=Botpack.FemaleSounds.hinjur4
	 Die=Botpack.FemaleSounds.death1d
	 Deaths(0)=Botpack.FemaleSounds.death1d
	 Deaths(1)=Botpack.FemaleSounds.death2a
	 Deaths(2)=Botpack.FemaleSounds.death3c
	 Deaths(3)=Botpack.FemaleSounds.decap01
	 Deaths(4)=Botpack.FemaleSounds.death41
	 Deaths(5)=Botpack.FemaleSounds.death42
	 GaspSound=Botpack.FemaleSounds,lgasp1
	 JumpSound=Botpack.FemaleSounds.Fjump1
     CarcassType=TFemale1Carcass
     HitSound1=Botpack.FemaleSounds.linjur2
     HitSound2=Botpack.FemaleSounds.linjur3
     LandGrunt=Botpack.FemaleSounds.lland1
	 UWHit1=Botpack.FemaleSounds.UWHit01
	 UWHit2=MUWHit2
	 bIsFemale=true
	 StatusDoll=texture'Botpack.Woman'
	 StatusBelt=texture'Botpack.WomanBelt'
	 VoicePackMetaClass="BotPack.VoiceFemale"
}
