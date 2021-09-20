#pragma once

#include <assert.h>

#define EXP_ASSERT(n) assert(n)
#define EXP_ASSERT_RESULT(n) (EXP_ASSERT((n) == ::expanse::ErrorCode::kOK))
