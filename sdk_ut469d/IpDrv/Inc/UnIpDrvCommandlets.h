/*=============================================================================
	UnIpDrvCommandlets.h: IpDrv package commandlet declarations.
	Copyright 2000 Epic Games, Inc. All Rights Reserved.

Revision history:
	* (4/14/99) Created by Brandon Reinhart
	* (4/15/99) Ported to UCC Commandlet interface by Brandon Reinhart

Todo:
	* Gspy style interface could be expanded to complete compliance
	  (allows for custom queries)
=============================================================================*/

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack(push,OBJECT_ALIGNMENT)
#endif

struct FMasterMap
{
	INT Version;
	TMap<FString, FString> Map;

	friend FArchive& operator<<( FArchive& Ar, FMasterMap& M )
	{
		guard(FMasterMap<<);
		return Ar << M.Version << M.Map;
		unguard;
	}
};
class UMasterServerCommandlet : public UCommandlet
{
	DECLARE_CLASS(UMasterServerCommandlet, UCommandlet, CLASS_Transient,IpDrv);

	FSocketData ListenSocket;
	FOperationStats OpStats, OldStats;

	INT MASTER_TIMEOUT;

	// Output
	FString GameName;
	FString OpMode;

	// TextFile Mode
	FString OutputFileName;

	// TCPLink Mode
	INT TCPPort;
	FSocketData TCPSocket;
	SOCKET Connections[100];
	INT ConnectionTimer[100];
	INT ConnectionCount;

	// Server Map
	TMap<FString, FString> ValidationMap;	// Servers awaiting validation.
	TArray<FMasterMap> MasterMaps;			// Servers in the master list, by version number
	INT NumServers;
	FTime Last10Seconds, Last300Seconds;
	INT IgnoredValidations;
	INT RejectedValidations;
	INT MaxGroupGameVer;

	void InitSockets( const TCHAR* ConfigFileName );
	UBOOL ConsoleReadInput();
	void ListenSockets();
	void GSValidate( FString* ValidationString, FString* const ValidationResult, FString* ValidateGameName );
	UBOOL GetNextKey( FString* Message, FString* Result );
	void ServiceMessage( FString& Message, sockaddr_in* FromAddr );
	void DoHeartbeat( sockaddr_in* FromAddr, FString& PortID, INT GameVer );
	void DoValidate( sockaddr_in* FromAddr, FString& ChallengeResponse );
	void PollConnections();
	void ServiceTCPMessage( FString& Message, INT Index );
	void PurgeValidationMap();
	void PurgeMasterMap();
	void WriteMasterMap();
	void CleanUp();
	INT Main( const TCHAR* Parms );
};

class UUpdateServerCommandlet : public UCommandlet
{
	DECLARE_CLASS(UUpdateServerCommandlet, UCommandlet, CLASS_Transient,IpDrv);

	// System
	FSocketData Socket;
	FOperationStats OpStats;

	// Key/Response
	TMap<FString, FString> Pairs;

	FArchive* Log;

	UBOOL GetNextKey( FString* Message, FString* Result );
	INT SendResponse( FString Key, sockaddr_in* FromAddr );
	FString GetIpAddress( sockaddr_in* FromAddr );
	void ServiceMessage( FString Message, sockaddr_in* FromAddr );
	void InitSockets( const TCHAR* ConfigFileName );
	UBOOL ConsoleReadInput( const TCHAR* ConfigFileName );
	void ListenSockets();
	void ReadKeyResponses( const TCHAR* ConfigFileName );
	void CleanUp();
	void ReloadSettings( const TCHAR* ConfigFileName );
	INT Main( const TCHAR* Parms );
};

class UCompressCommandlet : public UCommandlet
{
	DECLARE_CLASS(UCompressCommandlet, UCommandlet, CLASS_Transient,IpDrv);

	INT Main( const TCHAR* Parms );
};

class UDecompressCommandlet : public UCommandlet
{
	DECLARE_CLASS(UDecompressCommandlet, UCommandlet, CLASS_Transient,IpDrv);

	INT Main( const TCHAR* Parms );
};

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (pop)
#endif

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
