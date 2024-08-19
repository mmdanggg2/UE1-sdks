#pragma warning(push)
#pragma warning(disable: 4121) /* 'JOBOBJECT_IO_RATE_CONTROL_INFORMATION_NATIVE_V2': alignment of a member was sensitive to packing */
#include <windows.h>
#pragma warning(pop)
#include <commctrl.h>

enum eBS {
	eBS_OPTIONS		= 0,
	eBS_STATS		= 1,
	eBS_MAX			= 2
};

class TBuildSheet
{
public:

	TBuildSheet();
	~TBuildSheet();

	void OpenWindow( HINSTANCE hInst, HWND hWndOwner );
	void Show( BOOL bShow );
	void GetDataFromSurfs(void);
	void Build();
	void BuildBSP();
	void RefreshStats();

	PROPSHEETPAGE m_pages[eBS_MAX]{};
    PROPSHEETHEADER m_psh{};
	HWND m_hwndSheet;
	BOOL m_bShow;
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
