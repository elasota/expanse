// xcharconv_ryu_tables.h internal header

// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception


// Copyright 2018 Ulf Adams
// Copyright (c) Microsoft Corporation. All rights reserved.

// Boost Software License - Version 1.0 - August 17th, 2003

// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:

// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.


#pragma once

#include <cstdint>

namespace msstl
{

// https://github.com/ulfjack/ryu
// See xcharconv_ryu.h for the exact commit.
// (Keep the cgmanifest.json commitHash in sync.)

// clang-format off

// vvvvvvvvvv DERIVED FROM digit_table.h vvvvvvvvvv

// A table of all two-digit numbers. This is used to speed up decimal digit
// generation by copying pairs of digits into the final output.
template <class _CharT> struct _Digit_table { };

template <>
struct _Digit_table<char>
{
	static char table[200];
};

template <>
struct _Digit_table<wchar_t>
{
	static wchar_t table[200];
};


// ^^^^^^^^^^ DERIVED FROM digit_table.h ^^^^^^^^^^

// vvvvvvvvvv DERIVED FROM d2s_full_table.h vvvvvvvvvv

// These tables are generated by PrintDoubleLookupTable.
extern uint64_t __DOUBLE_POW5_INV_SPLIT[292][2];
extern uint64_t __DOUBLE_POW5_SPLIT[326][2];

// ^^^^^^^^^^ DERIVED FROM d2s_full_table.h ^^^^^^^^^^

// vvvvvvvvvv DERIVED FROM d2fixed_full_table.h vvvvvvvvvv

static const int __TABLE_SIZE = 64;

extern uint16_t __POW10_OFFSET[__TABLE_SIZE];

extern uint64_t __POW10_SPLIT[1224][3];

static const int __TABLE_SIZE_2 = 69;
static const int __ADDITIONAL_BITS_2 = 120;

extern uint16_t __POW10_OFFSET_2[__TABLE_SIZE_2];

extern uint8_t __MIN_BLOCK_2[__TABLE_SIZE_2];

extern uint64_t __POW10_SPLIT_2[3133][3];

// ^^^^^^^^^^ DERIVED FROM d2fixed_full_table.h ^^^^^^^^^^

// clang-format on

}
