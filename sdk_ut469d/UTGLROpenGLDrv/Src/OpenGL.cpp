/*=============================================================================
	OpenGL.cpp: Unreal OpenGL support implementation for Windows and Linux.
	Copyright 1999 Epic Games, Inc. All Rights Reserved.

	OpenGL renderer by Daniel Vogel <vogel@lokigames.com>
	Loki Software, Inc.

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
	* (NOT ALWAYS) Fixed Windows bitdepth switching (Daniel Vogel)

	* Various modifications and additions by Chris Dohnal
	* Initial TruForm based on TruForm renderer modifications by NitroGL
	* Additional TruForm and Deus Ex updates by Leonhard Gruenschloss
	* Various modifications and additions by Smirftsch / Oldunreal

	* Unreal Tournament Version 469 port (*!!)
	* Font rendering fixes by Fernando Velazquez
	* New texture upload system (+ BPTC support) by Fernando Velazquez
	* ARB shader writer by Fernando Velazquez
	* Separated into render paths by Fernando Velazquez
	** See: OpenGL_ARBRendering.cpp, OpenGL_LegacyRendering.cpp, OpenGL_HitTesting for more info.
	* Framebuffer usage by Fernando Velazquez


	UseTrilinear	whether to use trilinear filtering
	UseHDTextures	whether to use compressed textures
	MaxAnisotropy	maximum level of anisotropy used
	LODBias			texture lod bias
	GammaOffset		offset for the gamma correction
	AlwaysMipmap	generate mipmaps for textures without mips


TODO:
	- DOCUMENTATION!!! (especially all subtle assumptions)

=============================================================================*/

#include "OpenGLDrv.h"
#include "UTGLROpenGL.h"

#if __UNIX__
	#include <dlfcn.h>
#endif

/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

const TCHAR *UOpenGLRenderDevice::g_pSection = TEXT("OpenGLDrv.OpenGLRenderDevice");
static BYTE InternalContextType = CONTEXTTYPE_DETECT;
UBOOL UOpenGLRenderDevice::GUsingGammaRamp = 0;

UOpenGLRenderDevice::SetGLProc UOpenGLRenderDevice::SetGL[CONTEXTTYPE_MAX] = 
{
	&UOpenGLRenderDevice::SetGLError,
	&UOpenGLRenderDevice::SetGL1,
	&UOpenGLRenderDevice::SetGL3,
	&UOpenGLRenderDevice::SetGLError,
	&UOpenGLRenderDevice::SetGLError
};

#ifdef __UNIX__
static SDL_GLContext Context = nullptr;

static void CheckContext( SDL_Window* Window=nullptr)
{
	guard(CheckContext);
	
	return; //For now

	if ( !Context )
	{
		Context = SDL_GL_GetCurrentContext();
		debugf( NAME_Init, TEXT("SDL2 OpenGL context query %s"), Context ? TEXT("success") : TEXT("failed") );
	}
	else
	{
		SDL_GLContext Current = SDL_GL_GetCurrentContext();
		if ( Current != Context )
		{
			if ( Current )
			{	
				debugf( NAME_Init, TEXT("SDL2 OpenGL context replaced") );
				FOpenGLBase::InitProcs(true);
				FOpenGLBase::InitCapabilities(true);
				Context = Current;
			}
			else if ( Window )
			{
				debugf( NAME_Init, TEXT("SDL2 OpenGL context lost") );
				SDL_GL_MakeCurrent(Window,Context);
			}
		}
	}
	
	if ( Context && Window && (Window != SDL_GL_GetCurrentWindow()) )
		SDL_GL_MakeCurrent(Window,Context);

	unguard;
}

#endif

static UEnum* ContextEnum = nullptr;

static void SetPreferredGPU( UBOOL Preferred)
{
	void* Module = nullptr;
#if __WIN32__
	Module = (void*)GetModuleHandleA(nullptr);
#elif __UNIX__
	Module = dlopen(nullptr, RTLD_LOCAL|RTLD_NOW);
#endif

	if ( Module )
	{
		INT* NV = (INT*)appGetDllExport(Module, TEXT("NvOptimusEnablement"));
		if ( NV )
			*NV = !!Preferred;

		INT* AMD = (INT*)appGetDllExport(Module, TEXT("AmdPowerXpressRequestHighPerformance"));
		if ( AMD )
			*AMD = !!Preferred;

		debugfSlow(NAME_Init, TEXT("NV=%d, AMD=%d"), NV, AMD);
	}
}

/*-----------------------------------------------------------------------------
	OpenGLDrv.
-----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(UOpenGLRenderDevice);


int UOpenGLRenderDevice::dbgPrintf(const char *format, ...) {
	const unsigned int DBG_PRINT_BUF_SIZE = 1024;
	char dbgPrintBuf[DBG_PRINT_BUF_SIZE];
	va_list vaArgs;
	int iRet = 0;

	va_start(vaArgs, format);

#if __WIN32__
#pragma warning(push)
#pragma warning(disable : 4996)
	iRet = vsnprintf(dbgPrintBuf, DBG_PRINT_BUF_SIZE, format, vaArgs);
	dbgPrintBuf[DBG_PRINT_BUF_SIZE - 1] = '\0';
#pragma warning(pop)

	OutputDebugStringW(appFromAnsi(dbgPrintBuf));
#endif

	va_end(vaArgs);

	return iRet;
}


void UOpenGLRenderDevice::StaticConstructor() {
	guard(UOpenGLRenderDevice::StaticConstructor);

#ifdef UTGLR_DX_BUILD
	const UBOOL UTGLR_DEFAULT_OneXBlending = 1;
#else
	const UBOOL UTGLR_DEFAULT_OneXBlending = 0;
#endif

#if defined UTGLR_DX_BUILD || defined UTGLR_RUNE_BUILD
	const UBOOL UTGLR_DEFAULT_UseHDTextures = 0;
#else
	const UBOOL UTGLR_DEFAULT_UseHDTextures = 1;
#endif

#define CPP_PROPERTY_LOCAL(_name) _name, CPP_PROPERTY(_name)

	//Set parameter defaults and add parameters
	SC_AddFloatConfigParam(TEXT("LODBias"), CPP_PROPERTY_LOCAL(LODBias), 0.0f);
	SC_AddFloatConfigParam(TEXT("GammaOffset"), CPP_PROPERTY_LOCAL(GammaOffset), 0.0f);
	SC_AddFloatConfigParam(TEXT("GammaOffsetRed"), CPP_PROPERTY_LOCAL(GammaOffsetRed), 0.0f);
	SC_AddFloatConfigParam(TEXT("GammaOffsetGreen"), CPP_PROPERTY_LOCAL(GammaOffsetGreen), 0.0f);
	SC_AddFloatConfigParam(TEXT("GammaOffsetBlue"), CPP_PROPERTY_LOCAL(GammaOffsetBlue), 0.0f);
	SC_AddBoolConfigParam(1,  TEXT("GammaCorrectScreenshots"), CPP_PROPERTY_LOCAL(GammaCorrectScreenshots), 0);
	SC_AddBoolConfigParam(0,  TEXT("OneXBlending"), CPP_PROPERTY_LOCAL(OneXBlending), UTGLR_DEFAULT_OneXBlending);
	SC_AddBoolConfigParam(4,  TEXT("AlwaysMipmap"), CPP_PROPERTY_LOCAL(AlwaysMipmap), 0);
	SC_AddBoolConfigParam(3,  TEXT("UsePrecache"), CPP_PROPERTY_LOCAL(UsePrecache), 0);
	SC_AddBoolConfigParam(2,  TEXT("UseTrilinear"), CPP_PROPERTY_LOCAL(UseTrilinear), 1);
	SC_AddBoolConfigParam(1,  TEXT("UseHDTextures"), CPP_PROPERTY_LOCAL(UseHDTextures), UTGLR_DEFAULT_UseHDTextures);
	SC_AddBoolConfigParam(0,  TEXT("Use16BitTextures"), CPP_PROPERTY_LOCAL(Use16BitTextures), 0);
	SC_AddIntConfigParam(TEXT("MaxAnisotropy"), CPP_PROPERTY_LOCAL(MaxAnisotropy), 16);
	SC_AddBoolConfigParam(0,  TEXT("ForceNoSmooth"), CPP_PROPERTY_LOCAL(ForceNoSmooth), 0);
	SC_AddIntConfigParam(TEXT("RefreshRate"), CPP_PROPERTY_LOCAL(RefreshRate), 0);
	SC_AddIntConfigParam(TEXT("DetailMax"), CPP_PROPERTY_LOCAL(DetailMax), 3);
	SC_AddBoolConfigParam(2,  TEXT("ShaderAlphaHUD"), CPP_PROPERTY_LOCAL(ShaderAlphaHUD), 0);
	SC_AddBoolConfigParam(1,  TEXT("UseDrawGouraud469"), CPP_PROPERTY_LOCAL(UseDrawGouraud469), 1);
	SC_AddBoolConfigParam(0,  TEXT("UseStaticGeometry"), CPP_PROPERTY_LOCAL(UseStaticGeometry), 0);
	SC_AddIntConfigParam(TEXT("SwapInterval"), CPP_PROPERTY_LOCAL(SwapInterval), -1);
	SC_AddBoolConfigParam(2, TEXT("UseShaderGamma"), CPP_PROPERTY_LOCAL(UseShaderGamma), 0);
#if UTGLR_USES_SCENENODE_HACK
	SC_AddBoolConfigParam(1,  TEXT("SceneNodeHack"), CPP_PROPERTY_LOCAL(SceneNodeHack), 1);
#endif
	SC_AddBoolConfigParam(0,  TEXT("UseAA"), CPP_PROPERTY_LOCAL(UseAA), 0);
	SC_AddIntConfigParam(TEXT("NumAASamples"), CPP_PROPERTY_LOCAL(NumAASamples), 4);
	SC_AddBoolConfigParam(0,  TEXT("NoAATiles"), CPP_PROPERTY_LOCAL(NoAATiles), 1);
#if UTGLR_SUPPORT_DXGI_INTEROP
	SC_AddBoolConfigParam(1, TEXT("UseLowLatencySwapchain"), CPP_PROPERTY_LOCAL(UseLowLatencySwapchain), 1);
#endif

#if defined RENDER_DEVICE_OLDUNREAL
	SC_AddBoolConfigParam(0,  TEXT("UseLightmapAtlas"), CPP_PROPERTY_LOCAL(UseLightmapAtlas), 1);
#endif

	SurfaceSelectionColor = FColor(0, 0, 31, 31);
	UStruct* ColorStruct = FindObject<UStruct>(UObject::StaticClass(), TEXT("Color"));
	if (!ColorStruct)
		ColorStruct = new(UObject::StaticClass(), TEXT("Color")) UStruct(NULL);
	ColorStruct->PropertiesSize = sizeof(FColor); 
	new(GetClass(), TEXT("SurfaceSelectionColor"), RF_Public)UStructProperty(CPP_PROPERTY(SurfaceSelectionColor), TEXT("Options"), CPF_Config, ColorStruct);

	/*UEnum* */ContextEnum = new(GetClass(),TEXT("ContextType")) UEnum(nullptr);
	new(ContextEnum->Names) FName( TEXT("AutoDetect") );
	new(ContextEnum->Names) FName( TEXT("Compatibility") );
	new(ContextEnum->Names) FName( TEXT("Core_3_3")  );
	new(ContextEnum->Names) FName( TEXT("Core_4_5")  );
	new(ContextEnum->Names) FName( TEXT("ES_3_1")  );
	new(GetClass(), TEXT("ContextType"), RF_Public) UByteProperty(CPP_PROPERTY(ContextType), TEXT("Options"), CPF_Config, ContextEnum);
	new(GetClass(), TEXT("SelectedContextType"), RF_Public) UByteProperty(CPP_PROPERTY(SelectedContextType), TEXT("Options"), CPF_Config|CPF_Const|CPF_EditConst, ContextEnum);

