// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#include "cli/command_monitor.h"

#include <giomm.h>
#include <glib-unix.h>
#include <glibmm.h>

#include <csignal>
#include <iostream>
#include <string>

#include "common/dbus.h"

namespace
{
    std::string enabled_str(bool enabled)
    {
        return enabled ? "Yes" : "No";
    }

    gboolean sigint_callback(void *main_loop)
    {
        static_cast<Glib::MainLoop *>(main_loop)->quit();
        return G_SOURCE_CONTINUE;
    }
}

namespace ConnectivityManager::Cli
{
    CommandMonitor::CommandMonitor() : Command("monitor", "Monitor changes")
    {
    }

    bool CommandMonitor::parse_arguments(int argc, char *argv[], std::ostream &output)
    {
        Glib::OptionGroup main_group("monitor", "Monitor Options");
        Glib::OptionContext context;

        {
            Glib::OptionEntry entry;
            entry.set_short_name('i');
            entry.set_long_name("initial-state");
            entry.set_description("Print initial state");
            main_group.add_entry(entry, arguments_.initial_state);
        }

        context.set_main_group(main_group);

        try {
            context.parse(argc, argv);
        } catch (const Glib::Error &error) {
            output << Glib::get_prgname() << ": " << error.what() << '\n';
            return false;
        }

        if (argc > 1) {
            output << Glib::get_prgname() << ": unknown argument " << argv[1] << '\n';
            return false;
        }

        return true;
    }

    bool CommandMonitor::invoke()
    {
        if (arguments_.initial_state)
            print_initial_state();

        connect_signals();

        return monitor_until_ctrl_c();
    }

    bool CommandMonitor::monitor_until_ctrl_c() const
    {
        Glib::RefPtr<Glib::MainLoop> main_loop = Glib::MainLoop::create();

        guint sigint_source_id = g_unix_signal_add(SIGINT, sigint_callback, main_loop.get());
        if (sigint_source_id == 0) {
            std::cout << "Failed to setup Ctrl-C handler.\n";
            return false;
        }

        main_loop->run();

        g_source_remove(sigint_source_id);

        return true;
    }

    void CommandMonitor::print_initial_state() const
    {
        std::cout << "Wi-Fi:\n";
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
    }

    void CommandMonitor::connect_signals()
    {
        manager_proxy()->WiFiAvailable_changed().connect([&] {
            std::cout << "Wi-Fi Available: " << enabled_str(manager_proxy()->WiFiAvailable_get())
                      << '\n';
        });

        manager_proxy()->WiFiEnabled_changed().connect([&] {
            std::cout << "Wi-Fi Enabled: " << enabled_str(manager_proxy()->WiFiEnabled_get())
                      << '\n';
        });

        manager_proxy()->WiFiHotspotEnabled_changed().connect([&] {
            std::cout << "Wi-Fi Hotspot Enabled: "
                      << enabled_str(manager_proxy()->WiFiHotspotEnabled_get()) << '\n';
        });

        manager_proxy()->WiFiHotspotSSID_changed().connect([&] {
            std::cout << "Wi-Fi Hotspot Name/SSID: \"" << manager_proxy()->WiFiHotspotSSID_get()
                      << "\"\n";
        });

        manager_proxy()->WiFiHotspotPassphrase_changed().connect([&] {
            std::cout << "Wi-Fi hotspot passphrase: \""
                      << manager_proxy()->WiFiHotspotPassphrase_get() << "\"\n";
        });
    }
}
