//=============================================================================
// UBrowserServerListFactory
//		An abstract class to add servers to an existing server list.  
//		eg GameSpy, HTTP master servers, favorites list, etc
//=============================================================================
class UBrowserServerListFactory extends UWindowList
	abstract;

var UBrowserServerList PingedList;
var UBrowserServerList UnpingedList;
var UBrowserServerList Owner;


var bool bIncrementalPing;		// Servers are pinged as they come in

function Query(optional bool bBySuperset, optional bool bInitial)
{
}

function Shutdown(optional bool bBySuperset)
{
	Owner = None;
	PingedList = None;
	UnpingedList = None;
}

function QueryFinished(bool bSuccess, optional string ErrorMsg)
{
	Owner.QueryFinished(Self, bSuccess, ErrorMsg);
}

function UBrowserServerList FoundServer(string IP, int QueryPort, string Category, string GameName, optional string HostName)
{
	local UBrowserServerList NewListEntry;
	local bool bInstantPing;

	bInstantPing = Owner.Owner != None && Owner.Owner.UnpingedList == Owner;
	NewListEntry = Owner.FindExistingServer(IP, QueryPort);
	
	if (NewListEntry == None && bInstantPing && Owner.Owner.PingedList != None)
		NewListEntry = Owner.Owner.PingedList.FindExistingServer(IP, QueryPort);
		
	if (NewListEntry == None && bInstantPing && Owner.Owner.HiddenList != None)
		NewListEntry = Owner.Owner.HiddenList.FindExistingServer(IP, QueryPort);

	// Don't add if it's already in the existing list
	if(NewListEntry == None)
	{
		// Add it to the server list(s)
		NewListEntry = UBrowserServerList(Owner.CreateItem(Owner.Class));

		NewListEntry.IP = IP;
		NewListEntry.QueryPort = QueryPort;

		NewListEntry.Ping = 9999;
		if(HostName != "")
			NewListEntry.HostName = HostName;
		else
			NewListEntry.HostName = IP;
		NewListEntry.Category = Category;
		NewListEntry.GameName = GameName;
		NewListEntry.bLocalServer = False;

		Owner.AppendItem(NewListEntry);
		
		if (bInstantPing)
		{
			NewListEntry.PingServer(True, True, Owner.Owner.bNoSort);
			Owner.bPinged = true;
		}
	}

	NewListEntry.bOldServer = False;

	return NewListEntry;
}

function PlayerPawn GetPlayerOwner()
{
	return Owner.GetPlayerOwner();
}
