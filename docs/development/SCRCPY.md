# scrcpy Development Workflow

ReverieVR supports scrcpy as a developer diagnostic surface.

It is intentionally **not** required by the APK and should never become a hidden runtime dependency.

**Single-APK phone-first rule:** ReverieVR's entire core VR runtime and
hosted first-party games remain local to the phone. scrcpy is only an
optional desktop utility for debugging, mirroring, or forwarding
PC-connected input. There is no required desktop ReverieVR runtime, and
the phone must still launch, navigate by gaze, and run local media and
native games without any computer present. Any future PC-side read-only
optical-drive/file-sharing helper is an optional accessory used only
while that peripheral is needed; it cannot replace local emulator
execution or force a second Android app.

Ordinary phone/laptop Wi-Fi or a hotspot supports local application
networking regardless of internet connectivity, but this does **not**
backport Android 11's protected wireless ADB pairing to Android 10.
Wireless scrcpy on the reference Galaxy S9 may still need USB-authorized
ADB/TCP setup. A non-ADB phone-to-PC file/input bridge would have a
separate, explicitly authenticated local protocol, not fictitious USB
hub pass-through.

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

## Laptop controllers as a selectable VR input source

The controller connected to the **laptop** can be forwarded as a real Android HID gamepad over scrcpy's optional UHID gamepad transport. This works for wired USB debugging or a previously authorized wireless ADB session:

    scripts\scrcpy-reverie.bat --gamepad=uhid

Linux/macOS:

    ./scripts/scrcpy-reverie.sh --gamepad=uhid

For wireless use, establish a trusted ADB-over-Wi-Fi connection first and run the same scrcpy helper with `--gamepad=uhid`. The scrcpy UHID option depends on a recent scrcpy release, and Android reports its Xbox-360-compatible identity; it is **not** detectable as scrcpy purely from that generic identity. ReverieVR recognizes this identity as eligible **only when SCRCPY / LAPTOP has been explicitly selected**.

While wearing the headset, open **VR Home → Controller → CONTROLLER SOURCE**, then choose **SCRCPY / LAPTOP**. The option rejects activation until a compatible forwarded input device has appeared on Android. The other choices are **AUTO**, **DAYDREAM**, **STEAM CONTROLLER**, **ANDROID USB / BT** (ordinary on-phone HID gamepads), and **PHONE CONTROLLER** (paired via Stage A). Changing controller source updates persistent preferences, makes that source the primary controller for VR shell/game actions, resets previously held keys and tracked pointer pose, and never requires you to leave VR when a compatible controller is already present.

Daydream selection can initiate pairing while leaving the previous controller active until the BLE connection is ready; unavailable phone-controller pairing returns to Stage A rather than presenting a nonfunctional choice. If a selected scrcpy/Steam/Android controller disconnects, VR input switches back to Auto, with a best-effort Daydream reconnection if permissions are already available. The headset's Back menu and Stage A remain recovery paths.

**No Windows USB hubs or optical drives are magically passed through by scrcpy.** A laptop optical drive needs a separate explicit read-only bridge for game-image transfer; the emulator itself handles virtual ISO/CD media and memory cards. A keyboard plugged into the laptop and forwarded as UHID continues to use the normal on-desk keyboard twin rather than being confused with the selected gamepad.

## Concurrent PC workload and screen sharing

The PC is not a ReverieVR server or a VR graphics coprocessor. It may be
running voice chat (Discord/TeamSpeak), music, video rendering/transcoding,
disk-intensive jobs, or unrelated software. ReverieVR neither inspects nor
controls that work. Keep the VR stereo render loop wholly on Android and
independent of PC frame timing, host bandwidth, or availability.

Use scrcpy **only when the user wants external phone-to-PC display mirroring
or PC-forwarded inputs**. It already has a dedicated input-forwarding
route; ReverieVR should not invent a required second app/daemon to reproduce
that ability. A mirroring session may consume encoder, network and CPU/GPU
resources on both devices, so turn it off when unnecessary and measure
headset performance without it. Input events must not queue behind stream
frames or any optical-drive transfer.

A future PC-to-phone/window display feed would be a separate, opt-in
local-only feature, requiring explicit desktop capture authorization,
secure connection, independent latency/backpressure handling and a clear
stop action. Do not advertise it as implemented. A phone screen displayed
*inside* VR is a different local-only MediaProjection feature and must
never send screen pixels through ReverieVR's controller transport.
ReverieVR must never scan/OCR or analyze either display source.
See `docs/records/ADR-0023-OPTIONAL-PC-INTEROPERABILITY-AND-RESOURCE-ISOLATION.md`.

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
