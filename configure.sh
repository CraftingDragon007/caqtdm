#!/usr/bin/env bash

_caqtdm_env_sourced=0
if [ "${BASH_SOURCE[0]}" != "$0" ]; then
  _caqtdm_env_sourced=1
fi

_caqtdm_env_finish() {
  local status=$1
  local sourced=$_caqtdm_env_sourced
  unset -f _caqtdm_env_finish _caqtdm_env_quote _caqtdm_env_command_dir _caqtdm_env_qmake_version
  unset -f _caqtdm_env_detect_qmake _caqtdm_env_detect_qt_module _caqtdm_env_detect_qwt_lib_name _caqtdm_env_detect_epics_host_arch _caqtdm_env_detect
  unset -f _caqtdm_env_prompt_value _caqtdm_env_prompt_yes_no _caqtdm_env_configure _caqtdm_env_unset_config_vars
  unset -f _caqtdm_env_write _caqtdm_env_load _caqtdm_env_print _caqtdm_env_need_config
  unset _caqtdm_env_sourced _caqtdm_env_script_dir _caqtdm_env_file _caqtdm_env_arg _caqtdm_env_action _caqtdm_env_prefer_qt _caqtdm_env_command
  if [ "$sourced" -eq 1 ]; then
    return "$status"
  fi
  exit "$status"
}

_caqtdm_env_quote() {
  printf "%q" "$1"
}

_caqtdm_env_command_dir() {
  local command_path=$1
  command_path=$(command -v "$command_path" 2>/dev/null) || return 1
  cd "$(dirname "$command_path")/.." 2>/dev/null && pwd -P
}

_caqtdm_env_qmake_version() {
  local version
  version=$("$1" -query QT_VERSION 2>/dev/null | tr -d '\r')
  if [ -n "$version" ]; then
    printf "%s\n" "$version"
    return 0
  fi

  "$1" --version 2>/dev/null | sed -n 's/.*Using Qt version \([^ ]*\).*/\1/p' | tr -d '\r'
}

_caqtdm_env_detect_qt_module() {
  local module=$1 qt_version qt_major module_file pkg_config_name
  qt_version=$(_caqtdm_env_qmake_version "$QMAKE")
  qt_major=${qt_version%%.*}

  for module_file in \
    "$("$QMAKE" -query QT_INSTALL_ARCHDATA 2>/dev/null)/mkspecs/modules/qt_lib_${module}.pri" \
    "$("$QMAKE" -query QT_INSTALL_DATA 2>/dev/null)/mkspecs/modules/qt_lib_${module}.pri"; do
    [ -f "$module_file" ] && return 0
  done

  if command -v pkg-config >/dev/null 2>&1 && [ -n "$qt_major" ]; then
    case "$module" in
      serialbus) pkg_config_name="Qt${qt_major}SerialBus" ;;
      positioning) pkg_config_name="Qt${qt_major}Positioning" ;;
      opcua) pkg_config_name="Qt${qt_major}OpcUa" ;;
      *) pkg_config_name="Qt${qt_major}${module}" ;;
    esac
    pkg-config --exists "$pkg_config_name" 2>/dev/null && return 0
  fi

  return 1
}

