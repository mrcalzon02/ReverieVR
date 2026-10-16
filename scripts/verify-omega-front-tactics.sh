#!/usr/bin/env bash
# Host-only Omega Front battle-kernel regression. No NDK, Android SDK,
# GitHub Actions, network services, device, or shipped APK required.
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cxx="${CXX:-g++}"
binary="$(mktemp "${TMPDIR:-/tmp}/omega-front-tactics.XXXXXX")"
trap 'rm -f "$binary"' EXIT
"$cxx" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  -I"$root/app/src/main/jni/native" \
  "$root/app/src/test/native/omega_front_tactics_test.cpp" -o "$binary"
"$binary"
"$cxx" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -I"$root/app/src/main/jni/native" "$root/app/src/test/native/omega_front_controls_test.cpp" -o "$binary"
"$binary"
"$cxx" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -I"$root/app/src/main/jni/native" "$root/app/src/test/native/omega_front_checkpoint_test.cpp" -o "$binary"
"$binary"
"$cxx" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  -I"$root/app/src/main/jni/native" \
  "$root/app/src/test/native/omega_front_frontier_test.cpp" -o "$binary"
"$binary"
"$cxx" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  -I"$root/app/src/main/jni/native" \
  "$root/app/src/test/native/omega_front_session_test.cpp" -o "$binary"
"$binary"
"$cxx" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  -I"$root/app/src/main/jni/native" \
  "$root/app/src/test/native/omega_front_campaign_save_test.cpp" -o "$binary"
"$binary"
"$cxx" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -I"$root/app/src/main/jni/native" "$root/app/src/test/native/omega_front_objectives_test.cpp" -o "$binary"
"$binary"
"$cxx" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -I"$root/app/src/main/jni/native" "$root/app/src/test/native/omega_front_factions_test.cpp" -o "$binary"
"$binary"
"$cxx" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -I"$root/app/src/main/jni/native" "$root/app/src/test/native/omega_front_operations_test.cpp" -o "$binary"
"$binary"
