#pragma warning(push)
#include <windows.h>
#pragma warning(pop)
#include <commctrl.h>

enum eBS {
	eBS_OPTIONS		= 0,
	eBS_STATS		= 1,
	eBS_MAX			= 2
};

enum EZoningMode : INT
{
	ZONEMODE_Merge,
	ZONEMODE_Keep,
	ZONEMODE_Discard,
	ZONEMODE_BitMask = 3,
};

enum
{
	BUILDFLAGS_Geometry = (1<<0),
	BUILDFLAGS_BSP = (1 << 1),
	BUILDFLAGS_Lights = (1 << 2),
	BUILDFLAGS_Paths = (1 << 3),
};
enum
{
	OPTFLAGS_OptimizeGeometry = (1 << 0),
	OPTFLAGS_VisZones = (1 << 1),
	OPTFLAGS_MultiThreading = (1 << 2),
	OPTFLAGS_SelectedLights = (1 << 3),
	OPTFLAGS_GeometryVisOnly = (1 << 4),
	OPTFLAGS_OptimizeLightGeo = (1 << 5),
	OPTFLAGS_PostBuildSemis = (1 << 6),
	OPTFLAGS_NoFixCollision = (1 << 7),
};

class TBuildSheet
{
public:
	struct FBuildOptions
	{
		INT Cuts, Opt, Portals, ZoningMode;
		DWORD BuildFlags, OptFlags;

		void GetCmd_Geometry(TCHAR* Cmd);
		void GetCmd_BSP(TCHAR* Cmd);
		void GetCmd_Lighting(TCHAR* Cmd);
		void GetCmd_Paths(TCHAR* Cmd);
		void GetCmd_Full(TCHAR* Cmd, DWORD Flags);
	} BuildOptions;

	TBuildSheet();
	~TBuildSheet();

	void OpenWindow( HINSTANCE hInst, HWND hWndOwner );
	void Show( BOOL bShow );
	void GetDataFromSurfs(void);
	void Build();
	void BuildStep(DWORD Flags);
	void RefreshStats();

	void SaveSettings();
	void LoadSettings();

	void GetBuildOpts();

	PROPSHEETPAGE m_pages[eBS_MAX]{};
    PROPSHEETHEADER m_psh{};
	HWND m_hwndSheet;
	HWND m_hWndTooltip;
	BOOL m_bShow;
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
