class UBrowserServerListWindow extends UWindowPageWindow
	PerObjectConfig;

var config string				ServerListTitle;	// Non-localized page title
var config string				ListFactories[20];
var config string				URLAppend;
var config int					AutoRefreshTime;
var config bool					bNoAutoSort;
var config bool					bHidden;
var config bool					bFallbackFactories;

var config string				HiddenTypes[256];

var config string				FilterString;
var string						FilterStringCaps;
var config bool					bNoEmpty;
var config bool					bNoFull;
var config bool					bNoLocked;

var string						ServerListClassName;
var class<UBrowserServerList>	ServerListClass;

var UBrowserServerList			PingedList;
var UBrowserServerList			HiddenList;
var UBrowserServerList			UnpingedList;

var UBrowserServerListFactory	Factories[ArrayCount(ListFactories)];
var int							QueryDone[ArrayCount(ListFactories)];
var UBrowserServerGrid			Grid;
var string						GridClass;
var float						TimeElapsed;
var bool						bPingSuspend;
var bool						bPingResume;
var bool						bPingResumeIntial;
var bool						bNoSort;
var bool						bSuspendPingOnClose;
var UBrowserSubsetList			SubsetList;
var UBrowserSupersetList		SupersetList;
var class<UBrowserRightClickMenu>	RightClickMenuClass;
var bool						bShowFailedServers;
var bool						bHadInitialRefresh;
var int							FallbackFactory;

var UWindowEditControl			Filter;
var UWindowCheckbox				Empty;
var UWindowCheckbox				Full;
var UWindowCheckbox				Locked;

var UWindowHSplitter			HSplitter;
var UBrowserTypesGrid			TypesGrid;
var UBrowserTypesList			TypesList;

var UWindowVSplitter			VSplitter;
var UBrowserInfoWindow			InfoWindow;
var UBrowserInfoClientWindow	InfoClient;
var UBrowserServerList			InfoItem;
var localized string			InfoName;

const MinHeightForSplitter = 384;
const GapTop = 30;

var() Color						WhiteColor;

var localized string			PlayerCountLeader;
var localized string			ServerCountLeader;

var() localized string			FilterText;
var() localized string			FilterHelp;
var() localized string			EmptyText;
var() localized string			EmptyHelp;
var() localized string			FullText;
var() localized string			FullHelp;
var() localized string			LockedText;
var() localized string			LockedHelp;

// Status info
enum EPingState
{
	PS_QueryServer,
	PS_QueryFailed,
	PS_Pinging,
	PS_RePinging,
	PS_Done
};

var localized string			PlayerCountName;
var localized string			ServerCountName;
var	localized string			QueryServerText;
var	localized string			QueryFailedText;
var	localized string			PingingText;
var	localized string			CompleteText;

var string						ErrorString;
var EPingState					PingState;

function WindowShown()
{
	local UBrowserSupersetList l;

	Super.WindowShown();
	if(VSplitter.bWindowVisible)
	{
		if(UWindowVSplitter(InfoClient.ParentWindow) != None)
			VSplitter.SplitPos = UWindowVSplitter(InfoClient.ParentWindow).SplitPos;

		InfoClient.SetParent(VSplitter);
	}

	InfoClient.Server = InfoItem;
	if(InfoItem != None)
		InfoWindow.WindowTitle = InfoName$" - "$InfoItem.HostName;
	else
		InfoWindow.WindowTitle = InfoName;

	ResumePinging();

	for(l = UBrowserSupersetList(SupersetList.Next); l != None; l = UBrowserSupersetList(l.Next))
		l.SuperSetWindow.ResumePinging();
}

function WindowHidden()
{
	local UBrowserSupersetList l;

	Super.WindowHidden();
	SuspendPinging();

	for(l = UBrowserSupersetList(SupersetList.Next); l != None; l = UBrowserSupersetList(l.Next))
		l.SuperSetWindow.SuspendPinging();
}

