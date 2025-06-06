/*=============================================================================
	UnType.h: Unreal engine base type definitions.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/*-----------------------------------------------------------------------------
	UProperty.
-----------------------------------------------------------------------------*/

//
// An UnrealScript variable.
//
class CORE_API UProperty : public UField
{
	DECLARE_ABSTRACT_CLASS(UProperty,UField,0)
	DECLARE_WITHIN(UField)

	// Persistent variables.
	INT			ArrayDim;
	INT			ElementSize;
	DWORD		PropertyFlags;
	FName		Category;
	_WORD		RepOffset;

	// In memory variables.
	INT			Offset;
	UProperty*	PropertyLinkNext;
	UProperty*	ConfigLinkNext;
	UProperty*	ConstructorLinkNext;
	UProperty*	RepOwner;

	// Constructors.
	UProperty();
	UProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags );

	// UObject interface.
	void Serialize( FArchive& Ar );

	// UProperty interface.
	virtual UBOOL Identical( const void* A, const void* B ) const=0;
	virtual void ExportCpp( FArchive& Out, UBOOL IsLocal, UBOOL IsParm );
	virtual void ExportCppItem( FArchive& Out ) const=0;
	virtual INT GetElementSize() const = 0;
	virtual void SerializeItem( FArchive& Ar, void* Value ) const=0;
	virtual void ExportTextItem( TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, UBOOL HumanReadable )=0;
	virtual const TCHAR* ImportText( const TCHAR* Buffer, BYTE* Data, UBOOL HumanReadable ) const=0;
	virtual UBOOL ExportText( INT ArrayElement, TCHAR* ValueStr, BYTE* Data, BYTE* Delta, UBOOL HumanReadable );
	virtual void Link( FArchive& Ar, UProperty* Prev )=0;
	virtual void ExecLet(void*, FFrame&);
	virtual UBOOL Port();
	virtual BYTE GetID();

	// Inlines.
	UBOOL Matches( const void* A, const void* B, INT ArrayIndex ) const
	{
		guardSlow(UProperty::Matches);
		INT Ofs = Offset + ArrayIndex * ElementSize;
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
	DECLARE_CLASS(UByteProperty,UProperty,0)

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
	UBOOL Identical( const void* A, const void* B ) const;
	void ExportCppItem( FArchive& Out ) const;
	INT GetElementSize() const;
	void SerializeItem( FArchive& Ar, void* Value ) const;
	void ExportTextItem( TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, UBOOL HumanReadable );
	const TCHAR* ImportText( const TCHAR* Buffer, BYTE* Data, UBOOL HumanReadable ) const;
	void Link( FArchive& Ar, UProperty* Prev );
};

/*-----------------------------------------------------------------------------
	UIntProperty.
-----------------------------------------------------------------------------*/

//
// Describes a 32-bit signed integer variable.
//
class CORE_API UIntProperty : public UProperty
{
	DECLARE_CLASS(UIntProperty,UProperty,0)

	// Constructors.
	UIntProperty()
	{}
	UIntProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags )
	:	UProperty( EC_CppProperty, InOffset, InCategory, InFlags )
	{}

	// UProperty interface.
	UBOOL Identical(const void* A, const void* B) const;
	void ExportCppItem(FArchive& Out) const;
	INT GetElementSize() const;
	void SerializeItem(FArchive& Ar, void* Value) const;
	void ExportTextItem(TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, UBOOL HumanReadable);
	const TCHAR* ImportText(const TCHAR* Buffer, BYTE* Data, UBOOL HumanReadable) const;
	void Link(FArchive& Ar, UProperty* Prev);
};

/*-----------------------------------------------------------------------------
	UBoolProperty.
-----------------------------------------------------------------------------*/

//
// Describes a single bit flag variable residing in a 32-bit unsigned double word.
//
class CORE_API UBoolProperty : public UProperty
{
	DECLARE_CLASS(UBoolProperty,UProperty,0)

