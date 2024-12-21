/*=============================================================================
	Terrain editor.

	Revision history:
		* Created by Marco

=============================================================================*/
#pragma once

#include "UnLinker.h"
#include "UnTerrainInfo.h"

static void RefreshTerrainList();
static void RefreshListName(INT Index);
static void TryImportTerrains(const TCHAR* FileName, HWND Wnd);

static INT ObjCounter = 0;
class DlgNewTerrainWindow : public WCustomWindowBase
{
public:
	WLabel Prompt, SizeXLabel, SizeYLabel;
	WCoolButton OkButton;
	WCoolButton CancelButton;
	WEdit NameField, SizeXField, SizeYField;

	void OnClickTrue()
	{
		FString ItemName = NameField.GetText();
		if (ItemName.Len() && FindObject<AActor>(GEditor->Level->GetOuter(), *ItemName) != NULL)
		{
			appMsgf(TEXT("Can't add terrain with name '%ls', actor already excists in level."), *ItemName);
			return;
		}
		UViewport* ActiveViewport = NULL;
		UClient* C = UBitmap::__Client;
		for (INT i = 0; i < C->Viewports.Num(); ++i)
		{
			if (!C->Viewports(i) || !C->Viewports(i)->Actor)
				continue;
			if (C->Viewports(i)->Actor->RendMap >= REN_Wire && C->Viewports(i)->Actor->RendMap <= REN_PlainTex)
			{
				ActiveViewport = C->Viewports(i);
				break;
			}
			else if(!ActiveViewport)
				ActiveViewport = C->Viewports(i);
		}
		// Spawn new terrain object with desired resolution by default!
		ATerrainInfo* NewTerrain;
		{
			ATerrainInfo* DefaultObj = GetDefault<ATerrainInfo>();
			const INT DefX = DefaultObj->SizeX;
			const INT DefY = DefaultObj->SizeY;
			DefaultObj->SizeX = Clamp<INT>(appAtoi(*SizeXField.GetText()), 2, 1024);
			DefaultObj->SizeY = Clamp<INT>(appAtoi(*SizeYField.GetText()), 2, 1024);
			NewTerrain = reinterpret_cast<ATerrainInfo*>(GEditor->Level->SpawnActor(ATerrainInfo::StaticClass(), ItemName.Len() ? FName(*ItemName) : FName(NAME_None), NULL, NULL, ActiveViewport->Actor->Location, FRotator(0, 0, 0)));
			DefaultObj->SizeX = DefX;
			DefaultObj->SizeY = DefY;
		}

		GEditor->TerrainSettings.Terrain = NewTerrain;
		RefreshTerrainList();
		GEditor->RepaintAllViewports();

		_CloseWindow();
	}
	void OnClickFalse()
	{
		_CloseWindow();
	}

	DlgNewTerrainWindow(HWND inParentWin)
		: WCustomWindowBase(0)
		, Prompt(this), SizeXLabel(this), SizeYLabel(this)
		, OkButton(this, 0, FDelegate(this, (TDelegate)&DlgNewTerrainWindow::OnClickTrue))
		, CancelButton(this, 0, FDelegate(this, (TDelegate)&DlgNewTerrainWindow::OnClickFalse))
		, NameField(this), SizeXField(this), SizeYField(this)
	{
		PerformCreateWindowEx(0, TEXT("New Terrain"), (WS_OVERLAPPED | WS_CAPTION), 160, 160, 250, 150, inParentWin, NULL, NULL);

		FRect AreaRec = GetClientRect();
		INT yPos = 5;
		Prompt.OpenWindow(TRUE, 0);
		Prompt.SetText(TEXT("Name of your new terrain object:"));
		Prompt.MoveWindow(10, yPos, 230, 16, TRUE);
		yPos += 20;

		// Get an unique unused name.
		TCHAR* Str = appStaticString1024();
		while (TRUE)
		{
			appSprintf(Str, TEXT("Terrain_%i"), ObjCounter);
			if (FindObject<AActor>(GEditor->Level->GetOuter(), Str) == NULL)
				break;
			++ObjCounter;
		}
		NameField.OpenWindow(TRUE, 0, 0);
		NameField.MoveWindow(25, yPos, 200, 20, TRUE);
		NameField.SetText(Str);
		yPos += 26;

		SizeXLabel.OpenWindow(TRUE, 0);
		SizeXLabel.SetText(TEXT("X size:"));
		SizeXLabel.MoveWindow(10, yPos, 50, 16, TRUE);

		SizeXField.OpenWindow(TRUE, 0, 0);
		SizeXField.MoveWindow(60, yPos, 50, 20, TRUE);
		SizeXField.SetText(TEXT("64"));

		SizeYLabel.OpenWindow(TRUE, 0);
		SizeYLabel.SetText(TEXT("Y size:"));
		SizeYLabel.MoveWindow(120, yPos, 50, 16, TRUE);

		SizeYField.OpenWindow(TRUE, 0, 0);
		SizeYField.MoveWindow(170, yPos, 50, 20, TRUE);
		SizeYField.SetText(TEXT("64"));
		yPos += 25;

		OkButton.OpenWindow(TRUE, 80, yPos, 50, 20, TEXT("OK"));
		CancelButton.OpenWindow(TRUE, 145, yPos, 50, 20, TEXT("Cancel"));
	}
};

constexpr INT ValuesWidth = 150;
struct FTerrainValue
{
	WLabel* Label, * CurrentValue;
	WTrackBar* Track;

	FTerrainValue(WLabel* L, WLabel* C, WTrackBar* T)
		: Label(L), CurrentValue(C), Track(T)
	{}
	void Cleanup()
	{
		Label->DoDestroy();
		Label->DelayedDestroy();
		if (CurrentValue)
		{
			CurrentValue->DoDestroy();
			CurrentValue->DelayedDestroy();
		}
		if (Track)
		{
			Track->DoDestroy();
			Track->DelayedDestroy();
		}
	}
};

struct FTerrainItem : private FObjectsItem
{
	friend class WTerrainProperties;

	UStruct* MatStruct,* DecoStruct;
	ATerrainInfo* CurrentTerrain;
	INT ShowState;
	INT ShowIndex;

