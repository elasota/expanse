#pragma once

#include "HAssembly.h"

namespace expanse
{
	struct Result;

	namespace cc
	{
		struct IHAsmWriter
		{
			virtual Result Start(const HAsmHeader &asmHeader) = 0;

			virtual Result OpenDataSection(const HAsmOpenDataSectionInstruction &instr) = 0;
			virtual Result CloseDataSection() = 0;
			virtual Result WriteDataEmitOpt(const HAsmDataEmitOpt &emitOpt) = 0;
			virtual Result WriteDataSeekOpt(const HAsmDataSeekOpt &seekOpt) = 0;

			virtual Result Finish() = 0;
		};
	}
}
