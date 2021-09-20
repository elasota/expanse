#pragma once

#include "ArrayPtr.h"
#include "CAggregateType.h"
#include "CoreObject.h"
#include "CorePtr.h"
#include "FileCoordinate.h"
#include "Vector.h"
#include "PPTokenStr.h"

namespace expanse
{
	struct IAllocator;

	namespace cc
	{
		class CAbstractDeclarator;
		class CDeclarator;
		class CTypeName;
		class CInitializerList;
		class CInitializer;

		class CGrammarElement : public CoreObject
		{
		public:
			enum class Subtype
			{
				kStorageClassSpecifier,
				kTypeSpecifier,
				kTypeDefNameSpecifier,
				kTypeQualifier,
				kFunctionSpecifier,
				kDeclarationSpecifiers,
				kTypeQualifierList,
				kTypeName,
				kParameterDeclaration,
				kIdentifierList,
				kDirectDeclarator,
				kDirectDeclaratorContinuation,
				kDirectAbstractDeclarator,
				kDirectAbstractDeclaratorSuffix,
				kPointer,
				kParameterTypeList,
				kDeclarator,
				kAbstractDeclarator,
				kEnumerator,
				kEnumeratorList,
				kEnumSpecifier,
				kSpecifierQualifierList,
				kStructDeclarator,
				kStructDeclaratorList,
				kStructDeclaration,
				kStructDeclarationList,
				kStructOrUnionSpecifier,
				kExpression,
				kArgumentExpressionList,
				kInitializer,
				kInitializerList,
				kDesignator,
				kDesignatorList,
				kDesignation,
				kDesignatableInitializer,
				kIdentifier,
				kInitDeclarator,
				kInitDeclaratorList,
				kDeclarationList,
				kDeclaration,
			};

			explicit CGrammarElement(Subtype subtype);

			Subtype GetSubtype() const;

		private:
			Subtype m_subtype;
		};

		template<class T>
		class CGrammarElementListContainer : public CGrammarElement
		{
		public:
			CGrammarElementListContainer(CGrammarElement::Subtype subtype, ArrayPtr<CorePtr<T>> &&children);

			ArrayView<CorePtr<T>> GetChildren() const;

		private:
			ArrayPtr<CorePtr<T>> m_children;
		};

		class CDeclarationSpecifiers final : public CGrammarElementListContainer<CGrammarElement>
		{
		public:
			// Contains a list of mixed kStorageClassSpecifier, kTypeQualifier, kFunctionSpecifier, kTypeSpecifier, kTypeDefNameSpecifier, kStructOrUnionSpecifier, kEnumSpecifier
			explicit CDeclarationSpecifiers(ArrayPtr<CorePtr<CGrammarElement>> &&children);
		};

		class CToken final : public CGrammarElement
		{
		public:
			explicit CToken(Subtype subtype, const TokenStrView &token, const FileCoordinate &coordinate);

			const TokenStrView &GetToken() const;
			const FileCoordinate &GetCoordinate() const;

		private:
			TokenStrView m_token;
			FileCoordinate m_coord;
		};

		class CTypeQualifierList final : public CGrammarElementListContainer<CToken>
		{
		public:
			explicit CTypeQualifierList(ArrayPtr<CorePtr<CToken>> &&children);
		};

		enum class CBinaryOperator
		{
			kInvalid,

			kAssign,
			kMulAssign,
			kDivAssign,
			kModAssign,
			kAddAssign,
			kSubAssign,
			kLshAssign,
			kRshAssign,
			kBitAndAssign,
			kBitXorAssign,
			kBitOrAssign,

			kLogicalOr,
			kLogicalAnd,
			kBitOr,
			kBitXor,
			kBitAnd,
			kEqual,
			kNotEqual,
			kGreater,
			kLess,
			kGreaterOrEqual,
			kLessOrEqual,
			kRsh,
			kLsh,
			kAdd,
			kSub,
			kMul,
			kDiv,
			kMod,
			kComma,
			kIndex,
		};

		enum class CUnaryOperator
		{
			kInvalid,

			kPreIncrement,
			kPreDecrement,
			kPostIncrement,
			kPostDecrement,
			kReference,
			kDereference,
			kAbs,
			kNeg,
			kBitNot,
			kLogicalNot,
			kSizeOfExpr,

			kTempArgument,	// Transient operator used to parse argument lists
		};

		class CExpression : public CGrammarElement
		{
		public:
			enum class ExpressionSubtype
			{
				kUnary,
				kBinary,
				kTernary,
				kCast,
				kSizeOfType,
				kMember,
				kTypedInitializer,
				kTokenExpression,
				kInvoke,
			};

