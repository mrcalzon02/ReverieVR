#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
SOURCE="$ROOT/app/src/test/native/gumball_armada_test.cpp"
OBJECTIVES="$ROOT/app/src/test/native/gumball_arcade_objectives_test.cpp"
SHIPYARD="$ROOT/app/src/test/native/gumball_shipyard_test.cpp"
CAST="$ROOT/app/src/test/native/gumball_cast_encounters_test.cpp"
SUPPORT="$ROOT/app/src/test/native/gumball_support_contracts_test.cpp"
MENU="$ROOT/app/src/test/native/gumball_menu_layout_test.cpp"
MENU_HIT="$ROOT/app/src/test/native/gumball_menu_hit_test.cpp"
FORMATION="$ROOT/app/src/test/native/gumball_wave_formations_test.cpp"
READINESS="$ROOT/app/src/test/native/gumball_shipyard_readiness_test.cpp"
INCLUDE="$ROOT/app/src/main/jni/native"
for compiler in g++ clang++; do
    if ! command -v "$compiler" >/dev/null 2>&1; then
        echo "SKIP $compiler (not installed)"
        continue
    fi
    echo "Testing Gumball Armada with $compiler"
    "$compiler" -std=c++17 -O1 -Wall -Wextra -Werror -pedantic \
        -I"$INCLUDE" "$SOURCE" -o "$WORK/gumball-$compiler"
    "$WORK/gumball-$compiler"
    "$compiler" -std=c++17 -O1 -Wall -Wextra -Werror -pedantic \
        -I"$INCLUDE" "$OBJECTIVES" -o "$WORK/objectives-$compiler"
    "$WORK/objectives-$compiler"
    "$compiler" -std=c++17 -O1 -Wall -Wextra -Werror -pedantic \
        -I"$INCLUDE" "$SHIPYARD" -o "$WORK/shipyard-$compiler"
    "$WORK/shipyard-$compiler"
    "$compiler" -std=c++17 -O1 -Wall -Wextra -Werror -pedantic \
        -I"$INCLUDE" "$READINESS" -o "$WORK/readiness-$compiler"
    "$WORK/readiness-$compiler"
    "$compiler" -std=c++17 -O1 -Wall -Wextra -Werror -pedantic \
        -I"$INCLUDE" "$CAST" -o "$WORK/cast-$compiler"
    "$WORK/cast-$compiler"
    "$compiler" -std=c++17 -O1 -Wall -Wextra -Werror -pedantic \
        -I"$INCLUDE" "$SUPPORT" -o "$WORK/support-$compiler"
    "$WORK/support-$compiler"
    "$compiler" -std=c++17 -O1 -Wall -Wextra -Werror -pedantic \
        -I"$INCLUDE" "$MENU" -o "$WORK/menu-$compiler"
    "$WORK/menu-$compiler"
    "$compiler" -std=c++17 -O1 -Wall -Wextra -Werror -pedantic \
        -I"$INCLUDE" "$MENU_HIT" -o "$WORK/menuhit-$compiler"
    "$WORK/menuhit-$compiler"
    "$compiler" -std=c++17 -O1 -Wall -Wextra -Werror -pedantic \
        -I"$INCLUDE" "$FORMATION" -o "$WORK/formations-$compiler"
    "$WORK/formations-$compiler"
done
if command -v clang++ >/dev/null 2>&1; then
    clang++ -std=c++17 -O1 -g -Wall -Wextra -Werror -pedantic \
        -fsanitize=address,undefined -fno-omit-frame-pointer \
        -I"$INCLUDE" "$SOURCE" -o "$WORK/gumball-sanitized"
    "$WORK/gumball-sanitized"
    clang++ -std=c++17 -O1 -g -Wall -Wextra -Werror -pedantic \
        -fsanitize=address,undefined -fno-omit-frame-pointer \
        -I"$INCLUDE" "$OBJECTIVES" -o "$WORK/objectives-sanitized"
    "$WORK/objectives-sanitized"
    clang++ -std=c++17 -O1 -g -Wall -Wextra -Werror -pedantic \
        -fsanitize=address,undefined -fno-omit-frame-pointer \
        -I"$INCLUDE" "$SHIPYARD" -o "$WORK/shipyard-sanitized"
    "$WORK/shipyard-sanitized"
    clang++ -std=c++17 -O1 -g -Wall -Wextra -Werror -pedantic \
        -fsanitize=address,undefined -fno-omit-frame-pointer \
        -I"$INCLUDE" "$READINESS" -o "$WORK/readiness-sanitized"
    "$WORK/readiness-sanitized"
    clang++ -std=c++17 -O1 -g -Wall -Wextra -Werror -pedantic \
        -fsanitize=address,undefined -fno-omit-frame-pointer \
        -I"$INCLUDE" "$CAST" -o "$WORK/cast-sanitized"
    "$WORK/cast-sanitized"
    clang++ -std=c++17 -O1 -g -Wall -Wextra -Werror -pedantic \
        -fsanitize=address,undefined -fno-omit-frame-pointer \
        -I"$INCLUDE" "$SUPPORT" -o "$WORK/support-sanitized"
    "$WORK/support-sanitized"
    clang++ -std=c++17 -O1 -g -Wall -Wextra -Werror -pedantic \
        -fsanitize=address,undefined -fno-omit-frame-pointer \
        -I"$INCLUDE" "$MENU" -o "$WORK/menu-sanitized"
    "$WORK/menu-sanitized"
    clang++ -std=c++17 -O1 -g -Wall -Wextra -Werror -pedantic \
        -fsanitize=address,undefined -fno-omit-frame-pointer \
        -I"$INCLUDE" "$MENU_HIT" -o "$WORK/menuhit-sanitized"
    "$WORK/menuhit-sanitized"
    clang++ -std=c++17 -O1 -g -Wall -Wextra -Werror -pedantic \
        -fsanitize=address,undefined -fno-omit-frame-pointer \
        -I"$INCLUDE" "$FORMATION" -o "$WORK/formations-sanitized"
    "$WORK/formations-sanitized"
fi
