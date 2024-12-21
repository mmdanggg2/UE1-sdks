
#include "ScriptedAI.h"

#define NAMES_ONLY
#define AUTOGENERATE_NAME(name) FName SCRIPTEDAIED_##name=FName(TEXT(#name),FNAME_Intrinsic);
#define AUTOGENERATE_FUNCTION(cls,idx,name) IMPLEMENT_FUNCTION (cls, idx, name)
#include "ScriptedAIEdClasses.h"
#undef NAMES_ONLY
#undef AUTOGENERATE_NAME
#undef AUTOGENERATE_FUNCTION

#define REPLACE_FUNCTION(FuncName,FuncRef) { UFunction* F = FindObjectChecked<UFunction>(NULL, TEXT("ScriptedAI.") FuncName); F->Func = (Native)&FuncRef; F->FunctionFlags |= FUNC_Native; }

UEditorEngine* GEngine = NULL;

IMPLEMENT_PACKAGE(ScriptedAIEd)

IMPLEMENT_CLASS(UNativeHook);

void UNativeHook::execShowEdWindow(FFrame& Stack, RESULT_DECL)
{
	guard(UNativeHook::execShowEdWindow);
	P_FINISH;

	if (!GIsEditor)
	{
		Stack.Logf(NAME_ScriptWarning, TEXT("Kismet browser can only be opened in editor."));
		return;
	}
	if (!GEngine)
	{
		GEngine = CastChecked<UEditorEngine>(UEngine::GEngine);
		REPLACE_FUNCTION(TEXT("ScriptedAI.AddAction"), AScriptedAI::execAddAction);
		REPLACE_FUNCTION(TEXT("SObjectBase.LinkAction"), USObjectBase::execLinkAction);
		REPLACE_FUNCTION(TEXT("SObjectBase.Get3DViewport"), USObjectBase::execGet3DViewport);
	}
	OpenMainWindow();
	unguard;
}

void AScriptedAI::Initialize() // Just verify...
{
	guard(AScriptedAI::Initialize);
	bInit = TRUE;
	RebuildList();

	UObject* Top = TopOuter();
	for (INT i = 0; i < Actions.Num(); ++i)
	{
		USObjectBase* O = Actions(i);
		if (O->GetOuter() != Top)
			O->Rename(NULL, Top); // Correct an outer mistake from earlier version to allow copy-paste kismet actors.
	}
	unguardobj;
}

void AScriptedAI::RebuildList()
{
	guard(AScriptedAI::RebuildList);
	BeginPlayList = NULL;

	for (INT i = 0; i < Actions.Num(); ++i)
	{
		USObjectBase* O = Actions(i);
		if (!O)
		{
			Actions.Remove(i--, 1);
			continue;
		}
		O->Trigger = this;
		O->Level = Level;
		if (O->bRequestBeginPlay)
		{
			O->NextBeginPlay = BeginPlayList;
			BeginPlayList = O;
		}
		else O->NextBeginPlay = NULL;
	}
	unguard;
}

void USObjectBase::eventOnInitialize()
{
	guard(USObjectBase::eventOnInitialize);
	static FName NAME_OnInitialize(TEXT("OnInitialize"), FNAME_Intrinsic);
	FEditScriptScope ScScope;
	ProcessEvent(FindFunctionChecked(NAME_OnInitialize), NULL);
	unguardobj;
}
void USObjectBase::eventOnRemoved()
{
	guard(USObjectBase::eventOnRemoved);
	static FName NAME_OnRemoved(TEXT("OnRemoved"), FNAME_Intrinsic);
	FEditScriptScope ScScope;
	ProcessEvent(FindFunctionChecked(NAME_OnRemoved), NULL);
	unguardobj;
}

