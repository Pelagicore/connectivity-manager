// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#include "daemon/backend.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <utility>

#include "config.h"
#include "daemon/backends/connman_backend.h"

namespace ConnectivityManager::Daemon
{
    Backend::Backend() = default;

    Backend::~Backend() = default;

    std::unique_ptr<Backend> Backend::create_default()
    {
#if CONNECTIVITY_MANAGER_BACKEND == CONNECTIVITY_MANAGER_BACKEND_CONNMAN
        return std::make_unique<ConnManBackend>();
#else
#    error "Mising backend in create_backend()."
#endif
    }

    void Backend::critical_error()
    {
        signals_.critical_error.emit();
    }

    void Backend::wifi_status_set(WiFiStatus status)
    {
        if (state_.wifi.status == status)
            return;

        state_.wifi.status = status;
        signals_.wifi.status_changed.emit(status);
    }

    Backend::WiFiAccessPoint::Id Backend::wifi_access_point_next_id()
    {
        while (true) {
            wifi_access_point_last_id_++;

            if (wifi_access_point_last_id_ == WiFiAccessPoint::ID_EMPTY)
                wifi_access_point_last_id_++;

            if (!wifi_access_point_find(wifi_access_point_last_id_))
                break;
        }

        return wifi_access_point_last_id_;
    }

    Backend::WiFiAccessPoint *Backend::wifi_access_point_find(WiFiAccessPoint::Id id)
    {
        auto i = state_.wifi.access_points.find(id);

        return i == state_.wifi.access_points.cend() ? nullptr : &i->second;
    }

    void Backend::wifi_access_points_add_all(std::vector<WiFiAccessPoint> &&access_points)
    {
        wifi_access_points_remove_all();

        for (WiFiAccessPoint &access_point : access_points) {
            WiFiAccessPoint::Id id = access_point.id;
            assert(id != WiFiAccessPoint::ID_EMPTY);

            state_.wifi.access_points.emplace(id, std::move(access_point));
        }

        signals_.wifi.access_points_changed.emit(WiFiAccessPoint::Event::ADDED_ALL, nullptr);
    }

    void Backend::wifi_access_points_remove_all()
    {
        if (state_.wifi.access_points.empty())
            return;

        state_.wifi.access_points.clear();

        signals_.wifi.access_points_changed.emit(WiFiAccessPoint::Event::REMOVED_ALL, nullptr);
    }

    void Backend::wifi_access_point_add(WiFiAccessPoint &&access_point)
    {
        WiFiAccessPoint::Id id = access_point.id;
        assert(id != WiFiAccessPoint::ID_EMPTY);

        auto result = state_.wifi.access_points.emplace(id, std::move(access_point));

        signals_.wifi.access_points_changed.emit(WiFiAccessPoint::Event::ADDED_ONE,
                                                 &result.first->second);
    }

    void Backend::wifi_access_point_remove(const WiFiAccessPoint &access_point)
    {
        auto i = state_.wifi.access_points.find(access_point.id);
        if (i == state_.wifi.access_points.cend())
            return;

        const WiFiAccessPoint copy = std::move(i->second);
        state_.wifi.access_points.erase(i);

        signals_.wifi.access_points_changed.emit(WiFiAccessPoint::Event::REMOVED_ONE, &copy);
    }

    void Backend::wifi_access_point_ssid_set(WiFiAccessPoint &access_point, const std::string &ssid)
    {
        if (access_point.ssid == ssid)
            return;

        access_point.ssid = ssid;
        signals_.wifi.access_points_changed.emit(WiFiAccessPoint::Event::SSID_CHANGED,
                                                 &access_point);
    }

    void Backend::wifi_access_point_strength_set(WiFiAccessPoint &access_point,
                                                 WiFiAccessPoint::Strength strength)
    {
        if (access_point.strength == strength)
            return;

        access_point.strength = strength;
        signals_.wifi.access_points_changed.emit(WiFiAccessPoint::Event::STRENGTH_CHANGED,
                                                 &access_point);
    }

    void Backend::wifi_access_point_connected_set(WiFiAccessPoint &access_point, bool connected)
    {
        if (access_point.connected == connected)
            return;

        access_point.connected = connected;
        signals_.wifi.access_points_changed.emit(WiFiAccessPoint::Event::CONNECTED_CHANGED,
                                                 &access_point);
    }

    void Backend::wifi_hotspot_status_set(WiFiHotspotStatus status)
    {
        if (state_.wifi.hotspot_status == status)
            return;

        state_.wifi.hotspot_status = status;
        signals_.wifi.hotspot_status_changed.emit(status);
    }

    void Backend::wifi_hotspot_ssid_set(const std::string &ssid)
    {
        if (state_.wifi.hotspot_ssid == ssid)
            return;

        state_.wifi.hotspot_ssid = ssid;
        signals_.wifi.hotspot_ssid_changed.emit(ssid);
    }

    void Backend::wifi_hotspot_passphrase_set(const Glib::ustring &passphrase)
    {
        if (state_.wifi.hotspot_passphrase == passphrase)
            return;

        state_.wifi.hotspot_passphrase = passphrase;
        signals_.wifi.hotspot_passphrase_changed.emit(passphrase);
    }
}
