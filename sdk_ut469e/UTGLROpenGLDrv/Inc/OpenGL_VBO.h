/*=============================================================================
	OpenGL_VBO.h: Structures used in static GPU vertex buffers.

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

#define BUFFER_OFFSET(i) ((void*)(i))

namespace FGL
{
namespace VBO
{

//
// Default VBO prototype
//
struct FBase
{
	static constexpr bool HasStaticVertexData() { return false; }
	static constexpr bool HasStaticColor0Data() { return false; }
	static constexpr bool HasStaticColor1Data() { return false; }
	static constexpr bool HasStaticTexture0Data() { return false; }
	static constexpr bool HasStaticTexture1Data() { return false; }
	static constexpr bool HasStaticTexture2Data() { return false; }
	static constexpr bool HasStaticNormalData() { return false; }
	static constexpr bool HasStaticVertexParam0Data() { return false; }
	static constexpr bool HasStaticVertexParam1Data() { return false; }

	static constexpr intptr_t VertexOffset() { return 0; }
	static constexpr intptr_t Color0Offset() { return 0; }
	static constexpr intptr_t Color1Offset() { return 0; }
	static constexpr intptr_t Texture0Offset() { return 0; }
	static constexpr intptr_t Texture1Offset() { return 0; }
	static constexpr intptr_t Texture2Offset() { return 0; }
	static constexpr intptr_t NormalOffset() { return 0; }
	static constexpr intptr_t VertexParam0Offset() { return 0; }
	static constexpr intptr_t VertexParam1Offset() { return 0; }

	// Default sizes
	static constexpr GLenum VertexArraySize() { return FGL::FVertex3D::ArraySize; }
	static constexpr GLenum Color0ArraySize() { return FGL::FColorRGBA::ArraySize; }
	static constexpr GLenum Color1ArraySize() { return FGL::FColorRGBA::ArraySize; }
	static constexpr GLenum Texture0ArraySize() { return FGL::FTextureUV::ArraySize; }
	static constexpr GLenum Texture1ArraySize() { return FGL::FTextureUV::ArraySize; }
	static constexpr GLenum Texture2ArraySize() { return FGL::FTextureUV::ArraySize; }
	static constexpr GLenum NormalArraySize() { return FGL::FVertex3D::ArraySize; }
	static constexpr GLenum VertexParam0ArraySize() { return 1; }
	static constexpr GLenum VertexParam1ArraySize() { return 1; }

	// Default types
	static constexpr GLenum VertexArrayType() { return FGL::FVertex3D::ArrayType; }
	static constexpr GLenum Color0ArrayType() { return FGL::FColorRGBA::ArrayType; }
	static constexpr GLenum Color1ArrayType() { return FGL::FColorRGBA::ArrayType; }
	static constexpr GLenum Texture0ArrayType() { return FGL::FTextureUV::ArrayType; }
	static constexpr GLenum Texture1ArrayType() { return FGL::FTextureUV::ArrayType; }
	static constexpr GLenum Texture2ArrayType() { return FGL::FTextureUV::ArrayType; }
	static constexpr GLenum NormalArrayType() { return FGL::FVertex3D::ArrayType; }
	static constexpr GLenum VertexParam0ArrayType() { return GL_FLOAT; }
	static constexpr GLenum VertexParam1ArrayType() { return GL_FLOAT; }
};


//
// Static BSP
//
struct FStaticBsp : public FBase
{
	FGL::FVertex3D     Vertex;
	FGL::FTextureUVPan TextureUVPan;
	FGL::FTextureUV    LightUV, AtlasUV;
	GLfloat            AutoU, AutoV, Wavy;

	static constexpr bool HasStaticVertexData() { return true; }
	static constexpr bool HasStaticTexture0Data() { return true; }
	static constexpr bool HasStaticTexture1Data() { return true; }
	static constexpr bool HasStaticTexture2Data() { return true; }

	static constexpr intptr_t VertexOffset() { return 0; }
	static constexpr intptr_t Texture0Offset() { return sizeof(Vertex); }
	static constexpr intptr_t Texture1Offset() { return Texture0Offset() + sizeof(TextureUVPan); }
	static constexpr intptr_t Texture2Offset() { return Texture1Offset() + sizeof(LightUV) + sizeof(AtlasUV); }

	static constexpr GLenum Texture0ArraySize() { return FGL::FTextureUVPan::ArraySize; }
	static constexpr GLenum Texture1ArraySize() { return FGL::FTextureUV::ArraySize * 2; }
	static constexpr GLenum Texture2ArraySize() { return 3; }
};
struct FStaticBspGL3 : public FStaticBsp
{
	static constexpr GLenum Texture0ArraySize()     { return FGL::FTextureUV::ArraySize; }

	// VertexParam = TexturePan in shader
	static constexpr bool HasStaticVertexParamData() { return true; }
	static constexpr intptr_t VertexParam0Offset()       { return Texture0Offset() + sizeof(FGL::FTextureUV); }
	static constexpr GLenum VertexParam0ArraySize()   { return FGL::FTexturePan::ArraySize; }
};


//
// FillScreen
//
struct FFill : public FBase
{
	FGL::FVertex3D  Vertex;
	FGL::FTextureUV UV;

	static constexpr bool HasStaticVertexData() { return true; }
	static constexpr bool HasStaticTexture0Data() { return true; }

	static constexpr intptr_t VertexOffset() { return 0; }
	static constexpr intptr_t Texture0Offset() { return sizeof(Vertex); }
};

};
};
