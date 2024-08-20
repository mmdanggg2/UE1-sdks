/*=============================================================================
	OpenGL.cpp: Unreal OpenGL support implementation for Windows and Linux.
	Copyright 1999-2001 Epic Games, Inc. All Rights Reserved.
	
	Other URenderDevice subclasses include:
	* USoftwareRenderDevice: Software renderer.
	* UGlideRenderDevice: 3dfx Glide renderer.
	* UDirect3DRenderDevice: Direct3D renderer.
	* UOpenGLRenderDevice: OpenGL renderer.

	Revision history:
	* Created by Daniel Vogel based on XMesaGLDrv
	* Changes (John Fulmer, Jeroen Janssen)
	* Major changes (Daniel Vogel)
	* Ported back to Win32 (Fredrik Gustafsson)
	* Unification and addition of vertex arrays (Daniel Vogel)
	* Actor triangle caching (Steve Sinclair)
    * One pass fogging (Daniel Vogel)
	* Windows gamma support (Daniel Vogel)
	* 2X blending support (Daniel Vogel)
	* Better detail texture handling (Daniel Vogel)
	* Scaleability (Daniel Vogel)
	* Texture LOD bias (Daniel Vogel)	
	* RefreshRate support on Windows (Jason Dick)
	* Finer control over gamma (Daniel Vogel)
	* Initial support for Nerf Arena Blast (Sebastian Kaufel)
	* Removed FANCY_FISHEYE and TNT code (Sebastian Kaufel)
	* Editor fixes (Sebastian Kaufel)
	* Support for TEXF_RGBA8. (Sebastian Kaufel)
	* Force ShareLists for Editor (Sebastian Kaufel)
	* Removed Use4444Textures (Sebastian Kaufel)
	* Removed UseFilterSGIS (Sebastian Kaufel)
	* Always enable S3TC (Sebastian Kaufel)
	* Always use UseAlphaPalette (Sebastian Kaufel)
	* Removed UsePalette (Sebastian Kaufel)
	* Fixed Gamma Handling and made it relative to the orignal ramp (Sebastian Kaufel)
	* Removed MaxLogUOverV and MaxLogVOverU options (Sebastian Kaufel)
	* Removed GammaOffset option (Sebastian Kaufel)
	* Removed MinLogTextureSize/MaxLogTextureSize config options (Sebastian Kaufel)
	* Use static OpenGL 1.1/WGL functions. Load functions dynamic per instance. (Sebastian Kaufel)
	* Special handling for Masked/Non Masked P8 Textures. (Sebastian Kaufel)
	* Special handling for DXT1 (DXT1Mode). (Sebastian Kaufel)
	* Support for TEXF_RGB8, TEXF_DXT3 and TEXF_DXT5. (Sebastian Kaufel)

	UseTrilinear      whether to use trilinear filtering
	MaxAnisotropy     maximum level of anisotropy used
	MaxTMUnits        maximum number of TMUs UT will try to use
	DisableSpecialDT  disable the special detail texture approach
	LODBias           texture lod bias
	RefreshRate       requested refresh rate (Windows only)
	NoFiltering       uses GL_NEAREST as min/mag texture filters
	DXT1Mode           whether to upload TEFX_DXT1 as RGB, RGBA or depending on PF_Masked

TODO:
	- DOCUMENTATION!!! (especially all subtle assumptions)
	- get rid of some unnecessary state changes (#ifdef out)
	- Fix crash for too long shader compile error output.
	- Replace ClearZ for Lines/Points in Editor Ortho view with disabling of depth test.
	- Fix surface not fogged as seen in 02_NYC_BatteryPark
	- Further investigate gamma correction of textures.
	- Fix AugTarget/AugDrone display in DeusEx.
	- Add VSync wglSwapInterval/glxSwapInterval/SDL_GL_SetSwapInterval
	- Add SDL_GL_SetSwapInterval for SDL build.
	- Further investigate how to avoid 7777 resampling.
=============================================================================*/

#include "OpenGLDrv.h"
#include <math.h>

/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

#define MAX_TMUNITS          4	// vogel: maximum number of texture mapping units supported
#define VERTEX_ARRAY_SIZE 2000	// vogel: better safe than sorry

/*-----------------------------------------------------------------------------
	OpenGLDrv.
-----------------------------------------------------------------------------*/

//
// An OpenGL rendering device attached to a viewport.
//
class UOpenGLRenderDevice : public URenderDevice
{
	DECLARE_CLASS(UOpenGLRenderDevice,URenderDevice,CLASS_Config)

	// Information about a cached texture.
	struct FCachedTexture
	{
		// Ids[0]: Non Masked
		// Ids[1]: Masked
		GLuint Ids[2];
		INT BaseMip;
		INT UBits, VBits;
		INT UCopyBits, VCopyBits;
	};

	// Geometry 	
	struct FGLVertex
	{
		FLOAT x;
		FLOAT y;
		FLOAT z;
	} VertexArray[VERTEX_ARRAY_SIZE];

	// Texcoords	
	struct FGLTexCoord
	{
		struct FGLTMU {
			FLOAT u;
			FLOAT v;
		} TMU [MAX_TMUNITS];
	} TexCoordArray[VERTEX_ARRAY_SIZE];

	// Primary and secondary (specular) color
	struct FGLColor
	{
		FLOAT color[4];
		FLOAT specular[4];
	} ColorArray[VERTEX_ARRAY_SIZE];

	struct FGLMapDot
	{
		FLOAT u;
		FLOAT v;
	} MapDotArray [VERTEX_ARRAY_SIZE];
	
	// MultiPass rendering information
	struct FGLRenderPass
	{
		struct FGLSinglePass
		{
			INT Multi;
			FTextureInfo* Info;
			DWORD PolyFlags;
			FLOAT PanBias;
		} TMU[MAX_TMUNITS];

		FSavedPoly* Poly;
	} MultiPass;				// vogel: MULTIPASS!!! ;)

	struct FGammaRamp
	{
		_WORD red[256];
		_WORD green[256];
		_WORD blue[256];
	};

	enum EDXT1Modes
	{
		DXT1_RGB     = 0,
		DXT1_RGBA    = 1,
		DXT1_Special = 2,
	};

	enum EVSyncModes
	{
		VSync_Adaptive = 0,
		VSync_On       = 1,
		VSync_Off      = 2,
	};

#ifdef WIN32
	// Permanent variables.
	HGLRC hRC;
	HWND hWnd;
	HDC hDC;

	// Gamma Ramp to restore at exit	
	static FGammaRamp OriginalRamp;
#endif

	UBOOL WasFullscreen;

	TOpenGLMap<QWORD,FCachedTexture> LocalBindMap, *BindMap;

	TArray<FPlane> Modes;
	UViewport* Viewport;

	// Timing.
	DWORD BindCycles, ImageCycles, ComplexCycles, GouraudCycles, TileCycles, Resample7777Cycles;

	// Hardware constraints.
	BYTE DXT1Mode, VSync;
	FLOAT LODBias;
	INT MinLogTextureSize;
	INT MaxLogTextureSize;
	INT MaxAnisotropy;
	INT TMUnits;
	INT MaxTMUnits;
	INT RefreshRate;
	UBOOL UsePrecache;
	UBOOL UseMultiTexture;
	UBOOL ShareLists;
	UBOOL AlwaysMipmap;
	UBOOL UseTrilinear;
	UBOOL UseVertexSpecular;
	UBOOL UseCVA;
	UBOOL UseDetailAlpha;
	UBOOL NoFiltering;
	UBOOL BufferActorTris;
	UBOOL UseSRGBTextures;
	UBOOL UseSRGBLightmaps;
	UBOOL UseGammaCorrection;
	UBOOL UseEditorGammaCorrection;
	BITFIELD DetailTextures;
	INT BufferedVerts;

	UBOOL ColorArrayEnabled;
	UBOOL RenderFog;
	static UBOOL GammaFirstTime;

	INT AllocatedTextures;
	INT PassCount;

	// Hit info.
	BYTE* HitData;
	INT* HitSize;

	// Lock variables.
	FPlane FlashScale, FlashFog;
	FLOAT RProjZ, Aspect;
	DWORD CurrentPolyFlags;
	DWORD CurrentEnvFlags[MAX_TMUNITS];	
	TArray<INT> GLHitData;
	struct FTexInfo
	{
		QWORD CurrentCacheID;
		FLOAT UMult;
		FLOAT VMult;
		FLOAT UPan;
		FLOAT VPan;
	} TexInfo[MAX_TMUNITS];
	FLOAT RFX2, RFY2;
	GLuint AlphaTextureId;
	GLuint NoTextureId;

	// Static variables.
	static TOpenGLMap<QWORD,FCachedTexture> SharedBindMap;
	static INT NumDevices;
	static INT LockCount;

#ifdef WIN32
	static TArray<HGLRC> AllContexts;
	static HGLRC   hCurrentRC;
#else
	static UBOOL GLLoaded;
#endif

	// GL functions.
	#define GL_EXT(name) UBOOL SUPPORTS##name;
	#define GL_PROC(ext,ret,func,parms) ret (STDCALL *func)parms;
	#include "OpenGLFuncs.h"
	#include "WGLFuncs.h"
	#undef GL_EXT
	#undef GL_PROC

#ifdef RGBA_MAKE
#undef RGBA_MAKE
#endif

	inline int RGBA_MAKE( BYTE r, BYTE g, BYTE b, BYTE a)		// vogel: I hate macros...
	{
		return (a << 24) | (b <<16) | ( g<< 8) | r;
	}

	//
	// Constructors.
	//
	UOpenGLRenderDevice()
	{
		AllocatedTextures = 0;
		ResetContext();
	}

