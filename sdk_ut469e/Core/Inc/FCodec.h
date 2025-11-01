/*=============================================================================
	UnCodec.h: Data compression codecs.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Tim Sweeney.
=============================================================================*/

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack(push,OBJECT_ALIGNMENT)
#endif

/*-----------------------------------------------------------------------------
	Coder/decoder base class.
-----------------------------------------------------------------------------*/

class FCodecNotify
{
public:
	virtual UBOOL NotifyProgress( FLOAT Progress ) { return 1; }
};

class FCodec
{
public:
	virtual ~FCodec() noexcept(false) {}
	virtual UBOOL Encode( FArchive& In, FArchive& Out, FCodecNotify* Notify=NULL )=0;
	virtual UBOOL Decode( FArchive& In, FArchive& Out, FCodecNotify* Notify=NULL )=0;
};

/*-----------------------------------------------------------------------------
	Burrows-Wheeler inspired data compressor.
-----------------------------------------------------------------------------*/

#if defined(_MSC_VER) && !defined(ENGINE_API)
#define ENGINE_API DLL_IMPORT
#endif

#include "divsufsort.h"

//#define SHOW_PROGRESS

// Buggie: Some hardware failed to allocate exactly 32th thread, so we limit it to 31.
#define MAX_THREADS 31

inline static INT GetThreadsCount()
{
	guard(GetThreadsCount);
	INT ThreadsCount = Max<INT>(1, Min<INT>(GProcessorCount, MAX_THREADS));
	GWarn->Logf(TEXT("Used %d threads."), ThreadsCount);
	return ThreadsCount;
	unguard;
}

typedef struct
{
	BOOL Encode;
	TArray<BYTE> CompressBufferArray;
	BYTE* CompressBuffer;
	INT CompressLength;
	INT First, Last;
	TArray<INT>  Temp_;
	INT*  Temp;
	TArray<BYTE>  BufOut_;
	BYTE* BufOut;
} BWTData;

// Could be used as an actual thread function if we ever add proper threading abstractions to the code.
inline void BWTThread(BWTData* Data)
{
	if (Data->Encode) 
	{
		Data->CompressBuffer[Data->CompressLength] = 255;
		/*int ret = */divsufsort(Data->CompressBuffer, Data->Temp, Data->CompressLength + 1);
		Data->Temp[Data->CompressLength] = Data->CompressLength;
		Data->First = 0, Data->Last = 0;
		for (INT i = 0; i < Data->CompressLength + 1; i++) 
		{
			INT pos = Data->Temp[i];
			if (pos == 1) Data->First = i;
			else if (pos == 0) Data->Last = i;
			Data->BufOut[i] = Data->CompressBuffer[pos ? pos - 1 : 0];
		}
	}
	else
	{
		INT DecompressCount[256+1], RunningTotal[256+1];
		memset(&DecompressCount[0], 0, sizeof(DecompressCount));
		for( INT i=0; i<Data->CompressLength; i++ )
			DecompressCount[ i!=Data->Last ? Data->CompressBuffer[i] : 256 ]++;
		INT Sum = 0;
		for( INT i=0; i<257; i++ )
		{
			RunningTotal[i] = Sum;
			Sum += DecompressCount[i];
			DecompressCount[i] = 0;
		}
		for( INT i=0; i<Data->CompressLength; i++ )
		{
			INT Index = i!=Data->Last ? Data->CompressBuffer[i] : 256;
			Data->Temp[RunningTotal[Index] + DecompressCount[Index]++] = i;
		}
		for( INT i=Data->First,j=0 ; j<Data->CompressLength-1; i=Data->Temp[i],j++ )
			Data->BufOut[j] = Data->CompressBuffer[i];
	}
}

class BWTRunnable : public FRunnable
{
public:
	BWTData* Data;

	UBOOL Init() override
	{
		return TRUE;
	}
	DWORD Run() override
	{
		BWTThread(Data);
		return 0;
	}
	UBOOL Stop() override
	{
		return TRUE;
	}
	UBOOL Exit() override
	{
		return TRUE;
	}
};

const INT ThreadStackSize = 64*1024;
const INT RealBufferSize = 0x40000;

