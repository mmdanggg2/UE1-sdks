
#include "windows.h"
#include "ScriptedAI.h"
#include "ScWindows.h"
#include "UnFont.h"

static class WKismetBrowserWindow* BrowserWindow = NULL;

#define ID_CLOSE_MENU 5001
#define ID_REFRESH_VIEW 5002
#define ID_NEWKISMET 5003
#define ID_COPYTOCLIPBOARDKS 5004
#define ID_FLIPMOUSEDRAG 5199
#define ID_RENDERDEVICE 5200
#define ID_KISMETITEM 5500
#define ID_DELKISMETITEM 6500

#define ID_DELSELACTOR 2
#define ID_DUPLICACTOR 4
#define ID_CENTERVIEW 5
#define ID_CLIENTACTION 6
#define ID_CREATEGROUP 7
#define ID_EDITGROUP 8
#define ID_DELETEGROUP 9
#define ID_KISMETSPECIALBREAK 10
#define ID_ACTIONCLASSES 20
#define ID_VARCUSTOMACTION 2000

enum EGroupColors : DWORD
{
	GROUPC_Neon,
	GROUPC_Red,
	GROUPC_Blue,
	GROUPC_Green,
	GROUPC_Yellow,
	GROUPC_Grey,
	GROUPC_MAX,
};
static UBOOL bFontsInit = FALSE;
static UFont* MedFont = NULL;
static UFont* UWhiteFont = NULL;
static UTexture* WhiteTex = NULL;
static UTexture* ArrowTex = NULL;
static UTexture* SphereTex = NULL;
static UTexture* GroupTex[GROUPC_MAX];
static FTextureInfo* WhiteInfo;
static FTextureInfo* ArrowTexInfo;
static FTextureInfo* SphereTexInfo;
static FTextureInfo* GroupTexInfo[GROUPC_MAX];
static const FPlane HighlightColored(1.f, 1.f, 0.6f, 1.f);
static const FColor GroupTexColors[GROUPC_MAX] = { FColor(60,85,106,255),FColor(142,105,105,255),FColor(98,104,142,255),FColor(100,148,106,255),FColor(132,132,106,255),FColor(142,142,142,255) };
static const FColor GroupTextColors[GROUPC_MAX] = { FColor(128, 250, 255, 255),FColor(186,32,32,255),FColor(130,130,255,255),FColor(130,255,130,255),FColor(255,255,128,255),FColor(175,175,175,255) };
static const FPlane GroupEdgeColors[GROUPC_MAX] = { FPlane(0.89f,0.89f,0.65f,1.f),FPlane(0.72f,0.42f,0.42f,1.f),FPlane(0.56f,0.42f,0.80f,1.f),FPlane(0.42f,0.92f,0.35f,1.f),FPlane(0.92f,0.92f,0.45f,1.f),FPlane(0.74f,0.74f,0.74f,1.f) };
static const TCHAR* GroupColorNames[GROUPC_MAX] = { TEXT("Default"),TEXT("Red"),TEXT("Blue"),TEXT("Green"),TEXT("Yellow"),TEXT("Grey") };

inline void CorrectNewLines(FString& Str)
{
	TCHAR* S = &Str[0];
	while (*S)
	{
		if (*S == '|')
			*S = '\n';
		++S;
	}
}

struct FRenderDeviceList
{
	struct FRenderDevItem
	{
		FString ClassName, ItemName;
		FRenderDevItem(const FString& InClass)
			: ClassName(InClass)
		{
			INT iSplitter = InClass.InStr(TEXT("."));
			FString ClName = InClass.Mid(iSplitter+1);
			FString ClPackage = InClass.Left(iSplitter);
			ItemName = Localize(*ClName, TEXT("ClassCaption"), *ClPackage) + TEXT(" (") + ClPackage + TEXT(")");
		}
	};
	TArray<FRenderDevItem> List;

	FRenderDeviceList()
	{
		TArray<FRegistryObjectInfo> Result;
		UObject::GetRegistryObjects(Result, UClass::StaticClass(), URenderDevice::StaticClass(), FALSE);

		for (INT i = 0; i < Result.Num(); ++i)
			new (List) FRenderDevItem(Result(i).Object);
	}
	void PopulateMenu(UViewport* Viewport, HMENU Menu)
	{
		const TCHAR* CurRender = Viewport->RenDev ? Viewport->RenDev->GetClass()->GetPathName() : TEXT("");

		for (INT i = 0; i < List.Num(); ++i)
			AppendMenuW(Menu, MF_STRING | ((List(i).ClassName == CurRender) ? MF_CHECKED : MF_UNCHECKED), ID_RENDERDEVICE + i, *List(i).ItemName);
	}
	void UpdateStatus(UViewport* Viewport, HMENU Menu)
	{
		if (!Viewport->RenDev)
			return;
		const TCHAR* CurRender = Viewport->RenDev->GetClass()->GetPathName();

		for (INT i = 0; i < List.Num(); ++i)
			ModifyMenuW(Menu, ID_RENDERDEVICE + i, MF_BYCOMMAND | MF_STRING | ((List(i).ClassName == CurRender) ? MF_CHECKED : MF_UNCHECKED), ID_RENDERDEVICE + i, *List(i).ItemName);
	}
};

class FKismetHook : public FNotifyHook
{
public:
	void NotifyPostChange(void* Src);

	void NotifyExec(void* Src, const TCHAR* Cmd)
	{
		GEngine->NotifyExec(Src, Cmd);
	}
};

class FObjectsItemKSMT : public FObjectsItem
{
public:
	FObjectsItemKSMT(WPropertiesBase* InOwnerProperties, FTreeItem* InParent, DWORD InFlagMask, const TCHAR* InCaption, UBOOL InByCategory)
		: FObjectsItem(InOwnerProperties, InParent, InFlagMask, InCaption, InByCategory)
	{}
	void Expand()
	{
		guard(FObjectsItemKSMT::Expand);
		ValidateSelection();

		if (BaseClass)
		{
			// Add class/name property.
			UProperty* P = FindField<UProperty>(UObject::StaticClass(), TEXT("Name"));
			Children.AddItem(new FPropertyItem(OwnerProperties, this, P, P->GetFName(), P->Offset, -1));

			// Expand to show categories.
			TArray<FName> Categories;
			for (TFieldIterator<UProperty> It(BaseClass); It; ++It)
			{
				if (AcceptFlags(It->PropertyFlags) && It->Category != NAME_Object)
				{
					if (It->Category == It->GetOwnerClass()->GetFName())
						Children.AddItem(new FPropertyItem(OwnerProperties, this, *It, It->GetFName(), It->Offset, -1));
					else Categories.AddUniqueItem(It->Category);
				}
			}
			for (INT i = 0; i < Categories.Num(); i++)
				Children.AddItem(new FCategoryItem(OwnerProperties, this, BaseClass, Categories(i), 1));
			Children.AddItem(new FHiddenCategoryList(OwnerProperties, this, BaseClass));
		}

		FTreeItem::Expand();
		unguard;
	}
	void NotifyEditProperty(UProperty* P, UProperty* SubProperty, INT ArrayIndex)
	{
		guard(FObjectsItemKSMT::NotifyEditProperty);
		FObjectsItem::NotifyEditProperty(P, SubProperty, ArrayIndex);

		for(INT i=0; i< _Objects.Num(); ++i)
		{
			USObjectBase* Obj = Cast<USObjectBase>(_Objects(i));
			if (Obj)
				Obj->bEditDirty = TRUE;
		}
		unguard;
	}
};

class WKismetPrefsSplitter : public WProperties
{
public:
	FObjectsItemKSMT Root;

	WKismetPrefsSplitter(WWindow* OwnerWin)
		: WProperties(TEXT("KSMT_Prefs"), OwnerWin)
		, Root(this, NULL, CPF_Edit, TEXT(""), 1)
	{}
	FTreeItem* GetRoot()
	{
		return &Root;
	}
	void OnLeftButtonDown(INT X, INT Y);
};

struct FKismetAPI : public FViewportCallback
{
	UTexture* BgTexture;
	UTexture* HaloTexture;
	FRenderEntry* RenderList;
	FRenderArea ScreenArea;
	FMouseTarget MouseTarget;
	TArray<FGroupObject*> IndexedGroups;
	FDynEventLine* MouseEventLine;
	AScriptedAI* Manager;
	UBOOL bDragged;
	FVector2D MouseLastPos;
	FString Descriptor;
	UBOOL bMirrorMouseDrag;

	enum EMouseCursorType : BYTE
	{
		POINTER_Normal = EEditorMode::EM_CustomCursor,
		POINTER_All,
		POINTER_NESW,
		POINTER_NS,
		POINTER_NWSE,
		POINTER_WE,
		POINTER_Wait,
	} MouseCursorType;

	FKismetAPI()
		: FViewportCallback(nullptr)
		, RenderList(nullptr)
		, MouseEventLine(nullptr)
		, Manager(nullptr)
		, bDragged(FALSE)
		, MouseCursorType(POINTER_Normal)
	{
		//BgColor = GetDefault<UNativeHook>()->BGColor;
		BgTexture = GetDefault<UNativeHook>()->BgTexture;
		HaloTexture = GetDefault<UNativeHook>()->HaloTexture;
		if (!GConfig->GetBool(TEXT("Settings"), TEXT("MirrorDrag"), bMirrorMouseDrag, ScConfigInit))
			bMirrorMouseDrag = FALSE;
	}
	~FKismetAPI() noexcept(false)
	{
		ExitManager();
	}

	inline void SetViewport(UViewport* V)
	{
		Viewport = V;
		V->ChangeOutputAPI(this);
	}

	void ExitManager();
	void SetManager(AScriptedAI* NewManager);

	void DrawKismet(FSceneNode* Frame);

	void CenterView();
	FObjectEntry* AddAction(const FVector2D& Position, UClass* UC);
	FObjectEntry* DuplicateAction(USObjectBase* O);
	void DeleteAction(FObjectEntry* Ent);

	FGroupObject* AddNewGroup(const FString& Text, INT X, INT Y);
	void DeleteGroup(INT Index);
	void EditGroupText(const FString& NewText, INT Index);

	void RefreshSelection();
	void DeselectAll();

	void FlipMouseDrag()
	{
		bMirrorMouseDrag = !bMirrorMouseDrag;
		GConfig->SetBool(TEXT("Settings"), TEXT("MirrorDrag"), bMirrorMouseDrag, ScConfigInit);
	}

	inline void SetRealTime(UBOOL bEnable) const
	{
		guard(FKismetAPI::SetRealTime);
		if (bEnable)
			Viewport->Actor->ShowFlags |= SHOW_RealTime;
		else Viewport->Actor->ShowFlags &= ~SHOW_RealTime;
		unguard;
	}
	inline void Repaint() const
	{
		guardSlow(FKismetAPI::Repaint);
		Viewport->RepaintPending = TRUE;
		unguardSlow;
	}

	void Draw(UBOOL Blit) override
	{
		guard(FKismetAPI::Draw);
		if (!Viewport->Lock(FVector(1, 1, 1), FVector(0, 0, 0), FPlane(0, 0, 0, 0), 0, NULL, NULL))
			return;

		Viewport->Actor->CalcCameraActor = Viewport->Actor;
		Viewport->Actor->CameraRegion = Viewport->Actor->Region;
		URenderBase* Render = GEngine->Render;
		FSceneNode* Frame = Render->CreateMasterFrame(Viewport, FVector(0, 0, 0), FRotator(0, 0, 0), NULL);
		Render->PreRender(Frame);

		UCanvas* C = Viewport->Canvas;
		C->Update(Frame);
		C->DrawColor = FColor(255, 255, 255, 255);

		const FLOAT Dim = C->ClipY;
		C->DrawTile(BgTexture, 0.f, 0.f, C->ClipX - Dim, Dim, 0.f, 0.f, 0.f, 0.f, nullptr, 1.f, FPlane(1, 1, 1, 1), FPlane(0, 0, 0, 0), PF_TwoSided);
		C->DrawTile(BgTexture, C->ClipX - Dim, 0.f, Dim, Dim, 0.f, 0.f, BgTexture->USize, BgTexture->VSize, nullptr, 1.f, FPlane(1, 1, 1, 1), FPlane(0, 0, 0, 0), PF_TwoSided);
		
		DrawKismet(Frame);

		// Finish draw frame
		Render->PostRender(Frame);
		Viewport->Unlock(Blit);
		Render->FinishMasterFrame();

		++GFrameNumber;
		unguard;
	}

	UBOOL InputEvent(EInputKey iKey, EInputAction State, FLOAT Delta) override
	{
		guard(FKismetAPI::InputEvent);
		if (Manager)
		{
			if (iKey == IK_MouseWheelDown)
			{
				if (State == IST_Press)
					Manager->WinZoom = Max(Manager->WinZoom - 0.2f, 0.2f);
				return FALSE;
			}
			if (iKey == IK_MouseWheelUp)
			{
				if (State == IST_Press)
					Manager->WinZoom = Min(Manager->WinZoom + 0.2f, 2.f);
				return FALSE;
			}
			if (iKey == IK_Delete)
			{
				if (State == IST_Press)
					DeleteFocused();
				return FALSE;
			}
		}
		return FViewportCallback::InputEvent(iKey, State, Delta);
		unguard;
	}

	void Click(DWORD Buttons, FLOAT X, FLOAT Y) override;

	void MouseDelta(DWORD Buttons, FLOAT DX, FLOAT DY) override
	{
		guard(FKismetAPI::MouseDelta);
		if (!Manager)
			return;

		if ((Buttons & MOUSE_FirstHit) && Viewport->Input->KeyDown(IK_RightMouse))
			Viewport->SetMouseCapture(1, 1);
		if (Buttons & MOUSE_LastRelease)
			Viewport->SetMouseCapture(0, 0);

		if (Buttons & MOUSE_Right)
		{
			if (Buttons & MOUSE_Left)
			{
				Manager->WinZoom = Clamp(Manager->WinZoom + (DY * 0.025f), 0.2f, 1.f);
			}
			else if (bMirrorMouseDrag)
			{
				Manager->WinPos[0] -= (DX / Manager->WinZoom);
				Manager->WinPos[1] -= (DY / Manager->WinZoom);
			}
			else
			{
				Manager->WinPos[0] += (DX / Manager->WinZoom);
				Manager->WinPos[1] += (DY / Manager->WinZoom);
			}
		}
		unguard;
	}
	void MousePosition(DWORD Buttons, FLOAT X, FLOAT Y) override;

	BYTE GetCursor() override
	{
		return MouseCursorType;
	}
	const TCHAR* GetWindowName() override
	{
		return TEXT("Kismet Browser");
	}
	void TraceMouse(FLOAT X, FLOAT Y);

	void DeleteFocused();

