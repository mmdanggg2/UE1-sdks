/*=============================================================================
	OpenGL_GLSL3Rendering.cpp

	GLSL 3.30 shader rendering.

	Fully buffered.

	Notes on multi-buffering:
	* All arrays are separated and may be populated without buffer flushing/drawing.
	* The buffer only flushes existing data if draw order needs to be preserved (transparency, vert limits).
	* Because Textures need to be bound, changes in textures can cause buffer flushing.

	Decals and Fog:
	* In GLSL3 mode, modulated decals cause ComplexSurface buffering to be split into two stages.
	* Stage 1 renders everything and defers fog maps to Stage 2.
	* Decals in the GouraudTriangle buffer are drawn inbetween both stages.

	Due to the nature of the shaders, static Bsp geometry drawing is not supported.

	TODO:

	Revision history:
	* Created by Fernando Velazquez.
=============================================================================*/

#include "OpenGLDrv.h"
#include "UTGLROpenGL.h"
#include "FOpenGL3.h"


#define GL3 ((FOpenGL3*)GL)

#define COMPLEX       (*(FGL::DrawBuffer::FComplexGLSL3*)      UOpenGLRenderDevice::DrawBuffer.Complex)
#define COMPLEXSTATIC (*(FGL::DrawBuffer::FComplexAltGLSL3*)   UOpenGLRenderDevice::DrawBuffer.ComplexStatic)
#define GOURAUD       (*(FGL::DrawBuffer::FGouraudGLSL3*)      UOpenGLRenderDevice::DrawBuffer.Gouraud)
#define QUAD          (*(FGL::DrawBuffer::FQuadGLSL3*)         UOpenGLRenderDevice::DrawBuffer.Quad)
#define LINE          (*(FGL::DrawBuffer::FLineGLSL3*)         UOpenGLRenderDevice::DrawBuffer.Line)
#define FILL          (*(FGL::DrawBuffer::FFillGLSL3*)         UOpenGLRenderDevice::DrawBuffer.Fill)
#define DECAL         (*(FGL::DrawBuffer::FDecalGLSL3*)        UOpenGLRenderDevice::DrawBuffer.Decal)

#define BUFFER_QUADEXPANSION (*(FGL::VertexBuffer::CopyToVBO*) UOpenGLRenderDevice::DrawBuffer.GeneralBuffer[2])

/*-----------------------------------------------------------------------------
	Geometry buffering paths.

	Designed with 1:1 mapping of attribs to streaming data in mind.
	Provides us with an error free, branch free implementation for every vertex
	being written onto the buffer.

	Notes:
	- BT_TextureIndex is dependent on BT_Texture0, so we check for BT_Texture0
	instead to reduce amount of precompiled paths.
	- BT_Vertex is always assumed to be present.
-----------------------------------------------------------------------------*/

// Clip plane handling
static DWORD          ClipPlaneCount = 0;
static FPlane         ClipPlanes[32];

// Cached uniforms
static INT            csTextureScaleIndex = 0;
static INT            csTextureScaleIndexDecal = 0;
static FGL::uint8x4   TextureLayers;
static FGL::uint8x4   LightInfo;

// Complex surface decal status
static DWORD          csDecalQueued = 0;
static UBOOL          csVolumetrics = 0;

// Complex surface index arrays
static DWORD          csIndexCount = 0;

// Tile surface index arrays
static INT            TileCount = 0;

// Gouraud vertex, stride is constant
struct FGouraudVertexData
{
	enum EBufferPath
	{
		BP_Unlit,
		BP_Light,
		BP_LightFog,
	};
	FGL::DrawBuffer::FGouraudGLSL3::VertexType        Vertex;
	FGL::DrawBuffer::FGouraudGLSL3::Color0Type        Color0;
	FGL::DrawBuffer::FGouraudGLSL3::Color1Type        Color1;
	FGL::DrawBuffer::FGouraudGLSL3::Texture0Type      TextureUV;
	FGL::DrawBuffer::FGouraudGLSL3::TextureLayersType TextureLayers;
	FGL::DrawBuffer::FGouraudGLSL3::VertexParam1Type  TextureScaleIndices;
};
static_assert(sizeof(FGouraudVertexData) == FGL::DrawBuffer::FGouraudGLSL3::GetConstStride(), "Incorrect buffer stride");

//
// Buffer DrawComplexSurface geometry
//
static float csUDot = 0;
static float csVDot = 0;
static FGL::FTextureScale csLightScale;
static FGL::FTexturePan   csLightPan;
static FGL::FTextureScale csFogScale;
static FGL::FTexturePan   csFogPan;
template <DWORD Attribs> void BufferComplexSurface( FGL::DrawBuffer::FComplexGLSL3& Complex, FSurfaceInfo& Surface, FSurfaceFacet& Facet)
{
	using namespace FGL::DrawBuffer;

	// Polygon(s) will be texture mapped
	if ( Attribs & BT_Texture0 )
	{
		csUDot = Facet.MapCoords.XAxis | Facet.MapCoords.Origin;
		csVDot = Facet.MapCoords.YAxis | Facet.MapCoords.Origin;
		TextureLayers.Data[0] = UOpenGLRenderDevice::DrawBuffer.Draw.Layers[0];
		TextureLayers.Data[1] = UOpenGLRenderDevice::DrawBuffer.Draw.Layers[1];
		TextureLayers.Data[2] = UOpenGLRenderDevice::DrawBuffer.Draw.Layers[2];
	}

	// Light and fog scale may change, even if using the same atlas.
	if ( Attribs & (BT_Texture1|BT_Texture2) )
	{
		LightInfo.Data[0] = (BYTE) GetZoneNumber(Surface);
		LightInfo.Data[1] = UOpenGLRenderDevice::DrawBuffer.Draw.Layers[TMU_LightMap];
		LightInfo.Data[2] = UOpenGLRenderDevice::DrawBuffer.Draw.Layers[TMU_FogMap];
		if ( Attribs & BT_Texture1 )
		{
			csLightScale = *Surface.LightMap;
			csLightPan   = FGL::FTexturePan(*Surface.LightMap,-0.5);
		}
		if ( Attribs & BT_Texture2 )
		{
			csFogScale = *Surface.FogMap;
			csFogPan   = FGL::FTexturePan(*Surface.FogMap,-0.5);
		}
	}

	constexpr DWORD Stride = sizeof(FComplexGLSL3::VertexType)
		+ ((Attribs & BT_Color0)   ? sizeof(FComplexGLSL3::Color0Type)       : 0)
		+ ((Attribs & BT_Texture0) ? sizeof(FComplexGLSL3::Texture0Type)     : 0)
		+ ((Attribs & BT_Texture1) ? sizeof(FComplexGLSL3::Texture1Type)     : 0)
		+ ((Attribs & BT_Texture2) ? sizeof(FComplexGLSL3::Texture2Type)     : 0)
		+ ((Attribs & BT_Texture0) ? sizeof(FComplexGLSL3::TextureLayersType) : 0)
		+ ((Attribs & (BT_Texture1|BT_Texture2)) ? sizeof(FComplexGLSL3::LightInfoType) : 0)
		+ ((Attribs & BT_Texture0) ? sizeof(FComplexGLSL3::VertexParam0Type) : 0)
		+ ((Attribs & BT_Texture0) ? sizeof(FComplexGLSL3::VertexParam1Type) : 0);
	if ( GL_DEV && (Stride != Complex.BufferStride) )
		appErrorf( TEXT("Stride inconsistent: %i / %i (%i)"), Stride, Complex.BufferStride, Attribs);

	for ( FSavedPoly* Poly=Facet.Polys; Poly; Poly=Poly->Next)
	{
		INT NumPts = Poly->NumPts;
		if ( NumPts > 2 )
		{
			void* Stream;
			Complex.StreamStart(Stream);
			for ( INT v=0; v<NumPts; v++)
			{
				FVector& Point = Poly->Pts[v]->Point;
				StreamData(FGL::FVertex3D(Point), Stream);
				if ( Attribs & BT_Color0 )
					StreamData(FGL::FColorRGBA(Surface.FlatColor), Stream);
				if ( Attribs & (BT_Texture0|BT_Texture1|BT_Texture2) )
				{
					FLOAT U = (Facet.MapCoords.XAxis | Point) - csUDot;
					FLOAT V = (Facet.MapCoords.YAxis | Point) - csVDot;
					if ( Attribs & BT_Texture0 )
						StreamData(FGL::FTextureUV( U, V), Stream);
					if ( Attribs & BT_Texture1 )
						StreamData(FGL::FTextureUV( (U-csLightPan.UPan)*csLightScale.UScale, (V-csLightPan.VPan)*csLightScale.VScale), Stream);
					if ( Attribs & BT_Texture2 )
						StreamData(FGL::FTextureUV( (U-csFogPan.UPan)*csFogScale.UScale, (V-csFogPan.VPan)*csFogScale.VScale), Stream);
					StreamData(FGL::uint8x4(TextureLayers), Stream);
					if ( Attribs & (BT_Texture1|BT_Texture2) )
						StreamData(FGL::uint8x4(LightInfo), Stream);
					if ( Attribs & BT_Texture0 )
					{
						StreamData(FGL::FTexturePan(*Surface.Texture), Stream);
						StreamData(FGL::uint16x2((GLushort)csTextureScaleIndex, 0), Stream);
					}
				}
			}
			Complex.StreamEnd(Stream);
		}
	}
}

