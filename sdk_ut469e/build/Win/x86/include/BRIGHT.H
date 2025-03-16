//
// Header file
//
// BRIGHT - Best Reduction of Image color Gradations for High quality palettized Textures.
//          Compile with VC++ 6.0 as a Win95 console (command-line) application.
//
//          For UNREAL development / in-house use only.
//

#pragma once

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>

namespace Bright
{


template <class T> inline T	Max    ( const T A, const T B ) { return (A>=B) ? A : B; }
template <class T> inline T	Min    ( const T A, const T B ) { return (A<=B) ? A : B; }
template <class T> inline T	Square ( const T A            ) { return A*A; }
template <class T> inline T Clamp  ( const T X, const T Min, const T Max ) {return X<Min ? Min : X<Max ? X : Max;}
template <class T> inline T Abs    ( const T A            ) { return (A>=0) ? A: -A; }


// Defines
typedef		unsigned char	BYTE;
typedef     unsigned long   DWORD;
typedef		unsigned __int64 QWORD;
typedef		float			FLOAT;
typedef     unsigned short  WORD;
typedef     int				INT;


typedef struct 
	{
	BYTE r;
	BYTE g;
	BYTE b;
	BYTE a;
} PalEntry;


typedef struct 
	{
	BYTE r;
	BYTE g;
	BYTE b;
	BYTE a;
	float weight;
} WeighedPalEntry;


typedef struct 
{
	BYTE r;
	BYTE g;
	BYTE b;
	BYTE a;
} Pixel32;




typedef struct 
{
	union
	{
		BYTE r;
		BYTE g_input;
		BYTE g_output;
	};
	union
	{
		BYTE g;
		BYTE b_input;
		BYTE b_output;
	};
	union
	{
		BYTE b;
		BYTE r_input;
		BYTE r_output;
	};
} Pixel24;



/****************************** TYPES ********************************/

#pragma pack(push,1)

// Headers found at the beginning of a ".BMP" file.
struct FBitmapFileHeader
{
	union
	{
		BYTE  bfFirstByte;     // for PCX checking ....
		WORD  bfType;          // Always "BM".
	};

	union
	{
		BYTE  bfThirdByte;     // for TGA/PCX checking
		DWORD bfSize;          // Size of file in bytes. DWORDS ![[e]]
	};

    WORD  bfReserved1;     // Ignored.
    WORD  bfReserved2;     // Ignored.
    DWORD bfOffBits;       // Offset of bitmap in file.
};


struct FBitmapInfoHeader
{
    DWORD biSize;          // Size of header in bytes.
    DWORD biWidth;         // Width of bitmap in pixels.
    DWORD biHeight;        // Height of bitmap in pixels.
    WORD  biPlanes;        // Number of bit planes (always 1).
    WORD  biBitCount;      // Number of bits per pixel.
    DWORD biCompression;   // Type of compression (ingored).
    DWORD biSizeImage;     // Size of pixel array (usually 0).
    DWORD biXPelsPerMeter; // Ignored.
    DWORD biYPelsPerMeter; // Ignored.
    DWORD biClrUsed;       // Number of colors (usually 0).
    DWORD biClrImportant;  // Important colors (usually 0).
};


//
// 128-byte header found at the beginning of a ".PCX" file.
//

struct FPCXFileHeader
{
	BYTE	Manufacturer;		// 0    Always 10.
	BYTE	Version;			// 1    PCX file version. // 0=bw
	BYTE	Encoding;			// 2    1=run-length, 0=none.
	BYTE	BitsPerPixel;		// 3    1,2,4, or 8.
	WORD	XMin;				// 4    Dimensions of the image.
	WORD	YMin;				// 6    Dimensions of the image.
	WORD	XMax;				// 8    Dimensions of the image.
	WORD	YMax;				// A    Dimensions of the image.
	WORD	hdpi;				// B    Horizontal printer resolution.
	WORD	vdpi;				// C    Vertical printer resolution.
	BYTE	OldColorMap[48];	// E    Old colormap info data.
	BYTE	Reserved1;			// 3E   Must be 0.
	BYTE	NumPlanes;			// 3F   Number of color planes  (1, 3, 4, etc).
	WORD	BytesPerLine;		// 40   Number of bytes per scanline.
	WORD	PaletteType;		// 42   How to interpret palette: 1=color, 2=gray.
	WORD	HScreenSize;		// 44   Horizontal monitor size.
	WORD	VScreenSize;		// 46   Vertical monitor size.
	BYTE	Reserved2[54];		// 48   Must be 0.  
};


//
// TARGA header
//

struct FTGAFileHeader
{
	BYTE	IDlen;       // Number of bytes in Ident Field.
	BYTE	ColmpType;   // Color map type (0 or don't care for unmapped RGB;  1 for color map).
	BYTE	ImageType;   // Image type:  2 is unmapped RGB. 10 = rle'd RGB
	WORD	CMorg;       // Color map origin:
	WORD	CMlen;       // Color map length in # of colors.
	BYTE	CMbits;      // Number of bits per color map entry.
	WORD	Xoff;		 //	
	WORD	Yoff;		 // 
	WORD	XSize;       // Width of the image.
	WORD	YSize;       // Height of the image.
	BYTE	BPix;        // Number of bits per pixel in picture.
	BYTE	ImageDesc;   // Always 0 for TGA and TARGA images.

