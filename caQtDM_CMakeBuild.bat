@echo off
REM =============================================================================================
REM Build caQtDM using CMake and Ninja
REM This script uses the environment variables from caQtDM_Env.bat
REM =============================================================================================

echo =============================================================================================
echo caQtDM CMake Build Script
echo =============================================================================================

REM Load environment variables (same as qmake build)
call caQtDM_Env.bat %*
if "%CAQTDM_GENERAL_COMPILATION%"=="1" GOTO :eof

echo.
echo =============================================================================================
echo CMake Configuration
echo =============================================================================================
echo Using environment variables:
echo   QTHOME           = %QTHOME%
echo   QWTHOME          = %QWTHOME%
echo   EPICS_BASE       = %EPICS_BASE%
echo   EPICS_HOST_ARCH  = %EPICS_HOST_ARCH%
echo   CAQTDM_COLLECT   = %CAQTDM_COLLECT%
echo.

REM Create build directory
IF NOT EXIST build_cmake (
    echo Creating build directory: build_cmake
    mkdir build_cmake
)

REM Create binaries directory if it doesn't exist
IF NOT EXIST %CAQTDM_COLLECT% (
    echo Creating binaries directory: %CAQTDM_COLLECT%
    mkdir %CAQTDM_COLLECT%
)

cd build_cmake

echo.
echo =============================================================================================
echo Configuring CMake with Ninja generator...
echo =============================================================================================

REM Configure optional plugins
set CMAKE_OPTIONS=-G "Ninja" ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCAQTDM_QT_PREFIX=%QTHOME% ^
  -DCAQTDM_QWT_PREFIX=%QWTHOME% ^
  -DCAQTDM_EPICS_BASE=%EPICS_BASE% ^
  -DCAQTDM_EPICS_HOST_ARCH=%EPICS_HOST_ARCH% ^
  -DCAQTDM_INSTALL_ROOT=%CAQTDM_COLLECT%

REM Add GPS plugin if enabled
if defined CAQTDM_GPS (
    set CMAKE_OPTIONS=%CMAKE_OPTIONS% -DCAQTDM_ENABLE_GPS_PLUGIN=ON
)

REM Add Modbus plugin if enabled
if defined CAQTDM_MODBUS (
    set CMAKE_OPTIONS=%CMAKE_OPTIONS% -DCAQTDM_ENABLE_MODBUS_PLUGIN=ON
)

REM Add ZeroMQ/bsread plugin if ZMQ is defined
if defined ZMQINC (
    if defined ZMQLIB (
        set CMAKE_OPTIONS=%CMAKE_OPTIONS% ^
          -DCAQTDM_ENABLE_BSREAD_PLUGIN=ON ^
          -DCAQTDM_ZMQ_INCLUDE_DIR=%ZMQINC% ^
          -DCAQTDM_ZMQ_LIBRARY_DIR=%ZMQLIB%
    )
)

REM Add Qwt library name if specified
if defined QWTLIBNAME (
    set CMAKE_OPTIONS=%CMAKE_OPTIONS% -DCAQTDM_QWT_LIBRARY_NAME=%QWTLIBNAME%
)

REM Add Qwt include/lib directories if specified
if defined QWTINCLUDE (
    set CMAKE_OPTIONS=%CMAKE_OPTIONS% -DCAQTDM_QWT_INCLUDE_DIR=%QWTINCLUDE%
)
if defined QWTLIB (
    set CMAKE_OPTIONS=%CMAKE_OPTIONS% -DCAQTDM_QWT_LIBRARY_DIR=%QWTLIB%
)

REM Add Qwt version if specified
if defined QWTVERSION (
    set CMAKE_OPTIONS=%CMAKE_OPTIONS% -DCAQTDM_QWT_VERSION_STRING=%QWTVERSION%
)

REM Add EPICS include/lib directories if specified
if defined EPICSINCLUDE (
    set CMAKE_OPTIONS=%CMAKE_OPTIONS% -DCAQTDM_EPICS_INCLUDE_DIR=%EPICSINCLUDE%
)
if defined EPICSLIB (
    set CMAKE_OPTIONS=%CMAKE_OPTIONS% -DCAQTDM_EPICS_LIBRARY_DIR=%EPICSLIB%
)

echo Running: cmake .. %CMAKE_OPTIONS%
echo.

cmake .. %CMAKE_OPTIONS%

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: CMake configuration failed!
    cd ..
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo =============================================================================================
echo Building with Ninja...
echo =============================================================================================

set /a timerstart=(((1%time:~0,2%-100)*60*60)+((1%time:~3,2%-100)*60)+(1%time:~6,2%-100)^)

cmake --build . --config Release

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Build failed!
    cd ..
    pause
    exit /b %ERRORLEVEL%
)

set /a timerstop=(((1%time:~0,2%-100)*60*60)+((1%time:~3,2%-100)*60)+(1%time:~6,2%-100)^)
set /a timeseks=(%timerstop%-%timerstart%)
set /a timemins=(%timerstop%-%timerstart%)/60

echo.
echo =============================================================================================
echo Build completed successfully!
echo Build time: %timeseks% seconds (%timemins% minutes)
echo Binaries located in: %CAQTDM_COLLECT%
echo =============================================================================================

cd ..

pause
