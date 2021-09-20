#include "TextHAsmWriter.h"
#include "FileStream.h"
#include "Result.h"
#include "StaticArray.h"

namespace expanse
{
	namespace cc
	{
		TextHAsmWriter::TextHAsmWriter(FileStream *fs)
			: m_fs(fs)
			, m_sequentialDataOps(0)
		{

		}

		Result TextHAsmWriter::Start(const HAsmHeader &asmHeader)
		{
			CHECK(WriteString("hasm ptr="));
			CHECK(WriteLType(asmHeader.m_pointerLType));
			CHECK(WriteString("\n"));

			return ErrorCode::kOK;
		}

		Result TextHAsmWriter::OpenDataSection(const HAsmOpenDataSectionInstruction &instr)
		{
			CHECK(WriteString("data "));
			CHECK(WriteUInt(instr.m_objectID));

			if (instr.m_flags & HAsmOpenDataSectionInstruction::kFlagMergeable)
				CHECK(WriteString(" merge"));
			if (instr.m_flags & HAsmOpenDataSectionInstruction::kFlagReadOnly)
				CHECK(WriteString(" readonly"));
			if (instr.m_flags & HAsmOpenDataSectionInstruction::kFlagStructural)
			{
				CHECK(WriteString(" structural("));
				CHECK(WriteUInt(instr.m_structureReference));
				CHECK(WriteString(")"));
			}

			m_sequentialDataOps = 0;

			return ErrorCode::kOK;
		}

		Result TextHAsmWriter::CloseDataSection()
		{
			CHECK(WriteString("\nend\n"));

			return ErrorCode::kOK;
		}

		Result TextHAsmWriter::WriteDataEmitOpt(const HAsmDataEmitOpt &emitOpt)
		{
			if (m_sequentialDataOps == 16)
				m_sequentialDataOps = 0;

			if (m_sequentialDataOps == 0)
				CHECK(WriteString("\n   "));

			m_sequentialDataOps++;

			switch (emitOpt.m_codedType)
			{
			case HAsmDataEmitOpt::CodedType::kS8:
				CHECK(WriteString(" s8"));
				break;
			case HAsmDataEmitOpt::CodedType::kS16:
				CHECK(WriteString(" s16"));
				break;
			case HAsmDataEmitOpt::CodedType::kS32:
				CHECK(WriteString(" s32"));
				break;
			case HAsmDataEmitOpt::CodedType::kS64:
				CHECK(WriteString(" s64"));
				break;

			case HAsmDataEmitOpt::CodedType::kU8:
				CHECK(WriteString(" u8"));
				break;
			case HAsmDataEmitOpt::CodedType::kU16:
				CHECK(WriteString(" u16"));
				break;
			case HAsmDataEmitOpt::CodedType::kU32:
				CHECK(WriteString(" u32"));
				break;
			case HAsmDataEmitOpt::CodedType::kU64:
				CHECK(WriteString(" u64"));
				break;

			case HAsmDataEmitOpt::CodedType::kF32:
				CHECK(WriteString(" f32"));
				break;
			case HAsmDataEmitOpt::CodedType::kF64:
				CHECK(WriteString(" f64"));
				break;

			case HAsmDataEmitOpt::CodedType::kNullPtr:
				CHECK(WriteString(" nl"));
				break;

			case HAsmDataEmitOpt::CodedType::KLiteralPtr:
				CHECK(WriteString(" lp"));
				break;

			case HAsmDataEmitOpt::CodedType::kDataPtr:
			case HAsmDataEmitOpt::CodedType::kDataOffsetPtr:
				CHECK(WriteString(" dp"));
				break;
			default:
				EXP_ASSERT(false);
				return ErrorCode::kInternalError;
			}

			switch (emitOpt.m_codedType)
			{
			case HAsmDataEmitOpt::CodedType::kS8:
			case HAsmDataEmitOpt::CodedType::kS16:
			case HAsmDataEmitOpt::CodedType::kS32:
			case HAsmDataEmitOpt::CodedType::kS64:
			case HAsmDataEmitOpt::CodedType::kU8:
			case HAsmDataEmitOpt::CodedType::kU16:
			case HAsmDataEmitOpt::CodedType::kU32:
			case HAsmDataEmitOpt::CodedType::kU64:
			case HAsmDataEmitOpt::CodedType::kF32:
			case HAsmDataEmitOpt::CodedType::kF64:
			case HAsmDataEmitOpt::CodedType::KLiteralPtr:
				CHECK(WriteCompilerConst(emitOpt.m_compilerConst));
				break;

			case HAsmDataEmitOpt::CodedType::kNullPtr:
				break;

			case HAsmDataEmitOpt::CodedType::kDataPtr:
				CHECK(WriteUInt(emitOpt.m_symbolTableIndex));
				break;
			case HAsmDataEmitOpt::CodedType::kDataOffsetPtr:
				CHECK(WriteUInt(emitOpt.m_symbolTableIndex));
				CHECK(WriteCompilerConst(emitOpt.m_compilerConst));
				break;
			default:
				EXP_ASSERT(false);
				return ErrorCode::kInternalError;
			}

			return ErrorCode::kOK;
		}

