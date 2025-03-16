//=============================================================================
// UBrowserTypesMenu.
//=============================================================================
class UBrowserTypesMenu expands UWindowRightClickMenu;

var UWindowPulldownMenuItem ShowAll, HideAll;

var() localized string ShowAllName;
var() localized string HideAllName;

var UBrowserTypesGrid	Grid;

function Created()
{
	Super.Created();
	
	ShowAll = AddMenuItem(ShowAllName, None);
	HideAll = AddMenuItem(HideAllName, None);
}

function ExecuteItem(UWindowPulldownMenuItem I) 
{
	Grid.FilterToggle(I == ShowAll);

	Super.ExecuteItem(I);
}

defaultproperties
{
	ShowAllName="Show all"
	HideAllName="Hide all"
}
