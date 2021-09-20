#include "CLexer.h"

#include "ArrayView.h"
#include "CharCodes.h"
#include "IErrorReporter.h"

namespace expanse
{
	namespace cc
	{
		bool CLexer::TryGetToken(ArrayView<const uint8_t> contents, const FileCoordinate &inCoordinate, bool preprocessorRules, bool headerNamePermitted, IIncludeStackTrace &includeStackTrace, IErrorReporter *errorReporter, bool ignoreWhitespace, bool newLineIsWhitespace, ArrayView<const uint8_t> &outToken, TokenType &outTokenType, FileCoordinate &outCoordinate)
		{
			if (ignoreWhitespace)
			{
				FileCoordinate startCoordinate = inCoordinate;

				TokenType firstTokenType = TokenType::kInvalid;
				FileCoordinate firstTokenEndCoordinate;
				const bool haveToken = TryGetToken(contents, startCoordinate, preprocessorRules, headerNamePermitted, includeStackTrace, errorReporter, false, newLineIsWhitespace, outToken, firstTokenType, firstTokenEndCoordinate);
				if (!haveToken)
					return false;


				if (firstTokenType == TokenType::kWhitespace)
					return TryGetToken(contents, firstTokenEndCoordinate, preprocessorRules, headerNamePermitted, includeStackTrace, errorReporter, false, newLineIsWhitespace, outToken, outTokenType, outCoordinate);
				else
				{
					outCoordinate = firstTokenEndCoordinate;
					outTokenType = firstTokenType;
					return true;
				}
			}

			// Coalesce whitespace tokens
			FileCoordinate endCoordinate = inCoordinate;
			TokenType firstTokenType = TokenType::kInvalid;

			bool isFirstToken = false;
			for (;;)
			{
				FileCoordinate nextEndCoordinate;
				TokenType tokenType = TokenType::kInvalid;
				const bool haveToken = TryGetToken(contents, endCoordinate, preprocessorRules, headerNamePermitted, includeStackTrace, errorReporter, tokenType, nextEndCoordinate);
				if (!haveToken)
				{
					if (firstTokenType == TokenType::kInvalid)
						return false;
				}

				if (tokenType == TokenType::kComment || (tokenType == TokenType::kNewLine && newLineIsWhitespace))
					tokenType = TokenType::kWhitespace;

				if (firstTokenType == TokenType::kInvalid)
				{
					endCoordinate = nextEndCoordinate;
					firstTokenType = tokenType;
				}
				
				if (firstTokenType == TokenType::kWhitespace && tokenType == TokenType::kWhitespace)
				{
					endCoordinate = nextEndCoordinate;
					continue;
				}
				else
					break;
			}

			outToken = contents.Subrange(inCoordinate.m_fileOffset, endCoordinate.m_fileOffset - inCoordinate.m_fileOffset);
			outTokenType = firstTokenType;
			outCoordinate = endCoordinate;
			return true;
		}

