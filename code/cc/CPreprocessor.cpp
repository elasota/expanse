#include "CPreprocessor.h"

#include "AsyncFileRequest.h"
#include "AsyncFileSystem.h"
#include "CLexer.h"
#include "CPreprocessorTraceInfo.h"
#include "CharCodes.h"
#include "FileCache.h"
#include "FileStream.h"
#include "IErrorReporter.h"
#include "IncludeStack.h"
#include "Result.h"
#include "StrUtils.h"

expanse::cc::CPreprocessor::CPreprocessor(IAllocator *alloc, AsyncFileSystem *fs, FileCache *fileCache, FileStream *outStream, IErrorReporter *errorReporter)
	: m_state(State::kIdle)
	, m_includeStackTop(nullptr)
	, m_afs(fs)
	, m_includeStackDepth(0)
	, m_pathResolutionIndex(0)
	, m_fileCache(fileCache)
	, m_outStream(outStream)
	, m_errorReporter(errorReporter)
	, m_includeStackTrace(m_includeStackTop)
	, m_systemIncludePaths(alloc)
	, m_nonSystemIncludePaths(alloc)
	, m_objectLikeMacros(*alloc)
	, m_functionLikeMacros(*alloc)
{
}

expanse::cc::CPreprocessor::~CPreprocessor()
{
}

expanse::Result expanse::cc::CPreprocessor::StartRootFile(const UTF8StringView_t &device, const UTF8StringView_t &path)
{
	EXP_ASSERT(m_includeStackDepth == 0);

	CHECK_RV(CorePtr<AsyncFileRequest>, request, this->m_afs->Retrieve(device, path));
	m_currentFileRequest = std::move(request);

	m_state = State::kLoadingRootFile;

	return ErrorCode::kOK;
}

expanse::Result expanse::cc::CPreprocessor::PushResolvedInclude(ArrayPtr<uint8_t> &&contents, UTF8String_t &&device, UTF8String_t &&path)
{
	if (m_includeStackDepth == kIncludeStackLimit)
		return ErrorCode::kStackOverflow;

	IAllocator *alloc = GetCoreObjectAllocator();

	TokenStr canonicalName;
	{
		Vector<uint8_t> canonicalNameBuilder(alloc);
		CHECK(canonicalNameBuilder.Add(device.GetChars()));

		const uint8_t divider[] = { CharCode::kColon, CharCode::kSlash, CharCode::kSlash };
		CHECK(canonicalNameBuilder.Add(ArrayView<const uint8_t>(divider)));

		CHECK(canonicalNameBuilder.Add(path.GetChars()));

		CHECK_RV(ArrayPtr<uint8_t>, canonicalNameClone, canonicalNameBuilder.ConstView().Clone(alloc));
		canonicalName = TokenStr(std::move(canonicalNameClone));
	}

	CHECK_RV(CorePtr<IncludeStack>, newIncludeStack, New<IncludeStack>(alloc, alloc, m_includeStackTop, std::move(contents), std::move(device), std::move(path), std::move(canonicalName)));

	if (m_includeStackTop == nullptr)
	{
		m_includeStack = std::move(newIncludeStack);
		m_includeStackTop = m_includeStack;
	}
	else
		m_includeStackTop->Append(std::move(newIncludeStack));

	m_includeStackDepth++;

	m_state = State::kProcessing;

	return ErrorCode::kOK;
}

expanse::Result expanse::cc::CPreprocessor::ConvertLineBreaks(IAllocator *alloc, ArrayPtr<uint8_t> &contents)
{
	ArrayView<const uint8_t> contentsView = ArrayView<uint8_t>(contents);

	const size_t length = contents.Count();

	size_t numDeletedChars = 0;
	for (size_t i = 0; i < length; i++)
	{
		if (contentsView[i] == CharCode::kBackslash)
		{
			if (i != length - 1)
			{
				const uint8_t nextChar = contentsView[i + 1];
				if (nextChar == CharCode::kCarriageReturn || nextChar == CharCode::kLineFeed)
					numDeletedChars++;
			}
		}
		if (i != length - 1 && contentsView[i] == CharCode::kCarriageReturn && contentsView[i + 1] == CharCode::kLineFeed)
			numDeletedChars++;
	}

	CHECK_RV(ArrayPtr<uint8_t>, newContents, NewArrayUninitialized<uint8_t>(alloc, length - numDeletedChars));

	ArrayView<uint8_t> newContentsView = newContents;

	size_t outOffset = 0;
	size_t numExtraNewlines = 0;
	for (size_t i = 0; i < length; i++)
	{
		uint8_t thisChar = contentsView[i];
		if (thisChar == CharCode::kCarriageReturn)
		{
			thisChar = CharCode::kLineFeed;
			if (i != length - 1)
			{
				if (contentsView[i + 1] == CharCode::kLineFeed)
					i++;
			}
		}

		if (thisChar == CharCode::kLineFeed)
		{
			numExtraNewlines++;
			while (numExtraNewlines > 0)
			{
				newContentsView[outOffset++] = CharCode::kLineFeed;
				numExtraNewlines--;
			}
		}
		else if (thisChar == CharCode::kBackslash)
		{
			if (i == length - 1)
			{
				// Just drop the character
			}
			else
			{
				const uint8_t nextChar = contentsView[i + 1];
				if (nextChar == CharCode::kLineFeed)
				{
					numExtraNewlines++;
					i++;
				}
				else if (nextChar == CharCode::kCarriageReturn)
				{
					numExtraNewlines++;
					i++;

					if (i != length - 1 && contentsView[i + 1] == CharCode::kLineFeed)
						i++;
				}
				else
					newContentsView[outOffset++] = thisChar;
			}
		}
		else
			newContentsView[outOffset++] = thisChar;
	}

	while (numExtraNewlines > 0)
	{
		newContentsView[outOffset++] = CharCode::kLineFeed;
		numExtraNewlines--;
	}

	EXP_ASSERT(outOffset == newContentsView.Size());

	contents = std::move(newContents);

	return ErrorCode::kOK;
}