			explicit CExpression(ExpressionSubtype exprSubtype);
			ExpressionSubtype GetExprSubtype() const;

		private:
			ExpressionSubtype m_exprType;
		};

		class CMemberExpression final : public CExpression
		{
		public:
			CMemberExpression(CorePtr<CExpression> &&leftSide, CorePtr<CToken> &&identifier, bool dereference);

		public:
			CorePtr<CExpression> m_leftSide;
			CorePtr<CToken> m_identifier;
			bool m_dereference;
		};

		class CSizeOfTypeExpression final : public CExpression
		{
		public:
			CSizeOfTypeExpression(CorePtr<CTypeName> &&typeName);

		public:
			CorePtr<CTypeName> m_typeName;
		};

		class CUnaryExpression final : public CExpression
		{
		public:
			CUnaryExpression(CorePtr<CExpression> &&expr, CUnaryOperator unaryOp);

		public:
			CorePtr<CExpression> m_expr;
			CUnaryOperator m_unaryOp;
		};

		class CBinaryExpression final : public CExpression
		{
		public:
			CBinaryExpression(CorePtr<CExpression> &&leftExpr, CorePtr<CExpression> &&rightExpr, CBinaryOperator binOp);

		public:
			CorePtr<CExpression> m_leftExpr;
			CorePtr<CExpression> m_rightExpr;

			CBinaryOperator m_binOp;
		};

		class CTernaryExpression final : public CExpression
		{
		public:
			CTernaryExpression(CorePtr<CExpression> &&conditionExpr, CorePtr<CExpression> &&trueExpr, CorePtr<CExpression> &&falseExpr);

		public:
			CorePtr<CExpression> m_conditionExpr;
			CorePtr<CExpression> m_trueExpr;
			CorePtr<CExpression> m_falseExpr;
		};

		class CCastExpression final : public CExpression
		{
		public:
			CCastExpression(CorePtr<CTypeName> &&typeName, CorePtr<CExpression> &&rightSideExpr);

		public:
			CorePtr<CTypeName> m_typeName;
			CorePtr<CExpression> m_rightSideExpr;
		};		

		class CParameterDeclaration final : public CGrammarElement
		{
		public:
			enum class ParameterDeclarationSubtype
			{
				kDeclarator,
				kAbstractDeclarator,
				kNothing,
			};

			explicit CParameterDeclaration(CorePtr<CDeclarationSpecifiers> &&declSpecifiers);
			explicit CParameterDeclaration(CorePtr<CDeclarationSpecifiers> &&declSpecifiers, CorePtr<CDeclarator> &&declarator);
			explicit CParameterDeclaration(CorePtr<CDeclarationSpecifiers> &&declSpecifiers, CorePtr<CAbstractDeclarator> &&absDeclarator);

		private:
			CorePtr<CDeclarationSpecifiers> m_declSpecifiers;
			CorePtr<CDeclarator> m_declarator;
			CorePtr<CAbstractDeclarator> m_absDeclarator;
		};

		class CParameterTypeList final : public CGrammarElementListContainer<CParameterDeclaration>
		{
		public:
			explicit CParameterTypeList(ArrayPtr<CorePtr<CParameterDeclaration>> &&paramDecls, bool hasVarArgs);

		private:
			bool m_hasVarArgs;
		};

		class CIdentifierList final : public CGrammarElementListContainer<CToken>
		{
		public:
			explicit CIdentifierList(ArrayPtr<CorePtr<CToken>> &&identifiers);
		};

		class CDirectDeclaratorContinuation final : public CGrammarElement
		{
		public:
			enum class ContinuationType
			{
				kSquareBracket,
				kParamTypeList,
				kIdentifierList,
			};

			explicit CDirectDeclaratorContinuation(CorePtr<CTypeQualifierList> &&typeQualifierList, CorePtr<CExpression> &&assignmentExpr, bool hasStatic, bool hasAsterisk, const FileCoordinate &coord);
			explicit CDirectDeclaratorContinuation(CorePtr<CParameterTypeList> &&parameterTypeList, const FileCoordinate &coord);
			explicit CDirectDeclaratorContinuation(CorePtr<CIdentifierList> &&identifierList, const FileCoordinate &coord);

			const CTypeQualifierList *GetTypeQualifierList() const;
			const CExpression *GetAssignmentExpr() const;
			const CParameterTypeList *GetParameterTypeList() const;
			const CIdentifierList *GetIdentifierList() const;
			ContinuationType GetContinuationType() const;
			bool HasStatic() const;
			bool HasAsterisk() const;

