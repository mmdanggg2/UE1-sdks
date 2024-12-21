#pragma once

struct FPolyFlagItem
{
	const TCHAR* FlagName;
	DWORD FlagBit, ExcludeBit;
	FMenuItem* Item;

	FPolyFlagItem(const TCHAR* N, DWORD B, DWORD E = 0)
		: FlagName(N), FlagBit(B), ExcludeBit(E), Item(NULL)
	{}
};
enum
{
	EMEID_Import=1,
	EMEID_Save,
	EMEID_SaveAs,
};
static DWORD _PrevRCFlags = 0;

static void RC_MeshFaceSelect(INT ID)
{
	GEditor->MeshEdSelect(ID);
}
static void RC_MeshSetFlag(INT ID)
{
	UBOOL bAdd = (_PrevRCFlags & ID) == 0;
	GEditor->MeshEdSetFlags(ID, bAdd);
}
static void RC_MeshSmoothGroup(INT ID)
{
	GEditor->MeshEdSetFlags(ID, 2);
}
static void RC_MeshSetIndex(INT ID)
{
	GEditor->MeshEdSetTexture(ID & 0xFF);
}
static void RC_MeshEditPoly(INT ID)
{
	switch (ID)
	{
	case 0:
		GEditor->MeshEdDeleteSurf();
		break;
	case 1:
		GEditor->MeshEdNewSurf();
		break;
	case 2:
		GEditor->MeshEdSurfFlip();
		break;
	}
}

class WMeshEditor : public WCustomWindowBase
{
public:
	static WMeshEditor* GMeshEditor;
	UViewport* pViewport;
	FString PrevFilePath;

	WMeshEditor(HWND hWndParent)
	{
		guard(WMeshEditor::WMeshEditor);
		RECT desktop; // Get the size of screen to the variable desktop
		::GetWindowRect(GetDesktopWindow(), &desktop);
		INT xs = appRound(desktop.right * 0.88);
		INT ys = appRound(desktop.bottom * 0.88);
		PerformCreateWindowEx(WS_EX_CLIENTEDGE, TEXT("Mesh Editor"), WS_OVERLAPPEDWINDOW, desktop.right * 0.08, desktop.bottom * 0.08, xs, ys, hWndParent, NULL, NULL);

		// Create the mesh viewport
		pViewport = GEditor->Client->NewViewport(TEXT("MeshEditor"));
		check(pViewport);
		GEditor->Level->SpawnViewActor(pViewport);
		pViewport->Input->Init(pViewport);
		check(pViewport->Actor);
		pViewport->Actor->ShowFlags = SHOW_StandardView | SHOW_NoButtons | SHOW_ChildWindow;
		pViewport->Actor->RendMap = REN_MeshEditor;
		pViewport->Actor->Misc1 = 0;
		pViewport->Actor->Misc2 = 0;
		pViewport->OpenWindow(hWnd, 0, xs, ys, 0, 0);

		HMENU MenuBar = CreateMenu();

		// Main menu
		HMENU hSubMenu = CreatePopupMenu();
		AppendMenuW(hSubMenu, MF_STRING, EMEID_Import, TEXT("&Import mesh"));
		AppendMenuW(hSubMenu, MF_STRING, EMEID_Save, TEXT("&Save..."));
		AppendMenuW(hSubMenu, MF_STRING, EMEID_SaveAs, TEXT("Save &as..."));
		AppendMenuW(MenuBar, MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(hSubMenu), TEXT("&File"));

		SetMenu(hWnd, MenuBar);

		PositionChildControls();
		FString RenderDevice;
		GConfig->GetString(TEXT("Engine.Engine"), TEXT("WindowedRenderDevice"), RenderDevice);
		TryRenderDevice(pViewport, RenderDevice);

		Show(1);
		unguard;
	}
	void OnSize(DWORD Flags, INT NewX, INT NewY)
	{
		guard(WMeshEditor::OnSize);
		PositionChildControls();
		InvalidateRect(hWnd, NULL, FALSE);
		unguard;
	}
	void PositionChildControls()
	{
		guard(WMeshEditor::PositionChildControls);
		if (!pViewport)
			return;
		LockWindowUpdate(hWnd);

		FRect CR;
		CR = GetClientRect();

		::MoveWindow((HWND)pViewport->GetWindow(), 0, 0, CR.Width(), CR.Height(), 1);
		pViewport->Repaint(1);

		// Refresh the display.
		LockWindowUpdate(NULL);

		unguard;
	}
	void OnDestroy()
	{
		guard(WMeshEditor::OnDestroy);
		WCustomWindowBase::OnDestroy();
		WMeshEditor::GMeshEditor = NULL;
		unguard;
	}

