#include "HType.h"
#include "Hasher.h"
#include "ExpAssert.h"

namespace expanse
{
	namespace cc
	{
		HTypeQualifiers::HTypeQualifiers()
			: m_isConst(false)
			, m_isVolatile(false)
			, m_isRestrict(false)
		{
		}

		bool HTypeQualifiers::operator==(const HTypeQualifiers &other) const
		{
			return m_isConst == other.m_isConst && m_isVolatile == other.m_isVolatile && m_isRestrict == other.m_isRestrict;
		}

		bool HTypeQualifiers::operator!=(const HTypeQualifiers &other) const
		{
			return !((*this) == other);
		}

		HTypeQualifiers &HTypeQualifiers::operator|=(const HTypeQualifiers &other)
		{
			m_isConst |= other.m_isConst;
			m_isVolatile |= other.m_isVolatile;
			m_isRestrict |= other.m_isRestrict;
			return *this;
		}

		HTypeQualifiers HTypeQualifiers::operator|(const HTypeQualifiers &other) const
		{
			HTypeQualifiers clone = (*this);
			clone |= other;
			return clone;
		}

		uint32_t HTypeQualifiers::GetHash() const
		{
			uint8_t inputs[] = { m_isConst, m_isVolatile, m_isRestrict };
			return HashUtil::ComputePODHash(inputs, sizeof(inputs));
		}

		HTypeIntegral::HTypeIntegral(IntegralType intType, bool isUnsigned)
			: m_intType(intType)
			, m_isUnsigned(isUnsigned)
		{
		}

		HTypeIntegral::HTypeIntegral(const HTypeIntegral &other)
			: m_intType(other.m_intType)
			, m_isUnsigned(other.m_isUnsigned)
		{
		}

		bool HTypeIntegral::operator==(const HTypeIntegral &other) const
		{
			return m_intType == other.m_intType && m_isUnsigned == other.m_isUnsigned;
		}

		bool HTypeIntegral::operator!=(const HTypeIntegral &other) const
		{
			return !((*this) == other);
		}

		uint32_t HTypeIntegral::GetHash() const
		{
			uint8_t inputs[] = { static_cast<uint8_t>(m_intType), static_cast<uint8_t>(m_isUnsigned ? 0 : 1) };

			return HashUtil::ComputePODHash(inputs, sizeof(inputs));
		}

		HTypeFloating::HTypeFloating(FloatingType subtype, ComplexityClass complexityClass)
			: m_floatingType(subtype)
			, m_complexityClass(complexityClass)
		{
		}

		HTypeFloating::HTypeFloating(const HTypeFloating &other)
			: m_floatingType(other.m_floatingType)
			, m_complexityClass(other.m_complexityClass)
		{
		}

		bool HTypeFloating::operator==(const HTypeFloating &other) const
		{
			return m_floatingType == other.m_floatingType && m_complexityClass == other.m_complexityClass;
		}

		bool HTypeFloating::operator!=(const HTypeFloating &other) const
		{
			return !((*this) == other);
		}

		uint32_t HTypeFloating::GetHash() const
		{
			uint8_t inputs[] = { static_cast<uint8_t>(m_floatingType), static_cast<uint8_t>(m_complexityClass) };
			return HashUtil::ComputePODHash(inputs, sizeof(inputs));
		}

		HTypeEnum::HTypeEnum(HEnumDecl *decl)
			: m_decl(decl)
		{
		}

		bool HTypeEnum::operator==(const HTypeEnum &other) const
		{
			return m_decl == other.m_decl;
		}

		bool HTypeEnum::operator!=(const HTypeEnum &other) const
		{
			return !((*this) == other);
		}

		uint32_t HTypeEnum::GetHash() const
		{
			uintptr_t inputs[] = { reinterpret_cast<uintptr_t>(m_decl) };
			return HashUtil::ComputePODHash(inputs, sizeof(inputs));
		}


		HTypeAggregate::HTypeAggregate(HAggregateDecl *decl)
			: m_decl(decl)
		{
		}

		bool HTypeAggregate::operator==(const HTypeAggregate &other) const
		{
			return m_decl == other.m_decl;
		}

		bool HTypeAggregate::operator!=(const HTypeAggregate &other) const
		{
			return !((*this) == other);
		}