		bool CLexer::TryGetToken(ArrayView<const uint8_t> contents, const FileCoordinate &inCoordinate, bool preprocessorRules, bool headerNamePermitted, IIncludeStackTrace &includeStackTrace, IErrorReporter *errorReporter, TokenType &outTokenType, FileCoordinate &outCoordinate)
		{
			TokenType tokenType = TokenType::kInvalid;
			FileCoordinate coord = inCoordinate;
			FileCoordinate prevCoord = coord;

			if (coord.m_fileOffset == contents.Size())
				return false;

			const uint8_t firstChar = ConsumeLogicalChar(contents, coord);

			if (headerNamePermitted && (firstChar == CharCode::kLess || firstChar == CharCode::kDoubleQuote))
			{
				FileCoordinate ppHeaderCoord = inCoordinate;
				if (TryGetHeaderName(contents, ppHeaderCoord))
				{
					outTokenType = TokenType::kPPHeaderName;
					outCoordinate = ppHeaderCoord;
					return true;
				}
			}

			CharacterType firstCharType = CategorizeCharacter(firstChar);

			bool isCharSequence = false;
			uint8_t charSequenceTerminator = 0;
			if (firstChar == CharCode::kUppercaseL && (coord.m_fileOffset != contents.Size()))
			{
				FileCoordinate backupCoord = coord;
				const uint8_t secondChar = ConsumeLogicalChar(contents, coord);

				if (secondChar == CharCode::kSingleQuote || secondChar == CharCode::kDoubleQuote)
				{
					charSequenceTerminator = secondChar;
					isCharSequence = true;
				}
				else
					coord = backupCoord;
			}
			else if (firstChar == CharCode::kSingleQuote || firstChar == CharCode::kDoubleQuote)
			{
				isCharSequence = true;
				charSequenceTerminator = firstChar;
			}

			if (isCharSequence)
			{
				outTokenType = TokenType::kCharSequence;

				for (;;)
				{
					if (coord.m_fileOffset == contents.Size())
					{
						errorReporter->ReportError(prevCoord, includeStackTrace, CompilationErrorCode::kEndOfFileInCharacterSequence);
						outTokenType = TokenType::kInvalid;
						break;
					}

					prevCoord = coord;
					uint8_t nextChar = ConsumeLogicalChar(contents, coord);

					if (nextChar == CharCode::kLineFeed)
					{
						errorReporter->ReportError(prevCoord, includeStackTrace, CompilationErrorCode::kNewlineInCharacterSequence);
						outTokenType = TokenType::kInvalid;
						break;
					}
					else if (nextChar == charSequenceTerminator)
						break;
					else if (nextChar == CharCode::kBackslash)
					{
						// Escape sequence
						if (coord.m_fileOffset == contents.Size())
						{
							errorReporter->ReportError(prevCoord, includeStackTrace, CompilationErrorCode::kEndOfFileInCharacterSequence);
							outTokenType = TokenType::kInvalid;
							break;
						}

						nextChar = ConsumeLogicalChar(contents, coord);

						bool isSimpleEscape = false;
						bool isOctalEscape = false;
						bool isHexEscape = false;
						bool isInvalid = false;

						switch (nextChar)
						{
						case CharCode::kSingleQuote:
						case CharCode::kDoubleQuote:
						case CharCode::kQuestion:
						case CharCode::kBackslash:
						case CharCode::kLowercaseA:
						case CharCode::kLowercaseB:
						case CharCode::kLowercaseF:
						case CharCode::kLowercaseN:
						case CharCode::kLowercaseR:
						case CharCode::kLowercaseT:
						case CharCode::kLowercaseV:
							isSimpleEscape = true;
							break;
						case CharCode::kLowercaseX:
							isHexEscape = true;
							break;
						case CharCode::kDigit0:
						case CharCode::kDigit1:
						case CharCode::kDigit2:
						case CharCode::kDigit3:
						case CharCode::kDigit4:
						case CharCode::kDigit5:
						case CharCode::kDigit6:
						case CharCode::kDigit7:
							isOctalEscape = true;
							break;
						default:
							isInvalid = true;
							break;
						};

						if (isSimpleEscape)
						{
						}
						else if (isOctalEscape)
						{
							for (size_t i = 0; i < 2; i++)
							{
								if (coord.m_fileOffset == contents.Size())
									break;	// This should become invalid

								FileCoordinate backupCoord = coord;
								nextChar = ConsumeLogicalChar(contents, coord);

								if (IsOctalDigit(nextChar))
								{
									coord = backupCoord;
									break;
								}
							}
						}
						else if (isHexEscape)
						{
							for (;;)
							{
								if (coord.m_fileOffset == contents.Size())
									break;	// This should become invalid

								FileCoordinate backupCoord = coord;
								nextChar = ConsumeLogicalChar(contents, coord);

								if (!IsHexDigit(nextChar))
								{
									coord = backupCoord;
									break;
								}
							}
						}

						if (isInvalid)
						{
							errorReporter->ReportError(prevCoord, includeStackTrace, CompilationErrorCode::kInvalidEscapeSequence);
							outTokenType = TokenType::kInvalid;
							break;
						}
					}
					else
						continue;
				}
			}
			else
			{
				switch (firstCharType)
				{
				case CharacterType::kControl:
				case CharacterType::kWhitespace:
					outTokenType = TokenType::kWhitespace;
					break;

				case CharacterType::kCarriageReturn:
				case CharacterType::kLineFeed:
					outTokenType = TokenType::kNewLine;
					break;

				case CharacterType::kPunctuation:
					{
						if (firstChar == CharCode::kBackslash)
						{
							if (TryGetIdentifier(contents, inCoordinate, includeStackTrace, errorReporter, coord))
								outTokenType = TokenType::kIdentifier;
							else
								outTokenType = TokenType::kPunctuation;
						}
						else if (firstChar == CharCode::kPeriod)
						{
							outTokenType = TokenType::kPunctuation;
							if (coord.m_fileOffset != contents.Size())
							{
								const uint8_t secondChar = ConsumeLogicalChar(contents, coord);
								if (secondChar >= CharCode::kDigit0 && secondChar <= CharCode::kDigit9)
								{
									if (preprocessorRules && TryGetPPNumber(contents, inCoordinate, includeStackTrace, errorReporter, coord))
										outTokenType = TokenType::kPPNumber;
									else if ((!preprocessorRules) && TryGetCNumber(contents, inCoordinate, includeStackTrace, errorReporter, coord))
										outTokenType = TokenType::kNumber;
								}
							}
						}
						else
						{
							outTokenType = TokenType::kPunctuation;
							if (firstChar == CharCode::kExclamation || firstChar == CharCode::kPercent || firstChar == CharCode::kAsterisk || firstChar == CharCode::kCaret)	// ! % * ^
							{
								if (coord.m_fileOffset != contents.Size())
								{
									FileCoordinate backupCoord = coord;
									const uint8_t secondChar = ConsumeLogicalChar(contents, coord);

									if (secondChar != CharCode::kEqual)	// != %= *= ^=
										coord = backupCoord;
								}
							}
							else if (firstChar == CharCode::kHash)	// #
							{
								if (coord.m_fileOffset != contents.Size())
								{
									FileCoordinate backupCoord = coord;
									const uint8_t secondChar = ConsumeLogicalChar(contents, coord);

									if (secondChar != CharCode::kHash)	// ##
										coord = backupCoord;
								}
							}
							else if (firstChar == CharCode::kMinus)		// -
							{
								if (coord.m_fileOffset != contents.Size())
								{
									FileCoordinate backupCoord = coord;
									const uint8_t secondChar = ConsumeLogicalChar(contents, coord);

									if (secondChar != CharCode::kMinus && secondChar != CharCode::kEqual && secondChar != CharCode::kGreater)	// --, -=, ->
										coord = backupCoord;
								}
							}
							else if (firstChar == CharCode::kAmpersand || firstChar == CharCode::kPlus || firstChar == CharCode::kLess || firstChar == CharCode::kGreater || firstChar == CharCode::kVerticalBar)		// & + < > |
							{
								if (coord.m_fileOffset != contents.Size())
								{
									FileCoordinate backupCoord = coord;
									const uint8_t secondChar = ConsumeLogicalChar(contents, coord);

									if (secondChar != firstChar && secondChar != CharCode::kEqual)	// &&, &=, ++, += <<, <=, >>, >=, ||, |=
										coord = backupCoord;
								}
							}
							else if (firstChar == CharCode::kPeriod)	// .
							{
								if (contents.Size() - coord.m_fileOffset >= 2)
								{
									FileCoordinate backupCoord = coord;
									const uint8_t secondChar = ConsumeLogicalChar(contents, coord);

									if (secondChar != CharCode::kPeriod)	// ..
										coord = backupCoord;
									else
									{
										const uint8_t thirdChar = ConsumeLogicalChar(contents, coord);
										if (thirdChar != CharCode::kPeriod)	// ...
											coord = backupCoord;
									}
								}
							}
							else if (firstChar == CharCode::kSlash)	// /
							{
								if (coord.m_fileOffset != contents.Size())
								{
									FileCoordinate backupCoord = coord;
									const uint8_t secondChar = ConsumeLogicalChar(contents, coord);

									if (secondChar == CharCode::kAsterisk)	// /*
									{
										outTokenType = TokenType::kComment;

										if (coord.m_fileOffset == contents.Size())
											outTokenType = TokenType::kInvalid;
										else
										{
											uint8_t prevChar = ConsumeLogicalChar(contents, coord);
											for (;;)
											{
												if (coord.m_fileOffset == contents.Size())
													outTokenType = TokenType::kInvalid;
												else
												{
													const uint8_t thisChar = ConsumeLogicalChar(contents, coord);
													if (prevChar == CharCode::kAsterisk && thisChar == CharCode::kSlash)
														break;

													prevChar = thisChar;
												}
											}
										}
									}
									else if (secondChar == CharCode::kSlash)	// //
									{
										outTokenType = TokenType::kComment;

										while (coord.m_fileOffset == contents.Size())
										{
											FileCoordinate backupCoord = coord;
											const uint8_t nextChar = ConsumeLogicalChar(contents, coord);
											if (nextChar == CharCode::kLineFeed)
											{
												coord = backupCoord;
												break;
											}
										}
									}
									else
										coord = backupCoord;
								}
							}
						}
					}
					break;
				case CharacterType::kDigit:
					{
						if (preprocessorRules)
						{
							(void)TryGetPPNumber(contents, inCoordinate, includeStackTrace, errorReporter, coord);
							outTokenType = TokenType::kPPNumber;
						}
						else
						{
							(void)TryGetCNumber(contents, inCoordinate, includeStackTrace, errorReporter, coord);
							outTokenType = TokenType::kNumber;
						}
					}
					break;
				case CharacterType::kLetter:
				case CharacterType::kUnderscore:
					(void)TryGetIdentifier(contents, inCoordinate, includeStackTrace, errorReporter, coord);
					outTokenType = TokenType::kIdentifier;
					break;
				default:
					errorReporter->ReportError(inCoordinate, includeStackTrace, CompilationErrorCode::kInvalidCharacter);
					outTokenType = TokenType::kInvalid;
					break;
				}
			}

			outCoordinate = coord;
			return true;
		}

