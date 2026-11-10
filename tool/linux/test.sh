#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
root_dir="$(cd -- "$script_dir/../.." && pwd)"
test_list="$root_dir/tool/testlist.txt"
binary="$root_dir/bin/debug/flux-parser"

if [[ ! -f "$test_list" ]]; then
    printf 'Test list not found: %s\n' "$test_list" >&2
    exit 1
fi

"$script_dir/build.sh"

ast_dir="$root_dir/log/ast"
mkdir -p "$ast_dir"

passed=0
failed=0
while IFS= read -r entry || [[ -n "$entry" ]]; do
    test_case="$(printf '%s' "$entry" | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//')"
    case "$test_case" in
        ''|\#*) continue ;;
    esac

    if [[ "$test_case" = /* ]]; then
        source_file="$test_case"
    elif [[ -f "$root_dir/$test_case" ]]; then
        source_file="$root_dir/$test_case"
    else
        source_file="$root_dir/testfile/$test_case"
    fi

    if [[ ! -f "$source_file" ]]; then
        printf 'FAIL: %s (source file not found: %s)\n' "$test_case" "$source_file" >&2
        failed=$((failed + 1))
        continue
    fi

    printf 'Running: %s\n' "$test_case"
    ast_file="$ast_dir/$(basename -- "$source_file").ast"
    if "$binary" "$source_file" "$ast_file"; then
        printf 'AST saved: %s\n' "$ast_file"
        printf 'PASS: %s\n' "$test_case"
        passed=$((passed + 1))
    else
        status=$?
        printf 'FAIL: %s (exit status %d)\n' "$test_case" "$status" >&2
        failed=$((failed + 1))
    fi
done < "$test_list"

printf '\nTest summary: %d passed, %d failed\n' "$passed" "$failed"
if (( failed > 0 )); then
    exit 1
fi
if (( passed == 0 )); then
    printf 'No test cases found in %s\n' "$test_list" >&2
    exit 1
fi