		Result TextHAsmWriter::WriteDataSeekOpt(const HAsmDataSeekOpt &seekOpt)
		{
			if (m_sequentialDataOps == 16)
				m_sequentialDataOps = 0;

			if (m_sequentialDataOps == 0)
				CHECK(WriteString("\n   "));

			m_sequentialDataOps++;

			CHECK(WriteString(" sk"));
			CHECK(WriteSInt(seekOpt.m_offset));

			return ErrorCode::kOK;
		}

		Result TextHAsmWriter::Finish()
		{
			return ErrorCode::kOK;
		}

		Result TextHAsmWriter::WriteSInt(const MaxSInt &sint)
		{
			MaxSInt zero = MaxSInt(0);

			if (sint < zero)
			{
				StaticArray<char, MaxSInt::kMaxDecimalDigits + 2> writeBuffer;
				size_t writeOffset = 0;

				writeBuffer[writeOffset++] = '-';

				MaxSInt decomposeTemp = sint;
				while (decomposeTemp != 0)
				{
					int8_t remainder = 0;
					decomposeTemp.DivMod10(decomposeTemp, remainder);
					writeBuffer[writeOffset++] = '0' - remainder;
				}

				for (size_t i = 0; i < writeOffset / 2; i++)
					std::swap(writeBuffer[i], writeBuffer[writeOffset - 1 - i]);

				writeBuffer[writeOffset++] = 0;

				CHECK(WriteString(&writeBuffer.ConstView()[0]));
			}
			else
			{
				CHECK_RV(MaxUInt, uint, (sint.ToUnsigned()));
				CHECK(WriteUInt(uint));
			}

			return ErrorCode::kOK;
		}

		Result TextHAsmWriter::WriteUInt(const MaxUInt &uint)
		{
			MaxUInt zero = MaxUInt(0);

			if (uint == zero)
			{
				CHECK(WriteString("0"));
			}
			else
			{
				StaticArray<char, MaxSInt::kMaxDecimalDigits + 1> writeBuffer;
				size_t writeOffset = 0;
				MaxUInt decomposeTemp = uint;
				while (decomposeTemp != 0)
				{
					uint8_t remainder = 0;
					decomposeTemp.DivMod10(decomposeTemp, remainder);
					writeBuffer[writeOffset++] = '0' + remainder;
				}

				for (size_t i = 0; i < writeOffset / 2; i++)
					std::swap(writeBuffer[i], writeBuffer[writeOffset - 1 - i]);

				writeBuffer[writeOffset++] = 0;

				CHECK(WriteString(&writeBuffer.ConstView()[0]));
			}

			return ErrorCode::kOK;
		}

		Result TextHAsmWriter::WriteString(const char *str)
		{
			const size_t length = strlen(str);
			return m_fs->WriteAll(ArrayView<const char>(str, length));
		}

		Result TextHAsmWriter::WriteLType(LType lType)
		{
			const char *desc = nullptr;
			switch (lType)
			{
			case LType::kSInt8:
				desc = "s8";
				break;
			case LType::kSInt16:
				desc = "s16";
				break;
			case LType::kSInt32:
				desc = "s32";
				break;
			case LType::kSInt64:
				desc = "s64";
				break;

			case LType::kUInt8:
				desc = "u8";
				break;
			case LType::kUInt16:
				desc = "u16";
				break;
			case LType::kUInt32:
				desc = "u32";
				break;
			case LType::kUInt64:
				desc = "u64";
				break;

			case LType::kFloat32:
				desc = "f32";
				break;
			case LType::kFloat64:
				desc = "f64";
				break;

			case LType::kAddress:
				desc = "addr";
				break;

			default:
				EXP_ASSERT(false);
				return ErrorCode::kInternalError;
			}

			return WriteString(desc);
		}

		Result TextHAsmWriter::WriteCompilerConst(const CompilerConstant &c)
		{
			switch (c.GetLType())
			{
			case LType::kSInt8:
			case LType::kSInt16:
			case LType::kSInt32:
			case LType::kSInt64:
				return WriteSInt(c.GetSigned());

			case LType::kUInt8:
			case LType::kUInt16:
			case LType::kUInt32:
			case LType::kUInt64:
			case LType::kFloat32:
			case LType::kFloat64:
			case LType::kAddress:
				return WriteUInt(c.GetUnsigned());

			default:
				EXP_ASSERT(false);
				return ErrorCode::kInternalError;
			}
		}
	}
}
