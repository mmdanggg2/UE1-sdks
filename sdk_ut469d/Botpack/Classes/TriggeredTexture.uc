class TriggeredTexture extends Triggers;

var() Texture	DestinationTexture;
var() Texture	Textures[10];
var() bool		bTriggerOnceOnly;

var int CurrentTexture;

replication
{
	reliable if( Role==ROLE_Authority )
		CurrentTexture;
}

simulated event PostBeginPlay()
{
	Super.PostBeginPlay();
	CurrentTexture = 0;

	if( ScriptedTexture(DestinationTexture) != None )
		ScriptedTexture(DestinationTexture).NotifyActor = Self;
}

simulated event Destroyed()
{
	if( ScriptedTexture(DestinationTexture) != None && ScriptedTexture(DestinationTexture).NotifyActor == Self)
		ScriptedTexture(DestinationTexture).NotifyActor = None;
	
	Super.Destroyed();
}

event Trigger( Actor Other, Pawn EventInstigator )
{
	if( bTriggerOnceOnly && (CurrentTexture == 9 || Textures[CurrentTexture + 1] == None) )
		return;

	CurrentTexture++;
	if( CurrentTexture == 10 || Textures[CurrentTexture] == None )
		CurrentTexture = 0;
}

simulated event RenderTexture( ScriptedTexture Tex )
{
	Tex.DrawTile( 0, 0, Tex.USize, Tex.VSize, 0, 0, Textures[CurrentTexture].USize, Textures[CurrentTexture].VSize, Textures[CurrentTexture], False );
}

defaultproperties
{
	RemoteRole=ROLE_SimulatedProxy
	bAlwaysRelevant=True
	bNoDelete=True
}
