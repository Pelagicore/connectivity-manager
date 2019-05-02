// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#ifndef CONNECTIVITY_MANAGER_DAEMON_BACKENDS_CONNMAN_MANAGER_H
#define CONNECTIVITY_MANAGER_DAEMON_BACKENDS_CONNMAN_MANAGER_H

#include <giomm.h>
#include <glibmm.h>
#include <sigc++/sigc++.h>

#include <tuple>
#include <vector>

#include "daemon/backends/connman_agent.h"
#include "daemon/backends/connman_service.h"
#include "daemon/backends/connman_technology.h"
#include "generated/dbus/connman_proxy.h"

namespace ConnectivityManager::Daemon
{
    // Helper class for ConnMan manager. See doc/manager-api.txt in the ConnMan repo.
    //
    // Encapsulates asynchronous creation of D-Bus proxy, listens for manager changes in ConnMan and
    // delegates creation of technologies and services etc.
    class ConnManManager : public sigc::trackable
    {
    public:
        class Listener;

        explicit ConnManManager(Listener &listener);

        Glib::RefPtr<Gio::DBus::Connection> dbus_connection() const
        {
            return proxy_->dbusProxy()->get_connection();
        }

        void register_agent(const ConnManAgent &agent);

    private:
        using Proxy = net::connman::ManagerProxy;

        using ServicePropertiesArray =
            std::vector<std::tuple<Glib::DBusObjectPathString, ConnManService::PropertyMap>>;
        using TechnologyPropertiesArray =
            std::vector<std::tuple<Glib::DBusObjectPathString, ConnManTechnology::PropertyMap>>;

        void proxy_create_finish(const Glib::RefPtr<Gio::AsyncResult> &result);

        void name_owner_changed() const;

        void get_technologies_finish(const Glib::RefPtr<Gio::AsyncResult> &result) const;
        void technology_added(const Glib::DBusObjectPathString &path,
                              const ConnManTechnology::PropertyMap &properties) const;
        void technology_removed(const Glib::DBusObjectPathString &path) const;

        void get_services_finish(const Glib::RefPtr<Gio::AsyncResult> &result) const;
        void services_changed(const ServicePropertiesArray &changed,
                              const std::vector<Glib::DBusObjectPathString> &removed) const;

        void register_agent_finish(const Glib::RefPtr<Gio::AsyncResult> &result);

        Listener &listener_;
        Glib::RefPtr<Proxy> proxy_;
    };

    // Listener for manager events.
    //
    // manager_proxy_creation_failed() is called if proxy creation fails. This should be considered
    // a critical error since there are currently no retries of creation the proxy (why should it
    // succeed the second time?). Might as well quit the application and retry when restarting.
    //
    // manager_availability_changed() with "available" set to true must be called before any other
    // methods are allowed to be invoked on the manager since it means that the D-Bus proxy has been
    // created. If ConnMan disconnects from the bus, manager_availability_changed() will be called
    // with "available" set to false. Another call will be made with "available" set to true when
    // ConnMan has been restarted and taken a name on the bus again.
    //
    // manager_technology_add(), manager_technology_remove(), manager_service_add_or_change() and
    // manager_service_remove() will be called when ConnMan adds/removes technologies and services
    // (+ changes services in some cases, see doc/manager-api.txt).
    //
    // manager_register_agent_result() will be called when result of register_agent() is returned.
    class ConnManManager::Listener
    {
    public:
        virtual ~Listener() = default;

        virtual void manager_proxy_creation_failed() = 0;
        virtual void manager_availability_changed(bool available) = 0;

        virtual void manager_technology_add(const Glib::DBusObjectPathString &path,
                                            const ConnManTechnology::PropertyMap &properties) = 0;
        virtual void manager_technology_remove(const Glib::DBusObjectPathString &path) = 0;

        virtual void manager_service_add_or_change(
            const Glib::DBusObjectPathString &path,
            const ConnManService::PropertyMap &properties) = 0;
        virtual void manager_service_remove(const Glib::DBusObjectPathString &path) = 0;

        virtual void manager_register_agent_result(bool success) = 0;
    };
}

#endif // CONNECTIVITY_MANAGER_DAEMON_BACKENDS_CONNMAN_MANAGER_H