	void Expand()
	{
		guard(FTerrainItem::Expand);
		if (CurrentTerrain)
		{
			if (ShowState == 0)
			{
				if (!CurrentTerrain->TerrainMaterials.IsValidIndex(ShowIndex))
					return;

				for (TFieldIterator<UProperty> It(MatStruct); It; ++It)
				{
					if (AcceptFlags(It->PropertyFlags))
						Children.AddItem(new FPropertyItem(OwnerProperties, this, *It, It->GetFName(), It->Offset, -1));
				}
			}
			else if (ShowState == 1)
			{
				if (!CurrentTerrain->DecoLayers.IsValidIndex(ShowIndex))
					return;

				for (TFieldIterator<UProperty> It(DecoStruct); It; ++It)
				{
					if (AcceptFlags(It->PropertyFlags))
						Children.AddItem(new FPropertyItem(OwnerProperties, this, *It, It->GetFName(), It->Offset, -1));
				}
			}
		}
		FTreeItem::Expand();
		unguard;
	}
	BYTE* GetReadAddress(UProperty* InProperty, FTreeItem* Top, INT Index)
	{
		guard(FTerrainItem::GetReadAddress);
		if (!CurrentTerrain)
			return NULL;
		if (ShowState == 0 && CurrentTerrain->TerrainMaterials.IsValidIndex(ShowIndex))
			return reinterpret_cast<BYTE*>(&CurrentTerrain->TerrainMaterials(ShowIndex));
		else if (ShowState == 1 && CurrentTerrain->DecoLayers.IsValidIndex(ShowIndex))
			return reinterpret_cast<BYTE*>(&CurrentTerrain->DecoLayers(ShowIndex));
		return NULL;
		unguardf((TEXT("(%ls - %i)"), InProperty->GetPathName(), Index));
	}

public:
	FTerrainItem(WPropertiesBase* InOwnerProperties)
		: FObjectsItem(InOwnerProperties, NULL, CPF_Edit, TEXT(""), TRUE)
		, MatStruct(FindObjectChecked<UStruct>(ATerrainInfo::StaticClass(), TEXT("TerrainMaterial"), TRUE))
		, DecoStruct(FindObjectChecked<UStruct>(ATerrainInfo::StaticClass(), TEXT("DecorationLayer"), TRUE))
		, CurrentTerrain(NULL)
	{}
	void SetNothing()
	{
		OwnerProperties->SetItemFocus(0);
		_Objects.Empty(1);
		CurrentTerrain = NULL;
		OwnerProperties->ForceRefresh();
	}
	void OpenMaterial(ATerrainInfo* Terrain, INT MaterialIndex)
	{
		// Disable editing, to prevent crash due to edit-in-progress after empty objects list.
		OwnerProperties->SetItemFocus(0);

		CurrentTerrain = Terrain;
		_Objects.Empty(1);
		_Objects.AddItem(Terrain);
		ShowState = 0;
		ShowIndex = MaterialIndex;

		// Refresh all properties.
		OwnerProperties->ForceRefresh();
	}
	void OpenDecoration(ATerrainInfo* Terrain, INT DecoIndex)
	{
		// Disable editing, to prevent crash due to edit-in-progress after empty objects list.
		OwnerProperties->SetItemFocus(0);

		CurrentTerrain = Terrain;
		_Objects.Empty(1);
		_Objects.AddItem(Terrain);
		ShowState = 1;
		ShowIndex = DecoIndex;

		// Refresh all properties.
		OwnerProperties->ForceRefresh();
	}
	void SetProperty(UProperty* Property, FTreeItem* Offset, const TCHAR* Value, INT ExtraFlags)
	{
		guardSlow(FTerrainItem::SetProperty);
		FObjectsItem::SetProperty(Property, Offset, Value, ExtraFlags);
		if (!appStricmp(Property->GetName(), TEXT("Texture")) || !appStricmp(Property->GetName(), TEXT("Mesh")))
			RefreshListName(ShowIndex);
		unguardSlow;
	}
};
class WTerrainProperties : public WProperties
{
public:
	FTerrainItem Root;

	WTerrainProperties(WWindow* Owner)
		: WProperties(NAME_None, Owner)
		, Root(this)
	{}
	FTreeItem* GetRoot()
	{
		guard(WTerrainProperties::GetRoot);
		return &Root;
		unguard;
	}
	void OnDestroy()
	{
		guard(WTerrainProperties::OnDestroy);
		WWindow::OnDestroy();
		unguard;
	}
};

#define TERRAIN_ED_NAME TEXT("Terrain Editor")
class DlgTerrainEditor : public WCustomWindowBase
{
public:
	static DlgTerrainEditor* DlgTerrainEdit;

private:
	WLabel TerrainLabel, ToolLabel, MatLabel, DecoLabel;
	WListBox TerrainList, PaintModeList, MaterialList, DecorationList;
	ATerrainInfo* SelectedTerrain;
	TArray<ATerrainInfo*> TerrainListRef;
	WButton NewButton, ImportButton, AddMatButton, AddDecoButton;
	WTerrainProperties LocalProperties;
	TArray<FTerrainValue> Values;
	FString PrevOpenPath;

	enum EVisPage : BYTE
	{
		PAGE_None,
		PAGE_Material,
		PAGE_Decoration,
	};
	EVisPage VisiblePage;

