/*=============================================================================
	FOpenGLRenderTarget.h

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

#ifndef _INC_GL_RENDERTARGET
#define _INC_GL_RENDERTARGET

#include "FOpenGLTexture.h"

class FOpenGLFBOAttachment
{
public:
	GLuint RefCount{0};

	FOpenGLFBOAttachment()
		: RefCount(0)
	{}

	void Reference();
	void Unreference();

	virtual void Attach(GLenum Slot) = 0;
	virtual void Detach(GLenum Slot) = 0;
	virtual bool IsValid() = 0;
	virtual void Destroy() = 0;
};

class FOpenGLFBOAttachment_RenderBuffer : public FOpenGLFBOAttachment
{
public:
	GLuint    Renderbuffer{0};
	GLboolean Multisampled{0};

	void Attach(GLenum Slot);
	void Detach(GLenum Slot);
	bool IsValid();
	void Destroy();
};

class FOpenGLFBOAttachment_Texture : public FOpenGLFBOAttachment
{
public:
	FOpenGLTexture Texture;

	void Attach(GLenum Slot);
	void Detach(GLenum Slot);
	bool IsValid();
	void Destroy();
};



class FOpenGLRenderTarget
{
public:
	GLuint FBO;
	TMapExt<DWORD,FOpenGLFBOAttachment*> Attachments;

	FOpenGLRenderTarget();
	FOpenGLRenderTarget(ENoInit);

	void Create();
	bool IsValid();
	bool HasAttachment( GLenum Slot);

	//
	// BELOW STATE HANDLING NEEDS TO BE REFACTORED INTO FOpenGLBase
	//

	// These operations do not manipulate the GL_FRAMEBUFFER_EXT
	void SetAttachment( GLenum Slot, FOpenGLFBOAttachment* Attachment);

	// These operations clear the GL_FRAMEBUFFER_EXT binding
	void Clear();
	void Destroy();
};

/*-----------------------------------------------------------------------------
	FOpenGLFBOAttachment.
-----------------------------------------------------------------------------*/

inline void FOpenGLFBOAttachment::Reference()
{
	++RefCount;
}

inline void FOpenGLFBOAttachment::Unreference()
{
	GL_DEV_CHECK(RefCount > 0);
	if ( --RefCount == 0 )
		Destroy();
}


/*-----------------------------------------------------------------------------
	FOpenGLRenderTarget.
-----------------------------------------------------------------------------*/

inline FOpenGLRenderTarget::FOpenGLRenderTarget()
	: FBO{}
{}

inline FOpenGLRenderTarget::FOpenGLRenderTarget(ENoInit)
{}

inline void FOpenGLRenderTarget::Create()
{
	GL_DEV_CHECK(FBO == 0);
	FOpenGLBase::glGenFramebuffers(1, &FBO);
}

inline bool FOpenGLRenderTarget::IsValid()
{
	return FBO != 0;
}

inline bool FOpenGLRenderTarget::HasAttachment( GLenum Slot)
{
	return Attachments.Find(Slot) != nullptr;
}

#endif
