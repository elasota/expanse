#pragma once

#include "Thread.h"

#include "IncludeWindows.h"

namespace expanse
{
	class ThreadEvent;

	class Thread_Win32 final : public Thread
	{
	public:
		Thread_Win32();
		~Thread_Win32() override;

		virtual bool IsRunning() const override;
		virtual void WaitForExit() const override;
		virtual int GetExitCode() const override;
		virtual ThreadID_t GetID() const override;

	private:
		friend class Thread;

		struct ThreadStartData
		{
			ThreadFunc_t m_threadFunc;
			void *m_userdata;
			const wchar_t *m_name;
			ThreadEvent *m_startedEvent;
		};

		void Init(HANDLE handle, DWORD id);

		static DWORD WINAPI ThreadStart(LPVOID lpThreadParameter);

		HANDLE m_handle;
		DWORD m_id;
	};
}
