#include "MutexLock.h"
#include "Mutex.h"

namespace expanse
{
	MutexLock::MutexLock(Mutex *mutex)
		: m_mutex(mutex)
	{
		mutex->Lock();
	}

	MutexLock::~MutexLock()
	{
		this->Release();
	}

	void MutexLock::Release()
	{
		if (m_mutex)
		{
			m_mutex->Unlock();
			m_mutex = nullptr;
		}
	}
}