#if __APPLE__
	UEnum* ColorCorrectionModeEnum = new(GetClass(), TEXT("ColorCorrectionMode")) UEnum(nullptr);
	new(ColorCorrectionModeEnum->Names) FName(TEXT("InShader"));
	SC_AddByteConfigParam(TEXT("ColorCorrectionMode"), CPP_PROPERTY_LOCAL(ColorCorrectionMode), CC_InShader, ColorCorrectionModeEnum);
#else
	UEnum* ColorCorrectionModeEnum = new(GetClass(), TEXT("ColorCorrectionMode")) UEnum(nullptr);
	new(ColorCorrectionModeEnum->Names) FName(TEXT("InShader"));
	new(ColorCorrectionModeEnum->Names) FName(TEXT("UseFramebuffer"));
	new(ColorCorrectionModeEnum->Names) FName(TEXT("UseGammaRamp"));
	SC_AddByteConfigParam(TEXT("ColorCorrectionMode"), CPP_PROPERTY_LOCAL(ColorCorrectionMode), CC_UseFramebuffer, ColorCorrectionModeEnum);
#endif

	new( GetClass(), TEXT("SmoothMasking"), RF_Public) UBoolProperty( CPP_PROPERTY(SmoothMasking), TEXT("ShaderOptions"), CPF_Config);
	SmoothMasking = 1;

	new( GetClass(), TEXT("PreferDedicatedGPU"), RF_Public) UBoolProperty( CPP_PROPERTY(PreferDedicatedGPU), TEXT("Options"), CPF_Config);
	PreferDedicatedGPU = 0;

#undef CPP_PROPERTY_LOCAL
#undef CPP_PROPERTY_LOCAL_DCV

	//Driver flags
	SpanBased				= 0;
	SupportsFogMaps			= 1;
#ifdef UTGLR_RUNE_BUILD
	SupportsDistanceFog		= 1;
#else
	SupportsDistanceFog		= 0;
#endif
	FullscreenOnly			= 0;

	SupportsLazyTextures	= 0;
	PrefersDeferredLoad		= 0;

#if RENDER_DEVICE_OLDUNREAL == 469 && !__LINUX_ARM__
	SupportsUpdateTextureRect = 1;
#endif

	DescFlags |= RDDESCF_Certified;

	unguard;
}


void UOpenGLRenderDevice::SC_AddBoolConfigParam(DWORD BitMaskOffset, const TCHAR *pName, UBOOL &param, ECppProperty EC_CppProperty, INT InOffset, UBOOL defaultValue) {
	//param = (((defaultValue) != 0) ? 1 : 0) << BitMaskOffset; //Doesn't exactly work like a UBOOL "// Boolean 0 (false) or 1 (true)."
	// stijn: we no longer need the manual bitmask shifting in patch 469
	param = defaultValue;
	new(GetClass(), pName, RF_Public)UBoolProperty(EC_CppProperty, InOffset, TEXT("Options"), CPF_Config);
}

void UOpenGLRenderDevice::SC_AddIntConfigParam(const TCHAR *pName, INT &param, ECppProperty EC_CppProperty, INT InOffset, INT defaultValue) {
	param = defaultValue;
	new(GetClass(), pName, RF_Public)UIntProperty(EC_CppProperty, InOffset, TEXT("Options"), CPF_Config);
}

void UOpenGLRenderDevice::SC_AddFloatConfigParam(const TCHAR *pName, FLOAT &param, ECppProperty EC_CppProperty, INT InOffset, FLOAT defaultValue) {
	param = defaultValue;
	new(GetClass(), pName, RF_Public)UFloatProperty(EC_CppProperty, InOffset, TEXT("Options"), CPF_Config);
}

void UOpenGLRenderDevice::SC_AddByteConfigParam(const TCHAR* pName, DWORD& param, ECppProperty EC_CppProperty, INT InOffset, DWORD defaultValue, UEnum* Enum) {
	param = defaultValue;
	new(GetClass(), pName, RF_Public)UByteProperty(EC_CppProperty, InOffset, TEXT("Options"), CPF_Config, Enum);
}


void UOpenGLRenderDevice::DbgPrintInitParam(const char *pName, INT value) {
	dbgPrintf("utglr: %s = %d\n", pName, value);
	return;
}

void UOpenGLRenderDevice::DbgPrintInitParam(const char *pName, FLOAT value) {
	dbgPrintf("utglr: %s = %f\n", pName, value);
	return;
}

DWORD UOpenGLRenderDevice::GetContextType()
{
	GetDefault<UOpenGLRenderDevice>()->SelectedContextType = InternalContextType;
	return InternalContextType;
}

