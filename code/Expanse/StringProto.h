#pragma once

#include "TextEncoding.h"

#include <cstdint>

namespace expanse
{
	template<class TChar, TextEncoding TEncoding> struct XString;
	template<class TChar, TextEncoding TEncoding> struct StringView;

	typedef XString<char, TextEncoding::kASCII> ASCIIString_t;
	typedef XString<uint8_t, TextEncoding::kUTF8> UTF8String_t;
	typedef XString<uint16_t, TextEncoding::kUTF16> UTF16String_t;

	typedef StringView<char, TextEncoding::kASCII> ASCIIStringView_t;
	typedef StringView<uint8_t, TextEncoding::kUTF8> UTF8StringView_t;
	typedef StringView<uint16_t, TextEncoding::kUTF16> UTF16StringView_t;
}
