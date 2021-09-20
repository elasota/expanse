#include "CompilerConstant.h"
#include "ExpAssert.h"

namespace expanse
{
	namespace cc
	{
		CompilerConstant::CompilerConstant()
			: m_u(MaxSInt(0))
			, m_isSigned(true)
			, m_ltype(LType::kSInt32)
		{
		}

		CompilerConstant::~CompilerConstant()
		{
			DestructUnion();
		}

		CompilerConstant::CompilerConstant(LType lType, const MaxUInt &u)
			: m_u(u)
			, m_isSigned(false)
			, m_ltype(lType)
		{
			EXP_ASSERT(lType == LType::kUInt8 || lType == LType::kUInt16 || lType == LType::kUInt32 || lType == LType::kUInt64 || lType == LType::kFloat32 || lType == LType::kFloat64 || lType == LType::kAddress);
		}

		CompilerConstant::CompilerConstant(LType lType, const MaxSInt &u)
			: m_u(u)
			, m_isSigned(true)
			, m_ltype(lType)
		{
			EXP_ASSERT(lType == LType::kSInt8 || lType == LType::kSInt16 || lType == LType::kSInt32 || lType == LType::kSInt64);
		}

		CompilerConstant::CompilerConstant(const CompilerConstant &other)
			: m_u(other.m_isSigned ? VUnion(other.m_u.m_s) : VUnion(other.m_u.m_u))
			, m_ltype(other.m_ltype)
			, m_isSigned(other.m_isSigned)
		{
		}

		LType CompilerConstant::GetLType() const
		{
			return m_ltype;
		}

		MaxSInt CompilerConstant::GetSigned() const
		{
			EXP_ASSERT(m_isSigned);
			return m_u.m_s;
		}

		MaxUInt CompilerConstant::GetUnsigned() const
		{
			EXP_ASSERT(m_isSigned);
			return m_u.m_u;
		}

		CompilerConstant &CompilerConstant::operator=(const CompilerConstant &other)
		{
			if (m_isSigned == other.m_isSigned)
			{
				if (m_isSigned)
					m_u.m_s = other.m_u.m_s;
				else
					m_u.m_u = other.m_u.m_u;
			}
			else
			{
				DestructUnion();
				m_u.~VUnion();

				m_isSigned = other.m_isSigned;

				if (m_isSigned)
					new (&m_u) VUnion(other.m_u.m_s);
				else
					new (&m_u) VUnion(other.m_u.m_u);
			}

			m_ltype = other.m_ltype;

			return *this;
		}

		void CompilerConstant::DestructUnion()
		{
			if (m_isSigned)
				m_u.m_s.~MaxSInt();
			else
				m_u.m_u.~MaxUInt();
		}

		CompilerConstant::VUnion::VUnion(const MaxSInt &v)
			: m_s(v)
		{
		}

		CompilerConstant::VUnion::VUnion(const MaxUInt &v)
			: m_u(v)
		{
		}

		CompilerConstant::VUnion::~VUnion()
		{
		}
	}
}
