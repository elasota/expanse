#pragma once

#include "CoreObject.h"
#include "StringProto.h"

namespace expanse
{
	class AsyncFileRequest;
	class AsyncFileRequestHandle;
	template<class T> struct CorePtr;
	struct Result;
	template<class T> struct ResultRV;

	class AsyncFileSystem : public CoreObject
	{
	public:
		virtual ResultRV<CorePtr<AsyncFileRequest>> Retrieve(const UTF8StringView_t &device, const UTF8StringView_t &name) = 0;
	};
}
