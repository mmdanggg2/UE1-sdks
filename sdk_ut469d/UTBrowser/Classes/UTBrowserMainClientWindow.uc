class UTBrowserMainClientWindow expands UBrowserMainClientWindow;

function Created() 
{
	local int i;

	Super.Created();
	
	for (i = 0; i < ArrayCount(ServerListNames); i++)
		if (ServerListNames[i] == 'UBrowserAll')
		{
			if (class'UTBrowserUpdateMasterServerLink'.static.IsOutdatedFactory(FactoryWindows[i].ListFactories[0]))
				GetEntryLevel().Spawn(class'UTBrowserUpdateMasterServerLink');
			break;
		}
}

defaultproperties
{
	ServerListWindowClass="UTBrowser.UTBrowserServerListWindow"
	FavoriteServersClass="UTBrowser.UTBrowserFavoriteServers"
	UpdateServerClass="UTBrowser.UTBrowserUpdateServerWindow"
}
