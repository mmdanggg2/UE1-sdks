/*=============================================================================
	UnFieldCache.h: Cached UField iterator definition.
	Originally implemented in XC_Core, rewritten and improved for UT v469

	Revision history:
		* Created by Fernando Velazquez (Higor)
=============================================================================*/


/*-----------------------------------------------------------------------------
	Cache Model interface.
-----------------------------------------------------------------------------*/

//
// Feeds the iterator information on what fields we want to enumerate.
// 
class FFieldListCacheModel
{
public:
	enum EClassIteratorType
	{
		CI_Accept, //Iterate fields of this class and its superclasses
		CI_AcceptSkip, //Skip this class, iterate over superclass
		CI_Reject, //Do not iterate this class or its superclasses
	};

	//FFieldListCacheModel interface
	virtual UBOOL AcceptField( UField* Field)
	{
		return 0;
	}

	virtual EClassIteratorType AcceptClass( UClass* Class)
	{
		return CI_Accept;
	}

	virtual DWORD UniqueId()  // Must be unique in every subclass.
	{
		return 0;
	}
};

//
// Feeds the iterator properties with CPF_Travel flags.
//
class FPropertyListTravel : public FFieldListCacheModel
{
public:
	// FFieldListCacheModel interface
	virtual UBOOL AcceptField( UField* Field)
	{
		UProperty* Property = Cast<UProperty>(Field);
		return Property && (Property->PropertyFlags & CPF_Travel);
	}

	virtual EClassIteratorType AcceptClass( UClass* Class)
	{
		if (!Class || !Class->IsChildOf(AActor::StaticClass()))
			return CI_Reject;
		return CI_Accept;
	}

	virtual DWORD UniqueId()
	{
		return CPF_Travel;
	}
};


/*-----------------------------------------------------------------------------
	Iterator.
-----------------------------------------------------------------------------*/

//
// Base code for the field cache iterator and field list definition.
// NOTE: the Fields array always terminates in a "nullptr"
//
class ENGINE_API FFieldCacheIterator
{
protected:
	struct FFieldList
	{
		UClass* Class;
		INT FieldCount;
		UField* Fields[ZEROARRAY];
	};

	FFieldList* CurrentClass;
	INT         CurrentField;


	FFieldCacheIterator()
		: CurrentClass( NULL )
		, CurrentField( 0 )
	{}

	static FFieldList* GetClassCache( FFieldListCacheModel* Model, UClass* Class);

public:
	void operator++()          { ++CurrentField; }
	operator UBOOL() const     { return (CurrentClass != NULL) && (CurrentField < CurrentClass->FieldCount); }
	UField* operator*() const  { return CurrentClass->Fields[CurrentField]; }
	UField* operator->() const { return CurrentClass->Fields[CurrentField]; }
};


//
// Iterates through a cached field lists.
// * The iterator requires a FFieldListCacheModel type as template parameter.
// * In absence of a cached field list, the iterator will use the model to create one.
//
template <class T> class TFieldCacheIterator : public FFieldCacheIterator
{
	T CacheModel;

public:
	TFieldCacheIterator( UClass* Class)
	{
		guard(TFieldCacheIterator);
		check(Class);
		CurrentClass = GetClassCache( &CacheModel, Class);
		unguard;
	}

	// Util for finding a single UField of a specific type.
	template <class FIELDTYPE=UField> static FIELDTYPE* FindNamed( UClass* Class, const TCHAR* Name)
	{
		guard(TFieldCacheIterator::FindNamed);
		FName FindName(Name,FNAME_Find);
		if ( FindName != NAME_None )
		{
			for ( TFieldCacheIterator<T> It(Class) ; It ; ++It)
				if ( (It->GetFName() == FindName) && It->IsA(FIELDTYPE::StaticClass()) )
					return (FIELDTYPE*)*It;
		}
		return NULL;
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The end.
-----------------------------------------------------------------------------*/
