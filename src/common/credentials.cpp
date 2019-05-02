// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#include "common/credentials.h"

#include <glibmm.h>

#include <optional>
#include <string>
#include <tuple>

namespace ConnectivityManager::Common
{
    namespace
    {
        // Must match key strings in UserInputAgent D-Bus interface.
        constexpr char VALUE_TYPE_SSID_STR[] = "ssid";
        constexpr char VALUE_TYPE_USERNAME_STR[] = "username";
        constexpr char VALUE_TYPE_PASSWORD_STR[] = "password";
        constexpr char VALUE_TYPE_PASSWORD_ALTERNATIVE_STR[] = "password_alternative";

        // Must match password type strings in UserInputAgent D-Bus interface.
        constexpr char PASSWORD_TYPE_PASSPHRASE_STR[] = "passphrase";
        constexpr char PASSWORD_TYPE_WPA_PSK_STR[] = "wpa_psk";
        constexpr char PASSWORD_TYPE_WEP_KEY_STR[] = "wep_key";
        constexpr char PASSWORD_TYPE_WPS_PIN_STR[] = "wps_pin";

        using PasswordDBusValue = std::tuple<Glib::ustring, Glib::ustring>;

        template <typename T>
        std::optional<T> value_from_variant(const Glib::VariantBase &variant,
                                            const Glib::ustring &value_name)
        {
            try {
                return Glib::VariantBase::cast_dynamic<Glib::Variant<T>>(variant).get();
            } catch (const std::bad_cast &) {
                g_warning("Unexpected type for %s in credentials D-Bus value", value_name.c_str());
            }
            return {};
        }

        std::optional<Credentials::Password> password_from_variant(const Glib::VariantBase &variant,
                                                                   const Glib::ustring &value_name)
        {
            auto dbus_value = value_from_variant<PasswordDBusValue>(variant, value_name);
            if (!dbus_value)
                return {};

            Credentials::Password password;
            const Glib::ustring &type_str = std::get<0>(*dbus_value);

            if (type_str == PASSWORD_TYPE_PASSPHRASE_STR) {
                password.type = Credentials::Password::Type::PASSPHRASE;
            } else if (type_str == PASSWORD_TYPE_WPA_PSK_STR) {
                password.type = Credentials::Password::Type::WPA_PSK;
            } else if (type_str == PASSWORD_TYPE_WEP_KEY_STR) {
                password.type = Credentials::Password::Type::WEP_KEY;
            } else if (type_str == PASSWORD_TYPE_WPS_PIN_STR) {
                password.type = Credentials::Password::Type::WPS_PIN;
            } else {
                g_warning("Unknown password type \"%s\" in credentials D-Bus value",
                          type_str.c_str());
                return {};
            }

            password.value = std::get<1>(*dbus_value);

            return password;
        }

        Glib::VariantBase password_to_variant(const Credentials::Password &password)
        {
            Glib::ustring type_str;

            switch (password.type) {
            case Credentials::Password::Type::PASSPHRASE:
                type_str = PASSWORD_TYPE_PASSPHRASE_STR;
                break;
            case Credentials::Password::Type::WPA_PSK:
                type_str = PASSWORD_TYPE_WPA_PSK_STR;
                break;
            case Credentials::Password::Type::WEP_KEY:
                type_str = PASSWORD_TYPE_WEP_KEY_STR;
                break;
            case Credentials::Password::Type::WPS_PIN:
                type_str = PASSWORD_TYPE_WPS_PIN_STR;
                break;
            }

            return Glib::Variant<PasswordDBusValue>::create({type_str, password.value});
        }
    }

    std::optional<Credentials> Credentials::from_dbus_value(const DBusValue &dbus_value)
    {
        if (dbus_value.empty()) {
            g_warning("Credentials D-Bus value must contain at least one entry");
            return {};
        }

        Credentials credentials;

        for (const auto &[type, variant] : dbus_value) {
            if (type == VALUE_TYPE_SSID_STR) {
                credentials.ssid = value_from_variant<std::string>(variant, type);
                if (!credentials.ssid)
                    return {};

            } else if (type == VALUE_TYPE_USERNAME_STR) {
                credentials.username = value_from_variant<Glib::ustring>(variant, type);
                if (!credentials.username)
                    return {};

            } else if (type == VALUE_TYPE_PASSWORD_STR) {
                credentials.password = password_from_variant(variant, type);
                if (!credentials.password)
                    return {};

            } else if (type == VALUE_TYPE_PASSWORD_ALTERNATIVE_STR) {
                credentials.password_alternative = password_from_variant(variant, type);
                if (!credentials.password_alternative)
                    return {};

            } else {
                g_warning("Unknown value type \"%s\" in credentials D-Bus value", type.c_str());
                return {};
            }
        }

        return credentials;
    }

    Credentials::DBusValue Credentials::to_dbus_value(const Credentials &credentials)
    {
        DBusValue dbus_value;

        if (credentials.ssid)
            dbus_value.emplace(VALUE_TYPE_SSID_STR,
                               Glib::Variant<std::string>::create(*credentials.ssid));

        if (credentials.username)
            dbus_value.emplace(VALUE_TYPE_USERNAME_STR,
                               Glib::Variant<Glib::ustring>::create(*credentials.username));

        if (credentials.password)
            dbus_value.emplace(VALUE_TYPE_PASSWORD_STR, password_to_variant(*credentials.password));

        if (credentials.password_alternative)
            dbus_value.emplace(VALUE_TYPE_PASSWORD_ALTERNATIVE_STR,
                               password_to_variant(*credentials.password_alternative));

        return dbus_value;
    }
}
