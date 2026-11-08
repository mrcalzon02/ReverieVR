#!/usr/bin/env bash
# Existing reconnaissance prototypes: strict kernels/codec plus optional real GLES.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
CXX="${CXX:-g++}"
INC="$ROOT/app/src/main/jni/native"
TEST="$ROOT/app/src/test/native"
FLAGS=(-std=c++17 -O1 -Wall -Wextra -Wpedantic -Werror -I"$INC" -I"$TEST")
for title in lantern_desk forward_detachment; do
    "$CXX" "${FLAGS[@]}" "$TEST/${title}_simulation_test.cpp" "$INC/${title}_simulation.cpp" -o "$WORK/$title"
    "$WORK/$title"
done
"$CXX" "${FLAGS[@]}" "$TEST/recon_projects_save_test.cpp" \
    "$INC/lantern_desk_simulation.cpp" "$INC/forward_detachment_simulation.cpp" -o "$WORK/save"
"$WORK/save"
if [[ "${1:-}" == "--gles" ]]; then
    OUT="${2:-$WORK/captures}"
    mkdir -p "$OUT"
    OUT="$(cd "$OUT" && pwd)"
    # Keep existing shared optimized GL diagnostic formatting warnings visible.
    GLES_FLAGS=(-std=c++17 -O1 -Wall -Wextra -Wpedantic -I"$INC" -I"$TEST")
    for kind in 0 1; do
        if [[ "$kind" == 0 ]]; then title=lantern_desk; else title=forward_detachment; fi
        "$CXX" "${GLES_FLAGS[@]}" -DRECON_PROJECT_KIND="$kind" \
            "$TEST/recon_module_save_test.cpp" "$INC/${title}_simulation.cpp" -lEGL -lGLESv2 -lz -o "$WORK/module-$kind"
        (cd "$ROOT" && "$WORK/module-$kind" "$OUT")
    done
    echo "PASS actual recon module persistence; stereo PPM captures: $OUT"
fi
