#pragma once

#include "ArrayPtr.h"
#include "CAggregateType.h"
#include "CCompilerIncludeStackTracer.h"
#include "CoreObject.h"
#include "CompilerConfiguration.h"
#include "CGrammar.h"
#include "CLexer.h"
#include "CGlobalObjectInfo.h"
#include "HStorageClass.h"
#include "HashMap.h"
#include "Optional.h"

#include <cstdint>

#include "IErrorReporter.h"

namespace expanse
{
	struct Result;
	template<class T> struct ResultRV;

	namespace cc
	{
		class CDeclarationSpecifiers;
		class CDeclarator;
		class CEnumSpecifier;
		class CPreprocessorTraceInfo;
		class CScope;
		class CStructOrUnionSpecifier;
		class CToken;

		struct FileCoordinate;
		struct TokenStrView;
		struct IHAsmWriter;

		class CCompiler final : public CoreObject
		{
		public:
			CCompiler(IAllocator *alloc, IErrorReporter *errorReporter, ArrayPtr<uint8_t> &&contents, CPreprocessorTraceInfo *traceInfo, IHAsmWriter *asmWriter);

			Result Compile();

		private:
			struct TemporaryScope
			{
				TemporaryScope(CCompiler *compiler);
				~TemporaryScope();

				Result CreateScope();

			private:
				CorePtr<CScope> m_scope;
				CCompiler *m_compiler;
			};

			typedef CBinaryOperator (*BinOperatorResolver_t)(const TokenStrView &token);
			typedef ResultRV<bool> (CCompiler::*ExpressionParseFunc_t)(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative);

			ResultRV<bool> ParseTranslationUnit(FileCoordinate &coord);
			ResultRV<bool> ParseExternalDeclaration(FileCoordinate &coord);
			ResultRV<bool> ParseDeclSpecifiers(FileCoordinate &inOutCoordinate, CorePtr<CDeclarationSpecifiers> &outProduct, bool speculative);
			ResultRV<bool> ParseSingleDeclSpecifier(FileCoordinate &inOutCoordinate, CorePtr<CGrammarElement> &outProduct, bool speculative);
			ResultRV<bool> ParseDeclarator(FileCoordinate &inOutCoordinate, CorePtr<CDeclarator> &outProduct, bool speculative);
			ResultRV<bool> ParseAbstractDeclarator(FileCoordinate &inOutCoordinate, CorePtr<CAbstractDeclarator> &outProduct, bool speculative);
			ResultRV<bool> ParseDirectDeclaratorContinuation(FileCoordinate &inOutCoordinate, CorePtr<CDirectDeclaratorContinuation> &outProduct, bool speculative);
			ResultRV<bool> ParseDirectDeclarator(FileCoordinate &inOutCoordinate, CorePtr<CDirectDeclarator> &outProduct, bool speculative);
			ResultRV<bool> ParseDirectAbstractDeclarator(FileCoordinate &inOutCoordinate, CorePtr<CDirectAbstractDeclarator> &outProduct, bool speculative);
			ResultRV<bool> ParseDirectAbstractDeclaratorSuffix(FileCoordinate &inOutCoordinate, CorePtr<CDirectAbstractDeclaratorSuffix> &outProduct, bool speculative);
			ResultRV<bool> ParseEnumSpecifierAfterEnum(FileCoordinate &inOutCoordinate, CorePtr<CEnumSpecifier> &outProduct, bool speculative);
			ResultRV<bool> ParseEnumeratorList(FileCoordinate &inOutCoordinate, CorePtr<CEnumeratorList> &outProduct, bool speculative);
			ResultRV<bool> ParseEnumerator(FileCoordinate &inOutCoordinate, CorePtr<CEnumerator> &outProduct, bool speculative);
			ResultRV<bool> ParseStructSpecifierAfterStruct(FileCoordinate &inOutCoordinate, CorePtr<CStructOrUnionSpecifier> &outProduct, bool speculative);
			ResultRV<bool> ParseUnionSpecifierAfterUnion(FileCoordinate &inOutCoordinate, CorePtr<CStructOrUnionSpecifier> &outProduct, bool speculative);
			ResultRV<bool> ParseStructOrUnionSpecifierAfterDesignator(FileCoordinate &inOutCoordinate, CorePtr<CStructOrUnionSpecifier> &outProduct, bool speculative, CAggregateType structOrUnionType);
			ResultRV<bool> ParseInitDeclarator(FileCoordinate &inOutCoordinate, CorePtr<CInitDeclarator> &outProduct, bool speculative);
			ResultRV<bool> ParseInitDeclaratorList(FileCoordinate &inOutCoordinate, CorePtr<CInitDeclaratorList> &outProduct, bool speculative);
			ResultRV<bool> ParseDeclaration(FileCoordinate &inOutCoordinate, CorePtr<CDeclaration> &outProduct, bool speculative);
			ResultRV<bool> ParseDeclarationList(FileCoordinate &inOutCoordinate, CorePtr<CDeclarationList> &outProduct, bool speculative);
			ResultRV<bool> ParseStructDeclarationList(FileCoordinate &inOutCoordinate, CorePtr<CStructDeclarationList> &outProduct, bool speculative);
			ResultRV<bool> ParseStructDeclaration(FileCoordinate &inOutCoordinate, CorePtr<CStructDeclaration> &outProduct, bool speculative);
			ResultRV<bool> ParseSpecifierQualifierList(FileCoordinate &inOutCoordinate, CorePtr<CSpecifierQualifierList> &outProduct, bool speculative);
			ResultRV<bool> ParseStructDeclaratorList(FileCoordinate &inOutCoordinate, CorePtr<CStructDeclaratorList> &outProduct, bool speculative);
			ResultRV<bool> ParseStructDeclarator(FileCoordinate &inOutCoordinate, CorePtr<CStructDeclarator> &outProduct, bool speculative);
			ResultRV<bool> ParseTypeQualifier(FileCoordinate &inOutCoordinate, CorePtr<CToken> &outProduct, bool speculative);
			ResultRV<bool> ParseTypeSpecifier(FileCoordinate &inOutCoordinate, CorePtr<CGrammarElement> &outProduct, bool speculative);
			ResultRV<bool> ParsePointer(FileCoordinate &inOutCoordinate, CorePtr<CPointer> &outProduct, bool speculative);
			ResultRV<bool> ParseIdentifier(FileCoordinate &inOutCoordinate, CorePtr<CToken> &outProduct, bool speculative);
			ResultRV<bool> ParseTypeQualifierList(FileCoordinate &inOutCoordinate, CorePtr<CTypeQualifierList> &outProduct, bool speculative);
			ResultRV<bool> ParseParameterTypeList(FileCoordinate &inOutCoordinate, CorePtr<CParameterTypeList> &outProduct, bool speculative);
			ResultRV<bool> ParseParameterDeclaration(FileCoordinate &inOutCoordinate, CorePtr<CParameterDeclaration> &outProduct, bool speculative);
			ResultRV<bool> ParseIdentifierList(FileCoordinate &inOutCoordinate, CorePtr<CIdentifierList> &outProduct, bool speculative);
			ResultRV<bool> ParseTypeName(FileCoordinate &inOutCoordinate, CorePtr<CTypeName> &outProduct, bool speculative);

