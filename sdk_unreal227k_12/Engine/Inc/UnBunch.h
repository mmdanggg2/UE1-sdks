/*=============================================================================
	UnBunch.h: Unreal bunch class.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

//
// A bunch of data to send.
//
class ENGINE_API FOutBunch : public FBitWriter
{
public:
	// Variables.
	FOutBunch*		Next;
	FChannel*		Channel;
	FTime			Time;
	UBOOL			ReceivedAck;
	INT				ChIndex;
	INT				ChType;
	INT				ChSequence;
	INT				PacketId;
	BYTE			bOpen;
	BYTE			bClose;
	BYTE			bReliable;

	// Functions.
	FOutBunch();
	FOutBunch( FChannel* InChannel, UBOOL bClose );
	FArchive& operator<<( FName& Name );
	FArchive& operator<<( UObject*& Object );
};

//
// A bunch of data received from a channel.
//
class ENGINE_API FInBunch : public FBitReader
{
public:
	// Variables.
	INT				PacketId;
	FInBunch*		Next;
	UNetConnection*	Connection;
	INT				ChIndex;
	INT				ChType;
	INT				ChSequence;
	BYTE			bOpen;
	BYTE			bClose;
	BYTE			bReliable;

	// Functions.
  //elmuerte: security fix -- begin
	/*FInBunch( UNetConnection* InConnection )
	:	FBitReader	()
	,	Next		( NULL )
	,	Connection  ( InConnection )
	{}*/
  FInBunch( UNetConnection* InConnection ); //elmuerte: security fix -- end
	FArchive& operator<<( FName& Name );
	FArchive& operator<<( UObject*& Object );
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