class FCodecBWT : public FCodec
{
private:
	enum {MAX_BUFFER_SIZE=0x40000}; /* Hand tuning suggests this is an ideal size */
public:
	UBOOL Encode( FArchive& In, FArchive& Out, FCodecNotify* Notify=NULL )
	{
		guard(FCodecBWT::Encode);

		#ifdef SHOW_PROGRESS			
			Warn.LocalPrint(TEXT("0%"));
			FString Progress;
		#endif

		INT ThreadsCount = GetThreadsCount();

		FRunnableThread* RunThreads[MAX_THREADS];
		BWTRunnable* Runnables[MAX_THREADS];

		BWTData Data[MAX_THREADS];
		for (INT t = 0; t < ThreadsCount; t++) {
			Data[t].Encode = TRUE;
			Data[t].CompressBufferArray.Add(MAX_BUFFER_SIZE + 1);
			Data[t].CompressBuffer = &Data[t].CompressBufferArray(0);
			Data[t].Temp_.Add(MAX_BUFFER_SIZE + 1);
			Data[t].Temp = &Data[t].Temp_(0);
			Data[t].BufOut_.Add(MAX_BUFFER_SIZE + 1);
			Data[t].BufOut = &Data[t].BufOut_(0);
		}

		while( !In.AtEnd() )
		{
			INT Threads = ThreadsCount;
			for (INT t = 0; t < ThreadsCount; t++)
			{
				Data[t].CompressLength = Min<INT>( In.TotalSize()-In.Tell(), MAX_BUFFER_SIZE );
				if (Data[t].CompressLength <= 0)
				{
					Threads = t;
					break;
				}
				In.Serialize( Data[t].CompressBuffer, Data[t].CompressLength );

				Runnables[t] = new ::BWTRunnable();
				Runnables[t]->Data = &Data[t];
				RunThreads[t] = GThreadFactory->CreateThread(Runnables[t], FALSE, TRUE, ThreadStackSize);
				if (!RunThreads[t])
				{
					INT Error = appGetSystemErrorCode();
					GWarn->Logf(TEXT("FCodecBWT::Encode %d/%d CreateThread failed: %d (%ls)"), t, ThreadsCount, Error, appGetSystemErrorMessage(Error));
				}
			}

			// Run all chunks which not get thread, in the main thread.
			for (INT t = 0; t < Threads; t++)
				if (!RunThreads[t])
				{
					Runnables[t]->Run();
					delete Runnables[t];
				}

			for (INT t = 0; t < Threads; t++)
				if (RunThreads[t])
					RunThreads[t]->WaitForCompletion();

			for (INT t = 0; t < Threads; t++)
			{
				if (RunThreads[t])
					GThreadFactory->Destroy(RunThreads[t]);
				Out << Data[t].CompressLength << Data[t].First << Data[t].Last;
				Out.Serialize(Data[t].BufOut, Data[t].CompressLength + 1);
			}

			#ifdef SHOW_PROGRESS				
				Warn.LocalPrint(*FString::Printf(
					#ifdef DBG
						TEXT("\n")
					#else
						TEXT("\r")
					#endif
					TEXT("%.3f%%"), 100.f*In.Tell()/In.TotalSize()));
			#endif
		}
		#ifdef SHOW_PROGRESS
			Warn.LocalPrint(TEXT("\r"));
		#endif

		return 1;
		unguard;
	}
	UBOOL Decode( FArchive& In, FArchive& Out, FCodecNotify* Notify=NULL )
	{
		guard(FCodecBWT::Decode);

		INT ThreadsCount = GetThreadsCount();

		FRunnableThread* RunThreads[MAX_THREADS];
		BWTRunnable* Runnables[MAX_THREADS];

		BWTData Data[MAX_THREADS];
		for (INT t = 0; t < ThreadsCount; t++) {
			Data[t].Encode = false;
			Data[t].CompressBufferArray.Add(MAX_BUFFER_SIZE + 1);
			Data[t].CompressBuffer = &Data[t].CompressBufferArray(0);
			Data[t].Temp_.Add(MAX_BUFFER_SIZE + 1);
			Data[t].Temp = &Data[t].Temp_(0);
			Data[t].BufOut_.Add(MAX_BUFFER_SIZE);
			Data[t].BufOut = &Data[t].BufOut_(0);
		}

		while( !In.AtEnd() )
		{
			INT Threads = ThreadsCount;
			for (INT t = 0; t < ThreadsCount; t++) {
				if (In.AtEnd()) {
					Threads = t;
					break;
				}
				In << Data[t].CompressLength << Data[t].First << Data[t].Last;
				// stijn: made this non-fatal in v469
				if (Data[t].CompressLength > MAX_BUFFER_SIZE + 1
					|| Data[t].CompressLength > In.TotalSize() - In.Tell())
				{
					for (INT k = 0; k < t; k++)
					{
						if (!RunThreads[k])
							delete Runnables[k];
						else
						{
							RunThreads[k]->WaitForCompletion(); // better from kill
							GThreadFactory->Destroy(RunThreads[k]);
						}
					}
					return 0;
				}
//				check(Data[t].CompressLength<=MAX_BUFFER_SIZE+1);
//				check(Data[t].CompressLength<=In.TotalSize()-In.Tell());
				In.Serialize( &Data[t].CompressBuffer[0], ++Data[t].CompressLength );

				Runnables[t] = new ::BWTRunnable();
				Runnables[t]->Data = &Data[t];
				RunThreads[t] = GThreadFactory->CreateThread(Runnables[t], FALSE, TRUE, ThreadStackSize);
				if (!RunThreads[t])
				{
					INT Error = appGetSystemErrorCode();
					GWarn->Logf(TEXT("FCodecBWT::Decode %d/%d CreateThread failed: %d (%ls)"), t, ThreadsCount, Error, appGetSystemErrorMessage(Error));
				}
			}

			// Run all chunks which not get thread, in the main thread.
			for (INT t = 0; t < Threads; t++)
				if (!RunThreads[t])
				{
					Runnables[t]->Run();
					delete Runnables[t];
				}

			for (INT t = 0; t < Threads; t++)
				if (RunThreads[t])
					RunThreads[t]->WaitForCompletion();

			for (INT t = 0; t < Threads; t++)
			{
				if (RunThreads[t])
					GThreadFactory->Destroy(RunThreads[t]);
				Out.Serialize(Data[t].BufOut, Data[t].CompressLength-1);
			}

			if( Notify && !Notify->NotifyProgress( (FLOAT)In.Tell() / (FLOAT)In.TotalSize() ) )
				return 0;
		}
		return 1;
		unguard;
	}
};

