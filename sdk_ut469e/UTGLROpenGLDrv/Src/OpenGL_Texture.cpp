/*=============================================================================
	OpenGL_Texture.cpp: Texture handling code for OpenGLDrv.

	Revision history:
	* Created by Fernando Velazquez
	* Texture Pooling system by Fernando Velazquez
=============================================================================*/

#include "OpenGLDrv.h"
#include "UTGLROpenGL.h"

#include "OpenGL_TextureFormat.h"

#define GL_COLOR_INDEX 0x1900


/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

FGL::FTexturePool UOpenGLRenderDevice::TexturePool;


/*-----------------------------------------------------------------------------
	Texture pool.
-----------------------------------------------------------------------------*/

//
// Construct default Texture Pool
//
FGL::FTexturePool::FTexturePool()
	: DefaultLinearMinFilter(FGL::GetLinearMinificationFilter(false))
	, DefaultMaxAnisotropyValue(1)
	, UseArrayTextures(0)
{
}

static void ComposeInvalidTexture( FOpenGLTexture& Texture)
{
	guard(ComposeInvalidTexture);

	constexpr GLint  Size   = 64;
	constexpr GLuint XTexel = (255 << 0) | (0 << 8) | (255 << 16) | (255 << 24);
	const FTextureFormatInfo& Fmt = FTextureFormatInfo::Get(TEXF_RGBA8_);

	Texture.Create(GL_TEXTURE_2D_UTEXTURE);
	FOpenGLBase::ActiveInstance->TextureUnits.Bind(Texture);

	TArray<GLuint> Texels;
	Texels.AddZeroed( Size * Size );

	// Compose a purple cross on top of a transparent background
	const INT Border1 = Max(Size / 8,  1);
	const INT Border2 = Size - Border1;
	const INT Expand  = Size / 32;
	for ( INT i=Border1; i<Border2; i++)
	{
		INT ScanlineOffset = i * Size;
		for ( INT j=Max(Border1,i-Expand); j<Min(Border2,i+Expand); j++)
			Texels(ScanlineOffset + j) = XTexel;

		INT k = Size-i;
		for ( INT j=Max(Border1,k-Expand); j<Min(Border2,k+Expand); j++)
			Texels(ScanlineOffset + j) = XTexel;
	}

	FOpenGLBase::SetTextureStorage(Texture, Fmt, Size, Size, 1, 1);
	FOpenGLBase::SetTextureData(Texture, Fmt, 0, Size, Size, 0, &Texels(0), Texels.Num()*sizeof(XTexel));
	FOpenGLBase::SetTextureFilters(Texture, true);

	unguard;
}

static void ComposeDervMap( FOpenGLTexture& Texture)
{
	guard(ComposeDervMap);

	// Reference:
	// http://www.humus.name/index.php?page=3D&ID=61
	// Simplification:
	// - A texture sampled perfectly pixel to pixel should yield a derv of 1/255 (by looking up smallest mip)
	// - If texture is zoomed in 2x it should yield a derv of 2/255 and so on
	// The idea is to increase contrast of alpha channel as texture zooms in with the purpose of
	// ensuring that the (0,1) transition occurs in a pixel's worth of distance.
	//
	// The Derv map is a texture of 64x64 pixels that contains its size as value.
	// In order to make it work on textures with different sizes it is necessary to adjust the
	// scale of the Derv map before it's sampled in the fragment shader (Derv cannot go below 1/255)
	// - For every power of 2 increase in texture size, Derv UV must be higher by the same factor.
	//
	// NOTE: This currently cannot detect mipmapping on source!
	constexpr GLuint Size   = 64;
	constexpr GLuint Levels = TBits<Size>::BitCount; // 7
	const FTextureFormatInfo& Fmt = FTextureFormatInfo::Get(TEXF_R8);

	Texture.Create(GL_TEXTURE_2D);
	FOpenGLBase::ActiveInstance->TextureUnits.Bind(Texture);

	GLuint Level = 0;
	GLuint CurrentSize = Size;

	FOpenGLBase::SetTextureStorage(Texture, Fmt, Size, Size, 1, Levels);

	TArray<BYTE> ImageData;
	ImageData.SetSize(Size * Size);
	for ( Level=0; Level<Levels; Level++)
	{
		appMemset( &ImageData(0), CurrentSize, CurrentSize*CurrentSize);
		FOpenGLBase::SetTextureData(Texture, Fmt, Level, CurrentSize, CurrentSize, 0, &ImageData(0), ImageData.Num());
		CurrentSize >>= 1;
	}
	FOpenGLBase::SetTextureFilters(Texture, false);

	unguard;
}

static void ComposePlainColor( FOpenGLTexture& Texture, FColor Color)
{
	guard(ComposeNoDetail);

	constexpr GLint Size = 2;
	const FTextureFormatInfo& Fmt = FTextureFormatInfo::Get(TEXF_RGBA8_);

	DWORD Data[Size*Size];
	for ( INT i=0; i<Size*Size; i++)
		Data[i] = GET_COLOR_DWORD(Color);

	Texture.Create(GL_TEXTURE_2D_UTEXTURE);
	FOpenGLBase::ActiveInstance->TextureUnits.Bind(Texture);
	FOpenGLBase::SetTextureStorage(Texture, Fmt, Size, Size, 1, 1);
	FOpenGLBase::SetTextureData(Texture, Fmt, 0, Size, Size, 0, Data, sizeof(Data));
	FOpenGLBase::SetTextureFilters(Texture, false);

	unguard;
}

