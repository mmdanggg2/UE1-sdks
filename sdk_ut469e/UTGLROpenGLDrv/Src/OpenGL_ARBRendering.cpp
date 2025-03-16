/*=============================================================================
	OpenGL_ARBRendering.cpp

	ARBvp/fp rendering code

	Does not use NoTexture.
	Fully buffered.

	Notes on multi-buffering:
	* All arrays are separated and may be populated without buffer flushing/drawing.
	* The buffer only flushes existing data if draw order needs to be preserved (transparency, vert limits).
	* Because Textures need to be bound, changes in textures cause buffer flushing.

	Decals and Fog:
	* In ARB mode, modulated decals cause ComplexSurface buffering to be split into two stages.
	* Stage 1 renders everything and defers fog maps to Stage 2.
	* Decals in the GouraudTriangle buffer are drawn inbetween both stages.

	ARB Vertex Program globals:
	* Env[0]: Frame->Coords.Origin
	* Env[1]: Frame->Coords.XAxis
	* Env[2]: Frame->Coords.YAxis
	* Env[3]: Frame->Coords.ZAxis
	* Env[4]: WavyU, WavyV, 0, TimeSeconds
	* Env[5]: ZonePanU, ZonePanV, 0, 0
	* Env[6]: UseLightUV, UseLightUVAtlas, UseFogUV, UseFogUVAtlas
	* Env[7]: DervScaleU, DervScaleV, 0, 1

	ARB Fragment Program globals:
	* Env[0]: GammaRed, GammaGreen, GammaBlue, Brightness
	* Env[1]: 0, 0, AlphaReject, LightmapScale
	* Env[2]: ZoneAmbientColor

	Revision history:
	* Created by Fernando Velazquez.
	* Separated ARB rendering into render path procs.
	* DrawComplexPolygon_ARB -> Single pass rendering, can buffer more than one Complex Surface.
	* DrawComplexPolygon_ARB -> VBO buffering.
	* Draw2DLine_ARB, Draw3DLine_ARB, Draw2DPoint_ARB -> Buffering.
	* DrawGouraudTriangles_ARB -> Clip plane state buffering.
=============================================================================*/


#include "OpenGLDrv.h"
#include "UTGLROpenGL.h"
#include "FOpenGL12.h"

#define GL1 ((FOpenGL12*)GL)

#if USES_STATIC_BSP

static bool IsAtlasID( QWORD CacheID)
{
	return FStaticBspInfoBase::IsAtlasID(CacheID) != 0;
}

#else
# define IsAtlasID(CacheID) false
#endif

#define COMPLEX       (*(FGL::DrawBuffer::FComplexARB*)      DrawBuffer.Complex)
#define COMPLEXSTATIC (*(FGL::DrawBuffer::FComplexStaticARB*)DrawBuffer.ComplexStatic)
#define GOURAUD       (*(FGL::DrawBuffer::FGouraudARB*)      DrawBuffer.Gouraud)
#define QUAD          (*(FGL::DrawBuffer::FQuadARB*)         DrawBuffer.Quad)
#define LINE          (*(FGL::DrawBuffer::FLineARB*)         DrawBuffer.Line)
#define FILL          (*(FGL::DrawBuffer::FFillARB*)         DrawBuffer.Fill)
#define DECAL         (*(FGL::DrawBuffer::FDecalARB*)        DrawBuffer.Decal)

/*-----------------------------------------------------------------------------
	Render path procs.
-----------------------------------------------------------------------------*/


void UOpenGLRenderDevice::DrawComplexSurface_ARB( FSceneNode* Frame, FSurfaceInfo_DrawComplex& Surface, FSurfaceFacet& Facet)
{
	check(Surface.Texture); // TODO: Draw Textureless surf as a simple ZBuffer write (fixes surfs bleeding into skybox)

	FGL::Draw::TextureList TexDrawList;
	TexDrawList.Init(Surface, DetailTextures);

	DWORD PolyFlags = TexDrawList.PolyFlags[0];

#if USES_STATIC_BSP
	if ( CanBufferStaticComplexSurfaceGeometry_VBO( Surface, Facet) )
	{
		// VBO DrawComplex mode
		//
		// VBO contains and sends the following info to the vertex shader:
		// - vertex.position:     Untransformed Vertex list
		// - vertex.texcoords[0]: Unscaled U, Unscaled V, PanU, PanV
		// - vertex.texcoords[1]: AtlasU, AtlasV
		// - vertex.texcoords[2]: AutoUPan, AutoVPan, Wavy, 0
		//
		// When setting up a new VBO DrawComplex instance, we send the following info to the vertex shader:
		// - texture.attrib[6]:  TextureUScale, TextureVScale, DetailUScale, DetailVScale
		// - texture.attrib[7]:  MacroUScale, MacroVScale, 0, 0
		//

		// If there are pending volumetrics in the pipeline, draw them now
		if ( COMPLEXSTATIC.PendingVolumetrics )
			FlushDrawBuffer_ComplexSurfaceVBO_ARB();

		// Surfaces that are not transparent support multi-buffering.
		SetDrawBuffering( DRAWBUFFER_ComplexSurfaceVBO, !(PolyFlags & PF_NoMultiBuffering) );

		DWORD ZoneNumber = GetZoneNumber(Surface);
		FProgramID NewProgramID = MergeComplexSurfaceToVBODrawBuffer_ARB(TexDrawList, ZoneNumber);
		if ( NewProgramID.GetValue() )
		{
			// Flush ComplexSurfaceVBO buffer (over vert limit, diff texture, diff poly flags, diff program id)
			FlushDrawBuffers(DRAWBUFFER_ComplexSurfaceVBO); 
			DrawBuffer.ActiveType |= DRAWBUFFER_ComplexSurfaceVBO;

			auto& ComplexStatic = COMPLEXSTATIC;
			ComplexStatic.Setup(PolyFlags, TexDrawList);
			ComplexStatic.ProgramID = NewProgramID;
			ComplexStatic.ZoneId = ZoneNumber;

			if ( NewProgramID & FGL::Program::Texture0 )
				ComplexStatic.TextureScale[TMU_Base] = FGL::FTextureScale(*Surface.Texture, E_Precise);
			if ( NewProgramID & FGL::Program::Texture1 )
				ComplexStatic.TextureScale[TMU_DetailTexture] = FGL::FTextureScale(*Surface.DetailTexture);
			if ( NewProgramID & FGL::Program::Texture2 )
				ComplexStatic.TextureScale[TMU_MacroTexture] = FGL::FTextureScale(*Surface.MacroTexture);
			if ( NewProgramID & FGL::Program::Texture3 )
				ComplexStatic.LightUVAtlas = IsAtlasID(Surface.LightMap->CacheID);
			if ( NewProgramID & FGL::Program::Texture4 )
				ComplexStatic.FogUVAtlas = IsAtlasID(Surface.FogMap->CacheID);

		}
		BufferStaticComplexSurfaceGeometry_VBO_ARB(Facet);
	}
	else
#endif
	{
		// Buffered DrawComplex mode
		//
		// The buffer precalculates and sends the following info to the vertex shader:
		// - vertex.position:     Vertex list - transformed to UT screen space
		// - vertex.texcoords[0]: Unscaled U, Unscaled V, PanU, PanV
		// - vertex.texcoords[1]: LightU, Unscaled LightV, 0, 0
		// - vertex.texcoords[2]: FogU, Unscaled FogV, 0, 0
		// 
		// When setting up a new buffered DrawComplex instance, we send the following info to the vertex shader:
		// - texture.attrib[6]:  TextureUScale, TextureVScale, DetailUScale, DetailVScale
		// - texture.attrib[7]:  MacroUScale, MacroVScale, 0, 0

		// If there are pending volumetrics in the pipeline, draw them now
		if ( COMPLEX.PendingVolumetrics )
			FlushDrawBuffer_ComplexSurface_ARB();

		// Surfaces that are not transparent support multi-buffering.
		SetDrawBuffering( DRAWBUFFER_ComplexSurface, !(PolyFlags & PF_NoMultiBuffering) );
		auto& Complex = COMPLEX;

		DWORD ZoneNumber = GetZoneNumber(Surface);
		FProgramID NewProgramId = MergeComplexSurfaceToDrawBuffer_ARB(TexDrawList,ZoneNumber);
		if ( NewProgramId.GetValue() )
		{
			// Flush ComplexSurface buffer (diff texture, diff poly flags, diff program id)
			FlushDrawBuffers(DRAWBUFFER_ComplexSurface); 
			DrawBuffer.ActiveType |= DRAWBUFFER_ComplexSurface;
			Complex.Setup(PolyFlags, TexDrawList);
			Complex.ProgramID = NewProgramId;
			Complex.ZoneId = ZoneNumber;

			if ( NewProgramId & FGL::Program::Texture0 )
				Complex.TextureScale[TMU_Base] = FGL::FTextureScale(*Surface.Texture, E_Precise);
			if ( NewProgramId & FGL::Program::Texture1 )
				Complex.TextureScale[TMU_DetailTexture] = FGL::FTextureScale(*Surface.DetailTexture);
			if ( NewProgramId & FGL::Program::Texture2 )
				Complex.TextureScale[TMU_MacroTexture] = FGL::FTextureScale(*Surface.MacroTexture);
		}

		FProgramID ProgramId = Complex.ProgramID;

		// Polygon(s) will be texture mapped
		if ( ProgramId & (FGL::Program::Texture0|FGL::Program::Texture3|FGL::Program::Texture4) )
		{
			m_csUDot = Facet.MapCoords.XAxis | Facet.MapCoords.Origin;
			m_csVDot = Facet.MapCoords.YAxis | Facet.MapCoords.Origin;
		}

		// Light and fog scale may change, even if using the same atlas.
		FGL::FTextureScale LightScale;
		FGL::FTexturePan   LightPan;
		FGL::FTextureScale FogScale;
		FGL::FTexturePan   FogPan;
		if ( Surface.LightMap )
		{
			LightScale = *Surface.LightMap;
			LightPan   = FGL::FTexturePan(*Surface.LightMap,-0.5);
		}
		if ( Surface.FogMap )
		{
			FogScale = *Surface.FogMap;
			FogPan   = FGL::FTexturePan(*Surface.FogMap,-0.5);
		}

		for ( FSavedPoly* Poly=Facet.Polys; Poly; Poly=Poly->Next)
		{
			INT NumPts = Poly->NumPts;
			if ( NumPts > 2 )
			{
				if ( Complex.AtCapacity(NumPts) )
				{
					FlushDrawBuffer_ComplexSurface_ARB();
					DrawBuffer.ActiveType |= DRAWBUFFER_ComplexSurface;
				}

				Complex.Polys++;
				Complex.PolyVertsStart.AddItem(Complex.GetStreamBuffer()->Position / Complex.GetStride());
				Complex.PolyVertsCount.AddItem(NumPts);

				void* Stream;
				Complex.StreamStart(Stream);
				for ( INT v=0; v<NumPts; v++)
				{
					FVector& Point = Poly->Pts[v]->Point;
					StreamData(FGL::FVertex3D(Point), Stream);
					if ( ProgramId & FGL::Program::Color0 )
						StreamData(FGL::FColorRGBA(Surface.FlatColor), Stream);
					if ( ProgramId & (FGL::Program::Texture0|FGL::Program::Texture3|FGL::Program::Texture4) )
					{
						FLOAT U = (Facet.MapCoords.XAxis | Point) - m_csUDot;
						FLOAT V = (Facet.MapCoords.YAxis | Point) - m_csVDot;
						if ( ProgramId & FGL::Program::Texture0 )
							StreamData(FGL::FTextureUVPan( U, V, *Surface.Texture), Stream);
						if ( ProgramId & FGL::Program::Texture3 )
							StreamData(FGL::FTextureUV( (U-LightPan.UPan)*LightScale.UScale, (V-LightPan.VPan)*LightScale.VScale), Stream);
						if ( ProgramId & FGL::Program::Texture4 )
							StreamData(FGL::FTextureUV( (U-FogPan.UPan)*FogScale.UScale, (V-FogPan.VPan)*FogScale.VScale), Stream);
					}
				}
				Complex.StreamEnd(Stream);
			}
		}
	}
}