		uint32_t HTypeAggregate::GetHash() const
		{
			uintptr_t inputs[] = { reinterpret_cast<uintptr_t>(m_decl) };
			return HashUtil::ComputePODHash(inputs, sizeof(inputs));
		}

		HTypeFunction::HTypeFunction(HTypeUnqualifiedInterned *unqualRV, HTypeQualifiers rvQualifiers, size_t numParameters)
			: m_unqualRV(unqualRV)
			, m_rvQualifiers(rvQualifiers)
			, m_numParameters(numParameters)
		{
		}

		bool HTypeFunction::operator==(const HTypeFunction &other) const
		{
			return m_unqualRV == other.m_unqualRV && m_rvQualifiers == other.m_rvQualifiers && m_numParameters == other.m_numParameters;
		}

		bool HTypeFunction::operator!=(const HTypeFunction &other) const
		{
			return !((*this) == other);
		}

		uint32_t HTypeFunction::GetHash() const
		{
			uintptr_t inputs[] = { reinterpret_cast<uintptr_t>(m_unqualRV), m_rvQualifiers.GetHash(), m_numParameters };
			return HashUtil::ComputePODHash(inputs, sizeof(inputs));
		}

		HTypePointer::HTypePointer(HTypeUnqualifiedInterned *unqualChild, HTypeQualifiers childQualifiers)
			: m_unqualChild(unqualChild)
			, m_childQualifiers(childQualifiers)
		{
		}

		bool HTypePointer::operator==(const HTypePointer &other) const
		{
			return m_unqualChild == other.m_unqualChild && m_childQualifiers == other.m_childQualifiers;
		}

		bool HTypePointer::operator!=(const HTypePointer &other) const
		{
			return !((*this) == other);
		}

		uint32_t HTypePointer::GetHash() const
		{
			uintptr_t inputs[] = { reinterpret_cast<uintptr_t>(m_unqualChild), m_childQualifiers.GetHash() };
			return HashUtil::ComputePODHash(inputs, sizeof(inputs));
		}

		HTypeUnqualifiedUnion::HTypeUnqualifiedUnion()
		{
		}

		HTypeUnqualifiedUnion::HTypeUnqualifiedUnion(const HTypeIntegral &initialize)
			: m_int(initialize)
		{
		}

		HTypeUnqualifiedUnion::HTypeUnqualifiedUnion(const HTypeFloating &initialize)
			: m_flt(initialize)
		{
		}

		HTypeUnqualifiedUnion::HTypeUnqualifiedUnion(const HTypeAggregate &initialize)
			: m_agg(initialize)
		{
		}

		HTypeUnqualifiedUnion::HTypeUnqualifiedUnion(const HTypeEnum &initialize)
			: m_enum(initialize)
		{
		}

		HTypeUnqualifiedUnion::HTypeUnqualifiedUnion(const HTypeFunction &initialize)
			: m_func(initialize)
		{
		}

		HTypeUnqualifiedUnion::HTypeUnqualifiedUnion(const HTypePointer &initialize)
			: m_ptr(initialize)
		{
		}

		HTypeUnqualifiedUnion::~HTypeUnqualifiedUnion()
		{
		}

		HTypeUnqualified::HTypeUnqualified()
			: m_u()
			, m_subtype(Subtype::kInvalid)
		{
		}

		HTypeUnqualified::HTypeUnqualified(const HTypeVoid &voidType)
			: m_u()
			, m_subtype(Subtype::kVoid)
		{
		}

		HTypeUnqualified::HTypeUnqualified(const HTypeIntegral &intType)
			: m_u(intType)
			, m_subtype(Subtype::kIntegral)
		{
		}

		HTypeUnqualified::HTypeUnqualified(const HTypeFloating &floatingType)
			: m_u(floatingType)
			, m_subtype(Subtype::kFloating)
		{
		}

		HTypeUnqualified::HTypeUnqualified(const HTypeAggregate &aggType)
			: m_u(aggType)
			, m_subtype(Subtype::kAggregate)
		{
		}

		HTypeUnqualified::HTypeUnqualified(const HTypeEnum &enumType)
			: m_u(enumType)
			, m_subtype(Subtype::kEnum)
		{
		}

