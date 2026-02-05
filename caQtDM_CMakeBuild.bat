@echo off
REM Build caQtDM using CMake and Visual Studio
REM Uses environment variables from caQtDM_Env.bat

echo =============================================================================================
echo caQtDM CMake Build Script
echo =============================================================================================

REM Load environment variables
call caQtDM_Env.bat %*
if "%CAQTDM_GENERAL_COMPILATION%"=="1" GOTO :eof

echo.
echo Using environment:
echo   QTHOME           = %QTHOME%
echo   QWTHOME          = %QWTHOME%
echo   EPICS_BASE       = %EPICS_BASE%
echo   EPICS_HOST_ARCH  = %EPICS_HOST_ARCH%
echo   CAQTDM_COLLECT   = %CAQTDM_COLLECT%
echo.

REM Create build directory
IF NOT EXIST build_cmake (
    mkdir build_cmake
)

IF NOT EXIST %CAQTDM_COLLECT% (
    mkdir %CAQTDM_COLLECT%
)

cd build_cmake

echo =============================================================================================
echo Configuring CMake with Visual Studio generator...
echo =============================================================================================

set CMAKE_OPTIONS=-G "Visual Studio 17 2022" ^
  -A x64 ^
  -DCAQTDM_QT_PREFIX="%QTHOME%" ^
  -DCAQTDM_QWT_PREFIX="%QWTHOME%" ^
  -DCAQTDM_EPICS_BASE="%EPICS_BASE%" ^
  -DCAQTDM_EPICS_HOST_ARCH=%EPICS_HOST_ARCH% ^
  -DCAQTDM_INSTALL_ROOT="%CAQTDM_COLLECT%"

if defined CAQTDM_GPS set CMAKE_OPTIONS=%CMAKE_OPTIONS% -DCAQTDM_ENABLE_GPS_PLUGIN=ON
if defined CAQTDM_MODBUS set CMAKE_OPTIONS=%CMAKE_OPTIONS% -DCAQTDM_ENABLE_MODBUS_PLUGIN=ON

if defined ZMQINC (
    if defined ZMQLIB (
        set CMAKE_OPTIONS=%CMAKE_OPTIONS% ^
          -DCAQTDM_ENABLE_BSREAD_PLUGIN=ON ^
          -DCAQTDM_ZMQ_INCLUDE_DIR="%ZMQINC%" ^
          -DCAQTDM_ZMQ_LIBRARY_DIR="%ZMQLIB%"
    )
)

if defined QWTLIBNAME set CMAKE_OPTIONS=%CMAKE_OPTIONS% -DCAQTDM_QWT_LIBRARY_NAME=%QWTLIBNAME%
if defined QWTINCLUDE set CMAKE_OPTIONS=%CMAKE_OPTIONS% -DCAQTDM_QWT_INCLUDE_DIR="%QWTINCLUDE%"
if defined QWTLIB set CMAKE_OPTIONS=%CMAKE_OPTIONS% -DCAQTDM_QWT_LIBRARY_DIR="%QWTLIB%"
if defined QWTVERSION set CMAKE_OPTIONS=%CMAKE_OPTIONS% -DCAQTDM_QWT_VERSION_STRING=%QWTVERSION%
if defined EPICSINCLUDE set CMAKE_OPTIONS=%CMAKE_OPTIONS% -DCAQTDM_EPICS_INCLUDE_DIR="%EPICSINCLUDE%"
if defined EPICSLIB set CMAKE_OPTIONS=%CMAKE_OPTIONS% -DCAQTDM_EPICS_LIBRARY_DIR="%EPICSLIB%"

cmake .. %CMAKE_OPTIONS%

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake configuration failed!
    cd ..
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo =============================================================================================
echo Building...
echo =============================================================================================

set /a timerstart=(((1%time:~0,2%-100)*60*60)+((1%time:~3,2%-100)*60)+(1%time:~6,2%-100)^)

cmake --build . --config Release

if %ERRORLEVEL% NEQ 0 (
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
echo Build completed! (%timeseks%s / %timemins%m)
echo Binaries: %CAQTDM_COLLECT%
echo =============================================================================================

cd ..
pause