	void StaticConstructor()
	{
		guard(UOpenGLRenderDevice::StaticConstructor);

		// URenderDevice properties.
		//new(GetClass(),TEXT("SupportsLazyTextures"),    RF_Public)UBoolProperty ( CPP_PROPERTY(SupportsLazyTextures    ), TEXT("Options"), CPF_Config );
		//new(GetClass(),TEXT("PrefersDeferredLoad"),     RF_Public)UBoolProperty ( CPP_PROPERTY(PrefersDeferredLoad     ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("DetailTextures"),          RF_Public)UBoolProperty ( CPP_PROPERTY(DetailTextures          ), TEXT("Options"), CPF_Config );

		// Enums.
		UEnum* VSyncModes = new( GetClass(), TEXT("VSyncModes") )UEnum( NULL );
		UEnum* DXT1Modes  = new( GetClass(), TEXT("DXT1Modes")  )UEnum( NULL );

		// Enum members.
		new( VSyncModes->Names )FName( TEXT("Adaptive" )    );
		new( VSyncModes->Names )FName( TEXT("On" )          );
		new( VSyncModes->Names )FName( TEXT("Off")          );
		new( DXT1Modes->Names  )FName( TEXT("DXT1_RGB" )    );
		new( DXT1Modes->Names  )FName( TEXT("DXT1_RGBA" )   );
		new( DXT1Modes->Names  )FName( TEXT("DXT1_Special") );

		// Properties.
		new(GetClass(),TEXT("UsePrecache"),             RF_Public)UBoolProperty ( CPP_PROPERTY(UsePrecache             ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("ShareLists"),              RF_Public)UBoolProperty ( CPP_PROPERTY(ShareLists              ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("UseMultiTexture"),         RF_Public)UBoolProperty ( CPP_PROPERTY(UseMultiTexture         ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("UseTrilinear"),            RF_Public)UBoolProperty ( CPP_PROPERTY(UseTrilinear            ), TEXT("Options"), CPF_Config );	
		new(GetClass(),TEXT("AlwaysMipmap"),            RF_Public)UBoolProperty ( CPP_PROPERTY(AlwaysMipmap            ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("UseVertexSpecular"),       RF_Public)UBoolProperty ( CPP_PROPERTY(UseVertexSpecular       ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("UseDetailAlpha"),          RF_Public)UBoolProperty ( CPP_PROPERTY(UseDetailAlpha          ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("NoFiltering"),             RF_Public)UBoolProperty ( CPP_PROPERTY(NoFiltering             ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("RefreshRate"),             RF_Public)UIntProperty  ( CPP_PROPERTY(RefreshRate             ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("MaxTMUnits"),              RF_Public)UIntProperty  ( CPP_PROPERTY(MaxTMUnits              ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("MaxAnisotropy"),           RF_Public)UIntProperty  ( CPP_PROPERTY(MaxAnisotropy           ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("LODBias"),                 RF_Public)UFloatProperty( CPP_PROPERTY(LODBias                 ), TEXT("Options"), CPF_Config );

		// New properties.
		new(GetClass(),TEXT("UseGammaCorrection"),      RF_Public)UBoolProperty ( CPP_PROPERTY(UseGammaCorrection      ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("UseEditorGammaCorrection"),RF_Public)UBoolProperty ( CPP_PROPERTY(UseEditorGammaCorrection), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("VSync"),                   RF_Public)UByteProperty ( CPP_PROPERTY(VSync                   ), TEXT("Options"), CPF_Config, VSyncModes );
		new(GetClass(),TEXT("DXT1Mode"),                RF_Public)UByteProperty ( CPP_PROPERTY(DXT1Mode                ), TEXT("Options"), CPF_Config, DXT1Modes );

		// Experimental properties.
		//new(GetClass(),TEXT("UseSRGBTextures"),         RF_Public)UBoolProperty ( CPP_PROPERTY(UseSRGBTextures         ), TEXT("Options"), CPF_Config );
		//new(GetClass(),TEXT("UseSRGBLightmaps"),        RF_Public)UBoolProperty ( CPP_PROPERTY(UseSRGBLightmaps        ), TEXT("Options"), CPF_Config );

		// URenderDevice options.
		DescFlags                = RDDESCF_Certified;
		SpanBased                = 0;
		FullscreenOnly           = 0;
		SupportsFogMaps          = 1;
		SupportsDistanceFog      = 0;
		VolumetricLighting       = 1;
		ShinySurfaces            = 1;
		Coronas                  = 1;
		HighDetailActors         = 1;
		SupportsLazyTextures     = 0;
		PrefersDeferredLoad      = 0;
		DetailTextures           = 1;

		// UOpenGLRenderDevice options.
		UsePrecache              = 0;
    ShareLists               = 0;
		UseMultiTexture          = 1;
		UseTrilinear             = 1;
		AlwaysMipmap             = 0;
		UseVertexSpecular        = 1;
		UseDetailAlpha           = 1;
		NoFiltering              = 0;
		RefreshRate              = 0;
		MaxTMUnits               = 4; // Remove me.
		MaxAnisotropy            = 16;
		LODBias                  = 0.f;

		// New UOpenGLRenderDevice options.
		UseGammaCorrection       = 1;
		UseEditorGammaCorrection = 0;
		VSync									   = VSync_Adaptive;
		DXT1Mode                 = DXT1_RGB;

		// Experimental UOpenGLRenderDevice options.
		UseSRGBTextures          = 0;
		UseSRGBLightmaps         = 0;

		unguard;
	}

	void PostEditChange()
	{
		Flush( 0 );
	}

	void ResetContext()
	{
		// Clear OpenGL function pointers.
		#define GL_EXT(name) SUPPORTS##name=0;
		#define GL_PROC(ext,ret,func,parms) func=NULL;
		#include "OpenGLFuncs.h"
		#include "WGLFuncs.h"
		#undef GL_PROC
		#undef GL_EXT
	}

	//
	// Gamma Handling.
	//
	void UpdateGamma()
	{
		guard(UOpenGLRenderDevice::UpdateGamma);

		if ( GIsEditor ? UseEditorGammaCorrection : UseGammaCorrection )
		{
			FLOAT GammaCorrection = Clamp( Viewport->GetOuterUClient()->Brightness, 0.05f, 0.95f );

			// HACK HACK.
			if ( UseSRGBTextures )
				GammaCorrection += 0.1f;
			if ( UseSRGBLightmaps ) 
				GammaCorrection += 0.1f;

#ifdef __LINUX__
			// vogel: FIXME (talk to Sam)
			// SDL_SetGammaRamp( Ramp.red, Ramp.green, Ramp.blue );		
			FLOAT gamma = 0.4 + 2 * GammaCorrection;
			SDL_SetGamma( gamma, gamma, gamma );
#else
			if ( GammaFirstTime )
			{
				//OutputDebugString(TEXT("GetDeviceGammaRamp in SetGamma"));
				GetDeviceGammaRamp( hDC, &OriginalRamp );
				GammaFirstTime = 0;
			}

			FGammaRamp Ramp;
			for( INT i=0; i<256; i++ )
			{
				Ramp.red[i]   = Clamp( appRound(appPow(OriginalRamp.red[i]/65535.f,0.5f/GammaCorrection)*65535.f), 0, 65535 );
				Ramp.green[i] = Clamp( appRound(appPow(OriginalRamp.green[i]/65535.f,0.5f/GammaCorrection)*65535.f), 0, 65535 );
				Ramp.blue[i]  = Clamp( appRound(appPow(OriginalRamp.blue[i]/65535.f,0.5f/GammaCorrection)*65535.f), 0, 65535 );
			}

			//OutputDebugString(TEXT("SetDeviceGammaRamp in SetGamma"));
			SetDeviceGammaRamp( hDC, &Ramp );
#endif
		}
		unguard;
	}

	void RestoreGamma()
	{
		guard(UOpenGLRenderDevice::RestoreGamma);
		if ( !GammaFirstTime )
			SetDeviceGammaRamp( GetDC( GetDesktopWindow() ), &OriginalRamp );
		unguard;
	}

	UBOOL FindExt( const TCHAR* Name )
	{
		guard(UOpenGLRenderDevice::FindExt);
		UBOOL Result = strstr( (char*) glGetString(GL_EXTENSIONS), appToAnsi(Name) ) != NULL;
		if ( Result && !GIsEditor )
			debugf( NAME_Init, TEXT("Device supports: %s"), Name );
		return Result;
		unguard;
	}
	UBOOL FindWglExt( const TCHAR* Name, HDC hDC )
	{
		guard(UOpenGLRenderDevice::FindWglExt);
		UBOOL Result = strstr( (char*) glGetString(GL_EXTENSIONS), appToAnsi(Name) ) != NULL;
		if ( !Result && wglGetExtensionsStringARB )
			Result = strstr( (char*) wglGetExtensionsStringARB(hDC), appToAnsi(Name) ) != NULL;
		if ( Result && !GIsEditor )
			debugf( NAME_Init, TEXT("Device supports: %s"), Name );
		return Result;
		unguard;
	}
	void FindProc( void*& ProcAddress, char* Name, char* SupportName, UBOOL& Supports, UBOOL AllowExt )
	{
		guard(UOpenGLRenderDevice::FindProc);
#ifdef __LINUX__
		ProcAddress = (void*)SDL_GL_GetProcAddress( Name );
#else
		ProcAddress = wglGetProcAddress( Name );
#endif
		if( !ProcAddress )
		{
			if( Supports )
				debugf( TEXT("   Missing function '%s' for '%s' support"), appFromAnsi(Name), appFromAnsi(SupportName) );
			else
				debugf( TEXT("   Missing function '%s'"), appFromAnsi(Name) );
			Supports = 0;
		}
		unguard;
	}
	void FindProcs( UBOOL AllowExt )
	{
		guard(UOpenGLDriver::FindProcs);
		#define GL_EXT(name) if( AllowExt ) SUPPORTS##name = FindExt( TEXT(#name)+1 );
		#define GL_PROC(ext,ret,func,parms) FindProc( *(void**)&func, #func, #ext, SUPPORTS##ext, AllowExt );
		#include "OpenGLFuncs.h"
		#undef GL_EXT
		#undef GL_PROC
		unguard;
	}
	void FindWglProcs( HDC hDC )
	{
		guard(UOpenGLDriver::FindWglProcs);
		#define GL_EXT(name) SUPPORTS##name = FindWglExt( TEXT(#name)+1, hDC );
		#define GL_PROC(ext,ret,func,parms) FindProc( *(void**)&func, #func, #ext, SUPPORTS##ext, 1 );
		#include "WGLFuncs.h"
		#undef GL_EXT
		#undef GL_PROC
		unguard;
	}

	UBOOL FailedInitf( const TCHAR* Fmt, ... )
	{
		TCHAR TempStr[4096];
		GET_VARARGS( TempStr, ARRAY_COUNT(TempStr), Fmt );
		debugf( NAME_Init, TempStr );
		Exit();
		return 0;
	}
	void MakeCurrent()
	{
		guard(UOpenGLRenderDevice::MakeCurrent);
#ifdef WIN32
		check(hRC);
		check(hDC);
		if( hCurrentRC!=hRC )
		{
			verify(wglMakeCurrent(hDC,hRC));
			hCurrentRC = hRC;
		}
#endif
		unguard;
	}

	void Check( const TCHAR* Tag )
	{
		for ( GLenum Error = glGetError(); Error!=GL_NO_ERROR; Error = glGetError() )
		{
			const TCHAR* Msg;
			switch( Error )
			{
				case GL_INVALID_ENUM:                  Msg = TEXT("GL_INVALID_ENUM");                  break;
				case GL_INVALID_VALUE:                 Msg = TEXT("GL_INVALID_VALUE");                 break;
				case GL_INVALID_OPERATION:             Msg = TEXT("GL_INVALID_OPERATION");             break;
				case GL_INVALID_FRAMEBUFFER_OPERATION: Msg = TEXT("GL_INVALID_FRAMEBUFFER_OPERATION"); break;
				case GL_OUT_OF_MEMORY:                 Msg = TEXT("GL_OUT_OF_MEMORY");                 break;
				case GL_STACK_UNDERFLOW:               Msg = TEXT("GL_STACK_UNDERFLOW");               break;
				case GL_STACK_OVERFLOW:                Msg = TEXT("GL_STACK_OVERFLOW");                break;
				default:                               Msg = TEXT("UNKNOWN");                          break;
			};
			//appErrorf( TEXT("OpenGL Error: %s (%s)"), Msg, Tag );
			debugf( TEXT("OpenGL Error: %s (%s)"), Msg, Tag );
		}
	}
	
	void SetNoTexture( INT Multi )
	{
		guard(UOpenGLRenderDevice::SetNoTexture);
		if( TexInfo[Multi].CurrentCacheID != NoTextureId )
		{
			// Set small white texture.
			clock(BindCycles);
			glBindTexture( GL_TEXTURE_2D, NoTextureId );
			TexInfo[Multi].CurrentCacheID = NoTextureId;
			unclock(BindCycles);
		}
		unguard;
	}

	void SetAlphaTexture( INT Multi )
	{
		guard(UOpenGLRenderDevice::SetAlphaTexture);
		if( TexInfo[Multi].CurrentCacheID != AlphaTextureId )
		{
			// Set alpha gradient texture.
			clock(BindCycles);
			glBindTexture( GL_TEXTURE_2D, AlphaTextureId );
			TexInfo[Multi].CurrentCacheID = AlphaTextureId;
			unclock(BindCycles);
		}
		unguard;
	}

	void SetTexture( INT Multi, FTextureInfo& Info, DWORD PolyFlags, FLOAT PanBias )
	{
		guard(UOpenGLRenderDevice::SetTexture);

		// Set panning.
		FTexInfo& Tex = TexInfo[Multi];
		Tex.UPan      = Info.Pan.X + PanBias*Info.UScale;
		Tex.VPan      = Info.Pan.Y + PanBias*Info.VScale;

		// Determine slot. NOTE: Extend this later to DXT1
		INT CacheSlot = ((PolyFlags & PF_Masked) && (Info.Format==TEXF_P8 || (Info.Format==TEXF_DXT1 && DXT1Mode==DXT1_Special))) ? 1 : 0;

		// Find in cache.
		if( Info.CacheID==Tex.CurrentCacheID && !Info.bRealtimeChanged )
			return;

		// Make current.
		clock(BindCycles);
		Tex.CurrentCacheID = Info.CacheID;
		//debugf( TEXT("Tex.CurrentCacheID 0x%08x%08x, Multi %i" ), Tex.CurrentCacheID, Multi );
		FCachedTexture *Bind=BindMap->Find(Info.CacheID), *ExistingBind=Bind;

		if( !Bind )
		{
			// Figure out OpenGL-related scaling for the texture.
			Bind            = &BindMap->Set( Info.CacheID, FCachedTexture() );
			Bind->Ids[0]    = 0;
			Bind->Ids[1]    = 0;
			Bind->BaseMip   = Min(0,Info.NumMips-1);
			Bind->UCopyBits = 0;
			Bind->VCopyBits = 0;
			Bind->UBits     = Info.Mips[Bind->BaseMip]->UBits;
			Bind->VBits     = Info.Mips[Bind->BaseMip]->VBits;			
			if( Bind->UBits-Bind->VBits > MaxLogTextureSize )
			{
				Bind->VCopyBits += (Bind->UBits-Bind->VBits)-MaxLogTextureSize;
				Bind->VBits      = Bind->UBits-MaxLogTextureSize;
			}
			if( Bind->VBits-Bind->UBits > MaxLogTextureSize )
			{
				Bind->UCopyBits += (Bind->VBits-Bind->UBits)-MaxLogTextureSize;
				Bind->UBits      = Bind->VBits-MaxLogTextureSize;
			}
			if( Bind->UBits < MinLogTextureSize )
			{
				Bind->UCopyBits += MinLogTextureSize - Bind->UBits;
				Bind->UBits     += MinLogTextureSize - Bind->UBits;
			}
			if( Bind->VBits < MinLogTextureSize )
			{
				Bind->VCopyBits += MinLogTextureSize - Bind->VBits;
				Bind->VBits     += MinLogTextureSize - Bind->VBits;
			}
			if( Bind->UBits > MaxLogTextureSize )
			{			
				Bind->BaseMip += Bind->UBits-MaxLogTextureSize;
				Bind->VBits   -= Bind->UBits-MaxLogTextureSize;
				Bind->UBits    = MaxLogTextureSize;
				if( Bind->VBits < 0 )
				{
					Bind->VCopyBits = -Bind->VBits;
					Bind->VBits     = 0;
				}
			}
			if( Bind->VBits > MaxLogTextureSize )
			{			
				Bind->BaseMip += Bind->VBits-MaxLogTextureSize;
				Bind->UBits   -= Bind->VBits-MaxLogTextureSize;
				Bind->VBits    = MaxLogTextureSize;
				if( Bind->UBits < 0 )
				{
					Bind->UCopyBits = -Bind->UBits;
					Bind->UBits     = 0;
				}
			}
		}

		if ( Bind->Ids[CacheSlot]==0 )
		{
			glGenTextures( 1, &Bind->Ids[CacheSlot] );
			ExistingBind = 0;
			AllocatedTextures++;
		}
		glBindTexture( GL_TEXTURE_2D, Bind->Ids[CacheSlot] );
		unclock(BindCycles);

		// Account for all the impact on scale normalization.
		Tex.UMult = 1.0 / (Info.UScale * (Info.USize << Bind->UCopyBits));
		Tex.VMult = 1.0 / (Info.VScale * (Info.VSize << Bind->VCopyBits));

		// Upload if needed.
		if( !ExistingBind || Info.bRealtimeChanged )
		{
			// Cleanup texture flags.
			if ( SupportsLazyTextures )
				Info.Load();
			Info.bRealtimeChanged = 0;

			// Generate the palette.
			FColor  LocalPal[256];
			FColor* Palette = Info.Palette ? Info.Palette : LocalPal; // Save fallback for malformed P8.
			if( Info.Format==TEXF_P8 && Info.Palette && (PolyFlags & PF_Masked) )
			{
				// kaufel: could have kept the hack to modify and reset Info.Palette[0], but opted against.
				appMemcpy( LocalPal, Info.Palette, 256*sizeof(FColor) );
				LocalPal[0] = FColor(0,0,0,0);
				Palette = LocalPal;
			}

			// Download the texture.
			clock(ImageCycles);
			FMemMark Mark(GMem);
			BYTE* Compose  = New<BYTE>( GMem, (1<<(Bind->UBits+Bind->VBits))*4 );
			UBOOL SkipMipmaps = Info.NumMips==1 && !AlwaysMipmap;
		
			INT MaxLevel = Min(Bind->UBits,Bind->VBits) - MinLogTextureSize;

			for( INT Level=0; Level<=MaxLevel; Level++ )
			{
				// Convert the mipmap.
				INT MipIndex=Bind->BaseMip+Level, StepBits=0;
				if( MipIndex>=Info.NumMips )
				{
					StepBits = MipIndex - (Info.NumMips - 1);
					MipIndex = Info.NumMips - 1;
				}
				FMipmapBase* Mip            = Info.Mips[MipIndex];
				DWORD        Mask           = Mip->USize-1;			
				BYTE* 	     Src            = (BYTE*)Compose;
				GLuint       SourceFormat   = GL_RGBA;
				GLuint       InternalFormat = UseSRGBTextures ? GL_SRGB8_ALPHA8 : GL_RGBA8;
				GLsizei      CompImageSize  = 0;
				if( Mip->DataPtr )
				{
					if( Info.Format==TEXF_DXT1 )
					{
						InternalFormat = (DXT1Mode==DXT1_RGB || (DXT1Mode==DXT1_Special && !CacheSlot)) ? (UseSRGBTextures ? GL_COMPRESSED_SRGB_S3TC_DXT1_EXT : GL_COMPRESSED_RGB_S3TC_DXT1_EXT) : (UseSRGBTextures ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT : GL_COMPRESSED_RGBA_S3TC_DXT1_EXT);
						CompImageSize  = (1<<Max(0,Bind->UBits-Level)) * (1<<Max(0,Bind->VBits-Level)) / 2;
					}
					else if ( Info.Format==TEXF_DXT3 )
					{
						InternalFormat = UseSRGBTextures ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT : GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
						CompImageSize  = (1<<Max(0,Bind->UBits-Level)) * (1<<Max(0,Bind->VBits-Level));
					}
					else if ( Info.Format==TEXF_DXT5 )
					{
						InternalFormat = UseSRGBTextures ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
						CompImageSize  = (1<<Max(0,Bind->UBits-Level)) * (1<<Max(0,Bind->VBits-Level));
					}
					else if ( Info.Format==TEXF_P8 )
					{
						guard(ConvertP8_RGBA8888);
						InternalFormat = UseSRGBTextures ? GL_SRGB8_ALPHA8 : GL_RGBA8;
						SourceFormat   = GL_RGBA;
						FColor* Ptr    = (FColor*)Compose;
						for( INT i=0; i<(1<<Max(0,Bind->VBits-Level)); i++ )
						{
							BYTE* Base = (BYTE*)Mip->DataPtr + ((i<<StepBits)&(Mip->VSize-1))*Mip->USize;
							for( INT j=0; j<(1<<Max(0,Bind->UBits-Level+StepBits)); j+=(1<<StepBits) )
								*Ptr++ = Palette[Base[j&Mask]];						
						}
						unguard;
					}
					else if ( Info.Format==TEXF_RGBA8 )
					{
						InternalFormat = UseSRGBTextures ? GL_SRGB8_ALPHA8 : GL_RGBA8;
						SourceFormat   = GL_RGBA;
						Src            = (BYTE*)Mip->DataPtr;
					}
					else if ( Info.Format==TEXF_RGB8 )
					{
						InternalFormat = UseSRGBTextures ? GL_SRGB8_ALPHA8 : GL_RGBA8;
						SourceFormat   = GL_RGB;
						Src            = (BYTE*)Mip->DataPtr;
					}
					else if ( Info.Format==TEXF_RGBA7 )
					{
						guard(ConvertBGRA7777_RGBA8888);
						InternalFormat = UseSRGBLightmaps ? GL_SRGB8_ALPHA8 : GL_RGBA8;
						SourceFormat   = GL_BGRA; // Was GL_RGBA;

						clock(Resample7777Cycles);
						FColor* Ptr    = (FColor*)Compose;
						for( INT i=0; i<(1<<Max(0,Bind->VBits-Level)); i++ )
						{
							FColor* Base = (FColor*)Mip->DataPtr + Min<DWORD>((i<<StepBits)&(Mip->VSize-1),Info.VClamp-1)*Mip->USize;
							for( INT j=0; j<(1<<Max(0,Bind->UBits-Level+StepBits)); j+=(1<<StepBits) )
							{
								FColor& Src = Base[Min<DWORD>(j&Mask,Info.UClamp-1)];
								// vogel: optimize it.
								//Ptr->R      = 2 * Src.B;
								//Ptr->G      = 2 * Src.G;
								//Ptr->B      = 2 * Src.R;
								//Ptr->A      = 2 * Src.A; // because of 7777
								//Ptr++;
								// kaufel: get rid of this resampling and/or consider removing the anisotropy stuff.
								*(DWORD*)Ptr++ = GET_COLOR_DWORD(Src)<<1;
							}
						}
						unclock(Resample7777Cycles);
						unguard;
					}
					else
						appErrorf( TEXT("Unknown Texture format %i on %s"), Info.Format, Info.Texture ? Info.Texture->GetPathName() : TEXT("UNKNOWN") );
				}
				if( ExistingBind )
				{
					if ( Info.Format==TEXF_DXT1 || Info.Format==TEXF_DXT3 || Info.Format==TEXF_DXT5 )
					{
						guard(glCompressedTexSubImage2D);
						glCompressedTexSubImage2D
						( 
							GL_TEXTURE_2D, 
							Level, 
							0, 
							0, 
							1<<Max(0,Bind->UBits-Level), 
							1<<Max(0,Bind->VBits-Level), 
							InternalFormat,
							CompImageSize,
							Mip->DataPtr
						);
						unguard;
					}
					else
					{
						guard(glTexSubImage2D);
						glTexSubImage2D
						( 
							GL_TEXTURE_2D, 
							Level, 
							0, 
							0, 
							1<<Max(0,Bind->UBits-Level), 
							1<<Max(0,Bind->VBits-Level), 
							SourceFormat, 
							GL_UNSIGNED_BYTE, 
							Src
						);
						unguard;
					}
				}
				else
				{
					if ( Info.Format==TEXF_DXT1 || Info.Format==TEXF_DXT3 || Info.Format==TEXF_DXT5 )
					{
						guard(glCompressedTexImage2D);					
						glCompressedTexImage2D
						(
							GL_TEXTURE_2D, 
							Level, 
							InternalFormat, 
							1<<Max(0,Bind->UBits-Level), 
							1<<Max(0,Bind->VBits-Level), 
							0,
							CompImageSize,
							Mip->DataPtr
						);
						unguard;					
					}
					else
					{
						guard(glTexImage2D);
						glTexImage2D
						( 
							GL_TEXTURE_2D, 
							Level, 
							InternalFormat, 
							1<<Max(0,Bind->UBits-Level), 
							1<<Max(0,Bind->VBits-Level), 
							0, 
							SourceFormat, 
							GL_UNSIGNED_BYTE, 
							Src
						);
						unguard;
					}	
				}
				if( SkipMipmaps )
					break;
			}

			Mark.Pop();
			unclock(ImageCycles);

			// Set texture state.
			if( NoFiltering )
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			}
			else if( !(PolyFlags & PF_NoSmooth) )
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, SkipMipmaps ? GL_LINEAR : 
					(UseTrilinear ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR_MIPMAP_NEAREST) );
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				if ( MaxAnisotropy )
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, MaxAnisotropy );
			}
			else
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, SkipMipmaps ? GL_NEAREST : GL_NEAREST_MIPMAP_NEAREST );
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			}

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MaxLevel < 0 ? 0 : MaxLevel);

			// Cleanup.
			if( SupportsLazyTextures )
				Info.Unload();
		}

		unguard;
	}
	
	void SetBlend( DWORD PolyFlags )
	{
		guard(UOpenGLRenderDevice::SetBlend);

		if ( BufferedVerts > 0 )
			EndBuffering(); // flushes the vertex array!

		// Adjust PolyFlags according to Unreal's precedence rules.
		// Allows gouraud-polygonal fog only if specular is supported (1-pass fogging).
		if( (PolyFlags & (PF_RenderFog|PF_Translucent|PF_Modulated))!=PF_RenderFog || !UseVertexSpecular )
			PolyFlags &= ~PF_RenderFog;

		if( !(PolyFlags & (PF_Translucent|PF_Modulated)) )
			PolyFlags |= PF_Occlude;
		else if( PolyFlags & PF_Translucent )
			PolyFlags &= ~PF_Masked;

		// Detect changes in the blending modes.
		DWORD Xor = CurrentPolyFlags^PolyFlags;
		if( Xor & (PF_Translucent|PF_Modulated|PF_Invisible|PF_Occlude|PF_Masked|PF_Highlighted|PF_NoSmooth|PF_RenderFog|PF_Memorized) )
		{
			if( Xor & PF_Masked )
			{
				if ( PolyFlags & PF_Masked )
				{
					glEnable( GL_ALPHA_TEST );
				}
				else
				{
					glDisable( GL_ALPHA_TEST );
				}
			}
			if( Xor&(PF_Invisible|PF_Translucent|PF_Modulated|PF_Highlighted) )
			{
				if( !(PolyFlags & (PF_Invisible|PF_Translucent|PF_Modulated|PF_Highlighted)) )
				{
					glDisable( GL_BLEND );			
				}
				else if( PolyFlags & PF_Invisible )
				{
					glEnable( GL_BLEND );
					glBlendFunc( GL_ZERO, GL_ONE );			
				}
				else if( PolyFlags & PF_Translucent )
				{
					glEnable( GL_BLEND );
					glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_COLOR );				
				}
				else if( PolyFlags & PF_Modulated )
				{
					glEnable( GL_BLEND );
					glBlendFunc( GL_DST_COLOR, GL_SRC_COLOR );			
				}
				else if( PolyFlags & PF_Highlighted )
				{
					glEnable( GL_BLEND );
					glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );				
				}
			}
			if( Xor & PF_Invisible )
			{
				if ( PolyFlags & PF_Invisible )
					glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
				else
					glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );		
			}
			if( Xor & PF_Occlude )
			{
				if ( PolyFlags & PF_Occlude )
					glDepthMask( GL_TRUE );
				else
					glDepthMask( GL_FALSE );
			}
			if( Xor & PF_NoSmooth )
			{
				//Direct3DDevice7->SetTextureStageState( 0, D3DTSS_MAGFILTER, (PolyFlags & PF_NoSmooth) ? D3DTFG_POINT : D3DTFG_LINEAR );
				//Direct3DDevice7->SetTextureStageState( 0, D3DTSS_MINFILTER, (PolyFlags & PF_NoSmooth) ? D3DTFN_POINT : D3DTFN_LINEAR );
			}
			if( Xor & PF_RenderFog )
			{
				//Direct3DDevice7->SetRenderState( D3DRENDERSTATE_SPECULARENABLE, (PolyFlags&PF_RenderFog)!=0 );
			}
			if( Xor & PF_Memorized )
			{
				// Switch back and forth from multitexturing.
				//Direct3DDevice7->SetTextureStageState( 1, D3DTSS_COLOROP, (PolyFlags&PF_Memorized) ? D3DTOP_MODULATE   : D3DTOP_DISABLE );
				//Direct3DDevice7->SetTextureStageState( 1, D3DTSS_ALPHAOP, (PolyFlags&PF_Memorized) ? D3DTOP_SELECTARG2 : D3DTOP_DISABLE );
			}
			CurrentPolyFlags = PolyFlags;
		}
		unguard;
	}

	void SetTexEnv( INT Multi, DWORD PolyFlags )
	{
		guard(UOpenGLRenderDevice::SetTexEnv);

		DWORD Xor = /*CurrentEnvFlags[Multi]^*/PolyFlags;

		if ( Xor & PF_Modulated )
		{
			if ( SUPPORTS_GL_EXT_texture_env_combine && (Multi!=0) )
			{
				glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT );

				glTexEnvf( GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE );
				glTexEnvf( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_EXT, GL_MODULATE );

				glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE );
				glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_PREVIOUS_EXT );
				//glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE2_RGB_EXT, GL_TEXTURE );

				glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_EXT, GL_TEXTURE );
				glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_EXT, GL_PREVIOUS_EXT );
				//glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE2_ALPHA_EXT, GL_TEXTURE );

				glTexEnvf( GL_TEXTURE_ENV, GL_RGB_SCALE_EXT, 2.0 );
			}
			else
			{
				glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
			}
		}
		if ( Xor & PF_Highlighted )
		{
			if ( SUPPORTS_GL_NV_texture_env_combine4 )
			{
				// vogel: TODO: verify if NV_texture_env_combine4 is really needed or whether there is a better solution!
				// vogel: UPDATE: ATIX_texture_env_combine3's MODULATE_ADD should work too
				appErrorf( TEXT("IMPLEMENT ME") );
			}
			else
			{
				appErrorf( TEXT("don't call SetTexEnv with PF_Highlighted if NV_texture_env_combine4 is not supported") );
			}
		}		
		if ( Xor & PF_Memorized )	// Abused for detail texture approach
		{
			if ( UseDetailAlpha )
			{	
				glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT );

				glTexEnvf( GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_INTERPOLATE_EXT );	
				glTexEnvf( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_EXT, GL_REPLACE );
	
				glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE );
				glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_PRIMARY_COLOR_EXT );
				glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE2_RGB_EXT, GL_PREVIOUS_EXT );
	
				glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_EXT, GL_TEXTURE );
				//glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_EXT, GL_PRIMARY_COLOR_EXT );
				glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE2_ALPHA_EXT, GL_PREVIOUS_EXT );
				glTexEnvf( GL_TEXTURE_ENV, GL_RGB_SCALE_EXT, 1.0 );
			}
			else
			{
				glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT );

				glTexEnvf( GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_INTERPOLATE_EXT );
				glTexEnvf( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_EXT, GL_REPLACE );

				glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE );
				glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_PRIMARY_COLOR_EXT );
				glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE2_RGB_EXT, GL_PRIMARY_COLOR_EXT );

				glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_EXT, GL_TEXTURE );
				//glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_EXT, GL_PRIMARY_COLOR_EXT );
				glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE2_ALPHA_EXT, GL_PRIMARY_COLOR_EXT );
				glTexEnvf( GL_TEXTURE_ENV, GL_RGB_SCALE_EXT, 1.0 );
                        }       
		}

		CurrentEnvFlags[Multi] = PolyFlags;
		unguard;
	}

