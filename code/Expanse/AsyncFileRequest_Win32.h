#pragma once

#include "AsyncFileRequest.h"
#include "AsyncFileSystem_Win32.h"

#include "CorePtr.h"

namespace expanse
{
	class AsyncFileSystem_Win32;
	class AsyncFileSystem_Win32_WorkItem;
	struct IAllocator;

	class AsyncFileRequest_Win32 final : public AsyncFileRequest
	{
	public:
		explicit AsyncFileRequest_Win32(AsyncFileSystem_Win32 *fs);
		~AsyncFileRequest_Win32();

		void Init(CorePtr<AsyncFileSystem_Win32::WorkItem> &&workItem);

		bool IsFinished() const override;
		ErrorCode GetErrorCode() const override;
		ArrayPtr<uint8_t> TakeResult() override;
		void TakeIdentifier(UTF8String_t &outDevice, UTF8String_t &outPath) override;

	private:
		AsyncFileSystem_Win32 *m_fs;
		CorePtr<AsyncFileSystem_Win32::WorkItem> m_workItem;
	};
}
