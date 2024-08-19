//
// SniperRifle Fixes:
//
// Botpack.SniperRifle goes into state 'Zooming' whenever the player clicks the
// alt-fire button. This state activates Zoom by calling
// Engine.PlayerPawn.StartZoom. StartZoom sets Engine.PlayerPawn.bZooming to true
// and Engine.PlayerPawn.ZoomLevel to 0.0.
// 
// From this point onwards, Engine.PlayerPawn.UpdateEyeHeight gradually increases
// the Engine.PlayerPawn.ZoomLevel and adjusts the player FoV accordingly.
//
// Zooming is supposed to continue until Botpack.SniperRifle.Zooming.Tick notices
// that the player has released the alt-fire button. However, if the SniperRifle
// switches to a different state (e.g., the NormalFire state) before the player
// releases the alt-fire button, then zooming continues until ZoomLevel reaches its
// maximum value.
//
// This is obviously not what the weapon's creators intended and, for this reason,
// many sniperrifle subclasses fix this bug. Unfortunately, though, there are also
// several mods that either rely on the buggy behavior or that break when we apply
// the fixes in this compatibility class.
//
// Relevant Bug Reports:
// https://github.com/OldUnreal/UnrealTournamentPatches/issues/249
// https://github.com/OldUnreal/UnrealTournamentPatches/issues/248
//
// Internal Reports/PRs:
// SDK/PR/49
// SDK/PR/209
//
// Credits:
// Deaod, Sizzl, Buggie
//
class OUSniperRifle extends SniperRifle;

enum EZoomState
{
	ZS_None,
	ZS_Zooming,
	ZS_Zoomed,
	ZS_Reset
};
var EZoomState ZoomState;

simulated function EndZooming()
{
	if ( Owner.IsA('PlayerPawn') && PlayerPawn(Owner).Player.IsA('ViewPort') )
	{
		ZoomState = ZS_None;
		PlayerPawn(Owner).EndZoom();
	}
}

simulated function ClientPutDown(Weapon NextWeapon)
{
	Super.ClientPutDown(NextWeapon);
	EndZooming();
}

simulated function bool ClientAltFire(float Value)
{
	if (Owner.IsA('PlayerPawn') == false)
	{
		Pawn(Owner).bFire = 1;
		Pawn(Owner).bAltFire = 0;
		Global.Fire(0);
	}
	else
	{
		GotoState('Idle');
	}

	return true;
}

state NormalFire
{
    simulated function Tick(float DeltaTime)
	{
        Global.Tick(DeltaTime);
    }
}

state DownWeapon
{
    ignores Fire, AltFire, AnimEnd;

	function BeginState()
	{
		Super.BeginState();
		EndZooming();
	}
}

simulated function Tick(float DeltaTime)
{
	if (Owner != none &&
		Owner.IsA('PlayerPawn') &&
		bCanClientFire
	)
	{
		switch (ZoomState)
		{
		    case ZS_None:
			    if (Pawn(Owner).bAltFire != 0)
				{
				    if (PlayerPawn(Owner).Player.IsA('ViewPort'))
					    PlayerPawn(Owner).StartZoom();
    				SetTimer(0.2, true);
    				ZoomState = ZS_Zooming;
    			}
     			break;
    		case ZS_Zooming:
    			if (Pawn(Owner).bAltFire == 0)
				{
    				if (PlayerPawn(Owner).Player.IsA('ViewPort'))
    					PlayerPawn(Owner).StopZoom();
    				ZoomState = ZS_Zoomed;
    			}
    			break;
    		case ZS_Zoomed:
    			if (Pawn(Owner).bAltFire != 0)
				{
    				if (PlayerPawn(Owner).Player.IsA('ViewPort'))
    					PlayerPawn(Owner).EndZoom();
    				SetTimer(0.0, false);
    				ZoomState = ZS_Reset;
    			}
    			break;
    		case ZS_Reset:
    			if (Pawn(Owner).bAltFire == 0)
				{
    				ZoomState = ZS_None;
    			}
    			break;
    	}
	}
}

state Zooming
{
    simulated function Tick(float DeltaTime)
	{
        Global.Tick(DeltaTime);
    }

    simulated function BeginState()
	{
    }
}