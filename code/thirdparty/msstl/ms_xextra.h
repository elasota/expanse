#pragma once


// BITMASK OPERATIONS
#define MSSTL_BITMASK_OPS(_BITMASK) \
constexpr _BITMASK operator&(_BITMASK _Left, _BITMASK _Right) noexcept \
	{	/* return _Left & _Right */ \
	using _IntTy = ::std::underlying_type_t<_BITMASK>; \
	return (static_cast<_BITMASK>(static_cast<_IntTy>(_Left) & static_cast<_IntTy>(_Right))); \
	} \

#define MSSTL_ASSERT(n, str)
#define MSSTL_ASSERT_SINGLE(n)

#include <cstdint>

namespace msstl
{
	template <class _FloatingType>
	struct _Floating_type_traits;

	template <>
	struct _Floating_type_traits<float> {
		static constexpr int32_t _Mantissa_bits = 24; // FLT_MANT_DIG
		static constexpr int32_t _Exponent_bits = 8; // sizeof(float) * CHAR_BIT - FLT_MANT_DIG
		static constexpr int32_t _Maximum_binary_exponent = 127; // FLT_MAX_EXP - 1
		static constexpr int32_t _Minimum_binary_exponent = -126; // FLT_MIN_EXP - 1
		static constexpr int32_t _Exponent_bias = 127;
		static constexpr int32_t _Sign_shift = 31; // _Exponent_bits + _Mantissa_bits - 1
		static constexpr int32_t _Exponent_shift = 23; // _Mantissa_bits - 1

		using _Uint_type = uint32_t;

		static constexpr uint32_t _Exponent_mask = 0x000000FFu; // (1u << _Exponent_bits) - 1
		static constexpr uint32_t _Normal_mantissa_mask = 0x00FFFFFFu; // (1u << _Mantissa_bits) - 1
		static constexpr uint32_t _Denormal_mantissa_mask = 0x007FFFFFu; // (1u << (_Mantissa_bits - 1)) - 1
		static constexpr uint32_t _Special_nan_mantissa_mask = 0x00400000u; // 1u << (_Mantissa_bits - 2)
		static constexpr uint32_t _Shifted_sign_mask = 0x80000000u; // 1u << _Sign_shift
		static constexpr uint32_t _Shifted_exponent_mask = 0x7F800000u; // _Exponent_mask << _Exponent_shift
	};

	template <>
	struct _Floating_type_traits<double> {
		static constexpr int32_t _Mantissa_bits = 53; // DBL_MANT_DIG
		static constexpr int32_t _Exponent_bits = 11; // sizeof(double) * CHAR_BIT - DBL_MANT_DIG
		static constexpr int32_t _Maximum_binary_exponent = 1023; // DBL_MAX_EXP - 1
		static constexpr int32_t _Minimum_binary_exponent = -1022; // DBL_MIN_EXP - 1
		static constexpr int32_t _Exponent_bias = 1023;
		static constexpr int32_t _Sign_shift = 63; // _Exponent_bits + _Mantissa_bits - 1
		static constexpr int32_t _Exponent_shift = 52; // _Mantissa_bits - 1

		using _Uint_type = uint64_t;

		static constexpr uint64_t _Exponent_mask = 0x00000000000007FFu; // (1ULL << _Exponent_bits) - 1
		static constexpr uint64_t _Normal_mantissa_mask = 0x001FFFFFFFFFFFFFu; // (1ULL << _Mantissa_bits) - 1
		static constexpr uint64_t _Denormal_mantissa_mask = 0x000FFFFFFFFFFFFFu; // (1ULL << (_Mantissa_bits - 1)) - 1
		static constexpr uint64_t _Special_nan_mantissa_mask = 0x0008000000000000u; // 1ULL << (_Mantissa_bits - 2)
		static constexpr uint64_t _Shifted_sign_mask = 0x8000000000000000u; // 1ULL << _Sign_shift
		static constexpr uint64_t _Shifted_exponent_mask = 0x7FF0000000000000u; // _Exponent_mask << _Exponent_shift
	};

	template <>
	struct _Floating_type_traits<long double> : _Floating_type_traits<double> {};

	uint8_t BitScanReverse(uint32_t &index, uint32_t mask);
	uint8_t BitScanReverse64(uint32_t &index, uint64_t mask);

	inline uint32_t _Bit_scan_reverse(const uint32_t _Value) noexcept {
		uint32_t _Index; // Intentionally uninitialized for better codegen

		if (BitScanReverse(_Index, _Value)) {
			return _Index + 1;
		}

		return 0;
	}

	inline uint32_t _Bit_scan_reverse(const uint64_t _Value) noexcept {
		uint32_t _Index; // Intentionally uninitialized for better codegen

		if (BitScanReverse64(_Index, _Value)) {
			return _Index + 1;
		}

		return 0;
	}

	template <class _To, class _From>
	_To _Bit_cast(const _From& _Val) noexcept {
		_To _To_obj; // assumes default-init
		::memcpy(std::addressof(_To_obj), std::addressof(_Val), sizeof(_To));
		return _To_obj;
	}
}
