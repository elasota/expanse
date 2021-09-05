#pragma once

#include "StringProto.h"

namespace expanse
{
	template<class T> struct ResultRV;
	struct Result;
	struct IAllocator;

	class StrUtils
	{
	public:
		template<class TChar, TextEncoding TEncoding>
		static ResultRV<String<TChar, TEncoding>> Concat(IAllocator *alloc, StringView<TChar, TEncoding> a, StringView<TChar, TEncoding> b);

		template<class TChar, TextEncoding TEncoding>
		static Result Append(IAllocator *alloc, String<TChar, TEncoding> &a, StringView<TChar, TEncoding> b);
	};
}

#include "ResultRV.h"
#include "String.h"

namespace expanse
{
	template<class TChar, TextEncoding TEncoding>
	ResultRV<String<TChar, TEncoding>> StrUtils::Concat(IAllocator *alloc, StringView<TChar, TEncoding> a, StringView<TChar, TEncoding> b)
	{
		const size_t lengthA = a.Length();
		const size_t lengthB = b.Length();

		size_t limit = (std::numeric_limits<size_t>::max() / sizeof(TChar)) - lengthA;
		if (lengthB > limit)
			return ErrorCode::kOutOfMemory;

		limit -= lengthB;
		if (limit < 1)
			return ErrorCode::kOutOfMemory;

		const size_t needed = lengthA + lengthB + 1;

		CHECK_RV(ArrayPtr<uint8_t>, concatenated, NewArrayUninitialized<uint8_t>(alloc, needed));

		if (lengthA > 0)
			memcpy(&concatenated[0], &a.GetChars()[0], lengthA * sizeof(TChar));
		if (lengthB > 0)
			memcpy(&concatenated[lengthA], &b.GetChars()[0], lengthB * sizeof(TChar));
		concatenated[lengthA + lengthB] = 0;

		return String<TChar, TEncoding>::CreateFromZeroTerminatedArray(std::move(concatenated));
	}

	template<class TChar, TextEncoding TEncoding>
	Result StrUtils::Append(IAllocator *alloc, String<TChar, TEncoding> &a, StringView<TChar, TEncoding> b)
	{
		typedef String<TChar, TEncoding> StringType_t;
		CHECK_RV(StringType_t, result, Concat(alloc, StringView<TChar, TEncoding>(a), b));
		a = std::move(result);

		return ErrorCode::kOK;
	}
}
