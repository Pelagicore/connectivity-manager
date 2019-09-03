// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#include "common/string_to_uint64.h"

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <limits>

namespace ConnectivityManager::Common
{
    std::optional<std::uint64_t> string_to_uint64(const std::string &str)
    {
        if (str.empty() || !(str[0] >= '0' && str[0] <= '9')) {
            return {}; // std::strtoull() negates if '-' and allows leading ' '. Disallow.
        }

        char *end;
        constexpr int BASE = 10;
        unsigned long long value = std::strtoull(str.c_str(), &end, BASE);

        if (value == ULLONG_MAX && errno == ERANGE) {
            return {};
        }

        if (end == str.c_str() || end != str.c_str() + str.length()) {
            return {};
        }

        if (sizeof(value) > sizeof(std::uint64_t)) {
            if (value > std::numeric_limits<std::uint64_t>::max()) {
                return {};
            }
        }

        return value;
    }
}
