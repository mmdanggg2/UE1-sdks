/*=============================================================================
	OpenGL.h: Unreal OpenGL support header.
	Portions copyright 1999 Epic Games, Inc. All Rights Reserved.

	Revision history:

=============================================================================*/


//Make sure valid build config selected
#undef UTGLR_VALID_BUILD_CONFIG

#if defined(UNREAL_TOURNAMENT_OLDUNREAL) && !defined(UTGLR_UT_469_BUILD)
#define UTGLR_UT_469_BUILD 1
#endif

#ifdef UTGLR_UT_BUILD
#define UTGLR_VALID_BUILD_CONFIG 1
#endif
#ifdef UTGLR_UT_469_BUILD
#define UTGLR_VALID_BUILD_CONFIG 1
#endif
#ifdef UTGLR_DX_BUILD
#define UTGLR_VALID_BUILD_CONFIG 1
#endif
#ifdef UTGLR_RUNE_BUILD
#define UTGLR_VALID_BUILD_CONFIG 1
#endif

#if !UTGLR_VALID_BUILD_CONFIG
#error Valid build config not selected.
#endif
#undef UTGLR_VALID_BUILD_CONFIG

#define UTGLR_SUPPORT_DXGI_INTEROP 0


#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#ifdef WIN32
#include <tchar.h>
#define COBJMACROS
#define INITGUID
#define CINTERFACE
#define D3D11_NO_HELPERS
#include <windows.h>
#include <d3d11.h>
#endif

//#include <map>
#pragma warning(disable : 4663)
#pragma warning(disable : 4018)
//#include <vector>

#pragma warning(disable : 4244)
#pragma warning(disable : 4245)

#pragma warning(disable : 4146)

#include "c_gclip.h"


#ifndef FORCEINLINE
#define FORCEINLINE inline
#endif

//Per-game feature switches
#if UTGLR_RUNE_BUILD
#define UTGLR_USES_ALPHABLEND 1
#endif
#if UTGLR_UT_469_BUILD
#define UTGLR_USES_SCENENODE_HACK 0
#define UTGLR_USES_ALPHABLEND 1
#define USES_AMBIENTLESS_LIGHTMAPS 1
#define USES_STATIC_BSP 1
#undef UTGLR_NO_APP_MALLOC
#endif

#define GL_DEV 0

#if GL_DEV == 2
# define GL_ERROR_ASSERT { GLenum LocalError = FOpenGLBase::glGetError(); if(LocalError) {__debugbreak();} }
# define GL_DEV_CHECK(expr) { if(!(expr)) {__debugbreak();} }
#elif GL_DEV == 1
# define GL_ERROR_ASSERT { check(!FOpenGLBase::glGetError()); }
# define GL_DEV_CHECK(expr) { verify(expr); }
#else
# define GL_ERROR_ASSERT {}
# define GL_DEV_CHECK(expr) {}
#endif

//Default feature switches
#ifndef HACKFLAGS_NoNearZ
#define HACKFLAGS_NoNearZ 0x1
#endif

#ifndef UTGLR_USES_ALPHABLEND
#define UTGLR_USES_ALPHABLEND 0
#endif

#ifndef UTGLR_USES_SCENENODE_HACK
#define UTGLR_USES_SCENENODE_HACK 1
#endif

#ifndef USES_AMBIENTLESS_LIGHTMAPS
#define USES_AMBIENTLESS_LIGHTMAPS 0
#endif

#ifndef USES_STATIC_BSP
#define USES_STATIC_BSP 0
#endif

#if USES_SSE_INTRINSICS
	#include <xmmintrin.h>
	#include <emmintrin.h>

	//Higor: Compilers can't do abstractions for a living, so we expand on existing intrinsics.

	//Shuffle a single (float) XMM register
	#define _mm_pshufd_ps(v,i) _mm_castsi128_ps( _mm_shuffle_epi32( _mm_castps_si128(v), i))

	//Move 8 bytes from low to high
	inline __m128i _mm_movelh_si128( __m128i xmm1, __m128i xmm2) {
		return _mm_castps_si128(_mm_movelh_ps( _mm_castsi128_ps(xmm1), _mm_castsi128_ps(xmm2)));
	}

	//Load 8 bytes onto low and high
	inline __m128 _mm_load_xyxy( const float* Data) {
		__m128 xy = _mm_loadl_pi( _mm_setzero_ps(), (const __m64*)Data);
		return _mm_movelh_ps( xy, xy);
	}

	//Load 16 bytes (no cast)
	inline __m128i _mm_loadu_si128( const int* data) {
		return _mm_loadu_si128( (const __m128i*)data);
	}

	//Multiply float and int vector (float result)
	inline __m128 _mm_mul_ps_ext( __m128 xmmF, __m128i xmmI) {
		return _mm_mul_ps( xmmF, _mm_cvtepi32_ps(xmmI));
	}

	//Multiply float and int vector (float result)
	inline __m128 _mm_mul_ps_ext( __m128i xmmI, __m128 xmmF) {
		return _mm_mul_ps( _mm_cvtepi32_ps(xmmI), xmmF);
	}
#endif

//Debug defines
//#define UTGLR_DEBUG_SHOW_CALL_COUNTS
//#define UTGLR_DEBUG_WORLD_WIREFRAME
//#define UTGLR_DEBUG_ACTOR_WIREFRAME


