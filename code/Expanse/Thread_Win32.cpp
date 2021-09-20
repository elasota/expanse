#include "Thread_Win32.h"

#include "ArrayPtr.h"
#include "ExpAssert.h"
#include "ResultRV.h"
#include "XString.h"
#include "ThreadEvent.h"
#include "WindowsUtils.h"

namespace expanse
{
	Thread_Win32::~Thread_Win32()
	{
		if (m_handle)
		{
			WaitForExit();
			CloseHandle(m_handle);
		}
	}

	bool Thread_Win32::IsRunning() const
	{
		EXP_ASSERT(m_handle);
		DWORD result = WaitForSingleObject(m_handle, 0);
		return result == WAIT_OBJECT_0;
	}

	void Thread_Win32::WaitForExit() const
	{
		EXP_ASSERT(m_handle);
		WaitForSingleObject(m_handle, INFINITE);
	}

	int Thread_Win32::GetExitCode() const
	{
		EXP_ASSERT(m_handle);

		DWORD exitCode = 0;
		BOOL hasExited = GetExitCodeThread(m_handle, &exitCode);
		EXP_ASSERT(hasExited);

		return static_cast<int>(exitCode);
	}

	int Thread_Win32::GetID() const
	{
		return m_id;
	}

	Thread_Win32::Thread_Win32()
		: m_handle(nullptr)
		, m_id(0)
	{
	}

	void Thread_Win32::Init(HANDLE handle, DWORD id)
	{
		m_handle = handle;
		m_id = id;
	}

	DWORD WINAPI Thread_Win32::ThreadStart(LPVOID lpThreadParameter)
	{
		const ThreadStartData *startData = static_cast<const ThreadStartData *>(lpThreadParameter);
		ThreadFunc_t threadFunc = startData->m_threadFunc;
		void *userData = startData->m_userdata;
		ThreadEvent *startEvent = startData->m_startedEvent;

		startEvent->Set();

		return static_cast<DWORD>(threadFunc(userData));
	}

	ResultRV<CorePtr<Thread>> Thread::CreateThread(IAllocator *alloc, ThreadFunc_t threadFunc, void *userData, const UTF8StringView_t &name)
	{
		CHECK_RV(ArrayPtr<wchar_t>, threadName, WindowsUtils::ConvertToWideChar(alloc, name));
		CHECK_RV(CorePtr<ThreadEvent>, startupEvent, ThreadEvent::Create(alloc, UTF8StringView_t("Thread Startup Event"), true, false));
		CHECK_RV(CorePtr<Thread_Win32>, thread, New<Thread_Win32>(alloc));

		Thread_Win32::ThreadStartData startData;
		startData.m_name = threadName.GetBuffer();
		startData.m_startedEvent = startupEvent;
		startData.m_threadFunc = threadFunc;
		startData.m_userdata = userData;

		DWORD threadID = 0;
		HANDLE hThread = ::CreateThread(nullptr, 1024 * 1024, Thread_Win32::ThreadStart, &startData, 0, &threadID);
		if (hThread == nullptr)
			return ErrorCode::kSystemError;

		startupEvent->Wait();

		thread->Init(hThread, threadID);

		return CorePtr<Thread>(std::move(thread));
	}
}
