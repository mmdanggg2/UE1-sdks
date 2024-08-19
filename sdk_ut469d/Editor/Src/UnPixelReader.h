/*=============================================================================
	UnPixelReader.h: Pixel reader helpers for use in texture factories.

	Revision history:
		* Created by Fernando Velazquez (Higor)
=============================================================================*/


/*-----------------------------------------------------------------------------
	Texture importer parameters.
-----------------------------------------------------------------------------*/

class FImageImporter
{
public:
	const BYTE*       Buffer;
	DWORD             BufferLength;
	FFeedbackContext* Warn;
	INT               TextureFormat;
	TArray<FMipmap>   Mips;
	TArray<FColor>    ColorMap;
	INT               Alpha;

	FImageImporter( const BYTE* InBuffer, DWORD InBufferLength, FFeedbackContext* InWarn)
		: Buffer(InBuffer)
		, BufferLength(InBufferLength)
		, Warn(InWarn)
		, TextureFormat(INDEX_NONE)
		, Alpha(0)
	{}

	// Returns True if data stream represents a file of said format
	// Check that 'TextureFormat' isn't INDEX_NONE to see if import succeeded.
	bool ReadBMP();
	bool ReadPCX();
	bool ReadDDS();
	bool ReadPNG();

	bool CheckAlpha();
	bool ConvertToPremultipliedAlpha();
	bool MaskBlackPixels();
};


/*-----------------------------------------------------------------------------
	Palette reader parameters.
-----------------------------------------------------------------------------*/

enum EPaletteReaderMode
{
	PAL_RGB,  // PCX
	PAL_XBGR, // BMP
};

template < EPaletteReaderMode Mode > struct TColorReaderInfo
{
	static constexpr PTRINT ColorBytes = 0;
	static FColor Read( const BYTE* Stream) { return FColor(0,0,0,255); }
};
template<> struct TColorReaderInfo<PAL_RGB>
{
	static constexpr PTRINT ColorBytes = 3;
	static FColor Read( const BYTE* Stream) { return FColor( Stream[0], Stream[1], Stream[2], 255); }
};
template<> struct TColorReaderInfo<PAL_XBGR>
{
	static constexpr PTRINT ColorBytes = 4;
	static FColor Read( const BYTE* Stream) { return FColor( Stream[2], Stream[1], Stream[0], 255); }
};

/*-----------------------------------------------------------------------------
	Texture importer reader helpers.
-----------------------------------------------------------------------------*/

//
// Pixel reader commons
//
struct FPixelReader
{

	// Reader from header
	template < typename T > static T Read( const T& Value)
	{
		return ReadInternal<T>( (const BYTE*)&Value );
	}

	// Read from stream
	template < typename T > static T Read( const BYTE* Stream)
	{
		return ReadInternal<T>(Stream);
	}

	template < typename T > static bool IsPowerOfTwo( T N)         { return (N & (N-1)) == 0; }
	template < typename T > static bool IsPowerOfTwo( T N1, T N2)  { return ((N1 & (N1-1)) | (N2 & (N2-1))) == 0; }

	template < typename T > static BYTE CalcShift( T Mask)
	{
		for ( BYTE i=0; i<sizeof(T)*8; i++)
			if ( (Mask>>i) & 1 )
				return i;
		return sizeof(T)*8;
	}

private:
	template < typename T > static T ReadInternal( const BYTE* Stream)
	{
#if !__INTEL_BYTE_ORDER__
		if ( sizeof(T) == 1 )
		{
			auto Raw = ReadBYTE(Stream);
			return *(T*)&Raw;
		}
		else if ( sizeof(T) == 2 )
		{
			auto Raw = ReadWORD(Stream);
			return *(T*)&Raw;
		}
		else if ( sizeof(T) == 4 )
		{
			auto Raw = ReadDWORD(Stream);
			return *(T*)&Raw;
		}
		else if ( sizeof(T) == 8 )
		{
			auto Raw = ReadQWORD(Stream);
			return *(T*)&Raw;
		}
		else
#endif
			return *(T*)Stream;
	}

	static BYTE ReadBYTE( const BYTE* Stream)    { return Stream[0]; }
	static WORD ReadWORD( const BYTE* Stream)    { return ((WORD)ReadBYTE(Stream) | (WORD)ReadBYTE(Stream+1) << 8); }
	static DWORD ReadDWORD( const BYTE* Stream)  { return ((DWORD)ReadWORD(Stream) | (DWORD)ReadWORD(Stream+2) << 16); }
	static QWORD ReadQWORD( const BYTE* Stream)  { return ((QWORD)ReadDWORD(Stream) | (QWORD)ReadDWORD(Stream+4) << 32); }
};




//
// Palette reader commons
// TODO: Expand or move ReadColorMap somewhere else.
//
struct FPaletteReader
{
	template < EPaletteReaderMode Mode > static TArray<FColor> ReadColorMap( const BYTE* Stream, INT Count)
	{
		TArray<FColor> ColorMap(NUM_PAL_COLORS);
		INT i = 0;
		for ( ; i<Count; i++)
			ColorMap(i) = TColorReaderInfo<Mode>::Read( Stream + i * TColorReaderInfo<Mode>::ColorBytes);
		for ( ; i<NUM_PAL_COLORS; i++)
			ColorMap(i) = FColor(0,0,0,255);
		return ColorMap;
	}
};


