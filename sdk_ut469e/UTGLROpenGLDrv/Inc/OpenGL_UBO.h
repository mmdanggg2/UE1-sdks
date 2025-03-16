/*=============================================================================
	OpenGL_UBO.h: Structures used in GLSL uniforms.

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

static_assert(!(sizeof(glm::mat4) & 15), "Bad uniform element struct size");
static_assert(!(sizeof(FPlane) & 15), "Bad uniform element struct size");

//
// Template padding
//
template <class T> class TPad16 : public T
{
	BYTE _Pad[ 16 - sizeof(T)&15 ];

public:
	TPad16<T>()
	{}

	TPad16<T>( const T& Other)
		: T(Other)
	{}
};

//
// SetSceneNode parameters.
//
struct FGlobalRender_UBO
{
	static constexpr GLint UniformIndex = 0;

	glm::mat4 ProjectionMatrix; // Transform 3D vertex to View Space
	glm::mat4 ModelViewMatrix;
	FPlane    TileVector;       // DrawTile transformation parameters
	FPlane    WorldCoordsOrigin;// VBO transform, viewer origin
	FPlane    WorldCoordsXAxis; // VBO transform, X axis
	FPlane    WorldCoordsYAxis; // VBO transform, Y axis
	FPlane    WorldCoordsZAxis; // VBO transform, Z axis
	FPlane    ColorCorrection;  // Transform fragment color using these parameters.
	FLOAT     LightMapFactor;   // Blend factor for lightmaps (2 if OneXBlending, 4 otherwise)
	FLOAT     Padding0[3];

	static const ANSICHAR* GLSL_Struct();
	static constexpr GLsizeiptr GLSL_Size();
	void BufferAll();
};

//
// Static BSP parameters.
//
struct FStaticBsp_UBO
{
	static constexpr GLint UniformIndex = 1;

	FGL::FTexturePan WavyPan;
	FLOAT            TimeSeconds;
	union { struct { BYTE UpdateWavy, UpdateAmbient, UpdateAutoPan; }; DWORD UpdateFlags; };

	FColor           ZoneAmbientColor[64]; // Packed color, used for early testing
	FPlane           ZoneAmbientPlane[64]; // Vector color, used in shader
	TPad16<FGL::FTexturePan> ZoneAutoPan[64];      // UV pan offset for surfaces with auto-pan flags

	// Non buffered stuff
	INT NodeCount;
	INT VertexCount;

	static const ANSICHAR* GLSL_Struct();
	static constexpr GLsizeiptr GLSL_Size();
	void BufferWavyTime();
	void BufferAmbientPlaneArray();
	void BufferAutoPanArray();
};

//
// Texture parameters
// Note: this is a variable sized buffer.
//
struct FTextureParams_UBO
{
	static constexpr GLint UniformIndex = 2;

	struct FElement
	{
		FLOAT              Scale;
		FLOAT              _unused[2];
		FLOAT              BindlessID;
	};
	static_assert( sizeof(FElement)==16, "FTextureParams_UBO::FElement must be 16 bytes");

	UBOOL            PendingUpdate;
	TArray<FElement> Elements;

	static const ANSICHAR* GLSL_Struct();
	void BufferSingleElement( INT ElementIndex);
	void BufferElements();
};

//
// Collects sets of inmutable scale values for Complex Surface drawing
//
struct FComplexSurfaceScale_UBO
{
	typedef FGL::FcsTextureScale FElement;
	static_assert( sizeof(FElement)==16, "FComplexSurfaceScale_UBO::FElement must be 16 bytes");

	static constexpr GLint UniformIndex = 3;

	UBOOL            PendingUpdate;
	TArray<FElement> Elements;

	static const ANSICHAR* GLSL_Struct();
	static INT ArraySize();

	void BufferElements();
};

//
// Used to store lightmap parameters into a Buffer Texture
//
struct FLightMapParams_TBO
{
	FOpenGLBuffer Buffer;

	struct StaticData
	{
		FGL::FTextureUVPan Regular[65536];
		FGL::FTextureUVPan Atlased[65536];
	};
	static_assert( sizeof(StaticData)==2*1024*1024, "FLightMapParams_TBO::StaticData must be 2 megabytes");

	void* ClientBuffer;

	void Create();
	void Destroy();
};

/*-----------------------------------------------------------------------------
	Data descriptors.
-----------------------------------------------------------------------------*/

//
// Default GLSL parameters
//
template < typename T > struct TBufferInfoUBOBase
{
	static constexpr GLsizeiptr SourceSize() { return sizeof(T); }
	static constexpr GLsizeiptr BufferSize();
	
	static void BufferData( const T& Data, GLintptr Offset);
	template <GLint ArrayNum> static void BufferDataArray( const T* Data, GLintptr Offset);
};
template < typename T > struct TBufferInfoUBO : public TBufferInfoUBOBase<T> {};

template<typename T>
inline constexpr GLsizeiptr TBufferInfoUBOBase<T>::BufferSize()
{
	return (TBufferInfoUBO<T>::SourceSize() + 15) & (~15);
}

template<typename T>
inline void TBufferInfoUBOBase<T>::BufferData( const T& Data, GLintptr Offset)
{
	FOpenGLBase::glBufferSubData(GL_UNIFORM_BUFFER, Offset, TBufferInfoUBO<T>::SourceSize(), &Data);
}