void UOpenGLRenderDevice::DrawGouraudPolygon_ARB( FSceneNode* Frame, FSurfaceInfo_DrawGouraud& Surface, FTransTexture** Pts, INT NumPts)
{
	// Buffered GouraudPolygon mode
	//
	// The buffer precalculates and sends the following info to the vertex shader:
	// - vertex.position:        Vertex list - transformed to UT screen space
	// - vertex.texcoords[0]:    TextureU, TextureV, 0, 1
	// - vertex.color:           RGBA diffuse light
	// - vertex.color.secondary: RGB specular light
	//
	// When setting up a new GouraudTriangles instance, we send the following info to the vertex shader:
	// - texture.attrib[6]:      TextureUScale, TextureVScale, DetailUScale, DetailVScale
	//

	FGL::Draw::TextureList TexDrawList;
	Surface.PolyFlags &= ~(PF_Unlit);
	INT NumVerts = (NumPts-2) * 3;

	// Special decal path, color buffering done for forward compatibility with alpha decals
	if ( ((Surface.PolyFlags & PF_RenderFog) || !m_DGT) && (Surface.PolyFlags & (PF_Modulated|PF_AlphaBlend)) )
	{
		// Fixed data stream
		struct DecalData
		{
			FGL::FVertex3D  Vertex;
			FGL::FColorRGBA Color;
			FGL::FTextureUV UV;
		};

		// Setup buffer
		Surface.PolyFlags &= ~(PF_RenderFog);
		Surface.PolyFlags |= PF_Gouraud;
		Surface.DetailTexture = nullptr;
		TexDrawList.Init(Surface);

		// Flush Decal buffer if needed
		auto& Decal = DECAL;
		if ( (DrawBuffer.ActiveType & DRAWBUFFER_Decal)
			&&	(	Decal.AtCapacity(NumVerts)
				||	Decal.PolyFlags != Surface.PolyFlags
				||	!CompareTextures(TexDrawList,Decal.Textures)) )
		{
			FlushDrawBuffer_Decal_ARB();
		}

		// Decals do not break multi-buffering
		if ( SetDrawBuffering(DRAWBUFFER_Decal, true) )
		{
			Decal.Setup(Surface.PolyFlags, TexDrawList);
			Decal.TextureScale[TMU_Base] = FGL::FTextureScale(*Surface.Texture, E_Precise);
		}

		// Color (assumes all vertices have matching colors)
		DWORD Color;
		if ( Surface.PolyFlags & PF_AlphaBlend )  Color = FPlaneTo_RGB_A255(&Pts[0]->Light);
		else                                      Color = 0xFFFFFFFF;

		// Buffer triangle(s)
		void* Stream;
		Decal.StreamStart(Stream);
		void* StreamFirst = Stream;
		for ( INT j=0; j<NumPts; j++)
		{
			// Starting from point 4, copy 0 and previous
			if ( j >= 3 )
			{
				((DecalData*)Stream)[0] = ((DecalData*)StreamFirst)[0];
				((DecalData*)Stream)[1] = ((DecalData*)Stream)[-1];
				(*(DecalData**)&Stream) += 2;
			}
			FTransTexture& P = *Pts[j];
			StreamData(FGL::FVertex3D(P.Point), Stream);
			StreamData(FGL::FColorRGBA(Color), Stream);
			StreamData(FGL::FTextureUV(P.U, P.V), Stream); // Do not multiply, by UMult, VMult | do that in shader.
		}
		Decal.StreamEnd(Stream);
		return;
	}


	// Get textures
	TexDrawList.Init(Surface);

	auto& Gouraud = GOURAUD;

	// Flush Gouraud Triangle buffer if needed
	if ( (DrawBuffer.ActiveType & DRAWBUFFER_GouraudPolygon)
		&&	(	Gouraud.AtCapacity(NumVerts)
			||	Gouraud.PolyFlags != Surface.PolyFlags
			||	!CompareTextures(TexDrawList,Gouraud.Textures)) )
	{
		FlushDrawBuffers(DRAWBUFFER_GouraudPolygon);
	}

	// Gouraud Triangles that are not transparent support multi-buffering.
	if ( SetDrawBuffering( DRAWBUFFER_GouraudPolygon, !(Surface.PolyFlags & PF_NoMultiBuffering) ) )
	{
		Gouraud.Setup(Surface.PolyFlags, TexDrawList);
		Gouraud.TextureScale[TMU_Base] = FGL::FTextureScale(*Surface.Texture, E_Precise);
		if ( TexDrawList.Infos[1] )
			Gouraud.TextureScale[TMU_DetailTexture] = FGL::FTextureScale(*TexDrawList.Infos[1]);
	}

	// Buffer triangle(s)
	void* Stream;
	Gouraud.StreamStart(Stream);
	void* StreamFirst = Stream;
	for ( INT j=0; j<NumPts; j++)
	{
		// Starting from point 4, copy 0 and previous
		if ( j >= 3 )
		{
			DWORD Stride = Gouraud.GetStride();
			appMemcpy(Stream, StreamFirst, Stride);
			appMemcpy((BYTE*)Stream+Stride, (BYTE*)Stream-Stride, Stride);
			(*(BYTE**)&Stream) += Stride+Stride;
		}
		FTransTexture& P = *Pts[j];
		StreamData(FGL::FVertex3D(P.Point), Stream);
		if ( Surface.PolyFlags & PF_Gouraud )
		{
			if ( Surface.PolyFlags & PF_RenderFog )
			{
				// LEGACY ARB DOES NOT PASS SECONDARY COLOR WITH 4 CHANNELS!
				FLOAT f255_Times_One_Minus_FogW = 255.0f * (1.0f - P.Fog.W);
				StreamData(FGL::FColorRGBA(FPlaneTo_RGBScaled_A255(&P.Light, f255_Times_One_Minus_FogW)), Stream);
				StreamData(FGL::FColorRGBA(FPlaneTo_RGB_A0(&P.Fog)), Stream);
			}
			else
			{
#ifdef UTGLR_RUNE_BUILD
				StreamData(FGL::FColorRGBA(FPlaneTo_RGB_Aub(&P.Light,m_gpAlpha)), Stream);
#else
				StreamData(FGL::FColorRGBA(FPlaneTo_RGB_A255(&P.Light)), Stream);
#endif
			}
		}
		StreamData(FGL::FTextureUV(P.U, P.V), Stream); // Do not multiply, by UMult, VMult | do that in shader.
	}
	Gouraud.StreamEnd(Stream);
}

