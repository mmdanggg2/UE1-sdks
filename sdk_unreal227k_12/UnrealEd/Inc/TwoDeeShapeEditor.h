/*=============================================================================
	TwoDeeShapeEditor : 2D Shape Editor conversion from VB code
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall
		* Added pieces of code from UDelaunay project
		  - arbitrary shape triangulation code
		  - support for bezier segments

    Work-in-progress todo's:
	 - "Extrude to Bevel" should extrude towards the origin point, not 0,0

=============================================================================*/

#include <stdio.h>
#include <math.h>
extern FString GLastDir[eLASTDIR_MAX];

int GGridSize = 16;
FLOAT GGridSizeZoom = 16.f;
int GDefaultDetailLevel = 5;

// --------------------------------------------------------------
//
// EXTRUDE Dialog
//
// --------------------------------------------------------------

int GExtrudeDepth = 256;

class WDlgExtrude : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgExtrude,WDialog,UnrealEd)

	// Variables.
	WButton OkButton;
	WButton CancelButton;
	WEdit DepthEdit;

	int Depth;

	// Constructor.
	WDlgExtrude( UObject* InContext, WWindow* InOwnerWindow )
		: WDialog(TEXT("Extrude"), IDDIALOG_2DShapeEditor_Extrude, InOwnerWindow)
		, OkButton(this, IDOK, FDelegate(this, (TDelegate)&WDlgExtrude::OnOk))
		, CancelButton(this, IDCANCEL, FDelegate(this, (TDelegate)&WDlgExtrude::EndDialogFalse))
		, DepthEdit(this, IDEC_DEPTH)
		, Depth(0)
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgExtrude::OnInitDialog);
		WDialog::OnInitDialog();

		DepthEdit.SetText( *(FString::Printf(TEXT("%d"), GExtrudeDepth) ) );
		::SetFocus( DepthEdit.hWnd );

		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgExtrude::OnDestroy);
		WDialog::OnDestroy();
		unguard;
	}
	void OnClose()
	{
		Show(0);
	}
	INT OnSetCursor()
	{
		guard(WDlgExtrude::OnSetCursor);
		WWindow::OnSetCursor();
		SetCursor(LoadCursorW(NULL, IDC_ARROW));
		return 0;
		unguard;
	}
	virtual int DoModal()
	{
		guard(WDlgExtrude::DoModal);
		return WDialog::DoModal( hInstance );
		unguard;
	}
	void OnOk()
	{
		guard(WDlgExtrude::OnOk);

		Depth = appAtoi( *DepthEdit.GetText() );

		if( Depth <= 0 )
		{
			appMsgf( TEXT("Invalid input.") );
			return;
		}

		GExtrudeDepth = Depth;

		EndDialogTrue();
		unguard;
	}
};

// --------------------------------------------------------------
//
// EXTRUDETOPOINT Dialog
//
// --------------------------------------------------------------

int GExtrudeToPointDepth = 256;

class WDlgExtrudeToPoint : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgExtrudeToPoint,WDialog,UnrealEd)

	// Variables.
	WButton OkButton;
	WButton CancelButton;
	WEdit DepthEdit;

	int Depth;

	// Constructor.
	WDlgExtrudeToPoint( UObject* InContext, WWindow* InOwnerWindow )
		: WDialog(TEXT("ExtrudeToPoint"), IDDIALOG_2DShapeEditor_ExtrudeToPoint, InOwnerWindow)
		, OkButton(this, IDOK, FDelegate(this, (TDelegate)&WDlgExtrudeToPoint::OnOk))
		, CancelButton(this, IDCANCEL, FDelegate(this, (TDelegate)&WDlgExtrudeToPoint::EndDialogFalse))
		, DepthEdit(this, IDEC_DEPTH)
		, Depth(0)
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgExtrudeToPoint::OnInitDialog);
		WDialog::OnInitDialog();

		DepthEdit.SetText( *(FString::Printf(TEXT("%d"), GExtrudeToPointDepth) ) );
		::SetFocus( DepthEdit.hWnd );

		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgExtrudeToPoint::OnDestroy);
		WDialog::OnDestroy();
		unguard;
	}
	void OnClose()
	{
		guard(WDlgExtrudeToPoint::OnClose);
		Show(0);
		unguard;
	}
	INT OnSetCursor()
	{
		guard(WDlgExtrudeToPoint::OnSetCursor);
		WWindow::OnSetCursor();
		SetCursor(LoadCursorW(NULL, IDC_ARROW));
		return 0;
		unguard;
	}
	virtual int DoModal()
	{
		guard(WDlgExtrudeToPoint::DoModal);
		return WDialog::DoModal( hInstance );
		unguard;
	}
	void OnOk()
	{
		guard(WDlgExtrudeToPoint::OnOk);

		Depth = appAtoi( *DepthEdit.GetText() );

		if( Depth <= 0 )
		{
			appMsgf( TEXT("Invalid input.") );
			return;
		}

		GExtrudeToPointDepth = Depth;

		EndDialogTrue();
		unguard;
	}
};

// --------------------------------------------------------------
//
// EXTRUDETOBEVEL Dialog
//
// --------------------------------------------------------------

int GExtrudeToBevelDepth = 128, GExtrudeToBevelCapHeight = 32;

class WDlgExtrudeToBevel : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgExtrudeToBevel,WDialog,UnrealEd)

	// Variables.
	WButton OkButton;
	WButton CancelButton;
	WEdit DepthEdit, CapHeightEdit;

	int Depth, CapHeight;

	// Constructor.
	WDlgExtrudeToBevel( UObject* InContext, WWindow* InOwnerWindow )
		: WDialog(TEXT("ExtrudeToBevel"), IDDIALOG_2DShapeEditor_ExtrudeToBevel, InOwnerWindow)
		, OkButton(this, IDOK, FDelegate(this, (TDelegate)&WDlgExtrudeToBevel::OnOk))
		, CancelButton(this, IDCANCEL, FDelegate(this, (TDelegate)&WDlgExtrudeToBevel::EndDialogFalse))
		, DepthEdit(this, IDEC_DEPTH)
		, CapHeightEdit(this, IDEC_CAP_HEIGHT)
		, Depth(0)
		, CapHeight(0)
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgExtrudeToBevel::OnInitDialog);
		WDialog::OnInitDialog();

		DepthEdit.SetText( *(FString::Printf(TEXT("%d"), GExtrudeToBevelDepth) ) );
		CapHeightEdit.SetText( *(FString::Printf(TEXT("%d"), GExtrudeToBevelCapHeight) ) );
		::SetFocus( DepthEdit.hWnd );

		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgExtrudeToBevel::OnDestroy);
		WDialog::OnDestroy();
		unguard;
	}
	void OnClose()
	{
		guard(WDlgExtrudeToBevel::OnClose);
		Show(0);
		unguard;
	}
	INT OnSetCursor()
	{
		guard(WDlgExtrudeToBevel::OnSetCursor);
		WWindow::OnSetCursor();
		SetCursor(LoadCursorW(NULL, IDC_ARROW));
		return 0;
		unguard;
	}
	virtual int DoModal()
	{
		guard(WDlgExtrudeToBevel::DoModal);
		return WDialog::DoModal( hInstance );
		unguard;
	}
	void OnOk()
	{
		guard(WDlgExtrudeToBevel::OnOk);

		Depth = appAtoi( *DepthEdit.GetText() );
		CapHeight = appAtoi( *CapHeightEdit.GetText() );

		if( Depth <= 0 || CapHeight <= 0 )
		{
			appMsgf( TEXT("Invalid input.") );
			return;
		}

		GExtrudeToBevelDepth = Depth;
		GExtrudeToBevelCapHeight = CapHeight;

		EndDialogTrue();
		unguard;
	}
};

// --------------------------------------------------------------
//
// REVOLVE Dialog
//
// --------------------------------------------------------------

int GRevolveTotalSides = 8, GRevolveSides = 4;

class WDlgRevolve : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgRevolve,WDialog,UnrealEd)

	// Variables.
	WButton OkButton;
	WButton CancelButton;
	WEdit TotalSidesEdit;
	WEdit SidesEdit;

	int TotalSides, Sides;

	// Constructor.
	WDlgRevolve( UObject* InContext, WWindow* InOwnerWindow )
		: WDialog(TEXT("Revolve"), IDDIALOG_2DShapeEditor_Revolve, InOwnerWindow)
		, OkButton(this, IDOK, FDelegate(this, (TDelegate)&WDlgRevolve::OnOk))
		, CancelButton(this, IDCANCEL, FDelegate(this, (TDelegate)&WDlgRevolve::EndDialogFalse))
		, TotalSidesEdit(this, IDEC_TOTAL_SIDES)
		, SidesEdit(this, IDEC_SIDES)
		, TotalSides(0)
		, Sides(0)
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgRevolve::OnInitDialog);
		WDialog::OnInitDialog();

		TotalSidesEdit.SetText( *(FString::Printf(TEXT("%d"), GRevolveTotalSides) ) );
		::SetFocus( TotalSidesEdit.hWnd );
		SidesEdit.SetText( *(FString::Printf(TEXT("%d"), GRevolveSides) ) );

		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgRevolve::OnDestroy);
		WDialog::OnDestroy();
		unguard;
	}
	void OnClose()
	{
		guard(WDlgRevolve::OnClose);
		Show(0);
		unguard;
	}
	virtual int DoModal()
	{
		guard(WDlgRevolve::DoModal);
		return WDialog::DoModal( hInstance );
		unguard;
	}
	void OnOk()
	{
		guard(WDlgRevolve::OnOk);

		TotalSides = appAtoi( *TotalSidesEdit.GetText() );
		Sides = appAtoi( *SidesEdit.GetText() );

		if( TotalSides < 1
				|| Sides > TotalSides )
		{
			appMsgf( TEXT("Invalid input.") );
			return;
		}

		GRevolveTotalSides = TotalSides;
		GRevolveSides = Sides;

		EndDialogTrue();
		unguard;
	}
};

// --------------------------------------------------------------
//
// CUSTOM DETAIL Dialog
//
// --------------------------------------------------------------

class WDlgCustomDetail : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgCustomDetail,WDialog,UnrealEd)

	WButton OKButton, CancelButton;
	WEdit ValueEdit;

	// Variables.
	int Value{};

	// Constructor.
	WDlgCustomDetail( UObject* InContext, WWindow* InOwnerWindow )
	:	WDialog				( TEXT("Detail Level"), IDDIALOG_2DSE_CUSTOM_DETAIL, InOwnerWindow )
	,	OKButton			( this, IDOK, FDelegate(this,(TDelegate)&WDlgCustomDetail::OnOK) )
	,	CancelButton		( this, IDCANCEL, FDelegate(this,(TDelegate)&WDlgCustomDetail::EndDialogFalse) )
	,	ValueEdit			( this, IDEC_VALUE )
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgCustomDetail::OnInitDialog);
		WDialog::OnInitDialog();
		ValueEdit.SetText(TEXT("10"));
		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgCustomDetail::OnDestroy);
		WDialog::OnDestroy();
		::DestroyWindow( hWnd );
		unguard;
	}
	void OnClose()
	{
		Show(0);
	}
	virtual int DoModal()
	{
		guard(WDlgCustomDetail::DoModal);
		return WDialog::DoModal( hInstance );
		unguard;
	}

	void OnOK()
	{
		guard(WDlgCustomDetail::OnDestroy);
		Value = appAtoi( *ValueEdit.GetText() );
		EndDialogTrue();
		unguard;
	}
};

// --------------------------------------------------------------
//
// CYLINDER SHAPE Dialog
//
// --------------------------------------------------------------

class WDlgCylinderShape : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgCylinderShape, WDialog, UnrealEd)

	WButton OKButton, CancelButton;
	WEdit ValueEdit;

	// Variables.
	int Value;

	// Constructor.
	WDlgCylinderShape(UObject* InContext, WWindow* InOwnerWindow)
		: WDialog(TEXT("Cylinder Shape Properties"), IDDIALOG_2DSE_CYLINDER, InOwnerWindow)
		, CancelButton(this, IDCANCEL, FDelegate(this, (TDelegate)&WDialog::EndDialogFalse))
		, OKButton(this, IDOK, FDelegate(this, (TDelegate)&WDlgCylinderShape::OnOK))
		, ValueEdit(this, IDEC_VALUE)
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgCylinderShape::OnInitDialog);
		WDialog::OnInitDialog();
		ValueEdit.SetText(TEXT("8"));
		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgCylinderShape::OnDestroy);
		WDialog::OnDestroy();
		::DestroyWindow(hWnd);
		unguard;
	}
	void OnClose()
	{
		Show(0);
	}
	virtual int DoModal()
	{
		guard(WDlgCylinderShape::DoModal);
		return WDialog::DoModal(hInstance);
		unguard;
	}

	void OnOK()
	{
		guard(WDlgCylinderShape::OnDestroy);
		Value = appAtoi(*ValueEdit.GetText());
		EndDialogTrue();
		unguard;
	}
};

// --------------------------------------------------------------
//
// DATA TYPES
//
// --------------------------------------------------------------

class F2DSEVector : public FVector2D
{
public:
	F2DSEVector()
		: FVector2D(0.f,0.f), bSelected(FALSE)
	{}
	F2DSEVector(FLOAT x, FLOAT y)
		: FVector2D(x, y), bSelected(FALSE)
	{}
	F2DSEVector(const FVector2D& V, UBOOL bSelect = FALSE)
		: FVector2D(V.X, V.Y), bSelected(bSelect)
	{}

	inline UBOOL operator!=( F2DSEVector Other )
	{
		return (X != Other.X && Y != Other.Y);
	}
	inline UBOOL operator==( F2DSEVector Other )
	{
		return (X == Other.X && Y == Other.Y);
	}
	void SelectToggle()
	{
		bSelected = !bSelected;
		if( bSelected )
			SetTempPos();
	}
	void Select( BOOL bSel )
	{
		bSelected = bSel;
		if( bSelected )
			SetTempPos();
	}
	BOOL IsSel( void ) const
	{
		return bSelected;
	}
	void SetTempPos()
	{
		TempX = X;
		TempY = Y;
	}
	inline void GridSnap()
	{
		FVector2D Res = FVector2D::GridSnap(FVector2D(GGridSize, GGridSize));
		X = Res.X;
		Y = Res.Y;
	}

	float TempX{}, TempY{};

private:
	UBOOL bSelected;
};

enum eSEGTYPE {
	eSEGTYPE_LINEAR	= 0,
	eSEGTYPE_BEZIER	= 1
};

