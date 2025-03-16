//=============================================================================
// UBrowserServerPing: Query an Unreal server for its details
//=============================================================================
class UBrowserServerPing extends UdpLink;

var UBrowserServerList	Server;

var IpAddr				ServerIPAddr;
var float				RequestSentTime;
var float				LastDelta;
var name				QueryState;
var bool				bInitial;
var bool				bJustThisServer;
var bool				bNoSort;
var int					PingAttempts;
var int					AttemptNumber;
var int					BindAttempts;
var UBrowserPlayerList	ServerPlayerList;
var UBrowserRulesList	ServerRulesList;
var string              FilterString;

var localized string	AdminEmailText;
var localized string	AdminNameText;
var localized string	ChangeLevelsText;
var localized string	MultiplayerBotsText;
var localized string	FragLimitText;
var localized string	TimeLimitText;
var localized string	GameModeText;
var localized string	GameTypeText;
var localized string	GameVersionText;
var localized string	WorldLogText;
var localized string	MutatorsText;
var localized string	TrueString;
var localized string	FalseString;
var localized string	ServerAddressText;
var localized string	GoalTeamScoreText;
var localized string	MinPlayersText;
var localized string	PlayersText;
var localized string	MaxTeamsText;
var localized string	BalanceTeamsText;
var localized string	PlayersBalanceTeamsText;
var localized string	FriendlyFireText;
var localized string	MinNetVersionText;
var localized string	BotSkillText;
var localized string	TournamentText;
var localized string	ServerModeText;
var localized string	DedicatedText;
var localized string	NonDedicatedText;
var localized string	WorldLogWorkingText;
var localized string	WorldLogWorkingTrue;
var localized string	WorldLogWorkingFalse;
var localized string	PasswordText;

var localized string	TeamNameText;

var localized string	MapAuthorText;
var localized string	MapTitleText;
var localized string	IdealPlayerCountText;
var localized string	NumBotsText;
var localized string	OverTimeText;
var localized string	RemainingTimeText;
var localized string	GameSpeedText;
var localized string	AirControlText;
var localized string	WeaponStayText;
var localized string	UseTranslocatorText;
var localized string	MonsterSkillText;
var localized string	MonstersTotalText;
var localized string	UseLivesText;
var localized string	LivesText;

// config
var config int			MaxBindAttempts;
var config int			BindRetryTime;
var config int			PingTimeout;
var config bool			bUseMapName;

var string Teams[4];
var bool bTeams;
var int MaxTeams;

function ValidateServer()
{
	if(Server.ServerPing != Self)
	{
		Log("ORPHANED: "$Self);
		Destroy();
	}
}

function StartQuery(name S, int InPingAttempts)
{
	QueryState = S;
	ValidateServer();
	ServerIPAddr.Port = Server.QueryPort;
	GotoState('Resolving');
	PingAttempts=InPingAttempts;
	AttemptNumber=1;
}

function Resolved( IpAddr Addr )
{
	ServerIPAddr.Addr = Addr.Addr;

	GotoState('Binding');
}

function bool GetNextValue(string In, out string Out, out string Result)
{
	local int i;
	local bool bFoundStart;

	Result = "";
	bFoundStart = False;

	for(i=0;i<Len(In);i++) 
	{
		if(bFoundStart)
		{
			if(Mid(In, i, 1) == "\\")
			{
				Out = Right(In, Len(In) - i);
				return True;
			}
			else
			{
				Result $= Mid(In, i, 1);
			}
		}
		else
		{
			if(Mid(In, i, 1) == "\\")
			{
				bFoundStart = True;
			}
		}
	}

	return False;
}

function string LocalizeBoolValue(string Value)
{
	if(Value ~= "True")
		return TrueString;
	
	if(Value ~= "False")
		return FalseString;

	return Value;
}