	void SetTooltipText(const TCHAR* Text, const TCHAR* ObjectName = NULL)
	{
		guard(FKismetAPI::SetTooltipText);
		if (Text && *Text)
		{
			if (ObjectName && *ObjectName)
				Descriptor = FString(ObjectName) + TEXT("|") + Text;
			else Descriptor = Text;
			CorrectNewLines(Descriptor);
		}
		else Descriptor.Empty();
		Repaint();
		unguard;
	}
};

constexpr FLOAT TooltipShowTime = 1.f;
static const TCHAR STR_CopyToClip[] = TEXT("&Copy to clipboard");

class WKismetBrowserWindow : public WCustomWindowBase, public FOutputDevice
{
public:
	WKismetPrefsSplitter* Preferences;
	INT CurSplitSize;
	WDragInterceptor* DragInterceptor;
	FKismetHook Hook;
	FTime HideToolTimer;
	UViewport* pViewport;
	FKismetAPI* API;
	FObjectEntry* EditingObject;

	TArray<INT> KismetObjectItems;
	HMENU hKismetObjectsMenu, hKismetObjectDel, hKismetObjectSettings, MainWinMenu;
	FRenderDeviceList DeviceList;
	FString CurrentRenDev;
	UBOOL bHadLineBreak{};

	static AScriptedAI* SelectedManager;

	WKismetBrowserWindow()
		: WCustomWindowBase()
		, Preferences(nullptr)
		, CurSplitSize(400)
		, DragInterceptor(NULL)
		, pViewport(NULL)
		, EditingObject(NULL)
	{
		guard(WKismetBrowserWindow::WKismetBrowserWindow);
		BrowserWindow = this;

		// Obtain desired render device.
		GConfig->GetString(TEXT("Settings"), TEXT("RenDev"), CurrentRenDev, ScConfigInit);

		RECT desktop; // Get the size of screen to the variable desktop
		::GetWindowRect(GetDesktopWindow(), &desktop);

		PerformCreateWindowEx(WS_EX_CLIENTEDGE, TEXT("Kismet Browser"), WS_OVERLAPPEDWINDOW, desktop.right*0.08, desktop.bottom*0.08, desktop.right*0.88, desktop.bottom*0.88, NULL, NULL, NULL);

		HMENU hSubMenu;
		MainWinMenu = CreateMenu();

		// Main (menu)
		hSubMenu = CreatePopupMenu();
		AppendMenuW(hSubMenu, MF_STRING, ID_REFRESH_VIEW, TEXT("&Refresh selection"));
		AppendMenuW(hSubMenu, MF_STRING, ID_CLOSE_MENU, TEXT("&Close"));
		AppendMenuW(MainWinMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, TEXT("&Main"));

		hKismetObjectsMenu = CreatePopupMenu();
		AppendMenuW(hKismetObjectsMenu, MF_STRING, ID_NEWKISMET, TEXT("&New object"));
		AppendMenuW(hKismetObjectsMenu, MF_STRING | MF_DISABLED, ID_COPYTOCLIPBOARDKS, STR_CopyToClip);
		AppendMenuW(hKismetObjectsMenu, MF_MENUBARBREAK, 0, NULL);
		AppendMenuW(MainWinMenu, MF_STRING | MF_POPUP, (UINT_PTR)hKismetObjectsMenu, TEXT("&Kismet objects"));

		hKismetObjectDel = CreatePopupMenu();
		AppendMenuW(MainWinMenu, MF_STRING | MF_POPUP, (UINT_PTR)hKismetObjectDel, TEXT("&Delete kismet objects"));

		hKismetObjectSettings = CreatePopupMenu();
		AppendMenuW(MainWinMenu, MF_STRING | MF_POPUP, (UINT_PTR)hKismetObjectSettings, TEXT("&Render Device"));


		SetMenu(hWnd, MainWinMenu);

		Preferences = new WKismetPrefsSplitter(this);
		Preferences->OpenChildWindow(0);
		Preferences->NotifyHook = &Hook;

		pViewport = UBitmap::__Client->NewViewport(TEXT("KismetWindow"));
		API = new FKismetAPI;
		API->SetViewport(pViewport);
		GetLevel()->SpawnViewActor(pViewport);
		pViewport->Input->Init(pViewport);
		check(pViewport->Actor);
		pViewport->Actor->ShowFlags = SHOW_StandardView | SHOW_NoButtons | SHOW_ChildWindow;
		pViewport->Actor->RendMap = REN_DynLight;
		pViewport->Group = NAME_None;

		// Init positioning.
		OnSize(0, 0, 0);

		AppendMenuW(hKismetObjectSettings, MF_STRING | ((API->bMirrorMouseDrag) ? MF_CHECKED : MF_UNCHECKED), ID_FLIPMOUSEDRAG, TEXT("Flip mouse drag"));
		AppendMenuW(hKismetObjectSettings, MF_MENUBARBREAK, 0, NULL);
		DeviceList.PopulateMenu(pViewport, hKismetObjectSettings);

		if (SelectedManager)
		{
			if (SelectedManager->IsValid())
				API->SetManager(SelectedManager);
			else SelectedManager = NULL;
		}
		unguard;
	}
	void DoDestroy()
	{
		if (EditingObject)
		{
			EditingObject->bIsEditing = FALSE;
			EditingObject = NULL;
		}
		if (BrowserWindow == this)
			BrowserWindow = NULL;
		delete pViewport;
		delete API;
		Preferences->DelayedDestroy();
		WWindow::DoDestroy();
	}
	void OnClose()
	{
		if (BrowserWindow == this)
			BrowserWindow = NULL;
		WWindow::OnClose();
		DelayedDestroy();
	}
	void OnLeftButtonDown(INT X, INT Y)
	{
		if (GetMouseArea() == 1)
		{
			BeginSplitterDrag();
			return;
		}
	}
	void OnRightButtonDown(INT X, INT Y)
	{
	}
	void OnLeftButtonUp()
	{
	}
	void OnRightButtonUp()
	{
	}
	void OnSetFocus(HWND hWndLosingFocus)
	{
		pViewport->RepaintPending = TRUE;
	}
	void OnMouseMove(DWORD Flags, FPoint Location)
	{
	}
	void OnSize(DWORD Flags, INT NewX, INT NewY)
	{
		FRect WinRec = GetClientRect();
		if (Preferences)
			Preferences->MoveWindow(WinRec.Width() - CurSplitSize + 2, 0, CurSplitSize - 2, WinRec.Height(), 1);
		if (pViewport)
		{
			pViewport->OpenWindow(hWnd, 0, WinRec.Width() - CurSplitSize - 2, WinRec.Height(), 0, 0, CurrentRenDev.Len() ? *CurrentRenDev : nullptr);
			pViewport->RepaintPending = TRUE;
		}
	}
	void ChangeRenderDevice(INT Index)
	{
		guard(WKismetBrowserWindow::ChangeRenderDevice);
		if (pViewport)
		{
			if (CurrentRenDev == DeviceList.List(Index).ClassName)
				return;

			CurrentRenDev = DeviceList.List(Index).ClassName;
			if (pViewport->RenDev)
			{
				pViewport->RenDev->Exit();
				delete pViewport->RenDev;
				pViewport->RenDev = NULL;
			}
			GConfig->SetString(TEXT("Settings"), TEXT("RenDev"), *CurrentRenDev, ScConfigInit);
			GConfig->Flush(FALSE, ScConfigInit);
		}
		OnSize(0, 0, 0); // Flush viewport.
		DeviceList.UpdateStatus(pViewport, hKismetObjectSettings);
		unguard;
	}
	void OnCommand(INT Command);

	BYTE GetMouseArea()
	{
		INT XSplit = GetCurrentSplitPos();
		FPoint P = GetCursorPos();
		P.X -= GetClientRect().Min.X;
		if (Abs(P.X - XSplit) <= 6)
			return 1;
		else if (P.X>XSplit)
			return 2;
		return 0;
	}
	INT GetCurrentSplitPos()
	{
		FRect WinRec = GetClientRect();
		return WinRec.Width() - CurSplitSize;
	}
	INT OnSetCursor()
	{
		guard(WKismetBrowserWindow::OnSetCursor);
		BYTE Pos = GetMouseArea();
		if (Pos == 1)
		{
			SetCursor(LoadCursor(hInstanceWindow, MAKEINTRESOURCE(IDC_SplitWE)));
			return 1;
		}
		else if (Pos == 2)
			return Preferences->OnSetCursor();
		return 1;
		unguard;
	}
	void BeginSplitterDrag()
	{
		guard(WKismetBrowserWindow::BeginDrag);
		DragInterceptor = new WDragInterceptor(this, FPoint(0, INDEX_NONE), GetClientRect(), FPoint(3, 3));
		DragInterceptor->DragPos = FPoint(GetCurrentSplitPos(), GetCursorPos().Y);
		DragInterceptor->DragClamp = FRect(GetClientRect().Inner(FPoint(64, 0)));
		DragInterceptor->OpenWindow();
		unguard;
	}
	void OnFinishSplitterDrag(WDragInterceptor* Drag, UBOOL Success)
	{
		guard(WKismetBrowserWindow::OnFinishSplitterDrag);
		if (Success)
		{
			CurSplitSize -= Drag->DragPos.X - Drag->DragStart.X;
			InvalidateRect(*this, NULL, 0);
			UpdateWindow(*this);
			OnSize(0, 0, 0);
		}
		DragInterceptor = NULL;
		unguard;
	}

	void OpenPropertiesWindow(FObjectEntry* Obj)
	{
		if (EditingObject)
			EditingObject->bIsEditing = FALSE;
		EditingObject = Obj;
		Obj->bIsEditing = TRUE;
		UObject* Ref = Obj->GetObject();
		Preferences->Root.SetObjects(&Ref, 1);
	}
	void ClosePropertiesWindow()
	{
		if (EditingObject)
		{
			EditingObject->bIsEditing = FALSE;
			EditingObject = NULL;
		}
		Preferences->Root.SetObjects(NULL, 0);
	}
	void NoteObjectRemoved(FObjectEntry* Obj)
	{
		guard(WKismetBrowserWindow::NoteObjectRemoved);
		if (Obj == EditingObject)
			ClosePropertiesWindow();
		unguard;
	}

	void UpdateSelection();
	void SelectKismet(INT Index);
	void DeleteKismet(INT Index);

	void Serialize(const TCHAR* V, EName Event)
	{
		if (Event == NAME_Warning)
			MessageBox(hWnd, V, TEXT("Warning!"), (MB_OK | MB_SYSTEMMODAL | MB_ICONWARNING));
		else MessageBox(hWnd, V, TEXT("Message"), (MB_OK | MB_SYSTEMMODAL));
	}
};
AScriptedAI* WKismetBrowserWindow::SelectedManager = NULL;

void FKismetHook::NotifyPostChange(void* Src)
{
	BrowserWindow->pViewport->RepaintPending = TRUE;
}

void WKismetPrefsSplitter::OnLeftButtonDown(INT X, INT Y)
{
	BYTE Pos = ((WKismetBrowserWindow*)OwnerWindow)->GetMouseArea();
	if (Pos == 1)
		((WKismetBrowserWindow*)OwnerWindow)->BeginSplitterDrag();
	else WProperties::OnLeftButtonDown(X, Y);
}

class WNewKismetWindow : public WCustomWindowBase
{
public:
	WLabel Prompt;
	WCoolButton OkButton;
	WCoolButton CancelButton;
	WEdit EditField;

	virtual void OnClickTrue()
	{
		_CloseWindow();
	}
	virtual void OnClickFalse()
	{
		_CloseWindow();
	}

	WNewKismetWindow(const TCHAR* WinTitle, const TCHAR* Question, const TCHAR* DefString, INT BaseY = 20, const INT SizeY = 150)
		: WCustomWindowBase(0)
		, Prompt(this)
		, OkButton(this, 0, FDelegate(this, (TDelegate)&WNewKismetWindow::OnClickTrue))
		, CancelButton(this, 0, FDelegate(this, (TDelegate)&WNewKismetWindow::OnClickFalse))
		, EditField(this)
	{
		PerformCreateWindowEx(0, TEXT("DummyWindow"), WS_SYSMENU, 150, 150, 250, SizeY, BrowserWindow->hWnd, NULL, NULL);
		SetText(WinTitle);

		FRect AreaRec = GetClientRect();
		const INT XSize = AreaRec.Width();
		const INT YSize = AreaRec.Height();
		Prompt.OpenWindow(1, 0);
		Prompt.SetText(Question);
		Prompt.MoveWindow(10, BaseY, XSize - 20, 16, 1);
		BaseY += 22;

		EditField.OpenWindow(1, 0, 0);
		EditField.MoveWindow(25, BaseY, XSize - 50, 20, 1);
		EditField.SetText(DefString);

		OkButton.OpenWindow(1, 80, YSize - 25, 50, 20, TEXT("OK"));
		CancelButton.OpenWindow(1, 145, YSize - 25, 50, 20, TEXT("Cancel"));
	}
};

static const TCHAR* GrabUniqueName()
{
	UObject* Parent = GetLevel()->GetOuter();
	UClass* Class = AScriptedAI::StaticClass();
	static TCHAR Result[NAME_SIZE];
	TCHAR NewBase[NAME_SIZE];
	TCHAR TempIntStr[NAME_SIZE];

	// Make base name sans appended numbers.
	appStrcpy(NewBase, Class->GetName());
	TCHAR* End = NewBase + appStrlen(NewBase);
	while (End>NewBase && appIsDigit(End[-1]))
		End--;
	*End = 0;

	// Append numbers to base name.
	--Class->ClassUnique;
	do
	{
		appSprintf(TempIntStr, TEXT("_%i"), ++Class->ClassUnique);
		appStrncpy(Result, NewBase, NAME_SIZE - appStrlen(TempIntStr) - 1);
		appStrcat(Result, TempIntStr);
	} while (UObject::StaticFindObject(NULL, Parent, Result));

	return Result;
}

class WNewKismetDialog : public WNewKismetWindow
{
	WComboBox ClassBox;
	UClass* CurKismetClass;
	WLabel DescLabel;

public:
	// Constructor.
	WNewKismetDialog()
		: WNewKismetWindow(TEXT("New ScriptedAI trigger object"), TEXT("Name of object:"), GrabUniqueName(), 135, 250)
		, ClassBox(this)
		, DescLabel(this)
	{
		FRect AreaRec = GetClientRect();

		DescLabel.OpenWindow(1, 1, 0, 10, 30, AreaRec.Width() - 20, 100);
		ClassBox.OpenWindow(TRUE, TRUE, CBS_DROPDOWNLIST, 5, 2, AreaRec.Width() - 10, 20);

		UClass* ScBase = AScriptedAI::StaticClass();
		for (TObjectIterator<UClass> It; It; ++It)
		{
			if (It->IsChildOf(ScBase) && (It->ClassFlags & CLASS_Abstract) != CLASS_Abstract)
				ClassBox.AddString(It->GetPathName());
		}
		ClassBox.SetCurrent(ClassBox.FindString(ScBase->GetPathName()));
		ClassBox.SelectionChangeDelegate = FDelegate(this, (TDelegate)&WNewKismetDialog::OnClassChange);
		OnClassChange();
	}

