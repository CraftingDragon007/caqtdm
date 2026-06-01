#!/usr/bin/env bash
# docker-build.sh - build a portable caQtDM AppImage inside Rocky Linux 8.
#
# The resulting AppImage targets glibc >= 2.28 (RHEL/Rocky/AlmaLinux 8 and
# newer). The Docker image builds and caches Qt from source.
#
# Usage:
#   caQtDM_Viewer/package/appimage/docker-build.sh [create-appimage.sh flags]
#
# Examples:
#   caQtDM_Viewer/package/appimage/docker-build.sh
#   caQtDM_Viewer/package/appimage/docker-build.sh --branch Development
#   caQtDM_Viewer/package/appimage/docker-build.sh --no-checkout --without-bsread
#
# Environment overrides:
#   DOCKER_IMAGE      Image tag to build/use (default includes Qt version)
#   DOCKER_NO_BUILD   Set to 1 to skip rebuilding the image
#   DOCKER_NETWORK    Docker network mode (default: host)
#   OUTPUT_DIR        Where to write the final AppImage (default: script dir)
#   QT_VERSION        Qt version built into the image (default: 6.11.1)
#   QT_BUILD_JOBS     Parallel jobs for the Qt source build (default: auto)
#   OPENSSL_VERSION   OpenSSL version built into the image (default: 3.5.1)
#   PATCHELF_VERSION  patchelf version built into the image (default: 0.18.0)
#   QWT_VERSION       Qwt version built into the image (default: 6.3.0)
#   QWT_REF           Qwt git tag or branch (default: v$QWT_VERSION)
#   QWT_REPOSITORY    Qwt git repository URL (default: official SourceForge git)
#   EPICS_VERSION_TAG EPICS Base git tag built into the image (default: R7.0.10)
#   GCC_TOOLSET       RHEL/Rocky gcc-toolset version (default: 15)

set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "$SCRIPT_DIR/../../.." && pwd)"

QT_VERSION="${QT_VERSION:-6.11.1}"
QT_BUILD_JOBS="${QT_BUILD_JOBS:-auto}"
OPENSSL_VERSION="${OPENSSL_VERSION:-3.5.1}"
PATCHELF_VERSION="${PATCHELF_VERSION:-0.18.0}"
QWT_VERSION="${QWT_VERSION:-6.3.0}"
QWT_REF="${QWT_REF:-v${QWT_VERSION}}"
QWT_REPOSITORY="${QWT_REPOSITORY:-https://git.code.sf.net/p/qwt/git}"
EPICS_VERSION_TAG="${EPICS_VERSION_TAG:-R7.0.10}"
GCC_TOOLSET="${GCC_TOOLSET:-15}"

DOCKER_IMAGE="${DOCKER_IMAGE:-caqtdm-appimage-builder:rhel8-qt${QT_VERSION}}"
DOCKER_NO_BUILD="${DOCKER_NO_BUILD:-${DOCKER_NO_PULL:-0}}"
DOCKER_NETWORK="${DOCKER_NETWORK:-host}"
OUTPUT_DIR="${OUTPUT_DIR:-$SCRIPT_DIR}"

msg() {
  printf '\n==> %s\n' "$*" >&2
}

die() {
  printf 'error: %s\n' "$*" >&2
  exit 1
}

