// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#include "daemon/dbus_service.h"

#include <giomm.h>
#include <glibmm.h>

#include <cassert>
#include <memory>

#include "common/dbus.h"

namespace ConnectivityManager::Daemon
{
    DBusService::DBusService(const Glib::RefPtr<Glib::MainLoop> &main_loop, Backend &backend) :
        main_loop_(main_loop),
        backend_(backend),
        manager_(backend)
    {
    }

    DBusService::~DBusService()
    {
        unown_name();
    }

    void DBusService::own_name()
    {
        if (connection_id_ != 0)
            return;

        connection_id_ = Gio::DBus::own_name(Gio::DBus::BUS_TYPE_SYSTEM,
                                             Common::DBus::MANAGER_SERVICE_NAME,
                                             sigc::mem_fun(*this, &DBusService::bus_acquired),
                                             sigc::mem_fun(*this, &DBusService::name_acquired),
                                             sigc::mem_fun(*this, &DBusService::name_lost));
    }

    void DBusService::unown_name()
    {
        if (connection_id_ == 0)
            return;

        backend_signal_handler_.reset();

        Gio::DBus::unown_name(connection_id_);
        connection_id_ = 0;
    }

    void DBusService::bus_acquired(const Glib::RefPtr<Gio::DBus::Connection> &connection,
                                   const Glib::ustring & /*name*/)
    {
        connection_ = connection;

        backend_signal_handler_.emplace(*this);

        if (!wifi_access_points_create_all_and_register_on_bus()) {
            main_loop_->quit();
            return;
        }

        manager_.sync_with_backend(wifi_access_point_paths_sorted());

        if (manager_.register_object(connection_, Common::DBus::MANAGER_OBJECT_PATH) == 0) {
            main_loop_->quit();
            return;
        }
    }

    void DBusService::name_acquired(const Glib::RefPtr<Gio::DBus::Connection> & /*connection*/,
                                    const Glib::ustring &name)
    {
        g_info("Acquired D-Bus name %s", name.c_str());
    }

    void DBusService::name_lost(const Glib::RefPtr<Gio::DBus::Connection> & /*connection*/,
                                const Glib::ustring &name)
    {
        g_warning("Lost D-Bus name %s, quitting", name.c_str());
        main_loop_->quit();
    }

    bool DBusService::wifi_access_points_create_all_and_register_on_bus()
    {
        assert(connection_);

        wifi_access_points_.clear();

        for (const auto &[id, backend_ap] : backend_.state().wifi.access_points)
            wifi_access_points_.emplace(id, std::make_unique<WiFiAccessPoint>(backend_ap));

        bool all_registered = true;

        for (const auto &key_value : wifi_access_points_)
            if (!key_value.second->register_object(connection_))
                all_registered = false;

        return all_registered;
    }

    std::vector<Glib::DBusObjectPathString> DBusService::wifi_access_point_paths_sorted() const
    {
        std::vector<Glib::DBusObjectPathString> paths;

        for (const auto &key_value : wifi_access_points_)
            paths.emplace_back(key_value.second->object_path());

        // TODO: Sorted in order suitable to present to user, by strength etc.
        //
        // Stored in std::map with id as key so paths are sorted by id now.
        //
        // Have "std::vector<WiFiAccessPoint::Id> wifi.access_points_sorted" in Backend::State (or
        // pointers to elements in wifi.access_points, elements will not move). If in Backend it
        // will be easier to test. Could also add e.g. a "SORT_ORDER_CHANGED" entry in
        // Backend::WiFiAccessPoint::Event and only update paths property when that event is
        // received instead of listening to all events that may change order (prevents abstraction
        // leak... if logic changes, e.g. another field affects order, one would have to listen to
        // extra events outside of Backend, better with explicit "sort order changed" event).
        //
        // If sorting should be handled in DBusService, property must be set on more
        // Backend::WiFiAccessPoint::Event:s than now. But, leaning towards handling sorting in
        // Backend as mentioned above so unit tests can be added for it more easily.

        return paths;
    }

