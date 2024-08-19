//=============================================================================
// TCowCarcass.
// DO NOT USE THESE AS DECORATIONS
//=============================================================================
class TCowCarcass extends TMale1Carcass;

function CreateReplacement()
{
	if ( Mesh == Default.Mesh )
		MasterReplacement = class'TMaleMasterChunk';

	Super.CreateReplacement();
}

defaultproperties
{
	MasterReplacement=class'TCowMasterChunk'
}