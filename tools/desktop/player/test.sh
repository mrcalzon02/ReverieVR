#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
BASE="$ROOT/app/src/main/java/io/github/mrcalzon02/reverievr"
SRC="$ROOT/tools/desktop/player"
OUT="$ROOT/build/desktop-player/tests"
mkdir -p "$OUT"
javac --release 17 -encoding UTF-8 -d "$OUT" "$BASE/HomeEnvironment.java" "$BASE/GameModuleRegistry.java" \
 "$SRC/DesktopHubState.java" "$SRC/DesktopArcade.java" "$SRC/ReverieDesktopPlayer.java" "$SRC/DesktopHubTest.java"
java -Djava.awt.headless=true -cp "$OUT" io.github.mrcalzon02.reverievr.DesktopHubTest
