/*=============================================================================
	UnVector.inl: Unreal FVector inlines.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	NOTE: This file should ONLY be included by UnMath.h!
=============================================================================*/


//
// Constructors.
//
inline FVector::FVector()
{
}

inline FVector::FVector( FLOAT InX, FLOAT InY, FLOAT InZ )
	: X(InX), Y(InY), Z(InZ)
{
}

#if !MOD_BUILD
inline FVector::FVector( const FVector& V)
	: X(V.X), Y(V.Y), Z(V.Z)
{
}
#endif


//	
// Binary math operators.
//
inline FVector operator*( FLOAT Scale, const FVector& V )
{
	return FVector( V.X * Scale, V.Y * Scale, V.Z * Scale );
}

inline FVector FVector::operator^( const FVector& V ) const
{
	return FVector
	(
		Y * V.Z - Z * V.Y,
		Z * V.X - X * V.Z,
		X * V.Y - Y * V.X
	);
}

inline FLOAT FVector::operator|( const FVector& V ) const
{
	return X*V.X + Y*V.Y + Z*V.Z;
}

inline FVector FVector::operator+( const FVector& V ) const
{
	return FVector( X + V.X, Y + V.Y, Z + V.Z );
}

inline FVector FVector::operator-( const FVector& V ) const
{
	return FVector( X - V.X, Y - V.Y, Z - V.Z );
}

inline FVector FVector::operator*( FLOAT Scale ) const
{
	return FVector( X * Scale, Y * Scale, Z * Scale );
}

inline FVector FVector::operator/( FLOAT Scale ) const
{
	FLOAT RScale = 1.f/Scale;
	return FVector( X * RScale, Y * RScale, Z * RScale );
}

inline FVector FVector::operator*( const FVector& V ) const
{
	return FVector( X * V.X, Y * V.Y, Z * V.Z );
}

//
// Binary comparison operators.
//
inline UBOOL FVector::operator==( const FVector& V ) const
{
	return X==V.X && Y==V.Y && Z==V.Z;
}

inline UBOOL FVector::operator!=( const FVector& V ) const
{
	return X!=V.X || Y!=V.Y || Z!=V.Z;
}


//
// Unary operators.
//
inline FVector FVector::operator-() const
{
	return FVector( -X, -Y, -Z );
}


//
// Assignment operators.
//
inline FVector& FVector::operator=( const FVector& V)
{
	X = V.X;
	Y = V.Y;
	Z = V.Z;
	return *this;
}

inline FVector FVector::operator+=( const FVector& V )
{
	X += V.X;
	Y += V.Y;
	Z += V.Z;
	return *this;
}

inline FVector FVector::operator-=( const FVector& V )
{
	X -= V.X;
	Y -= V.Y;
	Z -= V.Z;
	return *this;
}

inline FVector FVector::operator*=( FLOAT Scale )
{
	X *= Scale;
	Y *= Scale;
	Z *= Scale;
	return *this;
}

inline FVector FVector::operator/=( FLOAT V )
{
	FLOAT RV = 1.f/V;
	X *= RV;
	Y *= RV;
	Z *= RV;
	return *this;
}

inline FVector FVector::operator*=( const FVector& V )
{
	X *= V.X;
	Y *= V.Y;
	Z *= V.Z;
	return *this;
}

inline FVector FVector::operator/=( const FVector& V )
{
	X /= V.X;
	Y /= V.Y;
	Z /= V.Z;
	return *this;
}

//
// Simple functions.
//
inline FLOAT FVector::Size() const
{
	return appSqrt( X*X + Y*Y + Z*Z );
}

inline FLOAT FVector::SizeSquared() const
{
	return X*X + Y*Y + Z*Z;
}

inline FLOAT FVector::Size2D() const 
{
	return appSqrt( X*X + Y*Y );
}

inline FLOAT FVector::SizeSquared2D() const 
{
	return X*X + Y*Y;
}

inline int FVector::IsNearlyZero() const
{
	return
		Abs(X)<KINDA_SMALL_NUMBER
		&&	Abs(Y)<KINDA_SMALL_NUMBER
		&&	Abs(Z)<KINDA_SMALL_NUMBER;
}

inline UBOOL FVector::IsZero() const
{
	return X==0.f && Y==0.f && Z==0.f;
}

inline UBOOL FVector::IsNan() const
{
	return appIsNan(X) || appIsNan(Y) || appIsNan(Z);
}

inline UBOOL FVector::Normalize()
{
	FLOAT SquareSum = X*X+Y*Y+Z*Z;
	if( SquareSum >= SMALL_NUMBER )
	{
		FLOAT Scale = 1.f/appSqrt(SquareSum);
		X *= Scale;
		Y *= Scale;
		Z *= Scale;
		return TRUE;
	}
	return FALSE;
}

inline FVector FVector::Projection() const
{
	FLOAT RZ = 1.f/Z;
	return FVector( X*RZ, Y*RZ, 1 );
}

#if !OLDUNREAL_BINARY_COMPAT
inline FVector FVector::SafeNormal() const
{
	FLOAT SquareSum = SizeSquared();
	if (SquareSum < SMALL_NUMBER)
		return FVector(0.f, 0.f, 0.f);

	FLOAT Size = appSqrt(SquareSum);
	FLOAT Scale = 1.f / Size;
	return FVector(*this) * Scale;
}
#endif

inline FVector FVector::UnsafeNormal() const
{
	FLOAT Scale = 1.f/appSqrt(X*X+Y*Y+Z*Z);
	return FVector( X*Scale, Y*Scale, Z*Scale );
}