	void OnClickTrue()
	{
		guard(WNewKismetDialog::OnClickTrue);
		ULevel* Level = GetLevel();
		FName NewObjName(*EditField.GetText());
		bool bResult = true;
		if (NewObjName != NAME_None)
		{
			AScriptedAI* K = FindObject<AScriptedAI>(Level->GetOuter(), *NewObjName, 0);
			if (K)
			{
				BrowserWindow->Log(NAME_Warning, TEXT("Can't create kismet: Duplicate ScriptedAI object name found."));
				bResult = false;
			}
		}
		if (bResult)
		{
			AScriptedAI* MG = (AScriptedAI*)Level->SpawnActor(CurKismetClass, NewObjName);
			if (MG)
			{
				MG->eventEditorInitTrigger();
				BrowserWindow->SelectKismet((INT)MG->GetIndex());
				BrowserWindow->UpdateSelection();
			}
		}
		WNewKismetWindow::OnClickTrue();
		unguard;
	}
	void OnClassChange()
	{
		guard(WNewKismetDialog::OnClassChange);
		CurKismetClass = FindObjectChecked<UClass>(NULL, *ClassBox.GetText(), TRUE);
		FStringOutputDevice InfoStr;
		InfoStr.Logf(TEXT("-[ %ls ]-"), CurKismetClass->GetName());
		if (CurKismetClass->ScriptText)
			GetClassDesc(CurKismetClass, InfoStr);
		else InfoStr.Log(TEXT("\n(no additional information)"));
		DescLabel.SetText(*InfoStr);
		unguard;
	}
};
class WGroupDialogBase : public WNewKismetWindow
{
public:
	WComboBox ColorBox;
	WLabel DescLabel;

	// Constructor.
	WGroupDialogBase(const TCHAR* WinTitle, const TCHAR* Question, const TCHAR* DefString, const INT DefColor = GROUPC_Neon)
		: WNewKismetWindow(WinTitle, Question, DefString)
		, ColorBox(this)
		, DescLabel(this)
	{
		DescLabel.OpenWindow(1, FALSE, 0, 10, 65, 45, 16);
		DescLabel.SetText(TEXT("Color:"));
		ColorBox.OpenWindow(TRUE, FALSE, CBS_DROPDOWNLIST, 57, 63, 75, 20);

		for (INT i = 0; i < GROUPC_MAX; ++i)
			ColorBox.AddString(GroupColorNames[i]);
		ColorBox.SetCurrent(DefColor);
	}

	FString GetDiagText()
	{
		guard(GetDiagText);
		const INT iColor = ColorBox.GetCurrent();
		if (iColor == GROUPC_Neon)
			return EditField.GetText();
		return FString::Printf(TEXT("\\%i"), iColor) + EditField.GetText();
		unguard;
	}
};
class WNewGroupDialog : public WGroupDialogBase
{
public:
	INT XPos, YPos;

	// Constructor.
	WNewGroupDialog(INT X, INT Y)
		: WGroupDialogBase(TEXT("New group..."), TEXT("Group description:"), TEXT(""))
		, XPos(X), YPos(Y)
	{}

	void OnClickTrue()
	{
		BrowserWindow->API->AddNewGroup(GetDiagText(), XPos, YPos);
		WNewKismetWindow::OnClickTrue();
	}
};
class WEditGroupDialog : public WGroupDialogBase
{
public:
	INT GroupIndex;

	// Constructor.
	WEditGroupDialog(const TCHAR* OldText, INT GIndex, INT DefColor)
		: WGroupDialogBase(TEXT("Edit group..."), TEXT("Group description:"), OldText, DefColor)
		, GroupIndex(GIndex)
	{}

	void OnClickTrue()
	{
		BrowserWindow->API->EditGroupText(GetDiagText(), GroupIndex);
		WNewKismetWindow::OnClickTrue();
	}
};

void WKismetBrowserWindow::OnCommand(INT Command)
{
	switch (Command)
	{
	case ID_CLOSE_MENU:
		OnClose();
		break;
	case ID_REFRESH_VIEW:
		UpdateSelection();
		break;
	case ID_NEWKISMET:
		{
			WNewKismetDialog* WTest = new WNewKismetDialog();
			WTest->Show(1);
		}
		break;
	case ID_COPYTOCLIPBOARDKS:
		if (SelectedManager)
		{
			for (FObjectIterator It; It; ++It)
				It->ClearFlags(RF_TagImp | RF_TagExp);

			FStringOutputDevice Ar;
			UExporter::ExportToOutputDevice(SelectedManager, NULL, Ar, TEXT("t3d"), 0);
			appClipboardCopy(*Ar);
		}
		break;
	case ID_FLIPMOUSEDRAG:
		{
			API->FlipMouseDrag();
			ModifyMenuW(hKismetObjectSettings, ID_FLIPMOUSEDRAG, MF_BYCOMMAND | MF_STRING | (API->bMirrorMouseDrag ? MF_CHECKED : MF_UNCHECKED), ID_FLIPMOUSEDRAG, TEXT("Flip mouse drag"));
		}
		break;
	default:
		{
			if (Command >= ID_RENDERDEVICE && Command < ID_KISMETITEM)
			{
				ChangeRenderDevice(Command - ID_RENDERDEVICE);
				break;
			}
			INT Offset = Command - ID_KISMETITEM;
			if (Offset >= 0 && Offset < KismetObjectItems.Num())
			{
				SelectKismet(KismetObjectItems(Offset));
				UpdateSelection();
				pViewport->RepaintPending = TRUE;
			}
			else
			{
				Offset = Command - ID_DELKISMETITEM;
				if (Offset >= 0 && Offset < KismetObjectItems.Num())
				{
					DeleteKismet(KismetObjectItems(Offset));
					UpdateSelection();
					pViewport->RepaintPending = TRUE;
				}
			}
		}
		break;
	}
}
void WKismetBrowserWindow::UpdateSelection()
{
	guard(UpdateSelection);
	for (INT i = 0; i<KismetObjectItems.Num(); ++i)
	{
		RemoveMenu(hKismetObjectsMenu, ID_KISMETITEM + i, MF_BYCOMMAND);
		RemoveMenu(hKismetObjectDel, ID_DELKISMETITEM + i, MF_BYCOMMAND);
	}
	if (bHadLineBreak)
	{
		bHadLineBreak = FALSE;
		RemoveMenu(hKismetObjectsMenu, ID_KISMETSPECIALBREAK, MF_BYCOMMAND);
	}
	KismetObjectItems.Empty();

	for(TActorIterator<AScriptedAI> It(GetLevel()); It; ++It)
	{
		AScriptedAI* M = *It;
		if (M) // Add info.
		{
			if (!M->bInit)
				M->Initialize();
			FString MenuName = FString::Printf(TEXT("%ls (%ls)"), M->GetName(), M->GetClass()->GetName());
			AppendMenuW(hKismetObjectsMenu, MF_STRING | (M == SelectedManager ? MF_CHECKED : 0), ID_KISMETITEM + KismetObjectItems.Num(), *MenuName);
			AppendMenuW(hKismetObjectDel, MF_STRING | (M == SelectedManager ? MF_CHECKED : 0), ID_DELKISMETITEM + KismetObjectItems.Num(), *MenuName);
			KismetObjectItems.AddItem((INT)M->GetIndex());
		}
	}
	for (TObjectIterator<AScriptedAI> It; It; ++It)
	{
		if (It->GetLevel() || !It->GetOuter())
			continue;
		if (!bHadLineBreak)
		{
			bHadLineBreak = TRUE;
			AppendMenuW(hKismetObjectsMenu, MF_MENUBARBREAK, ID_KISMETSPECIALBREAK, NULL);
		}
		if (!It->bInit)
			It->Initialize();
		FString MenuName = FString::Printf(TEXT("%ls (%ls)"), It->GetPathName(), It->GetClass()->GetName());
		AppendMenuW(hKismetObjectsMenu, MF_STRING | (*It == SelectedManager ? MF_CHECKED : 0), ID_KISMETITEM + KismetObjectItems.Num(), *MenuName);
		KismetObjectItems.AddItem((INT)It->GetIndex());
	}
	ModifyMenuW(hKismetObjectsMenu, ID_COPYTOCLIPBOARDKS, MF_BYCOMMAND | MF_STRING | (SelectedManager ? 0 : MF_DISABLED), ID_COPYTOCLIPBOARDKS, STR_CopyToClip);
	UpdateWindow(hWnd);
	pViewport->RepaintPending = TRUE;
	unguard;
}
void WKismetBrowserWindow::SelectKismet(INT Index)
{
	guard(WKismetBrowserWindow::SelectKismet);
	SelectedManager = Cast<AScriptedAI>(UObject::GetIndexedObject(Index));
	if (SelectedManager)
		API->SetManager(SelectedManager);
	unguard;
}
void WKismetBrowserWindow::DeleteKismet(INT Index)
{
	guard(WKismetBrowserWindow::DeleteKismet);
	AScriptedAI* M = Cast<AScriptedAI>(UObject::GetIndexedObject(Index));
	if (!M || M->bDeleteMe)
		return;
	if (!GWarn->YesNof(TEXT("Are you sure you wish to delete %ls?"), M->GetFullName()))
		return;
	if (M == SelectedManager)
	{
		SelectedManager = NULL;
		API->SetManager(NULL);
	}
	GetLevel()->DestroyActor(M);
	unguard;
}

FRenderEntry::FRenderEntry(FKismetAPI* A, BYTE InPri)
	: API(A)
	, Priority(InPri)
{
	guard(FRenderEntry::FRenderEntry);
	if (A->RenderList)
	{
		if (A->RenderList->Priority < InPri)
		{
			Next = A->RenderList;
			A->RenderList = this;
		}
		else
		{
			for (FRenderEntry* R = A->RenderList; R; R = R->Next)
			{
				if (!R->Next)
				{
					Next = nullptr;
					R->Next = this;
					break;
				}
				else if (R->Next->Priority < InPri)
				{
					Next = R->Next;
					R->Next = this;
					break;
				}
			}
		}
	}
	else
	{
		A->RenderList = this;
		Next = nullptr;
	}
	unguard;
}
FRenderEntry::~FRenderEntry() noexcept(false)
{
	guard(FRenderEntry::~FRenderEntry);
	if (API->RenderList == this)
		API->RenderList = Next;
	else
	{
		for(FRenderEntry* R=API->RenderList; R; R=R->Next)
			if (R->Next == this)
			{
				R->Next = Next;
				break;
			}
	}
	unguard;
}

inline FLOAT MakeHighlight(FLOAT V)
{
	return V + ((1.f - V) * 0.6f);
}
inline FPlane HighlightPlane(const FPlane& P)
{
	return FPlane(MakeHighlight(P.X), MakeHighlight(P.Y), MakeHighlight(P.Z), 1.f);
}

constexpr FLOAT LinkWidth = 12.f;
constexpr FLOAT LinkHeight = 6.f;
constexpr FLOAT VarWidth = 8.f;
constexpr FLOAT VarHeight = 12.f;
constexpr FLOAT VarRadius = 76.f;

inline FVector2D CubicInterp(const FVector2D& P0, const FVector2D& T0, const FVector2D& P1, const FVector2D& T1, const FLOAT A)
{
	const FLOAT A2 = (A * A);
	const FLOAT A3 = (A * A * A);

	return (((2 * A3) - (3 * A2) + 1) * P0) + ((A3 - (2 * A2) + A) * T0) + ((A3 - A2) * T1) + (((-2 * A3) + (3 * A2)) * P1);
}

FEventLine::FEventLine(FKismetAPI* A)
	: FRenderEntry(A, RENDORDER_EventLines)
	, LineColor(0.f, 0.f, 0.f, 1.f)
	, Highlight(FALSE)
{
}
void FEventLine::TraceLink(const FVector2D& Start, const FVector2D& SDir, const FVector2D& End, const FVector2D& EDir)
{
	guardSlow(FEventLine::TraceLink);
	INT Detail = Clamp<INT>(appFloor((Start - End).Size() * 0.02f), 12, 48);
	FLOAT Delta = (1.f / (Detail + 1.f));

	Points.Empty(Detail + 2);
	Points.AddItem(Start);
	Bounds = F2DBox(Start);

	for (INT i = 1; i <= Detail; ++i)
	{
		FLOAT Time = Delta * i;
		FVector2D Point = CubicInterp(Start, SDir, End, EDir, Time);
		Points.AddItem(Point);
		Bounds += Point;
	}
	Points.AddItem(End);
	Bounds += End;
	unguardSlow;
}
void FEventLine::UpdateLink(const FVector2D& Start, const FVector2D& End, UBOOL bVariable)
{
	guard(FEventLine::UpdateLink);
	Refresh();
	FVector2D DirStart, DirEnd;
	FLOAT EndDist = 2.f;
	if (bVariable)
	{
		DirStart = FVector2D(0.f, 1.f);
		DirEnd = (Start - End);
		DirEnd.Normalize();
		EndDist = 1.f;
	}
	else
	{
		DirStart = FVector2D(1.f, 0.f);
		DirEnd = FVector2D(-1.f, 0.f);
	}
	FLOAT Dist = (Start - End).Size();
	DirStart = DirStart * Dist * 2.f;
	DirEnd = DirEnd * Dist * (-EndDist);
	TraceLink(Start, DirStart, End, DirEnd);
	unguard;
}
void FEventLine::Draw(FSceneNode* Frame, const FRenderArea& Area)
{
	guard(FEventLine::Draw);
	URenderDevice* RenDev = Frame->Viewport->RenDev;
	const FPlane LC = Highlight ? FPlane(1.f, 1.f, 0.5f, 1.f) : LineColor;
	for (INT i = 1; i < Points.Num(); ++i)
		RenDev->Draw2DLine(Frame, LC, 0, FVector(Area.ToScreen(Points(i - 1)), 5.f), FVector(Area.ToScreen(Points(i)), 5.f));
	unguard;
}