bool UOpenGLRenderDevice::DetectContextType( DWORD PreferredContextType)
{
	guard(UOpenGLRenderDevice::DetectContextType);

#if __WIN32__
	if ( !FOpenGLBase::wglCreateContextAttribsARB ) // OpenGL 3.0 not supported?
	{
		FOpenGLBase::InitProcs(true);
		FOpenGLBase::InitCapabilities(true);
		if ( !InitGL1() )
			return false;
		InternalContextType = CONTEXTTYPE_GL1;
		return true;
	}
	check(FOpenGLBase::DummyWndClass);
#endif

	DWORD NewContextType = CONTEXTTYPE_DETECT;

	// Create the window
#if __WIN32__
	HWND DummyWindow = FOpenGLBase::CreateDummyWindow();
#else
	SDL_Window* DummyWindow = SDL_CreateWindow(
		"", 0, 0, 800, 600, 
		SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
#endif
	if ( !DummyWindow )
	{
		debugf( NAME_Warning, TEXT("Unable to create window for OpenGL Context evaluation"));
		return false;
	}

	// Create default Context
	FOpenGLBase::SetContextAttribute( FGL::EContextAttribute::PROFILE, FGL::PROFILE_UNDEFINED);
	FOpenGLBase::SetContextAttribute( FGL::EContextAttribute::MAJOR_VERSION, 0);
	FOpenGLBase::SetContextAttribute( FGL::EContextAttribute::MINOR_VERSION, 0);
	void* GLContext = FOpenGLBase::CreateContext(DummyWindow);

	UBOOL DefaultES = 0;
	UBOOL DefaultCompatibility = 1;
	GLint DefaultMajor = 0;
	GLint DefaultMinor = 0;
	FOpenGLBase::glGetIntegerv( GL_MAJOR_VERSION, &DefaultMajor);
	FOpenGLBase::glGetIntegerv( GL_MINOR_VERSION, &DefaultMinor);
	FString VersionString((const ANSICHAR*)FOpenGLBase::glGetString(GL_VERSION));
	DefaultES = VersionString.InStr(TEXT("OpenGL ES")) != INDEX_NONE;
	if ( VersionString.InStr(TEXT("Core")) != INDEX_NONE )
		DefaultCompatibility = 0;

	FOpenGLBase::DeleteContext(GLContext);
	GLContext = nullptr;

	// Choose detection order
#if __APPLE__
	static const DWORD ContextTypes[] = { CONTEXTTYPE_GL4, CONTEXTTYPE_GL3, CONTEXTTYPE_GL1};
#else
	static DWORD ContextTypes[] = { CONTEXTTYPE_GL4, CONTEXTTYPE_GLES, CONTEXTTYPE_GL1, CONTEXTTYPE_GL3};

	// If hardware supports at least OpenGL 4.3, prefer GL Core 3.3 over compatibility
	if ( DefaultMajor >= 4 && DefaultMinor >= 3 )
		Exchange(ContextTypes[2], ContextTypes[3]);
#endif

	TArray<DWORD> TryContext;
	if ( PreferredContextType )
		TryContext.AddUniqueItem(PreferredContextType);
	for ( INT i=0; i<ARRAY_COUNT(ContextTypes); i++)
		TryContext.AddUniqueItem(ContextTypes[i]);

	for ( INT i=0; i<TryContext.Num(); i++)
	{
		debugf( NAME_Init, TEXT("Evaluating context type %s..."), *ContextEnum->Names(TryContext(i)) );

		bool (*InitGL)() = nullptr;

		if ( TryContext(i) == CONTEXTTYPE_GL4 )
		{
			FOpenGLBase::SetContextAttribute( FGL::EContextAttribute::PROFILE, FGL::PROFILE_CORE);
			FOpenGLBase::SetContextAttribute( FGL::EContextAttribute::MAJOR_VERSION, 4);
			FOpenGLBase::SetContextAttribute( FGL::EContextAttribute::MINOR_VERSION, 5);
		}
		else if ( TryContext(i) == CONTEXTTYPE_GL3 )
		{
			InitGL = &InitGL3;
			FOpenGLBase::SetContextAttribute( FGL::EContextAttribute::PROFILE, FGL::PROFILE_CORE);
			FOpenGLBase::SetContextAttribute( FGL::EContextAttribute::MAJOR_VERSION, 3);
			FOpenGLBase::SetContextAttribute( FGL::EContextAttribute::MINOR_VERSION, 3);
		}
		else if ( TryContext(i) == CONTEXTTYPE_GL1 )
		{
			InitGL = &InitGL1;
			FOpenGLBase::SetContextAttribute( FGL::EContextAttribute::PROFILE, FGL::PROFILE_COMPATIBILITY);
			FOpenGLBase::SetContextAttribute( FGL::EContextAttribute::MAJOR_VERSION, 0);
			FOpenGLBase::SetContextAttribute( FGL::EContextAttribute::MINOR_VERSION, 0);
		}
		else if ( TryContext(i) == CONTEXTTYPE_GLES )
		{
			FOpenGLBase::SetContextAttribute( FGL::EContextAttribute::PROFILE, FGL::PROFILE_ES);
			FOpenGLBase::SetContextAttribute( FGL::EContextAttribute::MAJOR_VERSION, 3);
			FOpenGLBase::SetContextAttribute( FGL::EContextAttribute::MINOR_VERSION, 0);
		}
		else
			continue; //?

		// Is this GL interface implemented?
		if ( !InitGL )
			continue;

		// Attempt to create context with desired flags (if implemented)
		GLContext = FOpenGLBase::CreateContext(DummyWindow);
		if ( !GLContext && (TryContext(i) == CONTEXTTYPE_GL1) )
		{
			// Compatibility fallback - some SDL builds need this
			FOpenGLBase::SetContextAttribute( FGL::EContextAttribute::PROFILE, FGL::PROFILE_UNDEFINED);
			GLContext = FOpenGLBase::CreateContext(DummyWindow);
		}
		if ( !GLContext )
			continue;

		// Reacquire all procs using this context and test capabilities
		FOpenGLBase::InitProcs(true);
		FOpenGLBase::InitCapabilities(true);
		bool Success = InitGL();

		// Destroy temporary context.
		FOpenGLBase::DeleteContext(GLContext);
		GLContext = nullptr;

		if ( Success )
		{
			NewContextType = TryContext(i);
			break;
		}
	}

	// Destroy the Window
#if __WIN32__
	DestroyWindow(DummyWindow);
#else
	SDL_DestroyWindow(DummyWindow);
#endif
	InternalContextType = NewContextType;
	return NewContextType != CONTEXTTYPE_DETECT;

	unguard;
}

void UOpenGLRenderDevice::BuildGammaRamp(float redGamma, float greenGamma, float blueGamma, int brightness, FGammaRamp &ramp) {
	unsigned int u;

	//Parameter clamping
	if (brightness < -50) brightness = -50;
	if (brightness > 50) brightness = 50;

	float rcpRedGamma = 1.0f / (2.5f * redGamma);
	float rcpGreenGamma = 1.0f / (2.5f * greenGamma);
	float rcpBlueGamma = 1.0f / (2.5f * blueGamma);
	for (u = 0; u < 256; u++) {
		int iVal;
		int iValRed, iValGreen, iValBlue;

		//Initial value
		iVal = u;

		//Brightness
		iVal += brightness;
		//Clamping
		if (iVal < 0) iVal = 0;
		if (iVal > 255) iVal = 255;

		//Gamma
		iValRed = (int)appRound((float)appPow(iVal / 255.0f, rcpRedGamma) * 65535.0f);
		iValGreen = (int)appRound((float)appPow(iVal / 255.0f, rcpGreenGamma) * 65535.0f);
		iValBlue = (int)appRound((float)appPow(iVal / 255.0f, rcpBlueGamma) * 65535.0f);

		//Save results
		ramp.red[u] = (_WORD)iValRed;
		ramp.green[u] = (_WORD)iValGreen;
		ramp.blue[u] = (_WORD)iValBlue;
	}

	return;
}

void UOpenGLRenderDevice::BuildGammaRamp(float redGamma, float greenGamma, float blueGamma, int brightness, FByteGammaRamp &ramp) {
	unsigned int u;

	//Parameter clamping
	if (brightness < -50) brightness = -50;
	if (brightness > 50) brightness = 50;

	float rcpRedGamma = 1.0f / (2.5f * redGamma);
	float rcpGreenGamma = 1.0f / (2.5f * greenGamma);
	float rcpBlueGamma = 1.0f / (2.5f * blueGamma);
	for (u = 0; u < 256; u++) {
		int iVal;
		int iValRed, iValGreen, iValBlue;

		//Initial value
		iVal = u;

		//Brightness
		iVal += brightness;
		//Clamping
		if (iVal < 0) iVal = 0;
		if (iVal > 255) iVal = 255;

		//Gamma
		iValRed = (int)appRound((float)appPow(iVal / 255.0f, rcpRedGamma) * 255.0f);
		iValGreen = (int)appRound((float)appPow(iVal / 255.0f, rcpGreenGamma) * 255.0f);
		iValBlue = (int)appRound((float)appPow(iVal / 255.0f, rcpBlueGamma) * 255.0f);

		//Save results
		ramp.red[u] = (BYTE)iValRed;
		ramp.green[u] = (BYTE)iValGreen;
		ramp.blue[u] = (BYTE)iValBlue;
	}

	return;
}

void UOpenGLRenderDevice::SetGamma(FLOAT GammaCorrection)
{
	FGammaRamp gammaRamp;

	GammaCorrection += GammaOffset;

	//Color correction applied by shaders outside of legacy modes.
	if ( ColorCorrectionMode != CC_UseGammaRamp )
		return;

	//Do not attempt to set gamma if <= zero
	if ( GammaCorrection <= 0.0f )
		return;

	BuildGammaRamp(GammaCorrection + GammaOffsetRed, GammaCorrection + GammaOffsetGreen, GammaCorrection + GammaOffsetBlue, Brightness, gammaRamp);

#ifdef __UNIX__
	// Higor: How to acquire original gamma ramp?
	SDL_Window* Window = reinterpret_cast<SDL_Window*>(Viewport->GetWindow());
	GUsingGammaRamp = !SDL_SetWindowGammaRamp( Window, gammaRamp.red, gammaRamp.green, gammaRamp.blue );
#else

	HDC DC = FOpenGLBase::WindowDC_Get((HWND)GL->Window);

	//Remember previous gamma ramp setting
	if (g_gammaFirstTime)
	{
		if ( DC && GetDeviceGammaRamp(DC, &g_originalGammaRamp) )
			g_haveOriginalGammaRamp = true;
		g_gammaFirstTime = false;
	}

	//Set gamma ramp
	m_setGammaRampSucceeded = false;
	GUsingGammaRamp = (UBOOL)SetDeviceGammaRamp(DC, &gammaRamp);
	if ( GUsingGammaRamp )
	{
		m_setGammaRampSucceeded = true;
		SavedGammaCorrection = GammaCorrection;
	}
#endif
}

void UOpenGLRenderDevice::ResetGamma(void)
{
#ifdef __UNIX__

#else
	//Restore gamma ramp if original was successfully saved
	if ( g_haveOriginalGammaRamp )
	{
		HWND hDesktopWnd;
		HDC hDC;

		hDesktopWnd = GetDesktopWindow();
		hDC = GetDC(hDesktopWnd);

		// vogel: grrr, UClient::destroy is called before this gets called so hDC is invalid
		SetDeviceGammaRamp(hDC, &g_originalGammaRamp);

		ReleaseDC(hDesktopWnd, hDC);
		GUsingGammaRamp = false;
	}
#endif
}


bool UOpenGLRenderDevice::IsGLExtensionSupported(const char *pExtensionsString, const char *pExtensionName) {
	const char *pStart;
	const char *pWhere, *pTerminator;

	pStart = pExtensionsString;
	while (1) {
		pWhere = strstr(pStart, pExtensionName);
		if (pWhere == NULL) {
			break;
		}
		pTerminator = pWhere + strlen(pExtensionName);
		if ((pWhere == pStart) || (*(pWhere - 1) == ' ')) {
			if ((*pTerminator == ' ') || (*pTerminator == '\0')) {
				return true;
			}
		}
		pStart = pTerminator;
	}

	return false;
}

UBOOL UOpenGLRenderDevice::FailedInitf(const TCHAR* Fmt, ...) {
	TCHAR TempStr[4096];
	GET_VARARGS(TempStr, ARRAY_COUNT(TempStr), Fmt); // stijn: ok if this gets truncated
	debugf(NAME_Init, TempStr);
	Exit();
	return 0;
}



void UOpenGLRenderDevice::ShutdownAfterError()
{
	guard(UOpenGLRenderDevice::ShutdownAfterError);

	debugf(NAME_Exit, TEXT("UOpenGLRenderDevice::ShutdownAfterError"));

	if (DebugBit(DEBUG_BIT_BASIC))
		dbgPrintf("utglr: ShutdownAfterError\n");

	//ChangeDisplaySettings(NULL, 0);

	//Reset gamma ramp
	ResetGamma();

	unguard;
}


UBOOL UOpenGLRenderDevice::SetRes(INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen)
{
	guard(UOpenGLRenderDevice::SetRes);

	FOpenGLBase::ActiveInstance = nullptr;

	SelectedContextType = InternalContextType;
	GetDefault<UOpenGLRenderDevice>()->SelectedContextType = InternalContextType;

	FGL::Draw::InitCmdMem();

	bool NewWindow = (GL == nullptr) || (GL->Window != Viewport->GetWindow()); // Ugly, improve this layer.
	if ( GL )
	{
		// Allow Viewport to change this renderer's container
		GL->MakeCurrent(Viewport->GetWindow());
	}

	// Do not allow 16 bit colors anymore
	NewColorBytes = 4; 

	//Get debug bits
	{
		INT i = 0;
		if (!GConfig->GetInt(g_pSection, TEXT("DebugBits"), i)) i = 0;
		m_debugBits = i;
	}
	//Display debug bits
	if (DebugBit(DEBUG_BIT_ANY)) dbgPrintf("utglr: DebugBits = %u\n", m_debugBits);

#ifdef __UNIX__
	UnsetRes();

	INT MinDepthBits;

	m_usingAA = false;
	m_initNumAASamples = 1;
	m_curAAEnable = true;
	m_defAAEnable = true;

	// Minimum size of the depth buffer
	if (!GConfig->GetInt(g_pSection, TEXT("MinDepthBits"), MinDepthBits)) MinDepthBits = 16;
	//debugf( TEXT("MinDepthBits = %i"), MinDepthBits );
	// 16 is the bare minimum.
	if (MinDepthBits < 16) MinDepthBits = 16;

	INT RequestDoubleBuffer;
	if (!GConfig->GetInt(g_pSection, TEXT("RequestDoubleBuffer"), RequestDoubleBuffer)) RequestDoubleBuffer = 1;

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, (NewColorBytes <= 2) ? 5 : 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, (NewColorBytes <= 2) ? 5 : 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, (NewColorBytes <= 2) ? 5 : 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, MinDepthBits);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, RequestDoubleBuffer ? 1 : 0);

	//TODO: Set this correctly
	m_numDepthBits = MinDepthBits;

	// Change window size.
	if ( Viewport->ResizeViewport(Fullscreen ? (BLIT_Fullscreen | BLIT_OpenGL) : (BLIT_HardwarePaint | BLIT_OpenGL), NewX, NewY, NewColorBytes) )
	{
		RequestedFramebufferWidth = NewX;
		RequestedFramebufferHeight = NewY;
		RequestedFramebufferHighQuality = 1;
		FBO_Wait = 2;
	}
