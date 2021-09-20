#pragma once

namespace expanse
{
	namespace cc
	{
		struct FileCoordinate;
		struct IIncludeStackTrace;

		enum class ErrorSeverity
		{
			kWarning,
			kError,
		};

		enum class CompilationErrorCode
		{
			kUnknownError,

			kEndOfFileInCharacterSequence,
			kNewlineInCharacterSequence,
			kInvalidEscapeSequence,
			kInvalidCharacter,
			kInvalidIdentifier,
			kUnknownDirective,
			kInvalidIncludePath,
			kIncludePathEscapesDirectoryTree,
			kIncludeNotFound,

			kPPElifOutsideOfLogic,
			kPPElifAfterElse,
			kPPElseOutsideOfLogic,
			kPPElseAfterElse,
			kPPExpectedNewLineAfterElse,
			kPPEndIfOutsideOfLogic,
			kPPExpectedNewLineAfterEndIf,

			kExpectedExternalDeclaration,
			kExpectedDeclarationSpecifiers,
			kExpectedRightParenAfterDeclarator,
			kExpectedIdentifierOrOpenBrace,
			kExpectedIdentifier,
			kExpectedExpression,
			kExpectedTypeName,

			kDeclaratorMultipleStorageClasses,
			kInvalidTypeSpecifier,
			kDuplicateTypeSpecifier,
			kInvalidTypeSpecifierCombination,
			kMismatchedTagType,
			kDuplicateTag,
			kVariableSizeArrayedNotSupported,
			kAutoOrRegisterNotAllowedOnExternalDeclaration,
			kSymbolRedefinition,

			kUnexpectedEndOfFile,
			kUnexpectedToken,
		};

		enum class CompilationWarningCode
		{
			kUnknownWarning,

			kEmptyDeclaration,
			kStorageClassSpecifiedMultipleTimes,
		};

		struct IErrorReporter
		{
			virtual void ReportError(const FileCoordinate &fileCoordinate, IIncludeStackTrace &includeStackTrace, CompilationErrorCode errorCode) = 0;
		};
	}
}
