/*=============================================================================
	UnDrawStaticMesh.h: Unreal StaticMesh implementation
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Smirftsch
	Note:
		* Included by UnMeshRn.cpp.
=============================================================================*/

//
// Structure used by DrawLodMesh for sorting triangles.
//
struct FMeshSMSort
{
	FStaticMeshTri* Tri;
	INT Key,VertCode;
};

constexpr INT INVALID_LIGHT = -69;
constexpr INT UNLIT_LIGHT = -70;

QSORT_RETURN CDECL CompareSMFaceKey( const FMeshSMSort* A, const FMeshSMSort* B )
{
	return B->Key - A->Key;
}

void URender::DrawStaticMesh(FSceneNode* Frame, FDynamicSprite* Sprite, const FCoords& Coords, DWORD ExtraFlags)
{
	AActor* Owner = Sprite->Actor;

	guard(URender::DrawStaticMesh);
	FScopedMemMark Mark(GMem);
	UStaticMesh* Mesh = (UStaticMesh*)Owner->Mesh;
	UStaticMesh* ActualMesh = Mesh;
	INT iLodLevel = INDEX_NONE;
	URenderDevice* RenDev = Frame->Viewport->RenDev;
	const UBOOL bIsSoftware = (RenDev->SpanBased != 0);
	UBOOL bIsWhiteView = (Frame->Viewport->Actor->RendMap == REN_LightingOnly);
	UBOOL bFadeView = FALSE;
	FLOAT FadeAlpha;

	if (!Frame->Viewport->IsOrtho() && Owner->VisibilityRadius != 0.f && !(ExtraFlags & PF_Modulated))
		bFadeView = Owner->ShouldDistanceFade(Frame->Coords.Origin, &FadeAlpha);

	// Get Low Detail Model
	if( Mesh->SMLodLevels.Num() )
	{
		FLOAT RendZ = Frame->Proj.Z / Max<FLOAT>(Owner->Location.TransformPointBy(Frame->Coords).Z,0.1f);
		Mesh = Mesh->GetDisplayMesh(RendZ);
		if( Mesh!=ActualMesh )
		{
			for( iLodLevel=0; iLodLevel<ActualMesh->SMLodLevels.Num(); ++iLodLevel )
				if( ActualMesh->SMLodLevels(iLodLevel).Mesh==Mesh )
					break;
		}
	}
	Mesh->SMTris.Load();
	if (!Mesh->SMTris.Num()) // Sanity check.
		return;

	// Get transformed verts.
	UBOOL bWire = (Frame->Viewport->IsOrtho() || Frame->Viewport->Actor->RendMap==REN_Wire);
	FTransTexture* Samples=NULL;
	Samples = New<FTransTexture>(GMem,Mesh->FrameVerts);
	Mesh->GetFrame( &Samples->Point, sizeof(Samples[0]), bWire ? GMath.UnitCoords : Coords, Owner );

	if (!Mesh->bUV2MapInit)
		Mesh->InitUV2Mapping();

	 // Check pre-computed normals
	FCoords CompNormalCoords;
	bool bSupportPreComp = (Mesh->GetClass()==UStaticMesh::StaticClass());
	if( bSupportPreComp )
	{
		if (Owner->RenderInterpolate)
		{
			FRotator RenderRot;
			Owner->RenderInterpolate->GetRenderPos(NULL, &RenderRot);
			CompNormalCoords = Coords * RenderRot;
		}
		else CompNormalCoords = Coords * Owner->Rotation;
		CompNormalCoords = (CompNormalCoords * FScale(Owner->DrawScale3D)).TransposeAdjoint();
		FLOAT DetMulti = Abs<FLOAT>(DivSqrtApprox(CompNormalCoords.Determinant())) * Frame->Mirror;
		CompNormalCoords.XAxis*=DetMulti;
		CompNormalCoords.YAxis*=DetMulti;
		CompNormalCoords.ZAxis*=DetMulti;
		if( !Mesh->SMNormals.Num() )
			Mesh->CalcSMNormals();
	}

	// Compute outcodes.
	BYTE Outcode = FVF_OutReject;
	guardSlow(Outcode);
	for( INT i=0; i<Mesh->FrameVerts; i++ )
	{
		*reinterpret_cast<INT*>(&Samples[i].Normal.W) = INVALID_LIGHT;
		Samples[i].ComputeOutcode( Frame );
		Outcode &= Samples[i].Flags;
	}
	unguardSlow;

	// Render a wireframe view or textured view.
	if( bWire )
	{
		// Render each wireframe triangle.
		guardSlow(RenderWire);
		FPlane Color = (Owner->bIsMover ? (Owner->bSelected ? FPlane(1.f,0.3f,1.f,1) : FPlane(.7f,0.2f,.7f,1))
							 : (Owner->bSelected ? FPlane(.3f,1.f,.3f,1) : FPlane(.1f,.4f,.1f,1)));
		for( INT i=0; i<Mesh->SMTris.Num(); i++ )
		{
			FStaticMeshTri& Tri    = Mesh->SMTris(i);
			FVector*  P1     = &Samples[Tri.iVertex[2]].Point;
			for( int j=0; j<3; j++ )
			{
				FVector* P2 = &Samples[Tri.iVertex[j]].Point;
				RenDev->Draw3DLine( Frame, Color, LINE_DepthCued, *P1, *P2 );
				P1 = P2;
			}
		}
		unguardSlow;
		return;
	}

	// Mesh based particle effects.
	if( Owner->bParticles )
	{
		guardSlow(Particles);
#if !__PSX2_EE__
		if( !Owner->Texture )
			return;
		UTexture* Tex = Owner->Texture->Get();
		FTransform** SortedPts = New<FTransform*>(GMem,Mesh->FrameVerts);
		INT Count=0;
		const FLOAT Unlit = Owner->ScaleGlow;
		FPlane Color(Unlit, Unlit, Unlit, 1.f);
		if( Owner->ScaleGlow!=1.0 )
		{
			Color *= Owner->ScaleGlow;
			if( Color.X>1.0 ) Color.X=1.0;
			if( Color.Y>1.0 ) Color.Y=1.0;
			if( Color.Z>1.0 ) Color.Z=1.0;
		}
		INT i;
		for( i=0; i<Mesh->FrameVerts; i++ )
		{
			if( !Samples[i].Flags && Samples[i].Point.Z>1.0 )
			{
				Samples[i].Project( Frame );
				SortedPts[Count++] = &Samples[i];
			}
		}
		if (bIsSoftware)
		{
			Sort( SortedPts, Count );
		}
		for( i=0; i<Count; i++ )
		{
			if( !SortedPts[i]->Flags )
			{
				UTexture* SavedNext = NULL;
				UTexture* SavedCur = NULL;
				if ( Owner->bRandomFrame )
				{
					// pick texture from multiskins
					//Tex = Owner->MultiSkins[appCeil(((size_t)SortedPts[i]-(size_t)Samples)/3.f)&7];
					//Tex = Owner->MultiSkins[appRand() % 8];
					Tex = Owner->MultiSkins[(appCeil(i*(88.f / 17.f))) % 8];
					if (!Tex)
						Tex = Owner->Texture;

					INT frameCount=1;
					for( UTexture* Test=Tex->AnimNext; Test && Test!=Tex; Test=Test->AnimNext )
						frameCount++;
					INT Num = Clamp( appFloor(Owner->LifeFraction()* frameCount), 0, frameCount -1 );
					while( Num-- > 0 )
						Tex = Tex->AnimNext;
					SavedNext         = Tex->AnimNext;//sort of a hack!!
					SavedCur          = Tex->AnimCurrent;
					Tex->AnimNext = NULL;
					Tex->AnimCurrent = NULL;
				}
				if( Tex )
				{
					FLOAT XSize = SortedPts[i]->RZ * Tex->USize * Owner->DrawScale;
					FLOAT YSize = SortedPts[i]->RZ * Tex->VSize * Owner->DrawScale;
					Frame->Viewport->Canvas->DrawIcon
					(
						Tex,
						SortedPts[i]->ScreenX - XSize/2,
						SortedPts[i]->ScreenY - YSize/2,
						XSize,
						YSize,
						Sprite->SpanBuffer,
						Samples[i].Point.Z,
						Color,
						FPlane(0,0,0,0),
						ExtraFlags | PF_TwoSided | Tex->PolyFlags
					);
					Tex->AnimNext = SavedNext;
					Tex->AnimCurrent = SavedCur;
				}
			}
		}
#endif
		unguardSlow;
		return;
	}

	// Set up triangles.
	INT VisibleTriangles = 0;
	HasSpecialCoords = 0;
	FMeshSMSort* TriPool=NULL;
	FVector* TriNormals=NULL;
	DWORD PolyFlags=0;
	INT TriCount=Mesh->SMTris.Num();
	const UBOOL bSortFaces = (bIsSoftware || (bFadeView && !(ExtraFlags & (PF_Translucent | PF_Modulated))) || (ExtraFlags & PF_AlphaBlend));
	UBOOL bShouldSortFaces = bSortFaces;

	if( Outcode == 0 )
	{
		// Process triangles.
		guardSlow(Process);
		TriPool    = New<FMeshSMSort>(GMem,TriCount);
		TriNormals = (bSupportPreComp ? &Mesh->SMNormals(0) : New<FVector>(GMem,TriCount));
		INT LastGroup = INDEX_NONE;

		// Set up list for triangle sorting, adding all possibly visible triangles.
		FMeshSMSort* TriTop = &TriPool[0];
		for( INT i=0; i<TriCount; i++ )
		{
			FStaticMeshTri& Tri = Mesh->SMTris(i);
			if (LastGroup != Tri.GroupIndex)
			{
				const FStaticMeshTexGroup& Group = Mesh->SMGroups(Tri.GroupIndex);
				LastGroup = Tri.GroupIndex;
				PolyFlags = ExtraFlags | Group.RealPolyFlags;
				bShouldSortFaces |= ((PolyFlags & PF_AlphaBlend) != 0);
			}
			if (PolyFlags & PF_Invisible)
				continue;
			FTransform& V1 = Samples[Tri.iVertex[0]];
			FTransform& V2 = Samples[Tri.iVertex[1]];
			FTransform& V3 = Samples[Tri.iVertex[2]];
			

			// Compute triangle normal.
			FVector SurfNormal;
			if( bSupportPreComp )
				SurfNormal = TriNormals[i].TransformVectorBy(CompNormalCoords);
			else
			{
				TriNormals[i] = (V1.Point-V2.Point) ^ (V3.Point-V1.Point).NormalApprox() * Frame->Mirror;
				SurfNormal = TriNormals[i];
			}

			// See if potentially visible.
			if( !(V1.Flags & V2.Flags & V3.Flags) && ((PolyFlags & PF_TwoSided) || (V1.Point|SurfNormal)<0.0) )
			{
				// This is visible.
				TriTop->Tri = &Tri;
				TriTop->VertCode = (i*3);

				// Set the sort key.
				if (bSortFaces || (PolyFlags & PF_AlphaBlend))
					TriTop->Key = appRound( V1.Point.Z + V2.Point.Z + V3.Point.Z );
				else TriTop->Key = 1; // Sort front.

				// Add to list.
				VisibleTriangles++;
				TriTop++;
			}
		}
		unguardSlow;
	}

	UStaticLightData* D = NULL;
	if (Owner->IsRegisteredObject())
	{
		FStaticLightActor* LD = Owner->GetStaticLights();
		if (LD)
		{
			D = LD->GetStaticMeshData();
			if (D && iLodLevel >= 0 && iLodLevel < D->LODLevels.Num())
				D = D->LODLevels(iLodLevel);
		}
	}
	bool bHasLightData = (D && D->LightDataValid(Mesh));

	// Render triangles.
	if( VisibleTriangles>0 )
	{
		STAT(GStatRender.MeshPolyCount.Calls += VisibleTriangles);

		guardSlow(Render);
		UBOOL bMirrorMesh = (Frame->Mirror < 0.f);

		// Sort by depth.
		if(bSortFaces || bShouldSortFaces)
			appQsort( TriPool, VisibleTriangles, sizeof(FMeshSMSort), (QSORT_COMPARE)CompareSMFaceKey );

		// Lock the textures.
		LOCK_TEXTURES();

		// Init distance fog
		INIT_DISTANCE_FOG(Mesh->FrameVerts, Samples, Point.Z);

		// Build list of all incident lights on the mesh.
		LIGHT_SETUPFORACTOR;
		GBaseSpecularity = 0.f; // Static meshes shouldn't be effected by level bSpecularLight option!

		INT OldIndex=INDEX_NONE;
		FLOAT UInner=0.f,VInner=0.f;
		bool bHasDetailTex=false;
		FPlane* GBaseColors = (bHasLightData && D->ColorData.StaticLights.Num()) ? &D->ColorData.StaticLights(0) : NULL;
		BYTE* GBaseFlags = (bHasLightData && D->DynLightSources.Num()) ? &D->DynLightSources(0) : NULL;
		BYTE bSingleLightMap = (bHasLightData && D->bSingleLightmap);

		// Draw groups of triangles.
		guardSlow(DrawVisible);
		//debugf(TEXT("NoClipRender"));
		RenDev->BeginMeshDraw(Frame,Owner);
		INT i, j, NumPolies = 0;
		UBOOL bFlatShaded = 0;
		FVector ThisSurfNormal;
		DWORD ExtraDrawFlags = 0;

		UBOOL bDrawUV2LightMap = FALSE;
		if (!bIsSoftware && bHasLightData && Mesh->UseUV2Lightmap() && !bFadeView && !(ExtraFlags & (PF_Translucent | PF_AlphaBlend | PF_Unlit)) && D->Lightmap
			&& ((D->Lightmap->DetailLevel <= 0) || (D->Lightmap->DetailLevel == Mesh->LightmapDetail)))
		{
			bDrawUV2LightMap = TRUE;
		}

		for( i=0; i<VisibleTriangles; i++ )
		{
			// Set up the triangle.
			const FStaticMeshTri& Tri = *TriPool[i].Tri;
			const FStaticMeshTexGroup& Group = Mesh->SMGroups(Tri.GroupIndex);

			if( OldIndex!=Tri.GroupIndex )
			{
				OldIndex = Tri.GroupIndex;

				// Draw previous group.
				if( NumPolies )
				{
					DRAW_FOGGED_SURFLIST(*Info, NumPolies, (PolyFlags | ExtraDrawFlags));
					NumPolies = 0;
				}

				// Get texture.
				PolyFlags = Group.RealPolyFlags | ExtraFlags;
				GET_TEXTURE_INFO(Group.Texture, PolyFlags);

				GLightSpecular = Info->Texture->Specular;
				GLightSuperSpecular = Info->Texture->SuperGlow;
				UInner = Info->UScale * Info->USize;
				VInner = Info->VScale * Info->VSize;
					
				bFlatShaded = (bSupportPreComp && Group.SmoothGroup==INDEX_NONE);
				if( !bHasDetailTex && Info->Texture->DetailTexture )
					bHasDetailTex = true;
				PolyFlags = Group.RealPolyFlags | ExtraFlags;

				if (bFadeView)
				{
					if (PolyFlags & PF_Translucent)
						ExtraDrawFlags = 0;
					else ExtraDrawFlags = PF_AlphaBlend;
				}
			}

			// Set up texture coords.
			if( bFlatShaded )
				ThisSurfNormal = Tri.EdgeNormals[0].TransformVectorBy(CompNormalCoords).NormalApprox();

			for( j=0; j<3; j++ )
			{
				FTransTexture& Vert = Samples[Tri.iVertex[j]];
				FTransTexture& VertX = MeshVertList[NumPolies++];
				VertX.U = Tri.Tex[j].U * UInner;
				VertX.V = Tri.Tex[j].V * VInner;

				// Check if smoothing group has changed.
				INT& LightGroup = *reinterpret_cast<INT*>(&Vert.Normal.W);

				if (bDrawUV2LightMap)
				{
					if (PolyFlags & (PF_Translucent | PF_AlphaBlend))
					{
						if (bFlatShaded || LightGroup != OldIndex)
						{
							if (bFlatShaded)
							{
								Vert.Normal = ThisSurfNormal;
								LightGroup = INDEX_NONE;
							}
							else
							{
								Vert.Normal = Tri.EdgeNormals[j].TransformVectorBy(CompNormalCoords).NormalApprox();
								LightGroup = OldIndex;
							}

							guardSlow(Light);
							if (bBeyondFog)
							{
								Vert.Light = FPlane(0.f, 0.f, 0.f, 0.f);
								Vert.Fog = FogSurface.FogColor;
							}
							else
							{
								//debugf(TEXT("LightTyp!=3"));
								// Compute effect of each lightsource on this vertex.
								if (bHasLightData)
								{
									GVisibleLightFlags = GBaseFlags ? GBaseFlags[bSingleLightMap ? Tri.iVertex[j] : TriPool[i].VertCode + j] : 0;
									GBaseColour = GBaseColors ? GBaseColors[bSingleLightMap ? Tri.iVertex[j] : TriPool[i].VertCode + j] : FVector(0, 0, 0);
								}
								PolyFlags |= GLightManager->Light(Vert, PolyFlags);

								SCALE_LIGHT_BY_FADE(Vert.Light);
							}
							unguardSlow;
						}
					}
					else if (LightGroup != UNLIT_LIGHT)
					{
						LightGroup = UNLIT_LIGHT;
						Vert.Normal = Tri.EdgeNormals[j].TransformVectorBy(CompNormalCoords).NormalApprox();
						Vert.Light = FPlane(1.f, 1.f, 1.f, 1.f);

						SCALE_LIGHT_BY_FADE(Vert.Light);
					}
				}
				else
				{
					const UBOOL bLightNeedsCalc = (LightGroup == INVALID_LIGHT) || (bSupportPreComp && (bFlatShaded || LightGroup != Group.SmoothGroup));
					if (bLightNeedsCalc)
					{
						// Compute vertex normal.
						if (bSupportPreComp)
						{
							if (bFlatShaded)
							{
								Vert.Normal = ThisSurfNormal;
								LightGroup = INDEX_NONE;
							}
							else
							{
								Vert.Normal = Tri.EdgeNormals[j].TransformVectorBy(CompNormalCoords).NormalApprox();
								LightGroup = Group.SmoothGroup;
							}
						}
						else
						{
							FVector Norm(0, 0, 0);
							FMeshVertConnect& Connect = Mesh->Connects(Tri.iVertex[j]);
							INT* vLinks = &Mesh->VertLinks(Connect.TriangleListOffset);
							for (INT k = 0; k < Connect.NumVertTriangles; k++)
								Norm += TriNormals[vLinks[k]];
							Vert.Normal = FPlane(Vert.Point, Norm.NormalApprox());
						}

						guardSlow(Light);
						if (bBeyondFog)
						{
							Vert.Light = FPlane(0.f, 0.f, 0.f, 0.f);
							Vert.Fog = FogSurface.FogColor;
						}
						else
						{
							//debugf(TEXT("LightTyp!=3"));
							// Compute effect of each lightsource on this vertex.
							if (bHasLightData)
							{
								GVisibleLightFlags = GBaseFlags ? GBaseFlags[bSingleLightMap ? Tri.iVertex[j] : TriPool[i].VertCode + j] : 0;
								GBaseColour = GBaseColors ? GBaseColors[bSingleLightMap ? Tri.iVertex[j] : TriPool[i].VertCode + j] : FVector(0, 0, 0);
							}
							PolyFlags |= GLightManager->Light(Vert, PolyFlags);

							SCALE_LIGHT_BY_FADE(Vert.Light);
						}
						unguardSlow;

						// Project it:
						if (bIsSoftware)
							Vert.Project(Frame);
					}
				}

				VertX.Point = Vert.Point;
				VertX.Light = Vert.Light;
				VertX.Fog = Vert.Fog;
				VertX.Normal = Vert.Normal;

				if (bIsSoftware)
				{
					VertX.RZ = Vert.RZ;
					VertX.ScreenX = Vert.ScreenX;
					VertX.ScreenY = Vert.ScreenY;
					VertX.IntY = Vert.IntY;
				}

				if( PolyFlags & PF_Environment )
				{
					FVector T = Vert.Point.NormalApprox().MirrorByVector( Vert.Normal ).TransformVectorBy( Frame->Uncoords );
					VertX.U = (T.X+1.0) * 0.5 * UInner;
					VertX.V = (T.Y+1.0) * 0.5 * VInner;
				}
				if (PolyFlags & PF_Unlit)
				{
					VertX.Light = GUnlitColor;
					SCALE_LIGHT_BY_FADE(VertX.Light);
				}
			}
			if (bMirrorMesh)
				Exchange(MeshVertList[NumPolies - 1], MeshVertList[NumPolies - 3]);//...^^

			if( NumPolies>=(MAX_FULLPOLYLIST-2) )
			{
				DRAW_FOGGED_SURFLIST(*Info, NumPolies, (PolyFlags | ExtraDrawFlags));
				NumPolies = 0;
			}
		}
		if( NumPolies )
		{
			DRAW_FOGGED_SURFLIST(*Info, NumPolies, (PolyFlags | ExtraDrawFlags));
		}

		guardSlow(DrawDetail);
		// Draw detail textures
		if (!bBeyondFog && bHasDetailTex && !bIsSoftware && RenDev->MeshDetailTextures && RenDev->LegacyRenderingOnly /*&& bFogMode == FOGMODE_None*/ && !(ExtraFlags & (PF_RenderFog | PF_Translucent | PF_AlphaBlend)) && !bFadeView)
		{
			UBOOL bHasDetail = FALSE;
			BYTE LastZMode = 255;
			OldIndex = INDEX_NONE;
			PolyFlags = (PF_Modulated | PF_Flat);
			NumPolies = 0;

			for (i = 0; i < VisibleTriangles; i++)
			{
				// Set up the triangle.
				const FStaticMeshTri& Tri = *TriPool[i].Tri;

				if (OldIndex != Tri.GroupIndex)
				{
					OldIndex = Tri.GroupIndex;
					const FStaticMeshTexGroup& Group = Mesh->SMGroups(OldIndex);

					// Draw previous group.
					if (NumPolies)
					{
						DRAW_FOGGED_SURFLIST(*Info, NumPolies, PF_Modulated);
						NumPolies = 0;
					}
					if ((Group.RealPolyFlags | ExtraFlags) & (PF_Environment | PF_Translucent | PF_Masked)) // Skip enviroment/translucent surfaces.
					{
						bHasDetail = FALSE;
						continue;
					}

					// Get texture.
					UTexture* T = GET_TEXTURE(Group.Texture, 0);
					bHasDetail = (T && T->DetailTexture);
					if (bHasDetail)
					{
						Info = T->DetailTexture->GetTexture(INDEX_NONE, RenDev);
						UInner = Info->UScale * Info->USize * 8.f;
						VInner = Info->VScale * Info->VSize * 8.f;
						if (LastZMode == 255)
						{
							LastZMode = RenDev->SetZTestMode(ZTEST_Equal);
							//Frame->PushClipPlane(FPlane(0.f, 0.f, -1.f, -200.f));
						}
					}
				}
				if (!bHasDetail)
					continue;

				for (j = 0; j < 3; j++)
				{
					FTransTexture& Vert = Samples[Tri.iVertex[j]];
					FTransTexture& VertX = MeshVertList[NumPolies++];
					VertX.U = Tri.Tex[j].U * UInner;
					VertX.V = Tri.Tex[j].V * VInner;
					VertX.Point = Vert.Point;
					VertX.Normal = Vert.Normal;
				}
				if (bMirrorMesh)
					Exchange(MeshVertList[NumPolies - 1], MeshVertList[NumPolies - 3]);//...^^

				if (NumPolies >= (MAX_FULLPOLYLIST - 2))
				{
					DRAW_FOGGED_SURFLIST(*Info, NumPolies, PF_Modulated);
					NumPolies = 0;
				}
			}
			if (NumPolies)
			{
				DRAW_FOGGED_SURFLIST(*Info, NumPolies, PF_Modulated);
			}
			if (LastZMode != 255)
			{
				//Frame->PopClipPlane();
				RenDev->SetZTestMode(LastZMode);
			}
		}
		unguardSlow;

		guardSlow(DrawUV2Map);
		if (!bIsSoftware && Mesh->bHasUV2Map && !bFadeView && !(ExtraFlags & (PF_Translucent | PF_AlphaBlend)))
		{
			UTexture* UVSkin = Owner->Sprite ? Owner->Sprite : Mesh->UV2MapTexture;
			if (UVSkin || bDrawUV2LightMap)
			{
				const FMeshFloatUV* UVMap = &Mesh->SMUV2Map(0);
				const BYTE LastZMode = RenDev->SetZTestMode(ZTEST_Equal);
				Info = NULL;
				FPlane Color;

				for (INT Pass = 0; Pass < 2; ++Pass)
				{
					DWORD SkipRendFlags = (PF_Invisible | PF_AlphaBlend);
					if (Pass == 0)
					{
						if (Mesh->bUV2PreLightmap)
						{
							if (!bDrawUV2LightMap)
								continue;
							Info = GLightManager->BuildStaticMeshLightmap(Mesh, D);
							ExtraDrawFlags = PF_Modulated;
							Color = FPlane(1.f, 1.f, 1.f, 1.f);
							SkipRendFlags |= PF_Translucent;
						}
						else if (!UVSkin)
							continue;
					}
					else
					{
						if (Mesh->bUV2PreLightmap)
						{
							if (!UVSkin)
								continue;
							Info = NULL;
						}
						else if (bDrawUV2LightMap)
						{
							Info = GLightManager->BuildStaticMeshLightmap(Mesh, D);
							ExtraDrawFlags = PF_Modulated;
							Color = FPlane(1.f, 1.f, 1.f, 1.f);
							SkipRendFlags |= PF_Translucent;
						}
						else break;
					}

					if (!Info)
					{
						Info = UVSkin->GetTexture(INDEX_NONE, RenDev);
						ExtraDrawFlags = UVSkin->PolyFlags;
						switch (Mesh->UV2Style)
						{
						case STY_Masked:
							ExtraDrawFlags |= PF_Masked;
							break;
						case STY_Translucent:
							ExtraDrawFlags |= PF_Translucent;
							break;
						case STY_Modulated:
							ExtraDrawFlags |= PF_Modulated;
							break;
						case STY_AlphaBlend:
							ExtraDrawFlags |= PF_AlphaBlend;
							break;
						}
						Color = Owner->ActorGUnlitColor.IsZero() ? FPlane(1.f, 1.f, 1.f, 1.f) : Owner->ActorGUnlitColor.Plane();
					}

					UInner = Info->UScale * Info->USize;
					VInner = Info->VScale * Info->VSize;
					OldIndex = INDEX_NONE;
					NumPolies = 0;
					UBOOL bHideSurf = FALSE;

					for (i = 0; i < VisibleTriangles; i++)
					{
						// Set up the triangle.
						const FStaticMeshTri& Tri = *TriPool[i].Tri;

						if (OldIndex != Tri.GroupIndex)
						{
							OldIndex = Tri.GroupIndex;
							bHideSurf = (Mesh->SMGroups(OldIndex).RealPolyFlags & SkipRendFlags) != 0;
							if (!bHideSurf)
							{
								if (Mesh->SMGroups(OldIndex).RealPolyFlags & PF_Translucent)
									RenDev->SetZTestMode(ZTEST_LessEqual);
								else RenDev->SetZTestMode(ZTEST_Equal);
							}
						}
						if (bHideSurf)
							continue;

						for (j = 0; j < 3; j++)
						{
							FTransTexture& Vert = Samples[Tri.iVertex[j]];
							FTransTexture& VertX = MeshVertList[NumPolies++];
							VertX.U = Tri.UV2[j]->U * UInner;
							VertX.V = Tri.UV2[j]->V * VInner;
							VertX.Point = Vert.Point;
							VertX.Normal = Vert.Normal;
							VertX.Light = Color;

							if (ExtraDrawFlags & PF_Environment)
							{
								FVector T = Vert.Point.NormalApprox().MirrorByVector(Vert.Normal).TransformVectorBy(Frame->Uncoords);
								VertX.U = (T.X + 1.0) * 0.5 * UInner;
								VertX.V = (T.Y + 1.0) * 0.5 * VInner;
							}
						}
						if (bMirrorMesh)
							Exchange(MeshVertList[NumPolies - 1], MeshVertList[NumPolies - 3]);//...^^

						if (NumPolies >= (MAX_FULLPOLYLIST - 2))
						{
							DRAW_FOGGED_SURFLIST(*Info, NumPolies, ExtraDrawFlags);
							NumPolies = 0;
						}
					}
					if (NumPolies)
					{
						DRAW_FOGGED_SURFLIST(*Info, NumPolies, ExtraDrawFlags);
					}
				}
				RenDev->SetZTestMode(LastZMode);
			}
		}
		unguardSlow;

		RenDev->EndMeshDraw(Frame, Owner);
		DRAW_FOGGED_FINISH;
		unguardSlow;

		guard(DrawProjectors);
		// Draw projectors.
		if (GShowProjectors && !bBeyondFog && !(ExtraFlags & PF_Translucent))
		{
			GET_OWNER_PROJECTORS;
			if (ProjList.Num())
			{
				PRE_DRAWPROJECTORS;
				UBOOL bCanUseStatic = (Owner->bStatic != 0);
				for (INT i = 0; i < ProjList.Num(); i++)
				{
					PRE_LOOP_DRAWPROJECTORS;
					if (bCanUseStatic && P->bStaticMap)
					{
						FProjectorSurfMesh* M = ((FStaticProjector*)P->StaticMapData)->StaticMap.FindRef(Owner);
						if (M && (iLodLevel + 1) < M->SurfLOD.Num())
						{
							FProjectorSurf* S = M->SurfLOD(iLodLevel + 1);
							if (S)
								S->Draw(Frame, i, Sprite->SpanBuffer);
						}
					}
					else
					{
						for (INT j = 0; j < VisibleTriangles; ++j)
						{
							const FStaticMeshTri& Tri = *TriPool[j].Tri;
							for (BYTE z = 0; z < 3; z++)
								Pts[z] = &Samples[Tri.iVertex[z]];
							DrawDecalMeshFace(P, true, &Pts[0], 3, Frame, i);
						}
					}
					POST_LOOP_DRAWPROJECTORS;
				}
				POST_DRAWPROJECTORS;
			}
		}
		unguard;

		GLightManager->FinishActor();
		unguardSlow;
	}

	unguardf(( TEXT("(%ls)"), Owner->Mesh->GetFullName() ));
}
