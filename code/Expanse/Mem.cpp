#include "Mem.h"

namespace expanse
{
	void ObjectAllocator::Delete(CorePtrBase &corePtr)
	{
		CoreObject *obj = corePtr.m_object;
		if (obj)
		{
			corePtr.m_object = nullptr;
			obj->~CoreObject();
			Mem::Release(obj);
		}
	}
}
