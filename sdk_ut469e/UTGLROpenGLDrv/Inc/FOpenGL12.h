/*=============================================================================
	FOpenGL12.h: OpenGL 1/2 context abstraction.

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

/*#include "glcorearb.h"
#include "glext.h"*/

// TODO:
// #include "GL/glext.h"

#define GL_SMOOTH 0x1D01
#define GL_MODELVIEW 0x1700
#define GL_PROJECTION 0x1701
#define GL_TEXTURE_COORD_ARRAY 0x8078

typedef void (APIENTRYP PFNGLBEGINPROC)(GLenum mode);
typedef void (APIENTRYP PFNGLENDPROC)(void);
typedef void (APIENTRYP PFNGLVERTEX3F)(GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRYP PFNGLTEXCOORD2F)(GLfloat s, GLfloat t);
typedef void (APIENTRYP PFNGLCOLOR4FPROC)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
typedef void (APIENTRYP PFNGLSHADEMODEL)(GLenum mode);
typedef void (APIENTRYP PFNGLCLIPPLANEPROC)(GLenum plane, const GLdouble *equation);

typedef void (APIENTRYP PFNGLFRUSTUMPROC)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
typedef void (APIENTRYP PFNGLLOADIDENTITYPROC)(void);
typedef void (APIENTRYP PFNGLMATRIXMODEPROC)(GLenum mode);
typedef void (APIENTRYP PFNGLORTHOPROC)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
typedef void (APIENTRYP PFNGLSCALEFPROC)(GLfloat x, GLfloat y, GLfloat z);

typedef void (APIENTRYP PFNGLCOLORPOINTERPROC)(GLint size, GLenum type, GLsizei stride, const void *pointer);
typedef void (APIENTRYP PFNGLNORMALPOINTERPROC)(GLenum type, GLsizei stride, const void *pointer);
typedef void (APIENTRYP PFNGLTEXCOORDPOINTERPROC)(GLint size, GLenum type, GLsizei stride, const void *pointer);
typedef void (APIENTRYP PFNGLVERTEXPOINTERPROC)(GLint size, GLenum type, GLsizei stride, const void *pointer);

typedef void (APIENTRYP PFNGLTEXENVFPROC)(GLenum target, GLenum pname, GLfloat param);

typedef void (APIENTRYP PFNGLENABLECLIENTSTATEPROC)(GLenum array);
typedef void (APIENTRYP PFNGLDISABLECLIENTSTATEPROC)(GLenum array);

// Required procs
#define GL_ENTRYPOINTS_12(GLEntry) \
/* OpenGL 1 */ \
/*	GLEntry(PFNGLBEGINPROC,glBegin)*/ \
/*	GLEntry(PFNGLENDPROC,glEnd)*/ \
	GLEntry(PFNGLCOLOR4FPROC,glColor4f) \
/*	GLEntry(PFNGLTEXCOORD2F,glTexCoord2f)*/ \
/*	GLEntry(PFNGLVERTEX3F,glVertex3f)*/ \
	GLEntry(PFNGLSHADEMODEL,glShadeModel) \
	GLEntry(PFNGLCLIPPLANEPROC,glClipPlane) \
	GLEntry(PFNGLFRUSTUMPROC,glFrustum) \
	GLEntry(PFNGLLOADIDENTITYPROC,glLoadIdentity) \
	GLEntry(PFNGLMATRIXMODEPROC,glMatrixMode) \
	GLEntry(PFNGLORTHOPROC,glOrtho) \
	GLEntry(PFNGLSCALEFPROC,glScalef) \
	GLEntry(PFNGLCOLORPOINTERPROC,glColorPointer) \
	GLEntry(PFNGLNORMALPOINTERPROC,glNormalPointer) \
	GLEntry(PFNGLTEXCOORDPOINTERPROC,glTexCoordPointer) \
	GLEntry(PFNGLVERTEXPOINTERPROC,glVertexPointer) \
	GLEntry(PFNGLENABLECLIENTSTATEPROC,glEnableClientState) \
	GLEntry(PFNGLDISABLECLIENTSTATEPROC,glDisableClientState) \
	GLEntry(PFNGLTEXENVFPROC,glTexEnvf) \
