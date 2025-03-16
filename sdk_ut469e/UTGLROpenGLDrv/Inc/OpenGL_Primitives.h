/*=============================================================================
	OpenGL_Primitives.h
	
	Texture UV scaling and panning helper.
	3D vertex and primitives helper.

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/


/*-----------------------------------------------------------------------------
	Helpers.
-----------------------------------------------------------------------------*/

// Do not use approximations (when applicable)
enum EPrecise
{
	E_Precise,
};

static constexpr bool IsIntAttrib( GLenum ArrayType)
{
	return	(	ArrayType == GL_INT
			||	ArrayType == GL_UNSIGNED_INT
			||	ArrayType == GL_SHORT
			||	ArrayType == GL_UNSIGNED_SHORT
			||	ArrayType == GL_BYTE
			||	ArrayType == GL_UNSIGNED_BYTE );
}


/*-----------------------------------------------------------------------------
	Structures.
-----------------------------------------------------------------------------*/

namespace FGL
{


//
// Simple 3D vertex.
//
class FVertex3D
{
public:
	static constexpr GLint  ArraySize = 3;
	static constexpr GLenum ArrayType = GL_FLOAT;

	FLOAT X, Y, Z;

	FVertex3D();
	FVertex3D( FLOAT InX, FLOAT InY, FLOAT InZ);
	FVertex3D( const FVector& InV);

	static void SetQuad( FVertex3D* Verts, FLOAT X1, FLOAT X2, FLOAT Y1, FLOAT Y2, FLOAT Z);
};
static_assert( sizeof(FVertex3D) == sizeof(GLfloat)*3, "Error in FVertex3D struct size/alignment");


//
// Simple 2D coordinates sent to vertex shader.
//
class FTextureUV
{
public:
	static constexpr GLint  ArraySize = 2;
	static constexpr GLenum ArrayType = GL_FLOAT;

	FLOAT U, V;

	FTextureUV();
	FTextureUV( FLOAT InU, FLOAT InV);

	static void SetQuad( FTextureUV* UVs, FLOAT U1, FLOAT U2, FLOAT V1, FLOAT V2);

	struct x2
	{
		static constexpr GLint  ArraySize = 4;
		static constexpr GLenum ArrayType = GL_FLOAT;
		FLOAT Data[ArraySize];
	};
};
static_assert( sizeof(FTextureUV) == sizeof(GLfloat)*2, "Error in FTextureUV struct size/alignment");


//
// Simple 3D coordinates sent to vertex shader.
//
class FTextureUVW
{
public:
	static constexpr GLint  ArraySize = 3;
	static constexpr GLenum ArrayType = GL_FLOAT;

	FLOAT U, V, W;

	FTextureUVW();
	FTextureUVW( FLOAT InU, FLOAT InV, FLOAT InW);
};
static_assert( sizeof(FTextureUVW) == sizeof(GLfloat)*3, "Error in FTextureUVW struct size/alignment");


//
// Precomputed texture scaling applied on top of unscaled UV coordinates.
//
class FTextureScale
{
public:
	FLOAT UScale, VScale;

	FTextureScale();
	FTextureScale( const FTextureInfo& Info);
	FTextureScale( const FTextureInfo& Info, EPrecise);
};
static_assert( sizeof(FTextureScale) == sizeof(GLfloat)*2, "Error in FTextureScale struct size/alignment");


//
// Precomputed biased texture panning.
//
class FTexturePan
{
public:
	static constexpr GLint  ArraySize = 2;
	static constexpr GLenum ArrayType = GL_FLOAT;

	FLOAT UPan, VPan;

	FTexturePan();
	FTexturePan( FLOAT InUPan, FLOAT InVPan);
	FTexturePan( const FTextureInfo& Info);
	FTexturePan( const FTextureInfo& Info, FLOAT PanBias);
};
static_assert( sizeof(FTexturePan) == sizeof(GLfloat)*2, "Error in FTexturePan struct size/alignment");


//
// Complex surface UV coordinates and panning sent to vertex shader.
//
class FTextureUVPan : public FTextureUV, public FTexturePan
{
public:
	static constexpr GLint  ArraySize = 4;
	static constexpr GLenum ArrayType = GL_FLOAT;

