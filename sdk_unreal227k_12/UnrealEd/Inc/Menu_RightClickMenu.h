/*=============================================================================
	Contains all right-click menus without menu resources.
=============================================================================*/
#pragma once

static const TCHAR* KeyItemName[] = { TEXT("Plus"), TEXT("Minus"), TEXT("Enter"), TEXT("Space"), TEXT("Insert"), TEXT("Home"), TEXT("PageUp"), TEXT("PageDwn"), TEXT("End"),
	TEXT("Delete"), TEXT("Tab"), TEXT("Backspace"), TEXT("Pause"), TEXT("Up"), TEXT("Down"), TEXT("Left"),TEXT("Right"),TEXT("Multi"), TEXT("Divide"), TEXT("Esc") };

#define NUM_NUMBERKEYS (ID_VK_9-ID_VK_0+1)
#define NUM_LETTERS (ID_VK_Z-ID_VK_0+1)
#define NUM_FUNCTIONKEYS (ID_VK_F12-ID_VK_0+1)
#define NUM_COMMANDS (ID_VK_ESCAPE-ID_VK_0+1)

static const TCHAR* KeyNameToStr(INT InKey)
{
	static TCHAR Res[8];

	if (InKey < NUM_NUMBERKEYS)
	{
		Res[0] = '0' + InKey;
		Res[1] = 0;
	}
	else if (InKey < NUM_LETTERS)
	{
		Res[0] = 'A' + (InKey - NUM_NUMBERKEYS);
		Res[1] = 0;
	}
	else if (InKey < NUM_FUNCTIONKEYS)
	{
		Res[0] = 'F';
		InKey -= NUM_LETTERS;
		if (InKey >= 10)
		{
			Res[1] = '1';
			Res[2] = '0' + (InKey - 10);
			Res[3] = 0;
		}
		else
		{
			Res[1] = '0' + InKey;
			Res[2] = 0;
		}
	}
	else if (InKey < NUM_COMMANDS)
	{
		return KeyItemName[InKey - NUM_FUNCTIONKEYS];
	}
	else
	{
		Res[0] = 'N';
		Res[1] = 'U';
		Res[2] = 'M';
		Res[3] = '0' + (InKey - NUM_COMMANDS);
		Res[4] = 0;
	}
	return Res;
}
static void RC_RunRCCmd(INT ID)
{
	GEditor->Exec(CmdMenuList.CmdList(ID));
}
FMenuItem* FCmdRCItem::AddCmdItem(FPersistentRCMenu* M, const TCHAR* Name, const TCHAR* Cmd, const TCHAR* MenuKey)
{
	static TCHAR Desc[64];
	UBOOL bHasDesc = FALSE;

	const FEdKeyBinding* K = GEditor->FindMappedKey(Cmd);
	if (!K)
	{
		if (MenuKey)
		{
			bHasDesc = TRUE;
			appStrcpy(Desc, TEXT("\t&"));
			appStrcat(Desc, MenuKey);
		}
	}
	else
	{
		const TCHAR* CKey = KeyNameToStr(K->Key);
		bHasDesc = TRUE;
		appStrcpy(Desc, TEXT("\t["));
		if(K->bCtrl)
			appStrcat(Desc, TEXT("Ctrl+"));
		if (K->bAlt)
			appStrcat(Desc, TEXT("Alt+"));
		if (K->bShift)
			appStrcat(Desc, TEXT("Shift+"));
		if (MenuKey && *CKey == *MenuKey)
		{
			MenuKey = NULL;
			appStrcat(Desc, TEXT("&"));
		}
		appStrcat(Desc, CKey);
		appStrcat(Desc, TEXT("]"));
		
		if (MenuKey)
		{
			appStrcat(Desc, TEXT(" &"));
			appStrcat(Desc, MenuKey);
		}
	}
	FMenuItem* Result = M->AddItem(Name, &RC_RunRCCmd, CmdList.Num(), FALSE, FALSE, FALSE, (bHasDesc ? Desc : nullptr));
	CmdList.AddItem(Cmd);
	return Result;
}

static TArray<UClass*> CustomEntries;

static void RC_AddActor(INT ID)
{
	switch (ID)
	{
	case 0:
		GEditor->Exec(*(FString::Printf(TEXT("ACTOR ADD CLASS=\"%ls\""), GEditor->CurrentClass->GetPathName())));
		break;
	case 1:
		GEditor->Exec(TEXT("ACTOR ADD CLASS=\"Engine.Light\""));
		break;
	case 2:
		GEditor->Exec(*(FString::Printf(TEXT("ACTOR ADD MESH NAME=\"%ls\" SNAP=1"), UEditorEngine::CurrentMesh->GetPathName())));
		break;
	}
	GEditor->Exec(TEXT("POLY SELECT NONE"));
}
static void RC_AddCustomActor(INT ID)
{
	GEditor->Exec(*(FString::Printf(TEXT("ACTOR ADD CLASS=\"%ls\""), CustomEntries(ID)->GetPathName())));
	GEditor->Exec(TEXT("POLY SELECT NONE"));
}
static void UpdateAddActor(FMenuItem* AddActor, FMenuItem* AddMesh)
{
	AddActor->Logf(TEXT("Add %ls Here\t&A"), GEditor->CurrentClass ? GEditor->CurrentClass->GetName() : TEXT("None"));
	AddActor->SetDisabled(!GEditor->CurrentClass || (GEditor->CurrentClass->ClassFlags & (CLASS_Abstract | CLASS_NoUserCreate | CLASS_Transient)) || !GEditor->CurrentClass->IsChildOf(AActor::StaticClass()));

	if(!UEditorEngine::CurrentMesh)
		AddMesh->Log(TEXT("Add Mesh None"));
	else AddMesh->Logf(TEXT("Add %ls %ls Here"), UEditorEngine::CurrentMesh->GetClass()->GetName(), UEditorEngine::CurrentMesh->GetName());
	AddMesh->SetDisabled(UEditorEngine::CurrentMesh == NULL);
}

static void FillAddActorsList(FPersistentRCMenu& Menu, FMenuItem*& AddActor, FMenuItem*& AddMesh)
{
	guard(FillAddActorsList);
	AddActor = Menu.AddItem(TEXT(""), &RC_AddActor, 0);
	AddMesh = Menu.AddItem(TEXT(""), &RC_AddActor, 2);
	Menu.AddItem(TEXT("Add Light Here\t[&L+Click]"), &RC_AddActor, 1);

	if(!CustomEntries.Num())
	{
		TMultiMap<FString, FString>* Sec = GConfig->GetSectionPrivate(TEXT("ContextMenuClassAdd"), 0, 1, GUEdIni);
		if (Sec)
		{
			INT i;
			for (i = 0; i < 16; ++i)
			{
				FString* Res = Sec->Find(FString::Printf(TEXT("Custom%i"), (i + 1)));
				if (Res)
				{
					UClass* C = FindObject<UClass>(ANY_PACKAGE, *Res->StripDelimiter(), 1);
					if (C && C->IsChildOf(AActor::StaticClass()))
					{
						C->AddToRoot();
						CustomEntries.AddUniqueItem(C);
					}
				}
				else break;
			}
			TArray<FString> Values;
			Sec->MultiFind(TEXT("Custom"), Values);
			for (i = 0; i < Values.Num(); ++i)
			{
				UClass* C = FindObject<UClass>(ANY_PACKAGE, *Values(i).StripDelimiter(), 1);
				if (C && C->IsChildOf(AActor::StaticClass()))
				{
					C->AddToRoot();
					CustomEntries.AddUniqueItem(C);
				}
			}
		}
	}
	for (INT i = 0; i < CustomEntries.Num(); ++i)
	{
		if (i <= 8)
			Menu.AddItem(*FString::Printf(TEXT("Add %ls Here\t&%i"), CustomEntries(i)->GetName(), (i + 1)), &RC_AddCustomActor, i);
		else Menu.AddItem(*FString::Printf(TEXT("Add %ls Here"), CustomEntries(i)->GetName()), &RC_AddCustomActor, i);
	}
	Menu.AddLineBreak();
	unguard;
}

