/*=============================================================================
	UnConn.h: Unreal network connection base class.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

class UNetDriver;

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack(push,OBJECT_ALIGNMENT)
#endif

/*-----------------------------------------------------------------------------
	UNetConnection.
-----------------------------------------------------------------------------*/

//
// Whether to support net lag and packet loss testing.
//
#define DO_ENABLE_NET_TEST 1

//
// State of a connection.
//
enum EConnectionState
{
	USOCK_Invalid   = 0, // Connection is invalid, possibly uninitialized.
	USOCK_Closed    = 1, // Connection permanently closed.
	USOCK_Pending	= 2, // Connection is awaiting connection.
	USOCK_Open      = 3, // Connection is open.
};

#if DO_ENABLE_NET_TEST
//
// A lagged packet
//
struct DelayedPacket
{
	TArray<BYTE> Data;
	FTime SendTime;
};
#endif

struct FDownloadInfo
{
	UClass* Class;
	FString ClassName;
	FString Params;
	UBOOL Compression;
};

struct FChannelAccs
{
	INT				InBunAcc,  OutBunAcc;	// Bunch accumulator.
	INT				InByteAcc, OutByteAcc;	// Byte accumulator.
};

//
// A network connection.
//
class ENGINE_API UNetConnection : public UPlayer
{
	DECLARE_ABSTRACT_CLASS(UNetConnection,UPlayer,CLASS_Transient|CLASS_Config,Engine)

	// Constants.
	enum{ MAX_PROTOCOL_VERSION = 1     };	// Maximum protocol version supported.
	enum{ MIN_PROTOCOL_VERSION = 1     };	// Minimum protocol version supported.
	enum{ MAX_CHANNELS         = 1023  };	// Maximum channels.

	// Connection information.
	UNetDriver*		Driver;					// Owning driver.
	EConnectionState State;					// State this connection is in.
	FURL			URL;					// URL of the other side.
	UPackageMap*	PackageMap;				// Package map between local and remote.

	// Negotiated parameters.
	INT				ProtocolVersion;		// Protocol version we're communicating with (<=PROTOCOL_VERSION).
	INT				RemoteVersion;			// The remote engine version number
	INT				MaxPacket;				// Maximum packet size.
	INT				PacketOverhead;			// Bytes overhead per packet sent.
	UBOOL			InternalAck;			// Internally ack all packets, for 100% reliable connections.
	INT				Challenge;				// Server-generated challenge.
	INT				NegotiatedVer;			// Negotiated version for new channels.
	INT				UserFlags;				// User-specified flags.
	FStringNoInit	RequestURL;				// URL requested by client

	// Internal.
	FTime			LastReceiveTime;		// Last time a packet was received, for timeout checking.
	FTime			LastSendTime;			// Last time a packet was sent, for keepalives.
	FTime			LastTickTime;			// Last time of polling.
	FTime			LastRepTime;			// Time of last replication.
	INT				QueuedBytes;			// Bytes assumed to be queued up.
	INT				TickCount;				// Count of ticks.
	UBOOL			PassedChallenge;		// Whether client has passed the challenge/response mechanism. // elmuerte: jack_porter security fix

	// Merge info.
	FBitWriterMark  LastStart;				// Most recently sent bunch start.
	FBitWriterMark  LastEnd;				// Most recently sent bunch end.
	UBOOL			AllowMerge;				// Whether to allow merging.
	UBOOL			TimeSensitive;			// Whether contents are time-sensitive.
	FOutBunch*		LastOutBunch;			// Most recent outgoing bunch.
	FOutBunch		LastOut;

	// Stat display.
	FTime			StatUpdateTime;			// Time of last stat update.
	FLOAT			StatPeriod;				// Interval between gathering stats.
	FLOAT			InRate,    OutRate;		// Rate for last interval.
	FLOAT			InPackets, OutPackets;	// Packet counts.
	FLOAT			InBunches, OutBunches;	// Bunch counts.
	FLOAT			InLoss,    OutLoss;		// Packet loss percent.
	FLOAT			InOrder,   OutOrder;	// Out of order incoming packets.
	FLOAT			BestLag,   AvgLag;		// Lag.

