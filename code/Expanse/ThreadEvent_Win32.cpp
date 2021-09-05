#include "ThreadEvent_Win32.h"

#include "ArrayPtr.h"
#include "Result.h"
#include "ResultRV.h"

#include "WindowsUtils.h"

namespace expanse
{
	ThreadEvent_Win32::ThreadEvent_Win32()
		: m_handle(nullptr)
	{
	}

	ThreadEvent_Win32::~ThreadEvent_Win32()
	{
		if (m_handle)
			CloseHandle(m_handle);
	}

	void ThreadEvent_Win32::WaitTimed(uint32_t msec)
	{
		WaitForSingleObject(m_handle, static_cast<DWORD>(msec));
	}

	void ThreadEvent_Win32::Wait()
	{
		WaitForSingleObject(m_handle, INFINITE);
	}

	void ThreadEvent_Win32::Set()
	{
		SetEvent(m_handle);
	}

	void ThreadEvent_Win32::Reset()
	{
		ResetEvent(m_handle);
	}

	void ThreadEvent_Win32::Init(HANDLE handle)
	{
		m_handle = handle;
	}

	ResultRV<CorePtr<ThreadEvent>> ThreadEvent::Create(IAllocator *alloc, const UTF8StringView_t &name, bool autoReset, bool startSignaled)
	{
		CHECK_RV(ArrayPtr<wchar_t>, nameWStr, WindowsUtils::ConvertToWideChar(alloc, name));
		CHECK_RV(CorePtr<ThreadEvent_Win32>, threadEvent, New<ThreadEvent_Win32>(alloc));

		HANDLE eventHdl = CreateEventW(nullptr, ((autoReset) ? FALSE : TRUE), ((startSignaled) ? TRUE : FALSE), nameWStr.GetBuffer());
		if (eventHdl == nullptr)
			return ErrorCode::kSystemError;

		threadEvent->Init(eventHdl);

		return CorePtr<ThreadEvent>(std::move(threadEvent));
	}
}
