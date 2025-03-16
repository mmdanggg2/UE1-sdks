/*=============================================================================
	OpenGL_FrameBuffer.cpp: OpenGL framebuffer handling.

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

#include "OpenGLDrv.h"
#include "UTGLROpenGL.h"

#include "OpenGL_TextureFormat.h"

static UBOOL LogNextError = 0;

static bool CreateColorTexture
(
	INT Width,
	INT Height,
	const FTextureFormatInfo& Format,
	FOpenGLRenderTarget& RenderTarget,
	FOpenGLFBOAttachment_Texture& FBOTexture
)
{
	guard(CreateColorTexture);
	GL_DEV_CHECK(FBOTexture.RefCount == 0);
	GL_DEV_CHECK(FBOTexture.Texture.Texture == 0);

	FBOTexture.Texture = FOpenGLTexture();
	FBOTexture.Texture.Create(GL_TEXTURE_2D);

	FOpenGLBase::glBindTexture(GL_TEXTURE_2D, FBOTexture.Texture.Texture);
	FOpenGLBase::SetTextureStorage(FBOTexture.Texture, Format, Width, Height, 1, 1);
	FOpenGLBase::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	FOpenGLBase::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	FOpenGLBase::SetTextureFilters(FBOTexture.Texture, true);
	FOpenGLBase::glBindTexture(GL_TEXTURE_2D, 0); //!!

	// If attachment validates the framebuffer, consider it a success
	RenderTarget.SetAttachment(GL_COLOR_ATTACHMENT0_EXT, &FBOTexture);
	if ( FOpenGLBase::glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT) == GL_FRAMEBUFFER_COMPLETE_EXT )
		return true;

	// If attachment does not validate the framebuffer, remove it and fail
	RenderTarget.SetAttachment(GL_COLOR_ATTACHMENT0_EXT, nullptr);
	return false;
	unguard;
}

static bool CreateColorRenderbuffer
(
	INT Width,
	INT Height,
	INT Samples,
	GLenum InternalFormat,
	FOpenGLRenderTarget& RenderTarget,
	FOpenGLFBOAttachment_RenderBuffer& FBORenderbuffer
)
{
	guard(CreateColorRenderbuffer);
	GL_DEV_CHECK(FBORenderbuffer.RefCount == 0);
	GL_DEV_CHECK(FBORenderbuffer.Renderbuffer == 0);

	FOpenGLBase::glGenRenderbuffers( 1, &FBORenderbuffer.Renderbuffer);
	FOpenGLBase::glBindRenderbuffer( GL_RENDERBUFFER_EXT, FBORenderbuffer.Renderbuffer);
	if ( Samples > 1 )
		FOpenGLBase::glRenderbufferStorageMultisample( GL_RENDERBUFFER_EXT, Samples, InternalFormat, Width, Height);
	else
		FOpenGLBase::glRenderbufferStorage( GL_RENDERBUFFER_EXT, InternalFormat, Width, Height);
	FOpenGLBase::glBindRenderbuffer( GL_RENDERBUFFER_EXT, 0);

	// If attachment validates the framebuffer, consider it a success
	RenderTarget.SetAttachment(GL_COLOR_ATTACHMENT0_EXT, &FBORenderbuffer);
	if ( FOpenGLBase::glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT) == GL_FRAMEBUFFER_COMPLETE_EXT )
		return true;

	// If attachment does not validate the framebuffer, remove it and fail
	RenderTarget.SetAttachment(GL_COLOR_ATTACHMENT0_EXT, nullptr);
	return false;
	unguard;
}

static void CreateDepthRenderbuffer
(
	INT Width,
	INT Height,
	INT Samples,
	FOpenGLFBOAttachment_RenderBuffer& FBORenderbuffer
)
{
	guard(CreateDepthRenderbuffer);
	GL_DEV_CHECK(FBORenderbuffer.RefCount == 0);
	GL_DEV_CHECK(FBORenderbuffer.Renderbuffer == 0);

	GLenum Format = (UOpenGLRenderDevice::GetContextType() == CONTEXTTYPE_GLES) ? GL_DEPTH_COMPONENT32F : GL_DEPTH_COMPONENT;

	FOpenGLBase::glGenRenderbuffers(1, &FBORenderbuffer.Renderbuffer);
	FOpenGLBase::glBindRenderbuffer(GL_RENDERBUFFER_EXT, FBORenderbuffer.Renderbuffer);
	if ( Samples > 1 )
		FOpenGLBase::glRenderbufferStorageMultisample(GL_RENDERBUFFER_EXT, Samples, Format, Width, Height);
	else
		FOpenGLBase::glRenderbufferStorage(GL_RENDERBUFFER_EXT, Format, Width, Height);
	FOpenGLBase::glBindRenderbuffer(GL_RENDERBUFFER_EXT, 0);
	unguard;
}

static void CreateRenderTargets( FOpenGLRenderTarget* R, INT N)
{
	for ( INT i=0; i<N; i++)
		if ( !R[i].IsValid() )
			R[i].Create();
}

static void ClearRenderTargets( FOpenGLRenderTarget* R, INT N)
{
	for ( INT i=0; i<N; i++)
		if ( R[i].IsValid() )
			R[i].Destroy();
}

static void DestroyRenderTargets( FOpenGLRenderTarget* R, INT N)
{
	for ( INT i=0; i<N; i++)
		if ( R[i].IsValid() )
			R[i].Destroy();
}


bool UOpenGLRenderDevice::UpdateMainFramebuffer()
{
	guard(UOpenGLRenderDevice::UpdateMainFramebuffer);

	// Should be an assertion, but fail gracefully instead.
	if ( !FOpenGLBase::SupportsFramebuffer )
		return false;

	bool MSAA;               // MSAA requested
	INT  Samples;            // MSAA samples requested
	bool NeedSwapchain;      // Swapchain render target needed
	bool NeedOffscreen;      // Offscreen render target needed
	bool NeedOffscreenMSAA;  // Offscreen render target needed (MSAA)
	
	NeedSwapchain     = false; // TODO: D3D12/Vulkan
	MSAA              = FOpenGLBase::SupportsFramebufferMultisampling && UseAA && (NumAASamples > 1);
	Samples           = MSAA ? Min(NumAASamples, FOpenGLBase::MaxFramebufferSamples) : 1;
	NeedOffscreen     = ColorCorrectionMode == CC_UseFramebuffer;
	NeedOffscreenMSAA = MSAA && (NeedOffscreen || NeedSwapchain);

	ERenderTarget NewRenderTarget =
		NeedOffscreenMSAA ? RENDERTARGET_Offscreen_MSAA :
		NeedOffscreen     ? RENDERTARGET_Offscreen      :
		NeedSwapchain     ? RENDERTARGET_Swapchain      :
		                    RENDERTARGET_System         ;

	// Offscreen rendering will not be done at all
	if ( NewRenderTarget == RENDERTARGET_System || !MainFramebuffer_Required )
	{
		DestroyMainFramebuffer();
		return false;
	}

	// Destruction pass 1:
	// If the primary render target is being downgraded, destroy render targets
	// and attachments that are no longer relevant
	//
	// Note: this may destroy the depth attachments
	if ( NewRenderTarget < PrimaryRenderTarget )
	{
		if ( (NewRenderTarget < RENDERTARGET_Offscreen_MSAA) && Offscreen.IsMSAA() )
		{
			if ( GL_DEV )
				debugf(NAME_DevGraphics, TEXT("OpenGL: destroying multisampled render targets (1)"));
			DestroyRenderTargets(Offscreen.RenderTarget_MSAA, Offscreen.Buffers);
		}
		if ( (NewRenderTarget < RENDERTARGET_Offscreen) && Offscreen.RenderTarget[0].IsValid() )
		{
			if ( GL_DEV )
				debugf(NAME_DevGraphics, TEXT("OpenGL: destroying ofscreen render targets (1)"));
			DestroyRenderTargets(Offscreen.RenderTarget, Offscreen.Buffers);
		}
		if ( (NewRenderTarget < RENDERTARGET_Swapchain) && Offscreen.IsPresentingToSwapchain() )
		{
			if ( GL_DEV )
				debugf(NAME_DevGraphics, TEXT("OpenGL: destroying swapchain render targets (1)"));
			DestroyRenderTargets(Offscreen.RenderTarget_Swapchain, Offscreen.Buffers);
		}
	}

	INT   Width            = RequestedFramebufferWidth;
	INT   Height           = RequestedFramebufferHeight;
	DWORD BitsPerChannel   = Clamp<DWORD>(FramebufferBpc, FB_BPC_8bit, FB_BPC_16bit);
	bool  HasOffscreen     = Offscreen.RenderTarget[0].IsValid();
	bool  HasOffscreenMSAA = Offscreen.IsMSAA();
	bool  ResetAttachments          = (Offscreen.Width != Width) || (Offscreen.Height != Height) || (Offscreen.Bpc != FramebufferBpc);
	bool  ResetAttachmentsMSAA      = ResetAttachments || (Offscreen.Samples != Samples);
	bool  ResetAttachmentsSwapchain = ResetAttachments;
	bool  ResetDepthBuffer          = ResetAttachments || ResetAttachmentsMSAA || ResetAttachmentsSwapchain || (NewRenderTarget != PrimaryRenderTarget);

	// Nothing needs to be reset
	if ( !ResetAttachments && !ResetAttachmentsMSAA && !ResetAttachmentsSwapchain && !ResetDepthBuffer )
		return true;

	// Destruction pass 2:
	// If the render targets need to be reset due to changes in properties, 
	// then clear out attachments while keeping the FBO.
	//
	// Note: this may destroy the depth attachments
	if ( ResetAttachmentsMSAA && Offscreen.IsMSAA() )
	{
		if ( GL_DEV )
			debugf(NAME_DevGraphics, TEXT("OpenGL: clearing multisampled render targets (2)"));
		ClearRenderTargets(Offscreen.RenderTarget_MSAA, Offscreen.Buffers);
	}
	if ( ResetAttachments && Offscreen.RenderTarget[0].IsValid() )
	{
		if ( GL_DEV )
			debugf(NAME_DevGraphics, TEXT("OpenGL: clearing ofscreen render targets (2)"));
		ClearRenderTargets(Offscreen.RenderTarget, Offscreen.Buffers);
	}
	if ( ResetAttachmentsSwapchain && Offscreen.IsPresentingToSwapchain() )
	{
		if ( GL_DEV )
			debugf(NAME_DevGraphics, TEXT("OpenGL: clearing swapchain render targets (2)"));
		ClearRenderTargets(Offscreen.RenderTarget_Swapchain, Offscreen.Buffers);
	}
	if ( ResetDepthBuffer && Offscreen.DepthAttachment[0].IsValid() )
	{
		if ( GL_DEV )
			debugf(NAME_DevGraphics, TEXT("OpenGL: clearing ofscreen depth buffer (2)"));
		FOpenGLRenderTarget* RenderTargets = Offscreen.RenderTarget;
		for ( INT i=0; i<Offscreen.TotalBuffers; i++)
			if ( RenderTargets[i].IsValid() )
				RenderTargets[i].SetAttachment(GL_DEPTH_ATTACHMENT, nullptr);
		GL_DEV_CHECK(!Offscreen.DepthAttachment[0].IsValid());
	}

	// Dangerous on Swapchain mode
	// TODO: Immediate swapchain FBO creation
	if ( FBO_Wait > 0 )
	{
		FBO_Wait--;
		LogNextError = 1;
		return false;
	}

	UBOOL LogError = LogNextError;
	LogNextError = 0;

	// Setup offscreen render target
	// Render target may exist in empty state, so check attachment instead
	if ( NeedOffscreen && !Offscreen.ColorAttachment[0].IsValid() )
	{
		CreateRenderTargets(Offscreen.RenderTarget, Offscreen.Buffers);

		// The idea behind more bits is to prevent quality loss after brightness/gamma correction.
		static const BYTE F[3] = {TEXF_RGBA8_, TEXF_RGB10A2, TEXF_RGBA16};

		for ( INT i=0; i<Offscreen.Buffers; i++)
		{
			// TODO: NEED ABSTRACTION
			FOpenGLBase::glBindFramebuffer(GL_FRAMEBUFFER_EXT, Offscreen.RenderTarget[i].FBO);
		AGAIN:
			if ( !CreateColorTexture(Width, Height, FTextureFormatInfo::Get(F[BitsPerChannel]), Offscreen.RenderTarget[i], Offscreen.ColorAttachment[i]) )
			{
				if ( BitsPerChannel > 0 )
				{
					BitsPerChannel--;
					goto AGAIN;
				}
				if ( LogError )
					debugf(NAME_Init, TEXT("OpenGL: Failed to create Color Texture for offscreen framebuffer [mode=%i]"), this->PrimaryRenderTarget);
			}
			GL_ERROR_ASSERT
		}
	}

	// Setup offscreen MSAA render target
	// Render target may exist in empty state, so check attachment instead
	if ( NeedOffscreenMSAA && !Offscreen.ColorAttachment_MSAA[0].IsValid() )
	{
		CreateRenderTargets(Offscreen.RenderTarget_MSAA, Offscreen.Buffers);

		static const GLenum F[3] = {GL_RGBA8, GL_RGB10_A2, GL_RGBA16};

		for ( INT i=0; i<Offscreen.Buffers; i++)
		{
			FOpenGLBase::glBindFramebuffer(GL_FRAMEBUFFER_EXT, Offscreen.RenderTarget_MSAA[i].FBO);
		AGAIN_MSAA:
			if ( !CreateColorRenderbuffer(Width, Height, Samples, F[BitsPerChannel], Offscreen.RenderTarget_MSAA[i], Offscreen.ColorAttachment_MSAA[i]) )
			{
				if ( BitsPerChannel > 0 )
				{
					BitsPerChannel--;
					goto AGAIN_MSAA;
				}
				if ( LogError )
					debugf(NAME_Init, TEXT("OpenGL: Failed to create Color Renderbuffer for MSAA offscreen framebuffer [mode=%i]"), this->PrimaryRenderTarget);
			}
			GL_ERROR_ASSERT
		}
	}

	// Create depth buffers if missing
	// Note: Some drivers fail to create a new FBO after resolution change, try again on next frame.
	if ( !Offscreen.DepthAttachment[0].IsValid() )
	{
		FOpenGLRenderTarget* PrimaryRenderTarget =
			(NewRenderTarget == RENDERTARGET_Offscreen_MSAA) ? Offscreen.RenderTarget_MSAA :
			(NewRenderTarget == RENDERTARGET_Swapchain)      ? Offscreen.RenderTarget_Swapchain : 
			                                                   Offscreen.RenderTarget;

		if ( GL_DEV )
			debugf(NAME_DevGraphics, TEXT("OpenGL: creating offscreen depth buffer [mode=%i]"), NewRenderTarget);
		for ( INT i=0; i<Offscreen.Buffers; i++)
		{
			CreateDepthRenderbuffer(Width, Height, Samples, Offscreen.DepthAttachment[i]);

			FOpenGLBase::glBindFramebuffer(GL_FRAMEBUFFER_EXT, PrimaryRenderTarget[i].FBO);
			PrimaryRenderTarget[i].SetAttachment(GL_DEPTH_ATTACHMENT_EXT, &Offscreen.DepthAttachment[i]);
			if ( FOpenGLBase::glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT )
			{
				if ( LogError )
					debugf(NAME_Init, TEXT("OpenGL: Failed to create framebuffer, offscreen framebuffer incomplete [mode=%i]"), this->PrimaryRenderTarget);
				DestroyMainFramebuffer();
				return false;
			}
		}
	}
	GL_ERROR_ASSERT

	FOpenGLBase::glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
	Offscreen.Width   = Width;
	Offscreen.Height  = Height;
	Offscreen.Bpc     = BitsPerChannel;
	Offscreen.Samples = Samples;

	RequestedFramebufferWidth            = Width;
	RequestedFramebufferHeight           = Height;

	PrimaryRenderTarget = NewRenderTarget;

	// If a framebuffer format isn't supported, fix config
	FramebufferBpc = BitsPerChannel;
	INT NumBpc = (BitsPerChannel == FB_BPC_16bit) ? 16 :
		         (BitsPerChannel == FB_BPC_10bit) ? 10 :
	                                                 8;

	if ( !HasOffscreen || ResetAttachments )
		debugf( NAME_Init, TEXT("OpenGL: %s main Framebuffer: %ix%i  Bpc=%i  AA=%s"),
			HasOffscreen ? TEXT("Resized") : TEXT("Created"),
			Width, Height, NumBpc,
			(Samples > 1) ? *FString::Printf(TEXT("true (%i)"),Samples) : TEXT("false"));
	return true;

	unguard;
}

void UOpenGLRenderDevice::LockMainFramebuffer()
{
	guard(UOpenGLRenderDevice::LockMainFramebuffer);

	INT i = (Offscreen.CurrentBuffer + 1) % Offscreen.Buffers;
	Offscreen.CurrentBuffer = i;

	if ( Offscreen.RenderTarget_MSAA[i].IsValid() )
		FOpenGLBase::glBindFramebuffer(GL_FRAMEBUFFER_EXT, Offscreen.RenderTarget_MSAA[i].FBO);
	else if ( Offscreen.RenderTarget[i].IsValid() )
		FOpenGLBase::glBindFramebuffer(GL_FRAMEBUFFER_EXT, Offscreen.RenderTarget[i].FBO);
	else if ( Offscreen.RenderTarget_Swapchain[i].IsValid() )
		FOpenGLBase::glBindFramebuffer(GL_FRAMEBUFFER_EXT, Offscreen.RenderTarget_Swapchain[i].FBO);
	else
		return;

	MainFramebuffer_Locked = 1;

	unguard;
}

void UOpenGLRenderDevice::DestroyMainFramebuffer()
{
	guard(UOpenGLRenderDevice::DestroyMainFramebuffer);
	if ( GL_DEV )
		debugf( NAME_Init, TEXT("OpenGL: Destroying framebuffers [mode=%i]"), PrimaryRenderTarget);
	PrimaryRenderTarget = RENDERTARGET_System;
	MainFramebuffer_Locked = 0;
	DestroyRenderTargets(Offscreen.RenderTarget, Offscreen.TotalBuffers);
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

	INT i = Offscreen.CurrentBuffer;

	// Handled by render path
	if ( Offscreen.RenderTarget[i].IsValid() && m_pBlitFramebuffer )
	{
		(this->*m_pBlitFramebuffer)();
		return;
	}

	// Blit the multisampled buffer first.
	if ( Offscreen.RenderTarget_MSAA[i].IsValid() )
		CopyFramebuffer(Offscreen.RenderTarget_MSAA[i].FBO, Offscreen.RenderTarget[i].FBO, Offscreen.Width, Offscreen.Height, true);

	// Render the texture into main screen
	if ( Offscreen.RenderTarget[i].IsValid() )
	{
		// Invalidate contents of FBO depth before unbinding it (non-MSAA mode)
		if ( Offscreen.RenderTarget[i].HasAttachment(GL_DEPTH_ATTACHMENT_EXT) && FOpenGLBase::SupportsDataInvalidation )
		{
			GLenum Attachments[] = { GL_DEPTH_ATTACHMENT };
			FOpenGLBase::glInvalidateFramebuffer( GL_FRAMEBUFFER_EXT, ARRAY_COUNT(Attachments), Attachments);
		}
		FOpenGLBase::glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);

		// Process all pending draws
		FlushDrawBuffers();

		// Temporarily enable color correction if needed.
		INT OldShaderColorCorrection = m_ShaderColorCorrection;
		m_ShaderColorCorrection = (ColorCorrectionMode == CC_UseFramebuffer);
		if ( m_pFillScreen )
			(this->*m_pFillScreen)(&Offscreen.ColorAttachment[i].Texture, nullptr, PF_NoSmooth);
		FOpenGLBase::glFlush();
		m_ShaderColorCorrection = OldShaderColorCorrection;

		// Invalidate contents of FBO texture attachment.
		if ( FOpenGLBase::SupportsDataInvalidation )
			FOpenGLBase::glInvalidateTexImage(Offscreen.ColorAttachment[i].Texture.Texture, 0);
	}

	MainFramebuffer_Locked = 0;
	unguard;
}


/*-----------------------------------------------------------------------------
	FOpenGLFBOAttachment_RenderBuffer.
-----------------------------------------------------------------------------*/

