#include "IncludeWindows.h"
#include "AsyncFileSystem_Win32.h"
#include "FileStream.h"
#include "SynchronousFileSystem_Win32.h"
#include "Services.h"
#include "ServiceCollection.h"
#include "Mem.h"
#include "Result.h"
#include "WindowsGlobals.h"
#include "WindowsUtils.h"

#include <shellapi.h>
#include <utility>

expanse::Result TestCC(expanse::IAllocator *alloc, expanse::AsyncFileSystem *asyncFS, expanse::FileStream *outFile, expanse::FileStream *traceOutFile);

class Allocator_Win32 final : public expanse::IAllocator
{
public:
	void *Alloc(size_t size, size_t alignment) override;
	void Release(void *ptr) override;
	void *Realloc(void *ptr, size_t newSize, size_t alignment) override;

	static const uint32_t kSentinelStart = 0xaabe4410;
};

struct MemBlockInfo
{
	void *m_baseAddress;
	size_t m_size;
	size_t m_alignment;
	uint32_t m_sentinel;
};

void *Allocator_Win32::Alloc(size_t size, size_t alignment)
{
	if (size == 0)
		return nullptr;

	size_t extraRequired = sizeof(MemBlockInfo) + alignment - 1;

	const size_t maxSize = std::numeric_limits<size_t>::max() - extraRequired;
	if (maxSize < size)
		return nullptr;

	uint8_t *mem = static_cast<uint8_t*>(malloc(size + extraRequired));
	uint8_t *memBlockEndAddressBase = mem + sizeof(MemBlockInfo);

	size_t padding = alignment - static_cast<size_t>(reinterpret_cast<uintptr_t>(memBlockEndAddressBase) % static_cast<uintptr_t>(alignment));

	if (padding == alignment)
		padding = 0;

	uint8_t *memBlockEndAddress = memBlockEndAddressBase + padding;

	MemBlockInfo memBlockInfo;
	memBlockInfo.m_size = size;
	memBlockInfo.m_baseAddress = mem;
	memBlockInfo.m_alignment = alignment;
	memBlockInfo.m_sentinel = kSentinelStart;

	memcpy(memBlockEndAddress - sizeof(MemBlockInfo), &memBlockInfo, sizeof(MemBlockInfo));

	return memBlockEndAddressBase;
}

void Allocator_Win32::Release(void *ptr)
{
	if (ptr == nullptr)
		return;

	MemBlockInfo memBlockInfo;
	memcpy(&memBlockInfo, static_cast<uint8_t*>(ptr) - sizeof(MemBlockInfo), sizeof(MemBlockInfo));

	EXP_ASSERT(memBlockInfo.m_sentinel == kSentinelStart);

	free(memBlockInfo.m_baseAddress);
}

void *Allocator_Win32::Realloc(void *ptr, size_t newSize, size_t alignment)
{
	if (ptr == nullptr)
		return this->Alloc(newSize, alignment);

	MemBlockInfo memBlockInfo;
	memcpy(&memBlockInfo, static_cast<uint8_t*>(ptr) - sizeof(MemBlockInfo), sizeof(MemBlockInfo));

	if (memBlockInfo.m_alignment != alignment)
		return nullptr;

	void *newMem = this->Alloc(newSize, alignment);
	if (!newMem)
		return nullptr;

	size_t copySize = memBlockInfo.m_size;
	if (newSize < copySize)
		newSize = copySize;

	memcpy(newMem, ptr, copySize);

	free(memBlockInfo.m_baseAddress);

	return newMem;
}

static expanse::Result CheckedWinMain()
{
	Allocator_Win32 alloc;

	expanse::WindowsGlobals &winGlobals = expanse::WindowsGlobals::ms_instance;

	const LPWSTR *argv = winGlobals.m_argv;
	const int argc = winGlobals.m_argc;

	expanse::ServiceCollection serviceCollection;

	expanse::ServiceCollection::ms_primaryInstance = &serviceCollection;

	CHECK_RV(expanse::CorePtr<expanse::SynchronousFileSystem_Win32>, syncFileSystem, expanse::New<expanse::SynchronousFileSystem_Win32>(&alloc));

	serviceCollection.m_syncFileSystem = syncFileSystem;

	CHECK_RV(expanse::CorePtr<expanse::AsyncFileSystem_Win32>, asyncFileSystem, expanse::New<expanse::AsyncFileSystem_Win32>(&alloc, syncFileSystem));
	CHECK(asyncFileSystem->Initialize());

	serviceCollection.m_asyncFileSystem = asyncFileSystem;

	for (int i = 0; i < argc; i++)
	{
		if (!wcscmp(argv[i], L"-data"))
		{
			i++;
			if (i == argc)
				return expanse::ErrorCode::kInvalidArgument;

			CHECK_RV(expanse::UTF8String_t, dataDir, expanse::WindowsUtils::ConvertToUTF8(&alloc, argv[i]));

			CHECK(syncFileSystem->SetGamePath(std::move(dataDir)));
		}
	}

	CHECK_RV(expanse::CorePtr<expanse::FileStream>, outFile, syncFileSystem->Open(expanse::UTF8StringView_t("game"), expanse::UTF8StringView_t("logic/test.i"), expanse::SynchronousFileSystem::Permission::kWrite, expanse::SynchronousFileSystem::CreationDisposition::kCreateAlways));
	CHECK_RV(expanse::CorePtr<expanse::FileStream>, traceOutFile, syncFileSystem->Open(expanse::UTF8StringView_t("game"), expanse::UTF8StringView_t("logic/test.tr"), expanse::SynchronousFileSystem::Permission::kWrite, expanse::SynchronousFileSystem::CreationDisposition::kCreateAlways));

	///////////////////////////////////////////////////////////////////////////////
	// Main function
	CHECK(TestCC(&alloc, serviceCollection.m_asyncFileSystem, outFile, traceOutFile));

	return expanse::ErrorCode::kOK;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	int argc = 0;
	LPWSTR *argv = CommandLineToArgvW(pCmdLine, &argc);
	if (argv == nullptr)
		return static_cast<int>(expanse::ErrorCode::kOutOfMemory);

	expanse::WindowsGlobals &winGlobals = expanse::WindowsGlobals::ms_instance;

	winGlobals.m_hInstance = hInstance;
	winGlobals.m_hPrevInstance = hPrevInstance;
	winGlobals.m_cmdShow = nCmdShow;
	winGlobals.m_argc = argc;
	winGlobals.m_argv = argv;

	expanse::ErrorCode errorCode = expanse::ErrorCode::kOK;
	{
		expanse::Result result(CheckedWinMain());
		errorCode = result.GetErrorCode();
		result.Handle();
	}

	LocalFree(argv);

	return static_cast<int>(errorCode);
}