	// Variables.
	BITFIELD BitMask;

	// Constructors.
	UBoolProperty()
	{}
	UBoolProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags )
	:	UProperty( EC_CppProperty, InOffset, InCategory, InFlags )
	,	BitMask( 1 )
	{}

	// UObject interface.
	void Serialize( FArchive& Ar );

	// UProperty interface.
	UBOOL Identical(const void* A, const void* B) const;
	void ExportCppItem(FArchive& Out) const;
	INT GetElementSize() const;
	void SerializeItem(FArchive& Ar, void* Value) const;
	void ExportTextItem(TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, UBOOL HumanReadable);
	const TCHAR* ImportText(const TCHAR* Buffer, BYTE* Data, UBOOL HumanReadable) const;
	void Link(FArchive& Ar, UProperty* Prev);
	void ExecLet(void*, FFrame&);
};

/*-----------------------------------------------------------------------------
	UFloatProperty.
-----------------------------------------------------------------------------*/

//
// Describes an IEEE 32-bit floating point variable.
//
class CORE_API UFloatProperty : public UProperty
{
	DECLARE_CLASS(UFloatProperty,UProperty,0)

	// Constructors.
	UFloatProperty()
	{}
	UFloatProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags )
	:	UProperty( EC_CppProperty, InOffset, InCategory, InFlags )
	{}

	// UProperty interface.
	UBOOL Identical(const void* A, const void* B) const;
	void ExportCppItem(FArchive& Out) const;
	INT GetElementSize() const;
	void SerializeItem(FArchive& Ar, void* Value) const;
	void ExportTextItem(TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, UBOOL HumanReadable);
	const TCHAR* ImportText(const TCHAR* Buffer, BYTE* Data, UBOOL HumanReadable) const;
	void Link(FArchive& Ar, UProperty* Prev);
};

/*-----------------------------------------------------------------------------
	UObjectProperty.
-----------------------------------------------------------------------------*/

//
// Describes a reference variable to another object which may be nil.
//
class CORE_API UObjectProperty : public UProperty
{
	DECLARE_CLASS(UObjectProperty,UProperty,0)

	// Variables.
	class UClass* PropertyClass;
	UObjectProperty* NextReference;

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
	UBOOL Identical(const void* A, const void* B) const;
	void ExportCppItem(FArchive& Out) const;
	INT GetElementSize() const;
	void SerializeItem(FArchive& Ar, void* Value) const;
	void ExportTextItem(TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, UBOOL HumanReadable);
	const TCHAR* ImportText(const TCHAR* Buffer, BYTE* Data, UBOOL HumanReadable) const;
	void Link(FArchive& Ar, UProperty* Prev);
};

/*-----------------------------------------------------------------------------
	UObjectProperty.
-----------------------------------------------------------------------------*/

//
// Describes a reference variable to another object which may be nil.
//
class CORE_API UClassProperty : public UObjectProperty
{
	DECLARE_CLASS(UClassProperty,UObjectProperty,0)

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
	const TCHAR* ImportText( const TCHAR* Buffer, BYTE* Data, UBOOL HumanReadable ) const;
	BYTE GetID()
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
	DECLARE_CLASS(UNameProperty,UProperty,0)

	// Constructors.
	UNameProperty()
	{}
	UNameProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags )
	:	UProperty( EC_CppProperty, InOffset, InCategory, InFlags )
	{}

	// UProperty interface.
	UBOOL Identical(const void* A, const void* B) const;
	void ExportCppItem(FArchive& Out) const;
	INT GetElementSize() const;
	void SerializeItem(FArchive& Ar, void* Value) const;
	void ExportTextItem(TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, UBOOL HumanReadable);
	const TCHAR* ImportText(const TCHAR* Buffer, BYTE* Data, UBOOL HumanReadable) const;
	void Link(FArchive& Ar, UProperty* Prev);
};

