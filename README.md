# Connectivity Manager

D-Bus service for managing connectivity.

Connectivity Manager exposes a [D-Bus interface](data/com.luxoft.ConnectivityManager.xml) for
managing connectivity that is supposed to be easy to use for implementing graphical user interfaces.
A backend abstraction exists in the daemon to allow for changing implementation without modifying
the D-Bus interface.

Connectivity Manager is in early stages of development and currently only supports connecting to
Wi-Fi access points. In the near future support for Bluetooth, VPN, different user profiles etc.
will be added.

# Dependencies

- ConnMan (if built with ConnMan backend)
- gdbus-codegen-glibmm
- glibmm (2.56)
- googletest (1.8.1, for tests, optional)

# Building

[Meson](https://mesonbuild.com/) is used for building. To build simply run:

```shell
meson build
ninja -C build
```

# Running Tests

Tests can be run with:

```shell
meson test -C build
```

"No tests defined." is printed if the required version of googletest could not be found.

# Command Line Interface

`cmcli` is a command line tool that can be used to interact with the Connectivity Manager daemon. It
was mainly developed as a means to test the D-Bus interface without a graphical UI. Help summary:

```
$ cmcli -h
Usage:
  cmcli [OPTION…] [COMMAND]

Commands ('[COMMAND] -h/--help' for details):
  monitor             Monitor changes
  wifi                Wi-Fi operations

Help Options:
  -h, --help       Show help options

Application Options:
  --version        Print version and exit
```

More help for a specific command can be obtained by adding `-h` or `--help` after the command.

```
$ cmcli wifi -h
Usage:
  cmcli wifi [OPTION…] [COMMAND]

Commands:
  enable          Enable Wi-Fi
  disable         Disable Wi-Fi
  status          Show Wi-Fi status and access points
  connect         Connect to Wi-Fi access point
  disconnect      Disconnect from Wi-Fi access point
  enable-hotspot  Enable Wi-Fi hotspot
  disable-hotspot Disable Wi-Fi hotspot

Help Options:
  -h, --help           Show help options

Application Options:
  -s, --ssid           SSID for connect, disconnect or enable-hotspot
  -p, --passphrase     Hotspot passphrase for enable-hotspot
```

For example, to connect to a Wi-Fi Access Point with `cmcli`, first run `cmcli wifi status` to
display current Wi-Fi status.

```
$ cmcli wifi status
Wi-Fi Status:

  Available: Yes
  Enabled  : Yes

  Hotspot Enabled   : No
  Hotspot Name/SSID : ""
  Hotspot Passphrase: ""

  Access Points (* = connected):
    AnAccessPoint (Strength: 89)
    AnotherOne (Strength: 65)
    YetAnotherOne (Strength: 59)
    ...
```

If Wi-Fi is not enabled run `cmcli wifi enable` to enable it. If Wi-Fi was just enabled it might
take a while for access points to show up in the list. Just run `cmcli wifi status` again after a
short while to give the scanning of nearby access points a chance to finish.

Use the `connect` subcommand and `--ssid` option to connect to an access point.

```
$ cmcli wifi connect --ssid AnAccessPoint
```

A password will be prompted for if one is required. `cmcli` will exit when the access point has been
connected to or an error has occurred, in which case an error message should have been printed.

# License and Copyright

Copyright © 2019 Luxoft Sweden AB

The source code in this repository is subject to the terms of the MPL-2.0 license, please see
included "LICENSE" file for details. License information for any other files is either explicitly
stated or defaults to the MPL-2.0 license.
