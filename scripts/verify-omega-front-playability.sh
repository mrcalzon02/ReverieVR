#!/usr/bin/env bash
# Execute EVERY Omega Front test, collecting all failures in one workflow
# run. These tests validate simulation and 2D map -> battle transitions;
# Android/VR GLES input is separately exercised in emulator instrumentation.
set -uo pipefail
ulimit -c 0 || true
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
tmp="$(mktemp -d "${TMPDIR:-/tmp}/omega-front-tests.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT
count=0
failures=0
for source in "$root"/app/src/test/native/omega_front_*_test.cpp; do
    name="$(basename "$source" .cpp)"
    count=$((count + 1))
    if ! "${CXX:-c++}" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
       -I "$root/app/src/main/jni/native" "$source" -o "$tmp/$name"; then
        echo "FAIL BUILD: $name" >&2
        failures=$((failures + 1))
        continue
    fi
    if "$tmp/$name"; then
        echo "PASS: $name"
    else
        echo "FAIL EXECUTION: $name" >&2
        failures=$((failures + 1))
    fi
done
echo "Omega Front full headless suite: $((count - failures))/$count passed"
if ((failures > 0)); then
    echo "FAIL: $failures Omega Front tests need root-cause repair" >&2
    exit 1
fi