template<typename T>
template <GLint ArrayNum>
inline void TBufferInfoUBOBase<T>::BufferDataArray( const T* Data, GLintptr Offset)
{
	if ( TBufferInfoUBO<T>::SourceSize() == TBufferInfoUBO<T>::BufferSize() )
		FOpenGLBase::glBufferSubData(GL_UNIFORM_BUFFER, Offset, TBufferInfoUBO<T>::SourceSize()*ArrayNum, &Data[0]);
	else
		for ( GLint i=0; i<ArrayNum; i++)
			BufferData( Data[i], Offset + TBufferInfoUBO<T>::BufferSize() * i);
}

//
// FCoords
// In 64 bit builds it can buffer the Coords with a single call due to FVector alignment
//
template <> struct TBufferInfoUBO<FCoords> : public TBufferInfoUBOBase<FCoords>
{
	static constexpr GLsizeiptr BufferSize() { return TBufferInfoUBO<FVector>::BufferSize() * 4; }

	static void BufferData( const FCoords& Data, GLintptr Offset)
	{
		TBufferInfoUBO<FVector>::BufferDataArray<4>( &Data.Origin, Offset);
	}
};


/*-----------------------------------------------------------------------------
	FGlobalRender_UBO.
-----------------------------------------------------------------------------*/

inline const ANSICHAR* FGlobalRender_UBO::GLSL_Struct()
{
	return
R"(layout(std140) uniform GlobalRender
{
    mat4 ProjectionMatrix;
    mat4 ModelViewMatrix;
    vec4 TileVector;
    vec3 WorldCoordsOrigin;
    vec3 WorldCoordsXAxis;
    vec3 WorldCoordsYAxis;
    vec3 WorldCoordsZAxis;
    vec4 ColorCorrection;
    float LightMapFactor;
};
)";
}

inline constexpr GLsizeiptr FGlobalRender_UBO::GLSL_Size()
{
	return sizeof(FGlobalRender_UBO);
}

inline void FGlobalRender_UBO::BufferAll()
{
	FOpenGLBase::glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(FGlobalRender_UBO), this);
}

/*-----------------------------------------------------------------------------
	FStaticBsp_UBO.
-----------------------------------------------------------------------------*/

inline const ANSICHAR* FStaticBsp_UBO::GLSL_Struct()
{
	return
R"(layout(std140) uniform StaticBsp
{
    vec3 WavyTime;
    vec4 ZoneAmbientPlane[64];
    vec2 ZoneAutoPan[64];
};
)";
}

inline constexpr GLsizeiptr FStaticBsp_UBO::GLSL_Size()
{
	return 16
		+ TBufferInfoUBO<FPlane>::BufferSize() * 64
		+ TBufferInfoUBO<FGL::FTexturePan>::BufferSize() * 64;
}

inline void FStaticBsp_UBO::BufferWavyTime()
{
	static constexpr GLintptr Offset = 0;

	FOpenGLBase::glBufferSubData(GL_UNIFORM_BUFFER, Offset, sizeof(FGL::FTexturePan) + sizeof(FLOAT), &WavyPan);
}

inline void FStaticBsp_UBO::BufferAmbientPlaneArray()
{
	static constexpr GLintptr Offset = 0
		+ 16;

	TBufferInfoUBO<FPlane>::BufferDataArray<64>( ZoneAmbientPlane, Offset);
}

inline void FStaticBsp_UBO::BufferAutoPanArray()
{
	static constexpr GLintptr Offset = 0
		+ 16
		+ TBufferInfoUBO<FPlane>::BufferSize() * 64;

	TBufferInfoUBO<TPad16<FGL::FTexturePan>>::BufferDataArray<64>( ZoneAutoPan, Offset);
}


/*-----------------------------------------------------------------------------
	FTextureParams_UBO.
-----------------------------------------------------------------------------*/

inline const ANSICHAR* FTextureParams_UBO::GLSL_Struct()
{
	return 
R"(layout(std140) uniform TextureParams
{
    vec4 TexInfo[TEXINFOMAX];
};
)";
}

inline void FTextureParams_UBO::BufferSingleElement( INT ElementIndex)
{
	if ( Elements.IsValidIndex(ElementIndex) )
		FOpenGLBase::glBufferSubData(GL_UNIFORM_BUFFER, sizeof(FTextureParams_UBO::FElement)*ElementIndex, sizeof(FTextureParams_UBO::FElement), &Elements(ElementIndex));
}

inline void FTextureParams_UBO::BufferElements()
{
	PendingUpdate = 0;
	FOpenGLBase::glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(FTextureParams_UBO::FElement)*Elements.Num(), &Elements(0));
}


/*-----------------------------------------------------------------------------
	FComplexSurfaceScale_UBO.
-----------------------------------------------------------------------------*/

inline const ANSICHAR* FComplexSurfaceScale_UBO::GLSL_Struct()
{
	return 
R"(layout(std140) uniform ComplexSurfaceScale
{
    vec4 csScale[CSSCALEMAX];
};
)";
}

inline INT FComplexSurfaceScale_UBO::ArraySize()
{
	return Min<INT>( 256, Min(FOpenGLBase::MaxUniformBlockSize, 65536) / sizeof(FComplexSurfaceScale_UBO::FElement));
}

inline void FComplexSurfaceScale_UBO::BufferElements()
{
	PendingUpdate = 0;
	FOpenGLBase::glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(FComplexSurfaceScale_UBO::FElement)*Elements.Num(), &Elements(0));
}