#ifdef __LINUX__
	// URenderDevice interface.
	UBOOL Init( UViewport* InViewport, INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen )
	{
		guard(UOpenGLRenderDevice::Init);

		debugf( TEXT("Initializing OpenGLDrv...") );

		// Init global GL.
		if( NumDevices==0 )
		{
			// Bind the library.
			FString OpenGLLibName;
			FString Section = TEXT("OpenGLDrv.OpenGLRenderDevice");
			// Default to libGL.so.1 if not defined
			if (!GConfig->GetString( *Section, TEXT("OpenGLLibName"), OpenGLLibName ))
				OpenGLLibName = TEXT("libGL.so.1");

			if ( !GLLoaded )
			{
				// Only call it once as succeeding calls will 'fail'.
				debugf( TEXT("binding %s"), *OpenGLLibName );
				if ( SDL_GL_LoadLibrary( *OpenGLLibName ) == -1 )
					appErrorf( TEXT(SDL_GetError()) );
  				GLLoaded = true;
			}

			SUPPORTS_GL = 1;
			FindProcs( 0 );
			if( !SUPPORTS_GL )
				return 0;		
		}
		NumDevices++;

		BindMap = (ShareLists || GIsEditor) ? &SharedBindMap : &LocalBindMap;
		Viewport = InViewport;

		// Try to change resolution to desired.
		if( !SetRes( NewX, NewY, NewColorBytes, Fullscreen ) )
			return FailedInitf( LocalizeError("ResFailed") );

		return 1;
		unguard;
	}
	
	void UnsetRes()
	{
		guard(UOpenGLRenderDevice::UnsetRes);
		Flush( 1 );
		unguard;
	}
