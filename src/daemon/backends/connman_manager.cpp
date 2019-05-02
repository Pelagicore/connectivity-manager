// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#include "daemon/backends/connman_manager.h"

#include <glibmm.h>

#include "daemon/backends/connman_dbus.h"

namespace ConnectivityManager::Daemon
{
    ConnManManager::ConnManManager(Listener &listener) : listener_(listener)
    {
        Proxy::createForBus(Gio::DBus::BUS_TYPE_SYSTEM,
                            Gio::DBus::PROXY_FLAGS_NONE,
                            ConnManDBus::SERVICE_NAME,
                            ConnManDBus::MANAGER_OBJECT_PATH,
                            sigc::mem_fun(*this, &ConnManManager::proxy_create_finish));
    }

    void ConnManManager::proxy_create_finish(const Glib::RefPtr<Gio::AsyncResult> &result)
    {
        try {
            proxy_ = Proxy::createForBusFinish(result);
        } catch (const Glib::Error &e) {
            g_warning("Failed to create D-Bus proxy for ConnMan manager: %s", e.what().c_str());
            listener_.manager_proxy_creation_failed();
            return;
        }

        proxy_->TechnologyAdded_signal.connect(
            sigc::mem_fun(*this, &ConnManManager::technology_added));

        proxy_->TechnologyRemoved_signal.connect(
            sigc::mem_fun(*this, &ConnManManager::technology_removed));

        proxy_->ServicesChanged_signal.connect(
            sigc::mem_fun(*this, &ConnManManager::services_changed));

        proxy_->dbusProxy()->property_g_name_owner().signal_changed().connect(
            sigc::mem_fun(*this, &ConnManManager::name_owner_changed));

        name_owner_changed();
    }

    void ConnManManager::name_owner_changed() const
    {
        bool available = !proxy_->dbusProxy()->get_name_owner().empty();

        if (available) {
            proxy_->GetTechnologies(sigc::mem_fun(*this, &ConnManManager::get_technologies_finish));
            proxy_->GetServices(sigc::mem_fun(*this, &ConnManManager::get_services_finish));
        }

        listener_.manager_availability_changed(available);
    }

    void ConnManManager::get_technologies_finish(const Glib::RefPtr<Gio::AsyncResult> &result) const
    {
        TechnologyPropertiesArray array;

        try {
            proxy_->GetTechnologies_finish(array, result);
        } catch (const Glib::Error &e) {
            g_warning("Failed to get ConnMan technologies: %s", e.what().c_str());
            return;
        }

        for (const auto &[path, properties] : array)
            listener_.manager_technology_add(path, properties);
    }

    void ConnManManager::technology_added(const Glib::DBusObjectPathString &path,
                                          const ConnManTechnology::PropertyMap &properties) const
    {
        listener_.manager_technology_add(path, properties);
    }

    void ConnManManager::technology_removed(const Glib::DBusObjectPathString &path) const
    {
        listener_.manager_technology_remove(path);
    }

    void ConnManManager::get_services_finish(const Glib::RefPtr<Gio::AsyncResult> &result) const
    {
        ServicePropertiesArray array;

        try {
            proxy_->GetServices_finish(array, result);
        } catch (const Glib::Error &e) {
            g_warning("Failed to get ConnMan services: %s", e.what().c_str());
            return;
        }

        for (const auto &[path, properties] : array)
            listener_.manager_service_add_or_change(path, properties);
    }

    void ConnManManager::services_changed(
        const ServicePropertiesArray &changed,
        const std::vector<Glib::DBusObjectPathString> &removed) const
    {
        for (const auto &[path, properties] : changed)
            listener_.manager_service_add_or_change(path, properties);

        for (const auto &path : removed)
            listener_.manager_service_remove(path);
    }

    void ConnManManager::register_agent(const ConnManAgent &agent)
    {
        Glib::DBusObjectPathString agent_path(agent.object_path());

        proxy_->RegisterAgent(agent_path,
                              sigc::mem_fun(*this, &ConnManManager::register_agent_finish));
    }

    void ConnManManager::register_agent_finish(const Glib::RefPtr<Gio::AsyncResult> &result)
    {
        bool success = false;

        try {
            proxy_->RegisterAgent_finish(result);
            success = true;
        } catch (const Glib::Error &error) {
            g_warning("Failed to register agent with ConnMan Manager: %s", error.what().c_str());
        }

        listener_.manager_register_agent_result(success);
    }
}
