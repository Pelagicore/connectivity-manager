# Connectivity Manager

D-Bus service for managing connectivity.

# Dependencies

- gdbus-codgen-glibmm
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

# License and Copyright

Copyright © 2019 Luxoft Sweden AB

The source code in this repository is subject to the terms of the MPL-2.0 license, please see
included "LICENSE" file for details. License information for any other files is either explicitly
stated or defaults to the MPL-2.0 license.
