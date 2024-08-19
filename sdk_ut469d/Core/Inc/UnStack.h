/*=============================================================================
	UnStack.h: UnrealScript execution stack definition.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack(push,OBJECT_ALIGNMENT)
#endif

class UStruct;

/*-----------------------------------------------------------------------------
	Constants & types.
-----------------------------------------------------------------------------*/

// Sizes.
enum {MAX_STRING_CONST_SIZE		= 256               };
enum {MAX_CONST_SIZE			= 16 *sizeof(TCHAR) };
enum {MAX_FUNC_PARMS			= 16                };

//
// UnrealScript intrinsic return value declaration.
//
#define RESULT_DECL void*const Result

//
// guardexec mechanism for script debugging.
//
#define unguardexecSlow  unguardfSlow(( TEXT("(%s @ %s : %04X)"), *FObjectFullName(Stack.Object), *FObjectFullName(Stack.Node), Stack.Code - &Stack.Node->Script(0) ))
#define unguardexec      unguardf    (( TEXT("(%s @ %s : %04X)"), *FObjectFullName(Stack.Object), *FObjectFullName(Stack.Node), Stack.Code - &Stack.Node->Script(0) ))

//
// State flags.
//
enum EStateFlags
{
	// State flags.
	STATE_Editable		= 0x00000001,	// State should be user-selectable in UnrealEd.
	STATE_Auto			= 0x00000002,	// State is automatic (the default state).
	STATE_Simulated     = 0x00000004,   // State executes on client side.
};

//
// Function flags.
//
enum EFunctionFlags
{
	// Function flags.
	FUNC_Final			= 0x00000001,	// Function is final (prebindable, non-overridable function).
	FUNC_Defined		= 0x00000002,	// Function has been defined (not just declared).
	FUNC_Iterator		= 0x00000004,	// Function is an iterator.
	FUNC_Latent		    = 0x00000008,	// Function is a latent state function.
	FUNC_PreOperator	= 0x00000010,	// Unary operator is a prefix operator.
	FUNC_Singular       = 0x00000020,   // Function cannot be reentered.
	FUNC_Net            = 0x00000040,   // Function is network-replicated.
	FUNC_NetReliable    = 0x00000080,   // Function should be sent reliably on the network.
	FUNC_Simulated		= 0x00000100,	// Function executed on the client side.
	FUNC_Exec		    = 0x00000200,	// Executable from command line.
	FUNC_Native			= 0x00000400,	// Native function.
	FUNC_Event          = 0x00000800,   // Event function.
	FUNC_Operator       = 0x00001000,   // Operator function.
	FUNC_Static         = 0x00002000,   // Static function.
	FUNC_NoExport       = 0x00004000,   // Don't export intrinsic function to C++.
	FUNC_Const          = 0x00008000,   // Function doesn't modify this object.
	FUNC_Invariant      = 0x00010000,   // Return value is purely dependent on parameters; no state dependencies or internal state changes.
	FUNC_Redirect		= 0x00040000,	// Calls to this function should be redirected

	// Combinations of flags.
	FUNC_FuncInherit        = FUNC_Exec | FUNC_Event,
	FUNC_FuncOverrideMatch	= FUNC_Exec | FUNC_Final | FUNC_Latent | FUNC_PreOperator | FUNC_Iterator | FUNC_Static,
	FUNC_NetFuncFlags       = FUNC_Net | FUNC_NetReliable,
};

//
// Evaluatable expression item types.
//
enum EExprToken
{
	// Variable references.
	EX_LocalVariable		= 0x00,	// A local variable.
	EX_InstanceVariable		= 0x01,	// An object variable.
	EX_DefaultVariable		= 0x02,	// Default variable for a concrete object.

