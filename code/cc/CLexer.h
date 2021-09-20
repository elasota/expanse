#pragma once

#include "FileCoordinate.h"

#include <cstdint>

namespace expanse
{
	template<class T> struct ArrayView;

	namespace cc
	{
		struct IIncludeStackTrace;
		struct IErrorReporter;

		class CLexer
		{
		public:
			enum class TokenType
			{
				kWhitespace,
				kIdentifier,
				kPunctuation,
				kCharSequence,
				kInvalid,
				kNewLine,
				kNumber,
				kComment,
				kEndOfFile,

				kPPNumber,
				kPPHeaderName,
			};

			enum class CharacterType : uint8_t
			{
				kControl,
				kWhitespace,
				kCarriageReturn,
				kLineFeed,
				kPunctuation,
				kUnicode,
				kDigit,
				kLetter,
				kUnderscore,
			};

			static bool TryGetToken(ArrayView<const uint8_t> contents, const FileCoordinate &coordinate, bool preprocessorRules, bool headerNamePermitted, IIncludeStackTrace &includeStackTrace, IErrorReporter *errorReporter, bool ignoreWhitespace, bool newLineIsWhitespace, ArrayView<const uint8_t> &outToken, TokenType &outTokenType, FileCoordinate &outCoordinate);

		private:
			static bool TryGetToken(ArrayView<const uint8_t> contents, const FileCoordinate &coordinate, bool preprocessorRules, bool headerNamePermitted, IIncludeStackTrace &includeStackTrace, IErrorReporter *errorReporter, TokenType &outTokenType, FileCoordinate &outCoordinate);
			static bool TryGetIdentifier(ArrayView<const uint8_t> contents, const FileCoordinate &coordinate, IIncludeStackTrace &includeStackTrace, IErrorReporter *errorReporter, FileCoordinate &outCoordinate);
			static bool TryGetPPNumber(ArrayView<const uint8_t> contents, const FileCoordinate &coordinate, IIncludeStackTrace &includeStackTrace, IErrorReporter *errorReporter, FileCoordinate &outCoordinate);
			static bool TryGetCNumber(ArrayView<const uint8_t> contents, const FileCoordinate &coordinate, IIncludeStackTrace &includeStackTrace, IErrorReporter *errorReporter, FileCoordinate &outCoordinate);
			static bool TryGetUniversalCharacterCode(ArrayView<const uint8_t> contents, const FileCoordinate &coordinate, IIncludeStackTrace &includeStackTrace, IErrorReporter *errorReporter, FileCoordinate &outCoordinate);
			static bool TryGetOctalNumber(ArrayView<const uint8_t> contents, FileCoordinate &coordinate);
			static bool TryGetHexNumber(ArrayView<const uint8_t> contents, FileCoordinate &coordinate);
			static bool TryGetDigitSequence(ArrayView<const uint8_t> contents, FileCoordinate &inOutCoordinate);
			static bool TryGetExponentPart(ArrayView<const uint8_t> contents, FileCoordinate &inOutCoordinate);
			static bool TryGetFloatingSuffix(ArrayView<const uint8_t> contents, FileCoordinate &inOutCoordinate);
			static bool TryGetIntegerSuffix(ArrayView<const uint8_t> contents, FileCoordinate &inOutCoordinate);
			static bool TryGetUnsignedSuffix(ArrayView<const uint8_t> contents, FileCoordinate &inOutCoordinate);
			static bool TryGetLongOrLongLongSuffix(ArrayView<const uint8_t> contents, FileCoordinate &inOutCoordinate);
			static bool TryGetHexDigitSequence(ArrayView<const uint8_t> contents, FileCoordinate &inOutCoordinate);
			static bool TryGetBinaryExponentPart(ArrayView<const uint8_t> contents, FileCoordinate &inOutCoordinate);
			static bool TryGetHeaderName(ArrayView<const uint8_t> contents, FileCoordinate &inOutCoordinate);

			static bool TryGetSingle(ArrayView<const uint8_t> contents, FileCoordinate &inOutCoordinate, const uint8_t charCode);
			static bool TryGetSingleInSet(ArrayView<const uint8_t> contents, FileCoordinate &inOutCoordinate, const uint8_t *charCodes);
			static bool TryGetMultipleInSet(ArrayView<const uint8_t> contents, FileCoordinate &inOutCoordinate, const uint8_t *charCodes);
			static bool TryGetMultipleMatchingCallback(ArrayView<const uint8_t> contents, FileCoordinate &inOutCoordinate, bool (*callback)(uint8_t charCode));

			static bool IsHexDigit(uint8_t charCode);
			static bool IsOctalDigit(uint8_t charCode);
			static bool IsDigit(uint8_t charCode);

			static CharacterType CategorizeCharacter(uint8_t byte);
			static uint8_t ConsumeLogicalChar(const ArrayView<const uint8_t> &contents, FileCoordinate &coord);

			static CharacterType ms_lowCharTypes[128];
		};
	}
}
