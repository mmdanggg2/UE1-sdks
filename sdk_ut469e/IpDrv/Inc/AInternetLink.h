/*=============================================================================
	AInternetLink.h.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

	// Constructors
	AInternetLink();

	// UObject interface
	void Destroy();

	// AActor interface
	UBOOL Tick( FLOAT DeltaTime, enum ELevelTick TickType );	

	// AInternetLink interface
	void Destroyed(); // TODO: Move to AActor interface
	SOCKET& GetSocket() 
	{ 
		return *(SOCKET*)&Socket;
	}
	SOCKET& GetRemoteSocket() 
	{ 
		return *(SOCKET*)&RemoteSocket;
	}
	FResolveInfo*& GetResolveInfo()
	{
		return *(FResolveInfo**)&PrivateResolveInfo;
	}

	INT EncodeText(const TCHAR* Src, TArray<BYTE>& Out) const;
	FString DecodeText(const BYTE* Src) const;

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