		bool CLexer::TryGetIdentifier(ArrayView<const uint8_t> contents, const FileCoordinate &coordinate, IIncludeStackTrace &includeStackTrace, IErrorReporter *errorReporter, FileCoordinate &outCoordinate)
		{
			FileCoordinate coord = coordinate;

			bool isFirst = true;

			if (coord.m_fileOffset == contents.Size())
				return false;

			const uint8_t firstChar = ConsumeLogicalChar(contents, coord);
			if (firstChar >= CharCode::kDigit0 && firstChar <= CharCode::kDigit9)
				return false;

			// Rewind to start
			coord = coordinate;

			for (;;)
			{
				if (coord.m_fileOffset == contents.Size())
					break;

				FileCoordinate backupCoord = coord;
				const uint8_t thisChar = ConsumeLogicalChar(contents, coord);
				if (thisChar >= CharCode::kDigit0 && thisChar <= CharCode::kDigit9)
					continue;
				else if (thisChar >= CharCode::kLowercaseA && thisChar <= CharCode::kLowercaseZ)
					continue;
				else if (thisChar >= CharCode::kUppercaseA && thisChar <= CharCode::kUppercaseZ)
					continue;
				else if (thisChar == CharCode::kUnderscore)
					continue;
				else if (thisChar == CharCode::kBackslash)
				{
					if (TryGetUniversalCharacterCode(contents, backupCoord, includeStackTrace, errorReporter, coord))
						continue;
					else
					{
						coord = backupCoord;
						break;
					}
				}
				else
				{
					coord = backupCoord;
					break;
				}
			}

			outCoordinate = coord;
			return true;
		}