_caqtdm_env_detect_qwt_lib_name() {
  local qt_version candidate file latest version
  qt_version=$(_caqtdm_env_qmake_version "$QMAKE")

  if [[ "$qt_version" == 5.* || "$_caqtdm_env_prefer_qt" == 5 ]]; then
    local candidates=(qwt-qt5 qwt5-qt5 qwt)
  else
    local candidates=(qwt qwt-qt6 qwt6-qt6)
  fi

  for candidate in "${candidates[@]}"; do
    latest=
    while IFS= read -r file; do
      latest=$file
    done < <(compgen -G "$QWTLIB/lib${candidate}.so*" 2>/dev/null | sort -V)

    if [ -n "$latest" ]; then
      QWTLIBNAME=$candidate
      version=${latest##*.so.}
      if [ "$version" != "$latest" ] && [[ "$version" =~ ^[0-9] ]]; then
        QWTVERSION=$version
      fi
      return 0
    fi

    if compgen -G "$QWTLIB/lib${candidate}.dylib*" >/dev/null || \
       compgen -G "$QWTLIB/lib${candidate}.a" >/dev/null; then
      QWTLIBNAME=$candidate
      return 0
    fi
  done

  QWTLIBNAME=${QWTLIBNAME:-qwt}
}

_caqtdm_env_detect_qmake() {
  local candidate version

  if [ "$_caqtdm_env_prefer_qt" = 5 ]; then
    local primary_major=5
    local fallback_major=6
    local primary_candidates=(qmake-qt5 qmake5 qmake)
    local fallback_candidates=(qmake6 qmake-qt6 qmake)
  else
    local primary_major=6
    local fallback_major=5
    local primary_candidates=(qmake6 qmake-qt6 qmake)
    local fallback_candidates=(qmake-qt5 qmake5 qmake)
  fi

  for candidate in "${primary_candidates[@]}"; do
    command -v "$candidate" >/dev/null 2>&1 || continue
    version=$(_caqtdm_env_qmake_version "$candidate")
    if [[ "$version" == "$primary_major".* ]]; then
      QMAKE=$(command -v "$candidate")
      QTHOME=$(_caqtdm_env_command_dir "$candidate")
      return 0
    fi
  done

  for candidate in "${fallback_candidates[@]}"; do
    command -v "$candidate" >/dev/null 2>&1 || continue
    version=$(_caqtdm_env_qmake_version "$candidate")
    if [[ "$version" != "$fallback_major".* ]]; then
      continue
    fi
    QMAKE=$(command -v "$candidate")
    QTHOME=$(_caqtdm_env_command_dir "$candidate")
    return 0
  done

  for candidate in qmake6 qmake-qt6 qmake-qt5 qmake5 qmake; do
    command -v "$candidate" >/dev/null 2>&1 || continue
    QMAKE=$(command -v "$candidate")
    QTHOME=$(_caqtdm_env_command_dir "$candidate")
    return 0
  done

  if [ "$_caqtdm_env_prefer_qt" = 5 ]; then
    QMAKE=${QMAKE:-qmake-qt5}
  else
    QMAKE=${QMAKE:-qmake6}
  fi
  QTHOME=${QTHOME:-/usr}
  return 1
}

_caqtdm_env_detect_epics_host_arch() {
  if [ -n "$EPICS_HOST_ARCH" ]; then
    return 0
  fi

  local epics_host_arch
  for epics_host_arch in "$EPICS_BASE/startup/EpicsHostArch" /usr/lib/epics/base/startup/EpicsHostArch; do
    [ -x "$epics_host_arch" ] || continue
    EPICS_HOST_ARCH=$($epics_host_arch 2>/dev/null)
    [ -n "$EPICS_HOST_ARCH" ] && return 0
  done

  EPICS_HOST_ARCH=$(uname -s 2>/dev/null | tr '[:upper:]' '[:lower:]')-$(uname -m 2>/dev/null)
}

_caqtdm_env_detect() {
  local qmake_ok=0 qt_version qt_major
  _caqtdm_env_detect_qmake || qmake_ok=1
  qt_version=$(_caqtdm_env_qmake_version "$QMAKE")
  qt_major=${qt_version%%.*}

  QWTHOME=${QWTHOME:-/usr}
  if command -v pkg-config >/dev/null 2>&1; then
    local qwt_pkg qwt_pkg_candidates
    if [ "$qt_major" = 5 ]; then
      qwt_pkg_candidates=(qwt-qt5 qwt5-qt5 qwt Qt5Qwt6)
    else
      qwt_pkg_candidates=(Qt6Qwt6 qwt-qt6 qwt6-qt6 qwt)
    fi
    for qwt_pkg in "${qwt_pkg_candidates[@]}"; do
      pkg-config --exists "$qwt_pkg" 2>/dev/null || continue
      QWTINCLUDE=$(pkg-config --variable=includedir "$qwt_pkg" 2>/dev/null)
      QWTLIB=$(pkg-config --variable=libdir "$qwt_pkg" 2>/dev/null)
      QWTVERSION=$(pkg-config --modversion "$qwt_pkg" 2>/dev/null)
      break
    done
  fi
  QWTINCLUDE=${QWTINCLUDE:-/usr/include/qwt}
  QWTLIB=${QWTLIB:-$QWTHOME/lib}
  if [ -z "$QWTLIBNAME" ] || [ -z "$QWTVERSION" ]; then
    _caqtdm_env_detect_qwt_lib_name
  fi
  QWTVERSION=${QWTVERSION:-6.1}

  if [ -z "$EPICS_BASE" ]; then
    local epics_base_candidate
    for epics_base_candidate in /usr/lib/epics/base /usr/local/epics/base /opt/epics/base; do
      if [ -d "$epics_base_candidate" ]; then
        EPICS_BASE=$epics_base_candidate
        break
      fi
    done
  fi
  EPICS_BASE=${EPICS_BASE:-/usr/lib/epics/base}
  _caqtdm_env_detect_epics_host_arch
  EPICSINCLUDE=${EPICSINCLUDE:-$EPICS_BASE/include}
  EPICSLIB=${EPICSLIB:-$EPICS_BASE/lib/$EPICS_HOST_ARCH}
  EPICS4LOCATION=${EPICS4LOCATION:-}

  QTCONTROLS_LIBS=${QTCONTROLS_LIBS:-$_caqtdm_env_script_dir/caQtDM_Binaries}
  CAQTDM_COLLECT=${CAQTDM_COLLECT:-$_caqtdm_env_script_dir/caQtDM_Binaries}
  QTBASE=${QTBASE:-$QTCONTROLS_LIBS}
  CAQTDM_CA_ARCHIVELIBS=${CAQTDM_CA_ARCHIVELIBS:-$_caqtdm_env_script_dir/caQtDM_Binaries}
  CAQTDM_LOGGING_ARCHIVELIBS=${CAQTDM_LOGGING_ARCHIVELIBS:-$_caqtdm_env_script_dir/caQtDM_Binaries}
  PYTHONVERSION=${PYTHONVERSION:-$(python3 -c 'import sys; print(f"{sys.version_info.major}.{sys.version_info.minor}")' 2>/dev/null)}
  PYTHONVERSION=${PYTHONVERSION:-3}
  PYTHONINCLUDE=${PYTHONINCLUDE:-$(python3-config --includes 2>/dev/null | awk '{print $1}' | sed 's/^-I//')}
  PYTHONINCLUDE=${PYTHONINCLUDE:-/usr/include/python$PYTHONVERSION}
  PYTHONLIB=${PYTHONLIB:-$(python3-config --prefix 2>/dev/null)/lib}
  PYTHONLIB=${PYTHONLIB:-/usr/lib}

  ZMQINC=${ZMQINC:-/usr/include}
  ZMQLIB=${ZMQLIB:-/usr/lib}

  CAQTDM_NORPATH=${CAQTDM_NORPATH:-}
  if _caqtdm_env_detect_qt_module positioning; then
    CAQTDM_GPS=1
  else
    CAQTDM_GPS=
  fi
  if _caqtdm_env_detect_qt_module serialbus; then
    CAQTDM_MODBUS=1
  else
    CAQTDM_MODBUS=
  fi
  if _caqtdm_env_detect_qt_module opcua; then
    CAQTDM_OPCUA=1
  else
    CAQTDM_OPCUA=
  fi

  if [ -n "$TROLLTECH" ]; then
    QTDM_RPATH=${QTDM_RPATH:-$QTBASE:$TROLLTECH/binQt}
  else
    QTDM_RPATH=${QTDM_RPATH:-$QTBASE}
  fi

  return "$qmake_ok"
}

_caqtdm_env_prompt_value() {
  local name=$1 description=$2 current answer
  eval "current=\${$name}"
  printf "%s [%s]: " "$description" "$current"
  read -r answer
  if [ -n "$answer" ]; then
    printf -v "$name" "%s" "$answer"
  fi
}

_caqtdm_env_prompt_yes_no() {
  local prompt=$1 default=${2:-n} answer suffix
  if [ "$default" = "y" ]; then
    suffix="Y/n"
  else
    suffix="y/N"
  fi
  printf "%s [%s]: " "$prompt" "$suffix"
  read -r answer
  answer=${answer:-$default}
  [[ "$answer" =~ ^[Yy]$|^[Yy][Ee][Ss]$ ]]
}

_caqtdm_env_configure() {
  local required_only=${1:-0}

  echo
  echo "Configure caQtDM build environment"
  echo "Press Enter to keep the value shown in brackets."
  echo

  _caqtdm_env_prompt_value QMAKE "qmake executable"
  _caqtdm_env_prompt_value QTHOME "Qt installation directory"
  _caqtdm_env_prompt_value QWTHOME "Qwt installation directory"
  _caqtdm_env_prompt_value QWTINCLUDE "Qwt include directory"
  _caqtdm_env_prompt_value QWTLIB "Qwt library directory"
  _caqtdm_env_prompt_value QWTVERSION "Qwt version"
  _caqtdm_env_prompt_value QWTLIBNAME "Qwt library name"
  _caqtdm_env_prompt_value EPICS_BASE "EPICS base directory"
  _caqtdm_env_prompt_value EPICS_HOST_ARCH "EPICS host architecture"
  EPICSINCLUDE=${EPICSINCLUDE:-$EPICS_BASE/include}
  EPICSLIB=${EPICSLIB:-$EPICS_BASE/lib/$EPICS_HOST_ARCH}
  _caqtdm_env_prompt_value EPICSINCLUDE "EPICS include directory"
  _caqtdm_env_prompt_value EPICSLIB "EPICS library directory"

  if [ "$required_only" -eq 0 ] || _caqtdm_env_prompt_yes_no "Configure optional paths and feature flags?" n; then
    _caqtdm_env_prompt_value EPICS4LOCATION "EPICS 4 location, optional"
    _caqtdm_env_prompt_value QTCONTROLS_LIBS "Local caQtDM library directory"
    _caqtdm_env_prompt_value CAQTDM_COLLECT "Build output directory"
    QTBASE=${QTBASE:-$QTCONTROLS_LIBS}
    _caqtdm_env_prompt_value QTBASE "QtControls build base directory"
    _caqtdm_env_prompt_value QTDM_RPATH "Runtime library search path"
    _caqtdm_env_prompt_value CAQTDM_CA_ARCHIVELIBS "Channel access archive library directory"
    _caqtdm_env_prompt_value CAQTDM_LOGGING_ARCHIVELIBS "Logging archive library directory"
    _caqtdm_env_prompt_value PYTHONVERSION "Python version"
    _caqtdm_env_prompt_value PYTHONINCLUDE "Python include directory"
    _caqtdm_env_prompt_value PYTHONLIB "Python library directory"
    _caqtdm_env_prompt_value ZMQINC "ZeroMQ include directory"
    _caqtdm_env_prompt_value ZMQLIB "ZeroMQ library directory"
    _caqtdm_env_prompt_value CAQTDM_NORPATH "Disable RPATH, 1=yes empty=no"
    _caqtdm_env_prompt_value CAQTDM_GPS "Build GPS support, 1=yes empty=no"
    _caqtdm_env_prompt_value CAQTDM_MODBUS "Build Modbus support, 1=yes empty=no"
    _caqtdm_env_prompt_value CAQTDM_OPCUA "Build OPC UA support, 1=yes empty=no"
  fi
}

_caqtdm_env_unset_config_vars() {
  unset \
    QMAKE QTHOME QWTHOME QWTINCLUDE QWTLIB QWTVERSION QWTLIBNAME \
    EPICS_BASE EPICS_HOST_ARCH EPICSINCLUDE EPICSLIB EPICS4LOCATION \
    QTCONTROLS_LIBS CAQTDM_COLLECT QTBASE QTDM_RPATH \
    CAQTDM_CA_ARCHIVELIBS CAQTDM_LOGGING_ARCHIVELIBS \
    PYTHONVERSION PYTHONINCLUDE PYTHONLIB ZMQINC ZMQLIB \
    CAQTDM_NORPATH CAQTDM_GPS CAQTDM_MODBUS CAQTDM_OPCUA
}

_caqtdm_env_write() {
  {
    echo "# caQtDM local build configuration"
    echo "# Generated by ./configure.sh. Run '$_caqtdm_env_command --reconfigure' to change it."
    local name value
    for name in \
      QMAKE QTHOME QWTHOME QWTINCLUDE QWTLIB QWTVERSION QWTLIBNAME \
      EPICS_BASE EPICS_HOST_ARCH EPICSINCLUDE EPICSLIB EPICS4LOCATION \
      QTCONTROLS_LIBS CAQTDM_COLLECT QTBASE QTDM_RPATH \
      CAQTDM_CA_ARCHIVELIBS CAQTDM_LOGGING_ARCHIVELIBS \
      PYTHONVERSION PYTHONINCLUDE PYTHONLIB ZMQINC ZMQLIB \
      CAQTDM_NORPATH CAQTDM_GPS CAQTDM_MODBUS CAQTDM_OPCUA; do
      eval "value=\${$name-}"
      case "$name:$value" in
        CAQTDM_NORPATH:0|CAQTDM_GPS:0|CAQTDM_MODBUS:0|CAQTDM_OPCUA:0) value= ;;
      esac
      printf "export %s=%s\n" "$name" "$(_caqtdm_env_quote "$value")"
    done
  } > "$_caqtdm_env_file"
}