/* MultiTexture */ \
	GLEntry(PFNGLCLIENTACTIVETEXTUREARBPROC,glClientActiveTextureARB) \
/* Secondary Color */ \
	GLEntry(PFNGLSECONDARYCOLORPOINTEREXTPROC,glSecondaryColorPointerEXT) \
/* ARB Program */ \
	GLEntry(PFNGLBINDPROGRAMARBPROC,glBindProgramARB) \
	GLEntry(PFNGLGENPROGRAMSARBPROC,glGenProgramsARB) \
	GLEntry(PFNGLDELETEPROGRAMSARBPROC,glDeleteProgramsARB) \
	GLEntry(PFNGLPROGRAMSTRINGARBPROC,glProgramStringARB) \
	GLEntry(PFNGLPROGRAMENVPARAMETER4FARBPROC,glProgramEnvParameter4fARB) \
	GLEntry(PFNGLPROGRAMENVPARAMETER4FVARBPROC,glProgramEnvParameter4fvARB) \
	GLEntry(PFNGLPROGRAMLOCALPARAMETER4FARBPROC,glProgramLocalParameter4fARB) \
	GLEntry(PFNGLVERTEXATTRIB4FARBPROC,glVertexAttrib4fARB) \
	GLEntry(PFNGLGETPROGRAMIVARBPROC,glGetProgramivARB)

// Required extensions
#define GL_EXTENSIONS_12(GLExt) \
	GLExt(SecondaryColor,TEXT("GL_EXT_secondary_color")) \
	GLExt(MultiTexture,TEXT("GL_ARB_multitexture")) \
	GLExt(VertexProgram,TEXT("GL_ARB_vertex_program")) \
	GLExt(FragmentProgram,TEXT("GL_ARB_fragment_program"))




class FOpenGL12 : public FOpenGLBase
{
public:
	FOpenGL12( void* InWindow);
	~FOpenGL12();

	static bool Init();

	// Declare GL12 procs and extensions
	#define GLEntry(type, name) static type name;
	#define GLExt(name,ext) static UBOOL Supports##name;
	GL_ENTRYPOINTS_12(GLEntry)
	GL_EXTENSIONS_12(GLExt)
	#undef GLExt
	#undef GLEntry

protected:
	// State control for first 8 texture mapping units
	BYTE TextureEnableBits;
	BYTE ClientTextureEnableBits;
	BYTE ClientStateBits;

	void* ActiveDrawBuffer;
	DWORD ActiveDrawBufferAttribs;
//	UBOOL ForceSetVertexBuffer; // TODO: Every time we bind a new ARRAY_BUFFER, we need to link it to a vertex buffer

	// State control for ARB programs
	FProgramID ProgramID;
	GLuint ActiveVertexProgram;
	GLuint ActiveFragmentProgram;
	TMapExt<FProgramID,GLuint> VertexPrograms;
	TMapExt<FProgramID,GLuint> FragmentPrograms;

	// State control for ARB program envs
	FPlane VertexEnvs[8];
	FPlane FragmentEnvs[8];

public:
	enum EClientStateBits
	{
		CS_VERTEX_ARRAY          = 1 << 0,
		CS_NORMAL_ARRAY          = 1 << 1,
		CS_COLOR_ARRAY           = 1 << 2,
		CS_SECONDARY_COLOR_ARRAY = 1 << 3,
	};

	// FOpenGLBase interface
	void Reset();
	void Lock();
	void Unlock();
	void SetProgram( FProgramID NewProgramID, void** ProgramData=nullptr);
	void FlushPrograms();

	// FOpenGL12
	void SetEnabledClientTextures( BYTE NewEnableBits);
	void SetEnabledClientStates( BYTE NewEnableBits);
	template <typename T> void SetVertexArray( T* DrawBuffer, GLuint StaticVBO=0);
	void SetTextures( FPendingTexture* Textures, BYTE NewTextureBits);