		bool CLexer::TryGetPPNumber(ArrayView<const uint8_t> contents, const FileCoordinate &coordinate, IIncludeStackTrace &includeStackTrace, IErrorReporter *errorReporter, FileCoordinate &outCoordinate)
		{
			// Parses a pp-number, the rules for which can be summarized as:
			// - It must start with a digit or a period followed by a digit
			// - After that, any digit, period, or identifier character in any sequence is legal
			// - A "+" or "-" character may be the last character if the preceding character is "e", "E", "p", or "P"
			FileCoordinate coord = coordinate;

			if (coord.m_fileOffset == contents.Size())
				return false;

			uint8_t thisChar = ConsumeLogicalChar(contents, coord);

			if (thisChar >= CharCode::kDigit0 && thisChar <= CharCode::kDigit9)
			{
			}
			else if (thisChar == CharCode::kPeriod)
			{
				if (coord.m_fileOffset == contents.Size())
					return false;

				thisChar = ConsumeLogicalChar(contents, coord);
				if (thisChar >= CharCode::kDigit0 && thisChar <= CharCode::kDigit9)
				{
				}
				else
					return false;
			}
			else
				return false;

			uint8_t prevChar = thisChar;
			for (;;)
			{
				if (coord.m_fileOffset == contents.Size())
					break;

				FileCoordinate backupCoord = coord;

				prevChar = thisChar;
				thisChar = ConsumeLogicalChar(contents, coord);

				if (thisChar >= CharCode::kDigit0 && thisChar <= CharCode::kDigit9)
					continue;
				else if (thisChar >= CharCode::kLowercaseA && thisChar <= CharCode::kLowercaseZ)
					continue;
				else if (thisChar >= CharCode::kUppercaseA && thisChar <= CharCode::kUppercaseZ)
					continue;
				else if (thisChar == CharCode::kUnderscore)
					continue;
				else if (thisChar == CharCode::kBackslash)
				{
					if (TryGetUniversalCharacterCode(contents, backupCoord, includeStackTrace, errorReporter, coord))
						continue;
					else
					{
						coord = backupCoord;
						break;
					}
				}
				else if (thisChar == CharCode::kPlus || thisChar == CharCode::kMinus)
				{
					if (prevChar != CharCode::kLowercaseE && prevChar != CharCode::kUppercaseE && prevChar != CharCode::kLowercaseP && prevChar != CharCode::kUppercaseP)
					{
						coord = backupCoord;
						break;
					}
				}
				else
				{
					coord = backupCoord;
					break;
				}
			}

			outCoordinate = coord;
			return true;
		}

		bool CLexer::TryGetCNumber(ArrayView<const uint8_t> contents, const FileCoordinate &coordinate, IIncludeStackTrace &includeStackTrace, IErrorReporter *errorReporter, FileCoordinate &outCoordinate)
		{
			// Parses an integer-constant or decimal-constant
			FileCoordinate coord = coordinate;

			if (coord.m_fileOffset == contents.Size())
				return false;

			uint8_t thisChar = ConsumeLogicalChar(contents, coord);

			if (thisChar == CharCode::kDigit0)
			{
				if (coord.m_fileOffset == contents.Size())
				{
					outCoordinate = coord;
					return true;
				}

				const uint8_t secondChar = ConsumeLogicalChar(contents, coord);
				if (secondChar == CharCode::kLowercaseX || secondChar == CharCode::kUppercaseX)
				{
					const bool succeeded = TryGetHexNumber(contents, coord);
					if (succeeded)
						outCoordinate = coord;
					return succeeded;
				}
				else
				{
					coord = coordinate;
					(void)TryGetOctalNumber(contents, coord);
					(void)TryGetIntegerSuffix(contents, coord);

					outCoordinate = coord;
					return true;
				}
			}

			// Possible sequences:
			//
			// integer-constant:
			//     digit-sequence [integer-suffix]
			// floating-constant:
			//     [digit-sequence] . digit-sequence [exponent-part] [floating-suffix]
			//     digit-sequence exponent-part [floating-suffix]
			//
			// floating-suffix = f, l, F, L
			// integer-suffix = unsigned-suffix / long-suffix or long-long-suffix in either order
			// exponent-part = e/E [sign] digit-sequence
			if (thisChar == CharCode::kPeriod)
			{
				// . digit-sequence [exponent-part] [floating-suffix]
				if (!TryGetDigitSequence(contents, coord))
					return false;

				(void)TryGetExponentPart(contents, coord);
				(void)TryGetFloatingSuffix(contents, coord);
			}
			else if (thisChar >= CharCode::kDigit0 && thisChar <= CharCode::kDigit9)
			{
				// One of:
				// digit-sequence [integer-suffix]
				// digit-sequence . digit-sequence [exponent-part] [floating-suffix]
				// digit-sequence exponent-part [floating-suffix]
				if (!TryGetDigitSequence(contents, coord))
					return false;

				if (TryGetIntegerSuffix(contents, coord))
				{
				}
				else if (TryGetExponentPart(contents, coord))
				{
					(void)TryGetFloatingSuffix(contents, coord);
				}
				else
				{

					if (coord.m_fileOffset != contents.Size())
					{
						FileCoordinate backupCoord = coord;
						const uint8_t nextChar = ConsumeLogicalChar(contents, coord);

						if (nextChar == CharCode::kPeriod && TryGetDigitSequence(contents, coord))
						{
							(void)TryGetExponentPart(contents, coord);
							(void)TryGetFloatingSuffix(contents, coord);
						}
						else
							coord = backupCoord;
					}
				}
			}
			else
				return false;

			outCoordinate = coord;
			return true;
		}

