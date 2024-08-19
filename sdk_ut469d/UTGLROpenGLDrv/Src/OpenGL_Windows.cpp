/*=============================================================================
	OpenGL_Windows.cpp

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

#include "OpenGLDrv.h"
#include "UTGLROpenGL.h"

#if _WIN32

/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

ATOM FOpenGLBase::DummyWndClass = 0;
static constexpr TCHAR* DummyWndClassName = TEXT("FOpenGLBase::Init");

HDC                FOpenGLBase::CurrentDC = nullptr;
TMapExt<void*,HDC> FOpenGLBase::WindowDC;


/*-----------------------------------------------------------------------------
	Private utils
-----------------------------------------------------------------------------*/

inline DWORD GetTypeHash( const void* A )
{
	return *(DWORD*)&A;
}

//
// Manage device contexts, prevent a window from using more than one
//
HDC FOpenGLBase::WindowDC_Get( void* Window)
{
	HDC* FoundDC = FOpenGLBase::WindowDC.Find(Window);
	if ( FoundDC )
		return *FoundDC;
	return FOpenGLBase::WindowDC.SetNoFind(Window, GetDC((HWND)Window));
}

void FOpenGLBase::WindowDC_Release( void* Window)
{
	HDC* FoundDC = FOpenGLBase::WindowDC.Find(Window);
	if ( FoundDC )
	{
		ReleaseDC((HWND)Window,*FoundDC);
		FOpenGLBase::WindowDC.Remove(Window);
	}
}


/*-----------------------------------------------------------------------------
	Initialization.
	Moved from OpenGL.cpp
-----------------------------------------------------------------------------*/

void UOpenGLRenderDevice::SetBasicPixelFormat(INT NewColorBytes)
{
	// Set res.
	INT nPixelFormat;
	BYTE DesiredColorBits   = (NewColorBytes <= 2) ? 16 : 32;
	BYTE DesiredDepthBits   = 24;
	BYTE DesiredStencilBits = 0;
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

	if (DebugBit(DEBUG_BIT_BASIC)) dbgPrintf("utglr: BasicInit\n");

	HDC DC = FOpenGLBase::WindowDC_Get(Viewport->GetWindow());
	check(DC);

	nPixelFormat = ChoosePixelFormat(DC, &pfd);
	if (!nPixelFormat)
	{
		pfd.cDepthBits = 24;
		nPixelFormat = ChoosePixelFormat(DC, &pfd);
	}
	if (!nPixelFormat)
	{
		pfd.cDepthBits = 16;
		nPixelFormat = ChoosePixelFormat(DC, &pfd);
	}

	Parse(appCmdLine(), TEXT("PIXELFORMAT="), nPixelFormat);
	debugf(NAME_Init, TEXT("Using pixel format %i"), nPixelFormat);
	check(nPixelFormat);

	verify(SetPixelFormat(DC, nPixelFormat, &pfd));

	//Get actual number of depth bits
	if (DescribePixelFormat(DC, nPixelFormat, sizeof(pfd), &pfd))
		m_numDepthBits = pfd.cDepthBits;
}

