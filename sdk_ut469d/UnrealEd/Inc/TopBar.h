/*=============================================================================
	TopBar : Class for handling the controls on the Top bar
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>

#define TB_BUTTON_WIDTH 24
#define TB_BUTTON_HEIGHT 24

extern HWND GhwndEditorFrame;

struct {
	int ID;
	int BitmapID;
	HBITMAP hbm;
	TCHAR ToolTip[64];
	int Width;
}
GTB_Buttons[] =
{
	ID_FileNew, IDBM_FILENEW, NULL, TEXT("New Map"), TB_BUTTON_WIDTH,
	ID_FileOpen, IDBM_FILEOPEN, NULL, TEXT("Open Map"), TB_BUTTON_WIDTH,
	ID_FileSave, IDBM_FILESAVE, NULL, TEXT("Save Map"), TB_BUTTON_WIDTH,
	-2, 0, NULL, TEXT(""), TB_BUTTON_WIDTH / 2,
	ID_EditUndo, IDBM_UNDO, NULL, TEXT("Undo"), TB_BUTTON_WIDTH,
	ID_EditRedo, IDBM_REDO, NULL, TEXT("Redo"), TB_BUTTON_WIDTH,
	-2, 0, NULL, TEXT(""), TB_BUTTON_WIDTH / 2,
	ID_EditFind, IDBM_EDITFIND, NULL, TEXT("Search for Actors"), TB_BUTTON_WIDTH,
	-2, 0, NULL, TEXT(""), TB_BUTTON_WIDTH / 2,
	ID_BrowserActor, IDBM_ACTORBROWSER, NULL, TEXT("Actor Class Browser"), TB_BUTTON_WIDTH,
	ID_BrowserGroup, IDBM_GROUPBROWSER, NULL, TEXT("Group Browser"), TB_BUTTON_WIDTH,
	ID_BrowserMusic, IDBM_MUSICBROWSER, NULL, TEXT("Music Browser"), TB_BUTTON_WIDTH,
	ID_BrowserSound, IDBM_SOUNDBROWSER, NULL, TEXT("Sound Browser"), TB_BUTTON_WIDTH,
	ID_BrowserTexture, IDBM_TEXTUREBROWSER, NULL, TEXT("Texture Browser"), TB_BUTTON_WIDTH,
	ID_BrowserMesh, IDBM_MESHVIEWER, NULL, TEXT("Mesh Browser"), TB_BUTTON_WIDTH,
	-2, 0, NULL, TEXT(""), TB_BUTTON_WIDTH / 2,
	ID_Tools2DEditor, IDBM_2DSE, NULL, TEXT("2D Shape Editor"), TB_BUTTON_WIDTH,
	IDMN_CODE_FRAME, IDBM_UNREALSCRIPT, NULL, TEXT("UnrealScript Editor"), TB_BUTTON_WIDTH,
	-2, 0, NULL, TEXT(""), TB_BUTTON_WIDTH / 2,
	ID_ViewActorText, IDBM_ACTORTEXTVIEW, NULL, TEXT("Actor Text View"), TB_BUTTON_WIDTH,
	ID_ViewActorProp, IDBM_ACTORPROPERTIES, NULL, TEXT("Actor Properties"), TB_BUTTON_WIDTH,
	ID_SurfProperties, IDBM_SURFACEPROPERTIES, NULL, TEXT("Surface Properties"), TB_BUTTON_WIDTH,
	-2, 0, NULL, TEXT(""), TB_BUTTON_WIDTH / 2,
	ID_BuildGeometry, IDBM_BUILDGEOM, NULL, TEXT("Build Geometry"), TB_BUTTON_WIDTH,
	ID_BuildLighting, IDBM_BUILDLIGHTING, NULL, TEXT("Build Lighting"), TB_BUTTON_WIDTH,
	ID_BuildPaths, IDBM_BUILDPATHS, NULL, TEXT("Build Paths"), TB_BUTTON_WIDTH,
	ID_BuildAll, IDBM_BUILDALL, NULL, TEXT("Build All (as per current build settings)"), TB_BUTTON_WIDTH,
	ID_BuildOptions, IDBM_BUILDOPTIONS, NULL, TEXT("Build Options"), TB_BUTTON_WIDTH,
	-2, 0, NULL, TEXT(""), TB_BUTTON_WIDTH / 2,
	ID_BuildPlay, IDBM_PLAYMAP, NULL, TEXT("Play Map!"), TB_BUTTON_WIDTH,
	-1, -1, NULL, TEXT(""), -1
};

class WTopBar : public WWindow
{
	DECLARE_WINDOWCLASS(WTopBar,WWindow,Window)

	WToolTip* ToolTipCtrl;
	TArray<WButton> Buttons;

	// Structors.
	WTopBar( FName InPersistentName, WWindow* InOwnerWindow )
		: WWindow(InPersistentName, InOwnerWindow)
		, ToolTipCtrl(NULL)
	{
	}

	void OpenWindow()
	{
		guard(WTopBar::OpenWindow);
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

		ToolTipCtrl = new WToolTip(this);
		ToolTipCtrl->OpenWindow();

		// Create the child buttons we need.
		int Pos = 2, VPos = 4;;
		for( int x = 0 ; GTB_Buttons[x].ID != -1 ; x++ )
		{
			if( GTB_Buttons[x].ID != -2 )
			{
				new(Buttons)WButton( this, GTB_Buttons[x].ID );
				WButton* pButton = &(Buttons(Buttons.Num() - 1));	check(pButton);
				pButton->OpenWindow( Pos, Pos, VPos, MulDiv(TB_BUTTON_WIDTH, DPIX, 96), MulDiv(TB_BUTTON_HEIGHT, DPIY, 96), NULL, 0, BS_OWNERDRAW );
				GTB_Buttons[x].hbm = (HBITMAP)LoadImageA( hInstance, MAKEINTRESOURCEA(GTB_Buttons[x].BitmapID), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR );	check(GTB_Buttons[x].hbm);
				ScaleImageAndReplace(GTB_Buttons[x].hbm);
				pButton->SetBitmap( GTB_Buttons[x].hbm );
				ToolTipCtrl->AddTool( pButton->hWnd, GTB_Buttons[x].ToolTip, GTB_Buttons[x].ID );
			}

			Pos += MulDiv(GTB_Buttons[x].Width, DPIX, 96);
		}
		unguard;
	}
	void OnDestroy()
	{
		guard(WTopBar::OnDestroy);

		delete ToolTipCtrl;

		for( int x = 0 ; GTB_Buttons[x].ID != -1 ; x++ )
			DeleteObject( GTB_Buttons[x].hbm );

		for( int x = 0 ; x < Buttons.Num() ; x++ )
			DestroyWindow( Buttons(x).hWnd );

		WWindow::OnDestroy();
		unguard;
	}
	void OnPaint()
	{
		guard(WTopBar::OnPaint);
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
	INT OnSetCursor()
	{
		guard(WTopBar::OnSetCursor);
		WWindow::OnSetCursor();
		SetCursor(LoadCursorW(NULL, IDC_ARROW));
		return 0;
		unguard;
	}
	void OnCommand( INT Command )
	{
		guard(WTopBar::OnCommand);

		switch( Command )
		{
			case WM_PB_PUSH:
				PostMessageW( GhwndEditorFrame, WM_COMMAND, LastlParam, 0L );
				break;
		}

		unguard;
	}
	// Updates the states of the buttons to match editor settings.
	void UpdateButtons()
	{
		guard(WTopBar::UpdateButtons);
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
