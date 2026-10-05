# Building ReverieVR

## Bootstrap toolchain

ReverieVR now combines:

- native Java/XML Android Stage A;
- direct Android BLE Daydream-controller backend;
- Google Cardboard SDK v1.35.0 pinned at commit `5969239e7c87f4cd64c8ec170ce1e7f4eb559e37`;
- OpenGL ES Cardboard rendering only.

Pinned build baseline:

- Android Gradle Plugin: **9.2.0**
- Gradle: **9.6.1**
- JDK: **17**
- compileSdk: **36**
- targetSdk: **36**
- minSdk: **26**
- Cardboard NDK: **29.0.14206865**
- CMake: **3.22.1+**

Cardboard Vulkan and Unity-plugin build paths are explicitly disabled.

## Clone

The Cardboard SDK is a pinned Git submodule. Clone recursively or initialize it after cloning:

    git submodule update --init --recursive

Verify:

    git -C third_party/cardboard rev-parse HEAD

Expected:

    5969239e7c87f4cd64c8ec170ce1e7f4eb559e37

## DOSBox Pure source

The DOS runtime is pinned separately from the Cardboard submodule.

Fetch and verify the accepted DOSBox Pure baseline:

    ./scripts/fetch-dosbox-pure.sh

Windows:

    scripts\fetch-dosbox-pure.bat

Expected commit:

    a4a0bab7f8931433588f2fcad9045c85b277373d

The checkout lives at `third_party/dosbox-pure/src` and is intentionally ignored by ReverieVR git. The committed pin/provenance record is `third_party/dosbox-pure/README.md`.

The normal Android build does not yet require this checkout until the native libretro frontend is activated. Once DOSBox Pure binaries are distributed, GPL corresponding-source/rebuild material becomes a release acceptance requirement.

## Local setup

Install:

1. JDK 17.
2. Android SDK Platform 36 and Build Tools 36.x.
3. Android NDK 29.0.14206865.
4. CMake 3.22.1 or compatible newer installed through Android SDK Manager.
5. No system Gradle installation is required.

ReverieVR commits the Gradle 9.6.1 wrapper. The wrapper JAR is pinned to Gradle's published SHA-256:

    497c8c2a7e5031f6aa847f88104aa80a93532ec32ee17bdb8d1d2f67a194a9c7

The downloaded Gradle 9.6.1 binary distribution is pinned to:

    9c0f7faeeb306cb14e4279a3e084ca6b596894089a0638e68a07c945a32c9e14

After cloning and initializing submodules, verify the bootstrap:

    ./scripts/verify-build-bootstrap.sh

Then test and build both applications:

    ./gradlew test :app:assembleDebug :controller-app:assembleDebug

Expected debug APKs:

    app/build/outputs/apk/debug/app-debug.apk
    controller-app/build/outputs/apk/debug/controller-app-debug.apk

Install the headset application on the Galaxy S9:

    adb install -r app/build/outputs/apk/debug/app-debug.apk

Install the optional companion controller on a second Android phone:

    adb install -r controller-app/build/outputs/apk/debug/controller-app-debug.apk

On Windows, use:

    gradlew.bat test :app:assembleDebug :controller-app:assembleDebug

The two APKs have different application IDs:

- headset: `io.github.mrcalzon02.reverievr`
- companion: `io.github.mrcalzon02.reverievr.controller`

GitHub Release assets must keep unambiguous package names. The headset updater only accepts assets named `ReverieVR.apk` or beginning with `ReverieVR-` and explicitly rejects names containing `controller`. Recommended release names:

    ReverieVR-v<version>.apk
    ReverieVR-Controller-v<version>.apk

## scrcpy diagnostic workflow

scrcpy is an optional developer tool, not a ReverieVR runtime dependency.

For DOS keyboard/mouse validation, current scrcpy supports physical-HID presentation:

    scrcpy --keyboard=uhid --mouse=uhid --stay-awake --no-audio

or the short form:

    scrcpy -KM --stay-awake --no-audio

