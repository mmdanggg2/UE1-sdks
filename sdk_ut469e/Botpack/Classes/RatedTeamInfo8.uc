class RatedTeamInfo8 expands RatedTeamInfo;

// Iron Skull
#exec TEXTURE IMPORT NAME=TSkaarj2 FILE=textures\teamsymbols\skaarjteam_b.pcx GROUP="TeamSymbols" MIPS=OFF

static function class<TournamentPlayer> GetMaleClass()
{
	return class<TournamentPlayer>(DynamicLoadObject("MultiMesh.TSkaarj",class'Class'));
}


defaultproperties
{
	TeamName="Iron Skull"
	TeamBio="The N.E.G. has long recognized the superiority of the Skaarj warrior as a military fighting machine, as was made clear in the brutal Human-Skaarj wars.  The Skaarj Hybrid is the result of secret military genetic research using both human and Skaarj DNA performed after the capture of a Skaarj scout ship. If proven in Tournament battle, the Hybrids shall become a leading force in ground based ops."
	TeamSymbol=texture'TSkaarj2'

	MaleClass=class'MultiMesh.TSkaarjBot'
	MaleSkin="TSkmSkins.PitF"
	FemaleClass=class'MultiMesh.TSkaarjBot'
	FemaleSkin="TSkmSkins.PitF"

	BotNames(0)="Reaper"
	BotClasses(0)="MultiMesh.TSkaarjBot"
	BotClassifications(0)="Classified: L9"
	BotSkins(0)="TSkmSkins.MekS"
	BotFaces(0)="TSkMSkins.Disconnect"
	BotBio(0)="No profile available. Level 9 security clearance required."

	BotNames(1)="Baetal"
	BotClasses(1)="MultiMesh.TSkaarjBot"
	BotClassifications(1)="Classified: L9"
	BotSkins(1)="TSkmSkins.PitF"
	BotFaces(1)="TSkMSkins.Baetal"
	BotBio(1)="No profile available. Level 9 security clearance required."

	BotNames(2)="Pharoh"
	BotClasses(2)="MultiMesh.TSkaarjBot"
	BotClassifications(2)="Classified: L9"
	BotSkins(2)="TSkmSkins.PitF"
	BotFaces(2)="TSkMSkins.Pharoh"
	BotBio(2)="No profile available. Level 9 security clearance required."

	BotNames(3)="Skrilax"
	BotClasses(3)="MultiMesh.TSkaarjBot"
	BotClassifications(3)="Classified: L9"
	BotSkins(3)="TSkmSkins.PitF"
	BotFaces(3)="TSkMSkins.Skrilax"
	BotBio(3)="No profile available. Level 9 security clearance required."

	BotNames(4)="Anthrax"
	BotClasses(4)="MultiMesh.TSkaarjBot"
	BotClassifications(4)="Classified: L9"
	BotSkins(4)="TSkmSkins.PitF"
	BotFaces(4)="TSkMSkins.Baetal"
	BotBio(4)="No profile available. Level 9 security clearance required."

	BotNames(5)="Entropy"
	BotClasses(5)="MultiMesh.TSkaarjBot"
	BotClassifications(5)="Classified: L9"
	BotSkins(5)="TSkmSkins.PitF"
	BotFaces(5)="TSkMSkins.Pharoh"
	BotBio(5)="No profile available. Level 9 security clearance required."
}
