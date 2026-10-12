# ReverieVR Native Runtime Services

**Status:** implemented first-party runtime services; higher-level configuration surface remains provisional and S9 comfort validation is still required where noted.

**Purpose:** document shell-owned services that native games consume indirectly around ABI v1. These services are part of the native-game platform contract even when they are not C ABI functions.

**Synchronization:** governed by `docs/native/CONTRACT_INDEX.md`. Strictly mapped helper changes require same-change updates here. Broader renderer integration is checked through factual anchors plus semantic review.

## Ownership model

A native module receives game-facing input and per-eye matrices through ABI v1, but ReverieVR deliberately keeps several cross-game behaviors in the shell so each module does not reimplement device-specific policy.

Current shell-owned native runtime services include:

- headset/controller coordinate ownership;
- bounded touchpad locomotion for admitted first-party native scenes;
- pointer origin/direction construction;
- Quick Menu and recovery interception;
- lifecycle pause/resume and GL-context handoff;
- global HUD continuity;
- module-private save isolation through the native host;
- coalesced native focus/activation/failure and fixed gameplay weapon/reload audio cues through shell `UiFeedback`.

This document covers the services around the module. Exact C ABI fields remain documented in `API_V1.md`.

## Bounded view-relative locomotion

### Current implementation

The reusable motion integrator is `BoundedViewRelativeLocomotion`.

Current parameters:

- radial touchpad deadzone: **0.28**;
- maximum horizontal speed: **0.70 m/s**;
- per-update time contribution capped at **50 ms**;
- horizontal motion only;
- no artificial rotation;
- no vertical travel;
- no inertia/coasting;
- diagonal input normalized so it is not faster than cardinal input.

Forward/back/strafe are resolved against the headset's current horizontal forward direction, not the controller quaternion.

The shell applies the resulting world translation once to the adjusted head view. The same translated frame is used for controller/pointer placement, so the module must not apply the same locomotion again.

### Module-declared envelopes

Shell locomotion is now an optional ABI-v1 capability rather than a renderer module-id special case. A module opts in with `REVERIE_NATIVE_CAPABILITY_SHELL_LOCOMOTION` and supplies its horizontal envelope through `ReverieNativeModuleCapabilitiesV1`.

Current declarations are:

- Procedural Test Chamber: **±1.55 m X/Z**;
- Between Deliveries: The Red Ledger VR: **±0.70 m lateral, ±0.32 m depth**.

The JNI host validates and exposes those values once at module launch; `NativeModuleRuntime` caches them, and the renderer reads the cached pair without per-frame JNI allocation.

A module with no compatible capability tail, a null capability block, no locomotion flag, or invalid bounds receives no shell locomotion.

### Gesture re-arm safety

`TouchpadLocomotionGate` prevents a touch already in progress from unexpectedly moving the player across:

- native-module entry;
- module changes;
- Quick Menu close/open transitions;
- touchpad click/select;
- controller interruption or blocked input.

A full touch release arms the next movement gesture. A blocked state or click disarms the gate.

The renderer also rejects stale/invalid controller samples. The current stale cutoff is **250 ms**.

This is important because continuous analog travel is a shell service, while touchpad click remains a game/select action.

### Coordinate and stereo invariant

Locomotion modifies one logical camera position before eye rendering. Both Cardboard eyes therefore observe the same player translation.

A native module:

- renders the eye matrices it receives;
- must not add a second copy of shell locomotion;
- may still use ABI movement axes for non-camera game mechanics when appropriate;
- must keep persistent simulation changes in `update`, not per-eye rendering.

## Hardware controller ownership and device-specific remapping

The Stage B controller-source menu has a source-family choice and now a
specific Android USB/BT gamepad selector: repeated selection of the Android
row cycles *unambiguous* connected device descriptors. Only that device
sends KeyEvent/MotionEvent input to the shell/hosted binding engine. When
explicitly selected hardware disconnects, the recovery rule switches to
Auto rather than trapping the headset. AUTO routes only one ordinary gamepad
at a time; original Daydream/Steam/scrcpy priority remains separate.
These source gates have no native ABI change.

This is **not** the planned per-controller physical button remapping UI:
RVBIND1 still supports existing normalized BindingInput → guest output
profiles only. Generic/Xbox 360/DualShock physical layouts, full remap and
game-specific overrides remain RV-0227 under
`docs/project/CONTROLLER_LAYOUTS_AND_BINDINGS.md`. Native modules must continue consuming
normalized hosted inputs and shell-owned recovery, not Android device IDs.

## Standard native hosted input profile

Native modules launch under one shared `native-standard` binding profile rather than game-id-specific profiles.

The current normalized button mapping is:

- Select → virtual joystick button **0**;
- the reclaimed `VOLUME_UP` input source → virtual joystick button **1**;
- the reclaimed `VOLUME_DOWN` input source → virtual joystick button **2**;
- Android gamepad B, X, Y, L1, R1, L2, R2, thumbstick L/R click, and Select
  (when reported as separate `BUTTON_SELECT`) → virtual joystick buttons
  **3–12**, respectively;
