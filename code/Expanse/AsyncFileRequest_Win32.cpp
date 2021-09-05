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
		{
			m_fs->CancelItem(m_workItem);
		}
	}

	void AsyncFileRequest_Win32::Init(CorePtr<AsyncFileSystem_Win32::WorkItem> &&workItem)
	{
		m_workItem = std::move(workItem);
	}
}
