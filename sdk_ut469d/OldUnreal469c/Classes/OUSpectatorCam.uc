//
// SpectatorCam Fix:
//
// The Botpack.SpectatorCam class did not have a mesh set in its defaultproperties.
// Fixing this bug in the base class is not safe as it might break replication
// of subclasses. See, e.g., OUWarHeadAmmo.
//
// Relevant Bug Report:
// https://github.com/OldUnreal/UnrealTournamentPatches/issues/454
//
// Internal Reports/PRs:
// SDK/PR/276
//
// Credits:
// Buggie
//
class OUSpectatorCam extends SpectatorCam;

defaultproperties
{
    Mesh=Mesh'UnrealShare.IPanel'
}