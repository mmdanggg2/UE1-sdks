//
// UnRenStd.h
//

/*------------------------------------------------------------------------------
	Unlit Meshes and Sprites.
------------------------------------------------------------------------------*/

//
// Calculates Unlit Color and for Mesh and Sprite rendering.
//
// ScaleGlow acted as some makeshift alpha modifier for translucent,
// so generalize that concept now.
//
// Unlit light is now matched to Surfaces, which is double the amount as
// before, but AmbientGlow will be ignored now.
//
// NarrowClamp will force maximum and minium values to 1.0, as this may
// be needed depending on the rendering backend implementation.
//
// Not setting NarrowClamp will allow 2.0 as maximum values, which matches
// Surface lighting maximum.
//
// You should only pass selected for Editor.
//
// For the non modulated case Color is meant to be directly multiplied
// with the Texture data, while Fog is meant as an additional highlighted
// overlay.
//
// For the modulated case it is intended to use the per channel supplied
// values to scale the modulation amount. See FScaleModulation.
//
// For Masked, you should use the Solid variants, Color.A will be alwas be 1.0.
// For Translucent, you should use the Highlighted variant, as Translucent
// was always some kind of premultiplied alphablendingish effect.
//
// Though the Actor is unlit, you should probably still calculate and 
// use the volumetric vertex Fog per vertex.
//

inline FColor32F FActorSolidUnlitColor( FLOAT ScaleGlow, UBOOL NarrowClamp=1 )
{
	ScaleGlow = NarrowClamp ? Clamp<FLOAT>( ScaleGlow, 0.0f, 1.0f ) : Max<FLOAT>( ScaleGlow, 0.0f );

	return FColor32F( ScaleGlow, ScaleGlow, ScaleGlow, 1.0f );
}

inline FColor32F FActorHighlightedUnlitColor( FLOAT ScaleGlow, UBOOL NarrowClamp=1 )
{
	ScaleGlow = NarrowClamp ? Clamp<FLOAT>( ScaleGlow, 0.0f, 1.0f ) : Max<FLOAT>( ScaleGlow, 0.0f );

	return FColor32F( ScaleGlow, ScaleGlow, ScaleGlow, Clamp<FLOAT>( ScaleGlow, 0.0f, 1.0f ) );
}

inline FColor32F FActorAlphaTextureUnlitColor( FLOAT ScaleGlow, UBOOL NarrowClamp=1 )
{
	//
	// After enforcing ScaleGlow>=0.0, we need to handle the ScaleGlow>1.0 
	// and ScaleGlow<1.0 cases seperate.
	//
	// For ScaleGlow>1.0, we want to pass ScaleGlow forward as is to affect
	// Color, but need to max Alpha at 1.0.
	//
	// For ScaleGlow<1.0 we can pass Alpha as is, but need to keep in mind
	// that we are doing straight alphablending here, so Color will get 
	// multiplied with Alpha during blending, so we want to always pass
	// 1.0 for the Color components.
	//
	FLOAT Alpha     = Clamp<FLOAT>( ScaleGlow, 0.0f, 1.0f );
	FLOAT OverGlow  = NarrowClamp ? 1.0f : Max<FLOAT>( ScaleGlow, 1.0f );

	return FColor32F( OverGlow,  OverGlow,  OverGlow, Alpha );
}

inline FColor32F FActorModulatedUnlitColor( FLOAT ScaleGlow, UBOOL NarrowClamp=1 )
{
	//
	// You know, negative ScaleGlow could actually be a thing in the Context of Modulation,
	// flipping whats below and above 0.5.
	//
	// I'm not sure if that would have any practical applications, but I'm leaving
	// it in for now, maybe there are some legit use cases. --han
	//
	// Alpha value may be subjected to change.
	//
	ScaleGlow = NarrowClamp ? Clamp<FLOAT>( ScaleGlow, 0.0f, 1.0f ) : ScaleGlow;

	return FColor32F( ScaleGlow, ScaleGlow, ScaleGlow, Abs<FLOAT>(ScaleGlow) );
}

// Unlikely to change. See large comment above.
#define FActorInvisibleUnlitColor   FActorSolidUnlitColor
#define FActorTranslucentUnlitColor FActorHighlightedUnlitColor
#define FActorMaskedUnlitColor      FActorSolidUnlitColor


