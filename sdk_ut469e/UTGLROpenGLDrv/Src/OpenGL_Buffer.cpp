/*=============================================================================
	OpenGL_Buffer.cpp: OpenGL buffer handling.

	Revision history:
	* Created by Fernando Velazquez
	* Static Complex Surface geometry buffering by Fernando Velazquez
=============================================================================*/

#include "OpenGLDrv.h"
#include "UTGLROpenGL.h"

/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

// Buffer names
GLuint UOpenGLRenderDevice::bufferId_StaticFillScreenVBO = 0;
GLuint UOpenGLRenderDevice::bufferId_StaticGeometryVBO   = 0;
GLuint UOpenGLRenderDevice::bufferId_GlobalRenderUBO     = 0;
GLuint UOpenGLRenderDevice::bufferId_StaticBspUBO        = 0;
GLuint UOpenGLRenderDevice::bufferId_TextureParamsUBO    = 0;
GLuint UOpenGLRenderDevice::bufferId_csTextureScaleUBO   = 0;

// Buffer data
FGL::VBO::FFill          UOpenGLRenderDevice::FillScreenData[4];
FStaticBsp_UBO           UOpenGLRenderDevice::StaticBspData{};
FTextureParams_UBO       UOpenGLRenderDevice::TextureParamsData{};
FComplexSurfaceScale_UBO UOpenGLRenderDevice::csTextureScaleData{};

TArray<FBspNodeVBOMapping> UOpenGLRenderDevice::VBONodeMappings;

TNumericTag<DWORD> FGL::DrawBuffer::FBase::GlobalIndex;

static TMapExt<FGL::FcsTextureScale,INT> csIndexMap;


/*-----------------------------------------------------------------------------
	Buffer Object abstractions.
-----------------------------------------------------------------------------*/

UOpenGLRenderDevice::FBuffers UOpenGLRenderDevice::Buffers{};

