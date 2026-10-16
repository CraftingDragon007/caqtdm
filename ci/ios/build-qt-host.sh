#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/checks.sh"

run_with_heartbeat() {
  local label="$1"
  shift
  "$@" &
  local command_pid=$!
  (
    while kill -0 "${command_pid}" 2>/dev/null; do
      sleep 60
      if kill -0 "${command_pid}" 2>/dev/null; then
        echo "$(date -u '+%Y-%m-%dT%H:%M:%SZ') Still running: ${label} (pid ${command_pid})"
      fi
    done
  ) &
  local heartbeat_pid=$!
  local result=0
  wait "${command_pid}" || result=$?
  kill "${heartbeat_pid}" 2>/dev/null || true
  wait "${heartbeat_pid}" 2>/dev/null || true
  if (( result != 0 )); then
    echo "::error::${label} failed with exit code ${result}" >&2
  fi
  return "${result}"
}

qt_source_root="${QT_ROOT}/${QT_VERSION}/source"
qt_source="${qt_source_root}/qt-everywhere-src-${QT_VERSION}"
qt_tarball="qt-everywhere-src-${QT_VERSION}.tar.xz"
qt_minor="$(echo "${QT_VERSION}" | cut -d. -f1,2)"
qt_url="https://download.qt.io/official_releases/qt/${qt_minor}/${QT_VERSION}/single/${qt_tarball}"
qt_archive="${RUNNER_TEMP}/${qt_tarball}"
qt_host_root="${RUNNER_TEMP}/qt-build-host"

mkdir -p "${qt_source_root}" "${qt_host_root}"
echo "::group::Download Qt ${QT_VERSION} source"
curl --fail --location --show-error --retry 3 --progress-bar \
  --write-out $'\nHTTP %{http_code}; downloaded %{size_download} bytes in %{time_total}s\n' \
  "${qt_url}" -o "${qt_archive}"
require_nonempty_file "${qt_archive}"
shasum -a 256 "${qt_archive}"
echo "::endgroup::"

echo "::group::Extract Qt ${QT_VERSION} source"
tar -xf "${qt_archive}" -C "${qt_source_root}"
require_dir "${qt_source}"
echo "::endgroup::"

qt_host_submodules='qtbase,qt5compat,qtsvg,qttools,qtimageformats,qtpositioning,qtopcua'
cd "${qt_host_root}"
echo "::group::Configure Qt ${QT_VERSION} host tools"
"${qt_source}/configure" \
  -developer-build \
  -submodules "${qt_host_submodules}" \
  -release -opensource -confirm-license \
  -nomake tests -nomake examples
echo "::endgroup::"
echo "::group::Build Qt host tools"
run_with_heartbeat "Build Qt host tools" cmake --build . --target host_tools --parallel
bash "${GITHUB_WORKSPACE}/ci/ios/verify-qt-host.sh" "${qt_host_root}"
echo "::endgroup::"
