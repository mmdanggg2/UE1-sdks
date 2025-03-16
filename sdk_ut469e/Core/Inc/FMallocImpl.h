/*=============================================================================
	FMallocImpl.h: FMalloc implementation header.
	Copyright 2024, OldUnreal. All Rights Reserved.

	Revision history:
		* Created by Buggie
=============================================================================*/

#if OLDUNREAL_USE_MIMALLOC
#include "FMallocMimalloc.h"
#define FMallocImpl FMallocMimalloc
#else
#include "FMallocAnsi.h"
#define FMallocImpl FMallocAnsi
#endif

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
