// APathNodeIterator.h

	void              BuildPath( FVector Start, FVector End );
	ANavigationPoint* GetFirst();
	ANavigationPoint* GetPrevious();
	ANavigationPoint* GetCurrent();
	ANavigationPoint* GetNext();
	ANavigationPoint* GetLast();
	ANavigationPoint* GetLastVisible();

	virtual bool      IsPathValid( FVector           Start, FVector           End, bool FullCheck = false );
	virtual bool      IsPathValid( ANavigationPoint* Start, FVector           End, bool FullCheck = false );
	virtual bool      IsPathValid( FVector           Start, ANavigationPoint* End, bool FullCheck = false );
	virtual bool      IsPathValid( FReachSpec* ReachSpec );
	virtual bool      IsPathHeightValid( FVector Start, FVector End );

	virtual int       CalcNodeCost( FReachSpec* ReachSpec );
// end of APathNodeIterator.h