	// Tokens.
	EX_Return				= 0x04,	// Return from function.
	EX_Switch				= 0x05,	// Switch.
	EX_Jump					= 0x06,	// Goto a local address in code.
	EX_JumpIfNot			= 0x07,	// Goto if not expression.
	EX_Stop					= 0x08,	// Stop executing state code.
	EX_Assert				= 0x09,	// Assertion.
	EX_Case					= 0x0A,	// Case.
	EX_Nothing				= 0x0B,	// No operation.
	EX_LabelTable			= 0x0C,	// Table of labels.
	EX_GotoLabel			= 0x0D,	// Goto a label.
	EX_EatString            = 0x0E, // Ignore a dynamic string.
	EX_Let					= 0x0F,	// Assign an arbitrary size value to a variable.
	EX_DynArrayElement      = 0x10, // Dynamic array element.!!
	EX_New                  = 0x11, // New object allocation.
	EX_ClassContext         = 0x12, // Class default metaobject context.
	EX_MetaCast             = 0x13, // Metaclass cast.
	EX_LetBool				= 0x14, // Let boolean variable.
	//
	EX_EndFunctionParms		= 0x16,	// End of function call parameters.
	EX_Self					= 0x17,	// Self object.
	EX_Skip					= 0x18,	// Skippable expression.
	EX_Context				= 0x19,	// Call a function through an object context.
	EX_ArrayElement			= 0x1A,	// Array element.
	EX_VirtualFunction		= 0x1B,	// A function call with parameters.
	EX_FinalFunction		= 0x1C,	// A prebound function call with parameters.
	EX_IntConst				= 0x1D,	// Int constant.
	EX_FloatConst			= 0x1E,	// Floating point constant.
	EX_StringConst			= 0x1F,	// String constant.
	EX_ObjectConst		    = 0x20,	// An object constant.
	EX_NameConst			= 0x21,	// A name constant.
	EX_RotationConst		= 0x22,	// A rotation constant.
	EX_VectorConst			= 0x23,	// A vector constant.
	EX_ByteConst			= 0x24,	// A byte constant.
	EX_IntZero				= 0x25,	// Zero.
	EX_IntOne				= 0x26,	// One.
	EX_True					= 0x27,	// Bool True.
	EX_False				= 0x28,	// Bool False.
	EX_NativeParm           = 0x29, // Native function parameter offset.
	EX_NoObject				= 0x2A,	// NoObject.
	EX_IntConstByte			= 0x2C,	// Int constant that requires 1 byte.
	EX_BoolVariable			= 0x2D,	// A bool variable which requires a bitmask.
	EX_DynamicCast			= 0x2E,	// Safe dynamic class casting.
	EX_Iterator             = 0x2F, // Begin an iterator operation.
	EX_IteratorPop          = 0x30, // Pop an iterator level.
	EX_IteratorNext         = 0x31, // Go to next iteration.
	EX_StructCmpEq          = 0x32,	// Struct binary compare-for-equal.
	EX_StructCmpNe          = 0x33,	// Struct binary compare-for-unequal.
	EX_UnicodeStringConst   = 0x34, // Unicode string constant.
	//
	EX_StructMember         = 0x36, // Struct member.
	//
	EX_GlobalFunction		= 0x38, // Call non-state version of a function.

	// Native conversions.
	EX_MinConversion		= 0x39,	// Minimum conversion token.
	EX_RotatorToVector		= 0x39,
	EX_ByteToInt			= 0x3A,
	EX_ByteToBool			= 0x3B,
	EX_ByteToFloat			= 0x3C,
	EX_IntToByte			= 0x3D,
	EX_IntToBool			= 0x3E,
	EX_IntToFloat			= 0x3F,
	EX_BoolToByte			= 0x40,
	EX_BoolToInt			= 0x41,
	EX_BoolToFloat			= 0x42,
	EX_FloatToByte			= 0x43,
	EX_FloatToInt			= 0x44,
	EX_FloatToBool			= 0x45,
	//
	EX_ObjectToBool			= 0x47,
	EX_NameToBool			= 0x48,
	EX_StringToByte			= 0x49,
	EX_StringToInt			= 0x4A,
	EX_StringToBool			= 0x4B,
	EX_StringToFloat		= 0x4C,
	EX_StringToVector		= 0x4D,
	EX_StringToRotator		= 0x4E,
	EX_VectorToBool			= 0x4F,
	EX_VectorToRotator		= 0x50,
	EX_RotatorToBool		= 0x51,
	EX_ByteToString			= 0x52,
	EX_IntToString			= 0x53,
	EX_BoolToString			= 0x54,
	EX_FloatToString		= 0x55,
	EX_ObjectToString		= 0x56,
	EX_NameToString			= 0x57,
	EX_VectorToString		= 0x58,
	EX_RotatorToString		= 0x59,
	EX_MaxConversion		= 0x60,	// Maximum conversion token.

	// Natives.
	EX_ExtendedNative		= 0x60,
	EX_FirstNative			= 0x70,
	EX_Max					= 0x1000,
};

//
// Latent functions.
//
enum EPollSlowFuncs
{
	EPOLL_Sleep			      = 384,
	EPOLL_FinishAnim	      = 385,
	EPOLL_FinishInterpolation = 302,
};

/*-----------------------------------------------------------------------------
	Execution stack helpers.
-----------------------------------------------------------------------------*/