struct GetUIName_Parms
{
	GetUIName_Parms(BITFIELD bG)
		: bGroupName(bG)
	{}
	BITFIELD bGroupName;
	FString Result;
};
FString USObjectBase::eventGetUIName(BITFIELD bGroupName)
{
	guard(USObjectBase::eventGetUIName);
	static FName NAME_GetUIName = TEXT("GetUIName");
	GetUIName_Parms Parms(bGroupName);
	FEditScriptScope ScScope;
	ProcessEvent(FindFunctionChecked(NAME_GetUIName), &Parms);
	return Parms.Result;
	unguardobj;
}
FString USVariableBase::eventGetInfo()
{
	static FName NAME_GetInfo = TEXT("GetInfo");
	FString Res;
	FEditScriptScope ScScope;
	ProcessEvent(FindFunctionChecked(NAME_GetInfo),&Res);
	return Res;
}
void USVariableBase::eventGetToolbar(TArray<FString>& Ar)
{
	static FName NAME_GetToolbar = TEXT("GetToolbar");
#if _USE_REF_PARMS
	struct { TArray<FString>* Entries; } Parms = { &Ar };
#else
	struct { TArray<FString> Entries; } Parms = { Ar };
#endif
	FEditScriptScope ScScope;
	ProcessEvent(FindFunctionChecked(NAME_GetToolbar), &Parms);
#if !_USE_REF_PARMS
	Ar = Parms.Entries;
#endif
}
void USVariableBase::eventOnSelectToolbar(INT Index)
{
	static FName NAME_OnSelectToolbar = TEXT("OnSelectToolbar");
	FEditScriptScope ScScope;
	ProcessEvent(FindFunctionChecked(NAME_OnSelectToolbar), &Index);
}

void AScriptedAI::eventEditorInitTrigger()
{
	guardSlow(AScriptedAI::eventEditorInitTrigger);
	static FName NAME_EditorInitTrigger = TEXT("EditorInitTrigger");
	FEditActorScriptScope ScScope(this);
	ProcessEvent(FindFunctionChecked(NAME_EditorInitTrigger), NULL);
	unguardSlow;
}
UBOOL AScriptedAI::eventEditorFilterAction(UClass* Item)
{
	guardSlow(AScriptedAI::eventEditorFilterAction);
	struct { UClass* InItem; BITFIELD ReturnValue; } Parms = { Item, 0 };
	static FName NAME_EditorFilterAction = TEXT("EditorFilterAction");
	FEditActorScriptScope ScScope(this);
	ProcessEvent(FindFunctionChecked(NAME_EditorFilterAction), &Parms);
	return (Parms.ReturnValue != 0);
	unguardSlow;
}
void AScriptedAI::eventEditorDrawFrame(class UCanvas* Canvas)
{
	guard(AScriptedAI::eventEditorDrawFrame);
	static FName NAME_EditorDrawFrame = TEXT("EditorDrawFrame");
	FEditActorScriptScope ScScope(this);
	ProcessEvent(FindFunctionChecked(NAME_EditorDrawFrame), &Canvas);
	unguard;
}

void AScriptedAI::execAddAction(FFrame& Stack, RESULT_DECL)
{
	guard(AScriptedAI::execAddAction);
	P_GET_CLASS(ActionClass);
	P_GET_INT(X);
	P_GET_INT(Y);
	P_FINISH;

	if (!ActionClass || (ActionClass->ClassFlags & CLASS_Abstract))
	{
		Stack.Logf(NAME_ScriptWarning, TEXT("AddAction FAILED: ActionClass is NULL or action is Abstract!"));
		return;
	}
	RESULT_GET(USObjectBase*) = AddAction(ActionClass, X, Y);
	unguard;
}

