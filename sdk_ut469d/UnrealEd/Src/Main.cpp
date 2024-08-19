/*=============================================================================
	Main.cpp: UnrealEd Windows startup.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

    Revision history:
		* Created by Tim Sweeney.

    Work-in-progress todo's:

=============================================================================*/

enum eLASTDIR {
	eLASTDIR_UNR = 0,
	eLASTDIR_UTX = 1,
	eLASTDIR_PCX = 2,
	eLASTDIR_UAX = 3,
	eLASTDIR_WAV = 4,
	eLASTDIR_BRUSH = 5,
	eLASTDIR_2DS = 6,
	eLASTDIR_USM = 7,
	eLASTDIR_UMX = 8,
	eLASTDIR_MUS = 9,
	eLASTDIR_CLS = 10,
	eLASTDIR_MAX = 11
};

enum eBROWSER {
	eBROWSER_MESH		= 0,
	eBROWSER_MUSIC		= 1,
	eBROWSER_SOUND		= 2,
	eBROWSER_ACTOR		= 3,
	eBROWSER_GROUP		= 4,
	eBROWSER_TEXTURE	= 5,
	eBROWSER_MAX		= 6,
};

#include "Engine.h"
#include "UnRender.h"

#if __STATIC_LINK

// Extra stuff for static links for Engine.
#include "UnEngineNative.h"
#include "UnCon.h"
#include "UnNet.h"

// Fire static stuff.
#include "UnFractal.h"
#include "UnFireNative.h"

// IpDrv static stuff.
#include "UnIpDrv.h"
#include "UnTcpNetDriver.h"
#include "UnIpDrvCommandlets.h"
#include "UnIpDrvNative.h"
#include "HTTPDownload.h"

// UWeb static stuff.
#include "UWeb.h"
#include "UWebNative.h"

// Render static stuff.
#include "Render.h"
#include "UnRenderNative.h"

// Editor static stuff.
#include "Editor.h"
#include "UnEditorNative.h"

// Audio devices
//#include "ALAudio.h"
//#include "Cluster.h"
# if !BUILD_64
#  include "UnGalaxy.h"
# endif

// Renderers
#include "D3D9Drv.h"
#include "UTGLRD3D9.h"
#include "SoftDrvPrivate.h"
#include "XOpenGLDrv.h"
#include "OpenGLDrv.h"
#include "UTGLROpenGL.h"

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shellapi.h>

// Windrv stuff
#include "Window.h"
#include "WinDrv.h"

#else

//#define STRICT
#pragma warning(push)
#pragma warning(disable: 4121) /* 'JOBOBJECT_IO_RATE_CONTROL_INFORMATION_NATIVE_V2': alignment of a member was sensitive to packing */
#include <windows.h>
#pragma warning(pop)
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shellapi.h>

#include "Window.h"

#endif

#include "..\..\Editor\Src\EditorPrivate.h"
#include "Res\resource.h"
#include "FConfigCacheIni.h"
#include "UnEngineWin.h"

#include "MRUList.h"
#include "DlgProgress.h"
#include "DlgRename.h"
#include "DlgSearchActors.h"
#include "ViewportWindowContainer.h"
#include "Browser.h"
#include "BrowserMaster.h"
WBrowserMaster* GBrowserMaster = NULL;
BOOL AllowOverwrite(const TCHAR* File);
// MapSave, MapSaveAs, MapOpen return TRUE if the save/open succeeded
BOOL MapSave(WWindow* Window);
BOOL MapSaveAs(WWindow* Window);
BOOL MapOpen(WWindow* Window);
#include "CodeFrame.h"
#include "DlgDialogTool.h"
#include "DlgTexProp.h"
#include "DlgBrushBuilder.h"
#include "DlgAddSpecial.h"
#include "DlgScaleLights.h"
#include "DlgScaleMap.h"
#include "DlgTexReplace.h"
#include "DlgArray.h"
#include "DlgRotateActors.h"
#include "SurfPropSheet.h"
#include "BuildSheet.h"
#include "DlgBrushImport.h"
#include "DlgViewportConfig.h"
#include "DlgMapImport.h"
#include "DlgStaticMesh.h"
#include "DlgGridOptions.h"
#include "TwoDeeShapeEditor.h"
#include "Extern.h"
#include "BrowserSound.h"
#include "BrowserMusic.h"
#include "BrowserGroup.h"
#include "BrowserTexture.h"
#include "BrowserMesh.h"

extern HWND GhwndBSPages[eBS_MAX];

// Just to keep track of the last viewport to get the focus.  The main editor
// app uses this to draw a white outline around the current viewport.
MRUList* GMRUList;
HWND GCurrentViewportFrame = NULL;
int GScrollBarWidth = GetSystemMetrics(SM_CXVSCROLL);
HWND GhwndEditorFrame = NULL;

enum EViewportStyle
{
	VSTYLE_Floating		= 0,
	VSTYLE_Fixed		= 1,
};

class WViewportFrame;
typedef struct {
	int RendMap;
	float PctLeft, PctTop, PctRight, PctBottom;	// Percentages of the parent window client size (VSTYLE_Fixed)
	float Left, Top, Right, Bottom;				// Literal window positions (VSTYLE_Floatin)
	WViewportFrame* m_pViewportFrame;
} VIEWPORTCONFIG;

// This is a list of all the viewport configs that are currently in effect.
TArray<VIEWPORTCONFIG> GViewports;

// Prefebbed viewport configs.  These should be in the same order as the buttons in DlgViewportConfig.
VIEWPORTCONFIG GTemplateViewportConfigs[6][4] =
{
	// 0
	REN_OrthXY,		0,		0,		.65f,		.50f,		0, 0, 0, 0,		NULL,
	REN_OrthXZ,		.65f,	0,		.35f,		.50f,		0, 0, 0, 0,		NULL,
	REN_DynLight,	0,		.50f,	.65f,		.50f,		0, 0, 0, 0,		NULL,
	REN_OrthYZ,		.65f,	.50f,	.35f,		.50f,		0, 0, 0, 0,		NULL,

	// 1
	REN_OrthXY,		0,		0,		.40f,		.40f,		0, 0, 0, 0,		NULL,
	REN_OrthXZ,		.40f,	0,		.30f,		.40f,		0, 0, 0, 0,		NULL,
	REN_OrthYZ,		.70f,	0,		.30f,		.40f,		0, 0, 0, 0,		NULL,
	REN_DynLight,	0,		.40f,	1.0f,		.60f,		0, 0, 0, 0,		NULL,

	// 2
	REN_DynLight,	0,		0,		.70f,		1.0f,		0, 0, 0, 0,		NULL,
	REN_OrthXY,		.70f,	0,		.30f,		.40f,		0, 0, 0, 0,		NULL,
	REN_OrthXZ,		.70f,	.40f,	.30f,		.30f,		0, 0, 0, 0,		NULL,
	REN_OrthYZ,		.70f,	.70f,	.30f,		.30f,		0, 0, 0, 0,		NULL,

	// 3
	REN_OrthXY,		0,		0,		1.0f,		.40f,		0, 0, 0, 0,		NULL,
	REN_DynLight,	0,		.40f,	1.0f,		.60f,		0, 0, 0, 0,		NULL,
	-1,	0, 0, 0, 0, 0, 0, 0, 0, NULL,
	-1,	0, 0, 0, 0, 0, 0, 0, 0, NULL,

	// 4
	REN_OrthXY,		0,		0,		.50f,		1.0f,		0, 0, 0, 0,		NULL,
	REN_DynLight,	.50f,	0,		.50f,		1.0f,		0, 0, 0, 0,		NULL,
	-1,	0, 0, 0, 0, 0, 0, 0, 0, NULL,
	-1,	0, 0, 0, 0, 0, 0, 0, 0, NULL,
	
	// 5
	REN_OrthXY,		0,		0,		1.0f,		1.0f,		0, 0, 0, 0,		NULL,
	-1,	0, 0, 0, 0, 0, 0, 0, 0, NULL,
	-1,	0, 0, 0, 0, 0, 0, 0, 0, NULL,
	-1,	0, 0, 0, 0, 0, 0, 0, 0, NULL,
};

int GViewportStyle, GViewportConfig;

#include "ViewportFrame.h"

FString GLastDir[eLASTDIR_MAX];
HMENU GMainMenu = NULL;

extern "C" {HINSTANCE hInstance;}
TCHAR GPackageInternal[64] = TEXT("UnrealEd");
extern "C" {const TCHAR* GPackage = GPackageInternal; }

// Brushes.
HBRUSH hBrushMode = CreateSolidBrush( RGB(0,96,0) );

extern FString GLastText;
#if __STATIC_LINK
extern FString GMapExt;
#else
EDITOR_API FString GMapExt;
#endif

// Forward declarations
void UpdateMenu();
// Classes.
class WMdiClient;
class WMdiFrame;
class WEditorFrame;
class WMdiDockingFrame;
class WLevelFrame;

// Memory allocator.
#include "FMallocAnsi.h"
FMallocAnsi Malloc;

// Log file.
#include "FOutputDeviceFile.h"
FOutputDeviceFile Log;

// Error handler.
#include "FOutputDeviceWindowsError.h"
FOutputDeviceWindowsError Error;

// Feedback.
#include "FFeedbackContextWindows.h"
FFeedbackContextWindows Warn;

// File manager.
#include "FFileManagerWindows.h"
FFileManagerWindows FileManager;

// Config.
#include "FConfigCacheIni.h"

WLevelFrame* GLevelFrame = NULL;
WCodeFrame* GCodeFrame = NULL;
#include "BrowserActor.h"

WEditorFrame* GEditorFrame = NULL;
W2DShapeEditor* G2DShapeEditor = NULL;
TSurfPropSheet* GSurfPropSheet = NULL;
TBuildSheet* GBuildSheet = NULL;
WBrowserSound* GBrowserSound = NULL;
WBrowserMusic* GBrowserMusic = NULL;
WBrowserGroup* GBrowserGroup = NULL;
WBrowserActor* GBrowserActor = NULL;
WBrowserTexture* GBrowserTexture = NULL;
WBrowserMesh* GBrowserMesh = NULL;
WDlgAddSpecial* GDlgAddSpecial = NULL;
WDlgScaleLights* GDlgScaleLights = NULL;
WDlgArray* GDlgArray = NULL;
WDlgRotateActors* GRotateActors = NULL;
WDlgScaleMap* GDlgScaleMap = NULL;
WDlgProgress* GDlgProgress = NULL;
WDlgSearchActors* GDlgSearchActors = NULL;
WViewText* GViewActorText = NULL;

UBOOL WBrowser::IsCurrent()
{
	return GBrowserMaster && GBrowserMaster->GetCurrent() == BrowserID;
}

#include "SelectGroups.h"
WSelectGroups* GSelectGroups = NULL;
WDlgTexReplace* GDlgTexReplace = NULL;

#include "ButtonBar.h"
#include "BottomBar.h"
#include "TopBar.h"
WButtonBar* GButtonBar;
WBottomBar* GBottomBar;
WTopBar* GTopBar;

// Custom ContextMenu
FString Custom1, Custom2, Custom3, Custom4, Custom5;

WNDPROC WGroupCheckListBox::SuperProc = NULL;
W_IMPLEMENT_CLASS(WGroupCheckListBox)

/*-----------------------------------------------------------------------------
	Global Funcs
-----------------------------------------------------------------------------*/
//
// MapSaveChanges returns:
// - IDYES if the changes were saved
// - IDNO if the changes were discarded
// - IDCANCEL if the action should be canceled
// 
INT MapSaveChanges(WWindow* Window);

UMesh* SavedMesh = NULL;
void PreSave()
{
	GEditor->Exec(TEXT("BRUSHCLIP DELETE"));
	// prevent make dependency to loaded Mesh in MeshBrowser
	SavedMesh = GBrowserMesh && GBrowserMesh->pViewport && GBrowserMesh->pViewport->Actor ? GBrowserMesh->pViewport->Actor->Mesh : NULL;
	if (SavedMesh)
		GBrowserMesh->pViewport->Actor->Mesh = NULL;
}

UBOOL PostSave(UBOOL Result)
{
	if (SavedMesh && GBrowserMesh && GBrowserMesh->pViewport && GBrowserMesh->pViewport->Actor)
		GBrowserMesh->pViewport->Actor->Mesh = SavedMesh;
	SavedMesh = NULL;
	return Result;
}

void RefreshEditor()
{
	guard(RefreshEditor);
	GBrowserMaster->RefreshAll();
	if (GBrowserGroup)
		GBrowserGroup->RefreshGroupList();
	GBuildSheet->RefreshStats();
	GButtonBar->RefreshBuilders();
	// refresh search
	GDlgSearchActors->OnShowWindow(GDlgSearchActors->m_bShow);
	unguard;
}

/*-----------------------------------------------------------------------------
	Document manager crappy abstraction.
-----------------------------------------------------------------------------*/

struct FDocumentManager
{
	virtual void OpenLevelView()=0;
} *GDocumentManager=NULL;

/*-----------------------------------------------------------------------------
	WMdiClient.
-----------------------------------------------------------------------------*/

// An MDI client window.
class WMdiClient : public WControl
{
	DECLARE_WINDOWSUBCLASS(WMdiClient,WControl,UnrealEd)
	WMdiClient( WWindow* InOwner )
	: WControl( InOwner, 0, SuperProc )
	{}
	void OpenWindow( CLIENTCREATESTRUCT* ccs )
	{
		guard(WMdiClient::OpenWindow);
		//must make nccreate work!! GetWindowClassName(),
		//!! WS_VSCROLL | WS_HSCROLL
        HWND hWndCreated = CreateWindowEx(0,TEXT("MDICLIENT"),NULL,WS_CHILD|WS_CLIPCHILDREN | WS_CLIPSIBLINGS,0,0,0,0,OwnerWindow->hWnd,(HMENU)0xCAC,hInstance,ccs);
		check(hWndCreated);
		check(!hWnd);
		AddWindow(this, TEXT("WMdiClient"));
		hWnd = hWndCreated;
		Show( 1 );
		unguard;
	}
};
WNDPROC WMdiClient::SuperProc = NULL;

/*-----------------------------------------------------------------------------
	WDockingFrame.
-----------------------------------------------------------------------------*/

// One of four docking frame windows on a MDI frame.
class WDockingFrame : public WWindow
{
	DECLARE_WINDOWCLASS(WDockingFrame,WWindow,UnrealEd)

	// Variables.
	INT DockDepth;
	WWindow* Child;

	// Functions.
	WDockingFrame( FName InPersistentName, WMdiFrame* InFrame, INT InDockDepth )
	:	WWindow			( InPersistentName, (WWindow*)InFrame )
	,	Child			( NULL )
	{
		// stijn: apply DPI scaling to the dockdepth
		DockDepth = MulDiv(InDockDepth, DPIX, 96);
	}
	void OpenWindow()
	{
		guard(WDockingFrame::OpenWindow);
		PerformCreateWindowEx
		(
			0,
			NULL,
			WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
			0, 0, 0, 0,
			OwnerWindow->hWnd,
			NULL,
			hInstance
		);
		Show(1);
		unguard;
	}
	void Dock( WWindow* InChild )
	{
		guard(WDockingFrame::Dock);
		Child = InChild;
		unguard;
	}
	void OnSize( DWORD Flags, INT InX, INT InY )
	{
		guard(WDockingFrame::OnSize);
		if( Child )
			Child->MoveWindow( GetClientRect(), TRUE );
		unguard;
	}
	void OnPaint()
	{
		guard(WDockingFrame::OnPaint);
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
};

/*-----------------------------------------------------------------------------
	WMdiFrame.
-----------------------------------------------------------------------------*/

#define DEFAULT_WIDTH 3

// An MDI frame window.
class WMdiFrame : public WWindow
{
	DECLARE_WINDOWCLASS(WMdiFrame,WWindow,UnrealEd)

	// Variables.
	WMdiClient MdiClient;
	WDockingFrame LeftFrame, BottomFrame, TopFrame;

	// Functions.
	WMdiFrame(FName InPersistentName)
		: WWindow(InPersistentName)
		, MdiClient(this)
		, LeftFrame(TEXT("MdiFrameLeft"), this, 2 * 2 + 32 * DEFAULT_WIDTH + GScrollBarWidth)
		, BottomFrame(TEXT("MdiFrameBottom"), this, 32)
		, TopFrame(TEXT("MdiFrameTop"), this, 32)
	{
		INT ToolBarWidth = DEFAULT_WIDTH;
		GConfig->GetInt(TEXT("Options"), TEXT("ToolBarWidth"), ToolBarWidth, GUEDIni);
		ToolBarWidth = Clamp(ToolBarWidth, 1, 6);
		LeftFrame.DockDepth += (ToolBarWidth - DEFAULT_WIDTH) * MulDiv(32, DPIY, 96);
	}
	LRESULT CallDefaultProc( UINT Message, WPARAM wParam, LPARAM lParam )
	{
		return DefFrameProcW( hWnd, MdiClient.hWnd, Message, wParam, lParam );
	}
	void OnCreate()
	{
		guard(WMdiFrame::OnCreate);
		WWindow::OnCreate();

		// Create docking frames.
		BottomFrame.OpenWindow();
		LeftFrame.OpenWindow();
		TopFrame.OpenWindow();

		GSurfPropSheet = new TSurfPropSheet;
		GSurfPropSheet->OpenWindow( hInstance, hWnd );
		GSurfPropSheet->Show( FALSE );

		GBuildSheet = new TBuildSheet;
		GBuildSheet->OpenWindow( hInstance, hWnd );
		GBuildSheet->Show( FALSE );

		unguard;
	}
	virtual void RepositionClient()
	{
		guard(WMdiFrame::RepositionClient);

		// Reposition docking frames.
		FRect Client = GetClientRect();
		BottomFrame.MoveWindow( FRect(LeftFrame.DockDepth, Client.Max.Y-BottomFrame.DockDepth, Client.Max.X, Client.Max.Y), 1 );
		LeftFrame  .MoveWindow( FRect(0, TopFrame.DockDepth, LeftFrame.DockDepth, Client.Max.Y), 1 );
		TopFrame.MoveWindow( FRect(0, 0, Client.Max.X, TopFrame.DockDepth), 1 );

		// Reposition MDI client window.
		MdiClient.MoveWindow( FRect(LeftFrame.DockDepth, TopFrame.DockDepth, Client.Max.X, Client.Max.Y-BottomFrame.DockDepth), 1 );

		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WMdiFrame::OnSize);
		RepositionClient();
		throw TEXT("NoRoute");
		unguard;
	}
	void OpenWindow()
	{
		guard(WMdiFrame::OpenWindow);
		PerformCreateWindowEx
		(
			WS_EX_APPWINDOW,
#if ENGINE_VERSION==227
			*FString::Printf(*LocalizeGeneral(TEXT("FrameWindow"), TEXT("UnrealEd")), *LocalizeGeneral(TEXT("Product"), TEXT("Core"))),
#else
			*FString::Printf(LocalizeGeneral(TEXT("FrameWindow"), TEXT("UnrealEd")), LocalizeGeneral(TEXT("Product"), TEXT("Core"))),
#endif
			WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_SIZEBOX | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			MulDiv(640, DPIX, 96),
			MulDiv(480, DPIY, 96),
			NULL,
			NULL,
			hInstance
		);
		ShowWindow( *this, SW_SHOWMAXIMIZED );
		unguard;
	}
	void OnSetFocus()
	{
		guard(WMdiFrame::OnSetFocus);
		SetFocus( MdiClient );
		unguard;
	}
	int OnSysCommand(INT Command)
	{
		// stijn: we have to catch the close command here, otherwise we cannot stop it from destroying windows
		if (Command == SC_CLOSE && GLevelFrame && MapSaveChanges(reinterpret_cast<WWindow*>(GLevelFrame)) == IDCANCEL)
			return 1;
		return WWindow::OnSysCommand(Command);
	}
};

