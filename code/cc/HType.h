#pragma once

#include <cstdint>

#include "CoreObject.h"
#include "CAggregateType.h"

namespace expanse
{
	namespace cc
	{
		class HAggregateDecl;
		class HEnumDecl;
		class HTypeUnqualifiedInterned;

		struct HTypeQualifiers
		{
			HTypeQualifiers();

			bool operator==(const HTypeQualifiers &other) const;
			bool operator!=(const HTypeQualifiers &other) const;

			HTypeQualifiers &operator|=(const HTypeQualifiers &other);
			HTypeQualifiers operator|(const HTypeQualifiers &other) const;

			uint32_t GetHash() const;

			bool m_isConst;
			bool m_isVolatile;
			bool m_isRestrict;
		};

		struct HTypeIntegral
		{
			enum class IntegralType
			{
				kBool,
				kChar,
				kShort,
				kInt,
				kLongInt,
				kLongLongInt,
			};

			explicit HTypeIntegral(IntegralType intType, bool isUnsigned);
			HTypeIntegral(const HTypeIntegral &other);

			IntegralType m_intType;
			bool m_isUnsigned;

			bool operator==(const HTypeIntegral &other) const;
			bool operator!=(const HTypeIntegral &other) const;

			uint32_t GetHash() const;

		private:
			HTypeIntegral() = delete;
		};

		struct HTypeFloating
		{
			enum class FloatingType
			{
				kSingle,
				kDouble,
				kLongDouble,
			};

			enum class ComplexityClass
			{
				kReal,
				kComplex,
				kImaginary,
			};

			explicit HTypeFloating(FloatingType subtype, ComplexityClass complexityClass);
			HTypeFloating(const HTypeFloating &other);

			bool operator==(const HTypeFloating &other) const;
			bool operator!=(const HTypeFloating &other) const;

			uint32_t GetHash() const;

		private:
			HTypeFloating() = delete;

			FloatingType m_floatingType;
			ComplexityClass m_complexityClass;
		};

		struct HTypeVoid
		{
		};

		struct HTypeEnum
		{
			HTypeEnum(HEnumDecl *decl);

			bool operator==(const HTypeEnum &other) const;
			bool operator!=(const HTypeEnum &other) const;

			uint32_t GetHash() const;

		private:
			HTypeEnum() = delete;

			HEnumDecl *m_decl;
		};

		struct HTypeAggregate
		{
			HTypeAggregate(HAggregateDecl *decl);

			bool operator==(const HTypeAggregate &other) const;
			bool operator!=(const HTypeAggregate &other) const;

			uint32_t GetHash() const;

		private:
			HTypeAggregate() = delete;

			HAggregateDecl *m_decl;
		};

		struct HTypeFunction
		{
			HTypeFunction(HTypeUnqualifiedInterned *unqualRV, HTypeQualifiers rvQualifiers, size_t numParameters);

			bool operator==(const HTypeFunction &other) const;
			bool operator!=(const HTypeFunction &other) const;

			uint32_t GetHash() const;

		private:
			HTypeUnqualifiedInterned *m_unqualRV;
			HTypeQualifiers m_rvQualifiers;
			size_t m_numParameters;
		};

		struct HTypePointer
		{
			HTypePointer(HTypeUnqualifiedInterned *unqualChild, HTypeQualifiers childQualifiers);

			bool operator==(const HTypePointer &other) const;
			bool operator!=(const HTypePointer &other) const;

			uint32_t GetHash() const;

		private:
			HTypeUnqualifiedInterned *m_unqualChild;
			HTypeQualifiers m_childQualifiers;
		};

		union HTypeUnqualifiedUnion
		{
			HTypeIntegral m_int;
			HTypeFloating m_flt;
			HTypeAggregate m_agg;
			HTypeEnum m_enum;
			HTypeFunction m_func;
			HTypePointer m_ptr;

			HTypeUnqualifiedUnion();
			HTypeUnqualifiedUnion(const HTypeIntegral &initialize);
			HTypeUnqualifiedUnion(const HTypeFloating &initialize);
			HTypeUnqualifiedUnion(const HTypeAggregate &initialize);
			HTypeUnqualifiedUnion(const HTypeEnum &initialize);
			HTypeUnqualifiedUnion(const HTypeFunction &initialize);
			HTypeUnqualifiedUnion(const HTypePointer &initialize);
			~HTypeUnqualifiedUnion();
		};

		struct HTypeUnqualified
		{
			enum class Subtype
			{
				kInvalid,

				kVoid,
				kIntegral,
				kFloating,
				kAggregate,
				kEnum,
				kFunction,
				kPointer,
			};

			HTypeUnqualified();
			HTypeUnqualified(const HTypeVoid &voidType);
			HTypeUnqualified(const HTypeIntegral &intType);
			HTypeUnqualified(const HTypeFloating &floatingType);
			HTypeUnqualified(const HTypeAggregate &aggType);
			HTypeUnqualified(const HTypeEnum &enumType);
			HTypeUnqualified(const HTypeFunction &func);
			HTypeUnqualified(const HTypePointer &ptr);
			HTypeUnqualified(const HTypeUnqualified &other);
			~HTypeUnqualified();

			HTypeUnqualified &operator=(const HTypeUnqualified &other);

			bool operator==(const HTypeUnqualified &other) const;
			bool operator!=(const HTypeUnqualified &other) const;

			Subtype GetSubtype() const;

			const HTypeIntegral &GetIntegral() const;
			const HTypeFloating &GetFloating() const;
			const HTypeAggregate &GetAggregate() const;
			const HTypeEnum &GetEnum() const;
			const HTypeFunction &GetFunction() const;
			const HTypePointer &GetPointer() const;

			uint32_t GetHash() const;

		private:
			Subtype m_subtype;
			HTypeUnqualifiedUnion m_u;
		};

		struct HTypeQualified final
		{
			HTypeQualified();
			HTypeQualified(const HTypeUnqualified &unqual, const HTypeQualifiers &qualifiers);
			~HTypeQualified();

			const HTypeUnqualified &GetUnqualified() const;
			const HTypeQualifiers &GetQualifiers() const;

			bool operator==(const HTypeQualified &other) const;
			bool operator!=(const HTypeQualified &other) const;

			uint32_t GetHash() const;

		private:
			HTypeUnqualified m_unqual;
			HTypeQualifiers m_qual;
		};

		class HTypeUnqualifiedInterned final : public CoreObject
		{
		public:
			explicit HTypeUnqualifiedInterned(const HTypeUnqualified &t);

			const HTypeUnqualified &GetType() const;

		private:
			HTypeUnqualified m_t;
		};

		class HAggregateDecl final : public CoreObject
		{
		public:
			explicit HAggregateDecl(CAggregateType aggType);

			CAggregateType GetAggregateType() const;

		private:
			CAggregateType m_aggType;
		};

		class HEnumDecl final : public CoreObject
		{
		public:
			HEnumDecl();

		private:
		};
	}
}

#include "Hasher.h"

namespace expanse
{
	template<>
	class Hasher<cc::HTypeQualified>
	{
	public:
		static Hash_t Compute(const cc::HTypeQualified &key);
	};

	template<>
	class Hasher<cc::HTypeUnqualified>
	{
	public:
		static Hash_t Compute(const cc::HTypeUnqualified &key);
	};
}
