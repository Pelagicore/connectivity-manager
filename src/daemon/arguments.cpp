// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#include "daemon/arguments.h"

#include <glibmm.h>

namespace ConnectivityManager::Daemon
{
    std::optional<Arguments> Arguments::parse(int argc, char *argv[], std::ostream &output)
    {
        Arguments arguments;
        Glib::OptionGroup main_group("main", "Main Options");
        Glib::OptionContext context;

        {
            Glib::OptionEntry entry;
            entry.set_long_name("version");
            entry.set_description("Print version and exit");
            main_group.add_entry(entry, arguments.print_version_and_exit);
        }

        context.set_main_group(main_group);

        try {
            context.parse(argc, argv);
        } catch (const Glib::Error &error) {
            output << Glib::get_prgname() << ": " << error.what() << '\n';
            return {};
        }

        if (argc > 1) {
            output << Glib::get_prgname() << ": unknown argument \"" << argv[1] << "\"\n";
            return {};
        }

        return arguments;
    }
}
