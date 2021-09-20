#include "CGrammar.h"

#pragma once

#include "ArrayPtr.h"
#include "CoreObject.h"
#include "CorePtr.h"
#include "FileCoordinate.h"
#include "Vector.h"
#include "PPTokenStr.h"

namespace expanse
{
	namespace cc
	{
		CGrammarElement::CGrammarElement(Subtype subtype)
			: m_subtype(subtype)
		{
		}

		CGrammarElement::Subtype CGrammarElement::GetSubtype() const
		{
			return m_subtype;
		}

		CDeclarationSpecifiers::CDeclarationSpecifiers(ArrayPtr<CorePtr<CGrammarElement>> &&children)
			: CGrammarElementListContainer<CGrammarElement>(Subtype::kDeclarationSpecifiers, std::move(children))
		{
		}

		CToken::CToken(Subtype subtype, const TokenStrView &token, const FileCoordinate &coordinate)
			: CGrammarElement(subtype)
			, m_token(token)
			, m_coord(coordinate)
		{
		}

		const TokenStrView &CToken::GetToken() const
		{
			return m_token;
		}

		const FileCoordinate &CToken::GetCoordinate() const
		{
			return m_coord;
		}

		CTypeQualifierList::CTypeQualifierList(ArrayPtr<CorePtr<CToken>> &&children)
			: CGrammarElementListContainer<CToken>(Subtype::kTypeQualifierList, std::move(children))
		{
		}

		CExpression::CExpression(ExpressionSubtype exprSubtype)
			: CGrammarElement(Subtype::kExpression)
			, m_exprType(exprSubtype)
		{
		}

		CExpression::ExpressionSubtype CExpression::GetExprSubtype() const
		{
			return m_exprType;
		}


		CMemberExpression::CMemberExpression(CorePtr<CExpression> &&leftSide, CorePtr<CToken> &&identifier, bool dereference)
			: CExpression(ExpressionSubtype::kMember)
			, m_leftSide(std::move(leftSide))
			, m_identifier(std::move(identifier))
			, m_dereference(dereference)
		{
		}

		CSizeOfTypeExpression::CSizeOfTypeExpression(CorePtr<CTypeName> &&typeName)
			: CExpression(ExpressionSubtype::kSizeOfType)
			, m_typeName(std::move(typeName))
		{
		}

		CUnaryExpression::CUnaryExpression(CorePtr<CExpression> &&expr, CUnaryOperator unaryOp)
			: CExpression(ExpressionSubtype::kUnary)
			, m_expr(std::move(expr))
			, m_unaryOp(unaryOp)
		{
		}

		CBinaryExpression::CBinaryExpression(CorePtr<CExpression> &&leftExpr, CorePtr<CExpression> &&rightExpr, CBinaryOperator binOp)
			: CExpression(ExpressionSubtype::kBinary)
			, m_leftExpr(std::move(leftExpr))
			, m_rightExpr(std::move(rightExpr))
			, m_binOp(binOp)
		{
		}

		CTernaryExpression::CTernaryExpression(CorePtr<CExpression> &&conditionExpr, CorePtr<CExpression> &&trueExpr, CorePtr<CExpression> &&falseExpr)
			: CExpression(ExpressionSubtype::kTernary)
			, m_conditionExpr(std::move(conditionExpr))
			, m_trueExpr(std::move(trueExpr))
			, m_falseExpr(std::move(falseExpr))
		{
		}

		CCastExpression::CCastExpression(CorePtr<CTypeName> &&typeName, CorePtr<CExpression> &&rightSideExpr)
			: CExpression(ExpressionSubtype::kCast)
			, m_typeName(std::move(typeName))
			, m_rightSideExpr(std::move(rightSideExpr))
		{
		}

		CParameterDeclaration::CParameterDeclaration(CorePtr<CDeclarationSpecifiers> &&declSpecifiers)
			: CGrammarElement(Subtype::kParameterDeclaration)
			, m_declSpecifiers(std::move(declSpecifiers))
		{
		}

