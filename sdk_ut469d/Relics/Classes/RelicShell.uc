class RelicShell expands Effects;

function Timer()
{
	Texture = None;
}

defaultproperties
{
	 bHidden=false
     RemoteRole=ROLE_SimulatedProxy
	 bOwnerNoSee=True
	 bNetTemporary=false
     DrawType=DT_Mesh
	 bAnimByOwner=True
	 bMeshEnviroMap=True
	 Fatness=157
	 Style=STY_Translucent
     DrawScale=1.00000
	 ScaleGlow=0.5
     AmbientGlow=64
	 bUnlit=true
	 Physics=PHYS_Trailer
	 Texture=none
	 bTrailerSameRotation=true
	 LODBias=+0.5
}