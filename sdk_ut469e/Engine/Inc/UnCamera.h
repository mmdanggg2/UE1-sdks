/*=============================================================================
	UnViewport.h: Unreal viewport object.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack(push,OBJECT_ALIGNMENT)
#endif

/*-----------------------------------------------------------------------------
	UCanvas.
-----------------------------------------------------------------------------*/

// Constants.
enum {MAX_COLOR_BYTES=9}; // Maximum expected frame buffer depth = 8 bytes = 64-bit.

//
// Flags for drawing text, not yet used.
//
enum ETextOut
{
	TEXTOUT_Center			= 0x01,	// Center horizontally.
	TEXTOUT_CenterVertical	= 0x02,	// Center vertically.
	TEXTOUT_Right			= 0x04, // Right justify.
	TEXTOUT_Bottom			= 0x08, // Bottom justify.
	TEXTOUT_Wrap			= 0x10,	// Word wrap.
};

//
// Blitting types.
//
enum EViewportBlitFlags
{
	// Bitflags.
	BLIT_Fullscreen     = 0x0001, // Fullscreen.
	BLIT_Temporary      = 0x0002, // Temporary viewport.
	BLIT_DibSection     = 0x0004, // Create a DibSection for windowed rendering.
	BLIT_DirectDraw     = 0x0008, // Create Direct3D along with DirectDraw.
	BLIT_Direct3D       = 0x0010, // Create Direct3D along with DirectDraw.
	BLIT_NoWindowChange = 0x0020, // Don't resize existing window frame.
	BLIT_NoWindowFrame  = 0x0040, // Turn off the window frame.
	BLIT_OpenGL			= 0x0080, // OpenGL rendering.
	BLIT_HardwarePaint  = 0x0100, // Window should be repainted in hardware when drawn.

	BLIT_Metal          = 0x1000, // Apple Metal Rendering.
	BLIT_Vulkan         = 0x2000, // Vulkan Rendering.
	// Special.
	BLIT_ParameterFlags	= BLIT_NoWindowChange, // Only parameters to ResizeViewport, not permanent flags.
};

//
// A drawing canvas.
//
class ENGINE_API UCanvas : public UObject
{
	DECLARE_CLASS(UCanvas,UObject,CLASS_Transient,Engine)
	NO_DEFAULT_CONSTRUCTOR(UCanvas)

	// Variables.
	UFont*			Font;	
	FLOAT			SpaceX, SpaceY;
	FLOAT			OrgX, OrgY;
	FLOAT			ClipX, ClipY;
	FLOAT			CurX, CurY;
	FLOAT			Z;
	BYTE			Style;
	FLOAT			CurYL;
	FColor			Color;
	BITFIELD		bCenter:1;
	BITFIELD		bNoSmooth:1;
	INT				X;
	INT				Y;
	UFont			*SmallFont, *MedFont, *BigFont, *LargeFont;
	UViewport*		Viewport;
	struct FSceneNode* Frame;
	class URenderBase* Render; // Only valid during UGameEngine::Draw

	//
	// OldUnreal font scaling.
	//

	// This is the scaling factor to apply to the old fonts (SmallFont, MedFont, ...).
	// We currently only use this to scale the stats font.
	// Other text is generally rendered with actual high-resolution fonts.
	INT 			UglyFontScaling;

	// Range of unicode code points for which we should include glyphs in dynamically created font textures.
	FString			DynamicFontUnicodeRange;

	// UObject interface.
	void StaticConstructor();	

	// UCanvas interface.
	virtual void Init( UViewport* InViewport );
	virtual void Update( FSceneNode* Frame );
	virtual void DrawTile( UTexture* Texture, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, class FSpanBuffer* SpanBuffer, FLOAT Z, FPlane Color, FPlane Fog, DWORD PolyFlags );
	virtual void DrawIcon( UTexture* Texture, FLOAT ScreenX, FLOAT ScreenY, FLOAT XSize, FLOAT YSize, class FSpanBuffer* SpanBuffer, FLOAT Z, FPlane Color, FPlane Fog, DWORD PolyFlags );
	virtual void DrawPattern( UTexture* Texture, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT Scale, FLOAT OrgX, FLOAT OrgY, class FSpanBuffer* SpanBuffer, FLOAT Z, FPlane Color, FPlane Fog, DWORD PolyFlags );
	virtual void WrappedStrLenf( UFont* Font, INT& XL, INT& YL, const TCHAR* Fmt, ... );
	virtual void VARARGS WrappedPrintf( UFont* Font, UBOOL Center, const TCHAR* Fmt, ... );
	virtual void SetClip( INT X, INT Y, INT XL, INT YL );

