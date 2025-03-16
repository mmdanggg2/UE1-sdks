//
// Stukka Fix:
//
// The Botpack.Stukka class did not have a drawtype set in its defaultproperties.
// Fixing this bug in the base class is not safe as it might break replication
// of subclasses. See, e.g., OUWarHeadAmmo.
//
// Relevant Bug Report:
// https://github.com/OldUnreal/UnrealTournamentPatches/issues/528
//
// Internal Reports/PRs:
// SDK/PR/267
//
// Credits:
// Buggie
//
class OUStukka extends Stukka;

defaultproperties
{
    DrawType=DT_Mesh
}