		bool CLexer::TryGetOctalNumber(ArrayView<const uint8_t> contents, FileCoordinate &inOutCoordinate)
		{
			const bool hasOctalDigits = TryGetMultipleMatchingCallback(contents, inOutCoordinate, IsOctalDigit);
			if (!hasOctalDigits)
				return false;

			(void)TryGetIntegerSuffix(contents, inOutCoordinate);

			return true;
		}

		bool CLexer::TryGetHexNumber(ArrayView<const uint8_t> contents, FileCoordinate &inOutCoordinate)
		{
			// This handles hexadecimal-floating-constant and integer-constant->hexadecimal-constant, following hexadecimal-prefix
			// Possible sequences:
			// hexadecimal-digit-sequence [integer-suffix]
			// . hexadecimal-digit-sequence binary-exponent-part [floating-suffix]
			// hexadecimal-digit-sequence . [hexadecimal-digit-sequence] binary-exponent-part [floating-suffix]
			// hexadecimal-digit-sequence binary-exponent-part [floating-suffix]

			FileCoordinate coord;
			if (TryGetSingle(contents, coord, CharCode::kPeriod))
			{
				// . hexadecimal-digit-sequence binary-exponent-part [floating-suffix]
				if (!TryGetHexDigitSequence(contents, coord))
					return false;

				if (!TryGetBinaryExponentPart(contents, coord))
					return false;

				(void)TryGetFloatingSuffix(contents, coord);
			}
			else if (TryGetHexDigitSequence(contents, coord))
			{
				// hexadecimal-digit-sequence . [hexadecimal-digit-sequence] binary-exponent-part [floating-suffix]
				// hexadecimal-digit-sequence [integer-suffix]
				// hexadecimal-digit-sequence binary-exponent-part [floating-suffix]

				FileCoordinate afterDigitSequenceCoord = coord;
				if (TryGetSingle(contents, coord, CharCode::kPeriod))
				{
					// hexadecimal-digit-sequence . [hexadecimal-digit-sequence] binary-exponent-part [floating-suffix]
					(void)TryGetHexDigitSequence(contents, coord);
					if (!TryGetBinaryExponentPart(contents, coord))
					{
						// Failed to get mandatory binary exponent part, but can still parse this as a simple digit sequence
						coord = afterDigitSequenceCoord;
					}
					else
					{
						(void)TryGetFloatingSuffix(contents, coord);
					}
				}
				else if (TryGetIntegerSuffix(contents, coord))
				{
					// hexadecimal-digit-sequence [integer-suffix]
				}
				else if (TryGetBinaryExponentPart(contents, coord))
				{
					// hexadecimal-digit-sequence binary-exponent-part [floating-suffix]
					(void)TryGetFloatingSuffix(contents, coord);
				}
				else
					return false;
			}
			else
				return false;

			inOutCoordinate = coord;
			return true;
		}

		bool CLexer::TryGetDigitSequence(ArrayView<const uint8_t> contents, FileCoordinate &inOutCoordinate)
		{
			return TryGetMultipleMatchingCallback(contents, inOutCoordinate, IsDigit);
		}

		bool CLexer::TryGetExponentPart(ArrayView<const uint8_t> contents, FileCoordinate &inOutCoordinate)
		{
			FileCoordinate coord = inOutCoordinate;

			if (coord.m_fileOffset == contents.Size())
				return false;

			const uint8_t firstChar = ConsumeLogicalChar(contents, coord);
			if (firstChar != CharCode::kLowercaseE && firstChar != CharCode::kUppercaseE)
				return false;

			const uint8_t secondChar = ConsumeLogicalChar(contents, coord);
			if (secondChar >= CharCode::kDigit0 && secondChar <= CharCode::kDigit9)
			{
				(void)TryGetDigitSequence(contents, coord);
				inOutCoordinate = coord;
				return true;
			}
			else if (secondChar == CharCode::kPlus || secondChar == CharCode::kMinus)
			{
				const bool haveDigits = TryGetDigitSequence(contents, coord);
				if (haveDigits)
				{
					inOutCoordinate = coord;
					return true;
				}
				else
					return false;
			}
			else
				return false;
		}

