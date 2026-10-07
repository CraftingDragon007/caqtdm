# caQtDM for ReactOS (32-bit MinGW)

This directory preserves the dependency settings and deployment procedure used
for the caQtDM build that was reported to launch successfully on ReactOS.
It contains source/configuration files, not prebuilt DLLs or a Qt installer.
Use the caQtDM source containing the Qt 5.7/MinGW compatibility changes from
this branch; these dependency files alone do not adapt an older caQtDM checkout.

## Requirements

Build on Windows, using an MSYS2 Bash shell and **Qt's bundled 32-bit MinGW**.
The working setup used:

| Component | Version / revision |
| --- | --- |
| Qt | 5.7.1, `mingw53_32` distribution |
| Compiler | Qt's MinGW 5.3.0, `mingw530_32` |
| Qwt | 6.3.0, commit `4ddf2f3878d0835f678da0f2aba1180f2ba208f6` |
| zlib | 1.2.8, headers and static `libz.a` in the bundled MinGW installation |
| EPICS Base | commit `04442786b6bdf0d01f4f973b97cb3c24bfb5a615` (`R7.0.10-4-g04442786b`) with its submodules |
| EPICS architecture | `win32-x86-mingw` |

Install MSYS2's Bash, Git, Perl, patch, and standard Unix tools. The compiler,
`mingw32-make`, `dlltool`, `objdump`, `qmake`, and `windeployqt` must come from
the Qt/MinGW installation above, not MSYS2's current 64-bit compiler.
An MSYS2 MinGW64 shell was used as the launcher; it does **not** determine the
target architecture when the 32-bit compiler is first in `PATH`.

