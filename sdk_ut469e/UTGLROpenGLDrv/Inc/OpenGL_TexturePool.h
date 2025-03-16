/*=============================================================================
	OpenGL_TexturePool.h
	
	High level Texture descriptor pool.
	Used to abstract various logic away from the renderer.

	Rules:
	- Pool index is invariant

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

#define GL_TEXTURE_2D_UTEXTURE (UOpenGLRenderDevice::TexturePool.UseArrayTextures ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D)

namespace FGL
{

enum ETexturePoolID
{
	TEXPOOL_ID_InvalidTexture,
	TEXPOOL_ID_DervMap,
	TEXPOOL_ID_NoDetail,
	TEXPOOL_ID_NoTexture,
	TEXPOOL_ID_LightMapParams,

	TEXPOOL_ID_START,
};


//
// Remap a Unreal Texture into a GL Texture
//
struct FTextureRemap
{
	INT   PoolIndex              {INDEX_NONE};
	INT   Layer                  {INDEX_NONE};
	BYTE  UFormat                {0};
#if UNREAL_TOURNAMENT_OLDUNREAL
	INT   RealtimeChangeCount    {0};
#endif

	void CheckRealtimeChangeCount( FTextureInfo& Info);
};


//
// Texture pooling system
//
class FTexturePool
{
public:
	class FTexRemapList : protected TMapExt<FCacheID,FTextureRemap>
	{
	public:
		using TMapExt::TMapExt;
		using TMap::Set;
		using TMap::Find;
		using TMap::Empty;
		using TMap::Num;
		using TMapExt::SetNoFind;
		using TMap::Dump;

		void EnsureReserved( INT ExtraPairs);
		FTextureRemap& operator()(INT Index);
		const FTextureRemap& operator()(INT Index) const;
	};

	class FPooledTexture : public FOpenGLTexture
	{
	public:
		DWORD              LayerBits[8]       {0};
		INT                UniformQueueIndex  {INDEX_NONE};
		TNumericTag<DWORD> UniformLockTag;
		DWORD              TextureKey         {0};

		static constexpr DWORD MAX_LAYERS = sizeof(LayerBits) * 8;

		FPooledTexture();
		FPooledTexture( GLenum Target);
	};

	class FTextureUniformEntry
	{
	public:
		INT PoolIndex;
		INT UniformIndex;
	};

	GLenum  DefaultLinearMinFilter;
	GLfloat DefaultMaxAnisotropyValue;
	UBOOL   UseArrayTextures;

	TArrayExt<FPooledTexture> Textures;
	FTexRemapList Remaps;

	TRoundQueue<FTextureUniformEntry> UniformQueue;
	TNumericTag<DWORD>                UniformLockTag;

	FTexturePool();

	void Lock();
	void Unlock();
	void Flush();
	void SetTrilinearFiltering( bool NewUseTrilinear);
	void SetAnisotropicFiltering( GLfloat NewMaxAnisotropy);

	FTextureRemap& GetRemap( const FTextureInfo& Info, DWORD PolyFlags=0);
	FPooledTexture& CreateTexture( GLenum Target, INT& Index);
	void UnlinkRemap( FTextureRemap& Remap);
	void DeleteTexture( INT Index);

	void InitUniformQueue( INT QueueSize);
	bool SetupUniformQueueItem( INT PoolIndex);

	void PreallocateArrayTextures();

private:
	FFreeIndexList FreeList;
};

}


/*-----------------------------------------------------------------------------
	FTextureRemap.
-----------------------------------------------------------------------------*/

inline void FGL::FTextureRemap::CheckRealtimeChangeCount( FTextureInfo& Info)
{
#if UNREAL_TOURNAMENT_OLDUNREAL
	if ( Info.Texture )
	{
		INT NewRealtimeChangeCount = Info.Texture->RealtimeChangeCount;
		if ( RealtimeChangeCount != NewRealtimeChangeCount )
			RealtimeChangeCount = NewRealtimeChangeCount;
		else
			Info.bRealtimeChanged = 0;
	}
#endif
}


/*-----------------------------------------------------------------------------
	FTexturePool.
-----------------------------------------------------------------------------*/

inline FGL::FTextureRemap& FGL::FTexturePool::GetRemap( const FTextureInfo& Info, DWORD PolyFlags)
{
	QWORD CacheID = Info.CacheID;

	FTextureRemap* Remap = Remaps.Find(CacheID);
	if ( Remap == nullptr )
	{
		Remap = &Remaps.SetNoFind(CacheID, FTextureRemap());
		Remap->UFormat = Info.Format;
	}
	return *Remap;
}


/*-----------------------------------------------------------------------------
	FTexRemapList.
-----------------------------------------------------------------------------*/

//
// Ensures that at least 'ExtraPairs' new pairs can be created without causing
// pair array reallocation.
//
inline void FGL::FTexturePool::FTexRemapList::EnsureReserved( INT ExtraPairs)
{
	INT TargetMax = Pairs.Num() + ExtraPairs;
	if ( TargetMax > Pairs.Max() )
	{
		INT AddCount = TargetMax - Pairs.Num();
		Pairs.Add(AddCount);
		Pairs.AddNoCheck(-AddCount);
	}
}

inline FGL::FTextureRemap& FGL::FTexturePool::FTexRemapList::operator()( INT Index)
{
	return Pairs(Index).Value;
}

inline const FGL::FTextureRemap& FGL::FTexturePool::FTexRemapList::operator()( INT Index) const
{
	return Pairs(Index).Value;
}


/*-----------------------------------------------------------------------------
	FPooledTexture.
-----------------------------------------------------------------------------*/

inline FGL::FTexturePool::FPooledTexture::FPooledTexture()
	: FOpenGLTexture()
{
}

inline FGL::FTexturePool::FPooledTexture::FPooledTexture( GLenum InTarget)
	: FOpenGLTexture()
{
	Create(InTarget);
}
