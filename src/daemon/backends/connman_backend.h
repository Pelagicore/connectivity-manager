// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#ifndef CONNECTIVITY_MANAGER_DAEMON_BACKENDS_CONNMAN_BACKEND_H
#define CONNECTIVITY_MANAGER_DAEMON_BACKENDS_CONNMAN_BACKEND_H

#include <giomm.h>
#include <glibmm.h>

#include <string>
#include <unordered_map>

#include "daemon/backend.h"
#include "daemon/backends/connman_agent.h"
#include "daemon/backends/connman_connect_queue.h"
#include "daemon/backends/connman_manager.h"
#include "daemon/backends/connman_service.h"
#include "daemon/backends/connman_technology.h"

namespace ConnectivityManager::Daemon
{
    // Backend implementation for ConnMan.
    //
    // See doc/overview-api.txt in the ConnMan repo for an overview of ConnMan's D-Bus API.
    //
    // Short overview of classes used by ConnManBackend to interact with ConnMan (path to
    // documentation in ConnMan repo in parenthesis if relevant):
    //
    // - ConnManManager (doc/manager-api.txt): Main entry point for accessing ConnMan over D-Bus.
    //
    // - ConnManTechnology (doc/technology-api.txt): Class for ConnMan technology objects on D-Bus.
    //
    // - ConnManService (doc/service-api.txt): Class for ConnMan services on D-Bus.
    //
    // - ConnManAgent (doc/agent-api.txt): D-Bus object called by ConnMan for passwords etc.
    //
    // - ConnManConnectQueue: Queued up connection requests if a connection is requested before
    //   ConnManBackend is ready to call ConnMan or a connection is pending.
    //
    // ConnManBackend owns a ConnManManager that tries to contact ConnMan on creation and will
    // signal to ConnManBackend when the ConnMan manager is available and disappears (e.g. the
    // ConnMan process is stopped). ConnManManager will request all technologies and services when
    // ConnMan is available on the bus and it will also listen for when technologies and services
    // are added or removed.
    //
    // ConnManBackend will create (and has ownership of) ConnManTechnology and ConnManService
    // instances and takes care of mapping ConnMan's technologies and services to Backend.
    //
    // ConnManAgent is registered with ConnMan as soon as ConnMan is available. If registration
    // fails or ConnMan for some reason deregistered the agent, ConnManBackend will try to register
    // it again when a connection attempt is made for a service.
    //
    // If the agent has not been successfully registered with ConnMan when connecting to a service,
    // the request will be queued up in ConnManConnectQueue. When a result is received from ConnMan
    // for the agent registration, the queue will be processed in FIFO order.
    //
    // Note that ConnMan uses strings in its D-Bus interface for SSID:s. Problematic since SSID:s
    // may not necessarily be UTF-8 (prior to the 2012 edition of the IEEE 802.11 standard) and it
    // is not allowed to send invalid UTF-8 strings over D-Bus. Current approach to handle this is
    // to replace invalid UTF-8 bytes with Unicode replacement character (U+FFFD). (Search for
    // string_to_valid_utf8() to see where conversion is needed.)
    class ConnManBackend final : public Backend,
                                 public ConnManManager::Listener,
                                 public ConnManAgent::Listener,
                                 public ConnManTechnology::Listener,
                                 public ConnManService::Listener
    {
    public:
        ConnManBackend();
        ~ConnManBackend() final;

        void wifi_enable() override;
        void wifi_disable() override;
        void wifi_connect(const WiFiAccessPoint &access_point,
                          ConnectFinished &&finished,
                          RequestCredentialsFromUser &&request_credentials) override;
        void wifi_disconnect(const WiFiAccessPoint &access_point) override;

        void wifi_hotspot_enable() override;
        void wifi_hotspot_disable() override;
        void wifi_hotspot_change_ssid(const std::string &ssid) override;
        void wifi_hotspot_change_passphrase(const Glib::ustring &passphrase) override;

    private:
        void wifi_technology_ready(ConnManTechnology &technology);
        void wifi_technology_removed();
        void wifi_technology_property_changed(ConnManTechnology::PropertyId id);

        // ConnManManager::Listner overrides and manager related methods.
        void manager_proxy_creation_failed() override;
        void manager_availability_changed(bool available) override;

        void manager_technology_add(const Glib::DBusObjectPathString &path,
                                    const ConnManTechnology::PropertyMap &properties) override;
        void manager_technology_remove(const Glib::DBusObjectPathString &path) override;

        void manager_service_add_or_change(const Glib::DBusObjectPathString &path,
                                           const ConnManService::PropertyMap &properties) override;
        void manager_service_remove(const Glib::DBusObjectPathString &path) override;

        void manager_register_agent_result(bool success) override;

        // ConnManAgent::Listener overrides and agent related methods.
        void agent_released() override;
        void agent_request_input(const Glib::DBusObjectPathString &service_path,
                                 Common::Credentials &&credentials,
                                 RequestCredentialsFromUserReply &&reply) override;

        void agent_register();

        // ConnManTechnology::Listener overrides and technology related methods.
        void technology_proxy_created(ConnManTechnology &technology) override;
        void technology_property_changed(ConnManTechnology &technology,
                                         ConnManTechnology::PropertyId id) override;

        // ConnManService::Listener overrides and service related methods.
        void service_proxy_created(ConnManService &service) override;
        void service_property_changed(ConnManService &service,
                                      ConnManService::PropertyId id) override;
        void service_connect_finished(ConnManService &service, bool success) override;

        void service_connect(ConnManService &service,
                             ConnectFinished &&finished,
                             RequestCredentialsFromUser &&request_credentials);

        WiFiAccessPoint *service_to_wifi_ap(ConnManService &service);
        ConnManService *service_from_wifi_ap(const WiFiAccessPoint &ap);

        ConnManManager manager_{*this};
        ConnManAgent agent_{*this};

        std::unordered_map<std::string, ConnManTechnology> technologies_;
        std::unordered_map<std::string, ConnManService> services_;

        ConnManTechnology *wifi_technology_ = nullptr;
        std::unordered_map<ConnManService *, WiFiAccessPoint::Id> wifi_service_to_ap_id_;

        ConnManConnectQueue connect_queue_;
    };
}

#endif // CONNECTIVITY_MANAGER_DAEMON_BACKENDS_CONNMAN_BACKEND_H
