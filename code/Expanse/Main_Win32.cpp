#include "IncludeWindows.h"
#include "AsyncFileSystem_Win32.h"
#include "SynchronousFileSystem_Win32.h"
#include "Services.h"
#include "ServiceCollection.h"
#include "Mem.h"
#include "Result.h"
#include "WindowsGlobals.h"

#include <shellapi.h>
#include <utility>

expanse::Result TestCC();

static expanse::Result CheckedWinMain()
{
	expanse::WindowsGlobals &winGlobals = expanse::WindowsGlobals::ms_instance;

	const LPWSTR *argv = winGlobals.m_argv;
	const int argc = winGlobals.m_argc;

	expanse::ServiceCollection serviceCollection;

	expanse::ServiceCollection::ms_primaryInstance = &serviceCollection;

	CHECK_RV(expanse::CorePtr<expanse::SynchronousFileSystem_Win32>, syncFileSystem, expanse::New<expanse::SynchronousFileSystem_Win32>());

	serviceCollection.m_syncFileSystem = syncFileSystem;

	CHECK_RV(expanse::CorePtr<expanse::AsyncFileSystem_Win32>, asyncFileSystem, expanse::New<expanse::AsyncFileSystem_Win32>(syncFileSystem));

	serviceCollection.m_asyncFileSystem = asyncFileSystem;

	CHECK(TestCC());

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