		bool CLexer::TryGetFloatingSuffix(ArrayView<const uint8_t> contents, FileCoordinate &inOutCoordinate)
		{
			static const uint8_t charSet[] = { CharCode::kLowercaseF, CharCode::kLowercaseL, CharCode::kUppercaseF, CharCode::kUppercaseL, 0 };
			return TryGetSingleInSet(contents, inOutCoordinate, charSet);
		}

		bool CLexer::TryGetIntegerSuffix(ArrayView<const uint8_t> contents, FileCoordinate &inOutCoordinate)
		{
			FileCoordinate coord = inOutCoordinate;

			if (TryGetUnsignedSuffix(contents, coord))
			{
				(void)TryGetLongOrLongLongSuffix(contents, coord);
				inOutCoordinate = coord;
				return true;
			}
			else if (TryGetLongOrLongLongSuffix(contents, coord))
			{
				(void)TryGetUnsignedSuffix(contents, coord);
				inOutCoordinate = coord;
				return true;
			}
			else
				return false;
		}

		bool CLexer::TryGetUnsignedSuffix(ArrayView<const uint8_t> contents, FileCoordinate &inOutCoordinate)
		{
			static const uint8_t charSet[] = { CharCode::kLowercaseU, CharCode::kUppercaseU, 0 };
			return TryGetSingleInSet(contents, inOutCoordinate, charSet);
		}

		bool CLexer::TryGetLongOrLongLongSuffix(ArrayView<const uint8_t> contents, FileCoordinate &inOutCoordinate)
		{
			FileCoordinate coord = inOutCoordinate;

			if (coord.m_fileOffset == contents.Size())
				return false;

			const uint8_t firstChar = ConsumeLogicalChar(contents, coord);
			if (firstChar == CharCode::kLowercaseL || firstChar == CharCode::kUppercaseL)
			{
				FileCoordinate afterFirstCharCoord = coord;

				if (coord.m_fileOffset == contents.Size())
				{
					inOutCoordinate = coord;
					return true;
				}

				const uint8_t secondChar = ConsumeLogicalChar(contents, coord);

				if (secondChar != firstChar)
					coord = afterFirstCharCoord;

				inOutCoordinate = coord;
				return true;
			}
			else
				return false;
		}

		bool CLexer::TryGetHexDigitSequence(ArrayView<const uint8_t> contents, FileCoordinate &inOutCoordinate)
		{
			return TryGetMultipleMatchingCallback(contents, inOutCoordinate, IsHexDigit);
		}

		bool CLexer::TryGetBinaryExponentPart(ArrayView<const uint8_t> contents, FileCoordinate &inOutCoordinate)
		{
			static const uint8_t expCodeSet[] = { CharCode::kLowercaseP, CharCode::kUppercaseP, 0 };
			static const uint8_t signCodeSet[] = { CharCode::kMinus, CharCode::kPlus, 0 };

			FileCoordinate coord = inOutCoordinate;
			if (!TryGetSingleInSet(contents, coord, expCodeSet))
				return false;

			(void)TryGetSingleInSet(contents, coord, signCodeSet);

			if (!TryGetDigitSequence(contents, coord))
				return false;

			inOutCoordinate = coord;
			return true;
		}

		bool CLexer::TryGetHeaderName(ArrayView<const uint8_t> contents, FileCoordinate &inOutCoordinate)
		{
			FileCoordinate coord = inOutCoordinate;
			if (coord.m_fileOffset == contents.Size())
				return false;

			const uint8_t firstChar = ConsumeLogicalChar(contents, coord);
			uint8_t terminalChar = 0;
			if (firstChar == CharCode::kLess)
				terminalChar = CharCode::kGreater;
			else if (firstChar == CharCode::kDoubleQuote)
				terminalChar = CharCode::kDoubleQuote;
			else
				return false;

			bool anyMidChars = false;
			for (;;)
			{
				if (coord.m_fileOffset == contents.Size())
					return false;

				const uint8_t nextChar = ConsumeLogicalChar(contents, coord);
				if (nextChar == terminalChar)
					break;
				else if (nextChar == CharCode::kLineFeed)
					return false;
				else
					anyMidChars = true;
			}

			if (anyMidChars)
			{
				inOutCoordinate = coord;
				return true;
			}
			else
				return false;
		}

		bool CLexer::TryGetUniversalCharacterCode(ArrayView<const uint8_t> contents, const FileCoordinate &coordinate, IIncludeStackTrace &includeStackTrace, IErrorReporter *errorReporter, FileCoordinate &outCoordinate)
		{
			FileCoordinate coord = coordinate;

			if (coord.m_fileOffset == contents.Size())
				return false;

			const uint8_t firstChar = ConsumeLogicalChar(contents, coord);

			if (firstChar != CharCode::kBackslash)
				return false;

			if (coord.m_fileOffset == contents.Size())
				return false;

			const uint8_t secondChar = ConsumeLogicalChar(contents, coord);
			size_t hexDigitsExpected = 0;
			if (secondChar == CharCode::kLowercaseU)
				hexDigitsExpected = 4;
			else if (secondChar == CharCode::kUppercaseU)
				hexDigitsExpected = 8;
			else
				return false;

			while (hexDigitsExpected > 0)
			{
				if (coord.m_fileOffset == contents.Size())
					break;

				const uint8_t nextChar = ConsumeLogicalChar(contents, coord);
				if (!IsHexDigit(nextChar))
					return false;

				hexDigitsExpected--;
			}

			outCoordinate = coord;
			return true;
		}

