// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#ifndef CONNECTIVITY_MANAGER_DAEMON_BACKENDS_CONNMAN_DBUS_H
#define CONNECTIVITY_MANAGER_DAEMON_BACKENDS_CONNMAN_DBUS_H

namespace ConnectivityManager::Daemon
{
    class ConnManDBus
    {
    public:
        static constexpr char SERVICE_NAME[] = "net.connman";
        static constexpr char MANAGER_OBJECT_PATH[] = "/";
    };
}

#endif // CONNECTIVITY_MANAGER_DAEMON_BACKENDS_CONNMAN_DBUS_H