void expanse::cc::CPreprocessor::Digest()
{
	if (m_state == State::kIdle || m_state == State::kFailed)
		return;

	Result result(this->DigestChecked());
	result.Handle();

	const ErrorCode errorCode = result.GetErrorCode();
	if (errorCode != ErrorCode::kOK)
		m_state = State::kFailed;
}

expanse::cc::CPreprocessor::State expanse::cc::CPreprocessor::GetState() const
{
	return m_state;
}

expanse::Result expanse::cc::CPreprocessor::FlushTrace(FileStream *traceStream)
{
	if (m_traceInfo)
	{
		CHECK(m_traceInfo->Write(traceStream));
	}

	return ErrorCode::kOK;
}

expanse::cc::CPreprocessor::Subrange::Subrange()
	: m_start(0)
	, m_length(0)
{
}

expanse::cc::CPreprocessor::Subrange::Subrange(size_t start, size_t length)
	: m_start(start)
	, m_length(length)
{
}

size_t expanse::cc::CPreprocessor::Subrange::GetStart() const
{
	return m_start;
}

size_t expanse::cc::CPreprocessor::Subrange::GetLength() const
{
	return m_length;
}


expanse::cc::CPreprocessor::PPToken::PPToken()
	: m_isWhitespace(false)
{
}

expanse::cc::CPreprocessor::PPTokenCollection::PPTokenCollection()
{
}

expanse::cc::CPreprocessor::PPTokenCollection::PPTokenCollection(PPTokenCollection &&other)
	: m_tokenContents(std::move(other.m_tokenContents))
	, m_tokens(std::move(other.m_tokens))
{
}

expanse::cc::CPreprocessor::PPTokenCollection::PPTokenCollection(ArrayPtr<uint8_t> &&tokenContents, ArrayPtr<ArrayView<const uint8_t>> &&tokens)
	: m_tokenContents(std::move(tokenContents))
	, m_tokens(std::move(tokens))
{
}

expanse::cc::CPreprocessor::PPTokenCollection::~PPTokenCollection()
{
}

expanse::ArrayView<const expanse::ArrayView<const uint8_t>> expanse::cc::CPreprocessor::PPTokenCollection::GetTokens() const
{
	return m_tokens.ConstView();
}

expanse::ArrayPtr<uint8_t> expanse::cc::CPreprocessor::PPTokenCollection::FlattenAndTakeContents()
{
	m_tokens = nullptr;
	return ArrayPtr<uint8_t>(std::move(m_tokenContents));
}

expanse::cc::CPreprocessor::PPTokenCollection &expanse::cc::CPreprocessor::PPTokenCollection::operator=(expanse::cc::CPreprocessor::PPTokenCollection &&other)
{
	if (this != &other)
	{
		m_tokenContents = std::move(other.m_tokenContents);
		m_tokens = std::move(other.m_tokens);
	}

	return *this;
}

bool expanse::cc::CPreprocessor::TokenEquals(const ArrayView<const uint8_t> &tokenChars, const char *str)
{
	size_t offset = 0;
	while (str[offset] != 0)
	{
		if (tokenChars[offset] != static_cast<uint8_t>(str[offset]))
			return false;
		offset++;
	}

	return true;
}

void expanse::cc::CPreprocessor::PopIncludeStack()
{
	if (m_includeStackTop)
	{
		IncludeStack *prev = m_includeStackTop->GetPrev();
		if (prev)
			prev->UnlinkNext();
		else
			m_includeStack = nullptr;
	}

	m_includeStackDepth--;
}

expanse::Result expanse::cc::CPreprocessor::DigestChecked()
{
	if (!m_traceInfo)
	{
		IAllocator *alloc = GetCoreObjectAllocator();
		CHECK_RV(CorePtr<CPreprocessorTraceInfo>, traceInfo, New<CPreprocessorTraceInfo>(alloc, alloc));
		m_traceInfo = std::move(traceInfo);
	}

	for (;;)
	{
		switch (m_state)
		{
		case State::kIdle:
			return ErrorCode::kOK;
		case State::kFailed:
			return ErrorCode::kOperationFailed;
		case State::kLoadingRootFile:
		case State::kLoadingLocalOnlyFile:
		case State::kLoadingLocalBeforeIncludeDirsFile:
		case State::kLoadingIncludeDirsFile:
		case State::kLoadingSystemDirsFile:
			{
				if (!m_currentFileRequest->IsFinished())
					return ErrorCode::kOK;

				ErrorCode errorCode = m_currentFileRequest->GetErrorCode();
				if (errorCode == ErrorCode::kOK)
				{
					ArrayPtr<uint8_t> results(m_currentFileRequest->TakeResult());

					CHECK(ConvertLineBreaks(this->GetCoreObjectAllocator(), results));

					UTF8String_t device;
					UTF8String_t path;
					m_currentFileRequest->TakeIdentifier(device, path);

					m_currentFileRequest = nullptr;
					CHECK(PushResolvedInclude(std::move(results), std::move(device), std::move(path)));
				}
				else if (errorCode == ErrorCode::kFileNotFound)
				{
					m_currentFileRequest = nullptr;
					CHECK(AdvanceToNextIncludePath());
				}
				else
					return RaiseIncludeError(errorCode);
			}
			break;

		case State::kProcessing:
			return Process();

		default:
			return ErrorCode::kInternalError;
		}
	}
}

