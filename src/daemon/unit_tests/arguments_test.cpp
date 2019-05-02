// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#include "daemon/arguments.h"

#include <gtest/gtest.h>

#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace ConnectivityManager::Daemon
{
    namespace
    {
        constexpr char ARGV0[] = "program";

        std::optional<Arguments> parse(std::vector<std::string> argv_strs)
        {
            std::vector<char *> argv;
            std::ostringstream output;

            argv.reserve(argv_strs.size());

            for (auto &argv_str : argv_strs)
                argv.push_back(argv_str.data());

            return Arguments::parse(argv.size(), argv.data(), output);
        }
    }

    TEST(Arguments, NoArgumentsSucceeds)
    {
        std::optional<Arguments> arguments = parse({ARGV0});

        EXPECT_TRUE(arguments.has_value());
    }

    TEST(Arguments, NonExistingArgumentFails)
    {
        std::optional<Arguments> arguments = parse({ARGV0, "--does_not_exist"});

        EXPECT_FALSE(arguments.has_value());
    }

    TEST(Arguments, VersionArgumentSetsPrintVersionAndExit)
    {
        std::optional<Arguments> arguments = parse({ARGV0, "--version"});

        ASSERT_TRUE(arguments.has_value());
        EXPECT_TRUE(arguments->print_version_and_exit);
    }
}