void FOpenGLFBOAttachment_RenderBuffer::Attach( GLenum Slot)
{
	GL_DEV_CHECK(Renderbuffer != 0);
	FOpenGLBase::glFramebufferRenderbuffer( GL_FRAMEBUFFER_EXT, Slot, GL_RENDERBUFFER_EXT, Renderbuffer);
}

void FOpenGLFBOAttachment_RenderBuffer::Detach( GLenum Slot)
{
	GL_DEV_CHECK(Renderbuffer != 0);
	FOpenGLBase::glFramebufferRenderbuffer( GL_FRAMEBUFFER_EXT, Slot, GL_RENDERBUFFER_EXT, 0);
}

bool FOpenGLFBOAttachment_RenderBuffer::IsValid()
{
	return Renderbuffer != 0;
}

void FOpenGLFBOAttachment_RenderBuffer::Destroy()
{
	if ( Renderbuffer != 0 )
	{
		FOpenGLBase::glDeleteRenderbuffers( 1, &Renderbuffer);
		Renderbuffer = 0;
	}
}


/*-----------------------------------------------------------------------------
	FOpenGLFBOAttachment_Texture.
-----------------------------------------------------------------------------*/

void FOpenGLFBOAttachment_Texture::Attach( GLenum Slot)
{
	GL_DEV_CHECK(Texture.Texture != 0);

	// Only supports TEXTURE_2D
	switch (Texture.Target)
	{
	case GL_TEXTURE_2D:
		FOpenGLBase::glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, Slot, Texture.Target, Texture.Texture, 0);
		break;
	default:
		break;
	};
}

