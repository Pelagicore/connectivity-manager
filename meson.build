project('connectivity-manager', 'cpp',
    version : '0.1.0',
    meson_version : '>=0.45',
    default_options : [
        'cpp_std=c++17',
        'warning_level=3'
    ])

giomm_dep = dependency('giomm-2.4', version : '>=2.56')
glib_dep = dependency('glib-2.0', version : '>=2.56')
glibmm_dep = dependency('glibmm-2.4', version : '>=2.56')
gtest_main_dep = dependency('gtest', main : true, required : false, version : '>=1.8.1')
systemd_dep = dependency('systemd')

if not gtest_main_dep.found()
    gtest_main_dep = disabler()
endif

cpp = meson.get_compiler('cpp')

extra_warnings = cpp.get_supported_arguments([
    '-Wformat=2',
    '-Woverloaded-virtual',
    '-Wshadow',
    '-Wsuggest-override'
])

add_project_arguments(extra_warnings, language : 'cpp')

subdir('data')
subdir('src')