static void BufferComplexSurfaceAlt( FGL::DrawBuffer::FComplexAltGLSL3& Complex, FSurfaceInfo& Surface, FSurfaceFacet& Facet)
{
	using namespace FGL::DrawBuffer;

	// Polygon(s) will be texture mapped
	csUDot = Facet.MapCoords.XAxis | Facet.MapCoords.Origin;
	csVDot = Facet.MapCoords.YAxis | Facet.MapCoords.Origin;
	TextureLayers.Data[0] = UOpenGLRenderDevice::DrawBuffer.Draw.Layers[0];
	TextureLayers.Data[1] = UOpenGLRenderDevice::DrawBuffer.Draw.Layers[1];
	TextureLayers.Data[2] = UOpenGLRenderDevice::DrawBuffer.Draw.Layers[2];
	LightInfo.Data[0] = (BYTE) GetZoneNumber(Surface);
	LightInfo.Data[1] = UOpenGLRenderDevice::DrawBuffer.Draw.Layers[TMU_LightMap];
	LightInfo.Data[2] = UOpenGLRenderDevice::DrawBuffer.Draw.Layers[TMU_FogMap];
	BYTE IsLightAtlas = Surface.LightMap ? FStaticBspInfoBase::IsAtlasID(Surface.LightMap->CacheID) : 0;
	BYTE IsFogAtlas   = Surface.FogMap   ? FStaticBspInfoBase::IsAtlasID(Surface.FogMap->CacheID)   : 0;
	LightInfo.Data[3] = IsLightAtlas | (IsFogAtlas << 1);

	constexpr DWORD Stride = sizeof(FComplexAltGLSL3::VertexType)
		+ sizeof(FComplexAltGLSL3::Texture0Type)
		+ sizeof(FComplexAltGLSL3::TextureLayersType)
		+ sizeof(FComplexAltGLSL3::LightInfoType)
		+ sizeof(FComplexAltGLSL3::VertexParam0Type)
		+ sizeof(FComplexAltGLSL3::VertexParam1Type);
	if ( GL_DEV && (Stride != Complex.BufferStride) )
		appErrorf( TEXT("Stride inconsistent: %i / %i"), Stride, Complex.BufferStride);

	for ( FSavedPoly* Poly=Facet.Polys; Poly; Poly=Poly->Next)
	{
		INT NumPts = Poly->NumPts;
		if ( NumPts > 2 )
		{
			void* Stream;
			Complex.StreamStart(Stream);
			for ( INT v=0; v<NumPts; v++)
			{
				FVector& Point = Poly->Pts[v]->Point;
				StreamData(FGL::FVertex3D(Point), Stream);

				FLOAT U = (Facet.MapCoords.XAxis | Point) - csUDot;
				FLOAT V = (Facet.MapCoords.YAxis | Point) - csVDot;
				StreamData(FGL::FTextureUV(U, V), Stream);
				StreamData(FGL::uint8x4(TextureLayers), Stream);
				StreamData(FGL::uint8x4(LightInfo), Stream);
				StreamData(FGL::FTexturePan(*Surface.Texture), Stream);
				StreamData(FGL::uint16x2((GLushort)csTextureScaleIndex, (GLushort)Poly->iNode), Stream);
			}
			Complex.StreamEnd(Stream);
		}
	}
}

static void CountVertsAndTris( const FSurfaceFacet& Facet, DWORD& OutVerts, DWORD& OutTris)
{
	DWORD Verts = 0;
	DWORD Tris  = 0;

	for ( FSavedPoly* Poly=Facet.Polys; Poly; Poly=Poly->Next)
	{
		INT NumPts = Poly->NumPts;
		if ( NumPts > 2 )
		{
			Verts += NumPts;
			Tris  += (NumPts - 2);
		}
	}

	OutVerts = Verts;
	OutTris  = Tris;
}

static void BufferComplexSurfaceIndices( FGL::VertexBuffer::CopyToVBO* IndexBuffer, INT PointFirst, FSurfaceFacet& Facet)
{
	GL_DEV_CHECK(IndexBuffer);
	DWORD* IndexData = (DWORD*)&IndexBuffer->Data[IndexBuffer->Position];

	for ( FSavedPoly* Poly=Facet.Polys; Poly; Poly=Poly->Next)
	{
		INT NumPts = Poly->NumPts;
		if ( NumPts > 2 )
		{
			for ( INT v=2; v<NumPts; v++)
			{
				*IndexData++ = PointFirst;
				*IndexData++ = PointFirst + v - 1;
				*IndexData++ = PointFirst + v;
			}
			PointFirst += NumPts;
		}
	}
	IndexBuffer->Position = (DWORD)(PTRINT)((BYTE*)IndexData - IndexBuffer->Data);
	csIndexCount  = IndexBuffer->Position / sizeof(GLuint);
}

//
// Buffer DrawGouraudPolygon geometry
//
template <FGouraudVertexData::EBufferPath BufferPath> void BufferGouraudPolygon( FGL::DrawBuffer::FGouraudGLSL3& Gouraud, FTransTexture** Pts, INT NumPts)
{
	using namespace FGL::DrawBuffer;

	TextureLayers.Data[0] = UOpenGLRenderDevice::DrawBuffer.Draw.Layers[0];
	TextureLayers.Data[1] = UOpenGLRenderDevice::DrawBuffer.Draw.Layers[1];

	// Buffer triangle(s)
	FGouraudVertexData* Stream;
	Gouraud.StreamStart(*(void**)&Stream);
	FGouraudVertexData* StreamFirst = Stream;
	for ( INT j=0; j<NumPts; j++)
	{
		// Starting from point 4, copy 0 and previous
		if ( j >= 3 )
		{
#if USES_TRAWDATA
			TRawData<FGouraudVertexData>::Copy(*(Stream)  , *(StreamFirst));
			TRawData<FGouraudVertexData>::Copy(*(Stream+1), *(Stream-1));
#else
			*(Stream)   = *(StreamFirst);
			*(Stream+1) = *(Stream-1);
#endif
			Stream += 2;
		}

		FTransTexture& P = *Pts[j];
		Stream->Vertex              = FGL::FVertex3D(P.Point);
		if ( BufferPath >= FGouraudVertexData::BP_Light )
			Stream->Color0          = FGL::FColorRGBA(FPlaneTo_RGB_A255(&P.Light));
		else
			Stream->Color0          = 0xFFFFFFFF;
		if ( BufferPath >= FGouraudVertexData::BP_LightFog )
			Stream->Color1          = FGL::FColorRGBA(FPlaneTo_RGBA(&P.Fog));
		else
			Stream->Color1          = 0x00000000;
		Stream->TextureUV           = FGL::FTextureUV(P.U, P.V); // Do not multiply, by UMult, VMult | do that in shader.
		Stream->TextureLayers       = FGL::uint8x4(TextureLayers);
		Stream->TextureScaleIndices = FGL::uint16x2((GLushort)csTextureScaleIndex, 0);
		Stream++;
	}
	Gouraud.StreamEnd(Stream);
}

//
// Buffer DrawGouraudPolygon geometry - decal case
//
void BufferDecal( FGL::DrawBuffer::FDecalGLSL3& Decal, FTransTexture** Pts, INT NumPts)
{
	using namespace FGL::DrawBuffer;

	TextureLayers.Data[0] = UOpenGLRenderDevice::DrawBuffer.DrawDecals.Layers[0];

	// Color (hack: assumes all vertices have matching colors)
	DWORD Color;
	if ( Decal.PolyFlags & PF_Modulated )
		Color = 0xFFFFFFFF;
	else
		Color = FPlaneTo_RGB_A255(&Pts[0]->Light);

	// Buffer triangle(s)
	void* Stream;
	Decal.StreamStart(Stream);
	void* StreamFirst = Stream;
	for ( INT j=0; j<NumPts; j++)
	{
		// Starting from point 4, copy 0 and previous
		if ( j >= 3 )
		{
			static_assert(FGL::DrawBuffer::FDecalGLSL3::BufferTypes == FGL::DrawBuffer::FDecalGLSL3::ForceAttribs, "Error: DecalGLSL3 does not have all attribs forced");
			constexpr DWORD Stride = FGL::DrawBuffer::FDecalGLSL3::GetConstStride();
#if USES_TRAWDATA
			struct V { BYTE _d[Stride]; };
			TRawData<V>::Copy( *(V*)((BYTE*)Stream)       , *(V*)(StreamFirst)  );
			TRawData<V>::Copy( *(V*)((BYTE*)Stream+Stride), *(V*)((BYTE*)Stream-Stride));
#else
			appMemcpy(Stream, StreamFirst, Stride);
			appMemcpy((BYTE*)Stream+Stride, (BYTE*)Stream-Stride, Stride);
#endif
			(*(BYTE**)&Stream) += Stride+Stride;
		}
		FTransTexture& P = *Pts[j];
		StreamData(FGL::FVertex3D(P.Point), Stream);
		StreamData(FGL::FColorRGBA(Color), Stream);
		StreamData(FGL::FTextureUV(P.U, P.V), Stream); // Do not multiply, by UMult, VMult | do that in shader.
		StreamData(FGL::uint8x4(TextureLayers), Stream);
		StreamData(FGL::uint16x2((GLushort)csTextureScaleIndexDecal, 0), Stream);
	}
	Decal.StreamEnd(Stream);
}

//
// Merges decal draws into main draw command list
//
static inline void MergeDecals()
{
	if ( UOpenGLRenderDevice::DrawBuffer.DrawDecals.First )
	{
		csDecalQueued = 0;

		// !!! this should never happen
		if ( UOpenGLRenderDevice::DrawBuffer.Draw.Last == nullptr )
			Exchange(UOpenGLRenderDevice::DrawBuffer.Draw, UOpenGLRenderDevice::DrawBuffer.DrawDecals);
		else
		{
			UOpenGLRenderDevice::DrawBuffer.Draw.Last->Next = UOpenGLRenderDevice::DrawBuffer.DrawDecals.First;
			UOpenGLRenderDevice::DrawBuffer.Draw.Last = UOpenGLRenderDevice::DrawBuffer.DrawDecals.Last;
			UOpenGLRenderDevice::DrawBuffer.DrawDecals.Reset();
		}
	}
}

//
// Select a Clip Plane index (and register if new)
//
static DWORD GetClipPlaneIndex( const FPlane& __restrict ClipPlane)
{
	for ( DWORD i=0; i<ClipPlaneCount; i++)
		if ( ClipPlanes[i] == ClipPlane )
			return i;

	if ( ClipPlaneCount<ARRAY_COUNT(ClipPlanes) )
	{
		ClipPlanes[ClipPlaneCount] = ClipPlane;
		return ClipPlaneCount++;
	}

	return 0;
}
static void ResetClipPlanes()
{
	ClipPlaneCount = 1;
	ClipPlanes[0] = FPlane(0,0,0,0);
}


/*-----------------------------------------------------------------------------
	Render path procs.
-----------------------------------------------------------------------------*/

