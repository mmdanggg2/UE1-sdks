//=============================================================================
// Logs game events for stat collection
//
// Logs to a file.
//=============================================================================
class StatLogFile extends StatLog
	native;

// OldUnreal File Encoding support
enum EFileEncoding
{
	// Plain ANSI single-byte encoding
	FILE_ENCODING_ANSI,
	// Windows UTF-16LE encoding without a BOM
	FILE_ENCODING_UTF16LE,
	// Windows UTF-16 encoding with a BOM
	FILE_ENCODING_UTF16LE_BOM,
	// UTF-8 encoding without a BOM
	FILE_ENCODING_UTF8,
	// UTF-8 encoding with a BOM
	FILE_ENCODING_UTF8_BOM
};

var EFileEncoding Encoding; // must be set before calling OpenLog
var bool bWatermark;

// Internal
var transient pointer LogAr; // C++ FArchive*.

// Configs
var string StatLogFile;
var string StatLogFinal;

// File Manipulation
native final function OpenLog();
native final function CloseLog();
native final function Watermark( string EventString );
native final function GetChecksum( out string Checksum );
native final function FileFlush();
native final function FileLog( string EventString );

// Logging.
function StartLog()
{
	local string FileName;
	local string AbsoluteTime;

	SaveConfig();

	AbsoluteTime = GetShortAbsoluteTime();
	if (!bWorld)
	{
	    if (LocalLogDir == "")
		    LocalLogDir = class'StatLogFile'.default.LocalLogDir;
			
		FileName = LocalLogDir$"/"$GameName$"."$LocalStandard$"."$AbsoluteTime$"."$Level.Game.GetServerPort();
		StatLogFile = FileName$".tmp";
		StatLogFinal = FileName$".log";
	}
	else
	{
	    if (WorldLogDir == "")
		    WorldLogDir = class'StatLogFile'.default.WorldLogDir;

		FileName = WorldLogDir$"/"$GameName$"."$WorldStandard$"."$AbsoluteTime$"."$Level.Game.GetServerPort();
		StatLogFile = FileName$".tmp";
		StatLogFinal = FileName$".log";
		bWatermark = True;
	}

	OpenLog();
}

function StopLog()
{
	FlushLog();
	CloseLog();
}

function FlushLog()
{
	FileFlush();
}

function LogEventString( string EventString )
{
	if( bWatermark )
		Watermark( EventString );
	FileLog( EventString );
	FlushLog();
}

// Return a logfile name if relevant.
function string GetLogFileName()
{
	return StatLogFinal;
}

function LogPlayerConnect(Pawn Player, optional string Checksum)
{
	if( bWorld )
	{
		if( Player.PlayerReplicationInfo.bIsABot )
			Checksum = "IsABot";
		if (Player.IsA('PlayerPawn'))
			LogEventString( GetTimeStamp()$Chr(9)$"player"$Chr(9)$"Connect"$Chr(9)$Player.PlayerReplicationInfo.PlayerName$Chr(9)$Player.PlayerReplicationInfo.PlayerID$Chr(9)$PlayerPawn(Player).bAdmin$Chr(9)$Checksum );
		else
			LogEventString( GetTimeStamp()$Chr(9)$"player"$Chr(9)$"Connect"$Chr(9)$Player.PlayerReplicationInfo.PlayerName$Chr(9)$Player.PlayerReplicationInfo.PlayerID$Chr(9)$False$Chr(9)$Checksum );
		LogPlayerInfo( Player );
	}
	else Super.LogPlayerConnect( Player, Checksum );
}

function LogGameEnd( string Reason )
{
	local string Checksum;

	if( bWorld )
	{
		bWatermark = False;
		GetChecksum( Checksum );
		LogEventString(GetTimeStamp()$Chr(9)$"game_end"$Chr(9)$Reason$Chr(9)$Checksum$"");
	}
	else Super.LogGameEnd(Reason);
}

// OldUnreal: use the code below if you want to override the encoding without
// breaking compatibility with earlier versions of StatLogFile

// function SetEncoding()
// {
//     local int EngineVersion;
//     local string EngineRevision;
//
//     EngineVersion = int(Level.EngineVersion);
//     if (EngineVersion >= 469)
//     {
//         EngineRevision = Level.GetPropertyText("EngineRevision");
//         EngineRevision = Left(EngineRevision, InStr(EngineRevision, " "));
//
//         if (Len(EngineRevision) > 0 && EngineRevision != "a" && EngineRevision != "b")
//         {
//             SetPropertyText("Encoding", "FILE_ENCODING_UTF8_BOM");
//             Log("Set encoding to UTF-8");
//         }
//     }
// }

defaultproperties
{
	Encoding=FILE_ENCODING_UTF16LE
	StatLogFile="../Logs/unreal.ngStats.Unknown.log"
}
