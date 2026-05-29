#!/usr/bin/env bash
# docker-build.sh — build a portable caQtDM AppImage inside Rocky Linux 8.
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
#   OPENSSL_VERSION   OpenSSL version built into the image (default: 3.5.1)
#   QWT_VERSION       Qwt version built into the image (default: 6.3.0)
#   QWT_REF           Qwt git tag or branch (default: v$QWT_VERSION)
#   QWT_REPOSITORY    Qwt git repository URL (default: official SourceForge git)
#   EPICS_VERSION_TAG EPICS Base git tag built into the image (default: R7.0.10)
#   GCC_TOOLSET       RHEL/Rocky gcc-toolset version (default: 10)

set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "$SCRIPT_DIR/../../.." && pwd)"

QT_VERSION="${QT_VERSION:-6.11.1}"
OPENSSL_VERSION="${OPENSSL_VERSION:-3.5.1}"
QWT_VERSION="${QWT_VERSION:-6.3.0}"
QWT_REF="${QWT_REF:-v${QWT_VERSION}}"
QWT_REPOSITORY="${QWT_REPOSITORY:-https://git.code.sf.net/p/qwt/git}"
EPICS_VERSION_TAG="${EPICS_VERSION_TAG:-R7.0.10}"
GCC_TOOLSET="${GCC_TOOLSET:-10}"

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

command -v docker >/dev/null 2>&1 || die "docker is required but was not found"

# ── Build the Docker image unless suppressed ──────────────────────────────────
if [ "$DOCKER_NO_BUILD" != "1" ]; then
  msg "Building Docker image $DOCKER_IMAGE (Rocky Linux 8 baseline)"
  docker build \
    --network "$DOCKER_NETWORK" \
    --build-arg "QT_VERSION=$QT_VERSION" \
    --build-arg "OPENSSL_VERSION=$OPENSSL_VERSION" \
    --build-arg "QWT_VERSION=$QWT_VERSION" \
    --build-arg "QWT_REF=$QWT_REF" \
    --build-arg "QWT_REPOSITORY=$QWT_REPOSITORY" \
    --build-arg "EPICS_VERSION_TAG=$EPICS_VERSION_TAG" \
    --build-arg "GCC_TOOLSET=$GCC_TOOLSET" \
    --tag "$DOCKER_IMAGE" \
    "$SCRIPT_DIR"
fi

# ── Resolve the absolute OUTPUT_DIR ──────────────────────────────────────────
mkdir -p "$OUTPUT_DIR"
OUTPUT_DIR="$(cd -- "$OUTPUT_DIR" && pwd)"

# ── Run the build inside the container ───────────────────────────────────────
#
# Mounts:
#   /src         → repository root (read-write; build artefacts go here)
#   /output      → OUTPUT_DIR (where the final .AppImage is written)
#
# The create-appimage.sh ENTRYPOINT receives all extra arguments passed to
# this script. By default it performs a fresh checkout in the ignored work/
# directory, avoiding stale object files from previous host builds. Pass
# --no-checkout only when intentionally building the mounted working tree.
#
# FUSE is required to run AppImages inside the container (appimagetool uses
# APPIMAGE_EXTRACT_AND_RUN=1 to work around that, and we set it here too).

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

if [ -t 1 ]; then
  docker_run_args+=(--tty)
fi

if [ -e /dev/fuse ]; then
  docker_run_args+=(--device /dev/fuse --cap-add SYS_ADMIN --security-opt apparmor:unconfined)
fi

msg "Running AppImage build inside $DOCKER_IMAGE"
docker run "${docker_run_args[@]}" "$DOCKER_IMAGE" "$@"

msg "AppImage written to: $OUTPUT_DIR"
ls -lh "$OUTPUT_DIR"/*.AppImage 2>/dev/null || true
