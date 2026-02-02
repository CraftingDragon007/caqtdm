# Example: Building with Your Configuration

This example shows how to build caQtDM using the exact configuration you provided.

## Your Environment Settings

Based on your configuration:
- Qt: `C:\Qt\Qt-6.10.0`
- Qwt: `C:\Qwt-6.3.0`
- EPICS: `C:\Users\houba_j\source\repos\epics-base`
- EPICS Host Arch: `windows-x64`
- Output: `C:\Users\houba_j\source\repos\caqtdm\caQtDM_Binaries`
- Plugins: GPS and Modbus enabled
- ZeroMQ: `C:\Program Files\ZeroMQ`

## Quick Start (Using the Build Script)

### Option 1: Edit caQtDM_Env.bat

Update the SELECT2 section in `caQtDM_Env.bat` with your paths:

```batch
:SELECT2 
  call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64
  
  set QTHOME=C:\Qt\Qt-6.10.0
  set QWTHOME=C:\Qwt-6.3.0
  set QWTINCLUDE=%QWTHOME%/include
  set QWTLIB=%QWTHOME%/lib
  set QWTLIBNAME=qwt
  set QWTVERSION=6.3
  set CAQTDMQTVER=QT6
  
  set EPICS_BASE=C:\Users\houba_j\source\repos\epics-base
  set EPICS_HOST_ARCH=windows-x64
  set EPICSINCLUDE=%EPICS_BASE%/include
  set EPICSLIB=%EPICS_BASE%/lib/%EPICS_HOST_ARCH%
  
  set QTCONTROLS_LIBS=C:\Users\houba_j\source\repos\caqtdm\caQtDM_Binaries
  set CAQTDM_COLLECT=C:\Users\houba_j\source\repos\caqtdm\caQtDM_Binaries
  
  set ZMQ=C:\Program Files\ZeroMQ
  set ZMQINC=%ZMQ%/include
  set ZMQLIB=%ZMQ%/lib
  
  set CAQTDM_GPS=1
  set CAQTDM_MODBUS=1
  
GOTO PRINTOUT
```

Then run:
```cmd
caQtDM_CMakeBuild.bat 2
```

### Option 2: Direct Command Line

Open "x64 Native Tools Command Prompt for VS 2022" and run:

```cmd
cd C:\Users\houba_j\source\repos\caqtdm

REM Set up Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64

REM Set environment variables
set QTHOME=C:\Qt\Qt-6.10.0
set QWTHOME=C:\Qwt-6.3.0
set EPICS_BASE=C:\Users\houba_j\source\repos\epics-base
set EPICS_HOST_ARCH=windows-x64
set PATH=%QTHOME%\bin;%PATH%

REM Create and enter build directory
mkdir build_cmake
cd build_cmake

REM Configure with CMake
cmake .. ^
  -G "Ninja" ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCAQTDM_QT_PREFIX=C:\Qt\Qt-6.10.0 ^
  -DCAQTDM_QWT_PREFIX=C:\Qwt-6.3.0 ^
  -DCAQTDM_QWT_INCLUDE_DIR=C:\Qwt-6.3.0\include ^
  -DCAQTDM_QWT_LIBRARY_DIR=C:\Qwt-6.3.0\lib ^
  -DCAQTDM_QWT_LIBRARY_NAME=qwt ^
  -DCAQTDM_QWT_VERSION_STRING=6.3 ^
  -DCAQTDM_EPICS_BASE=C:\Users\houba_j\source\repos\epics-base ^
  -DCAQTDM_EPICS_HOST_ARCH=windows-x64 ^
  -DCAQTDM_EPICS_INCLUDE_DIR=C:\Users\houba_j\source\repos\epics-base\include ^
  -DCAQTDM_EPICS_LIBRARY_DIR=C:\Users\houba_j\source\repos\epics-base\lib\windows-x64 ^
  -DCAQTDM_INSTALL_ROOT=C:\Users\houba_j\source\repos\caqtdm\caQtDM_Binaries ^
  -DCAQTDM_ENABLE_GPS_PLUGIN=ON ^
  -DCAQTDM_ENABLE_MODBUS_PLUGIN=ON ^
  -DCAQTDM_ENABLE_BSREAD_PLUGIN=ON ^
  -DCAQTDM_ZMQ_INCLUDE_DIR="C:\Program Files\ZeroMQ\include" ^
  -DCAQTDM_ZMQ_LIBRARY_DIR="C:\Program Files\ZeroMQ\lib"

REM Build
cmake --build . --config Release

REM Optional: Install to caQtDM_Binaries
cmake --install . --config Release
```

