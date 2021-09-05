#include "AsyncFileSystem_Win32.h"

#include "AsyncFileRequest_Win32.h"
#include "CorePtr.h"
#include "ExpAssert.h"
#include "FileStream.h"
#include "Mutex.h"
#include "MutexLock.h"
#include "Result.h"
#include "ResultRV.h"
#include "SynchronousFileSystem.h"
#include "Thread.h"
#include "ThreadEvent.h"

// This works via an IO thread.  AsyncFileRequest lifetimes must exist within the async file system lifetime.
// AsyncFileRequests own the WorkItem but must lock the async file system to modify it.
// A WorkItem is either in the queue, the current work item, or orphaned.

namespace expanse
{
	AsyncFileSystem_Win32::AsyncFileSystem_Win32(SynchronousFileSystem *syncFileSystem)
		: m_syncFileSystem(syncFileSystem)
		, m_queueHead(nullptr)
		, m_queueTail(nullptr)
		, m_inProgressItem(nullptr)
		, m_initialized(false)
	{
	}

	AsyncFileSystem_Win32::~AsyncFileSystem_Win32()
	{
		{
			MutexLock lock(m_queueMutex);

			for (WorkItem *workItem = m_queueHead; workItem; workItem = workItem->m_next)
				workItem->m_itemState = WorkItemState::kCancelled;

			if (m_inProgressItem)
				m_inProgressItem->m_itemState = WorkItemState::kCancelled;

			m_queueHead = nullptr;
			m_queueTail = nullptr;
			m_inProgressItem = nullptr;

			m_isQuitting = true;
		}

		m_ioWakeEvent->Set();

		// Stop the IO thread and wait
		m_ioThread = nullptr;
	}

	Result AsyncFileSystem_Win32::Initialize()
	{
		IAllocator *alloc = GetCoreObjectAllocator();

		CHECK_RV(CorePtr<ThreadEvent>, ioWakeEvent, ThreadEvent::Create(alloc, UTF8StringView_t(""), true, false));
		CHECK_RV(CorePtr<Mutex>, queueMutex, Mutex::Create(alloc));

		m_queueMutex = std::move(queueMutex);
		m_ioWakeEvent = std::move(ioWakeEvent);
		
		CHECK_RV(CorePtr<Thread>, ioThread, Thread::CreateThread(alloc, StaticThreadFunc, this, UTF8StringView_t("AsyncFileSystem")));

		m_initialized = true;
		return ErrorCode::kOK;
	}

	ResultRV<CorePtr<AsyncFileRequest>> AsyncFileSystem_Win32::Retrieve(const UTF8StringView_t &device, const UTF8StringView_t &path)
	{
		EXP_ASSERT(m_initialized);

		IAllocator *alloc = GetCoreObjectAllocator();

		CHECK_RV(UTF8String_t, deviceCopy, device.CloneToString(alloc));
		CHECK_RV(UTF8String_t, pathCopy, path.CloneToString(alloc));
		CHECK_RV(CorePtr<WorkItem>, workItem, New<WorkItem>(alloc));
		CHECK_RV(CorePtr<AsyncFileRequest_Win32>, asyncRequest, New<AsyncFileRequest_Win32>(alloc, this));

		WorkItem *workItemRef = workItem;

		WorkItemIdentifier id;
		id.m_device = std::move(deviceCopy);
		id.m_path = std::move(pathCopy);

		{
			MutexLock lock(m_queueMutex);
			EXP_ASSERT(!m_isQuitting);

			WorkItem *queueTail = m_queueTail;
			if (queueTail == nullptr)
				m_queueHead = m_queueTail = workItemRef;
			else
			{
				queueTail->m_next = workItemRef;
				workItemRef->m_prev = queueTail;

				m_queueTail = workItemRef;
			}
		}

		asyncRequest->Init(std::move(workItem));

		return CorePtr<AsyncFileRequest>(std::move(asyncRequest));
	}