UHID is preferred for DOS because the mouse is relative/captured like a physical HID mouse. The ordinary scrcpy SDK keyboard/mouse modes are also supported where Android delivers their injected key/mouse events, but SDK mouse input is absolute rather than a true captured relative mouse.

Requirements:

- enable Android USB debugging on the Galaxy S9;
- keep the phone connected over USB for the standard reference workflow;
- keep the physical phone display **on** while the device is in the Daydream View;
- do not use scrcpy's `--turn-screen-off` option during VR testing;
- do not treat scrcpy mirroring/recording sessions as performance or thermal acceptance evidence.

ReverieVR provides two convenience launchers:

    ./scripts/scrcpy-reverie.sh

or on Windows:

    scripts\scrcpy-reverie.bat

Both use:

    scrcpy --stay-awake --no-audio

and pass through any additional scrcpy arguments supplied by the developer.

`--no-audio` is intentional for the Galaxy S9 Android-10-era reference path: current scrcpy only forwards device audio on Android 11 or newer. Validate ReverieVR audio from the phone/headphones instead of treating silent host mirroring as an application failure.

Useful optional examples:

    ./scripts/scrcpy-reverie.sh --show-touches
    ./scripts/scrcpy-reverie.sh --record=reverie-debug.mkv

The mirror is intended for Stage A remote control, Stage B observation, recordings, UI review, crash reproduction, and controller/calibration debugging. Daydream BLE remains the authoritative headset input path.

## Reference-device validation order

On the Galaxy S9:

1. launch Stage A;
2. grant Bluetooth permissions;
3. wake the Daydream controller and run Pair / Sync;
4. verify Android bond/GATT connection;
5. run the live controller input test;
6. verify touchpad, click, Menu, Home, volume-button bits and controller battery;
7. return to Stage A and validate the alternate input sources:
   - install the ReverieVR Controller companion APK on a second Android phone, pair both phones in Android Bluetooth settings, start the companion server, tap **Use paired phone as controller**, select that phone, and verify orientation, touch/click, App/Home, physical volume forwarding, reconnect behavior, and real companion-phone battery telemetry;
   - optionally repeat with a historical compatible Daydream controller-emulator app and verify the same headset-side RFCOMM receiver works while battery correctly remains unknown;
   - connect a standard Bluetooth or USB Android gamepad and verify Stage A reports it as ready without pretending controller battery telemetry exists;
8. tap **Enter VR**;
9. verify stereo distortion/head tracking;
10. verify setup schema v2 appears and all six pages are readable;
11. on the controller familiarization page, verify the active source and last-action display update for Select, Back, Recenter, directional input and volume where supported;
12. verify Back is captured on the familiarization page instead of navigating away, then repeat the familiarization test with each available input source;
13. verify head gaze + Select navigation using each available input source;
14. verify the global PHONE and CTRL battery progress bars remain visible in Home, setup and video playback when Battery HUD is enabled; verify unknown controller battery stays empty/`--` rather than reading 0%;
15. toggle Show Percentages and confirm the bars remain while the numeric values appear/disappear;
16. enable Look-up Reveal, look steeply upward, and verify the compact upper-right HUD drops lower into view; disable Look-up Reveal and confirm the HUD stays fixed;
17. verify Daydream/phone-emulator Home and generic-gamepad Start/Mode recenter software heading;
18. verify App/B/Back returns/backtracks and Exit returns to Stage A;
19. from Stage A, choose a known-good local video through the Android document picker;
20. verify the selected URI survives leaving and returning to Stage A;
21. enter VR and launch the selected video in **Flat cinema screen** mode;
22. verify Select toggles play/pause, Daydream/phone-emulator horizontal swipes or generic-gamepad left/right navigation seek by 10 seconds, and Back returns to the VR home without killing the shell;
23. repeat with a known-good mono equirectangular 360° video and verify head-look orientation, seam placement, and absence of horizontal mirroring;
24. background/resume ReverieVR during playback and verify the decoder reattaches if Android recreates the GL surface;
25. confirm swipe/axis polarity matches left = backward and right = forward; if hardware reports the opposite X orientation, capture that as a controller-axis defect rather than silently swapping protocol semantics;
26. start the scrcpy helper over USB and verify Stage A mouse/keyboard/touch control;
27. keep scrcpy running, enter Stage B, and verify stereo/head-tracked output remains observable while Daydream BLE input continues working;
28. confirm the phone display remains illuminated in the headset and that USB debugging does not disturb controller pairing;
29. where useful, capture a short scrcpy recording for defect evidence;
30. close scrcpy before sustained performance/thermal measurements;
31. run a sustained playback session without scrcpy and record frame pacing, decoder failures, battery drain, temperature/throttling, and audio/video sync.

