/*=============================================================================
	FCharWriter.h
	
	Simple ansi string writer

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

#define END_LINE "\n"

class FCharWriter
{
public:
	TArray<ANSICHAR> Data;

	FCharWriter()
	{
		Data.Reserve(1000);
		Data.AddNoCheck();
		Data(0) = '\0';
	}

#if __cplusplus > 201103L || _MSVC_LANG > 201103L
	template < INT Size > FCharWriter& operator<<( const char(&Input)[Size])
	{
		if ( Size > 1 )
		{
			INT i = Data.Add(Size-1) - 1;
			appMemcpy( &Data(i), Input, Size);
		}
		check(Data.Last() == '\0');
		return *this;
	}
#endif

	FCharWriter& operator<<( const char* Input)
	{
		const char* InputEnd = Input;
		while ( *InputEnd != '\0' )
			InputEnd++;
		if ( InputEnd != Input )
		{
			INT Len = (INT)(InputEnd - Input);
			INT i = Data.Add(Len) - 1;
			check(Len > 0);
			check(Len < 4096);
			appMemcpy( &Data(i), Input, Len+1);
		}
		check(Data.Last() == '\0');
		return *this;
	}

	FCharWriter& operator<<( INT Input)
	{
		TCHAR Buffer[16];
		appSprintf( Buffer, TEXT("%i"), Input);
		return *this << appToAnsi(Buffer);
	}


	const char* operator*()
	{
		return &Data(0);
	}

	GLsizei Length()
	{
		return (GLsizei)(Data.Num() - 1);
	}

	void Reset()
	{
		Data.EmptyNoRealloc();
		Data.AddNoCheck();
		Data(0) = '\0';
	}
};
