// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#ifndef CONNECTIVITY_MANAGER_DAEMON_DBUS_OBJECTS_MANAGER_H
#define CONNECTIVITY_MANAGER_DAEMON_DBUS_OBJECTS_MANAGER_H

#include <giomm.h>
#include <glibmm.h>

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/credentials.h"
#include "daemon/backend.h"
#include "daemon/dbus_name_watcher.h"
#include "generated/dbus/connectivity_manager_proxy.h"
#include "generated/dbus/connectivity_manager_stub.h"

namespace ConnectivityManager::Daemon
{
    class Manager : public com::luxoft::ConnectivityManagerStub
    {
    public:
        explicit Manager(Backend &backend);

        void sync_with_backend(std::vector<Glib::DBusObjectPathString> &&wifi_access_points);

        void Connect(const Glib::DBusObjectPathString &object,
                     const Glib::DBusObjectPathString &user_input_agent,
                     MethodInvocation &invocation) override;

        void Disconnect(const Glib::DBusObjectPathString &object,
                        MethodInvocation &invocation) override;

        bool WiFiAvailable_setHandler(bool value) override;
        bool WiFiAvailable_get() override;

        bool WiFiEnabled_setHandler(bool value) override;
        bool WiFiEnabled_get() override;

        bool WiFiAccessPoints_setHandler(
            const std::vector<Glib::DBusObjectPathString> &value) override;
        std::vector<Glib::DBusObjectPathString> WiFiAccessPoints_get() override;

        bool WiFiHotspotEnabled_setHandler(bool value) override;
        bool WiFiHotspotEnabled_get() override;

        bool WiFiHotspotSSID_setHandler(const std::string &value) override;
        std::string WiFiHotspotSSID_get() override;

        bool WiFiHotspotPassphrase_setHandler(const Glib::ustring &value) override;
        Glib::ustring WiFiHotspotPassphrase_get() override;

    private:
        // Information stored for calls to Connect().
        //
        // Connect() should not return with result to caller until connecting either succeeds or
        // fails. MethodInvocation is stored so it can be used when Backend returns with result.
        // Also stores path to com.luxoft.ConnectivityManager.UserInputAgent object provided by
        // client in Connect() call and monitors if client disappears from the bus.
        class PendingConnects
        {
        public:
            PendingConnects() = default;

            PendingConnects(const PendingConnects &other) = delete;
            PendingConnects(PendingConnects &&other) = delete;
            PendingConnects &operator=(const PendingConnects &other) = delete;
            PendingConnects &operator=(PendingConnects &&other) = delete;

            void add(const Glib::DBusObjectPathString &object,
                     MethodInvocation &invocation,
                     const Glib::DBusObjectPathString &user_input_agent_path);

            bool object_already_connecting(const Glib::DBusObjectPathString &object) const;

            void finished(const Glib::DBusObjectPathString &object, Backend::ConnectResult result);

            void request_credentials(const Glib::DBusObjectPathString &object,
                                     const Common::Credentials::Requested &requested,
                                     Backend::RequestCredentialsFromUserReply &&callback);

        private:
            using UserInputAgentProxy = com::luxoft::ConnectivityManager::UserInputAgentProxy;

            struct PendingConnect
            {
                std::optional<MethodInvocation> invocation;

                Glib::DBusObjectPathString user_input_agent_path;
                DBusNameWatcher user_input_agent_name_watcher;

                Common::Credentials::Requested credentials_requested;
                Backend::RequestCredentialsFromUserReply credentials_reply;
            };

            PendingConnect *find(const Glib::DBusObjectPathString &object);

            void remove(const Glib::DBusObjectPathString &object);

            void user_input_agent_proxy_name_disappeared(const Glib::DBusObjectPathString &object);

            void user_input_agent_proxy_ready(const Glib::DBusObjectPathString &object,
                                              const Glib::RefPtr<Gio::AsyncResult> &result);

            void credentials_reply_received(const Glib::DBusObjectPathString &object,
                                            const Glib::RefPtr<UserInputAgentProxy> &proxy,
                                            const Glib::RefPtr<Gio::AsyncResult> &result);

        private:
            std::unordered_map<std::string, PendingConnect> map_;
        };

        const Backend::WiFiAccessPoint *wifi_backend_ap_from_object_path(
            const Glib::DBusObjectPathString &path) const;

        Backend &backend_;

        struct
        {
            bool available = false;

            bool enabled = false;

            std::vector<Glib::DBusObjectPathString> access_points;

            bool hotspot_enabled = false;
            std::string hotspot_ssid;
            Glib::ustring hotspot_passphrase;
        } wifi_;

        PendingConnects pending_connects_;
    };
}

#endif // CONNECTIVITY_MANAGER_DAEMON_DBUS_OBJECTS_MANAGER_H
