#pragma once

#include "StringProto.h"

#include <cstdint>

namespace expanse
{
	template<class T> struct ArrayView;
	template<class T> struct ResultRV;
	struct IAllocator;

	template<class TChar, TextEncoding TEncoding>
	struct StringView
	{
	public:
		StringView();
		explicit StringView(const char *str);
		StringView(const uint8_t *chars, size_t length);

		ArrayView<const uint8_t> GetChars() const;
		size_t Length() const;

		bool operator==(const StringView<TChar, TEncoding> &other) const;
		bool operator!=(const StringView<TChar, TEncoding> &other) const;

		ResultRV<String<TChar, TEncoding>> CloneToString(IAllocator *alloc) const;

	private:
		const uint8_t *m_chars;
		size_t m_length;
	};
}

#include "ArrayView.h"
#include <cstring>

namespace expanse
{
	template<class TChar, TextEncoding TEncoding>
	inline StringView<TChar, TEncoding>::StringView()
		: m_chars(reinterpret_cast<const uint8_t*>(""))
		, m_length(0)
	{
	}

	template<class TChar, TextEncoding TEncoding>
	inline StringView<TChar, TEncoding>::StringView(const char *str)
		: m_chars(reinterpret_cast<const TChar*>(str))
		, m_length(strlen(str))
	{
		static_assert(sizeof(TChar) == sizeof(char), "Can't static initialize string view from char strings of different size");
	}

	template<class TChar, TextEncoding TEncoding>
	inline StringView<TChar, TEncoding>::StringView(const uint8_t *chars, size_t length)
		: m_chars(chars)
		, m_length(length)
	{
	}

	template<class TChar, TextEncoding TEncoding>
	inline ArrayView<const uint8_t> StringView<TChar, TEncoding>::GetChars() const
	{
		return ArrayView<const uint8_t>(m_chars, m_length);
	}

	template<class TChar, TextEncoding TEncoding>
	inline size_t StringView<TChar, TEncoding>::Length() const
	{
		return m_length;
	}

	template<class TChar, TextEncoding TEncoding>
	inline bool StringView<TChar, TEncoding>::operator!=(const StringView<TChar, TEncoding> &other) const
	{
		return !((*this) == other);
	}


	template<class TChar, TextEncoding TEncoding>
	inline bool StringView<TChar, TEncoding>::operator==(const StringView<TChar, TEncoding> &other) const
	{
		if (m_length != other.m_length)
			return false;

		if (m_chars == other.m_chars)
			return true;

		return memcmp(m_chars, other.m_chars, m_length * sizeof(TChar)) == 0;
	}

	template<class TChar, TextEncoding TEncoding>
	ResultRV<String<TChar, TEncoding>> StringView<TChar, TEncoding>::CloneToString(IAllocator *alloc) const
	{
		return String<TChar, TEncoding>::CreateFromBytes(alloc, ArrayView<const TChar>(m_chars, m_length));
	}
}
