#pragma once

#include "CoreObject.h"
#include "ErrorCode.h"
#include "StringProto.h"

#include <cstdint>

namespace expanse
{
	template<class T> struct ArrayPtr;

	class AsyncFileRequest : public CoreObject
	{
	public:
		virtual bool IsFinished() const = 0;
		virtual ErrorCode GetErrorCode() const = 0;
		virtual ArrayPtr<uint8_t> TakeResult() = 0;
		virtual void TakeIdentifier(UTF8String_t &outDevice, UTF8String_t &outPath) = 0;
	};
}
