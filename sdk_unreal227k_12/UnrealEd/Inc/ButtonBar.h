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
void ParseStringToArray( const TCHAR* pchDelim, FString String, TArray<FString>* _pArray);

#define dBUTTON_WIDTH	MulDiv(32, DPIX, 96)
#define dBUTTON_HEIGHT	MulDiv(32, DPIY, 96)
#define dCAPTION_HEIGHT	MulDiv(20, DPIY, 96)

struct WBB_Button
{
	INT ID;				// The command that is sent when the button is clicked
	HBITMAP hbm;		// The graphic that goes on the button
	FString Text;		// Differs based on Type (ToolTip or Label text)
	HWND hWnd;
	UBrushBuilder* Builder;
	UClass* Class;
	RECT rc;
	WCheckBox* pControl;
	FString ExecCommand;
	FCustomBitmap* ImgBitmap;
	INT edMode;

	WBB_Button(INT InID, const TCHAR* Txt, UClass* C, const TCHAR* ExecStr)
		: ID(InID), hbm(NULL), Text(Txt), Builder(NULL), Class(C), pControl(NULL), ExecCommand(ExecStr), ImgBitmap(NULL)
	{}
	~WBB_Button()
	{
		if (ImgBitmap)
			delete ImgBitmap;
	}
};

