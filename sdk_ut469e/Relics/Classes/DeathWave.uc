class DeathWave expands Effects;

var bool bLaughingPain;
var float OldShockDistance, ShockSize;
var int ICount;

simulated function Tick( float DeltaTime )
{
	if ((Level.NetMode != NM_DedicatedServer) && !bLaughingPain)
	{
		ShockSize =  9 * (Default.LifeSpan - LifeSpan) + 2.5/(LifeSpan/Default.LifeSpan+0.05);
		ScaleGlow = Lifespan;
		DrawScale = 0.5 * ShockSize;
	}
}

simulated function Timer()
{

	local actor Victims;
	local float damageScale, dist, MoScale;
	local vector dir;

	if (bLaughingPain)
	{
		bLaughingPain = False;
		SetTimer(0.1, True);
		if ( Level.NetMode != NM_DedicatedServer )
			SpawnEffects();
	    Mesh = Mesh'Botpack.ShockWavem';
		Skin = texture'RelicOrange';
		return;
	}

	if ( Level.NetMode != NM_DedicatedServer )
	{
		if (ICount==4) spawn(class'WarExplosion2',,,Location);
		ICount++;

		if ( Level.NetMode == NM_Client )
		{
			foreach VisibleCollidingActors( class 'Actor', Victims, ShockSize*29, Location )
				if ( Victims.Role == ROLE_Authority )
				{
					dir = Victims.Location - Location;
					dist = FMax(1,VSize(dir));
					dir = dir/dist +vect(0,0,0.3); 
					if ( (dist> OldShockDistance) || (dir dot Victims.Velocity <= 0))
					{
						MoScale = FMax(0, 1100 - 1.1 * Dist);
						Victims.Velocity = Victims.Velocity + dir * (MoScale + 20);	
						Victims.TakeDamage
						(
							MoScale,
							Instigator, 
							Victims.Location - 0.5 * (Victims.CollisionHeight + Victims.CollisionRadius) * dir,
							(1000 * dir),
							'Eradicated'
						);
					}
				}	
			return;
		}
	}

	foreach VisibleCollidingActors( class 'Actor', Victims, ShockSize*29, Location )
	{
		dir = Victims.Location - Location;
		dist = FMax(1,VSize(dir));
		dir = dir/dist + vect(0,0,0.3); 
		if (dist> OldShockDistance || (dir dot Victims.Velocity < 0))
		{
			MoScale = FMax(0, 1100 - 1.1 * Dist);
			if ( Victims.bIsPawn )
				Pawn(Victims).AddVelocity(dir * (MoScale + 20));
			else
				Victims.Velocity = Victims.Velocity + dir * (MoScale + 20);	
			Victims.TakeDamage
			(
				MoScale,
				Instigator, 
				Victims.Location - 0.5 * (Victims.CollisionHeight + Victims.CollisionRadius) * dir,
				(1000 * dir),
				'Eradicated'
			);
		}
	}	
	OldShockDistance = ShockSize*29;	
}

simulated function PostBeginPlay()
{
	local Pawn P;

	if ( Role == ROLE_Authority ) 
	{
		for ( P=Level.PawnList; P!=None; P=P.NextPawn )
			if ( P.IsA('PlayerPawn') && (VSize(P.Location - Location) < 3000) )
				PlayerPawn(P).ShakeView(0.5, 600000.0/VSize(P.Location - Location), 10);

		if ( Instigator != None )
			MakeNoise(10.0);
	}

	DrawScale = 1.3;
	bLaughingPain = True;
	SetPhysics(PHYS_Rotating);
	SetTimer(1.5, True);
	Spawn(class'DeathFear', Self, , Location, Rotation);
}

simulated function SpawnEffects()
{
	local vector TraceLoc, TraceNorm;
	local WarExplosion W;
	local NuclearMark M;

	 PlaySound(Sound'Expl03',,6.0);
	 W = spawn(class'WarExplosion',,,Location);
	 W.RemoteRole = ROLE_None;
	 Trace(TraceLoc, TraceNorm, Location + vect(0,0,-400));
	 M = Spawn(class'NuclearMark', Self, , TraceLoc, rotator(TraceNorm));
	 M.RemoteRole = ROLE_None;
}

defaultproperties
{
     RemoteRole=ROLE_SimulatedProxy
     LifeSpan=2.000000
     DrawType=DT_Mesh
     Style=STY_Translucent
     Mesh=Mesh'RelicSkull'
     AmbientGlow=254
     bUnlit=True
	 bAlwaysRelevant=true
     bFixedRotationDir=True
     RotationRate=(Yaw=10000)
     DesiredRotation=(Yaw=30000)
}
