//=============================================================================
// Event.
//=============================================================================
class Triggers extends Actor
	native;

defaultproperties
{
     bHidden=True
     CollisionRadius=+00040.000000
     CollisionHeight=+00040.000000
     bCollideActors=True
	 // OldUnreal: sounds played by a trigger are generally more important than sounds played by a player
	 TransientSoundPriority=32 
	 SoundPriority=32          
}
