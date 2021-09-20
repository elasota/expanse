#pragma once

#include "IIncludeStackTrace.h"
#include "FileCoordinate.h"

namespace expanse
{
	namespace cc
	{
		struct CCompilerIncludeStackTracer final : public IIncludeStackTrace
		{
			CCompilerIncludeStackTracer();

			void Reset() override;
			bool Pop() override;
			void GetCurrentFile(UTF8StringView_t &outDevice, UTF8StringView_t &outPath, FileCoordinate &outCoordinate) const override;
			TokenStrView GetCurrentTraceFile() const override;

		private:
			FileCoordinate m_coordinate;
		};
	}
}