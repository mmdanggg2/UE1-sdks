/*=============================================================================
	OpenGL_CacheID.h: Cache ID handling.

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

//Fixed texture cache ids
#define TEX_CACHE_ID_UNUSED     0xFFFFFFFFFFFFFFFFULL
#define TEX_CACHE_ID_DERV_TEX   0xFFFFFFFF00000040ULL
#define TEX_CACHE_ID_WHITE      0xFFFFFFFF00000020ULL

namespace FGL
{
	//
	//Texture cache id flags
	//
	enum ETexCacheIDFlags
	{
		TEX_CACHE_ID_FLAG_LOD    = 0b011,
		TEX_CACHE_ID_FLAG_MASKED = 0b100, // Higor: changed so it won't collide with LODSet

		TEX_CACHE_ID_FLAG_ALL    = 0b111,
	};


	//
	// Cache ID wrapper to ensure proper TMap hashing behaviour
	//
	struct FCacheID : public TDataWrapper<QWORD>
	{
		FCacheID( QWORD InValue ) : TDataWrapper(InValue) {}

		bool IsUTexture() const;
		bool IsMaskedUTexture() const;
	};


	//
	// Adjust a FTextureInfo's CacheID to ensure proper masked storage is used
	//
	inline void FixCacheID( FTextureInfo& Info, UBOOL UseMaskedStorage)
	{
		QWORD CacheID = Info.CacheID;
		if ( ((BYTE)CacheID >= CID_RenderTexture) && ((BYTE)CacheID < CID_RenderTexture+MAX_TEXTURE_LOD) )
		{
			// Masked in formats other than P8 simply hide Alpha < 0.5
			// Additionally, if Color[0] is already (0,0,0,0) then there's no need for double storage.
			if ( UseMaskedStorage && (Info.Format == TEXF_P8) && (Info.Palette[0] != FColor(0,0,0,0)) )
				CacheID |= TEX_CACHE_ID_FLAG_MASKED;

			Info.CacheID = CacheID;
		}
	}


	namespace Surface
	{
		//
		//	Templated Cache ID extraction from a surface.
		//	Used for draw command setup path.
		//
		inline QWORD CacheID( const FTextureInfo* Info)
		{
			return Info ? Info->CacheID : 0;
		}
	};

};


/*-----------------------------------------------------------------------------
	FCacheID.
-----------------------------------------------------------------------------*/

static inline DWORD GetTypeHash( const FGL::FCacheID& A)
{
	QWORD Value = A.GetValue();
	return (DWORD)Value^((DWORD)(Value>>16))^((DWORD)(Value>>32));
}

inline bool FGL::FCacheID::IsUTexture() const
{
	static_assert((CID_RenderTexture & TEX_CACHE_ID_FLAG_LOD) == 0, "FIXME");

	BYTE CacheType = (BYTE)this->Value & ~((BYTE)TEX_CACHE_ID_FLAG_ALL); // Remove LOD and Masked
	return CacheType == CID_RenderTexture;
}

inline bool FGL::FCacheID::IsMaskedUTexture() const
{
	static_assert((CID_RenderTexture & TEX_CACHE_ID_FLAG_LOD) == 0, "FIXME");

	BYTE CacheType = (BYTE)this->Value & ~((BYTE)TEX_CACHE_ID_FLAG_LOD); // Remove LOD
	return CacheType == (CID_RenderTexture|TEX_CACHE_ID_FLAG_MASKED);
}
