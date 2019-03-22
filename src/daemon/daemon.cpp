// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#include "daemon/daemon.h"

#include <glib-unix.h>
#include <glib.h>
#include <glibmm.h>

#include <cassert>
#include <csignal>
#include <cstddef>

namespace TemplateDBusService::Daemon
{
    namespace
    {
        gboolean sigint_and_sigterm_callback(void *daemon)
        {
            static_cast<Daemon *>(daemon)->quit();
            return G_SOURCE_CONTINUE;
        }

        gboolean sighup_callback(void *daemon)
        {
            static_cast<Daemon *>(daemon)->reload_config();
            return G_SOURCE_CONTINUE;
        }
    }

    Daemon::~Daemon()
    {
        unregister_signal_handlers();
    }

    int Daemon::run()
    {
        if (!register_signal_handlers())
            return EXIT_FAILURE;

        dbus_service_.own_name();

        main_loop_->run();

        dbus_service_.unown_name();

        return EXIT_SUCCESS;
    }

    void Daemon::quit() const
    {
        main_loop_->quit();
    }

    void Daemon::reload_config() const
    {
    }

    bool Daemon::register_signal_handlers()
    {
        assert(sigint_source_id_ == 0 && sigterm_source_id_ == 0 && sighup_source_id_ == 0);

        // g_unix_signal_add() is not wrapped in glibmm, use id:s even if it is a bit error prone.
        sigint_source_id_ = g_unix_signal_add(SIGINT, sigint_and_sigterm_callback, this);
        sigterm_source_id_ = g_unix_signal_add(SIGTERM, sigint_and_sigterm_callback, this);
        sighup_source_id_ = g_unix_signal_add(SIGHUP, sighup_callback, this);

        bool success = sigint_source_id_ != 0 && sigterm_source_id_ != 0 && sighup_source_id_ != 0;

        if (!success)
            unregister_signal_handlers();

        return success;
    }

    void Daemon::unregister_signal_handlers()
    {
        if (sigint_source_id_ != 0) {
            g_source_remove(sigint_source_id_);
            sigint_source_id_ = 0;
        }

        if (sigterm_source_id_ != 0) {
            g_source_remove(sigterm_source_id_);
            sigterm_source_id_ = 0;
        }

        if (sighup_source_id_ != 0) {
            g_source_remove(sighup_source_id_);
            sighup_source_id_ = 0;
        }
    }
}
