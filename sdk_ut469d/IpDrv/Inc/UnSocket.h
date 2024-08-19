/*============================================================================
	UnSocket.h: Common interface for WinSock and BSD sockets.

	Revision history:
		* Created by Mike Danylchuk
============================================================================*/

/*-----------------------------------------------------------------------------
	Definitions.
-----------------------------------------------------------------------------*/

#if __WINSOCK__
	typedef INT					SOCKLEN;
	#define GCC_OPT_INT_CAST
    #ifndef MSG_NOSIGNAL
	#define MSG_NOSIGNAL 		0
	#endif
	#ifndef INVALID_SOCKET
	#define INVALID_SOCKET		0
	#endif
	#ifndef SOCKET_ERROR
	#define SOCKET_ERROR		0
	#endif
	#define UDP_ERR_PORT_UNREACH WSAECONNRESET
	#define UDP_ERR_WOULD_BLOCK WSAEWOULDBLOCK
#endif

// Provide WinSock definitions for BSD sockets.
#if __BSD_SOCKETS__
	#if __BSD_SOCKETS_TRUE_BSD__ && !MACOSX
		typedef int socklen_t;
	#endif

	typedef int					SOCKET;
	typedef struct hostent		HOSTENT;
	typedef in_addr				IN_ADDR;
	typedef struct sockaddr		SOCKADDR;
	typedef struct sockaddr_in	SOCKADDR_IN;
	typedef struct linger		LINGER;
	typedef struct timeval		TIMEVAL;
	typedef TCHAR*				LPSTR;
	typedef socklen_t			SOCKLEN;

	#define INVALID_SOCKET		-1
	#define SOCKET_ERROR		-1
	#define WSAEWOULDBLOCK		EWOULDBLOCK
	#define WSAENOTSOCK			ENOTSOCK
	#define WSATRY_AGAIN		TRY_AGAIN
	#define WSAHOST_NOT_FOUND	HOST_NOT_FOUND
	#define WSANO_DATA			NO_ADDRESS
	#define LPSOCKADDR			sockaddr*

	#define closesocket			close
	#define ioctlsocket			ioctl
	#define WSAGetLastError()	errno

	#define GCC_OPT_INT_CAST	(DWORD*)

	#define UDP_ERR_PORT_UNREACH ECONNREFUSED
	#define UDP_ERR_WOULD_BLOCK EAGAIN

	#if __BSD_SOCKETS_TRUE_BSD__
		#define WSAGetLastHostError()	errno
	#else
		#define WSAGetLastHostError()	h_errno
	#endif
#endif

// IP address macros.
#if __WINSOCK__
	#define IP(sin_addr,n) sin_addr.S_un.S_un_b.s_b##n
#elif __BSD_SOCKETS__
	#define IP(sin_addr,n) ((BYTE*)&sin_addr.s_addr)[n-1]
#endif

/*----------------------------------------------------------------------------
	Functions.
----------------------------------------------------------------------------*/

UBOOL InitSockets( FString& Error );
UBOOL SetNonBlocking( INT Socket );
UBOOL SetSocketReuseAddr( INT Socket, UBOOL ReUse=1 );
UBOOL SetSocketLinger( INT Socket );
const char* SocketError( INT Code=-1 );
const char* SocketHostError( INT Code=-1 );
UBOOL IpMatches( sockaddr_in& A, sockaddr_in& B );
UBOOL IpAddressMatches( sockaddr_in& A, sockaddr_in& B ); //elmuerte: jack_porter security fix
void IpGetBytes( in_addr Addr, BYTE& Ip1, BYTE& Ip2, BYTE& Ip3, BYTE& Ip4 );
void IpSetBytes( in_addr& Addr, BYTE Ip1, BYTE Ip2, BYTE Ip3, BYTE Ip4 );
void IpGetInt( in_addr Addr, DWORD& Ip );
void IpSetInt( in_addr& Addr, DWORD Ip );
FString IpString( in_addr Addr, INT Port=0 );

/*----------------------------------------------------------------------------
	The End.
----------------------------------------------------------------------------*/
