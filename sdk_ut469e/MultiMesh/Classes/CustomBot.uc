//=============================================================================
// CustomBot.
//=============================================================================
class CustomBot extends MaleBotPlus
	abstract;

var mesh FallBackMesh;
var() string DefaultFace, TeamSkin, DefaultCustomPackage;

static function SetMyMultiSkin(Actor SkinActor, string SkinName, string FaceName, byte TeamNum)
{
	local string MeshName, FacePackage, SkinItem, FaceItem, SkinPackage;

	MeshName = SkinActor.GetItemName(string(SkinActor.Mesh));

	SkinItem = SkinActor.GetItemName(SkinName);
	FaceItem = SkinActor.GetItemName(FaceName);
	FacePackage = Left(FaceName, Len(FaceName) - Len(FaceItem));
	SkinPackage = Left(SkinName, Len(SkinName) - Len(SkinItem));
	log(SkinItem@FaceItem@FacePackage@SkinPackage);

	if(SkinPackage == "")
	{
		SkinPackage=default.DefaultCustomPackage;
		SkinName=SkinPackage$SkinName;
	}
	if(FacePackage == "")
	{
		FacePackage=default.DefaultCustomPackage;
		FaceName=FacePackage$FaceName;
	}

	// Set the fixed skin element.  If it fails, go to default skin & no face.
	if(!SetSkinElement(SkinActor, default.FixedSkin, SkinName$string(default.FixedSkin+1), default.DefaultSkinName$string(default.FixedSkin+1)))
	{
		SkinName = default.DefaultSkinName;
		FaceName = "";
	}

	// Set the face - if it fails, set the default skin for that face element.
	SetSkinElement(SkinActor, default.FaceSkin, FacePackage$SkinItem$String(default.FaceSkin+1)$FaceItem, SkinName$String(default.FaceSkin+1));

	// Set the team elements
	if( TeamNum != 255 )
	{
		SetSkinElement(SkinActor, default.TeamSkin1, SkinName$string(default.TeamSkin1+1)$"T_"$String(TeamNum), SkinName$string(default.TeamSkin1+1));
		SetSkinElement(SkinActor, default.TeamSkin2, SkinName$string(default.TeamSkin2+1)$"T_"$String(TeamNum), SkinName$string(default.TeamSkin2+1));
	}
	else
	{
		SetSkinElement(SkinActor, default.TeamSkin1, SkinName$string(default.TeamSkin1+1), "");
		SetSkinElement(SkinActor, default.TeamSkin2, SkinName$string(default.TeamSkin2+1), "");
	}

	// Set the talktexture
	if(Pawn(SkinActor) != None)
	{
		if(FaceName != "")
			Pawn(SkinActor).PlayerReplicationInfo.TalkTexture = Texture(DynamicLoadObject(FacePackage$SkinItem$"5"$FaceItem, class'Texture'));
		else
			Pawn(SkinActor).PlayerReplicationInfo.TalkTexture = None;
	}		
}

static function SetMultiSkin(Actor SkinActor, string SkinName, string FaceName, byte TeamNum)
{
	local string MeshName, SkinPackage;
	local Texture NewSkin;

	if ( SkinActor.Mesh == Default.FallBackMesh )
	{
		Super.SetMultiSkin(SkinActor, "CommandoSkins.cmdo", "Blake", TeamNum);
		return;
	}

	if ( SkinName == "" )
		SkinName = default.DefaultSkinName;

	if ( default.bisMultiSkinned )
	{
		if ( FaceName == "" )
			FaceName = default.DefaultFace;

		SetMyMultiSkin(SkinActor, SkinName, FaceName, TeamNum);
		return;
	}

	// only one skin for mesh

	MeshName = SkinActor.GetItemName(string(SkinActor.Mesh));
	if( TeamNum != 255 )
		SkinName = default.DefaultCustomPackage$default.TeamSkin$String(TeamNum);

	if( !SetSkinElement(SkinActor, 0, SkinName, default.DefaultSkinName) )
		SkinName = default.DefaultSkinName;

	NewSkin = Texture(DynamicLoadObject(SkinName, class'Texture'));
	if ( NewSkin != None )
		SkinActor.Skin = NewSkin;
	else
	{
		log("Failed to load "$SkinName);
		if(default.DefaultSkinName != "")
		{
			NewSkin = Texture(DynamicLoadObject(default.DefaultSkinName, class'Texture'));
			SkinActor.Skin = NewSkin;
		}
	}

	// Set the talktexture
	if( Pawn(SkinActor) != None )
	{
		if ( (SkinName != Default.DefaultSkinName) && (TeamNum == 255) )
		{
			Pawn(SkinActor).PlayerReplicationInfo.TalkTexture = Texture(DynamicLoadObject(SkinName$"Face", class'Texture'));
			if ( Pawn(SkinActor).PlayerReplicationInfo.TalkTexture == None )
				Pawn(SkinActor).PlayerReplicationInfo.TalkTexture = Texture(DynamicLoadObject(SkinName$"-Face", class'Texture'));
			if ( Pawn(SkinActor).PlayerReplicationInfo.TalkTexture == None )
				Pawn(SkinActor).PlayerReplicationInfo.TalkTexture = Texture(DynamicLoadObject(default.DefaultFace, class'Texture'));
		}
		else
			Pawn(SkinActor).PlayerReplicationInfo.TalkTexture = Texture(DynamicLoadObject(default.DefaultFace, class'Texture'));
	}
}

