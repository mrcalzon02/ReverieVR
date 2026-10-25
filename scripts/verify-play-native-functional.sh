#!/usr/bin/env bash
# Google Play candidate requires every native/diagnostic-intake functional smoke
# already mandated by the established phone-test publisher. No skips or stubs.
set -euo pipefail
cd "$(dirname "$0")/.."
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT
includes=(-I app/src/main/jni/native)
flags=(-std=c++17 -Wall -Wextra -Wpedantic -Werror)
native_test() {
  local output="$1"; shift
  c++ "${flags[@]}" "${includes[@]}" "$@" -o "$tmp/$output"
  "$tmp/$output"
}
node --check diagnostic-intake/src/index.js
node diagnostic-intake/test/worker-validation-smoke.cjs
native_test red-ledger-simulation   app/src/main/jni/native/red_ledger_simulation.cpp   app/src/test/native/red_ledger_simulation_test.cpp
for game in lantern_desk forward_detachment; do
  native_test "${game}-simulation"     "app/src/main/jni/native/${game}_simulation.cpp"     "app/src/test/native/${game}_simulation_test.cpp"
done
native_test breakwater-campaign   app/src/main/jni/native/breakwater_campaign.cpp   app/src/test/native/breakwater_campaign_test.cpp
native_test classic-games   app/src/test/native/classic_games_test.cpp
native_test procedural-material   app/src/main/jni/native/procedural_material_atlas.cpp   app/src/test/native/procedural_material_atlas_test.cpp
native_test red-ledger-static   app/src/main/jni/native/red_ledger_static_geometry.cpp   app/src/test/native/red_ledger_static_geometry_test.cpp
echo 'PASS: phone-test publisher native/diagnostic-intake functional smoke set completed'