static void CreateBufferTexture( FOpenGLTexture& Texture)
{
	guard(CreateBufferTexture);

	if ( !FOpenGLBase::SupportsTBO )
	{
		ComposePlainColor(Texture,FColor(0,0,0,0));
		return;
	}

	// Bind buffer texture but do not specify storage
	Texture.Create(GL_TEXTURE_BUFFER);
	FOpenGLBase::ActiveInstance->TextureUnits.Bind(Texture);

	unguard;
}

//
// Pre-render operations
//
void FGL::FTexturePool::Lock()
{
	guard(FTexturePool::Lock);
	checkSlow(FOpenGLBase::ActiveInstance);

	bool PostFlush = (Textures.Num() == 0);

	if ( PostFlush )
		Textures.Reserve(64);

	if ( Textures.Num() == 0 )
	{
		check(FOpenGLBase::ActiveInstance);

		Textures.AddZeroed(TEXPOOL_ID_START);
		FOpenGLBase::ActiveInstance->TextureUnits.SetActiveNoCheck(0);
		
		ComposeInvalidTexture(Textures(TEXPOOL_ID_InvalidTexture));
		ComposeDervMap(Textures(TEXPOOL_ID_DervMap));
		ComposePlainColor(Textures(TEXPOOL_ID_NoDetail), FColor(127,127,127,255) );
		ComposePlainColor(Textures(TEXPOOL_ID_NoTexture), FColor(255,255,255,255) );
		CreateBufferTexture(Textures(TEXPOOL_ID_LightMapParams));
	}

	if ( PostFlush && UseArrayTextures )
		PreallocateArrayTextures();

	unguard;
}

//
// Post-render operations
//
void FGL::FTexturePool::Unlock()
{
	guard(FTexturePool::Unlock);
	UniformLockTag.Increment();
	unguard;
}

//
// Empty the pool
//
void FGL::FTexturePool::Flush()
{
	guard(FTexturePool::Flush);

	// Unbind all Textures in all contexts
	for ( INT i=0; i<FOpenGLBase::Instances.Num(); i++)
	{
		FOpenGLBase* GL = FOpenGLBase::Instances(i);
		if ( GL )
		{
			GL->TextureUnits.UnbindAll();
			GL->TextureUnits.Reset(); // Ugly
		}
	}

	TArray<GLuint> TextureNames;
	TextureNames.Reserve(Textures.Num());
	for ( INT i=0; i<Textures.Num(); i++)
		if ( Textures(i).Texture )
			new(TextureNames) GLuint(Textures(i).Texture);

	if ( TextureNames.Num() )
		FOpenGLBase::glDeleteTextures(TextureNames.Num(), &TextureNames(0));

	Textures.Empty();
	Remaps.Empty();
	FreeList.Empty();
	InitUniformQueue(UniformQueue.Size());

	unguard;
}

//
// Change default texture minification filter
//
void FGL::FTexturePool::SetTrilinearFiltering( bool NewUseTrilinear)
{
	guard(FTexturePool::SetTrilinearFiltering);
	GLenum NewLinearMinFilter = FGL::GetLinearMinificationFilter(NewUseTrilinear);
	if ( NewLinearMinFilter != DefaultLinearMinFilter )
	{
		DefaultLinearMinFilter = NewLinearMinFilter; // Update early

		check(FOpenGLBase::ActiveInstance);
		FOpenGLBase* GL = FOpenGLBase::ActiveInstance;
		GL->TextureUnits.SetActive(TMU_Position);
		for ( INT i=TEXPOOL_ID_START ; i<Textures.Num(); i++)
		{
			FOpenGLTexture& Texture = Textures(i);
			if ( Texture.Texture && Texture.HasMipMaps() && !Texture.MagNearest && Texture.Allocated )
			{
				GL->TextureUnits.Bind(Texture);
				FOpenGLBase::glTexParameteri(Texture.Target, GL_TEXTURE_MIN_FILTER, NewLinearMinFilter);
			}
		}
		GL->TextureUnits.Unbind();
		GL->TextureUnits.SetActiveNoCheck(0);
	}
	unguard;
}

//
// Change default texture anisotropic filter
//
void FGL::FTexturePool::SetAnisotropicFiltering(GLfloat NewMaxAnisotropy)
{
	guard(FTexturePool::SetAnisotropicFiltering);

	if ( !FOpenGLBase::SupportsAnisotropy )
		return;

	GLfloat NewMaxAnisotropyValue = Clamp<GLfloat>( NewMaxAnisotropy, 1, 16);
	if ( NewMaxAnisotropyValue != DefaultMaxAnisotropyValue )
	{
		DefaultMaxAnisotropyValue = NewMaxAnisotropyValue; // Update early
		check(FOpenGLBase::ActiveInstance);
		FOpenGLBase* GL = FOpenGLBase::ActiveInstance;
		GL->TextureUnits.SetActive(TMU_Position);
		for ( INT i=TEXPOOL_ID_START ; i<Textures.Num(); i++)
		{
			FOpenGLTexture& Texture = Textures(i);
			if ( Texture.Texture && Texture.HasMipMaps() && Texture.Allocated && !Texture.MagNearest )
			{
				GL->TextureUnits.Bind(Texture);
				FOpenGLBase::glTexParameteri(Texture.Target, GL_TEXTURE_MAX_ANISOTROPY_EXT, NewMaxAnisotropyValue);
			}
		}
		GL->TextureUnits.Unbind();
		GL->TextureUnits.SetActiveNoCheck(0);
	}

	unguard;
}


/*-----------------------------------------------------------------------------
	Texture remap manipulation.
-----------------------------------------------------------------------------*/

