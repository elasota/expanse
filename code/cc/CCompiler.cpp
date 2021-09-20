#include "CCompiler.h"

#include "CGrammar.h"
#include "CLexer.h"
#include "CScope.h"
#include "FileCoordinate.h"
#include "HStorageClass.h"
#include "IHAsmWriter.h"
#include "Result.h"
#include "ResultRV.h"
#include "Optional.h"
#include "PPTokenStr.h"

// Speculative parse attempts to parse an optional construct.
#define SPECULATIVE_PARSE(type, name, func)	\
	CorePtr<type> name;\
	do {\
		\
		CorePtr<type> temp; \
		CHECK_RV(bool, parsedOK, (this->func)(coord, temp, true)); \
		if (parsedOK)\
			name = std::move(temp);\
	} while(false)

// Require parse attempts to parse a required construct.  If it fails, the function returns false.
#define REQUIRE_PARSE(type, name, func)	\
	CorePtr<type> name;\
	do {\
		\
		CorePtr<type> temp; \
		CHECK_RV(bool, parsedOK, (this->func)(coord, temp, speculative)); \
		if (!parsedOK)\
			return false;\
		EXP_ASSERT(speculative == true || temp != nullptr);\
		name = std::move(temp);\
	} while(false)

#define REQUIRE_TOKEN(name)	\
	TokenStrView name;\
	do\
	{\
		CLexer::TokenType tokenType;\
		if (!GetToken(name, coord, tokenType))\
		{\
			if (speculative)\
				return false;\
			ReportCompileError(CompilationErrorCode::kUnexpectedEndOfFile, coord);\
			return ErrorCode::kOperationFailed;\
		}\
	} while(false)

#define PEEK_TOKEN(name, speculativeCoord)	\
	TokenStrView name;\
	FileCoordinate speculativeCoord = coord;\
	do\
	{\
		CLexer::TokenType tokenType;\
		if (!GetToken(name, speculativeCoord, tokenType))\
		{\
			if (speculative)\
				return false;\
			ReportCompileError(CompilationErrorCode::kUnexpectedEndOfFile, coord);\
			return ErrorCode::kOperationFailed;\
		}\
	} while(false)

#define PEEK_TOKEN_WITH_TYPE(name, speculativeCoord, tokenType)	\
	TokenStrView name;\
	FileCoordinate speculativeCoord = coord;\
	CLexer::TokenType tokenType;\
	do\
	{\
		if (!GetToken(name, speculativeCoord, tokenType))\
		{\
			if (speculative)\
				return false;\
			ReportCompileError(CompilationErrorCode::kUnexpectedEndOfFile, coord);\
			return ErrorCode::kOperationFailed;\
		}\
	} while(false)


#define EXPECT_TOKEN(str)	\
	do\
	{\
		TokenStrView tempToken;\
		FileCoordinate prevCoord = coord;\
		CLexer::TokenType tokenType;\
		if (!GetToken(tempToken, coord, tokenType))\
		{\
			if (speculative)\
				return false;\
			ReportCompileError(CompilationErrorCode::kUnexpectedEndOfFile, coord);\
			return ErrorCode::kOperationFailed;\
		}\
		if (!tempToken.IsString(str))\
		{\
			if (speculative)\
				return false;\
			ReportCompileError(CompilationErrorCode::kUnexpectedToken, prevCoord);\
			return ErrorCode::kOperationFailed;\
		}\
	} while(false)

namespace expanse
{
	namespace cc
	{
		CCompiler::CCompiler(IAllocator *alloc, IErrorReporter *errorReporter, ArrayPtr<uint8_t> &&contents, CPreprocessorTraceInfo *traceInfo, IHAsmWriter *asmWriter)
			: m_contents(std::move(contents))
			, m_traceInfo(traceInfo)
			, m_currentScope(nullptr)
			, m_errorReporter(errorReporter)
			, m_asmWriter(asmWriter)
			, m_globalInternedTypes(*alloc)
			, m_tempInternedTypes(*alloc)
			, m_globalInternedAggregates(alloc)
			, m_tempInternedAggregates(alloc)
			, m_globalInternedEnums(alloc)
			, m_tempInternedEnums(alloc)
			, m_globalObjects(alloc)
		{
		}

		Result CCompiler::Compile()
		{
			m_globalScope = nullptr;
			CHECK(InitGlobalScope());

			HAsmHeader header;
			header.m_pointerLType = UnsignedLType(m_config.m_intptrLType);
			CHECK(m_asmWriter->Start(header));
			
			FileCoordinate coord;
			coord.m_column = 0;
			coord.m_lineNumber = 1;
			coord.m_fileOffset = 0;
			CHECK_RV(bool, isTranslationUnit, ParseTranslationUnit(coord));

			return ErrorCode::kOK;
		}

		ResultRV<bool> CCompiler::ParseTranslationUnit(FileCoordinate &inOutCoordinate)
		{
			bool anyExternalDecl = false;
			for (;;)
			{
				CHECK_RV(bool, haveExternalDecl, ParseExternalDeclaration(inOutCoordinate));
				anyExternalDecl = true;
			}

			if (!anyExternalDecl)
			{
				ReportCompileError(CompilationErrorCode::kExpectedExternalDeclaration, inOutCoordinate);
				return false;
			}

			return true;
		}

		ResultRV<bool> CCompiler::ParseExternalDeclaration(FileCoordinate &inOutCoordinate)
		{
			const bool speculative = false;

			// Either function-definition or declaration
			// function-definition: declaration-specifiers declarator [declaration-list] compound-statement
			// declaration: declaration-specifiers [init-declarator-list] ";"
			//
			// Possible situations:
			// declaration: declaration-specifiers ;
			//     Empty declaration
			// declaration: declaration-specifiers declarator ;
			// declaration: declaration-specifiers declarator = initializer [, init-declarator] ;
			// declaration: declaration-specifiers declarator , [init-declarator-list] ;
			// 
			// function-definition: declaration-specifiers declarator [declaration-list] {

			FileCoordinate coord = inOutCoordinate;
			REQUIRE_PARSE(CDeclarationSpecifiers, declSpecifiers, ParseDeclSpecifiers);

			PEEK_TOKEN(possibleSemiToken, possibleSemiCoord);
			if (possibleSemiToken.IsString(";"))
			{
				ReportCompileWarning(CompilationWarningCode::kEmptyDeclaration, coord);
				coord = possibleSemiCoord;
			}
			else
			{
				REQUIRE_PARSE(CDeclarator, declarator, ParseDeclarator);

				PEEK_TOKEN(postDeclaratorToken, postDeclaratorCoord);
				if (postDeclaratorToken.IsString("="))
				{
					coord = postDeclaratorCoord;

					CHECK(ParseAndCompileInitializerForDeclarator(declSpecifiers, declarator, coord));

					PEEK_TOKEN(postInitializerToken, postInitializerCoord);
					if (postInitializerToken.IsString(";"))
					{
					}
					else if (postInitializerToken.IsString(","))
					{
						CHECK(ParseAndCompileInitDeclaratorListEndingInSemi(declSpecifiers, coord));
					}
					else
					{
						ReportCompileError(CompilationErrorCode::kUnexpectedToken, coord);
						return ErrorCode::kOperationFailed;
					}
				}
				else if (postDeclaratorToken.IsString(","))
				{
					CHECK(CommitDeclarator(declSpecifiers, declarator));
					coord = postDeclaratorCoord;

					CHECK(ParseAndCompileInitDeclaratorListEndingInSemi(declSpecifiers, coord));
				}
				else if (postDeclaratorToken.IsString(";"))
				{
					coord = postDeclaratorCoord;
				}
				else
				{
					CHECK(CompileFunctionDefinitionAfterDeclarator(declSpecifiers, declarator, coord));
				}
			}

			inOutCoordinate = coord;
			return true;
		}

		ResultRV<bool> CCompiler::ParseDeclSpecifiers(FileCoordinate &inOutCoordinate, CorePtr<CDeclarationSpecifiers> &outProduct, bool speculative)
		{
			IAllocator *alloc = GetCoreObjectAllocator();

			// Series of one of:
			// storage-class-specifier
			// type-specifier
			// type-qualifier
			// function-specifier

			// Storage class specifier: typedef, extern, static, auto, register
			// type-specifier: void char short int long float double signed unsigned _Bool _Complex struct-or-union-specifier enum-specifier typedef-name
			// type-qualifier: const restrict volatile
			// function-specifier: inline

			FileCoordinate coord = inOutCoordinate;

			Vector<CorePtr<CGrammarElement>> elements(this->GetCoreObjectAllocator());
			REQUIRE_PARSE(CGrammarElement, firstElement, ParseSingleDeclSpecifier);
			CHECK(elements.Add(std::move(firstElement)));

			for (;;)
			{
				SPECULATIVE_PARSE(CGrammarElement, nextElement, ParseSingleDeclSpecifier);
				if (nextElement == nullptr)
					break;
				CHECK(elements.Add(std::move(nextElement)));
			}

			CHECK_RV(ArrayPtr<CorePtr<CGrammarElement>>, flat, elements.View().CloneTake(alloc));
			CHECK_RV_ASSIGN(outProduct, New<CDeclarationSpecifiers>(alloc, std::move(flat)));

			inOutCoordinate = coord;
			return true;
		}

		ResultRV<bool> CCompiler::ParseSingleDeclSpecifier(FileCoordinate &inOutCoordinate, CorePtr<CGrammarElement> &outProduct, bool speculative)
		{
			IAllocator *alloc = GetCoreObjectAllocator();

			// One of:
			// storage-class-specifier
			// type-specifier
			// type-qualifier
			// function-specifier

			// Storage class specifier: typedef, extern, static, auto, register
			// type-specifier: void char short int long float double signed unsigned _Bool _Complex struct-or-union-specifier enum-specifier typedef-name
			// type-qualifier: const restrict volatile
			// function-specifier: inline

			FileCoordinate coord = inOutCoordinate;

			FileCoordinate preTokenCoord = coord;
			REQUIRE_TOKEN(token);

			if (token.IsString("typedef") || token.IsString("extern") || token.IsString("static") || token.IsString("auto") || token.IsString("register"))
			{
				CHECK_RV(CorePtr<CToken>, tokenElement, New<CToken>(alloc, CGrammarElement::Subtype::kStorageClassSpecifier, token, preTokenCoord));
				outProduct = std::move(tokenElement);
			}
			else if (token.IsString("void") || token.IsString("char") || token.IsString("short") || token.IsString("int") || token.IsString("long")
				|| token.IsString("float") || token.IsString("double") || token.IsString("signed") || token.IsString("unsigned") || token.IsString("_Bool")
				|| token.IsString("_Complex"))
			{
				CHECK_RV(CorePtr<CToken>, tokenElement, New<CToken>(alloc, CGrammarElement::Subtype::kTypeSpecifier, token, preTokenCoord));
				outProduct = std::move(tokenElement);
			}
			else if (token.IsString("const") || token.IsString("restrict") || token.IsString("volatile"))
			{
				CHECK_RV(CorePtr<CToken>, tokenElement, New<CToken>(alloc, CGrammarElement::Subtype::kTypeQualifier, token, preTokenCoord));
				outProduct = std::move(tokenElement);
			}
			else if (token.IsString("inline"))
			{
				CHECK_RV(CorePtr<CToken>, tokenElement, New<CToken>(alloc, CGrammarElement::Subtype::kFunctionSpecifier, token, preTokenCoord));
				outProduct = std::move(tokenElement);
			}
			else if (token.IsString("enum"))
			{
				REQUIRE_PARSE(CEnumSpecifier, enumSpecifier, ParseEnumSpecifierAfterEnum);
				outProduct = std::move(enumSpecifier);
			}
			else if (token.IsString("struct"))
			{
				REQUIRE_PARSE(CStructOrUnionSpecifier, structSpecifier, ParseStructSpecifierAfterStruct);
				outProduct = std::move(structSpecifier);
			}
			else if (token.IsString("union"))
			{
				REQUIRE_PARSE(CStructOrUnionSpecifier, unionSpecifier, ParseUnionSpecifierAfterUnion);
				outProduct = std::move(unionSpecifier);
			}
			else
			{
				coord = preTokenCoord;
				REQUIRE_PARSE(CToken, typedefName, ParseTypedefName);
				outProduct = std::move(typedefName);
			}

			inOutCoordinate = coord;
			return true;
		}


		ResultRV<bool> CCompiler::ParseDeclarator(FileCoordinate &inOutCoordinate, CorePtr<CDeclarator> &outProduct, bool speculative)
		{
			FileCoordinate coord = inOutCoordinate;
			SPECULATIVE_PARSE(CPointer, pointer, ParsePointer);

			REQUIRE_PARSE(CDirectDeclarator, directDecl, ParseDirectDeclarator);

			CHECK_RV_ASSIGN(outProduct, New<CDeclarator>(GetCoreObjectAllocator(), std::move(pointer), std::move(directDecl), inOutCoordinate));

			inOutCoordinate = coord;
			return true;
		}

		ResultRV<bool> CCompiler::ParseAbstractDeclarator(FileCoordinate &inOutCoordinate, CorePtr<CAbstractDeclarator> &outProduct, bool speculative)
		{
			IAllocator *alloc = GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;
			SPECULATIVE_PARSE(CPointer, pointer, ParsePointer);

			if (pointer != nullptr)
			{
				SPECULATIVE_PARSE(CDirectAbstractDeclarator, directAbsDeclarator, ParseDirectAbstractDeclarator);
				if (directAbsDeclarator != nullptr)
				{
					CHECK_RV_ASSIGN(outProduct, New<CAbstractDeclarator>(alloc, std::move(pointer), std::move(directAbsDeclarator)));
				}
				else
				{
					CHECK_RV_ASSIGN(outProduct, New<CAbstractDeclarator>(alloc, std::move(pointer)));
				}
			}
			else
			{
				REQUIRE_PARSE(CDirectAbstractDeclarator, directAbsDeclarator, ParseDirectAbstractDeclarator);
				CHECK_RV_ASSIGN(outProduct, New<CAbstractDeclarator>(alloc, std::move(pointer), std::move(directAbsDeclarator)));
			}

			inOutCoordinate = coord;
			return true;
		}

