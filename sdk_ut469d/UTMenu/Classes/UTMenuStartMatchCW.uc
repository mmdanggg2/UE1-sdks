class UTMenuStartMatchCW expands UMenuStartMatchClientWindow;

var UWindowCheckbox ChangeLevelsCheck;
var localized string ChangeLevelsText;
var localized string ChangeLevelsHelp;

// Category
var UWindowComboControl CategoryCombo;
var localized string CategoryText;
var localized string CategoryHelp;
var localized string GeneralText;
var config string LastCategory;

function Created()
{
	local int i, j, k, Selection, BestCategory, CategoryCount;
	local class<GameInfo> TempClass;
	local string TempGame, NextGame;
	local string TempGames[ArrayCount(Games)];
	local string NextEntry, NextCategory;
	local string Categories[ArrayCount(Games)];
	local bool bFoundSavedGameClass, bAlreadyHave;
	local PlayerPawn PlayerOwner;

	local int ControlWidth, ControlLeft, ControlRight;
	local int CenterWidth, CenterPos;

	Super(UMenuDialogClientWindow).Created();

	DesiredWidth = 270;
	DesiredHeight = 100;

	ControlWidth = WinWidth/2.5;
	ControlLeft = (WinWidth/2 - ControlWidth)/2;
	ControlRight = WinWidth/2 + ControlLeft;

	CenterWidth = (WinWidth/4)*3;
	CenterPos = (WinWidth - CenterWidth)/2;

	BotmatchParent = UMenuBotmatchClientWindow(GetParent(class'UMenuBotmatchClientWindow'));
	if (BotmatchParent == None)
		Log("Error: UMenuStartMatchClientWindow without UMenuBotmatchClientWindow parent.");

	// Category
	CategoryCombo = UWindowComboControl(CreateControl(class'UWindowComboControl', CenterPos, 20, CenterWidth, 1));
	CategoryCombo.SetButtons(True);
	CategoryCombo.SetText(CategoryText);
	CategoryCombo.SetHelpText(CategoryHelp);
	CategoryCombo.SetFont(F_Normal);
	CategoryCombo.SetEditable(False);
	CategoryCombo.AddItem(GeneralText);
	
	PlayerOwner = GetPlayerOwner();
	
	// Add all categories.
	// Compile a list of all gametypes.
	for (i = 0; i < ArrayCount(Games); i++)
	{
		PlayerOwner.GetNextIntDesc("TournamentGameInfo", i, NextGame, NextCategory);
		if (NextGame == "")
			break;
		
		if (NextCategory != "")
		{
			for (j = 0; j < CategoryCount; j++)
				if (Categories[j] ~= NextCategory)
					break;
			if (j == CategoryCount)
			{
				CategoryCombo.AddItem(NextCategory);
				Categories[CategoryCount++] = NextCategory;
				if (NextCategory ~= LastCategory)
				{
					BestCategory = CategoryCount;
					k = 0;
				}
			}
		}
		else
			j = -1;
		if (BestCategory == j + 1)
			TempGames[k++] = NextGame;
	}
	if (i == ArrayCount(Games)) {
		PlayerOwner.GetNextIntDesc("TournamentGameInfo", i, NextGame, NextCategory);
		if (NextGame != "")
			Log("More than" @ ArrayCount(Games) @ "gameinfos listed in int files");
	}
	CategoryCombo.SetSelectedIndex(BestCategory);

	// Game Type
	GameCombo = UWindowComboControl(CreateControl(class'UWindowComboControl', CenterPos, 45, CenterWidth, 1));
	GameCombo.SetButtons(True);
	GameCombo.SetText(GameText);
	GameCombo.SetHelpText(GameHelp);
	GameCombo.SetFont(F_Normal);
	GameCombo.SetEditable(False);

	// Fill the control.
	MaxGames = 0;
	for (i = 0; i < k; i++)
	{
		Games[MaxGames] = TempGames[i];
		if ( !bFoundSavedGameClass && (Games[MaxGames] ~= BotmatchParent.GameType) )
		{
			bFoundSavedGameClass = true;
			Selection = MaxGames;
		}
		TempClass = Class<GameInfo>(DynamicLoadObject(Games[MaxGames], class'Class'));
		if (TempClass != None)
		{
		    GameCombo.AddItem(TempClass.Default.GameName);
		    MaxGames++;
		}
	}

	GameCombo.SetSelectedIndex(Selection);	
	BotmatchParent.GameType = Games[Selection];
	BotmatchParent.GameClass = Class<GameInfo>(DynamicLoadObject(BotmatchParent.GameType, class'Class'));

	// Map
	MapCombo = UWindowComboControl(CreateControl(class'UWindowComboControl', CenterPos, 70, CenterWidth, 1));
	MapCombo.SetButtons(True);
	MapCombo.SetText(MapText);
	MapCombo.SetHelpText(MapHelp);
	MapCombo.SetFont(F_Normal);
	MapCombo.SetEditable(False);

	IterateMaps(BotmatchParent.Map);

	// Map List Button
	MapListButton = UWindowSmallButton(CreateControl(class'UWindowSmallButton', CenterPos, 95, 48, 16));
	MapListButton.SetText(MapListText);
	MapListButton.SetFont(F_Normal);
	MapListButton.SetHelpText(MapListHelp);

	// Mutator Button
	MutatorButton = UWindowSmallButton(CreateControl(class'UWindowSmallButton', CenterPos, 120, 48, 16));
	MutatorButton.SetText(MutatorText);
	MutatorButton.SetFont(F_Normal);
	MutatorButton.SetHelpText(MutatorHelp);

	ControlWidth = WinWidth/2.5;
	ControlLeft = (WinWidth/2 - ControlWidth)/2;
	ControlRight = WinWidth/2 + ControlLeft;

	CenterWidth = (WinWidth/4)*3;
	CenterPos = (WinWidth - CenterWidth)/2;

	ChangeLevelsCheck = UWindowCheckbox(CreateControl(class'UWindowCheckbox', CenterPos, 145, ControlWidth, 1));
	ChangeLevelsCheck.SetText(ChangeLevelsText);
	ChangeLevelsCheck.SetHelpText(ChangeLevelsHelp);
	ChangeLevelsCheck.SetFont(F_Normal);
	ChangeLevelsCheck.Align = TA_Right;

	SetChangeLevels();

	Initialized = true;
}

