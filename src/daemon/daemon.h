// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#ifndef CONNECTIVITY_MANAGER_DAEMON_DAEMON_H
#define CONNECTIVITY_MANAGER_DAEMON_DAEMON_H

#include <glibmm.h>

#include <memory>
#include <string>

#include "daemon/backend.h"
#include "daemon/dbus_service.h"

namespace ConnectivityManager::Daemon
{
    class Daemon
    {
    public:
        explicit Daemon(std::unique_ptr<Backend> &&backend);
        ~Daemon();

        Daemon(const Daemon &other) = delete;
        Daemon(Daemon &&other) = delete;
        Daemon &operator=(const Daemon &other) = delete;
        Daemon &operator=(Daemon &&other) = delete;

        int run();
        void quit() const;

        void reload_config() const;

    private:
        bool register_signal_handlers();
        void unregister_signal_handlers();

        Glib::RefPtr<Glib::MainLoop> main_loop_ = Glib::MainLoop::create();

        guint sigint_source_id_ = 0;
        guint sigterm_source_id_ = 0;
        guint sighup_source_id_ = 0;

        std::unique_ptr<Backend> backend_;

        DBusService dbus_service_;
    };
}

#endif // CONNECTIVITY_MANAGER_DAEMON_DAEMON_H
