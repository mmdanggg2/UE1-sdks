/*=============================================================================
	OpenGL_DrawBuffer.h

	Draw buffering prototypes used by different render paths.

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/


static FORCEINLINE void SetPendingTextures( const FGL::Draw::TextureList& TexDrawList, FPendingTexture* PendingTextures, INT ArraySize)
{
	for ( INT i=0; i<ArraySize; i++)
	{
		if ( TexDrawList.TexRemaps[i] )
		{
			PendingTextures[i].PoolID = TexDrawList.TexRemaps[i]->PoolIndex;
			PendingTextures[i].RelevantPolyFlags = TexDrawList.PolyFlags[i];
			PendingTextures[i].ViewIndex = 0;
		}
		else
			PendingTextures[i].PoolID = INDEX_NONE;
	}
}

template<INT ArraySize> static FORCEINLINE void SetPendingTextures( const FGL::Draw::TextureList& TexDrawList, FPendingTexture (&PendingTextures)[ArraySize])
{
	SetPendingTextures(TexDrawList, PendingTextures, ArraySize);
}


namespace FGL
{

namespace VertexBuffer
{

//
// Common structure of a Vertex buffer
//
struct Base
{
	DWORD Position   {0};
	DWORD Bytes      {0};
	BYTE* Data       {nullptr};

	Base(DWORD InBytes=0)
	{}

	virtual ~Base()
	{}

	void Finish()
	{
		Position = 0;
	}
};

//
// Client memory
// Used for sourcing vertex memory from ram or for composing other larger mapped buffers
//
struct Client : public Base
{
	Client( DWORD InBytes)
	{
		Bytes = InBytes;
		Data = (BYTE*)appMalloc(InBytes, TEXT("CreateArrayBuffer"));
	}

	~Client()
	{
		if ( Data )
		{
			appFree(Data);
			Data = nullptr;
		}
	}
};

//
// Single VBO fill via glBufferSubData
// Discard allocated memory by using glBufferData -> nullptr
//
struct CopyToVBO : public Client
{
	GLuint VBO {0};

	CopyToVBO( DWORD InBytes)
		: Client(InBytes)
	{
		FOpenGLBase::CreateBuffer(VBO, GL_ARRAY_BUFFER, InBytes, nullptr, GL_STREAM_DRAW);
	}

	~CopyToVBO()
	{
		FOpenGLBase::DeleteBuffer(VBO);
	}

	void Update()
	{
		if ( Position > 0 )
		{
			FOpenGLBase::glBindBuffer(GL_ARRAY_BUFFER, VBO);
			FOpenGLBase::glBufferData(GL_ARRAY_BUFFER, Bytes, nullptr, GL_STREAM_DRAW);//!!
			FOpenGLBase::glBufferSubData(GL_ARRAY_BUFFER, 0, Position, Data);
		}
	}

	void Finish()
	{
		if ( Position > 0 )
		{
//			FOpenGLBase::glBindBuffer(GL_ARRAY_BUFFER, VBO);
//			FOpenGLBase::glBufferData(GL_ARRAY_BUFFER, Bytes, nullptr, GL_STREAM_DRAW);
			Base::Finish();
		}
	}
};

};


namespace DrawBuffer
{

//
// ARB:
// - Packs TexturePan with Texture0
// 
// GL3:
// - LightInfo contains layer indices for ZoneId, LightMap layer and FogMap layer
//
enum EBufferType
{
	BT_Vertex        = 1 <<  0, // Vertex is always present
	BT_Color0        = 1 <<  1,
	BT_Color1        = 1 <<  2,
	BT_Texture0      = 1 <<  3,
	BT_Texture1      = 1 <<  4,
	BT_Texture2      = 1 <<  5,
	BT_Normal        = 1 <<  6,
	BT_TextureLayers = 1 <<  7,
	BT_LightInfo     = 1 <<  8, // ZoneId, LightMap layer, FogMap layer

	// Vertex shader parameters (will be aliased)
	BT_VertexParam0  = 1 <<  9,
	BT_VertexParam1  = 1 << 10,

	BT_QuadXYUV      = BT_VertexParam0, // Tile (GL3)
	BT_TexturePan    = BT_VertexParam0, // ComplexSurface (GL3)
	BT_UBOIndices    = BT_VertexParam1, // ComplexSurface, Gouraud (GL3)

	BT_TextureAny    = BT_Texture0 | BT_Texture1 | BT_Texture2,
	BT_TypesARB      = BT_Vertex | BT_Color0 | BT_Color1 | BT_TextureAny | BT_Normal,

	BT_SpecialFlag   = 1 << 14,
	BT_StaticBuffer  = 1 << 15, // Uses a static VBO which provides data layout.
};


//
// Base prototypes
//
struct FBase
{
	DWORD               ActiveAttribs           {0};
	DWORD               PolyFlags               {0};
	FProgramID          ProgramID               {0};
	DWORD               BufferStride            {0};
	VertexBuffer::Base* Buffer                  {nullptr};

	TNumericTag<DWORD> LocalIndex;
	static TNumericTag<DWORD> GlobalIndex;

	FBase()
	{
		LocalIndex.GetValue() = GlobalIndex.GetValue() & 0xFFF; //Only use 12 bits
		GlobalIndex.Increment();
	}

	INT AdjustToStride()
	{
		DWORD i = (Buffer->Position + BufferStride - 1) / BufferStride;
		Buffer->Position = i * BufferStride;
		return (INT)i;
	}
};

template <typename T, typename BufferType, DWORD BufferSize> struct TBase : public FBase
{
	static constexpr DWORD BufferTypes = BT_Vertex;
	static constexpr DWORD ForceAttribs = BT_Vertex;
	typedef FGL::FVertex3D        VertexType;
	typedef FGL::FVertex3D        NormalType;
	typedef FGL::FColorRGBA       Color0Type;
	typedef FGL::FColorRGBA       Color1Type;
	typedef FGL::FTextureUV       Texture0Type;
	typedef FGL::FTextureUV       Texture1Type;
	typedef FGL::FTextureUV       Texture2Type;
	typedef FGL::uint8x4          TextureLayersType;
	typedef FGL::uint8x4          LightInfoType;
	typedef FGL::FTexturePan      VertexParam0Type;
	typedef FGL::uint16x4         VertexParam1Type;
	typedef FGL::VBO::FBase VBOType;

	TBase()
	{
		if ( BufferSize > 0 )
			Buffer = new BufferType(BufferSize);
	}

	~TBase()
	{
		if ( (BufferSize > 0) && Buffer )
		{
			delete Buffer;
			Buffer = nullptr;
		}
	}

	bool AtCapacity( INT InVertices)           { return Buffer->Position + BufferStride * InVertices > Buffer->Bytes; }
	DWORD GetBufferedVerts()                   { return Buffer->Position / BufferStride; }
	DWORD GetUnusedVerts()                     { return (Buffer->Bytes - Buffer->Position) / BufferStride; }

	BufferType* GetBuffer()                    { return (BufferType*)Buffer; }

	inline void StreamStart( void*& Stream)    { Stream = (void*)&Buffer->Data[Buffer->Position]; }
	inline void StreamEnd( void* Stream)       { Buffer->Position = ((BYTE*)Stream) - &Buffer->Data[0]; }

	DWORD GetAttribs( DWORD InPolyFlags, const FGL::Draw::TextureList& TexDrawList)
	{
	#define HAS_ATTRIB(bt,condition) ( (static_cast<T*>(this)->ForceAttribs & bt) || ((static_cast<T*>(this)->BufferTypes & bt) && (condition)) )

		DWORD Attribs = 0;
		if ( HAS_ATTRIB(BT_Vertex,       true) )                                Attribs |= BT_Vertex;
		if ( HAS_ATTRIB(BT_Color0,       InPolyFlags & PF_Gouraud) ) {          Attribs |= BT_Color0;
			if ( HAS_ATTRIB(BT_Color1,   InPolyFlags & PF_RenderFog) )          Attribs |= BT_Color1;
		}
		else if ( HAS_ATTRIB(BT_Color0,  InPolyFlags & PF_FlatShaded) )         Attribs |= BT_Color0; /*==PF_RenderFog*/
		if ( HAS_ATTRIB(BT_Texture0,     TexDrawList.Infos[0]) )                Attribs |= BT_Texture0;
		if ( HAS_ATTRIB(BT_Texture1,     TexDrawList.Infos[3]) )                Attribs |= BT_Texture1;
		if ( HAS_ATTRIB(BT_Texture2,     TexDrawList.Infos[4]) )                Attribs |= BT_Texture2;
		if ( HAS_ATTRIB(BT_Normal,       true) )                                Attribs |= BT_Normal;
		if ( HAS_ATTRIB(BT_TextureLayers,Attribs & BT_Texture0) )               Attribs |= BT_TextureLayers;
		if ( HAS_ATTRIB(BT_LightInfo,    Attribs & (BT_Texture1|BT_Texture2)) ) Attribs |= BT_LightInfo;
		if ( HAS_ATTRIB(BT_VertexParam0, true) )                                Attribs |= BT_VertexParam0;
		if ( HAS_ATTRIB(BT_VertexParam1, true) )                                Attribs |= BT_VertexParam1;
		return Attribs;
	#undef HAS_ATTRIB
	}

	DWORD GetAttribs( const FGL::Draw::Command* Draw)
	{
	#define HAS_ATTRIB(bt,condition) ( (static_cast<T*>(this)->ForceAttribs & bt) || ((static_cast<T*>(this)->BufferTypes & bt) && (condition)) )

		DWORD Attribs = 0;
		if ( HAS_ATTRIB(BT_Vertex,       true) )                                Attribs |= BT_Vertex;
		if ( HAS_ATTRIB(BT_Color0,       Draw->PolyFlags & PF_Gouraud) ) {      Attribs |= BT_Color0;
			if ( HAS_ATTRIB(BT_Color1,   Draw->PolyFlags & PF_RenderFog) )      Attribs |= BT_Color1;
		}
		else if ( HAS_ATTRIB(BT_Color0,  Draw->PolyFlags & PF_FlatShaded) )     Attribs |= BT_Color0; /*==PF_RenderFog*/
		if ( HAS_ATTRIB(BT_Texture0,     Draw->CacheID_Base) )                  Attribs |= BT_Texture0;
		if ( HAS_ATTRIB(BT_Texture1,     Draw->CacheID_Light) )                 Attribs |= BT_Texture1;
		if ( HAS_ATTRIB(BT_Texture2,     Draw->CacheID_Fog) )                   Attribs |= BT_Texture2;
		if ( HAS_ATTRIB(BT_Normal,       true) )                                Attribs |= BT_Normal;
		if ( HAS_ATTRIB(BT_TextureLayers,Attribs & BT_Texture0) )             Attribs |= BT_TextureLayers;
		if ( HAS_ATTRIB(BT_LightInfo,    Attribs & (BT_Texture1|BT_Texture2)) ) Attribs |= BT_LightInfo;
		if ( HAS_ATTRIB(BT_VertexParam0,  true) )                               Attribs |= BT_VertexParam0;
		if ( HAS_ATTRIB(BT_VertexParam1,  true) )                               Attribs |= BT_VertexParam1;
		return Attribs;
	#undef HAS_ATTRIB
	}

	static DWORD GetStaticAttribs()
	{
		DWORD StaticAttribs = 0;
		if ( T::VBOType::HasStaticVertexData() )        StaticAttribs |= BT_Vertex;
		if ( T::VBOType::HasStaticColor0Data() )        StaticAttribs |= BT_Color0;
		if ( T::VBOType::HasStaticColor1Data() )        StaticAttribs |= BT_Color1;
		if ( T::VBOType::HasStaticTexture0Data() )      StaticAttribs |= BT_Texture0;
		if ( T::VBOType::HasStaticTexture1Data() )      StaticAttribs |= BT_Texture1;
		if ( T::VBOType::HasStaticTexture2Data() )      StaticAttribs |= BT_Texture2;
		if ( T::VBOType::HasStaticNormalData() )        StaticAttribs |= BT_Normal;
		if ( T::VBOType::HasStaticVertexParam0Data() )  StaticAttribs |= BT_VertexParam0;
		if ( T::VBOType::HasStaticVertexParam1Data() )  StaticAttribs |= BT_VertexParam1;
		return StaticAttribs;
	}

	DWORD GetStride( DWORD Attribs)
	{
		DWORD Stride = 0;
	#define HAS_STATIC_ATTRIB(bt) (static_cast<T*>(this)->ForceAttribs & bt)
		if ( HAS_STATIC_ATTRIB(BT_Vertex) )       Stride += sizeof(typename T::VertexType);
		if ( HAS_STATIC_ATTRIB(BT_Color0) )       Stride += sizeof(typename T::Color0Type);
		if ( HAS_STATIC_ATTRIB(BT_Color1) )       Stride += sizeof(typename T::Color1Type);
		if ( HAS_STATIC_ATTRIB(BT_Texture0) )     Stride += sizeof(typename T::Texture0Type);
		if ( HAS_STATIC_ATTRIB(BT_Texture1) )     Stride += sizeof(typename T::Texture1Type);
		if ( HAS_STATIC_ATTRIB(BT_Texture2) )     Stride += sizeof(typename T::Texture2Type);
		if ( HAS_STATIC_ATTRIB(BT_Normal) )       Stride += sizeof(typename T::NormalType);
		if ( HAS_STATIC_ATTRIB(BT_TextureLayers) )Stride += sizeof(typename T::TextureLayersType);
		if ( HAS_STATIC_ATTRIB(BT_LightInfo) )    Stride += sizeof(typename T::LightInfoType);
		if ( HAS_STATIC_ATTRIB(BT_VertexParam0) ) Stride += sizeof(typename T::VertexParam0Type);
		if ( HAS_STATIC_ATTRIB(BT_VertexParam1) ) Stride += sizeof(typename T::VertexParam1Type);
	#define HAS_DYNAMIC_ATTRIB(bt) ( !HAS_STATIC_ATTRIB(bt) && ((static_cast<T*>(this)->BufferTypes & bt) & Attribs) )
		if ( HAS_DYNAMIC_ATTRIB(BT_Vertex) )       Stride += sizeof(typename T::VertexType);
		if ( HAS_DYNAMIC_ATTRIB(BT_Color0) )       Stride += sizeof(typename T::Color0Type);
		if ( HAS_DYNAMIC_ATTRIB(BT_Color1) )       Stride += sizeof(typename T::Color1Type);
		if ( HAS_DYNAMIC_ATTRIB(BT_Texture0) )     Stride += sizeof(typename T::Texture0Type);
		if ( HAS_DYNAMIC_ATTRIB(BT_Texture1) )     Stride += sizeof(typename T::Texture1Type);
		if ( HAS_DYNAMIC_ATTRIB(BT_Texture2) )     Stride += sizeof(typename T::Texture2Type);
		if ( HAS_DYNAMIC_ATTRIB(BT_Normal) )       Stride += sizeof(typename T::NormalType);
		if ( HAS_DYNAMIC_ATTRIB(BT_TextureLayers) ) Stride += sizeof(typename T::TextureLayersType);
		if ( HAS_DYNAMIC_ATTRIB(BT_LightInfo) )    Stride += sizeof(typename T::LightInfoType);
		if ( HAS_DYNAMIC_ATTRIB(BT_VertexParam0) ) Stride += sizeof(typename T::VertexParam0Type);
		if ( HAS_DYNAMIC_ATTRIB(BT_VertexParam1) ) Stride += sizeof(typename T::VertexParam1Type);
	#undef HAS_STATIC_ATTRIB
	#undef HAS_DYNAMIC_ATTRIB
		return Stride;
	}

	static constexpr DWORD GetConstStride()
	{
		return 0 +
	#define HAS_STATIC_ATTRIB(bt) ((T::ForceAttribs & bt) != 0)
		(HAS_STATIC_ATTRIB(BT_Vertex)         ? sizeof(typename T::VertexType)       : 0) +
		(HAS_STATIC_ATTRIB(BT_Color0)         ? sizeof(typename T::Color0Type)       : 0) +
		(HAS_STATIC_ATTRIB(BT_Color1)         ? sizeof(typename T::Color1Type)       : 0) +
		(HAS_STATIC_ATTRIB(BT_Texture0)       ? sizeof(typename T::Texture0Type)     : 0) +
		(HAS_STATIC_ATTRIB(BT_Texture1)       ? sizeof(typename T::Texture1Type)     : 0) +
		(HAS_STATIC_ATTRIB(BT_Texture2)       ? sizeof(typename T::Texture2Type)     : 0) +
		(HAS_STATIC_ATTRIB(BT_Normal)         ? sizeof(typename T::NormalType)       : 0) +
		(HAS_STATIC_ATTRIB(BT_TextureLayers)  ? sizeof(typename T::TextureLayersType): 0) +
		(HAS_STATIC_ATTRIB(BT_LightInfo)      ? sizeof(typename T::LightInfoType)    : 0) +
		(HAS_STATIC_ATTRIB(BT_VertexParam0)   ? sizeof(typename T::VertexParam0Type) : 0) +
		(HAS_STATIC_ATTRIB(BT_VertexParam1)   ? sizeof(typename T::VertexParam1Type) : 0);
	#undef HAS_STATIC_ATTRIB
	}


	void SetupLine( DWORD InLineFlags)
	{
		PolyFlags      = (InLineFlags & LINE_DepthCued) ? (PF_Highlighted|PF_Occlude|PF_Gouraud) : (PF_Highlighted|PF_NoZReject|PF_Gouraud);

		// Compose Attribs
		DWORD Attribs = BT_Vertex | BT_Color0;
		ActiveAttribs = Attribs;

		// Compose Stride
		DWORD Stride = 0;
		Stride += sizeof(typename T::VertexType);
		Stride += sizeof(typename T::Color0Type);
		BufferStride = Stride;
	}

	void Setup( DWORD InPolyFlags, const FGL::Draw::TextureList& TexDrawList)
	{
		DWORD Attribs       = static_cast<T*>(this)->GetAttribs(InPolyFlags, TexDrawList);
		DWORD StaticAttribs = static_cast<T*>(this)->GetStaticAttribs();

		ActiveAttribs    = Attribs | StaticAttribs;
		BufferStride     = static_cast<T*>(this)->GetStride(Attribs);
		PolyFlags        = InPolyFlags;

		// Set pending textures
		SetPendingTextures( TexDrawList, static_cast<T*>(this)->Textures);
	}

	void Setup( FGL::Draw::Command* Draw, DWORD ForceAttribs=0)
	{
		DWORD Attribs       = static_cast<T*>(this)->GetAttribs(Draw) | ForceAttribs;
		DWORD StaticAttribs = static_cast<T*>(this)->GetStaticAttribs();
		ActiveAttribs    = Attribs | StaticAttribs;
		Draw->Attribs    = Attribs | StaticAttribs;

		DWORD Stride = static_cast<T*>(this)->GetStride(Attribs);
		BufferStride = Stride;
		Draw->Stride = Stride;
	
		PolyFlags = Draw->PolyFlags; // Needed?

		// TODO: Set pending textures on Draw
	}
};

