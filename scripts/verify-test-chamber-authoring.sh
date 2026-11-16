#!/usr/bin/env bash
# Render a controller-authored project in both eyes after module recreation.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
OUT="${1:-$WORK/captures}"
mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"
cd "$ROOT"
"${CXX:-g++}" -std=c++17 -O1 -Iapp/src/main/jni/native -Iapp/src/test/native \
    -Ithird_party/openktg/src app/src/test/native/test_chamber_authoring_test.cpp \
    third_party/openktg/src/gentexture.cpp -lEGL -lGLESv2 -lz -o "$WORK/author"
EGL_PLATFORM="${EGL_PLATFORM:-surfaceless}" "$WORK/author" "$OUT"
