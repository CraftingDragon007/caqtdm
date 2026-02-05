# Building caQtDM with CMake on Windows

## Prerequisites

- Visual Studio 2019, 2022, or 2026
- CMake 3.21 or later
- Qt 5.15+ or Qt 6.x
- Qwt 6.1+
- EPICS Base 7.x

## Quick Start

### 1. Configure Environment

Edit `caQtDM_Env.bat` to set your paths:

```batch
set QTHOME=C:\Qt\Qt-6.10.0
set QWTHOME=C:\Qwt-6.3.0
set EPICS_BASE=C:\epics-base
set EPICS_HOST_ARCH=windows-x64
set CAQTDM_COLLECT=C:\caqtdm\caQtDM_Binaries
```

### 2. Run Build Script

```cmd
caQtDM_CMakeBuild.bat
```

##  Manual Build

Open "x64 Native Tools Command Prompt" and run:

```cmd
set QTHOME=C:\Qt\Qt-6.10.0
set QWTHOME=C:\Qwt-6.3.0
set EPICS_BASE=C:\epics-base
set EPICS_HOST_ARCH=windows-x64

mkdir build
cd build

cmake .. ^
  -G "Visual Studio 17 2022" ^
  -A x64 ^
  -DCAQTDM_QT_PREFIX=%QTHOME% ^
  -DCAQTDM_QWT_PREFIX=%QWTHOME% ^
  -DCAQTDM_EPICS_BASE=%EPICS_BASE% ^
  -DCAQTDM_EPICS_HOST_ARCH=%EPICS_HOST_ARCH%

REM Or use Visual Studio 18 2026:
REM cmake .. -G "Visual Studio 18 2026" -A x64 ...

cmake --build . --config Release
cmake --install . --config Release
```

## Optional Plugins

Enable plugins by setting environment variables before building:

```batch
set CAQTDM_GPS=1
set CAQTDM_MODBUS=1
set ZMQINC=C:\ZeroMQ\include
set ZMQLIB=C:\ZeroMQ\lib
```

Or use CMake options:

```cmd
-DCAQTDM_ENABLE_GPS_PLUGIN=ON ^
-DCAQTDM_ENABLE_MODBUS_PLUGIN=ON ^
-DCAQTDM_ENABLE_BSREAD_PLUGIN=ON ^
-DCAQTDM_ZMQ_INCLUDE_DIR=C:\ZeroMQ\include ^
-DCAQTDM_ZMQ_LIBRARY_DIR=C:\ZeroMQ\lib
```

## Important Notes

- **Use Visual Studio generator** (Visual Studio 17 2022 or Visual Studio 18 2026)
  - Do NOT use Ninja generator (has EPICS4 plugin compilation issues)
- Paths with spaces must be quoted: `"C:\Program Files\ZeroMQ\include"`
- Python calculations are disabled by default on Windows

## Troubleshooting

### Qt Not Found
```cmd
set CMAKE_PREFIX_PATH=%QTHOME%
```

### EPICS Libraries Not Found
Check that `%EPICS_BASE%\lib\%EPICS_HOST_ARCH%` exists and contains `.lib` files.

### ZeroMQ Not Found
CMake automatically detects ZeroMQ library names based on Visual Studio version.
Check that library files exist: `dir %ZMQLIB%\*.lib`
