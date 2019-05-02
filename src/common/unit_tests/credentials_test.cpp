// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#include "common/credentials.h"

#include <glibmm.h>
#include <gtest/gtest.h>

#include "common/scoped_silent_log_handler.h"

namespace ConnectivityManager::Common
{
    TEST(Credentials, ToFromDBusValuePreservsAllFields)
    {
        Credentials original;
        original.ssid = "Test SSID";
        original.username = "Test username";
        original.password = {Credentials::Password::Type::WPA_PSK, "Test WPA PSK"};
        original.password_alternative = {Credentials::Password::Type::WPS_PIN, "Test WPS PIN"};

        Credentials::DBusValue dbus_value = Credentials::to_dbus_value(original);
        std::optional<Credentials> converted = Credentials::from_dbus_value(dbus_value);

        ASSERT_TRUE(converted.has_value());

        EXPECT_EQ(original.ssid, converted->ssid);

        EXPECT_EQ(original.username, converted->username);

        EXPECT_EQ(original.password->type, converted->password->type);
        EXPECT_EQ(original.password->value, converted->password->value);

        EXPECT_EQ(original.password_alternative->type, converted->password_alternative->type);
        EXPECT_EQ(original.password_alternative->value, converted->password_alternative->value);
    }

    TEST(Credentials, DBusValueEmptyNotAllowed)
    {
        ScopedSilentLogHandler log_handler;
        Credentials::DBusValue empty;

        EXPECT_FALSE(Credentials::from_dbus_value(empty).has_value());
    }

    TEST(Credentials, DBusValueWithUnknownValueTypeNotAllowed)
    {
        ScopedSilentLogHandler log_handler;
        Credentials::DBusValue invalid = {{"unknown", Glib::Variant<Glib::ustring>::create("")}};

        EXPECT_FALSE(Credentials::from_dbus_value(invalid).has_value());
    }

    TEST(Credentials, DBusValueSSIDMustBeByteArray)
    {
        ScopedSilentLogHandler log_handler;
        const Glib::ustring type = "ssid";
        Credentials::DBusValue valid = {{type, Glib::Variant<std::string>::create("An SSID")}};
        Credentials::DBusValue invalid = {{type, Glib::Variant<int>::create(0)}};

        EXPECT_TRUE(Credentials::from_dbus_value(valid).has_value());
        EXPECT_FALSE(Credentials::from_dbus_value(invalid).has_value());
    }

    TEST(Credentials, DBusValueUsernameMustBeUTF8String)
    {
        ScopedSilentLogHandler log_handler;
        const Glib::ustring type = "username";
        Credentials::DBusValue valid = {{type, Glib::Variant<Glib::ustring>::create("A Name")}};
        Credentials::DBusValue invalid = {{type, Glib::Variant<int>::create(0)}};

        EXPECT_TRUE(Credentials::from_dbus_value(valid).has_value());
        EXPECT_FALSE(Credentials::from_dbus_value(invalid).has_value());
    }

    TEST(Credentials, DBusValueWithUnknownPasswordTypeNotAllowed)
    {
        ScopedSilentLogHandler log_handler;
        const Glib::ustring type = "password";
        auto invalid_password = Glib::Variant<std::tuple<Glib::ustring, Glib::ustring>>::create(
            {"unknown_password_type", "1"});
        Credentials::DBusValue invalid = {{type, invalid_password}};

        EXPECT_FALSE(Credentials::from_dbus_value(invalid).has_value());
    }

    TEST(Credentials, DBusValuePasswordValueMustBeUTF8String)
    {
        ScopedSilentLogHandler log_handler;
        const Glib::ustring type = "password";
        const Glib::ustring password_type = "passphrase";
        auto valid_password =
            Glib::Variant<std::tuple<Glib::ustring, Glib::ustring>>::create({password_type, "123"});
        Credentials::DBusValue valid = {{type, valid_password}};
        auto invalid_password =
            Glib::Variant<std::tuple<Glib::ustring, std::string>>::create({password_type, "123"});
        Credentials::DBusValue invalid = {{type, invalid_password}};

        EXPECT_TRUE(Credentials::from_dbus_value(valid).has_value());
        EXPECT_FALSE(Credentials::from_dbus_value(invalid).has_value());
    }
}