	// Shortcuts for Cluster
	void SetPos(FLOAT X, FLOAT Y);
	void SetOrigin(FLOAT X, FLOAT Y);
	void SetClip(FLOAT X, FLOAT Y);

	// Natives.
	DECLARE_FUNCTION(execStrLen)
	DECLARE_FUNCTION(execDrawText)
	DECLARE_FUNCTION(execDrawTile)
	DECLARE_FUNCTION(execDrawActor)
	DECLARE_FUNCTION(execDrawClippedActor)
	DECLARE_FUNCTION(execDrawTileClipped)
	DECLARE_FUNCTION(execDrawTextClipped)
	DECLARE_FUNCTION(execTextSize)
	DECLARE_FUNCTION(execDrawPortal)
	DECLARE_FUNCTION(execCreateFont)
	DECLARE_FUNCTION(execGetDesktopDPI)
    void eventReset()
    {
        ProcessEvent(FindFunctionChecked(TEXT("Reset")),NULL);
    }

private:
	// Internal functions.
	void VARARGS WrappedPrint( ERenderStyle Style, INT& XL, INT& YL, UFont* Font, UBOOL Center, const TCHAR* Text );
};


inline void UCanvas::SetPos(FLOAT X, FLOAT Y)
{
	CurX = X;
	CurY = Y;
}
inline void UCanvas::SetOrigin(FLOAT X, FLOAT Y)
{
	OrgX = X;
	OrgY = Y;
}
inline void UCanvas::SetClip(FLOAT X, FLOAT Y)
{
	ClipX = X;
	ClipY = Y;
}

/*-----------------------------------------------------------------------------
	FHitCause.
-----------------------------------------------------------------------------*/

//
// Information about a hit-test cause.
//
struct ENGINE_API FHitCause
{
	class FHitObserver* Observer;
	UViewport* Viewport;
	DWORD Buttons;
	FLOAT MouseX;
	FLOAT MouseY;
	FHitCause( FHitObserver* InObserver, UViewport* InViewport, DWORD InButtons, FLOAT InMouseX, FLOAT InMouseY )
	: Observer( InObserver ), Viewport( InViewport ), Buttons( InButtons ), MouseX( InMouseX ), MouseY( InMouseY )
	{}
};

/*-----------------------------------------------------------------------------
	FHitObserver.
-----------------------------------------------------------------------------*/

//
// Hit observer class for receiving hit-test notification.
//
class ENGINE_API FHitObserver
{
public:
	virtual void Click( const struct FHitCause& Cause, const struct HHitProxy& Hit ) {}
	virtual UBOOL IsHandled() { return FALSE; }
	virtual void SetHandled(UBOOL InHandled) {}
};

/*-----------------------------------------------------------------------------
	UViewport.
-----------------------------------------------------------------------------*/

// Information for rendering the viewport (detail level settings).
enum ERenderType
{
	REN_None			= 0,	// Hide completely.
	REN_Wire			= 1,	// Wireframe of EdPolys.
	REN_Zones			= 2,	// Show zones and zone portals.
	REN_Polys			= 3,	// Flat-shaded Bsp.
	REN_PolyCuts		= 4,	// Flat-shaded Bsp with normals displayed.
	REN_DynLight		= 5,	// Illuminated texture mapping.
	REN_PlainTex		= 6,	// Plain texture mapping.
	REN_OrthXY			= 13,	// Orthogonal overhead (XY) view.
	REN_OrthXZ			= 14,	// Orthogonal XZ view.
	REN_OrthYZ			= 15,	// Orthogonal YZ view.
	REN_TexView			= 16,	// Viewing a texture (no actor).
	REN_TexBrowser		= 17,	// Viewing a texture browser (no actor).
	REN_MeshView		= 18,	// Viewing a mesh.
	REN_MAX				= 20
};

// Viewport capabilities.
enum EViewportCaps
{
	CC_RGB565			= 1,	// RGB 565 format (otherwise 555).
	CC_Mask				= 0xff, // Caps mask which affects cached rendering info.
};

