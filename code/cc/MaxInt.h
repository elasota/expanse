#pragma once

#include <cstdint>
#include <cstddef>

#include "LType.h"

namespace expanse
{
	template<class T> struct ResultRV;

	namespace cc
	{
		struct MaxUInt;

		struct MaxSInt final
		{
			MaxSInt();
			MaxSInt(int64_t data);

			static MaxSInt Max();
			static MaxSInt Min();

			static const LType kLType = LType::kSInt64;
			static const unsigned int kMaxDecimalDigits = 19;

			void DivMod10(MaxSInt &outQuotient, int8_t &outRemainder) const;
			ResultRV<MaxUInt> ToUnsigned() const;

			bool operator<(const MaxSInt &other) const;
			bool operator<=(const MaxSInt &other) const;
			bool operator>(const MaxSInt &other) const;
			bool operator>=(const MaxSInt &other) const;
			bool operator==(const MaxSInt &other) const;
			bool operator!=(const MaxSInt &other) const;

			MaxSInt &operator=(const MaxSInt &other);

		private:
			int64_t m_data;
		};

		struct MaxUInt final
		{
			MaxUInt();
			MaxUInt(uint64_t data);

			static MaxUInt Max();
			static MaxUInt Min();

			static const LType kLType = LType::kUInt64;
			static const unsigned int kMaxDecimalDigits = 20;

			void DivMod10(MaxUInt &outQuotient, uint8_t &outRemainder) const;
			ResultRV<MaxSInt> ToSigned() const;

			bool operator<(const MaxUInt &other) const;
			bool operator<=(const MaxUInt &other) const;
			bool operator>(const MaxUInt &other) const;
			bool operator>=(const MaxUInt &other) const;
			bool operator==(const MaxUInt &other) const;
			bool operator!=(const MaxUInt &other) const;

			MaxUInt &operator=(const MaxUInt &other);

		private:
			uint64_t m_data;
		};
	}
}
