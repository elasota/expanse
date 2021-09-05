#pragma once

#include "CoreObject.h"
#include "ArrayPtr.h"
#include "StringProto.h"

#include <stdint.h>

namespace expanse
{
	template<class TChar, TextEncoding TEncoding> struct StringView;
	template<class T> struct ResultRV;

	template<class TChar, TextEncoding TEncoding>
	struct String final
	{
	public:
		String();
		String(String<TChar, TEncoding> &&other);
		~String();

		size_t Length() const;

		operator StringView<TChar, TEncoding>() const;

		bool operator==(const String<TChar, TEncoding> &other) const;
		bool operator!=(const String<TChar, TEncoding> &other) const;
		bool operator==(const StringView<TChar, TEncoding> &other) const;
		bool operator!=(const StringView<TChar, TEncoding> &other) const;
		bool operator==(std::nullptr_t) const;
		bool operator!=(std::nullptr_t) const;

		String<TChar, TEncoding> &operator=(String<TChar, TEncoding> &&other);

		ResultRV<String<TChar, TEncoding>> Clone() const;
		static ResultRV<String<TChar, TEncoding>> CreateFromBytes(ArrayView<const TChar> bytes);
		static ResultRV<String<TChar, TEncoding>> CreateFromZeroTerminatedArray(ArrayPtr<TChar> &&bytes);

	private:
		String<TChar, TEncoding>(const String<TChar, TEncoding>& other) = delete;
		explicit String<TChar, TEncoding>(ArrayPtr<TChar> &&chars);

		ArrayPtr<TChar> m_chars;
	};
}

#include <utility>

#include "StringView.h"

namespace expanse
{
	template<class TChar, TextEncoding TEncoding>
	inline String<TChar, TEncoding>::String()
	{
	}

	template<class TChar, TextEncoding TEncoding>
	inline String<TChar, TEncoding>::String(String<TChar, TEncoding> &&other)
		: m_chars(std::move(other.m_chars))
	{
	}

	template<class TChar, TextEncoding TEncoding>
	inline String<TChar, TEncoding>::~String()
	{
	}

	template<class TChar, TextEncoding TEncoding>
	inline size_t String<TChar, TEncoding>::Length() const
	{
		if (m_chars == nullptr)
			return 0;

		return m_chars.Count() - 1;
	}

	template<class TChar, TextEncoding TEncoding>
	inline String<TChar, TEncoding>::operator StringView<TChar, TEncoding>() const
	{
		if (m_chars == nullptr)
			return StringView<TChar, TEncoding>();
		else
			return StringView<TChar, TEncoding>(m_chars.GetBuffer(), m_chars.Count() - 1);
	}

	template<class TChar, TextEncoding TEncoding>
	inline bool String<TChar, TEncoding>::operator==(const String<TChar, TEncoding> &other) const
	{
		return (*this) == static_cast<StringView<TChar, TEncoding>>(other);
	}

	template<class TChar, TextEncoding TEncoding>
	inline bool String<TChar, TEncoding>::operator!=(const String<TChar, TEncoding> &other) const
	{
		return (*this) != static_cast<StringView<TChar, TEncoding>>(other);
	}

	template<class TChar, TextEncoding TEncoding>
	inline bool String<TChar, TEncoding>::operator==(const StringView<TChar, TEncoding> &other) const
	{
		return static_cast<StringView<TChar, TEncoding>>(*this) == static_cast<StringView<TChar, TEncoding>>(other);
	}

	template<class TChar, TextEncoding TEncoding>
	inline bool String<TChar, TEncoding>::operator!=(const StringView<TChar, TEncoding> &other) const
	{
		return !((*this) == other);
	}

	template<class TChar, TextEncoding TEncoding>
	inline bool String<TChar, TEncoding>::operator==(std::nullptr_t) const
	{
		return m_chars == nullptr;
	}

	template<class TChar, TextEncoding TEncoding>
	inline bool String<TChar, TEncoding>::operator!=(std::nullptr_t) const
	{
		return m_chars != nullptr;
	}

	template<class TChar, TextEncoding TEncoding>
	inline String<TChar, TEncoding> &String<TChar, TEncoding>::operator=(String<TChar, TEncoding> &&other)
	{
		m_chars = std::move(other.m_chars);
		return *this;
	}

	template<class TChar, TextEncoding TEncoding>
	inline String<TChar, TEncoding>::String(ArrayPtr<TChar> &&chars)
		: m_chars(std::move(chars))
	{
	}


	template<class TChar, TextEncoding TEncoding>
	ResultRV<String<TChar, TEncoding>> String<TChar, TEncoding>::Clone() const
	{
		return static_cast<UTF8StringView_t>(*this).CloneToString();
	}

	template<class TChar, TextEncoding TEncoding>
	ResultRV<String<TChar, TEncoding>> String<TChar, TEncoding>::CreateFromBytes(ArrayView<const TChar> bytes)
	{
		const size_t numBytes = bytes.Size();

		CHECK_RV(ArrayPtr<uint8_t>, bytesCopy, NewArray<uint8_t>(numBytes + 1));
		bytesCopy[numBytes] = 0;

		if (numBytes > 0)
			memcpy(&bytesCopy[0], &bytes[0], numBytes);

		return CreateFromZeroTerminatedArray(std::move(bytesCopy));
	}

	template<class TChar, TextEncoding TEncoding>
	ResultRV<String<TChar, TEncoding>> String<TChar, TEncoding>::CreateFromZeroTerminatedArray(ArrayPtr<TChar> &&bytes)
	{
		return String<TChar, TEncoding>(std::move(bytes));
	}
}
