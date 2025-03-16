/*=============================================================================
	UnString.h: Unreal String definitions.
	Based on the original implementation in UnTemplate.h

	Copyright 2024-2024 OldUnreal. All Rights Reserved.

	Revision history:
		* Created by Fernando Velazquez
=============================================================================*/

#define FSTRING(str) FString(TEXT(str))

//
// A dynamically sizeable string.
//
class CORE_API FString : protected TArray<TCHAR>
{
public:
	FString();
	FString( const FString& Other );
#if !MOD_BUILD
	FString( FString&& Other ) noexcept;
#endif
	FString( const TCHAR* In );
	FString( const TCHAR* Start, const TCHAR* End );
	FString( const TCHAR* In, INT MaxBytes );
	FString( ENoInit );
	FString( const ANSICHAR* AnsiIn );
	FString( const ANSICHAR* AnsiIn , StringEncoding Encoding );
	explicit FString( BYTE   Arg, INT Digits=1 );
	explicit FString( SBYTE  Arg, INT Digits=1 );
	explicit FString( _WORD  Arg, INT Digits=1 );
	explicit FString( SWORD  Arg, INT Digits=1 );
	explicit FString( INT    Arg, INT Digits=1 );
	explicit FString( DWORD  Arg, INT Digits=1 );
	explicit FString( FLOAT  Arg, INT Digits=1, INT RightDigits=0, UBOOL LeadZero=1 );

	FString& operator=( const TCHAR* Other );
	FString& operator=( const FString& Other );
#if !MOD_BUILD
	FString& operator=( FString&& Other );
#endif
	FString& operator=( const ANSICHAR* AnsiOther );

	TCHAR& operator[]( INT i );
	const TCHAR& operator[]( INT i ) const;

	FString& operator+=( const TCHAR Ch );
	FString& operator+=( const TCHAR* Str );
	FString& operator+=( const FString& Str );
	FString  operator+ ( const TCHAR* Str );
	FString  operator+ ( const FString& Str );
	FString& operator*=( const TCHAR* Str );
	FString& operator*=( const FString& Str );
	FString  operator* ( const TCHAR* Str ) const;
	FString  operator* ( const FString& Str ) const;
	UBOOL operator<=( const TCHAR* Other ) const;
	UBOOL operator< ( const TCHAR* Other ) const;
	UBOOL operator>=( const TCHAR* Other ) const;
	UBOOL operator> ( const TCHAR* Other ) const;
	UBOOL operator==( const TCHAR* Other ) const;
	UBOOL operator==( const FString& Other ) const;
	UBOOL operator!=( const TCHAR* Other ) const;
	UBOOL operator!=( const FString& Other ) const;

	const TCHAR* operator*() const;
	operator UBOOL() const;

	void Empty();
	void EmptyNoRealloc();
	void Shrink();
	void RecalculateLength(); // Used after converting a multi-byte encoded string to a fixed-width TCHAR string

	TArray<TCHAR>& GetCharArray();

	INT Len() const;
	UBOOL IsAlpha() const;
	UBOOL IsAlnum() const;
	UBOOL IsNum() const;
	UBOOL IsHex() const;
	UBOOL IsName( UBOOL AllowDigitStart = 0 ) const;

	// Legacy manipulation functions
	FString Left( INT Count ) const;
	FString LeftChop( INT Count ) const;
	FString Right( INT Count ) const;
	FString Mid( INT Start, INT Count=MAXINT ) const;
	INT InStr( const TCHAR* SubStr, UBOOL Right=0 ) const;
	INT InStr( const FString& SubStr, UBOOL Right=0 ) const;
	UBOOL Split( const FString& InS, FString* LeftS, FString* RightS, UBOOL Right=0 ) const;
	FString Caps() const;
	FString Locs() const;
	FString Replace( const TCHAR* Match, const TCHAR* Replacement ) const;

	// In-place manipulation functions
	void LeftInline( INT Count, EAllowShrinking AllowShrinking=EAllowShrinking::Yes );
	void LeftChopInline( INT Count, EAllowShrinking AllowShrinking=EAllowShrinking::Yes );
	void RightInline( INT Count, EAllowShrinking AllowShrinking=EAllowShrinking::Yes );
	void RightChopInline( INT Count, EAllowShrinking AllowShrinking=EAllowShrinking::Yes );
	void MidInline( INT Start, INT Count = MAXINT, EAllowShrinking AllowShrinking=EAllowShrinking::Yes );
	void ToUpperInline();
	void ToLowerInline();

	INT Int() const;
	FLOAT Float() const;

