//=============================================================================
// TCowMasterChunk
//=============================================================================
class TCowMasterChunk extends UTMasterCreatureChunk;

simulated function ClientExtraChunks()
{
	local carcass carc;
	local UT_bloodburst b;
	local PlayerPawn P;

	If ( Level.NetMode == NM_DedicatedServer )
		return;
	if ( class'GameInfo'.Default.bLowGore )
	{
		Destroy();
		return;
	}

	b = Spawn(class 'UT_BloodBurst');
	if ( bGreenBlood )
		b.GreenBlood();
	b.RemoteRole = ROLE_None;

	if ( Level.bHighDetailMode && !Level.bDropDetail )
	{
		if ( FRand() < 0.3 )
		{
			carc = Spawn(class 'UTLiver');
			if (carc != None)
				carc.Initfor(self);
		}
		else if ( FRand() < 0.5 )
		{
			carc = Spawn(class 'UTStomach');
			if (carc != None)
				carc.Initfor(self);
		}
		else
		{
			carc = Spawn(class 'UTHeart');
			if (carc != None)
				carc.Initfor(self);
		}
	}
	carc = Spawn(class 'UTPlayerChunks');
	if (carc != None)
		carc.Initfor(self);
	carc = Spawn(class 'UTPlayerChunks');
	if (carc != None)
		carc.Initfor(self);
	carc = Spawn(class 'UT_MaleTorso');
	if (carc != None)
		carc.Initfor(self);

	carc = Spawn(class 'UT_Thigh');
	if (carc != None)
		carc.Initfor(self);
}

defaultproperties
{
}