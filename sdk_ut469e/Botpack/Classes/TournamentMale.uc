//=============================================================================
// TournamentMale.
//=============================================================================
class TournamentMale extends TournamentPlayer
	abstract;

#exec OBJ LOAD FILE=..\Sounds\Male1Voice.uax PACKAGE=Male1Voice
#exec OBJ LOAD FILE=..\Sounds\MaleSounds.uax PACKAGE=Botpack.MaleSounds 

function PlayDying(name DamageType, vector HitLoc)
{
	BaseEyeHeight = Default.BaseEyeHeight;
	PlayDyingSound();
			
	if ( DamageType == 'Suicided' )
	{
		PlayAnim('Dead8',, 0.1);
		return;
	}

	// check for head hit
	if ( (DamageType == 'Decapitated') && !class'GameInfo'.Default.bVeryLowGore )
	{
		PlayDecap();
		return;
	}

	if ( FRand() < 0.15 )
	{
		PlayAnim('Dead2',,0.1);
		return;
	}

	// check for big hit
	if ( (Velocity.Z > 250) && (FRand() < 0.75) )
	{
		if ( FRand() < 0.5 )
			PlayAnim('Dead1',,0.1);
		else
			PlayAnim('Dead11',, 0.1);
		return;
	}

	// check for repeater death
	if ( (Health > -10) && ((DamageType == 'shot') || (DamageType == 'zapped')) )
	{
		PlayAnim('Dead9',, 0.1);
		return;
	}
		
	if ( (HitLoc.Z - Location.Z > 0.7 * CollisionHeight) && !class'GameInfo'.Default.bVeryLowGore )
	{
		if ( FRand() < 0.5 )
			PlayDecap();
		else
			PlayAnim('Dead7',, 0.1);
		return;
	}
	
	if ( Region.Zone.bWaterZone || (FRand() < 0.5) ) //then hit in front or back
		PlayAnim('Dead3',, 0.1);
	else
		PlayAnim('Dead8',, 0.1);
}

function PlayDecap()
{
	local carcass carc;

	PlayAnim('Dead4',, 0.1);
	if ( Level.NetMode != NM_Client )
	{
		carc = Spawn(class 'UT_HeadMale',,, Location + CollisionHeight * vect(0,0,0.8), Rotation + rot(3000,0,16384) );
		if (carc != None)
		{
			carc.Initfor(self);
			carc.Velocity = Velocity + VSize(Velocity) * VRand();
			carc.Velocity.Z = FMax(carc.Velocity.Z, Velocity.Z);
		}
	}
}

function PlayGutHit(float tweentime)
{
	if ( (AnimSequence == 'GutHit') || (AnimSequence == 'Dead2') )
	{
		if (FRand() < 0.5)
			TweenAnim('LeftHit', tweentime);
		else
			TweenAnim('RightHit', tweentime);
	}
	else if ( FRand() < 0.6 )
		TweenAnim('GutHit', tweentime);
	else
		TweenAnim('Dead8', tweentime);

}

function PlayHeadHit(float tweentime)
{
	if ( (AnimSequence == 'HeadHit') || (AnimSequence == 'Dead7') )
		TweenAnim('GutHit', tweentime);
	else if ( FRand() < 0.6 )
		TweenAnim('HeadHit', tweentime);
	else
		TweenAnim('Dead7', tweentime);
}

function PlayLeftHit(float tweentime)
{
	if ( (AnimSequence == 'LeftHit') || (AnimSequence == 'Dead9') )
		TweenAnim('GutHit', tweentime);
	else if ( FRand() < 0.6 )
		TweenAnim('LeftHit', tweentime);
	else 
		TweenAnim('Dead9', tweentime);
}

function PlayRightHit(float tweentime)
{
	if ( (AnimSequence == 'RightHit') || (AnimSequence == 'Dead1') )
		TweenAnim('GutHit', tweentime);
	else if ( FRand() < 0.6 )
		TweenAnim('RightHit', tweentime);
	else
		TweenAnim('Dead1', tweentime);
}

defaultproperties
{
     drown=Botpack.MaleSounds.drownM02
     breathagain=Botpack.MaleSounds.gasp02
     HitSound3=Botpack.MaleSounds.injurM04
     HitSound4=Botpack.MaleSounds.injurH5
	 Die=Botpack.MaleSounds.deathc1
	 Deaths(0)=Botpack.MaleSounds.deathc1
	 Deaths(1)=Botpack.MaleSounds.deathc51
	 Deaths(2)=Botpack.MaleSounds.deathc3
	 Deaths(3)=Botpack.MaleSounds.deathc4
	 Deaths(4)=Botpack.MaleSounds.deathc53
	 Deaths(5)=Botpack.MaleSounds.deathc53
	 GaspSound=Botpack.MaleSounds.hgasp1
	 JumpSound=Botpack.MaleSounds.jump1
     CarcassType=TMale1Carcass
     HitSound1=Botpack.MaleSounds.injurL2
     HitSound2=Botpack.MaleSounds.injurL04
	 UWHit1=Botpack.MaleSounds.UWinjur41
	 UWHit2=Botpack.MaleSounds.UWinjur42
	 LandGrunt=Botpack.MaleSounds.land01
	 VoicePackMetaClass="BotPack.VoiceMale"
}