	// 227 additions
	FString GetFilenameOnly() const;
	FString GetFileExtension() const;
	static FString GetFilenameOnlyStr(const TCHAR* Str);
	static FString GetFileExtensionStr(const TCHAR* Str);

	FString LeftPad( INT ChCount );
	FString RightPad( INT ChCount );
	static FString Printf( const TCHAR* Fmt, ... );
	static FString Printf( const TCHAR* Fmt, va_list Args);
	static FString& Printf(FString& Result, const TCHAR* Fmt, ...);
	static FString Chr( TCHAR Ch );
	static FString NiceFloat( FLOAT Value ); // Tries to preserve the dot and a zero afterwards, but ditches otherwise needless zeros.
	CORE_API friend FArchive& operator<<( FArchive& Ar, FString& S );
	friend struct FStringNoInit;

private:
	FString( INT InCount, const TCHAR* InSrc );
};

// stijn: same as FString but without iconv conversion for serialization
class CORE_API FStringRaw : public FString
{
public:
	FStringRaw(const TCHAR* Other)
	{
		if( (Data == NULL) || (&(*this)(0)!=Other) ) // gam
		{
			ArrayNum = (Other && *Other) ? appStrlen(Other)+1 : 0;
			if (ArrayMax != ArrayNum)
			{
				ArrayMax = ArrayNum;
				Realloc( sizeof(TCHAR) );
			}
			if( ArrayNum )
				appMemcpy( &(*this)(0), Other, ArrayNum*sizeof(TCHAR) );
		}
	}
	CORE_API friend FArchive& operator<<(FArchive& Ar, FStringRaw& S);
};

struct CORE_API FStringNoInit : public FString
{
	FStringNoInit()
		: FString( E_NoInit )
	{}
	FStringNoInit& operator=( const TCHAR* Other )
	{
		if( (Data == NULL) || ArrayNum == 0 || (&(*this)(0)!=Other) ) // gam
		{
			ArrayNum = (Other && *Other) ? appStrlen(Other)+1 : 0;
			if (ArrayMax != ArrayNum)
			{
				ArrayMax = ArrayNum;
				Realloc( sizeof(TCHAR) );
			}
			if( ArrayNum )
				appMemcpy( &(*this)(0), Other, ArrayNum*sizeof(TCHAR) );
		}
		return *this;
	}
	FStringNoInit& operator=( const FString& Other )
	{
		if( this != &Other )
		{
			ArrayNum = Other.Num();
			if (ArrayMax != ArrayNum)
			{
				ArrayMax = ArrayNum;
				Realloc( sizeof(TCHAR) );
			}
			if( ArrayNum )
				appMemcpy( &(*this)(0), *Other, ArrayNum*sizeof(TCHAR) );
		}
		return *this;
	}
};


/*----------------------------------------------------------------------------
	FString inlines.
----------------------------------------------------------------------------*/

//
// Constructors
//
inline FString::FString()
	: TArray<TCHAR>()
{}

inline FString::FString( const FString& Other )
	: TArray<TCHAR>( Other.ArrayNum )
{
	if( ArrayNum )
		appMemcpy( &(*this)(0), &Other(0), ArrayNum*sizeof(TCHAR) );
}
#if !MOD_BUILD
inline FString::FString( FString&& Other ) noexcept
	: TArray<TCHAR>( (TArray<TCHAR>&&)Other )
{}
#endif
inline FString::FString( const TCHAR* In )
	: TArray<TCHAR>( (In && *In) ? (appStrlen(In)+1) : 0 )
{
	if( ArrayNum )
		appMemcpy( &(*this)(0), In, ArrayNum*sizeof(TCHAR) );
}

inline FString::FString(const TCHAR* Start, const TCHAR* End)
	: TArray<TCHAR>((End > Start) ? ((End - Start) + 1) : 0)
{
	if (ArrayNum)
	{
		appMemcpy(&(*this)(0), Start, (ArrayNum - 1) * sizeof(TCHAR));
		(*this)(ArrayNum - 1) = 0;
	}
}

inline FString::FString( const TCHAR* In, INT MaxBytes )
	: TArray<TCHAR>( MaxBytes + 1 )
{
	// stijn: was
	// if ( ArrayNum && In && appStrlen(In) > MaxBytes)
	// before. This didn't really make sense...
	if ( ArrayNum && In)
	{
		INT CopyLen = Min(appStrlen(In), MaxBytes);
		if (CopyLen)
			appMemcpy(&(*this)(0), In, CopyLen * sizeof(TCHAR));
		(*this)(MaxBytes) = 0;
	}
}

inline FString::FString( ENoInit )
	: TArray<TCHAR>( E_NoInit )
{}

