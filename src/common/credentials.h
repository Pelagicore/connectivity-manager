// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#ifndef CONNECTIVITY_MANAGER_COMMON_CREDENTIALS_H
#define CONNECTIVITY_MANAGER_COMMON_CREDENTIALS_H

#include <glibmm.h>

#include <map>
#include <optional>
#include <string>

namespace ConnectivityManager::Common
{
    // Credentials for logging in to Wi-Fi access points etc. See the RequestCredentials method in
    // the com.luxoft.ConnectivityManager.UserInputAgent D-Bus interface for details.
    //
    // When credentials are requested, a Credentials::Requested struct should be used. See below.
    // The std::optional entries that are wanted will have a value. If there was a previous value
    // suitable to present to user as a default value, the entry will contain that value.
    //
    // When replying to a request, the entries filled in by the user should be set. Default values
    // can be copied from the received Credentials struct.
    struct Credentials
    {
        using DBusValue = std::map<Glib::ustring, Glib::VariantBase>;

        struct Password
        {
            enum class Type
            {
                PASSPHRASE,
                WPA_PSK,
                WEP_KEY,
                WPS_PIN
            };

            Type type = Type::PASSPHRASE;
            Glib::ustring value;
        };

        struct Requested;

        static constexpr std::nullopt_t NONE = std::nullopt;

        static std::optional<Credentials> from_dbus_value(const DBusValue &dbus_value);
        static DBusValue to_dbus_value(const Credentials &credentials);

        std::optional<std::string> ssid;

        std::optional<Glib::ustring> username;

        std::optional<Password> password;
        std::optional<Password> password_alternative; // Not set in replies. See RequestCredentials.
    };

    // Helper struct for connecting Credentials request with description of what it is for. String
    // constants are part of D-Bus API and are meant to be translated when presented to user. See
    // RequestCredentials method in the com.luxoft.ConnectivityManager.UserInputAgent interface.
    struct Credentials::Requested
    {
        static constexpr char TYPE_NETWORK[] = "network";
        static constexpr char TYPE_WIRELESS_NETWORK[] = "wireless network";
        static constexpr char TYPE_HIDDEN_WIRELESS_NETWORK[] = "hidden wireless network";

        Glib::ustring description_type;
        Glib::ustring description_id;

        Credentials credentials;
    };
}

#endif // CONNECTIVITY_MANAGER_COMMON_CREDENTIALS_H
