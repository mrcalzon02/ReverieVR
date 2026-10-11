# Original Steam Controller (2015) — Android input investigation

**Status:** Android HID inspection utility committed; not yet connected to UI or validated on handset. The original controller's full native reports have not been decoded in ReverieVR.

## Reference hardware evidence (Galaxy S9, user observations, 2026-10-10)
- Bluetooth pairing accepted with original Steam Controller.
- Android displayed “Configure physical keyboard”.
- In Firefox the right trackpad moved the system pointer.
- Trigger input acted as a mouse click; clicking the right trackpad did not register the click in that test.
- These facts do **not** establish complete gamepad/dual-trackpad/gyro/paddle support.

## Upstream protocol resources
- Linux GPL-2.0+ hid-steam driver: https://github.com/torvalds/linux/blob/master/drivers/hid/hid-steam.c — documents 2015 controller mouse/keyboard “lizard mode” and raw HID support. **Reference only**; do not copy GPL-licensed source into the application without licensing review.
- Original Steam Controller driver and documented event layout: https://github.com/cvuchener/steamcontroller-linux-kernel — stick, left/right pads, trigger axes, grips, gyro and accelerometer.
- SDL-derived original-controller Android work (marked untested): https://github.com/MrCloudy2/moonlight-android-steam-controller — compatibility research, not verified support.
- https://github.com/SonicDX12/SteamController-Android is specific to **2026 Ibex**; explicitly excludes the original 2015 controller. Do not use its report layout for this device.

## Implemented source
`app/src/main/java/io/github/mrcalzon02/reverievr/input/HidInputInspector.java`: non-invasive Android `InputDevice` enumeration and button/motion reports; read-only and designed to avoid logging text typed on physical keyboards.

## Integration steps
1. Connect inspector to an explicit developer diagnostic panel and lifecycle-safe device-added/removed/rescan controls; do not log every frame or persist events by default.
2. Hook key/motion dispatch (without consuming ordinary Android events) and display controller-specific event reports with device ID and source.
3. For observed lizard-mode controls, preserve normal Android mouse behavior and record supported buttons/axes. Do not infer trackpad click semantics from visual layout.
4. Only if needed, add optional original-controller vendor HID/BLE GATT report backend with pairing, permissions and report validation, plus transition back to safe default mode on detach/error.
5. Normalize complete physical inputs into module profiles; explicitly handle double input if default HID mouse and raw reports coexist. Keep essential shell recovery outside remappings.
6. Validate S9 pairing/reconnect, independent pads, touch/contact state, triggers, paddles, thumbstick, menu/face keys, gyro, battery if available, and haptics separately. No receiver is required for the observed Bluetooth pairing.

## Acceptance
Not complete until built, diagnostic events shown on a real S9, and behavior verified across the application, DOS host and native modules. No placeholder UI toggle or claims of raw driver access before actual support.