			const FileCoordinate &GetCoordinate() const;

		private:
			CorePtr<CTypeQualifierList> m_typeQualifierList;
			CorePtr<CExpression> m_assignmentExpr;
			CorePtr<CParameterTypeList> m_parameterTypeList;
			CorePtr<CIdentifierList> m_identifierList;

			ContinuationType m_contType;
			bool m_hasStatic;
			bool m_hasAsterisk;
			FileCoordinate m_coord;
		};

		class CDirectDeclarator final : public CGrammarElement
		{
		public:
			enum class DirectDeclaratorType
			{
				kIdentifier,
				kParenDeclarator,
				kDirectDeclaratorContinuation,
			};

			explicit CDirectDeclarator(CorePtr<CToken> &&identifier, const FileCoordinate &coord);
			explicit CDirectDeclarator(CorePtr<CDeclarator> &&decl, const FileCoordinate &coord);
			explicit CDirectDeclarator(CorePtr<CDirectDeclarator> &&directDecl, CorePtr<CDirectDeclaratorContinuation> &&continuation, const FileCoordinate &coord);

			const CToken *GetIdentifier() const;
			const CDeclarator *GetDeclarator() const;
			const CDirectDeclarator *GetNextDirectDeclarator() const;
			const CDirectDeclaratorContinuation *GetContinuation() const;

			DirectDeclaratorType GetDirectDeclType() const;

			const FileCoordinate &GetCoordinate() const;

		private:
			CorePtr<CToken> m_identifier;
			CorePtr<CDeclarator> m_decl;
			CorePtr<CDirectDeclarator> m_directDecl;
			CorePtr<CDirectDeclaratorContinuation> m_continuation;

			DirectDeclaratorType m_ddType;
			bool m_hasStatic;
			bool m_hasAsterisk;
			FileCoordinate m_coord;
		};

		class CDirectAbstractDeclaratorSuffix final : public CGrammarElement
		{
		public:
			enum class SuffixType
			{
				kSquareBracket,
				kAsterisk,
				kParamTypeList,
			};

			explicit CDirectAbstractDeclaratorSuffix();		// Only for asterisk case
			explicit CDirectAbstractDeclaratorSuffix(CorePtr<CTypeQualifierList> &&typeQualifierList, CorePtr<CExpression> &&assignmentExpr, bool hasStatic);
			explicit CDirectAbstractDeclaratorSuffix(CorePtr<CParameterTypeList> &&paramTypeList);

		private:
			CorePtr<CTypeQualifierList> m_typeQualifierList;
			CorePtr<CExpression> m_assignmentExpr;
			CorePtr<CParameterTypeList> m_parameterTypeList;

			SuffixType m_suffixType;
			bool m_hasStatic;
		};

		class CDirectAbstractDeclarator final : public CGrammarElement
		{
		public:
			explicit CDirectAbstractDeclarator(CorePtr<CAbstractDeclarator> &&optAbstractDecl, ArrayPtr<CorePtr<CDirectAbstractDeclaratorSuffix>> &&optSuffixes);

		private:
			CorePtr<CAbstractDeclarator> m_abstractDecl;
			ArrayPtr<CorePtr<CDirectAbstractDeclaratorSuffix>> m_suffixes;
		};

		class CPointer final : public CGrammarElement
		{
		public:
			struct IndirectionLevel
			{
				IndirectionLevel();
				IndirectionLevel(IndirectionLevel &&other);

				CorePtr<CTypeQualifierList> m_optTypeQualifierList;
			};

			explicit CPointer(ArrayPtr<IndirectionLevel> &&levels);

			ArrayView<const IndirectionLevel> GetIndirections() const;

		private:
			ArrayPtr<IndirectionLevel> m_levels;
		};

		class CDeclarator final : public CGrammarElement
		{
		public:
			CDeclarator(CorePtr<CPointer> &&optPointer, CorePtr<CDirectDeclarator> &&directDeclarator, const FileCoordinate &coord);

			const CPointer *GetOptPointer() const;
			const CDirectDeclarator *GetDirectDeclarator() const;
			const FileCoordinate &GetCoordinate() const;

		private:
			CorePtr<CPointer> m_optPointer;
			CorePtr<CDirectDeclarator> m_directDeclarator;
			FileCoordinate m_coord;
		};

		class CAbstractDeclarator final : public CGrammarElement
		{
		public:
			enum class AbstractDeclaratorType
			{
				kPointer,
				kPointerDirectAbstractDeclarator,
				kDirectAbstractDeclarator,
			};