static function GetMultiSkin( Actor SkinActor, out string SkinName, out string FaceName )
{
	local string FullSkinName, ShortSkinName;
	local int Size;

	if ( default.bisMultiSkinned
		|| (SkinActor.Mesh == Default.FallBackMesh) )
	{
		Super.GetMultiSkin(SkinActor,SkinName,FaceName);
		return;
	}

	// only one skin
	FaceName = "";

	if (SkinActor.Skin == None && TCowBot(SkinActor) != None && SkinActor.MultiSkins[1] != None)
	{
		FullSkinName  = String(SkinActor.MultiSkins[1]);
		Size = 65535;
	}
	else
	{
		FullSkinName  = String(SkinActor.Skin);
		Size = 4;
	}
	ShortSkinName = SkinActor.GetItemName(FullSkinName);
	SkinName = Left(FullSkinName, Len(FullSkinName) - Len(ShortSkinName)) $ Left(ShortSkinName, Size);
}

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();
	SetMyMesh();
	if ( Role == ROLE_SimulatedProxy )
		SetTimer(1.0, true);
}

simulated function Timer()
{
	if ( Role == ROLE_SimulatedProxy )
		SetMyMesh();

	Super.Timer();
}

simulated function SetMyMesh()
{
	local Mesh NewMesh;

	if ( class'MultiMeshMenu'.default.bForceDefaultMesh )
	{
		Mesh = FallBackMesh;
		Super.SetMultiSkin(self, "CommandoSkins.cmdo", "Blake", PlayerReplicationInfo.Team);
		return;
	}

	if ( Default.Mesh == none )
	{
		bIsMultiSkinned = true;
		if ( Mesh == None )
		{
			Mesh = FallBackMesh;
			Super.SetMultiSkin(self, "CommandoSkins.cmdo", "Blake", PlayerReplicationInfo.Team);
		}
	}
	else if ( (PlayerReplicationInfo == None) || !PlayerReplicationInfo.bIsSpectator )
		Mesh = Default.Mesh;
}

// should use PostNetBeginPlay, but version 400 doesn't have it, so put code here
simulated function Tick(float DeltaTime)
{
	if ( Role != ROLE_SimulatedProxy )
	{
		Disable('Tick');
		return;
	}
	if ( bIsMultiSkinned )
	{
		if ( MultiSkins[1] == None )
		{
			if ( bIsPlayer && (PlayerReplicationInfo != None)  )
				SetMultiSkin(self, "","", PlayerReplicationInfo.team);
			else
				SetMultiSkin(self, "","", 0);
		}
	}
	else if ( Skin == None )
		Skin = Default.Skin;

	if ( (PlayerReplicationInfo != None) 
		&& (PlayerReplicationInfo.Owner == None) )
		PlayerReplicationInfo.SetOwner(self);

	if ( Mesh == none ) //client doesn't have special mesh, make sure have valid skin
	{
		Mesh = FallBackMesh;
		self.Static.SetMultiSkin(self,"CommandoSkins.cmdo", "Blake", PlayerReplicationInfo.Team);
	}
	Disable('Tick');
}

function PlayVictoryDance()
{
	local float decision;

	decision = FRand();

	if ( decision < 0.4 )
		PlayAnim('Victory1',0.7, 0.2);
	else if ( decision < 0.7 )
		PlayAnim('Thrust',0.7, 0.2);
	else
		PlayAnim('Taunt1',0.7, 0.2);
}

// don't make assumptions deaths will also work as certain type of hit anim
function PlayGutHit(float tweentime)
{
	if ( AnimSequence == 'GutHit' )
	{
		if (FRand() < 0.5)
			TweenAnim('LeftHit', tweentime);
		else
			TweenAnim('RightHit', tweentime);
	}
	else
		TweenAnim('GutHit', tweentime);
}

function PlayHeadHit(float tweentime)
{
	if ( AnimSequence == 'HeadHit' )
		TweenAnim('GutHit', tweentime);
	else
		TweenAnim('HeadHit', tweentime);
}

function PlayLeftHit(float tweentime)
{
	if ( AnimSequence == 'LeftHit' )
		TweenAnim('GutHit', tweentime);
	else
		TweenAnim('LeftHit', tweentime);
}

function PlayRightHit(float tweentime)
{
	if ( AnimSequence == 'RightHit' )
		TweenAnim('GutHit', tweentime);
	else
		TweenAnim('RightHit', tweentime);
}

defaultproperties
{
	PlayerReplicationInfoClass='MultiMesh.CustomBotPlayerReplicationInfo'
	FallBackMesh=Mesh'Botpack.Commando'
	Mesh=Mesh'Botpack.Commando'
	SelectionMesh="Botpack.SelectionMale1"
	SpecialMesh="Botpack.TrophyMale1"
	JumpSound=TMJump3
	LandGrunt=MLand3
	CarcassType=TMale1Carcass
	Menuname="Custom Player"
	VoiceType="BotPack.VoiceMaleOne"
	TeamSkin1=2
	TeamSkin2=3
	FixedSkin=0
	FaceSkin=1
	DefaultSkinName="CommandoSkins.cmdo"
	DefaultPackage="CommandoSkins."
	VoicePackMetaClass="BotPack.VoiceMale"
    CarcassType=TMale1Carcass
}