void UOpenGLRenderDevice::DrawComplexSurface_GLSL3( FSceneNode* Frame, FSurfaceInfo_DrawComplex& Surface, FSurfaceFacet& Facet)
{
	check(Surface.Texture); // TODO: Draw Textureless surf as a simple ZBuffer write (fixes surfs bleeding into skybox)

	using namespace FGL::DrawBuffer;
	using namespace FGL::Draw;

	// Push decal draw queue into main queue if draw order is important
	if ( Surface.PolyFlags & PF_NoOcclude )
		MergeDecals();

#if USES_STATIC_BSP
	if ( Surface.LightMap && CanBufferStaticComplexSurfaceGeometry_VBO(Surface, Facet) )
	{
		// Buffered DrawComplex mode using static lightmap info
		//
		// The buffer precalculates and sends the following info to the vertex shader inputs:
		// - InVertex:        Vertex list - transformed to UT screen space
		// - InTexCoords0:    SurfaceU, SurfaceV (unscaled)
		// - InTextureLayers: Texture layers for [base, detail, macro, ?]
		// - InLightInfo:     ZoneIndex + Texture layers for [light, fog] + AtlasBits for light, fog
		// - InTexturePan:    PanU, PanV
		// - InUniformIndex:  Index to UBOs [texture scale, BSP node index]
		// 
		TextureList TexList;
		if ( !DrawBuffer.Draw.MergeWithLast(Surface, &UOpenGLRenderDevice::ExecDraw_ComplexSurfaceALT_GLSL3, DetailTextures, TexList) )
		{
			// Prepare shared buffer for new vertex format
			auto* NewDraw = DrawBuffer.Draw.Last;
			DWORD ForceAttribs = 0;
			if ( csDecalQueued )
				ForceAttribs |= BT_SpecialFlag;
			COMPLEXSTATIC.Setup(NewDraw, ForceAttribs);
			// COMPLEXSTATIC.AdjustToStride();

			// Set primitive info
			// PrimitiveStart = Starting index in Index Array
			NewDraw->PrimitiveStart = csIndexCount;
			NewDraw->PrimitiveCount = 0 * 3;

		}
		auto& ComplexStatic = COMPLEXSTATIC;

		// Adjust buffer offset based on stride and count Verts, Tris
		INT PointFirst = ComplexStatic.AdjustToStride();

		DWORD Verts, Tris;
		CountVertsAndTris(Facet, Verts, Tris);

		if ( ComplexStatic.AtCapacity(Verts) || ComplexStatic.IndexAtCapacity(Tris*3, sizeof(DWORD)) )
		{
			// Do not attempt to recover, simply move this complex surface to next batch
			ExecDraws_GLSL3();
			DrawComplexSurface_GLSL3( Frame, Surface, Facet);
			return; 
		}
		DrawBuffer.Draw.Last->PrimitiveCount += Tris * 3;

		// Select index in csTextureScale uniform (only if a lookup was done)
		if ( TexList.Units )
			csTextureScaleIndex = csGetTextureScaleIndexUBO(TexList);

		BufferComplexSurfaceAlt(ComplexStatic, Surface, Facet);
		BufferComplexSurfaceIndices(ComplexStatic.GetIndexBuffer(), PointFirst, Facet);
	}
	else
#endif
	{
		// Buffered DrawComplex mode
		//
		// The buffer precalculates and sends the following info to the vertex shader inputs:
		// - InVertex:        Vertex list - transformed to UT screen space
		// - InTexCoords0:    SurfaceU, SurfaceV (unscaled)
		// - InTexCoords1:    LightU, LightV
		// - InTexCoords2:    FogU, FogV
		// - InTextureLayers: Texture layers for [base, detail, macro, ?]
		// - InLightInfo:     ZoneIndex + Texture layers for [light, fog]
		// - InTexturePan:    PanU, PanV
		// - InUniformIndex:  Index to UBOs [texture scale, ?]
		// 
		TextureList TexList;
		if ( !DrawBuffer.Draw.MergeWithLast(Surface, &UOpenGLRenderDevice::ExecDraw_ComplexSurface_GLSL3, DetailTextures, TexList) )
		{
			// Prepare shared buffer for new vertex format
			auto* NewDraw = DrawBuffer.Draw.Last;
			DWORD ForceAttribs = 0;
			if ( csDecalQueued )        ForceAttribs |= BT_SpecialFlag;
			if ( GIsEditor )            ForceAttribs |= BT_Color0;
			if ( NewDraw->CacheID_Fog ) ForceAttribs |= BT_Texture1;
			COMPLEX.Setup(NewDraw, ForceAttribs);
			// COMPLEX.AdjustToStride();

			// Set primitive info
			// PrimitiveStart = Starting index in Index Array
			NewDraw->PrimitiveStart = csIndexCount;
			NewDraw->PrimitiveCount = 0 * 3;
		}
		auto& Complex = COMPLEX;

		// Adjust buffer offset based on stride and count Verts, Tris
		INT PointFirst = Complex.AdjustToStride();

		DWORD Verts, Tris;
		CountVertsAndTris(Facet, Verts, Tris);

		if ( Complex.AtCapacity(Verts) || Complex.IndexAtCapacity(Tris*3, sizeof(DWORD)) )
		{
			// Do not attempt to recover, simply move this complex surface to next batch
			ExecDraws_GLSL3();
			DrawComplexSurface_GLSL3( Frame, Surface, Facet);
			return; 
		}
		DrawBuffer.Draw.Last->PrimitiveCount += Tris * 3;

		// Select index in csTextureScale uniform (only if a lookup was done)
		if ( TexList.Units )
			csTextureScaleIndex = csGetTextureScaleIndexUBO(TexList);

		// Select a streaming path (precompiled)
		void (*StreamPath)(FGL::DrawBuffer::FComplexGLSL3&,FSurfaceInfo&,FSurfaceFacet&) = nullptr;
		DWORD Attribs = DrawBuffer.Draw.Last->Attribs & (BT_Vertex|BT_Color0|BT_Texture0|BT_Texture1|BT_Texture2);
		switch ( Attribs )
		{
			#define	CASE(bt) case(bt): StreamPath = BufferComplexSurface<bt>; break
			CASE(BT_Vertex);
			CASE(BT_Vertex|BT_Texture0);
			CASE(BT_Vertex|BT_Texture0|BT_Texture1);
			CASE(BT_Vertex|BT_Texture0|BT_Texture1|BT_Texture2);
			// Note: color paths are used in editor
			// Color uses 4 bytes per vertex, consider streaming all the time
			CASE(BT_Vertex|BT_Color0);
			CASE(BT_Vertex|BT_Color0|BT_Texture0);
			CASE(BT_Vertex|BT_Color0|BT_Texture0|BT_Texture1);
			CASE(BT_Vertex|BT_Color0|BT_Texture0|BT_Texture1|BT_Texture2);
			#undef CASE
		}
		verify(StreamPath);
		(*StreamPath)(COMPLEX, Surface, Facet);
		BufferComplexSurfaceIndices(Complex.GetIndexBuffer(), PointFirst, Facet);
	}
}

void UOpenGLRenderDevice::DrawGouraudPolygon_GLSL3( FSceneNode* Frame, FSurfaceInfo_DrawGouraud& Surface, FTransTexture** Pts, INT NumPts)
{
	using namespace FGL::DrawBuffer;
	using namespace FGL::Draw;

	INT NumVerts = (NumPts-2) * 3;
	Surface.PolyFlags &= ~(PF_Unlit);

	// Special decal path, color buffering done for forward compatibility with alpha decals
	if ( ((Surface.PolyFlags & PF_RenderFog) || !m_DGT) && (Surface.PolyFlags & (PF_Modulated|PF_AlphaBlend)) )
	{
		DWORD DecalPolyFlags = (Surface.PolyFlags & ~(PF_RenderFog)) | PF_Gouraud;
		Exchange(Surface.PolyFlags, DecalPolyFlags);
		Surface.DetailTexture = nullptr;

		csDecalQueued = 1;
		auto& LastDrawFunc = DrawBuffer.Draw.Last->Draw;
		if ( LastDrawFunc == &UOpenGLRenderDevice::ExecDraw_ComplexSurface_GLSL3
			|| LastDrawFunc == &UOpenGLRenderDevice::ExecDraw_ComplexSurfaceALT_GLSL3 )
			DrawBuffer.Draw.Last->Attribs |= BT_SpecialFlag;

		TextureList TexList;
		if ( !DrawBuffer.DrawDecals.MergeWithLast(Surface, &UOpenGLRenderDevice::ExecDraw_Decal_GLSL3, false, TexList) )
		{
			// Prepare shared buffer for new vertex format
			auto* NewDraw = DrawBuffer.DrawDecals.Last;
			DECAL.Setup(NewDraw);

			// Set primitive info
			NewDraw->PrimitiveStart = DECAL.AdjustToStride();
			NewDraw->PrimitiveCount = 0 * 3;
		}
		auto& Decal = DECAL;

		if ( Decal.AtCapacity(NumVerts) )
		{
			// Do not attempt to recover, simply move this gouraud polygon to next batch
			ExecDraws_GLSL3();
			Exchange(Surface.PolyFlags, DecalPolyFlags);
			DrawGouraudPolygon_GLSL3( Frame, Surface, Pts, NumPts);
			return; 
		}

		// Select index in csTextureScale uniform (only if a lookup was done)
		if ( TexList.Units )
			csTextureScaleIndexDecal = csGetTextureScaleIndexUBO(TexList);

		DrawBuffer.DrawDecals.Last->PrimitiveCount += NumVerts;

		// Select a streaming path (precompiled)
		BufferDecal(Decal, Pts, NumPts);
		return;
	}

	// Push decal draw queue into main queue if draw order is important
	if ( Surface.PolyFlags & PF_NoOcclude )
		MergeDecals();

	TextureList TexList;
	if ( !DrawBuffer.Draw.MergeWithLast(Surface, &UOpenGLRenderDevice::ExecDraw_GouraudTriangles_GLSL3, DetailTextures, TexList) )
	{
		// Prepare shared buffer for new vertex format
		auto* NewDraw = DrawBuffer.Draw.Last;
		GOURAUD.Setup(NewDraw);

		// Set primitive info
		NewDraw->PrimitiveStart = GOURAUD.AdjustToStride();
		NewDraw->PrimitiveCount = 0 * 3;
	}
	auto& Gouraud = GOURAUD;

	if ( Gouraud.AtCapacity(NumVerts) )
	{
		// Do not attempt to recover, simply move this gouraud polygon to next batch
		ExecDraws_GLSL3();
		DrawGouraudPolygon_GLSL3( Frame, Surface, Pts, NumPts);
		return; 
	}

	// Select index in csTextureScale uniform (only if a lookup was done)
	if ( TexList.Units )
		csTextureScaleIndex = csGetTextureScaleIndexUBO(TexList);

	DrawBuffer.Draw.Last->PrimitiveCount += NumVerts;

	// Select a streaming path (precompiled)
	void (*StreamPath)(FGL::DrawBuffer::FGouraudGLSL3&,FTransTexture**,INT) = nullptr;
	if ( Surface.PolyFlags & PF_Unlit )
		StreamPath = BufferGouraudPolygon<FGouraudVertexData::BP_Unlit>;
	else if ( !(Surface.PolyFlags & PF_RenderFog) )
		StreamPath = BufferGouraudPolygon<FGouraudVertexData::BP_Light>;
	else
		StreamPath = BufferGouraudPolygon<FGouraudVertexData::BP_LightFog>;
	(*StreamPath)(Gouraud, Pts, NumPts);
}

