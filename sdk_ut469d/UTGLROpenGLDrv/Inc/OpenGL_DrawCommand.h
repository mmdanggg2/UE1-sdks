/*=============================================================================
	OpenGL_DrawCommand.h
	
	Keeps track of draw calls

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

// TEMP
struct FPendingTexture
{
	INT    PoolID;
	DWORD  RelevantPolyFlags;
	DWORD  ViewIndex;
};

namespace FGL
{
namespace Draw
{

extern FMemStack    CmdMem;
extern FMemMark     CmdMark;

extern void InitCmdMem();
extern void ExitCmdMem();
extern FTextureInfo* LockDetailTexture( const FTextureInfo& BaseInfo);



struct TextureList
{
	static constexpr INT MaxUnits = TMU_FogMap+1;

	FGL::FTextureRemap* TexRemaps[MaxUnits];
	FTextureInfo*       Infos    [MaxUnits];
	DWORD               PolyFlags[MaxUnits];
	INT                 Units;
	INT                 UniformIndex[4];

	void Init( const FSurfaceInfo_DrawTile& Surf, UBOOL DetailTexture=0);
	void Init( const FSurfaceInfo_DrawGouraud& Surf, UBOOL DetailTexture=0);
	void Init( const FSurfaceInfo_DrawGouraudTris& Surf, UBOOL DetailTexture=0);
	void Init( const FSurfaceInfo_DrawComplex& Surf, UBOOL DetailTexture);
	void Init( const FSurfaceInfo_2DPoint& Surf, UBOOL DetailTexture=0);
	bool Matches( const FPendingTexture* Pending);
		
	bool QueueUniforms();

	// Get texture layer
	BYTE GetLayer( INT Unit)
	{
		return TexRemaps[Unit] ? (BYTE)TexRemaps[Unit]->Layer : 0;
	}

private:
	void Resolve();
};



class Command
{
public:
	typedef void (UOpenGLRenderDevice::*BufferDraw_t)( Command* Draw);

	// Header
	BufferDraw_t Draw;
	DWORD PolyFlags;
	QWORD CacheID_Base;
	QWORD CacheID_Light;
	QWORD CacheID_Fog;

	// Data
	FPendingTexture PendingTextures[TMU_FogMap + 1];
	DWORD PrimitiveStart;
	DWORD PrimitiveCount;
	DWORD Attribs;
	DWORD Stride;
	DWORD ClipPlaneID;

	Command* Next;

	void SetTextures( const TextureList& TexList);

	bool CanMerge( BufferDraw_t Draw, DWORD PolyFlags)
	{
		if	(	this->Draw          == Draw
			&&	this->PolyFlags     == PolyFlags )
		{
			return true;
		}
		return false;
	}

	bool CanMerge( BufferDraw_t Draw, DWORD PolyFlags, QWORD CacheID_Base, QWORD CacheID_Light=0, QWORD CacheID_Fog=0)
	{
		if	(	CanMerge(Draw, PolyFlags)
			&&	this->CacheID_Base  == CacheID_Base
			&&	this->CacheID_Light == CacheID_Light
			&&	this->CacheID_Fog   == CacheID_Fog )
		{
			return true;
		}
		return false;
	}

};

//
// Manages a simple linked list
// TODO: Could be turned into a template
//
class CommandChain
{
public:
	Command* First;
	Command* Last;

	// Cached everytime a texture lookup is done
	BYTE Layers[TMU_BASE_MAX];

	void Attach( Command* New)
	{
		if ( !First )
			First = New;
		else
			Last->Next = New;
		Last = New;
	}

	void Reset()
	{
		First = nullptr;
		Last = nullptr;
	}

	// General Textured draw
	template <typename SURF_T> bool MergeWithLast( const SURF_T& Surface, Command::BufferDraw_t Draw, UBOOL DetailTextures, TextureList& OutTexList)
	{
		using namespace FGL::Surface;

		bool TryArrayMerge = false;

		// Is this draw identical to previous?
		if	(	Last != nullptr
			&&	Last->Draw == Draw
			&&	Last->PolyFlags == Surface.PolyFlags )
		{
			if	(	(!TSurfaceInfoTraits<SURF_T>::UsesTexture || CacheID(Surface.Texture) == Last->CacheID_Base)
				&&	(!TSurfaceInfoTraits<SURF_T>::UsesLightMap || CacheID(Surface.LightMap) == Last->CacheID_Light)
				&&	(!TSurfaceInfoTraits<SURF_T>::UsesFogMap || CacheID(Surface.FogMap) == Last->CacheID_Fog)
				&&	(!TSurfaceInfoTraits<SURF_T>::UsesClipPlane || Surface.ClipPlaneID == Last->ClipPlaneID) )
			{
				OutTexList.Units = 0;
				return true;
			}

			TryArrayMerge = true;
		}

		// Initialize textures for merge checking and possibly new draw
		OutTexList.Init(Surface, DetailTextures);

		// Update layer list
		for ( DWORD i=0; i<OutTexList.Units; i++)
			Layers[i] = OutTexList.GetLayer(i);

		if ( TryArrayMerge && OutTexList.Matches(Last->PendingTextures) )
		{
			// Update CacheID's
			if ( TSurfaceInfoTraits<SURF_T>::UsesTexture )  Last->CacheID_Base  = CacheID(Surface.Texture);
			if ( TSurfaceInfoTraits<SURF_T>::UsesLightMap ) Last->CacheID_Light = CacheID(Surface.LightMap);
			if ( TSurfaceInfoTraits<SURF_T>::UsesFogMap )   Last->CacheID_Fog   = CacheID(Surface.FogMap);
			return true;
		}

		// Create command
		Command* NewDraw = new(CmdMem) Command;
		NewDraw->Next           = nullptr;
		NewDraw->Draw           = Draw;
		NewDraw->PolyFlags      = Surface.PolyFlags;
		if ( TSurfaceInfoTraits<SURF_T>::UsesTexture )
			NewDraw->CacheID_Base   = FGL::Surface::CacheID(Surface.Texture);
		if ( TSurfaceInfoTraits<SURF_T>::UsesLightMap )
			NewDraw->CacheID_Light  = FGL::Surface::CacheID(Surface.LightMap);
		if ( TSurfaceInfoTraits<SURF_T>::UsesFogMap )
			NewDraw->CacheID_Fog    = FGL::Surface::CacheID(Surface.FogMap);
		NewDraw->SetTextures(OutTexList);
		NewDraw->ClipPlaneID    = Surface.ClipPlaneID;

		Attach(NewDraw);

		return false;
	}
	// General Line draw
	bool MergeWithLast(Command::BufferDraw_t Draw, DWORD LineFlags)
	{
		// Is this draw identical to previous?
		if	(	Last != nullptr
			&&	Last->Draw == Draw
			&&	Last->PolyFlags == LineFlags )
		{
			return true;
		}

		// Create command
		Command* NewDraw = new(CmdMem) Command; // TODO: Use LineCommand type?
		NewDraw->Next           = nullptr;
		NewDraw->Draw           = Draw;
		NewDraw->PolyFlags      = LineFlags;

		Attach(NewDraw);

		return false;
	}


};

// Specialized 2D Point solid color tile
template <> inline bool CommandChain::MergeWithLast<FSurfaceInfo_2DPoint>( const FSurfaceInfo_2DPoint& Surface, Command::BufferDraw_t Draw, UBOOL DetailTextures, TextureList& OutTexList)
{
	using namespace FGL::Surface;

	// Is this draw identical to previous?
	if	(	Last != nullptr
			&&	Last->Draw == Draw
			&&	Last->PolyFlags == Surface.PolyFlags )
	{
		if ( TEX_CACHE_ID_WHITE == Last->CacheID_Base )
		{
			OutTexList.Units = 0;
			return true;
		}
	}

	// Update layer list
	Layers[0] = 0;

	// Create command
	Command* NewDraw = new(CmdMem) Command;
	NewDraw->Next           = nullptr;
	NewDraw->Draw           = Draw;
	NewDraw->PolyFlags      = Surface.PolyFlags;
	NewDraw->CacheID_Base   = TEX_CACHE_ID_WHITE;
	NewDraw->PendingTextures[0].PoolID = TEXPOOL_ID_NoTexture;
	NewDraw->PendingTextures[0].RelevantPolyFlags = Surface.PolyFlags;
	NewDraw->PendingTextures[0].ViewIndex = 0;
	NewDraw->ClipPlaneID    = Surface.ClipPlaneID;

	Attach(NewDraw);

	return false;
}


};
};


/*-----------------------------------------------------------------------------
	TextureList inlines.
-----------------------------------------------------------------------------*/

