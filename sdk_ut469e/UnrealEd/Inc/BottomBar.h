/*=============================================================================
	BottomBar : Class for handling the controls on the bottom bar
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>

//#define IDPB_MODE_CAMERA		19000

#define TTS_BALLOON             0x40
#define TTF_TRACK               0x0020
#define TTM_TRACKACTIVATE       (WM_USER + 17)  // wParam = TRUE/FALSE start end  lparam = LPTOOLINFO
#define TTM_TRACKPOSITION       (WM_USER + 18)  // lParam = dwPos

TOOLINFO* tiptr;

VOID CALLBACK TimerProc( 
    HWND hwnd,        // handle to window for timer messages 
    UINT message,     // WM_TIMER message 
    UINT_PTR idTimer,     // timer identifier 
    DWORD dwTime)     // current system time 
{
	KillTimer(hwnd, idTimer);
	SendMessage(hwnd, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)tiptr);
}

class FBallonHandler : public FOutputDevice
{
private:
	WWindow* Control;
	FOutputDevice* Base;
	HWND hwndToolTip;
	TOOLINFO ti{};
public:
	FBallonHandler(WWindow* InControl)
		: Control(InControl)
	{
		Base = GLog;
		GLog = this;

		hwndToolTip = CreateWindow(TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_BALLOON, 
			0, 0, 0, 0, NULL, NULL, hInstance, NULL);

		if (hwndToolTip)
		{
			ti.cbSize   = sizeof(ti);
			ti.uFlags   = TTF_IDISHWND | TTF_TRACK;
			ti.hwnd     = Control->hWnd;
			ti.uId      = (UINT_PTR)Control->hWnd;
			ti.hinst    = hInstance;

			GetClientRect(Control->hWnd, &ti.rect);

			SendMessage(hwndToolTip, TTM_ADDTOOL, 0, (LPARAM) &ti);
		}
	}
	~FBallonHandler()
	{
		GLog = Base;
	}
	void Serialize( const TCHAR* Data, enum EName Event )
	{
		Base->Serialize(Data, Event);
		if (Event == NAME_FriendlyError && hwndToolTip)
		{
			FString Text = Data;
			ti.lpszText = &Text[0];
			SendMessage(hwndToolTip, TTM_UPDATETIPTEXT, 0, (LPARAM) &ti);
			POINT pt = {MulDiv(11, WWindow::DPIX, 96), 0};
			ClientToScreen(Control->hWnd, &pt);
			SendMessage(hwndToolTip, TTM_TRACKPOSITION, 0, MAKELPARAM(pt.x,pt.y));
			SendMessage(hwndToolTip, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&ti);
			tiptr = &ti;

			SetTimer(hwndToolTip, 4, 10*1000, &TimerProc);
		}
	}
};

// --------------------------------------------------------------
//
// WBottomBar
//
// --------------------------------------------------------------

#define IDSC_LOG_COMMAND		19500
#define IDCB_LOG_COMMAND		19501
#define IDPB_LOG_WND			19502
#define IDCK_LOCK				19503
#define IDCK_SNAP_VERTEX		19504
#define IDCK_TOGGLE_GRID		19505
#define IDCB_GRID_SIZE			19506
#define IDCK_TOGGLE_ROT_GRID	19507
#define IDCB_ROT_GRID_SIZE		19508
#define IDCK_ZOOMCENTER_ALL		19509
#define IDCK_ZOOMCENTER			19510
#define IDCK_MAXIMIZE			19511
#define IDCK_TOGGLE_SOUND		19512
#define IDSC_BOX_SEL_INFO		19513

#define dSPACE_BETWEEN_GROUPS 16
extern HWND GhwndEditorFrame;

struct {
	int ID;
	int Width;
	int Height;
	int Pad;		// Amount of space to leave between this control and the one to it's right
	TCHAR ToolTip[64];
}
GWBB_WndPos[] =
{
	IDSC_LOG_COMMAND,		70, 	16,		4,			TEXT("Enter a Log Command and Press ENTER"),
	IDCB_LOG_COMMAND,		384,	22,		4,			TEXT("Enter a Log Command and Press ENTER"),
	IDPB_LOG_WND,			22,		20,		dSPACE_BETWEEN_GROUPS,	TEXT("Show Full Log Window"),
	IDCK_LOCK,				22,		20,		0,			TEXT("Toggle Texture Lock"),
	IDCK_SNAP_VERTEX,		22,		20,		dSPACE_BETWEEN_GROUPS,	TEXT("Toggle Vertex Snap"),
	IDCK_TOGGLE_GRID,		22,		20,		4,			TEXT("Toggle Snap Grid"),
	IDCB_GRID_SIZE,			48,		22,		dSPACE_BETWEEN_GROUPS,	TEXT("Snap Grid Size"),
	IDCK_TOGGLE_ROT_GRID,	22,		20,		dSPACE_BETWEEN_GROUPS,	TEXT("Toggle Rotation Grid"),
	//IDCB_ROT_GRID_SIZE,		64,		22,	dSPACE_BETWEEN_GROUPS,	TEXT("Rotation Grid Size"),
	//IDCK_ZOOMCENTER_ALL,	22,		20,		0,			TEXT("Center All Viewports on Selection"),
	//IDCK_ZOOMCENTER,		22,		20,		0,			TEXT("Center Viewport on Selection"),
	IDCK_TOGGLE_SOUND,		22,		20,		dSPACE_BETWEEN_GROUPS,	TEXT("Toggle Sound"),
	IDCK_MAXIMIZE,			22,		20,		dSPACE_BETWEEN_GROUPS,	TEXT("Maximize Viewport"),
	IDSC_BOX_SEL_INFO,		135,	20,		0,			TEXT("Selection Box Size"),
	-1, -1, -1, -1, TEXT("")
};

WNDPROC lpfnEditWndProc = NULL; // original wndproc for the combo box 
HWND GHwndBar = NULL;
FString GCommand;
LRESULT CALLBACK LogCommandEdit_Proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	try
	{
		switch (msg)
		{
		case WM_KEYDOWN:
			switch (wParam)
			{
				case VK_RETURN:
				{
					TCHAR Text[4096];
					::GetWindowTextW(hwnd, Text, ARRAY_COUNT(Text));
					GCommand = Text;
					GEditor->Exec(*GCommand);
					PostMessageW(GHwndBar, WM_COMMAND, WM_ADD_MRU, 0);
					return 0;
				}
			}
			break;
		}

		// Call the original window procedure for default processing.
		return CallWindowProcA(lpfnEditWndProc, hwnd, msg, wParam, lParam);
	}
	catch(...)
	{
		
	}
	return 0;
}

class WBottomBar : public WWindow
{
	DECLARE_WINDOWCLASS(WBottomBar,WWindow,Window)

	WToolTip* ToolTipCtrl{};

	WCustomLabel *LogCommandLabel{};
	WMRUComboBox *LogCommandCombo{};
	WComboBox *DragGridSizeCombo{};
	WPictureButton *TextureLockCheck{}, *SnapVertexCheck{}, *DragGridCheck{}, *RotGridCheck{}, *SoundCheck{}, *MaximizeCheck{};
	WButton *LogWndButton{};
	HBRUSH hbrDark;
	WLabel *BoxSelInfo{};

	TArray<WPictureButton> PictureButtons;
	TArray<WButton> Buttons;
	HBITMAP hbm, hbmLogWnd;
	BITMAP bm{};

	HWND hwndEdit{};

	FBallonHandler* BalloonHandler{};

	// Structors.
	WBottomBar( FName InPersistentName, WWindow* InOwnerWindow )
	:	WWindow( InPersistentName, InOwnerWindow )
	{
		//hbmLogWnd = NULL;
		hbm = (HBITMAP)LoadImageW( hInstance, MAKEINTRESOURCEW(IDB_BOTTOM_BAR), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR );	check(hbm);
		hbmLogWnd = (HBITMAP)LoadImageW( hInstance, MAKEINTRESOURCEW(IDB_BB_LOG_WND), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR );	check(hbmLogWnd);

		ScaleImageAndReplace(hbm);
		ScaleImageAndReplace(hbmLogWnd);

		GetObjectA( hbm, sizeof(BITMAP), (LPSTR)&bm );
		hbrDark = CreateSolidBrush(RGB(128,128,128));
	}
	void OpenWindow()
	{
		guard(WBottomBar::OpenWindow);
		MdiChild = 0;

		PerformCreateWindowEx
		(
			0,
			NULL,
			WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
			0,
			0,
			MulDiv(320, DPIX, 96),
			MulDiv(200, DPIY, 96),
			OwnerWindow ? OwnerWindow->hWnd : NULL,
			NULL,
			hInstance
		);
		unguard;
	}
	void OnDestroy()
	{
		guard(WBottomBar::OnDestroy);

		for( int x = 0 ; x < PictureButtons.Num() ; x++ )
			DestroyWindow( PictureButtons(x).hWnd );
		PictureButtons.Empty();
		for( int x = 0 ; x < Buttons.Num() ; x++ )
			DestroyWindow( Buttons(x).hWnd );
		Buttons.Empty();
		DeleteObject(hbm);
		DeleteObject(hbmLogWnd);
		DeleteObject(hbrDark);

		delete LogCommandLabel;
		delete LogCommandCombo;
		delete DragGridSizeCombo;
		delete BoxSelInfo;

		delete ToolTipCtrl;

		delete BalloonHandler;

		WWindow::OnDestroy();
		unguard;
	}
	void OnCreate()
	{
		guard(WBottomBar::OnCreate);
		WWindow::OnCreate();
		
		SetRedraw(false);

		GHwndBar = hWnd;

		ToolTipCtrl = new WToolTip(this);
		ToolTipCtrl->OpenWindow();

		// Create the child controls.  We also set their initial positions here, aligned to
		// the left hand edge.  When this window resizes itself, all controls will be shifted to
		// be aligned to the right edge, leaving room on the left for a text status line.

		RECT rc;
		::GetClientRect( hWnd, &rc );

		// stijn: apply DPI scaling to command label and combo box
#define ScaleX(x) MulDiv((x), DPIX, 96)
#define ScaleY(y) MulDiv((y), DPIY, 96)
		LogCommandLabel = new WCustomLabel( this, IDSC_LOG_COMMAND );
		LogCommandLabel->OpenWindow( 1, 0, 0 );
		LogCommandLabel->MoveWindow( 0, 0, ScaleX(32), ScaleY(32), 1);
		LogCommandLabel->SetText( TEXT("Command:") );

		LogCommandCombo = new WMRUComboBox( this, TEXT("History"), TEXT("Exec%d"), 16, IDCB_LOG_COMMAND );
		LogCommandCombo->OpenWindow( 1, 0, CBS_DROPDOWN );
		LogCommandCombo->MoveWindow( 0, 0, ScaleX(32), ScaleY(32), 1);
		//LogCommandCombo->SelectionChangeDelegate = FDelegate(this,(TDelegate)&WBottomBar::OnLogCommandSelChange);

		// We subclass the edit control inside of the combobox so we can catch events that
		// are normally invisible to us ... like the user pressing ENTER.
		hwndEdit = GetWindow( LogCommandCombo->hWnd, GW_CHILD );
		check(hwndEdit);
		lpfnEditWndProc = (WNDPROC)SetWindowLongPtr(hwndEdit, GWLP_WNDPROC, (LONG_PTR)LogCommandEdit_Proc);
		check(lpfnEditWndProc);

		new(Buttons)WButton( this, IDPB_LOG_WND );
		LogWndButton = &(Buttons(Buttons.Num() - 1));	check(LogWndButton);
		LogWndButton->OpenWindow( 1, 0, 0, ScaleX(22), ScaleY(20), NULL, 0, BS_OWNERDRAW );
		LogWndButton->SetBitmap( hbmLogWnd );

		BalloonHandler = new FBallonHandler(LogWndButton);

		new(PictureButtons)WPictureButton( this );
		TextureLockCheck = &(PictureButtons(PictureButtons.Num() - 1));	check(TextureLockCheck);
		TextureLockCheck->SetUp( TEXT(""), IDCK_LOCK, 
			0, 0, ScaleX(22), ScaleY(20),
			hbm, ScaleX(22), 0, ScaleX(22), ScaleY(20),
			hbm, ScaleX(22), ScaleY(20), ScaleX(22), ScaleY(20) );
		TextureLockCheck->OpenWindow();
		TextureLockCheck->OnSize( SIZE_MAXSHOW, ScaleX(22), ScaleY(20) );
		
		new(PictureButtons)WPictureButton( this );
		SnapVertexCheck = &(PictureButtons(PictureButtons.Num() - 1));	check(SnapVertexCheck);
		SnapVertexCheck->SetUp( TEXT(""), IDCK_SNAP_VERTEX, 
			0, 0, ScaleX(22), ScaleY(20),
			hbm, ScaleX(44), 0, ScaleX(22), ScaleY(20),
			hbm, ScaleX(44), ScaleY(20), ScaleX(22), ScaleY(20) );
		SnapVertexCheck->OpenWindow();
		SnapVertexCheck->OnSize( SIZE_MAXSHOW, ScaleX(22), ScaleY(20) );

		new(PictureButtons)WPictureButton( this );
		DragGridCheck = &(PictureButtons(PictureButtons.Num() - 1));	check(DragGridCheck);
		DragGridCheck->SetUp( TEXT(""), IDCK_TOGGLE_GRID, 
			0, 0, ScaleX(22), ScaleY(20),
			hbm, ScaleX(66), 0, ScaleX(22), ScaleY(20),
			hbm, ScaleX(66), ScaleY(20), ScaleX(22), ScaleY(20) );
		DragGridCheck->OpenWindow();
		DragGridCheck->OnSize( SIZE_MAXSHOW, ScaleX(22), ScaleY(20) );

		DragGridSizeCombo = new WComboBox( this, IDCB_GRID_SIZE );
		DragGridSizeCombo->OpenWindow( 1, 0 );
		DragGridSizeCombo->MoveWindow( 0, 0, 1, 1, 1);
		DragGridSizeCombo->AddString( TEXT("1") );
		DragGridSizeCombo->AddString( TEXT("2") );
		DragGridSizeCombo->AddString( TEXT("4") );
		DragGridSizeCombo->AddString( TEXT("8") );
		DragGridSizeCombo->AddString( TEXT("16") );
		DragGridSizeCombo->AddString( TEXT("32") );
		DragGridSizeCombo->AddString( TEXT("64") );
		DragGridSizeCombo->AddString( TEXT("128") );
		DragGridSizeCombo->AddString( TEXT("256") );
		DragGridSizeCombo->SelectionChangeDelegate = FDelegate(this,(TDelegate)&WBottomBar::OnDragGridSizeSelChange);

		new(PictureButtons)WPictureButton( this );
		RotGridCheck = &(PictureButtons(PictureButtons.Num() - 1));	check(RotGridCheck);
		RotGridCheck->SetUp( TEXT(""), IDCK_TOGGLE_ROT_GRID, 
			0, 0, ScaleX(22), ScaleY(20),
			hbm, ScaleX(88), 0, ScaleX(22), ScaleY(20),
			hbm, ScaleX(88), ScaleY(20), ScaleX(22), ScaleY(20));
		RotGridCheck->OpenWindow();
		RotGridCheck->OnSize( SIZE_MAXSHOW, ScaleX(22), ScaleY(20));

		new(PictureButtons)WPictureButton( this );
		SoundCheck = &(PictureButtons(PictureButtons.Num() - 1));	check(SoundCheck);
		SoundCheck->SetUp( TEXT(""), IDCK_TOGGLE_SOUND, 
			0, 0, ScaleX(22), ScaleY(20),
			hbm, ScaleX(176), 0, ScaleX(22), ScaleY(20),
			hbm, ScaleX(176), ScaleY(20), ScaleX(22), ScaleY(20) );
		SoundCheck->OpenWindow();
		SoundCheck->OnSize( SIZE_MAXSHOW, ScaleX(22), ScaleY(20) );

		new(PictureButtons)WPictureButton( this );
		MaximizeCheck = &(PictureButtons(PictureButtons.Num() - 1));	check(MaximizeCheck);
		MaximizeCheck->SetUp( TEXT(""), IDCK_MAXIMIZE,
			0, 0, ScaleX(22), ScaleY(20),
			hbm, ScaleX(154), 0, ScaleX(22), ScaleY(20),
			hbm, ScaleX(154), ScaleY(20), ScaleX(22), ScaleY(20));
		MaximizeCheck->OpenWindow();
		MaximizeCheck->OnSize( SIZE_MAXSHOW, ScaleX(22), ScaleY(20) );

		BoxSelInfo = new WLabel(this, IDSC_BOX_SEL_INFO);
		BoxSelInfo->OpenWindow(1, 0, 0);
		BoxSelInfo->MoveWindow(0, 0, ScaleX(135), ScaleY(32), 1);
		SendMessageW(BoxSelInfo->hWnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), FALSE);
		LONG lExStyle = GetWindowLong(BoxSelInfo->hWnd, GWL_EXSTYLE);
		lExStyle |= WS_EX_CLIENTEDGE;
		SetWindowLong(BoxSelInfo->hWnd, GWL_EXSTYLE, lExStyle);

		UpdateButtons();

		SetTimer(hWnd, 0, 100, NULL);

		LogCommandCombo->ReadMRU();

		SetRedraw(true);

		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WBottomBar::OnSize);
		WWindow::OnSize(Flags, NewX, NewY);
		PositionChildControls();
		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void PositionChildControls( void )
	{
		guard(WBottomBar::PositionChildControls);

		RECT rc;
		::GetClientRect( hWnd, &rc );

		int LastX = 2;

		for( int x = 0 ; GWBB_WndPos[x].ID != -1 ; x++ )
		{
			HWND hwnd = GetDlgItem( hWnd, GWBB_WndPos[x].ID );
			if( !hwnd )
			{
				// Check the other controls to see if we can get a match.
				switch( GWBB_WndPos[x].ID )
				{
					case IDPB_LOG_WND:
						hwnd = LogWndButton->hWnd;
						break;
					case IDCK_LOCK:
						hwnd = TextureLockCheck->hWnd;
						break;
					case IDCK_SNAP_VERTEX:
						hwnd = SnapVertexCheck->hWnd;
						break;
					case IDCK_TOGGLE_GRID:
						hwnd = DragGridCheck->hWnd;
						break;
					case IDCK_TOGGLE_ROT_GRID:
						hwnd = RotGridCheck->hWnd;
						break;
					case IDCK_TOGGLE_SOUND:
						hwnd = SoundCheck->hWnd;
						break;
					case IDCK_MAXIMIZE:
						hwnd = MaximizeCheck->hWnd;
						break;
				}
			}
			check(hwnd);

			int Height = MulDiv(GWBB_WndPos[x].Height, DPIY, 96);
			::MoveWindow( hwnd, LastX, rc.bottom / 2 - Height / 2, MulDiv(GWBB_WndPos[x].Width, DPIX, 96), Height, 1);
			LastX += MulDiv(GWBB_WndPos[x].Width + GWBB_WndPos[x].Pad, DPIX, 96);

			ToolTipCtrl->AddTool( hwnd, GWBB_WndPos[x].ToolTip, GWBB_WndPos[x].ID );
		}

		unguard;
	}
	void OnPaint()
	{
		guard(WBottomBar::OnPaint);
		PAINTSTRUCT PS;
		HDC hDC = BeginPaint( *this, &PS );
		HBRUSH brushBack = CreateSolidBrush( RGB(128,128,128) );

		FRect Rect = GetClientRect();
		FillRect( hDC, Rect, brushBack );
		MyDrawEdge( hDC, Rect, 1 );

		EndPaint( *this, &PS );

		DeleteObject( brushBack );
		unguard;
	}
	// Updates the states of the buttons to match editor settings.
	void OnCommand( INT Command )
	{
		guard(WBottomBar::OnCommand);

		switch( Command )
		{
			case WM_ADD_MRU:
				LogCommandCombo->AddMRU( GCommand );
				break;

			case WM_BB_RCLICK:
			{
			}
			break;

			case WM_PB_PUSH:
				ButtonClicked(LastlParam);
				break;

			default:
				switch( HIWORD(Command) )
				{
					case BN_CLICKED:
					{
						switch( LOWORD(Command) )
						{
							case IDCK_LOCK:
							{
								GEditor->Constraints.TextureLock = !GEditor->Constraints.TextureLock;
								GEditor->SaveConfig();
								UpdateButtons();
							}
							break;

							case IDCK_SNAP_VERTEX:
							{
								GEditor->Constraints.SnapVertices = !GEditor->Constraints.SnapVertices;
								GEditor->SaveConfig();
								UpdateButtons();
							}
							break;

							case IDCK_TOGGLE_GRID:
							{
								GEditor->Constraints.GridEnabled = !GEditor->Constraints.GridEnabled;
								GEditor->SaveConfig();
								UpdateButtons();
							}
							break;

							case IDCK_TOGGLE_ROT_GRID:
							{
								GEditor->Constraints.RotGridEnabled = !GEditor->Constraints.RotGridEnabled;
								GEditor->SaveConfig();
								UpdateButtons();
							}
							break;

							case IDCK_TOGGLE_SOUND:
							{
								ToggleSound();
								UpdateButtons();
							}
							break;

							case IDPB_LOG_WND:
							{
								if( GLogWindow )
								{
									GLogWindow->Show(1);
									SetFocus( *GLogWindow );
									GLogWindow->Display.ScrollCaret();
								}
								UpdateButtons();
							}
							break;

							// Center the current viewport on the selected object (make it fit in the viewport too)
							case IDCK_ZOOMCENTER:
							{
							}
							break;

							// Same as IDCK_ZOOMCENTER, but it affects ALL viewports.
							case IDCK_ZOOMCENTER_ALL:
							{
							}
							break;

							case IDCK_MAXIMIZE:
							{
								if( GCurrentViewportFrame )
								{
									if( IsZoomed( GCurrentViewportFrame ) )
										ShowWindow( GCurrentViewportFrame, SW_RESTORE );
									else
										ShowWindow( GCurrentViewportFrame, SW_MAXIMIZE );
									UpdateButtons();
								}
							}
							break;

							default:
								WWindow::OnCommand(Command);
								break;
						}
					}
					break;
				}
				break;
		}

		unguard;
	}
	void ButtonClicked( INT ID )
	{
		guard(WBottomBar::ButtonClicked);

		switch( ID )
		{
			case IDPB_LOG_WND:
				if( GLogWindow )
				{
					GLogWindow->Show(1);
					SetFocus( *GLogWindow );
					GLogWindow->Display.ScrollCaret();
				}
			break;

			case IDCK_LOCK:
			{
				GEditor->Constraints.TextureLock = !GEditor->Constraints.TextureLock;
				GEditor->SaveConfig();
			}
			break;

			case IDCK_SNAP_VERTEX:
			{
				GEditor->Constraints.SnapVertices = !GEditor->Constraints.SnapVertices;
				GEditor->SaveConfig();
			}
			break;

			case IDCK_TOGGLE_GRID:
			{
				GEditor->Constraints.GridEnabled = !GEditor->Constraints.GridEnabled;
				GEditor->SaveConfig();
			}
			break;

			case IDCK_TOGGLE_ROT_GRID:
			{
				GEditor->Constraints.RotGridEnabled = !GEditor->Constraints.RotGridEnabled;
				GEditor->SaveConfig();
			}
			break;

			case IDCK_TOGGLE_SOUND:
			{
				ToggleSound();
			}
			break;

			case IDCK_MAXIMIZE:
			{
				if( GCurrentViewportFrame )
				{
					if( IsZoomed( GCurrentViewportFrame ) )
						ShowWindow( GCurrentViewportFrame, SW_RESTORE );
					else
						ShowWindow( GCurrentViewportFrame, SW_MAXIMIZE );
				}
			}
			break;
		}

		UpdateButtons();
		InvalidateRect( hWnd, NULL, FALSE );

		unguard;
	}
	BOOL ToggleSound(BOOL bOnlyCheck = FALSE)
	{
		UBOOL bSoundOn = TRUE;
		UByteProperty* Property;
		
		if (GEditor && GEditor->Audio && 
			(Property = Cast<UByteProperty>(GEditor->Audio->FindObjectField(TEXT("SoundVolume")))) != nullptr)
		{
			FString Buffer;
			Property->ExportText(0, Buffer, (BYTE*)GEditor->Audio, (BYTE*)GEditor->Audio, PPF_Localized);
			if (Buffer.Len() == 0 || Buffer == TEXT("0"))
			{
				Buffer = TEXT("0");
				bSoundOn = FALSE;
			}
			   
			if (!bOnlyCheck)
			{
				static FString PrevSoundVolume;
				if (bSoundOn)
				{
					PrevSoundVolume = Buffer;
					Buffer = TEXT("0");
				}						
				else if (PrevSoundVolume.Len() > 0)
				{
					Buffer = PrevSoundVolume;
				}
				else
				{
					// We're trying to enable sound, but there is no known prior volume value
					// => Try to read it from the class defaults
					Buffer.Empty();	
					Property->ExportText(0 , Buffer, &GEditor->Audio->GetClass()->Defaults(0), &GEditor->Audio->GetClass()->Defaults(0), PPF_Localized);

					if (Buffer.Len() == 0 || Buffer == TEXT("0"))
						Buffer = TEXT("64");							
				}

				Property->ImportText(*Buffer, (BYTE*)GEditor->Audio + Property->Offset, PPF_Localized);
				GEditor->Audio->PostEditChange();
				bSoundOn = !bSoundOn;
			}
		}

		FStringOutputDevice Default;		
		if (GEditor->Exec(TEXT("get ini:Engine.Engine.AudioDevice SoundVolume"), Default) && Default == TEXT("0"))
			GEditor->Exec(TEXT("set ini:Engine.Engine.AudioDevice SoundVolume 64"));

		return bSoundOn;
	}
	INT OnSetCursor()
	{
		guard(WBottomBar::OnSetCursor);
		WWindow::OnSetCursor();
		SetCursor(LoadCursorW(NULL, IDC_ARROW));
		return 0;
		unguard;
	}
	// Updates the states of the buttons to match editor settings.
	void UpdateButtons()
	{
		guard(WBottomBar::UpdateButtons);

		TextureLockCheck->bOn = !GEditor->Constraints.TextureLock;
		InvalidateRect( TextureLockCheck->hWnd, NULL, 1 );
		SnapVertexCheck->bOn = !GEditor->Constraints.SnapVertices;
		InvalidateRect( SnapVertexCheck->hWnd, NULL, 1 );
		DragGridCheck->bOn = !GEditor->Constraints.GridEnabled;
		InvalidateRect( DragGridCheck->hWnd, NULL, 1 );
		RotGridCheck->bOn = !GEditor->Constraints.RotGridEnabled;
		InvalidateRect( RotGridCheck->hWnd, NULL, 1 );
		SoundCheck->bOn = !ToggleSound(TRUE);
		InvalidateRect( SoundCheck->hWnd, NULL, 1 );
		MaximizeCheck->bOn = !IsZoomed( GCurrentViewportFrame );
		InvalidateRect( MaximizeCheck->hWnd, NULL, 1 );

		FString Size;
	
		Size = *(FString::Printf(TEXT("%d"), (int)(GEditor->Constraints.GridSize.X) ) );
		DragGridSizeCombo->SetCurrent( DragGridSizeCombo->FindStringExact( *Size ) );

		UpdateBoxSelInfo();

		unguard;
	}

	void OnTimer()
	{
		UpdateBoxSelInfo();
	}

	void UpdateBoxSelInfo()
	{
		guard(WBottomBar::UpdateBoxSelInfo);

		if (!BoxSelInfo)
			return;

		FString Info(TEXT(""));

		UViewport* Viewport = (UViewport*)GCurrentViewport;

		if (GbIsBoxSel && Viewport)
		{
			FVector Delta = GBoxSelEnd - GBoxSelStart;

			FLOAT *OrthoAxis1, *OrthoAxis2;
			if (Viewport->Actor->RendMap == REN_OrthXY)
			{
				OrthoAxis1 = &Delta.X;
				OrthoAxis2 = &Delta.Y;
			}
			else if( Viewport->Actor->RendMap==REN_OrthXZ )
			{
				OrthoAxis1 = &Delta.X;
				OrthoAxis2 = &Delta.Z;
			}
			else // if( Viewport->Actor->RendMap==REN_OrthYZ )
			{
				OrthoAxis1 = &Delta.Y;
				OrthoAxis2 = &Delta.Z;
			}

			Info = FString::Printf(TEXT("%d, %d (%.1f)"), (INT)Abs(*OrthoAxis1), (INT)Abs(*OrthoAxis2), appSqrt(*OrthoAxis1 * *OrthoAxis1 + *OrthoAxis2 * *OrthoAxis2));
		}
		if (Info != BoxSelInfo->GetText())
		{
			InvalidateRect(BoxSelInfo->hWnd, NULL, FALSE);
			BoxSelInfo->SetText(*Info);
		}

		unguard;
	}

	void OnDragGridSizeSelChange()
	{
		guard(WBottomBar::OnDragGridSizeSelChange);

		FString Size = DragGridSizeCombo->GetString( DragGridSizeCombo->GetCurrent() );
		INT iSize = appAtoi(*Size);

		GEditor->Constraints.GridSize.X = GEditor->Constraints.GridSize.Y = GEditor->Constraints.GridSize.Z = iSize;
		GEditor->SaveConfig();
		UpdateButtons();
		PostMessageW( GhwndEditorFrame, WM_COMMAND, WM_REDRAWALLVIEWPORTS, 0 );

		unguard;
	}
	void OnLogCommandSelChange()
	{
		guard(WBottomBar::OnLogCommandSelChange);
		GEditor->Exec( *LogCommandCombo->GetString( LogCommandCombo->GetCurrent() ) );
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
