/*=============================================================================
	UnScript.h: UnrealScript execution engine.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

/*-----------------------------------------------------------------------------
	Native functions.
-----------------------------------------------------------------------------*/

//
// Native function table.
//
typedef void (UObject::*Native)( FFrame& TheStack, RESULT_DECL );
extern CORE_API Native GNatives[];
BYTE CORE_API GRegisterNative( INT iNative, const Native& Func );

//
// Registering a native function.
//
#ifdef _MSC_VER
	#define IMPLEMENT_FUNCTION(cls,num,func) \
		extern "C" { DLL_EXPORT Native int##cls##func = (Native)&cls::func; } \
		static BYTE cls##func##Temp = GRegisterNative( num, int##cls##func );
#elif __LINUX_X86__ || __LINUX_ARM__
	#define IMPLEMENT_FUNCTION(cls,num,func) \
		Native int##cls##func __asm__("int"#cls#func) = (Native)&cls::func;  \
		static BYTE cls##func##Temp = GRegisterNative( num, int##cls##func );
#elif MACOSX
	#define IMPLEMENT_FUNCTION(cls,num,func) \
		Native int##cls##func __asm__("_int"#cls#func) = (Native)&cls::func;  \
		static BYTE cls##func##Temp = GRegisterNative( num, int##cls##func );
#else
#error Unsupported OS
#endif

/*-----------------------------------------------------------------------------
	Macros.
-----------------------------------------------------------------------------*/

//
// Macros for grabbing parameters for native functions.
//
#define P_GET_UBOOL(var)              DWORD var=0;                         Stack.Step( Stack.Object, &var    );
#define P_GET_UBOOL_OPTX(var,def)     DWORD var=def;                       Stack.Step( Stack.Object, &var    );
#define P_GET_STRUCT(typ,var)         typ   var;                           Stack.Step( Stack.Object, &var    );
#define P_GET_STRUCT_OPTX(typ,var,def)typ   var=def;                       Stack.Step( Stack.Object, &var    );
#define P_GET_STRUCT_REF(typ,var)     typ   var##T; GPropAddr=0;           Stack.Step( Stack.Object, &var##T ); typ*     var = GPropAddr ? (typ    *)GPropAddr:&var##T;
#define P_GET_INT(var)                INT   var=0;                         Stack.Step( Stack.Object, &var    );
#define P_GET_INT_OPTX(var,def)       INT   var=def;                       Stack.Step( Stack.Object, &var    );
#define P_GET_INT_REF(var)            INT   var##T=0; GPropAddr=0;         Stack.Step( Stack.Object, &var##T ); INT*     var = GPropAddr ? (INT    *)GPropAddr:&var##T;
#define P_GET_FLOAT(var)              FLOAT var=0.f;                       Stack.Step( Stack.Object, &var    );
#define P_GET_FLOAT_OPTX(var,def)     FLOAT var=def;                       Stack.Step( Stack.Object, &var    );
#define P_GET_FLOAT_REF(var)          FLOAT var##T=0.f; GPropAddr=0;       Stack.Step( Stack.Object, &var##T ); FLOAT*   var = GPropAddr ? (FLOAT  *)GPropAddr:&var##T;
#define P_GET_BYTE(var)               BYTE  var=0;                         Stack.Step( Stack.Object, &var    );
#define P_GET_BYTE_OPTX(var,def)      BYTE  var=def;                       Stack.Step( Stack.Object, &var    );
#define P_GET_BYTE_REF(var)           BYTE  var##T=0; GPropAddr=0;         Stack.Step( Stack.Object, &var##T ); BYTE*    var = GPropAddr ? (BYTE   *)GPropAddr:&var##T;
#define P_GET_NAME(var)               FName var=NAME_None;                 Stack.Step( Stack.Object, &var    );
#define P_GET_NAME_OPTX(var,def)      FName var=def;                       Stack.Step( Stack.Object, &var    );
#define P_GET_NAME_REF(var)           FName var##T=NAME_None; GPropAddr=0; Stack.Step( Stack.Object, &var##T ); FName*   var = GPropAddr ? (FName  *)GPropAddr:&var##T;
#define P_GET_STR(var)                FString var;                         Stack.Step( Stack.Object, &var    );
#define P_GET_STR_OPTX(var,def)       FString var(def);                    Stack.Step( Stack.Object, &var    );
#define P_GET_STR_REF(var)            FString var##T; GPropAddr=0;         Stack.Step( Stack.Object, &var##T ); FString* var = GPropAddr ? (FString*)GPropAddr:&var##T;
#define P_GET_OBJECT(cls,var)         cls*  var=NULL;                      Stack.Step( Stack.Object, &var    );
#define P_GET_OBJECT_OPTX(cls,var,def)cls*  var=def;                       Stack.Step( Stack.Object, &var    );
#define P_GET_OBJECT_REF(cls,var)     cls*  var##T=NULL; GPropAddr=0;      Stack.Step( Stack.Object, &var##T ); cls**    var = GPropAddr ? (cls   **)GPropAddr:&var##T;
#define P_GET_ARRAY_REF(typ,var)      typ   var##T[256]; GPropAddr=0;      Stack.Step( Stack.Object,  var##T ); typ*     var = GPropAddr ? (typ    *)GPropAddr: var##T;
#if __EMSCRIPTEN__
#define P_GET_SKIP_OFFSET(var)        _WORD var; {checkSlow(*Stack.Code==EX_Skip); Stack.Code++; appMemcpy(&var, Stack.Code, sizeof(_WORD)); Stack.Code+=2; }
#else
#define P_GET_SKIP_OFFSET(var)        _WORD var; {checkSlow(*Stack.Code==EX_Skip); Stack.Code++; var=*(_WORD*)Stack.Code; Stack.Code+=2; }
#endif
#define P_FINISH                      Stack.Code++;

//
// Convenience macros.
//
#define P_GET_VECTOR(var)           P_GET_STRUCT(FVector,var)
#define P_GET_VECTOR_OPTX(var,def)  P_GET_STRUCT_OPTX(FVector,var,def)
#define P_GET_VECTOR_REF(var)       P_GET_STRUCT_REF(FVector,var)
#define P_GET_ROTATOR(var)          P_GET_STRUCT(FRotator,var)
#define P_GET_ROTATOR_OPTX(var,def) P_GET_STRUCT_OPTX(FRotator,var,def)
#define P_GET_ROTATOR_REF(var)      P_GET_STRUCT_REF(FRotator,var)
#define P_GET_ACTOR(var)            P_GET_OBJECT(AActor,var)
#define P_GET_ACTOR_OPTX(var,def)   P_GET_OBJECT_OPTX(AActor,var,def)
#define P_GET_ACTOR_REF(var)        P_GET_OBJECT_REF(AActor,var)

//
// Iterator macros.
//
#define PRE_ITERATOR \
	INT wEndOffset = Stack.ReadWord(); \
	BYTE B=0; \
	alignas(VECTOR_ALIGNMENT) BYTE Buffer[MAX_CONST_SIZE]; \
	BYTE *StartCode = Stack.Code; \
	do {
#define POST_ITERATOR \
		while( (B=*Stack.Code)!=EX_IteratorPop && B!=EX_IteratorNext ) \
			Stack.Step( Stack.Object, Buffer ); \
		if( *Stack.Code++==EX_IteratorNext ) \
			Stack.Code = StartCode; \
	} while( B != EX_IteratorPop );

// stijn: Added these to safely iterate over uscript linked lists, bailing when we see an item for the second time
#define SAFE_USCRIPT_FOREACH_START(Type, VarName, StartPtr, Next)		\
	{																	\
		TArray<Type> Seen;												\
		for (Type VarName = StartPtr; VarName; VarName = VarName->Next) \
		{																\
			if (Seen.FindItemIndex(VarName) != INDEX_NONE)				\
				break;													\
			Seen.AddItem(VarName);


#define SAFE_USCRIPT_FOREACH_END										\
		}																\
	}

/*-----------------------------------------------------------------------------
	FFrame implementation.
-----------------------------------------------------------------------------*/

inline FFrame::FFrame( UObject* InObject )
:	Node		(InObject ? InObject->GetClass() : NULL)
,	Object		(InObject)
,	Code		(NULL)
,	Locals		(NULL)
{}
inline FFrame::FFrame( UObject* InObject, UStruct* InNode, INT CodeOffset, void* InLocals )
:	Node		(InNode)
,	Object		(InObject)
,	Locals		((BYTE*)InLocals)
{
	// stijn: workaround for an annoying VC++ linkage bug
	TArray<char>* Tmp = reinterpret_cast<TArray<char>*>(&InNode->Script);
	Code = reinterpret_cast<BYTE*>(&(*Tmp)(CodeOffset));
}
FORCEINLINE void FFrame::Step( UObject* Context, RESULT_DECL )
{
	guardSlow(FFrame::Step);
	INT B = *Code++;
	(Context->*GNatives[B])( *this, Result );
	unguardfSlow(( TEXT("(%s @ %s : %04X)"), *FObjectFullName(Object), *FObjectFullName(Node), Code - &Node->Script(0) ));
}
inline INT FFrame::ReadInt()
{
	INT Result;
	#if __EMSCRIPTEN__
	appMemcpy(&Result, Code, sizeof(INT));
	#else
	Result = *(INT*)Code;
	#endif
	Code += sizeof(INT);
	return Result;
}
inline UObject* FFrame::ReadObject()
{
	UObject* Result;
#if BUILD_64
	INT ObjectIndex = *(INT*)Code;
	Result = UObject::GObjObjects(ObjectIndex);
#elif __EMSCRIPTEN__
	appMemcpy(&Result, Code, sizeof(INT));
#else
	Result = *(UObject**)Code;
#endif
	Code += sizeof(INT);	
	return Result;
}
inline FLOAT FFrame::ReadFloat()
{
	FLOAT Result;
	#if __EMSCRIPTEN__
	appMemcpy(&Result, Code, sizeof(FLOAT));
	#else
	Result = *(FLOAT*)Code;
	#endif
	Code += sizeof(FLOAT);
	return Result;
}
inline INT FFrame::ReadWord()
{
	INT Result;
	#if  __EMSCRIPTEN__
	_WORD Temporary;
	appMemcpy(&Temporary, Code, sizeof(_WORD));
	Result = Temporary;
	#else
	Result = *(_WORD*)Code;
	#endif
	Code += sizeof(_WORD);
	return Result;
}
inline FName FFrame::ReadName()
{
	FName Result;
	#if __EMSCRIPTEN__
	appMemcpy(&Result, Code, sizeof(FName));
	#else
	Result = *(FName*)Code;
	#endif
	Code += sizeof(FName);
	return Result;
}

/*-----------------------------------------------------------------------------
	Infinite loop detection.
-----------------------------------------------------------------------------*/

#define RUNAWAY_LIMIT 10000000
#define RECURSE_LIMIT 250

CORE_API extern INT GRunawayCount;
CORE_API extern INT GRecurseCount;

#if DO_GUARD
	#define CHECK_RUNAWAY(S) {if( ++GRunawayCount > RUNAWAY_LIMIT ) \
	{ \
		if (ParseParam(appCmdLine(),TEXT("norunaway"))) \
			GRunawayCount = 0; \
		else if (GRunawayCount < RUNAWAY_LIMIT + 1000) \
			(S)->Logf( NAME_ScriptWarning, TEXT("Runaway counter reached %d"), GRunawayCount ); \
		else \
		{ \
			(S)->Logf( NAME_Critical, TEXT("Runaway loop detected (over %i iterations)"), RUNAWAY_LIMIT ); \
			GRunawayCount = 0; \
		} \
	} \
}
#else
	#define CHECK_RUNAWAY
#endif

CORE_API void GInitRunaway();

/*-----------------------------------------------------------------------------
	FStateFrame implementation.
-----------------------------------------------------------------------------*/

inline FStateFrame::FStateFrame( UObject* InObject )
:	FFrame				( InObject )
,	CurrentFrame		( NULL )
,	StateNode			( InObject->GetClass() )
,	ProbeMask			( ~(QWORD)0 )
{}
#if OLDUNREAL_BINARY_COMPAT
inline const TCHAR* FStateFrame::Describe()
{
	return Node ? Node->GetFullName() : TEXT("None");
}
#endif
inline FString FStateFrame::DescribeSafe()
{
	return FObjectFullName(Node);
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
