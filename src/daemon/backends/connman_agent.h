// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#ifndef CONNECTIVITY_MANAGER_DAEMON_BACKENDS_CONNMAN_AGENT_H
#define CONNECTIVITY_MANAGER_DAEMON_BACKENDS_CONNMAN_AGENT_H

#include <giomm.h>
#include <glibmm.h>

#include <map>
#include <optional>

#include "common/credentials.h"
#include "daemon/backend.h"
#include "generated/dbus/connman_stub.h"

namespace ConnectivityManager::Daemon
{
    // Implementation of ConnMan agent object. See doc/agent-api.txt in the ConnMan repo.
    //
    // Called by ConnMan to ask for password etc. Not all methods are implemented.
    // org.freedesktop.DBus.Error.UnknownMethod will be returned for methods that are left out.
    //
    // Exposed on D-Bus under /com/luxoft/ConnectivityManager/ConnManAgent.
    class ConnManAgent : private net::connman::AgentStub
    {
    public:
        class Listener;

        enum class State
        {
            NOT_REGISTERED_WITH_MANAGER,
            REGISTERING_WITH_MANAGER,
            REGISTERED_WITH_MANAGER
        };

        explicit ConnManAgent(Listener &listener);

        Glib::ustring object_path() const;

        bool register_object(const Glib::RefPtr<Gio::DBus::Connection> &connection);
        bool registered_object() const
        {
            return usage_count() > 0;
        }

        State state() const
        {
            return state_;
        }

        void set_state(State state)
        {
            state_ = state;
        }

    private:
        using Fields = std::map<Glib::ustring, Glib::VariantBase>;

        void Release(MethodInvocation &invocation) override;

        void ReportError(const Glib::DBusObjectPathString &service,
                         const Glib::ustring &error,
                         MethodInvocation &invocation) override;

        void RequestBrowser(const Glib::DBusObjectPathString &service,
                            const Glib::ustring &url,
                            MethodInvocation &invocation) override;

        void RequestInput(const Glib::DBusObjectPathString &service,
                          const Fields &fields,
                          MethodInvocation &invocation) override;

        void Cancel(MethodInvocation &invocation) override;

        Listener &listener_;
        State state_ = State::NOT_REGISTERED_WITH_MANAGER;
    };

    // Listener for agent events.
    //
    // The way ConnMan is implemented now, agent_released() will only be called when ConnMan exits
    // cleanly. Need to clear any pending service connects that may rely on agent being registered
    // if this ever changes (instead of relying on only clearing pending connects when ConnMan
    // disappears from the bus.)
    //
    // agent_request_input() will be called when ConnMan requests "credentials". The reply callback
    // _must_ be called to inform ConnMan of the result.
    class ConnManAgent::Listener
    {
    public:
        virtual ~Listener() = default;

        virtual void agent_released() = 0;
        virtual void agent_request_input(const Glib::DBusObjectPathString &service_path,
                                         Common::Credentials &&credentials,
                                         Backend::RequestCredentialsFromUserReply &&reply) = 0;
    };
}

#endif // CONNECTIVITY_MANAGER_DAEMON_BACKENDS_CONNMAN_AGENT_H
