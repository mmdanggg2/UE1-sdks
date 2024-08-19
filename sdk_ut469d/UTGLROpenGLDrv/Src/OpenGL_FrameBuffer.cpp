/*=============================================================================
	OpenGL_FrameBuffer.cpp: OpenGL framebuffer handling.

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

#include "OpenGLDrv.h"
#include "UTGLROpenGL.h"

bool UOpenGLRenderDevice::UpdateMainFramebuffer()
{
	guard(UOpenGLRenderDevice::UpdateMainFramebuffer);

	// Should be an assertion, but fail gracefully instead.
	if ( !FOpenGLBase::SupportsFramebuffer )
		return false;

	if ( ColorCorrectionMode != CC_UseFramebuffer )
	{
		if ( MainFramebuffer_FBO )
			DestroyMainFramebuffer();
		return false;
	}

	INT Width = RequestedFramebufferWidth;
	INT Height = RequestedFramebufferHeight;
	UBOOL HighQuality = RequestedFramebufferHighQuality;
	bool HasFramebuffer     = MainFramebuffer_FBO      != 0;
	bool HasFramebufferMSAA = MainFramebuffer_FBO_MSAA != 0;
	bool MultiSample        = FOpenGLBase::SupportsFramebufferMultisampling && UseAA && (NumAASamples > 1);
	INT  Samples            = MultiSample ? Min(NumAASamples, FOpenGLBase::MaxFramebufferSamples) : 1;
	bool ResetAttachments = 
		(MainFramebuffer_Width != Width) || 
		(MainFramebuffer_Height != Height) ||
		(MainFramebuffer_HighQuality != HighQuality) ||
		(MultiSample != HasFramebufferMSAA) ||
		(MainFramebuffer_Samples_MSAA != Samples);

	if ( ResetAttachments )
		DestroyMainFramebuffer();
		
	if ( FBO_Wait > 0 )
	{
		FBO_Wait--;
		return false;
	}

	if ( !MainFramebuffer_Required )
		return false;

#if UTGLR_SUPPORT_DXGI_INTEROP
	if (UseLowLatencySwapchain)
	{
		// Generate names for the FBO, FBO surf, and render buffers
		if (!MainFramebuffer_FBO)
			FOpenGLBase::glGenFramebuffers(1, &MainFramebuffer_FBO);

		if (!MainFramebuffer_Texture.Texture)
			FOpenGLBase::glGenTextures(1, &MainFramebuffer_Texture.Texture);

		if (!MultiSample)
		{
			if (!MainFramebuffer_Depth)
				FOpenGLBase::glGenRenderbuffers(1, &MainFramebuffer_Depth);
		}
		else
		{
			if (!MainFramebuffer_FBO_MSAA)
				FOpenGLBase::glGenFramebuffers(1, &MainFramebuffer_FBO_MSAA);

			if (!MainFramebuffer_Color_MSAA && MultiSample)
				FOpenGLBase::glGenRenderbuffers(1, &MainFramebuffer_Color_MSAA);

			if (!MainFramebuffer_Depth_MSAA && MultiSample)
				FOpenGLBase::glGenRenderbuffers(1, &MainFramebuffer_Depth_MSAA);
		}

		if (GL->UpdateDXGISwapchain())
		{
			// All good. Our GL names now map to DirectX objects. We just need to attach the render buffers
			FOpenGLBase::glBindFramebuffer(GL_FRAMEBUFFER_EXT, MainFramebuffer_FBO);
			if (MainFramebuffer_Texture.Target == GL_TEXTURE_2D_ARRAY)
				FOpenGLBase::glFramebufferTextureLayer(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, MainFramebuffer_Texture.Texture, 0, 0);
			else
				FOpenGLBase::glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, MainFramebuffer_Texture.Texture, 0);

			if (!MultiSample)
			{
				FOpenGLBase::glFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, MainFramebuffer_Depth);
			}
			else
			{
				FOpenGLBase::glBindFramebuffer(GL_FRAMEBUFFER_EXT, MainFramebuffer_FBO_MSAA);
				FOpenGLBase::glFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, MainFramebuffer_Color_MSAA);
				FOpenGLBase::glFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, MainFramebuffer_Depth_MSAA);
			}

			goto CheckFramebuffer;
		}
	}
#endif

	if (!MainFramebuffer_FBO)
		FOpenGLBase::glGenFramebuffers(1, &MainFramebuffer_FBO);
	FOpenGLBase::glBindFramebuffer( GL_FRAMEBUFFER_EXT, MainFramebuffer_FBO);
	GL_ERROR_ASSERT

	// Setup main framebuffer's texture
	if ( !MainFramebuffer_Texture.Texture || ResetAttachments )
	{
		Texture_Again:
		if ( MainFramebuffer_Texture.Texture )
		{
			FOpenGLBase::glDeleteTextures(1, &MainFramebuffer_Texture.Texture);
			MainFramebuffer_Texture.Texture = 0;
		}
		if ( !MainFramebuffer_Texture.Texture )
		{
			MainFramebuffer_Texture = FOpenGLTexture();
			MainFramebuffer_Texture.Create(DebugUseGL3Resolve ? GL_TEXTURE_2D : GL_TEXTURE_2D_UTEXTURE);
		}

		FOpenGLBase::glBindTexture(MainFramebuffer_Texture.Target, MainFramebuffer_Texture.Texture);
		// The idea behind 16 bits is to prevent quality loss after brightness/gamma correction.
		FTextureFormatInfo& Format = TextureFormatInfo[HighQuality ? TEXF_RGBA16 : TEXF_RGBA8_];
		FOpenGLBase::SetTextureStorage(MainFramebuffer_Texture, Format, Width, Height, 1, 1);
		FOpenGLBase::glTexParameteri(MainFramebuffer_Texture.Target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		FOpenGLBase::glTexParameteri(MainFramebuffer_Texture.Target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		FOpenGLBase::SetTextureFilters(MainFramebuffer_Texture, true);
		FOpenGLBase::glBindTexture(MainFramebuffer_Texture.Target, 0);

		if ( MainFramebuffer_Texture.Target == GL_TEXTURE_2D_ARRAY )
			FOpenGLBase::glFramebufferTextureLayer(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, MainFramebuffer_Texture.Texture, 0, 0);
		else
			FOpenGLBase::glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, MainFramebuffer_Texture.Target, MainFramebuffer_Texture.Texture, 0);

		if ( FOpenGLBase::glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT )
		{
			if ( HighQuality )
			{
				HighQuality = 0;
				goto Texture_Again;
			}
		}
	}

	// Setup main framebuffer's depth (if needed)
	if ( (!MainFramebuffer_Depth || ResetAttachments) && !MultiSample )
	{
		if ( !MainFramebuffer_Depth )
			FOpenGLBase::glGenRenderbuffers( 1, &MainFramebuffer_Depth);
		FOpenGLBase::glBindRenderbuffer( GL_RENDERBUFFER_EXT, MainFramebuffer_Depth);
		FOpenGLBase::glRenderbufferStorage( GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, Width, Height);
		FOpenGLBase::glBindRenderbuffer( GL_RENDERBUFFER_EXT, 0);

		FOpenGLBase::glFramebufferRenderbuffer( GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, MainFramebuffer_Depth);
	}

	if ( MultiSample )
	{
		if ( !MainFramebuffer_FBO_MSAA )
			FOpenGLBase::glGenFramebuffers( 1, &MainFramebuffer_FBO_MSAA);
		FOpenGLBase::glBindFramebuffer( GL_FRAMEBUFFER_EXT, MainFramebuffer_FBO_MSAA);

		if ( !MainFramebuffer_Color_MSAA || ResetAttachments )
		{
			if ( !MainFramebuffer_Color_MSAA )
				FOpenGLBase::glGenRenderbuffers( 1, &MainFramebuffer_Color_MSAA);
			FOpenGLBase::glBindRenderbuffer( GL_RENDERBUFFER_EXT, MainFramebuffer_Color_MSAA);
			FOpenGLBase::glRenderbufferStorageMultisample( GL_RENDERBUFFER_EXT, Samples, HighQuality ? GL_RGBA16 : GL_RGBA8, Width, Height);
			FOpenGLBase::glBindRenderbuffer( GL_RENDERBUFFER_EXT, 0);

			FOpenGLBase::glFramebufferRenderbuffer( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, MainFramebuffer_Color_MSAA);
		}

		if ( !MainFramebuffer_Depth_MSAA || ResetAttachments )
		{
			if ( !MainFramebuffer_Depth_MSAA )
				FOpenGLBase::glGenRenderbuffers( 1, &MainFramebuffer_Depth_MSAA);
			FOpenGLBase::glBindRenderbuffer(GL_RENDERBUFFER_EXT, MainFramebuffer_Depth_MSAA);
			FOpenGLBase::glRenderbufferStorageMultisample( GL_RENDERBUFFER_EXT, Samples, GL_DEPTH_COMPONENT, Width, Height);
			FOpenGLBase::glBindRenderbuffer(GL_RENDERBUFFER_EXT, 0);

			FOpenGLBase::glFramebufferRenderbuffer( GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, MainFramebuffer_Depth_MSAA);
		}
	}

CheckFramebuffer:
	// Some drivers fail to create a new FBO after resolution change, try again on next frame.
	GLenum Status = FOpenGLBase::glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);
	if ( Status != GL_FRAMEBUFFER_COMPLETE_EXT )
	{
		DestroyMainFramebuffer();
		return false;
	}

	FOpenGLBase::glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
	RequestedFramebufferWidth       = MainFramebuffer_Width       = Width;
	RequestedFramebufferHeight      = MainFramebuffer_Height      = Height;
	RequestedFramebufferHighQuality = MainFramebuffer_HighQuality = HighQuality;
	MainFramebuffer_Samples_MSAA    = Samples;
	if ( !HasFramebuffer || ResetAttachments )
		debugf( NAME_Init, TEXT("OpenGL: %s main Framebuffer: %ix%i  HighQuality=%s  AA=%s"),
			HasFramebuffer ? TEXT("Resized") : TEXT("Created"),
			Width, Height,
			HighQuality ? TEXT("true") : TEXT("false"),
			(Samples > 1) ? *FString::Printf(TEXT("true (%i)"),Samples) : TEXT("false"));
	return true;

	unguard;
}

void UOpenGLRenderDevice::LockMainFramebuffer()
{
	guard(UOpenGLRenderDevice::LockMainFramebuffer);

	if ( MainFramebuffer_FBO_MSAA )
		FOpenGLBase::glBindFramebuffer(GL_FRAMEBUFFER_EXT, MainFramebuffer_FBO_MSAA);
	else if ( MainFramebuffer_FBO )
		FOpenGLBase::glBindFramebuffer(GL_FRAMEBUFFER_EXT, MainFramebuffer_FBO);
	else
		return;

	MainFramebuffer_Locked = 1;

	unguard;
}

void UOpenGLRenderDevice::DestroyMainFramebuffer()
{
	guard(UOpenGLRenderDevice::DestroyMainFramebuffer);
	if ( MainFramebuffer_FBO )
	{
		FOpenGLBase::glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
		FOpenGLBase::glDeleteFramebuffers(1, &MainFramebuffer_FBO);
		MainFramebuffer_FBO = 0;
	}
	if ( MainFramebuffer_Texture.Texture )
	{
		FOpenGLBase::glDeleteTextures( 1, &MainFramebuffer_Texture.Texture);
		MainFramebuffer_Texture.Texture = 0;
	}
	if ( MainFramebuffer_Depth )
	{
		FOpenGLBase::glDeleteRenderbuffers( 1, &MainFramebuffer_Depth);
		MainFramebuffer_Depth = 0;
	}
	if ( MainFramebuffer_FBO_MSAA )
	{
		FOpenGLBase::glDeleteFramebuffers( 1, &MainFramebuffer_FBO_MSAA);
		MainFramebuffer_FBO_MSAA = 0;
	}
	if ( MainFramebuffer_Color_MSAA )
	{
		FOpenGLBase::glDeleteRenderbuffers( 1, &MainFramebuffer_Color_MSAA);
		MainFramebuffer_Color_MSAA = 0;
	}
	if ( MainFramebuffer_Depth_MSAA )
	{
		FOpenGLBase::glDeleteRenderbuffers( 1, &MainFramebuffer_Depth_MSAA);
		MainFramebuffer_Depth_MSAA = 0;
	}
	MainFramebuffer_Locked = 0;
	unguard;
}

//
// Simple Framebuffer resolve operation
//
void UOpenGLRenderDevice::CopyFramebuffer( GLuint Source, GLuint Target, GLint Width, GLint Height, bool InvalidateSrc)
{
	FOpenGLBase::glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, Source);
	FOpenGLBase::glBindFramebuffer(GL_DRAW_FRAMEBUFFER_EXT, Target);
	FOpenGLBase::glBlitFramebuffer(0, 0, Width, Height, 0, 0, Width, Height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	// Invalidate contents if possible
	if ( InvalidateSrc && FOpenGLBase::glInvalidateFramebuffer )
	{
		GLenum Attachments[] = { GL_DEPTH_ATTACHMENT, GL_COLOR_ATTACHMENT0};
		FOpenGLBase::glInvalidateFramebuffer(GL_READ_FRAMEBUFFER_EXT, ARRAY_COUNT(Attachments), Attachments);
	}
}

void UOpenGLRenderDevice::BlitMainFramebuffer()
{
	guard(UOpenGLRenderDevice::BlitMainFramebuffer);

	// Handled by render path
	if ( MainFramebuffer_FBO && m_pBlitFramebuffer )
	{
		(this->*m_pBlitFramebuffer)();
		return;
	}

	// Blit the multisampled buffer first.
	if ( MainFramebuffer_FBO_MSAA )
		CopyFramebuffer(MainFramebuffer_FBO_MSAA, MainFramebuffer_FBO, MainFramebuffer_Width, MainFramebuffer_Height, true);

	// Render the texture into main screen
	if ( MainFramebuffer_FBO )
	{
		// Invalidate contents of FBO depth before unbinding it (non-MSAA mode)
		if ( MainFramebuffer_Locked && MainFramebuffer_Depth && FOpenGLBase::SupportsDataInvalidation )
		{
			GLenum Attachments[] = { GL_DEPTH_ATTACHMENT };
			FOpenGLBase::glInvalidateFramebuffer( GL_FRAMEBUFFER_EXT, ARRAY_COUNT(Attachments), Attachments);
		}
		FOpenGLBase::glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);

		// Process all pending draws
		FlushDrawBuffers();
		SetDisabledAAState();

		// Temporarily enable color correction if needed.
		INT OldShaderColorCorrection = m_ShaderColorCorrection;
		m_ShaderColorCorrection = (ColorCorrectionMode == CC_UseFramebuffer);
		if ( m_pFillScreen )
			(this->*m_pFillScreen)(&MainFramebuffer_Texture, nullptr, PF_NoSmooth);
		FOpenGLBase::glFlush();
		m_ShaderColorCorrection = OldShaderColorCorrection;

		// Invalidate contents of FBO texture attachment.
		if ( FOpenGLBase::SupportsDataInvalidation )
			FOpenGLBase::glInvalidateTexImage(MainFramebuffer_Texture.Texture, 0);
	}

	MainFramebuffer_Locked = 0;
	unguard;
}
