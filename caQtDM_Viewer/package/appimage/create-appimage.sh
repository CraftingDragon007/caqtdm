#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "$SCRIPT_DIR/../../.." && pwd)"

REPOSITORY_NAME="${REPOSITORY_NAME:-caqtdm}"
REPOSITORY="${REPOSITORY:-https://github.com/caqtdm/${REPOSITORY_NAME}.git}"
BRANCH_OR_TAG="${BRANCH_OR_TAG:-Development}"
QT_MAJOR="${QT_MAJOR:-6}"
QMAKE_BIN="${QMAKE_BIN:-qmake${QT_MAJOR}}"
WORK_DIR="${WORK_DIR:-$SCRIPT_DIR/work}"
BUILD_DIR="${BUILD_DIR:-$SCRIPT_DIR/build}"
OUTPUT_DIR="${OUTPUT_DIR:-$SCRIPT_DIR}"
TOOLS_DIR="${TOOLS_DIR:-$SCRIPT_DIR/tools}"
APPDIR="${APPDIR:-$BUILD_DIR/caQtDM.AppDir}"
SOURCE_DIR="${SOURCE_DIR:-}"
PACKAGE_VERSION="${PACKAGE_VERSION:-}"
DOWNLOAD_TOOLS="${DOWNLOAD_TOOLS:-1}"
CAQTDM_APPIMAGE_BSREAD="${CAQTDM_APPIMAGE_BSREAD:-1}"
JOBS="${JOBS:-$(nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || printf '1')}"
DESKTOP_ID="io.github.caqtdm.caqtdm"

USE_CHECKOUT=1
SKIP_BUILD=0
APPDIR_ONLY=0

usage() {
  cat <<EOF
Usage: $(basename "$0") [OPTION...]

Build a caQtDM AppImage using the same Qt/EPICS/Qwt environment style as the
existing Linux package definitions.

Options:
  --no-checkout        Build from the current repository checkout
  --source DIR         Build from an existing caQtDM source tree
  --branch REF         Git branch or tag to clone (default: Development)
  --repo URL           Git repository to clone (default: $REPOSITORY)
  --qt-major N         Qt major version for layout/wrappers (default: 6)
  --qmake PATH         qmake executable (default: qmake6)
  --skip-build         Reuse binaries from BINARY_DIR or the default build dir
  --appdir-only        Stop after creating the AppDir, do not make an AppImage
  --without-bsread     Exclude the bsread controlsystem plugin (default: on)
  --no-download-tools  Require linuxdeploy tools to be provided locally
  --help               Show this help

Useful environment overrides:
  EPICS_BASE, EPICS_HOST_ARCH, QWTLIBNAME, QWTINCLUDE, QWTLIB, QWTVERSION,
  CAQTDM_OPCUA=auto|1|0, CAQTDM_MODBUS=auto|1|0, CAQTDM_GPS=auto|1|0,
  QTDM_RPATH, CAQTDM_NORPATH=1, CAQTDM_APPIMAGE_BSREAD=0,
  APPIMAGE_STRIP=1, BINARY_DIR, OUTPUT_DIR, LINUXDEPLOY,
  LINUXDEPLOY_PLUGIN_QT, APPIMAGETOOL, JOBS

The AppImage is written to OUTPUT_DIR, which defaults to this directory.
EOF
}

msg() {
  printf '\n==> %s\n' "$*" >&2
}

die() {
  printf 'error: %s\n' "$*" >&2
  exit 1
}

require_command() {
  command -v "$1" >/dev/null 2>&1 || die "missing required command: $1"
}

download_file() {
  local url="$1"
  local dest="$2"

  if command -v curl >/dev/null 2>&1; then
    curl --fail --location --output "$dest" "$url"
  elif command -v wget >/dev/null 2>&1; then
    wget --output-document="$dest" "$url"
  else
    die "curl or wget is required to download linuxdeploy tools"
  fi
}

tool_arch() {
  case "$(uname -m)" in
    x86_64|amd64) printf 'x86_64' ;;
    aarch64|arm64) printf 'aarch64' ;;
    *) die "unsupported AppImage tool architecture: $(uname -m)" ;;
  esac
}

ensure_tool() {
  local var_name="$1"
  local file_name="$2"
  local url="$3"
  local path="${!var_name:-$TOOLS_DIR/$file_name}"

  if [ ! -x "$path" ]; then
    [ "$DOWNLOAD_TOOLS" = "1" ] || die "$var_name is not executable: $path"
    msg "Downloading $file_name"
    mkdir -p "$TOOLS_DIR"
    download_file "$url" "$path"
    chmod +x "$path"
  fi

  printf '%s\n' "$path"
}

detect_existing_dir() {
  local candidate
  for candidate in "$@"; do
    if [ -n "$candidate" ] && [ -d "$candidate" ]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done
  return 1
}

detect_libdir() {
  local pattern="$1"
  shift

  local candidate
  for candidate in "$@"; do
    if [ -d "$candidate" ] && compgen -G "$candidate/$pattern" >/dev/null; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done
  return 1
}

