gdbus_codegen = find_program('gdbus-codegen-glibmm3')

gdbus_codegen_deps = [
    giomm_dep,
    glibmm_dep
]

interfaces_dir = join_paths(meson.source_root(), 'data')
dbus_build_dir = meson.current_build_dir()

# com.luxoft.ConnectivityManager
cm_dbus_sources = custom_target('ConnectivityManager D-Bus source',
    command : [ gdbus_codegen, '--generate-cpp-code=' + join_paths(dbus_build_dir, 'connectivity_manager'), '@INPUT@' ],
    input : join_paths(interfaces_dir, 'com.luxoft.ConnectivityManager.xml'),
    output : [
        'connectivity_manager_common.cpp',
        'connectivity_manager_common.h',
        'connectivity_manager_proxy.cpp',
        'connectivity_manager_proxy.h',
        'connectivity_manager_stub.cpp',
        'connectivity_manager_stub.h'
    ])

cm_dbus_headers = [ cm_dbus_sources[1], cm_dbus_sources[3], cm_dbus_sources[5] ]

cm_dbus_lib = static_library('connectivity_manager_dbus',
    sources : cm_dbus_sources,
    dependencies : gdbus_codegen_deps)

cm_dbus_dep = declare_dependency(link_with : cm_dbus_lib, sources : cm_dbus_headers)

# net.connman
connman_dbus_sources = custom_target('ConnMan D-Bus source',
    command : [ gdbus_codegen, '--generate-cpp-code=' + join_paths(dbus_build_dir, 'connman'), '@INPUT@' ],
    input : join_paths(interfaces_dir, 'net.connman.xml'),
    output : [
        'connman_common.cpp',
        'connman_common.h',
        'connman_proxy.cpp',
        'connman_proxy.h',
        'connman_stub.cpp',
        'connman_stub.h'
    ])

connman_dbus_headers = [ connman_dbus_sources[1], connman_dbus_sources[3], connman_dbus_sources[5] ]

connman_dbus_lib = static_library('connman_dbus',
    sources : connman_dbus_sources,
    dependencies : gdbus_codegen_deps)

connman_dbus_dep = declare_dependency(link_with : connman_dbus_lib, sources : connman_dbus_headers)
