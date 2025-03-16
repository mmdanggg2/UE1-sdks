/*=============================================================================
	UnTemplate.h: Unreal templates.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

/*-----------------------------------------------------------------------------
	Type information.
-----------------------------------------------------------------------------*/

#ifndef _INCL_UNTEMPLATE_H_
#define _INCL_UNTEMPLATE_H_

//
// Type information for initialization.
//
template <class T> struct TTypeInfoBase
{
public:
	typedef const T& ConstInitType;
	static UBOOL NeedsDestructor() {return 1;}
	static UBOOL DefinitelyNeedsDestructor() {return 0;}
	static const T& ToInit( const T& In ) {return In;}
};
template <class T> struct TTypeInfo : public TTypeInfoBase<T>
{
};

template <> struct TTypeInfo<BYTE> : public TTypeInfoBase<BYTE>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
template <> struct TTypeInfo<SBYTE> : public TTypeInfoBase<SBYTE>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
template <> struct TTypeInfo<ANSICHAR> : public TTypeInfoBase<ANSICHAR>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
#if !WIN32
template <> struct TTypeInfo<TCHAR> : public TTypeInfoBase<TCHAR>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
#endif
template <> struct TTypeInfo<INT> : public TTypeInfoBase<INT>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
template <> struct TTypeInfo<DWORD> : public TTypeInfoBase<DWORD>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
template <> struct TTypeInfo<_WORD> : public TTypeInfoBase<_WORD>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
template <> struct TTypeInfo<SWORD> : public TTypeInfoBase<SWORD>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
template <> struct TTypeInfo<QWORD> : public TTypeInfoBase<QWORD>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
template <> struct TTypeInfo<SQWORD> : public TTypeInfoBase<SQWORD>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
template <> struct TTypeInfo<FName> : public TTypeInfoBase<FName>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
template <> struct TTypeInfo<UObject*> : public TTypeInfoBase<UObject*>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
template <> struct TTypeInfo<FLazyLoader*> : public TTypeInfoBase<FLazyLoader*>
{
public:
	static UBOOL NeedsDestructor() { return 0; }
};

/*-----------------------------------------------------------------------------
	C++ 11 templates
-----------------------------------------------------------------------------*/

// TRemoveReference<type> will remove any references from a type.
template <typename T> struct TRemoveReference      { typedef T Type; };
template <typename T> struct TRemoveReference<T& > { typedef T Type; };
template <typename T> struct TRemoveReference<T&&> { typedef T Type; };

// MoveTemp will cast a reference to an rvalue reference.
// This is UE's equivalent of std::move.
template <typename T>
FORCEINLINE typename TRemoveReference<T>::Type&& MoveTemp(T&& Obj)
{
	return (typename TRemoveReference<T>::Type&&)Obj;
}

// Forward will cast a reference to an rvalue reference.
// This is UE's equivalent of std::forward.
template <typename T> FORCEINLINE T&& Forward(typename TRemoveReference<T>::Type&  Obj) { return (T&&)Obj; }
template <typename T> FORCEINLINE T&& Forward(typename TRemoveReference<T>::Type&& Obj) { return (T&&)Obj; }


/*-----------------------------------------------------------------------------
	Standard templates.
-----------------------------------------------------------------------------*/

template< class T > inline T Abs( const T A )
{
	return (A>=(T)0) ? A : -A;
}
template< class T > inline T Sgn( const T A )
{
	return (A>0) ? 1 : ((A<0) ? -1 : 0);
}
template< class T > inline T Max( const T A, const T B )
{
	return (A>=B) ? A : B;
}
template< class T > inline T Max(const T A, const T B, const T C)
{
	T X = Max<T>(A, B);
	return (X >= C) ? X : C;
}
template< class T > inline T Max(const T A, const T B, const T C, const T D)
{
	T X = Max<T>(A, B);
	T Y = Max<T>(C, D);
	return (X >= Y) ? X : Y;
}
template< class T > inline T Min( const T A, const T B )
{
	return (A<=B) ? A : B;
}
template< class T > inline T Min(const T A, const T B, const T C)
{
	T X = Min<T>(A, B);
	return (X <= C) ? X : C;
}
template< class T > inline T Min(const T A, const T B, const T C, const T D)
{
	T X = Min<T>(A, B);
	T Y = Min<T>(C, D);
	return (X <= Y) ? X : Y;
}
template< class T > inline T Square( const T A )
{
	return A*A;
}
template< class T > inline T Cube(const T A)
{
	return A * A * A;
}
template< class T > inline T Quad(const T A)
{
	return A * A * A * A;
}
template< class T > inline T Clamp( const T X, const T Min, const T Max )
{
	return X<Min ? Min : X<Max ? X : Max;
}
template< class T > inline T Align( const T Ptr, PTRINT Alignment )
{
	return (T)(((PTRINT)Ptr + Alignment - 1) & ~(Alignment-1));
}
template< class T > inline void Exchange( T& A, T& B )
{
	T Temp = MoveTemp(A);
	A = MoveTemp(B);
	B = MoveTemp(Temp);
}
template< class T > T Lerp( const T& A, const T& B, FLOAT Alpha )
{
	return A + Alpha * (B-A);
}
inline DWORD GetTypeHash( const BYTE A )
{
	return A;
}
inline DWORD GetTypeHash( const SBYTE A )
{
	return A;
}
inline DWORD GetTypeHash( const _WORD A )
{
	return A;
}
inline DWORD GetTypeHash( const SWORD A )
{
	return A;
}
inline DWORD GetTypeHash( const INT A )
{
	return A;
}
inline DWORD GetTypeHash( const DWORD A )
{
	return A;
}
inline DWORD GetTypeHash( const QWORD A )
{
	return (DWORD)A+((DWORD)(A>>32) * 23);
}
inline DWORD GetTypeHash( const SQWORD A )
{
	return (DWORD)A+((DWORD)(A>>32) * 23);
}
inline DWORD GetTypeHash( const TCHAR* S )
{
	return appStrihash(S);
}
#define ExchangeB(A,B) {UBOOL T=A; A=B; B=T;}


/*----------------------------------------------------------------------------
	Constants.
----------------------------------------------------------------------------*/

// UE 5.4 styled parameter to control array and string shrinking
enum class EAllowShrinking
{
	No  = 0,
	Yes = 1,
};


/*----------------------------------------------------------------------------
	Standard macros.
----------------------------------------------------------------------------*/

// Number of elements in an array.
#define ARRAY_COUNT( array ) \
	( sizeof(array) / sizeof((array)[0]) )

// Offset of a struct member.
// The below, using offset 0 (NULL), makes gcc think we're misusing
//  the offsetof macro...we get around this by using 1 instead.  :) --ryan.
//#define STRUCT_OFFSET( struc, member )		\
//	( (PTRINT)&((struc*)0)->member )
#define STRUCT_OFFSET( struc, member )					\
	( reinterpret_cast<PTRINT>(&((struc*)1)->member ) - 1 )

// stijn: both of the above macros are invalid in standard C++
// the one below is valid as long as struc is a standard layout data type
//#define STRUCT_OFFSET(struc, member) offsetof(struc, member)


/*-----------------------------------------------------------------------------
	Metaprogramming utils.
-----------------------------------------------------------------------------*/

//
// Constant expression bit util.
//
template<QWORD N, DWORD Bit=63> struct TBits
{
	static constexpr DWORD BitCount = (N>>(Bit-1)) ? Bit : TBits<N,Bit-1>::BitCount;
	static constexpr QWORD BitMask = (1ull << BitCount) - 1;
	static constexpr QWORD MostSignificant = 1ull << (BitCount-1);
	static constexpr QWORD CeilLogTwo = (MostSignificant >= N) ? BitCount-1 : BitCount; //Compile time version of appCeilLogTwo
};
template<QWORD N> struct TBits<N,0>
{
	static constexpr DWORD BitCount = 0;
	static constexpr QWORD BitMask = 0;
	static constexpr QWORD MostSignificant = 0;
	static constexpr QWORD CeilLogTwo = 0; //appCeilLogTwo(0)=0
};
static_assert( TBits<20>::BitCount        == 5,        "Error in TBits<20> #BitCount#");
static_assert( TBits<20>::BitMask         == 0b011111, "Error in TBits<20> #BitMask#");
static_assert( TBits<20>::MostSignificant == 0b010000, "Error in TBits<20> #MostSignificant#");
static_assert( TBits<20>::CeilLogTwo      == 5,        "Error in TBits<20> #CeilLogTwo#");
static_assert( TBits<32>::BitCount        == 6,        "Error in TBits<32> #BitCount#");
static_assert( TBits<32>::BitMask         == 0b111111, "Error in TBits<32> #BitMask#");
static_assert( TBits<32>::MostSignificant == 0b100000, "Error in TBits<32> #MostSignificant#");
static_assert( TBits<32>::CeilLogTwo      == 5,        "Error in TBits<32> #CeilLogTwo#");


//
// Raw data util.
//
#define USES_TRAWDATA 1
template< typename T > struct TRawDataHelperBase
{
	static void CopySingle( T& Out, const T& In)
	{
		Out = In;
	}
	static void MoveSingle( T& To, T& From)
	{
		To = From;
		From = (T)0;
	}
	static void SwapSingle( T& A, T& B)
	{
		T C = A;
		A = B;
		B = C;
	}
	static bool EqualsSingle( const T& A, const T& B)
	{
		return A == B;
	}
};

template< typename T > struct TRawDataHelper : public TRawDataHelperBase<T>
{
};

// clang 14 and 15 emit aligned loads and stores here because it thinks all
// variables we use here are 16-byte aligned
#if USES_SSE_INTRINSICS && !defined(__clang__) 
template<> struct TRawDataHelper<__m128i> : public TRawDataHelperBase<__m128i>
{
	static void CopySingle( __m128i& Out, const __m128i& In)
	{
		_mm_storeu_si128( &Out, _mm_loadu_si128(&In) );
	}
	static void MoveSingle( __m128i& To, __m128i& From)
	{
		_mm_storeu_si128( &To, _mm_loadu_si128(&From) );
		_mm_storeu_si128( &From, _mm_setzero_si128() );
	}
	static void SwapSingle( __m128i& A, __m128i& B)
	{
		__m128i C;
		_mm_storeu_si128( &C, _mm_loadu_si128(&A) );
		_mm_storeu_si128( &A, _mm_loadu_si128(&B) );
		_mm_storeu_si128( &B, _mm_loadu_si128(&C) );
	}
	static bool EqualsSingle( const __m128i& A, const __m128i& B)
	{
		return _mm_movemask_ps( _mm_castsi128_ps(_mm_cmpeq_epi32(_mm_loadu_si128(&A),_mm_loadu_si128(&B))) ) == 0b1111;
	}
};
#endif