#else
	UBOOL Init( UViewport* InViewport, INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen )
	{
		guard(UOpenGLRenderDevice::Init);
		debugf( TEXT("Initializing OpenGLDrv...") );

		// Get list of device modes.
		for( INT i=0; ; i++ )
		{
#if UNICODE
			if( !GUnicodeOS )
			{
				DEVMODEA Tmp;
				appMemzero(&Tmp,sizeof(Tmp));
				Tmp.dmSize = sizeof(Tmp);
				if( !EnumDisplaySettingsA(NULL,i,&Tmp) )
					break;
				Modes.AddUniqueItem( FPlane(Tmp.dmPelsWidth,Tmp.dmPelsHeight,Tmp.dmBitsPerPel,Tmp.dmDisplayFrequency) );
			}
			else
#endif
			{
				DEVMODE Tmp;
				appMemzero(&Tmp,sizeof(Tmp));
				Tmp.dmSize = sizeof(Tmp);
				if( !EnumDisplaySettings(NULL,i,&Tmp) )
					break;
				Modes.AddUniqueItem( FPlane(Tmp.dmPelsWidth,Tmp.dmPelsHeight,Tmp.dmBitsPerPel,Tmp.dmDisplayFrequency) );
			}
		}

		// Init global GL.
		if( NumDevices==0 )
		{
			// FIXME: Create fake context and load WGL extensions.
		}
		NumDevices++;

		// Init this GL rendering context.
		BindMap = (ShareLists || GIsEditor) ? &SharedBindMap : &LocalBindMap;
		Viewport = InViewport;
		hWnd = (HWND)InViewport->GetWindow();
		check(hWnd);
		hDC = GetDC( hWnd );
		check(hDC);
#if 0 /* Print all PFD's exposed */
		INT Count = DescribePixelFormat( hDC, 0, 0, NULL );
		for( i=1; i<Count; i++ )
			PrintFormat( hDC, i );
#endif
		if( !SetRes( NewX, NewY, NewColorBytes, Fullscreen ) )
			return FailedInitf( LocalizeError("ResFailed") );

		return 1;
		unguard;
	}
	
	void UnsetRes()
	{
		guard(UOpenGLRenderDevice::UnsetRes);
		check(hRC)
		hCurrentRC = NULL;

		// kaufel: hack fix for unrealed crashing at exit.
		if ( GIsEditor )
			wglMakeCurrent( NULL, NULL );
		else
			verify(wglMakeCurrent( NULL, NULL ));

		verify(wglDeleteContext( hRC ));
		verify(AllContexts.RemoveItem(hRC)==1);
		hRC = NULL;
		if( WasFullscreen )
			TCHAR_CALL_OS(ChangeDisplaySettings(NULL,0),ChangeDisplaySettingsA(NULL,0));
		unguard;
	}

	void PrintFormat( HDC hDC, INT nPixelFormat )
	{
		guard(UOpenGLRenderDevice::PrintFormat);
		TCHAR Flags[1024]=TEXT("");
		PIXELFORMATDESCRIPTOR pfd;
		DescribePixelFormat( hDC, nPixelFormat, sizeof(pfd), &pfd );
		if( pfd.dwFlags & PFD_DRAW_TO_WINDOW )
			appStrcat( Flags, TEXT(" PFD_DRAW_TO_WINDOW") );
		if( pfd.dwFlags & PFD_DRAW_TO_BITMAP )
			appStrcat( Flags, TEXT(" PFD_DRAW_TO_BITMAP") );
		if( pfd.dwFlags & PFD_SUPPORT_GDI )
			appStrcat( Flags, TEXT(" PFD_SUPPORT_GDI") );
		if( pfd.dwFlags & PFD_SUPPORT_OPENGL )
			appStrcat( Flags, TEXT(" PFD_SUPPORT_OPENGL") );
		if( pfd.dwFlags & PFD_GENERIC_ACCELERATED )
			appStrcat( Flags, TEXT(" PFD_GENERIC_ACCELERATED") );
		if( pfd.dwFlags & PFD_GENERIC_FORMAT )
			appStrcat( Flags, TEXT(" PFD_GENERIC_FORMAT") );
		if( pfd.dwFlags & PFD_NEED_PALETTE )
			appStrcat( Flags, TEXT(" PFD_NEED_PALETTE") );
		if( pfd.dwFlags & PFD_NEED_SYSTEM_PALETTE )
			appStrcat( Flags, TEXT(" PFD_NEED_SYSTEM_PALETTE") );
		if( pfd.dwFlags & PFD_DOUBLEBUFFER )
			appStrcat( Flags, TEXT(" PFD_DOUBLEBUFFER") );
		if( pfd.dwFlags & PFD_STEREO )
			appStrcat( Flags, TEXT(" PFD_STEREO") );
		if( pfd.dwFlags & PFD_SWAP_LAYER_BUFFERS )
			appStrcat( Flags, TEXT("PFD_SWAP_LAYER_BUFFERS") );
		debugf( NAME_Init, TEXT("Pixel format %i:"), nPixelFormat );
		debugf( NAME_Init, TEXT("   Flags:%s"), Flags );
		debugf( NAME_Init, TEXT("   Pixel Type: %i"), pfd.iPixelType );
		debugf( NAME_Init, TEXT("   Bits: Color=%i R=%i G=%i B=%i A=%i"), pfd.cColorBits, pfd.cRedBits, pfd.cGreenBits, pfd.cBlueBits, pfd.cAlphaBits );
		debugf( NAME_Init, TEXT("   Bits: Accum=%i Depth=%i Stencil=%i"), pfd.cAccumBits, pfd.cDepthBits, pfd.cStencilBits );
		unguard;
	}
#endif

	UBOOL SetRes( INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen )
	{
		guard(UOpenGLRenderDevice::SetRes);
		
		FString	Section = TEXT("OpenGLDrv.OpenGLRenderDevice");

#ifdef __LINUX__
		UnsetRes();    

		INT MinDepthBits;
		
		// Minimum size of the depth buffer
		if (!GConfig->GetInt( *Section, TEXT("MinDepthBits"), MinDepthBits ))
			MinDepthBits = 16;
		//debugf( TEXT("MinDepthBits = %i"), MinDepthBits );
		// 16 is the bare minimum.
		if ( MinDepthBits < 16 )
			MinDepthBits = 16;

		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, NewColorBytes<=2 ? 5 : 8 );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, NewColorBytes<=2 ? 5 : 8 );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, NewColorBytes<=2 ? 5 : 8 );
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, MinDepthBits );
		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

		// Change window size.
		Viewport->ResizeViewport( Fullscreen ? (BLIT_Fullscreen|BLIT_OpenGL) : (BLIT_HardwarePaint|BLIT_OpenGL), NewX, NewY, NewColorBytes );
#else
		debugf( TEXT("Enter SetRes( %i, %i, %i, %i )"), NewX, NewY, NewColorBytes, Fullscreen );

		// If not fullscreen, and color bytes hasn't changed, do nothing.
		if( hRC && !Fullscreen && !WasFullscreen && NewColorBytes==Viewport->ColorBytes )
		{
			if( !Viewport->ResizeViewport( BLIT_HardwarePaint|BLIT_OpenGL, NewX, NewY, NewColorBytes ) )
				return 0;
			glViewport( 0, 0, NewX, NewY );
			return 1;
		}

		// Exit res.
		if( hRC )
		{
			debugf( TEXT("UnSetRes() -> hRc != NULL") );
			UnsetRes();
		}

		// Change display settings.
		if( Fullscreen )
		{
			INT FindX=NewX, FindY=NewY, BestError = MAXINT;
			for( INT i=0; i<Modes.Num(); i++ )
			{
				if( Modes(i).Z==NewColorBytes*8 )
				{
					INT Error
					=	(Modes(i).X-FindX)*(Modes(i).X-FindX)
					+	(Modes(i).Y-FindY)*(Modes(i).Y-FindY);
					if( Error < BestError )
					{
						NewX      
						= Modes(i).X;
						NewY      = Modes(i).Y;
						BestError = Error;
					}
				}
			}
#if UNICODE
			if( !GUnicodeOS )
			{
				DEVMODEA dm;
				ZeroMemory( &dm, sizeof(dm) );
				dm.dmSize       = sizeof(dm);
				dm.dmPelsWidth  = NewX;
				dm.dmPelsHeight = NewY;
				dm.dmBitsPerPel = NewColorBytes * 8;
				if ( RefreshRate )
				{
					dm.dmDisplayFrequency = RefreshRate;
					dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY | DM_BITSPERPEL;
				}
				else
				{
					dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
				}
				if( ChangeDisplaySettingsA( &dm, CDS_FULLSCREEN )!=DISP_CHANGE_SUCCESSFUL )
				{
					debugf( TEXT("ChangeDisplaySettingsA failed: %ix%i"), NewX, NewY );
					return 0;
				}
			}
			else
#endif
			{
				DEVMODE dm;
				ZeroMemory( &dm, sizeof(dm) );
				dm.dmSize       = sizeof(dm);
				dm.dmPelsWidth  = NewX;
				dm.dmPelsHeight = NewY;
				dm.dmBitsPerPel = NewColorBytes * 8;
				if ( RefreshRate )
				{
					dm.dmDisplayFrequency = RefreshRate;
					dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY | DM_BITSPERPEL;
				}
				else
				{
					dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
				}
				if( ChangeDisplaySettings( &dm, CDS_FULLSCREEN )!=DISP_CHANGE_SUCCESSFUL )
				{
					debugf( TEXT("ChangeDisplaySettings failed: %ix%i"), NewX, NewY );
					return 0;
				}
			}
		}

		// Change window size.
		UBOOL Result = Viewport->ResizeViewport( Fullscreen ? (BLIT_Fullscreen|BLIT_OpenGL) : (BLIT_HardwarePaint|BLIT_OpenGL), NewX, NewY, NewColorBytes );
		if( !Result )
		{
			if( Fullscreen )
				TCHAR_CALL_OS(ChangeDisplaySettings(NULL,0),ChangeDisplaySettingsA(NULL,0));
			return 0;
		}

		// Set res.
		//INT DesiredColorBits   = NewColorBytes<=2 ? 16 : 24; // vogel: changed to saner values 
		INT DesiredColorBits   = NewColorBytes<=2 ? 16 : 32; // kaufel: changed to 32 bit 
		INT DesiredStencilBits = 0;                          // NewColorBytes<=2 ? 0  : 8;
		INT DesiredDepthBits   = NewColorBytes<=2 ? 16 : 24; // NewColorBytes<=2 ? 16 : 32;
		PIXELFORMATDESCRIPTOR pfd =
		{
			sizeof(PIXELFORMATDESCRIPTOR),
			1,
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
			PFD_TYPE_RGBA,
			DesiredColorBits,
			0,0,0,0,0,0,
			0,0,
			0,0,0,0,0,
			DesiredDepthBits,
			DesiredStencilBits,
			0,
			PFD_MAIN_PLANE,
			0,
			0,0,0
		};
		INT nPixelFormat = ChoosePixelFormat( hDC, &pfd );
		Parse( appCmdLine(), TEXT("PIXELFORMAT="), nPixelFormat );
		if ( !GIsEditor )
			debugf( NAME_Init, TEXT("Using pixel format %i"), nPixelFormat );
		check(nPixelFormat);
		verify(SetPixelFormat( hDC, nPixelFormat, &pfd ));
		hRC = wglCreateContext( hDC );
		check(hRC);
		MakeCurrent();
		if( (ShareLists || GIsEditor) && AllContexts.Num() )
			verify(wglShareLists(AllContexts(0),hRC)==1);
		AllContexts.AddItem(hRC);
