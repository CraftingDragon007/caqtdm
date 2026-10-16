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
