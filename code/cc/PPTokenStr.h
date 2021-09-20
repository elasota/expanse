#pragma once

#include "ArrayPtr.h"
#include "ArrayView.h"

namespace expanse
{
	namespace cc
	{
		struct TokenStrView;

		struct TokenStr
		{
			TokenStr();
			explicit TokenStr(ArrayPtr<uint8_t> &&token);

			ArrayView<const uint8_t> GetToken() const;
			TokenStrView GetTokenView() const;

			bool operator==(const TokenStr &other) const;
			bool operator!=(const TokenStr &other) const;

		private:
			ArrayPtr<uint8_t> m_token;
		};

		struct TokenStrView
		{
			TokenStrView();
			explicit TokenStrView(const ArrayView<const uint8_t> &token);

			ArrayView<const uint8_t> GetToken() const;

			template<size_t TSize>
			bool IsString(const char (&str)[TSize]) const;

			bool operator==(const TokenStrView &other) const;
			bool operator!=(const TokenStrView &other) const;

		private:
			ArrayView<const uint8_t> m_token;
		};
	}
}

#include "Comparer.h"
#include "Hasher.h"

#include <cstring>

template<size_t TSize>
bool expanse::cc::TokenStrView::IsString(const char(&str)[TSize]) const
{
	return (m_token.Size() == TSize - 1) && ((TSize == 1) || !memcmp(str, &m_token[0], TSize - 1));
}

template<>
class expanse::Hasher<expanse::cc::TokenStr>
{
public:
	static Hash_t Compute(const expanse::cc::TokenStr &key);
	static Hash_t Compute(const expanse::cc::TokenStrView &key);
};

template<>
class expanse::Hasher<expanse::cc::TokenStrView>
{
public:
	static Hash_t Compute(const expanse::cc::TokenStrView &key);
};

template<>
class expanse::Comparer<expanse::cc::TokenStr>
{
public:
	static bool StrictlyEqual(const expanse::cc::TokenStr &a, const expanse::cc::TokenStr &b);
	static bool StrictlyEqual(const expanse::cc::TokenStr &a, const expanse::cc::TokenStrView &b);
};

template<>
class expanse::Comparer<expanse::cc::TokenStrView>
{
public:
	static bool StrictlyEqual(const expanse::cc::TokenStrView &a, const expanse::cc::TokenStr &b);
	static bool StrictlyEqual(const expanse::cc::TokenStrView &a, const expanse::cc::TokenStrView &b);
};
