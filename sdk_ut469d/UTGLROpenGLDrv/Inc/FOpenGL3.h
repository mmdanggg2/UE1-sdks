/*=============================================================================
	FOpenGL3.h: OpenGL 3.3 context abstraction.

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

/*#include "glcorearb.h"
#include "glext.h"*/

// TODO:
// #include "GL/glext.h"

// Required procs
#define GL_ENTRYPOINTS_3(GLEntry) \
/* GLSL */ \
	GLEntry(PFNGLCOMPILESHADERPROC,glCompileShader) \
	GLEntry(PFNGLCREATESHADERPROC,glCreateShader) \
	GLEntry(PFNGLDELETESHADERPROC,glDeleteShader) \
	GLEntry(PFNGLISSHADERPROC,glIsShader) \
	GLEntry(PFNGLGETSHADERINFOLOGPROC,glGetShaderInfoLog) \
	GLEntry(PFNGLGETSHADERIVPROC,glGetShaderiv) \
	GLEntry(PFNGLSHADERSOURCEPROC,glShaderSource) \
	GLEntry(PFNGLCREATEPROGRAMPROC,glCreateProgram) \
	GLEntry(PFNGLDELETEPROGRAMPROC,glDeleteProgram) \
	GLEntry(PFNGLISPROGRAMPROC,glIsProgram) \
	GLEntry(PFNGLGETPROGRAMINFOLOGPROC,glGetProgramInfoLog) \
	GLEntry(PFNGLGETPROGRAMIVPROC,glGetProgramiv) \
	GLEntry(PFNGLLINKPROGRAMPROC,glLinkProgram) \
	GLEntry(PFNGLATTACHSHADERPROC,glAttachShader) \
	GLEntry(PFNGLDETACHSHADERPROC,glDetachShader) \
\
	GLEntry(PFNGLUSEPROGRAMPROC,glUseProgram) \
	GLEntry(PFNGLBINDATTRIBLOCATIONPROC,glBindAttribLocation) \
	GLEntry(PFNGLGETUNIFORMLOCATIONPROC,glGetUniformLocation) \
	GLEntry(PFNGLUNIFORM1IPROC,glUniform1i) \
	GLEntry(PFNGLUNIFORM1FPROC,glUniform1f) \
	GLEntry(PFNGLUNIFORM4FPROC,glUniform4f) \
	GLEntry(PFNGLUNIFORM4FVPROC,glUniform4fv) \
	GLEntry(PFNGLBINDBUFFERBASEPROC,glBindBufferBase) \
	GLEntry(PFNGLBINDBUFFERRANGEPROC,glBindBufferRange) \
	GLEntry(PFNGLGETUNIFORMBLOCKINDEXPROC,glGetUniformBlockIndex) \
	GLEntry(PFNGLUNIFORMBLOCKBINDINGPROC,glUniformBlockBinding) \
	GLEntry(PFNGLENABLEVERTEXATTRIBARRAYPROC,glEnableVertexAttribArray) \
	GLEntry(PFNGLDISABLEVERTEXATTRIBARRAYPROC,glDisableVertexAttribArray) \
	GLEntry(PFNGLVERTEXATTRIBPOINTERPROC,glVertexAttribPointer) \
	GLEntry(PFNGLVERTEXATTRIBIPOINTERPROC,glVertexAttribIPointer)

// Required extensions
#define GL_EXTENSIONS_3(GLExt)


class FOpenGL3 : public FOpenGLBase
{
public:
	struct FProgramData
	{
		GLuint                 Program;
		FOpenGLUniform<FLOAT>  AlphaTest;
		FOpenGLUniform<FPlane> ColorGlobal;
		FOpenGLUniform<FPlane> ClipPlane;
		FOpenGLUniform<INT>    SampleCount;
		FOpenGLUniform<FLOAT>  LodBias;
	};

	FOpenGL3( void* InWindow);
	~FOpenGL3();

	static bool Init();
	
	// Declare GL3 procs and extensions
	#define GLEntry(type, name) static type name;
	#define GLExt(name,ext) static UBOOL Supports##name;
	GL_ENTRYPOINTS_3(GLEntry)
	GL_EXTENSIONS_3(GLExt)
	#undef GLExt
	#undef GLEntry

public:
	// FOpenGLUniform management (program must be in use)
	template <typename T> static void SetUniform( FOpenGLUniform<T>& Uniform, const T& NewValue);

protected:

	GLuint ActiveVAO;
	TMapExt<DWORD,GLuint> VAOs;