USObjectBase* AScriptedAI::AddAction(UClass* ActionClass, INT X, INT Y)
{
	guard(AScriptedAI::AddAction);
	USObjectBase* KObj = ConstructObject<USObjectBase>(ActionClass, GetOuter());
	if (!KObj)
		return nullptr;
	Actions.AddItem(KObj);

	KObj->Location[0] = X;
	KObj->Location[1] = Y;
	KObj->Level = Level;
	KObj->Trigger = this;
	CheckStatic();

	if (KObj->bRequestBeginPlay)
	{
		KObj->NextBeginPlay = BeginPlayList;
		BeginPlayList = KObj;
	}

	if (!KObj->bEditorInit)
	{
		KObj->bEditorInit = TRUE;
		KObj->Initialize();
	}
	return KObj;
	unguard;
}

void USObjectBase::execLinkAction(FFrame& Stack, RESULT_DECL)
{
	guard(USObjectBase::execLinkAction);
	P_GET_OBJECT(USObjectBase, Other);
	P_GET_INT_OPTX(OutIndex, 0);
	P_GET_INT_OPTX(InIndex, 0);
	P_FINISH;

	RESULT_GET(UBOOL) = FALSE;
	if (!Other)
		return;
	if (ObjType == Other->ObjType)
	{
		if (ObjType != 1) // Both sides must be an action!
			return;
		USActionBase* A = reinterpret_cast<USActionBase*>(this);
		USActionBase* B = reinterpret_cast<USActionBase*>(Other);
		if (!A->OutputLinks.IsValidIndex(OutIndex) || !B->InputLinks.IsValidIndex(InIndex))
			return;
		if (A->LinkedOutput.Num() <= OutIndex)
			A->LinkedOutput.AddZeroed(OutIndex + 1 - A->LinkedOutput.Num());
		A->LinkedOutput(OutIndex).Link = B;
		A->LinkedOutput(OutIndex).OutIndex = InIndex;
		RESULT_GET(UBOOL) = TRUE;
	}
	else
	{
		USActionBase* A = NULL;
		USVariableBase* B = NULL;
		if (ObjType == 1)
		{
			A = reinterpret_cast<USActionBase*>(this);
			if (Other->ObjType != 2) // Unknown type!
				return;
			B = reinterpret_cast<USVariableBase*>(Other);
		}
		else if (ObjType == 2)
		{
			// Flip order, if we're linking Var -> Action.
			B = reinterpret_cast<USVariableBase*>(this);
			if (Other->ObjType != 1) // Unknown type!
				return;
			A = reinterpret_cast<USActionBase*>(Other);
			OutIndex = InIndex;
		}
		else return;

		if (!A->VarLinks.IsValidIndex(OutIndex) || A->VarLinks(OutIndex).CanLinkTo(B) != VARLINK_Ok)
			return;
		A->VarLinks(OutIndex).Link = B;
		RESULT_GET(UBOOL) = TRUE;
	}
	unguard;
}

void USObjectBase::execGet3DViewport(FFrame& Stack, RESULT_DECL)
{
	guard(USObjectBase::execGet3DViewport);
	P_FINISH;

	if (!UEngine::GEngine->Client)
		return;
	APlayerPawn* BestActor = NULL;
	const auto& Viewports = UEngine::GEngine->Client->Viewports;
	for (INT i = 0; i < Viewports.Num(); ++i)
	{
		APlayerPawn* P = Viewports(i)->Actor;
		if (P)
		{
			if ((P->RendMap >= REN_Wire && P->RendMap <= REN_PlainTex) || (P->RendMap >= REN_LightingOnly && P->RendMap <= REN_Normals))
			{
				BestActor = P;
				break;
			}
			else if (P->RendMap >= REN_Wire && P->RendMap < REN_TexView)
				BestActor = P;
		}
	}
	if (!BestActor && Viewports.Num()) // Couldn't find anything, pick anything!
		BestActor = Viewports(0)->Actor;

	RESULT_GET(APlayerPawn*) = BestActor;
	unguard;
}

eVarLinkStatus FVariableLink::CanLinkTo(USVariableBase* Other)
{
	if (ExpectedType && !Other->GetClass()->IsChildOf(ExpectedType))
		return VARLINK_Mismatch;
	if (bOutput && Other->bConstant)
		return VARLINK_NonLValue;
	return VARLINK_Ok;
}

