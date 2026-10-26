#!/usr/bin/env bash
set -euo pipefail
# Compiles against actual, unmodified Beckenburg portable source-not copied forks.
REVERIE_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BECKENBURG_ROOT="${BECKENBURG_ROOT:-$(cd "$REVERIE_ROOT/.." && pwd)/The-Beckenburg-Project}"
WORKSPACE="${1:-$REVERIE_ROOT/build/sitka/authoring}"
OUT="$REVERIE_ROOT/build/sitka/editor-classes"
if [[ ! -f "$BECKENBURG_ROOT/src/main/java/org/beckenburg/editor/EditorProject.java" ]] || \
   [[ ! -f "$BECKENBURG_ROOT/water-module/src/main/java/dev/mrcalzon/water/WaterDocument.java" ]] || \
   [[ ! -f "$BECKENBURG_ROOT/src/main/java/dev/mrcalzon/weather/WeatherModule.java" ]]; then
  echo "Missing Beckenburg source. Set BECKENBURG_ROOT to a checkout of mrcalzon02/The-Beckenburg-Project." >&2
  exit 2
fi
mkdir -p "$OUT"
# Sources are the editor-kit, water-module, and shared weather classes used in
# Beckenburg/Malefic; no Beckenburg game or Malefic rendering engine is copied.
javac --release 17 -encoding UTF-8 -d "$OUT" \
  "$BECKENBURG_ROOT"/src/main/java/org/beckenburg/editor/*.java \
  "$BECKENBURG_ROOT"/water-module/src/main/java/dev/mrcalzon/water/*.java \
  "$BECKENBURG_ROOT"/src/main/java/dev/mrcalzon/weather/*.java \
  "$REVERIE_ROOT"/tools/sitka/SitkaEditor.java
exec java -cp "$OUT" SitkaEditor "$WORKSPACE"