	// Set program Envs, if NoTrack=True then do no tracking or checking at all (set is forced)
	template <bool NoTrack=false> void SetVertexShaderEnv( GLuint EnvIndex, GLfloat x, GLfloat y=0, GLfloat z=0, GLfloat w=0);
	template <bool NoTrack=false> void SetFragmentShaderEnv( GLuint EnvIndex, GLfloat x, GLfloat y=0, GLfloat z=0, GLfloat w=0);
	template <bool NoTrack=false> void SetVertexShaderEnv( GLuint EnvIndex, const FVector& vec3);
	template <bool NoTrack=false> void SetFragmentShaderEnv( GLuint EnvIndex, const FVector& vec3);
	template <bool NoTrack=false> void SetVertexShaderEnv( GLuint EnvIndex, const FPlane& vec4);
	template <bool NoTrack=false> void SetFragmentShaderEnv( GLuint EnvIndex, const FPlane& vec4);

	static GLuint CompileProgram( GLenum Target, const ANSICHAR* String, const GLuint Len);
};


template<typename T>
inline void FOpenGL12::SetVertexArray( T* DrawBuffer, GLuint StaticVBO)
{
	using namespace FGL::DrawBuffer;

	if ( (ActiveDrawBuffer != (void*)DrawBuffer) || ActiveDrawBufferAttribs != DrawBuffer->ActiveAttribs )
	{
		ActiveDrawBuffer = (void*)DrawBuffer;
		ActiveDrawBufferAttribs = DrawBuffer->ActiveAttribs;

		if ( (T::BufferTypes & BT_StaticBuffer) && StaticVBO )
		{
			constexpr GLsizei Stride = sizeof(typename T::VBOType);

			glBindBuffer(GL_ARRAY_BUFFER, StaticVBO);
			if ( T::VBOType::HasStaticVertexData() )
				glVertexPointer( T::VBOType::VertexArraySize(), T::VBOType::VertexArrayType(), Stride, BUFFER_OFFSET(T::VBOType::VertexOffset()) );
			if ( T::VBOType::HasStaticNormalData() )
				glNormalPointer( T::VBOType::NormalArrayType(), Stride, BUFFER_OFFSET(T::VBOType::NormalOffset()) );
			if ( T::VBOType::HasStaticColor0Data() )
				glColorPointer( T::VBOType::Color0ArraySize(), T::VBOType::Color0ArrayType(), Stride, BUFFER_OFFSET(T::VBOType::Color0Offset()) );
			if ( T::VBOType::HasStaticColor1Data() )/*SECONDARY COLOR DOES NOT SUPPORT ALPHA*/
				glSecondaryColorPointerEXT( 3, T::VBOType::Color1ArrayType(), Stride, BUFFER_OFFSET(T::VBOType::Color1Offset()) );
			if ( T::VBOType::HasStaticTexture0Data() )
				glTexCoordPointer( T::VBOType::Texture0ArraySize(), T::VBOType::Texture0ArrayType(), Stride, BUFFER_OFFSET(T::VBOType::Texture0Offset()) );
			if ( T::VBOType::HasStaticTexture1Data() )
			{
				glClientActiveTextureARB(GL_TEXTURE1_ARB);
				glTexCoordPointer( T::VBOType::Texture1ArraySize(), T::VBOType::Texture1ArrayType(), Stride, BUFFER_OFFSET(T::VBOType::Texture1Offset()) );
			}
			if ( T::VBOType::HasStaticTexture2Data() )
			{
				glClientActiveTextureARB(GL_TEXTURE2_ARB);
				glTexCoordPointer( T::VBOType::Texture2ArraySize(), T::VBOType::Texture2ArrayType(), Stride, BUFFER_OFFSET(T::VBOType::Texture2Offset()) );
			}
			if ( T::VBOType::HasStaticTexture1Data() || T::VBOType::HasStaticTexture2Data() )
				glClientActiveTextureARB(GL_TEXTURE0_ARB);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		DWORD Offset = 0;
		BYTE* Buffer = DrawBuffer->GetStreamBuffer()->Data;
		const GLsizei Stride = DrawBuffer->GetStride();
		INT TMU = 0;

		#define HAS_ATTRIB(bt) ((T::ForceAttribs & bt) || ((T::BufferTypes & bt) & ActiveDrawBufferAttribs) )
		if ( HAS_ATTRIB(BT_Vertex) )
		{
			glVertexPointer( T::VertexType::ArraySize, T::VertexType::ArrayType, Stride, Buffer + Offset);
			Offset += sizeof(typename T::VertexType);
		}
		if ( HAS_ATTRIB(BT_Normal) )
		{
			glNormalPointer( T::NormalType::ArrayType, Stride, Buffer + Offset);
			Offset += sizeof(typename T::NormalType);
		}
		if ( HAS_ATTRIB(BT_Color0) )
		{
			glColorPointer( T::Color0Type::ArraySize, T::Color0Type::ArrayType, Stride, Buffer + Offset);
			Offset += sizeof(typename T::Color0Type);
		}
		if ( HAS_ATTRIB(BT_Color1) )
		{
			glSecondaryColorPointerEXT( 3, T::Color1Type::ArrayType, Stride, Buffer + Offset);
			Offset += sizeof(typename T::Color1Type);/*SECONDARY COLOR DOES NOT SUPPORT ALPHA*/
		}
		if ( HAS_ATTRIB(BT_Texture0) )
		{
			glTexCoordPointer( T::Texture0Type::ArraySize, T::Texture0Type::ArrayType, Stride, Buffer + Offset);
			Offset += sizeof(typename T::Texture0Type);
		}
		if ( HAS_ATTRIB(BT_Texture1) )
		{
			TMU = 1;
			glClientActiveTextureARB(GL_TEXTURE1_ARB);
			glTexCoordPointer( T::Texture1Type::ArraySize, T::Texture1Type::ArrayType, Stride, Buffer + Offset);
			Offset += sizeof(typename T::Texture1Type);
		}
		if ( HAS_ATTRIB(BT_Texture2) )
		{
			TMU = 2;
			glClientActiveTextureARB(GL_TEXTURE2_ARB);
			glTexCoordPointer( T::Texture2Type::ArraySize, T::Texture2Type::ArrayType, Stride, Buffer + Offset);
			Offset += sizeof(typename T::Texture2Type);
		}
		//check(Offset == Stride);
		#undef HAS_ATTRIB
		if ( (T::BufferTypes & BT_Texture1|BT_Texture2) && TMU )
			glClientActiveTextureARB(GL_TEXTURE0_ARB);
	}
}

template <bool NoTrack>
inline bool ModifyEnv( FPlane& Env, const FPlane& NewEnv)
{
	if ( NoTrack )
		return true;
	if ( Env == NewEnv )
		return false;
	Env = NewEnv;
	return true;
}

template <bool NoTrack>
inline void FOpenGL12::SetVertexShaderEnv( GLuint EnvIndex, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	if ( ModifyEnv<NoTrack>(VertexEnvs[EnvIndex], FPlane(x,y,z,w)) )
		glProgramEnvParameter4fARB( GL_VERTEX_PROGRAM_ARB, EnvIndex, x, y, z, w);
}

template <bool NoTrack>
inline void FOpenGL12::SetFragmentShaderEnv( GLuint EnvIndex, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	if ( ModifyEnv<NoTrack>(FragmentEnvs[EnvIndex], FPlane(x,y,z,w)) )
		glProgramEnvParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, EnvIndex, x, y, z, w);
}

template <bool NoTrack>
inline void FOpenGL12::SetVertexShaderEnv( GLuint EnvIndex, const FVector& vec3)
{
	SetVertexShaderEnv<NoTrack>( EnvIndex, vec3.X, vec3.Y, vec3.Z, 0);
}

template <bool NoTrack>
inline void FOpenGL12::SetFragmentShaderEnv( GLuint EnvIndex, const FVector& vec3)
{
	SetFragmentShaderEnv<NoTrack>( EnvIndex, vec3.X, vec3.Y, vec3.Z, 0);
}

template <bool NoTrack>
inline void FOpenGL12::SetVertexShaderEnv( GLuint EnvIndex, const FPlane& vec4)
{
	if ( ModifyEnv<NoTrack>(VertexEnvs[EnvIndex], vec4) )
		glProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, EnvIndex, &vec4.X);
}

template <bool NoTrack>
inline void FOpenGL12::SetFragmentShaderEnv( GLuint EnvIndex, const FPlane& vec4)
{
	if ( ModifyEnv<NoTrack>(FragmentEnvs[EnvIndex], vec4) )
		glProgramEnvParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, EnvIndex, &vec4.X);
}


