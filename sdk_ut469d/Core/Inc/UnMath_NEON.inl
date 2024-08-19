/*=============================================================================
	UnMath_NEON.inl: Unreal NEON math helpers.
	Copyright 2021-2023 OldUnreal. All Rights Reserved.

	Revision history:
		* Created by Fernando Velazquez

	NOTE: This file should ONLY be included by UnMath.h!
=============================================================================*/

#include <arm_neon.h>

//
// Generates a FRSQRTE instruction for a fast approximation of inverse square root
//
inline FLOAT appFastInvSqrtNEON( FLOAT X )
{
	return vrsqrtes_f32(X);
}

// Performs a Newton iteration
inline FLOAT appInvSqrtNEON( FLOAT X )
{
	FLOAT z = vrsqrtes_f32(X);
	return 0.5f * z * (3.f - X *  z * z);
}


//
// Get reciprocal of a number
//
inline FLOAT appFastRcpNEON( FLOAT X )
{
	return vrecpes_f32(X);
}


/*-----------------------------------------------------------------------------
	Helpers.
	
	In ARMv8 it's a waste of resources to load vec3's by zeroing W
	W should simply be discarded at the end result (if used at all) 
-----------------------------------------------------------------------------*/


// Load FPlane
inline float32x4_t vld( const FPlane& P)
{
	return vld1q_f32(&P.X);
}

// Load FVector (loads W too!!)
inline float32x4_t vld( const FVector& V)
{
	return vld1q_f32(&V.X);
}

// Load FVector
inline float32x4_t vld_w0( const FVector& V)
{
	return float32x4_t{V.X, V.Y, V.Z, 0.f};
}

// Store as FPlane
inline FPlane FPlane_vst( float32x4_t v)
{
	FPlane Result;
	vst1q_f32(&Result.X, v);
	return Result;
}

// Store as FVector
inline FVector FVector_vst( float32x4_t v)
{
	FVector Result;
	vst1q_f32(&Result.X, v);
	return Result;
}


