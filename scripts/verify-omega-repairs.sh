#!/usr/bin/env bash
# Real Omega module GL, menu, campaign, save and guarded-exit diagnostics.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
OUT="${1:-$WORK/captures}"
mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"
"${CXX:-g++}" -std=c++17 -O1 -Wall -Wextra -Wpedantic -Werror \
    -I"$ROOT/app/src/main/jni/native" -I"$ROOT/scripts/audit/probes" \
    "$ROOT/scripts/audit/probes/omega_repair_probe.cpp" \
    -lEGL -lGLESv2 -lz -o "$WORK/omega"
cd "$ROOT"
"$WORK/omega" "$OUT"
