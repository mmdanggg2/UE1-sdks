class UMenuMenuBar extends UWindowMenuBar;

var UWindowPulldownMenu Game, Multiplayer, Stats, Tool, Help;
var UMenuModMenu Mods;
var UMenuOptionsMenu Options;

var UWindowMenuBarItem GameItem, MultiplayerItem, OptionsItem, StatsItem, ToolItem, HelpItem, ModItem;

var UWindowMenuBarItem OldHelpItem;

var UMenuHelpWindow HelpWindow;
var config bool ShowHelp;

var UWindowMenuBarItem OldSelected;
var string VersionText;
var bool bShowMenu;

var localized string GameName;
var localized string GameHelp;
var localized string MultiplayerName;
var localized string MultiplayerHelp;
var localized string OptionsName;
var localized string OptionsHelp;
var localized string StatsName;
var localized string StatsHelp;
var localized string ToolName;
var localized string ToolHelp;
var localized string HelpName;
var localized string HelpHelp;
var localized string VersionName;
var localized string ModName;
var localized string ModHelp;

var UMenuModMenuList ModItems;

var config string GameUMenuDefault;
var config string MultiplayerUMenuDefault;
var config string OptionsUMenuDefault;
var config string ModMenuClass;

function Class<UWindowPulldownMenu> GetMenuType(string ClassName)
{
	return Class<UWindowPulldownMenu>(DynamicLoadObject(ClassName, class'Class'));
}

function Created()
{
	local Class<UWindowPulldownMenu> GameUMenuType;
	local Class<UWindowPulldownMenu> MultiplayerUMenuType;
	local Class<UWindowPulldownMenu> OptionsUMenuType;
	local Class<UMenuModMenu> UMenuModMenuType;

	Super.Created();

	bAlwaysOnTop = True;

	GameItem = AddItem(GameName);
	if (GetLevel().Game != None)
		GameUMenuType = GetMenuType(GetLevel().Game.Default.GameUMenuType);
	if (GameUMenuType == None)
		GameUMenuType = GetMenuType(GameUMenuDefault);
	if (GameUMenuType == None)
		GameUMenuType = GetMenuType("UTMenu.UTGameMenu");
	Game = GameItem.CreateMenu(GameUMenuType);

	MultiplayerItem = AddItem(MultiplayerName);
	if(GetLevel().Game != None)
		MultiplayerUMenuType = GetMenuType(GetLevel().Game.Default.MultiplayerUMenuType);
	if (MultiplayerUMenuType == None)
		MultiplayerUMenuType = GetMenuType(MultiplayerUMenuDefault);
	if (MultiplayerUMenuType == None)
		MultiplayerUMenuType = GetMenuType("UTMenu.UTMultiplayerMenu");
	Multiplayer = MultiplayerItem.CreateMenu(MultiplayerUMenuType);

	OptionsItem = AddItem(OptionsName);
	if(GetLevel().Game != None)
		OptionsUMenuType = GetMenuType(GetLevel().Game.Default.GameOptionsMenuType);
	if (OptionsUMenuType == None)
		OptionsUMenuType = GetMenuType(OptionsUMenuDefault);
	if (OptionsUMenuType == None)
		OptionsUMenuType = GetMenuType("UTMenu.UTOptionsMenu");
	Options = UMenuOptionsMenu(OptionsItem.CreateMenu(OptionsUMenuType));

//	StatsItem = AddItem(StatsName);
//	Stats = StatsItem.CreateMenu(class'UMenuStatsMenu');

	ToolItem = AddItem(ToolName);
	Tool = ToolItem.CreateMenu(class'UMenuToolsMenu');

	if(LoadMods())
	{
		ModItem = AddItem(ModName);
		UMenuModMenuType = class<UMenuModMenu>(DynamicLoadObject(ModMenuClass, class'class'));
		if (UMenuModMenuType == None)
			UMenuModMenuType = Class'UMenuModMenu';
		Mods = UMenuModMenu(ModItem.CreateMenu(UMenuModMenuType));
		Mods.SetupMods(ModItems);
	}

	HelpItem = AddItem(HelpName);
	Help = HelpItem.CreateMenu(class'UMenuHelpMenu');

	UMenuHelpMenu(Help).Context.bChecked = ShowHelp;
	if (ShowHelp)
	{
		if(UMenuRootWindow(Root) != None)
			if(UMenuRootWindow(Root).StatusBar != None)
				UMenuRootWindow(Root).StatusBar.ShowWindow();
	}

	bShowMenu = True;

	Spacing = 12;
}

function SetHelp(string NewHelpText)
{
	if(UMenuRootWindow(Root) != None)
		if(UMenuRootWindow(Root).StatusBar != None)
			UMenuRootWindow(Root).StatusBar.SetHelp(NewHelpText);
}

function CloseUp()
{
	OldSelected = None;
	Super.CloseUp();
	ShowHelpItem(OldHelpItem);
}

function HideWindow()
{
	if(UMenuRootWindow(Root) != None)
		if(UMenuRootWindow(Root).StatusBar != None)
			UMenuRootWindow(Root).StatusBar.HideWindow();
	Super.HideWindow();
}

function ShowWindow()
{
	if (ShowHelp)
	{
		if(UMenuRootWindow(Root) != None)
			if(UMenuRootWindow(Root).StatusBar != None)
				UMenuRootWindow(Root).StatusBar.ShowWindow();
	}
	Super.ShowWindow();
}


