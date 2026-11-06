#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
root_dir="$(cd -- "$script_dir/../.." && pwd)"
output_dir="$root_dir/bin/debug"
binary="$output_dir/flux-parser"

mkdir -p "$output_dir"
"${CXX:-g++}" -std=c++17 -Wall -Wextra -Wpedantic -O0 -g \
    "$root_dir/src/parser.cpp" \
    "$root_dir/src/visualize.cpp" \
    -o "$binary"

printf 'Build succeeded: %s\n' "$binary"