		bool CLexer::TryGetSingle(ArrayView<const uint8_t> contents, FileCoordinate &inOutCoordinate, const uint8_t charCode)
		{
			FileCoordinate coord = inOutCoordinate;
			if (coord.m_fileOffset == contents.Size())
				return false;

			const uint8_t ch = ConsumeLogicalChar(contents, coord);
			if (ch == charCode)
			{
				inOutCoordinate = coord;
				return true;
			}

			return false;
		}

		bool CLexer::TryGetSingleInSet(ArrayView<const uint8_t> contents, FileCoordinate &inOutCoordinate, const uint8_t *charCodes)
		{
			FileCoordinate coord = inOutCoordinate;
			if (coord.m_fileOffset == contents.Size())
				return false;

			const uint8_t ch = ConsumeLogicalChar(contents, coord);
			while (charCodes[0] != 0)
			{
				if (charCodes[0] == ch)
				{
					inOutCoordinate = coord;
					return true;
				}
				charCodes++;
			}

			return false;
		}

		bool CLexer::TryGetMultipleInSet(ArrayView<const uint8_t> contents, FileCoordinate &inOutCoordinate, const uint8_t *charCodes)
		{
			bool haveAny = false;

			FileCoordinate coord = inOutCoordinate;
			for (;;)
			{
				FileCoordinate prevCoord = coord;
				if (contents.Size() == coord.m_lineNumber)
					break;

				const uint8_t ch = ConsumeLogicalChar(contents, coord);
				const uint8_t *scan = charCodes;
				bool matched = false;
				while (scan[0] != 0)
				{
					if (scan[0] == ch)
					{
						matched = true;
						break;
					}
					charCodes++;
				}

				if (matched)
					haveAny = true;
				else
				{
					coord = prevCoord;
					break;
				}
			}

			if (haveAny)
			{
				inOutCoordinate = coord;
				return true;
			}

			return false;
		}

		bool CLexer::TryGetMultipleMatchingCallback(ArrayView<const uint8_t> contents, FileCoordinate &inOutCoordinate, bool(*callback)(uint8_t charCode))
		{
			bool haveAny = false;

			FileCoordinate coord = inOutCoordinate;
			for (;;)
			{
				FileCoordinate prevCoord = coord;
				if (contents.Size() == coord.m_lineNumber)
					break;

				const uint8_t ch = ConsumeLogicalChar(contents, coord);

				if (callback(ch))
					haveAny = true;
				else
				{
					coord = prevCoord;
					break;
				}
			}

			if (haveAny)
			{
				inOutCoordinate = coord;
				return true;
			}

			return false;
		}

		bool CLexer::IsHexDigit(uint8_t charCode)
		{
			if (IsDigit(charCode))
				return true;

			if (charCode >= CharCode::kLowercaseA && charCode <= CharCode::kLowercaseF)
				return true;

			if (charCode >= CharCode::kUppercaseA && charCode <= CharCode::kUppercaseF)
				return true;

			return false;
		}

		bool CLexer::IsOctalDigit(uint8_t charCode)
		{
			if (charCode >= CharCode::kDigit0 && charCode <= CharCode::kDigit7)
				return true;

			return false;
		}

		bool CLexer::IsDigit(uint8_t charCode)
		{
			if (charCode >= CharCode::kDigit0 && charCode <= CharCode::kDigit9)
				return true;

			return false;
		}

		CLexer::CharacterType CLexer::CategorizeCharacter(uint8_t byte)
		{
			if (byte < 128)
				return ms_lowCharTypes[byte];
			else
				return CharacterType::kUnicode;
		}

		uint8_t CLexer::ConsumeLogicalChar(const ArrayView<const uint8_t> &contents, FileCoordinate &coord)
		{
			const uint8_t ch = contents[coord.m_fileOffset];
			coord.m_fileOffset++;
			coord.m_column++;

			if (ch == 13)
			{
				coord.m_lineNumber++;
				coord.m_column = 0;

				if (coord.m_fileOffset != contents.Size())
				{
					if (contents[coord.m_fileOffset] == 10)
						coord.m_fileOffset++;
				}

				return 10;
			}
			else if (ch == 10)
			{
				coord.m_lineNumber++;
				coord.m_column = 0;

				return 10;
			}
			else
				return ch;
		}

