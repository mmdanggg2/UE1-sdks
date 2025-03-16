/*=============================================================================
	TArrayExt.h

	TArray subclass with more specific functionality.

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/


template<class T> class TArrayExt : public TArray<T>
{
public:
//	using TArray::TArray;

	TArrayExt& operator=( const TArray<T>& Other )
	{
		TArray<T>::operator=(Other);
		return *this;
	}

	T& AddGetRef()
	{
		INT i = TArray<T>::Add();
		return this->operator()(i);
	}

	INT RemoveLastNoCheck()
	{
		INT i = TArray<T>::ArrayNum - 1;
		if( TTypeInfo<T>::NeedsDestructor() )
			(&(*this)(i))->~T();
		TArray<T>::ArrayNum = i;
		return i;
	}

	INT RemoveFastNoCheck( INT i)
	{
		INT Last = TArray<T>::ArrayNum - 1;
		if( TTypeInfo<T>::NeedsDestructor() )
		{
			(&(*this)(i))->~T();
#if USES_TRAWDATA
			TRawData<T>::Copy( ((*this)(i)), ((*this)(Last)) );
#else
			appMemcpy((&(*this)(i)), (&(*this)(Last)), sizeof(T));
#endif
		}
		else
			((*this)(i)) = ((*this)(Last));
		TArray<T>::ArrayNum = Last;
		return i;
	}
};
