#!/usr/bin/env bash

set -e

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)
cd "$script_dir"

usage() {
  cat <<'EOF'
Usage: ./build.sh [options]

Options:
  --qmake          Run qmake before the requested make target.
  --generate-compile-commands
                   Generate compile_commands.json from a make dry run.
  --clean          Run make clean, generating Makefiles first if needed.
  --distclean      Run make distclean, generating Makefiles first if needed.
  -j, --jobs N     Number of parallel build jobs. Defaults to nproc.
  -h, --help       Show this help.

Run ./configure.sh first to create .buildenv.
EOF
}

action=build
force_qmake=0
jobs=

while [ "$#" -gt 0 ]; do
  case "$1" in
    --qmake)
      force_qmake=1
      ;;
    --generate-compile-commands)
      action=compile_commands
      ;;
    --clean)
      action=clean
      ;;
    --distclean)
      action=distclean
      ;;
    -j|--jobs)
      shift
      if [ -z "${1:-}" ]; then
        echo "Missing value for --jobs" >&2
        exit 2
      fi
      jobs=$1
      ;;
    -j*)
      jobs=${1#-j}
      if [ -z "$jobs" ]; then
        echo "Missing value for -j" >&2
        exit 2
      fi
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
  shift
done

if [ ! -f .buildenv ]; then
  echo "Missing .buildenv. Run ./configure.sh first." >&2
  exit 1
fi

# shellcheck source=/dev/null
source ./.buildenv

make_cmd=${MAKE:-make}

if [ -z "${jobs:-}" ]; then
  if command -v nproc >/dev/null 2>&1; then
    jobs=$(nproc)
  else
    jobs=$(getconf _NPROCESSORS_ONLN 2>/dev/null || printf "1")
  fi
fi

if [ -z "${QMAKE:-}" ]; then
  echo "QMAKE is not set in .buildenv. Run ./configure.sh --reconfigure." >&2
  exit 1
fi

if ! command -v "$QMAKE" >/dev/null 2>&1 && [ ! -x "$QMAKE" ]; then
  echo "Configured QMAKE was not found: $QMAKE" >&2
  echo "Run ./configure.sh --reconfigure to update .buildenv." >&2
  exit 1
fi

mkdir -p \
  "${CAQTDM_COLLECT:-$script_dir/caQtDM_Binaries}" \
  "${CAQTDM_COLLECT:-$script_dir/caQtDM_Binaries}/designer" \
  "${CAQTDM_COLLECT:-$script_dir/caQtDM_Binaries}/controlsystems"

if [ "$force_qmake" -eq 1 ] || [ ! -f Makefile ]; then
  echo "========== qmake all.pro =========="
  "$QMAKE" all.pro
fi

generate_compile_commands() {
  local dry_run_file
  dry_run_file=$(mktemp)
  trap 'rm -f "$dry_run_file"' RETURN

  if [ ! -x ./generate_compile_commands.sh ]; then
    echo "Missing executable helper: ./generate_compile_commands.sh" >&2
    exit 1
  fi

  echo "========== generate compile_commands.json =========="
  LC_ALL=C "$make_cmd" -n -B -k -j1 all >"$dry_run_file" 2>/dev/null || true
  ./generate_compile_commands.sh "$script_dir" "$dry_run_file"
}

case "$action" in
  build)
    echo "========== make -j$jobs =========="
    "$make_cmd" -j"$jobs"
    echo "========== make -C caQtDM_UnitTests check =========="
    "$make_cmd" -C caQtDM_UnitTests check
    ;;
  clean)
    echo "========== make clean =========="
    "$make_cmd" clean
    ;;
  compile_commands)
    generate_compile_commands
    ;;
  distclean)
    echo "========== make distclean =========="
    "$make_cmd" distclean
    ;;
esac
