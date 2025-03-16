/*=============================================================================
	OpenGL_SurfaceModel.h: Unreal surface descriptors.

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

struct FSurfaceInfo_Base
{
	static constexpr FTextureInfo* Texture       = nullptr;
	static constexpr FTextureInfo* DetailTexture = nullptr;
	static constexpr FTextureInfo* MacroTexture  = nullptr;
	static constexpr FTextureInfo* LightMap      = nullptr;
	static constexpr FTextureInfo* FogMap        = nullptr;
	static constexpr DWORD         ClipPlaneID   = 0;
};

struct FSurfaceInfo_DrawTile : public FSurfaceInfo_Base
{
	DWORD         PolyFlags;
	FColor*       Color;
	FColor*       Fog;
	FTextureInfo* Texture;

};

struct FSurfaceInfo_DrawGouraud : public FSurfaceInfo_Base
{
	DWORD         PolyFlags;
	FTextureInfo* Texture;
	FTextureInfo* DetailTexture;
};

struct FSurfaceInfo_DrawGouraudTris : public FSurfaceInfo_Base
{
	DWORD         PolyFlags;
	FTextureInfo* Texture;
	FTextureInfo* DetailTexture;
	DWORD         ClipPlaneID;
};

struct FSurfaceInfo_DrawComplex : public FSurfaceInfo
{
	static constexpr DWORD ClipPlaneID   = 0;
};

struct FSurfaceInfo_2DPoint : public FSurfaceInfo_Base
{
	DWORD         PolyFlags;
};




//
// Traits
//
template <typename T> struct TSurfaceInfoTraitsBase
{
	static constexpr bool UsesTexture = false;
	static constexpr bool UsesDetailTexture = false;
	static constexpr bool UsesMacroTexture = false;
	static constexpr bool UsesLightMap = false;
	static constexpr bool UsesFogMap = false;
	static constexpr bool UsesClipPlane = false;
};

template <typename T> struct TSurfaceInfoTraits : public TSurfaceInfoTraitsBase<T>
{
};

template <> struct TSurfaceInfoTraits<FSurfaceInfo_DrawTile> : public TSurfaceInfoTraitsBase<FSurfaceInfo_DrawTile>
{
	static constexpr bool UsesTexture = true;
};

template <> struct TSurfaceInfoTraits<FSurfaceInfo_DrawGouraud> : public TSurfaceInfoTraitsBase<FSurfaceInfo_DrawGouraud>
{
	static constexpr bool UsesTexture = true;
	static constexpr bool UsesDetailTexture = true;
	static constexpr bool UsesClipPlane = true;
};

template <> struct TSurfaceInfoTraits<FSurfaceInfo_DrawGouraudTris> : public TSurfaceInfoTraitsBase<FSurfaceInfo_DrawGouraudTris>
{
	static constexpr bool UsesTexture = true;
	static constexpr bool UsesDetailTexture = true;
	static constexpr bool UsesClipPlane = true;
};

template <> struct TSurfaceInfoTraits<FSurfaceInfo_DrawComplex> : public TSurfaceInfoTraitsBase<FSurfaceInfo_DrawComplex>
{
	static constexpr bool UsesTexture = true;
	static constexpr bool UsesDetailTexture = true;
	static constexpr bool UsesMacroTexture = true;
	static constexpr bool UsesLightMap = true;
	static constexpr bool UsesFogMap = true;
};
