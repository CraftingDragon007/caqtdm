#!/usr/bin/env bash

ci_error() {
  echo "::error::$*" >&2
  return 1
}

require_file() {
  [[ -f "$1" ]] || ci_error "Missing file: $1"
}

require_nonempty_file() {
  [[ -s "$1" ]] || ci_error "Missing or empty file: $1"
}

require_dir() {
  [[ -d "$1" ]] || ci_error "Missing directory: $1"
}

require_executable() {
  [[ -x "$1" ]] || ci_error "Missing or nonexecutable file: $1"
}

require_equal() {
  [[ "$1" == "$2" ]] || ci_error "$3: expected '$2', got '$1'"
}

require_nonempty() {
  [[ -n "$1" ]] || ci_error "$2 is empty"
}

require_arch() {
  local file="$1" arch="$2" arches
  arches="$(lipo -archs "$file")" || ci_error "Cannot read architectures in $file"
  [[ " ${arches} " == *" ${arch} "* ]] || ci_error "$file: expected architecture $arch, found ${arches}"
}

# Print the CMake package dir ("cmake" directory holding the Qt6* per-module
# config subdirs) of a Qt build tree, located from Qt6OpcUaToolsConfig.cmake.
# Returns non-zero if not found. Needed because Qt >= 6.11 cross-configure no
# longer auto-derives QT_HOST_PATH_CMAKE_DIR from a developer build tree.
print_host_cmake_dir() {
  local root="${1:?Usage: print_host_cmake_dir HOST_ROOT}" pkg
  pkg="$(find "${root}" -name Qt6OpcUaToolsConfig.cmake -type f -print -quit)"
  [[ -n "${pkg}" ]] || return 1
  (cd "$(dirname "$(dirname "${pkg}")")" && pwd)
}

