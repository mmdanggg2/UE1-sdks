
#include "Engine.h"
#include "ScriptedAIEdClasses.h"
#include "UnRender.h"

constexpr TCHAR ScConfigInit[] = TEXT("ScriptedAIEd.ini");

struct FRenderEntry;
struct FActionEntry;
struct FVariableEntry;
struct FKismetAPI;

void GetClassDesc(UClass* C, FStringOutputDevice& Output);

class UEditorEngine : public UEngine, public FNotifyHook
{
};

extern UEditorEngine* GEngine;

void OpenMainWindow();

#define DECLARE_TDCLASS( TClass ) \
	public: \
	friend FArchive &operator<<( FArchive& Ar, TClass*& Res ) \
				{ return Ar << *(UObject**)&Res; } \
	virtual ~TClass() \
				{ ConditionalDestroy(); } \
	static void InternalConstructor( void* X ) \
				{ new( (EInternal*)X )TClass(); } \
	static UClass* StaticClass() \
	{ \
		static UClass* INTERNAL_CLASS = FindObjectChecked<UClass>(NULL,*FString::Printf(TEXT("ScriptedAI.%ls"),TEXT(#TClass)+1),1); \
		return INTERNAL_CLASS; \
	} \
	TClass(){}

struct FGroupBoxInfo
{
	FString GroupInfo;
	INT Pos[2], Size[2];

	FGroupBoxInfo(const FString& NewGrp, INT x, INT y)
		: GroupInfo(NewGrp)
	{
		Pos[0] = x;
		Pos[1] = y;
		Size[0] = 128;
		Size[1] = 128;
	}
	const TCHAR* GetGroupText(INT* Color) const;
};

struct FEditScriptScope
{
	UBOOL bOldScriptable;

	FEditScriptScope()
		: bOldScriptable(GIsScriptable)
	{
		GIsScriptable = TRUE;
		ALevelInfo::CurrentLevel->bBegunPlay = TRUE;
	}
	~FEditScriptScope()
	{
		GIsScriptable = bOldScriptable;
		ALevelInfo::CurrentLevel->bBegunPlay = (bOldScriptable != 0);
	}
};
struct FEditActorScriptScope : private FEditScriptScope
{
	AActor* Actor;
	ALevelInfo* OldLevel;

	FEditActorScriptScope(AActor* inActor)
		: FEditScriptScope(), Actor(inActor), OldLevel(inActor->Level)
	{
		inActor->Level = ALevelInfo::CurrentLevel;
	}
	~FEditActorScriptScope()
	{
		Actor->Level = OldLevel;
	}
};

class SCRIPTEDAIED_API AScriptedAI : public AKeypoint
{
	DECLARE_TDCLASS(AScriptedAI);

	FLOAT WinPos[2] GCC_PACK(INT_ALIGNMENT);
	FLOAT WinZoom;
	class USActionBase* TickList;
	class USObjectBase* BeginPlayList;
	TArrayNoInit<class USObjectBase*> Actions;
	TArrayNoInit<FGroupBoxInfo> GroupsInfo;
	BITFIELD bInit : 1 GCC_PACK(INT_ALIGNMENT);
	BITFIELD bLogMessages : 1;

	void CheckStatic();
	void Initialize();
	void RebuildList();

	USObjectBase* AddAction(UClass* ActionClass, INT X, INT Y);

	DECLARE_FUNCTION(execAddAction);
	void eventEditorInitTrigger();
	UBOOL eventEditorFilterAction(UClass* Item);
	void eventEditorDrawFrame(class UCanvas* Canvas);
};

class SCRIPTEDAIED_API USObjectBase : public UObject
{
	DECLARE_TDCLASS(USObjectBase);

	INT Location[2] GCC_PACK(INT_ALIGNMENT);
	class AScriptedAI* Trigger;
	class ALevelInfo* Level;
	class USObjectBase* NextBeginPlay;
	FColor DrawColor;
	FStringNoInit MenuName;
	FStringNoInit Description;
	void* EditData;
	BYTE ObjType;
	BITFIELD bRequestBeginPlay : 1 GCC_PACK(INT_ALIGNMENT);
	BITFIELD bReqPostBeginPlay : 1;
	BITFIELD bEditorInit : 1;
	BITFIELD bEditDirty : 1;
	BITFIELD bContainsActorRef : 1;

	void Initialize();
	void eventOnInitialize();
	void eventOnRemoved();
	FString eventGetUIName(BITFIELD bGroupName);

	DECLARE_FUNCTION(execLinkAction);
	DECLARE_FUNCTION(execGet3DViewport);
};

struct FEventLink
{
	class USActionBase* Link GCC_PACK(INT_ALIGNMENT);
	BYTE OutIndex;
	INT X GCC_PACK(INT_ALIGNMENT);
	INT Y;
};
enum eVarLinkStatus : BYTE
{
	VARLINK_Ok,
	VARLINK_Mismatch,
	VARLINK_NonLValue,
};
struct FVariableLink
{
	friend FActionEntry;

	FString Name GCC_PACK(INT_ALIGNMENT);
	FName PropName;
	class UClass* ExpectedType;
	class USVariableBase* Link;
	BITFIELD bOutput : 1 GCC_PACK(INT_ALIGNMENT);
	BITFIELD bInput : 1;
	class UProperty* LinkedProp GCC_PACK(INT_ALIGNMENT);
	INT X;
	INT Y;
	INT Color;

	eVarLinkStatus CanLinkTo(USVariableBase* Other);
	inline UObject** GetLinkedVarOffset(UObject* Obj)
	{
		if (LinkedProp)
		{
			// Unlink UScript variable too
			return (UObject**)(((BYTE*)Obj) + LinkedProp->Offset);
		}
		return NULL;
	}
protected:
	void SetVarLink(UObject* Obj, USVariableBase* NewLink);
};
class SCRIPTEDAIED_API USActionBase : public USObjectBase
{
	DECLARE_TDCLASS(USActionBase);

	class USActionBase* NextTimed GCC_PACK(INT_ALIGNMENT);
	TArrayNoInit<FString> OutputLinks;
	TArrayNoInit<FString> InputLinks;
	TArrayNoInit<FEventLink> LinkedOutput;
	TArrayNoInit<FVariableLink> VarLinks;
	FStringNoInit PostInfoText;
	BITFIELD bRequestTick : 1 GCC_PACK(INT_ALIGNMENT);
	BITFIELD bTickEnabled : 1;
	BITFIELD bClientAction : 1;

	void InitEditor();
	FActionEntry* GetActionEntry(FKismetAPI* API);
};

class SCRIPTEDAIED_API USVariableBase : public USObjectBase
{
	DECLARE_TDCLASS(USVariableBase);

	BITFIELD bNoReset : 1 GCC_PACK(INT_ALIGNMENT);
	BITFIELD bConstant : 1;

	FString eventGetInfo();
	void eventGetToolbar(TArray<FString>& Ar);
	void eventOnSelectToolbar(INT Index);
	FVariableEntry* GetVarEntry(FKismetAPI* API);
};

struct FRenderArea
{
	FVector2D Base;
	FLOAT Zoom;
	F2DBox Bounds;

	FRenderArea()
		: Zoom(1.f), Base(0.f, 0.f), Bounds(FVector2D(0.f, 0.f), FVector2D(1.f, 1.f))
	{}
	inline void UpdateBounds(AScriptedAI* AI, FSceneNode* Frame)
	{
		Zoom = AI->WinZoom;
		const FLOAT SizeX = Frame->FX / Zoom;
		const FLOAT SizeY = Frame->FY / Zoom;
		Base = FVector2D(AI->WinPos[0] - (SizeX * 0.5f), AI->WinPos[1] - (SizeY * 0.5f));
		Bounds = F2DBox(Base, FVector2D(Base.X + SizeX, Base.Y + SizeY));
	}
	inline FVector2D ToScreen(const FVector2D& V) const
	{
		return (V - Base) * Zoom;
	}
	inline FVector2D ToScreenScale(const FVector2D& V) const
	{
		return V * Zoom;
	}
	inline FVector2D ToScreenXY(FLOAT X, FLOAT Y) const
	{
		return (FVector2D(X, Y) - Base) * Zoom;
	}
	inline FLOAT ToScreenX(FLOAT X) const
	{
		return (X - Base.X) * Zoom;
	}
	inline FLOAT ToScreenY(FLOAT Y) const
	{
		return (Y - Base.Y) * Zoom;
	}
	inline FVector2D ToWorld(const FVector2D& V) const
	{
		return (V / Zoom) + Base;
	}
	inline FVector2D ToWorldScale(const FVector2D& V) const
	{
		return V / Zoom;
	}
};
struct FMouseTarget
{
	FRenderEntry* Entry;
	DWORD Index;

	FMouseTarget()
		: Entry(nullptr)
	{}
	inline void Clear()
	{
		Entry = nullptr;
	}
	inline void Set(FRenderEntry* E, DWORD Ix = 0)
	{
		Entry = E;
		Index = Ix;
	}
	inline UBOOL Matches(const FMouseTarget& T) const
	{
		return (Entry == T.Entry) && (Index == T.Index);
	}
};

enum ERenderOrder : BYTE
{
	RENDORDER_Action,
	RENDORDER_EventLines,
	RENDORDER_Groups,
};
enum EEntityType : BYTE
{
	ENTITY_Unknown,
	ENTITY_EventLine,
	ENTITY_Variable,
	ENTITY_Action,
	ENTITY_Group,
};
struct FRenderEntry
{
private:
	FKismetAPI* API;
	FRenderEntry* Next;
	BYTE Priority;

public:
	F2DBox Bounds;

	FRenderEntry(FKismetAPI* A, BYTE InPri);
	virtual ~FRenderEntry() noexcept(false);
	virtual void Init() {}
	virtual void Draw(FSceneNode* Frame, const FRenderArea& Area) = 0;
	virtual void CollectGarbage() {}
	virtual UBOOL Refresh()
	{
		return FALSE;
	}
	virtual void CheckMouseHit(const FVector2D& Mouse, FMouseTarget& Result) {}
	virtual void GainFocus(DWORD Index) {}
	virtual void LostFocus(DWORD Index) {}
	virtual void OnMouseClick(DWORD Index) {}
	virtual void DragEvent(const FVector2D& MouseDelta) {}
	virtual UBOOL BeginDrag(const FVector2D& Mouse) { return FALSE; }
	virtual EEntityType GetType() const = 0;

	inline FRenderEntry* GetNext() const
	{
		return Next;
	}
	inline FKismetAPI* GetAPI() const
	{
		return API;
	}
};
struct FEventLine : public FRenderEntry
{
private:
	TArray<FVector2D> Points;

public:
	FPlane LineColor;
	UBOOL Highlight;

	FEventLine(FKismetAPI* A);
	void TraceLink(const FVector2D& Start, const FVector2D& SDir, const FVector2D& End, const FVector2D& EDir);
	void UpdateLink(const FVector2D& Start, const FVector2D& End, UBOOL bVariable = FALSE);
	void Draw(FSceneNode* Frame, const FRenderArea& Area) override;

	EEntityType GetType() const override
	{
		return ENTITY_EventLine;
	}
};
struct FDynEventLine : public FEventLine
{
	enum ELineMode : BYTE
	{
		LINEMODE_Static,
		LINEMODE_VarEnd,
		LINEMODE_VarStart,
	};
private:
	FVector2D StartPos, StartDir, EndDir;
	ELineMode LineMode;

public:
	FMouseTarget MouseCache;
	FDynEventLine(FKismetAPI* A, const FVector2D& Start, const FVector2D& SDir, const FVector2D& EDir, ELineMode Mode);
	~FDynEventLine() noexcept(false);
	void MouseMove(const FVector2D& MousePos);
};
struct FObjectEntry : public FRenderEntry
{
protected:
	USObjectBase* Obj;

public:
	UBOOL bIsEditing; // Currently editing this object properties.

	FObjectEntry(FKismetAPI* A, USObjectBase* O);
	~FObjectEntry() noexcept(false);
	virtual void Update(FSceneNode* Frame) = 0;
	virtual void DisconnectAll() = 0;

	inline USObjectBase* GetObject() const
	{
		return Obj;
	}
};
struct FVariableEntry : public FObjectEntry
{
	friend FActionEntry;

	struct FVarLink
	{
		FActionEntry* Action;
		INT VarIndex;

		FVarLink(FActionEntry* E, INT ix)
			: Action(E), VarIndex(ix)
		{}
		inline void Set(FActionEntry* E, INT ix)
		{
			Action = E;
			VarIndex = ix;
		}
		inline UBOOL operator==(const FVarLink& L) const
		{
			return (L.Action == Action && L.VarIndex == VarIndex);
		}
	};
protected:
	FVector2D Position, MidPosition;
	FPlane VarColor, HiVarColor;
	FString Info;
	FVector2D TextPos, InfoPos;
	UBOOL Highlight;
	TArray<FVarLink> VarLinks;

public:
	FVariableEntry(FKismetAPI* A, USVariableBase* O);
	void Update(FSceneNode* Frame) override;
	void Draw(FSceneNode* Frame, const FRenderArea& Area) override;
	FVector2D GetInputPos(FSceneNode* Frame, const FVector2D& Start);
	void CheckMouseHit(const FVector2D& Mouse, FMouseTarget& Result) override;
	void GainFocus(DWORD Index) override;
	void LostFocus(DWORD Index) override;
	void OnMouseClick(DWORD Index) override;
	void DragEvent(const FVector2D& MouseDelta) override;
	UBOOL BeginDrag(const FVector2D& Mouse) override;
	void DisconnectAll() override;

	inline FPlane GetDrawColor() const
	{
		return VarColor;
	}
	inline USVariableBase* GetVariable() const
	{
		return reinterpret_cast<USVariableBase*>(Obj);
	}
	EEntityType GetType() const override
	{
		return ENTITY_Variable;
	}
};
struct FActionEntry : public FObjectEntry
{
	friend FVariableEntry;

	struct FOtherSideLink
	{
		FActionEntry* Action;
		INT OtherIndex;

		FOtherSideLink(FActionEntry* E, INT ix)
			: Action(E), OtherIndex(ix)
		{}
		inline void Set(FActionEntry* E, INT ix)
		{
			Action = E;
			OtherIndex = ix;
		}
		inline UBOOL operator==(const FOtherSideLink& L) const
		{
			return (L.Action == Action && L.OtherIndex == OtherIndex);
		}
	};
	struct FInputLink
	{
		TArray<FOtherSideLink> Inputs;
		FLOAT Y;
		UBOOL Highlight;
	};
	struct FOutputLink
	{
		FOtherSideLink Output;
		FEventLine* Line;
		FLOAT TextX, Y;
		UBOOL Highlight;
	};
	struct FVarLink
	{
		FVariableEntry* Var;
		FEventLine* Line;
		FLOAT TextX, X;
		FPlane VarColor;
		UBOOL Highlight;
	};
protected:
	FString Header;
	FVector2D Position, Size, InputBase, OutputBase;
	FLOAT HeaderX, HeaderHeight, ContentY, ContentYS, InputYDelta, OutputYDelta, VarTextY, VarY;
	FPlane ActionColor, HighlightColor;
	TArray<FInputLink> InputLinks;
	TArray<FOutputLink> OutputLinks;
	TArray<FVarLink> VarLinks;
	UBOOL Highlight, ClientAction;

public:
	FActionEntry(FKismetAPI* A, USActionBase* O);
	~FActionEntry() noexcept(false);
	void CollectGarbage() override;
	UBOOL Refresh() override;
	FVector2D GetInputPos(FSceneNode* Frame, INT Index);
	void Update(FSceneNode* Frame) override;
	void Init() override;
	void Draw(FSceneNode* Frame, const FRenderArea& Area) override;
	void CheckMouseHit(const FVector2D& Mouse, FMouseTarget& Result) override;
	void GainFocus(DWORD Index) override;
	void LostFocus(DWORD Index) override;
	void OnMouseClick(DWORD Index) override;
	void DragEvent(const FVector2D& MouseDelta) override;
	UBOOL BeginDrag(const FVector2D& Mouse) override;
	void DisconnectAll() override;

	UBOOL VerifyLinks();
	void LinkOutput(FActionEntry* Act, INT OutIndex, INT InIndex);
	void UnlinkOutput(INT OutIndex);
	void LinkVariable(FVariableEntry* Var, INT VarIndex);
	UBOOL TryLinkVariable(FVariableEntry* Var, INT VarIndex);

	inline USActionBase* GetAction() const
	{
		return reinterpret_cast<USActionBase*>(Obj);
	}
	EEntityType GetType() const override
	{
		return ENTITY_Action;
	}
};
struct FGroupObject : public FRenderEntry
{
	FString Title;
	FVector2D Position, Size;
	FLOAT TextY;
	INT GroupIndex, ColorIndex;
	UBOOL Highlight;

	FGroupObject(FKismetAPI* A, INT Index);
	~FGroupObject() noexcept(false);
	void Draw(FSceneNode* Frame, const FRenderArea& Area) override;
	UBOOL Refresh() override;
	void CheckMouseHit(const FVector2D& Mouse, FMouseTarget& Result) override;
	void GainFocus(DWORD Index) override;
	void LostFocus(DWORD Index) override;
	void OnMouseClick(DWORD Index) override;
	void DragEvent(const FVector2D& MouseDelta) override;
	UBOOL BeginDrag(const FVector2D& Mouse) override;

	EEntityType GetType() const override
	{
		return ENTITY_Group;
	}
};
