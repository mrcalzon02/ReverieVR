#!/usr/bin/env bash
# Execute actual module input and real bounded host queues; no audio device needed.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
cd "$ROOT"
# Optional toolchain locations for hosts without development linker aliases.
read -r -a GL_CFLAGS <<< "${BREAKWATER_GL_CFLAGS:-}"
read -r -a GL_LIBS <<< "${BREAKWATER_GL_LIBS:--lGLESv2}"
read -r -a TEST_FLAGS <<< "${BREAKWATER_AUDIO_TEST_FLAGS:-}"
"${CXX:-g++}" -std=c++17 -O1 -Wall -Wextra -Wpedantic -Werror \
    "${GL_CFLAGS[@]}" "${TEST_FLAGS[@]}" -Iapp/src/main/jni/native \
    app/src/test/native/breakwater_audio_test.cpp \
    app/src/main/jni/native/breakwater_campaign.cpp "${GL_LIBS[@]}" -lz \
    -o "$WORK/audio"
"$WORK/audio"
