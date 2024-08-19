/*=============================================================================
	UnPlane.inl: Unreal FPlane inlines.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	NOTE: This file should ONLY be included by UnMath.h!
=============================================================================*/

//
// Constructors.
//
inline FPlane::FPlane()
{}

inline FPlane::FPlane( const FPlane& P )
	:	X(P.X)
	,	Y(P.Y)
	,	Z(P.Z)
	,	W(P.W)
{}

inline FPlane::FPlane( const FVector& V )
	:	X(V.X)
	,	Y(V.Y)
	,	Z(V.Z)
	,	W(0)
{}

inline FPlane::FPlane( FLOAT InX, FLOAT InY, FLOAT InZ, FLOAT InW )
	:	X(InX)
	,	Y(InY)
	,	Z(InZ)
	,	W(InW)
{}

inline FPlane::FPlane( FVector InNormal, FLOAT InW )
	:	X(InNormal.X)
	,	Y(InNormal.Y)
	,	Z(InNormal.Z)
	,	W(InW)
{}

inline FPlane::FPlane( FVector InBase, const FVector &InNormal )
	:	X(InNormal.X)
	,	Y(InNormal.Y)
	,	Z(InNormal.Z)
	,	W(InBase | InNormal)
{}

inline FPlane::FPlane( FVector A, FVector B, FVector C )
{
	FVector Normal = ((B-A)^(C-A)).SafeNormal();
	X = Normal.X;
	Y = Normal.Y;
	Z = Normal.Z;
	W = A | Normal;
}


//
// Functions.
//
inline FLOAT FPlane::PlaneDot( const FVector &P ) const
{
	return X*P.X + Y*P.Y + Z*P.Z - W;
}

inline FPlane FPlane::Flip() const
{
	return FPlane(-X,-Y,-Z,-W);
}


//
// Binary math operators.
//
inline FLOAT FPlane::operator|( const FPlane& V) const
{
	return X*V.X + Y*V.Y + Z*V.Z + W*V.W;
}

inline FLOAT FPlane::operator|( const FVector& V) const
{
	return X*V.X + Y*V.Y + Z*V.Z;;
}

inline FPlane FPlane::operator+( const FPlane& V ) const
{
	return FPlane( X+V.X, Y+V.Y, Z+V.Z, W+V.W);
}

inline FPlane FPlane::operator-( const FPlane& V ) const
{
	return FPlane( X-V.X, Y-V.Y, Z-V.Z, W-V.W);
}

inline FPlane FPlane::operator*( const FPlane& V ) const
{
	return FPlane( X*V.X, Y*V.Y, Z*V.Z, W*V.W);
}

inline FPlane FPlane::operator*( FLOAT Scale ) const
{
	return FPlane( X*Scale, Y*Scale, Z*Scale, W*Scale);
}

inline FPlane FPlane::operator/( FLOAT Scale ) const
{
	FLOAT RScale = 1.f / Scale;
	return FPlane( X*RScale, Y*RScale, Z*RScale, W*RScale);
}

inline FPlane FPlane::operator+( const FVector& V ) const
{
	return FPlane( X+V.X, Y+V.Y, Z+V.Z, W);
}

inline FPlane FPlane::operator-( const FVector& V ) const
{
	return FPlane( X-V.X, Y-V.Y, Z-V.Z, W);
}

inline FPlane FPlane::operator*( const FVector& V ) const
{
	return FPlane( X*V.X, Y*V.Y, Z*V.Z, W);
}


//
// Binary comparison operators
//
inline UBOOL FPlane::operator==( const FPlane& V ) const
{
	return X==V.X && Y==V.Y && Z==V.Z && W==V.W;
}

inline UBOOL FPlane::operator!=( const FPlane& V ) const
{
	return X!=V.X || Y!=V.Y || Z!=V.Z || W!=V.W;
}


//
// Assignment operators.
//
inline FPlane FPlane::operator+=( const FPlane& V )
{
	X += V.X;	Y += V.Y;	Z += V.Z;	W += V.W;
	return *this;
}

inline FPlane FPlane::operator-=( const FPlane& V )
{
	X -= V.X;	Y -= V.Y;	Z -= V.Z;	W -= V.W;
	return *this;
}

inline FPlane FPlane::operator*=( const FPlane& V )
{
	X *= V.X;	Y *= V.Y;	Z *= V.Z;	W *= V.W;
	return *this;
}

inline FPlane FPlane::operator*=( FLOAT Scale )
{
	X *= Scale;	Y *= Scale;	Z *= Scale;	W *= Scale;
	return *this;
}

inline FPlane FPlane::operator/=( FLOAT Scale )
{
	FLOAT RScale = 1.f / Scale;
	X *= RScale;	Y *= RScale;	Z *= RScale;	W *= RScale;
	return *this;
}

inline FPlane FPlane::operator+=( const FVector& V )
{
	X += V.X;	Y += V.Y;	Z += V.Z;
	return *this;
}

inline FPlane FPlane::operator-=( const FVector& V )
{
	X -= V.X;	Y -= V.Y;	Z -= V.Z;
	return *this;
}

inline FPlane FPlane::operator*=( const FVector& V )
{
	X *= V.X;	Y *= V.Y;	Z *= V.Z;
	return *this;
}