#endif

		// Reset context.
		ResetContext();

		// Get info and extensions.
		FindProcs( 1 );
		FindWglProcs( hDC );

		guard(SwapControl);
		if ( SUPPORTS_WGL_EXT_swap_control && wglSwapIntervalEXT )
		{

			switch ( VSync )
			{
				case VSync_On:
					wglSwapIntervalEXT( 1 );
					break;
				case VSync_Off:
					wglSwapIntervalEXT( 0 );
					break;

				case VSync_Adaptive:
					if ( !SUPPORTS_WGL_EXT_swap_control_tear )
					{
						debugf( NAME_Init, TEXT("Adative VSync is not supported by device. Falling back to Off.") );
						wglSwapIntervalEXT( 0 );
					}
					else
						wglSwapIntervalEXT( -1 );
					break;
			}
		}
		else
			debugf( NAME_Init, TEXT("Changing VSync is not supported by device.") );
		unguard;

		// Don't kill editor log window.
		if ( !GIsEditor )
		{
			//PrintFormat( hDC, nPixelFormat );
			debugf( NAME_Init, TEXT("GL_VENDOR      : %s"), appFromAnsi((ANSICHAR*)glGetString(GL_VENDOR)) );
			debugf( NAME_Init, TEXT("GL_RENDERER    : %s"), appFromAnsi((ANSICHAR*)glGetString(GL_RENDERER)) );
			debugf( NAME_Init, TEXT("GL_VERSION     : %s"), appFromAnsi((ANSICHAR*)glGetString(GL_VERSION)) );
			SafePrint( NAME_Init, TEXT("GL_EXTENSIONS  : "), (ANSICHAR*)glGetString(GL_EXTENSIONS) );
			if ( wglGetExtensionsStringARB )
				SafePrint( NAME_Init, TEXT("WGL_EXTENSIONS : "), (ANSICHAR*)wglGetExtensionsStringARB(hDC) );
		}

		// Set modelview.
		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity();
		FLOAT Matrix[16] =
		{
			+1, +0, +0, +0,
			+0, -1, +0, +0,
			+0, +0, -1, +0,
			+0, +0, +0, +1,
		};
		glMultMatrixf( Matrix );

		UpdateGamma();

		UseVertexSpecular       = 1;
		AlwaysMipmap            = 0;
		SupportsTC              = 1;
		UseCVA	                = 1;

		SUPPORTS_GL_EXT_texture_env_combine |= SUPPORTS_GL_ARB_texture_env_combine;

		// Validate flags.
		if( !SUPPORTS_GL_ARB_multitexture )
			UseMultiTexture = 0;
		if( !SUPPORTS_GL_EXT_texture_compression_s3tc )
		  appErrorf( TEXT("S3 Texture Compression not supported.") );
		if( !SUPPORTS_GL_EXT_texture_env_combine )
			DetailTextures = 0;
		if( !SUPPORTS_GL_EXT_compiled_vertex_array )
			UseCVA = 0;
		if( !SUPPORTS_GL_EXT_secondary_color )
			UseVertexSpecular = 0;
		if( !SUPPORTS_GL_EXT_texture_filter_anisotropic )
			MaxAnisotropy  = 0;
		if( !SUPPORTS_GL_EXT_texture_env_combine )
			UseDetailAlpha = 0;
		if( !SUPPORTS_GL_EXT_texture_lod_bias )
			LODBias = 0;

		if ( !MaxTMUnits || (MaxTMUnits > MAX_TMUNITS) )
		{
			MaxTMUnits = MAX_TMUNITS;
		}

		if ( UseMultiTexture )
		{
			glGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, &TMUnits );
			debugf( TEXT("%i Texture Mapping Units found"), TMUnits );
			if (TMUnits > MaxTMUnits)
				TMUnits = MaxTMUnits;				
		}
		else
		{
			TMUnits = 1;
			UseDetailAlpha = 0;			
		}
		
		if ( MaxAnisotropy )
		{
			INT tmp;
			glGetIntegerv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &tmp );
			debugf( TEXT("MaxAnisotropy = %i"), tmp ); 
			if ( MaxAnisotropy > tmp )
				MaxAnisotropy = tmp;

			UseTrilinear = true; // Anisotropic filtering doesn't make much sense without trilinear filtering
		}

		BufferActorTris = UseVertexSpecular; // Only buffer when we can use 1 pass fogging

		// Special treatment for texture size stuff.
		//if (!GConfig->GetInt( *Section, TEXT("MinLogTextureSize"), MinLogTextureSize ))
			//MinLogTextureSize=0;
		//if (!GConfig->GetInt( *Section, TEXT("MaxLogTextureSize"), MaxLogTextureSize ))
			//MaxLogTextureSize=8;

		INT MaxTextureSize;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &MaxTextureSize);		
		INT Dummy = -1;
		while (MaxTextureSize > 0)
		{
			MaxTextureSize >>= 1;
			Dummy++;
		}
		
		MaxLogTextureSize = Dummy;
		MinLogTextureSize = 2;

		debugf( TEXT("MinLogTextureSize = %i"), MinLogTextureSize );
		debugf( TEXT("MaxLogTextureSize = %i"), MaxLogTextureSize );

		// Verify hardware defaults.
		check(MinLogTextureSize>=0);
		check(MaxLogTextureSize>=0);
		check(MinLogTextureSize<MaxLogTextureSize);
		check(MinLogTextureSize<=MaxLogTextureSize);

		// Flush textures.
		Flush(1);

		// Bind little white RGBA texture to ID 0.
		FColor Data[8*8];
		for( INT i=0; i<ARRAY_COUNT(Data); i++ )
			Data[i] = FColor(255,255,255,255);
		glGenTextures( 1, &NoTextureId );
		SetNoTexture( 0 );
		for( INT Level=0; 8>>Level; Level++ )
			glTexImage2D( GL_TEXTURE_2D, Level, 4, 8>>Level, 8>>Level, 0, GL_RGBA, GL_UNSIGNED_BYTE, Data );

		// Set permanent state.
		glEnable( GL_DEPTH_TEST );
		glShadeModel( GL_SMOOTH );
		glEnable( GL_TEXTURE_2D );
		glAlphaFunc( GL_GREATER, 0.5 );
		glDepthMask( GL_TRUE );
		glDepthFunc( GL_LEQUAL );
		glBlendFunc( GL_ONE, GL_ZERO );
		glEnable( GL_BLEND );
		glEnable( GL_DITHER );
		glPolygonOffset( -1.0f, -1.0 ); // !
		if ( -KINDA_SMALL_NUMBER>LODBias || LODBias>KINDA_SMALL_NUMBER )
		{
			glTexEnvf( GL_TEXTURE_FILTER_CONTROL_EXT, GL_TEXTURE_LOD_BIAS_EXT, LODBias );
			if ( SUPPORTS_GL_ARB_multitexture )
			{
				for (INT i=1; i<TMUnits; i++)
				{					
					glActiveTextureARB( GL_TEXTURE0_ARB + i);
					glTexEnvf( GL_TEXTURE_FILTER_CONTROL_EXT, GL_TEXTURE_LOD_BIAS_EXT, LODBias );
				}
				glActiveTextureARB( GL_TEXTURE0_ARB );	
			}
		}
		if ( UseDetailAlpha )			// vogel: alpha texture for better detail textures (no vertex alpha)
		{                                                               	
			BYTE AlphaData[256];
			for (INT ac=0; ac<256; ac++)
				AlphaData[ac] = 255 - ac;
			glGenTextures( 1, &AlphaTextureId );
			// vogel: could use 1D texture but opted against (for minor reasons)
			glBindTexture( GL_TEXTURE_2D, AlphaTextureId );
			glTexImage2D( GL_TEXTURE_2D, 0, GL_ALPHA, 256, 1, 0, GL_ALPHA, GL_UNSIGNED_BYTE, AlphaData );
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}

		glEnableClientState( GL_VERTEX_ARRAY );
		glVertexPointer( 3, GL_FLOAT, sizeof(FGLVertex), &VertexArray[0].x );
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		glTexCoordPointer( 2, GL_FLOAT, sizeof(FGLTexCoord), &TexCoordArray[0].TMU[0].u );
		if ( UseMultiTexture )
		{
			glClientActiveTextureARB( GL_TEXTURE1_ARB );
			glTexCoordPointer( 2, GL_FLOAT, sizeof(FGLTexCoord), &TexCoordArray[0].TMU[1].u );
			if ( TMUnits > 2 )
			{	
				glClientActiveTextureARB( GL_TEXTURE2_ARB );
				glTexCoordPointer( 2, GL_FLOAT, sizeof(FGLTexCoord), &TexCoordArray[0].TMU[2].u );
			}
			glClientActiveTextureARB( GL_TEXTURE0_ARB );			
		}
		glColorPointer( 4, GL_FLOAT, sizeof(FGLColor), &ColorArray[0].color[0] );
		if ( UseVertexSpecular )
			glSecondaryColorPointerEXT( 3, GL_FLOAT, sizeof(FGLColor), &ColorArray[0].specular[0] );

		// Init variables.
		BufferedVerts      = 0;
		ColorArrayEnabled  = 0;
		RenderFog          = 0;

		CurrentPolyFlags   = PF_Occlude;
		for (INT TMU=0; TMU<MAX_TMUNITS; TMU++)
			CurrentEnvFlags[TMU] = 0;

		// Remember fullscreenness.
		WasFullscreen = Fullscreen;
		return 1;

		unguard;
	}

	void SafePrint( EName Tag, TCHAR* Prefix, ANSICHAR* Str )
	{
		ANSICHAR Temp[1024];
		INT TempLen;
		FString TempString;
		ANSICHAR* LastSpace;
		ANSICHAR* GLString = Str;
		INT GLStringLen = strlen( GLString );

		while ( GLStringLen > 0 )
		{
			strncpy(Temp,GLString,960);
			Temp[960]    = 0;
			TempLen      = strlen( Temp );
			if ( TempLen < 960 )
			{
				debugf( Tag, TEXT("%s%s"), Prefix, appFromAnsi( Temp ) );
				break;
			}
			else
			{
				LastSpace  = (ANSICHAR*)strrchr( Temp, ' ' );
				*LastSpace = 0;
				debugf( Tag, TEXT("%s%s"), Prefix, appFromAnsi( Temp ) );
				GLString		+= LastSpace-Temp+1;
				GLStringLen -= LastSpace-Temp+1;
			}
		}
	}
	
	void Exit()
	{
		guard(UOpenGLRenderDevice::Exit);
		check(NumDevices>0);

		// Shut down RC.
		Flush( 0 );

#ifdef __LINUX__
		UnsetRes();

		// Shut down global GL.
		if( --NumDevices==0 )
		{
			SharedBindMap.~TOpenGLMap<QWORD,FCachedTexture>();		
		}
#else
		if( hRC )
			UnsetRes();

		// vogel: UClient::Destroy is called before this gets called so hDC is invalid
		//OutputDebugString(TEXT("SetDeviceGammaRamp in Exit"));
		//RestoreGamma();

		// Shut down this GL context. May fail if window was already destroyed.
		if( hDC )
			ReleaseDC(hWnd,hDC);

		// Shut down global GL.
		if( --NumDevices==0 )
		{
			SharedBindMap.~TOpenGLMap<QWORD,FCachedTexture>();
			AllContexts.~TArray<HGLRC>();
			RestoreGamma();
		}
#endif
		unguard;
	}
	
	void ShutdownAfterError()
	{
		guard(UOpenGLRenderDevice::ShutdownAfterError);
		debugf( NAME_Exit, TEXT("UOpenGLRenderDevice::ShutdownAfterError") );
		RestoreGamma();
		//ChangeDisplaySettings( NULL, 0 );
		unguard;
	}

	void Flush( UBOOL AllowPrecache )
	{
		guard(UOpenGLRenderDevice::Flush);
		TArray<GLuint> Binds;
		for( TOpenGLMap<QWORD,FCachedTexture>::TIterator It(*BindMap); It; ++It )
			for ( INT i=0; i<2; i++ )
				if ( It.Value().Ids[i] )
					Binds.AddItem( It.Value().Ids[i] );
		BindMap->Empty();
		// vogel: FIXME: add 0 and AlphaTextureId
		if ( Binds.Num() )
			glDeleteTextures( Binds.Num(), (GLuint*)&Binds(0) );
		debugf( TEXT("Flushed %i bound Textures."), Binds.Num() );
		AllocatedTextures = 0;
		if( AllowPrecache && UsePrecache && !GIsEditor )
			PrecacheOnFlip = 1;
		UpdateGamma();
		unguard;
	}
	void Flush()
	{
		Flush( 1 );
	}
	
	static QSORT_RETURN CDECL CompareRes( const FPlane* A, const FPlane* B )
	{
		return (QSORT_RETURN) ( (A->X-B->X)!=0.0 ? (A->X-B->X) : (A->Y-B->Y) );
	}
	
	UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar )
	{
		guard(UOpenGLRenderDevice::Exec);

		if( ParseCommand(&Cmd,TEXT("DGL")) )
		{
			if( ParseCommand(&Cmd,TEXT("BUFFERTRIS")) )
			{
				BufferActorTris = !BufferActorTris;
				if ( !UseVertexSpecular )
					BufferActorTris = 0;
				Ar.Logf( TEXT("BUFFERTRIS [%i]"), BufferActorTris );
				return 1;
			}			
			else if( ParseCommand(&Cmd,TEXT("CVA")) )
			{
				UseCVA = !UseCVA;
				if ( !SUPPORTS_GL_EXT_compiled_vertex_array )
					UseCVA = 0;				
				Ar.Logf( TEXT("CVA [%i]"), UseCVA );
				return 1;
			}
			else if( ParseCommand(&Cmd,TEXT("TRILINEAR")) )
			{
				UseTrilinear = !UseTrilinear;
				Ar.Logf( TEXT("TRILINEAR [%i]"), UseTrilinear );
				Flush(1);
				return 1;
			}
			else if( ParseCommand(&Cmd,TEXT("MULTITEX")) )
			{
				UseMultiTexture = !UseMultiTexture;				
				if( !SUPPORTS_GL_ARB_multitexture )
					UseMultiTexture = 0;
				if ( UseMultiTexture )
					glGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, &TMUnits );
				else
					TMUnits = 1;
				Ar.Logf( TEXT("MULTITEX [%i]"), UseMultiTexture );
				return 1;
			}
			else if( ParseCommand(&Cmd,TEXT("DETAILTEX")) )
			{
				DetailTextures = !DetailTextures;
				if( !SUPPORTS_GL_EXT_texture_env_combine )
					DetailTextures = 0;
				Ar.Logf( TEXT("DETAILTEX [%i]"), DetailTextures );
				return 1;
			}
			else if( ParseCommand(&Cmd,TEXT("DETAILALPHA")) )
			{
				UseDetailAlpha = !UseDetailAlpha;
				if( !SUPPORTS_GL_EXT_texture_env_combine )
					UseDetailAlpha = 0;
				Ar.Logf( TEXT("DETAILALPHA [%i]"), UseDetailAlpha );
				return 1;
			}
			else if( ParseCommand(&Cmd,TEXT("BUILD")) )
			{
				Ar.Logf( TEXT("OpenGL renderer built: %s"), appFromAnsi(__DATE__ " " __TIME__) );
				return 1;
			}
			else if( ParseCommand(&Cmd,TEXT("BINDSTATS")) )
			{
				Ar.Logf( TEXT("Dumping BindMap:") );
				BindMap->Dump( Ar );
				return 1;
			}
			return 0;
		}
		else if( ParseCommand(&Cmd,TEXT("GetRes")) )
		{
#ifdef __LINUX__
			// Changing Resolutions:
			// Entries in the resolution box in the console is
			// apparently controled building a string of relevant 
			// resolutions and sending it to the engine via Ar.Log()

			// Here I am querying SDL_ListModes for available resolutions,
			// and populating the dropbox with its output.
			FString Str = "";
			SDL_Rect **modes;
			INT i,j;

 			// Available fullscreen video modes
			modes = SDL_ListModes(NULL, SDL_FULLSCREEN);
                        
			if ( modes == (SDL_Rect **)0 ) 
			{
				debugf( NAME_Init, TEXT("No available fullscreen video modes") );
			} 
			else if ( modes == (SDL_Rect **)-1 ) 
			{
				debugf( NAME_Init, TEXT("No special fullscreen video modes") );
			} 
			else 
			{
				// count the number of available modes
				for ( i=0,j=0; modes[i]; ++i )	
					++j;
						
				// Load the string with resolutions from smallest to 
				// largest. SDL_ListModes() provides them from lg
				// to sm...
				for ( i=(j-1); i >= 0; --i )
					Str += FString::Printf( TEXT("%ix%i "), modes[i]->w, modes[i]->h);
			}
			
			// Send the resolution string to the engine.	
			Ar.Log( *Str.LeftChop(1) );
			return 1;
#else
			TArray<FPlane> Relevant;
			INT i;
			for( i=0; i<Modes.Num(); i++ )
				if( Modes(i).Z==Viewport->ColorBytes*8 )
					if
					(	(Modes(i).X!=320 || Modes(i).Y!=200)
					&&	(Modes(i).X!=640 || Modes(i).Y!=400) )
					Relevant.AddUniqueItem(FPlane(Modes(i).X,Modes(i).Y,0,0));
			appQsort( &Relevant(0), Relevant.Num(), sizeof(FPlane), (QSORT_COMPARE)CompareRes );
			FString Str;
			for( i=0; i<Relevant.Num(); i++ )
				Str += FString::Printf( TEXT("%ix%i "), (INT)Relevant(i).X, (INT)Relevant(i).Y );
			Ar.Log( *Str.LeftChop(1) );
			return 1;
#endif
		}
		return 0;
		unguard;
	}

	void Lock( FPlane InFlashScale, FPlane InFlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* InHitData, INT* InHitSize )
	{
		guard(UOpenGLRenderDevice::Lock);
		check(LockCount==0);
		BindCycles = ImageCycles = ComplexCycles = GouraudCycles = TileCycles = Resample7777Cycles = 0;
		++LockCount;

		// Make this context current.
		MakeCurrent();

		// Clear the Z buffer if needed.
		glClearColor( ScreenClear.X, ScreenClear.Y, ScreenClear.Z, ScreenClear.W );
		glClearDepth( 1.0 );
		glDepthRange( 0.0, 1.0 );
		SetBlend( PF_Occlude );
		glClear( GL_DEPTH_BUFFER_BIT|((RenderLockFlags&LOCKR_ClearScreen)?GL_COLOR_BUFFER_BIT:0) );

		glDepthFunc( GL_LEQUAL );

		// Remember stuff.
		FlashScale = InFlashScale;
		FlashFog   = InFlashFog;
		HitData    = InHitData;
		HitSize    = InHitSize;
		if( HitData )
		{
			*HitSize = 0;
			if( !GLHitData.Num() )
				GLHitData.Add( 16384 );
			glSelectBuffer( GLHitData.Num(), (GLuint*)&GLHitData(0) );
			glRenderMode( GL_SELECT );
			glInitNames();
		}
		unguard;
	}
	
	void SetSceneNode( FSceneNode* Frame )
	{
		guard(UOpenGLRenderDevice::SetSceneNode);

		EndBuffering();		// Flush vertex array before changing the projection matrix!

		// Precompute stuff.
		Aspect      = Frame->FY/Frame->FX;
		RProjZ      = appTan( Viewport->Actor->FovAngle * PI/360.0 );
		RFX2        = 2.0*RProjZ/Frame->FX;
		RFY2        = 2.0*RProjZ*Aspect/Frame->FY;

		// Set viewport.
		glViewport( Frame->XB, Viewport->SizeY-Frame->Y-Frame->YB, Frame->X, Frame->Y );

		// Set projection.
		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
		if ( Frame->Viewport->IsOrtho() )
		{
			if ( GIsEditor )
				glOrtho( -RProjZ, +RProjZ, -Aspect*RProjZ, +Aspect*RProjZ , 1.0, 32768.0 );
			else
				glOrtho( -RProjZ/2.0, +RProjZ/2.0, -Aspect*RProjZ/2.0, +Aspect*RProjZ/2.0, 0.5, 32768.0 );
		}
		else
			glFrustum( -RProjZ/2.0, +RProjZ/2.0, -Aspect*RProjZ/2.0, +Aspect*RProjZ/2.0, 0.5, 32768.0 );

		// Set clip planes.
		if( HitData )
		{
			FVector N[4];
			N[0] = (FVector((Viewport->HitX-Frame->FX2)*Frame->RProj.Z,0,1)^FVector(0,-1,0)).SafeNormal();
			N[1] = (FVector((Viewport->HitX+Viewport->HitXL-Frame->FX2)*Frame->RProj.Z,0,1)^FVector(0,+1,0)).SafeNormal();
			N[2] = (FVector(0,(Viewport->HitY-Frame->FY2)*Frame->RProj.Z,1)^FVector(+1,0,0)).SafeNormal();
			N[3] = (FVector(0,(Viewport->HitY+Viewport->HitYL-Frame->FY2)*Frame->RProj.Z,1)^FVector(-1,0,0)).SafeNormal();
			for( INT i=0; i<4; i++ )
			{
				double D0[4]={N[i].X,N[i].Y,N[i].Z,0};
				glClipPlane( (GLenum) (GL_CLIP_PLANE0+i), D0 );
				glEnable( (GLenum) (GL_CLIP_PLANE0+i) );
			}
		}
		unguard;
	}

	void Unlock( UBOOL Blit )
	{
		guard(UOpenGLRenderDevice::Unlock);
		EndBuffering();

		// Unlock and render.
		check(LockCount==1);
		//glFlush();
		if( Blit )
		{
			Check( TEXT("please report this bug") );
#ifdef __LINUX__
			SDL_GL_SwapBuffers();
#else
			verify(SwapBuffers( hDC ));
#endif
		}
		--LockCount;

		// Hits.
		if( HitData )
		{
			INT Records = glRenderMode( GL_RENDER );
			INT* Ptr = &GLHitData(0);
			DWORD BestDepth = MAXDWORD;
			for( INT i=0; i<Records; i++ )
			{
				INT   NameCount = *Ptr++;
				DWORD MinDepth  = *Ptr++;
				DWORD MaxDepth  = *Ptr++;
				if( MinDepth<=BestDepth )
				{
					BestDepth = MinDepth;
					*HitSize = 0;
					INT i;
					for( i=0; i<NameCount; )
					{
						INT Count = Ptr[i++];
						for( INT j=0; j<Count; j+=4 )
							*(INT*)(HitData+*HitSize+j) = Ptr[i++];
						*HitSize += Count;
					}
					check(i==NameCount);
				}
				Ptr += NameCount;
				(void)MaxDepth;
			}
			for( i=0; i<4; i++ )
				glDisable( GL_CLIP_PLANE0+i );
		}

		unguard;
	}

	void RenderPasses()
	{
		guard(UOpenGLRenderDevice::RenderPasses);
		if ( PassCount == 0 )
			return;

		FPlane Color( 1.0f, 1.0f, 1.0f, 1.0f );
		INT i;
	             		
		SetBlend( MultiPass.TMU[0].PolyFlags );

		for (i=0; i<PassCount; i++)
		{
			glActiveTextureARB( GL_TEXTURE0_ARB + i );
			glClientActiveTextureARB( GL_TEXTURE0_ARB + i );
			glEnable( GL_TEXTURE_2D );
			glEnableClientState(GL_TEXTURE_COORD_ARRAY );

			if ( i != 0 )
			 	SetTexEnv( i, MultiPass.TMU[i].PolyFlags );

			SetTexture( i, *MultiPass.TMU[i].Info, MultiPass.TMU[i].PolyFlags, MultiPass.TMU[i].PanBias );
		}			

		glColor4fv( &Color.X );

		INT Index = 0;
		for( FSavedPoly* Poly=MultiPass.Poly; Poly; Poly=Poly->Next )
		{
			for (i=0; i<Poly->NumPts; i++ )
			{
				FLOAT& U = MapDotArray[Index].u;
				FLOAT& V = MapDotArray[Index].v;

				for (INT t=0; t<PassCount; t++)
				{
					TexCoordArray[Index].TMU[t].u = (U-TexInfo[t].UPan)*TexInfo[t].UMult;
					TexCoordArray[Index].TMU[t].v = (V-TexInfo[t].VPan)*TexInfo[t].VMult;
				}	
				Index++;
			}			         
		}

		INT Start = 0;
		for( Poly=MultiPass.Poly; Poly; Poly=Poly->Next )
		{
			glDrawArrays( GL_TRIANGLE_FAN, Start, Poly->NumPts );
			Start += Poly->NumPts;
		}

		for (i=1; i<PassCount; i++)
		{
			glActiveTextureARB( GL_TEXTURE0_ARB + i );
			glClientActiveTextureARB( GL_TEXTURE0_ARB + i );
			glDisable( GL_TEXTURE_2D );
			glDisableClientState(GL_TEXTURE_COORD_ARRAY );
		}
		glActiveTextureARB( GL_TEXTURE0_ARB );
		glClientActiveTextureARB( GL_TEXTURE0_ARB );

		//printf("PassCount == %i\n", PassCount );
		PassCount = 0;
		
		unguard;
	}

	inline void AddRenderPass( FTextureInfo* Info, DWORD PolyFlags, FLOAT PanBias, UBOOL& ForceSingle, UBOOL Masked )
	{
		MultiPass.TMU[PassCount].Multi     = PassCount;
		MultiPass.TMU[PassCount].Info      = Info;
		MultiPass.TMU[PassCount].PolyFlags = PolyFlags;
		MultiPass.TMU[PassCount].PanBias   = PanBias;	      
		
		PassCount++;
		if ( PassCount >= TMUnits || ForceSingle )
		{			
			if( ForceSingle && Masked )
				glDepthFunc( GL_EQUAL );
			RenderPasses();
			ForceSingle = true;
		}
	}

	inline void SetPolygon( FSavedPoly* Poly )
	{
		MultiPass.Poly = Poly;
	}

	void DrawComplexSurface( FSceneNode* Frame, FSurfaceInfo& Surface, FSurfaceFacet& Facet )
	{
		guard(UOpenGLRenderDevice::DrawComplexSurface);
		check(Surface.Texture);
		clock(ComplexCycles);
		FLOAT UDot = Facet.MapCoords.XAxis | Facet.MapCoords.Origin;
		FLOAT VDot = Facet.MapCoords.YAxis | Facet.MapCoords.Origin;
		INT Index  = 0;
		UBOOL ForceSingle = false;
		UBOOL Masked = Surface.PolyFlags & PF_Masked;
		UBOOL FlatShaded = (Surface.PolyFlags & PF_FlatShaded) && GIsEditor;

		EndBuffering();		// vogel: might have still been locked (can happen!)

		// Vanilla rendering.
		if ( !FlatShaded )
		{
			// Buffer "static" geometry.
			for( FSavedPoly* Poly=Facet.Polys; Poly; Poly=Poly->Next )
			{
				for( INT i=0; i<Poly->NumPts; i++ )
				{
					MapDotArray[Index].u = (Facet.MapCoords.XAxis | Poly->Pts[i]->Point) - UDot;
					MapDotArray[Index].v = (Facet.MapCoords.YAxis | Poly->Pts[i]->Point) - VDot;
					VertexArray[Index].x  = Poly->Pts[i]->Point.X;
					VertexArray[Index].y  = Poly->Pts[i]->Point.Y;
					VertexArray[Index].z  = Poly->Pts[i]->Point.Z;
					Index++;
				}
			}
			if ( UseCVA )
			{
				glDisableClientState( GL_TEXTURE_COORD_ARRAY );	
				glLockArraysEXT( 0, Index );
				glEnableClientState( GL_TEXTURE_COORD_ARRAY );
			}

			// Mutually exclusive effects.
			if( Surface.DetailTexture && Surface.FogMap )
				Surface.DetailTexture = NULL;
					
			SetPolygon( Facet.Polys );
			PassCount = 0;

			AddRenderPass( Surface.Texture, Surface.PolyFlags, 0, ForceSingle, Masked );
       
			if ( Surface.MacroTexture )
				AddRenderPass( Surface.MacroTexture, PF_Modulated, -0.5, ForceSingle, Masked );

			if ( Surface.LightMap )
				AddRenderPass( Surface.LightMap, PF_Modulated, -0.5, ForceSingle, Masked );
			
			// vogel: have to implement it in SetTexEnv
			if ( 1 || !SUPPORTS_GL_NV_texture_env_combine4 )
				RenderPasses();

			if ( Surface.FogMap )
				AddRenderPass( Surface.FogMap, PF_Highlighted, -0.5, ForceSingle, Masked );

			RenderPasses();

			if ( UseCVA )
			{
				glDisableClientState( GL_TEXTURE_COORD_ARRAY );	
				glUnlockArraysEXT();
				glEnableClientState( GL_TEXTURE_COORD_ARRAY );
			}

			if( Masked )
				glDepthFunc( GL_EQUAL );

			// Draw detail texture overlaid, in a separate pass.
			if( Surface.DetailTexture && DetailTextures )
			{
				FLOAT NearZ  = 380.0f;
				FLOAT RNearZ = 1.0f / NearZ;
				UBOOL IsDetailing = false;	
				Index = 0;

				for( FSavedPoly* Poly=Facet.Polys; Poly; Poly=Poly->Next )
				{
					INT Start = Index;
					UBOOL IsNear[32], CountNear=0;
					for( INT i=0; i<Poly->NumPts; i++ )
					{
						IsNear[i] = Poly->Pts[i]->Point.Z < NearZ ? 1 : 0;
						CountNear += IsNear[i];
					}
					if( CountNear )
					{
						if ( !IsDetailing )
						{
							SetBlend( PF_Modulated );						
							//glEnable( GL_POLYGON_OFFSET_FILL );
							if ( UseDetailAlpha )	
							{
								glColor4f( 0.5f, 0.5f, 0.5f, 1.0 );
								SetAlphaTexture( 0 );
								glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

								glActiveTextureARB( GL_TEXTURE1_ARB );
								glClientActiveTextureARB( GL_TEXTURE1_ARB );
								glEnable( GL_TEXTURE_2D );						       
								glEnableClientState(GL_TEXTURE_COORD_ARRAY );

								SetTexture( 1, *Surface.DetailTexture, PF_Modulated, 0 );
								SetTexEnv( 1, PF_Memorized );
	  						}
							else
							{
								SetTexture( 0, *Surface.DetailTexture, PF_Modulated, 0 );
								SetTexEnv( 0, PF_Memorized );
								glEnableClientState( GL_COLOR_ARRAY );
							}
							IsDetailing = true;
						}
						for( INT i=0,j=Poly->NumPts-1; i<Poly->NumPts; j=i++ )
						{
							FLOAT U = Facet.MapCoords.XAxis | Poly->Pts[i]->Point;
							FLOAT V = Facet.MapCoords.YAxis | Poly->Pts[i]->Point;
							{				     	
								if ( UseDetailAlpha )	
								{	
									TexCoordArray[Index].TMU[0].u = Poly->Pts[i]->Point.Z * RNearZ;
									TexCoordArray[Index].TMU[0].v = Poly->Pts[i]->Point.Z * RNearZ;
									TexCoordArray[Index].TMU[1].u = (U-UDot-TexInfo[1].UPan)*TexInfo[1].UMult;
									TexCoordArray[Index].TMU[1].v = (V-VDot-TexInfo[1].VPan)*TexInfo[1].VMult;
								}
								else
								{
									TexCoordArray[Index].TMU[0].u = (U-UDot-TexInfo[0].UPan)*TexInfo[0].UMult;
									TexCoordArray[Index].TMU[0].v = (V-VDot-TexInfo[0].VPan)*TexInfo[0].VMult;
									// DWORD A = Min<DWORD>( appRound(100.f * (NearZ / Poly->Pts[i]->Point.Z - 1.f)), 255 );
									ColorArray[Index].color[0] = 0.5f;
									ColorArray[Index].color[1] = 0.5f;
									ColorArray[Index].color[2] = 0.5f;
									ColorArray[Index].color[3] = 0.5f * (1 - Clamp( Poly->Pts[i]->Point.Z, 0.0f, NearZ ) / NearZ );
								}
								VertexArray[Index].x     = Poly->Pts[i]->Point.X;
								VertexArray[Index].y     = Poly->Pts[i]->Point.Y;
								VertexArray[Index].z     = Poly->Pts[i]->Point.Z;
								Index++;
							}
						}
						glDrawArrays( GL_TRIANGLE_FAN, Start, Index - Start );
					}
				}
				if ( IsDetailing )
				{
					if ( UseDetailAlpha )
					{
						SetTexEnv( 1, PF_Modulated );
						glDisable( GL_TEXTURE_2D );
						glDisableClientState(GL_TEXTURE_COORD_ARRAY );
						glActiveTextureARB( GL_TEXTURE0_ARB );
						glClientActiveTextureARB( GL_TEXTURE0_ARB );
						glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
					}
					else
					{
						glDisableClientState(GL_COLOR_ARRAY );
					}
					SetTexEnv( 0, PF_Modulated );
					//glDisable(GL_POLYGON_OFFSET_FILL);
				}
			}
		}
		// UnrealEd flat shading.
		else
		{
			SetNoTexture( 0 );
			SetBlend( PF_FlatShaded );
			glColor3ubv( (BYTE*)&Surface.FlatColor );
			for( FSavedPoly* Poly=Facet.Polys; Poly; Poly=Poly->Next )
			{
				glBegin( GL_TRIANGLE_FAN );
				for( INT i=0; i<Poly->NumPts; i++ )
					glVertex3fv( &Poly->Pts[i]->Point.X );
				glEnd();
			}
		}

		// UnrealEd selection.
		if( (Surface.PolyFlags & PF_Selected) && GIsEditor )
		{
			SetNoTexture( 0 );
			SetBlend( PF_Highlighted );
			glColor4f( 0, 0, 0.5, 0.5 );
			for( FSavedPoly* Poly=Facet.Polys; Poly; Poly=Poly->Next )
			{
				glBegin( GL_TRIANGLE_FAN );
				for( INT i=0; i<Poly->NumPts; i++ )
					glVertex3fv( &Poly->Pts[i]->Point.X );
				glEnd();
			}
		}

		if( Masked && !FlatShaded )
			glDepthFunc( GL_LEQUAL );

		unclock(ComplexCycles);
		unguard;
	}
	
	void DrawGouraudPolygonOld( FSceneNode* Frame, FTextureInfo& Info, FTransTexture** Pts, INT NumPts, DWORD PolyFlags, FSpanBuffer* Span )
	{
		guard(UOpenGLRenderDevice::DrawGouraudPolygonOld);      
		clock(GouraudCycles);
	    
		INT Index     = 0;
		UBOOL Enabled = false;

		SetBlend( PolyFlags );
		SetTexture( 0, Info, PolyFlags, 0 );
		if( PolyFlags & PF_Modulated )
			glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
		else
		{
			glEnableClientState( GL_COLOR_ARRAY );
			Enabled = true;
		}
		
		for( INT i=0; i<NumPts; i++ )
		{
			FTransTexture* P = Pts[i];
			TexCoordArray[Index].TMU[0].u = P->U*TexInfo[0].UMult;
			TexCoordArray[Index].TMU[0].v = P->V*TexInfo[0].VMult;
			if( Enabled )
			{
				ColorArray[Index].color[0] = P->Light.X;
				ColorArray[Index].color[1] = P->Light.Y;
				ColorArray[Index].color[2] = P->Light.Z;
				ColorArray[Index].color[3] = 1.0f;
			}
			VertexArray[Index].x  = P->Point.X;
			VertexArray[Index].y  = P->Point.Y;
			VertexArray[Index].z  = P->Point.Z;
			Index++;
		}

		glDrawArrays( GL_TRIANGLE_FAN, 0, Index );

		if( (PolyFlags & (PF_RenderFog|PF_Translucent|PF_Modulated))==PF_RenderFog )
		{	
			Index = 0;
			if ( !Enabled )
			{
				glEnableClientState( GL_COLOR_ARRAY );
				Enabled = true;	
			}

			SetNoTexture( 0 );
			SetBlend( PF_Highlighted );

			for( INT i=0; i<NumPts; i++ )
			{
				FTransTexture* P = Pts[i];
				ColorArray[Index].color[0] = P->Fog.X;
				ColorArray[Index].color[1] = P->Fog.Y;
				ColorArray[Index].color[2] = P->Fog.Z;
				ColorArray[Index].color[3] = P->Fog.W;	
				Index++;
			}

			glDrawArrays( GL_TRIANGLE_FAN, 0, Index );
		}
		
		if( Enabled )
			glDisableClientState( GL_COLOR_ARRAY );

		unclock(GouraudCycles);
		unguard;
	}

	void EndBuffering()
	{
		if ( BufferedVerts > 0 )
		{
			if ( RenderFog )
			{
				glEnableClientState( GL_SECONDARY_COLOR_ARRAY_EXT );
				glEnable( GL_COLOR_SUM_EXT );	
			}

			// Actually render the triangles.
			glDrawArrays( GL_TRIANGLES, 0, BufferedVerts );

			if ( ColorArrayEnabled )
				glDisableClientState( GL_COLOR_ARRAY );
			if ( RenderFog )
			{
				glDisableClientState( GL_SECONDARY_COLOR_ARRAY_EXT );
				glDisable( GL_COLOR_SUM_EXT );
			}
			BufferedVerts = 0;
		}
	}

	void DrawGouraudPolygon( FSceneNode* Frame, FTextureInfo& Info, FTransTexture** Pts, INT NumPts, DWORD PolyFlags, FSpanBuffer* Span )
	{
		guard(UOpenGLRenderDevice::DrawGouraudPolygon);      
		if ( !BufferActorTris || NumPts != 3)
		{
			DrawGouraudPolygonOld( Frame, Info, Pts, NumPts, PolyFlags, Span );
			return;
		}
		clock(GouraudCycles);

		static DWORD lastPolyFlags = 0; // vogel: can't use CurrentFlags as SetBlend modifies it (especially PF_RenderFog)

		if ( ! ((lastPolyFlags==PolyFlags) && (TexInfo[0].CurrentCacheID == Info.CacheID) &&
			(BufferedVerts+NumPts < VERTEX_ARRAY_SIZE-1) && (BufferedVerts>0) ) )
		// flush drawing and set the state!
		{
			SetBlend( PolyFlags ); // SetBlend will call EndBuffering() to flush the vertex array
			SetTexture( 0, Info, PolyFlags, 0 );
			if( PolyFlags & PF_Modulated )
			{
				glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
				ColorArrayEnabled = false;
			}
			else
			{
				glEnableClientState( GL_COLOR_ARRAY );
				ColorArrayEnabled = true;
			}
			if( ((PolyFlags & (PF_RenderFog|PF_Translucent|PF_Modulated))==PF_RenderFog) && UseVertexSpecular )
				RenderFog = true;
			else	
				RenderFog = false;

			lastPolyFlags = PolyFlags;
		}

		for( INT i=0; i<NumPts; i++ )
		{
			FTransTexture* P = Pts[i];
			TexCoordArray[BufferedVerts].TMU[0].u = P->U*TexInfo[0].UMult;
			TexCoordArray[BufferedVerts].TMU[0].v = P->V*TexInfo[0].VMult;

			if ( RenderFog  && ColorArrayEnabled )
			{
				ColorArray[BufferedVerts].color[0] = P->Light.X * (1 - P->Fog.W);
				ColorArray[BufferedVerts].color[1] = P->Light.Y * (1 - P->Fog.W);
				ColorArray[BufferedVerts].color[2] = P->Light.Z * (1 - P->Fog.W);
				ColorArray[BufferedVerts].color[3] = 1.0f;
				
				ColorArray[BufferedVerts].specular[0] = P->Fog.X;
				ColorArray[BufferedVerts].specular[1] = P->Fog.Y;
				ColorArray[BufferedVerts].specular[2] = P->Fog.Z;
				ColorArray[BufferedVerts].specular[3] = 0.0f;
			}
			else if ( RenderFog )
			{
				ColorArray[BufferedVerts].color[0] = 1.0f - P->Fog.W;
				ColorArray[BufferedVerts].color[1] = 1.0f - P->Fog.W;
				ColorArray[BufferedVerts].color[2] = 1.0f - P->Fog.W;
				ColorArray[BufferedVerts].color[3] = 1.0f;
				
				ColorArray[BufferedVerts].specular[0] = P->Fog.X;
				ColorArray[BufferedVerts].specular[1] = P->Fog.Y;
				ColorArray[BufferedVerts].specular[2] = P->Fog.Z;
				ColorArray[BufferedVerts].specular[3] = 0.0f;

				ColorArrayEnabled = true;
				glEnableClientState( GL_COLOR_ARRAY );
			}
			else if ( ColorArrayEnabled )
			{
				ColorArray[BufferedVerts].color[0] = P->Light.X;
				ColorArray[BufferedVerts].color[1] = P->Light.Y;
				ColorArray[BufferedVerts].color[2] = P->Light.Z;
				ColorArray[BufferedVerts].color[3] = 1.0f;
			}

			VertexArray[BufferedVerts].x  = P->Point.X;
			VertexArray[BufferedVerts].y  = P->Point.Y;
			VertexArray[BufferedVerts].z  = P->Point.Z;
			BufferedVerts++;
		}
		unclock(GouraudCycles);
		unguard;
	}

	void DrawTile( FSceneNode* Frame, FTextureInfo& Info, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, class FSpanBuffer* Span, FLOAT Z, FPlane Color, FPlane Fog, DWORD PolyFlags )
	{
		guard(UOpenGLRenderDevice::DrawTile);
		clock(TileCycles);
		if( Info.Palette && Info.Palette[128].A!=255 && !(PolyFlags&PF_Translucent) )
			PolyFlags |= PF_Highlighted;
		SetBlend( PolyFlags );
		SetTexture( 0, Info, PolyFlags, 0 );
		if ( PolyFlags & PF_Modulated )
			glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
		else
		{
			Color.W  = 1.0f;
			glColor4fv( &Color.X );
		}

		if( Frame->Viewport->IsOrtho() )
		{
			glBegin( GL_TRIANGLE_FAN );
			glTexCoord2f( (U   )*TexInfo[0].UMult, (V   )*TexInfo[0].VMult ); glVertex3f( RFX2*(X   -Frame->FX2), RFY2*(Y   -Frame->FY2), 1.0 );
			glTexCoord2f( (U+UL)*TexInfo[0].UMult, (V   )*TexInfo[0].VMult ); glVertex3f( RFX2*(X+XL-Frame->FX2), RFY2*(Y   -Frame->FY2), 1.0 );
			glTexCoord2f( (U+UL)*TexInfo[0].UMult, (V+VL)*TexInfo[0].VMult ); glVertex3f( RFX2*(X+XL-Frame->FX2), RFY2*(Y+YL-Frame->FY2), 1.0 );
			glTexCoord2f( (U   )*TexInfo[0].UMult, (V+VL)*TexInfo[0].VMult ); glVertex3f( RFX2*(X   -Frame->FX2), RFY2*(Y+YL-Frame->FY2), 1.0 );
			glEnd();
		}
		else
		{
			glBegin( GL_TRIANGLE_FAN );
			glTexCoord2f( (U   )*TexInfo[0].UMult, (V   )*TexInfo[0].VMult ); glVertex3f( RFX2*Z*(X   -Frame->FX2), RFY2*Z*(Y   -Frame->FY2), Z );
			glTexCoord2f( (U+UL)*TexInfo[0].UMult, (V   )*TexInfo[0].VMult ); glVertex3f( RFX2*Z*(X+XL-Frame->FX2), RFY2*Z*(Y   -Frame->FY2), Z );
			glTexCoord2f( (U+UL)*TexInfo[0].UMult, (V+VL)*TexInfo[0].VMult ); glVertex3f( RFX2*Z*(X+XL-Frame->FX2), RFY2*Z*(Y+YL-Frame->FY2), Z );
			glTexCoord2f( (U   )*TexInfo[0].UMult, (V+VL)*TexInfo[0].VMult ); glVertex3f( RFX2*Z*(X   -Frame->FX2), RFY2*Z*(Y+YL-Frame->FY2), Z );
			glEnd();
		}
		unclock(TileCycles);
		unguard;
	}

	void Draw2DLine( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2 )
	{
		guard(UOpenGLRenderDevice::Draw2DLine);

		// kaufel: always clear z buffer for editor in non ortho view.
		if ( GIsEditor && !Frame->Viewport->IsOrtho() )
			ClearZ( Frame );

		SetNoTexture( 0 );
		SetBlend( PF_Highlighted );
		glColor3fv( &Color.X );
		glBegin( GL_LINES );
		glVertex3f( RFX2*P1.Z*(P1.X-Frame->FX2), RFY2*P1.Z*(P1.Y-Frame->FY2), P1.Z );
		glVertex3f( RFX2*P2.Z*(P2.X-Frame->FX2), RFY2*P2.Z*(P2.Y-Frame->FY2), P2.Z );
		glEnd();
		unguard;
	}

	void Draw3DLine( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2 )
	{
		guard(UOpenGLRenderDevice::Draw3DLine);

		P1 = P1.TransformPointBy( Frame->Coords );
		P2 = P2.TransformPointBy( Frame->Coords );

		if( Frame->Viewport->IsOrtho() )
		{
			// Zoom.
			P1.X = (P1.X) / Frame->Zoom + Frame->FX2;
			P1.Y = (P1.Y) / Frame->Zoom + Frame->FY2;
			P2.X = (P2.X) / Frame->Zoom + Frame->FX2;
			P2.Y = (P2.Y) / Frame->Zoom + Frame->FY2;
			P1.Z = P2.Z = 1;

			// See if points form a line parallel to our line of sight (i.e. line appears as a dot).
			if( Abs(P2.X-P1.X)+Abs(P2.Y-P1.Y)>=0.2 )
				Draw2DLine( Frame, Color, LineFlags, P1, P2 );
			else if( Frame->Viewport->Actor->OrthoZoom < ORTHO_LOW_DETAIL )
				Draw2DPoint( Frame, Color, LINE_None, P1.X-1, P1.Y-1, P1.X+1, P1.Y+1, P1.Z );
		}
		else
		{
			// kaufel: always clear z buffer for editor in non ortho view.
			if ( GIsEditor )
				ClearZ( Frame );

			SetNoTexture( 0 );
			SetBlend( PF_Highlighted );
			glColor3fv( &Color.X );
			glBegin( GL_LINES );
			glVertex3fv( &P1.X );
			glVertex3fv( &P2.X );
			glEnd();
		}
		unguard;
	}
	
	void Draw2DPoint( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FLOAT X1, FLOAT Y1, FLOAT X2, FLOAT Y2, FLOAT Z )
	{
		guard(UOpenGLRenderDevice::Draw2DPoint);
		SetBlend( PF_Highlighted );
		SetNoTexture( 0 );
		glColor3fv( &Color.X ); // vogel: was 4 - ONLY FOR UT!

		if( Frame->Viewport->IsOrtho() )
		{
			glBegin( GL_TRIANGLE_FAN );
			glVertex3f( RFX2*(X1-Frame->FX2-0.5), RFY2*(Y1-Frame->FY2-0.5), 1.0 );
			glVertex3f( RFX2*(X2-Frame->FX2+0.5), RFY2*(Y1-Frame->FY2-0.5), 1.0 );
			glVertex3f( RFX2*(X2-Frame->FX2+0.5), RFY2*(Y2-Frame->FY2+0.5), 1.0 );
			glVertex3f( RFX2*(X1-Frame->FX2-0.5), RFY2*(Y2-Frame->FY2+0.5), 1.0 );
			glEnd();
		}
		else if ( GIsEditor )
		{
			// kaufel: always clear z buffer for editor in non ortho view.
			ClearZ( Frame );

			if ( Z < 0.0 )
				Z = -Z;

			glBegin( GL_TRIANGLE_FAN );
			glVertex3f( RFX2*Z*(X1-Frame->FX2-0.5), RFY2*Z*(Y1-Frame->FY2-0.5), Z );
			glVertex3f( RFX2*Z*(X2-Frame->FX2+0.5), RFY2*Z*(Y1-Frame->FY2-0.5), Z );
			glVertex3f( RFX2*Z*(X2-Frame->FX2+0.5), RFY2*Z*(Y2-Frame->FY2+0.5), Z );
			glVertex3f( RFX2*Z*(X1-Frame->FX2-0.5), RFY2*Z*(Y2-Frame->FY2+0.5), Z );
			glEnd();
		}
		else
		{
			glBegin( GL_TRIANGLE_FAN );
			glVertex3f( RFX2*Z*(X1-Frame->FX2), RFY2*Z*(Y1-Frame->FY2), 0.5 );
			glVertex3f( RFX2*Z*(X2-Frame->FX2), RFY2*Z*(Y1-Frame->FY2), 0.5 );
			glVertex3f( RFX2*Z*(X2-Frame->FX2), RFY2*Z*(Y2-Frame->FY2), 0.5 );
			glVertex3f( RFX2*Z*(X1-Frame->FX2), RFY2*Z*(Y2-Frame->FY2), 0.5 );
			glEnd();		
		}
		unguard;
	}
	
	void ClearZ( FSceneNode* Frame )
	{
		guard(UOpenGLRenderDevice::ClearZ);
		SetBlend( PF_Occlude );
		glClear( GL_DEPTH_BUFFER_BIT );
		unguard;
	}
	
	void PushHit( const BYTE* Data, INT Count )
	{
		guard(UOpenGLRenderDevice::PushHit);
		glPushName( Count );
		for( INT i=0; i<Count; i+=4 )
			glPushName( *(INT*)(Data+i) );
		unguard;
	}
	
	void PopHit( INT Count, UBOOL bForce )
	{
		guard(UOpenGLRenderDevice::PopHit);
		glPopName();
		for( INT i=0; i<Count; i+=4 )
			glPopName();
		//!!implement bforce
		unguard;
	}
	
	// Stats.
	void GetStats( TCHAR* Result )
	{
		guard(UOpenGLRenderDevice::GetStats);
		appSprintf
		(
			Result,
			TEXT("OpenGL stats: Bind=%05.2f Image=%05.2f Complex=%05.2f Gouraud=%05.2f Tile=%05.2f 7777=%05.2f"),
			GSecondsPerCycle*1000*BindCycles,
			GSecondsPerCycle*1000*ImageCycles,
			GSecondsPerCycle*1000*ComplexCycles,
			GSecondsPerCycle*1000*GouraudCycles,
			GSecondsPerCycle*1000*TileCycles,
			GSecondsPerCycle*1000*Resample7777Cycles
		);
		unguard;
	}
	void DrawStats( FSceneNode* Frame )
	{
	}
	
	void ReadPixels( FColor* Pixels )
	{
		guard(UOpenGLRenderDevice::ReadPixels);
		glReadPixels( 0, 0, Viewport->SizeX, Viewport->SizeY, GL_RGBA, GL_UNSIGNED_BYTE, Pixels );
		for( INT i=0; i<Viewport->SizeY/2; i++ )
		{
			for( INT j=0; j<Viewport->SizeX; j++ )
			{
				Exchange( Pixels[j+i*Viewport->SizeX].R, Pixels[j+(Viewport->SizeY-1-i)*Viewport->SizeX].B );
				Exchange( Pixels[j+i*Viewport->SizeX].G, Pixels[j+(Viewport->SizeY-1-i)*Viewport->SizeX].G );
				Exchange( Pixels[j+i*Viewport->SizeX].B, Pixels[j+(Viewport->SizeY-1-i)*Viewport->SizeX].R );
			}
		}
		unguard;
	}

	void EndFlash()
	{
		if( FlashScale!=FPlane(.5,.5,.5,0) || FlashFog!=FPlane(0,0,0,0) )
		{
			SetBlend( PF_Highlighted );
			SetNoTexture( 0 );
			glColor4f( FlashFog.X, FlashFog.Y, FlashFog.Z, 1.0-Min(FlashScale.X*2.f,1.f) );
			FLOAT RFX2 = 2.0*RProjZ       /Viewport->SizeX;
			FLOAT RFY2 = 2.0*RProjZ*Aspect/Viewport->SizeY;
			glBegin( GL_TRIANGLE_FAN );
				glVertex3f( RFX2*(-Viewport->SizeX/2.0), RFY2*(-Viewport->SizeY/2.0), 1.0 );
				glVertex3f( RFX2*(+Viewport->SizeX/2.0), RFY2*(-Viewport->SizeY/2.0), 1.0 );
				glVertex3f( RFX2*(+Viewport->SizeX/2.0), RFY2*(+Viewport->SizeY/2.0), 1.0 );
				glVertex3f( RFX2*(-Viewport->SizeX/2.0), RFY2*(+Viewport->SizeY/2.0), 1.0 );
			glEnd();
		}
	}

	void PrecacheTexture( FTextureInfo& Info, DWORD PolyFlags )
	{
		guard(UOpenGLRenderDevice::PrecacheTexture);
		SetTexture( 0, Info, PolyFlags, 0 );
		unguard;
	}
};
IMPLEMENT_CLASS(UOpenGLRenderDevice);

// Static variables.
INT		UOpenGLRenderDevice::NumDevices    = 0;
INT		UOpenGLRenderDevice::LockCount     = 0;
UBOOL UOpenGLRenderDevice::GammaFirstTime = 1;
UOpenGLRenderDevice::FGammaRamp UOpenGLRenderDevice::OriginalRamp;
#ifdef WIN32
HGLRC		UOpenGLRenderDevice::hCurrentRC    = NULL;
TArray<HGLRC>	UOpenGLRenderDevice::AllContexts;
#else
UBOOL 		UOpenGLRenderDevice::GLLoaded	   = false;
#endif
TOpenGLMap<QWORD,UOpenGLRenderDevice::FCachedTexture> UOpenGLRenderDevice::SharedBindMap;

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
