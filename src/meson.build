private_include_dir = include_directories('.')

config_data = configuration_data()
config_data.set('backend', get_option('connectivity_backend').to_upper())

config_header = configure_file(configuration : config_data,
    input : 'config.h.in',
    output : 'config.h')

subdir('common')
subdir('generated')

subdir('cli')
subdir('daemon')
