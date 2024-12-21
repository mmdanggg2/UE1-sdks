
#include "Shtypes.h"
#include "Shlobj.h"
#include "Window.h"

void GetMousePos(INT& X, INT& Y, bool bAbsolute = false);

inline COLORREF GetDrawColor(BYTE Type)
{
	switch (Type)
	{
	case 0:
		return RGB(0, 0, 255);
	case 1:
		return RGB(128, 0, 128);
	case 2:
		return RGB(255, 255, 0);
	case 3:
		return RGB(255, 40, 40);
	case 4:
		return RGB(128, 128, 0);
	case 5:
		return RGB(255, 0, 255);
	case 255:
		return RGB(128, 128, 128);
	default:
		return RGB(0, 0, 0);
	}
}

inline ULevel* GetLevel()
{
	return (ULevel*)UEngine::GEngine->edGetSelection(ULevel::StaticClass());
}