copy_source_tree() {
  local source_dir="$1"
  local dest_dir="$2"
  local file_list="$BUILD_DIR/source-files.list"
  local rel_dest=""
  local rel_appimage_dir=""
  local tar_excludes=(
    --exclude=.git
    --exclude=./.git
  )

  if git -C "$source_dir" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    msg "Copying tracked source tree $source_dir to $dest_dir"
    rm -rf "$dest_dir"
    mkdir -p "$(dirname "$dest_dir")" "$BUILD_DIR"

    # A local clone provides .git metadata for version detection while the
    # explicit ls-files overlay keeps local tracked edits and skips untracked
    # build products, APKs, AppImages, IDE files, etc.
    git clone --shared --no-checkout "$source_dir" "$dest_dir"
    (
      cd "$source_dir"
      git ls-files -z | while IFS= read -r -d '' path; do
        [ -f "$path" ] || [ -L "$path" ] || continue
        printf '%s\0' "$path"
      done > "$file_list"
      tar --null -T "$file_list" -cf -
    ) | (
      cd "$dest_dir"
      tar -xf -
    )
    rm -f "$file_list"
    return 0
  fi

  case "$dest_dir/" in
    "$source_dir/"*)
      rel_dest="${dest_dir#$source_dir/}"
      tar_excludes+=(--exclude="$rel_dest")
      ;;
  esac

  case "$SCRIPT_DIR/" in
    "$source_dir/"*)
      rel_appimage_dir="${SCRIPT_DIR#$source_dir/}"
      tar_excludes+=(
        --exclude="$rel_appimage_dir/build"
        --exclude="$rel_appimage_dir/build-docker"
        --exclude="$rel_appimage_dir/tools"
        --exclude="$rel_appimage_dir/work"
        --exclude="$rel_appimage_dir/squashfs-root"
        --exclude="$rel_appimage_dir/*.AppImage"
        --exclude="$rel_appimage_dir/*.AppImage.zsync"
      )
      ;;
  esac

  msg "Copying source tree $source_dir to $dest_dir"
  rm -rf "$dest_dir"
  mkdir -p "$dest_dir"
  (
    cd "$source_dir"
    tar "${tar_excludes[@]}" -cf - .
  ) | (
    cd "$dest_dir"
    tar -xf -
  )
}

is_elf_file() {
  readelf -h "$1" >/dev/null 2>&1
}

is_patchable_elf() {
  local header
  header="$(readelf -h "$1" 2>/dev/null)" || return 1

  case "$header" in
    *"Type:"*"DYN"*|*"Type:"*"EXEC"*) return 0 ;;
    *) return 1 ;;
  esac
}

elf_has_missing_dependencies() {
  ldd "$1" 2>/dev/null | grep -q 'not found'
}

disable_non_elf_executables() {
  local directory="$1"
  local file

  while IFS= read -r -d '' file; do
    if ! is_elf_file "$file"; then
      msg "Clearing executable bit on non-ELF file before linuxdeploy: $file"
      chmod a-x "$file"
    fi
  done < <(find "$directory" -type f -perm /111 -print0)
}

find_runtime_library() {
  local name="$1"
  local triplet="$(host_triplet)"
  local candidate

  for candidate in \
    "/usr/lib/$name" \
    "/usr/lib64/$name" \
    "/lib/$name" \
    "/lib64/$name" \
    ${triplet:+"/usr/lib/$triplet/$name"} \
    ${triplet:+"/lib/$triplet/$name"}; do
    if [ -e "$candidate" ]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done

  return 1
}

copy_runtime_library() {
  local name="$1"
  local source

  if [ -e "$APPDIR/usr/lib/$name" ]; then
    return 0
  fi

  source="$(find_runtime_library "$name")" || return 0
  msg "Copying runtime library blacklisted by linuxdeploy: $name"
  install -Dm755 "$source" "$APPDIR/usr/lib/$name"
}

copy_blacklisted_runtime_libraries() {
  local library

  if compgen -G "$APPDIR/usr/lib/libhogweed.so*" >/dev/null || \
     compgen -G "$APPDIR/usr/lib/libgnutls.so*" >/dev/null; then
    copy_runtime_library libgmp.so.10
  fi

  # libGLdispatch.so.0 is blacklisted by linuxdeploy as a transitive dep of
  # libGL/libOpenGL, but Qt 6 requires libOpenGL.so.0 explicitly (not the
  # legacy libGL.so.1 stub).  RHEL 9 and other enterprise distros ship
  # libGL.so.1 but not libOpenGL.so.0, so bundle the whole GLvnd stack so the
  # AppImage is self-contained on those targets.
  for library in \
    libOpenGL.so.0 \
    libGLX.so.0 \
    libGLdispatch.so.0 \
    libEGL.so.1; do
    copy_runtime_library "$library"
  done

  # linuxdeploy also treats parts of the X11/font stack as system libraries, but
  # minimal Rocky/RHEL installations do not necessarily include them.
  for library in \
    libX11.so.6 \
    libX11-xcb.so.1 \
    libxcb.so.1 \
    libXdmcp.so.6 \
    libSM.so.6 \
    libICE.so.6 \
    libfontconfig.so.1 \
    libfreetype.so.6 \
    libexpat.so.1; do
    copy_runtime_library "$library"
  done
}

copy_qt_plugin_subdir() {
  local source_plugin_dir="$1"
  local plugin_subdir="$2"
  local source_dir="$source_plugin_dir/$plugin_subdir"
  local dest_dir="$APPDIR/usr/plugins/$plugin_subdir"
  local file

  [ -d "$source_dir" ] || return 0

  msg "Copying Qt plugin directory not deployed by linuxdeploy: $plugin_subdir"
  mkdir -p "$dest_dir"
  cp -a "$source_dir/." "$dest_dir/"

  while IFS= read -r -d '' file; do
    is_patchable_elf "$file" || continue
    patchelf --set-rpath '$ORIGIN/../../lib:$ORIGIN' "$file"
  done < <(find "$dest_dir" -type f -print0)
}

