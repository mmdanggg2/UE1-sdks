/*=============================================================================
	FMallocMimalloc.h: Mimalloc memory allocator.
	Copyright 2024, OldUnreal. All Rights Reserved.

	Revision history:
		* Created by Buggie
=============================================================================*/

#include "mimalloc.h"

//
// Mimalloc memory allocator.
//
class FMallocMimalloc : public FMalloc
{
public:
	// FMalloc interface.
	void* Malloc(size_t Size, const TCHAR* Tag)
	{
		guard(FMallocMimalloc::Malloc);
		void* Ptr = mi_malloc_aligned(Size, VECTOR_ALIGNMENT);
		check(Ptr != NULL);
		return Ptr;
		unguardf((TEXT("%uz %ls"), Size, Tag));
	}
	void* Realloc(void* Ptr, size_t NewSize, const TCHAR* Tag)
	{
		guard(FMallocMimalloc::Realloc);
		// Buggie: Special case when size is zero.
		// mi_realloc_aligned internally use malloc if alignment <= sizeof(uintptr_t).
		// Which lead to get non-NULL result for malloc(0) as per standard.
		// Which need free after or it be memory leak.
		// And UE1 code expect NULL in such case as result.
		// So we do free instead and return NULL pointer.
		void* NewPtr;
		if (NewSize == 0)
		{
			mi_free_aligned(Ptr, VECTOR_ALIGNMENT);
			NewPtr = NULL;
		}
		else
		{
			NewPtr = mi_realloc_aligned(Ptr, NewSize, VECTOR_ALIGNMENT);
			check(NewPtr != NULL);
		}
		return NewPtr;
		unguardf((TEXT("%08X %uz %ls"), reinterpret_cast<PTRINT>(Ptr), NewSize, Tag));
	}
	void Free(void* Ptr)
	{
		guard(FMallocMimalloc::Free);
		mi_free_aligned(Ptr, VECTOR_ALIGNMENT);
		unguardf((TEXT("%08X"), reinterpret_cast<PTRINT>(Ptr)));
	}
	void DumpAllocs() {}
	void HeapCheck() {}
	void Init() {}
	void Exit() {}
};
