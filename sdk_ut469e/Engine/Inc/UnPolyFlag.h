/*=============================================================================
	UnPolyFlag.h: Flags describing effects and properties of a bsp surface or 
    mesh polygon.
	Copyright 2023 OldUnreal. All Rights Reserved.

	Revision history:
		* Created by Stijn Volckaert
=============================================================================*/

#pragma once

enum EPolyFlags
{
	// Regular in-game flags.
	PF_None               = 0x00000000, // No Flags.
	PF_Invisible          = 0x00000001, // Poly is invisible.
	PF_Masked             = 0x00000002, // Poly should be drawn masked.
	PF_Translucent        = 0x00000004, // Poly is transparent.
	PF_NotSolid           = 0x00000008, // Poly is not solid, doesn't block.
	PF_Environment        = 0x00000010, // Poly should be drawn environment mapped.
	PF_ForceViewZone      = 0x00000010, // Force current iViewZone in OccludeBSP (reuse Environment flag)
	PF_Semisolid          = 0x00000020, // Poly is semi-solid = collision solid, Csg nonsolid.
	PF_Modulated          = 0x00000040, // Modulation transparency.
	PF_FakeBackdrop       = 0x00000080, // Poly looks exactly like backdrop.
	PF_TwoSided           = 0x00000100, // Poly is visible from both sides.
	PF_AutoUPan           = 0x00000200, // Automatically pans in U direction.
	PF_AutoVPan           = 0x00000400, // Automatically pans in V direction.
	PF_NoSmooth           = 0x00000800, // Don't smooth textures.
	PF_BigWavy            = 0x00001000, // Poly has a big wavy pattern in it.
	PF_SpecialPoly        = 0x00001000, // Game-specific poly-level render control (reuse BigWavy flag)	
	PF_SmallWavy          = 0x00002000, // Small wavy pattern (for water/enviro reflection).
	PF_Flat               = 0x00004000, // Flat surface.
	PF_LowShadowDetail    = 0x00008000, // Low detaul shadows.
	PF_NoMerge            = 0x00010000, // Don't merge poly's nodes before lighting when rendering.
	PF_CloudWavy          = 0x00020000, // Polygon appears wavy like clouds.
	PF_AlphaTexture		  = 0x00020000, // Honor texture alpha (reuse CloudWavy flag)
	PF_DirtyShadows       = 0x00040000, // Dirty shadows.
	PF_BrightCorners      = 0x00080000, // Brighten convex corners.
	PF_SpecialLit         = 0x00100000, // Only speciallit lights apply to this poly.
	PF_Gouraud            = 0x00200000, // Gouraud shaded.
	PF_NoBoundRejection   = 0x00200000, // Disable bound rejection in OccludeBSP (reuse Gourard flag)
	PF_Unlit              = 0x00400000, // Unlit.
	PF_HighShadowDetail   = 0x00800000, // High detail shadows.
	PF_RenderHint         = 0x01000000, // Rendering optimization hint.
	PF_Memorized          = 0x01000000, // Editor: Poly is remembered.
	PF_Selected           = 0x02000000, // Editor: Poly is selected.
	PF_Portal             = 0x04000000, // Portal between iZones.
	PF_Mirrored           = 0x08000000, // Reflective surface.
	PF_Highlighted        = 0x10000000, // Premultiplied alpha blending.
	PF_FlatShaded         = 0x40000000, // Render with FlatColor.
	PF_RenderFog          = 0x40000000, // Render with fogmapping.
	PF_EdProcessed        = 0x40000000, // FPoly was already processed in editorBuildFPolys.
	PF_EdCut              = 0x80000000, // FPoly has been split by SplitPolyWithPlane.  
	PF_Occlude            = 0x80000000, // Occludes even if PF_NoOcclude.

	// Combinations of PolyFlags.
	PF_NoOcclude          = PF_Invisible | PF_Highlighted | PF_AlphaTexture | PF_Translucent | PF_Modulated | PF_Masked, // In decreasing precedence.
	PF_NoEdit             = PF_Memorized | PF_Selected    | PF_EdProcessed  | PF_NoMerge     | PF_EdCut,
	PF_NoImport           = PF_NoEdit,
	PF_AddLast            = PF_Semisolid | PF_NotSolid,
	PF_NoAddToBSP         = PF_EdCut     | PF_EdProcessed | PF_Selected | PF_Memorized,
	PF_NoShadows          = PF_Unlit     | PF_Invisible   | PF_FakeBackdrop,
	PF_Transient          = PF_None,
};

#define PF_AlphaBlend					PF_AlphaTexture
#define PF_Straight_AlphaBlend			PF_AlphaTexture	// Render texture/surface with straight alphablending
#define PF_Premultiplied_AlphaBlend		PF_Highlighted  // Render texture/surface with premultiplied alphablending. This is what PF_Highlighted already did before OldUnreal took over maintenance