	void MeshFileOpen()
	{
		guard(MeshFileOpen);

		OPENFILENAMEA ofn;
		char File[8192] = "\0";

		ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
		ofn.lStructSize = sizeof(OPENFILENAMEA);
		ofn.hwndOwner = hWnd;
		ofn.lpstrFile = File;
		ofn.nMaxFile = sizeof(File);
		ofn.lpstrFilter = "Mesh Files (*.psk,*_d.3d)\0*.psk;*_d.3d\0All Files\0*.*\0\0";
		ofn.lpstrInitialDir = appToAnsi(*(GLastDir[eLASTDIR_BRUSH]));
		ofn.lpstrDefExt = "psk";
		ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

		// Display the Open dialog box.
		if (GetOpenFileNameA(&ofn))
		{
			// Convert the ANSI filename to UNICODE, and tell the editor to open it.
			FString S = (TCHAR*)appFromAnsi(File);
			PrevFilePath = S;
			GEditor->Exec(*(FString::Printf(TEXT("MESHED LOAD FILE=\"%ls\""), *S)));
			GLastDir[eLASTDIR_BRUSH] = S.GetFilePath().LeftChop(1);
		}
		GFileManager->SetDefaultDirectory(appBaseDir());
		unguard;
	}
	void MeshFileSaveAs()
	{
		guard(MeshFileSaveAs);

		OPENFILENAMEA ofn;
		char File[8192];
		appToAnsiInPlace(File, *PrevFilePath, sizeof(File));

		ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
		ofn.lStructSize = sizeof(OPENFILENAMEA);
		ofn.hwndOwner = hWnd;
		ofn.lpstrFile = File;
		ofn.nMaxFile = sizeof(File);
		const TCHAR* Ext = PrevFilePath.GetFileExtension();
		if (!appStricmp(Ext, TEXT("3d")))
			ofn.lpstrFilter = "VertMesh Files (*.3d)\0*_d.3d\0All Files\0*.*\0\0";
		else ofn.lpstrFilter = "SkeletalMesh Files (*.psk)\0*.psk\0All Files\0*.*\0\0";
		ofn.lpstrInitialDir = appToAnsi(*PrevFilePath.GetFilePath().LeftChop(1));
		ofn.lpstrDefExt = appToAnsi(Ext);
		ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;

		// Display the Open dialog box.
		if (GetSaveFileNameA(&ofn))
		{
			// Convert the ANSI filename to UNICODE, and tell the editor to open it.
			FString S = (TCHAR*)appFromAnsi(File);
			GEditor->Exec(*(FString::Printf(TEXT("MESHED SAVE FILE=\"%ls\""), *S)));
			GLastDir[eLASTDIR_BRUSH] = S.GetFilePath().LeftChop(1);
		}
		GFileManager->SetDefaultDirectory(appBaseDir());
		unguard;
	}
	void OnCommand(INT Command)
	{
		switch (Command)
		{
		case EMEID_Import:
			MeshFileOpen();
			break;
		case EMEID_Save:
			GEditor->Exec(TEXT("MESHED SAVE"));
			break;
		case EMEID_SaveAs:
			if (PrevFilePath.Len())
				MeshFileSaveAs();
			break;
		}
	}
	void RC_Menu()
	{
		static FPersistentRCMenu Menu;
		static FMenuGroup* GFlagPopup, * GTexIndexPopup, * GSurfSmoothGroup;
		static FMenuItem* GDelPoly, * GFlipPoly, * GSelSmooth;
		static FMenuItem* GTexPoly[12];
		static FMenuItem* GSurfSmooth[10];
		static FPolyFlagItem PFlags[] = {
			FPolyFlagItem(TEXT("Normal"),PF_None,(PF_Invisible | PF_Masked | PF_Translucent | PF_Modulated | PF_AlphaBlend | PF_TwoSided)),
			FPolyFlagItem(TEXT("Normal Two-Sided"),PF_TwoSided,(PF_Invisible | PF_Masked | PF_Translucent | PF_Modulated | PF_AlphaBlend)),
			FPolyFlagItem(TEXT("Masked"),PF_Masked),
			FPolyFlagItem(TEXT("Translucent"),PF_Translucent),
			FPolyFlagItem(TEXT("Modulated"),PF_Modulated),
			FPolyFlagItem(TEXT("Alpha Blend"),PF_AlphaBlend),
			FPolyFlagItem(TEXT("Weapon"),PF_Invisible),
			FPolyFlagItem(TEXT("Unlit"),PF_Unlit),
			FPolyFlagItem(TEXT("Flat"),PF_Flat),
			FPolyFlagItem(TEXT("Environment"),PF_Environment),
			FPolyFlagItem(TEXT("No Smooth"),PF_NoSmooth) };

		if (!Menu.bMenuCreated)
		{
			Menu.AddPopup(TEXT("Select...\t&S"));
			Menu.AddItem(TEXT("Matching texture group"), &RC_MeshFaceSelect, 0);
			Menu.AddItem(TEXT("Matching polyflags"), &RC_MeshFaceSelect, 1);
			Menu.AddItem(TEXT("Matching polyflags+texture"), &RC_MeshFaceSelect, 2);
			GSelSmooth = Menu.AddItem(TEXT("Matching smoothgroups"), &RC_MeshFaceSelect, 3);
			Menu.EndPopup();

			GFlagPopup = Menu.AddPopup(TEXT("Edit Flags...\t&F"));
			for (INT i = 0; i < ARRAY_COUNT(PFlags); ++i)
			{
				PFlags[i].Item = Menu.AddItem(PFlags[i].FlagName, &RC_MeshSetFlag, PFlags[i].FlagBit);
				if (i == 6)
					Menu.AddLineBreak();
			}
			Menu.EndPopup();

			GTexIndexPopup = Menu.AddPopup(TEXT("Edit TexIndex...\t&T"));
			for (INT i = 0; i < 12; ++i)
				GTexPoly[i] = Menu.AddItem(*FString::Printf(TEXT("#%i ..."),i), &RC_MeshSetIndex, i);
			Menu.EndPopup();

			GSurfSmoothGroup = Menu.AddPopup(TEXT("Edit SmoothGroups...\t&G"));
			for (INT i = 0; i < 10; ++i)
				GSurfSmooth[i] = Menu.AddItem(*FString::Printf(TEXT("#%i ..."), i), &RC_MeshSmoothGroup, i);
			Menu.EndPopup();

			GFlipPoly = Menu.AddItem(TEXT("Flip Polygon\t&P"), &RC_MeshEditPoly, 2);
			GDelPoly = Menu.AddItem(TEXT("Delete Polygon\t&D"), &RC_MeshEditPoly, 0);
			Menu.AddItem(TEXT("New Polygon\t&N"), &RC_MeshEditPoly, 1);
		}

		BYTE curIndex, maxIndex;
		DWORD curSmooth;
		UBOOL bSkeletalMesh;
		if (GEditor->MeshEdGetInfo(&_PrevRCFlags, &curIndex, &maxIndex, &curSmooth, bSkeletalMesh))
		{
			++maxIndex;
			GFlagPopup->SetHidden(FALSE);
			GTexIndexPopup->SetHidden(FALSE);
			GFlipPoly->SetDisabled(FALSE);
			GDelPoly->SetDisabled(FALSE);
			GSelSmooth->SetDisabled(!bSkeletalMesh);
			for (INT i = 0; i < ARRAY_COUNT(PFlags); ++i)
				PFlags[i].Item->SetChecked(((_PrevRCFlags & PFlags[i].FlagBit) == PFlags[i].FlagBit) && ((_PrevRCFlags & PFlags[i].ExcludeBit) == 0));
			for (INT i = 0; i < 12; ++i)
			{
				GTexPoly[i]->SetChecked(i == curIndex);
				GTexPoly[i]->SetDisabled(i > maxIndex);
			}
			GSurfSmoothGroup->SetHidden(!bSkeletalMesh);
			if (bSkeletalMesh)
			{
				for (INT i = 0; i < 10; ++i)
					GSurfSmooth[i]->SetChecked(i == curSmooth);
			}
		}
		else
		{
			GFlagPopup->SetHidden(TRUE);
			GTexIndexPopup->SetHidden(TRUE);
			GFlipPoly->SetDisabled(TRUE);
			GDelPoly->SetDisabled(TRUE);
			GSurfSmoothGroup->SetHidden(TRUE);
		}
		if (Menu.OpenMenu(hWnd) != INDEX_NONE)
			pViewport->Repaint(1);
	}
};

WMeshEditor* WMeshEditor::GMeshEditor = NULL;
