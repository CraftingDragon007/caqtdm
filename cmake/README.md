# Qt 6 CMake build

Use CMake 3.24 or newer and build outside the source tree:

```sh
cmake -S . -B ../caqtdm-build/local -G Ninja -DCAQTDM_EPICS_BASE=/path/to/epics-base
cmake --build ../caqtdm-build/local --parallel
ctest --test-dir ../caqtdm-build/local --output-on-failure
```

`CAQTDM_EPICS_BASE` takes precedence over the `EPICS_BASE` environment variable. `CAQTDM_EPICS_HOST_ARCH` takes precedence over `EPICS_HOST_ARCH`; CMake derives an architecture for known platforms when neither is set. OpenBSD requires an explicitly supplied architecture and a compatible EPICS Base build.

The target logic covers Linux, Windows, macOS, FreeBSD, OpenBSD, Android and iOS. Mobile builds require the matching Qt toolchain and EPICS/Qwt libraries. `CAQTDM_ALH2UI=ON` adds a diagnostic ALH converter that is not installed. Unit tests are registered with CTest on desktop platforms.

MinGW builds require zlib for the MinGW target (for example, MSYS2's `mingw-w64-x86_64-zlib`). Other Windows kits use an external zlib when found and otherwise use QtZlib.
