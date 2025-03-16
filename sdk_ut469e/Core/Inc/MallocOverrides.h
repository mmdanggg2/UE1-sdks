#if INLINE_OPERATORS
#define MAYBE_INLINE __forceinline
#else
#define MAYBE_INLINE
#endif

#include <new>

//
// C++ style memory allocation.
//
MAYBE_INLINE void* operator new(size_t Size)
{
	return appMalloc(Size, TEXT("new"));
}

MAYBE_INLINE void operator delete(void* Ptr) throw()
{
	appFree(Ptr);
}

MAYBE_INLINE void operator delete  (void* Ptr, const std::nothrow_t& Tag) noexcept
{
	appFree(Ptr);
}

MAYBE_INLINE void* operator new[](size_t Size, const TCHAR* Tag)
{
	return appMalloc(Size, Tag);
}

MAYBE_INLINE void* operator new[](size_t Size)
{
	return appMalloc(Size, TEXT("new"));
}

MAYBE_INLINE void operator delete[](void* Ptr) throw()
{
	appFree(Ptr);
}

MAYBE_INLINE void operator delete[](void* Ptr, const std::nothrow_t& Tag) noexcept
{
	appFree(Ptr);
}

MAYBE_INLINE void* operator new  (size_t Size, const std::nothrow_t& Tag) noexcept
{
	return appMalloc(Size, TEXT("new"));
}

MAYBE_INLINE void* operator new[](size_t Size, const std::nothrow_t& Tag) noexcept
{
	return appMalloc(Size, TEXT("new"));
}

//
// C++14-specific overrides
//
#if ((__cplusplus >= 201402L || _MSC_VER >= 1916))
MAYBE_INLINE void operator delete(void* Ptr, std::size_t Size) noexcept
{
	appFreeSize(Ptr, Size);
}

MAYBE_INLINE void operator delete[](void* Ptr, std::size_t Size) noexcept
{
	appFreeSize(Ptr, Size);
}
#endif

//
// C++17-specific overrides
//
#if (__cplusplus > 201402L && defined(__cpp_aligned_new)) && (!defined(__GNUC__) || (__GNUC__ > 5))
MAYBE_INLINE void operator delete  (void* Ptr, std::align_val_t Alignment) noexcept
{
	appFreeAligned(Ptr, Alignment);
}

MAYBE_INLINE void operator delete[](void* Ptr, std::align_val_t Alignment) noexcept
{
	appFreeAligned(Ptr, Alignment);
}

MAYBE_INLINE void operator delete  (void* Ptr, std::size_t Size, std::align_val_t Alignment) noexcept
{
	appFreeSizeAligned(Ptr, Size, Alignment);
}

MAYBE_INLINE void operator delete[](void* Ptr, std::size_t Size, std::align_val_t Alignment) noexcept
{
	appFreeSizeAligned(Ptr, Size, Alignment);
}

MAYBE_INLINE void operator delete  (void* Ptr, std::align_val_t Alignment, const std::nothrow_t&) noexcept
{
	appFreeAligned(Ptr, Alignment);
}

MAYBE_INLINE void operator delete[](void* Ptr, std::align_val_t Alignment, const std::nothrow_t&) noexcept
{
	appFreeAligned(Ptr, Alignment);
}

MAYBE_INLINE void* operator new(std::size_t Size, std::align_val_t Alignment)   noexcept(false)
{
	return appMallocAligned(Size, Alignment, TEXT("new"));
}

MAYBE_INLINE void* operator new[](std::size_t Size, std::align_val_t Alignment) noexcept(false)
{
	return appMallocAligned(Size, Alignment, TEXT("new"));
}

MAYBE_INLINE void* operator new  (std::size_t Size, std::align_val_t Alignment, const std::nothrow_t&) noexcept
{
	return appMallocAligned(Size, Alignment, TEXT("new"));
}

MAYBE_INLINE void* operator new[](std::size_t Size, std::align_val_t Alignment, const std::nothrow_t&) noexcept
{
	return appMallocAligned(Size, Alignment, TEXT("new"));
}
#endif