void UOpenGLRenderDevice::UOpenGLRenderDevice::DrawTile_GLSL3( FSceneNode* Frame, FSurfaceInfo_DrawTile& Surface, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, FLOAT Z)
{
	using namespace FGL::DrawBuffer;
	using namespace FGL::Draw;

	GL_DEV_CHECK(Surface.Texture);

	// Full draw if buffer is full
	if ( TileCount >= QUAD.MaxTileCount )
		ExecDraws_GLSL3();

	// Hack to modify transparency of HUD elements
	if ( (Surface.PolyFlags & PF_Translucent) && m_shaderAlphaHUD )
	{
		Surface.PolyFlags &= ~(PF_Translucent|PF_Modulated|PF_AlphaBlend);
		Surface.PolyFlags |= (PF_Highlighted|PF_AlphaHack);
	}

	// Push decal draw queue into main queue if draw order is important
	if ( Surface.PolyFlags & PF_NoOcclude )
		MergeDecals();

	TextureList TexList;
	if ( !DrawBuffer.Draw.MergeWithLast(Surface, &UOpenGLRenderDevice::ExecDraw_Quad_GLSL3, false, TexList) )
	{
		// Prepare shared buffer for new vertex format
		auto* NewDraw = DrawBuffer.Draw.Last;
		QUAD.Setup(NewDraw);

		// Set primitive info
		NewDraw->PrimitiveStart = TileCount * 6;
		NewDraw->PrimitiveCount = 0;
	}
	DrawBuffer.Draw.Last->PrimitiveCount += 6;
	TileCount++;

	TextureLayers.Data[0] = UOpenGLRenderDevice::DrawBuffer.Draw.Layers[0];

	// Stream the quad
	FLOAT PX1 = X - Frame->FX2;
	FLOAT PX2 = PX1 + XL;
	FLOAT PY1 = Y - Frame->FY2;
	FLOAT PY2 = PY1 + YL;

	FLOAT UMult = 1.0f / (Surface.Texture->UScale);
	FLOAT VMult = 1.0f / (Surface.Texture->VScale);
	FLOAT SU1 = (U)      * UMult;
	FLOAT SU2 = (U + UL) * UMult;
	FLOAT SV1 = (V)      * VMult;
	FLOAT SV2 = (V + VL) * VMult;

	DWORD dwColor = GET_COLOR_DWORD(Surface.Color);
	if ( !(Surface.PolyFlags & PF_AlphaBlend) )
		dwColor |= COLOR_NO_ALPHA;

	void* Stream;

	QUAD.StreamStart(Stream);
	StreamData(FGL::FVertex3D( PX1, PY1, Z), Stream); // 1
	StreamData(FGL::FColorRGBA( dwColor), Stream);
	StreamData(FGL::FTextureUV( SU1, SV1), Stream);
	StreamData(FGL::uint8x4(TextureLayers), Stream);

	StreamData(FGL::FVertex3D( PX2, PY1, Z), Stream); // 2
	StreamData(FGL::FColorRGBA( dwColor), Stream);
	StreamData(FGL::FTextureUV( SU2, SV1), Stream);
	StreamData(FGL::uint8x4(TextureLayers), Stream);

	StreamData(FGL::FVertex3D( PX2, PY2, Z), Stream); // 3
	StreamData(FGL::FColorRGBA( dwColor), Stream);
	StreamData(FGL::FTextureUV( SU2, SV2), Stream);
	StreamData(FGL::uint8x4(TextureLayers), Stream);


	StreamData(FGL::FVertex3D( PX2, PY2, Z), Stream); // 3
	StreamData(FGL::FColorRGBA( dwColor), Stream);
	StreamData(FGL::FTextureUV( SU2, SV2), Stream);
	StreamData(FGL::uint8x4(TextureLayers), Stream);

	StreamData(FGL::FVertex3D( PX1, PY2, Z), Stream); // 4
	StreamData(FGL::FColorRGBA( dwColor), Stream);
	StreamData(FGL::FTextureUV( SU1, SV2), Stream);
	StreamData(FGL::uint8x4(TextureLayers), Stream);

	StreamData(FGL::FVertex3D( PX1, PY1, Z), Stream); // 1
	StreamData(FGL::FColorRGBA( dwColor), Stream);
	StreamData(FGL::FTextureUV( SU1, SV1), Stream);
	StreamData(FGL::uint8x4(TextureLayers), Stream);
	QUAD.StreamEnd(Stream);
}

void UOpenGLRenderDevice::DrawTileCompute_GLSL3( FSceneNode* Frame, FSurfaceInfo_DrawTile& Surface, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, FLOAT Z)
{
	using namespace FGL::DrawBuffer;
	using namespace FGL::Draw;

	GL_DEV_CHECK(Surface.Texture);

	// Full draw if buffer is full
	if ( TileCount >= QUAD.MaxTileCount )
		ExecDraws_GLSL3();

	// Hack to modify transparency of HUD elements
	if ( (Surface.PolyFlags & PF_Translucent) && m_shaderAlphaHUD )
	{
		Surface.PolyFlags &= ~(PF_Translucent|PF_Modulated|PF_AlphaBlend);
		Surface.PolyFlags |= (PF_Highlighted|PF_AlphaHack);
	}

	// Push decal draw queue into main queue if draw order is important
	if ( Surface.PolyFlags & PF_NoOcclude )
		MergeDecals();

	TextureList TexList;
	if ( !DrawBuffer.Draw.MergeWithLast(Surface, &UOpenGLRenderDevice::ExecDraw_Quad_GLSL3, false, TexList) )
	{
		// Prepare shared buffer for new vertex format
		auto* NewDraw = DrawBuffer.Draw.Last;
		QUAD.Setup(NewDraw);

		// Set primitive info
		NewDraw->PrimitiveStart = TileCount * 6; // Stride never changes
		NewDraw->PrimitiveCount = 0;
	}
	DrawBuffer.Draw.Last->PrimitiveCount += 6;

	TextureLayers.Data[0] = UOpenGLRenderDevice::DrawBuffer.Draw.Layers[0];

	DWORD dwColor = GET_COLOR_DWORD(Surface.Color);
	if ( !(Surface.PolyFlags & PF_AlphaBlend) )
		dwColor |= COLOR_NO_ALPHA;

	// Stream quad to smaller buffer for later expansion
	struct FCompressedQuad
	{
		FLOAT X, Y, XL, YL;
		FLOAT U, V, UL, VL;
		FLOAT Z;
		FLOAT UVScale;
		DWORD PackedColor;
		DWORD PackedLayers;
	};
	FCompressedQuad* Quad = (FCompressedQuad*)&BUFFER_QUADEXPANSION.Data[TileCount * sizeof(FCompressedQuad)];
	TileCount++;
	Quad->X = X - Frame->FX2;
	Quad->Y = Y - Frame->FY2;
	Quad->XL = XL;
	Quad->YL = YL;
	Quad->U = U;
	Quad->V = V;
	Quad->UL = UL;
	Quad->VL = VL;
	Quad->Z = Z;
	Quad->UVScale = Surface.Texture->UScale;
	Quad->PackedColor = dwColor;
	Quad->PackedLayers = *(DWORD*)TextureLayers.Data;

	QUAD.GetStreamBuffer()->Position += QUAD.GetConstStride() * 6;
}

void UOpenGLRenderDevice::Draw3DLine_GLSL3( FSceneNode* Frame, FPlane& Color, DWORD LineFlags, FVector P1, FVector P2)
{
	using namespace FGL::Draw;

	// Account for possible stride adjustment by checking an extra line
	if ( LINE.AtCapacity(3) )
		ExecDraws_GLSL3();

	// Software transform
	// Needs to be done early because this may defer to Draw2DPoint
	P1 = P1.TransformPointBy(Frame->Coords);
	P2 = P2.TransformPointBy(Frame->Coords);
	if ( Frame->Viewport->IsOrtho() )
	{
		// Zoom.
		FLOAT rcpZoom = 1.0f / Frame->Zoom;
		FLOAT Pz1X = P1.X * rcpZoom;
		FLOAT Pz1Y = P1.Y * rcpZoom;
		FLOAT Pz2X = P2.X * rcpZoom;
		FLOAT Pz2Y = P2.Y * rcpZoom;

		// Draw small square instead (if line will be displayed as a dot)
		if ( Abs(Pz2X-Pz1X) + Abs(Pz2Y-Pz2X) < 0.2f )
		{
			Pz1X += Frame->FX2;
			Pz1Y += Frame->FY2;
			Draw2DPoint_GLSL3( Frame, Color, LineFlags, Pz1X - 1.0f, Pz1Y - 1.0f, Pz1X + 1.0f, Pz1Y + 1.0f, 1.0f);
			return;
		}

		// Project
		P1.X = Pz1X * m_RFX2;
		P1.Y = Pz1Y * m_RFY2;
		P1.Z = 1;
		P2.X = Pz2X * m_RFX2;
		P2.Y = Pz2Y * m_RFY2;
		P2.Z = 1;
	}

	if ( !DrawBuffer.Draw.MergeWithLast(&UOpenGLRenderDevice::ExecDraw_Line_GLSL3, LineFlags) )
	{
		// Push decal draw queue into main queue now
		MergeDecals();

		// Prepare shared buffer for new vertex format
		auto* NewDraw = DrawBuffer.Draw.Last;
		LINE.Setup(NewDraw);

		// Set primitive info
		NewDraw->PrimitiveStart = LINE.AdjustToStride();
		NewDraw->PrimitiveCount = 0 * 2;
	}
	DrawBuffer.Draw.Last->PrimitiveCount += 2;

	DWORD dwColor = FPlaneTo_RGB_A255(&Color);

	void* Stream;
	LINE.StreamStart(Stream);
	StreamData(FGL::FVertex3D(P1), Stream);
	StreamData(FGL::FColorRGBA(dwColor), Stream);
	StreamData(FGL::FVertex3D(P2), Stream);
	StreamData(FGL::FColorRGBA(dwColor), Stream);
	LINE.StreamEnd(Stream);
}

