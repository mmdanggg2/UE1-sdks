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

	TArray<WPictureButton> Buttons;
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
		hbm = (HBITMAP)LoadImageW( hInstance, MAKEINTRESOURCEW(IDBM_VF_TOOLBAR), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR );	check(hbm);
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
		for( int x = 0 ; x < Buttons.Num() ; x++ )
			DestroyWindow( Buttons(x).hWnd );
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
		unguard;
	}
	void SetCaption( FString InCaption )
	{
		guard(WVFToolBar::SetCaption);
		Caption = InCaption;
		InvalidateRect( hWnd, NULL, FALSE );
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
			::SetBkMode( hDC, TRANSPARENT );
			::DrawTextW( hDC, *Caption, Caption.Len(), &rc, DT_LEFT | DT_SINGLELINE );
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
	void AddButton( FString InToolTip, int InID, 
		int InClientLeft, int InClientTop, int InClientRight, int InClientBottom,
		int InBmpOffLeft, int InBmpOffTop, int InBmpOffRight, int InBmpOffBottom,
		int InBmpOnLeft, int InBmpOnTop, int InBmpOnRight, int InBmpOnBottom )
	{
		guard(WVFToolBar::AddButton);

		new(Buttons)WPictureButton( this );
		WPictureButton* ppb = &(Buttons(Buttons.Num() - 1));
		check(ppb);

		ppb->SetUp( InToolTip, InID, 
			InClientLeft, InClientTop, InClientRight, InClientBottom,
			hbm, InBmpOffLeft, InBmpOffTop, InBmpOffRight, InBmpOffBottom,
			hbm, InBmpOnLeft, InBmpOnTop, InBmpOnRight, InBmpOnBottom );
		ppb->OpenWindow();
		ppb->OnSize( SIZE_MAXSHOW, InClientRight, InClientBottom );

		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WVFToolBar::OnSize);
		WWindow::OnSize(Flags, NewX, NewY);

		for( int x = 0 ; x < Buttons.Num() ; x++ )
		{
			WPictureButton* ppb = &(Buttons(x));
			::MoveWindow( ppb->hWnd, ppb->ClientPos.left, ppb->ClientPos.top, ppb->ClientPos.right, ppb->ClientPos.bottom, 1 );
		}

		unguard;
	}
	void OnRightButtonUp()
	{
		guard(WVFToolBar::OnRightButtonUp);

		HMENU OuterMenu = AddHotkeys(LoadMenuW(hInstance, MAKEINTRESOURCEW(IDMENU_VF_CONTEXT)));
		HMENU l_menu = GetSubMenu(OuterMenu, 0);

		POINT pt;
		::GetCursorPos( &pt );

		// "Check" appropriate menu items based on current settings.
		CheckMenuItem( l_menu, ID_MapOverhead, (pViewport->Actor->RendMap == REN_OrthXY) ? MF_CHECKED : MF_UNCHECKED );
		CheckMenuItem( l_menu, ID_MapXZ, (pViewport->Actor->RendMap == REN_OrthXZ) ? MF_CHECKED : MF_UNCHECKED );
		CheckMenuItem( l_menu, ID_MapYZ, (pViewport->Actor->RendMap == REN_OrthYZ) ? MF_CHECKED : MF_UNCHECKED );
		CheckMenuItem( l_menu, ID_MapWire, (pViewport->Actor->RendMap == REN_Wire) ? MF_CHECKED : MF_UNCHECKED );
		CheckMenuItem( l_menu, ID_MapPolys, (pViewport->Actor->RendMap == REN_Polys) ? MF_CHECKED : MF_UNCHECKED );
		CheckMenuItem( l_menu, ID_MapPolyCuts, (pViewport->Actor->RendMap == REN_PolyCuts) ? MF_CHECKED : MF_UNCHECKED );
		CheckMenuItem( l_menu, ID_MapPlainTex, (pViewport->Actor->RendMap == REN_PlainTex) ? MF_CHECKED : MF_UNCHECKED );
		CheckMenuItem( l_menu, ID_MapDynLight, (pViewport->Actor->RendMap == REN_DynLight) ? MF_CHECKED : MF_UNCHECKED );
		CheckMenuItem( l_menu, ID_MapZones, (pViewport->Actor->RendMap == REN_Zones) ? MF_CHECKED : MF_UNCHECKED );
#if ENGINE_VERSION==227
		CheckMenuItem( l_menu, ID_LightingOnly, (pViewport->Actor->RendMap == REN_LightingOnly) ? MF_CHECKED : MF_UNCHECKED );
		CheckMenuItem( l_menu, ID_Normals, (pViewport->Actor->RendMap == REN_Normals) ? MF_CHECKED : MF_UNCHECKED);
#endif

		CheckMenuItem( l_menu, ID_ShowBrush, (pViewport->Actor->ShowFlags&SHOW_Brush) ? MF_CHECKED : MF_UNCHECKED );
		CheckMenuItem( l_menu, ID_ShowBackdrop, (pViewport->Actor->ShowFlags&SHOW_Backdrop) ? MF_CHECKED : MF_UNCHECKED );
#if ENGINE_VERSION==227
		CheckMenuItem( l_menu, ID_ShowRealTimeBackdrop, (pViewport->Actor->ShowFlags&SHOW_RealTimeBackdrop) ? MF_CHECKED : MF_UNCHECKED );
#endif
		CheckMenuItem( l_menu, ID_ShowCoords, (pViewport->Actor->ShowFlags&SHOW_Coords) ? MF_CHECKED : MF_UNCHECKED );
		CheckMenuItem( l_menu, ID_ShowTextureGrid, (pViewport->Actor->ShowFlags & SHOW_TextureGrid) ? MF_CHECKED : MF_UNCHECKED );
		CheckMenuItem( l_menu, ID_ShowMovingBrushes, (pViewport->Actor->ShowFlags&SHOW_MovingBrushes) ? MF_CHECKED : MF_UNCHECKED );
		CheckMenuItem( l_menu, ID_ShowMoverSurfaces, (pViewport->Actor->ShowFlags&SHOW_MoverSurfaces) ? MF_CHECKED : MF_UNCHECKED );
		CheckMenuItem( l_menu, ID_ShowMoverPath, (pViewport->Actor->ShowFlags&SHOW_MoverPath) ? MF_CHECKED : MF_UNCHECKED );
		CheckMenuItem( l_menu, ID_ShowPaths, (pViewport->Actor->ShowFlags&SHOW_Paths) ? MF_CHECKED : MF_UNCHECKED );
#if ENGINE_VERSION==227
		CheckMenuItem( l_menu, ID_ShowPathsPreview, (pViewport->Actor->ShowFlags&SHOW_PathsPreview) ? MF_CHECKED : MF_UNCHECKED );
		CheckMenuItem( l_menu, ID_ShowDistanceFog, (pViewport->Actor->ShowFlags&SHOW_DistanceFog) ? MF_CHECKED : MF_UNCHECKED );
		CheckMenuItem( l_menu, ID_LightColorIcon, (pViewport->Actor->ShowFlags&SHOW_LightColorIcon) ? MF_CHECKED : MF_UNCHECKED );
#endif
		CheckMenuItem( l_menu, ID_ShowEventLines, (pViewport->Actor->ShowFlags&SHOW_EventLines) ? MF_CHECKED : MF_UNCHECKED );
		CheckMenuItem( l_menu, ID_OccludeLines, (pViewport->Actor->ShowFlags&SHOW_OccludeLines) ? MF_CHECKED : MF_UNCHECKED );

		CheckMenuItem( l_menu, ID_Color16Bit, ((pViewport->ColorBytes==2) ? MF_CHECKED : MF_UNCHECKED ) );
		CheckMenuItem( l_menu, ID_Color32Bit, ((pViewport->ColorBytes==4) ? MF_CHECKED : MF_UNCHECKED ) );

		UpdateRendererMenu(l_menu, TRUE);

		DWORD ShowFilter = pViewport->Actor->ShowFlags & (SHOW_Actors | SHOW_ActorIcons | SHOW_ActorRadii);
		if		(ShowFilter==(SHOW_Actors | SHOW_ActorIcons)) CheckMenuItem( l_menu, ID_ActorsIcons, MF_CHECKED );
		else if (ShowFilter==(SHOW_Actors | SHOW_ActorRadii)) CheckMenuItem( l_menu, ID_ActorsRadii, MF_CHECKED );
		else if (ShowFilter==(SHOW_Actors                  )) CheckMenuItem( l_menu, ID_ActorsShow, MF_CHECKED );
		else CheckMenuItem( l_menu, ID_ActorsHide, MF_CHECKED );

		TrackPopupMenu( l_menu,
			TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
			pt.x, pt.y, 0,
			OwnerWindow->hWnd, NULL);
		DestroyMenu(OuterMenu);

		unguard;
	}
	void OnLeftButtonUp()
	{
		guard(WVFToolBar::OnLeftButtonUp);
		SendMessageW(OwnerWindow->hWnd, WM_COMMAND, WM_SETCURRENTVIEWPORT, (LPARAM)0);
		unguard;
	}
	// Sets the bOn variable in the various buttons
	void UpdateButtons()
	{
		guard(WVFToolBar::UpdateButtons);

		if( !pViewport ) return;

		for( int x = 0 ; x < Buttons.Num() ; x++ )
		{
			switch( Buttons(x).ID )
			{
				case IDMN_VF_REALTIME_PREVIEW:
					Buttons(x).bOn = pViewport->Actor->ShowFlags & SHOW_PlayerCtrl;
					break;

				case ID_MapDynLight:
					Buttons(x).bOn = (pViewport->Actor->RendMap == REN_DynLight);
					break;

				case ID_MapPlainTex:
					Buttons(x).bOn = (pViewport->Actor->RendMap == REN_PlainTex);
					break;

				case ID_MapWire:
					Buttons(x).bOn = (pViewport->Actor->RendMap == REN_Wire);
					break;

				case ID_MapOverhead:
					Buttons(x).bOn = (pViewport->Actor->RendMap == REN_OrthXY);
					break;

				case ID_MapXZ:
					Buttons(x).bOn = (pViewport->Actor->RendMap == REN_OrthXZ);
					break;

				case ID_MapYZ:
					Buttons(x).bOn = (pViewport->Actor->RendMap == REN_OrthYZ);
					break;

				case ID_MapPolys:
					Buttons(x).bOn = (pViewport->Actor->RendMap == REN_Polys);
					break;

				case ID_MapPolyCuts:
					Buttons(x).bOn = (pViewport->Actor->RendMap == REN_PolyCuts);
					break;

				case ID_MapZones:
					Buttons(x).bOn = (pViewport->Actor->RendMap == REN_Zones);
					break;

#if ENGINE_VERSION==227				
				case ID_LightingOnly:
					Buttons(x).bOn = (pViewport->Actor->RendMap == REN_LightingOnly);
					break;

				case ID_Normals:
					Buttons(x).bOn = (pViewport->Actor->RendMap == REN_Normals);
					break;
#endif
			}

			InvalidateRect( Buttons(x).hWnd, NULL, 1 );
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
	virtual LRESULT WndProc(UINT Message, WPARAM wParam, LPARAM lParam)
	{
		if (Message == WM_LBUTTONDBLCLK && OwnerWindow)
			PostMessage(OwnerWindow->hWnd, WM_SYSCOMMAND, IsZoomed(OwnerWindow->hWnd) ? SC_RESTORE : SC_MAXIMIZE, 0);

		return WWindow::WndProc(Message, wParam, lParam);
	}
	void ButtonClicked( INT ID )
	{
		guard(WVFToolBar::ButtonClicked);
		ULevel* Level = GEditor->Level;

		switch( ID )
		{
			case IDMN_VF_REALTIME_PREVIEW:
				for (INT i = 0; i < Level->Model->Surfs.Num(); i++)
				{
					FBspSurf& Surf = Level->Model->Surfs(i);
					if ((Surf.PolyFlags & PF_FakeBackdrop) && Surf.Actor && Surf.Actor->Region.Zone && Surf.Actor->Region.Zone->IsA(ASkyZoneInfo::StaticClass()))
					{
						GWarn->Logf(TEXT("Warning: PF_Fakebackdrop is set in a Skyzone, this can cause visual distortions if not intended!"));
						break;
					}
				}
				pViewport->Actor->ShowFlags ^= SHOW_PlayerCtrl;
				break;

			case ID_MapDynLight:
				pViewport->Actor->RendMap=REN_DynLight;
				pViewport->Repaint( 1 );
				break;

			case ID_MapPlainTex:
				pViewport->Actor->RendMap=REN_PlainTex;
				pViewport->Repaint( 1 );
				break;

			case ID_MapWire:
				pViewport->Actor->RendMap=REN_Wire;
				pViewport->Repaint( 1 );
				break;

			case ID_MapOverhead:
				pViewport->Actor->RendMap=REN_OrthXY;
				pViewport->Repaint( 1 );
				break;

			case ID_MapXZ:
				pViewport->Actor->RendMap=REN_OrthXZ;
				pViewport->Repaint( 1 );
				break;

			case ID_MapYZ:
				pViewport->Actor->RendMap=REN_OrthYZ;
				pViewport->Repaint( 1 );
				break;

			case ID_MapPolys:
				pViewport->Actor->RendMap=REN_Polys;
				pViewport->Repaint( 1 );
				break;

			case ID_MapPolyCuts:
				pViewport->Actor->RendMap=REN_PolyCuts;
				pViewport->Repaint( 1 );
				break;

			case ID_MapZones:
				pViewport->Actor->RendMap=REN_Zones;
				pViewport->Repaint( 1 );
				break;

#if ENGINE_VERSION==227			
			case ID_LightingOnly:
				pViewport->Actor->RendMap = REN_LightingOnly;
				pViewport->Repaint(1);
				break;

			case ID_Normals:
				pViewport->Actor->RendMap = REN_Normals;
				pViewport->Repaint(1);
				break;
#endif
		}

		UpdateButtons();
		InvalidateRect( hWnd, NULL, FALSE );

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
#if ENGINE_VERSION==227
	ID_LightingOnly, TEXT("Lighting Only (Alt+0)"), 22,
	ID_Normals, TEXT("Normals (Alt+N)"), 22,
#endif
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
			GConfig->GetString(*PersistentName, TEXT("Device"), Device, GUEDIni);
			Renderer = *Device;
		}
		// Remember this renderer
		GConfig->SetString(*PersistentName, TEXT("Device"), Renderer, GUEDIni);
		
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
		int RendMap, ShowBackdrop, ShowRealTimeBackdrop, ShowCoordinates, ShowTextureGrid, ShowPaths, ShowDistanceFog, ShowPathsPreview, ShowLightColorIcon, ShowActors, ShowEventLines, OccludeLines;
		
		// Get Saved RendMap
		if (!GConfig->GetInt(*PersistentName, TEXT("RendMap"), RendMap, GUEDIni))	RendMap = REN_OrthXY;

		// Get Saved Showflags
		if (!GConfig->GetInt(*PersistentName, TEXT("ShowBackdrop"), ShowBackdrop, GUEDIni)) ShowBackdrop = 0;
		if (!GConfig->GetInt(*PersistentName, TEXT("ShowRealTimeBackdrop"), ShowRealTimeBackdrop, GUEDIni)) ShowRealTimeBackdrop = 0;
		if (!GConfig->GetInt(*PersistentName, TEXT("ShowCoordinates"), ShowCoordinates, GUEDIni)) ShowCoordinates = 0;
		if (!GConfig->GetInt(*PersistentName, TEXT("ShowTextureGrid"), ShowTextureGrid, GUEDIni)) ShowTextureGrid = 0;
		if (!GConfig->GetInt(*PersistentName, TEXT("ShowPaths"), ShowPaths, GUEDIni)) ShowPaths = 0;
		if (!GConfig->GetInt(*PersistentName, TEXT("ShowPathsPreview"), ShowPathsPreview, GUEDIni)) ShowPathsPreview = 0;
		if (!GConfig->GetInt(*PersistentName, TEXT("ShowDistanceFog"), ShowDistanceFog, GUEDIni)) ShowDistanceFog = 0;
		if (!GConfig->GetInt(*PersistentName, TEXT("ShowLightColorIcon"), ShowLightColorIcon, GUEDIni)) ShowLightColorIcon = 0;
		if (!GConfig->GetInt(*PersistentName, TEXT("ShowActors"), ShowActors, GUEDIni)) ShowActors = SHOW_Actors;
		if (!GConfig->GetInt(*PersistentName, TEXT("ShowEventLines"), ShowEventLines, GUEDIni)) ShowEventLines = 1;
		if (!GConfig->GetInt(*PersistentName, TEXT("OccludeLines"), OccludeLines, GUEDIni)) OccludeLines = 1;
		ShowActors &= SHOW_Actors | SHOW_ActorIcons | SHOW_ActorRadii;

		DWORD ShowFlags = SHOW_Menu | SHOW_Frame | ShowActors | SHOW_Brush | SHOW_StandardView | SHOW_ChildWindow | SHOW_MovingBrushes | SHOW_MoverSurfaces;

		if (ShowBackdrop)
			ShowFlags ^= SHOW_Backdrop;
		if (ShowCoordinates)
			ShowFlags ^= SHOW_Coords;
		if (ShowTextureGrid)
			ShowFlags ^= SHOW_TextureGrid;
		if (ShowPaths)
			ShowFlags ^= SHOW_Paths;
#if ENGINE_VERSION==227
		if (ShowRealTimeBackdrop)
			ShowFlags ^= SHOW_RealTimeBackdrop;
		if (ShowPathsPreview)
			ShowFlags ^= SHOW_PathsPreview;
		if (ShowDistanceFog)
			ShowFlags ^= SHOW_DistanceFog;
		if (ShowDistanceFog)
			ShowFlags ^= SHOW_DistanceFog;
		if (ShowLightColorIcon)
			ShowFlags ^= SHOW_LightColorIcon;
#endif
		if (ShowEventLines)
			ShowFlags ^= SHOW_EventLines;
		if (OccludeLines)
			ShowFlags ^= SHOW_OccludeLines;

		// Get Saved RenDev
		FString Device;
		GConfig->GetString(*PersistentName, TEXT("Device"), Device, GUEDIni);

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
				pButton = &(VFToolbar->Buttons(Button));
				pButton->ClientPos.left = Pos;
				::MoveWindow( pButton->hWnd, pButton->ClientPos.left, pButton->ClientPos.top, pButton->ClientPos.right, pButton->ClientPos.bottom, 1 );
				Button++;
			}
			Pos += MulDiv(GVFButtons[x].Move, DPIX, 96);
		}
		unguard;
	}
	void OnDestroy()
	{
		guard(WViewportFrame::OnDestroy);

		GConfig->SetInt(*PersistentName, TEXT("RendMap"), pViewport->Actor->RendMap, GUEDIni);

		GConfig->SetInt(*PersistentName, TEXT("ShowBackdrop"), (pViewport->Actor->ShowFlags & SHOW_Backdrop) ? 1 : 0, GUEDIni);
		GConfig->SetInt(*PersistentName, TEXT("ShowCoordinates"), (pViewport->Actor->ShowFlags & SHOW_Coords) ? 1 : 0, GUEDIni);
		GConfig->SetInt(*PersistentName, TEXT("ShowTextureGrid"), (pViewport->Actor->ShowFlags & SHOW_TextureGrid) ? 1 : 0, GUEDIni);
		GConfig->SetInt(*PersistentName, TEXT("ShowPaths"), (pViewport->Actor->ShowFlags & SHOW_Paths) ? 1 : 0, GUEDIni);
		GConfig->SetInt(*PersistentName, TEXT("ShowActors"), 
			pViewport->Actor->ShowFlags & (SHOW_Actors | SHOW_ActorIcons | SHOW_ActorRadii), GUEDIni);
#if ENGINE_VERSION==227
		GConfig->SetInt(*PersistentName, TEXT("ShowRealTimeBackdrop"), (pViewport->Actor->ShowFlags & SHOW_RealTimeBackdrop) ? 1 : 0, GUEDIni);
		GConfig->SetInt(*PersistentName, TEXT("ShowPathsPreview"), (pViewport->Actor->ShowFlags & SHOW_PathsPreview) ? 1 : 0, GUEDIni);
		GConfig->SetInt(*PersistentName, TEXT("ShowDistanceFog"), (pViewport->Actor->ShowFlags & SHOW_DistanceFog) ? 1 : 0, GUEDIni);
		GConfig->SetInt(*PersistentName, TEXT("ShowLightColorIcon"), (pViewport->Actor->ShowFlags & SHOW_LightColorIcon) ? 1 : 0, GUEDIni);
#endif
		GConfig->SetInt(*PersistentName, TEXT("ShowEventLines"), (pViewport->Actor->ShowFlags & SHOW_EventLines) ? 1 : 0, GUEDIni);
		GConfig->SetInt(*PersistentName, TEXT("OccludeLines"), (pViewport->Actor->ShowFlags & SHOW_OccludeLines) ? 1 : 0, GUEDIni);
		
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
			::MoveWindow( (HWND)pViewport->GetWindow(), R.Min.X, R.Min.Y, R.Width() & 0xFFFFFFFE, R.Height(), 1 );
			pViewport->Repaint( 1 );
		}

		InvalidateRect( hWnd, NULL, FALSE );
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
		ULevel* Level = GEditor->Level;
		UBOOL DisableBackdrop = 0;
		UBOOL DisableRealtimePreview = 0;
		
		switch( Command )
		{
			case IDMN_VF_REALTIME_PREVIEW:
				for (INT i = 0; i < Level->Model->Surfs.Num(); i++)
				{
					FBspSurf& Surf = Level->Model->Surfs(i);
					if ((Surf.PolyFlags & PF_FakeBackdrop) && Surf.Actor && Surf.Actor->Region.Zone && Surf.Actor->Region.Zone->IsA(ASkyZoneInfo::StaticClass()))
					{
						appMsgf(TEXT("Can't enable Realtime Preview, PF_Fakebackdrop is set in a Skyzone!"));
						DisableRealtimePreview = 1;
						break;
					}
				}
				if (!DisableRealtimePreview)
					pViewport->Actor->ShowFlags ^= SHOW_PlayerCtrl;
				UpdateWindow();
				pViewport->Repaint(1);
				break;

			case WM_SETCURRENTVIEWPORT:
			{
				// Send a notification message to the editor frame.  This of course relies on the 
				// window hierarchy not changing ... if it does, update this!
				const HWND hwndEditorFrame = GetParent(GetParent(GetParent(*this)));
				SendMessageW(hwndEditorFrame, WM_COMMAND, WM_SETCURRENTVIEWPORT, (LPARAM)pViewport);
				break;
			}

			case WM_VIEWPORT_UPDATEWINDOWFRAME:
				UpdateWindow();
				break;

			case ID_MapDynLight:
				pViewport->Actor->RendMap=REN_DynLight;
				UpdateWindow();
				pViewport->Repaint( 1 );
				break;

			case ID_MapPlainTex:
				pViewport->Actor->RendMap=REN_PlainTex;
				UpdateWindow();
				pViewport->Repaint( 1 );
				break;

			case ID_MapWire:
				pViewport->Actor->RendMap=REN_Wire;
				UpdateWindow();
				pViewport->Repaint( 1 );
				break;

			case ID_MapOverhead:
				pViewport->Actor->RendMap=REN_OrthXY;
				UpdateWindow();
				pViewport->Repaint( 1 );
				break;

			case ID_MapXZ:
				pViewport->Actor->RendMap=REN_OrthXZ;
				UpdateWindow();
				pViewport->Repaint( 1 );
				break;

			case ID_MapYZ:
				pViewport->Actor->RendMap=REN_OrthYZ;
				UpdateWindow();
				pViewport->Repaint( 1 );
				break;

			case ID_MapPolys:
				pViewport->Actor->RendMap=REN_Polys;
				UpdateWindow();
				pViewport->Repaint( 1 );
				break;

			case ID_MapPolyCuts:
				pViewport->Actor->RendMap=REN_PolyCuts;
				UpdateWindow();
				pViewport->Repaint( 1 );
				break;

			case ID_MapZones:
				pViewport->Actor->RendMap=REN_Zones;
				UpdateWindow();
				pViewport->Repaint( 1 );
				break;
			
#if ENGINE_VERSION==227
			case ID_LightingOnly:
				pViewport->Actor->RendMap = REN_LightingOnly;
				UpdateWindow();
				pViewport->Repaint(1);
				break;

			case ID_Normals:
				pViewport->Actor->RendMap = REN_Normals;
				UpdateWindow();
				pViewport->Repaint(1);
				break;
#endif

			case ID_Color16Bit:
				pViewport->RenDev->SetRes( pViewport->SizeX, pViewport->SizeY, 2, 0 );
				pViewport->Repaint( 1 );
				break;

			case ID_Color32Bit:
				pViewport->RenDev->SetRes( pViewport->SizeX, pViewport->SizeY, 4, 0 );
				pViewport->Repaint( 1 );
				break;

			case ID_ShowBackdrop:
				for (INT i = 0; i < Level->Model->Surfs.Num(); i++)
				{
					FBspSurf& Surf = Level->Model->Surfs(i);
					if ((Surf.PolyFlags & PF_FakeBackdrop) && Surf.Actor && Surf.Actor->Region.Zone && Surf.Actor->Region.Zone->IsA(ASkyZoneInfo::StaticClass()))
					{
						appMsgf(TEXT("Can't enable SHOW_Backdrop, PF_Fakebackdrop is set in a Skyzone!"));
						DisableBackdrop = 1;
						break;
					}
				}
				if (!DisableBackdrop)
					pViewport->Actor->ShowFlags ^= SHOW_Backdrop;

				pViewport->Repaint(1);
				break;

#if ENGINE_VERSION==227
			case ID_ShowRealTimeBackdrop:
				for (INT i = 0; i < Level->Model->Surfs.Num(); i++)
				{
					FBspSurf& Surf = Level->Model->Surfs(i);
					if ((Surf.PolyFlags & PF_FakeBackdrop) && Surf.Actor && Surf.Actor->Region.Zone && Surf.Actor->Region.Zone->IsA(ASkyZoneInfo::StaticClass()))
					{
						appMsgf(TEXT("Can't enable SHOW_RealTimeBackdrop, PF_Fakebackdrop is set in a Skyzone!"));
						DisableBackdrop = 1;
						break;
					}
				}
				if (!DisableBackdrop)
					pViewport->Actor->ShowFlags ^= SHOW_RealTimeBackdrop;
				pViewport->Repaint(1);
				break;

			case ID_ShowDistanceFog:
				pViewport->Actor->ShowFlags ^= SHOW_DistanceFog;
				pViewport->Repaint(1);
				break;

			case ID_LightColorIcon:
				pViewport->Actor->ShowFlags ^= SHOW_LightColorIcon;
				pViewport->Repaint(1);
				break;

			case ID_ShowPathsPreview:
				pViewport->Actor->ShowFlags ^= SHOW_PathsPreview;
				pViewport->Repaint(1);
				break;
#endif

			case ID_ActorsShow:
				pViewport->Actor->ShowFlags &= ~(SHOW_Actors | SHOW_ActorIcons | SHOW_ActorRadii);
				pViewport->Actor->ShowFlags |= SHOW_Actors; 
				pViewport->Repaint( 1 );
				break;

			case ID_ActorsIcons:
				pViewport->Actor->ShowFlags &= ~(SHOW_Actors | SHOW_ActorIcons | SHOW_ActorRadii); 
				pViewport->Actor->ShowFlags |= SHOW_Actors | SHOW_ActorIcons;
				pViewport->Repaint( 1 );
				break;

			case ID_ActorsRadii:
				pViewport->Actor->ShowFlags &= ~(SHOW_Actors | SHOW_ActorIcons | SHOW_ActorRadii); 
				pViewport->Actor->ShowFlags |= SHOW_Actors | SHOW_ActorRadii;
				pViewport->Repaint( 1 );
				break;

			case ID_ActorsHide:
				pViewport->Actor->ShowFlags &= ~(SHOW_Actors | SHOW_ActorIcons | SHOW_ActorRadii); 
				pViewport->Repaint( 1 );
				break;

			case ID_ShowPaths:
				pViewport->Actor->ShowFlags ^= SHOW_Paths;
				pViewport->Repaint( 1 );
				break;

			case ID_ShowCoords:
				pViewport->Actor->ShowFlags ^= SHOW_Coords;
				pViewport->Repaint( 1 );
				break;
				
			case ID_ShowTextureGrid:
				pViewport->Actor->ShowFlags ^= SHOW_TextureGrid;
				pViewport->Repaint( 1 );
				break;

			case ID_ShowBrush:
				pViewport->Actor->ShowFlags ^= SHOW_Brush;
				pViewport->Repaint( 1 );
				break;

			case ID_ShowMovingBrushes:
				pViewport->Actor->ShowFlags ^= SHOW_MovingBrushes;
				pViewport->Repaint( 1 );
				break;

			case ID_ShowMoverSurfaces:
				pViewport->Actor->ShowFlags ^= SHOW_MoverSurfaces;
				pViewport->Repaint( 1 );
				break;

			case ID_ShowMoverPath:
				pViewport->Actor->ShowFlags ^= SHOW_MoverPath;
				pViewport->Repaint( 1 );
				break;

			case ID_ShowEventLines:
				pViewport->Actor->ShowFlags ^= SHOW_EventLines;
				pViewport->Repaint( 1 );
				break;

			case ID_OccludeLines:
				pViewport->Actor->ShowFlags ^= SHOW_OccludeLines;
				pViewport->Repaint( 1 );
				break;

			default:
				if (Command >= IDMN_RD_SOFTWARE && Command <= IDMN_RD_CUSTOM9)
				{
					SwitchRenderer(Command, TRUE);
					VFToolbar->pViewport = pViewport;

					// Resize the viewport so it fits inside the frame
					OnSize(0, -1, -1);

					// Update the global viewport config
					ComputePositionData();

					// Update the menu
					UpdateWindow();

					// Repaint
					pViewport->Repaint(1);
				}
				else
				{
					WWindow::OnCommand(Command);
					UpdateWindow();
				}
				break;
		}
		unguard;
	}
	void OnKeyUp(WPARAM wParam, LPARAM lParam)
	{
		guard(WViewportFrame::OnKeyUp);
		WWindow* TopWindow = OwnerWindow;
		while (TopWindow->OwnerWindow)
			TopWindow = TopWindow->OwnerWindow;
		TopWindow->OnKeyUp(wParam, lParam);
		unguard;
	}
	void UpdateWindow( void )
	{
		guard(WViewportFrame::UpdateWindow);
		if( !pViewport ) 
		{
			Caption = TEXT("Viewport Frame");
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

#if ENGINE_VERSION==227
			case REN_LightingOnly:
				Caption = TEXT("Lighting Only");
				break;

			case REN_Normals:
				Caption = TEXT("Render Normals");
				break;
#endif

			default:
				Caption = TEXT("Unknown");
				break;
		}

		SetText(*Caption);
		VFToolbar->SetCaption( Caption );
		VFToolbar->UpdateButtons();

		InvalidateRect( hWnd, NULL, 1);

		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
