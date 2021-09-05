#pragma once

#include <cstddef>

namespace expanse
{
	struct IAllocator
	{
		virtual void *Alloc(size_t size, size_t alignment) = 0;
		virtual void Release(void *ptr) = 0;
		virtual void *Realloc(void *ptr, size_t newSize, size_t alignment) = 0;
	};
}