#else
	debugf(TEXT("Enter SetRes()"));


	if ( GL && GL->Context && !NewWindow )
	{

		// If not fullscreen, and color bytes hasn't changed, do nothing.
		if ( !Fullscreen && !WasFullscreen && NewColorBytes == (INT)Viewport->ColorBytes )
		{
			if ( !Viewport->ResizeViewport(BLIT_HardwarePaint | BLIT_OpenGL, NewX, NewY, NewColorBytes) )
				return 0;
			RequestedFramebufferWidth = NewX;
			RequestedFramebufferHeight = NewY;
			FBO_Wait = 2;
			GL->SetViewport( 0, 0, NewX, NewY);
			return 1;
		}

		// Exit res.
		debugf(TEXT("UnSetRes() -> Context != NULL"));
		UnsetRes();
	}


	// Change display settings.
	if ( Fullscreen )
	{
		INT FindX = NewX, FindY = NewY, BestError = MAXINT;
		for (INT i = 0; i < Modes.Num(); i++)
		{
			if (Modes(i).Z==NewColorBytes*8)
			{
				INT Error
				=	(Modes(i).X-FindX)*(Modes(i).X-FindX)
				+	(Modes(i).Y-FindY)*(Modes(i).Y-FindY);
				if (Error < BestError) {
					NewX      = Modes(i).X;
					NewY      = Modes(i).Y;
					BestError = Error;
				}
			}
		}
		{
			DEVMODEA dma;
			DEVMODEW dmw;
			bool tryNoRefreshRate = true;

			ZeroMemory(&dma, sizeof(dma));
			ZeroMemory(&dmw, sizeof(dmw));
			dma.dmSize = sizeof(dma);
			dmw.dmSize = sizeof(dmw);
			dma.dmPelsWidth = dmw.dmPelsWidth = NewX;
			dma.dmPelsHeight = dmw.dmPelsHeight = NewY;
			dma.dmBitsPerPel = dmw.dmBitsPerPel = NewColorBytes * 8;
			dma.dmFields = dmw.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_POSITION;// | DM_BITSPERPEL;
			if (RefreshRate) {
				dma.dmDisplayFrequency = dmw.dmDisplayFrequency = RefreshRate;
				dma.dmFields |= DM_DISPLAYFREQUENCY;
				dmw.dmFields |= DM_DISPLAYFREQUENCY;

				if (ChangeDisplaySettingsW(&dmw, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL) {
					debugf(TEXT("ChangeDisplaySettings failed: %ix%i, %i Hz"), NewX, NewY, RefreshRate);
					dma.dmFields &= ~DM_DISPLAYFREQUENCY;
					dmw.dmFields &= ~DM_DISPLAYFREQUENCY;
				}
				else {
					tryNoRefreshRate = false;
				}
			}
			if (tryNoRefreshRate) {
				if (ChangeDisplaySettingsW(&dmw, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL) {
					debugf(TEXT("ChangeDisplaySettings failed: %ix%i"), NewX, NewY);

					return 0;
				}
			}
		}
	}

	// Change window size.
	debugf(NAME_DevGraphics, TEXT("Attempt (2) ResizeViewport(%i, %i, %i, %i)"), Fullscreen, NewX, NewY, NewColorBytes);
	UBOOL Result = Viewport->ResizeViewport(Fullscreen ? (BLIT_Fullscreen | BLIT_OpenGL) : (BLIT_HardwarePaint | BLIT_OpenGL), NewX, NewY, NewColorBytes);
	if (!Result) {
		if (Fullscreen) {
			ChangeDisplaySettingsW(NULL, 0);
		}
		return 0;
	}

	//Set default numDepthBits in case any failures might prevent it from being set later
	m_numDepthBits = 16;

	//Restrict NumAASamples range
	if (NumAASamples < 1)
		NumAASamples = 1;
	m_curAAEnable = true;
	m_defAAEnable = true;
	if ( NewWindow )
		SetAAPixelFormat(NewColorBytes);

#endif
	if ( !(this->*SetGL[GetContextType()])(Viewport->GetWindow()) )
		appFailAssert( "Unable to initialize OpenGL context", __FILE__, __LINE__);
	GL->MakeCurrent(GL->Window); // TEST

#if UTGLR_SUPPORT_DXGI_INTEROP
	if (!GIsEditor)
	{
		GL->CreateDXGISwapchain();
		GL->SetDXGIFullscreenState(Fullscreen);
	}
#endif

	//Reset previous SwapBuffers status to okay
	m_prevSwapBuffersStatus = true;

	debugf(NAME_Init, TEXT("Depth bits: %u"), m_numDepthBits);
	if (DebugBit(DEBUG_BIT_BASIC)) dbgPrintf("utglr: Depth bits: %u\n", m_numDepthBits);
	if (m_usingAA) {
		debugf(NAME_Init, TEXT("AA samples: %u"), m_initNumAASamples);
		if (DebugBit(DEBUG_BIT_BASIC)) dbgPrintf("utglr: AA samples: %u\n", m_initNumAASamples);
	}

	//Get other defaults
	if (!GConfig->GetInt(g_pSection, TEXT("Brightness"), Brightness)) Brightness = 0;

	//Debug parameter listing
	if (DebugBit(DEBUG_BIT_BASIC)) {
		#define UTGLR_DEBUG_SHOW_PARAM_REG(name) DbgPrintInitParam(#name, name)

		UTGLR_DEBUG_SHOW_PARAM_REG(LODBias);
		UTGLR_DEBUG_SHOW_PARAM_REG(GammaOffset);
		UTGLR_DEBUG_SHOW_PARAM_REG(GammaOffsetRed);
		UTGLR_DEBUG_SHOW_PARAM_REG(GammaOffsetGreen);
		UTGLR_DEBUG_SHOW_PARAM_REG(GammaOffsetBlue);
		UTGLR_DEBUG_SHOW_PARAM_REG(Brightness);
		UTGLR_DEBUG_SHOW_PARAM_REG(GammaCorrectScreenshots);
		UTGLR_DEBUG_SHOW_PARAM_REG(OneXBlending);
		UTGLR_DEBUG_SHOW_PARAM_REG(MaxAnisotropy);
		UTGLR_DEBUG_SHOW_PARAM_REG(RefreshRate);
		UTGLR_DEBUG_SHOW_PARAM_REG(AlwaysMipmap);
		UTGLR_DEBUG_SHOW_PARAM_REG(UsePrecache);
		UTGLR_DEBUG_SHOW_PARAM_REG(UseTrilinear);
		UTGLR_DEBUG_SHOW_PARAM_REG(UseHDTextures);
		UTGLR_DEBUG_SHOW_PARAM_REG(Use16BitTextures);
		UTGLR_DEBUG_SHOW_PARAM_REG(ForceNoSmooth);
		UTGLR_DEBUG_SHOW_PARAM_REG(DetailMax);
		UTGLR_DEBUG_SHOW_PARAM_REG(SwapInterval);
#if UTGLR_USES_SCENENODE_HACK
		UTGLR_DEBUG_SHOW_PARAM_REG(SceneNodeHack);
#endif
		UTGLR_DEBUG_SHOW_PARAM_REG(UseAA);
		UTGLR_DEBUG_SHOW_PARAM_REG(NumAASamples);
		UTGLR_DEBUG_SHOW_PARAM_REG(NoAATiles);
#if UTGLR_SUPPORT_DXGI_INTEROP
		UTGLR_DEBUG_SHOW_PARAM_REG(UseLowLatencySwapchain);
#endif

		#undef UTGLR_DEBUG_SHOW_PARAM_REG
		#undef UTGLR_DEBUG_SHOW_PARAM_DCV
	}

	//Special handling for WGL_EXT_swap_control
	//Restricted to a maximum of 10
		
	if ( NewWindow && (SwapInterval >= -1) && (SwapInterval <= 10))
	{
#if __WIN32__
		if ( FOpenGLBase::SUPPORTS_WGL_EXT_swap_control && FOpenGLBase::wglSwapIntervalEXT )
			if ( SwapInterval >= 0 || FOpenGLBase::SUPPORTS_WGL_EXT_swap_control_tear )
				FOpenGLBase::wglSwapIntervalEXT(SwapInterval);
#else
		//Add non-win32 swap control code here
		// stijn: added this in SDLDrv instead...
#endif
	}

	//Required extensions config validation pass
	ConfigValidate_RequiredExtensions();

	//Main config validation pass (after set TMUnits)
	ConfigValidate_Main();

	UBOOL flushTextures = true;
	UpdateSamplers(flushTextures);
	UpdateTextureFormat(flushTextures);
	UpdateBuffers();

	UpdateStateLocks();

	// Flush textures.
//	Flush(1);

	//Get maximum supported Texture Size
#if UNREAL_TOURNAMENT_OLDUNREAL
	// Tell Render interface how big textures can be
	MaxTextureSize = FOpenGLBase::MaxTextureSize;
#endif


#ifdef UTGLR_RUNE_BUILD
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_START, 0.0f);
#endif
	m_gpFogEnabled = false;

	// Init variables.
	DrawBuffer.ActiveType = 0;

	m_curBlendFlags = PF_Occlude;
	m_alphaToCoverageEnabled = false;
	m_curPolyFlags = 0;

	//Reset current frame count
	m_currentFrameCount = 0;

	// Remember fullscreenness.
	WasFullscreen = Fullscreen;

	// Higor: why is WinDrv passing NewX, NewY as -1 in EndFullscreen()?
	if (!Fullscreen && (NewX == INDEX_NONE) && (NewY == INDEX_NONE))
	{
		NewX = Viewport->SizeX;
		NewY = Viewport->SizeY;
	}
	RequestedFramebufferWidth = NewX;
	RequestedFramebufferHeight = NewY;
	RequestedFramebufferHighQuality = NewColorBytes > 2;
	FBO_Wait = 2;

	return 1;

	unguard;
}

void UOpenGLRenderDevice::UnsetRes()
{
	guard(UOpenGLRenderDevice::UnsetRes);

	//Flush textures, buffers, objects
	Flush(1);

#if __WIN32__
	if ( !GIsEditor && NumDevices && GL && GL->Context ) // TODO: Keep context in Windows
	{
//		FOpenGLBase::DeleteContext(GL->Context);
//		GL->Context = nullptr;
	}
	if (WasFullscreen)
		ChangeDisplaySettingsW(NULL, 0);
#endif

	unguard;
}


void UOpenGLRenderDevice::CheckGLErrorFlag(const TCHAR *pTag)
{
	const TCHAR* ErrorText = FOpenGLBase::GetGLError();
	if ( ErrorText && DebugBit(DEBUG_BIT_GL_ERROR))
		debugf(TEXT("OpenGL Error: %s (%s)"), ErrorText, pTag);
}


void UOpenGLRenderDevice::ConfigValidate_RequiredExtensions(void)
{
	if (!FOpenGLBase::SupportsLODBias)               LODBias = 0;
	if (!FOpenGLBase::SupportsFramebuffer)           AlwaysMipmap = 0;
}

template<INT M> inline bool IsPowerOfTwo( INT N)
{
	static_assert(TBits<M>::MostSignificant == M, "IsPowerOfTwo: maximum is not power of two");
	for ( INT MM=M; MM>0; MM>>=1)
		if ( N == MM )
			return true;
	return false;
}

void UOpenGLRenderDevice::ConfigValidate_Main(void) {

	// Limit DetailMax
	DetailMax = Clamp<INT>( DetailMax, 1, 3);

	// Normalize
	ForceNoSmooth = (ForceNoSmooth != 0);

	// Limit to hardware capabilities
	MaxAnisotropy = Clamp<INT>( MaxAnisotropy, 0, FOpenGLBase::MaxAnisotropy);

	// Must be a power of two
	if ( !IsPowerOfTwo<16>(NumAASamples) )
		NumAASamples = 2;

#if RENDER_DEVICE_OLDUNREAL == 469
	SupportsStaticBsp = true;
#endif

#if USES_AMBIENTLESS_LIGHTMAPS
	// Notify the render interface this Render Device supports this feature.
	UseAmbientlessLightmaps = true;
#endif

	// Keep context type display accurate
	SelectedContextType = InternalContextType;
	GetDefault<UOpenGLRenderDevice>()->SelectedContextType = InternalContextType; // Accept this context type

	return;
}


