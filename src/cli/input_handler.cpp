// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#include "cli/input_handler.h"

#include <giomm.h>
#include <glibmm.h>

#include <cassert>
#include <iostream>
#include <string>

namespace ConnectivityManager::Cli
{
    namespace
    {
        std::string prompt_for_value(const std::string &what, const std::string &default_value)
        {
            std::string prompt = what;
            std::string result;

            if (!default_value.empty()) {
                prompt += " (default: \"" + default_value + "\")";
            }
            prompt += ": ";

            std::cout << prompt;
            std::getline(std::cin, result);

            if (result.empty()) {
                result = default_value;
            }

            return result;
        }

        std::string password_type_str(Common::Credentials::Password::Type type)
        {
            std::string str = "unknown";

            switch (type) {
            case Common::Credentials::Password::Type::PASSPHRASE:
                str = "passphrase";
                break;
            case Common::Credentials::Password::Type::WPA_PSK:
                str = "WPA PSK";
                break;
            case Common::Credentials::Password::Type::WEP_KEY:
                str = "WEP key";
                break;
            case Common::Credentials::Password::Type::WPS_PIN:
                str = "WPS pin";
                break;
            }

            return str;
        }
    }

    InputHandler &InputHandler::instance()
    {
        static InputHandler input_handler;
        return input_handler;
    }

    bool InputHandler::register_user_input_agent(
        const Glib::RefPtr<Gio::DBus::Connection> &connection)
    {
        if (user_input_agent_.usage_count() == 0) {
            user_input_agent_.register_object(connection, user_input_agent_object_path());
        }

        return user_input_agent_.usage_count() != 0;
    }

    void InputHandler::run() const
    {
        assert(user_input_agent_.usage_count() != 0);

        main_loop_->run();
    }

    void InputHandler::quit() const
    {
        main_loop_->quit();
    }

    bool InputHandler::is_running() const
    {
        return main_loop_->is_running();
    }

    Common::Credentials InputHandler::prompt_for_credentials(const Glib::ustring &description_type,
                                                             const Glib::ustring &description_id,
                                                             const Common::Credentials &requested)
    {
        Common::Credentials credentials;

        std::cout << "Enter credentials for " << description_type;
        if (!description_id.empty()) {
            std::cout << " " << description_id;
        }
        std::cout << '\n';

        if (requested.ssid) {
            credentials.ssid = prompt_for_value("SSID", *requested.ssid);
        }

        if (requested.username) {
            credentials.username = prompt_for_value("Username", *requested.username);
        }

        if (requested.password) {
            auto type = requested.password->type;
            bool alternative_available = requested.password_alternative.has_value();
            const std::string want_alternative_str = "a";

            std::string what = "Password (" + password_type_str(type);
            if (alternative_available) {
                what += ", '" + want_alternative_str + "' to use alternative";
            }
            what += ")";

            Glib::ustring value = prompt_for_value(what, requested.password->value);

            if (alternative_available && value == want_alternative_str) {
                type = requested.password_alternative->type;
                what = "Password (" + password_type_str(type) + ", alternative)";
                value = prompt_for_value(what, requested.password->value);
            }

            credentials.password = {type, value};
        }

        return credentials;
    }

    void InputHandler::UserInputAgent::RequestCredentials(
        const Glib::ustring &description_type,
        const Glib::ustring &description_id,
        const std::map<Glib::ustring, Glib::VariantBase> &requested,
        MethodInvocation &invocation)
    {
        auto requested_credentials = Common::Credentials::from_dbus_value(requested);

        if (!requested_credentials) {
            invocation.ret(Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS,
                                            "Received invalid \"requested\" argument"));
            return;
        }

        if (!InputHandler::instance().is_running()) {
            invocation.ret(Gio::DBus::Error(Gio::DBus::Error::FAILED,
                                            "Unexpected request, not ready to ask user for input"));
            return;
        }

        Common::Credentials credentials = InputHandler::instance().prompt_for_credentials(
            description_type, description_id, *requested_credentials);

        invocation.ret(Common::Credentials::to_dbus_value(credentials));
    }
}
