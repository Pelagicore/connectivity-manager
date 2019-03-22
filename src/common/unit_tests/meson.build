# This is just a placeholder.
# The purpose of this file is to show the directory structure for unit tests.
# The unit_tests directory should be included per component
# and one test file should test one unit (e.g. class).

common_unit_tests_deps = [
    common_dep,
    glibmm_dep,
    gtest_main_dep
]

common_unit_tests_sources = [
]

common_unit_tests = executable('common-unit_tests',
    dependencies : common_unit_tests_deps,
    include_directories : private_include_dir,
    sources : common_unit_tests_sources)

test('libcommon unit tests', common_unit_tests)