UBOOL UOpenGLRenderDevice::Init(UViewport* InViewport, INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen)
{
	guard(UOpenGLRenderDevice::Init);

	debugf(TEXT("Initializing OpenGLDrv..."));

	SetPreferredGPU(PreferDedicatedGPU);

	if (NumDevices == 0)
	{
		g_gammaFirstTime = true;
		g_haveOriginalGammaRamp = false;
		m_SamplerInit = false;
		m_FormatInfo = false;
		m_initNumAASamples = INDEX_NONE;
	}
	m_DGT = 1;
	DebugUseGL3Resolve = 1;

	if ( !FOpenGLBase::Init() )
		return 0;

	if ( (GetContextType() == CONTEXTTYPE_DETECT) && !DetectContextType(ContextType) )
	{
		debugf( NAME_Init, TEXT("OpenGLDrv: Unable to find supported OpenGL Context") );
		return 0;
	}

	if ( FOpenGLBase::MaxTextureImageUnits < TMU_BASE_MAX )
	{
		const TCHAR* Error = TEXT("Insufficient Texture Image Mapping Units");
		if ( !GIsEditor )
			appErrorf(Error);
		else
			debugf(Error);
		return 0;
	}

#ifdef __UNIX__
	CheckContext();
#else

	// Get list of device modes.
	for ( INT i=0; ; i++)
	{
		DEVMODEW Tmp;
		appMemzero(&Tmp, sizeof(Tmp));
		Tmp.dmSize = sizeof(Tmp);
		if ( !EnumDisplaySettingsW(NULL, i, &Tmp) )
			break;
		if ( Tmp.dmBitsPerPel < 32 )
			continue;
		Modes.AddUniqueItem(FPlane(Tmp.dmPelsWidth, Tmp.dmPelsHeight, Tmp.dmBitsPerPel, Tmp.dmDisplayFrequency));
	}
#endif

	NumDevices++;
	Viewport = InViewport;

#if 0
	{
		//Print all PFD's exposed
		INT pf;
		INT pfCount = DescribePixelFormat(m_hDC, 0, 0, NULL);
		for (pf = 1; pf <= pfCount; pf++) {
			PrintFormat(m_hDC, pf);
		}
	}
#endif

	guard(UpdateConfig);
	TMultiMap<FString,FString>* Section = GConfig->GetSectionPrivate( g_pSection, 0, 0);
	if ( Section )
	{
		// Remove beta or ancient options that may be carried over from other INIs
		Section->Remove(TEXT("Translucency"));
		Section->Remove(TEXT("TextureClampHack"));
		Section->Remove(TEXT("DisableSpecialDT"));
		Section->Remove(TEXT("MaxLogTextureSize"));
		Section->Remove(TEXT("MinLogTextureSize"));
		Section->Remove(TEXT("MaxLogVOverU"));
		Section->Remove(TEXT("MaxLogUOverV"));
		Section->Remove(TEXT("UseVertexSpecular"));
		Section->Remove(TEXT("UseCVA"));
		Section->Remove(TEXT("UseBGRATextures"));
		Section->Remove(TEXT("UseS3TC"));
		Section->Remove(TEXT("UseTNT"));
		Section->Remove(TEXT("UseFilterSGIS"));
		Section->Remove(TEXT("Use4444Textures"));
		Section->Remove(TEXT("UseTexPool"));
		Section->Remove(TEXT("UseTexIdPool"));
		Section->Remove(TEXT("CacheStaticMaps"));
		Section->Remove(TEXT("TexDXT1ToDXT3"));
		Section->Remove(TEXT("RequestHighResolutionZ"));
		Section->Remove(TEXT("NoMaskedS3TC"));
		Section->Remove(TEXT("DynamicTexIdRecycleLevel"));
		Section->Remove(TEXT("SinglePassFog"));
		Section->Remove(TEXT("ShaderGammaCorrectHUD"));
		Section->Remove(TEXT("BufferTileQuads"));
		Section->Remove(TEXT("BufferActorTris"));
#if !UTGLR_USES_SCENENODE_HACK
		Section->Remove(TEXT("SceneNodeHack"));
#endif
		Section->Remove(TEXT("UseFragmentProgram"));
		Section->Remove(TEXT("ShaderType"));
		Section->Remove(TEXT("SinglePassFog"));
		Section->Remove(TEXT("SinglePassDetail"));
		Section->Remove(TEXT("UseSSE"));
		Section->Remove(TEXT("UseSSE2"));
		Section->Remove(TEXT("ZRangeHack"));
		Section->Remove(TEXT("BufferClippedActorTris"));
		Section->Remove(TEXT("UseZTrick"));
		Section->Remove(TEXT("MaxTMUnits"));
		Section->Remove(TEXT("ColorizeDetailTextures"));
		Section->Remove(TEXT("RenderPath"));
		Section->Remove(TEXT("UseSamplerObjects"));
		Section->Remove(TEXT("NoFiltering"));
		Section->Remove(TEXT("ShaderMasking"));
	}
	unguard;

	UpdateStateLocks();
	if ( !SetRes(NewX, NewY, NewColorBytes, Fullscreen) )
		return FailedInitf(LocalizeError("ResFailed"));

	return 1;
	unguard;
}

UBOOL UOpenGLRenderDevice::Exec( const TCHAR* Cmd, FOutputDevice& Ar)
{
	guard(UOpenGLRenderDevice::Exec);

	if ( URenderDevice::Exec(Cmd, Ar) )
		return 1;
	if ( ParseCommand(&Cmd, TEXT("DGL")) )
	{
		if ( ParseCommand(&Cmd, TEXT("BUILD")) )
		{
			debugf( TEXT("OpenGL renderer built: %s %s"), appFromAnsi(__DATE__), appFromAnsi(__TIME__));
			return 1;
		}
		else if ( ParseCommand(&Cmd, TEXT("AA")) )
		{
			if (m_usingAA)
			{
				m_defAAEnable = !m_defAAEnable;
				debugf(TEXT("AA Enable [%u]"), (m_defAAEnable) ? 1 : 0);
			}
			return 1;
		}
		else if ( ParseCommand(&Cmd, TEXT("NoInmutable")) )
		{
			FOpenGLBase::SupportsTextureStorage = false;
			debugf(TEXT("Disabled inmutable texture storage."));
			Flush(1);
			return 1;
		}
		else if ( ParseCommand(&Cmd, TEXT("FBO")) )
		{
			MainFramebuffer_Width *= -1;
			DebugUseGL3Resolve = !!!DebugUseGL3Resolve;
			debugf(TEXT("GL3 FBO mode %s."), DebugUseGL3Resolve ? TEXT("enabled") : TEXT("disabled"));
			return 1;
		}
	}
	else if ( ParseCommand(&Cmd, TEXT("GetRes")) )
	{
#ifdef __UNIX__

		FString Str = TEXT("");
		// Available fullscreen video modes
		INT display_count = 0, display_index = 0, mode_index = 0, i = 0;
		SDL_DisplayMode mode = { SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0 };

		if ((display_count = SDL_GetNumDisplayModes(0)) < 1)
		{
			debugf(NAME_Init, TEXT("No available fullscreen video modes"));
		}
		//debugf(TEXT("SDL_GetNumVideoDisplays returned: %i"), display_count);

		INT PrevW = 0, PrevH = 0;
		for (i = 0; i < display_count; i++)
		{
			mode.format = 0;
			mode.w = 0;
			mode.h = 0;
			mode.refresh_rate = 0;
			mode.driverdata = 0;
			if (SDL_GetDisplayMode(display_index, i, &mode) != 0)
				debugf(TEXT("SDL_GetDisplayMode failed: %ls"), SDL_GetError());			
//			debugf(TEXT("SDL_GetDisplayMode(0, 0, &mode):\t\t%i bpp\t%i x %i"), SDL_BITSPERPIXEL(mode.format), mode.w, mode.h);

			if (mode.w != PrevW || mode.h != PrevH)
				Str += FString::Printf(TEXT("%ix%i "), mode.w, mode.h);
			PrevW = mode.w;
			PrevH = mode.h;
		}
		// Send the resolution string to the engine.
		Ar.Log(*Str.LeftChop(1));
		return 1;
#else
		TArray<FPlane> Relevant;
		INT i;
		for (i = 0; i < Modes.Num(); i++) {
			if (Modes(i).Z == (Viewport->ColorBytes * 8))
				if
				(	(Modes(i).X!=320 || Modes(i).Y!=200)
				&&	(Modes(i).X!=640 || Modes(i).Y!=400) )
				Relevant.AddUniqueItem(FPlane(Modes(i).X, Modes(i).Y, 0, 0));
		}
		appQsort(&Relevant(0), Relevant.Num(), sizeof(FPlane), (QSORT_COMPARE)CompareRes);
		FString Str;
		for (i = 0; i < Relevant.Num(); i++) {
			Str += FString::Printf(TEXT("%ix%i "), (INT)Relevant(i).X, (INT)Relevant(i).Y);
		}
		Ar.Log(*Str.LeftChop(1));
		return 1;
#endif
	}

	return 0;

	unguard;
}