function ShowHelpItem(UWindowMenuBarItem I)
{
	switch(I)
	{
	case GameItem:
		SetHelp(GameHelp);
		break;	
	case MultiplayerItem:
		SetHelp(MultiplayerHelp);
		break;	
	case OptionsItem:
		SetHelp(OptionsHelp);
		break;	
//	case StatsItem:
//		SetHelp(StatsHelp);
//		break;
	case ToolItem:
		SetHelp(ToolHelp);
		break;	
	case HelpItem:
		SetHelp(HelpHelp);
		break;	
	case ModItem:
		SetHelp(ModHelp);
	default:
		SetHelp("");
		break;	
	}
}

function Select(UWindowMenuBarItem I)
{
	Super.Select(I);
	OldSelected = I;

	ShowHelpItem(I);
	Super.Select(I);
}

function BeforePaint(Canvas C, float X, float Y)
{
	Super.BeforePaint(C, X, Y);

	if(Over != OldHelpItem)
	{
		OldHelpItem = Over;
		ShowHelpItem(Over);
	}


	if(bShowMenu)
	{
		// pull the game menu down first time menu is created
		Selected = GameItem;
		Selected.Select();
		Select(Selected);
		bShowMenu = False;
	}
}

function DrawItem(Canvas C, UWindowList Item, float X, float Y, float W, float H)
{
	C.DrawColor.R = 255;
	C.DrawColor.G = 255;
	C.DrawColor.B = 255;
	
	if(UWindowMenuBarItem(Item).bHelp) W = W - 16;

	UWindowMenuBarItem(Item).ItemLeft = X;
	UWindowMenuBarItem(Item).ItemWidth = W;

	LookAndFeel.Menu_DrawMenuBarItem(Self, UWindowMenuBarItem(Item), X, Y, W, H, C);
}

function DrawMenuBar(Canvas C)
{
	local float W, H;

	VersionText = VersionName@GetLevel().EngineVersion$GetLevel().EngineRevision;

	LookAndFeel.Menu_DrawMenuBar(Self, C);

	C.Font = Root.Fonts[F_Normal];
	C.DrawColor.R = 0;
	C.DrawColor.G = 0;
	C.DrawColor.B = 0;

	TextSize(C, VersionText, W, H);
	ClipText(C, WinWidth - W - 20, 1.5, VersionText);
}

function LMouseDown(float X, float Y)
{
	if(X > WinWidth - 13) GetPlayerOwner().ConsoleCommand("togglefullscreen");
	Super.LMouseDown(X, Y);
}

function bool LoadMods()
{
	local int NumModClasses;
	local string NextModClass, NextModDesc;
	local int i;
	local UMenuModMenuList NewItem;
	local UMenuModMenuItem TempItem;

	GetPlayerOwner().GetNextIntDesc("UMenu.UMenuModMenuItem", 0, NextModClass, NextModDesc);

	if(NextModClass == "")
		return False;

	ModItems = New class'UMenuModMenuList';
	ModItems.SetupSentinel();

	while( (NextModClass != "") && (NumModClasses < 50) )
	{
		NewItem = UMenuModMenuList(ModItems.Append(class'UMenuModMenuList'));
		NewItem.MenuItemClassName = NextModClass;
		if(NextModDesc != "")
		{
			i = InStr(NextModDesc, ",");
			if(i==-1)
				NewItem.MenuCaption = NextModDesc;
			else
			{
				NewItem.MenuCaption = Left(NextModDesc, i);
				NewItem.MenuHelp= Mid(NextModDesc, i+1);
			}
		}
		else
		{
			TempItem = New class<UMenuModMenuItem>(DynamicLoadObject(NextModClass, class'Class'));
			TempItem.Setup();
			NewItem.MenuCaption = TempItem.MenuCaption;
			NewItem.MenuHelp = TempItem.MenuHelp;		
		}

		NumModClasses++;
		GetPlayerOwner().GetNextIntDesc("UMenu.UMenuModMenuItem", NumModClasses, NextModClass, NextModDesc);
	}
	
	return True;
}

function NotifyQuitUnreal()
{
	local UWindowMenuBarItem I;

	for(I = UWindowMenuBarItem(Items.Next); I != None; I = UWindowMenuBarItem(I.Next))
		if(I.Menu != None)
			I.Menu.NotifyQuitUnreal();
}

function NotifyBeforeLevelChange()
{
	local UWindowMenuBarItem I;

	for(I = UWindowMenuBarItem(Items.Next); I != None; I = UWindowMenuBarItem(I.Next))
		if(I.Menu != None)
			I.Menu.NotifyBeforeLevelChange();
}

function NotifyAfterLevelChange()
{
	local UWindowMenuBarItem I;

	for(I = UWindowMenuBarItem(Items.Next); I != None; I = UWindowMenuBarItem(I.Next))
		if(I.Menu != None)
			I.Menu.NotifyAfterLevelChange();
}

function MenuCmd(int Menu, int Item)
{
	bShowMenu = False;	
	Super.MenuCmd(Menu, Item);
}

defaultproperties
{
	GameName="&Game"
	MultiplayerName="&Multiplayer"
	OptionsName="&Options"
	StatsName="&Stats"
	ToolName="&Tools"
	HelpName="&Help"
	VersionName="Version"
	GameHelp="Start a new game, load a game, or quit."
	MultiplayerHelp="Host or join a multiplayer game."
	OptionsHelp="Configure settings."
	StatsHelp="Manage your local and world stats."
	ToolHelp="Enable various system tools."
	HelpHelp="Enable or disable help."
	ModName="M&od"
	ModHelp="Configure user-created mods you have installed."
	ModMenuClass="UMenu.UMenuModMenu"
}