## Expected Output

After successful build, check `C:\Users\houba_j\source\repos\caqtdm\caQtDM_Binaries`:

```
caQtDM_Binaries/
├── caQtDM.exe                    (Main application)
├── caQtDM_Lib.dll                (Core library)
├── qtcontrols.dll                (Controls library)
├── adlParser.dll                 (ADL parser)
├── edlParser.dll                 (EDL parser, if on Unix)
├── controlsystems/               (Control system plugins)
│   ├── demo_plugin.dll
│   ├── epics3_plugin.dll
│   ├── environment_plugin.dll
│   ├── modbus_plugin.dll        (if enabled)
│   ├── gps_plugin.dll           (if enabled)
│   └── bsread_Plugin.dll        (if enabled)
└── designer/                     (Qt Designer plugins)
    ├── qtcontrols_controllers_plugin.dll
    ├── qtcontrols_graphics_plugin.dll
    ├── qtcontrols_monitors_plugin.dll
    └── qtcontrols_utilities_plugin.dll
```

## Testing the Build

After building, test caQtDM:

```cmd
cd C:\Users\houba_j\source\repos\caqtdm\caQtDM_Binaries

REM Make sure Qt is in PATH
set PATH=C:\Qt\Qt-6.10.0\bin;%PATH%

REM Run caQtDM with a test file
caQtDM.exe ..\caQtDM_Tests\test.ui
```

## Troubleshooting for Your Setup

### If Qt is not found:
```cmd
set CMAKE_PREFIX_PATH=C:\Qt\Qt-6.10.0
```

### If Qwt is not found:
```cmd
REM Check that these files exist:
dir C:\Qwt-6.3.0\include\qwt_plot.h
dir C:\Qwt-6.3.0\lib\qwt.lib
```

### If EPICS libraries are not found:
```cmd
REM Check that the directory exists:
dir C:\Users\houba_j\source\repos\epics-base\lib\windows-x64

REM Check for required libraries:
dir C:\Users\houba_j\source\repos\epics-base\lib\windows-x64\ca.lib
dir C:\Users\houba_j\source\repos\epics-base\lib\windows-x64\Com.lib
```

### If ZeroMQ is not found:
```cmd
REM Check that these exist:
dir "C:\Program Files\ZeroMQ\include\zmq.h"
dir "C:\Program Files\ZeroMQ\lib\*.lib"
```

## Build Time

Expected build time with Ninja on a modern system:
- Clean build: 5-10 minutes
- Incremental build: 30 seconds - 2 minutes

## Next Steps

After successfully building:

1. **Test the application**: Run caQtDM with test files from `caQtDM_Tests`
2. **Configure Qt Designer**: Add the designer plugin path to Qt Designer
3. **Set up environment**: Create a startup script with proper PATH settings
4. **Deploy**: Copy Qt/Qwt DLLs if needed for distribution

## Comparison with QMake Build

The CMake build should produce identical binaries to the qmake build. Both:
- Support the same Qt/Qwt versions
- Include the same plugins
- Use the same EPICS libraries
- Produce the same executables and DLLs

Choose CMake if you prefer:
- Modern build system
- Better IDE integration
- Faster builds with Ninja
- Easier cross-platform development