//
// Unified version:
//
//  Solid/Masked: Ignore alpha.
//  Modulated:    Use alpha for all components.
//  Translucent:  Apply alpha to Diffuse for per channel alpha occlusion values.
//  Highlighted:  Just apply.
//  AlphaBlended: Divide RGB by alpha, or run custom code.
//
// Function is periodic in third argument with T=16*PI:
//  
//  FActorUnlitColor(Scale,Ambient,Time) = FActorUnlitColor(Scale,Ambient,Time+16*PI)
//
// You should actively exploit this behaviour to remove precission issues.
//
#define FACTORUNLITCOLOR_PERIOD (PI*16.0f)
inline FColor32F FActorUnlitColor( FLOAT InScaleGlow, BYTE AmbientGlow, FLOAT CurrentSeconds, UBOOL OldUnlitColors )
{
	if (OldUnlitColors)
	{
		FLOAT Unlit = Clamp(InScaleGlow * 0.5f + AmbientGlow / 256.f, 0.f, 1.f);
		return FColor32F(Unlit, Unlit, Unlit, 1.f);
	}
	
	// We could support negative ScaleGlow on modulated, but it would make things
	// a lot more complicated for a questionable feature.
	FLOAT ScaleGlow = Max<FLOAT>( InScaleGlow, 0.0f );

	// ScaleGlow acted as some makeshift alpha modifier for translucent, so this is the
	// generalization of that concept now.
	FLOAT Alpha = Min<FLOAT>( ScaleGlow, 1.0f );

	// Either constant ambient or periodic glowing for AmbientGlow==255.
	// The last case was originally not performed for unlit Actors.
	FLOAT Ambient = AmbientGlow==255 ? (0.25f+0.2f*appSin(CurrentSeconds*8.0f)) : (AmbientGlow/255.0f);

	// Idea here is that Unlit and ScaleGlow=1.0 should output the texture as is,
	// while the AmbientGlow is an added effect, which should also be affected
	// by the overall ScaleGlow modified.
	//
	// Originally ScaleGlow*0.5+AmbientGlow/255 presumably to account for
	// hardware limiation on color components above 1.0.
	FLOAT Component = ScaleGlow*(1.0f+Ambient);

	// Clamp maximum Light on Actors to 2.0, to match behaviour of surfaces.
	// Also note that some content might noticably break without this clamping,
	// so I'm quite fine witht keeping it here.
	Component = Clamp(Min<FLOAT>( Component, 2.0f ), 0.f, 1.f);

	// These components may need to be further clamped into 0..1,
	// as legacy RenDevs may expect this limited range without checks!
	return FColor32F( Component, Component, Component, Alpha );
}

/*------------------------------------------------------------------------------
	Concepts and examples.
------------------------------------------------------------------------------*/

//
// Example of how to scale the intensity of a modulation Color,
// based on some sort of alpha blending like multiplier.
//
// Not that Scale can be negative, which inverses the darkening
// and brightening effect.
//
// Color.A is assumed to be 1.0 for identify and Color.RGB is assumed
// to be 0.5 for identity.
//
inline FColor32F FScaleModulation( FColor32F Color, FLOAT Scale )
{
	return (Color-FGlobalColors::ClearGrey())*Scale+FGlobalColors::ClearGrey();
}

//
// Example of how to scale  the intensitfy of a modulation Color,
// based on some full channel alpha blending like multiplied.
//
inline FColor32F FScaleModulation( FColor32F Color, FColor32F Scale )
{

	return (Color-FGlobalColors::ClearGrey())*Scale+FGlobalColors::ClearGrey();
}

/*------------------------------------------------------------------------------
	Hardcoded Colors for Editor.
------------------------------------------------------------------------------*/

//
// Idea is to render selected Actors as Unlit and Unfogged, but instead
// use fog overlay to display the selection.
//
// I'm not sure yet, how to handle the modulated case. --han
//
inline FColor32F FActorSelectionFog()
{
	return FColor32F( 0.0f, 0.6f, 0.0f, 0.6f ); // Intense.
}

inline FColor32F FSurfaceSelectionFog()
{
	return FColor32F( 0.6f, 0.0f, 0.0f, 0.6f ); // Intense.
}

/*------------------------------------------------------------------------------
	The End.
------------------------------------------------------------------------------*/