	DlgTerrainEditor()
		: WCustomWindowBase(0)
		, TerrainLabel(this), ToolLabel(this), MatLabel(this), DecoLabel(this)
		, TerrainList(this), PaintModeList(this), MaterialList(this), DecorationList(this)
		, SelectedTerrain(NULL)
		, NewButton(this, 0, FDelegate(this, (TDelegate)&DlgTerrainEditor::OnCreateTerrain))
		, ImportButton(this, 0, FDelegate(this, (TDelegate)&DlgTerrainEditor::OnImportTerrain))
		, AddMatButton(this, 0, FDelegate(this, (TDelegate)&DlgTerrainEditor::OnAddMaterial))
		, AddDecoButton(this, 0, FDelegate(this, (TDelegate)&DlgTerrainEditor::OnAddDecoration))
		, LocalProperties(this)
		, VisiblePage(PAGE_None)
		, PrevOpenPath(appBaseDir())
	{
		PerformCreateWindowEx(WS_EX_CLIENTEDGE, TERRAIN_ED_NAME, (WS_OVERLAPPED | WS_CAPTION), 150, 150, 650, 400, GhwndEditorFrame, NULL, NULL);
		Show(1);
		InvalidateRect(hWnd, 0, TRUE);
		TerrainLabel.OpenWindow(1, 0);
		TerrainLabel.SetText(TEXT("Terrain:"));
		ToolLabel.OpenWindow(1, 0);
		ToolLabel.SetText(TEXT("Tools:"));
		MatLabel.OpenWindow(FALSE, 0);
		MatLabel.SetText(TEXT("Materials:"));
		DecoLabel.OpenWindow(FALSE, 0);
		DecoLabel.SetText(TEXT("Decorations:"));
		TerrainList.OpenWindow(TRUE, TRUE, FALSE, FALSE);
		TerrainList.SelectionChangeDelegate = FDelegate(this, (TDelegate)&DlgTerrainEditor::OnSelectTerrain);
		PaintModeList.OpenWindow(TRUE, TRUE, FALSE, FALSE);
		PaintModeList.SelectionChangeDelegate = FDelegate(this, (TDelegate)&DlgTerrainEditor::OnSelectPaintMode);
		MaterialList.OpenWindow(FALSE, TRUE, FALSE, FALSE);
		MaterialList.SelectionChangeDelegate = FDelegate(this, (TDelegate)&DlgTerrainEditor::OnSelectMaterial);
		DecorationList.OpenWindow(FALSE, TRUE, FALSE, FALSE);
		DecorationList.SelectionChangeDelegate = FDelegate(this, (TDelegate)&DlgTerrainEditor::OnSelectDecoration);
		LocalProperties.OpenChildWindow(0);
		LocalProperties.SetNotifyHook(GEditor);

		GConfig->GetString(TEXT("Directories"), TEXT("TERRAIN"), PrevOpenPath, GUEdIni);

		{
			FRect R = GetClientRect();
			INT XPos = 10;
			TerrainLabel.MoveWindow(XPos, 4, 110, 16, TRUE);
			TerrainList.MoveWindow(XPos, 22, 110, R.Height() - 46, TRUE);
			XPos += 120;
			ToolLabel.MoveWindow(XPos, 4, 100, 16, TRUE);
			PaintModeList.MoveWindow(XPos, 22, 100, R.Height() - 46, TRUE);
			XPos += 110;

			INT YPos = 16;
			AddTrackValue(XPos, YPos, TEXT("Outer Radius:"), 1, 256, 32);
			AddTrackValue(XPos, YPos, TEXT("Inner Radius:"), 1, 256, 32);
			AddTrackValue(XPos, YPos, TEXT("Strength:"), 1, 100, 10);

			YPos += 16;
			MatLabel.MoveWindow(XPos, YPos, ValuesWidth, 16, TRUE);
			DecoLabel.MoveWindow(XPos, YPos, ValuesWidth, 16, TRUE);
			YPos += 18;
			MaterialList.MoveWindow(XPos, YPos, ValuesWidth, R.Height() - YPos - 28, TRUE);
			DecorationList.MoveWindow(XPos, YPos, ValuesWidth, R.Height() - YPos - 28, TRUE);
			AddMatButton.OpenWindow(FALSE, XPos, R.Height() - 24, ValuesWidth, 20, TEXT("New Material"));
			AddDecoButton.OpenWindow(FALSE, XPos, R.Height() - 24, ValuesWidth, 20, TEXT("Add Deco layer"));

			XPos += (ValuesWidth + 10);
			LocalProperties.MoveWindow(XPos, 16, R.Width() - XPos - 8, R.Height() - 22, TRUE);

			NewButton.OpenWindow(TRUE, 4, R.Height() - 24, 76, 20, TEXT("New Terrain"));
			ImportButton.OpenWindow(TRUE, 82, R.Height() - 24, 50, 20, TEXT("Import"));
		}

		// Order must match to ETerrainPaintMode in Editor.h
		PaintModeList.AddString(TEXT("Brush paint"));
		PaintModeList.AddString(TEXT("Flatten"));
		PaintModeList.AddString(TEXT("Visibility"));
		PaintModeList.AddString(TEXT("Edge turn"));
		PaintModeList.AddString(TEXT("Material"));
		PaintModeList.AddString(TEXT("Decoration"));

		RefreshTerrains();
	}
	~DlgTerrainEditor()
	{
		MaybeDestroy();
	}
	void SetPage(EVisPage P)
	{
		if (VisiblePage == P)
			return;
		VisiblePage = P;
		MatLabel.Show(P == PAGE_Material);
		MaterialList.Show(P == PAGE_Material);
		AddMatButton.Show(P == PAGE_Material);
		DecoLabel.Show(P == PAGE_Decoration);
		DecorationList.Show(P == PAGE_Decoration);
		AddDecoButton.Show(P == PAGE_Decoration);

		switch (P)
		{
			case PAGE_Material:
			{
				if (SelectedTerrain)
					LocalProperties.Root.OpenMaterial(SelectedTerrain, INT(MaterialList.GetCurrent()));
				else LocalProperties.Root.SetNothing();
				break;
			}
			case PAGE_Decoration:
			{
				if (SelectedTerrain)
					LocalProperties.Root.OpenDecoration(SelectedTerrain, INT(DecorationList.GetCurrent()));
				else LocalProperties.Root.SetNothing();
				break;
			}
			default:
				LocalProperties.Root.SetNothing();
		}
	}
	WTrackBar* AddTrackValue(INT X, INT& Y, const TCHAR* ValueName, INT RangeMin, INT RangeMax, INT TickRate)
	{
		guardSlow(DlgTerrainEditor::AddTrackValue);
		WLabel* Label = new WLabel(this);
		Label->OpenWindow(1, 0);
		Label->SetText(ValueName);
		Label->MoveWindow(X, Y, ValuesWidth - 50, 16, TRUE);
		WLabel* CVLabel = new WLabel(this);
		CVLabel->OpenWindow(1, 0);
		CVLabel->SetText(TEXT("0 %"));
		CVLabel->MoveWindow(X + ValuesWidth - 50, Y, 50, 16, TRUE);
		Y += 16;
		WTrackBar* Track = new WTrackBar(this);
		Track->OpenWindow(TRUE);
		Track->SetTicFreq(TickRate);
		Track->SetRange(RangeMin, RangeMax);
		Track->MoveWindow(X,Y, ValuesWidth, 18, TRUE);
		Track->ThumbTrackDelegate = FDelegate(this, (TDelegate)&DlgTerrainEditor::OnValueChange);
		Track->ThumbPositionDelegate = FDelegate(this, (TDelegate)&DlgTerrainEditor::OnValueChange);
		Y += 24;
		new(Values) FTerrainValue(Label, CVLabel, Track);
		return Track;
		unguardSlow;
	}
	void DoDestroy()
	{
		guard(DlgTerrainEditor::DoDestroy);
		if (DlgTerrainEdit == this)
			DlgTerrainEdit = nullptr;
		for (INT i = 0; i < Values.Num(); ++i)
			Values(i).Cleanup();
		WCustomWindowBase::DoDestroy();
		unguard;
	}
	void OnClose()
	{
		guard(DlgTerrainEditor::OnClose);
		Show(0);
		GEditor->edcamSetMode(EM_ViewportMove);
		throw TEXT("NoRoute");
		unguard;
	}
	void UpdateCaption()
	{
		guard(DlgTerrainEditor::UpdateCaption);
		if (SelectedTerrain)
		{
			FString Caption = FString::Printf(TERRAIN_ED_NAME TEXT(" - %ls [%ix%i] selected"), SelectedTerrain->GetName(), INT(SelectedTerrain->SizeX), INT(SelectedTerrain->SizeY));
			SetText(*Caption);
		}
		else SetText(TERRAIN_ED_NAME);
		unguard;
	}
	
