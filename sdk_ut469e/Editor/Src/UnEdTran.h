/*=============================================================================
	UnEdTran.h: Unreal transaction tracking system
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

/*-----------------------------------------------------------------------------
	UTransactor.
-----------------------------------------------------------------------------*/

//
// Object responsible for tracking transactions for undo/redo.
//
class EDITOR_API UTransactor : public UObject
{
	DECLARE_ABSTRACT_CLASS(UTransactor,UObject,CLASS_Transient,Editor)

	// UTransactor interface.
	virtual void Reset( const TCHAR* Action )=0;
	virtual void Begin( const TCHAR* SessionName )=0;
	virtual void End()=0;
	virtual void Continue()=0;
	virtual UBOOL CanUndo( FString* Str=NULL )=0;
	virtual UBOOL CanRedo( FString* Str=NULL )=0;
	virtual UBOOL Undo()=0;
	virtual UBOOL Redo()=0;
	virtual FString UndoTitle()=0;
	virtual FString RedoTitle()=0;
	virtual FTransactionBase* CreateInternalTransaction()=0;
	virtual UBOOL IsMapChanged()=0;
	virtual void SetMapChanged(UBOOL InMapChanged)=0;
	virtual UBOOL IsMapChangedForAutosave()=0;
	virtual void SetMapChangedForAutosave(UBOOL InMapChanged)=0;
};

class EDITOR_API FTransaction : public FTransactionBase
{
public:
	// Record of an object.
	class FObjectRecord
	{
	public:
		// Variables.
		TArray<BYTE>	Data;
		UObject*		Object;
		FArray*			Array;
		INT				Index;
		INT				Count;
		INT				Oper;
		INT				ElementSize;
		STRUCT_AR		Serializer;
		STRUCT_DTOR		Destructor;
		UBOOL			Restored;

		// Constructors.
		FObjectRecord();	
		FObjectRecord(FTransaction* Owner, UObject* InObject, FArray* InArray, INT InIndex, INT InCount, INT InOper, INT InElementSize, STRUCT_AR InSerializer, STRUCT_DTOR InDestructor);

		// Functions.
		void SerializeContents(FArchive& Ar, INT InOper);
		void Restore(FTransaction* Owner);
		UBOOL IsSame(FObjectRecord& Other);
		friend FArchive& operator<<(FArchive& Ar, FObjectRecord& R);

		// Transfers data from an array.
		class FReader : public FArchive
		{
		public:
			FReader(FTransaction* InOwner, TArray<BYTE>& InBytes);
		private:
			void Serialize(void* Data, INT Num);
			FArchive& operator<<(class FName& N);
			FArchive& operator<<(class UObject*& Res);
			void Preload(UObject* Object);
			FTransaction* Owner;
			TArray<BYTE>& Bytes;
			INT Offset;
		};

		// Transfers data to an array.
		class FWriter : public FArchive
		{
		public:
			FWriter(TArray<BYTE>& InBytes);
		private:
			void Serialize(void* Data, INT Num);
			FArchive& operator<<(class FName& N);
			FArchive& operator<<(class UObject*& Res);
			TArray<BYTE>& Bytes;
		};
	};

	// Transaction variables.
	TArray<FObjectRecord>	Records;
	FString					Title;
	UBOOL					Flip;
	INT						Inc;

	// Constructor.
	FTransaction(const TCHAR* InTitle = NULL, UBOOL InFlip = 0);

	// FTransactionBase interface.
	void SaveObject(UObject* Object);
	void SaveArray(UObject* Object, FArray* Array, INT Index, INT Count, INT Oper, INT ElementSize, STRUCT_AR Serializer, STRUCT_DTOR Destructor);
	void Apply();

	// FTransaction interface.
	SIZE_T DataSize();
	const TCHAR* GetTitle();
	friend FArchive& operator<<(FArchive& Ar, FTransaction& T);

	// Transaction friends.
	friend class FObjectRecord;
	friend class FObjectRecord::FReader;
	friend class FObjectRecord::FWriter;
};

class EDITOR_API UTransBuffer : public UTransactor
{
	DECLARE_CLASS(UTransBuffer, UObject, CLASS_Transient, Editor)
	NO_DEFAULT_CONSTRUCTOR(UTransBuffer)

	// Variables.
	TArray<FTransaction>	UndoBuffer;
	INT						UndoCount;
	FString					ResetReason;
	INT						ActiveCount;
	SIZE_T					MaxMemory;
	UBOOL					Overflow;
	UBOOL					MapChanged;
	UBOOL					MapChangedForAutoSave;

	// Constructor.
	UTransBuffer(SIZE_T InMaxMemory);

	// UObject interface.
	void Serialize(FArchive& Ar);
	void Destroy();

	// UTransactor interface.
	void Reset(const TCHAR* Reason);
	void Begin(const TCHAR* SessionName);
	void End();
	void Continue();
	UBOOL CanUndo(FString* Str = NULL);
	UBOOL CanRedo(FString* Str = NULL);
	UBOOL Undo();
	UBOOL Redo();
	FString UndoTitle();
	FString RedoTitle();
	FTransactionBase* CreateInternalTransaction();
	UBOOL IsMapChanged();
	void SetMapChanged(UBOOL InMapChanged);
	UBOOL IsMapChangedForAutosave();
	void SetMapChangedForAutosave(UBOOL InMapChanged);

	// Functions.
	void FinishDo();
	SIZE_T UndoDataSize();
	void CheckState();
};

/*----------------------------------------------------------------------------
	The End.
----------------------------------------------------------------------------*/
