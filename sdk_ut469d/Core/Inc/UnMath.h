/*=============================================================================
	UnMath.h: Unreal math routines
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

/*-----------------------------------------------------------------------------
	Defintions.
-----------------------------------------------------------------------------*/

// Forward declarations.
class  FVector;
class  FPlane;
class  FCoords;
class  FRotator;
class  FScale;
class  FGlobalMath;
class  FMatrix;
class  FQuat;

// Fixed point conversion.
inline	INT Fix		(INT A)			{return A<<16;};
inline	INT Fix		(FLOAT A)		{return appRound(A*65536.f);};
inline	INT Unfix	(INT A)			{return A>>16;};

// Constants.
#undef  PI
#define PI 					(3.1415926535897932f)
#define SMALL_NUMBER		(1.e-8f)
#define KINDA_SMALL_NUMBER	(1.e-4f)

// Magic numbers for numerical precision.
#define DELTA			(0.00001f)
#define SLERP_DELTA		(0.0001f)

// SSE loadable constants
#if USES_SSE_INTRINSICS
	//Override CORE_API_SSE if we want to define constants in current library
	#ifndef CORE_API_SSE
		#define CORE_API_SSE CORE_API
	#endif

	inline FLOAT appFastInvSqrtSSE( FLOAT X );
#endif


/*-----------------------------------------------------------------------------
	Global functions.
-----------------------------------------------------------------------------*/

//
// Snap a value to the nearest grid multiple.
//
inline FLOAT FSnap( FLOAT Location, FLOAT Grid )
{
	if( Grid==0.f )	return Location;
	else			return appFloor((Location + 0.5f*Grid)/Grid)*Grid;
}

//
// Internal sheer adjusting function so it snaps nicely at 0 and 45 degrees.
//
inline FLOAT FSheerSnap (FLOAT Sheer)
{
	if		(Sheer < -0.65f) return Sheer + 0.15f;
	else if (Sheer > +0.65f) return Sheer - 0.15f;
	else if (Sheer < -0.55f) return -0.50f;
	else if (Sheer > +0.55f) return 0.50f;
	else if (Sheer < -0.05f) return Sheer + 0.05f;
	else if (Sheer > +0.05f) return Sheer - 0.05f;
	else					 return 0.f;
}

//
// Find the closest power of 2 that is >= N.
//
inline DWORD FNextPowerOfTwo( DWORD N )
{
	if (N<=0L		) return 0L;
	if (N<=1L		) return 1L;
	if (N<=2L		) return 2L;
	if (N<=4L		) return 4L;
	if (N<=8L		) return 8L;
	if (N<=16L	    ) return 16L;
	if (N<=32L	    ) return 32L;
	if (N<=64L 	    ) return 64L;
	if (N<=128L     ) return 128L;
	if (N<=256L     ) return 256L;
	if (N<=512L     ) return 512L;
	if (N<=1024L    ) return 1024L;
	if (N<=2048L    ) return 2048L;
	if (N<=4096L    ) return 4096L;
	if (N<=8192L    ) return 8192L;
	if (N<=16384L   ) return 16384L;
	if (N<=32768L   ) return 32768L;
	if (N<=65536L   ) return 65536L;
	else			  return 0;
}

//
// Add to a word angle, constraining it within a min (not to cross)
// and a max (not to cross).  Accounts for funkyness of word angles.
// Assumes that angle is initially in the desired range.
//
inline _WORD FAddAngleConfined( INT Angle, INT Delta, INT MinThresh, INT MaxThresh )
{
	if( Delta < 0 )
	{
		if( Delta<=-0x10000L || Delta<=-(INT)((_WORD)(Angle-MinThresh)))
			return (_WORD)MinThresh;
	}
	else if( Delta > 0 )
	{
		if( Delta>=0x10000L || Delta>=(INT)((_WORD)(MaxThresh-Angle)))
			return (_WORD)MaxThresh;
	}
	return (_WORD)(Angle+Delta);
}

//
// Eliminate all fractional precision from an angle.
//
INT ReduceAngle( INT Angle );

//
// Fast 32-bit float evaluations. 
// Warning: likely not portable, and useful on Pentium class processors only.
//

inline UBOOL IsSmallerPositiveFloat(float F1,float F2)
{
	return ( (*(DWORD*)&F1) < (*(DWORD*)&F2));
}

inline FLOAT MinPositiveFloat(float F1, float F2)
{
#if USES_SSE_INTRINSICS
	FLOAT Result;
	_mm_store_ss( &Result, _mm_min_ss(_mm_load_ss(&F1),_mm_load_ss(&F2)) );
	return Result;
#else
	if ( (*(DWORD*)&F1) < (*(DWORD*)&F2)) return F1; else return F2;
#endif
}

//
// Warning: 0 and -0 have different binary representations.
//

inline UBOOL EqualPositiveFloat(float F1, float F2)
{
	return ( *(DWORD*)&F1 == *(DWORD*)&F2 );
}

inline UBOOL IsNegativeFloat(float F1)
{
	return ( (*(DWORD*)&F1) >= (DWORD)0x80000000 ); // Detects sign bit.
}

inline FLOAT MaxPositiveFloat(float F1, float F2)
{
	if ( (*(DWORD*)&F1) < (*(DWORD*)&F2)) return F2; else return F1;
}

// Clamp F0 between F1 and F2, all positive assumed.
inline FLOAT ClampPositiveFloat(float F0, float F1, float F2)
{
	if      ( (*(DWORD*)&F0) < (*(DWORD*)&F1)) return F1;
	else if ( (*(DWORD*)&F0) > (*(DWORD*)&F2)) return F2;
	else return F0;
}

// Clamp any float F0 between zero and positive float Range
#define ClipFloatFromZero(F0,Range)\
{\
	if ( (*(DWORD*)&F0) >= (DWORD)0x80000000) F0 = 0.f;\
	else if	( (*(DWORD*)&F0) > (*(DWORD*)&Range)) F0 = Range;\
}

/*------------------------------------------------------------------------------------
	Approximate math functions.
------------------------------------------------------------------------------------*/

//
// Famous Fast inverse square root.
//
// A bit on the history of it:
// https://www.beyond3d.com/content/articles/8/ (tldr author unknown)
//
// A bit more detailed view, constant derivation and error bounds are
// discussed here:
// https://www.lomont.org/Math/Papers/2003/InvSqrt.pdf
// 
// Nomenclature: Nn, where n is the number of newton iterations.
//
// max rel. err.
//   n=0: (unknown) Large.
//   n=1: 0.175124  Rather large for resonable work, but should be enough for rejection work.
//   n=2: 0.000465  Quite good.
//   n=3: (unknown) Should be enough for almost everything.
//
// So use n=2 as a about safe default.

#if USES_SSE_INTRINSICS
	//See UnMath_SSE.inl
	#define appFastInvSqrt(x) appFastInvSqrtSSE(x)
#else
	#define appFastInvSqrt(x) appFastInvSqrtN2(x)
#endif
//

inline FLOAT appFastInvSqrtN0(FLOAT X)
{
	INT I = *(INT*)&X;

	// Trick.
	I = 0x5f375a86 - (I >> 1);
	X = *(FLOAT*)&I;

	// No Newton iteration.
	return X;
}
inline FLOAT appFastInvSqrtN1(FLOAT X)
{
	FLOAT H = 0.5f * X; INT I = *(INT*)&X;

	// Trick.
	I = 0x5f375a86 - (I >> 1);
	X = *(FLOAT*)&I;

	// Single Newton iteration.
	X = X * (1.5f - H * X * X);
	return X;
}
inline FLOAT appFastInvSqrtN2(FLOAT X)
{
	FLOAT H = 0.5f * X; INT I = *(INT*)&X;

	// Trick.
	I = 0x5f375a86 - (I >> 1);
	X = *(FLOAT*)&I;

	// Two Newton iterations.
	X = X * (1.5f - H * X * X);
	X = X * (1.5f - H * X * X);
	return X;
}
inline FLOAT appFastInvSqrtN3(FLOAT X)
{
	FLOAT H = 0.5f * X; INT I = *(INT*)&X;

	// Trick.
	I = 0x5f375a86 - (I >> 1);
	X = *(FLOAT*)&I;

	// Three Newton iterations.
	X = X * (1.5f - H * X * X);
	X = X * (1.5f - H * X * X);
	X = X * (1.5f - H * X * X);
	return X;
}

// Conveniance.
inline FLOAT appFastSqrt(FLOAT X) { return 1.f / appFastInvSqrt(X); }
inline FLOAT appFastSqrtN0(FLOAT X) { return 1.f / appFastInvSqrtN0(X); }
inline FLOAT appFastSqrtN1(FLOAT X) { return 1.f / appFastInvSqrtN1(X); }
inline FLOAT appFastSqrtN2(FLOAT X) { return 1.f / appFastInvSqrtN2(X); }
inline FLOAT appFastSqrtN3(FLOAT X) { return 1.f / appFastInvSqrtN3(X); }