// ShowFlags for viewport.
enum EViewportShowFlags
{
	SHOW_Frame     		= 0x00000001, 	// Show world bounding cube.
	SHOW_ActorRadii		= 0x00000002, 	// Show actor collision radii.
	SHOW_Backdrop  		= 0x00000004, 	// Show background scene.
	SHOW_Actors    		= 0x00000008,	// Show actors.
	SHOW_Coords    		= 0x00000010,	// Show brush/actor coords.
	SHOW_ActorIcons		= 0x00000020,	// Show actors as icons.
	SHOW_Brush			= 0x00000040,	// Show the active brush.
	SHOW_StandardView	= 0x00000080,	// Viewport is a standard view.
	SHOW_Menu			= 0x00000100,	// Show menu on viewport.
	SHOW_ChildWindow	= 0x00000200,	// Show as true child window.
	SHOW_MovingBrushes	= 0x00000400,	// Show moving brushes.
	SHOW_PlayerCtrl		= 0x00000800,	// Player controls are on.
	SHOW_Paths          = 0x00001000,   // Show paths.
	SHOW_NoButtons		= 0x00002000,	// No menu/view buttons.
	SHOW_RealTime		= 0x00004000,	// Update window in realtime.
	SHOW_BuildViewport	= 0x00008000,	// Internal viewport for build process.
	SHOW_TextureGrid	= 0x00010000,	// Use GTextureGrid for all surfaces in 3D render.
	SHOW_EventLines		= 0x00020000,	// Show Event lines.
	SHOW_MoverSurfaces	= 0x00040000,	// Show Mover surfaces.
	SHOW_MoverPath  	= 0x00080000,	// Show Mover path.
	SHOW_OccludeLines  	= 0x00100000,	// Occlude 3D lines by geometry.
};

// Mouse buttons and commands.
enum EMouseButtons	
{
	MOUSE_None			= 0x0000,
	MOUSE_Left			= 0x0001,		// Left mouse button.
	MOUSE_Right			= 0x0002,		// Right mouse button.
	MOUSE_Middle 		= 0x0004,		// Middle mouse button.
	MOUSE_FirstHit		= 0x0008,		// Sent when a mouse button is initially hit.
	MOUSE_LastRelease	= 0x0010,		// Sent when last mouse button is released.
	MOUSE_SetMode		= 0x0020,		// Called when a new viewport mode is first set.
	MOUSE_ExitMode		= 0x0040,		// Called when the existing mode is changed.
	MOUSE_Ctrl			= 0x0080,		// Ctrl is pressed.
	MOUSE_Shift			= 0x0100,		// Shift is pressed.
	MOUSE_Alt			= 0x0200,		// Alt is pressed.
	MOUSE_LeftDouble	= 0x0200,		// Left double click.
	MOUSE_WheelUp		= 0x0800,		// Mouse wheel up.
	MOUSE_WheelDown		= 0x1000,		// Mouse wheel down.
	MOUSE_Button4		= 0x2000,		// Shoulder button 1.
	MOUSE_Button5		= 0x4000		// Shoulder button 2.
};

//
// A viewport object, which associates an actor (which defines
// most view parameters) with a Windows window.
//
class ENGINE_API UViewport : public UPlayer
{
	DECLARE_ABSTRACT_CLASS(UViewport,UPlayer,CLASS_Transient,Engine)
	DECLARE_WITHIN(UClient)

	// Referenced objects.
	class UCanvas*		 Canvas;	// Viewport's painting canvas.
	class UInput*		 Input;		// Input system.
	class URenderDeviceOldUnreal469* RenDev;	// Render device.

	// Normal variables.
	UObject*		MiscRes;		// Used in in modes like EM_TEXVIEW.
	FName			Group;			// Group for editing.
	FTime			LastUpdateTime;	// Time of last update.
	INT				SizeX, SizeY;   // Buffer X & Y resolutions.
	unsigned int	ColorBytes;		// 1=256-color, 4=32-bit color.
	INT				FrameCount;		// Frame count, incremented when locked.
	DWORD			Caps;			// Capabilities (CC_).
	UBOOL			Current;		// If this is the current input viewport.
	UBOOL			Dragging;		// Dragging mouse.
	DWORD			RenderFlags;	// Render locking flags (only when locked).
	DWORD			ExtraPolyFlags;	// Additional poly flags associated with camera.

	// Level traveling.
	ETravelType		TravelType;
	FStringNoInit	TravelURL;
	UBOOL			bTravelItems;

	// Frame buffer info; only valid when locked.
	FTime			CurrentTime;	// Time when initially locked.
	BYTE*			ScreenPointer;	// Pointer to screen frame buffer, or NULL if none.
	INT				Stride;			// Stride in pixels.

	// Hit testing.
	UBOOL			HitTesting;		// Whether hit-testing.
	INT				HitX, HitY;		// Hit rectangle top left.
	INT				HitXL, HitYL;	// Hit size.
	TArray<INT>		HitSizes;		// Currently pushed hit sizes.

	// Saved-actor parameters.
	FLOAT SavedOrthoZoom, SavedFovAngle;
	INT SavedShowFlags, SavedRendMap, SavedMisc1, SavedMisc2;

