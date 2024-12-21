/*=============================================================================
	Main.cpp: UnrealEd Windows startup.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

    Revision history:
		* Created by Tim Sweeney.

    Work-in-progress todo's:

=============================================================================*/


enum eLASTDIR {
	eLASTDIR_UNR	= 0,
	eLASTDIR_UTX	= 1,
	eLASTDIR_PCX	= 2,
	eLASTDIR_UAX	= 3,
	eLASTDIR_WAV	= 4,
	eLASTDIR_BRUSH	= 5,
	eLASTDIR_2DS	= 6,
	eLASTDIR_USM	= 7,
	eLASTDIR_UMX	= 8,
	eLASTDIR_MUS	= 9,
	eLASTDIR_MAX	= 10
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
#include "UnFont.h"
#include "UnEngineNative.h"
#include "UnRender.h"
#include "UnNet.h"

// UPak static stuff.
#include "UPak.h"
#include "UnUPakNative.h"

// PathLogic static stuff.
#include "PathLogicClasses.h"
#include "UnPathLogicNative.h"

// NullNetDriver static stuff
#include "NullNetDriver.h"

// Fire static stuff.
#include "FireClasses.h"
#include "UnFluidSurface.h"
#include "UnProcMesh.h"
#include "UnFractal.h"
#include "UnFireNative.h"

// IpDrv static stuff.
#include "UnIpDrv.h"
#include "UnTcpNetDriver.h"
#include "UnIpDrvCommandlets.h"
#include "UnIpDrvNative.h"
#include "HTTPDownload.h"

#include "EditorPrivate.h"
#include "UnEditorNative.h"

#include "EmitterPrivate.h"
#include "UnEmitterNative.h"

// UWeb static stuff.
#include "UWebAdminPrivate.h"
#include "UnUWebAdminNative.h"

// Renderers
//#include "D3D9Drv.h"
#include "SoftDrvPrivate.h"
#include "XOpenGLDrv.h"
#include "OpenGLDrv.h"

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shellapi.h>

// Windrv stuff
#include "WinDrv.h"

#else

#pragma warning( disable : 4201 )
#define STRICT 1
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>

#include "..\..\Editor\Src\EditorPrivate.h"

#endif 
/*-----------------------------------------------------------------------------
	Document manager crappy abstraction.
-----------------------------------------------------------------------------*/

#include "Window.h"

struct FDocumentManager
{
	virtual void OpenLevelView() = 0;
} *GDocumentManager = NULL;

struct FCmdRCItem
{
	TArray<const TCHAR*> CmdList;

	FMenuItem* AddCmdItem(FPersistentRCMenu* M, const TCHAR* Name, const TCHAR* Cmd, const TCHAR* MenuKey = NULL);
};
FCmdRCItem CmdMenuList;

#include "Res\resource.h"
#include "FConfigCacheIni.h"
#include "UnEngineWin.h"
#include "ViewportWindowContainer.h"

#include "MRUList.h"
#include "DlgProgress.h"
#include "DlgRename.h"
#include "DlgSearchActors.h"
#include "Browser.h"
#include "BrowserMaster.h"
WBrowserMaster* GBrowserMaster = NULL;
#include "CodeFrame.h"
#include "DlgTexProp.h"
#include "DlgBrushBuilder.h"
#include "DlgAddSpecial.h"
#include "DlgScaleLights.h"
#include "DlgTexReplace.h"
#include "SurfPropSheet.h"
#include "BuildSheet.h"
#include "DlgBrushImport.h"
#include "DlgViewportConfig.h"
#include "DlgMapImport.h"
#include "DlgStaticMesh.h"
#include "DlgGridOptions.h"
#include "DlgPackageBrowser.h"
#include "DlgTerrainEdit.h"
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

struct UICommandHook : public FCppCmdHook
{
	UBOOL HookExec(const TCHAR* Cmd, FOutputDevice& Ar);
};

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
	WViewportFrame* pViewportFrame;
} VIEWPORTCONFIG;

// This is a list of all the viewport configs that are currently in effect.
TArray<VIEWPORTCONFIG> GViewports;

// Prefebbed viewport configs.  These should be in the same order as the buttons in DlgViewportConfig.
VIEWPORTCONFIG GTemplateViewportConfigs[4][4] =
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
};
int GViewportStyle, GViewportConfig;

FString GLastDir[eLASTDIR_MAX];
#if __STATIC_LINK
extern FString GMapExt;
#else
EDITOR_API FString GMapExt;
#endif
HMENU GMainMenu = NULL;

extern "C" {HINSTANCE hInstance;}
TCHAR GPackageInternal[64] = TEXT("UnrealEd");
extern "C" {const TCHAR* GPackage = GPackageInternal; }

// Brushes.
HBRUSH hBrushMode = CreateSolidBrush( RGB(0,96,0) );

extern FString GLastText;

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

WCodeFrame* GCodeFrame = NULL;

#include "BrowserActor.h"

WEditorFrame* GEditorFrame = NULL;
WLevelFrame* GLevelFrame = NULL;
W2DShapeEditor* G2DShapeEditor = NULL;
TSurfPropSheet* GSurfPropSheet = NULL;
TBuildSheet* GBuildSheet = NULL;
WBrowserMusic* GBrowserMusic = NULL;
WBrowserGroup* GBrowserGroup = NULL;
WBrowserActor* GBrowserActor = NULL;
WBrowserTexture* GBrowserTexture = NULL;
WDlgAddSpecial* GDlgAddSpecial = NULL;
WDlgScaleLights* GDlgScaleLights = NULL;
WDlgProgress* GDlgProgress = NULL;
WDlgSearchActors* GDlgSearchActors = NULL;
WDlgTexReplace* GDlgTexReplace = NULL;
DlgPackageBrowser* GDlgPackageBrowser = NULL;

#include "ButtonBar.h"
#include "BottomBar.h"
#include "TopBar.h"
WButtonBar* GButtonBar;
WBottomBar* GBottomBar;
WTopBar* GTopBar;

// Custom ContextMenu
FString Custom1, Custom2, Custom3, Custom4, Custom5;

void FileOpen( HWND hWnd );
void FileSaveChanges(HWND hWnd);

void RefreshEditor()
{
	guard(RefreshEditor);
	GBrowserMaster->RefreshAll();
	GBuildSheet->RefreshStats();
	GEditor->TerrainSettings.Terrain = NULL;
	unguard;
}

#include "Menu_RightClickMenu.h"

// Load/Save show flags for viewports.
static FModeEntry ShowFlagConfigs[] = {
	FModeEntry(SHOW_Backdrop,TEXT("ShowBackdrop"),1),
	FModeEntry(SHOW_RealTimeBackdrop,TEXT("ShowRealTimeBackdrop")),
	FModeEntry(SHOW_Coords,TEXT("ShowCoordinates"),1),
	FModeEntry(SHOW_Paths,TEXT("ShowPaths"),1),
	FModeEntry(SHOW_PathsPreview,TEXT("ShowPathsPreview"),1),
	FModeEntry(SHOW_DistanceFog,TEXT("ShowDistanceFog")),
	FModeEntry(SHOW_LightColorIcon,TEXT("ShowLightColorIcon")),
	FModeEntry(SHOW_StaticMeshes,TEXT("ShowStaticMesh")),
	FModeEntry(SHOW_Projectors,TEXT("ShowProjectors")),
	FModeEntry(SHOW_Emitters,TEXT("ShowEmitters")),
	FModeEntry(SHOW_Terrains,TEXT("ShowTerrains")),
	FModeEntry(SHOW_InGameMode,TEXT("InGamePreview"),1),
	FModeEntry(SHOW_Events,TEXT("ShowEvents"),1),
};
static void Load_ShowFlags(INT& Flags, const TCHAR* clName)
{
	UBOOL Flag = 0;
	for (INT i = 0; i < ARRAY_COUNT(ShowFlagConfigs); ++i)
	{
		if (!GConfig->GetBool(clName, ShowFlagConfigs[i].EntryName, Flag, GUEdIni))
			Flag = !ShowFlagConfigs[i].NotID;
		if (Flag)
			Flags |= ShowFlagConfigs[i].ModeID;
		else Flags &= ~ShowFlagConfigs[i].ModeID;
	}
}
static void Save_ShowFlags(const INT Flags, const TCHAR* clName)
{
	for (INT i = 0; i < ARRAY_COUNT(ShowFlagConfigs); ++i)
		GConfig->SetBool(clName, ShowFlagConfigs[i].EntryName, (Flags & ShowFlagConfigs[i].ModeID) != 0, GUEdIni);
}

#include "ViewportFrame.h"
#include "MeshEditor.h"

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
		guard(WMdiFrame::OpenWindow);
		//must make nccreate work!! GetWindowClassName(),
		//!! WS_VSCROLL | WS_HSCROLL
        HWND hWndCreated = CreateWindowEx(0,TEXT("MDICLIENT"),NULL,WS_CHILD|WS_CLIPCHILDREN | WS_CLIPSIBLINGS,0,0,0,0,OwnerWindow->hWnd,(HMENU)0xCAC,hInstance,ccs);
		check(hWndCreated);
		check(!hWnd);
		_Windows.AddItem( this );
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
	,   DockDepth       ( InDockDepth )
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

// An MDI frame window.
class WMdiFrame : public WWindow
{
	DECLARE_WINDOWCLASS(WMdiFrame,WWindow,UnrealEd)

	// Variables.
	WMdiClient MdiClient;
	WDockingFrame LeftFrame, BottomFrame, TopFrame;

