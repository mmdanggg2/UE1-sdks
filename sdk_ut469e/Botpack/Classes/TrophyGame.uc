class TrophyGame extends UTIntro;

var Class<Trophy> NewTrophyClass;
var int TrophyTime;
var rotator CorrectRotation;
var bool bTrophyInit;

event PlayerPawn Login
(
	string Portal,
	string Options,
	out string Error,
	class<PlayerPawn> SpawnClass
)
{
	local PlayerPawn NewPlayer;
	local TrophyDude TD;
	local int i;
	local Mesh M;

	NewPlayer = Super.Login(Portal, Options, Error, SpawnClass);

	if (SpawnClass.Default.SpecialMesh == "" || 
		(SpawnClass.Default.SpecialMesh == "Botpack.TrophyMale1" && SpawnClass.Default.Mesh != LodMesh'Botpack.Commando') ||
		(SpawnClass.Default.SpecialMesh == "Botpack.TrophyMale2" && SpawnClass.Default.Mesh != LodMesh'Botpack.Soldier') ||
		(SpawnClass.Default.SpecialMesh == "Botpack.TrophyFemale1" && SpawnClass.Default.Mesh != LodMesh'Botpack.FCommando') ||
		(SpawnClass.Default.SpecialMesh == "Botpack.TrophyFemale2" && SpawnClass.Default.Mesh != LodMesh'Botpack.SGirl'))
	{
		M = SpawnClass.Default.Mesh;
	}
	else
	{
		M = Mesh(DynamicLoadObject(SpawnClass.Default.SpecialMesh, class'Mesh'));
	}	

	foreach AllActors(class'TrophyDude', TD)
	{
		TD.Mesh = M;
		TD.Skin = NewPlayer.Skin;
		for (i=0; i<8; i++)
			TD.MultiSkins[i] = NewPlayer.MultiSkins[i];
		if (TD.HasAnim('Wave'))
			TD.PlayAnim('Wave', 0.3);
	}

	return NewPlayer;
}

function AcceptInventory( Pawn PlayerPawn)
{
	local DeathMatchTrophy DMT;
	local DominationTrophy DOMT;
	local CTFTrophy CTFT;
	local AssaultTrophy AT;
	local Challenge ChalT;
	local UTIntro SuperCast;

	Super.AcceptInventory(PlayerPawn);
	if ( (LadderObj != None) && !bTrophyInit )
	{
		bTrophyInit = true;
		
		// Hide trophies.
		foreach AllActors(class'DeathMatchTrophy', DMT)
		{
			CorrectRotation = DMT.Rotation;
			if (LadderObj.DMRank != 6)
			{
				DMT.bHidden = True;
			} else {
				if (LadderObj.LastMatchType == 1)
				{
					NewTrophyClass = DMT.Class;
					TrophyTime = 28;
					DMT.bHidden = True;
				}
			}
		}
		foreach AllActors(class'DominationTrophy', DOMT)
		{
			if (LadderObj.DOMRank != 6)
			{
				DOMT.bHidden = True;
			} else {
				if (LadderObj.LastMatchType == 3)
				{
					NewTrophyClass = DOMT.Class;
					TrophyTime = 30;
					DOMT.bHidden = True;
				}
			}
		}
		foreach AllActors(class'CTFTrophy', CTFT)
		{
			if (LadderObj.CTFRank != 6)
			{
				CTFT.bHidden = True;
				} else {
				if (LadderObj.LastMatchType == 2)
				{
					NewTrophyClass = CTFT.Class;
					TrophyTime = 30;
					CTFT.bHidden = True;
				}
			}
		}
		foreach AllActors(class'AssaultTrophy', AT)
		{
			if (LadderObj.ASRank != 6)
			{
				AT.bHidden = True;
			} else {
				if (LadderObj.LastMatchType == 4)
				{
					NewTrophyClass = AT.Class;
					TrophyTime = 29;
					AT.bHidden = True;
				}
			}
		}
		foreach AllActors(class'Challenge', ChalT)
		{
			if (LadderObj.ChalRank != 6)
			{
				ChalT.bHidden = True;
			} else {
				if (LadderObj.LastMatchType == 5)
				{
					NewTrophyClass = ChalT.Class;
					TrophyTime = 30;
					ChalT.bHidden = True;
				}
			}
		}
		// Award this dude the SECRET ROBOT BOSS MESH!!!
		if ((LadderObj.DMRank == 6) && (LadderObj.DOMRank == 6) &&
			(LadderObj.CTFRank == 6) && (LadderObj.ASRank == 6) &&
			(LadderObj.ChalRank == 6))
		{
			class'Ladder'.Default.HasBeatenGame = True;
			class'Ladder'.Static.StaticSaveConfig();
		}
	}
}

function Timer()
{
	local Trophy T;

	Super.Timer();

//	Log(Level.TimeSeconds);
	if (TrophyTime >= 0)
		TrophyTime--;
	if (NewTrophyClass != None)
	{
		if (TrophyTime == 0)
		{
			foreach AllActors(class'Trophy', T)
			{
				if (NewTrophyClass == T.Class)
				{
					PlayTrophyEffect(T);
					T.bHidden = False;
				}
			}
		}
	}
}

function PlayTrophyEffect(Trophy NewTrophy)
{
	Spawn(class'UTTeleportEffect',,, NewTrophy.Location, CorrectRotation);
	NewTrophy.PlaySound(sound'Resp2A',, 10.0);
}


defaultproperties
{
     HUDType=class'Botpack.CHEOLHUD'
}
