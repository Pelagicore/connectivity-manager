// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#ifndef CONNECTIVITY_MANAGER_DAEMON_BACKENDS_CONNMAN_SERVICE_H
#define CONNECTIVITY_MANAGER_DAEMON_BACKENDS_CONNMAN_SERVICE_H

#include <giomm.h>
#include <glibmm.h>
#include <sigc++/sigc++.h>

#include <cstdint>
#include <map>

#include "generated/dbus/connman_proxy.h"

namespace ConnectivityManager::Daemon
{
    // Helper class for ConnMan services. See doc/service-api.txt in the ConnMan repo.
    //
    // Encapsulates asynchronous creation of D-Bus proxy and handling of properties.
    //
    // ConnMan does not use the standard org.freedesktop.DBus.Properties interface. The D-Bus
    // generator can not generate setters, getters and signals for ConnMan's custom properties
    // interface so it must be handled manually.
    //
    // Note: At the moment there is no need for setting service properties. If this changes,
    // something similar to what is done in ConnManTechnology::SettableProperty has to be done.
    // Perhaps easiest to copy ConnManTechnology::SettableProperty to ConnManService. A bit of code
    // repetition but proxy type, id enum etc. are different and making SettableProperty generic
    // probably adds more complexity than it is worth... ?
    class ConnManService : public sigc::trackable
    {
    public:
        using PropertyMap = std::map<Glib::ustring, Glib::VariantBase>;
        using Strength = std::uint8_t;

        enum class Type
        {
            UNKNOWN,

            BLUETOOTH,
            ETHERNET,
            WIFI
        };

        enum class State
        {
            IDLE,
            FAILURE,
            ASSOCIATION,
            CONFIGURATION,
            READY,
            DISCONNECT,
            ONLINE
        };

        enum class PropertyId
        {
            NAME,
            STATE,
            STRENGTH
        };

        class Listener;

        ConnManService(Listener &listener,
                       const Glib::DBusObjectPathString &path,
                       const PropertyMap &properties);
        ~ConnManService();

        bool proxy_created() const
        {
            return proxy_ ? true : false;
        }

        void properties_changed(const PropertyMap &properties);

        Type type() const
        {
            return type_;
        }

        const Glib::ustring &name() const
        {
            return name_;
        }

        State state() const
        {
            return state_;
        }

        Strength strength() const
        {
            return strength_;
        }

        void connect();
        void disconnect();

    private:
        using Proxy = net::connman::ServiceProxy;

        Glib::ustring log_id_str() const;

        void proxy_create_finish(const Glib::RefPtr<Gio::AsyncResult> &result);

        void property_changed(const Glib::ustring &property_name, const Glib::VariantBase &value);

        void connect_finish(const Glib::RefPtr<Gio::AsyncResult> &result);
        void disconnect_finish(const Glib::RefPtr<Gio::AsyncResult> &result);

        Listener &listener_;
        Glib::RefPtr<Proxy> proxy_;

        const Type type_ = Type::UNKNOWN;

        Glib::ustring name_;
        State state_ = State::IDLE;
        Strength strength_ = 0;
    };

    // Listener for service events.
    //
    // service_proxy_created() is guaranteed to be called before any other method in Listener.
    // ConnManBackend will not consider the service available until this has been done. This is to
    // make sure that it is not used by the rest of the program before the proxy has been created.
    //
    // service_property_changed() is only called for properties that can change (not called for
    // constant properties at creation). PropertyId only has entries for these properties. Note that
    // a service can have its properties updated through the net.connman.Manager.ServicesChanged
    // signal (see doc/manager-api.txt in the ConnMan repo), not only its own PropertyChanged
    // signal. ConnManService::property_changed() does not call service_property_changed() if the
    // proxy has not been created.
    //
    // service_connect_finished() will be called when result of Connect() is returned from ConnMan.
    class ConnManService::Listener
    {
    public:
        virtual ~Listener() = default;

        virtual void service_proxy_created(ConnManService &service) = 0;
        virtual void service_property_changed(ConnManService &service, PropertyId id) = 0;
        virtual void service_connect_finished(ConnManService &service, bool success) = 0;
    };
}

#endif // CONNECTIVITY_MANAGER_DAEMON_BACKENDS_CONNMAN_SERVICE_H
