/*=============================================================================
	BrushBuilder : Properties of a brush builder
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>

RECT GWBBLastPos = { -1, -1, -1, -1 };

class WDlgBrushBuilder : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgBrushBuilder,WDialog,UnrealEd)

	// Variables.
	WButton BuildButton, CancelButton, ResetButton;

	WObjectProperties* pProps;
	UBrushBuilder* Builder;

	FRect OldCR = {};

	// Constructor.
	WDlgBrushBuilder( UObject* InContext, WWindow* InOwnerWindow, UBrushBuilder* InBuilder )
	:	WDialog		( TEXT("Brush Builder"), IDDIALOG_BRUSH_BUILDER, InOwnerWindow )
	,	BuildButton	( this, IDPB_BUILD, FDelegate(this,(TDelegate)&WDlgBrushBuilder::OnBuild) )
	,	CancelButton	( this, IDCANCEL,		FDelegate(this,(TDelegate)&WDlgBrushBuilder::EndDialogFalse) )
	,	ResetButton(this, IDPB_RESET, FDelegate(this, (TDelegate)&WDlgBrushBuilder::OnReset))
	{
		Builder = InBuilder;

		pProps = new WObjectProperties( NAME_None, CPF_Edit, TEXT(""), this, 1 );
		pProps->ShowTreeLines = 1;
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgBrushBuilder::OnInitDialog);
		WDialog::OnInitDialog();

		pProps->OpenChildWindow( IDSC_PROPS );
		SetupPropertyList();

		SetText( *FObjectName(Builder->GetClass()) );

		if( GWBBLastPos.left == -1 )
		{
			POINT pt;
			::GetCursorPos( &pt );
			GWBBLastPos.left = pt.x + 1;
			GWBBLastPos.top = pt.y + 1;
		}

		OldCR = GetClientRect();
		
		RECT Size;
		::GetWindowRect(hWnd, &Size);

		INT Width = Size.right - Size.left;
		INT Height = Size.bottom - Size.top;

		FString Pos;
		if (GConfig->GetString(TEXT("WindowPositions"), *PersistentName, Pos))
		{
			Parse(*Pos, TEXT("XL="), Width);
			Parse(*Pos, TEXT("YL="), Height);
		}

		::SetWindowPos( hWnd, HWND_TOP, GWBBLastPos.left, GWBBLastPos.top, Max(300, Width), Max(110, Height), 0 );

		unguard;
	}
	void SetupPropertyList()
	{
		guard(WDlgBrushBuilder::SetupPropertyList);

		// This is a massive hack.  The fields for the appropriate category are being hand fed
		// into the property window and the link to the object is being set up manually.  This
		// feels wrong in many ways to me, but it works and that's what counts at the moment.
		pProps->Root.Sorted = 0;
		pProps->Root._Objects.AddItem( Builder );
		for( TFieldIterator<UProperty> It(Builder->GetClass()); It; ++It )
			if( It->Category==FName(*FObjectName(Builder->GetClass())) && pProps->Root.AcceptFlags( It->PropertyFlags ) )
				pProps->Root.Children.AddItem( new FPropertyItem( pProps, &(pProps->Root), *It, It->GetFName(), It->Offset, -1 ) );
		pProps->Root.Expand();
		pProps->ResizeList();
		pProps->bAllowForceRefresh = 0;

		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgBrushBuilder::OnDestroy);

		::GetWindowRect( hWnd, &GWBBLastPos );

		WDialog::OnDestroy();

		delete pProps;
		::DestroyWindow( hWnd );

		unguard;
	}
	virtual void DoModeless()
	{
		guard(WDlgBrushBuilder::DoModeless);
		AddWindow(this, TEXT("WDlgBrushBuilder"));
		hWnd = CreateDialogParamA( hInstance, MAKEINTRESOURCEA(IDDIALOG_BRUSH_BUILDER), OwnerWindow?OwnerWindow->hWnd:NULL, (DLGPROC)StaticDlgProc, (LPARAM)this);
		if( !hWnd )
			appGetLastError();

		Show(1);
		unguard;
	}
	void OnReset()
	{
		guard(WDlgBrushBuilder::OnReset);

		UClass* Class = Builder->GetClass();
		FString Temp;
		INT i = 0;
		for (TFieldIterator<UProperty> It(Class); It; ++It)
		{
			if (It->Category == FName(Class->GetName()))
			{
				Temp = TEXT("");
				It->ExportText(0, Temp, &Class->Defaults(0), &Class->Defaults(0), PPF_Localized);
				debugf(TEXT("%ls"), *Temp);
				((FPropertyItem*)pProps->Root.Children(i))->SetValue(*Temp);

				if (i < pProps->Root.Children.Num()) //should never happen, but...
					i++;
			}
		}
		unguard;
	}
	void OnBuild()
	{
		guard(WDlgBrushBuilder::OnBuild);

		// Force all controls to save their values before trying to build the brush.
		for( INT i=0; i<pProps->Root.Children.Num(); i++ )
			((FPropertyItem*)pProps->Root.Children(i))->SendToControl();

		UBOOL GIsSavedScriptableSaved = 1;
		Exchange(GIsScriptable,GIsSavedScriptableSaved);
		Builder->eventBuild();
		Exchange(GIsScriptable,GIsSavedScriptableSaved);
		
		GEditor->Exec(TEXT("LEVEL REDRAW"));

		unguard;
	}
	void OnClose()
	{
		guard(WDlgBrushBuilder::OnClose);
		Show(0);
		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WDlgBrushBuilder::OnSize);
		WDialog::OnSize( Flags, NewX, NewY );
		PositionChildControls();
		unguard;
	}
	void PositionChildControls()
	{
		guard(WDlgBrushBuilder::PositionChildControls);
		
		FRect CR = GetClientRect();
		FPoint Diff(CR.Width() - OldCR.Width(), CR.Height() - OldCR.Height());

		PositionChildControl(hWnd, GetDlgItem(hWnd, IDLB_GROUP), 0, 0, Diff.X, Diff.Y);
		PositionChildControl(hWnd, GetDlgItem(hWnd, IDSC_PROPS), 0, 0, Diff.X, Diff.Y);
		PositionChildControl(GetDlgItem(hWnd, IDSC_PROPS), pProps->hWnd, 0, 0, Diff.X, Diff.Y);
		PositionChildControl(hWnd, GetDlgItem(hWnd, IDPB_BUILD), Diff.X, 0, 0, 0);
		PositionChildControl(hWnd, GetDlgItem(hWnd, IDPB_RESET), Diff.X, 0, 0, 0);
		PositionChildControl(hWnd, GetDlgItem(hWnd, IDCANCEL), Diff.X, 0, 0, 0);

		::InvalidateRect( hWnd, NULL, TRUE );
		::InvalidateRect( pProps->List.hWnd, NULL, TRUE );

		OldCR = CR;
		unguard;
	}
	void PositionChildControl(HWND Owner, HWND hWndItem, int dLeft, int dTop, int dRight, int dBottom)
	{
		guard(WDlgBrushBuilder::PositionChildControl);
		if (!dLeft && !dTop && !dRight && !dBottom)
			return;
		RECT R;
		::GetWindowRect(hWndItem, &R);
		MapWindowPoints(NULL, Owner, (LPPOINT)&R, 2);
		R.left += dLeft;
		R.top += dTop;
		R.right += dLeft + dRight;
		R.bottom += dTop + dBottom;
		::MoveWindow(hWndItem, R.left, R.top, R.right - R.left, R.bottom - R.top, TRUE);
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
