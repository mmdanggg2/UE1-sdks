class RelicInventory expands TournamentPickup;

#exec TEXTURE IMPORT NAME=RelicIconVengeance  FILE=Textures\RelicIconVengeance.PCX GROUP=Icons
#exec TEXTURE IMPORT NAME=RelicIconDefense    FILE=Textures\RelicIconDefense.PCX GROUP=Icons
#exec TEXTURE IMPORT NAME=RelicIconStrength   FILE=Textures\RelicIconStrength.PCX GROUP=Icons
#exec TEXTURE IMPORT NAME=RelicIconRedemption FILE=Textures\RelicIconRedemption.PCX GROUP=Icons
#exec TEXTURE IMPORT NAME=RelicIconSpeed	  FILE=Textures\RelicIconSpeed.PCX GROUP=Icons
#exec TEXTURE IMPORT NAME=RelicIconRegen	  FILE=Textures\RelicIconRegen.PCX GROUP=Icons

#exec AUDIO IMPORT FILE="Sounds\RelicPickup.WAV" NAME="RelicPickup" GROUP="Relics"

var Relic MyRelic;
var float IdleTime;
var RelicShell ShellEffect;
var texture ShellSkin;
var class<RelicShell> ShellType;

function Destroyed()
{
	if ( ShellEffect != None )
		ShellEffect.Destroy();
	if ( (MyRelic != None) && (MyRelic.SpawnedRelic == self) )
		MyRelic.SpawnRelic(0);

	Super.Destroyed();
}

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();

	if ( Level.NetMode == NM_DedicatedServer )
		return;
	
	CheckForHUDMutator();
}

simulated function CheckForHUDMutator()
{
	local Mutator M;
	local RelicHUDMutator RHM;
	local PlayerPawn P;

	ForEach AllActors(class'PlayerPawn', P)
		if ( P.myHUD != None )
		{
			// check if it already has a RelicHUDMutator
			M = P.myHud.HUDMutator;
			while ( M != None )
			{
				if ( M.IsA('RelicHUDMutator') )
					return;
				if ( M.NextHUDMutator == None && HudMutator(M) != None )
					M = HudMutator(M).NextRHUDMutator;
				else
					M = M.NextHUDMutator;
			}
			RHM = Spawn(class'RelicHUDMutator');
			RHM.RegisteraHUDMutator();
			if ( RHM.bIsHUDMutator )
			{
				if ( Role == ROLE_SimulatedProxy )
					SetTimer(0.0, false);
				return;
			}
			else
				RHM.Destroy();
		}	

	if ( Role == ROLE_SimulatedProxy )
		SetTimer(1.0, true);
}

simulated function Timer()
{
	Super.Timer();
	if ( Role == ROLE_SimulatedProxy )
		CheckForHUDMutator();
}

function bool HandlePickupQuery( inventory Item )
{
	if ( Item.IsA('RelicInventory') )
		return True;
	else 
		return Super.HandlePickupQuery( Item );
}

auto state Pickup
{
	function BeginState()
	{
		Super.BeginState();
		IdleTime = 0;
		SetOwner(None);
	}

	function Touch( actor Other )
	{
		if ( ValidTouch(Other) ) 
		{
			bHeldItem = True;
			Instigator = Pawn(Other);
			CheckForHUDMutator();
			BecomeItem();
			Pawn(Other).AddInventory( Self );

			if (Level.Game.LocalLog != None)
				Level.Game.LocalLog.LogPickup(Self, Pawn(Other));
			if (Level.Game.WorldLog != None)
				Level.Game.WorldLog.LogPickup(Self, Pawn(Other));

			if (bActivatable && Pawn(Other).SelectedItem==None) 
				Pawn(Other).SelectedItem=Self;

			if (bActivatable && bAutoActivate && Pawn(Other).bAutoActivate) 
				Self.Activate();

			if ( PickupMessageClass == None )
				Pawn(Other).ClientMessage(PickupMessage, 'Pickup');
			else
				Pawn(Other).ReceiveLocalizedMessage( PickupMessageClass, 0, None, None, Self.Class );
			PlaySound (PickupSound,,2.0);	
			PickupFunction(Pawn(Other));
		}
	}

	simulated function Landed(Vector HitNormal)
	{
		local rotator newRot;

		newRot = Rotation;
		newRot.pitch = 0;
		SetRotation(newRot);
		SetPhysics(PHYS_Rotating);
		IdleTime = 0;
		SetCollision( True, False, False );
	}
}

