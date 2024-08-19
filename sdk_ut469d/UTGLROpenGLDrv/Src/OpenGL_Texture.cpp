/*=============================================================================
	OpenGL_Texture.cpp: Texture handling code for OpenGLDrv.

	Revision history:
	* Created by Fernando Velazquez
	* Texture Pooling system by Fernando Velazquez
=============================================================================*/

#include "OpenGLDrv.h"
#include "UTGLROpenGL.h"

#define GL_COLOR_INDEX 0x1900


/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

FGL::FTexturePool UOpenGLRenderDevice::TexturePool;


/*-----------------------------------------------------------------------------
	Texture upload descriptors.
-----------------------------------------------------------------------------*/

void UOpenGLRenderDevice::UpdateTextureFormat( UBOOL& FlushTextures)
{
	guard(UOpenGLRenderDevice::UpdateTextureFormat);
	if ( !m_FormatInfo )
	{
		appMemzero( TextureFormatInfo, sizeof(TextureFormatInfo));

		// Initialize state locks
		PL_Use16BitTextures = 0;

		// Palette (default behaviour is to convert to RGBA8)
		RegisterTextureFormat( TEXF_P8,        GL_COLOR_INDEX8_EXT,              GL_COLOR_INDEX,   GL_UNSIGNED_BYTE);
		RegisterTextureConversion( TEXF_P8, &UOpenGLRenderDevice::ConvertP8_RGBA8);

		// BGRA7 Light/fog maps
		RegisterTextureFormat( TEXF_BGRA8_LM,  GL_RGBA8,                         GL_BGRA,          GL_UNSIGNED_BYTE);
//		RegisterTextureConversion( TEXF_BGRA8_LM, &UOpenGLRenderDevice::ConvertBGRA7_BGRA8);

		// BGR10A2 Light/fog maps
		RegisterTextureFormat( TEXF_RGB10A2_LM,GL_RGB10_A2,                      GL_RGBA,          GL_UNSIGNED_INT_2_10_10_10_REV);

		// Standard RGBA - BGRA
		RegisterTextureFormat( TEXF_RGBA8_,    GL_RGBA8,                         GL_RGBA,          GL_UNSIGNED_BYTE);
		RegisterTextureFormat( TEXF_BGRA8,     GL_RGBA8,                         GL_BGRA,          GL_UNSIGNED_BYTE);
		RegisterTextureFormat( TEXF_RGBA16,    GL_RGBA16,                        GL_RGBA,          GL_UNSIGNED_SHORT);

		// Shared exponent RGB
		if ( FOpenGLBase::SupportsRGB9_E5 )
			RegisterTextureFormat( TEXF_RGB9E5,GL_RGB9_E5_EXT,                   GL_RGB,           GL_UNSIGNED_INT_5_9_9_9_REV_EXT);
		
		// R8 (requires GL 3.0)
		RegisterTextureFormat( TEXF_R8,        GL_R8,                            GL_RED,           GL_UNSIGNED_BYTE);

		// S3TC
		if ( FOpenGLBase::SupportsS3TC )
		{
			RegisterTextureFormat( TEXF_BC1,   /*GL_COMPRESSED_RGB_S3TC_DXT1_EXT*/GL_COMPRESSED_RGBA_S3TC_DXT1_EXT); //DieHard's HD textures need this
			RegisterTextureFormat( TEXF_BC1_PA,GL_COMPRESSED_RGBA_S3TC_DXT1_EXT);
			RegisterTextureFormat( TEXF_BC2,   GL_COMPRESSED_RGBA_S3TC_DXT3_EXT);
			RegisterTextureFormat( TEXF_BC3,   GL_COMPRESSED_RGBA_S3TC_DXT5_EXT);
		}
		// RGTC
		if ( FOpenGLBase::SupportsRGTC )
		{
			RegisterTextureFormat( TEXF_BC4,   GL_COMPRESSED_RED_RGTC1);
			RegisterTextureFormat( TEXF_BC4_S, GL_COMPRESSED_SIGNED_RED_RGTC1);
			RegisterTextureFormat( TEXF_BC5,   GL_COMPRESSED_RG_RGTC2);
			RegisterTextureFormat( TEXF_BC5_S, GL_COMPRESSED_SIGNED_RG_RGTC2);
		}
		// BPTC
		if ( FOpenGLBase::SupportsBPTC )
		{
			RegisterTextureFormat( TEXF_BC6H,  GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB);
			RegisterTextureFormat( TEXF_BC6H_S,GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB);
			RegisterTextureFormat( TEXF_BC7,   GL_COMPRESSED_RGBA_BPTC_UNORM_ARB);
		}

		m_FormatInfo = 1;
	}

	if ( Use16BitTextures != PL_Use16BitTextures )
	{
		TextureFormatInfo[TEXF_RGBA8_].InternalFormat = Use16BitTextures ? GL_RGB5_A1 : GL_RGBA8;
		FlushTextures = true;
	}

	if ( UseHDTextures != (UBOOL)SupportsTC )
		FlushTextures = true;

	if ( UseHDTextures != (UBOOL)SupportsTC
	||   AlwaysMipmap  != PL_AlwaysMipmap )
		FlushTextures = true;

	unguard;
}

