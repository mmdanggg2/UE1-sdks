//=============================================================================
// Canvas: A drawing canvas.
// This is a built-in Unreal class and it shouldn't be modified.
//
// Notes.
//   To determine size of a drawable object, set Style to STY_None,
//   remember CurX, draw the thing, then inspect CurX and CurYL.
//=============================================================================
class Canvas extends Object
	native
	noexport
	config(System);

// Objects.
#exec Font Import File=Textures\SmallFont.bmp Name=SmallFont
#exec Font Import File=Textures\MedFont.pcx   Name=MedFont
#exec Font Import File=Textures\LargeFont.pcx Name=LargeFont
#exec Font Import File=Textures\BigFont.pcx   Name=BigFont

// Modifiable properties.
var font    Font;            // Font for DrawText.
var float   SpaceX, SpaceY;  // Spacing for after Draw*.
var float   OrgX, OrgY;      // Origin for drawing.
var float   ClipX, ClipY;    // Bottom right clipping region.
var float   CurX, CurY;      // Current position for drawing.
var float   Z;               // Z location. 1=no screenflash, 2=yes screenflash.
var byte    Style;           // Drawing style STY_None means don't draw.
var float   CurYL;           // Largest Y size since DrawText.
var color   DrawColor;       // Color for drawing.
var bool    bCenter;         // Whether to center the text.
var bool    bNoSmooth;       // Don't bilinear filter.
var const int SizeX, SizeY;  // Zero-based actual dimensions.

// Stock fonts.
var font SmallFont;          // Small system font.
var font MedFont;            // Medium system font.
var font BigFont;            // Big system font.
var font LargeFont;          // Large system font.

// Internal.
var const viewport Viewport;                // Viewport that owns the canvas.
var const pointer FramePtr;                 // Scene frame pointer.
var const pointer RenderPtr;	            // Render device pointer, only valid during UGameEngine::Draw
var const int     UglyFontScaling;          // Scale applied to system font glyphs. For internal use only. Mod authors should use Botpack.FontInfo to get appropriately scaled glyphs instead
var config string DynamicFontUnicodeRange;  // Unicode code points to generate glyphs for when using dynamically created font textures.

// These font families should be available on all of the platforms we support
enum FontFamily
{
	FF_Arial,       // sans-serif font. Windows will use Arial when requested. Linux/Mac use Helvetica.
	FF_Times,		// serif font. Windows uses Times New Roman. Linux/Mac use times
	FF_Courier,		// monospace font. Windows uses Courier New. Linux/Mac use courier
	FF_Tahoma,		// sans-serif font. This is the standard UWindow font. Linux/Mac will use Verdana if requested.
};

// native functions.
native(464) final function StrLen( coerce string String, out float XL, out float YL );
native(465) final function DrawText( coerce string Text, optional bool CR );
native(466) final function DrawTile( texture Tex, float XL, float YL, float U, float V, float UL, float VL );
native(467) final function DrawActor( Actor A, bool WireFrame, optional bool ClearZ );
native(468) final function DrawTileClipped( texture Tex, float XL, float YL, float U, float V, float UL, float VL );
native(469) final function DrawTextClipped( coerce string Text, optional bool bCheckHotKey );
native(470) final function TextSize( coerce string String, out float XL, out float YL );
native(471) final function DrawClippedActor( Actor A, bool WireFrame, int X, int Y, int XB, int YB, optional bool ClearZ );
native(480) final function DrawPortal( int X, int Y, int Width, int Height, actor CamActor, vector CamLocation, rotator CamRotation, optional int FOV, optional bool ClearZ );

//
// OldUnreal dynamic font creation support
//

// Note, this function may not be able to create the exact font that was 
// requested, but it will usually be able to create a similar-looking font.
native static final function Font CreateFont(FontFamily Font, int Size, bool Bold, bool Italic, bool Underlined, bool DPIScaled, bool AntiAliased);
native static final function int GetDesktopDPI();

// UnrealScript functions.
event Reset()
{
	Font            = Default.Font;
	UglyFontScaling = Default.UglyFontScaling;
	SpaceX          = Default.SpaceX;
	SpaceY          = Default.SpaceY;
	OrgX            = Default.OrgX;
	OrgY            = Default.OrgY;
	CurX            = Default.CurX;
	CurY            = Default.CurY;
	Style           = Default.Style;
	DrawColor       = Default.DrawColor;
	CurYL           = Default.CurYL;
	bCenter         = false;
	bNoSmooth       = false;
	Z               = 1.0;
}
final function SetPos( float X, float Y )
{
	CurX = X;
	CurY = Y;
}
final function SetOrigin( float X, float Y )
{
	OrgX = X;
	OrgY = Y;
}
final function SetClip( float X, float Y )
{
	ClipX = X;
	ClipY = Y;
}
final function DrawPattern( texture Tex, float XL, float YL, float Scale )
{
	DrawTile( Tex, XL, YL, (CurX-OrgX)*Scale, (CurY-OrgY)*Scale, XL*Scale, YL*Scale );
}
final function DrawIcon( texture Tex, float Scale )
{
	if ( Tex != None )
		DrawTile( Tex, Tex.USize*Scale, Tex.VSize*Scale, 0, 0, Tex.USize, Tex.VSize );
}
final function DrawRect( texture Tex, float RectX, float RectY )
{
	DrawTile( Tex, RectX, RectY, 0, 0, Tex.USize, Tex.VSize );
}

defaultproperties
{
     UglyFontScaling=1
     Style=1
     Z=1
     DrawColor=(R=127,G=127,B=127)
     SmallFont=Font'Engine.SmallFont'
     MedFont=Font'Engine.MedFont'
     BigFont=Font'Engine.BigFont'
     LargeFont=Font'Engine.LargeFont'
	 DynamicFontUnicodeRange="0000-007f,0080-00ff,0100-017f,0180-024f,0370-03ff,0400-04ff,0500-052f,2de0-2dff,a640-a69f,1c80-1c8f"
}
