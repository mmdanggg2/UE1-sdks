class RelicHUDMutator expands HUDMutator;

var Inventory MyRelic;
var float LastChecked, LastRendered;

simulated function PostRender(canvas C)
{
	local float Scale;
	local Inventory inv;
	local int i;

	if ( PlayerOwner != None && PlayerOwner == ChallengeHUD(PlayerOwner.MyHUD).PawnOwner )
	{
		if ( (MyRelic == None) || MyRelic.bDeleteMe || (MyRelic.Owner == None) || (MyRelic.Owner != PlayerOwner) )
		{
			if ( Level.TimeSeconds - LastChecked > 0.3 )
			{
				MyRelic = None;
				LastChecked = Level.TimeSeconds;
				inv = PlayerOwner.Inventory;
				While (inv != None )
				{
					if ( inv.IsA('RelicInventory') )
					{
						MyRelic = inv;
						break;
					}
					inv = inv.inventory;
					i++;
					if ( i > 200 )
						inv = none;
				}
			}
		}
		if ( MyRelic != None )
		{
			Scale = ChallengeHUD(PlayerOwner.MyHUD).Scale;
			C.DrawColor = ChallengeHUD(PlayerOwner.MyHUD).HUDColor;
			C.SetPos(C.ClipX - 64 * Scale, C.ClipY - 192 * Scale);
			C.DrawIcon(MyRelic.Icon, Scale);
		}
	}

	if ( NextHUDMutator != None )
		NextHUDMutator.PostRender( C );
}