patch_appdir_rpaths() {
  local qt_dir="qt${QT_MAJOR}"
  local app_lib_dir="$APPDIR/usr/lib/caqtdm/$qt_dir"
  local top_rpath='$ORIGIN:$ORIGIN/controlsystems:$ORIGIN/designer:$ORIGIN/../../..'
  local plugin_rpath='$ORIGIN:$ORIGIN/..:$ORIGIN/../../..'
  local file

  require_command patchelf

  while IFS= read -r -d '' file; do
    is_patchable_elf "$file" || continue
    patchelf --set-rpath "$top_rpath" "$file"
  done < <(find "$app_lib_dir" -maxdepth 1 -type f -print0)

  for file in "$app_lib_dir"/controlsystems/*.so "$app_lib_dir"/designer/*.so; do
    [ -e "$file" ] || continue
    is_patchable_elf "$file" || continue
    patchelf --set-rpath "$plugin_rpath" "$file"
  done
}

setup_appimage_strip_env() {
  case "${APPIMAGE_STRIP:-0}" in
    1|true|TRUE|yes|YES|on|ON)
      unset NO_STRIP
      msg "AppImage stripping enabled"
      ;;
    *)
      export NO_STRIP="${NO_STRIP:-true}"
      msg "AppImage stripping disabled via NO_STRIP=$NO_STRIP"
      ;;
  esac
}

qt_module_library_path() {
  local library_name="$1"
  local qt_lib_dir
  local candidate

  qt_lib_dir="$($QMAKE_BIN -query QT_INSTALL_LIBS 2>/dev/null || true)"
  for candidate in \
    ${qt_lib_dir:+"$qt_lib_dir/libQt${QT_MAJOR}${library_name}.so.${QT_MAJOR}"} \
    ${qt_lib_dir:+"$qt_lib_dir/libQt${QT_MAJOR}${library_name}.so"} \
    "/usr/lib/libQt${QT_MAJOR}${library_name}.so.${QT_MAJOR}" \
    "/usr/lib64/libQt${QT_MAJOR}${library_name}.so.${QT_MAJOR}"; do
    if [ -e "$candidate" ]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done

  return 1
}

qt_module_library_usable() {
  local module="$1"
  local library_name="$2"
  local library
  local log_dir="$BUILD_DIR/qt-module-$module"
  local log_file="$log_dir/ldd-r.log"
  local qt_lib_dir

  library="$(qt_module_library_path "$library_name")" || return 1
  qt_lib_dir="$(dirname "$library")"
  mkdir -p "$log_dir"

  LD_LIBRARY_PATH="$qt_lib_dir${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
    ldd -r "$library" > "$log_file" 2>&1 || true
  ! grep -Eq 'not found|undefined symbol' "$log_file"
}

appimage_bsread_enabled() {
  case "$CAQTDM_APPIMAGE_BSREAD" in
    1|true|TRUE|yes|YES|on|ON) return 0 ;;
    0|false|FALSE|no|NO|off|OFF|"") return 1 ;;
    *) die "CAQTDM_APPIMAGE_BSREAD must be 1 or 0" ;;
  esac
}

filter_appdir_controlsystem_plugins() {
  local app_lib_dir="$1"

  if appimage_bsread_enabled; then
    :
  else
    remove_appdir_controlsystem_plugin "$app_lib_dir" libbsread_Plugin.so \
      "use --without-bsread only when bsread should be excluded"
  fi

  [ -n "${CAQTDM_OPCUA:-}" ] || remove_appdir_controlsystem_plugin "$app_lib_dir" \
    libopcua_plugin.so "Qt OPC UA is not enabled"
  [ -n "${CAQTDM_MODBUS:-}" ] || remove_appdir_controlsystem_plugin "$app_lib_dir" \
    libmodbus_plugin.so "Qt Serial Bus is not enabled"
  [ -n "${CAQTDM_GPS:-}" ] || remove_appdir_controlsystem_plugin "$app_lib_dir" \
    libgps_plugin.so "Qt Positioning is not enabled"
}

remove_appdir_controlsystem_plugin() {
  local app_lib_dir="$1"
  local plugin_name="$2"
  local reason="$3"
  local plugin="$app_lib_dir/controlsystems/$plugin_name"

  if [ -e "$plugin" ]; then
    msg "Excluding $plugin_name from AppImage; $reason"
    rm -f "$plugin"
  fi
}

prepare_qt_plugin_tree() {
  local source_dir="$($QMAKE_BIN -query QT_INSTALL_PLUGINS 2>/dev/null || true)"
  local filtered_dir="$BUILD_DIR/qt-plugins-filtered"
  local dir file rel

  if [ -z "$source_dir" ] || [ ! -d "$source_dir" ]; then
    die "could not determine Qt plugin directory from $QMAKE_BIN"
  fi

  rm -rf "$filtered_dir"

  while IFS= read -r -d '' dir; do
    rel="${dir#$source_dir}"
    install -dm755 "$filtered_dir$rel"
  done < <(find "$source_dir" -type d -print0)

  while IFS= read -r -d '' file; do
    rel="${file#$source_dir/}"
    if is_elf_file "$file" && elf_has_missing_dependencies "$file"; then
      msg "Skipping Qt plugin with unresolved dependencies: $rel"
      continue
    fi
    install -Dm644 "$file" "$filtered_dir/$rel"
  done < <(find "$source_dir" -type f -print0)

  printf '%s\n' "$filtered_dir"
}

qt_designer_binary() {
  local qt_bin_dir="$($QMAKE_BIN -query QT_INSTALL_BINS 2>/dev/null || true)"
  local candidate

  for candidate in \
    ${qt_bin_dir:+"$qt_bin_dir/designer"} \
    ${qt_bin_dir:+"$qt_bin_dir/designer$QT_MAJOR"} \
    ${qt_bin_dir:+"$qt_bin_dir/designer-qt$QT_MAJOR"} \
    ${qt_bin_dir:+"$qt_bin_dir/designer-qt"}; do
    if [ -x "$candidate" ]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done

  return 1
}

create_qmake_wrapper() {
  local qt_plugin_dir="$1"
  local wrapper="$BUILD_DIR/qmake-appimage-wrapper"

  cat > "$wrapper" <<EOF
#!/usr/bin/env bash
set -euo pipefail

REAL_QMAKE="$QMAKE_BIN"
QT_PLUGIN_DIR="$qt_plugin_dir"

if [ "\${1:-}" = "-query" ] && [ "\${2:-}" = "QT_INSTALL_PLUGINS" ]; then
  printf '%s\\n' "\$QT_PLUGIN_DIR"
  exit 0
fi

if [ "\${1:-}" = "-query" ] && [ "\$#" -eq 1 ]; then
  "\$REAL_QMAKE" -query | sed "s|^QT_INSTALL_PLUGINS:.*|QT_INSTALL_PLUGINS:\$QT_PLUGIN_DIR|"
  exit 0
fi

exec "\$REAL_QMAKE" "\$@"
EOF
  chmod +x "$wrapper"
  printf '%s\n' "$wrapper"
}

host_triplet() {
  gcc -dumpmachine 2>/dev/null || true
}

qt_requires_cxx17() {
  case "$QT_MAJOR" in
    6|[7-9]|[1-9][0-9]*) return 0 ;;
    *) return 1 ;;
  esac
}

apply_qt6_cxx17_patch() {
  qt_requires_cxx17 || return 0

  local qtdefs="$SOURCE_DIR/caQtDM_Viewer/qtdefs.pri"
  [ -f "$qtdefs" ] || return 0

  if grep -Eq 'CONFIG[[:space:]]*\+=[[:space:]]*.*c\+\+17' "$qtdefs"; then
    return 0
  fi

  msg "Patching qtdefs.pri to request C++17 for Qt $QT_MAJOR"
  python3 - "$qtdefs" <<'PY'
from pathlib import Path
import sys

path = Path(sys.argv[1])
text = path.read_text()
block = "\ncontains(QT_VER_MAJ, 6) {\n    CONFIG += c++17\n}\n"
marker = "QT_VER_PAT = $$member(QT_VERSION, 2)\n"

if "c++17" in text:
    raise SystemExit(0)

if marker in text:
    text = text.replace(marker, marker + block, 1)
else:
    text = block.lstrip("\n") + "\n" + text

path.write_text(text)
PY
}

detect_python_version() {
  local token value

  for token in $(python3-config --ldflags --libs 2>/dev/null || true); do
    case "$token" in
      -lpython*)
        value="${token#-lpython}"
        if [ -n "$value" ]; then
          printf '%s\n' "$value"
          return 0
        fi
        ;;
    esac
  done

  value="$(python3 -c 'import sysconfig; print(sysconfig.get_config_var("LDVERSION") or sysconfig.get_config_var("VERSION") or "")' 2>/dev/null || true)"
  if [ -n "$value" ]; then
    printf '%s\n' "$value"
    return 0
  fi

  python3 --version 2>&1 | cut -d ' ' -f 2 | cut -d '.' -f 1-2
}

detect_python_include() {
  local token candidate

  for token in $(python3-config --includes 2>/dev/null || true); do
    case "$token" in
      -I*)
        candidate="${token#-I}"
        if [ -f "$candidate/Python.h" ]; then
          printf '%s\n' "$candidate"
          return 0
        fi
        ;;
    esac
  done

  candidate="$(python3 -c 'import sysconfig; print(sysconfig.get_path("include") or "")' 2>/dev/null || true)"
  if [ -f "$candidate/Python.h" ]; then
    printf '%s\n' "$candidate"
    return 0
  fi

  printf '/usr/include/python%s\n' "$(detect_python_version)"
}

detect_python_libdir() {
  local token candidate version

  for token in $(python3-config --ldflags 2>/dev/null || true); do
    case "$token" in
      -L*)
        candidate="${token#-L}"
        if compgen -G "$candidate/libpython*.so*" >/dev/null; then
          printf '%s\n' "$candidate"
          return 0
        fi
        ;;
    esac
  done

  candidate="$(python3 -c 'import sysconfig; print(sysconfig.get_config_var("LIBDIR") or "")' 2>/dev/null || true)"
  if compgen -G "$candidate/libpython*.so*" >/dev/null; then
    printf '%s\n' "$candidate"
    return 0
  fi

  version="$(detect_python_version)"
  detect_libdir "libpython${version}.so*" /usr/lib64 /usr/lib /usr/local/lib64 /usr/local/lib || printf '/usr/lib'
}

detect_python_path() {
  local name="$1"
  python3 -c 'import sysconfig, sys; print(sysconfig.get_path(sys.argv[1]) or "")' "$name" 2>/dev/null || true
}

install_python_runtime() {
  local stdlib platstdlib dest python_dir_name

  stdlib="$(detect_python_path stdlib)"
  platstdlib="$(detect_python_path platstdlib)"
  [ -d "$stdlib" ] || die "could not determine Python stdlib directory"

  python_dir_name="$(basename "$stdlib")"
  dest="$APPDIR/usr/lib/$python_dir_name"

  msg "Installing Python runtime from $stdlib"
  rm -rf "$dest"
  mkdir -p "$dest"

  (
    cd "$stdlib"
    tar \
      --exclude='__pycache__' \
      --exclude='*.pyc' \
      --exclude='test' \
      --exclude='tests' \
      --exclude='idlelib' \
      --exclude='tkinter' \
      --exclude='turtledemo' \
      --exclude='ensurepip' \
      -cf - .
  ) | (
    cd "$dest"
    tar -xf -
  )

  if [ -n "$platstdlib" ] && [ -d "$platstdlib/lib-dynload" ] && [ "$platstdlib" != "$stdlib" ]; then
    msg "Installing Python extension modules from $platstdlib/lib-dynload"
    mkdir -p "$dest/lib-dynload"
    cp -a "$platstdlib/lib-dynload/." "$dest/lib-dynload/"
  fi

  [ -f "$dest/encodings/__init__.py" ] || die "Python runtime copy is missing encodings/__init__.py"
}

resolve_qmake() {
  if command -v "$QMAKE_BIN" >/dev/null 2>&1; then
    QMAKE_BIN="$(command -v "$QMAKE_BIN")"
  elif [ "$QT_MAJOR" = "6" ] && command -v qmake6 >/dev/null 2>&1; then
    QMAKE_BIN="$(command -v qmake6)"
  elif command -v qmake >/dev/null 2>&1; then
    QMAKE_BIN="$(command -v qmake)"
  else
    die "qmake was not found; set QMAKE_BIN or install Qt development tools"
  fi

  local qt_version
  qt_version="$($QMAKE_BIN -query QT_VERSION 2>/dev/null || true)"
  if [ -n "$qt_version" ]; then
    QT_MAJOR="${qt_version%%.*}"
  fi
}

qt_has_module() {
  local module="$1"
  local detect_dir="$BUILD_DIR/qt-module-$module"
  local project_file="$detect_dir/detect.pro"
  local log_file="$detect_dir/qmake.log"

  rm -rf "$detect_dir"
  mkdir -p "$detect_dir"

  cat > "$project_file" <<EOF
TEMPLATE = aux
qtHaveModule($module) {
    message(CAQTDM_QT_MODULE_AVAILABLE_$module)
} else {
    message(CAQTDM_QT_MODULE_MISSING_$module)
}
EOF

  if (cd "$detect_dir" && "$QMAKE_BIN" "$project_file" > "$log_file" 2>&1); then
    grep -q "CAQTDM_QT_MODULE_AVAILABLE_$module" "$log_file"
  else
    return 1
  fi
}

setup_optional_features() {
  setup_optional_feature CAQTDM_MODBUS serialbus "Qt Serial Bus" SerialBus
  setup_optional_feature CAQTDM_GPS positioning "Qt Positioning" Positioning
  setup_optional_feature CAQTDM_OPCUA opcua "Qt OPC UA" OpcUa
}

setup_optional_feature() {
  local env_name="$1"
  local module="$2"
  local label="$3"
  local library_name="${4:-}"
  local value="${!env_name:-auto}"
  local ldd_log="$BUILD_DIR/qt-module-$module/ldd-r.log"

  case "$value" in
    0|false|FALSE|no|NO|off|OFF)
      unset "$env_name"
      ;;
    1|true|TRUE|yes|YES|on|ON)
      if [ -n "$library_name" ] && ! qt_module_library_usable "$module" "$library_name"; then
        die "$label runtime library is not usable; see $ldd_log"
      fi
      export "$env_name=1"
      ;;
    auto|AUTO|"")
      if qt_has_module "$module"; then
        if [ -n "$library_name" ] && ! qt_module_library_usable "$module" "$library_name"; then
          msg "$label module found but runtime library is not usable, building without $env_name (see $ldd_log)"
          unset "$env_name"
        else
          msg "$label module found, enabling $env_name"
          export "$env_name=1"
        fi
      else
        msg "$label module not found, building without $env_name"
        unset "$env_name"
      fi
      ;;
    *)
      die "$env_name must be auto, 1, or 0"
      ;;
  esac
}

prepare_source() {
  if [ -n "$SOURCE_DIR" ]; then
    USE_CHECKOUT=0
  fi

  if [ "$USE_CHECKOUT" -eq 1 ]; then
    SOURCE_DIR="$WORK_DIR/$REPOSITORY_NAME"
    msg "Cloning $REPOSITORY ($BRANCH_OR_TAG)"
    rm -rf "$SOURCE_DIR"
    mkdir -p "$WORK_DIR"
    git clone --depth 1 --branch "$BRANCH_OR_TAG" "$REPOSITORY" "$SOURCE_DIR"
  else
    local source_origin="${SOURCE_DIR:-$REPO_ROOT}"
    source_origin="$(cd -- "$source_origin" && pwd)"
    [ -f "$source_origin/all.pro" ] || die "source tree does not contain all.pro: $source_origin"
    mkdir -p "$WORK_DIR"
    SOURCE_DIR="$WORK_DIR/${REPOSITORY_NAME}-source"
    copy_source_tree "$source_origin" "$SOURCE_DIR"
  fi

  [ -f "$SOURCE_DIR/all.pro" ] || die "source tree does not contain all.pro: $SOURCE_DIR"
}

detect_package_version() {
  if [ -n "$PACKAGE_VERSION" ]; then
    return
  fi

  local version_file="$SOURCE_DIR/caQtDM_Viewer/qtdefs.pri"
  if [ -f "$version_file" ]; then
    PACKAGE_VERSION="$(
      sed -n 's/^[[:space:]]*CAQTDM_VERSION[[:space:]]*=[[:space:]]*[Vv]\{0,1\}\([0-9][0-9.]*\).*/\1/p' "$version_file" | head -n 1
    )"
  fi
  PACKAGE_VERSION="${PACKAGE_VERSION:-4.6.1}"
}

