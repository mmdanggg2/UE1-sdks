/*=============================================================================
	FOpenGLSwizzle.h

	Dependents:
	- FOpenGLTexture

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

#ifndef _INC_GL_SWIZZLE
#define _INC_GL_SWIZZLE

enum ESwizzle : GLubyte
{
	SWZ_UNDEFINED = 0,
	SWZ_RED,
	SWZ_GREEN,
	SWZ_BLUE,
	SWZ_ALPHA,
	SWZ_ZERO,
	SWZ_ONE
};

class FOpenGLSwizzle
{
public:
	union
	{
		GLuint Raw;
		GLubyte Channel[4];
		struct{	GLubyte Red, Green, Blue, Alpha; };
	};

	constexpr FOpenGLSwizzle();
	constexpr FOpenGLSwizzle( GLuint InRaw);
	constexpr FOpenGLSwizzle( ESwizzle InRed, ESwizzle InGreen, ESwizzle InBlue, ESwizzle InAlpha);

	operator bool() const;

	static GLenum ToGL( ESwizzle S);
};
static_assert(sizeof(FOpenGLSwizzle) == sizeof(GLuint), "Bad FOpenGLSwizzle size");

// Note: GL_TEXTURE_SWIZZLE_RGBA is not supported in OpenGL ES.
static constexpr GLenum SwizzleParameters[] {GL_TEXTURE_SWIZZLE_R, GL_TEXTURE_SWIZZLE_G, GL_TEXTURE_SWIZZLE_B, GL_TEXTURE_SWIZZLE_A };


/*-----------------------------------------------------------------------------
	FOpenGLSwizzle.
-----------------------------------------------------------------------------*/

inline constexpr FOpenGLSwizzle::FOpenGLSwizzle()
	: Red(SWZ_RED)
	, Green(SWZ_GREEN)
	, Blue(SWZ_BLUE)
	, Alpha(SWZ_ALPHA)
{
}

inline constexpr FOpenGLSwizzle::FOpenGLSwizzle( GLuint InRaw)
	: Raw(InRaw)
{
}

inline constexpr FOpenGLSwizzle::FOpenGLSwizzle( ESwizzle InRed, ESwizzle InGreen, ESwizzle InBlue, ESwizzle InAlpha)
	: Red(InRed)
	, Green(InGreen)
	, Blue(InBlue)
	, Alpha(InAlpha)
{
}

inline FOpenGLSwizzle::operator bool() const
{
	return Raw != 0;
}

inline GLenum FOpenGLSwizzle::ToGL( ESwizzle S)
{
	switch(S)
	{
	case(SWZ_RED):
		return GL_RED;
	case(SWZ_GREEN):
		return GL_GREEN;
	case(SWZ_BLUE):
		return GL_BLUE;
	case(SWZ_ALPHA):
		return GL_ALPHA;
	case(SWZ_ZERO):
		return GL_ZERO;
	case(SWZ_ONE):
		return GL_ONE;
	default:
		return 0;
	}
}

#endif
