/*=============================================================================
	ScaleMap : Allows for the scaling of selected actors
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Buggie

    Work-in-progress todo's:

=============================================================================*/

class WDlgScaleMap : public WDialogTool
{
	DECLARE_WINDOWCLASS(WDlgScaleMap,WDialogTool,UnrealEd)

	// Variables.
	WButton OKButton, CloseButton;
	WEdit XEdit, YEdit, ZEdit;
	WToolTip* ToolTipCtrl{};

	// Constructor.
	WDlgScaleMap( UObject* InContext, WWindow* InOwnerWindow )
	:	WDialogTool		( TEXT("ScaleMap"), IDDIALOG_SCALE_MAP, InOwnerWindow )
	,	OKButton		( this, IDOK,			FDelegate(this,(TDelegate)&WDlgScaleMap::OnOK) )
	,	CloseButton		( this, IDPB_CLOSE,		FDelegate(this,(TDelegate)&WDlgScaleMap::OnClose) )
	,	XEdit			( this, IDEC_FACTOR_X )
	,	YEdit			( this, IDEC_FACTOR_Y )
	,	ZEdit			( this, IDEC_FACTOR_Z )
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgScaleMap::OnInitDialog);
		WDialogTool::OnInitDialog();

		XEdit.SetText(TEXT("1.0"));
		YEdit.SetText(TEXT("1.0"));
		ZEdit.SetText(TEXT("1.0"));

		INT Checked[] = {
			IDCK_BRUSHES,
			IDRB_PERMANENT,
			IDRB_LIGHT_BRIGHTNESS_X,
			IDCK_LIGHT_RADIUS,
			IDRB_LIGHT_RADIUS_X,
			IDCK_SPRITES,
			IDRB_SPRITES_X,
			IDCK_MESHES,
			IDRB_MESHES_X,
			IDCK_LOCATIONS,
			IDCK_COLLISION_RADIUS,
			IDRB_COLLISION_RADIUS_X,
			IDCK_COLLISION_HEIGHT,
			IDCK_MOVERS_MOVEMENTS,
			IDCK_MOVERS_TIME,
			IDRB_MOVERS_TIME_MOVEMENT,
			IDCK_JUMPERS_JUMP_Z,
			IDCK_KICKERS_KICK_VELOCITY,
			IDCK_LOCATION_ID_RADIUS,
			IDRB_LOCATION_ID_RADIUS_X,
			IDCK_INTERMEDIATE_PATH_NODES,
			IDCK_SOUND_RADIUS,
			IDRB_SOUND_RADIUS_X,
			IDRB_SOUND_VOLUME_X,
		};
		for (INT i = 0; i < ARRAY_COUNT(Checked); i++)
			SendMessageW(GetDlgItem(hWnd, Checked[i]), BM_SETCHECK, (WPARAM)TRUE, 0);

		ToolTipCtrl = new WToolTip(this);
		ToolTipCtrl->OpenWindow();

		ToolTipCtrl->AddTool(GetDlgItem(hWnd, IDRB_PERMANENT), TEXT("Best mode"), IDRB_PERMANENT);
		ToolTipCtrl->AddTool(GetDlgItem(hWnd, IDRB_MAIN_SCALE), TEXT("Movers collision is broken (need transform permanently and recreate), can't properly scale brushes rotated from 90 degree step"), IDRB_MAIN_SCALE);
		ToolTipCtrl->AddTool(GetDlgItem(hWnd, IDRB_POST_SCALE), TEXT("Movers collision is broken, distorted when moving with rotation"), IDRB_POST_SCALE);

		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgScaleMap::OnDestroy);
		delete ToolTipCtrl;
		WDialogTool::OnDestroy();
		unguard;
	}
	virtual void DoModeless()
	{
		guard(WDlgScaleMap::DoModeless);
		AddWindow(this, TEXT("WDlgScaleMap"));
		hWnd = CreateDialogParamA( hInstance, MAKEINTRESOURCEA(IDDIALOG_SCALE_MAP), OwnerWindow?OwnerWindow->hWnd:NULL, (DLGPROC)StaticDlgProc, (LPARAM)this);
		if( !hWnd )
			appGetLastError();
		Show(1);
		unguard;
	}
	void OnClose()
	{
		guard(WDlgScaleMap::OnClose);
		Show(0);
		unguard;
	}
	void OnOK()
	{
		guard(WDlgScaleMap::OnTagEditChange);
		DOUBLE Factor[] = {
			appAtof(*XEdit.GetText()),
			appAtof(*YEdit.GetText()),
			appAtof(*ZEdit.GetText()),
		};
		DOUBLE AbsFactor[ARRAY_COUNT(Factor)], SqrtFactor[ARRAY_COUNT(Factor)];
		for (INT i = 0; i < ARRAY_COUNT(Factor); i++)
		{
			AbsFactor[i] = Factor[i] < 0.0f ? -Factor[i] : Factor[i];
			SqrtFactor[i] = Factor[i] < 0.0f ? -appSqrt(AbsFactor[i]) : appSqrt(AbsFactor[i]);
		}
		const INT iX = 0;
		const INT iY = 1;
		const INT iZ = 2;
		DOUBLE& X = Factor[iX];
		DOUBLE& Y = Factor[iY];
		DOUBLE& Z = Factor[iZ];
		DOUBLE& AbsX = AbsFactor[iX];
		DOUBLE& AbsY = AbsFactor[iY];
		DOUBLE& AbsZ = AbsFactor[iZ];

		BOOL bAllSame = X == Y && Y == Z;

		if (bAllSame && X == 0.0f)
		{
			::MessageBox(hWnd, TEXT("A scale factor must not be zero."), *GetText(), MB_OK);
			return;
		}

		if (bAllSame && X == 1.0f)
		{
			::MessageBox(hWnd, TEXT("A scale factor of 1 will not scale anything.\n"
				"To make selected actors smaller, enter a number between 0 and 1,\n"
				"to make its larger, enter a value greater than 1."), *GetText(), MB_OK);
			return;
		}

		TArray<BOOL> Checked;
		for (HWND h = GetWindow(hWnd, GW_CHILD); h; h = GetWindow(h, GW_HWNDNEXT))
		{
			int ControlId = GetDlgCtrlID(h);
			if (ControlId > 0)
			{
				if (ControlId >= Checked.Num())
					Checked.AddZeroed(ControlId + 1 - Checked.Num());
				Checked(ControlId) = SendMessageW(h, BM_GETCHECK, 0, 0) == BST_CHECKED;
			}
		}

		INT Selected = 0;
		BOOL bReverse = X*Y*Z < 0;
		if (bReverse)
			bAllSame = FALSE;

		for( FObjectIterator It; It; ++It )
			It->ClearFlags( RF_TagImp | RF_TagExp );

		GEditor->Trans->Begin( TEXT("Map Scaling") );

		GWarn->BeginSlowTask( TEXT("Scaling"), 1, 0);
		GEditor->NoteActorMovement(GEditor->Level);

		for (INT i = 0; i < GEditor->Level->Actors.Num(); i++)
		{
			AActor* Actor = GEditor->Level->Actors(i);
			if (Actor && Actor->bSelected && Cast<AInventory>(Actor) && Cast<AInventory>(Actor)->myMarker && 
				!Cast<AInventory>(Actor)->myMarker->bSelected)
				Cast<AInventory>(Actor)->myMarker->bSelected = TRUE;
		}

		for (INT i = 0; i < GEditor->Level->Actors.Num(); i++)
		{
			AActor* Actor = GEditor->Level->Actors(i);
			if (Actor && Actor->bSelected && !(Actor->GetFlags() & RF_TagImp))
			{
				Actor->SetFlags(RF_TagImp);
				Selected++;
				GWarn->StatusUpdatef( 0, i, TEXT("Scaling") );
				Actor->Modify();
				FRotator OldRotation = Actor->Rotation;
				BOOL bNotZeroOldRotation = !bAllSame || !OldRotation.IsZero();
				BOOL bIsBrush = Actor->IsBrush();
				BOOL bIsMovingBrush = bIsBrush && !Actor->bStatic;
				if (Checked(IDCK_LOCATIONS))
				{
					Actor->Location.X *= X;
					Actor->Location.Y *= Y;
					Actor->Location.Z *= Z;
					if (bNotZeroOldRotation && !Actor->IsStaticBrush())
						Actor->Rotation = (GMath.UnitCoords / OldRotation * FScale(FVector(X, Y, Z))).OrthoRotation();
				}
				if (bIsBrush)
				{
					ABrush* Brush = Cast<ABrush>(Actor);
					if (Checked(IDCK_BRUSHES))
					{
						if (Checked(IDRB_PERMANENT))
						{
							FVector OldPrePivot = Brush->PrePivot;
							Brush->PrePivot.X *= X;
							Brush->PrePivot.Y *= Y;
							Brush->PrePivot.Z *= Z;
							if (!(Brush->Brush->GetFlags() & RF_TagImp))
							{
								Brush->Brush->SetFlags(RF_TagImp);
								FScale Scale(FVector(X, Y, Z));
								FCoords Coords = GMath.UnitCoords / -Brush->PrePivot / Brush->MainScale / OldRotation * Scale * 
									Brush->Rotation * Brush->MainScale * -OldPrePivot;
								FCoords UnCoords = GMath.UnitCoords / Brush->MainScale / OldRotation / Scale * 
									Brush->Rotation * Brush->MainScale;

								Brush->Brush->Polys->Element.ModifyAllItems();
								for (INT poly = 0; poly < Brush->Brush->Polys->Element.Num(); poly++)
								{
									FPoly* Poly = &(Brush->Brush->Polys->Element(poly));
									
									Poly->TextureU = Poly->TextureU.TransformVectorBy(UnCoords);
									Poly->TextureV = Poly->TextureV.TransformVectorBy(UnCoords);

									Poly->Base = Poly->Base.TransformPointBy(Coords);

									for( INT vtx = 0 ; vtx < Poly->NumVertices ; vtx++ )
										Poly->Vertex[vtx] = Poly->Vertex[vtx].TransformPointBy(Coords);

									Poly->CalcNormal();
									if (bReverse)
										Poly->Reverse();
								}
							}
						}
						else if (Checked(IDRB_POST_SCALE))
						{
							if (bNotZeroOldRotation && bIsMovingBrush)
								Brush->Rotation = OldRotation;

							Brush->PostScale.Scale.X *= X;
							Brush->PostScale.Scale.Y *= Y;
							Brush->PostScale.Scale.Z *= Z;
						}
						else // IDRB_MAIN_SCALE
						{
							if (bNotZeroOldRotation)
							{
								FScale Scale(FVector(X, Y, Z));
								FCoords Coords = GMath.UnitCoords / OldRotation * Scale * Brush->Rotation;
								FVector Dir = Coords.XAxis + Coords.YAxis + Coords.ZAxis;
								Brush->MainScale.Scale *= Dir;
							}
							else
							{
								Brush->MainScale.Scale.X *= X;
								Brush->MainScale.Scale.Y *= Y;
								Brush->MainScale.Scale.Z *= Z;
							}
						}
						Brush->Brush->BuildBound();
						if (bIsMovingBrush)
							GEditor->csgPrepMovingBrush(Brush);
					}
					if (bIsMovingBrush)
					{
						AMover* Mover = Cast<AMover>(Brush);
						if (Checked(IDCK_MOVERS_TIME))
						{
							if (!Checked(IDRB_MOVERS_TIME_MOVEMENT))
								Mover->MoveTime *= AbsFactor[Checked(IDRB_MOVERS_TIME_X) ? iX : (Checked(IDRB_MOVERS_TIME_Y) ? iY : iZ)];
							else
							{
								INT Pos = Max(0, Min((INT)ARRAY_COUNT(Mover->KeyPos), (INT)Mover->KeyNum));
								FVector Move = Mover->KeyPos[Pos];
								FLOAT OldDist = Move.Size();
								Move.X *= X;
								Move.Y *= Y;
								Move.Z *= Z;
								FLOAT NewDist = Move.Size();
								if (OldDist != 0 && NewDist != OldDist)
									Mover->MoveTime *= NewDist/OldDist;
							}
						}
						if (Checked(IDCK_MOVERS_MOVEMENTS))
						{
							for (int i = 0; i < ARRAY_COUNT(Mover->KeyPos); i++)
							{
								Mover->KeyPos[i].X *= X;
								Mover->KeyPos[i].Y *= Y;
								Mover->KeyPos[i].Z *= Z;
							}
							if (!bAllSame && !Checked(IDRB_POST_SCALE))
								for (int i = 0; i < ARRAY_COUNT(Mover->KeyRot); i++)
									if (!Mover->KeyRot[i].IsZero())
										Mover->KeyRot[i] = (GMath.UnitCoords / (Mover->KeyRot[i] + OldRotation) * FScale(FVector(X, Y, Z))).OrthoRotation() - 
											Mover->Rotation;
						}
					}
				}
				else if (Cast<AInventory>(Actor) || Cast<ANavigationPoint>(Actor))
				{ // skip this one
				}
				else if (Checked(IDCK_MESHES) && Actor->DrawType == DT_Mesh)
				{
					Actor->DrawScale *= AbsFactor[Checked(IDRB_MESHES_X) ? iX : (Checked(IDRB_MESHES_Y) ? iY : iZ)];
				}
				else if (Checked(IDCK_SPRITES) && Actor->DrawType == DT_Sprite && !Actor->bHidden)
				{
					Actor->DrawScale *= AbsFactor[Checked(IDRB_SPRITES_X) ? iX : (Checked(IDRB_SPRITES_Y) ? iY : iZ)];
				}
				if (!Cast<APawn>(Actor) && !Cast<AInventory>(Actor) && !Cast<ANavigationPoint>(Actor))
				{
					if (Checked(IDCK_COLLISION_RADIUS))
						Actor->CollisionRadius *= AbsFactor[Checked(IDRB_COLLISION_RADIUS_X) ? iX : iY];

					if (Checked(IDCK_COLLISION_HEIGHT))
						Actor->CollisionHeight *= AbsZ;
				}

				if (Actor->LightType != LT_None)
				{
					if (Checked(IDCK_LIGHT_BRIGHTNESS))
						Actor->LightBrightness = Min(255, appRound(Actor->LightBrightness*AbsFactor[Checked(IDRB_LIGHT_BRIGHTNESS_X) ? iX : 
							(Checked(IDRB_LIGHT_BRIGHTNESS_Y) ? iY : iZ)]));
					if (Checked(IDCK_LIGHT_RADIUS))
						Actor->LightRadius = Min(255, appRound(Actor->LightRadius*AbsFactor[Checked(IDRB_LIGHT_RADIUS_X) ? iX : 
							(Checked(IDRB_LIGHT_RADIUS_Y) ? iY : iZ)]));
				}

				if (Checked(IDCK_SOUND_RADIUS))
					Actor->SoundRadius = Min(255, appRound(Actor->SoundRadius*AbsFactor[Checked(IDRB_SOUND_RADIUS_X) ? iX : 
						(Checked(IDRB_SOUND_RADIUS_Y) ? iY : iZ)]));

				if (Checked(IDCK_SOUND_VOLUME))
					Actor->SoundVolume = Min(255, appRound(Actor->SoundVolume*AbsFactor[Checked(IDRB_SOUND_VOLUME_X) ? iX : 
						(Checked(IDRB_SOUND_VOLUME_Y) ? iY : iZ)]));

				if (Cast<ATriggers>(Actor))
				{
					if (Checked(IDCK_JUMPERS_JUMP_Z))
					{
						UField* Field = Actor->FindObjectField(TEXT("JumpZ"));
						if (Field)
						{
							UFloatProperty* Property = Cast<UFloatProperty>(Field);
							if (Property)
								*(FLOAT*)((BYTE*)Actor + Property->Offset) *= SqrtFactor[iZ];
						}
					}
					if (Checked(IDCK_KICKERS_KICK_VELOCITY))
					{
						UField* Field = Actor->FindObjectField(TEXT("KickVelocity"));
						if (Field)
						{
							UStructProperty* Property = Cast<UStructProperty>(Field);
							if (Property && Property->Struct->GetFName() == NAME_Vector)
							{
								FVector& V = *(FVector*)((BYTE*)Actor + Property->Offset);
								V.X *= SqrtFactor[iX];
								V.Y *= SqrtFactor[iY];
								V.Z *= SqrtFactor[iZ];
							}
						}
					}
				}

				if (Cast<Alocationid>(Actor) && Checked(IDCK_LOCATION_ID_RADIUS))
					Cast<Alocationid>(Actor)->Radius *= AbsFactor[Checked(IDRB_LOCATION_ID_RADIUS_X) ? iX : (Checked(IDRB_LOCATION_ID_RADIUS_Y) ? iY : iZ)];

				Actor->PostEditChange();
			}
		}
		if (Checked(IDCK_INTERMEDIATE_PATH_NODES))
		{
			const FLOAT MAX_DIST = 800.0f;
			INT LastActor = GEditor->Level->Actors.Num();
			for (INT j = GEditor->Level->ReachSpecs.Num() - 1; j >= 0; j--)
			{
				FReachSpec& Check = GEditor->Level->ReachSpecs(j);
				if (Check.bPruned || !(Check.reachFlags & (APawn::R_WALK | APawn::R_FLY | APawn::R_SWIM | APawn::R_JUMP)) || !Check.Start || !Check.Start->bSelected || !Check.End || !Check.End->bSelected)
					continue;
				FVector Dir = Check.End->Location - Check.Start->Location;
				FLOAT Dist = Dir.Size();
				if (Dist <= MAX_DIST)
					continue;
				FName NAME_ScaleMap(TEXT("ScaleMap"));
				FRotator Rot(0, 0, 0);
				INT Parts = (Dist - 1)/MAX_DIST + 1;
				FLOAT Step = Dist/Parts;
				Dir.Normalize();
				for (INT k = 0; k < Parts; k++)
				{
					FVector Pos = Check.Start->Location + Dir*Step;
					BOOL bFound = FALSE;
					for (INT i = LastActor; i < GEditor->Level->Actors.Num(); i++)
					{
						AActor* Actor = GEditor->Level->Actors(i);
						if (Actor && (Pos - Actor->Location).Size() <= 2*Actor->CollisionRadius)
						{
							bFound = TRUE;
							break;
						}
					}
					if (bFound)
						continue;
					AActor* Spawned = GEditor->Level->SpawnActor
					(
						APathNode::StaticClass(),
						NAME_None,
						NULL,
						NULL,
						Pos,
						Rot,
						NULL,
						TRUE
					);
					if (Spawned)
						Spawned->Group = NAME_ScaleMap;
				}
			}
		}
		GWarn->EndSlowTask();
		
		GEditor->Trans->End();

		for( FObjectIterator It; It; ++It )
			It->ClearFlags( RF_TagImp | RF_TagExp );

		if (!Selected)
		{
			::MessageBox(hWnd, TEXT("No actors is selected on the map. Nothing to scale."), *GetText(), MB_OK);
			return;
		}

		GEditor->NoteSelectionChange( GEditor->Level );
		GEditor->RedrawLevel( GEditor->Level );

		::MessageBox(hWnd, *(FString::Printf(TEXT("Scaled %d actors."), Selected)), *GetText(), MB_OK);

		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
