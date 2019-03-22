gdbus_codegen = find_program('gdbus-codegen-glibmm3')

gdbus_codegen_deps = [
    giomm_dep,
    glibmm_dep
]

interfaces_dir = join_paths(meson.source_root(), 'data')
dbus_build_dir = meson.current_build_dir()

# com.luxoft.TemplateDBusService
tdbs_dbus_sources = custom_target('TemplateDBusService D-Bus source',
    command : [ gdbus_codegen, '--generate-cpp-code=' + join_paths(dbus_build_dir, 'template_dbus_service'), '@INPUT@' ],
    input : join_paths(interfaces_dir, 'com.luxoft.TemplateDBusService.xml'),
    output : [
        'template_dbus_service_common.cpp',
        'template_dbus_service_common.h',
        'template_dbus_service_proxy.cpp',
        'template_dbus_service_proxy.h',
        'template_dbus_service_stub.cpp',
        'template_dbus_service_stub.h'
    ])

tdbs_dbus_headers = [ tdbs_dbus_sources[1], tdbs_dbus_sources[3], tdbs_dbus_sources[5] ]

tdbs_dbus_lib = static_library('template_dbus_service_dbus',
    sources : tdbs_dbus_sources,
    dependencies : gdbus_codegen_deps)

tdbs_dbus_dep = declare_dependency(link_with : tdbs_dbus_lib, sources : tdbs_dbus_headers)