void UOpenGLRenderDevice::DrawTile_ARB( FSceneNode* Frame, FSurfaceInfo_DrawTile& Surface, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, FLOAT Z)
{
	// Hack to modify transparency of HUD elements
	if ( (Surface.PolyFlags & PF_Translucent) && m_shaderAlphaHUD )
	{
		Surface.PolyFlags &= ~(PF_Translucent|PF_Modulated|PF_AlphaBlend);
		Surface.PolyFlags |= (PF_Highlighted|PF_AlphaHack);
	}

	// Get texture
	FGL::Draw::TextureList TexDrawList;
	TexDrawList.Init(Surface);

	auto& Quad = QUAD;

	// Flush Quad buffer if needed (over vert limit, diff texture, diff poly flags)
	if ( (DrawBuffer.ActiveType & DRAWBUFFER_Quad)
		&&	(	Quad.AtCapacity(4)
			||	Quad.PolyFlags != Surface.PolyFlags)
			||	!CompareTextures(TexDrawList, Quad.Textures) )
	{
		FlushDrawBuffer_Quad_ARB();
	}

	// Tiles that are not transparent support multi-buffering.
	if ( SetDrawBuffering( DRAWBUFFER_Quad, !(Surface.PolyFlags & PF_NoMultiBuffering) ) )
		Quad.Setup(Surface.PolyFlags, TexDrawList);

	// Buffer the quad
	// Higor: SSE buffering would be nice, but ultimately we're storing the results back in memory
	// SSE buffering really shines when we can keep data in registers without storage for as long as possible.
	FLOAT PX1 = X - Frame->FX2;
	FLOAT PX2 = PX1 + XL;
	FLOAT PY1 = Y - Frame->FY2;
	FLOAT PY2 = PY1 + YL;

	FLOAT RPX1 = m_RFX2 * PX1;
	FLOAT RPX2 = m_RFX2 * PX2;
	FLOAT RPY1 = m_RFY2 * PY1;
	FLOAT RPY2 = m_RFY2 * PY2;
	if ( !Frame->Viewport->IsOrtho() )
	{
		RPX1 *= Z;
		RPX2 *= Z;
		RPY1 *= Z;
		RPY2 *= Z;
	}

	FTextureInfo& Info = *Surface.Texture;
	FLOAT UMult = 1.0f / (Info.UScale * Info.USize);
	FLOAT VMult = 1.0f / (Info.VScale * Info.VSize);
	FLOAT SU1 = (U)      * UMult;
	FLOAT SU2 = (U + UL) * UMult;
	FLOAT SV1 = (V)      * VMult;
	FLOAT SV2 = (V + VL) * VMult;

	DWORD dwColor = GET_COLOR_DWORD(Surface.Color);
	if ( (Surface.PolyFlags & PF_Gouraud) && !(Surface.PolyFlags & PF_AlphaBlend) )
		dwColor |= COLOR_NO_ALPHA;

	void* Stream;

	Quad.StreamStart(Stream);
	StreamData(FGL::FVertex3D( RPX1, RPY1, Z), Stream);
	StreamData(FGL::FColorRGBA(dwColor), Stream);
	StreamData(FGL::FTextureUV( SU1, SV1), Stream);

	StreamData(FGL::FVertex3D( RPX2, RPY1, Z), Stream);
	StreamData(FGL::FColorRGBA(dwColor), Stream);
	StreamData(FGL::FTextureUV( SU2, SV1), Stream);

	StreamData(FGL::FVertex3D( RPX2, RPY2, Z), Stream);
	StreamData(FGL::FColorRGBA(dwColor), Stream);
	StreamData(FGL::FTextureUV( SU2, SV2), Stream);

	StreamData(FGL::FVertex3D( RPX1, RPY2, Z), Stream);
	StreamData(FGL::FColorRGBA(dwColor), Stream);
	StreamData(FGL::FTextureUV( SU1, SV2), Stream);
	Quad.StreamEnd(Stream);
}

void UOpenGLRenderDevice::Draw3DLine_ARB( FSceneNode* Frame, FPlane& Color, DWORD LineFlags, FVector P1, FVector P2)
{
	auto& Line = LINE;

	// Flush line buffer if needed
	if ( (DrawBuffer.ActiveType & DRAWBUFFER_Line)
		&&	(	Line.AtCapacity(2) 
			||	Line.LineFlags != LineFlags) )
		FlushDrawBuffer_Line_ARB();

	// Lines that are not transparent and are depth filtered support multi-buffering.
	if ( SetDrawBuffering( DRAWBUFFER_Line, LineFlags==LINE_DepthCued ) )
	{
		Line.LineFlags = LineFlags;
		Line.SetupLine(LineFlags);
	}

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
			Draw2DPoint_ARB( Frame, Color, LineFlags, Pz1X - 1.0f, Pz1Y - 1.0f, Pz1X + 1.0f, Pz1Y + 1.0f, 1.0f);
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

	DWORD dwColor = FPlaneTo_RGB_A255(&Color);

	void* Stream;
	Line.StreamStart(Stream);
	StreamData(FGL::FVertex3D(P1), Stream);
	StreamData(FGL::FColorRGBA(dwColor), Stream);
	StreamData(FGL::FVertex3D(P2), Stream);
	StreamData(FGL::FColorRGBA(dwColor), Stream);
	Line.StreamEnd(Stream);
}

