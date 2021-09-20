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
		kOperationFailed = -7,
		kStackOverflow = -8,
		kFileNotFound = -9,
		kMalformedNumber = -10,
		kArithmeticOverflow = -11,

		kInternalError = -1000,
		kNotImplemented = -1001,
	};
}
