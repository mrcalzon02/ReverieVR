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

## 3D model sourcing and interactive handset visualization (2026-10-10)
Valve publicly released original **2015** Steam Controller externally visible mechanical CAD geometry on 2016-03-24: https://steamcommunity.com/app/353370/announcements/ . The announcement offers ZIP/CAD and separate-part STL assets under Creative Commons terms, but explicitly states that commercial creations require contacting Valve. **Do not confuse this with Valve's 2026 Steam Controller CAD release.**

Decision: use Valve's original 2015 model as geometry reference. The downloaded March 2016 separate-parts archive has now been inspected: its exact license is **CC BY-NC-SA 4.0**, copyright 2016 Valve Corporation, with commercial products requiring contact with Valve. Archive/license SHA256 and source URLs are recorded in `docs/records/STEAM-CONTROLLER-MESH-PROVENANCE.md`. No Valve geometry is vendored into Git; use the explicit noncommercial local importer rather than silently bundling derivatives into general/commercial distributions.

Offline asset pipeline is implemented under `tools/steam_controller/`: exact welding, named QEM parts with explicitly reported voxel approximations where required, source/output hashes, topology/bounds reports, meter-space GLB/assembled OBJ, precomputed GLES2 positions/normals, previews and independent readback. The inspected handoff contains 22 parts / 11,882 triangles; seven parts retain reported topology flags. Original ZIP/CAD are preserved separately. The user requested a Google Drive package for the online instance to perform programming/hookup. Renderer/controller integration, exact Android report mappings and device/performance acceptance remain planned. The rendering approximation must not be represented as unchanged manufacturing CAD or as an independently authored substitute.

Interaction: tie material highlights and pad-contact markers to observed live controller event IDs (do not invent activity when HID events are absent); use neutral/unavailable visual states for unsupported sensors; reuse the existing controller model/hand-anchor abstraction rather than implementing a second avatar/input routing path. Add a side-by-side 3D controller diagnostic view when the model and report mappings are both verified.

## Stage A alternative-controller entry (2026-10-10 source integration)
The source now recognizes an Android-visible device whose name includes "Steam Controller" and which exposes mouse, keyboard or gamepad events as eligible for the same **Enter VR** readiness gate used by Daydream/gamepads. It also reports its connection in the VR controller status without claiming native gamepad axes. Normal Android mouse/keyboard event routing already exists in `VrActivity`; physical-controller action mapping inside the 3D shell is a separate acceptance test. A generic keyboard/mouse must not unlock entry. This check relies on Android's published device name and sources, **not** a dedicated raw BLE protocol driver or a proof that every key maps to a shell action.

Source edits: `AndroidGamepadSupport.java`, `MainActivity.java`, `VrActivity.java`. Source and remote commits were read back; **no Galaxy S9 device run or Android compilation was performed in this iteration**. Acceptance requires pairing Steam Controller without Daydream present, confirming enabled Enter VR, entering and exiting VR, working pointer/select and Quick Menu recovery, disconnect/reconnect status, and rejecting unrelated Bluetooth keyboards as VR controllers.

## Galaxy S9 Android 10 reference and controller selection (2026-10-10)

Verified reference handset screenshot: SM-G960U Galaxy S9, Android 10/API 29, One UI 2.5, build QP1A.190711.020.G960USQS8FTK4, kernel 4.9.186-19871035. This is the phone's updated Android 10 firmware, not its original shipped version. The public InputDevice battery state API is API 31+; on API 29 the Steam Controller battery slot must stay explicitly unknown until independently verified native BLE/GATT data can supply it. Do not use hidden Android methods or fabricate battery readings.

Stage A exposes an active-controller selection between Automatic, Daydream and the already Android-connected original 2015 Steam Controller. Selecting Steam uses Android HID without USB/receiver, discovery, Location checks or unpairing. Selection governs VR entry, input routing, digital-twin appearance and power HUD ownership. Steam lizard-mode mouse motion can aim the virtual pointer and primary mouse click selects shell UI; native Valve reports/gyro/independent pads/haptics/battery remain unverified. This slice is source-only until Android build and real Galaxy S9 dual-pairing/reconnect/device tests.

## Bonded Bluetooth Battery Service probe — Android 10 (2026-10-11)

On the actual Galaxy S9 API 29 firmware, Android's API 31+ InputDevice battery interface is absent. When the original Steam Controller is explicitly selected and already enumerated by Android HID, the shell now activates a read-only asynchronous probe against an already **bonded** BLE controller's standard Battery Service (0x180F)/Battery Level characteristic (0x2A19). This does not scan, request Location, open any USB device, or send proprietary mode commands. A successful 1-byte 0–100 GATT reading populates the controller battery slot and is refreshed every two minutes while selected. Missing service, unavailable permissions, detach, failed read, timeout and controller switch leave the battery as **unknown**. No guessed voltage-to-percentage map is used. The original controller's BLE firmware may not expose 0x2A19; until independently tested on S9 this remains a conditional capability and proper native Valve battery/status packet support remains outstanding. A pure Java GATT decoder regression test covers invalid and boundary levels.

## Galaxy S9 observed original-controller alias repair — 2026-10-11

User-supplied Android 10 / One UI 2.5 Bluetooth screenshot lists the paired, reportedly actively used 2015 original controller as **`SteamController`** (one word). Prior `SteamControllerIdentity.matchesAndroidHid` recognized only `Steam Controller` and `Valve Steam Controller` and therefore rejected the actual advertised name. The single authoritative matcher now recognizes explicit bounded original-controller aliases, including `SteamController`, and is used by Android HID device detection, the active selector, virtual controller mouse source matching and the bonded-BLE battery probe. Regression cases verify the no-space alias with unknown VID 0 and Valve VID 0x28de, and reject other vendors, `Steam Deck`, generic `Steam` and 2026 controller labels.

**Do not conflate paired with connected:** Samsung's paired-device list does not alone prove Android has enumerated a live HID input device. `Enter VR` still requires a live, nonvirtual Android InputDevice reporting mouse, keyboard or gamepad sources, or an explicitly ready Daydream provider. If the new APK continues to display disconnected during actual controller input, collect an `HidInputInspector.enumerate()` diagnostic and associated actual HID event IDs / device sources on the S9, without assuming the Bluetooth friendly name equals Android's InputDevice name. No Location permission checks or USB receiver. The reported screenshot is real hardware identity evidence, not an end-to-end HID acceptance test.
