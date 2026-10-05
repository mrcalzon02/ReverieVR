# scrcpy Development Workflow

ReverieVR supports scrcpy as a developer diagnostic surface.

It is intentionally **not** required by the APK and should never become a hidden runtime dependency.

## Quick start

Enable USB debugging on the Android phone, connect it over USB, then run:

    ./scripts/scrcpy-reverie.sh

Windows:

    scripts\scrcpy-reverie.bat

The helpers execute:

    scrcpy --stay-awake --no-audio

and forward additional arguments.

Examples:

    ./scripts/scrcpy-reverie.sh --show-touches
    ./scripts/scrcpy-reverie.sh --record=reverie-debug.mkv
    scripts\scrcpy-reverie.bat --record=reverie-debug.mkv

## Why no audio?

The Galaxy S9 reference software may be Android 10. scrcpy audio forwarding requires Android 11 or newer.

Validate ReverieVR audio through the phone, wired headphones, Bluetooth headphones, or another device-side route.

## VR-specific rule

Never use:

    --turn-screen-off

while the phone is inside the headset. The phone panel is the headset display.

## What scrcpy should prove

A compatibility pass should demonstrate:

- Stage A can be navigated from the desktop;
- Bluetooth pairing/controller status remains visible;
- Daydream BLE continues working while ADB is active;
- Enter VR still transitions into Stage B;
- both stereo eye images remain visible in the mirror;
- media playback can be observed/recorded;
- leaving VR returns to the normal Stage A surface.

## What scrcpy should not prove

Do not use a scrcpy session as final evidence for:

- sustained frame-rate acceptance;
- thermal acceptance;
- battery-drain acceptance;
- encoder contention;
- end-to-end headset latency.

Close scrcpy and repeat those measurements directly on the phone.

## Troubleshooting

If `scrcpy` is not found, install scrcpy separately and ensure it is available on PATH.

If the phone is not visible:

    adb devices

Confirm the device is authorized for USB debugging.

If Stage A mirrors but desktop input does not work, verify Android's USB-debugging authorization and test direct physical touch. Do not add special app permissions merely to make scrcpy inject input.

If the scrcpy window has no sound on Android 10 or earlier, that is expected.

If Stage B is black only in scrcpy while the headset display is correct, capture that as an RV-0006 mirror-path defect; do not disable Cardboard rendering or add a fake non-VR fallback merely to satisfy the mirror.