			explicit CAbstractDeclarator(CorePtr<CPointer> &&pointer);
			explicit CAbstractDeclarator(CorePtr<CPointer> &&pointer, CorePtr<CDirectAbstractDeclarator> &&directDeclarator);
			explicit CAbstractDeclarator(CorePtr<CDirectAbstractDeclarator> &&directDeclarator);

		private:
			CorePtr<CPointer> m_optPointer;
			CorePtr<CDirectAbstractDeclarator> m_directDeclarator;

			AbstractDeclaratorType m_abstractDeclType;
		};

		class CEnumerator final : public CGrammarElement
		{
		public:
			explicit CEnumerator(CorePtr<CToken> &&enumerationConstant, CorePtr<CExpression> &&constantExpression);

		private:
			CorePtr<CToken> m_enumerationConstant;
			CorePtr<CExpression> m_constantExpression;
		};

		class CEnumeratorList final : public CGrammarElementListContainer<CEnumerator>
		{
		public:
			explicit CEnumeratorList(ArrayPtr<CorePtr<CEnumerator>> &&enumerators);
		};

		class CEnumSpecifier final : public CGrammarElement
		{
		public:
			explicit CEnumSpecifier(CorePtr<CToken> &&identifier, CorePtr<CEnumeratorList> &&enumeratorList, const FileCoordinate &coord);

			const CToken *GetIdentifier() const;
			const CEnumeratorList *GetEnumeratorList() const;
			const FileCoordinate &GetCoordinate() const;

		private:
			CorePtr<CToken> m_identifier;
			CorePtr<CEnumeratorList> m_enumeratorList;
			FileCoordinate m_coord;
		};

		class CSpecifierQualifierList final : public CGrammarElementListContainer<CGrammarElement>
		{
		public:
			explicit CSpecifierQualifierList(ArrayPtr<CorePtr<CGrammarElement>> &&children);

			// Children are of type kTypeSpecifier and kTypeQualifier
		};

		class CStructDeclarator final : public CGrammarElement
		{
		public:
			explicit CStructDeclarator(CorePtr<CDeclarator> &&declarator);
			explicit CStructDeclarator(CorePtr<CDeclarator> &&declarator, CorePtr<CExpression> &&constantExpression);
			explicit CStructDeclarator(CorePtr<CExpression> &&constantExpression);

		private:
			CorePtr<CDeclarator> m_declarator;
			CorePtr<CExpression> m_constantExpression;
		};

		class CStructDeclaratorList final : public CGrammarElementListContainer<CStructDeclarator>
		{
		public:
			explicit CStructDeclaratorList(ArrayPtr<CorePtr<CStructDeclarator>> &&children);
		};

		class CStructDeclaration final : public CGrammarElement
		{
		public:
			CStructDeclaration(CorePtr<CSpecifierQualifierList> &&specQualList, CorePtr<CStructDeclaratorList> &&structDeclaratorList);

		private:
			CorePtr<CSpecifierQualifierList> m_specQualList;
			CorePtr<CStructDeclaratorList> m_structDeclaratorList;
		};

		class CStructDeclarationList final : public CGrammarElementListContainer<CStructDeclaration>
		{
		public:
			CStructDeclarationList(ArrayPtr<CorePtr<CStructDeclaration>> &&declList);
		};

		class CInitDeclarator final : public CGrammarElement
		{
		public:
			CInitDeclarator(CorePtr<CDeclarator> &&declarator, CorePtr<CInitializer> &&initializer);

		private:
			CorePtr<CDeclarator> m_declarator;
			CorePtr<CInitializer> m_initializer;
		};

		class CInitDeclaratorList final : public CGrammarElementListContainer<CInitDeclarator>
		{
		public:
			CInitDeclaratorList(ArrayPtr<CorePtr<CInitDeclarator>> &&initDecls);
		};

		class CDeclaration final : public CGrammarElement
		{
		public:
			CDeclaration(CorePtr<CDeclarationSpecifiers> &&declSpecifiers, CorePtr<CInitDeclaratorList> &&optInitDeclaratorList);

		private:
			CorePtr<CDeclarationSpecifiers> m_declSpecifiers;
			CorePtr<CInitDeclaratorList> m_initDeclList;
		};

		class CDeclarationList final : public CGrammarElementListContainer<CDeclaration>
		{
		public:
			CDeclarationList(ArrayPtr<CorePtr<CDeclaration>> &&declList);
		};

