/*=============================================================================
	IpDrvPrivate.h: Unreal TCP/IP driver.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Tim Sweeney.
=============================================================================*/

// Socket API.
#if WIN32
	#define __WINSOCK__ 1
	#define SOCKET_API TEXT("WinSock")
#else
	#define __BSD_SOCKETS__ 1
	#if ((MACOSX) || (defined __FreeBSD__))
		#define __BSD_SOCKETS_TRUE_BSD__ 1
	#endif
	#define SOCKET_API TEXT("Sockets")
#endif

// WinSock includes.
#if __WINSOCK__
	#include <WS2tcpip.h>
	#pragma warning(push)
	#pragma warning(disable: 4121) /* 'JOBOBJECT_IO_RATE_CONTROL_INFORMATION_NATIVE_V2': alignment of a member was sensitive to packing */
	#include <windows.h>
	#pragma warning(pop)
	#include <conio.h>
	#pragma warning(disable: 4389) /* warning C4389: '==': signed/unsigned mismatch */
#endif

// BSD socket includes.
#if __BSD_SOCKETS__
	#include <stdio.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <sys/uio.h>
	#include <sys/ioctl.h>
	#include <sys/time.h>
	#include <errno.h>
	#include <pthread.h>
	#include <fcntl.h>

	// Handle glibc < 2.1.3.
	#ifndef MSG_NOSIGNAL
	#define MSG_NOSIGNAL 0x4000
	#endif
#endif

#include "Engine.h"
#include "UnNet.h"
#include "UnSocket.h"

inline SOCKET SocketCast(void* SockIn)
{
#if !BUILD_64
	return (SOCKET) SockIn;
#elif WIN32
	return reinterpret_cast<SOCKET>(SockIn);
#else
	return static_cast<SOCKET>(reinterpret_cast<PTRINT>(SockIn));
#endif
}

#ifdef _WIN32
// stijn: Win XP-compatible implementation of inet_ntop
inline const char* XpInetNtop(int Family, const in_addr* SrcAddr, char* DstString, int DstStringSize)
{
	struct sockaddr_in TmpAddr {};
	TmpAddr.sin_family = Family;
	memcpy(&TmpAddr.sin_addr, SrcAddr, sizeof(TmpAddr.sin_addr));

	if (WSAAddressToStringA(
		reinterpret_cast<struct sockaddr*>(&TmpAddr),
		sizeof(struct sockaddr_in),
		NULL,
		DstString,
		reinterpret_cast<LPDWORD>(&DstStringSize)) != 0)
	{
		return NULL;
	}
	return DstString;
}

// stijn: Win XP-compatible implementation of inet_pton
inline int XpInetPton(int af, const char* src, void* dst)
{
	struct sockaddr_storage ss;
	int size = sizeof(ss);
	char src_copy[INET6_ADDRSTRLEN + 1];

	ZeroMemory(&ss, sizeof(ss));
	/* stupid non-const API */
	strncpy(src_copy, src, INET6_ADDRSTRLEN + 1);
	src_copy[INET6_ADDRSTRLEN] = 0;

	if (WSAStringToAddressA(src_copy, af, NULL, (struct sockaddr*) & ss, &size) == 0) {
		switch (af) {
		case AF_INET:
			*(struct in_addr*)dst = ((struct sockaddr_in*) & ss)->sin_addr;
			return 1;
		case AF_INET6:
			*(struct in6_addr*)dst = ((struct sockaddr_in6*) & ss)->sin6_addr;
			return 1;
		}
	}
	return 0;
}
#endif

/*-----------------------------------------------------------------------------
	Definitions.
-----------------------------------------------------------------------------*/

// An IP address.
struct FIpAddr
{
	DWORD Addr;
	DWORD Port;
};

// SocketData.
struct FSocketData
{
	SOCKADDR_IN Addr;
	INT Port;
	SOCKET Socket;
};

// OperationStats
struct FOperationStats
{
	INT MessagesServiced;
	INT BytesReceived;
	INT BytesSent;
};

// Globals.
extern UBOOL GInitialized;

/*-----------------------------------------------------------------------------
	Host resolution thread.
-----------------------------------------------------------------------------*/