		ResultRV<bool> CCompiler::ParseDirectDeclaratorContinuation(FileCoordinate &inOutCoordinate, CorePtr<CDirectDeclaratorContinuation> &outProduct, bool speculative)
		{
			IAllocator *alloc = GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			CorePtr<CDirectDeclaratorContinuation> continuation;

			PEEK_TOKEN(possibleBracketToken, possibleBracketEndCoord);
			if (possibleBracketToken.IsString("["))
			{
				coord = possibleBracketEndCoord;

				PEEK_TOKEN(possibleStaticToken1, possibleStaticEndCoord1);
				if (possibleStaticToken1.IsString("static"))
				{
					// static [type-qualifier-list] assignment-expression
					coord = possibleStaticEndCoord1;

					SPECULATIVE_PARSE(CTypeQualifierList, typeQualifierList, ParseTypeQualifierList);
					REQUIRE_PARSE(CExpression, assignmentExpr, ParseAssignmentExpression);

					CHECK_RV(CorePtr<CDirectDeclaratorContinuation>, newDirectDecl, New<CDirectDeclaratorContinuation>(alloc, std::move(typeQualifierList), std::move(assignmentExpr), true, false, inOutCoordinate));
					continuation = std::move(newDirectDecl);
				}
				else
				{
					SPECULATIVE_PARSE(CTypeQualifierList, typeQualifierList, ParseTypeQualifierList);

					PEEK_TOKEN(possibleStaticOrAsteriskToken, possibleStaticOrAsteriskEndCoord);
					if (typeQualifierList != nullptr && possibleStaticOrAsteriskToken.IsString("static"))
					{
						// type-qualifier-list static assignment-expression
						coord = possibleStaticOrAsteriskEndCoord;

						REQUIRE_PARSE(CExpression, assignmentExpr, ParseAssignmentExpression);
						CHECK_RV(CorePtr<CDirectDeclaratorContinuation>, newDirectDecl, New<CDirectDeclaratorContinuation>(alloc, std::move(typeQualifierList), std::move(assignmentExpr), true, false, inOutCoordinate));
						continuation = std::move(newDirectDecl);
					}
					else if (possibleStaticOrAsteriskToken.IsString("*"))
					{
						// [type-qualifier-list] *
						coord = possibleStaticOrAsteriskEndCoord;

						CHECK_RV(CorePtr<CDirectDeclaratorContinuation>, newDirectDecl, New<CDirectDeclaratorContinuation>(alloc, std::move(typeQualifierList), nullptr, false, true, inOutCoordinate));
						continuation = std::move(newDirectDecl);
					}
					else
					{
						// [type-qualifier-list] [assignment-expression]
						SPECULATIVE_PARSE(CExpression, assignmentExpr, ParseAssignmentExpression);
						CHECK_RV(CorePtr<CDirectDeclaratorContinuation>, newDirectDecl, New<CDirectDeclaratorContinuation>(alloc, std::move(typeQualifierList), std::move(assignmentExpr), false, false, inOutCoordinate));
						continuation = std::move(newDirectDecl);
					}
				}

				EXPECT_TOKEN("]");
			}
			else
			{
				EXPECT_TOKEN("(");

				coord = possibleBracketEndCoord;

				SPECULATIVE_PARSE(CParameterTypeList, parameterTypeList, ParseParameterTypeList);
				if (parameterTypeList != nullptr)
				{
					CHECK_RV(CorePtr<CDirectDeclaratorContinuation>, newDirectDecl, New<CDirectDeclaratorContinuation>(alloc, std::move(parameterTypeList), inOutCoordinate));
					continuation = std::move(newDirectDecl);
				}
				else
				{
					SPECULATIVE_PARSE(CIdentifierList, identifierList, ParseIdentifierList);
					CHECK_RV(CorePtr<CDirectDeclaratorContinuation>, newDirectDecl, New<CDirectDeclaratorContinuation>(alloc, std::move(identifierList), inOutCoordinate));
					continuation = std::move(newDirectDecl);
				}

				EXPECT_TOKEN(")");
			}

			outProduct = std::move(continuation);

			inOutCoordinate = coord;
			return true;
		}

		ResultRV<bool> CCompiler::ParseDirectDeclarator(FileCoordinate &inOutCoordinate, CorePtr<CDirectDeclarator> &outProduct, bool speculative)
		{
			IAllocator *alloc = GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			CorePtr<CDirectDeclarator> prevDirectDecl;

			FileCoordinate ddStartCoord = coord;
			PEEK_TOKEN(lparenToken, lparenEndCoord);
			if (lparenToken.IsString("("))
			{
				coord = lparenEndCoord;

				REQUIRE_PARSE(CDeclarator, decl, ParseDeclarator);

				PEEK_TOKEN(rparenToken, rparenEndCoord);
				if (!rparenToken.IsString(")"))
				{
					if (speculative)
						return false;

					ReportCompileError(CompilationErrorCode::kExpectedRightParenAfterDeclarator, coord);
					return ErrorCode::kOperationFailed;
				}

				coord = rparenEndCoord;

				CHECK_RV_ASSIGN(prevDirectDecl, New<CDirectDeclarator>(alloc, std::move(decl), ddStartCoord));
			}
			else
			{
				REQUIRE_PARSE(CToken, identifier, ParseIdentifier);

				CHECK_RV_ASSIGN(prevDirectDecl, New<CDirectDeclarator>(alloc, std::move(identifier), ddStartCoord));
			}

			for (;;)
			{
				ddStartCoord = coord;
				SPECULATIVE_PARSE(CDirectDeclaratorContinuation, continuation, ParseDirectDeclaratorContinuation);
				if (continuation)
				{
					CHECK_RV_ASSIGN(prevDirectDecl, New<CDirectDeclarator>(alloc, std::move(prevDirectDecl), std::move(continuation), ddStartCoord));
				}
				else
					break;
			}
			
			outProduct = std::move(prevDirectDecl);

			inOutCoordinate = coord;
			return true;
		}


		ResultRV<bool> CCompiler::ParseDirectAbstractDeclarator(FileCoordinate &inOutCoordinate, CorePtr<CDirectAbstractDeclarator> &outProduct, bool speculative)
		{
			IAllocator *alloc = GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			CorePtr<CAbstractDeclarator> absDecl;

			FileCoordinate beforePossibleAbstractDecl = coord;
			PEEK_TOKEN(lparenToken, lparenEndCoord);
			if (lparenToken.IsString("("))
			{
				coord = lparenEndCoord;

				SPECULATIVE_PARSE(CAbstractDeclarator, speculativeAbsDecl, ParseAbstractDeclarator);
				if (speculativeAbsDecl != nullptr)
				{
					PEEK_TOKEN(rparenToken, rparenEndCoord);
					if (rparenToken.IsString(")"))
					{
						coord = rparenEndCoord;

						absDecl = std::move(speculativeAbsDecl);
					}
					else
					{
						coord = beforePossibleAbstractDecl;
					}
				}
				else
				{
					coord = beforePossibleAbstractDecl;
				}
			}

			Vector<CorePtr<CDirectAbstractDeclaratorSuffix>> suffixes(alloc);

			for (;;)
			{
				SPECULATIVE_PARSE(CDirectAbstractDeclaratorSuffix, suffix, ParseDirectAbstractDeclaratorSuffix);
				if (suffix == nullptr)
					break;

				CHECK(suffixes.Add(std::move(suffix)));
			}

			CHECK_RV(ArrayPtr<CorePtr<CDirectAbstractDeclaratorSuffix>>, flat, suffixes.View().CloneTake(alloc));
			CHECK_RV_ASSIGN(outProduct, New<CDirectAbstractDeclarator>(alloc, std::move(absDecl), std::move(flat)));

			inOutCoordinate = coord;
			return ErrorCode::kOK;
		}

		ResultRV<bool> CCompiler::ParseDirectAbstractDeclaratorSuffix(FileCoordinate &inOutCoordinate, CorePtr<CDirectAbstractDeclaratorSuffix> &outProduct, bool speculative)
		{
			IAllocator *alloc = GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			PEEK_TOKEN(possibleBracketToken, possibleBracketEndCoord);
			if (possibleBracketToken.IsString("["))
			{
				coord = possibleBracketEndCoord;

				PEEK_TOKEN(possibleStaticToken1, possibleStaticEndCoord1);
				if (possibleStaticToken1.IsString("static"))
				{
					// static [type-qualifier-list] assignment-expression
					coord = possibleStaticEndCoord1;

					SPECULATIVE_PARSE(CTypeQualifierList, typeQualifierList, ParseTypeQualifierList);
					REQUIRE_PARSE(CExpression, assignmentExpr, ParseAssignmentExpression);

					CHECK_RV_ASSIGN(outProduct, New<CDirectAbstractDeclaratorSuffix>(alloc, std::move(typeQualifierList), std::move(assignmentExpr), true));
				}
				else
				{
					SPECULATIVE_PARSE(CTypeQualifierList, typeQualifierList, ParseTypeQualifierList);

					if (typeQualifierList != nullptr)
					{
						PEEK_TOKEN(possibleStaticToken, possibleStaticEndCoord);
						if (possibleStaticToken.IsString("static"))
						{
							// type-qualifier-list static assignment-expression
							coord = possibleStaticEndCoord;

							REQUIRE_PARSE(CExpression, assignmentExpr, ParseAssignmentExpression);
							CHECK_RV_ASSIGN(outProduct, New<CDirectAbstractDeclaratorSuffix>(alloc, std::move(typeQualifierList), std::move(assignmentExpr), true));
						}
						else
						{
							// [type-qualifier-list] [assignment-expression]
							SPECULATIVE_PARSE(CExpression, assignmentExpr, ParseAssignmentExpression);
							CHECK_RV_ASSIGN(outProduct, New<CDirectAbstractDeclaratorSuffix>(alloc, std::move(typeQualifierList), std::move(assignmentExpr), false));
						}
					}
					else
					{
						PEEK_TOKEN(possibleAsteriskToken, possibleAsteriskEndCoord);
						if (possibleAsteriskToken.IsString("*"))
						{
							coord = possibleAsteriskEndCoord;

							CHECK_RV_ASSIGN(outProduct, New<CDirectAbstractDeclaratorSuffix>(alloc));
						}
						else
						{
							// [[assignment-expression]
							SPECULATIVE_PARSE(CExpression, assignmentExpr, ParseAssignmentExpression);
							CHECK_RV_ASSIGN(outProduct, New<CDirectAbstractDeclaratorSuffix>(alloc, nullptr, std::move(assignmentExpr), false));
						}
					}
				}

				EXPECT_TOKEN("]");
			}
			else if (possibleBracketToken.IsString("("))
			{
				coord = possibleBracketEndCoord;

				SPECULATIVE_PARSE(CParameterTypeList, parameterTypeList, ParseParameterTypeList);
				if (parameterTypeList != nullptr)
				{
					CHECK_RV_ASSIGN(outProduct, New<CDirectAbstractDeclaratorSuffix>(alloc, std::move(parameterTypeList)));
				}
				else
				{
					SPECULATIVE_PARSE(CIdentifierList, identifierList, ParseIdentifierList);
					CHECK_RV_ASSIGN(outProduct, New<CDirectAbstractDeclaratorSuffix>(alloc, std::move(identifierList)));
				}

				EXPECT_TOKEN(")");
			}

			inOutCoordinate = coord;
			return true;
		}

		ResultRV<bool> CCompiler::ParseEnumSpecifierAfterEnum(FileCoordinate &inOutCoordinate, CorePtr<CEnumSpecifier> &outProduct, bool speculative)
		{
			IAllocator *alloc = GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;
			SPECULATIVE_PARSE(CToken, identifier, ParseIdentifier);

			PEEK_TOKEN(openBraceToken, openBraceEndCoord);
			if (openBraceToken.IsString("{"))
			{
				coord = openBraceEndCoord;

				REQUIRE_PARSE(CEnumeratorList, enumeratorList, ParseEnumeratorList);
				CHECK_RV_ASSIGN(outProduct, New<CEnumSpecifier>(alloc, CEnumSpecifier(std::move(identifier), std::move(enumeratorList), inOutCoordinate)));

				PEEK_TOKEN(afterEnumeratorListToken, afterEnumeratorListEndCoord);
				if (afterEnumeratorListToken.IsString(","))
					coord = afterEnumeratorListEndCoord;

				EXPECT_TOKEN("}");
			}
			else
			{
				if (identifier != nullptr)
				{
					CHECK_RV_ASSIGN(outProduct, New<CEnumSpecifier>(alloc, CEnumSpecifier(std::move(identifier), nullptr, inOutCoordinate)));
				}
				else
				{
					if (speculative)
						return false;

					ReportCompileError(CompilationErrorCode::kExpectedIdentifierOrOpenBrace, coord);
					return ErrorCode::kOperationFailed;
				}
			}

			inOutCoordinate = coord;
			return true;
		}

		ResultRV<bool> CCompiler::ParseEnumeratorList(FileCoordinate &inOutCoordinate, CorePtr<CEnumeratorList> &outProduct, bool speculative)
		{
			IAllocator *alloc = GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			REQUIRE_PARSE(CEnumerator, firstEnumerator, ParseEnumerator);

			Vector<CorePtr<CEnumerator>> enumerators(alloc);
			CHECK(enumerators.Add(std::move(firstEnumerator)));

			for (;;)
			{
				FileCoordinate possibleEOLCoord = coord;

				PEEK_TOKEN(nextToken, nextTokenEndCoord);
				if (nextToken.IsString(","))
				{
					coord = nextTokenEndCoord;
					SPECULATIVE_PARSE(CEnumerator, nextEnumerator, ParseEnumerator);

					if (nextEnumerator == nullptr)
					{
						coord = possibleEOLCoord;
						break;
					}
					else
					{
						CHECK(enumerators.Add(std::move(nextEnumerator)));
					}
				}
			}

			CHECK_RV(ArrayPtr<CorePtr<CEnumerator>>, enumeratorsFlat, enumerators.View().CloneTake(alloc));
			CHECK_RV_ASSIGN(outProduct, New<CEnumeratorList>(alloc, std::move(enumeratorsFlat)));

			inOutCoordinate = coord;
			return true;
		}

