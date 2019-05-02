// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#ifndef CONNECTIVITY_MANAGER_CLI_INPUT_HANDLER_H
#define CONNECTIVITY_MANAGER_CLI_INPUT_HANDLER_H

#include <glibmm.h>

#include <map>

#include "common/credentials.h"
#include "common/dbus.h"
#include "generated/dbus/connectivity_manager_stub.h"

namespace ConnectivityManager::Cli
{
    class InputHandler
    {
    public:
        static InputHandler &instance();

        bool register_user_input_agent(const Glib::RefPtr<Gio::DBus::Connection> &connection);

        static Glib::DBusObjectPathString user_input_agent_object_path()
        {
            return Glib::DBusObjectPathString(Glib::ustring(Common::DBus::MANAGER_OBJECT_PATH) +
                                              "/Cli/UserInputAgent");
        }

        void run() const;
        void quit() const;

        bool is_running() const;

        Common::Credentials prompt_for_credentials(const Glib::ustring &description_type,
                                                   const Glib::ustring &description_id,
                                                   const Common::Credentials &requested);

    private:
        class UserInputAgent : public com::luxoft::ConnectivityManager::UserInputAgentStub
        {
        public:
            UserInputAgent() = default;

            UserInputAgent(const UserInputAgent &other) = delete;
            UserInputAgent &operator=(const UserInputAgent &other) = delete;

        private:
            void RequestCredentials(const Glib::ustring &description_type,
                                    const Glib::ustring &description_id,
                                    const std::map<Glib::ustring, Glib::VariantBase> &requested,
                                    MethodInvocation &invocation) override;
        };

        Glib::RefPtr<Glib::MainLoop> main_loop_ = Glib::MainLoop::create();

        UserInputAgent user_input_agent_;
    };
}

#endif // CONNECTIVITY_MANAGER_CLI_INPUT_HANDLER_H
