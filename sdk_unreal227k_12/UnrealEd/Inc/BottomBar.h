/*=============================================================================
	BottomBar : Class for handling the controls on the bottom bar
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>

//#define IDPB_MODE_CAMERA		19000

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

#define dSPACE_BETWEEN_GROUPS 16

struct {
	int ID;
	int Width;
	int Height;
	int Pad;		// Amount of space to leave between this control and the one to it's right
	TCHAR ToolTip[64];
}
GWBB_WndPos[] =
{
	IDSC_LOG_COMMAND,		64, 	20,		4,			TEXT("Enter a Log Command and Press ENTER"),
	IDCB_LOG_COMMAND,		384,	256,	4,			TEXT("Enter a Log Command and Press ENTER"),
	IDPB_LOG_WND,			22,		20,		dSPACE_BETWEEN_GROUPS,	TEXT("Show Full Log Window"),
	//IDCK_LOCK,				22,		22,		0,			TEXT("Toggle Selection Lock"),
	IDCK_SNAP_VERTEX,		22,		20,		dSPACE_BETWEEN_GROUPS,	TEXT("Toggle Vertex Snap"),
	IDCK_TOGGLE_GRID,		22,		20,		4,			TEXT("Toggle Drag Grid"),
	IDCB_GRID_SIZE,			64,		256,	dSPACE_BETWEEN_GROUPS,	TEXT("Drag Grid Size"),
	IDCK_TOGGLE_ROT_GRID,	22,		20,		4,			TEXT("Toggle Rotation Grid"),
	//IDCB_ROT_GRID_SIZE,		64,		256,	dSPACE_BETWEEN_GROUPS,	TEXT("Rotation Grid Size"),
	//IDCK_ZOOMCENTER_ALL,	22,		20,		0,			TEXT("Center All Viewports on Selection"),
	//IDCK_ZOOMCENTER,		22,		20,		0,			TEXT("Center Viewport on Selection"),
	IDCK_MAXIMIZE,			22,		20,		0,			TEXT("Maximize Viewport"),
	-1, -1, -1, -1, TEXT("")
};

WNDPROC lpfnEditWndProc = NULL; // original wndproc for the combo box 
HWND GHwndBar = NULL;
FString GCommand;
LRESULT CALLBACK LogCommandEdit_Proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_KEYDOWN:
			switch (wParam)
			{
				case VK_RETURN:
				{
					char Text[256];
					::GetWindowTextA( hwnd, Text, 256 );
					GCommand = appFromAnsi( Text );
					GEditor->Exec( *GCommand );
					PostMessageW( GHwndBar, WM_COMMAND, WM_ADD_MRU, 0 );
					return 0;
				}
			}
			break;
	}

	// Call the original window procedure for default processing.
	return CallWindowProcA(lpfnEditWndProc, hwnd, msg, wParam, lParam);
}

#define MAX_MRU_COMMANDS 16

class WBottomBar : public WWindow
{
	DECLARE_WINDOWCLASS(WBottomBar,WWindow,Window)

	WToolTip* ToolTipCtrl{};

	WCustomLabel *LogCommandLabel{};
	WComboBox *LogCommandCombo{}, *DragGridSizeCombo{};
	WPictureButton *SnapVertexCheck{}, *DragGridCheck{}, *RotGridCheck{}, *MaximizeCheck{};
	WButton *LogWndButton{};
	HBRUSH hbrDark;

	TArray<WPictureButton> PictureButtons;
	TArray<WButton> Buttons;
	HBITMAP hbm, hbmLogWnd;
	BITMAP bm{};
	FString MRUCommands[MAX_MRU_COMMANDS];
	int NumMRUCommands;

	// Structors.
	WBottomBar( FName InPersistentName, WWindow* InOwnerWindow )
	:	WWindow( InPersistentName, InOwnerWindow )
	{
		//hbmLogWnd = NULL;
		hbm = (HBITMAP)LoadImageA( hInstance, MAKEINTRESOURCEA(IDB_BOTTOM_BAR), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR );	check(hbm);
		hbmLogWnd = (HBITMAP)LoadImageA( hInstance, MAKEINTRESOURCEA(IDB_BB_LOG_WND), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR );	check(hbmLogWnd);

		ScaleImageAndReplace(hbm);
		ScaleImageAndReplace(hbmLogWnd);

		GetObjectA( hbm, sizeof(BITMAP), (LPSTR)&bm );
		NumMRUCommands = 0;
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

		delete ToolTipCtrl;

		WWindow::OnDestroy();
		unguard;
	}
	void OnCreate()
	{
		guard(WBottomBar::OnCreate);
		WWindow::OnCreate();

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

		LogCommandCombo = new WComboBox( this, IDCB_LOG_COMMAND );
		LogCommandCombo->OpenWindow( 1, 0, CBS_DROPDOWN );
		LogCommandCombo->MoveWindow( 0, 0, ScaleX(32), ScaleY(32), 1);
		LogCommandCombo->SelectionChangeDelegate = FDelegate(this,(TDelegate)&WBottomBar::OnLogCommandSelChange);

		// We subclass the edit control inside of the combobox so we can catch events that
		// are normally invisible to us ... like the user pressing ENTER.
		HWND hwndEdit = GetWindow( LogCommandCombo->hWnd, GW_CHILD );
		check(hwndEdit);
		lpfnEditWndProc = (WNDPROC)SetWindowLongPtr(hwndEdit, GWLP_WNDPROC, (LONG_PTR)LogCommandEdit_Proc);
		check(lpfnEditWndProc);

		new(Buttons)WButton( this, IDPB_LOG_WND );
		LogWndButton = &(Buttons(Buttons.Num() - 1));	check(LogWndButton);
		LogWndButton->OpenWindow( 1, 0, 0, ScaleX(22), ScaleY(20), NULL, 0, BS_OWNERDRAW );
		LogWndButton->SetBitmap( hbmLogWnd );
		
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
		RotGridCheck = &(PictureButtons(PictureButtons.Num() - 1));	check(SnapVertexCheck);
		RotGridCheck->SetUp( TEXT(""), IDCK_TOGGLE_ROT_GRID, 
			0, 0, ScaleX(22), ScaleY(20),
			hbm, ScaleX(88), 0, ScaleX(22), ScaleY(20),
			hbm, ScaleX(88), ScaleY(20), ScaleX(22), ScaleY(20));
		RotGridCheck->OpenWindow();
		RotGridCheck->OnSize( SIZE_MAXSHOW, ScaleX(22), ScaleY(20));

		new(PictureButtons)WPictureButton( this );
		MaximizeCheck = &(PictureButtons(PictureButtons.Num() - 1));	check(SnapVertexCheck);
		MaximizeCheck->SetUp( TEXT(""), IDCK_MAXIMIZE,
			0, 0, ScaleX(22), ScaleY(20),
			hbm, ScaleX(154), 0, ScaleX(22), ScaleY(20),
			hbm, ScaleX(154), ScaleY(20), ScaleX(22), ScaleY(20));
		MaximizeCheck->OpenWindow();
		MaximizeCheck->OnSize( SIZE_MAXSHOW, ScaleX(22), ScaleY(20) );

		UpdateButtons();

		unguard;
	}
	void RefreshComboBox()
	{
		guard(WBottomBar::RefreshComboBox);

		// Update the contents of the combobox with the MRU list of commands
		LogCommandCombo->Empty();

		for( int x = 0 ; x < NumMRUCommands ; x++ )
			LogCommandCombo->AddString( *MRUCommands[x] );

		unguard;
	}
	void AddCommandToMRU( FString InCommand )
	{
		guard(WBottomBar::AddCommandToMRU);

		// If the command already exists, leave.
		for( int x = 0 ; x < NumMRUCommands ; x++ )
			if( InCommand == MRUCommands[x] )
				return;

		// Otherwise, add it to end of the list.
		NumMRUCommands++;
		if( NumMRUCommands > MAX_MRU_COMMANDS )
		{
			NumMRUCommands = MAX_MRU_COMMANDS;
			for( int x = 0 ; x < MAX_MRU_COMMANDS - 1 ; x++ )
				MRUCommands[x] = MRUCommands[x + 1];
		}

		MRUCommands[NumMRUCommands - 1] = InCommand;

		RefreshComboBox();

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
					case IDCK_SNAP_VERTEX:
						hwnd = SnapVertexCheck->hWnd;
						break;
					case IDCK_TOGGLE_GRID:
						hwnd = DragGridCheck->hWnd;
						break;
					case IDCK_TOGGLE_ROT_GRID:
						hwnd = RotGridCheck->hWnd;
						break;
					case IDCK_MAXIMIZE:
						hwnd = MaximizeCheck->hWnd;
						break;
				}
			}
			check(hwnd);

			::MoveWindow( hwnd, LastX, rc.bottom / 4, MulDiv(GWBB_WndPos[x].Width, DPIX, 96), MulDiv(GWBB_WndPos[x].Height, DPIY, 96), 1);
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
				AddCommandToMRU( GCommand );
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
								GEditor->SelectionLock = !GEditor->SelectionLock;
								GEditor->SaveConfig();
								UpdateButtons();
							}
							break;

							case IDCK_SNAP_VERTEX:
							{
								GEditor->SnapVertices = !GEditor->SnapVertices;
								GEditor->SaveConfig();
								UpdateButtons();
							}
							break;

							case IDCK_TOGGLE_GRID:
							{
								GEditor->GridEnabled = !GEditor->GridEnabled;
								GEditor->SaveConfig();
								UpdateButtons();
							}
							break;

							case IDCK_TOGGLE_ROT_GRID:
							{
								GEditor->RotGridEnabled = !GEditor->RotGridEnabled;
								GEditor->SaveConfig();
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
				UpdateButtons();
				break;

			case IDCK_SNAP_VERTEX:
			{
				GEditor->SnapVertices = !GEditor->SnapVertices;
				GEditor->SaveConfig();
				UpdateButtons();
			}
			break;

			case IDCK_TOGGLE_GRID:
			{
				GEditor->GridEnabled = !GEditor->GridEnabled;
				GEditor->SaveConfig();
				UpdateButtons();
			}
			break;

			case IDCK_TOGGLE_ROT_GRID:
			{
				GEditor->RotGridEnabled = !GEditor->RotGridEnabled;
				GEditor->SaveConfig();
				UpdateButtons();
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
		}

		UpdateButtons();
		InvalidateRect( hWnd, NULL, FALSE );

		unguard;
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

		SendMessageW( GetDlgItem( hWnd, IDCK_LOCK ), BM_SETCHECK, (GEditor->SelectionLock?BST_CHECKED:BST_UNCHECKED), 0 );

		SnapVertexCheck->bOn = !GEditor->SnapVertices;
		InvalidateRect( SnapVertexCheck->hWnd, NULL, 1 );
		DragGridCheck->bOn = !GEditor->GridEnabled;
		InvalidateRect( DragGridCheck->hWnd, NULL, 1 );
		RotGridCheck->bOn = !GEditor->RotGridEnabled;
		InvalidateRect( RotGridCheck->hWnd, NULL, 1 );
		MaximizeCheck->bOn = !IsZoomed( GCurrentViewportFrame );
		InvalidateRect( MaximizeCheck->hWnd, NULL, 1 );

		FString Size;
	
		Size = *(FString::Printf(TEXT("%d"), (int)(GEditor->GridSize.X) ) );
		DragGridSizeCombo->SetCurrent( DragGridSizeCombo->FindStringExact( *Size ) );

		unguard;
	}

	void OnDragGridSizeSelChange()
	{
		guard(WBottomBar::OnDragGridSizeSelChange);

		FString Size = DragGridSizeCombo->GetString( DragGridSizeCombo->GetCurrent() );
		INT iSize = ::atoi( appToAnsi(*Size) );

		GEditor->GridSize.X = GEditor->GridSize.Y = GEditor->GridSize.Z = iSize;
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