// Private
inline FString::FString( INT InCount, const TCHAR* InSrc )
	:	TArray<TCHAR>( InCount ? InCount+1 : 0 )
{
	if ( ArrayNum )
		appStrncpy( &(*this)(0), InSrc, InCount+1 );
}


//
// Assignment operators
//
inline FString& FString::operator=( const TCHAR* Other )
{
	if( (Data == NULL) || ArrayMax == 0 || (&(*this)(0)!=Other) ) // gam
	{
		ArrayNum = (Other && *Other) ? appStrlen(Other)+1 : 0;
		if (ArrayMax < ArrayNum)
		{
			ArrayMax = ArrayNum;
			Realloc( sizeof(TCHAR) );
		}
		if( ArrayNum )
			appMemcpy( &(*this)(0), Other, ArrayNum*sizeof(TCHAR) );
	}
	return *this;
}

inline FString& FString::operator=( const FString& Other )
{
	if( this != &Other )
	{
		ArrayNum = Other.Num();
		if (ArrayMax < ArrayNum)
		{
			ArrayMax = ArrayNum;
			Realloc( sizeof(TCHAR) );
		}
		if( ArrayNum )
			appMemcpy( &(*this)(0), *Other, ArrayNum*sizeof(TCHAR) );
	}
	return *this;
}

#if !MOD_BUILD
inline FString& FString::operator=( FString&& Other )
{
	TRawData<FString>::Swap(*this, Other);
	//*(TArray<TCHAR>*)this = (TArray<TCHAR>&&)Other;
	return *this;
}
#endif


//
// Character accessors
//
inline TCHAR& FString::operator[]( INT i )
{
	guardSlow(FString::operator());
	checkSlow(i>=0);
	checkSlow(i<=ArrayNum);
	checkSlow(ArrayMax>=ArrayNum);
	return ((TCHAR*)Data)[i];
	unguardSlow;
}

inline const TCHAR& FString::operator[]( INT i ) const
{
	guardSlow(FString::operator());
	checkSlow(i>=0);
	checkSlow(i<=ArrayNum);
	checkSlow(ArrayMax>=ArrayNum);
	return ((TCHAR*)Data)[i];
	unguardSlow;
}


//
// Other operators
//
inline FString& FString::operator+=( const TCHAR Ch )
{
	if (Ch)
	{
		if (ArrayNum)
		{
			INT Index = ArrayNum - 1;
			Add(1);
			(*this)(Index) = Ch;
			(*this)(Index + 1) = 0;
		}
		else
		{
			Add(2);
			(*this)(0) = Ch;
			(*this)(1) = 0;
		}
	}
	return *this;
}

inline FString& FString::operator+=( const TCHAR* Str )
{
	if ( Str && *Str != '\0' ) // gam
	{
		if( ArrayNum )
		{
			INT Index = ArrayNum-1;
			Add( appStrlen(Str) );
			appStrcpy( &(*this)(Index), Str ); // stijn: mem safety ok
		}
		else
		{
			Add( appStrlen(Str)+1 );
			appStrcpy( &(*this)(0), Str ); // stijn: mem safety ok
		}
	}
	return *this;
}

inline FString& FString::operator+=( const FString& Str )
{
	return operator+=( *Str );
}

inline FString FString::operator+( const TCHAR* Str )
{
	return FString( *this ) += Str;
}

inline FString FString::operator+( const FString& Str )
{
	return operator+( *Str );
}

inline FString& FString::operator*=( const TCHAR* Str )
{
	if( ArrayNum>1 && (*this)(ArrayNum-2)!=PATH_SEPARATOR[0] )
		*this += PATH_SEPARATOR;
	return *this += Str;
}

inline FString& FString::operator*=( const FString& Str )
{
	return operator*=( *Str );
}

inline FString FString::operator*( const TCHAR* Str ) const
{
	return FString( *this ) *= Str;
}

inline FString FString::operator*( const FString& Str ) const
{
	return operator*( *Str );
}

inline UBOOL FString::operator<=( const TCHAR* Other ) const
{
	return !(appStricmp( **this, Other ) > 0);
}

inline UBOOL FString::operator<( const TCHAR* Other ) const
{
	return appStricmp( **this, Other ) < 0;
}

inline UBOOL FString::operator>=( const TCHAR* Other ) const
{
	return !(appStricmp( **this, Other ) < 0);
}

inline UBOOL FString::operator>( const TCHAR* Other ) const
{
	return appStricmp( **this, Other ) > 0;
}

inline UBOOL FString::operator==( const TCHAR* Other ) const
{
	return appStricmp( **this, Other )==0;
}