function SuspendPinging()
{
	if(bSuspendPingOnClose)
		bPingSuspend = True;
}

function ResumePinging()
{
	if(!bHadInitialRefresh)
		Refresh(False, True);	

	bPingSuspend = False;
	if(bPingResume)
	{
		bPingResume = False;
		UnpingedList.PingNext(bPingResumeIntial, bNoSort);
	}
}

function SaveFilters()
{
	local int i;
	local UBrowserTypesList RulesList;

	for(RulesList = UBrowserTypesList(TypesList.Next); RulesList != None; RulesList = UBrowserTypesList(RulesList.Next))
		if(!RulesList.bShow)
		{
			HiddenTypes[i++] = RulesList.Type;
			if (i == ArrayCount(HiddenTypes))
				break;
		}

	for(i = i; i < ArrayCount(HiddenTypes); i++)
		HiddenTypes[i] = "";

	FilterString = Filter.GetValue();
	bNoEmpty = !Empty.bChecked;
	bNoFull = !Full.bChecked;
	bNoLocked = !Locked.bChecked;

	FilterStringCaps = Caps(FilterString);

	SaveConfig();
}

function Created()
{
	local Class<UBrowserServerGrid> C;
	local int i;
	local UBrowserTypesList RulesList;
	
	ServerListClass = class<UBrowserServerList>(DynamicLoadObject(ServerListClassName, class'Class'));
	C = class<UBrowserServerGrid>(DynamicLoadObject(GridClass, class'Class'));
	Grid = UBrowserServerGrid(CreateWindow(C, 0, 0, WinWidth, WinHeight));
	Grid.SetAcceptsFocus();

	SubsetList = new class'UBrowserSubsetList';
	SubsetList.SetupSentinel();

	SupersetList = new class'UBrowserSupersetList';
	SupersetList.SetupSentinel();

	TypesList = New(None) class'UBrowserTypesList';
	TypesList.SetupSentinel(true);

	for(i = 0; i < ArrayCount(HiddenTypes); i++)
	{
		if (HiddenTypes[i] != "")
		{
			RulesList = new(None) class'UBrowserTypesList';
			RulesList.Type = HiddenTypes[i];
			TypesList.AppendItem(RulesList);
		}
	}

	Empty = UWindowCheckbox(CreateControl(class'UWindowCheckbox', 0, 0, WinWidth, 1));
	Empty.bChecked = !bNoEmpty;
	Empty.SetText(EmptyText);
	Empty.SetHelpText(EmptyHelp);
	Empty.SetFont(F_Normal);
	Empty.Align = TA_Right;
	Empty.TextColor = WhiteColor;

	Full = UWindowCheckbox(CreateControl(class'UWindowCheckbox', 0, 0, WinWidth, 1));
	Full.bChecked = !bNoFull;
	Full.SetText(FullText);
	Full.SetHelpText(FullHelp);
	Full.SetFont(F_Normal);
	Full.Align = TA_Right;
	Full.TextColor = WhiteColor;

	Locked = UWindowCheckbox(CreateControl(class'UWindowCheckbox', 0, 0, WinWidth, 1));
	Locked.bChecked = !bNoLocked;
	Locked.SetText(LockedText);
	Locked.SetHelpText(LockedHelp);
	Locked.SetFont(F_Normal);
	Locked.Align = TA_Right;
	Locked.TextColor = WhiteColor;

	Filter = UWindowEditControl(CreateControl(class'UWindowEditControl', 0, 0, WinWidth, 1));
	Filter.SetText(FilterText);
	Filter.SetHelpText(FilterHelp);
	Filter.SetFont(F_Normal);
	Filter.Align = TA_Left;
	Filter.TextColor = WhiteColor;
	Filter.EditAreaDrawX = 40;
	Filter.SetValue(FilterString); // must be last

	TypesGrid = UBrowserTypesGrid(CreateWindow(class'UBrowserTypesGrid', 0, 0, WinWidth, WinHeight));
	TypesGrid.SetAcceptsFocus();

	HSplitter = UWindowHSplitter(CreateWindow(class'UWindowHSplitter', 0, 0, WinWidth, WinHeight));
	TypesGrid.SetParent(HSplitter);
	Grid.SetParent(HSplitter);
	HSplitter.LeftClientWindow = TypesGrid;
	HSplitter.RightClientWindow = Grid;
	HSplitter.MinWinWidth = 60;
	HSplitter.bRightGrow = true;
	HSplitter.OldWinWidth = HSplitter.WinWidth;
	HSplitter.SplitPos = 180;
	HSplitter.SetAcceptsFocus();
	HSplitter.ShowWindow();

	VSplitter = UWindowVSplitter(CreateWindow(class'UWindowVSplitter', 0, 0, WinWidth, WinHeight));
	VSplitter.SetAcceptsFocus();
	VSplitter.MinWinHeight = 60;
	VSplitter.HideWindow();
	InfoWindow = UBrowserMainClientWindow(GetParent(class'UBrowserMainClientWindow')).InfoWindow;
	InfoClient = UBrowserInfoClientWindow(InfoWindow.ClientArea);

	ShowInfoArea(True, Root.WinHeight < MinHeightForSplitter);
}