_caqtdm_env_load() {
  # shellcheck source=/dev/null
  source "$_caqtdm_env_file"
}

_caqtdm_env_print() {
  echo "============================================================================================="
  echo "caQtDM build environment settings"
  echo "Configuration file: $_caqtdm_env_file"
  echo
  echo "Build:"
  printf "  %-28s %s\n" QMAKE "$QMAKE"
  printf "  %-28s %s\n" QTHOME "$QTHOME"
  printf "  %-28s %s\n" QWTHOME "$QWTHOME"
  printf "  %-28s %s\n" QWTINCLUDE "$QWTINCLUDE"
  printf "  %-28s %s\n" QWTLIB "$QWTLIB"
  printf "  %-28s %s\n" QWTLIBNAME "$QWTLIBNAME"
  printf "  %-28s %s\n" EPICS_BASE "$EPICS_BASE"
  printf "  %-28s %s\n" EPICS_HOST_ARCH "$EPICS_HOST_ARCH"
  printf "  %-28s %s\n" EPICSINCLUDE "$EPICSINCLUDE"
  printf "  %-28s %s\n" EPICSLIB "$EPICSLIB"
  printf "  %-28s %s\n" QTBASE "$QTBASE"
  printf "  %-28s %s\n" QTDM_RPATH "$QTDM_RPATH"
  echo
  echo "Optional libraries:"
  printf "  %-28s %s\n" CAQTDM_CA_ARCHIVELIBS "$CAQTDM_CA_ARCHIVELIBS"
  printf "  %-28s %s\n" CAQTDM_LOGGING_ARCHIVELIBS "$CAQTDM_LOGGING_ARCHIVELIBS"
  printf "  %-28s %s\n" PYTHONINCLUDE "$PYTHONINCLUDE"
  printf "  %-28s %s\n" PYTHONLIB "$PYTHONLIB"
  printf "  %-28s %s\n" ZMQINC "$ZMQINC"
  printf "  %-28s %s\n" ZMQLIB "$ZMQLIB"
  echo
  echo "Feature flags:"
  printf "  %-28s %s\n" CAQTDM_GPS "${CAQTDM_GPS:-disabled}"
  printf "  %-28s %s\n" CAQTDM_MODBUS "${CAQTDM_MODBUS:-disabled}"
  printf "  %-28s %s\n" CAQTDM_OPCUA "${CAQTDM_OPCUA:-disabled}"
  printf "  %-28s %s\n" CAQTDM_NORPATH "${CAQTDM_NORPATH:-disabled}"
  echo
  echo "Run '$_caqtdm_env_command --reconfigure' to change these settings."
  echo "Run './build.sh' to build caQtDM with this configuration."
  echo "============================================================================================="
}

