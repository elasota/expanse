#pragma once

#include "CorePtr.h"

namespace expanse
{
	class SynchronousFileSystem;
	class AsyncFileSystem;

	struct ServiceCollection
	{
		ServiceCollection();

		SynchronousFileSystem *m_syncFileSystem;
		AsyncFileSystem *m_asyncFileSystem;

		static ServiceCollection *ms_primaryInstance;
	};
}