void UOpenGLRenderDevice::Draw2DLine_ARB( FSceneNode* Frame, FPlane& Color, DWORD LineFlags, FVector P1, FVector P2)
{
	auto& Line = LINE;

	// Flush line buffer if needed
	if ( (DrawBuffer.ActiveType & DRAWBUFFER_Line)
		&&	(	Line.AtCapacity(2) 
			||	Line.LineFlags != LineFlags) )
		FlushDrawBuffer_Line_ARB();

	// Lines that are not transparent and are depth filtered support multi-buffering.
	if ( SetDrawBuffering( DRAWBUFFER_Line, LineFlags==LINE_DepthCued ) )
	{
		Line.LineFlags = LineFlags;
		Line.SetupLine(LineFlags);
	}

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

void UOpenGLRenderDevice::Draw2DPoint_ARB( FSceneNode* Frame, FPlane& Color, DWORD LineFlags, FLOAT X1, FLOAT Y1, FLOAT X2, FLOAT Y2, FLOAT Z)
{
	// 2D point uses Quad buffer
	DWORD PolyFlags = (LineFlags == LINE_DepthCued) ? PF_Highlighted|PF_Occlude : PF_Highlighted|PF_NoZReject;

	auto& Quad = QUAD;

	// Flush Quad buffer if needed (over vert limit, textured, diff poly flags)
	if ( (DrawBuffer.ActiveType & DRAWBUFFER_Quad)
	&&	(	Quad.AtCapacity(4)
		||	Quad.Textures[0].PoolID != INDEX_NONE
		||	Quad.PolyFlags != PolyFlags) )
	{
		FlushDrawBuffer_Quad_ARB();
	}

	// Points that are not transparent and are depth filtered support multi-buffering.
	if ( SetDrawBuffering( DRAWBUFFER_Quad, LineFlags==LINE_DepthCued ) )
	{
		Quad.SetupLine(LineFlags);
		Quad.Textures[0].PoolID = INDEX_NONE;
	}

	DWORD dwColor = FPlaneTo_RGB_A255(&Color);

	//Get point coordinates back in 3D
	FLOAT X1Pos = m_RFX2 * (X1 - Frame->FX2 - 0.5f);
	FLOAT Y1Pos = m_RFY2 * (Y1 - Frame->FY2 - 0.5f);
	FLOAT X2Pos = m_RFX2 * (X2 - Frame->FX2 + 0.5f);
	FLOAT Y2Pos = m_RFY2 * (Y2 - Frame->FY2 + 0.5f);
	if ( !Frame->Viewport->IsOrtho() )
	{
		X1Pos *= Z;
		Y1Pos *= Z;
		X2Pos *= Z;
		Y2Pos *= Z;
	}

	void* Stream;
	Quad.StreamStart(Stream);
	StreamData(FGL::FVertex3D(X1Pos,Y1Pos,Z), Stream);
	StreamData(FGL::FColorRGBA(dwColor), Stream);

	StreamData(FGL::FVertex3D(X2Pos,Y1Pos,Z), Stream);
	StreamData(FGL::FColorRGBA(dwColor), Stream);

	StreamData(FGL::FVertex3D(X2Pos,Y2Pos,Z), Stream);
	StreamData(FGL::FColorRGBA(dwColor), Stream);

	StreamData(FGL::FVertex3D(X1Pos,Y2Pos,Z), Stream);
	StreamData(FGL::FColorRGBA(dwColor), Stream);
	Quad.StreamEnd(Stream);
}



void UOpenGLRenderDevice::DrawGouraudTriangles_ARB( const FSceneNode* Frame, FSurfaceInfo_DrawGouraudTris& Surface, FTransTexture* const Pts, INT NumPts, DWORD DataFlags)
{
	Surface.PolyFlags &= ~(PF_Unlit);

	// Get textures
	FGL::Draw::TextureList TexDrawList;
	TexDrawList.Init(Surface, DetailTextures);

	auto& Gouraud = GOURAUD;

	// Flush Gouraud Triangle buffer if needed
	if ( (DrawBuffer.ActiveType & DRAWBUFFER_GouraudPolygon)
	&&	(	Gouraud.AtCapacity(3) //One triangle
		||	Gouraud.PolyFlags != Surface.PolyFlags
		||	!CompareTextures(TexDrawList,Gouraud.Textures)) )
	{
		FlushDrawBuffers(DRAWBUFFER_GouraudPolygon);
	}

	// Gouraud Triangles that are not transparent support multi-buffering.
	if ( SetDrawBuffering( DRAWBUFFER_GouraudPolygon, !(Surface.PolyFlags & PF_NoMultiBuffering) ) )
	{
		Gouraud.Setup(Surface.PolyFlags, TexDrawList);
		Gouraud.TextureScale[TMU_Base] = FGL::FTextureScale(*Surface.Texture, E_Precise);
		if ( Surface.DetailTexture )
			Gouraud.TextureScale[TMU_DetailTexture] = FGL::FTextureScale(*Surface.DetailTexture);
	}

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

	// Evaluate each Triangle for buffering.
	for ( INT i=0; i<NumPts; i+=3)
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

		// Draw and flush buffer if full.
		if ( Gouraud.AtCapacity(3) )
		{
			FlushDrawBuffer_GouraudTriangles_ARB();
			DrawBuffer.ActiveType |= DRAWBUFFER_GouraudPolygon;
		}

		// Buffer unclipped triangle
		void* Stream;
		Gouraud.StreamStart(Stream);
		for ( INT j=0; j<3; j++)
		{
			FTransTexture& P = *Tri[j];
			StreamData(FGL::FVertex3D(P.Point), Stream);
			if ( Surface.PolyFlags & PF_Gouraud )
			{
				if ( Surface.PolyFlags & PF_RenderFog )
				{
					// LEGACY ARB DOES NOT PASS SECONDARY COLOR WITH 4 CHANNELS!
					FLOAT f255_Times_One_Minus_FogW = 255.0f * (1.0f - P.Fog.W);
					StreamData(FGL::FColorRGBA(FPlaneTo_RGBScaled_A255(&P.Light, f255_Times_One_Minus_FogW)), Stream);
					StreamData(FGL::FColorRGBA(FPlaneTo_RGB_A0(&P.Fog)), Stream);
				}
				else
				{
#ifdef UTGLR_RUNE_BUILD
					StreamData(FGL::FColorRGBA(FPlaneTo_RGB_Aub(&P.Light,m_gpAlpha)), Stream);
#else
					StreamData(FGL::FColorRGBA(FPlaneTo_RGB_A255(&P.Light)), Stream);
#endif
				}
			}
			StreamData(FGL::FTextureUV(P.U,P.V), Stream); // Do not multiply, by UMult, VMult | do that in shader.
		}
		Gouraud.StreamEnd(Stream);
	}

	// All Triangles rejected or already flushed.
	if ( Gouraud.GetStreamBuffer()->Position == 0 )
		DrawBuffer.ActiveType &= ~DRAWBUFFER_GouraudPolygon;
}