	FTextureUVPan();
	FTextureUVPan( FLOAT InU, FLOAT InV, const FTextureInfo& Info);
	FTextureUVPan( FLOAT InU, FLOAT InV, FLOAT InUPan, FLOAT InVPan);
};
static_assert( sizeof(FTextureUVPan) == sizeof(GLfloat)*4, "Error in FTextureUVPan struct size/alignment");


//
// RGBA packed color dword.
//
class FColorRGBA
{
public:
	static constexpr GLint  ArraySize = 4;
	static constexpr GLenum ArrayType = GL_UNSIGNED_BYTE;

	DWORD Color;

	FColorRGBA();
	FColorRGBA( DWORD InColor);
	FColorRGBA( const FColor& InColor);

	operator bool() const;

	static void SetQuad( FColorRGBA* Colors, DWORD InColor);
	static FColorRGBA FlatColor( const FSurfaceInfo& Surface);
};
static_assert( sizeof(FColorRGBA) == sizeof(GLuint), "Error in FColorRGBA struct size/alignment");


//
// Defines four generic 8 bit integers
// 
class uint8x4
{
public:
	static constexpr GLint  ArraySize = 4;
	static constexpr GLenum ArrayType = GL_UNSIGNED_BYTE;

	GLubyte Data[4];

	uint8x4();
	uint8x4( GLubyte InData0, GLubyte InData1, GLubyte InData2, GLubyte InData3);
	uint8x4( const uint8x4& InOther);
};
static_assert( sizeof(uint8x4) == sizeof(GLuint), "Error in uint8x4 struct size/alignment");


//
// Defines two generic 16 bit integers
//
class uint16x2
{
public:
	static constexpr GLint  ArraySize = 2;
	static constexpr GLenum ArrayType = GL_UNSIGNED_SHORT;
	
	GLushort Data[2];

	uint16x2();
	uint16x2( GLushort InData0, GLushort InData1);
	uint16x2( const uint16x2& InOther);
};
static_assert( sizeof(uint16x2) == sizeof(GLuint), "Error in uint16x2 struct size/alignment");


//
// Defines four generic 16 bit integers
//
class uint16x4
{
public:
	static constexpr GLint  ArraySize = 4;
	static constexpr GLenum ArrayType = GL_UNSIGNED_SHORT;

	GLushort Data[4];

	uint16x4();
	uint16x4( GLushort InData0, GLushort InData1, GLushort InData2, GLushort InData3);
	uint16x4( const uint16x4& InOther);
};
static_assert( sizeof(uint16x4) == sizeof(GLuint)*2, "Error in uint16x4 struct size/alignment");


//
// Complex surface texture scale container
//
class FcsTextureScale : public FPlane
{
public:
	using FPlane::FPlane;
	// GetTypeHash implemented in global scope
};

};


/*-----------------------------------------------------------------------------
	FVertex3D.
-----------------------------------------------------------------------------*/

inline FGL::FVertex3D::FVertex3D()
{
}

inline FGL::FVertex3D::FVertex3D(FLOAT InX, FLOAT InY, FLOAT InZ)
	: X(InX), Y(InY), Z(InZ)
{
}

inline FGL::FVertex3D::FVertex3D(const FVector& InV)
	: X(InV.X), Y(InV.Y), Z(InV.Z)
{
}

inline void FGL::FVertex3D::SetQuad( FVertex3D* Verts, FLOAT X1, FLOAT X2, FLOAT Y1, FLOAT Y2, FLOAT Z)
{
	Verts[0] = FGL::FVertex3D( X1, Y1, Z);
	Verts[1] = FGL::FVertex3D( X2, Y1, Z);
	Verts[2] = FGL::FVertex3D( X2, Y2, Z);
	Verts[3] = FGL::FVertex3D( X1, Y2, Z);
}