void FLightMapParams_TBO::Create()
{
	constexpr GLsizeiptr BufferSize = sizeof(StaticData);
	
	if ( FOpenGLBase::SupportsPersistentMapping )
	{
		Buffer.CreateInmutable(GL_TEXTURE_BUFFER, BufferSize, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
		Buffer.Mapped = FOpenGLBase::glMapNamedBufferRange(Buffer.Buffer, 0, BufferSize, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
	}
	else
	{
		Buffer.CreateMutable(GL_TEXTURE_BUFFER, BufferSize, nullptr, GL_STATIC_DRAW);
		ClientBuffer = appMalloc(BufferSize, TEXT("FLightMapParams_TBO"));
	}
}

void FLightMapParams_TBO::Destroy()
{
	if ( !Buffer )
		return;

	if ( Buffer.Inmutable() )
	{
		FOpenGLBase::glUnmapNamedBuffer(Buffer.Buffer);
		Buffer.Mapped = nullptr;
	}
	else
	{
		appFree(ClientBuffer);
		ClientBuffer = nullptr;
	}
	FOpenGLBase::DeleteBuffer(Buffer.Buffer);
}


/*-----------------------------------------------------------------------------
	Buffer object handling.
-----------------------------------------------------------------------------*/

void UOpenGLRenderDevice::UpdateBuffers()
{
	guard(UOpenGLRenderDevice::UpdateBuffers);
	if ( FOpenGLBase::SupportsVBO )
	{
		// Create/update static fillscreen VBO
		// TODO: Eliminate use of this buffer in GL3 mode
		if ( GetContextType() == CONTEXTTYPE_GL3 || GetContextType() == CONTEXTTYPE_GLES )
		{
			FLOAT UL = (Offscreen.IsActive() ? 1.0f - (0.5f / RequestedFramebufferWidth) : 1.f);
			if ( !bufferId_StaticFillScreenVBO )
			{
				FillScreenData[0].Vertex = FGL::FVertex3D(-1,-1,0);
				FillScreenData[0].UV     = FGL::FTextureUV(0,0);
				FillScreenData[1].Vertex = FGL::FVertex3D(1,-1,0);
				FillScreenData[1].UV     = FGL::FTextureUV(UL,0);
				FillScreenData[2].Vertex = FGL::FVertex3D(1,1,0);
				FillScreenData[2].UV     = FGL::FTextureUV(UL,1);
				FillScreenData[3].Vertex = FGL::FVertex3D(-1,1,0);
				FillScreenData[3].UV     = FGL::FTextureUV(0,1);
				FOpenGLBase::CreateBuffer(bufferId_StaticFillScreenVBO, GL_ARRAY_BUFFER, sizeof(FillScreenData), FillScreenData, GL_STATIC_DRAW);
			}
			else if ( FillScreenData[1].UV.U != UL )
			{
				FillScreenData[1].UV.U = UL;
				FillScreenData[2].UV.U = UL;
				FOpenGLBase::glBindBuffer(GL_ARRAY_BUFFER, bufferId_StaticFillScreenVBO);
				FOpenGLBase::glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(FillScreenData), FillScreenData);
				FOpenGLBase::glBindBuffer(GL_ARRAY_BUFFER, 0);
			}
		}
	}
	unguard;
}

void UOpenGLRenderDevice::DestroyBufferObjects()
{
	guard(UOpenGLRenderDevice::DestroyBufferObjects);
	FOpenGLBase::DeleteBuffer(bufferId_StaticFillScreenVBO);
	FOpenGLBase::DeleteBuffer(bufferId_StaticGeometryVBO);
	FOpenGLBase::DeleteBuffer(bufferId_TextureParamsUBO);
	FOpenGLBase::DeleteBuffer(bufferId_GlobalRenderUBO);
	FOpenGLBase::DeleteBuffer(bufferId_StaticBspUBO);
	FOpenGLBase::DeleteBuffer(bufferId_csTextureScaleUBO);
	Buffers.LightMapParams.Destroy();
	csIndexMap.Empty();
	csTextureScaleData.Elements.Empty();
	unguard;
}

/*-----------------------------------------------------------------------------
	DrawComplexSurface texture scale container
-----------------------------------------------------------------------------*/

INT UOpenGLRenderDevice::csGetTextureScaleIndexUBO( FGL::Draw::TextureList& TexList)
{
	INT MaxUnits = Min<INT>(TexList.Units, 3);

	FGL::FcsTextureScale Key;
	Key.X = (MaxUnits > 0 && TexList.Infos[0]) ? TexList.Infos[0]->UScale : 1.0f;
	Key.Y = (MaxUnits > 1 && TexList.Infos[1]) ? TexList.Infos[1]->UScale : 1.0f;
	Key.Z = (MaxUnits > 2 && TexList.Infos[2]) ? TexList.Infos[2]->UScale : 1.0f;
	Key.W = 1.0f;

	INT* Index = csIndexMap.Find(Key);
	if ( Index == nullptr )
	{
		GL_DEV_CHECK(csTextureScaleData.Elements.Num() > 0);

		// Index is assigned incrementally
		// If the uniform is full, reset indices and keep old garbage data
		// this will prevent some of the already queued draws from having bad scaling
		INT NewIndex = csIndexMap.Num();
		if ( NewIndex >= csTextureScaleData.Elements.Num() )
		{
			csIndexMap.Empty();
			NewIndex = 0;
		}

		csTextureScaleData.PendingUpdate = 1;
		csTextureScaleData.Elements(NewIndex) = Key;
		Index = &csIndexMap.SetNoFind(Key, NewIndex);
	}
	return *Index;
}

void UOpenGLRenderDevice::csUpdateTextureScaleUBO()
{
	if ( !bufferId_csTextureScaleUBO )
	{
		INT ArraySize = FComplexSurfaceScale_UBO::ArraySize();
		GLsizeiptr BufferSize = sizeof(FComplexSurfaceScale_UBO::FElement) * ArraySize;

		csTextureScaleData.Elements.SetSize(ArraySize);
		FOpenGLBase::CreateBuffer(bufferId_csTextureScaleUBO, GL_UNIFORM_BUFFER, BufferSize, nullptr, GL_DYNAMIC_DRAW);
	}
	FOpenGLBase::glBindBufferBase(GL_UNIFORM_BUFFER, FComplexSurfaceScale_UBO::UniformIndex, bufferId_csTextureScaleUBO);
	csTextureScaleData.BufferElements();
	FOpenGLBase::glBindBuffer(GL_UNIFORM_BUFFER, 0);
}


/*-----------------------------------------------------------------------------
	Static BSP - VBO.
-----------------------------------------------------------------------------*/

static QWORD GeometryID = 0;

#if RENDER_DEVICE_OLDUNREAL == 469

# define RENDER_API 
# include "../../Render/Inc/UnAtlas.h"

// Higor: move these to render device if we're using this in Unreal Editor

void UOpenGLRenderDevice::SetStaticBsp( FStaticBspInfoBase& StaticBspInfo)
{
	guard(UOpenGLRenderDevice::SetStaticBsp);

	StaticBspInfo.Update();
//	StaticBspData.TimeSeconds = StaticBspInfo.Level->TimeSeconds.GetFloat();

	// May not change
	for ( INT i=0; i<FBspNode::MAX_ZONES; i++)
		if ( StaticBspData.ZoneAmbientColor[i] != StaticBspInfo.ZoneAmbientColor[i] )
		{
			StaticBspData.UpdateAmbient = true;
			StaticBspData.ZoneAmbientColor[i] = StaticBspInfo.ZoneAmbientColor[i];
			StaticBspData.ZoneAmbientPlane[i] = StaticBspInfo.ZoneAmbientColor[i].Plane();
		}

	// Always changes, but needed for ARB mode to update just once.
	StaticBspData.UpdateWavy = true; 
	StaticBspData.WavyPan.UPan = StaticBspInfo.WavyPanU;
	StaticBspData.WavyPan.VPan = StaticBspInfo.WavyPanV;

	// Always changes
	StaticBspData.UpdateAutoPan = true;
	for ( INT i=0; i<FBspNode::MAX_ZONES; i++)
		StaticBspData.ZoneAutoPan[i] = FGL::FTexturePan(StaticBspInfo.ZoneAutoPanU[i], StaticBspInfo.ZoneAutoPanV[i]);

	// GLSL mode
	if ( bufferId_StaticBspUBO && StaticBspData.UpdateFlags )
	{
		FOpenGLBase::glBindBuffer(GL_UNIFORM_BUFFER, bufferId_StaticBspUBO);
		StaticBspData.BufferWavyTime();
		if ( StaticBspData.UpdateAmbient )
			StaticBspData.BufferAmbientPlaneArray();
		StaticBspData.BufferAutoPanArray();
		FOpenGLBase::glBindBuffer(GL_UNIFORM_BUFFER, 0);

		StaticBspData.UpdateFlags = 0;
	}

	if ( GIsEditor )
		return;

	// Prevent use of static geometry if requirements are not met
	if ( ContextType == CONTEXTTYPE_GL3 || ContextType == CONTEXTTYPE_GLES )
	{
		if ( !FOpenGLBase::SupportsTBO || FOpenGLBase::MaxTextureBufferSize < 2*1024*1024 )
			return;
	}

	if ( !UseStaticGeometry )
	{
		if ( GeometryID )
		{
			FlushStaticGeometry();
			StaticBspInfo.ComputeStaticGeometry( EStaticBspMode::STATIC_BSP_None );
		}
		if ( Buffers.LightMapParams.Buffer )
			Buffers.LightMapParams.Destroy();
		return;
	}

	bool UpdateGeometry = (StaticBspInfo.SourceGeometryChanged || StaticBspInfo.GeometryID != GeometryID);

	if ( UpdateGeometry && FOpenGLBase::SupportsVBO )
	{
		DOUBLE StartTime = appSecondsNew();
		
		// Clear GPU cache
		FlushStaticGeometry();

		// Set persistent state
		StaticBspInfo.SourceGeometryChanged = 0;
		GeometryID = StaticBspInfo.GeometryID;

		// Compute new geometry
#if UTGLR_VBOSURF_TEST
		StaticBspInfo.ComputeStaticGeometry( EStaticBspMode::STATIC_BSP_PerSurf );
#else
		StaticBspInfo.ComputeStaticGeometry( EStaticBspMode::STATIC_BSP_PerNode );
#endif

		// This render device prefers precalculated light/atlas UV's
		StaticBspInfo.ComputeLightUV();

		// Prepare Vertex data (per-node version)
		VBONodeMappings.Empty();
		VBONodeMappings.AddZeroed( StaticBspInfo.NodeList.Num() );
		StaticBspData.NodeCount = StaticBspInfo.NodeList.Num();
		StaticBspData.VertexCount = StaticBspInfo.VertList.Num();
		TArray<FGL::VBO::FStaticBsp> VBOVertices( StaticBspInfo.VertList.Num() );

		for ( INT n=0; n<StaticBspInfo.NodeList.Num(); n++)
		{
			FStaticBspNode& Node = StaticBspInfo.NodeList(n);
			check(StaticBspInfo.SurfList.IsValidIndex(Node.SurfaceIndex)); // checkSlow
			FStaticBspSurf& Surf = StaticBspInfo.SurfList(Node.SurfaceIndex);
			for ( INT v=Node.VertexStart, vEnd=v+Node.VertexCount; v<vEnd; v++)
			{
				FStaticBspVertex    & SrcVert = StaticBspInfo.VertList(v);
				FGL::VBO::FStaticBsp& DstVert = VBOVertices(v);
				DstVert.Vertex       = FGL::FVertex3D(SrcVert.Point);
				DstVert.TextureUVPan = FGL::FTextureUVPan(SrcVert.TextureU, SrcVert.TextureV, (FLOAT)Surf.PanU, (FLOAT)Surf.PanV);
				DstVert.LightUV      = FGL::FTextureUV(SrcVert.LightU, SrcVert.LightV);
				DstVert.AtlasUV      = FGL::FTextureUV(SrcVert.AtlasU, SrcVert.AtlasV);
				DstVert.AutoU        = (Surf.PolyFlags & PF_AutoUPan)  ? 1 : 0;
				DstVert.AutoV        = (Surf.PolyFlags & PF_AutoVPan)  ? 1 : 0;
				DstVert.Wavy         = (Surf.PolyFlags & PF_SmallWavy) ? 1 : 0;
			}
			VBONodeMappings(n).VertexStart = Node.VertexStart;
			VBONodeMappings(n).VertexCount = Node.VertexCount;
		}

		// Create Vertex Buffer Object
		if ( VBOVertices.Num() )
		{
			if ( FOpenGLBase::SupportsPersistentMapping )
				FOpenGLBase::CreateInmutableBuffer(bufferId_StaticGeometryVBO, GL_ARRAY_BUFFER, sizeof(FGL::VBO::FStaticBsp)*VBOVertices.Num(), &VBOVertices(0).Vertex, 0);
			else
				FOpenGLBase::CreateBuffer(bufferId_StaticGeometryVBO, GL_ARRAY_BUFFER, sizeof(FGL::VBO::FStaticBsp)*VBOVertices.Num(), &VBOVertices(0).Vertex, GL_STATIC_DRAW);
		}	
		else
			VBONodeMappings.Empty();

		DOUBLE EndTime = appSecondsNew();
		debugf( NAME_Init, TEXT("[GL] Buffered %i Static BSP vertices (time=%f)"), VBOVertices.Num(), EndTime-StartTime);
	}

	if ( UpdateGeometry && FOpenGLBase::SupportsTBO )
	{
		DOUBLE StartTime = appSecondsNew();

		// Create Texture Buffer Object
		Buffers.LightMapParams.Create();

		bool PersistentMapped = Buffers.LightMapParams.Buffer.Mapped != nullptr;
		FLightMapParams_TBO::StaticData* Data = PersistentMapped ? 
			(FLightMapParams_TBO::StaticData*)Buffers.LightMapParams.Buffer.Mapped :
			(FLightMapParams_TBO::StaticData*)Buffers.LightMapParams.ClientBuffer;
		check(Data);

		// Write raw lightmap info into the staging/mapped memory
		for ( INT n=0; n<StaticBspInfo.NodeList.Num(); n++)
		{
			FStaticBspNode& Node = StaticBspInfo.NodeList(n);
			check(StaticBspInfo.SurfList.IsValidIndex(Node.SurfaceIndex)); // checkSlow
			FStaticBspSurf& Surf = StaticBspInfo.SurfList(Node.SurfaceIndex);

			if ( StaticBspInfo.LightMaps.IsValidIndex(Surf.iLightMap) )
			{
				FLightMapIndex& LM = StaticBspInfo.LightMaps(Surf.iLightMap);
				Data->Regular[n] = FGL::FTextureUVPan(LM.Pan.X, LM.Pan.Y, LM.UScale, LM.VScale);
			}
			if ( StaticBspInfo.AtlasMaps.IsValidIndex(Surf.iLightMap) )
			{
				FLightMapIndex& LM = StaticBspInfo.AtlasMaps(Surf.iLightMap);
				Data->Atlased[n] = FGL::FTextureUVPan(LM.Pan.X, LM.Pan.Y, LM.UScale, LM.VScale);
			}
		}

		// Push data to the GPU
		GLintptr   Offsets[] = {0, sizeof(FLightMapParams_TBO::StaticData)/2};
		GLsizeiptr Length    = StaticBspInfo.NodeList.Num() * sizeof(FGL::FTextureUVPan);

		FOpenGLBase::glBindBuffer(GL_TEXTURE_BUFFER, Buffers.LightMapParams.Buffer.Buffer);
		if ( PersistentMapped )
		{
			FOpenGLBase::glFlushMappedBufferRange(GL_TEXTURE_BUFFER, Offsets[0], Length);
			FOpenGLBase::glFlushMappedBufferRange(GL_TEXTURE_BUFFER, Offsets[1], Length);
		}
		else
		{
			FOpenGLBase::glBufferSubData(GL_TEXTURE_BUFFER, Offsets[0], Length, Data->Regular);
			FOpenGLBase::glBufferSubData(GL_TEXTURE_BUFFER, Offsets[1], Length, Data->Atlased);
		}
		FOpenGLBase::glBindBuffer(GL_TEXTURE_BUFFER, 0);

		DOUBLE EndTime = appSecondsNew();
		debugf( NAME_Init, TEXT("[GL] Buffered %i Static per-node LightMaps and AtlasMaps (time=%f)"), StaticBspInfo.NodeList.Num(), EndTime-StartTime);

		// Link texture to buffer
		if ( TexturePool.Textures.Num() == 0 )
			appErrorf(TEXT("Texture pool not yet initialized!"));
		else
		{
			GL->TextureUnits.SetActive(TMU_LightMapParams);
			FOpenGLBase::glBindTexture(GL_TEXTURE_BUFFER, TexturePool.Textures(FGL::TEXPOOL_ID_LightMapParams).Texture);
			FOpenGLBase::glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, Buffers.LightMapParams.Buffer.Buffer);
		}
	}

	unguard;
}

#else
void UOpenGLRenderDevice::SetStaticBsp( FStaticBspInfoBase& StaticBspInfo) {}
#endif

/*-----------------------------------------------------------------------------
	Static geometry - VBO.
-----------------------------------------------------------------------------*/

UBOOL FASTCALL UOpenGLRenderDevice::CanBufferStaticComplexSurfaceGeometry_VBO( FSurfaceInfo& Surface, FSurfaceFacet& Facet)
{
	if ( !UseStaticGeometry )
		return false;

	for ( FSavedPoly* Poly=Facet.Polys; Poly; Poly=Poly->Next)
		if ( !VBONodeMappings.IsValidIndex(Poly->iNode) )
			return false;
	return true;
}


void UOpenGLRenderDevice::FlushStaticGeometry()
{
	if ( GeometryID )
	{
		FOpenGLBase::DeleteBuffer(bufferId_StaticGeometryVBO);
		GeometryID = 0;
		VBONodeMappings.Empty();

		Buffers.LightMapParams.Destroy();
	}
}
