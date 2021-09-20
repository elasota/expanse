#include "CPreprocessorTraceInfo.h"
#include "FileCoordinate.h"
#include "FileStream.h"
#include "IIncludeStackTrace.h"
#include "StaticArray.h"
#include "StringProto.h"
#include "XString.h"



expanse::cc::CPreprocessorTrace::CPreprocessorTrace()
	: m_currentFileNameIndex(0)
	, m_currentLineNumber(0)
	, m_prevTraceIndexPlusOne(0)
{
}

bool expanse::cc::CPreprocessorTrace::operator==(const CPreprocessorTrace &other) const
{
	return m_currentFileNameIndex == other.m_currentFileNameIndex
		&& m_currentLineNumber == other.m_currentLineNumber
		&& m_prevTraceIndexPlusOne == other.m_prevTraceIndexPlusOne;
}

bool expanse::cc::CPreprocessorTrace::operator!=(const CPreprocessorTrace &other) const
{
	return !((*this) == other);
}

expanse::cc::CPreprocessorTraceInfo::CPreprocessorTraceInfo(IAllocator *alloc)
	: m_numSequentialLines(0)
	, m_fileNames(alloc)
	, m_fileNamesIndexes(*alloc)
	, m_traces(alloc)
	, m_traceToIndex(*alloc)
	, m_binaryData(alloc)
{
}

expanse::Result expanse::cc::CPreprocessorTraceInfo::AddLineInfo(IIncludeStackTrace *trace)
{
	trace->Reset();

	IAllocator *alloc = GetCoreObjectAllocator();
	bool isResync = true;

	CHECK_RV(CPreprocessorTrace, newTrace, GenerateTrace(trace));

	bool needToResync = false;
	if (m_binaryData.Size() == 0)
		needToResync = true;
	else
	{
		if (newTrace.m_currentLineNumber > 0
			&& (newTrace.m_currentLineNumber - 1 == m_currentTrace.m_currentLineNumber)
			&& newTrace.m_currentFileNameIndex == m_currentTrace.m_currentFileNameIndex
			&& newTrace.m_prevTraceIndexPlusOne == m_currentTrace.m_prevTraceIndexPlusOne)
			m_numSequentialLines++;
		else
		{
			while (m_numSequentialLines > 255)
			{
				CHECK(m_binaryData.Add(255));
				m_numSequentialLines -= 255;
			}
			if (m_numSequentialLines > 0)
			{
				CHECK(m_binaryData.Add(static_cast<uint8_t>(m_numSequentialLines)));
			}
			CHECK(m_binaryData.Add(0));		// Sync code

			needToResync = true;

			m_numSequentialLines = 0;
		}
	}

	m_currentTrace = newTrace;

	if (needToResync)
	{
		CHECK_RV(uint32_t, traceIndex, IndexTrace(m_currentTrace));
		CHECK(WriteUInt32(traceIndex));
	}

	return ErrorCode::kOK;
}


expanse::Result expanse::cc::CPreprocessorTraceInfo::Write(FileStream *fs)
{
	// Flush sequential lines
	while (m_numSequentialLines > 255)
	{
		CHECK(m_binaryData.Add(255));
		m_numSequentialLines -= 255;
	}
	if (m_numSequentialLines > 0)
	{
		CHECK(m_binaryData.Add(static_cast<uint8_t>(m_numSequentialLines)));
	}

	if (m_fileNames.Size() > std::numeric_limits<uint32_t>::max())
		return ErrorCode::kOutOfMemory;

	StaticArray<uint8_t, 4> binInt;

	ToBinary(static_cast<uint32_t>(m_fileNames.Size()), binInt);
	CHECK(fs->WriteAll(binInt.ConstView()));

	for (size_t i = 0; i < m_fileNames.Size(); i++)
	{
		const ArrayView<const uint8_t> fileNameBytes = m_fileNames[i].GetToken();

		if (fileNameBytes.Size() > std::numeric_limits<uint32_t>::max())
			return ErrorCode::kOutOfMemory;


		ToBinary(static_cast<uint32_t>(fileNameBytes.Size()), binInt);
		CHECK(fs->WriteAll(binInt.ConstView()));

		CHECK(fs->WriteAll(fileNameBytes));
	}

	ToBinary(static_cast<uint32_t>(m_traces.Size()), binInt);
	CHECK(fs->WriteAll(binInt.ConstView()));

	CHECK(fs->WriteAll(m_traces.ConstView()));

	if (m_binaryData.Size() > std::numeric_limits<uint32_t>::max())
		return ErrorCode::kOutOfMemory;

	ToBinary(static_cast<uint32_t>(m_binaryData.Size()), binInt);
	CHECK(fs->WriteAll(binInt.ConstView()));

	CHECK(fs->WriteAll(m_binaryData.ConstView()));

	return ErrorCode::kOK;
}