inline UBOOL FString::operator==( const FString& Other ) const
{
	return appStricmp( **this, *Other )==0;
}

inline UBOOL FString::operator!=( const TCHAR* Other ) const
{
	return appStricmp( **this, Other )!=0;
}

inline UBOOL FString::operator!=( const FString& Other ) const
{
	return appStricmp( **this, *Other )!=0;
}

inline const TCHAR* FString::operator*() const
{
	return Num() ? &(*this)(0) : TEXT("");
}

inline FString::operator UBOOL() const
{
	return Num() != 0;
}


//
// General container functions
//
inline void FString::Empty()
{
	if (Data)
		TArray<TCHAR>::Empty();
}

inline void FString::EmptyNoRealloc()
{
	TArray<TCHAR>::EmptyNoRealloc();
}

inline void FString::Shrink()
{
	TArray<TCHAR>::Shrink();
}

inline TArray<TCHAR>& FString::GetCharArray()
{
	//warning: Operations on the TArray<CHAR> can be unsafe, such as adding
	// non-terminating 0's or removing the terminating zero.
	return (TArray<TCHAR>&)*this;
}

inline void FString::RecalculateLength()
{
	const TCHAR* Buf = **this;
	if ( Buf )
		ArrayNum = (*Buf ? appStrlen(Buf) : 0) + 1;
	else
		ArrayNum = 0;
}

//
// Queries
//
inline INT FString::Len() const
{
	return Num() ? Num()-1 : 0;
}

inline UBOOL FString::IsAlpha() const
{
	INT Length = Len();
	for (INT Pos = 0; Pos < Length; Pos++)
		if (!appIsAlpha((*this)(Pos)))
			return 0;
	return 1;
}

inline UBOOL FString::IsAlnum() const
{
	INT Length = Len();
	for (INT Pos = 0; Pos < Length; Pos++)
		if (!appIsAlnum((*this)(Pos)))
			return 0;
	return 1;
}

inline UBOOL FString::IsNum() const
{
	INT Length = Len();
	for (INT Pos = 0; Pos < Length; Pos++)
		if (!appIsDigit((*this)(Pos)))
			return 0;
	return 1;
}

inline UBOOL FString::IsHex() const
{
	INT Length = Len();
	for (INT Pos = 0; Pos < Length; Pos++)
		if (!appIsHexDigit((*this)(Pos)))
			return 0;
	return 1;
}

inline UBOOL FString::IsName( UBOOL AllowDigitStart ) const
{
	guard(FString::IsName);
	if (Len() == 0)
		return 1;
	if (!AllowDigitStart && appIsDigit((*this)(0)))
		return 0;
	INT Length = Len();
	for (INT Pos = 0; Pos < Length; Pos++)
		if (!appIsAlnum((*this)(Pos)) && (*this)(Pos) != '_')
			return 0;
	return 1;
	unguard;
}


//
// Legacy string manipulation functions
//

inline FString FString::Left( INT Count ) const
{
	return FString( Clamp(Count,0,Len()), **this );
}

inline FString FString::LeftChop( INT Count ) const
{
	INT Length = Len();
	return FString( Clamp(Length-Count,0,Length), **this );
}

inline FString FString::Right( INT Count ) const
{
	INT Length = Len();
	return FString( **this + Length-Clamp(Count,0,Length) );
}

inline FString FString::Mid( INT Start, INT Count ) const
{
	INT Length = Len();
	DWORD End = Start+Count;
	Start    = Clamp( (DWORD)Start, (DWORD)0,     (DWORD)Length );
	End      = Clamp( (DWORD)End,   (DWORD)Start, (DWORD)Length );
	return FString( End-Start, **this + Start );
}

inline INT FString::InStr( const TCHAR* SubStr, UBOOL Right ) const
{
	if( !Right )
	{
		TCHAR* Tmp = appStrstr(**this,SubStr);
		return Tmp ? (Tmp-**this) : -1;
	}
	else
	{
		for( INT i=Len()-1; i>=0; i-- )
		{
			INT j;
			for( j=0; SubStr[j]; j++ )
				if( (*this)(i+j)!=SubStr[j] )
					break;
			if( !SubStr[j] )
				return i;
		}
		return -1;
	}
}

inline INT FString::InStr( const FString& SubStr, UBOOL Right ) const
{
	return InStr( *SubStr, Right );
}

inline UBOOL FString::Split( const FString& InS, FString* LeftS, FString* RightS, UBOOL Right ) const
{
	INT InPos = InStr(InS, Right);
	if( InPos<0 )
		return 0;
	if( LeftS )
		*LeftS = Left(InPos);
	if( RightS )
		*RightS = Mid(InPos+InS.Len());
	return 1;
}