state Activated
{
	function EndState()
	{
	}

	function Timer()
	{
		if ( Role == ROLE_SimulatedProxy )
			CheckForHUDMutator();
	}

	function BeginState()
	{
		IdleTime = 0;
	}
}

state DeActivated
{
Begin:
}

function DropInventory()
{
	local RelicInventory Dropped;
	local vector X,Y,Z;

	Dropped = Spawn(Class);
	Dropped.MyRelic = MyRelic;
	MyRelic.SpawnedRelic = Dropped;
	if (Owner != None )
	{
		Owner.GetAxes(Owner.Rotation,X,Y,Z);
		Dropped.SetLocation( Owner.Location + 0.8 * Owner.CollisionRadius * X + -0.5 * Owner.CollisionRadius * Y );
	}
	Dropped.RemoteRole	  = ROLE_SimulatedProxy;
	Dropped.Mesh          = PickupViewMesh;
	Dropped.DrawScale     = PickupViewScale;
	Dropped.bOnlyOwnerSee = false;
	Dropped.bHidden       = false;
	Dropped.bCarriedItem  = false;
	Dropped.NetPriority   = 1.4;
	Dropped.SetCollision( False, False, False );
	Dropped.SetPhysics(PHYS_Falling);
	Dropped.bCollideWorld = True;
	if ( Owner != None )
		Dropped.Velocity = Vector(Pawn(Owner).ViewRotation) * 500 + vect(0,0,220);
	Dropped.GotoState('PickUp', 'Dropped');
	// Remove from player's inventory.
	Destroy();
}

function PickupFunction(Pawn Other)
{
	Super.PickupFunction(Other);

	LightType = LT_None;
}


function FlashShell(float Duration)
{
	if (ShellEffect == None)
	{
		ShellEffect = Spawn(ShellType, Owner,,Owner.Location, Owner.Rotation); 
	}
	if ( ShellEffect != None )
	{
		ShellEffect.Mesh = Owner.Mesh;
		ShellEffect.DrawScale = Owner.Drawscale;
		ShellEffect.Texture = ShellSkin;
		ShellEffect.SetTimer(Duration, false);
	}
}

event float BotDesireability( pawn Bot )
{
	local Inventory Inv;

	// If we already have a Relic, we don't want another one.

	for( Inv=Bot.Inventory; Inv!=None; Inv=Inv.Inventory )   
		if ( Inv.IsA('RelicInventory') )
			return -1;

	return MaxDesireability;
}

defaultproperties
{
	ShellType=class'RelicShell'
	bAutoActivate=True
	bActivatable=True
	bHeldItem=False
	PickupViewMesh=LodMesh'Botpack.DomN'
	PickupMessage="You picked up a null Relic."
	PickupSound=Sound'RelicPickup'
    CollisionRadius=22.000000
    CollisionHeight=8.000000
    Mass=10.000000
	ShellSkin=FireTexture'UnrealShare.Belt_fx.ShieldBelt.RedShield'
	Skin=Texture'GreenSkin2'
    MaxDesireability=3.000000
	bUnlit=True
	AmbientGlow=255
	LightType=LT_None
	LightEffect=LE_NonIncidence
	LightBrightness=255
	LightRadius=5
    LightHue=170
    LightSaturation=255
	LightPeriod=64
	LightPhase=255
	DrawType=DT_Mesh
	Style=STY_Normal
    bFixedRotationDir=True
    RotationRate=(Roll=20000,Yaw=30000)
    DesiredRotation=(Roll=20000,Yaw=30000)
	RemoteRole=ROLE_SimulatedProxy
}
