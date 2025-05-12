/*=============================================================================
	UnModel.h: Unreal UModel definition.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

/*-----------------------------------------------------------------------------
	UModel.
-----------------------------------------------------------------------------*/

// Just lets TTransArray syntax continue to work with the pointers
template <class T>
class MagicPtr {
public:
	T* ptr;

	// Optional smart-pointer-like access
	T* operator->() { return ptr; }
	const T* operator->() const { return ptr; }

	T& operator*() { return *ptr; }
	const T& operator*() const { return *ptr; }

	// Transparent forwarding of operator()
	auto operator()(INT i) const -> decltype((*ptr)(i)) {
		return (*ptr)(i);
	}

	// Transparent forwarding of Num()
	template <typename... Args>
	auto Num(Args&&... args) const -> decltype(ptr->Num(std::forward<Args>(args)...)) {
		return ptr->Num(std::forward<Args>(args)...);
	}
};

struct DBOwner{
	UObject* dbOwner;
};

// Looks like what TTransArray evolved from, looks like the same structure but has extra object ptr before it.
template <class T>
class UDatabase : UObject, DBOwner, public TTransArray<T> {};

class UVectors : public UDatabase<FVector> {};
class UBspNodes : public UDatabase<FBspNode> {};
class UBspSurfs: public UDatabase<FBspSurf> {};
class UVerts : public UDatabase<FVert> {};

//
// Model objects are used for brushes and for the level itself.
//
enum {MAX_NODES  = 65536};
enum {MAX_POINTS = 128000};
class ENGINE_API UModel : public UPrimitive
{
	DECLARE_CLASS(UModel,UPrimitive,CLASS_RuntimeStatic)

	// Arrays and subobjects.
	MagicPtr<UVectors> Vectors;
	MagicPtr<UVectors> Points;
	MagicPtr<UBspNodes>	Nodes;
	MagicPtr<UBspSurfs>	Surfs;
	MagicPtr<UVerts>	Verts;
	UPolys*					Polys;
	TArray<FLightMapIndex>	LightMap;
	TArray<BYTE>			LightBits;
	TArray<FBox>			Bounds;
	TArray<INT>				LeafHulls;
	TArray<FLeaf>			Leaves;
	TArray<AActor*>			Lights;

	// Other variables.
	UBOOL					RootOutside;
	UBOOL					Linked;
	INT						MoverLink;
	INT						NumSharedSides;
	INT						NumZones;
	FZoneProperties			Zones[FBspNode::MAX_ZONES];

	// Constructors.
	UModel()
	: RootOutside( 1 )
	{
		EmptyModel( 1, 0 );
	}
	UModel( ABrush* Owner, UBOOL InRootOutside=1 );

	// UObject interface.
	void Serialize( FArchive& Ar );

	// UPrimitive interface.
	UBOOL PointCheck
	(
		FCheckResult	&Result,
		AActor			*Owner,
		FVector			Location,
		FVector			Extent,
		DWORD           ExtraNodeFlags
	);
	UBOOL LineCheck
	(
		FCheckResult	&Result,
		AActor			*Owner,
		FVector			End,
		FVector			Start,
		FVector			Extent,
		DWORD           ExtraNodeFlags
	);
	FBox GetCollisionBoundingBox( const AActor *Owner ) const;
	FBox GetRenderBoundingBox( const AActor* Owner, UBOOL Exact );

	// UModel interface.
	void Modify( UBOOL DoTransArrays=0 );
	void BuildBound();
	void Transform( ABrush* Owner );
	void EmptyModel( INT EmptySurfInfo, INT EmptyPolys );
	void ShrinkModel();
	UBOOL PotentiallyVisible( INT iLeaf1, INT iLeaf2 );
	BYTE FastLineCheck( FVector End, FVector Start );

	// UModel transactions.
	void ModifySelectedSurfs( UBOOL UpdateMaster );
	void ModifyAllSurfs( UBOOL UpdateMaster );
	void ModifySurf( INT Index, UBOOL UpdateMaster );

	// UModel collision functions.
	typedef void (*PLANE_FILTER_CALLBACK )(UModel *Model, INT iNode, int Param);
	typedef void (*SPHERE_FILTER_CALLBACK)(UModel *Model, INT iNode, int IsBack, int Outside, int Param);
	FPointRegion PointRegion( AZoneInfo* Zone, FVector Location ) const;
	FLOAT FindNearestVertex
	(
		const FVector	&SourcePoint,
		FVector			&DestPoint,
		FLOAT			MinRadius,
		INT				&pVertex
	) const;
	void PrecomputeSphereFilter
	(
		const FPlane	&Sphere
	);
	FLightMapIndex* GetLightMapIndex( INT iSurf )
	{
		guard(UModel::GetLightMapIndex);
		if( iSurf == INDEX_NONE ) return NULL;
		FBspSurf& Surf = Surfs(iSurf);
		if( Surf.iLightMap==INDEX_NONE || !LightMap.Num() ) return NULL;
		return &LightMap(Surf.iLightMap);
		unguard;
	}
};

/*----------------------------------------------------------------------------
	The End.
----------------------------------------------------------------------------*/