FGL::FTexturePool::FPooledTexture& FGL::FTexturePool::CreateTexture( GLenum Target, INT& Index)
{
	guard(CreateTexture);

	if ( !FreeList.GetAndPop(Index) )
	{
		PTRINT OldDataStart = (PTRINT)Textures.GetData();
		PTRINT OldDataEnd   = OldDataStart + Textures.Max() * sizeof(FPooledTexture);
		Index = Textures.Add();

		// Ugly hack to preserve pointers in Texture Mapping unit trackers
		PTRINT OffsetPointers = (PTRINT)Textures.GetData() - OldDataStart;
		if ( OffsetPointers != 0 )
		{
			for ( INT i=0; i<FOpenGLBase::Instances.Num(); i++)
			{
				FOpenGLBase* GL = FOpenGLBase::Instances(i);
				if ( GL )
				{
					for ( INT u=0; u<GL_MAX_TEXTURE_UNITS; u++)
					{
						if	(	(PTRINT)GL->TextureUnits.Bound[u] >= OldDataStart
							&&	(PTRINT)GL->TextureUnits.Bound[u] < OldDataEnd )
							*(PTRINT*)&GL->TextureUnits.Bound[u] += OffsetPointers;
					}
				}
			}
		}
	}

#if GL_DEV
	Textures(Index).Texture = 0; // Prevent assertion trip
#endif
	Textures(Index) = FGL::FTexturePool::FPooledTexture(Target);
	return Textures(Index);

	unguard;
}

void FGL::FTexturePool::DeleteTexture( INT Index)
{
	guard(DeleteTexture);

	if ( Textures.IsValidIndex(Index) && Textures(Index).Texture )
	{
		FOpenGLBase::glDeleteTextures( 1, &Textures(Index).Texture);
		if ( Textures(Index).UniformQueueIndex >= 0 )
		{
			UniformQueue(Textures(Index).UniformQueueIndex).PoolIndex = INDEX_NONE;
			Textures(Index).UniformQueueIndex = INDEX_NONE;
		}
		appMemzero( &Textures(Index), sizeof(FGL::FTexturePool::FPooledTexture));
		if ( Index+1 == Textures.Num() )
			Textures.AddNoCheck(-1);
		else
			FreeList.AddItem(Index);
	}

	unguard;
}

static inline void UnlinkRemapInner( FGL::FTextureRemap& Remap, FGL::FTexturePool::FPooledTexture& Texture)
{
	DWORD i   = Remap.Layer / 32;
	DWORD Bit = 1 << (Remap.Layer & 31);
	GL_DEV_CHECK((Texture.LayerBits[i] & Bit) != 0);

	Texture.LayerBits[i] &= ~Bit;
	Remap.PoolIndex = INDEX_NONE;
	Remap.Layer = INDEX_NONE;
}

void FGL::FTexturePool::UnlinkRemap( FGL::FTextureRemap& Remap)
{
	GL_DEV_CHECK(Remap.PoolIndex != INDEX_NONE);
	UnlinkRemapInner(Remap, Textures(Remap.PoolIndex));
}




//
// Setup Uniform Queue with default values
//
void FGL::FTexturePool::InitUniformQueue( INT QueueSize)
{
	guard(FTexturePool::InitUniformQueue);

	UniformQueue.SetSize(QueueSize);
	for ( INT i=0; i<QueueSize; i++)
	{
		FTextureUniformEntry& Entry = UniformQueue.QueueNew();
		Entry.PoolIndex = INDEX_NONE;
		Entry.UniformIndex = i;
	}
	UniformLockTag.SetValue(INDEX_NONE);
	check(UniformQueue.GetPosition() == 0);

	unguard;
}

//
// Links a pooled texture to a new Uniform Queue Item
//
bool FGL::FTexturePool::SetupUniformQueueItem( INT PoolIndex)
{
	FGL::FTexturePool::FPooledTexture& PooledTexture = Textures(PoolIndex);

	// If already in Queue, move to 'latest' and lock
	//
	// This ensures that a in a multitextured drawcall we don't purge a
	// texture uniform that has just recently been setup.
	if ( PooledTexture.UniformQueueIndex != INDEX_NONE )
	{
		if ( PooledTexture.UniformLockTag != UniformLockTag )
		{
			PooledTexture.UniformLockTag = UniformLockTag;
			INT Age = UniformQueue.GetPosition() - PooledTexture.UniformQueueIndex;
			if ( Age < 0 )
				Age += UniformQueue.Size();
			if ( Age > UniformQueue.Size() / 16 )
//			if ( PooledTexture.UniformQueueIndex != UniformQueue.GetPosition() )
			{
				FGL::FTexturePool::FTextureUniformEntry& QueueOld = UniformQueue(PooledTexture.UniformQueueIndex);
				FGL::FTexturePool::FTextureUniformEntry& QueueNew = UniformQueue.QueueNew();
				if ( QueueNew.PoolIndex != INDEX_NONE )
					Textures(QueueNew.PoolIndex).UniformQueueIndex = PooledTexture.UniformQueueIndex;
				PooledTexture.UniformQueueIndex = UniformQueue.GetPosition();
				Exchange( QueueOld, QueueNew);
			}
		}
		return false;
	}

	// If not in Queue, purge oldest entry (next in queue) and relink the uniform
	FGL::FTexturePool::FTextureUniformEntry& UniformEntry = UniformQueue.QueueNew();
	if ( Textures.IsValidIndex(UniformEntry.PoolIndex) )
	{
		check(Textures(UniformEntry.PoolIndex).UniformQueueIndex == UniformQueue.GetPosition()); //checkSlow
		Textures(UniformEntry.PoolIndex).UniformQueueIndex = INDEX_NONE;
	}
	UniformEntry.PoolIndex = PoolIndex;
	PooledTexture.UniformQueueIndex = UniformQueue.GetPosition();
	PooledTexture.UniformLockTag = UniformLockTag;
	return true;
}