detect_qwt_libname() {
  if [ -n "${QWTLIBNAME:-}" ]; then
    printf '%s\n' "$QWTLIBNAME"
    return
  fi

  local triplet="$(host_triplet)"
  local libdirs=(
    "${QWTHOME:-/usr}/lib"
    "${QWTHOME:-/usr}/lib64"
    "/usr/lib"
    "/usr/lib64"
    "/usr/local/lib"
    "/usr/local/lib64"
  )
  if [ -n "$triplet" ]; then
    libdirs+=("/usr/lib/$triplet" "/usr/local/lib/$triplet")
  fi

  local candidates=()
  if [ "$QT_MAJOR" = "6" ]; then
    candidates=(qwt-qt6 qwt qwt6)
  elif [ "$QT_MAJOR" = "5" ]; then
    candidates=(qwt-qt5 qwt)
  else
    candidates=(qwt)
  fi

  local candidate
  for candidate in "${candidates[@]}"; do
    if detect_libdir "lib${candidate}.so*" "${libdirs[@]}" >/dev/null; then
      printf '%s\n' "$candidate"
      return
    fi
  done

  printf '%s\n' "${candidates[0]}"
}

setup_build_env() {
  local qt_dir="qt${QT_MAJOR}"
  local triplet="$(host_triplet)"
  local binary_dir="${BINARY_DIR:-$BUILD_DIR/opt/caqtdm/lib/$qt_dir}"

  export QTHOME="${QTHOME:-/usr}"
  export QWTHOME="${QWTHOME:-/usr}"
  export QWTLIBNAME="$(detect_qwt_libname)"
  export QWTINCLUDE="${QWTINCLUDE:-$(detect_existing_dir "$QWTHOME/include/$QWTLIBNAME" "$QWTHOME/include/qwt" "$QWTHOME/include/$qt_dir/qwt" /usr/include/qwt "/usr/include/$qt_dir/qwt" || printf '/usr/include/qwt')}"
  export QWTLIB="${QWTLIB:-$(detect_libdir "lib${QWTLIBNAME}.so*" "$QWTHOME/lib" "$QWTHOME/lib64" /usr/lib /usr/lib64 ${triplet:+"/usr/lib/$triplet"} || printf '/usr/lib')}"
  export QWTVERSION="${QWTVERSION:-6.1}"

  export EPICS_HOST_ARCH="${EPICS_HOST_ARCH:-linux-$(uname -m)}"
  export EPICS_BASE="${EPICS_BASE:-${EPICS_BASE_TARGET:-$(detect_existing_dir /usr/lib/epics /usr/local/epics/base-7.0.10 /usr/local/epics/base-7.0.9 /usr || printf '/usr')}}"
  export EPICSINCLUDE="${EPICSINCLUDE:-$(detect_existing_dir "$EPICS_BASE/include/epics/include" "$EPICS_BASE/include" || printf '%s/include' "$EPICS_BASE")}"
  export EPICSLIB="${EPICSLIB:-$(detect_existing_dir "$EPICS_BASE/lib/$EPICS_HOST_ARCH" "$EPICS_BASE/lib/epics/lib/$EPICS_HOST_ARCH" "$EPICS_BASE/lib" || printf '%s/lib' "$EPICS_BASE")}"
  export EPICSEXTENSIONS="${EPICSEXTENSIONS:-$EPICS_BASE/extensions}"

  export ZMQ="${ZMQ:-/usr}"
  export ZMQINC="${ZMQINC:-$ZMQ/include}"
  export ZMQLIB="${ZMQLIB:-$(detect_libdir 'libzmq.so*' "$ZMQ/lib" "$ZMQ/lib64" /usr/lib /usr/lib64 ${triplet:+"/usr/lib/$triplet"} || printf '%s/lib' "$ZMQ")}"

  export PYTHONVERSION="${PYTHONVERSION:-$(detect_python_version)}"
  export PYTHONINCLUDE="${PYTHONINCLUDE:-$(detect_python_include)}"
  export PYTHONLIB="${PYTHONLIB:-$(detect_python_libdir)}"
  export HOMEBREW_MAKE_JOBS="${HOMEBREW_MAKE_JOBS:-$JOBS}"

  export CAQTDM_COLLECT="$binary_dir"
  export QTCONTROLS_LIBS="$binary_dir"
  export QTBASE="$binary_dir"
  export CAQTDM_CA_ARCHIVELIBS="$binary_dir"
  export CAQTDM_LOGGING_ARCHIVELIBS="$binary_dir"
  export QTDM_LIBINSTALL="$EPICSEXTENSIONS/lib/$EPICS_HOST_ARCH"
  export QTDM_BININSTALL="$EPICSEXTENSIONS/bin/$EPICS_HOST_ARCH"

  if [ -n "${CAQTDM_NORPATH:-}" ]; then
    msg "CAQTDM_NORPATH is set, building without caQtDM RPATH"
  else
    export QTDM_RPATH="${QTDM_RPATH:-\$ORIGIN:\$ORIGIN/..:\$ORIGIN/../..:\$ORIGIN/../../..:\$ORIGIN/controlsystems:\$ORIGIN/designer}"
  fi

  setup_optional_features

  mkdir -p "$binary_dir"
  BINARY_DIR="$binary_dir"
}