expanse::Result expanse::cc::CPreprocessor::AdvanceToNextIncludePath()
{
	switch (m_state)
	{
	case State::kLoadingRootFile:
		return ErrorCode::kOperationFailed;
	case State::kLoadingLocalOnlyFile:
		CHECK(RaiseIncludeError(ErrorCode::kFileNotFound));
		break;
	case State::kLoadingLocalBeforeIncludeDirsFile:
		m_state = State::kLoadingIncludeDirsFile;
		m_pathResolutionIndex = 0;
		CHECK(EnterLoadingState());
		break;
	case State::kLoadingIncludeDirsFile:
	case State::kLoadingSystemDirsFile:
		m_pathResolutionIndex++;
		CHECK(EnterLoadingState());
		break;
	default:
		EXP_ASSERT(false);
		return ErrorCode::kInternalError;
	}

	return ErrorCode::kOK;
}

expanse::Result expanse::cc::CPreprocessor::RaiseIncludeError(ErrorCode errorCode)
{
	m_errorReporter->ReportError(m_includeStackTop->GetFileCoordinate(), m_includeStackTrace, CompilationErrorCode::kIncludeNotFound);
	return ErrorCode::kOperationFailed;
}

expanse::Result expanse::cc::CPreprocessor::Process()
{
	for (;;)
	{
		if (m_state != State::kProcessing)
			return ErrorCode::kOK;

		CHECK(m_traceInfo->AddLineInfo(&m_includeStackTrace));
		CHECK(ProcessLine());
	}
}

expanse::Result expanse::cc::CPreprocessor::ProcessLine()
{
	IncludeStack *f = m_includeStackTop;

	FileCoordinate startCoord = f->GetFileCoordinate();

	CLexer::TokenType tokenType = CLexer::TokenType::kInvalid;
	FileCoordinate coord = startCoord;
	ArrayView<const uint8_t> token;
	const bool haveToken = CLexer::TryGetToken(f->GetFileContents(), coord, true, false, m_includeStackTrace, m_errorReporter, true, false, token, tokenType, coord);

	if (!haveToken)
	{
		m_state = State::kIdle;
		return ErrorCode::kOK;
	}

	if (tokenType == CLexer::TokenType::kInvalid)
	{
		m_state = State::kFailed;
		return ErrorCode::kOperationFailed;
	}

	if (token.Size() == 1 && token[0] == CharCode::kHash)
	{
		f->SetFileCoordinate(coord);
		CHECK(ProcessDirectiveLine());
	}
	else
	{
		CHECK(ProcessTextLine());
	}

	{
		uint8_t newLineChar[] = { CharCode::kLineFeed };
		CHECK(m_outStream->WriteAll(ArrayView<const uint8_t>(newLineChar)));
	}

	return ErrorCode::kOK;
}


expanse::Result expanse::cc::CPreprocessor::ProcessTextLine()
{
	IncludeStack *f = m_includeStackTop;

	if (!f->IsInActivePreprocessorBlock())
		return SkipLine();

	FileCoordinate coord = f->GetFileCoordinate();

	IAllocator *alloc = GetCoreObjectAllocator();
	CHECK_RV(ArrayPtr<PPToken>, simpleTokens, ParseTokenSequence(coord));
	CHECK_RV(PPTokenCollection, tokenSequence, CanonicalizeTokenSequence(simpleTokens.ConstView()));
	simpleTokens = nullptr;

	CHECK(ExpandTokenSequence(tokenSequence));
	ArrayPtr<uint8_t> flatSequence(tokenSequence.FlattenAndTakeContents());

	CHECK(m_outStream->WriteAll(flatSequence.ConstView()));

	f->SetFileCoordinate(coord);

	return ErrorCode::kOK;
}

expanse::Result expanse::cc::CPreprocessor::ProcessDirectiveLine()
{
	IncludeStack *f = m_includeStackTop;

	FileCoordinate startCoord = f->GetFileCoordinate();
	FileCoordinate coord = startCoord;

	CLexer::TokenType tokenType = CLexer::TokenType::kInvalid;
	ArrayView<const uint8_t> contents = f->GetFileContents();
	ArrayView<const uint8_t> token;
	const bool haveToken = CLexer::TryGetToken(contents, startCoord, true, false, m_includeStackTrace, m_errorReporter, true, false, token, tokenType, coord);

	if (!haveToken)
	{
		// Null directive
		return ErrorCode::kOK;
	}

	if (tokenType == CLexer::TokenType::kInvalid)
		return ErrorCode::kOK;

	if (tokenType == CLexer::TokenType::kNewLine)
	{
		// Null directive
	}
	else if (tokenType == CLexer::TokenType::kIdentifier)
	{
		// Logic blocks, must come first
		if (TokenEquals(token, "if"))
			CHECK(ProcessIfDirective(contents, coord));
		else if (TokenEquals(token, "ifdef"))
			CHECK(ProcessIfDefDirective(contents, coord));
		else if (TokenEquals(token, "ifndef"))
			CHECK(ProcessIfNDefDirective(contents, coord));
		else if (TokenEquals(token, "elif"))
			CHECK(ProcessElifDirective(contents, coord));
		else if (TokenEquals(token, "else"))
			CHECK(ProcessElseDirective(contents, coord));
		else if (TokenEquals(token, "endif"))
			CHECK(ProcessEndIfDirective(contents, coord));
		else
		{
			// Non-logic blocks
			if (m_includeStackTop->IsInActivePreprocessorBlock())
			{
				if (TokenEquals(token, "include"))
					CHECK(ProcessIncludeDirective(contents, coord));
				else if (TokenEquals(token, "define"))
					CHECK(ProcessDefineDirective(contents, coord));
				else if (TokenEquals(token, "undef"))
					CHECK(ProcessUndefDirective(contents, coord));
				else if (TokenEquals(token, "line"))
					CHECK(ProcessLineDirective(contents, coord));
				else if (TokenEquals(token, "error"))
					CHECK(ProcessErrorDirective(contents, coord));
				else if (TokenEquals(token, "pragma"))
					CHECK(ProcessPragmaDirective(contents, coord));
				else
				{
					m_errorReporter->ReportError(startCoord, m_includeStackTrace, CompilationErrorCode::kUnknownDirective);
					return ErrorCode::kOperationFailed;
				}
			}
			else
				return SkipLine();
		}
	}
	else
	{
		if (!m_includeStackTop->IsInActivePreprocessorBlock())
			return SkipLine();
		else
		{
			m_errorReporter->ReportError(startCoord, m_includeStackTrace, CompilationErrorCode::kUnknownDirective);
			return ErrorCode::kOperationFailed;
		}
	}

	f->SetFileCoordinate(coord);

	return ErrorCode::kOK;
}

