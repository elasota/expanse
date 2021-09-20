#pragma once

#include "MaxInt.h"
#include "CompilerConstant.h"
#include "LType.h"

namespace expanse
{
	namespace cc
	{
		enum class HAsmOpcode
		{
			kInvalid,

			kOpenDataSection,
			kOpenFunction,
		};

		struct HAsmOpenDataSectionInstruction
		{
			static const uint8_t kFlagMergeable = 1;
			static const uint8_t kFlagStructural = 2;
			static const uint8_t kFlagReadOnly = 3;

			size_t m_structureReference;
			size_t m_objectID;
			uint8_t m_flags;

			HAsmOpenDataSectionInstruction();
		};

		enum class HAsmDataSectionOpcode
		{
			kEmit,
			kSeek,
			kEnd,
		};

		struct HAsmDataEmitOpt
		{
			enum class CodedType : uint8_t
			{
				kS8,
				kS16,
				kS32,
				kS64,
				kU8,
				kU16,
				kU32,
				kU64,
				kF32,
				kF64,
				kNullPtr,
				KLiteralPtr,
				kDataPtr,
				kDataOffsetPtr,
			};

			HAsmDataEmitOpt();
			HAsmDataEmitOpt(const CompilerConstant &compilerConst, size_t symbolTableIndex, CodedType codedType);

			CompilerConstant m_compilerConst;
			size_t m_symbolTableIndex;
			CodedType m_codedType;
		};

		struct HAsmDataSeekOpt
		{
			HAsmDataSeekOpt();
			HAsmDataSeekOpt(const MaxSInt &offset);

			MaxSInt m_offset;
		};

		struct HAsmHeader
		{
			HAsmHeader();

			LType m_pointerLType;
		};
	}
}
