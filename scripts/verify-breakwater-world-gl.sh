#!/usr/bin/env bash
# Real game/module renderer; requires development EGL + GLES2 headers/libraries.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
cd "$ROOT"
# Override only for driver/toolchain environments lacking GLES linker aliases.
read -r -a GL_LIBS <<< "${BREAKWATER_GL_LIBS:--lEGL -lGLESv2}"
"${CXX:-g++}" -std=c++17 -O1 -Wall -Wextra -Wpedantic -Werror \
    -Iapp/src/main/jni/native app/src/test/native/breakwater_world_egl_test.cpp \
    app/src/main/jni/native/breakwater_campaign.cpp "${GL_LIBS[@]}" -lz -o "$WORK/world"
CAPTURES="${BREAKWATER_CAPTURE_DIR:-$WORK}"
mkdir -p "$CAPTURES"
EGL_PLATFORM="${EGL_PLATFORM:-surfaceless}" LIBGL_ALWAYS_SOFTWARE="${LIBGL_ALWAYS_SOFTWARE:-1}" \
    "$WORK/world" "$CAPTURES"
