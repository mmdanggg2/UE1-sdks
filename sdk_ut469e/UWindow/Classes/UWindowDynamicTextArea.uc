class UWindowDynamicTextArea expands UWindowDialogControl;

var config int MaxLines;

var bool bTopCentric;
var float DefaultTextHeight;
var bool bScrollOnResize;
var bool bVCenter;
var bool bHCenter;
var bool bAutoScrollbar;
var bool bVariableRowHeight;	// Assumes !bTopCentric, !bScrollOnResize
var float WrapWidth;

// private
var UWindowDynamicTextRow List;
var UWindowVScrollBar VertSB;
var float OldW, OldH;
var bool bDirty;
var int Count;
var int VisibleRows;
var int Font;
var Font AbsoluteFont;
var color TextColor;
var class<UWindowDynamicTextRow> RowClass;

// selection
var bool bSelect;
var string SelectedText;

var float ClickX, ClickY;
var float StartX, StartY;
var float EndX, EndY;

var() config color SelectionColor;

function Created()
{
	Super.Created();

	VertSB = UWindowVScrollbar(CreateWindow(class'UWindowVScrollbar', WinWidth-12, 0, 12, WinHeight));
	VertSB.bAlwaysOnTop = True;

	Clear();
}

function Clear()
{
	bDirty = True;

	if(List != None)
	{
		if(List.Next == None)
			return;
		List.DestroyList();
	}

	List = new RowClass;
	List.SetupSentinel();
}

function SetAbsoluteFont(Font F)
{
	AbsoluteFont = F;
}

function SetFont(int F)
{
	Font = F;
}

function SetTextColor(Color C)
{
	TextColor = C;
}

function TextAreaClipText2(Canvas C, float DrawX, float DrawY, coerce string S, optional bool bCheckHotkey)
{
	ClipText(C, DrawX, DrawY, S, bCheckHotkey);	
}

function TextAreaTextSize(Canvas C, string Text, out float W, out float H)
{
	TextSize(C, Text, W, H);
}

function BeforePaint( Canvas C, float X, float Y )
{
	Super.BeforePaint(C, X, Y);

	VertSB.WinTop = 0;
	VertSB.WinHeight = WinHeight;
	VertSB.WinWidth = LookAndFeel.Size_ScrollbarWidth;
	VertSB.WinLeft = WinWidth - LookAndFeel.Size_ScrollbarWidth;
}

function Paint2( Canvas C, float MouseX, float MouseY )
{
	local UWindowDynamicTextRow L;
	local int SkipCount, DrawCount;
	local int i;
	local float Y, Junk;
	local bool bWrapped;

	C.DrawColor = TextColor;

	if(AbsoluteFont != None)
		C.Font = AbsoluteFont;
	else
		C.Font = Root.Fonts[Font];

	if(OldW != WinWidth || OldH != WinHeight)
	{
		WordWrap(C, True);
		OldW = WinWidth;
		OldH = WinHeight;
		bWrapped = True;
	}
	else
	if(bDirty)
	{
		WordWrap(C, False);
		bWrapped = True;
	}

	if(bWrapped)
	{
		TextAreaTextSize(C, "A", Junk, DefaultTextHeight);
		VisibleRows = WinHeight / DefaultTextHeight;
		Count = List.Count();
		VertSB.SetRange(0, Count, VisibleRows);

		if(bScrollOnResize)
		{
			if(bTopCentric)
				VertSB.Pos = 0;
			else
				VertSB.Pos = VertSB.MaxPos;
		}

		if(bAutoScrollbar && !bVariableRowHeight)
		{
			if(Count <= VisibleRows)
				VertSB.HideWindow();
			else
				VertSB.ShowWindow();
		}
	}

	if(bTopCentric)
	{
		SkipCount = VertSB.Pos;
		L = UWindowDynamicTextRow(List.Next);
		for(i=0; i < SkipCount && (L != None) ; i++)
			L = UWindowDynamicTextRow(L.Next);

		if(bVCenter && Count <= VisibleRows)
			Y = int((WinHeight - (Count * DefaultTextHeight)) / 2);
		else
			Y = 1;

		DrawCount = 0;
		while(Y < WinHeight)
		{
			DrawCount++;
			if(L != None)
			{
				Y += DrawTextLine(C, L, Y);
				L = UWindowDynamicTextRow(L.Next);
			}
			else
				Y += DefaultTextHeight;
		}

		if(bVariableRowHeight)
		{
			VisibleRows = DrawCount - 1;

			while(VertSB.Pos + VisibleRows > Count)
				VisibleRows--;

			VertSB.SetRange(0, Count, VisibleRows);

			if(bAutoScrollbar)
			{
				if(Count <= VisibleRows)
					VertSB.HideWindow();
				else
					VertSB.ShowWindow();
			}
		}
	}
	else
	{
		SkipCount = Max(0, Count - (VisibleRows + VertSB.Pos));
		L = UWindowDynamicTextRow(List.Last);
		for(i=0; i < SkipCount && (L != List) ; i++)
			L = UWindowDynamicTextRow(L.Prev);

		Y = WinHeight - DefaultTextHeight;
		while(L != List && L != None && Y > -DefaultTextHeight)
		{
			DrawTextLine(C, L, Y);
			Y = Y - DefaultTextHeight;
			L = UWindowDynamicTextRow(L.Prev);
		}
	}
}