expanse::Result expanse::cc::CPreprocessor::ProcessIncludeDirective(const ArrayView<const uint8_t> &contents, FileCoordinate &inOutCoordinate)
{
	FileCoordinate coord = inOutCoordinate;
	ArrayView<const uint8_t> includePath;
	CLexer::TokenType tokenType = CLexer::TokenType::kInvalid;
	if (!CLexer::TryGetToken(contents, coord, true, true, m_includeStackTrace, m_errorReporter, true, false, includePath, tokenType, coord))
		m_errorReporter->ReportError(inOutCoordinate, m_includeStackTrace, CompilationErrorCode::kInvalidIncludePath);

	if (tokenType == CLexer::TokenType::kPPHeaderName || tokenType == CLexer::TokenType::kCharSequence)
	{
		m_errorReporter->ReportError(inOutCoordinate, m_includeStackTrace, CompilationErrorCode::kInvalidIncludePath);

		EXP_ASSERT(includePath.Size() >= 2);

		CHECK(StartIncluding(inOutCoordinate, includePath));
		inOutCoordinate = coord;

		return ErrorCode::kOK;
	}
	else
	{
		coord = inOutCoordinate;

		// pp-tokens case
		if (!CLexer::TryGetToken(contents, coord, true, true, m_includeStackTrace, m_errorReporter, false, false, includePath, tokenType, coord) || tokenType != CLexer::TokenType::kWhitespace)
		{
			m_errorReporter->ReportError(inOutCoordinate, m_includeStackTrace, CompilationErrorCode::kInvalidIncludePath);
			return ErrorCode::kOperationFailed;
		}

		IAllocator *alloc = GetCoreObjectAllocator();

		CHECK_RV(ArrayPtr<PPToken>, simpleTokens, ParseTokenSequence(coord));
		CHECK_RV(PPTokenCollection, tokenSequence, CanonicalizeTokenSequence(simpleTokens.ConstView()));
		simpleTokens = nullptr;

		CHECK(ExpandTokenSequence(tokenSequence));

		EXP_ASSERT(false);	// NOT YET IMPLEMENTED
	}

	return ErrorCode::kOK;
}

expanse::Result expanse::cc::CPreprocessor::ProcessDefineDirective(const ArrayView<const uint8_t> &contents, FileCoordinate &inOutCoordinate)
{
	EXP_ASSERT(false);	// NOT YET IMPLEMENTED

	return ErrorCode::kOK;
}

expanse::Result expanse::cc::CPreprocessor::ProcessUndefDirective(const ArrayView<const uint8_t> &contents, FileCoordinate &inOutCoordinate)
{
	EXP_ASSERT(false);	// NOT YET IMPLEMENTED

	return ErrorCode::kOK;
}

expanse::Result expanse::cc::CPreprocessor::ProcessLineDirective(const ArrayView<const uint8_t> &contents, FileCoordinate &inOutCoordinate)
{
	EXP_ASSERT(false);	// NOT YET IMPLEMENTED

	return ErrorCode::kOK;
}

expanse::Result expanse::cc::CPreprocessor::ProcessErrorDirective(const ArrayView<const uint8_t> &contents, FileCoordinate &inOutCoordinate)
{
	EXP_ASSERT(false);	// NOT YET IMPLEMENTED

	return ErrorCode::kOK;
}

expanse::Result expanse::cc::CPreprocessor::ProcessPragmaDirective(const ArrayView<const uint8_t> &contents, FileCoordinate &inOutCoordinate)
{
	EXP_ASSERT(false);	// NOT YET IMPLEMENTED

	return ErrorCode::kOK;
}

expanse::Result expanse::cc::CPreprocessor::ProcessIfDirective(const ArrayView<const uint8_t> &contents, FileCoordinate &inOutCoordinate)
{
	EXP_ASSERT(false);	// NOT YET IMPLEMENTED

	return ErrorCode::kOK;
}

expanse::Result expanse::cc::CPreprocessor::ProcessIfDefDirective(const ArrayView<const uint8_t> &contents, FileCoordinate &inOutCoordinate)
{
	if (!m_includeStackTop->IsInActivePreprocessorBlock())
	{
		CHECK(m_includeStackTop->PushLogic(PreprocessorLogicStack(inOutCoordinate, PreprocessorLogicState::kDisabled)));
		CHECK(SkipLine());
		return ErrorCode::kOK;
	}

	EXP_ASSERT(false);	// NOT YET IMPLEMENTED

	return ErrorCode::kOK;
}

expanse::Result expanse::cc::CPreprocessor::ProcessIfNDefDirective(const ArrayView<const uint8_t> &contents, FileCoordinate &inOutCoordinate)
{
	if (!m_includeStackTop->IsInActivePreprocessorBlock())
	{
		CHECK(m_includeStackTop->PushLogic(PreprocessorLogicStack(inOutCoordinate, PreprocessorLogicState::kDisabled)));
		CHECK(SkipLine());
		return ErrorCode::kOK;
	}

	EXP_ASSERT(false);	// NOT YET IMPLEMENTED

	return ErrorCode::kOK;
}

