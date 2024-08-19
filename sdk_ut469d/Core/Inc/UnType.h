/*=============================================================================
	UnType.h: Unreal engine base type definitions.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/*-----------------------------------------------------------------------------
	UProperty.
-----------------------------------------------------------------------------*/

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack(push,OBJECT_ALIGNMENT)
#endif

// Property exporting flags.
enum EPropertyPortFlags
{
	PPF_Localized = 1,
	PPF_Delimited = 2,
	PPF_Compact   = 4
};

//
// An UnrealScript variable.
//
class CORE_API UProperty : public UField
{
	DECLARE_ABSTRACT_CLASS(UProperty,UField,0,Core)
	DECLARE_WITHIN(UField)

	// Persistent variables.
	INT			ArrayDim;
	INT			ElementSize;
	DWORD		PropertyFlags;
	FName		Category;
	// stijn: we temporarily store the alignment of the property here
	union
	{
		_WORD   RepOffset;
		_WORD   LinkAlignment;
	};
	_WORD		RepIndex;

	// In memory variables.
	INT			Offset;
	UProperty*	PropertyLinkNext;
	UProperty*	ConfigLinkNext;
	UProperty*	ConstructorLinkNext;
#if !OLDUNREAL_BINARY_COMPAT
	UProperty*	NextRef;
#endif
	UProperty*	RepOwner;

	// Constructors.
	UProperty();
	UProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags );

	// UObject interface.
	void Serialize( FArchive& Ar );

	// UProperty interface.
	virtual void Link( FArchive& Ar, UProperty* Prev );
	virtual UBOOL Identical( const void* A, const void* B ) const=0;
	virtual void ExportCpp( FOutputDevice& Out, UBOOL IsLocal, UBOOL IsParm ) const;
	virtual void ExportCppItem( FOutputDevice& Out ) const=0;
	virtual void SerializeItem( FArchive& Ar, void* Value ) const=0;
	virtual UBOOL NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const;
	virtual void ExportTextItem( TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const=0;
	virtual const TCHAR* ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const=0;
	virtual UBOOL ExportText( INT ArrayElement, TCHAR* ValueStr, BYTE* Data, BYTE* Delta, INT PortFlags ) const;
	virtual void CopySingleValue( void* Dest, void* Src ) const;
	virtual void CopyCompleteValue( void* Dest, void* Src ) const;
	virtual void DestroyValue( void* Dest ) const;
	virtual UBOOL Port() const;
	virtual BYTE GetID() const;

	// stijn: UE3-style exporters. This is what ExportItem/ExportTextItem _SHOULD_ have been. 
	static void ExportTextItem(UProperty* This, FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags);
	UBOOL ExportText(INT ArrayElement, FString& ValueStr, BYTE* Data, BYTE* Delta, INT PortFlags);

#if OLDUNREAL_BINARY_COMPAT
	// stijn: UE2-style CleanupDestroyed interface. We only had this interface for UStructs before, 
	// but a lot of UProperties were not being cleaned up properly, which resulted in use-after-frees 
	// and possibly crashes
	static void CleanupDestroyed(UProperty* This, BYTE* Data, UObject* Owner);
#else
	virtual void CleanupDestroyed(BYTE* Data, UObject* Owner);
#endif
	
	// Inlines.
	UBOOL Matches( const void* A, const void* B, INT ArrayIndex ) const
	{
		guardSlow(UProperty::Matches);
		INT Ofs = Offset + ArrayIndex * ElementSize;
		if ((A && !B) ||
			(B && !A))
			return false;
		return Identical( (BYTE*)A + Ofs, B ? (BYTE*)B + Ofs : NULL );
		unguardobjSlow;
	}
	INT GetSize() const
	{
		guardSlow(UProperty::GetSize);
		return ArrayDim * ElementSize;
		unguardobjSlow;
	}
	UBOOL ShouldSerializeValue( FArchive& Ar ) const
	{
		guardSlow(UProperty::ShouldSerializeValue);
		UBOOL Skip
		=	((PropertyFlags & CPF_Native   )                      )
		||	((PropertyFlags & CPF_Transient) && Ar.IsPersistent() );
		return !Skip;
		unguardobjSlow;
	}
};