template< typename T > struct TRawData
{
public:
	static void Copy( T& Out, const T& In);
	static void Move( T& To, T& From);
	static void Swap( T& A, T& B);
	static bool Equals( const T& A, const T& B);

public:
	static constexpr bool Supports16 = !!USES_SSE_INTRINSICS
#if defined(__clang__)
		&& 0
#endif
		;
	static constexpr bool Supports8  = !!BUILD_64;

	// Precomputed steps.
	static constexpr PTRINT Offset16 = 0;
	static constexpr PTRINT Loops16  = Supports16 ? ((sizeof(T)-Offset16) / 16) : 0;

	static constexpr PTRINT Offset8  = Offset16 + Loops16 * 16;
	static constexpr PTRINT Loops8   = Supports8  ? ((sizeof(T)-Offset8)  / 8) : 0;

	static constexpr PTRINT Offset4  = Offset8  + Loops8 * 8;
	static constexpr PTRINT Loops4   = (sizeof(T) - Offset4) / 4;

	static constexpr PTRINT Offset2  = Offset4  + Loops4 * 4;
	static constexpr PTRINT Loops2   = (sizeof(T) - Offset2) / 2;

	static constexpr PTRINT Offset1  = Offset2  + Loops2 * 2;
	static constexpr PTRINT Loops1   = (sizeof(T) - Offset1);

private:

	// Loop inners
	template< typename D, DWORD Loops > static void CopyWithType( D& Out, const D& In)
	{
		for ( DWORD i=0; i<Loops; i++)
			TRawDataHelper<D>::CopySingle( (&Out)[i], (&In)[i]);
	}
	template< typename D, DWORD Loops > static void MoveWithType( D& To, D& From)
	{
		for ( DWORD i=0; i<Loops; i++)
			TRawDataHelper<D>::MoveSingle( (&To)[i], (&From)[i]);
	}
	template< typename D, DWORD Loops > static void SwapWithType( D& A, D& B)
	{
		for ( DWORD i=0; i<Loops; i++)
			TRawDataHelper<D>::SwapSingle( (&A)[i], (&B)[i]);
	}
	template< typename D, DWORD Loops > static bool EqualsWithType( const D& A, const D& B)
	{
		for ( DWORD i=0; i<Loops; i++)
			if ( !TRawDataHelper<D>::EqualsSingle((&A)[i],(&B)[i]) )
				return false;
		return true;
	}
};

template<typename T> inline void TRawData<T>::Copy( T& Out, const T& In)
{
#if USES_SSE_INTRINSICS && !defined(__clang__)
	CopyWithType<__m128i,Loops16>( *(__m128i*)&Out, *(__m128i*)&In);
#endif
	CopyWithType<QWORD,Loops8>( *(QWORD*)((BYTE*)&Out + Offset8), *(QWORD*)((BYTE*)&In + Offset8) );
	CopyWithType<DWORD,Loops4>( *(DWORD*)((BYTE*)&Out + Offset4), *(DWORD*)((BYTE*)&In + Offset4) );
	CopyWithType<WORD, Loops2>( *(WORD*) ((BYTE*)&Out + Offset2 ),*(WORD*) ((BYTE*)&In + Offset2) );
	CopyWithType<BYTE, Loops1>( *((BYTE*)&Out + Offset1), *((BYTE*)&In + Offset1) );
}
template<typename T> inline void TRawData<T>::Move( T& To, T& From)
{
#if USES_SSE_INTRINSICS && !defined(__clang__)
	MoveWithType<__m128i,Loops16>( *(__m128i*)&To, *(__m128i*)&From);
#endif
	MoveWithType<QWORD,Loops8>( *(QWORD*)((BYTE*)&To + Offset8), *(QWORD*)((BYTE*)&From + Offset8) );
	MoveWithType<DWORD,Loops4>( *(DWORD*)((BYTE*)&To + Offset4), *(DWORD*)((BYTE*)&From + Offset4) );
	MoveWithType<WORD, Loops2>( *(WORD*) ((BYTE*)&To + Offset2 ),*(WORD*) ((BYTE*)&From + Offset2) );
	MoveWithType<BYTE, Loops1>( *((BYTE*)&To + Offset1), *((BYTE*)&From + Offset1) );
}
template<typename T> inline void TRawData<T>::Swap( T& To, T& From)
{
#if USES_SSE_INTRINSICS && !defined(__clang__)
	SwapWithType<__m128i,Loops16>( *(__m128i*)&To, *(__m128i*)&From);
#endif
	SwapWithType<QWORD,Loops8>( *(QWORD*)((BYTE*)&To + Offset8), *(QWORD*)((BYTE*)&From + Offset8) );
	SwapWithType<DWORD,Loops4>( *(DWORD*)((BYTE*)&To + Offset4), *(DWORD*)((BYTE*)&From + Offset4) );
	SwapWithType<WORD, Loops2>( *(WORD*) ((BYTE*)&To + Offset2 ),*(WORD*) ((BYTE*)&From + Offset2) );
	SwapWithType<BYTE, Loops1>( *((BYTE*)&To + Offset1), *((BYTE*)&From + Offset1) );
}
template<typename T> inline bool TRawData<T>::Equals( const T& A, const T& B)
{
	return 
#if USES_SSE_INTRINSICS && !defined(__clang__)
	EqualsWithType<__m128i,Loops16>( *(__m128i*)&A, *(__m128i*)&B) &&
#endif
	EqualsWithType<QWORD,Loops8>( *(QWORD*)((BYTE*)&A + Offset8), *(QWORD*)((BYTE*)&B + Offset8) ) &&
	EqualsWithType<DWORD,Loops4>( *(DWORD*)((BYTE*)&A + Offset4), *(DWORD*)((BYTE*)&B + Offset4) ) &&
	EqualsWithType<WORD, Loops2>( *(WORD*) ((BYTE*)&A + Offset2 ),*(WORD*) ((BYTE*)&B + Offset2) ) &&
	EqualsWithType<BYTE, Loops1>( *((BYTE*)&A + Offset1), *((BYTE*)&B + Offset1) );
}


/*-----------------------------------------------------------------------------
	Span.
-----------------------------------------------------------------------------*/

//
// Describes a sub-range of elements in a list
//
template <class T> class TSpan
{
public:
	T*  Start;
	INT Count;

	TSpan()
		: Start(0), Count(0)
	{}
	TSpan(ENoInit)
	{}
	TSpan( T* InStart, INT InCount )
		: Start(InStart), Count(InCount)
	{}

	T* FindItem( const T& Match )
	{
		for ( INT i=0; i<Count; i++)
			if ( Start[i] == Match )
				return Start + i;
		return nullptr;
	}

	INT FindItemIndex( const T& Match )
	{
		for ( INT i=0; i<Count; i++)
			if ( Start[i] == Match )
				return i;
		return INDEX_NONE;
	}

	T& operator[]( INT Index)                { return Start[Index]; }
	const T& operator[]( INT Index) const    { return Start[Index]; }
};


/*-----------------------------------------------------------------------------
	Allocators.
-----------------------------------------------------------------------------*/

template <class T> class TAllocator
{};


/*-----------------------------------------------------------------------------
	Simple lazy pointer.
-----------------------------------------------------------------------------*/

template <class T> class TLazyPtr
{
public:
	using ElementType = T;

	TLazyPtr() = default;
	~TLazyPtr()              { Reset(); }

	T* Get();
	void Reset();

	T& operator*()           { return *Get(); }
	T* operator->()          { return Get(); }
	operator bool() const    { return Ptr != nullptr; }

	bool IsNull() const      { return Ptr == nullptr; }

protected:
	T* Ptr = nullptr;
};


template<class T>
inline T* TLazyPtr<T>::Get()
{
	if ( !Ptr )
		Ptr = new T;
	return Ptr;
}

template<class T>
inline void TLazyPtr<T>::Reset()
{
	if ( Ptr != nullptr )
	{
		delete Ptr;
		Ptr = nullptr;
	}
}


/*-----------------------------------------------------------------------------
	Dynamic array template.
-----------------------------------------------------------------------------*/