function BeforePaint(Canvas C, float X, float Y)
{
	local int ControlWidth, ControlLeft, ControlRight;
	local int CenterWidth, CenterPos;

	Super.BeforePaint(C, X, Y);

	ControlWidth = WinWidth/2.5;
	ControlLeft = (WinWidth/2 - ControlWidth)/2;
	ControlRight = WinWidth/2 + ControlLeft;

	CenterWidth = (WinWidth/4)*3;
	CenterPos = (WinWidth - CenterWidth)/2;

	CategoryCombo.SetSize(CenterWidth, 1);
	CategoryCombo.WinLeft = CenterPos;
	CategoryCombo.EditBoxWidth = 150;

	ChangeLevelsCheck.SetSize(ControlWidth, 1);		
	ChangeLevelsCheck.WinLeft = (WinWidth - ChangeLevelsCheck.WinWidth) / 2;
}

function CategoryChanged()
{
	local string CurCategory;
	local int i, k;
	local string NextGame, NextCategory;
	local string TempGames[ArrayCount(Games)];
	local class<GameInfo> TempClass;
	local PlayerPawn PlayerOwner;

	if (!Initialized)
		return;

	Initialized = false;

	CurCategory = CategoryCombo.GetValue();

	if (CurCategory == LastCategory)
	{
		Initialized = true;
		return;
	}
	LastCategory = CurCategory;
	
	PlayerOwner = GetPlayerOwner();

	GameCombo.Clear();
	
	if (CurCategory ~= GeneralText)
		CurCategory = "";
	
	for (i = 0; i < ArrayCount(Games); i++)
	{
		PlayerOwner.GetNextIntDesc("TournamentGameInfo", i, NextGame, NextCategory);
		if (NextGame == "")
			break;
		if (NextCategory ~= CurCategory)
			TempGames[k++] = NextGame;
	}
	if (i == ArrayCount(Games)) {
		PlayerOwner.GetNextIntDesc("TournamentGameInfo", i, NextGame, NextCategory);
		if (NextGame != "")
			Log("More than" @ ArrayCount(Games) @ "gameinfos listed in int files");
	}

	// Fill the control.
	MaxGames = 0;
	for (i = 0; i < k; i++)
	{
		Games[MaxGames] = TempGames[i];
		TempClass = Class<GameInfo>(DynamicLoadObject(Games[MaxGames], class'Class'));
		if (TempClass != None)
		{
			GameCombo.AddItem(TempClass.Default.GameName);
			MaxGames++;
		}
	}

	GameCombo.SetSelectedIndex(0);

	Initialized = true;

	GameChanged();

	SaveConfig();
}

function GameChanged()
{
	if (!Initialized)
		return;

	Super.GameChanged();
	SetChangeLevels();
}

function SetChangeLevels()
{
	local class<DeathMatchPlus> DMP;

	DMP = class<DeathMatchPlus>(BotmatchParent.GameClass);
	if(DMP == None)
	{
		ChangeLevelsCheck.HideWindow();
	}
	else
	{
		ChangeLevelsCheck.ShowWindow();
		ChangeLevelsCheck.bChecked = DMP.default.bChangeLevels;
	}
}

function Notify(UWindowDialogControl C, byte E)
{
	Super.Notify(C, E);

	switch(E)
	{
	case DE_Change:
		switch(C)
		{
		case CategoryCombo:
			CategoryChanged();
			break;
		case ChangeLevelsCheck:
			ChangeLevelsChanged();
			break;
		}
		break;
	}
}

function ChangeLevelsChanged()
{
	local class<DeathMatchPlus> DMP;

	DMP = class<DeathMatchPlus>(BotmatchParent.GameClass);
	if(DMP != None)
	{
		DMP.default.bChangeLevels = ChangeLevelsCheck.bChecked;
		DMP.static.StaticSaveConfig();
	}
}

defaultproperties
{
	CategoryText="Category:"
	CategoryHelp="Select a category of gametype!"
	GeneralText="Unreal Tournament"
	ChangeLevelsText="Auto Change Levels"
	ChangeLevelsHelp="If this setting is checked, the server will change levels according to the map list for this game type."
}