function ShowInfoArea(bool bShow, optional bool bFloating, optional bool bNoActivate)
{
	if(bShow)
	{
		if(bFloating)
		{
			VSplitter.HideWindow();
			VSplitter.TopClientWindow = None;
			VSplitter.BottomClientWindow = None;
			InfoClient.SetParent(InfoWindow);
			HSplitter.SetParent(Self);
			HSplitter.WinTop = GapTop;
			HSplitter.SetSize(WinWidth, WinHeight - GapTop);
			if(!InfoWindow.bWindowVisible)
				InfoWindow.ShowWindow();
			if(!bNoActivate)
				InfoWindow.BringToFront();
		}
		else
		{
			InfoWindow.HideWindow();
			VSplitter.ShowWindow();
			VSplitter.WinTop = GapTop;
			VSplitter.SetSize(WinWidth, WinHeight - GapTop);
			HSplitter.SetParent(VSplitter);
			HSplitter.WinTop = 0;
			InfoClient.SetParent(VSplitter);
			VSplitter.TopClientWindow = HSplitter;
			VSplitter.BottomClientWindow = InfoClient;
		}
	}
	else
	{
		InfoWindow.HideWindow();
		VSplitter.HideWindow();
		VSplitter.TopClientWindow = None;
		VSplitter.BottomClientWindow = None;
		InfoClient.SetParent(InfoWindow);
		HSplitter.SetParent(Self);
		HSplitter.WinTop = GapTop;
		HSplitter.SetSize(WinWidth, WinHeight - GapTop);
	}
}

function AutoInfo(UBrowserServerList I)
{
	if(Root.WinHeight >= MinHeightForSplitter || InfoWindow.bWindowVisible)
		ShowInfo(I, True);
}

function ShowInfo(UBrowserServerList I, optional bool bAutoInfo)
{
	if(I == None) return;
	ShowInfoArea(True, Root.WinHeight < MinHeightForSplitter, bAutoInfo);

	InfoItem = I;
	InfoClient.Server = InfoItem;
	InfoWindow.WindowTitle = InfoName$" - "$InfoItem.HostName;
	I.ServerStatus();
}

function ResolutionChanged(float W, float H)
{
	if(Root.WinHeight >= MinHeightForSplitter)
		ShowInfoArea(True, False);
	else
		ShowInfoArea(True, True);
	
	if(InfoWindow != None)
		InfoWindow.ResolutionChanged(W, H);

	Super.ResolutionChanged(W, H);
}