/*-----------------------------------------------------------------------------
	UByteProperty.
-----------------------------------------------------------------------------*/

//
// Describes an unsigned byte value or 255-value enumeration variable.
//
class CORE_API UByteProperty : public UProperty
{
	DECLARE_CLASS(UByteProperty,UProperty,0,Core)

	// Variables.
	UEnum* Enum;

	// Constructors.
	UByteProperty()
	{}
	UByteProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags, UEnum* InEnum=NULL )
	:	UProperty( EC_CppProperty, InOffset, InCategory, InFlags )
	,	Enum( InEnum )
	{}

	// UObject interface.
	void Serialize( FArchive& Ar );

	// UProperty interface.
	void Link( FArchive& Ar, UProperty* Prev );
	UBOOL Identical( const void* A, const void* B ) const;
	void SerializeItem( FArchive& Ar, void* Value ) const;
	UBOOL NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const;
	void ExportCppItem( FOutputDevice& Out ) const;
	void ExportTextItem( TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const;
	void ExportTextItem(FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags) const;
	const TCHAR* ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const;
	void CopySingleValue( void* Dest, void* Src ) const;
	void CopyCompleteValue( void* Dest, void* Src ) const;
};

/*-----------------------------------------------------------------------------
	UIntProperty.
-----------------------------------------------------------------------------*/

//
// Describes a 32-bit signed integer variable.
//
class CORE_API UIntProperty : public UProperty
{
	DECLARE_CLASS(UIntProperty,UProperty,0,Core)

	// Constructors.
	UIntProperty()
	{}
	UIntProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags )
	:	UProperty( EC_CppProperty, InOffset, InCategory, InFlags )
	{}

	// UProperty interface.
	void Link( FArchive& Ar, UProperty* Prev );
	UBOOL Identical( const void* A, const void* B ) const;
	void SerializeItem( FArchive& Ar, void* Value ) const;
	UBOOL NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const;
	void ExportCppItem( FOutputDevice& Out ) const;
	void ExportTextItem( TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const;
	void ExportTextItem(FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags) const;
	const TCHAR* ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const;
	void CopySingleValue( void* Dest, void* Src ) const;
	void CopyCompleteValue( void* Dest, void* Src ) const;
};

/*-----------------------------------------------------------------------------
	UBoolProperty.
-----------------------------------------------------------------------------*/

//
// Describes a single bit flag variable residing in a 32-bit unsigned double word.
//
class CORE_API UBoolProperty : public UProperty
{
	DECLARE_CLASS(UBoolProperty,UProperty,0,Core)

	// Variables.
	BITFIELD BitMask;

	// Constructors.
	UBoolProperty()
	{}
	UBoolProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags )
	:	UProperty( EC_CppProperty, InOffset, InCategory, InFlags )
	,	BitMask( FIRST_BITFIELD )
	{}

	// UObject interface.
	void Serialize( FArchive& Ar );

	// UProperty interface.
	void Link( FArchive& Ar, UProperty* Prev );
	UBOOL Identical( const void* A, const void* B ) const;
	void SerializeItem( FArchive& Ar, void* Value ) const;
	UBOOL NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const;
	void ExportCppItem( FOutputDevice& Out ) const;
	void ExportTextItem( TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const;
	void ExportTextItem(FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags) const;
	const TCHAR* ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const;
	void CopySingleValue( void* Dest, void* Src ) const;
};

/*-----------------------------------------------------------------------------
	UFloatProperty.
-----------------------------------------------------------------------------*/

//
// Describes an IEEE 32-bit floating point variable.
//
class CORE_API UFloatProperty : public UProperty
{
	DECLARE_CLASS(UFloatProperty,UProperty,0,Core)