	void AsyncFileSystem_Win32::CancelItem(WorkItem *workItem)
	{
		MutexLock lock(m_queueMutex);

		m_inProgressItem->m_itemState = WorkItemState::kCancelled;

		if (m_inProgressItem == workItem)
			m_inProgressItem = nullptr;
		else
		{
			if (m_queueHead == workItem)
				m_queueHead = workItem->m_next;
			if (m_queueTail == workItem)
				m_queueTail = workItem->m_prev;

			if (workItem->m_prev)
				workItem->m_prev->m_next = workItem->m_next;
			if (workItem->m_next)
				workItem->m_next->m_prev = workItem->m_prev;
		}
	}

	int AsyncFileSystem_Win32::StaticThreadFunc(void *self)
	{
		return static_cast<AsyncFileSystem_Win32*>(self)->ThreadFunc();
	}

	int AsyncFileSystem_Win32::ThreadFunc()
	{
		for (;;)
		{
			WorkItemIdentifier identifier;

			{
				MutexLock lock(m_queueMutex);
				if (m_isQuitting)
					return 0;

				WorkItem *headWorkItem = m_queueHead;
				if (headWorkItem == nullptr)
				{
					lock.Release();
					m_ioWakeEvent->Wait();
					continue;
				}

				m_inProgressItem = headWorkItem;
				m_queueHead = headWorkItem->m_next;
				if (m_queueHead)
					m_queueHead->m_prev = nullptr;
				if (m_queueTail == headWorkItem)
					m_queueTail = nullptr;

				EXP_ASSERT(headWorkItem->m_itemState == WorkItemState::kQueued);

				headWorkItem->m_next = nullptr;
				headWorkItem->m_prev = nullptr;
				headWorkItem->m_itemState = WorkItemState::kInProgress;

				identifier = std::move(headWorkItem->m_id);
			}

			WorkItemState outState = WorkItemState::kQueued;
			ArrayPtr<uint8_t> contents;
			TryLoadWorkItem(identifier, outState, contents);

			{
				MutexLock lock(m_queueMutex);
				WorkItem *inProgressItem = m_inProgressItem;

				if (inProgressItem)
				{
					inProgressItem->m_result = std::move(contents);
					inProgressItem->m_itemState = outState;
					inProgressItem->m_finished.fetch_add(1, std::memory_order_release);
					m_inProgressItem = nullptr;
				}
			}
		}
	}

	void AsyncFileSystem_Win32::TryLoadWorkItem(const WorkItemIdentifier &identifier, WorkItemState &outState, ArrayPtr<uint8_t> &outContents)
	{
		ArrayPtr<uint8_t> contents;

		Result result(TryLoadWorkItemChecked(identifier, outContents));
		result.Handle();

		if (result.GetErrorCode() == ErrorCode::kOK)
		{
			outState = WorkItemState::kFinished;
			outContents = std::move(contents);
		}
		else
			outState = WorkItemState::kFailed;
	}

	Result AsyncFileSystem_Win32::TryLoadWorkItemChecked(const WorkItemIdentifier &identifier, ArrayPtr<uint8_t> &outContents)
	{
		CHECK_RV(CorePtr<FileStream>, stream, m_syncFileSystem->Open(identifier.m_device, identifier.m_path, SynchronousFileSystem::Permission::kRead, SynchronousFileSystem::CreationDisposition::kOpenExisting));

		CHECK_RV(UFilePos_t, fileSize, stream->GetSize());

		if (fileSize > std::numeric_limits<size_t>::max())
			return ErrorCode::kOutOfMemory;

		IAllocator *alloc = GetCoreObjectAllocator();

		CHECK_RV(ArrayPtr<uint8_t>, contents, NewArray<uint8_t>(alloc, static_cast<size_t>(fileSize)));
		CHECK_RV(size_t, amountRead, stream->ReadPartial(ArrayView<uint8_t>(contents)));

		outContents = std::move(contents);

		return ErrorCode::kOK;
	}

	AsyncFileSystem_Win32::WorkItem::WorkItem()
		: m_itemState(WorkItemState::kQueued)
		, m_prev(nullptr)
		, m_next(nullptr)
	{
	}
}
