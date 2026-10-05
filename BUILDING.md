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

Then test and build:

    ./gradlew test :app:assembleDebug

Expected debug APK:

    app/build/outputs/apk/debug/app-debug.apk

Install:

    adb install -r app/build/outputs/apk/debug/app-debug.apk

On Windows, use `gradlew.bat test :app:assembleDebug`.

## Reference-device validation order

On the Galaxy S9:

1. launch Stage A;
2. grant Bluetooth permissions;
3. wake the Daydream controller and run Pair / Sync;
4. verify Android bond/GATT connection;
5. run the live controller input test;
6. verify touchpad, click, Menu, Home, volume-button bits and controller battery;
7. tap **Enter VR**;
8. verify stereo distortion/head tracking;
9. verify the first-run VR setup page is readable;
10. verify head gaze + touchpad click navigation;
11. verify Home recenters controller/software heading;
12. verify Menu returns/backtracks and Exit returns to Stage A;
13. from Stage A, choose a known-good local video through the Android document picker;
14. verify the selected URI survives leaving and returning to Stage A;
15. enter VR and launch the selected video in **Flat cinema screen** mode;
16. verify Daydream click toggles play/pause and Menu/back returns to the VR home without killing the shell;
17. repeat with a known-good mono equirectangular 360° video and verify head-look orientation, seam placement, and absence of horizontal mirroring;
18. background/resume ReverieVR during playback and verify the decoder reattaches if Android recreates the GL surface;
19. run a sustained playback session and record frame pacing, decoder failures, battery drain, temperature/throttling, and audio/video sync.

Do not mark RV-0003, RV-0091, RV-0092, RV-0100, RV-0500 or RV-0501 device-accepted until the relevant checks have actually run.