_caqtdm_env_need_config() {
  [ -x "$QMAKE" ] || [ -x "$(command -v "$QMAKE" 2>/dev/null)" ] || return 0
  [ -d "$QTHOME" ] || return 0
  [ -d "$QWTINCLUDE" ] || return 0
  [ -d "$QWTLIB" ] || return 0
  [ -d "$EPICS_BASE" ] || return 0
  [ -d "$EPICSINCLUDE" ] || return 0
  [ -d "$EPICSLIB" ] || return 0
  return 1
}

_caqtdm_env_script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)
_caqtdm_env_file="$_caqtdm_env_script_dir/.buildenv"
_caqtdm_env_action=load
_caqtdm_env_prefer_qt=6
_caqtdm_env_command="./configure.sh"
if [ "$_caqtdm_env_sourced" -eq 1 ]; then
  _caqtdm_env_command="source ./configure.sh"
fi

for _caqtdm_env_arg in "$@"; do
  case "$_caqtdm_env_arg" in
    --help|-h)
      echo "Usage: $_caqtdm_env_command [--reconfigure|--redetect] [--prefer-qt5]"
      echo "Loads .buildenv, creating it automatically on first use."
      echo "By default Qt 6 is detected first, falling back to Qt 5 if Qt 6 is unavailable."
      echo "Use --reconfigure to edit the current .buildenv values."
      echo "Use --redetect to ignore .buildenv and detect local settings again."
      echo "Use --prefer-qt5 to detect qmake-qt5/qmake5 and Qt 5 Qwt libraries first."
      _caqtdm_env_finish 0
      return 0 2>/dev/null || exit 0
      ;;
    --reconfigure)
      _caqtdm_env_action=reconfigure
      ;;
    --redetect)
      _caqtdm_env_action=redetect
      ;;
    --prefer-qt5)
      _caqtdm_env_prefer_qt=5
      ;;
    *)
      echo "Unknown option: $_caqtdm_env_arg" >&2
      echo "Run '$_caqtdm_env_command --help' for usage." >&2
      _caqtdm_env_finish 2
      return 2 2>/dev/null || exit 2
      ;;
  esac
