#include "MaxInt.h"
#include "Result.h"
#include "ResultRV.h"

#include <limits>

namespace expanse
{
	namespace cc
	{
		MaxSInt::MaxSInt()
			: m_data(0)
		{
		}

		MaxSInt::MaxSInt(int64_t data)
			: m_data(data)
		{
		}

		MaxSInt MaxSInt::Max()
		{
			return std::numeric_limits<int64_t>::max();
		}

		MaxSInt MaxSInt::Min()
		{
			return std::numeric_limits<int64_t>::min();
		}

		void MaxSInt::DivMod10(MaxSInt &outQuotient, int8_t &outRemainder) const
		{
			int64_t remainder = m_data % 10;
			int64_t quotient = m_data / 10;
			outQuotient = MaxSInt(quotient);
			outRemainder = static_cast<int8_t>(remainder);
		}

		ResultRV<MaxUInt> MaxSInt::ToUnsigned() const
		{
			if (m_data < 0)
				return ErrorCode::kArithmeticOverflow;

			return MaxUInt(static_cast<uint64_t>(m_data));
		}

		bool MaxSInt::operator<(const MaxSInt &other) const
		{
			return m_data < other.m_data;
		}

		bool MaxSInt::operator<=(const MaxSInt &other) const
		{
			return m_data <= other.m_data;
		}

		bool MaxSInt::operator>(const MaxSInt &other) const
		{
			return m_data > other.m_data;
		}

		bool MaxSInt::operator>=(const MaxSInt &other) const
		{
			return m_data >= other.m_data;
		}

		bool MaxSInt::operator==(const MaxSInt &other) const
		{
			return m_data == other.m_data;
		}

		bool MaxSInt::operator!=(const MaxSInt &other) const
		{
			return m_data != other.m_data;
		}

		MaxSInt &MaxSInt::operator=(const MaxSInt &other)
		{
			m_data = other.m_data;
			return *this;
		}

		MaxUInt::MaxUInt()
			: m_data(0)
		{
		}

		MaxUInt::MaxUInt(uint64_t data)
			: m_data(data)
		{
		}

		MaxUInt MaxUInt::Max()
		{
			return std::numeric_limits<uint64_t>::max();
		}

		MaxUInt MaxUInt::Min()
		{
			return std::numeric_limits<uint64_t>::min();
		}

		void MaxUInt::DivMod10(MaxUInt &outQuotient, uint8_t &outRemainder) const
		{
			uint64_t remainder = m_data % 10;
			uint64_t quotient = m_data / 10;
			outQuotient = MaxUInt(quotient);
			outRemainder = static_cast<uint8_t>(remainder);
		}

		ResultRV<MaxSInt> MaxUInt::ToSigned() const
		{
			if (m_data > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
				return ErrorCode::kArithmeticOverflow;
			return MaxSInt(static_cast<int64_t>(m_data));
		}

		bool MaxUInt::operator<(const MaxUInt &other) const
		{
			return m_data < other.m_data;
		}

		bool MaxUInt::operator<=(const MaxUInt &other) const
		{
			return m_data < other.m_data;
		}

		bool MaxUInt::operator>(const MaxUInt &other) const
		{
			return m_data > other.m_data;
		}

		bool MaxUInt::operator>=(const MaxUInt &other) const
		{
			return m_data >= other.m_data;
		}

		bool MaxUInt::operator==(const MaxUInt &other) const
		{
			return m_data == other.m_data;
		}

		bool MaxUInt::operator!=(const MaxUInt &other) const
		{
			return m_data != other.m_data;
		}


		MaxUInt &MaxUInt::operator=(const MaxUInt &other)
		{
			m_data = other.m_data;
			return *this;
		}
	}
}
