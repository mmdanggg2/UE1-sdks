/*=============================================================================
	UnMD5: Unreal MD5 lookup table

	Revision history:
	* Created by Michiel Hendriks
=============================================================================*/

#ifdef UTPG_MD5

#ifndef UTPG_MD5_H
#define UTPG_MD5_H

class CORE_API FMD5Record
{
public:
	FString Filename;
	FGuid   Guid;
	INT     Generation{};
	FString MD5Checksum;
	DWORD   FileTime{};

	FMD5Record() = default;

	FMD5Record( FString InFilename, FGuid InGuid, INT InGeneration, FString InMD5Checksum, DWORD InFiletime) 
		: Filename(InFilename)
		, Guid(InGuid)
		, Generation(InGeneration)
		, MD5Checksum(InMD5Checksum)
		, FileTime(InFiletime)
	{}

	FMD5Record( const TCHAR* InFilename, FArchive* OptAr=nullptr);

	CORE_API friend FArchive& operator<<( FArchive& Ar, FMD5Record& Record );
	bool operator==(const FMD5Record& B) const
	{
		// stijn: The (GUID, Generation) tuple uniquely identifies a file
		return B.Generation == Generation && B.Guid == Guid;
	}
	FString String();
};

class CORE_API UMD5Table
{	
public:
	CORE_API friend FArchive& operator<<( FArchive& Ar, UMD5Table& Table );

	bool LoadFromFile( const TCHAR* Filename, FString* OutError=nullptr);

	void AddRecord(FMD5Record* Record);
	INT RemoveRecord(FGuid InGuid, INT InGeneration); // fast on failure. kind of slow on success
	INT RemoveRecord(FString InFileName); // always slow
	bool Empty();

	const TCHAR* LookupMD5Checksum(FGuid InGuid, INT InGeneration); // fast
	FMD5Record* LookupMD5Record(FGuid InGuid, INT InGeneration); //fast
	const TCHAR* LookupMD5Checksum(FString InFilename, INT InGeneration); // slow
	TArray<FString> LookupMD5Checksums(FGuid InGuid); // ultra slow
	bool HasMD5Checksum( FGuid InGuid); //Used for master map generation.
	~UMD5Table();

	TMultiMap<FGuid, FMD5Record> Map;
};

#endif // UTPG_MD5_H

#endif // UTPG_MD5

/*----------------------------------------------------------------------------
	The End.
----------------------------------------------------------------------------*/
