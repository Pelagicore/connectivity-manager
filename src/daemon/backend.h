// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#ifndef CONNECTIVITY_MANAGER_DAEMON_BACKEND_H
#define CONNECTIVITY_MANAGER_DAEMON_BACKEND_H

#include <glibmm.h>
#include <sigc++/sigc++.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

#include "common/credentials.h"

namespace ConnectivityManager::Daemon
{
    // Abstract base class for backend.
    //
    // Contains state, signals and helper methods for concrete backends to use to set state and emit
    // signals when state changes.
    //
    // = Callbacks
    //
    // Callbacks are used to handle that connecting to a network should be asynchronous and are
    // passed to *_connect() (currently only wifi_connect() exists but more will probably be added):
    //
    // - ConnectFinished: Called when connect requested has either succeeded or failed.
    //
    // - RequestCredentialsFromUser: Called when a connect request requires credentials to be
    //   requested from user. E.g. a passphrase for a Wi-Fi access point needs to be entered.
    //   See Common::Credentials::Requested for description of what to request.
    //
    // - RequestCredentialsFromUserReply: Passed to RequestCredentialsFromUser and should be called
    //   when user has replied. If something fails this callback must be called with
    //   Common::Credentials::NONE to notify the backend about the failure.
    //
    // = WiFi
    //
    // WiFiStatus must be set to something other than UNAVAILABLE before calling any of the public
    // virtual wifi_* methods. A backend implementation should not do anything if this rule is not
    // followed and call site should be fixed.
    //
    // WiFiAccessPoint::Event::ADDED/REMOVED_ALL are used when WiFi is enabled/disabled to limit
    // signal emission. No WiFiAccessPoint is included in signal emission for these cases,
    // State.wifi.access_points should be used instead.
    //
    // Access points are stored in an unordered map (State::wifi::access_points) and are guaranteed
    // to have a unique id that can be used to identify them when e.g. mapping to D-Bus objects.
    class Backend
    {
    public:
        enum class ConnectResult
        {
            SUCCESS,
            FAILED
        };

        enum class WiFiStatus
        {
            UNAVAILABLE,
            DISABLED,
            ENABLED
        };

        enum class WiFiHotspotStatus
        {
            DISABLED,
            ENABLED
        };

        struct WiFiAccessPoint
        {
            enum class Event
            {
                ADDED_ALL,
                REMOVED_ALL,
                ADDED_ONE,
                REMOVED_ONE,

                SSID_CHANGED,
                STRENGTH_CHANGED,
                CONNECTED_CHANGED
            };

            using Id = std::uint64_t;
            using Strength = std::uint8_t; // 0-100

            static constexpr Id ID_EMPTY = 0;

            Id id = ID_EMPTY;
            std::string ssid;
            Strength strength = 0;
            bool connected = false;
        };

        struct State
        {
            struct
            {
                WiFiStatus status = WiFiStatus::UNAVAILABLE;

                std::unordered_map<WiFiAccessPoint::Id, WiFiAccessPoint> access_points;

                WiFiHotspotStatus hotspot_status = WiFiHotspotStatus::DISABLED;
                std::string hotspot_ssid;
                Glib::ustring hotspot_passphrase;
            } wifi;
        };

        struct Signals
        {
            sigc::signal<void> critical_error;

            struct
            {
                sigc::signal<void, WiFiStatus> status_changed;

                sigc::signal<void, WiFiAccessPoint::Event, const WiFiAccessPoint *>
                    access_points_changed;

                sigc::signal<void, WiFiHotspotStatus> hotspot_status_changed;
                sigc::signal<void, const std::string &> hotspot_ssid_changed;
                sigc::signal<void, const Glib::ustring &> hotspot_passphrase_changed;
            } wifi;
        };

        using ConnectFinished = std::function<void(ConnectResult result)>;

        using RequestCredentialsFromUserReply =
            std::function<void(const std::optional<Common::Credentials> &result)>;

        using RequestCredentialsFromUser =
            std::function<void(const Common::Credentials::Requested &requested,
                               RequestCredentialsFromUserReply &&callback)>;

        Backend();
        virtual ~Backend();

        Backend(const Backend &other) = delete;
        Backend(Backend &&other) = delete;
        Backend &operator=(const Backend &other) = delete;
        Backend &operator=(Backend &&other) = delete;

        static std::unique_ptr<Backend> create_default();

        virtual void wifi_enable() = 0;
        virtual void wifi_disable() = 0;

        virtual void wifi_connect(const WiFiAccessPoint &access_point,
                                  ConnectFinished &&finished,
                                  RequestCredentialsFromUser &&request_credentials) = 0;
        virtual void wifi_disconnect(const WiFiAccessPoint &access_point) = 0;

        virtual void wifi_hotspot_enable() = 0;
        virtual void wifi_hotspot_disable() = 0;
        virtual void wifi_hotspot_change_ssid(const std::string &ssid) = 0;
        virtual void wifi_hotspot_change_passphrase(const Glib::ustring &passphrase) = 0;

        const State &state() const
        {
            return state_;
        }

        Signals &signals()
        {
            return signals_;
        }

        bool wifi_available() const
        {
            return state_.wifi.status != WiFiStatus::UNAVAILABLE;
        }

        bool wifi_enabled() const
        {
            return state_.wifi.status == Backend::WiFiStatus::ENABLED;
        }

        bool wifi_hotspot_enabled() const
        {
            return state_.wifi.hotspot_status == Backend::WiFiHotspotStatus::ENABLED;
        }

    protected:
        void critical_error();

        void wifi_status_set(WiFiStatus status);

        WiFiAccessPoint::Id wifi_access_point_next_id();
        WiFiAccessPoint *wifi_access_point_find(WiFiAccessPoint::Id id);

        void wifi_access_points_add_all(std::vector<WiFiAccessPoint> &&access_points);
        void wifi_access_points_remove_all();

        void wifi_access_point_add(WiFiAccessPoint &&access_point);
        void wifi_access_point_remove(const WiFiAccessPoint &access_point);

        void wifi_access_point_ssid_set(WiFiAccessPoint &access_point, const std::string &ssid);
        void wifi_access_point_strength_set(WiFiAccessPoint &access_point,
                                            WiFiAccessPoint::Strength strength);
        void wifi_access_point_connected_set(WiFiAccessPoint &access_point, bool connected);

        void wifi_hotspot_status_set(WiFiHotspotStatus status);
        void wifi_hotspot_ssid_set(const std::string &ssid);
        void wifi_hotspot_passphrase_set(const Glib::ustring &passphrase);

    private:
        State state_;
        Signals signals_;

        WiFiAccessPoint::Id wifi_access_point_last_id_ = WiFiAccessPoint::ID_EMPTY;
    };
}

#endif // CONNECTIVITY_MANAGER_DAEMON_BACKEND_H