	// Functions.
	WMdiFrame( FName InPersistentName )
	:	WWindow		( InPersistentName )
	,	MdiClient	( this )
	,	BottomFrame	( TEXT("MdiFrameBottom"), this, 32 )
	,	LeftFrame	( TEXT("MdiFrameLeft"), this, 68 + GScrollBarWidth )
	,	TopFrame	( TEXT("MdiFrameTop"), this, 32 )
	{}
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
			*FString::Printf(*LocalizeGeneral(TEXT("FrameWindow"), TEXT("UnrealEd")), *LocalizeGeneral(TEXT("Product"), TEXT("Core"))),
			WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_SIZEBOX | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			MulDiv(640, DPIX, 96),
			MulDiv(480, DPIY, 96),
			NULL,
			NULL,
			hInstance
		);
		GSys->MainWindow = *this;
		ShowWindow( *this, SW_SHOWMAXIMIZED );
		unguard;
	}
	void OnSetFocus()
	{
		guard(WMdiFrame::OnSetFocus);
		SetFocus( MdiClient );
		unguard;
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
			GViewports(x).pViewportFrame = NULL;
		GViewports.Empty();
	}
	void SetMapFilename( FString _MapFilename )
	{
		MapFilename = _MapFilename;
		if( ::IsWindow( hWnd ) )
			SetText( *MapFilename );
	}
	FString GetMapFilename()
	{
		return MapFilename;
	}

	void OnDestroy()
	{
		guard(WLevelFrame::OnDestroy);

		if(!GIsCriticalError)
			FileSaveChanges(hWnd);

		for( int group = 0 ; group < GButtonBar->Groups.Num() ; group++ )
			GConfig->SetInt( TEXT("Groups"), *GButtonBar->Groups(group)->GroupName, GButtonBar->Groups(group)->iState, GUEdIni );

		// Save data out to config file, and clean up...
		GConfig->SetInt(TEXT("Viewports"), TEXT("Style"), GViewportStyle, GUEdIni );
		GConfig->SetInt(TEXT("Viewports"), TEXT("Config"), GViewportConfig, GUEdIni );

		for( int x = 0 ; x < GViewports.Num() ; x++)
		{
			TCHAR l_chName[20];
			appSprintf( l_chName, TEXT("U2Viewport%d"), x);

			if( GViewports(x).pViewportFrame
					&& ::IsWindow( GViewports(x).pViewportFrame->hWnd )
					&& !::IsIconic( GViewports(x).pViewportFrame->hWnd )
					&& !::IsZoomed( GViewports(x).pViewportFrame->hWnd ))
			{
				FRect R = GViewports(x).pViewportFrame->GetWindowRect();

				GConfig->SetInt( l_chName, TEXT("Active"), 1, GUEdIni );
				GConfig->SetInt( l_chName, TEXT("RendMap"), GViewports(x).pViewportFrame->pViewport->Actor->RendMap, GUEdIni );

				GConfig->SetFloat( l_chName, TEXT("PctLeft"), GViewports(x).PctLeft, GUEdIni );
				GConfig->SetFloat( l_chName, TEXT("PctTop"), GViewports(x).PctTop, GUEdIni );
				GConfig->SetFloat( l_chName, TEXT("PctRight"), GViewports(x).PctRight, GUEdIni );
				GConfig->SetFloat( l_chName, TEXT("PctBottom"), GViewports(x).PctBottom, GUEdIni );

				GConfig->SetFloat( l_chName, TEXT("Left"), GViewports(x).Left, GUEdIni );
				GConfig->SetFloat( l_chName, TEXT("Top"), GViewports(x).Top, GUEdIni );
				GConfig->SetFloat( l_chName, TEXT("Right"), GViewports(x).Right, GUEdIni );
				GConfig->SetFloat( l_chName, TEXT("Bottom"), GViewports(x).Bottom, GUEdIni );

				FString Device = GViewports(x).pViewportFrame->pViewport->RenDev->GetClass()->GetFullName();
				Device = Device.Right(Device.Len() - Device.InStr(TEXT(" ")) - 1);
				GConfig->SetString( l_chName, TEXT("Device"), *Device, GUEdIni );

				Save_ShowFlags(GViewports(x).pViewportFrame->pViewport->Actor->ShowFlags, l_chName);
			}
			else {

				GConfig->SetInt( l_chName, TEXT("Active"), 0, GUEdIni );
			}


			delete GViewports(x).pViewportFrame;
		}

		// "Last Directory"
		GConfig->SetString( TEXT("Directories"), TEXT("PCX"), *GLastDir[eLASTDIR_PCX], GUEdIni );
		GConfig->SetString( TEXT("Directories"), TEXT("WAV"), *GLastDir[eLASTDIR_WAV], GUEdIni );
		GConfig->SetString( TEXT("Directories"), TEXT("BRUSH"), *GLastDir[eLASTDIR_BRUSH], GUEdIni );
		GConfig->SetString( TEXT("Directories"), TEXT("2DS"), *GLastDir[eLASTDIR_2DS], GUEdIni );

		// Background image
		GConfig->SetInt( TEXT("Background Image"), TEXT("Active"), (hImage != NULL), GUEdIni );
		GConfig->SetInt( TEXT("Background Image"), TEXT("Mode"), BIMode, GUEdIni );
		GConfig->SetString( TEXT("Background Image"), TEXT("Filename"), *BIFilename, GUEdIni );

		::DeleteObject( hImage );

		unguard;
	}
	// Looks for an empty viewport slot, allocates a viewport and returns a pointer to it.
	WViewportFrame* NewViewportFrame(const TCHAR* ViewportName, UBOOL bNoSize)
	{
		guard(WLevelFrame::NewViewportFrame);

		// Clean up dead windows first.
		{
			for (int x = 0; x < GViewports.Num(); x++)
				if (!GViewports(x).pViewportFrame || (GViewports(x).pViewportFrame && !::IsWindow(GViewports(x).pViewportFrame->hWnd)))
					GViewports.Remove(x--);
		}

		if (GViewports.Num() > dED_MAX_VIEWPORTS)
		{
			appMsgf(TEXT("You are at the limit for open viewports."));
			return NULL;
		}

		// Check if the specified name is available
		{
			BOOL bIsUnused = 1;
			for (int x = 0; x < dED_MAX_VIEWPORTS; x++)
			{
				// See if this name is already taken			
				for (int y = 0; y < GViewports.Num(); y++)
				{
					if (FObjectName(GViewports(y).pViewportFrame->pViewport) == ViewportName)
					{
						bIsUnused = 0;
						break;
					}
				}

				if (bIsUnused)
					break;
			}
			if (!bIsUnused)
				return nullptr;
		}

		// Create the viewport.
		auto* View = new(GViewports)VIEWPORTCONFIG();
		View->PctLeft = 0;
		View->PctTop = 0;
		View->PctRight = bNoSize ? 0 : 50;
		View->PctBottom = bNoSize ? 0 : 50;
		View->Left = 0;
		View->Top = 0;
		View->Right = bNoSize ? 0 : 320;
		View->Bottom = bNoSize ? 0 : 200;
		View->pViewportFrame = new WViewportFrame(ViewportName, this);
		View->pViewportFrame->m_iIdx = GViewports.Num() - 1;

		return View->pViewportFrame;

		unguard;
	}

	// Causes all viewports to redraw themselves.  This is necessary so we can reliably switch
	// which window has the white focus outline.
	void RedrawAllViewports()
	{
		guard(WLevelFrame::RedrawAllViewports);
		for (int x = 0; x < GViewports.Num(); x++)
			if (GViewports(x).pViewportFrame)
				GViewports(x).pViewportFrame->ForceRepaint(TRUE);
		unguard;
	}

	// Changes the visual style of all open viewports to whatever the current style is.  This is also good
	// for forcing all viewports to recompute their positional data.
	void ChangeViewportStyle()
	{
		guard(WLevelFrame::ChangeViewportStyle);

		for( int x = 0 ; x < GViewports.Num() ; x++)
		{
			if( GViewports(x).pViewportFrame && ::IsWindow( GViewports(x).pViewportFrame->hWnd ) )
			{
				switch( GViewportStyle )
				{
					case VSTYLE_Floating:
						SetWindowLongPtr( GViewports(x).pViewportFrame->hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS );
						break;
					case VSTYLE_Fixed:
						SetWindowLongPtr( GViewports(x).pViewportFrame->hWnd, GWL_STYLE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS );
						SetWindowPos( GViewports(x).pViewportFrame->hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED); // Regarding MSDN: If you have changed certain window data using SetWindowLong, you must call SetWindowPos for the changes to take effect. Use the following combination for uFlags: SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED.
						break;
				}

				GViewports(x).pViewportFrame->ComputePositionData();
				SetWindowPos( GViewports(x).pViewportFrame->hWnd, HWND_TOP, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE );

				GViewports(x).pViewportFrame->AdjustToolbarButtons();
			}
		}

		unguard;
	}
	// Resizes all existing viewports to fit properly on the screen.
	void FitViewportsToWindow()
	{
		guard(WLevelFrame::FitViewportsToWindow);

		RECT R;
		if (::GetClientRect( GLevelFrame->hWnd, &R ))
		{
			for( int x = 0 ; x < GViewports.Num() ; x++)
			{
				if (!GViewports(x).pViewportFrame)
					continue;
				VIEWPORTCONFIG* pVC = &(GViewports(GViewports(x).pViewportFrame->m_iIdx));
				if( GViewportStyle == VSTYLE_Floating )
					::MoveWindow(GViewports(x).pViewportFrame->hWnd,
						pVC->Left, pVC->Top, pVC->Right, pVC->Bottom, 1);
				else
					::MoveWindow(GViewports(x).pViewportFrame->hWnd,
						pVC->PctLeft * R.right, pVC->PctTop * R.bottom,
						pVC->PctRight * R.right, pVC->PctBottom * R.bottom, 1);
			}
		}
		else GWarn->Logf(TEXT("GetWindowRect failed to get FitViewportsToWindow!"));
		unguard;
	}
	void CreateNewViewports(const int _Style, const int _Config)
	{
		guard(WLevelFrame::CreateNewViewports);

		GViewportStyle = _Style;
		GViewportConfig = _Config;

		// Get rid of any existing viewports.
		for (int x = 0; x < GViewports.Num(); x++)
			delete GViewports(x).pViewportFrame;
		GViewports.Empty();

		// Create new viewports
		switch (GViewportConfig)
		{
		case 0:		// classic
		default:
		{
			GLevelFrame->OpenFrameViewport(TEXT("U2Viewport0"), REN_OrthXY, SHOW_EditorOrtho, TRUE);
			GLevelFrame->OpenFrameViewport(TEXT("U2Viewport1"), REN_OrthXZ, SHOW_EditorOrtho, TRUE);
			GLevelFrame->OpenFrameViewport(TEXT("U2Viewport2"), REN_DynLight, SHOW_EditorMode, TRUE);
			GLevelFrame->OpenFrameViewport(TEXT("U2Viewport3"), REN_OrthYZ, SHOW_EditorOrtho, TRUE);
		}
		break;

		case 1:		// big one on buttom, small ones along top
		{
			GLevelFrame->OpenFrameViewport(TEXT("U2Viewport0"), REN_OrthXY, SHOW_EditorOrtho, TRUE);
			GLevelFrame->OpenFrameViewport(TEXT("U2Viewport1"), REN_OrthXZ, SHOW_EditorOrtho, TRUE);
			GLevelFrame->OpenFrameViewport(TEXT("U2Viewport2"), REN_OrthYZ, SHOW_EditorOrtho, TRUE);
			GLevelFrame->OpenFrameViewport(TEXT("U2Viewport3"), REN_DynLight, SHOW_EditorMode, TRUE);
		}
		break;

		case 2:		// big one on left side, small along right
		{
			GLevelFrame->OpenFrameViewport(TEXT("U2Viewport0"), REN_DynLight, SHOW_EditorMode, TRUE);
			GLevelFrame->OpenFrameViewport(TEXT("U2Viewport1"), REN_OrthXY, SHOW_EditorOrtho, TRUE);
			GLevelFrame->OpenFrameViewport(TEXT("U2Viewport2"), REN_OrthXZ, SHOW_EditorOrtho, TRUE);
			GLevelFrame->OpenFrameViewport(TEXT("U2Viewport3"), REN_OrthYZ, SHOW_EditorOrtho, TRUE);
		}
		break;

		case 3:		// 2 large windows, split horizontally
		{
			GLevelFrame->OpenFrameViewport(TEXT("U2Viewport0"), REN_OrthXY, SHOW_EditorOrtho, TRUE);
			GLevelFrame->OpenFrameViewport(TEXT("U2Viewport1"), REN_DynLight, SHOW_EditorMode, TRUE);
		}
		break;

		case 4:		// 1 large window
		{
			GLevelFrame->OpenFrameViewport(TEXT("U2Viewport0"), REN_DynLight, SHOW_EditorMode, TRUE);
		}
		break;
		}

		// Load initial data from templates
		if (GViewportConfig < 4)
		{
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
		}
		else if (GViewports.Num())
		{
			GViewports(0).PctLeft = 0.f;
			GViewports(0).PctTop = 0.f;
			GViewports(0).PctRight = 1.f;
			GViewports(0).PctBottom = 1.f;
		}

		// Set the viewports to their proper sizes.
		GViewportStyle = VSTYLE_Fixed;
		FitViewportsToWindow();
		GViewportStyle = _Style;
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
	void OpenWindow(UBOOL bMdi, UBOOL bMax)
	{
		guard(WLevelFrame::OpenWindow);
		MdiChild = bMdi;
		PerformCreateWindowEx
		(
			MdiChild
			? (WS_EX_MDICHILD)
			: (0),
			TEXT("Level"),
			(bMax ? WS_MAXIMIZE : 0) |
			(MdiChild
				? (WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
				: (WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS)),
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			MulDiv(512, DPIX, 96),
			MulDiv(384, DPIY, 96),
			MdiChild ? OwnerWindow->OwnerWindow->hWnd : OwnerWindow->hWnd,
			NULL,
			hInstance
		);
		if (!MdiChild)
		{
			SetWindowLongW(hWnd, GWL_STYLE, WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
			// MSDN: If you have changed certain window data using SetWindowLong, you must call SetWindowPos for the changes to take effect. Use the following combination for uFlags: SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED.
			SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
			OwnerWindow->Show(1);
		}

		// Load the accelerator table.
		hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));
		if (hAccel == NULL)
			GWarn->Logf(TEXT("Unable to load accelerators!"));

		WViewportWindowContainer::StaticInit(Level, hWnd);

		// Open the proper configuration of viewports.
		if (!GConfig->GetInt(TEXT("Viewports"), TEXT("Style"), GViewportStyle, GUEdIni))		GViewportStyle = VSTYLE_Fixed;
		if (!GConfig->GetInt(TEXT("Viewports"), TEXT("Config"), GViewportConfig, GUEdIni))	GViewportConfig = 0;

		for (int x = 0; x < dED_MAX_VIEWPORTS; x++)
		{
			FString ViewportName = FString::Printf(TEXT("U2Viewport%d"), x);

			INT Active;
			if (!GConfig->GetInt(*ViewportName, TEXT("Active"), Active, GUEdIni))		Active = 0;

			if (Active)
			{
				OpenFrameViewport(*ViewportName);
				VIEWPORTCONFIG* pVC = &(GViewports.Last());

				// Refresh viewport config
				if (!GConfig->GetFloat(*ViewportName, TEXT("PctLeft"), pVC->PctLeft, GUEdIni))	pVC->PctLeft = 0;
				if (!GConfig->GetFloat(*ViewportName, TEXT("PctTop"), pVC->PctTop, GUEdIni))	pVC->PctTop = 0;
				if (!GConfig->GetFloat(*ViewportName, TEXT("PctRight"), pVC->PctRight, GUEdIni))	pVC->PctRight = .5f;
				if (!GConfig->GetFloat(*ViewportName, TEXT("PctBottom"), pVC->PctBottom, GUEdIni))	pVC->PctBottom = .5f;

				if (!GConfig->GetFloat(*ViewportName, TEXT("Left"), pVC->Left, GUEdIni))	pVC->Left = 0;
				if (!GConfig->GetFloat(*ViewportName, TEXT("Top"), pVC->Top, GUEdIni))	pVC->Top = 0;
				if (!GConfig->GetFloat(*ViewportName, TEXT("Right"), pVC->Right, GUEdIni))	pVC->Right = 320;
				if (!GConfig->GetFloat(*ViewportName, TEXT("Bottom"), pVC->Bottom, GUEdIni))	pVC->Bottom = 200;
			}
		}

		if (!GConfig->GetString(TEXT("ContextMenuClassAdd"), TEXT("Custom1"), Custom1, GUEdIni))	Custom1.Empty();
		if (!GConfig->GetString(TEXT("ContextMenuClassAdd"), TEXT("Custom2"), Custom2, GUEdIni))	Custom2.Empty();
		if (!GConfig->GetString(TEXT("ContextMenuClassAdd"), TEXT("Custom3"), Custom3, GUEdIni))	Custom3.Empty();
		if (!GConfig->GetString(TEXT("ContextMenuClassAdd"), TEXT("Custom4"), Custom4, GUEdIni))	Custom4.Empty();
		if (!GConfig->GetString(TEXT("ContextMenuClassAdd"), TEXT("Custom5"), Custom5, GUEdIni))	Custom5.Empty();

		FitViewportsToWindow();

		// Background image
		UBOOL bActive;
		if (!GConfig->GetInt(TEXT("Background Image"), TEXT("Active"), bActive, GUEdIni))	bActive = 0;

		if (bActive)
		{
			if (!GConfig->GetInt(TEXT("Background Image"), TEXT("Mode"), BIMode, GUEdIni))	BIMode = eBIMODE_CENTER;
			if (!GConfig->GetString(TEXT("Background Image"), TEXT("Filename"), BIFilename, GUEdIni))	BIFilename.Empty();
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
		SetText( *GetMapFilename() );

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
	virtual void OpenFrameViewport(const TCHAR* ViewportName, INT RendMap = 0, INT ShowFlags = 0, BOOL ForceReinitialize = FALSE)
	{
		guard(WLevelFrame::OpenFrameViewport);

		// Open a viewport frame.
		WViewportFrame* pViewportFrame = NewViewportFrame(ViewportName, 1);

		if (pViewportFrame)
		{
			pViewportFrame->OpenWindow();
			if (ForceReinitialize)
				pViewportFrame->ForceInitializeViewport(RendMap, ShowFlags);
			else pViewportFrame->InitializeViewport();

			// Give focus to this window
			GCurrentViewport = pViewportFrame->pViewport;
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
	,	CancelButton( this, IDCANCEL, FDelegate(this,(TDelegate)&WDialog::EndDialogFalse) )
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
					TypeList.SetItemData( TypeList.AddString( *Default->Description ), (LPARAM)*It );
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

static const TCHAR* GetSuitablePath(const char* Ext)
{
	const TCHAR* CExt = appFromAnsi(Ext);
	INT ExtLen = appStrlen(CExt);
	for (INT i = 0; i < GSys->Paths.Num(); ++i)
	{
		const TCHAR* PathStr = *GSys->Paths(i);
		INT Len = appStrlen(PathStr);
		if (Len > (ExtLen + 1) && !appStricmp(&PathStr[Len - ExtLen], CExt) && PathStr[Len - ExtLen - 1] == '.')
		{
			TCHAR* Result = appStaticString1024();
			if (appStrfind(PathStr, TEXT(":")))
			{
				TCHAR* S = Result;
				while (*PathStr && *PathStr != '*')
					*S++ = *PathStr++;
				*S = 0;
			}
			else
			{
				appStrncpy(Result, appBaseDir(), 1024);
				TCHAR* S = Result;
				while (*S)
					++S;
				while (*PathStr && *PathStr != '*')
					*S++ = *PathStr++;
				*S = 0;
			}
			return Result;
		}
	}
	return appBaseDir();
}

static UBOOL CheckForConflict(const TCHAR* OutputFile)
{
	// Grab package name only.
	FString PackageName = FString::GetFilenameOnlyStr(OutputFile);

	// Check each path.
	TCHAR* TestFile = appStaticString4096();
	TCHAR* CompFile = appStaticString4096();
	for (INT i = 0; i < GSys->Paths.Num(); ++i)
	{
		const TCHAR* PathStr = *GSys->Paths(i);
		TCHAR* Str = TestFile;
		while (*PathStr)
		{
			if (*PathStr == '*')
			{
				appStrcpy(Str, *PackageName);
				appStrcat(Str, PathStr + 1);
				PathStr = NULL;
				break;
			}
			*Str++ = *PathStr++;
		}
		if (PathStr)
			*Str = 0;

		if (GFileManager->FileSize(TestFile) != INDEX_NONE)
		{
			GetFullPathNameW(TestFile, 4096, CompFile, NULL);

			if (appStricmp(CompFile, OutputFile))
			{
				if (GWarn->YesNof(TEXT("WARNING: Found another package with matching name:\n%ls\nDo you still want to save this package with the current name?"), CompFile))
					return FALSE;
				return TRUE;
			}
		}
	}
	return FALSE;
}

void CreateFileSave(HWND hWnd, const char* Filename, const char* FileExt, const char* Filter, UBOOL bSaveMap, FString* LastURL, const TCHAR* PckName = NULL)
{
	OPENFILENAMEA ofn;
	char File[8192];
	strcpy_s(File, ARRAY_COUNT(File), Filename);
	if (strlen(Filename)>1 && !strstr(Filename, "."))
	{
		strcat_s(File, ".");
		strcat_s(File, FileExt);
	}

	appMemzero(&ofn, sizeof(OPENFILENAMEA));
	ofn.lStructSize = sizeof(OPENFILENAMEA);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = File;
	ofn.nMaxFile = sizeof(File);
	ofn.lpstrFilter = Filter;
	ofn.lpstrInitialDir = appToAnsi(LastURL ? **LastURL : GetSuitablePath(FileExt));
	ofn.lpstrDefExt = FileExt;
	ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;

	UBOOL bResult = FALSE;
	const TCHAR* ResFile = NULL;

	// Display the Open dialog box.
	while ((bResult = GetSaveFileNameA(&ofn)) != FALSE)
	{
		GFileManager->SetDefaultDirectory(appBaseDir());
		ResFile = appFromAnsi(File);
		if (!CheckForConflict(ResFile))
			break;
	}
	GFileManager->SetDefaultDirectory(appBaseDir());

	// Display the Open dialog box.
	if (bResult)
	{
		TCHAR l_chCmd[255];
		if (bSaveMap)
		{
			// Convert the ANSI filename to UNICODE, and tell the editor to open it.
			GEditor->Exec(TEXT("BRUSHCLIP DELETE"));
			appSprintf(l_chCmd, TEXT("MAP SAVE FILE=\"%ls\""), ResFile);
			GEditor->Exec(l_chCmd);

			// Save the filename.
			GLevelFrame->SetMapFilename(ResFile);
			GMRUList->AddItem(GLevelFrame->GetMapFilename());
			GMRUList->AddToMenu(hWnd, GMainMenu, 1);
		}
		else
		{
			appSprintf(l_chCmd, TEXT("OBJ SAVEPACKAGE PACKAGE=\"%ls\" FILE=\"%ls\""), PckName, ResFile);
			GEditor->Exec(l_chCmd);
		}

		if (LastURL)
		{
			FString S = ResFile;
			*LastURL = S.GetFilePath().LeftChop(1);
		}
	}
}

void FileSaveAs( HWND hWnd )
{
	// Make sure we have a level loaded...
	if( !GLevelFrame ) { return; }

	char* pFilename = TCHAR_TO_ANSI(*GLevelFrame->GetMapFilename());
	char Filter[255];
	::sprintf_s(Filter,
		ARRAY_COUNT(Filter),
		"Map Files (*.%ls)%c*.%ls%cAll Files%c*.*%c%c",
		*GMapExt,
		'\0',
		*GMapExt,
		'\0',
		'\0',
		'\0',
		'\0');

	CreateFileSave(hWnd, pFilename, appToAnsi(*GMapExt), Filter, TRUE, &GLastDir[eLASTDIR_UNR]);
}

void FileSave( HWND hWnd )
{
	if( GLevelFrame ) {

		if( GLevelFrame->GetMapFilename().Len() )
		{
			GEditor->Exec( TEXT("BRUSHCLIP DELETE") );
			if (GEditor->AskSave)
				GEditor->Exec( *(FString::Printf(TEXT("MAP SAVE FILE=\"%ls\""), *GLevelFrame->GetMapFilename())) );
			else GEditor->Exec( *(FString::Printf(TEXT("MAP SAVE AUTOSAVE=1 FILE=\"%ls\""), *GLevelFrame->GetMapFilename())) );

			GMRUList->AddItem( GLevelFrame->GetMapFilename() );
			GMRUList->AddToMenu( hWnd, GMainMenu, 1 );
		}
		else
			FileSaveAs( hWnd );
	}
}

void FileSaveMisc(HWND hWnd, UPackage* P)
{
	if (P->PackageURL.Len())
		GEditor->Exec(*(FString::Printf(TEXT("OBJ SAVEPACKAGE PACKAGE=\"%ls\" FILE=\"%ls\""), P->GetName(), *P->PackageURL)));
	else
	{
		// Determine what type of package this is.
		static char* TypeList[] = {"u","usm","utx","umx","uax"};
		INT Type = 255;
		for (FObjectIterator It; It; ++It)
		{
			if (It->IsIn(P))
			{
				UObject* O = *It;
				if (O->GetClass() == UClass::StaticClass())
				{
					Type = 0;
					break;
				}
				else if (O->IsA(UMesh::StaticClass()))
					Type = 1;
				else if (O->IsA(UTexture::StaticClass()))
					Type = Min(Type, 2);
				else if (O->IsA(UMusic::StaticClass()))
					Type = Min(Type, 3);
				else if (O->IsA(USound::StaticClass()))
					Type = Min(Type, 4);
			}
		}
		if (Type >= ARRAY_COUNT(TypeList))
			Type = 0;

		char* pFilename = TCHAR_TO_ANSI(P->GetName());
		CreateFileSave(hWnd, pFilename, TypeList[Type], "Unreal Files (u/utx/uax/umx/usm)\0*.u;*.utx;*.uax;*.umx;*.usm\0All Files\0*.*\0\0", FALSE, nullptr, P->GetName());
	}
}

void FileSaveChanges( HWND hWnd )
{
	// If a level has been loaded and there is something in the undo buffer, ask the user
	// if they want to save.
	TCHAR l_chMsg[256];
	if (GLevelFrame && GEditor->Trans->bActionsDone())
	{
		FString MN = GLevelFrame->GetMapFilename();
		appSprintf( l_chMsg, TEXT("Save changes to %ls?"), MN.Len() ? *MN : TEXT("Untitled Map"));

		if( ::MessageBox( hWnd, l_chMsg, TEXT("UnrealEd"), MB_YESNO) == IDYES )
			FileSave( hWnd );
	}

	// Ask about rest of the packages.
	for (TObjectIterator<UPackage> It; It; ++It)
	{
		UPackage* P = *It;
		if (P->bIsDirty && !P->GetOuter() && P!=UObject::GetTransientPackage() && P!=GEditor->Level->TopOuter())
		{
			if (P->PackageURL.Len())
				appSprintf(l_chMsg, TEXT("Save changes to %ls?\n(%ls)"), P->GetName(), *P->PackageURL);
			else appSprintf(l_chMsg, TEXT("Save changes to %ls?"), P->GetName());

			if (::MessageBox(hWnd, l_chMsg, TEXT("UnrealEd"), MB_YESNO) == IDYES)
				FileSaveMisc(hWnd, P);
		}
	}
}

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
		BOOL bAnyClass = FALSE;
		UClass*	AllClass = NULL;

		for( int i=0; i<Level->Actors.Num(); i++ )
		{
			if( Level->Actors(i) && Level->Actors(i)->bSelected )
			{
				if( bAnyClass && Level->Actors(i)->GetClass() != AllClass )
					AllClass = NULL;
				else
					AllClass = Level->Actors(i)->GetClass();

				bAnyClass = TRUE;
				NumActors++;
			}
		}

		if( Item & eGI_NUM_SELECTED )
		{
			Ret.iValue = NumActors;
		}
		if( Item & eGI_CLASSNAME_SELECTED )
		{
			if( bAnyClass && AllClass )
				Ret.String = AllClass->GetName();
			else
				Ret.String = TEXT("Actor");
		}
		if( Item & eGI_CLASS_SELECTED )
		{
			if( bAnyClass && AllClass )
				Ret.pClass = AllClass;
			else
				Ret.pClass = NULL;
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
	UICommandHook CmdHook;

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
		SetText( *FString::Printf( *LocalizeGeneral(TEXT("FrameWindow"),TEXT("UnrealEd")), *LocalizeGeneral(TEXT("Product"),TEXT("Core"))) );

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

		GDlgTexReplace = new WDlgTexReplace( NULL, this );
		GDlgTexReplace->DoModeless();
		GDlgTexReplace->Show(0);

		GEditorFrame = this;

		unguard;
	}
	virtual void OnTimer()
	{
		guard(WEditorFrame::OnTimer);
		GEditor->Exec( TEXT("MAYBEAUTOSAVE") );
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
		delete GDlgProgress;
		delete GDlgSearchActors;
		delete GDlgTexReplace;
		delete GDlgPackageBrowser;

		appRequestExit( 0, TEXT("WEditorFrame: OnClose") );
		WMdiFrame::OnClose();
		unguard;
	}
	void OnCommand( INT Command )
	{
		guard(WEditorFrame::OnCommand);
		TCHAR l_chCmd[255];
		UBOOL DisableRealtimePreview = 0;

		if (Command >= ID_VK_0 && Command <= ID_VK_NUM9)
		{
			Command -= ID_VK_0;
			GEditor->SendKeyStroke((Command & 0xFF), (GetKeyState(VK_CONTROL) & 252) != 0, (GetKeyState(VK_MENU) & 252) != 0, (GetKeyState(VK_SHIFT) & 252) != 0);
			return;
		}

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
					if (GCurrentViewport != reinterpret_cast<UViewport*>(LastlParam) && LastlParam)
					{
						GCurrentViewport = reinterpret_cast<UViewport*>(LastlParam);
						for (int x = 0; x < GViewports.Num(); x++)
						{
							if (GViewports(x).pViewportFrame && (GCurrentViewport == GViewports(x).pViewportFrame->pViewport))
							{
								GCurrentViewportFrame = GViewports(x).pViewportFrame->hWnd;
								break;
							}
						}
					}
					GLevelFrame->RedrawAllViewports();
				}
				break;

			case ID_FileNew:
			{
				FileSaveChanges(hWnd);

				//WNewObject Dialog( NULL, this );
				//UObject* Result = Dialog.DoModal();
				//if( Cast<ULevel>(Result) )
				//{
					GEditor->Exec(TEXT("MAP NEW"));
					GLevelFrame->SetMapFilename( TEXT("") );
					OpenLevelView();
					GButtonBar->RefreshBuilders();
					if( GBrowserGroup )
						GBrowserGroup->RefreshGroupList();
					GBuildSheet->RefreshStats();
				//}
			}
			break;

			case ID_FileNewAdd:
			{
				FileSaveChanges( hWnd );
				GEditor->Exec(TEXT("MAP NEW ADDITIVE"));
				GLevelFrame->SetMapFilename( TEXT("") );
				OpenLevelView();
				GButtonBar->RefreshBuilders();
				if( GBrowserGroup )
					GBrowserGroup->RefreshGroupList();
			}
			break;

			case ID_FILE_IMPORT:
			{
				OPENFILENAMEA ofn;
				ANSICHAR File[8192] = "\0";

				ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
				ofn.lStructSize = sizeof(OPENFILENAMEA);
				ofn.hwndOwner = hWnd;
				ofn.lpstrFile = File;
				ofn.nMaxFile = sizeof(char) * 8192;
				ofn.lpstrFilter = "Unreal Text (*.t3d)\0*.t3d\0All Files\0*.*\0\0";
				ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_UNR]) );
				ofn.lpstrDefExt = "t3d";
				ofn.lpstrTitle = "Import Map";
				ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

				// Display the Open dialog box.
				if( GetOpenFileNameA(&ofn) )
				{
					WDlgMapImport l_dlg( this );
					if( l_dlg.DoModal( appFromAnsi( File ) ) )
					{
						GWarn->BeginSlowTask( TEXT("Importing Map"), 1, 0 );
						if( l_dlg.bNewMapCheck )
							appSprintf( l_chCmd, TEXT("MAP IMPORTADD FILE=\"%ls\""), appFromAnsi( File ) );
						else
						{
							GLevelFrame->SetMapFilename( TEXT("") );
							OpenLevelView();
							appSprintf( l_chCmd, TEXT("MAP IMPORT FILE=\"%ls\""), appFromAnsi( File ) );
						}
						GEditor->Exec( l_chCmd );
						GWarn->EndSlowTask();
						GEditor->RedrawLevel( GEditor->Level );

						FString S = appFromAnsi( File );
						GLastDir[eLASTDIR_UNR] = S.GetFilePath().LeftChop(1);

						RefreshEditor();
						if( l_dlg.bNewMapCheck )
							GButtonBar->RefreshBuilders();
						if( GBrowserGroup )
							GBrowserGroup->RefreshGroupList();
					}
				}
				GFileManager->SetDefaultDirectory(appBaseDir());
			}
			break;

			case ID_FILE_EXPORT:
			{
				OPENFILENAMEA ofn;
				char File[8192] = "\0";

				ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
				ofn.lStructSize = sizeof(OPENFILENAMEA);
				ofn.hwndOwner = hWnd;
				ofn.lpstrFile = File;
				ofn.nMaxFile = sizeof(char) * 8192;
				ofn.lpstrFilter = "Unreal Text (*.t3d)\0*.t3d\0All Files\0*.*\0\0";
				ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_UNR]) );
				ofn.lpstrDefExt = "t3d";
				ofn.lpstrTitle = "Export Map";
				ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;

				if( GetSaveFileNameA(&ofn) )
				{
					GEditor->Exec( TEXT("BRUSHCLIP DELETE") );
					GEditor->Exec( *(FString::Printf(TEXT("MAP EXPORT FILE=\"%ls\""), appFromAnsi( File ))));

					FString S = appFromAnsi( File );
					GLastDir[eLASTDIR_UNR] = S.GetFilePath().LeftChop(1);
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
				FileSaveChanges(hWnd);

				GLevelFrame->SetMapFilename( (TCHAR*)(*(GMRUList->Items[Command - IDMN_MRU1] ) ) );
				GEditor->Exec( *(FString::Printf(TEXT("MAP LOAD FILE=\"%ls\""), *GMRUList->Items[Command - IDMN_MRU1] )) );
				RefreshEditor();
				GButtonBar->RefreshBuilders();
			}
			break;

			case IDMN_LOAD_BACK_IMAGE:
			{
				OPENFILENAMEA ofn;
				char File[8192] = "\0";

				ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
				ofn.lStructSize = sizeof(OPENFILENAMEA);
				ofn.hwndOwner = hWnd;
				ofn.lpstrFile = File;
				ofn.nMaxFile = sizeof(char) * 8192;
				ofn.lpstrFilter = "Bitmaps (*.bmp)\0*.bmp\0All Files\0*.*\0\0";
				ofn.lpstrInitialDir = "..\\maps";
				ofn.lpstrTitle = "Open Image";
				ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_UTX]) );
				ofn.lpstrDefExt = "bmp";
				ofn.Flags = OFN_NOCHANGEDIR;

				// Display the Open dialog box.
				//
				if( GetOpenFileNameA(&ofn) )
				{
					GLevelFrame->LoadBackgroundImage(appFromAnsi( File ));

					FString S = appFromAnsi( File );
					GLastDir[eLASTDIR_UTX] = S.GetFilePath().LeftChop(1);
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

			case ID_FileOpen:
			{
				FileSaveChanges(hWnd);
				FileOpen( hWnd );
			}
			break;

			case ID_FileClose:
			{
				if( GLevelFrame && GEditor->Trans->CanUndo() )
				{
					TCHAR l_chMsg[256];

					appSprintf( l_chMsg, TEXT("Save changes to %ls?"), *GLevelFrame->GetMapFilename() );

					INT MessageID = ::MessageBox( hWnd, l_chMsg, TEXT("UnrealEd"), MB_YESNOCANCEL);
					if( MessageID == IDYES )
						FileSave( hWnd );
					else if( MessageID == IDCANCEL )
						break;
				}

				if( GLevelFrame )
				{
					GLevelFrame->_CloseWindow();
					delete GLevelFrame;
					GLevelFrame = NULL;
				}
			}
			break;

			case ID_FileSave:
			{
				FileSave( hWnd );
			}
			break;

			case ID_FileSaveAs:
			{
				FileSaveAs( hWnd );
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

			case ID_EditDuplicate:
			{
				GEditor->Exec( TEXT("DUPLICATE") ); // same as "ACTOR DUPLICATE"
			}
			break;

			case ID_EditFind:
			case IDMN_EDIT_SEARCH:
			{
				GDlgSearchActors->Show(1);
			}
			break;

			case IDMN_EDIT_SCALE_LIGHTS:
			{
				GDlgScaleLights->Show(1);
			}
			break;

			case IDMN_EDIT_TEX_REPLACE:
			{
				GDlgTexReplace->Show(1);
			}
			break;

			case ID_EditDelete:
			{
				GEditor->Exec( TEXT("DELETE") );
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
			{
				GEditor->Exec( TEXT("EDIT PASTE") );
			}
			break;

			case ID_EditPastePos:
			{
				GEditor->Exec( TEXT("EDIT PASTEPOS") );
			}
			break;

			case ID_EditSelectAllSurfs:
			{
				GEditor->Exec( TEXT("POLY SELECT ALL") );
			}
			break;

			case ID_ViewActorProp:
			{
				if( !GEditor->ActorProperties )
				{
					GEditor->ActorProperties = new WObjectProperties( TEXT("ActorProperties"), CPF_Edit, TEXT(""), NULL, 1 );
					GEditor->ActorProperties->OpenWindow( hWnd );
					GEditor->ActorProperties->SetNotifyHook( GEditor );
				}
				GEditor->UpdatePropertiesWindows();
				GEditor->ActorProperties->Show(1);
			}
			break;

			case ID_SurfProperties:
			case ID_ViewSurfaceProp:
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
				OPENFILENAMEA ofn;
				char File[8192] = "\0";

				ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
				ofn.lStructSize = sizeof(OPENFILENAMEA);
				ofn.hwndOwner = hWnd;
				ofn.lpstrFile = File;
				ofn.nMaxFile = sizeof(char) * 8192;
				ofn.lpstrFilter = "Brushes (*.u3d)\0*.u3d\0All Files\0*.*\0\0";
				ofn.lpstrInitialDir = "..\\maps";
				ofn.lpstrDefExt = "u3d";
				ofn.lpstrTitle = "Open Brush";
				ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

				// Display the Open dialog box.
				if( GetOpenFileNameA(&ofn) )
				{
					GEditor->Exec( *(FString::Printf(TEXT("BRUSH LOAD FILE=\"%ls\""), appFromAnsi( File ))));
					GEditor->RedrawLevel( GEditor->Level );
				}

				GFileManager->SetDefaultDirectory(appBaseDir());
				GButtonBar->RefreshBuilders();
			}
			break;

			case ID_BrushSaveAs:
			{
				OPENFILENAMEA ofn;
				char File[8192] = "\0";

				ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
				ofn.lStructSize = sizeof(OPENFILENAMEA);
				ofn.hwndOwner = hWnd;
				ofn.lpstrFile = File;
				ofn.nMaxFile = sizeof(char) * 8192;
				ofn.lpstrFilter = "Brushes (*.u3d)\0*.u3d\0All Files\0*.*\0\0";
				ofn.lpstrInitialDir = "..\\maps";
				ofn.lpstrDefExt = "u3d";
				ofn.lpstrTitle = "Save Brush";
				ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;

				if( GetSaveFileNameA(&ofn) )
					GEditor->Exec( *(FString::Printf(TEXT("BRUSH SAVE FILE=\"%ls\""), appFromAnsi( File ))));

				GFileManager->SetDefaultDirectory(appBaseDir());
			}
			break;

			case ID_BRUSH_IMPORT:
			{
				OPENFILENAMEA ofn;
				char File[8192] = "\0";

				ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
				ofn.lStructSize = sizeof(OPENFILENAMEA);
				ofn.hwndOwner = hWnd;
				ofn.lpstrFile = File;
				ofn.nMaxFile = sizeof(char) * 8192;
				ofn.lpstrFilter = "Import Types (*.t3d, *.dxf, *.asc, *.obj)\0*.t3d;*.dxf;*.asc;*.obj;\0All Files\0*.*\0\0";
				ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_BRUSH]) );
				ofn.lpstrDefExt = "t3d";
				ofn.lpstrTitle = "Import Brush";
				ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

				// Display the Open dialog box.
				if( GetOpenFileNameA(&ofn) )
				{
					WDlgBrushImport l_dlg( NULL, this );
					l_dlg.DoModal( appFromAnsi( File ) );
					GEditor->RedrawLevel( GEditor->Level );

					FString S = appFromAnsi( File );
					GLastDir[eLASTDIR_BRUSH] = S.GetFilePath().LeftChop(1);
				}

				GFileManager->SetDefaultDirectory(appBaseDir());
				GButtonBar->RefreshBuilders();
			}
			break;

			case ID_BRUSH_EXPORT:
			{
				OPENFILENAMEA ofn;
				char File[8192] = "\0";

				ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
				ofn.lStructSize = sizeof(OPENFILENAMEA);
				ofn.hwndOwner = hWnd;
				ofn.lpstrFile = File;
				ofn.nMaxFile = sizeof(char) * 8192;
				ofn.lpstrFilter = "Unreal Text (*.t3d)\0*.t3d\0Wavefront OBJ (*.obj)\0*.obj\0All Files\0*.*\0\0";
				ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_BRUSH]) );
				ofn.lpstrDefExt = "t3d";
				ofn.lpstrTitle = "Export Brush";
				ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;

				if( GetSaveFileNameA(&ofn) )
				{
					GEditor->Exec( *(FString::Printf(TEXT("BRUSH EXPORT FILE=\"%ls\""), appFromAnsi( File ))));

					FString S = appFromAnsi( File );
					GLastDir[eLASTDIR_BRUSH] = S.GetFilePath().LeftChop(1);
				}

				GFileManager->SetDefaultDirectory(appBaseDir());
				GButtonBar->RefreshBuilders();
			}
			break;

			case ID_BuildPlay:
			{
				UClient* Client = GEditor->Client;
				if (Client)
				{
					UBOOL bUpdated = FALSE;
					INT i;
					for (i = 0; i < Client->Viewports.Num(); i++)
					{
						if (Client->Viewports(i)->Actor->ShowFlags & SHOW_PlayerCtrl)
						{
							Client->Viewports(i)->Actor->ShowFlags &= ~SHOW_PlayerCtrl; //disable RealTimePreview - Smirftsch
							bUpdated = TRUE;
						}
					}
					if (bUpdated)
					{
						for (i = 0; i < GViewports.Num(); i++)
						{
							if (GViewports(i).pViewportFrame)
								GViewports(i).pViewportFrame->VFToolbar->UpdateButtons();
						}
					}
				}
				GEditor->Exec( TEXT("HOOK PLAYMAP") );
			}
			break;

			case ID_BuildLighting:
				GBuildSheet->BuildStep(BUILDFLAGS_Lights);
				break;

			case ID_BuildPaths:
				GBuildSheet->BuildStep(BUILDFLAGS_Paths);
				break;

			case ID_BuildGeometry:
			case ID_BuildBSP:
				GBuildSheet->BuildStep(BUILDFLAGS_Geometry | BUILDFLAGS_BSP);
				break;

			case ID_BuildAll:
				GBuildSheet->Build();
				break;

			case ID_BuildOptions:
			{
				GBuildSheet->Show( TRUE );
			}
			break;

			case ID_ViewToolsLog:
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
					delete G2DShapeEditor;

				G2DShapeEditor = new W2DShapeEditor( TEXT("2D Shape Editor"), this );
				G2DShapeEditor->OpenWindow();
			}
			break;