void UOpenGLRenderDevice::Draw2DLine_GLSL3( FSceneNode* Frame, FPlane& Color, DWORD LineFlags, FVector P1, FVector P2)
{
	using namespace FGL::Draw;

	// Account for stride adjustment by checking an extra primitive
	if ( LINE.AtCapacity(2 + 1) )
		ExecDraws_GLSL3();

	if ( !DrawBuffer.Draw.MergeWithLast(&UOpenGLRenderDevice::ExecDraw_Line_GLSL3, LineFlags) )
	{
		// Push decal draw queue into main queue now
		MergeDecals();

		// Prepare shared buffer for new vertex format
		auto* NewDraw = DrawBuffer.Draw.Last;
		LINE.Setup(NewDraw);

		// Set primitive info
		NewDraw->PrimitiveStart = LINE.AdjustToStride();
		NewDraw->PrimitiveCount = 0 * 2;
	}
	DrawBuffer.Draw.Last->PrimitiveCount += 2;

	auto& Line = LINE;

	//Get line coordinates back in 3D
	FLOAT X1Pos = m_RFX2 * (P1.X - Frame->FX2);
	FLOAT Y1Pos = m_RFY2 * (P1.Y - Frame->FY2);
	FLOAT X2Pos = m_RFX2 * (P2.X - Frame->FX2);
	FLOAT Y2Pos = m_RFY2 * (P2.Y - Frame->FY2);
	if ( !Frame->Viewport->IsOrtho() )
	{
		X1Pos *= P1.Z;
		Y1Pos *= P1.Z;
		X2Pos *= P2.Z;
		Y2Pos *= P2.Z;
	}

	DWORD dwColor = FPlaneTo_RGB_A255(&Color);

	void* Stream;
	Line.StreamStart(Stream);
	StreamData(FGL::FVertex3D(X1Pos, Y1Pos, P1.Z), Stream);
	StreamData(FGL::FColorRGBA(dwColor), Stream);
	StreamData(FGL::FVertex3D(X2Pos, Y2Pos, P2.Z), Stream);
	StreamData(FGL::FColorRGBA(dwColor), Stream);
	Line.StreamEnd(Stream);
}

void UOpenGLRenderDevice::Draw2DPoint_GLSL3( FSceneNode* Frame, FPlane& Color, DWORD LineFlags, FLOAT X1, FLOAT Y1, FLOAT X2, FLOAT Y2, FLOAT Z)
{
	using namespace FGL;
	using namespace FGL::Draw;

	// Full draw if buffer is full
	if ( TileCount >= QUAD.MaxTileCount )
		ExecDraws_GLSL3();

	// Setup Surface abstraction
	FSurfaceInfo_2DPoint Surface;
	Surface.PolyFlags = (LineFlags == LINE_DepthCued) ? PF_Highlighted|PF_Occlude : PF_Highlighted|PF_NoZReject;
	Surface.PolyFlags |= PF_Gouraud;

	TextureList TexList;
	if ( !DrawBuffer.Draw.MergeWithLast(Surface, &UOpenGLRenderDevice::ExecDraw_Quad_GLSL3, false, TexList) )
	{
		// Prepare shared buffer for new vertex format
		auto* NewDraw = DrawBuffer.Draw.Last;
		QUAD.Setup(NewDraw);

		// Set primitive info
		NewDraw->PrimitiveStart = TileCount * 6; // Stride never changes
		NewDraw->PrimitiveCount = 0;
	}
	DrawBuffer.Draw.Last->PrimitiveCount += 6;
	TileCount++;

	TextureLayers.Data[0] = UOpenGLRenderDevice::DrawBuffer.Draw.Layers[0];

	// 2D point uses Quad buffer
	DWORD dwColor = FPlaneTo_RGB_A255(&Color);

	//Get point coordinates
	FLOAT X1Pos = X1 - Frame->FX2 - 0.5f;
	FLOAT Y1Pos = Y1 - Frame->FY2 - 0.5f;
	FLOAT X2Pos = X2 - Frame->FX2 + 0.5f;
	FLOAT Y2Pos = Y2 - Frame->FY2 + 0.5f;

	void* Stream;
	QUAD.StreamStart(Stream);
	StreamData(FGL::FVertex3D(X1Pos,Y1Pos,Z), Stream);
	StreamData(FGL::FColorRGBA(dwColor), Stream);
	StreamData(FGL::FTextureUV(0.0,0.0), Stream);
	StreamData(FGL::uint8x4(TextureLayers), Stream);

	StreamData(FGL::FVertex3D(X2Pos,Y1Pos,Z), Stream);
	StreamData(FGL::FColorRGBA(dwColor), Stream);
	StreamData(FGL::FTextureUV(0.0,0.0), Stream);
	StreamData(FGL::uint8x4(TextureLayers), Stream);

	StreamData(FGL::FVertex3D(X2Pos,Y2Pos,Z), Stream);
	StreamData(FGL::FColorRGBA(dwColor), Stream);
	StreamData(FGL::FTextureUV(0.0,0.0), Stream);
	StreamData(FGL::uint8x4(TextureLayers), Stream);

	StreamData(FGL::FVertex3D(X1Pos,Y1Pos,Z), Stream);
	StreamData(FGL::FColorRGBA(dwColor), Stream);
	StreamData(FGL::FTextureUV(0.0,0.0), Stream);
	StreamData(FGL::uint8x4(TextureLayers), Stream);

	StreamData(FGL::FVertex3D(X2Pos,Y2Pos,Z), Stream);
	StreamData(FGL::FColorRGBA(dwColor), Stream);
	StreamData(FGL::FTextureUV(0.0,0.0), Stream);
	StreamData(FGL::uint8x4(TextureLayers), Stream);

	StreamData(FGL::FVertex3D(X1Pos,Y2Pos,Z), Stream);
	StreamData(FGL::FColorRGBA(dwColor), Stream);
	StreamData(FGL::FTextureUV(0.0,0.0), Stream);
	StreamData(FGL::uint8x4(TextureLayers), Stream);
	QUAD.StreamEnd(Stream);
}

void UOpenGLRenderDevice::DrawGouraudTriangles_GLSL3( const FSceneNode* Frame, FSurfaceInfo_DrawGouraudTris& Surface, FTransTexture* const Pts, INT NumPts, DWORD DataFlags)
{
	using namespace FGL::DrawBuffer;
	using namespace FGL::Draw;

	Surface.PolyFlags &= ~(PF_Unlit);

	if ( Frame->NearClip.W != 0.0f )
		Surface.ClipPlaneID = GetClipPlaneIndex(Frame->NearClip);

	TextureList TexList;
	if ( !DrawBuffer.Draw.MergeWithLast(Surface, &UOpenGLRenderDevice::ExecDraw_GouraudTriangles_GLSL3, DetailTextures, TexList) )
	{
		// Prepare shared buffer for new vertex format
		auto* NewDraw = DrawBuffer.Draw.Last;
		GOURAUD.Setup(NewDraw);

		// Set primitive info
		NewDraw->PrimitiveStart = GOURAUD.AdjustToStride();
		NewDraw->PrimitiveCount = 0 * 3;
	}

	auto& Gouraud = GOURAUD;

	if ( TexList.Units )
		csTextureScaleIndex = csGetTextureScaleIndexUBO(TexList);

	TextureLayers.Data[0] = UOpenGLRenderDevice::DrawBuffer.Draw.Layers[0];
	TextureLayers.Data[1] = UOpenGLRenderDevice::DrawBuffer.Draw.Layers[1];

	// Make sure we're using expected attribs:
	DWORD Attribs = DrawBuffer.Draw.Last->Attribs & (BT_Vertex|BT_Color0|BT_Color1|BT_Texture0);
	verify(Attribs == (BT_Vertex|BT_Color0|BT_Color1|BT_Texture0) || Attribs == (BT_Vertex|BT_Color0|BT_Texture0));

	// Setup triangle evaluation state.
	bool Mirror = (Frame->Mirror == -1.f);
	FLOAT EnviroUScale = 0;
	FLOAT EnviroVScale = 0;
	if ( Surface.PolyFlags & PF_Environment )
	{
		EnviroUScale = 0.5f * Surface.Texture->UScale * Surface.Texture->USize;
		EnviroVScale = 0.5f * Surface.Texture->VScale * Surface.Texture->VSize;
	}
	FTransTexture* Tri[3];

	// Buffer and draw as much as possible, split into multiple draws if too many triangles
	// Evaluate each Triangle for buffering.
	DWORD BufferedTris = 0;
	DWORD TriBudget    = Gouraud.GetUnusedVerts() / 3;
	INT i;
	for ( i=0; i<NumPts && BufferedTris<TriBudget; i+=3)
	{
		Tri[0] = Pts + i;
		Tri[1] = Pts + i + 1;
		Tri[2] = Pts + i + 2;

		// If outcoded, skip it.
		if ( Tri[0]->Flags & Tri[1]->Flags & Tri[2]->Flags )
			continue;

		// Flip
		if ( Mirror )
			Exchange( Tri[0], Tri[2]);

		// Backface reject if not two sided, otherwise flip to face camera.
		if ( FTriple( Tri[0]->Point, Tri[1]->Point, Tri[2]->Point) <= 0.0 )
		{
			if ( !(Surface.PolyFlags & PF_TwoSided) )
				continue;
			Exchange( Tri[2], Tri[0]);
		}

		// Environment mapping.
		if ( Surface.PolyFlags & PF_Environment )
		{
			for ( INT j=0; j<3; j++)
			{
				FTransTexture& P = *Tri[j];
				FVector T = P.Point.UnsafeNormal().MirrorByVector(P.Normal).TransformVectorBy(Frame->Uncoords);
				P.U = (T.X + 1.0f) * EnviroUScale;
				P.V = (T.Y + 1.0f) * EnviroVScale;
			}
		}

		// Buffer unclipped triangle
		FGouraudVertexData* Stream;
		Gouraud.StreamStart(*(void**)&Stream);
		for ( INT j=0; j<3; j++)
		{
			FTransTexture& P = *Tri[j];
			Stream->Vertex              = FGL::FVertex3D(P.Point);
			Stream->Color0          = FGL::FColorRGBA(FPlaneTo_RGB_A255(&P.Light));
			if ( Surface.PolyFlags & PF_RenderFog )
				Stream->Color1          = FGL::FColorRGBA(FPlaneTo_RGBA(&P.Fog));
			else
				Stream->Color1          = 0x00000000;
			Stream->TextureUV           = FGL::FTextureUV(P.U, P.V); // Do not multiply, by UMult, VMult | do that in shader.
			Stream->TextureLayers       = FGL::uint8x4(TextureLayers);
			Stream->TextureScaleIndices = FGL::uint16x2((GLushort)csTextureScaleIndex, 0);
			Stream++;
		}
		Gouraud.StreamEnd(Stream);
		BufferedTris++;
	}

	DrawBuffer.Draw.Last->PrimitiveCount += BufferedTris * 3;

	// If tri budget was used up without finishing all triangles
	// Then exec draws and continue on next batch
	if ( i < NumPts )
	{
		ExecDraws_GLSL3();
		DrawGouraudTriangles_GLSL3(Frame, Surface, Pts+i, NumPts-i, DataFlags);
	}
}

