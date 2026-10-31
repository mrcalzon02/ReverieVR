#!/usr/bin/env bash
set -euo pipefail
# Compile and execute exact vendored upstream portable editor regression fixtures.
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
VENDOR="$ROOT/tools/editor-kit/upstream"
TESTS="$ROOT/tools/editor-kit/upstream-tests/src/test/java/org/beckenburg/editor"
OUT="$ROOT/build/editor-ported-tests/classes"
mkdir -p "$OUT"
python3 "$ROOT/tools/editor-kit/sync-upstream.py"
javac --release 17 -encoding UTF-8 -d "$OUT" \
  "$VENDOR"/src/main/java/org/beckenburg/editor/*.java \
  "$VENDOR"/src/main/java/org/beckenburg/editor/ai/*.java \
  "$VENDOR"/src/main/java/dev/mrcalzon/weather/*.java \
  "$VENDOR"/lighting-module/src/main/java/dev/mrcalzon/lighting/*.java \
  "$VENDOR"/water-module/src/main/java/dev/mrcalzon/water/*.java \
  "$TESTS"/*.java "$TESTS"/ai/*.java
for test in EditorAnimationClipTest EditorSoundProfileTest EditorBillboardProfileTest \
            EditorCampaignFlowTest EditorMeshPipelineTest; do
  java -Djava.awt.headless=true -cp "$OUT" "org.beckenburg.editor.$test"
done
for test in NpcAiTelemetryTest NpcRuleWorkbenchTest; do
  java -Djava.awt.headless=true -cp "$OUT" "org.beckenburg.editor.ai.$test"
done
echo "PASS: seven real upstream editor/AI functional regressions"
