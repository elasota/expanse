#include "CCompiler.h"
#include "CPreprocessor.h"
#include "CPreprocessorTraceInfo.h"

#include "FileCache.h"
#include "FileCoordinate.h"
#include "IErrorReporter.h"
#include "Result.h"
#include "ResultRV.h"
#include "Mem.h"
#include "MemoryRWFileStream.h"
#include "Numerics.h"
#include "StringView.h"
#include "StringProto.h"
#include "TextHAsmWriter.h"

#include "IncludeWindows.h"

#include <cstdio>

namespace expanse
{
	class AsyncFileSystem;
}

struct ErrorReporter final : public expanse::cc::IErrorReporter
{
	void ReportError(const expanse::cc::FileCoordinate &fileCoordinate, expanse::cc::IIncludeStackTrace &includeStackTrace, expanse::cc::CompilationErrorCode errorCode)
	{
		includeStackTrace.Reset();

		expanse::UTF8StringView_t device;
		expanse::UTF8StringView_t path;
		expanse::cc::FileCoordinate coord;

		includeStackTrace.GetCurrentFile(device, path, coord);

		if (device.Length() > 0)
			fwrite(&device.GetChars()[0], device.Length(), 1, stderr);
		fputs("://", stderr);
		if (path.Length() > 0)
			fwrite(&path.GetChars()[0], path.Length(), 1, stderr);
		fprintf(stderr, "(%u,%u): %i\n", coord.m_lineNumber, coord.m_column, static_cast<int>(errorCode));
	}
};

expanse::Result TestCC(expanse::IAllocator *alloc, expanse::AsyncFileSystem *asyncFS, expanse::FileStream *outFile, expanse::FileStream *traceOutFile)
{
	ErrorReporter errorReporter;

	CHECK_RV(expanse::CorePtr<expanse::MemoryRWFileStream>, ppFile, expanse::New<expanse::MemoryRWFileStream>(alloc, alloc));
	CHECK_RV(expanse::CorePtr<expanse::MemoryRWFileStream>, traceFile, expanse::New<expanse::MemoryRWFileStream>(alloc, alloc));

	CHECK_RV(expanse::CorePtr<expanse::cc::FileCache>, fileCache, expanse::New<expanse::cc::FileCache>(alloc, alloc));
	CHECK_RV(expanse::CorePtr<expanse::cc::CPreprocessor>, preprocessor, expanse::New<expanse::cc::CPreprocessor>(alloc, alloc, asyncFS, fileCache, ppFile, &errorReporter));

	CHECK(preprocessor->StartRootFile(expanse::UTF8StringView_t("game"), expanse::UTF8StringView_t("logic/test.c")));

	for (;;)
	{
		preprocessor->Digest();

		expanse::cc::CPreprocessor::State state = preprocessor->GetState();
		if (state == expanse::cc::CPreprocessor::State::kIdle)
			break;

		if (state == expanse::cc::CPreprocessor::State::kFailed)
			return expanse::ErrorCode::kOperationFailed;
	}

	CHECK(preprocessor->FlushTrace(traceFile));
	preprocessor = nullptr;

	CHECK_RV(expanse::ArrayPtr<uint8_t>, ppContents, ppFile->ContentsToArray());
	CHECK_RV(expanse::ArrayPtr<uint8_t>, traceContents, traceFile->ContentsToArray());

	CHECK_RV(expanse::CorePtr<expanse::cc::CPreprocessorTraceInfo>, traceInfo, expanse::New<expanse::cc::CPreprocessorTraceInfo>(alloc, alloc));
	//CHECK(traceInfo->Load(traceContents));

	expanse::cc::TextHAsmWriter asmWriter(outFile);
	expanse::cc::IHAsmWriter *asmWriterPtr = &asmWriter;

	CHECK_RV(expanse::CorePtr<expanse::cc::CCompiler>, compiler, expanse::New<expanse::cc::CCompiler>(alloc, alloc, &errorReporter, std::move(ppContents), traceInfo, asmWriterPtr));
	CHECK(compiler->Compile());

	EXP_ASSERT(false);

	return expanse::ErrorCode::kOK;
}
