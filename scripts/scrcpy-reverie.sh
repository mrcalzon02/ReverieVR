#!/usr/bin/env sh
set -eu

if ! command -v scrcpy >/dev/null 2>&1; then
    echo "scrcpy was not found on PATH." >&2
    echo "Install scrcpy, enable USB debugging on the Android device, then retry." >&2
    exit 127
fi

echo "ReverieVR scrcpy diagnostic session"
echo "  baseline: --stay-awake --no-audio"
echo "  NOTE: do not use --turn-screen-off while the phone is in the VR headset."
echo "  NOTE: close scrcpy before formal performance/thermal acceptance runs."

exec scrcpy --stay-awake --no-audio "$@"
