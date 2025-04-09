/*=============================================================================
	OpenGL_Unix.cpp

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

#include "OpenGLDrv.h"
#include "UTGLROpenGL.h"

#if !_WIN32

/*-----------------------------------------------------------------------------
	FOpenGLBase initialization.
-----------------------------------------------------------------------------*/

bool FOpenGLBase::GLLoaded = false;

bool FOpenGLBase::Init()
{
	guard(FOpenGLBase::Init);

	static bool AttemptedInit = false;
	if ( AttemptedInit )
		return GLLoaded;
	AttemptedInit = true;

	// Bind the library.
	FString OpenGLLibName;

	// Default to libGL.so.1 if not defined
	if ( !GConfig->GetString(UOpenGLRenderDevice::g_pSection, TEXT("OpenGLLibName"), OpenGLLibName) )
	{
#if MACOSX
		OpenGLLibName = TEXT("/System/Library/Frameworks/OpenGL.framework/Libraries/libGL.dylib");
#else
		OpenGLLibName = TEXT("libGL.so.1");
#endif		
	}

	if (!GLLoaded)
	{
		// Only call it once as succeeding calls will 'fail'.
		debugf(TEXT("binding %s"), *OpenGLLibName);
		if ( SDL_GL_LoadLibrary(appToAnsi(*OpenGLLibName)) == -1 )
		{
			const TCHAR* Error = appFromAnsi(SDL_GetError());
			// Higor: Raspbian systems may have SDL2 pre-emptively load the library, absorb error.
			if ( appStricmp(Error,TEXT("OpenGL library already loaded")) )
				appErrorf(Error);
		}
		
		// Load relevant functions
		GLLoaded = InitProcs();
		if ( GLLoaded )
		{
			SDL_GL_SetAttribute( SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
		}
	}

	return GLLoaded;
	
	unguard;
}

void* FOpenGLBase::CreateContext( void* Window)
{
	SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0);
	
	// Enable object sharing
	if ( Instances.Num() )
	{
		for ( INT i=0; i<Instances.Num(); i++)
			if ( Instances(i)->Context )
			{
				FOpenGLBase* GL = Instances(i);
				SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
				SDL_GL_MakeCurrent((SDL_Window*)GL->Window, (SDL_GLContext)GL->Context);
				break;
			}
	}
	
	void* Result = (void*)SDL_GL_CreateContext( (SDL_Window*)Window );
	if ( Result )
		InitProcs(true); // Ugly, reacquire procs
	return Result;
}

void FOpenGLBase::DeleteContext( void* Context)
{
	if ( Context )
#if SDL3BUILD
		SDL_GL_DestroyContext((SDL_GLContext)Context);
#else
		SDL_GL_DeleteContext(Context);
#endif
}

bool FOpenGLBase::MakeCurrent( void* OnWindow)
{
	guard(FOpenGLBase::MakeCurrent);

	// Unreal Editor may run different OpenGL render devices, get actual current context.
	if ( GIsEditor )
		CurrentContext = SDL_GL_GetCurrentContext();

	if ( !OnWindow )
	{
		if ( CurrentContext )
		{
			CurrentWindow = nullptr;
			CurrentContext = nullptr;
			SDL_GL_MakeCurrent(nullptr, nullptr);
		}
		SetCurrentInstance(nullptr);
	}
	else
	{
		if ( Context && (Context != CurrentContext || OnWindow != CurrentWindow) )
		{
			Window = OnWindow;
			INT Result = SDL_GL_MakeCurrent((SDL_Window*)Window, (SDL_GLContext)Context);
#if SDL3BUILD
			if ( Result == 0 )
#else
			if ( Result != 0 )
#endif
			{
				debugf(TEXT("SDL_GL_MakeCurrent error %i - %ls"), Result, appFromAnsi(SDL_GetError()));
				if ( CurrentContext || CurrentWindow )
				{
					CurrentWindow = nullptr;
					CurrentContext = nullptr;
					ActiveInstance = nullptr;
					SDL_GL_MakeCurrent(nullptr, nullptr);
				}
				SetCurrentInstance(nullptr);
				return false;
			}
			CurrentWindow = Window;
			CurrentContext = Context;
		}
		SetCurrentInstance(this);
	}
	return true;

	unguard;
}


void FOpenGLBase::SetContextAttribute( FGL::EContextAttribute Attribute, DWORD Value)
{
	guard(FOpenGLBase::SetContextAttribute);

	switch ( Attribute )
	{
	case FGL::EContextAttribute::MAJOR_VERSION:
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,Value);
		break;
	case FGL::EContextAttribute::MINOR_VERSION:
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,Value);
		break;
	case FGL::EContextAttribute::PROFILE:
		if ( Value == FGL::PROFILE_UNDEFINED )
			Value = 0;
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,Value);
		break;
	case FGL::EContextAttribute::FLAGS:
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,Value);
		break;
	default:
		break;
	}

	unguard;
}

#endif /* !_WIN32 */