void FOpenGLFBOAttachment_Texture::Detach( GLenum Slot)
{
	GL_DEV_CHECK(Texture.Texture != 0);

	// Only supports TEXTURE_2D
	switch (Texture.Target)
	{
	case GL_TEXTURE_2D:
		FOpenGLBase::glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, Slot, Texture.Target, 0, 0);
		break;
	default:
		break;
	};
}

bool FOpenGLFBOAttachment_Texture::IsValid()
{
	return Texture.Texture != 0;
}

void FOpenGLFBOAttachment_Texture::Destroy()
{
	if ( Texture.Texture != 0 )
	{
		FOpenGLBase::glDeleteTextures( 1, &Texture.Texture);
		Texture.Texture = 0;
	}
}


/*-----------------------------------------------------------------------------
	FOpenGLRenderTarget.
-----------------------------------------------------------------------------*/

void FOpenGLRenderTarget::SetAttachment( GLenum Slot, FOpenGLFBOAttachment* Attachment)
{
	guard(FOpenGLRenderTarget::SetAttachment);
	GL_DEV_CHECK(FBO != 0);

	FOpenGLFBOAttachment** ExistingAttachment;
	FOpenGLFBOAttachment*  Attach = nullptr;
	FOpenGLFBOAttachment*  Detach = nullptr;

	// Map manipulation
	if ( !Attachment )
	{
		ExistingAttachment = Attachments.Find(Slot);
		if ( ExistingAttachment )
		{
			GL_DEV_CHECK(ExistingAttachment[0]);
			Detach = ExistingAttachment[0];
			Attachments.Remove(Slot);
		}
	}
	else
	{
		ExistingAttachment = &Attachments.FindOrSet(Slot, nullptr);
		if ( ExistingAttachment[0] != Attachment )
		{
			Attach = Attachment;
			Detach = ExistingAttachment[0];
			ExistingAttachment[0] = Attachment;
		}
	}

	// FBO manipulation
	if ( Detach )
	{
		Detach->Detach(Slot);
		Detach->Unreference();
	}
	if ( Attach )
	{
		Attach->Attach(Slot);
		Attach->Reference();
	}
	unguard;
}

void FOpenGLRenderTarget::Clear()
{
	GL_DEV_CHECK(FBO != 0);
	FOpenGLBase::glBindFramebuffer(GL_FRAMEBUFFER_EXT, FBO);

	for ( TMapExt<DWORD,FOpenGLFBOAttachment*>::TIterator It(Attachments); It; ++It)
	{
		GL_DEV_CHECK(It.Value() != nullptr);
		It.Value()->Detach(It.Key());
		It.Value()->Unreference();
	}
	Attachments.Empty();

	FOpenGLBase::glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
	FOpenGLBase::glDeleteFramebuffers(1, &FBO);
	FBO = 0;
}

inline void FOpenGLRenderTarget::Destroy()
{
	if ( !FBO )
		return;

	FOpenGLBase::glBindFramebuffer(GL_FRAMEBUFFER_EXT, FBO);

	for ( TMapExt<DWORD,FOpenGLFBOAttachment*>::TIterator It(Attachments); It; ++It)
	{
		GL_DEV_CHECK(It.Value() != nullptr);
		It.Value()->Unreference();
	}
	Attachments.Empty();

	FOpenGLBase::glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
	FOpenGLBase::glDeleteFramebuffers(1, &FBO);
	FBO = 0;
}

