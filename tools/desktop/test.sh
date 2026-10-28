#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BASE="$ROOT/app/src/main/java/io/github/mrcalzon02/reverievr"
OUT="$ROOT/build/desktop-workbench/tests"
mkdir -p "$OUT"
javac --release 17 -encoding UTF-8 -d "$OUT" \
 "$BASE/HomeEnvironment.java" "$BASE/GameModuleRegistry.java" "$BASE/ObjMeshReader.java" \
 "$ROOT/tools/desktop/SceneDocument.java" "$ROOT/tools/desktop/SceneDocumentTest.java" \
 "$ROOT/tools/desktop/ReverieDesktop.java"
java -cp "$OUT" io.github.mrcalzon02.reverievr.SceneDocumentTest
