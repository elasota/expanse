#pragma once

namespace expanse
{
	class Mutex;

	struct MutexLock final
	{
	public:
		MutexLock(Mutex *mutex);
		~MutexLock();

		void Release();

	private:
		Mutex *m_mutex;
	};
}