void UOpenGLRenderDevice::UOpenGLRenderDevice::DrawTileList_ARB(const FSceneNode* Frame, FSurfaceInfo_DrawTile& Surface, const FTileRect* TileList, INT TileCount, FLOAT Z)
{
	// Hack to modify transparency of HUD elements
	if ( (Surface.PolyFlags & PF_Translucent) && m_shaderAlphaHUD )
	{
		Surface.PolyFlags &= ~(PF_Translucent|PF_Modulated|PF_AlphaBlend);
		Surface.PolyFlags |= (PF_Highlighted|PF_AlphaHack);
	}

	// Get texture
	FGL::Draw::TextureList TexDrawList;
	TexDrawList.Init(Surface);

	auto& Quad = QUAD;

	// Flush Quad buffer if needed (over vert limit, diff texture, diff poly flags)
	if ( (DrawBuffer.ActiveType & DRAWBUFFER_Quad)
		&&	(	Quad.AtCapacity(4 * TileCount)
			||	Quad.PolyFlags != Surface.PolyFlags)
		||	!CompareTextures(TexDrawList, Quad.Textures) )
	{
		FlushDrawBuffer_Quad_ARB();
	}

	// Tiles that are not transparent support multi-buffering.
	if ( SetDrawBuffering( DRAWBUFFER_Quad, !(Surface.PolyFlags & PF_NoMultiBuffering) ) )
		Quad.Setup(Surface.PolyFlags, TexDrawList);


	// Buffer the quads
	DWORD dwColor = GET_COLOR_DWORD(Surface.Color);
	if ( !(Surface.PolyFlags & PF_AlphaBlend) )
		dwColor |= COLOR_NO_ALPHA;

	FTextureInfo& Info = *Surface.Texture;
	FLOAT UMult = 1.0f / (Info.UScale * Info.USize);
	FLOAT VMult = 1.0f / (Info.VScale * Info.VSize);

	for ( INT i=0; i<TileCount; i++ )
	{
		const FTileRect& Rect = TileList[i];

		FLOAT PX1 = Rect.X - Frame->FX2;
		FLOAT PX2 = PX1 + Rect.XL;
		FLOAT PY1 = Rect.Y - Frame->FY2;
		FLOAT PY2 = PY1 + Rect.YL;

		FLOAT RPX1 = m_RFX2 * PX1;
		FLOAT RPX2 = m_RFX2 * PX2;
		FLOAT RPY1 = m_RFY2 * PY1;
		FLOAT RPY2 = m_RFY2 * PY2;
		if ( !Frame->Viewport->IsOrtho() )
		{
			RPX1 *= Z;
			RPX2 *= Z;
			RPY1 *= Z;
			RPY2 *= Z;
		}

		FLOAT SU1 = (Rect.U)           * UMult;
		FLOAT SU2 = (Rect.U + Rect.UL) * UMult;
		FLOAT SV1 = (Rect.V)           * VMult;
		FLOAT SV2 = (Rect.V + Rect.VL) * VMult;

		void* Stream;
		Quad.StreamStart(Stream);
		StreamData(FGL::FVertex3D(RPX1, RPY1, Z), Stream);
		StreamData(FGL::FColorRGBA(dwColor), Stream);
		StreamData(FGL::FTextureUV(SU1, SV1), Stream);

		StreamData(FGL::FVertex3D(RPX2, RPY1, Z), Stream);
		StreamData(FGL::FColorRGBA(dwColor), Stream);
		StreamData(FGL::FTextureUV(SU2, SV1), Stream);

		StreamData(FGL::FVertex3D( RPX2, RPY2, Z), Stream);
		StreamData(FGL::FColorRGBA(dwColor), Stream);
		StreamData(FGL::FTextureUV(SU2, SV2), Stream);

		StreamData(FGL::FVertex3D( RPX1, RPY2, Z), Stream);
		StreamData(FGL::FColorRGBA(dwColor), Stream);
		StreamData(FGL::FTextureUV(SU1, SV2), Stream);
		Quad.StreamEnd(Stream);
	}
}

void UOpenGLRenderDevice::SetSceneNode_ARB( FSceneNode* Frame)
{
	// Vertex program env's
	if ( UseStaticGeometry && !SameCoords( Frame->Coords, SceneParams.Coords) )
	{
		// State tracked in device, not in GL1 interface
		GL1->SetVertexShaderEnv<true>( 0, Frame->Coords.Origin);
		GL1->SetVertexShaderEnv<true>( 1, Frame->Coords.XAxis);
		GL1->SetVertexShaderEnv<true>( 2, Frame->Coords.YAxis);
		GL1->SetVertexShaderEnv<true>( 3, Frame->Coords.ZAxis);
		SceneParams.Coords = Frame->Coords;
	}

	// Set viewport.
	GL1->SetViewport(Frame->XB, Viewport->SizeY - Frame->Y - Frame->YB, Frame->X, Frame->Y);

	// Set projection.
	SetTransformationMode( Frame->Viewport->IsOrtho() ? MATRIX_OrthoProjection : MATRIX_Projection, true);

	// Get NearClip.
	m_NearClip[0] = Frame->NearClip.X;
	m_NearClip[1] = Frame->NearClip.Y;
	m_NearClip[2] = Frame->NearClip.Z;
	m_NearClip[3] = Frame->NearClip.W != 0 ? -Frame->NearClip.W : 0; // Keep positive zero
}

void UOpenGLRenderDevice::FillScreen_ARB( const FOpenGLTexture* Texture, const FPlane* Color, DWORD PolyFlags)
{
	guard(FillScreen_ARB);

	FlushDrawBuffers();

	auto& Fill = FILL;

	// Buffer doesn't change, keeps code simple
	Fill.GetStreamBuffer()->Position = 0;
	Fill.BufferStride = sizeof(FGL::FVertex3D) + sizeof(FGL::FColorRGBA) + sizeof(FGL::FTextureUV);
	Fill.ActiveAttribs = Fill.BufferTypes;

	FLOAT XAdjust = Offscreen.IsActive() ? 0.5f / RequestedFramebufferWidth : 0.f;
	DWORD dwColor = Color ? FPlaneTo_RGBA(Color) : RGBA_MAKE(255,255,255,255);

	// TODO: Use VBO?

	// FillScreen does not wait for new draw calls or Unlock()
	void* Stream;
	Fill.StreamStart(Stream);
	StreamData(FGL::FVertex3D(-1,-1,0), Stream);
	StreamData(FGL::FColorRGBA(dwColor), Stream);
	StreamData(FGL::FTextureUV(0,0), Stream);

	StreamData(FGL::FVertex3D(1,-1,0), Stream);
	StreamData(FGL::FColorRGBA(dwColor), Stream);
	StreamData(FGL::FTextureUV(1.0f-XAdjust,0), Stream);

	StreamData(FGL::FVertex3D(1,1,0), Stream);
	StreamData(FGL::FColorRGBA(dwColor), Stream);
	StreamData(FGL::FTextureUV(1.0f-XAdjust,1), Stream);

	StreamData(FGL::FVertex3D(-1,1,0), Stream);
	StreamData(FGL::FColorRGBA(dwColor), Stream);
	StreamData(FGL::FTextureUV(0,1), Stream);
	Fill.StreamEnd(Stream);

	SetBlend(PolyFlags);

	FProgramID ProgramID = GetProgramID(PolyFlags) | FProgramID::FromVertexType(FGL::VertexProgram::Identity);
	if ( Texture ) ProgramID |= FGL::Program::Texture0;
	if ( Color )   ProgramID |= FGL::Program::Color0;

	if ( Texture )
	{
		GL1->TextureUnits.SetActive(0);
		GL1->TextureUnits.Bind(*Texture);
	}

	GL1->SetProgram(ProgramID);
	GL1->SetVertexArray(&Fill);
	GL1->SetEnabledClientTextures( Texture ? 1 : 0);
	GL1->SetEnabledClientStates(FOpenGL12::CS_VERTEX_ARRAY|FOpenGL12::CS_COLOR_ARRAY);

	FOpenGLBase::glDrawArrays( GL_TRIANGLE_FAN, 0, 4);

	unguard;
}

/*-----------------------------------------------------------------------------
	Buffer Flushing.

- Note on Complex Surface modes:
The Program ID is precomputed and used to control the state of the buffer.
Texture mapping units are instead bound depending on said ID.
-----------------------------------------------------------------------------*/

