#pragma once

#include "Mutex.h"

#include "IncludeWindows.h"

namespace expanse
{
	class Mutex_Win32 final : public Mutex
	{
	public:
		Mutex_Win32();
		~Mutex_Win32() override;

		void Lock() override;
		bool TryLock() override;
		void Unlock() override;

		static ResultRV<CorePtr<Mutex>> Create();

	private:
		CRITICAL_SECTION m_criticalSection;
	};
}