/*-----------------------------------------------------------------------------
	WBackgroundHolder.
-----------------------------------------------------------------------------*/

// Test.
class WBackgroundHolder : public WWindow
{
	DECLARE_WINDOWCLASS(WBackgroundHolder,WWindow,Window)

	// Structors.
	WBackgroundHolder( FName InPersistentName, WWindow* InOwnerWindow )
	:	WWindow( InPersistentName, InOwnerWindow )
	{}

	// WWindow interface.
	void OpenWindow()
	{
		guard(WBackgroundHolder::OpenWindow);
		MdiChild = 0;
		PerformCreateWindowEx
		(
			WS_EX_TOOLWINDOW | WS_EX_WINDOWEDGE,
			NULL,
			WS_CHILD | WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
			0,
			0,
			MulDiv(512, DPIX, 96),
			MulDiv(256, DPIY, 96),
			OwnerWindow ? OwnerWindow->hWnd : NULL,
			NULL,
			hInstance
		);
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	WLevelFrame.
-----------------------------------------------------------------------------*/

enum eBIMODE {
	eBIMODE_CENTER	= 0,
	eBIMODE_TILE	= 1,
	eBIMODE_STRETCH	= 2
};

class WLevelFrame : public WWindow
{
	DECLARE_WINDOWCLASS(WLevelFrame,WWindow,Window)

	// Variables.
	ULevel* Level;
	HBITMAP hImage;
	FString BIFilename;
	int BIMode;	// eBIMODE_

	// Structors.
	WLevelFrame( ULevel* InLevel, FName InPersistentName, WWindow* InOwnerWindow )
	:	WWindow( InPersistentName, InOwnerWindow )
	,	Level( InLevel )
	{
		SetMapFilename( TEXT("") );
		hImage = NULL;
		BIMode = eBIMODE_CENTER;
		BIFilename = TEXT("");

		for( int x = 0 ; x < GViewports.Num() ; x++)
			GViewports(x).m_pViewportFrame = NULL;
		GViewports.Empty();
	}
	void SetMapFilename( const TCHAR* _MapFilename )
	{
		MapFilename = _MapFilename;
		if( ::IsWindow( hWnd ) )
			SetText( *MapFilename );
	}
	const TCHAR* GetMapFilename()
	{
		return *MapFilename;
	}

	void OnDestroy()
	{
		guard(WLevelFrame::OnDestroy);

		//ChangeViewportStyle();

		for( int group = 0 ; group < GButtonBar->Groups.Num() ; group++ )
			GConfig->SetInt( TEXT("Groups"), *GButtonBar->Groups(group).GroupName, GButtonBar->Groups(group).iState, GUEDIni );

		// Save data out to config file, and clean up...
		GConfig->SetInt( TEXT("Viewports"), TEXT("Style"), GViewportStyle, GUEDIni );
		GConfig->SetInt( TEXT("Viewports"), TEXT("Config"), GViewportConfig, GUEDIni );

		for( int x = 0 ; x < dED_MAX_VIEWPORTS ; x++)
		{
			FString ViewportName = FString::Printf(TEXT("U2Viewport%d"), x);

			if ( x < GViewports.Num() && GViewports(x).m_pViewportFrame 
					&& ::IsWindow( GViewports(x).m_pViewportFrame->hWnd ) 
					&& !::IsIconic( GViewports(x).m_pViewportFrame->hWnd )
					&& !::IsZoomed( GViewports(x).m_pViewportFrame->hWnd ))
			{
				GConfig->SetInt( *ViewportName, TEXT("Active"), 1, GUEDIni );				

				GConfig->SetFloat( *ViewportName, TEXT("PctLeft"), GViewports(x).PctLeft, GUEDIni );
				GConfig->SetFloat( *ViewportName, TEXT("PctTop"), GViewports(x).PctTop, GUEDIni );
				GConfig->SetFloat( *ViewportName, TEXT("PctRight"), GViewports(x).PctRight, GUEDIni );
				GConfig->SetFloat( *ViewportName, TEXT("PctBottom"), GViewports(x).PctBottom, GUEDIni );

				GConfig->SetFloat( *ViewportName, TEXT("Left"), GViewports(x).Left, GUEDIni );
				GConfig->SetFloat( *ViewportName, TEXT("Top"), GViewports(x).Top, GUEDIni );
				GConfig->SetFloat( *ViewportName, TEXT("Right"), GViewports(x).Right, GUEDIni );
				GConfig->SetFloat( *ViewportName, TEXT("Bottom"), GViewports(x).Bottom, GUEDIni );				
			}
			else 
			{
				GConfig->SetInt( *ViewportName, TEXT("Active"), 0, GUEDIni );
			}

			if (x < GViewports.Num())
				delete GViewports(x).m_pViewportFrame;
		}

		// "Last Directory"
		GConfig->SetString( TEXT("Directories"), TEXT("PCX"), *GLastDir[eLASTDIR_PCX], GUEDIni );
		GConfig->SetString( TEXT("Directories"), TEXT("WAV"), *GLastDir[eLASTDIR_WAV], GUEDIni );
		GConfig->SetString( TEXT("Directories"), TEXT("BRUSH"), *GLastDir[eLASTDIR_BRUSH], GUEDIni );
		GConfig->SetString( TEXT("Directories"), TEXT("2DS"), *GLastDir[eLASTDIR_2DS], GUEDIni );

		// Background image
		GConfig->SetInt( TEXT("Background Image"), TEXT("Active"), (hImage != NULL), GUEDIni );
		GConfig->SetInt( TEXT("Background Image"), TEXT("Mode"), BIMode, GUEDIni );
		GConfig->SetString( TEXT("Background Image"), TEXT("Filename"), *BIFilename, GUEDIni );

		::DeleteObject( hImage );

		unguard;
	}
	// Looks for an empty viewport slot, allocates a viewport and returns a pointer to it.
	WViewportFrame* NewViewportFrame( const TCHAR* ViewportName, UBOOL bNoSize )
	{
		guard(WLevelFrame::NewViewportFrame);

		// Clean up dead windows first.
		for( int x = 0 ; x < GViewports.Num() ; x++)
			if( GViewports(x).m_pViewportFrame && !::IsWindow( GViewports(x).m_pViewportFrame->hWnd ) )
				GViewports.Remove(x);

		if( GViewports.Num() > dED_MAX_VIEWPORTS )
		{
			appMsgf( TEXT("You are at the limit for open viewports.") );
			return NULL;
		}

		// Check if the specified name is available
		BOOL bIsUnused = 1;
		for( int x = 0 ; x < dED_MAX_VIEWPORTS ; x++)
		{
			// See if this name is already taken			
			for (int y = 0; y < GViewports.Num(); y++)
			{
				if (FObjectName(GViewports(y).m_pViewportFrame->pViewport) == ViewportName)
				{
					bIsUnused = 0;
					break;
				}
			}

			if( bIsUnused )
				break;
		}

		if (!bIsUnused)
			return nullptr;		

		// Create the viewport.
		new(GViewports)VIEWPORTCONFIG();
		INT Index = GViewports.Num() - 1;
		GViewports(Index).PctLeft = 0;
		GViewports(Index).PctTop = 0;
		GViewports(Index).PctRight = bNoSize ? 0 : 50;
		GViewports(Index).PctBottom = bNoSize ? 0 : 50;
		GViewports(Index).Left = 0;
		GViewports(Index).Top = 0;
		GViewports(Index).Right = bNoSize ? 0 : 320;
		GViewports(Index).Bottom = bNoSize ? 0 : 200;
		GViewports(Index).m_pViewportFrame = new WViewportFrame( ViewportName, this );
		GViewports(Index).m_pViewportFrame->m_iIdx = Index;

		// Give focus to this window
#if OLDUNREAL_BINARY_COMPAT
		GCurrentViewport = (DWORD)(GViewports(Index).m_pViewportFrame);
#else
		GCurrentViewport = reinterpret_cast<UViewport*>(GViewports(Index).m_pViewportFrame);
#endif

		return GViewports(Index).m_pViewportFrame;

		unguard;
	}
	// Causes all viewports to redraw themselves.  This is necessary so we can reliably switch
	// which window has the white focus outline.
	void RedrawAllViewports()
	{
		guard(WLevelFrame::RedrawAllViewports);
		for( int x = 0 ; x < GViewports.Num() ; x++)
			InvalidateRect( GViewports(x).m_pViewportFrame->hWnd, NULL, 1 );
		unguard;
	}
	// Changes the visual style of all open viewports to whatever the current style is.  This is also good
	// for forcing all viewports to recompute their positional data.
	void ChangeViewportStyle()
	{
		guard(WLevelFrame::ChangeViewportStyle);

		for( int x = 0 ; x < GViewports.Num() ; x++)
		{
			if( GViewports(x).m_pViewportFrame && ::IsWindow( GViewports(x).m_pViewportFrame->hWnd ) )
			{
				HWND hWnd = GViewports(x).m_pViewportFrame->hWnd;
				if (::IsIconic(hWnd))
					ShowWindow(hWnd, SW_SHOWNOACTIVATE);
				if (::IsZoomed(hWnd))
					ShowWindow(hWnd, SW_SHOWNOACTIVATE);

				switch( GViewportStyle )
				{
					case VSTYLE_Floating:
						SetWindowLongW( hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS );
						break;
					case VSTYLE_Fixed:
						SetWindowLongW( hWnd, GWL_STYLE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS );
						// MSDN: If you have changed certain window data using SetWindowLong, you must call SetWindowPos for the changes to take effect. Use the following combination for uFlags: SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED.
						SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED); 
						break;
				}

				GViewports(x).m_pViewportFrame->ComputePositionData();
				SetWindowPos( hWnd, HWND_TOP, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE );

				GViewports(x).m_pViewportFrame->AdjustToolbarButtons();
			}
		}

		unguard;
	}
	// Resizes all existing viewports to fit properly on the screen.
	void FitViewportsToWindow()
	{
		guard(WLevelFrame::FitViewportsToWindow);
		RECT R;
		if (::GetClientRect(GLevelFrame->hWnd, &R))
		{
			for (int x = 0; x < GViewports.Num(); x++)
			{
				HWND hWnd = GViewports(x).m_pViewportFrame->hWnd;
				if (::IsIconic(hWnd))
					continue;

				if (::IsZoomed(hWnd))
				{
					::MoveWindow(hWnd, R.left, R.top, R.right, R.bottom, 1);
					continue;
				}
				
				VIEWPORTCONFIG* pVC = &(GViewports(GViewports(x).m_pViewportFrame->m_iIdx));
				if (GViewportStyle == VSTYLE_Floating)
					::MoveWindow(hWnd,
						pVC->Left, pVC->Top, pVC->Right, pVC->Bottom, 1);
				else
					::MoveWindow(hWnd,
						pVC->PctLeft * R.right, pVC->PctTop * R.bottom,
						pVC->PctRight * R.right, pVC->PctBottom * R.bottom, 1);
			}
		}
		else GWarn->Logf(TEXT("GetClientRect failed in FitViewportsToWindow"));
		unguard;
	}
	void CreateNewViewports( int _Style, int _Config )
	{
		guard(WLevelFrame::CreateNewViewports);

		GViewportStyle = _Style;
		GViewportConfig = _Config;

		// Get rid of any existing viewports.
		for( int x = 0 ; x < GViewports.Num() ; x++)
		{
			delete GViewports(x).m_pViewportFrame;
			GViewports(x).m_pViewportFrame = NULL;
		}
		GViewports.Empty();

		const DWORD ShowFlags = SHOW_Menu | SHOW_Frame | SHOW_Actors | SHOW_Brush | SHOW_StandardView | SHOW_ChildWindow | SHOW_MovingBrushes;

		// Create new viewports
		switch( GViewportConfig )
		{
			case 0:		// classic
			default:
			{
				GLevelFrame->OpenFrameViewport( TEXT("U2Viewport0"), REN_OrthXY, ShowFlags, TRUE );
				GLevelFrame->OpenFrameViewport( TEXT("U2Viewport1"), REN_OrthXZ, ShowFlags, TRUE );
				GLevelFrame->OpenFrameViewport( TEXT("U2Viewport2"), REN_DynLight, ShowFlags, TRUE );
				GLevelFrame->OpenFrameViewport( TEXT("U2Viewport3"), REN_OrthYZ, ShowFlags, TRUE);
			}
			break;

			case 1:		// big one on buttom, small ones along top
			{
				GLevelFrame->OpenFrameViewport( TEXT("U2Viewport0"), REN_OrthXY, ShowFlags, TRUE);
				GLevelFrame->OpenFrameViewport( TEXT("U2Viewport1"), REN_OrthXZ, ShowFlags, TRUE);
				GLevelFrame->OpenFrameViewport( TEXT("U2Viewport2"), REN_OrthYZ, ShowFlags, TRUE);
				GLevelFrame->OpenFrameViewport( TEXT("U2Viewport3"), REN_DynLight, ShowFlags, TRUE);
			}
			break;

			case 2:		// big one on left side, small along right
			{
				GLevelFrame->OpenFrameViewport( TEXT("U2Viewport0"), REN_DynLight, ShowFlags, TRUE);
				GLevelFrame->OpenFrameViewport( TEXT("U2Viewport1"), REN_OrthXY, ShowFlags, TRUE);
				GLevelFrame->OpenFrameViewport( TEXT("U2Viewport2"), REN_OrthXZ, ShowFlags, TRUE);
				GLevelFrame->OpenFrameViewport( TEXT("U2Viewport3"), REN_OrthYZ, ShowFlags, TRUE);
			}
			break;

			case 3:		// 2 large windows, split horizontally
			{
				GLevelFrame->OpenFrameViewport( TEXT("U2Viewport0"), REN_OrthXY, ShowFlags, TRUE);
				GLevelFrame->OpenFrameViewport( TEXT("U2Viewport1"), REN_DynLight, ShowFlags, TRUE);
			}
			break;

			case 4:		// 2 large windows, split vertically
			{
				GLevelFrame->OpenFrameViewport( TEXT("U2Viewport0"), REN_OrthXY, ShowFlags, TRUE);
				GLevelFrame->OpenFrameViewport( TEXT("U2Viewport1"), REN_DynLight, ShowFlags, TRUE);
			}
			break;

			case 5:		// 1 large window
			{
				GLevelFrame->OpenFrameViewport( TEXT("U2Viewport0"), REN_OrthXY, ShowFlags, TRUE);
			}
			break;
		}

		// Load initial data from templates
		for (int x = 0; x < GViewports.Num(); x++)
		{
			if (GTemplateViewportConfigs[0][x].PctLeft != -1)
			{
				GViewports(x).PctLeft = GTemplateViewportConfigs[GViewportConfig][x].PctLeft;
				GViewports(x).PctTop = GTemplateViewportConfigs[GViewportConfig][x].PctTop;
				GViewports(x).PctRight = GTemplateViewportConfigs[GViewportConfig][x].PctRight;
				GViewports(x).PctBottom = GTemplateViewportConfigs[GViewportConfig][x].PctBottom;
			}
		}

		// Set the viewports to their proper sizes.
		int SaveViewportStyle = VSTYLE_Fixed;
		Exchange( GViewportStyle, SaveViewportStyle );
		FitViewportsToWindow();
		Exchange( SaveViewportStyle, GViewportStyle );
		ChangeViewportStyle();

		unguard;
	}
	// WWindow interface.
	void OnKillFocus( HWND hWndNew )
	{
		guard(WLevelFrame::OnKillFocus);
		GEditor->Client->MakeCurrent( NULL );
		unguard;
	}
	void Serialize( FArchive& Ar )
	{
		guard(WLevelFrame::Serialize);
		WWindow::Serialize( Ar );
		Ar << Level;
		unguard;
	}
	void OpenWindow( UBOOL bMdi, UBOOL bMax )
	{
		guard(WLevelFrame::OpenWindow);
		MdiChild = bMdi;
		PerformCreateWindowEx
		(
			MdiChild
			?	(WS_EX_MDICHILD)
			:	(0),
			TEXT("Level"),
			(bMax ? WS_MAXIMIZE : 0 ) |
			(MdiChild
			?	(WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
			:	(WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS)),
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			MulDiv(512, DPIX, 96),
			MulDiv(384, DPIY, 96),
			MdiChild ? OwnerWindow->OwnerWindow->hWnd : OwnerWindow->hWnd,
			NULL,
			hInstance
		);
		if( !MdiChild )
		{
			SetWindowLongW( hWnd, GWL_STYLE, WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS );
			// MSDN: If you have changed certain window data using SetWindowLong, you must call SetWindowPos for the changes to take effect. Use the following combination for uFlags: SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED.
			SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED); 
			OwnerWindow->Show(1);
		}

		// Load the accelerator table.
		FString HotkeysSet;
		hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(
			GConfig->GetString(TEXT("Options"), TEXT("HotkeysSet"), HotkeysSet, GUEDIni) && HotkeysSet == TEXT("436") ?
			IDR_ACCELERATOR_436 : IDR_ACCELERATOR_469));
		if (hAccel == NULL)
			GWarn->Logf(TEXT("Unable to load accelerators!"));

		WViewportWindowContainer::StaticInit(Level, hWnd);

		// Open the proper configuration of viewports.
		if(!GConfig->GetInt( TEXT("Viewports"), TEXT("Style"), GViewportStyle, GUEDIni ))		GViewportStyle = VSTYLE_Fixed;
		if(!GConfig->GetInt( TEXT("Viewports"), TEXT("Config"), GViewportConfig, GUEDIni ))	GViewportConfig = 0;

		for( int x = 0 ; x < dED_MAX_VIEWPORTS ; x++)
		{
			FString ViewportName = FString::Printf(TEXT("U2Viewport%d"), x);

			INT Active;
			if(!GConfig->GetInt( *ViewportName, TEXT("Active"), Active, GUEDIni ))		Active = 0;

			if( Active )
			{				
				OpenFrameViewport(*ViewportName);
				VIEWPORTCONFIG* pVC = &(GViewports.Last());

				// Refresh viewport config
				if(!GConfig->GetFloat( *ViewportName, TEXT("PctLeft"), pVC->PctLeft, GUEDIni ))	pVC->PctLeft = 0;
				if(!GConfig->GetFloat( *ViewportName, TEXT("PctTop"), pVC->PctTop, GUEDIni ))	pVC->PctTop = 0;
				if(!GConfig->GetFloat( *ViewportName, TEXT("PctRight"), pVC->PctRight, GUEDIni ))	pVC->PctRight = .5f;
				if(!GConfig->GetFloat( *ViewportName, TEXT("PctBottom"), pVC->PctBottom, GUEDIni ))	pVC->PctBottom = .5f;

				if(!GConfig->GetFloat( *ViewportName, TEXT("Left"), pVC->Left, GUEDIni ))	pVC->Left = 0;
				if(!GConfig->GetFloat( *ViewportName, TEXT("Top"), pVC->Top, GUEDIni ))	pVC->Top = 0;
				if(!GConfig->GetFloat( *ViewportName, TEXT("Right"), pVC->Right, GUEDIni ))	pVC->Right = 320;
				if(!GConfig->GetFloat( *ViewportName, TEXT("Bottom"), pVC->Bottom, GUEDIni ))	pVC->Bottom = 200;				
			}
		}

		if (!GConfig->GetString(TEXT("ContextMenuClassAdd"), TEXT("Custom1"), Custom1, GUEDIni))	Custom1.Empty();
		if (!GConfig->GetString(TEXT("ContextMenuClassAdd"), TEXT("Custom2"), Custom2, GUEDIni))	Custom2.Empty();
		if (!GConfig->GetString(TEXT("ContextMenuClassAdd"), TEXT("Custom3"), Custom3, GUEDIni))	Custom3.Empty();
		if (!GConfig->GetString(TEXT("ContextMenuClassAdd"), TEXT("Custom4"), Custom4, GUEDIni))	Custom4.Empty();
		if (!GConfig->GetString(TEXT("ContextMenuClassAdd"), TEXT("Custom5"), Custom5, GUEDIni))	Custom5.Empty();

		FitViewportsToWindow();

		// Background image
		UBOOL bActive;
		if(!GConfig->GetInt( TEXT("Background Image"), TEXT("Active"), bActive, GUEDIni ))	bActive = 0;

		if( bActive )
		{
			if(!GConfig->GetInt( TEXT("Background Image"), TEXT("Mode"), BIMode, GUEDIni ))	BIMode = eBIMODE_CENTER;
			if(!GConfig->GetString( TEXT("Background Image"), TEXT("Filename"), BIFilename, GUEDIni ))	BIFilename.Empty();
			LoadBackgroundImage(BIFilename);
		}

		unguard;
	}
	void LoadBackgroundImage( FString Filename )
	{
		guard(WLevelFrame::LoadBackgroundImage);

		if( hImage ) 
			DeleteObject( hImage );

		hImage = (HBITMAP)LoadImageA( hInstance, appToAnsi( *Filename ), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE );

		if( hImage )
			BIFilename = Filename;
		else
			appMsgf ( TEXT("Error loading bitmap for background image.") );

		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WLevelFrame::OnSize);
		WWindow::OnSize( Flags, NewX, NewY );