		ResultRV<bool> CCompiler::ParseEnumerator(FileCoordinate &inOutCoordinate, CorePtr<CEnumerator> &outProduct, bool speculative)
		{
			IAllocator *alloc = GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			REQUIRE_PARSE(CToken, enumConstant, ParseIdentifier);

			PEEK_TOKEN(possibleEqualToken, possibleEqualEndCoord);
			if (possibleEqualToken.IsString("="))
			{
				FileCoordinate eqFailCoord = coord;
				coord = possibleEqualEndCoord;

				SPECULATIVE_PARSE(CExpression, constantExpr, ParseConstantExpression);
				if (constantExpr == nullptr)
					coord = eqFailCoord;
				else
				{
					CHECK_RV_ASSIGN(outProduct, New<CEnumerator>(alloc, std::move(enumConstant), std::move(constantExpr)));
				}
			}
			else
			{
				CHECK_RV_ASSIGN(outProduct, New<CEnumerator>(alloc, std::move(enumConstant), nullptr));
			}

			inOutCoordinate = coord;
			return true;
		}

		ResultRV<bool> CCompiler::ParseStructSpecifierAfterStruct(FileCoordinate &inOutCoordinate, CorePtr<CStructOrUnionSpecifier> &outProduct, bool speculative)
		{
			return ParseStructOrUnionSpecifierAfterDesignator(inOutCoordinate, outProduct, speculative, CAggregateType::kStruct);
		}

		ResultRV<bool> CCompiler::ParseUnionSpecifierAfterUnion(FileCoordinate &inOutCoordinate, CorePtr<CStructOrUnionSpecifier> &outProduct, bool speculative)
		{
			return ParseStructOrUnionSpecifierAfterDesignator(inOutCoordinate, outProduct, speculative, CAggregateType::kUnion);
		}

		ResultRV<bool> CCompiler::ParseStructOrUnionSpecifierAfterDesignator(FileCoordinate &inOutCoordinate, CorePtr<CStructOrUnionSpecifier> &outProduct, bool speculative, CAggregateType aggType)
		{
			IAllocator *alloc = GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;
			SPECULATIVE_PARSE(CToken, identifier, ParseIdentifier);

			PEEK_TOKEN(openBraceToken, openBraceEndCoord);
			if (openBraceToken.IsString("{"))
			{
				coord = openBraceEndCoord;

				REQUIRE_PARSE(CStructDeclarationList, structDeclarationList, ParseStructDeclarationList);
				CHECK_RV_ASSIGN(outProduct, New<CStructOrUnionSpecifier>(alloc, std::move(identifier), std::move(structDeclarationList), aggType, inOutCoordinate));

				EXPECT_TOKEN("}");
			}
			else
			{
				if (identifier != nullptr)
				{
					CHECK_RV_ASSIGN(outProduct, New<CStructOrUnionSpecifier>(alloc, std::move(identifier), nullptr, aggType, inOutCoordinate));
				}
				else
				{
					if (speculative)
						return false;

					ReportCompileError(CompilationErrorCode::kExpectedIdentifierOrOpenBrace, coord);
					return ErrorCode::kOperationFailed;
				}
			}

			inOutCoordinate = coord;
			return true;
		}

		ResultRV<bool> CCompiler::ParseStructDeclarationList(FileCoordinate &inOutCoordinate, CorePtr<CStructDeclarationList> &outProduct, bool speculative)
		{
			IAllocator *alloc = GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			Vector<CorePtr<CStructDeclaration>> structDecls(alloc);

			REQUIRE_PARSE(CStructDeclaration, firstDecl, ParseStructDeclaration);
			CHECK(structDecls.Add(std::move(firstDecl)));

			for (;;)
			{
				SPECULATIVE_PARSE(CStructDeclaration, nextDecl, ParseStructDeclaration);
				if (nextDecl == nullptr)
					break;

				CHECK(structDecls.Add(std::move(nextDecl)));
			}

			CHECK_RV(ArrayPtr<CorePtr<CStructDeclaration>>, flat, structDecls.View().CloneTake(alloc));
			CHECK_RV_ASSIGN(outProduct, New<CStructDeclarationList>(alloc, std::move(flat)));

			inOutCoordinate = coord;
			return true;
		}

		ResultRV<bool> CCompiler::ParseInitDeclarator(FileCoordinate &inOutCoordinate, CorePtr<CInitDeclarator> &outProduct, bool speculative)
		{
			IAllocator *alloc = GetCoreObjectAllocator();
			FileCoordinate coord = inOutCoordinate;

			REQUIRE_PARSE(CDeclarator, decl, ParseDeclarator);

			CorePtr<CInitializer> initializer;

			FileCoordinate backupCoord = coord;
			PEEK_TOKEN(equalToken, equalEndCoord);
			if (equalToken.IsString("="))
			{
				coord = equalEndCoord;
				SPECULATIVE_PARSE(CInitializer, possibleInitializer, ParseInitializer);
				if (possibleInitializer == nullptr)
					initializer = std::move(possibleInitializer);
				else
					coord = backupCoord;
			}

			CHECK_RV_ASSIGN(outProduct, New<CInitDeclarator>(alloc, std::move(decl), std::move(initializer)));

			inOutCoordinate = coord;
			return true;
		}

		ResultRV<bool> CCompiler::ParseInitDeclaratorList(FileCoordinate &inOutCoordinate, CorePtr<CInitDeclaratorList> &outProduct, bool speculative)
		{
			IAllocator *alloc = GetCoreObjectAllocator();
			FileCoordinate coord = inOutCoordinate;

			Vector<CorePtr<CInitDeclarator>> initDecls(alloc);

			REQUIRE_PARSE(CInitDeclarator, firstDecl, ParseInitDeclarator);
			CHECK(initDecls.Add(std::move(firstDecl)));

			for (;;)
			{
				SPECULATIVE_PARSE(CInitDeclarator, nextDecl, ParseInitDeclarator);
				if (nextDecl == nullptr)
					break;

				CHECK(initDecls.Add(std::move(nextDecl)));
			}

			CHECK_RV(ArrayPtr<CorePtr<CInitDeclarator>>, flat, initDecls.View().CloneTake(alloc));
			CHECK_RV_ASSIGN(outProduct, New<CInitDeclaratorList>(alloc, std::move(flat)));

			inOutCoordinate = coord;
			return true;
		}

		ResultRV<bool> CCompiler::ParseDeclaration(FileCoordinate &inOutCoordinate, CorePtr<CDeclaration> &outProduct, bool speculative)
		{
			IAllocator *alloc = GetCoreObjectAllocator();
			FileCoordinate coord = inOutCoordinate;

			REQUIRE_PARSE(CDeclarationSpecifiers, declSpecs, ParseDeclSpecifiers);
			SPECULATIVE_PARSE(CInitDeclaratorList, initDeclList, ParseInitDeclaratorList);
			EXPECT_TOKEN(";");

			CHECK_RV_ASSIGN(outProduct, New<CDeclaration>(alloc, std::move(declSpecs), std::move(initDeclList)));

			inOutCoordinate = coord;
			return true;
		}

		ResultRV<bool> CCompiler::ParseDeclarationList(FileCoordinate &inOutCoordinate, CorePtr<CDeclarationList> &outProduct, bool speculative)
		{
			IAllocator *alloc = GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			Vector<CorePtr<CDeclaration>> decls(alloc);

			REQUIRE_PARSE(CDeclaration, firstDecl, ParseDeclaration);
			CHECK(decls.Add(std::move(firstDecl)));

			for (;;)
			{
				SPECULATIVE_PARSE(CDeclaration, nextDecl, ParseDeclaration);
				if (nextDecl == nullptr)
					break;

				CHECK(decls.Add(std::move(nextDecl)));
			}

			CHECK_RV(ArrayPtr<CorePtr<CDeclaration>>, flat, decls.View().CloneTake(alloc));
			CHECK_RV_ASSIGN(outProduct, New<CDeclarationList>(alloc, std::move(flat)));

			inOutCoordinate = coord;
			return true;
		}

		ResultRV<bool> CCompiler::ParseStructDeclaration(FileCoordinate &inOutCoordinate, CorePtr<CStructDeclaration> &outProduct, bool speculative)
		{
			IAllocator *alloc = GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			REQUIRE_PARSE(CSpecifierQualifierList, specQualList, ParseSpecifierQualifierList);
			REQUIRE_PARSE(CStructDeclaratorList, structDeclList, ParseStructDeclaratorList);
			EXPECT_TOKEN(";");

			inOutCoordinate = coord;
			return true;
		}

		ResultRV<bool> CCompiler::ParseSpecifierQualifierList(FileCoordinate &inOutCoordinate, CorePtr<CSpecifierQualifierList> &outProduct, bool speculative)
		{
			IAllocator *alloc = GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			Vector<CorePtr<CGrammarElement>> elements(alloc);

			SPECULATIVE_PARSE(CToken, openingQualifier, ParseTypeQualifier);
			if (openingQualifier == nullptr)
			{
				REQUIRE_PARSE(CGrammarElement, openingSpecifier, ParseTypeSpecifier);

				CHECK(elements.Add(std::move(openingSpecifier)));
			}
			else
			{
				CHECK(elements.Add(CorePtr<CGrammarElement>(std::move(openingQualifier))));
			}

			for (;;)
			{
				SPECULATIVE_PARSE(CToken, qualifier, ParseTypeQualifier);
				if (qualifier != nullptr)
				{
					CHECK(elements.Add(CorePtr<CGrammarElement>(std::move(qualifier))));
					continue;
				}

				SPECULATIVE_PARSE(CGrammarElement, specifier, ParseTypeSpecifier);
				if (specifier != nullptr)
				{
					CHECK(elements.Add(CorePtr<CGrammarElement>(std::move(specifier))));
					continue;
				}

				break;
			}

			CHECK_RV(ArrayPtr<CorePtr<CGrammarElement>>, flat, elements.View().CloneTake(alloc));
			CHECK_RV_ASSIGN(outProduct, New<CSpecifierQualifierList>(alloc, std::move(flat)));

			inOutCoordinate = coord;
			return true;
		}

		ResultRV<bool> CCompiler::ParseStructDeclaratorList(FileCoordinate &inOutCoordinate, CorePtr<CStructDeclaratorList> &outProduct, bool speculative)
		{
			IAllocator *alloc = GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			Vector<CorePtr<CStructDeclarator>> elements(alloc);

			REQUIRE_PARSE(CStructDeclarator, firstDeclarator, ParseStructDeclarator);
			CHECK(elements.Add(std::move(firstDeclarator)));

			for (;;)
			{
				FileCoordinate nextDeclFailCoord = coord;

				PEEK_TOKEN(commaToken, commaEndCoord);
				if (commaToken.IsString(","))
				{
					coord = commaEndCoord;

					SPECULATIVE_PARSE(CStructDeclarator, nextDeclarator, ParseStructDeclarator);
					if (nextDeclarator)
					{
						CHECK(elements.Add(std::move(nextDeclarator)));
						continue;
					}
					else
						coord = nextDeclFailCoord;
				}

				break;
			}

			CHECK_RV(ArrayPtr<CorePtr<CStructDeclarator>>, flat, elements.View().CloneTake(alloc));
			CHECK_RV_ASSIGN(outProduct, New<CStructDeclaratorList>(alloc, std::move(flat)));

			inOutCoordinate = coord;
			return true;
		}

		ResultRV<bool> CCompiler::ParseStructDeclarator(FileCoordinate &inOutCoordinate, CorePtr<CStructDeclarator> &outProduct, bool speculative)
		{
			IAllocator *alloc = GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			SPECULATIVE_PARSE(CDeclarator, decl, ParseDeclarator);
			if (decl != nullptr)
			{
				FileCoordinate preColonCoord = coord;

				PEEK_TOKEN(colonToken, colonEndCoord);
				if (colonToken.IsString(":"))
				{
					SPECULATIVE_PARSE(CExpression, constExpr, ParseConstantExpression);
					if (constExpr != nullptr)
					{
						CHECK_RV_ASSIGN(outProduct, New<CStructDeclarator>(alloc, std::move(decl), std::move(constExpr)));
					}
					else
					{
						CHECK_RV_ASSIGN(outProduct, New<CStructDeclarator>(alloc, std::move(decl)));
						coord = preColonCoord;
					}
				}
				else
				{
					CHECK_RV_ASSIGN(outProduct, New<CStructDeclarator>(alloc, std::move(decl)));
				}
			}
			else
			{
				EXPECT_TOKEN(":");
				REQUIRE_PARSE(CExpression, constExpr, ParseConstantExpression);

				CHECK_RV_ASSIGN(outProduct, New<CStructDeclarator>(alloc, std::move(constExpr)));
			}


			inOutCoordinate = coord;
			return true;
		}

		ResultRV<bool> CCompiler::ParseTypeQualifier(FileCoordinate &inOutCoordinate, CorePtr<CToken> &outProduct, bool speculative)
		{
			IAllocator *alloc = GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			PEEK_TOKEN(token, tokenEndCoord);

			if (token.IsString("const") || token.IsString("restrict") || token.IsString("volatile"))
			{
				CHECK_RV_ASSIGN(outProduct, New<CToken>(alloc, CGrammarElement::Subtype::kTypeQualifier, token, coord));
				coord = tokenEndCoord;
			}
			else
			{
				if (speculative)
					return false;

				ReportCompileError(CompilationErrorCode::kUnexpectedToken, coord);
				return ErrorCode::kOperationFailed;
			}

			inOutCoordinate = coord;
			return true;
		}

		ResultRV<bool> CCompiler::ParseTypeSpecifier(FileCoordinate &inOutCoordinate, CorePtr<CGrammarElement> &outProduct, bool speculative)
		{
			IAllocator *alloc = GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			PEEK_TOKEN(token, tokenEndCoord);

			if (token.IsString("void") || token.IsString("char") || token.IsString("short") || token.IsString("int") || token.IsString("long")
				|| token.IsString("float") || token.IsString("double") || token.IsString("signed") || token.IsString("unsigned") || token.IsString("_Bool")
				|| token.IsString("_Complex"))
			{
				CHECK_RV_ASSIGN(outProduct, New<CToken>(alloc, CGrammarElement::Subtype::kTypeSpecifier, token, coord));
				coord = tokenEndCoord;
			}
			else if (token.IsString("struct"))
			{
				coord = tokenEndCoord;

				REQUIRE_PARSE(CStructOrUnionSpecifier, souSpecifier, ParseStructSpecifierAfterStruct);
				outProduct = CorePtr<CGrammarElement>(std::move(souSpecifier));
			}
			else if (token.IsString("union"))
			{
				coord = tokenEndCoord;

				REQUIRE_PARSE(CStructOrUnionSpecifier, souSpecifier, ParseUnionSpecifierAfterUnion);
				outProduct = CorePtr<CGrammarElement>(std::move(souSpecifier));
			}
			else if (token.IsString("enum"))
			{
				coord = tokenEndCoord;

				REQUIRE_PARSE(CEnumSpecifier, enumSpecifier, ParseEnumSpecifierAfterEnum);
				outProduct = CorePtr<CGrammarElement>(std::move(enumSpecifier));
			}
			else
			{
				REQUIRE_PARSE(CToken, typedefName, ParseTypedefName);
				outProduct = CorePtr<CGrammarElement>(std::move(typedefName));
			}

			inOutCoordinate = coord;
			return true;
		}

