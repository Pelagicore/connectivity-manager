// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#include "daemon/dbus_objects/manager.h"

#include <giomm.h>
#include <glibmm.h>

#include <optional>
#include <utility>

#include "common/credentials.h"
#include "daemon/dbus_objects/wifi_access_point.h"

namespace ConnectivityManager::Daemon
{
    Manager::Manager(Backend &backend) : backend_(backend)
    {
    }

    void Manager::sync_with_backend(std::vector<Glib::DBusObjectPathString> &&wifi_access_points)
    {
        const Backend::State &state = backend_.state();

        wifi_.available = state.wifi.status != Backend::WiFiStatus::UNAVAILABLE;
        wifi_.enabled = state.wifi.status == Backend::WiFiStatus::ENABLED;
        wifi_.access_points = std::move(wifi_access_points);
        wifi_.hotspot_enabled = state.wifi.hotspot_status == Backend::WiFiHotspotStatus::ENABLED;
        wifi_.hotspot_ssid = state.wifi.hotspot_ssid;
        wifi_.hotspot_passphrase = state.wifi.hotspot_passphrase;
    }

    void Manager::Connect(const Glib::DBusObjectPathString &object,
                          const Glib::DBusObjectPathString &user_input_agent,
                          MethodInvocation &invocation)
    {
        if (pending_connects_.object_already_connecting(object)) {
            // TODO: This limitation can be removed.
            //
            // Only here now due to object path being used as key to lookup pending connections.
            // Could e.g. have PendingConnect::add() return a unique "token" (std::uint64_t
            // incremented for each add()) and use that as key instead of Glib::DBusObjectPathString
            // in PendingConnects::map_. Bind token by value into lambdas below in this method
            // instead of object path.
            invocation.ret(Gio::DBus::Error(
                Gio::DBus::Error::FAILED,
                "Can not connect \"" + object +
                    "\", already connecting (limitation will be removed in future version"));
            return;
        }

        if (auto backend_ap = wifi_backend_ap_from_object_path(object); backend_ap) {
            pending_connects_.add(object, invocation, user_input_agent);

            backend_.wifi_connect(
                *backend_ap,
                [this, object](Backend::ConnectResult result) {
                    pending_connects_.finished(object, result);
                },
                [this, object](const Common::Credentials::Requested &requested,
                               Backend::RequestCredentialsFromUserReply &&callback) {
                    pending_connects_.request_credentials(object, requested, std::move(callback));
                });

            return;
        }

        invocation.ret(Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS,
                                        "Can not connect \"" + object + "\", unknown object"));
    }

    void Manager::Disconnect(const Glib::DBusObjectPathString &object, MethodInvocation &invocation)
    {
        if (auto backend_ap = wifi_backend_ap_from_object_path(object); backend_ap) {
            backend_.wifi_disconnect(*backend_ap);
            invocation.ret();
            return;
        }

        invocation.ret(Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS,
                                        "Can not disconnect \"" + object + "\", unknown object"));
    }

    bool Manager::WiFiAvailable_setHandler(bool value)
    {
        bool changed = wifi_.available != value;
        wifi_.available = value;

        return changed;
    }

    bool Manager::WiFiAvailable_get()
    {
        return wifi_.available;
    }

    bool Manager::WiFiEnabled_setHandler(bool value)
    {
        if (value && !backend_.wifi_available())
            throw Gio::DBus::Error(
                Gio::DBus::Error::FAILED,
                "Unable to set WiFiEnabled property to true, WiFi not available");

        bool changed = wifi_.enabled != value;
        wifi_.enabled = value;

        if (wifi_.enabled != backend_.wifi_enabled()) {
            if (wifi_.enabled)
                backend_.wifi_enable();
            else
                backend_.wifi_disable();
        }

        return changed;
    }

    bool Manager::WiFiEnabled_get()
    {
        return wifi_.enabled;
    }

    bool Manager::WiFiAccessPoints_setHandler(const std::vector<Glib::DBusObjectPathString> &value)
    {
        bool changed = wifi_.access_points != value;
        wifi_.access_points = value;

        return changed;
    }

    std::vector<Glib::DBusObjectPathString> Manager::WiFiAccessPoints_get()
    {
        return wifi_.access_points;
    }

    bool Manager::WiFiHotspotEnabled_setHandler(bool value)
    {
        if (value && !backend_.wifi_available())
            throw Gio::DBus::Error(
                Gio::DBus::Error::FAILED,
                "Unable to set WiFiHotspotEnabled property to true, WiFi not available");

        bool changed = wifi_.hotspot_enabled != value;
        wifi_.hotspot_enabled = value;

        if (wifi_.hotspot_enabled != backend_.wifi_hotspot_enabled()) {
            if (wifi_.hotspot_enabled)
                backend_.wifi_hotspot_enable();
            else
                backend_.wifi_hotspot_disable();
        }

        return changed;
    }

    bool Manager::WiFiHotspotEnabled_get()
    {
        return wifi_.hotspot_enabled;
    }

    bool Manager::WiFiHotspotSSID_setHandler(const std::string &value)
    {
        if (!backend_.wifi_available())
            throw Gio::DBus::Error(Gio::DBus::Error::FAILED,
                                   "Unable to set WiFiHotspotSSID property, WiFi not available");

        bool changed = wifi_.hotspot_ssid != value;
        wifi_.hotspot_ssid = value;

        if (backend_.state().wifi.hotspot_ssid != value)
            backend_.wifi_hotspot_change_ssid(value);

        return changed;
    }

    std::string Manager::WiFiHotspotSSID_get()
    {
        return wifi_.hotspot_ssid;
    }

    bool Manager::WiFiHotspotPassphrase_setHandler(const Glib::ustring &value)
    {
        if (!backend_.wifi_available())
            throw Gio::DBus::Error(
                Gio::DBus::Error::FAILED,
                "Unable to set WiFiHotspotPassphrase property, WiFi not available");

        bool changed = wifi_.hotspot_passphrase != value;
        wifi_.hotspot_passphrase = value;

        if (backend_.state().wifi.hotspot_passphrase != value)
            backend_.wifi_hotspot_change_passphrase(value);

        return changed;
    }

    Glib::ustring Manager::WiFiHotspotPassphrase_get()
    {
        return wifi_.hotspot_passphrase;
    }

    const Backend::WiFiAccessPoint *Manager::wifi_backend_ap_from_object_path(
        const Glib::DBusObjectPathString &path) const
    {
        auto id = WiFiAccessPoint::object_path_to_id(path);
        if (!id)
            return nullptr;

        const auto &access_points = backend_.state().wifi.access_points;

        auto i = access_points.find(*id);
        if (i == access_points.cend())
            return nullptr;

        return &i->second;
    }

    void Manager::PendingConnects::add(const Glib::DBusObjectPathString &object,
                                       MethodInvocation &invocation,
                                       const Glib::DBusObjectPathString &user_input_agent_path)
    {
        PendingConnect pending;

        pending.invocation = invocation;

        pending.user_input_agent_path = user_input_agent_path;
        pending.user_input_agent_name_watcher =
            DBusNameWatcher(invocation.getMessage()->get_connection(),
                            invocation.getMessage()->get_sender(),
                            [this, object](const auto & /*connection*/, const auto & /*name*/) {
                                user_input_agent_proxy_name_disappeared(object);
                            });

        map_.emplace(object, std::move(pending));
    }

    bool Manager::PendingConnects::object_already_connecting(
        const Glib::DBusObjectPathString &object) const
    {
        return map_.find(object) != map_.cend();
    }

    Manager::PendingConnects::PendingConnect *Manager::PendingConnects::find(
        const Glib::DBusObjectPathString &object)
    {
        auto i = map_.find(object);
        return i == map_.cend() ? nullptr : &i->second;
    }

    void Manager::PendingConnects::remove(const Glib::DBusObjectPathString &object)
    {
        map_.erase(object);
    }

    void Manager::PendingConnects::finished(const Glib::DBusObjectPathString &object,
                                            Backend::ConnectResult result)
    {
        PendingConnect *pending = find(object);
        if (!pending)
            return;

        if (result == Backend::ConnectResult::SUCCESS)
            pending->invocation->ret();
        else
            pending->invocation->ret(
                Gio::DBus::Error(Gio::DBus::Error::FAILED, "Failed to connect to " + object));

        if (pending->credentials_reply) {
            pending->credentials_reply(Common::Credentials::NONE);
            pending->credentials_reply = nullptr;
        }

        remove(object);
    }

    void Manager::PendingConnects::request_credentials(
        const Glib::DBusObjectPathString &object,
        const Common::Credentials::Requested &requested,
        Backend::RequestCredentialsFromUserReply &&callback)
    {
        PendingConnect *pending = find(object);

        if (!pending || pending->user_input_agent_path.empty()) {
            callback(Common::Credentials::NONE);
            return;
        }

        pending->credentials_requested = requested;
        pending->credentials_reply = std::move(callback);

        UserInputAgentProxy::createForBus(
            Gio::DBus::BUS_TYPE_SYSTEM,
            Gio::DBus::PROXY_FLAGS_NONE,
            pending->invocation->getMessage()->get_sender(),
            pending->user_input_agent_path,
            [this, object](const auto &result) { user_input_agent_proxy_ready(object, result); });
    }

    void Manager::PendingConnects::user_input_agent_proxy_name_disappeared(
        const Glib::DBusObjectPathString &object)
    {
        PendingConnect *pending = find(object);
        if (!pending)
            return;

        pending->user_input_agent_path.clear();
    }

    void Manager::PendingConnects::user_input_agent_proxy_ready(
        const Glib::DBusObjectPathString &object,
        const Glib::RefPtr<Gio::AsyncResult> &result)
    {
        Glib::RefPtr<UserInputAgentProxy> proxy;

        try {
            proxy = UserInputAgentProxy::createForBusFinish(result);
        } catch (const Glib::Error &e) {
            g_warning("Failed to create UserInputAgentProxy for %s: %s",
                      object.c_str(),
                      e.what().c_str());
        }

        PendingConnect *pending = find(object);
        if (!pending)
            return;

        if (!proxy) {
            if (pending->credentials_reply) {
                pending->credentials_reply(Common::Credentials::NONE);
                pending->credentials_reply = nullptr;
            }
            return;
        }

        constexpr int REQUEST_TIMEOUT_MS = 5 * 60 * 1000;

        proxy->RequestCredentials(
            pending->credentials_requested.description_type,
            pending->credentials_requested.description_id,
            Common::Credentials::to_dbus_value(pending->credentials_requested.credentials),
            [this, object, proxy](const auto &request_result) {
                credentials_reply_received(object, proxy, request_result);
            },
            {},
            REQUEST_TIMEOUT_MS);
    }

    void Manager::PendingConnects::credentials_reply_received(
        const Glib::DBusObjectPathString &object,
        const Glib::RefPtr<UserInputAgentProxy> &proxy,
        const Glib::RefPtr<Gio::AsyncResult> &result)
    {
        Common::Credentials::DBusValue dbus_value;

        try {
            proxy->RequestCredentials_finish(dbus_value, result);
        } catch (const Glib::Error &e) {
            g_warning("RequestCredentials() for %s failed: %s", object.c_str(), e.what().c_str());
        }

        PendingConnect *pending = find(object);
        if (!pending || !pending->credentials_reply)
            return;

        std::optional<Common::Credentials> reply;
        if (!dbus_value.empty())
            reply = Common::Credentials::from_dbus_value(dbus_value);

        pending->credentials_reply(reply);
        pending->credentials_reply = nullptr;
    }
}
