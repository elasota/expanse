#include "HAssembly.h"

#pragma once

#include "MaxInt.h"
#include "CompilerConstant.h"
#include "LType.h"

namespace expanse
{
	namespace cc
	{
		HAsmOpenDataSectionInstruction::HAsmOpenDataSectionInstruction()
			: m_flags(0)
			, m_objectID(0)
			, m_structureReference(0)
		{
		}

		HAsmDataEmitOpt::HAsmDataEmitOpt()
			: m_codedType(CodedType::kS32)
			, m_compilerConst(LType::kSInt32, MaxSInt(0))
			, m_symbolTableIndex(0)
		{
		}

		HAsmDataEmitOpt::HAsmDataEmitOpt(const CompilerConstant &compilerConst, size_t symbolTableIndex, CodedType codedType)
			: m_compilerConst(compilerConst)
			, m_symbolTableIndex(symbolTableIndex)
			, m_codedType(codedType)
		{
		}

		HAsmDataSeekOpt::HAsmDataSeekOpt()
			: m_offset(0)
		{
		}

		HAsmDataSeekOpt::HAsmDataSeekOpt(const MaxSInt &offset)
			: m_offset(offset)
		{
		}

		HAsmHeader::HAsmHeader()
			: m_pointerLType(LType::kUInt32)
		{
		}
	}
}
