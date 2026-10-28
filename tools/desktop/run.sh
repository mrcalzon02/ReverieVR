#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/build/desktop-workbench/classes"
BASE="$ROOT/app/src/main/java/io/github/mrcalzon02/reverievr"
mkdir -p "$OUT"
command -v javac >/dev/null || { echo 'JDK 17+ required (javac not found)' >&2; exit 2; }
javac --release 17 -encoding UTF-8 -d "$OUT" \
 "$BASE/HomeEnvironment.java" "$BASE/GameModuleRegistry.java" "$BASE/ObjMeshReader.java" \
 "$ROOT/tools/desktop/SceneDocument.java" "$ROOT/tools/desktop/ReverieDesktop.java"
exec java -cp "$OUT" io.github.mrcalzon02.reverievr.ReverieDesktop "$ROOT" "${1:-$ROOT/build/desktop-workspace}"
