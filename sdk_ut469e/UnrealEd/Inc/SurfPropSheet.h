#pragma warning(push)
#pragma warning(disable: 4121) /* 'JOBOBJECT_IO_RATE_CONTROL_INFORMATION_NATIVE_V2': alignment of a member was sensitive to packing */
#include <windows.h>
#pragma warning(pop)
#include <commctrl.h>

enum eSPS {
	eSPS_FLAGS1		= 0,
	//eSPS_FLAGS2		= 1,
	eSPS_ALIGNMENT	= 1,
	eSPS_STATS		= 2,
	eSPS_MAX		= 3
};

class TSurfPropSheet
{
public:

	TSurfPropSheet();
	~TSurfPropSheet();

	void OpenWindow( HINSTANCE hInst, HWND hWndOwner );
	void Show( BOOL bShow );
	void GetDataFromSurfs1();
	void RefreshStats();

	PROPSHEETPAGE m_pages[eSPS_MAX]{};
    PROPSHEETHEADER m_psh{};
	HWND m_hwndSheet;
	BOOL m_bShow;
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