		CParameterDeclaration::CParameterDeclaration(CorePtr<CDeclarationSpecifiers> &&declSpecifiers, CorePtr<CDeclarator> &&declarator)
			: CGrammarElement(Subtype::kParameterDeclaration)
			, m_declSpecifiers(std::move(declSpecifiers))
			, m_declarator(std::move(declarator))
		{
		}

		CParameterDeclaration::CParameterDeclaration(CorePtr<CDeclarationSpecifiers> &&declSpecifiers, CorePtr<CAbstractDeclarator> &&absDeclarator)
			: CGrammarElement(Subtype::kParameterDeclaration)
			, m_declSpecifiers(std::move(declSpecifiers))
			, m_absDeclarator(std::move(absDeclarator))
		{
		}

		CParameterTypeList::CParameterTypeList(ArrayPtr<CorePtr<CParameterDeclaration>> &&paramDecls, bool hasVarArgs)
			: CGrammarElementListContainer<CParameterDeclaration>(Subtype::kParameterTypeList, std::move(paramDecls))
			, m_hasVarArgs(hasVarArgs)
		{
		}

		CIdentifierList::CIdentifierList(ArrayPtr<CorePtr<CToken>> &&identifiers)
			: CGrammarElementListContainer<CToken>(Subtype::kIdentifierList, std::move(identifiers))
		{
		}

		CDirectDeclaratorContinuation::CDirectDeclaratorContinuation(CorePtr<CTypeQualifierList> &&typeQualifierList, CorePtr<CExpression> &&assignmentExpr, bool hasStatic, bool hasAsterisk, const FileCoordinate &coord)
			: CGrammarElement(Subtype::kDirectDeclaratorContinuation)
			, m_typeQualifierList(std::move(typeQualifierList))
			, m_assignmentExpr(std::move(assignmentExpr))
			, m_hasStatic(hasStatic)
			, m_hasAsterisk(hasAsterisk)
			, m_coord(coord)
			, m_contType(ContinuationType::kSquareBracket)
		{
		}

		CDirectDeclaratorContinuation::CDirectDeclaratorContinuation(CorePtr<CParameterTypeList> &&parameterTypeList, const FileCoordinate &coord)
			: CGrammarElement(Subtype::kDirectDeclaratorContinuation)
			, m_parameterTypeList(std::move(parameterTypeList))
			, m_coord(coord)
			, m_contType(ContinuationType::kParamTypeList)
		{
		}

		CDirectDeclaratorContinuation::CDirectDeclaratorContinuation(CorePtr<CIdentifierList> &&identifierList, const FileCoordinate &coord)
			: CGrammarElement(Subtype::kDirectDeclaratorContinuation)
			, m_identifierList(std::move(identifierList))
			, m_coord(coord)
			, m_contType(ContinuationType::kIdentifierList)
		{
		}

		const CTypeQualifierList *CDirectDeclaratorContinuation::GetTypeQualifierList() const
		{
			return m_typeQualifierList;
		}

		const CExpression *CDirectDeclaratorContinuation::GetAssignmentExpr() const
		{
			return m_assignmentExpr;
		}

		const CParameterTypeList *CDirectDeclaratorContinuation::GetParameterTypeList() const
		{
			return m_parameterTypeList;
		}

		const CIdentifierList *CDirectDeclaratorContinuation::GetIdentifierList() const
		{
			return m_identifierList;
		}

		CDirectDeclaratorContinuation::ContinuationType CDirectDeclaratorContinuation::GetContinuationType() const
		{
			return m_contType;
		}

		bool CDirectDeclaratorContinuation::HasStatic() const
		{
			return m_hasStatic;
		}

		bool CDirectDeclaratorContinuation::HasAsterisk() const
		{
			return m_hasAsterisk;
		}

