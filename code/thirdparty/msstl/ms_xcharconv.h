// xcharconv.h internal header

// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#pragma once

#include <cstdint>
#include <type_traits>
#include "ms_xextra.h"
#include "ms_xerrc.h"


namespace msstl {
	enum class chars_format {
		scientific = 0b001,
		fixed = 0b010,
		hex = 0b100,
		general = fixed | scientific,
	};

	MSSTL_BITMASK_OPS(chars_format)

		struct to_chars_result {
		char* ptr;
		errc ec;
	};

}
