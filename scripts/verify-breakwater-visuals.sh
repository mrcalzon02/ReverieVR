#!/usr/bin/env bash
# No GitHub Actions. Locally check render/UI geometry with strict host compilers.
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
  ran=$((ran+1))
  for name in breakwater_menu_layout breakwater_scene_refresh breakwater_gun_mesh breakwater_shop_feedback breakwater_cannon_trajectory breakwater_wave_schedule breakwater_wave_threat breakwater_armor breakwater_invasion_units breakwater_environment; do
    "$compiler" -std=c++17 -O1 -Wall -Wextra -Wpedantic -Werror \
      -I"$INC" "$TEST/${name}_test.cpp" -o "$WORK/${name}"
    "$WORK/${name}"
  done
done
if [[ "$ran" -eq 0 ]]; then
  echo "No C++17 compiler" >&2
  exit 1
fi