#if WIN32
DWORD STDCALL ResolveThreadEntry(void* Arg);
#else
void* ResolveThreadEntry(void* Arg);
#endif

//
// Class for creating a background thread to resolve a host.
//
class FResolveInfo
{
public:
	// Variables.
	in_addr			Addr{};
    ANSICHAR*	    HostName{};
    ANSICHAR*	    Error{};

#if WIN32
    HANDLE			ResolveThread{};	
#else
    pthread_t		ResolveThread{};
#endif

	// Functions.
	FResolveInfo( const TCHAR* InHostName )
	{
		HostName = (ANSICHAR*) malloc(appStrlen(InHostName) + 1);
		debugf( TEXT("Resolving %s..."), InHostName );
		if (HostName)
			appMemcpy( HostName, appToAnsi(InHostName), appStrlen(InHostName) + 1 );
	
#if WIN32
		// stijn: we set a rather large stack commit size in the build flags, but resolve threads don't need a lot of stack size...
		// 256Kb should be more than enough
		ResolveThread = CreateThread( NULL, 256 * 1024, ResolveThreadEntry, this, 0, NULL );
		check(ResolveThread);
#else
		pthread_create( &ResolveThread, NULL, &ResolveThreadEntry, this );
#endif

	}
	~FResolveInfo()
	{
		// In the _VERY_ unlikely event that the resolve thread is still active,
		// just let the memory leak. This is not ideal, but it beats memory
		// corruption.
		if (Resolved())
		{
			if (HostName) free(HostName);
            if (Error) free(Error);
#if !WIN32
			if (ResolveThread) pthread_join(ResolveThread, NULL);
#endif
		}

#if WIN32
		if (ResolveThread) CloseHandle(ResolveThread);
#endif
	}
	UBOOL Resolved()
	{
#if WIN32
		if (!ResolveThread)
			return 1;

		DWORD ExitCode = NULL;
		if (GetExitCodeThread(ResolveThread, &ExitCode))
			if (ExitCode == STILL_ACTIVE)
				return 0;

		return 1;
		
#else
		if (!ResolveThread)
			return 1;

		// does not send a signal. just checks if thread is still alive
        if (pthread_kill(ResolveThread, 0))
            return 1;

		return 0;
#endif
	}
	const in_addr GetAddr()
	{
		return Addr;
	}
	const TCHAR* GetHostName()
	{
		return HostName ? appFromAnsi(HostName) : TEXT("");
	}
};
/*-----------------------------------------------------------------------------
	Bind to next available port.
-----------------------------------------------------------------------------*/

#ifdef __linux__
#include <stdlib.h>  // (need rand() for bindnextport().  --ryan.)
#endif

//
// Bind to next available port.
//
inline int bindnextport( SOCKET s, struct sockaddr_in* addr, int portcount, int portinc )
{
	guard(bindnextport);

#ifdef __linux__
    // Long explanation follows:
    //
    // Using port 0 tells the OS to allocate an unused port. But, this will
    // allocate the same port as the socket we just closed on Linux's
    // TCP/IP stack. If you get a stray packet from the last socket, the game
    // gets VERY confused. This is most obvious by hitting a passworded server
    // with the wrong password. We connect, fail to gain access, user enters a
    // password and we closesocket() and immediately hit the server again with
    // a newly bound port...and gets some extra unexpected chatter that hangs
    // the client.
    //
    // The workaround is to pick our own random ports if port zero is
    // requested. If we can't find a valid port above 1024 (below 1024 is
    // reserved for the superuser) after a couple tries, we block for three
    // seconds and then continue as normal...the blocking sucks, but
    // tends to fix things, too. Oh well.
    //
    // The MacOS X and Win32 TCP/IP stacks do not exhibit this problem.
    //
    //    --ryan.

    if( addr->sin_port==0 )
    {
        // try 10 random ports...
        for (int i = 0; i < 10; i++)
        {
            short p = ((short) (31742.0f * rand() / (RAND_MAX + 1.0))) + 1024;
            addr->sin_port = htons(p);
            int rc = bindnextport(s, addr, portcount, portinc);
            if (rc != 0)
                return(rc);  // got one.
        }

        debugf( NAME_Warning, TEXT("Failed to manually bind random port. Letting OS do it instead.") );
        appSleep(3.0f);
        addr->sin_port = 0;
    }
#endif


	for( int i=0; i<portcount; i++ )
	{
		if( !bind( s, (sockaddr*)addr, sizeof(sockaddr_in) ) )
		{
			if (ntohs(addr->sin_port) != 0)
				return ntohs(addr->sin_port);
			else
			{
				// 0 means allocate a port for us, so find out what that port was
				struct sockaddr_in boundaddr;
#if WIN32
				INT size = sizeof(boundaddr);
#else
				socklen_t size = sizeof(boundaddr);
#endif
				getsockname ( s, (sockaddr*)(&boundaddr), &size);
				return ntohs(boundaddr.sin_port);
			}
		}
		if( addr->sin_port==0 )
			break;
		addr->sin_port = htons( ntohs(addr->sin_port) + portinc );
	}
	return 0;
	unguard;
}