void UOpenGLRenderDevice::Lock(FPlane InFlashScale, FPlane InFlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* InHitData, INT* InHitSize)
{
	UTGLR_DEBUG_CALL_COUNT(Lock);
	guard(UOpenGLRenderDevice::Lock);
	check(LockCount == 0);
	++LockCount;

	//Reset stats
	BindCycles = ImageCycles = ComplexCycles = GouraudCycles = TileCycles = 0;

	m_AASwitchCount = 0;
	m_sceneNodeCount = 0;
#if UTGLR_USES_SCENENODE_HACK
	m_sceneNodeHackCount = 0;
#endif
	m_shaderAlphaHUD = 0;
	m_ShaderColorCorrection = (ColorCorrectionMode == CC_InShader) || (ColorCorrectionMode == CC_UseFramebuffer && !MainFramebuffer_Required);

#ifdef __WIN32__
#else
	if ( GIsEditor )
		FOpenGLBase::CurrentContext = SDL_GL_GetCurrentContext();
	CheckContext( reinterpret_cast<SDL_Window*>(Viewport->GetWindow()) );
#endif
	GL->MakeCurrent(GL->Window);

	// Set GL state and clear the Z buffer if needed.
	GL->ViewportInfo.ForceSet = 1;
	FOpenGLBase::SetTrilinear(UseTrilinear);
	FOpenGLBase::SetAnisotropy(MaxAnisotropy);
	FOpenGLBase::SetLODBias(LODBias);
	FOpenGLBase::glClearColor(ScreenClear.X, ScreenClear.Y, ScreenClear.Z, ScreenClear.W);
	FOpenGLBase::glClearDepthf(1.0f);
	FOpenGLBase::glDepthRangef(0.0f, 1.0f);
	FOpenGLBase::glPolygonOffset(-1.0f, -1.0f);
	SetBlend(PF_Occlude);
	FOpenGLBase::glClear(GL_DEPTH_BUFFER_BIT | ((RenderLockFlags & LOCKR_ClearScreen) ? GL_COLOR_BUFFER_BIT : 0));
	if ( UpdateMainFramebuffer() )
	{
		LockMainFramebuffer();
		FOpenGLBase::glClear(GL_DEPTH_BUFFER_BIT | ((RenderLockFlags & LOCKR_ClearScreen) ? GL_COLOR_BUFFER_BIT : 0));
	}
	UpdateBuffers();

#if UTGLR_SUPPORT_DXGI_INTEROP
	if (UseLowLatencySwapchain)
		GL->LockDXGIFramebuffer();
#endif

	// Anti-aliasing state depends on whether we're using a AA back buffer or a AA framebuffer
	m_usingAA = UsingFBOMultisampling();
	FOpenGLBase::SetDervMapping(SmoothMasking && m_usingAA);

	TexturePool.Lock();

	//Deallocate compose memory
	FTextureUploadState::Compose.Empty();


	//Required extensions config validation pass
	ConfigValidate_RequiredExtensions();

	//Main config validation pass
	ConfigValidate_Main();

	if ( ColorCorrectionMode != CC_UseGammaRamp )
	{
		FLOAT Brightness = GIsEditor ? 1.f : Clamp(Viewport->GetOuterUClient()->Brightness * 2.0, 0.05, 2.99);
		if ( UseShaderGamma && !GIsEditor )
			FOpenGLBase::SetColorCorrection( Brightness, GammaOffset+GammaOffsetRed, GammaOffset+GammaOffsetGreen, GammaOffset+GammaOffsetBlue);
		else
			FOpenGLBase::SetColorCorrection( Brightness);
	}
	else
		FOpenGLBase::DisableColorCorrection();


	// Gamma Ramp control
	if ( NumDevices )
	{
		if ( (ColorCorrectionMode == CC_UseGammaRamp ) && !GUsingGammaRamp )
			SetGamma(Viewport->GetOuterUClient()->Brightness);
		else if ( (ColorCorrectionMode != CC_UseGammaRamp ) && GUsingGammaRamp )
			ResetGamma();
	}

	// Remember stuff.
	FlashScale = InFlashScale;
	FlashFog   = InFlashFog;

	//Selection setup
	m_HitData = InHitData;
	m_HitSize = InHitSize;
	m_HitCount = 0;
	if (m_HitData) {
		m_HitBufSize = *m_HitSize;
		*m_HitSize = 0;

		//Start selection
		m_gclip.SelectModeStart();
	}

	// Setup or update texture format list if needed.
	UBOOL FlushResources = false;
	if ( (DetailTextures != PL_DetailTextures) && !DetailTextures )
		FlushResources = true;
	UpdateSamplers(FlushResources);
	UpdateTextureFormat(FlushResources);

	//Flush if necessary due to config change
	GL_ERROR_ASSERT
	if ( FlushResources )
		Flush(1);
	GL_ERROR_ASSERT

	FOpenGLBase::ActiveInstance->Lock();
	UpdateStateLocks();



	// Init clip plane counter
	NumClipPlanes = 0;

	// Select rendering paths for all subsequent calls
	DWORD CurrentContextType = GetContextType();
	if ( m_HitData )
	{
		// HitTesting render path
		m_pDrawComplexSurface   = &UOpenGLRenderDevice::DrawComplexSurface_HitTesting;
		m_pDrawGouraudPolygon   = &UOpenGLRenderDevice::DrawGouraudPolygon_HitTesting;
		m_pDrawTile             = &UOpenGLRenderDevice::DrawTile_HitTesting;
		m_pDraw3DLine           = &UOpenGLRenderDevice::Draw3DLine_HitTesting;
		m_pDraw2DLine           = &UOpenGLRenderDevice::Draw2DLine_HitTesting;
		m_pDraw2DPoint          = &UOpenGLRenderDevice::Draw2DPoint_HitTesting;
		m_pDrawGouraudTriangles = nullptr;
		m_pSetSceneNode         = &UOpenGLRenderDevice::SetSceneNode_HitTesting;
		m_pFillScreen           = nullptr;
		m_pFlushDrawBuffers     = nullptr;
		m_pBlitFramebuffer      = nullptr;
	}
	else if ( CurrentContextType == CONTEXTTYPE_GL1 )
	{
		// ARB rendering
		m_pDrawComplexSurface   = &UOpenGLRenderDevice::DrawComplexSurface_ARB;
		m_pDrawGouraudPolygon   = &UOpenGLRenderDevice::DrawGouraudPolygon_ARB;;
		m_pDrawTile             = &UOpenGLRenderDevice::DrawTile_ARB;
		m_pDraw3DLine           = &UOpenGLRenderDevice::Draw3DLine_ARB;
		m_pDraw2DLine           = &UOpenGLRenderDevice::Draw2DLine_ARB;
		m_pDraw2DPoint          = &UOpenGLRenderDevice::Draw2DPoint_ARB;
		m_pDrawGouraudTriangles = &UOpenGLRenderDevice::DrawGouraudTriangles_ARB;
		m_pSetSceneNode         = &UOpenGLRenderDevice::SetSceneNode_ARB;
		m_pFillScreen           = &UOpenGLRenderDevice::FillScreen_ARB;
		m_pFlushDrawBuffers     = &UOpenGLRenderDevice::FlushDrawBuffers_ARB;
		m_pBlitFramebuffer      = nullptr;
	}
	else if ( CurrentContextType == CONTEXTTYPE_GL3 )
	{
		// No rendering
		m_pDrawComplexSurface   = &UOpenGLRenderDevice::DrawComplexSurface_GLSL3;
		m_pDrawGouraudPolygon   = &UOpenGLRenderDevice::DrawGouraudPolygon_GLSL3;
		m_pDrawTile             = &UOpenGLRenderDevice::DrawTile_GLSL3;
		m_pDraw3DLine           = &UOpenGLRenderDevice::Draw3DLine_GLSL3;
		m_pDraw2DLine           = &UOpenGLRenderDevice::Draw2DLine_GLSL3;
		m_pDraw2DPoint          = &UOpenGLRenderDevice::Draw2DPoint_GLSL3;
		m_pDrawGouraudTriangles = &UOpenGLRenderDevice::DrawGouraudTriangles_GLSL3;
		m_pSetSceneNode         = &UOpenGLRenderDevice::SetSceneNode_GLSL3;
		m_pFillScreen           = &UOpenGLRenderDevice::FillScreen_GLSL3;
		m_pFlushDrawBuffers     = &UOpenGLRenderDevice::ExecDraws_GLSL3; //!!!
		m_pBlitFramebuffer      = DebugUseGL3Resolve ? &UOpenGLRenderDevice::BlitFramebuffer_GLSL3 : nullptr;
		DrawBuffer.ActiveType = DRAWBUFFER_CommandBuffer;
	}
	else
	{
		// No rendering
		m_pDrawComplexSurface   = nullptr;
		m_pDrawGouraudPolygon   = nullptr;
		m_pDrawTile             = nullptr;
		m_pDraw3DLine           = nullptr;
		m_pDraw2DLine           = nullptr;
		m_pDraw2DPoint          = nullptr;
		m_pDrawGouraudTriangles = nullptr;
		m_pSetSceneNode         = nullptr;
		m_pFillScreen           = nullptr;
		m_pFlushDrawBuffers     = nullptr;
		m_pBlitFramebuffer      = nullptr;
	}

	// BUG: glClear does not set color in FBO Texture attachment, render a tile to properly apply color correction.
	if ( (RenderLockFlags & LOCKR_ClearScreen) && (ScreenClear.X + ScreenClear.Y + ScreenClear.Z > 0) && MainFramebuffer_FBO && m_pFillScreen )
	{
		// BUG: MSVC forces aligned load on unaligned function parameter
		FPlane FillColor;
		FillColor.X = ScreenClear.X;
		FillColor.Y = ScreenClear.Y;
		FillColor.Z = ScreenClear.Z;
		FillColor.W = 1.0f;
		(this->*m_pFillScreen)( nullptr, &FillColor, 0);
	}

	if ( !UseDrawGouraud469 )
		m_pDrawGouraudTriangles = nullptr;

	unguard;
}

void UOpenGLRenderDevice::SetSceneNode(FSceneNode* Frame)
{
	UTGLR_DEBUG_CALL_COUNT(SetSceneNode);
	guard(UOpenGLRenderDevice::SetSceneNode);

	// Flush vertex array before changing the projection matrix!
	FlushDrawBuffers(); 

	m_sceneNodeCount++;

	//No need to set default AA state here
	//No need to set default projection state as this function always sets/initializes it

	// Precompute stuff.
	FLOAT rcpFrameFX = 1.0f / Frame->FX;
	m_Aspect = Frame->FY * rcpFrameFX;
	m_RProjZ = appTan(Viewport->Actor->FovAngle * PI / 360.0);
	m_RFX2 = 2.0f * m_RProjZ * rcpFrameFX;
	m_RFY2 = 2.0f * m_RProjZ * rcpFrameFX;

#if UTGLR_USES_SCENENODE_HACK
	//Remember Frame->X and Frame->Y for scene node hack
	m_sceneNodeX = Frame->X;
	m_sceneNodeY = Frame->Y;
#endif

	// Is Framebuffer needed for this Viewport?
	if ( Viewport->Actor )
	{
		switch ( Viewport->Actor->RendMap )
		{
		case REN_None:
		case REN_Wire:
		case REN_TexView:
		case REN_TexBrowser:
			break;
		default:
			MainFramebuffer_Required = true;
			break;
		}
	}

	// Render Path
	if ( m_pSetSceneNode )
		(this->*m_pSetSceneNode)(Frame);


	unguard;
}

void UOpenGLRenderDevice::Unlock(UBOOL Blit)
{
	UTGLR_DEBUG_CALL_COUNT(Unlock);
	guard(UOpenGLRenderDevice::Unlock);	

	FlushDrawBuffers();

	// Disable active clip planes
	while ( NumClipPlanes > 0 )
		FOpenGLBase::glDisable( GL_CLIP_DISTANCE0 + (--NumClipPlanes) );

	BlitMainFramebuffer();

	BOOL SwappedBuffers = false;
#if UTGLR_SUPPORT_DXGI_INTEROP
	if (UseLowLatencySwapchain)
		SwappedBuffers = GL->UnlockDXGIFramebuffer();
#endif

	SetDefaultAAState();
	SetDefaultSamplerState();

	if ( GL )
		GL->Unlock();

	// Unlock and render.
	check(LockCount == 1);

	//glFlush();

	if (Blit)// && !SwappedBuffers) // stijn: for some reason, we still have to call SwapBuffers...
	{
		CheckGLErrorFlag(TEXT("please report this bug"));

		//Swap buffers
#ifdef __UNIX__
		guard(SwapWindow)
		SDL_Window* Window = reinterpret_cast<SDL_Window*>(Viewport->GetWindow());
		if ( Window )
			SDL_GL_SwapWindow(Window);
		unguard;
#else
		{
			HDC DC = GL->WindowDC_Get((HWND)Viewport->GetWindow());
			bool SwapBuffersStatus = (SwapBuffers(DC)) ? true : false;

			if (!m_prevSwapBuffersStatus)
				check(SwapBuffersStatus);
			m_prevSwapBuffersStatus = SwapBuffersStatus;
		}
#endif
	}

	TexturePool.Unlock();

	--LockCount;

	//If doing selection, end and return hits
	if (m_HitData) {
		INT i;

		//End selection
		m_gclip.SelectModeEnd();

		*m_HitSize = m_HitCount;

		//Disable clip planes
		for (i = 0; i < 5; i++) {
			m_gclip.SetCpEnable(i, false);
		}
	}

	//Increment current frame count
	m_currentFrameCount++;

#if 0
	dbgPrintf("AA switch count = %u\n", m_AASwitchCount);
	dbgPrintf("Scene node count = %u\n", m_sceneNodeCount);
	dbgPrintf("Scene node hack count = %u\n", m_sceneNodeHackCount);
	dbgPrintf("Stat 0 count = %u\n", m_stat0Count);
	dbgPrintf("Stat 1 count = %u\n", m_stat1Count);
#endif


	unguard;
}


