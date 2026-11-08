#!/usr/bin/env bash
# Actual Red Ledger/Gumball module save faults and real GLES stereo captures.
# Local diagnostic runner; requires a C++17 compiler, EGL/GLES2 and zlib.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
OUT="${1:-$WORK/captures}"
mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"
CXX="${CXX:-g++}"
INC="$ROOT/app/src/main/jni/native"
TEST="$ROOT/app/src/test/native"
# Shared diagnostic snprintf bounds can warn under optimized GCC. This runner
# retains warnings; the separate per-title codec/simulation runners use Werror.
FLAGS=(-std=c++17 -O1 -Wall -Wextra -Wpedantic -I"$INC" -I"$TEST")
"$CXX" "${FLAGS[@]}" "$TEST/red_ledger_module_save_test.cpp" \
    "$INC/red_ledger_simulation.cpp" "$INC/procedural_material_atlas.cpp" \
    "$INC/red_ledger_static_geometry.cpp" -lEGL -lGLESv2 -lz -o "$WORK/red"
"$CXX" "${FLAGS[@]}" "$TEST/gumball_module_save_test.cpp" \
    -lEGL -lGLESv2 -lz -o "$WORK/gumball"
cd "$ROOT"
"$WORK/red" "$OUT"
"$WORK/gumball" "$OUT"
echo "PASS actual featured persistence; stereo PPM captures: $OUT"
