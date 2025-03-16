//
// KTexComp public header file.
//
// Copyright (c) 2016 Intel Corporation
// Copyright (c) 2020 Sebastian Kaufel
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this 
// software and associated documentation files (the "Software"), to deal in the Software 
// without restriction, including without limitation the rights to use, copy, modify, 
// merge, publish, distribute, sublicense, and/or sell copies of the Software, and to 
// permit persons to whom the Software is furnished to do so, subject to the following 
// conditions: 
//
// The above copyright notice and this permission notice shall be included in all copies 
// or substantial portions of the Software.  
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
// CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
// OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#ifndef ISPCTC_NO_STDINT_INCLUDE
	#include <stdint.h>
#endif

#if __cplusplus
extern "C" 
{
#endif

//
// Shared.
//

struct rgba_surface 
{
	uint8_t* ptr;
	int32_t width;
	int32_t height;
	int32_t stride; // in bytes
};

struct rg_surface 
{
	uint8_t* ptr;
	int32_t width;
	int32_t height;
	int32_t stride; // in bytes
};

struct red_surface 
{
	uint8_t* ptr;
	int32_t width;
	int32_t height;
	int32_t stride; // in bytes
};

// Surface definitions.
#define ISPCTC_DELARE_SURFACE(name) \
	struct name \
	{ \
		uint8_t* Ptr; \
		int32_t  Width; \
		int32_t  Height; \
		int32_t  Stride; \
	};

// 8 bit unsigned int/component
ISPCTC_DELARE_SURFACE(ISPCTC_Surface_R8)
ISPCTC_DELARE_SURFACE(ISPCTC_Surface_RG8)
ISPCTC_DELARE_SURFACE(ISPCTC_Surface_RGBA8)

// 8 bit signed int/component
ISPCTC_DELARE_SURFACE(ISPCTC_Surface_R8S)
ISPCTC_DELARE_SURFACE(ISPCTC_Surface_RG8S)

// 16 bit unsigned int/component
ISPCTC_DELARE_SURFACE(ISPCTC_Surface_R16)
ISPCTC_DELARE_SURFACE(ISPCTC_Surface_RG16)
ISPCTC_DELARE_SURFACE(ISPCTC_Surface_RGBA16)

// 16 bit signed int/component
ISPCTC_DELARE_SURFACE(ISPCTC_Surface_R16S)
ISPCTC_DELARE_SURFACE(ISPCTC_Surface_RG16S)

// 16 bit float/component.
ISPCTC_DELARE_SURFACE(ISPCTC_Surface_R16F)
ISPCTC_DELARE_SURFACE(ISPCTC_Surface_RG16F)
ISPCTC_DELARE_SURFACE(ISPCTC_Surface_RGBA16F)

// 32 bit float/component
ISPCTC_DELARE_SURFACE(ISPCTC_Surface_R32F)
ISPCTC_DELARE_SURFACE(ISPCTC_Surface_RG32F)
ISPCTC_DELARE_SURFACE(ISPCTC_Surface_RGBA32F)

//
// BC1-BC5.
//

// BC1.
void BC1EncodeRGBA8( const ISPCTC_Surface_RGBA8* InputSurface, uint8_t* OutputBlocks );
void BC1EncodeRGBA16( const ISPCTC_Surface_RGBA16* InputSurface, uint8_t* OutputBlocks );
//void BC1EncodeRGBA32F( const ISPCTC_Surface_RGBA32F* InputSurface, uint8_t* OutputBlocks );

// BC1 Punchthrough Alpha.
void BC1PAEncodeRGBA8( const ISPCTC_Surface_RGBA8* InputSurface, uint8_t* OutputBlocks );
void BC1PAEncodeRGBA16( const ISPCTC_Surface_RGBA16* InputSurface, uint8_t* OutputBlocks );
//void BC1PAEncodeRGBA32F( const ISPCTC_Surface_RGBA32F* InputSurface, uint8_t* OutputBlocks );


// BC2.
void BC2EncodeRGBA8( const ISPCTC_Surface_RGBA8* InputSurface, uint8_t* OutputBlocks );
void BC2EncodeRGBA16( const ISPCTC_Surface_RGBA16* InputSurface, uint8_t* OutputBlocks );
//void BC2Encode_RGBA32F( const ISPCTC_Surface_RGBA32F* InputSurface, uint8_t* OutputBlocks );

// BC3.
void BC3EncodeRGBA8( const ISPCTC_Surface_RGBA8* InputSurface, uint8_t* OutputBlocks );
void BC3EncodeRGBA16( const ISPCTC_Surface_RGBA16* InputSurface, uint8_t* OutputBlocks );
//void BC3Encode_RGBA32F( const ISPCTC_Surface_RGBA32F* InputSurface, uint8_t* OutputBlocks );

// BC4.
void BC4EncodeR8( const ISPCTC_Surface_R8* InputSurface, uint8_t* OutputBlocks );
void BC4EncodeR16( const ISPCTC_Surface_R16* InputSurface, uint8_t* OutputBlocks );
//void BC4EncodeR32F( const ISPCTC_Surface_R32F* InputSurface, uint8_t* OutputBlocks );
//void BC4SEncodeR8S( const ISPCTC_Surface_R8S* InputSurface, uint8_t* OutputBlocks );
//void BC4SEncodeR16S( const ISPCTC_Surface_R16S* InputSurface, uint8_t* OutputBlocks );
//void BC4SEncodeSR32F( const ISPCTC_Surface_R32F* InputSurface, uint8_t* OutputBlocks );

// BC5.
void BC5EncodeRG8( const ISPCTC_Surface_RG8* InputSurface, uint8_t* OutputBlocks );
void BC5EncodeRG16( const ISPCTC_Surface_RG16* InputSurface, uint8_t* OutputBlocks );
//void BC5EncodeRG32F( const ISPCTC_Surface_RG32F* InputSurface, uint8_t* OutputBlocks );
//void BC5SEncodeRG8S( const ISPCTC_Surface_RG8S* InputSurface, uint8_t* OutputBlocks );
//void BC5SEncodeRG16S( const ISPCTC_Surface_RG16S* InputSurface, uint8_t* OutputBlocks );
//void BC5SEncodeRG32F( const ISPCTC_Surface_RG32F* InputSurface, uint8_t* OutputBlocks );

// BC1.
//void BC1DecodeRGBA8( const uint8_t* InputBlocks, const ISPCTC_Surface_RGBA8* OutputSurface );
//void BC1DecodeRGBA16( const uint8_t* InputBlocks, const ISPCTC_Surface_RGBA16* OutputSurface );
//void BC1DecodeRGBA32F( const uint8_t* InputBlocks, const ISPCTC_Surface_RGBA32F* OutputSurface );

// BC1 Punchthrough Alpha.
//void BC1PADecodeRGBA8( const uint8_t* InputBlocks, const ISPCTC_Surface_RGBA8* OutputSurface );
//void BC1PADecodeRGBA16( const uint8_t* InputBlocks, const ISPCTC_Surface_RGBA16* OutputSurface );
//void BC1PADecodeRGBA32F( const uint8_t* InputBlocks, const ISPCTC_Surface_RGBA32F* OutputSurface );

// BC2.
//void BC2DecodeRGBA8( const uint8_t* InputBlocks, const ISPCTC_Surface_RGBA8* OutputSurface );
//void BC2DecodeRGBA16( const uint8_t* InputBlocks, const ISPCTC_Surface_RGBA16* OutputSurface );
//void BC2DecodeRGBA32F( const uint8_t* InputBlocks, const ISPCTC_Surface_RGBA32F* OutputSurface );

// BC3.
//void BC3DecodeRGBA8( const uint8_t* InputBlocks, const ISPCTC_Surface_RGBA8* OutputSurface );
//void BC3DecodeRGBA16( const uint8_t* InputBlocks, const ISPCTC_Surface_RGBA16* OutputSurface );
//void BC3DecodeRGBA32F( const uint8_t* InputBlocks, const ISPCTC_Surface_RGBA32F* OutputSurface );

// BC4.
//void BC4DecodeR8( const uint8_t* InputBlocks, const ISPCTC_Surface_R8* OutputSurface );
//void BC4DecodeR16( const uint8_t* InputBlocks, const ISPCTC_Surface_R16* OutputSurface );
//void BC4DecodeR32F( const uint8_t* InputBlocks, const ISPCTC_Surface_R32F* OutputSurface );
//void BC4DecodeSigned_R8S( const uint8_t* InputBlocks, const ISPCTC_Surface_R8S* OutputSurface );
//void BC4DecodeSigned_R16S( const uint8_t* InputBlocks, const ISPCTC_Surface_R16S* OutputSurface );
//void BC4DecodeSigned_R32F( const uint8_t* InputBlocks, const ISPCTC_Surface_R32F* OutputSurface );

// BC5.
//void BC5DecodeRG8( const uint8_t* InputBlocks, const ISPCTC_Surface_RG8* OutputSurface );
//void BC5DecodeR16( const uint8_t* InputBlocks, const ISPCTC_Surface_RG16* OutputSurface );
//void BC5DecodeR32F( const uint8_t* InputBlocks, const ISPCTC_Surface_RG32F* OutputSurface );
//void BC5SDecodeR8S( const uint8_t* InputBlocks, const ISPCTC_Surface_RG8S* OutputSurface );
//void BC5SDecodeR16S( const uint8_t* InputBlocks, const ISPCTC_Surface_RG16S* OutputSurface );
//void BC5SDecodeR32F( const uint8_t* InputBlocks, const ISPCTC_Surface_RG32F* OutputSurface );







//
// BC6H and BC7.
//


struct bc6h_enc_settings
{
	bool slow_mode;
	bool fast_mode;
	int refineIterations_1p;
	int refineIterations_2p;
	int fastSkipTreshold;
};

// profiles for BC6H (RGB HDR)
void GetProfile_bc6h_veryfast(bc6h_enc_settings* settings);
void GetProfile_bc6h_fast(bc6h_enc_settings* settings);
void GetProfile_bc6h_basic(bc6h_enc_settings* settings);
void GetProfile_bc6h_slow(bc6h_enc_settings* settings);
void GetProfile_bc6h_veryslow(bc6h_enc_settings* settings);

struct bc7_enc_settings
{
	bool ModeEnabled[8];
	int RefineIterations[8];

	int fastSkipTreshold_mode1;
	int fastSkipTreshold_mode3;
	int fastSkipTreshold_mode7;

	int mode45_channel0;
	int refineIterations_channel;

	int channels;
	bool IgnoreAlphaError;
};

// profiles for RGB data (alpha channel will be ignored)
void GetProfile_ultrafast(bc7_enc_settings* settings);
void GetProfile_veryfast(bc7_enc_settings* settings);
void GetProfile_fast(bc7_enc_settings* settings);
void GetProfile_basic(bc7_enc_settings* settings);
void GetProfile_slow(bc7_enc_settings* settings);
void GetProfile_veryslow(bc7_enc_settings* settings);

// profiles for RGBA inputs
void GetProfile_alpha_ultrafast(bc7_enc_settings* settings);
void GetProfile_alpha_veryfast(bc7_enc_settings* settings);
void GetProfile_alpha_fast(bc7_enc_settings* settings);
void GetProfile_alpha_basic(bc7_enc_settings* settings);
void GetProfile_alpha_slow(bc7_enc_settings* settings);
void GetProfile_alpha_veryslow(bc7_enc_settings* settings);
void GetProfile_veryslow(bc7_enc_settings* settings);

// For library internal development use only.
void GetProfile_development( bc7_enc_settings* Settings );
void GetProfile_bc6h_development( bc6h_enc_settings* Settings );


// BC6H.
void BC6HEncodeRGBA16F( const ISPCTC_Surface_RGBA16F* InputSurface, uint8_t* OutputBlocks, bc6h_enc_settings* EncSettings );
//void BC6HEncodeRGBA32F( const ISPCTC_Surface_RGBA32F* InputSurface, uint8_t* OutputBlocks, bc6h_enc_settings* EncSettings );
//void BC6HSEncodeRGBA16F( const ISPCTC_Surface_RGBA16F* InputSurface, uint8_t* OutputBlocks, bc6h_enc_settings* EncSettings );
//void BC6HSEncodeRGBA32F( const ISPCTC_Surface_RGBA32F* InputSurface, uint8_t* OutputBlocks, bc6h_enc_settings* EncSettings );
//void BC6HDecodeRGBA16F( const uint8_t* InputBlocks, const ISPCTC_Surface_RGBA16F* OutputSurface );
//void BC6HDecodeRGBA32F( const uint8_t* InputBlocks, const ISPCTC_Surface_RGBA32F* OutputSurface );
//void BC6HSDecodeRGBA16F( const uint8_t* InputBlocks, const ISPCTC_Surface_RGBA16F* OutputSurface );
//void BC6HSDecodeRGBA32F( const uint8_t* InputBlocks, const ISPCTC_Surface_RGBA32F* OutputSurface );

// BC7.
void BC7EncodeRGBA8( const ISPCTC_Surface_RGBA8* InputSurface, uint8_t* OutputBlocks, bc7_enc_settings* EncSettings );
void BC7EncodeRGBA16( const ISPCTC_Surface_RGBA16* InputSurface, uint8_t* OutputBlocks, bc7_enc_settings* EncSettings );
//void BC7EncodeRGBA32F( const ISPCTC_Surface_RGBA32F* InputSurface, uint8_t* OutputBlocks, bc7_enc_settings* EncSettings );
//void BC7DecodeRGBA8( const uint8_t* InputBlocks, const ISPCTC_Surface_RGBA8* OutputSurface );
//void BC7DecodeRGBA16( const uint8_t* InputBlocks, const ISPCTC_Surface_RGBA16* OutputSurface );
//void BC7DecodeRGBA32F( const uint8_t* InputBlocks, const ISPCTC_Surface_RGBA32F* OutputSurface );



//
// ETC, ETC2 and EAC.
//

struct KETC1EncodeSettings;

int32_t KETC1EncodeSettingsSize();

// profiles for ETC
void GetProfile_etc_slow(KETC1EncodeSettings* settings);

// Encoding.
void ETC1EncodeRGBA8( const ISPCTC_Surface_RGBA8* InputSurface, uint8_t* OutputBlocks, KETC1EncodeSettings* Settings );
void ETC1EncodeRGBA16( const ISPCTC_Surface_RGBA16* InputSurface, uint8_t* OutputBlocks, KETC1EncodeSettings* Settings );
//void ETC1EncodeRGBA32F( const ISPCTC_Surface_RGBA32F* InputSurface, uint8_t* OutputBlocks, KETC1EncodeSettings* Settings );

// Decoding.
//void ETC1DecodeRGBA8( const uint8_t* InputBlocks, const ISPCTC_Surface_RGBA8* OutputSurface );
//void ETC1DecodeRGBA16( const uint8_t* InputBlocks, const ISPCTC_Surface_RGBA16* OutputSurface );
//void ETC1DecodeRGBA32F( const uint8_t* InputBlocks, const ISPCTC_Surface_RGBA32F* OutputSurface );



//
// ASTC.
//

struct astc_enc_settings // !! Add LDR as setting
{
	int block_width;
	int block_height;
	int channels;

	int fastSkipTreshold;
	int refineIterations;
};

// profiles for ASTC
void GetProfile_astc_fast(astc_enc_settings* settings, int block_width, int block_height);
void GetProfile_astc_alpha_fast(astc_enc_settings* settings, int block_width, int block_height);
void GetProfile_astc_alpha_slow(astc_enc_settings* settings, int block_width, int block_height);

// Encodeion.
void ASTCEncodeRGBA8( const ISPCTC_Surface_RGBA8* InputSurface, uint8_t* OutputBlocks, astc_enc_settings* EncSettings );
//void ASTCEncodeRGBA16( const ISPCTC_Surface_RGBA16* InputSurface, uint8_t* OutputBlocks, astc_enc_settings* EncSettings );
//void ASTCEncodeRGBA32F( const ISPCTC_Surface_RGBA32F* InputSurface, uint8_t* OutputBlocks, astc_enc_settings* EncSettings );

// Decodeion.
//void ASTCDecodeRGBA8( const uint8_t* InputBlocks, const ISPCTC_Surface_RGBA8* OutputSurface );
//void ASTCDecodeRGBA16( const uint8_t* InputBlocks, const ISPCTC_Surface_RGBA16* OutputSurface );
//void ASTCDecodeRGBA32F( const uint8_t* InputBlocks, const ISPCTC_Surface_RGBA32F* OutputSurface );




//
// Junk to weed out.
//

// helper function to replicate border pixels for the desired block sizes (bpp = 32 or 64)
void ReplicateBorders(rgba_surface* dst_slice, const rgba_surface* src_tex, int x, int y, int bpp);

/*
	Prototyping for new API. Subject to frequent change for now.

	Ideas:
		- Add size parameters similiar to snprintf vs. sprintf.
		- Add a return value, to indicate wrong input/output memory size or internal errors which are currently asserts.
		- Interface for BC6H or floating point input in general could have a version which does clamping, and one which doesnt.
		  - There is a closed thread in the bugtracker requesting input should always be clamped, but was rejected due to eventual
			  performance concerns. This needs some benchmarking, but my expectation is that it would at most would only matter for ultrafast profiles.
		- Use compiler pragmas/directives to nail down alignment of ISPCTC_Surface_* structures, so these remain compatible between with different alignment
		  settings (they might actually already be implicit safe due to how they would be aligned).
		- Add settings/profiles where missing.
		- Choose a suitable approach for dealing with profiles:
		  a) Only expose generic options as ultrafast/slow/etc. to user.
			b) Expose full control over individual parts of the compression profile
			   - Requires proper documentation
				 - Deal with future changes/addition to the profile structures.
				   - One way could be to expose function which returns the size of the profile struct used by the current build of the library beeing used
					   and provide wrapper functions to set individual members of the profile struct. That way code using the library could be written which remains
						 fully functional regardless of changes of the settings struct inside the library itself.
		- Expose control over the error metrics used for the compression:
		  - Explicit control.
			  - Weight of individual axis as exposed in s3tc.lib or squish
				  - e.g. using relativ luminance weights for ITU-R BT.709/sRGB primaries (0.2126,0.7152,0.0722) for linear data or for images encoded in sRGB space
					  using using an linear approximation of weights to relative luminance calculation of sRGB (0.309 0.609,0.082)
						[See: Burger, Burge: Principles of Digital Image Processing: Core Algorithms, Springer Verlag London Limited, 2009, p. 111.].
				  - Weight of alpha could be used for block compression codecs where alpha is not independent (e.g. BC7).
		  - This could also be generic, as in profiles for, lets say UI textures in sRGB or linearized sRGB gamut, for diffuse, roughness, ao, which may or may
			  not have more specific concerns about which errors are more important to minimize for the usage case.
		- Decompression:
		  - Expose control about rounding to integer types?
			- Having the output surface implicit specify the width/height of input feels a little off.
		- Choose a suitable approach to deal with non block sized input:
		  a) Disallow, and eventually still provide the ReplicateBorders helper and add support for 8/16/128 bpp to it.
			b) Allow/encourage
			   - Stride could still be required to be a multiple of the block size.
				 - For S3TC and power of two textures, the data inside a partial block can always be dublicated to the entire block to ensure weight of the original
				   meaningful pixels is preserved, while for non power of texture (e.g. 1x3 pixels) this cannot be done, so masking could be applied to preserve the original
					 weight in this case.
				 - BC6H/BC7/ETC1 could be really clever about using that extra information.
				 - This will affect quality for the highest mip levels.
*/

//
// Legacy ispc_texcomp.
//

/*
	Notes:
	 - input width and height need to be a multiple of block size
	 - BC4 input is 8 bit/pixel, BC5 input is 16 bit/pixel, otherwise LDR input is 32 bit/pixel (sRGB) and HDR is 64 bit/pixel (half float)
	 - dst buffer must be allocated with enough space for the compressed texture:
	   4 bytes/block for BC1/BC4/ETC1, 8 bytes/block for BC2/BC3/BC5/BC6H/BC7/ASTC
	   the blocks are stored in raster scan order (natural CPU texture layout)
	 - BC6H input needs to be non negative.
	 - you can use GetProfile_* functions to select various speed/quality tradeoffs.
	 - the RGB profiles are slightly faster as they ignore the alpha channel
*/

#if __cplusplus
}
#endif

//
// The End.
//
