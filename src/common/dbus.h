// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#ifndef CONNECTIVITY_MANAGER_COMMON_DBUS_H
#define CONNECTIVITY_MANAGER_COMMON_DBUS_H

namespace ConnectivityManager::Common
{
    class DBus
    {
    public:
        static constexpr char MANAGER_SERVICE_NAME[] = "com.luxoft.ConnectivityManager";
        static constexpr char MANAGER_OBJECT_PATH[] = "/com/luxoft/ConnectivityManager";
    };
}

#endif // CONNECTIVITY_MANAGER_COMMON_DBUS_H
