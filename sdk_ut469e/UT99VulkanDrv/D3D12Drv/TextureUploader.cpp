
#include "Precomp.h"
#include "TextureUploader.h"
#include <map>

#ifdef USE_SSE2
#include <immintrin.h>
#endif

TextureUploader* TextureUploader::GetUploader(ETextureFormat format)
{
	static std::map<ETextureFormat, std::unique_ptr<TextureUploader>> Uploaders;
	if (Uploaders.empty())
	{
#if !defined(OLDUNREAL469SDK)

		Uploaders[TEXF_P8].reset(new TextureUploader_P8());
		Uploaders[TEXF_RGBA7].reset(new TextureUploader_BGRA8_LM());
		Uploaders[TEXF_RGB16].reset(new TextureUploader_Simple(DXGI_FORMAT_B5G6R5_UNORM, 2));
		Uploaders[TEXF_DXT1].reset(new TextureUploader_4x4Block(DXGI_FORMAT_BC1_UNORM, 8));
		Uploaders[TEXF_RGB8].reset(new TextureUploader_RGB8());
		Uploaders[TEXF_RGBA8].reset(new TextureUploader_Simple(DXGI_FORMAT_B8G8R8A8_UNORM, 4));

#else
		// Original.
		Uploaders[TEXF_P8].reset(new TextureUploader_P8());
		Uploaders[TEXF_BGRA8_LM].reset(new TextureUploader_BGRA8_LM());
		Uploaders[TEXF_R5G6B5].reset(new TextureUploader_Simple(DXGI_FORMAT_B5G6R5_UNORM, 2));
		Uploaders[TEXF_BC1].reset(new TextureUploader_4x4Block(DXGI_FORMAT_BC1_UNORM, 8));
		Uploaders[TEXF_RGB8].reset(new TextureUploader_RGB8());
		Uploaders[TEXF_BGRA8].reset(new TextureUploader_Simple(DXGI_FORMAT_B8G8R8A8_UNORM, 4));

		// S3TC (continued).
		Uploaders[TEXF_BC2].reset(new TextureUploader_4x4Block(DXGI_FORMAT_BC2_UNORM, 16));
		Uploaders[TEXF_BC3].reset(new TextureUploader_4x4Block(DXGI_FORMAT_BC3_UNORM, 16));

		// RGTC.
		Uploaders[TEXF_BC4].reset(new TextureUploader_4x4Block(DXGI_FORMAT_BC4_UNORM, 8));
		Uploaders[TEXF_BC4_S].reset(new TextureUploader_4x4Block(DXGI_FORMAT_BC4_SNORM, 8));
		Uploaders[TEXF_BC5].reset(new TextureUploader_4x4Block(DXGI_FORMAT_BC5_UNORM, 16));
		Uploaders[TEXF_BC5_S].reset(new TextureUploader_4x4Block(DXGI_FORMAT_BC5_SNORM, 16));

		// BPTC.
		Uploaders[TEXF_BC7].reset(new TextureUploader_4x4Block(DXGI_FORMAT_BC7_UNORM, 16));
		Uploaders[TEXF_BC6H_S].reset(new TextureUploader_4x4Block(DXGI_FORMAT_BC6H_SF16, 16));
		Uploaders[TEXF_BC6H].reset(new TextureUploader_4x4Block(DXGI_FORMAT_BC6H_UF16, 16));

		// Normalized RGBA.
		Uploaders[TEXF_RGBA16].reset(new TextureUploader_Simple(DXGI_FORMAT_R16G16B16A16_UNORM, 8));
		Uploaders[TEXF_RGBA16_S].reset(new TextureUploader_Simple(DXGI_FORMAT_R16G16B16A16_SNORM, 8));
		//Uploaders[TEXF_RGBA32].reset(new TextureUploader_Simple(DXGI_FORMAT_R32G32B32A32_UNORM, 16));
		//Uploaders[TEXF_RGBA32_S].reset(new TextureUploader_Simple(DXGI_FORMAT_R32G32B32A32_SNORM, 16));

		// S3TC (continued).
		Uploaders[TEXF_BC1_PA].reset(new TextureUploader_4x4Block(DXGI_FORMAT_BC1_UNORM, 8));

		// Normalized RGBA (continued).
		Uploaders[TEXF_R8].reset(new TextureUploader_Simple(DXGI_FORMAT_R8_UNORM, 1));
		Uploaders[TEXF_R8_S].reset(new TextureUploader_Simple(DXGI_FORMAT_R8_SNORM, 1));
		Uploaders[TEXF_R16].reset(new TextureUploader_Simple(DXGI_FORMAT_R16_UNORM, 2));
		Uploaders[TEXF_R16_S].reset(new TextureUploader_Simple(DXGI_FORMAT_R16_SNORM, 2));
		//Uploaders[TEXF_R32].reset(new TextureUploader_Simple(DXGI_FORMAT_R32_UNORM, 4));
		//Uploaders[TEXF_R32_S].reset(new TextureUploader_Simple(DXGI_FORMAT_R32_SNORM, 4));
		Uploaders[TEXF_RG8].reset(new TextureUploader_Simple(DXGI_FORMAT_R8G8_UNORM, 2));
		Uploaders[TEXF_RG8_S].reset(new TextureUploader_Simple(DXGI_FORMAT_R8G8_SNORM, 2));
		Uploaders[TEXF_RG16].reset(new TextureUploader_Simple(DXGI_FORMAT_R16G16_UNORM, 4));
		Uploaders[TEXF_RG16_S].reset(new TextureUploader_Simple(DXGI_FORMAT_R16G16_SNORM, 4));
		//Uploaders[TEXF_RG32].reset(new TextureUploader_Simple(DXGI_FORMAT_R32G32_UNORM, 8));
		//Uploaders[TEXF_RG32_S].reset(new TextureUploader_Simple(DXGI_FORMAT_R32G32_SNORM, 8));
		//Uploaders[TEXF_RGB8_S].reset(new TextureUploader_Simple(DXGI_FORMAT_R8G8B8_SNORM, 3));
		//Uploaders[TEXF_RGB16_].reset(new TextureUploader_Simple(DXGI_FORMAT_R16G16B16_UNORM, 6));
		//Uploaders[TEXF_RGB16_S].reset(new TextureUploader_Simple(DXGI_FORMAT_R16G16B16_SNORM, 6));
		//Uploaders[TEXF_RGB32].reset(new TextureUploader_Simple(DXGI_FORMAT_R32G32B32_UNORM, 12));
		//Uploaders[TEXF_RGB32_S].reset(new TextureUploader_Simple(DXGI_FORMAT_R32G32B32_SNORM, 12));
		Uploaders[TEXF_RGBA8_].reset(new TextureUploader_Simple(DXGI_FORMAT_R8G8B8A8_UNORM, 4));
		Uploaders[TEXF_RGBA8_S].reset(new TextureUploader_Simple(DXGI_FORMAT_R8G8B8A8_SNORM, 4));

		// Floating point RGBA.
		//Uploaders[TEXF_R16_F].reset(new TextureUploader_Simple(DXGI_FORMAT_R16_SFLOAT, 2));
		//Uploaders[TEXF_R32_F].reset(new TextureUploader_Simple(DXGI_FORMAT_R32_SFLOAT, 4));
		//Uploaders[TEXF_RG16_F].reset(new TextureUploader_Simple(DXGI_FORMAT_R16G16_SFLOAT, 4));
		//Uploaders[TEXF_RG32_F].reset(new TextureUploader_Simple(DXGI_FORMAT_R32G32_SFLOAT, 8));
		//Uploaders[TEXF_RGB16_F].reset(new TextureUploader_Simple(DXGI_FORMAT_R16G16B16_SFLOAT, 6));
		//Uploaders[TEXF_RGB32_F].reset(new TextureUploader_Simple(DXGI_FORMAT_R32G32B32_SFLOAT, 12));
		//Uploaders[TEXF_RGBA16_F].reset(new TextureUploader_Simple(DXGI_FORMAT_R16G16B16A16_SFLOAT, 8));
		//Uploaders[TEXF_RGBA32_F].reset(new TextureUploader_Simple(DXGI_FORMAT_R32G32B32A32_SFLOAT, 16));

		// ETC1/ETC2/EAC.
		//Uploaders[TEXF_ETC1].reset(new TextureUploader_4x4Block(DXGI_FORMAT_ETC1_R8G8B8_UNORM_BLOCK, 8));
		//Uploaders[TEXF_ETC2].reset(new TextureUploader_4x4Block(DXGI_FORMAT_ETC2_R8G8B8_UNORM_BLOCK, 8));
		//Uploaders[TEXF_ETC2_PA].reset(new TextureUploader_4x4Block(DXGI_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK, 8));
		//Uploaders[TEXF_ETC2_RGB_EAC_A].reset(new TextureUploader_4x4Block(DXGI_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK, 16));
		//Uploaders[TEXF_EAC_R].reset(new TextureUploader_4x4Block(DXGI_FORMAT_EAC_R11_UNORM_BLOCK, 8));
		//Uploaders[TEXF_EAC_R_S].reset(new TextureUploader_4x4Block(DXGI_FORMAT_EAC_R11_SNORM_BLOCK, 8));
		//Uploaders[TEXF_EAC_RG].reset(new TextureUploader_4x4Block(DXGI_FORMAT_EAC_R11G11_UNORM_BLOCK, 16));
		//Uploaders[TEXF_EAC_RG_S].reset(new TextureUploader_4x4Block(DXGI_FORMAT_EAC_R11G11_SNORM_BLOCK, 16));

		// ASTC.
		//Uploaders[TEXF_ASTC_4x4].reset(new TextureUploader_2DBlock(DXGI_FORMAT_ASTC_4x4_UNORM_BLOCK, 4, 4, 16));
		//Uploaders[TEXF_ASTC_5x4].reset(new TextureUploader_2DBlock(DXGI_FORMAT_ASTC_5x4_UNORM_BLOCK, 5, 4, 16));
		//Uploaders[TEXF_ASTC_5x5].reset(new TextureUploader_2DBlock(DXGI_FORMAT_ASTC_5x5_UNORM_BLOCK, 5, 5, 16));
		//Uploaders[TEXF_ASTC_6x5].reset(new TextureUploader_2DBlock(DXGI_FORMAT_ASTC_6x5_UNORM_BLOCK, 6, 5, 16));
		//Uploaders[TEXF_ASTC_6x6].reset(new TextureUploader_2DBlock(DXGI_FORMAT_ASTC_6x6_UNORM_BLOCK, 6, 6, 16));
		//Uploaders[TEXF_ASTC_8x5].reset(new TextureUploader_2DBlock(DXGI_FORMAT_ASTC_8x5_UNORM_BLOCK, 8, 5, 16));
		//Uploaders[TEXF_ASTC_8x6].reset(new TextureUploader_2DBlock(DXGI_FORMAT_ASTC_8x6_UNORM_BLOCK, 8, 6, 16));
		//Uploaders[TEXF_ASTC_8x8].reset(new TextureUploader_2DBlock(DXGI_FORMAT_ASTC_8x8_UNORM_BLOCK, 8, 8, 16));
		//Uploaders[TEXF_ASTC_10x5].reset(new TextureUploader_2DBlock(DXGI_FORMAT_ASTC_10x5_UNORM_BLOCK, 10, 5, 16));
		//Uploaders[TEXF_ASTC_10x6].reset(new TextureUploader_2DBlock(DXGI_FORMAT_ASTC_10x6_UNORM_BLOCK, 10, 6, 16));
		//Uploaders[TEXF_ASTC_10x8].reset(new TextureUploader_2DBlock(DXGI_FORMAT_ASTC_10x8_UNORM_BLOCK, 10, 8, 16));
		//Uploaders[TEXF_ASTC_10x10].reset(new TextureUploader_2DBlock(DXGI_FORMAT_ASTC_10x10_UNORM_BLOCK, 10, 10, 16));
		//Uploaders[TEXF_ASTC_12x10].reset(new TextureUploader_2DBlock(DXGI_FORMAT_ASTC_12x10_UNORM_BLOCK, 12, 10, 16));
		//Uploaders[TEXF_ASTC_12x12].reset(new TextureUploader_2DBlock(DXGI_FORMAT_ASTC_12x12_UNORM_BLOCK, 12, 12, 16));
		//Uploaders[TEXF_ASTC_3x3x3].reset(new TextureUploader_3DBlock(DXGI_FORMAT_ASTC_3x3x3_UNORM_BLOCK, 3, 3, 3, 16));
		//Uploaders[TEXF_ASTC_4x3x3].reset(new TextureUploader_3DBlock(DXGI_FORMAT_ASTC_4x3x3_UNORM_BLOCK, 4, 3, 3, 16));
		//Uploaders[TEXF_ASTC_4x4x3].reset(new TextureUploader_3DBlock(DXGI_FORMAT_ASTC_4x4x3_UNORM_BLOCK, 4, 4, 3, 16));
		//Uploaders[TEXF_ASTC_4x4x4].reset(new TextureUploader_3DBlock(DXGI_FORMAT_ASTC_4x4x4_UNORM_BLOCK, 4, 4, 4, 16));
		//Uploaders[TEXF_ASTC_5x4x4].reset(new TextureUploader_3DBlock(DXGI_FORMAT_ASTC_5x4x4_UNORM_BLOCK, 5, 4, 4, 16));
		//Uploaders[TEXF_ASTC_5x5x4].reset(new TextureUploader_3DBlock(DXGI_FORMAT_ASTC_5x5x4_UNORM_BLOCK, 5, 5, 4, 16));
		//Uploaders[TEXF_ASTC_5x5x5].reset(new TextureUploader_3DBlock(DXGI_FORMAT_ASTC_5x5x5_UNORM_BLOCK, 5, 5, 5, 16));
		//Uploaders[TEXF_ASTC_6x5x5].reset(new TextureUploader_3DBlock(DXGI_FORMAT_ASTC_6x5x5_UNORM_BLOCK, 6, 5, 5, 16));
		//Uploaders[TEXF_ASTC_6x6x5].reset(new TextureUploader_3DBlock(DXGI_FORMAT_ASTC_6x6x5_UNORM_BLOCK, 6, 6, 5, 16));
		//Uploaders[TEXF_ASTC_6x6x6].reset(new TextureUploader_3DBlock(DXGI_FORMAT_ASTC_6x6x6_UNORM_BLOCK, 6, 6, 6, 16));

		// PVRTC.
		// Requires VK_IMG_format_pvrtc
		//Uploaders[DXGI_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG].reset(new TextureUploader_2DBlock(DXGI_FORMAT_ASTC_12x12_UNORM_BLOCK, 8, 4, 8));
		//Uploaders[DXGI_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG].reset(new TextureUploader_4x4Block(DXGI_FORMAT_ASTC_12x12_UNORM_BLOCK, 8));
		//Uploaders[DXGI_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG].reset(new TextureUploader_2DBlock(DXGI_FORMAT_ASTC_12x12_UNORM_BLOCK, 8, 4, 8));
		//Uploaders[DXGI_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG].reset(new TextureUploader_4x4Block(DXGI_FORMAT_ASTC_12x12_UNORM_BLOCK, 8));

		// RGBA (Integral).
		Uploaders[TEXF_R8_UI].reset(new TextureUploader_Simple(DXGI_FORMAT_R8_UINT, 1));
		Uploaders[TEXF_R8_I].reset(new TextureUploader_Simple(DXGI_FORMAT_R8_SINT, 1));
		Uploaders[TEXF_R16_UI].reset(new TextureUploader_Simple(DXGI_FORMAT_R16_UINT, 2));
		Uploaders[TEXF_R16_I].reset(new TextureUploader_Simple(DXGI_FORMAT_R16_SINT, 2));
		Uploaders[TEXF_R32_UI].reset(new TextureUploader_Simple(DXGI_FORMAT_R32_UINT, 4));
		Uploaders[TEXF_R32_I].reset(new TextureUploader_Simple(DXGI_FORMAT_R32_SINT, 4));
		Uploaders[TEXF_RG8_UI].reset(new TextureUploader_Simple(DXGI_FORMAT_R8G8_UINT, 2));
		Uploaders[TEXF_RG8_I].reset(new TextureUploader_Simple(DXGI_FORMAT_R8G8_SINT, 2));
		Uploaders[TEXF_RG16_UI].reset(new TextureUploader_Simple(DXGI_FORMAT_R16G16_UINT, 4));
		Uploaders[TEXF_RG16_I].reset(new TextureUploader_Simple(DXGI_FORMAT_R16G16_SINT, 4));
		Uploaders[TEXF_RG32_UI].reset(new TextureUploader_Simple(DXGI_FORMAT_R32G32_UINT, 8));
		Uploaders[TEXF_RG32_I].reset(new TextureUploader_Simple(DXGI_FORMAT_R32G32_SINT, 8));
		//Uploaders[TEXF_RGB8_UI].reset(new TextureUploader_Simple(DXGI_FORMAT_R8G8B8_UINT, 3));
		//Uploaders[TEXF_RGB8_I].reset(new TextureUploader_Simple(DXGI_FORMAT_R8G8B8_SINT, 3));
		//Uploaders[TEXF_RGB16_UI].reset(new TextureUploader_Simple(DXGI_FORMAT_R16G16B16_UINT, 6));
		//Uploaders[TEXF_RGB16_I].reset(new TextureUploader_Simple(DXGI_FORMAT_R16G16B16_SINT, 6));
		Uploaders[TEXF_RGB32_UI].reset(new TextureUploader_Simple(DXGI_FORMAT_R32G32B32_UINT, 12));
		Uploaders[TEXF_RGB32_I].reset(new TextureUploader_Simple(DXGI_FORMAT_R32G32B32_SINT, 12));
		Uploaders[TEXF_RGBA8_UI].reset(new TextureUploader_Simple(DXGI_FORMAT_R8G8B8A8_UINT, 4));
		Uploaders[TEXF_RGBA8_I].reset(new TextureUploader_Simple(DXGI_FORMAT_R8G8B8A8_SINT, 4));
		Uploaders[TEXF_RGBA16_UI].reset(new TextureUploader_Simple(DXGI_FORMAT_R16G16B16A16_UINT, 8));
		Uploaders[TEXF_RGBA16_I].reset(new TextureUploader_Simple(DXGI_FORMAT_R16G16B16A16_SINT, 8));
		Uploaders[TEXF_RGBA32_UI].reset(new TextureUploader_Simple(DXGI_FORMAT_R32G32B32A32_UINT, 16));
		Uploaders[TEXF_RGBA32_I].reset(new TextureUploader_Simple(DXGI_FORMAT_R32G32B32A32_SINT, 16));

		// Special.
		//Uploaders[TEXF_ARGB8].reset(new TextureUploader_ARGB8());
		//Uploaders[TEXF_ABGR8].reset(new TextureUploader_ABGR8());
		Uploaders[TEXF_RGB10A2].reset(new TextureUploader_RGB10A2());
		Uploaders[TEXF_RGB10A2_UI].reset(new TextureUploader_RGB10A2_UI());
		Uploaders[TEXF_RGB10A2_LM].reset(new TextureUploader_RGB10A2_LM());
		//Uploaders[TEXF_RGB9E5].reset(new TextureUploader_RGB9E5());
		//Uploaders[TEXF_P8_RGB9E5].reset(new TextureUploader_P8_RGB9E5());
		//Uploaders[TEXF_R1].reset(new TextureUploader_R1());
		//Uploaders[TEXF_RGB10A2_S].reset(new TextureUploader_RGB10A2_S());
		//Uploaders[TEXF_RGB10A2_I].reset(new TextureUploader_RGB10A2_I());
		//Uploaders[TEXF_R11G11B10_F].reset(new TextureUploader_R11G11B10_F());

		// Normalized BGR.
		//Uploaders[TEXF_B5G6R5].reset(new TextureUploader_B5G6R5());
		//Uploaders[TEXF_BGR8].reset(new TextureUploader_BGR8());

		// Double precission floating point RGBA.
		//Uploaders[TEXF_R64_F].reset(new TextureUploader_R64_F());
		//Uploaders[TEXF_RG64_F].reset(new TextureUploader_RG64_F());
		//Uploaders[TEXF_RGB64_F].reset(new TextureUploader_RGB64_F());
		//Uploaders[TEXF_RGBA64_F].reset(new TextureUploader_RGBA64_F());
#endif
	}

	auto it = Uploaders.find(format);
	if (it != Uploaders.end())
		return it->second.get();
	else
		return nullptr;
}

