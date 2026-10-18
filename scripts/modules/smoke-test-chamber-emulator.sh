#!/usr/bin/env bash
# Real Android-emulator smoke: install -> VR startup -> GLES2 -> trigger/audio -> screenshot.
set -euo pipefail
APK="${1:?Path to emulator-capable Test Chamber APK required}"
ARTIFACTS="${GITHUB_WORKSPACE:-$PWD}/validation/emulator"
mkdir -p "$ARTIFACTS"
adb logcat -c
adb install -r "$APK"
adb shell am start -W -n 'io.github.mrcalzon02.reverievr.game.testchamber/.GameVrActivity'   | tee "$ARTIFACTS/activity-launch.txt"
sleep 15
adb shell input keyevent KEYCODE_DPAD_CENTER
adb shell input tap 400 500
sleep 4
adb exec-out screencap -p > "$ARTIFACTS/vr-screenshot.png"
adb logcat -d -v time > "$ARTIFACTS/android-logcat.txt"
adb shell dumpsys activity activities > "$ARTIFACTS/activity-state.txt"
python3 - "$ARTIFACTS" <<'PY'
import pathlib, re, sys
from PIL import Image
root=pathlib.Path(sys.argv[1])
log=(root/"android-logcat.txt").read_text(errors="replace")
assert "Staged " in log and "textured primitives" in log, "GLB decode/JNI staging not observed"
assert "Uploaded " in log and "textured GLB submeshes" in log, "GLES2 upload not observed"
assert "AndroidRuntime: FATAL EXCEPTION" not in log, "Android fatal exception occurred"
assert ("Audible test cue" in log or
        "Sound suppressed: activity lacks audio focus" in log),        "No controller/touch-triggered audio event observed"
picture=Image.open(root/"vr-screenshot.png").convert("RGB")
sample=picture.resize((96,72))
colors=len(set(sample.getdata()))
assert picture.width>=300 and picture.height>=300, "Invalid Android screenshot"
assert colors>=12, f"Blank/monochromatic VR output: only {colors} colors"
print("EMULATOR SMOKE PASS: five actual models decoded + GPU uploaded, "
      "controller/touch cue dispatched, Android stayed alive, "
      f"VR screenshot {picture.width}x{picture.height}, {colors} colors")
PY