//
// Taylor series expansion of arccos.
//
#define ATP_PI (3.1415926535897932f)
inline FLOAT appAcosTaylorP1(FLOAT X)
{
	return (ATP_PI / 2.f) - X;
}
inline FLOAT appAcosTaylorP3(FLOAT X)
{
	//return (PI/2)-X-(1.f/6.f)*X*X*X;
	return (ATP_PI / 2.f) - X * (1.f + (1.f / 6.f) * X * X);
}
inline FLOAT appAcosTaylorP5(FLOAT X)
{
	//return (PI/2)-X-(1.f/6.f)*X*X*X-(3.f/40.f)*X*X*X*X*X;
	return (ATP_PI / 2.f) - X * (1.f + X * X * ((1.f / 6.f) + X * X * (3.f / 40.f)));
}
inline FLOAT appAcosTaylorP7(FLOAT X)
{
	//return (PI/2)-X-(1.f/6.f)*X*X*X-(3.f/40.f)*X*X*X*X*X-(5.f/112.f)*X*X*X*X*X*X*X;
	return (ATP_PI / 2.f) - X * (1.f + X * X * ((1.f / 6.f) + X * X * ((3.f / 40.f) + X * X * (5.f / 112.f))));
}
inline FLOAT appAcosTaylorP9(FLOAT X)
{
	return (ATP_PI / 2.f) - X - (1.f / 6.f) * X * X * X - (3.f / 40.f) * X * X * X * X * X - (5.f / 112.f) * X * X * X * X * X * X * X - (35.f / 1152.f) * X * X * X * X * X * X * X * X * X;
}

/*-----------------------------------------------------------------------------
	FVector.
-----------------------------------------------------------------------------*/

// Information associated with a floating point vector, describing its
// status as a point in a rendering context.
enum EVectorFlags
{
	FVF_OutXMin		= 0x04,	// Outcode rejection, off left hand side of screen.
	FVF_OutXMax		= 0x08,	// Outcode rejection, off right hand side of screen.
	FVF_OutYMin		= 0x10,	// Outcode rejection, off top of screen.
	FVF_OutYMax		= 0x20,	// Outcode rejection, off bottom of screen.
	FVF_OutNear     = 0x40, // Near clipping plane.
	FVF_OutFar      = 0x80, // Far clipping plane.
	FVF_OutReject   = (FVF_OutXMin | FVF_OutXMax | FVF_OutYMin | FVF_OutYMax), // Outcode rejectable.
	FVF_OutSkip		= (FVF_OutXMin | FVF_OutXMax | FVF_OutYMin | FVF_OutYMax), // Outcode clippable.
};

inline FString appVectorFlagsString(const BYTE VectorFlags)
{
	guard(appVectorFlagsString);

	BYTE Flags = VectorFlags;
	FString Result;

	if ((Flags & FVF_OutReject) == FVF_OutReject)
	{
		Flags &= ~FVF_OutReject;

		Result += TEXT("OutReject");
	}

	if (Flags & FVF_OutXMin) { Flags &= ~FVF_OutXMin; if (Result.Len()) Result += TEXT("| "); Result += TEXT("OutXMin"); }
	if (Flags & FVF_OutXMax) { Flags &= ~FVF_OutXMax; if (Result.Len()) Result += TEXT("| "); Result += TEXT("OutXMax"); }
	if (Flags & FVF_OutYMin) { Flags &= ~FVF_OutYMin; if (Result.Len()) Result += TEXT("| "); Result += TEXT("OutYMin"); }
	if (Flags & FVF_OutYMax) { Flags &= ~FVF_OutYMax; if (Result.Len()) Result += TEXT("| "); Result += TEXT("OutYMax"); }
	if (Flags & FVF_OutNear) { Flags &= ~FVF_OutNear; if (Result.Len()) Result += TEXT("| "); Result += TEXT("OutNear"); }
	if (Flags & FVF_OutFar) { Flags &= ~FVF_OutFar;  if (Result.Len()) Result += TEXT("| "); Result += TEXT("OutFar"); }

	if (Flags)
	{
		if (Result.Len())
			Result += TEXT("| ");

		Result += FString::Printf(TEXT("0x%02X"), Flags);
	}
	return Result;
	unguard;
}

//
// Floating point vector.
// 64 bit vectors are 16 bytes.
//
class alignas(VECTOR_ALIGNMENT) CORE_API FVector 
{
public:
	// Variables.
	FLOAT X,Y,Z;

	// Constructors.
	FVector();
	FVector( FLOAT InX, FLOAT InY, FLOAT InZ );
#if !MOD_BUILD
	FVector( const FVector& V);
#endif

	// Binary math operators.
	friend FVector operator*( FLOAT Scale, const FVector& V );
	FVector operator^( const FVector& V ) const;
	FLOAT operator|( const FVector& V ) const;
	
	FVector operator+( const FVector& V ) const;
	FVector operator-( const FVector& V ) const;
	FVector operator*( FLOAT Scale ) const;
	FVector operator/( FLOAT Scale ) const;
	FVector operator*( const FVector& V ) const;

	// Binary comparison operators.
	UBOOL operator==( const FVector& V ) const;
	UBOOL operator!=( const FVector& V ) const;

	// Unary operators.
	FVector operator-() const;

	// Assignment operators.
	FVector& operator=( const FVector& V);
	FVector operator+=( const FVector& V );
	FVector operator-=( const FVector& V );
	FVector operator*=( FLOAT Scale );
	FVector operator/=( FLOAT V );
	FVector operator*=( const FVector& V );
	FVector operator/=( const FVector& V );

	// Simple functions.
	FLOAT Size() const;
	FLOAT SizeSquared() const;
	FLOAT Size2D() const ;
	FLOAT SizeSquared2D() const;
	int IsNearlyZero() const; //UBOOL?
	UBOOL IsZero() const;
	UBOOL IsNan() const;
	UBOOL Normalize();
	UBOOL NormalizeSlow();
	FVector Projection() const;
	FVector UnsafeNormal() const;
	FVector GridSnap( const FVector& Grid );
	FVector BoundToCube( FLOAT Radius );
	void AddBounded( const FVector& V, FLOAT Radius=MAXSWORD );
	FLOAT& Component( INT Index );
	const FLOAT& Component( INT Index ) const;

	FLOAT SizeFast() const;
	FLOAT SizeFastN0() const;
	FLOAT SizeFastN1() const;
	FLOAT SizeFastN2() const;
	FLOAT SizeFastN3() const;
	FLOAT Size2DFast() const;
	FLOAT Size2DFastN0() const;
	FLOAT Size2DFastN1() const;
	FLOAT Size2DFastN2() const;
	FLOAT Size2DFastN3() const;

	// Return a boolean that is based on the vector's direction.
	// When      V==(0,0,0) Booleanize(0)=1.
	// Otherwise Booleanize(V) <-> !Booleanize(!B).
	UBOOL Booleanize();

	// Transformation.
	FVector TransformVectorBy( const FCoords& Coords ) const;
	FVector TransformPointBy( const FCoords& Coords ) const;
	FVector MirrorByVector( const FVector& MirrorNormal ) const;
	FVector MirrorByPlane( const FPlane& MirrorPlane ) const;
	FVector PivotTransform(const FCoords& Coords) const;

	// Complicated functions.
	FRotator Rotation();
	void FindBestAxisVectors( FVector& Axis1, FVector& Axis2 );
	FVector SafeNormal() const;
	FVector SafeNormalSlow() const;

	// Friends.
	friend FLOAT FDist( const FVector& V1, const FVector& V2 );
	friend FLOAT FDistSquared( const FVector& V1, const FVector& V2 );
	friend FLOAT FDist2D( const FVector& V1, const FVector& V2);
	friend FLOAT FDist2DSquared( const FVector& V1, const FVector& V2);
	friend UBOOL FPointsAreSame( const FVector& P, const FVector& Q );
	friend UBOOL FPointsAreNear( const FVector& Point1, const FVector& Point2, FLOAT Dist);
	friend FLOAT FPointPlaneDist( const FVector& Point, const FVector& PlaneBase, const FVector& PlaneNormal );
	friend FVector FLinePlaneIntersection( const FVector& Point1, const FVector& Point2, const FVector& PlaneOrigin, const FVector& PlaneNormal );
	friend FVector FLinePlaneIntersection( const FVector& Point1, const FVector& Point2, const FPlane& Plane );
	friend UBOOL FParallel( const FVector& Normal1, const FVector& Normal2 );
	friend UBOOL FCoplanar( const FVector& Base1, const FVector& Normal1, const FVector& Base2, const FVector& Normal2 );