	void OnSelectTerrain()
	{
		guard(DlgTerrainEditor::OnSelectTerrain);
		INT Item = (INT)TerrainList.GetCurrent();
		if (!TerrainListRef.IsValidIndex(Item))
			return;
		ATerrainInfo* T = TerrainListRef(Item);
		if (T && T->IsValid())
		{
			SelectedTerrain = T;
			UpdateCaption();
			RefreshMaterials();
			OnSelectPaintMode();
			GEditor->RepaintAllViewports();
		}
		unguard;
	}
	void OnSelectPaintMode()
	{
		guard(DlgTerrainEditor::OnSelectPaintMode);
		FTerrainModeSettings Settings;
		Settings.Mode = (INT)PaintModeList.GetCurrent();
		switch (Settings.Mode)
		{
		case TPM_Material:
			SetPage(PAGE_Material);
			Settings.Index = Max<INT>((INT)MaterialList.GetCurrent(), 0);
			break;
		case TPM_Decoration:
			SetPage(PAGE_Decoration);
			Settings.Index = Max<INT>((INT)DecorationList.GetCurrent(), 0);
			break;
		default:
			SetPage(PAGE_None);
			Settings.Index = 0;
		}
		Settings.Radius = FLOAT(Values(0).Track->GetPos()) / 100.f;
		Settings.InnerRadius = FLOAT(Values(1).Track->GetPos()) / 100.f;
		Settings.Strength = FLOAT(Values(2).Track->GetPos()) / 100.f;
		Settings.Terrain = SelectedTerrain;
		GEditor->edcamSetTerrainMode(Settings);
		unguard;
	}
	void OnCreateTerrain()
	{
		guard(DlgTerrainEditor::OnCreateTerrain);
		DlgNewTerrainWindow* D = new DlgNewTerrainWindow(hWnd);
		D->Show(TRUE);
		unguard;
	}
	void OnImportTerrain()
	{
		guard(DlgTerrainEditor::OnImportTerrain);
		OPENFILENAMEA ofn;
		char File[8192] = "\0";

		ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
		ofn.lStructSize = sizeof(OPENFILENAMEA);
		ofn.hwndOwner = hWnd;
		ofn.lpstrFile = File;
		ofn.nMaxFile = sizeof(File);
		ofn.lpstrFilter = "UnrealEngine 2 levels (*.ut2,*.un2)\0*.ut2;*.un2\0All Files\0*.*\0\0";
		ofn.lpstrInitialDir = appToAnsi(*PrevOpenPath);
		ofn.lpstrDefExt = "ut2";
		ofn.lpstrTitle = "Open UnrealEngine 2 level";
		ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

		// Display the Open dialog box.
		//
		if (GetOpenFileNameA(&ofn))
		{
			FString MapFilename = appFromAnsi(File);
			PrevOpenPath = MapFilename;
			GConfig->SetString(TEXT("Directories"), TEXT("TERRAIN"), *PrevOpenPath, GUEdIni);
			TryImportTerrains(*MapFilename, hWnd);
		}
		unguard;
	}
	void OnSelectMaterial()
	{
		guard(DlgTerrainEditor::OnSelectMaterial);
		if (SelectedTerrain)
			LocalProperties.Root.OpenMaterial(SelectedTerrain, INT(MaterialList.GetCurrent()));
		else LocalProperties.Root.SetNothing();
		OnSelectPaintMode();
		unguard;
	}
	void OnSelectDecoration()
	{
		guard(DlgTerrainEditor::OnSelectDecoration);
		if (SelectedTerrain)
			LocalProperties.Root.OpenDecoration(SelectedTerrain, INT(DecorationList.GetCurrent()));
		else LocalProperties.Root.SetNothing();
		OnSelectPaintMode();
		unguard;
	}
	void OnAddMaterial()
	{
		guard(DlgTerrainEditor::OnAddMaterial);
		if (SelectedTerrain)
		{
			UTexture* CurrentTex = GEditor->CurrentTexture;
			if (!CurrentTex || !(CurrentTex->GetFlags() & RF_Public))
				CurrentTex = GetDefault<ALevelInfo>()->DefaultTexture;
			INT i = SelectedTerrain->TerrainMaterials.AddZeroed();
			FTerrainMaterial& M = SelectedTerrain->TerrainMaterials(i);
			M.PaintMap.AddZeroed(SelectedTerrain->SizeX * SelectedTerrain->SizeY);
			M.Texture = CurrentTex;
			M.UScale = 1.f;
			M.VScale = 1.f;
			RefreshMaterials();
			MaterialList.SetCurrent(i, TRUE);
			OnSelectMaterial();
		}
		unguard;
	}
	void OnAddDecoration()
	{
		guard(DlgTerrainEditor::OnAddDecoration);
		if (SelectedTerrain)
		{
			UTexture* CurrentTex = GEditor->CurrentTexture;
			INT i = SelectedTerrain->DecoLayers.AddZeroed();
			FDecorationLayer& D = SelectedTerrain->DecoLayers(i);
			D.PaintMap.AddZeroed((SelectedTerrain->SizeX - 1) * (SelectedTerrain->SizeY - 1));
			D.ScaleMultiplier[0] = 0.9f;
			D.ScaleMultiplier[1] = 1.1f;
			D.VisibilityRadius = 1600.f;
			D.Density = 1.f;
			D.HeightOffset = 16.f;
			D.Seed = appRand() & 0xFFFF;
			if (GEditor->CurrentMesh)
				D.Mesh.AddItem(GEditor->CurrentMesh);
			RefreshMaterials();
			DecorationList.SetCurrent(i, TRUE);
			OnSelectDecoration();
		}
		unguard;
	}
	void OnValueChange()
	{
		SyncValues(FALSE);
	}
	void SyncValues(UBOOL bRead)
	{
		if (bRead)
		{
			const FTerrainModeSettings& S = GEditor->TerrainSettings;
			Values(0).Track->SetPos(Clamp<INT>(appRound(S.Radius * 100.f), 1, 256));
			Values(1).Track->SetPos(Clamp<INT>(appRound(S.InnerRadius * 100.f), 1, 256));
			Values(2).Track->SetPos(Clamp<INT>(appRound(S.Strength * 100.f), 1, 100));
		}
		else OnSelectPaintMode();
		TCHAR* Str = appStaticString1024();
		appSprintf(Str, TEXT("(%i)"), (INT)Values(0).Track->GetPos());
		Values(0).CurrentValue->SetText(Str);
		appSprintf(Str, TEXT("(%i)"), (INT)Values(1).Track->GetPos());
		Values(1).CurrentValue->SetText(Str);
		appSprintf(Str, TEXT("(%i %%)"), (INT)Values(2).Track->GetPos());
		Values(2).CurrentValue->SetText(Str);
	}
	void RefreshMaterials(UBOOL bKeepSelection = FALSE)
	{
		guard(DlgTerrainEditor::RefreshMaterials);
		MaterialList.Empty();
		DecorationList.Empty();
		if (SelectedTerrain)
		{
			INT i;
			for (i = 0; i < SelectedTerrain->TerrainMaterials.Num(); ++i)
			{
				FString Desc = FString::Printf(TEXT("[%i] %ls"), i, SelectedTerrain->TerrainMaterials(i).Texture->GetPathName());
				MaterialList.AddString(*Desc);
			}
			for (i = 0; i < SelectedTerrain->DecoLayers.Num(); ++i)
			{
				FString Desc = FString::Printf(TEXT("[%i] %ls"), i, SelectedTerrain->DecoLayers(i).Mesh.Num() ? SelectedTerrain->DecoLayers(i).Mesh(0)->GetPathName() : TEXT("None"));
				DecorationList.AddString(*Desc);
			}
		}
		if (bKeepSelection)
		{
			switch (VisiblePage)
			{
			case PAGE_Material:
				MaterialList.SetCurrent(GEditor->TerrainSettings.Index, TRUE);
				break;
			case PAGE_Decoration:
				DecorationList.SetCurrent(GEditor->TerrainSettings.Index, TRUE);
				break;
			default:
				break;
			}
		}
		else LocalProperties.Root.SetNothing();
		unguard;
	}
public:
	static void stSetVisible(UBOOL bShow)
	{
		guard(DlgTerrainEditor::stSetVisible);
		if (bShow)
		{
			if (!DlgTerrainEdit)
				DlgTerrainEdit = new DlgTerrainEditor();
			else
			{
				DlgTerrainEdit->Show(TRUE);
				DlgTerrainEdit->RefreshTerrains();
			}
		}
		else if (DlgTerrainEdit && DlgTerrainEdit->m_bShow)
			DlgTerrainEdit->Show(FALSE);
		unguard;
	}
	void RefreshTerrains()
	{
		guard(DlgTerrainEditor::RefreshTerrains);
		TerrainList.Empty();
		TerrainListRef.EmptyNoRealloc();
		INT SelItem = INDEX_NONE;
		for (TActorIterator<ATerrainInfo> It(GEditor->Level); It; ++It)
		{
			TerrainListRef.AddItem(*It);
			FString ListName(FString::Printf(TEXT("%ls [%ix%i]"), It->GetName(), INT(It->SizeX), INT(It->SizeY)));
			TerrainList.AddString(*ListName);
			if (GEditor->TerrainSettings.Terrain == *It)
				SelItem = TerrainListRef.Num() - 1;
		}
		if (SelItem != INDEX_NONE)
		{
			TerrainList.SetCurrent(SelItem, TRUE);
			SelectedTerrain = TerrainListRef(SelItem);
		}
		else
		{
			SelectedTerrain = NULL;
			GEditor->TerrainSettings.Terrain = NULL; // Make sure not to reference deleted terrain.
		}

		PaintModeList.SetCurrent(GEditor->TerrainSettings.Mode, TRUE);
		UpdateCaption();
		RefreshMaterials();
		SyncValues(TRUE);
		OnSelectPaintMode();
		unguard;
	}
	void RefreshListName(INT Index)
	{
		RefreshMaterials(TRUE);
	}
};
DlgTerrainEditor* DlgTerrainEditor::DlgTerrainEdit = NULL;

static void RefreshTerrainList()
{
	if(DlgTerrainEditor::DlgTerrainEdit && DlgTerrainEditor::DlgTerrainEdit->m_bShow)
		DlgTerrainEditor::DlgTerrainEdit->RefreshTerrains();
}
static void RefreshListName(INT Index)
{
	if (DlgTerrainEditor::DlgTerrainEdit)
		DlgTerrainEditor::DlgTerrainEdit->RefreshListName(Index);
}

