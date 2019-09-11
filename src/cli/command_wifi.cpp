// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#include "cli/command_wifi.h"

#include <glibmm.h>

#include <iostream>

#include "cli/input_handler.h"
#include "common/dbus.h"

// TODO: How to choose hidden AP for "connect"/"disconnect" from list shown by "status"?
//       Could add an option called e.g. -c/--choose for connect/disconnect that prints list
//       of AP:s in order returned by access_point_proxies() with each line in the format:
//       <index + 1>) SSID or <Hidden> (+ details as for "status" now)
//       And then prompt user for AP to connect to and accept both index (+1) and SSID.

namespace ConnectivityManager::Cli
{
    CommandWiFi::CommandWiFi() : Command("wifi", "Wi-Fi operations")
    {
    }

    bool CommandWiFi::parse_arguments(int argc, char *argv[], std::ostream &output)
    {
        static constexpr char SUBCOMMAND_SUMMARY[] =
            "Commands:\n"
            "  enable          Enable Wi-Fi\n"
            "  disable         Disable Wi-Fi\n"
            "  status          Show Wi-Fi status and access points\n"
            "  connect         Connect to Wi-Fi access point\n"
            "  disconnect      Disconnect from Wi-Fi access point\n"
            "  enable-hotspot  Enable Wi-Fi hotspot\n"
            "  disable-hotspot Disable Wi-Fi hotspot";

        auto parse_subcommand = [&](const Glib::ustring &str) {
            if (str == "enable")
                return Subcommand::ENABLE;
            if (str == "disable")
                return Subcommand::DISABLE;
            if (str == "status")
                return Subcommand::STATUS;
            if (str == "connect")
                return Subcommand::CONNECT;
            if (str == "disconnect")
                return Subcommand::DISCONNECT;
            if (str == "enable-hotspot")
                return Subcommand::ENABLE_HOTSPOT;
            if (str == "disable-hotspot")
                return Subcommand::DISABLE_HOTSPOT;

            output << Glib::get_prgname() << ": unknown command: \"" << str << "\"\n";
            return Subcommand::NONE;
        };

        auto parse_remaining = [&](const Glib::OptionGroup::vecustrings &remaining) {
            if (remaining.empty()) {
                output << Glib::get_prgname() << ": missing command\n";
                return false;
            }

            arguments_.subcommand = parse_subcommand(remaining[0]);
            if (arguments_.subcommand == Subcommand::NONE)
                return false;

            if (remaining.size() > 1) {
                output << Glib::get_prgname() << ": unknown argument: \"" << remaining[1] << "\"\n";
                return false;
            }

            return true;
        };

        auto verify_arguments = [&]() {
            bool ssid_required = arguments_.subcommand == Subcommand::CONNECT ||
                                 arguments_.subcommand == Subcommand::DISCONNECT;
            bool ssid_accepted =
                ssid_required || arguments_.subcommand == Subcommand::ENABLE_HOTSPOT;
            bool passphrase_accepted = arguments_.subcommand == Subcommand::ENABLE_HOTSPOT;

            if (ssid_required && arguments_.ssid.empty()) {
                output << Glib::get_prgname() << ": SSID required for connect and disconnect\n";
                return false;
            }

            if (!ssid_accepted && !arguments_.ssid.empty()) {
                output << Glib::get_prgname()
                       << ": SSID only accepted for connect, disconnect and enable-hotspot\n";
                return false;
            }

            if (!passphrase_accepted && !arguments_.passphrase.empty()) {
                output << Glib::get_prgname() << ": Passphrase only valid for enable-hotspot\n";
                return false;
            }

            return true;
        };

        Glib::OptionGroup main_group("wifi", "Wi-Fi Options");
        Glib::OptionContext context("[COMMAND]");
        Glib::OptionGroup::vecustrings remaining;

        {
            Glib::OptionEntry entry;
            entry.set_short_name('s');
            entry.set_long_name("ssid");
            entry.set_description("SSID for connect, disconnect or enable-hotspot");
            main_group.add_entry(entry, arguments_.ssid);
        }

        {
            Glib::OptionEntry entry;
            entry.set_short_name('p');
            entry.set_long_name("passphrase");
            entry.set_description("Hotspot passphrase for enable-hotspot");
            main_group.add_entry(entry, arguments_.passphrase);
        }

        {
            Glib::OptionEntry entry;
            entry.set_long_name(G_OPTION_REMAINING);
            main_group.add_entry(entry, remaining);
        }

        context.set_summary(SUBCOMMAND_SUMMARY);
        context.set_main_group(main_group);

        try {
            context.parse(argc, argv);
        } catch (const Glib::Error &error) {
            output << Glib::get_prgname() << ": " << error.what() << '\n';
            return false;
        }

        if (!parse_remaining(remaining))
            return false;

        return verify_arguments();
    }

    bool CommandWiFi::invoke()
    {
        switch (arguments_.subcommand) {
        case Subcommand::ENABLE:
            return enable();

        case Subcommand::DISABLE:
            return disable();

        case Subcommand::STATUS:
            return status();

        case Subcommand::CONNECT:
            return connect();

        case Subcommand::DISCONNECT:
            return disconnect();

        case Subcommand::ENABLE_HOTSPOT:
            return enable_hotspot();

        case Subcommand::DISABLE_HOTSPOT:
            return disable_hotspot();

        case Subcommand::NONE:
            break;
        }

        return false;
    }

    bool CommandWiFi::enable() const
    {
        try {
            manager_proxy()->WiFiEnabled_set_sync(true);
        } catch (const Glib::Error &e) {
            std::cout << "Failed to enable Wi-Fi: " << e.what() << '\n';
            return false;
        }

        return true;
    }

