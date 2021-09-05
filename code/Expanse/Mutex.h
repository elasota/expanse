#pragma once

#include "CoreObject.h"

#include <cstdint>

namespace expanse
{
	template<class T> struct CorePtr;
	template<class T> struct ResultRV;

	class Mutex : public CoreObject
	{
	public:
		virtual void Lock() = 0;
		virtual bool TryLock() = 0;
		virtual void Unlock() = 0;

		static ResultRV<CorePtr<Mutex>> Create();
	};
}
