daemon_deps = [
    cm_dbus_dep,
    common_dep,
    connman_dbus_dep,
    giomm_dep,
    glib_dep,
    glibmm_dep
]

daemon_sources = [
    'arguments.cpp',
    'arguments.h',
    'backend.cpp',
    'backend.h',
    'backends/connman_agent.cpp',
    'backends/connman_agent.h',
    'backends/connman_backend.cpp',
    'backends/connman_backend.h',
    'backends/connman_connect_queue.cpp',
    'backends/connman_connect_queue.h',
    'backends/connman_dbus.h',
    'backends/connman_manager.cpp',
    'backends/connman_manager.h',
    'backends/connman_service.cpp',
    'backends/connman_service.h',
    'backends/connman_technology.cpp',
    'backends/connman_technology.h',
    'daemon.cpp',
    'daemon.h',
    'dbus_objects/manager.cpp',
    'dbus_objects/manager.h',
    'dbus_objects/wifi_access_point.cpp',
    'dbus_objects/wifi_access_point.h',
    'dbus_service.cpp',
    'dbus_service.h'
]

daemon_main_sources = [
    'main.cpp',
    daemon_sources
]

daemon_exe = executable('connectivity-manager',
    dependencies : daemon_deps,
    include_directories : private_include_dir,
    sources : daemon_main_sources,
    install : true)

subdir('unit_tests')