void USObjectBase::Initialize()
{
	guard(USObjectBase::Initialize);
	bEditDirty = TRUE;
	if (ObjType == 1)
		((USActionBase*)this)->InitEditor();
	eventOnInitialize();
	unguardf((TEXT("(%ls)"), GetClass()->GetPathName()));
}

void AScriptedAI::CheckStatic()
{
	bStatic = 1;

	for (int i = (Actions.Num() - 1); i >= 0; --i)
	{
		USActionBase* O = (USActionBase*)Actions(i);
		if (!O)
		{
			Actions.Remove(i);
			continue;
		}
		if (O->ObjType == 1 && O->bRequestTick)
		{
			bStatic = 0;
			return;
		}
	}
}

void USActionBase::InitEditor()
{
	guard(USActionBase::InitEditor);
	for (INT i = 0; i < VarLinks.Num(); ++i)
	{
		FVariableLink& V = VarLinks(i);
		if (V.PropName != NAME_None)
		{
			UObjectProperty* P = FindField<UObjectProperty>(GetClass(), *V.PropName);
			if (!P)
				GWarn->Logf(TEXT("%ls has invalid variable link to variable %ls!"), GetFullName(), *V.PropName);
			else V.ExpectedType = P->PropertyClass;
			V.LinkedProp = P;
		}
	}
	unguardobj;
}

void FActionEntry::Init()
{
	guard(FActionEntry::Init);
	USActionBase* U = GetAction();
	VerifyLinks();
	U->bEditDirty = TRUE;
	INT i;

	for (i = 0; i < U->LinkedOutput.Num(); ++i)
	{
		FEventLink& E = U->LinkedOutput(i);
		if (E.Link && E.Link->InputLinks.IsValidIndex(E.OutIndex))
		{
			FActionEntry* A = E.Link->GetActionEntry(GetAPI());
			A->VerifyLinks();
			OutputLinks(i).Output.Set(A, E.OutIndex);
			new (A->InputLinks(E.OutIndex).Inputs) FOtherSideLink(this, i);
			E.Link->bEditDirty = TRUE;
		}
		else E.Link = NULL;
	}
	for (i = 0; i < U->VarLinks.Num(); ++i)
	{
		FVariableLink& V = U->VarLinks(i);
		if (V.Link)
		{
			FVariableEntry* A = V.Link->GetVarEntry(GetAPI());
			VarLinks(i).Var = A;
			new (A->VarLinks) FVariableEntry::FVarLink(this, i);
			V.Link->bEditDirty = TRUE;
		}
	}
	unguard;
}

