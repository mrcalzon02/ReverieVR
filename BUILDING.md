# Building ReverieVR

## Bootstrap toolchain

The current Stage A Android shell intentionally uses only the platform Android SDK and Java.

Pinned bootstrap versions:

- Android Gradle Plugin: 9.4.1
- Gradle: 9.6.1
- JDK: 17
- compileSdk: 36
- targetSdk: 36
- minSdk: 26
- external runtime libraries: none

The project does not currently require AndroidX, Compose, a game engine, Google VR Services, or network permission.

## First local setup

Install:

1. JDK 17.
2. Android SDK Platform 36 and Build Tools 36.x.
3. Gradle 9.6.1.

From the repository root:

    gradle :app:assembleDebug

The debug APK is expected at:

    app/build/outputs/apk/debug/app-debug.apk

Install to an attached Android device with:

    adb install -r app/build/outputs/apk/debug/app-debug.apk

## Gradle wrapper

The repository bootstrap does not yet contain a generated Gradle wrapper binary. Once a verified Gradle 9.6.1 environment is available, generate and commit the standard wrapper with:

    gradle wrapper --gradle-version 9.6.1 --distribution-type bin

Then subsequent builds should use:

    ./gradlew :app:assembleDebug

Do not substitute an unverified third-party wrapper binary.

## Current runtime boundary

The Stage A touchscreen activity is implemented independently of Stage B VR.

Enter VR is deliberately disabled until RV-0002/RV-0091/RV-0092 establish and verify:

- the VR renderer/runtime;
- Daydream controller discovery/pairing/input;
- the 2D-to-VR transition;
- the recovery transition back to Stage A.

This is capability honesty, not a placeholder success path.
