#include "ms_xextra.h"
#include <intrin.h>

namespace msstl
{
	uint8_t BitScanReverse(uint32_t &index, uint32_t mask)
	{
		unsigned long idx;
		const uint8_t result = _BitScanReverse(&idx, mask);
		index = static_cast<uint32_t>(idx);
		return result;
	}

	uint8_t BitScanReverse64(uint32_t &index, uint64_t mask)
	{
		unsigned long _Index; // Intentionally uninitialized for better codegen
#ifdef _WIN64
		const uint8_t result = _BitScanReverse64(&_Index, mask);
		index = static_cast<uint32_t>(_Index);
		return result;
#else // ^^^ 64-bit ^^^ / vvv 32-bit vvv
		uint32_t _Ui32 = static_cast<uint32_t>(mask >> 32);

		const uint8_t resultHigh = _BitScanReverse(&_Index, _Ui32);
		if (resultHigh != 0) {
			index = static_cast<uint32_t>(_Index);
			return resultHigh + 32;
		}

		_Ui32 = static_cast<uint32_t>(mask & 0xffffffffu);

		const uint8_t resultLow = _BitScanReverse(&_Index, _Ui32);
		index = static_cast<uint32_t>(_Index);
		return resultLow;
#endif // ^^^ 32-bit ^^^
	}
}
