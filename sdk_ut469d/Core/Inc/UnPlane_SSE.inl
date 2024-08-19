/*=============================================================================
	UnPlane_SSE.inl: Unreal FPlane SSE inlines.
	Based on CacusLib's math library.

	Revision history:
		* Created by Fernando Velazquez (Higor)

	NOTE: This file should ONLY be included by UnMath.h!
=============================================================================*/



//
// Constructors.
//
inline FPlane::FPlane()
{}

inline FPlane::FPlane( const FPlane& P )
{
	_mm_storeu_ps( &X, _mm(P) );
}

inline FPlane::FPlane( const FVector& V )
{
	_mm_storeu_ps( &X, _mm(V) ); //W=0
}

inline FPlane::FPlane( FLOAT InX, FLOAT InY, FLOAT InZ, FLOAT InW )
{
	_mm_storeu_ps( &X, _mm_set_ps(InW,InZ,InY,InX));
}

inline FPlane::FPlane( FVector InNormal, FLOAT InW )
	: X(InNormal.X)
	, Y(InNormal.Y)
	, Z(InNormal.Z)
	, W(InW)
{} // TODO: Vectorize

inline FPlane::FPlane( FVector InBase, const FVector &InNormal )
	: X(InNormal.X)
	, Y(InNormal.Y)
	, Z(InNormal.Z)
	, W(InBase | InNormal)
{} // TODO: Vectorize

inline FPlane::FPlane( FVector A, FVector B, FVector C )
{
	FVector Normal = ((B-A)^(C-A)).SafeNormal();
	X = Normal.X;
	Y = Normal.Y;
	Z = Normal.Z;
	W = A | Normal;
} // TODO: Vectorize result


//
// Functions.
//
inline FLOAT FPlane::PlaneDot( const FVector &P ) const
{
	// Used to fill the W coordinate with -1
	constexpr __m128 MM_PLANEDOT_W{ 0.f, 0.f, 0.f, -1.f};

	FLOAT Result;
	__m128 mm_x_y_z_w = _mm_or_ps( _mm(P), MM_PLANEDOT_W ); //PX,PY,PZ,-1
	mm_x_y_z_w = _mm_mul_ps( mm_x_y_z_w, _mm(*this) ); //X*PX, Y*PY, Z*PZ, -W   (x,y,z,w)
	_mm_store_ss( &Result, _mm_coords_sum_ps(mm_x_y_z_w));
	return Result;
}

inline FPlane FPlane::Flip() const
{
	FPlane Result;
	_mm_storeu_ps( &Result.X, _mm_sub_ps( _mm_setzero_ps(), _mm(*this)) );
	return Result;
}


//
// Binary math operators.
//
inline FLOAT FPlane::operator|( const FPlane& V) const
{
	FLOAT Result;
	__m128 mm_dot = _mm_coords_sum_ps( _mm_mul_ps( _mm(V), _mm(*this)) );
	_mm_store_ss( &Result, mm_dot);
	return Result;
}

inline FLOAT FPlane::operator|( const FVector& V) const
{
	FLOAT Result;
	__m128 mm_dot = _mm_coords_sum_ps( _mm_mul_ps( _mm(V), _mm(*this)) );
	_mm_store_ss( &Result, mm_dot);
	return Result;
}

inline FPlane FPlane::operator+( const FPlane& V ) const
{
	FPlane Result;
	_mm_storeu_ps( &Result.X, _mm_add_ps( _mm(*this), _mm(V)) );
	return Result;
}

inline FPlane FPlane::operator-( const FPlane& V ) const
{
	FPlane Result;
	_mm_storeu_ps( &Result.X, _mm_sub_ps( _mm(*this), _mm(V)) );
	return Result;
}

inline FPlane FPlane::operator*( const FPlane& V ) const
{
	FPlane Result;
	_mm_storeu_ps( &Result.X, _mm_mul_ps( _mm(*this), _mm(V)) );
	return Result;
}

inline FPlane FPlane::operator*( FLOAT Scale ) const
{
	FPlane Result;
	_mm_storeu_ps( &Result.X, _mm_mul_ps( _mm(*this), _mm_load1_ps(&Scale)) );
	return Result;
}

inline FPlane FPlane::operator/( FLOAT Scale ) const
{
	FPlane Result;
	FLOAT RScale = 1.f / Scale;
	_mm_storeu_ps( &Result.X, _mm_mul_ps( _mm(*this), _mm_load1_ps(&RScale)) );
	return Result;
}

inline FPlane FPlane::operator+( const FVector& V ) const
{
	FPlane Result;
	_mm_storeu_ps( &Result.X, _mm_add_ps( _mm(*this), _mm(V)) );
	return Result;
}

inline FPlane FPlane::operator-( const FVector& V ) const
{
	FPlane Result;
	_mm_storeu_ps( &Result.X, _mm_sub_ps( _mm(*this), _mm(V)) );
	return Result;
}

inline FPlane FPlane::operator*( const FVector& V ) const
{
	constexpr __m128 MM_ONE_W{ 0.f, 0.f, 0.f, 1.f };

	FPlane Result;
	__m128 mm_v = _mm_or_ps( _mm(V), MM_ONE_W ); // W must be 1
	_mm_storeu_ps( &Result.X, _mm_mul_ps( _mm(*this), mm_v) );
	return Result;
}

//
// Binary comparison operators
//
inline UBOOL FPlane::operator==( const FPlane& V ) const
{
	return _mm_movemask_ps( _mm_cmpeq_ps( _mm(*this), _mm(V)) ) == 0b1111;
}

inline UBOOL FPlane::operator!=( const FPlane& V ) const
{
	return _mm_movemask_ps( _mm_cmpneq_ps( _mm(*this), _mm(V)) ) != 0b0000;
}

//
// Assignment operators.
//
inline FPlane FPlane::operator+=( const FPlane& V )
{
	_mm_storeu_ps( &X, _mm_add_ps( _mm(*this), _mm(V)) );
	return *this;
}

inline FPlane FPlane::operator-=( const FPlane& V )
{
	_mm_storeu_ps( &X, _mm_sub_ps( _mm(*this), _mm(V)) );
	return *this;
}

inline FPlane FPlane::operator*=( const FPlane& V )
{
	_mm_storeu_ps( &X, _mm_mul_ps( _mm(*this), _mm(V)) );
	return *this;
}

inline FPlane FPlane::operator*=( FLOAT Scale )
{
	_mm_storeu_ps( &X, _mm_mul_ps( _mm(*this), _mm_load1_ps(&Scale)) );
	return *this;
}

inline FPlane FPlane::operator/=( FLOAT Scale )
{
	FLOAT RScale = 1.f / Scale;
	_mm_storeu_ps( &X, _mm_mul_ps( _mm(*this), _mm_load1_ps(&RScale)) );
	return *this;
}

inline FPlane FPlane::operator+=( const FVector& V )
{
	_mm_storeu_ps( &X, _mm_add_ps( _mm(*this), _mm(V)) );
	return *this;
}

inline FPlane FPlane::operator-=( const FVector& V )
{
	_mm_storeu_ps( &X, _mm_sub_ps( _mm(*this), _mm(V)) );
	return *this;
}

inline FPlane FPlane::operator*=( const FVector& V )
{
	constexpr __m128 MM_ONE_W{ 0.f, 0.f, 0.f, 1.f };

	__m128 mm_v = _mm_or_ps( _mm(V), MM_ONE_W ); // W must be 1
	_mm_storeu_ps( &X, _mm_mul_ps( _mm(*this), mm_v) );
	return *this;
}
