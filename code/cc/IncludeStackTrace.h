#pragma once

#include "IIncludeStackTrace.h"

namespace expanse
{
	namespace cc
	{
		class IncludeStack;

		struct IncludeStackTrace final : public IIncludeStackTrace
		{
		public:
			explicit IncludeStackTrace(IncludeStack *const&includeStack);

			void Reset() override;
			bool Pop() override;
			void GetCurrentFile(UTF8StringView_t &outDevice, UTF8StringView_t &outPath, FileCoordinate &outCoordinate) const override;
			TokenStrView GetCurrentTraceFile() const override;

		private:
			IncludeStack *const& m_top;
			IncludeStack *m_current;
		};
	}
}