		HTypeUnqualified::HTypeUnqualified(const HTypeFunction &func)
			: m_u(func)
			, m_subtype(Subtype::kFunction)
		{
		}

		HTypeUnqualified::HTypeUnqualified(const HTypePointer &ptr)
			: m_u(ptr)
			, m_subtype(Subtype::kPointer)
		{
		}

		HTypeUnqualified::HTypeUnqualified(const HTypeUnqualified &other)
			: m_u()
			, m_subtype(Subtype::kVoid)
		{
			(*this) = other;
		}

		HTypeUnqualified::~HTypeUnqualified()
		{
		}

		HTypeUnqualified &HTypeUnqualified::operator=(const HTypeUnqualified &other)
		{
			if (this->m_subtype == other.m_subtype)
			{
				switch (m_subtype)
				{
				case Subtype::kIntegral:
					m_u.m_int = other.m_u.m_int;
					break;
				case Subtype::kFloating:
					m_u.m_flt = other.m_u.m_flt;
					break;
				case Subtype::kAggregate:
					m_u.m_agg = other.m_u.m_agg;
					break;
				case Subtype::kEnum:
					m_u.m_enum = other.m_u.m_enum;
					break;
				case Subtype::kFunction:
					m_u.m_func = other.m_u.m_func;
					break;
				case Subtype::kPointer:
					m_u.m_ptr = other.m_u.m_ptr;
					break;
				case Subtype::kVoid:
				default:
					break;
				}
			}
			else
			{
				switch (m_subtype)
				{
				case Subtype::kIntegral:
					m_u.m_int.~HTypeIntegral();
					break;
				case Subtype::kFloating:
					m_u.m_flt.~HTypeFloating();
					break;
				case Subtype::kAggregate:
					m_u.m_agg.~HTypeAggregate();
					break;
				case Subtype::kEnum:
					m_u.m_enum.~HTypeEnum();
					break;
				case Subtype::kFunction:
					m_u.m_func.~HTypeFunction();
					break;
				case Subtype::kPointer:
					m_u.m_ptr.~HTypePointer();
					break;
				case Subtype::kVoid:
				default:
					break;
				}
				m_u.~HTypeUnqualifiedUnion();

				m_subtype = other.m_subtype;

				switch (m_subtype)
				{
				case Subtype::kVoid:
					new (&m_u) HTypeUnqualifiedUnion();
					break;
				case Subtype::kIntegral:
					new (&m_u) HTypeUnqualifiedUnion(other.m_u.m_int);
					break;
				case Subtype::kFloating:
					new (&m_u) HTypeUnqualifiedUnion(other.m_u.m_flt);
					break;
				case Subtype::kAggregate:
					new (&m_u) HTypeUnqualifiedUnion(other.m_u.m_agg);
					break;
				case Subtype::kEnum:
					new (&m_u) HTypeUnqualifiedUnion(other.m_u.m_enum);
					break;
				case Subtype::kFunction:
					new (&m_u) HTypeUnqualifiedUnion(other.m_u.m_func);
					break;
				case Subtype::kPointer:
					new (&m_u) HTypeUnqualifiedUnion(other.m_u.m_ptr);
					break;
				default:
					break;
				}
			}

			return *this;
		}

		bool HTypeUnqualified::operator==(const HTypeUnqualified &other) const
		{
			if (this == &other)
				return true;

			if (m_subtype != other.m_subtype)
				return false;

			switch (m_subtype)
			{
			case Subtype::kVoid:
				return true;
			case Subtype::kIntegral:
				return m_u.m_int == other.m_u.m_int;
			case Subtype::kFloating:
				return m_u.m_flt == other.m_u.m_flt;
			case Subtype::kAggregate:
				return m_u.m_agg == other.m_u.m_agg;
			case Subtype::kEnum:
				return m_u.m_enum == other.m_u.m_enum;
			case Subtype::kFunction:
				return m_u.m_func == other.m_u.m_func;
			case Subtype::kPointer:
				return m_u.m_ptr == other.m_u.m_ptr;
			default:
				return false;
			}
		}

		bool HTypeUnqualified::operator!=(const HTypeUnqualified &other) const
		{
			return !((*this) == other);
		}