		ResultRV<bool> CCompiler::ParsePointer(FileCoordinate &inOutCoordinate, CorePtr<CPointer> &outProduct, bool speculative)
		{
			IAllocator *alloc = GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			Vector<CPointer::IndirectionLevel> indirLevels(alloc);

			EXPECT_TOKEN("*");

			SPECULATIVE_PARSE(CTypeQualifierList, qualList, ParseTypeQualifierList);

			CPointer::IndirectionLevel indirLevel;
			indirLevel.m_optTypeQualifierList = std::move(qualList);

			CHECK(indirLevels.Add(std::move(indirLevel)));

			for (;;)
			{
				FileCoordinate backupCoord = coord;

				PEEK_TOKEN(asteriskToken, asteriskEndCoord);
				if (asteriskToken.IsString("*"))
				{
					coord = asteriskEndCoord;

					SPECULATIVE_PARSE(CTypeQualifierList, qualList2, ParseTypeQualifierList);
					CPointer::IndirectionLevel indirLevel2;
					indirLevel2.m_optTypeQualifierList = std::move(qualList2);

					CHECK(indirLevels.Add(std::move(indirLevel)));
				}
				else
					break;
			}

			CHECK_RV(ArrayPtr<CPointer::IndirectionLevel>, flat, indirLevels.View().CloneTake(alloc));
			CHECK_RV_ASSIGN(outProduct, New<CPointer>(alloc, std::move(flat)));

			inOutCoordinate = coord;
			return true;
		}

		ResultRV<bool> CCompiler::ParseIdentifier(FileCoordinate &inOutCoordinate, CorePtr<CToken> &outProduct, bool speculative)
		{
			IAllocator *alloc = GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			TokenStrView token;
			CLexer::TokenType tokenType;
			if (!GetToken(token, coord, tokenType))
			{
				if (speculative)
					return false;

				ReportCompileError(CompilationErrorCode::kUnexpectedEndOfFile, inOutCoordinate);
				return ErrorCode::kOperationFailed;
			}

			if (tokenType != CLexer::TokenType::kIdentifier || IsKeyword(token))
			{
				if (speculative)
					return false;

				ReportCompileError(CompilationErrorCode::kExpectedIdentifier, inOutCoordinate);
				return ErrorCode::kOperationFailed;
			}

			CHECK_RV_ASSIGN(outProduct, New<CToken>(alloc, CGrammarElement::Subtype::kIdentifier, token, inOutCoordinate));

			inOutCoordinate = coord;
			return true;
		}

		ResultRV<bool> CCompiler::ParseTypeQualifierList(FileCoordinate &inOutCoordinate, CorePtr<CTypeQualifierList> &outProduct, bool speculative)
		{
			IAllocator *alloc = GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			Vector<CorePtr<CToken>> elements(alloc);

			REQUIRE_PARSE(CToken, firstQualifier, ParseTypeQualifier);
			CHECK(elements.Add(std::move(firstQualifier)));

			for (;;)
			{
				SPECULATIVE_PARSE(CToken, qualifier, ParseTypeQualifier);
				if (qualifier != nullptr)
				{
					CHECK(elements.Add(CorePtr<CToken>(std::move(qualifier))));
					continue;
				}

				break;
			}

			CHECK_RV(ArrayPtr<CorePtr<CToken>>, flat, elements.View().CloneTake(alloc));
			CHECK_RV_ASSIGN(outProduct, New<CTypeQualifierList>(alloc, std::move(flat)));

			inOutCoordinate = coord;
			return true;
		}

		ResultRV<bool> CCompiler::ParseParameterTypeList(FileCoordinate &inOutCoordinate, CorePtr<CParameterTypeList> &outProduct, bool speculative)
		{
			IAllocator *alloc = GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			Vector<CorePtr<CParameterDeclaration>> elements(alloc);

			REQUIRE_PARSE(CParameterDeclaration, firstDeclaration, ParseParameterDeclaration);
			CHECK(elements.Add(std::move(firstDeclaration)));

			bool isVarArg = false;
			for (;;)
			{
				FileCoordinate preCommaCoord = coord;
				PEEK_TOKEN(commaToken, commaEndCoord);
				if (commaToken.IsString(","))
				{
					coord = commaEndCoord;

					PEEK_TOKEN(dotsToken, dotsEndCoord);
					if (dotsToken.IsString("..."))
					{
						coord = dotsEndCoord;
						isVarArg = true;
						break;
					}
					else
					{
						SPECULATIVE_PARSE(CParameterDeclaration, nextDeclaration, ParseParameterDeclaration);
						if (nextDeclaration != nullptr)
						{
							CHECK(elements.Add(std::move(nextDeclaration)));
						}
						else
						{
							coord = preCommaCoord;
							break;
						}
					}
				}
				else
				{
					break;
				}
			}

			CHECK_RV(ArrayPtr<CorePtr<CParameterDeclaration>>, flat, elements.View().CloneTake(alloc));
			CHECK_RV_ASSIGN(outProduct, New<CParameterTypeList>(alloc, std::move(flat), isVarArg));

			inOutCoordinate = coord;
			return true;
		}

		ResultRV<bool> CCompiler::ParseParameterDeclaration(FileCoordinate &inOutCoordinate, CorePtr<CParameterDeclaration> &outProduct, bool speculative)
		{
			IAllocator *alloc = GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			REQUIRE_PARSE(CDeclarationSpecifiers, declSpecifiers, ParseDeclSpecifiers);

			SPECULATIVE_PARSE(CDeclarator, declarator, ParseDeclarator);
			if (declarator != nullptr)
			{
				CHECK_RV_ASSIGN(outProduct, New<CParameterDeclaration>(alloc, std::move(declSpecifiers), std::move(declarator)));
			}
			else
			{
				SPECULATIVE_PARSE(CAbstractDeclarator, absDeclarator, ParseAbstractDeclarator);
				if (absDeclarator != nullptr)
				{
					CHECK_RV_ASSIGN(outProduct, New<CParameterDeclaration>(alloc, std::move(declSpecifiers), std::move(absDeclarator)));
				}
				else
				{
					CHECK_RV_ASSIGN(outProduct, New<CParameterDeclaration>(alloc, std::move(declSpecifiers)));
				}
			}

			inOutCoordinate = coord;
			return true;
		}

		ResultRV<bool> CCompiler::ParseIdentifierList(FileCoordinate &inOutCoordinate, CorePtr<CIdentifierList> &outProduct, bool speculative)
		{
			IAllocator *alloc = GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			Vector<CorePtr<CToken>> identifiers(alloc);

			REQUIRE_PARSE(CToken, firstIdentifier, ParseIdentifier);
			CHECK(identifiers.Add(std::move(firstIdentifier)));

			for (;;)
			{
				FileCoordinate possibleEOLCoord = coord;

				PEEK_TOKEN(nextToken, nextTokenEndCoord);
				if (nextToken.IsString(","))
				{
					coord = nextTokenEndCoord;
					SPECULATIVE_PARSE(CToken, nextIdentifier, ParseIdentifier);

					if (nextIdentifier == nullptr)
					{
						coord = possibleEOLCoord;
						break;
					}
					else
					{
						CHECK(identifiers.Add(std::move(nextIdentifier)));
					}
				}
				else
					break;
			}

			CHECK_RV(ArrayPtr<CorePtr<CToken>>, flat, identifiers.View().CloneTake(alloc));
			CHECK_RV_ASSIGN(outProduct, New<CIdentifierList>(alloc, std::move(flat)));

			inOutCoordinate = coord;
			return true;
		}

		ResultRV<bool> CCompiler::ParseTypeName(FileCoordinate &inOutCoordinate, CorePtr<CTypeName> &outProduct, bool speculative)
		{
			IAllocator *alloc = GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			REQUIRE_PARSE(CSpecifierQualifierList, specQualList, ParseSpecifierQualifierList);
			SPECULATIVE_PARSE(CAbstractDeclarator, absDecl, ParseAbstractDeclarator);

			CHECK_RV_ASSIGN(outProduct, New<CTypeName>(alloc, std::move(specQualList), std::move(absDecl)));

			inOutCoordinate = coord;
			return ErrorCode::kOK;
		}

		ResultRV<bool> CCompiler::ParseConstantExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative)
		{
			return ParseConditionalExpression(inOutCoordinate, outProduct, speculative);
		}