	FTime			LastNetSpeedUpdate; //elmuerte: limit netspeed change rate (Bug #104)

	// UED2 Skeletal Mesh support
	UBOOL			ShowSkel, ShowNames; //To enable/disable Skeleton Mesh information in MeshBrowser.

	// stijn: smooth lighting transition fix
	FLOAT			AccumulatedLightingDelta;
	FLOAT			NextAccumulatedLightingDelta;

	// stijn: fix for slow dragging in non-realtime viewports
	UBOOL			RepaintPending;

	// Constructor.
	UViewport();

	// UObject interface.
	void Destroy();
	void Serialize( FArchive& Ar );

	// FArchive interface.
	void Serialize( const TCHAR* Data, EName MsgType );

	// UPlayer interface.
	void ReadInput( FLOAT DeltaSeconds );

	// UViewport interface.
	virtual UBOOL SetDrag( UBOOL NewDrag );
	virtual UBOOL IsFullscreen()=0;
	virtual UBOOL Lock( FPlane FlashScale, FPlane FlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* HitData=NULL, INT* HitSize=NULL );
	virtual UBOOL ResizeViewport( DWORD BlitType, INT X=INDEX_NONE, INT Y=INDEX_NONE, INT ColorBytes=INDEX_NONE )=0;
	virtual void Unlock( UBOOL Blit=0 );
	virtual void SetModeCursor()=0;
	virtual void UpdateWindowFrame()=0;
	virtual void OpenWindow( void* ParentWindow, UBOOL Temporary, INT NewX, INT NewY, INT OpenX, INT OpenY, const TCHAR* ForcedRenDevClass=NULL )=0;
	virtual void CloseWindow()=0;
	virtual void UpdateInput( UBOOL Reset )=0;
	virtual void* GetWindow()=0;
	virtual void* GetServer();
	virtual void SetMouseCapture( UBOOL Capture, UBOOL Clip, UBOOL FocusOnly=0 )=0;
	virtual void Repaint( UBOOL Blit )=0;
	virtual UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar=*GLog );
	virtual void TryRenderDevice( const TCHAR* ClassName, INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen ) {}

	// Functions.
	void ExecuteHits( const FHitCause& Cause, BYTE* HitData, INT HitSize );
	void PushHit( const struct HHitProxy& Hit, INT Size );
	void PopHit( UBOOL bForce );
	UBOOL IsWire();
	void ExecMacro( const TCHAR* Filename, FOutputDevice& Ar=*GLog );	

	// UViewport inlines.
	BYTE* _Screen( INT X, INT Y )
	{
		return ScreenPointer + (X+Y*Stride)*ColorBytes;
	}
	UBOOL IsOrtho()
	{
		return GIsEditor && (Actor->RendMap == REN_OrthXY || Actor->RendMap == REN_OrthXZ || Actor->RendMap == REN_OrthYZ);
	}
	UBOOL IsRealtime()
	{
		return Actor && (Actor->ShowFlags&(SHOW_RealTime | SHOW_PlayerCtrl));
	}
	UBOOL IsRealtimeAudio()
	{
		return IsRealtime() && Actor->RendMap != REN_TexView  && Actor->RendMap != REN_TexBrowser && Actor->RendMap != REN_MeshView;
	}
	BYTE ByteID()
	{
		BYTE ColorBytesID[5]={0,0,1,2,3};
		return ColorBytesID[ColorBytes] | (Caps & 7)*4;
	}
	UBOOL IsPaused()
	{
		return !Actor || !Actor->Level || Actor->Level->Pauser != TEXT("");
	}

#ifdef UTPG_WINXP_FIREWALL
	virtual UBOOL FireWallHack(INT Cmd) { return 1; }	// JMW - Hack for fixing the UT2004 Firewall support
#endif
};

// Viewport hit-testing macros.
#define PUSH_HIT(frame,expr) if( (frame->Viewport)->HitTesting ) (frame->Viewport)->PushHit(expr,sizeof(expr));
#define POP_HIT(frame)       if( (frame->Viewport)->HitTesting ) (frame->Viewport)->PopHit(0);
#define POP_HIT_FORCE(frame) if( (frame->Viewport)->HitTesting ) (frame->Viewport)->PopHit(1);

/*-----------------------------------------------------------------------------
	Base hit proxy.
-----------------------------------------------------------------------------*/

