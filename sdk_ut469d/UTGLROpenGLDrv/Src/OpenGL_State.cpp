/*=============================================================================
	OpenGL_State.cpp: OpenGL Render Device State Control.

	There are many types of resource destruction:
	- Device local    (triggered by config changes and renderer destruction)
	- Engine          (triggered by 'FLUSH')
	- Global          (triggered when all renderers are destroyed)

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

#include "OpenGLDrv.h"
#include "UTGLROpenGL.h"

UOpenGLRenderDevice::FDrawBuffer UOpenGLRenderDevice::DrawBuffer{};

void UOpenGLRenderDevice::UpdateStateLocks()
{
	guard(UOpenGLRenderDevice::UpdateStateLocks);

	/* Required by Shader Compiler */
	PL_DetailMax               = DetailMax;
	PL_ShaderBrightness        = FOpenGLBase::GetShaderBrightness();

	/* Required by Texture system (mostly to trigger texture flush) */
	PL_DetailTextures          = DetailTextures;
	PL_ForceNoSmooth           = ForceNoSmooth;
	PL_Use16BitTextures        = Use16BitTextures;
	PL_AlwaysMipmap            = AlwaysMipmap;
	SupportsTC                 = UseHDTextures;

	/* Required for updating global Sampler objects */
	PL_MaxAnisotropy           = MaxAnisotropy;
	PL_LODBias                 = LODBias;
	PL_UseTrilinear            = UseTrilinear;

	/* Control masked transparency in fragment shader */
	m_SmoothMasking            = SmoothMasking && m_usingAA;

	unguard;
}


void UOpenGLRenderDevice::Exit()
{
	guard(UOpenGLRenderDevice::Exit);
	check(NumDevices > 0);

	debugf( NAME_Exit, TEXT("Exit %s"), GetName() );

#if __WIN32__
	//Reset gamma ramp
	if ( NumDevices == 1 ) //Higor: prevent Unreal Editor from resetting gamma after closing texture browser/properties
		ResetGamma();
#endif

	// Destroy non-shared GL objects
	if ( GL && GL->MakeCurrent(GL->Window) )
	{
		debugf( NAME_Exit, TEXT("Destroying locals..."));
		DestroyMainFramebuffer();
		DestroySamplers();
		GL->Reset();
	}

	if ( GL->Context )
		UnsetRes();

	// Shut down global GL.
	if ( --NumDevices == 0 || GIsRequestingExit || GIsCriticalError )
	{
		Flush(0);
		DrawBuffer.Destroy();
		DestroyBufferObjects();

#if UTGLR_SUPPORT_DXGI_INTEROP
		if (UseLowLatencySwapchain)
			GL->DestroyDXGISwapchain();
#endif
	}

	// Destroy Context
	if ( GL )
	{
		debugf( NAME_Exit, TEXT("Delete GL (%i)"), NumDevices);
		delete GL;
		GL = nullptr;
	}

	unguard;
}


/*-----------------------------------------------------------------------------
	Other deinitializators.
-----------------------------------------------------------------------------*/

void UOpenGLRenderDevice::Flush(UBOOL AllowPrecache)
{
	guard(UOpenGLRenderDevice::Flush);

	// Reset all OpenGL devices prior to Texture pool Flush
	for ( INT i=0; i<FOpenGLBase::Instances.Num(); i++)
	{
		FOpenGLBase* GL = FOpenGLBase::Instances(i);
		if ( GL && GL->MakeCurrent(GL->Window) )
			GL->Reset();
	}
	DestroyMainFramebuffer();
	FlushStaticGeometry();
	TexturePool.Flush();

	if ( AllowPrecache && UsePrecache && !GIsEditor )
		PrecacheOnFlip = 1;

	if ( !GIsEditor )
		SetGamma(Viewport->GetOuterUClient()->Brightness);

	// Force transformation matrices to be updated.
	SceneParams.Mode = INDEX_NONE;

	unguard;
}

void UOpenGLRenderDevice::FDrawBuffer::Destroy()
{
	if ( StreamBufferInit )
	{
		if ( Complex )       delete Complex;
		if ( ComplexStatic ) delete ComplexStatic;
		if ( Gouraud )       delete Gouraud;
		if ( Quad )          delete Quad;
		if ( Line )          delete Line;
		if ( Fill )          delete Fill;
		if ( Decal )         delete Decal;
		if ( GeneralBuffer[0] ) delete GeneralBuffer[0];
		if ( GeneralBuffer[1] ) delete GeneralBuffer[1];
		if ( GeneralBuffer[2] ) delete GeneralBuffer[2];
		appMemzero(this, sizeof(*this));
	}
}
