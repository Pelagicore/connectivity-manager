// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#include <giomm.h>
#include <glibmm.h>

#include <clocale>
#include <cstdlib>
#include <iostream>

#include "common/dbus.h"
#include "common/version.h"
#include "generated/dbus/template_dbus_service_proxy.h"

// Just a simple example of how to create a proxy and call a method in the service and shows use of
// libcommon.a. Note that *_sync() versions of the proxy methods are used below. This should only be
// done if it is OK to block for a very long time. Usually that is the case in a cli application.

namespace
{
    using Proxy = com::luxoft::TemplateDBusServiceProxy;
}

int main(int /*argc*/, char * /*argv*/ [])
{
    std::setlocale(LC_ALL, "");

    Glib::init();
    Gio::init();

    // Do some argument parsing here. E.g. nice to have a --version for both service and client.
    // auto arguments = TemplateDBusService::Cli::Arguments::parse(argc, argv);
    // if (!arguments)
    //     return EXIT_FAILURE;
    //
    // if (arguments->print_version_and_exit) {
    //     std::cout << Glib::get_prgname() << " " << TemplateDBusService::Common::VERSION << '\n';
    //     return EXIT_SUCCESS;
    // }

    Glib::RefPtr<Proxy> proxy =
        Proxy::createForBus_sync(Gio::DBus::BUS_TYPE_SYSTEM,
                                 Gio::DBus::PROXY_FLAGS_NONE,
                                 TemplateDBusService::Common::DBus::TEMPLATE_SERVICE_NAME,
                                 TemplateDBusService::Common::DBus::TEMPLATE_OBJECT_PATH);

    if (proxy->dbusProxy()->get_name_owner().empty()) {
        std::cout << "Service not available, quitting.\n";
        return EXIT_FAILURE;
    }

    std::cout << "Service method returned: " << proxy->RemoveMeFoo_sync(123) << '\n';

    return EXIT_SUCCESS;
}