- analog left stick X/Y → the host virtual joystick X/Y axes, independent
  of shell-owned Daydream locomotion, with a 0.15 radial per-axis deadzone.

The source now also preserves individually named physical Android HID
A/B/X/Y, shoulders, triggers, stick clicks, Back/Start/Mode and D-pad
digital key edges and analog L/R trigger axes as `BindingInput` values.
A still produces the normalized Select → native button 0. While a native
or DOS experience is actively receiving guest input, a physical Android
`BUTTON_B` is a guest input instead of an immediate shell Back action.
Outside active guest content (including shell menus) it remains Back.
Android system Back and physical Start/Mode keep their independent
shell-owned recovery paths; raw observation does not let a native
module intercept recovery. Android vendor-specific axis conventions and remappable shell
action UI need real controller report capture before a supported mapping
claim. These raw inputs can be used in existing RVBIND1 guest-output
profiles; there is not yet a complete physical remap editor.

Those `VOLUME_*` names identify the Android/hardware input sources; under the native hosted profile they are game buttons and do not perform system volume control. Volume adjustment remains a shell/menu responsibility.

Touchpad travel is deliberately **not** mapped into joystick movement by this profile. Shell-owned locomotion consumes the touchpad separately when the module declares `REVERIE_NATIVE_CAPABILITY_SHELL_LOCOMOTION`, preventing movement from being applied twice.

A newly admitted native game receives this standard profile without adding a game-id branch in `VrActivity`. A future need for materially different native input semantics should become an explicit reusable capability/profile contract rather than another hard-coded module-name switch.

## Pointer service

The shell constructs controller/gaze pointer state in the same translated world frame as the player camera.

ABI v1 receives only normalized pointer kinds/rays:

- tracked controller;
- virtual controller;
- none.

Modules do not parse BLE packets or raw Android/controller quaternions. The JNI boundary uses `ReverieNativeSanitizePointerV1` to clear invalid rays to pointer kind `NONE`; Red Ledger applies the same helper defensively before target intersection.

When the Quick Menu owns interaction, native module updates continue only under the shell's pause/input policy and do not receive a live native pointer ray.

## Recovery and input ownership

The shell reserves platform recovery behavior. Native games cannot become the sole owner of Menu/Home/Back/recenter routing.

Current native-game assumptions:

- Quick Menu remains shell-owned;
- module exit returns through shell lifecycle;
- touchpad click/select is distinct from shell locomotion;
- host-reserved controls do not become module-private bindings;
- movement stops on stale/disconnected/blocked controller state.

## Lifecycle service

The shell owns Android/Cardboard lifecycle and forwards the native lifecycle in a controlled order:

- create module instance;
- resume;
- create GL resources when the render context exists;
- update/render while admitted;
- release GL resources while the context is valid;
- pause/stop;
- destroy instance.

Exact callbacks are defined in `API_V1.md`.

## Persistence service

Native games use the ABI v1 host save functions rather than arbitrary filesystem access.

The shell/runtime chooses the module-private storage root; the native host validates slot names and performs bounded save reads/writes. Save schema/versioning remains game-owned.

## Native feedback cue service

Native modules may request focus/activation/failure and the three fixed gameplay cues (weapon fire, reload start, reload ready) through the optional `ReverieNativeHostV1.request_feedback` tail.

Requests made during a native update are sanitized/coalesced by flag and returned through `NativeModuleRuntime`; `VrShellRenderer` forwards them to the activity for `UiFeedback.focus()`, `activation()`, `failure()`, `weaponFire()`, `reloadStart()`, and `reloadReady()`. The three gameplay cues use reusable deterministic static AudioTracks, stopped on pause, with no per-eye sound synthesis. This provides no general PCM or MIDI interface.

Test Chamber requests an activation cue when its tint toggle changes. Red Ledger requests activation for completed targeted actions and failure for a targeted action that cannot complete.

The service remains event-level. It does not accept module PCM, sound files, positional audio buffers, arbitrary tones, or mixer controls. It provides only named one-shot cues. Haptics are also not implemented by this path.

## Current limitations and next API candidates

The following are **not** yet stable generalized services:

- per-game comfort-policy query beyond the implemented locomotion capability;
- per-module locomotion speed/deadzone policy (the shell still owns the common values);
- standard spatial collision/guardian service;
- shared mounted-tool/world-anchor primitive;
- arbitrary native-game audio submission/mixing or spatial-audio API beyond the implemented cue service;
- haptics;
- generalized generated-model service.

When one of these becomes implemented and reusable, add its authoritative source and normative documentation to the contract index before describing it as part of the SDK.

## Validation expectations

Static/JVM validation currently covers locomotion direction, deadzone, diagonal normalization, bounds, release stop, invalid inputs, frame-gap clamping, input ownership, and gesture re-arm.

Reference-device validation remains required for:

- touchpad polarity and comfort;
- real controller stale/reconnect behavior;
- stereo alignment during movement;
- pointer reach after translation;
- geometry intersections;
- sustained frame/thermal cost.

