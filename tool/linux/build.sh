#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
root_dir="$(cd -- "$script_dir/../.." && pwd)"
output_dir="$root_dir/bin/debug"
binary="$output_dir/flux-parser"
build_dir="$root_dir/build/debug"

if ! command -v cmake >/dev/null 2>&1; then
    printf 'CMake is required to build Flux.\n' >&2
    exit 1
fi
if ! command -v ninja >/dev/null 2>&1; then
    printf 'Ninja is required to build Flux.\n' >&2
    exit 1
fi

cmake -S "$root_dir" -B "$build_dir" -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_COMPILER="${CXX:-g++}"
cmake --build "$build_dir"

printf 'Build succeeded: %s\n' "$binary"
