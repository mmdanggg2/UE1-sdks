//=============================================================================
// TNali.
//=============================================================================
class TNali extends CustomPlayer;

// special animation functions
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
		PlayAnim('Dead',, 0.1);
		return;
	}

	// check for head hit
	if ( DamageType == 'Decapitated' )
	{
		PlayNaliDecap();
		return;
	}

	// check for big hit
	if ( Velocity.Z > 200 )
	{
		if ( FRand() < 0.65 )
			PlayAnim('Dead4',,0.1);
		else
			PlayAnim('Dead2',, 0.1);
		return;
	}

	if ( HitLoc.Z - Location.Z > 0.7 * CollisionHeight )
	{
		if ( FRand() < 0.35  )
			PlayNaliDecap();
		else
			PlayAnim('Dead2',, 0.1);
		return;
	}
	
	if ( FRand() < 0.6 ) //then hit in front or back
		PlayAnim('Dead',, 0.1);
	else
		PlayAnim('Dead2',, 0.1);
}

function PlayNaliDecap()
{
	local carcass carc;

	if ( class'GameInfo'.Default.bVeryLowGore )
	{
		PlayAnim('Dead2',, 0.1);
		return;
	}

	PlayAnim('Dead3',, 0.1);
	if ( Level.NetMode != NM_Client )
	{
		carc = Spawn(class 'TNaliHead',,, Location + CollisionHeight * vect(0,0,0.8), Rotation + rot(3000,0,16384) );
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
	bIsMultiSkinned=false
	DefaultFace="TNaliMeshSkins.nali-Face"
	TeamSkin="T_nali_"
	Mesh=mesh'EpicCustomModels.TNaliMesh'
	SelectionMesh="EpicCustomModels.TNaliMesh"
	DefaultSkinName="TNaliMeshSkins.Ouboudah"
	DefaultCustomPackage="TNaliMeshSkins."
    Footstep1=Sound'UnrealShare.walkC'
    Footstep2=Sound'UnrealShare.walkC'
    Footstep3=Sound'UnrealShare.walkC'
	Deaths(0)=Sound'UnrealShare.death1n'
	Deaths(1)=Sound'UnrealShare.death1n'
	Deaths(2)=Sound'UnrealShare.death2n'
	Deaths(3)=Sound'UnrealShare.bowing1n'
	Deaths(4)=Sound'UnrealShare.injur1n'
	Deaths(5)=Sound'UnrealShare.injur2n'
    HitSound1=fear1n
    HitSound2=cringe2n
    HitSound3=injur1n
    HitSound4=injur2n
    UWHit1=Sound'UnrealShare.MUWHit1'
    UWHit2=Sound'UnrealShare.MUWHit2'
	drown=MDrown1
	breathagain=cough1n
	GaspSound=breath1n
	JumpSound=MJump1
	LandGrunt=lland01
	Skin=texture'TNaliMeshSkins.Ouboudah'
	Menuname="Nali"
	VoiceType="MultiMesh.NaliVoice"
}