		ResultRV<bool> CCompiler::ParseConditionalExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative)
		{
			IAllocator *alloc = CCompiler::GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			REQUIRE_PARSE(CExpression, resultExpr, ParseLogicalOrExpression);

			FileCoordinate fallbackCoord = coord;
			bool isTernary = false;
			PEEK_TOKEN(questionToken, questionEndCoord);

			if (questionToken.IsString("?"))
			{
				coord = questionEndCoord;
				SPECULATIVE_PARSE(CExpression, trueExpression, ParseExpression);

				if (trueExpression != nullptr)
				{
					PEEK_TOKEN(colonToken, colonEndCoord);

					if (colonToken.IsString(":"))
					{
						coord = colonEndCoord;
						SPECULATIVE_PARSE(CExpression, falseExpression, ParseConditionalExpression);

						if (falseExpression != nullptr)
						{
							isTernary = true;
							CHECK_RV_ASSIGN(resultExpr, New<CTernaryExpression>(alloc, std::move(resultExpr), std::move(trueExpression), std::move(falseExpression)));
						}
					}
				}

			}

			if (!isTernary)
				coord = fallbackCoord;

			outProduct = std::move(resultExpr);

			inOutCoordinate = coord;
			return ErrorCode::kOK;
		}

		ResultRV<bool> CCompiler::ParseAssignmentExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative)
		{
			IAllocator *alloc = CCompiler::GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;
				
			bool isAssignment = true;
			SPECULATIVE_PARSE(CExpression, unaryExpr, ParseUnaryExpression);
			if (unaryExpr != nullptr)
			{
				PEEK_TOKEN(operatorToken, operatorEndCoord);

				CBinaryOperator binOp = CBinaryOperator::kInvalid;
				if (operatorToken.IsString("="))
					binOp = CBinaryOperator::kAssign;
				if (operatorToken.IsString("*="))
					binOp = CBinaryOperator::kMulAssign;
				if (operatorToken.IsString("/="))
					binOp = CBinaryOperator::kDivAssign;
				if (operatorToken.IsString("%="))
					binOp = CBinaryOperator::kModAssign;
				if (operatorToken.IsString("+="))
					binOp = CBinaryOperator::kAddAssign;
				if (operatorToken.IsString("-="))
					binOp = CBinaryOperator::kSubAssign;
				if (operatorToken.IsString("<<="))
					binOp = CBinaryOperator::kLshAssign;
				if (operatorToken.IsString(">>="))
					binOp = CBinaryOperator::kRshAssign;
				if (operatorToken.IsString("&="))
					binOp = CBinaryOperator::kBitAndAssign;
				if (operatorToken.IsString("^="))
					binOp = CBinaryOperator::kBitXorAssign;
				if (operatorToken.IsString("|="))
					binOp = CBinaryOperator::kBitOrAssign;
				else
					isAssignment = false;

				if (isAssignment)
				{
					coord = operatorEndCoord;
					SPECULATIVE_PARSE(CExpression, rightSide, ParseAssignmentExpression);

					if (rightSide == nullptr)
						isAssignment = false;
					else
					{
						CHECK_RV_ASSIGN(outProduct, New<CBinaryExpression>(alloc, std::move(unaryExpr), std::move(rightSide), binOp));
					}
				}
			}
			else
				isAssignment = false;

			if (!isAssignment)
				return ParseConditionalExpression(inOutCoordinate, outProduct, speculative);

			inOutCoordinate = coord;
			return ErrorCode::kOK;
		}

		ResultRV<bool> CCompiler::ParseLogicalOrExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative)
		{
			return DynamicParseLTRBinaryExpression(inOutCoordinate, outProduct, speculative, ResolveLogicalOrOperator, &CCompiler::ParseLogicalAndExpression);
		}

		ResultRV<bool> CCompiler::ParseLogicalAndExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative)
		{
			return DynamicParseLTRBinaryExpression(inOutCoordinate, outProduct, speculative, ResolveLogicalAndOperator, &CCompiler::ParseInclusiveOrExpression);
		}

		ResultRV<bool> CCompiler::ParseInclusiveOrExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative)
		{
			return DynamicParseLTRBinaryExpression(inOutCoordinate, outProduct, speculative, ResolveInclusiveOrOperator, &CCompiler::ParseExclusiveOrExpression);
		}

		ResultRV<bool> CCompiler::ParseExclusiveOrExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative)
		{
			return DynamicParseLTRBinaryExpression(inOutCoordinate, outProduct, speculative, ResolveExclusiveOrOperator, &CCompiler::ParseAndExpression);
		}

		ResultRV<bool> CCompiler::ParseAndExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative)
		{
			return DynamicParseLTRBinaryExpression(inOutCoordinate, outProduct, speculative, ResolveAndOperator, &CCompiler::ParseEqualityExpression);
		}

		ResultRV<bool> CCompiler::ParseEqualityExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative)
		{
			return DynamicParseLTRBinaryExpression(inOutCoordinate, outProduct, speculative, ResolveEqualityOperator, &CCompiler::ParseRelationalExpression);
		}

		ResultRV<bool> CCompiler::ParseRelationalExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative)
		{
			return DynamicParseLTRBinaryExpression(inOutCoordinate, outProduct, speculative, ResolveRelationalOperator, &CCompiler::ParseShiftExpression);
		}

		ResultRV<bool> CCompiler::ParseShiftExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative)
		{
			return DynamicParseLTRBinaryExpression(inOutCoordinate, outProduct, speculative, ResolveShiftOperator, &CCompiler::ParseAdditiveExpression);
		}

		ResultRV<bool> CCompiler::ParseAdditiveExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative)
		{
			return DynamicParseLTRBinaryExpression(inOutCoordinate, outProduct, speculative, ResolveAdditiveOperator, &CCompiler::ParseMultiplicativeExpression);
		}

		ResultRV<bool> CCompiler::ParseMultiplicativeExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative)
		{
			return DynamicParseLTRBinaryExpression(inOutCoordinate, outProduct, speculative, ResolveMultiplicativeOperator, &CCompiler::ParseCastExpression);
		}

		ResultRV<bool> CCompiler::ParseExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative)
		{
			return DynamicParseLTRBinaryExpression(inOutCoordinate, outProduct, speculative, ResolveCommaOperator, &CCompiler::ParseConditionalExpression);
		}

		ResultRV<bool> CCompiler::ParseUnaryExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative)
		{
			IAllocator *alloc = CCompiler::GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			FileCoordinate fallbackCoord = coord;

			PEEK_TOKEN(opToken, opEndCoord);
			CUnaryOperator unaryOp = CUnaryOperator::kInvalid;
			if (opToken.IsString("++"))
				unaryOp = CUnaryOperator::kPreIncrement;
			else if (opToken.IsString("--"))
				unaryOp = CUnaryOperator::kPreDecrement;

			bool isUnaryExpression = false;
			if (unaryOp != CUnaryOperator::kInvalid)
			{
				coord = opEndCoord;
				REQUIRE_PARSE(CExpression, rightSideExpr, ParseUnaryExpression);
				CHECK_RV_ASSIGN(outProduct, New<CUnaryExpression>(alloc, std::move(rightSideExpr), unaryOp));
				isUnaryExpression = true;
			}
			else
			{
				if (opToken.IsString("&"))
					unaryOp = CUnaryOperator::kReference;
				else if (opToken.IsString("*"))
					unaryOp = CUnaryOperator::kDereference;
				else if (opToken.IsString("+"))
					unaryOp = CUnaryOperator::kAbs;
				else if (opToken.IsString("-"))
					unaryOp = CUnaryOperator::kNeg;
				else if (opToken.IsString("~"))
					unaryOp = CUnaryOperator::kBitNot;
				else if (opToken.IsString("!"))
					unaryOp = CUnaryOperator::kLogicalNot;

				if (unaryOp != CUnaryOperator::kInvalid)
				{
					coord = opEndCoord;
					REQUIRE_PARSE(CExpression, rightSideExpr, ParseCastExpression);
					CHECK_RV_ASSIGN(outProduct, New<CUnaryExpression>(alloc, std::move(rightSideExpr), unaryOp));
					isUnaryExpression = true;
				}
				else
				{
					if (opToken.IsString("sizeof"))
					{
						coord = opEndCoord;

						FileCoordinate sizeofArgCoordinate = coord;
						bool isSizeofType = false;
						PEEK_TOKEN(lparenToken, lparenEndCoord);
						if (lparenToken.IsString("("))
						{
							coord = lparenEndCoord;

							SPECULATIVE_PARSE(CTypeName, typeName, ParseTypeName);
							if (typeName != nullptr)
							{
								PEEK_TOKEN(rparenToken, rparenEndCoord);
								if (rparenToken.IsString(")"))
								{
									coord = rparenEndCoord;
									isSizeofType = true;

									CHECK_RV_ASSIGN(outProduct, New<CSizeOfTypeExpression>(alloc, std::move(typeName)));
								}
							}
						}

						if (!isSizeofType)
						{
							coord = sizeofArgCoordinate;
							isUnaryExpression = true;

							REQUIRE_PARSE(CExpression, rightSideExpr, ParseUnaryExpression);
							CHECK_RV_ASSIGN(outProduct, New<CUnaryExpression>(alloc, std::move(rightSideExpr), CUnaryOperator::kSizeOfExpr));
						}

						isUnaryExpression = true;
					}
				}
			}

			if (!isUnaryExpression)
				return ParsePostfixExpression(inOutCoordinate, outProduct, speculative);

			inOutCoordinate = coord;
			return ErrorCode::kOK;
		}

		ResultRV<bool> CCompiler::ParsePostfixExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative)
		{
			IAllocator *alloc = CCompiler::GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			REQUIRE_PARSE(CExpression, leftSide, ParsePostfixExpressionLeftSide);

			for (;;)
			{
				FileCoordinate failCoord = coord;

				bool succeeded = false;
				PEEK_TOKEN(opToken, opEndCoord);

				if (opToken.IsString("."))
				{
					coord = opEndCoord;
					SPECULATIVE_PARSE(CToken, identifier, ParseIdentifier);
					if (identifier)
					{
						CHECK_RV_ASSIGN(leftSide, New<CMemberExpression>(alloc, std::move(leftSide), std::move(identifier), false));
						succeeded = true;
					}
				}
				else if (opToken.IsString("->"))
				{
					coord = opEndCoord;
					SPECULATIVE_PARSE(CToken, identifier, ParseIdentifier);
					if (identifier)
					{
						CHECK_RV_ASSIGN(leftSide, New<CMemberExpression>(alloc, std::move(leftSide), std::move(identifier), true));
						succeeded = true;
					}
				}
				else if (opToken.IsString("["))
				{
					coord = opEndCoord;
					SPECULATIVE_PARSE(CExpression, indexer, ParseExpression);
					if (indexer)
					{
						PEEK_TOKEN(rbracketToken, rbracketEndCoord);
						if (rbracketToken.IsString("]"))
						{
							coord = rbracketEndCoord;
							CHECK_RV_ASSIGN(leftSide, New<CBinaryExpression>(alloc, std::move(leftSide), std::move(indexer), CBinaryOperator::kIndex));
							succeeded = true;
						}
					}
				}
				else if (opToken.IsString("("))
				{
					coord = opEndCoord;

					SPECULATIVE_PARSE(CArgumentExpressionList, argList, ParseArgumentExpressionList);

					PEEK_TOKEN(rparenToken, rparenEndCoord);
					if (rparenToken.IsString(")"))
					{
						coord = rparenEndCoord;
						CHECK_RV_ASSIGN(leftSide, New<CInvokeExpression>(alloc, std::move(leftSide), std::move(argList)));
						succeeded = true;
					}
				}
				else if (opToken.IsString("++"))
				{
					coord = opEndCoord;
					CHECK_RV_ASSIGN(leftSide, New<CUnaryExpression>(alloc, std::move(leftSide), CUnaryOperator::kPostIncrement));
					succeeded = true;
				}
				else if (opToken.IsString("--"))
				{
					coord = opEndCoord;
					CHECK_RV_ASSIGN(leftSide, New<CUnaryExpression>(alloc, std::move(leftSide), CUnaryOperator::kPostDecrement));
					succeeded = true;
				}					

				if (!succeeded)
				{
					coord = failCoord;
					break;
				}
			}

			outProduct = std::move(leftSide);

			inOutCoordinate = coord;
			return ErrorCode::kOK;
		}

		ResultRV<bool> CCompiler::ParsePostfixExpressionLeftSide(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative)
		{
			IAllocator *alloc = CCompiler::GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			bool isTypedInitializer = false;

			FileCoordinate backupCoord = coord;

			PEEK_TOKEN(lparenToken, lparenEndCoord);
			if (lparenToken.IsString("("))
			{
				coord = lparenEndCoord;

				SPECULATIVE_PARSE(CTypeName, typeName, ParseTypeName);
				if (typeName)
				{
					// This is not speculative at this point, as primary-expressions can not contain this sequence of tokens if typeName is valid.
					REQUIRE_PARSE(CInitializerList, initList, ParsePostfixExpressionInitializerListAfterTypeName);
					CHECK_RV_ASSIGN(outProduct, New<CTypedInitializerExpression>(alloc, std::move(typeName), std::move(initList)));
					isTypedInitializer = true;
				}
			}

			if (!isTypedInitializer)
				return ParsePrimaryExpression(inOutCoordinate, outProduct, speculative);

			inOutCoordinate = coord;
			return ErrorCode::kOK;
		}

		ResultRV<bool> CCompiler::ParsePrimaryExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative)
		{
			IAllocator *alloc = CCompiler::GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			PEEK_TOKEN_WITH_TYPE(peToken, peTokenEndCoord, tokenType);
			if (peToken.IsString("("))
			{
				coord = peTokenEndCoord;

				REQUIRE_PARSE(CExpression, expr, ParseExpression);
				EXPECT_TOKEN(")");

				outProduct = std::move(expr);
			}
			else if (tokenType == CLexer::TokenType::kIdentifier)
			{
				REQUIRE_PARSE(CToken, identifier, ParseIdentifier);
				CHECK_RV_ASSIGN(outProduct, New<CTokenExpression>(alloc, peToken, CTokenExpression::TokenExpressionSubtype::kIdentifier));
			}
			else if (tokenType == CLexer::TokenType::kNumber)
			{
				coord = peTokenEndCoord;

				CHECK_RV_ASSIGN(outProduct, New<CTokenExpression>(alloc, peToken, CTokenExpression::TokenExpressionSubtype::kNumber));
			}
			else if (tokenType == CLexer::TokenType::kCharSequence)
			{
				coord = peTokenEndCoord;

				CHECK_RV_ASSIGN(outProduct, New<CTokenExpression>(alloc, peToken, CTokenExpression::TokenExpressionSubtype::kCharSequence));
			}
			else
			{
				if (speculative)
					return false;

				ReportCompileError(CompilationErrorCode::kExpectedExpression, inOutCoordinate);
				return ErrorCode::kOperationFailed;
			}

			inOutCoordinate = coord;
			return ErrorCode::kOK;
		}

		ResultRV<bool> CCompiler::ParsePostfixExpressionInitializerListAfterTypeName(FileCoordinate &inOutCoordinate, CorePtr<CInitializerList> &outProduct, bool speculative)
		{
			IAllocator *alloc = CCompiler::GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			EXPECT_TOKEN(")");
			EXPECT_TOKEN("{");

			REQUIRE_PARSE(CInitializerList, initList, ParseInitializerList);

			PEEK_TOKEN(commaToken, commaEndCoord);
			if (commaToken.IsString(","))
				coord = commaEndCoord;

			EXPECT_TOKEN("}");

			inOutCoordinate = coord;
			return ErrorCode::kOK;
		}

		ResultRV<bool> CCompiler::ParseInitializerList(FileCoordinate &inOutCoordinate, CorePtr<CInitializerList> &outProduct, bool speculative)
		{
			IAllocator *alloc = CCompiler::GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			Vector<CorePtr<CDesignatableInitializer>> items(alloc);

			REQUIRE_PARSE(CDesignatableInitializer, firstDesigInit, ParseDesignatableInitializer);
			CHECK(items.Add(std::move(firstDesigInit)));

			for (;;)
			{
				FileCoordinate backupCoord = coord;

				bool isExtension = false;
				PEEK_TOKEN(commaToken, commaEndCoord);
				if (commaToken.IsString(","))
				{
					coord = commaEndCoord;
					SPECULATIVE_PARSE(CDesignatableInitializer, nextDesigInit, ParseDesignatableInitializer);
					if (nextDesigInit)
					{
						CHECK(items.Add(std::move(firstDesigInit)));
						isExtension = true;
					}
				}

				if (!isExtension)
				{
					coord = backupCoord;
					break;
				}
			}

			CHECK_RV(ArrayPtr<CorePtr<CDesignatableInitializer>>, flat, items.View().CloneTake(alloc));
			CHECK_RV_ASSIGN(outProduct, New<CInitializerList>(alloc, std::move(flat)));

			inOutCoordinate = coord;
			return ErrorCode::kOK;
		}

		ResultRV<bool> CCompiler::ParseDesignatableInitializer(FileCoordinate &inOutCoordinate, CorePtr<CDesignatableInitializer> &outProduct, bool speculative)
		{
			IAllocator *alloc = CCompiler::GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			SPECULATIVE_PARSE(CDesignation, designation, ParseDesignation);
			REQUIRE_PARSE(CInitializer, initializer, ParseInitializer);

			CHECK_RV_ASSIGN(outProduct, New<CDesignatableInitializer>(alloc, std::move(designation), std::move(initializer)));

			inOutCoordinate = coord;
			return ErrorCode::kOK;
		}

		ResultRV<bool> CCompiler::ParseDesignation(FileCoordinate &inOutCoordinate, CorePtr<CDesignation> &outProduct, bool speculative)
		{
			IAllocator *alloc = CCompiler::GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			REQUIRE_PARSE(CDesignatorList, designatorList, ParseDesignatorList);
			EXPECT_TOKEN("=");

			CHECK_RV_ASSIGN(outProduct, New<CDesignation>(alloc, std::move(designatorList)));

			inOutCoordinate = coord;
			return ErrorCode::kOK;
		}

		ResultRV<bool> CCompiler::ParseDesignatorList(FileCoordinate &inOutCoordinate, CorePtr<CDesignatorList> &outProduct, bool speculative)
		{
			IAllocator *alloc = CCompiler::GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			Vector<CorePtr<CDesignator>> designators(alloc);

			REQUIRE_PARSE(CDesignator, firstDesignator, ParseDesignator);
			CHECK(designators.Add(std::move(firstDesignator)));

			for (;;)
			{
				SPECULATIVE_PARSE(CDesignator, nextDesignator, ParseDesignator);
				if (nextDesignator == nullptr)
					break;

				CHECK(designators.Add(std::move(nextDesignator)));
			}

			CHECK_RV(ArrayPtr<CorePtr<CDesignator>>, flat, designators.View().CloneTake(alloc));
			CHECK_RV_ASSIGN(outProduct, New<CDesignatorList>(alloc, std::move(flat)));

			inOutCoordinate = coord;
			return ErrorCode::kOK;
		}

		ResultRV<bool> CCompiler::ParseDesignator(FileCoordinate &inOutCoordinate, CorePtr<CDesignator> &outProduct, bool speculative)
		{
			IAllocator *alloc = CCompiler::GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			PEEK_TOKEN(lbracketToken, lbracketEndCoord);
			if (lbracketToken.IsString("["))
			{
				coord = lbracketEndCoord;

				REQUIRE_PARSE(CExpression, expr, ParseConstantExpression);
				EXPECT_TOKEN("]");

				CHECK_RV_ASSIGN(outProduct, New<CDesignator>(alloc, std::move(expr)));
			}
			else
			{
				EXPECT_TOKEN(".");
				REQUIRE_PARSE(CToken, identifier, ParseIdentifier);
				CHECK_RV_ASSIGN(outProduct, New<CDesignator>(alloc, std::move(identifier)));
			}

			inOutCoordinate = coord;
			return ErrorCode::kOK;
		}

		ResultRV<bool> CCompiler::ParseInitializer(FileCoordinate &inOutCoordinate, CorePtr<CInitializer> &outProduct, bool speculative)
		{
			IAllocator *alloc = CCompiler::GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			PEEK_TOKEN(lbraceToken, lbraceEndCoord);
			if (lbraceToken.IsString("{"))
			{
				coord = lbraceEndCoord;

				REQUIRE_PARSE(CInitializerList, initList, ParseInitializerList);
				PEEK_TOKEN(commaToken, commaEndCoord);
				if (commaToken.IsString(","))
					coord = commaEndCoord;
				EXPECT_TOKEN("}");
				CHECK_RV_ASSIGN(outProduct, New<CInitializer>(alloc, std::move(initList)));
			}
			else
			{
				REQUIRE_PARSE(CExpression, expr, ParseAssignmentExpression);
				CHECK_RV_ASSIGN(outProduct, New<CInitializer>(alloc, std::move(expr)));
			}

			inOutCoordinate = coord;
			return ErrorCode::kOK;

		}

		ResultRV<bool> CCompiler::ParseArgumentExpressionList(FileCoordinate &inOutCoordinate, CorePtr<CArgumentExpressionList> &outProduct, bool speculative)
		{
			IAllocator *alloc = CCompiler::GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			Vector<CorePtr<CExpression>> exprs(alloc);
			REQUIRE_PARSE(CExpression, firstExpr, ParseAssignmentExpression);

			for (;;)
			{
				FileCoordinate backupCoord = coord;

				bool succeeded = false;
				PEEK_TOKEN(commaToken, commaEndCoord);
				if (commaToken.IsString(","))
				{
					coord = commaEndCoord;

					SPECULATIVE_PARSE(CExpression, nextExpr, ParseAssignmentExpression);
					if (nextExpr)
					{
						succeeded = true;
						CHECK(exprs.Add(std::move(nextExpr)));
					}
				}

				if (!succeeded)
				{
					coord = backupCoord;
					break;
				}
			}

			CHECK_RV(ArrayPtr<CorePtr<CExpression>>, flat, exprs.View().CloneTake(alloc));
			CHECK_RV_ASSIGN(outProduct, New<CArgumentExpressionList>(alloc, std::move(flat)));

			inOutCoordinate = coord;
			return ErrorCode::kOK;
		}

		ResultRV<bool> CCompiler::ParseTypedefName(FileCoordinate &inOutCoordinate, CorePtr<CToken> &outProduct, bool speculative)
		{
			IAllocator *alloc = CCompiler::GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			PEEK_TOKEN_WITH_TYPE(identifierToken, identifierEndCoord, tokenType);

			if (tokenType != CLexer::TokenType::kIdentifier)
			{
				if (speculative)
					return false;

				ReportCompileError(CompilationErrorCode::kExpectedTypeName, inOutCoordinate);
				return ErrorCode::kOperationFailed;
			}

			const CIdentifierBinding *identifierBinding = m_currentScope->GetSymbolRecursive(identifierToken);
			if (identifierBinding == nullptr || identifierBinding->GetBindingType() != CIdentifierBinding::BindingType::kTypeDef)
			{
				if (speculative)
					return false;

				ReportCompileError(CompilationErrorCode::kExpectedTypeName, inOutCoordinate);
				return ErrorCode::kOperationFailed;
			}

			coord = identifierEndCoord;

			CHECK_RV_ASSIGN(outProduct, New<CToken>(alloc, CGrammarElement::Subtype::kTypeDefNameSpecifier, identifierToken, inOutCoordinate));

			inOutCoordinate = coord;
			return ErrorCode::kOK;
		}

		ResultRV<bool> CCompiler::DynamicParseLTRBinaryExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative, BinOperatorResolver_t opResolverFunc, ExpressionParseFunc_t nextPriorityFunc)
		{
			IAllocator *alloc = CCompiler::GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			REQUIRE_PARSE(CExpression, leftSide, *nextPriorityFunc);

			for (;;)
			{
				FileCoordinate backupCoord = coord;

				bool succeeded = false;
				PEEK_TOKEN(operatorToken, operatorEndCoord);
				CBinaryOperator binOp = opResolverFunc(operatorToken);
				if (binOp != CBinaryOperator::kInvalid)
				{
					coord = operatorEndCoord;

					SPECULATIVE_PARSE(CExpression, nextExpr, *nextPriorityFunc);
					if (nextExpr)
					{
						CHECK_RV_ASSIGN(leftSide, New<CBinaryExpression>(alloc, std::move(leftSide), std::move(nextExpr), binOp));
						succeeded = true;
					}
				}

				if (!succeeded)
				{
					coord = backupCoord;
					break;
				}
			}

			outProduct = std::move(leftSide);

			inOutCoordinate = coord;
			return ErrorCode::kOK;
		}

		ResultRV<bool> CCompiler::ParseCastExpression(FileCoordinate &inOutCoordinate, CorePtr<CExpression> &outProduct, bool speculative)
		{
			IAllocator *alloc = CCompiler::GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			bool isCast = false;
			PEEK_TOKEN(lparenToken, lparenEndCoord);
			if (lparenToken.IsString("("))
			{
				coord = lparenEndCoord;

				SPECULATIVE_PARSE(CTypeName, typeName, ParseTypeName);
				if (typeName != nullptr)
				{
					PEEK_TOKEN(rparenToken, rparenEndCoord);
					if (lparenToken.IsString(")"))
					{
						coord = rparenEndCoord;

						SPECULATIVE_PARSE(CExpression, rightSideExpr, ParseCastExpression);
						if (rightSideExpr != nullptr)
						{
							isCast = true;
							CHECK_RV_ASSIGN(outProduct, New<CCastExpression>(alloc, std::move(typeName), std::move(rightSideExpr)));
						}
					}
				}
			}

			if (!isCast)
				return ParseUnaryExpression(inOutCoordinate, outProduct, speculative);

			inOutCoordinate = coord;
			return ErrorCode::kOK;
		}

		CBinaryOperator CCompiler::ResolveLogicalOrOperator(const TokenStrView &token)
		{
			if (token.IsString("||"))
				return CBinaryOperator::kLogicalOr;

			return CBinaryOperator::kInvalid;
		}

		CBinaryOperator CCompiler::ResolveLogicalAndOperator(const TokenStrView &token)
		{
			if (token.IsString("&&"))
				return CBinaryOperator::kLogicalAnd;

			return CBinaryOperator::kInvalid;
		}

		CBinaryOperator CCompiler::ResolveInclusiveOrOperator(const TokenStrView &token)
		{
			if (token.IsString("|"))
				return CBinaryOperator::kBitOr;

			return CBinaryOperator::kInvalid;
		}

		CBinaryOperator CCompiler::ResolveExclusiveOrOperator(const TokenStrView &token)
		{
			if (token.IsString("^"))
				return CBinaryOperator::kBitXor;

			return CBinaryOperator::kInvalid;
		}

		CBinaryOperator CCompiler::ResolveAndOperator(const TokenStrView &token)
		{
			if (token.IsString("&"))
				return CBinaryOperator::kBitAnd;

			return CBinaryOperator::kInvalid;
		}

		CBinaryOperator CCompiler::ResolveEqualityOperator(const TokenStrView &token)
		{
			if (token.IsString("=="))
				return CBinaryOperator::kEqual;
			if (token.IsString("!="))
				return CBinaryOperator::kNotEqual;

			return CBinaryOperator::kInvalid;
		}

		CBinaryOperator CCompiler::ResolveRelationalOperator(const TokenStrView &token)
		{
			if (token.IsString(">"))
				return CBinaryOperator::kGreater;
			if (token.IsString("<"))
				return CBinaryOperator::kLess;
			if (token.IsString(">="))
				return CBinaryOperator::kGreaterOrEqual;
			if (token.IsString("<="))
				return CBinaryOperator::kLessOrEqual;

			return CBinaryOperator::kInvalid;
		}

		CBinaryOperator CCompiler::ResolveShiftOperator(const TokenStrView &token)
		{
			if (token.IsString(">>"))
				return CBinaryOperator::kRsh;
			if (token.IsString("<<"))
				return CBinaryOperator::kLsh;

			return CBinaryOperator::kInvalid;
		}

		CBinaryOperator CCompiler::ResolveAdditiveOperator(const TokenStrView &token)
		{
			if (token.IsString("+"))
				return CBinaryOperator::kAdd;
			if (token.IsString("-"))
				return CBinaryOperator::kSub;

			return CBinaryOperator::kInvalid;
		}

		CBinaryOperator CCompiler::ResolveMultiplicativeOperator(const TokenStrView &token)
		{
			if (token.IsString("*"))
				return CBinaryOperator::kMul;
			if (token.IsString("/"))
				return CBinaryOperator::kDiv;
			if (token.IsString("%"))
				return CBinaryOperator::kMod;

			return CBinaryOperator::kInvalid;
		}

		CBinaryOperator CCompiler::ResolveCommaOperator(const TokenStrView &token)
		{
			if (token.IsString(","))
				return CBinaryOperator::kComma;

			return CBinaryOperator::kInvalid;
		}

		Result CCompiler::ParseAndCompileInitializerForDeclarator(CDeclarationSpecifiers *declSpecifiers, CDeclarator *declarator, FileCoordinate &inOutCoordinate)
		{
			EXP_ASSERT(false);
			return ErrorCode::kNotImplemented;
		}

		Result CCompiler::ParseAndCompileInitDeclaratorListEndingInSemi(CDeclarationSpecifiers *declSpecifiers, FileCoordinate &inOutCoordinate)
		{
			EXP_ASSERT(false);
			return ErrorCode::kNotImplemented;
		}

		Result CCompiler::CompileFunctionDefinitionAfterDeclarator(CDeclarationSpecifiers *declSpecifiers, CDeclarator *declarator, FileCoordinate &inOutCoordinate)
		{
			IAllocator *alloc = CCompiler::GetCoreObjectAllocator();

			FileCoordinate coord = inOutCoordinate;

			CHECK(CommitDeclarator(declSpecifiers, declarator));

			SPECULATIVE_PARSE(CDeclarationList, declList, ParseDeclarationList);

			EXP_ASSERT(false);

			inOutCoordinate = coord;
			return ErrorCode::kOK;
		}

		ResultRV<HTypeQualified> CCompiler::ResolveTypeDefName(const CToken &tokenElement)
		{
			const CIdentifierBinding *identBinding = m_currentScope->GetSymbolRecursive(tokenElement.GetToken());
			if (identBinding != nullptr)
			{
				const HTypeQualified *qtype = identBinding->GetTypeDef();
				if (qtype != nullptr)
					return *qtype;
			}

			ReportCompileError(CompilationErrorCode::kExpectedTypeName, tokenElement.GetCoordinate());
			return ErrorCode::kOperationFailed;
		}

		Result CCompiler::CompileAggregateDefinition(HAggregateDecl *aggDecl, const CStructDeclarationList *declList)
		{
			EXP_ASSERT(false);
			return ErrorCode::kNotImplemented;
		}

		Result CCompiler::CompileEnumDefinition(HEnumDecl *enumDecl, const CEnumeratorList *enumList)
		{
			EXP_ASSERT(false);
			return ErrorCode::kNotImplemented;
		}

		ResultRV<HAggregateDecl*> CCompiler::DeclareStructOrUnionInCurrentScope(const TokenStrView &name, CAggregateType aggType)
		{
			IAllocator *alloc = GetCoreObjectAllocator();
			CHECK_RV(CorePtr<HAggregateDecl>, aggDecl, New<HAggregateDecl>(alloc, aggType));

			HAggregateDecl *aggDeclPtr = aggDecl;

			if (m_currentScope == m_globalScope)
			{
				CHECK(m_globalInternedAggregates.Add(std::move(aggDecl)));
			}
			else
			{
				CHECK(m_tempInternedAggregates.Add(std::move(aggDecl)));
			}

			if (name.GetToken().Size() > 0)
			{
				CHECK(m_currentScope->AddTag(name, CTagBinding(aggDeclPtr)));
			}

			return aggDeclPtr;
		}

		ResultRV<HEnumDecl*> CCompiler::DeclareEnumInCurrentScope(const TokenStrView &name)
		{
			IAllocator *alloc = GetCoreObjectAllocator();
			CHECK_RV(CorePtr<HEnumDecl>, enumDecl, New<HEnumDecl>(alloc));

			HEnumDecl *enumDeclPtr = enumDecl;

			if (m_currentScope == m_globalScope)
			{
				CHECK(m_globalInternedEnums.Add(std::move(enumDecl)));
			}
			else
			{
				CHECK(m_tempInternedEnums.Add(std::move(enumDecl)));
			}

			if (name.GetToken().Size() > 0)
			{
				CHECK(m_currentScope->AddTag(name, CTagBinding(enumDeclPtr)));
			}

			return enumDeclPtr;
		}

		ResultRV<HTypeUnqualified> CCompiler::ResolveStructOrUnionSpecifier(const CStructOrUnionSpecifier &souSpecifierElement)
		{
			// A couple of situations here:
			// - Tag alone: Attempt to look up tag recursively.  If tag exists, tag type must match.
			// - Declaration: Anonymous struct
			// - Tag + Declaration: Attempt to declare in current namespace.

			const CToken *identifier = souSpecifierElement.GetIdentifier();
			const CStructDeclarationList *declList = souSpecifierElement.GetStructDeclarationList();

			if (identifier && !declList)
			{
				// Identifier with no definition
				const CTagBinding *tagBinding = m_currentScope->GetTagRecursive(identifier->GetToken());
				if (tagBinding == nullptr)
				{
					// Tag doesn't exist, declare it here
					CHECK_RV(HAggregateDecl*, aggDecl, DeclareStructOrUnionInCurrentScope(identifier->GetToken(), souSpecifierElement.GetAggregateType()));
					return HTypeUnqualified(HTypeAggregate(aggDecl));
				}
				else
				{
					// Tag does exist, enforce that it's the same type
					if (tagBinding->GetBindingType() == CTagBinding::BindingType::kAggregate)
					{
						HAggregateDecl *aggDecl = tagBinding->GetAggregateDecl();
						if (aggDecl->GetAggregateType() == souSpecifierElement.GetAggregateType())
							return HTypeUnqualified(HTypeAggregate(aggDecl));
					}

					ReportCompileError(CompilationErrorCode::kMismatchedTagType, identifier->GetCoordinate());
					return ErrorCode::kOperationFailed;
				}
			}
			else if (!identifier && declList)
			{
				// Definition with no identifier
				CHECK_RV(HAggregateDecl*, aggDecl, DeclareStructOrUnionInCurrentScope(TokenStrView(), souSpecifierElement.GetAggregateType()));
				CHECK(CompileAggregateDefinition(aggDecl, declList));

				return HTypeUnqualified(HTypeAggregate(aggDecl));
			}
			else if (identifier && declList)
			{
				// Identifier with definition.  Ensure that there isn't another of the same tag in the current scope and define it.
				const CTagBinding *currentScopeBinding = m_currentScope->GetTagLocal(identifier->GetToken());
				if (currentScopeBinding)
				{
					ReportCompileError(CompilationErrorCode::kDuplicateTag, identifier->GetCoordinate());
					return ErrorCode::kOperationFailed;
				}

				CHECK_RV(HAggregateDecl*, aggDecl, DeclareStructOrUnionInCurrentScope(identifier->GetToken(), souSpecifierElement.GetAggregateType()));
				CHECK(CompileAggregateDefinition(aggDecl, declList));

				return HTypeUnqualified(HTypeAggregate(aggDecl));
			}
			else
			{
				EXP_ASSERT(false);
				return ErrorCode::kInternalError;
			}
		}

		ResultRV<HTypeUnqualified> CCompiler::ResolveEnumSpecifier(const CEnumSpecifier &enumElement)
		{
			const CToken *identifier = enumElement.GetIdentifier();
			const CEnumeratorList *declList = enumElement.GetEnumeratorList();

			if (identifier && !declList)
			{
				// Identifier with no definition
				const CTagBinding *tagBinding = m_currentScope->GetTagRecursive(identifier->GetToken());
				if (tagBinding == nullptr)
				{
					// Tag doesn't exist, declare it here
					CHECK_RV(HEnumDecl*, enumDecl, DeclareEnumInCurrentScope(identifier->GetToken()));
					return HTypeUnqualified(HTypeEnum(enumDecl));
				}
				else
				{
					// Tag does exist, enforce that it's the same type
					if (tagBinding->GetBindingType() == CTagBinding::BindingType::kEnum)
					{
						HEnumDecl *enumDecl = tagBinding->GetEnumDecl();
						return HTypeUnqualified(HTypeEnum(enumDecl));
					}

					ReportCompileError(CompilationErrorCode::kMismatchedTagType, identifier->GetCoordinate());
					return ErrorCode::kOperationFailed;
				}
			}
			else if (!identifier && declList)
			{
				// Definition with no identifier
				CHECK_RV(HEnumDecl*, enumDecl, DeclareEnumInCurrentScope(TokenStrView()));
				CHECK(CompileEnumDefinition(enumDecl, declList));

				return HTypeUnqualified(HTypeEnum(enumDecl));
			}
			else if (identifier && declList)
			{
				// Identifier with definition.  Ensure that there isn't another of the same tag in the current scope and define it.
				const CTagBinding *currentScopeBinding = m_currentScope->GetTagLocal(identifier->GetToken());
				if (currentScopeBinding)
				{
					ReportCompileError(CompilationErrorCode::kDuplicateTag, identifier->GetCoordinate());
					return ErrorCode::kOperationFailed;
				}

				CHECK_RV(HEnumDecl*, enumDecl, DeclareEnumInCurrentScope(identifier->GetToken()));
				CHECK(CompileEnumDefinition(enumDecl, declList));

				return HTypeUnqualified(HTypeEnum(enumDecl));
			}
			else
			{
				EXP_ASSERT(false);
				return ErrorCode::kInternalError;
			}
		}

		Result CCompiler::ResolveDeclSpecifiers(const CDeclarationSpecifiers *declSpecifiers, HTypeQualifiers &outQualifiers, bool &outIsInline, Optional<HStorageClass> &outStorageClass, HTypeUnqualified &outUnqualifiedType)
		{
			HTypeQualifiers qualifiers;
			bool isRestrict = false;
			bool isVolatile = false;
			bool isInline = false;

			const int kSignedBit = 1;
			const int kUnsignedBit = 2;
			const int kShortBit = 4;
			const int kLongBit = 8;
			const int kLongLongBit = 16;
			const int kComplexBit = 32;

			const int kVoidBit = 64;
			const int kCharBit = 128;
			const int kIntBit = 256;
			const int kFloatBit = 512;
			const int kDoubleBit = 1024;
			const int kBoolBit = 2048;

			const int validTypeBitCombinations[] =
			{
				kVoidBit,
				kCharBit,
				(kSignedBit | kCharBit),
				(kUnsignedBit | kCharBit),
				kShortBit, (kSignedBit | kShortBit), (kShortBit | kIntBit), (kSignedBit | kShortBit | kIntBit),
				(kUnsignedBit | kShortBit), (kUnsignedBit | kShortBit | kIntBit),
				kIntBit, kSignedBit, (kSignedBit | kIntBit),
				kUnsignedBit, (kUnsignedBit | kIntBit),
				kLongBit, (kSignedBit | kLongBit), (kLongBit | kIntBit), (kSignedBit | kLongBit | kIntBit),
				(kUnsignedBit | kLongBit), (kUnsignedBit | kLongBit | kIntBit),
				kLongLongBit, (kSignedBit | kLongLongBit), (kLongLongBit | kIntBit), (kSignedBit | kLongLongBit | kIntBit),
				(kUnsignedBit | kLongLongBit) | (kUnsignedBit | kLongLongBit | kIntBit),
				kFloatBit,
				kDoubleBit,
				(kLongBit | kDoubleBit),
				kBoolBit,
				(kFloatBit | kComplexBit),
				(kDoubleBit | kComplexBit),
				(kLongBit | kDoubleBit | kComplexBit),
			};

			const size_t numValidTypeBitCombinations = sizeof(validTypeBitCombinations) / sizeof(validTypeBitCombinations[0]);

			int declSpecQualifiers = 0;

			Optional<HTypeUnqualified> unqualifiedType;

			// Contains a list of mixed kStorageClassSpecifier, kTypeQualifier, kFunctionSpecifier, kTypeSpecifier, kTypeDefNameSpecifier, kStructOrUnionSpecifier, kEnumSpecifier
			for (const CGrammarElement *element : declSpecifiers->GetChildren())
			{
				switch (element->GetSubtype())
				{
				case CGrammarElement::Subtype::kStorageClassSpecifier:
					{
						const CToken *tokenElement = static_cast<const CToken*>(element);
						const TokenStrView token = static_cast<const CToken*>(element)->GetToken();

						HStorageClass storageClass = HStorageClass::kInvalid;
						if (token.IsString("typedef"))
							storageClass = HStorageClass::kTypeDef;
						else if (token.IsString("extern"))
							storageClass = HStorageClass::kExtern;
						else if (token.IsString("static"))
							storageClass = HStorageClass::kStatic;
						else if (token.IsString("auto"))
							storageClass = HStorageClass::kAuto;
						else if (token.IsString("register"))
							storageClass = HStorageClass::kRegister;
						else
						{
							EXP_ASSERT(false);
							return ErrorCode::kInternalError;
						}

						if (outStorageClass.IsSet())
						{
							if (outStorageClass.Get() == storageClass)
								this->ReportCompileWarning(CompilationWarningCode::kStorageClassSpecifiedMultipleTimes, tokenElement->GetCoordinate());
							else
							{
								this->ReportCompileError(CompilationErrorCode::kDeclaratorMultipleStorageClasses, tokenElement->GetCoordinate());
								return ErrorCode::kOperationFailed;
							}

							outStorageClass = storageClass;
						}
					}
					break;
				case CGrammarElement::Subtype::kTypeDefNameSpecifier:
					{
						const CToken *tokenElement = static_cast<const CToken*>(element);

						if (declSpecQualifiers != 0 || unqualifiedType.IsSet())
						{
							this->ReportCompileError(CompilationErrorCode::kInvalidTypeSpecifier, tokenElement->GetCoordinate());
							return ErrorCode::kOperationFailed;
						}

						CHECK_RV(HTypeQualified, qType, ResolveTypeDefName(*tokenElement));
						unqualifiedType = qType.GetUnqualified();
						qualifiers |= qType.GetQualifiers();
					}
					break;
				case CGrammarElement::Subtype::kTypeSpecifier:
					{
						const CToken *tokenElement = static_cast<const CToken*>(element);
						const TokenStrView token = static_cast<const CToken*>(element)->GetToken();

						int invalidQualifiersMask = 0;
						int newBit = 0;
						if (token.IsString("void"))
							newBit = kVoidBit;
						else if (token.IsString("char"))
							newBit = kCharBit;
						else if (token.IsString("short"))
							newBit = kShortBit;
						else if (token.IsString("int"))
							newBit = kIntBit;
						else if (token.IsString("long"))
						{
							if ((declSpecQualifiers & kLongBit) == 0)
								newBit = kLongLongBit;
							else
								newBit = kLongBit;
						}
						else if (token.IsString("float"))
							newBit = kFloatBit;
						else if (token.IsString("double"))
							newBit = kDoubleBit;
						else if (token.IsString("signed"))
							newBit = kSignedBit;
						else if (token.IsString("unsigned"))
							newBit = kUnsignedBit;
						else if (token.IsString("_Bool"))
							newBit = kBoolBit;
						else if (token.IsString("float"))
							newBit = kFloatBit;
						else if (token.IsString("double"))
							newBit = kDoubleBit;
						else if (token.IsString("_Complex"))
							newBit = kComplexBit;
						else
						{
							EXP_ASSERT(false);
							return ErrorCode::kInternalError;
						}

						if (declSpecQualifiers & newBit)
						{
							this->ReportCompileError(CompilationErrorCode::kDuplicateTypeSpecifier, tokenElement->GetCoordinate());
							return ErrorCode::kOperationFailed;
						}

						declSpecQualifiers |= newBit;

						bool someTypeValid = false;
						for (size_t i = 0; i < numValidTypeBitCombinations; i++)
						{
							if ((declSpecQualifiers & validTypeBitCombinations[i]) == declSpecQualifiers)
							{
								someTypeValid = true;
								break;
							}
						}

						if (!someTypeValid)
						{
							this->ReportCompileError(CompilationErrorCode::kInvalidTypeSpecifierCombination, tokenElement->GetCoordinate());
							return ErrorCode::kOperationFailed;
						}
					}
					break;
				case CGrammarElement::Subtype::kStructOrUnionSpecifier:
					{
						const CStructOrUnionSpecifier *souSpecifierElement = static_cast<const CStructOrUnionSpecifier*>(element);

						if (declSpecQualifiers != 0 || unqualifiedType.IsSet())
						{
							this->ReportCompileError(CompilationErrorCode::kInvalidTypeSpecifier, souSpecifierElement->GetCoordinate());
							return ErrorCode::kOperationFailed;
						}

						CHECK_RV(HTypeUnqualified, uqType, ResolveStructOrUnionSpecifier(*souSpecifierElement));
						unqualifiedType = uqType;
					}
					break;
				case CGrammarElement::Subtype::kEnumSpecifier:
					{
						const CEnumSpecifier *enumSpecifierElement = static_cast<const CEnumSpecifier*>(element);

						if (declSpecQualifiers != 0 || unqualifiedType.IsSet())
						{
							this->ReportCompileError(CompilationErrorCode::kInvalidTypeSpecifier, enumSpecifierElement->GetCoordinate());
							return ErrorCode::kOperationFailed;
						}

						CHECK_RV(HTypeUnqualified, uqType, ResolveEnumSpecifier(*enumSpecifierElement));
						unqualifiedType = uqType;
					}
					break;
				case CGrammarElement::Subtype::kTypeQualifier:
					{
						const CToken *tokenElement = static_cast<const CToken*>(element);
						const TokenStrView token = static_cast<const CToken*>(element)->GetToken();

						if (token.IsString("const"))
							qualifiers.m_isConst = true;
						else if (token.IsString("restrict"))
							qualifiers.m_isRestrict = true;
						else if (token.IsString("volatile"))
							qualifiers.m_isVolatile = true;
						else
						{
							EXP_ASSERT(false);
							return ErrorCode::kInternalError;
						}
					}
					break;
				case CGrammarElement::Subtype::kFunctionSpecifier:
					{
						const CToken *tokenElement = static_cast<const CToken*>(element);
						const TokenStrView token = static_cast<const CToken*>(element)->GetToken();

						if (token.IsString("inline"))
							isInline = true;
						else
						{
							EXP_ASSERT(false);
							return ErrorCode::kInternalError;
						}
					}
					break;
				default:
					EXP_ASSERT(false);
					return ErrorCode::kInternalError;
				}
			}

			if (!unqualifiedType.IsSet())
			{
				switch (declSpecQualifiers)
				{
				case 0:
					unqualifiedType = HTypeUnqualified(HTypeIntegral(HTypeIntegral::IntegralType::kInt, false));
					break;
				case kVoidBit:
					unqualifiedType = HTypeUnqualified(HTypeVoid());
					break;
				case kCharBit:
					unqualifiedType = HTypeUnqualified(HTypeIntegral(HTypeIntegral::IntegralType::kChar, m_config.m_plainCharIsUnsigned));
					break;
				case (kSignedBit | kCharBit):
					unqualifiedType = HTypeUnqualified(HTypeIntegral(HTypeIntegral::IntegralType::kChar, false));
					break;
				case (kUnsignedBit | kCharBit):
					unqualifiedType = HTypeUnqualified(HTypeIntegral(HTypeIntegral::IntegralType::kChar, true));
					break;
				case kShortBit:
				case (kSignedBit | kShortBit):
				case (kShortBit | kIntBit):
				case (kSignedBit | kShortBit | kIntBit):
					unqualifiedType = HTypeUnqualified(HTypeIntegral(HTypeIntegral::IntegralType::kShort, false));
					break;
				case (kUnsignedBit | kShortBit):
				case (kUnsignedBit | kShortBit | kIntBit):
					unqualifiedType = HTypeUnqualified(HTypeIntegral(HTypeIntegral::IntegralType::kShort, true));
					break;
				case kIntBit:
				case kSignedBit:
				case (kSignedBit | kIntBit):
					unqualifiedType = HTypeUnqualified(HTypeIntegral(HTypeIntegral::IntegralType::kInt, false));
					break;
				case kUnsignedBit:
				case (kUnsignedBit | kIntBit):
					unqualifiedType = HTypeUnqualified(HTypeIntegral(HTypeIntegral::IntegralType::kInt, true));
					break;
				case kLongBit:
				case (kSignedBit | kLongBit):
				case (kLongBit | kIntBit):
				case (kSignedBit | kLongBit | kIntBit):
					unqualifiedType = HTypeUnqualified(HTypeIntegral(HTypeIntegral::IntegralType::kLongInt, false));
					break;

				case (kUnsignedBit | kLongBit):
				case (kUnsignedBit | kLongBit | kIntBit):
					unqualifiedType = HTypeUnqualified(HTypeIntegral(HTypeIntegral::IntegralType::kLongInt, true));
					break;

				case kLongLongBit:
				case (kSignedBit | kLongLongBit):
				case (kLongLongBit | kIntBit):
				case (kSignedBit | kLongLongBit | kIntBit):
					unqualifiedType = HTypeUnqualified(HTypeIntegral(HTypeIntegral::IntegralType::kLongLongInt, false));
					break;

				case (kUnsignedBit | kLongLongBit):
				case (kUnsignedBit | kLongLongBit | kIntBit):
					unqualifiedType = HTypeUnqualified(HTypeIntegral(HTypeIntegral::IntegralType::kLongLongInt, true));
					break;

				case kFloatBit:
					unqualifiedType = HTypeUnqualified(HTypeFloating(HTypeFloating::FloatingType::kSingle, HTypeFloating::ComplexityClass::kReal));
					break;

				case kDoubleBit:
					unqualifiedType = HTypeUnqualified(HTypeFloating(HTypeFloating::FloatingType::kDouble, HTypeFloating::ComplexityClass::kReal));
					break;

				case (kLongBit | kDoubleBit):
					unqualifiedType = HTypeUnqualified(HTypeFloating(HTypeFloating::FloatingType::kLongDouble, HTypeFloating::ComplexityClass::kReal));
					break;

				case kBoolBit:
					unqualifiedType = HTypeUnqualified(HTypeIntegral(HTypeIntegral::IntegralType::kBool, true));
					break;

				case (kFloatBit | kComplexBit):
					unqualifiedType = HTypeUnqualified(HTypeFloating(HTypeFloating::FloatingType::kSingle, HTypeFloating::ComplexityClass::kComplex));
					break;

				case (kDoubleBit | kComplexBit):
					unqualifiedType = HTypeUnqualified(HTypeFloating(HTypeFloating::FloatingType::kDouble, HTypeFloating::ComplexityClass::kComplex));
					break;

				case (kLongBit | kDoubleBit | kComplexBit):
					unqualifiedType = HTypeUnqualified(HTypeFloating(HTypeFloating::FloatingType::kLongDouble, HTypeFloating::ComplexityClass::kComplex));
					break;

				default:
					EXP_ASSERT(false);
					return ErrorCode::kInternalError;
				}
			}

			EXP_ASSERT(unqualifiedType.IsSet());

			outQualifiers = qualifiers;
			outIsInline = isInline;
			outUnqualifiedType = unqualifiedType.Get();

			return ErrorCode::kOK;
		}

		ResultRV<HTypeUnqualified> CCompiler::ResolvePointer(const CPointer &pointer, const HTypeQualified &innerType)
		{
			HTypeQualified result = innerType;
			for (const CPointer::IndirectionLevel &indirLevel : pointer.GetIndirections())
			{
				const CTypeQualifierList *qualList = indirLevel.m_optTypeQualifierList;
				if (qualList)
					result = HTypeQualified(result.GetUnqualified(), result.GetQualifiers() | ResolveQualifiers(*qualList));

				CHECK_RV(HTypeUnqualifiedInterned*, interned, InternType(result.GetUnqualified()));
				result = HTypeQualified(HTypePointer(interned, result.GetQualifiers()), HTypeQualifiers());
			}

			return result.GetUnqualified();
		}

		HTypeQualifiers CCompiler::ResolveQualifiers(const CTypeQualifierList &qualList)
		{
			HTypeQualifiers qualifiers;
			for (const CToken *tokenElement : qualList.GetChildren())
			{
				const TokenStrView token = tokenElement->GetToken();
				if (token.IsString("const"))
					qualifiers.m_isConst = true;
				else if (token.IsString("volatile"))
					qualifiers.m_isVolatile = true;
				else if (token.IsString("restrict"))
					qualifiers.m_isRestrict = true;
				else
				{
					EXP_ASSERT(false);
				}
			}

			return qualifiers;
		}

		Result CCompiler::ResolveDeclarator(const CDeclarationSpecifiers *declSpecifiers, const CDeclarator *topDeclarator, Optional<HStorageClass> &outStorageClass, TokenStrView &outName, HTypeQualified &declType)
		{
			HTypeQualifiers qualifiers;
			bool isInline = false;
			HTypeUnqualified unqualified;
			CHECK(ResolveDeclSpecifiers(declSpecifiers, qualifiers, isInline, outStorageClass, unqualified));

			HTypeQualified result = HTypeQualified(unqualified, qualifiers);
			const CDirectDeclarator *ddec = nullptr;
			const CDeclarator *declarator = topDeclarator;

			for (;;)
			{
				if (declarator)
				{
					const CPointer *declPointer = declarator->GetOptPointer();
					if (declPointer)
					{
						CHECK_RV(HTypeUnqualified, ptrType, ResolvePointer(*declPointer, result));
						result = HTypeQualified(ptrType, HTypeQualifiers());
					}

					ddec = declarator->GetDirectDeclarator();
					declarator = nullptr;
				}

				EXP_ASSERT(ddec);

				CDirectDeclarator::DirectDeclaratorType form = ddec->GetDirectDeclType();

				switch (form)
				{
				case CDirectDeclarator::DirectDeclaratorType::kIdentifier:
					outName = ddec->GetIdentifier()->GetToken();
					declType = result;
					return ErrorCode::kOK;
				case CDirectDeclarator::DirectDeclaratorType::kParenDeclarator:
					declarator = ddec->GetDeclarator();
					ddec = nullptr;
					break;
				case CDirectDeclarator::DirectDeclaratorType::kDirectDeclaratorContinuation:
					{
						const CDirectDeclaratorContinuation *continuation = ddec->GetContinuation();

						switch (continuation->GetContinuationType())
						{
						case CDirectDeclaratorContinuation::ContinuationType::kIdentifierList:
							{
								size_t numParameters = 0;
								const CIdentifierList *identifierList = continuation->GetIdentifierList();

								if (identifierList != nullptr)
									numParameters = identifierList->GetChildren().Size();

								CHECK_RV(HTypeUnqualifiedInterned*, internedRV, InternType(result.GetUnqualified()));
								result = HTypeQualified(HTypeFunction(internedRV, result.GetQualifiers(), numParameters), HTypeQualifiers());
							}
							break;
						case CDirectDeclaratorContinuation::ContinuationType::kParamTypeList:
							EXP_ASSERT(false);
							return ErrorCode::kNotImplemented;
							break;
						case CDirectDeclaratorContinuation::ContinuationType::kSquareBracket:
							{
								if (continuation->GetTypeQualifierList() || continuation->HasAsterisk() || continuation->HasStatic())
								{
									ReportCompileError(CompilationErrorCode::kVariableSizeArrayedNotSupported, ddec->GetCoordinate());
									return ErrorCode::kOperationFailed;
								}

								EXP_ASSERT(false);
								return ErrorCode::kNotImplemented;
							}
							break;
						default:
							EXP_ASSERT(false);
							return ErrorCode::kInternalError;
						}

						ddec = ddec->GetNextDirectDeclarator();
					}
					break;
				default:
					EXP_ASSERT(false);
					return ErrorCode::kInternalError;
				}
			}

			EXP_ASSERT(false);
			return ErrorCode::kNotImplemented;
		}

		Result CCompiler::CommitDeclarator(CDeclarationSpecifiers *declSpecifiers, CDeclarator *declarator)
		{
			Optional<HStorageClass> storageClass;
			TokenStrView name;
			HTypeQualified declType;
			CHECK(ResolveDeclarator(declSpecifiers, declarator, storageClass, name, declType));

			if (storageClass.IsSet() && storageClass.Get() == HStorageClass::kTypeDef)
			{
				const CIdentifierBinding *identifier = m_currentScope->GetSymbolLocal(name);
				if (identifier == nullptr)
				{
					ReportCompileError(CompilationErrorCode::kSymbolRedefinition, declarator->GetCoordinate());
					return ErrorCode::kOperationFailed;
				}

				CHECK(m_currentScope->DefineIdentifier(name, CIdentifierBinding(declType)));

				return ErrorCode::kOK;
			}

			const bool isFileScope = (m_currentScope == m_globalScope);

			// Commits a declarator to the current scope and creates an identifier and possibly a reserved definition, as defined in 6.2 and 6.9.2
			// Linkages:
			//
			// "static" file scope declaration -> internal linkage
			// "extern" declaration:
			//     If prior declaration has internal or external linkage, inherit prior declaration.
			//     If no prior declaration, or prior declaration has no linkage, new declaration has external linkage.
			// Functions with no storage class specifier are "extern"
			// Objects with file scope are "extern"
			//
			// Objects with file scope, no initializer, and no storage class or "static" storage class are tentative definitions.
			//
			// The end result of this is that we need to resolve this as either a global object or a local object.
			// Global object references have one of two types: Speculative and real.
			//
			// Speculative objects are required for the following situation:
			// int func(void)
			// {
			//     extern int symbol;
			//     return symbol;
			// }
			// float symbol;
			// 
			// In this case, the two are the same object despite having different types
			const bool isFunctionDefinition = (declType.GetUnqualified().GetSubtype() == HTypeUnqualified::Subtype::kFunction);

			if (isFileScope)
			{
				if (storageClass.IsSet())
				{
					if (storageClass.Get() == HStorageClass::kAuto || storageClass.Get() == HStorageClass::kRegister)
					{
						ReportCompileError(CompilationErrorCode::kAutoOrRegisterNotAllowedOnExternalDeclaration, declarator->GetCoordinate());
						return ErrorCode::kOperationFailed;
					}
				}
			}

			// Scope determination rules:
			// struct, union, and enum tags: Current scope
			// 

			EXP_ASSERT(false);
			return ErrorCode::kNotImplemented;
		}


		bool CCompiler::GetToken(TokenStrView &token, FileCoordinate &coord, CLexer::TokenType &tokenType)
		{
			if (m_lastParseCoord.IsSet() && m_lastParseCoord.Get() == coord)
			{
				token = m_lastToken;
				coord = m_lastEndCoord;
				tokenType = m_lastTokenType;
				return true;
			}

			FileCoordinate newCoord;
			ArrayView<const uint8_t> newToken;
			CLexer::TokenType newTokenType = CLexer::TokenType::kInvalid;
			const bool hasToken = CLexer::TryGetToken(m_contents.ConstView(), coord, false, false, m_tracer, m_errorReporter, true, true, newToken, newTokenType, newCoord);
			if (hasToken)
			{
				m_lastParseCoord = coord;
				m_lastToken = TokenStrView(newToken);
				m_lastEndCoord = newCoord;
				m_lastTokenType = newTokenType;

				coord = newCoord;
				token = TokenStrView(newToken);;
				tokenType = newTokenType;
				return true;
			}

			return false;
		}


		void CCompiler::ReportCompileError(CompilationErrorCode errorCode, const FileCoordinate &coord)
		{
		}

		void CCompiler::ReportCompileWarning(CompilationWarningCode warningCode, const FileCoordinate &coord)
		{
		}

		ResultRV<HTypeUnqualifiedInterned*> CCompiler::InternType(const HTypeUnqualified &t)
		{
			const bool isTemporary = (m_currentScope != m_globalScope);

			if (isTemporary)
			{
				HashMapIterator<HTypeUnqualified, CorePtr<HTypeUnqualifiedInterned>> it = m_globalInternedTypes.Find(t);
				if (it != m_globalInternedTypes.end())
					return it.Value().Get();
			}

			HashMap<HTypeUnqualified, CorePtr<HTypeUnqualifiedInterned>> &hashMap = (isTemporary ? m_tempInternedTypes : m_globalInternedTypes);
			HashMapIterator<HTypeUnqualified, CorePtr<HTypeUnqualifiedInterned>> it = hashMap.Find(t);
			if (it == hashMap.end())
			{
				CHECK_RV(CorePtr<HTypeUnqualifiedInterned>, interned, New<HTypeUnqualifiedInterned>(GetCoreObjectAllocator(), t));
				HTypeUnqualifiedInterned *result = interned;
				CHECK(hashMap.Insert(t, std::move(interned)));
				return result;
			}
			else
				return it.Value().Get();
		}

		Result CCompiler::InitGlobalScope()
		{
			return ErrorCode::kOK;
		}

		bool CCompiler::IsKeyword(TokenStrView &token)
		{
			return token.IsString("auto") || token.IsString("enum") || token.IsString("restrict") || token.IsString("unsigned") ||
				token.IsString("break") || token.IsString("extern") || token.IsString("return") || token.IsString("void") ||
				token.IsString("case") || token.IsString("float") || token.IsString("short") || token.IsString("volatile") ||
				token.IsString("char") || token.IsString("for") || token.IsString("signed") || token.IsString("while") ||
				token.IsString("const") || token.IsString("goto") || token.IsString("sizeof") || token.IsString("_Bool") ||
				token.IsString("continue") || token.IsString("if") || token.IsString("static") || token.IsString("_Complex") ||
				token.IsString("default") || token.IsString("inline") || token.IsString("struct") || token.IsString("_Imaginary") ||
				token.IsString("do") || token.IsString("int") || token.IsString("switch") ||
				token.IsString("double") || token.IsString("long") || token.IsString("else") ||
				token.IsString("register") || token.IsString("union") || token.IsString("typedef");
		}

		CCompiler::TemporaryScope::TemporaryScope(CCompiler *compiler)
			: m_compiler(compiler)
		{
		}

		CCompiler::TemporaryScope::~TemporaryScope()
		{
			if (m_scope != nullptr)
				m_compiler->m_currentScope = m_scope->GetParentScope();
		}

		Result CCompiler::TemporaryScope::CreateScope()
		{
			IAllocator *alloc = m_compiler->GetCoreObjectAllocator();

			CHECK_RV(CorePtr<CScope>, scope, New<CScope>(alloc, alloc, m_compiler->m_currentScope));
			m_compiler->m_currentScope = scope;

			m_scope = std::move(scope);

			return ErrorCode::kOK;
		}
	}
}