#if 0
			case IDMN_SubLevelsMenu:
			{
				WSubLevelWindow::ShowMenu();
			}
			break;
#endif

			case ID_ToolsCleanupLvl:
			{
				GEditor->CleanupMapGarbage(GEditor->Level);
			}
			break;

			case ID_TOOLS_MESHEDITOR:
			{
				if (!WMeshEditor::GMeshEditor)
					WMeshEditor::GMeshEditor = new WMeshEditor(hWnd);
				else WMeshEditor::GMeshEditor->Show(1);
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
							if (GViewports(y).pViewportFrame && FObjectName(GViewports(y).pViewportFrame->pViewport) == ViewportName)
							{
								bFoundViewport = 1;
								break;
							}
						}

						if (!bFoundViewport)
						{
							// This one is available
							GLevelFrame->OpenFrameViewport(*ViewportName, REN_OrthXY, SHOW_EditorMode, TRUE);
							break;
						}
					}
				}
			}
			break;

			case IDMN_VIEWPORT_CLOSEALL:
			{
				for( int x = 0 ; x < GViewports.Num() ; x++)
					delete GViewports(x).pViewportFrame;
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
					Preferences = new WConfigProperties(TEXT("Preferences"), *LocalizeGeneral(TEXT("AdvancedOptionsTitle"), TEXT("Window"), TEXT("int")), *LocalizeGeneral(TEXT("AdvancedOptionsTitle"), TEXT("Window")));
					Preferences->OpenWindow( *this );
					Preferences->SetNotifyHook( this );
					Preferences->ForceRefresh();
				}
				Preferences->Show(1);
			}
			break;

			case WM_EDC_SAVEMAP:
			{
				FileSave( hWnd );
			}
			break;

			case WM_EDC_SAVEMAPAS:
			{
				FileSaveAs( hWnd );
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

			case WM_EDC_LOADMAP:
			{
				FileOpen( hWnd );
			}
			break;

			case ID_FILE_PACKAGEEXPLORER:
			{
				if (!GDlgPackageBrowser)
					GDlgPackageBrowser = new DlgPackageBrowser;
				else GDlgPackageBrowser->Show(1);
			}
			break;

			case WM_EDC_PLAYMAP:
			{
				GEditor->Exec( TEXT("HOOK PLAYMAP") );
			}
			break;

			case WM_EDC_BROWSE:
			{
				if( GEditor->BrowseClass->IsChildOf(UTexture::StaticClass()) ||
					GEditor->BrowseClass->IsChildOf(UPalette::StaticClass()) ||
					GEditor->BrowseClass->IsChildOf(UFont::StaticClass()))
					GBrowserMaster->ShowBrowser(eBROWSER_TEXTURE);
				else if( GEditor->BrowseClass->IsChildOf(USound::StaticClass()) )
					GBrowserMaster->ShowBrowser(eBROWSER_SOUND);
				else if( GEditor->BrowseClass->IsChildOf(UMusic::StaticClass()) )
					GBrowserMaster->ShowBrowser(eBROWSER_MUSIC);
				else if( GEditor->BrowseClass->IsChildOf(UClass::StaticClass()) )
					GBrowserMaster->ShowBrowser(eBROWSER_ACTOR);
				else if( GEditor->BrowseClass->IsChildOf(UMesh::StaticClass()) )
					GBrowserMaster->ShowBrowser(eBROWSER_MESH);
			}
			break;

			case WM_EDC_USECURRENT:
			if( GEditor->UseDest )
			{
				UObject* Cur = NULL;

				if( GEditor->BrowseClass->IsChildOf(UPalette::StaticClass()) )
					{ if( GEditor->CurrentTexture )
						Cur = GEditor->CurrentTexture->Palette; }
				else if( GEditor->BrowseClass->IsChildOf(UTexture::StaticClass()) )
					{ if( GEditor->CurrentTexture )
						Cur = GEditor->CurrentTexture; }
				else if (GEditor->BrowseClass->IsChildOf(UFont::StaticClass()))
					{ if (GEditor->CurrentFont)
						Cur = GEditor->CurrentFont; }
				else if( GEditor->BrowseClass->IsChildOf(USound::StaticClass()) )
					{ if( GBrowserSound )
						Cur = GBrowserSound->GetSound(); }
				else if( GEditor->BrowseClass->IsChildOf(UMusic::StaticClass()) )
					{ if( GBrowserMusic )
						Cur = GBrowserMusic->GetSelectedSong(); }
				else if( GEditor->BrowseClass->IsChildOf(UClass::StaticClass()) )
					{ if( GEditor->CurrentClass )
						Cur = GEditor->CurrentClass; }
				else if( GEditor->BrowseClass->IsChildOf(UMesh::StaticClass()) )
					{ if( GBrowserMesh )
						Cur = GBrowserMesh->GetMesh(); }
				if (Cur)
				{
					FString Result = FString::Printf(TEXT("%ls'#%u'"), Cur->GetClass()->GetName(), Cur->GetIndex());
					//debugf(TEXT("USECURRENT %ls : %ls"), Cur->GetPathName(), *Result);
					GEditor->UseDest->SetValue(*Result);
				}
			}
			break;

			case WM_EDC_CURTEXCHANGE:
			{
				if( GBrowserMaster->CurrentBrowser == eBROWSER_TEXTURE )
				{
					GBrowserTexture->SetCaption();
					GBrowserTexture->pViewport->Repaint(1);
				}
			}
			break;

			case WM_EDC_SELPOLYCHANGE:
			{
				GSurfPropSheet->GetDataFromSurfs1();
				GSurfPropSheet->RefreshStats();
			}
			break;

			case WM_EDC_SELCHANGE:
			{
				GSurfPropSheet->GetDataFromSurfs1();
				GSurfPropSheet->RefreshStats();
			}
			break;

			case WM_EDC_RTCLICKTEXTURE:
			{
				GBrowserTexture->pViewport->Repaint(1);
				MENU_TextureContextMenu(GBrowserTexture->hWnd);
				/*POINT pt;
				HMENU menu = GetSubMenu( LoadMenuW(hInstance, MAKEINTRESOURCEW(IDMENU_BrowserTexture_Context)), 0 );
				::GetCursorPos( &pt );
				TrackPopupMenu( menu,
					TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
					pt.x, pt.y, 0,
					GBrowserTexture->hWnd, NULL);*/
			}
			break;

			case WM_EDC_RTCLICKPOLY:
			{
				MENU_SurfaceMenu(hWnd);
			}
			break;

			case WM_EDC_RTCLICKACTOR:
			{
				MENU_ActorsMenu(hWnd);
			}
			break;

			case WM_EDC_RTCLICKVERTEX:
			{
				MENU_VertexMenu(hWnd);
			}
			break;

			case WM_EDC_RTCLICKMESHEDIT:
			{
				if (WMeshEditor::GMeshEditor)
					WMeshEditor::GMeshEditor->RC_Menu();
			}
			break;

			case WM_EDC_RTCLICKWINDOW:
			case WM_EDC_RTCLICKWINDOWCANADD:
			{
				MENU_BackgroundMenu(hWnd);
			}
			break;

			case WM_EDC_MAPCHANGE:
			{
			}
			break;

			case WM_EDC_VIEWPORTUPDATEWINDOWFRAME:
			{
				for( int x = 0 ; x < GViewports.Num() ; x++)
					if( GViewports(x).pViewportFrame && ::IsWindow( GViewports(x).pViewportFrame->hWnd ) )
						UpdateWindow(GViewports(x).pViewportFrame->hWnd);
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
				GEditor->Exec( *(FString::Printf( TEXT("ACTOR ADD CLASS=%ls"), GEditor->CurrentClass->GetPathName() ) ) );
				GEditor->Exec( TEXT("POLY SELECT NONE") );
			}
			break;

			case ID_BackdropPopupAddMeshHere:
			{
				appSprintf( l_chCmd, TEXT("ACTOR ADD MESH NAME=%ls SNAP=1"), *GBrowserMesh->GetCurrentMeshName() );
				GEditor->Exec( l_chCmd );
				GEditor->Exec( TEXT("POLY SELECT NONE") );
			}
			break;

			case ID_BackdropPopupAddLightHere:
			{
				GEditor->Exec( TEXT("ACTOR ADD CLASS=LIGHT") );
				GEditor->Exec( TEXT("POLY SELECT NONE") );
			}
			break;

			case ID_BackdropMakeStaticCollision:
			{
				GEditor->Exec( TEXT("MESH STATICCOLLISION") );
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
				GEditor->Exec( TEXT("MAP GRID X=0 Y=0 Z=0") );
			}
			break;

			case ID_BackdropPopupGridCustom:
			{
				WDlgCustomGrid dlg( NULL, this );
				if( dlg.DoModal() )
				{
					INT GridSize=dlg.Value;
					appSprintf( l_chCmd, TEXT("MAP GRID X=%i Y=%i Z=%i"), GridSize,GridSize,GridSize );
					GEditor->Exec( l_chCmd );
				}
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

			// Root
			case ID_SurfPopupAddClass:
			{
				if( GEditor->CurrentClass )
				{
					GEditor->Exec( *(FString::Printf(TEXT("ACTOR ADD CLASS=%ls"), GEditor->CurrentClass->GetPathName())));
					GEditor->Exec( TEXT("POLY SELECT NONE") );
				}
			}
			break;

			case ID_SurfPopupAddMeshHere:
			{
				appSprintf( l_chCmd, TEXT("ACTOR ADD MESH NAME=%ls SNAP=1"), *GBrowserMesh->GetCurrentMeshName() );
				GEditor->Exec( l_chCmd );
				GEditor->Exec( TEXT("POLY SELECT NONE") );
			}
			break;

			case ID_SurfPopupAddLight:
			{
				GEditor->Exec( TEXT("ACTOR ADD CLASS=LIGHT") );
				GEditor->Exec( TEXT("POLY SELECT NONE") );
			}
			break;

			case ID_SurfPopupApplyTexture:
			{
				GEditor->Exec( TEXT("POLY SETTEXTURE") );
			}
			break;

			case ID_AlignViewportCameras:
			{
				GEditor->Exec( TEXT("CAMERA ALIGN") );
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
				GEditor->Exec( TEXT("POLY TEXALIGN WALLX") );
			}
			break;

			case ID_SurfPopupAlignWallY:
			{
				GEditor->Exec( TEXT("POLY TEXALIGN WALLY") );
			}
			break;

			// Select Surfaces
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
				GEditor->Exec( TEXT("POLY SELECT MATCHING POLYFLAGS") );
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
				GEditor->Exec( TEXT("POLY SELECT MEMORY INTERSECTION") );
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
			case ID_SurfPopupReset:
			{
				GEditor->Exec( TEXT("POLY RESET") );
			}
			break;

			case ID_SurfTessellate:
			{
				GEditor->Exec( TEXT("POLY TESSELLATE") );
			}
			break;

			//
			// ACTOR POPUP MENU
			//

			// Root
			case IDMENU_ActorPopupProperties:
			{
				GEditor->Exec( TEXT("HOOK ACTORPROPERTIES") );
			}
			break;

			case IDMENU_SnapToGrid:
			{
				GEditor->Exec( TEXT("ACTOR SNAPTOGRID") );
			}
			break;

			case IDMENU_ActorPopupSelectAllClass:
			{
				t_GetInfoRet gir = GetInfo( GEditor->Level, eGI_NUM_SELECTED | eGI_CLASSNAME_SELECTED );

				if( gir.iValue )
				{
					appSprintf( l_chCmd, TEXT("ACTOR SELECT OFCLASS CLASS=%ls"), *gir.String );
					GEditor->Exec( l_chCmd );
				}
			}
			break;
			case IDMENU_ActorPopupSelectMatch:
			{
				GEditor->Exec( TEXT("ACTOR SELECT MATCHING") );
			}
			break;

			case ID_EditSelectAllActors:
			case IDMENU_ActorPopupSelectAll:
			{
				GEditor->Exec( TEXT("ACTOR SELECT ALL") );
			}
			break;

			case IDMENU_ActorSelectInside:
			{
				GEditor->Exec( TEXT("ACTOR SELECT INSIDE") );
			}
			break;

			case ID_EditSelectNone:
			case IDMENU_ActorPopupSelectNone:
			{
				GEditor->Exec( TEXT("SELECT NONE") );
			}
			break;

			case ID_EditReplace:
			case IDMENU_ActorPopupReplace:
			{
				if (GEditor->CurrentClass)
				{
					appSprintf( l_chCmd, TEXT("ACTOR REPLACE CLASS=%ls"), GEditor->CurrentClass->GetPathName() );
					GEditor->Exec( l_chCmd );
				}
			}
			break;

			case IDMENU_ActorPopupReplaceNP:
			{
				if (GEditor->CurrentClass)
				{
					appSprintf(l_chCmd, TEXT("ACTOR REPLACE CLASS=%ls KEEP=1"), GEditor->CurrentClass->GetPathName());
					GEditor->Exec(l_chCmd);
				}
			}
			break;

			case IDMENU_ActorPopupDuplicate:
			{
				GEditor->Exec( TEXT("ACTOR DUPLICATE") );
			}
			break;

			case IDMENU_ActorPopupDelete:
			{
				GEditor->Exec( TEXT("ACTOR DELETE") );
			}
			break;

			case ID_EditSelectInvert:
			case IDMENU_ActorPopupSelectInvert:
			{
				GEditor->Exec( TEXT("ACTOR SELECT INVERT") );
			}
			break;

			case IDPB_CAMERA_SPEED_PLUS:
			{
				debugf(TEXT("Plus"));
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
				debugf(TEXT("Minus"));
				if (GEditor->MovementSpeed == 16)
					GEditor->Exec(TEXT("MODE SPEED=4"));
				else if (GEditor->MovementSpeed == 4)
					GEditor->Exec(TEXT("MODE SPEED=1"));
				else GEditor->Exec( TEXT("MODE SPEED=16") );
				GButtonBar->UpdateButtons();
			}
			break;

			case IDMENU_ActorPopupEditScript:
			{
				//GBrowserMaster->ShowBrowser(eBROWSER_ACTOR);
				t_GetInfoRet gir = GetInfo( GEditor->Level, eGI_CLASS_SELECTED );
				GCodeFrame->AddClass( gir.pClass );
			}
			break;

			case IDMENU_ActorPopupMakeCurrent:
			{
				t_GetInfoRet gir = GetInfo( GEditor->Level, eGI_CLASSNAME_SELECTED );
				GEditor->Exec( *(FString::Printf(TEXT("SETCURRENTCLASS CLASS=%ls"), *gir.String)) );
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
						// movers already contain model, attempt build new one lead to link polys, which make different align in editor from game
						// see https://github.com/OldUnreal/UnrealTournamentPatches/issues/784
#if 0
						Brush->Brush->EmptyModel( 1, 0 );
						Brush->Brush->BuildBound();
						GEditor->bspBuild( Brush->Brush, BSP_Good, 15, 1, 0 );
						GEditor->bspRefresh( Brush->Brush, 1 );
						GEditor->bspValidateBrush( Brush->Brush, 1, 1 );
						GEditor->bspBuildBounds( Brush->Brush );
#endif
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
				GEditor->Exec( TEXT("ACTOR ALIGN") );
			}
			break;

			//Convert
			case IDMENU_ConvertStaticMesh:
			{
				WDlgConvertToStaticMesh l_dlg( NULL, this );
				FString Package = GBrowserMesh->pMeshPackCombo->GetString(GBrowserMesh->pMeshPackCombo->GetCurrent());
				l_dlg.DoModal(Package);
				if (bOk)
				{
					delete GBrowserMesh;
					GBrowserMesh = new WBrowserMesh( TEXT("Mesh Browser"), GBrowserMaster, GEditorFrame->hWnd );
					check(GBrowserMesh);
					GBrowserMesh->OpenWindow( 1 );
					GBrowserMesh->pMeshPackCombo->SetCurrent( GBrowserMesh->pMeshPackCombo->FindStringExact( *l_dlg.Package) );
					GBrowserMesh->pMeshCombo->SetCurrent( GBrowserMesh->pMeshCombo->FindStringExact( *l_dlg.Name) );
					GBrowserMesh->RefreshAll();
					GBrowserMaster->ShowBrowser(eBROWSER_MESH);
				}
			}
			break;
			case IDMENU_ConvertBrush:
			{
				GEditor->Exec( TEXT("ACTOR CONVERT BRUSH") );
			}
			break;

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
				appSprintf( l_chCmd, TEXT("MAP SETBRUSH CLEARFLAGS=%d SETFLAGS=%d"), PF_Semisolid + PF_NotSolid, 0);
				GEditor->Exec( l_chCmd );
			}
			break;

			case IDMENU_ActorPopupMakeSemisolid:
			{
				appSprintf( l_chCmd, TEXT("MAP SETBRUSH CLEARFLAGS=%d SETFLAGS=%d"), PF_Semisolid + PF_NotSolid, PF_Semisolid );
				GEditor->Exec( l_chCmd );
			}
			break;

			case IDMENU_ActorPopupMakeNonSolid:
			{
				appSprintf( l_chCmd, TEXT("MAP SETBRUSH CLEARFLAGS=%d SETFLAGS=%d"), PF_Semisolid + PF_NotSolid, PF_NotSolid );
				GEditor->Exec( l_chCmd );
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

			case ID_HelpContents:
			{
				appSprintf( l_chCmd, TEXT("../Help/UnrealEd.chm"));
				appLaunchURL( l_chCmd );
			}
			break;

			case ID_UEDWebHelp:
			{
				appSprintf( l_chCmd, TEXT("https://wiki.oldunreal.com"));
				appLaunchURL( l_chCmd );
			}
			break;

			case ID_EpicWeb:
			{
				appSprintf( l_chCmd, TEXT("https://www.epicgames.com/"));
				appLaunchURL( l_chCmd );
			}
			break;

			case ID_HelpWeb:
			{
				appSprintf( l_chCmd, TEXT("https://www.oldunreal.com/"));
				appLaunchURL( l_chCmd );
			}
			break;

			case ID_HelpSupport:
			{
				appSprintf( l_chCmd, TEXT("https://forums.oldunreal.com"));
				appLaunchURL( l_chCmd );
			}
			break;

			case ID_HelpAbout:
			{
				appMsgf(TEXT("UnrealEd2.2, original Version copyright 2000, Epic Games Inc. Updated to Version 2.2 by www.oldunreal.com"));
			}
			break;

			case ID_ActorsShow:
				if (GCurrentViewport)
				{
					GCurrentViewport->Actor->ShowFlags ^= SHOW_Actors;
					GCurrentViewport->RepaintPending = TRUE;
				}
				break;

			case ID_ShowBrush:
				if (GCurrentViewport)
				{
					GCurrentViewport->Actor->ShowFlags ^= SHOW_Brush;
					GCurrentViewport->RepaintPending = TRUE;
				}
				break;

			case ID_ShowBackdrop:
				if (GCurrentViewport)
				{
					GCurrentViewport->Actor->ShowFlags ^= SHOW_Backdrop;
					GCurrentViewport->RepaintPending = TRUE;
				}
				break;

			case ID_ShowRealTimeBackdrop:
				if (GCurrentViewport)
				{
					GCurrentViewport->Actor->ShowFlags ^= SHOW_RealTimeBackdrop;
					GCurrentViewport->RepaintPending = TRUE;
				}
				break;

			case IDPB_SHOW_SELECTED:
				GEditor->Exec(TEXT("ACTOR HIDE UNSELECTED"));
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

			case IDMN_VF_REALTIME_PREVIEW:
				for (int x = 0; x < GViewports.Num(); x++)
				{
					if (GViewports(x).pViewportFrame && GCurrentViewport == GViewports(x).pViewportFrame->pViewport)
					{
						GViewports(x).pViewportFrame->pViewport->Actor->ShowFlags ^= SHOW_PlayerCtrl;
						GViewports(x).pViewportFrame->VFToolbar->UpdateButtons();
						GViewports(x).pViewportFrame->pViewport->RepaintPending = TRUE;
						break;
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

void FileOpen( HWND hWnd )
{
	guard(FileOpen);
	//FileSaveChanges( hWnd );

	OPENFILENAMEA ofn;
	char File[8192] = "\0";

	ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
	ofn.lStructSize = sizeof(OPENFILENAMEA);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = File;
	ofn.nMaxFile = sizeof(File);
	char Filter[8192];
	::sprintf_s( Filter,ARRAY_COUNT(Filter),
		"Map Files (*.%ls)%c*.%ls%cAll Files%c*.*%c%c",
		*GMapExt,
		'\0',
		*GMapExt,
		'\0',
		'\0',
		'\0',
		'\0' );
	ofn.lpstrFilter = Filter;
	ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_UNR]) );
	ofn.lpstrDefExt = appToAnsi( *GMapExt );
	ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

	// Display the Open dialog box.
	if( GetOpenFileNameA(&ofn) )
	{
		// Make sure there's a level frame open.
		GEditorFrame->OpenLevelView();

		// Convert the ANSI filename to UNICODE, and tell the editor to open it.
		GLevelFrame->SetMapFilename( (TCHAR*)appFromAnsi(File) );
		GEditor->Exec( *(FString::Printf(TEXT("MAP LOAD FILE=\"%ls\""), *GLevelFrame->GetMapFilename() ) ) );

		FString S = GLevelFrame->GetMapFilename();
		GMRUList->AddItem( GLevelFrame->GetMapFilename() );
		GMRUList->AddToMenu( hWnd, GMainMenu, 1 );

		GLastDir[eLASTDIR_UNR] = S.GetFilePath().LeftChop(1);

		GMRUList->AddItem( GLevelFrame->GetMapFilename() );
		GMRUList->AddToMenu( hWnd, GMainMenu, 1 );
	}

	// Make sure that the browsers reflect any new data the map brought with it.
	RefreshEditor();
	GButtonBar->RefreshBuilders();

	GFileManager->SetDefaultDirectory(appBaseDir());
	unguard;
}

void SaveBackups()
{
	guard(SaveBackups);
	UBOOL bFirst = FALSE;

	for (TObjectIterator<UPackage> It; It; ++It)
	{
		UPackage* P = *It;
		if (P->bIsDirty && !P->GetOuter() && P != UObject::GetTransientPackage())
		{
			if (!bFirst)
			{
				GFileManager->MakeDirectory(TEXT("Backup"));
				bFirst = TRUE;
			}
			FString SaveFile;
			if (P->PackageURL.Len())
				SaveFile = P->PackageURL.GetFilenameOnly() + TEXT(".") + P->PackageURL.GetFileExtension();
			else SaveFile = FString(P->GetName()) + TEXT(".u");
			SaveFile = FString(TEXT("Backup")) * SaveFile;

			UObject::SavePackage(P, NULL, RF_Standalone, *SaveFile, GWarn, NULL, 3);
		}
	}
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
	GMalloc = &Malloc;
	GMalloc->Init();

	// Remember instance.
	GIsStarted = 1;
	hInstance = hInInstance;

	// Set package name.
	appStrncpy(const_cast<TCHAR*>(GPackage), appPackage(), 64);

#if __STATIC_LINK
	// Clean lookups.
	memset(GNativeLookupFuncs, 0, sizeof(NativeLookup) * ARRAY_COUNT(GNativeLookupFuncs));
	// Core lookups.
	INT i = 0;
	GNativeLookupFuncs[i++] = &FindCoreUObjectNative;
	GNativeLookupFuncs[i++] = &FindCoreUCommandletNative;
	GNativeLookupFuncs[i++] = &FindCoreULogHandlerNative;
	GNativeLookupFuncs[i++] = &FindCoreUScriptHookNative;
	GNativeLookupFuncs[i++] = &FindCoreULocaleNative;
	GNativeLookupFuncs[i++] = &FindNetSerializeNative;

	GNativeLookupFuncs[i++] = &FindEngineAActorNative;
	GNativeLookupFuncs[i++] = &FindEngineAPawnNative;
	GNativeLookupFuncs[i++] = &FindEngineAPlayerPawnNative;
	GNativeLookupFuncs[i++] = &FindEngineADecalNative;
	GNativeLookupFuncs[i++] = &FindEngineATimeDemoNative;
	GNativeLookupFuncs[i++] = &FindEngineAStatLogNative;
	GNativeLookupFuncs[i++] = &FindEngineAStatLogFileNative;
	GNativeLookupFuncs[i++] = &FindEngineAZoneInfoNative;
	GNativeLookupFuncs[i++] = &FindEngineAWarpZoneInfoNative;
	GNativeLookupFuncs[i++] = &FindEngineALevelInfoNative;
	GNativeLookupFuncs[i++] = &FindEngineAGameInfoNative;
	GNativeLookupFuncs[i++] = &FindEngineANavigationPointNative;
	GNativeLookupFuncs[i++] = &FindEngineUCanvasNative;
	GNativeLookupFuncs[i++] = &FindEngineUConsoleNative;
	GNativeLookupFuncs[i++] = &FindEngineUEngineNative;
	GNativeLookupFuncs[i++] = &FindEngineUScriptedTextureNative;
	GNativeLookupFuncs[i++] = &FindEngineUShadowBitMapNative;
	GNativeLookupFuncs[i++] = &FindEngineUIK_SolverBaseNative;
	GNativeLookupFuncs[i++] = &FindEngineAProjectorNative;
	GNativeLookupFuncs[i++] = &FindEngineUPX_PhysicsDataBaseNative;
	GNativeLookupFuncs[i++] = &FindEngineUPXJ_BaseJointNative;
	GNativeLookupFuncs[i++] = &FindEngineUPhysicsAnimationNative;
	GNativeLookupFuncs[i++] = &FindEngineURenderIteratorNative;
	GNativeLookupFuncs[i++] = &FindEngineAVolumeNative;
	GNativeLookupFuncs[i++] = &FindEngineAInventoryNative;
	GNativeLookupFuncs[i++] = &FindEngineAProjectileNative;
	GNativeLookupFuncs[i++] = &FindEngineUClientPreloginSceneNative;
	GNativeLookupFuncs[i++] = &FindEngineUServerPreloginSceneNative;

	GNativeLookupFuncs[i++] = &FindFireAProceduralMeshNative;
	GNativeLookupFuncs[i++] = &FindFireUPaletteModifierNative;
	GNativeLookupFuncs[i++] = &FindFireUConstantColorNative;
	GNativeLookupFuncs[i++] = &FindFireUTextureModifierBaseNative;
	GNativeLookupFuncs[i++] = &FindFireAFluidSurfaceInfoNative;

	GNativeLookupFuncs[i++] = &FindEmitterAXRopeDecoNative;
	GNativeLookupFuncs[i++] = &FindEmitterAXEmitterNative;
	GNativeLookupFuncs[i++] = &FindEmitterAXParticleEmitterNative;
	GNativeLookupFuncs[i++] = &FindEmitterAXWeatherEmitterNative;

	GNativeLookupFuncs[i++] = &FindUPakAPawnPathNodeIteratorNative;
	GNativeLookupFuncs[i++] = &FindUPakAPathNodeIteratorNative;

	GNativeLookupFuncs[i++] = &FindPathLogicUPathLogicManagerNative;

	GNativeLookupFuncs[i++] = &FindEditorUBrushBuilderNative;
	GNativeLookupFuncs[i++] = &FindEditorUEdGUI_CheckBoxNative;
	GNativeLookupFuncs[i++] = &FindEditorUEdGUI_ComponentNative;
	GNativeLookupFuncs[i++] = &FindEditorUEdGUI_BaseNative;
	GNativeLookupFuncs[i++] = &FindEditorUEdGUI_ComboBoxNative;
	GNativeLookupFuncs[i++] = &FindEditorUEdGUI_WindowFrameNative;

	GNativeLookupFuncs[i++] = &FindIpDrvAInternetLinkNative;
	GNativeLookupFuncs[i++] = &FindIpDrvAUdpLinkNative;
	GNativeLookupFuncs[i++] = &FindIpDrvATcpLinkNative;

	GNativeLookupFuncs[i++] = &FindUWebAdminUWebQueryNative;

#endif

	// Begin.
#if !defined(_DEBUG) && DO_GUARD
	try
	{
#endif
		// Set mode.
		GIsClient = GIsServer = GIsEditor = GLazyLoad = 1;
		GIsScriptable = 0;

		// Start main loop.
		GIsGuarded=1;

		const TCHAR* CmdLine = GetCommandLine();
		appInit(TEXT("Unreal"), CmdLine, &Malloc, &Log, &Error, &Warn, &FileManager, FConfigCacheIni::Factory, 1);
		GetPropResult = new FStringOutputDevice;

		// Init windowing.
		InitWindowing();
		IMPLEMENT_WINDOWCLASS(WMdiFrame,CS_DBLCLKS);
		IMPLEMENT_WINDOWCLASS(WEditorFrame,CS_DBLCLKS);
		IMPLEMENT_WINDOWCLASS(WBackgroundHolder,CS_DBLCLKS);
		IMPLEMENT_WINDOWCLASS(WLevelFrame,CS_DBLCLKS);
		IMPLEMENT_WINDOWCLASS(WDockingFrame,CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW);
		IMPLEMENT_WINDOWCLASS(WCodeFrame,CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW);
		IMPLEMENT_WINDOWCLASS(W2DShapeEditor,CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW);
		//IMPLEMENT_WINDOWCLASS(WSubLevelWindowBase, CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW);
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

		// Windows.
		WEditorFrame Frame;

#if !defined(_DEBUG) && DO_GUARD
		try // HACK: To prevent editor from asking if you want to save level on crash.
		{
#endif
			GDocumentManager = &Frame;
			Frame.OpenWindow();
			InvalidateRect(Frame, NULL, 1);
			UpdateWindow(Frame);
			UBOOL ShowLog = ParseParam(appCmdLine(), TEXT("log"));
			if (!ShowLog && !ParseParam(appCmdLine(), TEXT("server")))
				InitSplash(TEXT("EdSplash.bmp"));

			// Init.
			GLogWindow = new WLog(*Log.Filename, Log.LogAr->GetAr(), TEXT("EditorLog"), &Frame);
			GLogWindow->OpenWindow(ShowLog, 0);
			GLogWindow->MoveWindow(100, 100, 800, 450, 0);

			// Init engine.
			GEditor = CastChecked<UEditorEngine>(InitEngine());
			GhwndEditorFrame = GEditorFrame->hWnd;

			// Set up autosave timer.  We ping the engine once a minute and it determines when and
			// how to do the autosave.
			SetTimer(GEditorFrame->hWnd, 900, 60000, NULL);

			//Check for ini and if not existing, create a new one.
			if (!Parse(appCmdLine(), TEXT("UEDINI="), GUEdIni, 256))
			{
				FString EditorIni = TEXT("");
				if (GConfig->GetString(TEXT("Editor.EditorEngine"), TEXT("EditorIni"), EditorIni, GIni))
				{
					if (EditorIni.Len())
					{
						appSprintf(GUEdIni, TEXT("%ls.ini"), *EditorIni);
					}
					else appSprintf(GUEdIni, TEXT("%ls.ini"), GPackage);
				}
				else appSprintf(GUEdIni, TEXT("%ls.ini"), GPackage);
			}


			debugf(TEXT("Using ini: %ls"), GUEdIni);
			if (GFileManager->FileSize(GUEdIni) < 0)
			{
				// Create UnrealEd.ini from DefaultUnrealEd.ini.
				FString S;
				if (!appLoadFileToString(S, TEXT("DefaultUnrealEd.ini"), GFileManager))
					appErrorf(*LocalizeError(TEXT("MissinGUEdIni")), "DefaultUnrealEd.ini");
				appSaveStringToFile(S, GUEdIni);
			}

			{
				// Create a fully qualified pathname for the log file.  If we don't do this, pieces of the log file
				// tends to get written into various directories as the editor starts up.
				FString chLogFilename = appBaseDir();
				chLogFilename *= GPackage;

				if (ParseParam(appCmdLine(), TEXT("TIMESTAMPLOG")))
				{
					INT Year, Month, DayOfWeek, Day, Hour, Min, Sec, MSec;
					appSystemTime(Year, Month, DayOfWeek, Day, Hour, Min, Sec, MSec);
					chLogFilename += FString::Printf(TEXT("_%02d-%02d-%02d_%02d-%02d-%02d"), Year, Month, Day, Hour, Min, Sec);

				}
				Log.Filename = chLogFilename + TEXT(".log");
			}

			// Initialize "last dir" array
			if (!GConfig->GetString(TEXT("Directories"), TEXT("PCX"), GLastDir[eLASTDIR_PCX], GUEdIni))		GLastDir[eLASTDIR_PCX] = TEXT("..\\Textures");
			if (!GConfig->GetString(TEXT("Directories"), TEXT("WAV"), GLastDir[eLASTDIR_WAV], GUEdIni))		GLastDir[eLASTDIR_WAV] = TEXT("..\\Sounds");
			if (!GConfig->GetString(TEXT("Directories"), TEXT("BRUSH"), GLastDir[eLASTDIR_BRUSH], GUEdIni))		GLastDir[eLASTDIR_BRUSH] = TEXT("..\\Maps");
			if (!GConfig->GetString(TEXT("Directories"), TEXT("2DS"), GLastDir[eLASTDIR_2DS], GUEdIni))		GLastDir[eLASTDIR_2DS] = TEXT("..\\Maps");
			if (!GConfig->GetString(TEXT("Directories"), TEXT("USM"), GLastDir[eLASTDIR_USM], GUEdIni))		GLastDir[eLASTDIR_USM] = TEXT("..\\Meshes");
			if (!GConfig->GetString(TEXT("Directories"), TEXT("UMX"), GLastDir[eLASTDIR_UMX], GUEdIni))		GLastDir[eLASTDIR_UMX] = TEXT("..\\Music");
			if (!GConfig->GetString(TEXT("Directories"), TEXT("UAX"), GLastDir[eLASTDIR_UAX], GUEdIni))		GLastDir[eLASTDIR_UAX] = TEXT("..\\Sounds");
			if (!GConfig->GetString(TEXT("Directories"), TEXT("UTX"), GLastDir[eLASTDIR_UTX], GUEdIni))		GLastDir[eLASTDIR_UTX] = TEXT("..\\Textures");
			if (!GConfig->GetString(TEXT("Directories"), TEXT("UNR"), GLastDir[eLASTDIR_UNR], GUEdIni))		GLastDir[eLASTDIR_UNR] = TEXT("..\\Maps");

			if (!GConfig->GetString(TEXT("URL"), TEXT("MapExt"), GMapExt, TEXT("Unreal.ini")))		GMapExt = TEXT("unr");
			GEditor->Exec(*(FString::Printf(TEXT("MODE MAPEXT=%ls"), *GMapExt)));

			// Init input.
			UInput::StaticInitInput();

			// Toolbar.
			GButtonBar = new WButtonBar(TEXT("EditorToolbar"), &Frame.LeftFrame);
			GButtonBar->OpenWindow();
			Frame.LeftFrame.Dock(GButtonBar);
			Frame.LeftFrame.OnSize(SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE, 0, 0);

			GBottomBar = new WBottomBar(TEXT("BottomBar"), &Frame.BottomFrame);
			GBottomBar->OpenWindow();
			Frame.BottomFrame.Dock(GBottomBar);
			Frame.BottomFrame.OnSize(SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE, 0, 0);

			GTopBar = new WTopBar(TEXT("TopBar"), &Frame.TopFrame);
			GTopBar->OpenWindow();
			Frame.TopFrame.Dock(GTopBar);
			Frame.TopFrame.OnSize(SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE, 0, 0);

			GBrowserMaster = new WBrowserMaster(TEXT("Master Browser"), GEditorFrame);
			GBrowserMaster->OpenWindow(0);
			GBrowserMaster->Browsers[eBROWSER_MESH] = (WBrowser**)(&GBrowserMesh);
			GBrowserMaster->Browsers[eBROWSER_MUSIC] = (WBrowser**)(&GBrowserMusic);
			GBrowserMaster->Browsers[eBROWSER_SOUND] = (WBrowser**)(&GBrowserSound);
			GBrowserMaster->Browsers[eBROWSER_ACTOR] = (WBrowser**)(&GBrowserActor);
			GBrowserMaster->Browsers[eBROWSER_GROUP] = (WBrowser**)(&GBrowserGroup);
			GBrowserMaster->Browsers[eBROWSER_TEXTURE] = (WBrowser**)(&GBrowserTexture);
			::InvalidateRect(GBrowserMaster->hWnd, NULL, 1);

			// Open a blank level on startup.
			Frame.OpenLevelView();

			// Reopen whichever windows we need to.
			UBOOL bDocked, bActive;

			if (!GConfig->GetInt(TEXT("Mesh Browser"), TEXT("Docked"), bDocked, GUEdIni))	bDocked = FALSE;
			SendMessageW(GEditorFrame->hWnd, WM_COMMAND, bDocked ? WM_BROWSER_DOCK : WM_BROWSER_UNDOCK, eBROWSER_MESH);
			if (!bDocked)
			{
				if (!GConfig->GetInt(*GBrowserMesh->PersistentName, TEXT("Active"), bActive, GUEdIni))	bActive = FALSE;
				GBrowserMesh->Show(bActive);
			}

			if (!GConfig->GetInt(TEXT("Music Browser"), TEXT("Docked"), bDocked, GUEdIni))	bDocked = FALSE;
			SendMessageW(GEditorFrame->hWnd, WM_COMMAND, bDocked ? WM_BROWSER_DOCK : WM_BROWSER_UNDOCK, eBROWSER_MUSIC);
			if (!bDocked)
			{
				if (!GConfig->GetInt(*GBrowserMusic->PersistentName, TEXT("Active"), bActive, GUEdIni))	bActive = FALSE;
				GBrowserMusic->Show(bActive);
			}

			if (!GConfig->GetInt(TEXT("Sound Browser"), TEXT("Docked"), bDocked, GUEdIni))	bDocked = FALSE;
			SendMessageW(GEditorFrame->hWnd, WM_COMMAND, bDocked ? WM_BROWSER_DOCK : WM_BROWSER_UNDOCK, eBROWSER_SOUND);
			if (!bDocked)
			{
				if (!GConfig->GetInt(*GBrowserSound->PersistentName, TEXT("Active"), bActive, GUEdIni))	bActive = FALSE;
				GBrowserSound->Show(bActive);
			}

			if (!GConfig->GetInt(TEXT("Actor Browser"), TEXT("Docked"), bDocked, GUEdIni))	bDocked = FALSE;
			SendMessageW(GEditorFrame->hWnd, WM_COMMAND, bDocked ? WM_BROWSER_DOCK : WM_BROWSER_UNDOCK, eBROWSER_ACTOR);
			if (!bDocked)
			{
				if (!GConfig->GetInt(*GBrowserActor->PersistentName, TEXT("Active"), bActive, GUEdIni))	bActive = FALSE;
				GBrowserActor->Show(bActive);
			}

			if (!GConfig->GetInt(TEXT("Group Browser"), TEXT("Docked"), bDocked, GUEdIni))	bDocked = FALSE;
			SendMessageW(GEditorFrame->hWnd, WM_COMMAND, bDocked ? WM_BROWSER_DOCK : WM_BROWSER_UNDOCK, eBROWSER_GROUP);
			if (!bDocked)
			{
				if (!GConfig->GetInt(*GBrowserGroup->PersistentName, TEXT("Active"), bActive, GUEdIni))	bActive = FALSE;
				GBrowserGroup->Show(bActive);
			}

			if (!GConfig->GetInt(TEXT("Texture Browser"), TEXT("Docked"), bDocked, GUEdIni))	bDocked = FALSE;
			SendMessageW(GEditorFrame->hWnd, WM_COMMAND, bDocked ? WM_BROWSER_DOCK : WM_BROWSER_UNDOCK, eBROWSER_TEXTURE);
			if (!bDocked)
			{
				if (!GConfig->GetInt(*GBrowserTexture->PersistentName, TEXT("Active"), bActive, GUEdIni))	bActive = FALSE;
				GBrowserTexture->Show(bActive);
			}

			if (!GConfig->GetInt(TEXT("CodeFrame"), TEXT("Active"), bActive, GUEdIni))	bActive = FALSE;
			if (bActive)	ShowCodeFrame(GEditorFrame);

			GCodeFrame = new WCodeFrame(TEXT("CodeFrame"), GEditorFrame);
			GCodeFrame->OpenWindow(0, 0);

			GMainMenu = LoadMenuW(hInstance, MAKEINTRESOURCEW(IDMENU_MainMenu));
			SetMenu(GEditorFrame->hWnd, GMainMenu);

			GMRUList = new MRUList(TEXT("MRU"));
			GMRUList->ReadINI();
			GMRUList->AddToMenu(GEditorFrame->hWnd, GMainMenu, 1);

			ExitSplash();

			if (appStrlen(appCmdLine()) && GFileManager->FileSize(appCmdLine()) > 0)
			{
				debugf(TEXT("Loading map %ls"), appCmdLine());
				// Convert the ANSI filename to UNICODE, and tell the editor to open it.
				GLevelFrame->SetMapFilename((TCHAR*)appCmdLine());
				GEditor->Exec(*(FString::Printf(TEXT("MAP LOAD FILE=\"%ls\""), *GLevelFrame->GetMapFilename())));

				FString S = GLevelFrame->GetMapFilename();
				GMRUList->AddItem(GLevelFrame->GetMapFilename());
				GMRUList->AddToMenu(GEditorFrame->hWnd, GMainMenu, 1);

				GLastDir[eLASTDIR_UNR] = S.GetFilePath().LeftChop(1);

				GMRUList->AddItem(GLevelFrame->GetMapFilename());
				GMRUList->AddToMenu(GEditorFrame->hWnd, GMainMenu, 1);
			}
			else RefreshEditor();

			if (!GIsRequestingExit)
				MainLoop(GEditor);

			GDocumentManager = NULL;
			GFileManager->Delete(GRunningFile);
			if (GLogWindow)
				delete GLogWindow;
			appPreExit();
			GIsGuarded = 0;
			delete GMRUList;
			::DestroyWindow(GCodeFrame->hWnd);
			delete GCodeFrame;
#if !defined(_DEBUG) && DO_GUARD
		}
		catch (...)
		{
			GIsCriticalError = TRUE;
			GIsRequestingExit = TRUE;
			throw;
		}
	}
	catch (TCHAR* Err)
	{
		// Crashed.
		Error.Serialize(Err, NAME_Error);
		try
		{
			SaveBackups();
		}
		catch (...)
		{
			GWarn->Logf(NAME_Critical, TEXT("Double error when trying to create backup saves!"));
		}
		GIsGuarded = 0;
		Error.HandleError();
	}
	catch( ... )
	{
		try
		{
			SaveBackups();
		}
		catch (...)
		{
			GWarn->Logf(NAME_Critical, TEXT("Double error when trying to create backup saves!"));
		}
		// Crashed.
		Error.HandleError();
	}
#endif

	// Shut down.
	appExit();
	GIsStarted = 0;
	return 0;
}

void RC_ActorConvertTo(INT ID)
{
	WDlgConvertToStaticMesh l_dlg(NULL, GEditorFrame);
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
void RC_ChangeGrid(INT ID)
{
	if (ID < ARRAY_COUNT(PreGridSizes))
	{
		INT GridSize = PreGridSizes[ID];
		GEditor->Exec(*FString::Printf(TEXT("MAP GRID X=%i Y=%i Z=%i"), GridSize, GridSize, GridSize));
	}
	else
	{
		WDlgCustomGrid dlg(NULL, GEditorFrame);
		if (dlg.DoModal())
		{
			INT GridSize = dlg.Value;
			GEditor->Exec(*FString::Printf(TEXT("MAP GRID X=%i Y=%i Z=%i"), GridSize, GridSize, GridSize));
		}
	}
}

UBOOL UICommandHook::HookExec(const TCHAR* Cmd, FOutputDevice& Ar)
{
	guard(UICommandHook::HookExec);
	if (ParseCommand(&Cmd, TEXT("SHOWACTORS")))
	{
		if (GCurrentViewport)
		{
			if (ParseCommand(&Cmd, TEXT("XOR")))
				GCurrentViewport->Actor->ShowFlags ^= SHOW_Actors;
			else if (ParseCommand(&Cmd, TEXT("ON")) || ParseCommand(&Cmd, TEXT("1")))
				GCurrentViewport->Actor->ShowFlags |= SHOW_Actors;
			else if (ParseCommand(&Cmd, TEXT("OFF")) || ParseCommand(&Cmd, TEXT("0")))
				GCurrentViewport->Actor->ShowFlags &= ~SHOW_Actors;
			else Ar.Log(TEXT("Valid show options are: SHOWACTORS XOR/ON/OFF"));
			GCurrentViewport->RepaintPending = TRUE;
		}
		return TRUE;
	}
	else if (ParseCommand(&Cmd, TEXT("BROWSE")))
	{
		if (ParseCommand(&Cmd, TEXT("MASTER")))
			GBrowserMaster->Show(1);
		else if (ParseCommand(&Cmd, TEXT("ACTOR")))
			GBrowserMaster->ShowBrowser(eBROWSER_ACTOR);
		else if (ParseCommand(&Cmd, TEXT("TEXTURE")))
			GBrowserMaster->ShowBrowser(eBROWSER_TEXTURE);
		else if (ParseCommand(&Cmd, TEXT("MESH")))
			GBrowserMaster->ShowBrowser(eBROWSER_MESH);
		else if (ParseCommand(&Cmd, TEXT("SOUND")))
			GBrowserMaster->ShowBrowser(eBROWSER_SOUND);
		else if (ParseCommand(&Cmd, TEXT("MUSIC")))
			GBrowserMaster->ShowBrowser(eBROWSER_MUSIC);
		else if (ParseCommand(&Cmd, TEXT("GROUP")))
			GBrowserMaster->ShowBrowser(eBROWSER_GROUP);
		else if (ParseCommand(&Cmd, TEXT("SCRIPT")))
			GEditorFrame->OnCommand(IDMN_CODE_FRAME);
		else if (ParseCommand(&Cmd, TEXT("PREFABS")))
			appMsgf(TEXT("Not implemented yet."));
		else Ar.Log(TEXT("Valid browse options are: BROWSE MASTER/ACTOR/TEXTURE/MESH/SOUND/MUSIC/GROUP/SCRIPT"));
		return TRUE;
	}
	else if (ParseCommand(&Cmd, TEXT("OPEN")))
	{
		if (ParseCommand(&Cmd, TEXT("BRUSH")))
			GEditorFrame->OnCommand(ID_BrushOpen);
		else if (ParseCommand(&Cmd, TEXT("FINDACTOR")))
			GDlgSearchActors->Show(1);
		else if (ParseCommand(&Cmd, TEXT("HELP")))
			GEditorFrame->OnCommand(ID_HelpContents);
		else if (ParseCommand(&Cmd, TEXT("2DEDITOR")))
			GEditorFrame->OnCommand(ID_Tools2DEditor);
		else if (ParseCommand(&Cmd, TEXT("SURFPROPERTIES")))
			GEditorFrame->OnCommand(ID_SurfProperties);
		else if (ParseCommand(&Cmd, TEXT("MAPCLEANUP")))
			GEditorFrame->OnCommand(ID_ToolsCleanupLvl);
		else if (ParseCommand(&Cmd, TEXT("PREFERENCES")))
			GEditorFrame->OnCommand(ID_ToolsPrefs);
		else if (ParseCommand(&Cmd, TEXT("WEBHELP")))
			GEditorFrame->OnCommand(ID_UEDWebHelp);
		else if (ParseCommand(&Cmd, TEXT("ACTORPROPERTIES")))
			GEditorFrame->OnCommand(ID_ViewActorProp);
		else if (ParseCommand(&Cmd, TEXT("LEVELPROPERTIES")))
			GEditorFrame->OnCommand(ID_ViewLevelProp);
		else if (ParseCommand(&Cmd, TEXT("LOG")))
			GEditorFrame->OnCommand(ID_ViewToolsLog);
		else if (ParseCommand(&Cmd, TEXT("SCRIPT")))
			GEditorFrame->OnCommand(IDMENU_ActorPopupEditScript);
		else if (ParseCommand(&Cmd, TEXT("LIGHTSCALE")))
			GDlgScaleLights->Show(1);
		else if (ParseCommand(&Cmd, TEXT("TEXREPLACE")))
			GDlgTexReplace->Show(1);
		else Ar.Log(TEXT("Valid open options are: OPEN BRUSH/FINDACTOR/HELP/2DEDITOR/SURFPROPERTIES/MAPCLEANUP/PREFERENCES/WEBHELP/ACTORPROPERTIES/LEVELPROPERTIES/LOG/SCRIPT/LIGHTSCALE/TEXREPLACE"));
		return TRUE;
	}
	else if (ParseCommand(&Cmd, TEXT("BUILD")))
	{
		if (ParseCommand(&Cmd, TEXT("ALL")))
			GBuildSheet->Build();
		else if (ParseCommand(&Cmd, TEXT("OPTIONS")))
			GBuildSheet->Show(TRUE);
		else if (ParseCommand(&Cmd, TEXT("PLAY")))
			GEditorFrame->OnCommand(ID_BuildPlay);
		else Ar.Log(TEXT("Valid build options are: BUILD ALL/OPTIONS/PLAY"));
		return TRUE;
	}
	else if (ParseCommand(&Cmd, TEXT("MAPFILE")))
	{
		if (ParseCommand(&Cmd, TEXT("NEW")))
			GEditorFrame->OnCommand(ID_FileNew);
		else if (ParseCommand(&Cmd, TEXT("NEWADDITIVE")))
			GEditorFrame->OnCommand(ID_FileNewAdd);
		else if (ParseCommand(&Cmd, TEXT("OPEN")))
			GEditorFrame->OnCommand(ID_FileOpen);
		else if (ParseCommand(&Cmd, TEXT("SAVE")))
			GEditorFrame->OnCommand(ID_FileSave);
		else if (ParseCommand(&Cmd, TEXT("SAVEAS")))
			GEditorFrame->OnCommand(ID_FileSaveAs);
		else Ar.Log(TEXT("Valid file options are: MAPFILE NEW/NEWADDITIVE/OPEN/SAVE/SAVEAS"));
		return TRUE;
	}
	else if (ParseCommand(&Cmd, TEXT("DRAWMODE")))
	{
		INT iCmd = 0;
		if (ParseCommand(&Cmd, TEXT("LIGHTONLY")))
			iCmd = ID_LightingOnly;
		else if (ParseCommand(&Cmd, TEXT("DYNAMICLIGHT")))
			iCmd = ID_MapDynLight;
		else if (ParseCommand(&Cmd, TEXT("UNLIT")))
			iCmd = ID_MapPlainTex;
		else if (ParseCommand(&Cmd, TEXT("BSPCUTS")))
			iCmd = ID_MapPolyCuts;
		else if (ParseCommand(&Cmd, TEXT("BSPPOLYS")))
			iCmd = ID_MapPolys;
		else if (ParseCommand(&Cmd, TEXT("WIREFRAME")))
			iCmd = ID_MapWire;
		else if (ParseCommand(&Cmd, TEXT("ZONES")))
			iCmd = ID_MapZones;
		else if (ParseCommand(&Cmd, TEXT("NORMALS")))
			iCmd = ID_Normals;
		else if (ParseCommand(&Cmd, TEXT("OVERHEAD")))
			iCmd = ID_MapOverhead;
		else if (ParseCommand(&Cmd, TEXT("XZ")))
			iCmd = ID_MapXZ;
		else if (ParseCommand(&Cmd, TEXT("YZ")))
			iCmd = ID_MapYZ;
		else Ar.Log(TEXT("Valid draw options are: DRAWMODE LIGHTONLY/DYNAMICLIGHT/NOLIGHT/BSPCUTS/BSPPOLYS/WIREFRAME/ZONES/NORMALS/OVERHEAD/XZ/YZ"));

		if (iCmd)
		{
			for (int x = 0; x < GViewports.Num(); x++)
			{
				if (GViewports(x).pViewportFrame && GCurrentViewport == GViewports(x).pViewportFrame->pViewport)
				{
					GViewports(x).pViewportFrame->OnCommand(iCmd);
					break;
				}
			}
		}
		return TRUE;
	}
	else if (ParseCommand(&Cmd, TEXT("SHOWFLAGS")))
	{
		if (GCurrentViewport)
		{
			if (ParseCommand(&Cmd, TEXT("BACKDROP")))
				GCurrentViewport->Actor->ShowFlags ^= SHOW_Backdrop;
			else if (ParseCommand(&Cmd, TEXT("REALTIMEBACKDROP")))
				GCurrentViewport->Actor->ShowFlags ^= SHOW_RealTimeBackdrop;
			else if (ParseCommand(&Cmd, TEXT("BRUSH")))
				GCurrentViewport->Actor->ShowFlags ^= SHOW_Brush;
			else if (ParseCommand(&Cmd, TEXT("REALTIME")))
				GCurrentViewport->Actor->ShowFlags ^= SHOW_PlayerCtrl;
			else
			{
				Ar.Logf(TEXT("Unknown ShowFlags '%ls'"), Cmd);
				return TRUE;
			}
			GCurrentViewport->RepaintPending = TRUE;
		}
		return TRUE;
	}
	else if (ParseCommand(&Cmd, TEXT("CAMSPEED")))
	{
		if (ParseCommand(&Cmd, TEXT("ADD")))
			GEditorFrame->OnCommand(IDPB_CAMERA_SPEED_PLUS);
		else if (ParseCommand(&Cmd, TEXT("SUB")))
			GEditorFrame->OnCommand(IDPB_CAMERA_SPEED_MINUS);
		return TRUE;
	}
	else if (ParseCommand(&Cmd, TEXT("MAKECURRENT")))
	{
		t_GetInfoRet gir = GetInfo(GEditor->Level, eGI_CLASSNAME_SELECTED);
		GEditor->Exec(*(FString::Printf(TEXT("SETCURRENTCLASS CLASS=%ls"), *gir.String)));
		return TRUE;
	}
	else if (ParseCommand(&Cmd, TEXT("TERRAINMODE")))
	{
		DlgTerrainEditor::stSetVisible(ParseCommand(&Cmd, TEXT("ENABLE")));
		return TRUE;
	}
	return FALSE;
	unguard;
}

void DlgPackageBrowser::OnLoadPackages()
{
	guard(DlgPackageBrowser::OnLoadPackages);
	const INT num = static_cast<INT>(PackageList.GetSelectedCount());
	if (!num)
		return;
	INT* SelList = new INT[num];
	INT i;
	PackageList.GetSelectedItems(num, SelList);

	// Verify if about to load maps.
	for (i = 0; i < num; ++i)
	{
		if (FileList(RefList(SelList[i])).bIsMapFile && GWarn->YesNof(TEXT("You are about to load package '%ls' which is a map file.\nDo you want to load it as active level instead?"), *FileList(RefList(SelList[i])).FullFilePath))
		{
			GLevelFrame->SetMapFilename(FileList(RefList(SelList[i])).FullFilePath);
			SelList[i] = INDEX_NONE;

			// Make sure there's a level frame open.
			GEditorFrame->OpenLevelView();

			// Convert the ANSI filename to UNICODE, and tell the editor to open it.
			GEditor->Exec(*(FString::Printf(TEXT("MAP LOAD FILE=\"%ls\""), *GLevelFrame->GetMapFilename())));
			GMRUList->AddItem(GLevelFrame->GetMapFilename());
			GMRUList->AddToMenu(hWnd, GMainMenu, 1);
			break;
		}
	}

	GWarn->BeginSlowTask(TEXT("Loading packages..."), TRUE, FALSE);
	TCHAR l_chCmd[1024];
	for (i = 0; i < num; ++i)
	{
		GWarn->StatusUpdatef(i, num, TEXT("Loading packages..."));
		if (SelList[i] == INDEX_NONE)
			continue;
		appSnprintf(l_chCmd, 1024, TEXT("OBJ LOAD FILE=\"%ls\""), *FileList(RefList(SelList[i])).FullFilePath);
		GEditor->Exec(l_chCmd);
	}
	GWarn->EndSlowTask();
	delete[] SelList;
	GBrowserMaster->RefreshAll();
	unguard;
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