		CLexer::CharacterType CLexer::ms_lowCharTypes[128] =
		{
			// 0
			CharacterType::kControl,		// NUL
			CharacterType::kControl,		// SOH
			CharacterType::kControl,		// STX
			CharacterType::kControl,		// ETX
			CharacterType::kControl,		// EOT
			CharacterType::kControl,		// ENQ
			CharacterType::kControl,		// ACK
			CharacterType::kControl,		// BEL
			CharacterType::kControl,		// BS
			CharacterType::kControl,		// HT
			CharacterType::kLineFeed,		// LF
			CharacterType::kControl,		// VT
			CharacterType::kControl,		// FF
			CharacterType::kCarriageReturn,	// CR
			CharacterType::kControl,		// SO
			CharacterType::kControl,		// SI

			// 1
			CharacterType::kControl,		// DLE
			CharacterType::kControl,		// DC1
			CharacterType::kControl,		// DC2
			CharacterType::kControl,		// DC3
			CharacterType::kControl,		// DC4
			CharacterType::kControl,		// NAK
			CharacterType::kControl,		// SYN
			CharacterType::kControl,		// ETB
			CharacterType::kControl,		// CAN
			CharacterType::kControl,		// EM
			CharacterType::kControl,		// SUB
			CharacterType::kControl,		// ESC
			CharacterType::kControl,		// FS
			CharacterType::kControl,		// GS
			CharacterType::kControl,		// RS
			CharacterType::kControl,		// US

			// 2
			CharacterType::kWhitespace,		// Space
			CharacterType::kPunctuation,	// !
			CharacterType::kPunctuation,	// "
			CharacterType::kPunctuation,	// #
			CharacterType::kPunctuation,	// $
			CharacterType::kPunctuation,	// %
			CharacterType::kPunctuation,	// &
			CharacterType::kPunctuation,	// '
			CharacterType::kPunctuation,	// (
			CharacterType::kPunctuation,	// )
			CharacterType::kPunctuation,	// *
			CharacterType::kPunctuation,	// +
			CharacterType::kPunctuation,	// ,
			CharacterType::kPunctuation,	// -
			CharacterType::kPunctuation,	// .
			CharacterType::kPunctuation,	// /

			// 3
			CharacterType::kDigit,			// 0
			CharacterType::kDigit,			// 1
			CharacterType::kDigit,			// 2
			CharacterType::kDigit,			// 3
			CharacterType::kDigit,			// 4
			CharacterType::kDigit,			// 5
			CharacterType::kDigit,			// 6
			CharacterType::kDigit,			// 7
			CharacterType::kDigit,			// 8
			CharacterType::kDigit,			// 9
			CharacterType::kPunctuation,	// :
			CharacterType::kPunctuation,	// ;
			CharacterType::kPunctuation,	// <
			CharacterType::kPunctuation,	// =
			CharacterType::kPunctuation,	// >
			CharacterType::kPunctuation,	// ?

			// 4
			CharacterType::kPunctuation,	// @
			CharacterType::kLetter,			// A
			CharacterType::kLetter,			// B
			CharacterType::kLetter,			// C
			CharacterType::kLetter,			// D
			CharacterType::kLetter,			// E
			CharacterType::kLetter,			// F
			CharacterType::kLetter,			// G
			CharacterType::kLetter,			// H
			CharacterType::kLetter,			// I
			CharacterType::kLetter,			// J
			CharacterType::kLetter,			// K
			CharacterType::kLetter,			// L
			CharacterType::kLetter,			// M
			CharacterType::kLetter,			// N
			CharacterType::kLetter,			// O

			// 5
			CharacterType::kLetter,			// P
			CharacterType::kLetter,			// Q
			CharacterType::kLetter,			// R
			CharacterType::kLetter,			// S
			CharacterType::kLetter,			// T
			CharacterType::kLetter,			// U
			CharacterType::kLetter,			// V
			CharacterType::kLetter,			// W
			CharacterType::kLetter,			// X
			CharacterType::kLetter,			// Y
			CharacterType::kLetter,			// Z
			CharacterType::kPunctuation,	// [
			CharacterType::kPunctuation,	// Backslash
			CharacterType::kPunctuation,	// ]
			CharacterType::kPunctuation,	// ^
			CharacterType::kUnderscore,		// _

			// 6
			CharacterType::kPunctuation,	// `
			CharacterType::kLetter,			// a
			CharacterType::kLetter,			// b
			CharacterType::kLetter,			// c
			CharacterType::kLetter,			// d
			CharacterType::kLetter,			// e
			CharacterType::kLetter,			// f
			CharacterType::kLetter,			// g
			CharacterType::kLetter,			// h
			CharacterType::kLetter,			// i
			CharacterType::kLetter,			// j
			CharacterType::kLetter,			// k
			CharacterType::kLetter,			// l
			CharacterType::kLetter,			// m
			CharacterType::kLetter,			// n
			CharacterType::kLetter,			// o

			// 7
			CharacterType::kLetter,			// p
			CharacterType::kLetter,			// q
			CharacterType::kLetter,			// r
			CharacterType::kLetter,			// s
			CharacterType::kLetter,			// t
			CharacterType::kLetter,			// u
			CharacterType::kLetter,			// v
			CharacterType::kLetter,			// w
			CharacterType::kLetter,			// x
			CharacterType::kLetter,			// y
			CharacterType::kLetter,			// z
			CharacterType::kPunctuation,	// {
			CharacterType::kPunctuation,	// |
			CharacterType::kPunctuation,	// }
			CharacterType::kPunctuation,	// ~
			CharacterType::kControl,		// DEL
		};
	}
}