/*-----------------------------------------------------------------------------
	FTextureUV.
-----------------------------------------------------------------------------*/

inline FGL::FTextureUV::FTextureUV()
{
}

inline FGL::FTextureUV::FTextureUV( FLOAT InU, FLOAT InV)
	: U(InU)
	, V(InV)
{
}

inline void FGL::FTextureUV::SetQuad( FTextureUV* UVs, FLOAT U1, FLOAT U2, FLOAT V1, FLOAT V2)
{
	UVs[0] = FGL::FTextureUV( U1, V1);
	UVs[1] = FGL::FTextureUV( U2, V1);
	UVs[2] = FGL::FTextureUV( U2, V2);
	UVs[3] = FGL::FTextureUV( U1, V2);
}


/*-----------------------------------------------------------------------------
	FTextureUVW.
-----------------------------------------------------------------------------*/

inline FGL::FTextureUVW::FTextureUVW()
{
}

inline FGL::FTextureUVW::FTextureUVW( FLOAT InU, FLOAT InV, FLOAT InW)
	: U(InU)
	, V(InV)
	, W(InW)
{
}

/*-----------------------------------------------------------------------------
	FTextureScale.
-----------------------------------------------------------------------------*/

inline FGL::FTextureScale::FTextureScale()
{
}

inline FGL::FTextureScale::FTextureScale( const FTextureInfo& Info)
{
#if USES_SSE_INTRINSICS
	__m128  mm_uv_scale = _mm_loadu_ps( &Info.UScale);
	__m128i mm_uv_size  = _mm_loadu_si128( &Info.USize); // Could be optimized for single load, but a _mm_setzero_ps is needed with intrinsics.
	__m128  mm_uv_div   = _mm_mul_ps_ext( mm_uv_scale, mm_uv_size); // See UTGLROpenGL.h for this define
	__m128  mm_uv_mult  = _mm_rcp_ps( mm_uv_div);
	_mm_storel_pi( (__m64*)&UScale, mm_uv_mult);
#else
	UScale = 1.0f / (Info.UScale * Info.USize);
	VScale = 1.0f / (Info.VScale * Info.VSize);
#endif
}

inline FGL::FTextureScale::FTextureScale( const FTextureInfo& Info, EPrecise)
{
#if USES_SSE_INTRINSICS
	const float f_one = 1.0f;
	__m128  mm_uv_scale = _mm_loadu_ps( &Info.UScale);
	__m128i mm_uv_size  = _mm_loadu_si128( &Info.USize); // Could be optimized for single load, but a _mm_setzero_ps is needed with intrinsics.
	__m128  mm_uv_div   = _mm_mul_ps_ext( mm_uv_scale, mm_uv_size); // See UTGLROpenGL.h for this define
	__m128  mm_one      = _mm_load_ps1( &f_one);
	__m128  mm_uv_mult  = _mm_div_ps( mm_one, mm_uv_div);
	_mm_storel_pi( (__m64*)&UScale, mm_uv_mult);
#else
	UScale = 1.0f / (Info.UScale * Info.USize);
	VScale = 1.0f / (Info.VScale * Info.VSize);
#endif
}


/*-----------------------------------------------------------------------------
	FTexturePan.
-----------------------------------------------------------------------------*/

inline FGL::FTexturePan::FTexturePan()
{
}

inline FGL::FTexturePan::FTexturePan( FLOAT InUPan, FLOAT InVPan)
	: UPan(InUPan)
	, VPan(InVPan)
{
}

inline FGL::FTexturePan::FTexturePan( const FTextureInfo& Info)
	: UPan(Info.Pan.X)
	, VPan(Info.Pan.Y)
{
}

