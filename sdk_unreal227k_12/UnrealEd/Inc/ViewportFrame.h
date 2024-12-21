/*=============================================================================
	ViewportFrame : Simple window to hold a viewport into a level
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

extern TSurfPropSheet* GSurfPropSheet;
extern TBuildSheet* GBuildSheet;

class WVFToolBar : public WWindow, public WViewportWindowContainer
{
	DECLARE_WINDOWCLASS(WVFToolBar,WWindow,Window)

	TArray<WPictureButton*> Buttons;
	HBITMAP hbm;
	BITMAP bm;
	FString Caption;	
	HBRUSH brushBack;
	HPEN penLine;	

	// Structors.
	WVFToolBar( FName InPersistentName, WWindow* InOwnerWindow )
	:	WWindow( InPersistentName, InOwnerWindow )
	,	WViewportWindowContainer(*InPersistentName, *InPersistentName)
	{
		hbm = (HBITMAP)LoadImageA( hInstance, MAKEINTRESOURCEA(IDBM_VF_TOOLBAR), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR );	check(hbm);
		ScaleImageAndReplace(hbm);
		GetObjectA( hbm, sizeof(BITMAP), (LPSTR)&bm );
		brushBack = CreateSolidBrush( RGB(128,128,128) );
		penLine = CreatePen( PS_SOLID, 1, RGB(80,80,80) );
	}

	// WWindow interface.
	void OpenWindow()
	{
		guard(WVFToolBar::OpenWindow);
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
		SendMessageW( *this, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(0,0) );
		unguard;
	}
	void OnDestroy()
	{
		guard(WVFToolBar::OnDestroy);
		WWindow::OnDestroy();
		for (int x = 0; x < Buttons.Num(); x++)
		{
			DestroyWindow(Buttons(x)->hWnd);
			delete Buttons(x);
		}
		Buttons.Empty();
		DeleteObject( hbm );
		DeleteObject( brushBack );
		DeleteObject( penLine );
		unguard;
	}
	void OnCreate()
	{
		guard(WVFToolBar::OnCreate);
		WWindow::OnCreate();
		ViewportOwnerWindow = OwnerWindow->hWnd;
		unguard;
	}
	void SetCaption( FString InCaption )
	{
		guard(WVFToolBar::SetCaption);
		Caption = InCaption;
		ForceRepaint();
		unguard;
	}
	void OnPaint()
	{
		guard(WVFToolBar::OnPaint);
		PAINTSTRUCT PS;
		HDC hDC = BeginPaint( *this, &PS );

		RECT rc;
		::GetClientRect( hWnd, &rc );

		FillRect( hDC, &rc, brushBack );

		rc.left += 2;
		rc.top += 1;
		if( GViewportStyle == VSTYLE_Fixed )
		{
			const ANSICHAR* AnsiCaption = appToAnsi(*Caption);
			::SetBkMode( hDC, TRANSPARENT );
			::DrawTextA( hDC, AnsiCaption, static_cast<int>(::strlen(AnsiCaption)), &rc, DT_LEFT | DT_SINGLELINE );
		}
		rc.left -= 2;
		rc.top -= 1;

		HPEN OldPen = (HPEN)SelectObject( hDC, penLine );
		::MoveToEx( hDC, 0, rc.bottom - 4, NULL );
		::LineTo( hDC, rc.right, rc.bottom - 4 );
		SelectObject( hDC, OldPen );

		EndPaint( *this, &PS );
		unguard;
	}
	WPictureButton* AddButton( const TCHAR* InToolTip, int InID,
		int InClientLeft, int InClientTop, int InClientRight, int InClientBottom,
		int InBmpOffLeft, int InBmpOffTop, int InBmpOffRight, int InBmpOffBottom,
		int InBmpOnLeft, int InBmpOnTop, int InBmpOnRight, int InBmpOnBottom )
	{
		guard(WVFToolBar::AddButton);
		WPictureButton* ppb = new WPictureButton( this );
		Buttons.AddItem(ppb);
		check(ppb);

		ppb->SetUp( InToolTip, InID, 
			InClientLeft, InClientTop, InClientRight, InClientBottom,
			hbm, InBmpOffLeft, InBmpOffTop, InBmpOffRight, InBmpOffBottom,
			hbm, InBmpOnLeft, InBmpOnTop, InBmpOnRight, InBmpOnBottom );
		ppb->OpenWindow();
		ppb->OnSize( SIZE_MAXSHOW, InClientRight, InClientBottom );

		return ppb;
		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WVFToolBar::OnSize);
		WWindow::OnSize(Flags, NewX, NewY);

		for( int x = 0 ; x < Buttons.Num() ; x++ )
		{
			auto* ppb = Buttons(x);
			::MoveWindow( ppb->hWnd, ppb->ClientPos.left, ppb->ClientPos.top, ppb->ClientPos.right, ppb->ClientPos.bottom, 1 );
		}

		unguard;
	}
	void OnRightButtonUp()
	{
		guard(WVFToolBar::OnRightButtonUp);
		UpdateRMBMenu(this);
		unguard;
	}
	// Sets the bOn variable in the various buttons
	void UpdateButtons()
	{
		guard(WVFToolBar::UpdateButtons);

		if( !pViewport) 
			return;

		for( int x = 0 ; x < Buttons.Num() ; x++ )
		{
			auto* ppb = Buttons(x);
			switch( ppb->ID )
			{
				case IDMN_VF_REALTIME_PREVIEW:
					ppb->bOn = pViewport->Actor->ShowFlags & SHOW_PlayerCtrl;
					break;

				case ID_MapDynLight:
					ppb->bOn = (pViewport->Actor->RendMap == REN_DynLight);
					break;

				case ID_MapPlainTex:
					ppb->bOn = (pViewport->Actor->RendMap == REN_PlainTex);
					break;

				case ID_MapWire:
					ppb->bOn = (pViewport->Actor->RendMap == REN_Wire);
					break;

				case ID_MapOverhead:
					ppb->bOn = (pViewport->Actor->RendMap == REN_OrthXY);
					break;

				case ID_MapXZ:
					ppb->bOn = (pViewport->Actor->RendMap == REN_OrthXZ);
					break;

				case ID_MapYZ:
					ppb->bOn = (pViewport->Actor->RendMap == REN_OrthYZ);
					break;

				case ID_MapPolys:
					ppb->bOn = (pViewport->Actor->RendMap == REN_Polys);
					break;

				case ID_MapPolyCuts:
					ppb->bOn = (pViewport->Actor->RendMap == REN_PolyCuts);
					break;

				case ID_MapZones:
					ppb->bOn = (pViewport->Actor->RendMap == REN_Zones);
					break;
	
				case ID_LightingOnly:
					ppb->bOn = (pViewport->Actor->RendMap == REN_LightingOnly);
					break;

				case ID_Normals:
					ppb->bOn = (pViewport->Actor->RendMap == REN_Normals);
					break;
			}

			ppb->ForceRepaint(TRUE);
		}

		unguard;
	}
	void OnCommand( INT Command )
	{
		guard(WVFToolBar::OnCommand);
		switch( Command ) {

			case WM_PB_PUSH:
				ButtonClicked(LastlParam);
				SendMessageW( OwnerWindow->hWnd, WM_COMMAND, WM_VIEWPORT_UPDATEWINDOWFRAME, 0 );
				break;

			default:
				WWindow::OnCommand(Command);
				break;
		}
		unguard;
	}
	void ButtonClicked( INT ID )
	{
		guard(WVFToolBar::ButtonClicked);
		ULevel* Level = GEditor->Level;

		switch( ID )
		{
			case IDMN_VF_REALTIME_PREVIEW:
				pViewport->Actor->ShowFlags ^= SHOW_PlayerCtrl;
				break;

			case ID_MapDynLight:
				pViewport->Actor->RendMap=REN_DynLight;
				break;

			case ID_MapPlainTex:
				pViewport->Actor->RendMap=REN_PlainTex;
				break;

			case ID_MapWire:
				pViewport->Actor->RendMap=REN_Wire;
				break;

			case ID_MapOverhead:
				pViewport->Actor->RendMap=REN_OrthXY;
				break;

			case ID_MapXZ:
				pViewport->Actor->RendMap=REN_OrthXZ;
				break;

			case ID_MapYZ:
				pViewport->Actor->RendMap=REN_OrthYZ;
				break;

			case ID_MapPolys:
				pViewport->Actor->RendMap=REN_Polys;
				break;

			case ID_MapPolyCuts:
				pViewport->Actor->RendMap=REN_PolyCuts;
				break;

			case ID_MapZones:
				pViewport->Actor->RendMap=REN_Zones;
				break;
	
			case ID_LightingOnly:
				pViewport->Actor->RendMap = REN_LightingOnly;
				break;

			case ID_Normals:
				pViewport->Actor->RendMap = REN_Normals;
				break;
		}
		pViewport->RepaintPending = TRUE;

		UpdateButtons();
		ForceRepaint();

		unguard;
	}
};

struct
{
	int ID;
	TCHAR ToolTip[64];
	int Move;
} GVFButtons[] =
{
	IDMN_VF_REALTIME_PREVIEW, TEXT("Realtime Preview (P)"), 22,
	-1, TEXT(""), 11,
	ID_MapOverhead, TEXT("Top (Alt+7)"), 22,
	ID_MapXZ, TEXT("Front (Alt+8)"), 22,
	ID_MapYZ, TEXT("Side (Alt+9)"), 22,	-1, TEXT(""), 11,
	ID_MapWire, TEXT("Perspective (Alt+1)"), 22,
	ID_MapPolys, TEXT("Texture Usage (Alt+3)"), 22,
	ID_MapPolyCuts, TEXT("BSP Cuts (Alt+4)"), 22,
	ID_MapPlainTex, TEXT("Textured (Alt+6)"), 22,
	ID_MapDynLight, TEXT("Dynamic Light (Alt+5)"), 22,
	ID_MapZones, TEXT("Zone/Portal (Alt+2)"), 22,
	ID_LightingOnly, TEXT("Lighting Only (Alt+0)"), 22,
	ID_Normals, TEXT("Normals (Alt+N)"), 22,
	-2, TEXT(""), 0
};

class WViewportFrame : public WWindow, public WViewportWindowContainer
{
	DECLARE_WINDOWCLASS(WViewportFrame,WWindow,Window)

	int m_iIdx;				// Index into the global TArray of viewport frames (GViewports)
	FString Caption;
	HBITMAP bmpToolbar{};
	WVFToolBar* VFToolbar;

	// Structors.
	WViewportFrame( FName InPersistentName, WWindow* InOwnerWindow )
	:	WWindow( InPersistentName, InOwnerWindow )
	,	WViewportWindowContainer(*InPersistentName, *InPersistentName)
	{
		m_iIdx = INDEX_NONE;
		Caption = TEXT("");
		VFToolbar = NULL;
	}

	// WWindow interface.
	void OpenWindow()
	{
		guard(WViewportFrame::OpenWindow);
		MdiChild = 0;

		PerformCreateWindowEx
		(
			0,
			TEXT("Viewport"),
			(GViewportStyle == VSTYLE_Floating)
				? WS_OVERLAPPEDWINDOW | WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS
				: WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
			0,
			0,
			MulDiv(320, DPIX, 96),
			MulDiv(200, DPIY, 96),
			OwnerWindow ? OwnerWindow->hWnd : NULL,
			NULL,
			hInstance
		);

		VFToolbar = new WVFToolBar( TEXT("VFToolbar"), this );
		VFToolbar->OpenWindow();

		int BmpPos = 0;
		for( int x = 0 ; GVFButtons[x].ID != -2 ; x++ )
		{
			if( GVFButtons[x].ID != -1 )
			{
				VFToolbar->AddButton( GVFButtons[x].ToolTip, GVFButtons[x].ID,
					0, 0, MulDiv(22, DPIX, 96), MulDiv(20, DPIY, 96),
					MulDiv((22*BmpPos), DPIX, 96), 0, MulDiv(22, DPIY, 96), MulDiv(20, DPIY, 96),
					MulDiv((22*BmpPos), DPIX, 96), MulDiv(20, DPIY, 96), MulDiv(22, DPIX, 96), MulDiv(20, DPIY, 96));
				BmpPos++;
			}
		}
		AdjustToolbarButtons();

		VFToolbar->UpdateButtons();
		UpdateWindow();		

		unguard;
	}
	// Initializes this viewport with the specified rendmap, and renderer
	void ForceInitializeViewport(INT RendMap, DWORD ShowFlags, const TCHAR* Renderer=NULL)
	{
		FString Device;
		if (!Renderer)
		{
			//Renderer = DEFAULT_UED_RENDERER_PATHNAME;
			GConfig->GetString(*PersistentName, TEXT("Device"), Device, GUEdIni);
			Renderer = *Device;
		}
		// Remember this renderer
		GConfig->SetString(*PersistentName, TEXT("Device"), Renderer, GUEdIni);
		
		// Figure out dimensions and create the viewport
		FRect R = GetClientRect();
		CreateViewport(ShowFlags, RendMap, R.Width(), R.Height(), R.Min.X, R.Min.Y, Renderer);
		check(pViewport);		

		// Forces things to set themselves up properly when the viewport is first assigned.
		OnSize(SIZE_MAXSHOW, R.Max.X, R.Max.Y);

		// compute position data
		ComputePositionData();

		VFToolbar->pViewport = pViewport;
		UpdateWindow();
	}
	// Initialize the viewport with the rendmap and showflags read from the ini file
	void InitializeViewport()
	{
		INT ShowFlags, RendMap;
		
		// Get Saved RendMap
		if (!GConfig->GetInt(*PersistentName, TEXT("RendMap"), RendMap, GUEdIni))	RendMap = REN_OrthXY;

		// Get Saved Showflags
		ShowFlags = SHOW_EditorMode;
		Load_ShowFlags(ShowFlags, *PersistentName);

		// Get Saved RenDev
		FString Device;
		GConfig->GetString(*PersistentName, TEXT("Device"), Device, GUEdIni);

		// All good
		ForceInitializeViewport(RendMap, ShowFlags, *Device);
	}
	// call this function when the viewportstyle changes to move buttons on the toolbar back and forth.
	void AdjustToolbarButtons()
	{
		guard(WViewportFrame::AdjustToolbarButtons);
		int Button = 0, Pos = (GViewportStyle == VSTYLE_Floating ? 0 : MulDiv(100, DPIX, 96) );
		WPictureButton* pButton;
		for( int x = 0 ; GVFButtons[x].ID != -2 ; x++ )
		{
			if( GVFButtons[x].ID != -1 )
			{
				pButton = VFToolbar->Buttons(Button);
				pButton->ClientPos.left = Pos;
				::MoveWindow( pButton->hWnd, pButton->ClientPos.left, pButton->ClientPos.top, pButton->ClientPos.right, pButton->ClientPos.bottom, TRUE);
				Button++;
			}
			Pos += MulDiv(GVFButtons[x].Move, DPIX, 96);
		}
		unguard;
	}
	void OnDestroy()
	{
		guard(WViewportFrame::OnDestroy);
		DestroyWindow(VFToolbar->hWnd);
		delete VFToolbar;
		unguard;
	}
	void OnCreate()
	{
		guard(WViewportFrame::OnCreate);
		WWindow::OnCreate();
		ViewportOwnerWindow = hWnd;
		unguard;
	}
	void OnPaint()
	{
		guard(WViewportFrame::OnPaint);
		PAINTSTRUCT PS;
		HDC hDC = BeginPaint( *this, &PS );
		FRect rectOutline = GetClientRect();
		HBRUSH brushOutline = CreateSolidBrush( RGB(192,192,192) );

		rectOutline.Min.X += 2;
		rectOutline.Min.Y += 2;
		rectOutline.Max.X -= 2;
		rectOutline.Max.Y -= 2;

		// The current viewport has a white border.
#if OLDUNREAL_BINARY_COMPAT
		if (GCurrentViewport == (DWORD)pViewport)
#else
		if( GCurrentViewport == pViewport )
#endif
			FillRect( hDC, GetClientRect(), (HBRUSH)GetStockObject(WHITE_BRUSH) );
		else
			FillRect( hDC, GetClientRect(), (HBRUSH)GetStockObject(BLACK_BRUSH) );

		FillRect( hDC, rectOutline, brushOutline );

		EndPaint( *this, &PS );

		DeleteObject(brushOutline);
		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WViewportFrame::OnSize);
		WWindow::OnSize(Flags, NewX, NewY);

		FRect R = GetClientRect();
		if( VFToolbar )
			::MoveWindow( VFToolbar->hWnd, 3, 3, R.Max.X - 6, MulDiv(24, DPIY, 96), 1 );

		RECT rcVFToolbar = { 0, 0, 0, 0 };
		if( VFToolbar )
			::GetClientRect( VFToolbar->hWnd, &rcVFToolbar );

		if( pViewport )
		{
			// Viewport should leave a border around the outside so we can show which viewport is active.
			// This border will be colored white if the viewport is active, black if not.

			R.Max.X -= 3;
			R.Max.Y -= 3;
			R.Min.X += 3;
			R.Min.Y += (rcVFToolbar.bottom - rcVFToolbar.top);
			::MoveWindow( (HWND)pViewport->GetWindow(), R.Min.X, R.Min.Y, R.Width(), R.Height(), 1 );
			pViewport->RepaintPending = TRUE;
		}

		ForceRepaint();
		unguard;
	}
	// Computes the viewport frame's position data relative to the parent window
	void ComputePositionData()
	{
		guard(WViewportFrame::ComputePositionData);
		VIEWPORTCONFIG* pVC = &(GViewports(m_iIdx));

		// Get the size of the client area we are being put into.
		RECT rcParent;
		HWND hwndParent = GetParent(GetParent(hWnd));

		if (hwndParent && ::GetClientRect(hwndParent, &rcParent))
		{
			WINDOWPLACEMENT wPos;
			wPos.length = sizeof(wPos);
			if (::GetWindowPlacement(hWnd, &wPos))
			{
				// Get restored size of this window, in workspace coordinates.
				RECT rcThis = wPos.rcNormalPosition;

				// Compute the sizing percentages for this window.
				//

				if (rcParent.right > 0 || rcParent.bottom > 0) // IsIconized won't return reliable results here. Rather assume it's minimzed if 0. max check below may avoid a crash, but will still mess up viewports.
				{
					// Avoid divide by zero
					rcParent.right = Max<INT>(rcParent.right, 1);
					rcParent.bottom = Max<INT>(rcParent.bottom, 1);

					pVC->PctLeft = rcThis.left / (float)rcParent.right;
					pVC->PctTop = rcThis.top / (float)rcParent.bottom;
					pVC->PctRight = (rcThis.right / (float)rcParent.right) - pVC->PctLeft;
					pVC->PctBottom = (rcThis.bottom / (float)rcParent.bottom) - pVC->PctTop;

					// Clamp the percentages to be INT's ... this prevents the viewports from drifting
					// between sessions.
					pVC->PctLeft = appRound(pVC->PctLeft * 100.0f) / 100.0f;
					pVC->PctTop = appRound(pVC->PctTop * 100.0f) / 100.0f;
					pVC->PctRight = appRound(pVC->PctRight * 100.0f) / 100.0f;
					pVC->PctBottom = appRound(pVC->PctBottom * 100.0f) / 100.0f;

					pVC->Left = rcThis.left;
					pVC->Top = rcThis.top;
					pVC->Right = rcThis.right - pVC->Left;
					pVC->Bottom = rcThis.bottom - pVC->Top;
				}
			}
			else GWarn->Logf(TEXT("GetWindowPlacement failed to get ViewportFrame!"));
		}
		unguard;
	}
	void OnCommand( INT Command )
	{
		guard(WViewportFrame::OnCommand);

		switch( Command )
		{
			case IDMN_VF_REALTIME_PREVIEW:
				pViewport->Actor->ShowFlags ^= SHOW_PlayerCtrl;
				UpdateWindow();
				pViewport->RepaintPending = TRUE;
				break;

			case WM_VIEWPORT_UPDATEWINDOWFRAME:
				UpdateWindow();
				break;

			case ID_MapDynLight:
				pViewport->Actor->RendMap=REN_DynLight;
				UpdateWindow();
				pViewport->RepaintPending = TRUE;
				break;

			case ID_MapPlainTex:
				pViewport->Actor->RendMap=REN_PlainTex;
				UpdateWindow();
				pViewport->RepaintPending = TRUE;
				break;

			case ID_MapWire:
				pViewport->Actor->RendMap=REN_Wire;
				UpdateWindow();
				pViewport->RepaintPending = TRUE;
				break;

			case ID_MapOverhead:
				pViewport->Actor->RendMap=REN_OrthXY;
				UpdateWindow();
				pViewport->RepaintPending = TRUE;
				break;

			case ID_MapXZ:
				pViewport->Actor->RendMap=REN_OrthXZ;
				UpdateWindow();
				pViewport->RepaintPending = TRUE;
				break;

			case ID_MapYZ:
				pViewport->Actor->RendMap=REN_OrthYZ;
				UpdateWindow();
				pViewport->RepaintPending = TRUE;
				break;

			case ID_MapPolys:
				pViewport->Actor->RendMap=REN_Polys;
				UpdateWindow();
				pViewport->RepaintPending = TRUE;
				break;

			case ID_MapPolyCuts:
				pViewport->Actor->RendMap=REN_PolyCuts;
				UpdateWindow();
				pViewport->RepaintPending = TRUE;
				break;

			case ID_MapZones:
				pViewport->Actor->RendMap=REN_Zones;
				UpdateWindow();
				pViewport->RepaintPending = TRUE;
				break;
			
			case ID_LightingOnly:
				pViewport->Actor->RendMap = REN_LightingOnly;
				UpdateWindow();
				pViewport->RepaintPending = TRUE;
				break;

			case ID_Normals:
				pViewport->Actor->RendMap = REN_Normals;
				UpdateWindow();
				pViewport->RepaintPending = TRUE;
				break;

			case IDMN_RD_SOFTWARE:
			case IDMN_RD_OPENGL:
			case IDMN_RD_D3D:
			case IDMN_RD_D3D9:
			case IDMN_RD_XOPENGL:
			case IDMN_RD_CUSTOMRENDER:
			case IDMN_RD_ICBINDX11:
			{
				SwitchRenderer(Command, TRUE);

				//
				VFToolbar->pViewport = pViewport;

				// Resize the viewport so it fits inside the frame
				OnSize(0, -1, -1);

				// Update the global viewport config
				ComputePositionData();

				// Update the menu
				UpdateWindow();

				// Repaint
				pViewport->RepaintPending = TRUE;
				break;
			}

			case ID_Color32Bit:
				pViewport->RenDev->SetRes( pViewport->SizeX, pViewport->SizeY, 4, 0 );
				pViewport->RepaintPending = TRUE;
				break;

			case ID_ShowBackdrop:
				pViewport->Actor->ShowFlags ^= SHOW_Backdrop;
				pViewport->RepaintPending = TRUE;
				break;

			case ID_ShowRealTimeBackdrop:
				pViewport->Actor->ShowFlags ^= SHOW_RealTimeBackdrop;
				pViewport->RepaintPending = TRUE;
				break;

			case ID_ShowDistanceFog:
				pViewport->Actor->ShowFlags ^= SHOW_DistanceFog;
				pViewport->RepaintPending = TRUE;
				break;

			case ID_LightColorIcon:
				pViewport->Actor->ShowFlags ^= SHOW_LightColorIcon;
				pViewport->RepaintPending = TRUE;
				break;

			case ID_ShowPathsPreview:
				pViewport->Actor->ShowFlags ^= SHOW_PathsPreview;
				pViewport->RepaintPending = TRUE;
				break;

			case ID_ActorsShow:
				pViewport->Actor->ShowFlags &= ~(SHOW_Actors | SHOW_ActorIcons | SHOW_ActorRadii);
				pViewport->Actor->ShowFlags |= SHOW_Actors; 
				pViewport->RepaintPending = TRUE;
				break;

			case ID_ActorsIcons:
				pViewport->Actor->ShowFlags &= ~(SHOW_Actors | SHOW_ActorIcons | SHOW_ActorRadii); 
				pViewport->Actor->ShowFlags |= SHOW_Actors | SHOW_ActorIcons;
				pViewport->RepaintPending = TRUE;
				break;

			case ID_ActorsRadii:
				pViewport->Actor->ShowFlags &= ~(SHOW_Actors | SHOW_ActorIcons | SHOW_ActorRadii); 
				pViewport->Actor->ShowFlags |= SHOW_Actors | SHOW_ActorRadii;
				pViewport->RepaintPending = TRUE;
				break;

			case ID_ActorsHide:
				pViewport->Actor->ShowFlags &= ~(SHOW_Actors | SHOW_ActorIcons | SHOW_ActorRadii); 
				pViewport->RepaintPending = TRUE;
				break;

			case ID_ShowPaths:
				pViewport->Actor->ShowFlags ^= SHOW_Paths;
				pViewport->RepaintPending = TRUE;
				break;

			case ID_ShowCoords:
				pViewport->Actor->ShowFlags ^= SHOW_Coords;
				pViewport->RepaintPending = TRUE;
				break;

			case ID_ShowBrush:
				pViewport->Actor->ShowFlags ^= SHOW_Brush;
				pViewport->RepaintPending = TRUE;
				break;

			case ID_ShowMovingBrushes:
				pViewport->Actor->ShowFlags ^= SHOW_MovingBrushes;
				pViewport->RepaintPending = TRUE;
				break;

			default:
				WWindow::OnCommand(Command);
				UpdateWindow();
				break;
		}
		unguard;
	}
	void UpdateWindow( void )
	{
		guard(WViewportFrame::UpdateWindow);
		if( !pViewport )
		{
			Caption = TEXT("UpdateWindow");
			return;
		}
		if (!pViewport->Actor)
		{
			Caption = TEXT("UpdateWindow Frame no Actor");
			return;
		}

		switch( pViewport->Actor->RendMap )
		{
			case REN_Wire:
				Caption = TEXT("Wireframe");
				break;

			case REN_Zones:
				Caption = TEXT("Zone/Portal");
				break;

			case REN_Polys:
				Caption = TEXT("Texture Use");
				break;

			case REN_PolyCuts:
				Caption = TEXT("BSP Cuts");
				break;

			case REN_DynLight:
				Caption = TEXT("Dynamic Light");
				break;

			case REN_PlainTex:
				Caption = TEXT("Textured");
				break;

			case REN_OrthXY:
				Caption = TEXT("Top");
				break;

			case REN_OrthXZ:
				Caption = TEXT("Front");
				break;

			case REN_OrthYZ:
				Caption = TEXT("Side");
				break;

			case REN_TexView:
				Caption = TEXT("Texture View");
				break;

			case REN_TexBrowser:
				Caption = TEXT("Texture Browser");
				break;

			case REN_MeshView:
				Caption = TEXT("Mesh Viewer");
				break;

			case REN_LightingOnly:
				Caption = TEXT("Lighting Only");
				break;

			case REN_Normals:
				Caption = TEXT("Render Normals");
				break;

			default:
				Caption = TEXT("Unknown");
				break;
		}

		SetText(*Caption);
		VFToolbar->SetCaption( Caption );
		VFToolbar->UpdateButtons();

		ForceRepaint(TRUE);
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
