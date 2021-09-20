#pragma once

#include "ArrayPtr.h"
#include "CoreObject.h"
#include "HashMap.h"
#include "PPTokenStr.h"
#include "Vector.h"

#include <cstdint>

namespace expanse
{
	struct IAllocator;
	struct Result;
	class FileStream;
	template<class T, size_t TSize> struct StaticArray;

	namespace cc
	{
		struct IIncludeStackTrace;

		struct CPreprocessorTrace
		{
			CPreprocessorTrace();

			uint32_t m_currentFileNameIndex;
			uint32_t m_currentLineNumber;
			uint32_t m_prevTraceIndexPlusOne;

			bool operator==(const CPreprocessorTrace &other) const;
			bool operator!=(const CPreprocessorTrace &other) const;
		};

		class CPreprocessorTraceInfo final : public CoreObject
		{
		public:
			explicit CPreprocessorTraceInfo(IAllocator *alloc);

			Result AddLineInfo(IIncludeStackTrace *trace);
			Result Write(FileStream *fs);

		private:
			ResultRV<CPreprocessorTrace> GenerateTrace(IIncludeStackTrace *trace);
			ResultRV<CPreprocessorTrace> GenerateTraceRecursive(IIncludeStackTrace *trace);
			ResultRV<uint32_t> IndexTrace(const CPreprocessorTrace &trace);
			ResultRV<uint32_t> IndexFileName(const TokenStrView &token);

			Result WriteUInt32(uint32_t value);
			static void ToBinary(uint32_t value, StaticArray<uint8_t, 4> &outBin);

			CPreprocessorTrace m_currentTrace;

			size_t m_numSequentialLines;

			Vector<TokenStr> m_fileNames;
			HashMap<TokenStrView, uint32_t> m_fileNamesIndexes;

			Vector<CPreprocessorTrace> m_traces;
			HashMap<CPreprocessorTrace, uint32_t> m_traceToIndex;

			Vector<uint8_t> m_binaryData;
		};
	}
}

#include "Hasher.h"

namespace expanse
{
	template<>
	class Hasher<expanse::cc::CPreprocessorTrace> final : public DefaultHasher<expanse::cc::CPreprocessorTrace, true>
	{
	public:
	};
}
