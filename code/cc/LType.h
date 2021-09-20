#pragma once

namespace expanse
{
	namespace cc
	{
		enum class LType
		{
			kSInt8,
			kSInt16,
			kSInt32,
			kSInt64,

			kUInt8,
			kUInt16,
			kUInt32,
			kUInt64,

			kFloat32,
			kFloat64,

			kAddress,
		};

		LType UnsignedLType(LType lType);
		LType SignedLType(LType lType);
	}
}