		FitViewportsToWindow();

		unguard;
	}
	INT OnSetCursor()
	{
		guard(WLevelFrame::OnSetCursor);
		WWindow::OnSetCursor();
		SetCursor(LoadCursorW(NULL, IDC_ARROW));
		return 0;
		unguard;
	}
	void OnPaint()
	{
		guard(WLevelFrame::OnPaint);
		PAINTSTRUCT PS;
		HDC hDC = BeginPaint( *this, &PS );
		FillRect( hDC, GetClientRect(), (HBRUSH)(COLOR_WINDOW+1) );
		DrawImage( hDC );
		EndPaint( *this, &PS );

		// Put the name of the map into the titlebar.
		SetText( GetMapFilename() );

		unguard;
	}
	void DrawImage( HDC _hdc )
	{
		guard(WLevelFrame::DrawImage);
		if( !hImage ) return;

		HDC hdcMem;
		HBITMAP hbmOld;
		BITMAP bitmap;

		// Prepare the bitmap.
		//
		GetObjectA( hImage, sizeof(BITMAP), (LPSTR)&bitmap );
		hdcMem = CreateCompatibleDC(_hdc);
		hbmOld = (HBITMAP)SelectObject(hdcMem, hImage);

		// Display it.
		//
		RECT l_rc;
		::GetClientRect( hWnd, &l_rc );
		switch( BIMode )
		{
			case eBIMODE_CENTER:
			{
				BitBlt(_hdc,
				   (l_rc.right - bitmap.bmWidth) / 2, (l_rc.bottom - bitmap.bmHeight) / 2,
				   bitmap.bmWidth, bitmap.bmHeight,
				   hdcMem,
				   0, 0,
				   SRCCOPY);
			}
			break;

			case eBIMODE_TILE:
			{
				int XSteps = (int)(l_rc.right / bitmap.bmWidth) + 1;
				int YSteps = (int)(l_rc.bottom / bitmap.bmHeight) + 1;

				for( int x = 0 ; x < XSteps ; x++ )
					for( int y = 0 ; y < YSteps ; y++ )
						BitBlt(_hdc,
						   (x * bitmap.bmWidth), (y * bitmap.bmHeight),
						   bitmap.bmWidth, bitmap.bmHeight,
						   hdcMem,
						   0, 0,
						   SRCCOPY);
			}
			break;

			case eBIMODE_STRETCH:
			{
				StretchBlt(
					_hdc,
				   0, 0,
				   l_rc.right, l_rc.bottom,
				   hdcMem,
				   0, 0,
				   bitmap.bmWidth, bitmap.bmHeight,
				   SRCCOPY);
			}
			break;
		}

		// Clean up.
		//
		SelectObject(hdcMem, hbmOld);
		DeleteDC(hdcMem);
		unguard;
	}

	// Opens a new viewport window.  It creates a viewportframe of the specified size, then creates
	// a viewport that fits inside of it.
	virtual void OpenFrameViewport( const TCHAR* ViewportName, INT RendMap=0, DWORD ShowFlags=0, BOOL ForceReinitialize=FALSE )
	{
		guard(WLevelFrame::OpenFrameViewport);
		
		// Open a viewport frame.
		WViewportFrame* pViewportFrame = NewViewportFrame( ViewportName, 1 );

		if (pViewportFrame)
		{
			pViewportFrame->OpenWindow();
			if (ForceReinitialize)
				pViewportFrame->ForceInitializeViewport(RendMap, ShowFlags);
			else
				pViewportFrame->InitializeViewport();
		}
		
		unguard;
	}

	void OnClose()
	{
		Show(0);
	}
private:

	FString MapFilename;
};



/*-----------------------------------------------------------------------------
	WNewObject.
-----------------------------------------------------------------------------*/

// New object window.
class WNewObject : public WDialog
{
	DECLARE_WINDOWCLASS(WNewObject,WDialog,UnrealEd)

	// Variables.
	WButton OkButton;
	WButton CancelButton;
	WListBox TypeList;
	WObjectProperties Props;
	UObject* Context;
	UObject* Result;
 
	// Constructor.
	WNewObject( UObject* InContext, WWindow* InOwnerWindow )
	:	WDialog		( TEXT("NewObject"), IDDIALOG_NewObject, InOwnerWindow )
	,	OkButton    ( this, IDOK,     FDelegate(this,(TDelegate)&WNewObject::OnOk) )
	,	CancelButton( this, IDCANCEL, FDelegate(this,(TDelegate)&WNewObject::EndDialogFalse) )
	,	TypeList	( this, IDC_TypeList )
	,	Props		( NAME_None, CPF_Edit, TEXT(""), this, 0 )
	,	Context     ( InContext )
	,	Result		( NULL )
	{
		Props.ShowTreeLines = 0;
		TypeList.DoubleClickDelegate=FDelegate(this,(TDelegate)&WNewObject::OnOk);
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WNewObject::OnInitDialog);
		WDialog::OnInitDialog();
		for( TObjectIterator<UClass> It; It; ++It )
		{
			if( It->IsChildOf(UFactory::StaticClass()) )
			{
				UFactory* Default = (UFactory*)It->GetDefaultObject();
				if( Default->bCreateNew )
					TypeList.SetItemData( TypeList.AddString( *Default->Description ), *It );
			}
		}
		Props.OpenChildWindow( IDC_PropHolder );
		TypeList.SetCurrent( 0, 1 );
		TypeList.SelectionChangeDelegate = FDelegate(this,(TDelegate)&WNewObject::OnSelChange);
		OnSelChange();
		unguard;
	}
	void OnDestroy()
	{
		guard(WNewObject::OnDestroy);
		WDialog::OnDestroy();
		unguard;
	}
	virtual UObject* DoModal()
	{
		guard(WNewObject::DoModal);
		WDialog::DoModal( hInstance );
		return Result;
		unguard;
	}

	// Notifications.
	void OnSelChange()
	{
		guard(WNewObject::OnSelChange);
		INT Index = TypeList.GetCurrent();
		if( Index>=0 )
		{
			UClass*   Class   = (UClass*)TypeList.GetItemData(Index);
			UObject*  Factory = ConstructObject<UFactory>( Class );
			Props.Root.SetObjects( &Factory, 1 );
			EnableWindow( OkButton, 1 );
		}
		else
		{
			Props.Root.SetObjects( NULL, 0 );
			EnableWindow( OkButton, 0 );
		}
		unguard;
	}
	void OnOk()
	{
		guard(WNewObject::OnOk);
		if( Props.Root._Objects.Num() )
		{
			UFactory* Factory = CastChecked<UFactory>(Props.Root._Objects(0));
			Result = Factory->FactoryCreateNew( Factory->SupportedClass, NULL, NAME_None, 0, Context, GWarn );
			if( Result )
				EndDialogTrue();
		}
		unguard;
	}

	// WWindow interface.
	void Serialize( FArchive& Ar )
	{
		guard(WNewObject::Serialize);
		WDialog::Serialize( Ar );
		Ar << Context;
		for( INT i=0; i<TypeList.GetCount(); i++ )
		{
			UObject* Obj = (UClass*)TypeList.GetItemData(i);
			Ar << Obj;
		}
		unguard;
	}
};

enum eGI {
	eGI_NUM_SELECTED		= 1,
	eGI_CLASSNAME_SELECTED	= 2,
	eGI_NUM_SURF_SELECTED	= 4,
	eGI_CLASS_SELECTED		= 8
};

typedef struct tag_GetInfoRet {
	int iValue;
	FString String;
	UClass*	pClass;
} t_GetInfoRet;

t_GetInfoRet GetInfo( ULevel* Level, int Item )
{
	guard(GetInfo);

	t_GetInfoRet Ret;

	Ret.iValue = 0;
	Ret.String = TEXT("");

	// ACTORS
	if( Item & eGI_NUM_SELECTED
			|| Item & eGI_CLASSNAME_SELECTED 
			|| Item & eGI_CLASS_SELECTED )
	{
		int NumActors = 0;
		UClass*	BaseClass = NULL; // find lowest common base class.

		for( int i=0; i<Level->Actors.Num(); i++ )
		{
			AActor* Actor = Level->Actors(i);
			if( Actor && Actor->bSelected )
			{
				UClass* AClass = Actor->GetClass();
				if( BaseClass==NULL )	
					BaseClass = AClass;
				else
					while( !AClass->IsChildOf(BaseClass) )
						BaseClass = BaseClass->GetSuperClass();

				NumActors++;
			}
		}

		if( Item & eGI_NUM_SELECTED )
		{
			Ret.iValue = NumActors;
		}
		if( Item & eGI_CLASSNAME_SELECTED )
		{
			Ret.String = FObjectPathName(BaseClass);
		}
		if( Item & eGI_CLASS_SELECTED )
		{
			Ret.pClass = BaseClass;
		}
	}

	// SURFACES
	if( Item & eGI_NUM_SURF_SELECTED)
	{
		int NumSurfs = 0;

		for( INT i=0; i<Level->Model->Surfs.Num(); i++ )
		{
			FBspSurf *Poly = &Level->Model->Surfs(i);

			if( Poly->PolyFlags & PF_Selected )
			{
				NumSurfs++;
			}
		}

		if( Item & eGI_NUM_SURF_SELECTED )
		{
			Ret.iValue = NumSurfs;
		}
	}

	return Ret;

	unguard;
}

void ShowCodeFrame( WWindow* Parent )
{
	if( GCodeFrame
			&& ::IsWindow( GCodeFrame->hWnd ) )
	{
		GCodeFrame->Show(1);
		::BringWindowToTop( GCodeFrame->hWnd );
	}
}


/*-----------------------------------------------------------------------------
	WEditorFrame.
-----------------------------------------------------------------------------*/

// Editor frame window.
class WEditorFrame : public WMdiFrame, public FNotifyHook, public FDocumentManager
{
	DECLARE_WINDOWCLASS(WEditorFrame,WMdiFrame,UnrealEd)

	// Variables.
	WBackgroundHolder BackgroundHolder;
	WConfigProperties* Preferences;
	BOOL RecoveryTitle{};

	// Constructors.
	WEditorFrame()
	: WMdiFrame( TEXT("EditorFrame") )
	, BackgroundHolder( NAME_None, &MdiClient )
	, Preferences( NULL )
	{
	}

