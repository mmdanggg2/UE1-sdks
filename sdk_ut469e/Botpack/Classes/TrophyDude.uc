class TrophyDude extends Decoration;

var bool bFinal;
var int Pos;

function PostBeginPlay()
{
	SetTimer(38.0, True);
}

function Timer()
{
	SetTimer(0.0, False);
	if (HasAnim('Trophy5'))
		PlayAnim('Trophy5', 0.3);
	else 
		if (HasAnim('Victory1')) 
			PlayAnim('Victory1', 0.5);
	Pos = 0;
}

function AnimEnd()
{
	if (Pos < 2)
	{
		if (HasAnim('Trophy4')) 
		{
			PlayAnim('Trophy4', 0.3);
			Pos++;
		}
		else 
			if (Pos == 0 && HasAnim('Taunt1')) 
				PlayAnim('Taunt1', 0.4);
			else
				if (HasAnim('Breath1')) 
					PlayAnim('Breath1', 0.3);
		Pos++;
	}
}

defaultproperties
{
	bStatic=False
	DrawType=DT_Mesh
	Mesh=TrophyMale1
}