		class CStructOrUnionSpecifier final : public CGrammarElement
		{
		public:
			CStructOrUnionSpecifier(CorePtr<CToken> &&optIdentifier, CorePtr<CStructDeclarationList> &&structDeclarationList, CAggregateType aggType, const FileCoordinate &coord);

			CAggregateType GetAggregateType() const;
			const CToken *GetIdentifier() const;
			const CStructDeclarationList *GetStructDeclarationList() const;
			const FileCoordinate &GetCoordinate() const;

		private:
			CAggregateType m_aggType;
			CorePtr<CToken> m_optIdentifier;
			CorePtr<CStructDeclarationList> m_structDeclarationList;
			FileCoordinate m_coord;
		};

		class CTypeName final : public CGrammarElement
		{
		public:
			explicit CTypeName(CorePtr<CSpecifierQualifierList> &&specQualList, CorePtr<CAbstractDeclarator> &&optAbsDecl);

		private:
			CorePtr<CSpecifierQualifierList> m_specQualList;
			CorePtr<CAbstractDeclarator> m_optAbsDecl;
		};

		class CArgumentExpressionList final : public CGrammarElementListContainer<CExpression>
		{
		public:
			CArgumentExpressionList(ArrayPtr<CorePtr<CExpression>> &&exprList);
		};

		class CInvokeExpression final : public CExpression
		{
		public:
			CInvokeExpression(CorePtr<CExpression> &&expr, CorePtr<CArgumentExpressionList> &&optArgList);

		private:
			CorePtr<CExpression> m_expr;
			CorePtr<CArgumentExpressionList> m_optArgList;
		};

		class CInitializer final : public CGrammarElement
		{
		public:
			enum class InitializerSubtype
			{
				kExpression,
				kInitializerList,
			};

			explicit CInitializer(CorePtr<CExpression> &&expr);
			explicit CInitializer(CorePtr<CInitializerList> &&initializerList);

		private:
			CorePtr<CExpression> m_expr;
			CorePtr<CInitializerList> m_initializerList;
			InitializerSubtype m_initSubtype;
		};

		class CDesignator final : public CGrammarElement
		{
		public:
			enum class DesignatorSubtype
			{
				kIndexExpression,
				kMemberIdentifier,
			};

			explicit CDesignator(CorePtr<CExpression> &&indexExpr);
			explicit CDesignator(CorePtr<CToken> &&identifier);

		private:
			CorePtr<CExpression> m_indexExpr;
			CorePtr<CToken> m_identifier;
			DesignatorSubtype m_designatorSubtype;
		};


		class CDesignatorList final : public CGrammarElementListContainer<CDesignator>
		{
		public:
			explicit CDesignatorList(ArrayPtr<CorePtr<CDesignator>> &&designators);
		};

		class CDesignation final : public CGrammarElement
		{
		public:
			explicit CDesignation(CorePtr<CDesignatorList> &&designationList);

		private:
			CorePtr<CDesignatorList> m_designationList;
		};

		class CDesignatableInitializer final : public CGrammarElement
		{
		public:
			explicit CDesignatableInitializer(CorePtr<CInitializer> &&expr);
			explicit CDesignatableInitializer(CorePtr<CDesignation> &&designation, CorePtr<CInitializer> &&expr);

		private:
			CorePtr<CDesignation> m_designation;
			CorePtr<CInitializer> m_expr;
		};

		class CInitializerList final : public CGrammarElementListContainer<CDesignatableInitializer>
		{
		public:
			CInitializerList(ArrayPtr<CorePtr<CDesignatableInitializer>> &&initializers);
		};

		class CTypedInitializerExpression final : public CExpression
		{
		public:
			explicit CTypedInitializerExpression(CorePtr<CTypeName> &&typeName, CorePtr<CInitializerList> &&initList);

		private:
			CorePtr<CTypeName> m_typeName;
			CorePtr<CInitializerList> m_initList;
		};

		class CTokenExpression final : public CExpression
		{
		public:
			enum class TokenExpressionSubtype
			{
				kIdentifier,
				kNumber,
				kCharSequence,
			};

			explicit CTokenExpression(const TokenStrView &token, TokenExpressionSubtype subtype);

		private:
			TokenExpressionSubtype m_subtype;
			TokenStrView m_token;
		};

		template<class T>
		CGrammarElementListContainer<T>::CGrammarElementListContainer(CGrammarElement::Subtype subtype, ArrayPtr<CorePtr<T>> &&children)
			: CGrammarElement(subtype)
			, m_children(std::move(children))
		{
		}

		template<class T>
		ArrayView<CorePtr<T>> CGrammarElementListContainer<T>::GetChildren() const
		{
			return m_children.View();
		}
	}
}
