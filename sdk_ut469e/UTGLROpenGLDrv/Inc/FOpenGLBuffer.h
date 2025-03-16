/*=============================================================================
	FOpenGLBuffer.h: High level OpenGL buffe object information.

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

#ifndef _INC_GL_BUFFER
#define _INC_GL_BUFFER

//
// High level Buffer object
//
class FOpenGLBuffer
{
public:
	GLuint     Buffer;
	GLenum     Target; // Buffers can be bound to any target, this indicates intended use target
	GLuint     Index; // Index of the binding point
	GLsizeiptr Size;

	GLenum     Usage; // If mutable, expected usage during allocation
	GLbitfield Flags; // If inmutable, flags used to allocate storage

	void*      Mapped;

	operator bool();

	void CreateMutable( GLenum InTarget, GLsizeiptr InSize, const void* InData, GLenum InUsage);
	void CreateInmutable( GLenum InTarget, GLsizeiptr InSize, const void* InData, GLbitfield InFlags);

	bool Inmutable() const;
};

/*-----------------------------------------------------------------------------
	FOpenGLBuffer.
-----------------------------------------------------------------------------*/

inline FOpenGLBuffer::operator bool()
{
	return Buffer != 0;
}

inline void FOpenGLBuffer::CreateMutable( GLenum InTarget, GLsizeiptr InSize, const void* InData, GLenum InUsage)
{
	FOpenGLBase::CreateBuffer(Buffer, InTarget, InSize, InData, InUsage);
	Target = InTarget;
	Size = InSize;
	Usage = InUsage;
}

inline void FOpenGLBuffer::CreateInmutable( GLenum InTarget, GLsizeiptr InSize, const void* InData, GLbitfield InFlags)
{
	FOpenGLBase::CreateInmutableBuffer(Buffer, InTarget, InSize, InData, InFlags);
	Target = InTarget;
	Size = InSize;
	Flags = InFlags;
}

inline bool FOpenGLBuffer::Inmutable() const
{
	return Flags != 0;
}

#endif