function string LocalizeSkin(string SkinName)
{
	local string MeshName, Junk, SkinDesc;

	MeshName = Left(SkinName, InStr(SkinName, "."));

	GetNextSkin(MeshName, SkinName$"1", 0, Junk, SkinDesc);
	if(Junk == "")
		GetNextSkin(MeshName, SkinName, 0, Junk, SkinDesc);
	if(Junk == "")
		return GetItemName(SkinName);
	
	return SkinDesc;
}

function string LocalizeTeam(string TeamNum)
{
	if(TeamNum == "255")
		return "None";
	return TeamNum;
}

function AddRule(string Rule, string Value)
{
	local UBrowserRulesList  RulesList;

	ValidateServer();

	for(RulesList = UBrowserRulesList(ServerRulesList.Next); RulesList != None; RulesList = UBrowserRulesList(RulesList.Next))
		if(RulesList.Rule == Rule)
			return; // Rule already exists

	// Add the rule
	RulesList = UBrowserRulesList(ServerRulesList.Append(class'UBrowserRulesList'));
	RulesList.Rule = Rule;
	RulesList.Value = Value;
}

state Binding
{
Begin:
	if( BindPort() == 0 )
	{
		Log("UBrowserServerPing: Port failed to bind.  Attempt "$BindAttempts);
		BindAttempts++;

		ValidateServer();
		if(BindAttempts == MaxBindAttempts)
			Server.PingDone(bInitial, bJustThisServer, False, bNoSort);
		else
			GotoState('BindFailed');
	}
	else
	{
		GotoState(QueryState);
	}
}

state BindFailed
{
	event Timer()
	{
		GotoState('Binding');
	}

Begin:
	SetTimer(BindRetryTime, False);
}

state GetStatus 
{
	event ReceivedText( IpAddr Addr, string Text )
	{
		local string Value, Str;
		local int Pos, Num;
		local string In;
		local string Out;
		local byte ID;
		local bool bOK;
		local UBrowserPlayerList PlayerEntry;

		ValidateServer();
		In = Text;
		do 
		{
			bOK = GetNextValue(In, Out, Value);
			In = Out;
			if(Left(Value, 7) == "player_")
			{
				ID = Int(Mid(Value, 7));

				PlayerEntry = ServerPlayerList.FindID(ID);
				if(PlayerEntry == None) 
					PlayerEntry = UBrowserPlayerList(ServerPlayerList.Append(class'UBrowserPlayerList'));
				PlayerEntry.PlayerID = ID;

				bOK = GetNextValue(In, Out, Value);
				In = Out;
				PlayerEntry.PlayerName = Value;
				FilterString $= Caps(Value) $ Chr(13);
			} 
			else if(Left(Value, 6) == "frags_") 
			{
				ID = Int(Mid(Value, 6));

				bOK = GetNextValue(In, Out, Value);
				In = Out;
				PlayerEntry = ServerPlayerList.FindID(ID);
				PlayerEntry.PlayerFrags = Int(Value);
			}
			else if(Left(Value, 5) == "ping_")
			{
				ID = Int(Mid(Value, 5));

				bOK = GetNextValue(In, Out, Value);
				In = Out;
				PlayerEntry = ServerPlayerList.FindID(ID);
				PlayerEntry.PlayerPing = Int(Right(Value, len(Value) - 1));  // leading space
			}
			else if(Left(Value, 5) == "team_")
			{
				ID = Int(Mid(Value, 5));

				bOK = GetNextValue(In, Out, Value);
				In = Out;
				if (!bTeams)
				{
					Pos = InStr(In, "score_");
					if (Pos >= 0 && Pos < 20)
						bTeams = true;
				}
				if (!bTeams)
				{
					PlayerEntry = ServerPlayerList.FindID(ID);
					PlayerEntry.PlayerTeam = LocalizeTeam(Value);
				}
				else if (Value != "")
					Teams[ID] = Value;
			}
			else if(Left(Value, 5) == "skin_")
			{
				ID = Int(Mid(Value, 5));

				bOK = GetNextValue(In, Out, Value);
				In = Out;
				PlayerEntry = ServerPlayerList.FindID(ID);
				PlayerEntry.PlayerSkin = LocalizeSkin(Value);
			}
			else if(Left(Value, 5) == "face_")
			{
				ID = Int(Mid(Value, 5));

				bOK = GetNextValue(In, Out, Value);
				In = Out;
				PlayerEntry = ServerPlayerList.FindID(ID);
				PlayerEntry.PlayerFace = GetItemName(Value);
			}
			else if(Left(Value, 5) == "mesh_")
			{
				ID = Int(Mid(Value, 5));

				bOK = GetNextValue(In, Out, Value);
				In = Out;
				PlayerEntry = ServerPlayerList.FindID(ID);
				PlayerEntry.PlayerMesh = Value;
			}
			else if(Left(Value, 9) == "ngsecret_")
			{
				ID = Int(Mid(Value, 9));

				bOK = GetNextValue(In, Out, Value);
				In = Out;
				PlayerEntry = ServerPlayerList.FindID(ID);
				PlayerEntry.PlayerStats = Value;
			}
			else if(Value == "final")
			{
				Server.FilterString = FilterString;
				SetLists();
				Server.StatusDone(True);
				return;
			}
			else if(Value ~= "gamever")
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;
				AddRule(GameVersionText, Value);
			}
			else if(Value ~= "minnetver")
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;				
				AddRule(MinNetVersionText, Value);
			}
			else if(Value ~= "gametype")
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;				
				AddRule(GameTypeText, Value);
				FilterString $= Caps(Value) $ Chr(13);
			}
