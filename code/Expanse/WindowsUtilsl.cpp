#include "WindowsUtils.h"

#include "Unicode.h"
#include "ArrayPtr.h"
#include "ResultRV.h"
#include "StaticArray.h"
#include "StringProto.h"
#include "StringView.h"
#include "Mem.h"

namespace expanse
{
	ResultRV<ArrayPtr<wchar_t>> WindowsUtils::ConvertToWideChar(IAllocator *alloc, const UTF8StringView_t &utf8String)
	{
		StaticArray<uint16_t, UTF16::kMaxEncodedCharacters> utf16Encoded;

		// Count characters
		size_t numUTF16Characters = 0;

		{
			uint32_t codePoint = 0;

			ArrayView<const uint8_t> stringBytes = utf8String.GetChars();

			while (stringBytes.Size() > 0)
			{
				size_t numDigested = 0;

				if (!UTF8::Decode(stringBytes, numDigested, codePoint))
					return ErrorCode::kMalformedUTF8String;

				stringBytes = stringBytes.Subrange(numDigested);

				size_t numEmitted = 0;
				UTF16::Encode(utf16Encoded, numEmitted, codePoint);

				numUTF16Characters += numEmitted;
			}
		}

		CHECK_RV(ArrayPtr<wchar_t>, wcharChars, NewArray<wchar_t>(alloc, numUTF16Characters + 1));

		wcharChars[numUTF16Characters] = static_cast<wchar_t>(0);

		{
			uint32_t codePoint = 0;
			size_t writeOffset = 0;

			ArrayView<const uint8_t> stringBytes = utf8String.GetChars();

			while (stringBytes.Size() > 0)
			{
				size_t numDigested = 0;

				if (!UTF8::Decode(stringBytes, numDigested, codePoint))
					return ErrorCode::kMalformedUTF8String;

				stringBytes = stringBytes.Subrange(numDigested);

				size_t numEmitted = 0;
				UTF16::Encode(utf16Encoded, numEmitted, codePoint);

				for (size_t i = 0; i < numEmitted; i++)
					wcharChars[writeOffset + i] = static_cast<wchar_t>(utf16Encoded[i]);

				writeOffset += numEmitted;
			}
		}

		return wcharChars;
	}


	LARGE_INTEGER WindowsUtils::Int64ToLargeInteger(int64_t value)
	{
		LARGE_INTEGER result;
		result.QuadPart = value;
		return result;
	}

	int64_t WindowsUtils::LargeIntegerToInt64(const LARGE_INTEGER &value)
	{
		return value.QuadPart;
	}
}
