#pragma once

#include "IHAsmWriter.h"

namespace expanse
{
	class FileStream;

	namespace cc
	{
		struct CompilerConstant;

		class TextHAsmWriter final : public IHAsmWriter
		{
		public:
			explicit TextHAsmWriter(FileStream *fs);

			Result Start(const HAsmHeader &asmHeader) override;

			Result OpenDataSection(const HAsmOpenDataSectionInstruction &instr) override;
			Result CloseDataSection() override;
			Result WriteDataEmitOpt(const HAsmDataEmitOpt &emitOpt) override;
			Result WriteDataSeekOpt(const HAsmDataSeekOpt &seekOpt) override;

			Result Finish() override;

		private:
			TextHAsmWriter() = delete;

			Result WriteSInt(const MaxSInt &sint);
			Result WriteUInt(const MaxUInt &uint);
			Result WriteString(const char *str);
			Result WriteLType(LType lType);
			Result WriteCompilerConst(const CompilerConstant &c);

			FileStream *m_fs;
			size_t m_sequentialDataOps;
		};
	}
}
