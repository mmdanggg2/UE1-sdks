class RatedTeamInfo7 expands RatedTeamInfo;

// Red Claw
#exec TEXTURE IMPORT NAME=TSkaarj1 FILE=textures\teamsymbols\skaarjteam_a.pcx GROUP="TeamSymbols" MIPS=OFF

static function class<TournamentPlayer> GetMaleClass()
{
	return class<TournamentPlayer>(DynamicLoadObject("MultiMesh.TSkaarj",class'Class'));
}


defaultproperties
{
	TeamName="Red Claw"
	TeamBio="The N.E.G. has long recognized the superiority of the Skaarj warrior as a military fighting machine, as was made clear in the brutal Human-Skaarj wars.  The Skaarj Hybrid is the result of secret military genetic research using both human and Skaarj DNA performed after the capture of a Skaarj scout ship. If proven in Tournament battle, the Hybrids shall become a leading force in ground based ops."
	TeamSymbol=texture'TSkaarj1'

	MaleClass=class'MultiMesh.TSkaarjBot'
	MaleSkin="TSkmSkins.Warr"
	FemaleClass=class'MultiMesh.TSkaarjBot'
	FemaleSkin="TSkmSkins.Warr"

	BotNames(0)="Dominator"
	BotClasses(0)="MultiMesh.TSkaarjBot"
	BotClassifications(0)="Classified: L9"
	BotSkins(0)="TskmSkins.Warr"
	BotFaces(0)="TSkMSkins.Dominator"
	BotBio(0)="No profile available. Level 9 security clearance required."

	BotNames(1)="Berserker"
	BotClasses(1)="MultiMesh.TSkaarjBot"
	BotClassifications(1)="Classified: L9"
	BotSkins(1)="TskmSkins.Warr"
	BotFaces(1)="TSkMSkins.Berserker"
	BotBio(1)="No profile available. Level 9 security clearance required."

	BotNames(2)="Guardian"
	BotClasses(2)="MultiMesh.TSkaarjBot"
	BotClassifications(2)="Classified: L9"
	BotSkins(2)="TskmSkins.Warr"
	BotFaces(2)="TSkMSkins.Guardian"
	BotBio(2)="No profile available. Level 9 security clearance required."

	BotNames(3)="Devastator"
	BotClasses(3)="MultiMesh.TSkaarjBot"
	BotClassifications(3)="Classified: L9"
	BotSkins(3)="TskmSkins.Warr"
	BotFaces(3)="TSkMSkins.Dominator"
	BotBio(3)="No profile available. Level 9 security clearance required."

	BotNames(4)="Pestilence"
	BotClasses(4)="MultiMesh.TSkaarjBot"
	BotClassifications(4)="Classified: L9"
	BotSkins(4)="TskmSkins.Warr"
	BotFaces(4)="TSkMSkins.Berserker"
	BotBio(4)="No profile available. Level 9 security clearance required."

	BotNames(5)="Plague"
	BotClasses(5)="MultiMesh.TSkaarjBot"
	BotClassifications(5)="Classified: L9"
	BotSkins(5)="TskmSkins.Warr"
	BotFaces(5)="TSkMSkins.Guardian"
	BotBio(5)="No profile available. Level 9 security clearance required."
}