UBOOL FActionEntry::VerifyLinks()
{
	guard(FActionEntry::VerifyLinks);
	USActionBase* U = GetAction();
	UBOOL bResult = FALSE;
	if (InputLinks.Num() != U->InputLinks.Num())
	{
		bResult = TRUE;
		if (InputLinks.Num() < U->InputLinks.Num())
			InputLinks.AddZeroed(U->InputLinks.Num() - InputLinks.Num());
		else
		{
			// Unlink any excessive links.
			for (INT i = U->InputLinks.Num(); i < InputLinks.Num(); ++i)
			{
				FInputLink& E = InputLinks(i);
				for (INT j = 0; j < E.Inputs.Num(); ++j)
				{
					E.Inputs(j).Action->UnlinkOutput(E.Inputs(j).OtherIndex);
				}
			}
			InputLinks.Remove(U->InputLinks.Num(), InputLinks.Num() - U->InputLinks.Num());
		}
	}
	if (OutputLinks.Num() != U->OutputLinks.Num())
	{
		bResult = TRUE;
		if (U->OutputLinks.Num() > OutputLinks.Num())
			OutputLinks.AddZeroed(U->OutputLinks.Num() - OutputLinks.Num());
		else
		{
			// Unlink any excessive links.
			for (INT i = U->OutputLinks.Num(); i < OutputLinks.Num(); ++i)
			{
				if (OutputLinks(i).Output.Action)
					UnlinkOutput(i);
			}
			OutputLinks.Remove(U->OutputLinks.Num(), OutputLinks.Num() - U->OutputLinks.Num());
		}
	}
	if (U->LinkedOutput.Num() > U->OutputLinks.Num())
	{
		U->LinkedOutput.Remove(U->OutputLinks.Num(), U->LinkedOutput.Num() - U->OutputLinks.Num());
	}
	if (VarLinks.Num() != U->VarLinks.Num())
	{
		bResult = TRUE;
		if (U->VarLinks.Num() > VarLinks.Num())
			VarLinks.AddZeroed(U->VarLinks.Num() - VarLinks.Num());
		else
		{
			// Unlink any excessive links.
			for (INT i = U->VarLinks.Num(); i < VarLinks.Num(); ++i)
			{
				if (VarLinks(i).Var)
					LinkVariable(NULL, i);
			}
			VarLinks.Remove(U->VarLinks.Num(), VarLinks.Num() - U->VarLinks.Num());
		}
	}
	return bResult;
	unguard;
}

// Helper functions
void FActionEntry::LinkOutput(FActionEntry* Act, INT OutIndex, INT InIndex)
{
	guard(FActionEntry::LinkOutput);
	FOutputLink& L = OutputLinks(OutIndex);
	if (L.Output.Action == Act && L.Output.OtherIndex == InIndex)
	{
		UnlinkOutput(OutIndex);
		return;
	}
	USActionBase* U = GetAction();
	if (L.Output.Action)
	{
		L.Output.Action->InputLinks(L.Output.OtherIndex).Highlight = FALSE;
		L.Output.Action->InputLinks(L.Output.OtherIndex).Inputs.RemoveItem(FOtherSideLink(this, OutIndex));
	}
	if (L.Line)
	{
		delete L.Line;
		L.Line = nullptr;
	}
	L.Highlight = FALSE;
	L.Output.Set(Act, InIndex);
	if (Act)
	{
		Act->InputLinks(InIndex).Inputs.AddItem(FOtherSideLink(this, OutIndex));
		if (!U->LinkedOutput.IsValidIndex(OutIndex))
			U->LinkedOutput.AddZeroed(OutIndex + 1 - GetAction()->LinkedOutput.Num());
		U->LinkedOutput(OutIndex).Link = Act->GetAction();
		U->LinkedOutput(OutIndex).OutIndex = InIndex;
	}
	else if (U->LinkedOutput.IsValidIndex(OutIndex))
		U->LinkedOutput(OutIndex).Link = nullptr;
	U->bEditDirty = TRUE;
	unguard;
}
void FActionEntry::UnlinkOutput(INT OutIndex)
{
	guard(FActionEntry::UnlinkOutput);
	FOutputLink& L = OutputLinks(OutIndex);
	if (L.Output.Action)
	{
		L.Output.Action->InputLinks(L.Output.OtherIndex).Highlight = FALSE;
		L.Output.Action->InputLinks(L.Output.OtherIndex).Inputs.RemoveItem(FOtherSideLink(this, OutIndex));
	}
	if (L.Line)
	{
		delete L.Line;
		L.Line = nullptr;
	}
	L.Highlight = FALSE;
	L.Output.Set(nullptr, INDEX_NONE);
	if (GetAction()->LinkedOutput.IsValidIndex(OutIndex))
		GetAction()->LinkedOutput(OutIndex).Link = nullptr;
	GetAction()->bEditDirty = TRUE;
	unguard;
}
void FActionEntry::LinkVariable(FVariableEntry* Var, INT VarIndex)
{
	guard(FActionEntry::LinkVariable);
	FVarLink& V = VarLinks(VarIndex);
	if (V.Var)
	{
		V.Var->Highlight = FALSE;
		V.Var->VarLinks.RemoveItem(FVariableEntry::FVarLink(this, VarIndex));
	}
	if (V.Line)
	{
		delete V.Line;
		V.Line = nullptr;
	}
	V.Highlight = FALSE;
	if (V.Var == Var)
		V.Var = nullptr;
	else
	{
		V.Var = Var;
		if (Var)
			new (Var->VarLinks) FVariableEntry::FVarLink(this, VarIndex);
	}
	GetAction()->VarLinks(VarIndex).SetVarLink(GetAction(), Var ? Var->GetVariable() : nullptr);
	GetAction()->bEditDirty = TRUE;
	unguard;
}

