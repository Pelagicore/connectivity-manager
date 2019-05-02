// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#ifndef CONNECTIVITY_MANAGER_DAEMON_DBUS_NAME_WATCHER_H
#define CONNECTIVITY_MANAGER_DAEMON_DBUS_NAME_WATCHER_H

#include <giomm.h>
#include <glibmm.h>

#include <utility>

namespace ConnectivityManager::Daemon
{
    // RAII helper for watching a name on the bus.
    //
    // Meant to be used when service should call a method in client (as is the case for e.g. the
    // UserInputAgent interface). Service needs to know if a client disconnects from the bus before
    // method should be called.
    class DBusNameWatcher
    {
    public:
        DBusNameWatcher() = default;

        DBusNameWatcher(const Glib::RefPtr<Gio::DBus::Connection> &connection,
                        const Glib::ustring &name,
                        const Gio::DBus::SlotNameVanished &name_vanished)
        {
            watch_id_ = Gio::DBus::watch_name(
                connection, name, Gio::DBus::SlotNameAppeared(), name_vanished);
        }

        DBusNameWatcher(const DBusNameWatcher &other) = delete;

        DBusNameWatcher(DBusNameWatcher &&other) noexcept :
            watch_id_(std::exchange(other.watch_id_, 0))
        {
        }

        ~DBusNameWatcher()
        {
            if (watch_id_ != 0)
                Gio::DBus::unwatch_name(watch_id_);
        }

        DBusNameWatcher &operator=(const DBusNameWatcher &other) = delete;

        DBusNameWatcher &operator=(DBusNameWatcher &&other) noexcept
        {
            watch_id_ = std::exchange(other.watch_id_, 0);
            return *this;
        }

    private:
        guint watch_id_ = 0;
    };
}

#endif // CONNECTIVITY_MANAGER_DAEMON_DBUS_NAME_WATCHER_H
