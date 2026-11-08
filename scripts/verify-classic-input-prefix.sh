#!/usr/bin/env bash
# Memory-safety probe of the actual shared Classic module Update.
# Needs a C++17 compiler with ASan/UBSan, Khronos GLES2 headers, GLESv2 and zlib.
# No Android SDK, GL context, headset, workflow or network is used.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
INC="$ROOT/app/src/main/jni/native"
PROBE="$ROOT/scripts/audit/probes/classic_input_prefix_probe.cpp"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
CXX="${CXX:-g++}"
headers=()
if [[ -n "${CLASSIC_TEST_GL_INCLUDE:-}" ]]; then
    headers=(-I"$CLASSIC_TEST_GL_INCLUDE")
fi
"$CXX" -std=c++17 -O1 -g -pthread -fsanitize=address,undefined \
    -fno-omit-frame-pointer -ffunction-sections -fdata-sections \
    -Wl,--gc-sections -DCLASSIC_GAME_KIND=1 -I"$INC" "${headers[@]}" \
    "$PROBE" -lGLESv2 -lz -o "$WORK/input-prefix"
# This probe checks bounds/undefined behavior. LeakSanitizer is optional;
# tracing-based execution environments cannot run it. Override if supported.
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" \
UBSAN_OPTIONS="${UBSAN_OPTIONS:-halt_on_error=1:print_stacktrace=1}" \
    "$WORK/input-prefix"
