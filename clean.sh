#!/usr/bin/env bash

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)
exec "$script_dir/build.sh" --distclean "$@"