void UOpenGLRenderDevice::DrawTileList_GLSL3( const FSceneNode* Frame, FSurfaceInfo_DrawTile& Surface, const FTileRect* TileList, INT NumTiles, FLOAT Z)
{
	using namespace FGL::DrawBuffer;
	using namespace FGL::Draw;

	GL_DEV_CHECK(Surface.Texture);

	// Full draw if buffer is full
	if ( TileCount + NumTiles >= QUAD.MaxTileCount )
		ExecDraws_GLSL3();

	// Hack to modify transparency of HUD elements
	if ( (Surface.PolyFlags & PF_Translucent) && m_shaderAlphaHUD )
	{
		Surface.PolyFlags &= ~(PF_Translucent|PF_Modulated|PF_AlphaBlend);
		Surface.PolyFlags |= (PF_Highlighted|PF_AlphaHack);
	}

	// Push decal draw queue into main queue if draw order is important
	if ( Surface.PolyFlags & PF_NoOcclude )
		MergeDecals();

	TextureList TexList;
	if ( !DrawBuffer.Draw.MergeWithLast(Surface, &UOpenGLRenderDevice::ExecDraw_Quad_GLSL3, false, TexList) )
	{
		// Prepare shared buffer for new vertex format
		auto* NewDraw = DrawBuffer.Draw.Last;
		QUAD.Setup(NewDraw);

		// Set primitive info
		NewDraw->PrimitiveStart = TileCount * 6;
		NewDraw->PrimitiveCount = 0;
	}
	DrawBuffer.Draw.Last->PrimitiveCount += 6 * NumTiles;
	TileCount += NumTiles;

	TextureLayers.Data[0] = UOpenGLRenderDevice::DrawBuffer.Draw.Layers[0];

	// Stream the quads
	DWORD dwColor = GET_COLOR_DWORD(Surface.Color);
	if ( !(Surface.PolyFlags & PF_AlphaBlend) )
		dwColor |= COLOR_NO_ALPHA;

	FLOAT UMult = 1.0f / (Surface.Texture->UScale);
	FLOAT VMult = 1.0f / (Surface.Texture->VScale);

	void* Stream;
	for ( INT i=0; i<NumTiles; i++)
	{
		const FTileRect& Tile = TileList[i];

		FLOAT PX1 = Tile.X - Frame->FX2;
		FLOAT PX2 = PX1 + Tile.XL;
		FLOAT PY1 = Tile.Y - Frame->FY2;
		FLOAT PY2 = PY1 + Tile.YL;

		FLOAT SU1 = (Tile.U)           * UMult;
		FLOAT SU2 = (Tile.U + Tile.UL) * UMult;
		FLOAT SV1 = (Tile.V)           * VMult;
		FLOAT SV2 = (Tile.V + Tile.VL) * VMult;

		QUAD.StreamStart(Stream);
		StreamData(FGL::FVertex3D( PX1, PY1, Z), Stream); // 1
		StreamData(FGL::FColorRGBA( dwColor), Stream);
		StreamData(FGL::FTextureUV( SU1, SV1), Stream);
		StreamData(FGL::uint8x4(TextureLayers), Stream);

		StreamData(FGL::FVertex3D( PX2, PY1, Z), Stream); // 2
		StreamData(FGL::FColorRGBA( dwColor), Stream);
		StreamData(FGL::FTextureUV( SU2, SV1), Stream);
		StreamData(FGL::uint8x4(TextureLayers), Stream);

		StreamData(FGL::FVertex3D( PX2, PY2, Z), Stream); // 3
		StreamData(FGL::FColorRGBA( dwColor), Stream);
		StreamData(FGL::FTextureUV( SU2, SV2), Stream);
		StreamData(FGL::uint8x4(TextureLayers), Stream);


		StreamData(FGL::FVertex3D( PX2, PY2, Z), Stream); // 3
		StreamData(FGL::FColorRGBA( dwColor), Stream);
		StreamData(FGL::FTextureUV( SU2, SV2), Stream);
		StreamData(FGL::uint8x4(TextureLayers), Stream);

		StreamData(FGL::FVertex3D( PX1, PY2, Z), Stream); // 4
		StreamData(FGL::FColorRGBA( dwColor), Stream);
		StreamData(FGL::FTextureUV( SU1, SV2), Stream);
		StreamData(FGL::uint8x4(TextureLayers), Stream);

		StreamData(FGL::FVertex3D( PX1, PY1, Z), Stream); // 1
		StreamData(FGL::FColorRGBA( dwColor), Stream);
		StreamData(FGL::FTextureUV( SU1, SV1), Stream);
		StreamData(FGL::uint8x4(TextureLayers), Stream);
		QUAD.StreamEnd(Stream);
	}
}

void UOpenGLRenderDevice::DrawTileListCompute_GLSL3( const FSceneNode* Frame, FSurfaceInfo_DrawTile& Surface, const FTileRect* TileList, INT NumTiles, FLOAT Z)
{
	using namespace FGL::DrawBuffer;
	using namespace FGL::Draw;

	GL_DEV_CHECK(Surface.Texture);

	// Full draw if buffer is full
	if ( TileCount + NumTiles >= QUAD.MaxTileCount )
		ExecDraws_GLSL3();

	// Hack to modify transparency of HUD elements
	if ( (Surface.PolyFlags & PF_Translucent) && m_shaderAlphaHUD )
	{
		Surface.PolyFlags &= ~(PF_Translucent|PF_Modulated|PF_AlphaBlend);
		Surface.PolyFlags |= (PF_Highlighted|PF_AlphaHack);
	}

	// Push decal draw queue into main queue if draw order is important
	if ( Surface.PolyFlags & PF_NoOcclude )
		MergeDecals();

	TextureList TexList;
	if ( !DrawBuffer.Draw.MergeWithLast(Surface, &UOpenGLRenderDevice::ExecDraw_Quad_GLSL3, false, TexList) )
	{
		// Prepare shared buffer for new vertex format
		auto* NewDraw = DrawBuffer.Draw.Last;
		QUAD.Setup(NewDraw);

		// Set primitive info
		NewDraw->PrimitiveStart = TileCount * 6; // Stride never changes
		NewDraw->PrimitiveCount = 0;
	}
	DrawBuffer.Draw.Last->PrimitiveCount += 6 * NumTiles;

	TextureLayers.Data[0] = UOpenGLRenderDevice::DrawBuffer.Draw.Layers[0];

	DWORD dwColor = GET_COLOR_DWORD(Surface.Color);
	if ( !(Surface.PolyFlags & PF_AlphaBlend) )
		dwColor |= COLOR_NO_ALPHA;

	// Stream quad to smaller buffer for later expansion
	struct FCompressedQuad
	{
		FLOAT X, Y, XL, YL;
		FLOAT U, V, UL, VL;
		FLOAT Z;
		FLOAT UVScale;
		DWORD PackedColor;
		DWORD PackedLayers;
	};
	static_assert(sizeof(FCompressedQuad) == 48, "Fix FCompressedQuad struct");

	FCompressedQuad* Quad = (FCompressedQuad*)&BUFFER_QUADEXPANSION.Data[TileCount * sizeof(FCompressedQuad)];
	for ( INT i=0; i<NumTiles; i++)
	{
		const FTileRect& Tile = TileList[i];
		Quad->X = Tile.X - Frame->FX2;
		Quad->Y = Tile.Y - Frame->FY2;
		Quad->XL = Tile.XL;
		Quad->YL = Tile.YL;
		Quad->U = Tile.U;
		Quad->V = Tile.V;
		Quad->UL = Tile.UL;
		Quad->VL = Tile.VL;
		Quad->Z = Z;
		Quad->UVScale = Surface.Texture->UScale;
		Quad->PackedColor = dwColor;
		Quad->PackedLayers = *(DWORD*)TextureLayers.Data;
		Quad++;
	}

	TileCount += NumTiles;
	QUAD.GetStreamBuffer()->Position += QUAD.GetConstStride() * NumTiles * 6;
}


void UOpenGLRenderDevice::SetSceneNode_GLSL3( FSceneNode* Frame)
{
	// Set viewport.
	GL3->SetViewport(Frame->XB, Viewport->SizeY - Frame->Y - Frame->YB, Frame->X, Frame->Y);

	// Set projection matrix
	bool UpdateGlobalRender = false;
	DWORD DesiredMode = Frame->Viewport->IsOrtho() ? MATRIX_OrthoProjection : MATRIX_Projection;
	if ( (SceneParams.Mode != DesiredMode) || (SceneParams.FovAngle != Viewport->Actor->FovAngle) 
		|| (SceneParams.FX != Frame->FX) || (SceneParams.FY != Frame->FY) )
	{
		SceneParams.Mode = DesiredMode;
		SceneParams.FovAngle = Viewport->Actor->FovAngle;
		SceneParams.FX = Frame->FX;
		SceneParams.FY = Frame->FY;

		GlobalRenderData.ModelViewMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, -1.0f, -1.0f));
		if ( DesiredMode == MATRIX_OrthoProjection )
		{
			GlobalRenderData.ProjectionMatrix = glm::ortho(-m_RProjZ, +m_RProjZ, -m_Aspect*m_RProjZ, +m_Aspect*m_RProjZ, zNear, zFar)
				* GlobalRenderData.ModelViewMatrix;
		}
		else
		{
			GlobalRenderData.ProjectionMatrix = glm::frustum(-m_RProjZ*zNear, +m_RProjZ*zNear, -m_Aspect*m_RProjZ*zNear, +m_Aspect*m_RProjZ*zNear, zNear, zFar)
				* GlobalRenderData.ModelViewMatrix;
		}

		GlobalRenderData.TileVector = FPlane(m_RFX2, m_RFY2, 1.0f, (FLOAT)(DesiredMode == MATRIX_OrthoProjection));
		UpdateGlobalRender = true;
	}

	// Set world coordinates
	if ( *(FVector*)&GlobalRenderData.WorldCoordsOrigin != Frame->Coords.Origin
		|| *(FVector*)&GlobalRenderData.WorldCoordsXAxis != Frame->Coords.XAxis
		|| *(FVector*)&GlobalRenderData.WorldCoordsYAxis != Frame->Coords.YAxis
		|| *(FVector*)&GlobalRenderData.WorldCoordsZAxis != Frame->Coords.ZAxis )
	{
		GlobalRenderData.WorldCoordsOrigin = Frame->Coords.Origin;
		GlobalRenderData.WorldCoordsXAxis = Frame->Coords.XAxis;
		GlobalRenderData.WorldCoordsYAxis = Frame->Coords.YAxis;
		GlobalRenderData.WorldCoordsZAxis = Frame->Coords.ZAxis;
		UpdateGlobalRender = true;
	}

	if ( UpdateGlobalRender )
	{
		FOpenGLBase::glBindBuffer(GL_UNIFORM_BUFFER, bufferId_GlobalRenderUBO);
		GlobalRenderData.BufferAll();
		FOpenGLBase::glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}
}