bool UOpenGLRenderDevice::SetAAPixelFormat(INT NewColorBytes)
{
	if ( !FOpenGLBase::wglChoosePixelFormatARB )
	{
		m_usingAA = false;
		m_initNumAASamples = 1;
		SetBasicPixelFormat(NewColorBytes);
		return false;
	}

	guard(UOpenGLRenderDevice::SetAAPixelFormat);

	BOOL bRet;
	int iFormats[1];
	UINT nNumFormats;
	PIXELFORMATDESCRIPTOR tempPfd;
	BYTE DesiredColorBits = (NewColorBytes <= 2) ? 16 : 32;
	BYTE DesiredDepthBits = 24;

	INT iPixelFormatAttribList[] =
	{
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB, DesiredColorBits,
		WGL_DEPTH_BITS_ARB, DesiredDepthBits,
		WGL_STENCIL_BITS_ARB, 0,//DesiredStencilBits,
		WGL_SAMPLE_BUFFERS_ARB, UseAA,
		WGL_SAMPLES_ARB, UseAA ? NumAASamples : 1,
		0 // End of attributes list
	};
	INT& attrib_DepthBits = iPixelFormatAttribList[11];
	INT& attrib_AA        = iPixelFormatAttribList[15];
	INT& attrib_AASamples = iPixelFormatAttribList[17];

	// AA cannot be changed on already initialized windows
	if ( m_initNumAASamples != INDEX_NONE )
	{
		attrib_AA        = m_initNumAASamples > 1;
		attrib_AASamples = m_initNumAASamples;
	}

	// Higor: nVidia drivers crash if we pass WGL_SAMPLE_BUFFERS_ARB=0 and WGL_SAMPLES_ARB=1
	if ( !attrib_AA )
	{
		iPixelFormatAttribList[14] = 0;
	}

	HDC DC = FOpenGLBase::WindowDC_Get(Viewport->GetWindow());
	check(DC);

AGAIN:
	bRet = FOpenGLBase::wglChoosePixelFormatARB(DC, iPixelFormatAttribList, NULL, 1, iFormats, &nNumFormats);
	if ((bRet == FALSE) || (nNumFormats == 0))
	{
		attrib_DepthBits = 24;
		bRet = FOpenGLBase::wglChoosePixelFormatARB(DC, iPixelFormatAttribList, NULL, 1, iFormats, &nNumFormats);
	}
	if ((bRet == FALSE) || (nNumFormats == 0))
	{
		attrib_DepthBits = 16;
		bRet = FOpenGLBase::wglChoosePixelFormatARB(DC, iPixelFormatAttribList, NULL, 1, iFormats, &nNumFormats);
	}
	if ((bRet == FALSE) || (nNumFormats == 0))
	{
		// Lower AA until one is supported
		if ( attrib_AA )
		{
			debugf( NAME_Init, TEXT("wglChoosePixelFormatARB: Unable to choose %i AA samples"), attrib_AASamples);
			attrib_DepthBits = DesiredDepthBits;
			attrib_AASamples = Max<INT>((attrib_AASamples >> 1), 1);
			if ( attrib_AASamples == 1 )
				attrib_AA = GL_FALSE;
			goto AGAIN;
		}
		if (DebugBit(DEBUG_BIT_BASIC))
			dbgPrintf("utglr: AAInit failed\n");
		return false;
	}

	appMemzero(&tempPfd, sizeof(tempPfd));
	tempPfd.nSize = sizeof(tempPfd);
	verify(SetPixelFormat(DC, iFormats[0], &tempPfd));

	//Get actual number of depth bits
	PFNWGLGETPIXELFORMATATTRIBIVARBPROC p_wglGetPixelFormatAttribivARB = reinterpret_cast<PFNWGLGETPIXELFORMATATTRIBIVARBPROC>(FOpenGLBase::wglGetProcAddress("wglGetPixelFormatAttribivARB"));
	if ( p_wglGetPixelFormatAttribivARB != NULL )
	{
		int iAttribute = WGL_DEPTH_BITS_ARB;
		int iValue = m_numDepthBits;

		if ( p_wglGetPixelFormatAttribivARB(DC, iFormats[0], 0, 1, &iAttribute, &iValue) )
			m_numDepthBits = iValue;
	}

	// AA cannot be changed on already initialized windows
	if ( m_initNumAASamples == INDEX_NONE )
	{
		m_usingAA          = !!attrib_AA;
		m_initNumAASamples = attrib_AA ? attrib_AASamples : 1;
	}
	return true;

	unguard;
}


/*-----------------------------------------------------------------------------
	FOpenGLBase initialization.
	All contexts share lists.
-----------------------------------------------------------------------------*/

HMODULE FOpenGLBase::hOpenGL32 = nullptr;

