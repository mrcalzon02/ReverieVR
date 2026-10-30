#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
VENDOR="$ROOT/tools/editor-kit/upstream"
OUT="$ROOT/build/sitka/editor-test-classes"
mkdir -p "$OUT"
javac --release 17 -encoding UTF-8 -d "$OUT" \
 "$VENDOR"/src/main/java/org/beckenburg/editor/*.java \
 "$VENDOR"/water-module/src/main/java/dev/mrcalzon/water/*.java \
 "$VENDOR"/src/main/java/dev/mrcalzon/weather/*.java \
 "$VENDOR"/lighting-module/src/main/java/dev/mrcalzon/lighting/*.java \
 "$ROOT"/tools/desktop/LightingAuthoringPanel.java \
 "$ROOT"/tools/desktop/LightingAuthoringPanelTest.java \
 "$ROOT"/tools/sitka/SitkaEditor.java \
 "$ROOT"/tools/sitka/SitkaEditorCoreTest.java
java -Djava.awt.headless=true -cp "$OUT" SitkaEditorCoreTest
java -Djava.awt.headless=true -cp "$OUT" io.github.mrcalzon02.reverievr.LightingAuthoringPanelTest
