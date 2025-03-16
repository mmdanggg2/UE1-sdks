/*
**
**	TextureClass.h 
**	Implementation of texture (image) class and misc functions
**	for use with the BRIGHT image processing utility.
**
*/

#pragma once

#include <windows.h>
#include <math.h>
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include <float.h>


namespace Bright
{
	
int		FastRoundFtoI(float F);
void    InitializeGlobals();
void	SetupStatistics(); 
void	CloseStatistics();
int		BinNumber();
void	SetupSampleDepth( int Red, int Green, int Blue, int Alpha, int CSpace);


class Texture
{
	public:
	// member vars

	// Flags/stats
	int    IDnumber;	//
	int    Status;		//
	int	   Modulate;	// if <> 0, modulated-palette will be created
	int    MipsNeeded;	// number of mipmaps needed to sample (could be 0)
	int    Dither;		//

	int    DestScaleX;  // if these are !=0 , they take precedence over the common scaling factor.
	int    DestScaleY;  //

	int			MaskChecked;  // checked for mask if >0
	int			MaskFlag;     // mask detected if >0 
	PalEntry	MaskColor;	  // main mask color 

	int			BlackForce;
	int			TargetColors;
	int         MakeStream; //

	float		Importance;   // relative importance, when doing sampling...

	char*		InFileName;
	char*		OutFileName;      
	char*       BaseFileName; // For numbered output...

	int			ImageHeight, ImageWidth;	// 
	Pixel24*	Bit24Map;					// 24-bit bitmap
	Pixel32*    Bit32Map;
	unsigned char* Input8BitField;			// When reading 8-bit pictures..
	BYTE*		InMask;
	BYTE*       StreamMap;                 // 8 bit input

	int			TempHeight, TempWidth;		//
	Pixel32*	TempMap;					// Allocated/deallocated for scaling etc..
	BYTE*		TempMask;					//

	int			OutHeight, OutWidth;		//
	BYTE*		Bit8Map;					// Output

	//float		ScaleX,ScaleY;				// Integer scaling -> GLOBALS for now..
	float		MaskTolerance;				// 'magic wand' tolerance YCC 

	int        InputPaletteSize;
	int        FinalPaletteSize;
	int        WeighedPaletteSize;

	PalEntry*			InputPalette;	// Filled if input was 8-bit picture.
	PalEntry*			FinalPalette;	//
	WeighedPalEntry*	WeighedPalette; // for the preliminary palette clustering.
	float*				PalWeights;      // palette weights...

	int					*MatchTableR,*MatchTableG,*MatchTableB,*MatchTableA;
	int					LastBestMatch;


	// constructor: set defaults & pre-allocate stuff.
	Texture()
	{
		InFileName  = new char[200];
		OutFileName = new char[200];
		BaseFileName = new char [200];
		InFileName[0] =  0;
		OutFileName[0] = 0;
		BaseFileName[0] = 0;

		MatchTableR = new int[512];
		MatchTableG = new int[512];
		MatchTableB = new int[512];
		MatchTableA = new int[512];

		DestScaleX = 0;
		DestScaleY = 0;

		BlackForce = 0;
		TargetColors = 0;
		MakeStream = 0;

		Modulate =0;
		Status = 0;
		static int UniqueID = 0;
		IDnumber = UniqueID++;
		MaskChecked = 0;
		MaskFlag = 0;
		MipsNeeded = 0;
		Importance = 1.0f;
		Dither = 0;

		Bit24Map =		NULL;
		Bit32Map =		NULL;
		Bit8Map  =		NULL;
		Input8BitField =NULL;
		TempMap     =	NULL;
		TempMask	=	NULL;
		InMask		=	NULL;
		StreamMap  =   NULL;

		// permanently allocated variables
		InputPalette  = new PalEntry[256];
		FinalPalette  = new PalEntry[256];

		WeighedPalette	= new WeighedPalEntry[256];
		PalWeights		= new float[256];

	    InputPaletteSize = 0;
		FinalPaletteSize = 0;
		WeighedPaletteSize = 0;
	}

	void SleepTexture() // delete all allocs except its name and temp palette.
	{
		// Assumed all fields were allocated using NEW...

		if (Bit24Map)		delete (Bit24Map);
		if (Bit32Map)		delete (Bit32Map);
		if (Input8BitField) delete (Input8BitField);
		if (Bit8Map)		delete (Bit8Map);
		if (TempMap)        delete (TempMap);
		if (TempMask)       delete (TempMask);
		if (InMask)         delete (InMask);

		Bit24Map =		 NULL;
		Bit32Map =		 NULL;
		Input8BitField = NULL;
		Bit8Map  =		 NULL;
		TempMap  =		 NULL;
		TempMask =		 NULL;
		InMask   =       NULL;
	}

	// Destructor
	~Texture()
	{
	}
	
	// Interface.

	void  SetDiskFileName(char*  DFname)
	{
		strcpy(InFileName,DFname);
	}

	void  SetOutFileName(char* DFname)
	{
		strcpy(OutFileName, DFname);
	}

	void  SetOutFileNameNumbered(char* DFname)
	{
		char CharBuf[40];
		strcpy(OutFileName, strcat( _itoa(UniqueStatic,CharBuf,10), DFname) );
		UniqueStatic++;
	}



	void  ShowStats();		// Shows sizes 
	void  SampleColors(INT MaxAllowed); // can be cumulative.
	void  QuantizePalette();
	void  QuantizeWeighedPalette();

	void  CreateBit8Map();   // Creates & maps the properly scaled output picture with FinalPalette.
	int   WriteBit8Map();

	int   WriteBit24Map(); // write Bit24Map to disk using OutFile name ( always BMP.. )
	int   WriteBit32Map(); // write Bit32Map to disk using OutFile name ( always targa ? )

	// palette functions
	void SortPaletteByBrightness();
	void ReorderPalette();
	void SmoothPalette();

	void DetectAndEqualizeMask();

	void ConvertBGRtoRGB();

	int ReadTextureFile();
	int  WriteBMP24File();
	int  ReadPCXFile();
	int  ReadTGAFile();

	// Experimental
	void CleanupTransparency();
	void TestScaling();
	void SynthesizeImage();
	void CreateBumpMap(float BumpBias);
	void CreateStreamMap(float StreamValue);

	// private parts
	private:
	HANDLE		hFile;	/* Handle of input file. */

	Texture*  Associated; //reserved 
	void  ScaleTexture(int MipLevel);
	void  DetermineMipNumber(); // fills in MipsNeeded.
	void  FixPalette();
	void  FixWeighedPalette();
	void  DetectMainMask();
	void  CreateMainMask();

	int BestDitherColor(int r, int g, int b);
	int BestColor(int r, int g, int b, int a);


	void BuildColorMatchTables();

};

}; // namespace Bright