// Declare WGL procs
#define GLEntry(type, name) type FOpenGLBase::name = nullptr;
GL_ENTRYPOINTS_DLL_WGL(GLEntry)
GL_ENTRYPOINTS_WGL(GLEntry)
#undef GLEntry

// Declare WGL exts
#define GLExt(name) bool FOpenGLBase::SUPPORTS##name = false;
GL_EXTENSIONS_WGL(GLExt)
#undef GLExt




bool FOpenGLBase::Init()
{
	guard(FOpenGLBase::Init);

	static bool AttemptedInit = false;
	if ( AttemptedInit )
		return (wglChoosePixelFormatARB != nullptr);
	AttemptedInit = true;

	// Load OpenGL32.dll
	if ( !hOpenGL32 && (hOpenGL32=LoadLibraryA(GL_DLL)) == nullptr )
	{
		debugf( NAME_Init, LocalizeError("NoFindGL"), appFromAnsi(GL_DLL));
		return false;
	}

	// Load functions
	#define GLEntry(type,name) name = (type)GetProcAddress(hOpenGL32,#name);
	GL_ENTRYPOINTS_DLL_WGL(GLEntry)
	#undef GLEntry

	// Verify functions
	bool Success = true;
	#define GLEntry(type,name) \
	if ( name == nullptr ) \
	{ \
		debugf( NAME_Init, LocalizeError("MissingFunc"), appFromAnsi(#name)); \
		Success = false; \
	}
	GL_ENTRYPOINTS_DLL_WGL(GLEntry)
	#undef GLEntry
	if ( !Success )
	{
		FreeLibrary(hOpenGL32);
		hOpenGL32 = nullptr;
		return false;
	}


	HINSTANCE hInstance = GetModuleHandleW(NULL);

	// Initialize and register a dummy Window class
	if ( !DummyWndClass )
	{
		WNDCLASS WC;
		appMemzero(&WC,sizeof(WC));
		WC.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
		WC.lpfnWndProc = DefWindowProcW;
		WC.hInstance = hInstance;
		WC.lpszClassName = DummyWndClassName;
		WC.lpfnWndProc = DefWindowProc;
		DummyWndClass = RegisterClass(&WC);
		if ( !DummyWndClass )
		{
			debugf( NAME_Init, TEXT("RegisterClass() failed.") );
			return false;
		}
	}

	// Create window
	HWND DummyHWND = CreateDummyWindow();
	if ( !DummyHWND )
		return false;

	HDC DummyDC = FOpenGLBase::WindowDC_Get(DummyHWND);
	const TCHAR* Error = nullptr;
	HGLRC DummyRC;

	// Create context and get WGL extension functions
	if ( (DummyRC=wglCreateContext(DummyDC)) == 0 )
		Error = TEXT("wglCreateContext() failed.");
	else if ( wglMakeCurrent( DummyDC, DummyRC) == 0 )
		Error = TEXT("wglMakeCurrent() failed.");
	else
	{
		// Load relevant functions
		#define GLEntry(type,name) name = (type)GetGLProc(#name);
		GL_ENTRYPOINTS_WGL(GLEntry)
		#undef GLEntry

		if ( !InitProcs() )
			Error = TEXT("Failed to initialize OpenGL common interface");
		else if ( !wglChoosePixelFormatARB )
			Error = TEXT("wglGetProcAddress() failed.");
		else
		{
			const char *WGLExtensions;
			if ( wglGetExtensionsStringARB
				&& (WGLExtensions=wglGetExtensionsStringARB(DummyDC)) != nullptr )
			{
				// Get relevant extensions
				#define GLExt(name) SUPPORTS##name = UOpenGLRenderDevice::IsGLExtensionSupported( WGLExtensions, &((#name)[1]));
				GL_EXTENSIONS_WGL(GLExt)
				#undef GLExt

				if ( !SUPPORTS_WGL_ARB_multisample )	debugf( NAME_Warning, TEXT("Device does not support Multisampling AA.") );
				if ( !SUPPORTS_WGL_EXT_swap_control )	debugf( NAME_Warning, TEXT("Device does not support VSync.") );
			}
//			InitCapabilities();
		}
		wglMakeCurrent( nullptr, nullptr);
		wglDeleteContext(DummyRC);
	}
	ReleaseDC(DummyHWND, DummyDC);
	DestroyWindow(DummyHWND);

	if ( Error != nullptr )
	{
		GLog->Log(Error);
		return false;
	}

	return true;
	unguard;
}

HWND FOpenGLBase::CreateDummyWindow()
{
	HINSTANCE hInstance = GetModuleHandleW(NULL);

	// Create window
	HWND DummyHWND = CreateWindow(
		DummyWndClassName, DummyWndClassName,
		WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		0, 0, 1, 1,
		nullptr, nullptr, hInstance, nullptr);

	if ( DummyHWND == nullptr )
	{
		debugf( NAME_Init, TEXT("CreateDummyWindow: CreateWindow() failed.") );
		return nullptr;
	}

	HDC DummyDC = FOpenGLBase::WindowDC_Get(DummyHWND);

	// Enable Hardware acceleration
	PIXELFORMATDESCRIPTOR PFD;
	appMemzero( &PFD, sizeof(PFD));
	PFD.nSize = sizeof(PFD);
	PFD.nVersion = 1;
	PFD.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	PFD.iPixelType = PFD_TYPE_RGBA;
	PFD.cColorBits = 32;
	PFD.cDepthBits = 24;
	PFD.cStencilBits = 8;

	INT PFDID = ChoosePixelFormat( DummyDC, &PFD);
	if ( PFDID == 0 )
	{
		debugf( NAME_Init, TEXT("CreateDummyWindow: ChoosePixelFormat() failed.") );
		FOpenGLBase::WindowDC_Release(DummyHWND);
		DestroyWindow(DummyHWND);
		return nullptr;
	}

	if ( SetPixelFormat( DummyDC, PFDID, &PFD) == 0 )
	{
		debugf( NAME_Init, TEXT("CreateDummyWindow: SetPixelFormat() failed.") );
		FOpenGLBase::WindowDC_Release(DummyHWND);
		DestroyWindow(DummyHWND);
		return nullptr;
	}

	return DummyHWND;
}


//
// Persistent GL context attributes
//
struct GLAttrib // Could use a generic TPair class
{
	GLenum Key;
	GLenum Value;

	bool operator==( const GLAttrib& Other) const { return Key == Other.Key; }
};
static TArray<GLAttrib> ContextAttribs; // Could use a TStaticArray of 6 elements...


void* FOpenGLBase::CreateContext( void* Window)
{
	guard(FOpenGLBase::CreateContext);
	HGLRC Context = nullptr;
	HDC   DC      = FOpenGLBase::WindowDC_Get(Window);;
	if ( DC )
	{
		if ( wglCreateContextAttribsARB )
		{
			GLAttrib Attribs[8] = {0};
			GLint i;
			for ( i=0; i<ContextAttribs.Num(); i++)
				Attribs[i] = ContextAttribs(i);
			// Dangerous
//			Attribs[i++] = GLAttrib{ WGL_CONTEXT_OPENGL_NO_ERROR_ARB, GL_TRUE};

			Context = wglCreateContextAttribsARB(DC, nullptr, (int*)Attribs);
		}
		else
			Context = wglCreateContext(DC);

		// Attempt to setup with sharing
		if ( Context )
		{
			if ( Instances.Num() )
			{
				bool ShareSuccess = true;
				for ( INT i=0; i<Instances.Num(); i++)
					if ( Instances(i)->Context )
					{
						ShareSuccess = wglShareLists((HGLRC)Instances(i)->Context, Context) != 0;
						if ( ShareSuccess )
							break;
					}
				if ( !ShareSuccess )
				{
					debugf( NAME_Init, TEXT("OpenGL context error, unable to share lists") );
					goto CONTEXT_SETUP_FAILURE;
				}
			}

			if ( !wglMakeCurrent(DC, Context) )
			{
				debugf( NAME_Init, TEXT("OpenGL context error, unable to make current") );
				goto CONTEXT_SETUP_FAILURE;
			}
			CurrentWindow = Window;
			CurrentContext = Context;

			// Ugly, reacquire procs and extensions.
			if ( !Instances.Num() )
				InitProcs(true);

			if ( false )
			{
				CONTEXT_SETUP_FAILURE:
				wglDeleteContext(Context);
				Context = nullptr;
			}
		}

	}

	if ( !Context )
		FOpenGLBase::WindowDC_Release(Window);

	return (void*)Context;
	unguard;
}

void FOpenGLBase::DeleteContext( void* Context)
{
	guard(FOpenGLBase::DeleteContext);
	if ( Context )
	{
		if ( Context == CurrentContext )
			wglMakeCurrent(nullptr, nullptr);
		wglDeleteContext((HGLRC)Context);

		for ( INT i=0; i<Instances.Num(); i++ )
			if ( Instances(i)->Context == Context )
			{
				FOpenGLBase::WindowDC_Release(Instances(i)->Window);
				Instances(i)->Context = nullptr;
			}
	}
	unguard;
}

bool FOpenGLBase::MakeCurrent( void* OnWindow)
{
	guard(FOpenGLBase::MakeCurrent);

	// Unreal Editor may run different OpenGL render devices, get actual current context.
	if ( GIsEditor )
		CurrentContext = wglGetCurrentContext();

	if ( !OnWindow )
	{
		if ( CurrentContext )
		{
			CurrentWindow  = nullptr;
			CurrentContext = nullptr;
			CurrentDC      = nullptr;
			wglMakeCurrent(nullptr, nullptr);
		}
		ActiveInstance = nullptr;
	}
	else 
	{
		if ( Context && (Context != CurrentContext || OnWindow != CurrentWindow) )
		{
			// Same Context, different window!!
			if ( OnWindow != Window )
			{
				WindowDC_Release(Window);
				Window = OnWindow;
			}
			HDC DC = WindowDC_Get(Window);
			if ( !DC || !wglMakeCurrent(DC, (HGLRC)Context) )
			{
				if ( CurrentContext || CurrentWindow )
				{
					CurrentWindow = nullptr;
					CurrentContext = nullptr;
					ActiveInstance = nullptr;
					wglMakeCurrent(nullptr, nullptr);
					debugf(NAME_DevGraphics, TEXT("OpenGLDrv: wglMakeCurrent failure."));
				}
				return false;
			}
			CurrentWindow = Window;
			CurrentContext = Context;
		}
		ActiveInstance = this;
	}
	return true;

	unguard;
}

void FOpenGLBase::SetContextAttribute( FGL::EContextAttribute Attribute, DWORD Value)
{
	guard(FOpenGLBase::SetContextAttribute);

	bool Add = false;
	GLAttrib Attrib{ 0, Value};

	switch ( Attribute )
	{
	case FGL::EContextAttribute::PROFILE:
		Attrib.Key = WGL_CONTEXT_PROFILE_MASK_ARB;
		Add = (Value != FGL::PROFILE_UNDEFINED);
		break;
	case FGL::EContextAttribute::MAJOR_VERSION:
		Attrib.Key = WGL_CONTEXT_MAJOR_VERSION_ARB;
		Add = (Value != 0);
		break;
	case FGL::EContextAttribute::MINOR_VERSION:
		Attrib.Key = WGL_CONTEXT_MINOR_VERSION_ARB;
		Add = (Value != 0);
		break;
	}

	ContextAttribs.RemoveItem(Attrib); // Equivalent to remove by key
	if ( Add )
		ContextAttribs.AddItem(Attrib);

	unguard;
}

#endif /*_WIN32 */