void UOpenGLRenderDevice::FlushDrawBuffers_ARB( DWORD BuffersToFlush)
{
	DWORD Flush = DrawBuffer.ActiveType & BuffersToFlush;

	if ( Flush & DRAWBUFFER_ComplexSurface )
		FlushDrawBuffer_ComplexSurface_ARB();

	if ( Flush & DRAWBUFFER_ComplexSurfaceVBO )
		FlushDrawBuffer_ComplexSurfaceVBO_ARB();

	if ( Flush & DRAWBUFFER_GouraudPolygon )
		FlushDrawBuffer_GouraudTriangles_ARB();

	if ( Flush & DRAWBUFFER_Decal )
		FlushDrawBuffer_Decal_ARB();

	if ( COMPLEX.PendingVolumetrics > 0 )
		FlushDrawBuffer_ComplexSurface_ARB();

	if ( COMPLEXSTATIC.PendingVolumetrics > 0 )
		FlushDrawBuffer_ComplexSurfaceVBO_ARB();

	if ( Flush & DRAWBUFFER_Line )
		FlushDrawBuffer_Line_ARB();

	if ( Flush & DRAWBUFFER_Quad )
		FlushDrawBuffer_Quad_ARB();

	DrawBuffer.ActiveType &= ~Flush;
}



void UOpenGLRenderDevice::FlushDrawBuffer_ComplexSurface_ARB()
{
	guard(UOpenGLRenderDevice::FlushDrawBuffer_ComplexSurface_ARB);

	auto& Complex = COMPLEX;

	// Nothing to draw
	if ( Complex.Polys == 0 )
		return;

	// Apply decals prior to drawing volumetrics
	if ( Complex.PendingVolumetrics && (DrawBuffer.ActiveType & DRAWBUFFER_Decal) )
		FlushDrawBuffer_Decal_ARB();

	SetDefaultAAState();

	FProgramID ProgramID(Complex.ProgramID);
	ProgramID &= ~(FGL::Program::Color0);
	DWORD PolyFlags = Complex.PolyFlags;


	// Handle program and blend mode
	if ( DrawBuffer.ActiveType & DRAWBUFFER_Decal )
	{
		// Decals in pipeline, draw without volumetrics (if present)
		if ( ProgramID & FGL::Program::Texture4 )
		{
			ProgramID &= ~(FGL::Program::Texture4/*|FGL::Program::ColorCorrection*/);
			Complex.PendingVolumetrics = 1;
		}
	}
	else if ( Complex.PendingVolumetrics )
	{
		// Draw volumetrics only
		checkSlow(ProgramID & FGL::Program::Texture4);
		// TODO: Figure out linear fog
		ProgramID &= ~(FGL::Program::Texture0|FGL::Program::Texture1|FGL::Program::Texture2|FGL::Program::Texture3|FGL::Program::DervMapped|FGL::Program::ColorCorrection|FGL::Program::ColorCorrectionAlpha|FGL::Program::Color0);
		if ( ColorCorrectionMode == CC_InShader ) 
			ProgramID |= Complex.ProgramID & FGL::Program::ColorCorrection; // Higor: Color combination is glitchy but better than nothing.
		PolyFlags = PF_Highlighted;
		m_rpSetDepthEqual = 1;
		FOpenGLBase::glDepthFunc(GL_EQUAL);
		Complex.PendingVolumetrics = 0;
	}
	// else { Draw all in single pass }

	GL1->SetTextures( Complex.Textures, ProgramID.GetTextures());
	GL1->SetProgram(ProgramID);
	if ( USES_AMBIENTLESS_LIGHTMAPS && (ProgramID & FGL::Program::Texture3) ) // Non-VBO doesn't need panning
	{
		INT ZoneId = Complex.ZoneId;
		GL1->SetVertexShaderEnv  ( 5, StaticBspData.ZoneAutoPan[ZoneId].UPan, StaticBspData.ZoneAutoPan[ZoneId].VPan);
		GL1->SetFragmentShaderEnv( 2, StaticBspData.ZoneAmbientPlane[ZoneId]);
	}
	if ( ProgramID & (FGL::Program::Texture0|FGL::Program::Texture1) )
	{
		FOpenGL12::glVertexAttrib4fARB( 6
			, Complex.TextureScale[TMU_Base].UScale
			, Complex.TextureScale[TMU_Base].VScale
			, Complex.TextureScale[TMU_DetailTexture].UScale
			, Complex.TextureScale[TMU_DetailTexture].VScale);
	}
	if ( ProgramID & (FGL::Program::Texture2) )
	{
		FOpenGL12::glVertexAttrib4fARB( 7
			, Complex.TextureScale[TMU_MacroTexture].UScale
			, Complex.TextureScale[TMU_MacroTexture].VScale
			, 0
			, 0);
	}
	SetBlend(PolyFlags);


	// Handle buffers and states
	GL1->SetVertexArray(&Complex);
	GL1->SetEnabledClientStates(FOpenGL12::CS_VERTEX_ARRAY);
	BYTE EnableBits = 0;
	if ( ProgramID & FGL::Program::Texture0 ) EnableBits |= 0b00000001; // Tex UV
	if ( ProgramID & FGL::Program::Texture3 ) EnableBits |= 0b00000010; // Light UV
	if ( ProgramID & FGL::Program::Texture4 ) EnableBits |= 0b00000100; // Fog UV
	GL1->SetEnabledClientTextures(EnableBits);


	// Draw
	FOpenGLBase::glMultiDrawArrays( GL_TRIANGLE_FAN, &Complex.PolyVertsStart(0), &Complex.PolyVertsCount(0), Complex.Polys);

	// Draw flat shaded surfaces
	if ( Complex.ProgramID & FGL::Program::Color0 )
	{
		SetBlend(PF_Highlighted);
		GL1->SetProgram( FProgramID::FromVertexType(FGL::VertexProgram::ComplexSurface) | FGL::Program::Color0);
		GL1->SetEnabledClientTextures(0b00000000);
		GL1->SetEnabledClientStates(FOpenGL12::CS_VERTEX_ARRAY|FOpenGL12::CS_COLOR_ARRAY);

		FOpenGLBase::glMultiDrawArrays( GL_TRIANGLE_FAN, &Complex.PolyVertsStart(0), &Complex.PolyVertsCount(0), Complex.Polys);
	}

	// Undo DepthFunc change
	if ( m_rpSetDepthEqual )
	{
		m_rpSetDepthEqual = 0;
		FOpenGLBase::glDepthFunc(GL_LEQUAL);
	}

	// End
	if ( Complex.PendingVolumetrics == 0 )
	{
		Complex.Polys = 0;
		Complex.PolyVertsStart.EmptyNoRealloc();
		Complex.PolyVertsCount.EmptyNoRealloc();
		Complex.GetStreamBuffer()->Finish();
		Complex.Textures[TMU_Base].PoolID = INDEX_NONE;
		DrawBuffer.ActiveType &= ~(DRAWBUFFER_ComplexSurface);
	}
	else
	{
		DrawBuffer.ActiveType &= ~DRAWBUFFER_ComplexSurface;
	}

	unguard;
}

