#include "AsyncFileRequest_Win32.h"

namespace expanse
{
	AsyncFileRequest_Win32::AsyncFileRequest_Win32(AsyncFileSystem_Win32 *fs)
		: m_fs(fs)
	{
	}

	AsyncFileRequest_Win32::~AsyncFileRequest_Win32()
	{
		if (m_workItem != nullptr)
			m_fs->CancelItem(m_workItem);
	}

	void AsyncFileRequest_Win32::Init(CorePtr<AsyncFileSystem_Win32::WorkItem> &&workItem)
	{
		m_workItem = std::move(workItem);
	}


	bool AsyncFileRequest_Win32::IsFinished() const
	{
		if (m_workItem == nullptr)
			return false;

		return m_workItem->m_finished.load(std::memory_order_acquire);
	}

	ErrorCode AsyncFileRequest_Win32::GetErrorCode() const
	{
		return m_workItem->m_errorCode;
	}

	ArrayPtr<uint8_t> AsyncFileRequest_Win32::TakeResult()
	{
		return ArrayPtr<uint8_t>(std::move(m_workItem->m_result));
	}

	void AsyncFileRequest_Win32::TakeIdentifier(UTF8String_t &outDevice, UTF8String_t &outPath)
	{
		outDevice = std::move(m_workItem->m_id.m_device);
		outPath = std::move(m_workItem->m_id.m_path);
	}
}
