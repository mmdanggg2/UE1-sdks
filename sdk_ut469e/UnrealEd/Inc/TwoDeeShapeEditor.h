/*=============================================================================
	TwoDeeShapeEditor : 2D Shape Editor conversion from VB code
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall
		* Added pieces of code from UDelaunay project
		  - arbitrary shape triangulation code
		  - support for bezier segments

=============================================================================*/

#include <stdio.h>
#include <math.h>
extern FString GLastDir[eLASTDIR_MAX];

int GGridSize = 16, GGridSizeZoom = 16;
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
		DepthEdit.SetSelection(0, -1);

		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgExtrude::OnDestroy);
		WDialog::OnDestroy();
		unguard;
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
		DepthEdit.SetSelection(0, -1);

		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgExtrudeToPoint::OnDestroy);
		WDialog::OnDestroy();
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
		DepthEdit.SetSelection(0, -1);

		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgExtrudeToBevel::OnDestroy);
		WDialog::OnDestroy();
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
		TotalSidesEdit.SetSelection(0, -1);

		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgRevolve::OnDestroy);
		WDialog::OnDestroy();
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
		::SetFocus(ValueEdit.hWnd);
		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgCustomDetail::OnDestroy);
		WDialog::OnDestroy();
		::DestroyWindow( hWnd );
		unguard;
	}
	virtual int DoModal()
	{
		guard(WDlgCustomDetail::DoModal);
		return WDialog::DoModal( hInstance );
		unguard;
	}

	void OnOK()
	{
		guard(WDlgCustomDetail::OnOK);
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
	WEdit EditRadius;
	WEdit EditSides;
	WCheckBox CheckAlign;

	// Variables.
	FLOAT Radius;
	INT Sides;
	BOOL AlignToSide;

	// Constructor.
	WDlgCylinderShape(UObject* InContext, WWindow* InOwnerWindow)
		: WDialog(TEXT("Cylinder Shape Properties"), IDDIALOG_2DSE_CYLINDER, InOwnerWindow)
		, CancelButton(this, IDCANCEL, FDelegate(this, (TDelegate)&WDialog::EndDialogFalse))
		, OKButton(this, IDOK, FDelegate(this, (TDelegate)&WDlgCylinderShape::OnOK))
		, EditRadius(this, IDEC_VALUE)
		, EditSides(this, IDEC_SIDES)
		, CheckAlign(this, IDCK_ALIGN)
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgCylinderShape::OnInitDialog);
		WDialog::OnInitDialog();
		EditRadius.SetText(TEXT("128"));
		::SetFocus(EditRadius.hWnd);
		EditRadius.SetSelection(0, -1);
		EditSides.SetText(TEXT("8"));
		CheckAlign.SetCheck(BST_CHECKED);
		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgCylinderShape::OnDestroy);
		WDialog::OnDestroy();
		unguard;
	}
	virtual int DoModal()
	{
		guard(WDlgCylinderShape::DoModal);
		return WDialog::DoModal(hInstance);
		unguard;
	}

	void OnOK()
	{
		guard(WDlgCylinderShape::OnOK);
		Radius = appAtof(*EditRadius.GetText());
		Sides = appAtoi(*EditSides.GetText());
		AlignToSide = CheckAlign.IsChecked();
		EndDialogTrue();
		unguard;
	}
};

// --------------------------------------------------------------
//
// DATA TYPES
//
// --------------------------------------------------------------

class F2DSEVector : public FVector
{
public:
	F2DSEVector()
	{
		X = Y = Z = 0;
		bSelected = 0;
	}
	F2DSEVector( float x, float y, float z)
		: FVector( x, y, z )
	{
		bSelected = 0;
	}
	~F2DSEVector()
	{}

