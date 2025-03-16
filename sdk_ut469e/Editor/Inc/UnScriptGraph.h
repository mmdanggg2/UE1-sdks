/*=============================================================================
	UnScriptGraph.h. UnrealScript Decompiler.
	Copyright 2016-2020 Sebastian Kaufel. All Rights Reserved.

	Revision history:
	 * Created by Sebastian Kaufel as part of UScriptRaysCommandlet.cpp
	 * Refactored for support for GLSL cross decompilation.
=============================================================================*/

/*-----------------------------------------------------------------------------
	Configuration and Macros.
-----------------------------------------------------------------------------*/

// Suppresses most development output and voluntary crashes.
#define SCRIPT_RAYS_SHIPPING 0

// Used for error logging if a resolve during digestion failed.
#if SCRIPT_RAYS_SHIPPING || 0
	#define GDigestResolveErrorDefault GWarn
#else
	#define GDigestResolveErrorDefault GError
#endif

// Used for higher level output during regain.
//
// Notes
//  * regainf(fmt,...) warnf(fmt,__VA_ARGS__) in earlier _MSC_VER and earlier gcc #define regainf(fmt, a...) warnf(fmt, ##a)
//  * _MSC_VER>=1900 most likely isn't the correct version for the check, it's just used mask out vc6 in practice.
//
#if SCRIPT_RAYS_SHIPPING || 1
	#if !_MSC_VER || _MSC_VER>=1900
    #define regainf(...)
	#else
		#define regainf
	#endif
#else
	#if !_MSC_VER || _MSC_VER>=1900
		#define regainf(fmt,...) warnf( fmt, ##__VA_ARGS__ )
	#else
		#define regainf warnf
	#endif
#endif

