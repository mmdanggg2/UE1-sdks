//=============================================================================
// UTBrowserUpdateMasterServerLink.
//=============================================================================
class UTBrowserUpdateMasterServerLink expands UTBrowserUpdateServerLink;

var UTBrowserServerListWindow ConfigObj;

function UTBrowserServerListWindow GetConfigObj()
{
	SetPropertyText("ConfigObj", "UBrowserAll");
	if (ConfigObj == None)
		ConfigObj = new(None, 'UBrowserAll') class'UTBrowserServerListWindow';
	return ConfigObj;
}

event Spawned()
{
	local string Factory, List[ArrayCount(ConfigObj.ListFactories)];
	local int i, j;
	
	GetConfigObj();
	ConfigObj.bFallbackFactories = False;

    for (i = 0; i < ArrayCount(ConfigObj.ListFactories); ++i)
	{
        if (GetDefaultValue(Factory, ConfigObj, "ListFactories", i))
		{
            if (IsGoodFactory(Factory))
			{
                List[j++] = Factory;
            }
        }
    }	
	
	SetMasterServers(List, j);
	QueryUpdateServer();
}

event Destroyed()
{
    ConfigObj = None;
	Super.Destroyed();
}

function QueryUpdateServer()
{
	SetupURIs();
	CurrentURI = GetMaster;
	BrowseCurrentURI();
}

function HTTPError(int ErrorCode)
{
	Log("Failed to load master server list from" @ GetURL() @ "- error code" @ ErrorCode);
	Destroy();
}

function string GetURL()
{
	return "http://" $ UpdateServerAddress $ ":" $ UpdateServerPort $ URIs[GetMaster];
}

static function bool IsOutdatedFactory(string Factory)
{
	return InStr(Factory, ".epicgames.com") != -1 ||
		InStr(Factory, ".gamespy.com") != -1;
}

static function bool IsGoodFactory(string Factory)
{
	return InStr(Caps(Factory), "MASTERSERVERADDRESS") != -1 &&
		!IsOutdatedFactory(Factory);
}

function SetMasterServers(string List[ArrayCount(ConfigObj.ListFactories)], int k)
{
	local string Factory;
	local int i, j;
	
	if (k > 0)
	{
		for (i = 0; i < ArrayCount(List) && k < ArrayCount(List); i++)
		{
			Factory = ConfigObj.ListFactories[i];
			if (Factory == "" || IsOutdatedFactory(Factory))
				continue;
			for (j = 0; j < k; j++)
				if (List[j] == Factory)
					break;
			if (j == k)
				List[k++] = Factory;
		}
			
		for (j = 0; j < ArrayCount(List); j++)
			ConfigObj.ListFactories[j] = List[j];
	}
	
	if (ConfigObj.bHadInitialRefresh)
		ConfigObj.Refresh(False, True);
	ConfigObj.SaveConfig();
}

function HTTPReceivedData(string NetConfig)
{
	local string Factory, List[ArrayCount(ConfigObj.ListFactories)];
	local int i, j, k, n;
	
	Log("Successfully loaded list of master server list from" @ GetURL());
	
	k = 0;
	while (NetConfig != "")
	{
		i = InStr(NetConfig, chr(10));
		if (i == -1)
			i = Len(NetConfig);
		n = i;
		if (Mid(NetConfig, n - 1, 1) == chr(13))
			n--;
		Factory = Left(NetConfig, n);
		if (n > 20 && k < ArrayCount(List) && IsGoodFactory(Factory))
		{
			for (j = 0; j < k; j++)
				if (List[j] == Factory)
					break;
			if (j == k)
				List[k++] = Factory;
		}
		NetConfig = Mid(NetConfig, i + 1);
	}
	
	SetMasterServers(List, k);
	Destroy();
}

defaultproperties
{
}
