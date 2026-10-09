# CMake build

Use CMake 3.24 or newer and build outside the source tree. Qt 6 is selected
automatically when available; pass `CAQTDM_QT_MAJOR_VERSION=5` to build with
Qt 5.15 instead:

```sh
cmake -S . -B ../caqtdm-build/local -G Ninja -DCAQTDM_EPICS_BASE=/path/to/epics-base
cmake --build ../caqtdm-build/local --parallel
ctest --test-dir ../caqtdm-build/local --output-on-failure
```

```sh
cmake -S . -B ../caqtdm-build/qt5 -G Ninja \
  -DCAQTDM_QT_MAJOR_VERSION=5 -DCAQTDM_EPICS_BASE=/path/to/epics-base
cmake --build ../caqtdm-build/qt5 --parallel
ctest --test-dir ../caqtdm-build/qt5 --output-on-failure
```

Qt 5 CMake builds currently support desktop platforms; mobile builds use Qt 6.

`CAQTDM_EPICS_BASE` takes precedence over the `EPICS_BASE` environment variable. `CAQTDM_EPICS_HOST_ARCH` takes precedence over `EPICS_HOST_ARCH`; CMake derives an architecture for known platforms when neither is set. OpenBSD requires an explicitly supplied architecture and a compatible EPICS Base build.

The target logic covers Linux, Windows, macOS, FreeBSD, OpenBSD, Android and iOS. Mobile builds require the matching Qt toolchain and EPICS/Qwt libraries. `CAQTDM_ALH2UI=ON` adds a diagnostic ALH converter that is not installed. Unit tests are registered with CTest on desktop platforms.

MinGW builds require zlib for the MinGW target (for example, MSYS2's `mingw-w64-x86_64-zlib`). Other Windows kits use an external zlib when found and otherwise use QtZlib.

Install
-------

Desktop builds install the viewer, converter tools, shared libraries, and
control-system and Designer plugins into the selected CMake prefix. Qt, EPICS,
and Qwt remain host dependencies. For example:

```sh
cmake --install ../caqtdm-build/local --prefix /opt/caqtdm
```

The executable and converter tools go under `bin`; shared libraries go under
the configured CMake library directory (usually `lib`); control-system and
Designer plugins go under its `controlsystems` and `designer` subdirectories.
The viewer searches the configured install library directory for control-system
plugins. Mobile applications continue to use their Qt deployment workflows and
do not install intermediate static libraries or plugins.
