/*=============================================================================
	UnActor.h: AActor class inlines.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
        * Aug 30, 1996: Mark added PLevel
		* Oct 19, 1996: Tim redesigned to eliminate redundency
=============================================================================*/

/*-----------------------------------------------------------------------------
	FActorLink.
-----------------------------------------------------------------------------*/

//
// Linked list of actors.
//
struct FActorLink
{
	// Variables.
	AActor*     Actor;
	FActorLink* Next;

	// Functions.
	FActorLink( AActor* InActor, FActorLink* InNext )
	: Actor(InActor), Next(InNext)
	{}
};

/*-----------------------------------------------------------------------------
	AActor inlines.
-----------------------------------------------------------------------------*/

//
// Brush checking.
//
inline UBOOL AActor::IsBrush()       const {return Brush!=NULL && IsA(ABrush::StaticClass());}
inline UBOOL AActor::IsStaticBrush() const {return Brush!=NULL && IsA(ABrush::StaticClass()) &&  bStatic;}
inline UBOOL AActor::IsMovingBrush() const {return Brush!=NULL && IsA(ABrush::StaticClass()) && !bStatic;}

//
// See if this actor is owned by TestOwner.
//
inline UBOOL AActor::IsOwnedBy( const AActor* TestOwner ) const
{
	guardSlow(AActor::IsOwnedBy);
	for( const AActor* Arg=this; Arg; Arg=Arg->Owner )
	{
		if( Arg == TestOwner )
			return 1;
	}
	return 0;
	unguardSlow;
}

//
// Get the top-level owner of an actor.
//
inline AActor* AActor::GetTopOwner()
{
	AActor* Top;
	for( Top=this; Top->Owner; Top=Top->Owner );
	return Top;
}


//
// See if this actor is in the specified zone.
//
inline UBOOL AActor::IsInZone( const AZoneInfo* TestZone ) const
{
	return Region.Zone!=Level ? Region.Zone==TestZone : 1;
}

//
// If this actor is a player, return it as a Pawn.
// Otherwise return NULL.
//
inline APlayerPawn* AActor::GetPlayerPawn() const
{
	guardSlow(AActor::GetPlayerPawn);

	// Only descendents of the pawn class may be players.
	if( !IsA(APlayerPawn::StaticClass()) )
		return NULL;

	// Players must have player objects.
	if( ((APlayerPawn*)this)->Player == NULL )
		return NULL;

	// This is a player.
	return (APlayerPawn*)this;

	unguardSlow;
}

//
// Return whether this actor looks like a player.
// Note - it could be a bot, so it may not have a viewport
//
inline UBOOL AActor::IsPlayer() const
{
	guardSlow(AActor::IsPlayer);

	// Only descendents of the pawn class may be players.
	if( !IsA(APawn::StaticClass()) )
		return 0;

	return ((APawn*)this)->bIsPlayer;

	unguardSlow;
}

//
// Determine if BlockingActor should block actors of the given class.
// This routine needs to be reflexive or else it will create funky
// results, i.e. A->IsBlockedBy(B) <-> B->IsBlockedBy(A).
//
inline UBOOL AActor::IsBlockedBy( const AActor* Other ) const
{
	guardSlow(AActor::IsBlockedBy);
	checkSlow(this!=NULL);
	checkSlow(Other!=NULL);

	if( Other==Level )
		return bCollideWorld;
	else if( Other->IsBrush() )
		return bCollideWorld && (GetPlayerPawn() ? Other->bBlockPlayers : Other->bBlockActors);
	else if ( IsBrush() )
		return Other->bCollideWorld && (Other->GetPlayerPawn() ? bBlockPlayers : bBlockActors);
	else
		return ( (GetPlayerPawn() || IsA(AProjectile::StaticClass())) ? Other->bBlockPlayers : Other->bBlockActors)
		&&	   ( (Other->GetPlayerPawn() || Other->IsA(AProjectile::StaticClass())) ? bBlockPlayers : bBlockActors);

	unguardSlow;
}

//
// Return whether this actor's movement is based on another actor.
//
inline UBOOL AActor::IsBasedOn( const AActor* Other ) const
{
	guard(AActor::IsBasedOn);
	for( const AActor* Test=this; Test!=NULL; Test=Test->Base )
		if( Test == Other )
			return 1;
	return 0;
	unguard;
}

//
// Return the level of an actor.
//
inline class ULevel* AActor::GetLevel() const
{
	return XLevel;
}

//
// Return the actor's view rotation.
//
inline FRotator AActor::GetViewRotation()
{
	guardSlow(AActor::GetViewRotation);
	return IsA(APawn::StaticClass()) ? ((APawn*)this)->ViewRotation : Rotation;
	unguardSlow;
}

//
// Return a timestamp for the actor's level
//
inline FString AActor::GetGameTimeStamp() const
{	
	AGameReplicationInfo* GRI = nullptr;
	
	// stijn: try to grab it directly from the viewport actor as this works better with UTPure
	if (UBitmap::__Client &&
		UBitmap::__Client->Viewports.Num() > 0 &&
		UBitmap::__Client->Viewports(0)->Actor &&
		(!XLevel || UBitmap::__Client->Viewports(0)->Actor->GetLevel() == XLevel) &&
		UBitmap::__Client->Viewports(0)->Actor->GameReplicationInfo)
	{
		GRI = UBitmap::__Client->Viewports(0)->Actor->GameReplicationInfo;
	}
	else if (XLevel &&
		XLevel->GetLevelInfo() && 
		XLevel->GetLevelInfo()->Game && 
		XLevel->GetLevelInfo()->Game->GameReplicationInfo)
	{
		GRI = XLevel->GetLevelInfo()->Game->GameReplicationInfo;		
	}

	if (GRI)
	{
		const INT Minute = GRI->RemainingTime / 60;
		const INT Second = GRI->RemainingTime % 60;
		return FString::Printf(TEXT("[%02d:%02d]"), Minute, Second);
	}
	if (XLevel && XLevel->GetLevelInfo())
	{
		return FString::Printf(TEXT("[%lf]"), XLevel->GetLevelInfo()->TimeSeconds);
	}
	return TEXT("[??:??]");
}


/*-----------------------------------------------------------------------------
	AActor audio.
-----------------------------------------------------------------------------*/

//
// Get the actor's primitive.
//
inline UPrimitive* AActor::GetPrimitive() const
{
	guardSlow(AActor::GetPrimitive);
	if     ( Brush  ) return Brush;
	else if( Mesh   ) return Mesh;
	else              return GetLevel()->Engine->Cylinder;
	unguardSlow;
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
