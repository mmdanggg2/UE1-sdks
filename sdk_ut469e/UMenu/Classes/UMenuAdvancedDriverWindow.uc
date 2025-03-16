class UMenuAdvancedDriverWindow expands UMenuOptionsWindow;

var UWindowSmallCloseButton CloseButton;

function Created() 
{
	bStatusBar = False;
	bSizable = True;

	Super.Created();

	CloseButton = UWindowSmallCloseButton(CreateWindow(class'UWindowSmallCloseButton', WinWidth-56, WinHeight-24, 48, 16));

	SetSizePos();
}

function Resized()
{
	Super.Resized();
	ClientArea.SetSize(ClientArea.WinWidth, ClientArea.WinHeight-24);
	if ( CloseButton != None )
	{
		CloseButton.WinLeft = ClientArea.WinLeft+ClientArea.WinWidth-52;
		CloseButton.WinTop = ClientArea.WinTop+ClientArea.WinHeight+4;
	}
}

function Paint(Canvas C, float X, float Y)
{
	local Texture T;

	T = GetLookAndFeelTexture();
	DrawUpBevel( C, ClientArea.WinLeft, ClientArea.WinTop + ClientArea.WinHeight, ClientArea.WinWidth, 24, T);

	Super.Paint(C, X, Y);
}

defaultproperties
{
    ClientClass=class'UMenuAdvancedDriverScrollClient'
}
