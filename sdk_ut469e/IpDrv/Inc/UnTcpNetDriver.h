/*=============================================================================
	UnTcpNetDriver.h: Unreal TCP/IP driver.
	Copyright 1997-2000 Epic Games, Inc. All Rights Reserved.

	Revision history:
	* Created by Brandon Reinhart.
=============================================================================*/

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack(push,OBJECT_ALIGNMENT)
#endif

/*-----------------------------------------------------------------------------
	UTcpipConnection.
-----------------------------------------------------------------------------*/

//
// Windows socket class.
//
class DLL_EXPORT_CLASS UTcpipConnection : public UNetConnection
{
	DECLARE_CLASS(UTcpipConnection,UNetConnection,CLASS_Config|CLASS_Transient,IpDrv)
	NO_DEFAULT_CONSTRUCTOR(UTcpipConnection)

	// Variables.
	sockaddr_in		RemoteAddr;
	SOCKET			Socket;
	UBOOL			OpenedLocally;
	FResolveInfo*	ResolveInfo;
	FTime			OpenedTime; //elmuerte: jack_porter security fix

	// Constructors and destructors.
	UTcpipConnection( SOCKET InSocket, UNetDriver* InDriver, sockaddr_in InRemoteAddr, EConnectionState InState, UBOOL InOpenedLocally, const FURL& InURL );
	void Destroy();

	void LowLevelSend( void* Data, INT Count );
	FString LowLevelGetRemoteAddress();
	FString LowLevelDescribe();
};

/*-----------------------------------------------------------------------------
	UTcpNetDriver.
-----------------------------------------------------------------------------*/

//!!oldver
// Keep track of recently closed sockets for 5 seconds to avoid reopening them with acks
struct FRecentClosedSocket
{
	FTime		Expires;
	FString		Address;

	FRecentClosedSocket() {}
	FRecentClosedSocket(FTime InExpires, FString InAddress) :
		Expires(InExpires),	Address(InAddress) {}

	friend FArchive &operator<<( FArchive& Ar, FRecentClosedSocket& S )
		{return Ar << S.Expires << S.Address;}
};
//
// Windows sockets network driver.
//
class DLL_EXPORT_CLASS UTcpNetDriver : public UNetDriver
{
	DECLARE_CLASS(UTcpNetDriver,UNetDriver,CLASS_Transient|CLASS_Config,IpDrv)

	// Variables.
	sockaddr_in	LocalAddr;
	SOCKET		Socket;
	TArray<FRecentClosedSocket> RecentClosed;
	UBOOL OverrideBufferAllocation;
	INT MaxConnPerIPPerMinute;   // elmuerte: jack_porter security fix
	UBOOL LogMaxConnPerIPPerMin; // elmuerte: jack_porter security fix
#ifdef __UNIX__
	FLOAT RecvSizeMult;
	FLOAT SendSizeMult;
#endif
	UBOOL AllowPlayerPortUnreach;
	UBOOL LogPortUnreach;

	// Constructor.
	UTcpNetDriver()
	{}

	// UObject interface.
	void Serialize( FArchive& Ar );
	void StaticConstructor();

	// UNetDriver interface.
	UBOOL InitConnect( FNetworkNotify* InNotify, FURL& ConnectURL, FString& Error );
	UBOOL InitListen( FNetworkNotify* InNotify, FURL& LocalURL, FString& Error );
	void TickDispatch( FLOAT DeltaTime );
	FString LowLevelGetNetworkNumber();
	void LowLevelDestroy();

	// UTcpNetDriver interface.
	UBOOL InitBase( UBOOL Connect, FNetworkNotify* InNotify, FURL& URL, FString& Error );
	UTcpipConnection* GetServerConnection();
};

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (pop)
#endif
