#pragma once

#include "CoreObject.h"
#include "ArrayPtr.h"
#include "StringProto.h"

#include <stdint.h>

namespace expanse
{
	template<class TChar, TextEncoding TEncoding> struct StringView;
	template<class T> struct ResultRV;
	struct IAllocator;

	template<class TChar, TextEncoding TEncoding>
	struct XString final
	{
	public:
		XString();
		XString(XString<TChar, TEncoding> &&other);
		~XString();

		size_t Length() const;

		operator StringView<TChar, TEncoding>() const;

		ArrayView<const TChar> GetChars() const;

		bool operator==(const XString<TChar, TEncoding> &other) const;
		bool operator!=(const XString<TChar, TEncoding> &other) const;
		bool operator==(const StringView<TChar, TEncoding> &other) const;
		bool operator!=(const StringView<TChar, TEncoding> &other) const;
		bool operator==(std::nullptr_t) const;
		bool operator!=(std::nullptr_t) const;

		XString<TChar, TEncoding> &operator=(XString<TChar, TEncoding> &&other);

		ResultRV<XString<TChar, TEncoding>> Clone(IAllocator *alloc) const;
		static ResultRV<XString<TChar, TEncoding>> CreateFromBytes(IAllocator *alloc, ArrayView<const TChar> bytes);
		static ResultRV<XString<TChar, TEncoding>> CreateFromZeroTerminatedArray(ArrayPtr<TChar> &&bytes);

	private:
		XString<TChar, TEncoding>(const XString<TChar, TEncoding>& other) = delete;
		explicit XString<TChar, TEncoding>(ArrayPtr<TChar> &&chars);

		ArrayPtr<TChar> m_chars;
	};
}

#include "ArrayPtr.h"
#include "ResultRV.h"
#include "StringView.h"

#include <utility>

namespace expanse
{
	template<class TChar, TextEncoding TEncoding>
	inline XString<TChar, TEncoding>::XString()
	{
	}

	template<class TChar, TextEncoding TEncoding>
	inline XString<TChar, TEncoding>::XString(XString<TChar, TEncoding> &&other)
		: m_chars(std::move(other.m_chars))
	{
	}

	template<class TChar, TextEncoding TEncoding>
	inline XString<TChar, TEncoding>::~XString()
	{
	}

	template<class TChar, TextEncoding TEncoding>
	inline size_t XString<TChar, TEncoding>::Length() const
	{
		if (m_chars == nullptr)
			return 0;

		return m_chars.Count() - 1;
	}

	template<class TChar, TextEncoding TEncoding>
	inline XString<TChar, TEncoding>::operator StringView<TChar, TEncoding>() const
	{
		if (m_chars == nullptr)
			return StringView<TChar, TEncoding>();
		else
			return StringView<TChar, TEncoding>(m_chars.GetBuffer(), m_chars.Count() - 1);
	}

	template<class TChar, TextEncoding TEncoding>
	ArrayView<const TChar> XString<TChar, TEncoding>::GetChars() const
	{
		if (m_chars.Count() == 0)
			return ArrayView<const TChar>(nullptr, 0);
		return ArrayView<const TChar>(m_chars.GetBuffer(), m_chars.Count() - 1);
	}

	template<class TChar, TextEncoding TEncoding>
	inline bool XString<TChar, TEncoding>::operator==(const XString<TChar, TEncoding> &other) const
	{
		return (*this) == static_cast<StringView<TChar, TEncoding>>(other);
	}

	template<class TChar, TextEncoding TEncoding>
	inline bool XString<TChar, TEncoding>::operator!=(const XString<TChar, TEncoding> &other) const
	{
		return (*this) != static_cast<StringView<TChar, TEncoding>>(other);
	}

	template<class TChar, TextEncoding TEncoding>
	inline bool XString<TChar, TEncoding>::operator==(const StringView<TChar, TEncoding> &other) const
	{
		return static_cast<StringView<TChar, TEncoding>>(*this) == static_cast<StringView<TChar, TEncoding>>(other);
	}

	template<class TChar, TextEncoding TEncoding>
	inline bool XString<TChar, TEncoding>::operator!=(const StringView<TChar, TEncoding> &other) const
	{
		return !((*this) == other);
	}

	template<class TChar, TextEncoding TEncoding>
	inline bool XString<TChar, TEncoding>::operator==(std::nullptr_t) const
	{
		return m_chars == nullptr;
	}

	template<class TChar, TextEncoding TEncoding>
	inline bool XString<TChar, TEncoding>::operator!=(std::nullptr_t) const
	{
		return m_chars != nullptr;
	}

	template<class TChar, TextEncoding TEncoding>
	inline XString<TChar, TEncoding> &XString<TChar, TEncoding>::operator=(XString<TChar, TEncoding> &&other)
	{
		m_chars = std::move(other.m_chars);
		return *this;
	}

	template<class TChar, TextEncoding TEncoding>
	inline XString<TChar, TEncoding>::XString(ArrayPtr<TChar> &&chars)
		: m_chars(std::move(chars))
	{
	}


	template<class TChar, TextEncoding TEncoding>
	ResultRV<XString<TChar, TEncoding>> XString<TChar, TEncoding>::Clone(IAllocator *alloc) const
	{
		return static_cast<UTF8StringView_t>(*this).CloneToString(alloc);
	}

	template<class TChar, TextEncoding TEncoding>
	ResultRV<XString<TChar, TEncoding>> XString<TChar, TEncoding>::CreateFromBytes(IAllocator *alloc, ArrayView<const TChar> bytes)
	{
		const size_t numBytes = bytes.Size();

#ifndef CHECK_RV
#error "wtf"
#endif
		CHECK_RV(ArrayPtr<TChar>, bytesCopy, NewArray<uint8_t>(alloc, numBytes + 1));
		bytesCopy[numBytes] = 0;

		if (numBytes > 0)
			memcpy(&bytesCopy[0], &bytes[0], numBytes);

		return CreateFromZeroTerminatedArray(std::move(bytesCopy));
	}

	template<class TChar, TextEncoding TEncoding>
	ResultRV<XString<TChar, TEncoding>> XString<TChar, TEncoding>::CreateFromZeroTerminatedArray(ArrayPtr<TChar> &&bytes)
	{
		return XString<TChar, TEncoding>(std::move(bytes));
	}
}