	// Serializer.
	friend FArchive& operator<<( FArchive& Ar, FVector& V )
	{
		return Ar << V.X << V.Y << V.Z;
	}

	// Returns a string description.
	FString String() const
	{
		guardSlow(FVector::String);
		return FString::Printf(TEXT("(X=%f,Y=%f,Z=%f)"), X, Y, Z);
		unguardSlow;
	}

	FString StringRaw() const
	{
		guardSlow(FVector::String);
		return FString::Printf(TEXT("(X=%f,Y=%f,Z=%f) - (0x%08x,0x%08x,0x%08x)"), X, Y, Z, *reinterpret_cast<const DWORD*>(&X), *reinterpret_cast<const DWORD*>(&Y), *reinterpret_cast<const DWORD*>(&Z));
		unguardSlow;
	}
};

// Used by the multiple vertex editing function to keep track of selected vertices.
class ABrush;
class CORE_API FVertexHit
{
public:
	// Variables.
	ABrush* pBrush;
	INT PolyIndex;
	INT VertexIndex;

	// Constructors.
	FVertexHit()
	{
		pBrush = NULL;
		PolyIndex = VertexIndex = 0;
	}
	FVertexHit( ABrush* _pBrush, INT _PolyIndex, INT _VertexIndex )
	{
		pBrush = _pBrush;
		PolyIndex = _PolyIndex;
		VertexIndex = _VertexIndex;
	}

	// Functions.
	UBOOL operator==( const FVertexHit& V ) const
	{
		return pBrush==V.pBrush && PolyIndex==V.PolyIndex && VertexIndex==V.VertexIndex;
	}
	UBOOL operator!=( const FVertexHit& V ) const
	{
		return pBrush!=V.pBrush || PolyIndex!=V.PolyIndex || VertexIndex!=V.VertexIndex;
	}
};

class FPoly;
class CORE_API FFaceDragHit
{
public:
	FFaceDragHit( ABrush* InBrush, FPoly* InPoly )
	{
		Brush = InBrush;
		Poly = InPoly;
	}

	ABrush* Brush;
	FPoly* Poly;
};

/*-----------------------------------------------------------------------------
	FPlane.
-----------------------------------------------------------------------------*/

class alignas(VECTOR_ALIGNMENT) CORE_API FPlane
{
public:
	// Variables.
	FLOAT X,Y,Z,W;

	// Constructors.
	FPlane();
	FPlane( const FPlane& P );
	FPlane( const FVector& V );
	FPlane( FLOAT InX, FLOAT InY, FLOAT InZ, FLOAT InW );
	FPlane( FVector InNormal, FLOAT InW );
	FPlane( FVector InBase, const FVector &InNormal );
	FPlane( FVector A, FVector B, FVector C );

	// FVector conversion- automatic conversion is const only.
	operator const FVector&() const
	{
		return *(FVector*)this;
	}

	// Functions.
	FLOAT PlaneDot( const FVector &P ) const;
	FPlane Flip() const;
	FPlane TransformPlaneByOrtho( const FCoords &Coords ) const;

	FLOAT SizeSquared() const
	{
		return X * X + Y * Y + Z * Z;
	}

	// Binary math operators.
	FLOAT operator|( const FPlane& V ) const;
	FLOAT operator|( const FVector& V ) const;
	FPlane operator+( const FPlane& V ) const;
	FPlane operator-( const FPlane& V ) const;
	FPlane operator*( const FPlane& V ) const;
	FPlane operator*( FLOAT Scale ) const;
	FPlane operator/( FLOAT Scale ) const;
	FPlane operator+( const FVector& V ) const;
	FPlane operator-( const FVector& V ) const;
	FPlane operator*( const FVector& V ) const;

	// Binary comparison operators
	UBOOL operator==( const FPlane& V ) const;
	UBOOL operator!=( const FPlane& V ) const;

	// Assignment operators.
	FPlane operator+=( const FPlane& V );
	FPlane operator-=( const FPlane& V );
	FPlane operator*=( const FPlane& V );
	FPlane operator*=( FLOAT Scale );
	FPlane operator/=( FLOAT Scale );
	FPlane operator+=( const FVector& V );
	FPlane operator-=( const FVector& V );
	FPlane operator*=( const FVector& V );

	// Serializer.
	friend FArchive& operator<<( FArchive& Ar, FPlane &P )
	{
		return Ar << (FVector&)P << P.W;
	}

	// Returns a string description.
	FString String() const
	{
		guardSlow(FPlane::String);
		return FString::Printf(TEXT("(X=%f,Y=%f,Z=%f,W=%f)"), X, Y, Z, W);
		unguardSlow;
	}

	FString StringRaw() const
	{
		guardSlow(FPlane::String);
		return FString::Printf(TEXT("(X=%f,Y=%f,Z=%f,W=%f) - (0x%08x,0x%08x,0x%08x,0x%08x)"), X, Y, Z, W, *reinterpret_cast<const DWORD*>(&X), *reinterpret_cast<const DWORD*>(&Y), *reinterpret_cast<const DWORD*>(&Z), *reinterpret_cast<const DWORD*>(&W));
		unguardSlow;
	}
};


/*-----------------------------------------------------------------------------
	FSphere.
-----------------------------------------------------------------------------*/

class CORE_API FSphere : public FPlane
{
public:
	// Constructors.
	FSphere()
	{}
	FSphere( INT )
	:	FPlane(0,0,0,0)
	{}
	FSphere( FVector V, FLOAT W )
	:	FPlane( V, W )
	{}
	FSphere( const FVector* Pts, INT Count );
	friend FArchive& operator<<( FArchive& Ar, FSphere& S )
	{
		guardSlow(FSphere<<);
		if( Ar.Ver()<=61 )//oldver
			Ar << (FVector&)S;
		else
			Ar << (FPlane&)S;
		return Ar;
		unguardSlow
	}
};

/*-----------------------------------------------------------------------------
	FScale.
-----------------------------------------------------------------------------*/

// An axis along which sheering is performed.
enum ESheerAxis
{
	SHEER_None = 0,
	SHEER_XY   = 1,
	SHEER_XZ   = 2,
	SHEER_YX   = 3,
	SHEER_YZ   = 4,
	SHEER_ZX   = 5,
	SHEER_ZY   = 6
};

inline FString appSheerAxisString(const BYTE SheerAxis)
{
	guard(appSheerAxisString);
	switch (SheerAxis)
	{
		case SHEER_None: return TEXT("None");
		case SHEER_XY:   return TEXT("XY");
		case SHEER_XZ:   return TEXT("XZ");
		case SHEER_YX:   return TEXT("YX");
		case SHEER_YZ:   return TEXT("YZ");
		case SHEER_ZX:   return TEXT("ZX");
		case SHEER_ZY:   return TEXT("ZY");

			// MAX.
		//case SHEER_MAX:  return TEXT("MAX");

			// Unknown.
		default:         return FString::Printf(TEXT("0x%02X"), SheerAxis);
	}
	unguard;
}

//
// Scaling and sheering info associated with a brush.  This is 
// easily-manipulated information which is built into a transformation
// matrix later.
//
class CORE_API FScale 
{
public:
	// Variables.
	FVector		Scale;
	FLOAT		SheerRate;
	BYTE		SheerAxis; // From ESheerAxis

	// Serializer.
	friend FArchive& operator<<( FArchive& Ar, FScale &S )
	{
		return Ar << S.Scale << S.SheerRate << S.SheerAxis;
	}

	// Constructors.
	FScale() {}
	FScale( const FVector &InScale, FLOAT InSheerRate, ESheerAxis InSheerAxis )
	:	Scale(InScale), SheerRate(InSheerRate), SheerAxis(InSheerAxis) {}
#if !MOD_BUILD
	FScale(const FVector& InScale)
		: Scale(InScale), SheerRate(0.f), SheerAxis(SHEER_None) {}
#endif

	// Operators.
	UBOOL operator==( const FScale &S ) const
	{
		return Scale==S.Scale && SheerRate==S.SheerRate && SheerAxis==S.SheerAxis;
	}

	// Functions.
	FLOAT Orientation()
	{
		return Sgn(Scale.X * Scale.Y * Scale.Z);
	}

	// Returns a string description.
	FString String() const
	{
		guardSlow(FScale::String);
		return FString::Printf(TEXT("(Scale=%s,SheerRate=%f,SheerAxis=%s)"), *Scale.String(), SheerRate, *appSheerAxisString(SheerAxis));
		unguardSlow;
	}
};

/*-----------------------------------------------------------------------------
	FCoords.
-----------------------------------------------------------------------------*/