expanse::Result expanse::cc::CPreprocessor::ProcessElifDirective(const ArrayView<const uint8_t> &contents, FileCoordinate &inOutCoordinate)
{
	PreprocessorLogicStack *topLogic = m_includeStackTop->GetTopLogic();
	if (topLogic == nullptr)
	{
		m_errorReporter->ReportError(inOutCoordinate, m_includeStackTrace, CompilationErrorCode::kPPElifOutsideOfLogic);
		return ErrorCode::kOperationFailed;
	}

	if (topLogic->m_elseEncountered)
	{
		m_errorReporter->ReportError(inOutCoordinate, m_includeStackTrace, CompilationErrorCode::kPPElifAfterElse);
		return ErrorCode::kOperationFailed;
	}

	if (topLogic->m_state == PreprocessorLogicState::kNotYetActive)
	{
		topLogic->m_state = PreprocessorLogicState::kDisabled;
		CHECK(SkipLine());
		return ErrorCode::kOK;
	}

	EXP_ASSERT(false);	// NOT YET IMPLEMENTED

	return ErrorCode::kOK;
}

expanse::Result expanse::cc::CPreprocessor::ProcessElseDirective(const ArrayView<const uint8_t> &contents, FileCoordinate &inOutCoordinate)
{
	PreprocessorLogicStack *topLogic = m_includeStackTop->GetTopLogic();
	if (topLogic == nullptr)
	{
		m_errorReporter->ReportError(inOutCoordinate, m_includeStackTrace, CompilationErrorCode::kPPElseOutsideOfLogic);
		return ErrorCode::kOperationFailed;
	}

	if (topLogic->m_elseEncountered)
	{
		m_errorReporter->ReportError(inOutCoordinate, m_includeStackTrace, CompilationErrorCode::kPPElseAfterElse);
		return ErrorCode::kOperationFailed;
	}

	topLogic->m_elseEncountered = true;

	if (topLogic->m_state == PreprocessorLogicState::kActive)
		topLogic->m_state = PreprocessorLogicState::kDisabled;
	else if (topLogic->m_state == PreprocessorLogicState::kNotYetActive)
		topLogic->m_state = PreprocessorLogicState::kActive;

	ArrayView<const uint8_t> token;
	CLexer::TokenType tokenType = CLexer::TokenType::kInvalid;
	if (!CLexer::TryGetToken(contents, inOutCoordinate, true, false, m_includeStackTrace, m_errorReporter, true, false, token, tokenType, inOutCoordinate))
		return ErrorCode::kOK;

	if (tokenType != CLexer::TokenType::kNewLine)
	{
		m_errorReporter->ReportError(inOutCoordinate, m_includeStackTrace, CompilationErrorCode::kPPExpectedNewLineAfterElse);
		return ErrorCode::kOperationFailed;
	}

	return ErrorCode::kOK;
}

expanse::Result expanse::cc::CPreprocessor::ProcessEndIfDirective(const ArrayView<const uint8_t> &contents, FileCoordinate &inOutCoordinate)
{
	PreprocessorLogicStack *topLogic = m_includeStackTop->GetTopLogic();
	if (topLogic == nullptr)
	{
		m_errorReporter->ReportError(inOutCoordinate, m_includeStackTrace, CompilationErrorCode::kPPEndIfOutsideOfLogic);
		return ErrorCode::kOperationFailed;
	}

	m_includeStackTop->PopLogic();

	ArrayView<const uint8_t> token;
	CLexer::TokenType tokenType = CLexer::TokenType::kInvalid;
	if (!CLexer::TryGetToken(contents, inOutCoordinate, true, false, m_includeStackTrace, m_errorReporter, true, false, token, tokenType, inOutCoordinate))
		return ErrorCode::kOK;

	if (tokenType != CLexer::TokenType::kNewLine)
	{
		m_errorReporter->ReportError(inOutCoordinate, m_includeStackTrace, CompilationErrorCode::kPPExpectedNewLineAfterEndIf);
		return ErrorCode::kOperationFailed;
	}

	return ErrorCode::kOK;
}


expanse::ResultRV<expanse::ArrayPtr<expanse::cc::CPreprocessor::PPToken>> expanse::cc::CPreprocessor::ParseTokenSequence(FileCoordinate &inOutCoordinate)
{
	IAllocator *alloc = GetCoreObjectAllocator();
	ArrayView<const uint8_t> contents = m_includeStackTop->GetFileContents();
	Vector<PPToken> ppTokens(alloc);

	for (;;)
	{
		ArrayView<const uint8_t> token;
		CLexer::TokenType tokenType = CLexer::TokenType::kInvalid;
		if (!CLexer::TryGetToken(contents, inOutCoordinate, true, false, m_includeStackTrace, m_errorReporter, false, false, token, tokenType, inOutCoordinate))
			break;

		if (tokenType == CLexer::TokenType::kNewLine)
			break;

		if (tokenType == CLexer::TokenType::kInvalid)
			return ErrorCode::kOperationFailed;

		PPToken ppToken;
		ppToken.m_isWhitespace = (tokenType == CLexer::TokenType::kWhitespace);
		ppToken.m_token = token;

		CHECK(ppTokens.Add(ppToken));
	}

	const size_t numTokens = ppTokens.Size();
	CHECK_RV(ArrayPtr<PPToken>, flattenedTokens, NewArray<PPToken>(alloc, numTokens));

	for (size_t i = 0; i < numTokens; i++)
		flattenedTokens[i] = ppTokens[i];

	return std::move(flattenedTokens);
}