	// Stat accumulators.
	FLOAT			LagAcc, BestLagAcc;		// Previous msec lag.
	INT				InLossAcc, OutLossAcc;	// Packet loss accumulator.
	INT				InPktAcc,  OutPktAcc;	// Packet accumulator.
	INT				InBunAcc,  OutBunAcc;	// Bunch accumulator.
	INT				InByteAcc, OutByteAcc;	// Byte accumulator.
	INT				InOrdAcc,  OutOrdAcc;	// Out of order accumulator.
	INT				LagCount;				// Counter for lag measurement.
	INT				HighLossCount;			// Counts high packet loss.
	FTime			LastTime,CumulativeTime;
	FLOAT			FrameTime,AverageFrameTime;
	INT				CountedFrames;

	// Packet.
	FBitWriter		Out;					// Outgoing packet.
	FTime			OutLagTime[256];		// For lag measuring.
	INT				OutLagPacketId[256];	// For lag measuring.
	INT				InPacketId;				// Full incoming packet index.
	INT				OutPacketId;			// Most recently sent packet.
	INT 			OutAckPacketId;			// Most recently acked outgoing packet.

	// Channel table.
	UChannel*  Channels     [ MAX_CHANNELS ];
	INT        OutReliable  [ MAX_CHANNELS ];
	INT        InReliable   [ MAX_CHANNELS ];
	TArray<INT> QueuedAcks, ResendAcks;
	TArray<UChannel*> OpenChannels;
	TArray<AActor*> SentTemporaries;
	TMap<AActor*,UActorChannel*> ActorChannels;

	// File Download
	UDownload*				Download;
	TArray<FDownloadInfo>	DownloadInfo;
	INT						TotalFilesNeeded;		// stijn: Number of files we have to download
	INT						TotalDownloadSize;		// stijn: Total size of all files we need to download
	INT						TotalDownloadedSize;	// stijn: Total size of all files we've downloaded so far (excluding the file we're downloading right now)

	// Channel stats
	FChannelAccs			ChannelAccsData[2][MAX_CHANNELS];
	FChannelAccs*			ChannelAccs;
	FLOAT					LastRealTime;

#if DO_ENABLE_NET_TEST
	// For development.
	INT				PktLoss;
	INT				PktOrder;
	INT				PktDup;
	INT				PktLag;
	TArray<DelayedPacket> Delayed;
#endif

	// Actor channel cleanup
	TArray<AActor*> ActorsClosed;

	// Constructors and destructors.
	UNetConnection();
	UNetConnection( UNetDriver* Driver, const FURL& InURL );

	// UObject interface.
	void Serialize( FArchive& Ar );
	void Destroy();

	// UPlayer interface.
	void ReadInput( FLOAT DeltaSeconds );

	// FArchive interface.
	void Serialize( const TCHAR* Data, EName MsgType );

	// FExec interface.
	UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar=*GLog );

	// UNetConnection interface.
	virtual FString LowLevelGetRemoteAddress()=0;
	virtual FString LowLevelDescribe()=0;
	virtual void LowLevelSend( void* Data, INT Count )=0; //!! "Looks like an FArchive"
	virtual void InitOut();
	virtual void AssertValid();
	virtual void SendAck( INT PacketId, UBOOL FirstTime=1 );
	virtual void FlushNet();
	virtual void Tick();
	virtual INT IsNetReady( UBOOL Saturate );
	virtual void HandleClientPlayer( APlayerPawn* Pawn );

	// Functions.
	void PurgeAcks();
	void SendPackageMap();
	void SendPackageMapResponse();
	void PreSend( INT SizeBits );
	void PostSend();
	void ReceivedRawPacket( void* Data, INT Count );//!! "looks like an FArchive"
	INT SendRawBunch( FOutBunch& Bunch, UBOOL InAllowMerge );
	UNetDriver* GetDriver() {return Driver;}
	class UControlChannel* GetControlChannel();
	UChannel* CreateChannel( enum EChannelType Type, UBOOL bOpenedLocally, INT ChannelIndex=INDEX_NONE );
	void ReceivedPacket( FBitReader& Reader );
	void ReceivedNak( INT NakPacketId );
	void ReceiveFile( INT PackageIndex, FDownloadInfo* DlInfo );
	void SlowAssertValid()
	{
#if DO_GUARD_SLOW
		AssertValid();
#endif
	}
	FChannelAccs* GetChannelAccs();
	public:
};

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (pop)
#endif

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
