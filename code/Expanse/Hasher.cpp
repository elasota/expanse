#include "Hasher.h"

#include "xxhash32.h"

namespace expanse
{
	namespace HashUtil
	{
		Hash_t ComputePODHash(const void *data, size_t size)
		{
			XXHash32 myhash(0);
			myhash.add(data, size);
			return static_cast<Hash_t>(myhash.hash());
		}
	}
}
