#pragma once

#include "CoreObject.h"
#include "StringProto.h"

#include <cstdint>

namespace expanse
{
	template<class T> struct CorePtr;
	template<class T> struct ResultRV;
	struct IAllocator;

	class ThreadEvent : public CoreObject
	{
	public:
		virtual void WaitTimed(uint32_t msec) = 0;
		virtual void Wait() = 0;
		virtual void Set() = 0;
		virtual void Reset() = 0;

		static ResultRV<CorePtr<ThreadEvent>> Create(IAllocator *alloc, const UTF8StringView_t &name, bool autoReset, bool startSignaled);

	protected:
		ThreadEvent();
	};
}


namespace expanse
{
	inline ThreadEvent::ThreadEvent()
	{
	}
}