template <typename T, typename BufferType, DWORD BufferSize> struct TComplexBase : public TBase<T,BufferType,BufferSize>
{
	TComplexBase()
		: PendingVolumetrics(0)
		, Polys(0)
	{}

	UBOOL       PendingVolumetrics;

	INT         Polys;
	TArray<INT> PolyVertsStart;
	TArray<INT> PolyVertsCount;
};


//
// ARB buffers
//
struct FComplexARB : public TComplexBase<FComplexARB, FGL::VertexBuffer::Client ,256*1024>
{
	static constexpr DWORD BufferTypes = BT_Vertex|BT_Color0|BT_Texture0|BT_Texture1|BT_Texture2;
	typedef FGL::FTextureUVPan Texture0Type;

	DWORD              ZoneId;
	FPendingTexture    Textures[TMU_FogMap+1];
	FGL::FTextureScale TextureScale[TMU_MacroTexture+1];
};

struct FComplexStaticARB : public TComplexBase<FComplexStaticARB, FGL::VertexBuffer::Base ,0>
{
	static constexpr DWORD BufferTypes  = BT_StaticBuffer;
	static constexpr DWORD ForceAttribs = 0;
	typedef FGL::VBO::FStaticBsp VBOType;

	DWORD              ZoneId;
	FPendingTexture    Textures[TMU_FogMap+1];
	FGL::FTextureScale TextureScale[TMU_MacroTexture+1];
	BYTE               LightUVAtlas;
	BYTE               FogUVAtlas;
};

