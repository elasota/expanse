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
		static ResultRV<XString<TChar, TEncoding>> Concat(IAllocator *alloc, StringView<TChar, TEncoding> a, StringView<TChar, TEncoding> b);

		template<class TChar, TextEncoding TEncoding>
		static Result Append(IAllocator *alloc, XString<TChar, TEncoding> &a, StringView<TChar, TEncoding> b);

		template<class T>
		static size_t IntToStr(ArrayView<uint8_t> *outChars, T integer);
	};
}

#include "ResultRV.h"
#include "XString.h"

#include <algorithm>

namespace expanse
{
	template<class TChar, TextEncoding TEncoding>
	ResultRV<XString<TChar, TEncoding>> StrUtils::Concat(IAllocator *alloc, StringView<TChar, TEncoding> a, StringView<TChar, TEncoding> b)
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

		return XString<TChar, TEncoding>::CreateFromZeroTerminatedArray(std::move(concatenated));
	}

	template<class TChar, TextEncoding TEncoding>
	Result StrUtils::Append(IAllocator *alloc, XString<TChar, TEncoding> &a, StringView<TChar, TEncoding> b)
	{
		typedef XString<TChar, TEncoding> StringType_t;
		CHECK_RV(StringType_t, result, Concat(alloc, StringView<TChar, TEncoding>(a), b));
		a = std::move(result);

		return ErrorCode::kOK;
	}

	template<class T>
	size_t StrUtils::IntToStr(ArrayView<uint8_t> *outChars, T integer)
	{
		if (integer < 0)
		{
			if (outChars)
				(*outChars)[0] = '-';

			size_t emitted = 1;
			while (integer != 0)
			{
				const T prevInteger = integer;
				integer /= static_cast<T>(10);
				const T remainder = prevInteger - (integer * static_cast<T>(10));

				if (outChars)
					(*outChars)[emitted] = static_cast<uint8_t>('0' - remainder);

				emitted++;
			}

			if (outChars)
			{
				const size_t numDigits = emitted - 1;

				for (size_t i = 0; i < numDigits / 2; i++)
					std::swap((*outChars)[i + 1], (*outChars)[emitted - i]);
			}

			return emitted;
		}
		else if (integer == 0)
		{
			if (outChars)
				(*outChars)[0] = '0';
			return 1;
		}
		else // if (integer > 0)
		{
			size_t emitted = 0;
			while (integer != 0)
			{
				const T remainder = integer % static_cast<T>(10);
				integer /= static_cast<T>(10);

				if (outChars)
					(*outChars)[emitted] = static_cast<uint8_t>('0' + remainder);
				emitted++;
			}

			if (outChars)
			{
				const size_t numDigits = emitted;

				for (size_t i = 0; i < numDigits / 2; i++)
					std::swap((*outChars)[i], (*outChars)[emitted - i]);
			}

			return emitted;
		}
	}
}
