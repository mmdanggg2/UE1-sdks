class UTBrowserServerGrid expands UBrowserServerGrid;

var UWindowGridColumn ngStats;
var UWindowGridColumn Ver;
var UWindowGridColumn Specs;

var UBrowserScreenshotCW ScreenshotCW;

var localized string ngStatsName;
var localized string VersionName;
var localized string SpecsName;
var localized string EnabledText;
var UBrowserServerList ConnectToServer;
var bool bWaitingForNgStats;

var UWindowMessageBox AskNgStats;
var localized string AskNgStatsTitle;
var localized string AskNgStatsText;

var localized string ActiveText;
var localized string InactiveText;

function CreateColumns()
{
	Super.CreateColumns();

	Ver	= AddColumn(VersionName, 20);
	Specs = AddColumn(SpecsName, 40);
	ngStats	= AddColumn(ngStatsName, 80);
}

function DrawCell(Canvas C, float X, float Y, UWindowGridColumn Column, UBrowserServerList List)
{
	switch(Column)
	{
	case Ver:
		Column.ClipText( C, X, Y, string(List.GameVer) );
		break;
	case Specs:
		Column.ClipText( C, X, Y, List.NumSpectators $ "/" $ List.MaxSpectators );
		break;
	case ngStats:
		if( List.GameVer >= 406 && UTBrowserServerList(List).bNGWorldStats )
		{
			if( UTBrowserServerList(List).bNGWorldStatsActive )
				Column.ClipText( C, X, Y, ActiveText );
			else
				Column.ClipText( C, X, Y, InactiveText );
		}
		else
		if(UTBrowserServerList(List).bNGWorldStatsActive)
			Column.ClipText( C, X, Y, EnabledText );
		break;
	default:
		Super.DrawCell(C, X, Y, Column, List);
		break;
	}
}

function int BySpecs(UBrowserServerList T, UBrowserServerList B)
{
	local int Result;

	if(B == None) return -1;
	
	if(T.Ping == 9999) return 1;
	if(B.Ping == 9999) return -1;

	if(T.NumSpectators > B.NumSpectators)
	{
		Result = -1;
	}
	else
	if (T.NumSpectators < B.NumSpectators)
	{
		Result = 1;
	}
	else
	{
		if (T.MaxSpectators > B.MaxSpectators)
		{
			Result = -1;
		}
		else
		if(T.MaxSpectators < B.MaxSpectators)
		{
			Result = 1;
		}
		else
		{
			Result = ByPlayers(T, B);
		}
	}

	if(bSortDescending)
		Result = -Result;

	return Result;
}

function int Compare(UBrowserServerList T, UBrowserServerList B)
{
	switch(SortByColumn)
	{
	case Ver:
		if( T.GameVer == B.GameVer )
			return ByName(T, B);

		if( T.GameVer >= B.GameVer )
		{
			if(bSortDescending)
				return 1;
			else
				return -1;
		}
		else
		{
			if(bSortDescending)
				return -1;
			else
				return 1;
		}
		
		break;
	case Specs:
		return BySpecs(T, B);
		
		break;
	case ngStats:
		if( UTBrowserServerList(T).bNGWorldStatsActive == UTBrowserServerList(B).bNGWorldStatsActive )
		{
			if( UTBrowserServerList(T).bNGWorldStats == UTBrowserServerList(B).bNGWorldStats )
				return ByName(T, B);

			if( UTBrowserServerList(T).bNGWorldStats )
			{
				if(bSortDescending)
					return 1;
				else
					return -1;
			}
			else
			{
				if(bSortDescending)
					return -1;
				else
					return 1;
			}
		}
		if(UTBrowserServerList(T).bNGWorldStatsActive)
		{
			if(bSortDescending)
				return 1;
			else
				return -1;
		}
		else
		{
			if(bSortDescending)
				return -1;
			else
				return 1;
		}

		break;
	default:
		return Super.Compare(T, B);
		break;
	}
}

function MessageBoxDone(UWindowMessageBox W, MessageBoxResult Result)
{
	if(W == AskNgStats)
	{
		AskNgStats = None;
		if(Result == MR_Cancel)
			return;
		else
		if(Result == MR_Yes)
		{
			ShowModal(Root.CreateWindow(class<UWindowWindow>(DynamicLoadObject("UTMenu.ngWorldSecretWindow", class'Class')), 100, 100, 200, 200, Root, True));
			bWaitingForNgStats = True;
		}
		else
		{
			GetPlayerOwner().ngSecretSet = True;
			GetPlayerOwner().SaveConfig();
			ReallyJoinServer(ConnectToServer);
		}
	}
}

function JoinServer(UBrowserServerList Server)
{
	local bool bEmpty;
	if(Server != None) 
	{
		bEmpty = Server.GamePort == 0;
		if (bEmpty)
			Server.GamePort = Server.QueryPort - 1;
		ReallyJoinServer(Server);
		if (bEmpty)
			Server.GamePort = 0;
	}
}

function BeforePaint(Canvas C, float X, float Y)
{
	Super.BeforePaint(C, X, Y);
	if(bWaitingForNgStats && !WaitModal())
	{
		ReallyJoinServer(ConnectToServer);
		bWaitingForNgStats = False;
	}
}

function ReallyJoinServer(UBrowserServerList Server)
{
	local int i;
	for (i = 0; i < 5; i++)
	{
		SetPropertyText("ScreenshotCW", "UBrowserScreenshotCW" $ i);
		if (ScreenshotCW != None)
			ScreenshotCW.Screenshot = None;
	}
	GetPlayerOwner().ClientTravel("unreal://"$Server.IP$":"$Server.GamePort$UBrowserServerListWindow(GetParent(class'UBrowserServerListWindow')).URLAppend, TRAVEL_Absolute, false);
	GetParent(class'UWindowFramedWindow').Close();
	Root.Console.CloseUWindow();
}

defaultproperties
{
	ngStatsName="Stats Logging"
	VersionName="Version"
	SpecsName="Spectators"
	EnabledText="Enabled"
	ActiveText="Active"
	InactiveText="Inactive"
	AskNgStatsTitle="Use ngWorldStats?"
	AskNgStatsText="This server has stat accumulation enabled. Your ngWorldStats password has not been set. If you set a new ngWorldStats password, you can record all of your gameplay stats (Kills, Suicides, etc) online! If you do not set a password you will opt out of stat accumulation.\\n\\nDo you want to set an ngWorldStats password?"
}
