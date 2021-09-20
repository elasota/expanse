#pragma once

#include "CoreObject.h"
#include "CorePtr.h"
#include "HashMap.h"
#include "IncludeStackTrace.h"
#include "PPTokenStr.h"
#include "StringProto.h"
#include "Vector.h"
#include "XString.h"

namespace expanse
{
	template<class T> struct ResultRV;
	template<class T> struct Vector;
	struct Result;
	class AsyncFileSystem;
	class AsyncFileRequest;
	class FileStream;
	struct IAllocator;

	namespace cc
	{
		class CPreprocessorTraceInfo;
		class IncludeStack;
		class FileCache;
		struct IErrorReporter;

		class CPreprocessor final : public CoreObject
		{
		public:
			enum class State
			{
				kIdle,
				kLoadingRootFile,
				kLoadingLocalOnlyFile,
				kLoadingLocalBeforeIncludeDirsFile,
				kLoadingIncludeDirsFile,
				kLoadingSystemDirsFile,
				kProcessing,
				kFailed,
			};

			CPreprocessor(IAllocator *alloc, AsyncFileSystem *fs, FileCache *fileCache, FileStream *outStream, IErrorReporter *errorReporter);
			~CPreprocessor();

			Result StartRootFile(const UTF8StringView_t &device, const UTF8StringView_t &path);
			Result PushResolvedInclude(ArrayPtr<uint8_t> &&contents, UTF8String_t &&device, UTF8String_t &&path);

			static Result ConvertLineBreaks(IAllocator *alloc, ArrayPtr<uint8_t> &contents);
			Result AddIncludeDirectory(bool isSystem, const UTF8StringView_t &device, const UTF8StringView_t &path);

			void Digest();
			State GetState() const;

			Result FlushTrace(FileStream *traceStream);

		private:
			struct IncludePath
			{
				UTF8String_t m_device;
				UTF8String_t m_path;
			};

			struct Subrange
			{
			public:
				Subrange();
				Subrange(size_t start, size_t length);

				size_t GetStart() const;
				size_t GetLength() const;

			private:
				size_t m_start;
				size_t m_length;
			};

			struct PPToken
			{
				PPToken();

				ArrayView<const uint8_t> m_token;
				bool m_isWhitespace;
			};

			struct PPTokenCollection
			{
			public:
				PPTokenCollection();
				PPTokenCollection(PPTokenCollection &&other);
				PPTokenCollection(ArrayPtr<uint8_t> &&tokenContents, ArrayPtr<ArrayView<const uint8_t>> &&tokens);
				~PPTokenCollection();

				ArrayView<const ArrayView<const uint8_t>> GetTokens() const;
				ArrayPtr<uint8_t> FlattenAndTakeContents();

				PPTokenCollection &operator=(PPTokenCollection &&other);

			private:
				ArrayPtr<uint8_t> m_tokenContents;
				ArrayPtr<ArrayView<const uint8_t>> m_tokens;

				PPTokenCollection(const PPTokenCollection &other) = delete;

				PPTokenCollection &operator=(const PPTokenCollection &other) = delete;
			};

			struct FunctionLikeMacro
			{
			public:
			};

			static bool TokenEquals(const ArrayView<const uint8_t> &tokenChars, const char *str);

			static const unsigned int kIncludeStackLimit = 256;

			void PopIncludeStack();
			Result DigestChecked();
			Result AdvanceToNextIncludePath();
			Result RaiseIncludeError(ErrorCode errorCode);
			Result Process();
			Result ProcessLine();
			Result ProcessTextLine();
			Result ProcessDirectiveLine();

			Result ProcessIncludeDirective(const ArrayView<const uint8_t> &contents, FileCoordinate &inOutCoordinate);
			Result ProcessDefineDirective(const ArrayView<const uint8_t> &contents, FileCoordinate &inOutCoordinate);
			Result ProcessUndefDirective(const ArrayView<const uint8_t> &contents, FileCoordinate &inOutCoordinate);
			Result ProcessLineDirective(const ArrayView<const uint8_t> &contents, FileCoordinate &inOutCoordinate);
			Result ProcessErrorDirective(const ArrayView<const uint8_t> &contents, FileCoordinate &inOutCoordinate);
			Result ProcessPragmaDirective(const ArrayView<const uint8_t> &contents, FileCoordinate &inOutCoordinate);
			Result ProcessIfDirective(const ArrayView<const uint8_t> &contents, FileCoordinate &inOutCoordinate);
			Result ProcessIfDefDirective(const ArrayView<const uint8_t> &contents, FileCoordinate &inOutCoordinate);
			Result ProcessIfNDefDirective(const ArrayView<const uint8_t> &contents, FileCoordinate &inOutCoordinate);
			Result ProcessElifDirective(const ArrayView<const uint8_t> &contents, FileCoordinate &inOutCoordinate);
			Result ProcessElseDirective(const ArrayView<const uint8_t> &contents, FileCoordinate &inOutCoordinate);
			Result ProcessEndIfDirective(const ArrayView<const uint8_t> &contents, FileCoordinate &inOutCoordinate);

			ResultRV<ArrayPtr<PPToken>> ParseTokenSequence(FileCoordinate &inOutCoordinate);
			ResultRV<PPTokenCollection> CanonicalizeTokenSequence(const ArrayView<const PPToken> &ppTokens);
			Result ExpandTokenSequence(PPTokenCollection &outTokenSequence);
			Result StartIncluding(const FileCoordinate &blameLocation, const ArrayView<const uint8_t> &token);
			Result SplitToPathComponents(Vector<ArrayView<const uint8_t>> &components, const ArrayView<const uint8_t> &pathRef) const;

			Result EnterLoadingState();
			Result SkipLine();

			static bool ValidatePathComponent(const ArrayView<const uint8_t> &component);
			static bool IsIdentifier(const ArrayView<const uint8_t> &token);
				
			CorePtr<IncludeStack> m_includeStack;
			IncludeStack *m_includeStackTop;
			size_t m_includeStackDepth;
			IncludeStackTrace m_includeStackTrace;

			CorePtr<CPreprocessorTraceInfo> m_traceInfo;

			AsyncFileSystem *m_afs;
			State m_state;

			UTF8String_t m_pathBeingResolved;
			size_t m_pathResolutionIndex;

			FileCache *m_fileCache;

			FileStream *m_outStream;

			IErrorReporter *m_errorReporter;

			CorePtr<AsyncFileRequest> m_currentFileRequest;

			Vector<IncludePath> m_systemIncludePaths;
			Vector<IncludePath> m_nonSystemIncludePaths;

			HashMap<TokenStr, PPTokenCollection> m_objectLikeMacros;
			HashMap<TokenStr, FunctionLikeMacro> m_functionLikeMacros;
		};
	}
}

