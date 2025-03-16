//=============================================================================
// TCow.
//=============================================================================
class TCow extends CustomPlayer;

simulated function SetMyMesh()
{
	Super.SetMyMesh();
	bIsMultiSkinned = true;
}

static function SetMultiSkin(Actor SkinActor, string SkinName, string FaceName, byte TeamNum)
{
	local string SkinItem, SkinPackage;

	if ( SkinActor.Mesh == Default.FallBackMesh )
	{
		Super.SetMultiSkin(SkinActor, "CommandoSkins.cmdo", "Blake", TeamNum);
		return;
	}

	// two skins

	if ( SkinName == "" )
		SkinName = default.DefaultSkinName;
	else
	{
		SkinItem = SkinActor.GetItemName(SkinName);
		SkinPackage = Left(SkinName, Len(SkinName) - Len(SkinItem));
	
		if( SkinPackage == "" )
		{
			SkinPackage=default.DefaultCustomPackage;
			SkinName=SkinPackage$SkinName;
		}
	}
	if( !SetSkinElement(SkinActor, 1, SkinName, default.DefaultSkinName) )
		SkinName = default.DefaultSkinName;

	// Set the team elements
	if( TeamNum < 4 )
		SetSkinElement(SkinActor, 2, default.DefaultCustomPackage$default.TeamSkin$String(TeamNum), SkinName);
	else
		SkinActor.MultiSkins[2] = Default.MultiSkins[2];

	// Set the talktexture
	if( Pawn(SkinActor) != None && Pawn(SkinActor).PlayerReplicationInfo != None )
	{
		if ( (SkinName != Default.DefaultSkinName) && (TeamNum == 255) )
		{
			Pawn(SkinActor).PlayerReplicationInfo.TalkTexture = Texture(DynamicLoadObject(SkinName$"Face", class'Texture'));
			if ( Pawn(SkinActor).PlayerReplicationInfo.TalkTexture == None )
				Pawn(SkinActor).PlayerReplicationInfo.TalkTexture = Texture(DynamicLoadObject(default.DefaultFace, class'Texture'));
		}
		else
			Pawn(SkinActor).PlayerReplicationInfo.TalkTexture = Texture(DynamicLoadObject(default.DefaultFace, class'Texture'));
	}
}

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
		PlayAnim('Dead2',, 0.1);
		return;
	}

	// check for head hit
	if ( DamageType == 'Decapitated' )
	{
		PlayCowDecap();
		return;
	}

	// check for big hit
	if ( Velocity.Z > 200 )
	{
		PlayAnim('Dead3',,0.1);
		return;
	}

	if ( HitLoc.Z - Location.Z > 0.7 * CollisionHeight )
	{
		PlayAnim('Dead2',, 0.1);
		return;
	}
	
	PlayAnim('Dead1',, 0.1);
}

function PlayCowDecap()
{
	local carcass carc;

	if ( class'GameInfo'.Default.bVeryLowGore )
	{
		PlayAnim('Dead2',, 0.1);
		return;
	}

	PlayAnim('Dead4',, 0.1);
	if ( Level.NetMode != NM_Client )
	{
		carc = Spawn(class 'TCowHead',,, Location + CollisionHeight * vect(0,0,0.8), Rotation + rot(3000,0,16384) );
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
	Skin=texture'TCowMeshSkins.WarCow'
	MultiSkins(0)=texture'TCowMeshSkins.WarCow'
	MultiSkins(1)=texture'TCowMeshSkins.WarCow'
	MultiSkins(2)=texture'EpicCustomModels.CowPack'
	DefaultFace="TCowMeshSkins.WarCowFace"
	TeamSkin="T_cow_"
	Mesh=mesh'EpicCustomModels.TCowMesh'
	SelectionMesh="EpicCustomModels.TCowMesh"
	Menuname="Nali Cow"
	DefaultSkinName="TCowMeshSkins.WarCow"
	DefaultCustomPackage="TCowMeshSkins."
    Footstep1=Sound'UnrealShare.walkC'
    Footstep2=Sound'UnrealShare.walkC'
    Footstep3=Sound'UnrealShare.walkC'
	Deaths(0)=DeathC1c
	Deaths(1)=DeathC1c
	Deaths(2)=DeathC1c
	Deaths(3)=DeathC1c
	Deaths(4)=cMoo2c
	Deaths(5)=cMoo2c
    HitSound1=injurC1c
    HitSound2=injurC2c
    HitSound3=injurC1c
    HitSound4=cMoo2c
    UWHit1=Sound'UnrealShare.MUWHit1'
    UWHit2=Sound'UnrealShare.MUWHit2'
	drown=MDrown1
	breathagain=cough1n
	GaspSound=breath1n
	JumpSound=MJump1
	LandGrunt=lland01
	VoiceType="MultiMesh.CowVoice"
	CarcassType=TCowCarcass
}
