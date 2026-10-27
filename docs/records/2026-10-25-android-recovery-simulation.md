# RV-REC-02/03 — Android recovery simulation and capture laboratory

**Scope:** Real Android emulator and Java policy execution; simulated Daydream controller input, not real Daydream hardware.
**Repository:** `mrcalzon02/ReverieVR` `main`; [recovery issue #6](https://github.com/mrcalzon02/ReverieVR/issues/6).
**Workflow:** [ReverieVR Android Recovery Simulation + Capture](../../.github/workflows/reverie-android-recovery-simulation.yml).
**Entry:** `workflow_dispatch` or a scoped source/workflow edit. No phone-test publication trigger and no automatic new APK release.

## Intent

Reuse the existing, already-proven Android 10 Google APIs x86_64 emulator/NDK/GLES2 path to reproduce recovery failures before transferring an APK to the physical Galaxy S9. The workflow must be deterministic, must preserve separate provenance for **simulated** input and **actual** Android execution, must capture inspectable evidence, and must fail on any missing assertion rather than treating a screenshot or clean Gradle task as a passing user journey.

## Executed architecture

1. **Build two actual debug APKs from the same Git SHA** under the existing `app/build.gradle` configuration and x86_64 emulator-native libraries, with synthetic build epochs `1760000000` and `1760000600`, resulting in explicit Android versionCodes `182163200` and `182163800`. Verify both APKs with Android's `apksigner` and check their signing-cert SHA-256 fingerprints are exactly equal. This checks package replacement rules, **not** continuity of the published permanent phone-test signer.
2. Install the older APK and the Android instrumentation APK on a clean Android 10 emulator using actual `adb install` and Android PackageManager. Write sentinel data in *the target app's own* `SharedPreferences` **and** private files via installed instrumentation. Test against the real `GazeFallbackPolicy` and `ControllerPoseVisibility` classes: headset-only, linked acquisition, 800 ms pose jitter, held noninteractive mesh, 30-second stall, disconnect 4-second grace, reconnect suppression, intentional gaze, plus 5,000 simulated frames with repeated missing BLE pose notifications.
3. Capture old-version Android `MainActivity` framebuffer via real `screencap`, and package/activity dumps. Install the higher-version APK **over** the old installation using `adb install -r` without clearing data. Verify both private file and preferences remained intact through the actual app process and byte-for-byte file comparison.
4. Reuse the existing `HubGameModulesInstrumentedTest#allRegisteredGamesAcceptIsolatedAcquisitionUpdateRollback` to exercise **real** `RvPackInstaller` version 1 -> version 2 -> rollback across registered game modules. Reuse `DownloadedNativeGameAssetsInstrumentedTest#omegaFrontReallyTravelsDeploysOrdersAndRestoresInGles` to render a native GLES2 tactical PNG, then collect that real framebuffer. Capture new-version Stage-A screencap and actual logcat.
5. Publish a 14-day downloadable Actions **artifact** containing `capture-manifest.json`, three PNG images, input-state timeline CSV, installation/instrumentation transcripts, two package dumps, signer/version metadata, SHA256 checksums, and logcat. Do not upload duplicate ~140MB development APKs when source, checksums and install test evidence suffice.

## Exact assertions

- Two debuggable x86_64 APKs built and have distinct ordered versionCodes but the same debug signing certificate.
- Older and newer APK installed with Android's actual PackageManager, package persisted, and post-update versionCode read back from `dumpsys package`.
- Each named instrumentation test produces `OK (1 test)`, no silent JUnit failure.
- All 13 discrete controller/gaze policy transitions pass; 5,000 additional jitter frames produce *zero unintended gaze activations*, including stale/non-clickable poses.
- Actual target-app `SharedPreferences` and private file survive reinstall (not reset and not manually migrated).
- Isolated real module upgrade and rollback pass.
- Stage-A before/after and native Omega Front GLES2 PNG screenshot signatures validated; artifact retains sha256 and logs.
- An error in any test or unsupported screenshot/installer path fails the workflow rather than inventing an acceptance status.

## Explicit limitations / subsequent integration

This CI trace feeds **synthetic** pose-valid/connected/timestamps to production policy classes. It does not generate physical Bluetooth HCI traffic, phone IMU motion, camera-lens stereo images, home-screen gaze-ray pixel trajectories, Daydream clickable touchpad input, electrical disconnects or user-visible perceptual flicker. The Android keyboard/DPAD is not a substitute for a Daydream wand. Screenshot frames come from Stage A and a native offscreen GLES framebuffer, **not optical through-lens captures**.

The APK upgrade uses two same-source **debug** APKs sharing the CI runner's debug key to exercise Android replacement/data-preservation. It does not assert the production GitHub UpdateChecker chose a release, downloaded its APK, or accepted the persistent phone-test certificate. The actual signed `phone-test-1411-1` release at `57f0182` was separately published with the expected lineage; an on-device update through the application's UI is still an independent RV-REC-03 gate.

**Next engineering extensions, contingent on passing this lab:** integrate synthetic controller transition injection *through the actual renderer's input bus* (not only policy calls), verify ray visibility/no activation in captured VR Home frames, automate production-signer in-place upgrade with compatible ARM emulator/system images or owned device farm, and run the reference Galaxy S9 with actual Daydream controller for lag, comfort and all in-headset input handoffs.

## Truth/evidence

The initial workflow and test files constitute a **committed implementation with a pending first Actions result**. A workflow URL or successfully constructed artifact cannot be called passing until the named run completes and the detailed transcript is checked. Store first passing or failing run URL, job id, exact source commit, capture artifact, remediation and the current device-state boundary in issue #6 and `docs/project/RECOVERY_DEVELOPMENT_OBJECT.md`.
