/*=============================================================================
	OpenGL_HitTesting.cpp

	Hit testing path of rendering functions.

	Revision history:
	* Moved from OpenGL.cpp
	* Separated Hit Testing into a render path proc (Fernando Velazquez)
=============================================================================*/

#include "OpenGLDrv.h"
#include "UTGLROpenGL.h"

void UOpenGLRenderDevice::DrawComplexSurface_HitTesting( FSceneNode* Frame, FSurfaceInfo_DrawComplex& Surface, FSurfaceFacet & Facet)
{
	guard(HitTesting);
	check(m_HitData);

	//Hit select path
	for ( FSavedPoly* Poly=Facet.Polys; Poly; Poly=Poly->Next)
	{
		INT NumPts = Poly->NumPts;
		CGClip::vec3_t triPts[3];
		INT i;
		const FTransform* Pt;

		Pt = Poly->Pts[0];
		triPts[0].x = Pt->Point.X;
		triPts[0].y = Pt->Point.Y;
		triPts[0].z = Pt->Point.Z;

		for ( i=2; i<NumPts; i++)
		{
			Pt = Poly->Pts[i-1];
			triPts[1].x = Pt->Point.X;
			triPts[1].y = Pt->Point.Y;
			triPts[1].z = Pt->Point.Z;

			Pt = Poly->Pts[i];
			triPts[2].x = Pt->Point.X;
			triPts[2].y = Pt->Point.Y;
			triPts[2].z = Pt->Point.Z;

			m_gclip.SelectDrawTri(Frame, triPts);
		}
	}

	unguard;
}

void UOpenGLRenderDevice::DrawGouraudPolygon_HitTesting( FSceneNode* Frame, FSurfaceInfo_DrawGouraud& Surface, FTransTexture** Pts, INT NumPts)
{
	guard(HitTesting);
	check(m_HitData);

	CGClip::vec3_t triPts[3];
	const FTransTexture* Pt;
	INT i;

	Pt = Pts[0];
	triPts[0].x = Pt->Point.X;
	triPts[0].y = Pt->Point.Y;
	triPts[0].z = Pt->Point.Z;

	for ( i=2; i<NumPts; i++)
	{
		Pt = Pts[i-1];
		triPts[1].x = Pt->Point.X;
		triPts[1].y = Pt->Point.Y;
		triPts[1].z = Pt->Point.Z;

		Pt = Pts[i];
		triPts[2].x = Pt->Point.X;
		triPts[2].y = Pt->Point.Y;
		triPts[2].z = Pt->Point.Z;

		m_gclip.SelectDrawTri(Frame, triPts);
	}

	unguard;
}

void UOpenGLRenderDevice::DrawTile_HitTesting( FSceneNode* Frame, FSurfaceInfo_DrawTile& Surface, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, FLOAT Z)
{
	guard(HitTesting);
	check(m_HitData);

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

	m_gclip.SelectDrawTri(Frame, triPts);

	triPts[0].x = RPX1;
	triPts[0].y = RPY1;
	triPts[0].z = Z;

	triPts[1].x = RPX2;
	triPts[1].y = RPY2;
	triPts[1].z = Z;

	triPts[2].x = RPX1;
	triPts[2].y = RPY2;
	triPts[2].z = Z;

	m_gclip.SelectDrawTri(Frame, triPts);

	// Deal with transparent pixels
	if ( m_gclip.CheckNewSelectHit() && (Surface.PolyFlags & PF_Masked) && Surface.Texture && Surface.Texture->Mips[0] )
	{
		// Calc UV of the hit
		float HitX = Viewport->HitX + Viewport->HitXL / 2;
		float HitY = Viewport->HitY + Viewport->HitYL / 2;
		float X2 = X + XL;
		float Y2 = Y + YL;
		float HitAlphaX = 1.0f - (X2 - HitX) / XL;
		float HitAlphaY = 1.0f - (Y2 - HitY) / YL;
		INT USize = Surface.Texture->Mips[0]->USize;
		INT VSize = Surface.Texture->Mips[0]->VSize;
		INT HitU = appRound(U + UL * HitAlphaX) % USize;
		INT HitV = appRound(V + VL * HitAlphaY) % VSize;

		if ( Surface.Texture->Format == TEXF_P8 )
		{
			// Reject if hits a pixel with palette index 0
			if ( Surface.Texture->Mips[0]->DataPtr[HitU + HitV * USize] == 0 )
				m_gclip.ClearNewSelectHit();
		}
	}

	unguard;
}

