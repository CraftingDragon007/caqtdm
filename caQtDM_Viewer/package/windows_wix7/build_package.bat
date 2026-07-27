@echo off
setlocal enabledelayedexpansion
rem ============================================================================
rem  caQtDM Windows installer (MSI) build with WiX v7
rem
rem  Usage:   build_package.bat [x64|arm64]        (default: x64)
rem
rem  Requirements:
rem    - WiX v7 CLI installed:   dotnet tool install --global wix
rem    - The build environment is loaded automatically from caQtDM_Env.bat in
rem      the project root, matching the architecture (x64 = profile 2,
rem      arm64 = profile 4, to be added there). It must provide:
rem        QTHOME, EPICS_BASE, EPICS_HOST_ARCH, CAQTDM_COLLECT,
rem        ZMQBIN, ZMQLIBRARY, QWTHOME, SSLBIN, LIBSSL, LIBCRYPTO
rem    - Optional:
rem        CAQTDM_SIGNER       certificate subject for signtool (else unsigned)
rem        TIMESTAPER          timestamp server for signtool
rem
rem  The MSI version is read from the FILEVERSION of caQtDM.exe (which gets it
rem  from qtdefs.pri via caQtDM.rc), so it can never differ from the binary.
rem
rem  Build artifacts are created in the subfolder project_<arch> (cleaned on
rem  every run); the finished MSI is moved to %CAQTDM_COLLECT% like all the
rem  other binaries. Name: caQtDM_<maj>_<min>_<patch>_<arch>.msi
rem ============================================================================

rem --- architecture -----------------------------------------------------------
set ARCH=%1
if "%ARCH%"=="" set ARCH=x64
set ENVPROFILE=
if /i "%ARCH%"=="x64" set ENVPROFILE=2
if /i "%ARCH%"=="arm64" set ENVPROFILE=4
if not defined ENVPROFILE (
    echo ERROR: unknown architecture "%ARCH%" - use x64 or arm64
    exit /b 1
)

rem --- load the matching build environment (non-interactive) -------------------
rem     profile 2 = Qt6 / x64
rem     profile 4 = Qt6 / arm64  (has to be added to caQtDM_Env.bat before the
rem                               first arm64 package can be built - it must set
rem                               its own QTHOME, EPICS_*, CAQTDM_COLLECT etc.)
call "%~dp0..\..\..\caQtDM_Env.bat" %ENVPROFILE%

cd /d %~dp0

rem --- WiX CLI available? -----------------------------------------------------
where wix >nul 2>&1
if errorlevel 1 (
    echo ERROR: wix.exe not found in PATH.
    echo Install WiX v7 with:  dotnet tool install --global wix
    exit /b 1
)

rem --- required environment variables ----------------------------------------
set MISSING=
for %%V in (QTHOME EPICS_BASE EPICS_HOST_ARCH CAQTDM_COLLECT ZMQBIN ZMQLIBRARY QWTHOME SSLBIN LIBSSL LIBCRYPTO) do (
    if not defined %%V set MISSING=!MISSING! %%V
)
if not "!MISSING!"=="" (
    echo ERROR: missing environment variables:!MISSING!
    echo Set them as for the classic build, e.g. via caQtDM_Env.bat
    exit /b 1
)

if not exist "%CAQTDM_COLLECT%\caQtDM.exe" (
    echo ERROR: "%CAQTDM_COLLECT%\caQtDM.exe" not found - build caQtDM first.
    exit /b 1
)

rem --- make sure the WiX extensions are installed (idempotent) ----------------
wix extension add -g WixToolset.UI.wixext/7.0.0 >nul 2>&1
wix extension add -g WixToolset.Util.wixext/7.0.0 >nul 2>&1

rem --- read the version from the caQtDM binary --------------------------------
rem Use the numeric FILEVERSION parts (4.6.1.0), NOT the FileVersion string -
rem the string contains the git suffix (V4.6.1_Development_xxxxxxxx) which is
rem not a valid MSI ProductVersion.
set FILEVERSION=
for /f %%v in ('powershell -NoProfile -Command "$vi=(Get-Item '%CAQTDM_COLLECT%\caQtDM.exe').VersionInfo; '{0}.{1}.{2}.{3}' -f $vi.FileMajorPart,$vi.FileMinorPart,$vi.FileBuildPart,$vi.FilePrivatePart"') do set FILEVERSION=%%v
if "%FILEVERSION%"=="" (
    echo ERROR: could not read FILEVERSION from "%CAQTDM_COLLECT%\caQtDM.exe"
    exit /b 1
)

rem file name part: 4.6.1.0 -> 4_6_1 (drop the 4th digit, dots to underscores)
for /f "tokens=1-3 delims=." %%a in ("%FILEVERSION%") do set MSIVERSION=%%a_%%b_%%c
set MSINAME=caQtDM_%MSIVERSION%_%ARCH%.msi

rem --- clean build subfolder (like project_x64 in the old WiX3 build) ----------
set BUILDDIR=project_%ARCH%
if exist %BUILDDIR% del /q /f /s %BUILDDIR%\*.* >nul
if not exist %BUILDDIR% mkdir %BUILDDIR%

rem --- build ------------------------------------------------------------------
echo Building %MSINAME%  (ProductVersion %FILEVERSION%, arch %ARCH%) ...
wix build caQtDM.wxs -arch %ARCH% ^
    -d ProductVersion=%FILEVERSION% ^
    -d CAQTDM_TARGETDIRNAME=windows-%ARCH% ^
    -ext WixToolset.UI.wixext ^
    -ext WixToolset.Util.wixext ^
    -out %BUILDDIR%\%MSINAME%
if errorlevel 1 (
    echo ERROR: wix build failed.
    exit /b 1
)

rem --- optional signing --------------------------------------------------------
if defined CAQTDM_SIGNER (
    signtool sign /fd SHA256 /n %CAQTDM_SIGNER% /t %TIMESTAPER% %BUILDDIR%\%MSINAME%
    if errorlevel 1 (
        echo ERROR: signing failed.
        exit /b 1
    )
) else (
    echo NOTE: CAQTDM_SIGNER not set - %MSINAME% stays unsigned.
)

rem --- move the MSI to the collect directory like all other binaries -----------
move /y %BUILDDIR%\%MSINAME% "%CAQTDM_COLLECT%\" >nul
if errorlevel 1 (
    echo ERROR: could not move %MSINAME% to "%CAQTDM_COLLECT%"
    exit /b 1
)

echo.
echo Done: %CAQTDM_COLLECT%\%MSINAME%
endlocal
