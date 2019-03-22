version_header = vcs_tag(input : 'version.h.in', output : 'version.h')

common_sources = [
    'dbus.h',
    version_header
]

common_lib = static_library('common',
    include_directories : private_include_dir,
    sources : common_sources)

common_dep = declare_dependency(link_with : common_lib)

subdir('unit_tests')