			ResultRV<bool> ParseConstantExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative);
			ResultRV<bool> ParseConditionalExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative);
			ResultRV<bool> ParseAssignmentExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative);
			ResultRV<bool> ParseLogicalOrExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative);
			ResultRV<bool> ParseLogicalAndExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative);
			ResultRV<bool> ParseInclusiveOrExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative);
			ResultRV<bool> ParseExclusiveOrExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative);
			ResultRV<bool> ParseAndExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative);
			ResultRV<bool> ParseEqualityExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative);
			ResultRV<bool> ParseRelationalExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative);
			ResultRV<bool> ParseShiftExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative);
			ResultRV<bool> ParseAdditiveExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative);
			ResultRV<bool> ParseMultiplicativeExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative);
			ResultRV<bool> ParseCastExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative);
			ResultRV<bool> ParseExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative);
			ResultRV<bool> ParseUnaryExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative);
			ResultRV<bool> ParsePostfixExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative);
			ResultRV<bool> ParsePostfixExpressionLeftSide(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative);
			ResultRV<bool> ParsePrimaryExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative);
			ResultRV<bool> ParsePostfixExpressionInitializerListAfterTypeName(FileCoordinate &inOutCoordinate, CorePtr<CInitializerList> &outProduct, bool speculative);
			ResultRV<bool> ParseInitializerList(FileCoordinate &inOutCoordinate, CorePtr<CInitializerList> &outProduct, bool speculative);
			ResultRV<bool> ParseDesignatableInitializer(FileCoordinate &inOutCoordinate, CorePtr<CDesignatableInitializer> &outProduct, bool speculative);
			ResultRV<bool> ParseDesignation(FileCoordinate &inOutCoordinate, CorePtr<CDesignation> &outProduct, bool speculative);
			ResultRV<bool> ParseDesignatorList(FileCoordinate &inOutCoordinate, CorePtr<CDesignatorList> &outProduct, bool speculative);
			ResultRV<bool> ParseDesignator(FileCoordinate &inOutCoordinate, CorePtr<CDesignator> &outProduct, bool speculative);
			ResultRV<bool> ParseInitializer(FileCoordinate &inOutCoordinate, CorePtr<CInitializer> &outProduct, bool speculative);
			ResultRV<bool> ParseArgumentExpressionList(FileCoordinate &inOutCoordinate, CorePtr<CArgumentExpressionList> &outProduct, bool speculative);
			ResultRV<bool> ParseTypedefName(FileCoordinate &inOutCoordinate, CorePtr<CToken> &outProduct, bool speculative);

			ResultRV<bool> DynamicParseLTRBinaryExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative, BinOperatorResolver_t opResolverFunc, ExpressionParseFunc_t nextPriorityFunc);

			static CBinaryOperator ResolveLogicalOrOperator(const TokenStrView &token);
			static CBinaryOperator ResolveLogicalAndOperator(const TokenStrView &token);
			static CBinaryOperator ResolveInclusiveOrOperator(const TokenStrView &token);
			static CBinaryOperator ResolveExclusiveOrOperator(const TokenStrView &token);
			static CBinaryOperator ResolveAndOperator(const TokenStrView &token);
			static CBinaryOperator ResolveEqualityOperator(const TokenStrView &token);
			static CBinaryOperator ResolveRelationalOperator(const TokenStrView &token);
			static CBinaryOperator ResolveShiftOperator(const TokenStrView &token);
			static CBinaryOperator ResolveAdditiveOperator(const TokenStrView &token);
			static CBinaryOperator ResolveMultiplicativeOperator(const TokenStrView &token);
			static CBinaryOperator ResolveCommaOperator(const TokenStrView &token);

			Result ParseAndCompileInitializerForDeclarator(CDeclarationSpecifiers *declSpecifiers, CDeclarator *declarator, FileCoordinate &inOutCoordinate);
			Result ParseAndCompileInitDeclaratorListEndingInSemi(CDeclarationSpecifiers *declSpecifiers, FileCoordinate &inOutCoordinate);
			Result CompileFunctionDefinitionAfterDeclarator(CDeclarationSpecifiers *declSpecifiers, CDeclarator *declarator, FileCoordinate &inOutCoordinate);

			ResultRV<HTypeQualified> ResolveTypeDefName(const CToken &tokenElement);

			Result CompileAggregateDefinition(HAggregateDecl *aggDecl, const CStructDeclarationList *declList);
			Result CompileEnumDefinition(HEnumDecl *enumDecl, const CEnumeratorList *enumList);
			ResultRV<HAggregateDecl*> DeclareStructOrUnionInCurrentScope(const TokenStrView &name, CAggregateType aggType);
			ResultRV<HEnumDecl*> DeclareEnumInCurrentScope(const TokenStrView &name);
			ResultRV<HTypeUnqualified> ResolveStructOrUnionSpecifier(const CStructOrUnionSpecifier &souSpecifierElement);
			ResultRV<HTypeUnqualified> ResolveEnumSpecifier(const CEnumSpecifier &souSpecifierElement);

			Result ResolveDeclSpecifiers(const CDeclarationSpecifiers *declSpecifiers, HTypeQualifiers &outQualifiers, bool &outIsInline, Optional<HStorageClass> &outStorageClass, HTypeUnqualified &outUnqualifiedType);
			Result ResolveDeclarator(const CDeclarationSpecifiers *declSpecifiers, const CDeclarator *declarator, Optional<HStorageClass> &outStorageClass, TokenStrView &outName, HTypeQualified &outDeclType);
			ResultRV<HTypeUnqualified> ResolvePointer(const CPointer &pointer, const HTypeQualified &innerType);
			static HTypeQualifiers ResolveQualifiers(const CTypeQualifierList &qualList);
			Result CommitDeclarator(CDeclarationSpecifiers *declSpecifiers, CDeclarator *declarator);

			bool GetToken(TokenStrView &token, FileCoordinate &coord, CLexer::TokenType &tokenType);

			void ReportCompileError(CompilationErrorCode errorCode, const FileCoordinate &coord);
			void ReportCompileWarning(CompilationWarningCode warningCode, const FileCoordinate &coord);

			ResultRV<HTypeUnqualifiedInterned*> InternType(const HTypeUnqualified &t);

			static bool IsKeyword(TokenStrView &token);

			Result InitGlobalScope();

			ArrayPtr<uint8_t> m_contents;
			CPreprocessorTraceInfo *m_traceInfo;

			CScope *m_currentScope;
			CorePtr<CScope> m_globalScope;

			Optional<FileCoordinate> m_lastParseCoord;
			TokenStrView m_lastToken;
			FileCoordinate m_lastEndCoord;
			CLexer::TokenType m_lastTokenType;

			HashMap<HTypeUnqualified, CorePtr<HTypeUnqualifiedInterned>> m_globalInternedTypes;
			HashMap<HTypeUnqualified, CorePtr<HTypeUnqualifiedInterned>> m_tempInternedTypes;

			Vector<CorePtr<HAggregateDecl>> m_globalInternedAggregates;
			Vector<CorePtr<HAggregateDecl>> m_tempInternedAggregates;

			Vector<CorePtr<HEnumDecl>> m_globalInternedEnums;
			Vector<CorePtr<HEnumDecl>> m_tempInternedEnums;
			Vector<CGlobalObjectInfo> m_globalObjects;
			HashMap<TokenStrView, size_t> m_externalLinkageLookup;

			CCompilerIncludeStackTracer m_tracer;
			IErrorReporter *m_errorReporter;

			IHAsmWriter *m_asmWriter;

			CompilerConfiguration m_config;
		};
	}
}
