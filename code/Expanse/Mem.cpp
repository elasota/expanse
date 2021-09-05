#include "Mem.h"

#include "CoreObject.h"

namespace expanse
{
	void ObjectAllocator::DeleteObject(CoreObject *obj)
	{
		if (!obj)
			return;

		IAllocator *alloc = obj->GetCoreObjectAllocator();
		obj->~CoreObject();
		alloc->Release(obj);
	}
}