		const FileCoordinate &CDirectDeclaratorContinuation::GetCoordinate() const
		{
			return m_coord;
		}

		CDirectDeclarator::CDirectDeclarator(CorePtr<CToken> &&identifier, const FileCoordinate &coord)
			: CGrammarElement(Subtype::kDirectDeclarator)
			, m_identifier(std::move(identifier))
			, m_ddType(DirectDeclaratorType::kIdentifier)
			, m_hasStatic(false)
			, m_hasAsterisk(false)
			, m_coord(coord)
		{
		}

		CDirectDeclarator::CDirectDeclarator(CorePtr<CDeclarator> &&decl, const FileCoordinate &coord)
			: CGrammarElement(Subtype::kDirectDeclarator)
			, m_decl(std::move(decl))
			, m_ddType(DirectDeclaratorType::kParenDeclarator)
			, m_hasStatic(false)
			, m_hasAsterisk(false)
			, m_coord(coord)
		{
		}

		CDirectDeclarator::CDirectDeclarator(CorePtr<CDirectDeclarator> &&directDecl, CorePtr<CDirectDeclaratorContinuation> &&continuation, const FileCoordinate &coord)
			: CGrammarElement(Subtype::kDirectDeclarator)
			, m_directDecl(std::move(directDecl))
			, m_continuation(std::move(continuation))
			, m_coord(coord)
			, m_ddType(DirectDeclaratorType::kDirectDeclaratorContinuation)
		{
		}

		const CToken *CDirectDeclarator::GetIdentifier() const
		{
			return m_identifier;
		}

		const CDeclarator *CDirectDeclarator::GetDeclarator() const
		{
			return m_decl;
		}

		const CDirectDeclarator *CDirectDeclarator::GetNextDirectDeclarator() const
		{
			return m_directDecl;
		}

		const CDirectDeclaratorContinuation *CDirectDeclarator::GetContinuation() const
		{
			return m_continuation;
		}

		CDirectDeclarator::DirectDeclaratorType CDirectDeclarator::GetDirectDeclType() const
		{
			return m_ddType;
		}

		const FileCoordinate &CDirectDeclarator::GetCoordinate() const
		{
			return m_coord;
		}

		CDirectAbstractDeclaratorSuffix::CDirectAbstractDeclaratorSuffix()
			: CGrammarElement(Subtype::kDirectAbstractDeclaratorSuffix)
			, m_suffixType(SuffixType::kAsterisk)
			, m_hasStatic(false)
		{
		}
		
		CDirectAbstractDeclaratorSuffix::CDirectAbstractDeclaratorSuffix(CorePtr<CTypeQualifierList> &&typeQualifierList, CorePtr<CExpression> &&assignmentExpr, bool hasStatic)
			: CGrammarElement(Subtype::kDirectAbstractDeclaratorSuffix)
			, m_typeQualifierList(std::move(typeQualifierList))
			, m_assignmentExpr(std::move(assignmentExpr))
			, m_suffixType(SuffixType::kSquareBracket)
			, m_hasStatic(hasStatic)
		{
		}

		CDirectAbstractDeclaratorSuffix::CDirectAbstractDeclaratorSuffix(CorePtr<CParameterTypeList> &&paramTypeList)
			: CGrammarElement(Subtype::kDirectAbstractDeclaratorSuffix)
			, m_parameterTypeList(std::move(paramTypeList))
			, m_suffixType(SuffixType::kParamTypeList)
			, m_hasStatic(false)
		{
		}


		CDirectAbstractDeclarator::CDirectAbstractDeclarator(CorePtr<CAbstractDeclarator> &&optAbstractDecl, ArrayPtr<CorePtr<CDirectAbstractDeclaratorSuffix>> &&optSuffixes)
			: CGrammarElement(Subtype::kDirectAbstractDeclarator)
			, m_abstractDecl(std::move(optAbstractDecl))
			, m_suffixes(std::move(optSuffixes))
		{
		}

