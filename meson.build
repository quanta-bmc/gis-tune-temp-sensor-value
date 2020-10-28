project(
    'tune-temp-sensor-value',
    'cpp',
    version: '1.0',
    default_options: [
        'cpp_std=c++17',
    ],
)

executable(
    'tune-temp-sensor-value',
    [
        'tune-temp-sensor-value.cpp',
    ],
    dependencies: [
        dependency('sdbusplus'),
        dependency('phosphor-dbus-interfaces'),
        dependency('sdeventplus'),
    ],
    install: true,
    install_dir: get_option('bindir')
)

if(get_option('tests').enabled())
    test( 'test_tune-temp-sensor-value', executable(
        'test_tune-temp-sensor-value',
        [
            'test/test_tune-temp-sensor-value.cpp',
        ],
        dependencies: [
            dependency('sdbusplus'),
            dependency('gtest'),
        ],
        install: true,
        install_dir: get_option('bindir')
        )
    )
endif

systemd = dependency('systemd')
conf_data = configuration_data()
conf_data.set('bindir', get_option('prefix') / get_option('bindir'))
configure_file(
  input: 'tune-temp-sensor-value.service.in',
  output: 'tune-temp-sensor-value.service',
  configuration: conf_data,
  install: true,
  install_dir: systemd.get_pkgconfig_variable('systemdsystemunitdir'))
