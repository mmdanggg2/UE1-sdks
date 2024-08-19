/*=============================================================================
	TemplateQueue.h

	Element queue.

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/


//
// Specialized round queue
//
template <typename T> class TRoundQueue 
{
public:
	TRoundQueue();
	
	void SetSize( INT NewSize);
	T& QueueNew();
	T& operator()( INT Index);

	INT Size() const                  { return Data.Num(); }
	INT GetPosition() const           { return Position.GetValue(); }

protected:
	TArray<T>        Data;
	TNumericTag<INT> Round;
	TNumericTag<INT> Position;
};


/*-----------------------------------------------------------------------------
	TRoundQueue.
-----------------------------------------------------------------------------*/

template<typename T>
inline TRoundQueue<T>::TRoundQueue()
{
}

template<typename T>
inline void TRoundQueue<T>::SetSize( INT NewSize)
{
	if ( NewSize != Data.Num() )
		Data.SetSize(NewSize);
	Position.Reset();
	Round.Reset();
}

template<typename T>
inline T& TRoundQueue<T>::QueueNew()
{
	check(Data.Num()); //checkSlow

	Position.Increment();
	if ( Position.GetValue() >= Data.Num() )
	{
		Position.Reset();
		Round.Increment();
	}
	return Data(Position.GetValue());
}

template<typename T>
inline T& TRoundQueue<T>::operator()(INT Index)
{
	return Data(Index);
}

