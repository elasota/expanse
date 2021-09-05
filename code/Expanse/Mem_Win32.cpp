#include "Mem.h"

#include <malloc.h>

namespace expanse
{
	void *Mem::Alloc(size_t size, size_t alignment)
	{
		return _aligned_malloc(size, alignment);
	}

	void Mem::Release(void *ptr)
	{
		return _aligned_free(ptr);
	}

	void *Mem::Realloc(void *ptr, size_t newSize, size_t alignment)
	{
		return _aligned_realloc(ptr, newSize, alignment);
	}
}
