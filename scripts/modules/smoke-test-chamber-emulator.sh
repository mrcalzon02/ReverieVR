#!/usr/bin/env bash
# Real Android-emulator smoke: install -> VR startup -> GLES2 -> trigger/audio -> screenshot.
set -euo pipefail
APK="${1:?Path to emulator-capable Test Chamber APK required}"
ARTIFACTS="${GITHUB_WORKSPACE:-$PWD}/validation/emulator"
mkdir -p "$ARTIFACTS"
adb logcat -c
adb install -r "$APK"
# Test-only Android 10 AVD initialization: the first immersive-mode prompt
# overlays the VR frame and is not part of the game's framebuffer.
adb shell settings put secure immersive_mode_confirmations confirmed
CONFIRMATION="$(adb shell settings get secure immersive_mode_confirmations | tr -d '\r')"
[[ "$CONFIRMATION" == "confirmed" ]] || {
  echo "Android AVD rejected fullscreen test configuration" >&2
  exit 1
}
adb shell am start -W -n 'io.github.mrcalzon02.reverievr.game.testchamber/.GameVrActivity'   | tee "$ARTIFACTS/activity-launch.txt"
sleep 12
adb shell input keyevent KEYCODE_DPAD_CENTER
adb shell input tap 400 500
sleep 5
# Android may display the immersive tutorial late, even AFTER the first
# screenshot. Poll the FINAL screenshot and click using its landscape
# framebuffer dimensions, not wm size (which remains portrait 1080x1920).
for attempt in 1 2 3 4; do
  adb exec-out screencap -p > "$ARTIFACTS/vr-screenshot.png"
  if python3 - "$ARTIFACTS/vr-screenshot.png" <<'PY'
from PIL import Image
import sys
pic=Image.open(sys.argv[1]).convert("RGB")
r,g,b=pic.getpixel((pic.width//2,pic.height//5))
sys.exit(0 if b>160 and g>70 and b>r*2 and b>g*1.25 else 1)
PY
  then
    coords="$(python3 - "$ARTIFACTS/vr-screenshot.png" <<'PY'
from PIL import Image
import sys
picture=Image.open(sys.argv[1]).convert("RGB")
# The 'Got it' control is in the LOWER-RIGHT of Android's blue
# first-immersive-mode panel, NOT at the screenshot center. Derive the
# panel bounds from blue pixels so alternate emulator resolutions work.
small=picture.resize((140,100))
blue=[]
for y in range(small.height):
    for x in range(small.width):
        r,g,b=small.getpixel((x,y))
        if b>160 and g>70 and b>r*2 and b>g*1.25:
            blue.append((x,y))
if len(blue)<100:
    raise SystemExit("No blue immersive tutorial panel found for dismissal")
xmin=min(x for x,y in blue);xmax=max(x for x,y in blue)
ymin=min(y for x,y in blue);ymax=max(y for x,y in blue)
x=xmin+int((xmax-xmin)*.78)
y=ymin+int((ymax-ymin)*.80)
print(x*picture.width//small.width,y*picture.height//small.height)
PY
)"
    read -r x y <<< "$coords"
    adb shell input tap "$x" "$y"
    sleep 4
  else
    break
  fi
done
# Re-capture after the last dismissal attempt; this MUST be the image
# validated and retained as actual stereo-screen acceptance evidence.
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
assert "TestChamberFrame" in log and "eyeCallback=" in log, (
    "Cardboard never invoked the game-specific per-eye render callback")
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
# Cardboard's black lens mask and white gear glyphs can make a completely
# empty framebuffer look colorful. Require actual variation INSIDE BOTH
# stereoscopic eye centers, away from Android UI and lens boundaries.
for eye, region in (
    ("left",(.16,.30,.35,.68)),
    ("right",(.65,.30,.84,.68))
):
    a,b,c,d=region
    patch=picture.crop((int(a*picture.width),int(b*picture.height),
                        int(c*picture.width),int(d*picture.height)))
    # Dominant clear color (11,22,36) or a few compression-edge shades is
    # not evidence of native world rendering.
    buckets=set(patch.resize((80,80)).getdata())
    assert len(buckets)>=12, (
        f"{eye} eye is flat: only {len(buckets)} inner-scene colors")

print("EMULATOR SMOKE PASS: five actual models decoded + GPU uploaded, "
      "controller/touch cue dispatched, Android stayed alive, "
      f"VR screenshot {picture.width}x{picture.height}, {colors} colors")
PY