// A segment represents the line drawn between 2 vertices on a shape.
class FSegment
{
public:
	FSegment()
	{
		SegType = eSEGTYPE_LINEAR;
		DetailLevel = 10;
		LineColor = FColor(128, 128, 128, 0);
	}
	FSegment(const F2DSEVector& vtx1, const F2DSEVector& vtx2)
	{
		Vertex[0] = vtx1;
		Vertex[1] = vtx2;
		SegType = eSEGTYPE_LINEAR;
		DetailLevel = 10;
		LineColor = FColor(128, 128, 128, 0);
	}
	FSegment(const FVector2D& vtx1, const FVector2D& vtx2)
	{
		Vertex[0] = vtx1;
		Vertex[1] = vtx2;
		SegType = eSEGTYPE_LINEAR;
		DetailLevel = 10;
		LineColor = FColor(128, 128, 128, 0);
	}
	~FSegment()
	{}

	inline FSegment operator=( FSegment Other )
	{
		Vertex[0] = Other.Vertex[0];
		Vertex[1] = Other.Vertex[1];
		ControlPoint[0] = Other.ControlPoint[0];
		ControlPoint[1] = Other.ControlPoint[1];
		return *this;
	}
	inline UBOOL operator==( FSegment Other )
	{
		return( Vertex[0] == Other.Vertex[0] && Vertex[1] == Other.Vertex[1] );
	}
	FVector2D GetHalfwayPoint()
	{
		guard(FSegment::GetHalfwayPoint);

		FVector2D Dir = Vertex[1] - Vertex[0];
		FLOAT Size = Dir.Size();
		Dir /= Size;

		FVector2D HalfWay = Vertex[0] + (Dir * (Size / 2.f));
		HalfWay = HalfWay.GridSnap(FVector2D(GGridSize, GGridSize));
		return HalfWay;

		unguard;
	}
	void GenerateControlPoint()
	{
		guard(FSegment::GetOneThird);

		FVector2D Dir = Vertex[1] - Vertex[0];
		FLOAT Size = Dir.Size();
		Dir /= Size;

		ControlPoint[0] = Vertex[0] + (Dir * (Size / 3.0f));
		ControlPoint[1] = Vertex[1] - (Dir * (Size / 3.0f));

		unguard;
	}
	void SetSegType( int InType )
	{
		guard(FSegment::SetSegType);
		if( InType == SegType ) return;
		SegType = InType;
		if( InType == eSEGTYPE_BEZIER )
		{
			GenerateControlPoint();
			DetailLevel = GDefaultDetailLevel;
		}
		unguard;
	}
	UBOOL IsSel() const
	{
		guard(FSegment::IsSel);
		return ( Vertex[0].IsSel() || ControlPoint[0].IsSel() || ControlPoint[1].IsSel() );
		unguard;
	}
	void Select()
	{
		Vertex[0].Select(1);
	}
	void GetBezierPoints( TArray<FVector2D>& pBezierPoints ) const
	{
		guard(FSegment::GetBezierPoints);
		F2DSEVector ccp[4];
		ccp[0] = Vertex[0];
		ccp[1] = ControlPoint[0];
		ccp[2] = ControlPoint[1];
		ccp[3] = Vertex[1];
		F2DSEVector pt;

		pBezierPoints.Empty();
		new (pBezierPoints) FVector2D(Vertex[0]);
		for( int n = 1 ; n <= DetailLevel ; ++n )
		{
			double nD = (double)n / (DetailLevel + 1);
			Curve( nD, ccp, pt );
			new (pBezierPoints) FVector2D(pt);
		}
		new (pBezierPoints) FVector2D(Vertex[1]);
		unguard;
	}
	// Get point on cubic bezier curve (slow)
	void Curve( double nP, F2DSEVector ControlPoint[4], F2DSEVector& Position ) const
	{
		guard(FSegment::Curve);
		double bf[4];
		CurveBlendingFunction( nP, bf );
		Position.X = Position.Y = 0;
		for( int n = 0 ; n < 4 ; ++n )
			Position += ControlPoint[n] * bf[n];
		unguard;
	}
	// Evaluate Bernstein polynomials B_0^3(p) ... B_3^3(p)
	void CurveBlendingFunction( double nP, double pBf[4] ) const
	{
		guard(FSegment::CurveBlendingFunction);
		double nQ = 1 - nP;
		double nP2 = nP * nP;
		double nQ2 = nQ * nQ;
		pBf[0] = nQ2 * nQ;
		pBf[1] = 3 * nQ2 * nP;
		pBf[2] = 3 * nP2 * nQ;
		pBf[3] = nP2 * nP;
		unguard;
	}

	inline UBOOL OverlapsItself() const
	{
		return (Abs(Vertex[0].X - Vertex[1].X) < 0.1f && Abs(Vertex[0].Y - Vertex[1].Y) < 0.1f);
	}

	F2DSEVector Vertex[2], ControlPoint[2];
	FColor LineColor;
	int SegType,		// eSEGTYPE_
		DetailLevel;	// Detail of bezier curve
	UBOOL bUsed{};
};

inline INT AddToList(TArray<FVector2D>& Verts, const FVector2D NewPoint)
{
	for (INT i = 0; i < Verts.Num(); ++i)
		if (Verts(i).DistSquared(NewPoint) < 0.025f)
			return i;
	return Verts.AddItem(NewPoint);
}
inline void AddSegment(F2DTessellator& Tess, F2DTessellator::FInputShape* Shape, FSegment& Segment)
{
	if (Segment.SegType == eSEGTYPE_BEZIER)
	{
		TArray<FVector2D> BezierPoints;
		Segment.GetBezierPoints(BezierPoints);
		for (int bz = BezierPoints.Num() - 1; bz > 0; bz--)
			Shape->Links.AddItem(AddToList(Tess.Vertices, FVector2D(BezierPoints(bz))));
	}
	else Shape->Links.AddItem(AddToList(Tess.Vertices, FVector2D(Segment.Vertex[1])));
	Segment.bUsed = TRUE;
}

// A shape is a closed series of segments (i.e. a triangle is 3 segments).
class FShape
{
public:
	void ComputeHandlePosition(UBOOL bAllowMerge = 0)
	{
		guard(FShape::ComputeHandlePosition);
		Handle = F2DSEVector(0.f, 0.f);
		int seg;
		// Remove vertices that are overlapping.
		if (bAllowMerge)
		{
			for (seg = 0; seg < Segments.Num(); seg++)
				if (Segments(seg).OverlapsItself())
					Segments.Remove(seg--);
		}
		for (seg = 0; seg < Segments.Num(); seg++)
			Handle += Segments(seg).Vertex[0];
		Handle /= Segments.Num();
		unguard;
	}

	// Breaks up the shape into triangles using the Delaunay triangulation method.
	void PopulateTesselator(F2DTessellator& Tess)
	{
		guard(FShape::PopulateTesselator);

		// Build a CCW list of the vertices that make up this shape.
		INT seg;
		for( seg = 0 ; seg < Segments.Num() ; seg++ )
			Segments(seg).bUsed = FALSE;

		TArray<F2DTessellator::FInputShape>& Shapes = Tess.Shapes;
		F2DTessellator::FInputShape* Current = nullptr;
		F2DSEVector* Match = nullptr;

		for(;;)
		{
			Current = nullptr;
			for (seg = 0; seg < Segments.Num(); seg++)
			{
				if (!Segments(seg).bUsed)
				{
					Current = new (Shapes) F2DTessellator::FInputShape;
					AddSegment(Tess, Current, Segments(seg));
					Match = &Segments(0).Vertex[0];
					break;
				}
			}
			if (!Current)
				break;

			for (seg = 0; seg < Segments.Num(); seg++)
			{
				if (!Segments(seg).bUsed && Segments(seg).Vertex[1].DistSquared(*Match) < 0.025f)
				{
					AddSegment(Tess, Current, Segments(seg));
					Match = &Segments(seg).Vertex[0];
					seg = 0;
				}
			}
		}
		unguard;
	}

	inline UBOOL HasVertSelected() const
	{
		for (INT seg = 0; seg < Segments.Num(); seg++)
			if (Segments(seg).IsSel())
				return TRUE;
		return FALSE;
	}

	F2DSEVector Handle;
	TArray<FSegment> Segments;
};

#define d2dSE_SELECT_TOLERANCE 4

// --------------------------------------------------------------
//
// W2DSHAPEEDITOR
//
// --------------------------------------------------------------

static inline void SkipSpaces(const TCHAR*& Str)
{
	while (*Str == ' ' || *Str == '\t')
		++Str;
}
static inline void ParseVector2D(const TCHAR* Str, FLOAT& X, FLOAT& Y)
{
	SkipSpaces(Str);
	if (*Str == '(')
		++Str;
	SkipSpaces(Str);
	X = appAtof(Str);
	while (appIsDigit(*Str) || *Str == '.' || *Str == 'e' || *Str == 'E' || *Str == 'f' || *Str == 'F' || *Str == '-' || *Str == '+')
		++Str;
	SkipSpaces(Str);
	if (*Str != ',')
		return;
	++Str;
	SkipSpaces(Str);
	Y = appAtof(Str);
}