build_caqtdm() {
  local qmake_args=()

  qt_requires_cxx17 && qmake_args+=("CONFIG+=c++17")

  if [ "$SKIP_BUILD" -eq 1 ]; then
    msg "Skipping build, using $BINARY_DIR"
    [ -x "$BINARY_DIR/caQtDM" ] || die "caQtDM binary not found in BINARY_DIR: $BINARY_DIR"
    return
  fi

  msg "Building caQtDM with $QMAKE_BIN"
  (
    cd "$SOURCE_DIR"
    "$QMAKE_BIN" ./all.pro "${qmake_args[@]}"
    make -j"$JOBS"
  )

  [ -x "$BINARY_DIR/caQtDM" ] || die "build finished but $BINARY_DIR/caQtDM was not created"
}

append_launcher_environment() {
  local path="$1"
  local target_qt_dir="$2"

  cat >> "$path" <<EOF
if [ -z "\${APPDIR:-}" ]; then
  APPDIR="\$(cd "\$(dirname "\$(readlink -f "\$0")")/../.." && pwd)"
fi

CAQTDM_LIB_DIR="\$APPDIR/usr/lib/caqtdm/$target_qt_dir"
CAQTDM_PYTHON_DIR="\$(find "\$APPDIR/usr/lib" -maxdepth 1 -type d -name 'python3*' | sort | tail -n 1 || true)"
export LD_LIBRARY_PATH="\$CAQTDM_LIB_DIR:\$CAQTDM_LIB_DIR/controlsystems:\$CAQTDM_LIB_DIR/designer:\$APPDIR/usr/lib\${LD_LIBRARY_PATH:+:\$LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="\$APPDIR/usr/plugins:\$APPDIR/usr/lib/$target_qt_dir/plugins:\$CAQTDM_LIB_DIR\${QT_PLUGIN_PATH:+:\$QT_PLUGIN_PATH}"
export QT_QPA_PLATFORM_PLUGIN_PATH="\$APPDIR/usr/plugins/platforms"
if [ -n "\$CAQTDM_PYTHON_DIR" ]; then
  export PYTHONHOME="\$APPDIR/usr"
  export PYTHONPATH="\$CAQTDM_PYTHON_DIR:\$CAQTDM_PYTHON_DIR/lib-dynload\${PYTHONPATH:+:\$PYTHONPATH}"
fi
unset AT_SPI_BUS_ADDRESS
unset QT_LINUX_ACCESSIBILITY_ALWAYS_ON
EOF
}