usage() {
  cat <<EOF
Usage: $(basename "$0") [OPTION...]

Build a portable caQtDM AppImage inside the Rocky Linux 8 Docker environment.
Options are forwarded to create-appimage.sh after the Docker wrapper handles
its own --help option.

Common options passed through to create-appimage.sh:
  --no-checkout        Build from the mounted working tree
  --branch REF         Git branch or tag to clone inside the container
  --repo URL           Git repository to clone inside the container
  --source DIR         Build from an existing source tree inside the container
  --skip-build         Reuse binaries from BINARY_DIR
  --appdir-only        Stop after creating the AppDir
  --without-bsread     Exclude the bsread controlsystem plugin
  --no-download-tools  Require linuxdeploy tools to be available locally
  --help, -h           Show this help and exit

Environment overrides:
  DOCKER_IMAGE      Image tag to build/use (default: $DOCKER_IMAGE)
  DOCKER_NO_BUILD   Set to 1 to skip rebuilding the image
  DOCKER_NETWORK    Docker network mode (default: $DOCKER_NETWORK)
  OUTPUT_DIR        Where to write the final AppImage (default: $OUTPUT_DIR)
  QT_VERSION        Qt version built into the image (default: $QT_VERSION)
  QT_BUILD_JOBS     Parallel jobs for the Qt source build (default: $QT_BUILD_JOBS)
  OPENSSL_VERSION   OpenSSL version built into the image (default: $OPENSSL_VERSION)
  PATCHELF_VERSION  patchelf version built into the image (default: $PATCHELF_VERSION)
  QWT_VERSION       Qwt version built into the image (default: $QWT_VERSION)
  QWT_REF           Qwt git tag or branch (default: $QWT_REF)
  QWT_REPOSITORY    Qwt git repository URL (default: $QWT_REPOSITORY)
  EPICS_VERSION_TAG EPICS Base git tag built into the image (default: $EPICS_VERSION_TAG)
  GCC_TOOLSET       RHEL/Rocky gcc-toolset version (default: $GCC_TOOLSET)

Examples:
  $(basename "$0")
  $(basename "$0") --branch Development
  $(basename "$0") --no-checkout --without-bsread
EOF
}

parse_args() {
  local arg

  for arg in "$@"; do
    case "$arg" in
      --help|-h)
        usage
        exit 0
        ;;
    esac
  done
}

require_docker() {
  command -v docker >/dev/null 2>&1 || die "docker is required but was not found"
}

build_image() {
  [ "$DOCKER_NO_BUILD" = "1" ] && return 0

  msg "Building Docker image $DOCKER_IMAGE (Rocky Linux 8 baseline)"
  docker build \
    --network "$DOCKER_NETWORK" \
    --build-arg "QT_VERSION=$QT_VERSION" \
    --build-arg "QT_BUILD_JOBS=$QT_BUILD_JOBS" \
    --build-arg "OPENSSL_VERSION=$OPENSSL_VERSION" \
    --build-arg "PATCHELF_VERSION=$PATCHELF_VERSION" \
    --build-arg "QWT_VERSION=$QWT_VERSION" \
    --build-arg "QWT_REF=$QWT_REF" \
    --build-arg "QWT_REPOSITORY=$QWT_REPOSITORY" \
    --build-arg "EPICS_VERSION_TAG=$EPICS_VERSION_TAG" \
    --build-arg "GCC_TOOLSET=$GCC_TOOLSET" \
    --tag "$DOCKER_IMAGE" \
    "$SCRIPT_DIR"
}

resolve_output_dir() {
  mkdir -p "$OUTPUT_DIR"
  OUTPUT_DIR="$(cd -- "$OUTPUT_DIR" && pwd)"
}

build_docker_run_args() {
  docker_run_args=(
    --rm
    --interactive
    --network "$DOCKER_NETWORK"
    --volume "$REPO_ROOT:/src"
    --volume "$OUTPUT_DIR:/output"
    --env "APPIMAGE_EXTRACT_AND_RUN=1"
    --env "OUTPUT_DIR=/output"
    --env "BUILD_DIR=/src/caQtDM_Viewer/package/appimage/build-docker"
    --env "TOOLS_DIR=/src/caQtDM_Viewer/package/appimage/tools"
  )

  [ ! -t 1 ] || docker_run_args+=(--tty)

  if [ -e /dev/fuse ]; then
    docker_run_args+=(--device /dev/fuse --cap-add SYS_ADMIN --security-opt apparmor:unconfined)
  fi
}

run_container() {
  build_docker_run_args
  msg "Running AppImage build inside $DOCKER_IMAGE"
  docker run "${docker_run_args[@]}" "$DOCKER_IMAGE" "$@"
}

print_output_summary() {
  msg "AppImage written to: $OUTPUT_DIR"
  ls -lh "$OUTPUT_DIR"/*.AppImage 2>/dev/null || true
}

main() {
  parse_args "$@"
  require_docker
  build_image
  resolve_output_dir
  run_container "$@"
  print_output_summary
}

main "$@"
