#pragma once

#include <cstdint>

namespace expanse
{
	template<class T> struct ArrayView;

	class UTF8
	{
	public:
		static const unsigned int kMaxEncodedBytes = 4;

		static bool Decode(const ArrayView<const uint8_t> &characters, size_t &outCharactersDigested, uint32_t &outCodePoint);
		static void Encode(const ArrayView<uint8_t> &characters, size_t &outCharactersEmitted, uint32_t codePoint);
	};

	class UTF16
	{
	public:
		static const unsigned int kMaxEncodedCharacters = 2;

		static bool Decode(const ArrayView<const uint16_t> &characters, size_t &outCharactersDigested, uint32_t &outCodePoint);
		static void Encode(const ArrayView<uint16_t> &characters, size_t &outCharactersEmitted, uint32_t codePoint);
	};
}
