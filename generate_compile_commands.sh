#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
    echo "usage: $0 <project-dir> <dry-run-file>" >&2
    exit 1
fi

if ! command -v jq >/dev/null 2>&1; then
    echo "jq is required to generate compile_commands.json" >&2
    exit 1
fi

project_dir="$(realpath -m -- "$1")"
dry_run_file="$2"
cwd="$project_dir"
out_file="$project_dir/compile_commands.json"

declare -A seen
entries=0

split_shell_words() {
    local s=$1
    local i c
    local token=""
    local in_single=0
    local in_double=0
    local escaped=0
    local have_token=0

    args=()

    for ((i = 0; i < ${#s}; i++)); do
        c=${s:i:1}

        if (( escaped )); then
            token+="$c"
            have_token=1
            escaped=0
            continue
        fi

        if (( in_single )); then
            if [[ $c == "'" ]]; then
                in_single=0
            else
                token+="$c"
                have_token=1
            fi
            continue
        fi

        if (( in_double )); then
            case "$c" in
                '"')
                    in_double=0
                    ;;
                '\\')
                    escaped=1
                    have_token=1
                    ;;
                *)
                    token+="$c"
                    have_token=1
                    ;;
            esac
            continue
        fi

        case "$c" in
            "'")
                in_single=1
                have_token=1
                ;;
            '"')
                in_double=1
                have_token=1
                ;;
            '\\')
                escaped=1
                have_token=1
                ;;
            ' ' | $'\t')
                if (( have_token )); then
                    args+=("$token")
                    token=""
                    have_token=0
                fi
                ;;
            *)
                token+="$c"
                have_token=1
                ;;
        esac
    done

    if (( escaped || in_single || in_double )); then
        return 1
    fi

    if (( have_token )); then
        args+=("$token")
    fi
}

is_compiler() {
    case "$1" in
        cc|gcc|clang|c++|g++|clang++)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

is_source_file() {
    case "$1" in
        *.c|*.cc|*.cpp|*.cxx|*.c++|*.C)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

has_arg() {
    local wanted=$1
    shift

    local arg
    for arg in "$@"; do
        [[ $arg == "$wanted" ]] && return 0
    done

    return 1
}

json_entries_file="$(mktemp)"
trap 'rm -f "$json_entries_file"' EXIT

while IFS= read -r raw_line || [[ -n $raw_line ]]; do
    line="${raw_line#"${raw_line%%[![:space:]]*}"}"
    line="${line%"${line##*[![:space:]]}"}"

    [[ -z $line ]] && continue

    if [[ $line =~ Entering[[:space:]]directory[[:space:]]\'([^\']+)\' ]]; then
        cwd="$(realpath -m -- "${BASH_REMATCH[1]}")"
        continue
    fi

    if [[ $line =~ Leaving[[:space:]]directory[[:space:]]\'([^\']+)\' ]]; then
        cwd="$project_dir"
        continue
    fi

    if ! split_shell_words "$line"; then
        continue
    fi

    [[ ${#args[@]} -eq 0 ]] && continue

    compiler="$(basename -- "${args[0]}")"

    if ! is_compiler "$compiler"; then
        continue
    fi

    if ! has_arg "-c" "${args[@]}"; then
        continue
    fi

    source=""
    for arg in "${args[@]}"; do
        if is_source_file "$arg"; then
            source="$arg"
        fi
    done

    [[ -z $source ]] && continue

    if [[ $source = /* ]]; then
        candidate="$source"
    else
        candidate="$cwd/$source"
    fi

    if ! source_path="$(realpath -e -- "$candidate" 2>/dev/null)"; then
        continue
    fi

    if [[ -n ${seen[$source_path]+x} ]]; then
        continue
    fi

    seen["$source_path"]=1

    jq -cn \
        --arg directory "$cwd" \
        --arg command "$line" \
        --arg file "$source_path" \
        '{directory: $directory, command: $command, file: $file}' >> "$json_entries_file"

    ((entries += 1))
done < "$dry_run_file"

jq -s . "$json_entries_file" > "$out_file"

echo "generated compile_commands.json with $entries entries"