void FVariableLink::SetVarLink(UObject* Obj, USVariableBase* NewLink)
{
	guard(FVariableLink::SetVarLink);
	Link = NewLink;
	UObject** Ref = GetLinkedVarOffset(Obj);
	if (Ref)
		*Ref = NewLink;
	unguard;
}

FActionEntry* USActionBase::GetActionEntry(FKismetAPI* API)
{
	guard(USActionBase::GetActionEntry);
	if (!bEditorInit)
	{
		bEditorInit = TRUE;
		Initialize();
	}
	FActionEntry* Result = reinterpret_cast<FActionEntry*>(EditData);
	if (!Result)
	{
		Result = new FActionEntry(API, this);
		EditData = Result;
	}
	return Result;
	unguard;
}
FVariableEntry* USVariableBase::GetVarEntry(FKismetAPI* API)
{
	guard(USVariableBase::GetVarEntry);
	if (!bEditorInit)
	{
		bEditorInit = TRUE;
		Initialize();
	}
	FVariableEntry* Result = reinterpret_cast<FVariableEntry*>(EditData);
	if (!Result)
	{
		Result = new FVariableEntry(API, this);
		EditData = Result;
	}
	return Result;
	unguard;
}

void GetClassDesc(UClass* C, FStringOutputDevice& Output)
{
	FString Result;
	const TCHAR* S = *C->ScriptText->Text;
	while (*S)
	{
		// Ship whitespaces.
		while (*S == ' ' || *S == '\t' || *S == '\r' || *S == '\n')
			++S;

		if (*S != '/') // Begin actual code, end here.
			break;
		++S;
		if (*S == '/') // Comment until end of line.
		{
			++S;

			// Skip comment formatting block and whitespaces.
			while (*S == ' ' || *S == '\t' || *S == '=' || *S == '/')
				++S;

			// End if end of comment line.
			if (*S == '\r' || *S == '\n')
				continue;

			const TCHAR* Start = S;
			while (*S && *S != '\r' && *S != '\n')
				++S;

			// Has enough information to go with.
			if (Start <= (S + 1))
			{
				if (Result.Len())
					Result += FString(TEXT("\n")) + FString(Start, S);
				else Result = FString(Start, S);
			}
		}
		else if (*S == '*') // Comment block
		{
			++S;
			while (*S)
			{
				// Skip comment formatting block and whitespaces.
				while (*S == ' ' || *S == '\t' || *S == '=' || *S == '/' || *S == '\r' || *S == '\n')
					++S;

				// Reached end of comment block.
				if (*S == '*' && S[1] == '/')
					break;

				const TCHAR* Start = S;
				while (*S && *S != '\r' && *S != '\n')
					++S;

				// Has enough information to go with.
				if (Start <= (S + 1))
				{
					if (Result.Len())
						Result += FString(TEXT("\n")) + FString(Start, S);
					else Result = FString(Start, S);
				}
			}
		}
		else break;
	}

	if (Result.Len() < 3)
		Output.Log(TEXT("\n(no comments)"));
	else
	{
		Output.Log(TEXT("\n"));
		Output.Log(Result);
	}
}

//===============================================================================================
// Render
