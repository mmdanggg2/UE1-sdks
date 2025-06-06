/*=============================================================================
	FCodec.h: Data compression codecs.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Tim Sweeney.
=============================================================================*/

#ifndef FCODEC_H
#define FCODEC_H

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (push,OBJECT_ALIGNMENT)
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
	virtual UBOOL Encode( FArchive& In, FArchive& Out, FCodecNotify* Notify=NULL ) _VF_BASE_RET(0);
	virtual UBOOL Decode( FArchive& In, FArchive& Out, FCodecNotify* Notify=NULL ) _VF_BASE_RET(0);
};

/*-----------------------------------------------------------------------------
	Burrows-Wheeler inspired data compressor.
-----------------------------------------------------------------------------*/

class FCodecBWT : public FCodec
{
private:
	enum {MAX_BUFFER_SIZE=0x40000}; /* Hand tuning suggests this is an ideal size */
	CORE_API static BYTE* CompressBuffer;
	CORE_API static INT CompressLength;
	static INT ClampedBufferCompare( const INT* P1, const INT* P2 )
	{
		guardSlow(FCodecBWT::ClampedBufferCompare);
		BYTE* B1 = CompressBuffer + *P1;
		BYTE* B2 = CompressBuffer + *P2;
		for( INT Count=CompressLength-Max(*P1,*P2); Count>0; Count--,B1++,B2++ )
		{
			if( *B1 < *B2 )
				return -1;
			else if( *B1 > *B2 )
				return 1;
		}
		return *P1 - *P2;
		unguardSlow;
	}
public:
	UBOOL Encode( FArchive& In, FArchive& Out, FCodecNotify* Notify=NULL )
	{
		guard(FCodecBWT::Encode);
		TArray<BYTE> CompressBufferArray(MAX_BUFFER_SIZE);
		TArray<INT>  CompressPosition   (MAX_BUFFER_SIZE+1);
		CompressBuffer = &CompressBufferArray(0);
		INT i, First=0, Last=0;
		while( !In.AtEnd() )
		{
			CompressLength = Min<INT>( In.TotalSize()-In.Tell(), MAX_BUFFER_SIZE );
			In.Serialize( CompressBuffer, CompressLength );
			for( i=0; i<CompressLength+1; i++ )
				CompressPosition(i) = i;
			appQsort( &CompressPosition(0), CompressLength+1, sizeof(INT), (QSORT_COMPARE)ClampedBufferCompare );
			for( i=0; i<CompressLength+1; i++ )
				if( CompressPosition(i)==1 )
					First = i;
				else if( CompressPosition(i)==0 )
					Last = i;
			Out << CompressLength << First << Last;
			for( i=0; i<CompressLength+1; i++ )
				Out << CompressBuffer[CompressPosition(i)?CompressPosition(i)-1:0];
			//GWarn->Logf(TEXT("Compression table"));
			//for( i=0; i<CompressLength+1; i++ )
			//	GWarn->Logf(TEXT("    %03i: %ls"),CompressPosition(i)?CompressBuffer[CompressPosition(i)-1]:-1,appFromAnsi((ANSICHAR*)CompressBuffer+CompressPosition(i)));
		}
		return 1;
		unguard;
	}
	UBOOL Decode( FArchive& In, FArchive& Out, FCodecNotify* Notify=NULL )
	{
		guard(FCodecBWT::Decode);
		TArray<BYTE> DecompressBuffer(MAX_BUFFER_SIZE+1);
		TArray<INT>  Temp(MAX_BUFFER_SIZE+1);
		INT DecompressLength, DecompressCount[256+1], RunningTotal[256+1], i, j;
		while( !In.AtEnd() )
		{
			INT First, Last;
			In << DecompressLength << First << Last;
			check(DecompressLength<=MAX_BUFFER_SIZE+1);
			check(DecompressLength<=In.TotalSize()-In.Tell());
			In.Serialize( &DecompressBuffer(0), ++DecompressLength );
			for( i=0; i<257; i++ )
				DecompressCount[ i ]=0;
			for( i=0; i<DecompressLength; i++ )
				DecompressCount[ i!=Last ? DecompressBuffer(i) : 256 ]++;
			INT Sum = 0;
			for( i=0; i<257; i++ )
			{
				RunningTotal[i] = Sum;
				Sum += DecompressCount[i];
				DecompressCount[i] = 0;
			}
			for( i=0; i<DecompressLength; i++ )
			{
				INT Index = i!=Last ? DecompressBuffer(i) : 256;
				Temp(RunningTotal[Index] + DecompressCount[Index]++) = i;
			}
			for( i=First,j=0 ; j<DecompressLength-1; i=Temp(i),j++ )
				Out << DecompressBuffer(i);

			if( Notify && !Notify->NotifyProgress( (FLOAT)In.Tell() / (FLOAT)In.TotalSize() ) )
				return 0;
		}
		return 1;
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	RLE compressor.
-----------------------------------------------------------------------------*/

class FCodecRLE : public FCodec
{
private:
	enum {RLE_LEAD=5};
	UBOOL EncodeEmitRun( FArchive& Out, BYTE Char, BYTE Count )
	{
		for( INT Down=Min<INT>(Count,RLE_LEAD); Down>0; Down-- )
			Out << Char;
		if( Count>=RLE_LEAD )
			Out << Count;
		return 1;
	}
public:
	UBOOL Encode( FArchive& In, FArchive& Out, FCodecNotify* Notify=NULL )
	{
		guard(FCodecRLE::Encode);
		BYTE PrevChar=0, PrevCount=0, B;
		while( !In.AtEnd() )
		{
			In << B;
			if( B!=PrevChar || PrevCount==255 )
			{
				EncodeEmitRun( Out, PrevChar, PrevCount );
				PrevChar  = B;
				PrevCount = 0;
			}
			PrevCount++;
		}
		EncodeEmitRun( Out, PrevChar, PrevCount );
		return 1;
		unguard;
	}
	UBOOL Decode( FArchive& In, FArchive& Out, FCodecNotify* Notify=NULL )
	{
		guard(FCodecRLE::Decode);
		INT Count=0;
		BYTE PrevChar=0, B, C;
		while( !In.AtEnd() )
		{
			In << B;
			Out << B;
			if( B!=PrevChar )
			{
				PrevChar = B;
				Count    = 1;
			}
			else if( ++Count==RLE_LEAD )
			{
				In << C;
				check(C>=2);
				while( C-->RLE_LEAD )
					Out << B;
				Count = 0;
			}
			if( Notify && !Notify->NotifyProgress( (FLOAT)In.Tell() / (FLOAT)In.TotalSize() ) )
				return 0;
		}
		return 1;
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	Huffman codec.
-----------------------------------------------------------------------------*/

class FCodecHuffman : public FCodec
{
private:
	struct FHuffman
	{
		INT Ch, Count;
		TArray<FHuffman*> Child;
		TArray<BYTE> Bits;
		FHuffman( INT InCh )
		: Ch(InCh), Count(0)
		{
		}
		~FHuffman()
		{
			for( INT i=0; i<Child.Num(); i++ )
				delete Child( i );
		}
		void PrependBit( BYTE B )
		{
			Bits.Insert( 0 );
			Bits(0) = B;
			for( INT i=0; i<Child.Num(); i++ )
				Child(i)->PrependBit( B );
		}
		void WriteTable( FBitWriter& Writer )
		{
			Writer.WriteBit( Child.Num()!=0 );
			if( Child.Num() )
				for( INT i=0; i<Child.Num(); i++ )
					Child(i)->WriteTable( Writer );
			else
			{
				BYTE B = Ch;
				Writer << B;
			}
		}
		void ReadTable( FBitReader& Reader )
		{
			if( Reader.ReadBit() )
			{
				Child.Add( 2 );
				for( INT i=0; i<Child.Num(); i++ )
				{
					Child( i ) = new FHuffman( -1 );
					Child( i )->ReadTable( Reader );
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
		INT SavedPos = In.Tell();
		INT Total=0, i;

		// Compute character frequencies.
		TArray<FHuffman*> Huff(256);
		for( i=0; i<256; i++ )
			Huff(i) = new FHuffman(i);
		TArray<FHuffman*> Index = Huff;
		while( !In.AtEnd() )
			Huff(Arctor<BYTE>(In))->Count++, Total++;
		In.Seek( SavedPos );
		Out << Total;

		// Build compression table.
		while( Huff.Num()>1 && Huff.Last()->Count==0 )
			delete Huff.Pop();
		INT BitCount = Huff.Num()*(8+1);
		while( Huff.Num()>1 )
		{
			FHuffman* Node  = new FHuffman( -1 );
			Node->Child.Add( 2 );
			for( i=0; i<Node->Child.Num(); i++ )
			{
				Node->Child(i) = Huff.Pop();
				Node->Child(i)->PrependBit(i);
				Node->Count += Node->Child(i)->Count;
			}
			for( i=0; i<Huff.Num(); i++ )
				if( Huff(i)->Count < Node->Count )
					break;
			Huff.Insert( i );
			Huff( i ) = Node;
			BitCount++;
		}
		FHuffman* Root = Huff.Pop();

		// Calc stats.
		while( !In.AtEnd() )
			BitCount += Index(Arctor<BYTE>(In))->Bits.Num();
		In.Seek( SavedPos );

		// Save table and bitstream.
		FBitWriter Writer( BitCount );
		Root->WriteTable( Writer );
		while( !In.AtEnd() )
		{
			FHuffman* P = Index(Arctor<BYTE>(In));
			for( INT j=0; j<P->Bits.Num(); j++ )
				Writer.WriteBit( P->Bits(j) );
		}
		check(!Writer.IsError());
		check(Writer.GetNumBits()==BitCount);
		Out.Serialize( Writer.GetData(), Writer.GetNumBytes() );

		// Finish up.
		delete Root;
		return 1;

		unguard;
	}
	UBOOL Decode( FArchive& In, FArchive& Out, FCodecNotify* Notify=NULL )
	{
		guard(FCodecHuffman::Decode);
		INT Total, Remaining;
		In << Total;
		Remaining = Total;
		TArray<BYTE> InArray( In.TotalSize()-In.Tell() );
		In.Serialize( &InArray(0), InArray.Num() );
		FBitReader Reader( &InArray(0), InArray.Num()*8 );
		FHuffman Root(-1);
		Root.ReadTable( Reader );
		while( Remaining-- > 0 )
		{
			check(!Reader.AtEnd());
			FHuffman* Node;
			for( Node=&Root; Node->Ch==-1; Node=Node->Child(Reader.ReadBit()) );
			BYTE B = Node->Ch;
			Out << B;
			if( Notify && !Notify->NotifyProgress( (FLOAT)(Total-Remaining) / (FLOAT)Total ) )
				return 0;
		}
		return 1;
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
		BYTE List[256], B, C;
		INT i;
		for( i=0; i<256; i++ )
			List[i] = i;
		while( !In.AtEnd() )
		{
			In << B;
			for( i=0; i<256; i++ )
				if( List[i]==B )
					break;
			check(i<256);
			C = i;
			Out << C;
			INT NewPos=0;
			for( ; i>NewPos; i-- )
				List[i]=List[i-1];
			List[NewPos] = B;
		}
		return 1;
		unguard;
	}
	UBOOL Decode( FArchive& In, FArchive& Out, FCodecNotify* Notify=NULL )
	{
		guard(FCodecMTF::Decode);
		BYTE List[256], B, C;
		INT i;
		for( i=0; i<256; i++ )
			List[i] = i;
		while( !In.AtEnd() )
		{
			In << B;
			C = List[B];
			Out << C;
			INT NewPos=0;
			for( i=B; i>NewPos; i-- )
				List[i]=List[i-1];
			List[NewPos] = C;
			if( Notify && !Notify->NotifyProgress( (FLOAT)In.Tell() / (FLOAT)In.TotalSize() ) )
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
	FCodecNotify* Notify;
	UBOOL NotifyProgress( FLOAT Progress )
	{
		if( Notify && !Notify->NotifyProgress( ((FLOAT)Stage / (FLOAT)Codecs.Num()) + (Progress / (FLOAT)Codecs.Num()) ) )
			return 0;
		return 1;
	}
	UBOOL Code( FArchive& In, FArchive& Out, INT Step, INT First, UBOOL (FCodec::*Func)(FArchive&,FArchive&,FCodecNotify*), FCodecNotify* InNotify=NULL )
	{
		guard(FCodecFull::Code);
		TArray<BYTE> InData, OutData;
		FLOAT TotalTime=0.f;
		Notify = InNotify;
		FOutputDevice* LogOut = (GIsEditor ? GWarn : GLog);
		for( INT i=0; i<Codecs.Num(); i++ )
		{
			FBufferReader Reader(InData);
			FBufferWriter Writer(OutData);
			FTime StartTime, EndTime;
			StartTime = appSeconds();
			Stage = i;
			if( !(Codecs(First + Step*i)->*Func)( *(i ? &Reader : &In), *(i<Codecs.Num()-1 ? &Writer : &Out), this ) )
				return 0;
			EndTime = appSeconds() - StartTime;
			TotalTime += EndTime.GetFloat();
			LogOut->Logf(NAME_Progress, TEXT("stage %d: %f secs"), i, EndTime.GetFloat() );
			if( i<Codecs.Num()-1 )
			{
				InData = OutData;
				OutData.Empty();
			}
		}
		LogOut->Logf(NAME_Progress, TEXT("Total: %f secs"), TotalTime );
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

#endif

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
