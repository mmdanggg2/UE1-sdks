/*=============================================================================
	D3D9.cpp: Unreal Direct3D9 support implementation for Windows.
	Copyright 1999 Epic Games, Inc. All Rights Reserved.

	OpenGL renderer by Daniel Vogel <vogel@lokigames.com>
	Loki Software, Inc.

	Other URenderDevice subclasses include:
	* USoftwareRenderDevice: Software renderer.
	* UGlideRenderDevice: 3dfx Glide renderer.
	* UDirect3DRenderDevice: Direct3D renderer.
	* UD3D9RenderDevice: Direct3D9 renderer.

	Revision history:
	* Created by Daniel Vogel based on XMesaGLDrv
	* Changes (John Fulmer, Jeroen Janssen)
	* Major changes (Daniel Vogel)
	* Ported back to Win32 (Fredrik Gustafsson)
	* Unification and addition of vertex arrays (Daniel Vogel)
	* Actor triangle caching (Steve Sinclair)
	* One pass fogging (Daniel Vogel)
	* Windows gamma support (Daniel Vogel)
	* 2X blending support (Daniel Vogel)
	* Better detail texture handling (Daniel Vogel)
	* Scaleability (Daniel Vogel)
	* Texture LOD bias (Daniel Vogel)
	* RefreshRate support on Windows (Jason Dick)
	* Finer control over gamma (Daniel Vogel)
	* (NOT ALWAYS) Fixed Windows bitdepth switching (Daniel Vogel)

	* Various modifications and additions by Chris Dohnal
	* Initial TruForm based on TruForm renderer modifications by NitroGL
	* Additional TruForm and Deus Ex updates by Leonhard Gruenschloss
	* Various modifications and additions by Smirftsch / Oldunreal


	UseTrilinear	whether to use trilinear filtering
	UseS3TC			whether to use compressed textures
	MaxAnisotropy	maximum level of anisotropy used
	MaxTMUnits		maximum number of TMUs UT will try to use
	LODBias			texture lod bias
	RefreshRate		requested refresh rate (Windows only)
	GammaOffset		offset for the gamma correction


TODO:
	- DOCUMENTATION!!! (especially all subtle assumptions)

=============================================================================*/

#include "D3D9Drv.h"
#include "D3D9.h"

#ifdef _WIN32
#include <mmsystem.h>
#endif


/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

#ifdef UTD3D9R_USE_DEBUG_D3D9_DLL
static const char *g_d3d9DllName = "d3d9d.dll";
#else
static const char *g_d3d9DllName = "d3d9.dll";
#endif

static const TCHAR *g_pSection = TEXT("D3D9Drv.D3D9RenderDevice");

static UBOOL g_bSupportsBumpMap = FALSE;

/*-----------------------------------------------------------------------------
	Vertex programs.
-----------------------------------------------------------------------------*/

//Vertex shader definitions
///////////////////////////

//Vertex shader output registers
// o0 - Transformed position
// o1 - Primary color
// o2 - Secondary color
// o3 - Normal
// o4 - TexCoord 0
// o5 - TexCoord 1
// o6 - TexCoord 2
// o7 - TexCoord 3
// o8 - TexCoord 4 / Position

//Vertex shader global constants
// [c0 - c3] - Projection matrix
// c4 - Complex surface XAxis and UDot
// c5 - Complex surface YAxis and VDot
// [c6 - c9] - Up to 4 texture [UPan, VPan, UMult, VMult]

#if 0
static const char *g_tempShaderString =
	"vs_3_0\n"

	"dcl_position v0\n"
	"dcl_color v5\n"
	"dcl_texcoord0 v7\n"

	"dcl_position o0.xyzw\n"
	"dcl_color o1.rgba\n"
	"dcl_texcoord0 o4.xy\n"

	"mov o4.xy, v7\n"

	"mov o1, v5\n"

	"m4x4 o0, v0, c0\n"
;
#endif
static const DWORD g_vpDefaultRenderingState[] = {
	0xFFFE0300, 0x0200001F, 0x80000000, 0x900F0000, 0x0200001F, 0x8000000A, 0x900F0005, 0x0200001F,
	0x80000005, 0x900F0007, 0x0200001F, 0x80000000, 0xE00F0000, 0x0200001F, 0x8000000A, 0xE00F0001,
	0x0200001F, 0x80000005, 0xE0030004, 0x02000001, 0xE0030004, 0x90E40007, 0x02000001, 0xE00F0001,
	0x90E40005, 0x03000014, 0xE00F0000, 0x90E40000, 0xA0E40000, 0x0000FFFF
};

#if 0
static const char *g_tempShaderString =
	"vs_3_0\n"

	"dcl_position v0\n"
	"dcl_color v5\n"
	"dcl_color1 v6\n"
	"dcl_texcoord0 v7\n"

	"dcl_position o0.xyzw\n"
	"dcl_color o1.rgba\n"
	"dcl_color1 o2.rgba\n"
	"dcl_texcoord0 o4.xy\n"

	"mov o4.xy, v7\n"

	"mov o1, v5\n"
	"mov o2, v6\n"

	"m4x4 o0, v0, c0\n"
;
#endif
static const DWORD g_vpDefaultRenderingStateWithFog[] = {
	0xFFFE0300, 0x0200001F, 0x80000000, 0x900F0000, 0x0200001F, 0x8000000A, 0x900F0005, 0x0200001F,
	0x8001000A, 0x900F0006, 0x0200001F, 0x80000005, 0x900F0007, 0x0200001F, 0x80000000, 0xE00F0000,
	0x0200001F, 0x8000000A, 0xE00F0001, 0x0200001F, 0x8001000A, 0xE00F0002, 0x0200001F, 0x80000005,
	0xE0030004, 0x02000001, 0xE0030004, 0x90E40007, 0x02000001, 0xE00F0001, 0x90E40005, 0x02000001,
	0xE00F0002, 0x90E40006, 0x03000014, 0xE00F0000, 0x90E40000, 0xA0E40000, 0x0000FFFF
};

#if 0
static const char *g_tempShaderString =
	"vs_3_0\n"

	"dcl_position v0\n"
	"dcl_color v5\n"
	"dcl_texcoord0 v7\n"

	"dcl_position o0.xyzw\n"
	"dcl_color o1.rgba\n"
	"dcl_texcoord0 o4.xy\n"
	"dcl_texcoord4 o8.xyzw\n"

	"mov o4.xy, v7\n"

	"mov o1, v5\n"

	"m4x4 o0, v0, c0\n"
	"mov o8, v0\n"
;
#endif
static const DWORD g_vpDefaultRenderingStateWithLinearFog[] = {
	0xFFFE0300, 0x0200001F, 0x80000000, 0x900F0000, 0x0200001F, 0x8000000A, 0x900F0005, 0x0200001F,
	0x80000005, 0x900F0007, 0x0200001F, 0x80000000, 0xE00F0000, 0x0200001F, 0x8000000A, 0xE00F0001,
	0x0200001F, 0x80000005, 0xE0030004, 0x0200001F, 0x80040005, 0xE00F0008, 0x02000001, 0xE0030004,
	0x90E40007, 0x02000001, 0xE00F0001, 0x90E40005, 0x03000014, 0xE00F0000, 0x90E40000, 0xA0E40000,
	0x02000001, 0xE00F0008, 0x90E40000, 0x0000FFFF
};

#if 0
static const char *g_tempShaderString =
	"vs_3_0\n"

	"dcl_position v0\n"
	"dcl_color v5\n"

	"dcl_position o0.xyzw\n"
	"dcl_color o1.rgba\n"
	"dcl_texcoord0 o4.xy\n"

	"dp3 r0.x, v0, c4\n"
	"dp3 r0.y, v0, c5\n"
	"add r0.x, r0.x, -c4.w\n"
	"add r0.y, r0.y, -c5.w\n"

	"add r1.xy, r0.xy, -c6.xy\n"
	"mul o4.xy, r1.xy, c6.zw\n"

	"mov o1, v5\n"

	"m4x4 o0, v0, c0\n"
;
#endif
static const DWORD g_vpComplexSurfaceSingleTexture[] = {
	0xFFFE0300, 0x0200001F, 0x80000000, 0x900F0000, 0x0200001F, 0x8000000A, 0x900F0005, 0x0200001F,
	0x80000000, 0xE00F0000, 0x0200001F, 0x8000000A, 0xE00F0001, 0x0200001F, 0x80000005, 0xE0030004,
	0x03000008, 0x80010000, 0x90E40000, 0xA0E40004, 0x03000008, 0x80020000, 0x90E40000, 0xA0E40005,
	0x03000002, 0x80010000, 0x80000000, 0xA1FF0004, 0x03000002, 0x80020000, 0x80550000, 0xA1FF0005,
	0x03000002, 0x80030001, 0x80540000, 0xA1540006, 0x03000005, 0xE0030004, 0x80540001, 0xA0FE0006,
	0x02000001, 0xE00F0001, 0x90E40005, 0x03000014, 0xE00F0000, 0x90E40000, 0xA0E40000, 0x0000FFFF
};

#if 0
static const char *g_tempShaderString =
	"vs_3_0\n"

	"dcl_position v0\n"
	"dcl_color v5\n"

	"dcl_position o0.xyzw\n"
	"dcl_color o1.rgba\n"
	"dcl_texcoord0 o4.xy\n"
	"dcl_texcoord1 o5.xy\n"

	"dp3 r0.x, v0, c4\n"
	"dp3 r0.y, v0, c5\n"
	"add r0.x, r0.x, -c4.w\n"
	"add r0.y, r0.y, -c5.w\n"

	"add r1.xy, r0.xy, -c6.xy\n"
	"mul o4.xy, r1.xy, c6.zw\n"

	"add r1.xy, r0.xy, -c7.xy\n"
	"mul o5.xy, r1.xy, c7.zw\n"

	"mov o1, v5\n"

	"m4x4 o0, v0, c0\n"
;
#endif
static const DWORD g_vpComplexSurfaceDualTexture[] = {
	0xFFFE0300, 0x0200001F, 0x80000000, 0x900F0000, 0x0200001F, 0x8000000A, 0x900F0005, 0x0200001F,
	0x80000000, 0xE00F0000, 0x0200001F, 0x8000000A, 0xE00F0001, 0x0200001F, 0x80000005, 0xE0030004,
	0x0200001F, 0x80010005, 0xE0030005, 0x03000008, 0x80010000, 0x90E40000, 0xA0E40004, 0x03000008,
	0x80020000, 0x90E40000, 0xA0E40005, 0x03000002, 0x80010000, 0x80000000, 0xA1FF0004, 0x03000002,
	0x80020000, 0x80550000, 0xA1FF0005, 0x03000002, 0x80030001, 0x80540000, 0xA1540006, 0x03000005,
	0xE0030004, 0x80540001, 0xA0FE0006, 0x03000002, 0x80030001, 0x80540000, 0xA1540007, 0x03000005,
	0xE0030005, 0x80540001, 0xA0FE0007, 0x02000001, 0xE00F0001, 0x90E40005, 0x03000014, 0xE00F0000,
	0x90E40000, 0xA0E40000, 0x0000FFFF
};

#if 0
static const char *g_tempShaderString =
	"vs_3_0\n"

	"dcl_position v0\n"
	"dcl_color v5\n"

	"dcl_position o0.xyzw\n"
	"dcl_color o1.rgba\n"
	"dcl_texcoord0 o4.xy\n"
	"dcl_texcoord1 o5.xy\n"
	"dcl_texcoord2 o6.xy\n"

	"dp3 r0.x, v0, c4\n"
	"dp3 r0.y, v0, c5\n"
	"add r0.x, r0.x, -c4.w\n"
	"add r0.y, r0.y, -c5.w\n"

	"add r1.xy, r0.xy, -c6.xy\n"
	"mul o4.xy, r1.xy, c6.zw\n"

	"add r1.xy, r0.xy, -c7.xy\n"
	"mul o5.xy, r1.xy, c7.zw\n"

	"add r1.xy, r0.xy, -c8.xy\n"
	"mul o6.xy, r1.xy, c8.zw\n"

	"mov o1, v5\n"

	"m4x4 o0, v0, c0\n"
;
#endif
static const DWORD g_vpComplexSurfaceTripleTexture[] = {
	0xFFFE0300, 0x0200001F, 0x80000000, 0x900F0000, 0x0200001F, 0x8000000A, 0x900F0005, 0x0200001F,
	0x80000000, 0xE00F0000, 0x0200001F, 0x8000000A, 0xE00F0001, 0x0200001F, 0x80000005, 0xE0030004,
	0x0200001F, 0x80010005, 0xE0030005, 0x0200001F, 0x80020005, 0xE0030006, 0x03000008, 0x80010000,
	0x90E40000, 0xA0E40004, 0x03000008, 0x80020000, 0x90E40000, 0xA0E40005, 0x03000002, 0x80010000,
	0x80000000, 0xA1FF0004, 0x03000002, 0x80020000, 0x80550000, 0xA1FF0005, 0x03000002, 0x80030001,
	0x80540000, 0xA1540006, 0x03000005, 0xE0030004, 0x80540001, 0xA0FE0006, 0x03000002, 0x80030001,
	0x80540000, 0xA1540007, 0x03000005, 0xE0030005, 0x80540001, 0xA0FE0007, 0x03000002, 0x80030001,
	0x80540000, 0xA1540008, 0x03000005, 0xE0030006, 0x80540001, 0xA0FE0008, 0x02000001, 0xE00F0001,
	0x90E40005, 0x03000014, 0xE00F0000, 0x90E40000, 0xA0E40000, 0x0000FFFF
};

#if 0
static const char *g_tempShaderString =
	"vs_3_0\n"

	"dcl_position v0\n"
	"dcl_color v5\n"

	"dcl_position o0.xyzw\n"
	"dcl_color o1.rgba\n"
	"dcl_texcoord0 o4.xy\n"
	"dcl_texcoord1 o5.xy\n"
	"dcl_texcoord2 o6.xy\n"
	"dcl_texcoord3 o7.xy\n"

	"dp3 r0.x, v0, c4\n"
	"dp3 r0.y, v0, c5\n"
	"add r0.x, r0.x, -c4.w\n"
	"add r0.y, r0.y, -c5.w\n"

	"add r1.xy, r0.xy, -c6.xy\n"
	"mul o4.xy, r1.xy, c6.zw\n"

	"add r1.xy, r0.xy, -c7.xy\n"
	"mul o5.xy, r1.xy, c7.zw\n"

	"add r1.xy, r0.xy, -c8.xy\n"
	"mul o6.xy, r1.xy, c8.zw\n"

	"add r1.xy, r0.xy, -c9.xy\n"
	"mul o7.xy, r1.xy, c9.zw\n"

	"mov o1, v5\n"

	"m4x4 o0, v0, c0\n"
;
#endif
static const DWORD g_vpComplexSurfaceQuadTexture[] = {
	0xFFFE0300, 0x0200001F, 0x80000000, 0x900F0000, 0x0200001F, 0x8000000A, 0x900F0005, 0x0200001F,
	0x80000000, 0xE00F0000, 0x0200001F, 0x8000000A, 0xE00F0001, 0x0200001F, 0x80000005, 0xE0030004,
	0x0200001F, 0x80010005, 0xE0030005, 0x0200001F, 0x80020005, 0xE0030006, 0x0200001F, 0x80030005,
	0xE0030007, 0x03000008, 0x80010000, 0x90E40000, 0xA0E40004, 0x03000008, 0x80020000, 0x90E40000,
	0xA0E40005, 0x03000002, 0x80010000, 0x80000000, 0xA1FF0004, 0x03000002, 0x80020000, 0x80550000,
	0xA1FF0005, 0x03000002, 0x80030001, 0x80540000, 0xA1540006, 0x03000005, 0xE0030004, 0x80540001,
	0xA0FE0006, 0x03000002, 0x80030001, 0x80540000, 0xA1540007, 0x03000005, 0xE0030005, 0x80540001,
	0xA0FE0007, 0x03000002, 0x80030001, 0x80540000, 0xA1540008, 0x03000005, 0xE0030006, 0x80540001,
	0xA0FE0008, 0x03000002, 0x80030001, 0x80540000, 0xA1540009, 0x03000005, 0xE0030007, 0x80540001,
	0xA0FE0009, 0x02000001, 0xE00F0001, 0x90E40005, 0x03000014, 0xE00F0000, 0x90E40000, 0xA0E40000,
	0x0000FFFF
};

#if 0
static const char *g_tempShaderString =
	"vs_3_0\n"

	"dcl_position v0\n"
	"dcl_color v5\n"
	"dcl_texcoord4 v8\n"

	"dcl_position o0.xyzw\n"
	"dcl_color o1.rgba\n"
	"dcl_texcoord0 o4.xyzw\n"
	"dcl_texcoord4 o8.xyzw\n"

	"def c10, 4.223f, 4.223f, 0.0f, 1.0f\n"

	"dp3 r0.x, v0, c4\n"
	"dp3 r0.y, v0, c5\n"
	"add r0.x, r0.x, -c4.w\n"
	"add r0.y, r0.y, -c5.w\n"

	"add r1.xy, r0.xy, -c6.xy\n"
	"mul r1.xy, r1.xy, c6.zw\n"
	"mul o4.xyzw, r1.xyxy, c10.wwxy\n"

	"mov o1, v5\n"

	"m4x4 o0, v0, c0\n"
	"mov o8, v0\n"
;
#endif
static const DWORD g_vpDetailTexture[] = {
	0xFFFE0300, 0x0200001F, 0x80000000, 0x900F0000, 0x0200001F, 0x8000000A, 0x900F0005, 0x0200001F,
	0x80040005, 0x900F0008, 0x0200001F, 0x80000000, 0xE00F0000, 0x0200001F, 0x8000000A, 0xE00F0001,
	0x0200001F, 0x80000005, 0xE00F0004, 0x0200001F, 0x80040005, 0xE00F0008, 0x05000051, 0xA00F000A,
	0x408722D1, 0x408722D1, 0x00000000, 0x3F800000, 0x03000008, 0x80010000, 0x90E40000, 0xA0E40004,
	0x03000008, 0x80020000, 0x90E40000, 0xA0E40005, 0x03000002, 0x80010000, 0x80000000, 0xA1FF0004,
	0x03000002, 0x80020000, 0x80550000, 0xA1FF0005, 0x03000002, 0x80030001, 0x80540000, 0xA1540006,
	0x03000005, 0x80030001, 0x80540001, 0xA0FE0006, 0x03000005, 0xE00F0004, 0x80440001, 0xA04F000A,
	0x02000001, 0xE00F0001, 0x90E40005, 0x03000014, 0xE00F0000, 0x90E40000, 0xA0E40000, 0x02000001,
	0xE00F0008, 0x90E40000, 0x0000FFFF
};

#if 0
static const char *g_tempShaderString =
	"vs_3_0\n"

	"dcl_position v0\n"
	"dcl_color v5\n"
	"dcl_texcoord4 v8\n"

	"dcl_position o0.xyzw\n"
	"dcl_color o1.rgba\n"
	"dcl_texcoord0 o4.xy\n"
	"dcl_texcoord1 o5.xyzw\n"
	"dcl_texcoord4 o8.xyzw\n"

	"def c10, 4.223f, 4.223f, 0.0f, 1.0f\n"

	"dp3 r0.x, v0, c4\n"
	"dp3 r0.y, v0, c5\n"
	"add r0.x, r0.x, -c4.w\n"
	"add r0.y, r0.y, -c5.w\n"

	"add r1.xy, r0.xy, -c6.xy\n"
	"mul o4.xy, r1.xy, c6.zw\n"

	"add r1.xy, r0.xy, -c7.xy\n"
	"mul r1.xy, r1.xy, c7.zw\n"
	"mul o5.xyzw, r1.xyxy, c10.wwxy\n"

	"mov o1, v5\n"

	"m4x4 o0, v0, c0\n"
	"mov o8, v0\n"
;
#endif
static const DWORD g_vpComplexSurfaceSingleTextureAndDetailTexture[] = {
	0xFFFE0300, 0x0200001F, 0x80000000, 0x900F0000, 0x0200001F, 0x8000000A, 0x900F0005, 0x0200001F,
	0x80040005, 0x900F0008, 0x0200001F, 0x80000000, 0xE00F0000, 0x0200001F, 0x8000000A, 0xE00F0001,
	0x0200001F, 0x80000005, 0xE0030004, 0x0200001F, 0x80010005, 0xE00F0005, 0x0200001F, 0x80040005,
	0xE00F0008, 0x05000051, 0xA00F000A, 0x408722D1, 0x408722D1, 0x00000000, 0x3F800000, 0x03000008,
	0x80010000, 0x90E40000, 0xA0E40004, 0x03000008, 0x80020000, 0x90E40000, 0xA0E40005, 0x03000002,
	0x80010000, 0x80000000, 0xA1FF0004, 0x03000002, 0x80020000, 0x80550000, 0xA1FF0005, 0x03000002,
	0x80030001, 0x80540000, 0xA1540006, 0x03000005, 0xE0030004, 0x80540001, 0xA0FE0006, 0x03000002,
	0x80030001, 0x80540000, 0xA1540007, 0x03000005, 0x80030001, 0x80540001, 0xA0FE0007, 0x03000005,
	0xE00F0005, 0x80440001, 0xA04F000A, 0x02000001, 0xE00F0001, 0x90E40005, 0x03000014, 0xE00F0000,
	0x90E40000, 0xA0E40000, 0x02000001, 0xE00F0008, 0x90E40000, 0x0000FFFF
};

#if 0
static const char *g_tempShaderString =
	"vs_3_0\n"

	"dcl_position v0\n"
	"dcl_color v5\n"
	"dcl_texcoord4 v8\n"

	"dcl_position o0.xyzw\n"
	"dcl_color o1.rgba\n"
	"dcl_texcoord0 o4.xy\n"
	"dcl_texcoord1 o5.xy\n"
	"dcl_texcoord2 o6.xyzw\n"
	"dcl_texcoord4 o8.xyzw\n"

	"def c10, 4.223f, 4.223f, 0.0f, 1.0f\n"

	"dp3 r0.x, v0, c4\n"
	"dp3 r0.y, v0, c5\n"
	"add r0.x, r0.x, -c4.w\n"
	"add r0.y, r0.y, -c5.w\n"

	"add r1.xy, r0.xy, -c6.xy\n"
	"mul o4.xy, r1.xy, c6.zw\n"

	"add r1.xy, r0.xy, -c7.xy\n"
	"mul o5.xy, r1.xy, c7.zw\n"

	"add r1.xy, r0.xy, -c8.xy\n"
	"mul r1.xy, r1.xy, c8.zw\n"
	"mul o6.xyzw, r1.xyxy, c10.wwxy\n"

	"mov o1, v5\n"

	"m4x4 o0, v0, c0\n"
	"mov o8, v0\n"
;
#endif
static const DWORD g_vpComplexSurfaceDualTextureAndDetailTexture[] = {
	0xFFFE0300, 0x0200001F, 0x80000000, 0x900F0000, 0x0200001F, 0x8000000A, 0x900F0005, 0x0200001F,
	0x80040005, 0x900F0008, 0x0200001F, 0x80000000, 0xE00F0000, 0x0200001F, 0x8000000A, 0xE00F0001,
	0x0200001F, 0x80000005, 0xE0030004, 0x0200001F, 0x80010005, 0xE0030005, 0x0200001F, 0x80020005,
	0xE00F0006, 0x0200001F, 0x80040005, 0xE00F0008, 0x05000051, 0xA00F000A, 0x408722D1, 0x408722D1,
	0x00000000, 0x3F800000, 0x03000008, 0x80010000, 0x90E40000, 0xA0E40004, 0x03000008, 0x80020000,
	0x90E40000, 0xA0E40005, 0x03000002, 0x80010000, 0x80000000, 0xA1FF0004, 0x03000002, 0x80020000,
	0x80550000, 0xA1FF0005, 0x03000002, 0x80030001, 0x80540000, 0xA1540006, 0x03000005, 0xE0030004,
	0x80540001, 0xA0FE0006, 0x03000002, 0x80030001, 0x80540000, 0xA1540007, 0x03000005, 0xE0030005,
	0x80540001, 0xA0FE0007, 0x03000002, 0x80030001, 0x80540000, 0xA1540008, 0x03000005, 0x80030001,
	0x80540001, 0xA0FE0008, 0x03000005, 0xE00F0006, 0x80440001, 0xA04F000A, 0x02000001, 0xE00F0001,
	0x90E40005, 0x03000014, 0xE00F0000, 0x90E40000, 0xA0E40000, 0x02000001, 0xE00F0008, 0x90E40000,
	0x0000FFFF
};


//Stream definitions
////////////////////

static const D3DVERTEXELEMENT9 g_oneColorStreamDef[] = {
	{ 0, 0,  D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,	0 },
	{ 0, 12, D3DDECLTYPE_D3DCOLOR,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,		0 },
	D3DDECL_END()
};

static const D3DVERTEXELEMENT9 g_standardSingleTextureStreamDef[] = {
	{ 0, 0,  D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,	0 },
	{ 0, 12, D3DDECLTYPE_D3DCOLOR,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,		0 },
	{ 2, 0,  D3DDECLTYPE_FLOAT2,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,	0 },
	D3DDECL_END()
};

static const D3DVERTEXELEMENT9 g_standardDoubleTextureStreamDef[] = {
	{ 0, 0,  D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,	0 },
	{ 0, 12, D3DDECLTYPE_D3DCOLOR,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,		0 },
	{ 2, 0,  D3DDECLTYPE_FLOAT2,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,	0 },
	{ 3, 0,  D3DDECLTYPE_FLOAT2,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,	1 },
	D3DDECL_END()
};

static const D3DVERTEXELEMENT9 g_standardTripleTextureStreamDef[] = {
	{ 0, 0,  D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,	0 },
	{ 0, 12, D3DDECLTYPE_D3DCOLOR,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,		0 },
	{ 2, 0,  D3DDECLTYPE_FLOAT2,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,	0 },
	{ 3, 0,  D3DDECLTYPE_FLOAT2,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,	1 },
	{ 4, 0,  D3DDECLTYPE_FLOAT2,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,	2 },
	D3DDECL_END()
};

static const D3DVERTEXELEMENT9 g_standardQuadTextureStreamDef[] = {
	{ 0, 0,  D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,	0 },
	{ 0, 12, D3DDECLTYPE_D3DCOLOR,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,		0 },
	{ 2, 0,  D3DDECLTYPE_FLOAT2,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,	0 },
	{ 3, 0,  D3DDECLTYPE_FLOAT2,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,	1 },
	{ 4, 0,  D3DDECLTYPE_FLOAT2,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,	2 },
	{ 5, 0,  D3DDECLTYPE_FLOAT2,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,	3 },
	D3DDECL_END()
};

static const D3DVERTEXELEMENT9 *g_standardNTextureStreamDefs[MAX_TMUNITS] = {
	g_standardSingleTextureStreamDef,
	g_standardDoubleTextureStreamDef,
	g_standardTripleTextureStreamDef,
	g_standardQuadTextureStreamDef
};

static const D3DVERTEXELEMENT9 g_twoColorSingleTextureStreamDef[] = {
	{ 0, 0,  D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,	0 },
	{ 0, 12, D3DDECLTYPE_D3DCOLOR,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,		0 },
	{ 1, 0,  D3DDECLTYPE_D3DCOLOR,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,		1 },
	{ 2, 0,  D3DDECLTYPE_FLOAT2,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,	0 },
	D3DDECL_END()
};


/*-----------------------------------------------------------------------------
	Fragment programs.
-----------------------------------------------------------------------------*/

//Pixel shader definitions
///////////////////////////

//Pixel shader input registers
// v0 - Unused
// v1 - Primary color
// v2 - Secondary color
// v3 - Normal
// v4 - TexCoord 0
// v5 - TexCoord 1
// v6 - TexCoord 2
// v7 - TexCoord 3
// v8 - TexCoord 4 / Position

//Pixel shader global constants
// c0.x - Alpha test level
// c0.g - Unused
// c0.b - Unused
// c0.a - Lightmap blend scale factor
// c3   - Linear fog color
// c4.x - Linear fog [1.0 / (end - start)]
// c4.y - Linear fog [end / (end - start)]

#if 0
static const char *g_tempShaderString =
	"ps_3_0\n"

	"dcl_color v1.rgba\n"
	"dcl_texcoord0 v4.xy\n"
	"dcl_2d s0\n"

	"texld r0, v4, s0\n"

	"mul r0, r0, v1\n"

	"sub r4, r0.aaaa, c0.xxxx\n"
	"texkill r4\n"

	"mov oC0, r0\n"
;
#endif
static const DWORD g_fpDefaultRenderingState[] = {
	0xFFFF0300, 0x0200001F, 0x8000000A, 0x900F0001, 0x0200001F, 0x80000005, 0x90030004, 0x0200001F,
	0x90000000, 0xA00F0800, 0x03000042, 0x800F0000, 0x90E40004, 0xA0E40800, 0x03000005, 0x800F0000,
	0x80E40000, 0x90E40001, 0x03000002, 0x800F0004, 0x80FF0000, 0xA1000000, 0x01000041, 0x800F0004,
	0x02000001, 0x800F0800, 0x80E40000, 0x0000FFFF
};

#if 0
static const char *g_tempShaderString =
	"ps_3_0\n"

	"dcl_color v1.rgba\n"
	"dcl_color1 v2.rgba\n"
	"dcl_texcoord0 v4.xy\n"
	"dcl_2d s0\n"

	"texld r0, v4, s0\n"

	"mul r0, r0, v1\n"
	"add r0.rgb, r0, v2\n"

	"sub r4, r0.aaaa, c0.xxxx\n"
	"texkill r4\n"

	"mov oC0, r0\n"
;
#endif
static const DWORD g_fpDefaultRenderingStateWithFog[] = {
	0xFFFF0300, 0x0200001F, 0x8000000A, 0x900F0001, 0x0200001F, 0x8001000A, 0x900F0002, 0x0200001F,
	0x80000005, 0x90030004, 0x0200001F, 0x90000000, 0xA00F0800, 0x03000042, 0x800F0000, 0x90E40004,
	0xA0E40800, 0x03000005, 0x800F0000, 0x80E40000, 0x90E40001, 0x03000002, 0x80070000, 0x80E40000,
	0x90E40002, 0x03000002, 0x800F0004, 0x80FF0000, 0xA1000000, 0x01000041, 0x800F0004, 0x02000001,
	0x800F0800, 0x80E40000, 0x0000FFFF
};

#if 0
static const char *g_tempShaderString =
	"ps_3_0\n"

	"dcl_color v1.rgba\n"
	"dcl_texcoord0 v4.xy\n"
	"dcl_2d s0\n"
	"dcl_texcoord4 v8.xyzw\n"

	"texld r0, v4, s0\n"

	"mul r0, r0, v1\n"

	"sub r4, r0.aaaa, c0.xxxx\n"
	"texkill r4\n"

	"mad_sat r1.x, -v8.z, c4.x, c4.y\n"
	"lrp r0.rgb, r1.xxxx, r0, c3\n"

	"mov oC0, r0\n"
;
#endif
static const DWORD g_fpDefaultRenderingStateWithLinearFog[] = {
	0xFFFF0300, 0x0200001F, 0x8000000A, 0x900F0001, 0x0200001F, 0x80000005, 0x90030004, 0x0200001F,
	0x90000000, 0xA00F0800, 0x0200001F, 0x80040005, 0x900F0008, 0x03000042, 0x800F0000, 0x90E40004,
	0xA0E40800, 0x03000005, 0x800F0000, 0x80E40000, 0x90E40001, 0x03000002, 0x800F0004, 0x80FF0000,
	0xA1000000, 0x01000041, 0x800F0004, 0x04000004, 0x80110001, 0x91AA0008, 0xA0000004, 0xA0550004,
	0x04000012, 0x80070000, 0x80000001, 0x80E40000, 0xA0E40003, 0x02000001, 0x800F0800, 0x80E40000,
	0x0000FFFF
};

#if 0
static const char *g_tempShaderString =
	"ps_3_0\n"

	"dcl_texcoord0 v4.xy\n"
	"dcl_2d s0\n"

	"texld r0, v4, s0\n"

	"sub r4, r0.aaaa, c0.xxxx\n"
	"texkill r4\n"

	"mov oC0, r0\n"
;
#endif
static const DWORD g_fpComplexSurfaceSingleTexture[] = {
	0xFFFF0300, 0x0200001F, 0x80000005, 0x90030004, 0x0200001F, 0x90000000, 0xA00F0800, 0x03000042,
	0x800F0000, 0x90E40004, 0xA0E40800, 0x03000002, 0x800F0004, 0x80FF0000, 0xA1000000, 0x01000041,
	0x800F0004, 0x02000001, 0x800F0800, 0x80E40000, 0x0000FFFF
};

#if 0
static const char *g_tempShaderString =
	"ps_3_0\n"

	"dcl_texcoord0 v4.xy\n"
	"dcl_2d s0\n"
	"dcl_texcoord1 v5.xy\n"
	"dcl_2d s1\n"

	"texld r0, v4, s0\n"
	"texld r1, v5, s1\n"

	"mul r0, r0, r1\n"
	"mul r0.rgb, r0, c0.aaaa\n"

	"sub r4, r0.aaaa, c0.xxxx\n"
	"texkill r4\n"

	"mov oC0, r0\n"
;
#endif
static const DWORD g_fpComplexSurfaceDualTextureModulated[] = {
	0xFFFF0300, 0x0200001F, 0x80000005, 0x90030004, 0x0200001F, 0x90000000, 0xA00F0800, 0x0200001F,
	0x80010005, 0x90030005, 0x0200001F, 0x90000000, 0xA00F0801, 0x03000042, 0x800F0000, 0x90E40004,
	0xA0E40800, 0x03000042, 0x800F0001, 0x90E40005, 0xA0E40801, 0x03000005, 0x800F0000, 0x80E40000,
	0x80E40001, 0x03000005, 0x80070000, 0x80E40000, 0xA0FF0000, 0x03000002, 0x800F0004, 0x80FF0000,
	0xA1000000, 0x01000041, 0x800F0004, 0x02000001, 0x800F0800, 0x80E40000, 0x0000FFFF
};

#if 0
static const char *g_tempShaderString =
	"ps_3_0\n"

	"dcl_texcoord0 v4.xy\n"
	"dcl_2d s0\n"
	"dcl_texcoord1 v5.xy\n"
	"dcl_2d s1\n"
	"dcl_texcoord2 v6.xy\n"
	"dcl_2d s2\n"

	"texld r0, v4, s0\n"
	"texld r1, v5, s1\n"
	"texld r2, v6, s2\n"

	"mul r0, r0, r1\n"
	"mul r0.rgb, r0, c0.aaaa\n"

	"mul r0, r0, r2\n"
	"mul r0.rgb, r0, c0.aaaa\n"

	"sub r4, r0.aaaa, c0.xxxx\n"
	"texkill r4\n"

	"mov oC0, r0\n"
;
#endif
static const DWORD g_fpComplexSurfaceTripleTextureModulated[] = {
	0xFFFF0300, 0x0200001F, 0x80000005, 0x90030004, 0x0200001F, 0x90000000, 0xA00F0800, 0x0200001F,
	0x80010005, 0x90030005, 0x0200001F, 0x90000000, 0xA00F0801, 0x0200001F, 0x80020005, 0x90030006,
	0x0200001F, 0x90000000, 0xA00F0802, 0x03000042, 0x800F0000, 0x90E40004, 0xA0E40800, 0x03000042,
	0x800F0001, 0x90E40005, 0xA0E40801, 0x03000042, 0x800F0002, 0x90E40006, 0xA0E40802, 0x03000005,
	0x800F0000, 0x80E40000, 0x80E40001, 0x03000005, 0x80070000, 0x80E40000, 0xA0FF0000, 0x03000005,
	0x800F0000, 0x80E40000, 0x80E40002, 0x03000005, 0x80070000, 0x80E40000, 0xA0FF0000, 0x03000002,
	0x800F0004, 0x80FF0000, 0xA1000000, 0x01000041, 0x800F0004, 0x02000001, 0x800F0800, 0x80E40000,
	0x0000FFFF
};

#if 0
static const char *g_tempShaderString =
	"ps_3_0\n"

	"dcl_texcoord0 v4.xy\n"
	"dcl_2d s0\n"
	"dcl_texcoord1 v5.xy\n"
	"dcl_2d s1\n"

	"def c1, 1.0f, 1.0f, 1.0f, 1.0f\n"

	"texld r0, v4, s0\n"
	"texld r1, v5, s1\n"

	"sub r4, r0.aaaa, c0.xxxx\n"
	"texkill r4\n"

	"sub r1.a, c1.a, r1.a\n"
	"mad r0.rgb, r0, r1.aaaa, r1\n"

	"mov oC0, r0\n"
;
#endif
static const DWORD g_fpComplexSurfaceSingleTextureWithFog[] = {
	0xFFFF0300, 0x0200001F, 0x80000005, 0x90030004, 0x0200001F, 0x90000000, 0xA00F0800, 0x0200001F,
	0x80010005, 0x90030005, 0x0200001F, 0x90000000, 0xA00F0801, 0x05000051, 0xA00F0001, 0x3F800000,
	0x3F800000, 0x3F800000, 0x3F800000, 0x03000042, 0x800F0000, 0x90E40004, 0xA0E40800, 0x03000042,
	0x800F0001, 0x90E40005, 0xA0E40801, 0x03000002, 0x800F0004, 0x80FF0000, 0xA1000000, 0x01000041,
	0x800F0004, 0x03000002, 0x80080001, 0xA0FF0001, 0x81FF0001, 0x04000004, 0x80070000, 0x80E40000,
	0x80FF0001, 0x80E40001, 0x02000001, 0x800F0800, 0x80E40000, 0x0000FFFF
};

#if 0
static const char *g_tempShaderString =
	"ps_3_0\n"

	"dcl_texcoord0 v4.xy\n"
	"dcl_2d s0\n"
	"dcl_texcoord1 v5.xy\n"
	"dcl_2d s1\n"
	"dcl_texcoord2 v6.xy\n"
	"dcl_2d s2\n"

	"def c1, 1.0f, 1.0f, 1.0f, 1.0f\n"

	"texld r0, v4, s0\n"
	"texld r1, v5, s1\n"
	"texld r2, v6, s2\n"

	"mul r0, r0, r1\n"
	"mul r0.rgb, r0, c0.aaaa\n"

	"sub r4, r0.aaaa, c0.xxxx\n"
	"texkill r4\n"

	"sub r2.a, c1.a, r2.a\n"
	"mad r0.rgb, r0, r2.aaaa, r2\n"

	"mov oC0, r0\n"
;
#endif
static const DWORD g_fpComplexSurfaceDualTextureModulatedWithFog[] = {
	0xFFFF0300, 0x0200001F, 0x80000005, 0x90030004, 0x0200001F, 0x90000000, 0xA00F0800, 0x0200001F,
	0x80010005, 0x90030005, 0x0200001F, 0x90000000, 0xA00F0801, 0x0200001F, 0x80020005, 0x90030006,
	0x0200001F, 0x90000000, 0xA00F0802, 0x05000051, 0xA00F0001, 0x3F800000, 0x3F800000, 0x3F800000,
	0x3F800000, 0x03000042, 0x800F0000, 0x90E40004, 0xA0E40800, 0x03000042, 0x800F0001, 0x90E40005,
	0xA0E40801, 0x03000042, 0x800F0002, 0x90E40006, 0xA0E40802, 0x03000005, 0x800F0000, 0x80E40000,
	0x80E40001, 0x03000005, 0x80070000, 0x80E40000, 0xA0FF0000, 0x03000002, 0x800F0004, 0x80FF0000,
	0xA1000000, 0x01000041, 0x800F0004, 0x03000002, 0x80080002, 0xA0FF0001, 0x81FF0002, 0x04000004,
	0x80070000, 0x80E40000, 0x80FF0002, 0x80E40002, 0x02000001, 0x800F0800, 0x80E40000, 0x0000FFFF
};

#if 0
static const char *g_tempShaderString =
	"ps_3_0\n"

	"dcl_texcoord0 v4.xy\n"
	"dcl_2d s0\n"
	"dcl_texcoord1 v5.xy\n"
	"dcl_2d s1\n"
	"dcl_texcoord2 v6.xy\n"
	"dcl_2d s2\n"
	"dcl_texcoord3 v7.xy\n"
	"dcl_2d s3\n"

	"def c1, 1.0f, 1.0f, 1.0f, 1.0f\n"

	"texld r0, v4, s0\n"
	"texld r1, v5, s1\n"
	"texld r2, v6, s2\n"
	"texld r3, v7, s3\n"

	"mul r0, r0, r1\n"
	"mul r0.rgb, r0, c0.aaaa\n"

	"mul r0, r0, r2\n"
	"mul r0.rgb, r0, c0.aaaa\n"

	"sub r4, r0.aaaa, c0.xxxx\n"
	"texkill r4\n"

	"sub r3.a, c1.a, r3.a\n"
	"mad r0.rgb, r0, r3.aaaa, r3\n"

	"mov oC0, r0\n"
;
#endif
static const DWORD g_fpComplexSurfaceTripleTextureModulatedWithFog[] = {
	0xFFFF0300, 0x0200001F, 0x80000005, 0x90030004, 0x0200001F, 0x90000000, 0xA00F0800, 0x0200001F,
	0x80010005, 0x90030005, 0x0200001F, 0x90000000, 0xA00F0801, 0x0200001F, 0x80020005, 0x90030006,
	0x0200001F, 0x90000000, 0xA00F0802, 0x0200001F, 0x80030005, 0x90030007, 0x0200001F, 0x90000000,
	0xA00F0803, 0x05000051, 0xA00F0001, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000, 0x03000042,
	0x800F0000, 0x90E40004, 0xA0E40800, 0x03000042, 0x800F0001, 0x90E40005, 0xA0E40801, 0x03000042,
	0x800F0002, 0x90E40006, 0xA0E40802, 0x03000042, 0x800F0003, 0x90E40007, 0xA0E40803, 0x03000005,
	0x800F0000, 0x80E40000, 0x80E40001, 0x03000005, 0x80070000, 0x80E40000, 0xA0FF0000, 0x03000005,
	0x800F0000, 0x80E40000, 0x80E40002, 0x03000005, 0x80070000, 0x80E40000, 0xA0FF0000, 0x03000002,
	0x800F0004, 0x80FF0000, 0xA1000000, 0x01000041, 0x800F0004, 0x03000002, 0x80080003, 0xA0FF0001,
	0x81FF0003, 0x04000004, 0x80070000, 0x80E40000, 0x80FF0003, 0x80E40003, 0x02000001, 0x800F0800,
	0x80E40000, 0x0000FFFF
};

#if 0
static const char *g_tempShaderString =
	"ps_3_0\n"

	"dcl_color v1.rgba\n"
	"dcl_texcoord0 v4.xy\n"
	"dcl_2d s0\n"
	"dcl_texcoord4 v8.xyzw\n"

	"def c1, 0.002631578947f, 0.999f, 0.0f, 1.0f\n"

	"mul_sat r1.x, v8.z, c1.x\n"
	"sub r0, c1.yyyy, r1.xxxx\n"
	"texkill r0\n"

	"texld r0, v4, s0\n"
	"lrp r2, r1.xxxx, v1, r0\n"

	"mov oC0, r2\n"
;
#endif
static const DWORD g_fpDetailTexture[] = {
	0xFFFF0300, 0x0200001F, 0x8000000A, 0x900F0001, 0x0200001F, 0x80000005, 0x90030004, 0x0200001F,
	0x90000000, 0xA00F0800, 0x0200001F, 0x80040005, 0x900F0008, 0x05000051, 0xA00F0001, 0x3B2C7692,
	0x3F7FBE77, 0x00000000, 0x3F800000, 0x03000005, 0x80110001, 0x90AA0008, 0xA0000001, 0x03000002,
	0x800F0000, 0xA0550001, 0x81000001, 0x01000041, 0x800F0000, 0x03000042, 0x800F0000, 0x90E40004,
	0xA0E40800, 0x04000012, 0x800F0002, 0x80000001, 0x90E40001, 0x80E40000, 0x02000001, 0x800F0800,
	0x80E40002, 0x0000FFFF
};

#if 0
static const char *g_tempShaderString =
	"ps_3_0\n"

	"dcl_color v1.rgba\n"
	"dcl_texcoord0 v4.xyzw\n"
	"dcl_2d s0\n"
	"dcl_texcoord4 v8.xyzw\n"

	"def c1, 0.002631578947f, 0.999f, 0.0f, 1.0f\n"
	"def c2, 4.223f, 4.223f, 0.0f, 1.0f\n"

	"mul_sat r1.x, v8.z, c1.x\n"
	"sub r0, c1.yyyy, r1.xxxx\n"
	"texkill r0\n"

	"texld r0, v4.xy, s0\n"
	"lrp r2, r1.xxxx, v1, r0\n"

	"mul_sat r1.x, r1.x, c2.x\n"

	"texld r0, v4.zw, s0\n"
	"lrp r3, r1.xxxx, v1, r0\n"

	"mul r2, r2, r3\n"
	"add r2, r2, r2\n"

	"mov oC0, r2\n"
;
#endif
static const DWORD g_fpDetailTextureTwoLayer[] = {
	0xFFFF0300, 0x0200001F, 0x8000000A, 0x900F0001, 0x0200001F, 0x80000005, 0x900F0004, 0x0200001F,
	0x90000000, 0xA00F0800, 0x0200001F, 0x80040005, 0x900F0008, 0x05000051, 0xA00F0001, 0x3B2C7692,
	0x3F7FBE77, 0x00000000, 0x3F800000, 0x05000051, 0xA00F0002, 0x408722D1, 0x408722D1, 0x00000000,
	0x3F800000, 0x03000005, 0x80110001, 0x90AA0008, 0xA0000001, 0x03000002, 0x800F0000, 0xA0550001,
	0x81000001, 0x01000041, 0x800F0000, 0x03000042, 0x800F0000, 0x90540004, 0xA0E40800, 0x04000012,
	0x800F0002, 0x80000001, 0x90E40001, 0x80E40000, 0x03000005, 0x80110001, 0x80000001, 0xA0000002,
	0x03000042, 0x800F0000, 0x90FE0004, 0xA0E40800, 0x04000012, 0x800F0003, 0x80000001, 0x90E40001,
	0x80E40000, 0x03000005, 0x800F0002, 0x80E40002, 0x80E40003, 0x03000002, 0x800F0002, 0x80E40002,
	0x80E40002, 0x02000001, 0x800F0800, 0x80E40002, 0x0000FFFF
};

#if 0
static const char *g_tempShaderString =
	"ps_3_0\n"

	"dcl_color v1.rgba\n"
	"dcl_texcoord0 v4.xy\n"
	"dcl_2d s0\n"
	"dcl_texcoord1 v5.xy\n"
	"dcl_2d s1\n"
	"dcl_texcoord4 v8.xyzw\n"

	"def c1, 0.002631578947f, 0.999f, 0.0f, 1.0f\n"

	"texld r0, v4, s0\n"

	"sub r4, r0.aaaa, c0.xxxx\n"
	"texkill r4\n"

	"mul_sat r1.x, v8.z, c1.x\n"
	"if_le r1.x, c1.y\n"
		"texld r2, v5, s1\n"
		"lrp r3, r1.xxxx, v1, r2\n"

		"mul r0.rgb, r0, r3\n"
		"add r0.rgb, r0, r0\n"
	"endif\n"

	"mov oC0, r0\n"
;
#endif
static const DWORD g_fpSingleTextureAndDetailTexture[] = {
	0xFFFF0300, 0x0200001F, 0x8000000A, 0x900F0001, 0x0200001F, 0x80000005, 0x90030004, 0x0200001F,
	0x90000000, 0xA00F0800, 0x0200001F, 0x80010005, 0x90030005, 0x0200001F, 0x90000000, 0xA00F0801,
	0x0200001F, 0x80040005, 0x900F0008, 0x05000051, 0xA00F0001, 0x3B2C7692, 0x3F7FBE77, 0x00000000,
	0x3F800000, 0x03000042, 0x800F0000, 0x90E40004, 0xA0E40800, 0x03000002, 0x800F0004, 0x80FF0000,
	0xA1000000, 0x01000041, 0x800F0004, 0x03000005, 0x80110001, 0x90AA0008, 0xA0000001, 0x02060029,
	0x80000001, 0xA0550001, 0x03000042, 0x800F0002, 0x90E40005, 0xA0E40801, 0x04000012, 0x800F0003,
	0x80000001, 0x90E40001, 0x80E40002, 0x03000005, 0x80070000, 0x80E40000, 0x80E40003, 0x03000002,
	0x80070000, 0x80E40000, 0x80E40000, 0x0000002B, 0x02000001, 0x800F0800, 0x80E40000, 0x0000FFFF
};

#if 0
static const char *g_tempShaderString =
	"ps_3_0\n"

	"dcl_color v1.rgba\n"
	"dcl_texcoord0 v4.xy\n"
	"dcl_2d s0\n"
	"dcl_texcoord1 v5.xyzw\n"
	"dcl_2d s1\n"
	"dcl_texcoord4 v8.xyzw\n"

	"def c1, 0.002631578947f, 0.999f, 0.0f, 1.0f\n"
	"def c2, 4.223f, 4.223f, 0.0f, 1.0f\n"

	"texld r0, v4, s0\n"

	"sub r4, r0.aaaa, c0.xxxx\n"
	"texkill r4\n"

	"mul_sat r1.x, v8.z, c1.x\n"
	"if_le r1.x, c1.y\n"
		"texld r2, v5.xy, s1\n"
		"lrp r3, r1.xxxx, v1, r2\n"

		"mul r0.rgb, r0, r3\n"
		"add r0.rgb, r0, r0\n"

		"mul_sat r1.x, r1.x, c2.x\n"

		"texld r2, v5.zw, s1\n"
		"lrp r3, r1.xxxx, v1, r2\n"

		"mul r0.rgb, r0, r3\n"
		"add r0.rgb, r0, r0\n"
	"endif\n"

	"mov oC0, r0\n"
;
#endif
static const DWORD g_fpSingleTextureAndDetailTextureTwoLayer[] = {
	0xFFFF0300, 0x0200001F, 0x8000000A, 0x900F0001, 0x0200001F, 0x80000005, 0x90030004, 0x0200001F,
	0x90000000, 0xA00F0800, 0x0200001F, 0x80010005, 0x900F0005, 0x0200001F, 0x90000000, 0xA00F0801,
	0x0200001F, 0x80040005, 0x900F0008, 0x05000051, 0xA00F0001, 0x3B2C7692, 0x3F7FBE77, 0x00000000,
	0x3F800000, 0x05000051, 0xA00F0002, 0x408722D1, 0x408722D1, 0x00000000, 0x3F800000, 0x03000042,
	0x800F0000, 0x90E40004, 0xA0E40800, 0x03000002, 0x800F0004, 0x80FF0000, 0xA1000000, 0x01000041,
	0x800F0004, 0x03000005, 0x80110001, 0x90AA0008, 0xA0000001, 0x02060029, 0x80000001, 0xA0550001,
	0x03000042, 0x800F0002, 0x90540005, 0xA0E40801, 0x04000012, 0x800F0003, 0x80000001, 0x90E40001,
	0x80E40002, 0x03000005, 0x80070000, 0x80E40000, 0x80E40003, 0x03000002, 0x80070000, 0x80E40000,
	0x80E40000, 0x03000005, 0x80110001, 0x80000001, 0xA0000002, 0x03000042, 0x800F0002, 0x90FE0005,
	0xA0E40801, 0x04000012, 0x800F0003, 0x80000001, 0x90E40001, 0x80E40002, 0x03000005, 0x80070000,
	0x80E40000, 0x80E40003, 0x03000002, 0x80070000, 0x80E40000, 0x80E40000, 0x0000002B, 0x02000001,
	0x800F0800, 0x80E40000, 0x0000FFFF
};

#if 0
static const char *g_tempShaderString =
	"ps_3_0\n"

	"dcl_color v1.rgba\n"
	"dcl_texcoord0 v4.xy\n"
	"dcl_2d s0\n"
	"dcl_texcoord1 v5.xy\n"
	"dcl_2d s1\n"
	"dcl_texcoord2 v6.xy\n"
	"dcl_2d s2\n"
	"dcl_texcoord4 v8.xyzw\n"

	"def c1, 0.002631578947f, 0.999f, 0.0f, 1.0f\n"

	"texld r0, v4, s0\n"
	"texld r1, v5, s1\n"

	"mul r0, r0, r1\n"
	"mul r0.rgb, r0, c0.aaaa\n"

	"sub r4, r0.aaaa, c0.xxxx\n"
	"texkill r4\n"

	"mul_sat r1.x, v8.z, c1.x\n"
	"if_le r1.x, c1.y\n"
		"texld r2, v6, s2\n"
		"lrp r3, r1.xxxx, v1, r2\n"

		"mul r0.rgb, r0, r3\n"
		"add r0.rgb, r0, r0\n"
	"endif\n"

	"mov oC0, r0\n"
;
#endif
static const DWORD g_fpDualTextureAndDetailTexture[] = {
	0xFFFF0300, 0x0200001F, 0x8000000A, 0x900F0001, 0x0200001F, 0x80000005, 0x90030004, 0x0200001F,
	0x90000000, 0xA00F0800, 0x0200001F, 0x80010005, 0x90030005, 0x0200001F, 0x90000000, 0xA00F0801,
	0x0200001F, 0x80020005, 0x90030006, 0x0200001F, 0x90000000, 0xA00F0802, 0x0200001F, 0x80040005,
	0x900F0008, 0x05000051, 0xA00F0001, 0x3B2C7692, 0x3F7FBE77, 0x00000000, 0x3F800000, 0x03000042,
	0x800F0000, 0x90E40004, 0xA0E40800, 0x03000042, 0x800F0001, 0x90E40005, 0xA0E40801, 0x03000005,
	0x800F0000, 0x80E40000, 0x80E40001, 0x03000005, 0x80070000, 0x80E40000, 0xA0FF0000, 0x03000002,
	0x800F0004, 0x80FF0000, 0xA1000000, 0x01000041, 0x800F0004, 0x03000005, 0x80110001, 0x90AA0008,
	0xA0000001, 0x02060029, 0x80000001, 0xA0550001, 0x03000042, 0x800F0002, 0x90E40006, 0xA0E40802,
	0x04000012, 0x800F0003, 0x80000001, 0x90E40001, 0x80E40002, 0x03000005, 0x80070000, 0x80E40000,
	0x80E40003, 0x03000002, 0x80070000, 0x80E40000, 0x80E40000, 0x0000002B, 0x02000001, 0x800F0800,
	0x80E40000, 0x0000FFFF
};

#if 0
static const char *g_tempShaderString =
	"ps_3_0\n"

	"dcl_color v1.rgba\n"
	"dcl_texcoord0 v4.xy\n"
	"dcl_2d s0\n"
	"dcl_texcoord1 v5.xy\n"
	"dcl_2d s1\n"
	"dcl_texcoord2 v6.xyzw\n"
	"dcl_2d s2\n"
	"dcl_texcoord4 v8.xyzw\n"

	"def c1, 0.002631578947f, 0.999f, 0.0f, 1.0f\n"
	"def c2, 4.223f, 4.223f, 0.0f, 1.0f\n"

	"texld r0, v4, s0\n"
	"texld r1, v5, s1\n"

	"mul r0, r0, r1\n"
	"mul r0.rgb, r0, c0.aaaa\n"

	"sub r4, r0.aaaa, c0.xxxx\n"
	"texkill r4\n"

	"mul_sat r1.x, v8.z, c1.x\n"
	"if_le r1.x, c1.y\n"
		"texld r2, v6.xy, s2\n"
		"lrp r3, r1.xxxx, v1, r2\n"

		"mul r0.rgb, r0, r3\n"
		"add r0.rgb, r0, r0\n"

		"mul_sat r1.x, r1.x, c2.x\n"

		"texld r2, v6.zw, s2\n"
		"lrp r3, r1.xxxx, v1, r2\n"

		"mul r0.rgb, r0, r3\n"
		"add r0.rgb, r0, r0\n"
	"endif\n"

	"mov oC0, r0\n"
;
#endif
static const DWORD g_fpDualTextureAndDetailTextureTwoLayer[] = {
	0xFFFF0300, 0x0200001F, 0x8000000A, 0x900F0001, 0x0200001F, 0x80000005, 0x90030004, 0x0200001F,
	0x90000000, 0xA00F0800, 0x0200001F, 0x80010005, 0x90030005, 0x0200001F, 0x90000000, 0xA00F0801,
	0x0200001F, 0x80020005, 0x900F0006, 0x0200001F, 0x90000000, 0xA00F0802, 0x0200001F, 0x80040005,
	0x900F0008, 0x05000051, 0xA00F0001, 0x3B2C7692, 0x3F7FBE77, 0x00000000, 0x3F800000, 0x05000051,
	0xA00F0002, 0x408722D1, 0x408722D1, 0x00000000, 0x3F800000, 0x03000042, 0x800F0000, 0x90E40004,
	0xA0E40800, 0x03000042, 0x800F0001, 0x90E40005, 0xA0E40801, 0x03000005, 0x800F0000, 0x80E40000,
	0x80E40001, 0x03000005, 0x80070000, 0x80E40000, 0xA0FF0000, 0x03000002, 0x800F0004, 0x80FF0000,
	0xA1000000, 0x01000041, 0x800F0004, 0x03000005, 0x80110001, 0x90AA0008, 0xA0000001, 0x02060029,
	0x80000001, 0xA0550001, 0x03000042, 0x800F0002, 0x90540006, 0xA0E40802, 0x04000012, 0x800F0003,
	0x80000001, 0x90E40001, 0x80E40002, 0x03000005, 0x80070000, 0x80E40000, 0x80E40003, 0x03000002,
	0x80070000, 0x80E40000, 0x80E40000, 0x03000005, 0x80110001, 0x80000001, 0xA0000002, 0x03000042,
	0x800F0002, 0x90FE0006, 0xA0E40802, 0x04000012, 0x800F0003, 0x80000001, 0x90E40001, 0x80E40002,
	0x03000005, 0x80070000, 0x80E40000, 0x80E40003, 0x03000002, 0x80070000, 0x80E40000, 0x80E40000,
	0x0000002B, 0x02000001, 0x800F0800, 0x80E40000, 0x0000FFFF
};


/*-----------------------------------------------------------------------------
	D3D9Drv.
-----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(UD3D9RenderDevice);


#ifdef UTD3D9R_INCLUDE_SHADER_ASM
void UD3D9RenderDevice::AssembleShader(void) {
	HRESULT hResult;
	LPD3DXBUFFER pBufCompiled = NULL;
	LPD3DXBUFFER pBufErrors = NULL;

	dbgPrintf("Enter shader assembly\n");

	hResult = D3DXAssembleShader(g_tempShaderString, strlen(g_tempShaderString), NULL, NULL,
		0, &pBufCompiled, &pBufErrors);
	if (FAILED(hResult)) {
		DWORD bufSize;
		std::string sMsg;
		DWORD u;
		VOID *bufPtr;

		dbgPrintf("ERROR ASSEMBLING SHADER\n");

		bufSize = pBufErrors->GetBufferSize();
		bufPtr = pBufErrors->GetBufferPointer();
		dbgPrintf("Size = %u\n", bufSize);
		for (u = 0; u < bufSize; u++) {
			sMsg += *((const char *)bufPtr + u);
		}
		dbgPrintf("Data = %ls\n", sMsg.c_str());

		//Free buffers
		if (pBufCompiled) pBufCompiled->Release();
		if (pBufErrors) pBufErrors->Release();

		return;
	}
	dbgPrintf("SHADER ASSEMBLED OKAY\n");

	{
		DWORD compBufSize = pBufCompiled->GetBufferSize();
		VOID *compBufPtr = pBufCompiled->GetBufferPointer();
		DWORD u;
		TCHAR dbgStr[1024];

		dbgPrintf("Compiled Size = %u\n", compBufSize);
		dbgPrintf("Data = \n");
		dbgStr[0] = _T('\0');
		for (u = 0; u < compBufSize; u += 4) {
			TCHAR numStr[32];

			if ((u != 0) && ((u % 32) == 0)) {
				dbgPrintf("%ls\n", appToAnsi(dbgStr));
				dbgStr[0] = _T('\0');
			}
			appSprintf(numStr, TEXT("0x%08X, "), *(DWORD *)((BYTE *)compBufPtr + u));
			appStrcat(dbgStr, numStr);
		}
		dbgPrintf("%ls\n", appToAnsi(dbgStr));
	}

	dbgPrintf("Leave shader assembly\n");

	//Free buffers
	if (pBufCompiled) pBufCompiled->Release();
	if (pBufErrors) pBufErrors->Release();

	return;
}
#endif


int UD3D9RenderDevice::dbgPrintf(const char *format, ...) {
	const unsigned int DBG_PRINT_BUF_SIZE = 1024;
	char dbgPrintBuf[DBG_PRINT_BUF_SIZE];
	va_list vaArgs;
	int iRet = 0;

	va_start(vaArgs, format);

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4996)
	iRet = vsnprintf(dbgPrintBuf, DBG_PRINT_BUF_SIZE, format, vaArgs);
	dbgPrintBuf[DBG_PRINT_BUF_SIZE - 1] = '\0';
#pragma warning(pop)

	TCHAR_CALL_OS(OutputDebugStringW(appFromAnsi(dbgPrintBuf)), OutputDebugStringA(dbgPrintBuf));
#endif

	va_end(vaArgs);

	return iRet;
}


void UD3D9RenderDevice::StaticConstructor() {
	unsigned int u;

	guard(UD3D9RenderDevice::StaticConstructor);

#ifdef UTGLR_DX_BUILD
	const UBOOL UTGLR_DEFAULT_OneXBlending = 1;
#else
	const UBOOL UTGLR_DEFAULT_OneXBlending = 0;
#endif

#if defined UTGLR_DX_BUILD || defined UTGLR_RUNE_BUILD
	const UBOOL UTGLR_DEFAULT_UseS3TC = 0;
#else
	const UBOOL UTGLR_DEFAULT_UseS3TC = 1;
#endif

#if defined UTGLR_DX_BUILD || defined UTGLR_RUNE_BUILD
	const UBOOL UTGLR_DEFAULT_ZRangeHack = 0;
#else
	const UBOOL UTGLR_DEFAULT_ZRangeHack = 1;
#endif

#define CPP_PROPERTY_LOCAL(_name) _name, CPP_PROPERTY(_name)
#define CPP_PROPERTY_LOCAL_DCV(_name) DCV._name, CPP_PROPERTY(DCV._name)

	UEnum* VSyncs = new(GetClass(), TEXT("VSyncs"))UEnum(NULL);

	// Enum members.
	new(VSyncs->Names)FName(TEXT("Off"));
	new(VSyncs->Names)FName(TEXT("On"));

	//Set parameter defaults and add parameters
	SC_AddFloatConfigParam(TEXT("LODBias"), CPP_PROPERTY_LOCAL(LODBias), 0.0f);
	SC_AddFloatConfigParam(TEXT("GammaOffset"), CPP_PROPERTY_LOCAL(GammaOffset), 0.0f);
	SC_AddFloatConfigParam(TEXT("GammaOffsetRed"), CPP_PROPERTY_LOCAL(GammaOffsetRed), 0.0f);
	SC_AddFloatConfigParam(TEXT("GammaOffsetGreen"), CPP_PROPERTY_LOCAL(GammaOffsetGreen), 0.0f);
	SC_AddFloatConfigParam(TEXT("GammaOffsetBlue"), CPP_PROPERTY_LOCAL(GammaOffsetBlue), 0.0f);
	SC_AddBoolConfigParam(1,  TEXT("GammaCorrectScreenshots"), CPP_PROPERTY_LOCAL(GammaCorrectScreenshots), 0);
	SC_AddBoolConfigParam(0,  TEXT("OneXBlending"), CPP_PROPERTY_LOCAL(OneXBlending), UTGLR_DEFAULT_OneXBlending);
	SC_AddIntConfigParam(TEXT("MinLogTextureSize"), CPP_PROPERTY_LOCAL(MinLogTextureSize), 0);
	SC_AddIntConfigParam(TEXT("MaxLogTextureSize"), CPP_PROPERTY_LOCAL(MaxLogTextureSize), 12);
	SC_AddBoolConfigParam(5,  TEXT("UseMultiTexture"), CPP_PROPERTY_LOCAL(UseMultiTexture), 1);
	SC_AddBoolConfigParam(4,  TEXT("UsePrecache"), CPP_PROPERTY_LOCAL(UsePrecache), 0);
	SC_AddBoolConfigParam(3,  TEXT("UseTrilinear"), CPP_PROPERTY_LOCAL(UseTrilinear), 0);
	SC_AddBoolConfigParam(2,  TEXT("UseS3TC"), CPP_PROPERTY_LOCAL(UseS3TC), UTGLR_DEFAULT_UseS3TC);
	SC_AddBoolConfigParam(1,  TEXT("Use16BitTextures"), CPP_PROPERTY_LOCAL(Use16BitTextures), 0);
	SC_AddBoolConfigParam(0,  TEXT("Use565Textures"), CPP_PROPERTY_LOCAL(Use565Textures), 0);
	SC_AddIntConfigParam(TEXT("MaxAnisotropy"), CPP_PROPERTY_LOCAL(MaxAnisotropy), 0);
	SC_AddBoolConfigParam(0,  TEXT("NoFiltering"), CPP_PROPERTY_LOCAL(NoFiltering), 0);
	SC_AddIntConfigParam(TEXT("MaxTMUnits"), CPP_PROPERTY_LOCAL(MaxTMUnits), 0);
	SC_AddIntConfigParam(TEXT("RefreshRate"), CPP_PROPERTY_LOCAL(RefreshRate), 0);
	SC_AddIntConfigParam(TEXT("DetailMax"), CPP_PROPERTY_LOCAL(DetailMax), 0);
	SC_AddBoolConfigParam(8,  TEXT("DetailClipping"), CPP_PROPERTY_LOCAL(DetailClipping), 0);
	SC_AddBoolConfigParam(7,  TEXT("ColorizeDetailTextures"), CPP_PROPERTY_LOCAL(ColorizeDetailTextures), 0);
	//SC_AddBoolConfigParam(6,  TEXT("SinglePassFog"), CPP_PROPERTY_LOCAL(SinglePassFog), 1);
	SC_AddBoolConfigParam(5,  TEXT("SinglePassDetail"), CPP_PROPERTY_LOCAL_DCV(SinglePassDetail), 0);
	SC_AddBoolConfigParam(4,  TEXT("UseSSE"), CPP_PROPERTY_LOCAL(UseSSE), 1);
	SC_AddBoolConfigParam(3,  TEXT("UseSSE2"), CPP_PROPERTY_LOCAL(UseSSE2), 1);
	SC_AddBoolConfigParam(2,  TEXT("UseTexIdPool"), CPP_PROPERTY_LOCAL(UseTexIdPool), 1);
	SC_AddBoolConfigParam(1,  TEXT("UseTexPool"), CPP_PROPERTY_LOCAL(UseTexPool), 1);
	SC_AddBoolConfigParam(0,  TEXT("CacheStaticMaps"), CPP_PROPERTY_LOCAL(CacheStaticMaps), 1);
	SC_AddIntConfigParam(TEXT("DynamicTexIdRecycleLevel"), CPP_PROPERTY_LOCAL(DynamicTexIdRecycleLevel), 100);
	SC_AddBoolConfigParam(1,  TEXT("TexDXT1ToDXT3"), CPP_PROPERTY_LOCAL(TexDXT1ToDXT3), 0);
	//SC_AddBoolConfigParam(0,  TEXT("UseFragmentProgram"), CPP_PROPERTY_LOCAL_DCV(UseFragmentProgram), 0);
	SC_AddEnumConfigParam(TEXT("UseVSync"), CPP_PROPERTY_LOCAL(UseVSync), VSyncs, 1);
#if 0 // Now done in Engine!
	SC_AddIntConfigParam(TEXT("FrameRateLimit"), CPP_PROPERTY_LOCAL(FrameRateLimit), 0);
#endif
	SC_AddBoolConfigParam(6,  TEXT("SceneNodeHack"), CPP_PROPERTY_LOCAL(SceneNodeHack), 1);
	SC_AddBoolConfigParam(5,  TEXT("SmoothMaskedTextures"), CPP_PROPERTY_LOCAL(SmoothMaskedTextures), 0);
	SC_AddBoolConfigParam(4,  TEXT("MaskedTextureHack"), CPP_PROPERTY_LOCAL(MaskedTextureHack), 1);
	SC_AddBoolConfigParam(3,  TEXT("UseTripleBuffering"), CPP_PROPERTY_LOCAL(UseTripleBuffering), 0);
	SC_AddBoolConfigParam(2,  TEXT("UsePureDevice"), CPP_PROPERTY_LOCAL(UsePureDevice), 1);
	SC_AddBoolConfigParam(1,  TEXT("UseSoftwareVertexProcessing"), CPP_PROPERTY_LOCAL(UseSoftwareVertexProcessing), 0);
	SC_AddBoolConfigParam(0,  TEXT("UseAA"), CPP_PROPERTY_LOCAL(UseAA), 0);
	SC_AddIntConfigParam(TEXT("NumAASamples"), CPP_PROPERTY_LOCAL(NumAASamples), 4);
	SC_AddBoolConfigParam(3, TEXT("UseEnhancedLightmaps"), CPP_PROPERTY_LOCAL(UseEnhancedLightmaps), 0);
	SC_AddBoolConfigParam(2,  TEXT("NoAATiles"), CPP_PROPERTY_LOCAL(NoAATiles), 1);
	SC_AddBoolConfigParam(1,  TEXT("ZRangeHack"), CPP_PROPERTY_LOCAL(ZRangeHack), UTGLR_DEFAULT_ZRangeHack);
	SC_AddBoolConfigParam(0,  TEXT("UseHardwareClipping"), CPP_PROPERTY_LOCAL(UseHardwareClipping), 1);

#undef CPP_PROPERTY_LOCAL
#undef CPP_PROPERTY_LOCAL_DCV

	//Driver flags
	SpanBased				= 0;
	SupportsFogMaps			= 1;
#ifdef UTGLR_RUNE_BUILD
	SupportsDistanceFog		= 1;
#elif UTGLR_UNREAL_227_BUILD
	SupportsDistanceFog		= 1;
	LegacyRenderingOnly		= 1;
#else
	SupportsDistanceFog		= 0;
#endif
	FullscreenOnly			= 0;

	SupportsLazyTextures	= 0;
	PrefersDeferredLoad		= 0;

	//Mark device pointers as invalid
	m_d3d9 = NULL;
	m_d3dDevice = NULL;

	//Invalidate fixed texture ids
	m_pNoTexObj = NULL;
	m_pAlphaTexObj = NULL;

	//Mark all vertex buffer objects as invalid
	m_d3dVertexColorBuffer = NULL;
	m_d3dSecondaryColorBuffer = NULL;
	for (u = 0; u < MAX_TMUNITS; u++) {
		m_d3dTexCoordBuffer[u] = NULL;
	}

	//Mark all vertex declarations as not created
	m_oneColorVertexDecl = NULL;
	for (u = 0; u < MAX_TMUNITS; u++) {
		m_standardNTextureVertexDecl[u] = NULL;
	}
	m_twoColorSingleTextureVertexDecl = NULL;

	//Mark all vertex shader definitions as not created
	m_vpDefaultRenderingState = NULL;
	m_vpDefaultRenderingStateWithFog = NULL;
	m_vpDefaultRenderingStateWithLinearFog = NULL;
	for (u = 0; u < MAX_TMUNITS; u++) {
		m_vpComplexSurface[u] = NULL;
	}
	m_vpDetailTexture = NULL;
	m_vpComplexSurfaceSingleTextureAndDetailTexture = NULL;
	m_vpComplexSurfaceDualTextureAndDetailTexture = NULL;

	//Mark all fragment shader definitions as not created
	m_fpDefaultRenderingState = NULL;
	m_fpDefaultRenderingStateWithFog = NULL;
	m_fpDefaultRenderingStateWithLinearFog = NULL;
	m_fpComplexSurfaceSingleTexture = NULL;
	m_fpComplexSurfaceDualTextureModulated = NULL;
	m_fpComplexSurfaceTripleTextureModulated = NULL;
	m_fpComplexSurfaceSingleTextureWithFog = NULL;
	m_fpComplexSurfaceDualTextureModulatedWithFog = NULL;
	m_fpComplexSurfaceTripleTextureModulatedWithFog = NULL;
	m_fpDetailTexture = NULL;
	m_fpDetailTextureTwoLayer = NULL;
	m_fpSingleTextureAndDetailTexture = NULL;
	m_fpSingleTextureAndDetailTextureTwoLayer = NULL;
	m_fpDualTextureAndDetailTexture = NULL;
	m_fpDualTextureAndDetailTextureTwoLayer = NULL;

	//Reset TMUnits in case resource cleanup code is ever called before this is initialized
	TMUnits = 0;

	//Clear the SetRes is device reset flag
	m_SetRes_isDeviceReset = false;

	//Frame rate limit timer not yet initialized
	m_frameRateLimitTimerInitialized = false;

	DescFlags |= RDDESCF_Certified;

	unguard;
}


void UD3D9RenderDevice::SC_AddBoolConfigParam(DWORD BitMaskOffset, const TCHAR *pName, UBOOL &param, ECppProperty EC_CppProperty, INT InOffset, UBOOL defaultValue) {
	param = (((defaultValue) != 0) ? 1 : 0) << BitMaskOffset; //Doesn't exactly work like a UBOOL "// Boolean 0 (false) or 1 (true)."
	new(GetClass(), pName, RF_Public)UBoolProperty(EC_CppProperty, InOffset, TEXT("Options"), CPF_Config);
}

void UD3D9RenderDevice::SC_AddIntConfigParam(const TCHAR *pName, INT &param, ECppProperty EC_CppProperty, INT InOffset, INT defaultValue) {
	param = defaultValue;
	new(GetClass(), pName, RF_Public)UIntProperty(EC_CppProperty, InOffset, TEXT("Options"), CPF_Config);
}

void UD3D9RenderDevice::SC_AddFloatConfigParam(const TCHAR *pName, FLOAT &param, ECppProperty EC_CppProperty, INT InOffset, FLOAT defaultValue) {
	param = defaultValue;
	new(GetClass(), pName, RF_Public)UFloatProperty(EC_CppProperty, InOffset, TEXT("Options"), CPF_Config);
}

void UD3D9RenderDevice::SC_AddByteConfigParam(const TCHAR *pName, BYTE &param, ECppProperty EC_CppProperty, INT InOffset, BYTE defaultValue) {
	param = defaultValue;
	new(GetClass(), pName, RF_Public)UByteProperty(EC_CppProperty, InOffset, TEXT("Options"), CPF_Config);
}

void UD3D9RenderDevice::SC_AddEnumConfigParam(const TCHAR *pName, BYTE &param, ECppProperty EC_CppProperty, INT InOffset, UEnum* Enum, BYTE defaultValue) {
	param = defaultValue;
	new(GetClass(), pName, RF_Public)UByteProperty(EC_CppProperty, InOffset, TEXT("Options"), CPF_Config, Enum);
}

void UD3D9RenderDevice::DbgPrintInitParam(const char *pName, INT value) {
	dbgPrintf("utd3d9r: %ls = %d\n", pName, value);
	return;
}

void UD3D9RenderDevice::DbgPrintInitParam(const char *pName, FLOAT value) {
	dbgPrintf("utd3d9r: %ls = %f\n", pName, value);
	return;
}


#ifdef UTGLR_INCLUDE_SSE_CODE
bool UD3D9RenderDevice::CPU_DetectCPUID(void) {
	//Check for cpuid instruction support
	__try {
		__asm {
			//CPUID function 0
			xor eax, eax
			cpuid
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}

	return true;
}

bool UD3D9RenderDevice::CPU_DetectSSE(void) {
	bool bSupportsSSE;

	//Check for cpuid instruction support
	if (CPU_DetectCPUID() != true) {
		return false;
	}

	//Check for SSE support
	bSupportsSSE = false;
	__asm {
		//CPUID function 1
		mov eax, 1
		cpuid

		//Check the SSE bit
		test edx, 0x02000000
		jz l_no_sse

		//Set bSupportsSSE to true
		mov bSupportsSSE, 1

l_no_sse:
	}

	//Return if no CPU SSE support
	if (bSupportsSSE == false) {
		return bSupportsSSE;
	}

	//Check for SSE OS support
	__try {
		__asm {
			//Execute SSE instruction
			xorps xmm0, xmm0
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		//Clear SSE support flag
		bSupportsSSE = false;
	}

	return bSupportsSSE;
}

bool UD3D9RenderDevice::CPU_DetectSSE2(void) {
	bool bSupportsSSE2;

	//Check for cpuid instruction support
	if (CPU_DetectCPUID() != true) {
		return false;
	}

	//Check for SSE2 support
	bSupportsSSE2 = false;
	__asm {
		//CPUID function 1
		mov eax, 1
		cpuid

		//Check the SSE2 bit
		test edx, 0x04000000
		jz l_no_sse2

		//Set bSupportsSSE2 to true
		mov bSupportsSSE2, 1

l_no_sse2:
	}

	//Return if no CPU SSE2 support
	if (bSupportsSSE2 == false) {
		return bSupportsSSE2;
	}

	//Check for SSE2 OS support
	__try {
		__asm {
			//Execute SSE2 instruction
			xorpd xmm0, xmm0
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		//Clear SSE2 support flag
		bSupportsSSE2 = false;
	}

	return bSupportsSSE2;
}
#endif //UTGLR_INCLUDE_SSE_CODE


void UD3D9RenderDevice::InitFrameRateLimitTimerSafe(void) {
	//Only initialize once
	if (m_frameRateLimitTimerInitialized) {
		return;
	}
	m_frameRateLimitTimerInitialized = true;

#ifdef _WIN32
	//Request high resolution timer
	timeBeginPeriod(1);
#endif

	return;
}

void UD3D9RenderDevice::ShutdownFrameRateLimitTimer(void) {
	//Only shutdown once
	if (!m_frameRateLimitTimerInitialized) {
		return;
	}
	m_frameRateLimitTimerInitialized = false;

#ifdef _WIN32
	//Release high resolution timer
	timeEndPeriod(1);
#endif

	return;
}

static void FASTCALL Buffer3BasicVerts(UD3D9RenderDevice *pRD, FTransTexture* Pts, INT NumPts)
{
	FGLTexCoord* pTexCoordArray = &pRD->m_pTexCoordArray[0][pRD->m_bufferedVerts];
	FGLVertexColor* pVertexColorArray = &pRD->m_pVertexColorArray[pRD->m_bufferedVerts];
	pRD->m_bufferedVerts += NumPts;

	for (INT i = 0; i < NumPts; ++i, ++Pts)
	{
		*pTexCoordArray++ = FGLTexCoord(Pts->U * pRD->TexInfo[0].UMult, Pts->V * pRD->TexInfo[0].VMult);
		*pVertexColorArray++ = FGLVertexColor(Pts->Point);
	}
}

static void FASTCALL Buffer3ColoredVerts(UD3D9RenderDevice* pRD, FTransTexture* Pts, INT NumPts)
{
	FGLTexCoord *pTexCoordArray = &pRD->m_pTexCoordArray[0][pRD->m_bufferedVerts];
	FGLVertexColor *pVertexColorArray = &pRD->m_pVertexColorArray[pRD->m_bufferedVerts];
	pRD->m_bufferedVerts += NumPts;

	for (INT i = 0; i < NumPts; ++i, ++Pts)
	{
		*pTexCoordArray++ = FGLTexCoord(Pts->U * pRD->TexInfo[0].UMult, Pts->V * pRD->TexInfo[0].VMult);
		*pVertexColorArray++ = FGLVertexColor(Pts->Point, UD3D9RenderDevice::FPlaneTo_BGR_A255(&Pts->Light));
	}
}

static void FASTCALL Buffer3ColoredVertsAlpha(UD3D9RenderDevice* pRD, FTransTexture* Pts, INT NumPts)
{
	FGLTexCoord* pTexCoordArray = &pRD->m_pTexCoordArray[0][pRD->m_bufferedVerts];
	FGLVertexColor* pVertexColorArray = &pRD->m_pVertexColorArray[pRD->m_bufferedVerts];
	pRD->m_bufferedVerts += NumPts;

	for (INT i = 0; i < NumPts; ++i, ++Pts)
	{
		*pTexCoordArray++ = FGLTexCoord(Pts->U * pRD->TexInfo[0].UMult, Pts->V * pRD->TexInfo[0].VMult);
		*pVertexColorArray++ = FGLVertexColor(Pts->Point, UD3D9RenderDevice::FPlaneTo_BGRA(&Pts->Light));
	}
}

static void FASTCALL Buffer3FoggedVerts(UD3D9RenderDevice* pRD, FTransTexture* Pts, INT NumPts)
{
	FGLTexCoord* pTexCoordArray = &pRD->m_pTexCoordArray[0][pRD->m_bufferedVerts];
	FGLVertexColor* pVertexColorArray = &pRD->m_pVertexColorArray[pRD->m_bufferedVerts];
	FGLSecondaryColor* pSecondaryColorArray = &pRD->m_pSecondaryColorArray[pRD->m_bufferedVerts];
	pRD->m_bufferedVerts += NumPts;

	for (INT i = 0; i < NumPts; ++i, ++Pts)
	{
		*pTexCoordArray++ = FGLTexCoord(Pts->U * pRD->TexInfo[0].UMult, Pts->V * pRD->TexInfo[0].VMult);
		*pVertexColorArray++ = FGLVertexColor(Pts->Point, UD3D9RenderDevice::FPlaneTo_BGRScaled_A255(&Pts->Light, 255.0f * (1.0f - Pts->Fog.W)));
		*pSecondaryColorArray++ = FGLSecondaryColor(UD3D9RenderDevice::FPlaneTo_BGR_A0(&Pts->Fog));
	}
}

#ifdef UTGLR_INCLUDE_SSE_CODE
__declspec(naked) static void FASTCALL Buffer3ColoredVerts_SSE(UD3D9RenderDevice *pRD, FTransTexture** Pts) {
	static float f255 = 255.0f;
	__asm {
		//pRD is in ecx
		//Pts is in edx

		push ebx
		push esi
		push edi

		mov eax, [ecx]UD3D9RenderDevice.m_bufferedVerts

		lea ebx, [eax*8]
		add ebx, [ecx]UD3D9RenderDevice.m_pTexCoordArray[0]

		mov edi, eax
		shl edi, 4
		add edi, [ecx]UD3D9RenderDevice.m_pVertexColorArray

		//m_bufferedVerts += 3
		add eax, 3
		mov [ecx]UD3D9RenderDevice.m_bufferedVerts, eax

		lea eax, [ecx]UD3D9RenderDevice.TexInfo
		movss xmm0, [eax]FTexInfo.UMult
		movss xmm1, [eax]FTexInfo.VMult
		movss xmm2, f255

		//Pts in edx
		//Get PtsPlus12B
		lea esi, [edx + 12]

v_loop:
			mov eax, [edx]
			add edx, 4

			movss xmm3, [eax]FTransTexture.U
			mulss xmm3, xmm0
			movss [ebx]FGLTexCoord.u, xmm3
			movss xmm3, [eax]FTransTexture.V
			mulss xmm3, xmm1
			movss [ebx]FGLTexCoord.v, xmm3
			add ebx, TYPE FGLTexCoord

			mov ecx, [eax]FOutVector.Point.X
			mov [edi]FGLVertexColor.x, ecx
			mov ecx, [eax]FOutVector.Point.Y
			mov [edi]FGLVertexColor.y, ecx
			mov ecx, [eax]FOutVector.Point.Z
			mov [edi]FGLVertexColor.z, ecx

			movss xmm3, [eax]FTransSample.Light + 0
			mulss xmm3, xmm2
			movss xmm4, [eax]FTransSample.Light + 4
			mulss xmm4, xmm2
			movss xmm5, [eax]FTransSample.Light + 8
			mulss xmm5, xmm2
			cvtss2si eax, xmm3
			shl eax, 16
			cvtss2si ecx, xmm4
			and ecx, 255
			shl ecx, 8
			or eax, ecx
			cvtss2si ecx, xmm5
			and ecx, 255
			or ecx, 0xFF000000
			or eax, ecx
			mov [edi]FGLVertexColor.color, eax
			add edi, TYPE FGLVertexColor

			cmp edx, esi
			jne v_loop

		pop edi
		pop esi
		pop ebx

		ret
	}
}

__declspec(naked) static void FASTCALL Buffer3ColoredVerts_SSE2(UD3D9RenderDevice *pRD, FTransTexture** Pts) {
	static __m128 fColorMul = { 255.0f, 255.0f, 255.0f, 0.0f };
	static DWORD alphaOr = 0xFF000000;
	__asm {
		//pRD is in ecx
		//Pts is in edx

		push ebx
		push esi
		push edi

		mov eax, [ecx]UD3D9RenderDevice.m_bufferedVerts

		lea ebx, [eax*8]
		add ebx, [ecx]UD3D9RenderDevice.m_pTexCoordArray[0]

		mov edi, eax
		shl edi, 4
		add edi, [ecx]UD3D9RenderDevice.m_pVertexColorArray

		//m_bufferedVerts += 3
		add eax, 3
		mov [ecx]UD3D9RenderDevice.m_bufferedVerts, eax

		lea eax, [ecx]UD3D9RenderDevice.TexInfo
		movss xmm0, [eax]FTexInfo.UMult
		movss xmm1, [eax]FTexInfo.VMult
		movaps xmm2, fColorMul
		movd xmm3, alphaOr

		//Pts in edx
		//Get PtsPlus12B
		lea esi, [edx + 12]

v_loop:
			mov eax, [edx]
			add edx, 4

			movss xmm4, [eax]FTransTexture.U
			mulss xmm4, xmm0
			movss [ebx]FGLTexCoord.u, xmm4
			movss xmm4, [eax]FTransTexture.V
			mulss xmm4, xmm1
			movss [ebx]FGLTexCoord.v, xmm4
			add ebx, TYPE FGLTexCoord

			mov ecx, [eax]FOutVector.Point.X
			mov [edi]FGLVertexColor.x, ecx
			mov ecx, [eax]FOutVector.Point.Y
			mov [edi]FGLVertexColor.y, ecx
			mov ecx, [eax]FOutVector.Point.Z
			mov [edi]FGLVertexColor.z, ecx

			movups xmm4, [eax]FTransSample.Light
			shufps xmm4, xmm4, 0xC6
			mulps xmm4, xmm2
			cvtps2dq xmm4, xmm4
			packssdw xmm4, xmm4
			packuswb xmm4, xmm4
			por xmm4, xmm3
			movd [edi]FGLVertexColor.color, xmm4
			add edi, TYPE FGLVertexColor

			cmp edx, esi
			jne v_loop

		pop edi
		pop esi
		pop ebx

		ret
	}
}
#endif

#ifdef UTGLR_INCLUDE_SSE_CODE
__declspec(naked) static void FASTCALL Buffer3FoggedVerts_SSE(UD3D9RenderDevice *pRD, FTransTexture** Pts) {
	static float f255 = 255.0f;
	static float f1 = 1.0f;
	__asm {
		//pRD is in ecx
		//Pts is in edx

		push ebx
		push esi
		push edi
		push ebp
		sub esp, 4

		mov eax, [ecx]UD3D9RenderDevice.m_bufferedVerts

		lea ebx, [eax*8]
		add ebx, [ecx]UD3D9RenderDevice.m_pTexCoordArray[0]

		mov edi, eax
		shl edi, 4
		add edi, [ecx]UD3D9RenderDevice.m_pVertexColorArray

		lea esi, [eax*4]
		add esi, [ecx]UD3D9RenderDevice.m_pSecondaryColorArray

		//m_bufferedVerts += 3
		add eax, 3
		mov [ecx]UD3D9RenderDevice.m_bufferedVerts, eax

		lea eax, [ecx]UD3D9RenderDevice.TexInfo
		movss xmm0, [eax]FTexInfo.UMult
		movss xmm1, [eax]FTexInfo.VMult
		movss xmm2, f255

		//Pts in edx
		//Get PtsPlus12B
		lea ebp, [edx + 12]

v_loop:
			mov eax, [edx]
			add edx, 4

			movss xmm3, [eax]FTransTexture.U
			mulss xmm3, xmm0
			movss [ebx]FGLTexCoord.u, xmm3
			movss xmm3, [eax]FTransTexture.V
			mulss xmm3, xmm1
			movss [ebx]FGLTexCoord.v, xmm3
			add ebx, TYPE FGLTexCoord

			mov [esp], ebx

			movss xmm6, f1
			subss xmm6, [eax]FTransSample.Fog + 12
			mulss xmm6, xmm2

			mov ecx, [eax]FOutVector.Point.X
			mov [edi]FGLVertexColor.x, ecx
			mov ecx, [eax]FOutVector.Point.Y
			mov [edi]FGLVertexColor.y, ecx
			mov ecx, [eax]FOutVector.Point.Z
			mov [edi]FGLVertexColor.z, ecx

			movss xmm3, [eax]FTransSample.Light + 0
			mulss xmm3, xmm6
			movss xmm4, [eax]FTransSample.Light + 4
			mulss xmm4, xmm6
			movss xmm5, [eax]FTransSample.Light + 8
			mulss xmm5, xmm6
			cvtss2si ebx, xmm3
			shl ebx, 16
			cvtss2si ecx, xmm4
			and ecx, 255
			shl ecx, 8
			or ebx, ecx
			cvtss2si ecx, xmm5
			and ecx, 255
			or ecx, 0xFF000000
			or ebx, ecx
			mov [edi]FGLVertexColor.color, ebx
			add edi, TYPE FGLVertexColor

			mov ebx, [esp]

			movss xmm3, [eax]FTransSample.Fog + 0
			mulss xmm3, xmm2
			movss xmm4, [eax]FTransSample.Fog + 4
			mulss xmm4, xmm2
			movss xmm5, [eax]FTransSample.Fog + 8
			mulss xmm5, xmm2
			cvtss2si eax, xmm3
			and eax, 255
			shl eax, 16
			cvtss2si ecx, xmm4
			and ecx, 255
			shl ecx, 8
			or eax, ecx
			cvtss2si ecx, xmm5
			and ecx, 255
			or eax, ecx
			mov [esi]FGLSecondaryColor.specular, eax
			add esi, TYPE FGLSecondaryColor

			cmp edx, ebp
			jne v_loop

		add esp, 4
		pop ebp
		pop edi
		pop esi
		pop ebx

		ret
	}
}

__declspec(naked) static void FASTCALL Buffer3FoggedVerts_SSE2(UD3D9RenderDevice *pRD, FTransTexture** Pts) {
	static __m128 fColorMul = { 255.0f, 255.0f, 255.0f, 0.0f };
	static DWORD alphaOr = 0xFF000000;
	static float f1 = 1.0f;
	__asm {
		//pRD is in ecx
		//Pts is in edx

		push ebx
		push esi
		push edi
		push ebp

		mov eax, [ecx]UD3D9RenderDevice.m_bufferedVerts

		lea ebx, [eax*8]
		add ebx, [ecx]UD3D9RenderDevice.m_pTexCoordArray[0]

		mov edi, eax
		shl edi, 4
		add edi, [ecx]UD3D9RenderDevice.m_pVertexColorArray

		lea esi, [eax*4]
		add esi, [ecx]UD3D9RenderDevice.m_pSecondaryColorArray

		//m_bufferedVerts += 3
		add eax, 3
		mov [ecx]UD3D9RenderDevice.m_bufferedVerts, eax

		lea eax, [ecx]UD3D9RenderDevice.TexInfo
		movss xmm0, [eax]FTexInfo.UMult
		movss xmm1, [eax]FTexInfo.VMult
		movaps xmm2, fColorMul
		movd xmm3, alphaOr

		//Pts in edx
		//Get PtsPlus12B
		lea ebp, [edx + 12]

v_loop:
			mov eax, [edx]
			add edx, 4

			movss xmm4, [eax]FTransTexture.U
			mulss xmm4, xmm0
			movss [ebx]FGLTexCoord.u, xmm4
			movss xmm4, [eax]FTransTexture.V
			mulss xmm4, xmm1
			movss [ebx]FGLTexCoord.v, xmm4
			add ebx, TYPE FGLTexCoord

			movss xmm6, f1
			subss xmm6, [eax]FTransSample.Fog + 12
			mulss xmm6, xmm2
			shufps xmm6, xmm6, 0x00

			mov ecx, [eax]FOutVector.Point.X
			mov [edi]FGLVertexColor.x, ecx
			mov ecx, [eax]FOutVector.Point.Y
			mov [edi]FGLVertexColor.y, ecx
			mov ecx, [eax]FOutVector.Point.Z
			mov [edi]FGLVertexColor.z, ecx

			movups xmm4, [eax]FTransSample.Light
			shufps xmm4, xmm4, 0xC6
			mulps xmm4, xmm6
			cvtps2dq xmm4, xmm4
			packssdw xmm4, xmm4
			packuswb xmm4, xmm4
			por xmm4, xmm3
			movd [edi]FGLVertexColor.color, xmm4
			add edi, TYPE FGLVertexColor

			movups xmm4, [eax]FTransSample.Fog
			shufps xmm4, xmm4, 0xC6
			mulps xmm4, xmm2
			cvtps2dq xmm4, xmm4
			packssdw xmm4, xmm4
			packuswb xmm4, xmm4
			movd [esi]FGLSecondaryColor.specular, xmm4
			add esi, TYPE FGLSecondaryColor

			cmp edx, ebp
			jne v_loop

		pop ebp
		pop edi
		pop esi
		pop ebx

		ret
	}
}
#endif


void UD3D9RenderDevice::BuildGammaRamp(float redGamma, float greenGamma, float blueGamma, int brightness, D3DGAMMARAMP &ramp) {
	unsigned int u;

	//Parameter clamping
	if (brightness < -50) brightness = -50;
	if (brightness > 50) brightness = 50;

	float rcpRedGamma = 1.0f / (2.5f * redGamma);
	float rcpGreenGamma = 1.0f / (2.5f * greenGamma);
	float rcpBlueGamma = 1.0f / (2.5f * blueGamma);
	for (u = 0; u < 256; u++) {
		int iVal;
		int iValRed, iValGreen, iValBlue;

		//Initial value
		iVal = u;

		//Brightness
		iVal += brightness;
		//Clamping
		if (iVal < 0) iVal = 0;
		if (iVal > 255) iVal = 255;

		//Gamma
		iValRed = (int)appRound((float)appPow(iVal / 255.0f, rcpRedGamma) * 65535.0f);
		iValGreen = (int)appRound((float)appPow(iVal / 255.0f, rcpGreenGamma) * 65535.0f);
		iValBlue = (int)appRound((float)appPow(iVal / 255.0f, rcpBlueGamma) * 65535.0f);

		//Save results
		ramp.red[u] = (_WORD)iValRed;
		ramp.green[u] = (_WORD)iValGreen;
		ramp.blue[u] = (_WORD)iValBlue;
	}

	return;
}

void UD3D9RenderDevice::BuildGammaRamp(float redGamma, float greenGamma, float blueGamma, int brightness, FByteGammaRamp &ramp) {
	unsigned int u;

	//Parameter clamping
	if (brightness < -50) brightness = -50;
	if (brightness > 50) brightness = 50;

	float rcpRedGamma = 1.0f / (2.5f * redGamma);
	float rcpGreenGamma = 1.0f / (2.5f * greenGamma);
	float rcpBlueGamma = 1.0f / (2.5f * blueGamma);
	for (u = 0; u < 256; u++) {
		int iVal;
		int iValRed, iValGreen, iValBlue;

		//Initial value
		iVal = u;

		//Brightness
		iVal += brightness;
		//Clamping
		if (iVal < 0) iVal = 0;
		if (iVal > 255) iVal = 255;

		//Gamma
		iValRed = (int)appRound((float)appPow(iVal / 255.0f, rcpRedGamma) * 255.0f);
		iValGreen = (int)appRound((float)appPow(iVal / 255.0f, rcpGreenGamma) * 255.0f);
		iValBlue = (int)appRound((float)appPow(iVal / 255.0f, rcpBlueGamma) * 255.0f);

		//Save results
		ramp.red[u] = (BYTE)iValRed;
		ramp.green[u] = (BYTE)iValGreen;
		ramp.blue[u] = (BYTE)iValBlue;
	}

	return;
}

void UD3D9RenderDevice::SetGamma(FLOAT GammaCorrection) {
	D3DGAMMARAMP gammaRamp;

	GammaCorrection += GammaOffset;

	//Do not attempt to set gamma if <= zero
	if (GammaCorrection <= 0.0f) {
		return;
	}

	BuildGammaRamp(GammaCorrection + GammaOffsetRed, GammaCorrection + GammaOffsetGreen, GammaCorrection + GammaOffsetBlue, Brightness, gammaRamp);

	m_setGammaRampSucceeded = false;
	m_d3dDevice->SetGammaRamp(0, D3DSGR_NO_CALIBRATION, &gammaRamp);
	if (1) {
		m_setGammaRampSucceeded = true;
		SavedGammaCorrection = GammaCorrection;
	}

	return;
}

void UD3D9RenderDevice::ResetGamma(void) {
	return;
}



UBOOL UD3D9RenderDevice::FailedInitf(const TCHAR* Fmt, ...) {
	TCHAR TempStr[4096];
	GET_VARARGS(TempStr, ARRAY_COUNT(TempStr), Fmt);
	debugf(NAME_Init, TempStr);
	Exit();
	return 0;
}

void UD3D9RenderDevice::Exit() {
	guard(UD3D9RenderDevice::Exit);
	check(NumDevices > 0);

	//Shutdown D3D
	if (m_d3d9) {
		UnsetRes();
	}

	//Reset gamma ramp
	ResetGamma();

	//Timer shutdown
	ShutdownFrameRateLimitTimer();

	//Shut down global D3D
	if (--NumDevices == 0) {
#if 0
		//Free modules
		if (hModuleD3d9) {
			verify(FreeLibrary(hModuleD3d9));
			hModuleD3d9 = NULL;
		}
#endif
	}

	unguard;
}

void UD3D9RenderDevice::ShutdownAfterError() {
	guard(UD3D9RenderDevice::ShutdownAfterError);

	debugf(NAME_Exit, TEXT("UD3D9RenderDevice::ShutdownAfterError"));

	if (DebugBit(DEBUG_BIT_BASIC)) {
		dbgPrintf("utd3d9r: ShutdownAfterError\n");
	}

	//ChangeDisplaySettings(NULL, 0);

	//Reset gamma ramp
	ResetGamma();

	unguard;
}


UBOOL UD3D9RenderDevice::SetRes(INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen) {
	guard(UD3D9RenderDevice::SetRes);

	HRESULT hResult;
	bool saved_SetRes_isDeviceReset;

	//Get debug bits
	{
		INT i = 0;
		if (!GConfig->GetInt(g_pSection, TEXT("DebugBits"), i)) i = 0;
		m_debugBits = i;
	}
	//Display debug bits
	if (DebugBit(DEBUG_BIT_ANY)) dbgPrintf("utd3d9r: DebugBits = %u\n", m_debugBits);

	DecompFormat = TEXF_RGBA8_;

	debugfSlow(TEXT("Enter SetRes()"));

	//Save parameters in case need to reset device
	m_SetRes_NewX = NewX;
	m_SetRes_NewY = NewY;
	m_SetRes_NewColorBytes = NewColorBytes;
	m_SetRes_Fullscreen = Fullscreen;

	//Save copy of SetRes is device reset flag
	saved_SetRes_isDeviceReset = m_SetRes_isDeviceReset;
	//Reset SetRes is device reset flag
	m_SetRes_isDeviceReset = false;

	// If not fullscreen, and color bytes hasn't changed, do nothing.
	//If SetRes called due to device reset, do full destroy/recreate
	if (m_d3dDevice && !saved_SetRes_isDeviceReset && !Fullscreen && !WasFullscreen && (NewColorBytes == Viewport->ColorBytes)) {
		//Resize viewport
		if (!Viewport->ResizeViewport(BLIT_HardwarePaint | BLIT_Direct3D, NewX, NewY, NewColorBytes)) {
			return 0;
		}

		//Free old resources if they exist
		FreePermanentResources();

		//Get real viewport size
		NewX = Viewport->SizeX;
		NewY = Viewport->SizeY;

		//Don't break editor and tiny windowed mode
		if (NewX < 16) NewX = 16;
		if (NewY < 16) NewY = 16;

		//Set new size
		m_d3dpp.BackBufferWidth = NewX;
		m_d3dpp.BackBufferHeight = NewY;

		//Reset device
		hResult = m_d3dDevice->Reset(&m_d3dpp);
		if (FAILED(hResult)) {
			appErrorf(TEXT("Failed to create D3D device for new window size"));
		}

		//Initialize permanent rendering state, including allocation of some resources
		InitPermanentResourcesAndRenderingState();

		//Set viewport
		D3DVIEWPORT9 d3dViewport;
		d3dViewport.X = 0;
		d3dViewport.Y = 0;
		d3dViewport.Width = NewX;
		d3dViewport.Height = NewY;
		d3dViewport.MinZ = 0.0f;
		d3dViewport.MaxZ = 1.0f;
		m_d3dDevice->SetViewport(&d3dViewport);

		return 1;
	}


	// Exit res.
	if (m_d3d9) {
		debugf(TEXT("UnSetRes() -> m_d3d9 != NULL"));
		UnsetRes();
	}

	//Search for closest resolution match if fullscreen requested
	//No longer changing resolution here
	if (Fullscreen) {
		INT FindX = NewX, FindY = NewY, BestError = MAXINT;
		for (INT i = 0; i < Modes.Num(); i++) {
			if (Modes(i).Z==NewColorBytes*8) {
				INT Error
				=	(Modes(i).X-FindX)*(Modes(i).X-FindX)
				+	(Modes(i).Y-FindY)*(Modes(i).Y-FindY);
				if (Error < BestError) {
					NewX      = Modes(i).X;
					NewY      = Modes(i).Y;
					BestError = Error;
				}
			}
		}
	}

	// Change window size.
	UBOOL Result = Viewport->ResizeViewport(Fullscreen ? (BLIT_Fullscreen | BLIT_Direct3D) : (BLIT_HardwarePaint | BLIT_Direct3D), NewX, NewY, NewColorBytes);
	if (!Result) {
		return 0;
	}


	//Create main D3D9 object
	m_d3d9 = pDirect3DCreate9(D3D_SDK_VERSION);
	if (!m_d3d9) {
		appErrorf(TEXT("Direct3DCreate9 failed"));
	}


	//Get D3D caps
	hResult = m_d3d9->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &m_d3dCaps);
	if (FAILED(hResult)) {
		appErrorf(TEXT("GetDeviceCaps failed (0x%08X)"), hResult);
	}

	// Check if hardware supports bump mapping.
	g_bSupportsBumpMap = ((m_d3dCaps.TextureOpCaps & (D3DTEXOPCAPS_BUMPENVMAP | D3DTEXOPCAPS_BUMPENVMAPLUMINANCE)) // Does this device support the two bump mapping blend operations?
		&& m_d3dCaps.MaxTextureBlendStages >= 3); // Does this device support up to three blending stages?
	debugfSlow(TEXT("D3D supports bumpmap: %ls"), g_bSupportsBumpMap ? GTrue : GFalse);

	//Get adapter identifier for other vendor specific checks
	m_isATI = false;
	m_isNVIDIA = false;
	{
		D3DADAPTER_IDENTIFIER9 ident;

		hResult = m_d3d9->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &ident);
		if (FAILED(hResult)) {
			appErrorf(TEXT("GetAdapterIdentifier failed (0x%08X)"), hResult);
		}

		debugf(TEXT("D3D adapter driver      : %ls"), appFromAnsi(ident.Driver));
		debugf(TEXT("D3D adapter description : %ls"), appFromAnsi(ident.Description));
		debugf(TEXT("D3D adapter id          : 0x%04X:0x%04X"), ident.VendorId, ident.DeviceId);

		//Identifying vendor based on vendor id number
		if (ident.VendorId == 0x1002) {
			m_isATI = true;
		}
		else if (ident.VendorId == 0x10DE) {
			m_isNVIDIA = true;
		}
	}


	//Create D3D device

	//Get current display mode
	D3DDISPLAYMODE d3ddm;
	hResult = m_d3d9->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);
	if (FAILED(hResult)) {
		appErrorf(TEXT("Failed to get current display mode (0x%08X)"), hResult);
	}

	//Check if SetRes device reset
	//If so, get current bit depth
	//But don't check if was fullscreen
	if (saved_SetRes_isDeviceReset && !Fullscreen) {
		switch (d3ddm.Format) {
		case D3DFMT_R5G6B5: NewColorBytes = 2; break;
		case D3DFMT_X1R5G5B5: NewColorBytes = 2; break;
		case D3DFMT_A1R5G5B5: NewColorBytes = 2; break;
		default:
			NewColorBytes = 4;
		}
	}
	//Update saved NewColorBytes
	m_SetRes_NewColorBytes = NewColorBytes;

	//Don't break editor and tiny windowed mode
	if (NewX < 16) NewX = 16;
	if (NewY < 16) NewY = 16;

	//Set presentation parameters
	appMemzero(&m_d3dpp, sizeof(m_d3dpp));
	m_d3dpp.Windowed = TRUE;
	m_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	m_d3dpp.BackBufferWidth = NewX;
	m_d3dpp.BackBufferHeight = NewY;
	m_d3dpp.BackBufferFormat = d3ddm.Format;
	m_d3dpp.EnableAutoDepthStencil = TRUE;

	//Check if should be full screen
	if (Fullscreen) {
		m_d3dpp.Windowed = FALSE;
		m_d3dpp.BackBufferFormat = (NewColorBytes <= 2) ? D3DFMT_R5G6B5 : D3DFMT_X8R8G8B8;
	}

	//Choose initial depth buffer format
	m_d3dpp.AutoDepthStencilFormat = D3DFMT_D32;
	m_numDepthBits = 32;

	//Reduce depth buffer format if necessary based on what's supported
	if (m_d3dpp.AutoDepthStencilFormat == D3DFMT_D32) {
		if (!CheckDepthFormat(d3ddm.Format, m_d3dpp.BackBufferFormat, D3DFMT_D32)) {
			m_d3dpp.AutoDepthStencilFormat = D3DFMT_D24X8;
			m_numDepthBits = 24;
		}
	}
	if (m_d3dpp.AutoDepthStencilFormat == D3DFMT_D24X8) {
		if (!CheckDepthFormat(d3ddm.Format, m_d3dpp.BackBufferFormat, D3DFMT_D24X8)) {
			m_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
			m_numDepthBits = 16;
		}
	}

	m_usingAA = false;
	m_curAAEnable = true;
	m_defAAEnable = true;
	m_initNumAASamples = NumAASamples;

	//Select AA mode
	if (UseAA) {
		D3DMULTISAMPLE_TYPE MultiSampleType;

		switch (NumAASamples) {
		case  0: MultiSampleType = D3DMULTISAMPLE_NONE; break;
		case  1: MultiSampleType = D3DMULTISAMPLE_NONE; break;
		case  2: MultiSampleType = D3DMULTISAMPLE_2_SAMPLES; break;
		case  3: MultiSampleType = D3DMULTISAMPLE_3_SAMPLES; break;
		case  4: MultiSampleType = D3DMULTISAMPLE_4_SAMPLES; break;
		case  5: MultiSampleType = D3DMULTISAMPLE_5_SAMPLES; break;
		case  6: MultiSampleType = D3DMULTISAMPLE_6_SAMPLES; break;
		case  7: MultiSampleType = D3DMULTISAMPLE_7_SAMPLES; break;
		case  8: MultiSampleType = D3DMULTISAMPLE_8_SAMPLES; break;
		case  9: MultiSampleType = D3DMULTISAMPLE_9_SAMPLES; break;
		case 10: MultiSampleType = D3DMULTISAMPLE_10_SAMPLES; break;
		case 11: MultiSampleType = D3DMULTISAMPLE_11_SAMPLES; break;
		case 12: MultiSampleType = D3DMULTISAMPLE_12_SAMPLES; break;
		case 13: MultiSampleType = D3DMULTISAMPLE_13_SAMPLES; break;
		case 14: MultiSampleType = D3DMULTISAMPLE_14_SAMPLES; break;
		case 15: MultiSampleType = D3DMULTISAMPLE_15_SAMPLES; break;
		case 16: MultiSampleType = D3DMULTISAMPLE_16_SAMPLES; break;
		default:
			MultiSampleType = D3DMULTISAMPLE_NONE;
		}
		m_d3dpp.MultiSampleType = MultiSampleType;

		hResult = m_d3d9->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_d3dpp.BackBufferFormat, m_d3dpp.Windowed, m_d3dpp.MultiSampleType, NULL);
		if (FAILED(hResult)) {
			m_d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
		}
		hResult = m_d3d9->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_d3dpp.AutoDepthStencilFormat, m_d3dpp.Windowed, m_d3dpp.MultiSampleType, NULL);
		if (FAILED(hResult)) {
			m_d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
		}

		if (m_d3dpp.MultiSampleType != D3DMULTISAMPLE_NONE) {
			m_usingAA = true;
		}
	}

	//Set swap interval
	if (UseVSync >= 0) {
		switch (UseVSync) {
		case 0:
			if (m_d3dCaps.PresentationIntervals & D3DPRESENT_INTERVAL_IMMEDIATE) {
				m_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
			}
			break;
		case 1:
			if (m_d3dCaps.PresentationIntervals & D3DPRESENT_INTERVAL_ONE) {
				m_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
			}
			break;
		default:
			;
		}
	}


	//Set increased back buffer count if using triple buffering
	if (UseTripleBuffering) {
		m_d3dpp.BackBufferCount = 2;
	}

	//Set initial HW/SW vertex processing preference
	m_doSoftwareVertexInit = false;
	if (UseSoftwareVertexProcessing) {
		m_doSoftwareVertexInit = true;
	}

	//Try HW vertex init if not forcing SW vertex init
	if (!m_doSoftwareVertexInit) {
		bool tryDefaultRefreshRate = true;
		DWORD behaviorFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING;

		//Check if should use pure device
		if (UsePureDevice && (m_d3dCaps.DevCaps & D3DDEVCAPS_PUREDEVICE)) {
			behaviorFlags |= D3DCREATE_PUREDEVICE;
		}

		//Possibly attempt to set refresh rate if fullscreen
		if (!m_d3dpp.Windowed && (RefreshRate > 0)) {
			//Attempt to create with specific refresh rate
			m_d3dpp.FullScreen_RefreshRateInHz = RefreshRate;
			hResult = m_d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hWnd, behaviorFlags, &m_d3dpp, &m_d3dDevice);
			//Try again if triple buffering failed
			if (FAILED(hResult) && UseTripleBuffering && (m_d3dpp.BackBufferCount != 2)) {
				hResult = m_d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hWnd, behaviorFlags, &m_d3dpp, &m_d3dDevice);
			}
			if (FAILED(hResult)) {
				debugf(NAME_Init, TEXT("Failed to create D3D device with a refreshrate of %i and hardware vertex processing (0x%08X)"), RefreshRate, hResult);
			}
			else {
				tryDefaultRefreshRate = false;
			}
		}

		if (tryDefaultRefreshRate) {
			//Attempt to create with default refresh rate
			m_d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
			hResult = m_d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hWnd, behaviorFlags, &m_d3dpp, &m_d3dDevice);
			//Try again if triple buffering failed
			if (FAILED(hResult) && UseTripleBuffering && (m_d3dpp.BackBufferCount != 2)) {
				hResult = m_d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hWnd, behaviorFlags, &m_d3dpp, &m_d3dDevice);
			}
			if (FAILED(hResult)) {
				debugf(NAME_Init, TEXT("Failed to create D3D device with hardware vertex processing (0x%08X)"), hResult);
				m_doSoftwareVertexInit = true;
			}
		}
	}
	//Try SW vertex init if forced earlier or if HW vertex init failed
	if (m_doSoftwareVertexInit) {
		bool tryDefaultRefreshRate = true;
		DWORD behaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

		//Possibly attempt to set refresh rate if fullscreen
		if (!m_d3dpp.Windowed && (RefreshRate > 0)) {
			//Attempt to create with specific refresh rate
			m_d3dpp.FullScreen_RefreshRateInHz = RefreshRate;
			hResult = m_d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hWnd, behaviorFlags, &m_d3dpp, &m_d3dDevice);
			//Try again if triple buffering failed
			if (FAILED(hResult) && UseTripleBuffering && (m_d3dpp.BackBufferCount != 2)) {
				hResult = m_d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hWnd, behaviorFlags, &m_d3dpp, &m_d3dDevice);
			}
			if (FAILED(hResult)) {
				debugf(NAME_Init, TEXT("Failed to create D3D device with software vertex processing (0x%08X)"), hResult);
			}
			else {
				tryDefaultRefreshRate = false;
			}
		}

		if (tryDefaultRefreshRate) {
			//Attempt to create with default refresh rate
			m_d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
			hResult = m_d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hWnd, behaviorFlags, &m_d3dpp, &m_d3dDevice);
			//Try again if triple buffering failed
			if (FAILED(hResult) && UseTripleBuffering && (m_d3dpp.BackBufferCount != 2)) {
				hResult = m_d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hWnd, behaviorFlags, &m_d3dpp, &m_d3dDevice);
			}
			if (FAILED(hResult)) {
				appErrorf(TEXT("Failed to create D3D device (0x%08X)"), hResult);
			}
		}
	}


	//Reset previous SwapBuffers status to okay
	m_prevSwapBuffersStatus = true;

	debugf(NAME_Init, TEXT("Depth bits: %u"), m_numDepthBits);
	if (DebugBit(DEBUG_BIT_BASIC)) dbgPrintf("utd3d9r: Depth bits: %u\n", m_numDepthBits);
	if (m_usingAA) {
		debugf(NAME_Init, TEXT("AA samples: %u"), m_initNumAASamples);
		if (DebugBit(DEBUG_BIT_BASIC)) dbgPrintf("utd3d9r: AA samples: %u\n", m_initNumAASamples);
	}

	//Get other defaults
	if (!GConfig->GetInt(g_pSection, TEXT("Brightness"), Brightness)) Brightness = 0;

	//Debug parameter listing
	if (DebugBit(DEBUG_BIT_BASIC)) {
		#define UTGLR_DEBUG_SHOW_PARAM_REG(name) DbgPrintInitParam(#name, name)
		#define UTGLR_DEBUG_SHOW_PARAM_DCV(name) DbgPrintInitParam(#name, DCV.name)

		UTGLR_DEBUG_SHOW_PARAM_REG(LODBias);
		UTGLR_DEBUG_SHOW_PARAM_REG(GammaOffset);
		UTGLR_DEBUG_SHOW_PARAM_REG(GammaOffsetRed);
		UTGLR_DEBUG_SHOW_PARAM_REG(GammaOffsetGreen);
		UTGLR_DEBUG_SHOW_PARAM_REG(GammaOffsetBlue);
		UTGLR_DEBUG_SHOW_PARAM_REG(Brightness);
		UTGLR_DEBUG_SHOW_PARAM_REG(GammaCorrectScreenshots);
		UTGLR_DEBUG_SHOW_PARAM_REG(OneXBlending);
		UTGLR_DEBUG_SHOW_PARAM_REG(MinLogTextureSize);
		UTGLR_DEBUG_SHOW_PARAM_REG(MaxLogTextureSize);
		UTGLR_DEBUG_SHOW_PARAM_REG(MaxAnisotropy);
		UTGLR_DEBUG_SHOW_PARAM_REG(TMUnits);
		UTGLR_DEBUG_SHOW_PARAM_REG(MaxTMUnits);
		UTGLR_DEBUG_SHOW_PARAM_REG(RefreshRate);
		UTGLR_DEBUG_SHOW_PARAM_REG(UseMultiTexture);
		UTGLR_DEBUG_SHOW_PARAM_REG(UsePrecache);
		UTGLR_DEBUG_SHOW_PARAM_REG(UseTrilinear);
//		UTGLR_DEBUG_SHOW_PARAM_REG(UseVertexSpecular);
		UTGLR_DEBUG_SHOW_PARAM_REG(UseS3TC);
		UTGLR_DEBUG_SHOW_PARAM_REG(Use16BitTextures);
		UTGLR_DEBUG_SHOW_PARAM_REG(Use565Textures);
		UTGLR_DEBUG_SHOW_PARAM_REG(NoFiltering);
		UTGLR_DEBUG_SHOW_PARAM_REG(DetailMax);
//		UTGLR_DEBUG_SHOW_PARAM_REG(UseDetailAlpha);
		UTGLR_DEBUG_SHOW_PARAM_REG(DetailClipping);
		UTGLR_DEBUG_SHOW_PARAM_REG(ColorizeDetailTextures);
		UTGLR_DEBUG_SHOW_PARAM_REG(SinglePassFog);
		UTGLR_DEBUG_SHOW_PARAM_DCV(SinglePassDetail);
//		UTGLR_DEBUG_SHOW_PARAM_REG(BufferActorTris);
//		UTGLR_DEBUG_SHOW_PARAM_REG(BufferClippedActorTris);
		UTGLR_DEBUG_SHOW_PARAM_REG(UseSSE);
		UTGLR_DEBUG_SHOW_PARAM_REG(UseSSE2);
		UTGLR_DEBUG_SHOW_PARAM_REG(UseTexIdPool);
		UTGLR_DEBUG_SHOW_PARAM_REG(UseTexPool);
		UTGLR_DEBUG_SHOW_PARAM_REG(CacheStaticMaps);
		UTGLR_DEBUG_SHOW_PARAM_REG(DynamicTexIdRecycleLevel);
		UTGLR_DEBUG_SHOW_PARAM_REG(TexDXT1ToDXT3);
		UTGLR_DEBUG_SHOW_PARAM_DCV(UseFragmentProgram);
		UTGLR_DEBUG_SHOW_PARAM_REG(UseVSync);
#if 0
		UTGLR_DEBUG_SHOW_PARAM_REG(FrameRateLimit);
#endif
		UTGLR_DEBUG_SHOW_PARAM_REG(SceneNodeHack);
		UTGLR_DEBUG_SHOW_PARAM_REG(SmoothMaskedTextures);
		UTGLR_DEBUG_SHOW_PARAM_REG(MaskedTextureHack);
		UTGLR_DEBUG_SHOW_PARAM_REG(UseTripleBuffering);
		UTGLR_DEBUG_SHOW_PARAM_REG(UsePureDevice);
		UTGLR_DEBUG_SHOW_PARAM_REG(UseSoftwareVertexProcessing);
		UTGLR_DEBUG_SHOW_PARAM_REG(UseAA);
		UTGLR_DEBUG_SHOW_PARAM_REG(NumAASamples);
		UTGLR_DEBUG_SHOW_PARAM_REG(NoAATiles);
		UTGLR_DEBUG_SHOW_PARAM_REG(ZRangeHack);

		#undef UTGLR_DEBUG_SHOW_PARAM_REG
		#undef UTGLR_DEBUG_SHOW_PARAM_DCV
	}


#ifdef UTGLR_INCLUDE_SSE_CODE
	if (UseSSE) {
		if (!CPU_DetectSSE()) {
			UseSSE = 0;
		}
	}
	if (UseSSE2) {
		if (!CPU_DetectSSE2()) {
			UseSSE2 = 0;
		}
	}
#else
	UseSSE = 0;
	UseSSE2 = 0;
#endif
	if (DebugBit(DEBUG_BIT_BASIC)) dbgPrintf("utd3d9r: UseSSE = %u\n", UseSSE);
	if (DebugBit(DEBUG_BIT_BASIC)) dbgPrintf("utd3d9r: UseSSE2 = %u\n", UseSSE2);

	SetGamma(Viewport->GetOuterUClient()->Brightness);

	//Restrict dynamic tex id recycle level range
	if (DynamicTexIdRecycleLevel < 10) DynamicTexIdRecycleLevel = 10;

	//Always use vertex specular unless caps check fails later
	UseVertexSpecular = 1;

	SupportsTC = UseS3TC;
	SupportsAlphaBlend = 1;
	SupportsHDLightmaps = UseEnhancedLightmaps;
	UnsupportHDLightFlags = PF_Masked; // TODO - Fix masked BSP textures with this.

	BufferActorTris = 1;
	BufferClippedActorTris = 1;

	UseDetailAlpha = 1;


	//Set DXT texture capability flags
	//Check for DXT1 support
	m_dxt1TextureCap = true;
	hResult = m_d3d9->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3ddm.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_DXT1);
	if (FAILED(hResult)) {
		m_dxt1TextureCap = false;
	}
	//Check for DXT3 support
	m_dxt3TextureCap = true;
	hResult = m_d3d9->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3ddm.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_DXT3);
	if (FAILED(hResult)) {
		m_dxt3TextureCap = false;
	}
	//Check for DXT5 support
	m_dxt5TextureCap = true;
	hResult = m_d3d9->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3ddm.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_DXT5);
	if (FAILED(hResult)) {
		m_dxt5TextureCap = false;
	}

	//Set 16-bit texture capability flags
	m_16BitTextureCap = true;
	m_565TextureCap = true;
	//Check RGBA5551
	hResult = m_d3d9->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3ddm.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_A1R5G5B5);
	if (FAILED(hResult)) {
		m_16BitTextureCap = false;
	}
	//Check RGB555
	hResult = m_d3d9->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3ddm.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_X1R5G5B5);
	if (FAILED(hResult)) {
		m_16BitTextureCap = false;
	}
	//Check RGB565
	hResult = m_d3d9->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3ddm.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_R5G6B5);
	if (FAILED(hResult)) {
		m_565TextureCap = false;
	}

	//Set alpha texture capability flag
	m_alphaTextureCap = true;
	hResult = m_d3d9->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3ddm.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_A8);
	if (FAILED(hResult)) {
		m_alphaTextureCap = false;
	}

	//Set alpha to coverage capability flag
	m_supportsAlphaToCoverage = false;
	//ATI says all cards supporting the DX9 feature set support alpha to coverage
	//NVIDIA has a way to detect alpha to coverage support
	//In this case will start filtering based on if supports ps3.0
	if (m_d3dCaps.PixelShaderVersion >= D3DPS_VERSION(3,0)) {
		if (m_isATI) {
			//Supported on ATI DX9 class HW
			m_supportsAlphaToCoverage = true;
		}
		else if (m_isNVIDIA) {
			//Check if alpha to coverage supported for NVIDIA
			hResult = m_d3d9->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
				D3DFMT_X8R8G8B8, 0, D3DRTYPE_SURFACE, (D3DFORMAT)MAKEFOURCC('A', 'T', 'O', 'C'));
			if (FAILED(hResult)) {
			}
			else {
				m_supportsAlphaToCoverage = true;
			}
		}
	}


	// Validate flags.

	//Special extensions validation for init only config pass
	if (!m_dxt1TextureCap) SupportsTC = 0;
#ifdef UTGLR_UNREAL_227_BUILD
	if (!m_dxt3TextureCap) SupportsTC = 0;
	if (!m_dxt5TextureCap) SupportsTC = 0;
#endif

	//DCV refresh
	ConfigValidate_RefreshDCV();

	//Required extensions config validation pass
	ConfigValidate_RequiredExtensions();


	if (!MaxTMUnits || (MaxTMUnits > MAX_TMUNITS)) {
		MaxTMUnits = MAX_TMUNITS;
	}

	if (UseMultiTexture) {
		TMUnits = m_d3dCaps.MaxSimultaneousTextures;
		debugf(TEXT("%i Texture Mapping Units found"), TMUnits);
		if (TMUnits > MaxTMUnits) {
			TMUnits = MaxTMUnits;
		}
	}
	else {
		TMUnits = 1;
	}


	//Main config validation pass (after set TMUnits)
	ConfigValidate_Main();


	if (MaxAnisotropy < 0) {
		MaxAnisotropy = 0;
	}
	if (MaxAnisotropy) {
		int iMaxAnisotropyLimit;
		iMaxAnisotropyLimit = m_d3dCaps.MaxAnisotropy;
		debugf(TEXT("MaxAnisotropy: %i"), iMaxAnisotropyLimit);
		if (MaxAnisotropy > iMaxAnisotropyLimit) {
			MaxAnisotropy = iMaxAnisotropyLimit;
		}
	}

	if (SupportsTC) {
		debugf(TEXT("Trying to use S3TC extension."));
	}

	//Use default if MaxLogTextureSize <= 0
	if (MaxLogTextureSize <= 0) MaxLogTextureSize = 12;

	INT D3DMaxTextureSize = Min(m_d3dCaps.MaxTextureWidth, m_d3dCaps.MaxTextureHeight);
	MaxTextureSize = D3DMaxTextureSize;
	INT Dummy = -1;
	while (D3DMaxTextureSize > 0) {
		D3DMaxTextureSize >>= 1;
		Dummy++;
	}

	if ((MaxLogTextureSize > Dummy) || (SupportsTC)) MaxLogTextureSize = Dummy;
	if ((MinLogTextureSize < 2) || (SupportsTC)) MinLogTextureSize = 2;

	MaxLogUOverV = MaxLogTextureSize;
	MaxLogVOverU = MaxLogTextureSize;
	if (SupportsTC) {
		//Current texture scaling code might not work well with compressed textures in certain cases
		//Hopefully no odd restrictions on hardware that supports compressed textures
	}
	else {
		INT MaxTextureAspectRatio = m_d3dCaps.MaxTextureAspectRatio;
		if (MaxTextureAspectRatio > 0) {
			INT MaxLogTextureAspectRatio = -1;
			while (MaxTextureAspectRatio > 0) {
				MaxTextureAspectRatio >>= 1;
				MaxLogTextureAspectRatio++;
			}
			if (MaxLogTextureAspectRatio < MaxLogUOverV) MaxLogUOverV = MaxLogTextureAspectRatio;
			if (MaxLogTextureAspectRatio < MaxLogVOverU) MaxLogVOverU = MaxLogTextureAspectRatio;
		}
	}

	debugf(TEXT("MaxTextureSize: %i"), MaxTextureSize);
	debugf(TEXT("MinLogTextureSize: %i"), MinLogTextureSize);
	debugf(TEXT("MaxLogTextureSize: %i"), MaxLogTextureSize);

	debugf(TEXT("UseDetailAlpha: %i"), UseDetailAlpha);


	//Set pointers to aligned memory
	MapDotArray = (FGLMapDot *)Align (m_MapDotArrayMem, VERTEX_ARRAY_ALIGN);


	// Verify hardware defaults.
	check(MinLogTextureSize >= 0);
	check(MaxLogTextureSize >= 0);
	check(MinLogTextureSize <= MaxLogTextureSize);

	// Flush textures.
	Flush(1);

	//Invalidate fixed texture ids
	m_pNoTexObj = NULL;
	m_pAlphaTexObj = NULL;

	//Initialize permanent rendering state, including allocation of some resources
	InitPermanentResourcesAndRenderingState();


	//Initialize previous lock variables
	PL_DetailTextures = DetailTextures;
	PL_OneXBlending = OneXBlending;
	PL_MaxLogUOverV = MaxLogUOverV;
	PL_MaxLogVOverU = MaxLogVOverU;
	PL_MinLogTextureSize = MinLogTextureSize;
	PL_MaxLogTextureSize = MaxLogTextureSize;
	PL_NoFiltering = NoFiltering;
	PL_UseTrilinear = UseTrilinear;
	PL_Use16BitTextures = Use16BitTextures;
	PL_Use565Textures = Use565Textures;
	PL_TexDXT1ToDXT3 = TexDXT1ToDXT3;
	PL_MaxAnisotropy = MaxAnisotropy;
	PL_SmoothMaskedTextures = SmoothMaskedTextures;
	PL_MaskedTextureHack = MaskedTextureHack;
	PL_LODBias = LODBias;
	PL_UseDetailAlpha = UseDetailAlpha;
	PL_SinglePassDetail = SinglePassDetail;
	PL_UseFragmentProgram = UseFragmentProgram;
	PL_UseSSE = UseSSE;
	PL_UseSSE2 = UseSSE2;


	//Reset current frame count
	m_currentFrameCount = 0;

	// Remember fullscreenness.
	WasFullscreen = Fullscreen;

	return 1;

	unguard;
}

void UD3D9RenderDevice::UnsetRes() {
	guard(UD3D9RenderDevice::UnsetRes);

	check(m_d3d9);
	check(m_d3dDevice);

	//Flush textures
	Flush(1);

	//Free fixed textures if they were allocated
	if (m_pNoTexObj) {
		m_pNoTexObj->Release();
		m_pNoTexObj = NULL;
	}
	if (m_pAlphaTexObj) {
		m_pAlphaTexObj->Release();
		m_pAlphaTexObj = NULL;
	}

	//Free permanent resources
	FreePermanentResources();

	//Release D3D device
	m_d3dDevice->Release();
	m_d3dDevice = NULL;

	//Release main D3D9 object
	m_d3d9->Release();
	m_d3d9 = NULL;

	unguard;
}


bool UD3D9RenderDevice::CheckDepthFormat(D3DFORMAT adapterFormat, D3DFORMAT backBufferFormat, D3DFORMAT depthBufferFormat) {
	HRESULT hResult;

	//Check depth format
	hResult = m_d3d9->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, adapterFormat, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, depthBufferFormat);
	if (FAILED(hResult)) {
		return false;
	}

	//Check depth format compatibility
	hResult = m_d3d9->CheckDepthStencilMatch(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, adapterFormat, backBufferFormat, depthBufferFormat);
	if (FAILED(hResult)) {
		return false;
	}

	return true;
}


void UD3D9RenderDevice::ConfigValidate_RefreshDCV(void) {
	#define UTGLR_REFRESH_DCV(_name) _name = DCV._name

	UTGLR_REFRESH_DCV(SinglePassDetail);
	UTGLR_REFRESH_DCV(UseFragmentProgram);

	#undef UTGLR_REFRESH_DCV

	return;
}

void UD3D9RenderDevice::ConfigValidate_RequiredExtensions(void) {
	//if (m_d3dCaps.PixelShaderVersion < D3DPS_VERSION(3,0)) UseFragmentProgram = 0;
	//if (m_d3dCaps.VertexShaderVersion < D3DVS_VERSION(3,0))
	UseFragmentProgram = 0; //FIXME: Disabled due to distance fog issues.
	SinglePassFog = 1; //FIXME: Enabled due to distance fog issues.
	if (!(m_d3dCaps.TextureOpCaps & D3DTEXOPCAPS_BLENDCURRENTALPHA)) DetailTextures = 0;
	if (!(m_d3dCaps.TextureOpCaps & D3DTEXOPCAPS_BLENDCURRENTALPHA)) UseDetailAlpha = 0;
	if (!m_alphaTextureCap) UseDetailAlpha = 0;
	if (!(m_d3dCaps.TextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC)) MaxAnisotropy = 0;
	if (!(m_d3dCaps.RasterCaps & D3DPRASTERCAPS_MIPMAPLODBIAS)) LODBias = 0;
	if (!m_dxt3TextureCap) TexDXT1ToDXT3 = 0;
	if (!m_16BitTextureCap) Use16BitTextures = 0;
	if (!m_565TextureCap) Use565Textures = 0;

	if (!(m_d3dCaps.TextureOpCaps & D3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR)) SinglePassFog = 0;

	//Force 1X blending if no 2X modulation support
	if (!(m_d3dCaps.TextureOpCaps & D3DTEXOPCAPS_MODULATE2X)) OneXBlending = 0x1;	//Must use proper bit offset for Bool param

	return;
}

void UD3D9RenderDevice::ConfigValidate_Main(void) {
	//Detail alpha requires at least two texture units
	if (TMUnits < 2) UseDetailAlpha = 0;

	//Single pass detail texturing requires at least 4 texture units
	if (TMUnits < 4) SinglePassDetail = 0;
	//Single pass detail texturing requires detail alpha
	if (!UseDetailAlpha) SinglePassDetail = 0;

	//Limit maximum DetailMax
	if (DetailMax > 3) DetailMax = 3;

	return;
}


void UD3D9RenderDevice::InitPermanentResourcesAndRenderingState(void) {
	guard(UD3D9RenderDevice::InitPermanentResourcesAndRenderingState);

	unsigned int u;
	HRESULT hResult;

#ifdef UTD3D9R_INCLUDE_SHADER_ASM
	AssembleShader();
#endif

	//Set view matrix
	D3DMATRIX d3dView = { +1.0f,  0.0f,  0.0f,  0.0f,
						   0.0f, -1.0f,  0.0f,  0.0f,
						   0.0f,  0.0f, -1.0f,  0.0f,
						   0.0f,  0.0f,  0.0f, +1.0f };
	m_d3dDevice->SetTransform(D3DTS_VIEW, &d3dView);

	//Little white texture for no texture operations
	InitNoTextureSafe();

	//Clear fsBlendInfo to set any unused values to known state
	m_fsBlendInfo[0] = 0.0f;
	m_fsBlendInfo[1] = 0.0f;
	m_fsBlendInfo[2] = 0.0f;
	m_fsBlendInfo[3] = 0.0f;

	//Set alpha test disabled
	m_fsBlendInfo[0] = 0.0f;

	m_d3dDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
	m_d3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
	SetZTestMode(ZTEST_LessEqual);

	m_d3dDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
	m_d3dDevice->SetRenderState(D3DRS_ALPHAREF, 127);

	m_d3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
	m_d3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);

	m_d3dDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
	m_d3dDevice->SetRenderState(D3DRS_DITHERENABLE, TRUE);

#ifdef UTGLR_RUNE_BUILD
	m_d3dDevice->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_LINEAR);
	FLOAT fFogStart = 0.0f;
	m_d3dDevice->SetRenderState(D3DRS_FOGSTART, *(DWORD *)&fFogStart);
	m_gpFogEnabled = false;
#elif UTGLR_UNREAL_227_BUILD
	m_d3dDevice->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_LINEAR);
#endif

	m_d3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	m_d3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	//Color and alpha modulation on texEnv0
	m_d3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	m_d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

	//Set default texture stage tracking values
	for (u = 0; u < MAX_TMUNITS; u++) {
		m_curTexStageParams[u] = CT_DEFAULT_TEX_PARAMS;
	}

	if (LODBias) {
		SetTexLODBiasState(TMUnits);
	}

	if (MaxAnisotropy) {
		SetTexMaxAnisotropyState(TMUnits);
	}

	if (UseDetailAlpha) {	// vogel: alpha texture for better detail textures (no vertex alpha)
		InitAlphaTextureSafe();
	}

	//Initialize texture environment state
	InitOrInvalidateTexEnvState();

	//Reset current texture ids to hopefully unused values
	for (u = 0; u < MAX_TMUNITS; u++) {
		TexInfo[u].CurrentCacheID = TEX_CACHE_ID_UNUSED;
		TexInfo[u].pBind = NULL;
	}


	//Create vertex buffers
	D3DPOOL vertexBufferPool = D3DPOOL_DEFAULT;

	//Vertex and primary color
	hResult = m_d3dDevice->CreateVertexBuffer(sizeof(FGLVertexColor) * VERTEX_ARRAY_SIZE, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, vertexBufferPool, &m_d3dVertexColorBuffer, NULL);
	if (FAILED(hResult)) {
		appErrorf(TEXT("CreateVertexBuffer failed"));
	}

	//Secondary color
	hResult = m_d3dDevice->CreateVertexBuffer(sizeof(FGLSecondaryColor) * VERTEX_ARRAY_SIZE, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, vertexBufferPool, &m_d3dSecondaryColorBuffer, NULL);
	if (FAILED(hResult)) {
		appErrorf(TEXT("CreateVertexBuffer failed"));
	}

	//TexCoord
	for (u = 0; u < TMUnits; u++) {
		hResult = m_d3dDevice->CreateVertexBuffer(sizeof(FGLTexCoord) * VERTEX_ARRAY_SIZE, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, vertexBufferPool, &m_d3dTexCoordBuffer[u], NULL);
		if (FAILED(hResult)) {
			appErrorf(TEXT("CreateVertexBuffer failed"));
		}
	}


	//Create stream definitions

	//Stream definition with vertices and color
	hResult = m_d3dDevice->CreateVertexDeclaration(g_oneColorStreamDef, &m_oneColorVertexDecl);
	if (FAILED(hResult)) {
		appErrorf(TEXT("CreateVertexDeclaration failed"));
	}

	//Standard stream definitions with vertices, color, and a variable number of tex coords
	for (u = 0; u < TMUnits; u++) {
		hResult = m_d3dDevice->CreateVertexDeclaration(g_standardNTextureStreamDefs[u], &m_standardNTextureVertexDecl[u]);
		if (FAILED(hResult)) {
			appErrorf(TEXT("CreateVertexDeclaration failed"));
		}
	}

	//Stream definition with vertices, two colors, and one tex coord
	hResult = m_d3dDevice->CreateVertexDeclaration(g_twoColorSingleTextureStreamDef, &m_twoColorSingleTextureVertexDecl);
	if (FAILED(hResult)) {
		appErrorf(TEXT("CreateVertexDeclaration failed"));
	}


	//Initialize vertex buffer state tracking information
	m_curVertexBufferPos = 0;
	m_vertexColorBufferNeedsDiscard = false;
	m_secondaryColorBufferNeedsDiscard = false;
	for (u = 0; u < MAX_TMUNITS; u++) {
		m_texCoordBufferNeedsDiscard[u] = false;
	}


	//Set stream sources
	//Vertex and primary color
	hResult = m_d3dDevice->SetStreamSource(0, m_d3dVertexColorBuffer, 0, sizeof(FGLVertexColor));
	if (FAILED(hResult)) {
		appErrorf(TEXT("SetStreamSource failed"));
	}

	//Secondary Color
	hResult = m_d3dDevice->SetStreamSource(1, m_d3dSecondaryColorBuffer, 0, sizeof(FGLSecondaryColor));
	if (FAILED(hResult)) {
		appErrorf(TEXT("SetStreamSource failed"));
	}

	//TexCoord
	for (u = 0; u < TMUnits; u++) {
		hResult = m_d3dDevice->SetStreamSource(2 + u, m_d3dTexCoordBuffer[u], 0, sizeof(FGLTexCoord));
		if (FAILED(hResult)) {
			appErrorf(TEXT("SetStreamSource failed"));
		}
	}


	//Setup fragment programs
	if (UseFragmentProgram) {
		//Attempt to initialize fragment program mode
		TryInitializeFragmentProgramMode();
	}


	//Set default stream definition
	hResult = m_d3dDevice->SetVertexDeclaration(m_standardNTextureVertexDecl[0]);
	if (FAILED(hResult)) {
		appErrorf(TEXT("SetVertexDeclaration failed"));
	}
	m_curVertexDecl = m_standardNTextureVertexDecl[0];

	//No vertex shader current at initialization
	m_curVertexShader = NULL;

	//No pixel shader current at initialization
	m_curPixelShader = NULL;


	//Initialize texture state cache information
	m_texEnableBits = 0x1;

	// Init variables.
	m_bufferedVertsType = BV_TYPE_NONE;
	m_bufferedVerts = 0;

	m_curBlendFlags = PF_Occlude;
	m_smoothMaskedTexturesBit = 0;
	m_useAlphaToCoverageForMasked = false;
	m_alphaToCoverageEnabled = false;
	m_alphaTestEnabled = false;
	m_curPolyFlags = 0;
	m_curPolyFlags2 = 0;

	//Initialize color flags
	m_requestedColorFlags = 0;

	//Initialize Z range hack state
	m_useZRangeHack = false;
	m_nearZRangeHackProjectionActive = false;
	m_requestNearZRangeHackProjection = false;


	unguard;
}

void UD3D9RenderDevice::FreePermanentResources(void) {
	guard(UD3D9RenderDevice::FreePermanentResources);

	unsigned int u;
	HRESULT hResult;

	//Free fragment programs if they were allocated and leave fragment program mode if necessary
	ShutdownFragmentProgramMode();


	//Unset stream sources
	//Vertex
	hResult = m_d3dDevice->SetStreamSource(0, NULL, 0, 0);
	if (FAILED(hResult)) {
		appErrorf(TEXT("SetStreamSource failed"));
	}

	//Secondary Color
	hResult = m_d3dDevice->SetStreamSource(1, NULL, 0, 0);
	if (FAILED(hResult)) {
		appErrorf(TEXT("SetStreamSource failed"));
	}

	//TexCoord
	for (u = 0; u < TMUnits; u++) {
		hResult = m_d3dDevice->SetStreamSource(2 + u, NULL, 0, 0);
		if (FAILED(hResult)) {
			appErrorf(TEXT("SetStreamSource failed"));
		}
	}


	//Free vertex buffers
	if (m_d3dVertexColorBuffer) {
		m_d3dVertexColorBuffer->Release();
		m_d3dVertexColorBuffer = NULL;
	}
	if (m_d3dSecondaryColorBuffer) {
		m_d3dSecondaryColorBuffer->Release();
		m_d3dSecondaryColorBuffer = NULL;
	}
	for (u = 0; u < TMUnits; u++) {
		if (m_d3dTexCoordBuffer[u]) {
			m_d3dTexCoordBuffer[u]->Release();
			m_d3dTexCoordBuffer[u] = NULL;
		}
	}


	//Set vertex declaration to something else so that it isn't using a current vertex declaration
	m_d3dDevice->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);

	//Free stream definitions
	//Standard stream definition with vertices and color
	if (m_oneColorVertexDecl) {
		m_oneColorVertexDecl->Release();
		m_oneColorVertexDecl = NULL;
	}
	//Standard stream definitions with vertices, color, and a variable number of tex coords
	for (u = 0; u < TMUnits; u++) {
		if (m_standardNTextureVertexDecl[u]) {
			m_standardNTextureVertexDecl[u]->Release();
			m_standardNTextureVertexDecl[u] = NULL;
		}
	}
	//Stream definition with vertices, two colors, and one tex coord
	if (m_twoColorSingleTextureVertexDecl) {
		m_twoColorSingleTextureVertexDecl->Release();
		m_twoColorSingleTextureVertexDecl = NULL;
	}

	unguard;
}


UBOOL UD3D9RenderDevice::Init(UViewport* InViewport, INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen) {
	guard(UD3D9RenderDevice::Init);

	debugf(TEXT("Initializing D3D9Drv..."));

	m_WithDistanceFog = false;
	m_NumClipPlanes = 0;
	m_LastZMode = 255;

	if (NumDevices == 0) {
		g_gammaFirstTime = true;
		g_haveOriginalGammaRamp = false;
	}

	// Get list of device modes.
	for (INT i = 0; ; i++) {
		UBOOL UnicodeOS;

#if defined(NO_UNICODE_OS_SUPPORT) || !defined(UNICODE)
		UnicodeOS = 0;
#elif defined(NO_ANSI_OS_SUPPORT)
		UnicodeOS = 1;
#else
		UnicodeOS = GUnicodeOS;
#endif

		if (!UnicodeOS) {
#if defined(NO_UNICODE_OS_SUPPORT) || !defined(UNICODE) || !defined(NO_ANSI_OS_SUPPORT)
			DEVMODEA Tmp;
			appMemzero(&Tmp, sizeof(Tmp));
			Tmp.dmSize = sizeof(Tmp);
			if (!EnumDisplaySettingsA(NULL, i, &Tmp)) {
				break;
			}
			Modes.AddUniqueItem(FPlane(Tmp.dmPelsWidth, Tmp.dmPelsHeight, Tmp.dmBitsPerPel, Tmp.dmDisplayFrequency));
#endif
		}
		else {
#if !defined(NO_UNICODE_OS_SUPPORT) && defined(UNICODE)
			DEVMODEW Tmp;
			appMemzero(&Tmp, sizeof(Tmp));
			Tmp.dmSize = sizeof(Tmp);
			if (!EnumDisplaySettingsW(NULL, i, &Tmp)) {
				break;
			}
			Modes.AddUniqueItem(FPlane(Tmp.dmPelsWidth, Tmp.dmPelsHeight, Tmp.dmBitsPerPel, Tmp.dmDisplayFrequency));
#endif
		}
	}

	//Load D3D9 library
	if (!hModuleD3d9) {
		hModuleD3d9 = LoadLibraryA(g_d3d9DllName);
		if (!hModuleD3d9) {
			debugf(NAME_Init, TEXT("Failed to load %ls"), appFromAnsi(g_d3d9DllName));
			return 0;
		}
		pDirect3DCreate9 = (LPDIRECT3DCREATE9)GetProcAddress(hModuleD3d9, "Direct3DCreate9");
		if (!pDirect3DCreate9) {
			debugf(NAME_Init, TEXT("Failed to load function from %ls"), appFromAnsi(g_d3d9DllName));
			return 0;
		}
	}

	NumDevices++;

	// Init this rendering context.
	m_zeroPrefixBindTrees = m_localZeroPrefixBindTrees;
	m_nonZeroPrefixBindTrees = m_localNonZeroPrefixBindTrees;
	m_nonZeroPrefixBindChain = &m_localNonZeroPrefixBindChain;
	m_RGBA8TexPool = &m_localRGBA8TexPool;

	Viewport = InViewport;


	//Remember main window handle and get its DC
	m_hWnd = (HWND)InViewport->GetWindow();
	check(m_hWnd);
	m_hDC = GetDC(m_hWnd);
	check(m_hDC);

	// FIXME! This prevents the freeze on start problem, but only ugly workaround.
	if (Fullscreen)
	{
		if (!SetRes(Viewport->GetOuterUClient()->WindowedViewportX, Viewport->GetOuterUClient()->WindowedViewportY, NewColorBytes, 0)) {
			return FailedInitf(*LocalizeError(TEXT("ResFailed")));
		}
	}

	if (!SetRes(NewX, NewY, NewColorBytes, Fullscreen)) {
		return FailedInitf(*LocalizeError(TEXT("ResFailed")));
	}

	SupportsNoClipRender	= UseHardwareClipping;

	return 1;
	unguard;
}

UBOOL UD3D9RenderDevice::Exec(const TCHAR* Cmd, FOutputDevice& Ar) {
	guard(UD3D9RenderDevice::Exec);

	if (URenderDevice::Exec(Cmd, Ar)) {
		return 1;
	}
	if (ParseCommand(&Cmd, TEXT("DGL"))) {
		if (ParseCommand(&Cmd, TEXT("BUFFERTRIS"))) {
			BufferActorTris = !BufferActorTris;
			if (!UseVertexSpecular) BufferActorTris = 0;
			debugf(TEXT("BUFFERTRIS [%i]"), BufferActorTris);
			return 1;
		}
		else if (ParseCommand(&Cmd, TEXT("BUILD"))) {
			debugf(TEXT("D3D9 renderer built: ?????"));
			return 1;
		}
		else if (ParseCommand(&Cmd, TEXT("AA"))) {
			if (m_usingAA) {
				m_defAAEnable = !m_defAAEnable;
				debugf(TEXT("AA Enable [%u]"), (m_defAAEnable) ? 1 : 0);
			}
			return 1;
		}

		return 0;
	}
	else if (ParseCommand(&Cmd, TEXT("GetRes"))) {
		TArray<FPlane> Relevant;
		INT i;
		for (i = 0; i < Modes.Num(); i++) {
			if (Modes(i).Z == (Viewport->ColorBytes * 8))
				if
				(	(Modes(i).X!=320 || Modes(i).Y!=200)
				&&	(Modes(i).X!=640 || Modes(i).Y!=400) )
				Relevant.AddUniqueItem(FPlane(Modes(i).X, Modes(i).Y, 0, 0));
		}
		appQsort(&Relevant(0), Relevant.Num(), sizeof(FPlane), (QSORT_COMPARE)CompareRes);
		FString Str;
		for (i = 0; i < Relevant.Num(); i++) {
			Str += FString::Printf(TEXT("%ix%i "), (INT)Relevant(i).X, (INT)Relevant(i).Y);
		}
		Ar.Log(*Str.LeftChop(1));
		return 1;
	}

	return 0;

	unguard;
}

void UD3D9RenderDevice::Lock(FPlane InFlashScale, FPlane InFlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* InHitData, INT* InHitSize) {
	UTGLR_DEBUG_CALL_COUNT(Lock);
	guard(UD3D9RenderDevice::Lock);
	check(LockCount == 0);
	++LockCount;


	//Reset stats
	BindCycles = ImageCycles = ComplexCycles = GouraudCycles = TileCycles = BlendCycles = GouraudPolyListCycles = 0;

	m_vpEnableCount = 0;
	m_vpSwitchCount = 0;
	m_fpEnableCount = 0;
	m_fpSwitchCount = 0;
	m_AASwitchCount = 0;
	m_sceneNodeCount = 0;
	m_sceneNodeHackCount = 0;
	m_vbFlushCount = 0;
	m_stat0Count = 0;
	m_stat1Count = 0;


	HRESULT hResult;

	//Check for lost device
	hResult = m_d3dDevice->TestCooperativeLevel();
	if (FAILED(hResult)) {
#if 0
{
	dbgPrintf("utd3d9r: Device Lost\n");
}
#endif
		//Wait for device to become available again
		while (1) {
			//Check if device can be reset and restored
			if (hResult == D3DERR_DEVICENOTRESET) {
				//Set new resolution
				m_SetRes_isDeviceReset = true;
				if (!SetRes(m_SetRes_NewX, m_SetRes_NewY, m_SetRes_NewColorBytes, m_SetRes_Fullscreen)) {
					appErrorf(TEXT("Failed to reset lost D3D device"));
				}

				//Exit wait loop
				break;
			}
			//If not lost and not ready to be restored, error
			else if (hResult != D3DERR_DEVICELOST) {
				appErrorf(TEXT("Error checking for lost D3D device"));
			}
			//Otherwise, device is lost and cannot be restored yet

			//Wait
			Sleep(100);

			//Don't wait for device to become available here to prevent deadlock
			break;
		}
	}

	//D3D begin scene
	if (FAILED(m_d3dDevice->BeginScene())) {
		appErrorf(TEXT("BeginScene failed"));
	}

	//Clear the Z-buffer
	if (1 || GIsEditor || (RenderLockFlags & LOCKR_ClearScreen)) {
		SetBlend(PF_Occlude);
		m_d3dDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER | ((RenderLockFlags & LOCKR_ClearScreen) ? D3DCLEAR_TARGET : 0), (DWORD)FColor(ScreenClear).TrueColor(), 1.0f, 0);
	}
	m_LastZMode = 255;
	SetZTestMode(ZTEST_LessEqual);


	bool flushTextures = false;
	bool needShaderReload = false;


	//DCV refresh
	ConfigValidate_RefreshDCV();

	//Required extensions config validation pass
	ConfigValidate_RequiredExtensions();


	//Main config validation pass
	ConfigValidate_Main();


	//Detect changes in 1X blending setting and force tex env flush if necessary
	if (OneXBlending != PL_OneXBlending) {
		PL_OneXBlending = OneXBlending;
		InitOrInvalidateTexEnvState();
	}

	//Prevent changes to these parameters
	MaxLogUOverV = PL_MaxLogUOverV;
	MaxLogVOverU = PL_MaxLogVOverU;
	MinLogTextureSize = PL_MinLogTextureSize;
	MaxLogTextureSize = PL_MaxLogTextureSize;

	//Detect changes in various texture related options and force texture flush if necessary
	if (NoFiltering != PL_NoFiltering) {
		PL_NoFiltering = NoFiltering;
		flushTextures = true;
	}
	if (UseTrilinear != PL_UseTrilinear) {
		PL_UseTrilinear = UseTrilinear;
		flushTextures = true;
	}
	if (Use16BitTextures != PL_Use16BitTextures) {
		PL_Use16BitTextures = Use16BitTextures;
		flushTextures = true;
	}
	if (Use565Textures != PL_Use565Textures) {
		PL_Use565Textures = Use565Textures;
		flushTextures = true;
	}
	if (TexDXT1ToDXT3 != PL_TexDXT1ToDXT3) {
		PL_TexDXT1ToDXT3 = TexDXT1ToDXT3;
		flushTextures = true;
	}
	//MaxAnisotropy cannot be negative
	if (MaxAnisotropy < 0) {
		MaxAnisotropy = 0;
	}
	if (MaxAnisotropy > m_d3dCaps.MaxAnisotropy) {
		MaxAnisotropy = m_d3dCaps.MaxAnisotropy;
	}
	if (MaxAnisotropy != PL_MaxAnisotropy) {
		PL_MaxAnisotropy = MaxAnisotropy;
		flushTextures = true;

		SetTexMaxAnisotropyState(TMUnits);
	}

	if (SmoothMaskedTextures != PL_SmoothMaskedTextures) {
		PL_SmoothMaskedTextures = SmoothMaskedTextures;

		//Clear masked blending state if set before adjusting smooth masked textures bit
		SetBlend(PF_Occlude);

		//Disable alpha to coverage if currently enabled
		if (m_alphaToCoverageEnabled) {
			m_alphaToCoverageEnabled = false;
			DisableAlphaToCoverageNoCheck();
		}
	}

	if (MaskedTextureHack != PL_MaskedTextureHack) {
		PL_MaskedTextureHack = MaskedTextureHack;
		flushTextures = true;
	}

	if (LODBias != PL_LODBias) {
		PL_LODBias = LODBias;
		SetTexLODBiasState(TMUnits);
	}

	if (DetailTextures != PL_DetailTextures) {
		PL_DetailTextures = DetailTextures;
		flushTextures = true;
		if (DetailTextures) {
			needShaderReload = true;
		}
	}

	if (UseDetailAlpha != PL_UseDetailAlpha) {
		PL_UseDetailAlpha = UseDetailAlpha;
		if (UseDetailAlpha) {
			InitAlphaTextureSafe();
		}
	}

	if (SinglePassDetail != PL_SinglePassDetail) {
		PL_SinglePassDetail = SinglePassDetail;
		if (SinglePassDetail) {
			needShaderReload = true;
		}
	}


	if (UseFragmentProgram != PL_UseFragmentProgram) {
		PL_UseFragmentProgram = UseFragmentProgram;
		if (UseFragmentProgram) {
			//Attempt to initialize fragment program mode
			TryInitializeFragmentProgramMode();
			needShaderReload = false;
		}
		else {
			//Free fragment programs if they were allocated and leave fragment program mode if necessary
			ShutdownFragmentProgramMode();
		}
	}

	//Check if fragment program reload is necessary
	if (UseFragmentProgram) {
		if (needShaderReload) {
			//Attempt to initialize fragment program mode
			TryInitializeFragmentProgramMode();
		}
	}

	//Not using fixed function alpha test with fragment programs
	if (UseFragmentProgram) {
		//Disabled fixed function alpha test if currently enabled
		if (m_alphaTestEnabled) {
			m_alphaTestEnabled = false;
			m_d3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);

			//Select a blending state with no alpha test
			SetBlend(PF_Occlude);
		}
	}

	//Smooth masked textures bit controls alpha blend for masked textures
	m_smoothMaskedTexturesBit = 0;
	m_useAlphaToCoverageForMasked = false;
	if (SmoothMaskedTextures) {
		//Use alpha to coverage if using AA and enough samples, and if supported at all
		//Also requiring fragment program mode for alpha to coverage with D3D9
		if (UseFragmentProgram && m_usingAA && (m_initNumAASamples >= 4) && m_supportsAlphaToCoverage) {
			m_useAlphaToCoverageForMasked = true;
		}
		else {
			m_smoothMaskedTexturesBit = PF_Masked;
		}
	}


	if (UseSSE != PL_UseSSE) {
#ifdef UTGLR_INCLUDE_SSE_CODE
		if (UseSSE) {
			if (!CPU_DetectSSE()) {
				UseSSE = 0;
			}
		}
#else
		UseSSE = 0;
#endif
		PL_UseSSE = UseSSE;
	}
	if (UseSSE2 != PL_UseSSE2) {
#ifdef UTGLR_INCLUDE_SSE_CODE
		if (UseSSE2) {
			if (!CPU_DetectSSE2()) {
				UseSSE2 = 0;
			}
		}
#else
		UseSSE2 = 0;
#endif
		PL_UseSSE2 = UseSSE2;
	}


	//Shared fragment program parameters
	if (UseFragmentProgram) {
		//Lightmap blend scale factor
		m_fsBlendInfo[3] = (OneXBlending) ? 1.0f : 2.0f;

		m_d3dDevice->SetPixelShaderConstantF(0, m_fsBlendInfo, 1);
	}

#if 0 // Marco: Argh need to rewrite this assembly...
	//Initialize buffer verts proc pointers
	m_pBuffer3BasicVertsProc = Buffer3BasicVerts;
	m_pBuffer3ColoredVertsProc = Buffer3ColoredVerts;
	m_pBuffer3FoggedVertsProc = Buffer3FoggedVerts;

#ifdef UTGLR_INCLUDE_SSE_CODE
	//Initialize SSE buffer verts proc pointers
	if (UseSSE) {
		m_pBuffer3ColoredVertsProc = Buffer3ColoredVerts_SSE;
		m_pBuffer3FoggedVertsProc = Buffer3FoggedVerts_SSE;
	}
	if (UseSSE2) {
		m_pBuffer3ColoredVertsProc = Buffer3ColoredVerts_SSE2;
		m_pBuffer3FoggedVertsProc = Buffer3FoggedVerts_SSE2;
	}
#endif //UTGLR_INCLUDE_SSE_CODE
#endif

	m_pBuffer3VertsProc = NULL;


	//Initialize render passes no check proc pointers
	if (UseFragmentProgram) {
		m_pRenderPassesNoCheckSetupProc = &UD3D9RenderDevice::RenderPassesNoCheckSetup_FP;
		m_pRenderPassesNoCheckSetup_SingleOrDualTextureAndDetailTextureProc = &UD3D9RenderDevice::RenderPassesNoCheckSetup_SingleOrDualTextureAndDetailTexture_FP;
	}
	else {
		m_pRenderPassesNoCheckSetupProc = &UD3D9RenderDevice::RenderPassesNoCheckSetup;
		m_pRenderPassesNoCheckSetup_SingleOrDualTextureAndDetailTextureProc = &UD3D9RenderDevice::RenderPassesNoCheckSetup_SingleOrDualTextureAndDetailTexture;
	}

	//Initialize buffer detail texture data proc pointer
	m_pBufferDetailTextureDataProc = &UD3D9RenderDevice::BufferDetailTextureData;
#ifdef UTGLR_INCLUDE_SSE_CODE
	if (UseSSE2) {
		m_pBufferDetailTextureDataProc = &UD3D9RenderDevice::BufferDetailTextureData_SSE2;
	}
#endif //UTGLR_INCLUDE_SSE_CODE


	//Precalculate the cutoff for buffering actor triangles based on config settings
	if (!BufferActorTris) {
		m_bufferActorTrisCutoff = 0;
	}
	else if (!BufferClippedActorTris) {
		m_bufferActorTrisCutoff = 3;
	}
	else {
		m_bufferActorTrisCutoff = 10;
	}

	//Precalculate detail texture color
	if (ColorizeDetailTextures) {
		m_detailTextureColor4ub = 0x00408040;
	}
	else {
		m_detailTextureColor4ub = 0x00808080;
	}

	//Precalculate mask for MaskedTextureHack based on if it's enabled
	m_maskedTextureHackMask = (MaskedTextureHack) ? TEX_CACHE_ID_FLAG_MASKED : 0;

	// Remember stuff.
	FlashScale = InFlashScale;
	FlashFog   = InFlashFog;

	//Selection setup
	m_HitData = InHitData;
	m_HitSize = InHitSize;
	m_HitCount = 0;
	if (m_HitData) {
		m_HitBufSize = *m_HitSize;
		*m_HitSize = 0;

		//Start selection
		m_gclip.SelectModeStart();
	}

	//Flush textures if necessary due to config change
	if (flushTextures) {
		Flush(1);
	}

	unguard;
}

void UD3D9RenderDevice::SetSceneNode(FSceneNode* Frame) {
	UTGLR_DEBUG_CALL_COUNT(SetSceneNode);
	guard(UD3D9RenderDevice::SetSceneNode);

	EndBuffering();		// Flush vertex array before changing the projection matrix!

	TopSceneNode = Frame;
	m_sceneNodeCount++;

	//No need to set default AA state here
	//No need to set default projection state as this function always sets/initializes it
	SetDefaultStreamState();
	SetDefaultTextureState();

	// Precompute stuff.
	FLOAT rcpFrameFX = 1.0f / Frame->FX;
	m_Aspect = Frame->FY * rcpFrameFX;
	m_RProjZ = appTan(Viewport->Actor->FovAngle * PI / 360.0);
	m_RFX2 = 2.0f * m_RProjZ * rcpFrameFX;
	m_RFY2 = 2.0f * m_RProjZ * rcpFrameFX;

	//Remember Frame->X and Frame->Y
	m_sceneNodeX = Frame->X;
	m_sceneNodeY = Frame->Y;

	// Set viewport.
	D3DVIEWPORT9 d3dViewport;
	d3dViewport.X = Frame->XB;
	d3dViewport.Y = Frame->YB;
	d3dViewport.Width = Frame->X;
	d3dViewport.Height = Frame->Y;
	d3dViewport.MinZ = 0.0f;
	d3dViewport.MaxZ = 1.0f;
	m_d3dDevice->SetViewport(&d3dViewport);

	//Decide whether or not to use Z range hack
	m_useZRangeHack = false;
	if (ZRangeHack /*&& !GIsEditor*/) {
		m_useZRangeHack = true;
	}

	// Set projection.
	if (Frame->Viewport->IsOrtho()) {
		//Don't use Z range hack if ortho projection
		m_useZRangeHack = false;

		SetOrthoProjection();
	}
	else {
		SetProjectionStateNoCheck(false);
	}

	//Set clip planes if doing selection
	if (m_HitData) {
		if (Frame->Viewport->IsOrtho()) {
			float cp[4];
			FLOAT nX = Viewport->HitX - Frame->FX2;
			FLOAT pX = nX + Viewport->HitXL;
			FLOAT nY = Viewport->HitY - Frame->FY2;
			FLOAT pY = nY + Viewport->HitYL;

			nX *= m_RFX2;
			pX *= m_RFX2;
			nY *= m_RFY2;
			pY *= m_RFY2;

			cp[0] = +1.0; cp[1] = 0.0; cp[2] = 0.0; cp[3] = -nX;
			m_gclip.SetCp(0, cp);
			m_gclip.SetCpEnable(0, true);

			cp[0] = 0.0; cp[1] = +1.0; cp[2] = 0.0; cp[3] = -nY;
			m_gclip.SetCp(1, cp);
			m_gclip.SetCpEnable(1, true);

			cp[0] = -1.0; cp[1] = 0.0; cp[2] = 0.0; cp[3] = +pX;
			m_gclip.SetCp(2, cp);
			m_gclip.SetCpEnable(2, true);

			cp[0] = 0.0; cp[1] = -1.0; cp[2] = 0.0; cp[3] = +pY;
			m_gclip.SetCp(3, cp);
			m_gclip.SetCpEnable(3, true);

			//Near clip plane
			cp[0] = 0.0f; cp[1] = 0.0f; cp[2] = 1.0f; cp[3] = -0.5f;
			m_gclip.SetCp(4, cp);
			m_gclip.SetCpEnable(4, true);
		}
		else {
			FVector N[4];
			float cp[4];
			INT i;

			FLOAT nX = Viewport->HitX - Frame->FX2;
			FLOAT pX = nX + Viewport->HitXL;
			FLOAT nY = Viewport->HitY - Frame->FY2;
			FLOAT pY = nY + Viewport->HitYL;

			N[0] = (FVector(nX * Frame->RProj.Z, 0, 1) ^ FVector(0, -1, 0)).SafeNormal();
			N[1] = (FVector(pX * Frame->RProj.Z, 0, 1) ^ FVector(0, +1, 0)).SafeNormal();
			N[2] = (FVector(0, nY * Frame->RProj.Z, 1) ^ FVector(+1, 0, 0)).SafeNormal();
			N[3] = (FVector(0, pY * Frame->RProj.Z, 1) ^ FVector(-1, 0, 0)).SafeNormal();

			for (i = 0; i < 4; i++) {
				cp[0] = N[i].X;
				cp[1] = N[i].Y;
				cp[2] = N[i].Z;
				cp[3] = 0.0f;
				m_gclip.SetCp(i, cp);
				m_gclip.SetCpEnable(i, true);
			}

			//Near clip plane
			cp[0] = 0.0f; cp[1] = 0.0f; cp[2] = 1.0f; cp[3] = -0.5f;
			m_gclip.SetCp(4, cp);
			m_gclip.SetCpEnable(4, true);
		}
	}

	// Reset clipping!
	if (m_NumClipPlanes)
	{
		m_d3dDevice->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
		m_NumClipPlanes = 0;
	}

	unguard;
}

void UD3D9RenderDevice::Unlock(UBOOL Blit) {
	UTGLR_DEBUG_CALL_COUNT(Unlock);
	guard(UD3D9RenderDevice::Unlock);

	EndBuffering();

	SetDefaultAAState();
	SetDefaultProjectionState();
	SetDefaultStreamState();
	SetDefaultTextureState();

	// Unlock and render.
	check(LockCount == 1);

	//D3D end scene
	if (FAILED(m_d3dDevice->EndScene())) {
		debugf(TEXT("EndScene failed"));
	}

	if (Blit) {
		HRESULT hResult;
		bool swapBuffersStatus;

		//Present
		hResult = m_d3dDevice->Present(NULL, NULL, NULL, NULL);
		swapBuffersStatus = (FAILED(hResult)) ? false : true;
		//Don't signal error if device is lost
		if (hResult == D3DERR_DEVICELOST) swapBuffersStatus = true;

		check(swapBuffersStatus);
		if (!m_prevSwapBuffersStatus) {
			check(swapBuffersStatus);
		}
		m_prevSwapBuffersStatus = swapBuffersStatus;
	}

	--LockCount;

	//If doing selection, end and return hits
	if (m_HitData) {
		INT i;

		//End selection
		m_gclip.SelectModeEnd();

		*m_HitSize = m_HitCount;

		//Disable clip planes
		for (i = 0; i < 5; i++) {
			m_gclip.SetCpEnable(i, false);
		}
	}

	//Scan for old textures
	if (UseTexIdPool) {
		//Scan for old textures
		ScanForOldTextures();
	}

	//Increment current frame count
	m_currentFrameCount++;

	//Check for optional frame rate limit
#if 0 // Now done in Engine
	if (FrameRateLimit >= 20) {
#if defined UTGLR_DX_BUILD || defined UTGLR_RUNE_BUILD
		FLOAT curFrameTimestamp;
#else
		FTime curFrameTimestamp;
#endif
		float timeDiff;
		float rcpFrameRateLimit;

		//First time timer init if necessary
		InitFrameRateLimitTimerSafe();

		curFrameTimestamp = appSeconds();
		timeDiff = curFrameTimestamp - m_prevFrameTimestamp;
		m_prevFrameTimestamp = curFrameTimestamp;

		rcpFrameRateLimit = 1.0f / FrameRateLimit;
		if (timeDiff < rcpFrameRateLimit) {
			float sleepTime;

			sleepTime = rcpFrameRateLimit - timeDiff;
			appSleep(sleepTime);

			m_prevFrameTimestamp = appSeconds();
		}
	}
#endif

#if 0
	dbgPrintf("VP enable count = %u\n", m_vpEnableCount);
	dbgPrintf("VP switch count = %u\n", m_vpSwitchCount);
	dbgPrintf("FP enable count = %u\n", m_fpEnableCount);
	dbgPrintf("FP switch count = %u\n", m_fpSwitchCount);
	dbgPrintf("AA switch count = %u\n", m_AASwitchCount);
	dbgPrintf("Scene node count = %u\n", m_sceneNodeCount);
	dbgPrintf("Scene node hack count = %u\n", m_sceneNodeHackCount);
	dbgPrintf("VB flush count = %u\n", m_vbFlushCount);
	dbgPrintf("Stat 0 count = %u\n", m_stat0Count);
	dbgPrintf("Stat 1 count = %u\n", m_stat1Count);
#endif


	unguard;
}

void UD3D9RenderDevice::Flush(UBOOL AllowPrecache) {
	guard(UD3D9RenderDevice::Flush);
	unsigned int u;

	if (!m_d3dDevice) {
		return;
	}

	EndBuffering();

	//Disable fog.
	m_gpFogEnabled = false;
	m_d3dDevice->SetRenderState(D3DRS_FOGENABLE, FALSE);

	for (u = 0; u < TMUnits; u++) {
		m_d3dDevice->SetTexture(u, NULL);
	}

	for (u = 0; u < NUM_CTTree_TREES; u++) {
		DWORD_CTTree_t *zeroPrefixBindTree = &m_zeroPrefixBindTrees[u];
		for (DWORD_CTTree_t::node_t *zpbmPtr = zeroPrefixBindTree->begin(); zpbmPtr != zeroPrefixBindTree->end(); zpbmPtr = zeroPrefixBindTree->next_node(zpbmPtr)) {
			zpbmPtr->data.pTexObj->Release();
		}
		zeroPrefixBindTree->clear(&m_DWORD_CTTree_Allocator);
	}

	for (u = 0; u < NUM_CTTree_TREES; u++) {
		QWORD_CTTree_t *nonZeroPrefixBindTree = &m_nonZeroPrefixBindTrees[u];
		for (QWORD_CTTree_t::node_t *nzpbmPtr = nonZeroPrefixBindTree->begin(); nzpbmPtr != nonZeroPrefixBindTree->end(); nzpbmPtr = nonZeroPrefixBindTree->next_node(nzpbmPtr)) {
			nzpbmPtr->data.pTexObj->Release();
		}
		nonZeroPrefixBindTree->clear(&m_QWORD_CTTree_Allocator);
	}

	m_nonZeroPrefixBindChain->mark_as_clear();

	for (TexPoolMap_t::node_t *RGBA8TpPtr = m_RGBA8TexPool->begin(); RGBA8TpPtr != m_RGBA8TexPool->end(); RGBA8TpPtr = m_RGBA8TexPool->next_node(RGBA8TpPtr)) {
		while (QWORD_CTTree_NodePool_t::node_t *texPoolNodePtr = RGBA8TpPtr->data.try_remove()) {
			texPoolNodePtr->data.pTexObj->Release();
			m_QWORD_CTTree_Allocator.free_node(texPoolNodePtr);
		}
	}
	m_RGBA8TexPool->clear(&m_TexPoolMap_Allocator);

	while (QWORD_CTTree_NodePool_t::node_t *nzpnpPtr = m_nonZeroPrefixNodePool.try_remove()) {
		m_QWORD_CTTree_Allocator.free_node(nzpnpPtr);
	}

	AllocatedTextures = 0;

	//Reset current texture ids to hopefully unused values
	for (u = 0; u < MAX_TMUNITS; u++) {
		TexInfo[u].CurrentCacheID = TEX_CACHE_ID_UNUSED;
		TexInfo[u].pBind = NULL;
	}

	if (AllowPrecache && UsePrecache && !GIsEditor) {
		PrecacheOnFlip = 1;
	}

	SetGamma(Viewport->GetOuterUClient()->Brightness);

	unguard;
}

void UD3D9RenderDevice::DrawComplexSurface(FSceneNode* Frame, FSurfaceInfo& Surface, FSurfaceFacet& Facet) {
	UTGLR_DEBUG_CALL_COUNT(DrawComplexSurface);
	guard(UD3D9RenderDevice::DrawComplexSurface);

	EndBuffering();

	if (SceneNodeHack) {
		if ((Frame->X != m_sceneNodeX) || (Frame->Y != m_sceneNodeY)) {
			m_sceneNodeHackCount++;
			SetSceneNode(Frame);
		}
	}

	SetDefaultAAState();
	SetDefaultProjectionState();
	//This function uses cached stream state information
	//This function uses cached texture state information

	check(Surface.Texture);

	//Hit select path
	if (m_HitData) {
		for (FSavedPoly* Poly = Facet.Polys; Poly; Poly = Poly->Next) {
			INT NumPts = Poly->NumPts;
			CGClip::vec3_t triPts[3];
			INT i;
			const FTransform* Pt;

			Pt = Poly->Pts[0];
			triPts[0].x = Pt->Point.X;
			triPts[0].y = Pt->Point.Y;
			triPts[0].z = Pt->Point.Z;

			for (i = 2; i < NumPts; i++) {
				Pt = Poly->Pts[i - 1];
				triPts[1].x = Pt->Point.X;
				triPts[1].y = Pt->Point.Y;
				triPts[1].z = Pt->Point.Z;

				Pt = Poly->Pts[i];
				triPts[2].x = Pt->Point.X;
				triPts[2].y = Pt->Point.Y;
				triPts[2].z = Pt->Point.Z;

				m_gclip.SelectDrawTri(triPts);
			}
		}

		return;
	}

	clockFast(ComplexCycles);

	//Calculate UDot and VDot intermediates for complex surface
	m_csUDot = Facet.MapCoords.XAxis | Facet.MapCoords.Origin;
	m_csVDot = Facet.MapCoords.YAxis | Facet.MapCoords.Origin;

	//Buffer static geometry
	//Sets m_csPolyCount
	//m_csPtCount set later from return value
	//Sets MultiDrawFirstArray and MultiDrawCountArray
	INT numVerts;
	if (UseFragmentProgram) {
		numVerts = BufferStaticComplexSurfaceGeometry_VP(Facet);
	}
	else {
		numVerts = BufferStaticComplexSurfaceGeometry(Facet);
	}

	//Reject invalid surfaces early
	if (numVerts == 0) {
		return;
	}

	//Save number of points
	m_csPtCount = numVerts;

	//See if detail texture should be drawn
	//FogMap and DetailTexture are mutually exclusive effects
	bool drawDetailTexture = false;
	if ((DetailTextures != 0) && Surface.DetailTexture && !Surface.FogMap && !m_WithDistanceFog) {
		drawDetailTexture = true;
	}

	//Check for detail texture
	if (drawDetailTexture == true) {
		DWORD anyIsNearBits;

		//Buffer detail texture data
		anyIsNearBits = (this->*m_pBufferDetailTextureDataProc)(380.0f);

		//Do not draw detail texture if no vertices are near
		if (anyIsNearBits == 0) {
			drawDetailTexture = false;
		}
	}


	DWORD PolyFlags = Surface.PolyFlags;

	//Initialize render passes state information
	m_rpPassCount = 0;
	m_rpTMUnits = TMUnits;
	m_rpForceSingle = false;
	m_rpMasked = ((PolyFlags & PF_Masked) == 0) ? false : true;
	m_rpSetDepthEqual = false;
	m_rpColor = 0xFFFFFFFF;


	//Do static render passes state setup
	if (UseFragmentProgram) {
		const FVector &XAxis = Facet.MapCoords.XAxis;
		const FVector &YAxis = Facet.MapCoords.YAxis;
		FLOAT vsParams[8] = { XAxis.X, XAxis.Y, XAxis.Z, m_csUDot,
							  YAxis.X, YAxis.Y, YAxis.Z, m_csVDot };

		m_d3dDevice->SetVertexShaderConstantF(4, vsParams, 2);
	}

	AddRenderPass(Surface.Texture, PolyFlags & ~PF_FlatShaded, 0.0f);

	if (Surface.MacroTexture) {
		AddRenderPass(Surface.MacroTexture, PF_Modulated, -0.5f);
	}

#if 0 // TODO
	if (g_bSupportsBumpMap && Surface.BumpMap) {
			AddRenderPass(Surface.BumpMap, & ~PF_FlatShaded, 0.0f);
	}
#endif

	if (Surface.LightMap) {
		AddRenderPass(Surface.LightMap, PF_Modulated, -0.5f);
	}

	if (Surface.FogMap) {
		//Check for single pass fog mode
		if (!SinglePassFog) {
			RenderPasses();
		}

		AddRenderPass(Surface.FogMap, PF_Highlighted, -0.5f);
	}

	// Draw detail texture overlaid, in a separate pass.
	if (drawDetailTexture == true) {
		bool singlePassDetail = false;

		//Check if single pass detail mode is enabled and if can use it
		if (SinglePassDetail) {
			//Only attempt single pass detail if single texture rendering wasn't forced earlier
			if (!m_rpForceSingle) {
				//Single pass detail must be done with one or two normal passes
				if ((m_rpPassCount == 1) || (m_rpPassCount == 2)) {
					singlePassDetail = true;
				}
			}
		}

		if (singlePassDetail) {
			RenderPasses_SingleOrDualTextureAndDetailTexture(*Surface.DetailTexture);
		}
		else {
			RenderPasses();

			bool clipDetailTexture = (DetailClipping != 0);

			if (m_rpMasked) {
				//Cannot use detail texture clipping with masked mode
				//It will not work with m_d3dDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_EQUAL);
				clipDetailTexture = false;

				if (m_rpSetDepthEqual == false) {
					SetZTestMode(ZTEST_Equal);
					m_rpSetDepthEqual = true;
				}
			}

			//This function should only be called if at least one polygon will be detail textured
			if (UseFragmentProgram) {
				DrawDetailTexture_FP(*Surface.DetailTexture);
			}
			else {
				DrawDetailTexture(*Surface.DetailTexture, clipDetailTexture);
			}
		}
	}
	else {
		RenderPasses();
	}

	// UnrealEd selection.
	if (PolyFlags & (PF_Selected | PF_FlatShaded))
	{
		DWORD polyColor;

		//No need to set default AA state here as it is always set on entry to DrawComplexSurface
		//No need to set default projection state here as it is always set on entry to DrawComplexSurface
		SetDefaultStreamState();
		SetDefaultTextureState();

		SetNoTexture(0);
		SetBlend(PF_Highlighted);

		if (PolyFlags & PF_FlatShaded) {
			FPlane Color;

			Color.X = Surface.FlatColor.R / 255.0f;
			Color.Y = Surface.FlatColor.G / 255.0f;
			Color.Z = Surface.FlatColor.B / 255.0f;
			Color.W = 0.85f;
			if (PolyFlags & PF_Selected) {
				Color.X *= 1.5f;
				Color.Y *= 1.5f;
				Color.Z *= 1.5f;
				Color.W = 1.0f;
			}

			polyColor = FPlaneTo_BGRAClamped(&Color);
		}
		else {
			polyColor = 0x7F00007F;
		}

		for (FSavedPoly* Poly = Facet.Polys; Poly; Poly = Poly->Next) {
			INT NumPts = Poly->NumPts;

			//Make sure at least NumPts entries are left in the vertex buffers
			if ((m_curVertexBufferPos + NumPts) >= VERTEX_ARRAY_SIZE) {
				FlushVertexBuffers();
			}

			//Lock vertexColor and texCoord0 buffers
			LockVertexColorBuffer();
			LockTexCoordBuffer(0);

			FGLTexCoord *pTexCoordArray = m_pTexCoordArray[0];
			FGLVertexColor *pVertexColorArray = m_pVertexColorArray;

			for (INT i = 0; i < Poly->NumPts; i++) {
				pTexCoordArray[i].u = 0.5f;
				pTexCoordArray[i].v = 0.5f;

				pVertexColorArray[i].x = Poly->Pts[i]->Point.X;
				pVertexColorArray[i].y = Poly->Pts[i]->Point.Y;
				pVertexColorArray[i].z = Poly->Pts[i]->Point.Z;
				pVertexColorArray[i].color = polyColor;
			}

			//Unlock vertexColor and texCoord0 buffers
			UnlockVertexColorBuffer();
			UnlockTexCoordBuffer(0);

			//Draw the triangle fan
			m_d3dDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, m_curVertexBufferPos, NumPts - 2);

			//Advance vertex buffer position
			m_curVertexBufferPos += NumPts;
		}
	}

	if (m_rpSetDepthEqual == true) {
		SetZTestMode(ZTEST_LessEqual);
	}

	unclockFast(ComplexCycles);
	unguard;
}

#ifdef UTGLR_RUNE_BUILD
void UD3D9RenderDevice::PreDrawFogSurface() {
	UTGLR_DEBUG_CALL_COUNT(PreDrawFogSurface);
	guard(UD3D9RenderDevice::PreDrawFogSurface);

	EndBuffering();

	SetDefaultAAState();
	SetDefaultProjectionState();
	SetDefaultStreamState();
	SetDefaultTextureState();

	SetBlend(PF_AlphaBlend);
	SetNoTexture(0);

	unguard;
}

void UD3D9RenderDevice::PostDrawFogSurface() {
	UTGLR_DEBUG_CALL_COUNT(PostDrawFogSurface);
	guard(UD3D9RenderDevice::PostDrawFogSurface);

	SetBlend(0);

	unguard;
}

void UD3D9RenderDevice::DrawFogSurface(FSceneNode* Frame, FFogSurf &FogSurf) {
	UTGLR_DEBUG_CALL_COUNT(DrawFogSurface);
	guard(UD3D9RenderDevice::DrawFogSurface);

	FPlane Modulate(Clamp(FogSurf.FogColor.X, 0.0f, 1.0f), Clamp(FogSurf.FogColor.Y, 0.0f, 1.0f), Clamp(FogSurf.FogColor.Z, 0.0f, 1.0f), 0.0f);

	FLOAT RFogDistance = 1.0f / FogSurf.FogDistance;

	if (FogSurf.PolyFlags & PF_Masked) {
		SetZTestMode(ZTEST_Equal);
	}

	//Set stream state
	SetDefaultStreamState();

	for (FSavedPoly* Poly = FogSurf.Polys; Poly; Poly = Poly->Next) {
		INT NumPts = Poly->NumPts;

		//Make sure at least NumPts entries are left in the vertex buffers
		if ((m_curVertexBufferPos + NumPts) >= VERTEX_ARRAY_SIZE) {
			FlushVertexBuffers();
		}

		//Lock vertexColor and texCoord0 buffers
		LockVertexColorBuffer();
		LockTexCoordBuffer(0);

		INT Index = 0;
		for (INT i = 0; i < NumPts; i++) {
			FTransform* P = Poly->Pts[i];

			Modulate.W = P->Point.Z * RFogDistance;
			if (Modulate.W > 1.0f) {
				Modulate.W = 1.0f;
			}
			else if (Modulate.W < 0.0f) {
				Modulate.W = 0.0f;
			}

			FGLVertexColor &destVertexColor = m_pVertexColorArray[Index];
			destVertexColor.x = P->Point.X;
			destVertexColor.y = P->Point.Y;
			destVertexColor.z = P->Point.Z;
			destVertexColor.color = FPlaneTo_BGRA(&Modulate);

			FGLTexCoord &destTexCoord = m_pTexCoordArray[0][Index];
			destTexCoord.u = 0.0f;
			destTexCoord.v = 0.0f;

			Index++;
		}

		//Unlock vertexColor and texCoord0 buffers
		UnlockVertexColorBuffer();
		UnlockTexCoordBuffer(0);

		//Draw the triangles
		m_d3dDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, m_curVertexBufferPos, NumPts - 2);

		//Advance vertex buffer position
		m_curVertexBufferPos += NumPts;
	}

	if (FogSurf.PolyFlags & PF_Masked) {
		SetZTestMode(ZTEST_LessEqual);
	}

	unguard;
}

void UD3D9RenderDevice::PreDrawGouraud(FSceneNode* Frame, FLOAT FogDistance, FPlane FogColor) {
	UTGLR_DEBUG_CALL_COUNT(PreDrawGouraud);
	guard(UD3D9RenderDevice::PreDrawGouraud);

	if (FogDistance > 0.0f) {
		EndBuffering();

		//Enable fog
		m_gpFogEnabled = true;
		if (UseFragmentProgram) {
			FLOAT psParams[8];
			FLOAT rcpFogLen;

			//Fog color
			psParams[0] = FogColor.X;
			psParams[1] = FogColor.Y;
			psParams[2] = FogColor.Z;
			psParams[3] = FogColor.W;

			//Fog distance info
			rcpFogLen = 1.0f / (FogDistance - 0.0f);
			psParams[4] = rcpFogLen;
			psParams[5] = FogDistance * rcpFogLen;
			psParams[6] = 0.0f;
			psParams[7] = 0.0f;

			m_d3dDevice->SetPixelShaderConstantF(3, psParams, 2);
		}
		else {
			//Enable fog
			m_d3dDevice->SetRenderState(D3DRS_FOGENABLE, TRUE);

			//Default fog mode is LINEAR
			//Default fog start is 0.0f
			m_d3dDevice->SetRenderState(D3DRS_FOGCOLOR, FPlaneTo_BGRAClamped(&FogColor));
			FLOAT fFogDistance = FogDistance;
			m_d3dDevice->SetRenderState(D3DRS_FOGEND, *(DWORD *)&fFogDistance);
		}
	}

	unguard;
}

void UD3D9RenderDevice::PostDrawGouraud(FLOAT FogDistance) {
	UTGLR_DEBUG_CALL_COUNT(PostDrawGouraud);
	guard(UD3D9RenderDevice::PostDrawGouraud);

	if (FogDistance > 0.0f) {
		EndBuffering();

		//Disable fog
		m_gpFogEnabled = false;
		if (UseFragmentProgram) {
		}
		else {
			//Disable fog
			m_d3dDevice->SetRenderState(D3DRS_FOGENABLE, FALSE);
		}
	}

	unguard;
}
#endif

#ifdef UTGLR_UNREAL_227_BUILD
void UD3D9RenderDevice::PreDrawFogSurface() {
	UTGLR_DEBUG_CALL_COUNT(PreDrawFogSurface);
	guard(UD3D9RenderDevice::PreDrawFogSurface);

	EndBuffering();

	SetDefaultAAState();
	SetDefaultProjectionState();
	SetDefaultStreamState();
	SetDefaultTextureState();

	SetBlend(PF_AlphaBlend);

	SetNoTexture(0);

	unguard;
}

void UD3D9RenderDevice::PostDrawFogSurface() {
	UTGLR_DEBUG_CALL_COUNT(PostDrawFogSurface);
	guard(UD3D9RenderDevice::PostDrawFogSurface);

	SetBlend(0);

	unguard;
}

void UD3D9RenderDevice::DrawFogSurface(FSceneNode* Frame, FFogSurf &FogSurf) {
	UTGLR_DEBUG_CALL_COUNT(DrawFogSurface);
	guard(UD3D9RenderDevice::DrawFogSurface);

	FPlane FogColor	= FogSurf.FogColor;
	FPlane Modulate(Clamp(FogColor.X, 0.0f, 1.0f), Clamp(FogColor.Y, 0.0f, 1.0f), Clamp(FogColor.Z, 0.0f, 1.0f), 0.0f);

	FLOAT RFogDistanceEnd = FogSurf.FogDistanceEnd;
	FLOAT RFogDistanceStart = FogSurf.FogDistanceStart;
	FLOAT RFogDensity = FogSurf.FogDensity;

	if (FogSurf.PolyFlags & PF_Masked) {
		SetZTestMode(ZTEST_Equal);
	}

	//Set stream state
	SetDefaultStreamState();

	for (FSavedPoly* Poly = FogSurf.Polys; Poly; Poly = Poly->Next) {
		INT NumPts = Poly->NumPts;

		//Make sure at least NumPts entries are left in the vertex buffers
		if ((m_curVertexBufferPos + NumPts) >= VERTEX_ARRAY_SIZE) {
			FlushVertexBuffers();
		}

		//Lock vertexColor and texCoord0 buffers
		LockVertexColorBuffer();
		LockTexCoordBuffer(0);

		INT Index = 0;
		for (INT i = 0; i < NumPts; i++) {
			FTransform* P = Poly->Pts[i];

			if (FogSurf.FogMode == 2)//EXP2
				Modulate.W = 1.f - Clamp<FLOAT>(exp(-pow(-RFogDensity * Poly->Pts[i]->Point.Z, 2.f)),0.f,1.f);
			else if (FogSurf.FogMode == 1)//EXP
				Modulate.W = 1.f - Clamp<FLOAT>(exp(-RFogDensity * Poly->Pts[i]->Point.Z),0.f,1.f);
			else //Linear
				Modulate.W = 1.f - Clamp<FLOAT>((RFogDistanceEnd-Poly->Pts[i]->Point.Z)/(RFogDistanceEnd-RFogDistanceStart),0.f,1.f);

			FGLVertexColor &destVertexColor = m_pVertexColorArray[Index];
			destVertexColor.x = P->Point.X;
			destVertexColor.y = P->Point.Y;
			destVertexColor.z = P->Point.Z;
			destVertexColor.color = FPlaneTo_BGRA(&Modulate);

			FGLTexCoord &destTexCoord = m_pTexCoordArray[0][Index];
			destTexCoord.u = 0.0f;
			destTexCoord.v = 0.0f;

			Index++;
		}

		//Unlock vertexColor and texCoord0 buffers
		UnlockVertexColorBuffer();
		UnlockTexCoordBuffer(0);

		//Draw the triangles
		m_d3dDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, m_curVertexBufferPos, NumPts - 2);

		//Advance vertex buffer position
		m_curVertexBufferPos += NumPts;
	}

	if (FogSurf.PolyFlags & PF_Masked) {
		SetZTestMode(ZTEST_LessEqual);
	}

	unguard;
}

void UD3D9RenderDevice::PreDrawGouraud(FSceneNode* Frame, FFogSurf &FogSurf) {
	UTGLR_DEBUG_CALL_COUNT(PreDrawGouraud);
	guard(UD3D9RenderDevice::PreDrawGouraud);

	EndBuffering();

	m_WithDistanceFog = true;
	FLOAT FogStart = FogSurf.FogDistanceStart;
	FLOAT FogEnd	= FogSurf.FogDistanceEnd;
	FLOAT FogDensity= FogSurf.FogDensity;

	FPlane FogColor	= FogSurf.FogColor;
	if( FogSurf.PolyFlags & PF_Modulated)
	{
		FogColor=FPlane(0.5f,0.5f,0.5f,0.f);
	}
	else if( FogSurf.PolyFlags & PF_Translucent)
	{
		FogColor=FPlane(0.f,0.f,0.f,0.f);
	}

	//Enable fog
	//m_gpFogEnabled = true;
	if (UseFragmentProgram) {
		FLOAT FogRange=(FogEnd - FogStart);
		FLOAT rcpFogLen = 1.0f/FogRange;
		FLOAT psParams[8];

		//Fog color
		psParams[0] = FogColor.X;
		psParams[1] = FogColor.Y;
		psParams[2] = FogColor.Z;
		psParams[3] = FogColor.W;

		//Fog distance info
		// c4.x - Linear fog [1.0 / (end - start)]
		// c4.y - Linear fog [end / (end - start)]
		psParams[4] = rcpFogLen;
		psParams[5] = FogEnd * rcpFogLen;
		psParams[6] = 0.0f;
		psParams[7] = 0.0f;

		m_d3dDevice->SetPixelShaderConstantF(3, psParams, 2);
		m_gpFogEnabled = true;
	}
	else {
		//Enable fog
		m_d3dDevice->SetRenderState(D3DRS_FOGENABLE, TRUE);
		m_d3dDevice->SetRenderState(D3DRS_FOGCOLOR, FPlaneTo_BGRAClamped(&FogColor));

		if (FogSurf.FogMode >= 2){
			m_d3dDevice->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_EXP2);
			m_d3dDevice->SetRenderState(D3DRS_FOGDENSITY, *(DWORD *)&FogDensity);
		}
		else if (FogSurf.FogMode == 1){
			m_d3dDevice->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_EXP);
			m_d3dDevice->SetRenderState(D3DRS_FOGDENSITY, *(DWORD *)&FogDensity);
		}
		else{
			m_d3dDevice->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_LINEAR);
			m_d3dDevice->SetRenderState(D3DRS_FOGSTART, *((DWORD*)(&FogStart)));
			m_d3dDevice->SetRenderState(D3DRS_FOGEND, *(DWORD *)&FogEnd);
		}
	}
	unguard;
}

void UD3D9RenderDevice::PostDrawGouraud(FSceneNode* Frame, FFogSurf &FogSurf) {
	UTGLR_DEBUG_CALL_COUNT(PostDrawGouraud);
	guard(UD3D9RenderDevice::PostDrawGouraud);
	EndBuffering();
	//Disable fog
	if (UseFragmentProgram)
	{
		if (FogSurf.bBSP) //Hack! FixMe by implementing corresponding fragment shader functions.
		{
			PreDrawFogSurface();
			DrawFogSurface(Frame, FogSurf); //Unlike D3DRS_FOGENABLE this has to be done AFTER DrawComplexSurface as an additional pass. SLOW!
			SetBlend(0);
		}
		m_gpFogEnabled = false;
	}
	m_d3dDevice->SetRenderState(D3DRS_FOGENABLE, FALSE);
	m_WithDistanceFog = false;
	unguard;
}
void UD3D9RenderDevice::DrawGouraudPolyList( FSceneNode* Frame, FTextureInfo& Info, FTransTexture* Pts, int NumPts, DWORD PolyFlags, FSpanBuffer* Span)
{
	UTGLR_DEBUG_CALL_COUNT(DrawGouraudPolyList);
	guard(UD3D9RenderDevice::DrawGouraudPolyList);
	clockFast(GouraudPolyListCycles);
	//DrawGouraudPolygon only uses PolyFlags2 locally
	DWORD PolyFlags2 = 0;

	EndBufferingExcept(BV_TYPE_GOURAUD_POLYS);

	if (SceneNodeHack) {
		if ((Frame->X != m_sceneNodeX) || (Frame->Y != m_sceneNodeY)) {
			m_sceneNodeHackCount++;
			SetSceneNode(Frame);
		}
	}

	//Hit select path
	if (m_HitData)
	{
		INT Count = NumPts/3;
		CGClip::vec3_t triPts[3];

		for( INT j=0; j<Count; ++j )
		{
			const FTransTexture* Pt = &Pts[j*3];
			for( BYTE i=0; i<3; ++i )
			{
				triPts[i].x = Pt[i].Point.X;
				triPts[i].y = Pt[i].Point.Y;
				triPts[i].z = Pt[i].Point.Z;
			}
			m_gclip.SelectDrawTri(triPts);
		}
		return;
	}

	if (Info.Modifier)
	{
		FLOAT UM = Info.USize, VM = Info.VSize;
		for (INT i = 0; i < NumPts; ++i)
			Info.Modifier->TransformPointUV(Pts[i].U, Pts[i].V, UM, VM);
	}

	if (m_bufferActorTrisCutoff < 3)
	{
		EndBuffering();

		SetDefaultAAState();
		SetDefaultTextureState();

		//Decide if should request near Z range hack projection
		bool requestNearZRangeHackProjection = false;
		if (m_useZRangeHack && (GUglyHackFlags & HACKFLAGS_NoNearZ))
			requestNearZRangeHackProjection = true;

		//Set projection state
		SetProjectionState(requestNearZRangeHackProjection);

		//Check if should render fog and if vertex specular is supported
		bool drawFog = (((PolyFlags & (PF_RenderFog | PF_Translucent | PF_Modulated | PF_AlphaBlend)) == PF_RenderFog) && UseVertexSpecular) ? true : false;

		//If not drawing fog, disable the PF_RenderFog flag
		if (!drawFog)
			PolyFlags &= ~PF_RenderFog;

		SetBlend(PolyFlags);
		SetTextureNoPanBias(0, Info, PolyFlags);

		{
			IDirect3DVertexDeclaration9 *vertexDecl = (drawFog) ? m_twoColorSingleTextureVertexDecl : m_standardNTextureVertexDecl[0];
			IDirect3DVertexShader9 *vertexShader = NULL;
			IDirect3DPixelShader9 *pixelShader = NULL;

			if (UseFragmentProgram)
			{
				vertexShader = m_vpDefaultRenderingState;
				pixelShader = m_fpDefaultRenderingState;
				if (drawFog)
				{
					vertexShader = m_vpDefaultRenderingStateWithFog;
					pixelShader = m_fpDefaultRenderingStateWithFog;
				}
				if (m_gpFogEnabled)
				{
					vertexShader = m_vpDefaultRenderingStateWithLinearFog;
					pixelShader = m_fpDefaultRenderingStateWithLinearFog;
				}
			}

			//Set stream state
			SetStreamState(vertexDecl, vertexShader, pixelShader);
		}

		//Make sure at least NumPts entries are left in the vertex buffers
		if ((m_curVertexBufferPos + NumPts) >= VERTEX_ARRAY_SIZE)
			FlushVertexBuffers();

		//Lock vertexColor and texCoord0 buffers
		//Lock secondary color buffer if fog
		LockVertexColorBuffer();
		if (drawFog)
			LockSecondaryColorBuffer();
		LockTexCoordBuffer(0);

#ifdef UTGLR_DEBUG_ACTOR_WIREFRAME
		m_d3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
#endif

		INT Count = NumPts/3;
		FTransTexture* Ptz = NULL;
		INT i;
		for( INT j=0; j<Count; ++j )
		{
			Ptz = &Pts[j*3];
			for (i = 0; i < 3; i++)
			{
				FTransTexture& P = Ptz[i];

				FGLTexCoord &destTexCoord = m_pTexCoordArray[0][i];
				destTexCoord.u = P.U * TexInfo[0].UMult;
				destTexCoord.v = P.V * TexInfo[0].VMult;

				FGLVertexColor &destVertexColor = m_pVertexColorArray[i];
				destVertexColor.x = P.Point.X;
				destVertexColor.y = P.Point.Y;
				destVertexColor.z = P.Point.Z;

				if (PolyFlags & PF_Modulated)
					destVertexColor.color = 0xFFFFFFFF;
				else if (drawFog)
				{
					FLOAT f255_Times_One_Minus_FogW = 255.0f * (1.0f - P.Fog.W);
					destVertexColor.color = FPlaneTo_BGRScaled_A255(&P.Light, f255_Times_One_Minus_FogW);
					m_pSecondaryColorArray[i].specular = FPlaneTo_BGR_A0(&P.Fog);
				}
				else if (PolyFlags & (PF_AlphaBlend | PF_Modulated))
					destVertexColor.color = FPlaneTo_BGRA(&P.Light);
				else destVertexColor.color = FPlaneTo_BGR_A255(&P.Light);
			}

			//Draw the triangles
			m_d3dDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, m_curVertexBufferPos, 1);
		}

		//Unlock vertexColor and texCoord0 buffers
		//Unlock secondary color buffer if fog
		UnlockVertexColorBuffer();
		if (drawFog)
			UnlockSecondaryColorBuffer();
		UnlockTexCoordBuffer(0);

		//Advance vertex buffer position
		m_curVertexBufferPos += NumPts;

#ifdef UTGLR_DEBUG_ACTOR_WIREFRAME
		m_d3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
#endif

		return;
	}

	//Load texture cache id
	QWORD CacheID = Info.CacheID;

	//Only attempt to alter texture cache id on certain textures
	if ((CacheID & 0xFF) == 0xE0) {
		//Alter texture cache id if masked texture hack is enabled and texture is masked
		CacheID |= ((PolyFlags & PF_Masked) ? TEX_CACHE_ID_FLAG_MASKED : 0) & m_maskedTextureHackMask;

		//Check for 16 bit texture option
		if (Use16BitTextures) {
			if (Info.Palette && (Info.Palette[128].A == 255)) {
				CacheID |= TEX_CACHE_ID_FLAG_16BIT;
			}
		}
	}

	//Decide if should request near Z range hack projection
	if (m_useZRangeHack && (GUglyHackFlags & HACKFLAGS_NoNearZ)) {
		PolyFlags2 |= PF2_NEAR_Z_RANGE_HACK;
	}

	//Check if need to start new poly buffering
	//Make sure enough entries are left in the vertex buffers
	//based on the current position when it was locked
	if ((m_curPolyFlags != PolyFlags) ||
		(m_curPolyFlags2 != PolyFlags2) ||
		(TexInfo[0].CurrentCacheID != CacheID) ||
		((m_curVertexBufferPos + m_bufferedVerts + NumPts) >= (VERTEX_ARRAY_SIZE - 14)) ||
		(m_bufferedVerts == 0))
	{
		//Flush any previously buffered gouraud polys
		EndBuffering();

		//Check if vertex buffer flush is required
		if ((m_curVertexBufferPos + m_bufferedVerts + NumPts) >= (VERTEX_ARRAY_SIZE - 14))
			FlushVertexBuffers();

		//Start gouraud polygon buffering
		StartBuffering(BV_TYPE_GOURAUD_POLYS);

		//Check if should render fog and if vertex specular is supported
		//Also set other color flags
		if (PolyFlags & PF_Modulated)
			m_requestedColorFlags = 0;
		else
		{
			m_requestedColorFlags = CF_COLOR_ARRAY;
			if (((PolyFlags & (PF_RenderFog | PF_Translucent | PF_Modulated | PF_AlphaBlend)) == PF_RenderFog) && UseVertexSpecular)
				m_requestedColorFlags |= CF_FOG_MODE;
		}

		//If not drawing fog, disable the PF_RenderFog flag
		if (!(m_requestedColorFlags & CF_FOG_MODE)) {
			PolyFlags &= ~PF_RenderFog;
		}

		//Update current poly flags
		m_curPolyFlags = PolyFlags;
		m_curPolyFlags2 = PolyFlags2;

		//Set request near Z range hack projection flag
		m_requestNearZRangeHackProjection = (PolyFlags2 & PF2_NEAR_Z_RANGE_HACK) ? true : false;

		//Set default texture state
		SetDefaultTextureState();

		SetBlend(PolyFlags);

		if ((PolyFlags & PF_AlphaBlend) && (Info.Texture->PolyFlags & PF_Modulated)) //hack for
			SetBlend(Info.Texture->PolyFlags); // alphablend decal drawing

		SetTextureNoPanBias(0, Info, PolyFlags);

		//Lock vertexColor and texCoord0 buffers
		//Lock secondary color buffer if fog
		LockVertexColorBuffer();
		if (m_requestedColorFlags & CF_FOG_MODE) {
			LockSecondaryColorBuffer();
		}
		LockTexCoordBuffer(0);

		{
			IDirect3DVertexDeclaration9 *vertexDecl = (m_requestedColorFlags & CF_FOG_MODE) ? m_twoColorSingleTextureVertexDecl : m_standardNTextureVertexDecl[0];
			IDirect3DVertexShader9 *vertexShader = NULL;
			IDirect3DPixelShader9 *pixelShader = NULL;

			if (UseFragmentProgram)
			{
				vertexShader = m_vpDefaultRenderingState;
				pixelShader = m_fpDefaultRenderingState;
				if (m_requestedColorFlags & CF_FOG_MODE)
				{
					vertexShader = m_vpDefaultRenderingStateWithFog;
					pixelShader = m_fpDefaultRenderingStateWithFog;
				}
				if (m_gpFogEnabled)
				{
					vertexShader = m_vpDefaultRenderingStateWithLinearFog;
					pixelShader = m_fpDefaultRenderingStateWithLinearFog;
				}
			}

			//Set stream state
			SetStreamState(vertexDecl, vertexShader, pixelShader);
		}

		//Select a buffer verts proc
		if (m_requestedColorFlags & CF_FOG_MODE)
			m_pBuffer3VertsProc = Buffer3FoggedVerts;
		else if (m_requestedColorFlags & CF_COLOR_ARRAY)
		{
			if (PolyFlags & PF_AlphaBlend)
				m_pBuffer3VertsProc = Buffer3ColoredVertsAlpha;
			else m_pBuffer3VertsProc = Buffer3ColoredVerts;
		}
		else m_pBuffer3VertsProc = Buffer3BasicVerts;

		// Overflow check...
		NumPts = Min(NumPts, VERTEX_ARRAY_LIMIT);
	}

	//Buffer vertices
	(m_pBuffer3VertsProc)(this, Pts, NumPts);
	unclockFast(GouraudPolyListCycles);
	unguard;
}
#endif

void UD3D9RenderDevice::DrawTile(FSceneNode* Frame, FTextureInfo& Info, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, class FSpanBuffer* Span, FLOAT Z, FPlane Color, FPlane Fog, DWORD PolyFlags) {
	UTGLR_DEBUG_CALL_COUNT(DrawTile);
	guard(UD3D9RenderDevice::DrawTile);

	//DrawTile does not use PolyFlags2
	const DWORD PolyFlags2 = 0;

	EndBufferingExcept(BV_TYPE_TILES);

	if (SceneNodeHack) {
		if ((Frame->X != m_sceneNodeX) || (Frame->Y != m_sceneNodeY)) {
			m_sceneNodeHackCount++;
			SetSceneNode(Frame);
		}
	}

	//Adjust Z coordinate if Z range hack is active
	if (m_useZRangeHack) {
		if ((Z >= 0.5f) && (Z < 8.0f)) {
			Z = (((Z - 0.5f) / 7.5f) * 4.0f) + 4.0f;
		}
	}

	FLOAT PX1 = X - Frame->FX2;
	FLOAT PX2 = PX1 + XL;
	FLOAT PY1 = Y - Frame->FY2;
	FLOAT PY2 = PY1 + YL;

	FLOAT RPX1 = m_RFX2 * PX1;
	FLOAT RPX2 = m_RFX2 * PX2;
	FLOAT RPY1 = m_RFY2 * PY1;
	FLOAT RPY2 = m_RFY2 * PY2;
	if (!Frame->Viewport->IsOrtho()) {
		RPX1 *= Z;
		RPX2 *= Z;
		RPY1 *= Z;
		RPY2 *= Z;
	}

	//Hit select path
	if (m_HitData) {
		CGClip::vec3_t triPts[3];

		triPts[0].x = RPX1;
		triPts[0].y = RPY1;
		triPts[0].z = Z;

		triPts[1].x = RPX2;
		triPts[1].y = RPY1;
		triPts[1].z = Z;

		triPts[2].x = RPX2;
		triPts[2].y = RPY2;
		triPts[2].z = Z;

		m_gclip.SelectDrawTri(triPts);

		triPts[0].x = RPX1;
		triPts[0].y = RPY1;
		triPts[0].z = Z;

		triPts[1].x = RPX2;
		triPts[1].y = RPY2;
		triPts[1].z = Z;

		triPts[2].x = RPX1;
		triPts[2].y = RPY2;
		triPts[2].z = Z;

		m_gclip.SelectDrawTri(triPts);

		return;
	}

	if (1) {
		//Load texture cache id
		QWORD CacheID = Info.CacheID;

		//Only attempt to alter texture cache id on certain textures
		if ((CacheID & 0xFF) == 0xE0) {
			//Alter texture cache id if masked texture hack is enabled and texture is masked
			CacheID |= ((PolyFlags & PF_Masked) ? TEX_CACHE_ID_FLAG_MASKED : 0) & m_maskedTextureHackMask;

			//Check for 16 bit texture option
			if (Use16BitTextures) {
				if (Info.Palette && (Info.Palette[128].A == 255)) {
					CacheID |= TEX_CACHE_ID_FLAG_16BIT;
				}
			}
		}

		//Check if need to start new tile buffering
		if ((m_curPolyFlags != PolyFlags) ||
			(m_curPolyFlags2 != PolyFlags2) ||
			(TexInfo[0].CurrentCacheID != CacheID) ||
			((m_curVertexBufferPos + m_bufferedVerts) >= (VERTEX_ARRAY_SIZE - 6)) ||
			(m_bufferedVerts == 0))
		{
			//Flush any previously buffered tiles
			EndBuffering();

			//Check if vertex buffer flush is required
			if ((m_curVertexBufferPos + m_bufferedVerts) >= (VERTEX_ARRAY_SIZE - 6)) {
				FlushVertexBuffers();
			}

			//Start tile buffering
			StartBuffering(BV_TYPE_TILES);

			//Update current poly flags (before possible local modification)
			m_curPolyFlags = PolyFlags;
			m_curPolyFlags2 = PolyFlags2;

#ifdef UTGLR_RUNE_BUILD
			if (Info.Palette && Info.Palette[128].A != 255 && !(PolyFlags & (PF_Translucent | PF_AlphaBlend))) {
#elif UTGLR_UNREAL_227_BUILD
			if (Info.Palette && Info.Palette[128].A != 255 && !(PolyFlags & (PF_Translucent | PF_AlphaBlend))) {
#else
			if (Info.Palette && Info.Palette[128].A != 255 && !(PolyFlags & PF_Translucent)) {
#endif
				PolyFlags |= PF_Highlighted | PF_Occlude;
			}

			//Set default texture state
			SetDefaultTextureState();

			SetBlend(PolyFlags);
			SetTextureNoPanBias(0, Info, PolyFlags);

			if (PolyFlags & PF_Modulated) {
				m_requestedColorFlags = 0;
			}
			else {
				m_requestedColorFlags = CF_COLOR_ARRAY;
			}

			//Lock vertexColor and texCoord0 buffers
			LockVertexColorBuffer();
			LockTexCoordBuffer(0);

			//Set stream state
			SetDefaultStreamState();
		}

		//Get tile color
		DWORD tileColor;
		if (PolyFlags & PF_Modulated)
			tileColor = 0xFFFFFFFF;
		else
		{
			if (UseSSE2) {
#ifdef UTGLR_INCLUDE_SSE_CODE
				constexpr __m128 fColorMul = { 255.0f, 255.0f, 255.0f, 0.0f };
				__m128 fColorMulReg;
				__m128 fColor;
				__m128 fAlpha;
				__m128i iColor;

				fColorMulReg = fColorMul;
				fColor = _mm_loadu_ps(&Color.X);
				fColor = _mm_mul_ps(fColor, fColorMulReg);

				//RGBA to BGRA
				fColor = _mm_shuffle_ps(fColor, fColor, _MM_SHUFFLE(3, 0, 1, 2));

				fAlpha = _mm_setzero_ps();
				fAlpha = _mm_move_ss(fAlpha, fColorMulReg);
#ifdef UTGLR_RUNE_BUILD
				if (PolyFlags & PF_AlphaBlend) {
					fAlpha = _mm_mul_ss(fAlpha, _mm_load_ss(&Info.Texture->Alpha));
				}
#elif UTGLR_UNREAL_227_BUILD
				if (PolyFlags & PF_AlphaBlend)
					fAlpha = _mm_mul_ss(fAlpha, _mm_load_ss(&Color.W));
#endif
				fAlpha = _mm_shuffle_ps(fAlpha, fAlpha, _MM_SHUFFLE(0, 1, 1, 1));

				fColor = _mm_or_ps(fColor, fAlpha);

				iColor = _mm_cvtps_epi32(fColor);
				iColor = _mm_packs_epi32(iColor, iColor);
				iColor = _mm_packus_epi16(iColor, iColor);

				tileColor = _mm_cvtsi128_si32(iColor);
#endif
			}
			else {
#ifdef UTGLR_RUNE_BUILD
				if (PolyFlags & PF_AlphaBlend) {
					Color.W = Info.Texture->Alpha;
					tileColor = FPlaneTo_BGRAClamped(&Color);
				}
				else {
					tileColor = FPlaneTo_BGRClamped_A255(&Color);
				}
#elif UTGLR_UNREAL_227_BUILD
				if (PolyFlags & PF_AlphaBlend)
					tileColor = FPlaneTo_BGRAClamped(&Color);
				else tileColor = FPlaneTo_BGRClamped_A255(&Color);
#else
				tileColor = FPlaneTo_BGRClamped_A255(&Color);
#endif
	}
		}
		
		//Buffer the tile
		FGLVertexColor *pVertexColorArray = &m_pVertexColorArray[m_bufferedVerts];
		FGLTexCoord *pTexCoordArray = &m_pTexCoordArray[0][m_bufferedVerts];

		pVertexColorArray[0] = FGLVertexColor(RPX1, RPY1, Z, tileColor);
		pVertexColorArray[1] = FGLVertexColor(RPX2, RPY1, Z, tileColor);
		pVertexColorArray[2] = FGLVertexColor(RPX2, RPY2, Z, tileColor);
		pVertexColorArray[3] = FGLVertexColor(RPX1, RPY1, Z, tileColor);
		pVertexColorArray[4] = FGLVertexColor(RPX2, RPY2, Z, tileColor);
		pVertexColorArray[5] = FGLVertexColor(RPX1, RPY2, Z, tileColor);

		FLOAT TexInfoUMult = TexInfo[0].UMult;
		FLOAT TexInfoVMult = TexInfo[0].VMult;

		FLOAT SU1 = (U) * TexInfoUMult;
		FLOAT SU2 = (U + UL) * TexInfoUMult;
		FLOAT SV1 = (V) * TexInfoVMult;
		FLOAT SV2 = (V + VL) * TexInfoVMult;

		pTexCoordArray[0] = FGLTexCoord(SU1, SV1);
		pTexCoordArray[1] = FGLTexCoord(SU2, SV1);
		pTexCoordArray[2] = FGLTexCoord(SU2, SV2);
		pTexCoordArray[3] = FGLTexCoord(SU1, SV1);
		pTexCoordArray[4] = FGLTexCoord(SU2, SV2);
		pTexCoordArray[5] = FGLTexCoord(SU1, SV2);

		if (Info.Modifier)
		{
			for(INT i=0; i<6; i++)
				Info.Modifier->TransformPoint(pTexCoordArray[i].u, pTexCoordArray[i].v);
		}

		m_bufferedVerts += 6;
	}

	unguard;
}

void UD3D9RenderDevice::Draw3DLine(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2) {
	UTGLR_DEBUG_CALL_COUNT(Draw3DLine);
	guard(UD3D9RenderDevice::Draw3DLine);

	P1 = P1.TransformPointBy(Frame->Coords);
	P2 = P2.TransformPointBy(Frame->Coords);
	if (Frame->Viewport->IsOrtho()) {
		// Zoom.
		FLOAT rcpZoom = 1.0f / Frame->Zoom;
		P1.X = (P1.X * rcpZoom) + Frame->FX2;
		P1.Y = (P1.Y * rcpZoom) + Frame->FY2;
		P2.X = (P2.X * rcpZoom) + Frame->FX2;
		P2.Y = (P2.Y * rcpZoom) + Frame->FY2;
		P1.Z = P2.Z = 1;

		// See if points form a line parallel to our line of sight (i.e. line appears as a dot).
		if (Abs(P2.X - P1.X) + Abs(P2.Y - P1.Y) >= 0.2f) {
			Draw2DLine(Frame, Color, LineFlags, P1, P2);
		}
		else if (Frame->Viewport->Actor->OrthoZoom < ORTHO_LOW_DETAIL) {
			Draw2DPoint(Frame, Color, LINE_None, P1.X - 1.0f, P1.Y - 1.0f, P1.X + 1.0f, P1.Y + 1.0f, P1.Z);
		}
	}
	else {
		EndBufferingExcept(BV_TYPE_LINES);

		//Hit select path
		if (m_HitData) {
			CGClip::vec3_t lnPts[2];

			lnPts[0].x = P1.X;
			lnPts[0].y = P1.Y;
			lnPts[0].z = P1.Z;

			lnPts[1].x = P2.X;
			lnPts[1].y = P2.Y;
			lnPts[1].z = P2.Z;

			m_gclip.SelectDrawLine(lnPts);

			return;
		}

		//Draw3DLine does not use PolyFlags2
		const DWORD PolyFlags = PF_Highlighted | PF_Occlude;
		const DWORD PolyFlags2 = 0;

		//Check if need to start new line buffering
		if ((m_curPolyFlags != PolyFlags) ||
			(m_curPolyFlags2 != PolyFlags2) ||
			(TexInfo[0].CurrentCacheID != TEX_CACHE_ID_NO_TEX) ||
			((m_curVertexBufferPos + m_bufferedVerts) >= (VERTEX_ARRAY_SIZE - 2)) ||
			(m_bufferedVerts == 0))
		{
			//Flush any previously buffered lines
			EndBuffering();

			//Check if vertex buffer flush is required
			if ((m_curVertexBufferPos + m_bufferedVerts) >= (VERTEX_ARRAY_SIZE - 2)) {
				FlushVertexBuffers();
			}

			//Start line buffering
			StartBuffering(BV_TYPE_LINES);

			SetDefaultAAState();
			SetDefaultProjectionState();
			SetDefaultStreamState();
			SetDefaultTextureState();

			//Update current poly flags
			m_curPolyFlags = PolyFlags;
			m_curPolyFlags2 = PolyFlags2;

			//Set blending and no texture for lines
			SetBlend(PolyFlags);
			SetNoTexture(0);

			//Lock vertexColor and texCoord0 buffers
			LockVertexColorBuffer();
			LockTexCoordBuffer(0);
		}

		//Get line color
		DWORD lineColor = FPlaneTo_BGRClamped_A255(&Color);

		//Buffer the line
		FGLVertexColor *pVertexColorArray = &m_pVertexColorArray[m_bufferedVerts];
		FGLTexCoord *pTexCoordArray = &m_pTexCoordArray[0][m_bufferedVerts];

		pVertexColorArray[0].x = P1.X;
		pVertexColorArray[0].y = P1.Y;
		pVertexColorArray[0].z = P1.Z;
		pVertexColorArray[0].color = lineColor;

		pVertexColorArray[1].x = P2.X;
		pVertexColorArray[1].y = P2.Y;
		pVertexColorArray[1].z = P2.Z;
		pVertexColorArray[1].color = lineColor;

		pTexCoordArray[0].u = 0.0f;
		pTexCoordArray[0].v = 0.0f;

		pTexCoordArray[1].u = 1.0f;
		pTexCoordArray[1].v = 0.0f;

		m_bufferedVerts += 2;
	}
	unguard;
}

void UD3D9RenderDevice::Draw2DLine(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2) {
	UTGLR_DEBUG_CALL_COUNT(Draw2DLine);
	guard(UD3D9RenderDevice::Draw2DLine);

	EndBufferingExcept(BV_TYPE_LINES);

	//Get line coordinates back in 3D
	FLOAT X1Pos = m_RFX2 * (P1.X - Frame->FX2);
	FLOAT Y1Pos = m_RFY2 * (P1.Y - Frame->FY2);
	FLOAT X2Pos = m_RFX2 * (P2.X - Frame->FX2);
	FLOAT Y2Pos = m_RFY2 * (P2.Y - Frame->FY2);
	if (!Frame->Viewport->IsOrtho()) {
		X1Pos *= P1.Z;
		Y1Pos *= P1.Z;
		X2Pos *= P2.Z;
		Y2Pos *= P2.Z;
	}

	//Hit select path
	if (m_HitData) {
		CGClip::vec3_t lnPts[2];

		lnPts[0].x = X1Pos;
		lnPts[0].y = Y1Pos;
		lnPts[0].z = P1.Z;

		lnPts[1].x = X2Pos;
		lnPts[1].y = Y2Pos;
		lnPts[1].z = P2.Z;

		m_gclip.SelectDrawLine(lnPts);

		return;
	}

	//Draw2DLine does not use PolyFlags2
	const DWORD PolyFlags = PF_Highlighted | PF_Occlude;
	const DWORD PolyFlags2 = 0;

	//Check if need to start new line buffering
	if ((m_curPolyFlags != PolyFlags) ||
		(m_curPolyFlags2 != PolyFlags2) ||
		(TexInfo[0].CurrentCacheID != TEX_CACHE_ID_NO_TEX) ||
		((m_curVertexBufferPos + m_bufferedVerts) >= (VERTEX_ARRAY_SIZE - 2)) ||
		(m_bufferedVerts == 0))
	{
		//Flush any previously buffered lines
		EndBuffering();

		//Check if vertex buffer flush is required
		if ((m_curVertexBufferPos + m_bufferedVerts) >= (VERTEX_ARRAY_SIZE - 2)) {
			FlushVertexBuffers();
		}

		//Start line buffering
		StartBuffering(BV_TYPE_LINES);

		SetDefaultAAState();
		SetDefaultProjectionState();
		SetDefaultStreamState();
		SetDefaultTextureState();

		//Update current poly flags
		m_curPolyFlags = PolyFlags;
		m_curPolyFlags2 = PolyFlags2;

		//Set blending and no texture for lines
		SetBlend(PolyFlags);
		SetNoTexture(0);

		//Lock vertexColor and texCoord0 buffers
		LockVertexColorBuffer();
		LockTexCoordBuffer(0);
	}

	//Get line color
	DWORD lineColor = FPlaneTo_BGRClamped_A255(&Color);

	//Buffer the line
	FGLVertexColor *pVertexColorArray = &m_pVertexColorArray[m_bufferedVerts];
	FGLTexCoord *pTexCoordArray = &m_pTexCoordArray[0][m_bufferedVerts];

	pVertexColorArray[0].x = X1Pos;
	pVertexColorArray[0].y = Y1Pos;
	pVertexColorArray[0].z = P1.Z;
	pVertexColorArray[0].color = lineColor;

	pVertexColorArray[1].x = X2Pos;
	pVertexColorArray[1].y = Y2Pos;
	pVertexColorArray[1].z = P2.Z;
	pVertexColorArray[1].color = lineColor;

	pTexCoordArray[0].u = 0.0f;
	pTexCoordArray[0].v = 0.0f;

	pTexCoordArray[1].u = 1.0f;
	pTexCoordArray[1].v = 0.0f;

	m_bufferedVerts += 2;

	unguard;
}

void UD3D9RenderDevice::Draw2DPoint(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FLOAT X1, FLOAT Y1, FLOAT X2, FLOAT Y2, FLOAT Z) {
	UTGLR_DEBUG_CALL_COUNT(Draw2DPoint);
	guard(UD3D9RenderDevice::Draw2DPoint);

	EndBufferingExcept(BV_TYPE_POINTS);

	//Get point coordinates back in 3D
	FLOAT X1Pos = m_RFX2 * (X1 - Frame->FX2);
	FLOAT Y1Pos = m_RFY2 * (Y1 - Frame->FY2);
	FLOAT X2Pos = m_RFX2 * (X2 - Frame->FX2);
	FLOAT Y2Pos = m_RFY2 * (Y2 - Frame->FY2);
	if (Frame->Viewport->IsOrtho())
		Z = 1.f;
	else
	{
		Z = Abs(Z);
		X1Pos *= Z;
		Y1Pos *= Z;
		X2Pos *= Z;
		Y2Pos *= Z;
	}

	//Hit select path
	if (m_HitData)
	{
		CGClip::vec3_t triPts[3];

		triPts[0].x = X1Pos;
		triPts[0].y = Y1Pos;
		triPts[0].z = Z;

		triPts[1].x = X2Pos;
		triPts[1].y = Y1Pos;
		triPts[1].z = Z;

		triPts[2].x = X2Pos;
		triPts[2].y = Y2Pos;
		triPts[2].z = Z;

		m_gclip.SelectDrawTri(triPts);

		triPts[0].x = X1Pos;
		triPts[0].y = Y1Pos;
		triPts[0].z = Z;

		triPts[1].x = X2Pos;
		triPts[1].y = Y2Pos;
		triPts[1].z = Z;

		triPts[2].x = X1Pos;
		triPts[2].y = Y2Pos;
		triPts[2].z = Z;

		m_gclip.SelectDrawTri(triPts);

		return;
	}

	//Draw2DPoint does not use PolyFlags2
	const DWORD PolyFlags = PF_Highlighted | PF_Occlude;
	const DWORD PolyFlags2 = 0;

	//Check if need to start new point buffering
	if ((m_curPolyFlags != PolyFlags) ||
		(m_curPolyFlags2 != PolyFlags2) ||
		(TexInfo[0].CurrentCacheID != TEX_CACHE_ID_NO_TEX) ||
		((m_curVertexBufferPos + m_bufferedVerts) >= (VERTEX_ARRAY_SIZE - 6)) ||
		(m_bufferedVerts == 0))
	{
		//Flush any previously buffered points
		EndBuffering();

		//Check if vertex buffer flush is required
		if ((m_curVertexBufferPos + m_bufferedVerts) >= (VERTEX_ARRAY_SIZE - 6)) {
			FlushVertexBuffers();
		}

		//Start point buffering
		StartBuffering(BV_TYPE_POINTS);

		SetDefaultAAState();
		SetDefaultProjectionState();
		SetDefaultStreamState();
		SetDefaultTextureState();

		//Update current poly flags
		m_curPolyFlags = PolyFlags;
		m_curPolyFlags2 = PolyFlags2;

		//Set blending and no texture for points
		SetBlend(PolyFlags);
		SetNoTexture(0);

		//Lock vertexColor and texCoord0 buffers
		LockVertexColorBuffer();
		LockTexCoordBuffer(0);
	}

	//Get point color
	DWORD pointColor = FPlaneTo_BGRClamped_A255(&Color);

	//Buffer the point
	FGLVertexColor *pVertexColorArray = &m_pVertexColorArray[m_bufferedVerts];
	FGLTexCoord *pTexCoordArray = &m_pTexCoordArray[0][m_bufferedVerts];

	pVertexColorArray[0].x = X1Pos;
	pVertexColorArray[0].y = Y1Pos;
	pVertexColorArray[0].z = Z;
	pVertexColorArray[0].color = pointColor;

	pVertexColorArray[1].x = X2Pos;
	pVertexColorArray[1].y = Y1Pos;
	pVertexColorArray[1].z = Z;
	pVertexColorArray[1].color = pointColor;

	pVertexColorArray[2].x = X2Pos;
	pVertexColorArray[2].y = Y2Pos;
	pVertexColorArray[2].z = Z;
	pVertexColorArray[2].color = pointColor;

	pVertexColorArray[3].x = X1Pos;
	pVertexColorArray[3].y = Y1Pos;
	pVertexColorArray[3].z = Z;
	pVertexColorArray[3].color = pointColor;

	pVertexColorArray[4].x = X2Pos;
	pVertexColorArray[4].y = Y2Pos;
	pVertexColorArray[4].z = Z;
	pVertexColorArray[4].color = pointColor;

	pVertexColorArray[5].x = X1Pos;
	pVertexColorArray[5].y = Y2Pos;
	pVertexColorArray[5].z = Z;
	pVertexColorArray[5].color = pointColor;

	FLOAT SU1 = 0.0f;
	FLOAT SU2 = 1.0f;
	FLOAT SV1 = 0.0f;
	FLOAT SV2 = 1.0f;

	pTexCoordArray[0].u = SU1;
	pTexCoordArray[0].v = SV1;

	pTexCoordArray[1].u = SU2;
	pTexCoordArray[1].v = SV1;

	pTexCoordArray[2].u = SU2;
	pTexCoordArray[2].v = SV2;

	pTexCoordArray[3].u = SU1;
	pTexCoordArray[3].v = SV1;

	pTexCoordArray[4].u = SU2;
	pTexCoordArray[4].v = SV2;

	pTexCoordArray[5].u = SU1;
	pTexCoordArray[5].v = SV2;

	m_bufferedVerts += 6;

	unguard;
}


void UD3D9RenderDevice::ClearZ(FSceneNode* Frame) {
	UTGLR_DEBUG_CALL_COUNT(ClearZ);
	guard(UD3D9RenderDevice::ClearZ);

	EndBuffering();

	//Default AA state not required for glClear
	//Default projection state not required for glClear
	//Default stream state not required for glClear
	//Default texture state not required for glClear

	SetBlend(PF_Occlude);
	m_d3dDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER, 0, 1.0f, 0);

	if (Viewport->HitTesting)
		m_gclip.ClearDepth();

	unguard;
}

void UD3D9RenderDevice::PushHit(const BYTE* Data, INT Count) {
	guard(UD3D9RenderDevice::PushHit);

	INT i;

	EndBuffering();

	//Add to stack
	for (i = 0; i < Count; i += 4) {
		DWORD hitName = *(DWORD *)(Data + i);
		m_gclip.PushHitName(hitName);
	}

	unguard;
}

void UD3D9RenderDevice::PopHit(INT Count, UBOOL bForce) {
	guard(UD3D9RenderDevice::PopHit);

	EndBuffering();

	INT i;
	bool selHit;

	//Handle hit
	selHit = m_gclip.CheckNewSelectHit();
	if (selHit || bForce) {
		DWORD nHitNameBytes;

		nHitNameBytes = m_gclip.GetHitNameStackSize() * 4;
		if (nHitNameBytes <= m_HitBufSize) {
			m_gclip.GetHitNameStackValues((unsigned int *)m_HitData, nHitNameBytes / 4);
			m_HitCount = nHitNameBytes;
		}
		else {
			m_HitCount = 0;
		}
	}

	//Remove from stack
	for (i = 0; i < Count; i += 4) {
		m_gclip.PopHitName();
	}

	unguard;
}

void UD3D9RenderDevice::ReadPixels(FColor* Pixels, UBOOL bGammaCorrectOutput) {
	guard(UD3D9RenderDevice::ReadPixels);

	INT x, y;
	INT SizeX, SizeY;
	INT StartX = 0, StartY = 0;
	HRESULT hResult;
	IDirect3DSurface9 *d3dsFrontBuffer = NULL;
	D3DDISPLAYMODE d3ddm;
	HDC hDibDC = 0;
	HBITMAP hDib = 0;
	LPVOID pDibData = 0;
	DWORD *pScreenshot = 0;
	INT screenshotPitch;

	SizeX = Viewport->SizeX;
	SizeY = Viewport->SizeY;

	//Get current display mode
	hResult = m_d3dDevice->GetDisplayMode(0, &d3ddm);
	if (FAILED(hResult)) {
		return;
	}

	//Allocate resources and get screen data
	if (m_d3dpp.Windowed) { //Windowed
		struct {
			BITMAPINFOHEADER bmiHeader;
			DWORD bmiColors[3];
		} bmi;
		HBITMAP hOldBitmap;

		//Create memory DC
		hDibDC = CreateCompatibleDC(m_hDC);
		if (!hDibDC) {
			return;
		}

		//DIB format
		appMemzero(&bmi.bmiHeader, sizeof(bmi.bmiHeader));
		bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
		bmi.bmiHeader.biWidth = SizeX;
		bmi.bmiHeader.biHeight = -SizeY;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_BITFIELDS;
		bmi.bmiHeader.biSizeImage = SizeX * SizeY * 4;
		bmi.bmiHeader.biXPelsPerMeter = 0;
		bmi.bmiHeader.biYPelsPerMeter = 0;
		bmi.bmiHeader.biClrUsed = 0;
		bmi.bmiHeader.biClrImportant = 0;
		bmi.bmiColors[0] = 0x00FF0000;
		bmi.bmiColors[1] = 0x0000FF00;
		bmi.bmiColors[2] = 0x000000FF;

		//Create DIB
		hDib = CreateDIBSection(
			hDibDC,
			(BITMAPINFO *)&bmi,
			DIB_RGB_COLORS,
			&pDibData,
			NULL,
			0);
		if (!hDib) {
			DeleteDC(hDibDC);
			return;
		}

		//Get copy of window contents
		hOldBitmap = (HBITMAP)SelectObject(hDibDC, hDib);
		BitBlt(hDibDC, 0, 0, SizeX, SizeY, m_hDC, 0, 0, SRCCOPY);
		SelectObject(hDibDC, hOldBitmap);

		//Set pointer to screenshot data and pitch
		pScreenshot = (DWORD *)pDibData;
		screenshotPitch = bmi.bmiHeader.biWidth * 4;
	}
	else { //Fullscreen
		//Create surface to hold screenshot
		hResult = m_d3dDevice->CreateOffscreenPlainSurface(d3ddm.Width, d3ddm.Height, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &d3dsFrontBuffer, NULL);
		if (FAILED(hResult)) {
			return;
		}

		//Get copy of front buffer
		hResult = m_d3dDevice->GetFrontBufferData(0, d3dsFrontBuffer);
		if (FAILED(hResult)) {
			//Release surface to hold screenshot
			d3dsFrontBuffer->Release();

			return;
		}

		//Clamp size just in case
		if (SizeX > d3ddm.Width) SizeX = d3ddm.Width;
		if (SizeY > d3ddm.Height) SizeY = d3ddm.Height;

		//Lock screenshot surface
		D3DLOCKED_RECT lockRect;
		hResult = d3dsFrontBuffer->LockRect(&lockRect, NULL, D3DLOCK_NOSYSLOCK | D3DLOCK_READONLY);
		if (FAILED(hResult)) {
			//Release surface to hold screenshot
			d3dsFrontBuffer->Release();

			return;
		}

		//Set pointer to screenshot data and pitch
		pScreenshot = (DWORD *)lockRect.pBits;
		screenshotPitch = lockRect.Pitch;
	}


	//Copy screenshot data
	if (pScreenshot) {
		INT DestSizeX = Viewport->SizeX;
		pScreenshot = (DWORD *)((BYTE *)pScreenshot + (StartY * screenshotPitch));
		for (y = 0; y < SizeY; y++) {
			for (x = 0; x < SizeX; x++) {
				DWORD dwPixel = pScreenshot[StartX + x];
				Pixels[(y * DestSizeX) + x] = FColor(((dwPixel >> 0) & 0xFF), ((dwPixel >> 8) & 0xFF), ((dwPixel >> 16) & 0xFF), 0xFF);
			}
			pScreenshot = (DWORD *)((BYTE *)pScreenshot + screenshotPitch);
		}
	}


	//Free resources
	if (m_d3dpp.Windowed) { //Windowed
		if (hDib) {
			DeleteObject(hDib);
		}
		if (hDibDC) {
			DeleteDC(hDibDC);
		}
	}
	else { //Fullscreen
		//Unlock screenshot surface
		d3dsFrontBuffer->UnlockRect();

		//Release surface to hold screenshot
		d3dsFrontBuffer->Release();
	}


	//Gamma correct screenshots if the option is true and the gamma ramp was set successfully
	if (bGammaCorrectOutput && GammaCorrectScreenshots && m_setGammaRampSucceeded) {
		INT DestSizeX = Viewport->SizeX;
		INT DestSizeY = Viewport->SizeY;
		FByteGammaRamp gammaByteRamp;
		BuildGammaRamp(SavedGammaCorrection, SavedGammaCorrection, SavedGammaCorrection, Brightness, gammaByteRamp);
		for (y = 0; y < DestSizeY; y++) {
			for (x = 0; x < DestSizeX; x++) {
				Pixels[x + y * DestSizeX].R = gammaByteRamp.red[Pixels[x + y * DestSizeX].R];
				Pixels[x + y * DestSizeX].G = gammaByteRamp.green[Pixels[x + y * DestSizeX].G];
				Pixels[x + y * DestSizeX].B = gammaByteRamp.blue[Pixels[x + y * DestSizeX].B];
			}
		}
	}

	unguard;
}

void UD3D9RenderDevice::EndFlash() {
	UTGLR_DEBUG_CALL_COUNT(EndFlash);
	guard(UD3D9RenderDevice::EndFlash);
	if ((FlashScale != FPlane(0.5f, 0.5f, 0.5f, 0.0f)) || (FlashFog != FPlane(0.0f, 0.0f, 0.0f, 0.0f))) {
		EndBuffering();

		SetDefaultAAState();
		SetDefaultProjectionState();
		SetDefaultStreamState();
		SetDefaultTextureState();

		SetBlend(PF_Highlighted);
		SetNoTexture(0);

		FPlane tempPlane = FPlane(FlashFog.X, FlashFog.Y, FlashFog.Z, 1.0f - Min(FlashScale.X * 2.0f, 1.0f));
		DWORD flashColor = FPlaneTo_BGRA(&tempPlane);

		FLOAT RFX2 = m_RProjZ;
		FLOAT RFY2 = m_RProjZ * m_Aspect;

		//Adjust Z coordinate if Z range hack is active
		FLOAT ZCoord = 1.0f;
		if (m_useZRangeHack) {
			ZCoord = (((ZCoord - 0.5f) / 7.5f) * 4.0f) + 4.0f;
		}

		//Make sure at least 4 entries are left in the vertex buffers
		if ((m_curVertexBufferPos + 4) >= VERTEX_ARRAY_SIZE) {
			FlushVertexBuffers();
		}

		//Lock vertexColor and texCoord0 buffers
		LockVertexColorBuffer();
		LockTexCoordBuffer(0);

		FGLTexCoord *pTexCoordArray = m_pTexCoordArray[0];
		FGLVertexColor *pVertexColorArray = m_pVertexColorArray;

		pTexCoordArray[0].u = 0.0f;
		pTexCoordArray[0].v = 0.0f;

		pTexCoordArray[1].u = 1.0f;
		pTexCoordArray[1].v = 0.0f;

		pTexCoordArray[2].u = 1.0f;
		pTexCoordArray[2].v = 1.0f;

		pTexCoordArray[3].u = 0.0f;
		pTexCoordArray[3].v = 1.0f;

		pVertexColorArray[0].x = RFX2 * (-1.0f * ZCoord);
		pVertexColorArray[0].y = RFY2 * (-1.0f * ZCoord);
		pVertexColorArray[0].z = ZCoord;
		pVertexColorArray[0].color = flashColor;

		pVertexColorArray[1].x = RFX2 * (+1.0f * ZCoord);
		pVertexColorArray[1].y = RFY2 * (-1.0f * ZCoord);
		pVertexColorArray[1].z = ZCoord;
		pVertexColorArray[1].color = flashColor;

		pVertexColorArray[2].x = RFX2 * (+1.0f * ZCoord);
		pVertexColorArray[2].y = RFY2 * (+1.0f * ZCoord);
		pVertexColorArray[2].z = ZCoord;
		pVertexColorArray[2].color = flashColor;

		pVertexColorArray[3].x = RFX2 * (-1.0f * ZCoord);
		pVertexColorArray[3].y = RFY2 * (+1.0f * ZCoord);
		pVertexColorArray[3].z = ZCoord;
		pVertexColorArray[3].color = flashColor;

		//Unlock vertexColor and texCoord0 buffers
		UnlockVertexColorBuffer();
		UnlockTexCoordBuffer(0);

		//Draw the square
		m_d3dDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, m_curVertexBufferPos, 2);

		//Advance vertex buffer position
		m_curVertexBufferPos += 4;
	}
	unguard;
}

void UD3D9RenderDevice::PrecacheTexture(FTextureInfo& Info, DWORD PolyFlags) {
	guard(UD3D9RenderDevice::PrecacheTexture);
	SetTextureNoPanBias(0, Info, PolyFlags);
	unguard;
}


//This function is safe to call multiple times to initialize once
void UD3D9RenderDevice::InitNoTextureSafe(void) {
	guard(UD3D9RenderDevice::InitNoTexture);
	unsigned int u, v;
	HRESULT hResult;
	D3DLOCKED_RECT lockRect;
	DWORD *pTex;

	//Return early if already initialized
	if (m_pNoTexObj != 0) {
		return;
	}

	//Create the texture
	hResult = m_d3dDevice->CreateTexture(4, 4, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &m_pNoTexObj, NULL);
	if (FAILED(hResult)) {
		appErrorf(TEXT("CreateTexture (basic RGBA8) failed"));
	}

	//Lock texture level 0
	if (FAILED(m_pNoTexObj->LockRect(0, &lockRect, NULL, D3DLOCK_NOSYSLOCK))) {
		appErrorf(TEXT("Texture lock failed"));
	}

	//Write texture
	pTex = (DWORD *)lockRect.pBits;
	for (u = 0; u < 4; u++) {
		for (v = 0; v < 4; v++) {
			pTex[v] = 0xFFFFFFFF;
		}
		pTex = (DWORD *)((BYTE *)pTex + lockRect.Pitch);
	}

	//Unlock texture level 0
	if (FAILED(m_pNoTexObj->UnlockRect(0))) {
		appErrorf(TEXT("Texture unlock failed"));
	}

	return;
	unguard;
}

//This function is safe to call multiple times to initialize once
void UD3D9RenderDevice::InitAlphaTextureSafe(void) {
	guard(UD3D9RenderDevice::InitAlphaTexture);
	unsigned int u;
	HRESULT hResult;
	D3DLOCKED_RECT lockRect;
	BYTE *pTex;

	//Return early if already initialized
	if (m_pAlphaTexObj != 0) {
		return;
	}

	//Create the texture
	hResult = m_d3dDevice->CreateTexture(256, 1, 1, 0, D3DFMT_A8, D3DPOOL_MANAGED, &m_pAlphaTexObj, NULL);
	if (FAILED(hResult)) {
		appErrorf(TEXT("CreateTexture (alpha) failed"));
	}

	//Lock texture level 0
	if (FAILED(m_pAlphaTexObj->LockRect(0, &lockRect, NULL, D3DLOCK_NOSYSLOCK))) {
		appErrorf(TEXT("Texture lock failed"));
	}

	//Write texture
	pTex = (BYTE *)lockRect.pBits;
	for (u = 0; u < 256; u++) {
		pTex[u] = 255 - u;
	}

	//Unlock texture level 0
	if (FAILED(m_pAlphaTexObj->UnlockRect(0))) {
		appErrorf(TEXT("Texture unlock failed"));
	}

	return;
	unguard;
}

void UD3D9RenderDevice::ScanForOldTextures(void) {
	guard(UD3D9RenderDevice::ScanForOldTextures);

	unsigned int u;
	FCachedTexture *pCT;

	//Prevent currently bound textures from being recycled
	for (u = 0; u < MAX_TMUNITS; u++) {
		FCachedTexture *pBind = TexInfo[u].pBind;
		if (pBind != NULL) {
			//Update last used frame count so that the texture will not be recycled
			pBind->LastUsedFrameCount = m_currentFrameCount;

			//Move node to tail of linked list if in LRU list
			if (pBind->bindType == BIND_TYPE_NON_ZERO_PREFIX_LRU_LIST) {
				m_nonZeroPrefixBindChain->unlink(pBind);
				m_nonZeroPrefixBindChain->link_to_tail(pBind);
			}
		}
	}

	pCT = m_nonZeroPrefixBindChain->begin();
	while (pCT != m_nonZeroPrefixBindChain->end()) {
		DWORD numFramesSinceUsed = m_currentFrameCount - pCT->LastUsedFrameCount;
		if (numFramesSinceUsed > DynamicTexIdRecycleLevel) {
			//See if the tex pool is not enabled, or the tex format is not RGBA8, or the texture has mipmaps
			if (!UseTexPool || (pCT->texFormat != D3DFMT_A8R8G8B8) || (pCT->texParams.filter & CT_HAS_MIPMAPS_BIT)) {
				//Remove node from linked list
				m_nonZeroPrefixBindChain->unlink(pCT);

				//Get pointer to node in bind map
				QWORD_CTTree_t::node_t *pNode = (QWORD_CTTree_t::node_t *)((BYTE *)pCT - (PTRINT)&(((QWORD_CTTree_t::node_t *)0)->data));
				//Extract tree index
				BYTE treeIndex = pCT->treeIndex;
				//Advanced cached texture pointer to next entry in linked list
				pCT = pCT->pNext;

				//Remove node from bind map
				m_nonZeroPrefixBindTrees[treeIndex].remove(pNode);

				//Delete the texture
				pNode->data.pTexObj->Release();
#if 0
{
	static unsigned int s_c;
	dbgPrintf("utd3d9r: Texture delete = %u\n", s_c++);
}
#endif

				continue;
			}
			else {
				TexPoolMap_t::node_t *texPoolPtr;

#if 0
{
	static unsigned int s_c;
	dbgPrintf("utd3d9r: TexPool free = %u, Id = 0x%08X, u = %u, v = %u\n",
		s_c++, (DWORD)pCT->pTexObj, pCT->UBits, pCT->VBits);
}
#endif

				//Remove node from linked list
				m_nonZeroPrefixBindChain->unlink(pCT);

				//Create a key from the lg2 width and height of the texture object
				TexPoolMapKey_t texPoolKey = MakeTexPoolMapKey(pCT->UBits, pCT->VBits);

				//Get pointer to node in bind map
				QWORD_CTTree_t::node_t *pNode = (QWORD_CTTree_t::node_t *)((BYTE *)pCT - (PTRINT)&(((QWORD_CTTree_t::node_t *)0)->data));
				//Extract tree index
				BYTE treeIndex = pCT->treeIndex;
				//Advanced cached texture pointer to next entry in linked list
				pCT = pCT->pNext;

				//Remove node from bind map
				m_nonZeroPrefixBindTrees[treeIndex].remove(pNode);

				//See if the key does not yet exist
				texPoolPtr = m_RGBA8TexPool->find(texPoolKey);
				//If the key does not yet exist, add an empty vector in its place
				if (texPoolPtr == 0) {
					texPoolPtr = m_TexPoolMap_Allocator.alloc_node();
					texPoolPtr->key = texPoolKey;
					texPoolPtr->data = QWORD_CTTree_NodePool_t();
					m_RGBA8TexPool->insert(texPoolPtr);
				}

				//Add node plus texture id to a list in the tex pool based on its dimensions
				texPoolPtr->data.add(pNode);

				continue;
			}
		}

		//The list is sorted
		//Stop searching on first one not to be recycled
		break;

		pCT = pCT->pNext;
	}

	unguard;
}

void UD3D9RenderDevice::SetNoTextureNoCheck(INT Multi) {
	STAT(STAT_TRACK_TEXTURELOCK);

	guard(UD3D9RenderDevice::SetNoTexture);

	// Set small white texture.
	//Set texture
	m_d3dDevice->SetTexture(Multi, m_pNoTexObj);

	//Set filter
	SetTexFilter(Multi, CT_MIN_FILTER_POINT | CT_MIP_FILTER_NONE);

	TexInfo[Multi].CurrentCacheID = TEX_CACHE_ID_NO_TEX;
	TexInfo[Multi].pBind = NULL;

	unguard;
}

void UD3D9RenderDevice::SetAlphaTextureNoCheck(INT Multi) {
	STAT(STAT_TRACK_TEXTURELOCK);

	guard(UD3D9RenderDevice::SetAlphaTexture);

	//Set texture
	m_d3dDevice->SetTexture(Multi, m_pAlphaTexObj);

	//Set filter
	SetTexFilter(Multi, CT_MIN_FILTER_LINEAR | CT_MIP_FILTER_NONE | CT_MAG_FILTER_LINEAR_NOT_POINT_BIT | CT_ADDRESS_U_CLAMP | CT_ADDRESS_V_CLAMP);

	TexInfo[Multi].CurrentCacheID = TEX_CACHE_ID_ALPHA_TEX;
	TexInfo[Multi].pBind = NULL;

	unguard;
}

//This function must use Tex.CurrentCacheID and NEVER use Info.CacheID to reference the texture cache id
//This makes it work with the masked texture hack code
void UD3D9RenderDevice::SetTextureNoCheck(DWORD texNum, FTexInfo& Tex, FTextureInfo& Info, DWORD PolyFlags) {
	STAT(STAT_TRACK_TEXTURELOCK);

	guard(UD3D9RenderDevice::SetTexture);

	// Make current.
	clockFast(BindCycles);

	FCachedTexture* pBind = nullptr;
	bool existingBind = BindTexture(texNum, Tex, Info, PolyFlags, pBind);

	//Save pointer to current texture bind for current texture unit
	Tex.pBind = pBind;

	//Set texture
	m_d3dDevice->SetTexture(texNum, pBind->pTexObj);

	unclockFast(BindCycles);

	// Account for all the impact on scale normalization.
	Tex.UMult = pBind->UMult;
	Tex.VMult = pBind->VMult;

	//Check for any changes to dynamic texture object parameters
	{
		BYTE desiredDynamicTexBits;

		desiredDynamicTexBits = (PolyFlags & PF_NoSmooth) ? DT_NO_SMOOTH_BIT : 0;
		if (desiredDynamicTexBits != pBind->dynamicTexBits) {
			BYTE dynamicTexBitsXor;

			dynamicTexBitsXor = desiredDynamicTexBits ^ pBind->dynamicTexBits;

			//Update dynamic tex bits early as there are no subsequent dependencies
			pBind->dynamicTexBits = desiredDynamicTexBits;

			if (dynamicTexBitsXor & DT_NO_SMOOTH_BIT) {
				BYTE desiredTexParamsFilter;

				//Set partial desired filter tex params
				desiredTexParamsFilter = 0;
				if (NoFiltering) {
					desiredTexParamsFilter |= CT_MIN_FILTER_POINT | CT_MIP_FILTER_NONE;
				}
				else if (PolyFlags & PF_NoSmooth) {
					desiredTexParamsFilter |= CT_MIN_FILTER_POINT;
					desiredTexParamsFilter |= ((pBind->texParams.filter & CT_HAS_MIPMAPS_BIT) == 0) ? CT_MIP_FILTER_NONE : CT_MIP_FILTER_POINT;
				}
				else {
					desiredTexParamsFilter |= (MaxAnisotropy) ? CT_MIN_FILTER_ANISOTROPIC : CT_MIN_FILTER_LINEAR;
					desiredTexParamsFilter |= ((pBind->texParams.filter & CT_HAS_MIPMAPS_BIT) == 0) ? CT_MIP_FILTER_NONE : (UseTrilinear ? CT_MIP_FILTER_LINEAR : CT_MIP_FILTER_POINT);
					desiredTexParamsFilter |= CT_MAG_FILTER_LINEAR_NOT_POINT_BIT;
				}

				//Store partial updated texture parameter state in cached texture object
				const BYTE MODIFIED_TEX_PARAMS_FILTER_BITS = CT_MIN_FILTER_MASK | CT_MIP_FILTER_MASK | CT_MAG_FILTER_LINEAR_NOT_POINT_BIT;
				pBind->texParams.filter = (pBind->texParams.filter & ~MODIFIED_TEX_PARAMS_FILTER_BITS) | desiredTexParamsFilter;
			}
		}
	}

	// Upload if needed.
	if (!existingBind || Info.bRealtimeChanged) {
		FColor paletteIndex0;

		// Cleanup texture flags.
		if (SupportsLazyTextures) {
			Info.Load();
		}
		Info.bRealtimeChanged = 0;

		//Set palette index 0 to black for masked paletted textures
		if (Info.Palette && (PolyFlags & PF_Masked)) {
			paletteIndex0 = Info.Palette[0];
			Info.Palette[0] = FColor(0,0,0,0);
		}

		// Download the texture.
		clockFast(ImageCycles);

		m_texConvertCtx.pBind = pBind;

		UBOOL SkipMipmaps = (Info.NumMips == 1);
		INT MaxLevel = pBind->MaxLevel;

		//Only calculate texture filter parameters for new textures
		if (!existingBind) {
			tex_params_t desiredTexParams;

			//Set desired filter tex params
			desiredTexParams.filter = 0;
			if (NoFiltering) {
				desiredTexParams.filter |= CT_MIN_FILTER_POINT | CT_MIP_FILTER_NONE;
			}
			else if (PolyFlags & PF_NoSmooth) {
				desiredTexParams.filter |= CT_MIN_FILTER_POINT;
				desiredTexParams.filter |= SkipMipmaps ? CT_MIP_FILTER_NONE : CT_MIP_FILTER_POINT;
			}
			else {
				desiredTexParams.filter |= (MaxAnisotropy) ? CT_MIN_FILTER_ANISOTROPIC : CT_MIN_FILTER_LINEAR;
				desiredTexParams.filter |= SkipMipmaps ? CT_MIP_FILTER_NONE : (UseTrilinear ? CT_MIP_FILTER_LINEAR : CT_MIP_FILTER_POINT);
				desiredTexParams.filter |= CT_MAG_FILTER_LINEAR_NOT_POINT_BIT;
			}

			if (!SkipMipmaps) {
				desiredTexParams.filter |= CT_HAS_MIPMAPS_BIT;
			}

#ifdef  UTGLR_UNREAL_227_BUILD
			// Check if clamped or wrapped (f.e. for Skyboxes). Smirftsch
			if (Info.UClampMode) {
				desiredTexParams.filter |= CT_ADDRESS_U_CLAMP;
			}

			if (Info.VClampMode) {
				desiredTexParams.filter |= CT_ADDRESS_V_CLAMP;
			}
#endif

			//Store updated texture parameter state in cached texture object
			pBind->texParams = desiredTexParams;
		}


		//Some textures only upload the base texture
		INT MaxUploadLevel = MaxLevel;
		if (SkipMipmaps) {
			MaxUploadLevel = 0;
		}


		//Set initial texture width and height in the context structure
		//Setup code must ensure that both UBits and VBits are greater than or equal to 0
		m_texConvertCtx.texWidthPow2 = 1 << pBind->UBits;
		m_texConvertCtx.texHeightPow2 = 1 << pBind->VBits;

		guard(WriteTexture);
		INT Level;
		for (Level = 0; Level <= MaxUploadLevel; Level++) {
			// Convert the mipmap.
			INT MipIndex = pBind->BaseMip + Level;
			INT stepBits = 0;
			if (MipIndex >= Info.NumMips) {
				stepBits = MipIndex - (Info.NumMips - 1);
				MipIndex = Info.NumMips - 1;
			}
			m_texConvertCtx.stepBits = stepBits;

			FMipmapBase* Mip = Info.Mips[MipIndex];
			if (!Mip || !Mip->DataPtr) {
				//Skip looking at any subsequent mipmap pointers
				break;
			}
			else {
				//Lock texture level
				if (FAILED(pBind->pTexObj->LockRect(Level, &m_texConvertCtx.lockRect, NULL, D3DLOCK_NOSYSLOCK))) {
					appErrorf(TEXT("Texture lock failed"));
				}

				//Texture data copy and potential conversion if necessary
				switch (pBind->texType) {
				case TEX_TYPE_COMPRESSED_DXT1:
					guard(ConvertDXT1_DXT1);
					ConvertDXT1_DXT1(Mip, Level);
					unguard;
					break;

				case TEX_TYPE_COMPRESSED_DXT1_TO_DXT3:
					guard(ConvertDXT1_DXT3);
					ConvertDXT1_DXT3(Mip, Level);
					unguard;
					break;

				case TEX_TYPE_COMPRESSED_DXT3:
					guard(ConvertDXT3);
					ConvertDXT35_DXT35(Mip, Level);
					unguard;
					break;

				case TEX_TYPE_COMPRESSED_DXT5:
					guard(ConvertDXT5);
					ConvertDXT35_DXT35(Mip, Level);
					unguard;
					break;

				case TEX_TYPE_PALETTED:
					//Not supported
					break;

				case TEX_TYPE_HAS_PALETTE:
					switch (pBind->texFormat) {
					case D3DFMT_R5G6B5:
						guard(ConvertP8_RGB565);
						if (stepBits == 0) {
							ConvertP8_RGB565_NoStep(Mip, Info.Palette, Level);
						}
						else {
							ConvertP8_RGB565(Mip, Info.Palette, Level);
						}
						unguard;
						break;

					case D3DFMT_X1R5G5B5:
					case D3DFMT_A1R5G5B5:
						guard(ConvertP8_RGBA5551);
						if (stepBits == 0) {
							ConvertP8_RGBA5551_NoStep(Mip, Info.Palette, Level);
						}
						else {
							ConvertP8_RGBA5551(Mip, Info.Palette, Level);
						}
						unguard;
						break;

					default:
						guard(ConvertP8_RGBA8888);
						if (stepBits == 0) {
							ConvertP8_RGBA8888_NoStep(Mip, Info.Palette, Level);
						}
						else {
							ConvertP8_RGBA8888(Mip, Info.Palette, Level);
						}
						unguard;
					}
					break;

				default:
					guard(ConvertBGRA7777);
					(this->*pBind->pConvertBGRA7777)(Mip, Level);
					unguard;
				}

				DWORD texWidth, texHeight;

				//Get current texture width and height
				texWidth = m_texConvertCtx.texWidthPow2;
				texHeight = m_texConvertCtx.texHeightPow2;

				//Calculate and save next texture width and height
				//Both are divided by two down to a floor of 1
				//Texture width and height must be even powers of 2 for the following code to work
				m_texConvertCtx.texWidthPow2 = (texWidth & 0x1) | (texWidth >> 1);
				m_texConvertCtx.texHeightPow2 = (texHeight & 0x1) | (texHeight >> 1);

				//Unlock texture level
				if (FAILED(pBind->pTexObj->UnlockRect(Level))) {
					appErrorf(TEXT("Texture unlock failed"));
				}
			}
		}
		unguard;

		unclockFast(ImageCycles);

		//Restore palette index 0 for masked paletted textures
		if (Info.Palette && (PolyFlags & PF_Masked)) {
			Info.Palette[0] = paletteIndex0;
		}

		// Cleanup.
		if (SupportsLazyTextures) {
			Info.Unload();
		}
	}

	//Set texture filter parameters
	SetTexFilter(texNum, pBind->texParams.filter);

	unguardf((TEXT("(Tex %ls Palette %p)"), Info.Texture->GetFullName(), Info.Palette));
}

bool UD3D9RenderDevice::BindTexture(DWORD texNum, FTexInfo& Tex, FTextureInfo& Info, DWORD PolyFlags, FCachedTexture*& Bind)
{
	const bool isZeroPrefixCacheID = ((Tex.CurrentCacheID & 0xFFFFFFFF00000000ULL) == 0) ? true : false;

	FCachedTexture* pBind = NULL;
	bool existingBind = false;
	HRESULT hResult;

	if (isZeroPrefixCacheID) {
		DWORD CacheIDSuffix = (Tex.CurrentCacheID & 0x00000000FFFFFFFFULL);

		DWORD_CTTree_t* zeroPrefixBindTree = &m_zeroPrefixBindTrees[CTZeroPrefixCacheIDSuffixToTreeIndex(CacheIDSuffix)];
		DWORD_CTTree_t::node_t* bindTreePtr = zeroPrefixBindTree->find(CacheIDSuffix);
		if (bindTreePtr)
		{
			pBind = &bindTreePtr->data;
			existingBind = true;
		}
		else
		{
			DWORD_CTTree_t::node_t* pNewNode;

			//Insert new texture info
			pNewNode = m_DWORD_CTTree_Allocator.alloc_node();
			pNewNode->key = CacheIDSuffix;
			zeroPrefixBindTree->insert(pNewNode);
			pBind = &pNewNode->data;
			pBind->TexInfoID = FCachedTexture::MakeInfoID(Info);

			//Set bind type
			pBind->bindType = BIND_TYPE_ZERO_PREFIX;

			//Set default tex params
			pBind->texParams = CT_DEFAULT_TEX_PARAMS;
			pBind->dynamicTexBits = (PolyFlags & PF_NoSmooth) ? DT_NO_SMOOTH_BIT : 0;

			//Cache texture info for the new texture
			CacheTextureInfo(pBind, Info, PolyFlags);

#if 0
			{
				static int si;
				dout << L"utd3d9r: Create texture zp = " << si++ << std::endl;
			}
#endif
			//Create the texture
			hResult = m_d3dDevice->CreateTexture(
				1U << pBind->UBits, 1U << pBind->VBits, (Info.NumMips == 1) ? 1 : (pBind->MaxLevel + 1),
				0, pBind->texFormat, D3DPOOL_MANAGED, &pBind->pTexObj, NULL);
			if (FAILED(hResult)) {
				if (UsePrecache)
					UsePrecache = 0;
				GWarn->Logf(TEXT("Ran out of texture memory. You can try the x64 version of Unreal 227 if your graphics card provides more than 4GB texture memory."));
				Flush(0);
				return 0;
				//appErrorf(TEXT("CreateTexture failed"));
			}

			//Allocate a new texture id
			AllocatedTextures++;
		}
	}
	else {
		DWORD CacheIDSuffix = (Tex.CurrentCacheID & 0x00000000FFFFFFFFULL);
		DWORD treeIndex = CTNonZeroPrefixCacheIDSuffixToTreeIndex(CacheIDSuffix);

		QWORD_CTTree_t* nonZeroPrefixBindTree = &m_nonZeroPrefixBindTrees[treeIndex];
		QWORD_CTTree_t::node_t* bindTreePtr = nonZeroPrefixBindTree->find(Tex.CurrentCacheID);
		if (bindTreePtr)
		{
			pBind = &bindTreePtr->data;
			pBind->LastUsedFrameCount = m_currentFrameCount;

			//Check if texture is in LRU list
			if (pBind->bindType == BIND_TYPE_NON_ZERO_PREFIX_LRU_LIST) {
				//Move node to tail of linked list
				m_nonZeroPrefixBindChain->unlink(pBind);
				m_nonZeroPrefixBindChain->link_to_tail(pBind);
			}

			existingBind = true;
		}
		else {
			QWORD_CTTree_t::node_t* pNewNode;

			//Allocate a new node
			//Use the node pool if it is not empty
			pNewNode = m_nonZeroPrefixNodePool.try_remove();
			if (!pNewNode) {
				pNewNode = m_QWORD_CTTree_Allocator.alloc_node();
			}

			//Insert new texture info
			pNewNode->key = Tex.CurrentCacheID;
			nonZeroPrefixBindTree->insert(pNewNode);
			pBind = &pNewNode->data;
			pBind->LastUsedFrameCount = m_currentFrameCount;
			pBind->TexInfoID = FCachedTexture::MakeInfoID(Info);

			//Set bind type
			pBind->bindType = BIND_TYPE_NON_ZERO_PREFIX_LRU_LIST;
			if (CacheStaticMaps && ((Tex.CurrentCacheID & 0xFF) == 0x18)) {
				pBind->bindType = BIND_TYPE_NON_ZERO_PREFIX;
			}

			//Save tree index
			pBind->treeIndex = (BYTE)treeIndex;

			//Set default tex params
			pBind->texParams = CT_DEFAULT_TEX_PARAMS;
			pBind->dynamicTexBits = (PolyFlags & PF_NoSmooth) ? DT_NO_SMOOTH_BIT : 0;

			//Check if texture should be in LRU list
			if (pBind->bindType == BIND_TYPE_NON_ZERO_PREFIX_LRU_LIST) {
				//Add node to linked list
				m_nonZeroPrefixBindChain->link_to_tail(pBind);
			}

			//Cache texture info for the new texture
			CacheTextureInfo(pBind, Info, PolyFlags);

			//See if the tex pool is enabled
			bool needTexIdAllocate = true;
			if (UseTexPool) {
				//See if the format will be RGBA8
				//Only textures without mipmaps are stored in the tex pool
				if ((pBind->texType == TEX_TYPE_NORMAL) && (Info.NumMips == 1)) {
					TexPoolMap_t::node_t* texPoolPtr;

					//Create a key from the lg2 width and height of the texture object
					TexPoolMapKey_t texPoolKey = MakeTexPoolMapKey(pBind->UBits, pBind->VBits);

					//Search for the key in the map
					texPoolPtr = m_RGBA8TexPool->find(texPoolKey);
					if (texPoolPtr != 0) {
						QWORD_CTTree_NodePool_t::node_t* texPoolNodePtr;

						//Get a reference to the pool of nodes with tex ids of the right dimension
						QWORD_CTTree_NodePool_t& texPool = texPoolPtr->data;

						//Attempt to get a texture id for the tex pool
						if ((texPoolNodePtr = texPool.try_remove()) != 0) {
							//Use texture id from node in tex pool
							pBind->pTexObj = texPoolNodePtr->data.pTexObj;

							//Use tex params from node in tex pool
							pBind->texParams = texPoolNodePtr->data.texParams;
							pBind->dynamicTexBits = texPoolNodePtr->data.dynamicTexBits;

							//Then add node to free list
							m_nonZeroPrefixNodePool.add(texPoolNodePtr);

#if 0
							{
								static int si;
								dout << L"utd3d9r: TexPool retrieve = " << si++ << L", Id = 0x" << HexString((DWORD)pBind->pTexObj, 32)
									<< L", u = " << pBind->UBits << L", v = " << pBind->VBits << std::endl;
							}
#endif

							//Clear the need tex id allocate flag
							needTexIdAllocate = false;
						}
					}
				}
			}
			if (needTexIdAllocate) {
#if 0
				{
					static int si;
					dout << L"utd3d9r: Create texture nzp = " << si++ << std::endl;
				}
#endif
				//Create the texture
				hResult = m_d3dDevice->CreateTexture(
					1U << pBind->UBits, 1U << pBind->VBits, (Info.NumMips == 1) ? 1 : (pBind->MaxLevel + 1),
					0, pBind->texFormat, D3DPOOL_MANAGED, &pBind->pTexObj, NULL);
				if (FAILED(hResult)) {
					appErrorf(TEXT("CreateTexture failed"));
				}

				//Allocate a new texture id
				AllocatedTextures++;
			}
		}
	}

	if (pBind && existingBind && (pBind->TexInfoID != FCachedTexture::MakeInfoID(Info)))
	{
		debugfSlow(TEXT("D3D9 Error: Found 2 different textures using same CacheID."));
		pBind->pTexObj->Release();
		if (pBind->bindType == BIND_TYPE_NON_ZERO_PREFIX_LRU_LIST)
			m_nonZeroPrefixBindChain->unlink(pBind);

		if (isZeroPrefixCacheID)
		{
			DWORD CacheIDSuffix = (Tex.CurrentCacheID & 0x00000000FFFFFFFFULL);
			DWORD_CTTree_t* zeroPrefixBindTree = &m_zeroPrefixBindTrees[CTZeroPrefixCacheIDSuffixToTreeIndex(CacheIDSuffix)];
			DWORD_CTTree_t::node_t* bindTreePtr = zeroPrefixBindTree->find(CacheIDSuffix);
			zeroPrefixBindTree->remove(bindTreePtr);
			m_DWORD_CTTree_Allocator.free_node(bindTreePtr);
		}
		else
		{
			DWORD CacheIDSuffix = (Tex.CurrentCacheID & 0x00000000FFFFFFFFULL);
			DWORD treeIndex = CTNonZeroPrefixCacheIDSuffixToTreeIndex(CacheIDSuffix);
			QWORD_CTTree_t* nonZeroPrefixBindTree = &m_nonZeroPrefixBindTrees[treeIndex];
			QWORD_CTTree_t::node_t* bindTreePtr = nonZeroPrefixBindTree->find(Tex.CurrentCacheID);
			nonZeroPrefixBindTree->remove(bindTreePtr);
			m_QWORD_CTTree_Allocator.free_node(bindTreePtr);
		}
		return BindTexture(texNum, Tex, Info, PolyFlags, Bind);
	}

	Bind = pBind;
	return existingBind;
}

static DWORD ModeList[] = { D3DCMP_LESS, D3DCMP_EQUAL, D3DCMP_LESSEQUAL, D3DCMP_GREATER, D3DCMP_GREATEREQUAL, D3DCMP_NOTEQUAL, D3DCMP_ALWAYS };
BYTE UD3D9RenderDevice::SetZTestMode(BYTE Mode)
{
	if (m_LastZMode == Mode || Mode >= ARRAY_COUNT(ModeList))
		return Mode;

	EndBuffering();

	m_d3dDevice->SetRenderState(D3DRS_ZFUNC, ModeList[Mode]);
	BYTE Prev = m_LastZMode;
	m_LastZMode = Mode;
	return Prev;
}

void UD3D9RenderDevice::CacheTextureInfo(FCachedTexture *pBind, const FTextureInfo &Info, DWORD PolyFlags) {
#if 0
{
	dbgPrintf("utd3d9r: CacheId = %08X:%08X\n",
		(DWORD)((QWORD)Info.CacheID >> 32), (DWORD)((QWORD)Info.CacheID & 0xFFFFFFFF));
}
{
	const UTexture *pTexture = Info.Texture;
	const TCHAR *pName = pTexture->GetFullName();
	if (pName) dbgPrintf("utd3d9r: TexName = %ls\n", appToAnsi(pName));
}
{
	dbgPrintf("utd3d9r: NumMips = %d\n", Info.NumMips);
}
{
	unsigned int u;
	TCHAR dbgStr[1024];
	TCHAR numStr[32];

	dbgStr[0] = _T('\0');
	appStrcat(dbgStr, TEXT("utd3d9r: ZPBindTree Size = "));
	for (u = 0; u < NUM_CTTree_TREES; u++) {
		appSprintf(numStr, TEXT("%u"), m_zeroPrefixBindTrees[u].calc_size());
		appStrcat(dbgStr, numStr);
		if (u != (NUM_CTTree_TREES - 1)) appStrcat(dbgStr, TEXT(", "));
	}
	dbgPrintf("%ls\n", appToAnsi(dbgStr));

	dbgStr[0] = _T('\0');
	appStrcat(dbgStr, TEXT("utd3d9r: NZPBindTree Size = "));
	for (u = 0; u < NUM_CTTree_TREES; u++) {
		appSprintf(numStr, TEXT("%u"), m_nonZeroPrefixBindTrees[u].calc_size());
		appStrcat(dbgStr, numStr);
		if (u != (NUM_CTTree_TREES - 1)) appStrcat(dbgStr, TEXT(", "));
	}
	dbgPrintf("%ls\n", appToAnsi(dbgStr));
}
#endif

	// Figure out scaling info for the texture.
	DWORD texFlags = 0;
	INT BaseMip = 0;
	INT MaxLevel;
	INT UBits = Info.Mips[0]->UBits;
	INT VBits = Info.Mips[0]->VBits;
	INT UCopyBits = 0;
	INT VCopyBits = 0;
	if ((UBits - VBits) > MaxLogUOverV) {
		VCopyBits += (UBits - VBits) - MaxLogUOverV;
		VBits = UBits - MaxLogUOverV;
	}
	if ((VBits - UBits) > MaxLogVOverU) {
		UCopyBits += (VBits - UBits) - MaxLogVOverU;
		UBits = VBits - MaxLogVOverU;
	}
	if (UBits < MinLogTextureSize) {
		UCopyBits += MinLogTextureSize - UBits;
		UBits += MinLogTextureSize - UBits;
	}
	if (VBits < MinLogTextureSize) {
		VCopyBits += MinLogTextureSize - VBits;
		VBits += MinLogTextureSize - VBits;
	}
	if (UBits > MaxLogTextureSize) {
		BaseMip += UBits - MaxLogTextureSize;
		VBits -= UBits - MaxLogTextureSize;
		UBits = MaxLogTextureSize;
		if (VBits < 0) {
			VCopyBits = -VBits;
			VBits = 0;
		}
	}
	if (VBits > MaxLogTextureSize) {
		BaseMip += VBits - MaxLogTextureSize;
		UBits -= VBits - MaxLogTextureSize;
		VBits = MaxLogTextureSize;
		if (UBits < 0) {
			UCopyBits = -UBits;
			UBits = 0;
		}
	}

	pBind->BaseMip = BaseMip;
	MaxLevel = Min(UBits, VBits) - MinLogTextureSize;
	if (MaxLevel < 0) {
		MaxLevel = 0;
	}
	pBind->MaxLevel = MaxLevel;
	pBind->UBits = UBits;
	pBind->VBits = VBits;

	pBind->UMult = 1.0f / (Info.UScale * (Info.USize << UCopyBits));
	pBind->VMult = 1.0f / (Info.VScale * (Info.VSize << VCopyBits));

	pBind->UClampVal = Info.UClamp - 1;
	pBind->VClampVal = Info.VClamp - 1;

	//Check for texture that does not require clamping
	//No clamp required if ((Info.UClamp == Info.USize) & (Info.VClamp == Info.VSize))
	if (((Info.UClamp ^ Info.USize) | (Info.VClamp ^ Info.VSize)) == 0) {
		texFlags |= TEX_FLAG_NO_CLAMP;
	}


	//Determine texture type
	//PolyFlags PF_Masked cannot change if existing texture is updated as it caches texture type information here
	pBind->texType = TEX_TYPE_NONE;
	if (SupportsTC) {
		switch (Info.Format) {
		case TEXF_DXT1:
			if (TexDXT1ToDXT3 && (!(PolyFlags & PF_Masked))) {
				pBind->texType = TEX_TYPE_COMPRESSED_DXT1_TO_DXT3;
				pBind->texFormat = D3DFMT_DXT3;
			}
			else {
				pBind->texType = TEX_TYPE_COMPRESSED_DXT1;
				pBind->texFormat = D3DFMT_DXT1;
			}
			break;

#ifdef UTGLR_UNREAL_227_BUILD
		case TEXF_DXT3:
			pBind->texType = TEX_TYPE_COMPRESSED_DXT3;
			pBind->texFormat = D3DFMT_DXT3;
			break;

		case TEXF_DXT5:
			pBind->texType = TEX_TYPE_COMPRESSED_DXT5;
			pBind->texFormat = D3DFMT_DXT5;
			break;
#endif

		default:
			;
		}
	}
	if (Info.Format == TEXF_RGB10A2_LM)
	{
		pBind->texType = TEX_TYPE_NORMAL_RGB10A2;
		pBind->pConvertBGRA7777 = &UD3D9RenderDevice::ConvertRGBA101002;
		pBind->texFormat = D3DFMT_A2R10G10B10;
	}
	else if (pBind->texType != TEX_TYPE_NONE) {
		//Using compressed texture
	}
	else if (Info.Palette) {
		pBind->texType = TEX_TYPE_HAS_PALETTE;
		pBind->texFormat = D3DFMT_A8R8G8B8;
		//Check if texture should be 16-bit
		if (PolyFlags & PF_Memorized) {
			pBind->texFormat = (PolyFlags & PF_Masked) ? D3DFMT_A1R5G5B5 : ((Use565Textures) ? D3DFMT_R5G6B5 : D3DFMT_X1R5G5B5);
		}
	}
	else
	{
		pBind->texType = TEX_TYPE_NORMAL;
		if (texFlags & TEX_FLAG_NO_CLAMP) {
			pBind->pConvertBGRA7777 = (Info.Format == TEXF_BGRA8) ? &UD3D9RenderDevice::ConvertBGRA8888_NoClamp : &UD3D9RenderDevice::ConvertBGRA7777_BGRA8888_NoClamp;
		}
		else {
			pBind->pConvertBGRA7777 = (Info.Format == TEXF_BGRA8) ? &UD3D9RenderDevice::ConvertBGRA8888 : &UD3D9RenderDevice::ConvertBGRA7777_BGRA8888;
		}
		pBind->texFormat = D3DFMT_A8R8G8B8;
	}

	return;
}


void UD3D9RenderDevice::ConvertDXT1_DXT1(const FMipmapBase *Mip, INT Level) {
	const DWORD *pSrc = (DWORD *)Mip->DataPtr;
	DWORD *pTex = (DWORD *)m_texConvertCtx.lockRect.pBits;
	DWORD UBlocks = 1U << Max(0, (INT)m_texConvertCtx.pBind->UBits - Level - 2);
	DWORD VBlocks = 1U << Max(0, (INT)m_texConvertCtx.pBind->VBits - Level - 2);

	for (DWORD v = 0; v < VBlocks; v++) {
		DWORD *pDest = pTex;
		for (DWORD u = 0; u < UBlocks; u++) {
			//Copy one block
			pDest[0] = pSrc[0];
			pDest[1] = pSrc[1];
			pSrc += 2;
			pDest += 2;
		}
		pTex = (DWORD *)((BYTE *)pTex + m_texConvertCtx.lockRect.Pitch);
	}

	UTGLR_DEBUG_TEX_CONVERT_COUNT(ConvertDXT1_DXT1);
}

void UD3D9RenderDevice::ConvertDXT1_DXT3(const FMipmapBase *Mip, INT Level) {
	const DWORD *pSrc = (DWORD *)Mip->DataPtr;
	DWORD *pTex = (DWORD *)m_texConvertCtx.lockRect.pBits;
	DWORD UBlocks = 1U << Max(0, (INT)m_texConvertCtx.pBind->UBits - Level - 2);
	DWORD VBlocks = 1U << Max(0, (INT)m_texConvertCtx.pBind->VBits - Level - 2);

	for (DWORD v = 0; v < VBlocks; v++) {
		DWORD *pDest = pTex;
		for (DWORD u = 0; u < UBlocks; u++) {
			//Copy one block
			pDest[0] = 0xFFFFFFFF;
			pDest[1] = 0xFFFFFFFF;
			pDest[2] = pSrc[0];
			pDest[3] = pSrc[1];
			pSrc += 2;
			pDest += 4;
		}
		pTex = (DWORD *)((BYTE *)pTex + m_texConvertCtx.lockRect.Pitch);
	}

	UTGLR_DEBUG_TEX_CONVERT_COUNT(ConvertDXT1_DXT3);
}

void UD3D9RenderDevice::ConvertDXT35_DXT35(const FMipmapBase *Mip, INT Level) {
	const DWORD *pSrc = (DWORD *)Mip->DataPtr;
	DWORD *pTex = (DWORD *)m_texConvertCtx.lockRect.pBits;
	DWORD UBlocks = 1U << Max(0, (INT)m_texConvertCtx.pBind->UBits - Level - 2);
	DWORD VBlocks = 1U << Max(0, (INT)m_texConvertCtx.pBind->VBits - Level - 2);

	for (DWORD v = 0; v < VBlocks; v++) {
		DWORD *pDest = pTex;
		for (DWORD u = 0; u < UBlocks; u++) {
			//Copy one block
			pDest[0] = pSrc[0];
			pDest[1] = pSrc[1];
			pDest[2] = pSrc[2];
			pDest[3] = pSrc[3];
			pSrc += 4;
			pDest += 4;
		}
		pTex = (DWORD *)((BYTE *)pTex + m_texConvertCtx.lockRect.Pitch);
	}

	UTGLR_DEBUG_TEX_CONVERT_COUNT(ConvertDXT35_DXT35);
}

void UD3D9RenderDevice::ConvertP8_RGBA8888(const FMipmapBase *Mip, const FColor *Palette, INT Level) {
	DWORD *pTex = (DWORD *)m_texConvertCtx.lockRect.pBits;
	INT StepBits = m_texConvertCtx.stepBits;
	DWORD UMask = Mip->USize - 1;
	DWORD VMask = Mip->VSize - 1;
	INT ij_inc = 1 << StepBits;
	INT i_stop = 1 << Max(0, (INT)m_texConvertCtx.pBind->VBits - Level + StepBits);
	INT j_stop = 1 << Max(0, (INT)m_texConvertCtx.pBind->UBits - Level + StepBits);
	INT i = 0;
	do { //i_stop always >= 1
		BYTE* Base = (BYTE*)Mip->DataPtr + (i & VMask) * Mip->USize;
		INT j = 0;
		do { //j_stop always >= 1
			DWORD dwColor = GET_COLOR_DWORD(Palette[Base[j & UMask]]);
			pTex[j] = (dwColor & 0xFF00FF00) | ((dwColor >> 16) & 0xFF) | ((dwColor << 16) & 0xFF0000);
		} while ((j += ij_inc) < j_stop);
		pTex = (DWORD *)((BYTE *)pTex + m_texConvertCtx.lockRect.Pitch);
	} while ((i += ij_inc) < i_stop);

	UTGLR_DEBUG_TEX_CONVERT_COUNT(ConvertP8_RGBA8888);
}

void UD3D9RenderDevice::ConvertP8_RGBA8888_NoStep(const FMipmapBase *Mip, const FColor *Palette, INT Level) {
	DWORD *pTex = (DWORD *)m_texConvertCtx.lockRect.pBits;
	DWORD UMask = Mip->USize - 1;
	DWORD VMask = Mip->VSize - 1;
	INT i_stop = m_texConvertCtx.texHeightPow2;
	INT j_stop = m_texConvertCtx.texWidthPow2;
	INT i = 0;
	do { //i_stop always >= 1
		BYTE* Base = (BYTE*)Mip->DataPtr + (i & VMask) * Mip->USize;
		INT j = 0;
		do { //j_stop always >= 1
			DWORD dwColor = GET_COLOR_DWORD(Palette[Base[j & UMask]]);
			pTex[j] = (dwColor & 0xFF00FF00) | ((dwColor >> 16) & 0xFF) | ((dwColor << 16) & 0xFF0000);
		} while ((j += 1) < j_stop);
		pTex = (DWORD *)((BYTE *)pTex + m_texConvertCtx.lockRect.Pitch);
	} while ((i += 1) < i_stop);

	UTGLR_DEBUG_TEX_CONVERT_COUNT(ConvertP8_RGBA8888_NoStep);
}

void UD3D9RenderDevice::ConvertP8_RGB565(const FMipmapBase *Mip, const FColor *Palette, INT Level) {
	_WORD *pTex = (_WORD *)m_texConvertCtx.lockRect.pBits;
	INT StepBits = m_texConvertCtx.stepBits;
	DWORD UMask = Mip->USize - 1;
	DWORD VMask = Mip->VSize - 1;
	INT ij_inc = 1 << StepBits;
	INT i_stop = 1 << Max(0, (INT)m_texConvertCtx.pBind->VBits - Level + StepBits);
	INT j_stop = 1 << Max(0, (INT)m_texConvertCtx.pBind->UBits - Level + StepBits);
	INT i = 0;
	do { //i_stop always >= 1
		BYTE* Base = (BYTE*)Mip->DataPtr + (i & VMask) * Mip->USize;
		INT j = 0;
		do { //j_stop always >= 1
			DWORD dwColor = GET_COLOR_DWORD(Palette[Base[j & UMask]]);
			pTex[j] = ((dwColor >> 19) & 0x001F) | ((dwColor >> 5) & 0x07E0) | ((dwColor << 8) & 0xF800);
		} while ((j += ij_inc) < j_stop);
		pTex = (WORD *)((BYTE *)pTex + m_texConvertCtx.lockRect.Pitch);
	} while ((i += ij_inc) < i_stop);

	UTGLR_DEBUG_TEX_CONVERT_COUNT(ConvertP8_RGB565);
}

void UD3D9RenderDevice::ConvertP8_RGB565_NoStep(const FMipmapBase *Mip, const FColor *Palette, INT Level) {
	_WORD *pTex = (_WORD *)m_texConvertCtx.lockRect.pBits;
	DWORD UMask = Mip->USize - 1;
	DWORD VMask = Mip->VSize - 1;
	INT i_stop = m_texConvertCtx.texHeightPow2;
	INT j_stop = m_texConvertCtx.texWidthPow2;
	INT i = 0;
	do { //i_stop always >= 1
		BYTE* Base = (BYTE*)Mip->DataPtr + (i & VMask) * Mip->USize;
		INT j = 0;
		do { //j_stop always >= 1
			DWORD dwColor = GET_COLOR_DWORD(Palette[Base[j & UMask]]);
			pTex[j] = ((dwColor >> 19) & 0x001F) | ((dwColor >> 5) & 0x07E0) | ((dwColor << 8) & 0xF800);
		} while ((j += 1) < j_stop);
		pTex = (_WORD *)((BYTE *)pTex + m_texConvertCtx.lockRect.Pitch);
	} while ((i += 1) < i_stop);

	UTGLR_DEBUG_TEX_CONVERT_COUNT(ConvertP8_RGB565_NoStep);
}

void UD3D9RenderDevice::ConvertP8_RGBA5551(const FMipmapBase *Mip, const FColor *Palette, INT Level) {
	_WORD *pTex = (_WORD *)m_texConvertCtx.lockRect.pBits;
	INT StepBits = m_texConvertCtx.stepBits;
	DWORD UMask = Mip->USize - 1;
	DWORD VMask = Mip->VSize - 1;
	INT ij_inc = 1 << StepBits;
	INT i_stop = 1 << Max(0, (INT)m_texConvertCtx.pBind->VBits - Level + StepBits);
	INT j_stop = 1 << Max(0, (INT)m_texConvertCtx.pBind->UBits - Level + StepBits);
	INT i = 0;
	do { //i_stop always >= 1
		BYTE* Base = (BYTE*)Mip->DataPtr + (i & VMask) * Mip->USize;
		INT j = 0;
		do { //j_stop always >= 1
			DWORD dwColor = GET_COLOR_DWORD(Palette[Base[j & UMask]]);
			pTex[j] = ((dwColor >> 19) & 0x001F) | ((dwColor >> 6) & 0x03E0) | ((dwColor << 7) & 0x7C00) | ((dwColor >> 16) & 0x8000);
		} while ((j += ij_inc) < j_stop);
		pTex = (WORD *)((BYTE *)pTex + m_texConvertCtx.lockRect.Pitch);
	} while ((i += ij_inc) < i_stop);

	UTGLR_DEBUG_TEX_CONVERT_COUNT(ConvertP8_RGBA5551);
}

void UD3D9RenderDevice::ConvertP8_RGBA5551_NoStep(const FMipmapBase *Mip, const FColor *Palette, INT Level) {
	_WORD *pTex = (_WORD *)m_texConvertCtx.lockRect.pBits;
	DWORD UMask = Mip->USize - 1;
	DWORD VMask = Mip->VSize - 1;
	INT i_stop = m_texConvertCtx.texHeightPow2;
	INT j_stop = m_texConvertCtx.texWidthPow2;
	INT i = 0;
	do { //i_stop always >= 1
		BYTE* Base = (BYTE*)Mip->DataPtr + (i & VMask) * Mip->USize;
		INT j = 0;
		do { //j_stop always >= 1
			DWORD dwColor = GET_COLOR_DWORD(Palette[Base[j & UMask]]);
			pTex[j] = ((dwColor >> 19) & 0x001F) | ((dwColor >> 6) & 0x03E0) | ((dwColor << 7) & 0x7C00) | ((dwColor >> 16) & 0x8000);
		} while ((j += 1) < j_stop);
		pTex = (_WORD *)((BYTE *)pTex + m_texConvertCtx.lockRect.Pitch);
	} while ((i += 1) < i_stop);

	UTGLR_DEBUG_TEX_CONVERT_COUNT(ConvertP8_RGBA5551_NoStep);
}

void UD3D9RenderDevice::ConvertRGBA101002(const FMipmapBase* Mip, INT Level) {
	DWORD* pTex = (DWORD*)m_texConvertCtx.lockRect.pBits;
	INT StepBits = m_texConvertCtx.stepBits;
	DWORD VMask = Mip->VSize - 1;
	DWORD VClampVal = m_texConvertCtx.pBind->VClampVal;
	DWORD UMask = Mip->USize - 1;
	DWORD UClampVal = m_texConvertCtx.pBind->UClampVal;
	INT ij_inc = 1 << StepBits;
	INT i_stop = 1 << Max(0, (INT)m_texConvertCtx.pBind->VBits - Level + StepBits);
	INT j_stop = 1 << Max(0, (INT)m_texConvertCtx.pBind->UBits - Level + StepBits);
	INT i = 0;
	do { //i_stop always >= 1
		FRedGreenBlue10Alpha2* Base = (FRedGreenBlue10Alpha2*)Mip->DataPtr + Min<DWORD>(i & VMask, VClampVal) * Mip->USize;
		INT j = 0;
		do { //j_stop always >= 1;
			DWORD dwColor = GET_COLOR_DWORD(Base[Min<DWORD>(j & UMask, UClampVal)]);
			//pTex[j] = ((dwColor & 0x000001FF) << 21) | ((dwColor & 0x0007FC00) >> 9) | ((dwColor & 0x1FF00000) >> 9) | (dwColor & 0xC0000000);
			pTex[j] = ((dwColor & 0x000001FF) << 21) | ((dwColor & 0x0007FC00) << 1) | ((dwColor & 0x1FF00000) >> 19) | (dwColor & 0xC0000000);
		} while ((j += ij_inc) < j_stop);
		pTex = (DWORD*)((BYTE*)pTex + m_texConvertCtx.lockRect.Pitch);
	} while ((i += ij_inc) < i_stop);

	UTGLR_DEBUG_TEX_CONVERT_COUNT(ConvertRGBA101002);
}

void UD3D9RenderDevice::ConvertBGRA7777_BGRA8888(const FMipmapBase *Mip, INT Level) {
	DWORD *pTex = (DWORD *)m_texConvertCtx.lockRect.pBits;
	INT StepBits = m_texConvertCtx.stepBits;
	DWORD VMask = Mip->VSize - 1;
	DWORD VClampVal = m_texConvertCtx.pBind->VClampVal;
	DWORD UMask = Mip->USize - 1;
	DWORD UClampVal = m_texConvertCtx.pBind->UClampVal;
	INT ij_inc = 1 << StepBits;
	INT i_stop = 1 << Max(0, (INT)m_texConvertCtx.pBind->VBits - Level + StepBits);
	INT j_stop = 1 << Max(0, (INT)m_texConvertCtx.pBind->UBits - Level + StepBits);
	INT i = 0;
	do { //i_stop always >= 1
		FColor* Base = (FColor*)Mip->DataPtr + Min<DWORD>(i & VMask, VClampVal) * Mip->USize;
		INT j = 0;
		do { //j_stop always >= 1;
			DWORD dwColor = GET_COLOR_DWORD(Base[Min<DWORD>(j & UMask, UClampVal)]);
			pTex[j] = dwColor << 1; // because of 7777
		} while ((j += ij_inc) < j_stop);
		pTex = (DWORD *)((BYTE *)pTex + m_texConvertCtx.lockRect.Pitch);
	} while ((i += ij_inc) < i_stop);

	UTGLR_DEBUG_TEX_CONVERT_COUNT(ConvertBGRA7777_BGRA8888);
}

void UD3D9RenderDevice::ConvertBGRA7777_BGRA8888_NoClamp(const FMipmapBase *Mip, INT Level) {
	DWORD *pTex = (DWORD *)m_texConvertCtx.lockRect.pBits;
	INT StepBits = m_texConvertCtx.stepBits;
	DWORD VMask = Mip->VSize - 1;
	DWORD UMask = Mip->USize - 1;
	INT ij_inc = 1 << StepBits;
	INT i_stop = 1 << Max(0, (INT)m_texConvertCtx.pBind->VBits - Level + StepBits);
	INT j_stop = 1 << Max(0, (INT)m_texConvertCtx.pBind->UBits - Level + StepBits);
	INT i = 0;
	do { //i_stop always >= 1
		FColor* Base = (FColor*)Mip->DataPtr + (DWORD)(i & VMask) * Mip->USize;
		INT j = 0;
		do { //j_stop always >= 1;
			DWORD dwColor = GET_COLOR_DWORD(Base[(DWORD)(j & UMask)]);
			pTex[j] = dwColor << 1; // because of 7777
		} while ((j += ij_inc) < j_stop);
		pTex = (DWORD *)((BYTE *)pTex + m_texConvertCtx.lockRect.Pitch);
	} while ((i += ij_inc) < i_stop);

	UTGLR_DEBUG_TEX_CONVERT_COUNT(ConvertBGRA7777_BGRA8888_NoClamp);
}

void UD3D9RenderDevice::ConvertBGRA8888(const FMipmapBase* Mip, INT Level) {
	DWORD* pTex = (DWORD*)m_texConvertCtx.lockRect.pBits;
	INT StepBits = m_texConvertCtx.stepBits;
	DWORD VMask = Mip->VSize - 1;
	DWORD VClampVal = m_texConvertCtx.pBind->VClampVal;
	DWORD UMask = Mip->USize - 1;
	DWORD UClampVal = m_texConvertCtx.pBind->UClampVal;
	INT ij_inc = 1 << StepBits;
	INT i_stop = 1 << Max(0, (INT)m_texConvertCtx.pBind->VBits - Level + StepBits);
	INT j_stop = 1 << Max(0, (INT)m_texConvertCtx.pBind->UBits - Level + StepBits);
	INT i = 0;
	do { //i_stop always >= 1
		FColor* Base = (FColor*)Mip->DataPtr + Min<DWORD>(i & VMask, VClampVal) * Mip->USize;
		INT j = 0;
		do { //j_stop always >= 1;
			pTex[j] = GET_COLOR_DWORD(Base[Min<DWORD>(j & UMask, UClampVal)]);
		} while ((j += ij_inc) < j_stop);
		pTex = (DWORD*)((BYTE*)pTex + m_texConvertCtx.lockRect.Pitch);
	} while ((i += ij_inc) < i_stop);

	UTGLR_DEBUG_TEX_CONVERT_COUNT(ConvertBGRA8888);
}

void UD3D9RenderDevice::ConvertBGRA8888_NoClamp(const FMipmapBase* Mip, INT Level) {
	DWORD* pTex = (DWORD*)m_texConvertCtx.lockRect.pBits;
	INT StepBits = m_texConvertCtx.stepBits;
	DWORD VMask = Mip->VSize - 1;
	DWORD UMask = Mip->USize - 1;
	INT ij_inc = 1 << StepBits;
	INT i_stop = 1 << Max(0, (INT)m_texConvertCtx.pBind->VBits - Level + StepBits);
	INT j_stop = 1 << Max(0, (INT)m_texConvertCtx.pBind->UBits - Level + StepBits);
	INT i = 0;
	do { //i_stop always >= 1
		FColor* Base = (FColor*)Mip->DataPtr + (DWORD)(i & VMask) * Mip->USize;
		INT j = 0;
		do { //j_stop always >= 1;
			pTex[j] = GET_COLOR_DWORD(Base[(DWORD)(j & UMask)]);
		} while ((j += ij_inc) < j_stop);
		pTex = (DWORD*)((BYTE*)pTex + m_texConvertCtx.lockRect.Pitch);
	} while ((i += ij_inc) < i_stop);

	UTGLR_DEBUG_TEX_CONVERT_COUNT(ConvertBGRA8888_NoClamp);
}

void UD3D9RenderDevice::EnableAlphaToCoverageNoCheck(void) {
	//Vendor specific enable alpha to coverage
	if (m_isATI) {
		m_d3dDevice->SetRenderState(D3DRS_POINTSIZE, MAKEFOURCC('A', '2', 'M', '1'));
	}
	else if (m_isNVIDIA) {
		m_d3dDevice->SetRenderState(D3DRS_ADAPTIVETESS_Y, (D3DFORMAT)MAKEFOURCC('A', 'T', 'O', 'C'));
	}

	return;
}

void UD3D9RenderDevice::DisableAlphaToCoverageNoCheck(void) {
	//Vendor specific disable alpha to coverage
	if (m_isATI) {
		m_d3dDevice->SetRenderState(D3DRS_POINTSIZE, MAKEFOURCC('A', '2', 'M', '0'));
	}
	else if (m_isNVIDIA) {
		m_d3dDevice->SetRenderState(D3DRS_ADAPTIVETESS_Y, D3DFMT_UNKNOWN);
	}

	return;
}

void UD3D9RenderDevice::SetBlendNoCheck(DWORD blendFlags) {
	guardSlow(UD3D9RenderDevice::SetBlendNoCheck);

	// Detect changes in the blending modes.
	DWORD Xor = m_curBlendFlags ^ blendFlags;

	//Update main copy of current blend flags early
	m_curBlendFlags = blendFlags;

#ifdef UTGLR_RUNE_BUILD
	constexpr DWORD GL_BLEND_FLAG_BITS = PF_Translucent | PF_Modulated | PF_Highlighted | PF_AlphaBlend;
#elif UTGLR_UNREAL_227_BUILD
	constexpr DWORD GL_BLEND_FLAG_BITS = PF_Translucent | PF_Modulated | PF_Highlighted | PF_AlphaBlend;
#else
	constexpr DWORD GL_BLEND_FLAG_BITS = PF_Translucent | PF_Modulated | PF_Highlighted;
#endif
	DWORD relevantBlendFlagBits = GL_BLEND_FLAG_BITS | m_smoothMaskedTexturesBit;
	if (Xor & (relevantBlendFlagBits))
	{
		if (!(blendFlags & (relevantBlendFlagBits))) {
			m_d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
			m_d3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
			m_d3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
		}
		else {
			if (blendFlags & PF_Translucent) {
				m_d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				m_d3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
				m_d3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCCOLOR);
			}
			else if (blendFlags & PF_Modulated) {
				m_d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				m_d3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_DESTCOLOR);
				m_d3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR);
			}
			else if (blendFlags & PF_Highlighted) {
				m_d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				m_d3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
				m_d3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			}
#ifdef UTGLR_RUNE_BUILD
			else if (blendFlags & PF_AlphaBlend) {
				m_d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				m_d3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
				m_d3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			}
#elif UTGLR_UNREAL_227_BUILD
			else if (blendFlags & PF_AlphaBlend) {
				m_d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				m_d3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
				m_d3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			}
#endif
			else if (blendFlags & PF_Masked) {
				m_d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				m_d3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
				m_d3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			}
		}
	}

	if (Xor & (PF_Masked | PF_AlphaBlend))
	{
		if (blendFlags & PF_AlphaBlend)
		{
			//Enable alpha test with alpha ref of D3D9 version of 0.01
			m_fsBlendInfo[0] = (2.0f / 255.0f) + KINDA_SMALL_NUMBER;

			if (UseFragmentProgram) {
				m_d3dDevice->SetPixelShaderConstantF(0, m_fsBlendInfo, 1);
			}
			else {
				m_alphaTestEnabled = true;
				m_d3dDevice->SetRenderState(D3DRS_ALPHAREF, 2);
				m_d3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
			}

			if (m_useAlphaToCoverageForMasked) {
				m_alphaToCoverageEnabled = false;
				DisableAlphaToCoverageNoCheck();
			}
		}
		else if (blendFlags & PF_Masked)
		{
			//Enable alpha test with alpha ref of D3D9 version of 0.5
			m_fsBlendInfo[0] = (127.0f / 255.0f) + KINDA_SMALL_NUMBER;

			if (UseFragmentProgram) {
				m_d3dDevice->SetPixelShaderConstantF(0, m_fsBlendInfo, 1);
			}
			else {
				m_alphaTestEnabled = true;
				m_d3dDevice->SetRenderState(D3DRS_ALPHAREF, 127);
				m_d3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
			}

			if (m_useAlphaToCoverageForMasked) {
				m_alphaToCoverageEnabled = true;
				EnableAlphaToCoverageNoCheck();
			}
		}
		else
		{
			//Disable alpha test
			m_fsBlendInfo[0] = 0.0f;

			if (UseFragmentProgram) {
				m_d3dDevice->SetPixelShaderConstantF(0, m_fsBlendInfo, 1);
			}
			else {
				m_alphaTestEnabled = false;
				m_d3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
			}

			if (m_useAlphaToCoverageForMasked) {
				m_alphaToCoverageEnabled = false;
				DisableAlphaToCoverageNoCheck();
			}
		}
	}
	if (Xor & PF_Invisible) {
		DWORD colorEnableBits = ((blendFlags & PF_Invisible) == 0) ? D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_RED : 0;
		m_d3dDevice->SetRenderState(D3DRS_COLORWRITEENABLE, colorEnableBits);
	}
	if (Xor & PF_Occlude) {
		DWORD flag = ((blendFlags & PF_Occlude) == 0) ? FALSE : TRUE;
		m_d3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, flag);
	}
	if (Xor & PF_RenderFog) {
		DWORD flag = ((blendFlags & PF_RenderFog) == 0) ? FALSE : TRUE;
		m_d3dDevice->SetRenderState(D3DRS_SPECULARENABLE, flag);
	}

	unguardSlow;
}

//This function will initialize or invalidate the texture environment state
//The current architecture allows both operations to be done in the same way
void UD3D9RenderDevice::InitOrInvalidateTexEnvState(void) {
	INT TMU;

	//For initialization, flags for all texture units are cleared
	//For initialization, first texture unit is modulated by default rather
	//than disabled, but priority bit encoding of flags will prevent problems
	//from the mismatch and will only result in one extra state update
	//For invalidation, flags for all texture units are also cleared as it is
	//fast enough and has no potential outside interaction side effects
	for (TMU = 0; TMU < MAX_TMUNITS; TMU++) {
		m_curTexEnvFlags[TMU] = 0;
	}

	//Set TexEnv 0 to modulated by default
	SetTexEnv(0, PF_Modulated);

	return;
}

void UD3D9RenderDevice::SetTexLODBiasState(INT TMUnits) {
	INT TMU;

	//Set texture LOD bias for all texture units
	for (TMU = 0; TMU < TMUnits; TMU++) {
		float fParam;

		//Set texture LOD bias
		fParam = LODBias;
		m_d3dDevice->SetSamplerState(TMU, D3DSAMP_MIPMAPLODBIAS, *(DWORD *)&fParam);
	}

	return;
}

void UD3D9RenderDevice::SetTexMaxAnisotropyState(INT TMUnits) {
	INT TMU;

	//Set maximum level of anisotropy for all texture units
	for (TMU = 0; TMU < TMUnits; TMU++) {
		m_d3dDevice->SetSamplerState(TMU, D3DSAMP_MAXANISOTROPY, MaxAnisotropy);
	}

	return;
}

inline DWORD F2DW(FLOAT f) { return *((DWORD*)&f); }

void UD3D9RenderDevice::SetTexEnvNoCheck(DWORD texUnit, DWORD texEnvFlags) {
	guardSlow(UD3D9RenderDevice::SetTexEnv);

	//Update current tex env flags early as there are no subsequent dependencies
	m_curTexEnvFlags[texUnit] = texEnvFlags;

	//Mark the texture unit as enabled
	m_texEnableBits |= 1U << texUnit;

	if (texEnvFlags & PF_Modulated) {
		D3DTEXTUREOP texOp;

		if ((texEnvFlags & PF_FlatShaded) || (texUnit != 0) && !OneXBlending) {
			texOp = D3DTOP_MODULATE2X;
		}
		else {
			texOp = D3DTOP_MODULATE;
		}

		m_d3dDevice->SetTextureStageState(texUnit, D3DTSS_COLOROP, texOp);
		m_d3dDevice->SetTextureStageState(texUnit, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

//		m_d3dDevice->SetTextureStageState(texUnit, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		m_d3dDevice->SetTextureStageState(texUnit, D3DTSS_COLORARG2, D3DTA_CURRENT);
	}
	else if (texEnvFlags & PF_Memorized) {
		m_d3dDevice->SetTextureStageState(texUnit, D3DTSS_COLOROP, D3DTOP_BLENDCURRENTALPHA);
		m_d3dDevice->SetTextureStageState(texUnit, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);

//		m_d3dDevice->SetTextureStageState(texUnit, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		m_d3dDevice->SetTextureStageState(texUnit, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	}
	else if (texEnvFlags & PF_Highlighted) {
		m_d3dDevice->SetTextureStageState(texUnit, D3DTSS_COLOROP, D3DTOP_MODULATEINVALPHA_ADDCOLOR);
		m_d3dDevice->SetTextureStageState(texUnit, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);

//		m_d3dDevice->SetTextureStageState(texUnit, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		m_d3dDevice->SetTextureStageState(texUnit, D3DTSS_COLORARG2, D3DTA_CURRENT);
	}
#if 0 // TODO...
	else if (texEnvFlags & PF_HeightMap) {
		m_d3dDevice->SetTextureStageState(texUnit, D3DTSS_COLOROP, D3DTOP_BUMPENVMAP);
		//m_d3dDevice->SetTextureStageState(texUnit, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		m_d3dDevice->SetTextureStageState(texUnit, D3DTSS_COLORARG2, D3DTA_CURRENT);

		// Set the bump mapping matrix.
		m_d3dDevice->SetTextureStageState(texUnit, D3DTSS_BUMPENVMAT00, F2DW(1.0f));
		m_d3dDevice->SetTextureStageState(texUnit, D3DTSS_BUMPENVMAT01, F2DW(0.0f));
		m_d3dDevice->SetTextureStageState(texUnit, D3DTSS_BUMPENVMAT10, F2DW(0.0f));
		m_d3dDevice->SetTextureStageState(texUnit, D3DTSS_BUMPENVMAT11, F2DW(1.0f));
	}
#endif

	unguardSlow;
}


void UD3D9RenderDevice::SetTexFilterNoCheck(DWORD texNum, BYTE texFilterParams) {
	guardSlow(UD3D9RenderDevice::SetTexFilter);

	BYTE texFilterParamsXor = m_curTexStageParams[texNum].filter ^ texFilterParams;

	//Update main copy of current tex filter params early
	m_curTexStageParams[texNum].filter = texFilterParams;

	if (texFilterParamsXor & CT_MIN_FILTER_MASK) {
		D3DTEXTUREFILTERTYPE texFilterType = D3DTEXF_POINT;

		switch (texFilterParams & CT_MIN_FILTER_MASK) {
		case CT_MIN_FILTER_POINT: texFilterType = D3DTEXF_POINT; break;
		case CT_MIN_FILTER_LINEAR: texFilterType = D3DTEXF_LINEAR; break;
		case CT_MIN_FILTER_ANISOTROPIC: texFilterType = D3DTEXF_ANISOTROPIC; break;
		default:
			;
		}

		m_d3dDevice->SetSamplerState(texNum, D3DSAMP_MINFILTER, texFilterType);
	}
	if (texFilterParamsXor & CT_MIP_FILTER_MASK) {
		D3DTEXTUREFILTERTYPE texFilterType = D3DTEXF_NONE;

		switch (texFilterParams & CT_MIP_FILTER_MASK) {
		case CT_MIP_FILTER_NONE: texFilterType = D3DTEXF_NONE; break;
		case CT_MIP_FILTER_POINT: texFilterType = D3DTEXF_POINT; break;
		case CT_MIP_FILTER_LINEAR: texFilterType = D3DTEXF_LINEAR; break;
		default:
			;
		}

		m_d3dDevice->SetSamplerState(texNum, D3DSAMP_MIPFILTER, texFilterType);
	}
	if (texFilterParamsXor & CT_MAG_FILTER_LINEAR_NOT_POINT_BIT) {
		m_d3dDevice->SetSamplerState(texNum, D3DSAMP_MAGFILTER, (texFilterParams & CT_MAG_FILTER_LINEAR_NOT_POINT_BIT) ? D3DTEXF_LINEAR : D3DTEXF_POINT);
	}
	if (texFilterParamsXor & CT_ADDRESS_U_CLAMP) {
		D3DTEXTUREADDRESS texAddressMode = (texFilterParams & CT_ADDRESS_U_CLAMP) ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP;
		m_d3dDevice->SetSamplerState(texNum, D3DSAMP_ADDRESSU, texAddressMode);
	}
	if (texFilterParamsXor & CT_ADDRESS_V_CLAMP) {
		D3DTEXTUREADDRESS texAddressMode = (texFilterParams & CT_ADDRESS_V_CLAMP) ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP;
		m_d3dDevice->SetSamplerState(texNum, D3DSAMP_ADDRESSV, texAddressMode);
	}

	unguardSlow;
}


void UD3D9RenderDevice::SetVertexDeclNoCheck(IDirect3DVertexDeclaration9 *vertexDecl) {
	HRESULT hResult;

	//Set vertex declaration
	hResult = m_d3dDevice->SetVertexDeclaration(vertexDecl);
	if (FAILED(hResult)) {
		appErrorf(TEXT("SetVertexDeclaration failed"));
	}

	//Save new current vertex declaration
	m_curVertexDecl = vertexDecl;

	return;
}

void UD3D9RenderDevice::SetVertexShaderNoCheck(IDirect3DVertexShader9 *vertexShader) {
	HRESULT hResult;

	//Set vertex shader
	hResult = m_d3dDevice->SetVertexShader(vertexShader);
	if (FAILED(hResult)) {
		appErrorf(TEXT("SetVertexShader failed"));
	}

	m_vpSwitchCount++;
	if ((vertexShader != NULL) && (m_curVertexShader == NULL)) m_vpEnableCount++;

	//Save new current vertex shader
	m_curVertexShader = vertexShader;

	return;
}

void UD3D9RenderDevice::SetPixelShaderNoCheck(IDirect3DPixelShader9 *pixelShader) {
	HRESULT hResult;

	//Set pixel shader
	hResult = m_d3dDevice->SetPixelShader(pixelShader);
	if (FAILED(hResult)) {
		appErrorf(TEXT("SetPixelShader failed"));
	}

	m_fpSwitchCount++;
	if ((pixelShader != NULL) && (m_curPixelShader == NULL)) m_fpEnableCount++;

	//Save new current pixel shader
	m_curPixelShader = pixelShader;

	return;
}


void UD3D9RenderDevice::SetAAStateNoCheck(bool AAEnable) {
	//Save new AA state
	m_curAAEnable = AAEnable;

	m_AASwitchCount++;

	//Set new AA state
	m_d3dDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, (AAEnable) ? TRUE : FALSE);

	return;
}


bool UD3D9RenderDevice::LoadVertexProgram(IDirect3DVertexShader9 **ppShader, const DWORD *pFunction, const char *pName) {
	HRESULT hResult;

	if (DebugBit(DEBUG_BIT_BASIC)) {
		dbgPrintf("utd3d9r: Loading vertex program \"%ls\"\n", pName);
	}

	hResult = m_d3dDevice->CreateVertexShader(pFunction, ppShader);
	if (FAILED(hResult)) {
		if (DebugBit(DEBUG_BIT_BASIC)) {
			dbgPrintf("utd3d9r: Vertex program load error\n");
		}

		return false;
	}

	return true;
}

bool UD3D9RenderDevice::LoadFragmentProgram(IDirect3DPixelShader9 **ppShader, const DWORD *pFunction, const char *pName) {
	HRESULT hResult;

	if (DebugBit(DEBUG_BIT_BASIC)) {
		dbgPrintf("utd3d9r: Loading fragment program \"%ls\"\n", pName);
	}

	hResult = m_d3dDevice->CreatePixelShader(pFunction, ppShader);
	if (FAILED(hResult)) {
		if (DebugBit(DEBUG_BIT_BASIC)) {
			dbgPrintf("utd3d9r: Fragment program load error\n");
		}

		return false;
	}

	return true;
}


bool UD3D9RenderDevice::InitializeFragmentPrograms(void) {
	bool initOk = true;


	//Create vertex programs if not already created
	#define UTGLR_VS_CONDITIONAL_LOAD(_var, _vp, _name) \
		if (!_var) { \
			initOk &= LoadVertexProgram(&_var, _vp, _name); \
		}


	//Default rendering state
	UTGLR_VS_CONDITIONAL_LOAD(m_vpDefaultRenderingState, g_vpDefaultRenderingState,
		"Default rendering state");

	//Default rendering state with fog
	UTGLR_VS_CONDITIONAL_LOAD(m_vpDefaultRenderingStateWithFog, g_vpDefaultRenderingStateWithFog,
		"Default rendering state with fog");

	//Default rendering state with linear fog
	UTGLR_VS_CONDITIONAL_LOAD(m_vpDefaultRenderingStateWithLinearFog, g_vpDefaultRenderingStateWithLinearFog,
		"Default rendering state with linear fog");

	//Complex surface single texture
	UTGLR_VS_CONDITIONAL_LOAD(m_vpComplexSurface[0], g_vpComplexSurfaceSingleTexture,
		"Complex surface single texture");

	if (TMUnits >= 2) {
		//Complex surface dual texture
		UTGLR_VS_CONDITIONAL_LOAD(m_vpComplexSurface[1], g_vpComplexSurfaceDualTexture,
			"Complex surface dual texture");
	}

	if (TMUnits >= 3) {
		//Complex surface triple texture
		UTGLR_VS_CONDITIONAL_LOAD(m_vpComplexSurface[2], g_vpComplexSurfaceTripleTexture,
			"Complex surface triple texture");
	}

	if (TMUnits >= 4) {
		//Complex surface quad texture
		UTGLR_VS_CONDITIONAL_LOAD(m_vpComplexSurface[3], g_vpComplexSurfaceQuadTexture,
			"Complex surface quad texture");
	}


	if (DetailTextures) {
		//Detail texture
		UTGLR_VS_CONDITIONAL_LOAD(m_vpDetailTexture, g_vpDetailTexture,
			"Detail texture");

		//Complex surface single texture and detail texture
		UTGLR_VS_CONDITIONAL_LOAD(m_vpComplexSurfaceSingleTextureAndDetailTexture, g_vpComplexSurfaceSingleTextureAndDetailTexture,
			"Complex surface single texture and detail texture");

		//Complex surface dual texture and detail texture
		UTGLR_VS_CONDITIONAL_LOAD(m_vpComplexSurfaceDualTextureAndDetailTexture, g_vpComplexSurfaceDualTextureAndDetailTexture,
			"Complex surface dual texture and detail texture");
	}


	#undef UTGLR_VS_CONDITIONAL_LOAD


	//Create fragment programs if not already created
	#define UTGLR_PS_CONDITIONAL_LOAD(_var, _fp, _name) \
		if (!_var) { \
			initOk &= LoadFragmentProgram(&_var, _fp, _name); \
		}


	//Default rendering state
	UTGLR_PS_CONDITIONAL_LOAD(m_fpDefaultRenderingState, g_fpDefaultRenderingState,
		"Default rendering state");

	//Default rendering state with fog
	UTGLR_PS_CONDITIONAL_LOAD(m_fpDefaultRenderingStateWithFog, g_fpDefaultRenderingStateWithFog,
		"Default rendering state with fog");

	//Default rendering state with linear fog
	UTGLR_PS_CONDITIONAL_LOAD(m_fpDefaultRenderingStateWithLinearFog, g_fpDefaultRenderingStateWithLinearFog,
		"Default rendering state with linear fog");


	//Complex surface single texture
	UTGLR_PS_CONDITIONAL_LOAD(m_fpComplexSurfaceSingleTexture, g_fpComplexSurfaceSingleTexture,
		"Complex surface single texture");

	//Complex surface dual texture modulated
	UTGLR_PS_CONDITIONAL_LOAD(m_fpComplexSurfaceDualTextureModulated, g_fpComplexSurfaceDualTextureModulated,
		"Complex surface dual texture modulated");

	//Complex surface triple texture modulated
	UTGLR_PS_CONDITIONAL_LOAD(m_fpComplexSurfaceTripleTextureModulated, g_fpComplexSurfaceTripleTextureModulated,
		"Complex surface triple texture modulated");


	//Complex surface single texture with fog
	UTGLR_PS_CONDITIONAL_LOAD(m_fpComplexSurfaceSingleTextureWithFog, g_fpComplexSurfaceSingleTextureWithFog,
		"Complex surface single texture with fog");

	//Complex surface dual texture modulated with fog
	UTGLR_PS_CONDITIONAL_LOAD(m_fpComplexSurfaceDualTextureModulatedWithFog, g_fpComplexSurfaceDualTextureModulatedWithFog,
		"Complex surface dual texture modulated with fog");

	//Complex surface triple texture modulated with fog
	UTGLR_PS_CONDITIONAL_LOAD(m_fpComplexSurfaceTripleTextureModulatedWithFog, g_fpComplexSurfaceTripleTextureModulatedWithFog,
		"Complex surface triple texture modulated with fog");


	if (DetailTextures) {
		//Detail texture
		UTGLR_PS_CONDITIONAL_LOAD(m_fpDetailTexture, g_fpDetailTexture,
			"Detail texture");

		//Detail texture two layer
		UTGLR_PS_CONDITIONAL_LOAD(m_fpDetailTextureTwoLayer, g_fpDetailTextureTwoLayer,
			"Detail texture two layer");

		//Single texture and detail texture
		UTGLR_PS_CONDITIONAL_LOAD(m_fpSingleTextureAndDetailTexture, g_fpSingleTextureAndDetailTexture,
			"Complex surface single texture and detail texture");

		//Single texture and detail texture two layer
		UTGLR_PS_CONDITIONAL_LOAD(m_fpSingleTextureAndDetailTextureTwoLayer, g_fpSingleTextureAndDetailTextureTwoLayer,
			"Complex surface single texture and detail texture two layer");

		//Dual texture and detail texture
		UTGLR_PS_CONDITIONAL_LOAD(m_fpDualTextureAndDetailTexture, g_fpDualTextureAndDetailTexture,
			"Complex surface dual texture and detail texture");

		//Dual texture and detail texture two layer
		UTGLR_PS_CONDITIONAL_LOAD(m_fpDualTextureAndDetailTextureTwoLayer, g_fpDualTextureAndDetailTextureTwoLayer,
			"Complex surface dual texture and detail texture two layer");
	}


	#undef UTGLR_PS_CONDITIONAL_LOAD

	return initOk;
}

//Attempts to initializes fragment program mode
//Safe to call multiple times as already created fragment programs will not be recreated
void UD3D9RenderDevice::TryInitializeFragmentProgramMode(void) {
	//Initialize fragment programs
	if (!InitializeFragmentPrograms()) {
		//Shutdown fragment program mode
		ShutdownFragmentProgramMode();

		//Disable fragment program mode
		DCV.UseFragmentProgram = 0;
		UseFragmentProgram = 0;
		PL_UseFragmentProgram = 0;

		if (DebugBit(DEBUG_BIT_BASIC)) dbgPrintf("utd3d9r: Fragment program initialization failed\n");
	}

	return;
}

//Shuts down fragment program mode if it is active
//Freeing the fragment program names takes care of releasing resources
//Safe to call even if fragment program mode is not supported or was never initialized
void UD3D9RenderDevice::ShutdownFragmentProgramMode(void) {
	//Make sure that a vertex program is not current
	SetVertexShaderNoCheck(NULL);

	//Make sure that a fragment program is not current
	SetPixelShaderNoCheck(NULL);


	#define UTGLR_VS_RELEASE(_var) \
		if (_var) { \
			_var->Release(); \
			_var = NULL; \
		}

	//Free vertex programs if they were created

	//Default rendering state
	UTGLR_VS_RELEASE(m_vpDefaultRenderingState);

	//Default rendering state with fog
	UTGLR_VS_RELEASE(m_vpDefaultRenderingStateWithFog);

	//Default rendering state with linear fog
	UTGLR_VS_RELEASE(m_vpDefaultRenderingStateWithLinearFog);


	//Complex surface single texture
	UTGLR_VS_RELEASE(m_vpComplexSurface[0]);

	//Complex surface double texture
	UTGLR_VS_RELEASE(m_vpComplexSurface[1]);

	//Complex surface triple texture
	UTGLR_VS_RELEASE(m_vpComplexSurface[2]);

	//Complex surface quad texture
	UTGLR_VS_RELEASE(m_vpComplexSurface[3]);


	//Detail texture
	UTGLR_VS_RELEASE(m_vpDetailTexture);

	//Complex surface single texture and detail texture
	UTGLR_VS_RELEASE(m_vpComplexSurfaceSingleTextureAndDetailTexture);

	//Complex surface dual texture and detail texture
	UTGLR_VS_RELEASE(m_vpComplexSurfaceDualTextureAndDetailTexture);


	#undef UTGLR_VS_RELEASE


	#define UTGLR_PS_RELEASE(_var) \
		if (_var) { \
			_var->Release(); \
			_var = NULL; \
		}

	//Free fragment programs if they were created

	//Default rendering state
	UTGLR_PS_RELEASE(m_fpDefaultRenderingState);

	//Default rendering state with fog
	UTGLR_PS_RELEASE(m_fpDefaultRenderingStateWithFog);

	//Default rendering state with linear fog
	UTGLR_PS_RELEASE(m_fpDefaultRenderingStateWithLinearFog);


	//Complex surface single texture
	UTGLR_PS_RELEASE(m_fpComplexSurfaceSingleTexture);

	//Complex surface dual texture modulated
	UTGLR_PS_RELEASE(m_fpComplexSurfaceDualTextureModulated);

	//Complex surface triple texture modulated
	UTGLR_PS_RELEASE(m_fpComplexSurfaceTripleTextureModulated);


	//Complex surface single texture with fog
	UTGLR_PS_RELEASE(m_fpComplexSurfaceSingleTextureWithFog);

	//Complex surface dual texture modulated with fog
	UTGLR_PS_RELEASE(m_fpComplexSurfaceDualTextureModulatedWithFog);

	//Complex surface triple texture modulated with fog
	UTGLR_PS_RELEASE(m_fpComplexSurfaceTripleTextureModulatedWithFog);


	//Detail texture
	UTGLR_PS_RELEASE(m_fpDetailTexture);

	//Detail texture two layer
	UTGLR_PS_RELEASE(m_fpDetailTextureTwoLayer);

	//Single texture and detail texture
	UTGLR_PS_RELEASE(m_fpSingleTextureAndDetailTexture);

	//Single texture and detail texture two layer
	UTGLR_PS_RELEASE(m_fpSingleTextureAndDetailTextureTwoLayer);

	//Dual texture and detail texture
	UTGLR_PS_RELEASE(m_fpDualTextureAndDetailTexture);

	//Dual texture and detail texture two layer
	UTGLR_PS_RELEASE(m_fpDualTextureAndDetailTextureTwoLayer);


	#undef UTGLR_PS_RELEASE

	return;
}


void UD3D9RenderDevice::SetProjectionStateNoCheck(bool requestNearZRangeHackProjection) {
	float left, right, bottom, top, zNear, zFar;
	float invRightMinusLeft, invTopMinusBottom, invNearMinusFar;
	D3DMATRIX d3dProj;

	//Save new Z range hack projection state
	m_nearZRangeHackProjectionActive = requestNearZRangeHackProjection;

	//Set default zNearVal
	FLOAT zNearVal = 0.5f;

	FLOAT zScaleVal = 1.0f;
	if (requestNearZRangeHackProjection) {
#ifdef UTGLR_DEBUG_Z_RANGE_HACK_WIREFRAME
		m_d3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
#endif

		zScaleVal = 0.125f;
		zNearVal = 0.5;
	}
	else {
#ifdef UTGLR_DEBUG_Z_RANGE_HACK_WIREFRAME
		m_d3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
#endif

		if (m_useZRangeHack) {
			zNearVal = 4.0f;
		}
	}

	left = -m_RProjZ * zNearVal;
	right = +m_RProjZ * zNearVal;
	bottom = -m_Aspect*m_RProjZ * zNearVal;
	top = +m_Aspect*m_RProjZ * zNearVal;
	zNear = 1.0f * zNearVal;

	//Set zFar
#ifdef UTGLR_UNREAL_227_BUILD
	zFar = 65536.0f; // Sunspire fix
#else
	zFar = 32768.0f;
#endif
	if (requestNearZRangeHackProjection) {
		zFar *= zScaleVal;
	}

	invRightMinusLeft = 1.0f / (right - left);
	invTopMinusBottom = 1.0f / (top - bottom);
	invNearMinusFar = 1.0f / (zNear - zFar);

	d3dProj.m[0][0] = 2.0f * zNear * invRightMinusLeft;
	d3dProj.m[0][1] = 0.0f;
	d3dProj.m[0][2] = 0.0f;
	d3dProj.m[0][3] = 0.0f;

	d3dProj.m[1][0] = 0.0f;
	d3dProj.m[1][1] = 2.0f * zNear * invTopMinusBottom;
	d3dProj.m[1][2] = 0.0f;
	d3dProj.m[1][3] = 0.0f;

	d3dProj.m[2][0] = 1.0f / (FLOAT)m_sceneNodeX;
	d3dProj.m[2][1] = -1.0f / (FLOAT)m_sceneNodeY;
	d3dProj.m[2][2] = zScaleVal * (zFar * invNearMinusFar);
	d3dProj.m[2][3] = -1.0f;

	d3dProj.m[3][0] = 0.0f;
	d3dProj.m[3][1] = 0.0f;
	d3dProj.m[3][2] = zScaleVal * zScaleVal * (zNear * zFar * invNearMinusFar);
	d3dProj.m[3][3] = 0.0f;

	m_d3dDevice->SetTransform(D3DTS_PROJECTION, &d3dProj);

	if (UseFragmentProgram) {
		FLOAT vsTransMatrix[16];

		//Transpose and scale by -1y and -1z
		vsTransMatrix[0]  = d3dProj.m[0][0];
		vsTransMatrix[1]  = -d3dProj.m[1][0];
		vsTransMatrix[2]  = -d3dProj.m[2][0];
		vsTransMatrix[3]  = d3dProj.m[3][0];
		vsTransMatrix[4]  = d3dProj.m[0][1];
		vsTransMatrix[5]  = -d3dProj.m[1][1];
		vsTransMatrix[6]  = -d3dProj.m[2][1];
		vsTransMatrix[7]  = d3dProj.m[3][1];
		vsTransMatrix[8]  = d3dProj.m[0][2];
		vsTransMatrix[9]  = -d3dProj.m[1][2];
		vsTransMatrix[10] = -d3dProj.m[2][2];
		vsTransMatrix[11] = d3dProj.m[3][2];
		vsTransMatrix[12] = d3dProj.m[0][3];
		vsTransMatrix[13] = -d3dProj.m[1][3];
		vsTransMatrix[14] = -d3dProj.m[2][3];
		vsTransMatrix[15] = d3dProj.m[3][3];

		m_d3dDevice->SetVertexShaderConstantF(0, vsTransMatrix, 4);
	}

	return;
}

void UD3D9RenderDevice::SetOrthoProjection(void) {
	float left, right, bottom, top, zNear, zFar;
	float invNearMinusFar;
	D3DMATRIX d3dProj;

	//Save new Z range hack projection state
	m_nearZRangeHackProjectionActive = false;

	left = -m_RProjZ * 0.5f;
	right = +m_RProjZ * 0.5f;
	bottom = -m_Aspect*m_RProjZ * 0.5f;
	top = +m_Aspect*m_RProjZ * 0.5f;
	zNear = 1.0f * 0.5f;
	zFar = 32768.0f;

	invNearMinusFar = 1.0f / (zNear - zFar);

	d3dProj.m[0][0] = 1.0f / (right - left);
	d3dProj.m[0][1] = 0.0f;
	d3dProj.m[0][2] = 0.0f;
	d3dProj.m[0][3] = 0.0f;

	d3dProj.m[1][0] = 0.0f;
	d3dProj.m[1][1] = 1.0f / (top - bottom);
	d3dProj.m[1][2] = 0.0f;
	d3dProj.m[1][3] = 0.0f;

	d3dProj.m[2][0] = 0.0f;
	d3dProj.m[2][1] = 0.0f;
	d3dProj.m[2][2] = invNearMinusFar;
	d3dProj.m[2][3] = 0.0f;

	d3dProj.m[3][0] = -1.0f / (FLOAT)m_sceneNodeX;
	d3dProj.m[3][1] = 1.0f / (FLOAT)m_sceneNodeY;
	d3dProj.m[3][2] = zNear * invNearMinusFar;
	d3dProj.m[3][3] = 1.0f;

	m_d3dDevice->SetTransform(D3DTS_PROJECTION, &d3dProj);

	if (UseFragmentProgram) {
		FLOAT vsTransMatrix[16];

		//Transpose and scale by -1y and -1z
		vsTransMatrix[0]  = d3dProj.m[0][0];
		vsTransMatrix[1]  = -d3dProj.m[1][0];
		vsTransMatrix[2]  = -d3dProj.m[2][0];
		vsTransMatrix[3]  = d3dProj.m[3][0];
		vsTransMatrix[4]  = d3dProj.m[0][1];
		vsTransMatrix[5]  = -d3dProj.m[1][1];
		vsTransMatrix[6]  = -d3dProj.m[2][1];
		vsTransMatrix[7]  = d3dProj.m[3][1];
		vsTransMatrix[8]  = d3dProj.m[0][2];
		vsTransMatrix[9]  = -d3dProj.m[1][2];
		vsTransMatrix[10] = -d3dProj.m[2][2];
		vsTransMatrix[11] = d3dProj.m[3][2];
		vsTransMatrix[12] = d3dProj.m[0][3];
		vsTransMatrix[13] = -d3dProj.m[1][3];
		vsTransMatrix[14] = -d3dProj.m[2][3];
		vsTransMatrix[15] = d3dProj.m[3][3];

		m_d3dDevice->SetVertexShaderConstantF(0, vsTransMatrix, 4);
	}

	return;
}


void UD3D9RenderDevice::RenderPassesExec(void) {
	guard(UD3D9RenderDevice::RenderPassesExec);

	//Some render passes paths may use fragment program

	if (m_rpMasked && m_rpForceSingle && !m_rpSetDepthEqual) {
		SetZTestMode(ZTEST_Equal);
		m_rpSetDepthEqual = true;
	}

	//Call the render passes no check setup proc
	(this->*m_pRenderPassesNoCheckSetupProc)();

	m_rpTMUnits = 1;
	m_rpForceSingle = true;


	for (INT PolyNum = 0; PolyNum < m_csPolyCount; PolyNum++) {
		m_d3dDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, m_curVertexBufferPos + MultiDrawFirstArray[PolyNum], MultiDrawCountArray[PolyNum] - 2);
	}

#ifdef UTGLR_DEBUG_WORLD_WIREFRAME
	m_d3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

	SetBlend(PF_Modulated);

	for (PolyNum = 0; PolyNum < m_csPolyCount; PolyNum++) {
		m_d3dDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, m_curVertexBufferPos + MultiDrawFirstArray[PolyNum], MultiDrawCountArray[PolyNum] - 2);
	}

	m_d3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
#endif

	//Advance vertex buffer position
	m_curVertexBufferPos += m_csPtCount;

#if 0
{
	dbgPrintf("utd3d9r: PassCount = %d\n", m_rpPassCount);
}
#endif
	m_rpPassCount = 0;


	unguard;
}

void UD3D9RenderDevice::RenderPassesExec_SingleOrDualTextureAndDetailTexture(FTextureInfo &DetailTextureInfo) {
	guard(UD3D9RenderDevice::RenderPassesExec_SingleOrDualTextureAndDetailTexture);

	//Some render passes paths may use fragment program

	//The dual texture and detail texture path can never be executed if single pass rendering were forced earlier
	//The depth function will never need to be changed due to single pass rendering here

	//Call the render passes no check setup dual texture and detail texture proc
	(this->*m_pRenderPassesNoCheckSetup_SingleOrDualTextureAndDetailTextureProc)(DetailTextureInfo);

	//Single texture rendering does not need to be forced here since the detail texture is always the last pass


	for (INT PolyNum = 0; PolyNum < m_csPolyCount; PolyNum++) {
		m_d3dDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, m_curVertexBufferPos + MultiDrawFirstArray[PolyNum], MultiDrawCountArray[PolyNum] - 2);
	}

#ifdef UTGLR_DEBUG_WORLD_WIREFRAME
	m_d3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

	SetBlend(PF_Modulated);

	for (PolyNum = 0; PolyNum < m_csPolyCount; PolyNum++) {
		m_d3dDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, m_curVertexBufferPos + MultiDrawFirstArray[PolyNum], MultiDrawCountArray[PolyNum] - 2);
	}

	m_d3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
#endif

	//Advance vertex buffer position
	m_curVertexBufferPos += m_csPtCount;

#if 0
{
	dbgPrintf("utd3d9r: PassCount = %d\n", m_rpPassCount);
}
#endif
	m_rpPassCount = 0;


	unguard;
}

//Must be called with (m_rpPassCount > 0)
void UD3D9RenderDevice::RenderPassesNoCheckSetup(void) {
	INT i;
	INT t;

	SetBlend(MultiPass.TMU[0].PolyFlags);

	i = 0;
	do {
		if (i != 0) {
			SetTexEnv(i, MultiPass.TMU[i].PolyFlags);
		}

		SetTexture(i, *MultiPass.TMU[i].Info, MultiPass.TMU[i].PolyFlags, MultiPass.TMU[i].PanBias);
	} while (++i < m_rpPassCount);

	//Set stream state based on number of texture units in use
	SetStreamState(m_standardNTextureVertexDecl[m_rpPassCount - 1], NULL, NULL);

	//Check for additional enabled texture units that should be disabled
	DisableSubsequentTextures(m_rpPassCount);

	//Make sure at least m_csPtCount entries are left in the vertex buffers
	if ((m_curVertexBufferPos + m_csPtCount) >= VERTEX_ARRAY_SIZE) {
		FlushVertexBuffers();
	}

	//Lock vertexColor and texCoord buffers
	LockVertexColorBuffer();
	t = 0;
	do {
		LockTexCoordBuffer(t);
	} while (++t < m_rpPassCount);

	//Write vertex and color
	const FGLVertex *pSrcVertexArray = m_csVertexArray;
	FGLVertexColor *pVertexColorArray = m_pVertexColorArray;
	DWORD rpColor = m_rpColor;
	i = m_csPtCount;
	do {
		pVertexColorArray->x = pSrcVertexArray->x;
		pVertexColorArray->y = pSrcVertexArray->y;
		pVertexColorArray->z = pSrcVertexArray->z;
		pVertexColorArray->color = rpColor;
		pSrcVertexArray++;
		pVertexColorArray++;
	} while (--i != 0);

	//Write texCoord
	t = 0;
	do {
		const FLOAT UPan = TexInfo[t].UPan;
		const FLOAT VPan = TexInfo[t].VPan;
		const FLOAT UMult = TexInfo[t].UMult;
		const FLOAT VMult = TexInfo[t].VMult;
		//const FCoords2D* CC = TexInfo[t].CoordsMod; <- TODO: Don't implement this until we have a fragment shader version of it too!
		const FGLMapDot *pMapDot = MapDotArray;
		FGLTexCoord *pTexCoord = m_pTexCoordArray[t];

		INT ptCounter = m_csPtCount;
		do {
			pTexCoord->u = (pMapDot->u - UPan) * UMult;
			pTexCoord->v = (pMapDot->v - VPan) * VMult;
			//if (CC)
			//	CC->TransformPointUV(pTexCoord->u, pTexCoord->v, UMult, VMult);

			pMapDot++;
			pTexCoord++;
		} while (--ptCounter != 0);
	} while (++t < m_rpPassCount);

	//Unlock vertexColor and texCoord buffers
	UnlockVertexColorBuffer();
	t = 0;
	do {
		UnlockTexCoordBuffer(t);
	} while (++t < m_rpPassCount);

	return;
}

//Must be called with (m_rpPassCount > 0)
void UD3D9RenderDevice::RenderPassesNoCheckSetup_FP(void) {
	INT i;
	FLOAT vsParams[MAX_TMUNITS * 4];
	IDirect3DPixelShader9 *pixelShader = NULL;

	SetBlend(MultiPass.TMU[0].PolyFlags);

	//Look for a fragment program that can use if they're enabled
	if (UseFragmentProgram) {
		if (m_rpPassCount == 1) {
			pixelShader = m_fpComplexSurfaceSingleTexture;
		}
		else if (m_rpPassCount == 2) {
			if (MultiPass.TMU[1].PolyFlags == PF_Modulated) {
				pixelShader = m_fpComplexSurfaceDualTextureModulated;
			}
			else if (MultiPass.TMU[1].PolyFlags == PF_Highlighted) {
				pixelShader = m_fpComplexSurfaceSingleTextureWithFog;
			}
		}
		else if (m_rpPassCount == 3) {
			if (MultiPass.TMU[2].PolyFlags == PF_Modulated) {
				pixelShader = m_fpComplexSurfaceTripleTextureModulated;
			}
			else if (MultiPass.TMU[2].PolyFlags == PF_Highlighted) {
				pixelShader = m_fpComplexSurfaceDualTextureModulatedWithFog;
			}
		}
		else if (m_rpPassCount == 4) {
			if (MultiPass.TMU[3].PolyFlags == PF_Highlighted) {
				pixelShader = m_fpComplexSurfaceTripleTextureModulatedWithFog;
			}
		}
	}

	//Must use a fragment program with ps3.0
	if (pixelShader == NULL) {
		pixelShader = m_fpComplexSurfaceSingleTexture;
	}

	i = 0;
	do {
		//No TexEnv setup for fragment program

		SetTexture(i, *MultiPass.TMU[i].Info, MultiPass.TMU[i].PolyFlags, MultiPass.TMU[i].PanBias);

		vsParams[(i * 4) + 0] = TexInfo[i].UPan;
		vsParams[(i * 4) + 1] = TexInfo[i].VPan;
		vsParams[(i * 4) + 2] = TexInfo[i].UMult;
		vsParams[(i * 4) + 3] = TexInfo[i].VMult;
	} while (++i < m_rpPassCount);
	m_d3dDevice->SetVertexShaderConstantF(6, vsParams, m_rpPassCount);

	//Set vertex program based on number of texture units in use
	//Set fragment program if found a suitable one
	SetStreamState(m_oneColorVertexDecl, m_vpComplexSurface[m_rpPassCount - 1], pixelShader);

	//Check for additional enabled texture units that should be disabled
	DisableSubsequentTextures(m_rpPassCount);

	//Make sure at least m_csPtCount entries are left in the vertex buffers
	if ((m_curVertexBufferPos + m_csPtCount) >= VERTEX_ARRAY_SIZE) {
		FlushVertexBuffers();
	}

	//Lock vertexColor buffer
	LockVertexColorBuffer();

	//Write vertex and color
	const FGLVertex *pSrcVertexArray = m_csVertexArray;
	FGLVertexColor *pVertexColorArray = m_pVertexColorArray;
	DWORD rpColor = m_rpColor;
	i = m_csPtCount;
	do {
		pVertexColorArray->x = pSrcVertexArray->x;
		pVertexColorArray->y = pSrcVertexArray->y;
		pVertexColorArray->z = pSrcVertexArray->z;
		pVertexColorArray->color = rpColor;
		pSrcVertexArray++;
		pVertexColorArray++;
	} while (--i != 0);

	//Unlock vertexColor buffer
	UnlockVertexColorBuffer();

	return;
}

//Must be called with (m_rpPassCount > 0)
void UD3D9RenderDevice::RenderPassesNoCheckSetup_SingleOrDualTextureAndDetailTexture(FTextureInfo &DetailTextureInfo) {
	INT i;
	INT t;
	FLOAT NearZ  = 380.0f;
	FLOAT RNearZ = 1.0f / NearZ;

	//Two extra texture units used for detail texture
	m_rpPassCount += 2;

	SetBlend(MultiPass.TMU[0].PolyFlags);

	//Surface texture must be 2X blended
	//Also force PF_Modulated for the TexEnv stage
	MultiPass.TMU[0].PolyFlags |= (PF_Modulated | PF_FlatShaded);

	//Detail texture uses first two texture units
	//Other textures use last two texture units
	i = 2;
	do {
		if (i != 0) {
			SetTexEnv(i, MultiPass.TMU[i - 2].PolyFlags);
		}

		SetTexture(i, *MultiPass.TMU[i - 2].Info, MultiPass.TMU[i - 2].PolyFlags, MultiPass.TMU[i - 2].PanBias);
	} while (++i < m_rpPassCount);

	SetAlphaTexture(0);

	SetTexEnv(1, PF_Memorized);
	SetTextureNoPanBias(1, DetailTextureInfo, PF_Modulated);

	//Set stream state based on number of texture units in use
	SetStreamState(m_standardNTextureVertexDecl[m_rpPassCount - 1], NULL, NULL);

	//Check for additional enabled texture units that should be disabled
	DisableSubsequentTextures(m_rpPassCount);

	//Make sure at least m_csPtCount entries are left in the vertex buffers
	if ((m_curVertexBufferPos + m_csPtCount) >= VERTEX_ARRAY_SIZE) {
		FlushVertexBuffers();
	}

	//Lock vertexColor and texCoord buffers
	LockVertexColorBuffer();
	t = 0;
	do {
		LockTexCoordBuffer(t);
	} while (++t < m_rpPassCount);

	//Write vertex and color
	const FGLVertex *pSrcVertexArray = m_csVertexArray;
	FGLVertexColor *pVertexColorArray = m_pVertexColorArray;
	DWORD detailColor = m_detailTextureColor4ub | 0xFF000000;
	i = m_csPtCount;
	do {
		pVertexColorArray->x = pSrcVertexArray->x;
		pVertexColorArray->y = pSrcVertexArray->y;
		pVertexColorArray->z = pSrcVertexArray->z;
		pVertexColorArray->color = detailColor;
		pSrcVertexArray++;
		pVertexColorArray++;
	} while (--i != 0);

	//Alpha texture for detail texture uses texture unit 0
	{
		INT t = 0;
		const FGLVertex *pVertex = &m_csVertexArray[0];
		FGLTexCoord *pTexCoord = m_pTexCoordArray[t];

		INT ptCounter = m_csPtCount;
		do {
			pTexCoord->u = pVertex->z * RNearZ;
			pTexCoord->v = 0.5f;

			pVertex++;
			pTexCoord++;
		} while (--ptCounter != 0);
	}
	//Detail texture uses texture unit 1
	//Remaining 1 or 2 textures use texture units 2 and 3
	t = 1;
	do {
		FLOAT UPan = TexInfo[t].UPan;
		FLOAT VPan = TexInfo[t].VPan;
		FLOAT UMult = TexInfo[t].UMult;
		FLOAT VMult = TexInfo[t].VMult;
		const FGLMapDot *pMapDot = &MapDotArray[0];
		FGLTexCoord *pTexCoord = m_pTexCoordArray[t];

		INT ptCounter = m_csPtCount;
		do {
			pTexCoord->u = (pMapDot->u - UPan) * UMult;
			pTexCoord->v = (pMapDot->v - VPan) * VMult;

			pMapDot++;
			pTexCoord++;
		} while (--ptCounter != 0);
	} while (++t < m_rpPassCount);

	//Unlock vertexColor and texCoord buffers
	UnlockVertexColorBuffer();
	t = 0;
	do {
		UnlockTexCoordBuffer(t);
	} while (++t < m_rpPassCount);

	return;
}

//Must be called with (m_rpPassCount > 0)
void UD3D9RenderDevice::RenderPassesNoCheckSetup_SingleOrDualTextureAndDetailTexture_FP(FTextureInfo &DetailTextureInfo) {
	INT i;
	DWORD detailTexUnit;
	IDirect3DVertexShader9 *vertexShader = NULL;
	IDirect3DPixelShader9 *pixelShader = NULL;
	FLOAT vsParams[3 * 4];

	//One extra texture unit used for detail texture
	m_rpPassCount += 1;

	//Detail texture is in the last texture unit
	detailTexUnit = (m_rpPassCount - 1);

	if (m_rpPassCount == 2) {
		vertexShader = m_vpComplexSurfaceSingleTextureAndDetailTexture;
	}
	else {
		vertexShader = m_vpComplexSurfaceDualTextureAndDetailTexture;
	}
	if (DetailMax >= 2) {
		if (m_rpPassCount == 2) {
			pixelShader = m_fpSingleTextureAndDetailTextureTwoLayer;
		}
		else {
			pixelShader = m_fpDualTextureAndDetailTextureTwoLayer;
		}
	}
	else {
		if (m_rpPassCount == 2) {
			pixelShader = m_fpSingleTextureAndDetailTexture;
		}
		else {
			pixelShader = m_fpDualTextureAndDetailTexture;
		}
	}

	SetBlend(MultiPass.TMU[0].PolyFlags);

	//First one or two textures in first two texture units
	i = 0;
	do {
		//No TexEnv setup for fragment program
		//Only works with modulated

		SetTexture(i, *MultiPass.TMU[i].Info, MultiPass.TMU[i].PolyFlags, MultiPass.TMU[i].PanBias);

		vsParams[(i * 4) + 0] = TexInfo[i].UPan;
		vsParams[(i * 4) + 1] = TexInfo[i].VPan;
		vsParams[(i * 4) + 2] = TexInfo[i].UMult;
		vsParams[(i * 4) + 3] = TexInfo[i].VMult;
	} while (++i < detailTexUnit);

	//Detail texture in second or third texture unit
	//No TexEnv to set in fragment program mode
	SetTextureNoPanBias(detailTexUnit, DetailTextureInfo, PF_Modulated);

	vsParams[(detailTexUnit * 4) + 0] = TexInfo[detailTexUnit].UPan;
	vsParams[(detailTexUnit * 4) + 1] = TexInfo[detailTexUnit].VPan;
	vsParams[(detailTexUnit * 4) + 2] = TexInfo[detailTexUnit].UMult;
	vsParams[(detailTexUnit * 4) + 3] = TexInfo[detailTexUnit].VMult;
	m_d3dDevice->SetVertexShaderConstantF(6, vsParams, m_rpPassCount);

	//Set stream state
	SetStreamState(m_oneColorVertexDecl, vertexShader, pixelShader);

	//Check for additional enabled texture units that should be disabled
	DisableSubsequentTextures(m_rpPassCount);

	//Make sure at least m_csPtCount entries are left in the vertex buffers
	if ((m_curVertexBufferPos + m_csPtCount) >= VERTEX_ARRAY_SIZE) {
		FlushVertexBuffers();
	}

	//Lock vertexColor buffer
	LockVertexColorBuffer();

	//Write vertex and color
	const FGLVertex *pSrcVertexArray = m_csVertexArray;
	FGLVertexColor *pVertexColorArray = m_pVertexColorArray;
	DWORD detailColor = m_detailTextureColor4ub | 0xFF000000;
	i = m_csPtCount;
	do {
		*pVertexColorArray++ = FGLVertexColor(pSrcVertexArray->x, pSrcVertexArray->y, pSrcVertexArray->z, detailColor);
		pSrcVertexArray++;
	} while (--i != 0);

	//Unlock vertexColor buffer
	UnlockVertexColorBuffer();

	return;
}

//Modified this routine to always set up detail texture state
//It should only be called if at least one polygon will be detail textured
void UD3D9RenderDevice::DrawDetailTexture(FTextureInfo &DetailTextureInfo, bool clipDetailTexture) {
	//Setup detail texture state
	SetBlend(PF_Modulated);

	//Set detail alpha mode flag
	bool detailAlphaMode = ((clipDetailTexture == false) && UseDetailAlpha) ? true : false;

	if (detailAlphaMode) {
		SetAlphaTexture(0);
		//TexEnv 0 is PF_Modulated by default

		SetTextureNoPanBias(1, DetailTextureInfo, PF_Modulated);
		SetTexEnv(1, PF_Memorized);

		//Set stream state for two textures
		SetStreamState(m_standardNTextureVertexDecl[1], NULL, NULL);

		//Check for additional enabled texture units that should be disabled
		DisableSubsequentTextures(2);
	}
	else {
		SetTextureNoPanBias(0, DetailTextureInfo, PF_Modulated);
		SetTexEnv(0, PF_Memorized);

		if (clipDetailTexture == true) {
			FLOAT fDepthBias = -1.0f;
			m_d3dDevice->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD *)&fDepthBias);
		}

		//Set stream state for one texture
		SetStreamState(m_standardNTextureVertexDecl[0], NULL, NULL);

		//Check for additional enabled texture units that should be disabled
		DisableSubsequentTextures(1);
	}


	//Get detail texture color
	DWORD detailColor = m_detailTextureColor4ub;

	INT detailPassNum = 0;
	FLOAT NearZ  = 380.0f;
	FLOAT RNearZ = 1.0f / NearZ;
	FLOAT DetailScale = 1.0f;
	do {
		//Set up new NearZ and rescan points if subsequent pass
		if (detailPassNum > 0) {
			//Adjust NearZ and detail texture scaling
			NearZ /= 4.223f;
			RNearZ *= 4.223f;
			DetailScale *= 4.223f;

			//Rescan points
			(this->*m_pBufferDetailTextureDataProc)(NearZ);
		}

		//Calculate scaled UMult and VMult for detail texture based on mode
		FLOAT DetailUMult;
		FLOAT DetailVMult;
		if (detailAlphaMode) {
			DetailUMult = TexInfo[1].UMult * DetailScale;
			DetailVMult = TexInfo[1].VMult * DetailScale;
		}
		else {
			DetailUMult = TexInfo[0].UMult * DetailScale;
			DetailVMult = TexInfo[0].VMult * DetailScale;
		}

		INT Index = 0;

		INT *pNumPts = &MultiDrawCountArray[0];
		DWORD *pDetailTextureIsNear = DetailTextureIsNearArray;
		for (DWORD PolyNum = 0; PolyNum < m_csPolyCount; PolyNum++, pNumPts++, pDetailTextureIsNear++) {
			DWORD NumPts = *pNumPts;
			DWORD isNearBits = *pDetailTextureIsNear;

			//Skip the polygon if it will not be detail textured
			if (isNearBits == 0) {
				Index += NumPts;
				continue;
			}
			INT StartIndex = Index;

			DWORD allPtsBits = ~(~0U << NumPts);
			//Detail alpha mode
			if (detailAlphaMode) {
				//Make sure at least NumPts entries are left in the vertex buffers
				if ((m_curVertexBufferPos + NumPts) >= VERTEX_ARRAY_SIZE) {
					FlushVertexBuffers();
				}

				//Lock vertexColor, texCoord0, and texCoord1 buffers
				LockVertexColorBuffer();
				LockTexCoordBuffer(0);
				LockTexCoordBuffer(1);

				FGLTexCoord *pTexCoord0 = m_pTexCoordArray[0];
				FGLTexCoord *pTexCoord1 = m_pTexCoordArray[1];
				FGLVertexColor *pVertexColorArray = m_pVertexColorArray;
				const FGLVertex *pSrcVertexArray = &m_csVertexArray[StartIndex];
				for (INT i = 0; i < NumPts; i++) {
					FLOAT U = MapDotArray[Index].u;
					FLOAT V = MapDotArray[Index].v;

					FLOAT PointZ_Times_RNearZ = pSrcVertexArray[i].z * RNearZ;
					pTexCoord0[i].u = PointZ_Times_RNearZ;
					pTexCoord0[i].v = 0.5f;
					pTexCoord1[i].u = (U - TexInfo[1].UPan) * DetailUMult;
					pTexCoord1[i].v = (V - TexInfo[1].VPan) * DetailVMult;

					pVertexColorArray[i].x = pSrcVertexArray[i].x;
					pVertexColorArray[i].y = pSrcVertexArray[i].y;
					pVertexColorArray[i].z = pSrcVertexArray[i].z;
					pVertexColorArray[i].color = detailColor | 0xFF000000;

					Index++;
				}

				//Unlock vertexColor,texCoord0, and texCoord1 buffers
				UnlockVertexColorBuffer();
				UnlockTexCoordBuffer(0);
				UnlockTexCoordBuffer(1);

				//Draw the triangles
				m_d3dDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, m_curVertexBufferPos, NumPts - 2);

				//Advance vertex buffer position
				m_curVertexBufferPos += NumPts;
			}
			//Otherwise, no clipping required, or clipping required, but DetailClipping not enabled
			else if ((clipDetailTexture == false) || (isNearBits == allPtsBits)) {
				//Make sure at least NumPts entries are left in the vertex buffers
				if ((m_curVertexBufferPos + NumPts) >= VERTEX_ARRAY_SIZE) {
					FlushVertexBuffers();
				}

				//Lock vertexColor and texCoord0 buffers
				LockVertexColorBuffer();
				LockTexCoordBuffer(0);

				FGLTexCoord *pTexCoord = m_pTexCoordArray[0];
				FGLVertexColor *pVertexColorArray = m_pVertexColorArray;
				const FGLVertex *pSrcVertexArray = &m_csVertexArray[StartIndex];
				for (INT i = 0; i < NumPts; i++) {
					FLOAT U = MapDotArray[Index].u;
					FLOAT V = MapDotArray[Index].v;

					pTexCoord[i].u = (U - TexInfo[0].UPan) * DetailUMult;
					pTexCoord[i].v = (V - TexInfo[0].VPan) * DetailVMult;

					pVertexColorArray[i].x = pSrcVertexArray[i].x;
					pVertexColorArray[i].y = pSrcVertexArray[i].y;
					pVertexColorArray[i].z = pSrcVertexArray[i].z;
					DWORD alpha = appRound((1.0f - (Clamp(pSrcVertexArray[i].z, 0.0f, NearZ) * RNearZ)) * 255.0f);
					pVertexColorArray[i].color = detailColor | (alpha << 24);

					Index++;
				}

				//Unlock vertexColor and texCoord0 buffers
				UnlockVertexColorBuffer();
				UnlockTexCoordBuffer(0);

				//Draw the triangles
				m_d3dDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, m_curVertexBufferPos, NumPts - 2);

				//Advance vertex buffer position
				m_curVertexBufferPos += NumPts;
			}
			//Otherwise, clipping required and DetailClipping enabled
			else {
				//Make sure at least (NumPts * 2) entries are left in the vertex buffers
				if ((m_curVertexBufferPos + (NumPts * 2)) >= VERTEX_ARRAY_SIZE) {
					FlushVertexBuffers();
				}

				//Lock vertexColor and texCoord0 buffers
				LockVertexColorBuffer();
				LockTexCoordBuffer(0);

				DWORD NextIndex = 0;
				DWORD isNear_i_bit = 1U << (NumPts - 1);
				DWORD isNear_j_bit = 1U;
				FGLTexCoord *pTexCoord = m_pTexCoordArray[0];
				for (INT i = 0, j = NumPts - 1; i < NumPts; j = i++, isNear_j_bit = isNear_i_bit, isNear_i_bit >>= 1) {
					const FGLVertex &Point = m_csVertexArray[Index];
					FLOAT U = MapDotArray[Index].u;
					FLOAT V = MapDotArray[Index].v;

					if (((isNear_i_bit & isNearBits) != 0) && ((isNear_j_bit & isNearBits) == 0)) {
						const FGLVertex &PrevPoint = m_csVertexArray[StartIndex + j];
						FLOAT PrevU = MapDotArray[StartIndex + j].u;
						FLOAT PrevV = MapDotArray[StartIndex + j].v;

						FLOAT dist = PrevPoint.z - Point.z;
						FLOAT m = 1.0f;
						if (dist > 0.001f) {
							m = (NearZ - Point.z) / dist;
						}
						FGLVertexColor *pVertexColor = &m_pVertexColorArray[NextIndex];
						pVertexColor->x = (m * (PrevPoint.x - Point.x)) + Point.x;
						pVertexColor->y = (m * (PrevPoint.y - Point.y)) + Point.y;
						pVertexColor->z = NearZ;
						DWORD alpha = 0;
						pVertexColor->color = detailColor | (alpha << 24);

						pTexCoord[NextIndex].u = ((m * (PrevU - U)) + U - TexInfo[0].UPan) * DetailUMult;
						pTexCoord[NextIndex].v = ((m * (PrevV - V)) + V - TexInfo[0].VPan) * DetailVMult;

						NextIndex++;
					}

					if ((isNear_i_bit & isNearBits) != 0) {
						pTexCoord[NextIndex].u = (U - TexInfo[0].UPan) * DetailUMult;
						pTexCoord[NextIndex].v = (V - TexInfo[0].VPan) * DetailVMult;

						FGLVertexColor *pVertexColor = &m_pVertexColorArray[NextIndex];
						pVertexColor->x = Point.x;
						pVertexColor->y = Point.y;
						pVertexColor->z = Point.z;
						DWORD alpha = appRound((1.0f - (Clamp(Point.z, 0.0f, NearZ) * RNearZ)) * 255.0f);
						pVertexColor->color = detailColor | (alpha << 24);

						NextIndex++;
					}

					if (((isNear_i_bit & isNearBits) == 0) && ((isNear_j_bit & isNearBits) != 0)) {
						const FGLVertex &PrevPoint = m_csVertexArray[StartIndex + j];
						FLOAT PrevU = MapDotArray[StartIndex + j].u;
						FLOAT PrevV = MapDotArray[StartIndex + j].v;

						FLOAT dist = Point.z - PrevPoint.z;
						FLOAT m = 1.0f;
						if (dist > 0.001f) {
							m = (NearZ - PrevPoint.z) / dist;
						}
						FGLVertexColor *pVertexColor = &m_pVertexColorArray[NextIndex];
						pVertexColor->x = (m * (Point.x - PrevPoint.x)) + PrevPoint.x;
						pVertexColor->y = (m * (Point.y - PrevPoint.y)) + PrevPoint.y;
						pVertexColor->z = NearZ;
						DWORD alpha = 0;
						pVertexColor->color = detailColor | (alpha << 24);

						pTexCoord[NextIndex].u = ((m * (U - PrevU)) + PrevU - TexInfo[0].UPan) * DetailUMult;
						pTexCoord[NextIndex].v = ((m * (V - PrevV)) + PrevV - TexInfo[0].VPan) * DetailVMult;

						NextIndex++;
					}

					Index++;
				}

				//Unlock vertexColor and texCoord0 buffers
				UnlockVertexColorBuffer();
				UnlockTexCoordBuffer(0);

				//Draw the triangles
				m_d3dDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, m_curVertexBufferPos, NextIndex - 2);

				//Advance vertex buffer position
				m_curVertexBufferPos += NextIndex;
			}
		}
	} while (++detailPassNum < DetailMax);


	//Clear detail texture state
	if (detailAlphaMode) {
		//TexEnv 0 was left in default state of PF_Modulated
	}
	else {
		SetTexEnv(0, PF_Modulated);

		if (clipDetailTexture == true) {
			FLOAT fDepthBias = 0.0f;
			m_d3dDevice->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD *)&fDepthBias);
		}
	}

	return;
}

//Modified this routine to always set up detail texture state
//It should only be called if at least one polygon will be detail textured
void UD3D9RenderDevice::DrawDetailTexture_FP(FTextureInfo &DetailTextureInfo) {
	INT Index = 0;

	//Setup detail texture state
	SetBlend(PF_Modulated);

	//No TexEnv to set in fragment program mode
	SetTextureNoPanBias(0, DetailTextureInfo, PF_Modulated);

	FLOAT vsParams[4] = { TexInfo[0].UPan, TexInfo[0].VPan, TexInfo[0].UMult, TexInfo[0].VMult };
	m_d3dDevice->SetVertexShaderConstantF(6, vsParams, 1);

	//Set vertex program and fragment program for detail texture
	IDirect3DPixelShader9 *pixelShader = m_fpDetailTexture;
	if (DetailMax >= 2) pixelShader = m_fpDetailTextureTwoLayer;
	SetStreamState(m_oneColorVertexDecl, m_vpDetailTexture, pixelShader);

	//Check for additional enabled texture units that should be disabled
	DisableSubsequentTextures(1);

	DWORD detailColor = m_detailTextureColor4ub | 0xFF000000;
	INT *pNumPts = &MultiDrawCountArray[0];
	DWORD *pDetailTextureIsNear = DetailTextureIsNearArray;
	DWORD csPolyCount = m_csPolyCount;
	for (DWORD PolyNum = 0; PolyNum < csPolyCount; PolyNum++, pNumPts++, pDetailTextureIsNear++) {
		INT NumPts = *pNumPts;
		DWORD isNearBits = *pDetailTextureIsNear;
		INT i;

		//Skip the polygon if it will not be detail textured
		if (isNearBits == 0) {
			Index += NumPts;
			continue;
		}

		//Make sure at least NumPts entries are left in the vertex buffers
		if ((m_curVertexBufferPos + NumPts) >= VERTEX_ARRAY_SIZE) {
			FlushVertexBuffers();
		}

		//Lock vertexColor buffer
		LockVertexColorBuffer();

		//Write vertex
		const FGLVertex *pSrcVertexArray = &m_csVertexArray[Index];
		FGLVertexColor *pVertexColorArray = m_pVertexColorArray;
		for (i = 0; i < NumPts; i++) {
			pVertexColorArray[i].x = pSrcVertexArray[i].x;
			pVertexColorArray[i].y = pSrcVertexArray[i].y;
			pVertexColorArray[i].z = pSrcVertexArray[i].z;
			pVertexColorArray[i].color = detailColor;
		}

		//Unlock vertexColor buffers
		UnlockVertexColorBuffer();

		//Draw the triangles
		m_d3dDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, m_curVertexBufferPos, NumPts - 2);

		//Advance vertex buffer position
		m_curVertexBufferPos += NumPts;

		Index += NumPts;
	}


	//Clear detail texture state
	//TexEnv 0 was left in default state of PF_Modulated

	return;
}

INT UD3D9RenderDevice::BufferStaticComplexSurfaceGeometry(const FSurfaceFacet& Facet) {
	INT numVerts = 0;

	//Buffer static geometry
	m_csPolyCount = 0;
	FGLMapDot *pMapDot = &MapDotArray[0];
	FGLVertex *pVertex = &m_csVertexArray[0];
	for (FSavedPoly* Poly = Facet.Polys; Poly; Poly = Poly->Next) {
		//Skip if no points
		INT NumPts = Poly->NumPts;
		if (NumPts <= 0) {
			continue;
		}

		DWORD csPolyCount = m_csPolyCount;
		MultiDrawFirstArray[csPolyCount] = numVerts;
		MultiDrawCountArray[csPolyCount] = NumPts;
		m_csPolyCount = csPolyCount + 1;

		numVerts += NumPts;
		if (numVerts > VERTEX_ARRAY_SIZE) {
			return 0;
		}
		FTransform **pPts = &Poly->Pts[0];
		do {
			const FVector &Point = (*pPts++)->Point;

			pMapDot->u = (Facet.MapCoords.XAxis | Point) - m_csUDot;
			pMapDot->v = (Facet.MapCoords.YAxis | Point) - m_csVDot;
			pMapDot++;

			pVertex->x = Point.X;
			pVertex->y = Point.Y;
			pVertex->z = Point.Z;
			pVertex++;
		} while (--NumPts != 0);
	}

	return numVerts;
}

INT UD3D9RenderDevice::BufferStaticComplexSurfaceGeometry_VP(const FSurfaceFacet& Facet) {
	INT numVerts = 0;

	//Buffer static geometry
	m_csPolyCount = 0;
	FGLVertex *pVertex = &m_csVertexArray[0];
	for (FSavedPoly* Poly = Facet.Polys; Poly; Poly = Poly->Next) {
		//Skip if no points
		INT NumPts = Poly->NumPts;
		if (NumPts <= 0) {
			continue;
		}

		DWORD csPolyCount = m_csPolyCount;
		MultiDrawFirstArray[csPolyCount] = numVerts;
		MultiDrawCountArray[csPolyCount] = NumPts;
		m_csPolyCount = csPolyCount + 1;

		numVerts += NumPts;
		if (numVerts > VERTEX_ARRAY_SIZE) {
			return 0;
		}
		FTransform **pPts = &Poly->Pts[0];
		do {
			const FVector &Point = (*pPts++)->Point;

			pVertex->x = Point.X;
			pVertex->y = Point.Y;
			pVertex->z = Point.Z;
			pVertex++;
		} while (--NumPts != 0);
	}

	return numVerts;
}

DWORD UD3D9RenderDevice::BufferDetailTextureData(FLOAT NearZ) {
	DWORD *pDetailTextureIsNear = DetailTextureIsNearArray;
	DWORD anyIsNearBits = 0;

	FGLVertex *pVertex = &m_csVertexArray[0];
	INT *pNumPts = &MultiDrawCountArray[0];
	DWORD csPolyCount = m_csPolyCount;
	do {
		INT NumPts = *pNumPts++;
		DWORD isNear = 0;

		do {
			isNear <<= 1;
			if (pVertex->z < NearZ) {
				isNear |= 1;
			}
			pVertex++;
		} while (--NumPts != 0);

		*pDetailTextureIsNear++ = isNear;
		anyIsNearBits |= isNear;
	} while (--csPolyCount != 0);

	return anyIsNearBits;
}

#ifdef UTGLR_INCLUDE_SSE_CODE
__declspec(naked) DWORD UD3D9RenderDevice::BufferDetailTextureData_SSE2(FLOAT NearZ) {
	__asm {
		movd xmm0, [esp+4]

		push esi
		push edi

		lea esi, [ecx]this.m_csVertexArray
		lea edx, [ecx]this.MultiDrawCountArray
		lea edi, [ecx]this.DetailTextureIsNearArray

		pxor xmm1, xmm1

		mov ecx, [ecx]this.m_csPolyCount

		poly_count_loop:
			mov eax, [edx]
			add edx, 4

			pxor xmm2, xmm2

			num_pts_loop:
				movss xmm3, [esi+8]
				add esi, TYPE FGLVertex

				pslld xmm2, 1

				cmpltss xmm3, xmm0
				psrld xmm3, 31

				por xmm2, xmm3

				dec eax
				jne num_pts_loop

			movd [edi], xmm2
			add edi, 4

			por xmm1, xmm2

			dec ecx
			jne poly_count_loop

		movd eax, xmm1

		pop edi
		pop esi

		ret 4
	}
}
#endif //UTGLR_INCLUDE_SSE_CODE

void UD3D9RenderDevice::EndBufferingNoCheck(void) {
	switch (m_bufferedVertsType) {
	case BV_TYPE_GOURAUD_POLYS:
		EndGouraudPolygonBufferingNoCheck();
		break;

	case BV_TYPE_TILES:
		EndTileBufferingNoCheck();
		break;

	case BV_TYPE_LINES:
		EndLineBufferingNoCheck();
		break;

	case BV_TYPE_POINTS:
		EndPointBufferingNoCheck();
		break;

	default:
		;
	}

	m_bufferedVerts = 0;

	return;
}

void UD3D9RenderDevice::EndGouraudPolygonBufferingNoCheck(void) {
	SetDefaultAAState();
	//EndGouraudPolygonBufferingNoCheck sets its own projection state
	//Stream state set when start buffering
	//Default texture state set when start buffering

	clockFast(GouraudCycles);

	//Set projection state
	SetProjectionState(m_requestNearZRangeHackProjection);

	//Unlock vertexColor and texCoord0 buffers
	//Unlock secondary color buffer if fog
	UnlockVertexColorBuffer();
	if (m_requestedColorFlags & CF_FOG_MODE) {
		UnlockSecondaryColorBuffer();
	}
	UnlockTexCoordBuffer(0);

#ifdef UTGLR_DEBUG_ACTOR_WIREFRAME
	m_d3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
#endif

	//Draw the triangles
	m_d3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, m_curVertexBufferPos, m_bufferedVerts / 3);

	//Advance vertex buffer position
	m_curVertexBufferPos += m_bufferedVerts;

#ifdef UTGLR_DEBUG_ACTOR_WIREFRAME
	m_d3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
#endif

	unclockFast(GouraudCycles);
}

void UD3D9RenderDevice::EndTileBufferingNoCheck(void) {
	if (NoAATiles) {
		SetDisabledAAState();
	}
	else {
		SetDefaultAAState();
	}
	SetDefaultProjectionState();
	//Stream state set when start buffering
	//Default texture state set when start buffering

	clockFast(TileCycles);

	//Unlock vertexColor and texCoord0 buffers
	UnlockVertexColorBuffer();
	UnlockTexCoordBuffer(0);

	//Draw the quads (stored as triangles)
	m_d3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, m_curVertexBufferPos, m_bufferedVerts / 3);

	//Advance vertex buffer position
	m_curVertexBufferPos += m_bufferedVerts;

	unclockFast(TileCycles);
}

void UD3D9RenderDevice::EndLineBufferingNoCheck(void) {
	//AA state set when start buffering
	//Projection state set when start buffering
	//Stream state set when start buffering
	//Default texture state set when start buffering

	//Unlock vertexColor and texCoord0 buffers
	UnlockVertexColorBuffer();
	UnlockTexCoordBuffer(0);

	//Draw the lines
	m_d3dDevice->DrawPrimitive(D3DPT_LINELIST, m_curVertexBufferPos, m_bufferedVerts / 2);

	//Advance vertex buffer position
	m_curVertexBufferPos += m_bufferedVerts;
}

void UD3D9RenderDevice::EndPointBufferingNoCheck(void) {
	//AA state set when start buffering
	//Projection state set when start buffering
	//Stream state set when start buffering
	//Default texture state set when start buffering

	//Unlock vertexColor and texCoord0 buffers
	UnlockVertexColorBuffer();
	UnlockTexCoordBuffer(0);

	//Draw the points (stored as triangles)
	m_d3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, m_curVertexBufferPos, m_bufferedVerts / 3);

	//Advance vertex buffer position
	m_curVertexBufferPos += m_bufferedVerts;
}

void UD3D9RenderDevice::GetStats(TCHAR* Result) {
	guard(UD3D9RenderDevice::GetStats);

	const double msPerCycle = GSecondsPerCycle * 1000.0f;
	appSprintf(
		Result,
		TEXT("D3D9 Cycles:\nBind=%04.1f\nImage=%04.1f\nComplex=%04.1f\nGouraud=%04.1f\nTile=%04.1f\nBlend=%04.1f\nGouraudPolyList=%04.1f"),
		msPerCycle * BindCycles,
		msPerCycle * ImageCycles,
		msPerCycle * ComplexCycles,
		msPerCycle * GouraudCycles,
		msPerCycle * TileCycles,
		msPerCycle * BlendCycles,
		msPerCycle * GouraudPolyListCycles
	);

	unguard;
}

void UD3D9RenderDevice::DrawStats(FSceneNode* Frame)
{
	guard(UD3D9RenderDevice::::DrawStats);

	INT CurY = 48;

#if ENGINE_VERSION<400
	Frame->Viewport->Canvas->DrawColor = FColor(255, 255, 255);
#else
	Frame->Viewport->Canvas->Color = FColor(255, 255, 255);
#endif
	Frame->Viewport->Canvas->CurX = 392;
	Frame->Viewport->Canvas->CurY = CurY;
	Frame->Viewport->Canvas->WrappedPrintf(Frame->Viewport->Canvas->MedFont, 0, TEXT("[D3D9]"));
	Frame->Viewport->Canvas->CurX = 400;
	Frame->Viewport->Canvas->CurY = (CurY += 8);
	Frame->Viewport->Canvas->WrappedPrintf(Frame->Viewport->Canvas->MedFont, 0, TEXT("Cycles:"));
	Frame->Viewport->Canvas->CurX = 400;
	Frame->Viewport->Canvas->CurY = (CurY += 8);
	Frame->Viewport->Canvas->WrappedPrintf(Frame->Viewport->Canvas->MedFont, 0, TEXT("Bind____________= %05.2f"), GSecondsPerCycle * 1000 * BindCycles);
	Frame->Viewport->Canvas->CurX = 400;
	Frame->Viewport->Canvas->CurY = (CurY += 8);
	Frame->Viewport->Canvas->WrappedPrintf(Frame->Viewport->Canvas->MedFont, 0, TEXT("Image___________= %05.2f"), GSecondsPerCycle * 1000 * ImageCycles);
	Frame->Viewport->Canvas->CurX = 400;
	Frame->Viewport->Canvas->CurY = (CurY += 8);
	Frame->Viewport->Canvas->WrappedPrintf(Frame->Viewport->Canvas->MedFont, 0, TEXT("Blend___________= %05.2f"), GSecondsPerCycle * 1000 * BlendCycles);
	Frame->Viewport->Canvas->CurX = 400;
	Frame->Viewport->Canvas->CurY = (CurY += 8);
	Frame->Viewport->Canvas->WrappedPrintf(Frame->Viewport->Canvas->MedFont, 0, TEXT("Tile____________= %05.2f"), GSecondsPerCycle * 1000 * TileCycles);
	Frame->Viewport->Canvas->CurX = 400;
	Frame->Viewport->Canvas->CurY = (CurY += 8);
	Frame->Viewport->Canvas->WrappedPrintf(Frame->Viewport->Canvas->MedFont, 0, TEXT("ComplexSurface__= %05.2f"), GSecondsPerCycle * 1000 * ComplexCycles);
	Frame->Viewport->Canvas->CurX = 400;
	Frame->Viewport->Canvas->CurY = (CurY += 8);
	Frame->Viewport->Canvas->WrappedPrintf(Frame->Viewport->Canvas->MedFont, 0, TEXT("GouraudPoly_____= %05.2f"), GSecondsPerCycle * 1000 * GouraudCycles);
	Frame->Viewport->Canvas->CurX = 400;
	Frame->Viewport->Canvas->CurY = (CurY += 8);
	Frame->Viewport->Canvas->WrappedPrintf(Frame->Viewport->Canvas->MedFont, 0, TEXT("GouraudPolyList_= %05.2f"), GSecondsPerCycle * 1000 * GouraudPolyListCycles);

	unguard;
}

// Clipping planes!
BYTE UD3D9RenderDevice::PushClipPlane(const FPlane& Plane)
{
	if (m_NumClipPlanes == D3DMAXUSERCLIPPLANES)
		return 2;

	EndBuffering();
	float NP[4] = { Plane.X,Plane.Y,Plane.Z,-Plane.W };
	m_d3dDevice->SetClipPlane(m_NumClipPlanes, NP);
	++m_NumClipPlanes;
	m_d3dDevice->SetRenderState(D3DRS_CLIPPLANEENABLE, (1 << m_NumClipPlanes) - 1);
	return 1;
}
BYTE UD3D9RenderDevice::PopClipPlane()
{
	if (m_NumClipPlanes == 0)
		return 2;

	EndBuffering();
	--m_NumClipPlanes;
	m_d3dDevice->SetRenderState(D3DRS_CLIPPLANEENABLE, (1 << m_NumClipPlanes) - 1);
	return 1;
}

struct FStackedRTT
{
	FRenderToTexture* Tex;
	IDirect3DTexture9* Output;
	IDirect3DSurface9* RendSurface;
	IDirect3DSurface9* Surface;
	IDirect3DSurface9* StencilSurface;
	FStackedRTT* Previous;
	FSceneNode* PrevNode;

	FStackedRTT(FRenderToTexture* T, FStackedRTT* P, FSceneNode* TopNode)
		: Tex(T), Output(NULL), Surface(NULL), StencilSurface(NULL), Previous(P), PrevNode(TopNode)
	{}
	~FStackedRTT()
	{
		if (StencilSurface)
			StencilSurface->Release();
		if (Surface)
			Surface->Release();
		if (RendSurface)
			RendSurface->Release();
		if (Output)
			Output->Release();
	}
};
static FStackedRTT* RTTStack = NULL;
static IDirect3DSurface9* TopRenderTarget = NULL;
static IDirect3DSurface9* TopStencil = NULL;

#define VERIFY_PTT_CALL(fnc,call) if (call != D3D_OK) \
	{ \
		warnf(TEXT("D3D9 PushRenderToTexture: %ls FAILED!"), TEXT(#fnc)); \
		delete NewStack; \
		return FALSE; \
	}

UBOOL UD3D9RenderDevice::PushRenderToTexture(FRenderToTexture* Tex)
{
	guard(UD3D9RenderDevice::PushRenderToTexture);
	FStackedRTT* NewStack = new FStackedRTT(Tex, RTTStack, TopSceneNode);
	if (!RTTStack)
	{
		VERIFY_PTT_CALL(GetRenderTarget, m_d3dDevice->GetRenderTarget(0, &TopRenderTarget));
		VERIFY_PTT_CALL(GetDepthStencilSurface, m_d3dDevice->GetDepthStencilSurface(&TopStencil));
	}
	VERIFY_PTT_CALL(CreateRenderTarget, m_d3dDevice->CreateRenderTarget(Tex->ScreenX, Tex->ScreenY, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, FALSE, &NewStack->RendSurface, NULL));
	VERIFY_PTT_CALL(CreateTexture, m_d3dDevice->CreateTexture(Tex->ScreenX, Tex->ScreenY, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &NewStack->Output, NULL));
	VERIFY_PTT_CALL(GetSurfaceLevel, NewStack->Output->GetSurfaceLevel(0, &NewStack->Surface));
	VERIFY_PTT_CALL(SetRenderTarget, m_d3dDevice->SetRenderTarget(0, NewStack->RendSurface));
	VERIFY_PTT_CALL(CreateDepthStencilSurface, m_d3dDevice->CreateDepthStencilSurface(Tex->ScreenX, Tex->ScreenY, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &NewStack->StencilSurface, NULL));
	VERIFY_PTT_CALL(SetDepthStencilSurface, m_d3dDevice->SetDepthStencilSurface(NewStack->StencilSurface));
	RTTStack = NewStack;
	SetSceneNode(Tex->Frame);
	return TRUE;
	unguard;
}
#undef VERIFY_PTT_CALL

void UD3D9RenderDevice::PopRenderToTexture()
{
	guard(UD3D9RenderDevice::PopRenderToTexture);
	verify(RTTStack != NULL);
	if (RTTStack->Previous)
	{
		verify(m_d3dDevice->SetRenderTarget(0, RTTStack->Previous->RendSurface) == D3D_OK);
		verify(m_d3dDevice->SetDepthStencilSurface(RTTStack->Previous->StencilSurface) == D3D_OK);
	}
	else
	{
		verify(m_d3dDevice->SetRenderTarget(0, TopRenderTarget) == D3D_OK);
		verify(m_d3dDevice->SetDepthStencilSurface(TopStencil) == D3D_OK);
	}
	const INT XSize = RTTStack->Tex->ScreenX;
	const INT YSize = RTTStack->Tex->ScreenY;
	HDC BitmapHDC;
	RTTStack->Surface->GetDC(&BitmapHDC);
	HDC hMemoryDC = CreateCompatibleDC(BitmapHDC);
	HBITMAP hMemoryBmp = CreateCompatibleBitmap(hMemoryDC, XSize, YSize);
	HBITMAP hOldBitmap = static_cast<HBITMAP>(SelectObject(hMemoryDC, hMemoryBmp));

	BitBlt(hMemoryDC, 0, 0, XSize, YSize, BitmapHDC, 0, 0, SRCCOPY);
	DWORD* DestBits = reinterpret_cast<DWORD*>(RTTStack->Tex->ScreenPtr);
	for (INT x = 0; x < XSize; ++x)
	{
		for (INT y = 0; y < YSize; ++y)
		{
			COLORREF color = GetPixel(hMemoryDC, x, y);
			*DestBits++ = color;
		}
	}

	SelectObject(hMemoryDC, hOldBitmap);
	DeleteObject(hMemoryBmp);
	DeleteDC(hMemoryDC);
	RTTStack->Surface->ReleaseDC(BitmapHDC);

	SetSceneNode(RTTStack->PrevNode);
	FStackedRTT* Prev = RTTStack->Previous;
	delete RTTStack;
	RTTStack = Prev;
	unguard;
}


// Static variables.
INT UD3D9RenderDevice::NumDevices = 0;
INT UD3D9RenderDevice::LockCount = 0;

HMODULE UD3D9RenderDevice::hModuleD3d9 = NULL;
LPDIRECT3DCREATE9 UD3D9RenderDevice::pDirect3DCreate9 = NULL;

bool UD3D9RenderDevice::g_gammaFirstTime = false;
bool UD3D9RenderDevice::g_haveOriginalGammaRamp = false;

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