//
// A coordinate system matrix.
//
class alignas(VECTOR_ALIGNMENT) CORE_API FCoords
{
public:
	FVector	Origin;
	FVector	XAxis;
	FVector YAxis;
	FVector ZAxis;

	// Constructors.
	FCoords() {}
	FCoords( const FVector &InOrigin )
	:	Origin(InOrigin), XAxis(1,0,0), YAxis(0,1,0), ZAxis(0,0,1) {}
	FCoords( const FVector &InOrigin, const FVector &InX, const FVector &InY, const FVector &InZ )
	:	Origin(InOrigin), XAxis(InX), YAxis(InY), ZAxis(InZ) {}

	// Functions.
	FCoords MirrorByVector( const FVector& MirrorNormal ) const;
	FCoords MirrorByPlane( const FPlane& MirrorPlane ) const;
	FCoords	Transpose() const;
	FCoords Inverse() const;
	FCoords PivotInverse() const;
	FCoords ApplyPivot(const FCoords& CoordsB) const;
	FRotator OrthoRotation() const;

	// Operators.
	FCoords& operator*=	(const FCoords   &TransformCoords);
	FCoords	 operator*	(const FCoords   &TransformCoords) const;
	FCoords& operator*=	(const FVector   &Point);
	FCoords  operator*	(const FVector   &Point) const;
	FCoords& operator*=	(const FRotator  &Rot);
	FCoords  operator*	(const FRotator  &Rot) const;
	FCoords& operator*=	(const FScale    &Scale);
	FCoords  operator*	(const FScale    &Scale) const;
	FCoords& operator/=	(const FVector   &Point);
	FCoords  operator/	(const FVector   &Point) const;
	FCoords& operator/=	(const FRotator  &Rot);
	FCoords  operator/	(const FRotator  &Rot) const;
	FCoords& operator/=	(const FScale    &Scale);
	FCoords  operator/	(const FScale    &Scale) const;

	// Serializer.
	friend FArchive& operator<<( FArchive& Ar, FCoords& F )
	{
		return Ar << F.Origin << F.XAxis << F.YAxis << F.ZAxis;
	}

	// Returns a string description.
	FString String() const
	{
		guardSlow(FCoords::String);
		return FString::Printf(TEXT("(Origin=%s,XAxis=%s,YAxis=%s,ZAxis=%s)"), *Origin.String(), *XAxis.String(), *YAxis.String(), *ZAxis.String());
		unguardSlow;
	}
};

/*-----------------------------------------------------------------------------
	FModelCoords.
-----------------------------------------------------------------------------*/

//
// A model coordinate system, describing both the covariant and contravariant
// transformation matrices to transform points and normals by.
//
class CORE_API FModelCoords
{
public:
	// Variables.
	FCoords PointXform;		// Coordinates to transform points by  (covariant).
	FCoords VectorXform;	// Coordinates to transform normals by (contravariant).

	// Constructors.
	FModelCoords()
	{}
	FModelCoords( const FCoords& InCovariant, const FCoords& InContravariant )
	:	PointXform(InCovariant), VectorXform(InContravariant)
	{}

	// Functions.
	FModelCoords Inverse()
	{
		return FModelCoords( VectorXform.Transpose(), PointXform.Transpose() );
	}

	// Returns a string description.
	FString String() const
	{
		guardSlow(FModelCoords::String);
		return FString::Printf(TEXT("(PointXform=%s,VectorXform=%s)"), *PointXform.String(), *VectorXform.String());
		unguardSlow;
	}
};

/*-----------------------------------------------------------------------------
	FRotator.
-----------------------------------------------------------------------------*/

//
// Rotation.
//
class CORE_API FRotator
{
public:
	// Variables.
	INT Pitch; // Looking up and down (0=Straight Ahead, +Up, -Down).
	INT Yaw;   // Rotating around (running in circles), 0=East, +North, -South.
	INT Roll;  // Rotation about axis of screen, 0=Straight, +Clockwise, -CCW.

	// Serializer.
	friend FArchive& operator<<( FArchive& Ar, FRotator& R )
	{
		return Ar << R.Pitch << R.Yaw << R.Roll;
	}

	// Constructors.
	FRotator() {}
	FRotator( INT InPitch, INT InYaw, INT InRoll )
	:	Pitch(InPitch), Yaw(InYaw), Roll(InRoll) {}

	// Binary arithmetic operators.
	FRotator operator+( const FRotator &R ) const
	{
		return FRotator( Pitch+R.Pitch, Yaw+R.Yaw, Roll+R.Roll );
	}
	FRotator operator-( const FRotator &R ) const
	{
		return FRotator( Pitch-R.Pitch, Yaw-R.Yaw, Roll-R.Roll );
	}
	FRotator operator*( FLOAT Scale ) const
	{
		return FRotator( appRound(Pitch*Scale), appRound(Yaw*Scale), appRound(Roll*Scale) );
	}
	friend FRotator operator*( FLOAT Scale, const FRotator &R )
	{
		return FRotator( appRound(R.Pitch*Scale), appRound(R.Yaw*Scale), appRound(R.Roll*Scale) );
	}
	FRotator operator*= (FLOAT Scale)
	{
		Pitch = appRound(Pitch*Scale); Yaw = appRound(Yaw*Scale); Roll = appRound(Roll*Scale);
		return *this;
	}
	// Binary comparison operators.
	UBOOL operator==( const FRotator &R ) const
	{
		return Pitch==R.Pitch && Yaw==R.Yaw && Roll==R.Roll;
	}
	UBOOL operator!=( const FRotator &V ) const
	{
		return Pitch!=V.Pitch || Yaw!=V.Yaw || Roll!=V.Roll;
	}
	// Assignment operators.
	FRotator operator+=( const FRotator &R )
	{
		Pitch += R.Pitch; Yaw += R.Yaw; Roll += R.Roll;
		return *this;
	}
	FRotator operator-=( const FRotator &R )
	{
		Pitch -= R.Pitch; Yaw -= R.Yaw; Roll -= R.Roll;
		return *this;
	}
	// Functions.
	FRotator Reduce() const
	{
		return FRotator( ReduceAngle(Pitch), ReduceAngle(Yaw), ReduceAngle(Roll) );
	}
	int IsZero() const
	{
		return ((Pitch&65535)==0) && ((Yaw&65535)==0) && ((Roll&65535)==0);
	}
	FRotator Add( INT DeltaPitch, INT DeltaYaw, INT DeltaRoll )
	{
		Yaw   += DeltaYaw;
		Pitch += DeltaPitch;
		Roll  += DeltaRoll;
		return *this;
	}
	FRotator AddBounded( INT DeltaPitch, INT DeltaYaw, INT DeltaRoll )
	{
		Yaw  += DeltaYaw;
		Pitch = FAddAngleConfined(Pitch,DeltaPitch,192*0x100,64*0x100);
		Roll  = FAddAngleConfined(Roll, DeltaRoll, 192*0x100,64*0x100);
		return *this;
	}
	FRotator GridSnap( const FRotator &RotGrid )
	{
		return FRotator
		(
			appRound(FSnap(Pitch,RotGrid.Pitch)),
			appRound(FSnap(Yaw,  RotGrid.Yaw)),
			appRound(FSnap(Roll, RotGrid.Roll))
		);
	}
	FVector Vector();

	// Returns a string description.
	FString String() const
	{
		guardSlow(FRotator::String);
		return FString::Printf(TEXT("(Pitch=%i,Yaw=%i,Roll=%i)"), Pitch, Yaw, Roll);
		unguardSlow;
	}
};

/*-----------------------------------------------------------------------------
	Bounds.
-----------------------------------------------------------------------------*/

//
// A rectangular minimum bounding volume.
//
class CORE_API FBox
{
public:
	// Variables.
	FVector Min;
	FVector Max;
	BYTE IsValid;

	// Constructors.
	FBox() {}
	FBox(INT) : Min(0,0,0), Max(0,0,0), IsValid(0) {}
	FBox( const FVector& InMin, const FVector& InMax ) : Min(InMin), Max(InMax), IsValid(1) {}
	FBox( const FVector* Points, INT Count );

	// Accessors.
	FVector& GetExtrema( int i )
	{
		return (&Min)[i];
	}
	const FVector& GetExtrema( int i ) const
	{
		return (&Min)[i];
	}

