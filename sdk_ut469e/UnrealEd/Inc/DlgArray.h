/*=============================================================================
	Array : Allows spawn duplicates for selected actor with desired step changes
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Buggie

    Work-in-progress todo's:

=============================================================================*/

class WDlgArray : public WDialogTool
{
	DECLARE_WINDOWCLASS(WDlgArray,WDialogTool,UnrealEd)

	// Variables.
	WButton OKButton, CloseButton;
	WEdit MoveX;
	WEdit MoveY;
	WEdit MoveZ;
	WEdit RotateX;
	WEdit RotateY;
	WEdit RotateZ;
	WEdit ScaleX;
	WEdit ScaleY;
	WEdit ScaleZ;
	WEdit Duplicates;

	// Constructor.
	WDlgArray( UObject* InContext, WWindow* InOwnerWindow )
	:	WDialogTool		( TEXT("Array"), IDDIALOG_ARRAY, InOwnerWindow )
	,	OKButton		( this, IDOK,			FDelegate(this,(TDelegate)&WDlgArray::OnOK) )
	,	CloseButton		( this, IDPB_CLOSE,		FDelegate(this,(TDelegate)&WDlgArray::OnClose) )
	,	MoveX			( this, IDEC_MOVE_X )
	,	MoveY			( this, IDEC_MOVE_Y )
	,	MoveZ			( this, IDEC_MOVE_Z )
	,	RotateX			( this, IDEC_ROTATE_X )
	,	RotateY			( this, IDEC_ROTATE_Y )
	,	RotateZ			( this, IDEC_ROTATE_Z )
	,	ScaleX			( this, IDEC_SCALE_X )
	,	ScaleY			( this, IDEC_SCALE_Y )
	,	ScaleZ			( this, IDEC_SCALE_Z )
	,	Duplicates		( this, IDEC_DUPLICATES )
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgArray::OnInitDialog);
		WDialogTool::OnInitDialog();

		MoveX.SetText(TEXT("0"));
		MoveY.SetText(TEXT("0"));
		MoveZ.SetText(TEXT("0"));
		RotateX.SetText(TEXT("0"));
		RotateY.SetText(TEXT("0"));
		RotateZ.SetText(TEXT("0"));
		ScaleX.SetText(TEXT("1"));
		ScaleY.SetText(TEXT("1"));
		ScaleZ.SetText(TEXT("1"));
		Duplicates.SetText(TEXT("1"));

		unguard;
	}
	virtual void DoModeless(UBOOL bShow = TRUE)
	{
		guard(WDlgArray::DoModeless);
		AddWindow(this, TEXT("WDlgArray"));
		hWnd = CreateDialogParamW( hInstance, MAKEINTRESOURCEW(IDDIALOG_ARRAY), OwnerWindow?OwnerWindow->hWnd:NULL, (DLGPROC)StaticDlgProc, (LPARAM)this);
		if( !hWnd )
			appGetLastError();
		Show(bShow);
		unguard;
	}
	void OnClose()
	{
		guard(WDlgArray::OnClose);
		Show(0);
		unguard;
	}
	void OnOK()
	{
		guard(WDlgArray::OnOK);

		FVector LocStep(appAtof(*MoveX.GetText()), appAtof(*MoveY.GetText()), appAtof(*MoveZ.GetText()));
		FRotator RotStep(appAtoi(*RotateX.GetText()), appAtoi(*RotateY.GetText()), appAtoi(*RotateZ.GetText()));
		FVector ScaleStep(appAtof(*ScaleX.GetText()), appAtof(*ScaleY.GetText()), appAtof(*ScaleZ.GetText()));
		DOUBLE DrawScaleStep = (Abs(ScaleStep.X) + Abs(ScaleStep.Y) + Abs(ScaleStep.Z))/3;
		BOOL bReverse = ScaleStep.X*ScaleStep.Y*ScaleStep.Z < 0;
		INT Dups = appAtoi(*Duplicates.GetText());

		if (ScaleStep.X == 0.0f || ScaleStep.Y == 0.0f || ScaleStep.Z == 0.0f)
		{
			::MessageBox(hWnd, TEXT("A scale factor must not be zero."), *GetText(), MB_OK);
			return;
		}

		GEditor->Trans->Begin( TEXT("Array") );
		GEditor->Level->Modify();

		for (INT j = Dups <= 0 ? Dups - 1 : 0; j < Dups; j++)
		{
			INT MaxActor = Dups <= 0 ? 0 : GEditor->Level->Actors.Num();
			if (Dups > 0)
				GEditor->edactDuplicateSelected(GEditor->Level, FVector(0.f, 0.f, 0.f));
			for (INT i = MaxActor; i < GEditor->Level->Actors.Num(); i++)
			{
				AActor* Actor = GEditor->Level->Actors(i);
				if (Actor && Actor->bSelected && Actor != GEditor->Level->Brush() && (Actor->GetFlags() & RF_Transactional))
				{
					Actor->Modify();
					FRotator OldRotation = Actor->Rotation;
					Actor->Location += (Dups != -1 ? LocStep : LocStep.TransformVectorBy(GMath.UnitCoords * Actor->Rotation));
					Actor->Rotation += RotStep;
					if (!Actor->IsBrush())
						Actor->DrawScale *= DrawScaleStep;
					else
					{
						ABrush* Brush = Cast<ABrush>(Actor);
						FScale Scale(ScaleStep);
						FCoords Coords = GMath.UnitCoords / -Brush->PrePivot / Brush->MainScale / OldRotation * Scale * 
							Brush->Rotation * Brush->MainScale * -Brush->PrePivot;
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
			}
		}

		GEditor->Trans->End();

		GEditor->NoteSelectionChange( GEditor->Level );
		GEditor->RedrawLevel( GEditor->Level );

		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
