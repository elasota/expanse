#pragma once

#include "AsyncFileSystem.h"
#include "CorePtr.h"
#include "String.h"

#include <atomic>

namespace expanse
{
	class AsyncFileRequest_Win32;
	class SynchronousFileSystem;
	class Mutex;
	class Thread;
	class ThreadEvent;

	class AsyncFileSystem_Win32 final : public AsyncFileSystem
	{
	public:
		AsyncFileSystem_Win32(SynchronousFileSystem *syncFileSystem);
		~AsyncFileSystem_Win32();

		Result Initialize() override;
		ResultRV<CorePtr<AsyncFileRequest>> Retrieve(const UTF8StringView_t &device, const UTF8StringView_t &path) override;

		enum WorkItemState
		{
			kQueued,
			kInProgress,
			kFailed,
			kCancelled,
			kFinished,
		};

		struct WorkItemIdentifier
		{
			UTF8String_t m_device;
			UTF8String_t m_path;
		};

		struct WorkItem final : public CoreObject
		{
			WorkItem();

			ArrayPtr<uint8_t> m_result;
			WorkItemState m_itemState;	// Requires lock
			WorkItemIdentifier m_id;
			WorkItem *m_prev;
			WorkItem *m_next;
			std::atomic<int> m_finished;
		};

		void CancelItem(WorkItem *workItem);

	public:
		static int StaticThreadFunc(void *self);
		int ThreadFunc();

		void TryLoadWorkItem(const WorkItemIdentifier &identifier, WorkItemState &outState, ArrayPtr<uint8_t> &outContents);
		Result TryLoadWorkItemChecked(const WorkItemIdentifier &identifier, ArrayPtr<uint8_t> &outContents);

		CorePtr<Thread> m_ioThread;
		CorePtr<ThreadEvent> m_ioWakeEvent;
		CorePtr<Mutex> m_queueMutex;

		SynchronousFileSystem *m_syncFileSystem;
		WorkItem *m_queueHead;
		WorkItem *m_queueTail;
		WorkItem *m_inProgressItem;
		bool m_initialized;
		bool m_isQuitting;
	};
}
