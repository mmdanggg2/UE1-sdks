class SpeedShell expands RelicShell;

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();

	if ( Level.bHighDetailMode )
		SetTimer(0.2, True);
}

simulated function Timer()
{
	Super.Timer();

	if ( !Level.bDropDetail && (Owner != None) && (Owner.Velocity != vect(0, 0, 0)) )
		Spawn(class'SpeedShadow', Owner, , Owner.Location, Owner.Rotation);

	if ( Level.bDropDetail )
		SetTimer(0.5, true);
	else
		SetTimer(0.2, True);
}


defaultproperties
{
}
