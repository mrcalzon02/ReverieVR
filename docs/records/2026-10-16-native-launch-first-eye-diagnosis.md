# 2026-10-16 — Galaxy S9 native-launch first-eye failure

**Source:** User-exported `beta-ReverieVR-diagnostics-20261016-124803.zip`, Android 10 / SM-G960U / arm64-v8a, build `0.1.0-dev`, `versionCode=214291878`.

## Reproduced symptoms

The installed build recorded `Started module=` and `Stage B launched module=` followed immediately by `In-headset launch failure id=... reason=` and `Native module stopped` for four independent modules: `classic-pong` (20:46:27.419–.422Z), `procedural-test-chamber` (20:46:43.181–.183Z), `between-deliveries-red-ledger` (20:46:46.433–.434Z), and `breakwater-battery` (20:46:48.947–.949Z). The export was truncated after four complete deflate log members and a partial fifth; the four launch attempts were in a recovered, complete member. Neither native-create admission nor selection/binding failed.

## Root cause and source/release distinction

Version code derives from the phone-test build epoch seconds minus 1577836800; `214291878` resolves to `2026-10-16T05:31:18Z`. Phone Test #284, released at 05:36:25Z from `02a64918beea2475e6f07fb9675dc3291c14c81d`, still has the unguarded `MODE_NATIVE` branch in `VrShellRenderer.onDrawEyeInternal`. It sets `rendered=false` whenever `nativeSurfaceReady=false`, even in the selection frame where `onNewFrame` has not initialized the new module's GL surface. It then stops the module and logs an empty error because no JNI guest render callback ran. This exactly explains the same-millisecond, empty-reason pattern.

The newer authoritative main branch already includes the correct pending-first-eye guard: `if (!nativeSurfaceReady) { ... return; }` **before** calling `nativeModuleRuntime.renderEye`, while retaining fatal reporting for real GL/context/guest failures. The source regression guard `scripts/verify-native-launch-handoff.py` enforces the intended sequencing. The user-installed #284 does not contain that correction, regardless of newer `main` source or source-only checks.

## Required acceptance

- Publish and read back a signed phone-test prerelease built from a main commit **containing** the pending-first-eye guard and existing regression test. The pre-existing explicitly authorized phone-test release pipeline is the permitted build path; do not add a new workflow.
- Check package versionCode > `214291878`, same package/signing identity, APK assets, checksum and Git revision.
- On physical Galaxy S9/Daydream: launch Classic Pong, Test Chamber, Red Ledger, and Breakwater; verify actual **first left/right eye frames**, sustained display, controller input and clean exit/re-entry. The signed build alone does not establish device playability.
- For future failure capture, preserve `GL initialization` versus `guest eye render` distinctions and the underlying error; never classify the pending surface as a crash or silently bypass genuine module failures.

**Status at diagnosis:** User device reproduction confirmed; source fix present on main; corrected installable APK and hardware retest required.
