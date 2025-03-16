//=============================================================================
// UBrowserTypesList.
//=============================================================================
class UBrowserTypesList expands UWindowList;

var string			Type;
var int				iCount;
var bool			bShow;

// Sentinel only
var int				SortColumn;
var bool			bDescending;

function SortByColumn(int Column)
{
	if(SortColumn == Column)
	{
		bDescending = !bDescending;
	}
	else
	{
		SortColumn = Column;
		bDescending = False;
	}

	Sort();
}

function int Compare(UWindowList T, UWindowList B)
{
	local int Result;
	local UBrowserTypesList TT, TB;

	if(B == None) return -1; 

	TT = UBrowserTypesList(T);
	TB = UBrowserTypesList(B);

	switch(SortColumn)
	{
		case 1:
			if(TT.iCount < TB.iCount)
				Result = -1;
			else
				Result = 1;			
			break;	
		case 2:
			if(TT.bShow)
				Result = -1;
			else
				Result = 1;			
			break;
		default:
			if(TT.Type < TB.Type)
				Result = -1;
			else
				Result = 1;			
			break;
	}

	if(UBrowserTypesList(Sentinel).bDescending)
		Result = -Result;

	return Result;
}

defaultproperties
{
}
