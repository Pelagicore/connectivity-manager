dbus_data_dir = join_paths(get_option('prefix'), get_option('datadir'), 'dbus-1')
dbus_interfaces_dir = join_paths(dbus_data_dir, 'interfaces')
dbus_services_dir = join_paths(dbus_data_dir, 'system-services')
dbus_sysconf_dir = join_paths(dbus_data_dir, 'system.d')

install_data('com.luxoft.Template.conf', install_dir : dbus_sysconf_dir)
install_data('com.luxoft.Template.service', install_dir : dbus_services_dir)
install_data('com.luxoft.Template.xml', install_dir : dbus_interfaces_dir)

systemd_unit_dir = systemd_dep.get_pkgconfig_variable('systemdsystemunitdir')
systemd_unit_config = configuration_data()
systemd_unit_config.set('bindir', join_paths(get_option('prefix'), get_option('bindir')))

configure_file(configuration : systemd_unit_config,
    input : 'template-dbus-service.service.in',
    output : 'template-dbus-service.service',
    install : true,
    install_dir : systemd_unit_dir)