struct FUE2PropertyTag
{
	// Variables.
	BYTE	Type;		// Type of property, 0=end.
	BYTE	Info;		// Packed info byte.
	FName	Name;		// Name of property.
	FName	ItemName;
	INT		Size;       // Property size.
	INT		ArrayIndex;	// Index if an array; else 0.

	// Constructors.
	FUE2PropertyTag()
		: Name(NAME_None), ItemName(NAME_None), Size(0)
	{}

	// Serializer.
	friend FArchive& operator<<(FArchive& Ar, FUE2PropertyTag& Tag)
	{
		// Name.
		Ar << Tag.Name;
		if (Tag.Name == NAME_None)
			return Ar;

		BYTE SizeByte;
		_WORD SizeWord;
		INT SizeInt;

		// Packed info byte:
		// Bit 0..3 = raw type.
		// Bit 4..6 = serialized size: [1 2 4 12 16 byte word int].
		// Bit 7    = array flag.
		Ar << Tag.Info;
		Tag.Type = Tag.Info & 0x0f;
		if (Tag.Type == NAME_StructProperty)
			Ar << Tag.ItemName;
		switch (Tag.Info & 0x70)
		{
		case 0x00:
			Tag.Size = 1;
			break;
		case 0x10:
			Tag.Size = 2;
			break;
		case 0x20:
			Tag.Size = 4;
			break;
		case 0x30:
			Tag.Size = 12;
			break;
		case 0x40:
			Tag.Size = 16;
			break;
		case 0x50:
			SizeByte = Tag.Size;
			Ar << SizeByte;
			Tag.Size = SizeByte;
			break;
		case 0x60:
			SizeWord = Tag.Size;
			Ar << SizeWord;
			Tag.Size = SizeWord;
			break;
		case 0x70:
			SizeInt = Tag.Size;
			Ar << SizeInt;
			Tag.Size = SizeInt;
			break;
		}
		if ((Tag.Info & 0x80) && Tag.Type != NAME_BoolProperty)
		{
			BYTE B
				= (Tag.ArrayIndex <= 127) ? (Tag.ArrayIndex)
				: (Tag.ArrayIndex <= 16383) ? (Tag.ArrayIndex >> 8) + 0x80
				: (Tag.ArrayIndex >> 24) + 0xC0;
			Ar << B;
			if ((B & 0x80) == 0)
			{
				Tag.ArrayIndex = B;
			}
			else if ((B & 0xC0) == 0x80)
			{
				BYTE C = Tag.ArrayIndex & 255;
				Ar << C;
				Tag.ArrayIndex = ((INT)(B & 0x7F) << 8) + ((INT)C);
			}
			else
			{
				BYTE C = Tag.ArrayIndex >> 16;
				BYTE D = Tag.ArrayIndex >> 8;
				BYTE E = Tag.ArrayIndex;
				Ar << C << D << E;
				Tag.ArrayIndex = ((INT)(B & 0x3F) << 24) + ((INT)C << 16) + ((INT)D << 8) + ((INT)E);
			}
		}
		else Tag.ArrayIndex = 0;
		return Ar;
	}
};

static UBOOL bForceLoadObject = FALSE;
#define SERIALIZE_UE2OBJ(var) Ar << *reinterpret_cast<UObject**>(&var)

struct FUE2Object
{
protected:
	FName ClassName, ObjectName;
	DWORD ObjectFlags;

public:
	FUE2Object(FName ClName, FObjectExport& Exp)
		: ClassName(ClName), ObjectName(Exp.ObjectName), ObjectFlags(Exp.ObjectFlags)
	{}
	FUE2Object(FObjectImport& Imp)
		: ClassName(Imp.ClassName), ObjectName(Imp.ObjectName), ObjectFlags(0)
	{}
	virtual void Serialize(FArchive& Ar)
	{
		// Serialize tagged properties.
		if (ClassName != NAME_Class)
		{
			if (ObjectFlags & RF_HasStack)
			{
				// Variables.
				UObject* Node;
				UObject* StateNode;
				QWORD   ProbeMask;
				INT     LatentAction;

				Ar << Node << StateNode;
				Ar << ProbeMask;
				Ar << LatentAction;
				if (Node)
				{
					INT Offset = INDEX_NONE;
					Ar << AR_INDEX(Offset);
				}
			}
			//debugf(TEXT("SerProps %ls"), *ObjectName);
			while (1)
			{
				FUE2PropertyTag Tag;
				Ar << Tag;
				if (Tag.Name == NAME_None)
				{
					//debugf(TEXT("End of properties."));
					break;
				}
				//debugf(TEXT("Found property %ls, size %i."), *Tag.Name, Tag.Size);
				const INT StartPos = Ar.Tell();
				const INT PropEnd = StartPos + Tag.Size;
				if (LoadProperty(Tag, Ar))
				{
					checkf(Ar.Tell() == PropEnd, TEXT("Serialized property %ls size mismatch (got %i, should been %i)"), *Tag.Name, (Ar.Tell() - StartPos), Tag.Size);
				}
				else Ar.Seek(PropEnd);
			}
		}
	}
	virtual UBOOL LoadProperty(FUE2PropertyTag& PT, FArchive& Ar)
	{
		return FALSE;
	}
	virtual void ExportData(const TCHAR* PckName)
	{}

	inline const TCHAR* GetName() const
	{
		return *ObjectName;
	}
	inline FName GetClass() const
	{
		return ClassName;
	}
	inline DWORD GetFlags() const
	{
		return ObjectFlags;
	}
};
struct FUE2TerrainNormalPair
{
	FVector Normal1;
	FVector Normal2;

	friend FArchive& operator<<(FArchive& Ar, FUE2TerrainNormalPair& N)
	{
		return Ar << N.Normal1 << N.Normal2;
	}
};
struct FUE2Mipmap
{
public:
	TArray<BYTE> DataArray; // Data.
	INT USize, VSize;
	BYTE UBits, VBits;
	FUE2Mipmap()
	{}
	friend FArchive& operator<<(FArchive& Ar, FUE2Mipmap& M)
	{
		guard(FUE2Mipmap << );
		// Lazy array hack.
		INT SeekPos;
		if (Ar.Ver() <= 61)
			Ar << AR_INDEX(SeekPos);
		else Ar << SeekPos;
		Ar << M.DataArray;
		Ar << M.USize << M.VSize << M.UBits << M.VBits;
		return Ar;
		unguard;
	}
};

struct FUE2Bitmap : public FUE2Object
{
	BYTE Format;
	INT USize, VSize;
	TArray<FUE2Mipmap> Mips;

