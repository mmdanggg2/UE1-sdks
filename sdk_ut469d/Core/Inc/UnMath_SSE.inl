/*=============================================================================
	UnMath_SSE.inl: Unreal SSE math helpers.

	Revision history:
		* Created by Fernando Velazquez (Higor)

	NOTE: This file should ONLY be included by UnMath.h!
=============================================================================*/

//
// Some internal operations are best kept within SSE registers, so we'll have
// both normal and _mm versions in some cases.
//


//
// Macros and defines
//
#define MM_3D_MASK         _mm_set_epi32(0x00000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF)
#define MM_ABS_MASK        _mm_set_epi32(0x7FFFFFFF,0x7FFFFFFF,0x7FFFFFFF,0x7FFFFFFF)
#define _mm_pshufd_ps(v,i) _mm_castsi128_ps( _mm_shuffle_epi32( _mm_castps_si128(v), i))

inline __m128 _mm_newton_n1_rsqrt_ss( __m128 x )
{
	const float three = 3.0f;
	const float onehalf = 0.5f;
	__m128 z = _mm_rsqrt_ss(x); // ~= 1/sqrt(x) (estimate)
	__m128 r = _mm_sub_ss( _mm_load_ss(&three), _mm_mul_ss( _mm_mul_ss(x, z), z)); //3-xzz
	return _mm_mul_ss( _mm_load_ss(&onehalf), _mm_mul_ss(z, r) ); //(0.5)z * (3-xzz)
}

inline __m128 _mm_coords_sum_ps( __m128 x)
{
	__m128 w = _mm_shuffle_ps( x, x, 0b10110001); //x,y,z,w >> y,x,w,z
//	__m128 w = _mm_pshufd_ps( v, 0b10110001); /*SSE2 version*/
	x = _mm_add_ps( x, w); // x+y,-,z+w,-
	w = _mm_movehl_ps( w, x); // >> z+w,-,-,-
	w = _mm_add_ss( w, x); // x+y+z+w,-,-,-
	return w;
}

inline __m128 _mm_coords_sum_xy_ps( __m128 x)
{
	__m128 w = _mm_shuffle_ps( x, x, 0b10110001); //x,y,z,w >> y,x,w,z
//	__m128 w = _mm_pshufd_ps( v, 0b10110001); /*SSE2 version*/
	x = _mm_add_ss( x, w); // x+y,-,-,-
	return x;
}


// SSE instruction set provides us with RSQRTSS for a fast approximation of inverse square root
// https://software.intel.com/en-us/cpp-compiler-developer-guide-and-reference-arithmetic-intrinsics-2
//
// When applying Newton iterations the error should be as follows:
//   n=0: 0.000254273
//   n=1: 1.19209e-07

inline FLOAT appFastInvSqrtSSE( FLOAT X )
{
	FLOAT Result;
	_mm_store_ss( &Result, _mm_rsqrt_ss(_mm_load_ss(&X)) );
	return Result;
}

inline FLOAT appInvSqrtSSE( FLOAT X )
{
	FLOAT Result;
	_mm_store_ss( &Result, _mm_newton_n1_rsqrt_ss(_mm_load_ss(&X)) );
	return Result;
}


//
// Get reciprocal of a number
// NOTE: DIVSS is really good in newer gen CPU's, only use this if you really want to squeeze a couple of cycles
//
inline FLOAT appFastRcpSSE( FLOAT X )
{
	FLOAT Result;
	_mm_store_ss( &Result, _mm_rcp_ss(_mm_load_ss(&X)) );
	return Result;
}


/*-----------------------------------------------------------------------------
	Helpers.
	
	While modern x86/x86-64 compilers tend to produce better code than old assemblies
	they struggle to vectorize even the simplest constructors, operations and checks!!!
	
	Keeping operations in packed forms will eliminate stores and loads inbetween functions.
-----------------------------------------------------------------------------*/


// Load FPlane to XMM
inline __m128 _mm( const FPlane& P )
{
	return _mm_loadu_ps( &P.X );
}


// Load FVector to XMM safely (we can specify whether we want to zero the unused float)
template<UBOOL bZeroW=1>
inline __m128 _mm( const FVector& V)
{
	#if VECTOR_ALIGNMENT == 16
		__m128 mm = _mm_loadu_ps( &V.X);  //X,Y,Z,?
		if ( bZeroW )
			mm = _mm_and_ps( mm, _mm_castsi128_ps(MM_3D_MASK)); //X,Y,Z,0
	#else
		__m128 mm = _mm_setzero_ps();    //0,0,0,0 (some compilers already do this prior to _mm_load_ss)
		mm = _mm_load_ss( &V.Z );        //Z,0,0,0
		mm = _mm_movelh_ps( mm, mm);     //Z,0,Z,0
		mm = _mm_loadl_pi( mm, (const __m64*)&V.X ); //X,Y,Z,0 (we load the low 8 bytes onto mm)
	#endif
		return mm;
}


/*-----------------------------------------------------------------------------
	Test tools.
-----------------------------------------------------------------------------*/

//
// Test fast inverse Square Root
//
inline void SSE_TestInvSqrt( INT Count )
{
	if ( Count <= 0 )
		return;

	DWORD DivCycles = 0;
	DWORD InvCycles = 0;
	DWORD Inv1Cycles = 0;

	FLOAT F = 1337.f;

	clockFast(DivCycles);
	for ( INT i=0 ; i<Count ; i++ )
		F = 1.f / appSqrt(F);
	unclockFast(DivCycles);

	clockFast(InvCycles);
	for ( INT i=0 ; i<Count ; i++ )
		F = appFastInvSqrtSSE(F);
	unclockFast(InvCycles);

	clockFast(Inv1Cycles);
	for ( INT i=0 ; i<Count ; i++ )
		F = appInvSqrtSSE(F);
	unclockFast(Inv1Cycles);

	debugf( NAME_Init, TEXT("InvSqrt test cycles: Div=%i, Rcp=%i, RcpN1=%i"), DivCycles, InvCycles, Inv1Cycles);
}

//
// Simple test
//
inline void SSE_MathTest()
{
	guard(SSE_MathTest);

	FVector A( 1, 2, 3);
	FVector B( -2, -3, -4);
	FPlane XYPlane( 0, 0, 1, 0); // Horizontal plane that passes through the origin

	FVector C = (A + B) * -1;
	debugf( NAME_Init, TEXT("Expected: (1,1,1), Got %s"), *C.String());

	FLOAT AZDist = XYPlane.PlaneDot( A);
	FLOAT BZDist = XYPlane.PlaneDot( B);
	debugf( NAME_Init, TEXT("Expected [3,-4], Got [%f,%f]"), AZDist, BZDist);

	FVector Dir( 10, 0, 0);
	FVector DirNormal = Dir.SafeNormal();
	FLOAT DirSize = Dir.Size();
	debugf( NAME_Init, TEXT("Expected (1,0,0)*10, Got %s*%f"), *DirNormal.String(), DirSize);

	unguard;
}