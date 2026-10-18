#!/bin/sh
# Android-free regression: uses the exact Java class consumed by the APK.
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
SOURCE="$ROOT/game-apps/test-chamber/src/main/java/io/github/mrcalzon02/reverievr/game/testchamber/TestChamberTargeting.java"
TEST="$ROOT/game-apps/test-chamber/src/test-standalone/java/io/github/mrcalzon02/reverievr/game/testchamber/TestChamberTargetingStandaloneTest.java"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT HUP INT TERM
javac -d "$WORK" "$SOURCE" "$TEST"
java -cp "$WORK" io.github.mrcalzon02.reverievr.game.testchamber.TestChamberTargetingStandaloneTest
