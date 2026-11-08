#!/usr/bin/env bash
set -euo pipefail

TASK_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TASK_REPO_ROOT="${1:-$(cd "$TASK_SCRIPT_DIR/../.." && pwd)}"
TASK_SOURCE_DIR="$TASK_REPO_ROOT/app/src/main/java/io/github/mrcalzon02/reverievr"

for TASK_SOURCE in UiFeedback BreakwaterAudio BreakwaterAudioPolicy MusicVolumePolicy; do
  if [[ ! -f "$TASK_SOURCE_DIR/$TASK_SOURCE.java" ]]; then
    printf 'Missing production source: %s\n' "$TASK_SOURCE_DIR/$TASK_SOURCE.java" >&2
    exit 2
  fi
done

if command -v javac >/dev/null 2>&1; then
  TASK_JAVAC=(javac)
elif java --describe-module jdk.compiler >/dev/null 2>&1; then
  TASK_JAVAC=(java --module jdk.compiler/com.sun.tools.javac.Main)
else
  printf 'Java 17+ with the jdk.compiler module is required.\n' >&2
  exit 2
fi

TASK_BUILD_DIR="$(mktemp -d "${TMPDIR:-/tmp}/reverie-audio-tests.XXXXXX")"
trap 'rm -rf "$TASK_BUILD_DIR"' EXIT
mkdir -p "$TASK_BUILD_DIR/ui" "$TASK_BUILD_DIR/backend"

task_java_files() {
  if command -v rg >/dev/null 2>&1; then
    rg --files --null -g '*.java' "$1"
  else
    find "$1" -type f -name '*.java' -print0
  fi
}
mapfile -d '' -t TASK_COMMON < <(task_java_files "$TASK_SCRIPT_DIR/common")
mapfile -d '' -t TASK_UI < <(task_java_files "$TASK_SCRIPT_DIR/ui")
mapfile -d '' -t TASK_BACKEND < <(task_java_files "$TASK_SCRIPT_DIR/backend")

"${TASK_JAVAC[@]}" -source 17 -target 17 -Xlint:-options -encoding UTF-8 -d "$TASK_BUILD_DIR/ui" \
  "${TASK_COMMON[@]}" "${TASK_UI[@]}" \
  "$TASK_SOURCE_DIR/UiFeedback.java" "$TASK_SOURCE_DIR/BreakwaterAudioPolicy.java"
java -cp "$TASK_BUILD_DIR/ui" io.github.mrcalzon02.reverievr.UiFeedbackHarness

"${TASK_JAVAC[@]}" -source 17 -target 17 -Xlint:-options -encoding UTF-8 -d "$TASK_BUILD_DIR/backend" \
  "${TASK_COMMON[@]}" "${TASK_BACKEND[@]}" \
  "$TASK_SOURCE_DIR/UiFeedback.java" "$TASK_SOURCE_DIR/BreakwaterAudio.java" \
  "$TASK_SOURCE_DIR/BreakwaterAudioPolicy.java" "$TASK_SOURCE_DIR/MusicVolumePolicy.java"
java -cp "$TASK_BUILD_DIR/backend" io.github.mrcalzon02.reverievr.BreakwaterAudioBackendHarness