expanse::ResultRV<expanse::cc::CPreprocessor::PPTokenCollection> expanse::cc::CPreprocessor::CanonicalizeTokenSequence(const ArrayView<const PPToken> &ppTokens)
{
	size_t totalLength = 0;
	for (size_t i = 0; i < ppTokens.Size(); i++)
	{
		const PPToken &token = ppTokens[i];
		size_t canonicalLength = 0;
		if (token.m_isWhitespace)
			canonicalLength = 1;
		else
			canonicalLength = token.m_token.Size();

		if (std::numeric_limits<size_t>::max() - totalLength < canonicalLength)
			return ErrorCode::kOutOfMemory;

		totalLength += canonicalLength;
	}

	CHECK_RV(ArrayPtr<uint8_t>, tokenContents, NewArrayUninitialized<uint8_t>(GetCoreObjectAllocator(), totalLength));
	CHECK_RV(ArrayPtr<ArrayView<const uint8_t>>, tokens, NewArray<ArrayView<const uint8_t>>(GetCoreObjectAllocator(), ppTokens.Size()));

	size_t writeOffset = 0;
	for (size_t i = 0; i < ppTokens.Size(); i++)
	{
		const PPToken &token = ppTokens[i];

		if (token.m_isWhitespace)
		{
			tokenContents[writeOffset] = CharCode::kSpace;
			tokens[i] = tokenContents.ConstView().Subrange(writeOffset, 1);
			writeOffset++;
		}
		else
		{
			const size_t tokenSize = token.m_token.Size();
			if (tokenSize > 0)
				memcpy(&tokenContents[writeOffset], &token.m_token[0], tokenSize);

			tokens[i] = tokenContents.ConstView().Subrange(writeOffset, tokenSize);
			writeOffset += tokenSize;
		}
	}

	return PPTokenCollection(std::move(tokenContents), std::move(tokens));
}

expanse::Result expanse::cc::CPreprocessor::ExpandTokenSequence(PPTokenCollection &tokenSequence)
{
	bool haveAnyMacros = false;
	size_t firstMacroIndex = 0;

	const ArrayView<const ArrayView<const uint8_t>> tokens = tokenSequence.GetTokens();

	const size_t numTokens = tokens.Size();
	for (size_t i = 0; i < numTokens; i++)
	{
		const ArrayView<const uint8_t> token = tokens[i];
		if (IsIdentifier(token))
		{
			const TokenStrView tokenStr(token);

			if (m_functionLikeMacros.Contains(tokenStr) || m_objectLikeMacros.Contains(tokenStr))
			{
				haveAnyMacros = true;
				firstMacroIndex = i;
				break;
			}
		}
	}

	if (!haveAnyMacros)
		return ErrorCode::kOK;

	IAllocator *alloc = GetCoreObjectAllocator();

	Vector<uint8_t> newTokens(alloc);
	Vector<Subrange> newTokenRanges(alloc);

	EXP_ASSERT(false);	// NOT YET IMPLEMENTED

	return ErrorCode::kNotImplemented;
}

expanse::Result expanse::cc::CPreprocessor::SplitToPathComponents(Vector<ArrayView<const uint8_t>> &components, const ArrayView<const uint8_t> &pathRef) const
{
	const ArrayView<const uint8_t> path = pathRef;

	size_t scanStart = 0;
	while (scanStart < path.Size())
	{
		size_t scanEnd = scanStart;
		size_t nextScan = path.Size();
		while (scanEnd < path.Size())
		{
			if (path[scanEnd] == CharCode::kSlash)
			{
				nextScan = scanEnd + 1;
				break;
			}

			scanEnd++;
		}

		const ArrayView<const uint8_t> includePathComponent = path.Subrange(scanStart, scanEnd - scanStart);

		CHECK(components.Add(includePathComponent));

		scanStart = nextScan;
	}

	return ErrorCode::kOK;
}

expanse::Result expanse::cc::CPreprocessor::StartIncluding(const FileCoordinate &blameLocation, const ArrayView<const uint8_t> &tokenRef)
{
	IAllocator *alloc = GetCoreObjectAllocator();

	const ArrayView<const uint8_t> token = tokenRef;

	if (token.Size() < 2 || (token[0] != CharCode::kDoubleQuote && token[0] != CharCode::kLess))
	{
		m_errorReporter->ReportError(blameLocation, m_includeStackTrace, CompilationErrorCode::kInvalidIncludePath);
		return ErrorCode::kOperationFailed;
	}

	const bool isSystemPath = (token[0] == CharCode::kLess);

	if (isSystemPath)
	{
		if (token[token.Size() - 1] != CharCode::kGreater)
		{
			m_errorReporter->ReportError(blameLocation, m_includeStackTrace, CompilationErrorCode::kInvalidIncludePath);
			return ErrorCode::kOperationFailed;
		}
	}
	else
	{
		if (token[token.Size() - 1] != CharCode::kDoubleQuote)
		{
			m_errorReporter->ReportError(blameLocation, m_includeStackTrace, CompilationErrorCode::kInvalidIncludePath);
			return ErrorCode::kOperationFailed;
		}
	}

	const ArrayView<const uint8_t> includePath = token.Subrange(1, token.Size() - 2);

	Vector<ArrayView<const uint8_t>> includePathComponents(alloc);
	CHECK(SplitToPathComponents(includePathComponents, includePath));

	Vector<ArrayView<const uint8_t>> resolvedPathComponents(alloc);

	const size_t numComponents = includePathComponents.Size();
	bool isValid = true;
	bool convertedToLocalPath = false;
	if (numComponents == 0)
		isValid = false;
	else
	{
		for (size_t ci = 0; ci < numComponents; ci++)
		{
			ArrayView<const uint8_t> includePathComponent = includePathComponents[ci];

			if (includePathComponent.Size() == 2 && includePathComponent[0] == CharCode::kPeriod && includePathComponent[1] == CharCode::kPeriod)
			{
				if (resolvedPathComponents.Size() == 0)
				{
					bool escapedTree = false;
					if (isSystemPath)
						escapedTree = true;
					else
					{
						if (convertedToLocalPath)
							escapedTree = true;
						else
						{
							convertedToLocalPath = true;

							UTF8StringView_t currentDevice;
							UTF8StringView_t currentPath;
							m_includeStackTop->GetFileName(currentDevice, currentPath);

							CHECK(SplitToPathComponents(resolvedPathComponents, currentPath.GetChars()));

							if (resolvedPathComponents.Size() < 2)
								escapedTree = true;
							else
								resolvedPathComponents.Resize(resolvedPathComponents.Size() - 2);
						}
					}

					if (escapedTree)
					{
						m_errorReporter->ReportError(blameLocation, m_includeStackTrace, CompilationErrorCode::kIncludePathEscapesDirectoryTree);
						return ErrorCode::kOperationFailed;
					}
				}
				else
					resolvedPathComponents.Resize(resolvedPathComponents.Size() - 1);
			}
			else
			{
				if (ValidatePathComponent(includePathComponent))
					CHECK(resolvedPathComponents.Add(includePathComponent));
				else
				{
					isValid = false;
					break;
				}
			}
		}
	}

	if (resolvedPathComponents.Size() == 0)
		isValid = false;

	if (!isValid)
	{
		m_errorReporter->ReportError(blameLocation, m_includeStackTrace, CompilationErrorCode::kInvalidIncludePath);
		return ErrorCode::kOperationFailed;
	}

	size_t maxRemaining = std::numeric_limits<size_t>::max();

	size_t mergedLength = 0;
	for (size_t i = 0; i < resolvedPathComponents.Size(); i++)
	{
		size_t maxComponentSize = std::numeric_limits<size_t>::max() - mergedLength;

		if (maxComponentSize == 0)
			return ErrorCode::kOutOfMemory;

		maxComponentSize--;
		mergedLength++;

		if (resolvedPathComponents[i].Size() > maxComponentSize)
			return ErrorCode::kOutOfMemory;

		mergedLength += resolvedPathComponents[i].Size();
	}

	CHECK_RV(ArrayPtr<uint8_t>, combinedPath, NewArray<uint8_t>(alloc, mergedLength));

	size_t mergeOffset = 0;
	for (size_t i = 0; i < resolvedPathComponents.Size(); i++)
	{
		const size_t componentSize = resolvedPathComponents[i].Size();

		memcpy(&combinedPath[mergeOffset], &resolvedPathComponents[i][0], componentSize);
		mergeOffset += componentSize;
		combinedPath[mergeOffset] = CharCode::kSlash;
		mergeOffset++;
	}

	EXP_ASSERT(mergeOffset == mergedLength);

	combinedPath[mergeOffset - 1] = 0;

	CHECK_RV(UTF8String_t, pathBeingResolved, UTF8String_t::CreateFromZeroTerminatedArray(std::move(combinedPath)));
	m_pathBeingResolved = std::move(pathBeingResolved);

	if (isSystemPath)
		m_state = State::kLoadingSystemDirsFile;
	else if (convertedToLocalPath)
		m_state = State::kLoadingLocalOnlyFile;
	else
		m_state = State::kLoadingLocalBeforeIncludeDirsFile;

	m_pathResolutionIndex = 0;

	CHECK(EnterLoadingState());

	return ErrorCode::kOK;
}