//
// stijn: UnrealScript functions and states need an execution stack to store
// local variables and function parameters, to keep track of important values
// such as the "this" pointer (stored in FFrame::Object) and the program counter
// (stored in FFrame::Code), and to remember latent actions.
//
// The type of FFrame we use depends on how we invoked the currently executing
// UScript function/state:
//
// i) We can begin executing UnrealScript code by calling UObject::ProcessEvent
// in C++ code. ProcessEvent sets up a top-level FFrame, allocates stack space
// for the function parameters and local variables of the callee, and
// initializes the parameters by copying the contents of ProcessEvent's Parms
// buffer into the new FFrame. We then execute opcodes one by one using this
// top-level FFrame as our UScript execution stack until we reach an EX_Return
// opcode or until we hit the end of the function.
//
// ii) We can begin executing UnrealScript state code by calling
// AActor::ProcessState. ProcessState does not allocate any new FFrames, but
// rather uses the StateFrame of the object whose state we're executing as the
// top-level frame. ProcessState executes opcodes one by one until it reaches an
// EX_Stop opcode.
//
// iii) We can call new UScript functions using UScript opcodes such as
// EX_VirtualFunction and EX_GlobalFunction. These opcodes allocate a new FFrame
// for the callee, allocate new stack space for the callee's function parameters
// and local variables, and initialize the local variables by executing the
// UScript opcodes that proceed the function call opcode until an
// EX_EndFunctionParms opcode is found. The function parameter opcodes execute
// using the FFrame/FStateFrame of the caller, but write their results into the
// FFrame of the callee.
//
// There are nasty interactions between the GotoState/GotoLabel functions and
// the various opcodes/internal helper macros/VM functions that read or modify
// the program counter. Most of the operations that update the program counter
// will do so by calculating a new program counter value, which they then store
// in the Code field of the current FFrame. These operations include:
//
// 1) Computed-jump opcodes such as EX_JumpIfNot and EX_Switch update the
// program counter of the current FFrame/FStackFrame
// 2) Actor iterators modify the program counter when they jump to the iterator
// body
// 3) Opcodes that need to skip over instructions after encountering a null
// object (accessed nones) increment the program counter with a statically
// computed skip offset. These opcodes include EX_Context and EX_ClassContext.
// 4) The FFrame::Step function fetches the next UScript opcode from the
// location pointed to by the program counter, increments the program counter by
// 1 (the size of a UScript opcode), and executes the fetched opcode.
// 5) Opcodes that call UScript functions fetch and execute function parameter
// opcodes one by one until the program counter reaches an EX_EndFunctionParms
// opcode.
// 6) C++ parameters used to read the parameters of a native function called
// from UScript read and modify the UScript program counter.
//
// The GotoState/GotoLabel functions also compute a new program counter, but
// they store the updated program counter directly in the StateFrame. If we're
// executing non-state code at the time of the GotoState/GotoLabel call, the
// current FFrame and the StateFrame will be different frames (see the
// description of mechanisms i and iii above).
//
// Similarly, if we're executing a UScript function called from state code, the
// current FFrame and the StateFrame will also be different frames (see the
// description of mechanism iii above).
//
// If, however, we are executing top-level state code at the time of the
// GotoState/GotoLabel call, the top-level FFrame _IS_ the StateFrame (see
// mechanism ii above). This means that if any of the operations listed above
// executes between the GotoState/GotoLabel call and the next instruction fetch
// in AActor::ProcessState, we will corrupt the calculated program counter, and
// AActor::ProcessState will jump to the wrong location in the new state.
// 

//
// Information about script execution at one stack level.
//
struct CORE_API FFrame : public FOutputDevice
{	
	// Variables.

	//
	// stijn: This is the node whose code we're currently executing.
	// Might be a class, function, struct, or state. The state might be a super
	// state of the object's _actual_ state (which is stored in
	// FStateFrame::StateNode)
	// 
	UStruct*	Node;
	UObject*	Object;
	BYTE*		Code;
	BYTE*		Locals;

	// Constructors.
	FFrame( UObject* InObject );
	FFrame( UObject* InObject, UStruct* InNode, INT CodeOffset, void* InLocals );

	// Functions.
	void Step( UObject* Context, RESULT_DECL );
	void Serialize( const TCHAR* V, enum EName Event );
	INT ReadInt();
	UObject* ReadObject();
	FLOAT ReadFloat();
	INT ReadWord();
	FName ReadName();
};

//
// Information about script execution at the main stack level.
// This part of an actor's script state is saveable at any time.
//
struct CORE_API FStateFrame : public FFrame
{
	// Variables.
	FFrame* CurrentFrame;
	UState* StateNode;
	QWORD   ProbeMask;
	INT     LatentAction;

	//
	// stijn: Object whose state we're currently processing. Marco called this
	// the StateExecutor in 227. If we execute a GotoLabel or GotoState on an
	// object whose state is currently being processed, we will need to:
	// 
	// 1) Restore the original value of StateFrame->Code before returning from
	// GotoState/GotoLabel. This is necessary because we need to delay the
	// control-flow transfer to the new State/Label until we're done executing
	// the current nested expression.
	//
	// 2) Remember the computed jump target in GotoState/GotoLabel, and jump
	// to the computed target as soon as we're done executing the current nested
	// expression.
	// 
	static UObject* CurrentObject;
	static BYTE* PendingStateCode;
	static BOOL  HavePendingStateCode;

	// Functions.
	FStateFrame( UObject* InObject );
#if OLDUNREAL_BINARY_COMPAT
	const TCHAR* Describe();
#else
	FString Describe() { return DescribeSafe(); }
#endif
	FString DescribeSafe();
};

/*-----------------------------------------------------------------------------
	Script execution helpers.
-----------------------------------------------------------------------------*/

//
// Base class for UnrealScript iterator lists.
//
struct FIteratorList
{
	FIteratorList* Next;
	FIteratorList() {}
	FIteratorList( FIteratorList* InNext ) : Next( InNext ) {}
	FIteratorList* GetNext() { return (FIteratorList*)Next; }
};

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack(pop)
#endif

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
