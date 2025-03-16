/*=============================================================================
	OpenGL_TextureFormat.h
	
	Texture Format handling header.

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

typedef bool (FASTCALL *f_TextureConvert)(struct FTextureUploadState&);

struct FTextureUploadState
{
	FTextureInfo*   Info          = nullptr;
	BYTE*           Data          = nullptr;
	FOpenGLTexture* Texture       = nullptr;
	INT             Level         = 0;
	INT             CurrentMip    = 0;
	DWORD           PolyFlags     = 0;
	INT             USize         = 0;
	INT             VSize         = 0;
	INT             Layer         = 0;
	FOpenGLSwizzle  Swizzle       = 0;
	BYTE            Format        = 0;
	BYTE            InitialFormat = 0;

	FTextureUploadState() = delete;
	FTextureUploadState( const FTextureInfo* InInfo, DWORD InPolyFlags=0)
		: Info          ((FTextureInfo*)InInfo)
		, PolyFlags     (InPolyFlags)
		, Format        (InInfo->Format)
		, InitialFormat (InInfo->Format)
	{}

	// Selects a conversion function, this changes the Format!!
	f_TextureConvert SelectHardwareConversion();
	f_TextureConvert SelectSoftwareConversion();
};

struct FTextureFormatInfo
{
	GLenum InternalFormat;
	GLenum SourceFormat;
	GLenum Type;
	GLubyte BlockWidth;
	GLubyte BlockHeight;
	GLubyte BlockBytes;
	GLubyte Supported:1;
	GLubyte Compressed:1;

	FTextureFormatInfo()
	{}

	GLuint GetTextureBytes( GLuint Width, GLuint Height, GLuint Depth=1) const
	{
		GLuint Size = BlockBytes;
		Size *= Align<GLuint>(Width,BlockWidth) / (GLuint)BlockWidth;
		Size *= Align<GLuint>(Height,BlockHeight) / (GLuint)BlockHeight;
		// No 3D for now.
		return Size;
	}

	static void Init( bool bUseTC);
	static const FTextureFormatInfo& Get( GLubyte Format);
};

//
// Checks if palette indices match RGBA values (TrueType font)
// TODO: Move this out of header
static inline bool PaletteGrayscaleFont( FColor* Palette)
{
	for ( DWORD i=0; i<NUM_PAL_COLORS; i++)
		if ( GET_COLOR_DWORD(Palette[i]) != ((i)|(i<<8)|(i<<16)|(i<<24)) )
			return false;
	return true;
}
