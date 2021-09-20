#include "CCompilerIncludeStackTracer.h"
#include "PPTokenStr.h"

namespace expanse
{
	namespace cc
	{
		CCompilerIncludeStackTracer::CCompilerIncludeStackTracer()
		{
		}

		void CCompilerIncludeStackTracer::Reset()
		{
		}

		bool CCompilerIncludeStackTracer::Pop()
		{
			return false;
		}

		void CCompilerIncludeStackTracer::GetCurrentFile(UTF8StringView_t &outDevice, UTF8StringView_t &outPath, FileCoordinate &outCoordinate) const
		{
		}

		TokenStrView CCompilerIncludeStackTracer::GetCurrentTraceFile() const
		{
			return TokenStrView();
		}
	}
}