function Resized()
{
	Super.Resized();
	if(VSplitter.bWindowVisible)
	{
		VSplitter.SetSize(WinWidth, WinHeight - VSplitter.WinTop);
		VSplitter.OldWinHeight = VSplitter.WinHeight;
		VSplitter.SplitPos = VSplitter.WinHeight - Min(VSplitter.WinHeight / 1.5, 600);
	}
	else
		Grid.SetSize(WinWidth, WinHeight);
}

function Notify(UWindowDialogControl C, byte E)
{
	Super.Notify(C, E);

	switch(E)
	{
	case DE_Change:
		switch(C)
		{
		case Filter:
		case Empty:
		case Full:
		case Locked:
			SaveFilters();
			break;
		}
		break;
	}
}

function AddSubset(UBrowserSubsetFact Subset)
{
	local UBrowserSubsetList l;

	for(l = UBrowserSubsetList(SubsetList.Next); l != None; l = UBrowserSubsetList(l.Next))
		if(l.SubsetFactory == Subset)
			return;
	
	l = UBrowserSubsetList(SubsetList.Append(class'UBrowserSubsetList'));
	l.SubsetFactory = Subset;
}

function AddSuperSet(UBrowserServerListWindow Superset)
{
	local UBrowserSupersetList l;

	for(l = UBrowserSupersetList(SupersetList.Next); l != None; l = UBrowserSupersetList(l.Next))
		if(l.SupersetWindow == Superset)
			return;
	
	l = UBrowserSupersetList(SupersetList.Append(class'UBrowserSupersetList'));
	l.SupersetWindow = Superset;
}

function RemoveSubset(UBrowserSubsetFact Subset)
{
	local UBrowserSubsetList l;

	for(l = UBrowserSubsetList(SubsetList.Next); l != None; l = UBrowserSubsetList(l.Next))
		if(l.SubsetFactory == Subset)
			l.Remove();
}

function RemoveSuperset(UBrowserServerListWindow Superset)
{
	local UBrowserSupersetList l;

	for(l = UBrowserSupersetList(SupersetList.Next); l != None; l = UBrowserSupersetList(l.Next))
		if(l.SupersetWindow == Superset)
			l.Remove();
}

function UBrowserServerList AddFavorite(UBrowserServerList Server)
{
	return UBrowserServerListWindow(UBrowserMainClientWindow(GetParent(class'UBrowserMainClientWindow')).Favorites.Page).AddFavorite(Server);
}

function Refresh(optional bool bBySuperset, optional bool bInitial, optional bool bSaveExistingList, optional bool bInNoSort)
{
	bHadInitialRefresh = True;

	if(!bSaveExistingList)
	{
		InfoItem = None;
		InfoClient.Server = None;
	}

	if(!bSaveExistingList && PingedList != None)
	{
		PingedList.DestroyList();
		PingedList = None;
		Grid.SelectedServer = None;

		HiddenList.DestroyList();
		HiddenList = None;
	}

	if(PingedList == None)
	{
		PingedList=New ServerListClass;
		PingedList.Owner = Self;
		PingedList.SetupSentinel(True);
		PingedList.bSuspendableSort = True;

		HiddenList=New ServerListClass;
		HiddenList.Owner = Self;
		HiddenList.SetupSentinel();
	}
	else
	{
		PingedList.AppendListCopy(HiddenList);
		HiddenList.Clear();
		TagServersAsOld();
	}

	if(UnpingedList != None)
		UnpingedList.DestroyList();
	
	if(!bSaveExistingList)
	{
		UnpingedList = New ServerListClass;
		UnpingedList.Owner = Self;
		UnpingedList.SetupSentinel(False);
	}

	PingState = PS_QueryServer;
	ShutdownFactories(bBySuperset);
	CreateFactories(bSaveExistingList);
	Query(bBySuperset, bInitial, bInNoSort);

	if(!bInitial)
		RefreshSubsets();
}