	// Functions.
	FBox& operator+=( const FVector &Other )
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
	FBox operator+( const FVector& Other ) const
	{
		return FBox(*this) += Other;
	}
	FBox& operator+=( const FBox& Other )
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
		else *this = Other;
		return *this;
	}
	FBox operator+( const FBox& Other ) const
	{
		return FBox(*this) += Other;
	}
	FBox TransformBy( const FCoords& Coords ) const
	{
		FBox NewBox(0);
		for( int i=0; i<2; i++ )
			for( int j=0; j<2; j++ )
				for( int k=0; k<2; k++ )
					NewBox += FVector( GetExtrema(i).X, GetExtrema(j).Y, GetExtrema(k).Z ).TransformPointBy( Coords );
		return NewBox;
	}
	FBox ExpandBy( FLOAT W ) const
	{
		return FBox( Min - FVector(W,W,W), Max + FVector(W,W,W) );
	}
	UBOOL Contains( const FVector& Other) const
	{
		return IsValid 
			&& Other.X >= Min.X && Other.X <= Max.X
			&& Other.Y >= Min.Y && Other.Y <= Max.Y
			&& Other.Z >= Min.Z && Other.Z <= Max.Z;
	}

	// Serializer.
	friend FArchive& operator<<( FArchive& Ar, FBox& Bound )
	{
		return Ar << Bound.Min << Bound.Max << Bound.IsValid;
	}

	// Returns a string description.
	FString String() const
	{
		guardSlow(FBox::String);
		return FString::Printf(TEXT("(Min=%s,Max=%s,IsValid=%i)"), *Min.String(), *Max.String(), IsValid);
		unguardSlow;
	}
};

/*-----------------------------------------------------------------------------
	FGlobalMath.
-----------------------------------------------------------------------------*/

//
// Global mathematics info.
//
class CORE_API FGlobalMath
{
public:
	// Constants.
	enum {ANGLE_SHIFT 	= 2};		// Bits to right-shift to get lookup value.
	enum {ANGLE_BITS	= 14};		// Number of valid bits in angles.
	enum {NUM_ANGLES 	= 16384}; 	// Number of angles that are in lookup table.
	enum {NUM_SQRTS		= 16384};	// Number of square roots in lookup table.
	enum {ANGLE_MASK    =  (((1<<ANGLE_BITS)-1)<<(16-ANGLE_BITS))};

	// Class constants.
	const FVector  	WorldMin;
	const FVector  	WorldMax;
	const FCoords  	UnitCoords;
	const FScale   	UnitScale;
	const FCoords	ViewCoords;

	// Basic math functions.
	FLOAT Sqrt( int i )
	{
		return SqrtFLOAT[i]; 
	}
	FLOAT SinTab( int i )
	{
		return TrigFLOAT[((i>>ANGLE_SHIFT)&(NUM_ANGLES-1))];
	}
	FLOAT CosTab( int i )
	{
		return TrigFLOAT[(((i+16384)>>ANGLE_SHIFT)&(NUM_ANGLES-1))];
	}
	FLOAT SinFloat( FLOAT F )
	{
		return SinTab(appRound((F*65536.f)/(2.f*PI)));
	}
	FLOAT CosFloat( FLOAT F )
	{
		return CosTab(appRound((F*65536.f)/(2.f*PI)));
	}

	// Constructor.
	FGlobalMath();

private:
	// Tables.
	FLOAT  TrigFLOAT		[NUM_ANGLES];
	FLOAT  SqrtFLOAT		[NUM_SQRTS];
	FLOAT  LightSqrtFLOAT	[NUM_SQRTS];
};

inline INT ReduceAngle( INT Angle )
{
	return Angle & FGlobalMath::ANGLE_MASK;
};

/*-----------------------------------------------------------------------------
	Floating point constants.
-----------------------------------------------------------------------------*/

//
// Lengths of normalized vectors (These are half their maximum values
// to assure that dot products with normalized vectors don't overflow).
//
#define FLOAT_NORMAL_THRESH				(0.0001f)

//
// Magic numbers for numerical precision.
//
#define THRESH_POINT_ON_PLANE			(0.10f)		/* Thickness of plane for front/back/inside test */
#define THRESH_POINT_ON_SIDE			(0.20f)		/* Thickness of polygon side's side-plane for point-inside/outside/on side test */
#define THRESH_POINTS_ARE_SAME			(0.002f)	/* Two points are same if within this distance */
#define THRESH_POINTS_ARE_NEAR			(0.015f)	/* Two points are near if within this distance and can be combined if imprecise math is ok */
#define THRESH_NORMALS_ARE_SAME			(0.00002f)	/* Two normal points are same if within this distance */
													/* Making this too large results in incorrect CSG classification and disaster */
#define THRESH_VECTORS_ARE_NEAR			(0.0004f)	/* Two vectors are near if within this distance and can be combined if imprecise math is ok */
													/* Making this too large results in lighting problems due to inaccurate texture coordinates */
#define THRESH_SPLIT_POLY_WITH_PLANE	(0.25f)		/* A plane splits a polygon in half */
#define THRESH_SPLIT_POLY_PRECISELY		(0.01f)		/* A plane exactly splits a polygon */
#define THRESH_ZERO_NORM_SQUARED		(0.0001f)	/* Size of a unit normal that is considered "zero", squared */
#define THRESH_VECTORS_ARE_PARALLEL		(0.02f)		/* Vectors are parallel if dot product varies less than this */


/*-----------------------------------------------------------------------------
	FVector friends.
-----------------------------------------------------------------------------*/

//
// Compare two points and see if they're the same, using a threshold.
// Returns 1=yes, 0=no.  Uses fast distance approximation.
//
inline int FPointsAreSame( const FVector &P, const FVector &Q )
{
	FLOAT Temp;
	Temp=P.X-Q.X;
	if( (Temp > -THRESH_POINTS_ARE_SAME) && (Temp < THRESH_POINTS_ARE_SAME) )
	{
		Temp=P.Y-Q.Y;
		if( (Temp > -THRESH_POINTS_ARE_SAME) && (Temp < THRESH_POINTS_ARE_SAME) )
		{
			Temp=P.Z-Q.Z;
			if( (Temp > -THRESH_POINTS_ARE_SAME) && (Temp < THRESH_POINTS_ARE_SAME) )
			{
				return 1;
			}
		}
	}
	return 0;
}

//
// Compare two points and see if they're the same, using a threshold.
// Returns 1=yes, 0=no.  Uses fast distance approximation.
//
inline int FPointsAreNear( const FVector &Point1, const FVector &Point2, FLOAT Dist )
{
	FLOAT Temp;
	Temp=(Point1.X - Point2.X); if (Abs(Temp)>=Dist) return 0;
	Temp=(Point1.Y - Point2.Y); if (Abs(Temp)>=Dist) return 0;
	Temp=(Point1.Z - Point2.Z); if (Abs(Temp)>=Dist) return 0;
	return 1;
}

//
// Calculate the signed distance (in the direction of the normal) between
// a point and a plane.
//
inline FLOAT FPointPlaneDist
(
	const FVector &Point,
	const FVector &PlaneBase,
	const FVector &PlaneNormal
)
{
	return (Point - PlaneBase) | PlaneNormal;
}

//
// See if two normal vectors (or plane normals) are nearly parallel.
//
inline int FParallel( const FVector &Normal1, const FVector &Normal2 )
{
	FLOAT NormalDot = Normal1 | Normal2;
	return (Abs (NormalDot - 1.f) <= THRESH_VECTORS_ARE_PARALLEL);
}

//
// See if two planes are coplanar.
//
inline int FCoplanar( const FVector &Base1, const FVector &Normal1, const FVector &Base2, const FVector &Normal2 )
{
	if      (!FParallel(Normal1,Normal2)) return 0;
	else if (FPointPlaneDist (Base2,Base1,Normal1) > THRESH_POINT_ON_PLANE) return 0;
	else    return 1;
}


/*-----------------------------------------------------------------------------
	FPlane implementation.
-----------------------------------------------------------------------------*/

//
// Transform a point by a coordinate system, moving
// it by the coordinate system's origin if nonzero.
//
inline FPlane FPlane::TransformPlaneByOrtho( const FCoords &Coords ) const
{
	const FVector& XYZ = *this;
	FVector Normal( XYZ|Coords.XAxis, XYZ|Coords.YAxis, XYZ|Coords.ZAxis );
	return FPlane( Normal, W - (Coords.Origin.TransformVectorBy(Coords) | Normal) );
}

/*-----------------------------------------------------------------------------
	FCoords functions.
-----------------------------------------------------------------------------*/

//
// Return this coordinate system's transpose.
// If the coordinate system is orthogonal, this is equivalent to its inverse.
//
inline FCoords FCoords::Transpose() const
{
	return FCoords
	(
		-Origin.TransformVectorBy(*this),
		FVector( XAxis.X, YAxis.X, ZAxis.X ),
		FVector( XAxis.Y, YAxis.Y, ZAxis.Y ),
		FVector( XAxis.Z, YAxis.Z, ZAxis.Z )
	);
}

