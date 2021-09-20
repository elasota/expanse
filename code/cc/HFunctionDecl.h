#pragma once

#include "ArrayPtr.h"
#include "PPTokenStr.h"
#include "HType.h"

namespace expanse
{
	namespace cc
	{
		struct HTypeQualified;
		struct HFunctionDef;
		struct HFunctionProto;

		struct HFunctionDecl
		{
			HFunctionDecl(const TokenStrView &name, size_t numArguments, HTypeQualified m_returnType);

			void SetDefinition(HFunctionDef *def);
			void SetProto(HFunctionProto *proto);
			void SetReturnType(const HTypeQualified &returnType);

		private:
			TokenStrView m_name;
			size_t m_numArguments;
			HTypeQualified m_returnType;
			HFunctionDef *m_def;
		};
	}
}
