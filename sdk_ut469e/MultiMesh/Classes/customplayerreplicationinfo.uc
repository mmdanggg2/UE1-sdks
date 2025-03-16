//=============================================================================
// CustomPlayerReplicationInfo.
//=============================================================================
class CustomPlayerReplicationInfo expands PlayerReplicationInfo;


simulated function PostBeginPlay()
{
	SetTimer(2.0, true);
	Super.PostBeginPlay();
}
 					
simulated function Timer()
{
	local Pawn P;
	local Mesh NewMesh;

	if ( TalkTexture == None )
		TalkTexture = Texture(DynamicLoadObject("SoldierSkins.blkt5Malcom",class'Texture'));
	if ( Role == ROLE_Authority )
		Super.Timer();
	else
		ForEach AllActors(class'Pawn',P)
			if ( P.PlayerReplicationInfo == self )
			{
				if ( (P.Mesh == None) && P.IsA('CustomPlayer') )
					CustomPlayer(P).SetMyMesh();
				else
					SetTimer(0.0, false);				
				return;
			}	
}