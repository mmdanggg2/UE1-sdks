class UMenuAdvancedDriverScrollClient expands UWindowScrollingDialogClient;


function Created()
{
	ClientClass = class'UMenuAdvancedDriverClientWindow';
	FixedAreaClass = None;
	Super.Created();
}

function GetDesiredDimensions(out float W, out float H)
{	
	Super(UWindowWindow).GetDesiredDimensions(W, H);
	H += 30;
}
