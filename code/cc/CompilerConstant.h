#pragma once

#include <cstdint>
#include "LType.h"
#include "MaxInt.h"

namespace expanse
{
	namespace cc
	{
		struct CompilerConstant final
		{
			CompilerConstant();
			CompilerConstant(LType lType, const MaxUInt &u);
			CompilerConstant(LType lType, const MaxSInt &u);
			CompilerConstant(const CompilerConstant &other);
			~CompilerConstant();

			LType GetLType() const;
			MaxSInt GetSigned() const;
			MaxUInt GetUnsigned() const;

			CompilerConstant &operator=(const CompilerConstant &other);

		private:
			LType m_ltype;

			void DestructUnion();

			union VUnion
			{
				MaxSInt m_s;
				MaxUInt m_u;

				VUnion(const MaxSInt &s);
				VUnion(const MaxUInt &s);
				~VUnion();
			};

			VUnion m_u;
			bool m_isSigned;
		};
	}
}