struct FGouraudARB : public TBase<FGouraudARB, FGL::VertexBuffer::Client, 256*1024>
{
	static constexpr DWORD BufferTypes  = BT_Vertex|BT_Color0|BT_Color1|BT_Texture0;

	FPendingTexture    Textures[TMU_DetailTexture+1];
	FGL::FTextureScale TextureScale[TMU_DetailTexture+1];
};

struct FQuadARB : public TBase<FQuadARB, FGL::VertexBuffer::Client ,64*1024>
{
	static constexpr DWORD BufferTypes = BT_Vertex|BT_Color0|BT_Texture0;

	FPendingTexture  Textures[TMU_Base+1];
};

struct FLineARB : public TBase<FLineARB, FGL::VertexBuffer::Client ,64*1024>
{
	static constexpr DWORD BufferTypes  = BT_Vertex|BT_Color0;
	static constexpr DWORD ForceAttribs = BT_Vertex|BT_Color0;

	DWORD LineFlags;
};

static constexpr DWORD FillBufferSizeARB = (sizeof(FGL::FVertex3D) + sizeof(FGL::FColorRGBA) + sizeof(FGL::FTextureUV)) * 4;
struct FFillARB : public TBase<FFillARB, FGL::VertexBuffer::Client, FillBufferSizeARB>
{
	static constexpr DWORD BufferTypes = BT_Vertex|BT_Color0|BT_Texture0;
};

