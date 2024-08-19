/*=============================================================================
	UnShadTex.h: Unreal shadow texture class
=============================================================================*/

typedef struct
{
	FCoords Coords, Uncoords;
	FLOAT FX, FY, FX2, FY2, FX15, FY15;
	FVector	Proj, RProj;
	FLOAT PrjXM, PrjXP, PrjYM, PrjYP;
	FVector ViewSides[4];
} ShadowDef;


class ENGINE_API UShadowTexture : public UTexture
{
	DECLARE_CLASS(UShadowTexture,UTexture,0,Engine)

	// Constructors. Executed on both instantation and reloading of an object.
	UShadowTexture();

	// UObject interface.	
	void PostLoad();
	void PostEditChange() {PostLoad();}
	void Destroy();

	// UTexture interface.
	void Init( INT InUSize, INT InVSize );
	void Tick(FLOAT DeltaSeconds);

	// UShadowTexture Interface
	void DrawPolygon(FVector *Pts, INT NumPts);
	void ClearShadow(void);

private:
};


/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
