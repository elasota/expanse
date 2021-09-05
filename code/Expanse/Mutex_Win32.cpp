#include "Mutex_Win32.h"

#include "Mem.h"

namespace expanse
{
	Mutex_Win32::Mutex_Win32()
	{
		InitializeCriticalSection(&m_criticalSection);
	}

	Mutex_Win32::~Mutex_Win32()
	{
		DeleteCriticalSection(&m_criticalSection);
	}

	void Mutex_Win32::Lock()
	{
		EnterCriticalSection(&m_criticalSection);
	}

	bool Mutex_Win32::TryLock()
	{
		return (TryEnterCriticalSection(&m_criticalSection) != FALSE);
	}

	void Mutex_Win32::Unlock()
	{
		LeaveCriticalSection(&m_criticalSection);
	}

	ResultRV<CorePtr<Mutex>> Mutex::Create()
	{
		CHECK_RV(CorePtr<Mutex_Win32>, mutex, New<Mutex_Win32>());
		return CorePtr<Mutex>(std::move(mutex));
	}
}