inline void FGL::Draw::TextureList::Init( const FSurfaceInfo_DrawTile& Surface, UBOOL DetailTexture)
{
	PolyFlags[TMU_Base]          = Surface.PolyFlags;
	Infos[TMU_Base]              = Surface.Texture;

	Units = TMU_Base + 1;

	Resolve();
}

inline void FGL::Draw::TextureList::Init( const FSurfaceInfo_DrawGouraud& Surface, UBOOL DetailTexture)
{
	DWORD NoSmoothFlag = Surface.PolyFlags & PF_NoSmooth;

	PolyFlags[TMU_Base]          = Surface.PolyFlags;
	PolyFlags[TMU_DetailTexture] = PF_Modulated | NoSmoothFlag;

	Infos[TMU_Base]              = Surface.Texture;
	Infos[TMU_DetailTexture]     = DetailTexture ? Surface.DetailTexture : nullptr;

	Units = TMU_DetailTexture + 1;

	Resolve();
}

inline void FGL::Draw::TextureList::Init( const FSurfaceInfo_DrawGouraudTris& Surf, UBOOL DetailTexture)
{
	Init((const FSurfaceInfo_DrawGouraud&)Surf, DetailTexture);
}

inline void FGL::Draw::TextureList::Init( const FSurfaceInfo_DrawComplex& Surface, UBOOL DetailTexture)
{
	DWORD NoSmoothFlag = Surface.PolyFlags & PF_NoSmooth;
	PolyFlags[TMU_Base]          = Surface.PolyFlags;
	PolyFlags[TMU_DetailTexture] = PF_Modulated | NoSmoothFlag;
	PolyFlags[TMU_MacroTexture]  = PF_Modulated | NoSmoothFlag;
	PolyFlags[TMU_LightMap]      = PF_Modulated;
	PolyFlags[TMU_FogMap]        = PF_Modulated;

	Infos[TMU_Base]              = Surface.Texture;
	Infos[TMU_DetailTexture]     = DetailTexture ? Surface.DetailTexture : nullptr;
	Infos[TMU_MacroTexture]      = Surface.MacroTexture;
	Infos[TMU_LightMap]          = Surface.LightMap;
	Infos[TMU_FogMap]            = Surface.FogMap;

	Units = TMU_FogMap + 1;

	Resolve();
}

inline void FGL::Draw::TextureList::Init( const FSurfaceInfo_2DPoint& Surface, UBOOL DetailTexture)
{
// Unused
}

inline bool FGL::Draw::TextureList::Matches( const FPendingTexture* Pending)
{
	for ( INT i=0; i<Units; i++)
	{
		INT PoolIndex = TexRemaps[i] ? TexRemaps[i]->PoolIndex : INDEX_NONE;
		if ( PoolIndex != Pending[i].PoolID )
			return false;
	}
	return true;
}

/*-----------------------------------------------------------------------------
	Command inlines.
-----------------------------------------------------------------------------*/

inline void FGL::Draw::Command::SetTextures( const FGL::Draw::TextureList& TexList)
{
	for ( INT i=0; i<TexList.Units; i++)
	{
		if ( TexList.TexRemaps[i] )
		{
			PendingTextures[i].PoolID = TexList.TexRemaps[i]->PoolIndex;
			PendingTextures[i].RelevantPolyFlags = TexList.PolyFlags[i];
			PendingTextures[i].ViewIndex = 0;
		}
		else
			PendingTextures[i].PoolID = INDEX_NONE;
	}
}
