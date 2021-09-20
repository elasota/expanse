#include "CScope.h"

namespace expanse
{
	namespace cc
	{
		CIdentifierBinding::BindingUnion::BindingUnion()
		{
		}

		CIdentifierBinding::BindingUnion::BindingUnion(const HTypeQualified &qualType)
			: m_type(qualType)
		{
		}

		CIdentifierBinding::BindingUnion::BindingUnion(const CSpeculativeGlobalObjectBinding &specGlobal)
			: m_specGlobal(specGlobal)
		{
		}

		CIdentifierBinding::BindingUnion::BindingUnion(const CRealGlobalObjectBinding &realGlobal)
			: m_realGlobal(realGlobal)
		{
		}

		CIdentifierBinding::BindingUnion::BindingUnion(const CIdentifierLocalObjectBinding &localObject)
			: m_localObject(localObject)
		{
		}

		CIdentifierBinding::CIdentifierBinding()
			: m_u()
			, m_bindingType(BindingType::kInvalid)
		{
		}

		CIdentifierBinding::CIdentifierBinding(const CIdentifierBinding &other)
			: m_u()
			, m_bindingType(other.m_bindingType)
		{
			switch (m_bindingType)
			{
			case BindingType::kLocalObject:
				m_u.~BindingUnion();
				new (&m_u) BindingUnion(other.m_u.m_localObject);
				break;
			case BindingType::kRealGlobalObject:
				m_u.~BindingUnion();
				new (&m_u) BindingUnion(other.m_u.m_realGlobal);
				break;
			case BindingType::kSpeculativeGlobalObject:
				m_u.~BindingUnion();
				new (&m_u) BindingUnion(other.m_u.m_specGlobal);
				break;
			case BindingType::kTypeDef:
				m_u.~BindingUnion();
				new (&m_u) BindingUnion(other.m_u.m_type);
				break;
			default:
				break;
			}
		}


		CIdentifierBinding::CIdentifierBinding(const HTypeQualified &qualType)
			: m_u(qualType)
			, m_bindingType(BindingType::kTypeDef)
		{
		}

		CIdentifierBinding::CIdentifierBinding(const CSpeculativeGlobalObjectBinding &specGlobal)
			: m_u(specGlobal)
			, m_bindingType(BindingType::kSpeculativeGlobalObject)
		{
		}

		CIdentifierBinding::CIdentifierBinding(const CRealGlobalObjectBinding &realGlobal)
			: m_u(realGlobal)
			, m_bindingType(BindingType::kRealGlobalObject)
		{
		}

		CIdentifierBinding::CIdentifierBinding(const CIdentifierLocalObjectBinding &localObject)
			: m_u(localObject)
			, m_bindingType(BindingType::kLocalObject)
		{
		}

		CIdentifierBinding::~CIdentifierBinding()
		{
			DestructUnion();
		}

		CIdentifierBinding::BindingType CIdentifierBinding::GetBindingType() const
		{
			return m_bindingType;
		}

		const HTypeQualified *CIdentifierBinding::GetTypeDef() const
		{
			if (m_bindingType != BindingType::kTypeDef)
				return nullptr;

			return &m_u.m_type;
		}

		void CIdentifierBinding::DestructUnion()
		{
			switch (m_bindingType)
			{
			case BindingType::kTypeDef:
				m_u.m_type.~HTypeQualified();
				break;
			case BindingType::kRealGlobalObject:
				m_u.m_realGlobal.~CRealGlobalObjectBinding();
				break;
			case BindingType::kSpeculativeGlobalObject:
				m_u.m_specGlobal.~CSpeculativeGlobalObjectBinding();
				break;
			case BindingType::kLocalObject:
				m_u.m_localObject.~CIdentifierLocalObjectBinding();
				break;
			default:
				break;
			}
		}

		CIdentifierBinding &CIdentifierBinding::operator=(const CIdentifierBinding &other)
		{
			if (this == &other)
				return *this;

			if (m_bindingType == other.m_bindingType)
			{
				switch (m_bindingType)
				{
				case BindingType::kTypeDef:
					m_u.m_type = other.m_u.m_type;
					break;
				case BindingType::kRealGlobalObject:
					m_u.m_realGlobal = other.m_u.m_realGlobal;
					break;
				case BindingType::kSpeculativeGlobalObject:
					m_u.m_specGlobal = other.m_u.m_specGlobal;
					break;
				case BindingType::kLocalObject:
					m_u.m_localObject = other.m_u.m_localObject;
					break;
				default:
					break;
				}
			}
			else
			{
				DestructUnion();
				m_u.~BindingUnion();

				m_bindingType = other.m_bindingType;

				switch (m_bindingType)
				{
				case BindingType::kTypeDef:
					new (&m_u) BindingUnion(other.m_u.m_type);
					break;
				case BindingType::kRealGlobalObject:
					new (&m_u) BindingUnion(other.m_u.m_realGlobal);
					break;
				case BindingType::kSpeculativeGlobalObject:
					new (&m_u) BindingUnion(other.m_u.m_specGlobal);
					break;
				case BindingType::kLocalObject:
					new (&m_u) BindingUnion(other.m_u.m_localObject);
					break;
				default:
					break;
				}
			}

			return *this;
		}