	// Constructors.
	UFloatProperty()
	{}
	UFloatProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags )
	:	UProperty( EC_CppProperty, InOffset, InCategory, InFlags )
	{}

	// UProperty interface.
	void Link( FArchive& Ar, UProperty* Prev );
	UBOOL Identical( const void* A, const void* B ) const;
	void SerializeItem( FArchive& Ar, void* Value ) const;
	UBOOL NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const;
	void ExportCppItem( FOutputDevice& Out ) const;
	void ExportTextItem( TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const;
	void ExportTextItem(FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags) const;
	const TCHAR* ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const;
	void CopySingleValue( void* Dest, void* Src ) const;
	void CopyCompleteValue( void* Dest, void* Src ) const;
};

/*-----------------------------------------------------------------------------
	UObjectProperty.
-----------------------------------------------------------------------------*/

//
// Describes a reference variable to another object which may be nil.
//
class CORE_API UObjectProperty : public UProperty
{
	DECLARE_CLASS(UObjectProperty,UProperty,0,Core)

	// Variables.
	class UClass* PropertyClass;
#if OLDUNREAL_BINARY_COMPAT
	UProperty* NextRef;
#endif

	// Constructors.
	UObjectProperty()
	{}
	UObjectProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags, UClass* InClass )
	:	UProperty( EC_CppProperty, InOffset, InCategory, InFlags )
	,	PropertyClass( InClass )
	{}

	// UObject interface.
	void Serialize( FArchive& Ar );

	// UProperty interface.
	void Link( FArchive& Ar, UProperty* Prev );
	UBOOL Identical( const void* A, const void* B ) const;
	void SerializeItem( FArchive& Ar, void* Value ) const;
	UBOOL NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const;
	void ExportCppItem( FOutputDevice& Out ) const;
	void ExportTextItem( TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const; // stijn: deprecated. do NOT use!!!
	void ExportTextItem(FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags) const;
	const TCHAR* ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const;
	void CleanupDestroyed(BYTE* Data, UObject* Owner);
	void CopySingleValue( void* Dest, void* Src ) const;
	void CopyCompleteValue( void* Dest, void* Src ) const;
};

/*-----------------------------------------------------------------------------
	UObjectProperty.
-----------------------------------------------------------------------------*/

//
// Describes a reference variable to another object which may be nil.
//
class CORE_API UClassProperty : public UObjectProperty
{
	DECLARE_CLASS(UClassProperty,UObjectProperty,0,Core)

	// Variables.
	class UClass* MetaClass;

	// Constructors.
	UClassProperty()
	{}
	UClassProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags, UClass* InMetaClass )
	:	UObjectProperty( EC_CppProperty, InOffset, InCategory, InFlags, UClass::StaticClass() )
	,	MetaClass( InMetaClass )
	{}

	// UObject interface.
	void Serialize( FArchive& Ar );

	// UProperty interface.
	const TCHAR* ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const;
	BYTE GetID() const
	{
		return NAME_ObjectProperty;
	}
};

/*-----------------------------------------------------------------------------
	UNameProperty.
-----------------------------------------------------------------------------*/

//
// Describes a name variable pointing into the global name table.
//
class CORE_API UNameProperty : public UProperty
{
	DECLARE_CLASS(UNameProperty,UProperty,0,Core)

	// Constructors.
	UNameProperty()
	{}
	UNameProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags )
	:	UProperty( EC_CppProperty, InOffset, InCategory, InFlags )
	{}

	// UProperty interface.
	void Link( FArchive& Ar, UProperty* Prev );
	UBOOL Identical( const void* A, const void* B ) const;
	void SerializeItem( FArchive& Ar, void* Value ) const;
	void ExportCppItem( FOutputDevice& Out ) const;
	void ExportTextItem( TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const;
	void ExportTextItem(FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags) const;
	const TCHAR* ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const;
	void CopySingleValue( void* Dest, void* Src ) const;
	void CopyCompleteValue( void* Dest, void* Src ) const;
};