FDynEventLine::FDynEventLine(FKismetAPI* A, const FVector2D& Start, const FVector2D& SDir, const FVector2D& EDir, ELineMode Mode)
	: FEventLine(A)
	, StartPos(Start)
	, StartDir(SDir)
	, EndDir(EDir)
	, LineMode(Mode)
	, MouseCache(A->MouseTarget)
{
	A->MouseEventLine = this;
}
FDynEventLine::~FDynEventLine() noexcept(false)
{
	if (GetAPI()->MouseEventLine == this)
		GetAPI()->MouseEventLine = nullptr;
}
void FDynEventLine::MouseMove(const FVector2D& MousePos)
{
	guard(FDynEventLine::MouseMove);
	switch (LineMode)
	{
	case LINEMODE_VarEnd:
	{
		FVector2D DirEnd = (StartPos - MousePos);
		FLOAT Dist = DirEnd.Size();
		FVector2D DirStart = StartDir * Dist * 2.f;
		DirEnd.Normalize();
		DirEnd = DirEnd * -Dist;
		TraceLink(StartPos, DirStart, MousePos, DirEnd);
		break;
	}
	case LINEMODE_VarStart:
	{
		FVector2D DirStart = (MousePos - StartPos);
		FVector2D NDirStart = DirStart;
		NDirStart.Normalize();
		FVector2D RealStart = StartPos + (NDirStart * (VarRadius * 0.5f));
		FLOAT Dist = (RealStart - MousePos).Size();
		DirStart = NDirStart * Dist;
		FVector2D DirEnd = EndDir * (Dist * -2.f);
		TraceLink(RealStart, DirStart, MousePos, DirEnd);
		break;
	}
	default:
	{
		FLOAT Dist = (StartPos - MousePos).Size() * 2.f;
		FVector2D DirStart = StartDir * Dist;
		FVector2D DirEnd = EndDir * -Dist;
		TraceLink(StartPos, DirStart, MousePos, DirEnd);
		break;
	}
	}
	unguard;
}

FObjectEntry::FObjectEntry(FKismetAPI* A, USObjectBase* O)
	: FRenderEntry(A, RENDORDER_Action)
	, Obj(O)
	, bIsEditing(FALSE)
{
	O->EditData = this;
}
FObjectEntry::~FObjectEntry() noexcept(false)
{
	if (Obj && Obj->IsValid() && Obj->EditData == this)
		Obj->EditData = NULL;
}

FVariableEntry::FVariableEntry(FKismetAPI* A, USVariableBase* O)
	: FObjectEntry(A, O)
	, VarColor(O->DrawColor.Plane())
	, Highlight(FALSE)
{
	Position = FVector2D(O->Location[0], O->Location[1]);
	HiVarColor = HighlightPlane(VarColor);
	O->bEditDirty = TRUE;
}
void FVariableEntry::Update(FSceneNode* Frame)
{
	guard(FVariableEntry::Update);
	USVariableBase* V = reinterpret_cast<USVariableBase*>(Obj);
	V->bEditDirty = FALSE;
	MidPosition = Position + FVector2D(VarRadius * 0.5f, VarRadius * 0.5f);
	Bounds = F2DBox(Position, FVector2D(Position.X + VarRadius, Position.Y + VarRadius));

	Info = V->eventGetInfo();
	INT XS, YS;
	UCanvas* C = Frame->Viewport->Canvas;
	if (Info.Len())
	{
		C->WrappedStrLenf(MedFont, XS, YS, *V->MenuName);
		TextPos = FVector2D((VarRadius - FLOAT(XS)) * 0.5f, VarRadius * 0.5f - YS - 1);
		C->WrappedStrLenf(MedFont, XS, YS, *Info);
		InfoPos = FVector2D((VarRadius - FLOAT(XS)) * 0.5f, VarRadius * 0.5f + 1);
	}
	else
	{
		C->WrappedStrLenf(MedFont, XS, YS, *V->MenuName);
		TextPos = FVector2D((VarRadius - FLOAT(XS)) * 0.5f, (VarRadius - FLOAT(YS)) * 0.5f);
	}
	unguard;
}
void FVariableEntry::Draw(FSceneNode* Frame, const FRenderArea& Area)
{
	guard(FVariableEntry::Draw);
	USVariableBase* V = reinterpret_cast<USVariableBase*>(Obj);
	URenderDevice* RenDev = Frame->Viewport->RenDev;
	const FVector2D Cur = Area.ToScreen(Position);
	const FLOAT Size = VarRadius * Area.Zoom;

	if (bIsEditing)
	{
		const FLOAT HaloExtent = 28.f * Area.Zoom;
		const FLOAT HaloRes = GetAPI()->HaloTexture->USize;
		FTextureInfo* HaloInfo = GetAPI()->HaloTexture->GetTexture(INDEX_NONE, RenDev);

		const FPlane HaloColor(1.f, 1.f, 0.25f, 1.f);
		RenDev->DrawTile(Frame, *HaloInfo, Cur.X - HaloExtent, Cur.Y - HaloExtent, Size + (HaloExtent * 2.f), Size + (HaloExtent * 2.f), 0.f, 0.f, HaloRes, HaloRes, nullptr, 1.f, HaloColor, FPlane(), PF_Translucent);
	}

	RenDev->DrawTile(Frame, *SphereTexInfo, Cur.X - 2, Cur.Y - 2, Size + 4, Size + 4, 0.f, 0.f, 256.f, 256.f, nullptr, 1.f, (Highlight ? HiVarColor : VarColor), FPlane(0, 0, 0, 0), PF_Masked);
	RenDev->DrawTile(Frame, *SphereTexInfo, Cur.X, Cur.Y, Size, Size, 0.f, 0.f, 256.f, 256.f, nullptr, 1.f, (Highlight ? FPlane(0.54f, 0.54f, 0.54f, 1.f) : FPlane(0.42f, 0.42f, 0.42f, 1.f)), FPlane(0, 0, 0, 0), PF_Masked);

	UCanvas* C = Frame->Viewport->Canvas;
	C->DrawColor = FColor(255, 255, 255, 255);
	C->CurX = Cur.X + (TextPos.X * Area.Zoom);
	C->CurY = Cur.Y + (TextPos.Y * Area.Zoom);
	C->WrappedPrintf(MedFont, FALSE, *V->MenuName);
	if (Info.Len())
	{
		C->CurX = Cur.X + (InfoPos.X * Area.Zoom);
		C->CurY = Cur.Y + (InfoPos.Y * Area.Zoom);
		C->WrappedPrintf(MedFont, FALSE, *Info);
	}
	unguard;
}
FVector2D FVariableEntry::GetInputPos(FSceneNode* Frame, const FVector2D& Start)
{
	guard(FVariableEntry::GetInputPos);
	USVariableBase* V = reinterpret_cast<USVariableBase*>(Obj);
	if (V->bEditDirty)
		Update(Frame);
	FVector2D Dir = Start - MidPosition;
	Dir.Normalize();
	return MidPosition + Dir * (VarRadius * 0.5f);
	unguard;
}
void FVariableEntry::CheckMouseHit(const FVector2D& Mouse, FMouseTarget& Result)
{
	guardSlow(FVariableEntry::CheckMouseHit);
	if ((Mouse - MidPosition).SizeSquared() <= (VarRadius * VarRadius * 0.25f))
		Result.Set(this);
	unguardSlow;
}
void FVariableEntry::GainFocus(DWORD Index)
{
	guardSlow(FVariableEntry::GainFocus);
	Highlight = TRUE;
	GetAPI()->SetTooltipText(*GetVariable()->Description, *GetVariable()->MenuName);
	for (INT i = 0; i < VarLinks.Num(); ++i)
	{
		FVarLink& V = VarLinks(i);
		if (V.Action->VarLinks(V.VarIndex).Line)
			V.Action->VarLinks(V.VarIndex).Line->Highlight = TRUE;
		V.Action->VarLinks(V.VarIndex).Highlight = TRUE;
	}
	unguardSlow;
}
void FVariableEntry::LostFocus(DWORD Index)
{
	guardSlow(FVariableEntry::LostFocus);
	Highlight = FALSE;
	for (INT i = 0; i < VarLinks.Num(); ++i)
	{
		FVarLink& V = VarLinks(i);
		if (V.Action->VarLinks(V.VarIndex).Line)
			V.Action->VarLinks(V.VarIndex).Line->Highlight = FALSE;
		V.Action->VarLinks(V.VarIndex).Highlight = FALSE;
	}
	unguardSlow;
}
void FVariableEntry::OnMouseClick(DWORD Index)
{
	guard(FVariableEntry::OnMouseClick);
	if (GetAPI()->MouseEventLine)
	{
		FMouseTarget PrevInfo = GetAPI()->MouseEventLine->MouseCache;
		delete GetAPI()->MouseEventLine;
		GetAPI()->SetRealTime(FALSE);

		if (PrevInfo.Entry->GetType() == ENTITY_Action && PrevInfo.Index >= 2000)
			reinterpret_cast<FActionEntry*>(PrevInfo.Entry)->TryLinkVariable(this, PrevInfo.Index - 2000);
	}
	else
	{
		BrowserWindow->OpenPropertiesWindow(this);
	}
	unguard;
}
void FVariableEntry::DragEvent(const FVector2D& MouseDelta)
{
	guard(FVariableEntry::DragEvent);
	Position.X += MouseDelta.X;
	Position.Y += MouseDelta.Y;
	GetVariable()->Location[0] = appRound(Position.X);
	GetVariable()->Location[1] = appRound(Position.Y);
	GetVariable()->bEditDirty = TRUE;

	for (INT i = 0; i < VarLinks.Num(); ++i)
		VarLinks(i).Action->GetAction()->bEditDirty = TRUE;
	unguard;
}
UBOOL FVariableEntry::BeginDrag(const FVector2D& Mouse)
{
	return TRUE;
}
void FVariableEntry::DisconnectAll()
{
	guard(FVariableEntry::DisconnectAll);
	for (INT i = (VarLinks.Num() - 1); i >= 0; --i)
		VarLinks(i).Action->LinkVariable(nullptr, VarLinks(i).VarIndex);
	unguard;
}

