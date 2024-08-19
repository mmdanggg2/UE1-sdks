/*=============================================================================
	FOpenGLTexture.h: High level OpenGL texture object information.

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

#ifndef _INC_GL_TEXTURE
#define _INC_GL_TEXTURE

#include "FOpenGLSwizzle.h"

//
// High level Texture object
//
class FOpenGLTexture
{
public:
	friend class FOpenGLBase;
	friend class FOpenGL12;

	GLuint    Texture;
	GLenum    Target;
	GLenum    Format;
	GLushort  BaseLevel;
	GLushort  MaxLevel;
	GLushort  USize;
	GLushort  VSize;
	GLushort  WSize; // Or array layers
	GLubyte   Allocated:1;
	GLubyte   InmutableFormat:1;
	GLubyte   MagNearest:1; // Magnification filter set to nearest
	GLubyte   Compressed:1;
	GLubyte   TextureView:1;

	FOpenGLSwizzle Swizzle;

	FOpenGLTexture();

	void Create( GLenum InTarget);

	bool HasMipMaps() const;
	DWORD EncodeKey() const;
};

/*-----------------------------------------------------------------------------
	FOpenGLTexture.
-----------------------------------------------------------------------------*/

inline FOpenGLTexture::FOpenGLTexture()
	: Texture(0)
	, Target(0)
	, Format(0)
	, BaseLevel(0)
	, MaxLevel(0) //API sets it to 1000?
	, USize(0)
	, VSize(0)
	, WSize(0)
	, Allocated(0)
	, InmutableFormat(0)
	, MagNearest(0)
	, Compressed(0)
	, TextureView(0)
	, Swizzle(0)
{
}

inline void FOpenGLTexture::Create( GLenum InTarget)
{
	GL_DEV_CHECK(Texture == 0);

	FOpenGLBase::glGenTextures(1, &Texture);
	Target = InTarget;
}

inline bool FOpenGLTexture::HasMipMaps() const
{
	return BaseLevel != MaxLevel;
}

inline DWORD FOpenGLTexture::EncodeKey() const
{
	// Key is 24 bits long
	DWORD Key = 0;

	// 5 bits: UBits
	DWORD UTest = (DWORD)(USize - 1);
	for ( SBYTE b=15; b>=0; b--)
		if ( UTest & (1 << b) )
		{
			Key |= b+1;
			break;
		}

	// 5 bits: VBits
	DWORD VTest = (DWORD)(VSize - 1);
	for ( SBYTE b=15; b>=0; b--)
		if ( VTest & (1 << b) )
		{
			Key |= (b+1) << 5;
			break;
		}

	// 1 bit: MipMap
	if ( HasMipMaps() )
		Key |= 1 << 10;

	// 1 bit: Swizzle
	if ( Swizzle )
		Key |= 1 << 11;

	// 12 bits: Format (13-16 always has 0x8XXX)
	Key |= (Format & 0x0FFF) << 12;

	return Key;
}

#endif