//
// Mirror the coordinates about a normal vector.
//
inline FCoords FCoords::MirrorByVector( const FVector& MirrorNormal ) const
{
	return FCoords
	(
		Origin.MirrorByVector( MirrorNormal ),
		XAxis .MirrorByVector( MirrorNormal ),
		YAxis .MirrorByVector( MirrorNormal ),
		ZAxis .MirrorByVector( MirrorNormal )
	);
}

//
// Mirror the coordinates about a plane.
//
inline FCoords FCoords::MirrorByPlane( const FPlane& Plane ) const
{
	return FCoords
	(
		Origin.MirrorByPlane ( Plane ),
		XAxis .MirrorByVector( Plane ),
		YAxis .MirrorByVector( Plane ),
		ZAxis .MirrorByVector( Plane )
	);
}

/*-----------------------------------------------------------------------------
	FCoords operators.
-----------------------------------------------------------------------------*/

//
// Transform this coordinate system by another coordinate system.
//
inline FCoords& FCoords::operator*=( const FCoords& TransformCoords )
{
	//!! Proper solution:
	//Origin = Origin.TransformPointBy( TransformCoords.Inverse().Transpose() );
	// Fast solution assuming orthogonal coordinate system:
	Origin = Origin.TransformPointBy ( TransformCoords );
	XAxis  = XAxis .TransformVectorBy( TransformCoords );
	YAxis  = YAxis .TransformVectorBy( TransformCoords );
	ZAxis  = ZAxis .TransformVectorBy( TransformCoords );
	return *this;
}
inline FCoords FCoords::operator*( const FCoords &TransformCoords ) const
{
	return FCoords(*this) *= TransformCoords;
}

//
// Transform this coordinate system by a pitch-yaw-roll rotation.
//
inline FCoords& FCoords::operator*=( const FRotator &Rot )
{
	// Apply yaw rotation.
	*this *= FCoords
	(	
		FVector( 0.f, 0.f, 0.f ),
		FVector( +GMath.CosTab(Rot.Yaw), +GMath.SinTab(Rot.Yaw), +0.f ),
		FVector( -GMath.SinTab(Rot.Yaw), +GMath.CosTab(Rot.Yaw), +0.f ),
		FVector( +0.f, +0.f, +1.f )
	);

	// Apply pitch rotation.
	*this *= FCoords
	(	
		FVector( 0.f, 0.f, 0.f ),
		FVector( +GMath.CosTab(Rot.Pitch), +0.f, +GMath.SinTab(Rot.Pitch) ),
		FVector( +0.f, +1.f, +0.f ),
		FVector( -GMath.SinTab(Rot.Pitch), +0.f, +GMath.CosTab(Rot.Pitch) )
	);

	// Apply roll rotation.
	*this *= FCoords
	(	
		FVector( 0.f, 0.f, 0.f ),
		FVector( +1.f, +0.f, +0.f ),
		FVector( +0.f, +GMath.CosTab(Rot.Roll), -GMath.SinTab(Rot.Roll) ),
		FVector( +0.f, +GMath.SinTab(Rot.Roll), +GMath.CosTab(Rot.Roll) )
	);
	return *this;
}
inline FCoords FCoords::operator*( const FRotator &Rot ) const
{
	return FCoords(*this) *= Rot;
}

inline FCoords& FCoords::operator*=( const FVector &Point )
{
	Origin -= Point;
	return *this;
}
inline FCoords FCoords::operator*( const FVector &Point ) const
{
	return FCoords(*this) *= Point;
}

//
// Detransform this coordinate system by a pitch-yaw-roll rotation.
//
inline FCoords& FCoords::operator/=( const FRotator &Rot )
{
	// Apply inverse roll rotation.
	*this *= FCoords
	(
		FVector( 0.f, 0.f, 0.f ),
		FVector( +1.f, -0.f, +0.f ),
		FVector( -0.f, +GMath.CosTab(Rot.Roll), +GMath.SinTab(Rot.Roll) ),
		FVector( +0.f, -GMath.SinTab(Rot.Roll), +GMath.CosTab(Rot.Roll) )
	);

	// Apply inverse pitch rotation.
	*this *= FCoords
	(
		FVector( 0.f, 0.f, 0.f ),
		FVector( +GMath.CosTab(Rot.Pitch), +0.f, -GMath.SinTab(Rot.Pitch) ),
		FVector( +0.f, +1.f, -0.f ),
		FVector( +GMath.SinTab(Rot.Pitch), +0.f, +GMath.CosTab(Rot.Pitch) )
	);

	// Apply inverse yaw rotation.
	*this *= FCoords
	(
		FVector( 0.f, 0.f, 0.f ),
		FVector( +GMath.CosTab(Rot.Yaw), -GMath.SinTab(Rot.Yaw), -0.f ),
		FVector( +GMath.SinTab(Rot.Yaw), +GMath.CosTab(Rot.Yaw), +0.f ),
		FVector( -0.f, +0.f, +1.f )
	);
	return *this;
}
inline FCoords FCoords::operator/( const FRotator &Rot ) const
{
	return FCoords(*this) /= Rot;
}

inline FCoords& FCoords::operator/=( const FVector &Point )
{
	Origin += Point;
	return *this;
}
inline FCoords FCoords::operator/( const FVector &Point ) const
{
	return FCoords(*this) /= Point;
}

//
// Transform this coordinate system by a scale.
// Note: Will return coordinate system of opposite handedness if
// Scale.X*Scale.Y*Scale.Z is negative.
//
inline FCoords& FCoords::operator*=( const FScale &Scale )
{
	// Apply sheering.
	FLOAT   Sheer      = FSheerSnap( Scale.SheerRate );
	FCoords TempCoords = GMath.UnitCoords;
	switch( Scale.SheerAxis )
	{
		case SHEER_XY:
			TempCoords.XAxis.Y = Sheer;
			break;
		case SHEER_XZ:
			TempCoords.XAxis.Z = Sheer;
			break;
		case SHEER_YX:
			TempCoords.YAxis.X = Sheer;
			break;
		case SHEER_YZ:
			TempCoords.YAxis.Z = Sheer;
			break;
		case SHEER_ZX:
			TempCoords.ZAxis.X = Sheer;
			break;
		case SHEER_ZY:
			TempCoords.ZAxis.Y = Sheer;
			break;
		default:
			break;
	}
	*this *= TempCoords;

	// Apply scaling.
	XAxis    *= Scale.Scale;
	YAxis    *= Scale.Scale;
	ZAxis    *= Scale.Scale;
	Origin.X /= Scale.Scale.X;
	Origin.Y /= Scale.Scale.Y;
	Origin.Z /= Scale.Scale.Z;

	return *this;
}
inline FCoords FCoords::operator*( const FScale &Scale ) const
{
	return FCoords(*this) *= Scale;
}

//
// Detransform a coordinate system by a scale.
//
inline FCoords& FCoords::operator/=( const FScale &Scale )
{
	// Deapply scaling.
	XAxis    /= Scale.Scale;
	YAxis    /= Scale.Scale;
	ZAxis    /= Scale.Scale;
	Origin.X *= Scale.Scale.X;
	Origin.Y *= Scale.Scale.Y;
	Origin.Z *= Scale.Scale.Z;

	// Deapply sheering.
	FCoords TempCoords(GMath.UnitCoords);
	FLOAT Sheer = FSheerSnap( Scale.SheerRate );
	switch( Scale.SheerAxis )
	{
		case SHEER_XY:
			TempCoords.XAxis.Y = -Sheer;
			break;
		case SHEER_XZ:
			TempCoords.XAxis.Z = -Sheer;
			break;
		case SHEER_YX:
			TempCoords.YAxis.X = -Sheer;
			break;
		case SHEER_YZ:
			TempCoords.YAxis.Z = -Sheer;
			break;
		case SHEER_ZX:
			TempCoords.ZAxis.X = -Sheer;
			break;
		case SHEER_ZY:
			TempCoords.ZAxis.Y = -Sheer;
			break;
		default: // SHEER_NONE
			break;
	}
	*this *= TempCoords;

	return *this;
}
inline FCoords FCoords::operator/( const FScale &Scale ) const
{
	return FCoords(*this) /= Scale;
}

/*-----------------------------------------------------------------------------
	Random numbers.
-----------------------------------------------------------------------------*/

//
// Compute pushout of a box from a plane.
//
inline FLOAT FBoxPushOut( FVector Normal, FVector Size )
{
	return Abs(Normal.X*Size.X) + Abs(Normal.Y*Size.Y) + Abs(Normal.Z*Size.Z);
}

//
// Return a uniformly distributed random unit vector.
//
inline FVector VRand()
{
	FVector Result;
	do
	{
		// Check random vectors in the unit sphere so result is statistically uniform.
		Result.X = appFrand()*2 - 1;
		Result.Y = appFrand()*2 - 1;
		Result.Z = appFrand()*2 - 1;
	} while( Result.SizeSquared() > 1.f );
	return Result.UnsafeNormal();
}

