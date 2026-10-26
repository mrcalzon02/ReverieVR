#!/usr/bin/env bash
# Called inside reactivecircus/android-emulator-runner's Android 10 AVD.
# REAL Android PackageManager + renderer/JNI, SYNTHETIC controller trace.
# No physical Daydream BLE/IMU, stereoscopic optics or release-feed download.
set -euo pipefail
PKG=io.github.mrcalzon02.reverievr
RUNNER="$PKG.test/androidx.test.runner.AndroidJUnitRunner"
TEST="$PKG.RecoverySimulationInstrumentedTest"
OUT=validation/recovery-simulation
OLD="$OUT/ReverieVR-debug-old-x86_64.apk"
NEW="$OUT/ReverieVR-debug-new-x86_64.apk"
TEST_APK="$OUT/ReverieVR-android-tests.apk"
mkdir -p "$OUT"
test -s "$OLD" && test -s "$NEW" && test -s "$TEST_APK"

run_test() {
  local klass="$1" method="$2" name="$3"
  adb shell am instrument -w -r -e class "$klass#$method" "$RUNNER" |
    tee "$OUT/$name.txt"
  grep -F 'OK (1 test)' "$OUT/$name.txt"
}
capture() {
  local phase="$1"
  adb shell am start -W -n "$PKG/.MainActivity" |
    tee "$OUT/$phase-launch.txt"
  sleep 2
  adb exec-out screencap -p > "$OUT/$phase-screen.png"
  adb shell dumpsys activity activities > "$OUT/$phase-activities.txt"
  adb shell dumpsys package "$PKG" > "$OUT/$phase-package.txt"
  python3 - "$OUT/$phase-screen.png" <<'PY'
from pathlib import Path
import sys
b=Path(sys.argv[1]).read_bytes()
assert len(b)>300 and b.startswith(bytes((137,80,78,71,13,10,26,10))), "Android screenshot missing or invalid"
print("PASS actual Android emulator framebuffer:",sys.argv[1],len(b),"bytes")
PY
}
adb logcat -c
adb install -r "$OLD" | tee "$OUT/old-install.txt"
adb install -r "$TEST_APK" | tee "$OUT/instrumentation-install.txt"
grep -Fx 'Success' "$OUT/old-install.txt"
run_test "$TEST" writeUpgradeSentinel old-private-data-written
run_test "$TEST" controllerDropoutReconnectAndGazeCapture controller-pose-trace
adb exec-out run-as "$PKG" cat files/recovery-controller-trace.csv > "$OUT/controller-pose-trace.csv"
adb exec-out run-as "$PKG" cat files/recovery-upgrade-sentinel.txt > "$OUT/private-data-before.txt"
test -s "$OUT/controller-pose-trace.csv"
grep -F 'jitter_5000_frames_stale_count' "$OUT/controller-pose-trace.csv"
capture before-upgrade
grep -Eq 'versionCode=182163200([^0-9]|$)' "$OUT/before-upgrade-package.txt"

adb shell am force-stop "$PKG"
adb install -r "$NEW" | tee "$OUT/new-install.txt"
grep -Fx 'Success' "$OUT/new-install.txt"
run_test "$TEST" verifyUpgradeSentinel new-private-data-preserved
adb exec-out run-as "$PKG" cat files/recovery-upgrade-sentinel.txt > "$OUT/private-data-after.txt"
cmp "$OUT/private-data-before.txt" "$OUT/private-data-after.txt"
run_test "$PKG.HubGameModulesInstrumentedTest" allRegisteredGamesAcceptIsolatedAcquisitionUpdateRollback module-update-rollback
run_test "$PKG.DownloadedNativeGameAssetsInstrumentedTest" omegaFrontReallyTravelsDeploysOrdersAndRestoresInGles native-omega-gles-capture
adb exec-out run-as "$PKG" cat files/omega-front-ci-tactical.png > "$OUT/native-omega-tactical.png"
capture after-upgrade
grep -Eq 'versionCode=182163800([^0-9]|$)' "$OUT/after-upgrade-package.txt"
adb logcat -d -v time > "$OUT/android-logcat.txt"

python3 - "$OUT" <<'PY'
import hashlib,json,sys
from pathlib import Path
p=Path(sys.argv[1])
images=['before-upgrade-screen.png','after-upgrade-screen.png','native-omega-tactical.png']
for name in images:
    b=(p/name).read_bytes()
    assert len(b)>300 and b.startswith(bytes((137,80,78,71,13,10,26,10))),f"Missing actual PNG: {name}"
trace=(p/'controller-pose-trace.csv').read_text()
assert trace.count('TRACE ') == 0
assert 'jitter_5000_frames_stale_count,' in trace
assert trace.count('\n')>=15
assert (p/'private-data-before.txt').read_bytes()==(p/'private-data-after.txt').read_bytes()
assert 'PASS controller gaze trace' in (p/'android-logcat.txt').read_text()
result={'lab':'ReverieVR Android Recovery Simulation',
        'android_api':29,'emulator_arch':'x86_64',
        'update':'real PackageManager in-place debug APK replacement',
        'old_version_code':182163200,'new_version_code':182163800,
        'private_preferences_and_file':'instrumentation-verified',
        'controller_input':'synthetic policy timeline, not physical Daydream BLE',
        'game_revision':'real RvPackInstaller revision/rollback test',
        'visual_capture':'actual emulator Stage-A screenshots and GLES2 offscreen render',
        'production_phone_test_signer':'not exercised (debug signer only)',
        's9_headset_acceptance':'not tested',
        'captures':{name:{'size_bytes':(p/name).stat().st_size,
            'sha256':hashlib.sha256((p/name).read_bytes()).hexdigest()} for name in images}}
(p/'capture-manifest.json').write_text(json.dumps(result,indent=2)+'\n')
print('PASS: Android emulator upgrade, retained private state, 5,000-frame gaze simulation, real module rollback and GLES2 capture')
PY
sha256sum "$OUT"/*.png "$OUT"/*.apk > "$OUT/SHA256SUMS.txt"