	FUE2Bitmap(FObjectExport& Exp)
		: FUE2Object(TEXT("Texture"), Exp)
	{}
	void Serialize(FArchive& Ar)
	{
		guard(FUE2Bitmap::Serialize);
		FUE2Object::Serialize(Ar);
		if (Ar.Ver() == 110) // U2
		{
			BYTE Temp;
			Ar << Temp;
		}
		Ar << Mips;
		unguard;
	}
	UBOOL LoadProperty(FUE2PropertyTag& PT, FArchive& Ar)
	{
		if (PT.Name == FName(TEXT("Format")))
		{
			Ar << Format;
			return TRUE;
		}
		else if (PT.Name == FName(TEXT("USize")))
		{
			Ar << USize;
			return TRUE;
		}
		else if (PT.Name == FName(TEXT("VSize")))
		{
			Ar << VSize;
			return TRUE;
		}
		return FALSE;
	}
	inline _WORD GetHeightmap(const INT x, const INT y) const
	{
		_WORD Result = (127 << 8);

		switch (Format)
		{
		case 10: // TEXF_G16:
		{
			Result = *(_WORD*)(&Mips(0).DataArray((x + y * USize) * 2));
			Result = INTEL_ORDER16(Result);
			break;
		}
		default: // TEXF_P8:
			Result = ((_WORD)Mips(0).DataArray(x + y * USize)) << 8;
		}
		return Result;
	}
	inline BYTE GetLayerWeight(const INT x, const INT y) const
	{
		switch (Format)
		{
		case 5: // TEXF_RGBA8:
		{
			FColor* C = (FColor*)&Mips(0).DataArray(0);
			return C[x + y * USize].A;
		}
		default:
			return 0;
		}
	}
};
struct FUE2TerrainInfo : public FUE2Object
{
	struct FUE2TerrainLayer
	{
		FUE2Bitmap* Texture;
		FUE2Bitmap* AlphaMap;
		FLOAT UScale;
		FLOAT VScale;
		FLOAT UPan;
		FLOAT VPan;
		BYTE TextureMapAxis;
		FLOAT TextureRotation;
		FRotator LayerRotation;
		FMatrix TerrainMatrix;
		FLOAT KFriction;
		FLOAT KRestitution;
		FUE2Bitmap* LayerWeightMap;

		friend FArchive& operator<<(FArchive& Ar, FUE2TerrainLayer& L)
		{
			guard(FUE2TerrainLayer << );
			bForceLoadObject = TRUE;
			SERIALIZE_UE2OBJ(L.Texture);
			SERIALIZE_UE2OBJ(L.AlphaMap);
			Ar << L.UScale << L.VScale << L.UPan << L.VPan << L.TextureMapAxis << L.TextureRotation << L.LayerRotation << L.TerrainMatrix.XPlane << L.TerrainMatrix.YPlane << L.TerrainMatrix.ZPlane << L.TerrainMatrix.WPlane;
			if (Ar.Ver() != 110)
			{
				Ar << L.KFriction << L.KRestitution;
			}
			return Ar;
			unguard;
		}
	};

	FVector TerrainScale, Location;
	TArray<UObject*> Sectors;
	TArray<FVector> Vertices;
	INT	HeightmapX;
	INT	HeightmapY;
	INT SectorsX;
	INT SectorsY;
	TArray<FUE2TerrainNormalPair> FaceNormals;
	FCoords ToWorld;
	FCoords ToHeightmap;
	TArray<_WORD> OldHeightmap;
	TArray<DWORD> QuadVisibilityBitmap;
	TArray<DWORD> EdgeTurnBitmap;
	TArray<FColor> VertexColors;
	FUE2Bitmap* TerrainMap;
	TArray<FUE2TerrainLayer> Layers;

	FUE2TerrainInfo(FObjectExport& Exp)
		: FUE2Object(TEXT("TerrainInfo"), Exp)
	{
		TerrainScale = FVector(64.f, 64.f, 64.f);
		Location = FVector(0, 0, 0);
	}
	void Serialize(FArchive& Ar)
	{
		guard(FUE2TerrainInfo::Serialize);
		FUE2Object::Serialize(Ar);

		if (Ar.Ver() == 110) // U2
		{
			INT Temp;
			Ar << Temp;
		}
		Ar << Sectors
			<< Vertices
			<< SectorsX << SectorsY
			<< FaceNormals;

		if (Ar.Ver() < 83)
		{
			TArray<FPlane> TempLighting;
			Ar << TempLighting;
		}

		Ar << ToWorld << ToHeightmap;
		if (Ar.Ver() < 76)
			Ar << OldHeightmap;
		Ar << HeightmapX << HeightmapY;

		if (Ar.Ver() > 74 && Ar.Ver() < 82)
		{
			for (INT x = 0; x < 16; x++)
			{
				INT Dummy;
				FString DummyString;
				Ar << Dummy << Dummy << Dummy << Dummy << Dummy << Dummy << Dummy << DummyString;
			}
		}
		if (Ar.Ver() == 86 || Ar.Ver() == 87)
			Ar << QuadVisibilityBitmap;
		unguard;
	}
	UBOOL LoadProperty(FUE2PropertyTag& PT, FArchive& Ar)
	{
		if (PT.Name == FName(TEXT("TerrainScale")))
		{
			Ar << TerrainScale;
			return TRUE;
		}
		else if (PT.Name == FName(TEXT("QuadVisibilityBitmap")))
		{
			Ar << QuadVisibilityBitmap;
			return TRUE;
		}
		else if (PT.Name == FName(TEXT("EdgeTurnBitmap")))
		{
			Ar << EdgeTurnBitmap;
			return TRUE;
		}
		else if (PT.Name == FName(TEXT("Location")))
		{
			Ar << Location;
			return TRUE;
		}
		else if (PT.Name == FName(TEXT("TerrainMap")))
		{
			SERIALIZE_UE2OBJ(TerrainMap);
			return TRUE;
		}
		else if (PT.Name == FName(TEXT("Layers")))
		{
			if (Layers.Num() <= PT.ArrayIndex)
				Layers.AddZeroed(PT.ArrayIndex + 1 - Layers.Num());
			FUE2TerrainLayer& TL = Layers(PT.ArrayIndex);
			if (Ar.LicenseeVer() < 0x1C || Ar.Ver() == 110) // Oldver or U2
				Ar << TL;
			else
			{
				while (1)
				{
					FUE2PropertyTag Tag;
					Ar << Tag;
					if (Tag.Name == NAME_None)
						break;

					const INT StartPos = Ar.Tell();
					const INT EndPos = StartPos + Tag.Size;
					if (Tag.Name == FName(TEXT("Texture")))
					{
						bForceLoadObject = TRUE;
						SERIALIZE_UE2OBJ(TL.Texture);
					}
					else if (Tag.Name == FName(TEXT("AlphaMap")))
						SERIALIZE_UE2OBJ(TL.AlphaMap);
					else if (Tag.Name == FName(TEXT("UScale")))
						Ar << TL.UScale;
					else if (Tag.Name == FName(TEXT("VScale")))
						Ar << TL.VScale;
					else if (Tag.Name == FName(TEXT("UPan")))
						Ar << TL.UPan;
					else if (Tag.Name == FName(TEXT("VPan")))
						Ar << TL.VPan;
					else if (Tag.Name == FName(TEXT("TextureMapAxis")))
						Ar << TL.TextureMapAxis;
					else if (Tag.Name == FName(TEXT("LayerRotation")))
						Ar << TL.LayerRotation;
					Ar.Seek(EndPos);
				}
			}
			debugf(TEXT("LoadLayer %i Texture %ls AlphaMap %ls"), PT.ArrayIndex, TL.Texture ? TL.Texture->GetName() : TEXT("None"), TL.AlphaMap ? TL.AlphaMap->GetName() : TEXT("None"));
			return TRUE;
		}
		else if (PT.Name == FName(TEXT("DecoLayers")) && Ar.Ver() == 127 && Ar.LicenseeVer() == 29) // Some UT2004 maps has this bug where actual size is 1 byte bigger than what was tagged.
		{
			INT Count = 0;
			Ar << AR_INDEX(Count);
			for (INT i = 0; i < Count; ++i)
			{
				while (1)
				{
					FUE2PropertyTag Tag;
					Ar << Tag;
					if (Tag.Name == NAME_None)
						break;
					Ar.Seek(Ar.Tell() + Tag.Size);
				}
			}
		}
		return FALSE;
	}
	inline UBOOL GetQuadVisibilityBitmap(INT x, INT y) const
	{
		const INT BitIndex = x + y * HeightmapX;
		const INT BitMapIndex = (BitIndex >> 5);
		if (QuadVisibilityBitmap.Num() <= BitMapIndex)
			return TRUE;
		return (QuadVisibilityBitmap(BitMapIndex) & (1 << (BitIndex & 0x1f))) ? TRUE : FALSE;
	}
	inline UBOOL GetEdgeTurnBitmap(INT x, INT y) const
	{
		const INT BitIndex = x + y * HeightmapX;
		const INT BitMapIndex = (BitIndex >> 5);
		if (EdgeTurnBitmap.Num() <= BitMapIndex)
			return FALSE;
		return (EdgeTurnBitmap(BitMapIndex) & (1 << (BitIndex & 0x1f))) ? TRUE : FALSE;
	}