/*			else if(Value ~= "gamemode") // "openplaying"
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;
				AddRule(GameModeText, Value);
			}*/
			else if(Value ~= "timelimit") 
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;				
				AddRule(TimeLimitText, Value);
			}
			else if(Value ~= "fraglimit") 
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;				
				AddRule(FragLimitText, Value);
			}
			else if(Value ~= "MultiplayerBots") 
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;				
				AddRule(MultiplayerBotsText, LocalizeBoolValue(Value));
			}
			else if(Value ~= "AdminName") 
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;				
				AddRule(AdminNameText, Value);
				FilterString $= Caps(Value) $ Chr(13);
			}
			else if(Value ~= "AdminEMail")
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;				
				AddRule(AdminEmailText, Value);
				FilterString $= Caps(Value) $ Chr(13);
			}
			else if(Value ~= "WantWorldLog")
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;				
				AddRule(WorldLogText, LocalizeBoolValue(Value));
			}
			else if(Value ~= "WorldLog")
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;				
				if( Server.GameVer >= 406 )
				{
					if( Value ~= "True" )
						AddRule(WorldLogWorkingText, WorldLogWorkingTrue);
					else
						AddRule(WorldLogWorkingText, WorldLogWorkingFalse);
				}
				else
					AddRule(WorldLogText, LocalizeBoolValue(Value));
			}
			else if(Value ~= "mutators")
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;
				Num = 0;
				FilterString $= Caps(Value) $ Chr(13);
				while (Value != "")
				{
					Pos = InStr(Value, ",");
					if (Pos == -1)
						Str = Value;
					else
						Str = Left(Value, Pos);
					Pos = Len(Str) + 1;
					while (Mid(Value, Pos, 1) == " ")
						Pos++;
					if (++Num < 10)
						AddRule(MutatorsText @ " " $ Num, Str);
					else
						AddRule(MutatorsText @ Num, Str);
					Value = Mid(Value, Pos);
				}
			}
			else if(Value ~= "goalteamscore")
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;
				AddRule(GoalTeamScoreText, Value);		
			}
			else if(Value ~= "minplayers")
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;
				if(Value == "0")
					AddRule(MultiplayerBotsText, FalseString);
				else
					AddRule(MinPlayersText, Value@PlayersText);		
			}
			else if(Value ~= "changelevels")
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;				
				AddRule(ChangeLevelsText, LocalizeBoolValue(Value));		
			}
			else if(Value ~= "botskill")
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;				
				AddRule(BotSkillText, Value);
				FilterString $= Caps(Value) $ Chr(13);
			}
			else if(Value ~= "maxteams")
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;				
				AddRule(MaxTeamsText, Value);
				MaxTeams = int(Value);
			}
			else if(Value ~= "balanceteams")
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;				
				AddRule(BalanceTeamsText, LocalizeBoolValue(Value));
			}
			else if(Value ~= "playersbalanceteams")
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;				
				AddRule(PlayersBalanceTeamsText, LocalizeBoolValue(Value));
			}
			else if(Value ~= "friendlyfire")
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;				
				AddRule(FriendlyFireText, Value);
			}
			else if(Value ~= "gamestyle")
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;				
				AddRule(GameModeText, Value);
				FilterString $= Caps(Value) $ Chr(13);
			}
			else if(Value ~= "tournament")
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;				
				AddRule(TournamentText, LocalizeBoolValue(Value));
			}
			else if(Value ~= "listenserver")
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;				
				if(bool(Value))
					AddRule(ServerModeText, NonDedicatedText);
				else
					AddRule(ServerModeText, DedicatedText);
			}
			else if(Value ~= "password")
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;
				Server.bLocked = bool(Value);
				AddRule(PasswordText, LocalizeBoolValue(Value));
			}
			else if(Value ~= "echo_replay" || Value ~= "echo_reply" || Value ~= "echo")
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;
				bTeams = true;
			}
			else if (bTeams && Mid(Value, Len(Value) - 2, 1) == "_" && 
				string(int(Right(Value, 1))) == Right(Value, 1))
			{
				ID = int(Right(Value, 1));
				bOK = GetNextValue(In, Out, Str);
				In = Out;
				if (Left(Value, 6) ~= "score_")
					Str = string(int(Str));
				if (ID < MaxTeams && Str != "")
					AddRule(TeamNameText @ Teams[ID] @ Caps(Left(Value, 1)) $
						Mid(Value, 1, Len(Value) - 3), Str);
			}
			else if(Value ~= "NumBots")
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;
				if (Value != "")
					AddRule(NumBotsText, Value);
			}
			else if(Value ~= "bOverTime")
			{
				bOK = GetNextValue(In, Out, Str);
				In = Out;
				bOK = GetNextValue(In, Out, Value);
				if (Value == "RemainingTime")
				{
					In = Out;
					bOK = GetNextValue(In, Out, Value);
					In = Out;
					if (Value != "" && !bool(Str))
						AddRule(RemainingTimeText, FormatTime(Value));
					bOK = GetNextValue(In, Out, Value);
				}
				if (Value == "ElapsedTime")
				{
					In = Out;
					bOK = GetNextValue(In, Out, Value);
					In = Out;
					if (Value != "" && bool(Str))
						AddRule(RemainingTimeText, "-" $ FormatTime(Value) @ "(" $ OverTimeText $ ")");
				}
			}
			else if(Value ~= "GameSpeed")
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;
				if (Value != "")
				{
					Value = int(float(Value)*100) $ "%";
					AddRule(GameSpeedText, Value);
					FilterString = FilterString $ Caps(Value) $ Chr(13);
				}
			}
			else if(Value ~= "AirControl")
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;
				if (Value != "")
				{
					Value = int(float(Value)*100) $ "%";
					AddRule(AirControlText, Value);
					FilterString = FilterString $ Caps(Value) $ Chr(13);
				}
			}
			else if(Value ~= "bMultiWeaponStay")
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;
				if (Value != "")
					AddRule(WeaponStayText, LocalizeBoolValue(Value));
			}
			else if(Value ~= "bUseTranslocator")
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;
				if (Value != "")
					AddRule(UseTranslocatorText, LocalizeBoolValue(Value));
			}
			else if(Value ~= "bUseLives")
			{
				bOK = GetNextValue(In, Out, Str);
				In = Out;
				if (Str != "")
					AddRule(UseLivesText, LocalizeBoolValue(Str));
				bOK = GetNextValue(In, Out, Value);
				if (Value == "Lives")
				{
					In = Out;
					bOK = GetNextValue(In, Out, Value);
					In = Out;
					if (Value != "" && bool(Str))
						AddRule(LivesText, Value);
				}
			}
			else if(Value ~= "MonsterSkill")
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;
				if (Value != "")
				{
					AddRule(MonsterSkillText, Value);
				}
			}
			else if(Value ~= "MonstersTotal")
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;
				if (Value != "")
					AddRule(MonstersTotalText, Value);
			}
			else if(Value ~= "Author")
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;
				if (Value != "")
				{
					AddRule(MapAuthorText, Value);
					FilterString = FilterString $ Caps(Value) $ Chr(13);
				}
			}
			else if(Value ~= "IdealPlayerCount")
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;
				if (Value != "")
					AddRule(IdealPlayerCountText, Value);
			}
			else if(Value ~= "maptitle")
			{
				bOK = GetNextValue(In, Out, Value);
				In = Out;
				if (Value != "")
					AddRule(MapTitleText, Value);
			}
			else if(Value ~= "queryid" ||
				Value ~= "hostname" ||
				Value ~= "hostport" ||
				Value ~= "mapname" ||
				Value ~= "gamemode" ||
				Value ~= "gamename" ||
				Value ~= "numplayers" ||
				Value ~= "maxplayers" ||
				Value ~= "location" ||
				Value ~= "unknown_query")
			{ // skip
				bOK = GetNextValue(In, Out, Value);
				In = Out;
			}
			else
			{
				bOK = GetNextValue(In, Out, Str);
				In = Out;
				if (Str != "")
					AddRule(Value, Str);
			}
		} until(!bOK);
	}
	
	function string FormatTime(coerce int Num)
	{
		local string Ret;
		local int M, S;
		S = Num % 60;
		M = Num / 60;
		Ret = string(S);
		if (S < 10)
			Ret = "0" $ Ret;
		Ret = M $ ":" $ Ret;
		if (M < 10)
			Ret = "0" $ Ret;
		return Ret;
	}

	event Timer()
	{
		if(AttemptNumber < PingAttempts)
		{
			Log("Timed out getting player replies.  Attempt "$AttemptNumber);
			AttemptNumber++;
			GotoState(QueryState);
		}
		else
		{
			SetLists();
			Server.StatusDone(False);
			Log("Timed out getting player replies.  Giving Up");
		}
	}
	
	function SetLists()
	{
		if(Server.PlayerList != None)
		{
			if (ServerPlayerList != None)
			{
				ServerPlayerList.SortColumn = Server.PlayerList.SortColumn;
				ServerPlayerList.bDescending = Server.PlayerList.bDescending;
			}
			Server.PlayerList.DestroyList();
		}
		Server.PlayerList = ServerPlayerList;

		if(Server.RulesList != None)
		{
			if (ServerRulesList != None)
			{
				ServerRulesList.SortColumn = Server.RulesList.SortColumn;
				ServerRulesList.bDescending = Server.RulesList.bDescending;
			}
			Server.RulesList.DestroyList();
		}
		Server.RulesList = ServerRulesList;
	}
