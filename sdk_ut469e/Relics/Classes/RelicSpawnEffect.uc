class RelicSpawnEffect expands Effects;

simulated function PostBeginPlay()
{
	local int i;

	Super.PostBeginPlay();
	if ( Level.bDropDetail )
		LightType = LT_None;
	if ( Owner != None )
	{
		mesh = Owner.Mesh;
		if ( !Owner.bIsPawn )
			skin = Owner.Skin;
		else
			skin = texture'RelicBlue';

		Style = STY_Translucent;
		ScaleGlow = 1.0;

		if (Owner.IsA('Inventory'))
			DrawScale = Inventory(Owner).PickupViewScale;
		else
			DrawScale = Owner.DrawScale;
	}
	Playsound(sound'RespawnSound2');
}

simulated function Tick(float Delta)
{
	ScaleGlow -= Delta;
	if (ScaleGlow <= 0)
		Destroy();
}

defaultproperties
{
	RemoteRole=ROLE_SimulatedProxy
	DrawType=DT_Mesh
    bUnlit=True
	bNetOptional=true
	bNetTemporary=true
    LightType=LT_Steady
    LightEffect=LE_NonIncidence
    LightBrightness=210
    LightHue=30
    LightSaturation=224
    LightRadius=5
}