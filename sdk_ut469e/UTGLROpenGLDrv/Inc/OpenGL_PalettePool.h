/*=============================================================================
	OpenGL_PalettePool.h
	
	Palette pooling system.

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

namespace FGL
{

//
// Remap a Unreal Palette into a GL memory block
//
struct FPaletteRemap
{
	INT PoolIndex;
	GLsizei Offset;

	// Mirrored from FPaletteBlock
	GLuint SSBO;
};

//
// Defines a GL memory block containing palettes
//
struct FPaletteBlock
{
	GLsizei NumPalettes;
	GLuint SSBO;
};


//
// Palette pooling system
//
class FPalettePool
{
public:

	TMapExt<FCacheID,FPaletteRemap> Remaps;
	TArrayExt<FPaletteBlock> Blocks;
	
	GLsizei BlockSize = 0;

	void Init();
	void Flush();

	FPaletteRemap& GetRemap(FCacheID CacheID, const FColor* Colors);
};

}