void UOpenGLRenderDevice::DrawComplexSurface( FSceneNode* Frame, FSurfaceInfo& Surface, FSurfaceFacet& Facet)
{
	UTGLR_DEBUG_CALL_COUNT(DrawComplexSurface);
	guard(UOpenGLRenderDevice::DrawComplexSurface);

	ApplySceneNodeHack(Frame);
	if ( !GIsEditor )
		Surface.PolyFlags &= ~(PF_Selected|PF_FlatShaded);
	else
	{
		// Adjust flat shading
		DWORD ColorFlags = Surface.PolyFlags & (PF_Selected|PF_FlatShaded);
		switch (ColorFlags)
		{
		case (PF_Selected|PF_FlatShaded):
			Surface.PolyFlags &= (~PF_Selected);
			Surface.FlatColor.R = Min( (INT)Surface.FlatColor.R*3/2, 255);
			Surface.FlatColor.G = Min( (INT)Surface.FlatColor.G*3/2, 255);
			Surface.FlatColor.B = Min( (INT)Surface.FlatColor.B*3/2, 255);
			Surface.FlatColor.A = 255;
			break;
		case (PF_FlatShaded):
			Surface.FlatColor.A = 216;
			break;
		case (PF_Selected):
			Surface.FlatColor = SurfaceSelectionColor; //FColor(0,0,127,71);
			Surface.PolyFlags &= (~PF_Selected);
			Surface.PolyFlags |= PF_FlatShaded;
			break;
		default:
			break;
		}
	}

	// Adjust PolyFlags
	UBOOL UseMaskedStorage;
	Surface.PolyFlags = FixPolyFlags(Surface.PolyFlags, UseMaskedStorage) & (PF_Masked|PF_Translucent|PF_Invisible|PF_Modulated|PF_Highlighted|PF_AlphaBlend|PF_NoSmooth|PF_Occlude|PF_FlatShaded|PF_Selected);
	if ( ForceNoSmooth )
		Surface.PolyFlags |= PF_NoSmooth;

	// Adjust CacheID
	if ( Surface.Texture )
		FGL::FixCacheID(*Surface.Texture, UseMaskedStorage);

	// Deal with DetailTexture
	// TODO: Deal with this in UnRender
	FTextureInfo* SavedDetailTexture = Surface.DetailTexture;
	if ( !DetailTextures )
		Surface.DetailTexture = nullptr;

	// Render path
	if ( m_pDrawComplexSurface )
		(this->*m_pDrawComplexSurface)(Frame,(FSurfaceInfo_DrawComplex&)Surface,Facet);

	// TODO: Deal with this in UnRender
	if ( !DetailTextures )
		Surface.DetailTexture = SavedDetailTexture;

	unguard;
}

#ifdef UTGLR_RUNE_BUILD
void UOpenGLRenderDevice::PreDrawFogSurface() {
	UTGLR_DEBUG_CALL_COUNT(PreDrawFogSurface);
	guard(UOpenGLRenderDevice::PreDrawFogSurface);

	FlushDrawBuffers();

	SetDefaultAAState();
	SetDefaultColorState();
	SetDefaultShaderState();
	SetDefaultTextureState();

	SetBlend(PF_AlphaBlend);
	SetNoTexture(0);

	unguard;
}

void UOpenGLRenderDevice::PostDrawFogSurface() {
	UTGLR_DEBUG_CALL_COUNT(PostDrawFogSurface);
	guard(UOpenGLRenderDevice::PostDrawFogSurface);

	SetBlend(0);

	unguard;
}

void UOpenGLRenderDevice::DrawFogSurface(FSceneNode* Frame, FFogSurf &FogSurf) {
	UTGLR_DEBUG_CALL_COUNT(DrawFogSurface);
	guard(UOpenGLRenderDevice::DrawFogSurface);

	FPlane Modulate(FogSurf.FogColor.X, FogSurf.FogColor.Y, FogSurf.FogColor.Z, 0.0f);

	FLOAT RFogDistance = 1.0f / FogSurf.FogDistance;

	if (FogSurf.PolyFlags & PF_Masked) {
		glDepthFunc(GL_EQUAL);
	}

	for (FSavedPoly* Poly = FogSurf.Polys; Poly; Poly = Poly->Next) {
		glBegin(GL_TRIANGLE_FAN);
		for (INT i = 0; i < Poly->NumPts; i++) {
			Modulate.W = Poly->Pts[i]->Point.Z * RFogDistance;
			if (Modulate.W > 1.0f) {
				Modulate.W = 1.0f;
			}
			else if (Modulate.W < 0.0f) {
				Modulate.W = 0.0f;
			}
			glColor4fv(&Modulate.X);
			glVertex3fv(&Poly->Pts[i]->Point.X);
		}
		glEnd();
	}

	if (FogSurf.PolyFlags & PF_Masked) {
		glDepthFunc(ZTrickFunc);
	}

	unguard;
}

void UOpenGLRenderDevice::PreDrawGouraud(FSceneNode* Frame, FLOAT FogDistance, FPlane FogColor) {
	UTGLR_DEBUG_CALL_COUNT(PreDrawGouraud);
	guard(UOpenGLRenderDevice::PreDrawGouraud);

	if (FogDistance > 0.0f) {
		FlushDrawBuffers();

		//Enable fog
		m_gpFogEnabled = true;
		FOpenGLBase::glEnable(GL_FOG);

//		glFogi(GL_FOG_MODE, GL_LINEAR);
		glFogfv(GL_FOG_COLOR, &FogColor.X);
//		glFogf(GL_FOG_START, 0.0f);
		glFogf(GL_FOG_END, FogDistance);
	}

	unguard;
}

void UOpenGLRenderDevice::PostDrawGouraud(FLOAT FogDistance) {
	UTGLR_DEBUG_CALL_COUNT(PostDrawGouraud);
	guard(UOpenGLRenderDevice::PostDrawGouraud);

	if (FogDistance > 0.0f) {
		FlushDrawBuffers();

		//Disable fog
		m_gpFogEnabled = false;
		FOpenGLBase::glDisable(GL_FOG);
	}

	unguard;
}
#endif

void UOpenGLRenderDevice::DrawGouraudPolygon( FSceneNode* Frame, FTextureInfo& Info, FTransTexture** Pts, INT NumPts, DWORD PolyFlags, FSpanBuffer* Span)
{
	UTGLR_DEBUG_CALL_COUNT(DrawGouraudPolygon);
	guard(UOpenGLRenderDevice::DrawGouraudPolygon);
	
	//Reject invalid polygons early so that other parts of the code do not have to deal with them
	if (NumPts < 3)
		return;

	ApplySceneNodeHack(Frame);

	// Adjust PolyFlags
	UBOOL UseMaskedStorage;
	PolyFlags = FixPolyFlags(PolyFlags, UseMaskedStorage);
	if ( !(PolyFlags & PF_Modulated) )
		PolyFlags |= PF_Gouraud;
	if ( PolyFlags & PF_Translucent )
		PolyFlags &= ~PF_RenderFog;
	if ( ForceNoSmooth )
		PolyFlags |= PF_NoSmooth;
	if ( GUglyHackFlags & HACKFLAGS_NoNearZ )
		PolyFlags |= PF_NoNearZ;

	// Adjust CacheID
	FGL::FixCacheID(Info, UseMaskedStorage);

	// Make surface
	FSurfaceInfo_DrawGouraud Surface;
	Surface.PolyFlags     = PolyFlags;
	Surface.Texture       = &Info;
	if ( DetailTextures && m_DGT )
		Surface.DetailTexture = FGL::Draw::LockDetailTexture(Info);
	else
		Surface.DetailTexture = nullptr;

	// Render path
	if ( m_pDrawGouraudPolygon )
		(this->*m_pDrawGouraudPolygon)(Frame,Surface,Pts,NumPts);

	unguard;
}

void UOpenGLRenderDevice::DrawTile(FSceneNode* Frame, FTextureInfo& Info, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, class FSpanBuffer* Span, FLOAT Z, FPlane Color, FPlane Fog, DWORD PolyFlags)
{
	UTGLR_DEBUG_CALL_COUNT(DrawTile);
	guard(UOpenGLRenderDevice::DrawTile);

	// stijn: fix for invisible actor icons in ortho viewports
	if (GIsEditor &&
		Frame->Viewport->Actor &&
		(Frame->Viewport->IsOrtho() || Abs(Z) <= SMALL_NUMBER))
	{
		Z = 1.f;
	}

	ApplySceneNodeHack(Frame);

	// Adjust PolyFlags
	UBOOL UseMaskedStorage;
	PolyFlags = FixPolyFlags(PolyFlags, UseMaskedStorage) & ~PF_TwoSided;
	if ( !(PolyFlags & PF_Modulated) )
		PolyFlags |= PF_Gouraud;

	// Adjust CacheID
	FGL::FixCacheID(Info, UseMaskedStorage);

	// Make surface
	FSurfaceInfo_DrawTile Surface;
	Surface.PolyFlags       = PolyFlags;
	*(DWORD*)&Surface.Color = FPlaneTo_RGBAClamped(&Color);
//	*(DWORD*)&Surface.Fog   = FPlaneTo_RGBAClamped(&Fog);
	Surface.Texture         = &Info;

	// Render path
	if ( m_pDrawTile )
		(this->*m_pDrawTile)(Frame,Surface,X,Y,XL,YL,U,V,UL,VL,Z);

	unguard;
}

void UOpenGLRenderDevice::Draw3DLine(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2)
{
	UTGLR_DEBUG_CALL_COUNT(Draw3DLine);
	guard(UOpenGLRenderDevice::Draw3DLine);

	// Render path
	if ( m_pDraw3DLine )
		(this->*m_pDraw3DLine)(Frame,Color,LineFlags,P1,P2);

	unguard;
}

void UOpenGLRenderDevice::Draw2DLine(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2)
{
	UTGLR_DEBUG_CALL_COUNT(Draw2DLine);
	guard(UOpenGLRenderDevice::Draw2DLine);

	// Render path
	if ( m_pDraw2DLine )
		(this->*m_pDraw2DLine)(Frame,Color,LineFlags,P1,P2);

	unguard;
}

void UOpenGLRenderDevice::Draw2DPoint(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FLOAT X1, FLOAT Y1, FLOAT X2, FLOAT Y2, FLOAT Z)
{
	UTGLR_DEBUG_CALL_COUNT(Draw2DPoint);
	guard(UOpenGLRenderDevice::Draw2DPoint);

	// Higor:
	// Absurd bug in Editor, Z is the actual point's Z location in world so anything under the mid plane won't draw.
	// This also messes up Hit testing!!
	if ( GIsEditor )
		Z = 1.0f;

	// Render path
	if ( m_pDraw2DPoint )
		(this->*m_pDraw2DPoint)(Frame,Color,LineFlags,X1,Y1,X2,Y2,Z);

	unguard;
}