void UOpenGLRenderDevice::Draw3DLine_HitTesting( FSceneNode* Frame, FPlane& Color, DWORD LineFlags, FVector P1, FVector P2)
{
	guard(HitTesting);
	check(m_HitData);

	P1 = P1.TransformPointBy(Frame->Coords);
	P2 = P2.TransformPointBy(Frame->Coords);
	if (Frame->Viewport->IsOrtho())
	{
		// Zoom.
		FLOAT rcpZoom = 1.0f / Frame->Zoom;
		P1.X = (P1.X * rcpZoom) + Frame->FX2;
		P1.Y = (P1.Y * rcpZoom) + Frame->FY2;
		P2.X = (P2.X * rcpZoom) + Frame->FX2;
		P2.Y = (P2.Y * rcpZoom) + Frame->FY2;
		P1.Z = P2.Z = 1;

		// See if points form a line parallel to our line of sight (i.e. line appears as a dot).
		if (Abs(P2.X - P1.X) + Abs(P2.Y - P1.Y) >= 0.2f)
			Draw2DLine_HitTesting(Frame, Color, LineFlags, P1, P2);
		else
			Draw2DPoint_HitTesting(Frame, Color, LINE_None, P1.X - 1.0f, P1.Y - 1.0f, P1.X + 1.0f, P1.Y + 1.0f, P1.Z);
	}
	else
	{
		// If line is being drawn on top of world, it has selection priority.
		if ( !(LineFlags & LINE_DepthCued) )
		{
			P1 /= P1.Z;
			P2 /= P2.Z;
		}

		CGClip::vec3_t lnPts[2];

		lnPts[0].x = P1.X;
		lnPts[0].y = P1.Y;
		lnPts[0].z = P1.Z;

		lnPts[1].x = P2.X;
		lnPts[1].y = P2.Y;
		lnPts[1].z = P2.Z;

		m_gclip.SelectDrawLine(Frame, lnPts);

		return;
	}

	// TODO: !(LineFlags & DepthCued) lines must always pass the depth test
	unguard;
}

void UOpenGLRenderDevice::Draw2DLine_HitTesting( FSceneNode* Frame, FPlane& Color, DWORD LineFlags, FVector P1, FVector P2)
{
	guard(HitTesting);
	check(m_HitData);

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

	CGClip::vec3_t lnPts[2];

	lnPts[0].x = X1Pos;
	lnPts[0].y = Y1Pos;
	lnPts[0].z = P1.Z;

	lnPts[1].x = X2Pos;
	lnPts[1].y = Y2Pos;
	lnPts[1].z = P2.Z;

	// TODO: !(LineFlags & DepthCued) lines must always pass the depth test
	m_gclip.SelectDrawLine(Frame, lnPts);

	unguard;
}

void UOpenGLRenderDevice::Draw2DPoint_HitTesting( FSceneNode* Frame, FPlane& Color, DWORD LineFlags, FLOAT X1, FLOAT Y1, FLOAT X2, FLOAT Y2, FLOAT Z)
{
	guard(HitTesting);
	check(m_HitData);

	// Hack to fix UED selection problem with selection brush
	// TODO: Instead always pass depth test
	if ( GIsEditor )
		Z = 1.0f;

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

	m_gclip.SelectDrawTri(Frame, triPts);

	triPts[0].x = X1Pos;
	triPts[0].y = Y1Pos;
	triPts[0].z = Z;

	triPts[1].x = X2Pos;
	triPts[1].y = Y2Pos;
	triPts[1].z = Z;

	triPts[2].x = X1Pos;
	triPts[2].y = Y2Pos;
	triPts[2].z = Z;

	m_gclip.SelectDrawTri(Frame, triPts);

	unguard;
}


void UOpenGLRenderDevice::UOpenGLRenderDevice::SetSceneNode_HitTesting( FSceneNode* Frame)
{
	guard(HitTesting);
	check(m_HitData);

	if ( Frame->Viewport->IsOrtho() )
	{
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
	else
	{
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

		for (i = 0; i < 4; i++)
		{
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

	unguard;
}