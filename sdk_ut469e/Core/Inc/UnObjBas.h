/*=============================================================================
	UnObjBas.h: Unreal object base class.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

/*-----------------------------------------------------------------------------
	Core enumerations.
-----------------------------------------------------------------------------*/

//
// Flags for loading objects.
//
enum ELoadFlags
{
	LOAD_None			= 0x00000,	// No flags.
	LOAD_NoFail			= 0x00001,	// Critical error if load fails.
	LOAD_NoWarn			= 0x00002,	// Don't display warning if load fails.
	LOAD_Throw			= 0x00008,	// Throw exceptions upon failure.
	LOAD_Verify			= 0x00010,	// Only verify existance; don't actually load.
	LOAD_AllowDll		= 0x00020,	// Allow plain DLLs.
	LOAD_DisallowFiles  = 0x00040,	// Don't load from file.
	LOAD_NoVerify       = 0x00080,  // Don't verify imports yet.
	LOAD_Forgiving      = 0x01000,  // Forgive missing imports (set them to NULL).
	LOAD_Quiet			= 0x02000,  // No log warnings.
	LOAD_NoRemap        = 0x04000,  // No remapping of packages.
	LOAD_CheckCache		= 0x10000,	// 469: Check cache.ini if we don't find the file elsewhere
	
	// LoadFlags that need to be propagated when loading linkers recursively
	LOAD_Propagate      = LOAD_CheckCache,  
};

//
// Package flags.
//
enum EPackageFlags
{
	PKG_AllowDownload	= 0x0001,	// Allow downloading package.
	PKG_ClientOptional  = 0x0002,	// Purely optional for clients.
	PKG_ServerSideOnly  = 0x0004,   // Only needed on the server side.
	PKG_BrokenLinks     = 0x0008,   // Loaded from linker with broken import links.
	PKG_Unsecure        = 0x0010,   // Not trusted.
	PKG_RequireMD5      = 0x0020,   // Server is requiring MD5 from the client.
	PKG_Need			= 0x8000,	// Client needs to download this package.

	// Flags we want to keep after loading a package.
	PKG_PostLoadKeep    = PKG_AllowDownload | PKG_ClientOptional | PKG_ServerSideOnly | PKG_Unsecure | PKG_Need,
};

//
// Internal enums.
//
enum ENativeConstructor    {EC_NativeConstructor};
enum EStaticConstructor    {EC_StaticConstructor};
enum EInternal             {EC_Internal};
enum ECppProperty          {EC_CppProperty};
enum EInPlaceConstructor   {EC_InPlaceConstructor};

//
// Result of GotoState.
//
enum EGotoState
{
	GOTOSTATE_NotFound		= 0,
	GOTOSTATE_Success		= 1,
	GOTOSTATE_Preempted		= 2,
};

//
// Flags describing a class.
//
enum EClassFlags
{
	// Base flags.
	CLASS_Abstract          = 0x00001,  // Class is abstract and can't be instantiated directly.
	CLASS_Compiled			= 0x00002,  // Script has been compiled successfully.
	CLASS_Config			= 0x00004,  // Load object configuration at construction time.
	CLASS_Transient			= 0x00008,	// This object type can't be saved; null it out at save time.
	CLASS_Parsed            = 0x00010,	// Successfully parsed.
	CLASS_Localized         = 0x00020,  // Class contains localized text.
	CLASS_SafeReplace       = 0x00040,  // Objects of this class can be safely replaced with default or NULL.
	CLASS_RuntimeStatic     = 0x00080,	// Objects of this class are static during gameplay.
	CLASS_NoExport          = 0x00100,  // Don't export to C++ header.
	CLASS_NoUserCreate      = 0x00200,  // Don't allow users to create in the editor.
	CLASS_PerObjectConfig   = 0x00400,  // Handle object configuration on a per-object basis, rather than per-class.
	CLASS_NativeReplication = 0x00800,  // Replication handled in C++.
	CLASS_EditorExpanded    = 0x02000,	// Editor use only. Set when the Actor Browser has added this class' children to the actor tree.
	CLASS_SourceStripped	= 0x04000,	// OldUnreal: This class is source-stripped and should not be recompiled unless someone has written code for it

	// Flags to inherit from base class.
	CLASS_Inherit           = CLASS_Transient | CLASS_Config | CLASS_Localized | CLASS_SafeReplace | CLASS_RuntimeStatic | CLASS_PerObjectConfig,
	CLASS_RecompilerClear   = CLASS_Inherit | CLASS_Abstract | CLASS_NoExport | CLASS_NativeReplication,
};

//
// Flags associated with each property in a class, overriding the
// property's default behavior.
//
enum EPropertyFlags
{
	// Regular flags.
	CPF_Edit		 = 0x00000001, // Property is user-settable in the editor.
	CPF_Const		 = 0x00000002, // Actor's property always matches class's default actor property.
	CPF_Input		 = 0x00000004, // Variable is writable by the input system.
	CPF_ExportObject = 0x00000008, // Object can be exported with actor.
	CPF_OptionalParm = 0x00000010, // Optional parameter (if CPF_Param is set).
	CPF_Net			 = 0x00000020, // Property is relevant to network replication.
	CPF_ConstRef     = 0x00000040, // Reference to a constant object.
	CPF_Parm		 = 0x00000080, // Function/When call parameter.
	CPF_OutParm		 = 0x00000100, // Value is copied out after function call.
	CPF_SkipParm	 = 0x00000200, // Property is a short-circuitable evaluation function parm.
	CPF_ReturnParm	 = 0x00000400, // Return value.
	CPF_CoerceParm	 = 0x00000800, // Coerce args into this function parameter.
	CPF_Native       = 0x00001000, // Property is native: C++ code is responsible for serializing it.
	CPF_Transient    = 0x00002000, // Property is transient: shouldn't be saved, zero-filled at load time.
	CPF_Config       = 0x00004000, // Property should be loaded/saved as permanent profile.
	CPF_Localized    = 0x00008000, // Property should be loaded as localizable text.
	CPF_Travel       = 0x00010000, // Property travels across levels/servers.
	CPF_EditConst    = 0x00020000, // Property is uneditable in the editor.
	CPF_GlobalConfig = 0x00040000, // Load config from base class, not subclass.
	CPF_OnDemand     = 0x00100000, // Object or dynamic array loaded on demand only.
	CPF_New			 = 0x00200000, // Automatically create inner object.
	CPF_NeedCtorLink = 0x00400000, // Fields need construction/destruction.

	// Combinations of flags.
	CPF_ParmFlags           = CPF_OptionalParm | CPF_Parm | CPF_OutParm | CPF_SkipParm | CPF_ReturnParm | CPF_CoerceParm,
	CPF_PropagateFromStruct = CPF_Const | CPF_Native | CPF_Transient,
};