	// State control for GLSL shaders
	FProgramID ProgramID;
	static TMapExt<FProgramID,FProgramData> Programs;
	static TMapExt<FProgramID,GLuint> VertexShaders;
	static TMapExt<FProgramID,GLuint> GeometryShaders;
	static TMapExt<FProgramID,GLuint> FragmentShaders;

public:
	// FOpenGLBase interface
	void Reset();
	void Lock();
	void Unlock();
	void SetProgram( FProgramID NewProgramID, void** ProgramData=nullptr);
	void FlushPrograms();

	// FOpenGL3
	bool SetVertexArrayBufferless();
	template <typename T> bool SetVertexArray( T* DrawBuffer, GLuint StaticVBO=0, bool ForceUpdate=false);
	void SetTextures( FPendingTexture* Textures, BYTE NewTextureBits);

	static GLuint CompileShader( GLenum Target, const ANSICHAR* Source);
};


/*-----------------------------------------------------------------------------
	FOpenGLUniform management (program must be in use).
-----------------------------------------------------------------------------*/

template<typename T>
inline void FOpenGL3::SetUniform( FOpenGLUniform<T>& Uniform, const T& NewValue)
{
	if ( (Uniform.Location != GL_INVALID_INDEX) && (Uniform.Value != NewValue) )
	{
		Uniform.Value = NewValue;
		glUniform1i(Uniform.Location, (GLint)NewValue);
	}
}

template<>
inline void FOpenGL3::SetUniform<GLfloat>( FOpenGLUniform<GLfloat>& Uniform, const GLfloat& NewValue)
{
	if ( (Uniform.Location != GL_INVALID_INDEX) /*&& (Uniform.Value != NewValue)*/ )
	{
		Uniform.Value = NewValue;
		glUniform1f(Uniform.Location, NewValue);
	}
}

template<>
inline void FOpenGL3::SetUniform<FPlane>( FOpenGLUniform<FPlane>& Uniform, const FPlane& NewValue)
{
	if ( (Uniform.Location != GL_INVALID_INDEX) && (Uniform.Value != NewValue) )
	{
		Uniform.Value = NewValue;
		glUniform4fv(Uniform.Location, 1, &NewValue.X);
	}
}

inline bool FOpenGL3::SetVertexArrayBufferless()
{
	// Local index uses 12 bits, so use one of the reserved 4 for this special VAO
	const DWORD Key = 0x1000;

	GLuint* pVAO = VAOs.Find(Key);
	if ( pVAO )
	{
		if ( ActiveVAO != *pVAO )
			glBindVertexArray(ActiveVAO = *pVAO);
		return false;
	}

	pVAO = &VAOs.SetNoFind(Key, 0);
	glGenVertexArrays(1, pVAO);
	glBindVertexArray(ActiveVAO = *pVAO);
	return true;
}