Begin:
	// Player info

	ValidateServer();
	ServerPlayerList = New(None) class'UBrowserPlayerList';
	ServerPlayerList.SetupSentinel();	

	ServerRulesList = New(None) class'UBrowserRulesList';
	ServerRulesList.SetupSentinel();
	if (Server.GamePort == 0 && Server.QueryPort > 0)
		AddRule(ServerAddressText, "unreal://"$Server.IP$":"$string(Server.QueryPort - 1));
	else
		AddRule(ServerAddressText, "unreal://"$Server.IP$":"$string(Server.GamePort));

	SendText( ServerIPAddr, "\\status\\" $
		"\\echo\\" $
		"\\teams\\" $
		"\\level_property\\Author\\" $
		"\\level_property\\IdealPlayerCount\\" $
		"\\game_property\\NumBots\\" $
		"\\game_property\\bOverTime\\" $
		"\\game_property\\RemainingTime\\" $
		"\\game_property\\ElapsedTime\\" $
		"\\game_property\\GameSpeed\\" $
		"\\game_property\\AirControl\\" $
		"\\game_property\\bMultiWeaponStay\\" $
		"\\game_property\\bUseTranslocator\\" $
		"\\game_property\\bUseLives\\" $
		"\\game_property\\Lives\\" $
		"\\game_property\\MonstersTotal\\" $
		"\\game_property\\MonsterSkill\\" $		
		// Bug fix for XServerQuery <= 2.00.  Without a
		// known command at the end, XServerQuery will not
		// respond, and the server browser will mark the server
		// as offline.
		"\\echo\\" );
	SetTimer(PingTimeout + FRand(), False);
}

