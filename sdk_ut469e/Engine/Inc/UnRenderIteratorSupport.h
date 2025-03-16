/*----------------------------------------------------------------------------
	ActorBuffer.
----------------------------------------------------------------------------*/

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack(push,OBJECT_ALIGNMENT)
#endif

struct ActorBuffer
{
	BYTE Buffer[ sizeof(AActor) ];
};

/*----------------------------------------------------------------------------
	FActorNode.
----------------------------------------------------------------------------*/

class ENGINE_API FActorNode
{
public:
	ActorBuffer ActorProxy;
	FActorNode* NextNode;

	// Constructors.
	FActorNode(): NextNode( NULL ) {}
	~FActorNode() 
	{
		if( NextNode != NULL )
			delete NextNode;
	}
};

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (pop)
#endif
