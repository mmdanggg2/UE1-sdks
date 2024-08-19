/*=============================================================================
	UnShadTex.cpp:  RUNE Shadow texture class

============================================================================*/

#include "EnginePrivate.h"
#include "UnRender.h"

#define MAX_SHADOW_SIZE 128

INT HackRaster[MAX_SHADOW_SIZE][2];

//==========================================================================
//
// UShadowTexture::UShadowTexture
//
//==========================================================================

UShadowTexture::UShadowTexture()
:	UTexture()
{	
	guard(UShadowTexture::UShadowTexture);

	// Set appropriate texture flags
	bParametric = 1;
	bRealtime   = 1;

	unguard;
}

//==========================================================================
//
// UShadowTexture::Destroy
//
//==========================================================================

void UShadowTexture::Destroy()
{
	guard(UScriptedTexture::Destroy);

	Super::Destroy();
	
	unguard;
}

//==========================================================================
//
// UShadowTexture::Init
//
//==========================================================================

void UShadowTexture::Init(INT InUSize, INT InVSize)
{
	guard(UShadowTexture::Init);

	// Init base class.
	UTexture::Init( InUSize, InVSize );

    unguard;
}

//==========================================================================
//
// UShadowTexture::PostLoad
//
//==========================================================================

void UShadowTexture::PostLoad()
{
	guard(UShadowTexture::PostLoad);

	UTexture::PostLoad();

	unguard;
}

//==========================================================================
//
// UShadowTexture::Tick
//
//==========================================================================

void UShadowTexture::Tick(FLOAT DeltaSeconds) 
{
	guard(UShadowTexture::Tick);

	Super::Tick(DeltaSeconds); 

	unguard;
}

//==========================================================================
//
// RasterPolygon
//
//==========================================================================

INT RasterStartY, RasterEndY;
void RasterPolygon(FVector *Pts, INT NumPts)
{
	INT newX;
	INT YInt[16]; // No more than 16 points allowed

	if(NumPts > 16)
	{
		return;
	}

	for(INT i = 0; i < NumPts; i++)
	{
		YInt[i] = appFloor(Pts[i].Y);
	}

	// Compute integer coords.
	RasterStartY = RasterEndY = YInt[0];

	for(i = 1; i < NumPts; i++)
	{
		INT Y = YInt[i];

		if(Y < RasterStartY)
			RasterStartY = Y;
		else if(Y > RasterEndY)
			RasterEndY = Y;
	}

	if(RasterStartY < 0 || RasterEndY > MAX_SHADOW_SIZE)
	{
		RasterStartY = Clamp(RasterStartY, (INT)0, (INT)(MAX_SHADOW_SIZE - 1));
		RasterEndY   = Clamp(RasterEndY, (INT)0, (INT)(MAX_SHADOW_SIZE - 1));

		for(INT i = 0; i < NumPts; i++)
		{
			YInt[i] = Clamp(YInt[i], (INT)0, (INT)(MAX_SHADOW_SIZE - 1));
			Pts[i].Y = Clamp(Pts[i].Y, (FLOAT)0, (FLOAT)(MAX_SHADOW_SIZE - 1));
		}
	}

	// Rasterize the edges.
	// OPTIMIZE:  Use a simpler edge-draw method... this doens't need sub-pixel accuracy
	INT Top, Bot;
	INT P0, P1;
	for( P0 = 0, P1 = 1, i = 0; i < NumPts; i++)
	{
		if(YInt[P0] != YInt[P1])
		{
			INT Index;
			if(YInt[P1] > YInt[P0])
			{
				Bot = P1;
				Top = P0;
				Index = 1;
			}
			else
			{
				Bot = P0;
				Top = P1;
				Index = 0;
			}

			INT *Set = HackRaster[0] + YInt[Top] * 2 + Index;

			DOUBLE YAdj  = YInt[Top] - Pts[Top].Y;
			DOUBLE FDX   = 65536.0 * (Pts[Bot].X - Pts[Top].X) / (Pts[Bot].Y - Pts[Top].Y);
			DWORD  X     = appFloor(65536.0 * Pts[Top].X + YAdj * FDX);
			DWORD  DX    = appFloor(FDX);
			INT    Count = YInt[Bot] - YInt[Top];
			while( Count > 0 )
			{
				newX = Unfix(X += DX);
				if(newX < 0)
					newX = 0;
				if(newX >= 128)
					newX = 127;

				*Set = newX;

//				check(*Set >= 0); // DEBUG!
//				check(*Set < 128); // DEBUG!

				Set += 2;
				Count -= 1;
			}
		}

		P0++;
		P1 = (P1 + 1) % NumPts;
	}
}

//==========================================================================
//
// UShadowTexture::DrawPolygon
//
//==========================================================================

void UShadowTexture::DrawPolygon(FVector *Pts, INT NumPts)
{
	guardSlow(UShadowTexture::DrawPolygon);

	INT i;
	INT count;
	BYTE *Dest;
	BYTE color;

	RasterPolygon(Pts, NumPts);

//	check(RasterStartY >= 0);
//	check(RasterEndY >= 0);
//	check(RasterStartY <= 128);
//	check(RasterEndY <= 128);
//	check(RasterStartY <= RasterEndY);

	Dest = &Mips(0).DataArray(0) + RasterStartY * USize;
	for(i = RasterStartY; i < RasterEndY; i++, Dest += USize)
	{
		count = *(HackRaster[i] + 1) - *HackRaster[i];

//		check(count <= 128);
//		check(*HackRaster[i] >= 0);
//		check(*HackRaster[i] <= 128);
//		check(*HackRaster[i] + count <= 128);

		color = 128 - (i * 0.75);
		if(count > 0)
		{
			appMemset(Dest + *HackRaster[i], color, count);
		}
	}

	unguardSlow;
}

//==========================================================================
//
// UShadowTexture::ClearShadow
//
//==========================================================================

void UShadowTexture::ClearShadow(void)
{
	appMemset(&Mips(0).DataArray(0), 128, USize * VSize);
	bRealtimeChanged = true;
}


IMPLEMENT_CLASS(UShadowTexture);

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