function string ParseReply(string Text, string Key)
{
	local int i;
	local string Temp;

	i=InStr(Text, "\\"$Key$"\\");
	Temp = Mid(Text, i + Len(Key) + 2);
	return Left(Temp, InStr(Temp, "\\"));
}

state GetInfo
{
	event ReceivedText(IpAddr Addr, string Text)
	{
		local string Temp;
		local float ElapsedTime;

		// Make sure this packet really is for us.
		Temp = IpAddrToString(Addr);
		if ( StripPort(IpAddrToString(ServerIPAddr)) != StripPort(Temp) )
			return;

		ValidateServer();
		ElapsedTime = (Level.TimeSeconds - RequestSentTime) / Level.TimeDilation;
		Server.Ping = Max(1000*ElapsedTime - (0.5*LastDelta) - 10, 4); // subtract avg client and server frametime from ping.
		if(!Server.bKeepDescription)
			Server.HostName = Server.IP;
		Server.GamePort = 0;
		Server.MapName = "";
		Server.MapTitle = "";
		Server.MapDisplayName = "";
		Server.GameType = "";
		Server.GameMode = "";
		Server.NumPlayers = 0;
		Server.ReportedPlayersCount = 0;
		Server.MaxPlayers = 0;
		Server.NumSpectators = 0;
		Server.MaxSpectators = 0;
		Server.GameVer = 0;
		Server.MinNetVer = 0;

		Temp = ParseReply(Text, "hostname");
		if(Temp != "" && !Server.bKeepDescription)
			Server.HostName = Temp;

		Temp = ParseReply(Text, "hostport");
		if(Temp != "")
			Server.GamePort = Int(Temp);

		Temp = ParseReply(Text, "mapname");
		if(Temp != "")
			Server.MapName = Temp;

		Temp = ParseReply(Text, "maptitle");
		if(Temp != "")
		{
			Server.MapTitle = Temp;
			Server.MapDisplayName = Server.MapTitle;
			if(Server.MapTitle == "" || Server.MapTitle ~= "Untitled" || bUseMapName)
				Server.MapDisplayName = Server.MapName;
		}
		
		Temp = ParseReply(Text, "gametype");
		if(Temp != "")
			Server.GameType = Temp;
	
		Temp = ParseReply(Text, "numplayers");
		if(Temp != "")
		{
			Server.NumPlayers = Int(Temp);
			Server.ReportedPlayersCount = Server.NumPlayers;
		}
		
		Temp = ParseReply(Text, "NumPlayers");
		if(Temp != "" && string(Int(Temp)) == Temp)
			Server.NumPlayers = Int(Temp);

		Temp = ParseReply(Text, "maxplayers");
		if(Temp != "")
			Server.MaxPlayers = Int(Temp);
		
		Temp = ParseReply(Text, "NumSpectators");
		if(Temp != "")
			Server.NumSpectators = Int(Temp);

		Temp = ParseReply(Text, "MaxSpectators");
		if(Temp != "")
			Server.MaxSpectators = Int(Temp);
	
		Temp = ParseReply(Text, "gamemode");
		if(Temp != "")
			Server.GameMode = Temp;

		Temp = ParseReply(Text, "gamever");
		if(Temp != "")
			Server.GameVer = Int(Temp);

		Temp = ParseReply(Text, "minnetver");
		if(Temp != "")
			Server.MinNetVer = Int(Temp);

		if( Server.DecodeServerProperties(Text) )
		{
			Server.PingDone(bInitial, bJustThisServer, True, bNoSort);
			Disable('Tick');
		}
	}

	event Tick(Float DeltaTime)
	{
		LastDelta = DeltaTime;
	}

	event Timer()
	{
		ValidateServer();
		if(AttemptNumber < PingAttempts)
		{
			Log("Ping Timeout from "$Server.IP$".  Attempt "$AttemptNumber);
			AttemptNumber++;
			GotoState(QueryState);
		}
		else
		{
			Log("Ping Timeout from "$Server.IP$" Giving Up");

			Server.Ping = 9999;
			Server.GamePort = 0;
			Server.MapName = "";
			Server.MapDisplayName = "";
			Server.MapTitle = "";
			Server.GameType = "";
			Server.GameMode = "";
			Server.NumPlayers = 0;
			Server.ReportedPlayersCount = 0;
			Server.MaxPlayers = 0;
			Server.NumSpectators = 0;
			Server.MaxSpectators = 0;

			Disable('Tick');

			Server.PingDone(bInitial, bJustThisServer, False, bNoSort);
		}
	}

	function SendRequest()
	{
		Enable('Tick');
		RequestSentTime = Level.TimeSeconds;
		SendText( ServerIPAddr, "\\info\\" $ 
			"\\game_property\\NumPlayers\\" $
			"\\game_property\\NumSpectators\\" $
			"\\game_property\\MaxSpectators\\" $
			// Bug fix for XServerQuery <= 2.00.  Without a
			// known command at the end, XServerQuery will not
			// respond, and the server browser will mark the server
			// as offline.
			"\\echo\\");
		SetTimer(PingTimeout + FRand(), False);
	}

Begin:
	SendRequest();
}