// Hit proxy declaration macro.
#define DECLARE_HIT_PROXY(cls,parent) \
	const TCHAR* GetName() const \
		{ return TEXT(#cls); } \
	UBOOL IsA( const TCHAR* Str ) const \
		{ return appStricmp(TEXT(#cls),Str)==0 || parent::IsA(Str); }

// Base class for detecting user-interface hits.
struct ENGINE_API HHitProxy
{
	union
	{
		mutable INT Size;
		HHitProxy* Parent{};
	};
	const TCHAR* GetName() const
	{
		return TEXT("HHitProxy");
	}
	virtual UBOOL IsA( const TCHAR* Str ) const
	{
		return appStricmp(TEXT("HHitProxy"),Str)==0;
	}
	virtual void Click( const FHitCause& Cause )
	{
		Cause.Observer->Click( Cause, *this );
	}
};

/*-----------------------------------------------------------------------------
	UClient.
-----------------------------------------------------------------------------*/

class FGlyphInfo
{
public:
	TCHAR Ch{};			    // Code Point
	INT CellHeight{};		// Height of the character cell
	INT CellWidth{};		// Width of the character cell
	INT BoxHeight{};	    // Height of the smallest rectangle that completely encloses the glyph (i.e., the black box)
	INT BoxWidth{};			// Width of the smallest rectangle that completely encloses the glyph (i.e., the black box)
	INT OutlineSize{};      // Size of the outline bitmap (only used for TrueType fonts rendered on Windows using GetGlyphOutline)
	INT BoxOriginY{};		// Horizontal Offset of the GetGlyphOutline black box within the character cell
	INT BoxOriginX{};		// Vertical Offset of the GetGlyphOutline black box within the character cell (aka the left bearing)
	INT RightBearing{};		// Spacing on the right side of the glyph black box 
	void* GlyphSurf{};	    // Precached glyph surface - SDL2_ttf needs this to calculate BlitHeight and BlitWidth
};

//
// Client, responsible for tracking viewports.
//
class ENGINE_API UClient : public UObject
{
	DECLARE_ABSTRACT_CLASS(UClient,UObject,CLASS_Config,Engine)

	// Variables.
	UEngine*			Engine;
	TArray<UViewport*>	Viewports;
	INT					DrawCycles;

	// Configurable.
	BITFIELD	CaptureMouse;
	BITFIELD	CurvedSurfaces;
	BITFIELD	ScreenFlashes;
	BITFIELD	NoLighting;
	BITFIELD	Decals;
	BITFIELD	NoDynamicLights;
	BITFIELD    NoFractalAnim;
	INT			WindowedViewportX;
	INT			WindowedViewportY;
	INT			WindowedColorBits;
	INT			FullscreenViewportX;
	INT			FullscreenViewportY;
	INT			FullscreenColorBits;
	FLOAT		Brightness;
	FLOAT		MipFactor;
	INT			TextureLODSet[LODSET_MAX];
	FLOAT		MinDesiredFrameRate;
	FLOAT		FrameRateLimit;
	INT			ParticleDensity;
	
	// Constructors.
	UClient();
	void StaticConstructor();

	// UObject interface.
	void Serialize( FArchive& Ar );
	void Destroy();
	void PostEditChange();

	// UClient interface.
	virtual void Init( UEngine* InEngine )=0;
	virtual void Flush( UBOOL AllowPrecache );
	virtual void ShowViewportWindows( DWORD ShowFlags, int DoShow )=0;
	virtual void EnableViewportWindows( DWORD ShowFlags, int DoEnable )=0;
	virtual void Tick()=0;
	virtual UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar=*GLog )=0;
	virtual class UViewport* NewViewport( const FName Name )=0;
	virtual void MakeCurrent( UViewport* NewViewport )=0;

	// stijn: clipboard interface added in v469
#if !OLDUNREAL_BINARY_COMPAT
	virtual FString GetClipboardText()=0;
	virtual UBOOL SetClipboardText(FString& Text)=0;
#endif

#if !OLDUNREAL_BINARY_COMPAT && !WIN32
	virtual INT GetDPIScaledX(INT X)=0;
	virtual INT GetDPIScaledY(INT Y)=0;
	virtual void* CreateTTFFont(const TCHAR* FontName, int Height, int Italic, int Bold, int Underlined, int AntiAliased)=0;
	virtual void MakeGlyphsList(FBitmask& RequestedGlyphs, void* InFont, TArray<FGlyphInfo>& OutGlyphs, INT AntiAlias)=0;
	virtual void RenderGlyph(FGlyphInfo& Info, UTexture* PageTexture, INT X, INT Y, INT YAdjust)=0;
	virtual void DestroyGlyphsList(TArray<FGlyphInfo>& Glyphs)=0;
	virtual void DestroyTTFFont(void* Font)=0;
#endif
};

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (pop)
#endif

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