function TagServersAsOld()
{
	local UBrowserServerList l;

	for(l = UBrowserServerList(PingedList.Next);l != None;l = UBrowserServerList(l.Next)) 
		l.bOldServer = True;
}

function RemoveOldServers()
{
	local UBrowserServerList l, n;

	l = UBrowserServerList(PingedList.Next);
	while(l != None) 
	{
		n = UBrowserServerList(l.Next);

		if(l.bOldServer)
		{
			if(Grid.SelectedServer == l)
				Grid.SelectedServer = n;

			l.Remove();
		}
		l = n;
	}
}

function RefreshSubsets()
{
	local UBrowserSubsetList l, NextSubset;

	for(l = UBrowserSubsetList(SubsetList.Next); l != None; l = UBrowserSubsetList(l.Next))
		l.bOldElement = True;

	l = UBrowserSubsetList(SubsetList.Next);
	while(l != None && l.bOldElement)
	{
		NextSubset = UBrowserSubsetList(l.Next);
		l.SubsetFactory.Owner.Owner.Refresh(True);
		l = NextSubset;
	}
}

function RePing()
{
	PingState = PS_RePinging;
	PingedList.InvalidatePings();
	PingedList.PingServers(True, False);
}

function QueryFinished(UBrowserServerListFactory Fact, bool bSuccess, optional string ErrorMsg)
{
	local int i;
	local bool bDone;

	bDone = True;
	for(i=0;i<ArrayCount(Factories);i++)
	{
		if(Factories[i] != None)
		{
			if(Factories[i] == Fact)
				QueryDone[i] = 1;
			if(QueryDone[i] == 0)
				bDone = False;
		}
	}

	if(!bSuccess)
	{
		PingState = PS_QueryFailed;
		ErrorString = ErrorMsg;

		// don't ping and report success if we have no servers.
		if(bDone && UnpingedList.Count() == 0)
		{
			if( bFallbackFactories )
			{
				FallbackFactory++;
				if( FallbackFactory < ArrayCount(ListFactories) && ListFactories[FallbackFactory] != "" )
					Refresh();	// try the next fallback master server
				else
					FallbackFactory = 0;
			}
			return;
		}
	}
	else
		ErrorString = "";

	if(bDone)
	{
		RemoveOldServers();

		PingState = PS_Pinging;
		if(!bNoSort && !Fact.bIncrementalPing)
			PingedList.Sort();
		UnpingedList.PingServers(True, bNoSort || Fact.bIncrementalPing);
	}
}

function PingFinished()
{
	PingState = PS_Done;
}

function CreateFactories(bool bUsePingedList)
{
	local int i;

	for(i=0;i<ArrayCount(ListFactories);i++)
	{
		if(ListFactories[i] == "")
			break;
		if(!bFallbackFactories || FallbackFactory == i)
		{
			Factories[i] = UBrowserServerListFactory(BuildObjectWithProperties(ListFactories[i]));
			
			Factories[i].PingedList = PingedList;
			Factories[i].UnpingedList = UnpingedList;
		
			if(bUsePingedList)
				Factories[i].Owner = PingedList;
			else
				Factories[i].Owner = UnpingedList;
		}
		QueryDone[i] = 0;
	}	
}

function ShutdownFactories(optional bool bBySuperset)
{
	local int i;

	for(i=0;i<ArrayCount(Factories);i++)
	{
		if(Factories[i] != None) 
		{
			Factories[i].Shutdown(bBySuperset);
			Factories[i] = None;
		}
	}	
}

function Query(optional bool bBySuperset, optional bool bInitial, optional bool bInNoSort)
{
	local int i;

	bNoSort = bInNoSort;

	// Query all our factories
	for(i=0;i<ArrayCount(Factories);i++)
	{
		if(Factories[i] != None)
			Factories[i].Query(bBySuperset, bInitial);
	}
}