//
// RandomSpreadVector
// By Paul Du Bois, Infinite Machine
//
// Return a random unit vector within a cone of spread_degrees.
// (Distribution is such that there is no bunching near the axis.)
//
// - Create an FRotator with pitch < spread_degrees/2, yaw = 0, roll = random.
//   The tricky bit is getting the probability distribution of pitch
//   correct.  If it's flat, particles will tend to cluster around the pole.
//
// - For a given pitch phi, the probability p(phi) should be proportional to
//   the surface area of the slice of sphere defined by that pitch and a
//   random roll.  This is 2 pi (r sin phi) (r d phi) =~ sin phi
//
// - To map a flat distribution (FRand) to our new p(phi), we first find the
//   CDF (cumulative distribution fn) which is basically the integral of
//   p(phi) over its domain, normalized to have a total area of 1.  Smaller
//   spreads are modeled by limiting the domain of p(phi) and normalizing
//   the CDF for the smaller domain.  The CDF is always monotonically
//   non-decreasing and has range [0,1].
//
// - The insight is that the inverse of the CDF is exactly what we need to
//   convert a flat distribution to our p(phi).  aCDF: [0,1] -> angle.
//   (insert handwaving argument)
//
// - CDF(x) = integral(0->x) sin phi d phi
//          = -cos(x) - -cos(0)
//          = K * (1 - cos(x)) (K is normalizing factor to make CDF(domain_max)=1)
//        K = 1/max_cos_val   max_cos_val = (1 - cos(max pitch))
//
// - aCDF(y) = acos(1-y/K) = acos(1-y*max_cos_val)
//
inline FVector RandomSpreadVector(FLOAT spread_degrees)
{
    FLOAT max_pitch = Clamp(spread_degrees * (PI / 180.0f / 2.0f),0.0f,180.0f);
    FLOAT K = 1.0f - appCos(max_pitch);
    FLOAT pitch = appAcos(1.0f - appFrand()*K);  // this is the aCDF
    FLOAT rand_roll = appFrand() * (2.0f * PI);
    FLOAT radius = appSin(pitch);
    return FVector(appCos(pitch),radius*appSin(rand_roll),radius*appCos(rand_roll));
}

/*-----------------------------------------------------------------------------
	Advanced geometry.
-----------------------------------------------------------------------------*/

//
// Find the intersection of an infinite line (defined by two points) and
// a plane.  Assumes that the line and plane do indeed intersect; you must
// make sure they're not parallel before calling.
//
inline FVector FLinePlaneIntersection
(
	const FVector &Point1,
	const FVector &Point2,
	const FVector &PlaneOrigin,
	const FVector &PlaneNormal
)
{
	return
		Point1
	+	(Point2-Point1)
	*	(((PlaneOrigin - Point1)|PlaneNormal) / ((Point2 - Point1)|PlaneNormal));
}
inline FVector FLinePlaneIntersection
(
	const FVector &Point1,
	const FVector &Point2,
	const FPlane  &Plane
)
{
	return
		Point1
	+	(Point2-Point1)
	*	((Plane.W - (Point1|Plane))/((Point2 - Point1)|Plane));
}

/*-----------------------------------------------------------------------------
	FPlane functions.
-----------------------------------------------------------------------------*/

//
// Compute intersection point of three planes.
// Return 1 if valid, 0 if infinite.
//
inline UBOOL FIntersectPlanes3( FVector& I, const FPlane& P1, const FPlane& P2, const FPlane& P3 )
{
	guard(FIntersectPlanes3);

	// Compute determinant, the triple product P1|(P2^P3)==(P1^P2)|P3.
	FLOAT Det = ((FVector)P1 ^ (FVector)P2) | (FVector)P3;
	if( Square(Det) < Square(0.001f) )
	{
		// Degenerate.
		I = FVector(0,0,0);
		return 0;
	}
	else
	{
		// Compute the intersection point, guaranteed valid if determinant is nonzero.
		I = (P1.W*((FVector)P2^(FVector)P3) + P2.W*((FVector)P3^(FVector)P1) + P3.W*((FVector)P1^(FVector)P2)) / Det;
	}
	return 1;
	unguard;
}

//
// Compute intersection point and direction of line joining two planes.
// Return 1 if valid, 0 if infinite.
//
inline UBOOL FIntersectPlanes2( FVector& I, FVector& D, const FPlane& P1, const FPlane& P2 )
{
	guard(FIntersectPlanes2);

	// Compute line direction, perpendicular to both plane normals.
	D = (FVector)P1 ^ (FVector)P2;
	FLOAT DD = D.SizeSquared();
	if( DD < Square(0.001f) )
	{
		// Parallel or nearly parallel planes.
		D = I = FVector(0,0,0);
		return 0;
	}
	else
	{
		// Compute intersection.
		I = (P1.W*((FVector)P2^D) + P2.W*(D^(FVector)P1)) / DD;
		D.Normalize();
		return 1;
	}
	unguard;
}

/*-----------------------------------------------------------------------------
	FRotator functions.
-----------------------------------------------------------------------------*/

//
// Convert a rotation into a vector facing in its direction.
//
inline FVector FRotator::Vector()
{
	return (GMath.UnitCoords / *this).XAxis;
}


/*-----------------------------------------------------------------------------
	FMatrix.          
-----------------------------------------------------------------------------*/
// Floating point 4 x 4  (4 x 3)  KNI-friendly matrix
class CORE_API FMatrix
{
public:

	// Variables.
	FPlane XPlane; // each plane [x,y,z,w] is a *column* in the matrix.
	FPlane YPlane;
	FPlane ZPlane;
	FPlane WPlane;

	FLOAT& M(INT i,INT j) {return ((FLOAT*)&XPlane)[i*4+j];}
	const FLOAT& M(INT i,INT j) const {return ((FLOAT*)&XPlane)[i*4+j];}

	// Constructors.
	FMatrix()
	{}
	FMatrix( FPlane InX, FPlane InY, FPlane InZ )
	:	XPlane(InX), YPlane(InY), ZPlane(InZ), WPlane(0,0,0,0)
	{}
	FMatrix( FPlane InX, FPlane InY, FPlane InZ, FPlane InW )
	:	XPlane(InX), YPlane(InY), ZPlane(InZ), WPlane(InW)
	{}


	// Regular transform
	FVector TransformFVector(const FVector &V) const
	{
		FVector FV;

		FV.X = V.X * M(0,0) + V.Y * M(0,1) + V.Z * M(0,2) + M(0,3);
		FV.Y = V.X * M(1,0) + V.Y * M(1,1) + V.Z * M(1,2) + M(1,3);
		FV.Z = V.X * M(2,0) + V.Y * M(2,1) + V.Z * M(2,2) + M(2,3);

		return FV;
	}

	// Homogeneous transform
	FPlane TransformFPlane(const FPlane &P) const
	{
		FPlane FP;

		FP.X = P.X * M(0,0) + P.Y * M(0,1) + P.Z * M(0,2) + M(0,3);
		FP.Y = P.X * M(1,0) + P.Y * M(1,1) + P.Z * M(1,2) + M(1,3);
		FP.Z = P.X * M(2,0) + P.Y * M(2,1) + P.Z * M(2,2) + M(2,3);
		FP.W = P.X * M(3,0) + P.Y * M(3,1) + P.Z * M(3,2) + M(3,3);

		return FP;
	}

	FQuat FMatrixToFQuat();

	// Combine transforms binary operation MxN
	friend FMatrix CombineTransforms(const FMatrix& M, const FMatrix& N);
	friend FMatrix FMatrixFromFCoords(const FCoords& FC);
	friend FCoords FCoordsFromFMatrix(const FMatrix& FM);

	// Returns a string description.
	FString String() const
	{
		guardSlow(FMatrix::String);
		return FString::Printf(TEXT("(XPlane=%s,YPlane=%s,ZPlane=%s,WPlane=%s)"), *XPlane.String(), *YPlane.String(), *ZPlane.String(), *WPlane.String());
		unguardSlow;
	}
};

FMatrix CombineTransforms(const FMatrix& M, const FMatrix& N);

// Conversions for Unreal1 coordinate system class.

inline FMatrix FMatrixFromFCoords(const FCoords& FC) 
{
	FMatrix M;
	M.XPlane = FPlane( FC.XAxis.X, FC.XAxis.Y, FC.XAxis.Z, FC.Origin.X );
	M.YPlane = FPlane( FC.YAxis.X, FC.YAxis.Y, FC.YAxis.Z, FC.Origin.Y );
	M.ZPlane = FPlane( FC.ZAxis.X, FC.ZAxis.Y, FC.ZAxis.Z, FC.Origin.Z );
	M.WPlane = FPlane( 0.f,        0.f,        0.f,        1.f         );
	return M;
}