	// WWindow interface.
	void OnCreate()
	{
		guard(WEditorFrame::OnCreate);
		WMdiFrame::OnCreate();
#if ENGINE_VERSION==227
		SetText(*FString::Printf(*LocalizeGeneral(TEXT("FrameWindow"), TEXT("UnrealEd")), *LocalizeGeneral(TEXT("Product"), TEXT("Core"))));
#else
		SetText( *FString::Printf( LocalizeGeneral(TEXT("FrameWindow"),TEXT("UnrealEd")), LocalizeGeneral(TEXT("Product"),TEXT("Core"))) );
#endif

		// Create MDI client.
		CLIENTCREATESTRUCT ccs;
        ccs.hWindowMenu = NULL; 
        ccs.idFirstChild = 60000;
		MdiClient.OpenWindow( &ccs );

		// Background.
		BackgroundHolder.OpenWindow();

		NE_EdInit( hWnd, hWnd );

		// Set up progress dialog.
		GDlgProgress = new WDlgProgress( NULL, this );
		GDlgProgress->DoModeless();

		Warn.hWndProgressBar = ::GetDlgItem( GDlgProgress->hWnd, IDPG_PROGRESS);
		Warn.hWndProgressText = ::GetDlgItem( GDlgProgress->hWnd, IDSC_MSG);
		Warn.hWndProgressDlg = GDlgProgress->hWnd;
		Warn.hWndCancelButton = GDlgProgress->CancelButton.hWnd;

		GDlgSearchActors = new WDlgSearchActors( NULL, this );
		GDlgSearchActors->DoModeless();
		GDlgSearchActors->Show(0);

		GDlgScaleLights = new WDlgScaleLights( NULL, this );
		GDlgScaleLights->DoModeless();
		GDlgScaleLights->Show(0);

		GDlgScaleMap = new WDlgScaleMap( NULL, this );
		GDlgScaleMap->DoModeless();
		GDlgScaleMap->Show(0);

		GDlgTexReplace = new WDlgTexReplace( NULL, this );
		GDlgTexReplace->DoModeless();
		GDlgTexReplace->Show(0);

		GDlgArray = new WDlgArray( NULL, this );
		GDlgArray->DoModeless();
		GDlgArray->Show(0);

		GRotateActors = new WDlgRotateActors( NULL, this );
		GRotateActors->DoModeless();
		GRotateActors->Show(0);

		GViewActorText = new WViewText( TEXT("ViewActorText"), this );
		GViewActorText->OpenWindow();
		HFONT hFont = CreateFont (-11, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, TEXT("Courier New"));
		if (!hFont)
			hFont = (HFONT)GetStockObject(ANSI_FIXED_FONT);
		GViewActorText->Display.SetFont(hFont);
		GViewActorText->Show(0);
		SetTimer( hWnd, ID_ViewActorText, 1000, NULL);

		GEditorFrame = this;

		unguard;
	}
	void UpdateActorText()
	{
		guard(WEditorFrame::UpdateActorText);
		if (!GViewActorText->m_bShow)
			return;
		FStringOutputDevice Ar;
		FString Caption;
		if (GEditor && GEditor->Level)
		{	
			TArray<UObject*> SelectedActors;
			int n = 0;
			UClass* BaseClass = NULL;
			for( INT i=0; i<GEditor->Level->Actors.Num(); i++ )
			{
				AActor* Obj = GEditor->Level->Actors(i);
				if (Obj && Obj->bSelected )
				{
					if (BaseClass == NULL)
					{
						BaseClass = Obj->GetClass();
						for( FObjectIterator It; It; ++It )
							It->ClearFlags( RF_TagImp | RF_TagExp );
					}
					while( !Obj->GetClass()->IsChildOf(BaseClass) )
						BaseClass = BaseClass->GetSuperClass();
					Ar.Serialize(*(FString::Printf(TEXT("--- %ls %ls ---\r\n"), Obj->GetClass()->GetName(), *Obj->GetFName())), NAME_None);
					Obj->ExportProperties(Ar, Obj->GetClass(), (BYTE*)Obj, 0, Obj->GetClass(), &Obj->GetClass()->Defaults(0));
					n++;
				}
			}
			if (BaseClass)
			{
				for( FObjectIterator It; It; ++It )
					It->ClearFlags( RF_TagImp | RF_TagExp );
				if (n == 1)
					Caption = FString::Printf( LocalizeGeneral("PropSingle",TEXT("Window")), BaseClass->GetName() );
				else
					Caption = FString::Printf( LocalizeGeneral("PropMulti",TEXT("Window")), BaseClass->GetName(), n );
			}
		}
		if (!Caption.Len())
			Caption = LocalizeGeneral("PropNone",TEXT("Window"));
		GViewActorText->UpdateText(Caption, *Ar);
		unguard;
	}
	virtual void OnTimer()
	{
		guard(WEditorFrame::OnTimer);
		if (LastwParam == ID_ViewActorText)
		{
			UpdateActorText();
			return;
		}
		GEditor->Exec( TEXT("MAYBEAUTOSAVE") );
		GConfig->Flush(1);
		unguard;
	}
	void RepositionClient()
	{
		guard(WEditorFrame::RepositionClient);
		WMdiFrame::RepositionClient();
		BackgroundHolder.MoveWindow( MdiClient.GetClientRect(), 1 );
		unguard;
	}	
	void OnClose()
	{
		guard(WEditorFrame::OnClose);

		GLevelFrame->ChangeViewportStyle();

		::DestroyWindow( GLevelFrame->hWnd );
		delete GLevelFrame;

		KillTimer( hWnd, 900 );

		GMRUList->WriteINI();

		delete GSurfPropSheet;
		delete GBuildSheet;
		delete G2DShapeEditor;
		delete GBrowserSound;
		delete GBrowserMusic;
		delete GBrowserGroup;
		delete GBrowserMaster;
		delete GBrowserActor;
		delete GBrowserTexture;
		delete GBrowserMesh;
		delete GDlgAddSpecial;
		delete GDlgScaleLights;
		delete GDlgArray;
		delete GRotateActors;
		delete GDlgScaleMap;
		delete GDlgProgress;
		delete GDlgSearchActors;
		delete GSelectGroups;
		delete GDlgTexReplace;
		
#if ENGINE_VERSION==227
		appRequestExit(0, TEXT("WEditorFrame: OnClose"));
#else
		appRequestExit( 0 );
#endif
		WMdiFrame::OnClose();
		unguard;
	}
	virtual LRESULT WndProc(UINT Message, WPARAM wParam, LPARAM lParam)
	{
		if (GRecoveryMode && !RecoveryTitle)
		{
			RecoveryTitle = TRUE;
			FString Title = GEditorFrame->GetText();
			INT Pos = Title.InStr(TEXT(" - "));
			if (Pos != -1)
				Title = Title.Left(Pos);
			Title += TEXT(" [Recovery Mode]");
			GEditorFrame->SetText(*Title);
		}
		
		// From Accelerator
		if (Message == WM_COMMAND && HIWORD(wParam) == 1 &&
			GBottomBar && GBottomBar->hwndEdit && GetFocus() == GBottomBar->hwndEdit)
			return 0;

		return WMdiFrame::WndProc(Message, wParam, lParam);
	}
	void OnCommand( INT Command )
	{
		guard(WEditorFrame::OnCommand);

		switch( Command )
		{
			case WM_REDRAWALLVIEWPORTS:
			{
				GEditor->RedrawLevel( GEditor->Level );
				GButtonBar->UpdateButtons();
				GBottomBar->UpdateButtons();
				GTopBar->UpdateButtons();
			}
			break;

			case WM_SETCURRENTVIEWPORT:
			{
#if OLDUNREAL_BINARY_COMPAT
				if (GCurrentViewport != (DWORD)LastlParam && LastlParam)
				{
					GCurrentViewport = (DWORD)LastlParam;
					for (int x = 0; x < GViewports.Num(); x++)
					{
						if (GCurrentViewport == (DWORD)GViewports(x).m_pViewportFrame->pViewport)
						{
							GCurrentViewportFrame = GViewports(x).m_pViewportFrame->hWnd;
							break;
						}
					}
				}
#else
				if (GCurrentViewport != reinterpret_cast<UViewport*>(LastlParam) && LastlParam)
				{
					GCurrentViewport = reinterpret_cast<UViewport*>(LastlParam);
					for (int x = 0; x < GViewports.Num(); x++)
					{
						if (GCurrentViewport == reinterpret_cast<UViewport*>(GViewports(x).m_pViewportFrame->pViewport))
						{
							GCurrentViewportFrame = GViewports(x).m_pViewportFrame->hWnd;
							break;
						}
					}
				}
#endif
				GLevelFrame->RedrawAllViewports();
			}
			break;

			case ID_FileNew:
			case ID_FileNewAdd:
			{
				if (MapSaveChanges(this) != IDCANCEL)
				{
					GEditor->Exec(*FString::Printf(TEXT("MAP NEW%ls"), Command == ID_FileNewAdd ? TEXT(" ADDITIVE") : TEXT("")));
					GLevelFrame->SetMapFilename(TEXT(""));
					OpenLevelView();
					RefreshEditor();
				}
			}
			break;

			case ID_FILE_IMPORT:
			{
				TArray<FString> Files;
				if (OpenFilesWithDialog(
					*(GLastDir[eLASTDIR_UNR]),
					TEXT("Unreal Text (*.t3d)\0*.t3d\0All Files\0*.*\0\0"),
					TEXT("t3d"),
					TEXT("Import Map"),
					Files,
					FALSE
				))
				{
					WDlgMapImport l_dlg( this );
					if( l_dlg.DoModal( *Files(0) ) )
					{
						GWarn->BeginSlowTask( TEXT("Importing Map"), 1, 0 );
						if (l_dlg.bNewMapCheck)
						{
							GEditor->Exec(*FString::Printf(TEXT("MAP IMPORTADD FILE=\"%ls\""), *Files(0)));
						}
						else
						{
							GLevelFrame->SetMapFilename( TEXT("") );
							OpenLevelView();
							GEditor->Exec(*FString::Printf(TEXT("MAP IMPORT FILE=\"%ls\""), *Files(0)));
						}
						GWarn->EndSlowTask();
						GEditor->RedrawLevel( GEditor->Level );

						GLastDir[eLASTDIR_UNR] = appFilePathName(*Files(0));

						RefreshEditor();
					}
				}

				GFileManager->SetDefaultDirectory(appBaseDir());
			}
			break;

			case ID_FILE_EXPORT:
			{
				FString File;
				FString Name = appFileBaseName(GLevelFrame->GetMapFilename());				
				if( Name.InStr( TEXT(".") ) != -1 )
					Name = Name.Left( Name.InStr( TEXT("."), true ) );
				if (GetSaveNameWithDialog(
					*Name,
					*GLastDir[eLASTDIR_UNR],
					TEXT("Unreal Text (*.t3d)\0*.t3d\0All Files\0*.*\0\0"),
					TEXT("t3d"),
					TEXT("Export Map"),
					File
				))
				{
					GEditor->Exec( TEXT("BRUSHCLIP DELETE") );
					GEditor->Exec( *(FString::Printf(TEXT("MAP EXPORT FILE=\"%ls\""), *File )));
					GLastDir[eLASTDIR_UNR] = appFilePathName(*File);
				}

				GFileManager->SetDefaultDirectory(appBaseDir());
			}
			break;

			case IDMN_MRU1:
			case IDMN_MRU2:
			case IDMN_MRU3:
			case IDMN_MRU4:
			case IDMN_MRU5:
			case IDMN_MRU6:
			case IDMN_MRU7:
			case IDMN_MRU8:
			{
				if (MapSaveChanges(this) != IDCANCEL && 
					GEditor->Exec(*(FString::Printf(TEXT("MAP LOAD FILE=\"%ls\""), *GMRUList->Items[Command - IDMN_MRU1]))))
				{
					GLevelFrame->SetMapFilename((TCHAR*)(*(GMRUList->Items[Command - IDMN_MRU1])));
					RefreshEditor();
					GMRUList->AddItem(GLevelFrame->GetMapFilename());
					GMRUList->AddToMenu(GEditorFrame->hWnd, GMainMenu, 1);
					GMRUList->WriteINI();
					GConfig->Flush(0);
				}
			}
			break;

			case IDMN_LOAD_BACK_IMAGE:
			{
				TArray<FString> Files;
				if (OpenFilesWithDialog(
					*GLastDir[eLASTDIR_UTX],
					TEXT("Bitmaps (*.bmp)\0*.bmp\0All Files\0*.*\0\0"),
					TEXT("bmp"),
					TEXT("Open Image"),
					Files
				))
				{
					GLevelFrame->LoadBackgroundImage(*Files(0));
					GLastDir[eLASTDIR_UTX] = appFilePathName(*Files(0));
				}

				InvalidateRect( GLevelFrame->hWnd, NULL, FALSE );
			}
			break;

			case IDMN_CLEAR_BACK_IMAGE:
			{
				::DeleteObject( GLevelFrame->hImage );
				GLevelFrame->hImage = NULL;
				GLevelFrame->BIFilename = TEXT("");
				InvalidateRect( GLevelFrame->hWnd, NULL, FALSE );
			}
			break;

			case IDMN_BI_CENTER:
			{
				GLevelFrame->BIMode = eBIMODE_CENTER;
				InvalidateRect( GLevelFrame->hWnd, NULL, FALSE );
			}
			break;

			case IDMN_BI_TILE:
			{
				GLevelFrame->BIMode = eBIMODE_TILE;
				InvalidateRect( GLevelFrame->hWnd, NULL, FALSE );
			}
			break;

			case IDMN_BI_STRETCH:
			{
				GLevelFrame->BIMode = eBIMODE_STRETCH;
				InvalidateRect( GLevelFrame->hWnd, NULL, FALSE );
			}
			break;

			case WM_EDC_LOADMAP:
			case ID_FileOpen:
			{
				if (MapSaveChanges(this) != IDCANCEL)
					MapOpen(this);
			}
			break;

			case ID_FileClose:
			{
				if (MapSaveChanges(this) == IDCANCEL)
					break;

				if( GLevelFrame )
				{
					GLevelFrame->_CloseWindow();
					delete GLevelFrame;
					GLevelFrame = NULL;
				}
			}
			break;

			case WM_EDC_SAVEMAP:
			case ID_FileSave:
			{
				MapSave( this );
			}
			break;

			case WM_EDC_SAVEMAPAS:
			case ID_FileSaveAs:
			{
				MapSaveAs( this );
			}
			break;
			
			case WM_BRUSHBUILDER_UPDATE:
			{
				GButtonBar->RefreshBuilders();
			}
			break;

			case ID_BrowserMaster:
			{
				GBrowserMaster->Show(1);
			}
			break;

			case ID_BrowserTexture:
			{
				GBrowserMaster->ShowBrowser(eBROWSER_TEXTURE);
			}
			break;

			case ID_BrowserMesh:
			{
				GBrowserMaster->ShowBrowser(eBROWSER_MESH);
			}
			break;

			case ID_BrowserActor:
			{
				GBrowserMaster->ShowBrowser(eBROWSER_ACTOR);
			}
			break;

			case ID_BrowserSound:
			{
				GBrowserMaster->ShowBrowser(eBROWSER_SOUND);
			}
			break;

			case ID_BrowserMusic:
			{
				GBrowserMaster->ShowBrowser(eBROWSER_MUSIC);
			}
			break;

			case ID_BrowserGroup:
			{
				GBrowserMaster->ShowBrowser(eBROWSER_GROUP);
			}
			break;

			case ID_BrowserPrefabs:
			{
				appMsgf(TEXT("Not implemented yet."));
			}
			break;

			case IDMN_CODE_FRAME:
			{
				GBrowserMaster->ShowBrowser(eBROWSER_ACTOR);
				ShowCodeFrame( this );
			}
			break;

			case ID_FileExit:
			{
				OnClose();
			}
			break;

			case ID_EditUndo:
			{
				GEditor->Exec( TEXT("TRANSACTION UNDO") );
			}
			break;

			case ID_EditRedo:
			{
				GEditor->Exec( TEXT("TRANSACTION REDO") );
			}
			break;

			case ID_EditFind:
			{
				GDlgSearchActors->Show(1);
			}
			break;

			case IDMENU_ActorPopupSelectGroups:
			{
				delete GSelectGroups;
				GSelectGroups = new WSelectGroups(TEXT("SelectGroups"), this);
				GSelectGroups->OpenWindow();
			}
			break;

			case IDMN_EDIT_SCALE_LIGHTS:
			{
				GDlgScaleLights->Show(1);
			}
			break;

			case IDMN_EDIT_SCALE_MAP:
			{
				GDlgScaleMap->Show(1);
			}
			break;

			case IDMN_EDIT_TEX_REPLACE:
			{
				GDlgTexReplace->Show(1);
			}
			break;

			case IDMN_EDIT_ARRAY:
			{
				GDlgArray->Show(1);
			}
			break;

			case IDMN_EDIT_ROTATE_ACTORS:
			{
				GRotateActors->Show(1);
			}
			break;

			case ID_EditDelete:
			{
				GEditor->Exec( TEXT("ACTOR DELETE") );
			}
			break;

			case ID_EditCut:
			{
				GEditor->Exec( TEXT("EDIT CUT") );
			}
			break;

			case ID_EditCopy:
			{
				GEditor->Exec( TEXT("EDIT COPY") );
			}
			break;

			case ID_EditPaste:
			case ID_EditPasteNoOffset:
			{
				GEditor->Exec( TEXT("EDIT PASTE") );
				if (Command == ID_EditPasteNoOffset)
				{
					for( INT i=0; i<GEditor->Level->Actors.Num(); i++ )
					{
						AActor* Actor = GEditor->Level->Actors(i);
						if (Actor && Actor->bSelected && Actor != GEditor->Level->Brush() && (Actor->GetFlags() & RF_Transactional))
						{
							Actor->Modify();
							Actor->Location -= FVector(32.0, 32.0, 32.0);
						}
					}
					GEditor->NoteSelectionChange(GEditor->Level);
				}
			}
			break;

			case ID_EditPastePos:
			{
				GEditor->Exec(TEXT("EDIT PASTEPOS"));
			}
			break;

			case ID_EditSelectAllSurfs:
			{
				GEditor->Exec( TEXT("POLY SELECT ALL") );
			}
			break;

			case ID_SurfProperties:
			{
				GEditor->UpdatePropertiesWindows();
				GSurfPropSheet->Show( TRUE );
			}
			break;

			case ID_ViewLevelProp:
			{
				if( !GEditor->LevelProperties )
				{
					GEditor->LevelProperties = new WObjectProperties( TEXT("LevelProperties"), CPF_Edit, TEXT("Level Properties"), NULL, 1 );
					GEditor->LevelProperties->OpenWindow( hWnd );
					GEditor->LevelProperties->SetNotifyHook( GEditor );
				}
				GEditor->LevelProperties->Root.SetObjects( (UObject**)&GEditor->Level->Actors(0), 1 );
				GEditor->LevelProperties->Show(1);
			}
			break;

			case ID_BrushClip:
			{
				GEditor->Exec( TEXT("BRUSHCLIP") );
				GEditor->RedrawLevel( GEditor->Level );
			}
			break;

			case ID_BrushClipSplit:
			{
				GEditor->Exec( TEXT("BRUSHCLIP SPLIT") );
				GEditor->RedrawLevel( GEditor->Level );
			}
			break;

			case ID_BrushClipFlip:
			{
				GEditor->Exec( TEXT("BRUSHCLIP FLIP") );
				GEditor->RedrawLevel( GEditor->Level );
			}
			break;

			case ID_BrushClipDelete:
			{
				GEditor->Exec( TEXT("BRUSHCLIP DELETE") );
				GEditor->RedrawLevel( GEditor->Level );
			}
			break;

			case ID_BrushAdd:
			{
				GEditor->Exec( TEXT("BRUSH ADD") );
				GEditor->RedrawLevel( GEditor->Level );
			}
			break;

			case ID_BrushSubtract:
			{
				GEditor->Exec( TEXT("BRUSH SUBTRACT") );
				GEditor->RedrawLevel( GEditor->Level );
			}
			break;

			case ID_BrushIntersect:
			{
				GEditor->Exec( TEXT("BRUSH FROM INTERSECTION") );
				GEditor->RedrawLevel( GEditor->Level );
			}
			break;

			case ID_BrushDeintersect:
			{
				GEditor->Exec( TEXT("BRUSH FROM DEINTERSECTION") );
				GEditor->RedrawLevel( GEditor->Level );
			}
			break;

			case ID_BrushAddMover:
			{
				GEditor->Exec( TEXT("BRUSH ADDMOVER") );
				GEditor->RedrawLevel( GEditor->Level );
			}
			break;

			case ID_BrushAddSpecial:
			{
				if( !GDlgAddSpecial )
				{
					GDlgAddSpecial = new WDlgAddSpecial( NULL, GEditorFrame );
					GDlgAddSpecial->DoModeless();
				}
				else
					GDlgAddSpecial->Show(1);
			}
			break;

			case ID_BrushOpen:
			{
				TArray<FString> Files;
				if (OpenFilesWithDialog(
					*GLastDir[eLASTDIR_BRUSH],
					TEXT("Brushes (*.u3d)\0*.u3d\0All Files\0*.*\0\0"),
					TEXT("u3d"),
					TEXT("Open Brush"),
					Files
				))
				{
					GEditor->Exec( *(FString::Printf(TEXT("BRUSH LOAD FILE=\"%ls\""), *Files(0))));
					GEditor->RedrawLevel( GEditor->Level );
					GLastDir[eLASTDIR_BRUSH] = appFilePathName(*Files(0));
				}

				GFileManager->SetDefaultDirectory(appBaseDir());
				GButtonBar->RefreshBuilders();
			}
			break;

			case ID_BrushSaveAs:
			{
				FString File;
				if (GetSaveNameWithDialog(
					TEXT(""),
					*GLastDir[eLASTDIR_BRUSH],
					TEXT("Brushes (*.u3d)\0*.u3d\0All Files\0*.*\0\0"),
					TEXT("u3d"),
					TEXT("Save Brush"),
					File
				))
				{
					GEditor->Exec(*(FString::Printf(TEXT("BRUSH SAVE FILE=\"%ls\""), *File)));
					GLastDir[eLASTDIR_BRUSH] = appFilePathName(*File);
				}
				GFileManager->SetDefaultDirectory(appBaseDir());
			}
			break;

			case ID_BRUSH_IMPORT:
			{
				TArray<FString> Files;
				if (OpenFilesWithDialog(
					*GLastDir[eLASTDIR_BRUSH],
					TEXT("Import Types (*.t3d, *.dxf, *.asc, *.obj)\0*.t3d;*.dxf;*.asc;*.obj\0All Files\0*.*\0\0"),
					TEXT("*.t3d;*.dxf;*.asc;"),
					TEXT("Import Brush"),
					Files,
					FALSE
				))
				{
					WDlgBrushImport l_dlg( NULL, this );
					l_dlg.DoModal( *Files(0) );
					GEditor->RedrawLevel( GEditor->Level );
					GLastDir[eLASTDIR_BRUSH] = appFilePathName(*Files(0));
				}

				GFileManager->SetDefaultDirectory(appBaseDir());
				GButtonBar->RefreshBuilders();
			}
			break;

			case ID_BRUSH_EXPORT:
			{
				FString File;
				if (GetSaveNameWithDialog(
					TEXT(""),
					*GLastDir[eLASTDIR_BRUSH],
					TEXT("Unreal Text (*.t3d)\0*.t3d\0Wavefront OBJ (*.obj)\0*.obj\0All Files\0*.*\0\0"),
					TEXT("*.t3d;*.obj"),
					TEXT("Export Brush"),
					File
				))
				{
					GEditor->Exec( *(FString::Printf(TEXT("BRUSH EXPORT FILE=\"%ls\""), *File )));
					GLastDir[eLASTDIR_BRUSH] = appFilePathName(*File);
				}

				GFileManager->SetDefaultDirectory(appBaseDir());
				GButtonBar->RefreshBuilders();
			}
			break;

			case ID_BuildPlay:
			{
				UClient* Client = GEditor->Client;
				for (int i = 0; i < Client->Viewports.Num(); i++)
				{
					if (Client->Viewports(i)->Actor->ShowFlags & SHOW_PlayerCtrl)
					{
						debugf(TEXT("disabling RealtimePreview")); //disable RealTimePreview - Smirftsch
						Client->Viewports(i)->Actor->ShowFlags ^= SHOW_PlayerCtrl;
					}
				}
				for (int x = 0; x < GViewports.Num(); x++)
				{
					GViewports(x).m_pViewportFrame->VFToolbar->UpdateButtons();
				}
				GConfig->Flush(1);
				PreSave();
				GFileManager->SetDefaultDirectory(appBaseDir());
				PostSave(GEditor->Exec( TEXT("HOOK PLAYMAP") ));
			}
			break;

			case ID_BuildGeometry:
			{
				UBOOL bVisibleOnly = SendMessageA( ::GetDlgItem( GhwndBSPages[eBS_OPTIONS], IDCK_ONLY_VISIBLE), BM_GETCHECK, 0, 0 ) == BST_CHECKED;
				GEditor->Exec( TEXT("HIDELOG") );
				GEditor->Exec( *(FString::Printf(TEXT("MAP REBUILD VISIBLEONLY=%d"), bVisibleOnly) ) );
				GBuildSheet->RefreshStats();
			}
			break;

			case ID_BuildLighting:
			{
				UBOOL bVisibleOnly = SendMessageA( ::GetDlgItem( GhwndBSPages[eBS_OPTIONS], IDCK_ONLY_VISIBLE), BM_GETCHECK, 0, 0 ) == BST_CHECKED;
				UBOOL bSelected = SendMessageA(::GetDlgItem( GhwndBSPages[eBS_OPTIONS], IDCK_SEL_LIGHTS_ONLY), BM_GETCHECK, 0, 0 ) == BST_CHECKED;
				GEditor->Exec( TEXT("HIDELOG") );
				GEditor->Exec( *(FString::Printf(TEXT("LIGHT APPLY SELECTED=%d VISIBLEONLY=%d"), bSelected, bVisibleOnly) ) );
				GBuildSheet->RefreshStats();
				GEditor->Flush(1);
			}
			break;

			case ID_BuildPaths:
			{
				GEditor->Exec( TEXT("HIDELOG") );
				GEditor->Exec( TEXT("PATHS DEFINE") );
				GBuildSheet->RefreshStats();
			}
			break;

			case ID_BuildBSP:
			{
				GBuildSheet->BuildBSP();
				GBuildSheet->RefreshStats();
			}
			break;

			case ID_BuildAll:
			{
				GBuildSheet->Build();
				GBuildSheet->RefreshStats();
			}
			break;

			case ID_BuildOptions:
			{
				GBuildSheet->Show( TRUE );
			}
			break;

			case IDMN_CF_FIND_NEXT:
			{
				if( GLogWindow )
				{
					GLogWindow->Show(1);
					SetFocus( *GLogWindow );
					GLogWindow->Display.ScrollCaret();
				}
			}
			break;

			case ID_Tools2DEditor:
			{
				if (G2DShapeEditor)
					G2DShapeEditor->Show(1);
				else
				{
					G2DShapeEditor = new W2DShapeEditor( TEXT("2D Shape Editor"), this );
					G2DShapeEditor->OpenWindow();
				}
			}
			break;

			case ID_ToolsCleanupLvl:
			{
				GEditor->CleanupMapGarbage(GEditor->Level);
			}
			break;

			case ID_ViewNewFree:
			{
				if (GViewportStyle == VSTYLE_Floating)
				{
					// Get an available viewport name
					for (int x = 0; x < dED_MAX_VIEWPORTS; x++)
					{
						FString ViewportName = FString::Printf(TEXT("U2Viewport%d"), x);
						BOOL bFoundViewport = 0;
						
						// See if this name is already taken			
						for (int y = 0; y < GViewports.Num(); y++)
						{
							if (FObjectName(GViewports(y).m_pViewportFrame->pViewport) == ViewportName)
							{
								bFoundViewport = 1;
								break;
							}
						}

						if (!bFoundViewport)
						{
							// This one is available
							GLevelFrame->OpenFrameViewport(*ViewportName, REN_OrthXY, SHOW_Menu | SHOW_Frame | SHOW_Actors | SHOW_Brush | SHOW_StandardView | SHOW_ChildWindow | SHOW_MovingBrushes, TRUE);
							break;
						}
					}					
				}
			}
			break;

			case IDMN_VIEWPORT_CLOSEALL:
			{
				for( int x = 0 ; x < GViewports.Num() ; x++)
				{
					delete GViewports(x).m_pViewportFrame;
					GViewports(x).m_pViewportFrame = NULL;
				}
				GViewports.Empty();
			}
			break;

			case IDMN_VIEWPORT_FLOATING:
			{
				GViewportStyle = VSTYLE_Floating;
				UpdateMenu();
				GLevelFrame->ChangeViewportStyle();
			}
			break;

			case IDMN_VIEWPORT_FIXED:
			{
				GViewportStyle = VSTYLE_Fixed;
				UpdateMenu();
				GLevelFrame->ChangeViewportStyle();
			}
			break;

			case IDMN_VIEWPORT_CONFIG:
			{
				WDlgViewportConfig l_dlg( NULL, this );
				if( l_dlg.DoModal( GViewportConfig ) )
					GLevelFrame->CreateNewViewports( GViewportStyle, l_dlg.ViewportConfig );
				GEditor->RedrawLevel( GEditor->Level );
			}
			break;

			case ID_ViewPrefs:
			case ID_ToolsPrefs:
			{
				if( !Preferences )
				{
#if ENGINE_VERSION==227
					Preferences = new WConfigProperties( TEXT("Preferences"), *LocalizeGeneral(TEXT("AdvancedOptionsTitle"),TEXT("Window")) );
#else
					Preferences = new WConfigProperties(TEXT("Preferences"), LocalizeGeneral(TEXT("AdvancedOptionsTitle"), TEXT("Window")));
#endif
					Preferences->OpenWindow( *this );
					Preferences->SetNotifyHook( this );
					Preferences->ForceRefresh();
				}
				Preferences->Show(1);
			}
			break;

			case WM_BROWSER_DOCK:
			{
				guard(WM_BROWSER_DOCK);
				int Browsr = LastlParam;
				switch( Browsr )
				{
					case eBROWSER_ACTOR:
						delete GBrowserActor;
						GBrowserActor = new WBrowserActor( TEXT("Actor Browser"), GBrowserMaster, GEditorFrame->hWnd );
						check(GBrowserActor);
						GBrowserActor->OpenWindow( 1 );
						GBrowserMaster->ShowBrowser(eBROWSER_ACTOR);
						break;

					case eBROWSER_GROUP:
						delete GBrowserGroup;
						GBrowserGroup = new WBrowserGroup( TEXT("Group Browser"), GBrowserMaster, GEditorFrame->hWnd );
						check(GBrowserGroup);
						GBrowserGroup->OpenWindow( 1 );
						GBrowserMaster->ShowBrowser(eBROWSER_GROUP);
						break;

					case eBROWSER_MUSIC:
						delete GBrowserMusic;
						GBrowserMusic = new WBrowserMusic( TEXT("Music Browser"), GBrowserMaster, GEditorFrame->hWnd );
						check(GBrowserMusic);
						GBrowserMusic->OpenWindow( 1 );
						GBrowserMaster->ShowBrowser(eBROWSER_MUSIC);
						break;

					case eBROWSER_SOUND:
						delete GBrowserSound;
						GBrowserSound = new WBrowserSound( TEXT("Sound Browser"), GBrowserMaster, GEditorFrame->hWnd );
						check(GBrowserSound);
						GBrowserSound->OpenWindow( 1 );
						GBrowserMaster->ShowBrowser(eBROWSER_SOUND);
						break;

					case eBROWSER_TEXTURE:
						delete GBrowserTexture;
						GBrowserTexture = new WBrowserTexture( TEXT("Texture Browser"), GBrowserMaster, GEditorFrame->hWnd );
						check(GBrowserTexture);
						GBrowserTexture->OpenWindow( 1 );
						GBrowserMaster->ShowBrowser(eBROWSER_TEXTURE);
						break;

					case eBROWSER_MESH:
						delete GBrowserMesh;
						GBrowserMesh = new WBrowserMesh( TEXT("Mesh Browser"), GBrowserMaster, GEditorFrame->hWnd );
						check(GBrowserMesh);
						GBrowserMesh->OpenWindow( 1 );
						GBrowserMaster->ShowBrowser(eBROWSER_MESH);
						break;
				}
				unguard;
			}
			break;

			case WM_BROWSER_UNDOCK:
			{
				guard(WM_BROWSER_UNDOCK);
				int Browsr = LastlParam;
				switch( Browsr )
				{
					case eBROWSER_ACTOR:
						delete GBrowserActor;
						GBrowserActor = new WBrowserActor( TEXT("Actor Browser"), GEditorFrame, GEditorFrame->hWnd );
						check(GBrowserActor);
						GBrowserActor->OpenWindow( 0 );
						GBrowserMaster->ShowBrowser(eBROWSER_ACTOR);
						break;

					case eBROWSER_GROUP:
						delete GBrowserGroup;
						GBrowserGroup = new WBrowserGroup( TEXT("Group Browser"), GEditorFrame, GEditorFrame->hWnd );
						check(GBrowserGroup);
						GBrowserGroup->OpenWindow( 0 );
						GBrowserMaster->ShowBrowser(eBROWSER_GROUP);
						break;

					case eBROWSER_MUSIC:
						delete GBrowserMusic;
						GBrowserMusic = new WBrowserMusic( TEXT("Music Browser"), GEditorFrame, GEditorFrame->hWnd );
						check(GBrowserMusic);
						GBrowserMusic->OpenWindow( 0 );
						GBrowserMaster->ShowBrowser(eBROWSER_MUSIC);
						break;

					case eBROWSER_SOUND:
						delete GBrowserSound;
						GBrowserSound = new WBrowserSound( TEXT("Sound Browser"), GEditorFrame, GEditorFrame->hWnd );
						check(GBrowserSound);
						GBrowserSound->OpenWindow( 0 );
						GBrowserMaster->ShowBrowser(eBROWSER_SOUND);
						break;

					case eBROWSER_TEXTURE:
						delete GBrowserTexture;
						GBrowserTexture = new WBrowserTexture( TEXT("Texture Browser"), GEditorFrame, GEditorFrame->hWnd );
						check(GBrowserTexture);
						GBrowserTexture->OpenWindow( 0 );
						GBrowserMaster->ShowBrowser(eBROWSER_TEXTURE);
						break;

					case eBROWSER_MESH:
						delete GBrowserMesh;
						GBrowserMesh = new WBrowserMesh( TEXT("Mesh Browser"), GEditorFrame, GEditorFrame->hWnd );
						check(GBrowserMesh);
						GBrowserMesh->OpenWindow( 0 );
						GBrowserMaster->ShowBrowser(eBROWSER_MESH);
						break;
				}

				GBrowserMaster->RefreshBrowserTabs( -1 );
				unguard;
			}
			break;

			case WM_EDC_CAMMODECHANGE:
			{
				if( GButtonBar )
				{
					GEditor->Exec( TEXT("BRUSHCLIP DELETE") );
					GButtonBar->UpdateButtons();
					GBottomBar->UpdateButtons();
					GTopBar->UpdateButtons();
				}
			}
			break;

			case WM_EDC_PLAYMAP:
			{
				GConfig->Flush(1);
				GEditor->Exec( TEXT("HOOK PLAYMAP") );
			}
			break;

			case WM_EDC_BROWSE:
			{
				if (GEditor->BrowseClass->IsChildOf(UTexture::StaticClass()) ||
					GEditor->BrowseClass->IsChildOf(UPalette::StaticClass()))
					GBrowserMaster->ShowBrowser(eBROWSER_TEXTURE);
				else if (GEditor->BrowseClass->IsChildOf(USound::StaticClass()))
					GBrowserMaster->ShowBrowser(eBROWSER_SOUND);
				else if (GEditor->BrowseClass->IsChildOf(UMusic::StaticClass()))
					GBrowserMaster->ShowBrowser(eBROWSER_MUSIC);
				else if (GEditor->BrowseClass->IsChildOf(UClass::StaticClass()))
					GBrowserMaster->ShowBrowser(eBROWSER_ACTOR);
				else if (GEditor->BrowseClass->IsChildOf(UMesh::StaticClass()))
					GBrowserMaster->ShowBrowser(eBROWSER_MESH);
			}
			break;

			case WM_EDC_USECURRENT:
			{
				FString Cur, CurClass;

				if (GEditor->BrowseClass->IsChildOf(UPalette::StaticClass()))
				{
					if (GEditor->CurrentTexture && GEditor->CurrentTexture->Palette)
					{
						Cur = FObjectPathName(GEditor->CurrentTexture->Palette);
						CurClass = FObjectName(GEditor->CurrentTexture->Palette->GetClass());
					}
				}
				else if (GEditor->BrowseClass->IsChildOf(UTexture::StaticClass()))
				{
					if (GEditor->CurrentTexture)
					{
						Cur = FObjectPathName(GEditor->CurrentTexture);
						CurClass = FObjectName(GEditor->CurrentTexture->GetClass());
					}
				}
				else if (GEditor->BrowseClass->IsChildOf(USound::StaticClass()))
				{
					if (GBrowserSound)
					{
						Cur = GBrowserSound->GetCurrentPathName();
						CurClass = FObjectName(GEditor->BrowseClass);
					}
				}
				else if (GEditor->BrowseClass->IsChildOf(UMusic::StaticClass()))
				{
					if (GBrowserMusic)
					{
						Cur = GBrowserMusic->GetCurrentPathName();
						CurClass = FObjectName(GEditor->BrowseClass);
					}
				}
				else if (GEditor->BrowseClass->IsChildOf(UClass::StaticClass()))
				{
					if (GEditor->CurrentClass)
					{
						Cur = GEditor->CurrentClass->GetPathName();
						CurClass = FObjectName(GEditor->BrowseClass);
					}
				}
				else if (GEditor->BrowseClass->IsChildOf(UMesh::StaticClass()))
				{
					if (GBrowserMesh)
					{
						Cur = GBrowserMesh->GetCurrentMeshName();
						CurClass = FObjectName(GEditor->BrowseClass);
					}
				}
				if (Cur.Len() && GEditor->UseDest)
				{
					FString Tmp = FString::Printf(TEXT("%ls'%ls'"), *CurClass, *Cur);
					GEditor->UseDest->SetValue(*Tmp);
				}
			}
			break;

			case WM_EDC_CURTEXCHANGE:
			{
				if (GSurfPropSheet)
					GSurfPropSheet->RefreshStats();
				if (GBrowserTexture)
				{
					GBrowserTexture->SetCaption();
					GBrowserTexture->pViewport->Repaint(1);
				}
			}
			break;

			case WM_EDC_SELPOLYCHANGE:
			case WM_EDC_SELCHANGE:
			{
				GSurfPropSheet->GetDataFromSurfs1();
				GSurfPropSheet->RefreshStats();
			}
			break;

			case WM_EDC_RTCLICKTEXTURE:
			{
				POINT pt;
				HMENU menu = GetSubMenu( AddHotkeys(LoadMenuW(hInstance, MAKEINTRESOURCEW(IDMENU_BrowserTexture_Context))), 0 );
				::GetCursorPos( &pt );
				TrackPopupMenu( menu,
					TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
					pt.x, pt.y, 0,
					GBrowserTexture->hWnd, NULL);
			}
			break;

			case WM_EDC_RTCLICKPOLY:
			{
				POINT l_point;

				::GetCursorPos(&l_point);
				HMENU l_menu = GetSubMenu(AddHotkeys(LoadMenuW(hInstance, MAKEINTRESOURCEW(IDMENU_SurfPopup))), 0);

				t_GetInfoRet gir = GetInfo(GEditor->Level, eGI_NUM_SURF_SELECTED);

				MenuItemPrintf(l_menu, ID_SurfProperties, gir.iValue);

				if (GEditor->CurrentClass)
					MenuItemPrintf(l_menu, ID_BackdropPopupAddClassHere, *FObjectPathName(GEditor->CurrentClass));
				else
					DeleteMenu(l_menu, ID_BackdropPopupAddClassHere, MF_BYCOMMAND);

				if (GEditor->CurrentTexture)
					MenuItemPrintf(l_menu, ID_SurfPopupApplyTexture, *FObjectName(GEditor->CurrentTexture));
				else
					DeleteMenu(l_menu, ID_SurfPopupApplyTexture, MF_BYCOMMAND);
					
#if ENGINE_VERSION==227					
				if ((GBrowserMesh->GetCurrentMeshName() != NAME_None))
					MenuItemPrintf(l_menu, ID_BackdropPopupAddMeshHere, *GBrowserMesh->GetCurrentMeshName());				
				else
					DeleteMenu(l_menu, ID_BackdropPopupAddMeshHere, MF_BYCOMMAND);
#endif
					
				if (Custom1.Len() > 0)
					MenuItemPrintf(l_menu, ID_BackdropPopupAddCustom1Here, *Custom1);
				else
					DeleteMenu(l_menu, ID_BackdropPopupAddCustom1Here, MF_BYCOMMAND);
				
				if (Custom2.Len() > 0)
					MenuItemPrintf(l_menu, ID_BackdropPopupAddCustom2Here, *Custom2);
				else
					DeleteMenu(l_menu, ID_BackdropPopupAddCustom2Here, MF_BYCOMMAND);
				
				if (Custom3.Len() > 0)
					MenuItemPrintf(l_menu, ID_BackdropPopupAddCustom3Here, *Custom3);
				else
					DeleteMenu(l_menu, ID_BackdropPopupAddCustom3Here, MF_BYCOMMAND);
				
				if (Custom4.Len() > 0)
					MenuItemPrintf(l_menu, ID_BackdropPopupAddCustom4Here, *Custom4);
				else
					DeleteMenu(l_menu, ID_BackdropPopupAddCustom4Here, MF_BYCOMMAND);
				
				if (Custom5.Len() > 0)
					MenuItemPrintf(l_menu, ID_BackdropPopupAddCustom5Here, *Custom5);
				else
					DeleteMenu(l_menu, ID_BackdropPopupAddCustom5Here, MF_BYCOMMAND);

				// See if we have any playerstarts in the level
				if (GEditor->Level)
					for (INT i = 0; i < GEditor->Level->Actors.Num(); ++i)
						if (Cast<APlayerStart>(GEditor->Level->Actors(i)))
						{
							// ok, no need to show the "Add playerstart" item then
							DeleteMenu(l_menu, ID_SurfPopupAddPlayerStart, MF_BYCOMMAND);
							break;
						}

				TrackPopupMenu(l_menu,
					TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
					l_point.x, l_point.y, 0,
					hWnd, NULL);
			}
			break;

			case WM_EDC_RTCLICKACTOR:
			case WM_EDC_RTCLICKWINDOW:
			case WM_EDC_RTCLICKWINDOWCANADD:
			{
				POINT l_point;

				::GetCursorPos(&l_point);

				HMENU l_menu = GetSubMenu(AddHotkeys(LoadMenuW(hInstance, MAKEINTRESOURCEW(Command == WM_EDC_RTCLICKACTOR ?
					IDMENU_ActorPopup : IDMENU_BackdropPopup))), 0);

				if (Command == WM_EDC_RTCLICKACTOR)
				{
					t_GetInfoRet gir = GetInfo(GEditor->Level, eGI_NUM_SELECTED | eGI_CLASSNAME_SELECTED | eGI_CLASS_SELECTED);

					MenuItemPrintf(l_menu, ID_ViewActorProp, *gir.String, gir.iValue);

					MenuItemPrintf(l_menu, IDMENU_ActorPopupSelectAllClass, *gir.String);

					MenuItemPrintf(l_menu, IDMENU_ActorPopupReplace, GEditor->CurrentClass ? *FObjectPathName(GEditor->CurrentClass) : TEXT("..."));

					MenuItemPrintf(l_menu, IDMENU_ActorPopupReplaceNP, GEditor->CurrentClass ? *FObjectPathName(GEditor->CurrentClass) : TEXT("..."));

					EnableMenuItem(l_menu, IDMENU_ActorPopupEditScript, (gir.pClass == NULL));
					EnableMenuItem(l_menu, IDMENU_ActorPopupMakeCurrent, (gir.pClass == NULL));
				}

				if (GEditor->CurrentClass)
					MenuItemPrintf(l_menu, ID_BackdropPopupAddClassHere, *FObjectPathName(GEditor->CurrentClass));
				else
					DeleteMenu(l_menu, ID_BackdropPopupAddClassHere, MF_BYCOMMAND);

#if ENGINE_VERSION==227	
				if ((GBrowserMesh->GetCurrentMeshName() != NAME_None))
					MenuItemPrintf(l_menu, ID_BackdropPopupAddMeshHere, *GBrowserMesh->GetCurrentMeshName());
				else
					DeleteMenu(l_menu, ID_BackdropPopupAddMeshHere, MF_BYCOMMAND);
#endif

				if (Custom1.Len() > 0)
					MenuItemPrintf(l_menu, ID_BackdropPopupAddCustom1Here, *Custom1);
				else
					DeleteMenu(l_menu, ID_BackdropPopupAddCustom1Here, MF_BYCOMMAND);

				if (Custom2.Len() > 0)
					MenuItemPrintf(l_menu, ID_BackdropPopupAddCustom2Here, *Custom2);
				else
					DeleteMenu(l_menu, ID_BackdropPopupAddCustom2Here, MF_BYCOMMAND);

				if (Custom3.Len() > 0)
					MenuItemPrintf(l_menu, ID_BackdropPopupAddCustom3Here, *Custom3);
				else
					DeleteMenu(l_menu, ID_BackdropPopupAddCustom3Here, MF_BYCOMMAND);

				if (Custom4.Len() > 0)
					MenuItemPrintf(l_menu, ID_BackdropPopupAddCustom4Here, *Custom4);
				else
					DeleteMenu(l_menu, ID_BackdropPopupAddCustom4Here, MF_BYCOMMAND);

				if (Custom5.Len() > 0)
					MenuItemPrintf(l_menu, ID_BackdropPopupAddCustom5Here, *Custom5);
				else
					DeleteMenu(l_menu, ID_BackdropPopupAddCustom5Here, MF_BYCOMMAND);

				TrackPopupMenu(l_menu,
					TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
					l_point.x, l_point.y, 0,
					hWnd, NULL);
			}
			break;

			case WM_EDC_MAPCHANGE:
			{
			}
			break;

			case WM_EDC_VIEWPORTUPDATEWINDOWFRAME:
			{
				for( int x = 0 ; x < GViewports.Num() ; x++)
					if( GViewports(x).m_pViewportFrame && ::IsWindow( GViewports(x).m_pViewportFrame->hWnd ) )
						UpdateWindow(GViewports(x).m_pViewportFrame->hWnd);
			}
			break;

			case WM_EDC_SURFPROPS:
			{
				GEditor->UpdatePropertiesWindows();
				GSurfPropSheet->Show( TRUE );
			}
			break;

			//
			// BACKDROP POPUP
			//

			// Root
			case ID_BackdropPopupAddClassHere:
			{
				if (GEditor->CurrentClass)
				{
					GEditor->Exec( *(FString::Printf( TEXT("ACTOR ADD CLASS=%ls"), *FObjectPathName(GEditor->CurrentClass) ) ) );
					GEditor->Exec( TEXT("POLY SELECT NONE") );
				}
			}
			break;

			case ID_BackdropPopupAddMeshHere:
			{
				GEditor->Exec(*(FString::Printf(TEXT("ACTOR ADD MESH NAME=%ls SNAP=1"), *GBrowserMesh->GetCurrentMeshName())));
				GEditor->Exec(TEXT("POLY SELECT NONE"));
			}
			break;

			case ID_BackdropPopupAddLightHere:
			{
				GEditor->Exec( TEXT("ACTOR ADD CLASS=ENGINE.LIGHT") );
				GEditor->Exec( TEXT("POLY SELECT NONE") );
			}
			break;

			case ID_BackdropPopupMoveBrushHere:
			{
				if (GEditor && GEditor->Level && GEditor->Level->Brush())
				{
					FVector Location  = GEditor->ClickLocation;
					GEditor->Constraints.Snap( Location, FVector(0, 0, 0) );
					GEditor->Level->Brush()->Location = Location;
					GEditor->RedrawLevel(GEditor->Level);
				}
			}
			break;

			case ID_BackdropPopupAddCustom1Here:
			{
				GEditor->Exec(*(FString::Printf(TEXT("ACTOR ADD CLASS=%ls"), *Custom1)));
				GEditor->Exec(TEXT("POLY SELECT NONE"));
			}
			break;

			case ID_BackdropPopupAddCustom2Here:
			{
				GEditor->Exec(*(FString::Printf(TEXT("ACTOR ADD CLASS=%ls"), *Custom2)));
				GEditor->Exec(TEXT("POLY SELECT NONE"));
			}
			break;

			case ID_BackdropPopupAddCustom3Here:
			{
				GEditor->Exec(*(FString::Printf(TEXT("ACTOR ADD CLASS=%ls"), *Custom3)));
				GEditor->Exec(TEXT("POLY SELECT NONE"));
			}
			break;

			case ID_BackdropPopupAddCustom4Here:
			{
				GEditor->Exec(*(FString::Printf(TEXT("ACTOR ADD CLASS=%ls"), *Custom4)));
				GEditor->Exec(TEXT("POLY SELECT NONE"));
			}
			break;

			case ID_BackdropPopupAddCustom5Here:
			{
				GEditor->Exec(*(FString::Printf(TEXT("ACTOR ADD CLASS=%ls"), *Custom5)));
				GEditor->Exec(TEXT("POLY SELECT NONE"));
			}
			break;

			case ID_BackdropMakeStaticCollision:
			{
				GEditor->Exec(TEXT("MESH STATICCOLLISION"));
			}
			break;

			case ID_BackdropPopupLevelProperties:
			{
				if( !GEditor->LevelProperties )
				{
					GEditor->LevelProperties = new WObjectProperties( TEXT("LevelProperties"), CPF_Edit, TEXT("Level Properties"), NULL, 1 );
					GEditor->LevelProperties->OpenWindow( hWnd );
					GEditor->LevelProperties->SetNotifyHook( GEditor );
				}
				GEditor->LevelProperties->Root.SetObjects( (UObject**)&GEditor->Level->Actors(0), 1 );
				GEditor->LevelProperties->Show(1);
			}
			break;			

			// Grid
			case ID_BackdropPopupGrid1:
			{
				GEditor->Exec( TEXT("MAP GRID X=1 Y=1 Z=1") );
			}
			break;

			case ID_BackdropPopupGrid2:
			{
				GEditor->Exec( TEXT("MAP GRID X=2 Y=2 Z=2") );
			}
			break;

			case ID_BackdropPopupGrid4:
			{
				GEditor->Exec( TEXT("MAP GRID X=4 Y=4 Z=4") );
			}
			break;

			case ID_BackdropPopupGrid8:
			{
				GEditor->Exec( TEXT("MAP GRID X=8 Y=8 Z=8") );
			}
			break;

			case ID_BackdropPopupGrid16:
			{
				GEditor->Exec( TEXT("MAP GRID X=16 Y=16 Z=16") );
			}
			break;

			case ID_BackdropPopupGrid32:
			{
				GEditor->Exec( TEXT("MAP GRID X=32 Y=32 Z=32") );
			}
			break;

			case ID_BackdropPopupGrid64:
			{
				GEditor->Exec( TEXT("MAP GRID X=64 Y=64 Z=64") );
			}
			break;

			case ID_BackdropPopupGrid128:
			{
				GEditor->Exec( TEXT("MAP GRID X=128 Y=128 Z=128") );
			}
			break;

			case ID_BackdropPopupGrid256:
			{
				GEditor->Exec( TEXT("MAP GRID X=256 Y=256 Z=256") );
			}
			break;

			case ID_BackdropPopupGridDisabled:
			{
				GEditor->Exec(TEXT("MAP GRID X=0 Y=0 Z=0"));
			}
			break;

			case ID_BackdropPopupGridCustom:
			{
				WDlgCustomGrid dlg(NULL, this);
				if (dlg.DoModal())
					GEditor->Exec(*FString::Printf(TEXT("MAP GRID X=%i Y=%i Z=%i"), dlg.Value, dlg.Value, dlg.Value));
			}
			break;

			// Pivot
			case ID_BackdropPopupPivotSnapped:
			{
				GEditor->Exec( TEXT("PIVOT SNAPPED") );
			}
			break;

			case ID_BackdropPopupPivot:
			{
				GEditor->Exec( TEXT("PIVOT HERE") );
			}
			break;

			//
			// SURFACE POPUP MENU
			//

			case ID_SurfPopupAddPlayerStart:
			{
				GEditor->Exec(TEXT("ACTOR ADD CLASS=ENGINE.PLAYERSTART"));
				GEditor->Exec(TEXT("POLY SELECT NONE"));				
			}
			break;

			case ID_SurfPopupApplyTexture:
			{
				GEditor->Exec( TEXT("POLY SETTEXTURE") );
			}
			break;

			case ID_AlignViewportCameras:
			{
				GEditor->Exec(TEXT("CAMERA ALIGN"));
			}
			break;

			// Align Selected
			case ID_SurfPopupAlignFloor:
			{
				GEditor->Exec( TEXT("POLY TEXALIGN FLOOR") );
			}
			break;

			case ID_SurfPopupAlignWallDirection:
			{
				GEditor->Exec( TEXT("POLY TEXALIGN WALLDIR") );
			}
			break;

			case ID_SurfPopupAlignWallPanning:
			{
				GEditor->Exec( TEXT("POLY TEXALIGN WALLPAN") );
			}
			break;

			case ID_SurfPopupUnalign:
			{
				GEditor->Exec( TEXT("POLY TEXALIGN DEFAULT") );
			}
			break;

			case ID_SurfPopupAlignWallX:
			{
				GEditor->Exec(TEXT("POLY TEXALIGN WALLX"));
			}
			break;

			case ID_SurfPopupAlignWallY:
			{
				GEditor->Exec(TEXT("POLY TEXALIGN WALLY"));
			}
			break;

			case ID_SurfPopupAlignWalls:
			{
				GEditor->Exec(TEXT("POLY TEXALIGN WALLS"));
			}
			break;

			case ID_SurfPopupAlignAuto:
			{
				GEditor->Exec(TEXT("POLY TEXALIGN AUTO"));
			}
			break;

			case ID_SurfPopupAlignWallColumn:
				GEditor->Exec(TEXT("POLY TEXALIGN WALLCOLUMN"));
				break;

			case ID_SurfPopupAlignOneTile:
				GEditor->Exec(TEXT("POLY TEXALIGN ONETILE"));
				break;

			case ID_SurfPopupAlignOneTileU:
				GEditor->Exec(TEXT("POLY TEXALIGN ONETILE U"));
				break;

			case ID_SurfPopupAlignOneTileV:
				GEditor->Exec(TEXT("POLY TEXALIGN ONETILE V"));
				break;

			// Select Surfaces
			case ID_SurfPopupSelectMatchingZones:
			{
				GEditor->Exec( TEXT("POLY SELECT ZONE") );
			}
			break;
			
			case ID_SurfPopupSelectMatchingGroups:
			{
				GEditor->Exec( TEXT("POLY SELECT MATCHING GROUPS") );
			}
			break;

			case ID_SurfPopupSelectMatchingItems:
			{
				GEditor->Exec( TEXT("POLY SELECT MATCHING ITEMS") );
			}
			break;

			case ID_SurfPopupSelectMatchingBrush:
			{
				GEditor->Exec( TEXT("POLY SELECT MATCHING BRUSH") );
			}
			break;

			case ID_SurfPopupSelectMatchingTexture:
			{
				GEditor->Exec( TEXT("POLY SELECT MATCHING TEXTURE") );
			}
			break;

			case ID_SurfPopupSelectMatchingPolyFlags:
			{
				GEditor->Exec(TEXT("POLY SELECT MATCHING POLYFLAGS"));
			}
			break;

			case ID_SurfPopupSelectAllAdjacents:
			{
				GEditor->Exec( TEXT("POLY SELECT ADJACENT ALL") );
			}
			break;

			case ID_SurfPopupSelectAdjacentCoplanars:
			{
				GEditor->Exec( TEXT("POLY SELECT ADJACENT COPLANARS") );
			}
			break;

			case ID_SurfPopupSelectAdjacentWalls:
			{
				GEditor->Exec( TEXT("POLY SELECT ADJACENT WALLS") );
			}
			break;

			case ID_SurfPopupSelectAdjacentFloors:
			{
				GEditor->Exec( TEXT("POLY SELECT ADJACENT FLOORS") );
			}
			break;

			case ID_SurfPopupSelectAdjacentSlants:
			{
				GEditor->Exec( TEXT("POLY SELECT ADJACENT SLANTS") );
			}
			break;

			case ID_SurfPopupSelectReverse:
			{
				GEditor->Exec( TEXT("POLY SELECT REVERSE") );
			}
			break;

			case ID_SurfPopupMemorize:
			{
				GEditor->Exec( TEXT("POLY SELECT MEMORY SET") );
			}
			break;

			case ID_SurfPopupRecall:
			{
				GEditor->Exec( TEXT("POLY SELECT MEMORY RECALL") );
			}
			break;

			case ID_SurfPopupOr:
			{
				GEditor->Exec( TEXT("POLY SELECT MEMORY INTERSECT") );
			}
			break;

			case ID_SurfPopupAnd:
			{
				GEditor->Exec( TEXT("POLY SELECT MEMORY UNION") );
			}
			break;

			case ID_SurfPopupXor:
			{
				GEditor->Exec( TEXT("POLY SELECT MEMORY XOR") );
			}
			break;

			case ID_SurfPopupReset:
			{
				GEditor->Exec(TEXT("POLY RESET"));
			}
			break;

			case ID_SurfTessellate:
			{
				GEditor->Exec(TEXT("POLY TESSELLATE"));
			}
			break;

			case ID_SurfMakeTextureCurrent:
			{
				GEditor->Exec(TEXT("POLY MAKETEXTURECURRENT"));
			}
			break;

			case ID_ViewActorText:
			{
				UpdateActorText();
				GViewActorText->Show(1);
			}
			break;

			//
			// ACTOR POPUP MENU
			//

			// Root
			case ID_ViewActorProp:
			{
				GEditor->Exec( TEXT("HOOK ACTORPROPERTIES") );
			}
			break;

			case IDMENU_SnapToGrid:
			{
				GEditor->Exec(TEXT("ACTOR SNAPTOGRID"));
			}
			break;

			case IDMENU_ActorPopupSelectAllClass:
			{
				t_GetInfoRet gir = GetInfo( GEditor->Level, eGI_NUM_SELECTED | eGI_CLASSNAME_SELECTED );

				if( gir.iValue )
					GEditor->Exec(*FString::Printf(TEXT("ACTOR SELECT OFCLASS CLASS=%ls"), *gir.String));
			}
			break;

			case IDMENU_ActorPopupSelectMatch:
			{
				GEditor->Exec(TEXT("ACTOR SELECT MATCHING"));
			}
			break;

			case IDMENU_ActorPopupSelectAll:
			{
				GEditor->Exec( TEXT("ACTOR SELECT ALL") );
			}
			break;

			case IDMENU_ActorSelectInside:
			{
				GEditor->Exec(TEXT("ACTOR SELECT INSIDE"));
			}
			break;

			case IDPB_SHOW_SELECTED:
				GEditor->Exec(TEXT("ACTOR HIDE UNSELECTED"));
				GEditor->RedrawLevel( GEditor->Level );
				break;

			case IDPB_HIDE_SELECTED:
				GEditor->Exec(TEXT("ACTOR HIDE SELECTED"));
				break;

			case IDPB_HIDE_INVERT:
				GEditor->Exec(TEXT("ACTOR HIDE INVERT"));
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

			case IDMENU_ActorPopupSelectNone:
			{
				GEditor->Exec( TEXT("SELECT NONE") );
			}
			break;

			case IDMENU_ActorPopupReplace:
			{
				if (GEditor->CurrentClass)
					GEditor->Exec(*FString::Printf(TEXT("ACTOR REPLACE CLASS=%ls"), *FObjectPathName(GEditor->CurrentClass)));
			}
			break;

			case IDMENU_ActorPopupReplaceNP:
			{
				if (GEditor->CurrentClass)
					GEditor->Exec(*FString::Printf(TEXT("ACTOR REPLACE CLASS=%ls KEEP=1"), *FObjectPathName(GEditor->CurrentClass)));
			}
			break;

			case IDMENU_ActorPopupSelectInvert:
			{
				GEditor->Exec(TEXT("ACTOR SELECT INVERT"));
			}
			break;

			case IDPB_CAMERA_SPEED_PLUS:
			{
				if (GEditor->MovementSpeed == 1)
					GEditor->Exec(TEXT("MODE SPEED=4"));
				else if (GEditor->MovementSpeed == 4)
					GEditor->Exec(TEXT("MODE SPEED=16"));
				else GEditor->Exec(TEXT("MODE SPEED=1"));
				GButtonBar->UpdateButtons();
			}
			break;

			case IDPB_CAMERA_SPEED_MINUS:
			{
				if (GEditor->MovementSpeed == 16)
					GEditor->Exec(TEXT("MODE SPEED=4"));
				else if (GEditor->MovementSpeed == 4)
					GEditor->Exec(TEXT("MODE SPEED=1"));
				else GEditor->Exec(TEXT("MODE SPEED=16"));
				GButtonBar->UpdateButtons();
			}
			break;

			case IDMENU_ActorPopupDuplicate:
			case IDMENU_ActorPopupDuplicateNoOffset:
			{
				GEditor->Exec( TEXT("ACTOR DUPLICATE") );
				if (Command == IDMENU_ActorPopupDuplicateNoOffset)
				{
					for( INT i=0; i<GEditor->Level->Actors.Num(); i++ )
					{
						AActor* Actor = GEditor->Level->Actors(i);
						if (Actor && Actor->bSelected && Actor != GEditor->Level->Brush() && (Actor->GetFlags() & RF_Transactional))
						{
							Actor->Modify();
							Actor->Location -= FVector(GEditor->Constraints.GridSize.X, GEditor->Constraints.GridSize.Y, 0);
						}
					}
					GEditor->NoteSelectionChange(GEditor->Level);
				}
			}
			break;

			case IDMENU_ActorPopupEditScript:
			{
				GBrowserMaster->ShowBrowser(eBROWSER_ACTOR);
				t_GetInfoRet gir = GetInfo( GEditor->Level, eGI_CLASS_SELECTED );
				GCodeFrame->AddClass( gir.pClass );
			}
			break;			

			case IDMENU_ActorPopupMakeCurrent:
			{
				t_GetInfoRet gir = GetInfo( GEditor->Level, eGI_CLASSNAME_SELECTED );
				GEditor->Exec( *(FString::Printf(TEXT("SETCURRENTCLASS CLASS=%ls"), *gir.String)) );
				GBrowserActor->SelectClass(GEditor->CurrentClass);
			}
			break;

			case IDMENU_ActorPopupMerge:
			{
				GWarn->BeginSlowTask( TEXT("Merging Faces"), 1, 0 );
				for( int i=0; i<GEditor->Level->Actors.Num(); i++ )
				{
					GWarn->StatusUpdatef( i, GEditor->Level->Actors.Num(), TEXT("Merging Faces") );
					AActor* pActor = GEditor->Level->Actors(i);
					if( pActor && pActor->bSelected && pActor->IsBrush() )
						GEditor->bspValidateBrush( pActor->Brush, 1, 1 );
				}
				GEditor->RedrawLevel( GEditor->Level );
				GWarn->EndSlowTask();
			}
			break;

			case IDMENU_ActorPopupSeparate:
			{
				GWarn->BeginSlowTask( TEXT("Separating Faces"), 1, 0 );
				for( int i=0; i<GEditor->Level->Actors.Num(); i++ )
				{
					GWarn->StatusUpdatef( i, GEditor->Level->Actors.Num(), TEXT("Separating Faces") );
					AActor* pActor = GEditor->Level->Actors(i);
					if( pActor && pActor->bSelected && pActor->IsBrush() )
						GEditor->bspUnlinkPolys( pActor->Brush );
				}
				GEditor->RedrawLevel( GEditor->Level );
				GWarn->EndSlowTask();
			}
			break;

			// Select Brushes
			case IDMENU_ActorPopupSelectBrushesAdd:
			{
				GEditor->Exec( TEXT("MAP SELECT ADDS") );
			}
			break;

			case IDMENU_ActorPopupSelectBrushesSubtract:
			{
				GEditor->Exec( TEXT("MAP SELECT SUBTRACTS") );
			}
			break;

			case IDMENU_ActorPopupSubtractBrushesSemisolid:
			{
				GEditor->Exec( TEXT("MAP SELECT SEMISOLIDS") );
			}
			break;

			case IDMENU_ActorPopupSelectBrushesNonsolid:
			{
				GEditor->Exec( TEXT("MAP SELECT NONSOLIDS") );
			}
			break;

			// Movers
			case IDMN_ActorPopupShowPolys:
			{
				for( INT i=0; i<GEditor->Level->Actors.Num(); i++ )
				{
					ABrush* Brush = Cast<ABrush>(GEditor->Level->Actors(i));
					if( Brush && Brush->IsMovingBrush() && Brush->bSelected )
					{
						Brush->Brush->EmptyModel( 1, 0 );
						Brush->Brush->BuildBound();
						GEditor->bspBuild( Brush->Brush, BSP_Good, 15, 1, 0 );
						GEditor->bspRefresh( Brush->Brush, 1 );
						GEditor->bspValidateBrush( Brush->Brush, 1, 1 );
						GEditor->bspBuildBounds( Brush->Brush );

						GEditor->bspBrushCSG( Brush, GEditor->Level->Model, 0, CSG_Add, 1 );
					}
				}
				GEditor->RedrawLevel( GEditor->Level );
			}
			break;

			case IDMENU_ActorPopupKey0:
			{
				GEditor->Exec( TEXT("ACTOR KEYFRAME NUM=0") );
			}
			break;

			case IDMENU_ActorPopupKey1:
			{
				GEditor->Exec( TEXT("ACTOR KEYFRAME NUM=1") );
			}
			break;

			case IDMENU_ActorPopupKey2:
			{
				GEditor->Exec( TEXT("ACTOR KEYFRAME NUM=2") );
			}
			break;

			case IDMENU_ActorPopupKey3:
			{
				GEditor->Exec( TEXT("ACTOR KEYFRAME NUM=3") );
			}
			break;

			case IDMENU_ActorPopupKey4:
			{
				GEditor->Exec( TEXT("ACTOR KEYFRAME NUM=4") );
			}
			break;

			case IDMENU_ActorPopupKey5:
			{
				GEditor->Exec( TEXT("ACTOR KEYFRAME NUM=5") );
			}
			break;

			case IDMENU_ActorPopupKey6:
			{
				GEditor->Exec( TEXT("ACTOR KEYFRAME NUM=6") );
			}
			break;

			case IDMENU_ActorPopupKey7:
			{
				GEditor->Exec( TEXT("ACTOR KEYFRAME NUM=7") );
			}
			break;

			// Reset
			case IDMENU_ActorPopupResetOrigin:
			{
				GEditor->Exec( TEXT("ACTOR RESET LOCATION") );
			}
			break;

			case IDMENU_ActorPopupResetPivot:
			{
				GEditor->Exec( TEXT("ACTOR RESET PIVOT") );
			}
			break;

			case IDMENU_ActorPopupResetRotation:
			{
				GEditor->Exec( TEXT("ACTOR RESET ROTATION") );
			}
			break;

			case IDMENU_ActorPopupResetScaling:
			{
				GEditor->Exec( TEXT("ACTOR RESET SCALE") );
			}
			break;

			case IDMENU_ActorResetPolyFlags:
			{
				GEditor->Exec(TEXT("ACTOR RESET POLYFLAGS"));
			}
			break;

			case IDMENU_ActorPopupResetAll:
			{
				GEditor->Exec( TEXT("ACTOR RESET ALL") );
			}
			break;

			// Transform
			case IDMENU_ActorPopupMirrorX:
			{
				GEditor->Exec( TEXT("ACTOR MIRROR X=-1") );
			}
			break;

			case IDMENU_ActorPopupMirrorY:
			{
				GEditor->Exec( TEXT("ACTOR MIRROR Y=-1") );
			}
			break;

			case IDMENU_ActorPopupMirrorZ:
			{
				GEditor->Exec( TEXT("ACTOR MIRROR Z=-1") );
			}
			break;

			case IDMENU_ActorPopupPerm:
			{
				GEditor->Exec( TEXT("ACTOR APPLYTRANSFORM") );
			}
			break;

			case IDMENU_ActorAlign:
			{
				GEditor->Exec(TEXT("ACTOR ALIGN"));
			}
			break;

			//Convert
#if ENGINE_VERSION==227
			case IDMENU_ConvertStaticMesh:
			{
				WDlgConvertToStaticMesh l_dlg(NULL, this);
				FString Package = GBrowserMesh->pMeshPackCombo->GetString(GBrowserMesh->pMeshPackCombo->GetCurrent());
				l_dlg.DoModal(Package);
				if (bOk)
				{
					delete GBrowserMesh;
					GBrowserMesh = new WBrowserMesh(TEXT("Mesh Browser"), GBrowserMaster, GEditorFrame->hWnd);
					check(GBrowserMesh);
					GBrowserMesh->OpenWindow(1);
					GBrowserMesh->pMeshPackCombo->SetCurrent(GBrowserMesh->pMeshPackCombo->FindStringExact(*l_dlg.Package));
					GBrowserMesh->pMeshCombo->SetCurrent(GBrowserMesh->pMeshCombo->FindStringExact(*l_dlg.Name));
					GBrowserMesh->RefreshAll();
					GBrowserMaster->ShowBrowser(eBROWSER_MESH);
				}
			}
			break;
			
			case IDMENU_ConvertBrush:
			{
				GEditor->Exec(TEXT("ACTOR CONVERT BRUSH"));
			}
			break;

			case ID_CONVERT_DYNAMIC_ZONE:
			{
				GEditor->Exec(TEXT("ACTOR CONVERT DYNAMICZONE"));
			}
			break;
#endif

			// Order
			case IDMENU_ActorPopupToFirst:
			{
				GEditor->Exec( TEXT("MAP SENDTO FIRST") );
			}
			break;

			case IDMENU_ActorPopupToLast:
			{
				GEditor->Exec( TEXT("MAP SENDTO LAST") );
			}
			break;

			// Copy Polygons
			case IDMENU_ActorPopupToBrush:
			{
				GEditor->Exec( TEXT("MAP BRUSH GET") );
			}
			break;

			case IDMENU_ActorPopupFromBrush:
			{
				GEditor->Exec( TEXT("MAP BRUSH PUT") );
			}
			break;

			// Solidity
			case IDMENU_ActorPopupMakeSolid:
			{
				GEditor->Exec(*FString::Printf(TEXT("MAP SETBRUSH CLEARFLAGS=%d SETFLAGS=%d"), PF_Semisolid + PF_NotSolid, 0));
			}
			break;

			case IDMENU_ActorPopupMakeSemisolid:
			{
				GEditor->Exec(*FString::Printf(TEXT("MAP SETBRUSH CLEARFLAGS=%d SETFLAGS=%d"), PF_Semisolid + PF_NotSolid, PF_Semisolid));
			}
			break;

			case IDMENU_ActorPopupMakeNonSolid:
			{
				GEditor->Exec(*FString::Printf(TEXT("MAP SETBRUSH CLEARFLAGS=%d SETFLAGS=%d"), PF_Semisolid + PF_NotSolid, PF_NotSolid));
			}
			break;

			// CSG
			case IDMENU_ActorPopupMakeAdd:
			{
				GEditor->Exec( *(FString::Printf(TEXT("MAP SETBRUSH CSGOPER=%d"), CSG_Add) ) );
			}
			break;

			case IDMENU_ActorPopupMakeSubtract:
			{
				GEditor->Exec( *(FString::Printf(TEXT("MAP SETBRUSH CSGOPER=%d"), CSG_Subtract) ) );
			}
			break;

			// Help Menu
			case ID_HelpContents:
			{
				appLaunchURL(TEXT("../Help/UnrealEd.chm"));
			}
			break;

			case ID_UEDWebHelp:
			{
				appLaunchURL(TEXT("https://www.oldunreal.com/phpBB3/viewforum.php?f=36"));
			}
			break;

			case ID_EpicWeb:
			{
				appLaunchURL(TEXT("http://www.epicgames.com/"));
			}
			break;

			case ID_HelpWeb:
			{
				appLaunchURL(TEXT("https://www.oldunreal.com/"));
			}
			break;

			case ID_HelpSupport:
			{
				appLaunchURL(TEXT("https://www.oldunreal.com/phpBB3/"));
			}
			break;

			case ID_HelpAbout:
			{
				appMsgf(TEXT("UnrealEd 2.2, original version copyright 2000, Epic Games Inc.\n") 
					TEXT("Updated to version 2.2 by www.oldunreal.com\n\n")
					TEXT("Engine: %d%ls"), GEditor->GetVersion(), *GEditor->GetRevision());
			}
			break;

			case IDMN_VF_REALTIME_PREVIEW:
			{
				INT DisableRealtimePreview = 0;
				for (INT i = 0; i < GEditor->Level->Model->Surfs.Num(); i++)
				{
					FBspSurf& Surf = GEditor->Level->Model->Surfs(i);
					if ((Surf.PolyFlags & PF_FakeBackdrop) && Surf.Actor && Surf.Actor->Region.Zone && Surf.Actor->Region.Zone->IsA(ASkyZoneInfo::StaticClass()))
					{
						appMsgf(TEXT("Can't enable Realtime Preview, PF_Fakebackdrop is set in a Skyzone!"));
						DisableRealtimePreview = 1;
						break;
					}
				}
				if (!DisableRealtimePreview)
				{
					for (int x = 0; x < GViewports.Num(); x++)
					{
#if OLDUNREAL_BINARY_COMPAT
						if (GCurrentViewport == (DWORD)(GViewports(x).m_pViewportFrame->pViewport))
#else
						if (GCurrentViewport == reinterpret_cast<UViewport*>(GViewports(x).m_pViewportFrame->pViewport))
#endif					
						{

							GViewports(x).m_pViewportFrame->pViewport->Actor->ShowFlags ^= SHOW_PlayerCtrl;
							UpdateWindow(GViewports(x).m_pViewportFrame->hWnd);
							GViewports(x).m_pViewportFrame->pViewport->Repaint(1);
							break;
						}
					}
				}
			}
			break;

			case ID_MapWire:
			case ID_MapZones:
			case ID_MapPolys:
			case ID_MapPolyCuts:
			case ID_MapDynLight:
			case ID_MapPlainTex:
			case ID_MapOverhead:
			case ID_MapXZ:
			case ID_MapYZ:
			case ID_LightingOnly:
			{
				for (int x = 0; x < GViewports.Num(); x++)
				{
#if OLDUNREAL_BINARY_COMPAT
					if (GCurrentViewport == (DWORD)(GViewports(x).m_pViewportFrame->pViewport))
#else
					if (GCurrentViewport == reinterpret_cast<UViewport*>(GViewports(x).m_pViewportFrame->pViewport))
#endif					
					{
						GViewports(x).m_pViewportFrame->OnCommand(Command);
						break;
					}
				}
			}
			break;
		
			default:
				WMdiFrame::OnCommand(Command);
			}
		unguard;
	}
	void NotifyDestroy( void* Other )
	{
		if( Other==Preferences )
			Preferences=NULL;
	}