	void ImportObject(ATerrainInfo* Terrain) const
	{
		guard(FUE2TerrainInfo::ImportObject);
		if (HeightmapX < 2 || HeightmapX>1024 || HeightmapY < 2 || HeightmapY>1024)
		{
			appMsgf(TEXT("Failed to import %ls, terrain dimensions out of valid range!"), GetName());
			return;
		}
		debugf(TEXT("Import terrain %ls %ix%i (dim %f,%f,%f)"), GetName(), HeightmapX, HeightmapY, TerrainScale.X, TerrainScale.Y, TerrainScale.Z);
		const INT TotalSize = HeightmapX * HeightmapY;
		UTerrainMesh* TM = Terrain->TerrainPrimitive;
		Terrain->SizeX = HeightmapX;
		Terrain->SizeY = HeightmapY;
		Terrain->TerrainScale = TerrainScale;
		Terrain->TerrainScale.Z = Terrain->TerrainScale.Z * 256.f;
		const UBOOL FlipHeightMap = (Terrain->TerrainScale.Z<0.f);
		if (FlipHeightMap)
			Terrain->TerrainScale.Z = -Terrain->TerrainScale.Z;

		_WORD* Heightmap = reinterpret_cast<_WORD*>(&Terrain->TerrainData.HeightMap(0));
		INT x, y, i;
		for (i = 0, y = 0; y < HeightmapY; ++y)
		{
			for (x = 0; x < HeightmapX; ++x)
			{
				Heightmap[i] = TerrainMap ? TerrainMap->GetHeightmap(x, y) : (127 << 8);
				++i;
			}
		}
		if (FlipHeightMap)
		{
			for (i = 0; i < TotalSize; ++i)
				Heightmap[i] = MAXWORD - Heightmap[i];
		}

		const INT FinalX = HeightmapX - 1;
		const INT FinalY = HeightmapY - 1;
		for (i = 0, y = 0; y < FinalY; ++y)
		{
			for (x = 0; x < FinalX; ++x)
			{
				if (!GetQuadVisibilityBitmap(x, y))
					TM->SetSurfHidden(i, TRUE);
				if (!GetEdgeTurnBitmap(x, y))
					TM->SetEdgeTurn(i, TRUE);
				++i;
			}
		}

		Terrain->TerrainMaterials.Empty();
		INT j = 0;
		for (i = 0; i < Layers.Num(); ++i)
		{
			const FUE2TerrainLayer& TL = Layers(i);
			if (TL.Texture && TL.AlphaMap)
			{
				GLog->Logf(TEXT("Layer %i - %ls %ls"), Terrain->TerrainMaterials.Num(), *TL.Texture->GetClass(), TL.Texture->GetName());
				j = Terrain->TerrainMaterials.AddZeroed();
				FTerrainMaterial& M = Terrain->TerrainMaterials(j);
				M.LayerRotation = TL.LayerRotation;
				M.Texture = FindObject<UTexture>(ANY_PACKAGE, TL.Texture->GetName());
				if (!M.Texture)
					M.Texture = GetDefault<ALevelInfo>()->DefaultTexture;
				M.TextureMapAxis = TL.TextureMapAxis;
				M.UPan = TL.UPan;
				M.VPan = TL.VPan;
				M.UScale = TL.UScale;
				M.VScale = TL.VScale;
				M.PaintMap.SetSize(TotalSize);
				BYTE* pnt = &M.PaintMap(0);

				for (y = 0; y < HeightmapY; ++y)
				{
					for (x = 0; x < HeightmapX; ++x)
					{
						*pnt = TL.AlphaMap->GetLayerWeight(x, y);
						++pnt;
					}
				}
			}
		}

		unguard;
	}
};

class UE2LinkerLoad : public FArchive
{
public:
	FPackageFileSummary Summary;
	TArray<FName> NameMap;
	TArray<FObjectImport> ImportMap;
	TArray<FObjectExport> ExportMap;

	TArray<FUE2Object*> ExportObjects, ImportObjects;
	TArray<FUE2TerrainInfo*> Terrains;

	FArchive* FileAr;
	UBOOL bSucceeded;

	UE2LinkerLoad(FArchive* Ar)
		: FileAr(Ar)
		, bSucceeded(FALSE)
	{
		guard(UE2LinkerLoad::UE2LinkerLoad);
		ArMaxSerializeSize = 1000;
		ArIsLoading = 1;
		ArIsSaving = 0;
		ArVer = 128;
		ArLicenseeVer = 0x1D;

		*FileAr << Summary;

		if (FileAr->IsError() || Summary.Generations.Num() == 0)
		{
			appMsgf(TEXT("Invalid file header!"));
			return;
		}

		ArVer = Summary.GetFileVersion();
		ArLicenseeVer = Summary.GetFileVersionLicensee();

		guard(LoadNames);
		// Load and map names.
		if (Summary.NameCount > 0)
		{
			FileAr->Seek(Summary.NameOffset);
			for (INT i = 0; i < Summary.NameCount; i++)
			{
				// Read the name entry from the file.
				FNameEntry NameEntry;
				*FileAr << NameEntry;
				NameMap.AddItem(FName(NameEntry.Name, FNAME_Add));
			}
		}
		unguard;

		guard(LoadImports);
		// Load import map.
		if (Summary.ImportCount > 0)
		{
			ImportObjects.Empty(Summary.ImportCount);
			ImportObjects.AddZeroed(Summary.ImportCount);

			FileAr->Seek(Summary.ImportOffset);
			for (INT i = 0; i < Summary.ImportCount; i++)
			{
				*this << *new(ImportMap)FObjectImport;
			}
		}
		unguard;

		guard(LoadExports);
		// Load export map.
		if (Summary.ExportCount > 0)
		{
			ExportObjects.Empty(Summary.ExportCount);
			ExportObjects.AddZeroed(Summary.ExportCount);

			FileAr->Seek(Summary.ExportOffset);
			for (INT i = 0; i < Summary.ExportCount; i++)
			{
				*this << *new(ExportMap)FObjectExport;
			}
		}
		unguard;

		guard(LoadTerrains);
		// Verify map HAS terrains.
		FName TerrainName(TEXT("TerrainInfo"));
		for (INT i = 0; i < Summary.ExportCount; i++)
		{
			if (GetExportClassName(i) == TerrainName)
			{
				FUE2TerrainInfo* T = new FUE2TerrainInfo(ExportMap(i));
				FileAr->Seek(ExportMap(i).SerialOffset);
				T->Serialize(*this);
				Terrains.AddItem(T);
				ExportObjects(i) = T;
				bSucceeded = TRUE;
			}
		}
		unguard;
		if (!bSucceeded)
			appMsgf(TEXT("Error: Level has no terrain objects!"));
		unguard;
	}
	~UE2LinkerLoad()
	{
		delete FileAr;
		INT i;
		for (i = 0; i < ExportObjects.Num(); ++i)
			delete ExportObjects(i);
		for (i = 0; i < ImportObjects.Num(); ++i)
			delete ImportObjects(i);
	}

