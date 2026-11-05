#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
"${CXX:-g++}" -std=c++17 -Wall -Wextra -Wpedantic -Werror -O2 \
 "$ROOT/app/src/test/native/guess_the_door_game_test.cpp" \
 -o "$TMP/guess_the_door_game_test"
"$TMP/guess_the_door_game_test"