		HTypeUnqualified::Subtype HTypeUnqualified::GetSubtype() const
		{
			return m_subtype;
		}

		const HTypeIntegral &HTypeUnqualified::GetIntegral() const
		{
			EXP_ASSERT(m_subtype == Subtype::kIntegral);
			return m_u.m_int;
		}

		const HTypeFloating &HTypeUnqualified::GetFloating() const
		{
			EXP_ASSERT(m_subtype == Subtype::kFloating);
			return m_u.m_flt;
		}

		const HTypeAggregate &HTypeUnqualified::GetAggregate() const
		{
			EXP_ASSERT(m_subtype == Subtype::kAggregate);
			return m_u.m_agg;
		}

		const HTypeEnum &HTypeUnqualified::GetEnum() const
		{
			EXP_ASSERT(m_subtype == Subtype::kEnum);
			return m_u.m_enum;
		}

		const HTypeFunction &HTypeUnqualified::GetFunction() const
		{
			EXP_ASSERT(m_subtype == Subtype::kFunction);
			return m_u.m_func;
		}

		const HTypePointer &HTypeUnqualified::GetPointer() const
		{
			EXP_ASSERT(m_subtype == Subtype::kPointer);
			return m_u.m_ptr;
		}


		uint32_t HTypeUnqualified::GetHash() const
		{
			uint32_t subhash = 0;

			switch (m_subtype)
			{
				break;
			case Subtype::kIntegral:
				subhash = m_u.m_int.GetHash();
				break;
			case Subtype::kFloating:
				subhash = m_u.m_flt.GetHash();
				break;
			case Subtype::kAggregate:
				subhash = m_u.m_agg.GetHash();
				break;
			case Subtype::kEnum:
				subhash = m_u.m_enum.GetHash();
				break;
			case Subtype::kFunction:
				subhash = m_u.m_func.GetHash();
				break;
			case Subtype::kPointer:
				subhash = m_u.m_ptr.GetHash();
				break;
			case Subtype::kVoid:
			default:
				subhash = 0;
				break;
			}

			uint32_t inputs[] = { static_cast<uint32_t>(m_subtype), subhash };

			return HashUtil::ComputePODHash(inputs, sizeof(inputs));
		}

		HTypeQualified::HTypeQualified()
		{
		}

		HTypeQualified::HTypeQualified(const HTypeUnqualified &unqual, const HTypeQualifiers &qual)
			: m_unqual(unqual)
			, m_qual(qual)
		{
		}

		HTypeQualified::~HTypeQualified()
		{
		}

		const HTypeUnqualified &HTypeQualified::GetUnqualified() const
		{
			return m_unqual;
		}

		const HTypeQualifiers &HTypeQualified::GetQualifiers() const
		{
			return m_qual;
		}

		bool HTypeQualified::operator==(const HTypeQualified &other) const
		{
			return m_qual == other.m_qual && m_unqual == other.m_unqual;
		}

		bool HTypeQualified::operator!=(const HTypeQualified &other) const
		{
			return !((*this) == other);
		}


		uint32_t HTypeQualified::GetHash() const
		{
			uint32_t inputs[] = { m_qual.GetHash(), m_unqual.GetHash() };
			return HashUtil::ComputePODHash(inputs, sizeof(inputs));
		}

		HTypeUnqualifiedInterned::HTypeUnqualifiedInterned(const HTypeUnqualified &t)
			: m_t(t)
		{
		}

		const HTypeUnqualified &HTypeUnqualifiedInterned::GetType() const
		{
			return m_t;
		}

		HAggregateDecl::HAggregateDecl(CAggregateType aggType)
			: m_aggType(aggType)
		{
		}

		CAggregateType HAggregateDecl::GetAggregateType() const
		{
			return m_aggType;
		}

		HEnumDecl::HEnumDecl()
		{
		}
	}
}


namespace expanse
{
	Hash_t Hasher<cc::HTypeQualified>::Compute(const cc::HTypeQualified &key)
	{
		return key.GetHash();
	}

	Hash_t Hasher<cc::HTypeUnqualified>::Compute(const cc::HTypeUnqualified &key)
	{
		return key.GetHash();
	}
}