static constexpr DWORD DecalBufferSizeARB = (sizeof(FGL::FVertex3D) + sizeof(FGL::FColorRGBA) + sizeof(FGL::FTextureUV)) * 2048;
struct FDecalARB : public TBase<FDecalARB, FGL::VertexBuffer::Client, DecalBufferSizeARB>
{
	static constexpr DWORD BufferTypes  = BT_Vertex|BT_Color0|BT_Texture0;
	static constexpr DWORD ForceAttribs = BT_Vertex|BT_Color0|BT_Texture0;

	FPendingTexture    Textures[TMU_Base+1];
	FGL::FTextureScale TextureScale[TMU_Base+1];
};



//
// GLSL3 buffers
//
struct FComplexGLSL3 : public TComplexBase<FComplexGLSL3, FGL::VertexBuffer::CopyToVBO, 0>
{
	static constexpr DWORD BufferTypes = BT_Vertex|BT_Color0|BT_Texture0|BT_Texture1|BT_Texture2|BT_TextureLayers|BT_LightInfo|BT_TexturePan|BT_UBOIndices;

	typedef FGL::FTexturePan      VertexParam0Type;
	typedef FGL::uint16x2         VertexParam1Type;
};

struct FGouraudGLSL3 : public TBase<FGouraudGLSL3, FGL::VertexBuffer::CopyToVBO, 0>
{
	static constexpr DWORD BufferTypes  = BT_Vertex|BT_Color0|BT_Color1|BT_Texture0|BT_TextureLayers|BT_UBOIndices;
	static constexpr DWORD ForceAttribs = BT_Vertex|BT_Color0|BT_Texture0|BT_TextureLayers|BT_UBOIndices;