Qt's archived installer is available from the
[Qt 5.7.1 archive](https://download.qt.io/archive/qt/5.7/5.7.1/).
Qwt sources are available from the [Qwt project](https://qwt.sourceforge.io/).
EPICS sources are available from
[epics-base/epics-base](https://github.com/epics-base/epics-base).
Use the pinned EPICS revision, not just the `R7.0.10` tag: its Windows
`osdTime.h` already contains the MinGW `timespec` guard needed here.

The EPICS submodule revisions in the working checkout were:

| Module | Revision |
| --- | --- |
| pvData | `e069c699e9658f0764b3bbdd404ba9ae00aa56c5` (8.0.7) |
| normativeTypes | `e67486c3195af95ea62ff2b9a149f9874e051b77` (6.0.2) |
| pvAccess | `b14e327ac8e06c3560975a4c3a3ca9571143f89d` (7.1.8) |
| pvaClient | `65d2cce75303ffeeda2ed9514e79e91abdcac7be` (4.8.1) |
| pvDatabase | `8cf550ff57ab8b573a95af2e9602643b8af98110` (4.7.2) |
| pva2pva | `3a08da445b46e2e7029406d24425cdd3dfd7da24` (1.4.1) |

## Files in this directory

| File | Purpose |
| --- | --- |
| `env.sh` | Build environment, native Windows paths, and the x86 EPICS architecture |
| `CONFIG_SITE.local` | EPICS CRT, C++11 ABI, and math settings, without duplicated flags |
| `RELEASE.local.in` | EPICS Base location for individual PV-module builds |
| `msvcrt-extra.def` | Generates the missing MinGW import library entry for `_mkgmtime64` |
| `epics-mingw-environment.patch` | Replaces EPICS's `_putenv_s` call with `_putenv` for MinGW |
| `qwtconfig.pri`, `qwtbuild.pri` | Copies of the working Qwt 6.3.0 configuration |
| `QWT-LICENSE` | License for the copied Qwt files |
| `test-environment.c` | Checks EPICS environment set, replacement, and unset behavior |
| `deploy.sh` | Stages application/dependency DLLs and runs release `windeployqt` |
| `check-deployment.sh` | Rejects non-x86 binaries, debug Qt/unsupported CRT imports, and missing Qt DLLs |
| `plugin-probe.cpp`, `plugin-probe.pro` | Checks actual custom-widget construction and EPICS plugin loading |

## 1. Set up the environment

Keep dependency sources, build output, and collection directories **outside
the caQtDM source checkout**. Use paths without spaces. Defaults in `env.sh`
match this layout:

```text
C:/Qt57/5.7/mingw53_32
C:/Qt57/Tools/mingw530_32
C:/Qt57/reactos-compat
$HOME/qwt-git
$HOME/epics-base
$HOME/caqtdm-reactos-build
$HOME/caqtdm-reactos-collect
```

From the caQtDM repository root:

```bash
source caQtDM_Viewer/package/reactos/env.sh
qmake -v
gcc -dumpmachine
gcc -dumpversion
gcc -print-file-name=libz.a
```

Expect Qt 5.7.1, `i686-w64-mingw32`, and GCC 5.3.0. `env.sh` accepts path
overrides exported **before** sourcing it, such as `QTHOME`, `MINGW_HOME`,
`QWTHOME`, `EPICS_BASE`, `REACTOS_CRT_DIR`, `CAQTDM_BUILD`, and
`CAQTDM_COLLECT`. It sets `QWTVERSION=6.3.0`, matching the actual headers;
the original session's `6.1` setting was stale.

QtControls uses zlib for camera decompression. `gcc -print-file-name=libz.a`
must print an existing library path, not just `libz.a`, and `zlib.h` must be
available to this compiler. The working MinGW installation supplied both;
there was no additional zlib DLL to deploy.

Some MSYS2 launchers set `$HOME` to the Windows user profile rather than
`C:/msys64/home/<user>`. If your dependencies live elsewhere, export their
actual paths before sourcing `env.sh`.

Source the environment again in every new shell, including incremental builds:
qmake/make can otherwise regenerate Makefiles with missing dependency paths.
Do not source the repository's Unix `caQtDM_Env` for this Windows recipe.
Do not run the MSVC `.bat` build or packaging profiles.

For the same optional-plugin selection as the working build:

```bash
unset CAQTDM_WEB CAQTDM_MODBUS CAQTDM_GPS CAQTDM_OPCUA CAQTDM_ALH2UI
unset ZMQ ZMQINC ZMQLIB
```

EPICS3, EPICS4, archiveSF, internal, environment, and demo plugins are built.
ArchiveHTTP is excluded by the source's Qt-version gate because Qt 5.7 lacks
the APIs it requires. Python and ZeroMQ are not needed for this recipe.

## Known limitations

- **ArchiveHTTP is unavailable in this build.** Its build gate requires
  Qt 5.15 or newer, while this ReactOS recipe uses Qt 5.7.1. The plugin is
  intentionally not compiled or deployed, so channels using ArchiveHTTP
  cannot retrieve archive data. Copying a plugin built against newer Qt into
  this package is not a supported workaround. ArchiveSF is a separate plugin
  and does not replace ArchiveHTTP's backend. No Qt 5.7 port is included.
- **System memory reporting is incorrect on ReactOS**, as reported during
  runtime testing. An XP-compatible Windows API implementation may be needed;
  this remains deferred. Do not rely on the displayed memory figures.

## 2. Build Qwt in release mode

Use a separate Qwt 6.3.0 source tree. Copy both configuration files:

```bash
cp "$REACTOS_PACKAGE_DIR/qwtconfig.pri" "$QWTHOME/qwtconfig.pri"
cp "$REACTOS_PACKAGE_DIR/qwtbuild.pri" "$QWTHOME/qwtbuild.pri"
cd "$QWTHOME/src"
qmake src.pro -spec win32-g++ \
    "CONFIG-=debug debug_and_release build_all" "CONFIG+=release"
mingw32-make -j4
```

This builds the runtime library without examples, playground, or tests.
The expected outputs are `$QWTHOME/lib/qwt.dll` and
`$QWTHOME/lib/libqwt.a` (the DLL import library).
Qwt's Designer plugin is not required to run the viewer.

**Never copy a debug Qwt DLL.** In the original build, debug and release
targets both wrote `lib/qwt.dll`; the last target overwrote the other.
That caused the misleading `Qt5Cored.dll was not found` failure.
The qmake arguments above explicitly remove dual-mode builds. If recovering
an old dual-mode build without regenerating qmake, force its release target:

```bash
mingw32-make -B -j4 release
```

Use that last command only when the existing Makefile has a `release` target.
Do not subsequently run its debug/all target.

## 3. Configure and build EPICS

For a new checkout:

```bash
git clone https://github.com/epics-base/epics-base.git "$EPICS_BASE"
git -C "$EPICS_BASE" checkout 04442786b6bdf0d01f4f973b97cb3c24bfb5a615
git -C "$EPICS_BASE" submodule update --init --recursive
```

Generate the x86 import library for `_mkgmtime64`, then apply the environment
patch **once**, before building:

```bash
mkdir -p "$REACTOS_CRT_DIR"
cp "$REACTOS_PACKAGE_DIR/msvcrt-extra.def" "$REACTOS_CRT_DIR/"
dlltool -m i386 -D msvcrt.dll \
    -d "$REACTOS_CRT_DIR/msvcrt-extra.def" \
    -l "$REACTOS_CRT_DIR/libroscrtcompat.a"

cd "$EPICS_BASE"
git apply --check "$REACTOS_PACKAGE_DIR/epics-mingw-environment.patch"
git apply "$REACTOS_PACKAGE_DIR/epics-mingw-environment.patch"
cp "$REACTOS_PACKAGE_DIR/CONFIG_SITE.local" configure/CONFIG_SITE.local

for module in pvData normativeTypes pvAccess pvaClient pvDatabase pva2pva; do
    sed "s|@EPICS_BASE@|$EPICS_BASE|" "$REACTOS_PACKAGE_DIR/RELEASE.local.in" \
        > "$EPICS_BASE/modules/$module/configure/RELEASE.local"
done
```

Back up/merge any existing `configure/CONFIG_SITE.local` instead of overwriting
unrelated site settings. The supplied file requires `REACTOS_CRT_DIR` from
`env.sh`; it is a native Windows path with forward slashes.
Likewise, merge rather than overwrite pre-existing module `RELEASE.local`
files. Individual PV-module builds require these files even when `EPICS_BASE`
is already exported in the shell.
On an already patched tree, `git apply --reverse --check` can confirm that
the patch is present; do not apply it twice.

The compatibility settings are intentional:

- `__MINGW_USE_VC2005_COMPAT` keeps `time_t` at 64 bits, matching this caQtDM
  build. Do not remove it from just one component.
- `roscrtcompat` is an **import library**, not a replacement `msvcrt.dll`
  and not an `_mkgmtime32` shim. ReactOS exports `_mkgmtime64`; its
  `_mkgmtime32` and `_putenv_s` exports are Vista-version gated.
- The environment patch removes the explicit `_putenv_s` call. Changing
  the compiler flag alone cannot remove that call.
- `-std=c++11` must apply to all PV libraries. Mixing older TR1
  `shared_ptr` libraries with caQtDM's `std::shared_ptr` causes link errors.
- `-D_finite=isfinite` addresses the old compiler's math naming.

Build the production targets in dependency order, avoiding EPICS's unrelated
test/example targets:

```bash
(
    set -e
    for dir in configure src modules/libcom/src modules/ca/src modules/database/src; do
        mingw32-make -C "$EPICS_BASE/$dir" -j4
    done

    for module in pvData normativeTypes pvAccess pvaClient pvDatabase; do
        mingw32-make -C "$EPICS_BASE/modules/$module/configure" -j4
        mingw32-make -C "$EPICS_BASE/modules/$module/src" -j4
        if [[ "$module" == pvAccess ]]; then
            mingw32-make -C "$EPICS_BASE/modules/$module/src/ca" -j4
            mingw32-make -C "$EPICS_BASE/modules/$module/src/ioc" -j4
        fi
    done
    mingw32-make -C "$EPICS_BASE/modules/pva2pva/configure" -j4
    mingw32-make -C "$EPICS_BASE/modules/pva2pva/pdbApp" -j4
)
```

Stop on any error; do not proceed with partially built libraries. The full
EPICS test build previously failed on unqualified `isnan` calls, so it is not
the build entry point for this recipe. Use a clean dependency tree when changing
these ABI flags; an incremental build can retain objects compiled in TR1 mode.
This includes `pvAccess/src/ca` and `pvAccess/src/ioc`, not just
`pvAccess/src`. A stale TR1 `pvAccessCA.dll` can link into the package but fail
when EPICS4 is loaded at runtime with "The specified procedure could not be found".

Check the environment patch on the Windows build host:

```bash
gcc "$REACTOS_PACKAGE_DIR/test-environment.c" \
    -I"$EPICSINCLUDE" -I"$EPICSINCLUDE/os/WIN32" \
    -I"$EPICSINCLUDE/compiler/gcc" -D__MINGW_USE_VC2005_COMPAT \
    -L"$EPICSLIB" -lCom -o "$EPICS_BASE/test-environment.exe"
PATH="$(cygpath -u "$EPICS_BASE")/bin/$EPICS_HOST_ARCH:$PATH" \
    "$EPICS_BASE/test-environment.exe"
```

Expect `EPICS environment set/replace/unset passed.` This is a host-side test;
it does not replace running the deployed application on ReactOS.

## 4. Build caQtDM

Return to the same shell with `env.sh` sourced:

```bash
mkdir -p "$CAQTDM_BUILD" "$CAQTDM_COLLECT"
cd "$CAQTDM_BUILD"
qmake "$CAQTDM_SOURCE/all.pro" -spec win32-g++
mingw32-make -j4 release
```

Do not build inside the caQtDM checkout. Keep the compiler, Qt, Qwt, and EPICS
libraries consistently **32-bit and release**. The build also creates caQtDM
unit-test executables; building them does not mean they have been executed.

The source's MinGW post-link commands collect the viewer/core libraries and
control-system plugins. Parser libraries are built before QtControls. Do not
replace its `.a` import/static libraries with MSVC `.lib` files.

## 5. Deploy to ReactOS

After a successful build:

```bash
bash "$REACTOS_PACKAGE_DIR/deploy.sh"
```

This copies the viewer, core libraries, release Qwt, and all 12 EPICS DLLs,
then runs:

```bash
windeployqt --release --compiler-runtime --no-translations --no-opengl-sw \
    --dir "$CAQTDM_COLLECT" \
    "$CAQTDM_COLLECT/caQtDM.exe" "$CAQTDM_COLLECT/caQtDM_Lib.dll" \
    "$CAQTDM_COLLECT/qtcontrols.dll" "$CAQTDM_COLLECT/qwt.dll" \
    "$CAQTDM_COLLECT/designer/qtcontrols_controllers_plugin.dll" \
    "$CAQTDM_COLLECT/designer/qtcontrols_graphics_plugin.dll" \
    "$CAQTDM_COLLECT/designer/qtcontrols_monitors_plugin.dll" \
    "$CAQTDM_COLLECT/designer/qtcontrols_utilities_plugin.dll"
```

**Include `qwt.dll` as an input.** Deploying only `caQtDM.exe` missed
`Qt5OpenGL.dll`. The core libraries are also inputs so their Designer/XML
dependencies are detected.

The collection should contain:

```text
caQtDM.exe, caQtDM_Lib.dll, qtcontrols.dll, qwt.dll
adlParser.dll, prcParser.dll, alhParser.dll
ca.dll, Com.dll, dbCore.dll, dbRecStd.dll, nt.dll
pvAccess.dll, pvAccessCA.dll, pvAccessIOC.dll, pvaClient.dll
pvData.dll, pvDatabase.dll, qsrv.dll
Qt5Core.dll, Qt5Designer.dll, Qt5Gui.dll, Qt5Network.dll
Qt5OpenGL.dll, Qt5PrintSupport.dll, Qt5Svg.dll, Qt5Widgets.dll, Qt5Xml.dll
libgcc_s_dw2-1.dll, libstdc++-6.dll, libwinpthread-1.dll
platforms/qwindows.dll
controlsystems/*_plugin.dll
designer/qtcontrols_{controllers,graphics,monitors,utilities}_plugin.dll
imageformats/, iconengines/, printsupport/, bearer/
```

Qt deployment also stages ANGLE/D3D libraries when needed. Preserve those
files from the working deployment. A modern Windows `D3Dcompiler_47.dll` can
import API-set/UCRT libraries unavailable on ReactOS; successful viewer launch
does not establish that every ANGLE/OpenGL rendering path works.
Do not replace ReactOS's system `msvcrt.dll` with a Windows copy.

`deploy.sh` runs `check-deployment.sh`, which can also be invoked separately:

```bash
bash "$REACTOS_PACKAGE_DIR/check-deployment.sh"
```

It rejects debug Qt imports, `_putenv_s`, `_mkgmtime32`, non-x86 binaries,
and missing imported Qt DLLs. It is not a complete check of ReactOS system API
compatibility. Copy **the whole collection directory**, retaining the plugin
subdirectories, to ReactOS and launch `caQtDM.exe` there.
Static/import `.a` files are only needed for building, not for deployment.

The four caQtDM Designer plugins are also **required at viewer runtime**:
`QUiLoader` uses them to instantiate caQtDM widgets from UI files. They are
built in `$CAQTDM_BUILD/caQtDM_QtControls/plugins/release/` and `deploy.sh`
copies them to `designer/` beside the executable. Without them, standard Qt
widgets may appear while caQtDM widgets, their macro processing, and their PV
subscriptions are missing.

## Runtime plugin diagnosis

Import checks alone cannot prove that a Qt plugin loads. Build the small probe
outside the source checkout:

```bash
mkdir -p "$CAQTDM_BUILD/plugin-probe"
cd "$CAQTDM_BUILD/plugin-probe"
qmake "$REACTOS_PACKAGE_DIR/plugin-probe.pro" -spec win32-g++
mingw32-make -j4
cp reactos-plugin-probe.exe "$CAQTDM_COLLECT/"
"$CAQTDM_COLLECT/reactos-plugin-probe.exe"
```

Run it from the collection directory on ReactOS as well. It must instantiate
`caLineEdit`, `caNumeric`, `caLabel`, and `caShellCommand`, and load both EPICS
plugins. It exits nonzero and prints the loader error if any check fails.
Loading EPICS plugins does not prove network connectivity to an IOC.

For ReactOS loader diagnostics, run from a command prompt:

```bat
set QT_DEBUG_PLUGINS=1
set QT_LOGGING_RULES=caqtdm.lib.loadplugins.debug=true;caqtdm.lib.fileio.debug=true
caQtDM.exe -macro "NAME=value" panel.ui > reactos-runtime.log 2>&1
```

Also inspect the viewer's caQtDM Messages window for control-system plugin and
channel errors. Confirm the plugin probe works before debugging IOC addressing
or macros: a missing custom widget has no channel subscription to connect.
