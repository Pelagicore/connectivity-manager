// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#ifndef CONNECTIVITY_MANAGER_DAEMON_DBUS_SERVICE_H
#define CONNECTIVITY_MANAGER_DAEMON_DBUS_SERVICE_H

#include <glibmm.h>
#include <sigc++/sigc++.h>

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "daemon/backend.h"
#include "daemon/dbus_objects/manager.h"
#include "daemon/dbus_objects/wifi_access_point.h"

namespace ConnectivityManager::Daemon
{
    class DBusService
    {
    public:
        DBusService(const Glib::RefPtr<Glib::MainLoop> &main_loop, Backend &backend);
        ~DBusService();

        DBusService(const DBusService &other) = delete;
        DBusService(DBusService &&other) = delete;
        DBusService &operator=(const DBusService &other) = delete;
        DBusService &operator=(DBusService &&other) = delete;

        void own_name();
        void unown_name();

    private:
        // Helper for listening to backend signals that disconnects automatically when destroyed.
        //
        // Stored in an std::optional to handle the fact that DBusService should not listen to
        // backend changes until bus has been acquired and all objects have been registered
        // (stubs/objects can not handle being called until this has been done).
        class BackendSignalHandler : public sigc::trackable
        {
        public:
            explicit BackendSignalHandler(DBusService &service);

        private:
            void wifi_status_changed(Backend::WiFiStatus status) const;
            void wifi_access_points_changed(Backend::WiFiAccessPoint::Event event,
                                            const Backend::WiFiAccessPoint *access_point) const;

            void wifi_hotspot_status_changed(Backend::WiFiHotspotStatus status) const;
            void wifi_hotspot_ssid_changed(const std::string &ssid) const;
            void wifi_hotspot_passphrase_changed(const Glib::ustring &passphrase) const;

            DBusService &service_;
        };

        void bus_acquired(const Glib::RefPtr<Gio::DBus::Connection> &connection,
                          const Glib::ustring &name);
        void name_acquired(const Glib::RefPtr<Gio::DBus::Connection> &connection,
                           const Glib::ustring &name);
        void name_lost(const Glib::RefPtr<Gio::DBus::Connection> &connection,
                       const Glib::ustring &name);

        bool wifi_access_points_create_all_and_register_on_bus();
        std::vector<Glib::DBusObjectPathString> wifi_access_point_paths_sorted() const;

        Glib::RefPtr<Glib::MainLoop> main_loop_;

        Backend &backend_;
        std::optional<BackendSignalHandler> backend_signal_handler_;

        guint connection_id_ = 0;
        Glib::RefPtr<Gio::DBus::Connection> connection_;

        Manager manager_;
        std::map<WiFiAccessPoint::Id, std::unique_ptr<WiFiAccessPoint>> wifi_access_points_;
    };
}

#endif // CONNECTIVITY_MANAGER_DAEMON_DBUS_SERVICE_H
