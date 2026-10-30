#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BASE="$ROOT/app/src/main/java/io/github/mrcalzon02/reverievr"
OUT="$ROOT/build/desktop-workbench/tests"
mkdir -p "$OUT"
javac --release 17 -encoding UTF-8 -d "$OUT" \
 "$BASE/HomeEnvironment.java" "$BASE/GameModuleRegistry.java" "$BASE/ObjMeshReader.java" \
 "$ROOT/tools/editor-kit/upstream"/src/main/java/org/beckenburg/editor/*.java \
 "$ROOT/tools/editor-kit/upstream"/lighting-module/src/main/java/dev/mrcalzon/lighting/*.java \
 "$ROOT/tools/desktop/LightingAuthoringPanel.java" \
 "$ROOT/tools/desktop/SceneDocument.java" "$ROOT/tools/desktop/SceneDocumentTest.java" \
 "$ROOT/tools/desktop/ReverieSceneProject.java" "$ROOT/tools/desktop/ReverieSceneProjectTest.java" \
 "$ROOT/tools/desktop/ReverieDesktop.java"
java -cp "$OUT" io.github.mrcalzon02.reverievr.SceneDocumentTest

java -cp "$OUT" io.github.mrcalzon02.reverievr.ReverieSceneProjectTest
