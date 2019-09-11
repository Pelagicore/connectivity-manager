// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#ifndef CONNECTIVITY_MANAGER_DAEMON_DBUS_OBJECTS_WIFI_ACCESS_POINT_H
#define CONNECTIVITY_MANAGER_DAEMON_DBUS_OBJECTS_WIFI_ACCESS_POINT_H

#include <giomm.h>
#include <glibmm.h>

#include <optional>
#include <string>

#include "daemon/backend.h"
#include "generated/dbus/connectivity_manager_stub.h"

namespace ConnectivityManager::Daemon
{
    // Implementation of com.luxoft.ConnectivityManager.WiFiAccessPoint D-Bus interface.
    //
    // Exposed on bus under /com/luxoft/ConnectivityManager/WiFiAccessPoints/<id>. Id is just taken
    // from Backend::WiFiAccessPoint since it is guaranteed to be unique and mapping from an object
    // path to a Backend::WiFiAccessPoint does not require any extra state.
    class WiFiAccessPoint : public com::luxoft::ConnectivityManager::WiFiAccessPointStub
    {
    public:
        using Id = Backend::WiFiAccessPoint::Id;

        explicit WiFiAccessPoint(const Backend::WiFiAccessPoint &backend_ap);

        Glib::ustring object_path() const;

        static std::optional<Id> object_path_to_id(const Glib::DBusObjectPathString &path);

        bool register_object(const Glib::RefPtr<Gio::DBus::Connection> &connection)
        {
            return WiFiAccessPointStub::register_object(connection, object_path()) != 0;
        }

    private:
        bool SSID_setHandler(const std::string &value) override;
        std::string SSID_get() override;

        bool Strength_setHandler(guchar value) override;
        guchar Strength_get() override;

        bool Connected_setHandler(bool value) override;
        bool Connected_get() override;

        static Glib::ustring object_path_prefix();

        Id id_ = 0;
        std::string ssid_;
        guchar strength_ = 0;
        bool connected_ = false;
    };
}

#endif // CONNECTIVITY_MANAGER_DAEMON_DBUS_OBJECTS_WIFI_ACCESS_POINT_H
