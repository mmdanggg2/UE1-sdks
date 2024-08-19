/*=============================================================================
	UnPlane_NEON.inl: Unreal FPlane inlines.
	Copyright 2021-2023 OldUnreal. All Rights Reserved.

	Revision history:
		* Created by Fernando Velazquez

	NOTE: This file should ONLY be included by UnMath.h!
=============================================================================*/

//
// Constructors.
//
inline FPlane::FPlane()
{}

inline FPlane::FPlane( const FPlane& P )
{
	vst1q_f32(&X, vld(P));
}

inline FPlane::FPlane( const FVector& V )
{
	vst1q_f32(&X, vld_w0(V));
}

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
	return FPlane_vst(vnegq_f32(vld(*this)));
}


//
// Binary math operators.
//
inline FLOAT FPlane::operator|( const FPlane& V) const
{
	return vaddvq_f32(vld(*this) * vld(V));
}

inline FLOAT FPlane::operator|( const FVector& V) const
{
	return vaddvq_f32(vld(*this) * vld_w0(V));
}

inline FPlane FPlane::operator+( const FPlane& V ) const
{
	return FPlane_vst(vld(*this) + vld(V));
}

inline FPlane FPlane::operator-( const FPlane& V ) const
{
	return FPlane_vst(vld(*this) - vld(V));
}

inline FPlane FPlane::operator*( const FPlane& V ) const
{
	return FPlane_vst(vld(*this) * vld(V));
}

inline FPlane FPlane::operator*( FLOAT Scale ) const
{
	return FPlane_vst(vld(*this) * Scale);
}

inline FPlane FPlane::operator/( FLOAT Scale ) const
{
	return FPlane_vst(vld(*this) / Scale);
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
	uint32x4_t cmp32 = vceqq_f32(vld(*this), vld(V));
	uint16x4_t cmp16 = vmovn_u32(cmp32);
	return vget_lane_u64(vreinterpret_u64_u16(cmp16), 0) == 0xFFFFFFFFFFFFFFFFull;
}

inline UBOOL FPlane::operator!=( const FPlane& V ) const
{
	uint32x4_t cmp32 = vceqq_f32(vld(*this), vld(V));
	uint16x4_t cmp16 = vmovn_u32(cmp32);
	return vget_lane_u64(vreinterpret_u64_u16(cmp16), 0) != 0xFFFFFFFFFFFFFFFFull;
}


//
// Assignment operators.
//
inline FPlane FPlane::operator+=( const FPlane& V )
{
	*this = FPlane_vst(vld(*this) + vld(V));
	return *this;
}

inline FPlane FPlane::operator-=( const FPlane& V )
{
	*this = FPlane_vst(vld(*this) - vld(V));
	return *this;
}

inline FPlane FPlane::operator*=( const FPlane& V )
{
	*this = FPlane_vst(vld(*this) * vld(V));
	return *this;
}

inline FPlane FPlane::operator*=( FLOAT Scale )
{
	*this = FPlane_vst(vld(*this) * Scale);
	return *this;
}

inline FPlane FPlane::operator/=( FLOAT Scale )
{
	*this = FPlane_vst(vld(*this) / Scale);
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
