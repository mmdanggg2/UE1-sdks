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

	FAtlasFrame() {}
	FAtlasFrame( INT InU, INT InV, INT InUL, INT InVL, const FLightMapIndex& Index)
		: U(InU), V(InV), UL(InUL), VL(InVL)
		, DynamicTag(0), Volumetric(0)
		, UScale(Index.UScale), VScale(Index.VScale)
		, Pan( Index.Pan.X-U*UScale, Index.Pan.Y-V*VScale, Index.Pan.Z)
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

	UBOOL Lock( FTextureInfo& Info, INT iLightMap, BYTE AtlasID);

	void Empty();
	void RestoreStaticFrame( FAtlasFrame& Frame);
};




//
// Main lightmap atlas system
//
struct RENDER_API FAtlasManager
{
	// Global state
	UBOOL               Enabled;
	UBOOL               AtlasDebug;
	INT                 MaxTextureSize;
	UBOOL               NoDynamicLights;
	UBOOL               UNUSED_HW_MERGE;
	ETextureFormat      Format;

	// Local state
	ULevel*             Level;
	UModel*             Model;
	FAtlasMap           StaticZoneMap[FBspNode::MAX_ZONES];
	FSceneNode*         Frame;
	INT                 FrameCount;

	// State trackers
	UBOOL ST_Enabled;
	UBOOL ST_UsingAtlas;
	UBOOL ST_NoDynamicLights;
	UBOOL ST_VolumetricLighting;
	INT ST_LightMapCount;
	INT ST_LightBitsCount;
#ifdef PROTOTYPE_STATIC_GEOMETRY_BUFFERING
	INT ST_PolysRebuildTag;
	INT ST_LightRebuildTag;
#endif
#ifdef PROTOTYPE_RENDEV_469_COMPATIBILITY
	UBOOL ST_UpdateTextureRectSupport;
#endif

	UBOOL UpdateCacheTag;

	FAtlasManager();

	void PushFrame( FSceneNode* NewFrame);
	void PopFrame();
	void CheckForLevel( ULevel* InLevel);

	void BuildStaticLights();
	UBOOL IsStaticLight( AActor* Light);

	static FORCEINLINE UBOOL IsAtlasID( QWORD CacheID)
	{
		return ((CacheID & FAtlasMap::ID_And) | FAtlasMap::ID_Or) == CacheID;
	}

	// Temporary, for external render device access
	// Exports the atlas versions of a model's LightMap array
	// To be applied on the LightMap texture passed to DrawComplexSurface
	virtual UBOOL ExportLightMapIndex( UModel* Model, INT iNode, FLightMapIndex& Index, UBOOL Back);
};
extern RENDER_API FAtlasManager GAtlasManager;
