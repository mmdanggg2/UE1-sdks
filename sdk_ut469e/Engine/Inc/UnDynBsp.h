/*=============================================================================
	UnDynBsp.h: Unreal dynamic Bsp object support
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

/*---------------------------------------------------------------------------------------
	FMovingBrushTrackerBase virtual base class.
---------------------------------------------------------------------------------------*/

enum EUpdateMode
{
	UM_Normal = 0,
	UM_StartBatch = 1,
	UM_AddToBatch = 2,
	UM_EndBatch = 3,
	UM_DedicatedServer = 4,
	UM_Editor = 5,
};

//
// Moving brush tracker.
//
class FMovingBrushTrackerBase
{
public:
	// Constructors/destructors.
	virtual ~FMovingBrushTrackerBase() noexcept(false) {};

	// Public operations:
	virtual void Update( AActor* Actor, EUpdateMode UpdateMode = UM_Normal )=0;
	virtual void Flush( AActor* Actor )=0;
	virtual UBOOL SurfIsDynamic( INT iSurf )=0;
	virtual void CountBytes( FArchive& Ar )=0;
};
ENGINE_API FMovingBrushTrackerBase* GNewBrushTracker( ULevel* Level );

/*---------------------------------------------------------------------------------------
	The End.
---------------------------------------------------------------------------------------*/