static void RC_SurfProperties(INT ID)
{
	GEditor->UpdatePropertiesWindows();
	GSurfPropSheet->Show(TRUE);
}

//=============================================================================================
void MENU_SurfaceMenu(HWND Wnd)
{
	guard(MENU_SurfaceMenu);
	INT i;
	static FPersistentRCMenu Menu;
	static FMenuItem* SurfProps, * AddActor, * AddMesh;
	ULevel* Level = GEditor->Level;

	if (!Menu.bMenuCreated)
	{
		SurfProps = CmdMenuList.AddCmdItem(&Menu, TEXT(""), TEXT("HOOK OPEN SURFPROPERTIES"), TEXT("F"));
		Menu.AddLineBreak();

		FillAddActorsList(Menu, AddActor, AddMesh);

		Menu.AddPopup(TEXT("Align Selected...\t&N"));
		{
			CmdMenuList.AddCmdItem(&Menu, TEXT("Align as Floor/Ceiling"), TEXT("POLY TEXALIGN FLOOR"), TEXT("F"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Align Wall Direction"), TEXT("POLY TEXALIGN WALLDIR"), TEXT("D"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Align Wall Panning"), TEXT("POLY TEXALIGN WALLPAN"), TEXT("P"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Align to Wall around X Axis"), TEXT("POLY TEXALIGN WALLX"), TEXT("X"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Align to Wall around Y Axis"), TEXT("POLY TEXALIGN WALLY"), TEXT("Y"));
			Menu.AddLineBreak();
			CmdMenuList.AddCmdItem(&Menu, TEXT("Unalign back to default"), TEXT("POLY TEXALIGN DEFAULT"), TEXT("U"));
		}
		Menu.EndPopup();

		CmdMenuList.AddCmdItem(&Menu, TEXT("Reset Surface"), TEXT("POLY RESET"));
		Menu.AddPopup(TEXT("Modify surface...\t&M"));
		CmdMenuList.AddCmdItem(&Menu, TEXT("Tesselate"), TEXT("POLY TESSELLATE"), TEXT("T"));
		CmdMenuList.AddCmdItem(&Menu, TEXT("Merge"), TEXT("POLY MERGE"), TEXT("M"));
		CmdMenuList.AddCmdItem(&Menu, TEXT("Flip"), TEXT("POLY FLIP"), TEXT("F"));
		CmdMenuList.AddCmdItem(&Menu, TEXT("Delete"), TEXT("POLY DELETE"), TEXT("D"));
		Menu.EndPopup();
		Menu.AddLineBreak();

		Menu.AddPopup(TEXT("Select Surfaces...\t&S"));
		{
			CmdMenuList.AddCmdItem(&Menu, TEXT("Select All Surfaces"), TEXT("POLY SELECT ALL"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Select None"), TEXT("POLY SELECT NONE"));
			Menu.AddLineBreak();
			CmdMenuList.AddCmdItem(&Menu, TEXT("Matching Groups"), TEXT("POLY SELECT MATCHING GROUPS"), TEXT("G"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Matching Items"), TEXT("POLY SELECT MATCHING ITEMS"), TEXT("I"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Matching Brush"), TEXT("POLY SELECT MATCHING BRUSH"), TEXT("B"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Matching Texture"), TEXT("POLY SELECT MATCHING TEXTURE"), TEXT("T"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Matching PolyFlags"), TEXT("POLY SELECT MATCHING POLYFLAGS"), TEXT("P"));
			Menu.AddLineBreak();
			CmdMenuList.AddCmdItem(&Menu, TEXT("All Adjacents"), TEXT("POLY SELECT ADJACENT ALL"), TEXT("J"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Adjacent Coplanars"), TEXT("POLY SELECT ADJACENT COPLANARS"), TEXT("C"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Adjacent Walls"), TEXT("POLY SELECT ADJACENT WALLS"), TEXT("W"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Adjacent Floors/Ceilings"), TEXT("POLY SELECT ADJACENT FLOORS"), TEXT("F"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Adjacent Slants"), TEXT("POLY SELECT ADJACENT SLANTS"), TEXT("S"));
			Menu.AddLineBreak();
			CmdMenuList.AddCmdItem(&Menu, TEXT("Reverse"), TEXT("POLY SELECT REVERSE"), TEXT("Q"));
			Menu.AddLineBreak();
			CmdMenuList.AddCmdItem(&Menu, TEXT("Memorize Set"), TEXT("POLY SELECT MEMORY SET"), TEXT("M"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Recall Memory"), TEXT("POLY SELECT MEMORY RECALL"), TEXT("R"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Or with Memory"), TEXT("POLY SELECT MEMORY INTERSECTION"), TEXT("O"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("And with Memory"), TEXT("POLY SELECT MEMORY UNION"), TEXT("A"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Xor with Memory"), TEXT("POLY SELECT MEMORY XOR"), TEXT("X"));
		}
		Menu.EndPopup();

		CmdMenuList.AddCmdItem(&Menu, TEXT("Apply Texture"), TEXT("POLY SETTEXTURE"), TEXT("T"));
		Menu.AddLineBreak();

		CmdMenuList.AddCmdItem(&Menu, TEXT("Align Viewports to 3D Camera"), TEXT("CAMERA ALIGN"), TEXT("C"));
		CmdMenuList.AddCmdItem(&Menu, TEXT("Paste Actor here"), TEXT("EDIT PASTEPOS"));
		Menu.AddLineBreak();
		CmdMenuList.AddCmdItem(&Menu, TEXT("Play Level from here"), TEXT("HOOK PLAYMAP CUSTOM=1"), TEXT("P"));
		Menu.AddLineBreak();
		CmdMenuList.AddCmdItem(&Menu, TEXT("Show viewport information"), TEXT("VIEWPORTINFO"), TEXT("-"));
	}
	{
		INT NumSurfs = 0;
		for (i = 0; i < Level->Model->Surfs.Num(); i++)
		{
			FBspSurf* Poly = &Level->Model->Surfs(i);

			if (Poly->PolyFlags & PF_Selected)
			{
				NumSurfs++;
			}
		}
		SurfProps->Logf(TEXT("Surface Properties (%i Selected)"), NumSurfs);
	}

	UpdateAddActor(AddActor, AddMesh);

	Menu.OpenMenu(Wnd);
	unguard;
}

static void RC_MoverAction(INT ID)
{
	if (ID == INDEX_NONE)
	{
		for (INT i = 0; i < GEditor->Level->Actors.Num(); i++)
		{
			ABrush* Brush = Cast<ABrush>(GEditor->Level->Actors(i));
			if (Brush && Brush->IsMovingBrush() && Brush->bSelected)
			{
				Brush->Brush->EmptyModel(1, 0);
				Brush->Brush->BuildBound();
				GEditor->bspBuild(Brush->Brush, BSP_Good, 15, 1, 0);
				GEditor->bspRefresh(Brush->Brush, 1);
				GEditor->bspValidateBrush(Brush->Brush, 1, 1);
				GEditor->bspBuildBounds(Brush->Brush);

				GEditor->bspBrushCSG(Brush, GEditor->Level->Model, 0, CSG_Add, 1);
			}
		}
		GEditor->RedrawLevel(GEditor->Level);
	}
	else GEditor->Exec(*FString::Printf(TEXT("ACTOR KEYFRAME NUM=%i"), ID));
}

void RC_ActorConvertTo(INT ID);

static UClass* SelClassType;
static AActor* SelSingleActor;
static void RC_ActorSelect(INT ID)
{
	GEditor->Exec(*FString::Printf(TEXT("ACTOR SELECT OFCLASS CLASS=%ls"), SelClassType->GetPathName()));
}
static void RC_MapBuildStep(INT ID)
{
	if (SelSingleActor)
		GEditor->Exec(*FString::Printf(TEXT("BSP STEP %ls"), SelSingleActor->GetName()));
}

static void RC_ReplaceDupeActor(INT ID)
{
	GEditor->Exec(*FString::Printf(TEXT("ACTOR REPLACE CLASS=%ls KEEP=%i"), GEditor->CurrentClass->GetPathName(), ID));
}

//=============================================================================================
void MENU_ActorsMenu(HWND Wnd)
{
	guard(MENU_ActorsMenu);
	INT i;
	constexpr INT MAX_MOVER_KEYS = ARRAY_COUNT(AMover::KeyPos);
	static FPersistentRCMenu Menu;
	static FMenuItem* ActorProps, * ActorPropStatic, * AddActor, * AddMesh, * AllActors, * AllMatchMesh, * ReplaceWith, * ReplaceWithP, * ToBrushConv, * ToMeshConv, * TransformPerma, * RecomputeBSP, * BSPBuildUntil, * PathPreviewMode, * MergeBrushes;
	static FMenuGroup* MoverGroup, * ReplaceGroup, * CSGGroup, * SolidGroup, * PolygonsGroup;
	static FMenuItem* MoverKeys[MAX_MOVER_KEYS], *NewMoverKeys[2];
	static INT OldMoverKey = INDEX_NONE;
	static INT OldFinalKey = 1;
	ULevel* Level = GEditor->Level;

	if (!Menu.bMenuCreated)
	{
		ActorProps = CmdMenuList.AddCmdItem(&Menu, TEXT(""), TEXT("HOOK ACTORPROPERTIES"), TEXT("F"));
		ActorPropStatic = CmdMenuList.AddCmdItem(&Menu, TEXT(""), TEXT("HOOK ACTORPROPERTIES PERSISTENT"));
		Menu.AddLineBreak();
		FillAddActorsList(Menu, AddActor, AddMesh);

		CmdMenuList.AddCmdItem(&Menu, TEXT("Actor align to grid"), TEXT("ACTOR ALIGN"), TEXT("G"));
		CmdMenuList.AddCmdItem(&Menu, TEXT("Create StaticMesh collision"), TEXT("MESH STATICCOLLISION"));
		Menu.AddLineBreak();

		MoverGroup = Menu.AddPopup(TEXT("Movers\t&M"));
		{
			Menu.AddItem(TEXT("Show Polys\t&S"), &RC_MoverAction, INDEX_NONE);
			Menu.AddLineBreak();

			TCHAR KeyDesc[128], IntDesc[6];
			for (i = 0; i < MAX_MOVER_KEYS; ++i)
			{
				appSnprintf(KeyDesc, 128, TEXT("Key %i"), i);
				if (i == 0)
					appStrncat(KeyDesc, TEXT(" (Base)"), 128);
				if (i < 9)
				{
					appSnprintf(IntDesc, 6, TEXT("\t&%i"), i + 1);
					appStrncat(KeyDesc, IntDesc, 128);
				}
				MoverKeys[i] = Menu.AddItem(KeyDesc, &RC_MoverAction, i, FALSE, FALSE, (i > 0));
			}

			Menu.AddLineBreak();
			NewMoverKeys[0] = CmdMenuList.AddCmdItem(&Menu, TEXT("New Key from current"), TEXT("ACTOR KEYFRAME NEW CURRENT"));
			NewMoverKeys[1] = CmdMenuList.AddCmdItem(&Menu, TEXT("New Key from base"), TEXT("ACTOR KEYFRAME NEW"));
		}
		Menu.EndPopup();

		Menu.AddPopup(TEXT("Reset\t&R"));
		{
			CmdMenuList.AddCmdItem(&Menu, TEXT("Move to Origin"), TEXT("ACTOR RESET LOCATION"), TEXT("O"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Reset Pivot"), TEXT("ACTOR RESET PIVOT"), TEXT("P"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Reset Rotation"), TEXT("ACTOR RESET ROTATION"), TEXT("R"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Reset Scaling"), TEXT("ACTOR RESET SCALE"), TEXT("S"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Reset All"), TEXT("ACTOR RESET ALL"), TEXT("A"));
		}
		Menu.EndPopup();

		Menu.AddPopup(TEXT("Transform\t&T"));
		{
			CmdMenuList.AddCmdItem(&Menu, TEXT("Mirror about X"), TEXT("ACTOR MIRROR X=-1"), TEXT("X"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Mirror about Y"), TEXT("ACTOR MIRROR Y=-1"), TEXT("Y"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Mirror about Z (vertical)"), TEXT("ACTOR MIRROR Z=-1"), TEXT("Z"));
			TransformPerma = CmdMenuList.AddCmdItem(&Menu, TEXT("Transform permanently"), TEXT("ACTOR APPLYTRANSFORM"), TEXT("T"));
			RecomputeBSP = CmdMenuList.AddCmdItem(&Menu, TEXT("Rebuild dynamic BSP model"), TEXT("ACTOR REBUILDMODEL"), TEXT("R"));
		}
		Menu.EndPopup();

		Menu.AddPopup(TEXT("Convert\t&C"));
		{
			ToMeshConv = Menu.AddItem(TEXT("To StaticMesh\t&S"), &RC_ActorConvertTo, 0);
			ToBrushConv = CmdMenuList.AddCmdItem(&Menu, TEXT("To Brush"), TEXT("ACTOR CONVERT BRUSH"), TEXT("B"));
		}
		Menu.EndPopup();

		Menu.AddPopup(TEXT("Order\t&O"));
		{
			CmdMenuList.AddCmdItem(&Menu, TEXT("To First"), TEXT("MAP SENDTO FIRST"), TEXT("F"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("To Last"), TEXT("MAP SENDTO LAST"), TEXT("L"));
		}
		Menu.EndPopup();

		PolygonsGroup = Menu.AddPopup(TEXT("Polygons\t&Y"));
		{
			CmdMenuList.AddCmdItem(&Menu, TEXT("To Brush"), TEXT("MAP BRUSH GET"), TEXT("T"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("From Brush"), TEXT("MAP BRUSH PUT"), TEXT("F"));
			MergeBrushes = CmdMenuList.AddCmdItem(&Menu, TEXT("Merge Brushes"), TEXT("MAP BRUSH GET MERGED"));
			Menu.AddLineBreak();
			CmdMenuList.AddCmdItem(&Menu, TEXT("Merge"), TEXT("BRUSH MERGE"), TEXT("M"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Separate"), TEXT("BRUSH SEPARATE"), TEXT("S"));
			Menu.AddLineBreak();
			CmdMenuList.AddCmdItem(&Menu, TEXT("Snap vertices to grid"), TEXT("ACTOR SNAPTOGRID"), TEXT("G"));
		}
		Menu.EndPopup();

		SolidGroup = Menu.AddPopup(TEXT("Solidity\t&S"));
		{
			CmdMenuList.AddCmdItem(&Menu, TEXT("Solid"), TEXT("MAP SETBRUSH CLEARFLAGS=40 SETFLAGS=0"), TEXT("S"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Semisolid"), TEXT("MAP SETBRUSH CLEARFLAGS=40 SETFLAGS=32"), TEXT("E"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Nonsolid"), TEXT("MAP SETBRUSH CLEARFLAGS=40 SETFLAGS=8"), TEXT("N"));
		}
		Menu.EndPopup();

		CSGGroup = Menu.AddPopup(TEXT("CSG\t&G"));
		{
			CmdMenuList.AddCmdItem(&Menu, TEXT("Additive"), TEXT("MAP SETBRUSH CSGOPER=1"), TEXT("A"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Subtractive"), TEXT("MAP SETBRUSH CSGOPER=2"), TEXT("S"));
			Menu.AddLineBreak();
			BSPBuildUntil = Menu.AddItem(TEXT("BSP Build until this actor\t&U"), &RC_MapBuildStep, 0);
			CmdMenuList.AddCmdItem(&Menu, TEXT("BSP Build Prev Step"), TEXT("BSP STEP -1"), TEXT("P"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("BSP Build Next Step"), TEXT("BSP STEP 1"), TEXT("N"));
		}
		Menu.EndPopup();
		Menu.AddLineBreak();

		AllActors = Menu.AddItem(TEXT(""), &RC_ActorSelect, 0, FALSE, FALSE, FALSE, TEXT("\t&A"));
		AllMatchMesh = CmdMenuList.AddCmdItem(&Menu, TEXT(""), TEXT("ACTOR SELECT MATCHING"), TEXT(""));
		CmdMenuList.AddCmdItem(&Menu, TEXT("Select All"), TEXT("ACTOR SELECT ALL"));
		CmdMenuList.AddCmdItem(&Menu, TEXT("Select Invert"), TEXT("ACTOR SELECT INVERT"));
		CmdMenuList.AddCmdItem(&Menu, TEXT("Select None"), TEXT("SELECT NONE"));
		Menu.AddPopup(TEXT("Select Brushes"));
		{
			CmdMenuList.AddCmdItem(&Menu, TEXT("Adds"), TEXT("MAP SELECT ADDS"), TEXT("A"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Subtracts"), TEXT("MAP SELECT SUBTRACTS"), TEXT("S"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Semisolids"), TEXT("MAP SELECT SEMISOLIDS"), TEXT("E"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Nonsolids"), TEXT("MAP SELECT NONSOLIDS"), TEXT("N"));
		}
		Menu.EndPopup();
		CmdMenuList.AddCmdItem(&Menu, TEXT("Select Inside brush"), TEXT("ACTOR SELECT INSIDE"));
		Menu.AddLineBreak();

		Menu.AddPopup(TEXT("Hide Actors...\t&H"));
		{
			CmdMenuList.AddCmdItem(&Menu, TEXT("Hide Selected Actors"), TEXT("ACTOR HIDE SELECTED"), TEXT("A"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Hide UnSelected Actors"), TEXT("ACTOR HIDE UNSELECTED"), TEXT("S"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Invert Hidden Actors view"), TEXT("ACTOR HIDE INVERT"), TEXT("E"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Show All (hidden) Actors"), TEXT("ACTOR UNHIDE ALL"), TEXT("U"));
		}
		Menu.EndPopup();
		Menu.AddLineBreak();

		ReplaceGroup = Menu.AddPopup(TEXT("Replace...\t&R"));
		{
			ReplaceWith = Menu.AddItem(TEXT(""), &RC_ReplaceDupeActor, 0);
			ReplaceWithP = Menu.AddItem(TEXT(""), &RC_ReplaceDupeActor, 1);
		}
		Menu.EndPopup();
		CmdMenuList.AddCmdItem(&Menu, TEXT("Duplicate"), TEXT("ACTOR DUPLICATE"), TEXT("D"));
		CmdMenuList.AddCmdItem(&Menu, TEXT("Delete Actor"), TEXT("DELETE"));
		CmdMenuList.AddCmdItem(&Menu, TEXT("Edit script"), TEXT("HOOK OPEN SCRIPT"), TEXT("E"));
		CmdMenuList.AddCmdItem(&Menu, TEXT("Make Current"), TEXT("HOOK MAKECURRENT"));
		Menu.AddLineBreak();
		CmdMenuList.AddCmdItem(&Menu, TEXT("Cut Actor"), TEXT("EDIT CUT"), TEXT("X"));
		CmdMenuList.AddCmdItem(&Menu, TEXT("Copy Actor"), TEXT("EDIT COPY"), TEXT("C"));
		Menu.AddLineBreak();
		PathPreviewMode = CmdMenuList.AddCmdItem(&Menu, TEXT("Preview Interpolation path"), TEXT("ACTOR INTERPOLATE"), TEXT("I"));
		CmdMenuList.AddCmdItem(&Menu, TEXT("Play Level from here"), TEXT("HOOK PLAYMAP CUSTOM=1"), TEXT("P"));
		Menu.AddLineBreak();
		CmdMenuList.AddCmdItem(&Menu, TEXT("Show viewport information"), TEXT("VIEWPORTINFO"), TEXT("-"));
	}

	{
		INT NumActors = 0;
		INT NumBrushes = 0;
		UClass* AllClass = NULL;
		UMesh* AllMesh = NULL;
		AActor* A;
		UBOOL bSelectMover = FALSE;
		UBOOL bAnyMesh = FALSE;
		INT MoverKey = 0;
		INT MoverFinalKey = 0;
		UBOOL bHasABrush = FALSE;
		UBOOL bHasBrush = FALSE;
		UBOOL bHasDynBSP = FALSE;
		SelSingleActor = nullptr;

		for(FActorIterator It(Level); It; ++It)
		{
			A = *It;
			if (A && A->bSelected)
			{
				SelSingleActor = A;
				if (NumActors && A->GetClass() != AllClass)
					AllClass = NULL;
				else
					AllClass = A->GetClass();
				if (NumActors && A->Mesh != AllMesh)
					AllMesh = NULL;
				else AllMesh = A->Mesh;
				if (A->bIsMover)
				{
					if (!bSelectMover)
					{
						MoverKey = Min<INT>(reinterpret_cast<AMover*>(A)->KeyNum, MAX_MOVER_KEYS - 1);
						MoverFinalKey = Clamp<INT>(reinterpret_cast<AMover*>(A)->NumKeys, 1, MAX_MOVER_KEYS);
					}
					else
					{
						if (MoverKey != reinterpret_cast<AMover*>(A)->KeyNum)
							MoverKey = INDEX_NONE;
						if (MoverFinalKey != Clamp<INT>(reinterpret_cast<AMover*>(A)->NumKeys, 1, MAX_MOVER_KEYS))
							MoverFinalKey = INDEX_NONE;
					}
					bSelectMover = TRUE;
				}
				bAnyMesh = bAnyMesh || (A->Mesh != NULL);
				bHasABrush = (bHasABrush || A->GetClass()==ABrush::StaticClass());
				if (A->GetClass() == ABrush::StaticClass())
					++NumBrushes;
				bHasBrush = bHasBrush || A->Brush;
				bHasDynBSP = bHasDynBSP || (A->Brush && A->GetClass() != ABrush::StaticClass());
				NumActors++;
			}
		}
		ActorProps->Logf(TEXT("%ls Properties (%i Selected)"), AllClass ? AllClass->GetName() : TEXT("Actor"), NumActors);
		ActorPropStatic->Logf(TEXT("%ls Properties (Persistent - %i Selected)"), AllClass ? AllClass->GetName() : TEXT("Actor"), NumActors);

		SelClassType = AllClass;
		if (AllClass)
		{
			AllActors->Logf(TEXT("Select all %ls Actors"), AllClass->GetName());
			AllActors->SetHidden(0);
		}
		else AllActors->SetHidden(1);

		if (AllMesh)
			AllMatchMesh->Logf(TEXT("Select matching mesh %ls Actors"), AllMesh->GetName());
		else AllMatchMesh->Log(TEXT("Select matching meshes"));
		AllMatchMesh->SetDisabled(!bAnyMesh);
		ToMeshConv->SetDisabled(!bAnyMesh && !bHasBrush);
		ToBrushConv->SetDisabled(!bAnyMesh);

		PolygonsGroup->SetHidden(!bHasBrush);
		if (bHasBrush)
			MergeBrushes->SetDisabled(NumBrushes <= 1);
		CSGGroup->SetHidden(!bHasABrush);
		SolidGroup->SetHidden(!bHasABrush);
		TransformPerma->SetDisabled(!bHasABrush);
		RecomputeBSP->SetDisabled(!bHasDynBSP);
		BSPBuildUntil->SetDisabled(NumActors!=1 || SelSingleActor->GetClass()!=ABrush::StaticClass() || SelSingleActor==Level->Brush());

		MoverGroup->SetHidden(!bSelectMover);
		if (bSelectMover)
		{
			if (MoverFinalKey == INDEX_NONE)
				MoverFinalKey = MAX_MOVER_KEYS;
			if (MoverKey >= MoverFinalKey)
				MoverFinalKey = MoverKey + 1;

			if (OldFinalKey != MoverFinalKey)
			{
				if (MoverFinalKey > OldFinalKey)
				{
					for (i = OldFinalKey; i < MoverFinalKey; ++i)
						MoverKeys[i]->SetHidden(FALSE);
				}
				else
				{
					for (i = MoverFinalKey; i < OldFinalKey; ++i)
						MoverKeys[i]->SetHidden(TRUE);
				}
				OldFinalKey = MoverFinalKey;
				NewMoverKeys[0]->SetDisabled(MoverFinalKey >= MAX_MOVER_KEYS);
				NewMoverKeys[1]->SetDisabled(MoverFinalKey >= MAX_MOVER_KEYS);
			}
			if (OldMoverKey != MoverKey)
			{
				if (OldMoverKey >= 0)
					MoverKeys[OldMoverKey]->SetChecked(0);
				if (MoverKey >= 0)
					MoverKeys[MoverKey]->SetChecked(1);
				OldMoverKey = MoverKey;
			}
		}
		PathPreviewMode->SetHidden(NumActors!=1 || !SelSingleActor->IsA(AInterpolationPoint::StaticClass()));
	}

	if (GEditor->CurrentClass && !(GEditor->CurrentClass->ClassFlags & (CLASS_Abstract | CLASS_NoUserCreate | CLASS_Transient)) && GEditor->CurrentClass->IsChildOf(AActor::StaticClass()))
	{
		ReplaceWith->Logf(TEXT("Replace with %ls"), GEditor->CurrentClass->GetPathName());
		ReplaceWithP->Logf(TEXT("Replace with %ls (keep values)"), GEditor->CurrentClass->GetPathName());
		ReplaceGroup->SetHidden(FALSE);
	}
	else ReplaceGroup->SetHidden(TRUE);

	UpdateAddActor(AddActor, AddMesh);

	Menu.OpenMenu(Wnd);
	unguard;
}

static INT PreGridSizes[] = { 1,2,4,8,16,32,64,128,256 };
void RC_ChangeGrid(INT ID);

//=============================================================================================
void MENU_BackgroundMenu(HWND Wnd)
{
	guard(MENU_BackgroundMenu);
	INT i;
	static FPersistentRCMenu Menu;
	static FMenuItem* AddActor, * AddMesh,*GridEnable;
	static INT LastGridIndex = INDEX_NONE;
	static FMenuItem* GridBtns[ARRAY_COUNT(PreGridSizes) + 1];

	if (!Menu.bMenuCreated)
	{
		FillAddActorsList(Menu, AddActor, AddMesh);

		Menu.AddPopup(TEXT("Grid\t&G"));
		{
			for (i = 0; i < ARRAY_COUNT(PreGridSizes); ++i)
				GridBtns[i] = Menu.AddItem(*FString::Printf(i==0 ? TEXT("%i unit\t&%i") : TEXT("%i units\t&%i"), PreGridSizes[i],(i+1)), &RC_ChangeGrid, i);
			GridBtns[i] = Menu.AddItem(TEXT(""), &RC_ChangeGrid, i);
			GridEnable = CmdMenuList.AddCmdItem(&Menu, TEXT("Toggle Snap to Grid"), TEXT("MAP GRID X=0 Y=0 Z=0"), TEXT("T"));
		}
		Menu.EndPopup();

		Menu.AddPopup(TEXT("Pivot\t&P"));
		{
			CmdMenuList.AddCmdItem(&Menu, TEXT("Place pivot snapped here"), TEXT("PIVOT SNAPPED"), TEXT("S"));
			CmdMenuList.AddCmdItem(&Menu, TEXT("Toggle Snap to Grid"), TEXT("PIVOT HERE"), TEXT("P"));
		}
		Menu.EndPopup();
		Menu.AddLineBreak();

		CmdMenuList.AddCmdItem(&Menu, TEXT("Level Properties"), TEXT("HOOK OPEN LEVELPROPERTIES"), TEXT("V"));
		Menu.AddLineBreak();

		CmdMenuList.AddCmdItem(&Menu, TEXT("Paste Actor"), TEXT("EDIT PASTE"), TEXT("T"));
		CmdMenuList.AddCmdItem(&Menu, TEXT("Paste Actor here"), TEXT("EDIT PASTEPOS"), TEXT("H"));

		Menu.AddLineBreak();
		CmdMenuList.AddCmdItem(&Menu, TEXT("Show viewport information"), TEXT("VIEWPORTINFO"), TEXT("-"));
	}

	{
		INT CurGrid = appRound(GEditor->GridSize.X);
		for (i = 0; i < ARRAY_COUNT(PreGridSizes); ++i)
			if (PreGridSizes[i] == CurGrid)
				break;
		if (i != LastGridIndex)
		{
			if (LastGridIndex >= 0)
				GridBtns[LastGridIndex]->SetChecked(FALSE);
			LastGridIndex = i;
			GridBtns[i]->SetChecked(TRUE);
			if (i < ARRAY_COUNT(PreGridSizes))
				GridBtns[ARRAY_COUNT(PreGridSizes)]->Log(TEXT("Custom\t&0"));
		}
		if (i == ARRAY_COUNT(PreGridSizes))
			GridBtns[ARRAY_COUNT(PreGridSizes)]->Logf(TEXT("Custom (%i units)\t&0"), CurGrid);
		GridEnable->SetChecked(GEditor->GridEnabled != 0);
	}

	UpdateAddActor(AddActor, AddMesh);

	Menu.OpenMenu(Wnd);
	unguard;
}

struct FModeEntry
{
	INT ModeID, NotID;
	const TCHAR* EntryName;
	FMenuItem* MenuButton;

	FModeEntry(INT ID, const TCHAR* Name, INT xOr = 0)
		: ModeID(ID), EntryName(Name), MenuButton(NULL), NotID(xOr)
	{}
};
static FModeEntry ViewportModesList[] = {
	FModeEntry(REN_OrthXY,TEXT("Top")),
	FModeEntry(REN_OrthXZ,TEXT("Front")),
	FModeEntry(REN_OrthYZ,TEXT("Side")),
	FModeEntry(REN_Wire,TEXT("Viewframe")),
	FModeEntry(REN_Polys,TEXT("Texture Usage")),
	FModeEntry(REN_PolyCuts,TEXT("BSP Cuts")),
	FModeEntry(REN_PlainTex,TEXT("Textured")),
	FModeEntry(REN_DynLight,TEXT("Dynamic Lighting")),
	FModeEntry(REN_Zones,TEXT("Zones/Portals")),
	FModeEntry(REN_LightingOnly,TEXT("Lighting Only")),
	FModeEntry(REN_Normals,TEXT("Normals")),
};
static FModeEntry ViewportShowList[] = {
	FModeEntry(SHOW_Brush,TEXT("Show Active Brush\t&[&B]")),
	FModeEntry(SHOW_MovingBrushes,TEXT("Show Moving Brushes\t&M")),
	FModeEntry(SHOW_Backdrop,TEXT("Show Backdrop\t&K")),
	FModeEntry(SHOW_RealTimeBackdrop,TEXT("Show Realtime Backdrop\t&R")),
	FModeEntry(SHOW_Coords,TEXT("Show Coordinates\t&C")),
	FModeEntry(SHOW_Paths,TEXT("Show Paths\t&P")),
	FModeEntry(SHOW_PathsPreview,TEXT("Show Paths Preview\t&W")),
	FModeEntry(SHOW_Events,TEXT("Show Events\t&E")),
	FModeEntry(SHOW_DistanceFog,TEXT("Show Distance Fog\t&F")),
	FModeEntry(SHOW_LightColorIcon,TEXT("Show LightIcon Color\t&L")),
	FModeEntry(SHOW_Projectors,TEXT("Show Projectors\t&M")),
	FModeEntry(SHOW_StaticMeshes,TEXT("Show Static Meshes\t&S")),
	FModeEntry(SHOW_Emitters,TEXT("Show Emitters\t&E")),
	FModeEntry(SHOW_Terrains,TEXT("Show Terrains\t&T")),
}; 
static FModeEntry ViewportActorShowList[] = {
	FModeEntry(SHOW_Actors,TEXT("Full Actor View\t[&H]"),(SHOW_ActorIcons | SHOW_ActorRadii)),
	FModeEntry((SHOW_Actors | SHOW_ActorIcons),TEXT("Icon View\t&I"),SHOW_ActorRadii),
	FModeEntry((SHOW_Actors | SHOW_ActorRadii),TEXT("Radii View\t&R"),SHOW_ActorIcons),
	FModeEntry(0,TEXT("Hide Actors\t&H"),(SHOW_Actors | SHOW_ActorIcons | SHOW_ActorRadii)),
	FModeEntry(SHOW_InGameMode,TEXT("Show In-Game Preview\t&P")),
};

static UViewport* _PendingViewportCmd = NULL;
static WWindow* _PendingCallbackWnd = NULL;
static WViewportWindowContainer* _PendingViewportcontainer = NULL;

static void RC_ViewportMode(INT ID)
{
	_PendingViewportCmd->Actor->RendMap = ViewportModesList[ID].ModeID;
	_PendingViewportCmd->RepaintPending = TRUE;
	_PendingCallbackWnd->OnCommand(WM_PB_PUSH);
}
static void RC_ViewportShowFlags(INT ID)
{
	_PendingViewportCmd->Actor->ShowFlags ^= ViewportShowList[ID].ModeID;
	_PendingViewportCmd->RepaintPending = TRUE;
}
static void RC_ViewportActorShowFlags(INT ID)
{
	if (ViewportActorShowList[ID].NotID)
	{
		_PendingViewportCmd->Actor->ShowFlags &= ~ViewportActorShowList[ID].NotID;
		_PendingViewportCmd->Actor->ShowFlags |= ViewportActorShowList[ID].ModeID;
	}
	else _PendingViewportCmd->Actor->ShowFlags ^= ViewportActorShowList[ID].ModeID;
	_PendingViewportCmd->RepaintPending = TRUE;
}
static void RC_SetColorMode(INT ID)
{
	_PendingViewportCmd->RenDev->SetRes(_PendingViewportCmd->SizeX, _PendingViewportCmd->SizeY, ID==0 ? 2 : 4, 0);
	_PendingViewportCmd->RepaintPending = TRUE;
}
static void RC_SetRenderDevice(INT ID)
{
	 //debugf(TEXT("RC_SetRenderDevice SwitchRenderer %ls ConfigName %ls"), _PendingViewportCmd->GetFullName(), *_PendingViewportcontainer->ConfigName);
	 _PendingCallbackWnd->OwnerWindow->OnCommand(IDMN_RD_SOFTWARE + ID);// IDMN_RD_SOFTWARE has to be started with. See resource.h
}

//=============================================================================================
void MENU_ContextMenu(HWND Wnd, UViewport* Viewport, WWindow* CallbackWindow, WViewportWindowContainer* Viewportcontainer)
{
	guard(MENU_ContextMenu);
	_PendingViewportCmd = Viewport;
	_PendingCallbackWnd = CallbackWindow;
	_PendingViewportcontainer = Viewportcontainer;

	INT i;
	static FPersistentRCMenu Menu;
	static TArray<FMenuItem*> DrvButtons;

	if (!Menu.bMenuCreated)
	{
		Menu.AddPopup(TEXT("Mode\t&M"));
		{
			for (i = 0; i < ARRAY_COUNT(ViewportModesList); ++i)
			{
				ViewportModesList[i].MenuButton = Menu.AddItem(ViewportModesList[i].EntryName, &RC_ViewportMode, i);
				if(i==2)
					Menu.AddLineBreak();
			}
		}
		Menu.EndPopup();

		Menu.AddPopup(TEXT("View\t&V"));
		{
			for (i = 0; i < ARRAY_COUNT(ViewportShowList); ++i)
			{
				ViewportShowList[i].MenuButton = Menu.AddItem(ViewportShowList[i].EntryName, &RC_ViewportShowFlags, i);
				if (i == 1)
					Menu.AddLineBreak();
			}
		}
		Menu.EndPopup();

		Menu.AddPopup(TEXT("Actors\t&A"));
		{
			for (i = 0; i < ARRAY_COUNT(ViewportActorShowList); ++i)
				ViewportActorShowList[i].MenuButton = Menu.AddItem(ViewportActorShowList[i].EntryName, &RC_ViewportActorShowFlags, i);
		}
		Menu.EndPopup();

		Menu.AddLineBreak();

		{
			for (i = 0; i < _PendingViewportcontainer->GViewportRenderers.Num(); ++i)
			{
				INT iSplit = _PendingViewportcontainer->GViewportRenderers(i).InStr(TEXT("."));
				//debugf(TEXT("MENU_ContextMenu adding %ls"), *_PendingViewportcontainer->GViewportRenderers(i).Left(iSplit));
				DrvButtons.AddItem(Menu.AddItem(*_PendingViewportcontainer->GViewportRenderers(i).Left(iSplit), &RC_SetRenderDevice, i));
			}
		}
	}

	{
		INT r = Viewport->Actor->RendMap;
		for (i = 0; i < ARRAY_COUNT(ViewportModesList); ++i)
			ViewportModesList[i].MenuButton->SetChecked(r == ViewportModesList[i].ModeID);

		r = Viewport->Actor->ShowFlags;
		for (i = 0; i < ARRAY_COUNT(ViewportShowList); ++i)
			ViewportShowList[i].MenuButton->SetChecked((r & ViewportShowList[i].ModeID) != 0);

		for (i = 0; i < ARRAY_COUNT(ViewportActorShowList); ++i)
		{
			if(!ViewportActorShowList[i].ModeID)
				ViewportActorShowList[i].MenuButton->SetChecked((r & ViewportActorShowList[i].NotID) == 0);
			else if (!ViewportActorShowList[i].NotID)
				ViewportActorShowList[i].MenuButton->SetChecked((r & ViewportActorShowList[i].ModeID) == ViewportActorShowList[i].ModeID);
			else ViewportActorShowList[i].MenuButton->SetChecked((r & ViewportActorShowList[i].ModeID) == ViewportActorShowList[i].ModeID && (r & ViewportActorShowList[i].NotID) == 0);
		}

		FString DeviceName = Viewport->RenDev->GetClass()->GetPathName();
		for (i = 0; i < DrvButtons.Num(); ++i)
		{
			DrvButtons(i)->SetChecked(DeviceName == *_PendingViewportcontainer->GViewportRenderers(i));
			//debugf(TEXT("Current DeviceName %ls == %ls"), *DeviceName, *_PendingViewportcontainer->GViewportRenderers(i));
		}
	}
	Menu.OpenMenu(Wnd);
	unguard;
}

static void RC_TexturePrefs(INT ID)
{
	switch (ID)
	{
	case 0:
		GBrowserTexture->OnCommand(IDMN_TB_PROPERTIES);
		break;
	case 1:
		GBrowserTexture->OnCommand(IDMN_TB_DELETE);
		break;
	case 2:
		GBrowserTexture->OnCommand(IDMN_TB_RENAME);
		break;
	case 3:
		GBrowserTexture->OnCommand(IDMN_TB_REMOVE);
		break;
	case 4:
		GBrowserTexture->OnCommand(IDMN_TB_EXPORT_BMP);
		break;
	case 5:
		GBrowserTexture->OnCommand(IDMN_TB_EXPORT_PCX);
		break;
	case 6:
		GBrowserTexture->OnCommand(IDMN_TB_EXPORT_PNG);
		break;
	case 7:
		GBrowserTexture->OnCommand(IDMN_TB_EXPORT_DDS);
		break;
	case 8:
		if (GEditor->CurrentTexture)
			appClipboardCopy(GEditor->CurrentTexture->GetPathName());
		else appClipboardCopy(GEditor->CurrentFont->GetPathName());
		break;
	}
}
static void RC_TextureCompression(INT ID)
{
	GBrowserTexture->OnCommand(ID);
}

//=============================================================================================
void MENU_TextureContextMenu(HWND Wnd)
{
	guard(MENU_TextureContextMenu);
	if (GEditor->CurrentTexture)
	{
		static FPersistentRCMenu Menu;
		static FMenuItem* CompressBtn[10], * AddMips, * RemoveMips;

		if (!Menu.bMenuCreated)
		{
			Menu.AddItem(TEXT("Properties\t&P"), &RC_TexturePrefs, 0);
			Menu.AddItem(TEXT("Delete\t&D"), &RC_TexturePrefs, 1);
			Menu.AddItem(TEXT("Rename...\t&R"), &RC_TexturePrefs, 2);
			Menu.AddItem(TEXT("Remove from Level\t&M"), &RC_TexturePrefs, 3);
			Menu.AddLineBreak();
			Menu.AddItem(TEXT("Export to BMP...\t&B"), &RC_TexturePrefs, 4);
			Menu.AddItem(TEXT("Export to PCX...\t&E"), &RC_TexturePrefs, 5);
			Menu.AddItem(TEXT("Export to PNG...\t&N"), &RC_TexturePrefs, 6);
			Menu.AddItem(TEXT("Export to DDS...\t&N"), &RC_TexturePrefs, 7);
			Menu.AddLineBreak();
			Menu.AddItem(TEXT("Copy Object name"), &RC_TexturePrefs, 8);
			Menu.AddLineBreak();

			Menu.AddPopup(TEXT("Compress/Convert to...\t&C"));
			{
				CompressBtn[0] = Menu.AddItem(TEXT("BC1 (DXT1)\t&1"), &RC_TextureCompression, IDMN_TB_SETCOMPRESS_BC1);
				CompressBtn[1] = Menu.AddItem(TEXT("BC2 (DXT3)\t&2"), &RC_TextureCompression, IDMN_TB_SETCOMPRESS_BC2);
				CompressBtn[2] = Menu.AddItem(TEXT("BC3 (DXT5)\t&3"), &RC_TextureCompression, IDMN_TB_SETCOMPRESS_BC3);
				CompressBtn[3] = Menu.AddItem(TEXT("BC4 (Height map)\t&4"), &RC_TextureCompression, IDMN_TB_SETCOMPRESS_BC4);
				CompressBtn[4] = Menu.AddItem(TEXT("BC5 (Normal map)\t&5"), &RC_TextureCompression, IDMN_TB_SETCOMPRESS_BC5);
				CompressBtn[5] = Menu.AddItem(TEXT("BC7 \t&7"), &RC_TextureCompression, IDMN_TB_SETCOMPRESS_BC7);
				CompressBtn[6] = Menu.AddItem(TEXT("Palette (P8)\t&P"), &RC_TextureCompression, IDMN_TB_SETCOMPRESS_P8);
				CompressBtn[7] = Menu.AddItem(TEXT("Monocrome\t&M;"), &RC_TextureCompression, IDMN_TB_SETCOMPRESS_MONO);
				Menu.AddLineBreak();
				CompressBtn[8] = Menu.AddItem(TEXT("RGBA8 (uncompressed)"), &RC_TextureCompression, IDMN_TB_SETCOMPRESS_DECOM);
				CompressBtn[9] = Menu.AddItem(TEXT("BGRA8 (uncompressed)"), &RC_TextureCompression, IDMN_TB_SETCOMPRESS_DECOMBGRA);
			}
			Menu.EndPopup();
			AddMips = Menu.AddItem(TEXT("Add mipmaps"), &RC_TextureCompression, IDMN_TB_SETMIPS_ADD);
			RemoveMips = Menu.AddItem(TEXT("Remove mipmaps"), &RC_TextureCompression, IDMN_TB_SETMIPS_DEL);
		}
		if (GEditor->CurrentTexture->Mips.Num() > 1)
		{
			AddMips->SetDisabled(TRUE);
			RemoveMips->SetDisabled(FALSE);
		}
		else
		{
			AddMips->SetDisabled(GEditor->CurrentTexture->GetClass() != UTexture::StaticClass());
			RemoveMips->SetDisabled(TRUE);
		}
		INT iFormatIndex = INDEX_NONE;
		switch (GEditor->CurrentTexture->Format)
		{
		case TEXF_BC1: iFormatIndex = 0; break;
		case TEXF_BC2: iFormatIndex = 1; break;
		case TEXF_BC3: iFormatIndex = 2; break;
		case TEXF_BC4: iFormatIndex = 3; break;
		case TEXF_BC5: iFormatIndex = 4; break;
		case TEXF_BC7: iFormatIndex = 5; break;
		case TEXF_P8: iFormatIndex = 6; break;
		case TEXF_MONO: iFormatIndex = 7; break;
		case TEXF_RGBA8_: iFormatIndex = 8; break;
		case TEXF_BGRA8: iFormatIndex = 9; break;
		}
		for (INT i = 0; i < 10; ++i)
			CompressBtn[i]->SetChecked(i == iFormatIndex);

		Menu.OpenMenu(Wnd);
	}
	else if (GEditor->CurrentFont)
	{
		static FPersistentRCMenu Menu;

		if (!Menu.bMenuCreated)
		{
			Menu.AddItem(TEXT("Properties\t&P"), &RC_TexturePrefs, 0);
			Menu.AddItem(TEXT("Delete\t&D"), &RC_TexturePrefs, 1);
			Menu.AddItem(TEXT("Rename...\t&R"), &RC_TexturePrefs, 2);
			Menu.AddLineBreak();
			Menu.AddItem(TEXT("Copy Object name"), &RC_TexturePrefs, 8);
		}
		Menu.OpenMenu(Wnd);
	}
	unguard;
}

//=============================================================================================
void MENU_SoundContextMenu(HWND Wnd)
{
	guard(MENU_SoundContextMenu);
	static FPersistentRCMenu Menu;

	if (!Menu.bMenuCreated)
	{
		Menu.AddItem(TEXT("Play\t&P"), &RC_SoundOpts, 0);
		Menu.AddItem(TEXT("Stop\t&S"), &RC_SoundOpts, 1);
		Menu.AddLineBreak();
		Menu.AddItem(TEXT("Rename...\t&R"), &RC_SoundOpts, 2);
		Menu.AddItem(TEXT("Export...\t&E"), &RC_SoundOpts, 3);
		Menu.AddItem(TEXT("Copy Object name"), &RC_SoundOpts, 4);
		Menu.AddLineBreak();

		Menu.AddPopup(TEXT("Compress...\t&C"));
		{
			Menu.AddItem(TEXT("Very low quality (0)"), &RC_SoundOpts, 5);
			Menu.AddItem(TEXT("Low quality (3)"), &RC_SoundOpts, 6);
			Menu.AddItem(TEXT("Medium quality (7)"), &RC_SoundOpts, 7);
			Menu.AddItem(TEXT("High quality (11)"), &RC_SoundOpts, 8);
			Menu.AddLineBreak();
			Menu.AddItem(TEXT("Decompress"), &RC_SoundOpts, 9);
		}
		Menu.EndPopup();
		Menu.AddItem(TEXT("Delete\t&D"), &RC_SoundOpts, 10);
	}
	Menu.OpenMenu(Wnd);
	unguard;
}
void MENU_AnimContextMenu(HWND Wnd, FName AnimName, UMesh* MeshType)
{
	guard(MENU_AnimContextMenu);
	static FPersistentRCMenu Menu;
	static FMenuItem* ItemA, * ItemB, * ItemC;

	if (!Menu.bMenuCreated)
	{
		ItemA = Menu.AddItem(TEXT("Copy Animation name"), &RC_AnimOpts, 0);
		Menu.AddLineBreak();
		ItemB = Menu.AddItem(TEXT("Copy Mesh name"), &RC_AnimOpts, 1);
		ItemC = Menu.AddItem(TEXT("Copy Animation Set name"), &RC_AnimOpts, 2);
	}
	ItemA->SetDisabled(AnimName == NAME_None);
	ItemB->SetDisabled(MeshType == NULL);
	ItemC->SetDisabled(!MeshType || !MeshType->IsA(USkeletalMesh::StaticClass()) || !reinterpret_cast<USkeletalMesh*>(MeshType)->DefaultAnimation);
	Menu.OpenMenu(Wnd);
	unguard;
}

//=============================================================================================
void MENU_VertexMenu(HWND Wnd)
{
	guard(MENU_VertexMenu);
	static FPersistentRCMenu Menu;

	if (!Menu.bMenuCreated)
	{
		CmdMenuList.AddCmdItem(&Menu, TEXT("Snap brush here"), TEXT("PIVOT SNAPPEDMOVE"), TEXT("P"));
		CmdMenuList.AddCmdItem(&Menu, TEXT("Snap vertex to grid"), TEXT("BSP VERTSNAP"), TEXT("N"));
		CmdMenuList.AddCmdItem(&Menu, TEXT("Create new BSP face"), TEXT("BSP CREATEFACE"), TEXT("C"));
		CmdMenuList.AddCmdItem(&Menu, TEXT("Split line"), TEXT("BSP VERTSPLIT"), TEXT("S"));
	}
	Menu.OpenMenu(Wnd);
	unguard;
}
