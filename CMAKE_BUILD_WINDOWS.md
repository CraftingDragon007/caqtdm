# Building caQtDM with CMake and Ninja on Windows

This guide provides step-by-step instructions for building caQtDM using CMake and Ninja on Windows with Visual Studio.

## Prerequisites

Before building, ensure you have the following installed:

1. **Visual Studio 2019 or later** (Professional, Community, or Enterprise)
2. **CMake 3.21 or later** - Download from [cmake.org](https://cmake.org/download/)
3. **Ninja build system** - Download from [ninja-build.org](https://ninja-build.org/) or install via Visual Studio
4. **Qt 5.15+ or Qt 6.x** (Qt 6.10.0 recommended)
5. **Qwt 6.1+** (Qwt 6.3.0 recommended)
6. **EPICS Base 7.x**

## Method 1: Using the Provided Batch Script (Recommended)

This is the easiest method - it automatically uses your environment settings from `caQtDM_Env.bat`.

### Step 1: Configure Your Environment

Edit `caQtDM_Env.bat` to set your paths. For example:

```batch
set QTHOME=C:\Qt\Qt-6.10.0
set QWTHOME=C:\Qwt-6.3.0
set EPICS_BASE=C:\Users\your_username\source\repos\epics-base
set EPICS_HOST_ARCH=windows-x64
set CAQTDM_COLLECT=C:\Users\your_username\source\repos\caqtdm\caQtDM_Binaries

REM Optional: Enable plugins
set CAQTDM_GPS=1
set CAQTDM_MODBUS=1

REM Optional: ZeroMQ for bsread plugin
set ZMQINC=C:\Program Files\ZeroMQ\include
set ZMQLIB=C:\Program Files\ZeroMQ\lib
```

### Step 2: Run the Build Script

Simply run:

```cmd
caQtDM_CMakeBuild.bat
```

Or select a specific configuration (1, 2, or 3 from caQtDM_Env.bat):

```cmd
caQtDM_CMakeBuild.bat 2
```

The script will:
- Load your environment variables
- Configure CMake with Ninja generator
- Build the project
- Place binaries in `%CAQTDM_COLLECT%`

## Method 2: Manual CMake Configuration

If you prefer manual control, follow these steps:

### Step 1: Open Visual Studio Developer Command Prompt

Open "x64 Native Tools Command Prompt for VS 2022" (or your VS version)

### Step 2: Set Environment Variables

```cmd
set QTHOME=C:\Qt\Qt-6.10.0
set QWTHOME=C:\Qwt-6.3.0
set EPICS_BASE=C:\Users\your_username\source\repos\epics-base
set EPICS_HOST_ARCH=windows-x64
set PATH=%QTHOME%\bin;%PATH%
```

### Step 3: Create and Enter Build Directory

```cmd
cd C:\path\to\caqtdm
mkdir build_cmake
cd build_cmake
```

### Step 4: Configure with CMake

Basic configuration:

```cmd
cmake .. ^
  -G "Ninja" ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCAQTDM_QT_PREFIX=%QTHOME% ^
  -DCAQTDM_QWT_PREFIX=%QWTHOME% ^
  -DCAQTDM_EPICS_BASE=%EPICS_BASE% ^
  -DCAQTDM_EPICS_HOST_ARCH=%EPICS_HOST_ARCH%
```

With optional features:

```cmd
cmake .. ^
  -G "Ninja" ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCAQTDM_QT_PREFIX=C:\Qt\Qt-6.10.0 ^
  -DCAQTDM_QWT_PREFIX=C:\Qwt-6.3.0 ^
  -DCAQTDM_EPICS_BASE=C:\epics-base ^
  -DCAQTDM_EPICS_HOST_ARCH=windows-x64 ^
  -DCAQTDM_ENABLE_GPS_PLUGIN=ON ^
  -DCAQTDM_ENABLE_MODBUS_PLUGIN=ON ^
  -DCAQTDM_ENABLE_BSREAD_PLUGIN=ON ^
  -DCAQTDM_ZMQ_INCLUDE_DIR="C:\Program Files\ZeroMQ\include" ^
  -DCAQTDM_ZMQ_LIBRARY_DIR="C:\Program Files\ZeroMQ\lib"
```

### Step 5: Build

```cmd
cmake --build . --config Release
```

Or for parallel builds:

```cmd
ninja
```

### Step 6: Install (Optional)

```cmd
cmake --install . --config Release
```

## Method 3: Using Visual Studio Generator

If you prefer Visual Studio projects instead of Ninja:

```cmd
cmake .. ^
  -G "Visual Studio 17 2022" ^
  -A x64 ^
  -DCAQTDM_QT_PREFIX=%QTHOME% ^
  -DCAQTDM_QWT_PREFIX=%QWTHOME% ^
  -DCAQTDM_EPICS_BASE=%EPICS_BASE% ^
  -DCAQTDM_EPICS_HOST_ARCH=%EPICS_HOST_ARCH%

cmake --build . --config Release
```

This creates a Visual Studio solution that you can open in the IDE.

## Configuration Options

### Required Variables

- `CAQTDM_QT_PREFIX` or `QTHOME` environment variable - Qt installation directory
- `CAQTDM_QWT_PREFIX` or `QWTHOME` environment variable - Qwt installation directory
- `CAQTDM_EPICS_BASE` or `EPICS_BASE` environment variable - EPICS base directory
- `CAQTDM_EPICS_HOST_ARCH` or `EPICS_HOST_ARCH` environment variable - EPICS architecture (e.g., `windows-x64`)

### Optional Variables

**Qwt Configuration:**
- `CAQTDM_QWT_INCLUDE_DIR` - Qwt include directory (if not in standard location)
- `CAQTDM_QWT_LIBRARY_DIR` - Qwt library directory
- `CAQTDM_QWT_LIBRARY_NAME` - Qwt library name (default: qwt)
- `CAQTDM_QWT_VERSION_STRING` - Qwt version string

**EPICS Configuration:**
- `CAQTDM_EPICS_INCLUDE_DIR` - EPICS include directory
- `CAQTDM_EPICS_LIBRARY_DIR` - EPICS library directory

**Plugin Options:**
- `CAQTDM_ENABLE_GPS_PLUGIN` - Enable GPS plugin (ON/OFF)
- `CAQTDM_ENABLE_MODBUS_PLUGIN` - Enable Modbus plugin (ON/OFF)
- `CAQTDM_ENABLE_BSREAD_PLUGIN` - Enable bsread plugin (ON/OFF)
- `CAQTDM_ENABLE_EPICS4_PLUGIN` - Enable EPICS4 plugin (ON/OFF)

**Python Support:**
- `CAQTDM_ENABLE_PYTHONCALC` - Enable Python-assisted calculations (default: OFF on Windows, ON on Linux)
- `CAQTDM_PYTHON_VERSION` - Python version (e.g., "3.11")
- `CAQTDM_PYTHON_INCLUDE_DIR` - Python include directory
- `CAQTDM_PYTHON_LIBRARY_DIR` - Python library directory
- `CAQTDM_PYTHON_LIBRARY` - Full path to Python library file

To enable Python support on Windows:
```cmd
-DCAQTDM_ENABLE_PYTHONCALC=ON ^
-DCAQTDM_PYTHON_VERSION=3.11 ^
-DCAQTDM_PYTHON_INCLUDE_DIR=C:\Python311\include ^
-DCAQTDM_PYTHON_LIBRARY_DIR=C:\Python311\libs ^
-DCAQTDM_PYTHON_LIBRARY=C:\Python311\libs\python311.lib
```

**ZeroMQ (for bsread):**
- `CAQTDM_ZMQ_INCLUDE_DIR` - ZeroMQ include directory
- `CAQTDM_ZMQ_LIBRARY_DIR` - ZeroMQ library directory

**Installation:**
- `CAQTDM_INSTALL_ROOT` - Installation root directory
- `CAQTDM_INSTALL_BINDIR` - Binary installation directory
- `CAQTDM_INSTALL_LIBDIR` - Library installation directory

**Important Note on Paths with Spaces:**

When passing paths that contain spaces (like `C:\Program Files\ZeroMQ`) to CMake, you **must** enclose them in double quotes:

```cmd
-DCAQTDM_ZMQ_INCLUDE_DIR="C:\Program Files\ZeroMQ\include"
```

Without quotes, CMake will interpret the space as a separator and fail with warnings like:
```
CMake Warning: Ignoring extra path from command line: "Files\ZeroMQ\include"
```

## Troubleshooting

### Paths with Spaces Cause Errors

**Problem:** CMake warnings about "Ignoring extra path" or libraries not found even though the path is correct.

**Solution:** Ensure all paths with spaces are properly quoted:
```cmd
REM Wrong - will fail if path has spaces
-DCAQTDM_ZMQ_INCLUDE_DIR=C:\Program Files\ZeroMQ\include

REM Correct - quotes protect spaces
-DCAQTDM_ZMQ_INCLUDE_DIR="C:\Program Files\ZeroMQ\include"
```

The `caQtDM_CMakeBuild.bat` script automatically handles this for you when using environment variables.

### CMake Cannot Find Qt

Make sure Qt's CMake files are accessible:
```cmd
set CMAKE_PREFIX_PATH=%QTHOME%
```

### CMake Cannot Find Qwt

Specify explicit paths:
```cmd
-DCAQTDM_QWT_INCLUDE_DIR=C:\Qwt-6.3.0\include ^
-DCAQTDM_QWT_LIBRARY_DIR=C:\Qwt-6.3.0\lib
```

### EPICS Libraries Not Found

Check that `EPICS_HOST_ARCH` is correct:
```cmd
dir %EPICS_BASE%\lib
```

You should see a directory matching your architecture (e.g., `windows-x64`).

### Ninja Not Found

Install Ninja via:
- Visual Studio Installer (Individual Components → Ninja)
- Download from [ninja-build.org](https://ninja-build.org/)
- Use `scoop install ninja` or `choco install ninja`

### Python Library Not Found

**Error:**
```
CMake Error: Could not find Python library for version 3.11.
Set CAQTDM_PYTHON_LIBRARY or disable CAQTDM_ENABLE_PYTHONCALC.
```

**Solution 1 (Recommended):** Python calculations are disabled by default on Windows. If you don't need Python support, the build will proceed without this feature.

**Solution 2:** If you want to enable Python calculations, install Python and specify its location:

```cmd
cmake .. ^
  -DCAQTDM_ENABLE_PYTHONCALC=ON ^
  -DCAQTDM_PYTHON_VERSION=3.11 ^
  -DCAQTDM_PYTHON_INCLUDE_DIR=C:\Python311\include ^
  -DCAQTDM_PYTHON_LIBRARY=C:\Python311\libs\python311.lib
```

To find your Python installation:
```cmd
where python
python --version
```

## Build Output

After a successful build, you'll find:
- `caQtDM.exe` - Main viewer application
- `caQtDM_Lib.dll` - Main library
- `qtcontrols.dll` - Qt controls library
- `controlsystems/` - Control system plugins
- `designer/` - Qt Designer plugins

## Running caQtDM

After building, you can run caQtDM:

```cmd
cd %CAQTDM_COLLECT%
caQtDM.exe test.ui
```

Make sure Qt and other DLLs are in your PATH or copy them to the binaries directory.

## Comparison: CMake vs QMake

| Feature | QMake (caQtDM_BuildAll.bat) | CMake (caQtDM_CMakeBuild.bat) |
|---------|----------------------------|-------------------------------|
| Build tool | jom/nmake | Ninja/MSBuild |
| Configuration | .pro files | CMakeLists.txt |
| Speed | Moderate | Fast (with Ninja) |
| IDE support | Qt Creator | Visual Studio, CLion, VS Code |
| Out-of-tree builds | Limited | Full support |

Both build systems are maintained and fully functional. Choose based on your preference and toolchain.
