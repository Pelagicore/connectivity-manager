common_deps = [
    glibmm_dep
]

version_header = vcs_tag(input : 'version.h.in', output : 'version.h')

common_sources = [
    'credentials.cpp',
    'credentials.h',
    'dbus.h',
    'scoped_silent_log_handler.h',
    'string_to_uint64.cpp',
    'string_to_uint64.h',
    'string_to_valid_utf8.cpp',
    'string_to_valid_utf8.h',
    version_header
]

common_lib = static_library('common',
    dependencies : common_deps,
    include_directories : private_include_dir,
    sources : common_sources)

common_dep = declare_dependency(link_with : common_lib)

subdir('unit_tests')
