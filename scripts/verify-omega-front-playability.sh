#!/usr/bin/env bash
# All Omega Front headless progression tests, including boot -> combat,
# a playable order, and checkpoint round-trip. Does not claim VR device proof.
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
tmp="$(mktemp -d "${TMPDIR:-/tmp}/omega-front-tests.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT
count=0
for source in "$root"/app/src/test/native/omega_front_*_test.cpp; do
    name="$(basename "$source" .cpp)"
    "${CXX:-c++}" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
        -I "$root/app/src/main/jni/native" "$source" -o "$tmp/$name"
    "$tmp/$name"
    count=$((count + 1))
done
echo "PASS: $count Omega Front C++ simulation and launch-flow tests"
