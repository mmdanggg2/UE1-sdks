//=============================================================================
// TSkaarjbot.
//=============================================================================
class TSkaarjbot extends CustomBot;

// special animation functions
function PlayDuck()
{
	BaseEyeHeight = 0;
	PlayAnim('Duck',1, 0.25);
}

function FastInAir()
{
	local float TweenTime;

	BaseEyeHeight =  0.7 * Default.BaseEyeHeight;
	if ( AnimSequence == 'DodgeF' )
		TweenTime = 1;
	else if ( GetAnimGroup(AnimSequence) == 'Jumping' )
	{
		TweenAnim('DodgeF', 1);
		return;
	}
	else 
		TweenTime = 0.3;

	if ( (Weapon == None) || (Weapon.Mass < 20) )
		TweenAnim('JumpSMFR', TweenTime);
	else
		TweenAnim('JumpLGFR', TweenTime); 
}
	
function PlayInAir()
{
	local float TweenTime;

	BaseEyeHeight =  0.7 * Default.BaseEyeHeight;
	if ( AnimSequence == 'DodgeF' )
		TweenTime = 2;
	else if ( GetAnimGroup(AnimSequence) == 'Jumping' )
	{
		TweenAnim('DodgeF', 2);
		return;
	}
	else 
		TweenTime = 0.7;

	if ( (Weapon == None) || (Weapon.Mass < 20) )
		TweenAnim('JumpSMFR', TweenTime);
	else
		TweenAnim('JumpLGFR', TweenTime); 
}

function PlayDodge(bool bDuckLeft)
{
	bCanDuck = false;
	Velocity.Z = 200;
	if ( bDuckLeft )
		PlayAnim('RollLeft', 1.35, 0.06);
	else 
		PlayAnim('RollRight', 1.35, 0.06);
}

function PlayDying(name DamageType, vector HitLoc)
{
	if ( Mesh == FallBackMesh )
	{
		Super.PlayDying(DamageType, HitLoc);
		return;
	}
	BaseEyeHeight = Default.BaseEyeHeight;
	PlayDyingSound();
			
	if ( DamageType == 'Suicided' )
	{
		PlayAnim('Dead1',, 0.1);
		return;
	}

	// check for head hit
	if ( DamageType == 'Decapitated' )
	{
		PlaySkaarjDecap();
		return;
	}

	// check for big hit
	if ( Velocity.Z > 200 )
	{
		if ( FRand() < 0.65 )
			PlayAnim('Dead4',,0.1);
		else if ( FRand() < 0.5 )
			PlayAnim('Dead2',, 0.1);
		else
			PlayAnim('Dead3',, 0.1);
		return;
	}

	// check for repeater death
	if ( (Health > -10) && ((DamageType == 'shot') || (DamageType == 'zapped')) )
	{
		PlayAnim('Dead9',, 0.1);
		return;
	}

	if ( HitLoc.Z - Location.Z > 0.7 * CollisionHeight )
	{
		if ( FRand() < 0.35  )
			PlaySkaarjDecap();
		else
			PlayAnim('Dead2',, 0.1);
		return;
	}
	
	if ( FRand() < 0.6 ) //then hit in front or back
		PlayAnim('Dead1',, 0.1);
	else
		PlayAnim('Dead3',, 0.1);
}

function PlaySkaarjDecap()
{
	local carcass carc;

	if ( class'GameInfo'.Default.bVeryLowGore )
	{
		PlayAnim('Dead2',, 0.1);
		return;
	}

	PlayAnim('Dead5',, 0.1);
	if ( Level.NetMode != NM_Client )
	{
		carc = Spawn(class 'TSkaarjHead',,, Location + CollisionHeight * vect(0,0,0.8), Rotation + rot(3000,0,16384) );
		if (carc != None)
		{
			carc.Initfor(self);
			carc.RemoteRole = ROLE_SimulatedProxy;
			carc.Velocity = Velocity + VSize(Velocity) * VRand();
			carc.Velocity.Z = FMax(carc.Velocity.Z, Velocity.Z);
		}
	}
}

defaultproperties
{
	LODBias=+0.72
	TeamSkin1=2
	TeamSkin2=3
	FixedSkin=0
	FaceSkin=1
	MultiSkins(0)=texture'TSkMSkins.Warr1'
	MultiSkins(1)=texture'TSkMSkins.Warr1'
	MultiSkins(2)=texture'TSkMSkins.Warr3'
	MultiSkins(3)=texture'TSkMSkins.Warr2'
	bIsMultiSkinned=true
	DefaultFace="Dominator"
	TeamSkin="T_skaarj_"
	Mesh=mesh'EpicCustomModels.TSkM'
	SelectionMesh="EpicCustomModels.TSkM"
	Menuname="Skaarj Hybrid"
	DefaultSkinName="TSkMSkins.Warr"
	DefaultCustomPackage="TSkMSkins."
     drown=Sound'UnrealI.SKPDrown1'
     breathagain=Sound'UnrealI.SKPGasp1'
     Footstep1=Sound'MultiMesh.SkWalk'
     Footstep2=Sound'MultiMesh.SkWalk'
     Footstep3=Sound'MultiMesh.SkWalk'
     HitSound3=Sound'UnrealI.SKPInjur3'
     HitSound4=Sound'UnrealI.SKPInjur4'
     GaspSound=Sound'UnrealI.SKPGasp1'
     UWHit1=Sound'UnrealI.MUWHit1'
     UWHit2=Sound'UnrealI.MUWHit2'
     LandGrunt=Sound'UnrealI.Land1SK'
     JumpSound=Sound'UnrealI.SKPJump1'
     HitSound1=Sound'UnrealI.SKPInjur1'
     HitSound2=Sound'UnrealI.SKPInjur2'
	 Deaths(0)=Sound'UnrealI.SKPDeath1'
	 Deaths(1)=Sound'UnrealI.SKPDeath2'
	 Deaths(2)=Sound'UnrealI.SKPDeath3'
	 Deaths(3)=Sound'UnrealI.SKPDeath4'
	 Deaths(4)=Sound'UnrealI.SKPDeath1'
	 Deaths(5)=Sound'UnrealI.SKPDeath3'
	 VoiceType="MultiMesh.SkaarjVoice"
}
