/*=============================================================================
	ButtonBar : Class for handling the left hand button bar
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>

extern WEditorFrame* GEditorFrame;
extern WDlgAddSpecial* GDlgAddSpecial;

#define dBUTTON_LEFT    MulDiv(2, DPIX, 96)
#define dBUTTON_WIDTH	MulDiv(32, DPIX, 96)
#define dBUTTON_HEIGHT	MulDiv(32, DPIY, 96)
#define dCAPTION_HEIGHT	MulDiv(20, DPIY, 96)

typedef struct {
	int ID;				// The command that is sent when the button is clicked
	HBITMAP hbm;		// The graphic that goes on the button
	FString Text;		// Differs based on Type (ToolTip or Label text)
	HWND hWnd;
	UBrushBuilder* Builder;
	UClass* Class;
	RECT rc;
	WWindow* pControl;
	FString ExecCommand;
	UBOOL bMeshMover;
} WBB_Button;

typedef struct {
	FString Name;
	int ID;			// Starting at IDMN_MOVER_TYPES
	UBOOL bMeshMover;
} WBB_MoverType;

extern int GScrollBarWidth;

// --------------------------------------------------------------
//
// WBUTTONGROUP
// - Holds a group of related buttons
// - can be expanded/contracted
//
// --------------------------------------------------------------

enum eBGSTATE {
	eBGSTATE_DOWN	= 0,
	eBGSTATE_UP		= 1
};

static HBRUSH hBtnBkg = CreateSolidBrush(RGB(128, 128, 128));

class WButtonGroup : public WWindow
{
	DECLARE_WINDOWCLASS(WButtonGroup,WWindow,Window)
	
	FString Caption;
	WButton* pExpandButton;
	TArray<WBB_Button> Buttons;
	HBITMAP hbmDown, hbmUp;
	int iState, LastX, LastY, InRow;
	WToolTip* ToolTipCtrl{};
	FString GroupName;
	TArray<WBB_MoverType> MoverTypes;
	WDlgBrushBuilder* pDBB;
	HBITMAP hbmCamSpeed[3]{};

	// Structors.
	WButtonGroup( FName InPersistentName, WWindow* InOwnerWindow )
	:	WWindow( InPersistentName, InOwnerWindow )
	{
		iState = eBGSTATE_DOWN;
		hbmDown = (HBITMAP)LoadImageA( hInstance, MAKEINTRESOURCEA(IDBM_DOWN_ARROW), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR );	check(hbmDown);
		hbmUp = (HBITMAP)LoadImageA( hInstance, MAKEINTRESOURCEA(IDBM_UP_ARROW), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR );	check(hbmUp);

		ScaleImageAndReplace(hbmDown);
		ScaleImageAndReplace(hbmUp);

		InRow = 2;
		LastX = dBUTTON_LEFT;
		LastY = dCAPTION_HEIGHT;
		pExpandButton = NULL;
		pDBB = NULL;
	}

	// WWindow interface.
	void OpenWindow()
	{
		guard(WButtonGroup::OpenWindow);
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

		HBRUSH hNew = CreateSolidBrush( RGB(128,128,128) );                                                                          
		HBRUSH hOld = (HBRUSH) SetClassLongPtr(hWnd, GCLP_HBRBACKGROUND, (LONG_PTR) hNew);                                           
		if (hOld)
			DeleteBrush(hOld);

		if(!GConfig->GetInt( TEXT("Groups"), *GroupName, iState, GUEDIni ))	iState = eBGSTATE_DOWN;
		UpdateButton();
		unguard;
	}
	void OnDestroy()
	{
		guard(WButtonGroup::OnDestroy);

		for( int x = 0 ; x < Buttons.Num() ; x++ )
		{
			::DeleteObject( Buttons(x).hbm );
			::DestroyWindow( Buttons(x).hWnd );
		}
		if( pExpandButton )
		{
			DestroyWindow( pExpandButton->hWnd );
			delete pExpandButton;
		}
		delete pDBB;
		DeleteObject(hbmDown);
		DeleteObject(hbmUp);
		if (hbmCamSpeed[0])
			DeleteObject(hbmCamSpeed[0]);
		if (hbmCamSpeed[1])
			DeleteObject(hbmCamSpeed[1]);
		if (hbmCamSpeed[2])
			DeleteObject(hbmCamSpeed[2]);
		delete ToolTipCtrl;
		WWindow::OnDestroy();
		unguard;
	}
	INT OnSetCursor()
	{
		guard(WButtonGroup::OnSetCursor);
		WWindow::OnSetCursor();
		SetCursor(LoadCursorW(NULL, IDC_CROSS));
		return 0;
		unguard;
	}
	void OnCreate()
	{
		guard(WButtonGroup::OnCreate);

		ToolTipCtrl = new WToolTip(this);
		ToolTipCtrl->OpenWindow();

		pExpandButton = new WButton( this, IDPB_EXPAND, FDelegate(this, (TDelegate)&WButtonGroup::OnExpandButton) );
		pExpandButton->OpenWindow( 1, 0, 0, MulDiv(19, DPIX, 96), MulDiv(19, DPIY, 96), TEXT(""), 1, BS_OWNERDRAW );
		UpdateButton();

		WWindow::OnCreate();
		unguard;
	}
	virtual HBRUSH OnCtlColorBtn(HDC wParam, HWND lParam)                                                                            
	{                                                                                                                                
		return hBtnBkg;                                                                                                              
	}
	void OnExpandButton()
	{
		guard(WButtonGroup::OnExpandButton);
		iState = (iState == eBGSTATE_DOWN) ? eBGSTATE_UP : eBGSTATE_DOWN;
		SendMessageW( OwnerWindow->hWnd, WM_COMMAND, WM_EXPANDBUTTON_CLICKED, 0L );
		UpdateButton();
		unguard;
	}
	void UpdateButton()
	{
		guard(WButtonGroup::UpdateButton);
		pExpandButton->SetBitmap( (iState == eBGSTATE_DOWN) ? hbmUp : hbmDown );
		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WButtonGroup::OnSize);
		WWindow::OnSize(Flags, NewX, NewY);
		RECT rect;
		::GetClientRect( hWnd, &rect );

		::MoveWindow( pExpandButton->hWnd, rect.right - MulDiv(30, DPIX, 96), 0, 
			MulDiv(19, DPIX, 96), 
			MulDiv(19, DPIY, 96), 1 );

		InRow = (rect.right - rect.left) / dBUTTON_WIDTH;

		PositionChildControls();

		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void PositionChildControls()
	{
		guard(WButtonGroup::PositionChildControls);

		LastX = dBUTTON_LEFT;
		LastY = dCAPTION_HEIGHT;

		for (int x = 0; x < Buttons.Num(); x++)
		{
			::MoveWindow(Buttons(x).hWnd, LastX, LastY, dBUTTON_WIDTH, dBUTTON_HEIGHT, 1);
			LastX += dBUTTON_WIDTH;
			if (LastX >= dBUTTON_WIDTH * InRow)
			{
				LastX = dBUTTON_LEFT;
				LastY += dBUTTON_HEIGHT;
			}
		}

		unguard;
	}
	void OnPaint()
	{
		guard(WButtonGroup::OnPaint);
		PAINTSTRUCT PS;
		HDC hDC = BeginPaint( *this, &PS );

		RECT rect;
		::GetClientRect( hWnd, &rect );

		MyDrawEdge( hDC, &rect, 1 );

		rect.top += MulDiv(6, DPIY, 96);		rect.bottom = rect.top + MulDiv(3, DPIY, 96);
		rect.left += MulDiv(3, DPIX, 96);		rect.right -= MulDiv(32, DPIX, 96);
		MyDrawEdge( hDC, &rect, 1 );
		rect.top += MulDiv(6, DPIY, 96);		rect.bottom = rect.top + MulDiv(3, DPIY, 96);
		MyDrawEdge( hDC, &rect, 1 );

		EndPaint( *this, &PS );

		unguard;
	}
	// Adds a button onto the bar.  The button will be positioned automatically.
	void AddButton( int _iID, UBOOL bAutoCheck, FString BMPFilename, FString Text, UClass* Class, FString ExecCommand = TEXT("") )
	{
		guard(WButtonGroup::AddButton);

		// Add the button into the array and set it up
		new(Buttons)WBB_Button;
		WBB_Button* pWBButton = &(Buttons(Buttons.Num() - 1));
		check(pWBButton);

		if( BMPFilename.Len() )
		{
			FString Filename = *(FString::Printf(TEXT("editorres\\%ls.bmp"), *BMPFilename));
			pWBButton->hbm = (HBITMAP)LoadImageA( hInstance, appToAnsi( *Filename ), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE );
			ScaleImageAndReplace(pWBButton->hbm);
			if (!pWBButton->hbm)
			{
				pWBButton->hbm = (HBITMAP)LoadImageA(hInstance, "BBGeneric", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
				ScaleImageAndReplace(pWBButton->hbm);
			}
		}
		else
			pWBButton->hbm = NULL;

		pWBButton->ExecCommand = ExecCommand;
		pWBButton->ID = _iID;
		pWBButton->Text = Text;
		pWBButton->Class = Class;

//		if( pWBButton->ID >= IDPB_BRUSH_BUILDERS )
//			pWBButton->Builder = ConstructObject<UBrushBuilder>(Class);
//		else
			pWBButton->Builder = NULL;

		// Create the physical button/label
		WCheckBox* pButton = new WCheckBox( this, _iID );
		pWBButton->pControl = pButton;
		pButton->OpenWindow( 1, 0, 0, dBUTTON_WIDTH, dBUTTON_HEIGHT, NULL, bAutoCheck, 1, BS_PUSHLIKE | BS_OWNERDRAW );
		pButton->SetBitmap( pWBButton->hbm );

		pWBButton->hWnd = pButton->hWnd;

		ToolTipCtrl->AddTool( pButton->hWnd, Text, _iID );

		unguard;
	}
	int GetFullHeight()
	{
		guard(WButtonGroup::GetFullHeight);

		if( iState == eBGSTATE_DOWN )
			return dCAPTION_HEIGHT + (((Buttons.Num() + InRow - 1) / InRow) * dBUTTON_HEIGHT);
		else
			return dCAPTION_HEIGHT;

		unguard;
	}
	void OnCommand( INT Command )
	{
		guard(WButtonGroup::OnCommand);

		if( Command >= IDMN_MOVER_TYPES && Command <= IDMN_MOVER_TYPES_MAX )
		{
			// Figure out which mover was chosen
			for( int x = 0 ; x < MoverTypes.Num() ; x++ )
			{
				WBB_MoverType* pwbbmt = &(MoverTypes(x));
				if( pwbbmt->ID == Command )
				{
					GEditor->Exec( *(FString::Printf(TEXT("BRUSH ADDMOVER CLASS=%ls MESH=%ls MM=%ls"), *pwbbmt->Name, *GBrowserMesh->GetCurrentMeshName(), pwbbmt->bMeshMover ? TEXT("true") : TEXT("false")) ) );
					break;
				}
			}
		}

		switch( Command )
		{
			case WM_BB_RCLICK:
				{
					switch( LastlParam ) {

						case IDPB_ADD_MOVER:
						{
							// Create a context menu with all the available kinds of movers on it.
							CreateMoverTypeList();

							HMENU menu = CreatePopupMenu();
							MENUITEMINFOA mif;

							mif.cbSize = sizeof(MENUITEMINFO);
							mif.fMask = MIIM_TYPE | MIIM_ID;
							mif.fType = MFT_STRING;

							for( int x = 0 ; x < MoverTypes.Num() ; x++ )
							{
								WBB_MoverType* pwbbmt = &(MoverTypes(x));

								mif.dwTypeData = const_cast<ANSICHAR*>(appToAnsi( *(pwbbmt->Name) ));
								mif.wID = pwbbmt->ID;

								InsertMenuItemA( menu, 99999, FALSE, &mif );
							}

							POINT point;
							::GetCursorPos(&point);
							TrackPopupMenu( menu,
								TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
								point.x, point.y, 0,
								hWnd, NULL);
						}
						break;

						default:
						{
							// Right clicking a brush builder button will bring up it's editable fields.
							WBB_Button* pwbb;

							for( int x = 0 ; x < Buttons.Num() ; x++ )
							{
								pwbb = &(Buttons(x));
								if( pwbb->ID == LastlParam
										&& pwbb->Builder)
								{
									delete pDBB;
									pDBB = new WDlgBrushBuilder( NULL, this, pwbb->Builder );
									pDBB->DoModeless();
									break;
								}
							}
						}
						break;
					}
				}
				break;

			default:
				switch( HIWORD(Command) ) {

					case BN_CLICKED:
						ButtonClicked(LOWORD(Command));
						break;

					default:
						WWindow::OnCommand(Command);
						break;
				}
				break;
		}

		unguard;
	}
	void CreateMoverTypeList(void)
	{
		guard(WButtonGroup::CreateMoverTypeList);

		MoverTypes.Empty();

		int ID = IDMN_MOVER_TYPES;

		// Add the normal mover and mesh mover on top
		WBB_MoverType* pwbbmt = new(MoverTypes)WBB_MoverType();
		pwbbmt->ID = ID;
		pwbbmt->Name = TEXT("Mover");
		pwbbmt->bMeshMover = 0;
		ID++;

#if ENGINE_VERSION==227		
		pwbbmt = new(MoverTypes)WBB_MoverType();
		pwbbmt->ID = ID;
		pwbbmt->Name = TEXT("Mover [mesh]");
		pwbbmt->bMeshMover = 1;
		ID++;
#endif

		// Now add the child classes of "mover"
		for (TObjectIterator<UClass> It; It; ++It)
		{
			if (It->IsChildOf(AMover::StaticClass()) && *It != AMover::StaticClass())
			{
				pwbbmt = new(MoverTypes)WBB_MoverType();
				pwbbmt->Name = It->GetName();
				pwbbmt->ID = ID;
				pwbbmt->bMeshMover = 0;
				ID++;
#if ENGINE_VERSION==227
				pwbbmt = new(MoverTypes)WBB_MoverType();
				pwbbmt->Name = FString::Printf(TEXT("%ls [mesh]"), It->GetName());
				pwbbmt->ID = ID; // Negative ID to indicate mesh mover.
				pwbbmt->bMeshMover = 1;
				ID++;
#endif
			}
		}

		unguard;
	}
	// Searches the list for the button that was clicked, and executes the appropriate command.
	void ButtonClicked( int ID )
	{
		guard(WButtonGroup::ButtonClicked);

		ID = ::Abs(ID);
		if( ID >= IDPB_USER_DEFINED && ID <= IDPB_USER_DEFINED_MAX )
		{
			for( int x = 0 ; x < Buttons.Num() ; x++ )
			{
				WBB_Button* pwbb = &(Buttons(x));
				if( pwbb->ID == ID )
				{
					GEditor->Exec( *(pwbb->ExecCommand) );
					return;
				}
			}
		}

		switch( ID )
		{
			case IDPB_CAMERA_SPEED:
				{
					WCheckBox* pButton = GetButton(IDPB_CAMERA_SPEED);	check(pButton);
					if( GEditor->MovementSpeed == 1 )
						GEditor->Exec( TEXT("MODE SPEED=4") );
					else if( GEditor->MovementSpeed == 4 )
						GEditor->Exec( TEXT("MODE SPEED=16") );
					else
						GEditor->Exec( TEXT("MODE SPEED=1") );
				}
				break;

			case IDPB_MODE_CAMERA:
				GEditor->edcamSetMode( EM_ViewportMove );
				break;

			case IDPB_MODE_SHEER:
				GEditor->edcamSetMode(EM_BrushSheer);
				break;

			case IDPB_MODE_SCALE:
				GEditor->edcamSetMode( EM_BrushScale );
				break;

			case IDPB_MODE_STRETCH:
				GEditor->edcamSetMode( EM_BrushStretch );
				break;

			case IDPB_MODE_SNAPSCALE:
				GEditor->edcamSetMode( EM_BrushSnap );
				break;

			case IDPB_MODE_ROTATE:
				GEditor->edcamSetMode( EM_BrushRotate );
				break;

			case IDPB_MODE_VERTEX_EDIT:
				GEditor->edcamSetMode( EM_VertexEdit );
				break;

			case IDPB_MODE_BRUSH_CLIP:
				GEditor->edcamSetMode( EM_BrushClip );
				break;

			//case IDPB_MODE_FACE_DRAG:
				//GEditor->edcamSetMode( EM_FaceDrag );
				//break;

			case IDPB_SHOW_SELECTED:
				GEditor->Exec( TEXT("ACTOR HIDE UNSELECTED") );
				GEditor->RedrawLevel( GEditor->Level );
				break;

			case IDPB_HIDE_SELECTED:
				GEditor->Exec( TEXT("ACTOR HIDE SELECTED") );
				break;

			case IDPB_SELECT_INSIDE:
				GEditor->Exec(TEXT("ACTOR SELECT INSIDE"));
				break;
			
			case IDPB_SELECT_ALL:
				GEditor->Exec(TEXT("ACTOR SELECT ALL"));
				break;

			case IDPB_SHOW_ALL:
				GEditor->Exec( TEXT("ACTOR UNHIDE ALL") );
				break;

			case IDPB_INVERT_SELECTION:
				GEditor->Exec( TEXT("ACTOR SELECT INVERT") );
				break;

			case IDPB_BRUSHCLIP:
				GEditor->Exec( TEXT("BRUSHCLIP") );
				GEditor->RedrawLevel( GEditor->Level );
				break;

			case IDPB_BRUSHCLIP_SPLIT:
				GEditor->Exec( TEXT("BRUSHCLIP SPLIT") );
				GEditor->RedrawLevel( GEditor->Level );
				break;

			case IDPB_BRUSHCLIP_FLIP:
				GEditor->Exec( TEXT("BRUSHCLIP FLIP") );
				GEditor->RedrawLevel( GEditor->Level );
				break;

			case IDPB_BRUSHCLIP_DELETE:
				GEditor->Exec( TEXT("BRUSHCLIP DELETE") );
				GEditor->RedrawLevel( GEditor->Level );
				break;

			case IDPB_TEXTURE_PAN:
				GEditor->edcamSetMode( EM_TexturePan );
				break;

			case IDPB_TEXTURE_ROTATE:
				GEditor->edcamSetMode( EM_TextureRotate );
				break;

			case IDPB_MODE_ADD:
				GEditor->Exec( TEXT("BRUSH ADD") );
				break;

			case IDPB_MODE_SUBTRACT:
				GEditor->Exec( TEXT("BRUSH SUBTRACT") );
				break;

			case IDPB_MODE_INTERSECT:
				GEditor->Exec( TEXT("BRUSH FROM INTERSECTION") );
				break;

			case IDPB_MODE_DEINTERSECT:
				GEditor->Exec( TEXT("BRUSH FROM DEINTERSECTION") );
				break;

			case IDPB_ADD_SPECIAL:
				{
					if( !GDlgAddSpecial )
					{
						GDlgAddSpecial = new WDlgAddSpecial( NULL, this );
						GDlgAddSpecial->DoModeless();
					}
					else
						GDlgAddSpecial->Show(1);
				}
				break;

			case IDPB_ADD_MOVER:
				GEditor->Exec( TEXT("BRUSH ADDMOVER") );
				break;

			case IDPB_ALIGN_VIEWPORTCAM:
				GEditor->Exec(TEXT("CAMERA ALIGN"));
				break;

			default:
				// A brush builder must have been clicked.  Loop through the
				// list and see which one.
				WBB_Button* pwbb;

				for( int x = 0 ; x < Buttons.Num() ; x++ )
				{
					pwbb = &(Buttons(x));
					if( pwbb->ID == ID )
					{
						check( pwbb->Builder );
						UBOOL GIsSavedScriptableSaved = 1;
						Exchange(GIsScriptable,GIsSavedScriptableSaved);
						pwbb->Builder->eventBuild();
						Exchange(GIsScriptable,GIsSavedScriptableSaved);
						
						GEditor->Exec(TEXT("LEVEL REDRAW"));
					}
				}
				break;
		}

		UpdateButtons();

		unguard;
	}
	// Updates the states of the buttons to match editor settings.
	void UpdateButtons()
	{
		guard(WButtonGroup::UpdateButtons);

		if( GetDlgItem( hWnd, IDPB_MODE_CAMERA ) )	// Make sure we're in the mode group before bothering
		{
//			WCheckBox* pcheckbox;

			GetButton(IDPB_MODE_CAMERA)->SetCheck(GEditor->Mode == EM_ViewportMove);
			GetButton(IDPB_MODE_SHEER)->SetCheck(GEditor->Mode == EM_BrushSheer);
			GetButton(IDPB_MODE_STRETCH)->SetCheck(GEditor->Mode == EM_BrushStretch);
			GetButton(IDPB_MODE_SCALE)->SetCheck(GEditor->Mode == EM_BrushScale);
			GetButton(IDPB_MODE_SNAPSCALE)->SetCheck(GEditor->Mode == EM_BrushSnap);
			GetButton(IDPB_MODE_ROTATE)->SetCheck(GEditor->Mode == EM_BrushRotate);
			GetButton(IDPB_TEXTURE_PAN)->SetCheck(GEditor->Mode == EM_TexturePan);
			GetButton(IDPB_TEXTURE_ROTATE)->SetCheck(GEditor->Mode == EM_TextureRotate);
			GetButton(IDPB_MODE_VERTEX_EDIT)->SetCheck(GEditor->Mode == EM_VertexEdit);
			GetButton(IDPB_MODE_BRUSH_CLIP)->SetCheck(GEditor->Mode == EM_BrushClip);
			//GetButton( IDPB_MODE_FACE_DRAG )->SetCheck( GEditor->Mode==EM_FaceDrag );

			// Force all the buttons to paint themselves
			InvalidateRect(GetDlgItem(hWnd, IDPB_MODE_CAMERA), NULL, 1);
			InvalidateRect(GetDlgItem(hWnd, IDPB_MODE_SHEER), NULL, 1);
			InvalidateRect(GetDlgItem(hWnd, IDPB_MODE_STRETCH), NULL, 1);
			InvalidateRect(GetDlgItem(hWnd, IDPB_MODE_SCALE), NULL, 1);
			InvalidateRect(GetDlgItem(hWnd, IDPB_MODE_SNAPSCALE), NULL, 1);
			InvalidateRect(GetDlgItem(hWnd, IDPB_MODE_ROTATE), NULL, 1);
			InvalidateRect(GetDlgItem(hWnd, IDPB_TEXTURE_PAN), NULL, 1);
			InvalidateRect(GetDlgItem(hWnd, IDPB_TEXTURE_ROTATE), NULL, 1);
			InvalidateRect(GetDlgItem(hWnd, IDPB_MODE_VERTEX_EDIT), NULL, 1);
			InvalidateRect(GetDlgItem(hWnd, IDPB_MODE_BRUSH_CLIP), NULL, 1);
			//InvalidateRect( GetDlgItem( hWnd, IDPB_MODE_FACE_DRAG ), NULL, 1 );
		}

		if( GetDlgItem( hWnd, IDPB_CAMERA_SPEED ) )
		{
			WCheckBox* pButton = GetButton(IDPB_CAMERA_SPEED);

			if (!hbmCamSpeed[0])
			{
				hbmCamSpeed[0] = (HBITMAP)LoadImageA( hInstance, MAKEINTRESOURCEA(IDBM_CAMSPEED1), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR );	check(hbmCamSpeed[0]);
				hbmCamSpeed[1] = (HBITMAP)LoadImageA( hInstance, MAKEINTRESOURCEA(IDBM_CAMSPEED2), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR );	check(hbmCamSpeed[1]);
				hbmCamSpeed[2] = (HBITMAP)LoadImageA( hInstance, MAKEINTRESOURCEA(IDBM_CAMSPEED3), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR );	check(hbmCamSpeed[2]);

				ScaleImageAndReplace(hbmCamSpeed[0]);
				ScaleImageAndReplace(hbmCamSpeed[1]);
				ScaleImageAndReplace(hbmCamSpeed[2]);
			}

			if( GEditor->MovementSpeed == 1 )
				pButton->SetBitmap( hbmCamSpeed[0] );
			else if( GEditor->MovementSpeed == 4 )
				pButton->SetBitmap( hbmCamSpeed[1] );
			else
				pButton->SetBitmap( hbmCamSpeed[2] );
			InvalidateRect( pButton->hWnd, NULL, 1 );
		}

		unguard;
	}
	WCheckBox* GetButton( INT InID )
	{
		guard(WButtonGroup::GetButton);
		for( int x = 0 ; x < Buttons.Num() ; x++ )
		{
			WBB_Button* pwbb = &(Buttons(x));
			if(pwbb->ID == InID )
				return (WCheckBox*)pwbb->pControl;
		}

		check(0);	// this should never happen
		return NULL;
		unguard;
	}
	void RefreshBuilders()
	{
		guard(WButtonGroup::RefreshBuilders);
		
		SetRedraw(false);

		// Delete brush builder window
		if ( pDBB )
		{
			delete pDBB;
			pDBB = nullptr;
		}

		INT Added = 0;

		// See if brush builders need a button
		for ( INT i=0; i<GEditor->BrushBuilders.Num(); i++)
		{
			UBrushBuilder* NeedBuilder = GEditor->BrushBuilders(i);
			for( INT b=0; b<Buttons.Num(); b++)
			{
				//WBB_Button& pwbb = Buttons(b);
				if( Buttons(b).Class == NeedBuilder->GetClass() )
				{
					Buttons(b).Builder = NeedBuilder;
					NeedBuilder = nullptr;
					break;
				}
			}
			if ( NeedBuilder )
			{
				INT ID = Buttons.Num() ? Buttons.Last().ID + 1 : IDPB_BRUSH_BUILDERS;
				AddButton( ID, 0, NeedBuilder->BitmapFilename, NeedBuilder->ToolTip, NeedBuilder->GetClass() );
				Buttons.Last().Builder = NeedBuilder;
				Added++;
			}
		}
		
		SetRedraw(true);

		if ( Added )
		{
			OnExpandButton(); // Higor: lol
			OnExpandButton();
		}
		unguard;
	}
};

// --------------------------------------------------------------
//
// WBUTTONBAR
//
// --------------------------------------------------------------

class WButtonBar : public WWindow
{
	DECLARE_WINDOWCLASS(WButtonBar,WWindow,Window)

	TArray<WButtonGroup> Groups;
	int LastX{}, LastY{};
	WVScrollBar* pScrollBar;
	int iScroll;

	// Structors.
	WButtonBar( FName InPersistentName, WWindow* InOwnerWindow )
	:	WWindow( InPersistentName, InOwnerWindow )
	{
		pScrollBar = NULL;
		iScroll = 0;
	}

	// WWindow interface.
	void OpenWindow()
	{
		guard(WButtonBar::OpenWindow);
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
	void RefreshScrollBar( void )
	{
		guard(WButtonBar::RefreshScrollBar);

		if( !pScrollBar ) return;

		RECT rect;
		::GetClientRect( hWnd, &rect );

		if( (rect.bottom < GetHeightOfAllGroups()
				&& !IsWindowEnabled( pScrollBar->hWnd ) )
				|| (rect.bottom >= GetHeightOfAllGroups()
				&& IsWindowEnabled( pScrollBar->hWnd ) ) )
			iScroll = 0;

		// Set the scroll bar to have a valid range.
		//
		SCROLLINFO si;
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_DISABLENOSCROLL | SIF_RANGE | SIF_POS | SIF_PAGE;
		si.nPage = rect.bottom;
		si.nMin = 0;
		si.nMax = GetHeightOfAllGroups();
		si.nPos = iScroll;
		iScroll = SetScrollInfo( pScrollBar->hWnd, SB_CTL, &si, TRUE );

		EnableWindow( pScrollBar->hWnd, (rect.bottom < GetHeightOfAllGroups()) );
		PositionChildControls();

		unguard;
	}
	int GetHeightOfAllGroups()
	{
		guard(WButtonBar::GetHeightOfAllGroups);

		int Height = 0;

		for( int x = 0 ; x < Groups.Num() ; x++ )
			Height += Groups(x).GetFullHeight();

		return Height;

		unguard;
	}
	void OnDestroy()
	{
		guard(WButtonBar::OnDestroy);
		delete pScrollBar;
		WWindow::OnDestroy();
		unguard;
	}
	INT OnSetCursor()
	{
		guard(WButtonBar::OnSetCursor);
		WWindow::OnSetCursor();
		SetCursor(LoadCursorW(NULL, IDC_ARROW));
		return 0;
		unguard;
	}
	// Adds a button group.
	WButtonGroup* AddGroup( FString GroupName )
	{
		guard(WButtonBar::AddGroup);
		new(Groups)WButtonGroup( TEXT(""), this );
		WButtonGroup* pbuttongroup = &(Groups(Groups.Num() - 1));
		pbuttongroup->GroupName = GroupName;
		check(pbuttongroup);
		return pbuttongroup;
		unguard;
	}
	WButtonGroup* GetGroup( FString GroupName)
	{
		guard(WButtonBar::GetGroup);
		for ( INT i=0; i<Groups.Num(); i++)
			if ( !appStricmp( *Groups(i).GroupName, *GroupName) )
				return &Groups(i);
		return nullptr;
		unguard;
	}
	// Dispatch brush builder reset to 'Builders' group
	void RefreshBuilders()
	{
		guard(WButtonBar::RecreateBuilders);

		WButtonGroup* gBuilders = GetGroup( TEXT("Builders") );
		if ( gBuilders )
			gBuilders->RefreshBuilders();

		unguard;
	}
	void OnCreate()
	{
		guard(WButtonBar::OnCreate);
		WWindow::OnCreate();
		
		SetRedraw(false);

		HBRUSH hNew = CreateSolidBrush( RGB(128,128,128) );
		HBRUSH hOld = (HBRUSH) SetClassLongPtr(hWnd, GCLP_HBRBACKGROUND, (LONG_PTR) hNew);
		if (hOld)
			DeleteBrush(hOld);

		pScrollBar = new WVScrollBar( this, IDSB_SCROLLBAR2 );
		pScrollBar->OpenWindow( 1, 0, 0, MulDiv(320, DPIX, 96), MulDiv(200, DPIY, 96) );

		// Load the buttons onto the bar.
		WButtonGroup* pGroup;

		pGroup = AddGroup( TEXT("Modes") );
		pGroup->OpenWindow();
		pGroup->AddButton(IDPB_MODE_CAMERA, 0, TEXT("ModeCamera"), TEXT("Camera Movement"), NULL);
		pGroup->AddButton(IDPB_MODE_VERTEX_EDIT, 0, TEXT("ModeVertex"), TEXT("Vertex Editing"), NULL);
		pGroup->AddButton(IDPB_MODE_SHEER, 0, TEXT("ModeSheer"), TEXT("Brush Sheering"), NULL);
		pGroup->AddButton(IDPB_MODE_SCALE, 0, TEXT("ModeScale"), TEXT("Actor Scaling"), NULL);
		pGroup->AddButton(IDPB_MODE_STRETCH, 0, TEXT("ModeStretch"), TEXT("Actor Stretching"), NULL);
		pGroup->AddButton(IDPB_MODE_SNAPSCALE, 0, TEXT("ModeSnapScale"), TEXT("Snapped Brush Scaling"), NULL);
		pGroup->AddButton(IDPB_MODE_ROTATE, 0, TEXT("ModeRotate"), TEXT("Brush Rotate"), NULL);
		pGroup->AddButton(IDPB_TEXTURE_PAN, 0, TEXT("TexturePan"), TEXT("Texture Pan"), NULL);
		pGroup->AddButton(IDPB_TEXTURE_ROTATE, 0, TEXT("TextureRotate"), TEXT("Texture Rotate"), NULL);
		pGroup->AddButton(IDPB_MODE_BRUSH_CLIP, 0, TEXT("ModeBrushClip"), TEXT("Brush Clipping"), NULL);
		//pGroup->AddButton( IDPB_MODE_FACE_DRAG, 0, TEXT("ModeFaceDrag"), TEXT("Face Drag"), NULL );

		pGroup = AddGroup( TEXT("Clipping") );
		pGroup->OpenWindow();
		pGroup->AddButton(IDPB_BRUSHCLIP, 0, TEXT("BrushClip"), TEXT("Clip Selected Brushes"), NULL);
		pGroup->AddButton(IDPB_BRUSHCLIP_SPLIT, 0, TEXT("BrushClipSplit"), TEXT("Split Selected Brushes"), NULL);
		pGroup->AddButton(IDPB_BRUSHCLIP_FLIP, 0, TEXT("BrushClipFlip"), TEXT("Flip Clipping Normal"), NULL);
		pGroup->AddButton(IDPB_BRUSHCLIP_DELETE, 0, TEXT("BrushClipDelete"), TEXT("Delete Clipping Markers"), NULL);

		//int ID = IDPB_BRUSH_BUILDERS;

		pGroup = AddGroup( TEXT("Builders") );
		pGroup->OpenWindow();
		pGroup->RefreshBuilders();

		pGroup = AddGroup( TEXT("CSG") );
		pGroup->OpenWindow();
		pGroup->AddButton( IDPB_MODE_ADD, 0, TEXT("ModeAdd"), TEXT("Add"), NULL );
		pGroup->AddButton( IDPB_MODE_SUBTRACT, 0, TEXT("ModeSubtract"), TEXT("Subtract"), NULL );
		pGroup->AddButton( IDPB_MODE_INTERSECT, 0, TEXT("ModeIntersect"), TEXT("Intersect"), NULL );
		pGroup->AddButton( IDPB_MODE_DEINTERSECT, 0, TEXT("ModeDeintersect"), TEXT("Deintersect"), NULL );
		pGroup->AddButton( IDPB_ADD_SPECIAL, 0, TEXT("AddSpecial"), TEXT("Add Special Brush"), NULL );
		pGroup->AddButton( IDPB_ADD_MOVER, 0, TEXT("AddMover"), TEXT("Add Mover Brush (right click for all mover types)"), NULL );

		pGroup = AddGroup(TEXT("Misc"));
		pGroup->OpenWindow();
		pGroup->AddButton(IDPB_SHOW_SELECTED, 0, TEXT("ShowSelected"), TEXT("Show Selected Actors Only"), NULL);
		pGroup->AddButton(IDPB_HIDE_SELECTED, 0, TEXT("HideSelected"), TEXT("Hide Selected Actors"), NULL);
		pGroup->AddButton(IDPB_SELECT_INSIDE, 0, TEXT("SelectInside"), TEXT("Select Inside Actors"), NULL);
		pGroup->AddButton(IDPB_SELECT_ALL, 0, TEXT("SelectAll"), TEXT("Select All Actors"), NULL);
		pGroup->AddButton(IDPB_INVERT_SELECTION, 0, TEXT("InvertSelections"), TEXT("Invert Selection"), NULL);
		pGroup->AddButton(IDPB_SHOW_ALL, 0, TEXT("ShowAll"), TEXT("Show All Actors"), NULL);
		pGroup->AddButton(IDPB_CAMERA_SPEED, 0, TEXT("ModeCamera"), TEXT("Change Camera Speed"), NULL);
		pGroup->AddButton(IDPB_ALIGN_VIEWPORTCAM, 0, TEXT("AlignCameras"), TEXT("Align Viewport Cameras to 3D Viewport Camera"), NULL);

		FString Section, ButtonDef, ButtonName;
		for (INT i = 0; i < 30; i++)
		{
			Section = i ? FString::Printf(TEXT("UserDefinedGroup%i"), i) : TEXT("UserDefinedGroup");
			int NumButtons;
			if(!GConfig->GetInt( *Section, TEXT("NumButtons"), NumButtons, GUEDIni ))	NumButtons = 0;

			if (NumButtons || !i) // always show first group, even if no buttons
			{
				pGroup = AddGroup( Section );
				pGroup->OpenWindow();
			}

			if( NumButtons )
			{
				for( int x = 0 ; x < NumButtons ; x++ )
				{
					ButtonName = FString::Printf(TEXT("Button%d"), x);

					TArray<FString> Fields;
					GConfig->GetString( *Section, *ButtonName, ButtonDef, GUEDIni );
					ParseStringToArray( TEXT(","), ButtonDef, &Fields );

					pGroup->AddButton( IDPB_USER_DEFINED + x, 0, *Fields(1), *Fields(0), NULL, *Fields(2) );
				}
			}
		}

		PositionChildControls();
		UpdateButtons();
		
		SetRedraw(true);

		unguard;
	}
	void PositionChildControls()
	{
		guard(WButtonBar::PositionChildControls);

		RECT rect;
		int LastY = -iScroll;

		::GetClientRect( hWnd, &rect );

		// Figure out where each buttongroup window should go.
		for( int x = 0 ; x < Groups.Num() ; x++ )
		{
			::MoveWindow( Groups(x).hWnd, 0, LastY, rect.right - GScrollBarWidth, Groups(x).GetFullHeight(), 1 );
			LastY += Groups(x).GetFullHeight();
		}

		::MoveWindow( pScrollBar->hWnd, rect.right - GScrollBarWidth, 0, GScrollBarWidth, rect.bottom, 1 );

		unguard;
	}
	virtual void OnVScroll( WPARAM wParam, LPARAM lParam )
	{
		guard(WButtonBar::OnVScroll);

		if( (HWND)lParam == pScrollBar->hWnd || lParam == WM_MOUSEWHEEL ) {

			switch(LOWORD(wParam)) {

				case SB_LINEUP:
					iScroll -= 64;
					RefreshScrollBar();
					PositionChildControls();
					break;

				case SB_LINEDOWN:
					iScroll += 64;
					RefreshScrollBar();
					PositionChildControls();
					break;

				case SB_PAGEUP:
					iScroll -= 256;
					RefreshScrollBar();
					PositionChildControls();
					break;

				case SB_PAGEDOWN:
					iScroll += 256;
					RefreshScrollBar();
					PositionChildControls();
					break;

				case SB_THUMBTRACK:
					iScroll = (short int)HIWORD(wParam);
					RefreshScrollBar();
					PositionChildControls();
					break;
			}
		}
		unguard;
	}
	void OnPaint()
	{
		guard(WButtonBar::OnPaint);
		PAINTSTRUCT PS;
		HDC hDC = BeginPaint( *this, &PS );

		FRect Rect = GetClientRect();
		MyDrawEdge( hDC, Rect, 1 );

		EndPaint( *this, &PS );
		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WButtonBar::OnSize);
		WWindow::OnSize(Flags, NewX, NewY);
		PositionChildControls();
		RefreshScrollBar();
		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void OnCommand( INT Command )
	{
		guard(WButtonBar::OnCommand);

		switch( Command )
		{
			case WM_EXPANDBUTTON_CLICKED:
			{
				RefreshScrollBar();
				PositionChildControls();
			}
			break;

			default:
				WWindow::OnCommand(Command);
				break;
		}

		unguard;
	}
	void UpdateButtons()
	{
		guard(WButtonBar::UpdateButtons);

		for( int x = 0 ; x < Groups.Num() ; x++ )
			Groups(x).UpdateButtons();

		unguard;
	}
};
/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
