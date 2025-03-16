//=============================================================================
// InternetLink: Parent class for Internet connection classes
//=============================================================================
class InternetLink extends InternetInfo
	native
	transient;

//-----------------------------------------------------------------------------
// Types & Variables.

// An IP address.
struct IpAddr
{
	var int Addr;
	var int Port;
};

// Data receive mode.
// Cannot be set in default properties.
var enum ELinkMode
{
	MODE_Text, 
	MODE_Line,
	MODE_Binary
} LinkMode;

// Internal
var	const pointer Socket;
var const int Port;
var	const pointer RemoteSocket;
var private native const pointer PrivateResolveInfo;
var const int DataPending;

// Receive mode.
// If mode is MODE_Manual, received events will not be called.
// This means it is your responsibility to check the DataPending
// var and receive the data.
// Cannot be set in default properties.
var enum EReceiveMode
{
	RMODE_Manual,
	RMODE_Event
} ReceiveMode;

//
// OldUnreal: Text Encoding Mode.
//
// The InternetLink subclasses have several functions that send or receive ANSI text.
// Since we store all strings in some Unicode format internally, all of these functions
// require conversion between ANSI and the internal Unicode format.
//
// Unreal (<227j) and Unreal Tournament (<469c) performed this conversion by mapping
// Unicode code points up to 0xFF directly onto ANSI characters with the same byte
// representation, even if the characters represented by those bytes are not the
// same as the original Unicode character.
//
// This behavior/feature is fine for Unicode code points up to 0x7F, since those code
// points map directly onto (pretty much?) all ANSI code pages, but results in incorrect
// translations for all points between 0x7F-0xFF.
//
// Unfortunately, there seem to be a few mods that rely on the old (incorrect) behavior.
// To accomodate these mods, we use the old/incorrect encoding method
// (TEXTENC_Compatibility) by default.
//
// New mods should, however, set TextEncoding to TEXTENC_Native. In this mode, the engine
// converts unicode code points above 0x7F to UTF-8 characters.
//
var enum ETextEncoding
{
	TEXTENC_Compatibility,
	TEXTENC_Native	
} TextEncoding;

//-----------------------------------------------------------------------------
// Natives.

// Returns true if data is pending on the socket.
native function bool IsDataPending();

// Parses an Unreal URL into its component elements.
// Returns false if the URL was invalid.
native function bool ParseURL
(
	coerce string URL, 
	out string Addr, 
	out int Port, 
	out string LevelName,
	out string EntryName
);

// Resolve a domain or dotted IP.
// Nonblocking operation.  
// Triggers Resolved event if successful.
// Triggers ResolveFailed event if unsuccessful.
native function Resolve( coerce string Domain );

// Returns most recent winsock error.
native function int GetLastError();

// Convert an IP address to a string.
native function string IpAddrToString( IpAddr Arg );

// Convert a string to an IP
native function bool StringToIpAddr( string Str, out IpAddr Addr );

// Validate: Takes a challenge string and returns an encoded validation string.
native function string Validate( string ValidationString, string GameName );

native function GetLocalIP(out IpAddr Arg );

//-----------------------------------------------------------------------------
// Events.

// Called when domain resolution is successful.
// The IpAddr struct Addr contains the valid address.
event Resolved( IpAddr Addr );

// Called when domain resolution fails.
event ResolveFailed();

defaultproperties
{
    TextEncoding=TEXTENC_Compatibility
}
