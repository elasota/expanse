#include "LType.h"

namespace expanse
{
	namespace cc
	{
		LType UnsignedLType(LType lType)
		{
			switch (lType)
			{
			case LType::kSInt8:
				return LType::kUInt8;
			case LType::kSInt16:
				return LType::kUInt16;
			case LType::kSInt32:
				return LType::kUInt32;
			case LType::kSInt64:
				return LType::kUInt64;
			default:
				return lType;
			}
		}

		LType SignedLType(LType lType)
		{
			switch (lType)
			{
			case LType::kUInt8:
				return LType::kSInt8;
			case LType::kUInt16:
				return LType::kSInt16;
			case LType::kUInt32:
				return LType::kSInt32;
			case LType::kUInt64:
				return LType::kSInt64;
			default:
				return lType;
			}
		}
	}
}