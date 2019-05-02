// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#ifndef CONNECTIVITY_MANAGER_COMMON_STRING_TO_VALID_UTF8_H
#define CONNECTIVITY_MANAGER_COMMON_STRING_TO_VALID_UTF8_H

#include <glibmm.h>

#include <string>

namespace ConnectivityManager::Common
{
    Glib::ustring string_to_valid_utf8(const std::string &str);
}

#endif // CONNECTIVITY_MANAGER_COMMON_STRING_TO_VALID_UTF8_H