	void Serialize(void* V, INT Length)
	{
		FileAr->Serialize(V, Length);
	}
	INT MapName(FName* Name)
	{
		return 0;
	}
	INT MapObject(UObject* Object)
	{
		return 0;
	}
	INT Tell()
	{
		return FileAr->Tell();
	}
	INT TotalSize()
	{
		return FileAr->TotalSize();
	}
	FArchive& operator<<(class FName& Name)
	{
		NAME_INDEX NameIndex;
		*this << AR_INDEX(NameIndex);

		if (!NameMap.IsValidIndex(NameIndex))
			appErrorf(TEXT("Bad name index %i/%i"), (INT)NameIndex, NameMap.Num());
		Name = NameMap(NameIndex);

		return *this;
	}
	FArchive& operator<<(class UObject*& Res)
	{
		INT Index;
		*FileAr << AR_INDEX(Index);
		if (Index > 0)
		{
			--Index;
			FUE2Object* RefObj = ExportObjects(Index);
			if (!RefObj)
			{
				if (GetExportClassName(Index) == NAME_Texture)
					RefObj = new FUE2Bitmap(ExportMap(Index));
				if (RefObj)
				{
					bForceLoadObject = FALSE;
					INT OldPos = FileAr->Tell();
					FileAr->Seek(ExportMap(Index).SerialOffset);
					RefObj->Serialize(*this);
					FileAr->Seek(OldPos);
					ExportObjects(Index) = RefObj;
				}
				else if (bForceLoadObject)
				{
					RefObj = new FUE2Object(NAME_Texture, ExportMap(Index));
					ExportObjects(Index) = RefObj;
				}
			}
			if (RefObj)
				Res = reinterpret_cast<UObject*>(RefObj);
			//debugf(TEXT("GetRef %i - %ls %ls"), Index, *GetExportClassName(Index), *ExportMap(Index).ObjectName);
		}
		else if (Index < 0)
		{
			Index = -Index - 1;
			FUE2Object* RefObj = ImportObjects(Index);
			if (!RefObj)
			{
				if (bForceLoadObject)
				{
					RefObj = new FUE2Object(ImportMap(Index));
					ImportObjects(Index) = RefObj;
				}
			}
			if (RefObj)
				Res = reinterpret_cast<UObject*>(RefObj);
		}
		bForceLoadObject = FALSE;
		return *this;
	}
	void Seek(INT InPos)
	{
		FileAr->Seek(InPos);
	}
	FName GetExportClassName(INT i)
	{
		FObjectExport& Export = ExportMap(i);
		if (Export.ClassIndex < 0)
		{
			return ImportMap(-Export.ClassIndex - 1).ObjectName;
		}
		else if (Export.ClassIndex > 0)
		{
			return ExportMap(Export.ClassIndex - 1).ObjectName;
		}
		else
		{
			return NAME_Class;
		}
	}
};

class DlgImportTerrainWindow : public WCustomWindowBase
{
public:
	WLabel Prompt;
	WCoolButton OkButton;
	WCoolButton CancelButton;
	WListBox TerrainList;
	UE2LinkerLoad* Linker;

	void OnClickOk()
	{
		guard(DlgImportTerrainWindow::OnClickOk);
		UViewport* ActiveViewport = NULL;
		UClient* C = UBitmap::__Client;
		for (INT i = 0; i < C->Viewports.Num(); ++i)
		{
			if (!C->Viewports(i) || !C->Viewports(i)->Actor)
				continue;
			if (C->Viewports(i)->Actor->RendMap >= REN_Wire && C->Viewports(i)->Actor->RendMap <= REN_PlainTex)
			{
				ActiveViewport = C->Viewports(i);
				break;
			}
			else if (!ActiveViewport)
				ActiveViewport = C->Viewports(i);
		}
		ATerrainInfo* NewTerrain = NULL;
		const INT Num = (INT)TerrainList.GetSelectedCount();
		if (Num)
		{
			INT* Sel = appAllocaType(INT, Num);
			TerrainList.GetSelectedItems(Num, Sel);
			for (INT i = 0; i < Num; ++i)
			{
				// Spawn new terrain object with desired resolution by default!
				FUE2TerrainInfo* T = Linker->Terrains(Sel[i]);
				ATerrainInfo* DefaultObj = GetDefault<ATerrainInfo>();
				const INT DefX = DefaultObj->SizeX;
				const INT DefY = DefaultObj->SizeY;
				DefaultObj->SizeX = Clamp<INT>(T->HeightmapX, 2, 1024);
				DefaultObj->SizeY = Clamp<INT>(T->HeightmapY, 2, 1024);
				NewTerrain = reinterpret_cast<ATerrainInfo*>(GEditor->Level->SpawnActor(ATerrainInfo::StaticClass(), NAME_None, NULL, NULL, T->Location, FRotator(0, 0, 0)));
				DefaultObj->SizeX = DefX;
				DefaultObj->SizeY = DefY;
				T->ImportObject(NewTerrain);
				NewTerrain->RebuildTerrain();
			}
		}

		GEditor->TerrainSettings.Terrain = NewTerrain;
		RefreshTerrainList();
		GEditor->RepaintAllViewports();

		_CloseWindow();
		unguard;
	}
	void OnClickCancel()
	{
		_CloseWindow();
	}
	void DoDestroy()
	{
		guardSlow(DlgImportTerrainWindow::DoDestroy);
		delete Linker;
		WCustomWindowBase::DoDestroy();
		unguardSlow;
	}

	DlgImportTerrainWindow(HWND inParentWin, UE2LinkerLoad* L)
		: WCustomWindowBase(0)
		, Prompt(this)
		, OkButton(this, 0, FDelegate(this, (TDelegate)&DlgImportTerrainWindow::OnClickOk))
		, CancelButton(this, 0, FDelegate(this, (TDelegate)&DlgImportTerrainWindow::OnClickCancel))
		, TerrainList(this)
		, Linker(L)
	{
		guard(DlgImportTerrainWindow::DlgImportTerrainWindow);
		PerformCreateWindowEx(0, TEXT("Import Terrain"), (WS_OVERLAPPED | WS_CAPTION), 160, 160, 265, 260, inParentWin, NULL, NULL);

		FRect AreaRec = GetClientRect();
		INT yPos = 5;
		Prompt.OpenWindow(TRUE, 0);
		Prompt.SetText(TEXT("Select which terrain to import:"));
		Prompt.MoveWindow(10, yPos, 230, 16, TRUE);
		yPos += 20;

		TerrainList.OpenWindow(TRUE, TRUE, TRUE, FALSE);
		TerrainList.MoveWindow(10, yPos, 230, 150, TRUE);
		yPos += 160;

		OkButton.OpenWindow(TRUE, 80, yPos, 50, 20, TEXT("OK"));
		CancelButton.OpenWindow(TRUE, 145, yPos, 50, 20, TEXT("Cancel"));

		for (INT i = 0; i < L->Terrains.Num(); i++)
		{
			FString ItemName = FString::Printf(TEXT("%ls [%ix%i]"), L->Terrains(i)->GetName(), L->Terrains(i)->HeightmapX, L->Terrains(i)->HeightmapY);
			TerrainList.AddString(*ItemName);
		}
		unguard;
	}
};

static void TryImportTerrains(const TCHAR* FileName, HWND Wnd)
{
	FArchive* Ar = GFileManager->CreateFileReader(FileName);
	if (!Ar)
	{
		appMsgf(TEXT("Couldn't open file: %ls"), FileName);
		return;
	}
	UE2LinkerLoad* Linker = new UE2LinkerLoad(Ar);
	if (!Linker->bSucceeded)
	{
		delete Linker;
		return;
	}

	DlgImportTerrainWindow* D = new DlgImportTerrainWindow(Wnd, Linker);
	D->Show(TRUE);
}
