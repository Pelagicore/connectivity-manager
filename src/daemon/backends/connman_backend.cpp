// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#include "daemon/backends/connman_backend.h"

#include <giomm.h>
#include <glibmm.h>

#include <utility>
#include <vector>

#include "common/string_to_valid_utf8.h"

namespace ConnectivityManager::Daemon
{
    ConnManBackend::ConnManBackend() = default;

    ConnManBackend::~ConnManBackend() = default;

    void ConnManBackend::wifi_technology_ready(ConnManTechnology &technology)
    {
        auto aps_from_services = [&]() {
            std::vector<WiFiAccessPoint> aps;

            for (auto &i : services_) {
                ConnManService &service = i.second;

                if (service.type() == ConnManService::Type::WIFI && service.proxy_created()) {
                    WiFiAccessPoint &ap = aps.emplace_back();

                    ap.id = wifi_access_point_next_id();
                    ap.ssid = service.name();
                    ap.strength = service.strength();
                    ap.connected = service.state_to_connected();
                    ap.security = service.security_to_wifi_security();

                    wifi_service_to_ap_id_.emplace(&service, ap.id);
                }
            }

            return aps;
        };

        if (wifi_technology_) {
            g_warning("Received multiple WiFi technologies from ConnMan, using latest");
            wifi_technology_removed();
        }

        wifi_technology_ = &technology;

        wifi_status_set(technology.powered() ? WiFiStatus::ENABLED : WiFiStatus::DISABLED);
        wifi_access_points_add_all(aps_from_services());

        wifi_hotspot_status_set(technology.tethering() ? WiFiHotspotStatus::ENABLED :
                                                         WiFiHotspotStatus::DISABLED);
        wifi_hotspot_ssid_set(technology.tethering_identifier());
        wifi_hotspot_passphrase_set(technology.tethering_passphrase());
    }

    void ConnManBackend::wifi_technology_removed()
    {
        if (!wifi_technology_)
            return;

        wifi_technology_ = nullptr;
        wifi_service_to_ap_id_.clear();

        wifi_access_points_remove_all();
        wifi_hotspot_status_set(WiFiHotspotStatus::DISABLED);
        wifi_status_set(WiFiStatus::UNAVAILABLE);
    }

    void ConnManBackend::wifi_technology_property_changed(ConnManTechnology::PropertyId id)
    {
        switch (id) {
        case ConnManTechnology::PropertyId::POWERED:
            wifi_status_set(wifi_technology_->powered() ? WiFiStatus::ENABLED :
                                                          WiFiStatus::DISABLED);
            if (wifi_technology_->powered())
                wifi_technology_->scan();
            break;
        case ConnManTechnology::PropertyId::TETHERING:
            wifi_hotspot_status_set(wifi_technology_->tethering() ? WiFiHotspotStatus::ENABLED :
                                                                    WiFiHotspotStatus::DISABLED);
            break;
        case ConnManTechnology::PropertyId::TETHERING_IDENTIFIER:
            wifi_hotspot_ssid_set(wifi_technology_->tethering_identifier());
            break;
        case ConnManTechnology::PropertyId::TETHERING_PASSPHRASE:
            wifi_hotspot_passphrase_set(wifi_technology_->tethering_passphrase());
            break;
        default:
            break;
        }
    }

    void ConnManBackend::wifi_enable()
    {
        if (!wifi_technology_)
            return;

        wifi_technology_->set_powered(true);
    }

    void ConnManBackend::wifi_disable()
    {
        if (!wifi_technology_)
            return;

        wifi_technology_->set_powered(false);
    }

    void ConnManBackend::wifi_connect(const WiFiAccessPoint &access_point,
                                      ConnectFinished &&finished,
                                      RequestCredentialsFromUser &&request_credentials)
    {
        if (!wifi_technology_) {
            finished(ConnectResult::FAILED);
            return;
        }

        ConnManService *service = service_from_wifi_ap(access_point);
        if (!service) {
            finished(ConnectResult::FAILED);
            return;
        }

        service_connect(*service, std::move(finished), std::move(request_credentials));
    }