inline FVector FVector::GridSnap( const FVector& Grid )
{
	return FVector
	(
		FSnap(X, Grid.X),
		FSnap(Y, Grid.Y),
		FSnap(Z, Grid.Z)
	);
}

inline FVector FVector::BoundToCube( FLOAT Radius )
{
	return FVector
	(
		Clamp(X,-Radius,Radius),
		Clamp(Y,-Radius,Radius),
		Clamp(Z,-Radius,Radius)
	);
}

inline void FVector::AddBounded( const FVector& V, FLOAT Radius )
{
	*this = (*this + V).BoundToCube(Radius);
}

inline FLOAT& FVector::Component( INT Index )
{
	return (&X)[Index];
}

inline const FLOAT& FVector::Component( INT Index ) const
{
	return (&X)[Index];
}

inline FLOAT FVector::SizeFast() const
{
	return 1.f / appFastInvSqrt(X * X + Y * Y + Z * Z);
}

inline FLOAT FVector::SizeFastN0() const
{
	return 1.f / appFastInvSqrtN0(X * X + Y * Y + Z * Z);
}

inline FLOAT FVector::SizeFastN1() const
{
	return 1.f / appFastInvSqrtN1(X * X + Y * Y + Z * Z);
}

inline FLOAT FVector::SizeFastN2() const
{
	return 1.f / appFastInvSqrtN2(X * X + Y * Y + Z * Z);
}

inline FLOAT FVector::SizeFastN3() const
{
	return 1.f / appFastInvSqrtN3(X * X + Y * Y + Z * Z);
}

inline FLOAT FVector::Size2DFast() const
{
	return 1.f / appFastInvSqrt(X * X + Y * Y);
}

inline FLOAT FVector::Size2DFastN0() const
{
	return 1.f / appFastInvSqrtN0(X * X + Y * Y);
}

inline FLOAT FVector::Size2DFastN1() const
{
	return 1.f / appFastInvSqrtN1(X * X + Y * Y);
}

inline FLOAT FVector::Size2DFastN2() const
{
	return 1.f / appFastInvSqrtN2(X * X + Y * Y);
}

inline FLOAT FVector::Size2DFastN3() const
{
	return 1.f / appFastInvSqrtN3(X * X + Y * Y);
}


// Return a boolean that is based on the vector's direction.
// When      V==(0,0,0) Booleanize(0)=1.
// Otherwise Booleanize(V) <-> !Booleanize(!B).
inline UBOOL FVector::Booleanize()
{
	return
		X >  0.f ? 1 :
		X <  0.f ? 0 :
		Y >  0.f ? 1 :
		Y <  0.f ? 0 :
		Z >= 0.f ? 1 : 0;
}


/*-----------------------------------------------------------------------------
	Transformation.
-----------------------------------------------------------------------------*/

//
// Transform a point by a coordinate system, moving
// it by the coordinate system's origin if nonzero.
//
inline FVector FVector::TransformPointBy( const FCoords &Coords ) const
{
	FVector Temp = *this - Coords.Origin;
	return FVector(	Temp | Coords.XAxis, Temp | Coords.YAxis, Temp | Coords.ZAxis );
}

//
// Transform a directional vector by a coordinate system.
// Ignore's the coordinate system's origin.
//
inline FVector FVector::TransformVectorBy( const FCoords &Coords ) const
{
	return FVector(	*this | Coords.XAxis, *this | Coords.YAxis, *this | Coords.ZAxis );
}

//
// Apply 'pivot' transform: First rotate, then add the translation.
//
inline FVector FVector::PivotTransform(const FCoords& Coords) const
{
	return Coords.Origin + FVector( *this | Coords.XAxis, *this | Coords.YAxis, *this | Coords.ZAxis );
}

//
// Mirror a vector about a normal vector.
//
inline FVector FVector::MirrorByVector( const FVector& MirrorNormal ) const
{
	return *this - MirrorNormal * (2.f * (*this | MirrorNormal));
}

//
// Mirror a vector about a plane.
//
inline FVector FVector::MirrorByPlane( const FPlane& Plane ) const
{
	return *this - (FVector)Plane * (2.f * Plane.PlaneDot(*this) );
}


/*-----------------------------------------------------------------------------
	Friends.
-----------------------------------------------------------------------------*/

//
// Euclidean distance between two points.
//
inline FLOAT FDist( const FVector &V1, const FVector &V2 )
{
	return appSqrt( Square(V2.X-V1.X) + Square(V2.Y-V1.Y) + Square(V2.Z-V1.Z) );
}

//
// Squared distance between two points.
//
inline FLOAT FDistSquared( const FVector &V1, const FVector &V2 )
{
	return Square(V2.X-V1.X) + Square(V2.Y-V1.Y) + Square(V2.Z-V1.Z);
}

//
// Euclidean distance between two points in xy plane.
//
inline FLOAT FDist2D( const FVector &V1, const FVector &V2 )
{
	return appSqrt( Square(V2.X-V1.X) + Square(V2.Y-V1.Y) );
}

//
// Squared distance between two points in xy plane.
//
inline FLOAT FDist2DSquared( const FVector &V1, const FVector &V2 )
{
	return Square(V2.X-V1.X) + Square(V2.Y-V1.Y);
}

//
// Triple product of three vectors.
//
inline FLOAT FTriple( const FVector& X, const FVector& Y, const FVector& Z )
{
	return
		(	(X.X * (Y.Y * Z.Z - Y.Z * Z.Y))
		+	(X.Y * (Y.Z * Z.X - Y.X * Z.Z))
		+	(X.Z * (Y.X * Z.Y - Y.Y * Z.X)) );
}
