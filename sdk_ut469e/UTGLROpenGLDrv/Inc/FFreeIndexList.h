/*=============================================================================
	FFreeIndexList.h

	Array of indices, specialized for fast adding and removing of indices.

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/


class FFreeIndexList : protected TArray<INT>
{
public:
	using TArray::TArray;
	using TArray::AddItem;
	using TArray::Empty;
	using TArray::Num;
	~FFreeIndexList() { Empty(); }

	bool GetAndPop( INT& Index);
	void Populate( INT IndexCount);
};

/*-----------------------------------------------------------------------------
	FFreeIndexList.
-----------------------------------------------------------------------------*/

inline bool FFreeIndexList::GetAndPop( INT& Index)
{
	if ( this->ArrayNum > 0 )
	{
		Index = ((INT*)this->Data)[--(this->ArrayNum)];
		return true;
	}
	return false;
}

inline void FFreeIndexList::Populate( INT IndexCount)
{
	SetSize(IndexCount);
	for ( INT i=0; i<IndexCount; i++)
		this->operator()(i) = i;
}
