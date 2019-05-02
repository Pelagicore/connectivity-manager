// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#ifndef CONNECTIVITY_MANAGER_CLI_COMMAND_MONITOR_H
#define CONNECTIVITY_MANAGER_CLI_COMMAND_MONITOR_H

#include <glibmm.h>

#include <ostream>

#include "cli/command.h"

namespace ConnectivityManager::Cli
{
    class CommandMonitor : public Command
    {
    public:
        CommandMonitor();

        bool parse_arguments(int argc, char *argv[], std::ostream &output) override;

    private:
        bool invoke() override;

        bool monitor_until_ctrl_c() const;

        void print_initial_state() const;
        void connect_signals();

        struct
        {
            bool initial_state = false;
        } arguments_;
    };
}

#endif // CONNECTIVITY_MANAGER_CLI_COMMAND_MONITOR_H