		CPointer::IndirectionLevel::IndirectionLevel()
		{
		}

		CPointer::IndirectionLevel::IndirectionLevel(IndirectionLevel &&other)
			: m_optTypeQualifierList(std::move(other.m_optTypeQualifierList))
		{
		}

		CPointer::CPointer(ArrayPtr<IndirectionLevel> &&levels)
			: CGrammarElement(Subtype::kPointer)
			, m_levels(std::move(levels))
		{
		}

		ArrayView<const CPointer::IndirectionLevel> CPointer::GetIndirections() const
		{
			return m_levels.ConstView();
		}

		CDeclarator::CDeclarator(CorePtr<CPointer> &&optPointer, CorePtr<CDirectDeclarator> &&directDeclarator, const FileCoordinate &coord)
			: CGrammarElement(Subtype::kDeclarator)
			, m_optPointer(std::move(optPointer))
			, m_directDeclarator(std::move(directDeclarator))
			, m_coord(coord)
		{
		}

		const CPointer *CDeclarator::GetOptPointer() const
		{
			return m_optPointer;
		}

		const CDirectDeclarator *CDeclarator::GetDirectDeclarator() const
		{
			return m_directDeclarator;
		}

		const FileCoordinate &CDeclarator::GetCoordinate() const
		{
			return m_coord;
		}


		CAbstractDeclarator::CAbstractDeclarator(CorePtr<CPointer> &&pointer)
			: CGrammarElement(Subtype::kAbstractDeclarator)
			, m_optPointer(std::move(pointer))
			, m_abstractDeclType(AbstractDeclaratorType::kPointer)
		{
		}

		CAbstractDeclarator::CAbstractDeclarator(CorePtr<CPointer> &&pointer, CorePtr<CDirectAbstractDeclarator> &&directDeclarator)
			: CGrammarElement(Subtype::kAbstractDeclarator)
			, m_optPointer(std::move(pointer))
			, m_directDeclarator(std::move(directDeclarator))
			, m_abstractDeclType(AbstractDeclaratorType::kPointerDirectAbstractDeclarator)
		{
		}

		CAbstractDeclarator::CAbstractDeclarator(CorePtr<CDirectAbstractDeclarator> &&directDeclarator)
			: CGrammarElement(Subtype::kAbstractDeclarator)
			, m_directDeclarator(std::move(directDeclarator))
			, m_abstractDeclType(AbstractDeclaratorType::kDirectAbstractDeclarator)
		{
		}

		CEnumerator::CEnumerator(CorePtr<CToken> &&enumerationConstant, CorePtr<CExpression> &&constantExpression)
			: CGrammarElement(Subtype::kEnumerator)
			, m_enumerationConstant(std::move(enumerationConstant))
			, m_constantExpression(std::move(constantExpression))
		{
		}


		CEnumeratorList::CEnumeratorList(ArrayPtr<CorePtr<CEnumerator>> &&enumerators)
			: CGrammarElementListContainer<CEnumerator>(Subtype::kEnumeratorList, std::move(enumerators))
		{
		}

		CEnumSpecifier::CEnumSpecifier(CorePtr<CToken> &&identifier, CorePtr<CEnumeratorList> &&enumeratorList, const FileCoordinate &coord)
			: CGrammarElement(Subtype::kEnumSpecifier)
			, m_identifier(std::move(identifier))
			, m_enumeratorList(std::move(enumeratorList))
			, m_coord(coord)
		{
		}

		const CToken *CEnumSpecifier::GetIdentifier() const
		{
			return m_identifier;
		}

		const CEnumeratorList *CEnumSpecifier::GetEnumeratorList() const
		{
			return m_enumeratorList;
		}

		const FileCoordinate &CEnumSpecifier::GetCoordinate() const
		{
			return m_coord;
		}


		CSpecifierQualifierList::CSpecifierQualifierList(ArrayPtr<CorePtr<CGrammarElement>> &&children)
			: CGrammarElementListContainer<CGrammarElement>(Subtype::kSpecifierQualifierList, std::move(children))
		{
		}

