#!/usr/bin/env bash
set -euo pipefail

TASK_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TASK_REPO_ROOT="${1:-$(cd "$TASK_SCRIPT_DIR/../.." && pwd)}"
TASK_SOURCE_DIR="$TASK_REPO_ROOT/app/src/main/java/io/github/mrcalzon02/reverievr"
if command -v javac >/dev/null 2>&1; then
  TASK_JAVAC=(javac)
elif java --describe-module jdk.compiler >/dev/null 2>&1; then
  TASK_JAVAC=(java --module jdk.compiler/com.sun.tools.javac.Main)
else
  printf 'Java 17+ with the jdk.compiler module is required.\n' >&2
  exit 2
fi
TASK_BUILD_DIR="$(mktemp -d "${TMPDIR:-/tmp}/reverie-red-ledger-audio.XXXXXX")"
trap 'rm -rf "$TASK_BUILD_DIR"' EXIT
task_java_files() {
  if command -v rg >/dev/null 2>&1; then
    rg --files --null -g '*.java' "$1"
  else
    find "$1" -type f -name '*.java' -print0
  fi
}
TASK_COMMON=()
while IFS= read -r -d '' TASK_FILE; do
  # Compile the real bank instead of the existing routing-only double.
  [[ "$TASK_FILE" == */reverievr/RedLedgerSound.java ]] && continue
  TASK_COMMON+=("$TASK_FILE")
done < <(task_java_files "$TASK_SCRIPT_DIR/common")
mapfile -d '' -t TASK_HARNESS < <(task_java_files "$TASK_SCRIPT_DIR/red-ledger")
TASK_PRODUCTION=()
for TASK_CLASS in UiFeedback RedLedgerSound BreakwaterAudio BreakwaterAudioPolicy MusicVolumePolicy; do
  TASK_PRODUCTION+=("$TASK_SOURCE_DIR/$TASK_CLASS.java")
done
"${TASK_JAVAC[@]}" -source 17 -target 17 -Xlint:-options -encoding UTF-8 \
  -d "$TASK_BUILD_DIR" "${TASK_COMMON[@]}" "${TASK_HARNESS[@]}" "${TASK_PRODUCTION[@]}"
java -cp "$TASK_BUILD_DIR" io.github.mrcalzon02.reverievr.RedLedgerAudioLifecycleHarness \
  "$TASK_REPO_ROOT/app/src/main/res/raw"
