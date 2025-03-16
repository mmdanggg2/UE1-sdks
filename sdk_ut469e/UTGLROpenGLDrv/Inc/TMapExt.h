/*=============================================================================
	TMapExt.h

	TMap subclass with more specific functionality.

	Revision history:
	* Created by Fernando Velazquez
=============================================================================*/

template< class TK, class TI > class TMapExt : public TMap<TK,TI>
{
public:
	TMapExt& operator=( const TMapExt& Other )
	{
		TMapBase<TK,TI>::operator=( Other );
		return *this;
	}

	TI& SetNoFind( typename TTypeInfo<TK>::ConstInitType InKey, typename TTypeInfo<TI>::ConstInitType InValue )
	{
		return this->TMapBase<TK,TI>::Add( InKey, InValue );
	}

	TI& FindOrSet( typename TTypeInfo<TK>::ConstInitType InKey, typename TTypeInfo<TI>::ConstInitType DefaultValue )
	{
		TI* Found = this->Find(InKey);
		if ( Found )
			return *Found;
		return this->SetNoFind( InKey, DefaultValue);
	}
};