inline FGL::FTexturePan::FTexturePan( const FTextureInfo& Info, FLOAT PanBias)
{
#if USES_SSE_INTRINSICS
	__m128 mm_pan_bias = _mm_load1_ps( &PanBias);
	__m128 mm_pan      = _mm_loadu_ps( &Info.Pan.X);
	__m128 mm_uv_scale = _mm_loadu_ps( &Info.UScale );
	__m128 mm_uv_pan   = _mm_add_ps( mm_pan, _mm_mul_ps( mm_pan_bias, mm_uv_scale) );
	_mm_storel_pi( (__m64*)&UPan, mm_uv_pan);
#else
	UPan = Info.Pan.X + (PanBias * Info.UScale);
	VPan = Info.Pan.Y + (PanBias * Info.VScale);
#endif
}


/*-----------------------------------------------------------------------------
	FTextureUVPan.
-----------------------------------------------------------------------------*/

inline FGL::FTextureUVPan::FTextureUVPan()
{
}

inline FGL::FTextureUVPan::FTextureUVPan( FLOAT InU, FLOAT InV, const FTextureInfo& Info)
	: FTextureUV(InU,InV)
	, FTexturePan(Info)
{
}

inline FGL::FTextureUVPan::FTextureUVPan( FLOAT InU, FLOAT InV, FLOAT InUPan, FLOAT InVPan)
	: FTextureUV(InU,InV)
	, FTexturePan(InUPan,InVPan)
{
}


/*-----------------------------------------------------------------------------
	FColorRGBA.
-----------------------------------------------------------------------------*/

inline FGL::FColorRGBA::FColorRGBA()
{
}

inline FGL::FColorRGBA::FColorRGBA( DWORD InColor)
	: Color(InColor)
{
}

inline FGL::FColorRGBA::FColorRGBA( const FColor& InColor)
	: Color(GET_COLOR_DWORD(InColor))
{
}

inline FGL::FColorRGBA::operator bool() const
{
	return !!Color;
}

inline void FGL::FColorRGBA::SetQuad( FColorRGBA* Colors, DWORD InColor)
{
	Colors[0] = InColor;
	Colors[1] = InColor;
	Colors[2] = InColor;
	Colors[3] = InColor;
}


/*-----------------------------------------------------------------------------
	uint8x4.
-----------------------------------------------------------------------------*/

inline FGL::uint8x4::uint8x4()
{
}

inline FGL::uint8x4::uint8x4( GLubyte InData0, GLubyte InData1, GLubyte InData2, GLubyte InData3)
	: Data{InData0, InData1, InData2, InData3}
{
}

inline FGL::uint8x4::uint8x4( const uint8x4& InOther)
{
	*(DWORD*)this = *(DWORD*)&InOther;
}


/*-----------------------------------------------------------------------------
	uint16x2.
-----------------------------------------------------------------------------*/

inline FGL::uint16x2::uint16x2()
{
}

inline FGL::uint16x2::uint16x2( GLushort InData0, GLushort InData1)
	: Data{InData0, InData1}
{
}

inline FGL::uint16x2::uint16x2( const uint16x2& InOther)
{
	*(DWORD*)this = *(DWORD*)&InOther;
}


/*-----------------------------------------------------------------------------
	uint16x4.
-----------------------------------------------------------------------------*/

inline FGL::uint16x4::uint16x4()
{
}

inline FGL::uint16x4::uint16x4( GLushort InData0, GLushort InData1, GLushort InData2, GLushort InData3)
	: Data{InData0, InData1, InData2, InData3}
{
}

inline FGL::uint16x4::uint16x4( const uint16x4& InOther)
{
	((DWORD*)this)[0] = ((DWORD*)&InOther)[0];
	((DWORD*)this)[1] = ((DWORD*)&InOther)[1];
}


/*-----------------------------------------------------------------------------
	FcsTextureScale.
-----------------------------------------------------------------------------*/

static inline DWORD GetTypeHash( const FGL::FcsTextureScale& S)
{
#undef EXPONENT
#define EXPONENT(f) ((*(DWORD*)(&f)) >> 23)
	// Use DJB2 on the exponents
	DWORD hash = 5381;
	for ( INT i=0; i<3; i++)
		hash = ((hash << 5) + hash) + EXPONENT(((&S.X)[i]));
	return hash;
#undef EXPONENT
}