//
// Update the ShaderParams UBO
//
void UOpenGLRenderDevice::UpdateTextureParamsUBO( INT ElementIndex)
{
	guard(FTexturePool::UpdateShaderParamsUBO);

	// GL must setup the queue first.
	if ( !TexturePool.UniformQueue.Size() )
		return;

	// UBO is empty, allocate and copy all data.
	if ( !bufferId_TextureParamsUBO )
	{
		ElementIndex = INDEX_NONE;
		TextureParamsData.Elements.SetSize(TexturePool.UniformQueue.Size());
		FOpenGLBase::CreateBuffer(bufferId_TextureParamsUBO, GL_ARRAY_BUFFER, TexturePool.UniformQueue.Size()*sizeof(FTextureParams_UBO::FElement), nullptr, GL_STATIC_DRAW);
		FOpenGLBase::ActiveInstance->BindBufferBase(GL_UNIFORM_BUFFER, FTextureParams_UBO::UniformIndex, bufferId_TextureParamsUBO);
	}

	FOpenGLBase::glBindBuffer(GL_UNIFORM_BUFFER, bufferId_TextureParamsUBO);
	if ( ElementIndex == INDEX_NONE )
		TextureParamsData.BufferElements();
	else
		TextureParamsData.BufferSingleElement(ElementIndex);
	FOpenGLBase::glBindBuffer(GL_UNIFORM_BUFFER, 0);
//	debugf( NAME_Init, TEXT("Texture Params Uniform update %i"), ElementIndex);

	unguard;
}

void UOpenGLRenderDevice::PrecacheTexture( FTextureInfo& Info, DWORD PolyFlags)
{
	guard(UOpenGLRenderDevice::PrecacheTexture);
	if ( Info.Texture )
	{
		FSurfaceInfo_DrawTile Surface;
		Surface.PolyFlags = PolyFlags;
		Surface.Texture   = &Info;

		FGL::Draw::TextureList TextureList;
		TextureList.Init(Surface);

		if ( !TextureList.TexRemaps[0] || TextureList.TexRemaps[0]->PoolIndex == FGL::TEXPOOL_ID_InvalidTexture )
			debugf( NAME_Warning, TEXT("OpenGLRenderDevice: unable to precache Texture %s"), Info.Texture->GetName());
	}
	unguard;
}

/*-----------------------------------------------------------------------------
	Texture queuing.

	* In order to more efficiently queue multiple texture bindings it's
	necessary to run the queuing in multiple steps to avoid duplicate
	and unnecessary code.

	* Given that UTextures do not map 1:1 to GLTextures, it's necessary to first
	query the Remap table to obtain the required Texture bindings and
	(optionally) array indices.

	* For buffering multiple drawcalls into one we need to ensure that
	Texture bindings do not change, we use the previously acquired Remaps to
	figure out if the buffering can continue or if drawing needs to be forced.

	* After forcing drawing, then a new buffering chain can start with the new
	Texture bindings found in the previous Remap query step.
-----------------------------------------------------------------------------*/

//
// Resolve the remapping of multiple UTexture into GLTexture.
// Additionally create or update GLTextures if needed.
//
void FGL::Draw::TextureList::Resolve()
{
	guard(UOpenGLRenderDevice::ResolveTextures);

	UOpenGLRenderDevice::TexturePool.Remaps.EnsureReserved(Units);

	// Get Remaps
	for ( DWORD i=0; i<Units; i++)
	{
		if ( Infos[i] == nullptr )
			TexRemaps[i] = nullptr;
		else
		{
			// Get Remap
			FGL::FTextureRemap& Remap = UOpenGLRenderDevice::TexturePool.GetRemap(*Infos[i], PolyFlags[i]);
			TexRemaps[i] = &Remap;
			Remap.CheckRealtimeChangeCount(*Infos[i]);

			// Do not modify hardcoded textures
			if ( (DWORD)Remap.PoolIndex < FGL::TEXPOOL_ID_START )
				continue;

			if ( (Remap.PoolIndex == INDEX_NONE) || (Remap.UFormat != Infos[i]->Format) || Infos[i]->bRealtimeChanged )
			{
				// Failed to upload texture, remap to invalid
				if ( !FOpenGLBase::ActiveInstance->RenDev->UploadTexture( *this, i) )
					Remap.PoolIndex = FGL::TEXPOOL_ID_InvalidTexture;
			}
		}
	}
	FOpenGLBase::ActiveInstance->TextureUnits.SetActive(0);

	unguard;
}

//
// Lock and load a DetailTexture
//
static FTextureInfo TextureInfoDetail;
FTextureInfo* FGL::Draw::LockDetailTexture( const FTextureInfo& BaseInfo)
{
	if ( BaseInfo.Texture && BaseInfo.Texture->DetailTexture )
	{
		BaseInfo.Texture->DetailTexture->Lock(TextureInfoDetail, FTime(), BaseInfo.LOD, FOpenGLBase::ActiveInstance->RenDev);
		BaseInfo.Texture->DetailTexture->Unlock(TextureInfoDetail);
		return &TextureInfoDetail;
	}
	return nullptr;
}

