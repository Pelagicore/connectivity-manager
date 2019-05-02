// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#ifndef CONNECTIVITY_MANAGER_CLI_COMMAND_WIFI_H
#define CONNECTIVITY_MANAGER_CLI_COMMAND_WIFI_H

#include <glibmm.h>

#include <ostream>
#include <vector>

#include "cli/command.h"
#include "generated/dbus/connectivity_manager_proxy.h"

namespace ConnectivityManager::Cli
{
    class CommandWiFi : public Command
    {
    public:
        CommandWiFi();

        bool parse_arguments(int argc, char *argv[], std::ostream &output) override;

    private:
        using AccessPointProxy = com::luxoft::ConnectivityManager::WiFiAccessPointProxy;
        using AccessPointProxies = std::vector<Glib::RefPtr<AccessPointProxy>>;

        enum class Subcommand
        {
            NONE,

            ENABLE,
            DISABLE,
            STATUS,
            CONNECT,
            DISCONNECT,
            ENABLE_HOTSPOT,
            DISABLE_HOTSPOT
        };

        bool invoke() override;

        bool enable() const;
        bool disable() const;
        bool status() const;
        bool connect() const;
        bool disconnect() const;
        bool enable_hotspot() const;
        bool disable_hotspot() const;

        AccessPointProxies access_point_proxies() const;
        Glib::RefPtr<AccessPointProxy> access_point_proxy_with_ssid(const std::string &ssid) const;

        struct
        {
            Subcommand subcommand = Subcommand::NONE;
            Glib::ustring ssid;
            Glib::ustring passphrase;
        } arguments_;
    };
}

#endif // CONNECTIVITY_MANAGER_CLI_COMMAND_WIFI_H