done

case "$_caqtdm_env_action" in
  reconfigure)
    [ -f "$_caqtdm_env_file" ] && _caqtdm_env_load
    [ -f "$_caqtdm_env_file" ] || _caqtdm_env_detect
    _caqtdm_env_configure 0
    _caqtdm_env_write
    _caqtdm_env_load
    _caqtdm_env_print
    _caqtdm_env_finish 0
    return 0 2>/dev/null || exit 0
    ;;
  redetect)
    _caqtdm_env_unset_config_vars
    _caqtdm_env_detect
    _caqtdm_env_print
    if ! _caqtdm_env_prompt_yes_no "Use these redetected settings?" y; then
      _caqtdm_env_configure 0
    fi
    _caqtdm_env_write
    _caqtdm_env_load
    _caqtdm_env_print
    _caqtdm_env_finish 0
    return 0 2>/dev/null || exit 0
    ;;
esac

if [ ! -f "$_caqtdm_env_file" ]; then
  if [ "$_caqtdm_env_prefer_qt" = 5 ]; then
    echo "No .buildenv found. Detecting local build settings, preferring Qt 5."
  else
    echo "No .buildenv found. Detecting local build settings, preferring Qt 6."
  fi
  if _caqtdm_env_detect && ! _caqtdm_env_need_config; then
    _caqtdm_env_print
    if _caqtdm_env_prompt_yes_no "Use these detected settings?" y; then
      _caqtdm_env_write
    else
      _caqtdm_env_configure 0
      _caqtdm_env_write
    fi
  else
    echo "Automatic configuration needs input. Please review the detected values."
    _caqtdm_env_configure 1
    _caqtdm_env_write
  fi
fi

_caqtdm_env_load
_caqtdm_env_print
_caqtdm_env_finish 0