	inline F2DSEVector operator=( F2DSEVector Other )
	{
		X = Other.X;
		Y = Other.Y;
		Z = Other.Z;
		bSelected = Other.bSelected;
		return *this;
	}
	inline F2DSEVector operator=( FVector Other )
	{
		X = Other.X;
		Y = Other.Y;
		Z = Other.Z;
		return *this;
	}
	inline UBOOL operator!=( F2DSEVector Other )
	{
		return (X != Other.X && Y != Other.Y && Z != Other.Z);
	}
	inline UBOOL operator==( F2DSEVector Other )
	{
		return (X == Other.X && Y == Other.Y && Z == Other.Z);
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
	BOOL IsSel( void )
	{
		return bSelected;
	}
	void SetTempPos()
	{
		TempX = X;
		TempY = Y;
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
	F2DSEVector Vertex[2], ControlPoint[2];
	FColor LineColor = FColor(128, 128, 128, 0);
	int SegType = eSEGTYPE_LINEAR,		// eSEGTYPE_
		DetailLevel = 10;	// Detail of bezier curve
	UBOOL bUsed{};

	FSegment()
	{}
	FSegment( F2DSEVector vtx1, F2DSEVector vtx2 )
	{
		Vertex[0] = vtx1;
		Vertex[1] = vtx2;
	}
	FSegment( FVector vtx1, FVector vtx2 )
	{
		Vertex[0] = vtx1;
		Vertex[1] = vtx2;
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
	FVector GetHalfwayPoint()
	{
		guard(FSegment::GetHalfwayPoint);

		FVector Dir = Vertex[1] - Vertex[0];
		int Size = Dir.Size();
		Dir.Normalize();

		FVector HalfWay = Vertex[0] + (Dir * (Size / 2));
		HalfWay = HalfWay.GridSnap( FVector( GGridSize, GGridSize, GGridSize ) );
		return HalfWay;

		unguard;
	}
	void GenerateControlPoint()
	{
		guard(FSegment::GetOneThird);

		FVector Dir = Vertex[1] - Vertex[0];
		int Size = Dir.Size();
		Dir.Normalize();

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
	UBOOL IsSel()
	{
		guard(FSegment::IsSel);
		return ( Vertex[0].IsSel() || ControlPoint[0].IsSel() || ControlPoint[1].IsSel() );
		unguard;
	}
	void GetBezierPoints( TArray<FVector>* pBezierPoints )
	{
		guard(FSegment::GetBezierPoints);
		F2DSEVector ccp[4];
		ccp[0] = Vertex[0];
		ccp[1] = ControlPoint[0];
		ccp[2] = ControlPoint[1];
		ccp[3] = Vertex[1];
		F2DSEVector pt;

		pBezierPoints->Empty();
		(*pBezierPoints)(pBezierPoints->Add()) = Vertex[0];
		for( int n = 1 ; n <= DetailLevel ; ++n )
		{
			double nD = (double)n / (DetailLevel + 1);
			Curve( nD, ccp, pt );
			(*pBezierPoints)(pBezierPoints->Add()) = pt;
		}
		(*pBezierPoints)(pBezierPoints->Add()) = Vertex[1];
		unguard;
	}
	// Get point on cubic bezier curve (slow)
	void Curve( double nP, F2DSEVector ControlPoint[4], F2DSEVector& Position )
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
	void CurveBlendingFunction( double nP, double pBf[4] )
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
};

#define POSITION int

struct CPOLY_TREENODE
{
	int Vertex;
	POSITION NodeLeft;
	POSITION NodeRight;
};
struct CPOLY_QUEUEELEMENT
{
	int Edge;
	int Vertex;
	double Gamma;
};
struct CPOLY_VERTEX
{
	FVector XY;
	int FirstEdge;
	int EdgeCount;
};
struct CPOLY_SIDE
{
	int Vertex;
	int Triangle;
	int NextEdge;
};
struct CPOLY_EDGE
{
	CPOLY_SIDE SideLeft;
	CPOLY_SIDE SideRight;
	FVector BMin;
	FVector BMax;
};
struct STACKENTRY
{
	POSITION Node;
	int Dir;
};
struct CPOLY_TRIANGLE
{
	int Index;
	int Vertex1;
	int Vertex2;
	int Vertex3;
};
#define MAX_VERTS_2D 1200
#define MAX_COORD 10.0e20f

// A shape is a closed series of segments (i.e. a triangle is 3 segments).
class FShape
{
public:
	FShape()
	{
	}
	~FShape()
	{}

	TArray<CPOLY_QUEUEELEMENT> m_queue;
	TArray<CPOLY_TREENODE> m_tree;
	TArray<CPOLY_VERTEX> Vertices;
	TArray<CPOLY_EDGE> Edges;
	TMap<int, CPOLY_TRIANGLE> Triangles;
	static TArray<STACKENTRY> stack;

	void ComputeHandlePosition()
	{
		guard(FShape::ComputeHandlePosition);
		Handle = FVector(0, 0, 0);
		for( int seg = 0 ; seg < Segments.Num() ; seg++ )
			Handle += Segments(seg).Vertex[0];
		Handle /= Segments.Num();
		unguard;
	}
	void SelectSegment(INT Index, BOOL bSel)
	{
		guard(FShape::SelectSegment);
		if (Index < 0 || Index > Segments.Num())
			return;
		F2DSEVector& Point = Segments(Index).Vertex[0];
		Point.Select(bSel);
		F2DSEVector& PointBefore = Segments((Segments.Num() + Index - 1) % Segments.Num()).Vertex[1];
		if (PointBefore == Point)
			PointBefore.Select(bSel);
		F2DSEVector& PointAfter = Segments((Index + 1) % Segments.Num()).Vertex[1];
		if (PointAfter == Point)
			PointAfter.Select(bSel);
		unguard;
	}
	// Must be called before Tesselate
	UBOOL IsConvex()
	{
		guard(FShape::IsConvex);
		if (Vertices.Num() > 3)
		{
			INT ZSign = -1;
			INT n = Vertices.Num();
			for (INT i = 0; i < n; i++)
			{
				FVector d1 = Vertices((i + 2) % n).XY - Vertices((i + 1) % n).XY;
				FVector d2 = Vertices(i).XY - Vertices((i + 1) % n).XY;
				FLOAT Cross = d1.X*d2.Y - d1.Y*d2.X;
				if (Abs(Cross) < KINDA_SMALL_NUMBER)
					continue;
				if (ZSign == -1)
					ZSign = Cross > 0;
				else if (ZSign != (Cross > 0))
					return FALSE;
			}
		}
		return TRUE;
		unguard;
	}
	int AddEdge( int nVertex1, int nVertex2 )
	{
		guard(FShape::AddEdge);
		CPOLY_EDGE edge;
		CPOLY_SIDE& l = edge.SideLeft;
		CPOLY_SIDE& r = edge.SideRight;
		CPOLY_VERTEX& v1 = Vertices(nVertex1);
		CPOLY_VERTEX& v2 = Vertices(nVertex2);

		FBox bbox(v1.XY, v2.XY);
		edge.BMin = bbox.Min;
		edge.BMax = bbox.Max;

		l.Vertex = nVertex1;
		l.Triangle = -1;
		l.NextEdge = v1.FirstEdge;
		r.Vertex = nVertex2;
		r.Triangle = -1;
		r.NextEdge = v2.FirstEdge;

		int nEdge = Edges.Add();
		Edges(nEdge) = edge;

		v1.FirstEdge = nEdge;
		v1.EdgeCount++;
		v2.FirstEdge = nEdge;
		v2.EdgeCount++;

		return nEdge;
		unguard;
	}
	void InitializeBoundary( void )
	{
		guard(FShape::InitializeBoundary);
		int n, nV1, nV2, nE;
		
		int BoundaryVertexCount = Vertices.Num();

		Edges.Empty();

		for( n = 0; n < BoundaryVertexCount; ++n ) {
			Vertices(n).FirstEdge = -1;
			Vertices(n).EdgeCount = 0;
		}
		for( n = 0; n < BoundaryVertexCount; ++n ) {
			nV1 = n;
			nV2 = (n + 1) % BoundaryVertexCount;
			nE = AddEdge( nV1, nV2 );
		}
		unguard;
	}
	int FindEdge( int nVertex1, int nVertex2 )
	{	
		guard(FShape::FindEdge);
		int nFound = -1;
		int nEdge = Vertices(nVertex1).FirstEdge;
		for( int n = 0; n < Vertices(nVertex1).EdgeCount; ++n ) {
			if( nEdge < 0 )
				break;
			if( Edges(nEdge).SideLeft.Vertex == nVertex1 ) {
				if( Edges(nEdge).SideRight.Vertex == nVertex2 ) {
					nFound = nEdge;
					break;
				}
				nEdge = Edges(nEdge).SideLeft.NextEdge;
			} else {
				if( Edges(nEdge).SideLeft.Vertex == nVertex2 ) {
					nFound = nEdge;
					break;
				}
				nEdge = Edges(nEdge).SideRight.NextEdge;
			}
		}
		return nFound;
		unguard;
	}
	UBOOL FindSubTriangle( int nVertex1, int nVertex2, int nVertex3, int& nKey )
	{
		guard(FShape::FindSubTriangle);
		int nV1 = nVertex1;
		int nV2 = nVertex2;
		int nV3 = nVertex3;
		int nTemp;
		if( nV2 < nV1 ) { nTemp = nV1; nV1 = nV2; nV2 = nTemp; }
		if( nV3 < nV2 ) { nTemp = nV2; nV2 = nV3; nV3 = nTemp; }
		if( nV2 < nV1 ) { nTemp = nV1; nV1 = nV2; nV2 = nTemp; }

		nKey = (nV1 * MAX_VERTS_2D + nV2) * MAX_VERTS_2D + nV3;
		
		return (Triangles.Find( nKey ) != NULL);
		unguard;
	}
	int AddSubTriangle( int nVertex1, int nVertex2, int nVertex3 )
	{
		guard(FShape::AddSubTriangle);
		int nKey;
		UBOOL bFound = FindSubTriangle( nVertex1, nVertex2, nVertex3, nKey );
		if( !bFound ) {
			CPOLY_TRIANGLE tri;
			tri.Index = Triangles.Num();
			tri.Vertex1 = nVertex1;
			tri.Vertex2 = nVertex2;
			tri.Vertex3 = nVertex3;
			Triangles.Set( nKey, tri );
		}
		return nKey;
		unguard;
	}
	void ClearTesselation( void )
	{
		guard(FShape::ClearTesselation);
		Edges.Empty();
		Triangles.Empty();
		unguard;
	}
	// Breaks up the shape into triangles using the Delaunay triangulation method.
	void Tesselate( F2DSEVector InOrigin )
	{
		guard(FShape::Tesselate);

		ClearTesselation();

		CPOLY_TREENODE tn = { -1, -1, -1 };
		m_tree.Empty();
		POSITION posHead = m_tree.Add();
		m_tree(posHead) = tn;

		// Build a CCW list of the vertices that make up this shape.
		for( int seg = 0 ; seg < Segments.Num() ; seg++ )
			Segments(seg).bUsed = 0;

		Vertices.Empty();

		// Use up the first segment before going into the loop.
		if( Segments(0).SegType == eSEGTYPE_BEZIER )
		{
			TArray<FVector> BezierPoints;
			Segments(0).GetBezierPoints( &BezierPoints );
			for( int bz = BezierPoints.Num() - 1 ; bz > 0 ; bz-- )
			{
				new(Vertices)CPOLY_VERTEX;
				Vertices(Vertices.Num()-1).XY = BezierPoints(bz);
				Vertices(Vertices.Num()-1).FirstEdge = 0;
				Vertices(Vertices.Num()-1).EdgeCount = 0;
			}
		}
		else
		{
			new(Vertices)CPOLY_VERTEX;
			Vertices(Vertices.Num()-1).XY = Segments(0).Vertex[1];
			Vertices(Vertices.Num()-1).FirstEdge = 0;
			Vertices(Vertices.Num()-1).EdgeCount = 0;
		}
		Segments(0).bUsed = 1;

		F2DSEVector Match = Segments(0).Vertex[0];

		for( int seg = 0 ; seg < Segments.Num() ; seg++ )
		{
			if( !Segments(seg).bUsed
					&& Segments(seg).Vertex[1] == Match )
			{
				if( Segments(seg).SegType == eSEGTYPE_BEZIER )
				{
					TArray<FVector> BezierPoints;
					Segments(seg).GetBezierPoints( &BezierPoints );
					for( int bz = BezierPoints.Num() - 1 ; bz > 0 ; bz-- )
					{
						new(Vertices)CPOLY_VERTEX;
						Vertices(Vertices.Num()-1).XY = BezierPoints(bz);
						Vertices(Vertices.Num()-1).FirstEdge = 0;
						Vertices(Vertices.Num()-1).EdgeCount = 0;
					}
				}
				else
				{
					new(Vertices)CPOLY_VERTEX;
					Vertices(Vertices.Num()-1).XY = Segments(seg).Vertex[1];
					Vertices(Vertices.Num()-1).FirstEdge = 0;
					Vertices(Vertices.Num()-1).EdgeCount = 0;
				}

				Segments(seg).bUsed = 1;
				Match = Segments(seg).Vertex[0];

				seg = 0;
			}
		}

		InitializeBoundary();

		int nSize1 = Vertices.Num();
		check(nSize1 > 2);

		// populate 2d tree with vertices
		static constexpr int randomizeTab[256] = {
			 32, 197, 150,  24,  44, 194, 137, 215,
			188, 253, 177, 202, 100, 165, 225, 234,
			 84, 248, 214, 235, 119,  59,   4, 176,
			 82,  37,  80, 230, 171,  55, 166, 242,
			217, 120, 236, 251, 254,  11, 174, 106,
			 34,  78,  38,  90, 140,  95, 163, 228,
			 63,  72, 212,  85, 232, 128,  92,  39,
			 13, 186, 112, 199, 193,  29,  50,  31,
			170, 167, 183, 109, 160, 168,  71,  68,
			218, 222,  20, 233,  23, 125,  25,  21,
			207,  27, 220, 238, 244,  62,  86, 127,
			115, 157,  45, 118, 206, 104, 111,  77,
			226,   5,  41,  36,  52,  47,  67,  97,
			130, 237,  76, 139, 173,  12, 164,  75,
			204,  69, 141, 246,  74, 179,  53,  46,
			 49, 159, 142, 196, 149, 203, 122,   8,
			169,  87, 102, 132,  51, 219, 156,  96,
			 58, 105, 191, 107, 146,  30, 231, 124,
			229, 138,  18, 249, 151,  28,  26,   9,
			240,  98, 135, 161,  66,  81,  83, 148,
			241,  10, 209, 136, 158, 181,  43,  73,
			154, 162, 121, 247,  57, 245,  79, 131,
			198,  93, 189, 113, 143, 182, 152,  16,
			  0, 110, 195, 129,   7, 184,  19, 243,
			190, 101, 227, 180, 210,  48,  64,  60,
			  3, 201, 250, 144, 185,  15,  94, 208,
			134,  35, 155,  54, 133, 252, 145, 205,
			116,  61, 187,  17, 211, 114,  42,   6,
			221, 123,  22, 175, 172, 216,  70, 126,
			  1, 239, 213, 153, 108, 200, 255, 192,
			  2, 223, 178,  89, 117, 103,  88, 224,
			 14,  40,  56,  65,  91, 147,  33,  99
		};

		int nSize2 = ((nSize1 - 1) | 0xFF) + 1;
		{
			for( int n = 0; n < nSize2; ++n )
			{
				int nV = (n & ~0xFF) | randomizeTab[n & 0xFF];
				if( nV < nSize1 )
					AddTreeNode( nV, posHead );
			}
		}

		// queue up initial edges of the boundary
		{ 
			for( int n = 0; n < Edges.Num(); ++n )
				QueueDelaunayEdge( n );
		}

		int nVert1, nVert2, nVert3, nEdge1, nEdge2, nEdge3;
		int nTriangle;
		UBOOL bFree;

		while( m_queue.Num() )
		{
			CPOLY_QUEUEELEMENT qe = m_queue(0);
			m_queue.Remove(0);
			nEdge1 = qe.Edge;
			nVert3 = qe.Vertex;

			CPOLY_EDGE& edge1 = Edges(nEdge1);
			bFree = (edge1.SideLeft.Triangle < 0);
			nVert1 = edge1.SideLeft.Vertex;
			nVert2 = edge1.SideRight.Vertex;

			if( bFree )
			{
				nEdge2 = FindEdge( nVert3, nVert2 );
				nEdge3 = FindEdge( nVert1, nVert3 );

				// add new triangle
				nTriangle = AddSubTriangle( nVert1, nVert2, nVert3 );

				// got two new edges, queue them too
				Edges(nEdge1).SideLeft.Triangle = nTriangle;
				
				if( nEdge2 < 0 )
				{
					nEdge2 = AddEdge( nVert3, nVert2 );
					CPOLY_EDGE& edge2 = Edges(nEdge2);
					edge2.SideRight.Triangle = nTriangle;
					QueueDelaunayEdge( nEdge2 );
				}
				else
				{
					CPOLY_EDGE& edge2 = Edges(nEdge2);
					if( edge2.SideLeft.Triangle == -1 )
						edge2.SideLeft.Triangle = nTriangle;
					else
					{
						if( edge2.SideRight.Triangle == -1 )
							edge2.SideRight.Triangle = nTriangle;
					}
				}

				if( nEdge3 < 0 )
				{
					nEdge3 = AddEdge( nVert1, nVert3 );
					CPOLY_EDGE& edge3 = Edges(nEdge3);
					edge3.SideRight.Triangle = nTriangle;
					QueueDelaunayEdge( nEdge3 );
				}
				else
				{
					CPOLY_EDGE& edge3 = Edges(nEdge3);
					if( edge3.SideLeft.Triangle == -1 )
						edge3.SideLeft.Triangle = nTriangle;
					else
					{
						if( edge3.SideRight.Triangle == -1 )
							edge3.SideRight.Triangle = nTriangle;
					}
				}
			}
		}

		//
		// Massage the vertices/triangles before leaving ...
		//

		for( int vertex = 0 ; vertex < Vertices.Num() ; vertex++ )
		{
			Vertices(vertex).XY -= InOrigin;
		}

		unguard;
	}
	POSITION AddTreeNode( int nVertex, POSITION posNode )
	{
		guard(FShape::AddTreeNode);
		check( posNode != -1 );

		CPOLY_TREENODE* pNode;
		CPOLY_TREENODE* pParent;
		POSITION pos = posNode;
		FVector& pt = Vertices(nVertex).XY;
		UBOOL bResult;
		int nV;
		int nDir = 0;

		// NOTE: tree is not balanced (the extra effort would be overkill
		//  for such small vertex sets.  Splay trees are cool, though.)

		do {
			pNode = &m_tree( pos );
			pParent = pNode;
			nV = pNode->Vertex;

			if( nV < 0 )
				bResult = 0;
			else
				if( nDir)
					bResult = (pt.Y < Vertices(nV).XY.Y);
				else
					bResult = (pt.X < Vertices(nV).XY.X);

			if( bResult )
				pos = pNode->NodeLeft;
			else
				pos = pNode->NodeRight;

			nDir = (nDir + 1) & 1;
		} while( pos != -1 );
		
		CPOLY_TREENODE tn;
		tn.Vertex = nVertex;
		tn.NodeLeft = -1;
		tn.NodeRight = -1;
		pos = m_tree.Add();
		m_tree(pos) = tn;
		
		if( bResult )
			pParent->NodeLeft = pos;
		else
			pParent->NodeRight = pos;

		return pos;
		unguard;
	}
	// Gets orientation of a triangle (pt1, pt2, pt3).  Return values are:
	//         +2: counter-clockwise
	//         +1: collinear, pt3 between pt1 and pt2
	//          0: collinear, pt1 between pt2 and pt3
	//         -1: collinear, pt2 between pt1 and pt3
	//         -2: clockwise
	//
	int Orientation( const FVector& pt1,
					const FVector& pt2,
					const FVector& pt3 )
	{
		guard(FShape::Orientation);
		FVector v1 = pt2 - pt1;
		FVector v2 = pt3 - pt1;
		double d = v1.X * v2.Y - v1.Y * v2.X;

		if( fabs( d ) < 0.000001 ) {
			if( (v1 | v2 ) < 0 )
				return 0;
			else if( v1.Size() > v2.Size() )
				return -1;
			else
				return 1;
		} 
		else
		{
			if( d < 0 )
				return -2;
			else
				return 2;
		}
		unguard;
	}
	// Tests for any intersections of existing edges and the line segment
	//  between nV1 and nV2.
	bool TestEdgeIntersection( int nV1, int nV2 )
	{
		guard(FShape::TestEdgeIntersection);
		bool bIntersect = false;

		FVector p1 = Vertices(nV1).XY;
		FVector p2 = Vertices(nV2).XY;
		FBox bbox(p1, p2);
		FVector ptBMin = bbox.Min;
		FVector ptBMax = bbox.Max;

		// Linear search. Running time could be cut down by storing
		//  the edge data in a spatial R-Tree
		for( int n = 0; n < Edges.Num(); ++n )
		{
			CPOLY_EDGE& edge = Edges(n);
			if( ptBMin.X <= edge.BMax.X && ptBMin.Y <= edge.BMax.Y
					&& ptBMax.X >= edge.BMin.X && ptBMax.Y >= edge.BMin.Y )
			{
				int nV3 = edge.SideLeft.Vertex;
				int nV4 = edge.SideRight.Vertex;

				if( nV3 != nV1 && nV3 != nV2 && nV4 != nV1 && nV4 != nV2 )
				{
					FVector& q1 = Vertices(nV3).XY;
					FVector& q2 = Vertices(nV4).XY;
					int o1, o2, o3, o4;
					o1 = Orientation( p1, q1, q2 );
					o2 = Orientation( p2, q1, q2 );
					o3 = Orientation( q1, p2, p1 );
					o4 = Orientation( q2, p2, p1 );

					if( o1 * o2 <= 0 && o3 * o4 <= 0 )
					{
						bIntersect = true;
						break;
					}
				}
			}
		}

		return bIntersect;
		unguard;
	}
	// QueueDelaunayEdge searches a third vertex satisfying the Delaunay
	//  criterion for the given edge, and stores the search result in a queue.
	void QueueDelaunayEdge( int nEdge )
	{
		guard(FShape::QueueDelaunayEdge);		
		STACKENTRY se;

		FVector va, vb, vd, pc;
		int nVertex=0;
		double a0, b0, d0, d1, d2, r;
		UBOOL bInsideRect, b1, b2;

		int nV1 = Edges(nEdge).SideLeft.Vertex;
		int nV2 = Edges(nEdge).SideRight.Vertex;
		int nV3 = 0;
		FVector& p1 = Vertices(nV1).XY;
		FVector& p2 = Vertices(nV2).XY;
		FVector p3;

		FVector vc = p2 - p1;

		double c0 = vc.SizeSquared();
//		double c1 = sqrt( c0 );
		double cosC = 0;
		double cosCmin = 0;
		UBOOL bFound = 0;
		UBOOL bIntersectAll = 0;
		UBOOL bIntersect;

		while( !bFound )
		{
			stack.Empty();

			FVector rectMin( -MAX_COORD, -MAX_COORD, 0 );
			FVector rectMax( MAX_COORD, MAX_COORD, 0 );
			int nDir = 0;
			POSITION posNode = 0;//m_tree.GetHeadPosition();

			cosC = 0;
			cosCmin = 2;

			int nReturn = 0; // Return "address"

			for( ;; )
			{
				while( posNode != -1 )
				{
					// 2D tree range searching (Sedgewick: Algorithms)
					nV3 = m_tree( posNode ).Vertex;

					// LABEL 0
					if( nReturn < 1 )
					{
						if( nV3 < 0 )
							bInsideRect = 0;
						else
						{
							p3 = Vertices(nV3).XY;
							bInsideRect =
								(p3.X >= rectMin.X
								&& p3.Y >= rectMin.Y
								&& p3.X <= rectMax.X
								&& p3.Y <= rectMax.Y);
						}

						if( bInsideRect )
						{
							if( nV3 != nV1 && nV3 != nV2 )
							{
								if( Orientation( p1, p2, p3 ) == 2 )
								{
									va = p2 - p3;
									vb = p3 - p1;
									a0 = va.SizeSquared();
									b0 = vb.SizeSquared();
									// Delaunay criterion
									cosC = (a0 + b0 - c0) / sqrt( a0 * b0 );
									if( cosC < cosCmin )
									{
										if( bIntersectAll )
											bIntersect = TestEdgeIntersection( nV3, nV2 ) || TestEdgeIntersection( nV1, nV3 );
										else
											bIntersect = 0;

										if( !bIntersect )
										{
											bFound = 1;
											nVertex = nV3;
											cosCmin = cosC;
											d0 = vc.X * (p1.X + p2.X) + vc.Y * (p1.Y + p2.Y);
											d1 = vb.X * (p3.X + p1.X) + vb.Y * (p3.Y + p1.Y);
											d2 = 2 * (vc.X * va.Y - vc.Y * va.X);
											pc = FVector(
												(vc.Y * d1 - vb.Y * d0) / d2,
												(vb.X * d0 - vc.X * d1) / d2,
												0);
											vd = p1 - pc;
											r = vd.Size() * 1.01;
											FBox bbox( rectMin, rectMax );
											bbox += FVector( pc.X - r, pc.Y - r, 0);
											bbox += FVector( pc.X + r, pc.Y + r, 0);
											rectMin = bbox.Min;
											rectMax = bbox.Max;
										}
									}
								}
							}
						}
						
						if( nV3 < 0 )
							b1 = 0;
						else
						{
							p3 = Vertices(nV3).XY;
							b1 = (nDir == 0) ? (rectMin.X < p3.X) : (rectMin.Y < p3.Y);
						}

						if( b1 )
						{
							se.Node = posNode;
							se.Dir = nDir;
							stack(stack.Add()) = se;
							posNode = m_tree( posNode ).NodeLeft;
							nDir = (nDir + 1) & 1;
							nReturn = 2; // left branch
						}
					}

					// LABEL 1
					if( nReturn < 2 )
					{
						if( nV3 < 0 )
							b2 = 1;
						else
						{
							p3 = Vertices(nV3).XY;
							b2 = (nDir == 0) ? (rectMax.X >= p3.X) : (rectMax.Y >= p3.Y);
						}

						if( b2 )
						{
							// right branch
							posNode = m_tree( posNode ).NodeRight;
							nDir = (nDir + 1) & 1;
						}
						else
							break;

					}
					nReturn = 0;
				}

				if( stack.Num() <= 0 ) break;
				posNode = stack(stack.Num()-1).Node;
				nDir = stack(stack.Num()-1).Dir;
				stack.Remove( stack.Num()-1 );
				nReturn = 1;
			}

			if( !bFound )
				if( bIntersectAll )
					break;
			else 
				if( !bIntersectAll )
				{
					bIntersect = TestEdgeIntersection( nVertex, nV2 ) || TestEdgeIntersection( nV1, nVertex );
					if( bIntersect )
						bFound = 0;
					else
						break;
				}

			bIntersectAll = 1;
		}

		if( bFound )
		{
			CPOLY_QUEUEELEMENT qe;
			qe.Edge = nEdge;
			qe.Vertex = nVertex;
			qe.Gamma = cosCmin;
			int x = 0;
			for( ; x < m_queue.Num() ; x++ )
				if( m_queue( x ).Gamma > cosCmin )
					break;

			if( x < m_queue.Num() )
			{
				m_queue.Insert(x);
				m_queue(x) = qe;
			}
			else
				m_queue(m_queue.Add()) = qe;
		}
		unguard;
	}

	F2DSEVector Handle;
	TArray<FSegment> Segments;
};
TArray<STACKENTRY> FShape::stack;

#define d2dSE_SELECT_TOLERANCE 4

// --------------------------------------------------------------
//
// W2DSHAPEEDITOR
//
// --------------------------------------------------------------

void VectorTrim(FVector& V)
{
	INT Base = 10;
	V.X = (FLOAT)(INT)(V.X*Base)/Base;
	V.Y = (FLOAT)(INT)(V.Y*Base)/Base;
	V.Z = (FLOAT)(INT)(V.Z*Base)/Base;
}

// We can add this offset to tooltip IDs to filter out WM_NOTIFY messages
// produced by spammy dynamic tooltips.
#define TOOLTIP_OFFSET	1000000
#define ID_2DSE_TOOLBAR	29002
TBBUTTON tb2DSEButtons[] = {
	  { 0, ID_FileNew, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 1, ID_FileOpen, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 2, ID_FileSave, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 25, ID_EditUndo, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 26, ID_EditRedo, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 27, IDMN_2DSE_MERGE_SHAPES, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 28, IDMN_2DSE_DRAW_SHAPE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
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
	, { 12, ID_BrushIntersect, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 13, ID_EditDelete, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 14, IDMN_SEGMENT_LINEAR, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 15, IDMN_SEGMENT_BEZIER, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 16, IDMN_2DSE_PROCESS_SHEET, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 17, IDMN_2DSE_PROCESS_REVOLVE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 18, IDMN_2DSE_PROCESS_EXTRUDE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 19, IDMN_2DSE_PROCESS_EXTRUDETOPOINT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 20, IDMN_2DSE_PROCESS_EXTRUDETOBEVEL, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 21, IDMN_2DSE_XY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 22, IDMN_2DSE_XZ, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 23, IDMN_2DSE_YZ, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 24, IDMN_2DSE_TESSELLATE_SHAPES, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
};
struct {
	TCHAR ToolTip[64];
	int ID;
} ToolTips_2DSE[] = {
	TEXT("New"), ID_FileNew,
	TEXT("Open"), ID_FileOpen,
	TEXT("Save"), ID_FileSave,
	TEXT("Undo"), ID_EditUndo,
	TEXT("Redo"), ID_EditRedo,
	TEXT("Merge Shapes"), IDMN_2DSE_MERGE_SHAPES,
	TEXT("Draw Shape"), IDMN_2DSE_DRAW_SHAPE,
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
	TEXT("Split Segment(s)"), ID_BrushIntersect,
	TEXT("Delete"), ID_EditDelete,
	TEXT("Linear Segment"), IDMN_SEGMENT_LINEAR,
	TEXT("Bezier Segment"), IDMN_SEGMENT_BEZIER,
	TEXT("Zoom In"), IDMN_2DSE_ZOOM_IN,
	TEXT("Zoom Out"), IDMN_2DSE_ZOOM_OUT,
	TEXT("Place in XY direction"), IDMN_2DSE_XY,
	TEXT("Place in XZ direction"), IDMN_2DSE_XZ,
	TEXT("Place in YZ direction"), IDMN_2DSE_YZ,
	TEXT("Tessellate Shapes"), IDMN_2DSE_TESSELLATE_SHAPES,
	NULL, 0
};

class W2DShapeEditor;

struct F2DEditorState
{
	F2DSEVector m_origin;
	TArray<FShape> m_shapes;
	const TCHAR* Action;

	void Set(const W2DShapeEditor* Other, const TCHAR* InAction);
};

class W2DShapeEditor : public WWindow
{
	DECLARE_WINDOWCLASS(W2DShapeEditor,WWindow,Window)

	// Structors.
	W2DShapeEditor( FName InPersistentName, WWindow* InOwnerWindow )
	:	WWindow( InPersistentName, InOwnerWindow )
	{
		New();
		m_bDraggingCamera = m_bDraggingVerts = FALSE;
		hImage = NULL;
		ImageZoom = 1.0f;
		hWndToolBar = NULL;
		Zoom = 1.0f;
		HaveChanges = false;
	}

	WToolTip* ToolTipCtrl{};
	HWND hWndToolBar;
	FVector m_camera;			// The viewing camera position
	F2DSEVector m_origin;		// The origin point used for revolves and such
	BOOL m_bDraggingCamera{}, m_bDraggingVerts{}, m_bMouseHasMoved{}, HaveChanges{};
	FPoint m_pointOldPos{};
	FString MapFilename;
	HBITMAP hImage;
	float ImageZoom;
	RECT m_rcWnd{};
	POINT m_ContextPos{};
	float Zoom;
	HBITMAP ToolbarImage{};
	UBOOL TessellateShapes{};

	TArray<FShape> m_shapes;

	enum { UNDO_DEPTH = 64 };
	F2DEditorState UndoBuffer[UNDO_DEPTH];
	INT UndoTop{}, UndoBot{}, UndoPos{};
	UBOOL UndoSaved{};

	UBOOL DrawShape{};
	INT DrawShapeIndex = INDEX_NONE;
	F2DSEVector DrawShapeClickPos{};

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
		SendMessage(*this, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(0, 0));
		SetCaption();
		UpdateMenu();		
		unguard;
	}
	void Close()
	{
		guard(W2DShapeEditor::Close);
		if (HaveChanges)
		{
			HaveChanges = FALSE;
			Show(TRUE);
			if (::MessageBox(hWnd, TEXT("Save changes?"), TEXT("2DShapeEditor"), MB_YESNO) == IDYES)
				FileSave(hWnd);
		}
		unguard;
	}
	void OnDestroy()
	{
		guard(W2DShapeEditor::OnDestroy);
		Close();
		delete ToolTipCtrl;
		::DestroyWindow( hWndToolBar );
		if (ToolbarImage)
			DeleteObject(ToolbarImage);
		WWindow::OnDestroy();
		unguard;
	}
	int OnSysCommand(INT Command)
	{
		guard(W2DShapeEditor::OnSysCommand);
		if (Command == SC_CLOSE)
		{
			Show(0);
			return 1;
		}

		return 0;
		unguard;
	}
	void OnClose()
	{
		Show(0);
	}
	INT OnSetCursor()
	{
		guard(W2DShapeEditor::OnSetCursor);
		WWindow::OnSetCursor();
		if (DrawShape)
			SetCursor(LoadCursorW(hInstanceWindow, MAKEINTRESOURCEW(IDC_Draw)));
		else
			SetCursor(LoadCursorW(NULL, IDC_CROSS));
		return 0;
		unguard;
	}
	void OnCreate()
	{
		guard(W2DShapeEditor::OnCreate);
		WWindow::OnCreate();

		SetMenu( hWnd, AddHotkeys(LoadMenuW(hInstance, MAKEINTRESOURCEW(IDMENU_2DShapeEditor))) );

		ToolbarImage = reinterpret_cast<HBITMAP>(LoadImage(hInstance,
			MAKEINTRESOURCE(IDB_2DSE_TOOLBAR),
			IMAGE_BITMAP, 0, 0, 0));
		check(ToolbarImage);
		ScaleImageAndReplace(ToolbarImage);

		INT ImagesCount = -1;
		for (INT i = 0; i < ARRAY_COUNT(tb2DSEButtons); i++)
			if (tb2DSEButtons[i].idCommand)
				ImagesCount = Max(ImagesCount, tb2DSEButtons[i].iBitmap);

		// TOOLBAR
		hWndToolBar = CreateToolbarEx( 
			hWnd,
			WS_CHILD | WS_VISIBLE | CCS_ADJUSTABLE,
			ID_2DSE_TOOLBAR,
			++ImagesCount,
			NULL,
			reinterpret_cast<PTRINT>(ToolbarImage),
			(LPCTBBUTTON)&tb2DSEButtons,
			ARRAY_COUNT(tb2DSEButtons),
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

			ToolTipCtrl->AddTool( hWndToolBar, ToolTips_2DSE[tooltip].ToolTip, ToolTips_2DSE[tooltip].ID + TOOLTIP_OFFSET, &rect );
		}

		HaveChanges = false;

		unguard;
	}
	void OnPaint()
	{
		guard(W2DShapeEditor::OnPaint);
		PAINTSTRUCT PS;
		HDC hDC = BeginPaint( *this, &PS );

		HDC hdcWnd, hdcMem;
		HBITMAP hBitmap;
		HBRUSH l_brush, l_brushOld = NULL;

		::GetClientRect( hWnd, &m_rcWnd );

		hdcWnd = GetDC(hWnd);
		hdcMem = CreateCompatibleDC(hdcWnd);
		hBitmap = CreateCompatibleBitmap(hdcWnd, m_rcWnd.right, m_rcWnd.bottom );
		SelectObject(hdcMem, hBitmap);

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
		DeleteObject( hBitmap );
		unguard;
	}
	void ScaleShapes( float InScale )
	{
		guard(W2DShapeEditor::ScaleShapes);
		if (AskEndDrawMode())
			return;

		UndoSaved = FALSE;
		for( int shape = 0 ; shape < m_shapes.Num() ; shape++ )
		{
			for( int seg = 0 ; seg < m_shapes(shape).Segments.Num() ; seg++ )
			{
				MaybeSaveUndoState(TEXT("Scale Shapes"));
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
	inline INT UndoIndex(INT pos)
	{
		return (UNDO_DEPTH + pos) % UNDO_DEPTH;
	}
	inline void MaybeSaveUndoState(const TCHAR* InAction, UBOOL Check = TRUE)
	{
		if (!UndoSaved && Check)
			SaveUndoState(InAction);
	}
	void SaveUndoState(const TCHAR* InAction)
	{
		guard(W2DShapeEditor::SaveUndoState);
		UndoSaved = TRUE;
		UndoBuffer[UndoPos].Set(this, InAction);
		UndoTop = UndoPos = UndoIndex(UndoPos + 1);
		if (UndoBot == UndoTop)
			UndoBot = UndoIndex(UndoBot + 1);
		UpdateMenu();
		unguard;
	}
	// Dir => +1 = Redo, -1 = Undo
	void RestoreUndoState(INT Dir)
	{
		guard(W2DShapeEditor::RestoreUndoState);
		if (UndoPos != (Dir > 0 ? UndoTop : UndoBot))
		{
			if (Dir < 0 && UndoPos == UndoTop)
				UndoBuffer[UndoPos].Set(this, TEXT("Last state"));
			UndoPos = UndoIndex(UndoPos + Dir);
			const F2DEditorState& State = UndoBuffer[UndoPos];
			m_origin = State.m_origin;
			m_shapes = State.m_shapes;
			ComputeHandlePositions(1);
			InvalidateRect( hWnd, NULL, FALSE );
		}
		unguard;
	}
	void ResetUndoState()
	{
		guard(W2DShapeEditor::ResetUndoState);
		UndoBot = UndoTop = UndoPos = 0;
		for (INT i = 0; i < UNDO_DEPTH; i++)
			UndoBuffer[i].m_shapes.Empty();
		unguard;
	}
	void OnCommand( INT Command )
	{
		guard(W2DShapeEditor::OnCommand);
		switch( Command ) {

			case ID_FileExit:
				SendMessageW( hWnd, WM_CLOSE, 0, 0 );
				break;

			case IDMN_2DSE_ZOOM_IN:
				Zoom *= 2.0f;
				if( Zoom > 8.0f) Zoom = 8.0f;
				GGridSizeZoom = GGridSize * Zoom;
				InvalidateRect( hWnd, NULL, FALSE );
				break;

			case IDMN_2DSE_ZOOM_OUT:
				Zoom *= 0.5f;
				if( Zoom < 0.125f) Zoom = 0.125f;
				GGridSizeZoom = GGridSize * Zoom;
				InvalidateRect( hWnd, NULL, FALSE );
				break;

			case IDMN_2DSE_SCALE_UP:
				ScaleShapes(2.0f);
				break;

			case IDMN_2DSE_SCALE_DOWN:
				ScaleShapes(0.5f);
				break;

			case IDMN_2DSE_ROTATE90:
				RotateShape( 90 );
				break;

			case IDMN_2DSE_ROTATE45:
				RotateShape( 45 );
				break;

			case IDMN_DETAIL_1:
				SetSegmentDetail(1);
				break;

			case IDMN_DETAIL_2:
				SetSegmentDetail(2);
				break;

			case IDMN_DETAIL_3:
				SetSegmentDetail(3);
				break;

			case IDMN_DETAIL_4:
				SetSegmentDetail(4);
				break;

			case IDMN_DETAIL_5:
				SetSegmentDetail(5);
				break;

			case IDMN_DETAIL_10:
				SetSegmentDetail(10);
				break;

			case IDMN_DETAIL_15:
				SetSegmentDetail(15);
				break;

			case IDMN_DETAIL_20:
				SetSegmentDetail(20);
				break;

			case IDMN_DETAIL_CUSTOM:
				{
					if (AskEndDrawMode())
						break;
					WDlgCustomDetail dlg( NULL, this );
					if( dlg.DoModal() )
						SetSegmentDetail( dlg.Value );
				}
				break;

			case IDMN_2DSE_FLIP_VERT:
				Flip(0, TRUE);
				break;

			case IDMN_2DSE_FLIP_HORIZ:
				Flip(1, TRUE);
				break;

			case IDMN_2DSE_MERGE_SHAPES:
				MergeShapes();
				break;

			case IDMN_2DSE_DRAW_SHAPE:
				DrawShape = ToggleMenuItem(GetMenu(hWnd), IDMN_2DSE_DRAW_SHAPE);
				SendMessage(hWndToolBar, TB_SETSTATE, IDMN_2DSE_DRAW_SHAPE, 
					SendMessageW(hWndToolBar, TB_GETSTATE, IDMN_2DSE_DRAW_SHAPE, 0) & TBSTATE_CHECKED ? TBSTATE_ENABLED : TBSTATE_CHECKED | TBSTATE_ENABLED);
				DrawShapeIndex = INDEX_NONE;
				if (DrawShape)
				{
					UndoSaved = FALSE;
					DeselectAllVerts();
				}
				break;

			case IDPB_SELECT_ALL:
				SelectAll();
				break;

			case IDMN_SEGMENT_LINEAR:
				ChangeSegmentTypes(eSEGTYPE_LINEAR);
				break;

			case IDMN_SEGMENT_BEZIER:
				ChangeSegmentTypes(eSEGTYPE_BEZIER);
				break;

			case ID_FileNew:
				if (AskEndDrawMode())
					break;
				if (HaveChanges && ::MessageBox(hWnd, TEXT("Save changes?"), TEXT("2DShapeEditor"), MB_YESNO) == IDYES)
					FileSave(hWnd);
				MapFilename = TEXT("");
				SetCaption();
				New();
				break;

			case ID_BrushIntersect:
				SplitSides();
				break;

			case IDMN_2DSE_NEW_SHAPE:
				InsertNewShape();
				break;

			case IDMN_2DSE_NEW_TRIANGLE:
				InsertNewTriangle();
				break;

			case IDMN_2DSE_NEW_CYLINDER:
			{
				if (AskEndDrawMode())
					break;
				WDlgCylinderShape dlg(NULL, this);
				if (dlg.DoModal() == IDOK)
					InsertNewCylinder(dlg);
			}
			break;

			case IDMN_2DSE_FROM_SELECTED_SURFACE_BSP:
				InsertFromSelectedSurface(FALSE);
				break;

			case IDMN_2DSE_FROM_SELECTED_SURFACE_BRUSH:
				InsertFromSelectedSurface(TRUE);
				break;

			case ID_EditDelete:
				Delete();
				break;

			case ID_FileSave:
				FileSave( hWnd );
				break;

			case ID_FileSaveAs:
				FileSaveAs( hWnd );
				break;

			case ID_FileOpen:
				if (AskEndDrawMode())
					break;
				if (HaveChanges && ::MessageBox(hWnd, TEXT("Save changes?"), TEXT("2DShapeEditor"), MB_YESNO) == IDYES)
					FileSave(hWnd);
				FileOpen( hWnd );
				break;

			case ID_EditUndo:
				RestoreUndoState(-1);
				break;

			case ID_EditRedo:
				RestoreUndoState(+1);
				break;

			case IDMN_2DSEC_SET_ORIGIN:
				SetOrigin();
				break;

			case IDMN_2DSE_PROCESS_SHEET:
				ProcessSheet();
				break;

			case IDMN_2DSE_PROCESS_EXTRUDE:
				{
					if (AskEndDrawMode())
						break;
					WDlgExtrude Dialog( NULL, this );
					if( Dialog.DoModal() == IDOK )
						ProcessExtrude( Dialog.Depth );
				}
				break;

			case IDMN_2DSE_PROCESS_EXTRUDETOPOINT:
				{
					if (AskEndDrawMode())
						break;
					WDlgExtrudeToPoint Dialog( NULL, this );
					if( Dialog.DoModal() == IDOK )
						ProcessExtrudeToPoint( Dialog.Depth );
				}
				break;

			case IDMN_2DSE_PROCESS_EXTRUDETOBEVEL:
				{
					if (AskEndDrawMode())
						break;
					WDlgExtrudeToBevel Dialog( NULL, this );
					if( Dialog.DoModal() == IDOK )
						ProcessExtrudeToBevel( Dialog.Depth, Dialog.CapHeight );
				}
				break;

			case IDMN_2DSE_PROCESS_REVOLVE:
				{
					if (AskEndDrawMode())
						break;
					WDlgRevolve Dialog( NULL, this );
					if( Dialog.DoModal() == IDOK )
						ProcessRevolve( Dialog.TotalSides, Dialog.Sides );
				}
				break;

			case IDMN_2DSE_TESSELLATE_SHAPES:
				TessellateShapes = ToggleMenuItem(GetMenu(hWnd), IDMN_2DSE_TESSELLATE_SHAPES);
				SendMessage(hWndToolBar, TB_SETSTATE, IDMN_2DSE_TESSELLATE_SHAPES, 
					SendMessageW(hWndToolBar, TB_GETSTATE, IDMN_2DSE_TESSELLATE_SHAPES, 0) & TBSTATE_CHECKED ? TBSTATE_ENABLED : TBSTATE_CHECKED | TBSTATE_ENABLED);
				break;

			case IDMN_GRID_1:	
				GGridSize = 1;	
				GGridSizeZoom = GGridSize * Zoom;
				InvalidateRect( hWnd, NULL, FALSE );
				break;
			case IDMN_GRID_2:	
				GGridSize = 2;	
				GGridSizeZoom = GGridSize * Zoom;
				InvalidateRect( hWnd, NULL, FALSE );
				break;
			case IDMN_GRID_4:	
				GGridSize = 4;	
				GGridSizeZoom = GGridSize * Zoom;
				InvalidateRect( hWnd, NULL, FALSE );
				break;
			case IDMN_GRID_8:	
				GGridSize = 8;	
				GGridSizeZoom = GGridSize * Zoom;
				InvalidateRect( hWnd, NULL, FALSE );
				break;
			case IDMN_GRID_16:	
				GGridSize = 16;	
				GGridSizeZoom = GGridSize * Zoom;
				InvalidateRect( hWnd, NULL, FALSE );
				break;
			case IDMN_GRID_32:	
				GGridSize = 32;	
				GGridSizeZoom = GGridSize * Zoom;
				InvalidateRect( hWnd, NULL, FALSE );
				break;
			case IDMN_GRID_64:	
				GGridSize = 64;	
				GGridSizeZoom = GGridSize * Zoom;
				InvalidateRect( hWnd, NULL, FALSE );
				break;
			case IDMN_GRID_128:
				GGridSize = 128;
				GGridSizeZoom = GGridSize * Zoom;
				InvalidateRect(hWnd, NULL, FALSE);
				break;
			case IDMN_GRID_256:
				GGridSize = 256;
				GGridSizeZoom = GGridSize * Zoom;
				InvalidateRect(hWnd, NULL, FALSE);
				break;

			case IDMN_2DSE_CLEAR:
				if (AskEndDrawMode())
					break;
				if (!HaveChanges || ::MessageBox(hWnd, TEXT("Clear work area?"), TEXT("2DShapeEditor"), MB_YESNO) == IDYES)
				{
					SaveUndoState(TEXT("Clear Work Area"));
					m_shapes.Empty();
					HaveChanges = FALSE;
				}
				break;

			case IDMN_2DSE_GET_IMAGE:
				{
					if( !GEditor->CurrentTexture )
					{
						appMsgf(TEXT("Select a texture in the browser first."));
						return;
					}
#if ENGINE_VERSION==227
					if (GEditor->CurrentTexture->Format == TEXF_BC1 || GEditor->CurrentTexture->Format == TEXF_BC2 || GEditor->CurrentTexture->Format == TEXF_BC3 || GEditor->CurrentTexture->Format == TEXF_BC7)
					{
						appMsgf(TEXT("Import of DXT Textures not supported."));
						return;
					}
#endif
					GEditor->CurrentTexture->CheckBaseMip();
					if (GEditor->CurrentTexture->Mips.Num() == 0)
					{
						appMsgf(TEXT("Select a texture with P8 data."));
						return;
					}
					GEditor->CurrentTexture->Mips(0).LoadMip();

					BITMAPINFO bmi;
					BYTE* pBits = NULL;

					::ZeroMemory( &bmi, sizeof(BITMAPINFOHEADER) );
					bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
					bmi.bmiHeader.biWidth = GEditor->CurrentTexture->USize;
					bmi.bmiHeader.biHeight = -GEditor->CurrentTexture->VSize;
					bmi.bmiHeader.biPlanes = 1;
					bmi.bmiHeader.biBitCount = 24;
					bmi.bmiHeader.biCompression = BI_RGB;

					if( hImage ) 
						DeleteObject( hImage );
					hImage = CreateDIBSection( ::GetDC( hWnd ), &bmi, DIB_RGB_COLORS, (void**)&pBits, NULL, 0 );
					check( hImage );
					check( pBits );
					ImageZoom = 1.0f;

					// RGB DATA
					FColor* Palette = GEditor->CurrentTexture->GetColors();
					check(Palette);
					BYTE* pSrc = GEditor->CurrentTexture->Mips(0).DataPtr;
					BYTE* pDst = pBits;
					for( int x = 0 ; x < GEditor->CurrentTexture->USize * GEditor->CurrentTexture->VSize; x++ )
					{
						FColor color = Palette[ *pSrc ];
						*pDst = color.B;	pDst++;
						*pDst = color.G;	pDst++;
						*pDst = color.R;	pDst++;
						pSrc++;
					}
				}
				break;

			case IDMN_2DSE_OPEN_IMAGE:
				{
					TArray<FString> Files;
					if (OpenFilesWithDialog(
						*GLastDir[eLASTDIR_UTX], 
						TEXT("Bitmaps (*.bmp)\0*.bmp\0All Files\0*.*\0\0"),
						TEXT("bmp"),
						TEXT("Open Image"),
						Files, 
						FALSE
					))
					{
						if( hImage ) 
							DeleteObject( hImage );

						hImage = (HBITMAP)LoadImageW( hInstance, *Files(0), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE );
						ImageZoom = 1.0f;

						if (!hImage)
							appMsgf(TEXT("Error loading bitmap."));
						else
							ScaleImageAndReplace(hImage);

						GLastDir[eLASTDIR_UTX] = appFilePathName(*Files(0));
					}

					InvalidateRect( hWnd, NULL, FALSE );
				}
				break;

			case IDMN_2DSE_CHOOSE_LINE_COLOR:
				OnChooseColorButton();
				break;

			case IDMN_2DSE_DELETE_IMAGE:
				DeleteObject( hImage );
				hImage = NULL;
				InvalidateRect( hWnd, NULL, FALSE );
				break;

			case IDMN_2DSE_XY:
			case IDMN_2DSE_XZ:
			case IDMN_2DSE_YZ:
				{
					INT Directions[] = {IDMN_2DSE_XY, IDMN_2DSE_XZ, IDMN_2DSE_YZ};
					for (INT i = 0; i < ARRAY_COUNT(Directions); i++)
						SendMessage(hWndToolBar, TB_SETSTATE, Directions[i], Directions[i] != Command || 
							SendMessageW(hWndToolBar, TB_GETSTATE, Directions[i], 0) & TBSTATE_CHECKED ? TBSTATE_ENABLED : TBSTATE_CHECKED | TBSTATE_ENABLED);
				}
				break;

			default:
				WWindow::OnCommand(Command);
				break;
		}
		// Do not update the menu if a tooltip instigated the delivery of this WM_NOTIFY message.
		if (Command < TOOLTIP_OFFSET)
			UpdateMenu();
		unguard;
	}
	void SetDirection()
	{
		guard(W2DShapeEditor::SetDirection);
		INT Directions[] = {IDMN_2DSE_XY, IDMN_2DSE_XZ, IDMN_2DSE_YZ};
		FRotator Rotations[] = {FRotator(0, 0, 0), FRotator(0, 0, 16384), FRotator(-16384, 0, 0)};
		for (INT i = 0; i < ARRAY_COUNT(Directions); i++)
			if (SendMessageW(hWndToolBar, TB_GETSTATE, Directions[i], 0) & TBSTATE_CHECKED)
				GEditor->Level->Brush()->Rotation = Rotations[i];
		unguard;
	}
	void OnRightButtonDown(INT X, INT Y)
	{
		guard(W2DShapeEditor::OnRightButtonDown);

		m_bDraggingCamera = TRUE;
		m_pointOldPos.X = X;
		m_pointOldPos.Y = Y;
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
				if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
				{
					ImageZoom *= 1.1f;
					if (ImageZoom > 128.0f)
						ImageZoom = 128.0f;
				}
				else
				{
					Zoom *= 1.1f;
					if (Zoom > 8.0f)
						Zoom = 8.0f;
				}
				GGridSizeZoom = GGridSize * Zoom;
				InvalidateRect(hWnd, NULL, FALSE);
				UpdateMenu();
				break;

			case SB_LINEDOWN:
				if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
				{
					ImageZoom /= 1.1f;
					if (ImageZoom < 1.0f/32.0f)
						ImageZoom = 1.0f/32.0f;
				}
				else
				{
					Zoom /= 1.1f;
					if (Zoom < 0.125f)
						Zoom = 0.125f;
				}
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
		if (!ToolTipCtrl) // Reject premature calls.
			return;
		HMENU menu = GetMenu(hWnd);
		CheckMenuItem(menu, IDMN_GRID_1, MF_BYCOMMAND | ((GGridSize == 1) ? MF_CHECKED : MF_UNCHECKED));
		CheckMenuItem(menu, IDMN_GRID_2, MF_BYCOMMAND | ((GGridSize == 2) ? MF_CHECKED : MF_UNCHECKED));
		CheckMenuItem(menu, IDMN_GRID_4, MF_BYCOMMAND | ((GGridSize == 4) ? MF_CHECKED : MF_UNCHECKED));
		CheckMenuItem(menu, IDMN_GRID_8, MF_BYCOMMAND | ((GGridSize == 8) ? MF_CHECKED : MF_UNCHECKED));
		CheckMenuItem(menu, IDMN_GRID_16, MF_BYCOMMAND | ((GGridSize == 16) ? MF_CHECKED : MF_UNCHECKED));
		CheckMenuItem(menu, IDMN_GRID_32, MF_BYCOMMAND | ((GGridSize == 32) ? MF_CHECKED : MF_UNCHECKED));
		CheckMenuItem(menu, IDMN_GRID_64, MF_BYCOMMAND | ((GGridSize == 64) ? MF_CHECKED : MF_UNCHECKED));
		CheckMenuItem(menu, IDMN_GRID_128, MF_BYCOMMAND | ((GGridSize == 128) ? MF_CHECKED : MF_UNCHECKED));
		CheckMenuItem(menu, IDMN_GRID_256, MF_BYCOMMAND | ((GGridSize == 256) ? MF_CHECKED : MF_UNCHECKED));

		for (INT i = 0; i < ARRAY_COUNT(tb2DSEButtons); i++)
			if (tb2DSEButtons[i].idCommand)
			{
				UBOOL Undo = tb2DSEButtons[i].idCommand == ID_EditUndo;
				if (Undo || tb2DSEButtons[i].idCommand == ID_EditRedo)
				{
					UBOOL Can = UndoPos != (!Undo ? UndoTop : UndoBot);
					const TCHAR* Action;
					const TCHAR* Title;
					if (Undo)
					{
						Action = TEXT("Undo");
						Title = !Can ? TEXT("") : UndoBuffer[UndoIndex(UndoPos - 1)].Action;
					}
					else
					{
						Action = TEXT("Redo");
						Title = !Can ? TEXT("") : UndoBuffer[UndoPos].Action;
					}
					FString Text = !Can ? Action : FString::Printf(TEXT("%ls (%ls)"), Action, Title);
					ToolTipCtrl->UpdateToolTip(hWndToolBar, *Text, tb2DSEButtons[i].idCommand + TOOLTIP_OFFSET);
					SetTextMenuItem(menu, tb2DSEButtons[i].idCommand, *Text);
					EnableMenuItem(menu, tb2DSEButtons[i].idCommand, MF_BYCOMMAND | (Can ? MF_ENABLED : MF_GRAYED));
				}
			}
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
			HMENU OuterMenu = AddHotkeys(LoadMenuW(hInstance, MAKEINTRESOURCEW(IDMENU_2DShapeEditor_Context)));
			HMENU l_menu = GetSubMenu(OuterMenu, 0);

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
			DestroyMenu(OuterMenu);

			::ScreenToClient( hWnd, &m_ContextPos );
		}

		WWindow::OnRightButtonUp();
		unguard;
	}
	void OnMouseMove( DWORD Flags, FPoint MouseLocation )
	{
		guard(W2DShapeEditor::OnMouseMove);

		m_bMouseHasMoved = TRUE;

		FLOAT pct = 1.0f / Zoom;		

		FLOAT DeltaX = pct * (MouseLocation.X - m_pointOldPos.X);
		FLOAT DeltaY = pct * (MouseLocation.Y - m_pointOldPos.Y);

		if( m_bDraggingCamera )
		{
			m_camera.X += MouseLocation.X - m_pointOldPos.X;
			m_camera.Y += MouseLocation.Y - m_pointOldPos.Y;
			
			m_pointOldPos = MouseLocation;
			InvalidateRect( hWnd, NULL, FALSE );
		}
		else
		{
			if( m_bDraggingVerts )
			{
				// Origin...
				if( m_origin.IsSel() )
				{
					MaybeSaveUndoState(TEXT("Select/Move Vertices"), DeltaX != 0.f || DeltaY != 0.f);
					// Adjust temp positions
					m_origin.TempX += DeltaX;
					m_origin.TempY -= DeltaY;

					// Snap real positions to the grid.
					m_origin.X = m_origin.TempX;
					m_origin.Y = m_origin.TempY;
					m_origin = m_origin.GridSnap( FVector(GGridSize, GGridSize, GGridSize) );
				}

				// Handles...
				for( int shape = 0 ; shape < m_shapes.Num() ; shape++ )
				{
					if( m_shapes(shape).Handle.IsSel() )
					{
						MaybeSaveUndoState(TEXT("Select/Move Vertices"), DeltaX != 0.f || DeltaY != 0.f);
						// Adjust temp positions
						m_shapes(shape).Handle.TempX += DeltaX;
						m_shapes(shape).Handle.TempY -= DeltaY;

						// Snap real positions to the grid.
						m_shapes(shape).Handle.X = m_shapes(shape).Handle.TempX;
						m_shapes(shape).Handle.Y = m_shapes(shape).Handle.TempY;

						// Also move all of this shapes vertices.
						for (int seg = 0; seg < m_shapes(shape).Segments.Num(); seg++)
						{
							for (int vertex = 0; vertex < 2; vertex++)
							{
								F2DSEVector* Vertex = &(m_shapes(shape).Segments(seg).Vertex[vertex]);

								// Adjust temp positions
								Vertex->TempX += DeltaX;
								Vertex->TempY -= DeltaY;

								// Snap real positions to the grid.
								Vertex->X = Vertex->TempX;
								Vertex->Y = Vertex->TempY;
								*Vertex = Vertex->GridSnap(FVector(GGridSize, GGridSize, GGridSize));

								if (m_shapes(shape).Segments(seg).SegType == eSEGTYPE_BEZIER)
								{
									F2DSEVector* Vertex = &(m_shapes(shape).Segments(seg).ControlPoint[vertex]);

									// Adjust temp positions
									Vertex->TempX += DeltaX;
									Vertex->TempY -= DeltaY;

									// Snap real positions to the grid.
									Vertex->X = Vertex->TempX;
									Vertex->Y = Vertex->TempY;
									*Vertex = Vertex->GridSnap(FVector(GGridSize, GGridSize, GGridSize));
								}
							}
						}

						HaveChanges = true;
					}
				}

				// Vertices...
				for (int shape = 0; shape < m_shapes.Num(); shape++)
				{
					if (!m_shapes(shape).Handle.IsSel())
					{
						for (int seg = 0; seg < m_shapes(shape).Segments.Num(); seg++)
						{
							for (int vertex = 0; vertex < 2; vertex++)
							{
								if (m_shapes(shape).Segments(seg).Vertex[vertex].IsSel())
								{
									MaybeSaveUndoState(TEXT("Select/Move Vertices"), DeltaX != 0.f || DeltaY != 0.f);
									F2DSEVector* Vertex = &(m_shapes(shape).Segments(seg).Vertex[vertex]);

									// Adjust temp positions
									Vertex->TempX += DeltaX;
									Vertex->TempY -= DeltaY;

									// Snap real positions to the grid.
									Vertex->X = Vertex->TempX;
									Vertex->Y = Vertex->TempY;
									*Vertex = Vertex->GridSnap(FVector(GGridSize, GGridSize, GGridSize));
									HaveChanges = true;
								}
								if (m_shapes(shape).Segments(seg).SegType == eSEGTYPE_BEZIER
									&& m_shapes(shape).Segments(seg).ControlPoint[vertex].IsSel())
								{
									MaybeSaveUndoState(TEXT("Select/Move Vertices"), DeltaX != 0.f || DeltaY != 0.f);
									F2DSEVector* Vertex = &(m_shapes(shape).Segments(seg).ControlPoint[vertex]);

									// Adjust temp positions
									Vertex->TempX += DeltaX;
									Vertex->TempY -= DeltaY;

									// Snap real positions to the grid.
									Vertex->X = Vertex->TempX;
									Vertex->Y = Vertex->TempY;
									*Vertex = Vertex->GridSnap(FVector(GGridSize, GGridSize, GGridSize));
									HaveChanges = true;
								}
							}
						}
					}
				}
			}
		}

		m_pointOldPos = MouseLocation;
		ComputeHandlePositions();
		InvalidateRect( hWnd, NULL, FALSE );

		WWindow::OnMouseMove( Flags, MouseLocation );
		unguard;
	}
	void ComputeHandlePositions( UBOOL bAlwaysCompute = FALSE)
	{
		guard(W2DShapeEditor::ComputeHandlePositions);
		for( int shape = 0 ; shape < m_shapes.Num() ; shape++ )
			if( !m_shapes(shape).Handle.IsSel() || bAlwaysCompute )
				m_shapes(shape).ComputeHandlePosition();
		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void ChangeSegmentTypes( int InType )
	{
		guard(W2DShapeEditor::ChangeSegmentTypes);
		if (AskEndDrawMode())
			return;

		UndoSaved = FALSE;
		for( int shape = 0 ; shape < m_shapes.Num() ; shape++ )
			for( int seg = 0 ; seg < m_shapes(shape).Segments.Num() ; seg++ )
				if( m_shapes(shape).Segments(seg).IsSel() )
				{
					MaybeSaveUndoState(TEXT("Set Segments Type"));
					m_shapes(shape).Segments(seg).SetSegType(InType);
					HaveChanges = true;
				}

		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void SetSegmentDetail( int InDetailLevel )
	{
		guard(W2DShapeEditor::SetSegmentDetail);
		if (AskEndDrawMode())
			return;
		UndoSaved = FALSE;
		GDefaultDetailLevel = InDetailLevel;
		for( int shape = 0 ; shape < m_shapes.Num() ; shape++ )
			for( int seg = 0 ; seg < m_shapes(shape).Segments.Num() ; seg++ )
				if( m_shapes(shape).Segments(seg).IsSel() )
				{
					MaybeSaveUndoState(TEXT("Set Segments Detail Level"));
					m_shapes(shape).Segments(seg).DetailLevel = InDetailLevel;
					HaveChanges = true;
				}

		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void OnLeftButtonDown(INT X, INT Y)
	{
		guard(W2DShapeEditor::OnLeftButtonDown);
		UndoSaved = FALSE;

		if (DrawShape)
		{
			m_pointOldPos = GetCursorPos();
			FLOAT X = m_pointOldPos.X - m_camera.X - (m_rcWnd.right / 2);
			FLOAT Y = -(m_pointOldPos.Y - m_camera.Y - (m_rcWnd.bottom / 2));
			DrawShapeClickPos = FVector(X/Zoom, Y/Zoom, 0).GridSnap(FVector(GGridSize, GGridSize, GGridSize));
		}
		else
		{	
			m_bDraggingVerts = TRUE;
			m_pointOldPos = GetCursorPos();

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
				for (int seg = 0; seg < m_shapes(shape).Segments.Num(); seg++)
				{
					for (int vertex = 0; vertex < 2; vertex++)
					{
						m_shapes(shape).Segments(seg).Vertex[vertex].SetTempPos();
						m_shapes(shape).Segments(seg).ControlPoint[vertex].SetTempPos();
					}
				}
			}
		}

		InvalidateRect( hWnd, NULL, FALSE );

		WWindow::OnLeftButtonDown(X, Y);
		unguard;
	}
	void OnLeftButtonUp()
	{
		guard(W2DShapeEditor::OnLeftButtonUp);

		if (DrawShape)
		{
			if (DrawShapeIndex == INDEX_NONE || DrawShapeIndex < 0 || DrawShapeIndex >= m_shapes.Num() || m_shapes(DrawShapeIndex).Segments.Num() == 0)
				StartDrawShape();
			else
			{
				F2DSEVector& LastPoint = m_shapes(DrawShapeIndex).Segments(m_shapes(DrawShapeIndex).Segments.Num() - 1).Vertex[1];
				F2DSEVector& FirstPoint = m_shapes(DrawShapeIndex).Segments(0).Vertex[0];
				if (m_shapes(DrawShapeIndex).Segments.Num() != 1 && (LastPoint - FirstPoint).IsNearlyZero()) // Wrong shape. Start new.
				{
					DrawShapeIndex = INDEX_NONE;
					StartDrawShape();
				}
				else if (!(LastPoint - DrawShapeClickPos).IsNearlyZero())
				{
					SaveUndoState(TEXT("Draw Shape Segment"));
					if (m_shapes(DrawShapeIndex).Segments.Num() == 1 && (LastPoint - FirstPoint).IsNearlyZero())
						m_shapes(DrawShapeIndex).Segments(0).Vertex[1] = DrawShapeClickPos;
					else
					{
						new(m_shapes(DrawShapeIndex).Segments)FSegment(LastPoint, DrawShapeClickPos);
						if ((DrawShapeClickPos - FirstPoint).IsNearlyZero()) // Closed shape?
						{
							if (m_shapes(DrawShapeIndex).Segments.Num() < 3) // Remove too small shapes.
								m_shapes.Remove(DrawShapeIndex);
						}
					}
				}
			}
			ComputeHandlePositions(TRUE);
			InvalidateRect( hWnd, NULL, FALSE );
		}
		else
		{
			ReleaseCapture();
			m_bDraggingVerts = FALSE;

			if( !m_bMouseHasMoved && !(GetAsyncKeyState(VK_CONTROL) & 0x8000) )
				DeselectAllVerts();
			ComputeHandlePositions();
		}

		WWindow::OnLeftButtonUp();
		unguard;
	}
	void StartDrawShape()
	{
		guard(W2DShapeEditor::StartDrawShape);
		SaveUndoState(TEXT("Start Draw Shape"));
		if (DrawShapeIndex >= 0 && DrawShapeIndex < m_shapes.Num() && m_shapes(DrawShapeIndex).Segments.Num() == 0)
			m_shapes.Remove(DrawShapeIndex);
		new(m_shapes)FShape();
		DrawShapeIndex = m_shapes.Num() - 1;
		new(m_shapes(DrawShapeIndex).Segments)FSegment(DrawShapeClickPos, DrawShapeClickPos);
		unguard;
	}
	void EndDrawMode()
	{
		guard(W2DShapeEditor::EndDrawMode);
		if (DrawShape)
			OnCommand(IDMN_2DSE_DRAW_SHAPE); // toggle it
		unguard;
	}
	UBOOL AskEndDrawMode()
	{
		guard(W2DShapeEditor::AskEndDrawMode);
		if (!DrawShape)
			return FALSE;
		if (::MessageBox(hWnd, TEXT("Draw Shape Mode active. End it?"), TEXT("2DShapeEditor"), MB_YESNO) == IDYES)
		{
			EndDrawMode();
			return FALSE;
		}
		return TRUE;
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
		guard(W2DShapeEditor::OnChooseColorButton);
		if (AskEndDrawMode())
			return;
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
			UndoSaved = FALSE;
			FColor ChosenLineColor = FColor(GetRValue(cc.rgbResult), GetGValue(cc.rgbResult), GetBValue(cc.rgbResult), 0);
			for (int shape = 0; shape < m_shapes.Num(); shape++)
			{
				for (int seg = 0; seg < m_shapes(shape).Segments.Num(); seg++)
				{
					if (m_shapes(shape).Segments(seg).IsSel())
					{
						MaybeSaveUndoState(TEXT("Set Segments Color"));
						m_shapes(shape).Segments(seg).LineColor = ChosenLineColor;
						HaveChanges = true;
					}
				}
			}
			ComputeHandlePositions();
		}
		unguard;
	}
	void PositionChildControls( void )
	{
		guard(W2DShapeEditor::PositionChildControls);

		if( !::IsWindow( GetDlgItem( hWnd, ID_2DSE_TOOLBAR )))	return;

		SetRedraw(false);

		FRect CR = GetClientRect();
		::MoveWindow(hWndToolBar, 4, 0, 27 * MulDiv(16, DPIX, 96), 27 * MulDiv(16, DPIY, 96), 1);
		RECT R;
		if (::GetWindowRect( GetDlgItem( hWnd, ID_2DSE_TOOLBAR ), &R ))
			::MoveWindow( GetDlgItem( hWnd, ID_2DSE_TOOLBAR ), 0, 0, CR.Max.X, R.bottom, TRUE );

		SetRedraw(true);

		unguard;
	}
	virtual void OnKeyDown( TCHAR Ch )
	{
		guard(W2DShapeEditor::OnKeyDown);
		// Hot keys from old version
		if( Ch == 'I' && GetKeyState(VK_CONTROL) & 0x8000)
			SplitSides();
		unguard;
	}
	// Rotate the shapes by the speifued angle, around the origin,
	void RotateShape( int _Angle )
	{
		guard(W2DShapeEditor::RotateShape);
		if (AskEndDrawMode())
			return;
		FVector l_vec;
		FRotator StepRotation( 0, (65536.0f / 360.0f)  * _Angle, 0 );

		UndoSaved = FALSE;
		for (int shape = 0; shape < m_shapes.Num(); shape++)
		{
			for (int seg = 0; seg < m_shapes(shape).Segments.Num(); seg++)
			{
				MaybeSaveUndoState(TEXT("Rotate Shapes"));
				for (int vertex = 0; vertex < 2; vertex++)
				{
					l_vec = m_shapes(shape).Segments(seg).Vertex[vertex];
					l_vec = m_origin + (l_vec - m_origin).TransformVectorBy(GMath.UnitCoords * StepRotation);

					m_shapes(shape).Segments(seg).Vertex[vertex] = l_vec.GridSnap(FVector(GGridSize, GGridSize, GGridSize));

					l_vec = m_shapes(shape).Segments(seg).ControlPoint[vertex];
					l_vec = m_origin + (l_vec - m_origin).TransformVectorBy(GMath.UnitCoords * StepRotation);

					m_shapes(shape).Segments(seg).ControlPoint[vertex] = l_vec.GridSnap(FVector(GGridSize, GGridSize, GGridSize));
				}
				HaveChanges = true;
			}
		}

		ComputeHandlePositions(1);
		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	// Flips the shape across the origin.
	void Flip(UBOOL _bHoriz, UBOOL Trans = FALSE)
	{
		guard(W2DShapeEditor::Flip);
		if (AskEndDrawMode())
			return;

		// Flip the vertices across the origin.
		if (Trans)
			UndoSaved = FALSE;
		for( int shape = 0 ; shape < m_shapes.Num() ; shape++ )
		{
			for( int seg = 0 ; seg < m_shapes(shape).Segments.Num() ; seg++ )
			{
				if (Trans)
					MaybeSaveUndoState(TEXT("Flip Shapes"));
				for( int vertex = 0 ; vertex < 2 ; vertex++ )
				{
					FVector Dist = m_shapes(shape).Segments(seg).Vertex[vertex] - m_origin;
					FVector CPDist = m_shapes(shape).Segments(seg).ControlPoint[vertex] - m_origin;

					if( _bHoriz )
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
				Exchange( m_shapes(shape).Segments(seg).Vertex[0], m_shapes(shape).Segments(seg).Vertex[1] );
				Exchange( m_shapes(shape).Segments(seg).ControlPoint[0], m_shapes(shape).Segments(seg).ControlPoint[1] );
				if (Trans)
					HaveChanges = true;
			}
		}

		ComputeHandlePositions(1);
		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	// IndexTo must be < IndexFrom. We assume shapes are valid, do not contain gaps or breaks, and segments are properly ordered.
	UBOOL MergeShape(INT IndexTo, INT IndexFrom)
	{
		guard(W2DShapeEditor::MergeShape);
		FShape& To =  m_shapes(IndexTo);
		FShape& From =  m_shapes(IndexFrom);
		TArray<FSegment> Merged;
		for (INT i = 0; i < To.Segments.Num(); i++)
		{
			FSegment& SegTo = To.Segments(i);
			INT Found = INDEX_NONE;
			for (INT j = 0; j < From.Segments.Num(); j++)
			{
				FSegment& SegFrom = From.Segments(j);
				if (SegTo == SegFrom || (SegTo.Vertex[0] == SegFrom.Vertex[1] && SegTo.Vertex[1] == SegFrom.Vertex[0]))
				{
					Found = j;
					break;
				}
			}
			if (Found != INDEX_NONE)
			{
				for (INT k = 0; k < i; k++)
					Merged.AddItem(To.Segments(k));
				FSegment& SegFrom = From.Segments(Found);
				UBOOL bReverse = SegTo == SegFrom;
				if (bReverse)
					for (INT j = 1; j < From.Segments.Num(); j++)
					{
						FSegment& SegFrom = From.Segments(j);
						Exchange(SegFrom.Vertex[0], SegFrom.Vertex[1]);
						Exchange(SegFrom.ControlPoint[0], SegFrom.ControlPoint[1]);
					}
				INT Dir = bReverse ? -1 : 1;
				Found += From.Segments.Num();
				for (INT j = 1; j < From.Segments.Num(); j++)
					Merged.AddItem(From.Segments((Found + j*Dir) % From.Segments.Num()));
				for (INT k = i + 1; k < To.Segments.Num(); k++)
					Merged.AddItem(To.Segments(k));
				Reduce(Merged);
				MaybeSaveUndoState(TEXT("Merge Shapes"));
				ExchangeArray(To.Segments, Merged);
				m_shapes.Remove(IndexFrom);
				return TRUE;
			}
		}
		return FALSE;
		unguard;
	}
	void Reduce(TArray<FSegment>& Segments)
	{
		guard(W2DShapeEditor::MergeShapes);
		UBOOL Found = TRUE;
		while (Segments.Num() && Found)
		{
			Found = FALSE;
			// Eliminate segments that fully overlap.
			// Such segments can be created by merging shapes.
			for (INT i = 0; i < Segments.Num(); i++)
			{
				INT IndexPrev = i == 0 ? Segments.Num() - 1 : i - 1;
				FSegment& SegPrev = Segments(IndexPrev);
				FSegment& SegCurr = Segments(i);
				if (SegPrev.Vertex[0] == SegCurr.Vertex[1] && SegPrev.Vertex[1] == SegCurr.Vertex[0])
				{
					if (i == 0)
					{
						Segments.Remove(IndexPrev);
						Segments.Remove(i);
					}
					else
					{
						Segments.Remove(i);
						Segments.Remove(IndexPrev);
					}
					Found = TRUE;
					break;
				}
			}
			// Eliminate segments with zero length.
			for (INT i = 0; i < Segments.Num(); i++)
			{
				FSegment& SegCurr = Segments(i);
				if (SegCurr.Vertex[0] == SegCurr.Vertex[1] && SegCurr.SegType == eSEGTYPE_LINEAR)
				{
					Segments.Remove(i);
					Found = TRUE;
					i--;
				}
			}
		}
		unguard;
	}
	void MergeShapes()
	{
		guard(W2DShapeEditor::MergeShapes);
		if (AskEndDrawMode())
			return;
		UndoSaved = FALSE;
		UBOOL Found = TRUE;
		while (Found)
		{
			Found = FALSE;
			for (INT i = 0; i < m_shapes.Num(); i++)
				if (m_shapes(i).Handle.IsSel())
					for (INT j = i + 1; j < m_shapes.Num(); j++)
						if (m_shapes(j).Handle.IsSel() && MergeShape(i, j))
						{
							j--;
							Found = TRUE;
						}
		}
		ComputeHandlePositions(TRUE);
		unguard;
	}
	void SelectAll()
	{
		guard(W2DShapeEditor::SelectAll);
		if (AskEndDrawMode())
			return;
		UndoSaved = FALSE;
		for (INT i = 0; i < m_shapes.Num(); i++)
		{
			if (!m_shapes(i).Handle.IsSel())
			{
				MaybeSaveUndoState(TEXT("Select All"));
				m_shapes(i).Handle.Select(1);
			}
			for (INT j = 0; j < m_shapes(i).Segments.Num(); j++)
				if (!m_shapes(i).Segments(j).IsSel())
				{
					MaybeSaveUndoState(TEXT("Select All"));
					m_shapes(i).SelectSegment(j, TRUE);
				}
		}
		InvalidateRect(hWnd, NULL, FALSE);
		unguard;
	}
	void DeselectAllVerts()
	{
		guard(W2DShapeEditor::DeselectAllVerts);
		MaybeSaveUndoState(TEXT("Select/Move Vertices"));
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
			{
				MaybeSaveUndoState(TEXT("Toggle Vertex Selection"));
				m_origin.SelectToggle();
			}
			return TRUE;
		}

		// Check shape handles...
		for( int shape = 0 ; shape < m_shapes.Num() ; shape++ )
		{
			l_vtxTest.X = (float)l_click.X - (m_shapes(shape).Handle.X * Zoom);
			l_vtxTest.Y = (float)l_click.Y + (m_shapes(shape).Handle.Y * Zoom);

			if( l_vtxTest.Size() <= d2dSE_SELECT_TOLERANCE )
			{
				if( !bJustChecking )
				{
					MaybeSaveUndoState(TEXT("Toggle Vertex Selection"));
					m_shapes(shape).Handle.SelectToggle();
				}
				return TRUE;
			}
		}

		// Check vertices...
		for( int shape = 0 ; shape < m_shapes.Num() ; shape++ )
			for( int seg = 0 ; seg < m_shapes(shape).Segments.Num() ; seg++ )
				for( int vertex = 0 ; vertex < 2 ; vertex++ )
				{
					l_vtxTest.X = (float)l_click.X - (m_shapes(shape).Segments(seg).Vertex[vertex].X * Zoom);
					l_vtxTest.Y = (float)l_click.Y + (m_shapes(shape).Segments(seg).Vertex[vertex].Y * Zoom);

					if( l_vtxTest.Size() <= d2dSE_SELECT_TOLERANCE )
						if( bJustChecking )
							return TRUE;
						else
						{
							MaybeSaveUndoState(TEXT("Toggle Vertex Selection"));
							m_shapes(shape).Segments(seg).Vertex[vertex].SelectToggle();
						}

					l_vtxTest.X = (float)l_click.X - (m_shapes(shape).Segments(seg).ControlPoint[vertex].X * Zoom);
					l_vtxTest.Y = (float)l_click.Y + (m_shapes(shape).Segments(seg).ControlPoint[vertex].Y * Zoom);

					if( l_vtxTest.Size() <= d2dSE_SELECT_TOLERANCE )
						if( bJustChecking )
							return TRUE;
						else
						{
							MaybeSaveUndoState(TEXT("Toggle Vertex Selection"));
							m_shapes(shape).Segments(seg).ControlPoint[vertex].SelectToggle();
						}
				}

		return FALSE;
		unguard;
	}
	void New( void )
	{
		guard(W2DShapeEditor::New);
		if (AskEndDrawMode())
			return;
		ResetUndoState();
		m_camera.X = m_camera.Y = 0;
		m_origin.X = 0; m_origin.Y = 0;

		m_shapes.Empty();
		InsertNewShape();

		for (INT i = 0; i < m_shapes.Num(); ++i)
			for (INT s = 0; s < m_shapes(i).Segments.Num(); ++s)
				m_shapes(i).SelectSegment(s, FALSE);

		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void InsertNewShape()
	{
		guard(W2DShapeEditor::InsertNewShape);
		if (AskEndDrawMode())
			return;
		SaveUndoState(TEXT("Insert New Shape"));

		F2DSEVector A(-128,-128,0), B(-128,128,0), C(128,128,0), D(128,-128,0);
		new(m_shapes)FShape();
		new(m_shapes(m_shapes.Num()-1).Segments)FSegment(A, B);
		new(m_shapes(m_shapes.Num()-1).Segments)FSegment(B, C);
		new(m_shapes(m_shapes.Num()-1).Segments)FSegment(C, D);
		new(m_shapes(m_shapes.Num()-1).Segments)FSegment(D, A);
		
		for (INT i = 0; i < 4; i++)
			m_shapes(m_shapes.Num() - 1).SelectSegment(i, TRUE);
		m_shapes(m_shapes.Num() - 1).Handle.Select(1);

		HaveChanges = true;

		ComputeHandlePositions(TRUE);

		unguard;
	}
	void InsertNewTriangle()
	{
		guard(W2DShapeEditor::InsertNewTriangle);
		if (AskEndDrawMode())
			return;
		SaveUndoState(TEXT("Insert New Triangle"));

		F2DSEVector A(-128, -128, 0), B(-128, 128, 0), C(128, 128, 0);
		new(m_shapes)FShape();
		new(m_shapes(m_shapes.Num() - 1).Segments)FSegment(A, B);
		new(m_shapes(m_shapes.Num() - 1).Segments)FSegment(B, C);
		new(m_shapes(m_shapes.Num() - 1).Segments)FSegment(C, A);

		for (INT i = 0; i < 3; i++)
			m_shapes(m_shapes.Num() - 1).SelectSegment(i, TRUE);
		m_shapes(m_shapes.Num() - 1).Handle.Select(1);

		HaveChanges = true;

		ComputeHandlePositions(TRUE);

		unguard;
	}
	void InsertNewCylinder(WDlgCylinderShape Dlg)
	{
		guard(W2DShapeEditor::InsertNewCylinder);
		if (AskEndDrawMode())
			return;
		SaveUndoState(TEXT("Insert New Cylinder"));

		F2DSEVector Coords[64];

		INT Sides = Dlg.Sides;
		FLOAT Radius = Dlg.Radius;
		FLOAT Ofs = 0;
		BOOL AlignToSide = Dlg.AlignToSide;

		if (Sides > 64)
		{
			appMsgf(TEXT("To many sides. Reducing to 64."));
			Sides = 64;
		}

		if (AlignToSide)
		{
			Radius /= cos(PI/Sides);
			Ofs = 1;
		}

		new(m_shapes)FShape();
		INT i = 0;
		for (i = 0; i < Sides; i++)
			Coords[i] = F2DSEVector(Radius * sin((2 * i + Ofs) * PI / Sides), Radius * cos((2 * i + Ofs) * PI / Sides), 0);

		for (i = 0; i < Sides; i++)
		{
			if (i == Sides - 1)
				new(m_shapes(m_shapes.Num() - 1).Segments)FSegment(Coords[i], Coords[0]);
			else new(m_shapes(m_shapes.Num() - 1).Segments)FSegment(Coords[i], Coords[i + 1]);
		}

		for (i = 0; i < Sides; i++)
			m_shapes(m_shapes.Num() - 1).SelectSegment(i, TRUE);
		m_shapes(m_shapes.Num() - 1).Handle.Select(1);

		HaveChanges = true;

		ComputeHandlePositions(TRUE);

		unguard;
	}
	void InsertFromSelectedSurface(UBOOL bFromBrush)
	{
		guard(W2DShapeEditor::InsertFromSelectedSurface);
		if (AskEndDrawMode())
			return;

		UModel* Model = GEditor->Level->Model;
		FBspSurf* Surf = NULL;
		UBOOL bFound = FALSE;
		UBOOL bFilledSurfNodes = FALSE;
		for (INT i = 0; i < Model->Surfs.Num(); i++)
		{
			Surf = &Model->Surfs(i);
			if ((Surf->PolyFlags & PF_Selected))
			{	
				if (!bFromBrush)
				{
					if (!bFilledSurfNodes && Surf->Nodes.Num() == 0)
					{
						bFilledSurfNodes = TRUE;
						Model->PostLoad();
					}
					if (Surf->Nodes.Num() > 0)
						for (INT n = 0; n < Surf->Nodes.Num(); n++)
							if (Model->Nodes(Surf->Nodes(n)).NumVertices >= 2)
							{
								bFound = TRUE;
								break;
							}
				}
				else if (bFromBrush && Surf->iBrushPoly >= 0 &&
					Surf->Actor && Surf->Actor->Brush && Surf->Actor->Brush->Polys &&
					Surf->iBrushPoly < Surf->Actor->Brush->Polys->Element.Num() &&
					Surf->Actor->Brush->Polys->Element(Surf->iBrushPoly).NumVertices >= 2)
				{
					bFound = TRUE;
				}
				if (bFound)
					break;
			}
		}
		if (!bFound)
		{
			::MessageBox(hWnd, TEXT("No suitable selected surfaces found"), *GetText(), MB_OK);
			return;
		}
		SaveUndoState(bFromBrush ? TEXT("Insert New Shape From Brush") : TEXT("Insert New Shape From BSP"));

		TArray<FBspNode*> Nodes;			
		for (INT n = 0; n < Surf->Nodes.Num(); n++)
		{
			FBspNode* Node = &Model->Nodes(Surf->Nodes(n));
		
			if (Node->NumVertices >= 2)
				Nodes(Nodes.Add()) = Node;
		}

		FVector Origin(0, 0, 0);
		INT Cnt = 0;
		FVector X, Normal;

		if (bFromBrush)
		{
			FPoly &Poly = Surf->Actor->Brush->Polys->Element(Surf->iBrushPoly);
			for (INT i = 0; i < Poly.NumVertices; i++)
				Origin += Poly.Vertex[i];
			Cnt += Poly.NumVertices;
			Normal = Poly.Normal;
			X = Poly.TextureU;
		}
		else
		{
			for (INT n = 0; n < Nodes.Num(); n++)
			{
				FVert* Verts = &Model->Verts(Nodes(n)->iVertPool);
				if (n == 0)
					X = Model->Points(Verts[1].pVertex) - Model->Points(Verts[0].pVertex);
				for (INT i = 0; i < Nodes(n)->NumVertices; i++)
					Origin += Model->Points(Verts[i].pVertex);
				Cnt += Nodes(n)->NumVertices;
			}
			Normal = Model->Vectors(Surf->vNormal);
			X = Model->Vectors(Surf->vTextureU);
		}
		Origin /= Cnt;

		X.Normalize();
		FVector Y = X ^ Normal;
	
		if (bFromBrush)
		{
			FPoly &Poly = Surf->Actor->Brush->Polys->Element(Surf->iBrushPoly);

			new(m_shapes)FShape();
			FShape& Shape = m_shapes(m_shapes.Num() - 1);
			TArray<FSegment> &Segments = Shape.Segments;

			F2DSEVector First, Prev, Curr;
			for (INT i = 0; i < Poly.NumVertices; i++)
			{
				FVector A = Poly.Vertex[i] - Origin;
				Curr.X = X | A;
				Curr.Y = Y | A;
				if (i == 0)
					First = Curr;
				else
					new(Segments)FSegment(Prev, Curr);
				Prev = Curr;
			}
			new(Segments)FSegment(Curr, First);

			for (INT i = 0; i < Shape.Segments.Num(); i++)
				Shape.SelectSegment(i, TRUE);
			Shape.Handle.Select(1);
		}
		else
		{
			for (INT n = 0; n < Nodes.Num(); n++)
			{
				new(m_shapes)FShape();
				FShape& Shape = m_shapes(m_shapes.Num() - 1);
				TArray<FSegment> &Segments = Shape.Segments;	

				FVert* Verts = &Model->Verts(Nodes(n)->iVertPool);

				F2DSEVector First, Prev, Curr;
				for (INT i = 0; i < Nodes(n)->NumVertices; i++)
				{
					FVector A = Model->Points(Verts[i].pVertex) - Origin;
					Curr.X = X | A;
					Curr.Y = Y | A;
					if (i == 0)
						First = Curr;
					else
						new(Segments)FSegment(Prev, Curr);
					Prev = Curr;
				}
				new(Segments)FSegment(Curr, First);

				for (INT i = 0; i < Shape.Segments.Num(); i++)
					Shape.SelectSegment(i, TRUE);
				Shape.Handle.Select(1);
			}
		}

		HaveChanges = true;

		ComputeHandlePositions(TRUE);

		unguard;
	}
	void DrawGrid( HDC _hdc )
	{
		guard(W2DShapeEditor::DrawGrid);
		float l_iXStart, l_iYStart, l_iXEnd, l_iYEnd;
		FVector l_vecTopLeft;
		HPEN l_penOriginLines, l_penMajorLines, l_penMinorLines, l_penOld;

		float FGridSizeZoom = Max(Zoom*GGridSize, 8.0f);
		
		l_vecTopLeft.X = (m_camera.X * -1) - (m_rcWnd.right / 2);
		l_vecTopLeft.Y = (m_camera.Y * -1) - (m_rcWnd.bottom / 2);

		l_penMinorLines = CreatePen( PS_SOLID, 1, RGB( 235, 235, 235 ) );
		l_penMajorLines = CreatePen( PS_SOLID, 1, RGB( 215, 215, 215 ) );
		l_penOriginLines = CreatePen( PS_SOLID, 3, RGB( 225, 225, 225 ) );

		// Snap the starting position to the grid size.
		//
		l_iXStart = -fmod(l_vecTopLeft.X, FGridSizeZoom);
		if (l_iXStart < 0)
			l_iXStart += FGridSizeZoom;
		l_iYStart = -fmod(l_vecTopLeft.Y, FGridSizeZoom);
		if (l_iYStart < 0)
			l_iYStart += FGridSizeZoom;

		l_iXEnd = l_iXStart + m_rcWnd.right;
		l_iYEnd = l_iYStart + m_rcWnd.bottom;
		
		// Draw the lines.
		//
		l_penOld = (HPEN)SelectObject( _hdc, l_penMinorLines );

		int Major = 8*GGridSize*Zoom;

		for( float y = l_iYStart ; y < l_iYEnd ; y += FGridSizeZoom )
		{
			if( l_vecTopLeft.Y + y == 0 )
				SelectObject( _hdc, l_penOriginLines );
			else
				if( !((int)(l_vecTopLeft.Y + y) % Major) )
					SelectObject( _hdc, l_penMajorLines );
				else
					SelectObject( _hdc, l_penMinorLines );

			::MoveToEx( _hdc, 0, y, NULL );
			::LineTo( _hdc, m_rcWnd.right, y );
		}

		for( float x = l_iXStart ; x < l_iXEnd ; x += FGridSizeZoom )
		{
			if( l_vecTopLeft.X + x == 0 )
				SelectObject( _hdc, l_penOriginLines );
			else
				if( !((int)(l_vecTopLeft.X + x) % Major) )
					SelectObject( _hdc, l_penMajorLines );
				else
					SelectObject( _hdc, l_penMinorLines );

			::MoveToEx( _hdc, x, 0, NULL );
			::LineTo( _hdc, x, m_rcWnd.bottom );
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
			   l_vecLoc.X - ((bitmap.bmWidth*Zoom*ImageZoom)/2), l_vecLoc.Y - ((bitmap.bmHeight*Zoom*ImageZoom)/2),
			   bitmap.bmWidth*Zoom*ImageZoom, bitmap.bmHeight*Zoom*ImageZoom,
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

		FVector l_vecLoc = m_camera;
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
		FVector l_vecLoc = m_camera;
		HPEN l_penLine, l_penBold, l_penCP, l_penCPBold, l_penOld;

		l_penLine = CreatePen( PS_SOLID, 1, RGB( 128, 128, 128 ) );
		l_penBold = CreatePen( PS_SOLID, 3, RGB( 128, 128, 128 ) );
		l_penCP = CreatePen( PS_SOLID, 1, RGB( 0, 0, 255 ) );
		l_penCPBold = CreatePen( PS_SOLID, 2, RGB( 0, 0, 255 ) );

		// Figure out where the top left corner of the window is in world coords.
		//
		l_vecLoc.X += m_rcWnd.right / 2;
		l_vecLoc.Y += m_rcWnd.bottom / 2;

		l_penOld = (HPEN)SelectObject( _hdc, l_penLine );

		for( int shape = 0 ; shape < m_shapes.Num() ; shape++ )
		{
			for( int seg = 0 ; seg < m_shapes(shape).Segments.Num() ; seg++ )
			{
				switch( m_shapes(shape).Segments(seg).SegType )
				{
					case eSEGTYPE_LINEAR:
						if (m_shapes(shape).Segments(seg).IsSel())
						{
							SelectObject(_hdc, l_penBold);
						}
						else
						{
							SelectObject(_hdc, GetStockObject(DC_PEN));
							SetDCPenColor(_hdc, RGB(m_shapes(shape).Segments(seg).LineColor.R, m_shapes(shape).Segments(seg).LineColor.G, m_shapes(shape).Segments(seg).LineColor.B));
						}
						::MoveToEx( _hdc,
							l_vecLoc.X + (m_shapes(shape).Segments(seg).Vertex[0].X * Zoom),
							l_vecLoc.Y - (m_shapes(shape).Segments(seg).Vertex[0].Y * Zoom), NULL );
						::LineTo( _hdc,
							l_vecLoc.X + (m_shapes(shape).Segments(seg).Vertex[1].X * Zoom),
							l_vecLoc.Y - (m_shapes(shape).Segments(seg).Vertex[1].Y * Zoom) );
						break;

					case eSEGTYPE_BEZIER:

						// Generate list of vertices for bezier curve and render them as a line.
						TArray<FVector> BezierPoints;
						m_shapes(shape).Segments(seg).GetBezierPoints( &BezierPoints );
						if (m_shapes(shape).Segments(seg).IsSel())
						{
							SelectObject(_hdc, l_penBold);
						}
						else
						{
							SelectObject(_hdc, GetStockObject(DC_PEN));
							SetDCPenColor(_hdc, RGB(m_shapes(shape).Segments(seg).LineColor.R, m_shapes(shape).Segments(seg).LineColor.G, m_shapes(shape).Segments(seg).LineColor.B));
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

						// Render the control points and connecting lines.
						SelectObject( _hdc, (m_shapes(shape).Segments(seg).ControlPoint[0].IsSel() ? l_penCPBold : l_penCP) );
						::MoveToEx( _hdc,
							l_vecLoc.X + (m_shapes(shape).Segments(seg).Vertex[0].X * Zoom),
							l_vecLoc.Y - (m_shapes(shape).Segments(seg).Vertex[0].Y * Zoom), NULL );
						::LineTo( _hdc,
							l_vecLoc.X + (m_shapes(shape).Segments(seg).ControlPoint[0].X * Zoom),
							l_vecLoc.Y - (m_shapes(shape).Segments(seg).ControlPoint[0].Y * Zoom) );

						SelectObject( _hdc, (m_shapes(shape).Segments(seg).ControlPoint[1].IsSel() ? l_penCPBold : l_penCP) );
						::MoveToEx( _hdc,
							l_vecLoc.X + (m_shapes(shape).Segments(seg).Vertex[1].X * Zoom),
							l_vecLoc.Y - (m_shapes(shape).Segments(seg).Vertex[1].Y * Zoom), NULL );
						::LineTo( _hdc,
							l_vecLoc.X + (m_shapes(shape).Segments(seg).ControlPoint[1].X * Zoom),
							l_vecLoc.Y - (m_shapes(shape).Segments(seg).ControlPoint[1].Y * Zoom) );
						break;
				}
			}
		}

		SelectObject( _hdc, l_penOld );
		DeleteObject( l_penLine );
		DeleteObject( l_penBold );
		DeleteObject( l_penCP );
		DeleteObject( l_penCPBold );
		unguard;
	}
	void DrawShapeHandles( HDC _hdc )
	{
		guard(W2DShapeEditor::DrawShapeHandles);
		FVector l_vecLoc = m_camera;
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
		FVector l_vecLoc = m_camera;
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
		if (AskEndDrawMode())
			return;

		// Break each selected segment into two.
		UndoSaved = FALSE;
		for (int shape = 0; shape < m_shapes.Num(); shape++)
		{
			for (int seg = 0; seg < m_shapes(shape).Segments.Num(); seg++)
			{
				if (m_shapes(shape).Segments(seg).IsSel())
				{
					MaybeSaveUndoState(TEXT("Split Sides"));
					// Create a new segment half the size of this one, starting from the middle and extending
					// to the second vertex.
					FVector HalfWay = m_shapes(shape).Segments(seg).GetHalfwayPoint();
					new(m_shapes(shape).Segments)FSegment(HalfWay, m_shapes(shape).Segments(seg).Vertex[1]);

					// Move the original segments ending point to the halfway point.
					m_shapes(shape).Segments(seg).Vertex[1] = HalfWay;

					HaveChanges = true;
				}
			}
		}		

		ComputeHandlePositions();
		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void Delete( void )
	{
		guard(W2DShapeEditor::Delete);
		if (AskEndDrawMode())
			return;

		// Delete any vertices which are selected on the current shape.
		UndoSaved = FALSE;
		for (INT shape = 0; shape < m_shapes.Num(); shape++)
		{
			FShape& Shape = m_shapes(shape);
			if (Shape.Handle.IsSel())
			{
				MaybeSaveUndoState(TEXT("Delete Shape/Vertices"));
				m_shapes.Remove(shape--);
				HaveChanges = true;
			}
			else if (Shape.Segments.Num() > 3)
			{
				for (INT seg = 0; seg < Shape.Segments.Num(); seg++)
				{
					if (Shape.Segments(seg).IsSel())
					{
						MaybeSaveUndoState(TEXT("Delete Shape/Vertices"));

						F2DSEVector v1 = Shape.Segments(seg).Vertex[0];
						F2DSEVector v2 = Shape.Segments(seg).Vertex[1];
						FVector HalfWay = Shape.Segments(seg).GetHalfwayPoint();

						Shape.Segments.Remove(seg--);

						for (INT i = 0; i < Shape.Segments.Num(); i++)
							if ((Shape.Segments(i).Vertex[1] - v1).IsNearlyZero())
								Shape.Segments(i).Vertex[1].Select(FALSE);

						if (!(v1 - v2).IsNearlyZero())
						{
							for (INT i = 0; i < Shape.Segments.Num(); i++)
							{
								if ((Shape.Segments(i).Vertex[0] - v2).IsNearlyZero())
									Shape.Segments(i).Vertex[0] = HalfWay;
								if ((Shape.Segments(i).Vertex[1] - v1).IsNearlyZero())
									Shape.Segments(i).Vertex[1] = HalfWay;
							}
						}

						HaveChanges = true;
					}
				}
			}
		}
				
		ComputeHandlePositions();
		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void FileSaveAs( HWND hWnd )
	{
		guard(W2DShapeEditor::FileSaveAs)
		FString File;
		if (GetSaveNameWithDialog(
			*MapFilename,
			*GLastDir[eLASTDIR_2DS],
			TEXT("2D Shapes (*.2ds)\0*.2ds\0All Files\0*.*\0\0"),
			TEXT("2ds"),
			TEXT("Save 2D Shape"),
			File
		))
		{
			MapFilename = File;
			WriteShape( *File );
			GLastDir[eLASTDIR_2DS] = appFilePathName(*File);
		}

		SetCaption();
		unguard;
	}
	void FileSave( HWND hWnd )
	{
		guard(W2DShapeEditor::FileSave)
		if( MapFilename.Len() > 0 )
			WriteShape( *MapFilename );
		else
			FileSaveAs( hWnd );
		unguard;
	}
	void FileOpen( HWND hWnd )
	{
		guard(W2DShapeEditor::FileOpen)
		if (AskEndDrawMode())
			return;
		TArray<FString> Files;

		if (OpenFilesWithDialog(
			*GLastDir[eLASTDIR_2DS],
			TEXT("2D Shapes (*.2ds)\0*.2ds\0All Files\0*.*\0\0"),
			TEXT("2ds"),
			TEXT("Open 2D Shape"),
			Files,
			FALSE
		))
		{
			MapFilename = Files(0);
			ReadShape( *MapFilename );
			SetCaption();
			GLastDir[eLASTDIR_2DS] = appFilePathName(*MapFilename);
		}
		unguard;
	}
	void SetCaption( void )
	{
		FString Caption = FString::Printf(TEXT("2D Shape Editor - [%ls]"), *MapFilename );
		SetText( *Caption );
	}
	void WriteShape( const TCHAR* Filename )
	{
		FArchive* Archive;
		Archive = GFileManager->CreateFileWriter( Filename );

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

			Archive->Close();
			HaveChanges = false;
		}
	}
	void ReadShape( const TCHAR* Filename )
	{
		FArchive* Archive;
		Archive = GFileManager->CreateFileReader( Filename );

		if( Archive )
		{
			ResetUndoState();
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
					Archive->Serialize( &(pSeg->Vertex[0].Z), sizeof(float) );
					Archive->Serialize( &(pSeg->Vertex[1].X), sizeof(float) );
					Archive->Serialize( &(pSeg->Vertex[1].Y), sizeof(float) );
					Archive->Serialize( &(pSeg->Vertex[1].Z), sizeof(float) );
					Archive->Serialize( &(pSeg->ControlPoint[0].X), sizeof(float) );
					Archive->Serialize( &(pSeg->ControlPoint[0].Y), sizeof(float) );
					Archive->Serialize( &(pSeg->ControlPoint[0].Z), sizeof(float) );
					Archive->Serialize( &(pSeg->ControlPoint[1].X), sizeof(float) );
					Archive->Serialize( &(pSeg->ControlPoint[1].Y), sizeof(float) );
					Archive->Serialize( &(pSeg->ControlPoint[1].Z), sizeof(float) );
					Archive->Serialize( &(pSeg->SegType), sizeof(float) );
					Archive->Serialize( &(pSeg->DetailLevel), sizeof(float) );
				}
			}
			ComputeHandlePositions();

			Archive->Close();
			HaveChanges = false;
		}

		InvalidateRect( hWnd, NULL, FALSE );
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
#define REVERSE(k) ((2*n + 1 - (k)) % n)
	void ProcessSheet()
	{
		guard(W2DShapeEditor::ProcessSheet)
		if (AskEndDrawMode())
			return;

		FString Cmd;

		// Reverse the Y Axis
		Flip(0);

		Cmd += TEXT("BRUSH SET\n\n");
		INT Poly = INDEX_NONE;

		for( int shape = 0 ; shape < m_shapes.Num() ; shape++ )
		{
			FShape &fShape = m_shapes(shape);
			fShape.Tesselate(m_origin);
			INT BasePoly = Poly + 1;
			if (!TessellateShapes && fShape.IsConvex())
			{
				INT n = fShape.Vertices.Num();
				Cmd += *FString::Printf(TEXT("Begin Polygon Flags=8 Link=%d\n"), BasePoly);
				Poly++;
				for (INT i = 0, Added = 0; i < n; i++)
				{
					if (Added++ >= FPoly::MAX_VERTICES)
					{
						Cmd += TEXT("End Polygon\n");
						Cmd += *FString::Printf(TEXT("Begin Polygon Flags=8 Link=%d\n"), BasePoly);
						Poly++;
						FVector &First = fShape.Vertices(REVERSE(0)).XY;
						Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), First.X, First.Y, First.Z));
						FVector &Last = fShape.Vertices(REVERSE(i - 1)).XY;
						Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), Last.X, Last.Y, Last.Z));
						Added = 3;
					}
					FVector &Vertex = fShape.Vertices(REVERSE(i)).XY;
					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), Vertex.X, Vertex.Y, Vertex.Z));
				}
				Cmd += TEXT("End Polygon\n");
			}
			else
			{
				for( TMap<int, CPOLY_TRIANGLE>::TIterator It(fShape.Triangles); It ; ++It )
				{
					CPOLY_TRIANGLE* ptri = &(It.Value());
					Exchange( ptri->Vertex1, ptri->Vertex2 );

					Cmd += *FString::Printf(TEXT("Begin Polygon Flags=8 Link=%d\n"), BasePoly);
					Poly++;

					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
						fShape.Vertices(ptri->Vertex1).XY.X, fShape.Vertices(ptri->Vertex1).XY.Y, fShape.Vertices(ptri->Vertex1).XY.Z ));
					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
						fShape.Vertices(ptri->Vertex2).XY.X, fShape.Vertices(ptri->Vertex2).XY.Y, fShape.Vertices(ptri->Vertex1).XY.Z ));
					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
						fShape.Vertices(ptri->Vertex3).XY.X, fShape.Vertices(ptri->Vertex3).XY.Y, fShape.Vertices(ptri->Vertex1).XY.Z ));

					Cmd += TEXT("End Polygon\n");
				}
			}
		}

		Flip(0);

		GEditor->Exec( *Cmd );
		SetDirection();
		unguard;
	}
	void ProcessExtrude( int Depth )
	{
		guard(W2DShapeEditor::ProcessExtrude)
		if (AskEndDrawMode())
			return;

		// Reverse the Y Axis
		Flip(0);

		FString Cmd;

		Cmd += TEXT("BRUSH SET\n\n");
		INT Poly = INDEX_NONE;

		for( int shape = 0 ; shape < m_shapes.Num() ; shape++ )
		{
			FShape &fShape = m_shapes(shape);
			fShape.Tesselate(m_origin);

			// Top and bottom caps ...
			//
			INT BasePoly = Poly + 1;
			if (!TessellateShapes && fShape.IsConvex())
			{
				INT n = fShape.Vertices.Num();
				FLOAT Z = (FLOAT)Depth/2.0f;
				Cmd += *FString::Printf(TEXT("Begin Polygon Flags=0 Link=%d\n"), BasePoly);
				Poly++;
				for (INT i = 0, Added = 0; i < n; i++)
				{
					if (Added++ >= FPoly::MAX_VERTICES)
					{
						Cmd += TEXT("End Polygon\n");
						Cmd += *FString::Printf(TEXT("Begin Polygon Flags=0 Link=%d\n"), BasePoly);
						Poly++;
						FVector &First = fShape.Vertices((0)).XY;
						Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), First.X, First.Y, Z));
						FVector &Last = fShape.Vertices((i - 1)).XY;
						Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), Last.X, Last.Y, Z));
						Added = 3;
					}
					FVector &Vertex = fShape.Vertices((i)).XY;
					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), Vertex.X, Vertex.Y, Z));
				}
				Cmd += TEXT("End Polygon\n");

				BasePoly = Poly + 1;
				Z = -Z;
				Cmd += *FString::Printf(TEXT("Begin Polygon Flags=0 Link=%d\n"), BasePoly);
				Poly++;
				for (INT i = 0, Added = 0; i < n; i++)
				{
					if (Added++ >= FPoly::MAX_VERTICES)
					{
						Cmd += TEXT("End Polygon\n");
						Cmd += *FString::Printf(TEXT("Begin Polygon Flags=0 Link=%d\n"), BasePoly);
						Poly++;
						FVector &First = fShape.Vertices(REVERSE(0)).XY;
						Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), First.X, First.Y, Z));
						FVector &Last = fShape.Vertices(REVERSE(i - 1)).XY;
						Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), Last.X, Last.Y, Z));
						Added = 3;
					}
					FVector &Vertex = fShape.Vertices(REVERSE(i)).XY;
					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), Vertex.X, Vertex.Y, Z));
				}
				Cmd += TEXT("End Polygon\n");				
			}
			else
			{
				for( TMap<int, CPOLY_TRIANGLE>::TIterator It(fShape.Triangles); It ; ++It )
				{
					CPOLY_TRIANGLE* ptri = &(It.Value());

					Cmd += *FString::Printf(TEXT("Begin Polygon Flags=0 Link=%d\n"), BasePoly);
					Poly++;

					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
						fShape.Vertices(ptri->Vertex1).XY.X, fShape.Vertices(ptri->Vertex1).XY.Y, Depth / 2.0f));
					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
						fShape.Vertices(ptri->Vertex2).XY.X, fShape.Vertices(ptri->Vertex2).XY.Y, Depth / 2.0f));
					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
						fShape.Vertices(ptri->Vertex3).XY.X, fShape.Vertices(ptri->Vertex3).XY.Y, Depth / 2.0f));

					Cmd += TEXT("End Polygon\n");

					Exchange( ptri->Vertex1, ptri->Vertex2 );

					Cmd += *FString::Printf(TEXT("Begin Polygon Flags=0 Link=%d\n"), BasePoly + 1);
					Poly++;

					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
						fShape.Vertices(ptri->Vertex1).XY.X, fShape.Vertices(ptri->Vertex1).XY.Y, -(Depth / 2.0f) ));
					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
						fShape.Vertices(ptri->Vertex2).XY.X, fShape.Vertices(ptri->Vertex2).XY.Y, -(Depth / 2.0f) ));
					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
						fShape.Vertices(ptri->Vertex3).XY.X, fShape.Vertices(ptri->Vertex3).XY.Y, -(Depth / 2.0f) ));

					Cmd += TEXT("End Polygon\n");
				}
			}

			// Sides ...
			//
			for( int vtx = 0 ; vtx < fShape.Vertices.Num() ; vtx++ )
			{
				Cmd += TEXT("Begin Polygon Flags=0\n");
				Poly++;

				FVector* pvtxPrev = &( fShape.Vertices( (vtx ? vtx - 1 : fShape.Vertices.Num() - 1 ) ).XY );
				FVector* pvtx = &( fShape.Vertices(vtx).XY );

				Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
					pvtx->X, pvtx->Y, Depth / 2.0f ));
				Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
					pvtxPrev->X, pvtxPrev->Y, Depth / 2.0f ));

				Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
					pvtxPrev->X, pvtxPrev->Y, -(Depth / 2.0f) ));
				Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
					pvtx->X, pvtx->Y, -(Depth / 2.0f) ));

				Cmd += TEXT("End Polygon\n");
			}
		}

		GEditor->Exec( *Cmd );

		Flip(0);

		SetDirection();
		unguard;
	}
	void ProcessExtrudeToPoint( int Depth )
	{
		guard(W2DShapeEditor::ProcessExtrudeToPoint)
		if (AskEndDrawMode())
			return;

		// Flip the Y Axis
		Flip(0);

		FString Cmd;

		Cmd += TEXT("BRUSH SET\n\n");
		INT Poly = INDEX_NONE;

		for( int shape = 0 ; shape < m_shapes.Num() ; shape++ )
		{
			FShape &fShape = m_shapes(shape);
			fShape.Tesselate(m_origin);

			INT BasePoly = Poly + 1;
			if (!TessellateShapes && fShape.IsConvex())
			{
				INT n = fShape.Vertices.Num();
				Cmd += *FString::Printf(TEXT("Begin Polygon Flags=0 Link=%d\n"), BasePoly);
				Poly++;
				for (INT i = 0, Added = 0; i < n; i++)
				{
					if (Added++ >= FPoly::MAX_VERTICES)
					{
						Cmd += TEXT("End Polygon\n");
						Cmd += *FString::Printf(TEXT("Begin Polygon Flags=0 Link=%d\n"), BasePoly);
						Poly++;
						FVector &First = fShape.Vertices(REVERSE(0)).XY;
						Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), First.X, First.Y, First.Z));
						FVector &Last = fShape.Vertices(REVERSE(i - 1)).XY;
						Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), Last.X, Last.Y, Last.Z));
						Added = 3;
					}
					FVector &Vertex = fShape.Vertices(REVERSE(i)).XY;
					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), Vertex.X, Vertex.Y, Vertex.Z));
				}
				Cmd += TEXT("End Polygon\n");
			}
			else
			{
				for( TMap<int, CPOLY_TRIANGLE>::TIterator It(fShape.Triangles); It ; ++It )
				{
					CPOLY_TRIANGLE* ptri = &(It.Value());
					Exchange( ptri->Vertex1, ptri->Vertex2 );

					Cmd += *FString::Printf(TEXT("Begin Polygon Flags=0 Link=%d\n"), BasePoly);
					Poly++;

					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
						fShape.Vertices(ptri->Vertex1).XY.X, fShape.Vertices(ptri->Vertex1).XY.Y, fShape.Vertices(ptri->Vertex1).XY.Z ));
					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
						fShape.Vertices(ptri->Vertex2).XY.X, fShape.Vertices(ptri->Vertex2).XY.Y, fShape.Vertices(ptri->Vertex1).XY.Z ));
					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
						fShape.Vertices(ptri->Vertex3).XY.X, fShape.Vertices(ptri->Vertex3).XY.Y, fShape.Vertices(ptri->Vertex1).XY.Z ));

					Cmd += TEXT("End Polygon\n");
				}
			}

			// Sides ...
			//
			for( int vtx = 0 ; vtx < fShape.Vertices.Num() ; vtx++ )
			{
				Cmd += TEXT("Begin Polygon Flags=0\n");
				Poly++;

				FVector* pvtxPrev = &( fShape.Vertices( (vtx ? vtx - 1 : fShape.Vertices.Num() - 1 ) ).XY );
				FVector* pvtx = &( fShape.Vertices(vtx).XY );

				Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
					pvtxPrev->X, pvtxPrev->Y, pvtxPrev->Z ));
				Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
					pvtx->X, pvtx->Y, pvtx->Z ));
				Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
					m_origin.X, m_origin.Y, m_origin.Z + Depth ));

				Cmd += TEXT("End Polygon\n");
			}
		}

		GEditor->Exec( *Cmd );

		Flip(0);

		SetDirection();
		unguard;
	}
	void ProcessExtrudeToBevel( int Depth, int CapHeight )
	{
		guard(W2DShapeEditor::ProcessExtrudeToBevel)
		if (AskEndDrawMode())
			return;

		// Flip the Y Axis
		Flip(0);

		FString Cmd;
		float Dist = 1.0f - (CapHeight / (float)Depth);

		Cmd += TEXT("BRUSH SET\n\n");
		INT Poly = INDEX_NONE;

		for( int shape = 0 ; shape < m_shapes.Num() ; shape++ )
		{
			FShape &fShape = m_shapes(shape);
			fShape.Tesselate(m_origin);

			INT BasePoly = Poly + 1;
			if (!TessellateShapes && fShape.IsConvex())
			{
				INT n = fShape.Vertices.Num();
				Cmd += *FString::Printf(TEXT("Begin Polygon Flags=0 Link=%d\n"), BasePoly);
				Poly++;
				for (INT i = 0, Added = 0; i < n; i++)
				{
					if (Added++ >= FPoly::MAX_VERTICES)
					{
						Cmd += TEXT("End Polygon\n");
						Cmd += *FString::Printf(TEXT("Begin Polygon Flags=0 Link=%d\n"), BasePoly);
						Poly++;
						FVector &First = fShape.Vertices(REVERSE(0)).XY;
						Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), First.X, First.Y, First.Z));
						FVector &Last = fShape.Vertices(REVERSE(i - 1)).XY;
						Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), Last.X, Last.Y, Last.Z));
						Added = 3;
					}
					FVector &Vertex = fShape.Vertices(REVERSE(i)).XY;
					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), Vertex.X, Vertex.Y, Vertex.Z));
				}
				Cmd += TEXT("End Polygon\n");

				BasePoly = Poly + 1;
				Cmd += *FString::Printf(TEXT("Begin Polygon Flags=0 Link=%d\n"), BasePoly);
				Poly++;
				for (INT i = 0, Added = 0; i < n; i++)
				{
					if (Added++ >= FPoly::MAX_VERTICES)
					{
						Cmd += TEXT("End Polygon\n");
						Cmd += *FString::Printf(TEXT("Begin Polygon Flags=0 Link=%d\n"), BasePoly);
						Poly++;
						FVector First = fShape.Vertices((0)).XY*Dist;
						Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), First.X, First.Y, (float)CapHeight));
						FVector Last = fShape.Vertices((i - 1)).XY*Dist;
						Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), Last.X, Last.Y, (float)CapHeight));
						Added = 3;
					}
					FVector Vertex = fShape.Vertices((i)).XY*Dist;
					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), Vertex.X, Vertex.Y, (float)CapHeight));
				}
				Cmd += TEXT("End Polygon\n");
			}
			else 
			{
				for( TMap<int, CPOLY_TRIANGLE>::TIterator It(fShape.Triangles); It ; ++It )
				{
					CPOLY_TRIANGLE* ptri = &(It.Value());
					Exchange( ptri->Vertex1, ptri->Vertex2 );

					Cmd += *FString::Printf(TEXT("Begin Polygon Flags=0 Link=%d\n"), BasePoly);
					Poly++;

					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
						fShape.Vertices(ptri->Vertex1).XY.X, fShape.Vertices(ptri->Vertex1).XY.Y, fShape.Vertices(ptri->Vertex1).XY.Z ));
					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
						fShape.Vertices(ptri->Vertex2).XY.X, fShape.Vertices(ptri->Vertex2).XY.Y, fShape.Vertices(ptri->Vertex1).XY.Z ));
					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
						fShape.Vertices(ptri->Vertex3).XY.X, fShape.Vertices(ptri->Vertex3).XY.Y, fShape.Vertices(ptri->Vertex1).XY.Z ));

					Cmd += TEXT("End Polygon\n");

					Exchange( ptri->Vertex1, ptri->Vertex2 );

					Cmd += *FString::Printf(TEXT("Begin Polygon Flags=0 Link=%d\n"), BasePoly + 1);
					Poly++;

					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
						fShape.Vertices(ptri->Vertex1).XY.X * Dist, fShape.Vertices(ptri->Vertex1).XY.Y * Dist, (float)CapHeight ));
					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
						fShape.Vertices(ptri->Vertex2).XY.X * Dist, fShape.Vertices(ptri->Vertex2).XY.Y * Dist, (float)CapHeight ));
					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
						fShape.Vertices(ptri->Vertex3).XY.X * Dist, fShape.Vertices(ptri->Vertex3).XY.Y * Dist, (float)CapHeight ));

					Cmd += TEXT("End Polygon\n");
				}
			}

			// Sides ...
			//
			for( int vtx = 0 ; vtx < fShape.Vertices.Num() ; vtx++ )
			{
				Cmd += TEXT("Begin Polygon Flags=0\n");
				Poly++;

				FVector* pvtxPrev = &( fShape.Vertices( (vtx ? vtx - 1 : fShape.Vertices.Num() - 1 ) ).XY );
				FVector* pvtx = &( fShape.Vertices(vtx).XY );

				Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
					pvtxPrev->X, pvtxPrev->Y, pvtxPrev->Z ));
				Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
					pvtx->X, pvtx->Y, pvtx->Z ));

				Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
					pvtx->X * Dist, pvtx->Y * Dist, (float)CapHeight ));
				Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
					pvtxPrev->X * Dist, pvtxPrev->Y * Dist, (float)CapHeight ));

				Cmd += TEXT("End Polygon\n");
			}
		}

		GEditor->Exec( *Cmd );

		Flip(0);

		SetDirection();
		unguard;
	}
