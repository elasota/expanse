#pragma once

namespace expanse
{
	enum class ErrorCode
	{
		kOK = 0,

		kMalformedUTF8String = -1,
		kOutOfMemory = -2,

		kInvalidArgument = -3,
		kIOError = -4,
		kInvalidPath = -5,
		kSystemError = -6,
	};
}
