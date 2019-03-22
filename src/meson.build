private_include_dir = include_directories('.')

config_data = configuration_data()
config_data.set('TEMPLATE_DBUS_SERVICE_FOO', get_option('foo'))

config_header = configure_file(configuration : config_data,
    input : 'config.h.in',
    output : 'config.h')

subdir('common')
subdir('generated')

subdir('cli')
subdir('daemon')
