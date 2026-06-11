#!/usr/bin/env bash
set -Eeuo pipefail

APPDIR="${1:-squashfs-root}"

if [ ! -d "$APPDIR" ]; then
  printf 'error: AppDir not found: %s\n' "$APPDIR" >&2
  printf 'usage: %s [APPDIR]\n' "$(basename "$0")" >&2
  exit 1
fi

find "$APPDIR" -type f \( -perm -111 -o -name '*.so*' \) -print0 |
while IFS= read -r -d '' file; do
  versions="$(objdump -T "$file" 2>/dev/null | grep -oE 'GLIBC_[0-9.]+|GLIBCXX_[0-9.]+|CXXABI_[0-9.]+' | sort -Vu | tail -3 || true)"
  [ -n "$versions" ] || continue
  printf '== %s ==\n%s\n' "$file" "$versions"
done
