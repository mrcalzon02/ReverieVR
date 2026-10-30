#!/usr/bin/env bash
set -euo pipefail
# Offline Java 17 Sitka editor: the pinned Beckenburg portable editor, water
# and weather source are integrated directly into ReverieVR (desktop only).
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
VENDOR="$ROOT/tools/editor-kit/upstream"
WORKSPACE="${1:-$ROOT/build/sitka/authoring}"
OUT="$ROOT/build/sitka/editor-classes"
command -v javac >/dev/null || { echo "JDK 17+ required" >&2; exit 2; }
mkdir -p "$OUT"
javac --release 17 -encoding UTF-8 -d "$OUT" \
  "$VENDOR"/src/main/java/org/beckenburg/editor/*.java \
  "$VENDOR"/water-module/src/main/java/dev/mrcalzon/water/*.java \
  "$VENDOR"/src/main/java/dev/mrcalzon/weather/*.java \
  "$ROOT"/tools/sitka/SitkaEditor.java
exec java -cp "$OUT" SitkaEditor "$WORKSPACE"