void UOpenGLRenderDevice::DrawGouraudTriangles( const FSceneNode* Frame, const FTextureInfo& Info, FTransTexture* const Pts, INT NumPts, DWORD PolyFlags, DWORD DataFlags, FSpanBuffer* Span)
{
	guard(UOpenGLRenderDevice::DrawGouraudTriangles);

	// TODO: Evaluate removing this
	if ( NumPts < 3 )
		return;

	m_DGT = 1;
	// Render path
	if ( m_pDrawGouraudTriangles )
	{
		// Adjust PolyFlags
		UBOOL UseMaskedStorage;
		PolyFlags = FixPolyFlags(PolyFlags, UseMaskedStorage);
		if ( !(PolyFlags & PF_Modulated) )
			PolyFlags |= PF_Gouraud;
		if ( PolyFlags & PF_Translucent )
			PolyFlags &= ~PF_RenderFog;
		if ( ForceNoSmooth )
			PolyFlags |= PF_NoSmooth;
		if ( GUglyHackFlags & HACKFLAGS_NoNearZ )
			PolyFlags |= PF_NoNearZ;

		// Adjust CacheID
		FGL::FixCacheID(((FTextureInfo&)Info), UseMaskedStorage);

		// Make Surface
		FSurfaceInfo_DrawGouraudTris Surface;
		Surface.PolyFlags     = PolyFlags;
		Surface.Texture       = (FTextureInfo*)&Info;
		Surface.DetailTexture = FGL::Draw::LockDetailTexture(Info);
		Surface.ClipPlaneID   = 0;

		(this->*m_pDrawGouraudTriangles)(Frame,Surface,Pts,NumPts,DataFlags);
	}
	else // Default to DrawGouraudPolygon calls when no Render Path is active
		Super::DrawGouraudTriangles( Frame, Info, Pts, NumPts, PolyFlags, DataFlags, Span);
	m_DGT = 0;

	unguard;
}


void UOpenGLRenderDevice::ClearZ(FSceneNode* Frame)
{
	UTGLR_DEBUG_CALL_COUNT(ClearZ);
	guard(UOpenGLRenderDevice::ClearZ);

	FlushDrawBuffers();

	SetBlend(PF_Occlude);
	FOpenGLBase::glClear(GL_DEPTH_BUFFER_BIT);

	unguard;
}

void UOpenGLRenderDevice::PushHit(const BYTE* Data, INT Count) {
	guard(UOpenGLRenderDevice::PushHit);

	INT i;

	FlushDrawBuffers();

	//Add to stack
	for (i = 0; i < Count; i += 4) {
		DWORD hitName = *(DWORD *)(Data + i);
		m_gclip.PushHitName(hitName);
	}

	unguard;
}

void UOpenGLRenderDevice::PopHit(INT Count, UBOOL bForce) {
	guard(UOpenGLRenderDevice::PopHit);

	FlushDrawBuffers();

	INT i;
	bool selHit;

	//Handle hit
	selHit = m_gclip.CheckNewSelectHit();
	if (selHit || bForce) {
		size_t nHitNameBytes;

		nHitNameBytes = m_gclip.GetHitNameStackSize() * 4;
		if (nHitNameBytes <= m_HitBufSize) {
			m_gclip.GetHitNameStackValues((unsigned int *)m_HitData, nHitNameBytes / 4);
			m_HitCount = static_cast<INT>(nHitNameBytes);
		}
		else {
			m_HitCount = 0;
		}
	}

	//Remove from stack
	for (i = 0; i < Count; i += 4) {
		m_gclip.PopHitName();
	}

	unguard;
}

void UOpenGLRenderDevice::GetStats(TCHAR* Result) {
	guard(UOpenGLRenderDevice::GetStats);

	double msPerCycle = GSecondsPerCycleLong * 1000.0;
	appSprintf( // stijn: mem safety NOT OK
		Result,
		TEXT("OpenGL stats: Bind=%04.1f Image=%04.1f Complex=%04.1f Gouraud=%04.1f Tile=%04.1f"),
		msPerCycle * BindCycles,
		msPerCycle * ImageCycles,
		msPerCycle * ComplexCycles,
		msPerCycle * GouraudCycles,
		msPerCycle * TileCycles
	);

	unguard;
}

void UOpenGLRenderDevice::ReadPixels(FColor* Pixels) {
	guard(UOpenGLRenderDevice::ReadPixels);

	INT x, y;
	INT SizeX, SizeY;

	SizeX = Viewport->SizeX;
	SizeY = Viewport->SizeY;

	FOpenGLBase::glReadPixels(0, 0, SizeX, SizeY, GL_RGBA, GL_UNSIGNED_BYTE, Pixels);
	for (y = 0; y < SizeY / 2; y++) {
		for (x = 0; x < SizeX; x++) {
			DWORD& A = *(DWORD*)&Pixels[x + y * SizeX];
			DWORD& B = *(DWORD*)&Pixels[x + (SizeY - 1 - y) * SizeX];
			DWORD  T = A;
			// Swap B, R and set A=1
			A = RGBA_to_BGRA(B) | 0xFF000000; 
			B = RGBA_to_BGRA(T) | 0xFF000000;
//			Exchange(Pixels[x + y * SizeX].R, Pixels[x + (SizeY - 1 - y) * SizeX].B);
//			Exchange(Pixels[x + y * SizeX].G, Pixels[x + (SizeY - 1 - y) * SizeX].G);
//			Exchange(Pixels[x + y * SizeX].B, Pixels[x + (SizeY - 1 - y) * SizeX].R);
		}
	}

	//Gamma correct screenshots if the option is true and the gamma ramp was set successfully
	if (GammaCorrectScreenshots && m_setGammaRampSucceeded) {
		FByteGammaRamp gammaByteRamp;
		BuildGammaRamp(SavedGammaCorrection, SavedGammaCorrection, SavedGammaCorrection, Brightness, gammaByteRamp);
		for (y = 0; y < SizeY; y++) {
			for (x = 0; x < SizeX; x++) {
				Pixels[x + y * SizeX].R = gammaByteRamp.red[Pixels[x + y * SizeX].R];
				Pixels[x + y * SizeX].G = gammaByteRamp.green[Pixels[x + y * SizeX].G];
				Pixels[x + y * SizeX].B = gammaByteRamp.blue[Pixels[x + y * SizeX].B];
			}
		}
	}

	unguard;
}

void UOpenGLRenderDevice::EndFlash()
{
	UTGLR_DEBUG_CALL_COUNT(EndFlash);
	guard(UOpenGLRenderDevice::EndFlash);

	m_shaderAlphaHUD = (ShaderAlphaHUD != 0);

	// stijn: Render calls EndFlash when it is done drawing the world.
	// Render path
	if ( (FlashScale != FPlane(0.5f, 0.5f, 0.5f, 0.0f)) || (FlashFog != FPlane(0.0f, 0.0f, 0.0f, 0.0f)) )
	{
		if ( m_pFillScreen )
		{
			FPlane FogColor( FlashFog.X, FlashFog.Y, FlashFog.Z, 1.0f - Min(FlashScale.X * 2.0f, 1.0f));
			(this->*m_pFillScreen)( nullptr, &FogColor, PF_Highlighted);
		}
	}


	unguard;
}


void UOpenGLRenderDevice::SetBlendNoCheck(DWORD blendFlags) {
	guardSlow(UOpenGLRenderDevice::SetBlend);

	// Detect changes in the blending modes.
	DWORD Xor = m_curBlendFlags ^ blendFlags;

	//Save copy of current blend flags
	DWORD curBlendFlags = m_curBlendFlags;

	//Update main copy of current blend flags early
	m_curBlendFlags = blendFlags;

	const DWORD PF_Masked_Conditional = m_SmoothMasking ? PF_Masked : 0;
	const DWORD GL_BLEND_FLAG_BITS = PF_Translucent | PF_Modulated | PF_Highlighted | PF_AlphaBlend | PF_Masked_Conditional;
	if (Xor & GL_BLEND_FLAG_BITS) {
		if (!(blendFlags & GL_BLEND_FLAG_BITS)) {
			FOpenGLBase::glDisable(GL_BLEND);
		}
		else {
			if (!(curBlendFlags & GL_BLEND_FLAG_BITS)) {
				FOpenGLBase::glEnable(GL_BLEND);
			}
			if (blendFlags & PF_Translucent) {
				FOpenGLBase::glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);				
			}
			else if (blendFlags & PF_Modulated) {
				FOpenGLBase::glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
			}
			else if (blendFlags & PF_Highlighted) {
				FOpenGLBase::glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			}
			else if (blendFlags & (PF_AlphaBlend|PF_Masked_Conditional)) {
				FOpenGLBase::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}
		}
	}
	if (Xor & PF_Invisible) {
		UBOOL flag = ((blendFlags & PF_Invisible) == 0) ? GL_TRUE : GL_FALSE;
		FOpenGLBase::glColorMask(flag, flag, flag, flag);
	}
	if (Xor & PF_Occlude) {
		UBOOL flag = ((blendFlags & PF_Occlude) == 0) ? GL_FALSE : GL_TRUE;
		FOpenGLBase::glDepthMask(flag);
	}
	if (Xor & PF_NoZReject) {
		UBOOL flag = ((blendFlags & PF_NoZReject) == 0) ? GL_LEQUAL : GL_ALWAYS;
		FOpenGLBase::glDepthFunc(flag);
	}

	unguardSlow;
}

void UOpenGLRenderDevice::SetAAStateNoCheck(bool AAEnable)
{
	//Save new AA state
	m_curAAEnable = AAEnable;
	m_AASwitchCount++;

	//Set new AA state
	if (AAEnable)
		FOpenGLBase::glEnable(GL_MULTISAMPLE_ARB);
	else
		FOpenGLBase::glDisable(GL_MULTISAMPLE_ARB);
}

// TODO: This is part of rendering specification
inline DWORD UOpenGLRenderDevice::FixPolyFlags( DWORD PolyFlags, UBOOL& UseMaskedStorage)
{
	UseMaskedStorage = (PolyFlags & PF_Masked) 
		&& ((PolyFlags & PF_Translucent) || !(PolyFlags & (PF_Highlighted|PF_AlphaBlend)));

	PolyFlags &= ~PF_Clear;
	if ( PolyFlags & (PF_Translucent|PF_Highlighted|PF_AlphaBlend) )
		PolyFlags &= ~PF_Masked;
	if ( PolyFlags & PF_Translucent )
		PolyFlags &= ~(PF_Modulated|PF_Highlighted|PF_AlphaBlend);
	return PolyFlags;
}

inline void UOpenGLRenderDevice::ApplySceneNodeHack( FSceneNode* Frame)
{
#if UTGLR_USES_SCENENODE_HACK
	if ( SceneNodeHack )
	{
		if ( (Frame->X != m_sceneNodeX) || (Frame->Y != m_sceneNodeY))
		{
			m_sceneNodeHackCount++;
			SetSceneNode(Frame);
		}
	}
#endif
}




// Static variables.
INT UOpenGLRenderDevice::NumDevices = 0;
INT UOpenGLRenderDevice::LockCount  = 0;
#ifdef __WIN32__
#else
UBOOL UOpenGLRenderDevice::GLLoaded = false;
#endif

bool UOpenGLRenderDevice::g_gammaFirstTime = false;
bool UOpenGLRenderDevice::g_haveOriginalGammaRamp = false;
#ifdef __WIN32__
UOpenGLRenderDevice::FGammaRamp UOpenGLRenderDevice::g_originalGammaRamp;
#endif

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