    DBusService::BackendSignalHandler::BackendSignalHandler(DBusService &service) :
        service_(service)
    {
        Backend::Signals &signals = service_.backend_.signals();

        signals.wifi.status_changed.connect(
            sigc::mem_fun(*this, &BackendSignalHandler::wifi_status_changed));

        signals.wifi.access_points_changed.connect(
            sigc::mem_fun(*this, &BackendSignalHandler::wifi_access_points_changed));

        signals.wifi.hotspot_status_changed.connect(
            sigc::mem_fun(*this, &BackendSignalHandler::wifi_hotspot_status_changed));

        signals.wifi.hotspot_ssid_changed.connect(
            sigc::mem_fun(*this, &BackendSignalHandler::wifi_hotspot_ssid_changed));

        signals.wifi.hotspot_passphrase_changed.connect(
            sigc::mem_fun(*this, &BackendSignalHandler::wifi_hotspot_passphrase_changed));
    }

    void DBusService::BackendSignalHandler::wifi_status_changed(Backend::WiFiStatus status) const
    {
        service_.manager_.WiFiAvailable_set(status != Backend::WiFiStatus::UNAVAILABLE);
        service_.manager_.WiFiEnabled_set(status == Backend::WiFiStatus::ENABLED);
    }

    void DBusService::BackendSignalHandler::wifi_access_points_changed(
        Backend::WiFiAccessPoint::Event event,
        const Backend::WiFiAccessPoint *access_point) const
    {
        bool update_aps_property = false;

        switch (event) {
        case Backend::WiFiAccessPoint::Event::ADDED_ALL:
            service_.wifi_access_points_create_all_and_register_on_bus();
            update_aps_property = true;
            break;

        case Backend::WiFiAccessPoint::Event::REMOVED_ALL:
            service_.wifi_access_points_.clear();
            update_aps_property = true;
            break;

        case Backend::WiFiAccessPoint::Event::ADDED_ONE:
            service_.wifi_access_points_.emplace(access_point->id,
                                                 std::make_unique<WiFiAccessPoint>(*access_point));
            service_.wifi_access_points_[access_point->id]->register_object(service_.connection_);
            update_aps_property = true;
            break;

        case Backend::WiFiAccessPoint::Event::REMOVED_ONE:
            service_.wifi_access_points_.erase(access_point->id);
            update_aps_property = true;
            break;

        case Backend::WiFiAccessPoint::Event::SSID_CHANGED:
            service_.wifi_access_points_[access_point->id]->SSID_set(access_point->ssid);
            break;

        case Backend::WiFiAccessPoint::Event::STRENGTH_CHANGED:
            service_.wifi_access_points_[access_point->id]->Strength_set(access_point->strength);
            break;

        case Backend::WiFiAccessPoint::Event::CONNECTED_CHANGED:
            service_.wifi_access_points_[access_point->id]->Connected_set(access_point->connected);
            break;

        case Backend::WiFiAccessPoint::Event::SECURITY_CHANGED:
            service_.wifi_access_points_[access_point->id]->security_set(access_point->security);
            break;
        }

        if (update_aps_property) {
            service_.manager_.WiFiAccessPoints_set(service_.wifi_access_point_paths_sorted());
        }
    }

    void DBusService::BackendSignalHandler::wifi_hotspot_status_changed(
        Backend::WiFiHotspotStatus status) const
    {
        service_.manager_.WiFiHotspotEnabled_set(status == Backend::WiFiHotspotStatus::ENABLED);
    }

    void DBusService::BackendSignalHandler::wifi_hotspot_ssid_changed(const std::string &ssid) const
    {
        service_.manager_.WiFiHotspotSSID_set(ssid);
    }

    void DBusService::BackendSignalHandler::wifi_hotspot_passphrase_changed(
        const Glib::ustring &passphrase) const
    {
        service_.manager_.WiFiHotspotPassphrase_set(passphrase);
    }
}