#define BUF_SIZE 0x10000

/*-----------------------------------------------------------------------------
	RLE compressor.
-----------------------------------------------------------------------------*/

class FCodecRLE : public FCodec
{
private:
	enum {RLE_LEAD=5};
	inline void EncodeEmitRun( FArchive& Out, BYTE Char, BYTE Count )
	{
		for( INT Down=Min<INT>(Count,RLE_LEAD); Down>0; Down-- )
			Out << Char;
		if( Count>=RLE_LEAD )
			Out << Count;
	}
public:
	UBOOL Encode( FArchive& In, FArchive& Out, FCodecNotify* Notify=NULL )
	{
		guard(FCodecRLE::Encode);
		BYTE PrevChar=0, PrevCount=0, BufIn[BUF_SIZE];
		TArray<BYTE>  BufOut_(BUF_SIZE*6/5 + 10);
		BYTE* BufOut = &BufOut_(0);
		INT Length = In.TotalSize();
		while( Length > 0 )
		{
			INT BufLength = Min(Length, BUF_SIZE);
			In.Serialize( BufIn, BufLength );
			INT OutLength = 0;
			for( INT j=0; j<BufLength; j++ )
			{
				BYTE B = BufIn[j];
				if( B!=PrevChar || PrevCount==255 )
				{
					if (PrevCount == 1) {
						BufOut[OutLength++] = PrevChar;
					} else {
						for( INT End=OutLength + Min<INT>(PrevCount,RLE_LEAD); OutLength < End;)
							BufOut[OutLength++] = PrevChar;
						if( PrevCount>=RLE_LEAD )
							BufOut[OutLength++] = PrevCount;
					}
					PrevChar  = B;
					PrevCount = 1;
				} else PrevCount++;
			}
			Out.Serialize( BufOut, OutLength );
			Length -= BufLength;
		}
		EncodeEmitRun( Out, PrevChar, PrevCount );
		return 1;
		unguard;
	}
	UBOOL Decode( FArchive& In, FArchive& Out, FCodecNotify* Notify=NULL )
	{
		guard(FCodecRLE::Decode);
		INT Count=0;
		BYTE PrevChar=0, BufIn[BUF_SIZE];
		TArray<BYTE>  BufOut_(BUF_SIZE*255/(RLE_LEAD+1) + 10);
		BYTE* BufOut = &BufOut_(0);
		INT Total = In.TotalSize();
		INT Length = Total;
		INT i = 0;
		while( Length > 0 )
		{
			INT BufLength = Min(Length, BUF_SIZE);
			In.Serialize( BufIn, BufLength );
			INT OutLength = 0;
			for( INT j=0; j<BufLength; j++ )
			{
				BYTE B = BufIn[j];
				BufOut[OutLength++] = B;
				if( B!=PrevChar )
				{
					PrevChar = B;
					Count    = 1;
				}
				else if( ++Count==RLE_LEAD )
				{
					BYTE C;
					if (j == BufLength - 1) {
						In << C;
						Length--;
					} else {
						C = BufIn[++j];
					}
					// stijn: made this non-fatal in v469
					if (C < 2)
						return 0;
					C -= RLE_LEAD;
					memset(&BufOut[OutLength], B, C);					
					OutLength += C;
					Count = 0;
				}
			}
			Out.Serialize( BufOut, OutLength );
			Length -= BufLength;
			if( Notify && !(i++ & 0xF) && !Notify->NotifyProgress( (FLOAT)(Total - Length) / (FLOAT)Total ) )
				return 0;
		}
		return 1;
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	Huffman codec.
-----------------------------------------------------------------------------*/

static BYTE GShift[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};

class FCodecHuffman : public FCodec
{
private:
	struct FHuffman
	{
		INT Ch, Count;
		FHuffman* Child[2];
		TArray<BYTE> Bits;
		FHuffman( INT InCh = -1, INT InCount = 0 )
			: Ch(InCh), Count(InCount), Child()
		{
			Child[0] = NULL;
			Child[1] = NULL;
		}
		~FHuffman()
		{
			if (Child[0])
				for( INT i=0; i<2; i++ )
					delete Child[i];
		}
		void PrependBit( BYTE B )
		{
			Bits.Insert( 0 );
			Bits(0) = B;
			if (Child[0])
				for( INT i=0; i<2; i++ )
					Child[i]->PrependBit( B );
		}
		void WriteTable( FBitWriter& Writer )
		{
			if( Child[0] ) {
				Writer.WriteBit( 1 );
				for( INT i=0; i<2; i++ )
					Child[i]->WriteTable( Writer );
			}
			else
			{
				Writer.WriteBit( 0 );
				BYTE B = Ch;
				Writer << B;
			}
		}
		void ReadTable( FBitReader& Reader )
		{
			if( Reader.ReadBit() )
			{
				for( INT i=0; i<2; i++ )
				{
					FHuffman* Huffman = new FHuffman();
					Child[ i ] = Huffman;
					Huffman->ReadTable( Reader );
				}
			}
			else Ch = Arctor<BYTE>( Reader );
		}
	};
	static QSORT_RETURN CDECL CompareHuffman( const FHuffman** A, const FHuffman** B )
	{
		return (*B)->Count - (*A)->Count;
	}
public:
	UBOOL Encode( FArchive& In, FArchive& Out, FCodecNotify* Notify=NULL )
	{
		guard(FCodecHuffman::Encode);
		BYTE BufIn[BUF_SIZE];
		INT SavedPos = In.Tell();
		INT Total=In.TotalSize()-In.Tell();
		Out << Total;

		INT Counts[256] = {0};
		for( INT Length=Total; Length>0;) {
			INT BufLength = Min(Length, BUF_SIZE);
			In.Serialize( BufIn, BufLength );
			for( INT j=0; j<BufLength; j++ )
				Counts[BufIn[j]]++;
			Length -= BufLength;
		}
		In.Seek( SavedPos );

		// Compute character frequencies.
		TArray<FHuffman*> Huff(256);
		for( INT i=0; i<256; i++ )
			Huff(i) = new FHuffman(i, Counts[i]);
		TArray<FHuffman*> Index = Huff;

		// Build compression table.
		while( Huff.Num()>1 && Huff.Last()->Count==0 )
			delete Huff.Pop();
		INT BitCount = Huff.Num()*(8+1);
		while( Huff.Num()>1 )
		{
			FHuffman* Node  = new FHuffman();
			for( INT i=0; i<2; i++ )
			{
				FHuffman* Huffman = Huff.Pop();
				Node->Child[i] = Huffman;
				Huffman->PrependBit(i);
				Node->Count += Huffman->Count;
			}
			INT i, N = Huff.Num();
			for( i=0; i<N; i++ )
				if( Huff(i)->Count < Node->Count )
					break;
			Huff.Insert( i );
			Huff( i ) = Node;
			BitCount++;
		}
		FHuffman* Root = Huff.Pop();

		// Calc stats.
		for( INT i=0; i<256; i++ ) {
			INT Count = Counts[i];
			if (Count == 0) continue;
			BitCount += Index(i)->Bits.Num()*Count;
		}

		// Save table and bitstream.
		FBitWriter Writer( BitCount );
		Root->WriteTable( Writer );
		INT Pos = Writer.GetNumBits();
		BYTE* Data = Writer.GetData();
		for( INT Length=Total; Length>0;) {
			INT BufLength = Min(Length, BUF_SIZE);
			In.Serialize( BufIn, BufLength );
			for( INT j=0; j<BufLength; j++ ) {
				FHuffman* P = Index(BufIn[j]);
				for( INT i=0, N=P->Bits.Num(); i<N; i++ ) {
					if (P->Bits(i))
						Data[Pos>>3] |= GShift[Pos&7];
					Pos++;
				}
			}
			Length -= BufLength;
		}
		check(!Writer.IsError());
		check(Pos==BitCount);
		Out.Serialize( Data, (Pos + 7)>>3 );

		// Finish up.
		delete Root;
		return 1;

		unguard;
	}
	UBOOL Decode( FArchive& In, FArchive& Out, FCodecNotify* Notify=NULL )
	{
		guard(FCodecHuffman::Decode);
		INT Total;
		In << Total;
		TArray<BYTE> InArray( In.TotalSize()-In.Tell() );
		In.Serialize( &InArray(0), InArray.Num() );
		FBitReader Reader( &InArray(0), InArray.Num()*8 );
		FHuffman Root(-1);
		Root.ReadTable( Reader );
		INT Pos = Reader.GetPosBits();
		BYTE* Data = Reader.GetData();
		INT TotalBits = Reader.GetNumBits();
		TArray<BYTE> BufOut_(Total);
		BYTE* BufOut = &BufOut_(0);
		INT OutLength = 0;
		while( OutLength < Total )
		{	
			FHuffman* Node = &Root;
			while( Node->Ch==-1 ) {
				if (Pos >= TotalBits)
					return 0;
				Node = Node->Child[(Data[Pos>>3] & GShift[Pos&7]) != 0];
				Pos++;
			}
			BufOut[OutLength++] = Node->Ch;
			if( Notify && !(OutLength & 0xFFFFF) && !Notify->NotifyProgress( (FLOAT)(OutLength) / (FLOAT)Total ) )
				return 0;
		}
		InArray.Empty();
		Out.Serialize( BufOut, OutLength );
		// stijn: made this non-fatal in v469
		return TotalBits - Pos >= 0;
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	Move-to-front encoder.
-----------------------------------------------------------------------------*/

class FCodecMTF : public FCodec
{
public:
	UBOOL Encode( FArchive& In, FArchive& Out, FCodecNotify* Notify=NULL )
	{
		guard(FCodecMTF::Encode);
		BYTE List[256], BufIn[BUF_SIZE], BufOut[BUF_SIZE];
		INT Length = In.TotalSize();
		for( INT i=0; i<256; i++ ) List[i] = i;
		while( Length > 0 )
		{
			INT BufLength = Min(Length, BUF_SIZE);
			In.Serialize( BufIn, BufLength );
			for( INT j=0; j<BufLength; j++ )
			{
				BYTE B = BufIn[j];
				BYTE i = (BYTE)((BYTE*)memchr(&List[0], B, 256) - &List[0]);
				BufOut[j] = i;
				memmove(&List[1], &List[0], i);
				List[0] = B;
			}
			Out.Serialize( BufOut, BufLength );
			Length -= BufLength;
		}
		return 1;
		unguard;
	}
	UBOOL Decode( FArchive& In, FArchive& Out, FCodecNotify* Notify=NULL )
	{
		guard(FCodecMTF::Decode);
		BYTE List[256], BufIn[BUF_SIZE], BufOut[BUF_SIZE];
		INT Total = In.TotalSize();
		INT Length = Total;
		for( INT i=0; i<256; i++ ) List[i] = i;
		INT j = 0;
		while( Length > 0 )
		{
			INT BufLength = Min(Length, BUF_SIZE);
			In.Serialize( BufIn, BufLength );
			for( INT j=0; j<BufLength; j++ )
			{
				BYTE B = BufIn[j];
				BYTE C = List[B];
				BufOut[j] = C;
				memmove(&List[1], &List[0], B);
				List[0] = C;
			}
			Out.Serialize( BufOut, BufLength );
			Length -= BufLength;
			if( Notify && !(j++ & 0xF) && !Notify->NotifyProgress( (FLOAT)(Total - Length) / (FLOAT)Total ) )
				return 0;
		}
		return 1;
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	General compressor codec.
-----------------------------------------------------------------------------*/

class FCodecFull : public FCodec, FCodecNotify
{
private:
	TArray<FCodec*> Codecs;
	INT Stage;
	DOUBLE LastTime;
	INT LastStage;
	INT CodecsNum;
	FCodecNotify* Notify;
	inline UBOOL NotifyProgress( FLOAT Progress )
	{
		if (!Notify)
			return 1;
		DOUBLE CurTime = appSecondsNew();
		if (LastStage == Stage && CurTime - LastTime < 0.1)
			return 1;
		LastTime = CurTime;
		LastStage = Stage;
		if (!Notify->NotifyProgress( ((FLOAT)Stage / (FLOAT)CodecsNum) + (Progress / (FLOAT)CodecsNum) ) )
			return 0;
		return 1;
	}
	UBOOL Code( FArchive& In, FArchive& Out, INT Step, INT First, UBOOL (FCodec::*Func)(FArchive&,FArchive&,FCodecNotify*), FCodecNotify* InNotify=NULL )
	{
		guard(FCodecFull::Code);
		TArray<BYTE> InData, OutData;
		DOUBLE TotalTime=0.0;
		Notify = InNotify;
		LastTime = 0;
		LastStage = -1;
		CodecsNum = Codecs.Num();
		for( INT i=0; i<CodecsNum; i++ )
		{
			FBufferReader Reader(InData);
			FBufferWriter Writer(OutData);
			DOUBLE StartTime, EndTime;
			StartTime = appSeconds().GetDouble();
			Stage = i;
			if( !(Codecs(First + Step*i)->*Func)( *(i ? &Reader : &In), *(i<CodecsNum-1 ? &Writer : &Out), this ) )
				return 0;
			EndTime = appSeconds() - StartTime;
			TotalTime += EndTime;
			GWarn->Logf(TEXT("stage %d: %lf secs"), i, EndTime );
			if( i<CodecsNum-1 )
			{
				ExchangeArray(InData, OutData);
				if (i < CodecsNum - 2)
					OutData.EmptyNoRealloc();
				else
					OutData.Empty();
			}
		}
		GWarn->Logf(TEXT("Total: %f secs"), TotalTime );
		return 1;
		unguard;
	}
public:
	UBOOL Encode( FArchive& In, FArchive& Out, FCodecNotify* InNotify=NULL )
	{
		guard(FCodecFull::Encode);
		return Code( In, Out, 1, 0, &FCodec::Encode, InNotify );
		unguard;
	}
	UBOOL Decode( FArchive& In, FArchive& Out, FCodecNotify* InNotify=NULL )
	{
		guard(FCodecFull::Decode);
		return Code( In, Out, -1, Codecs.Num()-1, &FCodec::Decode, InNotify );
		unguard;
	}
	void AddCodec( FCodec* InCodec )
	{
		guard(FCodecFull::AddCodec);
		Codecs.AddItem( InCodec );
		unguard;
	}
	~FCodecFull() noexcept(false)
	{
		guard(FCodecFull::~FCodecFull);
		for( INT i=0; i<Codecs.Num(); i++ )
			delete Codecs( i );
		unguard;
	}
};

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (pop)
#endif

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
