// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#include "cli/arguments.h"

#include <glibmm.h>

#include <array>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>

#include "cli/command.h"
#include "cli/command_monitor.h"
#include "cli/command_wifi.h"

namespace ConnectivityManager::Cli
{
    namespace
    {
        using Commands = std::array<std::unique_ptr<Command>, 2>;

        const Commands &commands()
        {
            static const Commands commands = {std::make_unique<CommandMonitor>(),
                                              std::make_unique<CommandWiFi>()};
            return commands;
        }

        Command *get_command(const std::string &name)
        {
            for (auto &command : commands())
                if (command->name() == name)
                    return command.get();

            return nullptr;
        }

        std::string commands_summary()
        {
            constexpr std::size_t DESCRIPTION_ALIGNMENT = 20;
            std::string summary = "Commands ('[COMMAND] -h/--help' for details):\n";

            for (auto &command : commands()) {
                summary += "  " + command->name();
                summary += std::string(DESCRIPTION_ALIGNMENT - command->name().length(), ' ');
                summary += command->description();

                if (command != commands().back())
                    summary += "\n";
            }

            return summary;
        }

        void include_command_name_in_program_name(const Command *command)
        {
            std::string old_name = Glib::get_prgname();
            std::string new_name = old_name + " " + command->name();

            Glib::set_prgname(new_name);
        }
    }

    std::optional<Arguments> Arguments::parse(int argc, char *argv[], std::ostream &output)
    {
        Arguments arguments;
        Glib::OptionGroup main_group("main", "Main Options");
        Glib::OptionContext context("[COMMAND]");

        {
            Glib::OptionEntry entry;
            entry.set_long_name("version");
            entry.set_description("Print version and exit");
            main_group.add_entry(entry, arguments.print_version_and_exit);
        }

        context.set_strict_posix(true);
        context.set_summary(commands_summary());
        context.set_main_group(main_group);

        try {
            context.parse(argc, argv);
        } catch (const Glib::Error &error) {
            output << Glib::get_prgname() << ": " << error.what() << '\n';
            return {};
        }

        if (arguments.print_version_and_exit)
            return arguments;

        if (argc < 2) {
            output << Glib::get_prgname() << ": missing command\n";
            return {};
        }

        arguments.command = get_command(argv[1]);

        if (!arguments.command) {
            output << Glib::get_prgname() << ": unknown command \"" << argv[1] << "\"\n";
            return {};
        }

        include_command_name_in_program_name(arguments.command);

        if (!arguments.command->parse_arguments(argc - 1, argv + 1, output))
            return {};

        return arguments;
    }
}