write_launcher() {
  local path="$1"
  local target_qt_dir="$2"

  cat > "$path" <<'EOF'
#!/bin/sh
set -eu
EOF
  append_launcher_environment "$path" "$target_qt_dir"
  cat >> "$path" <<'EOF'

if [ "${1:-}" = "--designer" ]; then
  shift
  exec "$APPDIR/usr/bin/caqtdm-designer" "$@"
fi

exec "$CAQTDM_LIB_DIR/caQtDM" "$@"
EOF
  chmod +x "$path"
}

write_designer_launcher() {
  local path="$1"
  local target_qt_dir="$2"

  cat > "$path" <<'EOF'
#!/bin/sh
set -eu
EOF
  append_launcher_environment "$path" "$target_qt_dir"
  cat >> "$path" <<'EOF'

exec "$APPDIR/usr/bin/qt-designer" "$@"
EOF
  chmod +x "$path"
}

install_designer_binary() {
  local designer
  designer="$(qt_designer_binary)" || die "Qt Designer was not found; ensure qttools designer is built for $QMAKE_BIN"

  msg "Installing Qt Designer from $designer"
  install -Dm755 "$designer" "$APPDIR/usr/bin/qt-designer"
}

install_designer_plugin_links() {
  local qt_dir="$1"
  local app_lib_dir="$2"
  local plugin

  [ -d "$app_lib_dir/designer" ] || return 0

  for plugin in "$app_lib_dir"/designer/libqtcontrols_*.so; do
    [ -e "$plugin" ] || continue
    ln -sf "../../../caqtdm/$qt_dir/designer/$(basename "$plugin")" \
      "$APPDIR/usr/lib/$qt_dir/plugins/designer/$(basename "$plugin")"
  done
}

