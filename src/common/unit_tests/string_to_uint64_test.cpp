// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#include "common/string_to_uint64.h"

#include <gtest/gtest.h>

namespace ConnectivityManager::Common
{
    TEST(StringToUInt64, Valid)
    {
        EXPECT_EQ(0U, string_to_uint64("0").value_or(1U));
        EXPECT_EQ(1U, string_to_uint64("1").value_or(0U));
        EXPECT_EQ(123456U, string_to_uint64("123456").value_or(0U));
    }

    TEST(StringToUInt64, ValidMax)
    {
        EXPECT_EQ(0xffffffffffffffffU, string_to_uint64("18446744073709551615").value_or(0U));
    }

    TEST(StringToUInt64, OutOfRangeFails)
    {
        EXPECT_FALSE(string_to_uint64("18446744073709551616").has_value());
    }

    TEST(StringToUInt64, NegativeFails)
    {
        EXPECT_FALSE(string_to_uint64("-0").has_value());
        EXPECT_FALSE(string_to_uint64("-1").has_value());
        EXPECT_FALSE(string_to_uint64("-2").has_value());
        EXPECT_FALSE(string_to_uint64("-3432").has_value());
    }

    TEST(StringToUInt64, EmptyFails)
    {
        EXPECT_FALSE(string_to_uint64("").has_value());
    }

    TEST(StringToUInt64, SpaceOnlyFails)
    {
        EXPECT_FALSE(string_to_uint64(" ").has_value());
        EXPECT_FALSE(string_to_uint64("  ").has_value());
    }

    TEST(StringToUInt64, TrailingCharsFails)
    {
        EXPECT_FALSE(string_to_uint64("1 ").has_value());
        EXPECT_FALSE(string_to_uint64("1a").has_value());
        EXPECT_FALSE(string_to_uint64("1 a").has_value());
        EXPECT_FALSE(string_to_uint64("1.").has_value());
        EXPECT_FALSE(string_to_uint64("1.2").has_value());
        EXPECT_FALSE(string_to_uint64("1.23e2").has_value());
    }

    TEST(StringToUInt64, LeadingCharsFails)
    {
        EXPECT_FALSE(string_to_uint64(" 1").has_value());
        EXPECT_FALSE(string_to_uint64("+1").has_value());
        EXPECT_FALSE(string_to_uint64("a1").has_value());
        EXPECT_FALSE(string_to_uint64(".1").has_value());
    }
}
