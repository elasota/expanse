#include "PPTokenStr.h"

expanse::cc::TokenStr::TokenStr()
{
}

expanse::cc::TokenStr::TokenStr(ArrayPtr<uint8_t> &&token)
	: m_token(std::move(token))
{
}

expanse::ArrayView<const uint8_t> expanse::cc::TokenStr::GetToken() const
{
	return m_token.ConstView();
}

expanse::cc::TokenStrView expanse::cc::TokenStr::GetTokenView() const
{
	return TokenStrView(m_token.ConstView());
}

bool expanse::cc::TokenStr::operator==(const TokenStr &other) const
{
	if (this == &other)
		return true;

	return this->GetTokenView() == other.GetTokenView();
}

bool expanse::cc::TokenStr::operator!=(const TokenStr &other) const
{
	return !((*this) == other);
}

expanse::cc::TokenStrView::TokenStrView()
{
}

expanse::cc::TokenStrView::TokenStrView(const ArrayView<const uint8_t> &token)
	: m_token(token)
{
}

expanse::ArrayView<const uint8_t> expanse::cc::TokenStrView::GetToken() const
{
	return m_token;
}

bool expanse::cc::TokenStrView::operator==(const TokenStrView &other) const
{
	if (this == &other)
		return true;

	const ArrayView<const uint8_t> thisChars = m_token;
	const ArrayView<const uint8_t> otherChars = other.m_token;

	if (thisChars.Size() != otherChars.Size())
		return false;

	const size_t numChars = thisChars.Size();
	for (size_t i = 0; i < numChars; i++)
	{
		if (thisChars[i] != otherChars[i])
			return false;
	}

	return true;
}

bool expanse::cc::TokenStrView::operator!=(const TokenStrView &other) const
{
	return !((*this) == other);
}

expanse::Hash_t expanse::Hasher<expanse::cc::TokenStr>::Compute(const expanse::cc::TokenStr &key)
{
	return expanse::Hasher<expanse::cc::TokenStrView>::Compute(key.GetTokenView());
}

expanse::Hash_t expanse::Hasher<expanse::cc::TokenStr>::Compute(const expanse::cc::TokenStrView &key)
{
	return expanse::Hasher<expanse::cc::TokenStrView>::Compute(key);
}

expanse::Hash_t expanse::Hasher<expanse::cc::TokenStrView>::Compute(const expanse::cc::TokenStrView &key)
{
	const ArrayView<const uint8_t> token = key.GetToken();
	if (token.Size() == 0)
		return 0;

	return HashUtil::ComputePODHash(&token[0], token.Size());
}

bool expanse::Comparer<expanse::cc::TokenStr>::StrictlyEqual(const expanse::cc::TokenStr &a, const expanse::cc::TokenStr &b)
{
	return a.GetTokenView() == b.GetTokenView();
}

bool expanse::Comparer<expanse::cc::TokenStr>::StrictlyEqual(const expanse::cc::TokenStr &a, const expanse::cc::TokenStrView &b)
{
	return a.GetTokenView() == b;
}

bool expanse::Comparer<expanse::cc::TokenStrView>::StrictlyEqual(const expanse::cc::TokenStrView &a, const expanse::cc::TokenStr &b)
{
	return a == b.GetTokenView();
}

bool expanse::Comparer<expanse::cc::TokenStrView>::StrictlyEqual(const expanse::cc::TokenStrView &a, const expanse::cc::TokenStrView &b)
{
	return a == b;
}
