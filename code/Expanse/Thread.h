#pragma once

#include "CoreObject.h"
#include "StringProto.h"

namespace expanse
{
	typedef int(*ThreadFunc_t)(void *userdata);
	typedef int ThreadID_t;
	struct IAllocator;

	template<class T> struct CorePtr;
	template<class T> struct ResultRV;

	class Thread : public CoreObject
	{
	public:
		virtual bool IsRunning() const = 0;
		virtual void WaitForExit() const = 0;
		virtual int GetExitCode() const = 0;
		virtual ThreadID_t GetID() const = 0;

		static ResultRV<CorePtr<Thread>> CreateThread(IAllocator *alloc, ThreadFunc_t threadFunc, void *userData, const UTF8StringView_t &name);

	protected:
		Thread();
	};
}

namespace expanse
{
	inline Thread::Thread()
	{
	}
}