Until those pass, this is an implemented runtime service with pending device acceptance, not a finished comfort guarantee.

## Shell headset rig (not a native game API)

`PlayerHeadRig` derives the center-head world pose from the adjusted Cardboard head view and keeps left/right eye marker positions at the calibrated IPD. Its small oriented-box collider is currently consumed only for Development logging of controller-proxy overlap transitions; it does not constrain movement or supply a room-scale safety boundary. An allocation-free headset/eye wireframe feeds the implemented white-room diagnostic mirror; a full-scene mirror, third-person renderer and native ABI player-rig service do not exist yet. The shell does not change the Cardboard eye matrices to produce this geometry. See `docs/records/PLAYER-HEAD-RIG-CONTRACT.md`.

The white-room home environment now contains a right-side **diagnostic planar mirror**. `HeadsetMirrorProjection` reflects the rig and active handset proxy using separate calibrated eye origins and clips each segment to the panel. It does not reflect the full scene, represent a tracked body or modify native game rendering. The panel is not a native module SDK service; physical S9 stereo/comfort validation remains pending.

## External HID On/Off in VR Quick Settings (2026-10-12)

RV-0207 now has a persistent **EXTERNAL KEYBOARD + MOUSE: ON/OFF** control on the second row of the in-headset Quick Settings lower section. It defaults On and changes live VR routing for Android keyboard/mouse (Bluetooth, USB, named scrcpy UHID and SDK events with those source flags) while retaining controller/gamepad input, Daydream, headset tracking, Stage A touchscreen recovery and reserved Menu/Back/Quick Menu keyboard navigation. The source classifier separates keyboard/mouse from gamepad and the disabled state consumes optional HID before the text editor or virtual guest bus. Turning it Off resets held standard HID keys and mouse buttons, including releases into the guest even if the shell Quick Menu currently suppresses ordinary guest input. Opening the Quick Menu also releases held guest HID state before taking shell focus, so no stuck key remains while the shell owns control. The available/disconnected input-device state is separate from the enabled preference. JVM policy tests cover On, Off, gamepads and shell recovery. Galaxy S9 + scrcpy/Bluetooth/USB runtime validation and hosted pointer capture remain pending.

### Shared physical keyboard ownership in the VR Home desk (2026-10-12)

The Home desk supports a synchronized **physical-keyboard digital twin** for Bluetooth, USB HID and named scrcpy UHID keyboards reported as real Android `SOURCE_KEYBOARD` devices. The renderer inspects currently attached devices on resume and input hot-plug, excluding virtual Android keyboard devices, gamepads and original Steam Controller hybrid HID, and requires the `External keyboard + mouse` preference to remain On. Logical HID key identity is translated via the existing `StandardHidInputRouter.mapKey`; individual physical key down/up and device IDs are queued to the serialized GL thread alongside Unicode text edits. On-screen keycaps/legends light during held hardware keys, including short tap feedback after release, and follow disconnection/cancellation without stuck modifiers. Digital key presses light the matching cap and update the same caret, editor and modifier rules; physical Shift affects on-screen labels and physical Caps Lock toggles the same editor state. Unmappable/non-US-layout hardware key names are not fabricated; text still comes from Android `KeyEvent.getUnicodeChar()` rather than guessed scan codes. The `External keyboard + mouse` setting must be On. Key events from Android virtual keyboards, nonkeyboard input, gamepads, original Steam Controller hybrid HID devices, and Ctrl/Alt/Meta shortcuts remain outside the Home text editor. Hosted DOS/native sessions continue using the separate `StandardHidInputRouter` to deliver key up/down state through `VirtualInputBus`; the Home editor is not falsely advertised as typing directly into a guest game. Quick Menu stays shell-owned regardless of the external-input setting. Hardware/USB hub presence detection, non-US character layouts, hot-plug, on-device redraw/press latency and Galaxy S9 visual validation are still pending. Neither VR keypresses nor the UI twin sends USB/Bluetooth HID reports back to a physical keyboard or injects privileged Android-wide events.

### Controller-owned keyboard/mouse HID exemption (2026-10-12)

The default-On external keyboard/mouse gate now distinguishes **optional guest HID** from a selected input device performing core VR controller duties. The original Valve Steam Controller's Android BLE keyboard/mouse compatibility profile is controller-owned even when Android does not expose `SOURCE_GAMEPAD`. A scrcpy mouse performing the selected active shell pointer is likewise controller-owned, while an unselected scrcpy mouse remains optional external HID and is gated Off. Selected scrcpy **keyboard** keys are not automatically exempt: they remain external text/guest input except shell-reserved Quick Menu navigation. This avoids disabling the only active headset control path through a setting intended for optional guest peripherals. Device identity checks and controller selection still precede this policy; Gamepad, headset tracking and recovery remain unaffected. JVM tests cover Steam hybrid input, selected/unselected scrcpy mouse, selected-scrcpy keyboard separation, and Off behavior. On-device confirmation remains pending.
