#pragma once

#include "ThreadEvent.h"

#include "IncludeWindows.h"

namespace expanse
{
	class ThreadEvent_Win32 final : public ThreadEvent
	{
	public:
		ThreadEvent_Win32();
		~ThreadEvent_Win32() override;

		void WaitTimed(uint32_t msec) override;
		void Wait() override;
		void Set() override;
		void Reset() override;

	private:
		friend class ThreadEvent;

		void Init(HANDLE handle);

		HANDLE m_handle;
	};
}