template<typename T>
inline bool FOpenGL3::SetVertexArray( T* DrawBuffer, GLuint StaticVBO, bool ForceUpdate)
{
	using namespace FGL::DrawBuffer;

	// Encode unique VAO key
	DWORD Attribs = DrawBuffer->ActiveAttribs;
	DWORD Key = (Attribs << 16) ^ DrawBuffer->LocalIndex.GetValue();

	GLuint* pVAO = VAOs.Find(Key);
	if ( pVAO )
	{
		if ( ActiveVAO != *pVAO )
			glBindVertexArray(ActiveVAO = *pVAO);
		if ( !ForceUpdate )
			return false;
	}

	// Dynamic VBO not available
	if ( T::BufferTypes & (~FGL::DrawBuffer::BT_StaticBuffer) )
		check(DrawBuffer->GetBuffer()->VBO); // checkSlow

	if ( !pVAO )
	{
		pVAO = &VAOs.SetNoFind(Key, 0);
		glGenVertexArrays(1, pVAO);
		glBindVertexArray(ActiveVAO = *pVAO);
	}

	if ( (T::BufferTypes & FGL::DrawBuffer::BT_StaticBuffer) && StaticVBO )
	{
		glBindBuffer(GL_ARRAY_BUFFER, StaticVBO);
		if ( T::VBOType::HasStaticVertexData() )
			glVertexAttribPointer(0, T::VBOType::VertexArraySize(), T::VBOType::VertexArrayType(), GL_FALSE, sizeof(typename T::VBOType), BUFFER_OFFSET(T::VBOType::VertexOffset()) );
		if ( T::VBOType::HasStaticNormalData() )
			glVertexAttribPointer(6, T::VBOType::NormalArraySize(), T::VBOType::NormalArrayType(), GL_TRUE, sizeof(typename T::VBOType), BUFFER_OFFSET(T::VBOType::NormalOffset()) );
		if ( T::VBOType::HasStaticColor0Data() )
			glVertexAttribPointer(1, T::VBOType::Color0ArraySize(), T::VBOType::Color0ArrayType(), GL_TRUE, sizeof(typename T::VBOType), BUFFER_OFFSET(T::VBOType::Color0Offset()) );
		if ( T::VBOType::HasStaticColor1Data() )
			glVertexAttribPointer(2, T::VBOType::Color1ArraySize(), T::VBOType::Color1ArrayType(), GL_TRUE, sizeof(typename T::VBOType), BUFFER_OFFSET(T::VBOType::Color1Offset()) );
		if ( T::VBOType::HasStaticTexture0Data() )
			glVertexAttribPointer(3, T::VBOType::Texture0ArraySize(), T::VBOType::Texture0ArrayType(), GL_FALSE, sizeof(typename T::VBOType), BUFFER_OFFSET(T::VBOType::Texture0Offset()) );
		if ( T::VBOType::HasStaticTexture1Data() )
			glVertexAttribPointer(4, T::VBOType::Texture1ArraySize(), T::VBOType::Texture1ArrayType(), GL_FALSE, sizeof(typename T::VBOType), BUFFER_OFFSET(T::VBOType::Texture1Offset()) );
		if ( T::VBOType::HasStaticTexture2Data() )
			glVertexAttribPointer(5, T::VBOType::Texture2ArraySize(), T::VBOType::Texture2ArrayType(), GL_FALSE, sizeof(typename T::VBOType), BUFFER_OFFSET(T::VBOType::Texture2Offset()) );
		if ( T::VBOType::HasStaticVertexParam0Data() )
			glVertexAttribPointer(9, T::VBOType::VertexParam0ArraySize(), T::VBOType::VertexParam0ArrayType(), GL_FALSE, sizeof(typename T::VBOType), BUFFER_OFFSET(T::VBOType::VertexParam0Offset()) );
		if ( T::VBOType::HasStaticVertexParam1Data() )
			glVertexAttribPointer(10, T::VBOType::VertexParam1ArraySize(), T::VBOType::VertexParam1ArrayType(), GL_FALSE, sizeof(typename T::VBOType), BUFFER_OFFSET(T::VBOType::VertexParam1Offset()) );

		if ( T::VBOType::HasStaticVertexData() && (Attribs & (1<<0)) )
			glEnableVertexAttribArray(0);
		if ( T::VBOType::HasStaticNormalData() && (Attribs & (1<<6)) )
			glEnableVertexAttribArray(6);
		if ( T::VBOType::HasStaticColor0Data() && (Attribs & (1<<1)) )
			glEnableVertexAttribArray(1);
		if ( T::VBOType::HasStaticColor1Data() && (Attribs & (1<<2)) )
			glEnableVertexAttribArray(2);
		if ( T::VBOType::HasStaticTexture0Data() && (Attribs & (1<<3)) )
			glEnableVertexAttribArray(3);
		if ( T::VBOType::HasStaticTexture1Data() && (Attribs & (1<<4)) )
			glEnableVertexAttribArray(4);
		if ( T::VBOType::HasStaticTexture2Data() && (Attribs & (1<<5)) )
			glEnableVertexAttribArray(5);
		if ( T::VBOType::HasStaticVertexParam0Data() && (Attribs & (1<<9)) )
			glEnableVertexAttribArray(9);
		if ( T::VBOType::HasStaticVertexParam1Data() && (Attribs & (1<<10)) )
			glEnableVertexAttribArray(10);
	}
	// Higor: may cause redundant VBO binding on Setup stage but
	// puts less load on rendering stage as no tracking is done.
	glBindBuffer(GL_ARRAY_BUFFER, DrawBuffer->GetBuffer()->VBO);

	intptr_t Offset = 0;
	#define HAS_ATTRIB(bt) ((T::ForceAttribs & bt) || ((T::BufferTypes & bt) & Attribs) )
	if ( HAS_ATTRIB(BT_Vertex) ) // 0
	{
		glVertexAttribPointer(0, T::VertexType::ArraySize, T::VertexType::ArrayType, GL_FALSE, DrawBuffer->BufferStride, BUFFER_OFFSET(Offset));
		glEnableVertexAttribArray(0);
		Offset += sizeof(typename T::VertexType);
	}
	if ( HAS_ATTRIB(BT_Color0) ) // 1
	{
		glVertexAttribPointer(1, T::Color0Type::ArraySize, T::Color0Type::ArrayType, GL_TRUE, DrawBuffer->BufferStride, BUFFER_OFFSET(Offset));
		glEnableVertexAttribArray(1);
		Offset += sizeof(typename T::Color0Type);
	}
	if ( HAS_ATTRIB(BT_Color1) ) // 2
	{
		glVertexAttribPointer(2, T::Color1Type::ArraySize, T::Color1Type::ArrayType, GL_TRUE, DrawBuffer->BufferStride, BUFFER_OFFSET(Offset));
		glEnableVertexAttribArray(2);
		Offset += sizeof(typename T::Color1Type);
	}
	if ( HAS_ATTRIB(BT_Texture0) ) // 3
	{
		glVertexAttribPointer(3, T::Texture0Type::ArraySize, T::Texture0Type::ArrayType, GL_FALSE, DrawBuffer->BufferStride, BUFFER_OFFSET(Offset));
		glEnableVertexAttribArray(3);
		Offset += sizeof(typename T::Texture0Type);
	}
	if ( HAS_ATTRIB(BT_Texture1) ) // 4
	{
		glVertexAttribPointer(4, T::Texture1Type::ArraySize, T::Texture1Type::ArrayType, GL_FALSE, DrawBuffer->BufferStride, BUFFER_OFFSET(Offset));
		glEnableVertexAttribArray(4);
		Offset += sizeof(typename T::Texture1Type);
	}
	if ( HAS_ATTRIB(BT_Texture2) ) // 5
	{
		glVertexAttribPointer(5, T::Texture2Type::ArraySize, T::Texture2Type::ArrayType, GL_FALSE, DrawBuffer->BufferStride, BUFFER_OFFSET(Offset));
		glEnableVertexAttribArray(5);
		Offset += sizeof(typename T::Texture2Type);
	}
	if ( HAS_ATTRIB(BT_Normal) ) // 6
	{
		glVertexAttribPointer(6, T::NormalType::ArraySize, T::NormalType::ArrayType, GL_TRUE, DrawBuffer->BufferStride, BUFFER_OFFSET(Offset));
		glEnableVertexAttribArray(6); //GL_FALSE for normalization if we're doing it in CPU?
		Offset += sizeof(typename T::NormalType);
	}
	if ( HAS_ATTRIB(BT_TextureLayers) ) // 7
	{
		if ( IsIntAttrib(T::TextureLayersType::ArrayType) )
			glVertexAttribIPointer(7, T::TextureLayersType::ArraySize, T::TextureLayersType::ArrayType, DrawBuffer->BufferStride, BUFFER_OFFSET(Offset));
		else
			glVertexAttribPointer(7, T::TextureLayersType::ArraySize, T::TextureLayersType::ArrayType, GL_FALSE, DrawBuffer->BufferStride, BUFFER_OFFSET(Offset));
		glEnableVertexAttribArray(7);
		Offset += sizeof(typename T::TextureLayersType);
	}
	if ( HAS_ATTRIB(BT_LightInfo) ) // 8
	{
		if ( IsIntAttrib(T::LightInfoType::ArrayType) )
			glVertexAttribIPointer(8, T::LightInfoType::ArraySize, T::LightInfoType::ArrayType, DrawBuffer->BufferStride, BUFFER_OFFSET(Offset));
		else
			glVertexAttribPointer(8, T::LightInfoType::ArraySize, T::LightInfoType::ArrayType, GL_FALSE, DrawBuffer->BufferStride, BUFFER_OFFSET(Offset));
		glEnableVertexAttribArray(8);
		Offset += sizeof(typename T::LightInfoType);
	}
	if ( HAS_ATTRIB(BT_VertexParam0) ) // 9
	{
		if ( IsIntAttrib(T::VertexParam0Type::ArrayType) )
			glVertexAttribIPointer(9, T::VertexParam0Type::ArraySize, T::VertexParam0Type::ArrayType, DrawBuffer->BufferStride, BUFFER_OFFSET(Offset));
		else
			glVertexAttribPointer(9, T::VertexParam0Type::ArraySize, T::VertexParam0Type::ArrayType, GL_FALSE, DrawBuffer->BufferStride, BUFFER_OFFSET(Offset));
		glEnableVertexAttribArray(9);
		Offset += sizeof(typename T::VertexParam0Type);
	}
	if ( HAS_ATTRIB(BT_VertexParam1) ) // 10
	{
		if ( IsIntAttrib(T::VertexParam1Type::ArrayType) )
			glVertexAttribIPointer(10, T::VertexParam1Type::ArraySize, T::VertexParam1Type::ArrayType, DrawBuffer->BufferStride, BUFFER_OFFSET(Offset));
		else
			glVertexAttribPointer(10, T::VertexParam1Type::ArraySize, T::VertexParam1Type::ArrayType, GL_FALSE, DrawBuffer->BufferStride, BUFFER_OFFSET(Offset));
		glEnableVertexAttribArray(10);
		Offset += sizeof(typename T::VertexParam1Type);
	}
	#undef HAS_ATTRIB
	return true;
}