expanse::Result expanse::cc::CPreprocessor::EnterLoadingState()
{
	UTF8StringView_t device;
	ArrayView<const uint8_t> currentPathChars;

	if (m_state == State::kLoadingLocalOnlyFile)
	{
		UTF8StringView_t currentDevice;
		UTF8StringView_t currentPath;
		m_includeStackTop->GetFileName(currentDevice, currentPath);

		currentPathChars = m_pathBeingResolved.GetChars();
		device = currentDevice;
	}
	else
	{
		switch (m_state)
		{
		case State::kLoadingLocalBeforeIncludeDirsFile:
			{
				UTF8StringView_t currentDevice;
				UTF8StringView_t currentPath;
				m_includeStackTop->GetFileName(currentDevice, currentPath);

				currentPathChars = currentPath.GetChars();

				size_t parentPathOffset = 0;
				for (size_t i = 0; i < currentPath.Length(); i++)
				{
					if (currentPathChars[i] == CharCode::kSlash)
						parentPathOffset = i + 1;
				}

				currentPathChars = currentPathChars.Subrange(0, parentPathOffset);
				device = currentDevice;
			}
			break;
		case State::kLoadingIncludeDirsFile:
		case State::kLoadingSystemDirsFile:
			{
				const Vector<IncludePath> *includePathsPtr = nullptr;
				if (m_state == State::kLoadingIncludeDirsFile)
					includePathsPtr = &m_nonSystemIncludePaths;
				else if (m_state == State::kLoadingSystemDirsFile)
					includePathsPtr = &m_systemIncludePaths;
				else
				{
					EXP_ASSERT(false);
					return ErrorCode::kInternalError;
				}

				const Vector<IncludePath> &includePaths = *includePathsPtr;

				if (m_pathResolutionIndex == includePaths.Size())
				{
					m_errorReporter->ReportError(m_includeStackTop->GetFileCoordinate(), m_includeStackTrace, CompilationErrorCode::kIncludeNotFound);
					return ErrorCode::kOperationFailed;
				}
				else if (m_pathResolutionIndex > includePaths.Size())
				{
					EXP_ASSERT(false);
					return ErrorCode::kInternalError;
				}
				else
				{
					const IncludePath &includePath = includePaths[m_pathResolutionIndex++];
					device = includePath.m_device;
					currentPathChars = includePath.m_path.GetChars();
				}
			}
			break;
		default:
			EXP_ASSERT(false);
			return ErrorCode::kInternalError;
		};

		size_t limitCheck = std::numeric_limits<size_t>::max() - currentPathChars.Size();

		if (limitCheck < m_pathBeingResolved.Length())
			return ErrorCode::kOutOfMemory;
		limitCheck -= m_pathBeingResolved.Length();

		if (limitCheck < 1)
			return ErrorCode::kOutOfMemory;
		limitCheck -= 1;

		CHECK_RV(ArrayPtr<uint8_t>, combinedPath, NewArray<uint8_t>(GetCoreObjectAllocator(), currentPathChars.Size() + m_pathBeingResolved.Length() + 1));
		if (currentPathChars.Size() > 0)
			memcpy(&combinedPath[0], &currentPathChars[0], currentPathChars.Size());

		memcpy(&combinedPath[currentPathChars.Size()], &m_pathBeingResolved.GetChars()[0], m_pathBeingResolved.Length());

		combinedPath[currentPathChars.Size() + m_pathBeingResolved.Length()] = 0;

		CHECK_RV(UTF8String_t, resolvedPath, UTF8String_t::CreateFromZeroTerminatedArray(std::move(combinedPath)));
			
		CHECK_RV(CorePtr<AsyncFileRequest>, request, this->m_afs->Retrieve(device, resolvedPath));
		m_currentFileRequest = std::move(request);

		return ErrorCode::kOK;
	}

	return ErrorCode::kOK;
}

