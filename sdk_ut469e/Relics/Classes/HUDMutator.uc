class HUDMutator expands Mutator;

// crappy hack to work with version 400 (which didn't have the NextHUDMutator variable)
// this means its not compatible with other HUD mutators that aren't subclassed from RelicHUDMutator :(
var HUDMutator NextRHUDMutator;
var PlayerPawn PlayerOwner;// Registers the current mutator on the client to receive PostRender calls.
var bool bIsHudMutator;

// Fixed version of the Mutator function
simulated function RegisteraHUDMutator()
{
	local PlayerPawn P;

	ForEach AllActors(class'PlayerPawn', P)
		if ( P.myHUD != None )
		{
			NextRHUDMutator = HUDMutator(P.myHud.HUDMutator);
			NextHUDMutator = P.myHud.HUDMutator;
			P.myHUD.HUDMutator = Self;
			PlayerOwner = P;
			bIsHUDMutator = True;
			bHUDMutator = True;
		}	
}
