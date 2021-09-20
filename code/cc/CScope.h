#pragma once

#include "CoreObject.h"
#include "HashMap.h"
#include "HType.h"
#include "PPTokenStr.h"

namespace expanse
{
	struct IAllocator;
	template<class T> struct ResultRV;

	namespace cc
	{
		class CType;
		class CGlobalDefinition;
		class HAggregateDecl;
		class HEnumDecl;

		struct CSpeculativeGlobalObjectBinding
		{
			CSpeculativeGlobalObjectBinding();
			CSpeculativeGlobalObjectBinding(const HTypeQualified &apparentType, size_t index);

			HTypeQualified m_apparentType;
			size_t m_index;
		};

		struct CRealGlobalObjectBinding
		{
			CRealGlobalObjectBinding();
			CRealGlobalObjectBinding(size_t index);

			size_t m_index;
		};

		struct CIdentifierLocalObjectBinding
		{
			CIdentifierLocalObjectBinding();
			explicit CIdentifierLocalObjectBinding(size_t index);

			size_t m_index;
		};

		struct CIdentifierBinding
		{
			enum class BindingType
			{
				kInvalid,

				kTypeDef,
				kSpeculativeGlobalObject,
				kRealGlobalObject,
				kLocalObject,
			};

			union BindingUnion
			{
				HTypeQualified m_type;
				CSpeculativeGlobalObjectBinding m_specGlobal;
				CRealGlobalObjectBinding m_realGlobal;
				CIdentifierLocalObjectBinding m_localObject;

				BindingUnion();
				BindingUnion(const HTypeQualified &qualType);
				BindingUnion(const CSpeculativeGlobalObjectBinding &specGlobal);
				BindingUnion(const CRealGlobalObjectBinding &realGlobal);
				BindingUnion(const CIdentifierLocalObjectBinding &localObject);
				~BindingUnion();
			};

			CIdentifierBinding();
			CIdentifierBinding(const CIdentifierBinding &other);
			explicit CIdentifierBinding(const HTypeQualified &qualType);
			explicit CIdentifierBinding(const CSpeculativeGlobalObjectBinding &specGlobal);
			explicit CIdentifierBinding(const CRealGlobalObjectBinding &realGlobal);
			explicit CIdentifierBinding(const CIdentifierLocalObjectBinding &localObject);
			~CIdentifierBinding();

			BindingType GetBindingType() const;
			const HTypeQualified *GetTypeDef() const;
			size_t GetObjectIndex() const;

			CIdentifierBinding &operator=(const CIdentifierBinding &other);

		private:
			void DestructUnion();

			BindingUnion m_u;
			BindingType m_bindingType;
		};

		struct CTagBinding
		{
			enum class BindingType
			{
				kInvalid,

				kAggregate,
				kEnum,
			};

			union BindingUnion
			{
				HAggregateDecl *m_aggDecl;
				HEnumDecl *m_enumDecl;

				BindingUnion();
				BindingUnion(HAggregateDecl *aggDecl);
				BindingUnion(HEnumDecl *enumDecl);
				~BindingUnion();
			};

			CTagBinding();
			CTagBinding(HAggregateDecl *aggDecl);
			CTagBinding(HEnumDecl *enumDecl);
			~CTagBinding();

			BindingType GetBindingType() const;
			HAggregateDecl *GetAggregateDecl() const;
			HEnumDecl *GetEnumDecl() const;

			CTagBinding &operator=(const CTagBinding &other);

			bool operator==(const CTagBinding &other) const;
			bool operator!=(const CTagBinding &other) const;

		private:
			void DestructUnion();

			BindingUnion m_u;
			BindingType m_bindingType;
		};

		struct CLabelBinding
		{
		};

		class CScope final : public CoreObject
		{
		public:
			explicit CScope(IAllocator *alloc, CScope *parentScope);

			CScope *GetParentScope() const;
			const CIdentifierBinding *GetSymbolLocal(const TokenStrView &token) const;
			const CIdentifierBinding *GetSymbolRecursive(const TokenStrView &token) const;

			const CTagBinding *GetTagLocal(const TokenStrView &token) const;
			const CTagBinding *GetTagRecursive(const TokenStrView &token) const;

			Result DefineIdentifier(const TokenStrView &token, const CIdentifierBinding &binding);

			Result AddTag(const TokenStrView &token, const CTagBinding &tagBinding);

		private:
			HashMap<TokenStrView, CIdentifierBinding> m_symbols;
			HashMap<TokenStrView, CTagBinding> m_tags;
			HashMap<TokenStrView, CLabelBinding> m_labels;

			CScope *m_parentScope;
		};
	}
}