    void ConnManBackend::wifi_disconnect(const WiFiAccessPoint &access_point)
    {
        if (!wifi_technology_)
            return;

        ConnManService *service = service_from_wifi_ap(access_point);
        if (!service)
            return;

        service->disconnect();
    }

    void ConnManBackend::wifi_hotspot_enable()
    {
        if (!wifi_technology_)
            return;

        wifi_technology_->set_tethering(true);
    }

    void ConnManBackend::wifi_hotspot_disable()
    {
        if (!wifi_technology_)
            return;

        wifi_technology_->set_tethering(false);
    }

    void ConnManBackend::wifi_hotspot_change_ssid(const std::string &ssid)
    {
        if (!wifi_technology_)
            return;

        wifi_technology_->set_tethering_identifier(Common::string_to_valid_utf8(ssid));
    }

    void ConnManBackend::wifi_hotspot_change_passphrase(const Glib::ustring &passphrase)
    {
        if (!wifi_technology_)
            return;

        wifi_technology_->set_tethering_passphrase(passphrase);
    }

    void ConnManBackend::manager_proxy_creation_failed()
    {
        critical_error();
    }

    void ConnManBackend::manager_availability_changed(bool available)
    {
        if (available) {
            agent_register();
        } else {
            wifi_technology_removed();

            connect_queue_.fail_all_and_clear();

            services_.clear();
            technologies_.clear();

            agent_.set_state(ConnManAgent::State::NOT_REGISTERED_WITH_MANAGER);
        }
    }

    void ConnManBackend::manager_technology_add(const Glib::DBusObjectPathString &path,
                                                const ConnManTechnology::PropertyMap &properties)
    {
        manager_technology_remove(path);

        technologies_.try_emplace(path.raw(), *this, path, properties);
    }

    void ConnManBackend::manager_technology_remove(const Glib::DBusObjectPathString &path)
    {
        auto i = technologies_.find(path);
        if (i == technologies_.cend())
            return;

        const ConnManTechnology &technology = i->second;

        if (&technology == wifi_technology_)
            wifi_technology_removed();

        technologies_.erase(i);
    }

    void ConnManBackend::manager_service_add_or_change(
        const Glib::DBusObjectPathString &path,
        const ConnManService::PropertyMap &properties)
    {
        auto i = services_.find(path);

        if (i == services_.cend()) {
            services_.try_emplace(path.raw(), *this, path, properties);
        } else {
            ConnManService &service = i->second;
            service.properties_changed(properties);
        }
    }

    void ConnManBackend::manager_service_remove(const Glib::DBusObjectPathString &path)
    {
        auto i = services_.find(path);
        if (i == services_.cend())
            return;

        ConnManService &service = i->second;

        connect_queue_.remove_service(service);

        if (WiFiAccessPoint *ap = service_to_wifi_ap(service); ap) {
            wifi_service_to_ap_id_.erase(&service);
            wifi_access_point_remove(*ap);
        }

        services_.erase(i);
    }

    void ConnManBackend::manager_register_agent_result(bool success)
    {
        if (success) {
            agent_.set_state(ConnManAgent::State::REGISTERED_WITH_MANAGER);
            connect_queue_.connect_if_not_empty();
        } else {
            agent_.set_state(ConnManAgent::State::NOT_REGISTERED_WITH_MANAGER);
            connect_queue_.fail_all_and_clear();
        }
    }

    void ConnManBackend::agent_released()
    {
        connect_queue_.fail_all_and_clear();
    }