//
// Flags describing an object instance.
//
enum EObjectFlags
{
	RF_Transactional    = 0x00000001,   // Object is transactional.
	RF_Unreachable		= 0x00000002,	// Object is not reachable on the object graph.
	RF_Public			= 0x00000004,	// Object is visible outside its package.
	RF_TagImp			= 0x00000008,	// Temporary import tag in load/save.
	RF_TagExp			= 0x00000010,	// Temporary export tag in load/save.
	RF_SourceModified   = 0x00000020,   // Modified relative to source files.	
	RF_TagGarbage		= 0x00000040,	// Check during garbage collection.
	RF_WindowInit		= 0x00000080,	// Object was created in UWindows
	//
	RF_ActorObject      = 0x00000100,   // stijn: actor object. can get destroyed at any time
	RF_NeedLoad			= 0x00000200,   // During load, indicates object needs loading.
	RF_HighlightedName  = 0x00000400,	// A hardcoded name which should be syntax-highlighted.
	RF_EliminateObject  = 0x00000400,   // NULL out references to this during garbage collecion.
	RF_InSingularFunc   = 0x00000800,	// In a singular function.	//Deprecated
	RF_RemappedName     = 0x00000800,   // Name is remapped.
	RF_HasDangerousRefs = 0x00000800,   // stijn: used to flag non-actor objects that contain refs to actors
	RF_Suppress         = 0x00001000,	//warning: Mirrored in UnName.h. Suppressed log name.
	RF_StateChanged     = 0x00001000,   // Object did a state change.
	RF_InEndState       = 0x00002000,   // Within an EndState call.
	RF_UnsafeSpawn      = 0x00002000,   // Higor: Name is not safe to use for new objects.
	RF_Transient        = 0x00004000,	// Don't save object.
	RF_Preloading       = 0x00008000,   // Data is being preloaded from file.
	RF_LoadForClient	= 0x00010000,	// In-file load for client.
	RF_LoadForServer	= 0x00020000,	// In-file load for client.
	RF_LoadForEdit		= 0x00040000,	// In-file load for client.
	RF_Standalone       = 0x00080000,   // Keep object around for editing even if unreferenced.
	RF_NotForClient		= 0x00100000,	// Don't load this object for the game client.
	RF_NotForServer		= 0x00200000,	// Don't load this object for the game server.
	RF_NotForEdit		= 0x00400000,	// Don't load this object for the editor.
	RF_Destroyed        = 0x00800000,	// Object Destroy has already been called.
	RF_NeedPostLoad		= 0x01000000,   // Object needs to be postloaded.
	RF_HasStack         = 0x02000000,	// Has execution stack.
	RF_Native			= 0x04000000,   // Native (UClass only).
	RF_Marked			= 0x08000000,   // Marked (for debugging).
	RF_ErrorShutdown    = 0x10000000,	// ShutdownAfterError called.
	RF_DebugPostLoad    = 0x20000000,   // For debugging Serialize calls.
	RF_DebugSerialize   = 0x40000000,   // For debugging Serialize calls.
	RF_DebugDestroy     = 0x80000000,   // For debugging Destroy calls.
	RF_ContextFlags		= RF_NotForClient | RF_NotForServer | RF_NotForEdit, // All context flags.
	RF_LoadContextFlags	= RF_LoadForClient | RF_LoadForServer | RF_LoadForEdit, // Flags affecting loading.
	RF_Load  			= RF_ContextFlags | RF_LoadContextFlags | RF_Public | RF_Standalone | RF_Native | RF_SourceModified | RF_Transactional | RF_HasStack, // Flags to load from Unrealfiles.
	RF_Keep             = RF_Native | RF_Marked, // Flags to persist across loads.
	RF_ScriptMask		= RF_Transactional | RF_Public | RF_Transient | RF_NotForClient | RF_NotForServer | RF_NotForEdit // Script-accessible flags.
};

/*----------------------------------------------------------------------------
	Core types.
----------------------------------------------------------------------------*/

//
// Globally unique identifier.
//
class CORE_API FGuid
{
public:
	DWORD A,B,C,D;
	FGuid()
	{}
	FGuid( DWORD InA, DWORD InB, DWORD InC, DWORD InD )
	: A(InA), B(InB), C(InC), D(InD)
	{}
	friend UBOOL operator==(const FGuid& X, const FGuid& Y)
		{return X.A==Y.A && X.B==Y.B && X.C==Y.C && X.D==Y.D;}
	friend UBOOL operator!=(const FGuid& X, const FGuid& Y)
		{return X.A!=Y.A || X.B!=Y.B || X.C!=Y.C || X.D!=Y.D;}
	friend FArchive& operator<<( FArchive& Ar, FGuid& G )
	{
		guard(FGuid<<);
		return Ar << G.A << G.B << G.C << G.D;
		unguard;
	}
	TCHAR* String() const
	{
		FString& Result = appStaticFString();
		Result = FString::Printf(TEXT("%08X%08X%08X%08X"), A, B, C, D );
		return const_cast<TCHAR*>(*Result);
	}
	// stijn: Returns the maximum generation we can target for this GUID while preserving network compatibility
	INT GetNetCompatGeneration()
	{
		// Botpack
		if (A == 0x1c696576 && B == 0x11d38f44 && C == 0x100067b9 && D == 0xf6f8975a)
			return 14;
		// Core
		if (A == 0x4770b884 && B == 0x11d38e3e && C == 0x100067b9 && D == 0xf6f8975a)
			return 10;
		// Editor
		if (A == 0x4770b886 && B == 0x11d38e3e && C == 0x100067b9 && D == 0xf6f8975a)
			return 11;
		// Engine
		if (A == 0xd18a7b92 && B == 0x11d38f04 && C == 0x100067b9 && D == 0xf6f8975a)
			return 17;
		// Fire
		if (A == 0x4770b888 && B == 0x11d38e3e && C == 0x100067b9 && D == 0xf6f8975a)
			return 10;
		// IpDrv
		if (A == 0x4770b889 && B == 0x11d38e3e && C == 0x100067b9 && D == 0xf6f8975a)
			return 10;
		// IpServer
		if (A == 0x4770b88f && B == 0x11d38e3e && C == 0x100067b9 && D == 0xf6f8975a)
			return 11;
		// UBrowser
		if (A == 0x4770b88b && B == 0x11d38e3e && C == 0x100067b9 && D == 0xf6f8975a)
			return 10;
		// UTBrowser
		if (A == 0x4770b893 && B == 0x11d38e3e && C == 0x100067b9 && D == 0xf6f8975a)
			return 10;
		// UTMenu
		if (A == 0x1c696577 && B == 0x11d38f44 && C == 0x100067b9 && D == 0xf6f8975a)
			return 11;
		// UTServerAdmin
		if (A == 0x4770b891 && B == 0x11d38e3e && C == 0x100067b9 && D == 0xf6f8975a)
			return 10;
		// UWeb
		if (A == 0x4770b88a && B == 0x11d38e3e && C == 0x100067b9 && D == 0xf6f8975a)
			return 10;
		// UWindow
		if (A == 0x4770b887 && B == 0x11d38e3e && C == 0x100067b9 && D == 0xf6f8975a)
			return 10;
		// UnrealI
		if (A == 0x4770b88d && B == 0x11d38e3e && C == 0x100067b9 && D == 0xf6f8975a)
			return 1;
		// UnrealShare
		if (A == 0x4770b88c && B == 0x11d38e3e && C == 0x100067b9 && D == 0xf6f8975a)
			return 1;
		// De
		if (A == 0xfb1eb231 && B == 0x4d8bf569 && C == 0xae12c6bf && D == 0x1a08a2f7)
			return 1;
		// epiccustommodels
		if (A == 0x13f8255a && B == 0x11d3dba0 && C == 0x1000cbb9 && D == 0xf6f8975a)
			return 2;
		// multimesh 
		if (A == 0x2db53b00 && B == 0x11d3e900 && C == 0x1000d3b9 && D == 0xf6f8975a)
			return 8;
		// relics
		if (A == 0xd011f66e && B == 0x11d3e9b8 && C == 0x1000d5b9 && D == 0xf6f8975a)
			return 9;
		// relicsbindings
		if (A == 0x465ad1fe && B == 0x11d3df5c && C == 0x1000cdb9 && D == 0xf6f8975a)
			return 2;

		return 0;
	}
};
inline DWORD GetTypeHash(const FGuid G)
{
	return G.A;
}
inline INT CompareGuids( FGuid* A, FGuid* B )
{
	INT Diff;
	Diff = A->A-B->A; if( Diff ) return Diff;
	Diff = A->B-B->B; if( Diff ) return Diff;
	Diff = A->C-B->C; if( Diff ) return Diff;
	Diff = A->D-B->D; if( Diff ) return Diff;
	return 0;
}

//
// COM IUnknown interface.
//
class CORE_API FUnknown
{
public:
	virtual DWORD STDCALL QueryInterface( const FGuid& RefIID, void** InterfacePtr ) {return 0;}
	virtual DWORD STDCALL AddRef() {return 0;}
	virtual DWORD STDCALL Release() {return 0;}
};

//
// Information about a driver class.
//
class CORE_API FRegistryObjectInfo
{
public:
	FString Object;
	FString Class;
	FString MetaClass;
	FString Description;
	FString Autodetect;
	FRegistryObjectInfo()
	: Object(), Class(), MetaClass(), Description(), Autodetect()
	{}
};

//
// Information about a preferences menu item.
//
class CORE_API FPreferencesInfo
{
public:
	FString Caption;
	FString ParentCaption;
	FString Class;
	FName Category;
	UBOOL Immediate;
	FPreferencesInfo()
	: Caption(), ParentCaption(), Class(), Category(NAME_None), Immediate(0)
	{}
};