inline FString FString::Caps() const
{
	FString Result;
	const TCHAR* Buf = **this;
	if ( *Buf )
	{
		Result.SetSize( appStrlen(Buf) + 1);
		for( INT i=0; i<Result.ArrayNum; i++ )
			Result(i) = appToUpper(Buf[i]);
	}
	return Result;
}

inline FString FString::Locs() const
{
	FString Result;
	const TCHAR* Buf = **this;
	if ( *Buf )
	{
		Result.SetSize( appStrlen(Buf) + 1);
		for( INT i=0; i<Result.ArrayNum; i++ )
			Result(i) = appToLower(Buf[i]);
	}
	return Result;
}

inline FString FString::Replace( const TCHAR* Match, const TCHAR* Replacement ) const
{
	guard(FString::Replace);

	// Empty find field.
	if ( !Match || !*Match )
		return *this;

	FString Remainder(*this);
	FString Result;

	INT MatchLength = appStrlen(Match);

	while (Remainder.Len())
	{
		INT Index = Remainder.InStr(Match);

		if (Index == INDEX_NONE)
		{
			Result += Remainder;
			break;
		}

		Result += Remainder.Left(Index);
		Result += Replacement;

		Remainder.MidInline(Index + MatchLength, MAXINT, EAllowShrinking::No);
	}

	return Result;
	unguard;
}


//
// In-place string manipulation functions
// Note: TArray::Remove(NoRealloc) are avoided due to slow checks
//

FORCEINLINE void FString::LeftInline( INT Count, EAllowShrinking AllowShrinking )
{
	const INT Length = Len();
	Count = Clamp( Count, 0, Length);

	if ( AllowShrinking == EAllowShrinking::Yes )
		FArray::Remove( Count, Length-Count, sizeof(TCHAR));
	else
		FArray::RemoveNoRealloc( Count, Length-Count, sizeof(TCHAR));
}

FORCEINLINE void FString::LeftChopInline( INT Count, EAllowShrinking AllowShrinking )
{
	const INT Length = Len();
	Count = Clamp( Count, 0, Length);

	if ( AllowShrinking == EAllowShrinking::Yes )
		FArray::Remove( Length-Count, Count, sizeof(TCHAR));
	else
		FArray::RemoveNoRealloc( Length-Count, Count, sizeof(TCHAR));
}

FORCEINLINE void FString::RightInline( INT Count, EAllowShrinking AllowShrinking )
{
	const INT Length = Len();
	Count = Clamp( Count, 0, Length);

	if ( AllowShrinking == EAllowShrinking::Yes )
		FArray::Remove( 0, Length-Count, sizeof(TCHAR));
	else
		FArray::RemoveNoRealloc( 0, Length-Count, sizeof(TCHAR));
}

FORCEINLINE void FString::RightChopInline( INT Count, EAllowShrinking AllowShrinking )
{
	const INT Length = Len();
	Count = Clamp( Count, 0, Length);

	if ( AllowShrinking == EAllowShrinking::Yes )
		FArray::Remove( 0, Count, sizeof(TCHAR));
	else
		FArray::RemoveNoRealloc( 0, Count, sizeof(TCHAR));
}

FORCEINLINE void FString::MidInline( INT Start, INT Count, EAllowShrinking AllowShrinking )
{
	if ( Count != MAXINT )
		LeftInline(Count + Start, AllowShrinking);
	RightChopInline(Start, AllowShrinking);
}

FORCEINLINE void FString::ToUpperInline()
{
	const INT Length = Len();
	TCHAR* RawString = (TCHAR*)GetData();

	for ( INT i=0; i<Length; i++ )
		RawString[i] = appToUpper(RawString[i]);
}

FORCEINLINE void FString::ToLowerInline()
{
	const INT Length = Len();
	TCHAR* RawString = (TCHAR*)GetData();

	for ( INT i=0; i<Length; i++ )
		RawString[i] = appToLower(RawString[i]);
}


//
// String to number
//
inline INT FString::Int() const
{
	guardSlow(FString::Int);
	return appAtoi(**this);
	unguardSlow;
}

inline FLOAT FString::Float() const
{
	guardSlow(FString::Float);
	return appAtof(**this);
	unguardSlow;
}


//
// String generators
//
inline FString FString::NiceFloat( FLOAT Value )
{
	guard(FString::NiceFloat);
	FString Text = FString::Printf(TEXT("%g"), Value);
	if ( Text.InStr(TEXT(".")) == INDEX_NONE && Text.InStr(TEXT("e")) == INDEX_NONE )
		Text += TEXT(".0");
	return Text;
	unguard;
}