function UWindowDynamicTextRow AddText(string NewLine)
{
	local UWindowDynamicTextRow L;
	local string Temp;
	local int i;

	bDirty = True;
	
	i = InStr(NewLine, "\\n");
	if(i != -1)
	{
		Temp = Mid(NewLine, i+2);
		NewLine = Left(NewLine, i);		
	}
	else
		Temp = "";
	

	// reuse a row if possible
	L = CheckMaxRows();

	if(L != None)
		List.AppendItem(L);
	else
		L = UWindowDynamicTextRow(List.Append(RowClass));

	L.Text = NewLine;
	L.WrapParent = None;
	L.bRowDirty = True;

	if(Temp != "")
		AddText(Temp);

	return L;
}

function UWindowDynamicTextRow CheckMaxRows()
{
	local UWindowDynamicTextRow L;
	L = None;
	while(MaxLines > 0 && List.Count() > MaxLines - 1 && List.Next != None)
	{
		L = UWindowDynamicTextRow(List.Next);
		RemoveWrap(L);
		L.Remove();
	}
	return L;
}

function WordWrap(Canvas C, bool bForce)
{
	local UWindowDynamicTextRow L;

	for(L = UWindowDynamicTextRow(List.Next); L != None; L = UWindowDynamicTextRow(L.Next))
		if(L.WrapParent == None && (L.bRowDirty || bForce))
			WrapRow(C, L);

	bDirty = False;
}

function WrapRow(Canvas C, UWindowDynamicTextRow L)
{
	local UWindowDynamicTextRow CurrentRow, N;
	local float MaxWidth;
	local int WrapPos;

	if(WrapWidth == 0)
	{
		if(VertSB.bWindowVisible || bAutoScrollbar)
			MaxWidth = WinWidth - VertSB.WinWidth;
		else
			MaxWidth = WinWidth;
	}
	else
		MaxWidth = WrapWidth;

	L.bRowDirty = False;

	// fast check - single line?
	N = UWindowDynamicTextRow(L.Next);
	if(N == None || N.WrapParent != L)
	{
		if(GetWrapPos(C, L, MaxWidth) == -1)
			return;
	}

	RemoveWrap(L);
	CurrentRow = L;

	while(True)
	{
		WrapPos = GetWrapPos(C, CurrentRow, MaxWidth);
		if(WrapPos == -1)
			break;

		CurrentRow = SplitRowAt(CurrentRow, WrapPos);
	}
}

///////////////////////////////////////////////////////
// Functions to override to change format/layout
///////////////////////////////////////////////////////

function float DrawTextLine(Canvas C, UWindowDynamicTextRow L, float Y)
{
	local float X, W, H;

	if(bHCenter)
	{
		TextAreaTextSize(C, L.Text, W, H);
		if(VertSB.bWindowVisible)
			X = int(((WinWidth - VertSB.WinWidth) - W) / 2);
		else
			X = int((WinWidth - W) / 2);
	}
	else
		X = 2;

	X = int(X * Root.GUIScale + 0.5) / Root.GUIScale;

	TextAreaClipText(C, X, Y, L.Text);

	return DefaultTextHeight;
}


// find where to break the line
function int GetWrapPos(Canvas C, UWindowDynamicTextRow L, float MaxWidth)
{
	local float W, H, LineWidth, NextWordWidth;
	local string Input, NextWord;
	local int WordsThisRow, WrapPos;

	// quick check
	TextAreaTextSize(C, L.Text, W, H);
	if(W <= MaxWidth)
		return -1;

	Input = L.Text;
	WordsThisRow = 0;
	LineWidth = 0;
	WrapPos = 0;
	NextWord = "";

	while(Input != "" || NextWord != "")
	{
		if(NextWord == "")
		{
			RemoveNextWord(Input, NextWord);
			TextAreaTextSize(C, NextWord, NextWordWidth, H);
		}
		if(WordsThisRow > 0 && LineWidth + NextWordWidth > MaxWidth)
		{
			return WrapPos;
		}
		else
		{
			WrapPos += Len(NextWord);
			LineWidth += NextWordWidth;
			NextWord = "";
			WordsThisRow++;
		}
	}
	return -1;
}

