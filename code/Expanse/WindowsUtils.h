#pragma once

#include "IncludeWindows.h"
#include "StringProto.h"

#include <stdint.h>

namespace expanse
{
	template<class T> struct ArrayPtr;
	template<class T> struct ResultRV;
	struct IAllocator;

	class WindowsUtils
	{
	public:
		static ResultRV<ArrayPtr<wchar_t>> ConvertToWideChar(IAllocator *alloc, const UTF8StringView_t &utf8String);
		static LARGE_INTEGER Int64ToLargeInteger(int64_t value);
		static int64_t LargeIntegerToInt64(const LARGE_INTEGER &value);
	};
}