	// FDocumentManager interface.
	virtual void OpenLevelView()
	{
		guard(WEditorFrame::OpenLevelView);

		// This is making it so you can only open one level window - it will reuse it for each
		// map you load ... which is not really MDI.  But the editor has problems with 2+ level windows open.  
		// Fix if you can...
		if( !GLevelFrame )
		{
			GLevelFrame = new WLevelFrame( GEditor->Level, TEXT("LevelFrame"), &BackgroundHolder );
			GLevelFrame->OpenWindow( 1, 1 );
		}

		unguard;
	}
};


void UpdateMenu()
{
	guard(UpdateMenu);

	CheckMenuItem( GMainMenu, IDMN_VIEWPORT_FLOATING, MF_BYCOMMAND | (GViewportStyle == VSTYLE_Floating ? MF_CHECKED : MF_UNCHECKED) );
	CheckMenuItem( GMainMenu, IDMN_VIEWPORT_FIXED, MF_BYCOMMAND | (GViewportStyle == VSTYLE_Fixed ? MF_CHECKED : MF_UNCHECKED) );

	EnableMenuItem( GMainMenu, ID_ViewNewFree, MF_BYCOMMAND | (GViewportStyle == VSTYLE_Floating ? MF_ENABLED : MF_GRAYED) );

	unguard;
}

