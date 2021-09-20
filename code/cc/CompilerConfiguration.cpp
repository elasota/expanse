#include "CompilerConfiguration.h"

namespace expanse
{
	namespace cc
	{
		CompilerConfiguration::CompilerConfiguration()
			: m_emptyFunctionDeclGivenArguments(ErrorLevel::kWarning)
			, m_plainCharIsUnsigned(false)
			, m_shortIntLType(LType::kSInt16)
			, m_wcharLType(LType::kUInt16)
			, m_intLType(LType::kSInt32)
			, m_longIntLType(LType::kSInt32)
			, m_longLongIntLType(LType::kSInt64)
			, m_floatLType(LType::kFloat32)
			, m_doubleLType(LType::kFloat64)
			, m_intptrLType(LType::kSInt32)
		{
		}
	}
}