install_appdir_documentation() {
  local doc_dir="$SOURCE_DIR/caQtDM_QtControls/doc"
  local doc

  [ -d "$doc_dir" ] || return 0

  for doc in "$doc_dir"/*.qch "$doc_dir"/*.html "$doc_dir"/*.css; do
    [ -e "$doc" ] || continue
    cp -a "$doc" "$APPDIR/usr/share/doc/caqtdm/"
  done
}

create_appdir() {
  local qt_dir="qt${QT_MAJOR}"
  local app_lib_dir="$APPDIR/usr/lib/caqtdm/$qt_dir"

  msg "Creating AppDir at $APPDIR"
  rm -rf "$APPDIR"
  install -dm755 \
    "$app_lib_dir" \
    "$APPDIR/usr/bin" \
    "$APPDIR/usr/lib/$qt_dir/plugins/designer" \
    "$APPDIR/usr/share/doc/caqtdm"

  cp -a "$BINARY_DIR/." "$app_lib_dir/"
  install_designer_binary
  install_python_runtime
  filter_appdir_controlsystem_plugins "$app_lib_dir"
  patch_appdir_rpaths
  install_designer_plugin_links "$qt_dir" "$app_lib_dir"
  install_appdir_documentation

  if [ "$APPDIR_ONLY" -eq 1 ]; then
    finalize_appdir
  fi
}

install_appdir_launchers() {
  local qt_dir="qt${QT_MAJOR}"
  local app_lib_dir="$APPDIR/usr/lib/caqtdm/$qt_dir"

  install -dm755 "$APPDIR/usr/bin"
  write_launcher "$APPDIR/usr/bin/caQtDM" "$qt_dir"

  cat > "$APPDIR/usr/bin/caqtdm" <<'EOF'
#!/bin/sh
set -eu
DIR="$(cd "$(dirname "$(readlink -f "$0")")" && pwd)"
exec "$DIR/caQtDM" -style Fusion "$@"
EOF
  chmod +x "$APPDIR/usr/bin/caqtdm"

  write_designer_launcher "$APPDIR/usr/bin/caqtdm-designer" "$qt_dir"

  if [ -x "$app_lib_dir/adl2ui" ]; then
    ln -sf "../lib/caqtdm/$qt_dir/adl2ui" "$APPDIR/usr/bin/adl2ui"
  fi
  if [ -x "$app_lib_dir/edl2ui" ]; then
    ln -sf "../lib/caqtdm/$qt_dir/edl2ui" "$APPDIR/usr/bin/edl2ui"
  fi

  ln -sf usr/bin/caQtDM "$APPDIR/AppRun"
}

install_appdir_metadata() {
  local metadata_dir="$SCRIPT_DIR"

  install -dm755 \
    "$APPDIR/usr/share/applications" \
    "$APPDIR/usr/share/metainfo"

  if [ -f "$metadata_dir/$DESKTOP_ID.desktop" ]; then
    install -Dm644 "$metadata_dir/$DESKTOP_ID.desktop" "$APPDIR/usr/share/applications/$DESKTOP_ID.desktop"
  else
    cat > "$APPDIR/usr/share/applications/$DESKTOP_ID.desktop" <<EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=caQtDM
Comment=caQtDM is a popular Epics framework for developing panels
Categories=Science;DataVisualization;
Icon=$DESKTOP_ID
Exec=caQtDM
Terminal=false
EOF
  fi

  if [ -f "$metadata_dir/$DESKTOP_ID.metainfo.xml" ]; then
    install -Dm644 "$metadata_dir/$DESKTOP_ID.metainfo.xml" "$APPDIR/usr/share/metainfo/$DESKTOP_ID.appdata.xml"
  fi

  if [ -d "$metadata_dir/icons" ]; then
    install -dm755 "$APPDIR/usr/share/icons/hicolor"
    cp -a "$metadata_dir/icons/." "$APPDIR/usr/share/icons/hicolor/"
  fi

  local root_icon="$metadata_dir/icons/256x256/apps/$DESKTOP_ID.png"
  [ -f "$root_icon" ] || die "no AppImage icon found: $root_icon"

  rm -f "$APPDIR/$DESKTOP_ID.desktop" "$APPDIR/$DESKTOP_ID.png"
  ln -sf "usr/share/applications/$DESKTOP_ID.desktop" "$APPDIR/$DESKTOP_ID.desktop"
  cp "$root_icon" "$APPDIR/$DESKTOP_ID.png"
}

finalize_appdir() {
  install_appdir_metadata
  install_appdir_launchers
}

make_appimage() {
  if [ "$APPDIR_ONLY" -ne 0 ]; then
    return 0
  fi

  require_command patchelf
  require_command readelf
  require_command ldd

  local arch="$(tool_arch)"
  local linuxdeploy
  local linuxdeploy_qt
  local appimagetool_path
  linuxdeploy="$(ensure_tool LINUXDEPLOY "linuxdeploy-$arch.AppImage" "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-$arch.AppImage")"
  linuxdeploy_qt="$(ensure_tool LINUXDEPLOY_PLUGIN_QT "linuxdeploy-plugin-qt-$arch.AppImage" "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-$arch.AppImage")"
  appimagetool_path="$(ensure_tool APPIMAGETOOL "appimagetool-$arch.AppImage" "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-$arch.AppImage")"

  msg "Creating AppImage in $OUTPUT_DIR"
  mkdir -p "$OUTPUT_DIR"

  local qt_dir="qt${QT_MAJOR}"
  local app_lib_dir="$APPDIR/usr/lib/caqtdm/$qt_dir"
  local desktop_file="$APPDIR/usr/share/applications/$DESKTOP_ID.desktop"
  local root_icon_file="$APPDIR/$DESKTOP_ID.png"
  local qt_plugin_dir
  local qmake_wrapper
  local output_file
  qt_plugin_dir="$(prepare_qt_plugin_tree)"
  qmake_wrapper="$(create_qmake_wrapper "$qt_plugin_dir")"
  output_file="$OUTPUT_DIR/caQtDM-${PACKAGE_VERSION}-${arch}.AppImage"
  disable_non_elf_executables "$app_lib_dir"

  local deploy_args=(
    --appdir "$APPDIR"
    --desktop-file "$desktop_file"
    --icon-file "$root_icon_file"
    --executable "$app_lib_dir/caQtDM"
    --executable "$APPDIR/usr/bin/qt-designer"
    --plugin qt
  )

  local library
  while IFS= read -r -d '' library; do
    if is_elf_file "$library"; then
      deploy_args+=(--library "$library")
    else
      msg "Skipping non-ELF library candidate: $library"
    fi
  done < <(find "$app_lib_dir" -type f -name '*.so*' -print0)

  (
    cd "$OUTPUT_DIR"
    export APPIMAGE_EXTRACT_AND_RUN="${APPIMAGE_EXTRACT_AND_RUN:-1}"
    export QMAKE="$qmake_wrapper"
    export VERSION="$PACKAGE_VERSION"
    export PATH="$(dirname "$linuxdeploy_qt"):$PATH"
    export LD_LIBRARY_PATH="$app_lib_dir:$app_lib_dir/controlsystems:$app_lib_dir/designer:${EPICSLIB:-}:${LD_LIBRARY_PATH:-}"
    setup_appimage_strip_env
    install_appdir_metadata
    [ -f "$desktop_file" ] || die "AppImage desktop file was not created: $desktop_file"
    [ -f "$root_icon_file" ] || die "AppImage icon file was not created: $root_icon_file"
    "$linuxdeploy" "${deploy_args[@]}"
    copy_blacklisted_runtime_libraries
    [ -z "${CAQTDM_OPCUA:-}" ] || copy_qt_plugin_subdir "$qt_plugin_dir" opcua

    finalize_appdir

    rm -f "$output_file"
    ARCH="$arch" "$appimagetool_path" "$APPDIR" "$output_file"
  )
}

parse_args() {
  while [ "$#" -gt 0 ]; do
    case "$1" in
      --no-checkout)
        USE_CHECKOUT=0
        ;;
      --source)
        [ "$#" -ge 2 ] || die "--source requires a directory"
        SOURCE_DIR="$2"
        shift
        ;;
      --branch)
        [ "$#" -ge 2 ] || die "--branch requires a ref"
        BRANCH_OR_TAG="$2"
        shift
        ;;
      --repo|--repository)
        [ "$#" -ge 2 ] || die "$1 requires a URL"
        REPOSITORY="$2"
        shift
        ;;
      --qt-major)
        [ "$#" -ge 2 ] || die "--qt-major requires a version number"
        QT_MAJOR="$2"
        QMAKE_BIN="qmake$QT_MAJOR"
        shift
        ;;
      --qmake)
        [ "$#" -ge 2 ] || die "--qmake requires a path"
        QMAKE_BIN="$2"
        shift
        ;;
      --skip-build)
        SKIP_BUILD=1
        ;;
      --appdir-only)
        APPDIR_ONLY=1
        ;;
      --without-bsread)
        CAQTDM_APPIMAGE_BSREAD=0
        ;;
      --no-download-tools)
        DOWNLOAD_TOOLS=0
        ;;
      --help|-h)
        usage
        exit 0
        ;;
      *)
        die "unknown option: $1"
        ;;
    esac
    shift
  done
}

check_glibc_baseline() {
  # AppImages bundle all libraries except libc/libstdc++/libgcc_s, which must
  # come from the target host.  The AppImage will only run on hosts with glibc
  # >= the version used on the build machine.
  #
  # Recommended baseline: AlmaLinux/Rocky/RHEL 8 (glibc 2.28).
  # Anything newer than that breaks RHEL 8 compatibility.
  local glibc_version
  glibc_version="$(ldd --version 2>/dev/null | awk 'NR==1{print $NF}')" || true

  if [ -z "$glibc_version" ]; then
    return
  fi

  # Compare major.minor only (e.g. 2.28)
  local major minor
  major="${glibc_version%%.*}"
  minor="${glibc_version#*.}"
  minor="${minor%%.*}"

  # Warn if glibc > 2.28
  if [ "$major" -gt 2 ] || { [ "$major" -eq 2 ] && [ "$minor" -gt 28 ]; }; then
    msg "WARNING: Build host glibc is $glibc_version (> 2.28)."
    msg "  The resulting AppImage will NOT run on RHEL 8 or other systems"
    msg "  with glibc < $glibc_version."
    msg "  For a portable build, use the provided Docker wrapper:"
    msg "    caQtDM_Viewer/package/appimage/docker-build.sh [options]"
    msg "  Continuing anyway - set CAQTDM_APPIMAGE_IGNORE_GLIBC=1 to suppress this."
    if [ "${CAQTDM_APPIMAGE_IGNORE_GLIBC:-0}" != "1" ]; then
      printf '\n  Press Enter to continue or Ctrl-C to abort...' >&2
      read -r || true
    fi
  fi
}

main() {
  parse_args "$@"

  require_command git
  require_command make
  require_command python3
  require_command ldd
  check_glibc_baseline
  resolve_qmake
  prepare_source
  apply_qt6_cxx17_patch
  detect_package_version
  setup_build_env
  build_caqtdm
  create_appdir
  make_appimage

  if [ "$APPDIR_ONLY" -eq 1 ]; then
    msg "AppDir ready: $APPDIR"
  else
    msg "Done. AppImage output directory: $OUTPUT_DIR"
  fi
}

main "$@"
