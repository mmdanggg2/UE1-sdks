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


	UseTNT              workaround for buggy TNT/TNT2 drivers
	UseTrilinear		whether to use trilinear filtering			
	UseAlphaPalette		set to 0 for buggy drivers (GeForce)
	UseS3TC             whether to use compressed textures
	Use4444Textures		speedup for real low end cards (G200)
	MaxAnisotropy		maximum level of anisotropy used
	UseFilterSGIS		whether to use the SGIS filters
	MaxTMUnits          maximum number of TMUs UT will try to use
	DisableSpecialDT	disable the special detail texture approach
	LODBias             texture lod bias
	RefreshRate         requested refresh rate (Windows only)
	GammaOffset         offset for the gamma correction
	NoFiltering			uses GL_NEAREST as min/mag texture filters


TODO:
	- DOCUMENTATION!!! (especially all subtle assumptions)
	- get rid of some unnecessary state changes (#ifdef out)
	- array for FANCY_FISHEYE should be seperate from the rest

=============================================================================*/

#include "OpenGLDrv.h"
#include <math.h>

/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

#if __UNIX__
#define USE_SDL 1
#else
#define USE_SDL 0
#endif


#define FANCY_FISHEYE 0
#define MAX_TMUNITS   3			// vogel: maximum number of texture mapping units supported

#if FANCY_FISHEYE
#define VERTEX_ARRAY_SIZE 10000		// vogel: make sure FANCY_FISHEYE doesn't overflow array
#else
#define VERTEX_ARRAY_SIZE 2000		// vogel: better safe than sorry
#endif

#ifdef WIN32
#define STDGL 1	      			// Use standard GL driver or minidriver by default
#define DYNAMIC_BIND 1			// If 0, must static link to OpenGL32, Gdi32
#define GL_DLL (STDGL ? "OpenGL32.dll" : "3dfxgl.dll")
#endif

#ifdef __EMSCRIPTEN__
#define IMMEDIATE_MODE_DRAWARRAYS 1
#endif

/*-----------------------------------------------------------------------------
	OpenGLDrv.
-----------------------------------------------------------------------------*/

//
// An OpenGL rendering device attached to a viewport.
//
class UOpenGLRenderDevice : public URenderDevice
{
	DECLARE_CLASS(UOpenGLRenderDevice,URenderDevice,CLASS_Config,OpenGLDrv)

	// Information about a cached texture.
	struct FCachedTexture
	{
		GLuint Id;
		INT BaseMip;
		INT UBits, VBits;
		INT UCopyBits, VCopyBits;
		FPlane ColorNorm, ColorRenorm;

        // shut up Valgrind.  --ryan.
        FCachedTexture(void) :
            Id(0), BaseMip(0), UBits(0), VBits(0), UCopyBits(0), VCopyBits(0)
        {
            ColorNorm.X = ColorNorm.Y = ColorNorm.Z = ColorNorm.W = 0.0f;
            ColorRenorm.X = ColorRenorm.Y = ColorRenorm.Z = ColorRenorm.W = 0.0f;
        }
	};

    struct FGLVertexData
    {
    	// Geometry 	
    	struct FGLVertex
	    {
    		FLOAT x;
	    	FLOAT y;
		    FLOAT z;
    	} Vertex;

    	// Texcoords
	    struct FGLTexCoord
	    {
    		struct FGLTMU {
		    	FLOAT u;
			    FLOAT v;
    		} TMU [MAX_TMUNITS];
	    } TexCoord;

    	// Primary and secondary (specular) color
	    struct FGLColor
    	{
	    	INT color;
		    INT specular;
    	} Color;
    } VertexData[VERTEX_ARRAY_SIZE];

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

#ifdef WIN32
	// Permanent variables.
	HGLRC hRC;
	HWND hWnd;
	HDC hDC;

	// Gamma Ramp to restore at exit	
	FGammaRamp OriginalRamp; 
#endif

	UBOOL WasFullscreen;
	TMap<QWORD,FCachedTexture> LocalBindMap, *BindMap;
	TArray<FPlane> Modes;
	UViewport* Viewport;

	// Timing.
	DWORD BindCycles, ImageCycles, ComplexCycles, GouraudCycles, TileCycles;

	// Hardware constraints.
	FLOAT LODBias;
	FLOAT GammaOffset;
	INT MaxLogUOverV;
	INT MaxLogVOverU;
	INT MinLogTextureSize;
	INT MaxLogTextureSize;
	INT MaxAnisotropy;
	INT TMUnits;
	INT MaxTMUnits;
	INT RefreshRate;
    INT VARSize;
	UBOOL UsePrecache;
	UBOOL UseMultiTexture;
	UBOOL UsePalette;
	UBOOL ShareLists;
	UBOOL AlwaysMipmap;
	UBOOL UseTrilinear;
	UBOOL UseVertexSpecular;
	UBOOL UseAlphaPalette;
	UBOOL UseS3TC;
	UBOOL Use4444Textures;
	UBOOL UseTNT; 					// vogel: REMOVE ME - HACK!
	UBOOL UseCVA;
	UBOOL UseFilterSGIS;	
	UBOOL UseDetailAlpha;
	UBOOL DisableSpecialDT;				// vogel: TODO: should be collapsed with UseDetailAlpha
	UBOOL NoFiltering;
    UBOOL UseDrawArrays;
	WORD VertexIndexList[VERTEX_ARRAY_SIZE];
	UBOOL BufferActorTris;
	INT BufferedVerts;

	UBOOL ColorArrayEnabled;
	UBOOL RenderFog;
	UBOOL GammaFirstTime;

#ifdef IMMEDIATE_MODE_DRAWARRAYS
    bool bDrawTexCoords;
    bool bDrawColors;
    bool bDrawSecondaryColors;
#endif

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
		FPlane ColorNorm;
		FPlane ColorRenorm;
	} TexInfo[MAX_TMUNITS];
	FLOAT RFX2, RFY2;
	GLuint AlphaTextureId;
	GLuint NoTextureId;

#if FANCY_FISHEYE
	GLuint backbuffer;
	FLOAT  Timer;
	INT  FancyX, FancyY;	
	UBOOL  UseFisheye;
#endif

    // vertex_array_range support.
    UBOOL UseVAR;
    BYTE *VARSpace;
    INT VAROffset;
    INT VARMaxOffsets;

	// Static variables.
	static TMap<QWORD,FCachedTexture> *SharedBindMap;
	static INT NumDevices;
	static INT LockCount;

#ifdef WIN32
	static TArray<HGLRC> AllContexts;
	static HGLRC   hCurrentRC;
	static HMODULE hModuleGlMain;
	static HMODULE hModuleGlGdi;
#else
	static UBOOL GLLoaded;
#endif

	// GL functions.
	#define GL_EXT(name) static UBOOL SUPPORTS##name;
	#ifdef __EMSCRIPTEN__
	#define GL_PROC(ext,ret,func,parms)
	#else
	#define GL_PROC(ext,ret,func,parms) static ret (STDCALL *func)parms;
	#endif
	#include "OpenGLFuncs.h"
	#undef GL_EXT
	#undef GL_PROC

#ifdef RGBA_MAKE
#undef RGBA_MAKE
#endif

	inline int RGBA_MAKE( BYTE r, BYTE g, BYTE b, BYTE a)		// vogel: I hate macros...
	{
#ifdef __INTEL_BYTE_ORDER__
		return (a << 24) | (b <<16) | ( g<< 8) | r;
#elif MACOSX
        if (SUPPORTS_GL_ATI_array_rev_comps_in_4_bytes)
		    return (a << 24) | (b <<16) | ( g<< 8) | r;

		return (r << 24) | (g <<16) | ( b<< 8) | a;
#else
		return (r << 24) | (g <<16) | ( b<< 8) | a;
#endif
	}

#ifndef IMMEDIATE_MODE_DRAWARRAYS
    #define myglEnableClientState glEnableClientState
    #define myglDisableClientState glDisableClientState
#else
    // this is a gigantic hack until we work out our Emscripten rendering politics.
    void myglEnableClientState(const GLenum type)
    {
        switch (type)
        {
            case GL_VERTEX_ARRAY: break;  // always enabled, cool
            case GL_TEXTURE_COORD_ARRAY: bDrawTexCoords = true;  break;
            case GL_COLOR_ARRAY: bDrawColors = true;  break;
            case GL_SECONDARY_COLOR_ARRAY_EXT: bDrawSecondaryColors = true;  break;
            default: appErrorf(TEXT("uhoh, unexpected GL array type!")); break;
        }
        glEnableClientState(type);
    }

    void myglDisableClientState(const GLenum type)
    {
        switch (type)
        {
            case GL_TEXTURE_COORD_ARRAY: bDrawTexCoords = false;  break;
            case GL_COLOR_ARRAY: bDrawColors = false;  break;
            case GL_SECONDARY_COLOR_ARRAY_EXT: bDrawSecondaryColors = false;  break;
            default: appErrorf(TEXT("uhoh, unexpected GL array type!")); break;
        }
        glDisableClientState(type);
    }
#endif

	inline void DrawArrays( GLenum type, int start, int num )	// vogel: ... and macros hate me
	{
        #ifdef IMMEDIATE_MODE_DRAWARRAYS
        // this is nasty, but both glDrawArrays() and glDrawElements() are failing in Emscripten! Could probably move this to VBOs, but
        //  honestly, we should move to proper GLES 2.0 and dump Regal+LegacyGL instead.

        glBegin(type);
        for (int i = 0; i < num; i++)
        {
            const FGLVertexData &v = VertexData[i+start];
            //if (bDrawTexCoords)
            {
                if (!UseMultiTexture)
                    glTexCoord2f(v.TexCoord.TMU[0].u, v.TexCoord.TMU[0].v);
                else
                {
                    for (int t = 0; t < MAX_TMUNITS; t++)
                        glMultiTexCoord2fARB(GL_TEXTURE0 + t, v.TexCoord.TMU[t].u, v.TexCoord.TMU[t].v);
                }
            }

            if (bDrawColors && ColorArrayEnabled)
            {
                const GLubyte *colors = (const GLubyte *) &v.Color.color;
                glColor4ub(colors[0], colors[1], colors[2], colors[3]);
            }

            if (bDrawSecondaryColors)
            {
                const GLubyte *scolors = (const GLubyte *) &v.Color.specular;
                glSecondaryColor3ub(scolors[0], scolors[1], scolors[2]);
            }

            glVertex3f(v.Vertex.x, v.Vertex.y, v.Vertex.z);
        }
        glEnd();
        #else
	    if (UseVAR)
        {
            if (VAROffset + num >= VARMaxOffsets)
            {
                // You _should_ flush for safety here, but it's a speed hit
                //  that you don't need if you allocated a really big VAR
                //  chunk to start with...
                //glVertexArrayFlushNV();
                VAROffset = 0;
            }

            INT size = num * sizeof (FGLVertexData);
            BYTE *dest = VARSpace + (sizeof (FGLVertexData) * VAROffset);
            memcpy(dest, &VertexData[start], size);

            #if MACOSX
            //glFlushVertexArrayRangeAPPLE(size, dest);
            #endif

            start = VAROffset;
            VAROffset += num;

            if (!UseDrawArrays)
                for (INT i = 0; i < num; i++)
                    VertexIndexList[i] = (WORD) (start + i);
        }

        if (UseDrawArrays)
    		glDrawArrays( type, start, num );
        else
		    glDrawElements( type, num, GL_UNSIGNED_SHORT, &VertexIndexList[UseVAR ? 0 : start] );
        #endif
	}


    // UObject interface.
    void StaticConstructor()
	{
		guard(UOpenGLRenderDevice::StaticConstructor);

		new(GetClass(),TEXT("UsePrecache"),         RF_Public)UBoolProperty ( CPP_PROPERTY(UsePrecache         ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("ShareLists"),          RF_Public)UBoolProperty ( CPP_PROPERTY(ShareLists          ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("UseMultiTexture"),     RF_Public)UBoolProperty ( CPP_PROPERTY(UseMultiTexture     ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("UseTrilinear"),        RF_Public)UBoolProperty ( CPP_PROPERTY(UseTrilinear        ), TEXT("Options"), CPF_Config );	
		new(GetClass(),TEXT("UsePalette"),          RF_Public)UBoolProperty ( CPP_PROPERTY(UsePalette          ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("UseAlphaPalette"),     RF_Public)UBoolProperty ( CPP_PROPERTY(UseAlphaPalette     ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("AlwaysMipmap"),        RF_Public)UBoolProperty ( CPP_PROPERTY(AlwaysMipmap        ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("UseVertexSpecular"),   RF_Public)UBoolProperty ( CPP_PROPERTY(UseVertexSpecular   ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("UseS3TC"),             RF_Public)UBoolProperty ( CPP_PROPERTY(UseS3TC             ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("UseTNT"),              RF_Public)UBoolProperty ( CPP_PROPERTY(UseTNT              ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("Use4444Textures"),     RF_Public)UBoolProperty ( CPP_PROPERTY(Use4444Textures     ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("UseFilterSGIS"),       RF_Public)UBoolProperty ( CPP_PROPERTY(UseFilterSGIS       ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("DisableSpecialDT"),    RF_Public)UBoolProperty ( CPP_PROPERTY(DisableSpecialDT    ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("NoFiltering"),         RF_Public)UBoolProperty ( CPP_PROPERTY(NoFiltering         ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("UseDrawArrays"),       RF_Public)UBoolProperty ( CPP_PROPERTY(UseDrawArrays       ), TEXT("Options"), CPF_Config );

		//new(GetClass(),TEXT("SupportsLazyTextures"),RF_Public)UBoolProperty ( CPP_PROPERTY(((INT&) SupportsLazyTextures  )), TEXT("Options"), CPF_Config );
		//new(GetClass(),TEXT("PrefersDeferredLoad"), RF_Public)UBoolProperty ( CPP_PROPERTY(((INT&) PrefersDeferredLoad   )), TEXT("Options"), CPF_Config );


		new(GetClass(),TEXT("RefreshRate"),         RF_Public)UIntProperty  ( CPP_PROPERTY(RefreshRate         ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("MaxTMUnits"),          RF_Public)UIntProperty  ( CPP_PROPERTY(MaxTMUnits          ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("MaxAnisotropy"),       RF_Public)UIntProperty  ( CPP_PROPERTY(MaxAnisotropy       ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("MaxLogUOverV"),        RF_Public)UIntProperty  ( CPP_PROPERTY(MaxLogUOverV        ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("MaxLogVOverU"),        RF_Public)UIntProperty  ( CPP_PROPERTY(MaxLogVOverU        ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("MinLogTextureSize"),   RF_Public)UIntProperty  ( CPP_PROPERTY(MinLogTextureSize   ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("MaxLogTextureSize"),   RF_Public)UIntProperty  ( CPP_PROPERTY(MaxLogTextureSize   ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("VARSize"),             RF_Public)UIntProperty  ( CPP_PROPERTY(VARSize             ), TEXT("Options"), CPF_Config );

		new(GetClass(),TEXT("LODBias"),             RF_Public)UFloatProperty( CPP_PROPERTY(LODBias             ), TEXT("Options"), CPF_Config );
		new(GetClass(),TEXT("GammaOffset"),         RF_Public)UFloatProperty( CPP_PROPERTY(GammaOffset         ), TEXT("Options"), CPF_Config );
			
		SpanBased				= 0;
		SupportsFogMaps			= 1;
		SupportsDistanceFog		= 0;
		FullscreenOnly			= 0;

		SupportsLazyTextures	= 0;
		PrefersDeferredLoad		= 0;

		unguard;
	}
	
	
	// Implementation.	
	void SetGamma( FLOAT GammaCorrection )
	{
		if ( GammaCorrection <= 0.1f )
			GammaCorrection = 1.f;

		GammaCorrection += GammaOffset;

		FGammaRamp Ramp;
		for( INT i=0; i<256; i++ )
		{
			Ramp.red[i] = Ramp.green[i] = Ramp.blue[i] = appRound(appPow(i/255.f,1.0f/(2.5f*GammaCorrection))*65535.f);
		}
#if USE_SDL
		// vogel: FIXME (talk to Sam)
		// SDL_SetWindowGammaRamp( Viewport->Window, Ramp.red, Ramp.green, Ramp.blue );
		FLOAT gamma = 0.4 + 2 * GammaCorrection; 
		SDL_SetWindowBrightness( (SDL_Window *) Viewport->GetWindow(), gamma );
#else
		if ( GammaFirstTime )
		{
			//OutputDebugString(TEXT("GetDeviceGammaRamp in SetGamma"));
			GetDeviceGammaRamp( hDC, &OriginalRamp );
			GammaFirstTime = false;
		}
		//OutputDebugString(TEXT("SetDeviceGammaRamp in SetGamma"));
		SetDeviceGammaRamp( hDC, &Ramp );
#endif	       
	}

	UBOOL FindExt( const TCHAR* Name )
	{
		guard(UOpenGLRenderDevice::FindExt);
		UBOOL Result = strstr( (char*) glGetString(GL_EXTENSIONS), appToAnsi(Name) ) != NULL;
		if( Result )
			debugf( NAME_Init, TEXT("Device supports: %s"), Name );
		return Result;
		unguard;
	}

	void FindProc( void*& ProcAddress, const char* Name, const char* SupportName, UBOOL& Supports, UBOOL AllowExt )
	{
		guard(UOpenGLRenderDevice::FindProc);
//#ifndef __EMSCRIPTEN__
#if USE_SDL
		if( !ProcAddress )
			ProcAddress = (void*) SDL_GL_GetProcAddress( Name );
#else
#if DYNAMIC_BIND
		if( !ProcAddress )
			ProcAddress = GetProcAddress( hModuleGlMain, Name );
		if( !ProcAddress )
			ProcAddress = GetProcAddress( hModuleGlGdi, Name );
#endif
		if( !ProcAddress && Supports && AllowExt )
			ProcAddress = wglGetProcAddress( Name );
#endif
//#endif
		if( !ProcAddress )
		{
			if( Supports )
				debugf( TEXT("   Missing function '%s' for '%s' support"), appFromAnsi(Name), appFromAnsi(SupportName) );
			Supports = 0;
		}
		unguard;
	}

	void FindProcs( UBOOL AllowExt )
	{
		guard(UOpenGLDriver::FindProcs);
		#ifdef __EMSCRIPTEN__
SUPPORTS_GL = 1;
		#define GL_EXT(name)
    	#define GL_PROC(ext,ret,func,parms)
    	#else
		#define GL_EXT(name) if( AllowExt ) SUPPORTS##name = FindExt( &TEXT(#name)[1] );
		#define GL_PROC(ext,ret,func,parms) FindProc( *(void**)&func, #func, #ext, SUPPORTS##ext, AllowExt );
    	#endif
		#include "OpenGLFuncs.h"
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
        #if 0   // we probably shouldn't stall out to call glGetError.
		static bool NoMoreErrorReports = false;
		if (NoMoreErrorReports)
			return;

		GLenum Error = glGetError();
		if( Error!=GL_NO_ERROR )
		{
			const TCHAR* Msg;
			switch( Error )
			{
				case GL_INVALID_ENUM:
					Msg = TEXT("GL_INVALID_ENUM");
					break;
				case GL_INVALID_VALUE:
					Msg = TEXT("GL_INVALID_VALUE");
					break;
				case GL_INVALID_OPERATION:
					Msg = TEXT("GL_INVALID_OPERATION");
					break;
				case GL_STACK_OVERFLOW:
					Msg = TEXT("GL_STACK_OVERFLOW");
					break;
				case GL_STACK_UNDERFLOW:
					Msg = TEXT("GL_STACK_UNDERFLOW");
					break;
				case GL_OUT_OF_MEMORY:
					Msg = TEXT("GL_OUT_OF_MEMORY");
					break;
				default :
					Msg = TEXT("UNKNOWN");
			};
			//appErrorf( TEXT("OpenGL Error: %s (%s)"), Msg, Tag );
			debugf( TEXT("OpenGL Error: %s (%s)"), Msg, Tag );

			static INT NumReports = 0;
			if (++NumReports >= 10)
			{
				debugf(TEXT("(Not logging any more OpenGL errors.)"));
				NoMoreErrorReports = true;
			}
		}
        #endif
	}
	
	void SetNoTexture( INT Multi )
	{
		guard(UOpenGLRenderDevice::SetNoTexture);
		if( TexInfo[Multi].CurrentCacheID != NoTextureId )
		{
			// Set small white texture.
			clockFast(BindCycles);
			glBindTexture( GL_TEXTURE_2D, NoTextureId );
			TexInfo[Multi].CurrentCacheID = NoTextureId;
			unclockFast(BindCycles);
		}
		unguard;
	}

	void SetAlphaTexture( INT Multi )
	{
		guard(UOpenGLRenderDevice::SetAlphaTexture);
		if( TexInfo[Multi].CurrentCacheID != AlphaTextureId )
		{
			// Set alpha gradient texture.
			clockFast(BindCycles);
			glBindTexture( GL_TEXTURE_2D, AlphaTextureId );
			TexInfo[Multi].CurrentCacheID = AlphaTextureId;
			unclockFast(BindCycles);
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

		// Find in cache.
		if( Info.CacheID==Tex.CurrentCacheID && !Info.bRealtimeChanged )
			return;

		// Make current.
		clockFast(BindCycles);
		Tex.CurrentCacheID = Info.CacheID;
		FCachedTexture *Bind=BindMap->Find(Info.CacheID), *ExistingBind=Bind;
		if( !Bind )
		{			
			// Figure out OpenGL-related scaling for the texture.
			FCachedTexture *tex = new FCachedTexture();
			Bind            = &BindMap->Set( Info.CacheID, *tex );
			glGenTextures( 1, &Bind->Id );
			AllocatedTextures++;
			Bind->BaseMip   = Min(0,Info.NumMips-1);
			Bind->UCopyBits = 0;
			Bind->VCopyBits = 0;
			Bind->UBits     = Info.Mips[Bind->BaseMip]->UBits;
			Bind->VBits     = Info.Mips[Bind->BaseMip]->VBits;			
			if( Bind->UBits-Bind->VBits > MaxLogUOverV )
			{
				Bind->VCopyBits += (Bind->UBits-Bind->VBits)-MaxLogUOverV;
				Bind->VBits      = Bind->UBits-MaxLogUOverV;
			}
			if( Bind->VBits-Bind->UBits > MaxLogVOverU )
			{
				Bind->UCopyBits += (Bind->VBits-Bind->UBits)-MaxLogVOverU;
				Bind->UBits      = Bind->VBits-MaxLogVOverU;
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
		glBindTexture( GL_TEXTURE_2D, Bind->Id );
		unclockFast(BindCycles);

		// Account for all the impact on scale normalization.
		Tex.UMult = 1.0f / (Info.UScale * (Info.USize << Bind->UCopyBits));
		Tex.VMult = 1.0f / (Info.VScale * (Info.VSize << Bind->VCopyBits));

		// Upload if needed.
		if( !ExistingBind || Info.bRealtimeChanged )
		{
			// Cleanup texture flags.
			if ( SupportsLazyTextures )
				Info.Load();
			Info.bRealtimeChanged = 0;

			// Set maximum color.
			// Info.CacheMaxColor();
			*Info.MaxColor = FColor(255,255,255,255); // Brandon's color hack.
			Bind->ColorNorm = Info.MaxColor->Plane();
			Bind->ColorNorm.W = 1;

			Bind->ColorRenorm = Bind->ColorNorm;

			// Generate the palette.
			FColor LocalPal[256], *NewPal=Info.Palette, TempColor(0,0,0,0);	
			UBOOL Paletted;
			if ( UseAlphaPalette )
				Paletted = UsePalette && Info.Palette;
			else
				Paletted = UsePalette && Info.Palette && !(PolyFlags & PF_Masked) && Info.Palette[0].A==255;

			if( Info.Palette )
			{
				TempColor = Info.Palette[0];
				if( PolyFlags & PF_Masked )
					Info.Palette[0] = FColor(0,0,0,0);
				NewPal = LocalPal;
				for( INT i=0; i<256; i++ )
				{
					FColor& Src = Info.Palette[i];
					NewPal[i].R = Src.R;
					NewPal[i].G = Src.G;
					NewPal[i].B = Src.B;
					NewPal[i].A = Src.A;
				}	
				if( Paletted )
					glColorTableEXT( GL_TEXTURE_2D, GL_RGBA, 256, GL_RGBA, GL_UNSIGNED_BYTE, NewPal );
			}

			// Download the texture.
			clockFast(ImageCycles);
			FMemMark Mark(GMem);
			BYTE* Compose  = New<BYTE>( GMem, (1<<(Bind->UBits+Bind->VBits))*4 );
			UBOOL SkipMipmaps = Info.NumMips==1 && !AlwaysMipmap;
		
			INT MaxLevel;
			if ( UseTNT )
				MaxLevel = Max(Bind->UBits,Bind->VBits);
			else
				MaxLevel = Min(Bind->UBits,Bind->VBits) - MinLogTextureSize;

			for( INT Level=0; Level<=MaxLevel; Level++ )
			{
				// Convert the mipmap.
				INT MipIndex=Bind->BaseMip+Level, StepBits=0;
				if( MipIndex>=Info.NumMips )
				{
					StepBits = MipIndex - (Info.NumMips - 1);
					MipIndex = Info.NumMips - 1;
				}
				FMipmapBase* Mip      = Info.Mips[MipIndex];
				DWORD        Mask     = Mip->USize-1;			
				BYTE* 	     Src      = (BYTE*)Compose;
				GLuint       SourceFormat = GL_RGBA, InternalFormat = GL_RGBA8;
				if( Mip->DataPtr )
				{					
					if( (Info.Format == TEXF_DXT1) && SupportsTC )
					{
						guard(CompressedTexture_S3TC);
						InternalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;				       
						unguard;
					}
					else if( Paletted )
					{
						guard(ConvertP8_P8);
						SourceFormat   = GL_COLOR_INDEX;
						InternalFormat = GL_COLOR_INDEX8_EXT;
						BYTE* Ptr      = (BYTE*)Compose;
						for( INT i=0; i<(1<<Max(0,Bind->VBits-Level)); i++ )
						{
							BYTE* Base = (BYTE*)Mip->DataPtr + ((i<<StepBits)&(Mip->VSize-1))*Mip->USize;
							for( INT j=0; j<(1<<Max(0,Bind->UBits-Level+StepBits)); j+=(1<<StepBits) )
								*Ptr++ = Base[j&Mask];
						}
						unguard;
					}
					else if( Info.Palette )
					{
						guard(ConvertP8_RGBA8888);					     
						SourceFormat   = GL_RGBA;
						if ( Use4444Textures )
							InternalFormat = GL_RGBA4; // vogel: will cause banding in menus
						else
							InternalFormat = GL_RGBA8;
						FColor* Ptr    = (FColor*)Compose;
						for( INT i=0; i<(1<<Max(0,Bind->VBits-Level)); i++ )
						{
							BYTE* Base = (BYTE*)Mip->DataPtr + ((i<<StepBits)&(Mip->VSize-1))*Mip->USize;
							for( INT j=0; j<(1<<Max(0,Bind->UBits-Level+StepBits)); j+=(1<<StepBits) )
								*Ptr++ = NewPal[Base[j&Mask]];						
						}
						unguard;
					}		
					else
					{
						guard(ConvertBGRA7777_RGBA8888);
						SourceFormat   = GL_RGBA;
						InternalFormat = GL_RGBA8;
						FColor* Ptr    = (FColor*)Compose;
                        INT imax = (1<<Max(0,Bind->VBits-Level));
                        INT UClampMinus1 = Info.UClamp-1;
						for( INT i=0; i<imax; i++ )
						{
							FColor* Base = (FColor*)Mip->DataPtr + Min<DWORD>((i<<StepBits)&(Mip->VSize-1),Info.VClamp-1)*Mip->USize;
                            const INT jmax = (1<<Max(0,Bind->UBits-Level+StepBits));
                            const INT jinc = (1<<StepBits);
							for( INT j=0; j<jmax; j+=jinc )
							{
								FColor& Src = Base[Min<DWORD>(j&Mask,UClampMinus1)];
								// vogel: optimize it.
								Ptr->R      = 2 * Src.B;
								Ptr->G      = 2 * Src.G;
								Ptr->B      = 2 * Src.R;
								Ptr->A      = 2 * Src.A; // because of 7777

								Ptr++;
							}
						}
						unguard;
					}
				}
				if( ExistingBind )
				{
					if ( (Info.Format == TEXF_DXT1) && SupportsTC )
					{
						guard(glCompressedTexSubImage2D);
						glCompressedTexSubImage2DARB( 
							GL_TEXTURE_2D, 
							Level, 
							0, 
							0, 
							1<<Max(0,Bind->UBits-Level), 
							1<<Max(0,Bind->VBits-Level), 
							InternalFormat,
							(1<<Max(0,Bind->UBits-Level)) * (1<<Max(0,Bind->VBits-Level)) / 2,
							Mip->DataPtr );
						unguard;
					}
					else
					{
						guard(glTexSubImage2D);
						glTexSubImage2D( 
							GL_TEXTURE_2D, 
							Level, 
							0, 
							0, 
							1<<Max(0,Bind->UBits-Level), 
							1<<Max(0,Bind->VBits-Level), 
							SourceFormat, 
							GL_UNSIGNED_BYTE, 
							Src );
						unguard;
					}
				}
				else
				{
					if ( (Info.Format == TEXF_DXT1) && SupportsTC )
					{
						guard(glCompressedTexImage2D);					
						glCompressedTexImage2DARB(
							GL_TEXTURE_2D, 
							Level, 
							InternalFormat, 
							1<<Max(0,Bind->UBits-Level), 
							1<<Max(0,Bind->VBits-Level), 
							0,
							(1<<Max(0,Bind->UBits-Level)) * (1<<Max(0,Bind->VBits-Level)) / 2,
							Mip->DataPtr );
						unguard;					
       					}
					else
					{
						guard(glTexImage2D);
						glTexImage2D( 
							GL_TEXTURE_2D, 
							Level, 
							InternalFormat, 
							1<<Max(0,Bind->UBits-Level), 
							1<<Max(0,Bind->VBits-Level), 
							0, 
							SourceFormat, 
							GL_UNSIGNED_BYTE, 
							Src );
                        #ifdef __EMSCRIPTEN__
                        glGenerateMipmap(GL_TEXTURE_2D);
                        #endif
						unguard;
					}	
				}
				if( SkipMipmaps )
					break;
			}

			Mark.Pop();
			unclockFast(ImageCycles);

			// Set texture state.
			if( NoFiltering )
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			}
			else if( !(PolyFlags & PF_NoSmooth) )
			{
				if ( UseFilterSGIS && !SkipMipmaps )
				{
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_FILTER4_SGIS);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_FILTER4_SGIS);
				}
				else
				{
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, SkipMipmaps ? GL_LINEAR : 
						(UseTrilinear ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR_MIPMAP_NEAREST) );
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				}
				if ( MaxAnisotropy )
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, MaxAnisotropy );

			}
			else
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, SkipMipmaps ? GL_NEAREST : GL_NEAREST_MIPMAP_NEAREST );
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			}
		       
			if ( !UseTNT )
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MaxLevel < 0 ? 0 : MaxLevel);

			// Cleanup.
			if( Info.Palette )
				Info.Palette[0] = TempColor;
			if( SupportsLazyTextures )
				Info.Unload();
		}

		// Copy color norm.
		Tex.ColorNorm   = Bind->ColorNorm;
		Tex.ColorRenorm = Bind->ColorRenorm;

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

#if USE_SDL
	// URenderDevice interface.
	UBOOL Init( UViewport* InViewport, INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen )
	{
		guard(UOpenGLRenderDevice::Init);

		debugf( TEXT("Initializing OpenGLDrv...") );

        #ifdef IMMEDIATE_MODE_DRAWARRAYS
        bDrawTexCoords = false;
        bDrawColors = false;
        bDrawSecondaryColors = false;
        #endif

        if (SharedBindMap == NULL)
            SharedBindMap = new TMap<QWORD,UOpenGLRenderDevice::FCachedTexture>;

		GammaFirstTime = 1;
		// Init global GL.
		if( NumDevices==0 )
		{
			// Bind the library.
			FString OpenGLLibName;
			FString Section = TEXT("OpenGLDrv.OpenGLRenderDevice");
			// Default to (SDL's default) if not defined
			if (!GConfig->GetString( *Section, TEXT("OpenGLLibName"), OpenGLLibName ))
				OpenGLLibName = TEXT("");

			if ( !GLLoaded )
			{
				// Only call it once as succeeding calls will 'fail'.
				debugf( TEXT("binding %s"), *OpenGLLibName );
				if ( SDL_GL_LoadLibrary( OpenGLLibName.Len() ? TCHAR_TO_ANSI(*OpenGLLibName) : NULL ) == -1 )
					appErrorf( ANSI_TO_TCHAR(SDL_GetError()) );
  				GLLoaded = true;
			}

			SUPPORTS_GL = 1;
			FindProcs( 0 );
			if( !SUPPORTS_GL )
				return 0;		
		}
		NumDevices++;

		BindMap = ShareLists ? SharedBindMap : &LocalBindMap;
		Viewport = InViewport;
		for( WORD i = 0; i < VERTEX_ARRAY_SIZE; i++ )
			VertexIndexList[i] = i;

		// Try to change resolution to desired.
		if( !SetRes( NewX, NewY, NewColorBytes, Fullscreen ) )
			return FailedInitf( LocalizeError("ResFailed") );

		return 1;
		unguard;
	}
	
#else
	UBOOL Init( UViewport* InViewport, INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen )
	{
		guard(UOpenGLRenderDevice::Init);
		debugf( TEXT("Initializing OpenGLDrv...") );

		GammaFirstTime = 1;
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
#if DYNAMIC_BIND
			// Find DLL's.
			hModuleGlMain = LoadLibraryA( GL_DLL );
			if( !hModuleGlMain )
			{
				debugf( NAME_Init, LocalizeError("NoFindGL"), appFromAnsi(GL_DLL) );
				return 0;
			}
			hModuleGlGdi = LoadLibraryA( "GDI32.dll" );
			check(hModuleGlGdi);

			// Find functions.
			SUPPORTS_GL = 1;
			FindProcs( 0 );
			if( !SUPPORTS_GL )
				return 0;
#endif
			
		}
		NumDevices++;

		// Init this GL rendering context.
		BindMap = ShareLists ? SharedBindMap : &LocalBindMap;
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

	void PrintFormat( HDC hDC, INT nPixelFormat )
	{
		guard(UOpenGlRenderDevice::PrintFormat);
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

	void UnsetRes()
	{
		guard(UOpenGLRenderDevice::UnsetRes);

        #ifndef WIN32
		Flush( 1 );
        #endif

        if (UseVAR)
        {
            if (VARSpace != NULL)
            {
                #ifdef WIN32
                    glVertexArrayRangeNV( 0, 0 );
                    glDisableClientState(GL_VERTEX_ARRAY_RANGE_NV);
                    wglFreeMemoryNV(VARSpace);
                #elif MACOSX
                    glVertexArrayRangeAPPLE( 0, 0 );
                    glDisableClientState(GL_VERTEX_ARRAY_RANGE_APPLE);
                    delete[] VARSpace;
                #elif !defined(__EMSCRIPTEN__)
                    glVertexArrayRangeNV( 0, 0 );
                    glDisableClientState(GL_VERTEX_ARRAY_RANGE_NV);
                    glXFreeMemoryNV(VARSpace);
                #endif
                VARSpace = NULL;
            }

            VAROffset = VARMaxOffsets = 0;
        }

        #ifdef WIN32
		check(hRC)
		hCurrentRC = NULL;
		verify(wglMakeCurrent( NULL, NULL ));
		verify(wglDeleteContext( hRC ));
		verify(AllContexts.RemoveItem(hRC)==1);
		hRC = NULL;
		if( WasFullscreen )
			TCHAR_CALL_OS(ChangeDisplaySettings(NULL,0),ChangeDisplaySettingsA(NULL,0));
        #endif

		unguard;
	}


	UBOOL SetRes( INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen )
	{
		guard(UOpenGlRenderDevice::SetRes);
		
		FString	Section = TEXT("OpenGLDrv.OpenGLRenderDevice");

#if USE_SDL
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

        /* this isn't actually OpenGL ES code; Emscripten will force our fixed-function garbage to WebGL,
           but we need to lie here to get a GL context from the system at all. */
        #ifdef __EMSCRIPTEN__
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES );
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 0 );
        #endif

		// Change window size.
		debugf(TEXT("OpenGL::SetRes(%d,%d)"), NewX, NewY);
		Viewport->ResizeViewport( Fullscreen ? (BLIT_Fullscreen|BLIT_OpenGL) : (BLIT_HardwarePaint|BLIT_OpenGL), NewX, NewY, NewColorBytes );
		glViewport( 0, 0, NewX, NewY );

        #ifdef __EMSCRIPTEN__
        RegalMakeCurrent((RegalSystemContext)1);
        #endif

		if (SDL_GL_SetSwapInterval(-1) == -1)  // try swap tearing first...
			SDL_GL_SetSwapInterval(1);

#else // USE_SDL
		debugf( TEXT("Enter SetRes()") );

		// If not fullscreen, and color bytes hasn't changed, do nothing.
		if( hRC && !Fullscreen && !WasFullscreen && NewColorBytes==Viewport->ColorBytes )
		{
			if( !Viewport->ResizeViewport( BLIT_HardwarePaint|BLIT_OpenGL, NewX, NewY, NewColorBytes ) )
				return 0;
			glViewport( 0, 0, NewX, NewY );
			return 1;
		}

		// Exit res.
		if( hRC ) {
			debugf( TEXT("UnSetRes() -> hRc != NULL") );
			UnsetRes();
		}

#if !STDGL
		Fullscreen = 1; /* Minidrivers are fullscreen only!! */
#endif

		// Change display settings.
		if( Fullscreen  && STDGL)
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
				//dm.dmBitsPerPel = NewColorBytes * 8;
				if ( RefreshRate )
				{
					dm.dmDisplayFrequency = RefreshRate;
					dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;// | DM_BITSPERPEL;
				}
				else
				{
					dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;// | DM_BITSPERPEL;
				}
				if( ChangeDisplaySettingsA( &dm, CDS_FULLSCREEN )!=DISP_CHANGE_SUCCESSFUL )
				{
					debugf( TEXT("ChangeDisplaySettingsA failed: %ix%i"), NewX, NewY );
					return 0;
				}
			}
			else
#endif // UNICODE
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
					dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;// | DM_BITSPERPEL;
				}
				else
				{
					dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;// | DM_BITSPERPEL;
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
		INT DesiredColorBits   = NewColorBytes<=2 ? 16 : 24; // vogel: changed to saner values 
		INT DesiredStencilBits = 0;                          // NewColorBytes<=2 ? 0  : 8;
		INT DesiredDepthBits   = NewColorBytes<=2 ? 16 : 24; // NewColorBytes<=2 ? 16 : 32;
		PIXELFORMATDESCRIPTOR pfd =
		{
			sizeof(PIXELFORMATDESCRIPTOR),
			1,
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
			PFD_TYPE_RGBA,
			(BYTE) DesiredColorBits,
			0,0,0,0,0,0,
			0,0,
			0,0,0,0,0,
			(BYTE) DesiredDepthBits,
			(BYTE) DesiredStencilBits,
			0,
			PFD_MAIN_PLANE,
			0,
			0,0,0
		};
		INT nPixelFormat = ChoosePixelFormat( hDC, &pfd );
		Parse( appCmdLine(), TEXT("PIXELFORMAT="), nPixelFormat );
		debugf( NAME_Init, TEXT("Using pixel format %i"), nPixelFormat );
		check(nPixelFormat);
		verify(SetPixelFormat( hDC, nPixelFormat, &pfd ));
		hRC = wglCreateContext( hDC );
		check(hRC);
		MakeCurrent();
		if( ShareLists && AllContexts.Num() )
			verify(wglShareLists(AllContexts(0),hRC)==1);
		AllContexts.AddItem(hRC);
#endif // USE_SDL

		// Get info and extensions.
	
		//PrintFormat( hDC, nPixelFormat );
		debugf( NAME_Init, TEXT("GL_VENDOR     : %s"), appFromAnsi((ANSICHAR*)glGetString(GL_VENDOR)) );
		debugf( NAME_Init, TEXT("GL_RENDERER   : %s"), appFromAnsi((ANSICHAR*)glGetString(GL_RENDERER)) );
		debugf( NAME_Init, TEXT("GL_VERSION    : %s"), appFromAnsi((ANSICHAR*)glGetString(GL_VERSION)) );
		debugf( NAME_Init, TEXT("GL_EXTENSIONS : %s"), ANSI_TO_TCHAR((ANSICHAR*)glGetString(GL_EXTENSIONS)) );

		FindProcs( 1 );

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

		SetGamma( Viewport->GetOuterUClient()->Brightness );

		UseVertexSpecular       = 1;
		AlwaysMipmap            = 0;
		SupportsTC              = UseS3TC;
		UseCVA	                = 1;
		UseDetailAlpha			= !DisableSpecialDT;

		SUPPORTS_GL_EXT_texture_env_combine |= SUPPORTS_GL_ARB_texture_env_combine;
			
		// Validate flags.
		if( !SUPPORTS_GL_ARB_multitexture )
			UseMultiTexture = 0;
		if( !SUPPORTS_GL_EXT_paletted_texture )
			UsePalette = 0;
		if( !SUPPORTS_GL_EXT_texture_compression_s3tc || !SUPPORTS_GL_ARB_texture_compression)
			SupportsTC = 0;
		if( !SUPPORTS_GL_EXT_texture_env_combine )
			DetailTextures = 0;
		if( !SUPPORTS_GL_EXT_compiled_vertex_array )
			UseCVA = 0;
		if( !SUPPORTS_GL_EXT_secondary_color )
			UseVertexSpecular = 0;
		if( !SUPPORTS_GL_EXT_texture_filter_anisotropic )
			MaxAnisotropy  = 0;
		if( !SUPPORTS_GL_SGIS_texture_lod )
			UseFilterSGIS = 0;
		if( !SUPPORTS_GL_EXT_texture_env_combine )
			UseDetailAlpha = 0;
		if( !SUPPORTS_GL_EXT_texture_lod_bias )
			LODBias = 0;

        VAROffset = VARMaxOffsets = 0;
        VARSpace = NULL;

#if MACOSX
		UseVAR = SUPPORTS_GL_APPLE_vertex_array_range;
#else
		UseVAR = SUPPORTS_GL_NV_vertex_array_range;
#endif

        int var = VARSize * 1024 * 1024;
        // !!! FIXME: Clamp to Apple's guidelines...

        if (UseVAR)
        {
            int minvar = VERTEX_ARRAY_SIZE * sizeof (FGLVertexData);
            if (var < minvar)
            {
                minvar = (minvar < 1024 * 1024) ? 1 : ((minvar / 1024) / 1024);
                debugf(NAME_Init, TEXT("OpenGL: VARSize must be >= %d. Disabling VAR."), minvar);
                UseVAR = 0;
            }
        }

        if (UseVAR)
        {
#ifdef WIN32
            if ((wglAllocateMemoryNV) && (wglFreeMemoryNV))
                VARSpace = (BYTE *) wglAllocateMemoryNV(var, 0.0f, 0.0f, 0.5f);
#elif MACOSX
                VARSpace = new BYTE[var];
#elif !defined(__EMSCRIPTEN__)
            if ((glXAllocateMemoryNV) && (glXFreeMemoryNV))
                VARSpace = (BYTE *) glXAllocateMemoryNV(var, 0.0f, 0.0f, 0.5f);
#endif

            if (VARSpace == NULL)
            {
	    	    debugf( NAME_Init, TEXT("OpenGL: Failed to allocate %d bytes for VAR"), (int) VARSize );
                UseVAR = 0;
            }
            else
            {
#if MACOSX
                glVertexArrayParameteriAPPLE(GL_VERTEX_ARRAY_STORAGE_HINT_APPLE, GL_STORAGE_SHARED_APPLE);
                if (SUPPORTS_GL_ATI_array_rev_comps_in_4_bytes)
                    glVertexArrayParameteriAPPLE( GL_ARRAY_REV_COMPS_IN_4_BYTES_ATI, GL_TRUE );
                glVertexArrayRangeAPPLE(var, VARSpace);
                glEnableClientState(GL_VERTEX_ARRAY_RANGE_APPLE);
#else
                glVertexArrayRangeNV(var, VARSpace);
                glEnableClientState(GL_VERTEX_ARRAY_RANGE_NV);
#endif

		        debugf( NAME_Init, TEXT("OpenGL: Using vertex_array_range") );
            }

            VARMaxOffsets = var / sizeof (FGLVertexData);
        }

        if (UseVAR)
            UseCVA = 0;
        else
            SUPPORTS_GL_ATI_array_rev_comps_in_4_bytes = 0;

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

		if ( SupportsTC )
		{
			debugf( TEXT("Trying to use S3TC extension.") );
			UseTNT = 0;
		}

		// Special treatment for texture size stuff.
		if (!GConfig->GetInt( *Section, TEXT("MinLogTextureSize"), MinLogTextureSize ))
			MinLogTextureSize=0;
		if (!GConfig->GetInt( *Section, TEXT("MaxLogTextureSize"), MaxLogTextureSize ))
			MaxLogTextureSize=8;

		INT MaxTextureSize;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &MaxTextureSize);		
		INT Dummy = -1;
		while (MaxTextureSize > 0)
		{
			MaxTextureSize >>= 1;
			Dummy++;
		}
		
		if ( (MaxLogTextureSize > Dummy) || (SupportsTC) )
			MaxLogTextureSize = Dummy;
		if ( (MinLogTextureSize < 2) || (SupportsTC) )
			MinLogTextureSize = 2;	

		if ( SupportsTC )
		{
			MaxLogUOverV = MaxLogTextureSize;
			MaxLogVOverU = MaxLogTextureSize;
		}
		else
		{
			MaxLogUOverV = 8;
			MaxLogVOverU = 8;
		}

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
#if FANCY_FISHEYE
		UseFisheye=1;
		FancyX = 256;
		FancyY = 256;
		while ( FancyX < NewX )
			FancyX *= 2;
		while ( FancyY < NewY )
			FancyY *= 2;
			
		glGenTextures( 1, &backbuffer );
		glBindTexture( GL_TEXTURE_2D, backbuffer );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, FancyX, FancyY, 0, GL_RGB, GL_UNSIGNED_BYTE, 0 );
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#endif		
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
		glPolygonOffset( -1.0f, -1.0 );
		if ( LODBias )
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
            #ifdef __EMSCRIPTEN__
            glGenerateMipmap(GL_TEXTURE_2D);
            #endif
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}

		glEnableClientState( GL_VERTEX_ARRAY );

        FGLVertexData *ptr = (UseVAR) ? (FGLVertexData *) VARSpace : VertexData;
		glVertexPointer( 3, GL_FLOAT, sizeof(FGLVertexData), &ptr[0].Vertex.x );
		myglEnableClientState( GL_TEXTURE_COORD_ARRAY );
		glTexCoordPointer( 2, GL_FLOAT, sizeof(FGLVertexData), &ptr[0].TexCoord.TMU[0].u );
		if ( UseMultiTexture )
		{
			glClientActiveTextureARB( GL_TEXTURE1_ARB );
			glTexCoordPointer( 2, GL_FLOAT, sizeof(FGLVertexData), &ptr[0].TexCoord.TMU[1].u );
			if ( TMUnits > 2 )
			{	
				glClientActiveTextureARB( GL_TEXTURE2_ARB );
				glTexCoordPointer( 2, GL_FLOAT, sizeof(FGLVertexData), &ptr[0].TexCoord.TMU[2].u );
			}
			glClientActiveTextureARB( GL_TEXTURE0_ARB );			
		}
		glColorPointer( 4, GL_UNSIGNED_BYTE, sizeof(FGLVertexData), &ptr[0].Color.color );
		if ( UseVertexSpecular )
			glSecondaryColorPointerEXT( 3, GL_UNSIGNED_BYTE, sizeof(FGLVertexData), &ptr[0].Color.specular );

		// Init variables.
		BufferedVerts      = 0;
		ColorArrayEnabled  = 0;
		RenderFog          = 0;

		CurrentPolyFlags   = static_cast<DWORD>(PF_Occlude);
		for (INT TMU=0; TMU<MAX_TMUNITS; TMU++)
			CurrentEnvFlags[TMU] = 0;

		// Remember fullscreenness.
		WasFullscreen = Fullscreen;
		return 1;

		unguard;
	}
	
	void Exit()
	{
		guard(UOpenGLRenderDevice::Exit);
		check(NumDevices>0);

		// Shut down RC.
		Flush( 0 );

#if USE_SDL
		UnsetRes();

		// Shut down global GL.
		if( --NumDevices==0 )
		{
			SharedBindMap->~TMap<QWORD,FCachedTexture>();
		}
#else
		if( hRC )
			UnsetRes();

		// vogel: UClient::Destroy is called before this gets called so hDC is invalid
		//OutputDebugString(TEXT("SetDeviceGammaRamp in Exit"));
		SetDeviceGammaRamp( GetDC( GetDesktopWindow() ), &OriginalRamp );

		// Shut down this GL context. May fail if window was already destroyed.
		if( hDC )
			ReleaseDC(hWnd,hDC);

		// Shut down global GL.
		if( --NumDevices==0 )
		{
#if DYNAMIC_BIND && 0 /* Broken on some drivers */
			// Free modules.
			if( hModuleGlMain )
				verify(FreeLibrary( hModuleGlMain ));
			if( hModuleGlGdi )
				verify(FreeLibrary( hModuleGlGdi ));
#endif
			if (SharedBindMap)
				SharedBindMap->~TMap<QWORD,FCachedTexture>();
			//AllContexts.~TArray<HGLRC>();
			AllContexts.Empty();
		}
#endif
		unguard;
	}
	
	void ShutdownAfterError()
	{
		guard(UOpenGLRenderDevice::ShutdownAfterError);
		debugf( NAME_Exit, TEXT("UOpenGLRenderDevice::ShutdownAfterError") );
		//ChangeDisplaySettings( NULL, 0 );
		unguard;
	}
	
	void Flush( UBOOL AllowPrecache )
	{
		guard(UOpenGLRenderDevice::Flush);
		TArray<GLuint> Binds;
		for( TMap<QWORD,FCachedTexture>::TIterator It(*BindMap); It; ++It )
			Binds.AddItem( It.Value().Id );
		BindMap->Empty();
		// vogel: FIXME: add 0 and AlphaTextureId
		if ( Binds.Num() )
			glDeleteTextures( Binds.Num(), (GLuint*)&Binds(0) );
		AllocatedTextures = 0;
		if( AllowPrecache && UsePrecache && !GIsEditor )
			PrecacheOnFlip = 1;
		SetGamma( Viewport->GetOuterUClient()->Brightness );
		unguard;
	}
	
	static QSORT_RETURN CDECL CompareRes( const FPlane* A, const FPlane* B )
	{
		return (QSORT_RETURN) ( (A->X-B->X)!=0.0 ? (A->X-B->X) : (A->Y-B->Y) );
	}

	struct DisplayMode { int w; int h; int refresh; };
	static void AddMode(const DisplayMode &bestMode, TArray<DisplayMode> &modeArray, const int w, const int h)
	{
		if ((modeArray.Num() == 0) || ((w != bestMode.w) || (h != bestMode.h)))
		{
			if ((w <= bestMode.w) && (h <= bestMode.h))
			{
				const DisplayMode m = { w, h, bestMode.refresh };
				modeArray.AddItem(m);
			}
		}
	}

	UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar )
	{
		guard(UOpenGLRenderDevice::Exec);
		if( URenderDevice::Exec( Cmd, Ar ) )
		{
			return 1;
		}
		if( ParseCommand(&Cmd,TEXT("DGL")) )
		{
			if( ParseCommand(&Cmd,TEXT("BUFFERTRIS")) ) 
			{
				BufferActorTris = !BufferActorTris;
				if ( !UseVertexSpecular )
					BufferActorTris = 0;
				debugf( TEXT("BUFFERTRIS [%i]"), BufferActorTris );
			}			
			if( ParseCommand(&Cmd,TEXT("CVA")) ) 
			{
				UseCVA = !UseCVA;
				if ( !SUPPORTS_GL_EXT_compiled_vertex_array )
					UseCVA = 0;				
				debugf( TEXT("CVA [%i]"), UseCVA );
			}
			if( ParseCommand(&Cmd,TEXT("TRILINEAR")) ) 
			{
				UseTrilinear = !UseTrilinear;
				debugf( TEXT("TRILINEAR [%i]"), UseTrilinear );
				Flush(1);
			}
			if( ParseCommand(&Cmd,TEXT("RGBA4444")) ) 
			{
				Use4444Textures = !Use4444Textures;
				debugf( TEXT("RGBA4444 [%i]"), Use4444Textures );
				Flush(1);
			}
   			if( ParseCommand(&Cmd,TEXT("TNTFIX")) ) 
			{
				UseTNT = !UseTNT;
				debugf( TEXT("TNTFIX [%i]"), UseTNT );
				Flush(1);
			}
			if( ParseCommand(&Cmd,TEXT("MULTITEX")) ) 
			{
				UseMultiTexture = !UseMultiTexture;				
				if( !SUPPORTS_GL_ARB_multitexture )
					UseMultiTexture = 0;
				if ( UseMultiTexture )
					glGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, &TMUnits );
				else
					TMUnits = 1;
				debugf( TEXT("MULTITEX [%i]"), UseMultiTexture );
			}
			if( ParseCommand(&Cmd,TEXT("DETAILTEX")) ) 
			{
				DetailTextures = !DetailTextures;
				if( !SUPPORTS_GL_EXT_texture_env_combine )
					DetailTextures = 0;
				debugf( TEXT("DETAILTEX [%i]"), DetailTextures );
			}
			if( ParseCommand(&Cmd,TEXT("DETAILALPHA")) ) 
			{
				UseDetailAlpha = !UseDetailAlpha;
				if( !SUPPORTS_GL_EXT_texture_env_combine )
					UseDetailAlpha = 0;
				debugf( TEXT("DETAILALPHA [%i]"), UseDetailAlpha );
			}
			if( ParseCommand(&Cmd,TEXT("BUILD")) ) 
			{
#if USE_SDL
				debugf( TEXT("OpenGL renderer built: %s %s"), __DATE__, __TIME__ );
#else
				debugf( TEXT("OpenGL renderer built: VOGEL FIXME") );
#endif
			}
#if FANCY_FISHEYE
			if( ParseCommand(&Cmd,TEXT("FISHEYE")) ) 
			{
				UseFisheye = !UseFisheye;
				debugf( TEXT("FISHEYE  [%i]"), UseFisheye );
			}
#endif
			return 1;	// vogel: FIXME
		}
		else if( ParseCommand(&Cmd,TEXT("GetRes")) )
		{
#if USE_SDL
			// Changing Resolutions:
			// Entries in the resolution box in the console is
			// apparently controled building a string of relevant 
			// resolutions and sending it to the engine via Ar.Log()

			// since we use a FBO and blit to whatever FULLSCREEN_DESKTOP offers, just list
			//  some sane resolutions up to and including the desktop resolution.
			//  !!! FIXME: this will ignore full retina resolutions on Mac OS X, since the
			//  !!! FIXME:  desktop mode is logically scaled down. If we start offering HIDPI
			//  !!! FIXME:  flags with FULLSCREEN_DESKTOP, elsewhere, fix this.

			FString Str = TEXT("");
			DisplayMode bestMode = { 1024, 768, 60 };
			SDL_DisplayMode sdlBestMode;
			SDL_zero(sdlBestMode);
			if (SDL_GetDesktopDisplayMode(0, &sdlBestMode) != -1)
			{
				bestMode.w = sdlBestMode.w;
				bestMode.h = sdlBestMode.h;
				bestMode.refresh = sdlBestMode.refresh_rate;
			}

			TArray<DisplayMode> modeArray;
			AddMode(bestMode, modeArray, bestMode.w, bestMode.h);   // whatever this machine has.
			AddMode(bestMode, modeArray, 2880, 1800);  // MacBook Pro Retina
			AddMode(bestMode, modeArray, 1920, 1200);  // Apple Cinema Display 23", highest MacBook pro non-Retina setting.
			AddMode(bestMode, modeArray, 1920, 1080);  // 1080p
			AddMode(bestMode, modeArray, 1680, 1050);  // someone requested this on Twitter.
			AddMode(bestMode, modeArray, 1440, 900);   // MacBook Pro non-Retina default.
			AddMode(bestMode, modeArray, 1280, 720);   // 720p
			AddMode(bestMode, modeArray, 1024, 768);   // Some old popular resolutions...
			AddMode(bestMode, modeArray, 800, 600);
			AddMode(bestMode, modeArray, 640, 480);

			for (TArray<DisplayMode>::TIterator it(modeArray); it; ++it)
			{
				const DisplayMode &mode = *it;
				Str += FString::Printf( TEXT("%ix%i "), mode.w, mode.h);
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
		BindCycles = ImageCycles = ComplexCycles = GouraudCycles = TileCycles = 0;
		++LockCount;

		// Make this context current.
		MakeCurrent();

		// Clear the Z buffer if needed.
		glClearColor( ScreenClear.X, ScreenClear.Y, ScreenClear.Z, ScreenClear.W );
		glClearDepth( 1.0 );
		glDepthRange( 0.0, 1.0 );
		SetBlend( static_cast<DWORD>(PF_Occlude) );
		glClear( GL_DEPTH_BUFFER_BIT|((RenderLockFlags&LOCKR_ClearScreen)?GL_COLOR_BUFFER_BIT:0) );

		glDepthFunc( GL_LEQUAL );

		// Remember stuff.
		FlashScale = InFlashScale;
		FlashFog   = InFlashFog;
		HitData    = InHitData;
		HitSize    = InHitSize;
		if( HitData )
		{
			#ifdef __EMSCRIPTEN__
			appErrorf(TEXT("Uhoh, no glSelectBuffer in Emscripten!"));
			#else
			*HitSize = 0;
			if( !GLHitData.Num() )
				GLHitData.Add( 16384 );
			glSelectBuffer( GLHitData.Num(), (GLuint*)&GLHitData(0) );
			glRenderMode( GL_SELECT );
			glInitNames();
			#endif
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
			glOrtho( -RProjZ / 2.0, +RProjZ / 2.0, -Aspect*RProjZ / 2.0, +Aspect*RProjZ / 2.0, 1.0 / 2.0, 32768.0 );
		else			
			glFrustum( -RProjZ / 2.0, +RProjZ / 2.0, -Aspect*RProjZ / 2.0, +Aspect*RProjZ / 2.0, 1.0 / 2.0, 32768.0 );
			
		// Set clip planes.
		#ifndef __EMSCRIPTEN__
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
		#endif
		unguard;
	}

#if FANCY_FISHEYE
	void Transform(float x, float y, float *u, float *v)
	{
		FLOAT MaxU = Viewport->SizeX / (float) FancyX;
		FLOAT MaxV = Viewport->SizeY / (float) FancyY;
		
		FLOAT px = (x+1);
		FLOAT py = (y+1);

#if 1
		*u = (px + 0.05 * sin( 5 * PI * px + Timer) ) * MaxU / 2;
		*v = (py + 0.05 * cos( 5 * PI * py + Timer) ) * MaxV / 2;
#else
		float r = sqrt( x*x + y*y );	
		float theta = atan2(y,x);
		r = sin(PI / 2.0 * r);

		*u = (r * cos(theta) / 2 + 0.5) * MaxU;
		*v = (r * sin(theta) / 2 + 0.5) * MaxV;
#endif
	}
#endif
	
	void Unlock( UBOOL Blit )
	{
		guard(UOpenGLRenderDevice::Unlock);
		EndBuffering();

		// Unlock and render.
		check(LockCount==1);
		//glFlush();
		if( Blit )
	       	{
#if FANCY_FISHEYE
			if ( UseFisheye )
			{		
				glBindTexture( GL_TEXTURE_2D, backbuffer );
				glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, (GLuint) Viewport->SizeX, (GLuint) Viewport->SizeY );

				SetBlend(0);			

				glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
				
				FLOAT x,y,u,v;
				FLOAT n  = 0.05f;
				FLOAT A  = - Viewport->SizeY / (FLOAT) Viewport->SizeX ;
				FLOAT RZ = 1.0f / RProjZ;

				INT Index = 0;
				for ( x = -1.0f; x < 1.0f; x += n )
				{
					for ( y = -1.0f; y < 1.0f; y += n )
					{      
						Transform( x, y, &u, &v );
						VertexData[Index].TexCoord.TMU[0].u = u;
						VertexData[Index].TexCoord.TMU[0].v = v;
						VertexData[Index].Vertex.x  = x;
						VertexData[Index].Vertex.y  = y * A;
						VertexData[Index].Vertex.z  = RZ; 
						Index++;				
	
						Transform( x, y+n , &u, &v );
						VertexData[Index].TexCoord.TMU[0].u = u;
						VertexData[Index].TexCoord.TMU[0].v = v;
						VertexData[Index].Vertex.x  = x;
						VertexData[Index].Vertex.y  = (y+n) * A;
						VertexData[Index].Vertex.z  = RZ; 
						Index++;

						Transform( x+n, y+n , &u, &v );
						VertexData[Index].TexCoord.TMU[0].u = u;
						VertexData[Index].TexCoord.TMU[0].v = v;
						VertexData[Index].Vertex.x  = (x+n);
						VertexData[Index].Vertex.y  = (y+n) * A;
						VertexData[Index].Vertex.z  = RZ; 
						Index++;

						Transform( x+n, y , &u, &v );
						VertexData[Index].TexCoord.TMU[0].u = u;
						VertexData[Index].TexCoord.TMU[0].v = v;
						VertexData[Index].Vertex.x  = (x+n);
						VertexData[Index].Vertex.y  = y * A;
						VertexData[Index].Vertex.z  = RZ; 			
						Index++;
					}
				}

				glDisable(GL_DEPTH_TEST);
				DrawArrays( GL_QUADS, 0, Index );
				glEnable(GL_DEPTH_TEST);
				Timer += 0.02;
			}
#endif
			Check(TEXT("please report this bug"));
#if USE_SDL
			SDL_GL_SwapWindow((SDL_Window *) Viewport->GetWindow());
#else
			verify(SwapBuffers( hDC ));
#endif
		}
		--LockCount;

		// Hits.
		#ifndef __EMSCRIPTEN__
		if( HitData )
		{
			INT Records = glRenderMode( GL_RENDER );
			INT* Ptr = &GLHitData(0);
			DWORD BestDepth = static_cast<DWORD>(MAXDWORD);
			for( INT i=0; i<Records; i++ )
			{
				INT   NameCount = *Ptr++;
				DWORD MinDepth  = *Ptr++;
				DWORD MaxDepth  = *Ptr++;
				if( MinDepth<=BestDepth )
				{
					BestDepth = MinDepth;
					*HitSize = 0;
					INT x;
					for( x=0; x<NameCount; )
					{
						INT Count = Ptr[x++];
						for( INT j=0; j<Count; j+=4 )
							*(INT*)(HitData+*HitSize+j) = Ptr[x++];
						*HitSize += Count;
					}
					check(x==NameCount);
				}
				Ptr += NameCount;
				(void)MaxDepth;
			}
			for( INT i=0; i<4; i++ )
				glDisable( GL_CLIP_PLANE0+i );
		}
		#endif

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
			myglEnableClientState(GL_TEXTURE_COORD_ARRAY );

			if ( i != 0 )
			 	SetTexEnv( i, MultiPass.TMU[i].PolyFlags );

			SetTexture( i, *MultiPass.TMU[i].Info, MultiPass.TMU[i].PolyFlags, MultiPass.TMU[i].PanBias );

			Color.X *= TexInfo[i].ColorRenorm.X;
			Color.Y *= TexInfo[i].ColorRenorm.Y;
			Color.Z *= TexInfo[i].ColorRenorm.Z;
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
					VertexData[Index].TexCoord.TMU[t].u = (U-TexInfo[t].UPan)*TexInfo[t].UMult;
					VertexData[Index].TexCoord.TMU[t].v = (V-TexInfo[t].VPan)*TexInfo[t].VMult;
				}	
				Index++;
			}			         
		}

		INT Start = 0;
		for( FSavedPoly *Poly=MultiPass.Poly; Poly; Poly=Poly->Next )
		{
			DrawArrays( GL_TRIANGLE_FAN, Start, Poly->NumPts );
			Start += Poly->NumPts;
		}

		for (i=1; i<PassCount; i++)
		{
			glActiveTextureARB( GL_TEXTURE0_ARB + i );
			glClientActiveTextureARB( GL_TEXTURE0_ARB + i );
			glDisable( GL_TEXTURE_2D );
			myglDisableClientState(GL_TEXTURE_COORD_ARRAY );
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
		clockFast(ComplexCycles);
		FLOAT UDot = Facet.MapCoords.XAxis | Facet.MapCoords.Origin;
		FLOAT VDot = Facet.MapCoords.YAxis | Facet.MapCoords.Origin;
		INT Index  = 0;
		UBOOL ForceSingle = false;
		UBOOL Masked = Surface.PolyFlags & PF_Masked;

		EndBuffering();		// vogel: might have still been locked (can happen!)

		// Buffer "static" geometry.
		for( FSavedPoly* Poly=Facet.Polys; Poly; Poly=Poly->Next )
       		{
       			for( INT i=0; i<Poly->NumPts; i++ )
       			{
				MapDotArray[Index].u = (Facet.MapCoords.XAxis | Poly->Pts[i]->Point) - UDot;
				MapDotArray[Index].v = (Facet.MapCoords.YAxis | Poly->Pts[i]->Point) - VDot;
				
				VertexData[Index].Vertex.x  = Poly->Pts[i]->Point.X;
				VertexData[Index].Vertex.y  = Poly->Pts[i]->Point.Y;
				VertexData[Index].Vertex.z  = Poly->Pts[i]->Point.Z;
				Index++;
			}
		}
		if ( UseCVA )
		{
			myglDisableClientState( GL_TEXTURE_COORD_ARRAY );
			glLockArraysEXT( 0, Index );
			myglEnableClientState( GL_TEXTURE_COORD_ARRAY );
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
			myglDisableClientState( GL_TEXTURE_COORD_ARRAY );
			glUnlockArraysEXT();
			myglEnableClientState( GL_TEXTURE_COORD_ARRAY );
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
							myglEnableClientState(GL_TEXTURE_COORD_ARRAY );

							SetTexture( 1, *Surface.DetailTexture, PF_Modulated, 0 );
							SetTexEnv( 1, PF_Memorized );
	  					}
						else
						{
							SetTexture( 0, *Surface.DetailTexture, PF_Modulated, 0 );
							SetTexEnv( 0, PF_Memorized );
							myglEnableClientState( GL_COLOR_ARRAY );
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
								VertexData[Index].TexCoord.TMU[0].u = Poly->Pts[i]->Point.Z * RNearZ;
								VertexData[Index].TexCoord.TMU[0].v = Poly->Pts[i]->Point.Z * RNearZ;
								VertexData[Index].TexCoord.TMU[1].u = (U-UDot-TexInfo[1].UPan)*TexInfo[1].UMult;
								VertexData[Index].TexCoord.TMU[1].v = (V-VDot-TexInfo[1].VPan)*TexInfo[1].VMult;
							}
							else
							{
								VertexData[Index].TexCoord.TMU[0].u = (U-UDot-TexInfo[0].UPan)*TexInfo[0].UMult;
								VertexData[Index].TexCoord.TMU[0].v = (V-VDot-TexInfo[0].VPan)*TexInfo[0].VMult;
								// DWORD A = Min<DWORD>( appRound(100.f * (NearZ / Poly->Pts[i]->Point.Z - 1.f)), 255 );
								BYTE C = (BYTE) appRound((1 - Clamp( Poly->Pts[i]->Point.Z, 0.0f, NearZ ) / NearZ ) * 128);
								VertexData[Index].Color.color  = 0x00808080 | (C << 24); 
							}
							VertexData[Index].Vertex.x     = Poly->Pts[i]->Point.X;
							VertexData[Index].Vertex.y     = Poly->Pts[i]->Point.Y;
							VertexData[Index].Vertex.z     = Poly->Pts[i]->Point.Z;
							Index++;
						}
					}
					DrawArrays( GL_TRIANGLE_FAN, Start, Index - Start );
				}
			}
			if ( IsDetailing )
			{
				if ( UseDetailAlpha )
				{
					SetTexEnv( 1, PF_Modulated );
					glDisable( GL_TEXTURE_2D );
					myglDisableClientState(GL_TEXTURE_COORD_ARRAY );
					glActiveTextureARB( GL_TEXTURE0_ARB );
					glClientActiveTextureARB( GL_TEXTURE0_ARB );
					glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
				}
				else
				{
					myglDisableClientState(GL_COLOR_ARRAY );
				}
				SetTexEnv( 0, PF_Modulated );
				//glDisable(GL_POLYGON_OFFSET_FILL);
			}
		}


		// UnrealEd selection.
		if( (Surface.PolyFlags & PF_Selected) && GIsEditor )
		{
			SetNoTexture( 0 );
			SetBlend( PF_Highlighted );
			glBegin( GL_TRIANGLE_FAN );
			glColor4f( 0.0, 0.0, 0.5, 0.5 );			
			for( FSavedPoly* Poly=Facet.Polys; Poly; Poly=Poly->Next )
			{
				glBegin( GL_TRIANGLE_FAN );
				for( INT i=0; i<Poly->NumPts; i++ )
					glVertex3fv( &Poly->Pts[i]->Point.X );
				glEnd();
			}
			glEnd();
		}

		if( Masked )
			glDepthFunc( GL_LEQUAL );

		unclockFast(ComplexCycles);
		unguard;
	}
	
	void DrawGouraudPolygonOld( FSceneNode* Frame, FTextureInfo& Info, FTransTexture** Pts, INT NumPts, DWORD PolyFlags, FSpanBuffer* Span )
	{
		guard(UOpenGLRenderDevice::DrawGouraudPolygonOld);      
		clockFast(GouraudCycles);
	    
		INT Index     = 0;
		UBOOL Enabled = false;

		SetBlend( PolyFlags );
		SetTexture( 0, Info, PolyFlags, 0 );
		if( PolyFlags & PF_Modulated )
			glColor4f( TexInfo[0].ColorNorm.X, TexInfo[0].ColorNorm.Y, TexInfo[0].ColorNorm.Z, 1 );
		else
		{
			myglEnableClientState( GL_COLOR_ARRAY );
			Enabled = true;
		}
		
		for( INT i=0; i<NumPts; i++ )
		{
			FTransTexture* P = Pts[i];
			VertexData[Index].TexCoord.TMU[0].u = P->U*TexInfo[0].UMult;
			VertexData[Index].TexCoord.TMU[0].v = P->V*TexInfo[0].VMult;
			if( Enabled )
				VertexData[Index].Color.color = RGBA_MAKE(
					appRound( 255 * P->Light.X*TexInfo[0].ColorNorm.X ), 
					appRound( 255 * P->Light.Y*TexInfo[0].ColorNorm.Y ), 
					appRound( 255 * P->Light.Z*TexInfo[0].ColorNorm.Z ),
					255);
			VertexData[Index].Vertex.x  = P->Point.X;
			VertexData[Index].Vertex.y  = P->Point.Y;
			VertexData[Index].Vertex.z  = P->Point.Z;
			Index++;
		}

		DrawArrays( GL_TRIANGLE_FAN, 0, Index );

		if( (PolyFlags & (PF_RenderFog|PF_Translucent|PF_Modulated))==PF_RenderFog )
		{	
			Index = 0;
			if ( !Enabled )
			{
				myglEnableClientState( GL_COLOR_ARRAY );
				Enabled = true;	
			}	 		
			SetNoTexture( 0 );
			SetBlend( PF_Highlighted );
			for( INT i=0; i<NumPts; i++ )
			{
				FTransTexture* P = Pts[i];
		  		VertexData[Index].Color.color = RGBA_MAKE(
					appRound( 255 * P->Fog.X ), 
					appRound( 255 * P->Fog.Y ),
					appRound( 255 * P->Fog.Z ),
					appRound( 255 * P->Fog.W ));			
				Index++;
			}

			DrawArrays( GL_TRIANGLE_FAN, 0, Index );

		}
		
		if( Enabled )
			myglDisableClientState( GL_COLOR_ARRAY );

		unclockFast(GouraudCycles);
		unguard;
	}

	void EndBuffering()
	{
		if ( BufferedVerts > 0 )
		{
			if ( RenderFog )
			{
				myglEnableClientState( GL_SECONDARY_COLOR_ARRAY_EXT );
				glEnable( GL_COLOR_SUM_EXT );	
			}

			// Actually render the triangles.
			DrawArrays( GL_TRIANGLES, 0, BufferedVerts );

			if ( ColorArrayEnabled )
				myglDisableClientState( GL_COLOR_ARRAY );
			if ( RenderFog )
			{
				myglDisableClientState( GL_SECONDARY_COLOR_ARRAY_EXT );
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
		clockFast(GouraudCycles);

		static DWORD lastPolyFlags = 0; // vogel: can't use CurrentFlags as SetBlend modifies it (especially PF_RenderFog)

		if ( ! ((lastPolyFlags==PolyFlags) && (TexInfo[0].CurrentCacheID == Info.CacheID) &&
			(BufferedVerts+NumPts < VERTEX_ARRAY_SIZE-1) && (BufferedVerts>0) ) )
		// flush drawing and set the state!
		{
			SetBlend( PolyFlags ); // SetBlend will call EndBuffering() to flush the vertex array
			SetTexture( 0, Info, PolyFlags, 0 );
			if( PolyFlags & PF_Modulated )
			{
				glColor4f( TexInfo[0].ColorNorm.X, TexInfo[0].ColorNorm.Y, TexInfo[0].ColorNorm.Z, 1 );
				ColorArrayEnabled = false;
			}
			else
			{
				myglEnableClientState( GL_COLOR_ARRAY );
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
			VertexData[BufferedVerts].TexCoord.TMU[0].u = P->U*TexInfo[0].UMult;
			VertexData[BufferedVerts].TexCoord.TMU[0].v = P->V*TexInfo[0].VMult;
			if ( RenderFog  && ColorArrayEnabled )
			{
				VertexData[BufferedVerts].Color.color = RGBA_MAKE(
					appRound( 255 * P->Light.X*TexInfo[0].ColorNorm.X * (1 - P->Fog.W)), 
					appRound( 255 * P->Light.Y*TexInfo[0].ColorNorm.Y * (1 - P->Fog.W) ), 
					appRound( 255 * P->Light.Z*TexInfo[0].ColorNorm.Z * (1 - P->Fog.W) ),
					255);

				VertexData[BufferedVerts].Color.specular = RGBA_MAKE(
					appRound( 255 * P->Fog.X ), 
					appRound( 255 * P->Fog.Y ),
					appRound( 255 * P->Fog.Z ),
					0 );	
			}
			else if ( RenderFog )
			{
				VertexData[BufferedVerts].Color.color = RGBA_MAKE(
					appRound( 255 * TexInfo[0].ColorNorm.X * (1 - P->Fog.W)), 
					appRound( 255 * TexInfo[0].ColorNorm.Y * (1 - P->Fog.W) ), 
					appRound( 255 * TexInfo[0].ColorNorm.Z * (1 - P->Fog.W) ),
					255);

				VertexData[BufferedVerts].Color.specular = RGBA_MAKE(
					appRound( 255 * P->Fog.X ), 
					appRound( 255 * P->Fog.Y ),
					appRound( 255 * P->Fog.Z ),
					0 );
				ColorArrayEnabled = true;
				myglEnableClientState( GL_COLOR_ARRAY );
			}
			else if ( ColorArrayEnabled )
			{
				VertexData[BufferedVerts].Color.color = RGBA_MAKE(
					appRound( 255 * P->Light.X*TexInfo[0].ColorNorm.X ), 
					appRound( 255 * P->Light.Y*TexInfo[0].ColorNorm.Y ), 
					appRound( 255 * P->Light.Z*TexInfo[0].ColorNorm.Z ),
					255);
			}				
			VertexData[BufferedVerts].Vertex.x  = P->Point.X;
			VertexData[BufferedVerts].Vertex.y  = P->Point.Y;
			VertexData[BufferedVerts].Vertex.z  = P->Point.Z;
			BufferedVerts++;
		}
		unclockFast(GouraudCycles);
		unguard;
	}
	
	void DrawTile( FSceneNode* Frame, FTextureInfo& Info, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, class FSpanBuffer* Span, FLOAT Z, FPlane Color, FPlane Fog, DWORD PolyFlags )
	{
		guard(UOpenGLRenderDevice::DrawTile);
		clockFast(TileCycles);
		if( Info.Palette && Info.Palette[128].A!=255 && !(PolyFlags&PF_Translucent) )
			PolyFlags |= PF_Highlighted;
		SetBlend( PolyFlags );
		SetTexture( 0, Info, PolyFlags, 0 );
		Color.X *= TexInfo[0].ColorNorm.X;
		Color.Y *= TexInfo[0].ColorNorm.Y;
		Color.Z *= TexInfo[0].ColorNorm.Z;
		Color.W  = 1;	
		if ( PolyFlags & PF_Modulated )
			glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
		else
			glColor4fv( &Color.X );

		glBegin( GL_TRIANGLE_FAN );
		glTexCoord2f( (U   )*TexInfo[0].UMult, (V   )*TexInfo[0].VMult ); glVertex3f( RFX2*Z*(X   -Frame->FX2), RFY2*Z*(Y   -Frame->FY2), Z );
		glTexCoord2f( (U+UL)*TexInfo[0].UMult, (V   )*TexInfo[0].VMult ); glVertex3f( RFX2*Z*(X+XL-Frame->FX2), RFY2*Z*(Y   -Frame->FY2), Z );
		glTexCoord2f( (U+UL)*TexInfo[0].UMult, (V+VL)*TexInfo[0].VMult ); glVertex3f( RFX2*Z*(X+XL-Frame->FX2), RFY2*Z*(Y+YL-Frame->FY2), Z );
		glTexCoord2f( (U   )*TexInfo[0].UMult, (V+VL)*TexInfo[0].VMult ); glVertex3f( RFX2*Z*(X   -Frame->FX2), RFY2*Z*(Y+YL-Frame->FY2), Z );
		glEnd();
		unclockFast(TileCycles);
		unguard;
	}
	
	void Draw2DLine( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2 )
	{
		guard(UOpenGLRenderDevice::Draw2DLine);
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
		glBegin( GL_TRIANGLE_FAN );
		glVertex3f( RFX2*Z*(X1-Frame->FX2), RFY2*Z*(Y1-Frame->FY2), 0.5 );
		glVertex3f( RFX2*Z*(X2-Frame->FX2), RFY2*Z*(Y1-Frame->FY2), 0.5 );
		glVertex3f( RFX2*Z*(X2-Frame->FX2), RFY2*Z*(Y2-Frame->FY2), 0.5 );
		glVertex3f( RFX2*Z*(X1-Frame->FX2), RFY2*Z*(Y2-Frame->FY2), 0.5 );
		glEnd();
		unguard;
	}
	
	void ClearZ( FSceneNode* Frame )
	{
		guard(UOpenGLRenderDevice::ClearZ);
		SetBlend( static_cast<DWORD>(PF_Occlude) );
		glClear( GL_DEPTH_BUFFER_BIT );
		unguard;
	}
	
	void PushHit( const BYTE* Data, INT Count )
	{
		guard(UOpenGLRenderDevice::PushHit);
		#ifndef __EMSCRIPTEN__
		glPushName( Count );
		for( INT i=0; i<Count; i+=4 )
			glPushName( *(INT*)(Data+i) );
		#endif
		unguard;
	}
	
	void PopHit( INT Count, UBOOL bForce )
	{
		guard(UOpenGLRenderDevice::PopHit);
		#ifndef __EMSCRIPTEN__
		glPopName();
		for( INT i=0; i<Count; i+=4 )
			glPopName();
		//!!implement bforce
		#endif
		unguard;
	}
	
	void GetStats( TCHAR* Result )
	{
		guard(UOpenGLRenderDevice::GetStats);
		appSprintf
		(
			Result,
			TEXT("OpenGL stats: Bind=%04.1f Image=%04.1f Complex=%04.1f Gouraud=%04.1f Tile=%04.1f"),
			GSecondsPerCycle*1000 * BindCycles,
			GSecondsPerCycle*1000 * ImageCycles,
			GSecondsPerCycle*1000 * ComplexCycles,
			GSecondsPerCycle*1000 * GouraudCycles,
			GSecondsPerCycle*1000 * TileCycles
		);
		unguard;
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
#ifdef WIN32
HGLRC		UOpenGLRenderDevice::hCurrentRC    = NULL;
HMODULE		UOpenGLRenderDevice::hModuleGlMain = NULL;
HMODULE		UOpenGLRenderDevice::hModuleGlGdi  = NULL;
TArray<HGLRC>	UOpenGLRenderDevice::AllContexts;
#else
UBOOL 		UOpenGLRenderDevice::GLLoaded	   = false;
#endif
TMap<QWORD,UOpenGLRenderDevice::FCachedTexture> *UOpenGLRenderDevice::SharedBindMap = NULL;

// OpenGL function pointers.
#define GL_EXT(name) UBOOL UOpenGLRenderDevice::SUPPORTS##name=0;
#ifdef __EMSCRIPTEN__
#define GL_PROC(ext,ret,func,parms)
#else
#define GL_PROC(ext,ret,func,parms) ret (STDCALL *UOpenGLRenderDevice::func)parms;
#endif
#include "OpenGLFuncs.h"
#undef GL_EXT
#undef GL_PROC

void autoInitializeRegistrantsOpenGLDrv(void)
{
    UOpenGLRenderDevice::StaticClass();
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

