#!/usr/bin/env bash
# Offline production codec/storage and actual module lifecycle regressions.
# Module tests use real headers, with unused GL sections linked out. This
# does not replace Android lifecycle, real storage, controller or headset QA.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
INC="$ROOT/app/src/main/jni/native"
TEST="$ROOT/app/src/test/native"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
CXX="${CXX:-g++}"
"$CXX" -std=c++17 -O1 -pthread -Wall -Wextra -Wpedantic -Werror \
    -I"$INC" "$TEST/classic_games_save_test.cpp" -o "$WORK/codec"
"$WORK/codec"
# Optional path to the standard Khronos GLES2 headers for host-only systems.
headers=()
if [[ -n "${CLASSIC_TEST_GL_INCLUDE:-}" ]]; then
    headers=(-I"$CLASSIC_TEST_GL_INCLUDE")
fi
for kind in {0..7}; do
    "$CXX" -std=c++17 -O1 -pthread -ffunction-sections -fdata-sections \
        -Wl,--gc-sections -I"$INC" "${headers[@]}" -DCLASSIC_GAME_KIND="$kind" \
        "$TEST/classic_games_module_save_test.cpp" -o "$WORK/module-$kind"
    "$WORK/module-$kind"
done
