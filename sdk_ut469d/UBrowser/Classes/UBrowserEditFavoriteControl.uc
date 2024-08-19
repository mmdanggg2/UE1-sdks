class UBrowserEditFavoriteControl extends UWindowEditControl;

var UBrowserEditFavoriteCW CW;

function Created()
{
	if(!bNoKeyboard)
		SetAcceptsFocus();
	
	EditBox = UBrowserEditFavoriteBox(CreateWindow(class'UBrowserEditFavoriteBox', 0, 0, WinWidth, WinHeight)); 
	EditBox.NotifyOwner = Self;
	EditBox.bSelectOnFocus = True;

	EditBoxWidth = WinWidth / 2;

	SetEditTextColor(LookAndFeel.EditBoxTextColor);
}

function SetCW(UBrowserEditFavoriteCW Window)
{
    CW = Window;
	if (EditBox != None)
	    UBrowserEditFavoriteBox(EditBox).CW = CW;
}