/*-----------------------------------------------------------------------------
	UStrProperty.
-----------------------------------------------------------------------------*/

//
// Describes a dynamic string variable.
//
class CORE_API UStrProperty : public UProperty
{
	DECLARE_CLASS(UStrProperty,UProperty,0,Core)

	// Constructors.
	UStrProperty()
	{}
	UStrProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags )
	:	UProperty( EC_CppProperty, InOffset, InCategory, InFlags )
	{}

	// UObject interface.
	void Serialize( FArchive& Ar );

	// UProperty interface.
	void Link( FArchive& Ar, UProperty* Prev );
	UBOOL Identical( const void* A, const void* B ) const;
	void SerializeItem( FArchive& Ar, void* Value ) const;
	void ExportCppItem( FOutputDevice& Out ) const;
	void ExportTextItem( TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const;
	void ExportTextItem(FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags) const;
	const TCHAR* ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const;
	void CopySingleValue( void* Dest, void* Src ) const;
	void DestroyValue( void* Dest ) const;

	// Helpers.
	static void ExportTextItemHelper(FString& Out, const TCHAR* Value);
};

/*-----------------------------------------------------------------------------
	UFixedArrayProperty.
-----------------------------------------------------------------------------*/

//
// Describes a fixed length array.
//
class CORE_API UFixedArrayProperty : public UProperty
{
	DECLARE_CLASS(UFixedArrayProperty,UProperty,0,Core)

	// Variables.
	UProperty* Inner;
	INT Count;
#if OLDUNREAL_BINARY_COMPAT
	UProperty* NextRef;
#endif

	// Constructors.
	UFixedArrayProperty()
	{}
	UFixedArrayProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags )
	:	UProperty( EC_CppProperty, InOffset, InCategory, InFlags )
	{}

	// UObject interface.
	void Serialize( FArchive& Ar );

	// UProperty interface.
	void Link( FArchive& Ar, UProperty* Prev );
	UBOOL Identical( const void* A, const void* B ) const;
	void SerializeItem( FArchive& Ar, void* Value ) const;
	UBOOL NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const;
	void ExportCppItem( FOutputDevice& Out ) const;
	void ExportTextItem( TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const;
	void ExportTextItem(FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags) const;
	const TCHAR* ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const;
	void CopySingleValue( void* Dest, void* Src ) const;
	void DestroyValue( void* Dest ) const;
	void CleanupDestroyed(BYTE* Data, UObject* Owner);

	// UFixedArrayProperty interface.
	void AddCppProperty( UProperty* Property, INT Count );
};

/*-----------------------------------------------------------------------------
	UArrayProperty.
-----------------------------------------------------------------------------*/

//
// Describes a dynamic array.
//
class CORE_API UArrayProperty : public UProperty
{
	DECLARE_CLASS(UArrayProperty,UProperty,0,Core)

	// Variables.
	UProperty* Inner;
#if OLDUNREAL_BINARY_COMPAT
	UProperty* NextRef;
#endif
	INT        StaticArrayDim; // If replacing an old static array, keep a record

	// Constructors.
	UArrayProperty()
	{}
	UArrayProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags )
	:	UProperty( EC_CppProperty, InOffset, InCategory, InFlags )
	{}

	// UObject interface.
	void Serialize( FArchive& Ar );

	// UProperty interface.
	void Link( FArchive& Ar, UProperty* Prev );
	UBOOL Identical( const void* A, const void* B ) const;
	void SerializeItem( FArchive& Ar, void* Value ) const;
	UBOOL NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const;
	void ExportCppItem( FOutputDevice& Out ) const;
	void ExportTextItem( TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const;
	void ExportTextItem(FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags) const;
	const TCHAR* ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const;
	void CopySingleValue( void* Dest, void* Src ) const;
	void DestroyValue( void* Dest ) const;
	void CleanupDestroyed(BYTE* Data, UObject* Owner);

	// UArrayProperty interface.
	void AddCppProperty( UProperty* Property );

	void StaticConstructor();
};

