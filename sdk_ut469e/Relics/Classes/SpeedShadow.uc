class SpeedShadow expands Effects;

simulated function PostBeginPlay()
{
	local int i;

	Mesh = Owner.Mesh;
	AnimFrame = Owner.AnimFrame;
	AnimSequence = Owner.AnimSequence;
	AnimRate = Owner.AnimRate;
	TweenRate = Owner.TweenRate;
	AnimMinRate = Owner.AnimMinRate;
	AnimLast = Owner.AnimLast;
	bAnimLoop = Owner.bAnimLoop;
	bAnimFinished = Owner.bAnimFinished;

	if ( Owner.IsA('Pawn') && Pawn(Owner).bIsMultiSkinned )
	{
		for (i=0; i<8; i++)
			MultiSkins[i] = Owner.MultiSkins[i];
	}
	else
		Skin = Owner.Skin;
}

simulated function Tick(float Delta)
{
	ScaleGlow -= 3*Delta;
	if (ScaleGlow <= 0)
		Destroy();
}

defaultproperties
{
	 Mesh=mesh'Botpack.Soldier'
	 bAnimLoop=true
	 AnimRate=17
	 Style=STY_Translucent
	 DrawType=DT_Mesh
	 bUnlit=true
	 bOwnerNoSee=True
	 RemoteRole=ROLE_None
	 LODBias=+0.1
	 bHighDetail=true
}