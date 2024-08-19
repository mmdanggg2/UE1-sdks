//=============================================================================
// UBrowserTypesGrid.
//=============================================================================
class UBrowserTypesGrid expands UWindowGrid;

var() localized string TypeText;
var() localized string CountText;
var() localized string StateText;

var() localized string ShowText;
var() localized string HideText;

var UBrowserTypesMenu Menu;

var UBrowserTypesList Selected;
var int Count;

function Created() 
{
	Super.Created();

	RowHeight = 12;

	AddColumn(TypeText, 115);
	AddColumn(CountText, 20);
	AddColumn(StateText, 30);

	Menu = UBrowserTypesMenu(Root.CreateWindow(class'UBrowserTypesMenu', 0, 0, 100, 100, Self));
	Menu.HideWindow();
}

function PaintColumn(Canvas C, UWindowGridColumn Column, float MouseX, float MouseY) 
{
	local UBrowserTypesList TypesList, l;
	local int Visible;
	local int Skipped;
	local int Y;
	local int TopMargin;
	local int BottomMargin;
	local string Value;
	
	if(bShowHorizSB)
		BottomMargin = LookAndFeel.Size_ScrollbarWidth;
	else
		BottomMargin = 0;

	TopMargin = LookAndFeel.ColumnHeadingHeight;
	
	TypesList = UBrowserServerListWindow(GetParent(class'UBrowserServerListWindow')).TypesList;
	if(TypesList == None)
	{
		Count = 0;
		return;
	}
	else
		Count = TypesList.Count();

	C.Font = Root.Fonts[F_Normal];
	Visible = int((WinHeight - (TopMargin + BottomMargin))/RowHeight);
	
	VertSB.SetRange(0, Count+1, Visible);
	TopRow = VertSB.Pos;

	Skipped = 0;

	Y = 1;
	l = UBrowserTypesList(TypesList.Next);
	while((Y < RowHeight + WinHeight - RowHeight - (TopMargin + BottomMargin)) && (l != None)) 
	{
		if(Skipped >= VertSB.Pos)
		{
			// Draw highlight
			if(l == Selected)
				Column.DrawStretchedTexture( C, 0, Y-1 + TopMargin, Column.WinWidth, RowHeight + 1, Texture'Highlight');

			switch(Column.ColumnNum)
			{
			case 0:
				Column.ClipText( C, 2, Y + TopMargin, l.Type );
				break;
			case 1:
				Column.ClipText( C, 2, Y + TopMargin, l.iCount );
				break;
			case 2:
				Value = HideText;
				if (l.bShow)
					Value = ShowText;
				Column.ClipText( C, 2, Y + TopMargin, Value );
				break;
			}

			Y = Y + RowHeight;			
		} 
		Skipped ++;
		l = UBrowserTypesList(l.Next);
	}
}

function SortColumn(UWindowGridColumn Column) 
{
	UBrowserServerListWindow(GetParent(class'UBrowserServerListWindow')).TypesList.SortByColumn(Column.ColumnNum);
}

function RightClickRow(int Row, float X, float Y)
{
	local float MenuX, MenuY;

	WindowToGlobal(X, Y, MenuX, MenuY);
	Menu.WinLeft = MenuX;
	Menu.WinTop = MenuY;
	Menu.Grid = Self;
	Menu.ShowWindow();
}

function SelectRow(int Row)
{
	local UBrowserTypesList S;

	S = GetTypeUnderRow(Row);

	if(S != None)
		Selected = S;
}

function DoubleClickRow(int Row)
{
	local UBrowserTypesList TypesList;

	TypesList = GetTypeUnderRow(Row);
	if(TypesList == None)
		return;

	TypesList.bShow = !TypesList.bShow;
	UBrowserServerListWindow(GetParent(class'UBrowserServerListWindow')).SaveFilters();
}

function UBrowserTypesList GetTypeUnderRow(int Row)
{
	local UBrowserTypesList TypesList, l;
	local int i;

	TypesList = UBrowserServerListWindow(GetParent(class'UBrowserServerListWindow')).TypesList;
	if(TypesList == None)
		return None;

	l = UBrowserTypesList(TypesList.Next);
	while (l != None)
	{
		if (i++ == Row)
		{
			return l;
		}
		l = UBrowserTypesList(l.Next);
	}
	return None;
}

function int GetSelectedRow()
{
	local int i;
	local UBrowserTypesList TypesList, l;

	TypesList = UBrowserServerListWindow(GetParent(class'UBrowserServerListWindow')).TypesList;
	if(TypesList == None)
		return -1;

	l = UBrowserTypesList(TypesList.Next);
	while (l != None)
	{
		if (l == Selected)
		{
			return i;
		}
		i++;
		l = UBrowserTypesList(l.Next);
	}
	return -1;
}

function KeyDown(int Key, float X, float Y) 
{
	local int i;

	switch(Key)
	{
	case 0x26: // IK_Up
		SelectRow(Clamp(GetSelectedRow() - 1, 0, Count - 1));
		VertSB.Show(GetSelectedRow());
		break;
	case 0x28: // IK_Down
		SelectRow(Clamp(GetSelectedRow() + 1, 0, Count - 1));
		VertSB.Show(GetSelectedRow());
		break;
	case 0x20: // IK_Space:
	case 0x0D: // IK_Enter:
		DoubleClickRow(GetSelectedRow());
		break;
	default:
		Super.KeyDown(Key, X, Y);
		break;
	}
}

function FilterToggle(bool bShowAll)
{
	local UBrowserTypesList TypesList, l;

	TypesList = UBrowserServerListWindow(GetParent(class'UBrowserServerListWindow')).TypesList;
	if(TypesList == None)
		return;

	l = UBrowserTypesList(TypesList.Next);
	while (l != None)
	{
		l.bShow = bShowAll;
		l = UBrowserTypesList(l.Next);
	}
	UBrowserServerListWindow(GetParent(class'UBrowserServerListWindow')).SaveFilters();
}

defaultproperties
{
	TypeText="Game type"
	CountText="Count"
	StateText="State"
	ShowText="show"
	HideText="hide"
	bNoKeyboard=True
}
