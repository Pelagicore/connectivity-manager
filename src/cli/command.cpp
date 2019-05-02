// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#include "cli/command.h"

#include <glibmm.h>

#include <string>

namespace ConnectivityManager::Cli
{
    Command::Command(const std::string &name, const std::string &description) :
        name_(name),
        description_(description)
    {
    }

    bool Command::invoke(const Glib::RefPtr<ManagerProxy> &manager_proxy)
    {
        manager_proxy_ = manager_proxy;
        return invoke();
    }
}
