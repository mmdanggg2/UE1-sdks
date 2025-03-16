/*=============================================================================
	OpenGL_Palette.cpp: Palette handling code for OpenGLDrv.

	The palette pooling system consists on assigning 1kb blocks to individual
	palettes, and then assigning every 'remap' a buffer index and offset

	Given that compute shaders are needed to write to texture, there's no point
	in worrying about the type of buffer used for palettes, so SSBO's will be
	used in all cases.

	Each block of memory will have at least a size of 4 megabytes.

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

#include "OpenGLDrv.h"
#include "UTGLROpenGL.h"


/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

FGL::FPalettePool UOpenGLRenderDevice::PalettePool;


/*-----------------------------------------------------------------------------
	FPalettePool implementation.
-----------------------------------------------------------------------------*/

void FGL::FPalettePool::Init()
{
	// Minimum of 4 megabytes
	BlockSize = 4 * 1024 * 1024;
}

void FGL::FPalettePool::Flush()
{
	if ( Blocks.Num() == 0 )
		return;

	FMemMark Mark(GMem);

	// Unbind this slot from all contexts
	for ( INT i=0; i<FOpenGLBase::Instances.Num(); i++)
		FOpenGLBase::Instances(i)->BindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, 0);

	GLuint* Buffers = new(GMem, Blocks.Num()) GLuint;
	for ( INT i=0; i<Blocks.Num(); i++)
		Buffers[i] = Blocks(i).SSBO;

	FOpenGLBase::glDeleteBuffers(Blocks.Num(), Buffers);
	Blocks.Empty();
	Remaps.Empty();

	Mark.Pop();

	// debugf(NAME_DevGraphics, TEXT("FLUSHED SSBO"));
}

FGL::FPaletteRemap& FGL::FPalettePool::GetRemap(FGL::FCacheID CacheID, const FColor* Colors)
{
	FGL::FPaletteRemap* Remap = Remaps.Find(CacheID);
	if ( !Remap )
	{
		Remap = &Remaps.SetNoFind(CacheID,FGL::FPaletteRemap());

		if ( (Blocks.Num() > 0) && (Blocks.Last().NumPalettes * 1024 < BlockSize) )
		{
			// Assign to existing block
			Remap->PoolIndex = Blocks.Num() - 1;
		}
		else
		{
			// Assign to new block and create SSBO
			Remap->PoolIndex = Blocks.AddZeroed();
			FOpenGLBase::CreateInmutableBuffer(Blocks(Remap->PoolIndex).SSBO, GL_SHADER_STORAGE_BUFFER, BlockSize, nullptr, GL_DYNAMIC_STORAGE_BIT);

			// debugf(NAME_DevGraphics, TEXT("CREATED SSBO %i"), Blocks(Remap->PoolIndex).SSBO);
		}

		// Finish setting up remap
		FPaletteBlock& Block = Blocks(Remap->PoolIndex);
		Remap->Offset = Block.NumPalettes++ * 1024;
		Remap->SSBO = Block.SSBO;
		GL_DEV_CHECK(Remap->SSBO);

		// debugf(NAME_DevGraphics, TEXT("ASSIGNED SSBO %i TO REMAP %i WITH OFFSET %i"), Blocks(Remap->PoolIndex).SSBO, Remaps.Num()-1, Remap->Offset);

		// Upload color palette into SSBO
		FOpenGLBase::glBindBuffer(GL_SHADER_STORAGE_BUFFER, Remap->SSBO);
		FOpenGLBase::glBufferSubData(GL_SHADER_STORAGE_BUFFER, Remap->Offset, 1024, Colors);
		FOpenGLBase::glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}
	return *Remap;
}
