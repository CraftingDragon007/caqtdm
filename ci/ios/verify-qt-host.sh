#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/checks.sh"

host_root="${1:?Usage: verify-qt-host.sh HOST_BUILD_ROOT}"
host_bin="${host_root}/qtbase/bin"

for tool in qmake qopcuaxmldatatypes2cpp; do
  require_executable "${host_bin}/${tool}"
done

if [[ -z "$(find "${host_root}" -name Qt6OpcUaToolsConfig.cmake -type f -print -quit)" ]]; then
  ci_error "Missing Qt6OpcUaToolsConfig.cmake under ${host_root}"
fi

# Qt >= 6.11 cross-builds need QT_HOST_PATH_CMAKE_DIR to resolve to a real
# dir. Fail here, before any target build, if the host cmake dir is not
# derivable from the tree.
if ! host_cmake_dir="$(print_host_cmake_dir "${host_root}")"; then
  ci_error "Cannot derive host Qt cmake dir (Qt6OpcUaToolsConfig.cmake not found) under ${host_root}"
fi
require_dir "${host_cmake_dir}"
