/*=============================================================================
	UnEditorNative.h: Native function lookup table for static libraries.
	Copyright 2021 OldUnreal. All Rights Reserved.

	Revision history:
		* Created by Stijn Volckaert
=============================================================================*/

#ifndef UNEDITORNATIVE_H
#define UNEDITORNATIVE_H

DECLARE_NATIVE_TYPE(Editor, UBrushBuilder);

#define AUTO_INITIALIZE_REGISTRANTS_EDITOR \
	UBatchExportCommandlet::StaticClass();\
	UBrushBuilder::StaticClass();\
	UConformCommandlet::StaticClass();\
	UCheckUnicodeCommandlet::StaticClass();\
	UPackageFlagCommandlet::StaticClass();\
	UDataRipCommandlet::StaticClass();\
	UDumpFontInfoCommandlet::StaticClass();\
	UStripSourceCommandlet::StaticClass();\
	UDumpIntCommandlet::StaticClass();\
	UListObjectsCommandlet::StaticClass();\
	UMakeCommandlet::StaticClass();\
	UMasterCommandlet::StaticClass();\
	UUpdateUModCommandlet::StaticClass();\
	UChecksumPackageCommandlet::StaticClass();\
	UMD5Commandlet::StaticClass();\
	UMergeDXTCommandlet::StaticClass();\
	UPackageDumpCommandlet::StaticClass();\
	UExecCommandlet::StaticClass();\
	UTextBufferExporterTXT::StaticClass();\
	USoundExporterWAV::StaticClass();\
	UMusicExporterTracker::StaticClass();\
	UClassExporterH::StaticClass();\
	UStrippedClassExporterUC::StaticClass();\
	UClassExporterUC::StaticClass();\
	UPolysExporterT3D::StaticClass();\
	UPolysExporterOBJ::StaticClass();\
	UModelExporterT3D::StaticClass();\
	ULevelExporterT3D::StaticClass();\
	ULevelFactoryNew::StaticClass();\
	UClassFactoryNew::StaticClass();\
	UTextureFactoryNew::StaticClass();\
	UClassFactoryUC::StaticClass();\
	ULevelFactory::StaticClass();\
	UPolysFactory::StaticClass();\
	UModelFactory::StaticClass();\
	USoundFactory::StaticClass();\
	UMusicFactory::StaticClass();\
	UTextureExporterPCX::StaticClass();\
	UTextureExporterBMP::StaticClass();\
	UEditorEngine::StaticClass();\
	UTransBuffer::StaticClass();\
	UTransactor::StaticClass();\
	UBitArray::StaticClass();\
	UBitMatrix::StaticClass();\
	UTextureFactory::StaticClass();		 \
	UTrueTypeFontFactory::StaticClass(); \
	UFontFactory::StaticClass();



#endif