#define ID_2DSE_TOOLBAR	29002
TBBUTTON tb2DSEButtons[] = {
	{ 0, IDMN_2DSE_NEW, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 1, IDMN_2DSE_FileOpen, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 2, IDMN_2DSE_FileSave, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 3, IDMN_2DSE_ROTATE90, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 4, IDMN_2DSE_ROTATE45, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 5, IDMN_2DSE_FLIP_VERT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 6, IDMN_2DSE_FLIP_HORIZ, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 7, IDMN_2DSE_SCALE_UP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 8, IDMN_2DSE_SCALE_DOWN, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 9, IDMN_2DSE_ZOOM_IN, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 10, IDMN_2DSE_ZOOM_OUT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 11, IDMN_2DSE_CHOOSE_LINE_COLOR, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0 }
	, { 12, IDMN_2DSE_SPLIT_SIDE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 13, IDMN_2DSE_DELETE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 14, IDMN_SEGMENT_LINEAR, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 15, IDMN_SEGMENT_BEZIER, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 16, IDMN_2DSE_PROCESS_SHEET, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 17, IDMN_2DSE_PROCESS_REVOLVE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 18, IDMN_2DSE_PROCESS_EXTRUDE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 19, IDMN_2DSE_PROCESS_EXTRUDETOPOINT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 20, IDMN_2DSE_PROCESS_EXTRUDETOBEVEL, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
};
struct {
	TCHAR ToolTip[64];
	int ID;
} ToolTips_2DSE[] = {
	TEXT("New"), IDMN_2DSE_NEW,
	TEXT("Open"), IDMN_2DSE_FileOpen,
	TEXT("Save"), IDMN_2DSE_FileSave,
	TEXT("Rotate 90 Degrees"), IDMN_2DSE_ROTATE90,
	TEXT("Rotate 45 Degrees"), IDMN_2DSE_ROTATE45,
	TEXT("Flip Vertically"), IDMN_2DSE_FLIP_VERT,
	TEXT("Flip Horizontally"), IDMN_2DSE_FLIP_HORIZ,
	TEXT("Create a Sheet"), IDMN_2DSE_PROCESS_SHEET,
	TEXT("Extruded Shape"), IDMN_2DSE_PROCESS_EXTRUDE,
	TEXT("Revolved Shape"), IDMN_2DSE_PROCESS_REVOLVE,
	TEXT("Extrude to Point"), IDMN_2DSE_PROCESS_EXTRUDETOPOINT,
	TEXT("Extrude to Bevel"), IDMN_2DSE_PROCESS_EXTRUDETOBEVEL,
	TEXT("Scale Up"), IDMN_2DSE_SCALE_UP,
	TEXT("Scale Down"), IDMN_2DSE_SCALE_DOWN,
	TEXT("Choose Line Color"), IDMN_2DSE_CHOOSE_LINE_COLOR,
	TEXT("Split Segment(s)"), IDMN_2DSE_SPLIT_SIDE,
	TEXT("Delete"), IDMN_2DSE_DELETE,
	TEXT("Linear Segment"), IDMN_SEGMENT_LINEAR,
	TEXT("Bezier Segment"), IDMN_SEGMENT_BEZIER,
	TEXT("Zoom In"), IDMN_2DSE_ZOOM_IN,
	TEXT("Zoom Out"), IDMN_2DSE_ZOOM_OUT,
	NULL, 0
};

class W2DShapeEditor : public WWindow
{
	DECLARE_WINDOWCLASS(W2DShapeEditor,WWindow,Window)

	// Structors.
	W2DShapeEditor( FName InPersistentName, WWindow* InOwnerWindow )
	:	WWindow( InPersistentName, InOwnerWindow ), MapFilename(TEXT("Untitled.2ds")), bUntitledFile(TRUE)
	{
		New();
		m_bDraggingCamera = m_bDraggingVerts = FALSE;
		hImage = NULL;
		hWndToolBar = NULL;
		Zoom = 1.0f;
		GGridSizeZoom = GGridSize;
		m_ShapeDirty = FALSE;
		BgImage = NULL;
	}

	WToolTip* ToolTipCtrl{};
	HWND hWndToolBar;
	FVector2D m_camera;			// The viewing camera position
	F2DSEVector m_origin;		// The origin point used for revolves and such
	UBOOL m_ShapeDirty, m_bDraggingCamera, m_bDraggingVerts, m_bMouseHasMoved{};
	FPoint m_pointOldPos{};
	FString MapFilename;
	HBITMAP hImage;
	RECT m_rcWnd{};
	POINT m_ContextPos{};
	float Zoom;
	HBITMAP ToolbarImage{};
	UTexture* BgImage;
	UBOOL bUntitledFile;

	TArray<FShape> m_shapes;

	// WWindow interface.
	void OpenWindow()
	{
		guard(W2DShapeEditor::OpenWindow);
		MdiChild = 0;

		PerformCreateWindowEx
		(
			0,
			NULL,
			WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
			0,
			0,
			MulDiv(640, DPIX, 96),
			MulDiv(480, DPIY, 96),
			OwnerWindow ? OwnerWindow->hWnd : NULL,
			NULL,
			hInstance
		);
		SendMessageW(*this, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(0, 0));
		SetCaption();
		UpdateMenu();
		unguard;
	}
	void OnDestroy()
	{
		guard(W2DShapeEditor::OnDestroy);
		if (m_ShapeDirty && ::MessageBox(hWnd, TEXT("Save changes?"), TEXT("2DShapeEditor"), MB_YESNO) == IDYES)
			FileSave(hWnd);

		delete ToolTipCtrl;
		::DestroyWindow( hWndToolBar );
		if (ToolbarImage)
			DeleteObject(ToolbarImage);
		WWindow::OnDestroy();
		unguard;
	}
	void OnClose()
	{
		guard(W2DShapeEditor::OnClose);
		/*
		TCHAR l_chMsg[256];
		appSprintf( l_chMsg, TEXT("Do you really want to close?"));
		if( ::MessageBox( hWnd, l_chMsg, TEXT("UnrealEd"), MB_YESNO) == IDYES )
		*/
		Show(0);
		unguard;
	}
	INT OnSetCursor()
	{
		guard(W2DShapeEditor::OnSetCursor);
		WWindow::OnSetCursor();
		SetCursor(LoadCursorW(NULL, IDC_CROSS));
		return 0;
		unguard;
	}
	void OnCreate()
	{
		guard(W2DShapeEditor::OnCreate);
		WWindow::OnCreate();

		SetMenu( hWnd, LoadMenuW(hInstance, MAKEINTRESOURCEW(IDMENU_2DShapeEditor)) );

		ToolbarImage = reinterpret_cast<HBITMAP>(LoadImage(hInstance,
			MAKEINTRESOURCE(IDB_2DSE_TOOLBAR),
			IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS));
		check(ToolbarImage);
		ScaleImageAndReplace(ToolbarImage);

		// TOOLBAR
		hWndToolBar = CreateToolbarEx( 
			hWnd,
			WS_CHILD | WS_VISIBLE | CCS_ADJUSTABLE,
			ID_2DSE_TOOLBAR,
			21,
			NULL,
			reinterpret_cast<PTRINT>(ToolbarImage),
			(LPCTBBUTTON)&tb2DSEButtons,
			28,
			MulDiv(18, DPIX, 96), MulDiv(18, DPIY, 96),
			MulDiv(18, DPIX, 96), MulDiv(18, DPIY, 96),
			sizeof(TBBUTTON));

		if( !hWndToolBar )
			appMsgf( TEXT("Toolbar not created!") );

		ToolTipCtrl = new WToolTip(this);
		ToolTipCtrl->OpenWindow();

		for( int tooltip = 0 ; ToolTips_2DSE[tooltip].ID > 0 ; tooltip++ )
		{
			int index = SendMessageW( hWndToolBar, TB_COMMANDTOINDEX, ToolTips_2DSE[tooltip].ID, 0 );
			RECT rect;
			SendMessageW( hWndToolBar, TB_GETITEMRECT, index, (LPARAM)&rect);

			ToolTipCtrl->AddTool( hWndToolBar, ToolTips_2DSE[tooltip].ToolTip, tooltip, &rect );
		}

		unguard;
	}
	void OnPaint()
	{
		guard(W2DShapeEditor::OnPaint);
		PAINTSTRUCT PS;
		HDC hDC = BeginPaint( *this, &PS );

		HDC hdcWnd, hdcMem;
		HBITMAP t_hBitmap;
		HBRUSH l_brush, l_brushOld = NULL;

		::GetClientRect( hWnd, &m_rcWnd );

		hdcWnd = GetDC(hWnd);
		hdcMem = CreateCompatibleDC(hdcWnd);
		t_hBitmap = CreateCompatibleBitmap(hdcWnd, m_rcWnd.right, m_rcWnd.bottom );
		SelectObject(hdcMem, t_hBitmap);

		l_brush = CreateSolidBrush( RGB(255, 255, 255) );
		SelectObject( hdcMem, l_brush);
		FillRect( hdcMem, GetClientRect(), l_brush );
		l_brushOld = (HBRUSH)SelectObject( hdcMem, l_brushOld);

		DrawGrid( hdcMem );
		DrawImage( hdcMem );
		DrawOrigin( hdcMem );
		DrawSegments( hdcMem );
		DrawShapeHandles( hdcMem );
		DrawVertices( hdcMem );

		BitBlt(hDC,
			   0, 0,
			   m_rcWnd.right, m_rcWnd.bottom,
			   hdcMem,
			   0, 0,
			   SRCCOPY);

		EndPaint( *this, &PS );

		DeleteObject( l_brush );
		DeleteDC(hdcMem);
		ReleaseDC( hWnd, hdcWnd );
		DeleteObject(t_hBitmap);
		unguard;
	}
	void ScaleShapes( float InScale )
	{
		guard(W2DShapeEditor::ScaleShapes);

		m_ShapeDirty = TRUE;
		for( int shape = 0 ; shape < m_shapes.Num() ; shape++ )
		{
			for( int seg = 0 ; seg < m_shapes(shape).Segments.Num() ; seg++ )
			{
				for( int vertex = 0 ; vertex < 2 ; vertex++ )
				{
					m_shapes(shape).Segments(seg).Vertex[vertex].X *= InScale;
					m_shapes(shape).Segments(seg).Vertex[vertex].Y *= InScale;
					m_shapes(shape).Segments(seg).ControlPoint[vertex].X *= InScale;
					m_shapes(shape).Segments(seg).ControlPoint[vertex].Y *= InScale;
				}
			}
		}

		ComputeHandlePositions(1);
		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void SetBackgroundImage(UTexture* Tex, UBOOL bShowError = TRUE)
	{
		FTextureInfo* Info = Tex->GetTexture(INDEX_NONE, NULL);
		if (!Info || Info->Format != TEXF_P8)
		{
			if(bShowError)
				appMsgf(TEXT("Import of DXT Textures not supported."));
		}
		else
		{
			BITMAPINFO bmi;
			BYTE* pBits = NULL;

			::ZeroMemory(&bmi, sizeof(BITMAPINFOHEADER));
			bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmi.bmiHeader.biWidth = Info->USize;
			bmi.bmiHeader.biHeight = -Info->VSize;
			bmi.bmiHeader.biPlanes = 1;
			bmi.bmiHeader.biBitCount = 24;
			bmi.bmiHeader.biCompression = BI_RGB;

			if (hImage)
				DeleteObject(hImage);
			hImage = CreateDIBSection(::GetDC(hWnd), &bmi, DIB_RGB_COLORS, (void**)&pBits, NULL, 0);
			check(hImage);
			check(pBits);

			// RGB DATA
			FColor* Palette = Info->Palette;
			check(Palette);
			BYTE* pSrc = Info->Mips[0]->DataPtr;
			BYTE* pDst = pBits;
			INT pixCount = Info->USize * Info->VSize;
			for (int x = 0; x < pixCount; x++)
			{
				FColor color = Palette[*pSrc];
				*pDst = color.B;	pDst++;
				*pDst = color.G;	pDst++;
				*pDst = color.R;	pDst++;
				pSrc++;
			}
			BgImage = Tex;
		}
	}
	void OnCommand( INT Command )
	{
		guard(W2DShapeEditor::OnCommand);
		TCHAR l_chMsg[256];
		switch( Command ) {

			case ID_FileExit:
				appSprintf(l_chMsg, TEXT("Save changes?"));
				if (m_ShapeDirty && ::MessageBox(hWnd, l_chMsg, TEXT("UnrealEd"), MB_YESNO) == IDYES)
					FileSave(hWnd);
				SendMessageW( hWnd, WM_CLOSE, 0, 0 );
				UpdateMenu();
				break;

			case IDMN_2DSE_ZOOM_IN:
				Zoom *= 2.0f;
				if( Zoom > 8.0f) Zoom = 8.0f;
				GGridSizeZoom = GGridSize * Zoom;
				InvalidateRect( hWnd, NULL, FALSE );
				UpdateMenu();
				break;

			case IDMN_2DSE_ZOOM_OUT:
				Zoom *= 0.5f;
				if( Zoom < 0.125f) Zoom = 0.125f;
				GGridSizeZoom = GGridSize * Zoom;
				InvalidateRect( hWnd, NULL, FALSE );
				UpdateMenu();
				break;

			case IDMN_2DSE_SCALE_UP:
				ScaleShapes(2.0f);
				UpdateMenu();
				break;

			case IDMN_2DSE_SCALE_DOWN:
				ScaleShapes(0.5f);
				UpdateMenu();
				break;

			case IDMN_2DSE_ROTATE90:
				RotateShape( 90 );
				UpdateMenu();
				break;

			case IDMN_2DSE_ROTATE45:
				RotateShape( 45 );
				UpdateMenu();
				break;

			case IDMN_DETAIL_1:
				SetSegmentDetail(1);
				UpdateMenu();
				break;

			case IDMN_DETAIL_2:
				SetSegmentDetail(2);
				UpdateMenu();
				break;

			case IDMN_DETAIL_3:
				SetSegmentDetail(3);
				UpdateMenu();
				break;

			case IDMN_DETAIL_4:
				SetSegmentDetail(4);
				UpdateMenu();
				break;

			case IDMN_DETAIL_5:
				SetSegmentDetail(5);
				UpdateMenu();
				break;

			case IDMN_DETAIL_10:
				SetSegmentDetail(10);
				UpdateMenu();
				break;

			case IDMN_DETAIL_15:
				SetSegmentDetail(15);
				UpdateMenu();
				break;

			case IDMN_DETAIL_20:
				SetSegmentDetail(20);
				UpdateMenu();
				break;

			case IDMN_DETAIL_CUSTOM:
				{
					WDlgCustomDetail dlg( NULL, this );
					if( dlg.DoModal() )
						SetSegmentDetail( dlg.Value );
				}
				UpdateMenu();
				break;

			case IDMN_2DSE_FLIP_VERT:
				Flip(0);
				UpdateMenu();
				break;

			case IDMN_2DSE_FLIP_HORIZ:
				Flip(1);
				UpdateMenu();
				break;

			case IDMN_2DSE_FLIP_INSDE:
				Flip(2);
				UpdateMenu();
				break;

			case IDMN_2DSE_FLIP_INSDE_SEL:
				Flip(2, TRUE);
				break;

			case IDMN_SEGMENT_LINEAR:
				ChangeSegmentTypes(eSEGTYPE_LINEAR);
				UpdateMenu();
				break;

			case IDMN_SEGMENT_BEZIER:
				ChangeSegmentTypes(eSEGTYPE_BEZIER);
				UpdateMenu();
				break;

			case IDMN_2DSE_NEW:
				appSprintf( l_chMsg, TEXT("Save changes?"));
				if(m_ShapeDirty && ::MessageBox( hWnd, l_chMsg, TEXT("UnrealEd"), MB_YESNO) == IDYES )
					FileSave( hWnd );

				MapFilename = TEXT("Untitled.2ds");
				bUntitledFile = TRUE;
				SetCaption();
				New();
				UpdateMenu();
				break;

			case IDMN_2DSE_SPLIT_SIDE:
				SplitSides();
				UpdateMenu();
				break;

			case IDMN_2DSE_NEW_SHAPE:
				InsertNewShape();
				UpdateMenu();
				break;

			case IDMN_2DSE_NEW_TRIANGLE:
				InsertNewTriangle();
				UpdateMenu();
				break;

			case IDMN_2DSE_NEW_CYLINDER:
				{
					WDlgCylinderShape dlg(NULL, this);
					if (dlg.DoModal())
						InsertNewCylinder(dlg.Value);
				}
				UpdateMenu();
				break;

			case IDMN_2DSE_DELETE:
				Delete();
				UpdateMenu();
				break;

			case IDMN_2DSE_FileSave:
				FileSave( hWnd );
				UpdateMenu();
				break;

			case IDMN_2DSE_FileSaveAs:
				FileSaveAs( hWnd );
				UpdateMenu();
				break;

			case IDMN_2DSE_FileOpen:
				appSprintf( l_chMsg, TEXT("Save changes?"));
				if (m_ShapeDirty && ::MessageBox(hWnd, l_chMsg, TEXT("UnrealEd"), MB_YESNO) == IDYES)
					FileSave( hWnd );
				FileOpen( hWnd );
				UpdateMenu();
				break;

			case IDMN_2DSEC_SET_ORIGIN:
				SetOrigin();
				UpdateMenu();
				break;

			case IDMN_2DSE_PROCESS_SHEET:
				ProcessSheet();
				UpdateMenu();
				break;

			case IDMN_2DSE_PROCESS_EXTRUDE:
				{
					WDlgExtrude Dialog( NULL, this );
					if( Dialog.DoModal() == IDOK )
						ProcessExtrude( Dialog.Depth );
				}
				UpdateMenu();
				break;

			case IDMN_2DSE_PROCESS_EXTRUDETOPOINT:
				{
					WDlgExtrudeToPoint Dialog( NULL, this );
					if( Dialog.DoModal() == IDOK )
						ProcessExtrudeToPoint( Dialog.Depth );
				}
				UpdateMenu();
				break;

			case IDMN_2DSE_PROCESS_EXTRUDETOBEVEL:
				{
					WDlgExtrudeToBevel Dialog( NULL, this );
					if( Dialog.DoModal() == IDOK )
						ProcessExtrudeToBevel( Dialog.Depth, Dialog.CapHeight );
				}
				UpdateMenu();
				break;

			case IDMN_2DSE_PROCESS_REVOLVE:
				{
					WDlgRevolve Dialog( NULL, this );
					if( Dialog.DoModal() == IDOK )
						ProcessRevolve( Dialog.TotalSides, Dialog.Sides );
				}
				UpdateMenu();
				break;

			case IDMN_GRID_1:
				GGridSize = 1;
				GGridSizeZoom = GGridSize * Zoom;
				InvalidateRect( hWnd, NULL, FALSE );
				UpdateMenu();
				break;
			case IDMN_GRID_2:
				GGridSize = 2;
				GGridSizeZoom = GGridSize * Zoom;
				InvalidateRect( hWnd, NULL, FALSE );
				UpdateMenu();
				break;
			case IDMN_GRID_4:
				GGridSize = 4;
				GGridSizeZoom = GGridSize * Zoom;
				InvalidateRect( hWnd, NULL, FALSE );
				UpdateMenu();
				break;
			case IDMN_GRID_8:
				GGridSize = 8;
				GGridSizeZoom = GGridSize * Zoom;
				InvalidateRect( hWnd, NULL, FALSE );
				UpdateMenu();
				break;
			case IDMN_GRID_16:
				GGridSize = 16;
				GGridSizeZoom = GGridSize * Zoom;
				InvalidateRect( hWnd, NULL, FALSE );
				UpdateMenu();
				break;
			case IDMN_GRID_32:
				GGridSize = 32;
				GGridSizeZoom = GGridSize * Zoom;
				InvalidateRect( hWnd, NULL, FALSE );
				UpdateMenu();
				break;
			case IDMN_GRID_64:
				GGridSize = 64;
				GGridSizeZoom = GGridSize * Zoom;
				InvalidateRect( hWnd, NULL, FALSE );
				UpdateMenu();
				break;
			case IDMN_GRID_128:
				GGridSize = 128;
				GGridSizeZoom = GGridSize * Zoom;
				InvalidateRect( hWnd, NULL, FALSE );
				UpdateMenu();
				break;
			case IDMN_GRID_256:
				GGridSize = 256;
				GGridSizeZoom = GGridSize * Zoom;
				InvalidateRect( hWnd, NULL, FALSE );
				UpdateMenu();
				break;

			case IDMN_2DSE_GET_IMAGE:
				{
					if( !GEditor->CurrentTexture )
					{
						appMsgf(TEXT("Select a texture in the browser first."));
						return;
					}
					SetBackgroundImage(GEditor->CurrentTexture);
				}
				UpdateMenu();
				break;

			case IDMN_2DSE_OPEN_IMAGE:
				{
					OPENFILENAMEA ofn;
					char File[8192] = "\0";

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(char) * 8192;
					ofn.lpstrFilter = "Bitmaps (*.bmp)\0*.bmp\0All Files\0*.*\0\0";
					ofn.lpstrInitialDir = "..\\maps";
					ofn.lpstrTitle = "Open Image";
					ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_UTX]) );
					ofn.lpstrDefExt = "bmp";
					ofn.Flags = OFN_NOCHANGEDIR;

					// Display the Open dialog box.
					//
					if( GetOpenFileNameA(&ofn) )
					{
						if( hImage )
							DeleteObject( hImage );

						hImage = (HBITMAP)LoadImageA( hInstance, File, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE );

						if (!hImage)
							appMsgf(TEXT("Error loading bitmap."));
						else
							ScaleImageAndReplace(hImage);

						FString S = appFromAnsi( File );
						GLastDir[eLASTDIR_UTX] = S.GetFilePath().LeftChop(1);
						BgImage = NULL;
					}

					InvalidateRect( hWnd, NULL, FALSE );
				}
				UpdateMenu();
				break;

			case IDMN_2DSE_DELETE_IMAGE:
				DeleteObject( hImage );
				hImage = NULL;
				BgImage = NULL;
				InvalidateRect( hWnd, NULL, FALSE );
				UpdateMenu();
				break;

			case IDMN_2DSE_CHOOSE_LINE_COLOR:
				OnChooseColorButton();
				break;

			default:
				WWindow::OnCommand(Command);
				UpdateMenu();
				break;
		}
		unguard;
	}
	void OnRightButtonDown(INT X, INT Y)
	{
		guard(W2DShapeEditor::OnRightButtonDown);

		m_bDraggingCamera = TRUE;
		m_pointOldPos = FPoint(X, Y);
		SetCapture( hWnd );
		m_bMouseHasMoved = FALSE;

		WWindow::OnRightButtonDown(X, Y);
		unguard;
	}
	virtual void OnVScroll(WPARAM wParam, LPARAM lParam)
	{
		guard(W2DShapeEditor::OnVScroll);
		if (lParam == WM_MOUSEWHEEL)
		{
			switch (LOWORD(wParam))
			{
			case SB_LINEUP:
				Zoom *= 1.1f;
				if (Zoom > 8.0f)
					Zoom = 8.0f;
				GGridSizeZoom = GGridSize * Zoom;
				InvalidateRect(hWnd, NULL, FALSE);
				UpdateMenu();
				break;

			case SB_LINEDOWN:
				Zoom /= 1.1f;
				if (Zoom < 0.125f)
					Zoom = 0.125f;
				GGridSizeZoom = GGridSize * Zoom;
				InvalidateRect(hWnd, NULL, FALSE);
				UpdateMenu();
				break;
			}
		}
		unguard;
	}
	virtual void UpdateMenu()
	{
		guard(W2DShapeEditor::UpdateMenu);
		HMENU menu = GetMenu( hWnd );
		CheckMenuItem( menu, IDMN_GRID_1, MF_BYCOMMAND | ((GGridSize == 1) ? MF_CHECKED : MF_UNCHECKED) );
		CheckMenuItem( menu, IDMN_GRID_2, MF_BYCOMMAND | ((GGridSize == 2) ? MF_CHECKED : MF_UNCHECKED) );
		CheckMenuItem( menu, IDMN_GRID_4, MF_BYCOMMAND | ((GGridSize == 4) ? MF_CHECKED : MF_UNCHECKED) );
		CheckMenuItem( menu, IDMN_GRID_8, MF_BYCOMMAND | ((GGridSize == 8) ? MF_CHECKED : MF_UNCHECKED) );
		CheckMenuItem( menu, IDMN_GRID_16, MF_BYCOMMAND | ((GGridSize == 16) ? MF_CHECKED : MF_UNCHECKED) );
		CheckMenuItem( menu, IDMN_GRID_32, MF_BYCOMMAND | ((GGridSize == 32) ? MF_CHECKED : MF_UNCHECKED) );
		CheckMenuItem( menu, IDMN_GRID_64, MF_BYCOMMAND | ((GGridSize == 64) ? MF_CHECKED : MF_UNCHECKED) );
		CheckMenuItem( menu, IDMN_GRID_128, MF_BYCOMMAND | ((GGridSize == 128) ? MF_CHECKED : MF_UNCHECKED) );
		CheckMenuItem( menu, IDMN_GRID_256, MF_BYCOMMAND | ((GGridSize == 256) ? MF_CHECKED : MF_UNCHECKED) );
		unguard;
	}
	void OnRightButtonUp()
	{
		guard(W2DShapeEditor::OnRightButtonUp);

		ReleaseCapture();
		m_bDraggingCamera = FALSE;

		if( !m_bMouseHasMoved )
		{
			::GetCursorPos( &m_ContextPos );
			HMENU l_menu = GetSubMenu( LoadMenuW(hInstance, MAKEINTRESOURCEW(IDMENU_2DShapeEditor_Context)), 0 );

			CheckMenuItem( l_menu, IDMN_GRID_1, MF_BYCOMMAND | ((GGridSize == 1) ? MF_CHECKED : MF_UNCHECKED) );
			CheckMenuItem( l_menu, IDMN_GRID_2, MF_BYCOMMAND | ((GGridSize == 2) ? MF_CHECKED : MF_UNCHECKED) );
			CheckMenuItem( l_menu, IDMN_GRID_4, MF_BYCOMMAND | ((GGridSize == 4) ? MF_CHECKED : MF_UNCHECKED) );
			CheckMenuItem( l_menu, IDMN_GRID_8, MF_BYCOMMAND | ((GGridSize == 8) ? MF_CHECKED : MF_UNCHECKED) );
			CheckMenuItem( l_menu, IDMN_GRID_16, MF_BYCOMMAND | ((GGridSize == 16) ? MF_CHECKED : MF_UNCHECKED) );
			CheckMenuItem( l_menu, IDMN_GRID_32, MF_BYCOMMAND | ((GGridSize == 32) ? MF_CHECKED : MF_UNCHECKED) );
			CheckMenuItem( l_menu, IDMN_GRID_64, MF_BYCOMMAND | ((GGridSize == 64) ? MF_CHECKED : MF_UNCHECKED) );
			CheckMenuItem( l_menu, IDMN_GRID_128, MF_BYCOMMAND | ((GGridSize == 128) ? MF_CHECKED : MF_UNCHECKED) );
			CheckMenuItem( l_menu, IDMN_GRID_256, MF_BYCOMMAND | ((GGridSize == 256) ? MF_CHECKED : MF_UNCHECKED) );

			TrackPopupMenu( l_menu,
				TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
				m_ContextPos.x, m_ContextPos.y, 0,
				hWnd, NULL);

			::ScreenToClient( hWnd, &m_ContextPos );
		}

		WWindow::OnRightButtonUp();
		unguard;
	}
	void OnMouseMove( DWORD Flags, FPoint MouseLocation )
	{
		guard(W2DShapeEditor::OnMouseMove);

		m_bMouseHasMoved = TRUE;

		FPoint CurPos = GetCursorPos();
		FRect CR = GetClientRect();

		if (CurPos.X > CR.Max.X || CurPos.Y > CR.Max.Y || CurPos.X < CR.Min.X || CurPos.Y < CR.Min.Y)
			return;

		FVector2D Delta;
		Delta.X = (MouseLocation.X - m_pointOldPos.X);
		Delta.Y = (MouseLocation.Y - m_pointOldPos.Y);

		if( m_bDraggingCamera )
		{
			m_camera.X += Delta.X;
			m_camera.Y += Delta.Y;

			m_pointOldPos = MouseLocation;
			InvalidateRect( hWnd, NULL, FALSE );
		}
		else
		{
			if( m_bDraggingVerts )
			{
				Delta *= (1.f / Zoom);
				m_ShapeDirty = TRUE;

				// Origin...
				if( m_origin.IsSel() )
				{
					// Adjust temp positions
					m_origin.TempX += Delta.X;
					m_origin.TempY -= Delta.Y;

					// Snap real positions to the grid.
					m_origin.X = m_origin.TempX;
					m_origin.Y = m_origin.TempY;
					m_origin.GridSnap();
				}

				// Handles...
				int shape;
				for( shape = 0 ; shape < m_shapes.Num() ; shape++ )
				{
					if( m_shapes(shape).Handle.IsSel() )
					{
						// Adjust temp positions
						m_shapes(shape).Handle.TempX += Delta.X;
						m_shapes(shape).Handle.TempY -= Delta.Y;

						// Snap real positions to the grid.
						m_shapes(shape).Handle.X = m_shapes(shape).Handle.TempX;
						m_shapes(shape).Handle.Y = m_shapes(shape).Handle.TempY;

						// Also move all of this shapes vertices.
						for( int seg = 0 ; seg < m_shapes(shape).Segments.Num() ; seg++ )
							for( int vertex = 0 ; vertex < 2 ; vertex++ )
							{
								F2DSEVector* Vertex = &(m_shapes(shape).Segments(seg).Vertex[vertex]);

								// Adjust temp positions
								Vertex->TempX += Delta.X;
								Vertex->TempY -= Delta.Y;

								// Snap real positions to the grid.
								Vertex->X = Vertex->TempX;
								Vertex->Y = Vertex->TempY;
								Vertex->GridSnap();

								if( m_shapes(shape).Segments(seg).SegType == eSEGTYPE_BEZIER )
								{
									F2DSEVector* vert = &(m_shapes(shape).Segments(seg).ControlPoint[vertex]);

									// Adjust temp positions
									vert->TempX += Delta.X;
									vert->TempY -= Delta.Y;

									// Snap real positions to the grid.
									vert->X = vert->TempX;
									vert->Y = vert->TempY;
									vert->GridSnap();
								}
							}
					}
				}

				// Vertices...
				for( shape = 0 ; shape < m_shapes.Num() ; shape++ )
					if( !m_shapes(shape).Handle.IsSel() )
						for( int seg = 0 ; seg < m_shapes(shape).Segments.Num() ; seg++ )
						{
							for( int vertex = 0 ; vertex < 2 ; vertex++ )
							{
								if( m_shapes(shape).Segments(seg).Vertex[vertex].IsSel() )
								{
									F2DSEVector* Vertex = &(m_shapes(shape).Segments(seg).Vertex[vertex]);

									// Adjust temp positions
									Vertex->TempX += Delta.X;
									Vertex->TempY -= Delta.Y;

									// Snap real positions to the grid.
									Vertex->X = Vertex->TempX;
									Vertex->Y = Vertex->TempY;
									Vertex->GridSnap();
								}
								if( m_shapes(shape).Segments(seg).SegType == eSEGTYPE_BEZIER
										&& m_shapes(shape).Segments(seg).ControlPoint[vertex].IsSel() )
								{
									F2DSEVector* Vertex = &(m_shapes(shape).Segments(seg).ControlPoint[vertex]);

									// Adjust temp positions
									Vertex->TempX += Delta.X;
									Vertex->TempY -= Delta.Y;

									// Snap real positions to the grid.
									Vertex->X = Vertex->TempX;
									Vertex->Y = Vertex->TempY;
									Vertex->GridSnap();
								}
							}
						}

			}
		}

		m_pointOldPos = MouseLocation;
		ComputeHandlePositions();
		InvalidateRect( hWnd, NULL, FALSE );

		InvalidateRect( hWnd, NULL, FALSE );

		WWindow::OnMouseMove( Flags, MouseLocation );
		unguard;
	}
	void ComputeHandlePositions(UBOOL bAlwaysCompute = FALSE, UBOOL bAllowMerge = FALSE)
	{
		guard(W2DShapeEditor::ComputeHandlePositions);
		for( int shape = 0 ; shape < m_shapes.Num() ; shape++ )
			if( !m_shapes(shape).Handle.IsSel() || bAlwaysCompute )
				m_shapes(shape).ComputeHandlePosition(bAllowMerge);
		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void ChangeSegmentTypes( int InType )
	{
		guard(W2DShapeEditor::ChangeSegmentTypes);

		for( int shape = 0 ; shape < m_shapes.Num() ; shape++ )
			for( int seg = 0 ; seg < m_shapes(shape).Segments.Num() ; seg++ )
				if( m_shapes(shape).Segments(seg).IsSel() )
					m_shapes(shape).Segments(seg).SetSegType(InType);

		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void SetSegmentDetail( int InDetailLevel )
	{
		guard(W2DShapeEditor::SetSegmentDetail);
		GDefaultDetailLevel = InDetailLevel;
		for( int shape = 0 ; shape < m_shapes.Num() ; shape++ )
			for( int seg = 0 ; seg < m_shapes(shape).Segments.Num() ; seg++ )
				if( m_shapes(shape).Segments(seg).IsSel() )
					m_shapes(shape).Segments(seg).DetailLevel = InDetailLevel;

		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void OnLeftButtonDown(INT X, INT Y)
	{
		guard(W2DShapeEditor::OnLeftButtonDown);

		m_bDraggingVerts = TRUE;
		m_pointOldPos = FPoint(X, Y);

		if( GetAsyncKeyState(VK_CONTROL) & 0x8000 )
			ProcessHits( 0, TRUE );
		else
		{
			// If the user has clicked on a vertex, then select that vertex and put them into drag mode.  Otherwise,
			// leave the current selections alone and just drag them.
			if( ProcessHits( 1, TRUE ) )
				ProcessHits( 0, FALSE );
			else
				DeselectAllVerts();
		}

		// Set the temp positions on all vertices.
		for( int shape = 0 ; shape < m_shapes.Num() ; shape++ )
		{
			m_shapes(shape).Handle.SetTempPos();
			for( int seg = 0 ; seg < m_shapes(shape).Segments.Num() ; seg++ )
				for( int vertex = 0 ; vertex < 2 ; vertex++ )
				{
					m_shapes(shape).Segments(seg).Vertex[vertex].SetTempPos();
					m_shapes(shape).Segments(seg).ControlPoint[vertex].SetTempPos();
				}
		}

		InvalidateRect( hWnd, NULL, FALSE );

		WWindow::OnLeftButtonDown(X, Y);
		unguard;
	}
	void OnLeftButtonUp()
	{
		guard(W2DShapeEditor::OnLeftButtonUp);

		ReleaseCapture();
		m_bDraggingVerts = FALSE;

		if( !m_bMouseHasMoved && !(GetAsyncKeyState(VK_CONTROL) & 0x8000) )
			DeselectAllVerts();
		ComputeHandlePositions(FALSE, TRUE);

		WWindow::OnLeftButtonUp();
		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(W2DShapeEditor::OnSize);
		PositionChildControls();
		WWindow::OnSize(Flags, NewX, NewY);
		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void OnChooseColorButton()
	{
		CHOOSECOLORA cc;
		static COLORREF acrCustClr[16];
		appMemzero(&cc, sizeof(cc));
		cc.lStructSize = sizeof(cc);
		cc.hwndOwner = hWnd;
		cc.lpCustColors = (LPDWORD)acrCustClr;
		cc.rgbResult = 0;
		cc.Flags = CC_FULLOPEN | CC_RGBINIT;

		if (ChooseColorA(&cc) == TRUE)
		{
			FColor ChosenLineColor = FColor(GetRValue(cc.rgbResult), GetGValue(cc.rgbResult), GetBValue(cc.rgbResult), 0);
			for (int shape = 0; shape < m_shapes.Num(); shape++)
				for (int seg = 0; seg < m_shapes(shape).Segments.Num(); seg++)
					if (m_shapes(shape).Segments(seg).IsSel())
						m_shapes(shape).Segments(seg).LineColor = ChosenLineColor;
			ComputeHandlePositions();
		}
	}
	void PositionChildControls( void )
	{
		guard(W2DShapeEditor::PositionChildControls);

		if( !::IsWindow( GetDlgItem( hWnd, ID_2DSE_TOOLBAR )))	return;

		LockWindowUpdate( hWnd );

		FRect CR = GetClientRect();
		::MoveWindow(hWndToolBar, 4, 0, 27 * MulDiv(16, DPIX, 96), 27 * MulDiv(16, DPIY, 96), 1);
		RECT R;
		if (::GetWindowRect(GetDlgItem(hWnd, ID_2DSE_TOOLBAR), &R))
		{
			::MoveWindow(GetDlgItem(hWnd, ID_2DSE_TOOLBAR), 0, 0, CR.Max.X, R.bottom, TRUE);
		}

		LockWindowUpdate( NULL );

		unguard;
	}
	virtual void OnKeyDown( TCHAR Ch )
	{
		guard(W2DShapeEditor::OnKeyDown);
		// Hot keys from old version
		if( Ch == 'I' && GetKeyState(VK_CONTROL) & 0x8000)
			SplitSides();
		if( Ch == VK_DELETE )
			Delete();
		unguard;
	}
	// Rotate the shapes by the speifued angle, around the origin,
	void RotateShape( int _Angle )
	{
		guard(W2DShapeEditor::RotateShape);
		FVector2D l_vec;
		FRotator StepRotation( 0, (65536.0f / 360.0f)  * _Angle, 0 );
		m_ShapeDirty = TRUE;

		for( int shape = 0 ; shape < m_shapes.Num() ; shape++ )
			for( int seg = 0 ; seg < m_shapes(shape).Segments.Num() ; seg++ )
				for( int vertex = 0 ; vertex < 2 ; vertex++ )
				{
					l_vec = m_shapes(shape).Segments(seg).Vertex[vertex];
					l_vec = m_origin + ( l_vec - m_origin ).Vector().TransformVectorBy( GMath.UnitCoords * StepRotation);

					m_shapes(shape).Segments(seg).Vertex[vertex].X = l_vec.X;
					m_shapes(shape).Segments(seg).Vertex[vertex].Y = l_vec.Y;
					m_shapes(shape).Segments(seg).Vertex[vertex].GridSnap();

					m_shapes(shape).Segments(seg).ControlPoint[vertex].X = l_vec.X;
					m_shapes(shape).Segments(seg).ControlPoint[vertex].Y = l_vec.Y;
					m_shapes(shape).Segments(seg).ControlPoint[vertex].GridSnap();
				}

		ComputeHandlePositions(1);
		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	// Flips the shape across the origin.
	void Flip(INT FlipDir, UBOOL bSelectionOnly = FALSE)
	{
		guard(W2DShapeEditor::Flip);

		// Flip the vertices across the origin.
		for( int shape = 0 ; shape < m_shapes.Num() ; shape++ )
		{
			if (bSelectionOnly && !m_shapes(shape).HasVertSelected())
				continue;

			for( int seg = 0 ; seg < m_shapes(shape).Segments.Num() ; seg++ )
			{
				if (FlipDir < 2)
				{
					for (int vertex = 0; vertex < 2; vertex++)
					{
						FVector2D Dist = m_shapes(shape).Segments(seg).Vertex[vertex] - m_origin;
						FVector2D CPDist = m_shapes(shape).Segments(seg).ControlPoint[vertex] - m_origin;

						if (FlipDir)
						{
							m_shapes(shape).Segments(seg).Vertex[vertex].X -= (Dist.X * 2);
							m_shapes(shape).Segments(seg).ControlPoint[vertex].X -= (CPDist.X * 2);
						}
						else
						{
							m_shapes(shape).Segments(seg).Vertex[vertex].Y -= (Dist.Y * 2);
							m_shapes(shape).Segments(seg).ControlPoint[vertex].Y -= (CPDist.Y * 2);
						}
					}
				}
				Exchange( m_shapes(shape).Segments(seg).Vertex[0], m_shapes(shape).Segments(seg).Vertex[1] );
				Exchange( m_shapes(shape).Segments(seg).ControlPoint[0], m_shapes(shape).Segments(seg).ControlPoint[1] );
			}
		}

		m_ShapeDirty = TRUE;
		ComputeHandlePositions(1);
		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void DeselectAllVerts()
	{
		guard(W2DShapeEditor::DeselectAllVerts);
		for( int shape = 0 ; shape < m_shapes.Num() ; shape++ )
		{
			m_shapes(shape).Handle.Select(0);
			for( int seg = 0 ; seg < m_shapes(shape).Segments.Num() ; seg++ )
			{
				m_shapes(shape).Segments(seg).Vertex[0].Select(0);
				m_shapes(shape).Segments(seg).Vertex[1].Select(0);
				m_shapes(shape).Segments(seg).ControlPoint[0].Select(0);
				m_shapes(shape).Segments(seg).ControlPoint[1].Select(0);
			}
		}
		m_origin.Select(0);
		unguard;
	}
	BOOL ProcessHits( BOOL bJustChecking, BOOL _bCumulative )
	{
		guard(W2DShapeEditor::ProcessHits);

		if( !_bCumulative )
			DeselectAllVerts();

		// Get the click position in world space.
		//
		FPoint l_click = GetCursorPos();

		l_click.X += -m_camera.X - (m_rcWnd.right / 2);
		l_click.Y += -m_camera.Y - (m_rcWnd.bottom / 2);

		//
		// See if any vertex comes within the selection radius to this point.  If so, select it...
		//

		// Check origin ...
		F2DSEVector l_vtxTest;
		l_vtxTest.X = (float)l_click.X - (m_origin.X * Zoom);
		l_vtxTest.Y = (float)l_click.Y + (m_origin.Y * Zoom);
		if( l_vtxTest.Size() <= d2dSE_SELECT_TOLERANCE )
		{
			if( !bJustChecking )
				m_origin.SelectToggle();
			return TRUE;
		}

		// Check shape handles...
		int shape;
		for( shape = 0 ; shape < m_shapes.Num() ; shape++ )
		{
			l_vtxTest.X = (float)l_click.X - (m_shapes(shape).Handle.X * Zoom);
			l_vtxTest.Y = (float)l_click.Y + (m_shapes(shape).Handle.Y * Zoom);

			if( l_vtxTest.Size() <= d2dSE_SELECT_TOLERANCE )
			{
				if( !bJustChecking )
					m_shapes(shape).Handle.SelectToggle();
				return TRUE;
			}
		}

		// Check vertices...
		UBOOL bClickedShape = FALSE;
		for( shape = 0 ; (shape < m_shapes.Num() && !bClickedShape); shape++ )
			for( int seg = 0 ; seg < m_shapes(shape).Segments.Num() ; seg++ )
				for( int vertex = 0 ; vertex < 2 ; vertex++ )
				{
					l_vtxTest.X = (float)l_click.X - (m_shapes(shape).Segments(seg).Vertex[vertex].X * Zoom);
					l_vtxTest.Y = (float)l_click.Y + (m_shapes(shape).Segments(seg).Vertex[vertex].Y * Zoom);

					if( l_vtxTest.Size() <= d2dSE_SELECT_TOLERANCE )
                    {
						if( bJustChecking )
							return TRUE;
						else
							m_shapes(shape).Segments(seg).Vertex[vertex].SelectToggle();
						bClickedShape = TRUE;
						continue;
                    }

					l_vtxTest.X = (float)l_click.X - (m_shapes(shape).Segments(seg).ControlPoint[vertex].X * Zoom);
					l_vtxTest.Y = (float)l_click.Y + (m_shapes(shape).Segments(seg).ControlPoint[vertex].Y * Zoom);

					if( l_vtxTest.Size() <= d2dSE_SELECT_TOLERANCE )
                    {
						if( bJustChecking )
							return TRUE;
						else
							m_shapes(shape).Segments(seg).ControlPoint[vertex].SelectToggle();
						bClickedShape = TRUE;
                    }
				}

		return FALSE;
		unguard;
	}
	void New( void )
	{
		guard(W2DShapeEditor::New);
		m_camera.X = m_camera.Y = 0;
		m_origin.X = 0; m_origin.Y = 0;

		m_shapes.Empty();
		InsertNewShape();
		m_ShapeDirty = FALSE;

		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void InsertNewShape()
	{
		guard(W2DShapeEditor::InsertNewShape);

		m_ShapeDirty = TRUE;
		F2DSEVector A(-128,-128), B(-128,128), C(128,128), D(128,-128);
		FShape* S = new(m_shapes)FShape();
		new(S->Segments)FSegment(A, B);
		new(S->Segments)FSegment(B, C);
		new(S->Segments)FSegment(C, D);
		new(S->Segments)FSegment(D, A);

		for (INT i = 0; i < 4; i++)
			S->Segments(i).Select();

		ComputeHandlePositions();

		unguard;
	}
	void InsertNewTriangle()
	{
		guard(W2DShapeEditor::InsertNewShape);

		m_ShapeDirty = TRUE;
		F2DSEVector A(-128, -128), B(-128, 128), C(128, 128);
		FShape* S = new(m_shapes)FShape();
		new(S->Segments)FSegment(A, B);
		new(S->Segments)FSegment(B, C);
		new(S->Segments)FSegment(C, A);

		for (INT i = 0; i < 3; i++)
			S->Segments(i).Select();

		ComputeHandlePositions();

		unguard;
	}
	void InsertNewCylinder( INT Sides )
	{
		guard(W2DShapeEditor::InsertNewShape);

		F2DSEVector Coords[64];

		if (Sides > 64)
		{
			appMsgf(TEXT("To many sides. Reducing to 64."));
			Sides = 64;
		}

		FShape* S = new(m_shapes)FShape();
		INT i = 0;
		for ( i = 0; i < Sides; i++)
			Coords[i] = F2DSEVector(128 * sin((2 * i + 1)*PI / Sides), 128 * cos((2 * i + 1)*PI / Sides));

		for (i = 0; i < Sides; i++)
		{
			if (i==Sides-1)
				new(S->Segments)FSegment(Coords[i], Coords[0]);
			else new(S->Segments)FSegment(Coords[i], Coords[i + 1]);
		}

		for (i = 0; i < Sides; i++)
			S->Segments(i).Select();

		m_ShapeDirty = TRUE;
		ComputeHandlePositions();

		unguard;
	}
	void DrawGrid( HDC _hdc )
	{
		guard(W2DShapeEditor::DrawGrid);
		FLOAT l_iXStart, l_iYStart, l_iXEnd, l_iYEnd;
		FVector l_vecTopLeft;
		HPEN l_penOriginLines, l_penMajorLines, l_penMinorLines, l_penOld;

		FLOAT DrawGridSize = GGridSizeZoom;
		while (DrawGridSize < 8.f)
			DrawGridSize *= 2.f;

		l_vecTopLeft.X = (-m_camera.X) - (m_rcWnd.right / 2);
		l_vecTopLeft.Y = (-m_camera.Y) - (m_rcWnd.bottom / 2);

		l_penMinorLines = CreatePen( PS_SOLID, 1, RGB( 235, 235, 235 ) );
		l_penMajorLines = CreatePen( PS_SOLID, 1, RGB( 215, 215, 215 ) );
		l_penOriginLines = CreatePen( PS_SOLID, 3, RGB( 225, 225, 225 ) );

		// Snap the starting position to the grid size.
		//
		l_iXStart = DrawGridSize - (l_vecTopLeft.X - appFloor(l_vecTopLeft.X / DrawGridSize) * DrawGridSize);
		l_iYStart = DrawGridSize - (l_vecTopLeft.Y - appFloor(l_vecTopLeft.Y / DrawGridSize) * DrawGridSize);

		l_iXEnd = l_iXStart + m_rcWnd.right;
		l_iYEnd = l_iYStart + m_rcWnd.bottom;

		// Draw the lines.
		//
		l_penOld = (HPEN)SelectObject( _hdc, l_penMinorLines );
		INT flpos;

		for( FLOAT y = l_iYStart ; y < l_iYEnd ; y += DrawGridSize)
		{
			if( Abs(l_vecTopLeft.Y + y) < 4.f)
				SelectObject( _hdc, l_penOriginLines );
			else
				if( !((int)(l_vecTopLeft.Y + y) % (int)(128 * Zoom)) )
					SelectObject( _hdc, l_penMajorLines );
				else
					SelectObject( _hdc, l_penMinorLines );

			flpos = appRound(y);
			::MoveToEx( _hdc, 0, flpos, NULL );
			::LineTo( _hdc, m_rcWnd.right, flpos);
		}

		for( FLOAT x = l_iXStart ; x < l_iXEnd ; x += DrawGridSize)
		{
			if( Abs(l_vecTopLeft.X + x) < 4.f)
				SelectObject( _hdc, l_penOriginLines );
			else
				if( !((int)(l_vecTopLeft.X + x) % (int)(128 * Zoom)) )
					SelectObject( _hdc, l_penMajorLines );
				else
					SelectObject( _hdc, l_penMinorLines );

			flpos = appRound(x);
			::MoveToEx( _hdc, flpos, 0, NULL );
			::LineTo( _hdc, flpos, m_rcWnd.bottom );
		}

		SelectObject( _hdc, l_penOld );
		DeleteObject( l_penOriginLines );
		DeleteObject( l_penMinorLines );
		DeleteObject( l_penMajorLines );
		unguard;
	}
	void DrawImage( HDC _hdc )
	{
		guard(W2DShapeEditor::DrawImage);
		if( !hImage ) return;

		HDC hdcMem;
		HBITMAP hbmOld;
		BITMAP bitmap;
		FVector l_vecLoc;

		l_vecLoc.X = m_camera.X + (m_rcWnd.right / 2);
		l_vecLoc.Y = m_camera.Y + (m_rcWnd.bottom / 2);

		// Prepare the bitmap.
		//
		GetObjectA( hImage, sizeof(BITMAP), (LPSTR)&bitmap );
		hdcMem = CreateCompatibleDC(_hdc);
		hbmOld = (HBITMAP)SelectObject(hdcMem, hImage);

		// Display it.
		//
		StretchBlt(_hdc,
			   l_vecLoc.X - ((bitmap.bmWidth * Zoom) / 2), l_vecLoc.Y - ((bitmap.bmHeight * Zoom) / 2),
			   bitmap.bmWidth * Zoom, bitmap.bmHeight * Zoom,
			   hdcMem,
			   0, 0,
			   bitmap.bmWidth, bitmap.bmHeight,
			   SRCCOPY);

		// Clean up.
		//
		SelectObject(hdcMem, hbmOld);
		DeleteDC(hdcMem);
		unguard;
	}
	void DrawOrigin( HDC _hdc )
	{
		guard(W2DShapeEditor::DrawOrigin);
		HPEN l_pen, l_penSel, l_penOld;

		FVector2D l_vecLoc = m_camera;
		l_vecLoc.X += m_rcWnd.right / 2;
		l_vecLoc.Y += m_rcWnd.bottom / 2;

		l_pen = CreatePen( PS_SOLID, 2, RGB( 0, 255, 0 ) );
		l_penSel = CreatePen( PS_SOLID, 4, RGB( 255, 0, 0 ) );

		l_penOld = (HPEN)SelectObject( _hdc, (m_origin.IsSel() ? l_penSel : l_pen) );
		Rectangle( _hdc,
			l_vecLoc.X + (m_origin.X * Zoom) - 4,
			l_vecLoc.Y - (m_origin.Y * Zoom) + 4,
			l_vecLoc.X + (m_origin.X * Zoom) + 4,
			l_vecLoc.Y - (m_origin.Y * Zoom) - 4 );

		SelectObject( _hdc, l_penOld );
		DeleteObject( l_pen );
		DeleteObject( l_penSel );
		unguard;
	}
	void DrawSegments( HDC _hdc )
	{
		guard(W2DShapeEditor::DrawSegments);
		FVector2D l_vecLoc = m_camera;
		HPEN l_penLine, l_penBold, l_penCP, l_penCPBold, l_penOld, l_penNormal;

		l_penLine = CreatePen( PS_SOLID, 1, RGB( 128, 128, 128 ) );
		l_penBold = CreatePen( PS_SOLID, 3, RGB( 128, 128, 128 ) );
		l_penCP = CreatePen( PS_SOLID, 1, RGB( 0, 0, 255 ) );
		l_penCPBold = CreatePen( PS_SOLID, 2, RGB( 0, 0, 255 ) );
		l_penNormal = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));

		// Figure out where the top left corner of the window is in world coords.
		//
		l_vecLoc.X += m_rcWnd.right / 2;
		l_vecLoc.Y += m_rcWnd.bottom / 2;

		l_penOld = (HPEN)SelectObject( _hdc, l_penLine );

		for( int shape = 0 ; shape < m_shapes.Num() ; shape++ )
		{
			for( int seg = 0 ; seg < m_shapes(shape).Segments.Num() ; seg++ )
			{
				const FSegment& Seq = m_shapes(shape).Segments(seg);
				switch (Seq.SegType)
				{
					case eSEGTYPE_LINEAR:
						if (Seq.IsSel())
							SelectObject( _hdc, l_penBold);
						else
						{
							SelectObject(_hdc, GetStockObject(DC_PEN));
							SetDCPenColor(_hdc, RGB(Seq.LineColor.R, Seq.LineColor.G, Seq.LineColor.B));
						}
						::MoveToEx( _hdc,
							l_vecLoc.X + (Seq.Vertex[0].X * Zoom),
							l_vecLoc.Y - (Seq.Vertex[0].Y * Zoom), NULL );
						::LineTo( _hdc,
							l_vecLoc.X + (Seq.Vertex[1].X * Zoom),
							l_vecLoc.Y - (Seq.Vertex[1].Y * Zoom) );

						// Draw normal
						{
							FVector2D LineDir = (Seq.Vertex[0] - Seq.Vertex[1]);
							FVector2D MidPoint = Seq.Vertex[1] + (LineDir * 0.5f);
							if (LineDir.Normalize())
							{
								MidPoint = FVector2D(l_vecLoc.X + (MidPoint.X * Zoom), l_vecLoc.Y - (MidPoint.Y * Zoom));
								LineDir = FVector2D(LineDir.Y, LineDir.X) * 12.f;
								SelectObject(_hdc, l_penNormal);
								::MoveToEx(_hdc,
									MidPoint.X,
									MidPoint.Y, NULL);
								::LineTo(_hdc,
									MidPoint.X + LineDir.X,
									MidPoint.Y + LineDir.Y);
							}
						}
						break;

					case eSEGTYPE_BEZIER:

						// Generate list of vertices for bezier curve and render them as a line.
						TArray<FVector2D> BezierPoints;
						Seq.GetBezierPoints(BezierPoints);
						if (Seq.IsSel())
							SelectObject(_hdc, l_penBold);
						else
						{
							SelectObject(_hdc, GetStockObject(DC_PEN));
							SetDCPenColor(_hdc, RGB(Seq.LineColor.R, Seq.LineColor.G, Seq.LineColor.B));
						}
						for( int bz = 0 ; bz < BezierPoints.Num() - 1 ; bz++ )
						{
							::MoveToEx( _hdc,
								l_vecLoc.X + (BezierPoints(bz).X * Zoom),
								l_vecLoc.Y - (BezierPoints(bz).Y * Zoom), NULL );
							::LineTo( _hdc,
								l_vecLoc.X + (BezierPoints(bz+1) * Zoom).X,
								l_vecLoc.Y - (BezierPoints(bz+1) * Zoom).Y );
						}

						// Draw normal
						if(BezierPoints.Num()>2)
						{
							INT bzMid = Clamp(BezierPoints.Num() >> 1, 1, BezierPoints.Num() - 1);
							FVector2D LineDir = (BezierPoints(bzMid - 1) - BezierPoints(bzMid));
							FVector2D MidPoint = BezierPoints(bzMid) + (LineDir * 0.5f);
							if (LineDir.Normalize())
							{
								MidPoint = FVector2D(l_vecLoc.X + (MidPoint.X * Zoom), l_vecLoc.Y - (MidPoint.Y * Zoom));
								LineDir = FVector2D(LineDir.Y, LineDir.X) * 12.f;
								SelectObject(_hdc, l_penNormal);
								::MoveToEx(_hdc,
									MidPoint.X,
									MidPoint.Y, NULL);
								::LineTo(_hdc,
									MidPoint.X + LineDir.X,
									MidPoint.Y + LineDir.Y);
							}
						}

						// Render the control points and connecting lines.
						SelectObject( _hdc, (Seq.ControlPoint[0].IsSel() ? l_penCPBold : l_penCP) );

						::MoveToEx( _hdc,
							l_vecLoc.X + (Seq.Vertex[0].X * Zoom),
							l_vecLoc.Y - (Seq.Vertex[0].Y * Zoom), NULL );
						::LineTo( _hdc,
							l_vecLoc.X + (Seq.ControlPoint[0].X * Zoom),
							l_vecLoc.Y - (Seq.ControlPoint[0].Y * Zoom) );

						SelectObject( _hdc, (Seq.ControlPoint[1].IsSel() ? l_penCPBold : l_penCP) );

						::MoveToEx( _hdc,
							l_vecLoc.X + (Seq.Vertex[1].X * Zoom),
							l_vecLoc.Y - (Seq.Vertex[1].Y * Zoom), NULL );
						::LineTo( _hdc,
							l_vecLoc.X + (Seq.ControlPoint[1].X * Zoom),
							l_vecLoc.Y - (Seq.ControlPoint[1].Y * Zoom) );
						break;
				}
			}
		}

		SelectObject( _hdc, l_penOld );
		DeleteObject( l_penLine );
		DeleteObject( l_penBold );
		DeleteObject( l_penCP );
		DeleteObject( l_penCPBold );
		DeleteObject( l_penNormal );
		unguard; 
	}
	void DrawShapeHandles( HDC _hdc )
	{
		guard(W2DShapeEditor::DrawShapeHandles);
		FVector2D l_vecLoc = m_camera;
		HPEN l_penLine, l_penBold, l_penOld;

		l_penLine = CreatePen( PS_SOLID, 1, RGB( 255, 128, 0 ) );
		l_penBold = CreatePen( PS_SOLID, 3, RGB( 255, 128, 0 ) );

		// Figure out where the top left corner of the window is in world coords.
		//
		l_vecLoc.X += m_rcWnd.right / 2;
		l_vecLoc.Y += m_rcWnd.bottom / 2;

		l_penOld = (HPEN)SelectObject( _hdc, l_penLine );

		for( int shape = 0 ; shape < m_shapes.Num() ; shape++ )
		{
			SelectObject( _hdc, (m_shapes(shape).Handle.IsSel() ? l_penBold : l_penLine) );
			Rectangle( _hdc,
				l_vecLoc.X + (m_shapes(shape).Handle.X * Zoom) - 4,
				l_vecLoc.Y - (m_shapes(shape).Handle.Y * Zoom) + 4,
				l_vecLoc.X + (m_shapes(shape).Handle.X * Zoom) + 4,
				l_vecLoc.Y - (m_shapes(shape).Handle.Y * Zoom) - 4 );
		}

		SelectObject( _hdc, l_penOld );
		DeleteObject( l_penLine );
		DeleteObject( l_penBold );
		unguard;
	}
	void DrawVertices( HDC _hdc )
	{
		guard(W2DShapeEditor::DrawVertices);
		FVector2D l_vecLoc = m_camera;
		HPEN l_penVertex, l_penVertexBold, l_penCP, l_penCPBold, l_penOld;

		l_penVertex = CreatePen( PS_SOLID, 1, RGB( 0, 0, 0 ) );
		l_penVertexBold = CreatePen( PS_SOLID, 3, RGB( 255, 0, 0 ) );
		l_penCP = CreatePen( PS_SOLID, 1, RGB( 0, 0, 255 ) );
		l_penCPBold = CreatePen( PS_SOLID, 3, RGB( 0, 0, 255 ) );

		// Figure out where the top left corner of the window is in world coords.
		//
		l_vecLoc.X += m_rcWnd.right / 2;
		l_vecLoc.Y += m_rcWnd.bottom / 2;

		// Draw the vertices.
		//
		l_penOld = (HPEN)SelectObject( _hdc, l_penVertex );
		HBRUSH OldBrush = (HBRUSH)SelectObject( _hdc, GetStockObject(WHITE_BRUSH) );

		for( int shape = 0 ; shape < m_shapes.Num() ; shape++ )
			for( int seg = 0 ; seg < m_shapes(shape).Segments.Num() ; seg++ )
				for( int vertex = 0 ; vertex < 2 ; vertex++ )
				{
					SelectObject( _hdc, (m_shapes(shape).Segments(seg).Vertex[vertex].IsSel() ? l_penVertexBold : l_penVertex) );
					Rectangle( _hdc,
						l_vecLoc.X + (m_shapes(shape).Segments(seg).Vertex[vertex].X * Zoom) - 4,
						l_vecLoc.Y - (m_shapes(shape).Segments(seg).Vertex[vertex].Y * Zoom) + 4,
						l_vecLoc.X + (m_shapes(shape).Segments(seg).Vertex[vertex].X * Zoom) + 4,
						l_vecLoc.Y - (m_shapes(shape).Segments(seg).Vertex[vertex].Y * Zoom) - 4 );

					if( m_shapes(shape).Segments(seg).SegType == eSEGTYPE_BEZIER )
					{
						SelectObject( _hdc, (m_shapes(shape).Segments(seg).ControlPoint[vertex].IsSel() ? l_penCPBold : l_penCP) );
						Rectangle( _hdc,
							l_vecLoc.X + (m_shapes(shape).Segments(seg).ControlPoint[vertex].X * Zoom) - 4,
							l_vecLoc.Y - (m_shapes(shape).Segments(seg).ControlPoint[vertex].Y * Zoom) + 4,
							l_vecLoc.X + (m_shapes(shape).Segments(seg).ControlPoint[vertex].X * Zoom) + 4,
							l_vecLoc.Y - (m_shapes(shape).Segments(seg).ControlPoint[vertex].Y * Zoom) - 4 );
					}
				}

		SelectObject( _hdc, l_penOld );
		SelectObject( _hdc, OldBrush );
		DeleteObject( l_penVertex );
		DeleteObject( l_penVertexBold );
		DeleteObject( l_penCP );
		DeleteObject( l_penCPBold );
		unguard;
	}
	// Splits all selected sides in half.
	void SplitSides( void )
	{
		guard(W2DShapeEditor::SplitSides);

		// Break each selected segment into two.
		for( int shape = 0 ; shape < m_shapes.Num() ; shape++ )
			for( int seg = 0 ; seg < m_shapes(shape).Segments.Num() ; seg++ )
				if( m_shapes(shape).Segments(seg).IsSel() )
				{
					// Create a new segment half the size of this one, starting from the middle and extending
					// to the second vertex.
					FVector2D HalfWay = m_shapes(shape).Segments(seg).GetHalfwayPoint();
					new(m_shapes(shape).Segments)FSegment(HalfWay, FVector2D(m_shapes(shape).Segments(seg).Vertex[1]));

					// Move the original segments ending point to the halfway point.
					m_shapes(shape).Segments(seg).Vertex[1] = HalfWay;
				}

		m_ShapeDirty = TRUE;
		ComputeHandlePositions();
		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void Delete( void )
	{
		guard(W2DShapeEditor::Delete);

		// Delete any vertices which are selected on the current shape.
		for( int shape = 0 ; shape < m_shapes.Num() ; shape++ )
		{
			if( m_shapes(shape).Handle.IsSel() )
			{
				m_shapes.Remove(shape--);
			}
			else
			{
				for (int seg = 0; seg < m_shapes(shape).Segments.Num(); seg++)
					if (m_shapes(shape).Segments(seg).IsSel())
					{
						FVector2D HalfWay = m_shapes(shape).Segments(seg).GetHalfwayPoint();
						F2DSEVector v1, v2;

						v1 = m_shapes(shape).Segments(seg).Vertex[0];
						v2 = m_shapes(shape).Segments(seg).Vertex[1];

						m_shapes(shape).Segments.Remove(seg);
						if (m_shapes(shape).Segments.Num() < 3) // Delete entire shape if not enough verts remaining.
						{
							m_shapes.Remove(shape--);
							break;
						}

						for (int sg = 0; sg < m_shapes(shape).Segments.Num(); sg++)
						{
							if (m_shapes(shape).Segments(sg).Vertex[0] == v2)
								m_shapes(shape).Segments(sg).Vertex[0] = HalfWay;
							if (m_shapes(shape).Segments(sg).Vertex[1] == v1)
								m_shapes(shape).Segments(sg).Vertex[1] = HalfWay;
						}
					}
			}
		}

		m_ShapeDirty = TRUE;
		ComputeHandlePositions();
		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void FileSaveAs( HWND hWnd )
	{
		guard(W2DShapeEditor::FileSaveAs);
		OPENFILENAMEA ofn;
		char File[8192] = "\0";
		strcpy_s( File, ARRAY_COUNT(File), appToAnsi( *MapFilename ) );

		ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
		ofn.lStructSize = sizeof(OPENFILENAMEA);
		ofn.hwndOwner = hWnd;
		ofn.lpstrFile = File;
		ofn.nMaxFile = sizeof(char) * 8192;
		ofn.lpstrFilter = "2D Shapes (*.2ds)\0*.2ds\0All Files\0*.*\0\0";
		ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_2DS]) );
		ofn.lpstrDefExt = "2ds";
		ofn.lpstrTitle = "Save 2D Shape";
		ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;

		// Display the Open dialog box.
		//
		if( GetSaveFileNameA(&ofn) )
		{
			MapFilename = appFromAnsi( File );
			WriteShape( MapFilename );

			GLastDir[eLASTDIR_2DS] = MapFilename.GetFilePath().LeftChop(1);
			bUntitledFile = FALSE;
		}

		SetCaption();
		unguard;
	}
	void FileSave( HWND hWnd )
	{
		guard(W2DShapeEditor::FileSave);
		if (!bUntitledFile)
			WriteShape( *MapFilename );
		else
			FileSaveAs( hWnd );
		unguard;
	}
	void FileOpen( HWND hWnd )
	{
		guard(W2DShapeEditor::FileOpen);
		OPENFILENAMEA ofn;
		char File[8192] = "\0";

		ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
		ofn.lStructSize = sizeof(OPENFILENAMEA);
		ofn.hwndOwner = hWnd;
		ofn.lpstrFile = File;
		ofn.nMaxFile = sizeof(char) * 8192;
		ofn.lpstrFilter = "2D Shapes (*.2ds)\0*.2ds\0All Files\0*.*\0\0";
		ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_2DS]) );
		ofn.lpstrDefExt = "2ds";
		ofn.lpstrTitle = "Open 2D Shape";
		ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

		// Display the Open dialog box.
		//
		if( GetOpenFileNameA(&ofn) )
		{
			MapFilename = appFromAnsi( File );
			ReadShape( MapFilename );
			SetCaption();

			GLastDir[eLASTDIR_2DS] = MapFilename.GetFilePath().LeftChop(1);
			bUntitledFile = FALSE;
		}
		unguard;
	}
	void SetCaption( void )
	{
		FString Caption = FString::Printf(TEXT("2D Shape Editor - [%ls]"), *MapFilename );
		SetText( *Caption );
	}
	void WriteShape( FString Filename )
	{
#if 1
		FStringOutputDevice Ar;

		Ar.Log(TEXT("BEGIN Model\r\n"));
		Ar.Logf(TEXT("\tOrigin=(%g,%g)\r\n"), m_origin.X, m_origin.Y); // Origin
		Ar.Logf(TEXT("\tCamera=(%g,%g)\r\n"), m_camera.X, m_camera.Y);
		Ar.Logf(TEXT("\tZoom=%g\r\n"), Zoom);
		Ar.Logf(TEXT("\tGridSize=%i\r\n"), GGridSize);
		if (BgImage && BgImage->IsValid())
			Ar.Logf(TEXT("\tTexture=%ls\r\n"), BgImage->GetPathName());
		else BgImage = NULL;

		// Shapes
		for (int shape = 0; shape < m_shapes.Num(); shape++)
		{
			Ar.Log(TEXT("\tBEGIN Shape\r\n"));
			FShape& Shape = m_shapes(shape);
			for (int seg = 0; seg < Shape.Segments.Num(); seg++)
			{
				FSegment& Seq = Shape.Segments(seg);
				Ar.Log(TEXT("\t\tBEGIN Segment\r\n"));
				Ar.Logf(TEXT("\t\t\tVertex1=(%g,%g)\r\n"), Seq.Vertex[0].X, Seq.Vertex[0].Y);
				Ar.Logf(TEXT("\t\t\tVertex2=(%g,%g)\r\n"), Seq.Vertex[1].X, Seq.Vertex[1].Y);
				Ar.Logf(TEXT("\t\t\tControlPoint1=(%g,%g)\r\n"), Seq.ControlPoint[0].X, Seq.ControlPoint[0].Y);
				Ar.Logf(TEXT("\t\t\tControlPoint2=(%g,%g)\r\n"), Seq.ControlPoint[1].X, Seq.ControlPoint[1].Y);
				Ar.Logf(TEXT("\t\t\tType=%i\r\n"), Seq.SegType);
				Ar.Logf(TEXT("\t\t\tDetail=%i\r\n"), Seq.DetailLevel);
				Ar.Log(TEXT("\t\tEND Segment\r\n"));
			}
			Ar.Log(TEXT("\tEND Shape\r\n"));
		}
		Ar.Log(TEXT("END Model\r\n"));

		if (appSaveStringToFile(Ar, *Filename))
			m_ShapeDirty = FALSE;
		else appMsgf(TEXT("Failed to write file: %ls"), *Filename);
#else // Old
		FArchive* Archive;
		Archive = GFileManager->CreateFileWriter( *Filename );

		if( Archive )
		{
			// Origin
			//
			*Archive << m_origin.X << m_origin.Y;

			// Shapes
			//
			int Num = m_shapes.Num();
			*Archive << Num;
			for( int shape = 0 ; shape < m_shapes.Num() ; shape++ )
			{
				Num = m_shapes(shape).Segments.Num();
				*Archive << Num;
				for( int seg = 0 ; seg < m_shapes(shape).Segments.Num() ; seg++ )
				{
					FSegment* pSeg = &(m_shapes(shape).Segments(seg));
					*Archive << pSeg->Vertex[0].X << pSeg->Vertex[0].Y << pSeg->Vertex[0].Z
						<< pSeg->Vertex[1].X << pSeg->Vertex[1].Y << pSeg->Vertex[1].Z
						<< pSeg->ControlPoint[0].X << pSeg->ControlPoint[0].Y << pSeg->ControlPoint[0].Z
						<< pSeg->ControlPoint[1].X << pSeg->ControlPoint[1].Y << pSeg->ControlPoint[1].Z
						<< pSeg->SegType << pSeg->DetailLevel;
				}
			}

			delete Archive;
			m_ShapeDirty = FALSE;
		}
#endif
	}
	void ReadShape( FString Filename )
	{
		guard(W2DShapeEditor::ReadShape);

		guard(ParseFileStr);
		FString Contents;
		if (appLoadFileToString(Contents, *Filename) && Contents.Len())
		{
			const TCHAR* Str = *Contents;
			const TCHAR* L;
			INT Command = INDEX_NONE;
			FString Value;
			INT iValue;
			FShape* EditShape = NULL;
			FSegment* EditSegment = NULL;
			while (*Str && Command != 42)
			{
				while (*Str == ' ' || *Str == '\t' || *Str == '\n' || *Str == '\r')
					++Str;
				L = Str;
				while (*Str && *Str != '\r' && *Str != '\n')
					++Str;

				FString Line(L, Str);
				if (*Str)
					++Str;

				L = *Line;
				switch (Command)
				{
				case INDEX_NONE:
					if (!appStricmp(L, TEXT("BEGIN Model")))
					{
						m_camera.X = m_camera.Y = 0;
						m_shapes.Empty();
						if (hImage)
						{
							DeleteObject(hImage);
							hImage = NULL;
							BgImage = NULL;
						}
						Command = 0;
						break;
					}
					break;
				case 0:
					if (ParseCommand(&L, TEXT("END")))
					{
						Command = 42;
						break;
					}
					else if (ParseCommand(&L, TEXT("BEGIN")))
					{
						if (!appStricmp(L, TEXT("Shape")))
						{
							Command = 1;
							EditShape = new(m_shapes)FShape();
						}
						else GWarn->Logf(TEXT("Import 2D shape: Invalid BEGIN %ls (%i)"), L, Command);
					}
					else if (Parse(L, TEXT("Origin="), Value))
					{
						ParseVector2D(*Value, m_origin.X, m_origin.Y);
					}
					else if (Parse(L, TEXT("Camera="), Value))
					{
						ParseVector2D(*Value, m_camera.X, m_camera.Y);
					}
					else if (Parse(L, TEXT("Zoom="), Zoom))
					{
						GGridSizeZoom = GGridSize * Zoom;
					}
					else if (Parse(L, TEXT("GridSize="), GGridSize))
					{
						GGridSizeZoom = GGridSize * Zoom;
					}
					else if (Parse(L, TEXT("Texture="), Value))
					{
						UTexture* Tex = LoadObject<UTexture>(NULL, *Value, NULL, LOAD_NoWarn, NULL);
						if (Tex)
							SetBackgroundImage(Tex);
					}
					else GWarn->Logf(TEXT("Import 2D shape, Invalid line: %ls (%i)"), L, Command);
					break;
				case 1:
					if (ParseCommand(&L, TEXT("END")))
					{
						Command = 0;
						break;
					}
					else if (ParseCommand(&L, TEXT("BEGIN")))
					{
						if (!appStricmp(L, TEXT("Segment")))
						{
							Command = 2;
							EditSegment = new(EditShape->Segments)FSegment;
						}
						else GWarn->Logf(TEXT("Import 2D shape: Invalid BEGIN %ls (%i)"), L, Command);
					}
					else GWarn->Logf(TEXT("Import 2D shape, Invalid line: %ls (%i)"), L, Command);
					break;
				case 2:
					if (ParseCommand(&L, TEXT("END")))
					{
						Command = 1;
						break;
					}
					else if (Parse(L, TEXT("Vertex1="), Value))
					{
						ParseVector2D(*Value, EditSegment->Vertex[0].X, EditSegment->Vertex[0].Y);
					}
					else if (Parse(L, TEXT("Vertex2="), Value))
					{
						ParseVector2D(*Value, EditSegment->Vertex[1].X, EditSegment->Vertex[1].Y);
					}
					else if (Parse(L, TEXT("ControlPoint1="), Value))
					{
						ParseVector2D(*Value, EditSegment->ControlPoint[0].X, EditSegment->ControlPoint[0].Y);
					}
					else if (Parse(L, TEXT("ControlPoint2="), Value))
					{
						ParseVector2D(*Value, EditSegment->ControlPoint[1].X, EditSegment->ControlPoint[1].Y);
					}
					else if (Parse(L, TEXT("Type="), iValue))
						EditSegment->SegType = iValue;
					else if (Parse(L, TEXT("Detail="), iValue))
						EditSegment->DetailLevel = iValue;
					else GWarn->Logf(TEXT("Import 2D shape, Invalid line: %ls (%i)"), L, Command);
					break;
				}
			}
			if (Command >= 0)
			{
				m_ShapeDirty = FALSE;
				InvalidateRect(hWnd, NULL, FALSE);
				return;
			}
		}
		unguard;

		guard(ParseFileBin);
		FArchive* Archive = GFileManager->CreateFileReader( *Filename );

		if( Archive )
		{
			m_camera.X = m_camera.Y = 0;
			m_shapes.Empty();

			// Origin
			//
			Archive->Serialize( &m_origin.X, sizeof(float) );
			Archive->Serialize( &m_origin.Y, sizeof(float) );


			// Shapes
			//
			int NumShapes;
			Archive->Serialize( &NumShapes, sizeof(int) );
			FLOAT DummyZ;

			for( int shape = 0 ; shape < NumShapes ; shape++ )
			{
				new(m_shapes)FShape();

				int NumSegments;
				Archive->Serialize( &NumSegments, sizeof(int) );

				for( int seg = 0 ; seg < NumSegments ; seg++ )
				{
					new(m_shapes(shape).Segments)FSegment;
					FSegment* pSeg = &(m_shapes(shape).Segments(seg));
					Archive->Serialize( &(pSeg->Vertex[0].X), sizeof(float) );
					Archive->Serialize( &(pSeg->Vertex[0].Y), sizeof(float) );
					Archive->Serialize( &DummyZ, sizeof(float) );
					Archive->Serialize( &(pSeg->Vertex[1].X), sizeof(float) );
					Archive->Serialize( &(pSeg->Vertex[1].Y), sizeof(float) );
					Archive->Serialize( &DummyZ, sizeof(float) );
					Archive->Serialize( &(pSeg->ControlPoint[0].X), sizeof(float) );
					Archive->Serialize( &(pSeg->ControlPoint[0].Y), sizeof(float) );
					Archive->Serialize( &DummyZ, sizeof(float) );
					Archive->Serialize( &(pSeg->ControlPoint[1].X), sizeof(float) );
					Archive->Serialize( &(pSeg->ControlPoint[1].Y), sizeof(float) );
					Archive->Serialize( &DummyZ, sizeof(float) );
					Archive->Serialize( &(pSeg->SegType), sizeof(float) );
					Archive->Serialize( &(pSeg->DetailLevel), sizeof(float) );
				}
				ComputeHandlePositions();
			}

			delete Archive;
			m_ShapeDirty = FALSE;
		}
		unguard;

		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void SetOrigin( void )
	{
		POINT l_click = m_ContextPos;

		l_click.x += -m_camera.X - (m_rcWnd.right / 2);
		l_click.y += -m_camera.Y - (m_rcWnd.bottom / 2);

		l_click.x -= l_click.x % GGridSize;
		l_click.y -= l_click.y % GGridSize;

		m_origin.X = l_click.x;
		m_origin.Y = -l_click.y;

		InvalidateRect( hWnd, NULL, FALSE );
	}
	void TesselateShape(F2DTessellator& T)
	{
		guardSlow(W2DShapeEditor::TesselateShape);
		for (INT shape = 0; shape < m_shapes.Num(); shape++)
		{
			m_shapes(shape).PopulateTesselator(T);
		}
		T.Tesselate();

		//
		// Massage the vertices/triangles before leaving ...
		//
		const FVector2D vOrigin(m_origin);
		for (INT vertex = 0; vertex < T.Vertices.Num(); vertex++)
		{
			T.Vertices(vertex) -= vOrigin;
		}
		unguardSlow;
	}
	void ProcessSheet()
	{
		guard(W2DShapeEditor::ProcessSheet);

		FStringOutputDevice Cmd;

		// Reverse the Y Axis
		Flip(0);

		Cmd.Log(TEXT("BRUSH SET\n\n"));

		F2DTessellator T;
		TesselateShape(T);
		INT a, b, c, i;
		const FVector2D* V = &T.Vertices(0);

		for (i = 0; i < T.Tris.Num(); ++i)
		{
			const auto& ptri = T.Tris(i);
			a = ptri.Verts[1];
			b = ptri.Verts[0];
			c = ptri.Verts[2];

			Cmd.Logf(TEXT("Begin Polygon Flags=%i\n"), INT(PF_NotSolid));

			Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, 0.0\n"), V[a].X, V[a].Y);
			Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, 0.0\n"), V[b].X, V[b].Y);
			Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, 0.0\n"), V[c].X, V[c].Y);

			Cmd.Log(TEXT("End Polygon\n"));
		}

		Flip(0);

		GEditor->Exec( *Cmd );
		unguard;
	}
	void ProcessExtrude( int Depth )
	{
		guard(W2DShapeEditor::ProcessExtrude)

		// Reverse the Y Axis
		Flip(0);

		FStringOutputDevice Cmd;

		Cmd.Log(TEXT("BRUSH SET\n\n"));

		F2DTessellator T;
		TesselateShape(T);
		INT a, b, c, i;
		const FVector2D* V = &T.Vertices(0);

		// Top and bottom caps ...
		//
		for (i = 0; i < T.Tris.Num(); ++i)
		{
			const auto& ptri = T.Tris(i);
			a = ptri.Verts[0];
			b = ptri.Verts[1];
			c = ptri.Verts[2];

			Cmd.Log(TEXT("Begin Polygon Flags=0\n"));

			Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), V[a].X, V[a].Y, Depth / 2.0f);
			Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), V[b].X, V[b].Y, Depth / 2.0f);
			Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), V[c].X, V[c].Y, Depth / 2.0f);

			Cmd.Log(TEXT("End Polygon\n"));

			Exchange(a, b);

			Cmd.Log(TEXT("Begin Polygon Flags=0\n"));

			Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), V[a].X, V[a].Y, -(Depth / 2.0f));
			Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), V[b].X, V[b].Y, -(Depth / 2.0f));
			Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), V[c].X, V[c].Y, -(Depth / 2.0f));

			Cmd.Log(TEXT("End Polygon\n"));
		}

		// Sides ...
		//
		for (INT j = 0; j < T.Shapes.Num(); ++j)
		{
			const auto& Shape = T.Shapes(j);
			const INT NumLinks = Shape.Links.Num();
			const INT* iRef = &Shape.Links(0);
			for (i = 0; i < NumLinks; ++i)
			{
				Cmd.Log(TEXT("Begin Polygon Flags=0\n"));

				const FVector2D& pvtxPrev = V[iRef[i ? (i - 1) : (NumLinks - 1)]];
				const FVector2D& pvtx = V[iRef[i]];

				Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), pvtx.X, pvtx.Y, Depth / 2.0f);
				Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), pvtxPrev.X, pvtxPrev.Y, Depth / 2.0f);
				Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), pvtxPrev.X, pvtxPrev.Y, -(Depth / 2.0f));
				Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), pvtx.X, pvtx.Y, -(Depth / 2.0f));

				Cmd.Log(TEXT("End Polygon\n"));
			}
		}

		GEditor->Exec( *Cmd );

		Flip(0);

		unguard;
	}
	void ProcessExtrudeToPoint( int Depth )
	{
		guard(W2DShapeEditor::ProcessExtrudeToPoint)

		// Flip the Y Axis
		Flip(0);

		FStringOutputDevice Cmd;

		Cmd.Log(TEXT("BRUSH SET\n\n"));

		F2DTessellator T;
		TesselateShape(T);
		INT a, b, c, i;
		const FVector2D* V = &T.Vertices(0);

		for (i = 0; i < T.Tris.Num(); ++i)
		{
			const auto& ptri = T.Tris(i);
			a = ptri.Verts[1];
			b = ptri.Verts[0];
			c = ptri.Verts[2];

			Cmd.Log(TEXT("Begin Polygon Flags=0\n"));

			Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, 0.0\n"), V[a].X, V[a].Y);
			Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, 0.0\n"), V[b].X, V[b].Y);
			Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, 0.0\n"), V[c].X, V[c].Y);

			Cmd.Log(TEXT("End Polygon\n"));
		}

		// Sides ...
		//
		for (INT j = 0; j < T.Shapes.Num(); ++j)
		{
			const auto& Shape = T.Shapes(j);
			const INT NumLinks = Shape.Links.Num();
			const INT* iRef = &Shape.Links(0);
			for (i = 0; i < NumLinks; ++i)
			{
				Cmd.Log(TEXT("Begin Polygon Flags=0\n"));

				const FVector2D& pvtxPrev = V[iRef[i ? (i - 1) : (NumLinks - 1)]];
				const FVector2D& pvtx = V[iRef[i]];

				Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, 0.0\n"), pvtxPrev.X, pvtxPrev.Y);
				Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, 0.0\n"), pvtx.X, pvtx.Y);
				Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), m_origin.X, m_origin.Y, (FLOAT)Depth);

				Cmd.Log(TEXT("End Polygon\n"));
			}
		}

		GEditor->Exec( *Cmd );

		Flip(0);

		unguard;
	}
	void ProcessExtrudeToBevel( int Depth, int CapHeight )
	{
		guard(W2DShapeEditor::ProcessExtrudeToBevel)

		// Flip the Y Axis
		Flip(0);

		FStringOutputDevice Cmd;
		float Dist = 1.0f - (CapHeight / (float)Depth);

		Cmd.Log(TEXT("BRUSH SET\n\n"));

		F2DTessellator T;
		TesselateShape(T);
		INT a, b, c, i;
		const FVector2D* V = &T.Vertices(0);

		for (i = 0; i < T.Tris.Num(); ++i)
		{
			const auto& ptri = T.Tris(i);
			a = ptri.Verts[1];
			b = ptri.Verts[0];
			c = ptri.Verts[2];

			Cmd.Log(TEXT("Begin Polygon Flags=0\n"));

			Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, 0.0\n"), V[a].X, V[a].Y);
			Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, 0.0\n"), V[b].X, V[b].Y);
			Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, 0.0\n"), V[c].X, V[c].Y);

			Cmd.Log(TEXT("End Polygon\n"));

			Exchange(a, b);

			Cmd.Log(TEXT("Begin Polygon Flags=0\n"));

			Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), V[a].X * Dist, V[a].Y * Dist, (float)CapHeight);
			Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), V[b].X * Dist, V[b].Y * Dist, (float)CapHeight);
			Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), V[c].X * Dist, V[c].Y * Dist, (float)CapHeight);

			Cmd.Log(TEXT("End Polygon\n"));
		}

		// Sides ...
		//
		for (INT j = 0; j < T.Shapes.Num(); ++j)
		{
			const auto& Shape = T.Shapes(j);
			const INT NumLinks = Shape.Links.Num();
			const INT* iRef = &Shape.Links(0);
			for (i = 0; i < NumLinks; ++i)
			{
				Cmd.Log(TEXT("Begin Polygon Flags=0\n"));

				const FVector2D& pvtxPrev = V[iRef[i ? (i - 1) : (NumLinks - 1)]];
				const FVector2D& pvtx = V[iRef[i]];

				Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, 0.0\n"), pvtxPrev.X, pvtxPrev.Y);
				Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, 0.0\n"), pvtx.X, pvtx.Y);
				Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), pvtx.X * Dist, pvtx.Y * Dist, (float)CapHeight);
				Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), pvtxPrev.X * Dist, pvtxPrev.Y * Dist, (float)CapHeight);

				Cmd.Log(TEXT("End Polygon\n"));
			}
		}

		GEditor->Exec( *Cmd );

		Flip(0);

		unguard;
	}
	void ProcessRevolve( int TotalSides, int Sides )
	{
		guard(W2DShapeEditor::ProcessRevolve)

		BOOL bPositive, bNegative/*, bBrushSetDone */= FALSE;
		BOOL bFromLeftSide;

		// Make sure the origin is totally to the left or right of the shape.
		bPositive = bNegative = FALSE;
		for( int shape = 0 ; shape < m_shapes.Num() ; shape++ )
			for( int seg = 0 ; seg < m_shapes(shape).Segments.Num() ; seg++ )
				for( int vtx = 0 ; vtx < 2 ; vtx++ )
				{
					if( m_origin.X > m_shapes(shape).Segments(seg).Vertex[vtx].X )		bPositive = TRUE;
					if( m_origin.X < m_shapes(shape).Segments(seg).Vertex[vtx].X )		bNegative = TRUE;
				}

		if( bPositive && bNegative )
		{
			appMsgf( TEXT("Origin must be completely to the left or right side of the shape.") );
			return;
		}

		// When revolving from the left side, we have to flip the polys around.
		bFromLeftSide = ( bNegative && !bPositive );

		// Flip the Y Axis
		Flip(0);

		FStringOutputDevice Cmd;

		Cmd.Log(TEXT("BRUSH SET\n\n"));

		const FVector2D* pvtx, * pvtxPrev;
		FCoords StepRotation, StepRotation2;

		F2DTessellator T;
		TesselateShape(T);
		INT a, b, c, i;
		const FVector2D* V = &T.Vertices(0);

		if( Sides != TotalSides )	// Don't make end caps for a complete revolve
		{
			StepRotation = GMath.UnitCoords * FRotator((65536.0f / TotalSides) * Sides, 0, 0);
			for (i = 0; i < T.Tris.Num(); ++i)
			{
				const auto& ptri = T.Tris(i);
				if (bFromLeftSide)
				{
					a = ptri.Verts[1];
					b = ptri.Verts[0];
				}
				else
				{
					a = ptri.Verts[0];
					b = ptri.Verts[1];
				}
				c = ptri.Verts[2];

				Cmd.Log(TEXT("Begin Polygon Flags=0\n"));

				Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, 0.0\n"), V[a].X, V[a].Y);
				Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, 0.0\n"), V[b].X, V[b].Y);
				Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, 0.0\n"), V[c].X, V[c].Y);

				Cmd.Log(TEXT("End Polygon\n"));

				// Flip/Rotate the triangle to create the end cap
				Exchange(a, b);

				FVector vtx1 = V[a].Vector().TransformVectorBy( StepRotation );
				FVector vtx2 = V[b].Vector().TransformVectorBy( StepRotation );
				FVector vtx3 = V[c].Vector().TransformVectorBy( StepRotation );

				Cmd.Log(TEXT("Begin Polygon Flags=0\n"));

				Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), vtx1.X, vtx1.Y, vtx1.Z);
				Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), vtx2.X, vtx2.Y, vtx2.Z);
				Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), vtx3.X, vtx3.Y, vtx3.Z);

				Cmd.Log(TEXT("End Polygon\n"));
			}
		}

		// Sides ...
		//
		for (int side = 0; side < Sides; side++)
		{
			StepRotation = GMath.UnitCoords * FRotator((65536.0f / TotalSides) * side, 0, 0);
			StepRotation2 = GMath.UnitCoords * FRotator((65536.0f / TotalSides) * (side + 1), 0, 0);

			for (INT j = 0; j < T.Shapes.Num(); ++j)
			{
				const auto& Shape = T.Shapes(j);
				const INT NumLinks = Shape.Links.Num();
				const INT* iRef = &Shape.Links(0);
				for (i = 0; i < NumLinks; ++i)
				{
					Cmd.Log(TEXT("Begin Polygon Flags=0\n"));

					pvtxPrev = &V[iRef[i ? (i - 1) : (NumLinks - 1)]];
					pvtx = &V[iRef[i]];

					if (bFromLeftSide)	Exchange(pvtxPrev, pvtx);

					FVector vtxPrev = pvtxPrev->Vector().TransformVectorBy(StepRotation);
					FVector fvtx = pvtx->Vector().TransformVectorBy(StepRotation);
					FVector vtxPrev2 = pvtxPrev->Vector().TransformVectorBy(StepRotation2);
					FVector vtx2 = pvtx->Vector().TransformVectorBy(StepRotation2);

					Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), fvtx.X, fvtx.Y, fvtx.Z);
					Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), vtxPrev.X, vtxPrev.Y, vtxPrev.Z);
					Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), vtxPrev2.X, vtxPrev2.Y, vtxPrev2.Z);
					Cmd.Logf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), vtx2.X, vtx2.Y, vtx2.Z);

					Cmd.Log(TEXT("End Polygon\n"));
				}
			}
		}

		GEditor->Exec( *Cmd );

		Flip(0);

		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