void UOpenGLRenderDevice::FlushDrawBuffer_ComplexSurfaceVBO_ARB()
{
	guard(UOpenGLRenderDevice::FlushDrawBuffer_ComplexSurfaceVBO_ARB);

	auto& ComplexStatic = COMPLEXSTATIC;

	// Nothing to draw
	if ( ComplexStatic.Polys == 0 )
		return;

	// Apply decals prior to drawing volumetrics
	if ( ComplexStatic.PendingVolumetrics && (DrawBuffer.ActiveType & DRAWBUFFER_Decal) )
		FlushDrawBuffer_Decal_ARB();

	SetDefaultAAState();

	// StaticBSP
	if ( StaticBspData.UpdateWavy )
	{
		// State tracked in device, not GL1 interface
		StaticBspData.UpdateWavy = 0;
		GL1->SetFragmentShaderEnv<true>( 4, StaticBspData.WavyPan.UPan, StaticBspData.WavyPan.VPan, 0, StaticBspData.TimeSeconds);
	}

	// Handle buffers and states
	GL1->SetVertexArray(&ComplexStatic, bufferId_StaticGeometryVBO);
	GL1->SetEnabledClientStates(FOpenGL12::CS_VERTEX_ARRAY);
	GL1->SetEnabledClientTextures(0b00000111);

	DWORD PolyFlags = ComplexStatic.PolyFlags;
	FProgramID ProgramID = ComplexStatic.ProgramID;

	// Handle program and blend mode
	if ( DrawBuffer.ActiveType & DRAWBUFFER_Decal )
	{
		// Decals in pipeline, draw without volumetrics
		if ( ProgramID & FGL::Program::Texture4 )
		{
			ProgramID &= ~(FGL::Program::Texture4|FGL::Program::ColorCorrection);
			ComplexStatic.PendingVolumetrics = 1;
		}
	}
	else if ( ComplexStatic.PendingVolumetrics )
	{
		// Draw volumetrics only
		checkSlow(ProgramID & FGL::Program::Texture4);
		// TODO: Figure out linear fog
		ProgramID &= ~(FGL::Program::Texture0|FGL::Program::Texture1|FGL::Program::Texture2|FGL::Program::Texture3|FGL::Program::DervMapped|FGL::Program::ColorCorrection|FGL::Program::Color0);
		PolyFlags = PF_Highlighted;
		m_rpSetDepthEqual = 1;
		FOpenGLBase::glDepthFunc(GL_EQUAL);
		ComplexStatic.PendingVolumetrics = 0;
	}
//	else { Draw all in single pass }

	INT ZoneId = ComplexStatic.ZoneId;
	GL1->SetVertexShaderEnv  ( 5, StaticBspData.ZoneAutoPan[ZoneId].UPan, StaticBspData.ZoneAutoPan[ZoneId].VPan);
	GL1->SetFragmentShaderEnv( 2, StaticBspData.ZoneAmbientPlane[ZoneId]);
	if ( (ProgramID & (FGL::Program::Texture3|FGL::Program::Texture4)) )
	{
		INT LightUVAtlas = ComplexStatic.LightUVAtlas;
		INT FogUVAtlas   = ComplexStatic.FogUVAtlas;
		GL1->SetVertexShaderEnv( 6, 1-LightUVAtlas, LightUVAtlas, 1-FogUVAtlas, FogUVAtlas);
	}
	if ( ProgramID & (FGL::Program::Texture0|FGL::Program::Texture1) )
	{
		FOpenGL12::glVertexAttrib4fARB( 6
			, ComplexStatic.TextureScale[TMU_Base].UScale
			, ComplexStatic.TextureScale[TMU_Base].VScale
			, ComplexStatic.TextureScale[TMU_DetailTexture].UScale
			, ComplexStatic.TextureScale[TMU_DetailTexture].VScale);
	}
	if ( ProgramID & (FGL::Program::Texture2) )
	{
		FOpenGL12::glVertexAttrib4fARB( 7
			, ComplexStatic.TextureScale[TMU_MacroTexture].UScale
			, ComplexStatic.TextureScale[TMU_MacroTexture].VScale
			, 0
			, 0);
	}

	SetBlend(PolyFlags);
	GL1->SetProgram(ProgramID);
	GL1->SetTextures( ComplexStatic.Textures, ProgramID.GetTextures());

	// Draw
	FOpenGLBase::glMultiDrawArrays( GL_TRIANGLE_FAN, &ComplexStatic.PolyVertsStart(0), &ComplexStatic.PolyVertsCount(0), ComplexStatic.Polys);

	// Undo DepthFunc change
	if ( m_rpSetDepthEqual )
	{
		m_rpSetDepthEqual = 0;
		FOpenGLBase::glDepthFunc(GL_LEQUAL);
	}

	// End
	if ( !ComplexStatic.PendingVolumetrics )
	{
		ComplexStatic.Polys = 0;
		ComplexStatic.PolyVertsStart.EmptyNoRealloc();
		ComplexStatic.PolyVertsCount.EmptyNoRealloc();
		ComplexStatic.Textures[TMU_Base].PoolID = INDEX_NONE;
		ComplexStatic.Textures[TMU_LightMap].PoolID = INDEX_NONE;
		DrawBuffer.ActiveType &= ~(DRAWBUFFER_ComplexSurfaceVBO);
	}
	else
	{
		DrawBuffer.ActiveType &= ~DRAWBUFFER_ComplexSurfaceVBO;
	}

	unguard;
}

void UOpenGLRenderDevice::FlushDrawBuffer_GouraudTriangles_ARB()
{
	guard(UOpenGLRenderDevice::FlushDrawBuffer_GouraudTriangles_ARB);

	auto& Gouraud = GOURAUD;

	// Nothing to draw
	if ( Gouraud.GetStreamBuffer()->Position == 0 )
		return;

	SetDefaultAAState();

	// Set Blend
	DWORD PolyFlags = Gouraud.PolyFlags;
	SetBlend(PolyFlags);

	// Set Program
	FProgramID ProgramID = GetProgramID(PolyFlags);
	ProgramID |= GetProgramID(Gouraud.Textures, 2);
	ProgramID |= FProgramID::FromVertexType(FGL::VertexProgram::ComplexSurface);
	GL1->SetProgram(ProgramID);

	// Set Textures
	if ( ProgramID.GetTextures() )
	{
		FOpenGL12::glVertexAttrib4fARB( 6
			, Gouraud.TextureScale[TMU_Base].UScale
			, Gouraud.TextureScale[TMU_Base].VScale
			, Gouraud.TextureScale[TMU_DetailTexture].UScale
			, Gouraud.TextureScale[TMU_DetailTexture].VScale);
		GL1->SetTextures( Gouraud.Textures, ProgramID.GetTextures());
	}

	// Handle buffers
	GL1->SetVertexArray(&Gouraud);
	BYTE EnableBits =                           FOpenGL12::CS_VERTEX_ARRAY
	    | ((ProgramID & FGL::Program::Color0) ? FOpenGL12::CS_COLOR_ARRAY           : 0)
	    | ((ProgramID & FGL::Program::Color1) ? FOpenGL12::CS_SECONDARY_COLOR_ARRAY : 0);
	GL1->SetEnabledClientStates(EnableBits);
	GL1->SetEnabledClientTextures( (ProgramID & FGL::Program::Texture0) ? 1 : 0);

	// Enable NearClip plane
	if ( m_NearClip[3] != 0 )
	{
		FOpenGLBase::glEnable(GL_CLIP_DISTANCE0 + NumClipPlanes);		
		FOpenGL12::glClipPlane(GL_CLIP_DISTANCE0 + NumClipPlanes, m_NearClip);
		NumClipPlanes++;
	}

	// Draw
	FOpenGLBase::glDrawArrays( GL_TRIANGLES, 0, Gouraud.GetBufferedVerts());

	// Disable temporary states (Specular, NearClip)
	if ( m_NearClip[3] != 0 )
		FOpenGLBase::glDisable(GL_CLIP_DISTANCE0 + --NumClipPlanes);

	Gouraud.GetStreamBuffer()->Finish();
	DrawBuffer.ActiveType &= ~DRAWBUFFER_GouraudPolygon;

	unguard;
}

void UOpenGLRenderDevice::FlushDrawBuffer_Quad_ARB()
{
	guard(UOpenGLRenderDevice::FlushDrawBuffer_Quad_ARB);

	auto& Quad = QUAD;

	// Nothing to draw
	if ( Quad.GetStreamBuffer()->Position == 0 )
		return;

	if ( NoAATiles )
		SetDisabledAAState();
	else
		SetDefaultAAState();

	// Set Blend
	DWORD PolyFlags = Quad.PolyFlags;
	SetBlend(PolyFlags);

	// Set Program
	FProgramID ProgramID = GetProgramID(PolyFlags);
	ProgramID |= GetProgramID(Quad.Textures, 1);
	ProgramID |= FProgramID::FromVertexType(FGL::VertexProgram::Simple);
	GL1->SetProgram(ProgramID);

	// Set Textures
	if ( ProgramID.GetTextures() )
		GL1->SetTextures( Quad.Textures, ProgramID.GetTextures());

	// Handle buffers and states
	GL1->SetVertexArray(&Quad);
	BYTE EnableBits =                             FOpenGL12::CS_VERTEX_ARRAY
	    | ((ProgramID & FGL::Program::Color0)   ? FOpenGL12::CS_COLOR_ARRAY           : 0);
	GL1->SetEnabledClientStates(EnableBits);
	GL1->SetEnabledClientTextures( (ProgramID & FGL::Program::Texture0) ? 1 : 0 );

	// Draw
	FOpenGLBase::glDrawArrays(GL_QUADS, 0, Quad.GetBufferedVerts());

	Quad.GetStreamBuffer()->Finish();
	DrawBuffer.ActiveType &= ~DRAWBUFFER_Quad;

	unguard;
}