//
// Updates texture uniforms
//
// Return value:
// - False: All DrawBuffers needs to be flushed.
//
bool FGL::Draw::TextureList::QueueUniforms()
{
	// Light and Fog maps do not use Uniform parameters
	// TODO: Refactor this, GL 4.5 will use uniform parameters for lights and fog!!
	DWORD TextureCount = Min<DWORD>(Units, 3);

	static const INT DefaultIndices[3] = { 0, FGL::TEXPOOL_ID_NoDetail, FGL::TEXPOOL_ID_NoTexture};

	for ( INT i=0; i<TextureCount; i++)
	{
		if ( TexRemaps[i] )
		{
			auto& Texture = UOpenGLRenderDevice::TexturePool.Textures(TexRemaps[i]->PoolIndex);
			if ( UOpenGLRenderDevice::TexturePool.SetupUniformQueueItem(TexRemaps[i]->PoolIndex) )
			{
				// Update the UBO
				INT UBOindex = UOpenGLRenderDevice::TexturePool.UniformQueue(Texture.UniformQueueIndex).UniformIndex;
				UOpenGLRenderDevice::TextureParamsData.Elements(UBOindex).Scale = Infos[i]->UScale;
				UOpenGLRenderDevice::TextureParamsData.Elements(UBOindex).BindlessID = (FLOAT)INDEX_NONE;
//				if ( i == 0 )
//					debugf( NAME_DevGraphics, TEXT("Locked Texture uniform (PoolId=%i) (QueueIndex=%i) (UBOindex=%i)"), TexDrawList.TexRemaps[i]->PoolIndex, Texture.UniformQueueIndex, UBOindex);
				UOpenGLRenderDevice::UpdateTextureParamsUBO(UBOindex);
			}
			UniformIndex[i] = UOpenGLRenderDevice::TexturePool.UniformQueue(Texture.UniformQueueIndex).UniformIndex;
		}
		else
			UniformIndex[i] = DefaultIndices[i];
	}

	return true;
}


/*-----------------------------------------------------------------------------
	Texture upload utils.
-----------------------------------------------------------------------------*/

static INT GetBaseMip( FTextureInfo& Info, INT MaxTextureSize)
{
	INT i = 0;
	while ( i<Info.NumMips && (Info.Mips[i]->USize > MaxTextureSize || Info.Mips[i]->VSize > MaxTextureSize || !Info.Mips[i]->DataPtr) )
		i++;
	return i;
}

static INT GetTopMip( FTextureInfo& Info, INT BaseMip)
{
	checkSlow(BaseMip<Info.NumMips);

	INT CurrentUSize = Info.Mips[BaseMip]->USize;
	INT CurrentVSize = Info.Mips[BaseMip]->VSize;
	INT CurrentMip   = BaseMip;
NEXTMIP:
	INT NextUSize = Max( CurrentUSize/2, 1);
	INT NextVSize = Max( CurrentVSize/2, 1);
	INT NextMip   = CurrentMip + 1;
	if ( ((NextUSize != CurrentUSize) || (NextVSize != CurrentVSize)) && (NextMip < Info.NumMips) )
	{
		if ( Info.Mips[NextMip]->DataPtr && (Info.Mips[NextMip]->USize == NextUSize) && (Info.Mips[NextMip]->VSize == NextVSize) )
		{
			CurrentUSize = NextUSize;
			CurrentVSize = NextVSize;
			CurrentMip   = NextMip;
			goto NEXTMIP;
		}
	}
	return CurrentMip;
}

static INT CalcMaxLevel( FTextureInfo& Info, INT BaseMip, DWORD TexelSize)
{
	INT   MaxLevel = 0;
	DWORD USize    = (DWORD)Info.Mips[BaseMip]->USize;
	DWORD VSize    = (DWORD)Info.Mips[BaseMip]->VSize;

	while ( USize > TexelSize || VSize > TexelSize )
	{
		USize >>= 1;
		VSize >>= 1;
		MaxLevel++;
	}
	return MaxLevel;
}

static GLsizei SuggestLayerCount( const FTextureUploadState& State)
{
	constexpr SQWORD MaxAllocationSize = 1024 * 1024 * 2;

	SQWORD MemoryReq = FTextureBytes(State.Format, State.USize, State.VSize, 1);
	GL_DEV_CHECK(MemoryReq);
	GLsizei Layers = Max<GLsizei>(MaxAllocationSize / MemoryReq, State.Texture->WSize);

	return Clamp<GLsizei>(Layers, 1, FGL::FTexturePool::FPooledTexture::MAX_LAYERS);;
}

static bool CanAutogenerateMips( FTextureInfo& Info)
{
	// This is not a static texture
	if ( Info.bParametric || Info.bRealtime )
		return false;

	// Do not mipmap fog and lightmaps
	BYTE CacheType = Info.CacheID & 0xFF;
	if ( CacheType == CID_RenderFogMap || CacheType == CID_StaticMap || CacheType == CID_DynamicMap )
		return false;

	// Do not mipmap font pages
	for ( UObject* Outer=Info.Texture; Outer; Outer=Outer->GetOuter())
		if ( Outer->IsA(UFont::StaticClass()) )
			return false;

	// Disabled - need manual mipmapping!
	return false;

	// Only mipmap UTextures
	return Info.Texture != nullptr;
}

static INT GetUnusedLayer( FGL::FTexturePool::FPooledTexture& PooledTexture)
{
	for ( INT i=0; i<ARRAY_COUNT(PooledTexture.LayerBits); i++)
	{
		DWORD RLayerBits = ~PooledTexture.LayerBits[i];
		if ( RLayerBits != 0 )
		{
#if _MSC_VER
			DWORD Result;
			_BitScanForward( &Result, RLayerBits);
			return i * 32 + static_cast<INT>(Result);
#else
			return i * 32 + __builtin_ctz(RLayerBits);
#endif
		}
	}
	return PooledTexture.MAX_LAYERS;
}

