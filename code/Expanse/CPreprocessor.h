#pragma once

#include "CoreObject.h"

namespace expanse
{
	template<class T> struct CorePtr;
	class AsyncFileSystem;

	class CPreprocessor : public CoreObject
	{
	public:
		static CorePtr<CPreprocessor> Create(AsyncFileSystem *fs);

	protected:
		CPreprocessor();
	};
}
