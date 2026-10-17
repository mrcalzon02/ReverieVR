#!/usr/bin/env bash
# Host-only Red Ledger simulation, room and signage regressions; no Actions.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
INC="$ROOT/app/src/main/jni/native"
TEST="$ROOT/app/src/test/native"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
ran=0
for compiler in g++ clang++; do
  if ! command -v "$compiler" >/dev/null 2>&1; then
    echo "SKIP $compiler (unavailable)"
    continue
  fi
  ran=$((ran + 1))
  for name in red_ledger_faction_integration red_ledger_trade red_ledger_wholesale red_ledger_stock_layout red_ledger_original_assets red_ledger_signage red_ledger_mug_mesh red_ledger_static_geometry red_ledger_simulation; do
    extra=()
    if [[ "$name" == red_ledger_static_geometry ]]; then
      extra=("$INC/red_ledger_static_geometry.cpp")
    elif [[ "$name" == red_ledger_simulation || "$name" == red_ledger_faction_integration || "$name" == red_ledger_wholesale ]]; then
      extra=("$INC/red_ledger_simulation.cpp")
    fi
    link=()
    if [[ "$name" == red_ledger_original_assets ]]; then
      link=(-lz)
    fi
    "$compiler" -std=c++17 -O1 -Wall -Wextra -Wpedantic -Werror \
      -I"$INC" "$TEST/${name}_test.cpp" "${extra[@]}" \
      -o "$WORK/$name" "${link[@]}"
    if [[ "$name" == red_ledger_original_assets ]]; then
      "$WORK/$name" "$ROOT"
    else
      "$WORK/$name"
    fi
  done
done
if [[ "$ran" -eq 0 ]]; then
  echo "No C++17 compiler found" >&2
  exit 1
fi
