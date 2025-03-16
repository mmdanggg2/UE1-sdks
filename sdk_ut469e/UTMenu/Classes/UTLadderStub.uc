class UTLadderStub expands NotifyWindow
	abstract;

var globalconfig string StubClassName;
var class<UTLadderStub> StubClass;

static function class<UTLadderStub> GetStubClass()
{
	if(default.StubClass != None)
		return default.StubClass;

	default.StubClass = class<UTLadderStub>(DynamicLoadObject(default.StubClassName, class'Class'));
	return default.StubClass;
}

static function bool IsDemo()
{
	return (class'GameInfo'.Default.DemoBuild == 1);
}

static function bool DemoHasTuts()
{
	return (class'GameInfo'.Default.DemoHasTuts == 1);
}

static function SetupWinParams(UWindowWindow Win, UWindowRootWindow RootWin, out int W, out int H)
{
	local float KW, KH;

	KW = RootWin.WinWidth/1024;
	KH = RootWin.WinHeight/768;

	if (KW > KH) KW = KH;
	if (KW > 1) KW = 1;

	Win.WinWidth = KW*1024;
	Win.WinHeight = KW*768;
	Win.WinLeft = (RootWin.WinWidth - Win.WinWidth) / 2;
	Win.WinTop = (RootWin.WinHeight - Win.WinHeight) / 2;

	W = Win.WinWidth / 4;
	H = W;

	if(W > 256 || H > 256)
	{
		W = 256;
		H = 256;
	}
}

static function int GetAdjustedSize(UWindowRootWindow Root)
{
	local int S;

	S = Root.WinWidth;
	if (Root.WinWidth*3 > Root.WinHeight*4) S = Root.WinHeight*4/3;
	S *= Root.GUIScale;

	return S;
}

static function font GetHugeFont(UWindowRootWindow Root)
{
	local int S;

	S = GetAdjustedSize(Root);

	if (S < 512)
		return Font(DynamicLoadObject("LadderFonts.UTLadder12", class'Font'));
	else if (S < 640)
		return Font(DynamicLoadObject("LadderFonts.UTLadder16", class'Font'));
	else if (S < 800)
		return Font(DynamicLoadObject("LadderFonts.UTLadder20", class'Font'));
	else if (S < 1024)
		return Font(DynamicLoadObject("LadderFonts.UTLadder22", class'Font'));
	else
		return Font(DynamicLoadObject("LadderFonts.UTLadder30", class'Font'));
}

static function font GetBigFont(UWindowRootWindow Root)
{
	local int S;

	S = GetAdjustedSize(Root);

	if (S < 640)
		return Font(DynamicLoadObject("LadderFonts.UTLadder10", class'Font'));
	else if (S < 800)
		return Font(DynamicLoadObject("LadderFonts.UTLadder12", class'Font'));
	else if (S < 1024)
		return Font(DynamicLoadObject("LadderFonts.UTLadder16", class'Font'));
	else
		return Font(DynamicLoadObject("LadderFonts.UTLadder18", class'Font'));
}

static function font GetSmallFont(UWindowRootWindow Root)
{
	local int S;

	S = GetAdjustedSize(Root);

	if (S < 800)
		return Font(DynamicLoadObject("LadderFonts.UTLadder10", class'Font'));
	else if (S < 1024)
		return Font(DynamicLoadObject("LadderFonts.UTLadder14", class'Font'));
	else
		return Font(DynamicLoadObject("LadderFonts.UTLadder16", class'Font'));
}

static function font GetSmallestFont(UWindowRootWindow Root)
{
	local int S;

	S = GetAdjustedSize(Root);

	if (S < 800)
		return Font(DynamicLoadObject("LadderFonts.UTLadder10", class'Font'));
	else if (S < 1024)
		return Font(DynamicLoadObject("LadderFonts.UTLadder12", class'Font'));
	else
		return Font(DynamicLoadObject("LadderFonts.UTLadder14", class'Font'));
}

static function font GetAReallySmallFont(UWindowRootWindow Root)
{
	local int S;

	S = GetAdjustedSize(Root);

	if (S < 1024)
		return Font(DynamicLoadObject("LadderFonts.UTLadder8", class'Font'));
	else
		return Font(DynamicLoadObject("LadderFonts.UTLadder10", class'Font'));
}

static function font GetACompletelyUnreadableFont(UWindowRootWindow Root)
{
	local int S;

	S = GetAdjustedSize(Root);

	if (S < 1024)
		return Font(DynamicLoadObject("LadderFonts.UTLadder8", class'Font'));
	else
		return Font(DynamicLoadObject("LadderFonts.UTLadder8", class'Font'));
}

function EvaluateMatch(optional bool bTrophyVictory);

defaultproperties
{
	StubClassName="UTMenu.UTLadderStub"
}
