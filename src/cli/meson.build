cli_deps = [
    cm_dbus_dep,
    common_dep,
    giomm_dep,
    glib_dep,
    glibmm_dep
]

cli_sources = [
    'arguments.cpp',
    'arguments.h',
    'command.cpp',
    'command.h',
    'command_monitor.cpp',
    'command_monitor.h',
    'command_wifi.cpp',
    'command_wifi.h',
    'input_handler.cpp',
    'input_handler.h',
    'main.cpp'
]

executable('cmcli',
    dependencies : cli_deps,
    include_directories : private_include_dir,
    sources : cli_sources,
    install : true)
