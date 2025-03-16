/*=============================================================================
	UnPNG.h: Unreal PNG support.
	Copyright 2017-2018 Sebastian Kaufel. All Rights Reserved.

	Revision history:
		* Created by Sebastian Kaufel.
=============================================================================*/

#define TEXC_sRGB 0x03

/*-----------------------------------------------------------------------------
	FPortableNetworksGraphicArchive.
-----------------------------------------------------------------------------*/

// Integrates of libpng into FArchive workflow.
struct ENGINE_API FPortableNetworksGraphicArchive
{
	// Variables.
	FArchive* Ar{};
	bool ShouldDeleteAr{};
	struct png_struct_def* Write{};
	struct png_struct_def* Read{};
	struct png_info_def*   Info{};

	// Static function which mimics FFileManager interface.
	static FPortableNetworksGraphicArchive* CreateFileWriter( const TCHAR* Filename, DWORD WriteFlags=0, FOutputDevice* Error=GNull, FFileManager* FileManager=GFileManager );
	static FPortableNetworksGraphicArchive* CreateFileReader( const TCHAR* Filename, DWORD ReadFlags=0, FOutputDevice* Error=GNull, FFileManager* FileManager=GFileManager );

	// stijn: support for Texture export/import
	static FPortableNetworksGraphicArchive* CreateWriterInternal(FArchive* Orig, FOutputDevice* Error=GNull );
	static UBOOL ReadRawImage(const BYTE* RawData, DWORD RawDataLen, TArray<BYTE>& Data, BYTE& Format, BYTE& Colorspace, UINT& USize, UINT& VSize);
 
	// Image access.
	void WriteImage( BYTE* Data, BYTE Format, BYTE Colorspace, INT USize, INT VSize ); // !! Hardwired to URenderDevice::ReadPixels() for now.
	void ReadImage( TArray<BYTE>& Data, BYTE& Format, BYTE& Colorspace, UINT& USize, UINT& VSize );

	// Destructor.
	~FPortableNetworksGraphicArchive();

	// Archive Foo.
	UBOOL IsLoading()   { return ArIsLoading; }
	UBOOL IsSaving()    { return ArIsSaving;  }
	UBOOL IsError()     { return ArIsError;   }

private:
	enum EPortableNetworksGraphicArchiveWriter { E_PortableNetworksGraphicArchiveWriter };
	enum EPortableNetworksGraphicArchiveReader { E_PortableNetworksGraphicArchiveReader };

	// Constructors.
	FPortableNetworksGraphicArchive( EPortableNetworksGraphicArchiveWriter, FArchive* InAr, struct png_struct_def* InWrite, struct png_info_def* InInfo );
	FPortableNetworksGraphicArchive( EPortableNetworksGraphicArchiveReader, FArchive* InAr, struct png_struct_def* InRead, struct png_info_def* InInfo );

	// Callbacks.
	static void ArchiveWriteFunction( struct png_struct_def* Write, BYTE* Data, UINT Length );
	static void ArchiveReadFunction( struct png_struct_def* Read, BYTE* Data, UINT Length );
	static void RawReadFunction(struct png_struct_def* Read, BYTE* Data, UINT Length);

protected:
	// Status variables.
	UBOOL ArIsLoading;
	UBOOL ArIsSaving;
	UBOOL ArIsError;
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
