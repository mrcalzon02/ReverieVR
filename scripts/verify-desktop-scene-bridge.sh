#!/usr/bin/env bash
set -euo pipefail
# Runs without Android SDK, external network or headless Java desktop UI.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PACKAGE="$ROOT/app/src/main/java/io/github/mrcalzon02/reverievr"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/java"
javac --release 17 -d "$TMP/java" \
 "$PACKAGE/HomeEnvironment.java" \
 "$PACKAGE/GameModuleRegistry.java" \
 "$PACKAGE/ObjMeshReader.java" \
 "$ROOT/tools/editor-kit/upstream"/src/main/java/org/beckenburg/editor/*.java \
 "$ROOT/tools/desktop/SceneDocument.java" \
 "$ROOT/tools/desktop/SceneDocumentTest.java" \
 "$ROOT/tools/desktop/ReverieSceneProject.java" \
 "$ROOT/tools/desktop/ReverieSceneProjectTest.java" \
 "$ROOT/tools/desktop/ReverieDesktop.java"
java -cp "$TMP/java" io.github.mrcalzon02.reverievr.SceneDocumentTest "$TMP/from-java.rvscene"
java -cp "$TMP/java" io.github.mrcalzon02.reverievr.ReverieSceneProjectTest
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Wpedantic -Werror -O2 \
 "$ROOT/app/src/test/native/reverie_scene_document_test.cpp" -o "$TMP/scene-test"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Wpedantic -Werror -O2 \
 "$ROOT/app/src/test/native/reverie_scene_asset_scope_test.cpp" -o "$TMP/scope-test"
"$TMP/scene-test" "$TMP/from-java.rvscene" "$ROOT/app/src/main/assets/native/test-chamber/scene.rvscene"
"$TMP/scope-test"
printf 'Desktop/native authoring bridge: PASS (Java17 + native C++17 + packaged fixture)\n'