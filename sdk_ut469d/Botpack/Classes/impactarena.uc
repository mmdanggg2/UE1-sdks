//=============================================================================
// PulseArena.
// replaces all weapons and ammo with Pulseguns and pulsegun ammo
//=============================================================================

class ImpactArena expands Arena;

defaultproperties
{
	AmmoName=PAmmo
	AmmoString="Botpack.PAmmo"
	WeaponName=ImpactHammer
	WeaponString="Botpack.ImpactHammer"
	DefaultWeapon=class'Botpack.ImpactHammer'
}