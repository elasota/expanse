#pragma once

#include "StringProto.h"

namespace expanse
{
	namespace cc
	{
		struct FileCoordinate;
		struct TokenStrView;

		struct IIncludeStackTrace
		{
			virtual void Reset() = 0;
			virtual bool Pop() = 0;
			virtual void GetCurrentFile(UTF8StringView_t &outDevice, UTF8StringView_t &outPath, FileCoordinate &outCoordinate) const = 0;
			virtual TokenStrView GetCurrentTraceFile() const = 0;
		};
	}
}