state Resolving
{
Begin:
	Resolve( Server.IP );
}

defaultproperties
{
	AdminEmailText="Admin Email"
	AdminNameText="Admin Name"
	MultiplayerBotsText="Bots in Multiplayer"
	FragLimitText="Frag Limit"
	TimeLimitText="Time Limit"
	GameModeText="Game Mode"
	GameTypeText="Game Type"
	GameVersionText="Game Version"
	MinNetVersionText="Min. Compatible Version"
	MutatorsText="Mutators"
	GoalTeamScoreText="Required Team Score"
	MinPlayersText="Bots Enter Game for Min. of"
	PlayersText="Players"
	ChangeLevelsText="Change Levels"
	MaxTeamsText="Max Teams"
	BalanceTeamsText="Bots Balance Teams"
	PlayersBalanceTeamsText="Force Team Balance"
	FriendlyFireText="Friendly Fire Damage"
	BotSkillText="Bot Skill"
	TournamentText="Tournament Mode"
	TrueString="Enabled"
	FalseString="Disabled"
	MaxBindAttempts=5
	BindRetryTime=10
	PingTimeout=1
	ServerAddressText="Server Address"
	ServerModeText="Server Mode"
	DedicatedText="Dedicated"
	NonDedicatedText="Non-Dedicated"
	WorldLogText="ngWorldStats"
	WorldLogWorkingText="ngWorldStats Status"
	WorldLogWorkingTrue="Processing Stats Correctly"
	WorldLogWorkingFalse="Not Processing Stats Correctly"
	PasswordText="Requires Password"
	TeamNameText="Team"
	MapAuthorText="Map Author"
	MapTitleText="Map Title"
	IdealPlayerCountText="Ideal Players Count"
	NumBotsText="Count Bots"
	OverTimeText="Overtime"
	RemainingTimeText="Remaining Time"
	GameSpeedText="Game Speed"
	AirControlText="Air Control"
	WeaponStayText="Weapons Stay"
	UseTranslocatorText="Allow Translocator"
	MonsterSkillText="Monsters Skill"
	MonstersTotalText="Monsters Total Count"
	UseLivesText="Use Lives"
	LivesText="Lives"
	TextEncoding=TEXTENC_Native	
}
