/*=============================================================================
ADecal.h
=============================================================================*/
	ADecal(){}

	class UTexture* AttachDecal( FLOAT TraceDistance=100.f, FVector DecalDir=FVector(0,0,0) );
	void DeatachDecal( BYTE bCleanup=0 );
	void PostScriptDestroyed();

/*-----------------------------------------------------------------------------
The End.
-----------------------------------------------------------------------------*/
