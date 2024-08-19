//
// WarHeadAmmo Fix:
//
// Botpack.WarHeadAmmo originally had 'Botpack.Missile' as its PickupMesh, while
// it should have been its PickupViewMesh. OldUnreal fixed this in the Unreal
// Tournament v469a Patch but reverted the change in the v469c Patch because the
// defaultproperties of instances of fixed WarHeadAmmo subclasses were not getting
// replicated on v469a-v469b servers.
//
// Relevant Bug Report:
// https://github.com/OldUnreal/UnrealTournamentPatches/issues/454
//
// Credits:
// Chamberly
//
class OUWarHeadAmmo extends WarHeadAmmo;

defaultproperties
{
	PickupViewMesh=LodMesh'Botpack.missile'
}