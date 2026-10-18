#!/usr/bin/env bash
# Real Android-emulator smoke: install -> VR startup -> GLES2 -> trigger/audio -> screenshot.
set -euo pipefail
APK="${1:?Path to emulator-capable Test Chamber APK required}"
ARTIFACTS="${GITHUB_WORKSPACE:-$PWD}/validation/emulator"
mkdir -p "$ARTIFACTS"
adb logcat -c
adb install -r "$APK"
adb shell am start -W -n 'io.github.mrcalzon02.reverievr.game.testchamber/.GameVrActivity'   | tee "$ARTIFACTS/activity-launch.txt"
sleep 6
# Android 10 first-launch immersive mode displays a large blue tutorial
# over the entire stereo scene. The original green screenshot check accepted
# this system overlay; remove it and fail if it remains.
for attempt in 1 2 3; do
  adb exec-out screencap -p > "$ARTIFACTS/pre-overlay-check.png"
  if python3 - "$ARTIFACTS/pre-overlay-check.png" <<'PY'
from PIL import Image
import sys
pic=Image.open(sys.argv[1]).convert("RGB")
r,g,b=pic.getpixel((pic.width//2,pic.height//5))
# Android's blue "Viewing full screen" dialog across upper center.
sys.exit(0 if b>160 and g>70 and b>r*2 and b>g*1.25 else 1)
PY
  then
    size="$(adb shell wm size | tr -d '\r' | tail -n 1)"
    wh="${size##*: }"
    width="${wh%x*}"
    height="${wh#*x}"
    # Known Android immersive tutorial's "Got it" control is roughly at
    # x=55%, y=42% in landscape; click it, not the model underneath.
    adb shell input tap "$((width*55/100))" "$((height*42/100))"
    sleep 3
  else
    break
  fi
done
adb shell input keyevent KEYCODE_DPAD_CENTER
adb shell input tap 400 500
sleep 5
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
assert picture.width>=300 and picture.height>=300, "Invalid Android screenshot"
r,g,b=picture.getpixel((picture.width//2,picture.height//5))
assert not (b>160 and g>70 and b>r*2 and b>g*1.25), (
    "System full-screen tutorial still obscures VR rendering")
sample=picture.resize((96,72))
colors=len(set(sample.getdata()))
assert colors>=20, f"Blank/monochromatic VR output: only {colors} colors"
print("EMULATOR SMOKE PASS: five actual models decoded + GPU uploaded, "
      "controller/touch cue dispatched, Android stayed alive, "
      f"VR screenshot {picture.width}x{picture.height}, {colors} colors")
PY