    bool CommandWiFi::disable() const
    {
        try {
            manager_proxy()->WiFiEnabled_set_sync(false);
        } catch (const Glib::Error &e) {
            std::cout << "Failed to disable Wi-Fi: " << e.what() << '\n';
            return false;
        }

        return true;
    }

    bool CommandWiFi::status() const
    {
        auto enabled_str = [](bool enabled) { return enabled ? "Yes" : "No"; };

        std::cout << "Wi-Fi Status:\n";
        std::cout << '\n';
        std::cout << "  Available: " << enabled_str(manager_proxy()->WiFiAvailable_get()) << '\n';
        std::cout << "  Enabled  : " << enabled_str(manager_proxy()->WiFiEnabled_get()) << '\n';
        std::cout << '\n';
        std::cout << "  Hotspot Enabled   : "
                  << enabled_str(manager_proxy()->WiFiHotspotEnabled_get()) << '\n';
        std::cout << "  Hotspot Name/SSID : \"" << manager_proxy()->WiFiHotspotSSID_get() << "\"\n";
        std::cout << "  Hotspot Passphrase: \"" << manager_proxy()->WiFiHotspotPassphrase_get()
                  << "\"\n";
        std::cout << '\n';
        std::cout << "  Access Points (* = connected):\n";

        for (const auto &proxy : access_point_proxies()) {
            bool connected = proxy->Connected_get();
            std::string connected_marker = connected ? "*" : " ";
            std::string ssid = proxy->SSID_get();
            std::string name = ssid.empty() ? "<Hidden>" : ssid;
            std::string details = "Strength: " + std::to_string(proxy->Strength_get());
            std::string security = proxy->Security_get();

            if (!security.empty()) {
                details += ", Security: " + security;
            }

            std::cout << "  " << connected_marker << "  " << name << " (" << details << ")\n";
        }

        std::cout << '\n';

        return true;
    }

    bool CommandWiFi::connect() const
    {
        auto ap_proxy = access_point_proxy_with_ssid(arguments_.ssid);
        if (!ap_proxy) {
            std::cout << "No access point with name " << arguments_.ssid << '\n';
            return false;
        }

        auto connection = manager_proxy()->dbusProxy()->get_connection();
        if (!InputHandler::instance().register_user_input_agent(connection))
            return false;

        bool result = false;
        auto connect_finish = [&](const Glib::RefPtr<Gio::AsyncResult> &async_result) {
            try {
                manager_proxy()->Connect_finish(async_result);
                result = true;
            } catch (const Glib::Error &e) {
                std::cout << "Failed to connect to " << arguments_.ssid << ": " << e.what() << '\n';
            }

            InputHandler::instance().quit();
        };

        Glib::ustring ap_object_path = ap_proxy->dbusProxy()->get_object_path();
        constexpr int CONNECT_TIMEOUT_MS = 5 * 60 * 1000;

        manager_proxy()->Connect(Glib::DBusObjectPathString(ap_object_path),
                                 InputHandler::user_input_agent_object_path(),
                                 connect_finish,
                                 {},
                                 CONNECT_TIMEOUT_MS);

        InputHandler::instance().run();

        return result;
    }

    bool CommandWiFi::disconnect() const
    {
        auto ap_proxy = access_point_proxy_with_ssid(arguments_.ssid);
        if (!ap_proxy) {
            std::cout << "No access point with name " << arguments_.ssid << '\n';
            return false;
        }

        Glib::ustring ap_object_path = ap_proxy->dbusProxy()->get_object_path();

        try {
            manager_proxy()->Disconnect_sync(Glib::DBusObjectPathString(ap_object_path));
        } catch (const Glib::Error &e) {
            std::cout << "Failed to disconnect " << arguments_.ssid << ": " << e.what() << '\n';
            return false;
        }

        return true;
    }

    bool CommandWiFi::enable_hotspot() const
    {
        try {
            if (!arguments_.ssid.empty())
                manager_proxy()->WiFiHotspotSSID_set_sync(arguments_.ssid);

            if (!arguments_.passphrase.empty())
                manager_proxy()->WiFiHotspotPassphrase_set_sync(arguments_.passphrase);

            manager_proxy()->WiFiHotspotEnabled_set_sync(true);
        } catch (const Glib::Error &e) {
            std::cout << "Failed to enable Wi-Fi hotspot: " << e.what() << '\n';
            return false;
        }

        return true;
    }

    bool CommandWiFi::disable_hotspot() const
    {
        try {
            manager_proxy()->WiFiHotspotEnabled_set_sync(false);
        } catch (const Glib::Error &e) {
            std::cout << "Failed to disable Wi-Fi hotspot: " << e.what() << '\n';
            return false;
        }

        return true;
    }

    CommandWiFi::AccessPointProxies CommandWiFi::access_point_proxies() const
    {
        AccessPointProxies proxies;

        for (const auto &object_path : manager_proxy()->WiFiAccessPoints_get()) {
            proxies.emplace_back(
                AccessPointProxy::createForBus_sync(Gio::DBus::BUS_TYPE_SYSTEM,
                                                    Gio::DBus::PROXY_FLAGS_NONE,
                                                    Common::DBus::MANAGER_SERVICE_NAME,
                                                    object_path));
        }

        return proxies;
    }

    Glib::RefPtr<CommandWiFi::AccessPointProxy> CommandWiFi::access_point_proxy_with_ssid(
        const std::string &ssid) const
    {
        for (const auto &proxy : access_point_proxies())
            if (ssid == proxy->SSID_get())
                return proxy;

        return {};
    }
}
