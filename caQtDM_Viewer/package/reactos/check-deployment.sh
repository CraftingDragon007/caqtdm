#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

for required in caQtDM.exe caQtDM_Lib.dll qtcontrols.dll qwt.dll \
    Com.dll ca.dll Qt5OpenGL.dll platforms/qwindows.dll \
    controlsystems/epics3_plugin.dll controlsystems/epics4_plugin.dll; do
    if [[ ! -f "$CAQTDM_COLLECT/$required" ]]; then
        printf 'ERROR: missing deployment file: %s\n' "$required" >&2
        exit 1
    fi
done

count=0
while IFS= read -r file; do
    imports="$(objdump -p "$file")"
    if grep -Ei 'DLL Name: Qt5[^[:space:]]*d\.dll|[[:space:]]_(putenv_s|mkgmtime32)([[:space:]]|$)' <<<"$imports"; then
        printf 'ERROR: incompatible import in %s\n' "$file" >&2
        exit 1
    fi
    if ! grep -q 'file format pei-i386' <<<"$imports"; then
        printf 'ERROR: not a 32-bit x86 PE binary: %s\n' "$file" >&2
        exit 1
    fi
    while IFS= read -r dll; do
        if [[ ! -f "$CAQTDM_COLLECT/$dll" ]]; then
            printf 'ERROR: missing %s required by %s\n' "$dll" "$file" >&2
            exit 1
        fi
    done < <(sed -n 's/.*DLL Name: \(Qt5[^[:space:]]*\.dll\).*/\1/p' <<<"$imports")
    count=$((count + 1))
done < <(find "$CAQTDM_COLLECT" -type f \( -iname '*.dll' -o -iname '*.exe' \) -print)

printf 'Checked %s binaries: x86, no debug Qt or unsupported environment/time imports, all Qt DLLs present.\n' "$count"