//
// Base dynamic array.
//
class CORE_API FArray
{
public:
	void* GetData()
	{
		return Data;
	}
	const void* GetData() const
	{
		return Data;
	}
	UBOOL IsValidIndex( INT i ) const
	{
		return i>=0 && i<ArrayNum;
	}
	FORCEINLINE INT Num() const
	{
		//checkSlow(ArrayNum>=0);
		//checkSlow(ArrayMax>=ArrayNum);
		return ArrayNum;
	}
	FORCEINLINE INT Max() const
	{
		return ArrayMax;
	}
	void InsertZeroed( INT Index, INT Count, INT ElementSize )
	{
		guardSlow(FArray::InsertZeroed);
		// gam ---
		checkSlow(ElementSize>0);
		// sjs rem'd -trips all over- checkSlow(Count>0);
		checkSlow(Index>=0);
		checkSlow(Index<=ArrayNum);
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);
		// --- gam
		Insert( Index, Count, ElementSize );
		appMemzero( (BYTE*)Data+Index*ElementSize, Count*ElementSize );
		unguardSlow;
	}
	void Insert( INT Index, INT Count, INT ElementSize )
	{
		guardSlow(FArray::Insert);
		// gam ---
		checkSlow(ElementSize>0);
		// sjs rem'd -trips all over- checkSlow(Count>0);
		// --- gam
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);
		checkSlow(Index>=0);
		checkSlow(Index<=ArrayNum);

		INT OldNum = ArrayNum;
		if( (ArrayNum+=Count)>ArrayMax )
		{
			ArrayMax = ArrayNum + 3*ArrayNum/8 + 32;
			Realloc( ElementSize );
		}
		appMemmove
		(
			(BYTE*)Data + (Index+Count )*ElementSize,
			(BYTE*)Data + (Index       )*ElementSize,
			              (OldNum-Index)*ElementSize
		);

		unguardSlow;
	}
	INT AddNoCheck (INT Count)
	{
		INT Index = ArrayNum;
		ArrayNum += Count;
		return Index;
	}
	INT Add( INT Count, INT ElementSize )
	{
		guardSlow(FArray::Add);
		// gam ---
		// sjs rem'd -trips all over- checkSlow(Count>0);
		checkSlow(ElementSize>0);
		// --- gam
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);

		INT Index = ArrayNum;
		if( (ArrayNum+=Count)>ArrayMax )
		{
			ArrayMax = ArrayNum + 3*ArrayNum/8 + 32;
			Realloc( ElementSize );
		}

		return Index;
		unguardSlow;
	}
	INT AddZeroed( INT ElementSize, INT n=1 )
	{
		guardSlow(FArray::AddZeroed);
		// gam ---
		checkSlow(ElementSize>0);
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);
		// --- gam

		INT Index = Add( n, ElementSize );
		appMemzero( (BYTE*)Data+Index*ElementSize, n*ElementSize );
		return Index;
		unguardSlow;
	}
	void Shrink( INT ElementSize )
	{
		guardSlow(FArray::Shrink);
		checkSlow(ElementSize>0); // gam
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);
		if( ArrayMax != ArrayNum )
		{
			ArrayMax = ArrayNum;
			Realloc( ElementSize );
		}
		unguardSlow;
	}
	// sjs ---
	void SetSize( INT Count, INT ElementSize )
	{
		guardSlow(FArray::SetSize);
		// gam ---
		checkSlow(Count>=0);
		checkSlow(ElementSize>0);
		// --- gam
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);
		ArrayNum = Count;
		if (ArrayMax != Count)
		{
			ArrayMax = Count;
			Realloc( ElementSize );
		}
		unguardSlow;
	}
	// --- sjs
	void Empty( INT ElementSize, INT Slack=0 )
	{
		guardSlow(FArray::Empty);
		// gam ---
		checkSlow(ElementSize>0);
		checkSlow(Slack>=0);
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);
		// --- gam
		ArrayNum = 0;
		if (ArrayMax != Slack)
		{
			ArrayMax = Slack;
			Realloc( ElementSize );
		}
		unguardSlow;
	}
	FArray()
	:	Data	( NULL )
	,	ArrayNum( 0 )
	,	ArrayMax( 0 )
	{}
	FArray( ENoInit )
	{}
	~FArray()
	{
		guardSlow(FArray::~FArray);
		if( Data )
			appFree( Data );
		Data = NULL;
		ArrayNum = ArrayMax = 0;
		unguardSlow;
	}
	void CountBytes( FArchive& Ar, INT ElementSize )
	{
		guardSlow(FArray::CountBytes);
		Ar.CountBytes( ArrayNum*ElementSize, ArrayMax*ElementSize );
		unguardSlow;
	}
	void Remove( INT Index, INT Count, INT ElementSize );
	void RemoveNoRealloc( INT Index, INT Count, INT ElementSize );
protected:
	void Realloc( INT ElementSize );
	FArray( INT InNum, INT ElementSize )
	:	Data    ( NULL  )
	,	ArrayNum( InNum )
	,	ArrayMax( InNum )
	{
		if (InNum > 0)
			Realloc( ElementSize );
	}
	void* Data;
	INT	  ArrayNum;
	INT	  ArrayMax;
};