inline FCoords FCoordsFromFMatrix(const FMatrix& FM)
{
	FCoords C;
	C.Origin = FVector( FM.XPlane.W, FM.YPlane.W, FM.ZPlane.W );
	C.XAxis  = FVector( FM.XPlane.X, FM.XPlane.Y, FM.XPlane.Z );
	C.YAxis  = FVector( FM.YPlane.X, FM.YPlane.Y, FM.YPlane.Z );
	C.ZAxis  = FVector( FM.ZPlane.X, FM.ZPlane.Y, FM.ZPlane.Z );
	return C;
}



/*-----------------------------------------------------------------------------
	FQuat.          
-----------------------------------------------------------------------------*/

// floating point quaternion.
class CORE_API FQuat 
{
public:
	// Variables.
	FLOAT X,Y,Z,W;
	// X,Y,Z, W also doubles as the Axis/Angle format.

	// Constructors.
	FQuat()
	{}

	FQuat( FLOAT InX, FLOAT InY, FLOAT InZ, FLOAT InA )
	:	X(InX), Y(InY), Z(InZ), W(InA)
	{}

	// Binary operators.
	FQuat operator+( const FQuat& Q ) const
	{
		return FQuat( X + Q.X, Y + Q.Y, Z + Q.Z, W + Q.W );
	}

	FQuat operator-( const FQuat& Q ) const
	{
		return FQuat( X - Q.X, Y - Q.Y, Z - Q.Z, W - Q.W );
	}

	FQuat operator*( const FQuat& Q ) const
	{
		return FQuat( 
			X*Q.X - Y*Q.Y - Z*Q.Z - W*Q.W, 
			X*Q.Y + Y*Q.X + Z*Q.W - W*Q.Z, 
			X*Q.Z - Y*Q.W + Z*Q.X + W*Q.Y, 
			X*Q.W + Y*Q.Z - Z*Q.Y + W*Q.X
			);
	}

	FQuat operator*( const FLOAT& Scale ) const
	{
		return FQuat( Scale*X, Scale*Y, Scale*Z, Scale*W);			
	}
	
	// Unary operators.
	FQuat operator-() const
	{
		return FQuat( X, Y, Z, -W );
	}

    // Misc operators
	UBOOL operator!=( const FQuat& Q ) const
	{
		return X!=Q.X || Y!=Q.Y || Z!=Q.Z || W!=Q.W;
	}
	
	UBOOL Normalize()
	{
		// 
		FLOAT SquareSum = (FLOAT)(X*X+Y*Y+Z*Z+W*W);
		if( SquareSum >= DELTA )
		{
			FLOAT Scale = 1.0f/(FLOAT)appSqrt(SquareSum);
			X *= Scale; 
			Y *= Scale; 
			Z *= Scale;
			W *= Scale;
			return true;
		}
		else 
		{	
			X = 0.0f;
			Y = 0.0f;
			Z = 0.1f;
			W = 0.0f;
			return false;
		}
	}

	// Serializer.
	friend FArchive& operator<<( FArchive& Ar, FQuat& F )
	{
		return Ar << F.X << F.Y << F.Z << F.W;
	}


	// Returns a string description.
	FString String() const
	{
		guardSlow(FQuat::String);
		return FString::Printf(TEXT("(X=%f,Y=%f,Z=%f,W=%f)"), X, Y, Z, W);
		unguardSlow;
	}

	FMatrix FQuatToFMatrix();

	// Warning : assumes normalized quaternions.
	FQuat FQuatToAngAxis()
	{
		FLOAT scale = (FLOAT)appSin(W);
		FQuat A;

		if (scale >= DELTA)
		{
			A.X = Z / scale;
			A.Y = Y / scale;
			A.Z = Z / scale;
			A.W = (2.0f * appAcos (W)); 
			// Degrees: A.W = ((FLOAT)appAcos(W) * 360.0f) * INV_PI;  
		}
		else 
		{
			A.X = 0.0f;
			A.Y = 0.0f;
			A.Z = 1.0f;
			A.W = 0.0f; 
		}

		return A;
	};

	//
	// Angle-Axis to Quaternion. No normalized axis assumed.
	//
	FQuat AngAxisToFQuat()
	{
		FLOAT scale = X*X + Y*Y + Z*Z;
		FQuat Q;

		if (scale >= DELTA)
		{
			FLOAT invscale = 1.0f /(FLOAT)appSqrt(scale);
			Q.X = X * invscale;
			Q.Y = Y * invscale;
			Q.Z = Z * invscale;
			Q.W = appCos( W * 0.5f); //Radians assumed.
		}
		else
		{
			Q.X = 0.0f;
			Q.Y = 0.0f;
			Q.Z = 1.0f;
			Q.W = 0.0f; 
		}
		return Q;
	}	
};


// Dot product of axes to get cos of angle  #Warning some people use .W component here too !
inline FLOAT FQuatDot(const FQuat& Q1,const FQuat& Q2)
{
	return( Q1.X*Q2.X + Q1.Y*Q2.Y + Q1.Z*Q2.Z );
};

// Error measure (angle) between two quaternions, ranged [0..1]
inline FLOAT FQuatError(FQuat& Q1,FQuat& Q2)
{
	// Returns the hypersphere-angle between two quaternions; alignment shouldn't matter, though 
	// normalized input is expected.
	FLOAT cosom = Q1.X*Q2.X + Q1.Y*Q2.Y + Q1.Z*Q2.Z + Q1.W*Q2.W;
	return (Abs(cosom) < 0.9999999f) ? appAcos(cosom)*(1.f/PI) : 0.0f;
}

// Ensure quat1 points to same side of the hypersphere as quat2
inline void AlignFQuatWith(FQuat &quat1, const FQuat &quat2)
{
	FLOAT Minus  = Square(quat1.X-quat2.X) + Square(quat1.Y-quat2.Y) + Square(quat1.Z-quat2.Z) + Square(quat1.W-quat2.W);
	FLOAT Plus   = Square(quat1.X+quat2.X) + Square(quat1.Y+quat2.Y) + Square(quat1.Z+quat2.Z) + Square(quat1.W+quat2.W);

	if (Minus > Plus)
	{
		quat1.X = - quat1.X;
		quat1.Y = - quat1.Y;
		quat1.Z = - quat1.Z;
		quat1.W = - quat1.W;
	}
}

// No-frills spherical interpolation. Assumes aligned quaternions, and the output is not normalized.
inline FQuat SlerpQuat(const FQuat &quat1,const FQuat &quat2, float slerp)
{
	FQuat result;
	float omega,cosom,sininv,scale0,scale1;

	// Get cosine of angle betweel quats.
	cosom = quat1.X * quat2.X +
			quat1.Y * quat2.Y +
			quat1.Z * quat2.Z +
			quat1.W * quat2.W;

	if( cosom < 0.99999999f )
	{	
		omega = appAcos(cosom);
		sininv = 1.f/appSin(omega);
		scale0 = appSin((1.f - slerp) * omega) * sininv;
		scale1 = appSin(slerp * omega) * sininv;
		
		result.X = scale0 * quat1.X + scale1 * quat2.X;
		result.Y = scale0 * quat1.Y + scale1 * quat2.Y;
		result.Z = scale0 * quat1.Z + scale1 * quat2.Z;
		result.W = scale0 * quat1.W + scale1 * quat2.W;
		return result;
	}
	else
	{
		return quat1;
	}
	
}


/*-----------------------------------------------------------------------------
	Inlines.
-----------------------------------------------------------------------------*/

inline UBOOL FVector::NormalizeSlow()
{
	FLOAT Size = X * X + Y * Y + Z * Z;
	if (Size >= SMALL_NUMBER)
	{
		FLOAT Scale = 1.f / sqrtf(Size);
		X *= Scale; Y *= Scale; Z *= Scale;
		return TRUE;
	}
	return FALSE;
}

inline FVector FVector::SafeNormalSlow() const
{
	FLOAT Size = X * X + Y * Y + Z * Z;
	if (Size < SMALL_NUMBER)
		return FVector(0.f, 0.f, 0.f);
	
	FLOAT Scale = 1.f / sqrtf(Size);
	return FVector(*this) * Scale;
}

#if USES_SSE_INTRINSICS
	#include "UnMath_SSE.inl"
	#if VECTOR_ALIGNMENT == 16
		#include "UnVector_SSE_64.inl"
	#else
		#include "UnVector.inl"
	#endif
	#include "UnPlane_SSE.inl"
#else
	#include "UnVector.inl"
	#include "UnPlane.inl"
#endif


/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