inline int getlocalhostaddr( FOutputDevice& Out, in_addr &HostAddr )
{
	guard(getlocalhostaddr);
	int CanBindAll = 0;
	IpSetInt( HostAddr, INADDR_ANY );
	FString Home, HostName;
	ANSICHAR AnsiHostName[256]="";
#ifndef MACOSX
	if( gethostname( AnsiHostName, 256 ) )
		Out.Logf( TEXT("%s: gethostname failed (%s)"), SOCKET_API, appFromAnsi(SocketHostError()) );
#else
	strcpy(AnsiHostName, "localhost"); // stijn: mem safety ok
#endif
	HostName = appFromAnsi(AnsiHostName);
	if( Parse(appCmdLine(),TEXT("MULTIHOME="),Home) )
	{
		const TCHAR *A, *B, *C, *D;
		A=*Home;
		if
		(	(A=*Home)!=NULL
		&&	(B=appStrchr(A,'.'))!=NULL
		&&	(C=appStrchr(B+1,'.'))!=NULL
		&&	(D=appStrchr(C+1,'.'))!=NULL )
		{
			IpSetBytes( HostAddr, appAtoi(A), appAtoi(B+1), appAtoi(C+1), \
				appAtoi(D+1) );
		}
		else Out.Logf( TEXT("Invalid multihome IP address %s"), *Home );
	}
	else
	{
		struct addrinfo Hints;
		memset(&Hints, 0, sizeof(struct addrinfo));
		Hints.ai_family = AF_INET;
		Hints.ai_socktype = SOCK_STREAM;
		struct addrinfo* Result = nullptr;
		if (getaddrinfo(AnsiHostName, nullptr, &Hints, &Result) != 0)
		{
			Out.Logf( TEXT("getaddrinfo failed (%s)"), appFromAnsi(SocketHostError()) );
		}
		else if (Result->ai_family!=AF_INET)
		{
			Out.Logf( TEXT("getaddrinfo: non-Internet address (%s)"), appFromAnsi(SocketHostError()) );
		}
		else
		{
			HostAddr = ((sockaddr_in*)Result->ai_addr)->sin_addr;
			freeaddrinfo(Result);

			if( !ParseParam(appCmdLine(),TEXT("PRIMARYNET")) )
				CanBindAll = 1;

			static UBOOL First=0;
			if( !First )
			{
				First = 1;
				debugf( NAME_Init, TEXT("%s: I am %s (%s)"), SOCKET_API, *HostName, *IpString( HostAddr ) );
			}
		}
	}
	return CanBindAll;
	unguard;
}

//
// Get local IP to bind to
//
inline in_addr getlocalbindaddr( FOutputDevice& Out )
{
	guard(getlocalbindaddr);

	in_addr BindAddr;
	
	// If we can bind to all addresses, return 0.0.0.0
	if( getlocalhostaddr( Out, BindAddr ) )
		IpSetInt( BindAddr, INADDR_ANY );	
	
	return BindAddr;
	unguard;
}
/*-----------------------------------------------------------------------------
	Public includes.
-----------------------------------------------------------------------------*/

#include "IpDrvClasses.h"

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