//
// Templated dynamic array.
//
template< class T > class TArray : public FArray
{
public:
	typedef T ElementType;
	TArray()
	:	FArray()
	{}
	TArray( INT InNum )
	:	FArray( InNum, sizeof(T) )
	{}
	TArray( const TArray& Other )
	:	FArray( Other.ArrayNum, sizeof(T) )
	{
		guardSlow(TArray::copyctor);
		if( TTypeInfo<T>::NeedsDestructor() )
		{
			ArrayNum=0;
			for( INT i=0; i<Other.ArrayNum; i++ )
				new(*this)T(Other(i));
		}
		else if( sizeof(T)!=1 )
		{
			for( INT i=0; i<ArrayNum; i++ )
				(*this)(i) = Other(i);
		}
		else if ( ArrayNum > 0 )
		{
			appMemcpy( &(*this)(0), &Other(0), ArrayNum * sizeof(T) );
		}
		unguardSlow;
	}
	TArray( TArray&& Other )
	{
		TRawData<TArray>::Move(*this, Other);
	}
	TArray( ENoInit )
	: FArray( E_NoInit )
	{}
	~TArray()
	{
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);
		if( TTypeInfo<T>::NeedsDestructor() )
			for( INT i=0; i<ArrayNum; i++ )
				(&(*this)(i))->~T();
	}
    T& operator()( INT i )
	{
		guardSlow(TArray::operator());
		checkSlow(i>=0);
		checkSlow(i<=ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		checkSlow(ArrayMax>0);
		checkSlow(Data);
		return ((T*)Data)[i];
		unguardSlow;
	}
	const T& operator()( INT i ) const
	{
		guardSlow(TArray::operator());
		checkSlow(i>=0);
		checkSlow(i<=ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		checkSlow(ArrayMax>0);
		checkSlow(Data);
		return ((T*)Data)[i];
		unguardSlow;
	}
	T Pop()
	{
		guardSlow(TArray::Pop);
		check(ArrayNum>0);
		checkSlow(ArrayMax>=ArrayNum);
		T Result = ((T*)Data)[ArrayNum-1];
		Remove( ArrayNum-1 );
		return Result;
		unguardSlow;
	}
	T& Last( INT c=0 )
	{
		guardSlow(TArray::Last);
		check(c<ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		return ((T*)Data)[ArrayNum-c-1];
		unguardSlow;
	}
	const T& Last( INT c=0 ) const
	{
		guardSlow(TArray::Last);
		checkSlow(c<ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		return ((T*)Data)[ArrayNum-c-1];
		unguardSlow;
	}
	void Shrink()
	{
		guardSlow(TArray::Shrink);
		FArray::Shrink( sizeof(T) );
		unguardSlow;
	}
	UBOOL FindItem( const T& Item, INT& Index ) const
	{
		guardSlow(TArray::FindItem);
		for( Index=0; Index<ArrayNum; Index++ )
			if( (*this)(Index)==Item )
				return 1;
		return 0;
		unguardSlow;
	}
	INT FindItemIndex( const T& Item ) const
	{
		guardSlow(TArray::FindItemIndex);
		for( INT Index=0; Index<ArrayNum; Index++ )
			if( (*this)(Index)==Item )
				return Index;
		return INDEX_NONE;
		unguardSlow;
	}
	friend FArchive& operator<<( FArchive& Ar, TArray& A )
	{
		guard(TArray<<);
		A.CountBytes( Ar );
		if( sizeof(T)==1 )
		{
			// Serialize simple bytes which require no construction or destruction.
			Ar << AR_INDEX(A.ArrayNum);
			if( Ar.IsLoading() )
			{
				if (A.ArrayMax != A.ArrayNum)
				{
					A.ArrayMax = A.ArrayNum;
					A.Realloc( sizeof(T) );
				}
			}
            // gam ---
			if( A.Num() > 0 )
			Ar.Serialize( &A(0), A.Num() );
			else
				Ar.Serialize( NULL, 0 );
            // --- gam
		}
		else if( Ar.IsLoading() )
		{
			// Load array.
			INT NewNum = 0; // gam
			Ar << AR_INDEX(NewNum);
			A.Empty( NewNum );
			for( INT i=0; i<NewNum; i++ )
				Ar << *new(A)T;
		}
		else
		{
			// Save array.
			Ar << AR_INDEX(A.ArrayNum);
			for( INT i=0; i<A.ArrayNum; i++ )
				Ar << A( i );
		}
		return Ar;
		unguard;
	}
	void CountBytes( FArchive& Ar )
	{
		guardSlow(TArray::CountBytes);
		FArray::CountBytes( Ar, sizeof(T) );
		unguardSlow;
	}

	// Add, Insert, Remove, Empty interface.
	INT Add( INT n=1 )
	{
		guardSlow(TArray::Add);
		checkSlow(!TTypeInfo<T>::DefinitelyNeedsDestructor());
		return FArray::Add( n, sizeof(T) );
		unguardSlow;
	}
	// stijn: used in Render - only to be used if you've preallocated enough bytes (e.g., with Reserve)
	INT AddNoCheck (INT n=1)
	{
		return FArray::AddNoCheck(n);
	}
	// sjs ---
	void SetSize( INT n=1 )
	{
		guardSlow(TArray::SetSize);
		checkSlow(!TTypeInfo<T>::DefinitelyNeedsDestructor());
		FArray::SetSize( n, sizeof(T) );
		unguardSlow;
	}
	// --- sjs
	void Insert( INT Index, INT Count=1 )
	{
		guardSlow(TArray::Insert);
		checkSlow(!TTypeInfo<T>::DefinitelyNeedsDestructor());
		FArray::Insert( Index, Count, sizeof(T) );
		unguardSlow;
	}
	void InsertZeroed( INT Index, INT Count=1 )
	{
		guardSlow(TArray::InsertZeroed);
		checkSlow(!TTypeInfo<T>::DefinitelyNeedsDestructor());
		FArray::InsertZeroed( Index, Count, sizeof(T) );
		unguardSlow;
	}
	void Remove( INT Index, INT Count=1 )
	{
		guardSlow(TArray::Remove);
		check(Index>=0);
		check(Index<=ArrayNum);
		check(Index+Count<=ArrayNum);
		if( TTypeInfo<T>::NeedsDestructor() )
			for( INT i=Index; i<Index+Count; i++ )
				(&(*this)(i))->~T();
		FArray::Remove( Index, Count, sizeof(T) );
		unguardSlow;
	}
	void RemoveNoRealloc( INT Index, INT Count=1 )
	{
		guardSlow(TArray::RemoveNoRealloc);
		check(Index>=0);
		check(Index<=ArrayNum);
		check(Index+Count<=ArrayNum);
		if( TTypeInfo<T>::NeedsDestructor() )
			for( INT i=Index; i<Index+Count; i++ )
				(&(*this)(i))->~T();
		FArray::RemoveNoRealloc( Index, Count, sizeof(T) );
		unguardSlow;
	}
	void Empty( INT Slack=0 )
	{
		guardSlow(TArray::Empty);
		if( TTypeInfo<T>::NeedsDestructor() )
			for( INT i=0; i<ArrayNum; i++ )
				(&(*this)(i))->~T();
		FArray::Empty( sizeof(T), Slack );
		unguardSlow;
	}

	// Functions dependent on Add, Remove.
	TArray& operator+( const TArray& Other )
	{
		guardSlow(TArray::operator=);
		if( this != &Other )
		{
			for( INT i=0; i<Other.ArrayNum; i++ )
				new( *this )T( Other(i) );
		}
		return *this;
		unguardSlow;
	}
	TArray& operator+=( const TArray& Other )
	{
		guardSlow(TArray::operator=);
		if( this != &Other )
		{
			*this = *this + Other;
		}
		return *this;
		unguardSlow;
	}
	TArray& operator=( const TArray& Other )
	{
		guardSlow(TArray::operator=);
		if( this != &Other )
		{
			Empty( Other.ArrayNum );
			if ( Other.ArrayNum )
			{
				Reserve( Other.ArrayNum + 1);
				for( INT i=0; i<Other.ArrayNum; i++ )
					new( *this )T( Other(i) );
			}
		}
		return *this;
		unguardSlow;
	}
	TArray& operator=( TArray&& Other )
	{
		if ( this != &Other )
		{
			if ( Data )
				Empty();
			TRawData<TArray>::Move(*this, Other);
		}
		return *this;
	}
	INT AddItem( const T& Item )
	{
		guardSlow(TArray::AddItem);
		checkSlow(!TTypeInfo<T>::DefinitelyNeedsDestructor());
		new(*this) T(Item);
		return Num() - 1;
		unguardSlow;
	}
	INT AddZeroed( INT n=1 )
	{
		guardSlow(TArray::AddZeroed);
		return FArray::AddZeroed( sizeof(T), n );
		unguardSlow;
	}
	INT AddUniqueItem( const T& Item )
	{
		guardSlow(TArray::AddUniqueItem);
		checkSlow(!TTypeInfo<T>::DefinitelyNeedsDestructor());
		for( INT Index=0; Index<ArrayNum; Index++ )
			if( (*this)(Index)==Item )
				return Index;
		return AddItem( Item );
		unguardSlow;
	}
	INT RemoveItem( const T& Item )
	{
		guardSlow(TArray::RemoveItem);
		INT OriginalNum=ArrayNum;
		for( INT Index=0; Index<ArrayNum; Index++ )
			if( (*this)(Index)==Item )
				Remove( Index-- );
		return OriginalNum - ArrayNum;
		unguardSlow;
	}
	void InsertItem(INT Index, const T& Item)
	{
		guardSlow(TArray::InsertItem);
		Insert(Index);
		(*this)(Index) = Item;
		unguardSlow;
	}
	// stijn: used in Render
	void EmptyNoRealloc()
	{
		ArrayNum = 0;
	}
	// stijn: used in Render
	void Reserve(INT NewArrayMax)
	{
		if (NewArrayMax > ArrayMax)
		{
			ArrayMax = NewArrayMax;
			Realloc(sizeof(T));
		}
	}

	// Iterator.
	class TIterator
	{
	public:
		TIterator( TArray<T>& InArray ) : Array(InArray), Index(-1) { ++*this;      }
		void operator++()      { ++Index;                                           }
		void RemoveCurrent()   { Array.Remove(Index--); }
		INT GetIndex()   const { return Index;                                      }
		operator UBOOL() const { return Index < Array.Num();                        }
		T& operator*()   const { return Array(Index);                               }
		T* operator->()  const { return &Array(Index);                              }
		T& GetCurrent()  const { return Array( Index );                             }
		T& GetPrev()     const { return Array( Index ? Index-1 : Array.Num()-1 );   }
		T& GetNext()     const { return Array( Index<Array.Num()-1 ? Index+1 : 0 ); }
	private:
		TArray<T>& Array;
		INT Index;
	};
};

template<class T> class TArrayNoInit : public TArray<T>
{
public:
	TArrayNoInit()
		: TArray<T>(E_NoInit)
	{}
	TArrayNoInit &operator=( const TArrayNoInit& Other )
	{
		TArray<T>::operator=(Other);
		return *this;
	}
};

//
// Array operator news.
//
template <class T> void* operator new( size_t Size, TArray<T>& Array )
{
	INT Index = Array.FArray::Add(1,sizeof(T));
	return &Array(Index);
}
template <class T> void* operator new( size_t Size, TArray<T>& Array, INT Index )
{
	Array.FArray::Insert(Index,1,sizeof(T));
	return &Array(Index);
}

//
// Array exchanger.
//
template <class T> inline void ExchangeArray( TArray<T>& A, TArray<T>& B )
{
	guardSlow(ExchangeTArray);
	appMemswap( &A, &B, sizeof(FArray) );
	unguardSlow;
}

/*-----------------------------------------------------------------------------
	Transactional array.
-----------------------------------------------------------------------------*/

template< class T > class TTransArray : public TArray<T>
{
public:
	// Constructors.
	TTransArray( UObject* InOwner, INT InNum=0 )
	:	TArray<T>( InNum )
	,	Owner( InOwner )
	{
		checkSlow(Owner);
	}
	TTransArray( UObject* InOwner, const TArray<T>& Other )
	:	TArray<T>( Other )
	,	Owner( InOwner )
	{
		checkSlow(Owner);
	}
	TTransArray& operator=( const TTransArray& Other )
	{
		operator=( (const TArray<T>&)Other );
		return *this;
	}

	// Add, Insert, Remove, Empty interface.
	T Pop()
	{
		guardSlow(TTransArray::Pop);
		if (GUndo)
			GUndo->SaveArray(Owner, this, TArray<T>::ArrayNum - 1, 1, -1, sizeof(T), SerializeItem, DestructItem);
		return TArray<T>::Pop();
		unguardSlow;
	}
	INT Add( INT Count=1 )
	{
		guardSlow(TTransArray::Add);
		INT Index = TArray<T>::Add( Count );
		if( GUndo )
			GUndo->SaveArray( Owner, this, Index, Count, 1, sizeof(T), SerializeItem, DestructItem );
		return Index;
		unguardSlow;
	}
	INT AddNoCheck( INT Count=1 )
	{
		guardSlow(TTransArray::AddNoCheck);
		INT Index = TArray<T>::AddNoCheck( Count );
		if( GUndo )
			GUndo->SaveArray( Owner, this, Index, Count, 1, sizeof(T), SerializeItem, DestructItem );
		return Index;
		unguardSlow;
	}
	void Insert( INT Index, INT Count=1 )
	{
		guardSlow(TTransArray::Insert);
		FArray::Insert( Index, Count, sizeof(T) );
		if( GUndo )
			GUndo->SaveArray( Owner, this, Index, Count, 1, sizeof(T), SerializeItem, DestructItem );
		unguardSlow;
	}
	void InsertZeroed( INT Index, INT Count=1 )
	{
		guardSlow(TTransArray::InsertZeroed);
		FArray::InsertZeroed( Index, Count, sizeof(T) );
		if( GUndo )
			GUndo->SaveArray( Owner, this, Index, Count, 1, sizeof(T), SerializeItem, DestructItem );
		unguardSlow;
	}
	void SetSize(INT n = 1)
	{
		guardSlow(TTransArray::SetSize);
		INT Num = TArray<T>::ArrayNum;
		if (GUndo && Num > n)
			GUndo->SaveArray(Owner, this, n, Num - n, -1, sizeof(T), SerializeItem, DestructItem);
		TArray<T>::SetSize(n);
		if (GUndo && Num < n)
			GUndo->SaveArray(Owner, this, Num, n - Num, 1, sizeof(T), SerializeItem, DestructItem);
		unguardSlow;
	}
	void Remove( INT Index, INT Count=1 )
	{
		guardSlow(TTransArray::Remove);
		if( GUndo )
			GUndo->SaveArray( Owner, this, Index, Count, -1, sizeof(T), SerializeItem, DestructItem );
		TArray<T>::Remove( Index, Count );
		unguardSlow;
	}
	void RemoveNoRealloc( INT Index, INT Count=1 )
	{
		guardSlow(TTransArray::RemoveNoRealloc);
		if( GUndo )
			GUndo->SaveArray( Owner, this, Index, Count, -1, sizeof(T), SerializeItem, DestructItem );
		TArray<T>::RemoveNoRealloc( Index, Count );
		unguardSlow;
	}
	void Empty( INT Slack=0 )
	{
		guardSlow(TTransArray::Empty);
		if( GUndo )
			GUndo->SaveArray( Owner, this, 0, this->ArrayNum, -1, sizeof(T), SerializeItem, DestructItem );
		TArray<T>::Empty( Slack );
		unguardSlow;
	}
	void EmptyNoRealloc()
	{
		guardSlow(TTransArray::EmptyNoRealloc);
		if( GUndo )
			GUndo->SaveArray( Owner, this, 0, this->ArrayNum, -1, sizeof(T), SerializeItem, DestructItem );
		TArray<T>::EmptyNoRealloc();
		unguardSlow;
	}

	// Functions dependent on Add, Remove.
	TTransArray& operator=( const TArray<T>& Other )
	{
		guardSlow(TTransArray::operator=);
		if( this != &Other )
		{
			Empty( Other.Num() );
			for( INT i=0; i<Other.Num(); i++ )
				new( *this )T( Other(i) );
		}
		return *this;
		unguardSlow;
	}
	INT AddItem( const T& Item )
	{
		guardSlow(TTransArray::AddItem);
		checkSlow(!TTypeInfo<T>::DefinitelyNeedsDestructor());
		new(*this) T(Item);
		return TArray<T>::Num() - 1;
		unguardSlow;
	}
	INT AddZeroed( INT n=1 )
	{
		guardSlow(TTransArray::AddZeroed);
		INT Index = Add(n);
		if (!DO_GUARD_SLOW ||  n > 0)
		appMemzero( &(*this)(Index), n*sizeof(T) );
		return Index;
		unguardSlow;
	}
	INT AddUniqueItem( const T& Item )
	{
		guardSlow(TTransArray::AddUniqueItem);
		for( INT Index=0; Index<this->ArrayNum; Index++ )
			if( (*this)(Index)==Item )
				return Index;
		return AddItem( Item );
		unguardSlow;
	}
	INT RemoveItem( const T& Item )
	{
		guardSlow(TTransArray::RemoveItem);
		INT OriginalNum=this->ArrayNum;
		for( INT Index=0; Index<this->ArrayNum; Index++ )
			if( (*this)(Index)==Item )
				Remove( Index-- );
		return OriginalNum - this->ArrayNum;
		unguardSlow;
	}
	void InsertItem(INT Index, const T& Item)
	{
		guardSlow(TTransArray::InsertItem);
		Insert(Index);
		(*this)(Index) = Item;
		unguardSlow;
	}

	// TTransArray interface.
	UObject* GetOwner()
	{
		return Owner;
	}
	void ModifyItem( INT Index, INT Count = 1 )
	{
		guardSlow(TTransArray::ModifyItem);
		Count = Min(Count, this->Num() - Index);
		if( GUndo && Count > 0 )
			GUndo->SaveArray( Owner, this, Index, Count, 0, sizeof(T), SerializeItem, DestructItem );
		unguardSlow;
	}
	void ModifyAllItems()
	{
		guardSlow(TTransArray::ModifyAllItems);
		if( GUndo )
			GUndo->SaveArray( Owner, this, 0, this->Num(), 0, sizeof(T), SerializeItem, DestructItem );
		unguardSlow;
	}
	friend FArchive& operator<<( FArchive& Ar, TTransArray& A )
	{
		guard(TTransArray<<);
		if( !Ar.IsTrans() )
			Ar << (TArray<T>&)A;
		return Ar;
		unguard;
	}
protected:
	static void SerializeItem( FArchive& Ar, void* TPtr )
	{
		guardSlow(TArray::SerializeItem);
		Ar << *(T*)TPtr;
		unguardSlow;
	}
	static void DestructItem( void* TPtr )
	{
		guardSlow(TArray::SerializeItem);
		((T*)TPtr)->~T();
		unguardSlow;
	}
	UObject* Owner;
private:

	// Disallow the copy constructor.
	TTransArray( const TArray<T>& Other )
	{}
};

//
// Transactional array operator news.
//
template <class T> void* operator new( size_t Size, TTransArray<T>& Array )
{
	guardSlow(TArray::operator new);
	INT Index = Array.Add();
	return &Array(Index);
	unguardSlow;
}
template <class T> void* operator new( size_t Size, TTransArray<T>& Array, INT Index )
{
	guardSlow(TArray::operator new);
	Array.Insert(Index);
	return &Array(Index);
	unguardSlow;
}

/*-----------------------------------------------------------------------------
	Lazy loading.
-----------------------------------------------------------------------------*/

//
// Lazy loader base class.
//
class FLazyLoader
{
	friend class ULinkerLoad;
protected:
	FArchive*	 SavedAr;
	INT          SavedPos;
public:
	FLazyLoader()
	: SavedAr( NULL )
	, SavedPos( 0 )
	{}
	virtual void Load()=0;
	virtual void Unload()=0;
};

//
// Lazy-loadable dynamic array.
//
template <class T> class TLazyArray : public TArray<T>, public FLazyLoader
{
public:
	TLazyArray( INT InNum=0 )
	: TArray<T>( InNum )
	, FLazyLoader()
	{}
	~TLazyArray() noexcept(false)
	{
		guard(TLazyArray::~TLazyArray);
		if( SavedAr )
			SavedAr->DetachLazyLoader( this );
		unguard;
	}
#if LOAD_ON_DEMAND /* Breaks because of untimely accesses of operator() !! */
    T& operator()( INT i )
	{
		guardSlow(TArray::operator());
		checkSlow(i>=0);
		checkSlow(i<=ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		if( SavedPos>0 )
			Load();
		return ((T*)Data)[i];
		unguardSlow;
	}
	const T& operator()( INT i ) const
	{
		guardSlow(TArray::operator());
		checkSlow(i>=0);
		checkSlow(i<=ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		if( SavedPos>0 )
			Load();
		return ((T*)Data)[i];
		unguardSlow;
	}
#endif
	void Load()
	{
		// Make sure this array is loaded.
		guardSlow(TLazyArray::Load);
		if( SavedPos>0 )
		{
			// Lazy load it now.
			INT PushedPos = SavedAr->Tell();
			SavedAr->Seek( SavedPos );
			*SavedAr << (TArray<T>&)*this;
			SavedPos *= -1;
			SavedAr->Seek( PushedPos );
		}
		unguardSlow;
	}
	void DetachLazyLoader()
	{
		if( SavedAr )
			SavedAr->DetachLazyLoader( this );
		SavedPos = 0;
		SavedAr = NULL;
	}
	void Unload()
#if __GNUG__
		;	// Function declaration.
#else
	{
		// Make sure this array is unloaded.
		guard(TLazyArray::Unload);
		if( SavedPos<0 )
		{
			// Unload it now.
			TArray<T>::Empty();
			SavedPos *= -1;
		}
		unguard;
	}
#endif
	friend FArchive& operator<<( FArchive& Ar, TLazyArray& This )
	{
		guard(TLazyArray<<);
		if( Ar.IsLoading() )
		{
			INT SeekPos=0;
			if( Ar.Ver() <= 61 )
			{
				//oldver: Handles dynamic arrays of fixed-length serialized items only.
				Ar.AttachLazyLoader( &This );
				INT SkipCount = 0; // gam
				Ar << AR_INDEX(SkipCount);
				SeekPos = Ar.Tell() + SkipCount*sizeof(T);
					
			}
			else
			{
				Ar << SeekPos;
				Ar.AttachLazyLoader( &This );
			}
			if( !GLazyLoad )
				This.Load();
			Ar.Seek( SeekPos );
		}
		else if( Ar.IsSaving() && Ar.Ver()>61 )
		{
			// Save out count for skipping over this data.
			INT CountPos = Ar.Tell();
			Ar << CountPos << (TArray<T>&)This;
			INT EndPos = Ar.Tell();
			Ar.Seek( CountPos );
			Ar << EndPos;
			Ar.Seek( EndPos );
		}
		else Ar << (TArray<T>&)This;
		return Ar;
		unguard;
	}
};
#if __GNUG__
template <class T> void TLazyArray<T>::Unload()
{
	// Make sure this array is unloaded.
	 guard(TLazyArray::Unload);
	 if( SavedPos<0 && GLazyLoad )
	 {
		// Unload it now.
		this->Empty();
		SavedPos *= -1;
	 }
	 unguard;
}
#endif

/*-----------------------------------------------------------------------------
	Dynamic strings.
-----------------------------------------------------------------------------*/

#include "UnString.h"


inline DWORD GetTypeHash( const FString& S )
{
	return appStrihash(*S);
}
template <> struct TTypeInfo<FString> : public TTypeInfoBase<FString>
{
	typedef const TCHAR* ConstInitType;
	static const TCHAR* ToInit( const FString& In ) {return *In;}
	static UBOOL DefinitelyNeedsDestructor() {return 0;}
};

//
// String exchanger.
//
inline void ExchangeString( FString& A, FString& B )
{
	guardSlow(ExchangeTArray);
	appMemswap( &A, &B, sizeof(FString) );
	unguardSlow;
}

/*----------------------------------------------------------------------------
	Special archivers.
----------------------------------------------------------------------------*/

//
// String output device.
//
class FStringOutputDevice : public FString, public FOutputDevice
{
public:
	FStringOutputDevice( const TCHAR* InStr=TEXT("") )
	: FString( InStr )
	{}
	void Serialize( const TCHAR* Data, EName Event )
	{
		*this += (TCHAR*)Data;
	}
};

//
// Buffer writer.
//
class FBufferWriter : public FArchive
{
public:
	FBufferWriter( TArray<BYTE>& InBytes )
	: Bytes( InBytes )
	, Pos( 0 )
	{
		ArIsSaving = 1;
	}
	void Serialize( void* InData, INT Length )
	{
		if( Pos+Length>Bytes.Num() )
			Bytes.Add( Pos+Length-Bytes.Num() );
		if( Length == 1 )
			Bytes(Pos) = ((BYTE*)InData)[0];
		else
			appMemcpy( &Bytes(Pos), InData, Length );
		Pos += Length;
	}
	INT Tell()
	{
		return Pos;
	}
	void Seek( INT InPos )
	{
		Pos = InPos;
	}
	INT TotalSize()
	{
		return Bytes.Num();
	}
	void SetError()
	{
		ArIsError = 1;
	}
private:
	TArray<BYTE>& Bytes;
	INT Pos;
};

//
// Buffer archiver.
//
class FBufferArchive : public FBufferWriter, public TArray<BYTE>
{
public:
	FBufferArchive()
	: FBufferWriter( (TArray<BYTE>&)*this )
	{}
};

//
// Buffer reader.
//
class CORE_API FBufferReader : public FArchive
{
public:
	FBufferReader( const TArray<BYTE>& InBytes )
	:	Bytes	( InBytes )
	,	Pos 	( 0 )
	{
		ArIsLoading = ArIsTrans = 1;
	}
	void Serialize( void* Data, INT Num )
	{
		check(Pos>=0);
		check(Pos+Num<=Bytes.Num());
		if( Num == 1 )
			((BYTE*)Data)[0] = Bytes(Pos);
		else
			appMemcpy( Data, &Bytes(Pos), Num );
		Pos += Num;
	}
	INT Tell()
	{
		return Pos;
	}
	INT TotalSize()
	{
		return Bytes.Num();
	}
	void Seek( INT InPos )
	{
		check(InPos>=0);
		check(InPos<=Bytes.Num());
		Pos = InPos;
	}
	UBOOL AtEnd()
	{
		return Pos>=Bytes.Num();
	}
private:
	const TArray<BYTE>& Bytes;
	INT Pos;
};

/*----------------------------------------------------------------------------
	TMap.
----------------------------------------------------------------------------*/

//
// Maps unique keys to values.
//
template< class TK, class TI > class TMapBase
{
public:
	static constexpr INT InitialHashCount = 8; // Higor: make this a template parameter (use a traits class?)

protected:
	class TPair
	{
	public:
		INT HashNext;
		TK Key;
		TI Value;
		TPair( typename TTypeInfo<TK>::ConstInitType InKey, typename TTypeInfo<TI>::ConstInitType InValue )
		: Key( InKey ), Value( InValue )
		{}
		TPair()
		{}
		friend FArchive& operator<<( FArchive& Ar, TPair& F )
		{
			guardSlow(TMapBase::TPair<<);
			return Ar << F.Key << F.Value;
			unguardSlow;
		}
	};
	void Rehash()
	{
		guardSlow(TMapBase::Rehash);
		checkSlow(!(HashCount&(HashCount-1)));
		checkSlow(HashCount>=InitialHashCount);
		INT* NewHash = new INT[HashCount];
		{for( INT i=0; i<HashCount; i++ )
		{
			NewHash[i] = INDEX_NONE;
		}}
		{for( INT i=0; i<Pairs.Num(); i++ )
		{
			TPair& Pair    = Pairs(i);
			INT    iHash   = (GetTypeHash(Pair.Key) & (HashCount-1));
			Pair.HashNext  = NewHash[iHash];
			NewHash[iHash] = i;
		}}
		if( Hash )
			delete[] Hash;
		Hash = NewHash;
		unguardSlow;
	}
	void Rehash( INT NewHashCount)
	{
		guardSlow(TMapBase::RehashNew);
		checkSlow(!(NewHashCount&(NewHashCount-1)));
		checkSlow(NewHashCount>=InitialHashCount);
		INT* NewHash = ((NewHashCount != HashCount) || !Hash) ? new INT[NewHashCount] : Hash;
		for( INT i=0; i<NewHashCount; i++ )
			NewHash[i] = INDEX_NONE;
		for( INT i=0; i<Pairs.Num(); i++ )
		{
			TPair& Pair    = Pairs(i);
			INT    iHash   = (GetTypeHash(Pair.Key) & (NewHashCount-1));
			Pair.HashNext  = NewHash[iHash];
			NewHash[iHash] = i;
		}
		if ( NewHash != Hash )
		{	
			if ( Hash )
				delete[] Hash;
			Hash = NewHash;
		}
		HashCount = NewHashCount;
		unguardSlow;
	}
	void Unhash( INT PairIndex)
	{
		guardSlow(TMapBase::Unhash);
		INT HashRemove = GetTypeHash(Pairs(PairIndex).Key) & (HashCount-1);
		for ( INT* HashNextPtr=&Hash[HashRemove]; *HashNextPtr != INDEX_NONE; HashNextPtr=&Pairs(*HashNextPtr).HashNext)
			if ( *HashNextPtr == PairIndex )
			{
				*HashNextPtr = Pairs(PairIndex).HashNext;
				break;
			}
		unguardSlow;
	}
	void OffsetHash( INT FirstPair, INT Offset)
	{
		guardSlow(TMapBase::Unhash);
		for ( INT i=0; i<HashCount; i++)
			if ( Hash[i] >= FirstPair )
				Hash[i] += Offset;
		for ( INT i=0; i<Pairs.Num(); i++)
			if ( Pairs(i).HashNext >= FirstPair )
				Pairs(i).HashNext += Offset;
		unguardSlow;
	}
	void Relax()
	{
		guardSlow(TMapBase::Relax);
		INT NewHashCount = HashCount;
		while ( NewHashCount>Pairs.Num()*2+InitialHashCount )
			NewHashCount /= 2;
		Rehash(NewHashCount);
		unguardSlow;
	}
	TI& Add( typename TTypeInfo<TK>::ConstInitType InKey, typename TTypeInfo<TI>::ConstInitType InValue )
	{
		guardSlow(TMapBase::Add);
		TPair& Pair   = *new(Pairs)TPair( InKey, InValue );
		INT    iHash  = (GetTypeHash(Pair.Key) & (HashCount-1));
		Pair.HashNext = Hash[iHash];
		Hash[iHash]   = Pairs.Num()-1;
		if( HashCount*2+InitialHashCount < Pairs.Num() )
		{
			HashCount *= 2;
			Rehash();
		}
		return Pair.Value;
		unguardSlow;
	}
	TArray<TPair> Pairs;
	INT* Hash;
	INT HashCount;
public:
	TMapBase()
	:	Hash( NULL )
	,	HashCount( InitialHashCount )
	{
		guardSlow(TMapBase::TMapBase);
		Rehash();
		unguardSlow;
	}
	TMapBase( const TMapBase& Other )
	:	Pairs( Other.Pairs )
	,	Hash( NULL )
	,	HashCount( Other.HashCount )
	{
		guardSlow(TMapBase::TMapBase copy);
		Rehash();
		unguardSlow;
	}
	~TMapBase()
	{
		guardSlow(TMapBase::~TMapBase);
		if( Hash )
			delete[] Hash;
		Hash = NULL;
		HashCount = 0;
		unguardSlow;
	}
	TMapBase& operator=( const TMapBase& Other )
	{
		guardSlow(TMapBase::operator=);
		Pairs     = Other.Pairs;
		Rehash(Other.HashCount);
		return *this;
		unguardSlow;
	}
	void Empty()
	{
		guardSlow(TMapBase::Empty);
		checkSlow(!(HashCount&(HashCount-1)));
		Pairs.Empty();
		Rehash(InitialHashCount);
		unguardSlow;
	}
	TI& Set( typename TTypeInfo<TK>::ConstInitType InKey, typename TTypeInfo<TI>::ConstInitType InValue )
	{
		guardSlow(TMap::Set);
		for( INT i=Hash[(GetTypeHash(InKey) & (HashCount-1))]; i!=INDEX_NONE; i=Pairs(i).HashNext )
			if( Pairs(i).Key==InKey )
				{Pairs(i).Value=InValue; return Pairs(i).Value;}
		return Add( InKey, InValue );
		unguardSlow;
	}
	INT Remove( typename TTypeInfo<TK>::ConstInitType InKey )
	{
		guardSlow(TMapBase::Remove);
		INT Count=0;
		for( INT i=Pairs.Num()-1; i>=0; i-- )
			if( Pairs(i).Key==InKey )
				{Pairs.Remove(i); Count++;}
		if( Count )
			Relax();
		return Count;
		unguardSlow;
	}
	TI* Find( const TK& Key )
	{
		guardSlow(TMapBase::Find);
		for( INT i=Hash[(GetTypeHash(Key) & (HashCount-1))]; i!=INDEX_NONE; i=Pairs(i).HashNext )
			if( Pairs(i).Key==Key )
				return &Pairs(i).Value;
		return NULL;
		unguardSlow;
	}
	TI FindRef( const TK& Key )
	{
		guardSlow(TMapBase::Find);
		for( INT i=Hash[(GetTypeHash(Key) & (HashCount-1))]; i!=INDEX_NONE; i=Pairs(i).HashNext )
			if( Pairs(i).Key==Key )
				return Pairs(i).Value;
		return NULL;
		unguardSlow;
	}
	const TI* Find( const TK& Key ) const
	{
		guardSlow(TMapBase::Find);
		for( INT i=Hash[(GetTypeHash(Key) & (HashCount-1))]; i!=INDEX_NONE; i=Pairs(i).HashNext )
			if( Pairs(i).Key==Key )
				return &Pairs(i).Value;
		return NULL;
		unguardSlow;
	}
	friend FArchive& operator<<( FArchive& Ar, TMapBase& M )
	{
		guardSlow(TMapBase<<);
		Ar << M.Pairs;
		if( Ar.IsLoading() )
		{
			if ( M.Pairs.Num() > M.HashCount )
				M.HashCount = Min(1 << ((INT)appCeilLogTwo(M.Pairs.Num())), 64 * 1024);
			M.Rehash();
		}
		return Ar;
		unguardSlow;
	}
	void Dump( FOutputDevice& Ar )
	{
		guard(TMapBase::Dump);
		// Give some more useful statistics. --han
		INT NonEmpty = 0, Worst = 0;
		for (INT i = 0; i < HashCount; i++)
		{
			INT c = 0;
			for (INT j = Hash[i]; j != INDEX_NONE; j = Pairs(j).HashNext)
				c++;
			if (c > Worst)
				Worst = c;
			if (c > 0)
			{
				NonEmpty++;
				Ar.Logf(TEXT("   Hash[%i] = %i"), i, c);
			}
		}
		Ar.Logf(TEXT("TMapBase: %i items, worst %i, %i/%i hash slots used."), Pairs.Num(), Worst, NonEmpty, HashCount);
		unguard;
	}
	class TIterator
	{
	public:
		TIterator(TMapBase<TK, TI>& InMap) : Map(InMap), Pairs(InMap.Pairs), Index(0) {}
		void operator++() { ++Index; }
		void Increment() { ++Index; }
		void RemoveCurrent()
		{
			Map.Unhash(Index);
			Pairs.Remove(Index);
			Map.OffsetHash(Index, -1);
			Index--;
		}
		operator UBOOL() const { return Index < Pairs.Num(); }
		TK& Key() const { return Pairs(Index).Key; }
		TI& Value() const { return Pairs(Index).Value; }
	private:
		TMapBase<TK, TI>& Map;
		TArray<TPair>& Pairs;
		INT Index;
		friend class TUniqueKeyIterator;
	};
	class TUniqueKeyIterator : public TMapBase<TK, TI>::TIterator
	{
	public:
		TUniqueKeyIterator(TMapBase<TK, TI>& InMap)
			: TMapBase<TK, TI>::TIterator(InMap)
		{
			if (*this)
				VisitedKeys.AddUniqueItem(TMapBase<TK, TI>::TIterator::Key());
		}
		void operator++()
		{
			while (1)
			{
				TMapBase<TK, TI>::TIterator::Increment();

				if (!*this)
					return;

				INT OldNum = VisitedKeys.Num();
				VisitedKeys.AddUniqueItem(TMapBase<TK, TI>::TIterator::Key());
				if (OldNum != VisitedKeys.Num())
					return;
			}
		}
	private:
		TArray<TK> VisitedKeys;
	};
	friend class TUniqueKeyIterator;
	friend class TIterator;
};
template< class TK, class TI > class TMap : public TMapBase<TK,TI>
{
public:
	TMap& operator=( const TMap& Other )
	{
		TMapBase<TK,TI>::operator=( Other );
		return *this;
	}

	int Num()
	{
		guardSlow(TMap::Num);
		return this->Pairs.Num();
		unguardSlow;
	}
};
template< class TK, class TI > class TMultiMap : public TMapBase<TK,TI>
{
public:
	TMultiMap& operator=( const TMultiMap& Other )
	{
		TMapBase<TK,TI>::operator=( Other );
		return *this;
	}

	int Num()
	{
		guardSlow(TMultiMap::Num);
		return this->Pairs.Num();
		unguardSlow;
	}

	int Num(TArray<TK>& Keys)
	{
		guardSlow(TMultiMap::NumKeys);
		Keys.Empty();

		for( INT i=0;i<this->Pairs.Num();i++ )
		{
			INT j=Keys.FindItemIndex(this->Pairs(i).Key);
			if (j==INDEX_NONE)
			{
				j=Keys.AddZeroed();
				Keys(j)=this->Pairs(i).Key;
			}
		}
		return Keys.Num();
		unguardSlow;
	}

	void MultiFind( const TK& Key, TArray<TI>& Values ) 
	{
		guardSlow(TMultiMap::MultiFind);
		for( INT i=this->Hash[(GetTypeHash(Key) & (this->HashCount-1))]; i!=INDEX_NONE; i=this->Pairs(i).HashNext )
			if( this->Pairs(i).Key==Key )
				new(Values)TI(this->Pairs(i).Value);
		unguardSlow;
	}
	TI& Add( typename TTypeInfo<TK>::ConstInitType InKey, typename TTypeInfo<TI>::ConstInitType InValue )
	{
		return TMapBase<TK,TI>::Add( InKey, InValue );
	}
	TI& AddUnique( typename TTypeInfo<TK>::ConstInitType InKey, typename TTypeInfo<TI>::ConstInitType InValue )
	{
		for( INT i=this->Hash[(GetTypeHash(InKey) & (this->HashCount-1))]; i!=INDEX_NONE; i=this->Pairs(i).HashNext )
			if( this->Pairs(i).Key==InKey && this->Pairs(i).Value==InValue )
				return this->Pairs(i).Value;
		return Add( InKey, InValue );
	}
	inline INT Count(const TK& Key)
	{
		guardSlow(TMultiMap::Count);
		INT Result = 0;
		for (INT i = this->Hash[(GetTypeHash(Key) & (this->HashCount - 1))]; i != INDEX_NONE; i = this->Pairs(i).HashNext)
			if (this->Pairs(i).Key == Key)
				++Result;
		return Result;
		unguardSlow;
	}

	INT MultiRemove( const TK& InKey )
	{
		guardSlow(TMultiMap::RemoveMulti);
		INT Count = 0;
		for ( INT i=this->Pairs.Num()-1; i>=0; i-- )
			if ( this->Pairs(i).Key == InKey )
				{this->Pairs.Remove(i); Count++;}
		if ( Count )
			this->Relax();
		return Count;
		unguardSlow;
	}
	INT RemovePair( typename TTypeInfo<TK>::ConstInitType InKey, typename TTypeInfo<TI>::ConstInitType InValue )
	{
		guardSlow(TMultiMap::RemovePair);
		INT Count=0;
		for( INT i=this->Pairs.Num()-1; i>=0; i-- )
			if( this->Pairs(i).Key==InKey && this->Pairs(i).Value==InValue )
				{this->Pairs.Remove(i); Count++;}
		if( Count )
			this->Relax();
		return Count;
		unguardSlow;
	}
	TI* FindPair( const TK& Key, const TI& Value )
	{
		guardSlow(TMultiMap::FindPair);
		for( INT i=this->Hash[(GetTypeHash(Key) & (this->HashCount-1))]; i!=INDEX_NONE; i=this->Pairs(i).HashNext )
			if( this->Pairs(i).Key==Key && this->Pairs(i).Value==Value )
				return &this->Pairs(i).Value;
		return NULL;
		unguardSlow;
	}
};

// Unordered set, written by Marco
template< class TK > class TUnorderedSet
{
protected:
	class TValue
	{
	public:
		INT HashNext;
		TK Key;
		TValue(typename TTypeInfo<TK>::ConstInitType InKey)
			: Key(InKey)
		{}
		TValue()
		{}
		friend FArchive& operator<<(FArchive& Ar, TValue& F)
		{
			guardSlow(TUnorderedSet::TValue << );
			return Ar << F.Key;
			unguardSlow;
		}
	};
	void Rehash()
	{
		guardSlow(TUnorderedSet::Rehash);
		checkSlow(!(HashCount & (HashCount - 1)));
		checkSlow(HashCount >= 8);
		INT* NewHash = new INT[HashCount];
		{for (INT i = 0; i < HashCount; i++)
		{
			NewHash[i] = INDEX_NONE;
		}}
		{for (INT i = 0; i < Pairs.Num(); i++)
		{
			TValue& Pair = Pairs(i);
			INT    iHash = (GetTypeHash(Pair.Key) & (HashCount - 1));
			Pair.HashNext = NewHash[iHash];
			NewHash[iHash] = i;
		}}
		if (Hash)
			delete[] Hash;
		Hash = NewHash;
		unguardSlow;
	}
	void Relax()
	{
		guardSlow(TUnorderedSet::Relax);
		while (HashCount > Pairs.Num() * 2 + 8)
			HashCount /= 2;
		Rehash();
		unguardSlow;
	}
	void Add(typename TTypeInfo<TK>::ConstInitType InKey)
	{
		guardSlow(TUnorderedSet::Add);
		TValue& Pair = *new(Pairs)TValue(InKey);
		INT    iHash = (GetTypeHash(Pair.Key) & (HashCount - 1));
		Pair.HashNext = Hash[iHash];
		Hash[iHash] = Pairs.Num() - 1;
		if (HashCount * 2 + 8 < Pairs.Num())
		{
			HashCount *= 2;
			Rehash();
		}
		unguardSlow;
	}
	TArray<TValue> Pairs;
	INT* Hash;
	INT HashCount;
public:
	TUnorderedSet()
		: Hash(NULL)
		, HashCount(8)
	{
		guardSlow(TUnorderedSet::TMapBase);
		Rehash();
		unguardSlow;
	}
	TUnorderedSet(const TUnorderedSet& Other)
		: Pairs(Other.Pairs)
		, Hash(NULL)
		, HashCount(Other.HashCount)
	{
		guardSlow(TUnorderedSet::TMapBase copy);
		Rehash();
		unguardSlow;
	}
	~TUnorderedSet()
	{
		guardSlow(TUnorderedSet::~TMapBase);
		if (Hash)
			delete[] Hash;
		Hash = NULL;
		HashCount = 0;
		unguardSlow;
	}
	TUnorderedSet& operator=(const TUnorderedSet& Other)
	{
		guardSlow(TUnorderedSet::operator=);
		Pairs = Other.Pairs;
		HashCount = Other.HashCount;
		Rehash();
		return *this;
		unguardSlow;
	}
	void Empty()
	{
		guardSlow(TUnorderedSet::Empty);
		checkSlow(!(HashCount & (HashCount - 1)));
		Pairs.Empty();
		HashCount = 8;
		Rehash();
		unguardSlow;
	}
	UBOOL Set(typename TTypeInfo<TK>::ConstInitType InKey)
	{
		guardSlow(TUnorderedSet::Set);
		for (INT i = Hash[(GetTypeHash(InKey) & (HashCount - 1))]; i != INDEX_NONE; i = Pairs(i).HashNext)
			if (Pairs(i).Key == InKey)
				return 0;
		Add(InKey);
		return 1;
		unguardSlow;
	}
	UBOOL Remove(typename TTypeInfo<TK>::ConstInitType InKey)
	{
		guardSlow(TUnorderedSet::Remove);
		for (INT i = Hash[(GetTypeHash(InKey) & (HashCount - 1))]; i != INDEX_NONE; i = Pairs(i).HashNext)
			if (Pairs(i).Key == InKey)
			{
				Pairs.Remove(i);
				Relax();
				return 1;
			}
		return 0;
		unguardSlow;
	}
	UBOOL Find(const TK& Key) const
	{
		guardSlow(TUnorderedSet::Find);
		for (INT i = Hash[(GetTypeHash(Key) & (HashCount - 1))]; i != INDEX_NONE; i = Pairs(i).HashNext)
			if (Pairs(i).Key == Key)
				return 1;
		return 0;
		unguardSlow;
	}
	inline INT Num() const
	{
		guardSlow(TUnorderedSet::Remove);
		return Pairs.Num();
		unguardSlow;
	}
	friend FArchive& operator<<(FArchive& Ar, TUnorderedSet& M)
	{
		guardSlow(TUnorderedSet << );
		Ar << M.Pairs;
		if (Ar.IsLoading())
			M.Rehash();
		return Ar;
		unguardSlow;
	}
	class TIterator
	{
	public:
		TIterator(TUnorderedSet& InMap) : MP(InMap), Index(0), bDeleted(FALSE) {}
		~TIterator() { if (bDeleted) MP.Relax(); }
		void operator++() { ++Index; }
		void RemoveCurrent() { MP.Pairs.Remove(Index--); bDeleted = TRUE; }
		operator UBOOL() const { return Index < MP.Pairs.Num(); }
		TK& Key() const { return MP.Pairs(Index).Key; }
	private:
		TUnorderedSet& MP;
		INT Index;
		UBOOL bDeleted;
	};
	friend class TIterator;
};

/*----------------------------------------------------------------------------
	Dynamically allocated bitmasks
----------------------------------------------------------------------------*/

#define INT_ROUND_UP(x, multiple) \
	(((INT)(x) + (multiple)-1) & (~((multiple)-1)))

#define INT_ROUND_DOWN(x, multiple) \
	(((INT)(x)) & (~((multiple)-1)))

class FBitmask
{
public:
	FBitmask(INT NumBits)
	{
		Data.AddZeroed(INT_ROUND_UP(NumBits, 8) >> 3);
		HighestBitSet = -1;
	}

	BYTE GetBit(INT BitNum)
	{
		const INT Tmp        = INT_ROUND_DOWN(BitNum, 8);
		const INT DataOffset = Tmp >> 3;
		const BYTE BitShift  = BitNum - Tmp;

		checkSlow(DataOffset < Data.Num());

		return (Data(DataOffset) >> BitShift) & 0x1;
	}

	void SetBit(INT BitNum, BYTE BitValue)
	{
		const INT Tmp = INT_ROUND_DOWN(BitNum, 8);
		const INT DataOffset = Tmp >> 3;
		const BYTE BitShift = BitNum - Tmp;

		checkSlow(DataOffset < Data.Num());

		Data(DataOffset) = (Data(DataOffset) & ~(1 << BitShift)) | ((BitValue & 0x1) << BitShift);

		if (BitValue == 1)
			HighestBitSet = Max(BitNum, HighestBitSet);
		else
		{
			if (BitNum == HighestBitSet)
			{
				HighestBitSet = -1;
				for (INT i = 0; i < BitNum; ++i)
					if (GetBit(i))
						HighestBitSet = Max(i, HighestBitSet);
			}
		}
	}

	INT HighestBitSet;

private:
	TArray<BYTE> Data;	
};

/*----------------------------------------------------------------------------
	Sorting template.
----------------------------------------------------------------------------*/

//
// Sort elements. The sort is unstable, meaning that the ordering of equal 
// items is not necessarily preserved.
//
template<class T> struct TStack
{
	T* Min;
	T* Max;
};
template<class T> void Sort( T* First, INT Num )
{
	guard(Sort);
	if( Num<2 )
		return;
	TStack<T> RecursionStack[32]={{First,First+Num-1}}, Current, Inner;
	for( TStack<T>* StackTop=RecursionStack; StackTop>=RecursionStack; --StackTop )
	{
		Current = *StackTop;
	Loop:
		INT Count = Current.Max - Current.Min + 1;
		if( Count <= 8 )
		{
			// Use simple bubble-sort.
			while( Current.Max > Current.Min )
			{
				T *Max, *Item;
				for( Max=Current.Min, Item=Current.Min+1; Item<=Current.Max; Item++ )
					if( Compare(*Item, *Max) > 0 )
						Max = Item;
				Exchange( *Max, *Current.Max-- );
			}
		}
		else
		{
			// Grab middle element so sort doesn't exhibit worst-cast behaviour with presorted lists.
			Exchange( Current.Min[Count/2], Current.Min[0] );

			// Divide list into two halves, one with items <=Current.Min, the other with items >Current.Max.
			Inner.Min = Current.Min;
			Inner.Max = Current.Max+1;
			for( ; ; )
			{
				while( ++Inner.Min<=Current.Max && Compare(*Inner.Min, *Current.Min) <= 0 );
				while( --Inner.Max> Current.Min && Compare(*Inner.Max, *Current.Min) >= 0 );
				if( Inner.Min>Inner.Max )
					break;
				Exchange( *Inner.Min, *Inner.Max );
			}
			Exchange( *Current.Min, *Inner.Max );

			// Save big half and recurse with small half.
			if( Inner.Max-1-Current.Min >= Current.Max-Inner.Min )
			{
				if( Current.Min+1 < Inner.Max )
				{
					StackTop->Min = Current.Min;
					StackTop->Max = Inner.Max - 1;
					StackTop++;
				}
				if( Current.Max>Inner.Min )
				{
					Current.Min = Inner.Min;
					goto Loop;
				}
			}
			else
			{
				if( Current.Max>Inner.Min )
				{
					StackTop->Min = Inner  .Min;
					StackTop->Max = Current.Max;
					StackTop++;
				}
				if( Current.Min+1<Inner.Max )
				{
					Current.Max = Inner.Max - 1;
					goto Loop;
				}
			}
		}
	}
	unguard;
}

template<class T> void Sort(TArray<T>& Array)
{
	if (Array.Num())
		Sort<T>(&Array(0), Array.Num());
}

/*----------------------------------------------------------------------------
	TDoubleLinkedList.
----------------------------------------------------------------------------*/

//
// Simple double-linked list template.
//
template< class T > class TDoubleLinkedList : public T
{
public:
	TDoubleLinkedList* Next;
	TDoubleLinkedList** PrevLink;
	void Unlink()
	{
		if( Next )
			Next->PrevLink = PrevLink;
		*PrevLink = Next;
	}
	void Link( TDoubleLinkedList*& Before )
	{
		if( Before )
			Before->PrevLink = &Next;
		Next     = Before;
		PrevLink = &Before;
		Before   = this;
	}
};

/*----------------------------------------------------------------------------
	TList.
----------------------------------------------------------------------------*/

//
// Simple single-linked list template.
//
template <class ElementType> class TList
{
public:

	ElementType			Element;
	TList<ElementType>*	Next;

	// Constructor.
	TList(ENoInit)
	{}

	TList(ElementType InElement,TList<ElementType>* InNext = NULL)
	{
		Element = InElement;
		Next = InNext;
	}
};

/*----------------------------------------------------------------------------
	TLinkedListHandler.
----------------------------------------------------------------------------*/

//
// Templated linked list handler.
//
template < class THandlerType > class TLinkedListHandler
{
	typedef typename THandlerType::ObjectType ObjectType;
	ObjectType** StartPtr;

public:
	TLinkedListHandler( ObjectType*& InStart)
		: StartPtr(&InStart)
	{}

	UBOOL RemoveFromList( ObjectType* Remove, bool NullNext=true)
	{
		ObjectType** LinkPtr;
		ObjectType** NextPtr;
		for ( LinkPtr=StartPtr; *LinkPtr; LinkPtr=NextPtr)
		{
			NextPtr = &THandlerType::GetNext(*LinkPtr);
			if ( *LinkPtr == Remove )
			{
				*LinkPtr = *NextPtr;
				if ( NullNext )
					*NextPtr = nullptr;
				return true;
			}
		}
		return false;
	}

	void BreakInfiniteLoops()
	{
		TArray<ObjectType*> Found;
		ObjectType** LinkPtr;
		ObjectType** NextPtr;
		for ( LinkPtr=StartPtr; *LinkPtr; LinkPtr=NextPtr)
		{
			// Add current now so we can break up when objects reference 'self' as next.
			Found.AddItem(*LinkPtr);
			NextPtr = &THandlerType::GetNext(*LinkPtr);

			// Next is already part of the chain, break it up.
			if ( *NextPtr && (Found.FindItemIndex(*NextPtr) != INDEX_NONE) )
			{
				*NextPtr = nullptr;
				break;
			}
		}
	}
};

//
// This is an example of a generic handler for TLinkedListHandler
//
template < class T > struct TLinkedListHandlerPrototype
{
	/* Needed to tell the handler what kind of pointer it must be prototyped on */
	typedef T ObjectType;

	/* Needed to tell the handler how to get the next pointer in the chain */
	static T*& GetNext( T* Current)  { return Current->GetNext(); }
};


/*----------------------------------------------------------------------------
	FRainbowPtr.
----------------------------------------------------------------------------*/

//
// A union of pointers of all base types.
//
union CORE_API FRainbowPtr
{
	// All pointers.
	void*  PtrVOID;
	BYTE*  PtrBYTE;
	_WORD* PtrWORD;
	DWORD* PtrDWORD;
	QWORD* PtrQWORD;
	FLOAT* PtrFLOAT;

	// Conversion constructors.
	FRainbowPtr() {}
	FRainbowPtr( void* Ptr ) : PtrVOID(Ptr) {};
};

/*----------------------------------------------------------------------------
	TExtraDataCache - stijn: occassionally, we want to add additional fields 
	to one of the core engine structures but we would like to do so without 
	breaking binary compatibility. TExtraDataCache allows you to create a 
	cache in which we can store the additional fields separate from the 
	object they belong to.

	Lookups in this cache are expensive! Do not use this for 
	performance-critical code.
----------------------------------------------------------------------------*/

template<class BaseType, class ExtraDataType> 
class TExtraDataCache
{
public:
	TExtraDataCache() = default;
	~TExtraDataCache()
	{
		if (Cache)
		{
			Cache->Empty();
			delete Cache;
		}
	}	

	TExtraDataCache(TExtraDataCache& Other) = delete;
	TExtraDataCache& operator=(TExtraDataCache Other) = delete;
	TExtraDataCache& operator=(TExtraDataCache&& Other) = delete;
	TExtraDataCache(TExtraDataCache&& Other) = delete;

	ExtraDataType* Get(BaseType* ForBase)
	{
		if (!Cache)
			Cache = new TMap<BaseType*, ExtraDataType>;
		ExtraDataType* Result = Cache->Find(ForBase);
		if (!Result)
			return &Cache->Set(ForBase, ExtraDataType());
		return Result;
	}

	bool Remove(BaseType* ForBase)
	{
		if (!Cache)
			Cache = new TMap<BaseType*, ExtraDataType>;
		return Cache->Remove(ForBase) > 0;
	}

	void Copy(BaseType* Dst, BaseType* Src)
	{
		if (!Cache)
			Cache = new TMap<BaseType*, ExtraDataType>;
		ExtraDataType* Result = Cache->Find(Src);
		if (Result)
			return Cache->Set(Dst, *Result);
	}

private:
	TMap<BaseType*, ExtraDataType>* Cache{};
};

#define DECLARE_TEXTRA(BaseType, ExtraDataType)	\
	extern TExtraDataCache<BaseType, ExtraDataType> BaseType##ExtraDataCache;

#define IMPLEMENT_TEXTRA(BaseType, ExtraDataType)	\
	TExtraDataCache<BaseType, ExtraDataType> BaseType##ExtraDataCache;

#define TEXTRA(BaseType) \
	BaseType##ExtraDataCache

#endif // _INCL_UNTEMPLATE_H_

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