	typedef FGL::uint16x2         VertexParam1Type;
};

struct FQuadGLSL3 : public TBase<FQuadGLSL3, FGL::VertexBuffer::CopyToVBO, 0>
{
	static constexpr DWORD BufferTypes  = BT_Vertex|BT_Color0|BT_Texture0|BT_TextureLayers /* TODO: GEOMETRY EXPANSION*/;
	static constexpr DWORD ForceAttribs = BT_Vertex|BT_Color0|BT_Texture0|BT_TextureLayers;
};

struct FLineGLSL3 : public TBase<FLineGLSL3, FGL::VertexBuffer::CopyToVBO, 0>
{
	static constexpr DWORD BufferTypes  = BT_Vertex|BT_Color0;
	static constexpr DWORD ForceAttribs = BT_Vertex|BT_Color0;
};

struct FFillGLSL3 : public TBase<FFillGLSL3, FGL::VertexBuffer::CopyToVBO, 0>
{
	static constexpr DWORD BufferTypes  = BT_StaticBuffer;
	static constexpr DWORD ForceAttribs = 0;

	typedef FGL::VBO::FFill VBOType;
};

struct FDecalGLSL3 : public TBase<FDecalGLSL3, FGL::VertexBuffer::CopyToVBO, 4 * 1024 * 1024>
{
	static constexpr DWORD BufferTypes  = BT_Vertex|BT_Color0|BT_Texture0|BT_TextureLayers|BT_UBOIndices;
	static constexpr DWORD ForceAttribs = BT_Vertex|BT_Color0|BT_Texture0|BT_TextureLayers|BT_UBOIndices;

	typedef FGL::uint16x2         VertexParam1Type;
};


};
};