// Used for sanity checks during regain.
#if SCRIPT_RAYS_SHIPPING || 1
	#define checkRegain(cond) \
		if ( !(cond) ) \
		{ \
			warnf( TEXT("Regain sanity check failed: (") TEXT(#cond) TEXT(") REPORT THIS AS A BUG.")); \
			return 0; \
		}
#else
	#define checkRegain(cond) check(cond)
#endif

// Verbose output for building if/else if/else blocks.
#if SCRIPT_RAYS_SHIPPING || 1
	#if !_MSC_VER || _MSC_VER>=1900
		#define matchiff(...)
	#else
		#define matchiff
	#endif
#else
	#if !_MSC_VER || _MSC_VER>=1900
		#define matchiff(fmt, ...) warnf( fmt, ##__VA_ARGS__ )
	#else
		#define matchiff warnf
	#endif
#endif


// Verbose output for finding while loops which should be transformed into for loops.
#if SCRIPT_RAYS_SHIPPING || 1
	#if !_MSC_VER || _MSC_VER>=1900
		#define matchforf(...)
	#else
		#define matchforf
	#endif
#else
	#if !_MSC_VER || _MSC_VER>=1900
		#define matchforf(fmt,...) warnf( fmt, ##__VA_ARGS__ )
	#else
		#define matchforf warnf
	#endif
#endif

/*-----------------------------------------------------------------------------
	Forward Declarations.
-----------------------------------------------------------------------------*/

// Decompilation Information.
struct FIntrospectionInfoBase;
struct   FFieldIntrospectionInfo;
struct     FConstIntrospectionInfo;
struct     FEnumIntrospectionInfo;
struct     FPropertyIntrospectionInfo;
struct     FStructIntrospectionInfo;
struct       FFunctionIntrospectionInfo;
struct       FStateIntrospectionInfo;
struct         FClassIntrospectionInfo;
struct   FReplicationIntrospectionInfo;

// Hacks.
struct FScriptOutputConfiguration;

// Script Graph.
struct FScriptGraph;
struct FScriptGraphNode;

/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

// Shitty hack for ScriptRays configuration.
extern FScriptOutputConfiguration* GScriptOutputConfiguration;
EDITOR_API extern FOutputDevice* GDigestResolveError;

/*-----------------------------------------------------------------------------
	FScriptOutputConfiguration.
-----------------------------------------------------------------------------*/

struct FScriptOutputConfiguration
{
	// Configuration.
	UBOOL EliminateImplicitCasts;
	UBOOL EliminateParentheses;
	UBOOL EliminateBraces;
	UBOOL GroupInstanceVariables;
	UBOOL GroupStructMembers;
	UBOOL GroupLocalVariables;

	// Constructors.
	FScriptOutputConfiguration()
	: EliminateImplicitCasts(1)
	, EliminateParentheses(1)
	, EliminateBraces(0)
	, GroupInstanceVariables(0)
	, GroupStructMembers(0)
	, GroupLocalVariables(1)
	{}
};

/*-----------------------------------------------------------------------------
	FIntrospectionInfoBase.
-----------------------------------------------------------------------------*/

// Some base class. Currently only holds output helper.
struct FIntrospectionInfoBase
{
	// FIntrospectionInfoBase interface.
	virtual void Introspect()  {} // Reads in data and fills internal structures.
	virtual void Disassemble() {} // Creates a disassembly text.
	virtual void Decompile()   {} // Rebuilds supported set of high level control structures.
	virtual void ExportText( FOutputDevice& Ar, INT Indent ) {}

	// Checks whether a class has UnrealScript code.
	static UBOOL EDITOR_API IsScriptClass( UClass* Class );

	// Writes NumTabs to Ar.
	void Tab( FOutputDevice& Ar, INT Num=1 )
	{
		guard(FFieldIntrospectionInfo::WriteTabs);
		for ( INT i=0; i<Num; i++ )
			Ar.Log( TEXT("\t") );
		unguard;
	}

	// Writes Line Ending.
	void LineTerminator( FOutputDevice& Ar, INT Num=1 )
	{
		guard(FFieldIntrospectionInfo::NewLine);
		for ( INT i=0; i<Num; i++ )
			Ar.Log( LINE_TERMINATOR );
		unguard;
	}

	// Puts out a commend.
	void Comment( FOutputDevice& Ar, const TCHAR* Comment, INT Indent=0 )
	{
		guard(UScriptRaysCommandlet::Comment);
		Tab( Ar, Indent );
		if ( Comment && *Comment )
			Ar.Logf( TEXT("// %s") LINE_TERMINATOR, Comment );
		else
			Ar.Logf( TEXT("//") LINE_TERMINATOR );
		unguard;
	}

	// Puts out a comment header.
	void SectionComment( FOutputDevice& Ar, const TCHAR* Comment, UBOOL Bold=0 )
	{
		guard(UScriptRaysCommandlet::CommentHeader);
		if ( Bold )
			Ar.Logf( LINE_TERMINATOR TEXT("//=============================================================================") );
		else
			Ar.Logf( LINE_TERMINATOR TEXT("//-----------------------------------------------------------------------------") );
		if ( Comment )
		{
			Ar.Logf( LINE_TERMINATOR TEXT("// %s"), Comment );
		}
		if ( Bold )
			Ar.Logf( LINE_TERMINATOR TEXT("//=============================================================================") );
		else
			Ar.Logf( LINE_TERMINATOR TEXT("//-----------------------------------------------------------------------------") );
		Ar.Logf( LINE_TERMINATOR );
		unguard;
	}

	void Nada( FOutputDevice& Ar, INT NumTabs=0 )
	{
		guard(UScriptRaysCommandlet::Nada);
		Tab( Ar, NumTabs );
		Ar.Logf( TEXT("#error nada") LINE_TERMINATOR );
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	FFieldIntrospectionInfo.
-----------------------------------------------------------------------------*/

// UField Decompilation Info.
struct FFieldIntrospectionInfo : public FIntrospectionInfoBase
{
	// Variables.
	UField* Field;
	INT FieldIndex; // Index into child list of an UStruct.

	// Constructors.
	FFieldIntrospectionInfo( UField* InField, INT InFieldIndex=INDEX_NONE )
	: Field(InField)
	, FieldIndex(InFieldIndex)
	{}

	// FFieldIntrospectionInfo interface.
	UField* GetField()
	{
		return Field;
	}
	UBOOL IsPublic()
	{
		return (GetField()->GetFlags()&RF_Public)==RF_Public;
	}
	// !! Should probably add more wrapper around RF flags here. --han
};

// First I was like: I want to use Super
// Then I was like: (well, time will eventually tell).
// Now I like: That's nice. ^_^
#define DECLARE_FIELD_DECOMPILATION_INFO( TClass, TSuperClass, TType ) \
	typedef TSuperClass Super;\
	typedef TClass ThisClass; \
	typedef U##TType ThisType; \
	U##TType* Get##TType() \
	{ \
		return (U##TType*)Field; \
	}

/*-----------------------------------------------------------------------------
	FConstIntrospectionInfo.
-----------------------------------------------------------------------------*/

// UConst Decompilation Info.
struct FConstIntrospectionInfo : public FFieldIntrospectionInfo
{
	DECLARE_FIELD_DECOMPILATION_INFO(FConstIntrospectionInfo,FFieldIntrospectionInfo,Const);

	// Constructors.
	FConstIntrospectionInfo( UConst* InConst, INT InFieldIndex=INDEX_NONE )
	: FFieldIntrospectionInfo( InConst, InFieldIndex )
	{}

	// FIntrospectionInfoBase interface.
	void ExportText( FOutputDevice& Ar, INT Indent );
};

/*-----------------------------------------------------------------------------
	FEnumIntrospectionInfo.
-----------------------------------------------------------------------------*/

// UEnum Decompilation Info.
struct FEnumIntrospectionInfo : public FFieldIntrospectionInfo
{
	DECLARE_FIELD_DECOMPILATION_INFO(FEnumIntrospectionInfo,FFieldIntrospectionInfo,Enum);

	// Constructors.
	FEnumIntrospectionInfo( UEnum* InEnum, INT InFieldIndex=INDEX_NONE )
	: FFieldIntrospectionInfo( InEnum, InFieldIndex )
	{}

	// FIntrospectionInfoBase interface.
	void ExportText( FOutputDevice& Ar, INT Indent );
};

/*-----------------------------------------------------------------------------
	FPropertyIntrospectionInfo.
-----------------------------------------------------------------------------*/

// UProperty Decompilation Info.
struct FPropertyIntrospectionInfo : public FFieldIntrospectionInfo
{
	DECLARE_FIELD_DECOMPILATION_INFO(FPropertyIntrospectionInfo,FFieldIntrospectionInfo,Property);

	// Variables.
	TArray<FPropertyIntrospectionInfo> InnerProperties; // Used for UArrayProperty::Inner, UMapProperty::Key/Value, UFixedArrayProperty::Inner (which shouldn't show up though).
	EPropertyType Type;
	FString       TypeString;           // Human readable type declaration.
	UStruct*      StructType;           // Struct if a struct property.
	UEnum*        EnumType;             // Enum if an enum property.
	FString       ArrayString;          // Static array declraration to be put after variable name. Can be empty.
	FString       PropertyCommentText;  // Comment to show up after Property.

	// Constructors.
	FPropertyIntrospectionInfo( UProperty* InProperty, INT InFieldIndex=INDEX_NONE );

	// FIntrospectionInfoBase interface.
	void Introspect();
	void ExportText( FOutputDevice& Ar, INT Indent );

	// FPropertyIntrospectionInfo interface.
	FString ModifierString();
	UBOOL CanBeGroupedWith( FPropertyIntrospectionInfo& Other );
	UBOOL IsEdit()
	{
		return (GetProperty()->PropertyFlags&CPF_Edit)==CPF_Edit;
	}
	UBOOL IsNative()
	{
		return (GetProperty()->PropertyFlags&CPF_Native)==CPF_Native;
	}
	UBOOL IsLocalized()
	{
		return (GetProperty()->PropertyFlags&CPF_Localized)==CPF_Localized;
	}
	UBOOL IsTransient()
	{
		return (GetProperty()->PropertyFlags&CPF_Transient)==CPF_Transient;
	}
	UBOOL IsInput()
	{
		return (GetProperty()->PropertyFlags&CPF_Input)==CPF_Input;
	}
	UBOOL IsTravel()
	{
		return (GetProperty()->PropertyFlags&CPF_Travel)==CPF_Travel;
	}
	UBOOL IsConst()
	{
		return (GetProperty()->PropertyFlags&CPF_Const)==CPF_Const;
	}
	UBOOL IsEditConst()
	{
		return (GetProperty()->PropertyFlags&CPF_EditConst)==CPF_EditConst;
	}
	UBOOL IsExportObject()
	{
		return (GetProperty()->PropertyFlags&CPF_ExportObject)==CPF_ExportObject;
	}
	UBOOL IsConfig()
	{
		return (GetProperty()->PropertyFlags&CPF_Config)==CPF_Config;
	}
	UBOOL IsGlobalConfig() // globalconfig specifier always implies CPF_Config.
	{
		return (GetProperty()->PropertyFlags&CPF_GlobalConfig)==CPF_GlobalConfig;
	}
	UBOOL IsOptionalParm()
	{
		return (GetProperty()->PropertyFlags&CPF_OptionalParm)==CPF_OptionalParm;
	}
	UBOOL IsCoerceParm()
	{
		return (GetProperty()->PropertyFlags&CPF_CoerceParm)==CPF_CoerceParm;
	}
	UBOOL IsOutParm()
	{
		return (GetProperty()->PropertyFlags&CPF_OutParm)==CPF_OutParm;
	}
	UBOOL IsSkipParm()
	{
		return (GetProperty()->PropertyFlags&CPF_SkipParm)==CPF_SkipParm;
	}
	UBOOL IsNet()
	{
		return (GetProperty()->PropertyFlags&CPF_Net)==CPF_Net;
	}
#if LAUNCH_BAG
	UBOOL IsVisible()
	{
		return (GetProperty()->PropertyFlags&CPF_Visible)==CPF_Visible;
	}
	UBOOL IsAudible()
	{
		return (GetProperty()->PropertyFlags&CPF_Net)==CPF_Audible;
	}
#else
	UBOOL IsVisible()
	{
		return 0;
	}
	UBOOL IsAudible()
	{
		return 0;
	}
#endif
};

/*-----------------------------------------------------------------------------
	FStructIntrospectionInfo.
-----------------------------------------------------------------------------*/

// UStruct Decompilation Info.
struct EDITOR_API FStructIntrospectionInfo : public FFieldIntrospectionInfo
{
	DECLARE_FIELD_DECOMPILATION_INFO(FStructIntrospectionInfo,FFieldIntrospectionInfo,Struct);

	// Variables.
	TArray<FConstIntrospectionInfo>    Constants;
	TArray<FEnumIntrospectionInfo>     Enumerations;
	TArray<FStructIntrospectionInfo>   Structures;
	TArray<FPropertyIntrospectionInfo> Properties;
	TArray<UStruct*>                   StructDependencies; // Structs this Struct depends on. Required for sorting.

	// Constructors.
	FStructIntrospectionInfo( UStruct* InStruct, INT InFieldIndex=INDEX_NONE );

	// FIntrospectionInfoBase interface.
	void Introspect();
	void Disassemble();
	void Decompile();
	void ExportText( FOutputDevice& Ar, INT Indent );
};

/*-----------------------------------------------------------------------------
	FFunctionIntrospectionInfo.
-----------------------------------------------------------------------------*/

// UFunction Decompilation Info.
struct EDITOR_API FFunctionIntrospectionInfo : public FStructIntrospectionInfo
{
	DECLARE_FIELD_DECOMPILATION_INFO(FFunctionIntrospectionInfo,FStructIntrospectionInfo,Function);

	// Variables.
	TArray<FPropertyIntrospectionInfo> LocalProperties;     // Local variables.
	TArray<FPropertyIntrospectionInfo> Parameters;          // Function Parameters, optionally including return parameter.
	INT                                ReturnIndex;         // Optional index into Parameters for ReturnParam. Otherwise INDEX_NONE.
	FString                            FunctionAnnotation;  // Comment to show up above or right of function declaration.
	UBOOL                              FunctionDigested;    // Sucessfully digested bytecode of function.
	UBOOL                              FunctionDecompiled;  // Sucessfully decompiled bytecode of function.
	TArray<FString>                    FunctionDisassembly; // Disassembly produced by digest.
	TArray<FString>                    FunctionScriptCode;  // Decompiled ScriptCode for function.

	// Constructors.
	FFunctionIntrospectionInfo( UFunction* InFunction, INT InFieldIndex=INDEX_NONE );

	// FIntrospectionInfoBase interface.
	void Introspect();
	void Disassemble();
	void Decompile();
	void ExportText( FOutputDevice& Ar, INT Indent );

	// FFunctionIntrospectionInfo interface.
	FString FunctionKeyword();
	FString FunctionModifiers();
	FPropertyIntrospectionInfo* ReturnValueInfo()
	{
		guard(FFunctionIntrospectionInfo::ReturnValueInfo);
		if ( ReturnIndex==INDEX_NONE )
			return NULL_PTR;
		return &Parameters(ReturnIndex);
		unguard;
	}
	INT ParameterCount() // Returns number of actual function parameters, not including the return value.
	{
		return Parameters.Num() - (ReturnIndex!=INDEX_NONE ? 1 : 0);
	}
	UBOOL IsFunction() // Returns true if this is no [pre|post]operator. Returns true for events.
	{
		return (GetFunction()->FunctionFlags&FUNC_Operator)==0;
	}
	UBOOL IsPreOperator()
	{
		return (GetFunction()->FunctionFlags&(FUNC_Operator|FUNC_PreOperator))==(FUNC_Operator|FUNC_PreOperator);
	}
	UBOOL IsOperator() // Only returns true if this is neither a preoperator nor a postoperator.
	{
		return (GetFunction()->FunctionFlags&(FUNC_Operator|FUNC_PreOperator))==FUNC_Operator && GetFunction()->NumParms==3;
	}
	UBOOL IsPostOperator()
	{
		return (GetFunction()->FunctionFlags&(FUNC_Operator|FUNC_PreOperator))==FUNC_Operator && GetFunction()->NumParms==2;
	}
	UBOOL IsFinal()
	{
		return (GetFunction()->FunctionFlags&FUNC_Final)==FUNC_Final;
	}
	UBOOL IsStatic()
	{
		return (GetFunction()->FunctionFlags&FUNC_Static)==FUNC_Static;
	}
	UBOOL IsEvent()
	{
		return (GetFunction()->FunctionFlags&FUNC_Event)==FUNC_Event;
	}
	UBOOL IsNative()
	{
		return (GetFunction()->FunctionFlags&FUNC_Native)==FUNC_Native;
	}
	UBOOL IsExec()
	{
		return (GetFunction()->FunctionFlags&FUNC_Exec)==FUNC_Exec;
	}
	UBOOL IsSimulated()
	{
		return (GetFunction()->FunctionFlags&FUNC_Simulated)==FUNC_Simulated;
	}
	UBOOL IsSingular()
	{
		return (GetFunction()->FunctionFlags&FUNC_Singular)==FUNC_Singular;
	}
	UBOOL IsLatent()
	{
		return (GetFunction()->FunctionFlags&FUNC_Latent)==FUNC_Latent;
	}
	UBOOL IsIterator()
	{
		return (GetFunction()->FunctionFlags&FUNC_Iterator)==FUNC_Iterator;
	}
	UBOOL IsDefined() // Typically it is better to use IsStub() or IsEmpty() instead as obfuscator may remove this flag.
	{
		return (GetFunction()->FunctionFlags&FUNC_Defined)==FUNC_Defined;
	}
	UBOOL IsNet()
	{
		return (GetFunction()->FunctionFlags&FUNC_Net)==FUNC_Net;
	}
	UBOOL IsEmpty()
	{
		return !GetFunction()->Script.Num();
	}
	UBOOL IsStub()
	{
		if ( IsEmpty() )
		{
			return 1;
		}
		else if ( IsNative() )
		{
			return 1;
		}
		else if ( IsDefined() )
		{
			return 0;
		}
		// Return stub.
		else if ( GetFunction()->Script.Num()==2 && GetFunction()->Script(0)==EX_Return && GetFunction()->Script(1)==EX_Nothing )
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
};

/*-----------------------------------------------------------------------------
	FStateIntrospectionInfo.
-----------------------------------------------------------------------------*/

// UState Decompilation Info.
struct EDITOR_API FStateIntrospectionInfo : public FStructIntrospectionInfo
{
	DECLARE_FIELD_DECOMPILATION_INFO(FStateIntrospectionInfo,FStructIntrospectionInfo,State);

	// Variables.
	TArray<FFunctionIntrospectionInfo> Functions;
	TArray<UState*>                    StateDependencies; // States this State depends on. Required for sorting.
	TArray<FName>                      IgnoreNames;       // Names ignored by ProbeMask.
	TArray<FLabelEntry>                LabelTable;
	FString                            StateAnnotation;   // Comment to show up above state code.
	UBOOL                              StateDigested;     // Sucessfully digested bytecode of state.
	UBOOL                              StateDecompiled;   // Sucessfully decompiled bytecode of state.
	TArray<FString>                    StateDisassembly;  // Disassembly produced by digest.
	TArray<FString>                    StateScriptCode;   // Decompiled ScriptCode for state.

	// Constructors.
	FStateIntrospectionInfo( UState* InState, INT InFieldIndex=INDEX_NONE );

	// FIntrospectionInfoBase interface.
	void Introspect();
	void Disassemble();
	void Decompile();
	void ExportText( FOutputDevice& Ar, INT Indent );
};

/*-----------------------------------------------------------------------------
	FClassIntrospectionInfo.
-----------------------------------------------------------------------------*/

// UClass Decompilation Info.
struct EDITOR_API FClassIntrospectionInfo : public FStateIntrospectionInfo
{
	DECLARE_FIELD_DECOMPILATION_INFO(FClassIntrospectionInfo,FStateIntrospectionInfo,Class);

	// Variables.
	TArray<FReplicationIntrospectionInfo> Replications;     // Replication bin.
	TArray<FStateIntrospectionInfo>       States;           // States declared inside the class.
	UBOOL                                 ChildOfTransient; // Whether the class has a transient parent class. Used for culling.

	// Constructors.
	FClassIntrospectionInfo( UClass* InClass );

	// FIntrospectionInfoBase interface.
	void Introspect();
	void Disassemble();
	void Decompile();
	void ExportText( FOutputDevice& Ar, INT Indent, UBOOL WithDefProps = TRUE );
};

/*-----------------------------------------------------------------------------
	FReplicationIntrospectionInfo.
-----------------------------------------------------------------------------*/

struct EDITOR_API FReplicationIntrospectionInfo : public FIntrospectionInfoBase
{
	// Variables;
	FClassIntrospectionInfo* ClassInfo;      // Class this replication statement is part of.
	_WORD                    RepOffset;      // Offset into UClass::Script. First used for sorting.
	UBOOL                    Unreliable;     // Whether a function without FUNC_NetReliable is inside the bin.
	TArray<FName>            RepNames;       // Names of replicated Properties and Functions.
	TArray<BYTE>             RepCode;        // Bytecode of replication statement.
	UBOOL                    RepDigested;    // Sucessfully digested bytecode of statment.
	UBOOL                    RepDecompiled;  // Sucessfully decompiled statement.
	FString                  RepScriptCode;  // Decompiled ScriptCode for expression.
	FString                  RepAnnotation;  // Comment for ScriptCode.
	TArray<FString>          RepDisassembly; // Disassembly produced by digest.

	// Constructors.
	FReplicationIntrospectionInfo( UProperty* Property, FClassIntrospectionInfo* InClassInfo );
	FReplicationIntrospectionInfo( UFunction* Function, FClassIntrospectionInfo* InClassInfo );

	// FDecompilionReplicationInfo interface.
	UBOOL Bin( UProperty* Property );
	UBOOL Bin( UFunction* Function );
	void CopyCode(); // Creates a local copy of the bytecode. MUST BE CALLED AFTER SORTING.

	// FIntrospectionInfoBase interface.
	void Introspect();
	void Disassemble();
	void Decompile();
	void ExportText( FOutputDevice& Ar, INT Indent );
};

/*-----------------------------------------------------------------------------
	FScriptGraph.
-----------------------------------------------------------------------------*/

enum EScriptGraphNodeType
{
	// Raw types.
	SGNT_LocalVariable,      // A local variable.
	SGNT_InstanceVariable,   // An object variable.
	SGNT_DefaultVariable,    // Default variable for a concrete object.
	//                       // Unused.
	SGNT_Return,             // Return from function.
	SGNT_Switch,             // Switch.
	SGNT_Jump,               // Goto a local address in code.
	SGNT_JumpIfNot,          // Goto if not expression.
	SGNT_Stop,               // Stop executing state code.
	SGNT_Assert,             // Assertion.
	SGNT_Case,               // Case.
	SGNT_Nothing,            // No operation.
	SGNT_LabelTable,         // Table of labels.
	SGNT_GotoLabel,          // Goto a label.
	SGNT_EatString,          // Ignore a dynamic string.
	SGNT_Let,                // Assign an arbitrary size value to a variable.
	SGNT_DynArrayElement,    // Dynamic array element.!!
	SGNT_New,                // New object allocation.
	SGNT_ClassContext,       // Class default metaobject context.
	SGNT_MetaCast,           // Metaclass cast.
	SGNT_LetBool,            // Let boolean variable.
	//                       // Unused.
	//SGNT_EndFunctionParms, // Not used as a Node.
	SGNT_Self,               // Self object.
	SGNT_Skip,               // Skippable expression.
	SGNT_Context,            // Call a function or access a property through an object context.
	SGNT_ArrayElement,       // Array element.
	SGNT_VirtualFunction,    // A function call with parameters.
	SGNT_FinalFunction,      // A prebound function call with parameters.
	SGNT_IntConst,           // Int constant.
	SGNT_FloatConst,         // Floating point constant.
	SGNT_StringConst,        // String constant.
	SGNT_ObjectConst,        // An object constant.
	SGNT_NameConst,          // A name constant.
	SGNT_RotationConst,      // A rotation constant.
	SGNT_VectorConst,        // A vector constant.
	SGNT_ByteConst,          // A byte constant.
	SGNT_IntZero,            // Zero.
	SGNT_IntOne,             // One.
	SGNT_True,               // Bool True.
	SGNT_False,              // Bool False.
	SGNT_NativeParm,         // Not used as a Node.
	SGNT_NoObject,           // NoObject.
	//                       // Unused.
	SGNT_IntConstByte,       // Int constant that requires 1 byte.
	SGNT_BoolVariable,       // A bool variable which requires a bitmask.
	SGNT_DynamicCast,        // Safe dynamic class casting.
	SGNT_Iterator,           // Begin an iterator operation.
	SGNT_IteratorPop,        // Pop an iterator level.
	SGNT_IteratorNext,       // Go to next iteration.
	SGNT_StructCmpEq,        // Struct binary compare-for-equal.
	SGNT_StructCmpNe,        // Struct binary compare-for-unequal.
	SGNT_UnicodeStringConst, // Unicode string constant.
	//                       // Unused.
	SGNT_StructMember,       // Struct member.
#if ENGINE_VERSION==433
	SGNT_DynArrayToInt,      // A cast from DynArray to int (KWG).
#else
	//                       // Unused.
#endif
	SGNT_GlobalFunction,     // Call non-state version of a function.
	SGNT_RotatorToVector,    // A cast from Rotator to Vector.
	SGNT_ByteToInt,          // A cast from byte to int.
	SGNT_ByteToBool,         // A cast from byte to bool.
	SGNT_ByteToFloat,        // A cast from byte to float.
	SGNT_IntToByte,          // A cast from int to byte.
	SGNT_IntToBool,          // A cast from int to bool.
	SGNT_IntToFloat,         // A cast from int to float.
	SGNT_BoolToByte,         // A cast from bool to byte.
	SGNT_BoolToInt,          // A cast from bool to int.
	SGNT_BoolToFloat,        // A cast from bool to float.
	SGNT_FloatToByte,        // A cast from float to byte.
	SGNT_FloatToInt,         // A cast from float to int.
	SGNT_FloatToBool,        // A cast from float to bool.
#if ENGINE_VERSION==005 || ENGINE_VERSION==107
	SGNT_StringToName,       // A cast from string to name (Rune).
#else
	//                       // Unused.
#endif
	SGNT_ObjectToBool,       // A cast from Object to bool.
	SGNT_NameToBool,         // A cast from name to bool.
	SGNT_StringToByte,       // A cast from string to byte.
	SGNT_StringToInt,        // A cast from string to int.
	SGNT_StringToBool,       // A cast from string to bool.
	SGNT_StringToFloat,      // A cast from string to float.
	SGNT_StringToVector,     // A cast from string to Vector.
	SGNT_StringToRotator,    // A cast from string to Rotator.
	SGNT_VectorToBool,       // A cast from Vector to bool.
	SGNT_VectorToRotator,    // A cast from Vector to Rotator.
	SGNT_RotatorToBool,      // A cast from Rotator to bool.
	SGNT_ByteToString,       // A cast from byte to string.
	SGNT_IntToString,        // A cast from int to string.
	SGNT_BoolToString,       // A cast from bool to string.
	SGNT_FloatToString,      // A cast from float to string.
	SGNT_ObjectToString,     // A cast from Object to string.
	SGNT_NameToString,       // A cast from name to string.
	SGNT_VectorToString,     // A cast from Vector to string.
	SGNT_RotatorToString,    // A cast from Rotator to string.
#if ENGINE_VERSION==433
	SGNT_StringToName,       // A cast from string to name (KWG).
#else
	//                       // Unused.
#endif
	//                       // Unused.
	//                       // Unused.
	//                       // Unused.
	//                       // Unused.
	//                       // Unused.
	SGNT_NativeFunction,     // A native function call with parameters.

	//
	// Additional types.
	//

	// Trailings.
	SGNT_TrailingReturn,     // Marker for autogenerated return at function end.
	SGNT_TrailingStop,       // Marker for autogenerated stop at state end.

	// In place Label for state code.
	SGNT_Label,

	// While loop related.
	SGNT_WhileLoop,
	SGNT_WhileJumpBack,
	SGNT_WhileBreakJump,
	SGNT_WhileContinueJump,

	// While loop related.
	SGNT_ForLoop,
	SGNT_ForJumpBack,
	SGNT_ForBreakJump,
	SGNT_ForContinueJump,

	// Do-until loop related.
	SGNT_DoUntilLoop,
	SGNT_DoUntilBreakJump,
	SGNT_DoUntilContinueJump,

	// While-until loop related.
	SGNT_WhileUntilLoop,
	SGNT_WhileUntilJumpIfNotBack,
	SGNT_WhileUntilBreakJump,
	SGNT_WhileUntilContinueJump,

	// Iterator related.
	SGNT_IteratorLoop,
	SGNT_IteratorEndNext,
	SGNT_IteratorEndPop,
	SGNT_IteratorReturnPop,
	SGNT_IteratorBreakJump,
	SGNT_IteratorContinueNext,
	SGNT_IteratorContinueJump,

	// Switch related.
	SGNT_SwitchBlock,
	SGNT_SwitchBreakJump,

	// If related.
	SGNT_IfBlock,
	SGNT_ElseIfBlock,
	SGNT_ElseBlock,
	SGNT_IfEndJump,

	SGNT_MAX,
};

enum EScriptGraphOperatorType
{
	SGOT_NoOperator,
	SGOT_Operator,
	SGOT_PreOperator,
	SGOT_PostOperator,
};

enum EScriptGraphOperandFlags
{
	SGOF_None,
	SGOF_LeftOperand,
	SGOF_LeftToRightAssociative,
};

// A Node.
struct FScriptGraphNode
{
	// Variables.
	EScriptGraphNodeType Type;      // Type of node.
	FScriptGraph*        Graph;     // Graph this node belongs to.
	FScriptGraphNode*    Next;      // Next in execution.
	FScriptGraphNode*    Top;       // Nesting, set if inside loop, etc.
	FScriptGraphNode*    In;        // Used as input for Cast, Skip, etc.
	FScriptGraphNode*    InContext; // Used for context input for EX_Context to be used for In.
	FScriptGraphNode*    InIndex;   // Used as index input for EX_ArrayElement and EX_DynArrayElement.
	FScriptGraphNode*    InArray;   // Used as property input for EX_ArrayElement and EX_DynArrayElement.
	FScriptGraphNode*    InOuter;   // Used as Outer input for EX_New.
	FScriptGraphNode*    InName;    // Used as Name input for EX_New.
	FScriptGraphNode*    InFlags;   // Used as Flags input for EX_New.
	FScriptGraphNode*    InClass;   // Used as Class input for EX_New.
	FScriptGraphNode*    InLeft;    // Used as left expression for EX_Let/EX_LetBool/EX_StructCmpEq/EX_StructCmpNe.
	FScriptGraphNode*    InRight;   // Used as right expression for EX_Let/EX_LetBool/EX_StructCmpEq/EX_StructCmpNe.
	UState*              Context;   // Context of this node.
	INT                  Offset;    // Offset into Script.

	// Constant values.
	INT        IntConst;
	FLOAT      FloatConst;
	FString    StringConst;
	UObject*   ObjectConst;
	FName      NameConst;
	FRotator   RotationConst;
	FVector    VectorConst;
	BYTE       ByteConst;

	// Function related info.
	UFunction*                  Function;
	FName                       FunctionName;
	FFunctionIntrospectionInfo* FunctionInfo;
	TArray<FScriptGraphNode*>   ParmNodes;
	INT                         iNative;
	EScriptGraphOperatorType    OperatorType;

	// Property related info.
	UProperty* Property;
	FPropertyIntrospectionInfo* PropertyInfo;

	// Cast related info.
	FString CastTypeName;
	UBOOL   CastImplicit;

	// Table for EX_LabelTable.
	TArray<FLabelEntry> LabelTable;

	// Misc.
	UClass*  Class;          // For EX_MetaCast/EX_DynamicCast.
	UStruct* Struct;         // For EX_StructCmpEq/EX_StructCmpNe.
	UEnum*   Enum;           // For byte properties.
	_WORD    SkipSize;
	_WORD    AssertLine;
	_WORD    JumpOffset;     // For EX_Jump/EX_JumpIfNot.
	_WORD    IteratorOffset; // For EX_Iterator.
	_WORD    NextOffset;     // For EX_Case.
	BYTE     MemSize;
	FName    LabelName;      // For SGNT_Label.
	UBOOL    RawLabel;       // For destination of raw SGNT_Jump

	// Information for break/continue linking.
	DWORD    BreakOffset;       // Use DWORD to avoid using 0 and MAXWORD.
	DWORD    ContinueOffset;    // In case of while loop.
	DWORD    ContinueOffsetFor; // In case of for loop.

	TArray<FScriptGraphNode*> Cases; // Case entry nodes for SGNT_SwitchBlock.

	FScriptGraphNode* JumpNode; // For EX_Jump/EX_JumpIfNot after building JumpRefs.
	FScriptGraphNode* Inner;    // Inner of loops.

	// Jumps refering to this Node.
	TArray<FScriptGraphNode*> JumpFwdRefs;       // Forward EX_Jump referencing this Node.
	TArray<FScriptGraphNode*> JumpBackRefs;      // Backward EX_Jump referencing this Node.
	TArray<FScriptGraphNode*> JumpIfNotFwdRefs;  // Forward EX_JumpIfNot referencing this Node.
	TArray<FScriptGraphNode*> JumpIfNotBackRefs; // Backward EX_JumpIfNot referencing this Node.

	// While/for loop specific.
	FScriptGraphNode* ForInitNode;
	FScriptGraphNode* ForIterateNode;
	UBOOL CanBeWhileLoop;
	UBOOL CanBeForLoop;

	// Loop specific.
	FScriptGraphNode* LoopJumpBack; // Used for the jumpback node.

	// Constructors.
	FScriptGraphNode( FScriptGraph* InGraph, EScriptGraphNodeType InType, UState* NodeContext, INT InOffset )
	: Type(InType)
	, Graph(InGraph)
	, Next(NULL_PTR)
	, Top(NULL_PTR)
	, In(NULL_PTR)
	, InContext(NULL_PTR)
	, InIndex(NULL_PTR)
	, InArray(NULL_PTR)
	, InOuter(NULL_PTR)
	, InName(NULL_PTR)
	, InFlags(NULL_PTR)
	, InClass(NULL_PTR)
	, InLeft(NULL_PTR)
	, InRight(NULL_PTR)
	, Offset(InOffset)
	, Context(NodeContext)
	, IntConst(-1)
	, FloatConst(-1.f)
	, ObjectConst(NULL_PTR)
	, NameConst(TEXT("Invalid"))
	, RotationConst(FRotator(-1,-1,-1))
	, VectorConst(FVector(-1.f,-1.f,-1.f))
	, ByteConst(255)
	, Function(NULL_PTR)
	, FunctionName(TEXT("Invalid"))
	, FunctionInfo(NULL_PTR)
	, iNative(INDEX_NONE)
	, OperatorType(SGOT_NoOperator)
	, Property(NULL_PTR)
	, PropertyInfo(NULL_PTR)
	, CastTypeName(TEXT("Invalid"))
	, CastImplicit(0)
	, Class(NULL_PTR)
	, Struct(NULL_PTR)
	, Enum(NULL_PTR)
	, SkipSize(0)
	, AssertLine(0)
	, JumpOffset(MAXWORD)
	, IteratorOffset(0)
	, MemSize(0)
	, LabelName(NAME_None)
	, RawLabel(FALSE)
	, BreakOffset(~(DWORD)0)
	, ContinueOffset(~(DWORD)0)
	, ContinueOffsetFor(~(DWORD)0)
	, JumpNode(NULL_PTR)
	, Inner(NULL_PTR)
	, ForInitNode(NULL_PTR)
	, ForIterateNode(NULL_PTR)
	, CanBeWhileLoop(0)
	, CanBeForLoop(0)
	, LoopJumpBack(NULL_PTR)
	{
		guard(FScriptGraphNode::FScriptGraphNode);
		unguard;
	}
	virtual ~FScriptGraphNode()
	{
		if ( FunctionInfo )
			delete FunctionInfo;
		if ( PropertyInfo )
			delete FunctionInfo;
	}
};

enum ERegainPass
{
	PASS_Loop,
	PASS_If,
	PASS_Switch,
	PASS_Continue,
	PASS_For,
	PASS_Jump,
	PASS_Check,
	PASS_MAX_,
};

// A Graph.
struct FScriptGraph
{
	// Variables.
	TArray<FScriptGraphNode*>   Nodes;        // Nodes in graph.
	FScriptGraphNode*           RootNode;     // Root node.
	UState*                     RootContext;  // State or Class Context.
	FFunctionIntrospectionInfo* FunctionInfo; // FunctionInfo if decompiling a function.
	TArray<FString>*            DigestLog;    // Digest Log.

	// Constructors.
	FScriptGraph();
	~FScriptGraph();

	// FScriptGraph interface.
	FScriptGraphNode* AddNode( EScriptGraphNodeType NodeType, UState* NodeContext, INT NodeOffset )
	{
		guard(FScriptGraph::AddNode);
		FScriptGraphNode* NewNode = new FScriptGraphNode( this, NodeType, NodeContext, NodeOffset );
		Nodes.AddItem( NewNode );
		return NewNode;
		unguard;
	}

	// Digest logging.
	INT Digestf( const TCHAR* Fmt, ... )
	{
		guard(Digestf);
		TCHAR Temp[4096];
		GET_VARARGS( Temp, ARRAY_COUNT(Temp), Fmt );	
		if ( DigestLog )
			new(*DigestLog) FString( Temp );
		//warnf( Temp );
		return DigestLog->Num()-1;
		unguard;
	}
	void DigestUpdatef( INT Index, const TCHAR* Fmt, ... )
	{
		guard(DigestUpdatef);
		TCHAR Temp[4096];
		GET_VARARGS( Temp, ARRAY_COUNT(Temp), Fmt );	
		if ( DigestLog )
			(*DigestLog)(Index) = Temp;
		//warnf( Temp );
		unguard;
	}
	FString OffsetIndentString( INT Offset, INT Indent )
	{
		guard(OffsetIndentString);
		FString Temp = FString::Printf( TEXT("0x%04X: "), Offset );
		while ( Indent-->0 )
			//Temp += TEXT("\t");
			Temp += TEXT("  ");
		return Temp;
		unguard;
	}
	FString TabsString( INT NumTabs )
	{
		guard(TabsString);
		FString Temp;
		while ( NumTabs-->0 )
			Temp += TEXT("\t");
		return Temp;
		unguard;
	}

	// Finds (first) Node linking to ThisNode.
	FScriptGraphNode* GetPrevNode( FScriptGraphNode* ThisNode )
	{
		guard(FScriptGraph::GetPrevNode);
		if ( !ThisNode )
			return NULL_PTR;
		for ( INT iNode=0; iNode<Nodes.Num(); iNode++ )
		{
			FScriptGraphNode* Node = Nodes(iNode);
			if ( Node->Next==ThisNode )
				return Node;
		}
		return NULL_PTR;
		unguard;
	}
	// Walks over Next linking until it hits the last Node.
	FScriptGraphNode* GetEndNode( FScriptGraphNode* ThisNode )
	{
		guard(FScriptGraph::GetEndNode);
		if ( !ThisNode )
			return NULL_PTR;
		for ( ThisNode; ThisNode->Next; ThisNode=ThisNode->Next )
			;
		return ThisNode;
		unguard;
	}
	FScriptGraphNode* GetNodeByOffset( INT Offset )
	{
		guard(FScriptGraph::GetNodeByOffset);
		for ( INT iNode=0; iNode<Nodes.Num(); iNode++ )
		{
			FScriptGraphNode* Node = Nodes(iNode);
			if ( Node->Offset==Offset )
				return Node;
		}
		return NULL_PTR;
		unguard;
	}

	// Recursive worker bee.
	FScriptGraphNode* DigestCode( BYTE* Code, INT& Offset, UState* Context, INT Indent, FOutputDevice* Error );

	// Entry points.
	UBOOL DigestReplication( TArray<FString>& DigestLog, UClass* Class, BYTE* Code, INT Size, FOutputDevice* Error );
	UBOOL DigestFunction( TArray<FString>& DigestLog, UFunction* Function, BYTE* Code, INT Size, FOutputDevice* Error );
	UBOOL DigestState( TArray<FString>& DigestLog, UState* State, BYTE* Code, INT Size, FOutputDevice* Error );

	// Post digest processing.
	void BuildJumpRefs();
	void InsertLabels( TArray<FLabelEntry>& Labels );
	void DetectEnumConstants();
	void DetectImplicitCasts();
	void FlagFunctionEnd();
	void FlagStateEnd();
	UBOOL RegainControl();
	UBOOL RegainControlAt( FScriptGraphNode* Node, ERegainPass Pass );

	// Export ScriptText.
	UBOOL ExportReplicationText( FString& Condition, FString& Annotation );
	UBOOL ExportFunctionText( TArray<FString>& Lines, FString& Annotation );
	UBOOL ExportStateText( TArray<FString>& Lines, FString& Annotation );

	UBOOL ExportNodesText( TArray<FString>& Lines, FScriptGraphNode* StartNode, INT Indent, FString& GlobalAnnotation );
	UBOOL ExportExpression( FString& Expression, FScriptGraphNode* CurrentNode, BYTE TopOperPrecedence, EScriptGraphOperatorType TopOperType, DWORD OperandFlags, INT SuppressPadding, FString& LineAnnotation, FString& GlobalAnnotation );
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