Do not mark RV-0003, RV-0006, RV-0091, RV-0092, RV-0100, RV-0102, RV-0500 or RV-0501 device-accepted until the relevant checks have actually run.


## Logging and diagnostic acceptance

ReverieVR has two logging modes.

- **Standard** is suitable for ordinary use and records incidents plus major milestones.
- **Development** intentionally records high-frequency diagnostics including input, binding and per-frame VR state.

Development logging is diagnostic evidence, **not performance evidence**. Do not use a Development-logging run for frame-pacing, battery or thermal acceptance.

Before a device-debug session:

1. select Development logging in Stage A;
2. reproduce the issue;
3. return to Stage A if possible;
4. choose **Export diagnostic bundle**;
5. save the ZIP through Android's document provider;
6. review it before sharing or attaching it to an issue;
7. return logging to Standard before performance testing.

Fatal/unhandled exceptions are captured locally automatically. Nothing is uploaded automatically.

Validate log rotation during a long Development session and confirm the application remains bounded in memory/storage.

## Retro framebuffer presentation validation

Treat guest/source resolution separately from the Cardboard eye render target.

For hosted retro content, validate at least:

- 320x200 displayed with intended 4:3 geometry;
- 320x240 square-pixel 4:3;
- 640x350 and 640x400 with intended 4:3 geometry;
- 640x480 square-pixel 4:3;
- 720x400 text mode readability;
- an unknown/custom source mode falling back to native aspect.

Where integer scaling fits the intermediate guest surface, verify nearest/integer presentation remains pixel-stable. The VR shell/HUD should remain readable and independent of guest resolution.

Do not hard-code a generic 1.4x distortion factor or arbitrary 640x720/800x900/1280x1024 per-eye buffer. Cardboard remains authoritative for eye viewports/projection/distortion; render-scale changes require measured Galaxy S9 evidence.


## Bundled DOOM Shareware v1.9

Free/noncommercial ReverieVR releases include the original DOOM Shareware v1.9
archive by default.

Fetch and checksum-verify it before a release build:

    ./scripts/fetch-doom-shareware.sh

Windows:

    scripts\fetch-doom-shareware.bat

Expected file:

    third_party/doom-shareware/payload/default-content/doom19s.zip

Expected size:

    2450688 bytes

Expected SHA-256:

    cacf0142b31ca1af00796b4a0339e07992ac5f21bc3f81e7532fe1b5e1b486e6

A normal release build intentionally fails when the default shareware payload is
missing.

To create a distribution that deliberately omits DOOM Shareware:

    ./gradlew :app:assembleRelease -PexcludeDoomShareware

or on Windows:

    gradlew.bat :app:assembleRelease -PexcludeDoomShareware

Use the exclusion path for a commercial/paid distribution unless appropriate
shareware redistribution permission has been obtained for that distribution.

Do not replace the expected archive with:

- DOOM.WAD;
- DOOM2.WAD;
- Ultimate DOOM data;
- another commercial IWAD;
- an independently repacked shareware directory.

On-device validation must confirm that the packaged asset is re-verified,
registered as **DOOM Shareware v1.9**, and reaches the DOSBox Pure installer /
autostart flow without network access.
