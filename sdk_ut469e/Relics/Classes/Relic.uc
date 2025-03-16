class Relic expands Mutator
	abstract;

#exec TEXTURE IMPORT NAME=RelicRed    FILE=Textures\RelicRed.PCX    GROUP=Skins
#exec TEXTURE IMPORT NAME=RelicGreen  FILE=Textures\RelicGreen.PCX  GROUP=Skins
#exec TEXTURE IMPORT NAME=RelicOrange FILE=Textures\RelicOrange.PCX GROUP=Skins
#exec TEXTURE IMPORT NAME=RelicPurple FILE=Textures\RelicPurple.PCX GROUP=Skins
#exec TEXTURE IMPORT NAME=RelicBlue   FILE=Textures\RelicBlue.PCX   GROUP=Skins

var class<RelicInventory> RelicClass;
var int NumPoints;
var bool Initialized;
var RelicInventory SpawnedRelic;
var int NavPoint;

function PostBeginPlay()
{
	local NavigationPoint NP;

	if (Initialized)
		return;
	Initialized = True;

	// Calculate number of navigation points.
	for (NP = Level.NavigationPointList; NP != None; NP = NP.NextNavigationPoint)
	{
		if (NP.IsA('PathNode'))
			NumPoints++;
	}

	SpawnRelic(0);
	SetTimer(5.0, True);
}

function SpawnRelic(int RecurseCount)
{
	local int PointCount;
	local NavigationPoint NP;
	local RelicInventory Touching;

	NavPoint = Rand(NumPoints);
	for (NP = Level.NavigationPointList; NP != None; NP = NP.NextNavigationPoint)
	{
		if ( NP.IsA('PathNode') )
		{
			if (PointCount == NavPoint)
			{
				// check that there are no other relics here
				if ( RecurseCount < 3 )
					ForEach VisibleCollidingActors(class'RelicInventory', Touching, 40, NP.Location)
					{
						SpawnRelic(RecurseCount + 1);	
						return;
					}

				// Spawn it here.
				SpawnedRelic = Spawn(RelicClass, , , NP.Location);
				SpawnedRelic.MyRelic = Self;
				return;
			}
			PointCount++;
		}
	}
}

function Mutate(string MutateString, PlayerPawn Sender)
{
	local Inventory S;

	if (MutateString ~= "TossRelic")
	{
		S = Sender.FindInventoryType(RelicClass);
		if (S != None)
		{
			RelicInventory(S).DropInventory();
			Sender.DeleteInventory(S);
		}
	}

	if ( NextMutator != None )
		NextMutator.Mutate(MutateString, Sender);
}

function Timer()
{

	if ( (SpawnedRelic != None) && (SpawnedRelic.Owner == None) )
	{
		SpawnedRelic.IdleTime += 5;
		if ( SpawnedRelic.IdleTime >= 30 )
		{
			SpawnedRelic.IdleTime = 0;
			Spawn(class'RelicSpawnEffect', SpawnedRelic,, SpawnedRelic.Location, SpawnedRelic.Rotation);
			SpawnedRelic.Destroy();
		}
	}
}

defaultproperties
{
}