#define REVERSE_IF(cond, k) ((cond) ? REVERSE(k) : (k))
	void ProcessRevolve( int TotalSides, int Sides )
	{
		guard(W2DShapeEditor::ProcessRevolve)
		if (AskEndDrawMode())
			return;

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

		FString Cmd;

		Cmd += TEXT("BRUSH SET\n\n");
		INT Poly = INDEX_NONE;

		for( int shape = 0 ; shape < m_shapes.Num() ; shape++ )
		{
			FShape &fShape = m_shapes(shape);
			fShape.Tesselate(m_origin);

			FRotator StepRotation((65536.0f/TotalSides)*Sides, 0, 0);
			FCoords LastCoords = GMath.UnitCoords*StepRotation;

			if( Sides != TotalSides )	// Don't make end caps for a complete revolve
			{
				INT BasePoly = Poly + 1;
				if (!TessellateShapes && fShape.IsConvex())
				{
					INT n = fShape.Vertices.Num();
					Cmd += *FString::Printf(TEXT("Begin Polygon Flags=0 Link=%d\n"), BasePoly);
					Poly++;
					for (INT i = 0, Added = 0; i < n; i++)
					{
						if (Added++ >= FPoly::MAX_VERTICES)
						{
							Cmd += TEXT("End Polygon\n");
							Cmd += *FString::Printf(TEXT("Begin Polygon Flags=0 Link=%d\n"), BasePoly);
							Poly++;
							FVector &First = fShape.Vertices(REVERSE_IF(bFromLeftSide, 0)).XY;
							Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), First.X, First.Y, First.Z));
							FVector &Last = fShape.Vertices(REVERSE_IF(bFromLeftSide, i - 1)).XY;
							Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), Last.X, Last.Y, Last.Z));
							Added = 3;
						}
						FVector &Vertex = fShape.Vertices(REVERSE_IF(bFromLeftSide, i)).XY;
						Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), Vertex.X, Vertex.Y, Vertex.Z));
					}
					Cmd += TEXT("End Polygon\n");

					BasePoly = Poly + 1;
					Cmd += *FString::Printf(TEXT("Begin Polygon Flags=0 Link=%d\n"), BasePoly);
					Poly++;
					for (INT i = 0, Added = 0; i < n; i++)
					{
						if (Added++ >= FPoly::MAX_VERTICES)
						{
							Cmd += TEXT("End Polygon\n");
							Cmd += *FString::Printf(TEXT("Begin Polygon Flags=0 Link=%d\n"), BasePoly);
							Poly++;
							FVector First = fShape.Vertices(REVERSE_IF(!bFromLeftSide, 0)).XY.TransformVectorBy(LastCoords);
							Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), First.X, First.Y, First.Z));
							FVector Last = fShape.Vertices(REVERSE_IF(!bFromLeftSide, i - 1)).XY.TransformVectorBy(LastCoords);
							Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), Last.X, Last.Y, Last.Z));
							Added = 3;
						}
						FVector Vertex = fShape.Vertices(REVERSE_IF(!bFromLeftSide, i)).XY.TransformVectorBy(LastCoords);
						Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), Vertex.X, Vertex.Y, Vertex.Z));
					}
					Cmd += TEXT("End Polygon\n");

				}
				else
				{
					for( TMap<int, CPOLY_TRIANGLE>::TIterator It(fShape.Triangles); It ; ++It )
					{
						CPOLY_TRIANGLE* ptri = &(It.Value());

						if( bFromLeftSide ) Exchange( ptri->Vertex1, ptri->Vertex2 );

						Cmd += *FString::Printf(TEXT("Begin Polygon Flags=0 Link=%d\n"), BasePoly);
						Poly++;

						Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
							fShape.Vertices(ptri->Vertex1).XY.X, fShape.Vertices(ptri->Vertex1).XY.Y, fShape.Vertices(ptri->Vertex1).XY.Z ));
						Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
							fShape.Vertices(ptri->Vertex2).XY.X, fShape.Vertices(ptri->Vertex2).XY.Y, fShape.Vertices(ptri->Vertex1).XY.Z ));
						Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
							fShape.Vertices(ptri->Vertex3).XY.X, fShape.Vertices(ptri->Vertex3).XY.Y, fShape.Vertices(ptri->Vertex1).XY.Z ));

						Cmd += TEXT("End Polygon\n");

						// Flip/Rotate the triangle to create the end cap
						Exchange( ptri->Vertex1, ptri->Vertex2 );

						FVector vtx1 = fShape.Vertices(ptri->Vertex1).XY.TransformVectorBy(LastCoords);
						FVector vtx2 = fShape.Vertices(ptri->Vertex2).XY.TransformVectorBy(LastCoords);
						FVector vtx3 = fShape.Vertices(ptri->Vertex3).XY.TransformVectorBy(LastCoords);

						Cmd += *FString::Printf(TEXT("Begin Polygon Flags=0 Link=%d\n"), BasePoly + 1);
						Poly++;

						Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
							vtx1.X, vtx1.Y, vtx1.Z ));
						Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
							vtx2.X, vtx2.Y, vtx2.Z ));
						Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
							vtx3.X, vtx3.Y, vtx3.Z ));

						Cmd += TEXT("End Polygon\n");
					}
				}
			}

			// Sides ...
			//
			for( int side = 0 ; side < Sides ; side++ )
				for( int vtx = 0 ; vtx < fShape.Vertices.Num() ; vtx++ )
				{
					Cmd += TEXT("Begin Polygon Flags=0\n");
					Poly++;

					FVector *pvtx, *pvtxPrev;

					pvtxPrev = &( fShape.Vertices( (vtx ? vtx - 1 : fShape.Vertices.Num() - 1 ) ).XY );
					pvtx = &( fShape.Vertices(vtx).XY );

					if( bFromLeftSide )	Exchange( pvtxPrev, pvtx );

					FRotator StepRotation( (65536.0f / TotalSides) * side, 0, 0 ), StepRotation2( (65536.0f / TotalSides) * (side+1), 0, 0 );

					FVector vtxPrev = pvtxPrev->TransformVectorBy( GMath.UnitCoords * StepRotation );
					FVector fvtx = pvtx->TransformVectorBy( GMath.UnitCoords * StepRotation );
					FVector vtxPrev2 = pvtxPrev->TransformVectorBy( GMath.UnitCoords * StepRotation2 );
					FVector vtx2 = pvtx->TransformVectorBy( GMath.UnitCoords * StepRotation2 );
					
					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), vtx2.X, vtx2.Y, vtx2.Z ));
					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), fvtx.X, fvtx.Y, fvtx.Z ));
					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), vtxPrev.X, vtxPrev.Y, vtxPrev.Z ));
					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"), vtxPrev2.X, vtxPrev2.Y, vtxPrev2.Z ));

					VectorTrim(fvtx);
					VectorTrim(vtx2);
					VectorTrim(vtxPrev2);
					FVector Normal = (fvtx - vtx2) ^ (vtx2 - vtxPrev2);
					Normal.Normalize();
					FVector U = fvtx - vtx2;
					U.Normalize();
					FVector V = U ^ Normal;
					V.Normalize();

					Cmd += *(FString::Printf(TEXT("TextureU   %f, %f, %f\n"), U.X, U.Y, U.Z ));
					Cmd += *(FString::Printf(TEXT("TextureV   %f, %f, %f\n"), V.X, V.Y, V.Z ));

					Cmd += TEXT("End Polygon\n");
				}
		}

		GEditor->Exec( *Cmd );

		Flip(0);

		SetDirection();
		unguard;
	}
};

void F2DEditorState::Set(const W2DShapeEditor* Other, const TCHAR* InAction)
{
	guard(F2DEditorState:Set);
	Action = InAction;
	m_origin = Other->m_origin;
	m_shapes = Other->m_shapes;
	unguard
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
