/*=============================================================================
	UnAtlas.h: Light, fog map atlas builder header for Unreal Tournament v469.

	Revision history:
		* Created by Fernando Velazquez.

	See UnAtlas.cpp for more detailed information.
=============================================================================*/



//
// Describe a region in an atlas.
//
struct FAtlasFrame
{
	typedef INT KEY;
	typedef TMap<KEY,FAtlasFrame> MAP;

	INT U, V, UL, VL;
	INT DynamicTag;
	UBOOL Volumetric;

	FLOAT UScale, VScale;
	FVector Pan;
	INT FirstZone;

	FAtlasFrame() {}
	FAtlasFrame( INT InU, INT InV, INT InUL, INT InVL, const FLightMapIndex& Index, INT InFirstZone)
		: U(InU), V(InV), UL(InUL), VL(InVL)
		, DynamicTag(0), Volumetric(0)
		, UScale(Index.UScale), VScale(Index.VScale)
		, Pan( Index.Pan.X-U*UScale, Index.Pan.Y-V*VScale, Index.Pan.Z)
		, FirstZone( InFirstZone)
	{}
};

//
// Basic atlas
//
struct FAtlasMap
{
	static constexpr QWORD   ID_And = ~0xE000000000000000ull;
	static constexpr QWORD   ID_Or  =  0xA000000000000000ull;

	QWORD                    ID;
	FMipmap                  StaticMip;
	FMipmap                  DynamicMip;
	FMipmap                  VolumetricMip;
	FAtlasFrame::MAP         Frames;
	INT                      UpdateCacheTag;

	UBOOL Lock( FTextureInfo& Info, INT iLightMap, BYTE AtlasID, BYTE ZoneNumber);

	void Empty();
	void RestoreStaticFrame( FAtlasFrame& Frame);
};

//
// Static BSP render parameters.
// To be used for streaming static geometry to the GPU
// Atlas Manager version that additionally fills AtlasU, AtlasV on light compute.
//
struct RENDER_API FStaticBspInfoAtlas : public FStaticBspInfoBase
{
	FStaticBspInfoAtlas() {};
	FStaticBspInfoAtlas( ULevel* InLevel)
		: FStaticBspInfoBase(InLevel)
	{}

	// Precalculates Light, Atlas UVs
	void ComputeLightUV();
};
extern FStaticBspInfoAtlas GStaticBspParams;

//
// Main lightmap atlas system
//
struct RENDER_API FAtlasManager
{
	// Global state
	UBOOL               Enabled;
	UBOOL               ForceEnable;
	UBOOL               AtlasDebug;
	INT                 MaxTextureSize;
	UBOOL               NoDynamicLights;
	UBOOL               ForceRebuildLights;
	ETextureFormat      Format;
	UBOOL               UpdateTextureRect;

	// Local state
	FAtlasMap           LevelMap;
	FSceneNode*         Frame;
	INT                 FrameCount;

	// State trackers
	UBOOL ST_NoDynamicLights;
	UBOOL ST_VolumetricLighting;
	INT ST_LightMapCount;
	INT ST_LightBitsCount;
#ifdef PROTOTYPE_STATIC_GEOMETRY_BUFFERING
	INT ST_PolysRebuildTag;
	INT ST_LightRebuildTag;
#endif

	UBOOL UpdateCacheTag;

	FAtlasManager();

	FAtlasMap* GetAtlas( UModel* Model);
	void Flush();

	void PushFrame( FSceneNode* NewFrame);
	void PopFrame();
	void CheckForLevel( ULevel* Level);

	void BuildStaticLights( ULevel* Level);
	UBOOL IsStaticLight( AActor* Light);

	static FORCEINLINE UBOOL IsAtlasID( QWORD CacheID)
	{
		return ((CacheID & FAtlasMap::ID_And) | FAtlasMap::ID_Or) == CacheID;
	}
};
extern RENDER_API FAtlasManager GAtlasManager;
