#pragma once

#include "StringProto.h"

namespace expanse
{
	template<class T> struct ResultRV;
	struct Result;

	class StrUtils
	{
	public:
		template<class TChar, TextEncoding TEncoding>
		static ResultRV<UTF8String_t> Concat(StringView<TChar, TEncoding> a, StringView<TChar, TEncoding> b);

		template<class TChar, TextEncoding TEncoding>
		static Result Append(String<TChar, TEncoding> &a, StringView<TChar, TEncoding> b);
	};
}

#include "ResultRV.h"
#include "String.h"

namespace expanse
{
	template<class TChar, TextEncoding TEncoding>
	ResultRV<UTF8String_t> StrUtils::Concat(StringView<TChar, TEncoding> a, StringView<TChar, TEncoding> b)
	{
		const size_t lengthA = a.Length();
		const size_t lengthB = b.Length();

		size_t limit = std::numeric_limits<size_t>::max() - lengthA;
		if (lengthB > limit)
			return ErrorCode::kOutOfMemory;

		limit -= lengthB;
		if (limit < 1)
			return ErrorCode::kOutOfMemory;

		const size_t needed = lengthA + lengthB + 1;

		CHECK_RV(ArrayPtr<uint8_t>, concatenated, NewArrayUninitialized<uint8_t>(needed));

		if (lengthA > 0)
			memcpy(&concatenated[0], &a.GetChars()[0], lengthA);
		if (lengthB > 0)
			memcpy(&concatenated[lengthA], &b.GetChars()[0], lengthB);
		concatenated[lengthA + lengthB] = 0;

		return UTF8String_t::CreateFromZeroTerminatedArray(std::move(concatenated));
	}

	template<class TChar, TextEncoding TEncoding>
	Result StrUtils::Append(String<TChar, TEncoding> &a, StringView<TChar, TEncoding> b)
	{
		typedef String<TChar, TEncoding> StringType_t;
		CHECK_RV(StringType_t, result, Concat(StringView<TChar, TEncoding>(a), b));
		a = std::move(result);

		return ErrorCode::kOK;
	}
}