FActionEntry::FActionEntry(FKismetAPI* A, USActionBase* O)
	: FObjectEntry(A, O)
	, ActionColor(O->DrawColor.Plane())
	, Highlight(FALSE)
	, ClientAction(FALSE)
{
	Position = FVector2D(O->Location[0], O->Location[1]);
	HighlightColor = HighlightPlane(ActionColor);
}
FActionEntry::~FActionEntry() noexcept(false)
{
	guard(FActionEntry::~FActionEntry);
	INT i;
	for (i = 0; i < OutputLinks.Num(); ++i)
		delete OutputLinks(i).Line;
	for (i = 0; i < VarLinks.Num(); ++i)
		delete VarLinks(i).Line;
	unguard;
}
void FActionEntry::CollectGarbage()
{
	InputLinks.Empty();
	OutputLinks.Empty();
	VarLinks.Empty();
}
UBOOL FActionEntry::Refresh()
{
	guardSlow(FActionEntry::Refresh);
	return VerifyLinks();
	unguardSlow;
}
FVector2D FActionEntry::GetInputPos(FSceneNode* Frame, INT Index)
{
	guard(FActionEntry::GetInputPos);
	USActionBase* U = reinterpret_cast<USActionBase*>(Obj);
	if (VerifyLinks() || U->bEditDirty)
		Update(Frame);
	if (InputLinks.IsValidIndex(Index))
		return FVector2D(InputBase.X, InputLinks(Index).Y + (LinkHeight * 0.5f));
	return Position;
	unguard;
}
void FActionEntry::Update(FSceneNode* Frame)
{
	guard(FActionEntry::Update);
	USActionBase* U = GetAction();
	UCanvas* C = Frame->Viewport->Canvas;

	// Verify links datasize
	VerifyLinks();
	U->bEditDirty = FALSE;
	ClientAction = (U->bClientAction != 0);

	USActionBase* DefObj = reinterpret_cast<USActionBase*>(U->GetClass()->GetDefaultObject());
	Header = (U->PostInfoText.Len() ? FString::Printf(TEXT("%ls (%ls)"), *DefObj->MenuName, *U->PostInfoText) : *DefObj->MenuName);

	INT HeadXS, XS, YS, TextYS, i;
	C->CurX = 0.f;
	C->WrappedStrLenf(MedFont, XS, TextYS, TEXT("AAAW"));
	C->WrappedStrLenf(MedFont, HeadXS, YS, TEXT("%ls"), *Header);
	HeaderHeight = YS + 4.f;

	INT InputXS = 0;
	INT OutputXS = 0;
	INT VarXS = 0;
	if (InputLinks.Num())
	{
		for (i = 0; i < InputLinks.Num(); ++i)
		{
			C->WrappedStrLenf(MedFont, XS, YS, TEXT("%ls"), *U->InputLinks(i));
			InputXS = Max(InputXS, XS + 4);
		}
	}
	if (OutputLinks.Num())
	{
		for (i = 0; i < OutputLinks.Num(); ++i)
		{
			C->WrappedStrLenf(MedFont, XS, YS, TEXT("%ls"), *U->OutputLinks(i));
			OutputXS = Max(OutputXS, XS + 4);
			OutputLinks(i).TextX = XS;
		}
	}
	if (VarLinks.Num())
	{
		for (i = 0; i < VarLinks.Num(); ++i)
		{
			C->WrappedStrLenf(MedFont, XS, YS, TEXT("%ls"), *U->VarLinks(i).Name);
			VarLinks(i).X = XS;
			VarXS += (XS + 6);
		}
	}

	Size.X = Max(HeadXS, Max(VarXS, InputXS + OutputXS)) + 4.f;

	HeaderX = (Size.X - HeadXS) * 0.5f;
	HeaderHeight = TextYS + 4.f;
	ContentY = HeaderHeight + 2.f;
	ContentYS = Max(InputLinks.Num(), OutputLinks.Num()) * (TextYS + 7);

	if (InputLinks.Num())
	{
		InputBase.X = Position.X - LinkWidth;
		InputYDelta = ContentYS / (U->InputLinks.Num() + 1);
		InputBase.Y = Position.Y + ContentY + ContentYS - (InputYDelta * U->InputLinks.Num());
		FLOAT CurY = InputBase.Y - (LinkHeight * 0.5f);
		for (i = 0; i < InputLinks.Num(); ++i, CurY += InputYDelta)
		{
			FInputLink& L = InputLinks(i);
			L.Y = CurY;
			if (L.Inputs.Num())
			{
				for (INT j = 0; j < L.Inputs.Num(); ++j)
				{
					FOtherSideLink& OL = L.Inputs(j);
					FOutputLink& OS = OL.Action->OutputLinks(OL.OtherIndex);
					if (OS.Line)
					{
						// Make sure it still matches!
						OS.Line->UpdateLink(FVector2D(OL.Action->OutputBase.X + LinkWidth, OS.Y + (LinkHeight * 0.5f)), FVector2D(InputBase.X, CurY + (LinkHeight * 0.5f)));
					}
				}
			}
		}
		InputBase.Y -= (FLOAT(TextYS) * 0.5f);
	}
	if (OutputLinks.Num())
	{
		OutputBase.X = Position.X + Size.X;
		OutputYDelta = ContentYS / (U->OutputLinks.Num() + 1);
		OutputBase.Y = Position.Y + ContentY + ContentYS - (OutputYDelta * U->OutputLinks.Num());
		FLOAT CurY = OutputBase.Y - (LinkHeight * 0.5f);
		for (i = 0; i < OutputLinks.Num(); ++i, CurY += OutputYDelta)
		{
			FOutputLink& V = OutputLinks(i);
			V.TextX = OutputBase.X - V.TextX;
			V.Y = CurY;

			if (V.Output.Action)
			{
				FEventLine* Line = V.Line;
				if (!Line)
				{
					Line = new FEventLine(GetAPI());
					Line->LineColor = FPlane(0.f, 0.f, 0.f, 1.f);
					V.Line = Line;
				}
				Line->UpdateLink(FVector2D(OutputBase.X + LinkWidth, CurY + (LinkHeight * 0.5f)), V.Output.Action->GetInputPos(Frame, V.Output.OtherIndex));
			}
			else if (V.Line)
			{
				delete V.Line;
				V.Line = NULL;
			}
		}
		OutputBase.Y -= (FLOAT(TextYS) * 0.5f);
	}
	if (VarLinks.Num())
	{
		VarTextY = Position.Y + ContentY + ContentYS;
		ContentYS += TextYS;
		FLOAT CurX = 3.f;
		VarY = Position.Y + (ContentY + ContentYS + 2.f);
		for (i = 0; i < VarLinks.Num(); ++i)
		{
			FVarLink& V = VarLinks(i);
			const FLOAT TX = V.X;
			V.X = Position.X + (CurX + (TX * 0.5f)) - (VarWidth * 0.5f);
			V.TextX = CurX;
			CurX += (TX + 6.f);

			if (V.Var)
			{
				FEventLine* Line = V.Line;
				if (!Line)
				{
					Line = new FEventLine(GetAPI());
					V.Line = Line;
				}
				Line->LineColor = V.Var->GetDrawColor();
				FVector2D StartPos(V.X + (VarWidth * 0.5f), VarY + VarHeight);
				Line->UpdateLink(StartPos, V.Var->GetInputPos(Frame, StartPos), TRUE);
			}
			else if (V.Line)
			{
				delete V.Line;
				V.Line = NULL;
			}
		}
	}

	ContentYS += 2.f;
	Size.Y = ContentY + ContentYS;

	Bounds = F2DBox(Position, FVector2D(Position.X + Size.X, Position.Y + Size.Y));
	if (U->InputLinks.Num())
		Bounds.Min.X -= LinkWidth;
	if (U->OutputLinks.Num())
		Bounds.Max.X += LinkWidth;
	if (U->VarLinks.Num())
		Bounds.Max.Y += VarHeight;
	unguard;
}
void FActionEntry::Draw(FSceneNode* Frame, const FRenderArea& Area)
{
	guard(FActionEntry::Draw);
	UCanvas* C = Frame->Viewport->Canvas;
	USActionBase* U = GetAction();
	URenderDevice* RenDev = Frame->Viewport->RenDev;

	const FLOAT CurX = Area.ToScreenX(Position.X);
	FLOAT CurY = Area.ToScreenY(Position.Y);
	FLOAT SizeX = Size.X * Area.Zoom;
	FLOAT SizeY = HeaderHeight * Area.Zoom;

	if (bIsEditing)
	{
		const FLOAT HaloExtent = 16.f * Area.Zoom;
		const FLOAT HaloRes = GetAPI()->HaloTexture->USize;
		const FLOAT HalfRes = HaloRes * 0.5f;
		FTextureInfo* HaloInfo = GetAPI()->HaloTexture->GetTexture(INDEX_NONE, RenDev);
		const FLOAT RealY = (Size.Y * Area.Zoom);

		const FPlane HaloColor(0.5f, 0.5f, 0.125f, 1.f);
		RenDev->DrawTile(Frame, *HaloInfo, CurX - HaloExtent, CurY - HaloExtent, HaloExtent, HaloExtent, 0.f, 0.f, HalfRes, HalfRes, nullptr, 1.f, HaloColor, FPlane(), PF_Translucent);
		RenDev->DrawTile(Frame, *HaloInfo, CurX - HaloExtent, CurY, HaloExtent, RealY, 0.f, HalfRes, HalfRes, 0.f, nullptr, 1.f, HaloColor, FPlane(), PF_Translucent);
		RenDev->DrawTile(Frame, *HaloInfo, CurX - HaloExtent, CurY + RealY, HaloExtent, HaloExtent, 0.f, HalfRes, HalfRes, HalfRes, nullptr, 1.f, HaloColor, FPlane(), PF_Translucent);
		RenDev->DrawTile(Frame, *HaloInfo, CurX, CurY + RealY, SizeX, HaloExtent, HalfRes, HalfRes, 0.f, HalfRes, nullptr, 1.f, HaloColor, FPlane(), PF_Translucent);
		RenDev->DrawTile(Frame, *HaloInfo, CurX + SizeX, CurY + RealY, HaloExtent, HaloExtent, HalfRes, HalfRes, HalfRes, HalfRes, nullptr, 1.f, HaloColor, FPlane(), PF_Translucent);
		RenDev->DrawTile(Frame, *HaloInfo, CurX + SizeX, CurY, HaloExtent, RealY, HalfRes, HalfRes, HalfRes, 0.f, nullptr, 1.f, HaloColor, FPlane(), PF_Translucent);
		RenDev->DrawTile(Frame, *HaloInfo, CurX + SizeX, CurY - HaloExtent, HaloExtent, HaloExtent, HalfRes, 0.f, HalfRes, HalfRes, nullptr, 1.f, HaloColor, FPlane(), PF_Translucent);
		RenDev->DrawTile(Frame, *HaloInfo, CurX, CurY - HaloExtent, SizeX, HaloExtent, HalfRes, 0.f, 0.f, HalfRes, nullptr, 1.f, HaloColor, FPlane(), PF_Translucent);
	}

	RenDev->DrawTile(Frame, *WhiteInfo, CurX - 1.f, CurY - 1.f, SizeX + 2.f, SizeY + 2.f, 0.f, 0.f, 1.f, 1.f, nullptr, 1.f, Highlight ? HighlightColor : ActionColor, FPlane(0, 0, 0, 0), PF_None);
	RenDev->DrawTile(Frame, *WhiteInfo, CurX, CurY, SizeX, SizeY, 0.f, 0.f, 1.f, 1.f, nullptr, 1.f, ClientAction ? FPlane(0.25f, 0.45f, 0.25f, 1.f) : FPlane(0.45f, 0.15f, 0.35f, 1.f), FPlane(0, 0, 0, 0), PF_None);

	C->CurX = CurX + (HeaderX * Area.Zoom);
	C->CurY = CurY + (2.f * Area.Zoom);
	C->DrawColor = FColor(255, 255, 128, 255);
	C->WrappedPrintf(MedFont, FALSE, TEXT("%ls"), *Header);
	C->DrawColor = FColor(255, 255, 255, 255);

	CurY += (ContentY * Area.Zoom);
	SizeY = (ContentYS * Area.Zoom);
	RenDev->DrawTile(Frame, *WhiteInfo, CurX - 1.f, CurY - 1.f, SizeX + 2.f, SizeY + 2.f, 0.f, 0.f, 1.f, 1.f, nullptr, 1.f, Highlight ? HighlightColor : ActionColor, FPlane(0, 0, 0, 0), PF_None);
	RenDev->DrawTile(Frame, *WhiteInfo, CurX, CurY, SizeX, SizeY, 0.f, 0.f, 1.f, 1.f, nullptr, 1.f, FPlane(0.52f, 0.52f, 0.52f, 1.f), FPlane(0, 0, 0, 0), PF_None);

	const FLOAT BoxX = LinkWidth * Area.Zoom;
	const FLOAT BoxY = LinkHeight * Area.Zoom;

	if (InputLinks.Num())
	{
		const FLOAT XPos = Area.ToScreenX(InputBase.X);
		FLOAT YPos = Area.ToScreenY(InputBase.Y);
		const FLOAT YDelta = InputYDelta * Area.Zoom;
			
		for (INT i = 0; i < InputLinks.Num(); ++i, YPos += YDelta)
		{
			C->CurX = CurX + 1;
			C->CurY = YPos;
			C->WrappedPrintf(MedFont, FALSE, TEXT("%ls"), *U->InputLinks(i));
			RenDev->DrawTile(Frame, *WhiteInfo, XPos, Area.ToScreenY(InputLinks(i).Y), BoxX, BoxY, 0.f, 0.f, 1.f, 1.f, nullptr, 1.f, InputLinks(i).Highlight ? HighlightColored : FPlane(0.f, 0.f, 0.f, 1.f), FPlane(0, 0, 0, 0), PF_None);
		}
	}
	if (OutputLinks.Num())
	{
		const FLOAT XPos = Area.ToScreenX(OutputBase.X);
		FLOAT YPos = Area.ToScreenY(OutputBase.Y);
		const FLOAT YDelta = OutputYDelta * Area.Zoom;

		for (INT i = 0; i < OutputLinks.Num(); ++i, YPos += YDelta)
		{
			C->CurX = Area.ToScreenX(OutputLinks(i).TextX) - 1;
			C->CurY = YPos;
			C->WrappedPrintf(MedFont, FALSE, TEXT("%ls"), *U->OutputLinks(i));
			RenDev->DrawTile(Frame, *WhiteInfo, XPos, Area.ToScreenY(OutputLinks(i).Y), BoxX, BoxY, 0.f, 0.f, 1.f, 1.f, nullptr, 1.f, OutputLinks(i).Highlight ? HighlightColored : FPlane(0.f, 0.f, 0.f, 1.f), FPlane(0, 0, 0, 0), PF_None);
		}
	}
	if (VarLinks.Num())
	{
		const FLOAT TextYPos = Area.ToScreenY(VarTextY);
		const FLOAT VarYPos = Area.ToScreenY(VarY);
		const FLOAT VarX = VarWidth * Area.Zoom;
		const FLOAT VarY = VarHeight * Area.Zoom;

		for (INT i = 0; i < VarLinks.Num(); ++i)
		{
			C->CurX = CurX + (VarLinks(i).TextX * Area.Zoom);
			C->CurY = TextYPos;
			C->WrappedPrintf(MedFont, FALSE, TEXT("%ls"), *U->VarLinks(i).Name);

			FPlane ArrowColor(0.f, 0.f, 0.f, 1.f);
			if (VarLinks(i).Highlight)
				ArrowColor = HighlightColored;
			else if (U->VarLinks(i).ExpectedType && U->VarLinks(i).ExpectedType != USVariableBase::StaticClass())
				ArrowColor = reinterpret_cast<USVariableBase*>(U->VarLinks(i).ExpectedType->GetDefaultObject())->DrawColor.Plane();
			
			const FLOAT VarXPos = Area.ToScreenX(VarLinks(i).X);
			if(U->VarLinks(i).bInput && U->VarLinks(i).bOutput)
				RenDev->DrawTile(Frame, *WhiteInfo, VarXPos, VarYPos, VarX, VarY, 0.f, 0.f, 1.f, 1.f, nullptr, 1.f, ArrowColor, FPlane(0, 0, 0, 0), PF_None);
			else if (U->VarLinks(i).bOutput)
				RenDev->DrawTile(Frame, *ArrowTexInfo, VarXPos, VarYPos, VarX, VarY, 0.f, 0.f, 32.f, 32.f, nullptr, 1.f, ArrowColor, FPlane(0, 0, 0, 0), PF_Masked);
			else if (U->VarLinks(i).bInput)
				RenDev->DrawTile(Frame, *ArrowTexInfo, VarXPos, VarYPos, VarX, VarY, 0.f, 32.f, 32.f, -32.f, nullptr, 1.f, ArrowColor, FPlane(0, 0, 0, 0), PF_Masked);
			else RenDev->DrawTile(Frame, *WhiteInfo, VarXPos, VarYPos, VarX, VarY, 0.f, 0.f, 1.f, 1.f, nullptr, 1.f, ArrowColor, FPlane(1, 0, 0, 0), PF_None);
		}
	}

	unguard;
}

