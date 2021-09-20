#include "IncludeStackTrace.h"
#include "IncludeStack.h"

namespace expanse
{
	namespace cc
	{
		IncludeStackTrace::IncludeStackTrace(IncludeStack *const&includeStack)
			: m_current(includeStack)
			, m_top(includeStack)
		{
		}

		void IncludeStackTrace::Reset()
		{
			m_current = m_top;
		}

		bool IncludeStackTrace::Pop()
		{
			m_current = m_current->GetPrev();
			return (m_current != nullptr);
		}

		void IncludeStackTrace::GetCurrentFile(UTF8StringView_t &outDevice, UTF8StringView_t &outPath, FileCoordinate &outCoordinate) const
		{
			m_current->GetFileName(outDevice, outPath);
			outCoordinate = m_current->GetFileCoordinate();
		}

		TokenStrView IncludeStackTrace::GetCurrentTraceFile() const
		{
			return m_current->GetTraceFileName();
		}
	}
}
