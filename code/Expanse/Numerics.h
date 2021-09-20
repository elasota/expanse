#pragma once

#include <cstdint>

namespace expanse
{
	template<class T> struct ArrayView;
	struct Result;

	namespace numerics
	{
		Result StringToFloat32(const ArrayView<const uint8_t> &chars, float &result);
		Result StringToFloat64(const ArrayView<const uint8_t> &chars, double &result);
	}
}