void FActionEntry::CheckMouseHit(const FVector2D& Mouse, FMouseTarget& Result)
{
	guardSlow(FActionEntry::CheckMouseHit);
	if (Mouse.X >= Position.X && Mouse.X <= (Position.X + Size.X) && Mouse.Y >= Position.Y && Mouse.Y <= (Position.Y + Size.Y))
	{
		Result.Set(this, 1);
	}
	else
	{
		USActionBase* U = GetAction();
		if (InputLinks.Num())
		{
			if (Mouse.X >= InputBase.X && Mouse.X <= (InputBase.X + LinkWidth))
			{
				for (INT i = 0; i < InputLinks.Num(); ++i)
				{
					if (Mouse.Y >= InputLinks(i).Y && Mouse.Y <= (InputLinks(i).Y + LinkHeight))
					{
						Result.Set(this, 2 + i);
						return;
					}
				}
			}
		}
		if (OutputLinks.Num())
		{
			if (Mouse.X >= OutputBase.X && Mouse.X <= (OutputBase.X + LinkWidth))
			{
				for (INT i = 0; i < OutputLinks.Num(); ++i)
				{
					if (Mouse.Y >= OutputLinks(i).Y && Mouse.Y <= (OutputLinks(i).Y + LinkHeight))
					{
						Result.Set(this, 1000 + i);
						return;
					}
				}
			}
		}
		if (VarLinks.Num())
		{
			if (Mouse.Y >= VarY && Mouse.Y <= (VarY + VarHeight))
			{
				for (INT i = 0; i < VarLinks.Num(); ++i)
				{
					if (Mouse.X >= VarLinks(i).X && Mouse.X <= (VarLinks(i).X + VarWidth))
					{
						Result.Set(this, 2000 + i);
						return;
					}
				}
			}
		}
	}
	unguardSlow;
}
void FActionEntry::GainFocus(DWORD Index)
{
	guardSlow(FActionEntry::GainFocus);
	Highlight = (Index == 1);
	if (Index == 1)
		GetAPI()->SetTooltipText(*GetAction()->Description, *GetAction()->MenuName);
	else if (Index >= 2 && Index < 1000)
	{
		const INT ItemIndex = Index - 2;
		if (InputLinks.IsValidIndex(ItemIndex))
		{
			InputLinks(ItemIndex).Highlight = TRUE;
			TArray<FOtherSideLink>& L = InputLinks(ItemIndex).Inputs;
			for (INT i = 0; i < L.Num(); ++i)
			{
				FOutputLink& OL = L(i).Action->OutputLinks(L(i).OtherIndex);
				OL.Highlight = TRUE;
				if (OL.Line)
					OL.Line->Highlight = TRUE;
			}
		}
	}
	else if (Index >= 1000 && Index < 2000)
	{
		const INT ItemIndex = Index - 1000;
		if (OutputLinks.IsValidIndex(ItemIndex))
		{
			OutputLinks(ItemIndex).Highlight = TRUE;
			if (OutputLinks(ItemIndex).Line)
				OutputLinks(ItemIndex).Line->Highlight = TRUE;
			if (OutputLinks(ItemIndex).Output.Action)
				OutputLinks(ItemIndex).Output.Action->InputLinks(OutputLinks(ItemIndex).Output.OtherIndex).Highlight = TRUE;
		}
	}
	else if (Index >= 2000)
	{
		const INT ItemIndex = Index - 2000;
		if (VarLinks.IsValidIndex(ItemIndex))
		{
			VarLinks(ItemIndex).Highlight = TRUE;
			if (VarLinks(ItemIndex).Line)
				VarLinks(ItemIndex).Line->Highlight = TRUE;
			if (VarLinks(ItemIndex).Var)
				VarLinks(ItemIndex).Var->Highlight = TRUE;
		}
	}
	unguardSlow;
}
void FActionEntry::LostFocus(DWORD Index)
{
	guardSlow(FActionEntry::LostFocus);
	Highlight = FALSE;
	if (Index >= 2 && Index < 1000)
	{
		const INT ItemIndex = Index - 2;
		if (InputLinks.IsValidIndex(ItemIndex))
		{
			InputLinks(ItemIndex).Highlight = FALSE;
			TArray<FOtherSideLink>& L = InputLinks(ItemIndex).Inputs;
			for (INT i = 0; i < L.Num(); ++i)
			{
				FOutputLink& OL = L(i).Action->OutputLinks(L(i).OtherIndex);
				OL.Highlight = FALSE;
				if (OL.Line)
					OL.Line->Highlight = FALSE;
			}
		}
	}
	else if (Index >= 1000 && Index < 2000)
	{
		const INT ItemIndex = Index - 1000;
		if (OutputLinks.IsValidIndex(ItemIndex))
		{
			OutputLinks(ItemIndex).Highlight = FALSE;
			if (OutputLinks(ItemIndex).Line)
				OutputLinks(ItemIndex).Line->Highlight = FALSE;
			if (OutputLinks(ItemIndex).Output.Action)
				OutputLinks(ItemIndex).Output.Action->InputLinks(OutputLinks(ItemIndex).Output.OtherIndex).Highlight = FALSE;
		}
	}
	else if (Index >= 2000)
	{
		const INT ItemIndex = Index - 2000;
		if (VarLinks.IsValidIndex(ItemIndex))
		{
			VarLinks(ItemIndex).Highlight = FALSE;
			if (VarLinks(ItemIndex).Line)
				VarLinks(ItemIndex).Line->Highlight = FALSE;
			if (VarLinks(ItemIndex).Var)
				VarLinks(ItemIndex).Var->Highlight = FALSE;
		}
	}
	unguardSlow;
}
void FActionEntry::OnMouseClick(DWORD Index)
{
	guard(FActionEntry::OnMouseClick);
	if (GetAPI()->MouseEventLine)
	{
		FMouseTarget PrevInfo = GetAPI()->MouseEventLine->MouseCache;
		delete GetAPI()->MouseEventLine;
		GetAPI()->SetRealTime(FALSE);

		if (PrevInfo.Entry->GetType() == ENTITY_Action)
		{
			if (Index >= 2 && Index < 1000) // This is input
			{
				if (PrevInfo.Index >= 1000 && PrevInfo.Index < 2000) // Other is output
				{
					reinterpret_cast<FActionEntry*>(PrevInfo.Entry)->LinkOutput(this, (PrevInfo.Index - 1000), (Index - 2));
					GetAPI()->RefreshSelection();
				}
			}
			else if (Index >= 1000 && Index < 2000) // This is output
			{
				if (PrevInfo.Index >= 2 && PrevInfo.Index < 1000) // Other is input
				{
					LinkOutput(reinterpret_cast<FActionEntry*>(PrevInfo.Entry), (Index - 1000), (PrevInfo.Index - 2));
					GetAPI()->RefreshSelection();
				}
			}
		}
		else if (PrevInfo.Entry->GetType() == ENTITY_Variable)
		{
			if (Index >= 2000) // This is variable
				TryLinkVariable(reinterpret_cast<FVariableEntry*>(PrevInfo.Entry), (Index - 2000));
		}
		return;
	}

	if (Index == 1)
		BrowserWindow->OpenPropertiesWindow(this);
	else if (Index >= 2 && Index < 1000)
	{
		const INT ItemIndex = Index - 2;
		if (InputLinks.IsValidIndex(ItemIndex))
		{
			new FDynEventLine(GetAPI(), FVector2D(InputBase.X, InputLinks(ItemIndex).Y + (LinkHeight * 0.5f)), FVector2D(-1.f, 0.f), FVector2D(1.f, 0.f), FDynEventLine::ELineMode::LINEMODE_Static);
			GetAPI()->SetRealTime(TRUE);
		}
	}
	else if (Index >= 1000 && Index < 2000)
	{
		const INT ItemIndex = Index - 1000;
		if (OutputLinks.IsValidIndex(ItemIndex))
		{
			new FDynEventLine(GetAPI(), FVector2D(OutputBase.X + LinkWidth, OutputLinks(ItemIndex).Y + (LinkHeight * 0.5f)), FVector2D(1.f, 0.f), FVector2D(-1.f, 0.f), FDynEventLine::ELineMode::LINEMODE_Static);
			GetAPI()->SetRealTime(TRUE);
		}
	}
	else if (Index >= 2000)
	{
		const INT ItemIndex = Index - 2000;
		if (VarLinks.IsValidIndex(ItemIndex))
		{
			FDynEventLine* L = new FDynEventLine(GetAPI(), FVector2D(VarLinks(ItemIndex).X + (VarWidth * 0.5f), VarY + VarHeight), FVector2D(0.f, 1.f), FVector2D(0.f, 0.f), FDynEventLine::ELineMode::LINEMODE_VarEnd);
			USActionBase* U = GetAction();
			if (U->VarLinks(ItemIndex).ExpectedType && U->VarLinks(ItemIndex).ExpectedType != USVariableBase::StaticClass())
				L->LineColor = reinterpret_cast<USVariableBase*>(U->VarLinks(ItemIndex).ExpectedType->GetDefaultObject())->DrawColor.Plane();
			GetAPI()->SetRealTime(TRUE);
		}
	}
	unguard;
}
void FActionEntry::DisconnectAll()
{
	guard(FActionEntry::DisconnectAll);
	INT i, j;
	for (i = 0; i < InputLinks.Num(); ++i)
	{
		TArray<FOtherSideLink>& Inputs = InputLinks(i).Inputs;
		for (j = (Inputs.Num() - 1); j >= 0; --j)
			Inputs(j).Action->UnlinkOutput(Inputs(j).OtherIndex);
	}
	for (i = 0; i < OutputLinks.Num(); ++i)
		if (OutputLinks(i).Output.Action)
			UnlinkOutput(i);
	for (i = 0; i < VarLinks.Num(); ++i)
		if (VarLinks(i).Var)
			LinkVariable(nullptr, i);
	unguard;
}
UBOOL FActionEntry::TryLinkVariable(FVariableEntry* Var, INT VarIndex)
{
	guard(FActionEntry::TryLinkVariable);
	eVarLinkStatus Result = GetAction()->VarLinks(VarIndex).CanLinkTo(Var->GetVariable());
	if (Result == VARLINK_Ok)
	{
		LinkVariable(Var, VarIndex);
		GetAPI()->RefreshSelection();
		return TRUE;
	}
	else if (Result == VARLINK_Mismatch)
		BrowserWindow->Log(NAME_Warning, TEXT("ERROR: Can't link these variables, type mismatch!"));
	else BrowserWindow->Log(NAME_Warning, TEXT("ERROR: Can't link these variables, a value can't be written to this output!"));
	return FALSE;
	unguard;
}
void FActionEntry::DragEvent(const FVector2D& MouseDelta)
{
	guard(FActionEntry::DragEvent);
	Position.X += MouseDelta.X;
	Position.Y += MouseDelta.Y;
	GetAction()->Location[0] = appRound(Position.X);
	GetAction()->Location[1] = appRound(Position.Y);
	GetAction()->bEditDirty = TRUE;

	for (INT i = 0; i < InputLinks.Num(); ++i)
	{
		for (INT j = 0; j < InputLinks(i).Inputs.Num(); ++j)
			InputLinks(i).Inputs(j).Action->GetAction()->bEditDirty = TRUE;
	}
	unguard;
}
UBOOL FActionEntry::BeginDrag(const FVector2D& Mouse)
{
	return TRUE;
}

FGroupObject::FGroupObject(FKismetAPI* A, INT Index)
	: FRenderEntry(A, RENDORDER_Groups)
	, GroupIndex(Index)
	, Highlight(FALSE)
{
	A->IndexedGroups(Index) = this;
	FGroupBoxInfo& G = A->Manager->GroupsInfo(Index);
	Position = FVector2D(G.Pos[0], G.Pos[1]);
	Size = FVector2D(G.Size[0], G.Size[1]);
	Refresh();
}
FGroupObject::~FGroupObject() noexcept(false)
{
	guard(FGroupObject::~FGroupObject);
	if (GetAPI()->IndexedGroups.IsValidIndex(GroupIndex))
		GetAPI()->IndexedGroups(GroupIndex) = nullptr;
	unguard;
}
constexpr FLOAT EdgePixels = 4.f;
void FGroupObject::Draw(FSceneNode* Frame, const FRenderArea& Area)
{
	guard(FGroupObject::Draw);
	const FVector2D Pos = Area.ToScreen(Position);
	const FVector2D Scale = Area.ToScreenScale(Size);
	UCanvas* C = Frame->Viewport->Canvas;
	C->DrawColor = GroupTextColors[ColorIndex];
	C->CurX = Pos.X;
	C->CurY = Area.ToScreenY(TextY);
	C->WrappedPrintf(UWhiteFont, FALSE, TEXT("%ls"), *Title);

	const FPlane EdgeColor = Highlight ? GroupEdgeColors[ColorIndex] : (GroupEdgeColors[ColorIndex] * FPlane(0.5f, 0.5f, 0.5f, 1.f));
	Frame->Viewport->RenDev->DrawTile(Frame, *WhiteInfo, Pos.X, Pos.Y,										Scale.X, EdgePixels, 0.f, 0.f, 1.f, 1.f, nullptr, 1.f, EdgeColor, FPlane(0, 0, 0, 0), PF_None);
	Frame->Viewport->RenDev->DrawTile(Frame, *WhiteInfo, Pos.X, Pos.Y + Scale.Y - EdgePixels,				Scale.X, EdgePixels, 0.f, 0.f, 1.f, 1.f, nullptr, 1.f, EdgeColor, FPlane(0, 0, 0, 0), PF_None);
	Frame->Viewport->RenDev->DrawTile(Frame, *WhiteInfo, Pos.X, Pos.Y + EdgePixels,							EdgePixels, Scale.Y - (EdgePixels*2.f), 0.f, 0.f, 1.f, 1.f, nullptr, 1.f, EdgeColor, FPlane(0, 0, 0, 0), PF_None);
	Frame->Viewport->RenDev->DrawTile(Frame, *WhiteInfo, Pos.X + Scale.X - EdgePixels, Pos.Y + EdgePixels,	EdgePixels, Scale.Y - (EdgePixels*2.f), 0.f, 0.f, 1.f, 1.f, nullptr, 1.f, EdgeColor, FPlane(0, 0, 0, 0), PF_None);

	Frame->Viewport->RenDev->DrawTile(Frame, *GroupTexInfo[ColorIndex], Pos.X + EdgePixels, Pos.Y + EdgePixels, Scale.X - (EdgePixels * 2.f), Scale.Y - (EdgePixels * 2.f), 0.f, 0.f, 1.f, 1.f, nullptr, 1.f, FPlane(1.f, 1.f, 1.f, 1.f), FPlane(0, 0, 0, 0), PF_Modulated);
	unguard;
}

inline INT FromHEX(const TCHAR C)
{
	if (C >= '0' && C <= '9')
		return C - '0';
	else if (C >= 'A' && C <= 'F')
		return C - 'A' + 10;
	else if (C >= 'a' && C <= 'f')
		return C - 'a' + 10;
	return 0;
}
const TCHAR* FGroupBoxInfo::GetGroupText(INT* Color) const
{
	const TCHAR* Str = *GroupInfo;
	if (GroupInfo.Len() > 3 && Str[0] == '\\' && appIsHexDigit(Str[1]))
	{
		if (Color)
		{
			*Color = FromHEX(Str[1]);
			if (*Color < 0 || *Color >= GROUPC_MAX)
				*Color = GROUPC_Neon;
		}
		return Str + 2;
	}
	if (Color)
		*Color = GROUPC_Neon;
	return Str;
}

UBOOL FGroupObject::Refresh()
{
	guard(FGroupObject::Refresh);
	Title = GetAPI()->Manager->GroupsInfo(GroupIndex).GetGroupText(&ColorIndex);
	TextY = Position.Y - 14.f;
	Bounds = F2DBox(FVector2D(Position.X, TextY), Position + Size);
	return FALSE;
	unguard;
}

