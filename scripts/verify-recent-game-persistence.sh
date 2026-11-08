#!/usr/bin/env bash
# Actual module fault/lifecycle regressions. Requires host GLES2/zlib headers
# and libraries; no GL context or physical headset is used by these tests.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
cd "$ROOT"
BASE=(-std=c++17 -Wall -Wextra -Wpedantic -Werror -Iapp/src/main/jni/native)
"${CXX:-g++}" "${BASE[@]}" -O1 \
 app/src/test/native/breakwater_persistence_test.cpp \
 app/src/main/jni/native/breakwater_campaign.cpp -lGLESv2 -lz -o "$TMP/breakwater"
"$TMP/breakwater"
# O0 avoids a pre-existing GCC format-truncation warning in the shared GL
# diagnostic helper; these lifecycle tests do not execute that GL path.
"${CXX:-g++}" "${BASE[@]}" -O0 \
 app/src/test/native/guess_the_door_persistence_test.cpp -lGLESv2 -lz -o "$TMP/guess"
"$TMP/guess"
"${CXX:-g++}" "${BASE[@]}" -O2 \
 app/src/test/native/reverie_vr_workshop_recovery_test.cpp -o "$TMP/workshop"
"$TMP/workshop"
# OpenKTG's existing anonymous structs and unused static helpers are warnings
# in the Android target as well. All other diagnostics remain errors here.
"${CXX:-g++}" "${BASE[@]}" -O0 -Wno-error=pedantic -Wno-error=unused-function \
 -Ithird_party/openktg/src app/src/test/native/test_chamber_persistence_test.cpp \
 third_party/openktg/src/gentexture.cpp -lGLESv2 -lz -o "$TMP/test-chamber"
"$TMP/test-chamber"