/*-----------------------------------------------------------------------------
	UMapProperty.
-----------------------------------------------------------------------------*/

//
// Describes a dynamic map.
//
class CORE_API UMapProperty : public UProperty
{
	DECLARE_CLASS(UMapProperty,UProperty,0,Core)

	// Variables.
	UProperty* Key;
	UProperty* Value;

	// Constructors.
	UMapProperty()
	{}
	UMapProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags )
	:	UProperty( EC_CppProperty, InOffset, InCategory, InFlags )
	{}

	// UObject interface.
	void Serialize( FArchive& Ar );

	// UProperty interface.
	void Link( FArchive& Ar, UProperty* Prev );
	UBOOL Identical( const void* A, const void* B ) const;
	void SerializeItem( FArchive& Ar, void* Value ) const;
	UBOOL NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const;
	void ExportCppItem( FOutputDevice& Out ) const;
	void ExportTextItem( TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const;
	void ExportTextItem(FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags) const;
	const TCHAR* ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const;
	void CopySingleValue( void* Dest, void* Src ) const;
	void DestroyValue( void* Dest ) const;
};

/*-----------------------------------------------------------------------------
	UStructProperty.
-----------------------------------------------------------------------------*/

//
// Describes a structure variable embedded in (as opposed to referenced by) 
// an object.
//
class CORE_API UStructProperty : public UProperty
{
	DECLARE_CLASS(UStructProperty,UProperty,0,Core)

	// Variables.
	class UStruct* Struct;
#if OLDUNREAL_BINARY_COMPAT
	UProperty* NextRef;
#endif

	// Constructors.
	UStructProperty()
	{}
	UStructProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags, UStruct* InStruct )
	:	UProperty( EC_CppProperty, InOffset, InCategory, InFlags )
	,	Struct( InStruct )
	{}

	// UObject interface.
	void Serialize( FArchive& Ar );

	// 
	INT GetLinkAlignment();

	// UProperty interface.
	void Link( FArchive& Ar, UProperty* Prev );
	UBOOL Identical( const void* A, const void* B ) const;
	void SerializeItem( FArchive& Ar, void* Value ) const;
	UBOOL NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const;
	void ExportCppItem( FOutputDevice& Out ) const;
	void ExportTextItem( TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const;
	void ExportTextItem(FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags) const;
	const TCHAR* ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const;
	void CopySingleValue( void* Dest, void* Src ) const;
	void DestroyValue( void* Dest ) const;
	void CleanupDestroyed(BYTE* Data, UObject* Owner);
};

/*-----------------------------------------------------------------------------
	UPointerProperty.
-----------------------------------------------------------------------------*/

//
// Describes a C++ pointer
//
class CORE_API UPointerProperty : public UProperty
{
	DECLARE_CLASS(UPointerProperty, UProperty, 0, Core)

	// Constructors.
	UPointerProperty()
	{}
	UPointerProperty(ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags)
		: UProperty(EC_CppProperty, InOffset, InCategory, InFlags)
	{}

	// UProperty interface.
	void Link(FArchive& Ar, UProperty* Prev);
	UBOOL Identical(const void* A, const void* B) const;
	void SerializeItem(FArchive& Ar, void* Value) const;
	UBOOL NetSerializeItem(FArchive& Ar, UPackageMap* Map, void* Data) const;
	void ExportCppItem(FOutputDevice& Out) const;
	void ExportTextItem(TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags) const;
	void ExportTextItem(FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags) const;
	const TCHAR* ImportText(const TCHAR* Buffer, BYTE* Data, INT PortFlags) const;
	void CopySingleValue(void* Dest, void* Src) const;
	void CopyCompleteValue(void* Dest, void* Src) const;