//
// Validate the Remap's current texture and assign it to state.
// 
// If the Remap requires a reallocation then the texture will be
// unlinked and potentially deleted.
//
static bool ValidatePooledTexture( FTextureUploadState& State, FGL::FTextureRemap& Remap)
{
	using namespace FGL;

	if ( Remap.PoolIndex != INDEX_NONE )
	{
		GL_DEV_CHECK(Remap.Layer != INDEX_NONE);
		auto& PooledTexture = UOpenGLRenderDevice::TexturePool.Textures(Remap.PoolIndex);

		// If Unreal hasn't changed the format, select existing texture
		if ( Remap.UFormat == State.InitialFormat )
		{
			State.Layer = Remap.Layer;
			State.Texture = &PooledTexture;
			return true;
		}

		INT PoolIndex = Remap.PoolIndex;
//		debugf(NAME_DevGraphics, TEXT("Invalidated Remap to %i"), PoolIndex);
		UnlinkRemapInner(Remap, PooledTexture);

		// Delete the Texture if none of its layers is being used
		DWORD LayerBits = 0;
#if USES_SSE_INTRINSICS
		__m128i mm_layer_bits_0 = _mm_loadu_si128 ((__m128i const*)&PooledTexture.LayerBits[0]);
		__m128i mm_layer_bits_1 = _mm_loadu_si128 ((__m128i const*)&PooledTexture.LayerBits[4]);
		__m128i mm_cmp_result = _mm_cmpeq_epi32(_mm_or_si128(mm_layer_bits_0, mm_layer_bits_1), _mm_setzero_si128());
		LayerBits = _mm_movemask_epi8(mm_cmp_result);
#else
		for ( DWORD i=0; i<ARRAY_COUNT(PooledTexture.LayerBits); i++)
			LayerBits |= PooledTexture.LayerBits[i];
#endif
		if ( LayerBits == 0 )
			UOpenGLRenderDevice::TexturePool.DeleteTexture(PoolIndex);
	}
	return false;
}

//
// Selects (and potentially creates) a texture for this Remap
//
static void AssignPooledTexture( FTextureUploadState& State, FGL::FTextureRemap& Remap, const FTextureFormatInfo& Fmt, bool MipMapped)
{
	using namespace FGL;

	Remap.UFormat = State.InitialFormat;

	bool UseArray = !!UOpenGLRenderDevice::TexturePool.UseArrayTextures;
	DWORD TextureKey = 0;

	if ( UseArray )
	{
		// Obtain Key for fast lookup
		FOpenGLTexture Reference;
		Reference.USize = static_cast<GLushort>(State.USize);
		Reference.VSize = static_cast<GLushort>(State.VSize);
		Reference.MaxLevel += static_cast<GLushort>(MipMapped); // Ugly
		Reference.Swizzle = State.Swizzle;
		Reference.Format = Fmt.InternalFormat;
		TextureKey = Reference.EncodeKey();

		// Find existing array texture with matching qualities, data and a free layer
		for ( INT i=TEXPOOL_ID_START; i<UOpenGLRenderDevice::TexturePool.Textures.Num(); i++)
		{
			auto& PooledTexture = UOpenGLRenderDevice::TexturePool.Textures(i);
			if ( (PooledTexture.TextureKey == TextureKey) && PooledTexture.Texture && (PooledTexture.Target == GL_TEXTURE_2D_ARRAY) )
			{
				// Size checks may fail here because textures don't need to be preallocated
				INT Layer = GetUnusedLayer(PooledTexture);
				if ( Layer < (INT)PooledTexture.WSize )
				{
					Remap.PoolIndex = i;
					State.Texture = &PooledTexture;
					State.Layer = Remap.Layer = Layer;
					PooledTexture.LayerBits[Layer / 32] |= (1ul << (Layer & 31));
					return;
				}
			}
		}
	}

	// Create new texture and set default parameters (WSize not set yet).
	auto& PooledTexture = UOpenGLRenderDevice::TexturePool.CreateTexture(UseArray ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D, Remap.PoolIndex);
	State.Texture = &PooledTexture;
	State.Layer = Remap.Layer = 0;
	PooledTexture.LayerBits[0] = 1;
	PooledTexture.TextureKey = TextureKey;
}


/*-----------------------------------------------------------------------------
	Texture allocation, composition and transfer.
-----------------------------------------------------------------------------*/