expanse::Result expanse::cc::CPreprocessor::SkipLine()
{
	IncludeStack *f = m_includeStackTop;

	CLexer::TokenType tokenType = CLexer::TokenType::kInvalid;
	ArrayView<const uint8_t> contents = f->GetFileContents();
	ArrayView<const uint8_t> token;
	FileCoordinate coord = f->GetFileCoordinate();
	while (CLexer::TryGetToken(contents, coord, true, false, m_includeStackTrace, m_errorReporter, true, false, token, tokenType, coord))
	{
		if (tokenType == CLexer::TokenType::kInvalid)
			return ErrorCode::kOperationFailed;

		if (tokenType == CLexer::TokenType::kNewLine)
			break;
	}

	f->SetFileCoordinate(coord);

	return ErrorCode::kOK;
}


bool expanse::cc::CPreprocessor::ValidatePathComponent(const ArrayView<const uint8_t> &component)
{
	if (component.Size() == 0)
		return false;

	if (component[component.Size() - 1] == CharCode::kPeriod)
		return false;

	// Check for Win32 banned names
	if (component.Size() >= 3)
	{
		uint8_t banned[] =
		{
			CharCode::kUppercaseC, CharCode::kUppercaseO, CharCode::kUppercaseN,
			CharCode::kUppercaseP, CharCode::kUppercaseR, CharCode::kUppercaseN,
			CharCode::kUppercaseA, CharCode::kUppercaseU, CharCode::kUppercaseX,
			CharCode::kUppercaseN, CharCode::kUppercaseU, CharCode::kUppercaseL,
		};

		for (size_t banStart = 0; banStart < sizeof(banned); banStart += 3)
		{
			bool matched = true;
			for (size_t subBan = 0; subBan < 3; subBan++)
			{
				if ((component[subBan] & 0xDF) != banned[subBan + banStart])
				{
					matched = false;
					break;
				}
			}

			if (matched && (component.Size() == 3 || component[3] == CharCode::kPeriod))
				return false;
		}
	}

	if (component.Size() >= 4)
	{
		uint8_t banned[] =
		{
			CharCode::kUppercaseC, CharCode::kUppercaseO, CharCode::kUppercaseM,
			CharCode::kUppercaseL, CharCode::kUppercaseP, CharCode::kUppercaseT,
		};

		for (size_t banStart = 0; banStart < sizeof(banned); banStart += 3)
		{
			bool matched = true;
			for (size_t subBan = 0; subBan < 3; subBan++)
			{
				if ((component[subBan] & 0xDF) != banned[subBan + banStart])
				{
					matched = false;
					break;
				}
			}

			if (matched)
			{
				const uint8_t char4 = component[3];
				if (char4 >= CharCode::kDigit0 && char4 <= CharCode::kDigit9)
				{
					if (component.Size() == 4 || component[4] == CharCode::kPeriod)
						return false;
				}
			}
		}
	}

	// Check for sequential periods
	for (size_t i = 1; i < component.Size(); i++)
	{
		if (component[i] == CharCode::kPeriod && component[i - 1] == CharCode::kPeriod)
			return false;
	}

	// Check for single period
	if (component.Size() == 1 && component[0] == CharCode::kPeriod)
		return false;

	// Check for invalid chars
	for (size_t i = 1; i < component.Size(); i++)
	{
		uint8_t ch = component[i];

		if ((ch < CharCode::kUppercaseA || ch > CharCode::kUppercaseZ)
			&& (ch < CharCode::kLowercaseA || ch > CharCode::kLowercaseZ)
			&& (ch < CharCode::kDigit0 || ch > CharCode::kDigit9)
			&& ch != CharCode::kUnderscore
			&& ch != CharCode::kMinus
			&& ch != CharCode::kPeriod)
		{
			return false;
		}
	}

	return true;
}


bool expanse::cc::CPreprocessor::IsIdentifier(const ArrayView<const uint8_t> &token)
{
	const size_t tokenSize = token.Size();
	for (size_t i = 0; i < tokenSize; i++)
	{
		const uint8_t ch = token[i];
		if (ch == CharCode::kUnderscore
			|| (ch >= CharCode::kLowercaseA && ch <= CharCode::kLowercaseZ)
			|| (ch >= CharCode::kUppercaseA && ch <= CharCode::kUppercaseZ))
			continue;

		if (ch >= CharCode::kDigit0 && ch <= CharCode::kDigit9 && i != 0)
			continue;

		if (ch == CharCode::kBackslash)
		{
			if (tokenSize - i < 6)
				return false;

			size_t hexDigitsExpected = 0;
			if (token[i + 1] == CharCode::kLowercaseU)
				hexDigitsExpected = 4;
			else if (token[i + 1] == CharCode::kUppercaseU)
			{
				hexDigitsExpected = 8;
				if (tokenSize - i < 10)
					return false;
			}
			else
				return false;

			for (size_t j = 0; j < hexDigitsExpected; j++)
			{
				const uint8_t hexDigit = token[i + 2 + j];
				if ((ch < CharCode::kLowercaseA || ch > CharCode::kLowercaseF)
					&& (ch < CharCode::kUppercaseA || ch > CharCode::kUppercaseF)
					&& (ch < CharCode::kDigit0 || ch > CharCode::kDigit9))
					return false;
			}

			i += 1 + hexDigitsExpected;
		}
	}

	return true;
}
