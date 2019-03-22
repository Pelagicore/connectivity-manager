daemon_deps = [
    common_dep,
    giomm_dep,
    glib_dep,
    glibmm_dep,
    tdbs_dbus_dep
]

daemon_sources = [
    'arguments.cpp',
    'arguments.h',
    'daemon.cpp',
    'daemon.h',
    'dbus_service.cpp',
    'dbus_service.h'
]

daemon_main_sources = [
    'main.cpp',
    daemon_sources
]

daemon_exe = executable('template-dbus-service',
    dependencies : daemon_deps,
    include_directories : private_include_dir,
    sources : daemon_main_sources,
    install : true)

subdir('unit_tests')
