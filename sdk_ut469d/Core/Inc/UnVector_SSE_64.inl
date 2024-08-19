/*=============================================================================
	UnVector_SSE_64.inl: Unreal FVector SSE inlines for 64 bit systems.
	Based on CacusLib's math library.

When vector struct has a size of 16 bytes, we can safely load/store the 4th
float after Z.

By setting the golden rule of:
- 4th FLOAT is irrelevant in the result of the operation.
We can proceed to optimize the code by not zeroing said component on ANY
operation that returns a FVector, while zeroing it on all other operations that
do return something else.

	Revision history:
		* Created by Fernando Velazquez (Higor)

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
} //This can construct .data section constants, do not use SSE instructions

#if !MOD_BUILD
inline FVector::FVector( const FVector& V)
{
	_mm_storeu_ps( &X, _mm<false>(V));
}
#endif

//	
// Binary math operators.
//
inline FVector operator*( FLOAT Scale, const FVector& V )
{
	return V * Scale;
}

inline FVector FVector::operator^( const FVector& V ) const
{
	FVector Result;
	__m128 mm_xyz_t = _mm<false>(*this);
	__m128 mm_xyz_v = _mm<false>(V);
	__m128 mm_yzx_t = _mm_pshufd_ps( mm_xyz_t, 0b11001001); //PSHUFD is larger, but doesn't require a previous copy (via MOVAPS)
	__m128 mm_zxy_v = _mm_pshufd_ps( mm_xyz_v, 0b11010010);
	__m128 mm_zxy_t = _mm_pshufd_ps( mm_xyz_t, 0b11010010);
	__m128 mm_yzx_v = _mm_pshufd_ps( mm_xyz_v, 0b11001001);
	__m128 mm_cross = _mm_sub_ps
		(	_mm_mul_ps( mm_yzx_t, mm_zxy_v)
		,	_mm_mul_ps( mm_zxy_t, mm_yzx_v) );
	_mm_storeu_ps( &Result.X, mm_cross);
	return Result;
}

inline FLOAT FVector::operator|( const FVector& V ) const
{
	FLOAT Result;
	__m128 mm_mul   = _mm_mul_ps( _mm<false>(V), _mm<false>(*this));
	__m128 mm_mul3d = _mm_and_ps( mm_mul, _mm_castsi128_ps(MM_3D_MASK));
	__m128 mm_dot   = _mm_coords_sum_ps(mm_mul3d);
	_mm_store_ss( &Result, mm_dot);
	return Result;
//	return X*V.X + Y*V.Y + Z*V.Z;
}

inline FVector FVector::operator+( const FVector& V ) const
{
	FVector Result;
	_mm_storeu_ps( &Result.X, _mm_add_ps( _mm<false>(*this), _mm<false>(V)) );
	return Result;
}

inline FVector FVector::operator-( const FVector& V ) const
{
	FVector Result;
	_mm_storeu_ps( &Result.X, _mm_sub_ps( _mm<false>(*this), _mm<false>(V)) );
	return Result;
}

inline FVector FVector::operator*( FLOAT Scale ) const
{
	FVector Result;
	_mm_storeu_ps( &Result.X, _mm_mul_ps( _mm<false>(*this), _mm_load1_ps(&Scale)) );
	return Result;
}

inline FVector FVector::operator/( FLOAT V ) const
{
	FVector Result;
	FLOAT RV = 1.f/V;
	_mm_storeu_ps( &Result.X, _mm_mul_ps( _mm<false>(*this), _mm_load1_ps(&RV)) );
	return Result;
}

inline FVector FVector::operator*( const FVector& V ) const
{
	FVector Result;
	_mm_storeu_ps( &Result.X, _mm_mul_ps( _mm<false>(*this), _mm<false>(V)) );
	return Result;
}


//
// Binary comparison operators.
//
inline UBOOL FVector::operator==( const FVector& V ) const
{
	return (_mm_movemask_ps( _mm_cmpeq_ps( _mm<false>(*this), _mm<false>(V)) ) & 0b0111) == 0b0111;
}

inline UBOOL FVector::operator!=( const FVector& V ) const
{
	return (_mm_movemask_ps( _mm_cmpneq_ps( _mm<false>(*this), _mm<false>(V)) ) & 0b0111) != 0b0000;
}


//
// Unary operators.
//
inline FVector FVector::operator-() const
{
	//0x80000000, 0x80000000, 0x80000000, 0x00000000
	constexpr __m128 MM_XOR_SIGN{ -0.f, -0.f, -0.f,  0.f };

	FVector Result;
	_mm_storeu_ps( &Result.X, _mm_xor_ps(_mm<false>(*this), MM_XOR_SIGN) );
	return Result;
}


//
// Assignment operators.
//
inline FVector& FVector::operator=( const FVector& V)
{
	_mm_storeu_ps( &X, _mm<false>(V));
	return *this;
}

inline FVector FVector::operator+=( const FVector& V )
{
	_mm_storeu_ps( &X, _mm_add_ps( _mm<false>(*this), _mm<false>(V)) );
	return *this;
}

inline FVector FVector::operator-=( const FVector& V )
{
	_mm_storeu_ps( &X, _mm_sub_ps( _mm<false>(*this), _mm<false>(V)) );
	return *this;
}

inline FVector FVector::operator*=( FLOAT Scale )
{
	_mm_storeu_ps( &X, _mm_mul_ps( _mm<false>(*this), _mm_load1_ps(&Scale)) );
	return *this;
}

inline FVector FVector::operator/=( FLOAT V )
{
	FLOAT RV = 1.f / V;
	_mm_storeu_ps( &X, _mm_mul_ps( _mm<false>(*this), _mm_load1_ps(&RV)) );
	return *this;
}

inline FVector FVector::operator*=( const FVector& V )
{
	_mm_storeu_ps( &X, _mm_mul_ps( _mm<false>(*this), _mm<false>(V)) );
	return *this;
}

inline FVector FVector::operator/=( const FVector& V )
{
	_mm_storeu_ps( &X, _mm_div_ps( _mm<false>(*this), _mm<false>(V)) );
	return *this;

}


//
// Simple functions.
//
inline FLOAT FVector::Size() const
{
	FLOAT Result;
	__m128 mm_this = _mm(*this);
	__m128 mm_mul  = _mm_mul_ps( mm_this, mm_this);
	__m128 mm_dot  = _mm_coords_sum_ps(mm_mul);
	_mm_store_ss( &Result, _mm_sqrt_ss(mm_dot));
	return Result;
}

inline FLOAT FVector::SizeSquared() const
{
	FLOAT Result;
	__m128 mm_this = _mm(*this);
	__m128 mm_mul  = _mm_mul_ps( mm_this, mm_this);
	__m128 mm_dot  = _mm_coords_sum_ps(mm_mul);
	_mm_store_ss( &Result, mm_dot);
	return Result;
}

inline FLOAT FVector::Size2D() const 
{
	FLOAT Result;
	__m128 mm_this = _mm<false>(*this);
	__m128 mm_mul  = _mm_mul_ps( mm_this, mm_this);
	__m128 mm_dot  = _mm_coords_sum_xy_ps(mm_mul);
	_mm_store_ss( &Result, _mm_sqrt_ss(mm_dot));
	return Result;
}

inline FLOAT FVector::SizeSquared2D() const 
{
	FLOAT Result;
	__m128 mm_this = _mm<false>(*this);
	__m128 mm_mul  = _mm_mul_ps( mm_this, mm_this);
	__m128 mm_dot  = _mm_coords_sum_xy_ps(mm_mul);
	_mm_store_ss( &Result, mm_dot);
	return Result;
}

inline int FVector::IsNearlyZero() const
{
	__m128 mm_abs_this = _mm_and_ps( _mm<false>(*this), _mm_castsi128_ps(MM_ABS_MASK));
	__m128 mm_kinda_small_vector = _mm_set_ps( KINDA_SMALL_NUMBER, KINDA_SMALL_NUMBER, KINDA_SMALL_NUMBER, KINDA_SMALL_NUMBER);
	return (_mm_movemask_ps( _mm_cmplt_ps( mm_abs_this, mm_kinda_small_vector)) & 0b0111) == 0b0111;
}

inline UBOOL FVector::IsZero() const
{
	__m128 mm_zero = _mm_setzero_ps();
	return (_mm_movemask_ps( _mm_cmpeq_ps( _mm<false>(*this), mm_zero)) & 0b0111) == 0b0111;
}

inline UBOOL FVector::IsNan() const
{
	// Result of (X,Y,Z,W) & (m,m,m,0) == (m,m,m,0)
	// NOTE: 0x7F800000 represents (inf) and isnan returns 0
	__m128i mm_this = _mm_castps_si128( _mm<0>(*this) );
	__m128i mm_nan_mask = _mm_set_epi32( 0x00000000, 0x7FC00000, 0x7FC00000, 0x7FC00000);
	__m128i mm_nan_test = _mm_cmpeq_epi32( _mm_and_si128(mm_this,mm_nan_mask), mm_nan_mask );
	return (_mm_movemask_ps( _mm_castsi128_ps(mm_nan_test)) & 0b0111) != 0;
}

inline UBOOL FVector::Normalize() 
{
	FLOAT SquareSum = SizeSquared();
	if( SquareSum >= SMALL_NUMBER )
	{
		FLOAT Scale = appFastInvSqrtSSE(SquareSum);
		(*this) *= Scale;
		return 1;
	}
	else
		return 0;
}

inline FVector FVector::Projection() const
{
	FVector Result;
	const FLOAT One = 1.f;
	__m128 mm_one  = _mm_load_ss(&One);    //1,0,0,0
	__m128 mm_this = _mm<false>(*this);    //X,Y,Z,?
	__m128 mm_div  = _mm_shuffle_ps( mm_this, mm_one, 0b00001010); //Z,Z,1,1
	_mm_store_ps( &Result.X, _mm_div_ps(mm_this,mm_div)); //X*RZ, Y*RZ, 1, ? 
	return Result;
}

#if !OLDUNREAL_BINARY_COMPAT
inline FVector FVector::SafeNormal() const
{
	FLOAT SquareSum = SizeSquared();
	if (SquareSum < SMALL_NUMBER)
		return FVector(0.f, 0.f, 0.f);

	FLOAT Scale = appFastInvSqrtSSE(SquareSum);
	return FVector(*this) * Scale;
}
#endif

inline FVector FVector::UnsafeNormal() const
{
	return (*this) * appFastInvSqrtSSE( SizeSquared() );
}

inline FVector FVector::GridSnap( const FVector& Grid )
{
	//TODO
	return FVector
	(
		FSnap(X, Grid.X),
		FSnap(Y, Grid.Y),
		FSnap(Z, Grid.Z)
	);
}

inline FVector FVector::BoundToCube( FLOAT Radius )
{
	//TODO
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
	return 1.f / appFastInvSqrt( SizeSquared() );
}

inline FLOAT FVector::SizeFastN0() const
{
	return 1.f / appFastInvSqrtN0( SizeSquared() );
}

inline FLOAT FVector::SizeFastN1() const
{
	return 1.f / appFastInvSqrtN1( SizeSquared() );
}

inline FLOAT FVector::SizeFastN2() const
{
	return 1.f / appFastInvSqrtN2( SizeSquared() );
}

inline FLOAT FVector::SizeFastN3() const
{
	return 1.f / appFastInvSqrtN3( SizeSquared() );
}

inline FLOAT FVector::Size2DFast() const
{
	return 1.f / appFastInvSqrt( SizeSquared2D() );
}

inline FLOAT FVector::Size2DFastN0() const
{
	return 1.f / appFastInvSqrtN0( SizeSquared2D() );
}

inline FLOAT FVector::Size2DFastN1() const
{
	return 1.f / appFastInvSqrtN1( SizeSquared2D() );
}

inline FLOAT FVector::Size2DFastN2() const
{
	return 1.f / appFastInvSqrtN2( SizeSquared2D() );
}

inline FLOAT FVector::Size2DFastN3() const
{
	return 1.f / appFastInvSqrtN3( SizeSquared2D() );
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
	return Temp.TransformVectorBy( Coords);
}

//
// Transform a directional vector by a coordinate system.
// Ignore's the coordinate system's origin.
//
inline FVector FVector::TransformVectorBy( const FCoords &Coords ) const
{
	FVector Result;

	// Instead of calculating dot products individually, we'll do it in a single
	// operation by transposing the results and then adding all 3 vectors
	__m128 m_this = _mm<false>(*this);
	__m128 m_Xl = _mm_mul_ps( m_this, _mm<false>(Coords.XAxis)); //Xx,Xy,Xz
	__m128 m_Yl = _mm_mul_ps( m_this, _mm<false>(Coords.YAxis)); //Yx,Yy,Yz
	__m128 m_Zl = _mm_mul_ps( m_this, _mm<false>(Coords.ZAxis)); //Zx,Zy,Zz
	//Target:
	// Xx,Yx,Zx
	// Xy,Yy,Zy
	// Xz,Yz,Zz
	__m128 m_tmp1 = _mm_unpacklo_ps( m_Xl, m_Yl); //Xx,Yx,Xy,Yy
	__m128 m_tmp2 = _mm_unpackhi_ps( m_Xl, m_Yl); //Xz,Yz
	__m128 m_DOT = _mm_shuffle_ps( m_tmp2, m_Zl, 0b11100100); //Xz,Yz,Zz
	       m_tmp2 = _mm_movehl_ps( m_tmp2, m_tmp1); //Xy,Yy (when first parameter matches result, only one instruction is generated)
	       m_tmp1 = _mm_movelh_ps( m_tmp1, m_Zl); //Xx,Yx,Zx,Zy (same as above)
	       m_DOT = _mm_add_ps( m_DOT, m_tmp1); // Xx+Xz, Yx+Yz, Zx+Zz
	       m_tmp2 = _mm_shuffle_ps( m_tmp2, m_tmp1, 0b10110100); // Xy,Yy,Zy,Zx
	       m_DOT = _mm_add_ps( m_DOT, m_tmp2); // Xx+Xy+Xz, Yx+Yy+Yz, Zx+Zy+Zz

	_mm_storeu_ps( &Result.X, m_DOT);
	return Result;
}

//
// Apply 'pivot' transform: First rotate, then add the translation.
//
inline FVector FVector::PivotTransform(const FCoords& Coords) const
{
	return Coords.Origin + TransformVectorBy(Coords);
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
	return (V2-V1).Size();
}

//
// Squared distance between two points.
//
inline FLOAT FDistSquared( const FVector &V1, const FVector &V2 )
{
	return (V2-V1).SizeSquared();
}

//
// Euclidean distance between two points in xy plane.
//
inline FLOAT FDist2D( const FVector &V1, const FVector &V2 )
{
	return (V2-V1).Size2D();
}

//
// Squared distance between two points in xy plane.
//
inline FLOAT FDist2DSquared( const FVector &V1, const FVector &V2 )
{
	return (V2-V1).SizeSquared2D();
}

//
// Triple product of three vectors.
//
inline FLOAT FTriple( const FVector& X, const FVector& Y, const FVector& Z )
{
	return X | (Y^Z);
}