		CTagBinding::BindingUnion::BindingUnion()
		{
		}

		CTagBinding::BindingUnion::BindingUnion(HAggregateDecl *aggDecl)
			: m_aggDecl(aggDecl)
		{
		}

		CTagBinding::BindingUnion::BindingUnion(HEnumDecl *enumDecl)
			: m_enumDecl(enumDecl)
		{
		}

		CTagBinding::BindingUnion::~BindingUnion()
		{
		}

		CTagBinding::CTagBinding()
			: m_bindingType(BindingType::kInvalid)
		{
		}

		CTagBinding::CTagBinding(HAggregateDecl *aggDecl)
			: m_bindingType(BindingType::kAggregate)
			, m_u(aggDecl)
		{
		}

		CTagBinding::CTagBinding(HEnumDecl *enumDecl)
			: m_bindingType(BindingType::kEnum)
			, m_u(enumDecl)
		{
		}

		CTagBinding::~CTagBinding()
		{
			DestructUnion();
		}

		CTagBinding::BindingType CTagBinding::GetBindingType() const
		{
			return m_bindingType;
		}

		HAggregateDecl *CTagBinding::GetAggregateDecl() const
		{
			EXP_ASSERT(m_bindingType == BindingType::kAggregate);
			return m_u.m_aggDecl;
		}

		HEnumDecl *CTagBinding::GetEnumDecl() const
		{
			EXP_ASSERT(m_bindingType == BindingType::kEnum);
			return m_u.m_enumDecl;
		}

		CTagBinding &CTagBinding::operator=(const CTagBinding &other)
		{
			if (this == &other)
				return *this;

			DestructUnion();

			m_bindingType = other.m_bindingType;

			switch (m_bindingType)
			{
			case BindingType::kAggregate:
				new (&m_u) BindingUnion(other.m_u.m_aggDecl);
				break;
			case BindingType::kEnum:
				new (&m_u) BindingUnion(other.m_u.m_enumDecl);
				break;
			default:
				new (&m_u) BindingUnion();
				break;
			}

			return *this;
		}

		bool CTagBinding::operator==(const CTagBinding &other) const
		{
			if (m_bindingType != other.m_bindingType)
				return false;

			switch (m_bindingType)
			{
			case BindingType::kAggregate:
				return m_u.m_aggDecl == other.m_u.m_aggDecl;
			case BindingType::kEnum:
				return m_u.m_enumDecl == other.m_u.m_enumDecl;
			default:
				return true;
			}
		}

		bool CTagBinding::operator!=(const CTagBinding &other) const
		{
			return !((*this) == other);
		}

		void CTagBinding::DestructUnion()
		{
		}


		CScope::CScope(IAllocator *alloc, CScope *parentScope)
			: m_parentScope(parentScope)
			, m_symbols(*alloc)
			, m_tags(*alloc)
			, m_labels(*alloc)
		{
		}

		CScope *CScope::GetParentScope() const
		{
			return m_parentScope;
		}

		const CIdentifierBinding *CScope::GetSymbolLocal(const TokenStrView &token) const
		{
			HashMapConstIterator<TokenStrView, CIdentifierBinding> it = m_symbols.Find(token);
			if (it == m_symbols.end())
				return nullptr;
			return &it.Value();
		}

		const CIdentifierBinding *CScope::GetSymbolRecursive(const TokenStrView &token) const
		{
			const CScope *scope = this;

			while (scope != nullptr)
			{
				const CIdentifierBinding *binding = scope->GetSymbolLocal(token);
				if (binding != nullptr)
					return binding;

				scope = scope->m_parentScope;
			}

			return nullptr;
		}

		const CTagBinding *CScope::GetTagLocal(const TokenStrView &token) const
		{
			HashMapConstIterator<TokenStrView, CTagBinding> it = m_tags.Find(token);
			if (it == m_tags.end())
				return nullptr;
			return &it.Value();
		}

		const CTagBinding *CScope::GetTagRecursive(const TokenStrView &token) const
		{
			const CScope *scope = this;

			while (scope != nullptr)
			{
				const CTagBinding *binding = scope->GetTagLocal(token);
				if (binding != nullptr)
					return binding;

				scope = scope->m_parentScope;
			}

			return nullptr;
		}

		Result CScope::DefineIdentifier(const TokenStrView &name, const CIdentifierBinding &binding)
		{
			return m_symbols.Insert(name, binding);
		}

		Result CScope::AddTag(const TokenStrView &token, const CTagBinding &tagBinding)
		{
			return m_tags.Insert(token, tagBinding);
		}
	}
}
