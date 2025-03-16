/*============================================================================
	UnUnix.h: Unix system-specific declarations.

	Revision history:
		* Created by Mike Danylchuk
		* Modified for Unreal Engine standards compliance by Brandon Reinhart
============================================================================*/

// Pathnames.
class FString;
#define PATH(s) appUnixPath( s )
FString appUnixPath( const FString& Path );

// Strings
int stricmp( const char* s, const char* t );
int strnicmp( const char* s, const char* t, int n );
char* strupr( char* s );

// Networking.
unsigned long appGetLocalIP( void );

// Globally Unique Identifiers.
void appGetGUID( void* GUID );

// Signal Handling
void HandleSignal( int Signal );
void HandleInterrupt( int Signal );

/*----------------------------------------------------------------------------
	The End.
----------------------------------------------------------------------------*/