		CStructDeclarator::CStructDeclarator(CorePtr<CDeclarator> &&declarator)
			: CGrammarElement(Subtype::kStructDeclarator)
			, m_declarator(std::move(declarator))
		{
		}

		CStructDeclarator::CStructDeclarator(CorePtr<CDeclarator> &&declarator, CorePtr<CExpression> &&constantExpression)
			: CGrammarElement(Subtype::kStructDeclarator)
			, m_declarator(std::move(declarator))
			, m_constantExpression(std::move(constantExpression))
		{
		}

		CStructDeclarator::CStructDeclarator(CorePtr<CExpression> &&constantExpression)
			: CGrammarElement(Subtype::kStructDeclarator)
			, m_constantExpression(std::move(constantExpression))
		{
		}

		CStructDeclaratorList::CStructDeclaratorList(ArrayPtr<CorePtr<CStructDeclarator>> &&children)
			: CGrammarElementListContainer<CStructDeclarator>(Subtype::kStructDeclaratorList, std::move(children))
		{
		}

		CStructDeclaration::CStructDeclaration(CorePtr<CSpecifierQualifierList> &&specQualList, CorePtr<CStructDeclaratorList> &&structDeclaratorList)
			: CGrammarElement(Subtype::kStructDeclaration)
			, m_specQualList(std::move(specQualList))
			, m_structDeclaratorList(std::move(structDeclaratorList))
		{
		}

		CStructDeclarationList::CStructDeclarationList(ArrayPtr<CorePtr<CStructDeclaration>> &&declList)
			: CGrammarElementListContainer<CStructDeclaration>(Subtype::kStructDeclarationList, std::move(declList))
		{
		}

		CInitDeclarator::CInitDeclarator(CorePtr<CDeclarator> &&declarator, CorePtr<CInitializer> &&optInitializer)
			: CGrammarElement(Subtype::kInitDeclarator)
			, m_declarator(std::move(declarator))
			, m_initializer(std::move(optInitializer))
		{
		}

		CInitDeclaratorList::CInitDeclaratorList(ArrayPtr<CorePtr<CInitDeclarator>> &&initDecls)
			: CGrammarElementListContainer<CInitDeclarator>(Subtype::kInitDeclaratorList, std::move(initDecls))
		{
		}

		CDeclaration::CDeclaration(CorePtr<CDeclarationSpecifiers> &&declSpecifiers, CorePtr<CInitDeclaratorList> &&optInitDeclaratorList)
			: CGrammarElement(Subtype::kDeclaration)
			, m_declSpecifiers(std::move(declSpecifiers))
			, m_initDeclList(std::move(optInitDeclaratorList))
		{
		}

		CDeclarationList::CDeclarationList(ArrayPtr<CorePtr<CDeclaration>> &&declList)
			: CGrammarElementListContainer<CDeclaration>(Subtype::kDeclarationList, std::move(declList))
		{
		}

		CStructOrUnionSpecifier::CStructOrUnionSpecifier(CorePtr<CToken> &&optIdentifier, CorePtr<CStructDeclarationList> &&structDeclarationList, CAggregateType aggType, const FileCoordinate &coord)
			: CGrammarElement(Subtype::kStructOrUnionSpecifier)
			, m_aggType(aggType)
			, m_optIdentifier(std::move(optIdentifier))
			, m_structDeclarationList(std::move(structDeclarationList))
			, m_coord(coord)
		{
		}

		CAggregateType CStructOrUnionSpecifier::GetAggregateType() const
		{
			return m_aggType;
		}

		const CToken *CStructOrUnionSpecifier::GetIdentifier() const
		{
			return m_optIdentifier;
		}

		const CStructDeclarationList *CStructOrUnionSpecifier::GetStructDeclarationList() const
		{
			return m_structDeclarationList;
		}

