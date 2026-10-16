#!/usr/bin/env bash
# Offline host-only classic Pong physics + existing Classic Games regression.
# No Android SDK, GitHub Actions, or headset is required for these checks.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
INC="$ROOT/app/src/main/jni/native"
TEST="$ROOT/app/src/test/native"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
ran=0
for compiler in g++ clang++; do
    if ! command -v "$compiler" >/dev/null 2>&1; then
        echo "SKIP $compiler (not installed)"
        continue
    fi
    ran=$((ran + 1))
    for name in classic_pong_dynamics classic_games; do
        echo "Testing $name with $compiler"
        "$compiler" -std=c++17 -O1 -pthread -Wall -Wextra -Wpedantic -Werror \
            -I"$INC" "$TEST/${name}_test.cpp" -o "$WORK/${name}-$compiler"
        "$WORK/${name}-$compiler"
    done
done
if [[ "$ran" -eq 0 ]]; then
    echo "No usable C++17 compiler" >&2
    exit 1
fi
