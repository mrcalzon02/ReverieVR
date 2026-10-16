#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
SOURCE="$ROOT/app/src/test/native/gumball_armada_test.cpp"
OBJECTIVES="$ROOT/app/src/test/native/gumball_arcade_objectives_test.cpp"
INCLUDE="$ROOT/app/src/main/jni/native"
for compiler in g++ clang++; do
    if ! command -v "$compiler" >/dev/null 2>&1; then
        echo "SKIP $compiler (not installed)"
        continue
    fi
    echo "Testing Gumball Armada with $compiler"
    "$compiler" -std=c++17 -O1 -Wall -Wextra -Werror -pedantic \
        -I"$INCLUDE" "$SOURCE" -o "$WORK/gumball-$compiler"
    "$WORK/gumball-$compiler"
    "$compiler" -std=c++17 -O1 -Wall -Wextra -Werror -pedantic \
        -I"$INCLUDE" "$OBJECTIVES" -o "$WORK/objectives-$compiler"
    "$WORK/objectives-$compiler"
done
if command -v clang++ >/dev/null 2>&1; then
    clang++ -std=c++17 -O1 -g -Wall -Wextra -Werror -pedantic \
        -fsanitize=address,undefined -fno-omit-frame-pointer \
        -I"$INCLUDE" "$SOURCE" -o "$WORK/gumball-sanitized"
    "$WORK/gumball-sanitized"
    clang++ -std=c++17 -O1 -g -Wall -Wextra -Werror -pedantic \
        -fsanitize=address,undefined -fno-omit-frame-pointer \
        -I"$INCLUDE" "$OBJECTIVES" -o "$WORK/objectives-sanitized"
    "$WORK/objectives-sanitized"
fi