BOOL AllowOverwrite(const TCHAR* File)
{
	return !GRecoveryMode || GFileManager->FileSize(File) <= 0 || 
		GWarn->YesNof(TEXT("UnrealEd is in Recovery Mode and might be unstable.\n\nThe file ('%ls') you're trying to save might be corrupted.\n\nDo you want to continue?"), File);
}

BOOL MapSaveAs(WWindow* Window)
{
	// Make sure we have a level loaded...
	if (!GLevelFrame) { return FALSE; }

	BOOL Success = FALSE;
	FString File;
	if (Window->GetSaveNameWithDialog(
		GLevelFrame->GetMapFilename(),
		*GLastDir[eLASTDIR_UNR],
		*FString::Printf(TEXT("Map Files (*.%ls)%c*.%ls%cAll Files%c*.*%c%c"), *GMapExt, 0, *GMapExt, 0, 0, 0, 0),
		*GMapExt,
		TEXT("Save Map"),
		File
	) && AllowOverwrite(*File))
	{
		PreSave();
		Success = PostSave(GEditor->Exec(*FString::Printf(TEXT("MAP SAVE FILE=\"%ls\""), *File)));

		// Save the filename.
		if (Success)
		{
			GLevelFrame->SetMapFilename(*File);
			GMRUList->AddItem(GLevelFrame->GetMapFilename());
			GMRUList->AddToMenu(Window->hWnd, GMainMenu, 1);
			GMRUList->WriteINI();
			GConfig->Flush(0);
		}
		else
		{
			appMsgf(TEXT("Failed to save file %ls"), *File);
		}
		GLastDir[eLASTDIR_UNR] = appFilePathName(*File);		
	}

	GFileManager->SetDefaultDirectory(appBaseDir());
	return Success;
}