void UOpenGLRenderDevice::FillScreen_GLSL3( const FOpenGLTexture* Texture, const FPlane* Color, DWORD PolyFlags)
{
	guard(FillScreen_GLSL3);

	ExecDraws_GLSL3();

	// Check static VBO
	if ( !bufferId_StaticFillScreenVBO ) // VBO is not ready
		return;

	auto& Fill = FILL;
	Fill.ActiveAttribs = FGL::DrawBuffer::BT_Vertex | (Texture ? FGL::DrawBuffer::BT_Texture0 : 0);
	Fill.BufferStride = 0;

	// FillScreen does not wait for new draw calls or Unlock()
	SetBlend(PolyFlags);

	// FillScreen uses a global Color for screen flashes
	FProgramID ProgramID = GetProgramID(PolyFlags) | FProgramID::FromVertexType(FGL::VertexProgram::Identity);
	ProgramID &= ~FGL::Program::AlphaTest;
	if ( Texture ) ProgramID |= FGL::Program::Texture0;
	if ( Color )   ProgramID |= FGL::Program::ColorGlobal;

	if ( Texture )
	{
		GL3->TextureUnits.Bind(*Texture);
	}

	FOpenGL3::FProgramData* ProgramData;
	GL3->SetProgram(ProgramID, (void**)&ProgramData);
	GL3->SetVertexArray(&Fill, bufferId_StaticFillScreenVBO);
	if ( Color && ProgramData )
		GL3->SetUniform(ProgramData->ColorGlobal, *Color);

	FOpenGLBase::glDrawArrays( GL_TRIANGLE_FAN, 0, 4);

	unguard;
}

void UOpenGLRenderDevice::BlitFramebuffer_GLSL3()
{
	guard(BlitFramebuffer_GLSL3);
	using namespace FGL::SpecialProgram;

	// Process all pending draws
	ExecDraws_GLSL3();

	INT i = Offscreen.CurrentBuffer;

	// Blit the multisampled buffer first.
	if ( Offscreen.RenderTarget_MSAA[i].IsValid() )
		CopyFramebuffer(Offscreen.RenderTarget_MSAA[i].FBO, Offscreen.RenderTarget[i].FBO, Offscreen.Width, Offscreen.Height, true);

	// Setup render target.
	// This flushes the contents of the previous render target into texture memory unless invalidated
	FOpenGLBase::glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0); // TODO: DXGI Swapchains use a different render target

	// Disable depth test
	SetBlend(PF_NoZReject|PF_Occlude);

	GL3->TextureUnits.SetActive(0); // Needed?
	GL3->TextureUnits.Bind(Offscreen.ColorAttachment[i].Texture);

	// OpenGL 3.3 Core does not support compute pipelines
	FProgramID ProgramID = FProgramID::FromSpecial(FBO_Draw) | FGL::Program::Texture0;
	GL3->SetProgram(ProgramID, nullptr);
	GL3->SetVertexArrayBufferless();

	// TODO: Use a single triangle with Scissor test
	FOpenGLBase::glDrawArrays( GL_TRIANGLE_FAN, 0, 4);

	// Invalidate contents of FBO texture attachment.
	if ( FOpenGLBase::SupportsDataInvalidation )
		FOpenGLBase::glInvalidateTexImage(Offscreen.ColorAttachment[i].Texture.Texture, 0);

	MainFramebuffer_Locked = 0;
	unguard
}


/*-----------------------------------------------------------------------------
	Buffer Flushing.

- Note on Complex Surface modes:
The Program ID is precomputed and used to control the state of the buffer.
Texture mapping units are instead bound depending on said ID.
-----------------------------------------------------------------------------*/

//
// Push data to the quad expansion buffer instead and expand with a compute shader
//
static bool UpdateQuadCompute( FOpenGLBase* GL)
{
	if ( TileCount && FOpenGLBase::Compute.Programs.QuadExpansion )
	{
		BUFFER_QUADEXPANSION.Position = TileCount * 48;
		BUFFER_QUADEXPANSION.Update();
		GL->SetComputeQuadExpansion(BUFFER_QUADEXPANSION.VBO, QUAD.GetStreamBuffer()->VBO);

		GLuint Workgroups_X = (TileCount + 31) / 32;
		GLuint Workgroups_Y = 1;
		GLuint Workgroups_Z = 1;
		FOpenGLBase::glDispatchCompute( Workgroups_X, Workgroups_Y, Workgroups_Z);
		FOpenGLBase::glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT|GL_SHADER_STORAGE_BARRIER_BIT);

		// This buffer is being updated by the compute shader, do not override
		QUAD.GetStreamBuffer()->Position = 0;

		return true;
	}
	return false;
}

void UOpenGLRenderDevice::ExecDraws_GLSL3( DWORD Unused)
{
	guard(UOpenGLRenderDevice::ExecDraws_GLSL3);

	using namespace FGL::DrawBuffer;
	using namespace FGL::Draw;

//	DWORD Cycles = 0;	clockFast(Cycles);
//	DWORD DrawCount = 0;

	// Reset shader state
	GL3->SetProgram(INDEX_NONE, nullptr);

	if ( csTextureScaleData.PendingUpdate )
		csUpdateTextureScaleUBO();

	COMPLEX.GetStreamBuffer()->Update(); // Shared by other draw modes
	COMPLEX.GetIndexBuffer()->Update();  // Shared by other draw modes
	GOURAUD.GetStreamBuffer()->Update();
	if ( !UpdateQuadCompute(GL) )
		QUAD.GetStreamBuffer()->Update();
	DECAL.GetStreamBuffer()->Update();
	MergeDecals();

	csIndexCount  = 0;
	csDecalQueued = 0;
	csVolumetrics = 0;
	TileCount     = 0;
	Command* csVolumetricDraw = nullptr;

	Command* Draw = nullptr;
	for ( Draw=DrawBuffer.Draw.First; Draw; Draw=Draw->Next )
	{
//		DrawCount++;

		// Special case: defer volumetrics if decals come next
		if ( (Draw->Attribs & BT_SpecialFlag) && !csVolumetricDraw )
		{
			csVolumetrics = 1;
			csVolumetricDraw = Draw;
		}

		// Process draw
		(this->*Draw->Draw)(Draw);

		// Exiting decal mode
		if ( (Draw->Draw == &UOpenGLRenderDevice::ExecDraw_Decal_GLSL3) &&
			(!Draw->Next || (Draw->Next->Draw != &UOpenGLRenderDevice::ExecDraw_Decal_GLSL3)) )
		{
			if ( csVolumetricDraw )
			{
				// Draw all attached sequential volumetrics
				ExecDraw_ComplexSurfaceMultiVolumetric_GLSL3(csVolumetricDraw);
				csVolumetrics = 0;
				csVolumetricDraw = nullptr;
			}
		}
	}
	// Higor: extremely important in my AMD system.
	if ( DrawBuffer.Draw.First )
		FOpenGLBase::glFlush();

	DrawBuffer.Draw.Reset();
	CmdMark.Pop();
	COMPLEX.GetStreamBuffer()->Finish(); // Shared by other draw modes
	COMPLEX.GetIndexBuffer()->Finish();  // Shared by other draw modes
	GOURAUD.GetStreamBuffer()->Finish();
	QUAD.GetStreamBuffer()->Finish();
	DECAL.GetStreamBuffer()->Finish();
	ResetClipPlanes();

//	unclockFast(Cycles);	DOUBLE Time = Cycles * GSecondsPerCycleLong;
//	if ( Time > 0.04 )
//		debugf(NAME_DevGraphics, TEXT("Stall in ExecDraws (%i): %f"), DrawCount, Time);
	unguard;
}

void UOpenGLRenderDevice::ExecDraw_ComplexSurface_GLSL3( FGL::Draw::Command* Draw)
{
	guard(ComplexSurface);

	DWORD PolyFlags = Draw->PolyFlags;

	// Generate program ID
	FProgramID ProgramID = GetProgramID(Draw->PendingTextures,5);
	ProgramID |= GetProgramID(PolyFlags);
	ProgramID |= FGL::Program::VertexParam0; // GLSL3 passes TexturePan as an extra attribute
	ProgramID |= FProgramID::FromVertexType(FGL::VertexProgram::ComplexSurface);

	// GLSL3 uses Zone Light parameters
	if ( USES_AMBIENTLESS_LIGHTMAPS && (ProgramID & FGL::Program::Texture3) )
		ProgramID |= FGL::Program::ZoneLight;

	// Handle program and blend mode
	if ( csVolumetrics == 1 )
	{
		// Draw without volumetrics
		ProgramID &= ~(FGL::Program::Texture4/*|FGL::Program::ColorCorrection*/);
	}
	SetBlend(PolyFlags);

	FOpenGL3::FProgramData* ProgramData;
	GL3->SetTextures( Draw->PendingTextures, ProgramID.GetTextures());
	GL3->SetProgram(ProgramID & (~FGL::Program::Color0), (void**)&ProgramData);
	if ( ProgramData )
	{
		if ( ProgramID & FGL::Program::AlphaTest )
			GL3->SetUniform(ProgramData->AlphaTest, GetAlphaTest(PolyFlags));
	}

	// Handle buffers and states
	COMPLEX.ActiveAttribs = Draw->Attribs;
	COMPLEX.BufferStride  = Draw->Stride;
	GL3->SetVertexArray(&COMPLEX);

	// Draw
	FOpenGLBase::glDrawElements(GL_TRIANGLES, Draw->PrimitiveCount, GL_UNSIGNED_INT, BUFFER_OFFSET(Draw->PrimitiveStart * sizeof(DWORD)));

	// Draw flat shaded surfaces
	if ( ProgramID & FGL::Program::Color0 )
	{
		SetBlend(PF_Highlighted);
		GL3->SetProgram( FProgramID::FromVertexType(FGL::VertexProgram::ComplexSurface) | FGL::Program::Color0);
		FOpenGLBase::glDrawElements(GL_TRIANGLES, Draw->PrimitiveCount, GL_UNSIGNED_INT, BUFFER_OFFSET(Draw->PrimitiveStart * sizeof(DWORD)));
	}

	// Undo DepthFunc change
	if ( m_rpSetDepthEqual )
	{
		m_rpSetDepthEqual = 0;
		FOpenGLBase::glDepthFunc(GL_LEQUAL);
	}

	unguard;
}

