// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#ifndef CONNECTIVITY_MANAGER_COMMON_SCOPED_SILENT_LOG_HANDLER_H
#define CONNECTIVITY_MANAGER_COMMON_SCOPED_SILENT_LOG_HANDLER_H

#include <glib.h>

namespace ConnectivityManager::Common
{
    // Helper for silencing logs. Should only be used in tests when testing failure code paths.
    //
    // TODO: Would be nice if we could get rid of this. Final decision on how we should log has
    //       not been made. But if we do something similar to what is in QtIVI for Glib we could
    //       make it so a DLT handler is installed when building for target, a "test handler" is
    //       installed when running tests (silence like here) and the default log handler in Glib is
    //       used when building for local host. Or something...
    class ScopedSilentLogHandler
    {
    public:
        ScopedSilentLogHandler() = default;

        ~ScopedSilentLogHandler()
        {
            g_log_set_default_handler(original_handler_, nullptr);
        }

        ScopedSilentLogHandler(const ScopedSilentLogHandler &other) = delete;
        ScopedSilentLogHandler(ScopedSilentLogHandler &&other) = delete;
        ScopedSilentLogHandler &operator=(const ScopedSilentLogHandler &other) = delete;
        ScopedSilentLogHandler &operator=(ScopedSilentLogHandler &&other) = delete;

    private:
        static void silent_handler(const gchar * /*log_domain*/,
                                   GLogLevelFlags /*log_level*/,
                                   const gchar * /*message*/,
                                   gpointer /*unused_data*/)
        {
        }

        GLogFunc original_handler_ = g_log_set_default_handler(silent_handler, nullptr);
    };
}

#endif // CONNECTIVITY_MANAGER_COMMON_SCOPED_SILENT_LOG_HANDLER_H
