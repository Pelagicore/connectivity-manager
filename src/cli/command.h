// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#ifndef CONNECTIVITY_MANAGER_CLI_COMMAND_H
#define CONNECTIVITY_MANAGER_CLI_COMMAND_H

#include <glibmm.h>

#include <ostream>
#include <string>

#include "generated/dbus/connectivity_manager_proxy.h"

namespace ConnectivityManager::Cli
{
    class Command
    {
    public:
        using ManagerProxy = com::luxoft::ConnectivityManagerProxy;

        Command(const std::string &name, const std::string &description);

        virtual ~Command() = default;

        virtual bool parse_arguments(int argc, char *argv[], std::ostream &output) = 0;

        bool invoke(const Glib::RefPtr<ManagerProxy> &manager_proxy);

        const std::string &name() const
        {
            return name_;
        }

        const std::string &description() const
        {
            return description_;
        }

    protected:
        const Glib::RefPtr<ManagerProxy> &manager_proxy() const
        {
            return manager_proxy_;
        }

    private:
        virtual bool invoke() = 0;

        const std::string name_;
        const std::string description_;

        Glib::RefPtr<ManagerProxy> manager_proxy_;
    };
}

#endif // CONNECTIVITY_MANAGER_CLI_COMMAND_H
