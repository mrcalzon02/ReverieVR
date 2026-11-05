#!/usr/bin/env bash
# No GitHub Actions. Locally check render/UI geometry with strict host compilers.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
INC="$ROOT/app/src/main/jni/native"
TEST="$ROOT/app/src/test/native"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
cd "$ROOT" # material test reads the exact packaged files
python3 scripts/verify-shared-native-assets.py
ran=0
for compiler in g++ clang++; do
  if ! command -v "$compiler" >/dev/null 2>&1; then
    echo "SKIP $compiler (not installed)"
    continue
  fi
  ran=$((ran+1))
  for name in native_horizon_geometry breakwater_unit_geometry breakwater_materials breakwater_ocean breakwater_authored_materials breakwater_asset_composition breakwater_font_atlas breakwater_menu_layout breakwater_scene_refresh breakwater_gun_mesh breakwater_defense_points breakwater_aircraft_mesh breakwater_emplacement breakwater_shore_cladding breakwater_shop_feedback breakwater_cannon_trajectory breakwater_wave_schedule breakwater_wave_threat breakwater_armor breakwater_invasion_units breakwater_environment breakwater_sky; do
    "$compiler" -std=c++17 -O1 -Wall -Wextra -Wpedantic -Werror \
      -I"$INC" "$TEST/${name}_test.cpp" -o "$WORK/${name}" -lz
    "$WORK/${name}"
  done
  "$compiler" -std=c++17 -O1 -Wall -Wextra -Wpedantic -Werror \
    -I"$INC" "$TEST/native_asset_scope_test.cpp" -o "$WORK/native_scope"
  "$WORK/native_scope"
done
if [[ "$ran" -eq 0 ]]; then
  echo "No C++17 compiler" >&2
  exit 1
fi
