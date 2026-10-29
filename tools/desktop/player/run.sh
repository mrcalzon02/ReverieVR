#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
BASE="$ROOT/app/src/main/java/io/github/mrcalzon02/reverievr"
SRC="$ROOT/tools/desktop/player"
OUT="$ROOT/build/desktop-player/classes"
command -v javac >/dev/null || { echo 'JDK 17+ required' >&2; exit 2; }
mkdir -p "$OUT"
javac --release 17 -encoding UTF-8 -d "$OUT" \
 "$BASE/HomeEnvironment.java" "$BASE/GameModuleRegistry.java" \
 "$SRC/DesktopHubState.java" "$SRC/DesktopDeskKeyboard.java" "$SRC/DesktopArcade.java" "$SRC/ReverieDesktopPlayer.java"
exec java -cp "$OUT" io.github.mrcalzon02.reverievr.ReverieDesktopPlayer "${1:-$ROOT/build/desktop-workspace}"