function Paint(Canvas C, float X, float Y)
{
	DrawStretchedTexture(C, 0, 0, WinWidth, WinHeight, Texture'BlackTexture');
}

function Tick(float Delta)
{
	PingedList.Tick(Delta);

	if(PingedList.bNeedUpdateCount)
	{
		PingedList.UpdateServerCount();
		PingedList.bNeedUpdateCount = False;
	}

	// AutoRefresh local servers
	if(AutoRefreshTime > 0)
	{
		TimeElapsed += Delta;
		
		if(TimeElapsed > AutoRefreshTime)
		{
			TimeElapsed = 0;
			Refresh(,,True, bNoAutoSort);
		}
	}	
}

function BeforePaint(Canvas C, float X, float Y)
{
	local UBrowserMainWindow W;
	local UBrowserSupersetList l;
	local EPingState P;
	local int PercentComplete;
	local int TotalReturnedServers;
	local string E;
	local int TotalServers;
	local int PingedServers;
	local int MyServers;
	local int ControlTop, ControlLeft, ContolRight;

	const ControlHeight = 18;

	Super.BeforePaint(C, X, Y);

	ControlTop = (GapTop - ControlHeight)/2;
	ControlLeft = ControlHeight/2;
	ContolRight = ControlLeft;
	
	Locked.SetSize(50, ControlHeight);
	Locked.WinTop = ControlTop;
	Locked.WinLeft = WinWidth - Locked.WinWidth - ContolRight;
	ContolRight += Locked.WinWidth + ControlHeight;

	Full.SetSize(40, ControlHeight);
	Full.WinTop = ControlTop;
	Full.WinLeft = WinWidth - Full.WinWidth - ContolRight;
	ContolRight += Full.WinWidth + ControlHeight;

	Empty.SetSize(50, ControlHeight);
	Empty.WinTop = ControlTop;
	Empty.WinLeft = WinWidth - Empty.WinWidth - ContolRight;
	ContolRight += Empty.WinWidth + ControlHeight;

	Filter.SetSize(WinWidth - ContolRight - ControlLeft, ControlHeight);
	Filter.WinTop = ControlTop;
	Filter.WinLeft = ControlLeft;
	Filter.EditBoxWidth = Filter.WinWidth - Filter.EditAreaDrawX;

	W = UBrowserMainWindow(GetParent(class'UBrowserMainWindow'));
	l = UBrowserSupersetList(SupersetList.Next);

	if(l != None && PingState != PS_RePinging)
	{
		P = l.SupersetWindow.PingState;
		PingState = P;

		if(P == PS_QueryServer)
			TotalReturnedServers = l.SupersetWindow.UnpingedList.Count();

		PingedServers = l.SupersetWindow.PingedList.Count();
		TotalServers = l.SupersetWindow.UnpingedList.Count() + PingedServers;
		MyServers = PingedList.Count();
	
		E = l.SupersetWindow.ErrorString;
	}
	else
	{
		P = PingState;
		if(P == PS_QueryServer)
			TotalReturnedServers = UnpingedList.Count();

		PingedServers = PingedList.Count();
		TotalServers = UnpingedList.Count() + PingedServers;
		MyServers = PingedList.Count();

		E = ErrorString;
	}

	if(TotalServers > 0)
		PercentComplete = PingedServers*100.0/TotalServers;

	switch(P)
	{
	case PS_QueryServer:
		if(TotalReturnedServers > 0)
			W.DefaultStatusBarText(QueryServerText$" ("$ServerCountLeader$TotalReturnedServers$" "$ServerCountName$")");
		else
			W.DefaultStatusBarText(QueryServerText);
		break;
	case PS_QueryFailed:
		W.DefaultStatusBarText(QueryFailedText$E);
		break;
	case PS_Pinging:
	case PS_RePinging:
		W.DefaultStatusBarText(PingingText$" "$PercentComplete$"% "$CompleteText$". "$ServerCountLeader$MyServers$" "$ServerCountName$", "$PlayerCountLeader$PingedList.TotalPlayers$" "$PlayerCountName);
		break;
	case PS_Done:
		W.DefaultStatusBarText(ServerCountLeader$MyServers$" "$ServerCountName$", "$PlayerCountLeader$PingedList.TotalPlayers$" "$PlayerCountName);
		break;
	}

	ApplyTypeFilter(HiddenList, PingedList);
	ApplyTypeFilter(PingedList, HiddenList);
}