BOOL MapSave(WWindow* Window)
{
	if (GLevelFrame) 
	{
		if (::appStrlen(GLevelFrame->GetMapFilename()) && !GRecoveryMode)
		{
			PreSave();
			if (PostSave(GEditor->Exec(*(FString::Printf(TEXT("MAP SAVE FILE=\"%ls\""), GLevelFrame->GetMapFilename())))))
			{
				GMRUList->AddItem(GLevelFrame->GetMapFilename());
				GMRUList->AddToMenu(Window->hWnd, GMainMenu, 1);
				GMRUList->WriteINI();
				GConfig->Flush(0);
				return TRUE;
			}			
			appMsgf(TEXT("Failed to save file %ls"), GLevelFrame->GetMapFilename());
		}
		else
		{
			return MapSaveAs(Window);
		}
	}

	return FALSE;
}

static BOOL bAskSaveChanges = FALSE;

INT MapSaveChanges(WWindow* Window)
{
	// If a level has been loaded and there is something in the undo buffer, ask the user
	// if they want to save.
	if (GLevelFrame && GEditor->Trans->CanUndo() && !bAskSaveChanges)
	{
		bAskSaveChanges = TRUE;
		const TCHAR* MapName = GLevelFrame->GetMapFilename();
		INT MessageID = ::MessageBox(Window->hWnd, *FString::Printf(TEXT("Save changes to %ls?"), MapName[0] ? MapName : TEXT("Map")), TEXT("UnrealEd"), MB_YESNOCANCEL);
		bAskSaveChanges = FALSE;
		if (MessageID == IDYES)
		{
			if (MapSave(Window))
				return IDYES;
			return IDCANCEL;			
		}
		return MessageID;
	}
	return IDNO;
}

UBOOL MapOpen(WWindow* Window)
{
	guard(FileOpen);
	BOOL Success = FALSE;
	TArray<FString> Files;
	if (Window->OpenFilesWithDialog(
		*GLastDir[eLASTDIR_UNR],
		*FString::Printf(TEXT("Map Files (*.%ls)%c*.%ls%cAll Files%c*.*%c%c"), *GMapExt, 0, *GMapExt, 0, 0, 0, 0),
		*GMapExt,
		TEXT("Load Map"),
		Files,
		FALSE
	))
	{
		// Make sure there's a level frame open.
		GEditorFrame->OpenLevelView();

		// Convert the ANSI filename to UNICODE, and tell the editor to open it.
		FString S = *Files(0);
		Success = GEditor->Exec(*(FString::Printf(TEXT("MAP LOAD FILE=\"%ls\""), *S)));

		if (Success)
		{
			GLevelFrame->SetMapFilename(*S);
			GMRUList->AddItem(*S);
			GMRUList->AddToMenu(Window->hWnd, GMainMenu, 1);
			GMRUList->WriteINI();
			GConfig->Flush(0);
			GLastDir[eLASTDIR_UNR] = appFilePathName(*S);
		}		
	}

	// Make sure that the browsers reflect any new data the map brought with it.
	RefreshEditor();
	GFileManager->SetDefaultDirectory(appBaseDir());
	return Success;
	unguard;
}

/*-----------------------------------------------------------------------------
	WinMain.
-----------------------------------------------------------------------------*/

