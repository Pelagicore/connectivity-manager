daemon_unit_tests_deps = [
    daemon_deps,
    gtest_main_dep
]

daemon_unit_tests_sources = [
    'arguments_test.cpp'
]

daemon_unit_tests = executable('daemon-unit_tests',
    dependencies : daemon_unit_tests_deps,
    include_directories : private_include_dir,
    objects : daemon_exe.extract_objects(daemon_sources),
    sources : daemon_unit_tests_sources)

test('daemon unit tests', daemon_unit_tests)