constexpr FLOAT ScaleEdgeSize = 5.f;
void FGroupObject::CheckMouseHit(const FVector2D& Mouse, FMouseTarget& Result)
{
	guardSlow(FGroupObject::CheckMouseHit);
	if (Mouse.Y >= Position.Y)
	{
		DWORD Type = 0;
		if (Mouse.Y <= (Position.Y + ScaleEdgeSize))
		{
			if (Mouse.X <= (Position.X + ScaleEdgeSize))
				Type = 1;
			else if (Mouse.X >= (Position.X + Size.X - ScaleEdgeSize))
				Type = 3;
			else Type = 2;
		}
		else if (Mouse.Y >= (Position.Y + Size.Y - ScaleEdgeSize))
		{
			if (Mouse.X <= (Position.X + ScaleEdgeSize))
				Type = 6;
			else if (Mouse.X >= (Position.X + Size.X - ScaleEdgeSize))
				Type = 8;
			else Type = 7;
		}
		else if (Mouse.X <= (Position.X + ScaleEdgeSize))
			Type = 4;
		else if (Mouse.X >= (Position.X + Size.X - ScaleEdgeSize))
			Type = 5;
		Result.Set(this, Type);
	}
	unguardSlow;
}
void FGroupObject::GainFocus(DWORD Index)
{
	guardSlow(FGroupObject::GainFocus);
	if (Index > 0)
	{
		if (Index == 2 || Index == 7)
			GetAPI()->MouseCursorType = FKismetAPI::EMouseCursorType::POINTER_NS;
		else if (Index == 4 || Index == 5)
			GetAPI()->MouseCursorType = FKismetAPI::EMouseCursorType::POINTER_WE;
		else if (Index == 1 || Index == 8)
			GetAPI()->MouseCursorType = FKismetAPI::EMouseCursorType::POINTER_NWSE;
		else GetAPI()->MouseCursorType = FKismetAPI::EMouseCursorType::POINTER_NESW;
	}
	Highlight = TRUE;
	unguardSlow;
}
void FGroupObject::LostFocus(DWORD Index)
{
	guardSlow(FGroupObject::LostFocus);
	Highlight = FALSE;
	unguardSlow;
}
void FGroupObject::OnMouseClick(DWORD Index)
{
	guard(FGroupObject::OnMouseClick);
	unguard;
}
void FGroupObject::DragEvent(const FVector2D& MouseDelta)
{
	guardSlow(FGroupObject::DragEvent);
	switch (GetAPI()->MouseTarget.Index)
	{
	case 1:
		Position.X += MouseDelta.X;
		Position.Y += MouseDelta.Y;
		Size.X -= MouseDelta.X;
		Size.Y -= MouseDelta.Y;
		break;
	case 2:
		Position.Y += MouseDelta.Y;
		Size.Y -= MouseDelta.Y;
		break;
	case 3:
		Position.Y += MouseDelta.Y;
		Size.X += MouseDelta.X;
		Size.Y -= MouseDelta.Y;
		break;
	case 4:
		Position.X += MouseDelta.X;
		Size.X -= MouseDelta.X;
		break;
	case 5:
		Size.X += MouseDelta.X;
		break;
	case 6:
		Position.X += MouseDelta.X;
		Size.X -= MouseDelta.X;
		Size.Y += MouseDelta.Y;
		break;
	case 7:
		Size.Y += MouseDelta.Y;
		break;
	case 8:
		Size.X += MouseDelta.X;
		Size.Y += MouseDelta.Y;
		break;
	default:
		Position.X += MouseDelta.X;
		Position.Y += MouseDelta.Y;
		break;
	}
	Size.X = Max(Size.X, ScaleEdgeSize * 2.f);
	Size.Y = Max(Size.Y, ScaleEdgeSize * 2.f);
	FGroupBoxInfo& G = GetAPI()->Manager->GroupsInfo(GroupIndex);
	G.Pos[0] = appRound(Position.X);
	G.Pos[1] = appRound(Position.Y);
	G.Size[0] = appRound(Size.X);
	G.Size[1] = appRound(Size.Y);
	Refresh();
	unguardSlow;
}
UBOOL FGroupObject::BeginDrag(const FVector2D& Mouse)
{
	guardSlow(FGroupObject::BeginDrag);
	return TRUE;
	unguardSlow;
}

void FKismetAPI::ExitManager()
{
	guard(FKismetAPI::ExitManager);
	if (Manager)
	{
		FRenderEntry* Next;
		for (FRenderEntry* RE = RenderList; RE; RE = Next)
		{
			RE->CollectGarbage();
			Next = RE->GetNext();
			delete RE;
		}
		delete MouseEventLine;
		IndexedGroups.Empty();
		Manager = NULL;
	}
	bDragged = FALSE;
	unguard;
}
void FKismetAPI::SetManager(AScriptedAI* NewManager)
{
	guard(FKismetAPI::SetManager);
	BrowserWindow->ClosePropertiesWindow();
	if (Manager)
		ExitManager();
	Manager = NewManager;
	Repaint();
	if (!Manager)
		return;

	Descriptor.Empty();
	if (!Manager->bInit)
		Manager->Initialize();
	Manager->Modify();
	INT i;
	for (i = 0; i < Manager->Actions.Num(); ++i)
	{
		USObjectBase* O = Manager->Actions(i);
		if (!O)
		{
			Manager->Actions.Remove(i--);
			continue;
		}
		if (!O->bEditorInit)
		{
			O->bEditorInit = TRUE;
			O->Initialize();
		}
		if (O->ObjType == 1) // Action
			reinterpret_cast<USActionBase*>(O)->GetActionEntry(this);
		else if (O->ObjType == 2) // Variable
			reinterpret_cast<USVariableBase*>(O)->GetVarEntry(this);
	}
	IndexedGroups.SetSize(Manager->GroupsInfo.Num());
	for (i = 0; i < Manager->GroupsInfo.Num(); ++i)
		new FGroupObject(this, i);
	for (FRenderEntry* RE = RenderList; RE; RE = RE->GetNext())
		RE->Init();
	MouseTarget.Set(nullptr);
	unguard;
}

void FKismetAPI::DrawKismet(FSceneNode* Frame)
{
	guard(FKismetAPI::DrawKismet);

	UCanvas* C = Frame->Viewport->Canvas;
	C->Update(Frame);

	if (!bFontsInit)
	{
		bFontsInit = TRUE;
		MedFont = GetDefault<UNativeHook>()->MainFont;
		UWhiteFont = LoadObject<UFont>(NULL, TEXT("UnrealShare.WhiteFont"), NULL, LOAD_NoWarn, NULL);
		if (!UWhiteFont)
			UWhiteFont = C->MedFont;
		WhiteTex = GetDefault<ALevelInfo>()->WhiteTexture;
		ArrowTex = GetDefault<UNativeHook>()->ArrowTexture;
		SphereTex = GetDefault<UNativeHook>()->SphereTexture;
		//GroupTex = GetDefault<UNativeHook>()->GroupBGTexture;
		UClass* ConstColorClass = LoadObject<UClass>(NULL, TEXT("Fire.ConstantColor"), NULL, LOAD_NoFail, NULL);
		UProperty* ColorProp = FindField<UProperty>(ConstColorClass, TEXT("Color1"));
		for (INT i = 0; i < GROUPC_MAX; ++i)
		{
			UTexture* Tex = ConstructObject<UTexture>(ConstColorClass);
			Tex->AddToRoot();
			*reinterpret_cast<FColor*>((reinterpret_cast<BYTE*>(Tex) + ColorProp->Offset)) = GroupTexColors[i];
			GroupTex[i] = Tex;
		}
	}
	C->CurX = 0.f;
	C->CurY = 2.f;

	if (!Manager)
	{
		C->DrawColor = FColor(255, 0, 0, 255);
		C->WrappedPrintf(UWhiteFont, TRUE, TEXT("No Kismet object selected, select from menubar: Kismet objects -> New object"), Manager->GetName());
		return;
	}
	Frame->Viewport->RenDev->SetZTestMode(ZTEST_Always);
#if 0
	C->WrappedPrintf(UWhiteFont, TRUE, TEXT("In > %ls > Out"), Manager->GetName());
#else
	Manager->eventEditorDrawFrame(C);
#endif

	// Update actions.
	guard(UpdateActions);
	INT i;
	USObjectBase* O;
	FObjectEntry* OE;
	for (i = 0; i < Manager->Actions.Num(); ++i)
	{
		O = Manager->Actions(i);
		if (!O)
		{
			Manager->Actions.Remove(i--);
			continue;
		}
		if (O->ObjType == 1) // Action
			OE = reinterpret_cast<USActionBase*>(O)->GetActionEntry(this);
		else if (O->ObjType == 2) // Variable
			OE = reinterpret_cast<USVariableBase*>(O)->GetVarEntry(this);
		else OE = NULL;

		if (OE)
		{
			if (OE->Refresh())
				O->bEditDirty = TRUE;
			if (O->bEditDirty)
				OE->Update(Frame);
		}
	}
	unguard;

	guard(DrawActions);
	ScreenArea.UpdateBounds(Manager, Frame);
	WhiteInfo = WhiteTex->GetTexture(INDEX_NONE, Frame->Viewport->RenDev);
	ArrowTexInfo = ArrowTex->GetTexture(INDEX_NONE, Frame->Viewport->RenDev);
	SphereTexInfo = SphereTex->GetTexture(INDEX_NONE, Frame->Viewport->RenDev);
	for (INT i = 0; i < GROUPC_MAX; ++i)
		GroupTexInfo[i] = GroupTex[i]->GetTexture(INDEX_NONE, Frame->Viewport->RenDev);
	C->FontScale = ScreenArea.Zoom;
	C->ClipX += 2000.f;
	for (FRenderEntry* RE = RenderList; RE; RE = RE->GetNext())
	{
		if (ScreenArea.Bounds.BoundsOverlap(RE->Bounds))
			RE->Draw(Frame, ScreenArea);
	}
	C->FontScale = 1.f;
	unguard;

	if (Descriptor.Len())
	{
		guard(DrawTooltip);
		INT XS, YS;
		C->CurX = 1.f;
		C->CurY = 1.f;
		C->ClipX = Min(Frame->FX - 4.f, 300.f);
		C->WrappedStrLenf(UWhiteFont, XS, YS, TEXT("%ls"), *Descriptor);
		Frame->Viewport->RenDev->DrawTile(Frame, *WhiteInfo, 0.f, 0.f, XS + 4.f, YS + 4.f, 0.f, 0.f, 1.f, 1.f, nullptr, 1.f, FPlane(0.8f, 0.8f, 0.1f, 1.f), FPlane(0, 0, 0, 0), PF_None);
		Frame->Viewport->RenDev->DrawTile(Frame, *WhiteInfo, 0.f, 0.f, XS + 2.f, YS + 2.f, 0.f, 0.f, 1.f, 1.f, nullptr, 1.f, FPlane(0.4f, 0.4f, 0.025f, 1.f), FPlane(0, 0, 0, 0), PF_None);

		C->CurX = 1.f;
		C->CurY = 1.f;
		C->DrawColor = FColor(255, 255, 128, 255);
		C->WrappedPrintf(UWhiteFont, FALSE, TEXT("%ls"), *Descriptor);
		unguard;
	}

	C->ClipX = Frame->FX;
	Frame->Viewport->RenDev->SetZTestMode(ZTEST_LessEqual);
	unguard;
}

void FKismetAPI::TraceMouse(FLOAT X, FLOAT Y)
{
	guard(FKismetAPI::TraceMouse);
	FVector2D Mouse = ScreenArea.ToWorld(FVector2D(X, Y));
	FMouseTarget Target;

	for (FRenderEntry* RE = RenderList; RE; RE = RE->GetNext())
		if (RE->Bounds.PointOverlaps(Mouse))
			RE->CheckMouseHit(Mouse, Target);

	if (!MouseTarget.Matches(Target))
	{
		if (MouseTarget.Entry)
			MouseTarget.Entry->LostFocus(MouseTarget.Index);
		SetTooltipText(nullptr);
		MouseCursorType = POINTER_Normal;
		MouseTarget = Target;
		if (MouseTarget.Entry)
			MouseTarget.Entry->GainFocus(MouseTarget.Index);
		Repaint();
	}
	unguard;
}
void FKismetAPI::DeselectAll()
{
	guard(FKismetAPI::DeselectAll);
	if (MouseTarget.Entry)
	{
		MouseTarget.Entry->LostFocus(MouseTarget.Index);
		SetTooltipText(nullptr);
		MouseCursorType = POINTER_Normal;
		MouseTarget.Set(nullptr);
		BrowserWindow->ClosePropertiesWindow();
	}
	unguard;
}
void FKismetAPI::RefreshSelection()
{
	guard(FKismetAPI::RefreshSelection);
	if (MouseTarget.Entry)
	{
		MouseTarget.Entry->LostFocus(MouseTarget.Index);
		MouseCursorType = POINTER_Normal;
		MouseTarget.Entry->GainFocus(MouseTarget.Index);
	}
	unguard;
}

inline FString GetNameOfClass(const UObject* C)
{
	if (C->GetClass() == UClass::StaticClass() && ((UClass*)C)->IsChildOf(USObjectBase::StaticClass()))
		return ((USObjectBase*)((UClass*)C)->GetDefaultObject())->eventGetUIName(0);
	return C->GetName();
}

struct FClassesHandler
{
public:

	struct FClassGroup
	{
		TArray<FClassGroup*> SubGroups;
		TArray<UClass*> Classes;
		FString GroupName;
		UClass* GroupClass;

		FClassGroup(const FString& GName, UClass* GClass)
			: GroupName(GName), GroupClass(GClass)
		{
		}
		~FClassGroup()
		{
			for (INT i = 0; i<SubGroups.Num(); ++i)
				delete SubGroups(i);
		}
		void EmptyContents()
		{
			guard(FClassGroup::EmptyContents);
			for (INT i = 0; i < SubGroups.Num(); ++i)
				delete SubGroups(i);
			SubGroups.EmptyNoRealloc();
			Classes.Empty();
			unguard;
		}
		void AddContents(FRightClickMenu& Menu, INT& OffsetNum)
		{
			guardSlow(AddContents);
			static TCHAR PrintStr[512];
			BYTE bHasGroup = (GroupName.Len()>0);

			if (bHasGroup)
			{
				appSprintf(PrintStr, TEXT("Add [%ls]"), *GroupName);
				Menu.AddPopup(PrintStr);
			}
			for (INT i = 0; i<SubGroups.Num(); ++i)
				SubGroups(i)->AddContents(Menu, OffsetNum);
			for (INT i = 0; i<Classes.Num(); ++i)
			{
				appSprintf(PrintStr, TEXT("%ls.."), *GetNameOfClass(Classes(i)));
				Menu.AddItem(OffsetNum, PrintStr);
				++OffsetNum;
			}
			if (bHasGroup)
				Menu.EndPopup();
			unguardSlow;
		}
		UObject* GetClassIndex(INT& Index)
		{
			UObject* Result = NULL;

			for (INT i = 0; i<SubGroups.Num() && !Result; ++i)
				Result = SubGroups(i)->GetClassIndex(Index);
			if (!Result)
			{
				if (Index<Classes.Num())
					Result = Classes(Index);
				else Index -= Classes.Num();
			}
			return Result;
		}
		void SortObjects();

		void VerifyChildren()
		{
			if (SubGroups.Num())
			{
				for (INT i = 0; i < Classes.Num(); ++i)
				{
					UClass* C = Classes(i);
					for (INT j = 0; j < SubGroups.Num(); ++j)
						if (SubGroups(j)->GroupClass == C)
						{
							SubGroups(j)->Classes.AddItem(C);
							Classes.Remove(i--);
							break;
						}
				}
				for (INT i = 0; i < SubGroups.Num(); ++i)
					SubGroups(i)->VerifyChildren();
			}
		}
	};
private:
	FClassGroup *MainGroup;
	UClass* LastInitKismet{};

