class SkaarjVoice extends VoiceMaleTwo;

function Timer()
{
	local name MessageType;

	if ( bDelayedResponse )
	{
		bDelayedResponse = false;
		if ( PlayerPawn(Owner) != None )
		{
			if ( PlayerPawn(Owner).GameReplicationInfo.bTeamGame 
				 && (PlayerPawn(Owner).PlayerReplicationInfo.Team == DelayedSender.Team) )
				MessageType = 'TeamSay';
			else
				MessageType = 'Say';
			PlayerPawn(Owner).TeamMessage(DelayedSender, DelayedResponse, MessageType, false);
		}
	}
	if ( Phrase[PhraseNum] != None )
	{
		if ( PlayerPawn(Owner) != None && !PlayerPawn(Owner).bNoVoices 
			&& (Level.TimeSeconds - PlayerPawn(Owner).LastPlaySound > 2)  ) 
		{
			if ( (PlayerPawn(Owner).ViewTarget != None) && !PlayerPawn(Owner).ViewTarget.IsA('Carcass') )
			{
				PlayerPawn(Owner).ViewTarget.PlaySound(Phrase[PhraseNum], SLOT_Interface, 16.0,,,0.7);
				PlayerPawn(Owner).ViewTarget.PlaySound(Phrase[PhraseNum], SLOT_Misc, 16.0,,,0.7);
			}
			else
			{
				PlayerPawn(Owner).PlaySound(Phrase[PhraseNum], SLOT_Interface, 16.0,,,0.7);
				PlayerPawn(Owner).PlaySound(Phrase[PhraseNum], SLOT_Misc, 16.0,,,0.7);
			}
		}
		if ( PhraseTime[PhraseNum] == 0 )
			Destroy();
		else
		{
			SetTimer(PhraseTime[PhraseNum], false);
			PhraseNum++;
		}
	}
	else 
		Destroy();
}

defaultproperties
{
	TauntSound(0)=UnrealShare.chalnge1s
	TauntSound(1)=UnrealShare.chalnge3s
	TauntSound(2)=UnrealShare.roam11s
	TauntString(0)="$#%#!"
	TauntString(1)="%&$##!"
	TauntString(2)="*@#&$!"
}
