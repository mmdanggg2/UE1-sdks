class UBrowserFavoritesFact extends UBrowserServerListFactory;

var config int FavoriteCount;
var config string Favorites[100];

/* eg Favorites[0]=Host Name\10.0.0.1\7778\True */


function string ParseOption(string Input, int Pos)
{
	local int i;

	while(True)
	{
		if(Pos == 0)
		{
			i = InStr(Input, "\\");
			if(i != -1)
				Input = Left(Input, i);
			return Input;
		}

		i = InStr(Input, "\\");
		if(i == -1)
			return "";

		Input = Mid(Input, i+1);
		Pos--;
	}
}

function Query(optional bool bBySuperset, optional bool bInitial)
{
	local int i;
	local UBrowserServerList L;

	Super.Query(bBySuperset, bInitial);

	for(i=0;i<FavoriteCount;i++)
	{
		L = FoundServer(ParseOption(Favorites[i], 1), Int(ParseOption(Favorites[i], 2)), "", "Unreal", ParseOption(Favorites[i], 0));
		L.bKeepDescription = ParseOption(Favorites[i], 3) ~= (string(True));
	}

	QueryFinished(True);
}

function SaveFavorites()
{
	local UBrowserServerList I, Lists[3];
	local int k;

	FavoriteCount = 0;
	Lists[0] = UBrowserServerList(PingedList.Next);
	Lists[1] = UBrowserServerList(UnPingedList.Next);
	Lists[2] = UBrowserServerList(UnPingedList.Owner.HiddenList.Next);
	
	for (k = 0; k < ArrayCount(Lists); k++)
		for(I = Lists[k]; i!=None; I = UBrowserServerList(I.Next))
		{
			if(FavoriteCount == 100)
				break;
			Favorites[FavoriteCount] = I.HostName$"\\"$I.IP$"\\"$string(I.QueryPort)$"\\"$string(I.bKeepDescription);
	
			FavoriteCount++;
		}

	if(FavoriteCount < 100)
		Favorites[FavoriteCount] = "";

	SaveConfig();
}