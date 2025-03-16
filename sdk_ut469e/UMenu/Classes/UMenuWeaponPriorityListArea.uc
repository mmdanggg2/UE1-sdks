class UMenuWeaponPriorityListArea extends UMenuDialogClientWindow;

var UWindowWindow PriorityList;
var class<UMenuWeaponPriorityListBox> PriorityListClass;
var UWindowControlFrame Frame;

var UWindowCheckbox AutoSwitchCheck;
var localized string AutoSwitchText;
var localized string AutoSwitchHelp;

function Created()
{
	Super.Created();

	DesiredHeight = 200;
	DesiredWidth  = 140;

	PriorityList = CreateControl(PriorityListClass, 10, 10, 100, 200);
	Frame = UWindowControlFrame(CreateWindow(class'UWindowControlFrame', 10, 10, 100, 200));
	Frame.SetFrame(PriorityList);

	AutoSwitchCheck = UWindowCheckbox(CreateControl(class'UWindowCheckbox', 10, 120, 100, 1));
	AutoSwitchCheck.bChecked = !GetPlayerOwner().bNeverAutoSwitch;
	AutoSwitchCheck.SetText(AutoSwitchText);
	AutoSwitchCheck.SetHelpText(AutoSwitchHelp);
	AutoSwitchCheck.SetFont(F_Normal);
	AutoSwitchCheck.Align = TA_Left;
}

function BeforePaint(Canvas C, float MouseX, float MouseY)
{
	Super.BeforePaint(C, MouseX, MouseY);	

	Frame.WinLeft = 10;
	Frame.SetSize(WinWidth - 2*Frame.WinLeft, WinHeight-50);	
	AutoSwitchCheck.SetSize(Frame.WinWidth, 1);
	AutoSwitchCheck.WinLeft = Frame.WinLeft;
	AutoSwitchCheck.WinTop = WinHeight - 30;
}

function Notify(UWindowDialogControl C, byte E)
{
	switch(E)
	{
	case DE_Change:
		switch(C)
		{
		case AutoSwitchCheck:
			AutoSwitchChange();
			break;
		}
	}
	Super.Notify(C, E);
}

function AutoSwitchChange()
{
	GetPlayerOwner().NeverSwitchOnPickup(!AutoSwitchCheck.bChecked);
}

function SaveConfigs()
{
	Super.SaveConfigs();
	GetPlayerOwner().SaveConfig();
}

defaultproperties
{
	PriorityListClass=class'UMenuWeaponPriorityListBox';
	AutoSwitchText="Auto-Switch Weapons"
	AutoSwitchHelp="Automatically switch to a higher priority weapon when you pick it up.  If unchecked, you must switch to the new weapon manually.";
}