#ifdef UTGLR_DEBUG_SHOW_CALL_COUNTS
#define UTGLR_DEBUG_CALL_COUNT(name) \
	{ \
		static unsigned int s_c; \
		dbgPrintf("utglr: " #name " = %u\n", s_c); \
		s_c++; \
	}
#else
#define UTGLR_DEBUG_CALL_COUNT(name)
#endif

#ifdef UTGLR_DEBUG_SHOW_TEX_CONVERT_COUNTS
#define UTGLR_DEBUG_TEX_CONVERT_COUNT(name) \
	{ \
		static unsigned int s_c; \
		dbgPrintf("utglr: " #name " = %u\n", s_c); \
		s_c++; \
	}
#else
#define UTGLR_DEBUG_TEX_CONVERT_COUNT(name)
#endif


/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

#ifdef _WIN32
#define GL_DLL ("OpenGL32.dll")
#endif


//
// Projection constants
//
constexpr FLOAT zNear = 0.5f;
constexpr FLOAT zFar  = 49152.0f;
constexpr FLOAT dA = zFar / (zFar - zNear);
constexpr FLOAT dB = zFar * zNear / ( zNear - zFar );

constexpr FLOAT Tile_Depth = ( dA + dB / 1.00f );
constexpr FLOAT NoNearZ_ARB_Depth = ( dA + dB / 1.25f );


//
// Compare two coordinate systems
// Note: Win32 has this check optimized using 3 XMM vector comparisons
//
static inline bool SameCoords( const FCoords& A, const FCoords& B)
{
#if USES_TRAWDATA
	if ( sizeof(FCoords) == 12 * sizeof(FLOAT) )
		return TRawData<FCoords>::Equals( A, B);
#endif
	return A.Origin == B.Origin && A.XAxis == B.XAxis && A.YAxis == B.YAxis && A.ZAxis == B.ZAxis;
}


//
// For games that support it, get ZoneNumber of a surface.
//
static inline DWORD GetZoneNumber( const FSurfaceInfo& Surface)
{
#if RENDER_DEVICE_OLDUNREAL == 469
	return Surface.Zone->Region.ZoneNumber;
#else
	return 0;
#endif
}


//
// Stream data into a buffer, modifies pointer.
// Note: Compiler will optimize out writes to Stream pointer if code has no branches.
//
template <class P> static inline void StreamData( P&& Data, void*& Stream)
{
	*(P*)Stream = Data;
	((BYTE*&)Stream) += sizeof(Data);
}


/*-----------------------------------------------------------------------------
	OpenGLDrv.
-----------------------------------------------------------------------------*/

class FOpenGLBase;
class  FOpenGL12;
class  FOpenGL3;
class  FOpenGL45;
class FOpenGLTexture;
class UOpenGLRenderDevice;
struct FPendingTexture;
struct FTextureUploadState;
typedef bool (FASTCALL UOpenGLRenderDevice::*f_SoftwareConvert)( FTextureUploadState&, DWORD*);

namespace FGL
{
	namespace Draw
	{
		class Command;
		class CommandChain;
	};
	namespace DrawBuffer
	{
		struct FBase;
	};
};

struct FTextureFormatInfo
{
	GLenum InternalFormat;
	GLenum SourceFormat;
	GLenum Type;
	GLubyte BlockWidth;
	GLubyte BlockHeight;
	GLubyte BlockBytes;
	GLubyte Supported:1;
	GLubyte Compressed:1;

	f_SoftwareConvert SoftwareConvert;

	FTextureFormatInfo()
	{}

	FTextureFormatInfo( ETextureFormat TEXF)
	{
		BlockWidth = (GLubyte)FTextureBlockWidth(TEXF);
		BlockHeight = (GLubyte)FTextureBlockHeight(TEXF);
		BlockBytes = (GLubyte)FTextureBlockBytes(TEXF);
		Compressed = FIsCompressedFormat(TEXF) > 0;
	}

	GLuint GetTextureBytes( GLuint Width, GLuint Height, GLuint Depth=1) const
	{
		GLuint Size = BlockBytes;
		Size *= Align<GLuint>(Width,BlockWidth) / (GLuint)BlockWidth;
		Size *= Align<GLuint>(Height,BlockHeight) / (GLuint)BlockHeight;
		// No 3D for now.
		return Size;
	}
};

// Utils
#include "DataWrapper.h"
#include "FFreeIndexList.h"
#include "TArrayExt.h"
#include "TMapExt.h"
#include "TemplateQueue.h"

// Render model
#include "OpenGL_Color.h"
#include "OpenGL_SurfaceModel.h"
#include "OpenGL_SamplerModel.h"
#include "OpenGL_ProgramModel.h"
#include "OpenGL_CacheID.h"
#include "OpenGL_Primitives.h"

// Render interface
#include "FOpenGLSwizzle.h"
#include "FOpenGLBase.h"
#include "FOpenGLUniform.h"
#include "FOpenGLTexture.h"
#include "FOpenGLBase_Inlines.h"
#include "OpenGL_UBO.h"
#include "OpenGL_VBO.h"

#include "FCharWriter.h"
#include "OpenGL_TexturePool.h"
#include "OpenGL_DrawCommand.h"



struct FTextureUploadState
{
	FTextureInfo*   Info;
	BYTE*           Data;
	FOpenGLTexture* Texture;
	INT             Level;
	INT             CurrentMip;
	DWORD           PolyFlags;
	INT             USize, VSize, Layer;
	FOpenGLSwizzle  Swizzle;
	BYTE            Format;
	BYTE            InitialFormat;

	static TArray<BYTE> Compose;

	FTextureUploadState()
	{}

	FTextureUploadState( const FTextureInfo* InInfo, DWORD InPolyFlags=0)
		: Info((FTextureInfo*)InInfo)
		, Data(nullptr)
		, Texture(nullptr)
		, Level(0)
		, CurrentMip(0)
		, PolyFlags(InPolyFlags)
		, USize(0), VSize(0), Layer(0)
		, Swizzle(0)
		, Format(InInfo->Format)
		, InitialFormat(InInfo->Format)
	{}

	static bool FASTCALL AllocateCompose( INT USize, INT VSize, BYTE Format);
};


// VBO BspNode to surface mapping
struct FBspNodeVBOMapping
{
	INT VertexStart;
	INT VertexCount;
};


#ifdef _WIN32
typedef struct
{
	INT NewColorBytes;
	// TODO: Specify Context profile here (?)
} InitARBPixelFormatWndProcParams_t;
#endif

enum EColorCorrectionMode
{
	CC_InShader,
	CC_UseFramebuffer,
	CC_UseGammaRamp
};

//PolyFlags extension
enum EPolyFlagsExt
{
	// Unused bits that can be repurposed
	PF_Clear         = PF_SpecialLit|PF_Memorized|PF_Portal|PF_NoMerge|PF_LowShadowDetail|PF_HighShadowDetail,

	// Repurposed polyflags
	PF_AlphaHack     = PF_NoMerge,
	PF_NoZReject     = PF_Memorized,
	PF_NoNearZ       = PF_Portal,
#if !UTGLR_USES_ALPHABLEND
	PF_AlphaBlend    = 0,
#endif

	// PolyFlags that impact texture object state
	PF_Dynamic       = PF_NoSmooth,

	// PolyFlags that are not compatible with multi-buffering
	PF_NoMultiBuffering = PF_Translucent|PF_AlphaBlend|PF_Highlighted|PF_Modulated,
};

//Render path modes
enum EContextType
{
	CONTEXTTYPE_DETECT = 0,
	CONTEXTTYPE_GL1,
	CONTEXTTYPE_GL3,
	CONTEXTTYPE_GL4,
	CONTEXTTYPE_GLES,

	CONTEXTTYPE_MAX,
};

//Draw buffering mode
enum EDrawBufferMode
{
	DRAWBUFFER_GouraudPolygon     = 1 <<  0,
	DRAWBUFFER_Line               = 1 <<  1,
	DRAWBUFFER_Quad               = 1 <<  2,
	DRAWBUFFER_ComplexSurface     = 1 <<  3,
	DRAWBUFFER_ComplexSurfaceVBO  = 1 <<  4,
	DRAWBUFFER_Decal              = 1 <<  5,

	DRAWBUFFER_CommandBuffer      = 1 << 31, // This is a command buffer, always exec draws

	DRAWBUFFER_ALL                = 0xFFFFFFFF,
};

#include "OpenGL_DrawBuffer.h"

//
// An OpenGL rendering device attached to a viewport.
//
#if RENDER_DEVICE_OLDUNREAL == 469
class UOpenGLRenderDevice : public URenderDeviceOldUnreal469 {
	DECLARE_CLASS(UOpenGLRenderDevice, URenderDeviceOldUnreal469, CLASS_Config, OpenGLDrv)
#else
class UOpenGLRenderDevice : public URenderDevice {
	DECLARE_CLASS(UOpenGLRenderDevice, URenderDevice, CLASS_Config, OpenGLDrv)
#endif



	//Debug print function
	int dbgPrintf(const char *format, ...);

	//Debug bits
	DWORD m_debugBits;
	inline bool FASTCALL DebugBit(DWORD debugBit) {
		return ((m_debugBits & debugBit) != 0);
	}
	enum {
		DEBUG_BIT_BASIC		= 0x00000001,
		DEBUG_BIT_GL_ERROR	= 0x00000002,
		DEBUG_BIT_ANY		= 0xFFFFFFFF
	};

	// Conflict resolution for other engines
	#if RENDER_DEVICE_OLDUNREAL == 469
		//..
	#else
		static constexpr UBOOL UseLightmapAtlas = 0;
	#endif

	FLOAT m_csUDot;
	FLOAT m_csVDot;

	struct FGammaRamp {
		_WORD red[256];
		_WORD green[256];
		_WORD blue[256];
	};
	struct FByteGammaRamp {
		BYTE red[256];
		BYTE green[256];
		BYTE blue[256];
	};

	UBOOL WasFullscreen;

	bool m_prevSwapBuffersStatus;

	// Texture Pool system
	static FGL::FTexturePool TexturePool;

	TArray<FPlane> Modes;

	//Use UViewport* in URenderDevice
	//UViewport* Viewport;


	// Timing.
	DWORD BindCycles, ImageCycles, ComplexCycles, GouraudCycles, TileCycles;

	DWORD m_AASwitchCount;
	DWORD m_sceneNodeCount;
#if UTGLR_USES_SCENENODE_HACK
	DWORD m_sceneNodeHackCount;
#endif

	BYTE ContextType;
	BYTE SelectedContextType;
	BYTE m_padding0;
	BYTE m_padding1;
	static DWORD GetContextType();
	bool DetectContextType( DWORD PreferredContextType);

	FLOAT LODBias;
	DWORD ColorCorrectionMode;
	FLOAT GammaOffset;
	FLOAT GammaOffsetRed;
	FLOAT GammaOffsetGreen;
	FLOAT GammaOffsetBlue;
	INT Brightness;
	UBOOL GammaCorrectScreenshots;
	UBOOL OneXBlending;
	INT MaxAnisotropy;
	INT RefreshRate;
	UBOOL AlwaysMipmap;
	UBOOL UsePrecache;
	UBOOL UseTrilinear;
	UBOOL UseHDTextures;
	UBOOL Use16BitTextures;
	UBOOL ForceNoSmooth;
	INT DetailMax;
	UBOOL ShaderAlphaHUD;
	UBOOL UseDrawGouraud469;
	UBOOL UseStaticGeometry;
	INT SwapInterval;
	UBOOL UseShaderGamma;
#if UTGLR_USES_SCENENODE_HACK
	UBOOL SceneNodeHack;
#endif

	UBOOL UseAA;
	INT NumAASamples;
	UBOOL NoAATiles;

	FColor SurfaceSelectionColor;

	// stijn: GL on DXGI support
#if UTGLR_SUPPORT_DXGI_INTEROP
	UBOOL UseLowLatencySwapchain;
	ID3D11DeviceContext* D3DContext;
	IDXGISwapChain* DXGISwapChain;

	ID3D11Device* D3DDevice;
	HANDLE D3DDeviceInteropHandle;

	ID3D11RenderTargetView* D3DColorView;
	HANDLE D3DColorViewInteropHandle;

	ID3D11DepthStencilView* D3DDepthStencilView;
	HANDLE D3DDepthStencilViewInteropHandle;
#endif

	// Shader config
	UBOOL SmoothMasking;
	UBOOL m_SmoothMasking;

	UBOOL PreferDedicatedGPU;

	//Previous lock variables
	//Used to detect changes in settings
	BITFIELD PL_DetailTextures;
	UBOOL PL_ForceNoSmooth;
	UBOOL PL_AlwaysMipmap;
	UBOOL PL_UseTrilinear;
	UBOOL PL_Use16BitTextures;
	UBOOL PL_TexDXT1ToDXT3;
	INT PL_MaxAnisotropy;
	FLOAT PL_LODBias;
	INT PL_DetailMax;
	FLOAT PL_ShaderBrightness;

	static UBOOL GUsingGammaRamp;

	bool m_setGammaRampSucceeded;
	FLOAT SavedGammaCorrection;

	DWORD m_numDepthBits;

	INT m_rpPassCount;
	INT m_rpTMUnits;
	bool m_rpForceSingle;
	bool m_rpMasked;
	bool m_rpSetDepthEqual;
	bool m_padding_2;

	// Hit info.
	BYTE* m_HitData;
	INT* m_HitSize;
	INT m_HitBufSize;
	INT m_HitCount;
	CGClip m_gclip;


	DWORD m_currentFrameCount;

	// Lock variables.
	FPlane FlashScale, FlashFog;
	FLOAT m_RProjZ, m_Aspect;
	FLOAT m_RFX2, m_RFY2;
	GLdouble m_NearClip[4];
#if UTGLR_USES_SCENENODE_HACK
	INT m_sceneNodeX, m_sceneNodeY;
#endif

	bool m_usingAA;
	bool m_curAAEnable;
	bool m_defAAEnable;
	INT m_initNumAASamples;

	enum {
		PF2_NEAR_Z_RANGE_HACK   = 0x01,
		PF2_SHADER_ALPHA_HUD    = 0x02
	};
	DWORD m_curBlendFlags;
	bool m_alphaToCoverageEnabled;
	DWORD m_curPolyFlags;
	DWORD m_shaderAlphaHUD;
	INT m_bufferActorTrisCutoff;

	FLOAT m_detailTextureColor3f_1f[4];
	DWORD m_detailTextureColor4ub;

	BYTE m_gpAlpha;
	bool m_gpFogEnabled;

	// Static variables.
	static const TCHAR* g_pSection;
	static INT NumDevices;
	static INT LockCount;

	// FOpenGLBase initializator proxies.
	typedef bool (*InitGLProc)();
	static bool InitGL1();
	static bool InitGL3();
//	static bool InitGL4();
//	static bool InitGLES();

	FOpenGLBase* GL;
	typedef UBOOL (UOpenGLRenderDevice::*SetGLProc)( void* Window);
	static SetGLProc SetGL[CONTEXTTYPE_MAX];
	UBOOL SetGLError( void* Window) { return false; }
	UBOOL SetGL1( void* Window);
	UBOOL SetGL3( void* Window);
//	UBOOL SetGL4( void* Window);
//	UBOOL SetGLES( void* Window);

#if __WIN32__
#else
	static UBOOL GLLoaded;
#endif

	static bool g_gammaFirstTime;
	static bool g_haveOriginalGammaRamp;
#if __WIN32__
	static FGammaRamp g_originalGammaRamp;
#endif

	// UObject interface.
	void StaticConstructor();


	// Implementation.
	void FASTCALL SC_AddBoolConfigParam(DWORD BitMaskOffset, const TCHAR *pName, UBOOL &param, ECppProperty EC_CppProperty, INT InOffset, UBOOL defaultValue);
	void FASTCALL SC_AddIntConfigParam(const TCHAR *pName, INT &param, ECppProperty EC_CppProperty, INT InOffset, INT defaultValue);
	void FASTCALL SC_AddFloatConfigParam(const TCHAR *pName, FLOAT &param, ECppProperty EC_CppProperty, INT InOffset, FLOAT defaultValue);
	void FASTCALL SC_AddByteConfigParam(const TCHAR *pName, DWORD &param, ECppProperty EC_CppProperty, INT InOffset, DWORD defaultValue, UEnum* Enum=nullptr);

	void FASTCALL DbgPrintInitParam(const char *pName, INT value);
	void FASTCALL DbgPrintInitParam(const char *pName, FLOAT value);

	void BuildGammaRamp(float redGamma, float greenGamma, float blueGamma, int brightness, FGammaRamp &ramp);
	void BuildGammaRamp(float redGamma, float greenGamma, float blueGamma, int brightness, FByteGammaRamp &ramp);
	void SetGamma(FLOAT GammaCorrection);
	void ResetGamma(void);

	static bool FASTCALL IsGLExtensionSupported(const char *pExtensionsString, const char *pExtensionName);

	UBOOL FailedInitf(const TCHAR* Fmt, ...);
	void Exit();
	void ShutdownAfterError();

	UBOOL SetRes(INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen);
	void UnsetRes();

	void CheckGLErrorFlag(const TCHAR *pTag);

	void UpdateStateLocks();
	void ConfigValidate_RequiredExtensions(void);
	void ConfigValidate_Main(void);

#ifdef _WIN32
	void FASTCALL SetBasicPixelFormat(INT NewColorBytes);
	bool FASTCALL SetAAPixelFormat(INT NewColorBytes);
#endif


#ifndef __UNIX__
	void PrintFormat(HDC hDC, INT nPixelFormat) {
		guard(UOpenGlRenderDevice::PrintFormat);
		FString Flags;
		PIXELFORMATDESCRIPTOR pfd;
		DescribePixelFormat(hDC, nPixelFormat, sizeof(pfd), &pfd);
		if (pfd.dwFlags & PFD_DRAW_TO_WINDOW) Flags += TEXT(" PFD_DRAW_TO_WINDOW");
		if (pfd.dwFlags & PFD_DRAW_TO_BITMAP) Flags += TEXT(" PFD_DRAW_TO_BITMAP");
		if (pfd.dwFlags & PFD_SUPPORT_GDI) Flags += TEXT(" PFD_SUPPORT_GDI");
		if (pfd.dwFlags & PFD_SUPPORT_OPENGL) Flags += TEXT(" PFD_SUPPORT_OPENGL");
		if (pfd.dwFlags & PFD_GENERIC_ACCELERATED) Flags += TEXT(" PFD_GENERIC_ACCELERATED");
		if (pfd.dwFlags & PFD_GENERIC_FORMAT) Flags += TEXT(" PFD_GENERIC_FORMAT");
		if (pfd.dwFlags & PFD_NEED_PALETTE) Flags += TEXT(" PFD_NEED_PALETTE");
		if (pfd.dwFlags & PFD_NEED_SYSTEM_PALETTE) Flags += TEXT(" PFD_NEED_SYSTEM_PALETTE");
		if (pfd.dwFlags & PFD_DOUBLEBUFFER) Flags += TEXT(" PFD_DOUBLEBUFFER");
		if (pfd.dwFlags & PFD_STEREO) Flags += TEXT(" PFD_STEREO");
		if (pfd.dwFlags & PFD_SWAP_LAYER_BUFFERS) Flags += TEXT("PFD_SWAP_LAYER_BUFFERS");
		debugf(NAME_Init, TEXT("Pixel format %i:"), nPixelFormat);
		debugf(NAME_Init, TEXT("   Flags:%s"), *Flags);
		debugf(NAME_Init, TEXT("   Pixel Type: %i"), pfd.iPixelType);
		debugf(NAME_Init, TEXT("   Bits: Color=%i R=%i G=%i B=%i A=%i"), pfd.cColorBits, pfd.cRedBits, pfd.cGreenBits, pfd.cBlueBits, pfd.cAlphaBits);
		debugf(NAME_Init, TEXT("   Bits: Accum=%i Depth=%i Stencil=%i"), pfd.cAccumBits, pfd.cDepthBits, pfd.cStencilBits);
		unguard;
	}
#endif

	UBOOL Init(UViewport* InViewport, INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen);

	static QSORT_RETURN CDECL CompareRes(const FPlane* A, const FPlane* B) {
		return (QSORT_RETURN) (((A->X - B->X) != 0.0f) ? (A->X - B->X) : (A->Y - B->Y));
	}

	UBOOL Exec(const TCHAR* Cmd, FOutputDevice& Ar);
	void Lock(FPlane InFlashScale, FPlane InFlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* InHitData, INT* InHitSize);
	void SetSceneNode(FSceneNode* Frame);
	void Unlock(UBOOL Blit);
	void Flush(UBOOL AllowPrecache);

	void DrawComplexSurface(FSceneNode* Frame, FSurfaceInfo& Surface, FSurfaceFacet& Facet);
#ifdef UTGLR_RUNE_BUILD
	void PreDrawFogSurface();
	void PostDrawFogSurface();
	void DrawFogSurface(FSceneNode* Frame, FFogSurf &FogSurf);
	void PreDrawGouraud(FSceneNode* Frame, FLOAT FogDistance, FPlane FogColor);
	void PostDrawGouraud(FLOAT FogDistance);
#endif
	void DrawGouraudPolygon(FSceneNode* Frame, FTextureInfo& Info, FTransTexture** Pts, INT NumPts, DWORD PolyFlags, FSpanBuffer* Span);
	void DrawTile(FSceneNode* Frame, FTextureInfo& Info, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, class FSpanBuffer* Span, FLOAT Z, FPlane Color, FPlane Fog, DWORD PolyFlags);
	void Draw3DLine(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2);
	void Draw2DLine(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2);
	void Draw2DPoint(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FLOAT X1, FLOAT Y1, FLOAT X2, FLOAT Y2, FLOAT Z);

	void ClearZ(FSceneNode* Frame);
	void PushHit(const BYTE* Data, INT Count);
	void PopHit(INT Count, UBOOL bForce);
	void GetStats(TCHAR* Result);
	void ReadPixels(FColor* Pixels);
	void EndFlash();
	void PrecacheTexture(FTextureInfo& Info, DWORD PolyFlags);

	// OldUnreal UT extended interface
	void DrawGouraudTriangles(const FSceneNode* Frame, const FTextureInfo& Info, FTransTexture* const Pts, INT NumPts, DWORD PolyFlags, DWORD DataFlags, FSpanBuffer* Span) ;
	UBOOL SupportsTextureFormat(ETextureFormat Format);
	void UpdateTextureRect( FTextureInfo& Info, INT U, INT V, INT UL, INT VL);

	void SetStaticBsp( struct FStaticBspInfoBase& StaticBspInfo);
//	void DrawStaticBspNode( INT iNode, FSurfaceInfo& Surface) {};
//	void DrawStaticBspSurf( INT iSurf, FSurfaceInfo& Surface) {};

	// General operations
	DWORD FixPolyFlags( DWORD PolyFlags, UBOOL& UseMaskedStorage); // Inline defined in OpenGL.cpp
	void ApplySceneNodeHack( FSceneNode* Frame); // Inline defined in OpenGL.cpp


	// Draw Mode procs.
	void (UOpenGLRenderDevice::*m_pDrawComplexSurface)(FSceneNode*,FSurfaceInfo_DrawComplex&,FSurfaceFacet&);
	void (UOpenGLRenderDevice::*m_pDrawGouraudPolygon)(FSceneNode*,FSurfaceInfo_DrawGouraud&,FTransTexture**,INT);
	void (UOpenGLRenderDevice::*m_pDrawTile)(FSceneNode*,FSurfaceInfo_DrawTile&,FLOAT,FLOAT,FLOAT,FLOAT,FLOAT,FLOAT,FLOAT,FLOAT,FLOAT);
	void (UOpenGLRenderDevice::*m_pDraw3DLine)(FSceneNode*,FPlane&,DWORD,FVector,FVector);
	void (UOpenGLRenderDevice::*m_pDraw2DLine)(FSceneNode*,FPlane&,DWORD,FVector,FVector);
	void (UOpenGLRenderDevice::*m_pDraw2DPoint)(FSceneNode*,FPlane&,DWORD,FLOAT,FLOAT,FLOAT,FLOAT,FLOAT);
	void (UOpenGLRenderDevice::*m_pDrawGouraudTriangles)(const FSceneNode*,FSurfaceInfo_DrawGouraudTris&,FTransTexture* const,INT,DWORD);
	void (UOpenGLRenderDevice::*m_pSetSceneNode)(FSceneNode*);
	void (UOpenGLRenderDevice::*m_pFillScreen)(const FOpenGLTexture* Texture, const FPlane* Color, DWORD PolyFlags);
	void (UOpenGLRenderDevice::*m_pFlushDrawBuffers)(DWORD BuffersToFlush);
	void (UOpenGLRenderDevice::*m_pBlitFramebuffer)();

	// Hit testing
	void DrawComplexSurface_HitTesting(FSceneNode* Frame, FSurfaceInfo_DrawComplex& Surface, FSurfaceFacet& Facet);
	void DrawGouraudPolygon_HitTesting(FSceneNode* Frame, FSurfaceInfo_DrawGouraud& Surface, FTransTexture** Pts, INT NumPts);
	void DrawTile_HitTesting(FSceneNode* Frame, FSurfaceInfo_DrawTile& Surface, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, FLOAT Z);
	void Draw3DLine_HitTesting(FSceneNode* Frame, FPlane& Color, DWORD LineFlags, FVector P1, FVector P2);
	void Draw2DLine_HitTesting(FSceneNode* Frame, FPlane& Color, DWORD LineFlags, FVector P1, FVector P2);
	void Draw2DPoint_HitTesting(FSceneNode* Frame, FPlane& Color, DWORD LineFlags, FLOAT X1, FLOAT Y1, FLOAT X2, FLOAT Y2, FLOAT Z);
	// TODO: DrawGouraudTriangles
	void SetSceneNode_HitTesting(FSceneNode* Frame);

	// ARB
	void DrawComplexSurface_ARB(FSceneNode* Frame, FSurfaceInfo_DrawComplex& Surface, FSurfaceFacet& Facet);
	void DrawGouraudPolygon_ARB(FSceneNode* Frame, FSurfaceInfo_DrawGouraud& Surface, FTransTexture** Pts, INT NumPts);
	void DrawTile_ARB(FSceneNode* Frame, FSurfaceInfo_DrawTile& Surface, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, FLOAT Z);
	void Draw3DLine_ARB(FSceneNode* Frame, FPlane& Color, DWORD LineFlags, FVector P1, FVector P2);
	void Draw2DLine_ARB(FSceneNode* Frame, FPlane& Color, DWORD LineFlags, FVector P1, FVector P2);
	void Draw2DPoint_ARB(FSceneNode* Frame, FPlane& Color, DWORD LineFlags, FLOAT X1, FLOAT Y1, FLOAT X2, FLOAT Y2, FLOAT Z);
	void DrawGouraudTriangles_ARB(const FSceneNode* Frame, FSurfaceInfo_DrawGouraudTris& Surface, FTransTexture* const Pts, INT NumPts, DWORD DataFlags);
	void SetSceneNode_ARB(FSceneNode* Frame);
	void FillScreen_ARB(const FOpenGLTexture* Texture, const FPlane* Color, DWORD PolyFlags);
//	void BlitFramebuffer_ARB();

	// GLSL3
	void DrawComplexSurface_GLSL3(FSceneNode* Frame, FSurfaceInfo_DrawComplex& Surface, FSurfaceFacet& Facet);
	void DrawGouraudPolygon_GLSL3(FSceneNode* Frame, FSurfaceInfo_DrawGouraud& Surface, FTransTexture** Pts, INT NumPts);
	void DrawTile_GLSL3(FSceneNode* Frame, FSurfaceInfo_DrawTile& Surface, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, FLOAT Z);
	void Draw3DLine_GLSL3(FSceneNode* Frame, FPlane& Color, DWORD LineFlags, FVector P1, FVector P2);
	void Draw2DLine_GLSL3(FSceneNode* Frame, FPlane& Color, DWORD LineFlags, FVector P1, FVector P2);
	void Draw2DPoint_GLSL3(FSceneNode* Frame, FPlane& Color, DWORD LineFlags, FLOAT X1, FLOAT Y1, FLOAT X2, FLOAT Y2, FLOAT Z);
	void DrawGouraudTriangles_GLSL3(const FSceneNode* Frame, FSurfaceInfo_DrawGouraudTris& Surface, FTransTexture* const Pts, INT NumPts, DWORD DataFlags);
	void SetSceneNode_GLSL3(FSceneNode* Frame);
	void FillScreen_GLSL3(const FOpenGLTexture* Texture, const FPlane* Color, DWORD PolyFlags);
	void BlitFramebuffer_GLSL3();

	//
	// General internal states
	//
	INT NumClipPlanes;
	BYTE m_LightUVAtlas;
	BYTE m_FogUVAtlas;
	BYTE m_DGT;

	//
	// Draw buffering system
	//
	struct FDrawBuffer
	{
		GLuint               BufferId;
		DWORD                ActiveType;
		UBOOL                MultiBuffer;
		UBOOL                StreamBufferInit;

		FGL::DrawBuffer::FBase* Complex;
		FGL::DrawBuffer::FBase* ComplexStatic;
		FGL::DrawBuffer::FBase* Gouraud;
		FGL::DrawBuffer::FBase* Quad;
		FGL::DrawBuffer::FBase* Line;
		FGL::DrawBuffer::FBase* Fill;
		FGL::DrawBuffer::FBase* Decal;

		FGL::VertexBuffer::Base* GeneralBuffer[3];

		FGL::Draw::CommandChain Draw;
		FGL::Draw::CommandChain DrawDecals;

		~FDrawBuffer() { Destroy(); }
		void Destroy();

	};
	static FDrawBuffer DrawBuffer;

	FORCEINLINE void FlushDrawBuffers( DWORD BuffersToFlush=DRAWBUFFER_ALL)
	{
		if ( ((BuffersToFlush & DrawBuffer.ActiveType) != 0) && m_pFlushDrawBuffers )
			(this->*m_pFlushDrawBuffers)(BuffersToFlush);
	}

	bool FASTCALL SetDrawBuffering( DWORD NewDrawBufferType, bool NewSupportsMulti)
	{
		if ( !NewSupportsMulti || !DrawBuffer.MultiBuffer )
			FlushDrawBuffers( DrawBuffer.MultiBuffer ? DRAWBUFFER_ALL : ~NewDrawBufferType );
		if ( !(DrawBuffer.ActiveType & NewDrawBufferType) )
		{
			DrawBuffer.ActiveType |= NewDrawBufferType;
			DrawBuffer.MultiBuffer = NewSupportsMulti;
			return true;
		}
		return false;
	}

	// ARB DrawBuffer
	FProgramID FASTCALL MergeComplexSurfaceToDrawBuffer_ARB( const FGL::Draw::TextureList& TexDrawList, DWORD ZoneNumber);
	FProgramID FASTCALL MergeComplexSurfaceToVBODrawBuffer_ARB( const FGL::Draw::TextureList& TexDrawList, DWORD ZoneNumber);
	INT FASTCALL BufferStaticComplexSurfaceGeometry_VBO_ARB( FSurfaceFacet& Facet);

	void FlushDrawBuffer_ComplexSurface_ARB();
	void FlushDrawBuffer_ComplexSurfaceVBO_ARB();
	void FlushDrawBuffer_GouraudTriangles_ARB();
	void FlushDrawBuffer_Quad_ARB();
	void FlushDrawBuffer_Line_ARB();
	void FlushDrawBuffer_Decal_ARB();
	void FlushDrawBuffers_ARB( DWORD BuffersToFlush);

	// GLSL3 DrawBuffer
	void ExecDraw_ComplexSurface_GLSL3( FGL::Draw::Command* Draw);
	void ExecDraw_ComplexSurfaceMultiVolumetric_GLSL3( FGL::Draw::Command* Draw);
	void ExecDraw_GouraudTriangles_GLSL3( FGL::Draw::Command* Draw);
	void ExecDraw_Quad_GLSL3( FGL::Draw::Command* Draw);
	void ExecDraw_Line_GLSL3( FGL::Draw::Command* Draw);
	void ExecDraw_Decal_GLSL3( FGL::Draw::Command* Draw);
	void ExecDraws_GLSL3( DWORD Unused=0);


	// General DrawBuffer
	template<INT ArraySize> static FORCEINLINE bool CompareTextures( const FGL::Draw::TextureList& TexDrawList, const FPendingTexture (&PendingTextures)[ArraySize])
	{
		// Check Base, LightMap and FogMap only (others are assumed dependent on Base)
		if ( TexDrawList.TexRemaps[0] && TexDrawList.TexRemaps[0]->PoolIndex != PendingTextures[0].PoolID )
			return false;
		if ( (ArraySize > TMU_LightMap) && TexDrawList.TexRemaps[TMU_LightMap] && TexDrawList.TexRemaps[TMU_LightMap]->PoolIndex != PendingTextures[TMU_LightMap].PoolID )
			return false;
		if ( (ArraySize > TMU_FogMap) && TexDrawList.TexRemaps[TMU_FogMap] && TexDrawList.TexRemaps[TMU_FogMap]->PoolIndex != PendingTextures[TMU_FogMap].PoolID )
			return false;
		return true;
	}



	//
	// Framebuffer
	//
	INT    RequestedFramebufferWidth;
	INT    RequestedFramebufferHeight;
	UBOOL  RequestedFramebufferHighQuality;
	INT    FBO_Wait;
	UBOOL  MainFramebuffer_Locked;
	UBOOL  MainFramebuffer_Required;
	GLuint MainFramebuffer_FBO;
	FOpenGLTexture MainFramebuffer_Texture;
	GLuint MainFramebuffer_Depth;
	INT    MainFramebuffer_Width;
	INT    MainFramebuffer_Height;
	INT    MainFramebuffer_HighQuality;
	GLuint MainFramebuffer_FBO_MSAA;
	GLuint MainFramebuffer_Color_MSAA;
	GLuint MainFramebuffer_Depth_MSAA;
	INT    MainFramebuffer_Samples_MSAA;
	UBOOL  DebugUseGL3Resolve; // TODO: Remove once confirmed working in all platforms
	bool UpdateMainFramebuffer();
	void LockMainFramebuffer();
	void DestroyMainFramebuffer();
	void BlitMainFramebuffer();
	void CopyFramebuffer( GLuint Source, GLuint Target, GLint Width, GLint Height, bool InvalidateSrc);


	//
	// Texture sampler model
	//
	UBOOL m_SamplerInit;
	GLuint SamplerList[FGL::SamplerModel::SAMPLER_COMBINATIONS];
	GLuint SamplerBindings[TMU_EXTENDED_MAX];
	void UpdateSamplers( UBOOL& FlushTextures);
	void DestroySamplers();
	void SetDefaultSamplerState();
	void FASTCALL SetSampler( INT Multi, UBOOL Mips, UBOOL AlwaysSmooth, DWORD PolyFlags);
	void FASTCALL SetNoSampler( INT Multi);

	//
	// Texture upload system
	//
	bool FASTCALL UploadTexture( const FGL::Draw::TextureList& TexDrawList, INT TexIndex);

	//
	// Texture converter model
	//
	UBOOL m_FormatInfo;
	FTextureFormatInfo TextureFormatInfo[256];
	void UpdateTextureFormat( UBOOL& FlushTextures);
	void RegisterTextureFormat( BYTE Format, GLenum InternalFormat, GLenum SourceFormat=0, GLenum Type=0);
	void RegisterTextureConversion( BYTE Format, f_SoftwareConvert SoftwareConvert=nullptr);

	bool FASTCALL ConvertIdentity( FTextureUploadState& State, DWORD* QueryBytes);
	bool FASTCALL ConvertP8_RGBA8( FTextureUploadState& State, DWORD* QueryBytes);
	bool FASTCALL ConvertP8_RGB9_E5( FTextureUploadState& State, DWORD* QueryBytes);
	bool FASTCALL ConvertBGRA7_BGRA8( FTextureUploadState& State, DWORD* QueryBytes);

	//
	// Shader system
	//
	UBOOL m_ShaderColorCorrection;
	FProgramID FASTCALL GetProgramID( const FPendingTexture* Pending, DWORD Num);
	FProgramID FASTCALL GetProgramID( const FGL::Draw::TextureList& TexDrawList);
	FProgramID FASTCALL GetProgramID( DWORD PolyFlags);
	FLOAT FASTCALL GetAlphaTest( DWORD PolyFlags);

	//
	// Buffer system
	// TODO: Need flush tracker so other contexts bind UBO's again.
	//
	static GLuint bufferId_StaticFillScreenVBO;
	static GLuint bufferId_StaticGeometryVBO;
	static GLuint bufferId_GlobalRenderUBO;
	static GLuint bufferId_StaticBspUBO;
	static GLuint bufferId_TextureParamsUBO;
	static GLuint bufferId_csTextureScaleUBO;
	static FGL::VBO::FFill FillScreenData[4];
	       FGlobalRender_UBO        GlobalRenderData;
	static FStaticBsp_UBO           StaticBspData;
	static FTextureParams_UBO       TextureParamsData;
	static FComplexSurfaceScale_UBO csTextureScaleData;
	static TArray<FBspNodeVBOMapping> VBONodeMappings;

	void UpdateBuffers();
	static void DestroyBufferObjects();
	static void FlushStaticGeometry();
	UBOOL FASTCALL CanBufferStaticComplexSurfaceGeometry_VBO( FSurfaceInfo& Surface, FSurfaceFacet& Facet);
	static void UpdateTextureParamsUBO( INT Element=INDEX_NONE);
	static INT csGetTextureScaleIndexUBO( FGL::Draw::TextureList& TexList);
	static void csUpdateTextureScaleUBO();

	//
	// Matrix transformation system
	//
	enum ETransformationMode
	{
		MATRIX_OrthoProjection,
		MATRIX_Projection,
	};
	struct FSceneParams
	{
		DWORD   Mode;
		FLOAT   FovAngle;
		FLOAT   FX, FY;
		FCoords Coords;
	};
	FSceneParams SceneParams;

	void SetTransformationModeNoCheck( DWORD Mode);
	inline void SetTransformationMode( DWORD Mode, UBOOL Force=0)
	{
		if ( Force || (Mode != SceneParams.Mode) )
			SetTransformationModeNoCheck(Mode);
	}
	// Aliases
	inline void SetDefaultProjectionState()    { SetTransformationMode(MATRIX_Projection); }

	inline void FASTCALL SetBlend(DWORD PolyFlags) {
#ifdef UTGLR_RUNE_BUILD
		if (PolyFlags & PF_AlphaBlend) {
			if (!(PolyFlags & PF_Masked)) {
				PolyFlags |= PF_Occlude;
			}
			else {
				PolyFlags &= ~PF_Masked;
			}
		}
		else
#endif
		if (!(PolyFlags & (PF_Translucent | PF_Modulated | PF_Highlighted))) {
			PolyFlags |= PF_Occlude;
		}

		//Only check relevant blend flags
		DWORD blendFlags = PolyFlags & (PF_Translucent | PF_Modulated | PF_Invisible | PF_Occlude | PF_Masked | PF_Highlighted | PF_AlphaBlend | PF_NoZReject);
		if (m_curBlendFlags != blendFlags) {
			SetBlendNoCheck(blendFlags);
		}
	}
	void FASTCALL SetBlendNoCheck(DWORD blendFlags);

	inline bool UsingFBOMultisampling(void) {
		return MainFramebuffer_FBO ? (MainFramebuffer_FBO_MSAA != 0) : (m_initNumAASamples > 1);
	}

	inline void SetDefaultAAState(void) {
		if (m_defAAEnable != m_curAAEnable/* && !UsingFBOMultisampling()*/) {
			SetAAStateNoCheck(m_defAAEnable);
		}
	}
	inline void SetDisabledAAState(void) {
		if (m_curAAEnable && m_usingAA/* && !UsingFBOMultisampling()*/) {
			SetAAStateNoCheck(false);
		}
	}
	void FASTCALL SetAAStateNoCheck(bool AAEnable);
};

//
// Resolve a Program ID from Pending Textures array
//
inline FProgramID FASTCALL UOpenGLRenderDevice::GetProgramID( const FPendingTexture* Pending, DWORD Num)
{
	DWORD TextureBits = 0;
	for ( DWORD i=0; i<Num; i++)
		if ( Pending[i].PoolID >= 0 )
			TextureBits |= 1 << i;
	return FProgramID(TextureBits);
}

//
// Resolve a Program ID from a Texture Draw List
//
inline FProgramID FASTCALL UOpenGLRenderDevice::GetProgramID( const FGL::Draw::TextureList& TexDrawList)
{
	FProgramID ProgramID = GetProgramID(TexDrawList.PolyFlags[0]);
	for ( DWORD i=0; i<ARRAY_COUNT(TexDrawList.TexRemaps); i++)
		if ( TexDrawList.TexRemaps[i] != nullptr )
			ProgramID |= (1 << i);
	return ProgramID;
}

//
// Resolve a Program ID from PolyFlags
//
inline FProgramID FASTCALL UOpenGLRenderDevice::GetProgramID( DWORD PolyFlags)
{
	FProgramID ProgramID;

	// Color Correction
	if ( !(PolyFlags & PF_Modulated) )
	{
		if ( m_ShaderColorCorrection )
		{
			ProgramID |= FGL::Program::ColorCorrection;
			if ( PolyFlags & PF_Highlighted )
				ProgramID |= FGL::Program::ColorCorrectionAlpha;
		}
		if ( PolyFlags & PF_AlphaHack )
			ProgramID |= FGL::Program::AlphaHack;
	}

	// Alpha Test
	if ( PolyFlags & (PF_Masked|PF_AlphaBlend|PF_Highlighted) )
		ProgramID |= FGL::Program::AlphaTest;

	// DervMapping
	if ( PolyFlags & PF_Masked )
		ProgramID |= FGL::Program::DervMapped;

	// Gouraud shading
	if ( PolyFlags & PF_Gouraud )
	{
		ProgramID |= FGL::Program::Color0;
		if ( PolyFlags & PF_RenderFog )
			ProgramID |= FGL::Program::Color1;
	}
	else if ( PolyFlags & PF_FlatShaded /*==PF_RenderFog*/ )
		ProgramID |= FGL::Program::Color0;

	// NoNearZ
	if ( PolyFlags & PF_NoNearZ )
		ProgramID |= FGL::Program::NoNearZ;

	return ProgramID;
}

inline FLOAT FASTCALL UOpenGLRenderDevice::GetAlphaTest( DWORD PolyFlags)
{
	if ( PolyFlags & (PF_AlphaBlend|PF_Highlighted) )
		return 0.01f;
	if ( PolyFlags & PF_Masked )
		return (m_SmoothMasking != 0) ? 0.2f : 0.5f;
	return 0.0f;
}


#if __STATIC_LINK
#define AUTO_INITIALIZE_REGISTRANTS_OPENGLDRV \
	UOpenGLRenderDevice::StaticClass();
	
#endif

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