expanse::Result expanse::cc::CPreprocessorTraceInfo::WriteUInt32(uint32_t value)
{
	StaticArray<uint8_t, 4> bin;
	ToBinary(value, bin);
	CHECK(m_binaryData.Add(bin.ConstView()));

	return ErrorCode::kOK;
}


expanse::ResultRV<expanse::cc::CPreprocessorTrace> expanse::cc::CPreprocessorTraceInfo::GenerateTrace(IIncludeStackTrace *trace)
{
	trace->Reset();
	return GenerateTraceRecursive(trace);
}

expanse::ResultRV<expanse::cc::CPreprocessorTrace> expanse::cc::CPreprocessorTraceInfo::GenerateTraceRecursive(IIncludeStackTrace *trace)
{
	UTF8StringView_t device, path;
	FileCoordinate coord;
	trace->GetCurrentFile(device, path, coord);

	TokenStrView traceFileName = trace->GetCurrentTraceFile();

	uint32_t prevTraceIndexPlusOne = 0;
	if (trace->Pop())
	{
		CHECK_RV(expanse::cc::CPreprocessorTrace, prevTrace, GenerateTraceRecursive(trace));
		CHECK_RV(uint32_t, prevTraceIndex, IndexTrace(prevTrace));

		prevTraceIndexPlusOne = prevTraceIndex + 1;
	}

	CPreprocessorTrace newTrace;
	CHECK_RV_ASSIGN(newTrace.m_currentFileNameIndex, IndexFileName(traceFileName));
	newTrace.m_currentLineNumber = coord.m_lineNumber;
	newTrace.m_prevTraceIndexPlusOne = prevTraceIndexPlusOne;

	return newTrace;
}

expanse::ResultRV<uint32_t> expanse::cc::CPreprocessorTraceInfo::IndexTrace(const CPreprocessorTrace &trace)
{
	HashMapIterator<CPreprocessorTrace, uint32_t> it = m_traceToIndex.Find(trace);
	if (it == m_traceToIndex.end())
	{
		if (m_traces.Size() == std::numeric_limits<uint32_t>::max())
			return ErrorCode::kOutOfMemory;

		uint32_t traceIndex = static_cast<uint32_t>(m_traces.Size());
		CHECK(m_traces.Add(trace));
		CHECK(m_traceToIndex.Insert(trace, traceIndex));

		return traceIndex;
	}
	else
		return it.Value();
}

expanse::ResultRV<uint32_t> expanse::cc::CPreprocessorTraceInfo::IndexFileName(const TokenStrView &name)
{
	HashMapIterator<TokenStrView, uint32_t> it = m_fileNamesIndexes.Find(name);
	if (it == m_fileNamesIndexes.end())
	{
		if (m_fileNames.Size() == std::numeric_limits<uint32_t>::max())
			return ErrorCode::kOutOfMemory;

		CHECK_RV(ArrayPtr<uint8_t>, nameCopyBytes, name.GetToken().Clone(GetCoreObjectAllocator()));
		
		TokenStr nameCopy(std::move(nameCopyBytes));

		const uint32_t fileNameIndex = static_cast<uint32_t>(m_fileNames.Size());
		CHECK(m_fileNamesIndexes.Insert(nameCopy.GetTokenView(), fileNameIndex));
		CHECK(m_fileNames.Add(std::move(nameCopy)));

		return fileNameIndex;
	}
	else
		return it.Value();
}

void expanse::cc::CPreprocessorTraceInfo::ToBinary(uint32_t value, StaticArray<uint8_t, 4> &outBin)
{
	for (size_t i = 0; i < 4; i++)
	{
		const uint8_t b = static_cast<uint8_t>((value >> (i * 8)) & 0xff);
		outBin[i] = b;
	}
}
