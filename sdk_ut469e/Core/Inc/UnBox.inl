/*=============================================================================
	UnBox.inl: Unreal FBox inlines.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	NOTE: This file should ONLY be included by UnMath.h!
=============================================================================*/


//
// Constructors.
//
inline FBox::FBox(INT)
	: Min(0,0,0), Max(0,0,0), IsValid(0)
{
}

inline FBox::FBox( const FVector& InMin, const FVector& InMax )
	: Min(InMin), Max(InMax), IsValid(1)
{
}

//inline FBox::FBox( const FVector* Points, INT Count );

//
// Accessors
//
inline FVector& FBox::GetExtrema( int i )
{
	return (&Min)[i];
}

inline const FVector& FBox::GetExtrema( int i ) const
{
	return (&Min)[i];
}


//	
// Transformation functions
//
inline FBox& FBox::operator+=( const FVector &Other )
{
	if( IsValid )
	{
		Min.X = ::Min( Min.X, Other.X );
		Min.Y = ::Min( Min.Y, Other.Y );
		Min.Z = ::Min( Min.Z, Other.Z );

		Max.X = ::Max( Max.X, Other.X );
		Max.Y = ::Max( Max.Y, Other.Y );
		Max.Z = ::Max( Max.Z, Other.Z );
	}
	else
	{
		Min = Max = Other;
		IsValid = 1;
	}
	return *this;
}

inline FBox FBox::operator+( const FVector& Other ) const
{
	return FBox(*this) += Other;
}

inline FBox& FBox::operator+=( const FBox& Other )
{
	if( IsValid && Other.IsValid )
	{
		Min.X = ::Min( Min.X, Other.Min.X );
		Min.Y = ::Min( Min.Y, Other.Min.Y );
		Min.Z = ::Min( Min.Z, Other.Min.Z );

		Max.X = ::Max( Max.X, Other.Max.X );
		Max.Y = ::Max( Max.Y, Other.Max.Y );
		Max.Z = ::Max( Max.Z, Other.Max.Z );
	}
	else
		*this = Other;
	return *this;
}

inline FBox FBox::operator+( const FBox& Other ) const
{
	return FBox(*this) += Other;
}

inline FBox FBox::TransformBy( const FCoords& Coords ) const
{
	FBox NewBox(0);
	for( int i=0; i<2; i++ )
		for( int j=0; j<2; j++ )
			for( int k=0; k<2; k++ )
				NewBox += FVector( GetExtrema(i).X, GetExtrema(j).Y, GetExtrema(k).Z ).TransformPointBy( Coords );
	return NewBox;
}
inline FBox FBox::ExpandBy( FLOAT W ) const
{
	return FBox( Min - FVector(W,W,W), Max + FVector(W,W,W) );
}


//
// Checks functions
//
inline UBOOL FBox::Contains( const FVector& Other) const
{
	return IsValid 
		&& Other.X >= Min.X && Other.X <= Max.X
		&& Other.Y >= Min.Y && Other.Y <= Max.Y
		&& Other.Z >= Min.Z && Other.Z <= Max.Z;
}

inline FVector FBox::GetCenter() const
{
	return FVector( ( Min + Max ) * 0.5f );
}