void UOpenGLRenderDevice::ExecDraw_ComplexSurfaceMultiVolumetric_GLSL3( FGL::Draw::Command* Draw)
{
	guard(ComplexSurfaceMultiVolumetric);

	using namespace FGL::DrawBuffer;

	// Generate base program ID
	FProgramID BaseProgramID = FGL::Program::Texture4;
	BaseProgramID |= GetProgramID(Draw->PolyFlags);
	BaseProgramID &= ~(FGL::Program::DervMapped|FGL::Program::ColorCorrection|FGL::Program::ColorCorrectionAlpha|FGL::Program::Color0);
	if ( ColorCorrectionMode != CC_InShader ) 
		BaseProgramID &= ~FGL::Program::ColorCorrection; // Higor: Color combination is glitchy but better than nothing.

	// Setup common render state
	FOpenGLBase::glDepthFunc(GL_EQUAL);
	SetBlend(PF_Highlighted);


	while ( Draw )
	{
		// Choose a vertex program
		FProgramID ProgramID = BaseProgramID;
		if ( Draw->Draw == &UOpenGLRenderDevice::ExecDraw_ComplexSurface_GLSL3 )
		{
			ProgramID |= FProgramID::FromVertexType(FGL::VertexProgram::ComplexSurface);
			GL3->SetVertexArray(&COMPLEX);
		}
		else if ( Draw->Draw == &UOpenGLRenderDevice::ExecDraw_ComplexSurfaceALT_GLSL3 )
		{
			ProgramID |= FProgramID::FromVertexType(FGL::VertexProgram::ComplexSurfaceVBO);
			GL3->SetVertexArray(&COMPLEXSTATIC);
		}
		else
			break;

		if ( Draw->CacheID_Fog != 0 )
		{
			// Setup draw (program and polys)
			GL3->SetProgram(ProgramID, nullptr);
			GL3->SetTextures(Draw->PendingTextures, ProgramID.GetTextures());

			INT PrimitiveStart = Draw->PrimitiveStart;
			INT PrimitiveCount = 0;
			auto* First = Draw;
			do
			{
				PrimitiveCount += Draw->PrimitiveCount;
				Draw = Draw->Next;
			} while (Draw && (Draw->Draw == First->Draw) && (Draw->CacheID_Fog == First->CacheID_Fog) );

			// Draw
			FOpenGLBase::glDrawElements(GL_TRIANGLES, PrimitiveCount, GL_UNSIGNED_INT, BUFFER_OFFSET(PrimitiveStart * sizeof(DWORD)));
		}
		else
			Draw = Draw->Next;
	}

	FOpenGLBase::glDepthFunc(GL_LEQUAL);

	unguard;
}


void UOpenGLRenderDevice::ExecDraw_ComplexSurfaceALT_GLSL3( FGL::Draw::Command* Draw)
{
	guard(ComplexSurface);

	DWORD PolyFlags = Draw->PolyFlags;

	// Generate program ID
	FProgramID ProgramID = GetProgramID(Draw->PendingTextures,5);
	ProgramID |= GetProgramID(PolyFlags);
	ProgramID |= FGL::Program::VertexParam0; // GLSL3 passes TexturePan as an extra attribute
	ProgramID |= FProgramID::FromVertexType(FGL::VertexProgram::ComplexSurfaceVBO);

	// GLSL3 uses Zone Light parameters
	if ( USES_AMBIENTLESS_LIGHTMAPS && (ProgramID & FGL::Program::Texture3) )
		ProgramID |= FGL::Program::ZoneLight;

	// Handle program and blend mode
	if ( csVolumetrics == 1 )
	{
		// Draw without volumetrics
		ProgramID &= ~(FGL::Program::Texture4/*|FGL::Program::ColorCorrection*/);
	}
	SetBlend(PolyFlags);

	FOpenGL3::FProgramData* ProgramData;
	GL3->SetTextures( Draw->PendingTextures, ProgramID.GetTextures());
	GL3->SetProgram(ProgramID & (~FGL::Program::Color0), (void**)&ProgramData);
	if ( ProgramData )
	{
		if ( ProgramID & FGL::Program::AlphaTest )
			GL3->SetUniform(ProgramData->AlphaTest, GetAlphaTest(PolyFlags));
	}

	// Handle buffers and states
	COMPLEXSTATIC.ActiveAttribs = Draw->Attribs;
	COMPLEXSTATIC.BufferStride  = Draw->Stride;
	GL3->SetVertexArray(&COMPLEXSTATIC);

	// Draw
	FOpenGLBase::glDrawElements(GL_TRIANGLES, Draw->PrimitiveCount, GL_UNSIGNED_INT, BUFFER_OFFSET(Draw->PrimitiveStart * sizeof(DWORD)));

	// Draw flat shaded surfaces
	if ( ProgramID & FGL::Program::Color0 )
	{
		SetBlend(PF_Highlighted);
		GL3->SetProgram( FProgramID::FromVertexType(FGL::VertexProgram::ComplexSurface) | FGL::Program::Color0);
		FOpenGLBase::glDrawElements(GL_TRIANGLES, Draw->PrimitiveCount, GL_UNSIGNED_INT, BUFFER_OFFSET(Draw->PrimitiveStart * sizeof(DWORD)));
	}

	// Undo DepthFunc change
	if ( m_rpSetDepthEqual )
	{
		m_rpSetDepthEqual = 0;
		FOpenGLBase::glDepthFunc(GL_LEQUAL);
	}

	unguard;
}


void UOpenGLRenderDevice::ExecDraw_GouraudTriangles_GLSL3( FGL::Draw::Command* Draw)
{
	guard(GouraudTriangles);

	// Set Blend
	DWORD PolyFlags = Draw->PolyFlags;
	SetBlend(PolyFlags);

	// Set Program
	FProgramID ProgramID = GetProgramID(PolyFlags);
	ProgramID |= GetProgramID(Draw->PendingTextures, 2);
	ProgramID |= FProgramID::FromVertexType(FGL::VertexProgram::ComplexSurface);
	FOpenGL3::FProgramData* ProgramData;
	GL3->SetProgram(ProgramID, (void**)&ProgramData);
	if ( ProgramData )
	{
		if ( ProgramID & FGL::Program::AlphaTest )
			GL3->SetUniform(ProgramData->AlphaTest, GetAlphaTest(PolyFlags));
		GL3->SetUniform(ProgramData->ClipPlane, ClipPlanes[Draw->ClipPlaneID]);
	}

	// Set Textures
	if ( ProgramID.GetTextures() )
		GL3->SetTextures( Draw->PendingTextures, ProgramID.GetTextures());

	// Handle buffers
	GOURAUD.ActiveAttribs = Draw->Attribs;
	GOURAUD.BufferStride  = Draw->Stride;
	GL3->SetVertexArray(&GOURAUD);

	// Enable NearClip plane
	if ( Draw->ClipPlaneID > 0 )
		FOpenGLBase::glEnable(GL_CLIP_DISTANCE0);

	// Draw
	FOpenGLBase::glDrawArrays( GL_TRIANGLES, Draw->PrimitiveStart, Draw->PrimitiveCount);

	// Disable temporary states (Specular, NearClip)
	if ( Draw->ClipPlaneID > 0 )
		FOpenGLBase::glDisable(GL_CLIP_DISTANCE0);

	unguard;
}


void UOpenGLRenderDevice::ExecDraw_Decal_GLSL3( FGL::Draw::Command* Draw)
{
	guard(GouraudTriangles);

	// Set Blend
	DWORD PolyFlags = Draw->PolyFlags;
	SetBlend(PolyFlags);

	// Set Program
	FProgramID ProgramID = GetProgramID(PolyFlags);
	ProgramID |= GetProgramID(Draw->PendingTextures, 1);
	ProgramID |= FProgramID::FromVertexType(FGL::VertexProgram::ComplexSurface);
	GL3->SetProgram(ProgramID, nullptr);

	// Set Textures
	if ( ProgramID.GetTextures() )
		GL3->SetTextures( Draw->PendingTextures, ProgramID.GetTextures());

	// Handle buffers
	DECAL.ActiveAttribs = Draw->Attribs;
	DECAL.BufferStride  = Draw->Stride;
	GL3->SetVertexArray(&DECAL);

	// Draw
	FOpenGLBase::glDrawArrays( GL_TRIANGLES, Draw->PrimitiveStart, Draw->PrimitiveCount);

	unguard;
}



void UOpenGLRenderDevice::ExecDraw_Quad_GLSL3( FGL::Draw::Command* Draw)
{
	guard(Quad);

	// Set Blend
	DWORD PolyFlags = Draw->PolyFlags;
	SetBlend(PolyFlags);

	// Set Program
	FProgramID ProgramID = GetProgramID(PolyFlags);
	ProgramID |= GetProgramID(Draw->PendingTextures, 1);
	ProgramID |= FProgramID::FromVertexType(FGL::VertexProgram::Tile);

	FOpenGL3::FProgramData* ProgramData;
	GL3->SetProgram(ProgramID, (void**)&ProgramData);
	if ( ProgramData )
	{
		if ( ProgramID & FGL::Program::AlphaTest )
			GL3->SetUniform(ProgramData->AlphaTest, GetAlphaTest(PolyFlags));
	}

	// Set Textures
	if ( ProgramID.GetTextures() )
		GL3->SetTextures( Draw->PendingTextures, ProgramID.GetTextures());

	// Handle buffers and states
	QUAD.ActiveAttribs = Draw->Attribs;
	QUAD.BufferStride  = Draw->Stride;
	GL3->SetVertexArray(&QUAD);

	// Draw
	FOpenGLBase::glDrawArrays( GL_TRIANGLES, Draw->PrimitiveStart, Draw->PrimitiveCount);

	unguard;
}

void UOpenGLRenderDevice::ExecDraw_Line_GLSL3( FGL::Draw::Command* Draw)
{
	guard(Line);

//	auto& Line = LINE;

	// Set Program
	FProgramID ProgramID = FProgramID::FromVertexType(FGL::VertexProgram::Simple);
	ProgramID |= FGL::Program::Color0;
	FOpenGL3::FProgramData* ProgramData;
	GL3->SetProgram(ProgramID, (void**)&ProgramData);

	if ( Draw->PolyFlags & LINE_DepthCued )
		SetBlend(PF_Highlighted|PF_Occlude);
	else
		SetBlend(PF_Highlighted|PF_NoZReject);

	// Handle buffers and states
	// SET ATTRIBS?
	GL3->SetVertexArray(&LINE);

	// Draw
	FOpenGLBase::glDrawArrays(GL_LINES, Draw->PrimitiveStart, Draw->PrimitiveCount);

	unguard;
}

/*-----------------------------------------------------------------------------
	Utils.
-----------------------------------------------------------------------------*/
