//
// Flame Fix:
//
// The UnrealShare.Flame class did not have a mesh set in its defaultproperties.
// Fixing this bug in the base class is not safe as it might break replication
// of subclasses. See, e.g., OUWarHeadAmmo.
//
// Relevant Bug Report:
// https://github.com/OldUnreal/UnrealTournamentPatches/issues/454
//
// Credits:
// Buggie
//
class OUFlame extends Flame;

defaultproperties
{
    Mesh=NaliCow
}