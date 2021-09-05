#include "CorePtr.h"

#include "CoreObject.h"
#include "Mem.h"

namespace expanse
{
	void CorePtrBase::ReleaseObject(CoreObject *object)
	{
		EXP_ASSERT(object != nullptr);
		object->~CoreObject();
		Mem::Release(object);
	}
}
