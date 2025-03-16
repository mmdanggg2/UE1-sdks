/*=============================================================================
	UnBox_SSE_64.inl: Unreal FBox SSE inlines for 64 bit systems.

	Revision history:
		* Created by Fernando Velazquez (Higor)

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
{
	__m128 mm_min = _mm<false>(InMin);
	__m128 mm_max = _mm<false>(InMax);

	_mm_storeu_ps(&Min.X, mm_min); 
	_mm_storeu_ps(&Max.X, mm_max);

	IsValid = 1;
}


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
		__m128 mm_min = _mm_loadu_ps(&Min.X);
		__m128 mm_max = _mm_loadu_ps(&Max.X);
		__m128 mm_other = _mm<false>(Other);

		mm_min = _mm_min_ps(mm_min, mm_other);
		mm_max = _mm_max_ps(mm_max, mm_other);

		_mm_storeu_ps(&Min.X, mm_min);
		_mm_storeu_ps(&Max.X, mm_max);
	}
	else
	{
		__m128 mm_other = _mm<false>(Other);

		_mm_storeu_ps(&Min.X, mm_other);
		_mm_storeu_ps(&Max.X, mm_other);

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
		__m128 mm_min_a = _mm_loadu_ps(&Min.X);
		__m128 mm_min_b = _mm_loadu_ps(&Other.Min.X);
		__m128 mm_min   = _mm_min_ps(mm_min_a, mm_min_b);

		__m128 mm_max_a = _mm_loadu_ps(&Max.X);
		__m128 mm_max_b = _mm_loadu_ps(&Other.Max.X);
		__m128 mm_max   = _mm_max_ps(mm_max_a, mm_max_b);

		_mm_storeu_ps(&Min.X, mm_min);
		_mm_storeu_ps(&Max.X, mm_max);
	}
	else
		*this = Other;
	return *this;
}

inline FBox FBox::operator+( const FBox& Other ) const
{
	return FBox(*this) += Other;
}

static __m128 VECTORCALL _mm_transform_vector( __m128 mm_p, __m128 mm_x, __m128 mm_y, __m128 mm_z)
{
	// Instead of calculating dot products individually, we'll do it in a single
	// operation by transposing the results and then adding all 3 vectors
	mm_x = _mm_mul_ps(mm_x, mm_p); //Xx,Xy,Xz
	mm_y = _mm_mul_ps(mm_y, mm_p); //Yx,Yy,Yz
	mm_z = _mm_mul_ps(mm_z, mm_p); //Zx,Zy,Zz
	//Target:
	// Xx,Yx,Zx
	// Xy,Yy,Zy
	// Xz,Yz,Zz
	__m128 m_tmp1 = _mm_unpacklo_ps( mm_x, mm_y); //Xx,Yx,Xy,Yy
	__m128 m_tmp2 = _mm_unpackhi_ps( mm_x, mm_y); //Xz,Yz
	__m128 m_DOT = _mm_shuffle_ps( m_tmp2, mm_z, 0b11100100); //Xz,Yz,Zz
	       m_tmp2 = _mm_movehl_ps( m_tmp2, m_tmp1); //Xy,Yy (when first parameter matches result, only one instruction is generated)
	       m_tmp1 = _mm_movelh_ps( m_tmp1, mm_z); //Xx,Yx,Zx,Zy (same as above)
	       m_DOT = _mm_add_ps( m_DOT, m_tmp1); // Xx+Xz, Yx+Yz, Zx+Zz
	       m_tmp2 = _mm_shuffle_ps( m_tmp2, m_tmp1, 0b10110100); // Xy,Yy,Zy,Zx
	       m_DOT = _mm_add_ps( m_DOT, m_tmp2); // Xx+Xy+Xz, Yx+Yy+Yz, Zx+Zy+Zz
	return m_DOT;
}

inline FBox FBox::TransformBy( const FCoords& Coords ) const
{
	__m128 mm_points[8];

	// Load axes
	__m128 mm_c_org = _mm<false>(Coords.Origin);
	__m128 mm_c_x = _mm<false>(Coords.XAxis);
	__m128 mm_c_y = _mm<false>(Coords.YAxis);
	__m128 mm_c_z = _mm<false>(Coords.ZAxis);

	// Pre-emptively substract coordinate origin from Min, Max
	mm_points[0] = _mm<false>(Min) - mm_c_org; // 0x 0y 0z
	mm_points[1] = _mm<false>(Max) - mm_c_org; // 1x 1y 1z

	// And generate the remaining 6 points of the box
	mm_points[2] = _mm_shuffle_ps(mm_points[0], mm_points[1], 0b01100100); // 0x 0y 1z 1y
	mm_points[3] = _mm_shuffle_ps(mm_points[1], mm_points[0], 0b01100100); // 1x 1y 0z 0y
	mm_points[4] = _mm_pshufd_ps(mm_points[2], 0b101100); // 0x 1y 1z
	mm_points[5] = _mm_pshufd_ps(mm_points[3], 0b101100); // 1x 0y 0z
	mm_points[6] = _mm_shuffle_ps(mm_points[4], mm_points[5], 0b100100); // 0x 1y 1z
	mm_points[7] = _mm_shuffle_ps(mm_points[5], mm_points[4], 0b100100); // 1x 0y 0z

	// Transform points 0, 1 and compute Min Max
	mm_points[0] = _mm_transform_vector(mm_points[0], mm_c_x, mm_c_y, mm_c_z);
	mm_points[1] = _mm_transform_vector(mm_points[1], mm_c_x, mm_c_y, mm_c_z);
	__m128 mm_min_01 = _mm_min_ps(mm_points[0], mm_points[1]);
	__m128 mm_max_01 = _mm_max_ps(mm_points[1], mm_points[0]);

	// Transform points 2, 3 and compute Min Max
	mm_points[2] = _mm_transform_vector(mm_points[2], mm_c_x, mm_c_y, mm_c_z);
	mm_points[3] = _mm_transform_vector(mm_points[3], mm_c_x, mm_c_y, mm_c_z);
	__m128 mm_min_23 = _mm_min_ps(mm_points[2], mm_points[3]);
	__m128 mm_max_23 = _mm_max_ps(mm_points[3], mm_points[2]);

	// Compute Min Max for 0, 1, 2, 3
	__m128 mm_min_0123 = _mm_min_ps(mm_min_01, mm_min_23);
	__m128 mm_max_0123 = _mm_max_ps(mm_max_01, mm_max_23);

	// Transform points 4, 5 and compute Min Max
	mm_points[4] = _mm_transform_vector(mm_points[4], mm_c_x, mm_c_y, mm_c_z);
	mm_points[5] = _mm_transform_vector(mm_points[5], mm_c_x, mm_c_y, mm_c_z);
	__m128 mm_min_45 = _mm_min_ps(mm_points[4], mm_points[5]);
	__m128 mm_max_45 = _mm_max_ps(mm_points[5], mm_points[4]);

	// Transform points 6, 7 and compute Min Max
	mm_points[6] = _mm_transform_vector(mm_points[6], mm_c_x, mm_c_y, mm_c_z);
	mm_points[7] = _mm_transform_vector(mm_points[7], mm_c_x, mm_c_y, mm_c_z);
	__m128 mm_min_67 = _mm_min_ps(mm_points[6], mm_points[7]);
	__m128 mm_max_67 = _mm_max_ps(mm_points[7], mm_points[6]);

	// Compute Min Max for 4, 5, 6, 7
	__m128 mm_min_4567 = _mm_min_ps(mm_min_45, mm_min_67);
	__m128 mm_max_4567 = _mm_max_ps(mm_max_45, mm_max_67);

	// Compute Min Max for all points
	__m128 mm_min = _mm_min_ps(mm_min_0123, mm_min_4567);
	__m128 mm_max = _mm_max_ps(mm_max_0123, mm_max_4567);

	FBox NewBox;
	_mm_storeu_ps(&NewBox.Min.X, mm_min);
	_mm_storeu_ps(&NewBox.Max.X, mm_max);
	NewBox.IsValid = 1;
	return NewBox;
}

inline FBox FBox::ExpandBy( FLOAT W ) const
{
	__m128 mm_exp = _mm_load1_ps(&W);
	__m128 mm_min = _mm<false>(Min) - mm_exp;
	__m128 mm_max = _mm<false>(Max) + mm_exp;

	FBox NewBox;
	_mm_storeu_ps(&NewBox.Min.X, mm_min);
	_mm_storeu_ps(&NewBox.Max.X, mm_max);
	NewBox.IsValid = 1;
	return NewBox;
}


//
// Checks functions
//
inline UBOOL FBox::Contains( const FVector& Other) const
{
	if ( IsValid )
	{
		__m128 mm_other = _mm<false>(Other);
		__m128 mm_ge_min = _mm_cmpge_ps( _mm<false>(Min), mm_other);
		__m128 mm_le_max = _mm_cmple_ps( _mm<false>(Max), mm_other);
		return (_mm_movemask_ps(_mm_and_ps(mm_ge_min, mm_le_max)) & 0b111) != 0;
	}
	return false;
}

inline FVector FBox::GetCenter() const
{
	return (Min + Max) * 0.5f;
}

