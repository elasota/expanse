#pragma once

#include "HType.h"
#include "LType.h"

namespace expanse
{
	namespace cc
	{
		enum class ErrorLevel
		{
			kDisabled,
			kWarning,
			kError,
		};

		struct CompilerConfiguration
		{
			ErrorLevel m_emptyFunctionDeclGivenArguments;
			bool m_plainCharIsUnsigned;
			LType m_shortIntLType;
			LType m_wcharLType;
			LType m_intLType;
			LType m_longIntLType;
			LType m_longLongIntLType;
			LType m_floatLType;
			LType m_doubleLType;
			LType m_intptrLType;

			CompilerConfiguration();
		};
	}
}