//
// Information about a property
//
class CORE_API FRegistryPropertyInfo
{
public:
	FString Object;
	FString Class;
	FString Title;
	FString Description;
	TArray<FString> Values;
	TArray<FString> ValueDescriptions;
	FLOAT RangeMin;
	FLOAT RangeMax;
	FLOAT Step;
	union{ struct{
		BITFIELD Config:1;         // Editable config property
		BITFIELD RequireRestart:1; // Changing config property requires restart
		BITFIELD Constant:1;       // This property does not change
	}; BITFIELD Flags;};

	FRegistryPropertyInfo()
	: RangeMin(0), RangeMax(0), Step(0), Flags(0)
	{}
};

/*----------------------------------------------------------------------------
	Core macros.
----------------------------------------------------------------------------*/

// Special canonical package for FindObject, ParseObject.
#define ANY_PACKAGE ((UPackage*)-1)

// Define private default constructor.
#define NO_DEFAULT_CONSTRUCTOR(cls) \
	protected: cls() {} public:

// Guard macros.
#define unguardobjSlow		unguardfSlow(( TEXT("(%s)"), *FObjectFullName(this) ))
#define unguardobj			unguardf(( TEXT("(%s)"), *FObjectFullName(this) ))

// Verify the a class definition and C++ definition match up.
#define VERIFY_CLASS_OFFSET(Pre,ClassName,Member) \
	{for( TFieldIterator<UProperty> It( FindObjectChecked<UClass>( Pre##ClassName::StaticClass()->GetOuter(), TEXT(#ClassName) ) ); It; ++It ) \
		if( appStricmp(It->GetName(),TEXT(#Member))==0 ) \
			if( It->Offset != STRUCT_OFFSET(Pre##ClassName,Member) ) \
				appErrorf(TEXT("Class %s Member %s problem: Script=%i C++=%i"), TEXT(#ClassName), TEXT(#Member), It->Offset, STRUCT_OFFSET(Pre##ClassName,Member) );}

// Verify that C++ and script code agree on the size of a class.
#define VERIFY_CLASS_SIZE(ClassName) \
	check(sizeof(ClassName)==ClassName::StaticClass()->GetPropertiesSize());

// Verify a class definition and C++ definition match up (don't assert).
#if DO_GUARD_SLOW
#define VERIFY_CLASS_OFFSET_NODIE(Pre,ClassName,Member) \
{ \
	bool found = false; \
	for( TFieldIterator<UProperty> It( FindObjectChecked<UClass>( Pre##ClassName::StaticClass()->GetOuter(), TEXT(#ClassName) ) ); It; ++It ) \
	{ \
		if( appStricmp(It->GetName(),TEXT(#Member))==0 ) { \
			found = true; \
			if( It->Offset != STRUCT_OFFSET(Pre##ClassName,Member) && appStricmp(TEXT(#Member), TEXT("Region")) ) { \
				debugf(TEXT("VERIFY: Class %s Member %s problem: Script=%i C++=%i"), TEXT(#ClassName), TEXT(#Member), It->Offset, STRUCT_OFFSET(Pre##ClassName,Member) ); \
				Mismatch = true; \
			} \
		} \
	} \
	if (!found)															\
		debugf(TEXT("VERIFY: Class '%s' doesn't appear to have a '%s' member"), TEXT(#ClassName), TEXT(#Member) ); \
}
#else
#define VERIFY_CLASS_OFFSET_NODIE(Pre,ClassName,Member)
#endif

// Verify that C++ and script code agree on the size of a class (don't assert).
#define VERIFY_CLASS_SIZE_NODIE(ClassName) \
	if (sizeof(ClassName)!=ClassName::StaticClass()->GetPropertiesSize()) { \
		debugf(TEXT("VERIFY: Class %s size problem; Script=%i C++=%i"), TEXT(#ClassName), (int) ClassName::StaticClass()->GetPropertiesSize(), sizeof(ClassName)); \
		Mismatch = true; \
	}

// Declare the base UObject class.

#if !__STATIC_LINK
#define DECLARE_BASE_CLASS( TClass, TSuperClass, TStaticFlags, ... ) \
public: \
	/* Identification */ \
	enum {StaticClassFlags=TStaticFlags}; \
	private: static UClass PrivateStaticClass; public: \
	typedef TSuperClass Super;\
	typedef TClass ThisClass;\
	static UClass* StaticClass() \
		{ return &PrivateStaticClass; } \
	void* operator new( size_t Size, UObject* Outer=(UObject*)GetTransientPackage(), FName Name=NAME_None, DWORD SetFlags=0 ) \
		{ return StaticAllocateObject( StaticClass(), Outer, Name, SetFlags ); } \
	void* operator new( size_t Size, EInternal* Mem ) \
		{ return (void*)Mem; }
#else
#define DECLARE_BASE_CLASS( TClass, TSuperClass, TStaticFlags, TPackage ) \
public: \
	/* Identification */ \
	enum {StaticClassFlags=TStaticFlags}; \
	private: \
	static UClass* PrivateStaticClass; \
	public: \
	typedef TSuperClass Super;\
	typedef TClass ThisClass;\
	static UClass* GetPrivateStaticClass##TClass( const TCHAR* Package ); \
	static void InitializePrivateStaticClass##TClass(); \
	static UClass* StaticClass() \
	{ \
		if (!PrivateStaticClass) \
		{ \
			PrivateStaticClass = GetPrivateStaticClass##TClass( TEXT(#TPackage) ); \
			InitializePrivateStaticClass##TClass(); \
		} \
		return PrivateStaticClass; \
	} \
	void* operator new( size_t Size, UObject* Outer=(UObject*)GetTransientPackage(), FName Name=NAME_None, DWORD SetFlags=0 ) \
		{ return StaticAllocateObject( StaticClass(), Outer, Name, SetFlags ); } \
	void* operator new( size_t Size, EInternal* Mem ) \
		{ return (void*)Mem; }
#endif

// Declare a concrete class.
#define DECLARE_CLASS( TClass, TSuperClass, TStaticFlags, ... ) \
	DECLARE_BASE_CLASS( TClass, TSuperClass, TStaticFlags, __VA_ARGS__ ) \
	friend FArchive &operator<<( FArchive& Ar, TClass*& Res ) \
		{ return Ar << *(UObject**)&Res; } \
	virtual ~TClass() noexcept(false)   \
		{ ConditionalDestroy(); } \
	static void InternalConstructor( void* X ) \
		{ new( (EInternal*)X )TClass; } \

// Declare an abstract class.
#define DECLARE_ABSTRACT_CLASS( TClass, TSuperClass, TStaticFlags, ... ) \
	DECLARE_BASE_CLASS( TClass, TSuperClass, TStaticFlags | CLASS_Abstract, __VA_ARGS__ ) \
	friend FArchive &operator<<( FArchive& Ar, TClass*& Res ) \
		{ return Ar << *(UObject**)&Res; } \
	virtual ~TClass() noexcept(false)  \
		{ ConditionalDestroy(); } \

// Declare that objects of class being defined reside within objects of the specified class.
#define DECLARE_WITHIN( TWithinClass ) \
	typedef TWithinClass WithinClass; \
	TWithinClass* GetOuter##TWithinClass() const { return (TWithinClass*)GetOuter(); }

// Register a class at startup time.
#if __STATIC_LINK
#define IMPLEMENT_CLASS(TClass) \
	UClass* TClass::PrivateStaticClass = NULL; \
	UClass* TClass::GetPrivateStaticClass##TClass(const TCHAR* Package) \
		{ \
		UClass* ReturnClass; \
		ReturnClass = ::new UClass \
		(\
			EC_StaticConstructor, \
			sizeof(TClass), \
			StaticClassFlags, \
			FGuid(GUID1, GUID2, GUID3, GUID4), \
			&TEXT(#TClass)[1], \
			Package, \
			StaticConfigName(), \
			RF_Public | RF_Standalone | RF_Transient | RF_Native, \
			(void(*)(void*))TClass::InternalConstructor, \
			(void(UObject::*)())&TClass::StaticConstructor \
		); \
		check(ReturnClass); \
		return ReturnClass; \
		} \
		/* Called from ::StaticClass after GetPrivateStaticClass */ \
		void TClass::InitializePrivateStaticClass##TClass() \
		{ \
			/* No recursive ::StaticClass calls allowed. Setup extras. */ \
			if (TClass::Super::StaticClass() != TClass::PrivateStaticClass) \
				TClass::PrivateStaticClass->SuperField = TClass::Super::StaticClass(); \
			else \
				TClass::PrivateStaticClass->SuperField = NULL; \
			TClass::PrivateStaticClass->ClassWithin = TClass::WithinClass::StaticClass(); \
			TClass::PrivateStaticClass->SetClass(UClass::StaticClass()); \
			/* Perform UObject native registration. */ \
			if (TClass::PrivateStaticClass->GetInitialized() && TClass::PrivateStaticClass->GetClass() == TClass::PrivateStaticClass->StaticClass()) \
				TClass::PrivateStaticClass->Register(); \
		}
#elif _MSC_VER
	#define IMPLEMENT_CLASS(TClass) \
		UClass TClass::PrivateStaticClass \
		( \
			EC_NativeConstructor, \
			sizeof(TClass), \
			TClass::StaticClassFlags, \
			TClass::Super::StaticClass(), \
			TClass::WithinClass::StaticClass(), \
			FGuid(TClass::GUID1,TClass::GUID2,TClass::GUID3,TClass::GUID4), \
			&TEXT(#TClass)[1], \
			::GPackage, \
			StaticConfigName(), \
			RF_Public | RF_Standalone | RF_Transient | RF_Native, \
			(void(*)(void*))TClass::InternalConstructor, \
			(void(UObject::*)())&TClass::StaticConstructor \
		); \
		extern "C" DLL_EXPORT UClass* autoclass##TClass;\
		DLL_EXPORT UClass* autoclass##TClass = TClass::StaticClass();
#else
	#define IMPLEMENT_CLASS(TClass) \
		UClass TClass::PrivateStaticClass \
		( \
			EC_NativeConstructor, \
			sizeof(TClass), \
			TClass::StaticClassFlags, \
			TClass::Super::StaticClass(), \
			TClass::WithinClass::StaticClass(), \
			FGuid(TClass::GUID1,TClass::GUID2,TClass::GUID3,TClass::GUID4), \
			&TEXT(#TClass)[1], \
			::GPackage,			\
			StaticConfigName(), \
			RF_Public | RF_Standalone | RF_Transient | RF_Native, \
			(void(*)(void*))TClass::InternalConstructor, \
			(void(UObject::*)())&TClass::StaticConstructor \
		); \
		DLL_EXPORT {UClass* autoclass##TClass = TClass::StaticClass();}
#endif

// Define the package of the current DLL being compiled.
#define IMPLEMENT_PACKAGE(pkg) \
const TCHAR* GPackage = TEXT(#pkg); \
	IMPLEMENT_PACKAGE_PLATFORM(pkg)

/*-----------------------------------------------------------------------------
	UObject.
-----------------------------------------------------------------------------*/

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack(push,OBJECT_ALIGNMENT)
#endif

//
// stijn: Used during object construction
//
class UCompressedPointer
{
#define MAX_TEMPORARY_POINTERS (64 * 1024)
public:
	static const void* Pointers[MAX_TEMPORARY_POINTERS];
	static INT Store(const void* TempPointer);
	static const void* LoadAndInvalidate(INT PointerIndex);
};

//
// The base class of all objects.
//
class CORE_API UObject : public FUnknown
{
	// Declarations.
	DECLARE_BASE_CLASS(UObject,UObject,CLASS_Abstract,Core)
	typedef UObject WithinClass;
	enum {GUID1=0,GUID2=0,GUID3=0,GUID4=0};
	static const TCHAR* StaticConfigName() {return TEXT("System");}	

	// Friends.
	friend class FObjectIterator;
	friend class ULinkerLoad;
	friend class ULinkerSave;
	friend class UPackageMap;
	friend class FArchiveTagUsed;
	friend struct FObjectImport;
	friend struct FObjectExport;
	friend class UStruct;
	friend struct FFrame;
	friend class URegistry;
	friend class UEngine;
	friend class UClass;
	friend class UEditorEngine;

private:	
	// Internal per-object variables.
	INT						Index;				// Index of object into table.
	FORCE_64BIT_PADDING_DWORD;
	UObject*				HashNext;			// Next object in this hash bin.
	FStateFrame*			StateFrame;			// Main script execution stack.
	ULinkerLoad*			_Linker;			// Linker it came from, or NULL if none.

	// Index of this object in the linker's export map.
	// Note from stijn: During construction, a UObject's _LinkerIndex is overwritten with the
	// current value of GAutoRegister (i.e., a UObject pointer). The actual linker index is
	// calculated and stored when the object registers (in UObject::ProcessRegistrants)
	INT						_LinkerIndex;
	FORCE_64BIT_PADDING_DWORD;

	// Object this object resides in.
	// Note from stijn: During construction, a UObject's Outer is overwritten with a pointer
	// to its GPackage string. The actual Outer object pointer is derived and stored during
	// object registration (in UObject::Register).
	UObject*				Outer;

	DWORD					ObjectFlags;		// Private EObjectFlags used by object manager.

	// Name of the object.
	// Note from stijn: During construction, a UObject's Name is overwritten with a pointer
	// to a string containing its base name. During registration, the engine looks up
	// the base name in the name table and writes the actual FName value (in UObject::Register).
	FName					Name;				
	UClass*					Class;	  			// Class the object belongs to.

private:
	// Private systemwide variables.
	static UBOOL			GObjInitialized;	// Whether initialized.
	static UBOOL            GObjNoRegister;		// Registration disable.
	static INT				GObjBeginLoadCount;	// Count for BeginLoad multiple loads.
	static INT				GObjRegisterCount;  // ProcessRegistrants entry counter.
	static INT				GImportCount;		// Imports for EndLoad optimization.
	static UINT				GObjRevision;		// Global revision number which increased each time when new object created.
	static UObject*			GObjHash[4096];		// Object hash.
	static UObject*			GAutoRegister;		// Objects to automatically register.
	static TArray<UObject*> GObjLoaded;			// Objects that might need preloading.
	static TArray<UObject*>	GObjRoot;			// Top of active object graph.
	static TArray<UObject*>	GObjObjects;		// List of all objects.
	static TArray<INT>      GObjAvailable;		// Available object indices.
	static TArray<UObject*>	GObjLoaders;		// Array of loaders.
	static UPackage*		GObjTransientPkg;	// Transient package.
	static TCHAR			GObjCachedLanguage[32]; // Language;
	static FStringNoInit	TraceIndent;		// old field, not used anymore
	static TArray<UObject*> GObjRegistrants;		// Registrants during ProcessRegistrants call.
	static TArray<FPreferencesInfo> GObjPreferences; // Prefereces cache.
	static TArray<FRegistryObjectInfo> GObjDrivers; // Drivers cache.
	static TArray<FRegistryPropertyInfo> GObjProperties; // Properties cache
	static TMultiMap<FName,FName>* GObjPackageRemap; // Remap table for loading renamed packages.
	static TCHAR GLanguage[64];
	static TArray<INT> GObjHashes;
	static TArray<UObject*> GObjDangerousRefs;  // Objects possibly containing dangerous refs	

	// Private functions.
	void SetLinker( ULinkerLoad* L, INT I );
	void AddObject( INT Index );

	// Private systemwide functions.
	static ULinkerLoad* GetLoader( INT i );
	static FName MakeUniqueObjectName( UObject* Outer, UClass* Class );
	static UBOOL ResolveName( UObject*& Outer, const TCHAR*& Name, UBOOL Create, UBOOL Throw );
	static void SafeLoadError( DWORD LoadFlags, const TCHAR* Error, const TCHAR* Fmt, ... );
	static void PurgeGarbage();
	static void CacheDrivers( UBOOL ForceRefresh );

protected:
	inline void UTrace(UObject* InMethod, UBOOL InBegin, UBOOL IsStateCode);

public:
	// Constructors.
	UObject();
	UObject( const UObject& Src );
	UObject( ENativeConstructor, UClass* InClass, const TCHAR* InName, const TCHAR* InPackageName, DWORD InFlags );
	UObject( EStaticConstructor, const TCHAR* InName, const TCHAR* InPackageName, DWORD InFlags );
	UObject( EInPlaceConstructor, UClass* InClass, UObject* InOuter, FName InName, DWORD InFlags );
	UObject& operator=( const UObject& );
	void StaticConstructor();
	static void InternalConstructor( void* X )
		{ new( (EInternal*)X )UObject; }

	// Destructors.
	virtual ~UObject() noexcept(false);
	void operator delete( void* Object, size_t Size )
		{guard(UObject::operator delete); appFree( Object ); unguard;}

	// FUnknown interface.
	virtual DWORD STDCALL QueryInterface( const FGuid& RefIID, void** InterfacePtr );
	virtual DWORD STDCALL AddRef();
	virtual DWORD STDCALL Release();

	// UObject interface.
	virtual void ProcessEvent( UFunction* Function, void* Parms, void* Result=NULL );
	virtual void ProcessState( FLOAT DeltaSeconds );
	virtual UBOOL ProcessRemoteFunction( UFunction* Function, void* Parms, FFrame* Stack );
	virtual void Modify();
	virtual void PostLoad();
	virtual void Destroy();
	virtual void Serialize( FArchive& Ar );
	virtual UBOOL IsPendingKill() {return 0;}
	virtual EGotoState GotoState( FName State );
	virtual INT GotoLabel( FName Label );
	virtual void InitExecution();
	virtual void ShutdownAfterError();
	virtual void PostEditChange();
	virtual void CallFunction( FFrame& TheStack, RESULT_DECL, UFunction* Function );
	virtual UBOOL ScriptConsoleExec( const TCHAR* Cmd, FOutputDevice& Ar, UObject* Executor );
	virtual void Register();
	virtual void LanguageChange();

	// Systemwide functions.
	static UObject* StaticFindObject( UClass* Class, UObject* InOuter, const TCHAR* Name, UBOOL ExactClass=0 );
	static UObject* StaticFindObjectChecked( UClass* Class, UObject* InOuter, const TCHAR* Name, UBOOL ExactClass=0 );
	static UObject* StaticLoadObject( UClass* Class, UObject* InOuter, const TCHAR* Name, const TCHAR* Filename, DWORD LoadFlags, UPackageMap* Sandbox );
	static UClass* StaticLoadClass( UClass* BaseClass, UObject* InOuter, const TCHAR* Name, const TCHAR* Filename, DWORD LoadFlags, UPackageMap* Sandbox );
	static UObject* StaticAllocateObject( UClass* Class, UObject* InOuter=(UObject*)GetTransientPackage(), FName Name=NAME_None, DWORD SetFlags=0, UObject* Template=NULL, FOutputDevice* Error=GError, UObject* Ptr=NULL );
	static UObject* StaticConstructObject( UClass* Class, UObject* InOuter=(UObject*)GetTransientPackage(), FName Name=NAME_None, DWORD SetFlags=0, UObject* Template=NULL, FOutputDevice* Error=GError );
	static void StaticInit();
	static void StaticExit();
	static UBOOL StaticExec( const TCHAR* Cmd, FOutputDevice& Ar=*GLog );
	static void StaticTick();
	static UObject* LoadPackage( UObject* InOuter, const TCHAR* Filename, DWORD LoadFlags );
	static UBOOL SavePackage( UObject* InOuter, UObject* Base, DWORD TopLevelFlags, const TCHAR* Filename, FOutputDevice* Error=GError, ULinkerLoad* Conform=NULL );
	static void CollectGarbage( DWORD KeepFlags );
	static void SerializeRootSet( FArchive& Ar, DWORD KeepFlags, DWORD RequiredFlags );
	static UBOOL IsReferenced( UObject*& Res, DWORD KeepFlags, UBOOL IgnoreReference );
	static UBOOL AttemptDelete( UObject*& Res, DWORD KeepFlags, UBOOL IgnoreReference );
	static void BeginLoad();
	static void EndLoad();
	static void InitProperties( BYTE* Data, INT DataCount, UClass* DefaultsClass, BYTE* Defaults, INT DefaultsCount );
	static void ExitProperties( BYTE* Data, UClass* Class );
	static void ResetLoaders( UObject* InOuter, UBOOL DynamicOnly, UBOOL ForceLazyLoad );
	static UPackage* CreatePackage( UObject* InOuter, const TCHAR* PkgName );
	static UBOOL DetachEmptyPackage(UPackage* Package, ULinkerLoad*& Linker);
	static ULinkerLoad* GetPackageLinker( UObject* InOuter, const TCHAR* Filename, DWORD LoadFlags, UPackageMap* Sandbox, FGuid* CompatibleGuid );
	static void StaticShutdownAfterError();
	static UObject* GetIndexedObject( INT Index );
	static void GlobalSetProperty( const TCHAR* Value, UClass* Class, UProperty* Property, INT Offset, UBOOL Immediate );
	static void ExportProperties( FOutputDevice& Out, UClass* ObjectClass, BYTE* Object, INT Indent, UClass* DiffClass, BYTE* Diff );
	static void ExportPropertiesCleanup( UClass* ObjectClass, BYTE* Object ); // Higor: removes RF_TagImp from references
	static UBOOL GetIniFilenameAndSectionForObject(const UObject* Object, FString& IniFilename, FString& IniSection);
	static UBOOL GetIniFilenameAndSectionForClass(const UClass* Class, FString& IniFilename, FString& IniSection);
	static void ResetConfig( UClass* Class, const TCHAR* PropName=NULL );
	static void GetRegistryObjects( TArray<FRegistryObjectInfo>& Results, UClass* Class, UClass* MetaClass, UBOOL ForceRefresh );
	static void GetPreferences( TArray<FPreferencesInfo>& Results, const TCHAR* Category, UBOOL ForceRefresh );
	static void GetRegistryProperties( TArray<FRegistryPropertyInfo>& Results, UClass* Class, UBOOL ForceRefresh );
	static UBOOL GetInitialized();
	static UPackage* GetTransientPackage();
	static void VerifyLinker( ULinkerLoad* Linker );
	static void ProcessRegistrants();
	static void BindPackage( UPackage* Pkg );
	static const TCHAR* GetLanguage();
	static void SetLanguage( const TCHAR* LanguageExt );
	static INT GetObjectHash( FName ObjName, INT Outer )
	{
		return (ObjName.GetIndex() ^ Outer) & (ARRAY_COUNT(GObjHash)-1);
	}
	static void NoteAssignment(UObject* DestObject, UProperty* DestProperty, BYTE* DestPropertyAddress);
	static void UProfile(UObject* InMethod, UBOOL InBegin);
	static void UTrace(UBOOL bNewUTracing);
	static void UTrack(const TCHAR* TrackObject, const TCHAR* TrackProperty);
	static UINT GetGlobalRevision()
	{
		return GObjRevision;
	}

	// Functions.
	void AddToRoot();
	void RemoveFromRoot();
	// stijn: Do not use GetFullName or GetPathName directly! use FObjectFullName(Obj) or FObjectObjectPathName(Obj) instead.
	const TCHAR* GetFullName( TCHAR* Str=NULL ) const; 
	const TCHAR* GetPathName( UObject* StopOuter=NULL, TCHAR* Str=NULL ) const;
	FString GetFullNameSafe() const;
	FString GetPathNameSafe(UObject* StopOuter = NULL) const;
	UBOOL IsValid();
	void ConditionalRegister();
	UBOOL ConditionalDestroy();
	void ConditionalPostLoad();
	void ConditionalShutdownAfterError();
	UBOOL IsA( UClass* SomeBaseClass ) const;
	UBOOL IsIn( UObject* SomeOuter ) const;
	UBOOL IsProbing( FName ProbeName );
	void Rename( const TCHAR* NewName=NULL );
	UField* FindObjectFieldOfClass(FName InName, UClass* OfClass = NULL, UBOOL Global = 0) const;
	UField* FindObjectField( FName InName, UBOOL Global=0 );
	UFunction* FindFunction( FName InName, UBOOL Global=0 );
	UFunction* FindFunctionChecked( FName InName, UBOOL Global=0 );
	UState* FindState( FName InName );
	void SaveConfig( DWORD Flags=CPF_Config, const TCHAR* Filename=NULL );
	void LoadConfig( UBOOL Propagate=0, UClass* Class=NULL, const TCHAR* Filename=NULL, UBOOL Force=0 );	// rjp
	void ClearConfig( UClass* Class=NULL, const TCHAR* PropName=NULL );										// rjp
	UBOOL GetDefaultValue(UObject* Object, const TCHAR* PropName, INT ArrayIndex, FString& Out);
	void LoadLocalized( UBOOL Propagate=0, UClass* Class=NULL );
	void InitClassDefaultObject( UClass* InClass );
	void ProcessInternal( FFrame& TheStack, void*const Result );
	void ParseParms( const TCHAR* Parms );
	void HashObject();
	void UnhashObject(INT OuterIndex);
	UBOOL ExecHook(FFrame& TheStack, const TCHAR* Cmd, FOutputDevice& Ar);

	//
	// stijn: OldUnreal Compatibility Checking
	// 
	static BOOL IsNetCompatibleImport(UObject* Object, UPackage*& OuterPackage, ULinkerLoad*& OuterLinker, INT& SourceIndex, FName ClassName, FName PackageName, BOOL Warn);
	static BOOL IsNetCompatibleObject(UObject* Object, BOOL Warn);
	static BOOL IsNetCompatibleFunctionName(UFunction* Func, BOOL Warn);
	static UFunction* GetNetCompatibleFunction(UFunction* Func, BOOL Warn);
	static UFunction* GetNetCompatibleSuperFunction(UFunction* Func, BOOL Warn);
	static BOOL IsNetCompatibleFunctionCall(UFunction* Callee, INT ParamCount);
	static void ResetNetCompatibleWarnings();

	// stijn: UFunction redirection support
#if OLDUNREAL_USCRIPT_HOOKING
	static void RedirectStaticInit();
	static void RedirectStaticExit();
	static BOOL RegisterRedirect(UFunction* From, UFunction* To);
	static UFunction* DoRedirect(UFunction* InFunction);
	static BOOL RemoveRedirect(UFunction* From);
	static void ResetRedirects();
#endif

	// Accessors.
	UClass* GetClass() const
	{
		return Class;
	}
	void SetClass(UClass* NewClass)
	{
		Class = NewClass;
	}
	DWORD GetFlags() const
	{
		return ObjectFlags;
	}
	void SetFlags( DWORD NewFlags )
	{
		ObjectFlags |= NewFlags;
		checkSlow(Name!=NAME_None || !(ObjectFlags&RF_Public));
	}
	void ClearFlags( DWORD NewFlags )
	{
		ObjectFlags &= ~NewFlags;
		checkSlow(Name!=NAME_None || !(ObjectFlags&RF_Public));
	}
	const TCHAR* GetName() const
	{
		return *Name;
	}
	const FName GetFName() const
	{
		return Name;
	}
	UObject* GetOuter() const
	{
		return Outer;
	}
	DWORD GetIndex() const
	{
		return Index;
	}
	ULinkerLoad* GetLinker()
	{
		return _Linker;
	}
	INT GetLinkerIndex()
	{
		return _LinkerIndex;
	}
	FStateFrame* GetStateFrame()
	{
		return StateFrame;
	}

	// UnrealScript intrinsics.
	#define DECLARE_FUNCTION(func) void func( FFrame& TheStack, RESULT_DECL );
	DECLARE_FUNCTION(execUndefined)
	DECLARE_FUNCTION(execLocalVariable)
	DECLARE_FUNCTION(execInstanceVariable)
	DECLARE_FUNCTION(execDefaultVariable)
	DECLARE_FUNCTION(execArrayElement)
	DECLARE_FUNCTION(execDynArrayLength)
	DECLARE_FUNCTION(execDynArrayElement)
	DECLARE_FUNCTION(execDynArrayInsert)
	DECLARE_FUNCTION(execDynArrayRemove)
	DECLARE_FUNCTION(execBoolVariable)
	DECLARE_FUNCTION(execClassDefaultVariable)
	DECLARE_FUNCTION(execEndFunctionParms)
	DECLARE_FUNCTION(execNothing)
	DECLARE_FUNCTION(execStop)
	DECLARE_FUNCTION(execEndCode)
	DECLARE_FUNCTION(execSwitch)
	DECLARE_FUNCTION(execCase)
	DECLARE_FUNCTION(execJump)
	DECLARE_FUNCTION(execJumpIfNot)
	DECLARE_FUNCTION(execAssert)
	DECLARE_FUNCTION(execGotoLabel)
	DECLARE_FUNCTION(execLet)
	DECLARE_FUNCTION(execLetBool)
	DECLARE_FUNCTION(execEatString)
	DECLARE_FUNCTION(execSelf)
	DECLARE_FUNCTION(execContext)
	DECLARE_FUNCTION(execVirtualFunction)
	DECLARE_FUNCTION(execFinalFunction)
	DECLARE_FUNCTION(execGlobalFunction)
	DECLARE_FUNCTION(execStructCmpEq)
	DECLARE_FUNCTION(execStructCmpNe)
	DECLARE_FUNCTION(execStructMember)
	DECLARE_FUNCTION(execIntConst)
	DECLARE_FUNCTION(execFloatConst)
	DECLARE_FUNCTION(execStringConst)
	DECLARE_FUNCTION(execUnicodeStringConst)
	DECLARE_FUNCTION(execObjectConst)
	DECLARE_FUNCTION(execNameConst)
	DECLARE_FUNCTION(execByteConst)
	DECLARE_FUNCTION(execIntZero)
	DECLARE_FUNCTION(execIntOne)
	DECLARE_FUNCTION(execTrue)
	DECLARE_FUNCTION(execFalse)
	DECLARE_FUNCTION(execNoObject)
	DECLARE_FUNCTION(execIntConstByte)
	DECLARE_FUNCTION(execDynamicCast)
	DECLARE_FUNCTION(execMetaCast)
	DECLARE_FUNCTION(execByteToInt)
	DECLARE_FUNCTION(execByteToBool)
	DECLARE_FUNCTION(execByteToFloat)
	DECLARE_FUNCTION(execByteToString)
	DECLARE_FUNCTION(execIntToByte)
	DECLARE_FUNCTION(execIntToBool)
	DECLARE_FUNCTION(execIntToFloat)
	DECLARE_FUNCTION(execIntToString)
	DECLARE_FUNCTION(execBoolToByte)
	DECLARE_FUNCTION(execBoolToInt)
	DECLARE_FUNCTION(execBoolToFloat)
	DECLARE_FUNCTION(execBoolToString)
	DECLARE_FUNCTION(execFloatToByte)
	DECLARE_FUNCTION(execFloatToInt)
	DECLARE_FUNCTION(execFloatToBool)
	DECLARE_FUNCTION(execFloatToString)
	DECLARE_FUNCTION(execRotationConst)
	DECLARE_FUNCTION(execVectorConst)
	DECLARE_FUNCTION(execStringToVector)
	DECLARE_FUNCTION(execStringToRotator)
	DECLARE_FUNCTION(execVectorToBool)
	DECLARE_FUNCTION(execVectorToString)
	DECLARE_FUNCTION(execVectorToRotator)
	DECLARE_FUNCTION(execRotatorToBool)
	DECLARE_FUNCTION(execRotatorToVector)
	DECLARE_FUNCTION(execRotatorToString)
    DECLARE_FUNCTION(execRotRand)
    DECLARE_FUNCTION(execGetUnAxes)
    DECLARE_FUNCTION(execGetAxes)
    DECLARE_FUNCTION(execSubtractEqual_RotatorRotator)
    DECLARE_FUNCTION(execAddEqual_RotatorRotator)
    DECLARE_FUNCTION(execSubtract_RotatorRotator)
    DECLARE_FUNCTION(execAdd_RotatorRotator)
    DECLARE_FUNCTION(execDivideEqual_RotatorFloat)
    DECLARE_FUNCTION(execMultiplyEqual_RotatorFloat)
    DECLARE_FUNCTION(execDivide_RotatorFloat)
    DECLARE_FUNCTION(execMultiply_FloatRotator)
    DECLARE_FUNCTION(execMultiply_RotatorFloat)
    DECLARE_FUNCTION(execNotEqual_RotatorRotator)
    DECLARE_FUNCTION(execEqualEqual_RotatorRotator)
    DECLARE_FUNCTION(execMirrorVectorByNormal)
    DECLARE_FUNCTION(execVRand)
	DECLARE_FUNCTION(execRandomSpreadVector)
    DECLARE_FUNCTION(execInvert)
    DECLARE_FUNCTION(execNormal)
    DECLARE_FUNCTION(execVSize)
    DECLARE_FUNCTION(execSubtractEqual_VectorVector)
    DECLARE_FUNCTION(execAddEqual_VectorVector)
    DECLARE_FUNCTION(execDivideEqual_VectorFloat)
    DECLARE_FUNCTION(execMultiplyEqual_VectorVector)
    DECLARE_FUNCTION(execMultiplyEqual_VectorFloat)
    DECLARE_FUNCTION(execCross_VectorVector)
    DECLARE_FUNCTION(execDot_VectorVector)
    DECLARE_FUNCTION(execNotEqual_VectorVector)
    DECLARE_FUNCTION(execEqualEqual_VectorVector)
    DECLARE_FUNCTION(execGreaterGreater_VectorRotator)
    DECLARE_FUNCTION(execLessLess_VectorRotator)
    DECLARE_FUNCTION(execSubtract_VectorVector)
    DECLARE_FUNCTION(execAdd_VectorVector)
    DECLARE_FUNCTION(execDivide_VectorFloat)
    DECLARE_FUNCTION(execMultiply_VectorVector)
    DECLARE_FUNCTION(execMultiply_FloatVector)
    DECLARE_FUNCTION(execMultiply_VectorFloat)
    DECLARE_FUNCTION(execSubtract_PreVector)
	DECLARE_FUNCTION(execOrthoRotation);
	DECLARE_FUNCTION(execNormalize);
	DECLARE_FUNCTION(execObjectToBool)
	DECLARE_FUNCTION(execObjectToString)
	DECLARE_FUNCTION(execNameToBool)
	DECLARE_FUNCTION(execNameToString)
	DECLARE_FUNCTION(execStringToByte)
	DECLARE_FUNCTION(execStringToInt)
	DECLARE_FUNCTION(execStringToBool)
	DECLARE_FUNCTION(execStringToFloat)
	DECLARE_FUNCTION(execNot_PreBool)
	DECLARE_FUNCTION(execEqualEqual_BoolBool)
	DECLARE_FUNCTION(execNotEqual_BoolBool)
	DECLARE_FUNCTION(execAndAnd_BoolBool)
	DECLARE_FUNCTION(execXorXor_BoolBool)
	DECLARE_FUNCTION(execOrOr_BoolBool)
	DECLARE_FUNCTION(execMultiplyEqual_ByteByte)
	DECLARE_FUNCTION(execDivideEqual_ByteByte)
	DECLARE_FUNCTION(execAddEqual_ByteByte)
	DECLARE_FUNCTION(execSubtractEqual_ByteByte)
	DECLARE_FUNCTION(execAddAdd_PreByte)
	DECLARE_FUNCTION(execSubtractSubtract_PreByte)
	DECLARE_FUNCTION(execAddAdd_Byte)
	DECLARE_FUNCTION(execSubtractSubtract_Byte)
	DECLARE_FUNCTION(execComplement_PreInt)
	DECLARE_FUNCTION(execSubtract_PreInt)
	DECLARE_FUNCTION(execMultiply_IntInt)
	DECLARE_FUNCTION(execDivide_IntInt)
	DECLARE_FUNCTION(execAdd_IntInt)
	DECLARE_FUNCTION(execSubtract_IntInt)
	DECLARE_FUNCTION(execLessLess_IntInt)
	DECLARE_FUNCTION(execGreaterGreater_IntInt)
	DECLARE_FUNCTION(execGreaterGreaterGreater_IntInt)
	DECLARE_FUNCTION(execLess_IntInt)
	DECLARE_FUNCTION(execGreater_IntInt)
	DECLARE_FUNCTION(execLessEqual_IntInt)
	DECLARE_FUNCTION(execGreaterEqual_IntInt)
	DECLARE_FUNCTION(execEqualEqual_IntInt)
	DECLARE_FUNCTION(execNotEqual_IntInt)
	DECLARE_FUNCTION(execAnd_IntInt)
	DECLARE_FUNCTION(execXor_IntInt)
	DECLARE_FUNCTION(execOr_IntInt)
	DECLARE_FUNCTION(execMultiplyEqual_IntFloat)
	DECLARE_FUNCTION(execDivideEqual_IntFloat)
	DECLARE_FUNCTION(execAddEqual_IntInt)
	DECLARE_FUNCTION(execSubtractEqual_IntInt)
	DECLARE_FUNCTION(execAddAdd_PreInt)
	DECLARE_FUNCTION(execSubtractSubtract_PreInt)
	DECLARE_FUNCTION(execAddAdd_Int)
	DECLARE_FUNCTION(execSubtractSubtract_Int)
	DECLARE_FUNCTION(execRand)
	DECLARE_FUNCTION(execMin)
	DECLARE_FUNCTION(execMax)
	DECLARE_FUNCTION(execClamp)
	DECLARE_FUNCTION(execSubtract_PreFloat)
	DECLARE_FUNCTION(execMultiplyMultiply_FloatFloat)
	DECLARE_FUNCTION(execMultiply_FloatFloat)
	DECLARE_FUNCTION(execDivide_FloatFloat)
	DECLARE_FUNCTION(execPercent_FloatFloat)
	DECLARE_FUNCTION(execAdd_FloatFloat)
	DECLARE_FUNCTION(execSubtract_FloatFloat)
	DECLARE_FUNCTION(execLess_FloatFloat)
	DECLARE_FUNCTION(execGreater_FloatFloat)
	DECLARE_FUNCTION(execLessEqual_FloatFloat)
	DECLARE_FUNCTION(execGreaterEqual_FloatFloat)
	DECLARE_FUNCTION(execEqualEqual_FloatFloat)
	DECLARE_FUNCTION(execNotEqual_FloatFloat)
	DECLARE_FUNCTION(execComplementEqual_FloatFloat)
	DECLARE_FUNCTION(execMultiplyEqual_FloatFloat)
	DECLARE_FUNCTION(execDivideEqual_FloatFloat)
	DECLARE_FUNCTION(execAddEqual_FloatFloat)
	DECLARE_FUNCTION(execSubtractEqual_FloatFloat)
	DECLARE_FUNCTION(execAbs)
	DECLARE_FUNCTION(execDebugInfo) //DEBUGGER
	DECLARE_FUNCTION(execSin)
	DECLARE_FUNCTION(execCos)
	DECLARE_FUNCTION(execTan)
	DECLARE_FUNCTION(execAtan)
	DECLARE_FUNCTION(execExp)
	DECLARE_FUNCTION(execLoge)
	DECLARE_FUNCTION(execSqrt)
	DECLARE_FUNCTION(execSquare)
	DECLARE_FUNCTION(execFRand)
	DECLARE_FUNCTION(execFMin)
	DECLARE_FUNCTION(execFMax)
	DECLARE_FUNCTION(execFClamp)
	DECLARE_FUNCTION(execLerp)
	DECLARE_FUNCTION(execSmerp)
	DECLARE_FUNCTION(execConcat_StringString)
	DECLARE_FUNCTION(execConcatEqual_StringString)
	DECLARE_FUNCTION(execAt_StringString)
	DECLARE_FUNCTION(execAtEqual_StringString)
	DECLARE_FUNCTION(execLess_StringString)
	DECLARE_FUNCTION(execGreater_StringString)
	DECLARE_FUNCTION(execLessEqual_StringString)
	DECLARE_FUNCTION(execGreaterEqual_StringString)
	DECLARE_FUNCTION(execEqualEqual_StringString)
	DECLARE_FUNCTION(execNotEqual_StringString)
	DECLARE_FUNCTION(execComplementEqual_StringString)
	DECLARE_FUNCTION(execSubtractEqual_StringString)
	DECLARE_FUNCTION(execLen)
	DECLARE_FUNCTION(execInStr)
	DECLARE_FUNCTION(execMid)
	DECLARE_FUNCTION(execLeft)
	DECLARE_FUNCTION(execRight)
	DECLARE_FUNCTION(execCaps)
	DECLARE_FUNCTION(execLocs)
	DECLARE_FUNCTION(execChr)
	DECLARE_FUNCTION(execAsc)
	DECLARE_FUNCTION(execRepl)
	DECLARE_FUNCTION(execEqualEqual_ObjectObject)
	DECLARE_FUNCTION(execNotEqual_ObjectObject)
	DECLARE_FUNCTION(execEqualEqual_NameName)
	DECLARE_FUNCTION(execNotEqual_NameName)
	DECLARE_FUNCTION(execLog)
	DECLARE_FUNCTION(execWarn)
	DECLARE_FUNCTION(execNew)
	DECLARE_FUNCTION(execRandRange)
	DECLARE_FUNCTION(execClassIsChildOf)
	DECLARE_FUNCTION(execClassContext)
	DECLARE_FUNCTION(execGoto)
	DECLARE_FUNCTION(execGotoState)
	DECLARE_FUNCTION(execIsA)
	DECLARE_FUNCTION(execEnable)
	DECLARE_FUNCTION(execDisable)
	DECLARE_FUNCTION(execIterator)
	DECLARE_FUNCTION(execLocalize)
	DECLARE_FUNCTION(execNativeParm)
	DECLARE_FUNCTION(execGetPropertyText)
	DECLARE_FUNCTION(execSetPropertyText)
	DECLARE_FUNCTION(execSaveConfig)
	DECLARE_FUNCTION(execStaticSaveConfig)
	DECLARE_FUNCTION(execResetConfig)
	DECLARE_FUNCTION(execClearConfig)	// rjp
	DECLARE_FUNCTION(execStaticClearConfig)
	DECLARE_FUNCTION(execGetDefaultValue)
	DECLARE_FUNCTION(execGetEnum)
	DECLARE_FUNCTION(execDynamicLoadObject)
	DECLARE_FUNCTION(execIsInState)
	DECLARE_FUNCTION(execGetStateName)
	DECLARE_FUNCTION(execLineNumber)
	DECLARE_FUNCTION(execSetUTracing)
	DECLARE_FUNCTION(execIsUTracing)
	DECLARE_FUNCTION(execHighNative0)
	DECLARE_FUNCTION(execHighNative1)
	DECLARE_FUNCTION(execHighNative2)
	DECLARE_FUNCTION(execHighNative3)
	DECLARE_FUNCTION(execHighNative4)
	DECLARE_FUNCTION(execHighNative5)
	DECLARE_FUNCTION(execHighNative6)
	DECLARE_FUNCTION(execHighNative7)
	DECLARE_FUNCTION(execHighNative8)
	DECLARE_FUNCTION(execHighNative9)
	DECLARE_FUNCTION(execHighNative10)
	DECLARE_FUNCTION(execHighNative11)
	DECLARE_FUNCTION(execHighNative12)
	DECLARE_FUNCTION(execHighNative13)
	DECLARE_FUNCTION(execHighNative14)
	DECLARE_FUNCTION(execHighNative15)
	//DECLARE_FUNCTION(execUnloadPackage)

	// UnrealScript calling stubs.
    void eventBeginState()
    {
        ProcessEvent(FindFunctionChecked(NAME_BeginState),NULL);
    }
    void eventEndState()
    {
        ProcessEvent(FindFunctionChecked(NAME_EndState),NULL);
    }
};

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (pop)
#endif

inline FString FObjectName(const UObject* Object)
{
	if (!Object)
		return TEXT("None");
	return Object->GetName();
}

inline FString FObjectPathName(const UObject* Object)
{
	if (!Object)
		return TEXT("None");
	return Object->GetPathNameSafe();
}

inline FString FObjectFullName(const UObject* Object, UBOOL DoubleNone = 0)
{
	if (!Object)
		return DoubleNone ? TEXT("None None") : TEXT("None");
	return Object->GetFullNameSafe();
}

/*----------------------------------------------------------------------------
	Core templates.
----------------------------------------------------------------------------*/

// Hash function.
template <typename UINT=DWORD> inline UINT GetTypeHash( const UObject* A )
{
	return (UINT)(A ? A->GetIndex() : 0);
}

// Parse an object name in the input stream.
template< class T > UBOOL ParseObject( const TCHAR* Stream, const TCHAR* Match, T*& Obj, UObject* Outer )
{
	return ParseObject( Stream, Match, T::StaticClass(), *(UObject **)&Obj, Outer );
}

// Find an optional object.
template< class T > T* FindObject( UObject* Outer, const TCHAR* Name, UBOOL ExactClass=0 )
{
	return (T*)UObject::StaticFindObject( T::StaticClass(), Outer, Name, ExactClass );
}

// Find an object, no failure allowed.
template< class T > T* FindObjectChecked( UObject* Outer, const TCHAR* Name, UBOOL ExactClass=0 )
{
	return (T*)UObject::StaticFindObjectChecked( T::StaticClass(), Outer, Name, ExactClass );
}

// Dynamically cast an object type-safely.
template< class T > T* Cast( UObject* Src )
{
	return Src && Src->IsA(T::StaticClass()) ? (T*)Src : NULL;
}
template< class T, class U > T* CastChecked( U* Src )
{
	if( !Src || !Src->IsA(T::StaticClass()) )
		appErrorf( TEXT("Cast of %s to %s failed"), Src->GetFullName(), T::StaticClass()->GetName() );
	return (T*)Src;
}

// Load an object.
template< class T > T* LoadObject( UObject* Outer, const TCHAR* Name, const TCHAR* Filename, DWORD LoadFlags, UPackageMap* Sandbox )
{
	return (T*)UObject::StaticLoadObject( T::StaticClass(), Outer, Name, Filename, LoadFlags, Sandbox );
}

// Load a class object.
template< class T > UClass* LoadClass( UObject* Outer, const TCHAR* Name, const TCHAR* Filename, DWORD LoadFlags, UPackageMap* Sandbox )
{
	return UObject::StaticLoadClass( T::StaticClass(), Outer, Name, Filename, LoadFlags, Sandbox );
}

// Get default object of a class.
template< class T > T* GetDefault()
{
	return (T*)&T::StaticClass()->Defaults(0);
}

/*----------------------------------------------------------------------------
	Object iterators.
----------------------------------------------------------------------------*/

//
// Class for iterating through all objects.
//
class FObjectIterator
{
public:
	FObjectIterator(UClass* InClass = UObject::StaticClass());
	void operator++();
	UObject* operator*();
	UObject* operator->();
	operator UBOOL();
protected:
	UClass* Class;
	INT Index;
	const UBOOL IsClass;
};

//
// Class for iterating through all objects which inherit from a
// specified base class.
//
template< class T > class TObjectIterator : public FObjectIterator
{
public:
	TObjectIterator()
	:	FObjectIterator( T::StaticClass() )
	{}
	T* operator* ()
	{
		return (T*)FObjectIterator::operator*();
	}
	T* operator-> ()
	{
		return (T*)FObjectIterator::operator->();
	}
};

#define AUTO_INITIALIZE_REGISTRANTS \
	UObject::StaticClass(); \
	UClass::StaticClass(); \
	USubsystem::StaticClass(); \
	USystem::StaticClass(); \
	UProperty::StaticClass(); \
	UStructProperty::StaticClass(); \
	UField::StaticClass(); \
	UMapProperty::StaticClass(); \
	UArrayProperty::StaticClass(); \
	UFixedArrayProperty::StaticClass(); \
	UStrProperty::StaticClass(); \
	UNameProperty::StaticClass(); \
	UClassProperty::StaticClass(); \
	UObjectProperty::StaticClass(); \
	UFloatProperty::StaticClass(); \
	UBoolProperty::StaticClass(); \
	UIntProperty::StaticClass(); \
	UByteProperty::StaticClass(); \
	ULanguage::StaticClass(); \
	UTextBufferFactory::StaticClass(); \
	UFactory::StaticClass(); \
	UPackage::StaticClass(); \
	UCommandlet::StaticClass(); \
	ULinkerSave::StaticClass(); \
	ULinker::StaticClass(); \
	ULinkerLoad::StaticClass(); \
	UEnum::StaticClass(); \
	UTextBuffer::StaticClass(); \
	UPackageMap::StaticClass(); \
	UConst::StaticClass(); \
	UFunction::StaticClass(); \
	UStruct::StaticClass(); \
	UExporter::StaticClass(); \
	UState::StaticClass(); \
	UPointerProperty::StaticClass(); \
	URegistry::StaticClass();


/*----------------------------------------------------------------------------
	The End.
----------------------------------------------------------------------------*/