	void AddAction(UClass* C) // TODO - Fix this shit.
	{
		// First grab the chain of groups above.
		TArray<UClass*> Groups;
		for (UClass* T = C->GetSuperClass(); T != USObjectBase::StaticClass(); T = T->GetSuperClass())
			Groups.AddItem(T);

		// Then iterate down that chain.
		FClassGroup* Group = MainGroup;
		while (Groups.Num())
		{
			UClass* GC = Groups.Pop();
			FString G = *((USObjectBase*)GC->GetDefaultObject())->eventGetUIName(1);

			INT i;
			for (i = (Group->SubGroups.Num()-1); i>=0; --i)
				if (Group->SubGroups(i)->GroupName == G)
				{
					Group = Group->SubGroups(i);
					break;
				}
			if (i < 0)
			{
				FClassGroup* NewG = new FClassGroup(G, GC);
				Group->SubGroups.AddItem(NewG);
				Group = NewG;
			}
		}
		Group->Classes.AddItem(C);
	}
	void RefreshList(AScriptedAI* Manager)
	{
		guard(FClassesHandler::RefreshList);
		MainGroup->EmptyContents();
		for (TObjectIterator<UClass> It; It; ++It)
		{
			UClass* C = *It;
			if (!(C->ClassFlags & CLASS_Abstract))
			{
				if (C->IsChildOf(USObjectBase::StaticClass()) && !Manager->eventEditorFilterAction(C))
					AddAction(C);
			}
		}
		MainGroup->VerifyChildren();
		MainGroup->SortObjects();
		unguard;
	}
public:
	~FClassesHandler()
	{
		if (MainGroup)
			delete MainGroup;
	}
	FClassesHandler()
		: MainGroup(new FClassGroup(TEXT(""),NULL))
	{
		guardSlow(FClassesHandler::FClassesHandler);
		unguardSlow;
	}
	void ListActionClasses(FRightClickMenu& Menu, AScriptedAI* Manager)
	{
		guard(FClassesHandler::ListActionClasses);
		if (LastInitKismet != Manager->GetClass())
		{
			LastInitKismet = Manager->GetClass();
			RefreshList(Manager);
		}
		INT Offset = ID_ACTIONCLASSES;
		guard(FillActions);
		MainGroup->AddContents(Menu, Offset);
		unguard;
		unguard;
	}
	UClass* GrabClassIndex(INT Index)
	{
		guard(FClassesHandler::GrabClassIndex);
		return (UClass*)MainGroup->GetClassIndex(Index);
		unguardf((TEXT("(%i)"), Index));
	}
};

QSORT_RETURN CDECL CompareGroups(const FClassesHandler::FClassGroup** A, const FClassesHandler::FClassGroup** B)
{
	return appStricmp(*(*A)->GroupName, *(*B)->GroupName);
}
QSORT_RETURN CDECL CompareClasses(const UObject** A, const UObject** B)
{
	return appStricmp(*GetNameOfClass(*A), *GetNameOfClass(*B));
}
void FClassesHandler::FClassGroup::SortObjects()
{
	appQsort(SubGroups.GetData(), SubGroups.Num(), sizeof(FClassGroup*), (QSORT_COMPARE)CompareGroups);
	appQsort(Classes.GetData(), Classes.Num(), sizeof(UObject*), (QSORT_COMPARE)CompareClasses);

	for (INT i = 0; i<SubGroups.Num(); ++i)
		SubGroups(i)->SortObjects();
}

inline void AddCustomInfo(FRightClickMenu* RMenu, USVariableBase* V, INT Offset)
{
	TArray<FString> List;
	V->eventGetToolbar(List);
	
	if (List.Num())
	{
		for (INT i = 0; i < List.Num(); ++i)
			RMenu->AddItem(Offset + i, *List(i));
	}
}

void FKismetAPI::Click(DWORD Buttons, FLOAT X, FLOAT Y)
{
	guard(FKismetAPI::Click);
	if (!Manager)
		return;

	if (Buttons == MOUSE_Left)
	{
		TraceMouse(X, Y);
		if (MouseTarget.Entry)
			MouseTarget.Entry->OnMouseClick(MouseTarget.Index);
		else
		{
			BrowserWindow->ClosePropertiesWindow();
			if (MouseEventLine)
			{
				delete MouseEventLine;
				SetRealTime(FALSE);
			}
		}
	}
	else if (Buttons == MOUSE_Right)
	{
		if (MouseEventLine)
		{
			delete MouseEventLine;
			SetRealTime(FALSE);
		}

		Viewport->SetMouseCapture(0, 0);
		static FClassesHandler ClassHandle;
		INT Res;

		// Bring dropdown menu.
		{
			FRightClickMenu SelectMenu;

			if (MouseTarget.Entry && MouseTarget.Entry->GetType() == ENTITY_Action)
			{
				USActionBase* Action = reinterpret_cast<FActionEntry*>(MouseTarget.Entry)->GetAction();
				SelectMenu.AddItem(ID_DUPLICACTOR, TEXT("Duplicate action"));
				SelectMenu.AddItem(ID_CLIENTACTION, TEXT("Client action"), 0, Action->bClientAction != 0);
				SelectMenu.AddItem(ID_DELSELACTOR, TEXT("Remove action"));
			}
			else if (MouseTarget.Entry && MouseTarget.Entry->GetType() == ENTITY_Variable)
			{
				USVariableBase* Variable = reinterpret_cast<FVariableEntry*>(MouseTarget.Entry)->GetVariable();
				SelectMenu.AddItem(ID_DELSELACTOR, TEXT("Remove variable"));
				AddCustomInfo(&SelectMenu, Variable, ID_VARCUSTOMACTION);
			}
			else if (MouseTarget.Entry && MouseTarget.Entry->GetType() == ENTITY_Group)
			{
				ClassHandle.ListActionClasses(SelectMenu, Manager);
				SelectMenu.AddItem();
				SelectMenu.AddItem(ID_EDITGROUP, TEXT("Edit group text"));
				SelectMenu.AddItem(ID_DELETEGROUP, TEXT("Delete group"));
			}
			else
			{
				ClassHandle.ListActionClasses(SelectMenu, Manager);
			}
			SelectMenu.AddItem();
			SelectMenu.AddItem(ID_CREATEGROUP, TEXT("Add group..."));
			SelectMenu.AddItem(ID_CENTERVIEW, TEXT("Center view to random action"));
			Res = SelectMenu.OpenMenu(BrowserWindow->hWnd);
		}

		switch (Res)
		{
		case ID_CENTERVIEW:
			CenterView();
			break;
		case ID_CLIENTACTION:
			{
				USActionBase* Action = reinterpret_cast<FActionEntry*>(MouseTarget.Entry)->GetAction();
				Action->bClientAction = !Action->bClientAction;
				Action->bEditDirty = TRUE;
				Repaint();
			}
			break;
		case ID_DUPLICACTOR:
			DuplicateAction(reinterpret_cast<FActionEntry*>(MouseTarget.Entry)->GetAction());
			break;
		case ID_CREATEGROUP:
			{
				FVector2D P = ScreenArea.ToWorld(FVector2D(X, Y));
				WNewGroupDialog* G = new WNewGroupDialog(P.X, P.Y);
				G->Show(1);
			}
			break;
		case ID_EDITGROUP:
			{
				INT ix = reinterpret_cast<FGroupObject*>(MouseTarget.Entry)->GroupIndex;
				INT iColor = 0;
				const TCHAR* Str = Manager->GroupsInfo(ix).GetGroupText(&iColor);
				WEditGroupDialog* G = new WEditGroupDialog(Str, ix, iColor);
				G->Show(1);
			}
			break;
		case ID_DELETEGROUP:
			{
				INT ix = reinterpret_cast<FGroupObject*>(MouseTarget.Entry)->GroupIndex;
				DeleteGroup(ix);
			}
			break;
		case ID_DELSELACTOR:
			DeleteAction(reinterpret_cast<FObjectEntry*>(MouseTarget.Entry));
			TraceMouse(X, Y);
			break;
		default:
			{
				if (Res < ID_VARCUSTOMACTION)
				{
					int Offset = Res - ID_ACTIONCLASSES;
					if (Offset >= 0)
					{
						UClass* C = ClassHandle.GrabClassIndex(Offset);
						if (C)
						{
							FVector2D P = ScreenArea.ToWorld(FVector2D(X, Y));
							AddAction(P, C);
						}
					}
				}
				else
				{
					USVariableBase* Variable = reinterpret_cast<FVariableEntry*>(MouseTarget.Entry)->GetVariable();
					Variable->eventOnSelectToolbar(Res - ID_VARCUSTOMACTION);
				}
				break;
			}
		}
	}
	bDragged = FALSE;
	if (!MouseEventLine)
		SetRealTime(FALSE);
	unguard;
}

void FKismetAPI::DeleteFocused()
{
	guard(FKismetAPI::DeleteFocused);
	if (!Manager)
		return;

	if (MouseEventLine)
	{
		delete MouseEventLine;
		SetRealTime(FALSE);
	}
	Viewport->SetMouseCapture(0, 0);

	if (MouseTarget.Entry)
	{
		if (MouseTarget.Entry->GetType() == ENTITY_Action || MouseTarget.Entry->GetType() == ENTITY_Variable)
		{
			DeleteAction(reinterpret_cast<FObjectEntry*>(MouseTarget.Entry));
		}
		else if (MouseTarget.Entry->GetType() == ENTITY_Group)
		{
			DeleteGroup(reinterpret_cast<FGroupObject*>(MouseTarget.Entry)->GroupIndex);
		}
	}
	unguard;
}

void FKismetAPI::MousePosition(DWORD Buttons, FLOAT X, FLOAT Y)
{
	guard(FKismetAPI::MousePosition);
	if (MouseEventLine)
		MouseEventLine->MouseMove(ScreenArea.ToWorld(FVector2D(X, Y)));
	if (Buttons == 0)
	{
		bDragged = FALSE;
		TraceMouse(X, Y);
	}
	else if ((Buttons & MOUSE_Left) && MouseTarget.Entry)
	{
		if (!bDragged)
		{
			bDragged = TRUE;
			if (MouseTarget.Entry->BeginDrag(ScreenArea.ToWorld(FVector2D(X, Y))))
				SetRealTime(TRUE);
		}
		else
		{
			
			FVector2D MouseDelta = ScreenArea.ToWorldScale(FVector2D(X, Y) - MouseLastPos);
			MouseTarget.Entry->DragEvent(MouseDelta);
			if (Buttons & MOUSE_Shift)
			{
				Manager->WinPos[0] += MouseDelta.X;
				Manager->WinPos[1] += MouseDelta.Y;

				RECT rect = { 0 };
				GetWindowRect(reinterpret_cast<HWND>(Viewport->GetWindow()), &rect);
				SetCursorPos(rect.left + appRound(MouseLastPos.X), rect.top + appRound(MouseLastPos.Y));
				return;
			}
		}
		MouseLastPos = FVector2D(X, Y);
	}
	unguard;
}

void FKismetAPI::CenterView()
{
	guard(FKismetAPI::CenterView);
	if (Manager->Actions.Num())
	{
		for (INT i = 0; i < 5; ++i)
		{
			USObjectBase* O = Manager->Actions(appRandRange(0, Manager->Actions.Num()));
			if (O)
			{
				FVector2D P = ScreenArea.ToWorldScale(FVector2D(O->Location[0], O->Location[1]));
				Manager->WinPos[0] = P.X;
				Manager->WinPos[1] = P.Y;
				return;
			}
		}
	}
	Manager->WinPos[0] = 0.f;
	Manager->WinPos[1] = 0.f;
	Repaint();
	unguard;
}

FObjectEntry* FKismetAPI::AddAction(const FVector2D& Position, UClass* UC)
{
	guard(FKismetAPI::AddAction);
	USObjectBase* KObj = Manager->AddAction(UC, appRound(Position.X), appRound(Position.Y));
	if (!KObj)
	{
		BrowserWindow->Logf(TEXT("Couldn't add action %ls for an unknown reason."), UC->GetFullName());
		return nullptr;
	}
	FObjectEntry* Result = NULL;
	if (KObj->ObjType == 1) // Action
		Result = reinterpret_cast<USActionBase*>(KObj)->GetActionEntry(this);
	else if (KObj->ObjType == 2) // Variable
		Result = reinterpret_cast<USVariableBase*>(KObj)->GetVarEntry(this);
	verify(Result != NULL);
	Result->Init();
	Repaint();
	return Result;
	unguard;
}
FObjectEntry* FKismetAPI::DuplicateAction(USObjectBase* O)
{
	guard(FKismetAPI::DuplicateAction);
	FVector2D NewPos(O->Location[0] + 16.f, O->Location[1] + 16.f);
	FObjectEntry* Result = AddAction(NewPos, O->GetClass());
	if (!Result)
		return nullptr;

	USObjectBase* NewObj = NULL;
	if (Result->GetType() == ENTITY_Action)
		NewObj = reinterpret_cast<FActionEntry*>(Result)->GetAction();
	else if (Result->GetType() == ENTITY_Variable)
		NewObj = reinterpret_cast<FVariableEntry*>(Result)->GetVariable();

	BYTE* Src = reinterpret_cast<BYTE*>(O);
	BYTE* Dest = reinterpret_cast<BYTE*>(NewObj);
	for (TFieldIterator<UProperty> It(O->GetClass()); (It && It->GetOuter() != UObject::StaticClass()); ++It)
	{
		if (!(It->PropertyFlags & (CPF_Native | CPF_Transient)) && appStricmp(It->GetName(), TEXT("Location")) && appStricmp(It->GetName(), TEXT("NextBeginPlay")))
			It->CopyCompleteValue(Dest + It->Offset, Src + It->Offset, NewObj);
	}
	return Result;
	unguard;
}
void FKismetAPI::DeleteAction(FObjectEntry* Ent)
{
	guard(FKismetAPI::DeleteAction);
	BrowserWindow->NoteObjectRemoved(Ent);
	USObjectBase* O = Ent->GetObject();
	Ent->DisconnectAll();
	delete Ent;
	O->EditData = NULL;
	O->bEditorInit = FALSE;
	Manager->Actions.RemoveItem(O);
	Manager->RebuildList();
	Manager->CheckStatic();
	O->eventOnRemoved();
	Repaint();
	DeselectAll();
	unguard;
}

FGroupObject* FKismetAPI::AddNewGroup(const FString& Text, INT X, INT Y)
{
	guard(FKismetAPI::AddNewGroup);
	INT nIndex = Manager->GroupsInfo.Num();
	new(Manager->GroupsInfo)FGroupBoxInfo(Text, X, Y);
	IndexedGroups.Add();
	FGroupObject* Result = new FGroupObject(this, nIndex);
	Result->Init();
	Repaint();
	return Result;
	unguard;
}
void FKismetAPI::DeleteGroup(INT Index)
{
	guard(FKismetAPI::DeleteGroup);
	delete IndexedGroups(Index);
	for (INT i = (Index + 1); i < IndexedGroups.Num(); ++i)
		IndexedGroups(i)->GroupIndex = i - 1;

	Manager->GroupsInfo.Remove(Index);
	IndexedGroups.Remove(Index);
	Repaint();
	unguard;
}
void FKismetAPI::EditGroupText(const FString& NewText, INT Index)
{
	guard(FKismetAPI::EditGroupText);
	if (!Manager->GroupsInfo.IsValidIndex(Index))
		return;
	Manager->GroupsInfo(Index).GroupInfo = NewText;
	IndexedGroups(Index)->Refresh();
	Repaint();
	unguard;
}

void OpenMainWindow()
{
	guard(OpenMainWindow);

	if (BrowserWindow == NULL)
	{
		BrowserWindow = new WKismetBrowserWindow();
		BrowserWindow->Show(1);
	}
	else BrowserWindow->Show(1);
	BrowserWindow->UpdateSelection();
	unguard;
}