function ApplyTypeFilter(UBrowserServerList From, UBrowserServerList To)
{
	local UBrowserServerList List, Item;
	local UBrowserTypesList  RulesList;
	local bool bShow, bSkip;
	local string Chr13;

	Chr13 = Chr(13);
	if(From != HiddenList)
		bShow = true;
	else
		for(RulesList = UBrowserTypesList(TypesList.Next); RulesList != None; RulesList = UBrowserTypesList(RulesList.Next))
			RulesList.iCount = 0;
	List = UBrowserServerList(From.Next);
	while(List != None)
	{
		Item = List;
		List = UBrowserServerList(List.Next);

		bSkip = false;
		if (bNoEmpty && Item.NumPlayers == 0)
			bSkip = true;
		if (!bSkip && bNoFull && Item.NumPlayers == Item.MaxPlayers)
			bSkip = true;
		if (!bSkip && bNoLocked && Item.bLocked)
			bSkip = true;
		if (!bSkip && FilterStringCaps != "" && InStr(Caps(
			Item.HostName $ Chr13 $ 
			Item.MapName $ Chr13 $ 
			Item.MapTitle $ Chr13 $ 
			Item.MapDisplayName $ Chr13 $
			Item.GameType $ Chr13 $
			Item.GameMode $ Chr13
			) $ Item.FilterString, FilterStringCaps) == -1)
			bSkip = true;
		if(bSkip)
		{
			if(bShow)
			{
				Item.Remove();
				To.AppendItem(Item);
			}
			goto NextServer;
		}

		for(RulesList = UBrowserTypesList(TypesList.Next); RulesList != None; RulesList = UBrowserTypesList(RulesList.Next))
		{
			if(RulesList.Type ~= Item.GameType)
			{
				if(RulesList.bShow == bShow)
					RulesList.iCount++;
				else					
				{
					Item.Remove();
					To.AppendItem(Item);
				}

				goto NextServer; // Rule already exists
			}
		}

		// Add the rule
		RulesList = new(None) class'UBrowserTypesList';
		RulesList.Type = Item.GameType;
		RulesList.bShow = True;
		TypesList.AppendItem(RulesList);
		if(From == HiddenList)
		{
			Item.Remove();
			To.AppendItem(Item);
		}
		NextServer:
	}
}

defaultproperties
{
	GridClass="UBrowser.UBrowserServerGrid";
	bSuspendPingOnClose=True
	PlayerCountName="Players"
	PlayerCountLeader=""
	ServerCountName="Servers"
	ServerCountLeader=""
	QueryServerText="Querying master server (hit F5 if nothing happens)"
	QueryFailedText="Master Server Failed: "
	PingingText="Pinging Servers"
	CompleteText="Complete"
	ServerListClassName="UBrowser.UBrowserServerList"
	RightClickMenuClass=class'UBrowserRightClickMenu'
	bShowFailedServers=False
	InfoName="Info"
	bFallbackFactories=False
	FallbackFactory=0
	WhiteColor=(R=255,G=255,B=255)
	FilterText="Filter"
	FilterHelp="Filter servers by name, map, mutator, player name and so on."
	EmptyText="Empty"
	EmptyHelp="Show empty servers."
	FullText="Full"
	FullHelp="Show full servers."
	LockedText="Locked"
	LockedHelp="Show password-protected servers."
}