		const FileCoordinate &CStructOrUnionSpecifier::GetCoordinate() const
		{
			return m_coord;
		}

		CTypeName::CTypeName(CorePtr<CSpecifierQualifierList> &&specQualList, CorePtr<CAbstractDeclarator> &&optAbsDecl)
			: CGrammarElement(Subtype::kTypeName)
			, m_specQualList(std::move(specQualList))
			, m_optAbsDecl(std::move(optAbsDecl))
		{
		}

		CArgumentExpressionList::CArgumentExpressionList(ArrayPtr<CorePtr<CExpression>> &&exprList)
			: CGrammarElementListContainer<CExpression>(Subtype::kArgumentExpressionList, std::move(exprList))
		{
		}

		CInvokeExpression::CInvokeExpression(CorePtr<CExpression> &&expr, CorePtr<CArgumentExpressionList> &&optArgList)
			: CExpression(ExpressionSubtype::kInvoke)
			, m_expr(std::move(expr))
			, m_optArgList(std::move(optArgList))
		{
		}



		CInitializer::CInitializer(CorePtr<CExpression> &&expr)
			: CGrammarElement(Subtype::kInitializer)
			, m_expr(std::move(expr))
			, m_initSubtype(InitializerSubtype::kExpression)
		{
		}
		
		CInitializer::CInitializer(CorePtr<CInitializerList> &&initializerList)
			: CGrammarElement(Subtype::kInitializer)
			, m_initializerList(std::move(initializerList))
			, m_initSubtype(InitializerSubtype::kInitializerList)
		{
		}

		CDesignator::CDesignator(CorePtr<CExpression> &&indexExpr)
			: CGrammarElement(Subtype::kDesignator)
			, m_indexExpr(std::move(indexExpr))
			, m_designatorSubtype(DesignatorSubtype::kIndexExpression)
		{
		}

		CDesignator::CDesignator(CorePtr<CToken> &&identifier)
			: CGrammarElement(Subtype::kDesignator)
			, m_identifier(std::move(identifier))
			, m_designatorSubtype(DesignatorSubtype::kMemberIdentifier)
		{
		}


		CDesignatorList::CDesignatorList(ArrayPtr<CorePtr<CDesignator>> &&designators)
			: CGrammarElementListContainer<CDesignator>(Subtype::kDesignatorList, std::move(designators))
		{
		}

		CDesignation::CDesignation(CorePtr<CDesignatorList> &&designationList)
			: CGrammarElement(Subtype::kDesignation)
			, m_designationList(std::move(designationList))
		{
		}

		CDesignatableInitializer::CDesignatableInitializer(CorePtr<CInitializer> &&expr)
			: CGrammarElement(Subtype::kDesignatableInitializer)
			, m_expr(std::move(expr))
		{
		}

		CDesignatableInitializer::CDesignatableInitializer(CorePtr<CDesignation> &&designation, CorePtr<CInitializer> &&expr)
			: CGrammarElement(Subtype::kDesignatableInitializer)
			, m_designation(std::move(designation))
			, m_expr(std::move(expr))
		{
		}

		CInitializerList::CInitializerList(ArrayPtr<CorePtr<CDesignatableInitializer>> &&initializers)
			: CGrammarElementListContainer<CDesignatableInitializer>(Subtype::kInitializerList, std::move(initializers))
		{
		}

		CTypedInitializerExpression::CTypedInitializerExpression(CorePtr<CTypeName> &&typeName, CorePtr<CInitializerList> &&initList)
			: CExpression(ExpressionSubtype::kTypedInitializer)
			, m_typeName(std::move(typeName))
			, m_initList(std::move(initList))
		{
		}

		CTokenExpression::CTokenExpression(const TokenStrView &token, TokenExpressionSubtype subtype)
			: CExpression(ExpressionSubtype::kTokenExpression)
			, m_token(token)
			, m_subtype(subtype)
		{
		}
	}
}
