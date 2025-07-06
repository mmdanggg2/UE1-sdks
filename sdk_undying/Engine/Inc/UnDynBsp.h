/*=============================================================================
	UnDynBsp.h: Unreal dynamic Bsp object support
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

/*---------------------------------------------------------------------------------------
	FMovingBrushTrackerBase virtual base class.
---------------------------------------------------------------------------------------*/

//
// Moving brush tracker.
//
class FMovingBrushTrackerBase
{
	INT iSurfDynamicBase;
public:
	// Constructors/destructors.
	virtual ~FMovingBrushTrackerBase() noexcept(false) {};

	// Public operations:
	virtual void Update( AActor* Actor )=0;
	virtual void Flush( AActor* Actor )=0;
	inline UBOOL SurfIsDynamic(INT iSurf) { return iSurf >= iSurfDynamicBase; };
	virtual void CountBytes( FArchive& Ar )=0;
};
ENGINE_API FMovingBrushTrackerBase* GNewBrushTracker( ULevel* Level );

/*---------------------------------------------------------------------------------------
	The End.
---------------------------------------------------------------------------------------*/
