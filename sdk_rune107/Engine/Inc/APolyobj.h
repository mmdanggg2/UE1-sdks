/*=============================================================================
	APolyobj.h: Class functions residing in the APolyobj class.

=============================================================================*/

	// Constructors.
	APolyobj();

	// UObject interface.
	void PostLoad();
	void PostEditChange();

	// AActor interface.
	void Spawned();
	void PostEditMove();
	void PreRaytrace();
	void PostRaytrace();

	// ABrush interface.
	virtual void SetWorldRaytraceKey();
	virtual void SetBrushRaytraceKey();
	INT* GetOptimizedRepList( BYTE* InDefault, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map );

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