void UOpenGLRenderDevice::UpdateTextureRect( FTextureInfo& Info, INT U, INT V, INT UL, INT VL)
{
	guard(UOpenGLRenderDevice::UpdateTextureRect);

	if ( (Info.NumMips <= 0) || !Info.Mips[0]->DataPtr || !FOpenGLBase::ActiveInstance )
		return;

	// UpdateTextureRect is used by the lightmap atlas system to allow partial updates
	// prior to a DrawComplexSurface call, doing nothing here will cause the whole texture to
	// be updated afterwards.
	// This is the case for a texture that hasn't yet been fully created.

	// Get Remap
	FGL::FTextureRemap& Remap = TexturePool.GetRemap(Info, 0);
	if ( Remap.PoolIndex == INDEX_NONE )
		return;

	// Get Texture
	FOpenGLTexture& Texture = TexturePool.Textures(Remap.PoolIndex);
	check(TexturePool.Textures.IsValidIndex(Remap.PoolIndex));
	if ( !Texture.Allocated )
		return;

	// When not bound, choose an unused texture mapping unit to bind to
	INT i;
	for ( i=0; i<TMU_Position; i++)
		if ( FOpenGLBase::ActiveInstance->TextureUnits.IsBound(Texture, i) )
			break;

	// Bind and set texture
	FOpenGLBase::ActiveInstance->TextureUnits.SetActive(i);
	FOpenGLBase::ActiveInstance->TextureUnits.Bind(Texture);
	Info.bRealtimeChanged = 0;

	// Allocate temporary data and copy from source
	FTextureUploadState State(&Info);

	FMemMark Mark(GMem);
	INT DataBlock = FTextureBlockBytes( Info.Format);
	INT DataSize = FTextureBytes( Info.Format, UL, VL);
	State.USize = UL;
	State.VSize = VL;
	State.Data = new(GMem,DataSize) BYTE;

	{
		INT USize = Info.Mips[0]->USize;
		BYTE* Input  = Info.Mips[0]->DataPtr + (USize * V + U) * DataBlock;
		BYTE* Output = State.Data;
		BYTE* OutputEnd = Output + DataSize;
		while ( Output < OutputEnd )
		{
			appMemcpy( Output, Input, UL * DataBlock);
			Input += USize * DataBlock;
			Output += UL * DataBlock;
		}
	}

	f_TextureConvert SoftwareConversion = State.SelectSoftwareConversion();
	if ( SoftwareConversion )
		SoftwareConversion(State);

	// Upload sub image
	const FTextureFormatInfo& Fmt = FTextureFormatInfo::Get(State.Format);
	if ( !Fmt.Compressed ) // Ignore compressed textures (lightmaps are never compressed)
	{
		if ( Texture.Target == GL_TEXTURE_2D_ARRAY )
			FOpenGLBase::glTexSubImage3D(Texture.Target, State.Level, U, V, Remap.Layer, UL, VL, 1, Fmt.SourceFormat, Fmt.Type, State.Data);
		else
			FOpenGLBase::glTexSubImage2D(Texture.Target, State.Level, U, V, UL, VL, Fmt.SourceFormat, Fmt.Type, State.Data);
	}
	FOpenGLBase::ActiveInstance->TextureUnits.SetActive(0);

	Mark.Pop();

	unguard;
}

// TODO: Turn into global/static
bool FASTCALL UOpenGLRenderDevice::UploadTexture( const FGL::Draw::TextureList& TexDrawList, INT TexIndex)
{
	guard(UOpenGLRenderDevice::UploadTexture);
	using namespace FGL;

	// Setup Upload State
	FTextureUploadState State( TexDrawList.Infos[TexIndex], TexDrawList.PolyFlags[TexIndex]);
	State.Info->bRealtimeChanged = 0;

	// Check texture validity and select first mipmap.
	if ( State.Info->NumMips <= 0 )
	{
		debugf(NAME_DevGraphics, TEXT("UploadTexture: invalid mip count: %i (CacheID=%016X)"), State.Info->NumMips, State.Info->CacheID);
		return false;
	}

	// Base mip selection
	INT BaseMip = GetBaseMip( *State.Info, FOpenGLBase::MaxTextureSize);
	if ( BaseMip >= State.Info->NumMips )
	{
		debugf(NAME_DevGraphics, TEXT("UploadTexture: oversized texture, BaseMip=%i, NumMips=%i, MaxTextureSize=%i (CacheID=%016X)"), BaseMip, State.Info->NumMips, FOpenGLBase::MaxTextureSize, State.Info->CacheID);
		return false;
	}
	INT AutoMaxLevel = 0;

	// Deferred load
	if ( SupportsLazyTextures )
		State.Info->Load();

	// Populate base mip size
	{
		FMipmapBase& Mip = *State.Info->Mips[BaseMip];
		State.CurrentMip = BaseMip;
		State.USize = Mip.USize;
		State.VSize = Mip.VSize;
	}

	// Format selection (plus conversion)
	f_TextureConvert HardwareConversion = State.SelectHardwareConversion();
	f_TextureConvert SoftwareConversion = !HardwareConversion ? State.SelectSoftwareConversion() : nullptr;
	const FTextureFormatInfo& Fmt = FTextureFormatInfo::Get(State.Format);
	if ( !Fmt.Supported )
	{
		debugf(NAME_DevGraphics, TEXT("UploadTexture: unsupported texture format %s"), *FTextureFormatString(State.Format));
		return false;
	}

	// Calculate top mip so no unnecessary storage is done
	INT TopMip = GetTopMip(*State.Info, BaseMip);
	if ( (BaseMip == TopMip) && AlwaysMipmap && CanAutogenerateMips(*State.Info) )
		AutoMaxLevel = CalcMaxLevel( *State.Info, BaseMip, FTextureBlockWidth(State.Format) );

	// Validate or setup texture remap
	FTextureRemap& Remap = *TexDrawList.TexRemaps[TexIndex];
	if ( !ValidatePooledTexture(State, Remap) )
		AssignPooledTexture(State, Remap, Fmt, BaseMip != TopMip);

	GL->TextureUnits.SetActive(TMU_Position);
	GL->TextureUnits.Bind(*State.Texture);

	// New texture needs to be allocated, set default parameters and storage
	if ( !State.Texture->Allocated )
	{
		FOpenGLBase::SetTextureFilters(*State.Texture, false);
		if ( State.Swizzle )
			GL->TextureUnits.SetSwizzle(State.Swizzle);

		GLsizei Mips   = 1 + (AutoMaxLevel ? AutoMaxLevel : TopMip-BaseMip);
		GLsizei Layers = SuggestLayerCount(State);
		FOpenGLBase::SetTextureStorage(*State.Texture, Fmt, State.USize, State.VSize, Layers, Mips);

		if ( !State.Texture->Allocated )
			return false;
	}

	for ( State.CurrentMip=BaseMip; State.CurrentMip<=TopMip; State.CurrentMip++)
	{
		FMipmapBase& Mip = *State.Info->Mips[State.CurrentMip];
		State.Data   = Mip.DataPtr;
		State.USize  = Mip.USize; 
		State.VSize  = Mip.VSize;

		// Run hardware data conversion and upload on this mip
		if ( HardwareConversion )
		{
			HardwareConversion(State);
			State.Level++;
			continue;
		}

		// Run software data conversion on this mip prior to data upload
		if ( SoftwareConversion )
			SoftwareConversion(State);

		if ( State.Data )
		{
			GLsizei Width  = State.USize;
			GLsizei Height = State.VSize;
			GLsizei TextureBytes = Fmt.GetTextureBytes(Width, Height);
			FOpenGLBase::SetTextureData(*State.Texture, Fmt, State.Level, Width, Height, State.Layer, State.Data, TextureBytes);
			State.Level++;
		}
		else
		{
			// TODO: Run mipmap generation

			// If a sub-mip failed to be converted and uploaded, stop here.
			if ( State.Level > 0 )
				break;
		}
	}

	// Unbind Texture from upload unit
	if ( GL->TextureUnits.Active == TMU_Position )
		GL->TextureUnits.Unbind();

	if ( State.Level == 0 || !State.Texture->Allocated )
	{
		debugf(NAME_DevGraphics, TEXT("UploadTexture: no data uploaded (Format=%s) (CacheID=%016X)"), *FTextureFormatString(State.Format), State.Info->NumMips, State.Info->CacheID);
		return false;
	}

	// Update GL texture information
	if ( AutoMaxLevel )
	{
		bool Generated = FOpenGLBase::GenerateTextureMipmaps(*State.Texture);
		GL_DEV_CHECK(Generated);
		(void)Generated;
	}

	if ( SupportsLazyTextures )
		State.Info->Unload();

	return true;

	unguard;
}