/////////////////////////////////////////////////////////////////////////////

int TextureUploader_P8::GetUploadSize(int x, int y, int w, int h)
{
	return w * h * 4;
}

void TextureUploader_P8::UploadRect(void* d, FMipmapBase* mip, int x, int y, int w, int h, FColor* palette, bool masked)
{
	int pitch = mip->USize;
	BYTE* src = mip->DataPtr + x + y * pitch;
	FColor* Ptr = (FColor*)d;
	if (masked)
	{
		FColor translucent(0, 0, 0, 0);
		for (int i = 0; i < h; i++)
		{
			for (int j = 0; j < w; j++)
			{
				int idx = src[j];
				*Ptr++ = (idx != 0) ? palette[idx] : translucent;
			}
			src += pitch;
		}
	}
	else
	{
		for (int i = 0; i < h; i++)
		{
			for (int j = 0; j < w; j++)
			{
				int idx = src[j];
				*Ptr++ = palette[idx];
			}
			src += pitch;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////

int TextureUploader_RGB8::GetUploadSize(int x, int y, int w, int h)
{
	return w * h * 4;
}

void TextureUploader_RGB8::UploadRect(void* dst, FMipmapBase* mip, int x, int y, int w, int h, FColor* palette, bool masked)
{
	int pitch = mip->USize * 3;
	uint8_t* src = ((uint8_t*)mip->DataPtr) + x + y * pitch;
	auto Ptr = (FColor*)dst;
	for (int i = 0; i < h; i++)
	{
		int k = 0;
		for (int j = 0; j < w; j++)
		{
			Ptr->R = src[k++];
			Ptr->G = src[k++];
			Ptr->B = src[k++];
			Ptr->A = 255;
			Ptr++;
		}
		src += pitch;
	}
}

/////////////////////////////////////////////////////////////////////////////

int TextureUploader_BGRA8_LM::GetUploadSize(int x, int y, int w, int h)
{
	return w * h * 4;
}

void TextureUploader_BGRA8_LM::UploadRect(void* dst, FMipmapBase* mip, int x, int y, int w, int h, FColor* palette, bool masked)
{
#ifdef USE_SSE2
	int pitch = mip->USize;
	FColor* src = ((FColor*)mip->DataPtr) + x + y * pitch;
	auto Ptr = (FColor*)dst;
	if (w % 4 == 0)
	{
		for (int i = 0; i < h; i++)
		{
			for (int j = 0; j < w; j += 4)
			{
				__m128i p = _mm_loadu_si128((const __m128i*)(src + j));
				__m128i p_hi = _mm_unpackhi_epi8(p, _mm_setzero_si128());
				__m128i p_lo = _mm_unpacklo_epi8(p, _mm_setzero_si128());
				p_hi = _mm_shufflehi_epi16(p_hi, _MM_SHUFFLE(3, 0, 1, 2));
				p_hi = _mm_shufflelo_epi16(p_hi, _MM_SHUFFLE(3, 0, 1, 2));
				p_hi = _mm_slli_epi16(p_hi, 1);
				p_lo = _mm_shufflehi_epi16(p_lo, _MM_SHUFFLE(3, 0, 1, 2));
				p_lo = _mm_shufflelo_epi16(p_lo, _MM_SHUFFLE(3, 0, 1, 2));
				p_lo = _mm_slli_epi16(p_lo, 1);
				p = _mm_packus_epi16(p_lo, p_hi);
				_mm_storeu_si128((__m128i*)(Ptr + j), p);
			}
			Ptr += w;
			src += pitch;
		}
	}
	else
	{
		for (int i = 0; i < h; i++)
		{
			for (int j = 0; j < w; j++)
			{
				FColor Src = src[j];
				Ptr->R = Src.B << 1;
				Ptr->G = Src.G << 1;
				Ptr->B = Src.R << 1;
				Ptr->A = Src.A << 1;
				Ptr++;
			}
			src += pitch;
		}
	}
#else
	int pitch = mip->USize;
	FColor* src = ((FColor*)mip->DataPtr) + x + y * pitch;
	auto Ptr = (FColor*)dst;
	for (int i = 0; i < h; i++)
	{
		for (int j = 0; j < w; j++)
		{
			FColor Src = src[j];
			Ptr->R = Src.B << 1;
			Ptr->G = Src.G << 1;
			Ptr->B = Src.R << 1;
			Ptr->A = Src.A << 1;
			Ptr++;
		}
		src += pitch;
	}
#endif
}

/////////////////////////////////////////////////////////////////////////////

int TextureUploader_RGB10A2::GetUploadSize(int x, int y, int w, int h)
{
	return w * h * 8;
}

void TextureUploader_RGB10A2::UploadRect(void* dst, FMipmapBase* mip, int x, int y, int w, int h, FColor* palette, bool masked)
{
	int pitch = mip->USize;
	uint32_t* src = ((uint32_t*)mip->DataPtr) + x + y * pitch;
	uint16_t* Ptr = (uint16_t*)dst;
	for (int i = 0; i < h; i++)
	{
		for (int j = 0; j < w; j++)
		{
			uint32_t c = *Ptr;
			uint32_t r = (c >> 22) & 0x3ff;
			uint32_t g = (c >> 12) & 0x3ff;
			uint32_t b = (c >> 2) & 0x3ff;
			uint32_t a = c & 0x3;

			r = r * 0xffff / 0x3ff;
			g = g * 0xffff / 0x3ff;
			b = b * 0xffff / 0x3ff;
			a = a * 0xffff / 0x3;

			*(Ptr++) = r;
			*(Ptr++) = g;
			*(Ptr++) = b;
			*(Ptr++) = a;
		}
		src += pitch;
	}
}

/////////////////////////////////////////////////////////////////////////////

int TextureUploader_RGB10A2_UI::GetUploadSize(int x, int y, int w, int h)
{
	return w * h * 8;
}

void TextureUploader_RGB10A2_UI::UploadRect(void* dst, FMipmapBase* mip, int x, int y, int w, int h, FColor* palette, bool masked)
{
	int pitch = mip->USize;
	uint32_t* src = ((uint32_t*)mip->DataPtr) + x + y * pitch;
	uint16_t* Ptr = (uint16_t*)dst;
	for (int i = 0; i < h; i++)
	{
		for (int j = 0; j < w; j++)
		{
			uint32_t c = *Ptr;
			uint32_t r = (c >> 22) & 0x3ff;
			uint32_t g = (c >> 12) & 0x3ff;
			uint32_t b = (c >> 2) & 0x3ff;
			uint32_t a = c & 0x3;

			*(Ptr++) = r;
			*(Ptr++) = g;
			*(Ptr++) = b;
			*(Ptr++) = a;
		}
		src += pitch;
	}
}

/////////////////////////////////////////////////////////////////////////////

int TextureUploader_RGB10A2_LM::GetUploadSize(int x, int y, int w, int h)
{
	return w * h * 8;
}

void TextureUploader_RGB10A2_LM::UploadRect(void* dst, FMipmapBase* mip, int x, int y, int w, int h, FColor* palette, bool masked)
{
	int pitch = mip->USize;
	uint32_t* src = ((uint32_t*)mip->DataPtr) + x + y * pitch;
	uint16_t* Ptr = (uint16_t*)dst;
	for (int i = 0; i < h; i++)
	{
		for (int j = 0; j < w; j++)
		{
			uint32_t c = *Ptr;
			uint32_t r = (c >> 22) & 0x3ff;
			uint32_t g = (c >> 12) & 0x3ff;
			uint32_t b = (c >> 2) & 0x3ff;
			uint32_t a = c & 0x3;

			r = (r << 1) * 0xffff / 0xff;
			g = (g << 1) * 0xffff / 0xff;
			b = (b << 1) * 0xffff / 0xff;
			a = (a << 1) * 0xffff / 0x3;

			*(Ptr++) = r;
			*(Ptr++) = g;
			*(Ptr++) = b;
			*(Ptr++) = a;
		}
		src += pitch;
	}
}

/////////////////////////////////////////////////////////////////////////////

int TextureUploader_Simple::GetUploadSize(int x, int y, int w, int h)
{
	return w * h * BytesPerPixel;
}

void TextureUploader_Simple::UploadRect(void* d, FMipmapBase* mip, int x, int y, int w, int h, FColor* palette, bool masked)
{
	int pitch = mip->USize * BytesPerPixel;
	int size = w * BytesPerPixel;
	BYTE* src = mip->DataPtr + x * BytesPerPixel + y * pitch;
	BYTE* dst = (BYTE*)d;
	for (int i = 0; i < h; i++)
	{
		memcpy(dst, src, size);
		dst += size;
		src += pitch;
	}
}

/////////////////////////////////////////////////////////////////////////////

int TextureUploader_4x4Block::GetUploadSize(int x, int y, int w, int h)
{
	int x0 = x / 4;
	int y0 = y / 4;
	int x1 = (x + w + 3) / 4;
	int y1 = (y + h + 3) / 4;
	return (x1 - x0) * (y1 - y0) * BytesPerBlock;
}

void TextureUploader_4x4Block::UploadRect(void* d, FMipmapBase* mip, int x, int y, int w, int h, FColor* palette, bool masked)
{
	int x0 = x / 4;
	int y0 = y / 4;
	int x1 = (x + w + 3) / 4;
	int y1 = (y + h + 3) / 4;

	int pitch = (mip->USize + 3) / 4 * BytesPerBlock;
	int size = (x1 - x0) * BytesPerBlock;
	BYTE* src = mip->DataPtr + x0 * BytesPerBlock + y0 * pitch;
	BYTE* dst = (BYTE*)d;
	for (int i = y0; i < y1; i++)
	{
		memcpy(dst, src, size);
		dst += size;
		src += pitch;
	}
}


/////////////////////////////////////////////////////////////////////////////

int TextureUploader_2DBlock::GetUploadSize(int x, int y, int w, int h)
{
	int x0 = x / BlockX;
	int y0 = y / BlockY;
	int x1 = (x + w + BlockX - 1) / BlockX;
	int y1 = (y + h + BlockY - 1) / BlockY;
	return (x1 - x0) * (y1 - y0) * BytesPerBlock;
}

void TextureUploader_2DBlock::UploadRect(void* d, FMipmapBase* mip, int x, int y, int w, int h, FColor* palette, bool masked)
{
	int x0 = x / BlockX;
	int y0 = y / BlockY;
	int x1 = (x + w + BlockX - 1) / BlockX;
	int y1 = (y + h + BlockY - 1) / BlockY;

	int pitch = (mip->USize + BlockX - 1) / BlockX * BytesPerBlock;
	int size = (x1 - x0) * BytesPerBlock;
	BYTE* src = mip->DataPtr + x0 * BytesPerBlock + y0 * pitch;
	BYTE* dst = (BYTE*)d;
	for (int i = y0; i < y1; i++)
	{
		memcpy(dst, src, size);
		dst += size;
		src += pitch;
	}
}
