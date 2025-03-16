class UTBrowserUpdateServerLink expands UBrowserUpdateServerLink;

var int MOTDTry;

function SetupURIs()
{
	local string GameVer;
	GameVer = Level.EngineVersion $ Left(Level.EngineRevision, 1);
	if (InStr(Level.EngineRevision, "Release") == -1)
		GameVer $= "_p";

	MaxURI = 3;
	URIs[3] = "/UpdateServer/utmotd" $ GameVer $ ".html";
	URIs[2] = "/UpdateServer/utmotdfallback.html";
	URIs[1] = "/UpdateServer/utmasterserver.txt";
	URIs[0] = "/UpdateServer/utircserver.txt";
}

function HTTPError(int ErrorCode)
{
	if (ErrorCode == 404 && CurrentURI == GetMOTD && MOTDTry++ == 0)
	{
		URIs[GetMOTD] = "/UpdateServer/utmotd" $ Level.EngineVersion $ ".html";
		BrowseCurrentURI();
	}
	else
		Super.HTTPError(ErrorCode);
}