void UOpenGLRenderDevice::RegisterTextureFormat( BYTE Format, GLenum InternalFormat, GLenum SourceFormat, GLenum Type)
{
	FTextureFormatInfo& Desc = TextureFormatInfo[Format];
	Desc                 = FTextureFormatInfo((ETextureFormat)Format);
	Desc.Supported       = InternalFormat != 0;
	Desc.InternalFormat  = InternalFormat;
	Desc.SourceFormat    = SourceFormat ? SourceFormat : InternalFormat;
	Desc.Type            = Type;
	Desc.SoftwareConvert = &UOpenGLRenderDevice::ConvertIdentity;
}

void UOpenGLRenderDevice::RegisterTextureConversion( BYTE Format, f_SoftwareConvert SoftwareConvert)
{
	FTextureFormatInfo& Desc = TextureFormatInfo[Format];
	if ( !SoftwareConvert || (SoftwareConvert == &UOpenGLRenderDevice::ConvertIdentity) )
	{
		Desc.Supported       = true;
		Desc.SoftwareConvert = &UOpenGLRenderDevice::ConvertIdentity;
	}
	else
	{
		Desc.Supported       = false;
		Desc.SoftwareConvert = SoftwareConvert;
	}
}

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

	constexpr GLint    Size   = 64;
	constexpr GLushort XTexel = (31 << 0) | (0 << 5) | (31 << 10) | (1 << 15);

	Texture.Create(GL_TEXTURE_2D_UTEXTURE);
	FOpenGLBase::ActiveInstance->TextureUnits.Bind(Texture);

	TArray<GLushort> Texels;
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

	// Setup format descriptor for SetTextureStorage
	FTextureFormatInfo Format;
	Format.InternalFormat = GL_RGB5_A1;
	Format.SourceFormat   = GL_RGBA;
	Format.Type           = GL_UNSIGNED_SHORT_1_5_5_5_REV;
	Format.Compressed     = 0;

	FOpenGLBase::SetTextureStorage(Texture, Format, Size, Size, 1, 1);
	if ( Texture.Target == GL_TEXTURE_2D_ARRAY )
		FOpenGLBase::glTexSubImage3D(Texture.Target, 0, 0, 0, 0, Size, Size, 1, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, &Texels(0));
	else
		FOpenGLBase::glTexSubImage2D(Texture.Target, 0, 0, 0, Size, Size, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, &Texels(0));

	Texture.MagNearest = true;
	FOpenGLBase::glTexParameteri(Texture.Target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	FOpenGLBase::glTexParameteri(Texture.Target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

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

	Texture.Create(GL_TEXTURE_2D);
	FOpenGLBase::ActiveInstance->TextureUnits.Bind(Texture);

	GLuint Level = 0;
	GLuint CurrentSize = Size;
	if ( FOpenGLBase::SupportsRGTC )
	{
		// BC4 compressed image
		FOpenGLBase::SetTextureStorage(Texture, FOpenGLBase::ActiveInstance->RenDev->TextureFormatInfo[TEXF_BC4], Size, Size, 1, Levels);

		TArray<QWORD> TexelData;
		TexelData.SetSize( FTextureBytes(TEXF_BC4, Size, Size) );
		for ( Level=0; Level<Levels; Level++)
		{
			INT TexelCount = Square(Align(CurrentSize,4)/4);
			QWORD Texel = 0 | CurrentSize | (CurrentSize << 8); // Encode Red1, Red2, leave other bits as 0
			for ( INT i=0; i<TexelCount; i++)
				TexelData(i) = Texel;
			FOpenGLBase::glCompressedTexSubImage2D(Texture.Target, Level, 0, 0, CurrentSize, CurrentSize, GL_COMPRESSED_RED_RGTC1, TexelCount*8, &TexelData(0));
			CurrentSize >>= 1;
		}
	}
	else
	{
		// R8 uncompressed image
		FOpenGLBase::SetTextureStorage(Texture, FOpenGLBase::ActiveInstance->RenDev->TextureFormatInfo[TEXF_R8], Size, Size, 1, Levels);

		TArray<BYTE> ImageData;
		ImageData.SetSize( Size * Size);
		for ( Level=0; Level<Levels; Level++)
		{
			appMemset( &ImageData(0), CurrentSize, CurrentSize*CurrentSize);
			FOpenGLBase::glTexSubImage2D(Texture.Target, Level, 0, 0, CurrentSize, CurrentSize, GL_RED, GL_UNSIGNED_BYTE, &ImageData(0));
			CurrentSize >>= 1;
		}
	}
	FOpenGLBase::glTexParameteri(Texture.Target, GL_TEXTURE_MIN_FILTER, FGL::GetLinearMinificationFilter(true));
	FOpenGLBase::glTexParameteri(Texture.Target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	unguard;
}

static void ComposePlainColor( FOpenGLTexture& Texture, FColor Color)
{
	guard(ComposeNoDetail);

	constexpr GLint Size = 2;

	DWORD Data[Size*Size];
	for ( INT i=0; i<Size*Size; i++)
		Data[i] = GET_COLOR_DWORD(Color);

	Texture.Create(GL_TEXTURE_2D_UTEXTURE);
	FOpenGLBase::ActiveInstance->TextureUnits.Bind(Texture);
	FOpenGLBase::SetTextureStorage(Texture, FOpenGLBase::ActiveInstance->RenDev->TextureFormatInfo[TEXF_RGBA8_], Size, Size, 1, 1);

	if ( Texture.Target == GL_TEXTURE_2D_ARRAY )
		FOpenGLBase::glTexSubImage3D(Texture.Target, 0, 0, 0, 0, Size, Size, 1, GL_RGBA, GL_UNSIGNED_BYTE, Data);
	else
		FOpenGLBase::glTexSubImage2D(Texture.Target, 0, 0, 0, Size, Size, GL_RGBA, GL_UNSIGNED_BYTE, Data);

	FOpenGLBase::glTexParameteri(Texture.Target, GL_TEXTURE_MIN_FILTER, FGL::GetLinearMinificationFilter(false));
	FOpenGLBase::glTexParameteri(Texture.Target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

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

	while ( Textures.Num() < TEXPOOL_ID_START )
		Textures.AddItem( FPooledTexture() );

	if ( !Textures(TEXPOOL_ID_InvalidTexture).Texture )
		ComposeInvalidTexture(Textures(TEXPOOL_ID_InvalidTexture));

	if ( !Textures(TEXPOOL_ID_DervMap).Texture )
		ComposeDervMap(Textures(TEXPOOL_ID_DervMap));

	if ( !Textures(TEXPOOL_ID_NoDetail).Texture )
		ComposePlainColor(Textures(TEXPOOL_ID_NoDetail), FColor(127,127,127,255) );

	if ( !Textures(TEXPOOL_ID_NoTexture).Texture )
		ComposePlainColor(Textures(TEXPOOL_ID_NoTexture), FColor(255,255,255,255) );

	if ( PostFlush && UseArrayTextures )
	{
		PreallocateArrayTextures();
	}

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

	TArray<GLuint> Binds;
	Binds.Reserve(Textures.Num());
	for ( INT i=0; i<Textures.Num(); i++)
		if ( Textures(i).Texture )
		{
			INT Index = Binds.AddNoCheck(1);
			Binds(Index) = Textures(i).Texture;
		}

	if ( Binds.Num() )
		FOpenGLBase::glDeleteTextures( Binds.Num(), (GLuint*)&Binds(0));

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
		FOpenGLBase::CreatePersistentBuffer(bufferId_TextureParamsUBO, GL_UNIFORM_BUFFER, TexturePool.UniformQueue.Size()*sizeof(FTextureParams_UBO::FElement), nullptr, GL_DYNAMIC_STORAGE_BIT, GL_STATIC_DRAW);
	}

	FOpenGLBase::glBindBufferBase(GL_UNIFORM_BUFFER, FTextureParams_UBO::UniformIndex, bufferId_TextureParamsUBO);
	if ( ElementIndex == INDEX_NONE )
		TextureParamsData.BufferElements();
	else
		TextureParamsData.BufferSingleElement(ElementIndex);
	FOpenGLBase::glBindBuffer(GL_UNIFORM_BUFFER, 0);
//	debugf( NAME_Init, TEXT("Texture Params Uniform update %i"), ElementIndex);

	unguard;
}


/*-----------------------------------------------------------------------------
	Texture support query.
-----------------------------------------------------------------------------*/

UBOOL UOpenGLRenderDevice::SupportsTextureFormat( ETextureFormat Format)
{
	// P8 is always supported
	if ( Format == TEXF_P8 )
		return true;

	// Compressed formats require SupportsTC flag
	const FTextureFormatInfo& Desc = TextureFormatInfo[Format];
	if ( Desc.Compressed && !SupportsTC )
		return false;

	// We don't "support" textures that require software conversion
	return Desc.Supported != 0;
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
	Texture array merger.
	Deprecated
-----------------------------------------------------------------------------*/

void FGL::FTexturePool::TextureArrayMergeStage0()
{
	guard(FTexturePool::TextureArrayMergeStage0);
	unguard;
}

void FGL::FTexturePool::TextureArrayMergeStage1()
{
	guard(FTexturePool::TextureArrayMergeStage1);
	unguard;
}

void FGL::FTexturePool::TextureArrayMergeStage2_BufferedCopy()
{
	guard(FTexturePool::TextureArrayMergeStage2_BufferedCopy);

	// Code kept for later use
	// 
	// 
	// Allocate everything
/*	if ( !MergeScanner.Buffer )
	{
		check(!MergeScanner.Texture.Texture); //checkSlow

		GLsizei BaseLayers  = BaseTexture.WSize;
		GLsizei MergeLayers = MergeScanner.PendingMerge.Num() - 1;
		MergeScanner.Texture.Create(GL_TEXTURE_2D_ARRAY);
		FOpenGLBase::glBindTexture(GL_TEXTURE_2D_ARRAY, MergeScanner.Texture.Texture);
		FOpenGLBase::SetTextureStorage(MergeScanner.Texture, *MergeScanner.Format, BaseTexture.USize, BaseTexture.VSize, BaseLayers+MergeLayers, BaseTexture.MaxLevel+1);
		FOpenGLBase::SetTextureFilters(MergeScanner.Texture, BaseTexture.MagNearest);
		if ( BaseTexture.Swizzle )
			GL->TextureUnits.SetSwizzle(BaseTexture.Swizzle);

		// Due to API limitations the buffer must be able to contain the Target texture for the base copy
		FOpenGLBase::CreateBuffer(MergeScanner.Buffer, GL_PIXEL_PACK_BUFFER, TextureSize*BaseLayers, nullptr, GL_STREAM_COPY);
//		debugf( NAME_DevGraphics, TEXT("MergeScanner: created buffer of Size=%ik, (Contains %i layers) (C=%i/%04X)"), TextureSize * BaseLayers / 1024, BaseLayers, (INT)MergeScanner.Format->Compressed, (INT)MergeScanner.Texture.Format);
	}*/

/*	for ( GLushort i=0; i<=BaseTexture.MaxLevel; i++)
	{
		// Copy base to buffer
		FOpenGLBase::glBindBuffer(GL_PIXEL_PACK_BUFFER, MergeScanner.Buffer);
		FOpenGLBase::glBindTexture(GL_TEXTURE_2D_ARRAY, BaseTexture.Texture);
		if ( BaseTexture.Compressed )
			FOpenGLBase::glGetCompressedTexImage(GL_TEXTURE_2D_ARRAY, i, 0);
		else
			FOpenGLBase::glGetTexImage(GL_TEXTURE_2D_ARRAY, i, MergeScanner.Format->SourceFormat, MergeScanner.Format->Type, 0);
		FOpenGLBase::glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

		// Copy buffer to merged
		FOpenGLBase::glBindBuffer(GL_PIXEL_UNPACK_BUFFER, MergeScanner.Buffer);
		FOpenGLBase::glBindTexture(GL_TEXTURE_2D_ARRAY, MergeScanner.Texture.Texture);
		if ( BaseTexture.Compressed )
			FOpenGLBase::glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, i, 0, 0, 0, USize, VSize, BaseTexture.WSize, MergeScanner.Format->SourceFormat, TextureSize*BaseTexture.WSize, 0);
		else
			FOpenGLBase::glTexSubImage3D(GL_TEXTURE_2D_ARRAY, i, 0, 0, 0, USize, VSize, BaseTexture.WSize, MergeScanner.Format->SourceFormat, MergeScanner.Format->Type, 0);
		FOpenGLBase::glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

		if ( i != BaseTexture.MaxLevel )
		{
			USize = Max<GLuint>(USize >> 1, 1);
			VSize = Max<GLuint>(VSize >> 1, 1);
			TextureSize = MergeScanner.Format->GetTextureBytes(USize, VSize, 1);
		}
	}*/


	unguard;
}

void FGL::FTexturePool::TextureArrayMergeStage3_BufferedCopy()
{
	guard(FTexturePool::TextureArrayMergeStage3_BufferedCopy);

	// Code kept for later use
	// 
	// 
	// Copy to buffer
/*	if ( MergeScanner.PendingIndex < MergeScanner.PendingMerge.Num() )
	{
		check(MergeScanner.PendingLayer != 0);
		check(MergeScanner.PendingLayer < (GLint)BaseTexture.WSize);
		auto& NewRemap = Remaps(MergeScanner.PendingMerge(MergeScanner.PendingIndex));

		for ( GLushort i=0; i<=BaseTexture.MaxLevel; i++)
		{
			GLsizeiptr TextureSize = MergeScanner.Format->GetTextureBytes(USize, VSize, 1);

			// Copy new to buffer (if there is a mip, otherwise Base will get previous data)
			if ( i <= Textures(NewRemap.PoolIndex).MaxLevel )
			{
				FOpenGLBase::glBindBuffer(GL_PIXEL_PACK_BUFFER, MergeScanner.Buffer);
				FOpenGLBase::glBindTexture(GL_TEXTURE_2D_ARRAY, Textures(NewRemap.PoolIndex).Texture);
				if ( BaseTexture.Compressed )
					FOpenGLBase::glGetCompressedTexImage(GL_TEXTURE_2D_ARRAY, i, 0);
				else
					FOpenGLBase::glGetTexImage(GL_TEXTURE_2D_ARRAY, i, MergeScanner.Format->SourceFormat, MergeScanner.Format->Type, 0);
				FOpenGLBase::glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
			}

			// Copy buffer to base
			FOpenGLBase::glBindBuffer(GL_PIXEL_UNPACK_BUFFER, MergeScanner.Buffer);
			FOpenGLBase::glBindTexture(GL_TEXTURE_2D_ARRAY, BaseTexture.Texture);
			if ( BaseTexture.Compressed )
				FOpenGLBase::glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, i, 0, 0, MergeScanner.PendingLayer, USize, VSize, 1, MergeScanner.Format->SourceFormat, TextureSize, 0);
			else
				FOpenGLBase::glTexSubImage3D(GL_TEXTURE_2D_ARRAY, i, 0, 0, MergeScanner.PendingLayer, USize, VSize, 1, MergeScanner.Format->SourceFormat, MergeScanner.Format->Type, 0);
			FOpenGLBase::glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

			if ( i != BaseTexture.MaxLevel )
			{
				USize = Max<GLuint>(USize >> 1, 1);
				VSize = Max<GLuint>(VSize >> 1, 1);
			}
		}

		if ( (OldTexture != BaseTexture.Texture) && (OldTexture != Textures(NewRemap.PoolIndex).Texture) )
			FOpenGLBase::glBindTexture(GL_TEXTURE_2D_ARRAY, OldTexture);
	}*/

	unguard;
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
	GLsizei Layers = MaxAllocationSize / MemoryReq;

	if ( State.Texture && ((GLsizei)State.Texture->WSize > Layers) )
		Layers = State.Texture->WSize;

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
static void ValidatePooledTexture( FTextureUploadState& State, FGL::FTextureRemap& Remap)
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
			return;
		}

		INT PoolIndex = Remap.PoolIndex;
//		debugf(NAME_DevGraphics, TEXT("Invalidated Remap to %i"), PoolIndex);
		UnlinkRemapInner(Remap, PooledTexture);

		// Delete the Texture if none of its layers is being used
		if ( PooledTexture.LayerBits == 0 )
			UOpenGLRenderDevice::TexturePool.DeleteTexture(PoolIndex);
	}
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
	FTextureUploadState UploadState(&Info);

	FMemMark Mark(GMem);
	INT DataBlock = FTextureBlockBytes( Info.Format);
	INT DataSize = FTextureBytes( Info.Format, UL, VL);
	UploadState.Format = Info.Format;
	UploadState.USize = UL;
	UploadState.VSize = VL;
	UploadState.Data = new(GMem,DataSize,DataBlock) BYTE;

	{
		INT USize = Info.Mips[0]->USize;
		BYTE* Input  = Info.Mips[0]->DataPtr + (USize * V + U) * DataBlock;
		BYTE* Output = UploadState.Data;
		BYTE* OutputEnd = Output + DataSize;
		while ( Output < OutputEnd )
		{
			appMemcpy( Output, Input, UL * DataBlock);
			Input += USize * DataBlock;
			Output += UL * DataBlock;
		}
	}

	// Attempt conversion if needed
	if ( TextureFormatInfo[UploadState.Format].SoftwareConvert )
		(this->*TextureFormatInfo[UploadState.Format].SoftwareConvert)( UploadState, nullptr);

	// Upload sub image
	const FTextureFormatInfo& Fmt = TextureFormatInfo[UploadState.Format];
	if ( !Fmt.Compressed ) // Ignore compressed textures (lightmaps are never compressed)
	{
		if ( Texture.Target == GL_TEXTURE_2D_ARRAY )
			FOpenGLBase::glTexSubImage3D(Texture.Target, UploadState.Level, U, V, Remap.Layer, UL, VL, 1, Fmt.SourceFormat, Fmt.Type, UploadState.Data);
		else
			FOpenGLBase::glTexSubImage2D(Texture.Target, UploadState.Level, U, V, UL, VL, Fmt.SourceFormat, Fmt.Type, UploadState.Data);
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
		debugf( NAME_DevGraphics, TEXT("UploadTexture: invalid mip count: %i (CacheID=%016X)"), State.Info->NumMips, State.Info->CacheID);
		return false;
	}

	// Base mip selection
	INT BaseMip = GetBaseMip( *State.Info, FOpenGLBase::MaxTextureSize);
	if ( BaseMip >= State.Info->NumMips )
	{
		debugf( NAME_DevGraphics, TEXT("UploadTexture: oversized texture, BaseMip=%i, NumMips=%i, MaxTextureSize=%i (CacheID=%016X)"), BaseMip, State.Info->NumMips, FOpenGLBase::MaxTextureSize, State.Info->CacheID);
		return false;
	}
	INT AutoMaxLevel = 0;

	// Deferred load
	if ( SupportsLazyTextures )
		State.Info->Load();

	INT TopMip = BaseMip;
	for ( State.CurrentMip=BaseMip; State.CurrentMip<=TopMip; State.CurrentMip++)
	{
		FMipmapBase& Mip = *State.Info->Mips[State.CurrentMip];
		State.Format = State.Info->Format;
		State.Data   = Mip.DataPtr;
		State.USize  = Mip.USize; 
		State.VSize  = Mip.VSize;

		if ( TextureFormatInfo[State.Format].SoftwareConvert )
			(this->*TextureFormatInfo[State.Format].SoftwareConvert)( State, nullptr);

		const FTextureFormatInfo& Fmt = TextureFormatInfo[State.Format];
		if ( Fmt.Supported && State.Data )
		{
			// Higor:
			// 
			// Most of the important initialization must happen after the base MipMap
			// has been validated (optionally converted), this is necessary due to
			// how many of the parameters depend on the resulting format:
			// 
			// - Minimum (non-base) MipMap dimensions.
			// - MipMap count based on the above.
			// - Possible format changes.
			// - Texture channel swizzling.
			if ( State.Level == 0 )
			{
				FTextureRemap& Remap = *TexDrawList.TexRemaps[TexIndex];
				ValidatePooledTexture(State, Remap);

				// Ajust top mip so no unnecessary storage is done
				TopMip = GetTopMip( *State.Info, BaseMip);
				if ( (State.CurrentMip == TopMip) && AlwaysMipmap && CanAutogenerateMips(*State.Info) )
					AutoMaxLevel = CalcMaxLevel( *State.Info, State.CurrentMip, FTextureBlockWidth(State.Format) );

				if ( !State.Texture )
					AssignPooledTexture(State, Remap, Fmt, BaseMip != TopMip);

				if ( !State.Texture ) //!!! Relink to invalid texture
					return false;

				GL->TextureUnits.SetActive(TMU_Position);
				GL->TextureUnits.Bind(*State.Texture);

				// This is a new Texture, set default parameters
				if ( !State.Texture->Allocated )
				{
					FOpenGLBase::SetTextureFilters(*State.Texture, false);
					if ( State.Swizzle )
						GL->TextureUnits.SetSwizzle(State.Swizzle);

					GLsizei Mips = 1 + (AutoMaxLevel ? AutoMaxLevel : TopMip-State.CurrentMip);
					GLsizei Layers = SuggestLayerCount(State);
					FOpenGLBase::SetTextureStorage(*State.Texture, Fmt, State.USize, State.VSize, Layers, Mips);
//					if ( State.Info->Texture )
//						debugf(NAME_DevGraphics, TEXT("Creating %s %ix%i with %i mips and %i layers"), State.Info->Texture->GetName(), State.USize, State.VSize, Mips, Layers );
				}

				if ( !State.Texture->Allocated )
					break;
			}

			GLsizei Width  = State.USize;
			GLsizei Height = State.VSize;
			GLsizei TextureBytes = Fmt.GetTextureBytes(Width, Height);
			FOpenGLBase::SetTextureData(*State.Texture, Fmt, State.Level, Width, Height, State.Layer, State.Data, TextureBytes);
			State.Level++;
		}
		else
		{
			//Try next texture before failing
			//Try resizing if last mip

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
		debugf( NAME_DevGraphics, TEXT("UploadTexture: no data uploaded (Format=%s) (CacheID=%016X)"), *FTextureFormatString(State.Format), State.Info->NumMips, State.Info->CacheID);
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


TArray<BYTE> FTextureUploadState::Compose;

//
// Expands compose buffer to needed size.
//
bool FASTCALL FTextureUploadState::AllocateCompose( INT USize, INT VSize, BYTE Format)
{
	SQWORD NeededSize = FTextureBytes( Format, USize, VSize);
	// Compatible with TArray
	if ( (NeededSize > 0) && (NeededSize <= MAXINT) )
	{
		// We have the needed memory
		if ( Compose.GetData() && (Compose.Num() >= (INT)NeededSize) )
			return true;

		// Request memory
		Compose.SetSize( (INT)NeededSize );
		if ( Compose.GetData() )
			return true;
	}
//	appErrorf( TEXT("[GL EXT] Failed to allocate Compose memory %i (%i x %i)"), (INT)NeededSize, USize, VSize );
	return false;
}
#define CHECK_ALLOCATE_COMPOSE(usize,vsize,format) \
	if ( !FTextureUploadState::AllocateCompose( usize, vsize, format) ) \
	{ 	return false; }

//
// Checks if any color in palette has alpha transparency.
//
static bool PaletteHasAlpha( FColor* Palette)
{
	for ( INT i=0; i<NUM_PAL_COLORS; i++)
		if ( Palette[i].A != 255 )
			return true;
	return false;
}

//
// Checks if palette indices match RGBA values (TrueType font)
//
static bool PaletteGrayscaleFont( FColor* Palette)
{
	for ( DWORD i=0; i<NUM_PAL_COLORS; i++)
		if ( GET_COLOR_DWORD(Palette[i]) != ((i)|(i<<8)|(i<<16)|(i<<24)) )
			return false;
	return true;
}

//
// Convert a 7 bit color to a 8 bit color
//
static inline DWORD XYZA7_to_XYZA8( DWORD Src)
{
	DWORD ColorDWORD = (Src & 0x7F7F7F7F) << 1;
	ColorDWORD |= (ColorDWORD & 0x80808080) >> 7;
	return ColorDWORD;
}

//
// EXT_texture_shared_exponent
//
namespace GLSharedExponent
{
	// Specification
	constexpr INT B = 15; //Exponent bias
	constexpr INT N = 9; //Mantissa bits
	constexpr INT Emax = 30; //Maximum allowed biased exponent value
	constexpr INT SharedExpMax = (2^N-1)/2^N * 2^(Emax-B);

	// Conversion status
	static UBOOL Initialized = 0;
	static FLOAT RcpEP[Emax+1];
	static DWORD ColorMap[NUM_PAL_COLORS];

	static void Initialize()
	{
		Initialized = 1;
		for ( INT i=0; i<Emax+1; i++)
			RcpEP[i] = 1.0f / appPow(2,i-(B+N));
	}

	static DWORD PackColor( FLOAT* RGB_F)
	{
		INT RGB_I[3];
		FLOAT RGB_MAX = Max( RGB_F[0], RGB_F[1], RGB_F[2]) + 0.001;
		INT ExpShared = Max( -B-1, appFloor(log2(RGB_MAX))) + 1 + B; //Not refined!!!
		for ( INT i=0; i<3; i++)
			RGB_I[i] = appRound( RGB_F[i] * RcpEP[ExpShared]);

		return ((RGB_I[0] & 0b111111111)      )
			|  ((RGB_I[1] & 0b111111111) << 9 )
			|  ((RGB_I[2] & 0b111111111) << 18)
			|  ((ExpShared & 0b11111)    << 27);
	}

	static void GenerateColorMap( FColor* Palette)
	{
		FLOAT RGB_F[3];
		for ( INT i=0; i<NUM_PAL_COLORS; i++)
		{
			FColor& C = Palette[i];
			for ( INT j=0; j<3; j++)
			{
				BYTE& Component = (&C.R)[j];
				if ( Component == 127 || Component == 128 )
					RGB_F[j] = 127.5f / 255.f;
				else
					RGB_F[j] = (FLOAT)(INT)Component / 255.f; //Conversion to INT causes better codegen
			}
			ColorMap[i] = PackColor(RGB_F);
		}
	}
};


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
		const FTextureFormatInfo& Fmt = RenDev->TextureFormatInfo[Format];
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

/*-----------------------------------------------------------------------------
	Texture converters.

	Must be mip/level independent, as in all steps must return the same
	conversion path.
-----------------------------------------------------------------------------*/

bool FASTCALL UOpenGLRenderDevice::ConvertIdentity( FTextureUploadState& State, DWORD* QueryBytes)
{
	if ( QueryBytes )
	{
		FMipmapBase& Mip = *State.Info->Mips[State.CurrentMip];
		*QueryBytes = (DWORD)FTextureBytes( State.Format, Mip.USize, Mip.VSize, 1);
		return true;
	}
	return true;
}

bool FASTCALL UOpenGLRenderDevice::ConvertP8_RGBA8( FTextureUploadState& State, DWORD* QueryBytes)
{
	if ( QueryBytes )
	{
		FMipmapBase& Mip = *State.Info->Mips[State.CurrentMip];
		*QueryBytes = (DWORD)(Mip.USize * Mip.VSize) * sizeof(DWORD);
		return true;
	}

	// Converts to RGB9_E5 if this resembles a decal texture
	if ( FOpenGLBase::SupportsRGB9_E5
		&& !(State.PolyFlags & PF_Masked) )
	{
		// See if top left pixel in base mip is grey and check if there's no alpha channel
		FMipmapBase* BaseMip = State.Info->Mips[State.CurrentMip - State.Level];
		if ( BaseMip->DataPtr )
		{
			FColor TopLeft = State.Info->Palette[ *BaseMip->DataPtr ];
			if	(	(TopLeft.R == 127 || TopLeft.R == 128) 
				&&	(TopLeft.G == 127 || TopLeft.G == 128)
				&&	(TopLeft.B == 127 || TopLeft.B == 128)
				&&	!PaletteHasAlpha(State.Info->Palette) )
				return ConvertP8_RGB9_E5( State, QueryBytes);
		}
	}

	// Reinterpret as a R8 font with RGBA swizzle
	if ( FOpenGLBase::SupportsTextureSwizzle )
	{
		if ( PaletteGrayscaleFont(State.Info->Palette) )
		{
			State.Format = TEXF_R8;
			if ( State.Level == 0 )
				State.Swizzle = FOpenGLSwizzle( SWZ_RED, SWZ_RED, SWZ_RED, SWZ_RED);
			return true;
		}
	}

	FMipmapBase& Mip = *State.Info->Mips[State.CurrentMip];

	// Converts to RGBA8
	FColor FirstColor = State.Info->Palette[0];
	bool UseMaskedStorage = FGL::FCacheID(State.Info->CacheID).IsMaskedUTexture();
	if ( UseMaskedStorage )
		State.Info->Palette[0] = FColor(0,0,0,0);

	CHECK_ALLOCATE_COMPOSE( Mip.USize, Mip.VSize, TEXF_RGBA8_);
	BYTE* MapPtr = State.Data;
	BYTE* MapEnd = MapPtr + (PTRINT)(Mip.USize * Mip.VSize);

	State.Format   = TEXF_RGBA8_;
	State.Data     = &FTextureUploadState::Compose(0);
	DWORD* DestPtr = (DWORD*)State.Data;

	while ( MapPtr < MapEnd )
		*DestPtr++ = GET_COLOR_DWORD(State.Info->Palette[*MapPtr++]);

	if ( UseMaskedStorage )
		State.Info->Palette[0] = FirstColor;

	return true;
}

bool FASTCALL UOpenGLRenderDevice::ConvertP8_RGB9_E5( FTextureUploadState& State, DWORD* QueryBytes)
{
	if ( QueryBytes )
	{
		FMipmapBase& Mip = *State.Info->Mips[State.CurrentMip];
		*QueryBytes = (DWORD)(Mip.USize * Mip.VSize) * sizeof(DWORD);
		return true;
	}

	if ( !GLSharedExponent::Initialized )
		GLSharedExponent::Initialize();
	if ( State.Level == 0 )
		GLSharedExponent::GenerateColorMap(State.Info->Palette);

	FMipmapBase& Mip = *State.Info->Mips[State.CurrentMip];

	CHECK_ALLOCATE_COMPOSE( Mip.USize, Mip.VSize, TEXF_RGB9E5);
	BYTE* MapPtr = State.Data;
	BYTE* MapEnd = MapPtr + (PTRINT)(Mip.USize * Mip.VSize);

	State.Format = TEXF_RGB9E5;
	State.Data   = &FTextureUploadState::Compose(0);
	DWORD* DestPtr    = (DWORD*)State.Data;

	while ( MapPtr < MapEnd )
		*DestPtr++ = GLSharedExponent::ColorMap[*MapPtr++];

	return true;

}

bool FASTCALL UOpenGLRenderDevice::ConvertBGRA7_BGRA8( FTextureUploadState& State, DWORD* QueryBytes)
{
	if ( QueryBytes )
	{
		FMipmapBase& Mip = *State.Info->Mips[State.CurrentMip];
		*QueryBytes = (DWORD)(Mip.USize * Mip.VSize) * sizeof(DWORD);
		return true;
	}

	// Light and fog maps don't need software RGB scaling.
	BYTE CacheType = State.Info->CacheID & 0xFF;
	if ( CacheType == CID_StaticMap || CacheType == CID_DynamicMap || CacheType == CID_RenderFogMap )
	{
		State.Format = TEXF_BGRA8;
		return true;
	}

	FMipmapBase& Mip = *State.Info->Mips[State.CurrentMip];

	CHECK_ALLOCATE_COMPOSE( Mip.USize, Mip.VSize, TEXF_BGRA8);
	DWORD* SourcePtr = (DWORD*)State.Data;
	DWORD* SourceEnd = SourcePtr + (PTRINT)(Mip.USize*Mip.VSize);

	State.Format = TEXF_BGRA8;
	State.Data   = &FTextureUploadState::Compose(0);
	DWORD* DestPtr = (DWORD*)State.Data;

	while ( SourcePtr < SourceEnd )
		*DestPtr++ = XYZA7_to_XYZA8(*SourcePtr++);

	return true;
}