//
// Main window entry point.
//
INT WINAPI WinMain( HINSTANCE hInInstance, HINSTANCE hPrevInstance, char* InCmdLine, INT nCmdShow )
{
	// stijn: needs to be initialized early now because we use FStrings everywhere
	Malloc.Init();
	GMalloc = &Malloc;

	// Remember instance.
	GIsStarted = 1;
	hInstance = hInInstance;

	// Set package name.
	// stijn: In the executables, GPackage actually points to a writable TCHAR array 
	// of 64 elements so the const_cast on the next line is fine!
	appStrncpy(const_cast<TCHAR*>(GPackage), appPackage(), 64);

#if __STATIC_LINK
	// Clean lookups.
	for (INT k = 0; k < ARRAY_COUNT(GNativeLookupFuncs); k++)
	{
		GNativeLookupFuncs[k] = NULL;
	}

	// Core lookups.
	INT k = 0;
	GNativeLookupFuncs[k++] = &FindCoreUObjectNative;
	GNativeLookupFuncs[k++] = &FindCoreUCommandletNative;
	GNativeLookupFuncs[k++] = &FindCoreURegistryNative;

	// Engine lookups.
	GNativeLookupFuncs[k++] = &FindEngineAActorNative;
	GNativeLookupFuncs[k++] = &FindEngineAPawnNative;
	GNativeLookupFuncs[k++] = &FindEngineAPlayerPawnNative;
	GNativeLookupFuncs[k++] = &FindEngineADecalNative;
	GNativeLookupFuncs[k++] = &FindEngineAStatLogNative;
	GNativeLookupFuncs[k++] = &FindEngineAStatLogFileNative;
	GNativeLookupFuncs[k++] = &FindEngineAZoneInfoNative;
	GNativeLookupFuncs[k++] = &FindEngineAWarpZoneInfoNative;
	GNativeLookupFuncs[k++] = &FindEngineALevelInfoNative;
	GNativeLookupFuncs[k++] = &FindEngineAGameInfoNative;
	GNativeLookupFuncs[k++] = &FindEngineANavigationPointNative;
	GNativeLookupFuncs[k++] = &FindEngineUCanvasNative;
	GNativeLookupFuncs[k++] = &FindEngineUConsoleNative;
	GNativeLookupFuncs[k++] = &FindEngineUScriptedTextureNative;

	GNativeLookupFuncs[k++] = &FindIpDrvAInternetLinkNative;
	GNativeLookupFuncs[k++] = &FindIpDrvAUdpLinkNative;
	GNativeLookupFuncs[k++] = &FindIpDrvATcpLinkNative;

	// UWeb lookups.
	GNativeLookupFuncs[k++] = &FindUWebUWebResponseNative;
	GNativeLookupFuncs[k++] = &FindUWebUWebRequestNative;

	// UDemo lookups.
	//GNativeLookupFuncs[k++] = &FindudemoUUZHandlerNative;
	//GNativeLookupFuncs[k++] = &FindudemoUudnativeNative;
	//GNativeLookupFuncs[k++] = &FindudemoUDemoInterfaceNative;

	// Editor lookups.
	GNativeLookupFuncs[k++] = &FindEditorUBrushBuilderNative;
	
#endif

	// Begin.
#ifndef _DEBUG
	try
	{
#endif
		// Set mode.
		FString CmdLine = appPlatformBuildCmdLine(1);
		GIsClient = GIsServer = GIsEditor = GLazyLoad = 1;
		GDoCompatibilityChecks = !ParseParam(*CmdLine, TEXT("bytehax")) && !ParseParam(*CmdLine, TEXT("nocompat"));
		GFixCompatibilityIssues = ParseParam(*CmdLine, TEXT("fixcompat"));
		GIsScriptable = 0;

		// Start main loop.
		GIsGuarded=1;
		

		// Create a fully qualified pathname for the log file.  If we don't do this, pieces of the log file
		// tends to get written into various directories as the editor starts up.
		FString TmpFilename = appBaseDir();
		FString TmpLogName;

		if (Parse(*CmdLine, TEXT("LOG="), TmpLogName))
			TmpFilename += TmpLogName;
		else if (Parse(*CmdLine, TEXT("ABSLOG="), TmpLogName))
			TmpFilename = TmpLogName;
		else
			TmpFilename += TEXT("Editor.log");

		Log.Filename = TmpFilename;

		appInit(APP_NAME, *CmdLine, &Malloc, &Log, &Error, &Warn, &FileManager, FConfigCacheIni::Factory, 1 );
		GetPropResult = new FStringOutputDevice;

#if __STATIC_LINK
		AUTO_INITIALIZE_REGISTRANTS_ENGINE;
		AUTO_INITIALIZE_REGISTRANTS_EDITOR;
		//AUTO_INITIALIZE_REGISTRANTS_ALAUDIO;
		//AUTO_INITIALIZE_REGISTRANTS_CLUSTER;
# if __UNREAL_X86__ && !BUILD_64
		AUTO_INITIALIZE_REGISTRANTS_GALAXY;
# endif

		AUTO_INITIALIZE_REGISTRANTS_D3D9DRV;
		AUTO_INITIALIZE_REGISTRANTS_SOFTDRV;
		AUTO_INITIALIZE_REGISTRANTS_OPENGLDRV;
		AUTO_INITIALIZE_REGISTRANTS_XOPENGLDRV;

		AUTO_INITIALIZE_REGISTRANTS_WINDRV;
		AUTO_INITIALIZE_REGISTRANTS_WINDOW;

		AUTO_INITIALIZE_REGISTRANTS_FIRE;
		AUTO_INITIALIZE_REGISTRANTS_IPDRV;
		AUTO_INITIALIZE_REGISTRANTS_UWEB;
		AUTO_INITIALIZE_REGISTRANTS_RENDER;
#endif

		// Init windowing.
		InitWindowing();
		IMPLEMENT_WINDOWCLASS(WMdiFrame,CS_DBLCLKS);
		IMPLEMENT_WINDOWCLASS(WEditorFrame,CS_DBLCLKS);
		IMPLEMENT_WINDOWCLASS(WBackgroundHolder,CS_DBLCLKS);
		IMPLEMENT_WINDOWCLASS(WLevelFrame,CS_DBLCLKS);
		IMPLEMENT_WINDOWCLASS(WDockingFrame,CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW);
		IMPLEMENT_WINDOWCLASS(WCodeFrame,CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW);
		IMPLEMENT_WINDOWCLASS(W2DShapeEditor,CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW);
		IMPLEMENT_WINDOWCLASS(WSelectGroups, CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW);
		IMPLEMENT_WINDOWCLASS(WViewportFrame,CS_DBLCLKS);
		IMPLEMENT_WINDOWCLASS(WBrowser,CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW);
		IMPLEMENT_WINDOWCLASS(WBrowserSound,CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW);
		IMPLEMENT_WINDOWCLASS(WBrowserMusic,CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW);
		IMPLEMENT_WINDOWCLASS(WBrowserGroup,CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW);
		IMPLEMENT_WINDOWCLASS(WBrowserMaster,CS_DBLCLKS  | CS_VREDRAW | CS_HREDRAW);
		IMPLEMENT_WINDOWCLASS(WBrowserTexture,CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW);
		IMPLEMENT_WINDOWCLASS(WBrowserMesh,CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW);
		IMPLEMENT_WINDOWCLASS(WBrowserActor,CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW);
		IMPLEMENT_WINDOWSUBCLASS(WMdiClient,TEXT("MDICLIENT"));
		IMPLEMENT_WINDOWCLASS(WButtonBar,CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW);
		IMPLEMENT_WINDOWCLASS(WButtonGroup,CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW);
		IMPLEMENT_WINDOWCLASS(WBottomBar,CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW);
		IMPLEMENT_WINDOWCLASS(WVFToolBar,CS_DBLCLKS);
		IMPLEMENT_WINDOWCLASS(WTopBar,CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW);
		IMPLEMENT_WINDOWSUBCLASS(WGroupCheckListBox, TEXT("LISTBOX"));

		// Windows.
		WEditorFrame Frame;
		GDocumentManager = &Frame;
		Frame.OpenWindow();
		InvalidateRect( Frame, NULL, 1 );
		UpdateWindow( Frame );
		UBOOL ShowLog = ParseParam(appCmdLine(),TEXT("log"));
		if( !ShowLog && !ParseParam(appCmdLine(),TEXT("server")) )
		InitSplash( TEXT("EdSplash.bmp") );

		// Init.
		GLogWindow = new WLog( *Log.Filename, Log.LogAr->GetAr(), TEXT("EditorLog"), &Frame );
		GLogWindow->OpenWindow( ShowLog, 0 );

		// Init engine.
		GEditor = CastChecked<UEditorEngine>(InitEngine());
		GhwndEditorFrame = GEditorFrame->hWnd;

		// Set up autosave timer.  We ping the engine once a minute and it determines when and 
		// how to do the autosave.
		SetTimer( GEditorFrame->hWnd, 900, 60000, NULL);

		//Check for ini and if not existing, create a new one.
#if ENGINE_VERSION==227
		if (!Parse(appCmdLine(), TEXT("UEDINI="), GUEDIni, 256))
		{
			FString EditorIni = TEXT("");
			if (GConfig->GetString(TEXT("Editor.EditorEngine"), TEXT("EditorIni"), EditorIni, GIni))
			{
				if (EditorIni.Len())
				{
					appSprintf(GUEDIni, TEXT("%ls.ini"), *EditorIni);
				}
				else appSprintf(GUEDIni, TEXT("%ls.ini"), GPackage);
			}
			else appSprintf(GUEDIni, TEXT("%ls.ini"), GPackage);
		}

		debugf(TEXT("Using ini: %ls"), GUEDIni);
		if (GFileManager->FileSize(GUEDIni) < 0)
		{
			// Create UnrealEd.ini from DefaultUnrealEd.ini.
			FString S;
			if (!appLoadFileToString(S, TEXT("DefaultUnrealEd.ini"), GFileManager))
				appErrorf(*LocalizeError(TEXT("MissinGUEDIni")), "DefaultUnrealEd.ini");
			appSaveStringToFile(S, GUEDIni);
		}

		// Create a fully qualified pathname for the log file.  If we don't do this, pieces of the log file
		// tends to get written into various directories as the editor starts up.
		TCHAR chLogFilename[256] = TEXT("\0");

		appSprintf(chLogFilename, TEXT("%ls%ls"), appBaseDir(), GPackage);

		if (ParseParam(appCmdLine(), TEXT("TIMESTAMPLOG")))
		{
			INT Year, Month, DayOfWeek, Day, Hour, Min, Sec, MSec;
			appSystemTime(Year, Month, DayOfWeek, Day, Hour, Min, Sec, MSec);
			FString SystemTime = FString::Printf(TEXT("_%02d-%02d-%02d_%02d-%02d-%02d"), Year, Month, Day, Hour, Min, Sec);
			appStrcat(chLogFilename, *SystemTime);

		}
		appStrcat(chLogFilename, TEXT(".log"));
		appStrcpy(Log.Filename, chLogFilename);
#endif

		// Initialize "last dir" array
		if (!GConfig->GetString(TEXT("Directories"), TEXT("PCX"), GLastDir[eLASTDIR_PCX], GUEDIni))		GLastDir[eLASTDIR_PCX] = TEXT("..\\Textures");
		if (!GConfig->GetString(TEXT("Directories"), TEXT("WAV"), GLastDir[eLASTDIR_WAV], GUEDIni))		GLastDir[eLASTDIR_WAV] = TEXT("..\\Sounds");
		if (!GConfig->GetString(TEXT("Directories"), TEXT("BRUSH"), GLastDir[eLASTDIR_BRUSH], GUEDIni))		GLastDir[eLASTDIR_BRUSH] = TEXT("..\\Maps");
		if (!GConfig->GetString(TEXT("Directories"), TEXT("2DS"), GLastDir[eLASTDIR_2DS], GUEDIni))		GLastDir[eLASTDIR_2DS] = TEXT("..\\Maps");
#if ENGINE_VERSION==227
		if (!GConfig->GetString(TEXT("Directories"), TEXT("USM"), GLastDir[eLASTDIR_USM], GUEDIni))		GLastDir[eLASTDIR_USM] = TEXT("..\\Meshes");
#else
		if (!GConfig->GetString(TEXT("Directories"), TEXT("USM"), GLastDir[eLASTDIR_USM], GUEDIni))		GLastDir[eLASTDIR_USM] = TEXT("..\\System");
#endif
		if (!GConfig->GetString(TEXT("Directories"), TEXT("UMX"), GLastDir[eLASTDIR_UMX], GUEDIni))		GLastDir[eLASTDIR_UMX] = TEXT("..\\Music");
		if (!GConfig->GetString(TEXT("Directories"), TEXT("UAX"), GLastDir[eLASTDIR_UAX], GUEDIni))		GLastDir[eLASTDIR_UAX] = TEXT("..\\Sounds");
		if (!GConfig->GetString(TEXT("Directories"), TEXT("UTX"), GLastDir[eLASTDIR_UTX], GUEDIni))		GLastDir[eLASTDIR_UTX] = TEXT("..\\Textures");
		if (!GConfig->GetString(TEXT("Directories"), TEXT("UNR"), GLastDir[eLASTDIR_UNR], GUEDIni))		GLastDir[eLASTDIR_UNR] = TEXT("..\\Maps");

		if( !GConfig->GetString( TEXT("URL"), TEXT("MapExt"), GMapExt, SYSTEM_INI ) )		GMapExt = TEXT("unr");
		GEditor->Exec( *(FString::Printf(TEXT("MODE MAPEXT=%ls"), *GMapExt ) ) );

		// Init input.
		UInput::StaticInitInput();

		// Toolbar.
		GButtonBar = new WButtonBar( TEXT("EditorToolbar"), &Frame.LeftFrame );
		GButtonBar->OpenWindow();
		Frame.LeftFrame.Dock( GButtonBar );
		Frame.LeftFrame.OnSize( SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE, 0, 0 );

		GBottomBar = new WBottomBar( TEXT("BottomBar"), &Frame.BottomFrame );
		GBottomBar->OpenWindow();
		Frame.BottomFrame.Dock( GBottomBar );
		Frame.BottomFrame.OnSize( SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE, 0, 0 );

		GTopBar = new WTopBar( TEXT("TopBar"), &Frame.TopFrame );
		GTopBar->OpenWindow();
		Frame.TopFrame.Dock( GTopBar );
		Frame.TopFrame.OnSize( SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE, 0, 0 );

		GBrowserMaster = new WBrowserMaster( TEXT("Master Browser"), GEditorFrame );
		GBrowserMaster->OpenWindow( 0 );
		GBrowserMaster->Browsers[eBROWSER_MESH] = (WBrowser**)(&GBrowserMesh);
		GBrowserMaster->Browsers[eBROWSER_MUSIC] = (WBrowser**)(&GBrowserMusic);
		GBrowserMaster->Browsers[eBROWSER_SOUND] = (WBrowser**)(&GBrowserSound);
		GBrowserMaster->Browsers[eBROWSER_ACTOR] = (WBrowser**)(&GBrowserActor);
		GBrowserMaster->Browsers[eBROWSER_GROUP] = (WBrowser**)(&GBrowserGroup);
		GBrowserMaster->Browsers[eBROWSER_TEXTURE] = (WBrowser**)(&GBrowserTexture);
		::InvalidateRect( GBrowserMaster->hWnd, NULL, 1 );
		
		// Open a blank level on startup.
		Frame.OpenLevelView();

		// Reopen whichever windows we need to.
		UBOOL bDocked, bActive;

		if(!GConfig->GetInt( TEXT("Mesh Browser"), TEXT("Docked"), bDocked, GUEDIni ))	bDocked = FALSE;
		SendMessageW( GEditorFrame->hWnd, WM_COMMAND, bDocked ? WM_BROWSER_DOCK : WM_BROWSER_UNDOCK, eBROWSER_MESH );
		if( !bDocked ) 
		{
			if(!GConfig->GetInt( *GBrowserMesh->PersistentName, TEXT("Active"), bActive, GUEDIni ))	bActive = FALSE;
			GBrowserMesh->Show( bActive );
		}
		
		if(!GConfig->GetInt( TEXT("Music Browser"), TEXT("Docked"), bDocked, GUEDIni ))	bDocked = FALSE;
		SendMessageW( GEditorFrame->hWnd, WM_COMMAND, bDocked ? WM_BROWSER_DOCK : WM_BROWSER_UNDOCK, eBROWSER_MUSIC );
		if( !bDocked ) 
		{
			if(!GConfig->GetInt( *GBrowserMusic->PersistentName, TEXT("Active"), bActive, GUEDIni ))	bActive = FALSE;
			GBrowserMusic->Show( bActive );
		}

		if(!GConfig->GetInt( TEXT("Sound Browser"), TEXT("Docked"), bDocked, GUEDIni ))	bDocked = FALSE;
		SendMessageW( GEditorFrame->hWnd, WM_COMMAND, bDocked ? WM_BROWSER_DOCK : WM_BROWSER_UNDOCK, eBROWSER_SOUND );
		if( !bDocked ) 
		{
			if(!GConfig->GetInt( *GBrowserSound->PersistentName, TEXT("Active"), bActive, GUEDIni ))	bActive = FALSE;
			GBrowserSound->Show( bActive );
		}

		if(!GConfig->GetInt( TEXT("Actor Browser"), TEXT("Docked"), bDocked, GUEDIni ))	bDocked = FALSE;
		SendMessageW( GEditorFrame->hWnd, WM_COMMAND, bDocked ? WM_BROWSER_DOCK : WM_BROWSER_UNDOCK, eBROWSER_ACTOR );
		if( !bDocked ) 
		{
			if(!GConfig->GetInt( *GBrowserActor->PersistentName, TEXT("Active"), bActive, GUEDIni ))	bActive = FALSE;
			GBrowserActor->Show( bActive );
		}

		if(!GConfig->GetInt( TEXT("Group Browser"), TEXT("Docked"), bDocked, GUEDIni ))	bDocked = FALSE;
		SendMessageW( GEditorFrame->hWnd, WM_COMMAND, bDocked ? WM_BROWSER_DOCK : WM_BROWSER_UNDOCK, eBROWSER_GROUP );
		if( !bDocked ) 
		{
			if(!GConfig->GetInt( *GBrowserGroup->PersistentName, TEXT("Active"), bActive, GUEDIni ))	bActive = FALSE;
			GBrowserGroup->Show( bActive );
		}

		if(!GConfig->GetInt( TEXT("Texture Browser"), TEXT("Docked"), bDocked, GUEDIni ))	bDocked = FALSE;
		SendMessageW( GEditorFrame->hWnd, WM_COMMAND, bDocked ? WM_BROWSER_DOCK : WM_BROWSER_UNDOCK, eBROWSER_TEXTURE );
		if( !bDocked ) 
		{
			if(!GConfig->GetInt( *GBrowserTexture->PersistentName, TEXT("Active"), bActive, GUEDIni ))	bActive = FALSE;
			GBrowserTexture->Show( bActive );
		}

		GCodeFrame = new WCodeFrame( TEXT("CodeFrame"), GEditorFrame );
		GCodeFrame->OpenWindow( 0, 0 );
		
		//if(!GConfig->GetInt( TEXT("CodeFrame"), TEXT("Active"), bActive, GUEDIni ))	bActive = FALSE;
		//if( bActive )	ShowCodeFrame( GEditorFrame );

		GMainMenu = AddHotkeys(LoadMenuW( hInstance, MAKEINTRESOURCEW(IDMENU_MainMenu )));
		SetMenu( GEditorFrame->hWnd, GMainMenu );
		UpdateMenu();

		GMRUList = new MRUList( TEXT("MRU") );
		GMRUList->ReadINI();
		GMRUList->AddToMenu( GEditorFrame->hWnd, GMainMenu, 1 );

		ExitSplash();

		if (appStrlen(appCmdLine()) && GFileManager->FileSize(appCmdLine()) > 0)
		{
			debugf(TEXT("Loading map %ls"), appCmdLine());
			// Convert the ANSI filename to UNICODE, and tell the editor to open it.
			FString S = appCmdLine();
			BOOL Success = GEditor->Exec(*(FString::Printf(TEXT("MAP LOAD FILE=\"%ls\""), *S)));

			if (Success)
			{
				GLevelFrame->SetMapFilename(*S);
				GMRUList->AddItem(*S);
				GMRUList->AddToMenu(GEditorFrame->hWnd, GMainMenu, 1);
				GMRUList->WriteINI();
				GConfig->Flush(0);
			}

			GLastDir[eLASTDIR_UNR] = S.Left(S.InStr(TEXT("\\"), 1));
		}

		if( !GIsRequestingExit )
			MainLoop( GEditor );

		GDocumentManager=NULL;
		GFileManager->Delete(TEXT("Running.ini"),0,0);
		delete GLogWindow;
		appPreExit();
		GIsGuarded = 0;
		delete GMRUList;
		::DestroyWindow( GCodeFrame->hWnd );
		delete GCodeFrame;
#ifndef _DEBUG
	}
	catch( ... )
	{
		// Crashed.
		Error.HandleError();
	}
#endif

	// Shut down.
	appExit();
	GIsStarted = 0;
	return 0;
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
