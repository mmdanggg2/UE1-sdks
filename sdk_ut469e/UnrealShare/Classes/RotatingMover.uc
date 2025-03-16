//=============================================================================
// RotatingMover.
//=============================================================================
class RotatingMover extends Mover;

var() rotator RotateRate;

// Added in v469: internals to ensure proper networking
var bool bEnabled;
var float AccumulatedPitch;
var float AccumulatedYaw;
var float AccumulatedRoll;

replication
{
	reliable if ( Role==ROLE_Authority )
		bEnabled;
}

function BeginPlay();

simulated function Tick( float DeltaTime )
{
	local rotator NewRotation;

	if ( bEnabled )
	{
		// v469 TODO:
		// - Periodically send rotation to clients (without hogging bandwidth)
		// - Stop rotation code while mover is interpolating
		// - Normalize rotation (to same range of current keyframe)
		AccumulatedPitch += float(RotateRate.Pitch) * DeltaTime;
		AccumulatedYaw   += float(RotateRate.Yaw)   * DeltaTime;
		AccumulatedRoll  += float(RotateRate.Roll)  * DeltaTime;
		NewRotation.Pitch = Rotation.Pitch + int(AccumulatedPitch);
		NewRotation.Yaw   = Rotation.Yaw   + int(AccumulatedYaw);
		NewRotation.Roll  = Rotation.Roll  + int(AccumulatedRoll);
		SetRotation( NewRotation );
		AccumulatedPitch -= int(AccumulatedPitch);
		AccumulatedYaw   -= int(AccumulatedYaw);
		AccumulatedRoll  -= int(AccumulatedRoll);
	}
}

function Trigger( Actor other, Pawn EventInstigator )
{
	bEnabled = true;
}

function UnTrigger( Actor other, Pawn EventInstigator )
{
	bEnabled = false;
}

defaultproperties
{
    bEnabled=True
}