function UWindowDynamicTextRow SplitRowAt(UWindowDynamicTextRow L, int SplitPos)
{
	local UWindowDynamicTextRow N;

	N = UWindowDynamicTextRow(L.InsertAfter(RowClass));

	if(L.WrapParent == None)
		N.WrapParent = L;
	else
		N.WrapParent = L.WrapParent;

	N.Text = Mid(L.Text, SplitPos);
	L.Text = Left(L.Text, SplitPos);

	return N;
}

function RemoveNextWord(out string Text, out string NextWord)
{
	local int i;

	i = InStr(Text, " ");
	if(i == -1)
	{
		NextWord = Text;
		Text = "";
	}
	else
	{
		while(Mid(Text, i, 1) == " ")
			i++;

		NextWord = Left(Text, i);
		Text = Mid(Text, i);
	}
}

function RemoveWrap(UWindowDynamicTextRow L)
{
	local UWindowDynamicTextRow N;

	// Remove previous word-wrapping
	N = UWindowDynamicTextRow(L.Next);
	while(N != None && N.WrapParent == L)
	{
		L.Text = L.Text $ N.Text;
		N.Remove();
		N = UWindowDynamicTextRow(L.Next);
	}
}

function bool MouseWheelDown(float ScrollDelta)
{
	Super.MouseWheelDown(ScrollDelta);
	return true;
}

function bool MouseWheelUp(float ScrollDelta)
{
	Super.MouseWheelUp(ScrollDelta);
	VertSB.Scroll(int(ScrollDelta));
	return true;
}

// selection
function LMouseDown(float X, float Y)
{
	Super.LMouseDown(X, Y);
	ClickX = X;
	ClickY = Y;
}

function LMouseUp(float X, float Y)
{
	if (bMouseDown && SelectedText != "")
		GetPlayerOwner().CopyToClipboard(SelectedText);
	Super.LMouseUp(X, Y);
}

function Paint( Canvas C, float MouseX, float MouseY )
{
	SelectedText = "";
	bSelect = bMouseDown;
	if (bSelect)
	{
		if (MouseY < ClickY)
		{
			StartX = MouseX;
			EndX = ClickX;
		}
		else
		{
			StartX = ClickX;
			EndX = MouseX;
		}

		StartY = FMin(MouseY, ClickY);
		EndY = FMax(MouseY, ClickY);
	}
	Paint2(C, MouseX, MouseY);
}

function TextAreaClipText(Canvas C, float DrawX, float DrawY, coerce string S, optional bool bCheckHotkey)
{
	local int X1, X2, XS;
	local color Prev;
	local string Selected;
	local float DrawSelX;
	local float W, H;
	local int i;
	local float DrawBottomY;

	TextAreaClipText2(C, DrawX, DrawY, S, bCheckHotkey);
	
	if (!bSelect)
		return;
	
	DrawBottomY = DrawY + DefaultTextHeight;
	
	if (DrawY > EndY || DrawBottomY <= StartY)
		return;
	
	XS = Len(S);
	X1 = 0;
	X2 = XS;
	if (DrawY <= StartY && StartY < DrawBottomY && StartX > 0)
	{
		if (EndY < DrawBottomY && EndX < StartX)
		{
			H = EndX;
			EndX = StartX;
			StartX = H;
		}
		X1 = XS;
		for (i = 0; i < XS; i++)
		{
			TextAreaTextSize(C, Left(S, i + 1), W, H);
			if (DrawX + W > StartX)
			{
				X1 = i;
				break;
			}
		}
	}
	
	if (DrawY <= EndY && EndY < DrawBottomY)
	{
		if (DrawX > EndX)
			X2 = X1;
		else
		{
			for (i = X1 + 1; i < XS; i++)
			{
				TextAreaTextSize(C, Left(S, i), W, H);
				if (DrawX + W > EndX)
				{
					X2 = i;
					break;
				}
			}
		}
	}
	
	TextAreaTextSize(C, Left(S, X1), DrawSelX, H);
	DrawSelX += DrawX;
	
	Selected = Mid(S, X1, X2 - X1);
	
	if (DrawBottomY < EndY)
		SelectedText = Chr(13) $ Chr(10) $ SelectedText;
	SelectedText = Selected $ SelectedText;
	
	Prev = C.DrawColor;
	C.DrawColor = SelectionColor;
	TextAreaClipText2(C, DrawSelX, DrawY, Selected, bCheckHotkey);
	C.DrawColor = Prev;
}

defaultproperties
{
	Font=0
	WrapWidth=0
	bNoKeyboard=True
	bScrollOnResize=True
	MaxLines=0
	TextColor=(R=255,G=255,B=255);
	bVCenter=False
	bHCenter=False
	bAutoScrollbar=False
	bVariableRowHeight=False
	RowClass=class'UWindowDynamicTextRow'
	SelectionColor=(R=0,G=255,B=0)
}