    void ConnManBackend::agent_request_input(const Glib::DBusObjectPathString &service_path,
                                             Common::Credentials &&credentials,
                                             RequestCredentialsFromUserReply &&reply)
    {
        auto i = services_.find(service_path);
        if (i == services_.cend()) {
            g_warning("Received ConnMan agent credentials request for non-existing service");
            reply(Common::Credentials::NONE);
            return;
        }

        ConnManService &service = i->second;

        using Requested = Common::Credentials::Requested;
        Requested requested;

        if (service.type() == ConnManService::Type::WIFI) {
            // TODO: Is this an OK way to detect a hidden network? Think it is (does not seem to be
            //       a better way in the service D-Bus API) but should test. Connecting to hidden
            //       WiFi network has not been tested at all.
            if (!service.name().empty())
                requested.description_type = Requested::TYPE_WIRELESS_NETWORK;
            else
                requested.description_type = Requested::TYPE_HIDDEN_WIRELESS_NETWORK;
        } else {
            requested.description_type = Requested::TYPE_NETWORK;
        }

        requested.description_id = service.name();
        requested.credentials = std::move(credentials);

        connect_queue_.request_credentials(service, requested, std::move(reply));
    }

    void ConnManBackend::agent_register()
    {
        if (!agent_.registered_object()) {
            if (!agent_.register_object(manager_.dbus_connection())) {
                connect_queue_.fail_all_and_clear();
                return;
            }
        }

        if (agent_.state() == ConnManAgent::State::NOT_REGISTERED_WITH_MANAGER) {
            manager_.register_agent(agent_);
            agent_.set_state(ConnManAgent::State::REGISTERING_WITH_MANAGER);
        }
    }

    void ConnManBackend::technology_proxy_created(ConnManTechnology &technology)
    {
        if (technology.type() == ConnManTechnology::Type::WIFI)
            wifi_technology_ready(technology);
    }

    void ConnManBackend::technology_property_changed(ConnManTechnology &technology,
                                                     ConnManTechnology::PropertyId id)
    {
        if (&technology == wifi_technology_)
            wifi_technology_property_changed(id);
    }

    void ConnManBackend::service_proxy_created(ConnManService &service)
    {
        if (service.type() == ConnManService::Type::WIFI) {
            WiFiAccessPoint ap;

            ap.id = wifi_access_point_next_id();
            ap.ssid = service.name();
            ap.strength = service.strength();
            ap.connected = service.state_to_connected();
            ap.security = service.security_to_wifi_security();

            wifi_service_to_ap_id_.emplace(&service, ap.id);

            wifi_access_point_add(std::move(ap));
        }
    }

    void ConnManBackend::service_property_changed(ConnManService &service,
                                                  ConnManService::PropertyId id)
    {
        if (WiFiAccessPoint *ap = service_to_wifi_ap(service); ap) {
            switch (id) {
            case ConnManService::PropertyId::NAME:
                wifi_access_point_ssid_set(*ap, service.name());
                break;
            case ConnManService::PropertyId::SECURITY:
                wifi_access_point_security_set(*ap, service.security_to_wifi_security());
                break;
            case ConnManService::PropertyId::STATE:
                wifi_access_point_connected_set(*ap, service.state_to_connected());
                break;
            case ConnManService::PropertyId::STRENGTH:
                wifi_access_point_strength_set(*ap, service.strength());
                break;
            default:
                break;
            }
        }
    }

    void ConnManBackend::service_connect_finished(ConnManService &service, bool success)
    {
        connect_queue_.connect_finished(service, success);
    }

    void ConnManBackend::service_connect(ConnManService &service,
                                         ConnectFinished &&finished,
                                         RequestCredentialsFromUser &&request_credentials)
    {
        bool agent_registered = agent_.state() == ConnManAgent::State::REGISTERED_WITH_MANAGER;

        connect_queue_.enqueue(
            service, std::move(finished), std::move(request_credentials), agent_registered);

        if (!agent_registered)
            agent_register();
    }

    ConnManBackend::WiFiAccessPoint *ConnManBackend::service_to_wifi_ap(ConnManService &service)
    {
        if (service.type() != ConnManService::Type::WIFI)
            return nullptr;

        auto i = wifi_service_to_ap_id_.find(&service);

        return i == wifi_service_to_ap_id_.cend() ? nullptr : wifi_access_point_find(i->second);
    }

    ConnManService *ConnManBackend::service_from_wifi_ap(const WiFiAccessPoint &ap)
    {
        for (auto &[service, id] : wifi_service_to_ap_id_)
            if (ap.id == id)
                return service;

        return nullptr;
    }
}
