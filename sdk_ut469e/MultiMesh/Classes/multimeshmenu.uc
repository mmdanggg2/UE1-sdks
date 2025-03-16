class MultiMeshMenu expands UMenuModMenuItem
	config;

var config bool bForceDefaultMesh;

function Execute()
{
	MenuItem.bChecked = !MenuItem.bChecked;
	bForceDefaultMesh = MenuItem.bChecked;
	SaveConfig();
}


defaultproperties
{
	MenuCaption="&Force Default Models" 
	MenuHelp="Use the default (Male Soldier) model instead of any custom models"
}

