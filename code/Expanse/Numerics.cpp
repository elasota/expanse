#include "Numerics.h"

#include "ArrayView.h"
#include "Result.h"

#include "ms_charconv.h"

namespace expanse
{
	namespace numerics
	{
		Result StringToFloat32(const ArrayView<const uint8_t> &chars, float &result)
		{
			const uint8_t *start = chars.begin();
			const uint8_t *end = chars.end();
			msstl::from_chars_result fcResult = msstl::from_chars(reinterpret_cast<const char*>(start), reinterpret_cast<const char*>(end), result);
			if (fcResult.ec == msstl::errc())
				return ErrorCode::kOK;
			return ErrorCode::kMalformedNumber;
		}

		Result StringToFloat64(const ArrayView<const uint8_t> &chars, double &result)
		{
			const uint8_t *start = chars.begin();
			const uint8_t *end = chars.end();
			msstl::from_chars_result fcResult = msstl::from_chars(reinterpret_cast<const char*>(start), reinterpret_cast<const char*>(end), result);
			if (fcResult.ec == msstl::errc())
				return ErrorCode::kOK;
			return ErrorCode::kMalformedNumber;
		}
	}
}