	// stijn: UPackages encode the raw (i.e., on-disk) type of UProperties in a 
	// 4-bit tag. The GetID() function, which returns the raw type, must therefore 
	// return a value between 0 and 15. Unfortunately, UE1 already defined types 
	// for the entire 0-15 range before we added UPointerProperty. Luckily, it 
	// doesn't really hurt to "impersonate" intproperties here as it only affects
	// the on-disk representation of the pointer.
	BYTE GetID() const
	{
		return NAME_IntProperty;
	}
};

/*-----------------------------------------------------------------------------
	Field templates.
-----------------------------------------------------------------------------*/

//
// Find a typed field in a struct.
//
template <class T> T* FindField( UStruct* Owner, const TCHAR* FieldName )
{
	guard(FindField);
	for( TFieldIterator<T>It( Owner ); It; ++It )
		if( appStricmp( It->GetName(), FieldName )==0 )
			return *It;
	return NULL;
	unguard;
}

template <class T> T* FindOuter(UObject* Obj, BYTE bOuterMost = 0)
{
	guard(FindOuter);
	if (bOuterMost)
	{
		T* Best = NULL;
		while (Obj)
		{
			if (Obj->IsA(T::StaticClass()))
				Best = (T*)Obj;
			Obj = Obj->GetOuter();
		}
		return Best;
	}
	else
	{
		while (Obj)
		{
			if (Obj->IsA(T::StaticClass()))
				return (T*)Obj;
			Obj = Obj->GetOuter();
		}
	}
	return NULL;
	unguard;
}

/*-----------------------------------------------------------------------------
	UObject accessors that depend on UClass.
-----------------------------------------------------------------------------*/

//
// See if this object belongs to the specified class.
//
inline UBOOL UObject::IsA( class UClass* SomeBase ) const
{
	guardSlow(UObject::IsA);
	for( UClass* TempClass=Class; TempClass; TempClass=(UClass*)TempClass->SuperField )
		if( TempClass==SomeBase )
			return 1;
	return SomeBase==NULL;
	unguardobjSlow;
}

//
// See if this object is in a certain package.
//
inline UBOOL UObject::IsIn( class UObject* SomeOuter ) const
{
	guardSlow(UObject::IsA);
	for( UObject* It=GetOuter(); It; It=It->GetOuter() )
		if( It==SomeOuter )
			return 1;
	return SomeOuter==NULL;
	unguardobjSlow;
}

/*-----------------------------------------------------------------------------
	UStruct inlines.
-----------------------------------------------------------------------------*/

//
// UStruct inline comparer.
//
inline UBOOL UStruct::StructCompare( const void* A, const void* B )
{
	guardSlow(UStruct::StructCompare);
	for( TFieldIterator<UProperty> It(this); It; ++It )
		for( INT i=0; i<It->ArrayDim; i++ )
			if( !It->Matches(A,B,i) )
				return 0;
	unguardobjSlow;
	return 1;
}

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (pop)
#endif

/*-----------------------------------------------------------------------------
	C++ property macros.
-----------------------------------------------------------------------------*/

// The below, using offset 0 (NULL), makes gcc think we're misusing
//  the offsetof macro...we get around this by using 1 instead.  :) --ryan.
//#define CPP_PROPERTY(name) \
//	EC_CppProperty, (BYTE*)&((ThisClass*)NULL)->name - (BYTE*)NULL

// I should probably promote those to my normal headers... -han
#define PRE_PROPERTY_BITFIELD(TClass) \
    union \
    { \
        BITFIELD TClass##ConfigurationBitfield; \
        struct \
        { \

#define POST_PROPERTY_BITFIELD \
        };\
    };

#define CPP_PROPERTY(name) \
	EC_CppProperty, (BYTE*)&((ThisClass*)1)->name - (BYTE*)1

#define CPP_PROPERTY_BITFIELD(TClass) CPP_PROPERTY(TClass##ConfigurationBitfield)

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
