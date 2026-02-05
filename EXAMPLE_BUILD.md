# Example Build

## Using the Build Script

Edit `caQtDM_Env.bat` with your paths:

```batch
set QTHOME=C:\Qt\Qt-6.10.0
set QWTHOME=C:\Qwt-6.3.0
set EPICS_BASE=C:\epics-base
set EPICS_HOST_ARCH=windows-x64
set CAQTDM_COLLECT=C:\caqtdm\caQtDM_Binaries

REM Optional plugins
set CAQTDM_GPS=1
set CAQTDM_MODBUS=1
set ZMQINC=C:\ZeroMQ\include
set ZMQLIB=C:\ZeroMQ\lib
```

Then run:
```cmd
caQtDM_CMakeBuild.bat
```

## Manual Build

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
  -DCAQTDM_EPICS_HOST_ARCH=%EPICS_HOST_ARCH% ^
  -DCAQTDM_ENABLE_GPS_PLUGIN=ON ^
  -DCAQTDM_ENABLE_MODBUS_PLUGIN=ON

cmake --build . --config Release
```

## Testing

```cmd
cd %CAQTDM_COLLECT%
set PATH=%QTHOME%\bin;%PATH%
caQtDM.exe test.ui
```