/*-----------------------------------------------------------------------------
	UStrProperty.
-----------------------------------------------------------------------------*/

//
// Describes a dynamic string variable.
//
class CORE_API UStrProperty : public UProperty
{
	DECLARE_CLASS(UStrProperty,UProperty,0)

	// Constructors.
	UStrProperty()
	{}
	UStrProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags )
	:	UProperty( EC_CppProperty, InOffset, InCategory, InFlags )
	{}

	// UObject interface.
	void Serialize( FArchive& Ar );

	// UProperty interface.
	UBOOL Identical(const void* A, const void* B) const;
	void ExportCppItem(FArchive& Out) const;
	INT GetElementSize() const;
	void SerializeItem(FArchive& Ar, void* Value) const;
	void ExportTextItem(TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, UBOOL HumanReadable);
	const TCHAR* ImportText(const TCHAR* Buffer, BYTE* Data, UBOOL HumanReadable) const;
	void Link(FArchive& Ar, UProperty* Prev);
	void ExecLet(void*, FFrame&);
};


/*-----------------------------------------------------------------------------
	UArrayProperty.
-----------------------------------------------------------------------------*/

//
// Describes a dynamic array.
//
class CORE_API UArrayProperty : public UProperty
{
	DECLARE_CLASS(UArrayProperty,UProperty,0)

	// Variables.
	UProperty* Inner;

	// Constructors.
	UArrayProperty()
	{}
	UArrayProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags )
	:	UProperty( EC_CppProperty, InOffset, InCategory, InFlags )
	{}

	// UObject interface.
	void Serialize( FArchive& Ar );

	// UField interface.
	void AddCppProperty(UProperty* Property);

	// UProperty interface.
	UBOOL Identical(const void* A, const void* B) const;
	void ExportCppItem(FArchive& Out) const;
	INT GetElementSize() const;
	void SerializeItem(FArchive& Ar, void* Value) const;
	void ExportTextItem(TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, UBOOL HumanReadable);
	const TCHAR* ImportText(const TCHAR* Buffer, BYTE* Data, UBOOL HumanReadable) const;
	void Link(FArchive& Ar, UProperty* Prev);
	void ExecLet(void*, FFrame&);
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
	DECLARE_CLASS(UStructProperty,UProperty,0)

	// Variables.
	class UStruct* Struct;
	UStructProperty* NextStruct;

	// Constructors.
	UStructProperty()
	{}
	UStructProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags, UStruct* InStruct )
	:	UProperty( EC_CppProperty, InOffset, InCategory, InFlags )
	,	Struct( InStruct )
	{}

	// UObject interface.
	void Serialize( FArchive& Ar );

	// UProperty interface.
	UBOOL Identical(const void* A, const void* B) const;
	void ExportCppItem(FArchive& Out) const;
	INT GetElementSize() const;
	void SerializeItem(FArchive& Ar, void* Value) const;
	void ExportTextItem(TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, UBOOL HumanReadable);
	const TCHAR* ImportText(const TCHAR* Buffer, BYTE* Data, UBOOL HumanReadable) const;
	void Link(FArchive& Ar, UProperty* Prev);
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

//
// Return whether an object wants to receive a named probe message.
//
inline UBOOL UObject::IsProbing( FName ProbeName )
{
	guardSlow(UObject::IsProbing);
	return	(ProbeName.GetIndex() <  NAME_PROBEMIN)
	||		(ProbeName.GetIndex() >= NAME_PROBEMAX)
	||		(!StateFrame)
	||		(StateFrame->ProbeMask & ((QWORD)1 << (ProbeName.GetIndex() - NAME_PROBEMIN)));
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

/*-----------------------------------------------------------------------------
	C++ property macros.
-----------------------------------------------------------------------------*/

#define CPP_PROPERTY(name) \
	EC_CppProperty, (BYTE*)&((ThisClass*)NULL)->name - (BYTE*)NULL

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