void UOpenGLRenderDevice::FlushDrawBuffer_Line_ARB()
{
	guard(UOpenGLRenderDevice::FlushDrawBuffer_Line_ARB);

	SetDefaultAAState();

	auto& Line = LINE;

	// Program does not need texturing
	FProgramID ProgramID = FProgramID::FromVertexType(FGL::VertexProgram::Simple);
	ProgramID |= FGL::Program::Color0;
	GL1->SetProgram(ProgramID);
	if ( Line.LineFlags & LINE_DepthCued )
		SetBlend(PF_Highlighted|PF_Occlude);
	else
		SetBlend(PF_Highlighted|PF_NoZReject);

	// Handle buffers and states
	GL1->SetVertexArray(&Line);
	GL1->SetEnabledClientStates(FOpenGL12::CS_VERTEX_ARRAY|FOpenGL12::CS_COLOR_ARRAY);
	GL1->SetEnabledClientTextures(0);

	// Draw
	FOpenGLBase::glDrawArrays(GL_LINES, 0, Line.GetBufferedVerts());

	Line.GetStreamBuffer()->Finish();
	DrawBuffer.ActiveType &= ~DRAWBUFFER_Line;

	unguard;
}

void UOpenGLRenderDevice::FlushDrawBuffer_Decal_ARB()
{
	guard(UOpenGLRenderDevice::FlushDrawBuffer_Decal_ARB);

	auto& Decal = DECAL;

	// Nothing to draw
	if ( Decal.GetStreamBuffer()->Position == 0 )
		return;

	// Complex surface buffer in pipeline, draw without volumetrics prior to drawing decals
	if ( (DrawBuffer.ActiveType & DRAWBUFFER_ComplexSurface) && !COMPLEX.PendingVolumetrics )
		FlushDrawBuffer_ComplexSurface_ARB();
	if ( (DrawBuffer.ActiveType & DRAWBUFFER_ComplexSurfaceVBO) && !COMPLEXSTATIC.PendingVolumetrics )
		FlushDrawBuffer_ComplexSurfaceVBO_ARB();

	SetDefaultAAState();

	// Set Blend
	DWORD PolyFlags = Decal.PolyFlags;
	SetBlend(PolyFlags);

	// Set Program
	FProgramID ProgramID = GetProgramID(PolyFlags);
	ProgramID |= GetProgramID(Decal.Textures, 1);
	ProgramID |= FProgramID::FromVertexType(FGL::VertexProgram::ComplexSurface);
	GL1->SetProgram(ProgramID);

	// Set Textures
	if ( ProgramID.GetTextures() )
	{
		FOpenGL12::glVertexAttrib4fARB( 6
			, Decal.TextureScale[TMU_Base].UScale
			, Decal.TextureScale[TMU_Base].VScale
			, 1.0f
			, 1.0f);
		GL1->SetTextures( Decal.Textures, /*ProgramID.GetTextures()*/1);
	}

	// Handle buffers
	GL1->SetVertexArray(&Decal);
	BYTE EnableBits =                           FOpenGL12::CS_VERTEX_ARRAY
		| ((ProgramID & FGL::Program::Color0) ? FOpenGL12::CS_COLOR_ARRAY           : 0);
	GL1->SetEnabledClientStates(EnableBits);
	GL1->SetEnabledClientTextures(1);

	// Enable NearClip plane
	if ( m_NearClip[3] != 0 )
	{
		FOpenGLBase::glEnable(GL_CLIP_DISTANCE0 + NumClipPlanes);		
		FOpenGL12::glClipPlane(GL_CLIP_DISTANCE0 + NumClipPlanes, m_NearClip);
		NumClipPlanes++;
	}

	// Draw
	FOpenGLBase::glDrawArrays( GL_TRIANGLES, 0, Decal.GetBufferedVerts());

	// Disable temporary states (Specular, NearClip)
	if ( m_NearClip[3] != 0 )
		FOpenGLBase::glDisable(GL_CLIP_DISTANCE0 + --NumClipPlanes);

	Decal.GetStreamBuffer()->Position = 0;
	DrawBuffer.ActiveType &= ~DRAWBUFFER_Decal;

	unguard;
}


/*-----------------------------------------------------------------------------
	Utils.
-----------------------------------------------------------------------------*/

//
// Returns the new Program ID if cannot merge draw calls.
//
FProgramID FASTCALL UOpenGLRenderDevice::MergeComplexSurfaceToDrawBuffer_ARB( const FGL::Draw::TextureList& TexDrawList, DWORD ZoneNumber)
{
	// Generate program ID
	FProgramID ProgramID = GetProgramID(TexDrawList) | FProgramID::FromVertexType(FGL::VertexProgram::ComplexSurface);

	// Check if same program
	if ( (ProgramID != COMPLEX.ProgramID) || (TexDrawList.PolyFlags[0] != COMPLEX.PolyFlags) )
		return ProgramID;

	// Check if same Texture, LightMap, FogMap
	if ( !CompareTextures(TexDrawList,COMPLEX.Textures) )
		return ProgramID;

	// Check if same zone for ambientless lights
	if ( USES_AMBIENTLESS_LIGHTMAPS && (COMPLEX.ZoneId != ZoneNumber) )
		return ProgramID;

	return 0;
}

//
// Returns the new Program ID if cannot merge draw calls.
//
FProgramID FASTCALL UOpenGLRenderDevice::MergeComplexSurfaceToVBODrawBuffer_ARB( const FGL::Draw::TextureList& TexDrawList, DWORD ZoneNumber)
{
	// Generate program ID
	FProgramID ProgramID = GetProgramID(TexDrawList) | FProgramID::FromVertexType(FGL::VertexProgram::ComplexSurfaceVBO);

	// Check if same program
	if ( (ProgramID != COMPLEXSTATIC.ProgramID) || (TexDrawList.PolyFlags[0] != COMPLEXSTATIC.PolyFlags) )
		return ProgramID;

	// Check if same Texture, LightMap, FogMap
	if ( !CompareTextures(TexDrawList, COMPLEXSTATIC.Textures) )
		return ProgramID;

#if USES_AMBIENTLESS_LIGHTMAPS
	// Check if same zone
	if ( COMPLEXSTATIC.ZoneId != ZoneNumber )
		return ProgramID;
#endif

	return 0;
}

INT FASTCALL UOpenGLRenderDevice::BufferStaticComplexSurfaceGeometry_VBO_ARB( FSurfaceFacet& Facet)
{
	auto& ComplexStatic = COMPLEXSTATIC;
	if ( ComplexStatic.PolyVertsStart.Num() != StaticBspData.NodeCount )
	{
		ComplexStatic.PolyVertsStart.SetSize( StaticBspData.NodeCount );
		ComplexStatic.PolyVertsCount.SetSize( StaticBspData.NodeCount );
	}

	INT Polys = ComplexStatic.Polys;
	for ( FSavedPoly* Poly=Facet.Polys; Poly; Poly=Poly->Next)
	{
		FBspNodeVBOMapping& Mapping = VBONodeMappings(Poly->iNode);
		if ( Mapping.VertexCount )
		{
			ComplexStatic.PolyVertsStart(Polys) = Mapping.VertexStart;
			ComplexStatic.PolyVertsCount(Polys) = Mapping.VertexCount;
			Polys++;
		}
	}
	INT Buffered = Polys - ComplexStatic.Polys;
	ComplexStatic.Polys = Polys;

	return Buffered;
}
