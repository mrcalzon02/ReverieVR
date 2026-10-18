#!/usr/bin/env bash
# Host-only Omega Front combat-audio regression; no Android emulator,
# network fetch, or GitHub Actions dependency.
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
bin="$(mktemp "${TMPDIR:-/tmp}/omega-front-audio.XXXXXX")"
trap 'rm -f "$bin"' EXIT
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Wpedantic -Werror \
  -I "$root/app/src/main/jni/native" \
  "$root/app/src/test/native/omega_front_sound_test.cpp" \
  -o "$bin"
"$bin"
