// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#include "common/string_to_valid_utf8.h"

#include <glib.h>

namespace ConnectivityManager::Common
{
    // TODO: Glib::ustring::make_valid() is in Glibmm master, not backported to stable release yet.
    //       This helper function should be removed when it is or we have started to require
    //       glibmm >= 2.62. Backporting issue: https://gitlab.gnome.org/GNOME/glibmm/issues/40
    Glib::ustring string_to_valid_utf8(const std::string &str)
    {
        g_autofree char *valid = g_utf8_make_valid(str.c_str(), -1);
        return Glib::ustring(valid);
    }
}
