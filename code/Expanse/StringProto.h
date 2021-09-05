#pragma once

#include "TextEncoding.h"

#include <cstdint>

namespace expanse
{
	template<class TChar, TextEncoding TEncoding> struct String;
	template<class TChar, TextEncoding TEncoding> struct StringView;

	typedef String<char, TextEncoding::kASCII> ASCIIString_t;
	typedef String<uint8_t, TextEncoding::kUTF8> UTF8String_t;
	typedef String<uint16_t, TextEncoding::kUTF16> UTF16String_t;

	typedef StringView<char, TextEncoding::kASCII> ASCIIStringView_t;
	typedef StringView<uint8_t, TextEncoding::kUTF8> UTF8StringView_t;
	typedef StringView<uint16_t, TextEncoding::kUTF16> UTF16StringView_t;
}