typedef struct {
	FString Name, ClName;
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

enum EButtonGroupType : DWORD
{
	BGT_CameraMode,
	BGT_Clipping,
	BGT_Builders,
	BGT_CSG,
	BGT_Misc,
	BGT_User,
};

class WButtonGroup : public WWindow
{
	DECLARE_WINDOWCLASS(WButtonGroup, WWindow, Window);

	EButtonGroupType GroupType;
	FString Caption;
	WButton* pExpandButton;
	TArray<WBB_Button*> Buttons;
	HBITMAP hbmDown, hbmUp;
	int iState, LastX, LastY, iRefButton;
	WToolTip* ToolTipCtrl{};
	FString GroupName;
	TArray<WBB_MoverType> MoverTypes,VolumeTypes;
	WDlgBrushBuilder* pDBB;
	HBITMAP hbmCamSpeed[3]{};

	// Structors.
	WButtonGroup( FName InPersistentName, WWindow* InOwnerWindow, EButtonGroupType InType)
	:	WWindow( InPersistentName, InOwnerWindow ), GroupType(InType)
	{
		iState = eBGSTATE_DOWN;
		hbmDown = (HBITMAP)LoadImageA( hInstance, MAKEINTRESOURCEA(IDBM_DOWN_ARROW), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR );	check(hbmDown);
		hbmUp = (HBITMAP)LoadImageA( hInstance, MAKEINTRESOURCEA(IDBM_UP_ARROW), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR );	check(hbmUp);
		hbmCamSpeed[0] = (HBITMAP)LoadImageA( hInstance, MAKEINTRESOURCEA(IDBM_CAMSPEED1), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR );	check(hbmCamSpeed[0]);
		hbmCamSpeed[1] = (HBITMAP)LoadImageA( hInstance, MAKEINTRESOURCEA(IDBM_CAMSPEED2), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR );	check(hbmCamSpeed[1]);
		hbmCamSpeed[2] = (HBITMAP)LoadImageA( hInstance, MAKEINTRESOURCEA(IDBM_CAMSPEED3), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR );	check(hbmCamSpeed[2]);

		ScaleImageAndReplace(hbmDown);
		ScaleImageAndReplace(hbmUp);
		ScaleImageAndReplace(hbmCamSpeed[0]);
		ScaleImageAndReplace(hbmCamSpeed[1]);
		ScaleImageAndReplace(hbmCamSpeed[2]);

		LastX = 2;
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

		if(!GConfig->GetInt( TEXT("Groups"), *GroupName, iState, TEXT("UnrealEd.ini") ))	iState = eBGSTATE_DOWN;
		UpdateButton();
		unguard;
	}
	void OnDestroy()
	{
		guard(WButtonGroup::OnDestroy);

		for( int x = 0 ; x < Buttons.Num() ; x++ )
		{
			::DeleteObject( Buttons(x)->hbm );
			::DestroyWindow( Buttons(x)->hWnd );
			delete Buttons(x);
		}
		if( pExpandButton )
		{
			DestroyWindow( pExpandButton->hWnd );
			delete pExpandButton;
		}
		delete pDBB;
		DeleteObject(hbmDown);
		DeleteObject(hbmUp);
		DeleteObject(hbmCamSpeed[0]);
		DeleteObject(hbmCamSpeed[1]);
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

		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void OnPaint()
	{
		guard(WButtonGroup::OnPaint);
		PAINTSTRUCT PS;
		HDC hDC = BeginPaint( *this, &PS );
		HBRUSH brushBack = CreateSolidBrush( RGB(128,128,128) );

		RECT rect;
		::GetClientRect( hWnd, &rect );

		FillRect( hDC, &rect, brushBack );
		MyDrawEdge( hDC, &rect, 1 );

		rect.top += MulDiv(6, DPIY, 96);		rect.bottom = rect.top + MulDiv(3, DPIY, 96);
		rect.left += MulDiv(3, DPIX, 96);		rect.right -= MulDiv(32, DPIX, 96);
		MyDrawEdge( hDC, &rect, 1 );
		rect.top += MulDiv(6, DPIY, 96);		rect.bottom = rect.top + MulDiv(3, DPIY, 96);
		MyDrawEdge( hDC, &rect, 1 );

		EndPaint( *this, &PS );

		DeleteObject( brushBack );
		unguard;
	}
	// Adds a button onto the bar.  The button will be positioned automatically.
	WBB_Button* AddButton( int _iID, UBOOL bAutoCheck, const TCHAR* BMPFilename, UTexture* ButtonImage, const TCHAR* Text, UClass* Class, UBOOL bNewLine, const TCHAR* ExecCommand = TEXT("") )
	{
		guard(WButtonGroup::AddButton);

		if( bNewLine )
		{
			LastY += dBUTTON_HEIGHT / 2;
			if( LastX >= dBUTTON_WIDTH && LastX < (dBUTTON_WIDTH * 2) )
				LastY += dBUTTON_HEIGHT;

			LastX = 2;
		}

		// Add the button into the array and set it up
		WBB_Button* pWBButton = new WBB_Button(_iID, Text, Class, ExecCommand);
		Buttons.AddItem(pWBButton);
		check(pWBButton);

		if (ButtonImage)
		{
			UTexture* T = ButtonImage;
			FTextureInfo* Info = T->GetTexture(INDEX_NONE, NULL);
			if (Info && Info->Format == TEXF_P8)
			{
				FCustomBitmap* BI = new FCustomBitmap;
				FAnimIconData ImgData;
				ImgData.UClip = Min(Info->USize, 30);
				ImgData.USize = Info->USize;
				ImgData.VClip = Min(Info->VSize, 30);
				ImgData.VSize = Info->VSize;
				ImgData.Data = Info->Mips[0]->DataPtr;
				ImgData.Palette = Info->Palette;

				const FColor BgColor(128, 128, 128, 255);
				const FColor* BgColorRef = NULL;
				if (T->PolyFlags & PF_Masked)
					BgColorRef = &BgColor;

				if (BI->CreateIconTex(ImgData, BgColorRef))
				{
					ScaleImageAndReplace(BI->GetBitmap());
					pWBButton->hbm = BI->GetBitmap();
					pWBButton->ImgBitmap = BI;
				}
				else delete BI;
			}
		}
		if (BMPFilename && *BMPFilename && !pWBButton->hbm)
		{
			FString Filename = FString::Printf(TEXT("editorres\\%ls.bmp"), BMPFilename);
			pWBButton->hbm = (HBITMAP)LoadImageA( hInstance, appToAnsi( *Filename ), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE );
			ScaleImageAndReplace(pWBButton->hbm);
			if (!pWBButton->hbm)
			{
				pWBButton->hbm = (HBITMAP)LoadImageA(hInstance, "editorres\\BBGeneric.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
				ScaleImageAndReplace(pWBButton->hbm);
			}
		}

		if (Class)
		{
			verify(Class->IsChildOf(UBrushBuilder::StaticClass()));
			pWBButton->Builder = ConstructObject<UBrushBuilder>(Class);
			UEditorEngine::BuilderButtons.AddItem(pWBButton->Builder);
		}

		// Create the physical button/label
		WCheckBox* pButton = new WCheckBox( this, _iID );
		pWBButton->pControl = pButton;
		pButton->OpenWindow( 1, LastX, LastY, dBUTTON_WIDTH, dBUTTON_HEIGHT, NULL, bAutoCheck, 1, BS_PUSHLIKE | BS_OWNERDRAW );
		pButton->SetBitmap( pWBButton->hbm );

		pWBButton->hWnd = pButton->hWnd;

		// Update position data
		LastX += dBUTTON_WIDTH;
		if( LastX >= dBUTTON_WIDTH * 2 )
		{
			LastX = 2;
			LastY += dBUTTON_HEIGHT;
		}

		ToolTipCtrl->AddTool( pButton->hWnd, Text, _iID );

		return pWBButton;
		unguard;
	}
	int GetFullHeight() const
	{
		guard(WButtonGroup::GetFullHeight);

		if( iState == eBGSTATE_DOWN )
			return dCAPTION_HEIGHT + (((Buttons.Num() + 1) / 2) * dBUTTON_HEIGHT);
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
					FString Cmd = FString::Printf(TEXT("BRUSH ADDMOVER CLASS=%ls MESH=%ls MM=%ls"),
						*pwbbmt->ClName,
						*GBrowserMesh->GetCurrentMeshName(),
						pwbbmt->bMeshMover ? TEXT("true") : TEXT("false"));
					GEditor->Exec( *Cmd );
					return;
				}
			}
		}
		if (Command >= IDMN_VOLUME_TYPES && Command <= IDMN_VOLUME_TYPES_MAX)
		{
			// Figure out which mover was chosen
			for (int x = 0; x < VolumeTypes.Num(); x++)
			{
				WBB_MoverType* pwbbmt = &(VolumeTypes(x));
				if (pwbbmt->ID == Command)
				{
					FString Cmd = FString::Printf(TEXT("BRUSH ADDVOLUME CLASS=%ls"), *pwbbmt->ClName);
					GEditor->Exec(*Cmd);
					return;
				}
			}
		}
		if (Command == WM_BB_RCLICK)
		{
			switch (GroupType)
			{
			case BGT_Builders:
			{
				INT Index = LastlParam - IDPB_BRUSH_BUILDERS;
				delete pDBB;
				pDBB = new WDlgBrushBuilder(NULL, this, Buttons(Index)->Builder);
				pDBB->DoModeless();
				break;
			}
			case BGT_CSG:
			{
				if (LastlParam == IDPB_ADD_MOVER)
				{
					// Create a context menu with all the available kinds of movers on it.
					CreateMoverTypeList();

					HMENU menu = CreatePopupMenu();
					MENUITEMINFOA mif;

					mif.cbSize = sizeof(MENUITEMINFO);
					mif.fMask = MIIM_TYPE | MIIM_ID;
					mif.fType = MFT_STRING;

					for (int x = 0; x < MoverTypes.Num(); x++)
					{
						WBB_MoverType* pwbbmt = &(MoverTypes(x));

						mif.dwTypeData = const_cast<ANSICHAR*>(appToAnsi(*(pwbbmt->Name)));
						mif.wID = pwbbmt->ID;

						InsertMenuItemA(menu, 99999, FALSE, &mif);
					}

					POINT point;
					::GetCursorPos(&point);
					TrackPopupMenu(menu,
						TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
						point.x, point.y, 0,
						hWnd, NULL);
				}
				else if (LastlParam == IDPB_ADD_VOLUME)
					CreateVolumeTypeList();
				}
			}
		}
		else
		{
			switch( HIWORD(Command) ) {

				case BN_CLICKED:
					ButtonClicked(LOWORD(Command));
					break;
				default:
					WWindow::OnCommand(Command);
					break;
			}
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
		pwbbmt->ID = ID++;
		pwbbmt->Name = TEXT("Mover");
		pwbbmt->ClName = AMover::StaticClass()->GetPathName();
		pwbbmt->bMeshMover = 0;

		pwbbmt = new(MoverTypes)WBB_MoverType();
		pwbbmt->ID = ID++;
		pwbbmt->Name = TEXT("Mover [mesh]");
		pwbbmt->ClName = AMover::StaticClass()->GetPathName();
		pwbbmt->bMeshMover = 1;

		// Now add the child classes of "mover"
		for (TObjectIterator<UClass> It; It; ++It)
			if (It->IsChildOf(AMover::StaticClass()) && *It != AMover::StaticClass() && !(It->ClassFlags & (CLASS_Abstract | CLASS_Transient | CLASS_NoUserCreate)))
			{
				pwbbmt = new(MoverTypes)WBB_MoverType();
				pwbbmt->Name = It->GetName();
				pwbbmt->ClName = It->GetPathName();
				pwbbmt->ID = ID++;
				pwbbmt->bMeshMover = 0;

				pwbbmt = new(MoverTypes)WBB_MoverType();
				pwbbmt->Name = FString::Printf(TEXT("%ls [mesh]"),It->GetName());
				pwbbmt->ClName = It->GetPathName();
				pwbbmt->ID = ID++;
				pwbbmt->bMeshMover = 1;
			}

		unguard;
	}
	void CreateVolumeTypeList(void)
	{
		guard(WButtonGroup::CreateVolumeTypeList);

		VolumeTypes.Empty();

		WBB_MoverType* pwbbmt;
		int ID = IDMN_VOLUME_TYPES;

		// Now add the child classes of "mover"
		for (TObjectIterator<UClass> It; It; ++It)
			if (It->IsChildOf(AActor::StaticClass()) && It->GetDefaultActor()->bSpecialBrushActor && !(It->ClassFlags & (CLASS_Abstract | CLASS_Transient)))
			{
				pwbbmt = new(VolumeTypes)WBB_MoverType();
				pwbbmt->Name = It->GetName();
				pwbbmt->ClName = It->GetPathName();
				pwbbmt->ID = ID++;
			}

		HMENU menu = CreatePopupMenu();
		MENUITEMINFOA mif;

		mif.cbSize = sizeof(MENUITEMINFO);
		mif.fMask = MIIM_TYPE | MIIM_ID;
		mif.fType = MFT_STRING;

		for (int x = 0; x < VolumeTypes.Num(); x++)
		{
			pwbbmt = &(VolumeTypes(x));

			mif.dwTypeData = const_cast<ANSICHAR*>(appToAnsi(*(pwbbmt->Name)));
			mif.wID = pwbbmt->ID;

			InsertMenuItemA(menu, 99999, FALSE, &mif);
		}

		POINT point;
		::GetCursorPos(&point);
		TrackPopupMenu(menu,
			TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
			point.x, point.y, 0,
			hWnd, NULL);
		unguard;
	}
	// Searches the list for the button that was clicked, and executes the appropriate command.
	void ButtonClicked( int ID )
	{
		guard(WButtonGroup::ButtonClicked);

		switch (GroupType)
		{
		case BGT_CameraMode:
		{
			INT Index = ID - IDPB_BRUSH_BUILDERS;
			if (Buttons.IsValidIndex(Index))
				GEditor->edcamSetMode(Buttons(Index)->edMode);
			break;
		}
		case BGT_Clipping:
		{
			switch (ID)
			{
			case IDPB_BRUSHCLIP:
				GEditor->Exec(TEXT("BRUSHCLIP"));
				GEditor->RedrawLevel(GEditor->Level);
				break;

			case IDPB_BRUSHCLIP_SPLIT:
				GEditor->Exec(TEXT("BRUSHCLIP SPLIT"));
				GEditor->RedrawLevel(GEditor->Level);
				break;

			case IDPB_BRUSHCLIP_FLIP:
				GEditor->Exec(TEXT("BRUSHCLIP FLIP"));
				GEditor->RedrawLevel(GEditor->Level);
				break;

			case IDPB_BRUSHCLIP_DELETE:
				GEditor->Exec(TEXT("BRUSHCLIP DELETE"));
				GEditor->RedrawLevel(GEditor->Level);
				break;
			}
			break;
		}
		case BGT_Builders:
		{
			INT Index = ID - IDPB_BRUSH_BUILDERS;
			if (Buttons.IsValidIndex(Index))
				Buttons(Index)->Builder->Build();
			break;
		}
		case BGT_CSG:
		{
			switch (ID)
			{
			case IDPB_MODE_ADD:
				GEditor->Exec(TEXT("BRUSH ADD"));
				break;

			case IDPB_MODE_SUBTRACT:
				GEditor->Exec(TEXT("BRUSH SUBTRACT"));
				break;

			case IDPB_MODE_INTERSECT:
				GEditor->Exec(TEXT("BRUSH FROM INTERSECTION"));
				break;

			case IDPB_MODE_DEINTERSECT:
				GEditor->Exec(TEXT("BRUSH FROM DEINTERSECTION"));
				break;

			case IDPB_ADD_SPECIAL:
			{
				if (!GDlgAddSpecial)
				{
					GDlgAddSpecial = new WDlgAddSpecial(NULL, this);
					GDlgAddSpecial->DoModeless();
				}
				else
					GDlgAddSpecial->Show(1);
			}
			break;

			case IDPB_ADD_MOVER:
				GEditor->Exec(TEXT("BRUSH ADDMOVER"));
				break;

			case IDPB_ADD_VOLUME:
				CreateVolumeTypeList();
				break;
			}
			break;
		}
		case BGT_Misc:
			switch (ID)
			{
			case IDPB_CAMERA_SPEED:
			{
				if (GEditor->MovementSpeed == 1)
					GEditor->Exec(TEXT("MODE SPEED=4"));
				else if (GEditor->MovementSpeed == 4)
					GEditor->Exec(TEXT("MODE SPEED=16"));
				else
					GEditor->Exec(TEXT("MODE SPEED=1"));
				break;
			}
			case IDPB_SHOW_SELECTED:
				GEditor->Exec(TEXT("ACTOR HIDE UNSELECTED"));
				break;
			case IDPB_HIDE_SELECTED:
				GEditor->Exec(TEXT("ACTOR HIDE SELECTED"));
				break;
			case IDPB_SELECT_INSIDE:
				GEditor->Exec(TEXT("ACTOR SELECT INSIDE"));
				break;
			case IDPB_SELECT_ALL:
				GEditor->Exec(TEXT("ACTOR SELECT ALL"));
				break;
			case IDPB_SHOW_ALL:
				GEditor->Exec(TEXT("ACTOR UNHIDE ALL"));
				break;
			case IDPB_INVERT_SELECTION:
				GEditor->Exec(TEXT("ACTOR SELECT INVERT"));
				break;
			case IDPB_ALIGN_VIEWPORTCAM:
				GEditor->Exec(TEXT("CAMERA ALIGN"));
				break;
			}
		default:
		{
			INT Index = ID - IDPB_USER_DEFINED;
			if (Buttons.IsValidIndex(Index))
				GEditor->Exec(*(Buttons(Index)->ExecCommand));
			break;
		}
		}

		UpdateButtons();

		unguardf((TEXT("(%i)"), ID));
	}
	// Updates the states of the buttons to match editor settings.
	void UpdateButtons()
	{
		guard(WButtonGroup::UpdateButtons);

		if (GroupType == BGT_CameraMode)	// Make sure we're in the mode group before bothering
		{
			INT CurMode = GEditor->Mode;
			for (INT i = 0; i < Buttons.Num(); ++i)
			{
				Buttons(i)->pControl->SetCheck(Buttons(i)->edMode == CurMode);
				InvalidateRect(Buttons(i)->pControl->hWnd, NULL, 1);
			}
		}
		else if (GroupType == BGT_Misc)
		{
			WCheckBox* pButton = Buttons(iRefButton)->pControl;

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
};

// --------------------------------------------------------------
//
// WBUTTONBAR
//
// --------------------------------------------------------------

class WButtonBar : public WWindow
{
	DECLARE_WINDOWCLASS(WButtonBar,WWindow,Window)

	TArray<WButtonGroup*> Groups;
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
		si.fMask = SIF_DISABLENOSCROLL | SIF_RANGE | SIF_POS;
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
			Height += Groups(x)->GetFullHeight();

		return Height;

		unguard;
	}
	void OnDestroy()
	{
		guard(WButtonBar::OnDestroy);
		for (int x = 0; x < Groups.Num(); x++)
			delete Groups(x);
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
	WButtonGroup* AddGroup( FString GroupName, EButtonGroupType Type)
	{
		guard(WButtonBar::AddGroup);
		WButtonGroup* NewGroup = new WButtonGroup(TEXT(""), this, Type);
		Groups.AddItem(NewGroup);
		NewGroup->GroupName = GroupName;
		return NewGroup;
		unguard;
	}
	// Doing things like loading new maps and such will cause us to have to recreate
	// brush builder objects.
	void RefreshBuilders()
	{
		guardSlow(WButtonBar::RefreshBuilders);
		// No-op.
		unguardSlow;
	}
	void OnCreate()
	{
		guard(WButtonBar::OnCreate);
		WWindow::OnCreate();

		pScrollBar = new WVScrollBar( this, IDSB_SCROLLBAR2 );
		pScrollBar->OpenWindow( 1, 0, 0, MulDiv(320, DPIX, 96), MulDiv(200, DPIY, 96) );
		
		INT i;
		TArray<UBrushBuilder*> BrushBuilders;
		for (TObjectIterator<UClass> ItC; ItC; ++ItC)
			if (!(ItC->ClassFlags & CLASS_Abstract))
			{
				if (ItC->IsChildOf(UBrushBuilder::StaticClass()))
					BrushBuilders.AddItem(reinterpret_cast<UBrushBuilder*>(ItC->GetDefaultObject()));
			}

		// Load the buttons onto the bar.
		WButtonGroup* pGroup;

		pGroup = AddGroup(TEXT("Modes"), BGT_CameraMode);
		pGroup->OpenWindow();

		for (INT i = 0; i < GEditor->EditModeList.Num(); ++i)
		{
			const FEditModeType& M = GEditor->EditModeList(i);
			WBB_Button* Button = pGroup->AddButton(IDPB_BRUSH_BUILDERS + i, 0, TEXT("ModeCamera"), M.Image, *M.ToolTip, NULL, 0);
			Button->edMode = M.Mode;
		}

		pGroup = AddGroup(TEXT("Clipping"), BGT_Clipping);
		pGroup->OpenWindow();
		pGroup->AddButton( IDPB_BRUSHCLIP, 0, TEXT("BrushClip"), NULL, TEXT("Clip Selected Brushes"), NULL, 0 );
		pGroup->AddButton( IDPB_BRUSHCLIP_SPLIT, 0, TEXT("BrushClipSplit"), NULL, TEXT("Split Selected Brushes"), NULL, 0 );
		pGroup->AddButton( IDPB_BRUSHCLIP_FLIP, 0, TEXT("BrushClipFlip"), NULL, TEXT("Flip Clipping Normal"), NULL, 0 );
		pGroup->AddButton( IDPB_BRUSHCLIP_DELETE, 0, TEXT("BrushClipDelete"), NULL, TEXT("Delete Clipping Markers"), NULL, 0 );

		pGroup = AddGroup(TEXT("Builders"), BGT_Builders);
		pGroup->OpenWindow();
		for (i = 0; i < BrushBuilders.Num(); ++i)
			pGroup->AddButton(IDPB_BRUSH_BUILDERS + i, 0, *BrushBuilders(i)->BitmapFilename, BrushBuilders(i)->BitmapImage, *BrushBuilders(i)->ToolTip, BrushBuilders(i)->GetClass(), 0);

		pGroup = AddGroup(TEXT("CSG"), BGT_CSG);
		pGroup->OpenWindow();
		pGroup->AddButton( IDPB_MODE_ADD, 0, TEXT("ModeAdd"), NULL, TEXT("Add"), NULL, 0 );
		pGroup->AddButton( IDPB_MODE_SUBTRACT, 0, TEXT("ModeSubtract"), NULL, TEXT("Subtract"), NULL, 0 );
		pGroup->AddButton( IDPB_MODE_INTERSECT, 0, TEXT("ModeIntersect"), NULL, TEXT("Intersect"), NULL, 0 );
		pGroup->AddButton( IDPB_MODE_DEINTERSECT, 0, TEXT("ModeDeintersect"), NULL, TEXT("Deintersect"), NULL, 0 );
		pGroup->AddButton( IDPB_ADD_SPECIAL, 0, TEXT("AddSpecial"), NULL, TEXT("Add Special Brush"), NULL, 0 );
		pGroup->AddButton( IDPB_ADD_MOVER, 0, TEXT("AddMover"), NULL, TEXT("Add Mover Brush (right click for all mover types)"), NULL, 0 );
		pGroup->AddButton( IDPB_ADD_VOLUME, 0, TEXT("AddVolume"), NULL, TEXT("Add Volume Brush"), NULL, 0);

		pGroup = AddGroup(TEXT("Misc"), BGT_Misc);
		pGroup->OpenWindow();
		pGroup->AddButton( IDPB_SHOW_SELECTED, 0, TEXT("ShowSelected"), NULL, TEXT("Show Selected Actors Only"), NULL, 0 );
		pGroup->AddButton( IDPB_HIDE_SELECTED, 0, TEXT("HideSelected"), NULL, TEXT("Hide Selected Actors"), NULL, 0 );
		pGroup->AddButton( IDPB_SELECT_INSIDE, 0, TEXT("SelectInside"), NULL, TEXT("Select Inside Actors"), NULL, 0 );
		pGroup->AddButton( IDPB_SELECT_ALL, 0, TEXT("SelectAll"), NULL, TEXT("Select All Actors"), NULL, 0 );
		pGroup->AddButton( IDPB_INVERT_SELECTION, 0, TEXT("InvertSelections"), NULL, TEXT("Invert Selection"), NULL, 0 );
		pGroup->AddButton( IDPB_SHOW_ALL, 0, TEXT("ShowAll"), NULL, TEXT("Show All Actors"), NULL, 0 );
		pGroup->iRefButton = pGroup->Buttons.Num();
		pGroup->AddButton( IDPB_CAMERA_SPEED, 0, TEXT("ModeCamera"), NULL, TEXT("Change Camera Speed"), NULL, 0 );
		pGroup->AddButton( IDPB_ALIGN_VIEWPORTCAM, 0, TEXT("AlignCameras"), NULL, TEXT("Align Viewport Cameras to 3D Viewport Camera"), NULL, 0 );

		INT NumButtons;
		if (GConfig->GetInt(TEXT("UserDefinedGroup"), TEXT("NumButtons"), NumButtons, GUEdIni) && NumButtons)
		{
			pGroup = AddGroup(TEXT("UserDefined"), BGT_User);
			pGroup->OpenWindow();

			for( int x = 0 ; x < NumButtons ; x++ )
			{
				FString ButtonDef;
				FString ButtonName = FString::Printf(TEXT("Button%d"), x);

				TArray<FString> Fields;
				GConfig->GetString(TEXT("UserDefinedGroup"), *ButtonName, ButtonDef, GUEdIni);
				ParseStringToArray( TEXT(","), ButtonDef, &Fields );

				pGroup->AddButton( IDPB_USER_DEFINED + x, 0, *Fields(1), NULL, *Fields(0), NULL, 0, *Fields(2) );
			}
		}

		PositionChildControls();
		UpdateButtons();

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
			::MoveWindow( Groups(x)->hWnd, 0, LastY, rect.right - GScrollBarWidth, Groups(x)->GetFullHeight(), 1 );
			LastY += Groups(x)->GetFullHeight();
		}

		::MoveWindow( pScrollBar->hWnd, rect.right - GScrollBarWidth, 0, GScrollBarWidth, rect.bottom, 1 );

		unguard;
	}
	virtual void OnVScroll( WPARAM wParam, LPARAM lParam )
	{
		guard(WButtonBar::OnVScroll);
		if( (HWND)lParam == pScrollBar->hWnd ) {

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
		HBRUSH brushBack = CreateSolidBrush( RGB(128,128,128) );

		FRect Rect = GetClientRect();
		FillRect( hDC, Rect, brushBack );
		MyDrawEdge( hDC, Rect, 1 );

		EndPaint( *this, &PS );

		DeleteObject( brushBack );
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
			Groups(x)->UpdateButtons();
		unguard;
	}
};
/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