//
// Array Texture preallocator
// Evaluates all loaded Textures in the game
//
void FGL::FTexturePool::PreallocateArrayTextures()
{
	guard(FTexturePool::PreallocateArrayTextures);

	if ( !UTexture::__Client )
		return;

	UOpenGLRenderDevice* RenDev = FOpenGLBase::ActiveInstance->RenDev;
	INT* TextureLODSet = UTexture::__Client->TextureLODSet;

	// TODO: Eliminate need to key UTexture scale
	// UTexture scale should be stored in a separate uniform that
	// doesn't depend on the ArrayTexture itself
	// ArrayTextures should all be compatible regardless of scaling
	TMapExt<DWORD,INT> TextureCountMap;

	// Use these to encode a key
	FOpenGLTexture Reference;
	FTextureInfo Info;

	for ( TObjectIterator<UTexture> TIt; TIt; ++TIt)
	{
		UTexture* Tex = *TIt;

		// Pick a format
		ETextureFormat   Format = TEXF_P8;
		TArray<FMipmap>* Mips = nullptr;
		if ( Tex->bHasComp && RenDev->SupportsTextureFormat((ETextureFormat)Tex->CompFormat) )
		{
			Format = (ETextureFormat)Tex->CompFormat;
			Mips   = &Tex->CompMips;
		}
		else if ( !Tex->Format || RenDev->SupportsTextureFormat((ETextureFormat)Tex->Format) )
		{
			Format = (ETextureFormat)Tex->Format;
			Mips   = &Tex->Mips;
		}
		
		if ( !Mips || !Mips->Num() )
			continue;

		// Pick a LOD.
		BYTE LODSet = Min<BYTE>(Tex->LODSet, LODSET_MAX-1);
		BYTE LOD    = Min<BYTE>(TextureLODSet[LODSet], Min(Mips->Num()-1, MAX_TEXTURE_LOD-1));

		// Alter format
		FOpenGLSwizzle Swizzle(0);
		if ( Format == TEXF_P8 && Tex->Palette && (Tex->Palette->Colors.Num() >= NUM_PAL_COLORS) )
		{
			// TODO: Swizzled storage will not be needed where Texture Views can be used
			if ( FOpenGLBase::SupportsTextureSwizzle && PaletteGrayscaleFont(&Tex->Palette->Colors(0)) )
			{
				Format = TEXF_R8;
				Swizzle = FOpenGLSwizzle( SWZ_RED, SWZ_RED, SWZ_RED, SWZ_RED);
			}
		}
		if ( Format == TEXF_P8 )
			Format = TEXF_RGBA8_;

		// Validate format
		const FTextureFormatInfo& Fmt = FTextureFormatInfo::Get(Format);
		if ( !Fmt.Supported )
		{
			if ( GL_DEV )
				debugf(NAME_DevGraphics, TEXT("DISCARD TEXTURE (unsupported format): %s"), *FTextureFormatString(Format) );
			continue;
		}

		// Encode key
		const FMipmap& Mip = Mips->operator()(LOD);
		Reference.USize = static_cast<GLushort>(Mip.USize);
		Reference.VSize = static_cast<GLushort>(Mip.VSize);
		Reference.MaxLevel = static_cast<GLushort>(Mips->Num()-1 > LOD); // Ugly
		Reference.Swizzle = Swizzle;
		Reference.Format = Fmt.InternalFormat;

		// Count up
		INT& Counter = TextureCountMap.FindOrSet(Reference.EncodeKey(), 0);
		Counter++;
	}

	for ( TMap<DWORD,INT>::TIterator MIt(TextureCountMap); MIt; ++MIt)
	{
		// Create unallocated textures and suggest minimum layer count for each
		for ( INT c=MIt.Value(); c>0; c-=FPooledTexture::MAX_LAYERS)
		{
			INT PoolIndex;
			auto& PooledTexture = CreateTexture(GL_TEXTURE_2D_ARRAY, PoolIndex);
			PooledTexture.TextureKey = MIt.Key();
			PooledTexture.WSize = Min<GLsizei>(c, FPooledTexture::MAX_LAYERS);
		}
	}

	unguard;
}