	// >* Ident field, IDlen bytes long
	// >* Color map palette,  size (CMbits/8)* CMlen 
	// >* Image data, Xsize*Ysize * BPix/8
};


/*
	If the 2 lsb's of imagetype are 1, the data is for a greyscale image.
	For imagetype of 7 or less the file has uncompressed picture data. 
	For imagetype of 8 or more it is RLE compressed as follows:
     1. Get the code byte. The 'count' is {(code & 127) + 1} (1 thru 128).
     2. If the msb = 0, output the next 'count' colors.
     3. If the msb = 1, output the next color 'count' times.
	Targa 8 is usually gray scale. 
	For Targa 16, there are 2 bytes/color; 5 lsb's are blue, next 5 bits are green, next 5 bits are red. 
	For Targa 24, there are 3 ytes/color; 1st is blue, next is green, last is red. 
	For Targa 32, there are 4 bytes/color; 1st is blue, next is green, next is red, and last throw away.
*/


#pragma pack(pop)

/*
struct PCXheader
     {    BYTE Mfg;           // ID for ZSoft files, 0A hex   
          BYTE Ver;           // Version, 5   
          BYTE Encode;        // Encoded pic data if 1   
          BYTE Bpp;           // Bits/pixel   
          int  X1;            // Left col   
          int  Y1;            // Top row   
          int  X2;            // Right col   
          int  Y2;            // Bottom row   
          int  Xres;          // Horiz size of screen   
          int  Yres;          // Vert size of screen   
          BYTE zpal[48];      // 3 bytes/palette reg   
          BYTE Vmode;         // Video mode, ignored   
          BYTE Planes;        // # color planes in pic   
          int  Bpl;           // Bytes/line in pic   
     } ;
This is followed by 60 bytes of filler (0's), and then the picture data.
The number of colors is 1 << (Bpp * Planes).
If Ver = 5, go to the 769th byte from the end of the file. if this is 12, the
     next 768 bytes are byte triples (R, G, B) of palette data.
ZSoft files for VGA are 1 byte/pixel, and for EGA are on a plane basis
     (B, G, R, I) for each row.
ZSoft files are encoded as follows:
 1. Read a code byte. The 'count' is the 6 lsb's (63 max).
 2. If the 2 msb's of code = 1, the 'count' is the 6 lsb's (63 max).
    Then read a data byte and repeat it 'count' times to the output.
 3. If the 2 msb's of code = 0, the code byte is written directly to the output.
 4. If an output data byte of C0 - FF is required, it is written as 'C1h,data'
    where the C1h indicates a count of 1.
*/

#pragma warning (disable : 4035) // legalize implied return value

/*
inline BOOL FloatIsSmaller(float F1, float F2)
{
	// ONLY POSITIVE numbers

	// DWORD LocalCheck;
	__asm
	{
		mov eax,F1
		mov ebx,F2
		cmp eax,ebx
		sbb eax,eax
	}

	//if (LocalCheck )
	//	if (F1 >= F2) printf("###########mistake made 1");
	//if (! LocalCheck )
		//if (F1 < F2)  printf("###########mistake made 2");
	//return (LocalCheck !=0);

}
*/


inline BOOL FloatIsSmaller(float F1,float F2)
{
	return ( (*(DWORD*)&F1) < (*(DWORD*)&F2));
}


inline int FastRoundFtoI(float F)
{
	int Tempint;
	__asm
	{
		fld   [F]
		fistp [Tempint] // store simply with default rounding !
	}
	return (Tempint);
}

inline int OurFtoi(float F)
{
	int Tempint;
	__asm
	{
		fld   [F]
		fistp [Tempint] // store simply with default rounding !
	}
	return (Tempint);
}




//
// Simple fast buffered file class. To speed up output. TODO: extend to reading files ?
//

class FastFileClass
{
	public:

	HANDLE  FileHandle;
	BYTE*	Buffer;
	int		BufCount;
	int     Error;
	unsigned long BytesWritten;
	#define	BufSize (8192)
	
	// Ctor
	FastFileClass()
	{
		Buffer = NULL;
		BufCount = 0;
		Error = 0;
		Buffer = new BYTE[BufSize];
	}

	int CreateNewFile(char *FileName)
	{
		Error = 0;
		if ((FileHandle = CreateFile(FileName, GENERIC_WRITE,FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL))
		    == INVALID_HANDLE_VALUE)
		{
			Error = 1;
		}

		return Error;
	}

	void Write(void *Source,int ByteCount)
	{
		if ((BufCount+ByteCount) < BufSize)
		{
			memmove(&Buffer[BufCount],Source,ByteCount);
			BufCount += ByteCount;
		}
		else  
		if (ByteCount <= BufSize)
		{
			if (BufCount !=0)
			{
				WriteFile(FileHandle,Buffer,BufCount,&BytesWritten, NULL);
				BufCount = 0;
			}
			memmove(Buffer,Source,ByteCount);
			BufCount += ByteCount;
		}
		else // Too big a chunk to be buffered.
		{
			if (BufCount != 0) 
			{
				WriteFile(FileHandle,Buffer,BufCount,&BytesWritten,NULL);
				BufCount = 0;
			}
			WriteFile(FileHandle,Source,ByteCount,&BytesWritten,NULL);
		}
	}

	inline void WriteByte(const BYTE OutChar)
	{
		if (BufCount < BufSize)
		{
			Buffer[BufCount] = OutChar;
			BufCount++;
			return;
		}
		else
		{
			BYTE OutExtra = OutChar;
			Write(&OutExtra, 1);
		}
	}

	
	//
	//int Read(int ByteCount, BYTE* Destination)
	//{
	//}
	//

	int GetError()
	{
		return Error;
	}


	int Close()
	{
		// flush 
		if (BufCount)
		{
			WriteFile(FileHandle, Buffer, BufCount, &BytesWritten, NULL); 
			BufCount = 0;
			CloseHandle(FileHandle);
		}
		
		// deallocate
		if (Buffer) delete Buffer;

		return Error;
	}




};


// #define ext extern
    extern  int  UniqueStatic;

// Options set at command line.
	extern  int  RefScaling;
	extern  int  DitherSetting;
	extern  int  CreateRamp;
	extern  int  SampleMips;
	extern  int  MipsLimit;
	extern  int  ForcedPalette;		
	extern  int  DetailCommand;
	extern  int  QualityCommand;
	extern  int  WeighMethod;
	extern  int  GlobalTargetColors;
	extern  int  MultiFile;
	extern  int  CommonColors;
	extern  int  AutoCommon;
	extern  int  OverrideWrite;
	extern  int  Wildcards;
	extern  int  PinkScreen;
	extern  int  ColMask;
	extern  int  MaskColorR;
	extern  int  MaskColorG;
	extern  int  MaskColorB;
	extern  int   SizeBias;
	extern  int   ColorSpace;
	extern  int   BestQuant;
	extern  int   BinsLimit;
	extern  int   GlobalBlackForce;  
	extern  float Trans;
	extern  int   Allow8;
	extern  int   DebugInfo;
	extern  int   PcxOut;
	extern  int   MaskCheckRequested;
	extern	float MaskFatness;
	extern  float MaskMagic;
	extern  float ScaleX;
	extern  float ScaleY;
	extern  float CommonError;
	extern  int   VisibleAlpha;
	extern  int   MultiplyAlpha;
	extern  int   OnlyAlpha;

	extern  int   PinkMask;

	extern  int   BmpOut;
	extern  int   TgaOut;
	extern  int   PcxOut;

	extern  int   Limit;
	extern  float Gamma;
	extern  int   Balance;

	extern  int  Clustering;
	extern  int  OutputPalette;
	extern  int  WritePalette;

	extern  int  PalSmooth; // #debug
	extern  int  Modulate;  // #debug

	extern  int  Grey;
	extern  int  GreyPal;
	extern  int  CreateOutMask;
	extern  int  SmoothSample;
	extern  int  FullRGBAOutput;
	extern  float AlphaBias;

    int main(int argc, char** argv);

}; // namespace Bright
