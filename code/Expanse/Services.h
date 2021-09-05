#pragma once

namespace expanse
{
	class SynchronousFileSystem;
	class AsyncFileSystem;
	template<class T> struct CorePtr;

	struct Services
	{
		static SynchronousFileSystem *GetSynchronousFileSystem();
		static AsyncFileSystem *GetAsyncFileSystem();
	};
}
