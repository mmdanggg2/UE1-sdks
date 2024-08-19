class UBrowserEditFavoriteCW expands UWindowDialogClientWindow;

var UWindowEditControl	DescriptionEdit;
var localized string	DescriptionText;

var UWindowCheckbox		UpdateDescriptionCheck;
var localized string	UpdateDescriptionText;

var UWindowEditControl	IPEdit;
var localized string	IPText;

var UWindowEditControl	GamePortEdit;
var localized string	GamePortText;

var UWindowEditControl	QueryPortEdit;
var localized string	QueryPortText;

function Created()
{
	local float ControlOffset, CenterPos, CenterWidth;

	Super.Created();
	
	DescriptionEdit = UWindowEditControl(CreateControl(class'UWindowEditControl', 10, 10, 220, 1));
	DescriptionEdit.SetText(DescriptionText);
	DescriptionEdit.SetFont(F_Normal);
	DescriptionEdit.SetNumericOnly(False);
	DescriptionEdit.SetMaxLength(300);
	DescriptionEdit.EditBoxWidth = 100;

	UpdateDescriptionCheck = UWindowCheckbox(CreateControl(class'UWindowCheckbox', 10, 30, 136, 1));
	UpdateDescriptionCheck.SetText(UpdateDescriptionText);
	UpdateDescriptionCheck.SetFont(F_Normal);

	IPEdit = UWindowEditControl(CreateControl(class'UBrowserEditFavoriteControl', 10, 50, 220, 1));
	IPEdit.SetText(IPText);
	IPEdit.SetFont(F_Normal);
	IPEdit.SetNumericOnly(False);
	IPEdit.SetMaxLength(100);
	IPEdit.EditBoxWidth = 100;
	UBrowserEditFavoriteControl(IPEdit).SetCW(self);
	
	GamePortEdit = UWindowEditControl(CreateControl(class'UWindowEditControl', 10, 70, 160, 1));
	GamePortEdit.SetText(GamePortText);
	GamePortEdit.SetFont(F_Normal);
	GamePortEdit.SetNumericOnly(True);
	GamePortEdit.SetMaxLength(5);
	GamePortEdit.EditBoxWidth = 40;

	QueryPortEdit = UWindowEditControl(CreateControl(class'UWindowEditControl', 10, 90, 160, 1));
	QueryPortEdit.SetText(QueryPortText);
	QueryPortEdit.SetFont(F_Normal);
	QueryPortEdit.SetNumericOnly(True);
	QueryPortEdit.SetMaxLength(5);
	QueryPortEdit.EditBoxWidth = 40;

	DescriptionEdit.BringToFront();
	LoadCurrentValues();
}

function LoadCurrentValues()
{
	local UBrowserServerList L;
	local int GamePort;

	L = UBrowserRightClickMenu(ParentWindow.OwnerWindow).List;

	DescriptionEdit.SetValue(L.HostName);
	UpdateDescriptionCheck.bChecked = !L.bKeepDescription;
	IPEdit.SetValue(L.IP);
	GamePort = L.GamePort;
	if (GamePort == 0 && L.QueryPort > 0)
		GamePort = L.QueryPort - 1;
	GamePortEdit.SetValue(string(GamePort));
	QueryPortEdit.SetValue(string(L.QueryPort));
}

function BeforePaint(Canvas C, float X, float Y)
{
	Super.BeforePaint(C, X, Y);

	DescriptionEdit.WinWidth = WinWidth - 20;
	DescriptionEdit.EditBoxWidth = WinWidth - 140;

	IPEdit.WinWidth = WinWidth - 20;
	IPEdit.EditBoxWidth = WinWidth - 140;	
}

function Notify(UWindowDialogControl C, byte E)
{
	Super.Notify(C, E);

	if((C == UBrowserEditFavoriteWindow(ParentWindow).OKButton && E == DE_Click))
		OKPressed();
	if (C == GamePortEdit && E == DE_Change)
		QueryPortEdit.SetValue("" $ (Int(GamePortEdit.GetValue()) + 1));
}

function OKPressed()
{
	local UBrowserServerList L;

	L = UBrowserRightClickMenu(ParentWindow.OwnerWindow).List;

	L.HostName = DescriptionEdit.GetValue();
	L.bKeepDescription = !UpdateDescriptionCheck.bChecked;
	L.IP = IPEdit.GetValue();
	L.GamePort = Int(GamePortEdit.GetValue());
	L.QueryPort = Int(QueryPortEdit.GetValue());
	
	UBrowserFavoritesFact(UBrowserFavoriteServers(UBrowserRightClickMenu(ParentWindow.OwnerWindow).Grid.GetParent(class'UBrowserFavoriteServers')).Factories[0]).SaveFavorites();
	L.PingServer(False, True, True);

	ParentWindow.Close();
}

function NotifyEditPaste()
{
    local int i, Port;
	local string Host;
	
    Host = IPEdit.GetValue();
	i = InStr(Host, "://");
	if (i != -1) // cut protocol
		Host = Mid(Host, i + 3);
	i = InStr(Host, "?");
	if (i != -1) // cut query part
		Host = Left(Host, i);
	i = InStr(Host, "]");
	if (i != -1) // IPv6 address
	{
		i = InStr(Host, "]:");
		if (i != -1)
			i++;
	}
	else
		i = InStr(Host, ":");
	if (i != -1) // cut port
	{
		Port = Int(Mid(Host, i + 1));
		if (Port != 0)
		{
			GamePortEdit.SetValue("" $ Port);
			QueryPortEdit.SetValue("" $ (Port + 1));
		}
		Host = Left(Host, i);
	}
	while (Len(Host) > 0 && Asc(Mid(Host, 0, 1)) <= 32) // trim left
		Host = Mid(Host, 1);
	while (Len(Host) > 0 && Asc(Mid(Host, Len(Host) - 1, 1)) <= 32) // trim right
		Host = Left(Host, Len(Host) - 1);
	if (Host != IPEdit.GetValue())
	{
		IPEdit.EditBox.Offset = 0;
		IPEdit.SetValue(Host);
	}
}

defaultproperties
{
	DescriptionText="Description"
	UpdateDescriptionText="Auto-Update Description"
	IPText="Server Address"
	GamePortText="Server Port Number"
	QueryPortText="Query Port Number"
}
