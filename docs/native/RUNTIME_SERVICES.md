## Omega Front strategic campaign save lifecycle — 2026-10-15

The Development-only native `omega-front` module now uses the existing ABI-v1 `read_save`/`write_save` callbacks for the independent `frontier.v1` slot. This 2,048-byte, versioned and integrity-checked file contains the 16-sector political/ecological/economic map and a nested deterministic combat checkpoint; the former `hearthwell.v1` skirmish slot is not auto-promoted into invalid strategic history. The whole session is validated transactionally before replacing live state. It saves after legitimate map actions or tactical commands, changes of view/pause/outcome, every 16 active battle pulses, and native Pause. Neither rendered eye saves. No new ABI fields or permissions. Host regression/mock-GL reload passed locally; Android NDK/APK, phone persistence, physical controller/thermal results remain unknown.

## Omega Front Daydream war table input — 2026-10-15

The source-wired Development-only `omega-front` game receives `native-classics` input so the Daydream touchpad axes move the highlighted 24×24 tile **without holding down Select**. A click selects the current friendly unit or executes the chosen real tactical order; secondary/App cycles move/attack/repair/resupply while selected, pauses without a selected soldier, and opens the finished after-action report (instead of restarting the battle). Host-normalized pointer rays can select board cells when available. Home and shell recovery remain shell-owned. All game state advances only through update in 250-ms command pulses. Both stereo eyes render the same state, through a managed GLES2 VBO/context lifecycle and original procedural terrain/miniature silhouettes. NDK/APK, physical Daydream input, legibility and thermal verification remain pending.

## Native-game launch handoff on the first eye frame (2026-10-15 diagnostics)

The Samsung Galaxy S9 phone-test bundle (versionCode `214291878`, exported 2026-10-15 21:44 Alaska time) records 22 failed launches across 13 distinct native modules. Each failure followed a successful `Stage B launched` milestone in 0–3 ms, with an empty native last-error string and **no** `Native module GL initialization failed` or `Native module render failed` incident. This is evidence of a shell-level launch/frame sequencing defect, not proof of module shader incompatibility.

Native shelf Select and last-activity resume can enter `MODE_NATIVE` during `onNewFrame` *after* the frame's native GL-initialization dispatch point. The first `onDrawEyeInternal` call must therefore treat `nativeSurfaceReady == false` as initialization **pending**, not as a failed guest eye render. The first pending eye pass draws only the shell pointer/quick-menu overlay and returns; the next `onNewFrame` initializes the guest GL surface, updates simulation once, and subsequent eye passes render the module. Real surface initialization and subsequent guest render errors keep the existing fail-closed reporting and stop/cleanup behavior.

Regression source-order guard: `python3 scripts/verify-native-launch-handoff.py`. This checks the pending-surface path precedes the render-failure path, and ensures actual native GL and render failures remain fatal. **This is a source-level correction verified against the remote tree, not yet a built APK or Galaxy S9 gameplay acceptance.** Reference phone retest must confirm both eyes, real input, and return-to-Home behavior for representative classic games and native applications.

## Gumball Armada native arcade binding and hosted lifetime (2026-10-15)

The Development-only `gumball-armada` module has no shell-locomotion capability. The existing classic touchpad-inclusive native profile is selected for this seated arcade flight experience so the Daydream pad supplies the game's normalized X/Y steering **without holding Select**, while touchpad click supplies held firing; Menu and Home remain shell-owned. In its chart state those same axes choose three immediate routes or four paid ship upgrades, and click activates. Fixed bounded game state advances once per update, not per eye, and uses the existing native module-private save service. The renderer is low-cost planar GLES2 inside the tracked stereoscopic scene. Combo slogans are presently **visual text only**; the planned deep-voice announcer is not implemented and the standard host audio service cannot accept arbitrary voiced PCM. APK and reference-headset acceptance remain pending.

## White-room backdrop proximity and DOS fullscreen separation (2026-10-13)

The white room's rear ceiling, floor, wall, trim, seams and wall-art geometry are drawn from one pre-transformed mesh with the rear depth beyond z=-3.5 compressed by 0.24 instead of 0.52; the back wall now sits around z=-4.1 rather than z=-4.8, closer to the primary screen plane around z=-3.0. This leaves all the physical console screens, controller hit targets, desk, furniture, foreground environment geometry, user IPD, head pose and charge HUD unchanged. The current fullscreen DOS renderer already draws an identity-projected quad across **each full Cardboard eye viewport**, omitting the room; in-room DOS options are hidden until fullscreen is exited, while the separately invoked universal Quick Menu remains available for recovery. Device confirmation remains necessary.

## Daydream semantic controls and unobtrusive event telemetry (2026-10-13)

The original Daydream hardware has only a click-capable capacitive pad, directional touch position, Menu, Home, two volume buttons, 3DoF orientation, gyro and acceleration. It does **not** provide real positional tracking, separate finger-force levels, or dedicated double-tap/hold buttons. The input router now recognizes click **tap** (after the 350ms double-tap window), **double tap**, **press and hold** (threshold 650ms), and a strong **controller acceleration shake** with arming/cooldown; these become named `BindingInput` events in the existing action/host-binding path. Native profile mappings reserve available guest joystick buttons 13, 14 and 15 for tap/double/held respectively. The original select click still activates immediately. Original Menu and Home stay shell-owned; shake remains telemetry-only by default because the controller's independent shake/recenter recovery must not fire an accidental gameplay command. Extreme head-up/down, signed head roll and sharp left/right head turns are emitted as normalized action pulses and diagnostic events but remain unbound by default, preventing accidental guest input. Doom's deliberate pitch deadzone remains the only title-specific head-tilt guest navigation map.

A new independent 448x320 alpha-textured, six-line control history HUD sits around the **middle-left** of each raw eye viewport. Grey monospace lines are translucent, fade over 12 seconds and require no per-frame bitmap upload beyond 500ms fade intervals. It displays only detected transitions/gestures, never every raw tracking packet; the battery HUD retains its own separate texture, vertices and reveal behavior. Per explicit user request the battery arcs and labels are shifted **another 20% down and 20% toward eye center**, on top of their earlier 10% downward placement. Their shape, scale, spacing and texture UV crop are unchanged. This applies to both eyes. Galaxy S9 headset comfort and false-positive gesture rates remain to be verified.

The universal Quick Menu's **Bindings** action also exits DOS fullscreen to the windowed 3D view before opening its configuration pane, matching the Back/options route. The deliberately invoked universal Quick Menu itself may appear temporarily over fullscreen, but no DOS menu may remain layered on the full-eye guest display.

## Full-eye DOS, foreground room proximity and Doom head-tilt navigation (2026-10-13)

DOS `FULLSCREEN: ON` now bypasses HomeEnvironmentRenderer entirely and draws a flat DOS framebuffer across **every pixel of each separate Cardboard eye viewport** with an identity NDC matrix (both eyes, not a floating room screen). This is intentionally full-bleed rather than letterboxed, which can stretch the original 4:3 raster on non-4:3 eye viewports. The shell power HUD keeps its original separate compositor and all headset/view matrices remain unchanged. The DOS options rail is not rendered over fullscreen; Back restores the windowed DOS screen before opening DOS Options, and selecting fullscreen from DOS binding/tuning options automatically resumes the guest. The **universal** Quick Menu remains separately accessible only by intentional activation. Windowed DOS still draws in the world as before.

The White Room rear shell (walls, ceiling, floor rear edge, seams, trim and wall art) has been moved from about z=-6 to z=-4.8 by a one-time mesh transformation, matching its raycast geometry. Screens at z=-3, physical keyboard, dais, furniture, mirror, controller pose, and the other two outdoor environments are unchanged.

Bundled **Doom Shareware** now maps headset forward-vector Y (positive when looking up) to held guest **Up/Forward** and **Down/Back** keys, which also navigate the original Doom keyboard menus. Entry dead zone is ±0.21 in vertical-forward units (~12°), with release at 70% (~8.5°), and the two sources (head tilt and pad directional arrows) do not cancel one another when released. Horizontal Doom turning still uses the existing head yaw relative mouse channel. Original Doom v1.9 does **not** provide vertical free-camera pitch, and this change does not assert otherwise. Other DOS profiles and headset physical pitch matrices are unchanged. Android and Galaxy S9 validation are pending.

## Native game shader error presentation (2026-10-13)

The native Home and library failure footer are short. When the host's full error includes a `detail=` segment from a module's GLES shader compiler/linker, the shell displays that useful bounded driver message first rather than truncating it behind a long JNI prefix. The full module id, native host error and shader detail remain in the diagnostics incident, and the existing fail-closed start/GL/render handling is unchanged. No input, view, controller or battery HUD changes.

## Native classic-game shader diagnostics (2026-10-13)

The six classic modules (Pong, Snake, Chutes and Ladders, Checkers, Go, Chess) use the shared shader builder's new bounded diagnostic variant. If a Galaxy S9 GLES2 driver rejects their vertex shader, fragment shader, or program link, the first-party module reports the failing stage and available driver information through the already-implemented host logging callback. The host carries that bounded guest error into the existing in-headset failure notice, alongside the module id and GLES error code. No new UI, module ABI, game rules, head/controller calibration, successful GL path or charge HUD transformation was introduced. Signed build and real-phone test remain pending.

## Native module GL-context creation isolation (2026-10-13)

An inherited shell GLES error flag can predate a module's shader/VBO setup as well as its eye render. The native host now isolates inherited errors before invoking either `on_gl_context_created` or `render_eye`, logging at most one inherited-error warning per active module session. The module's own lifecycle callbacks and fail-closed policy are unchanged; reported GL creation failures now identify their module and available GL code for the in-headset failure notice. This is not a workaround for failed guest shader compilation or unsupported hardware.

## In-headset native launch failure evidence (2026-10-13)

The native games/apps shelf and Home now show the title id and last bounded native host error when a packaged module fails to start, initialize its GL surface, or render an eye. The runtime still stops failed modules and releases its hosted binding profile; this is **not** a retry loop or a failure bypass. The displayed failure text is cleared by the next successful native-module launch. The existing Android incident diagnostics remain authoritative for the full error and are unchanged. Normal Back/Home and successful game navigation are unchanged; user-facing headset acceptance remains pending.

## Native game entry: GL scissor capability fix (2026-10-13)

The shell now reads the GLES2 `GL_SCISSOR_TEST` enable flag with `GLES20.glIsEnabled`, preserving the original scissor-state restore behavior. The previous `glGetIntegerv(GL_SCISSOR_TEST,...)` query is not the documented capability query and can introduce an inherited `GL_INVALID_ENUM` into the first native draw; every native module checks OpenGL errors when rendering, so the shared error could force a false return to Home. Native host GL-error isolation remains an independent diagnostic defense, not a replacement for correcting the bad shell query. Actual Galaxy S9 native launch and dual-eye render still require physical proof.

## Headset-only pointer beam suppression (2026-10-13)

Shell fallback to headset gaze retains live hit testing, focus highlights and five-second dwell selection, including controller-loss and DOS operation. The projected blue pointer ribbon is now rendered **only** for a live tracked controller or explicitly selected virtual gamepad pointer; losing a controller does not generate a beam from the headset's origin. This changes no native input ray, game rules, actual controller beam, camera pose or system power HUD. Device visual confirmation remains pending.

## DOS mouse, true fullscreen and Quick Menu recovery corrections (2026-10-13)

The **Headset View → Mouse** profile now reverses its two relative mouse scales (yaw -650, pitch +650) in the virtual guest-input mapping only; it does not invert the headset's physical VR view, controller pose or other game/mouse profiles. Fullscreen DOS now occupies each complete Cardboard eye viewport and renders as a head-fixed aspect-fitted 2D DOS image, using nearest-filtered pixels and black letterbox when aspect requires it; windowed DOS retains the existing floating display and options rail. The rail is neither drawn nor ray-hittable in active fullscreen DOS. Back and the universal Quick Menu remain accessible, and DOS Options can toggle back to windowed mode. The Quick Menu has a dedicated **CLOSE MENU** target independent of Home or Exit Phone. Previously controller pose loss/entry auto-opened the modal while hosted; that implicit popup is removed. With no usable controller, an intentional five-second look upward still opens shell recovery; lower the gaze to re-arm after closing. Headset brightness, pose transforms, battery HUD and screen-panel calibration are unchanged. Source fixes require Android unit/build and reference S9 usability proof.

## DOS headset/controller mouse modes retain touchpad navigation (2026-10-13)

The **Headset View → Mouse** and **Controller Aim → Mouse** DOS profiles now expose Daydream touchpad X/Y through four thresholded DOS arrow keys and corresponding RetroPad directional buttons 4–7, while their mouse movement remains headset- or controller-orientation-driven. The existing touchpad release event returns keys/buttons to neutral. **Touchpad → Mouse** deliberately remains swipe-only for guest mouse motion, avoiding unwanted arrow presses; existing standard, FPS and DOOM configurations are unchanged. The shell still owns Quick Menu and recovery. Source regression tests cover the profile distinction; signed APK and reference S9 validation remain pending.

## DOS shell focus ownership and held-input release (2026-10-13)

While DOS gameplay is visible, the shell can take input focus in two ways: hovering the DOS options controls, or opening the universal Quick Menu using the controller or the headset-only recovery gesture. Previously the host suppressed *new* input while these surfaces were active, but a preexisting guest mouse/key/joystick hold or a queued fast Select pulse was not reliably cleared. The DOS shell now explicitly releases the active `InputBindingEngine` when first entering a DOS UI target and whenever the universal Quick Menu opens from DOS. Release also discards queued one-frame taps through the existing `VirtualInputBus.releaseAll()` path. The controller-relative DOS mouse baseline is rearmed when focus changes, preventing a jump on return. DOS gameplay continues under its existing overlay pause behavior; hover alone does not pause or resume emulation. No headset quaternion, controller calibration, battery HUD positioning, NDK ABI or user-facing menu entry changed. Galaxy S9 interaction proof remains pending.

## Hosted input press retention and slow mouse aim (2026-10-13)

The phone's controller/input callbacks and the DOS/native render workers use different frame clocks. Previously a short digital Select/key/joy button pulse that completed between two `VirtualInputBus.capture()` calls could disappear, even though it had been accepted by the binding profile. The bus now preserves each previously unseen rising edge until the **next** consumer frame; a normal sustained hold stays down across frames, and release is visible the next frame. `releaseAll()` discards both current held inputs and queued unseen presses on Quick Menu takeover, profile switching, recovery or disconnect so no click leaks into the resumed game. This is bounded by existing fixed-size input arrays and allocates nothing per frame.

DOS relative mouse also carries subpixel fractions between native frames. The integer motion actually forwarded to DOSBox Pure is now the **same** integer amount used to advance the on-video cursor/absolute pointer. Small deliberate head/controller rotations therefore accumulate into eventual motion instead of being permanently rounded to zero. Pausing/releases discard the remainder. No pose transform, shell HUD, controller calibration or native ABI changed. Source implementation needs APK compilation and Galaxy S9 interaction proof.

## DOS mouse-source selection (2026-10-13 source integration)

DOS Quick Menu → Bindings opens **DOS Mouse Control** with direct choices for **Headset View → Mouse**, **Controller Aim → Mouse**, **Touchpad → Mouse**, **Head + Touchpad**, plus retained **Pad Arrows + Head** and **DOS FPS Head Look** layouts. **Advanced / Tuning** retains profile cycling, sensitivity, deadzone and reset controls; a selection is persisted per imported DOS module using the existing repository metadata and takes effect in its currently running session. All mouse modes keep Select → guest left-click, Android gamepad D-pad / shell recovery, and use the existing DOS libretro relative-mouse input. Controller orientation is read from valid Daydream pose snapshots only while its DOS profile is active, normalized and converted to yaw/pitch deltas; stale samples, loss of tracking and vertical singularities rearm without injecting large jumps. The mode changes guest mouse input, never headset orientation or controller calibration; no separate desktop app or APK is required. Android and S9 end-to-end input acceptance remain unverified.

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
- coalesced native focus/activation/failure and fixed gameplay weapon/reload/artillery/airstrike audio cues through shell `UiFeedback`.

This document covers the services around the module. Exact C ABI fields remain documented in `API_V1.md`.

## Launcher discovery and runtime admission

The in-headset **Native Apps / Projects** shelf lists packaged, admitted native modules (including classic games) plus genuine shell applications: OBJ model viewer/import, local video, packaged music, VR text pad and controller/binding laboratory. **Native Games** is a direct shortcut for classic games and admitted first-party project-game previews. Every packaged native game launches through the existing `NativeModuleRuntime.start` path with the same hosted input and recovery; the shell application handlers remain owned by the shell.

`between-deliveries-red-ledger` and `breakwater-battery` remain Development-only. The native catalogue excludes them outside Development logging, and runtime admission independently enforces the same restriction. Ministry of Intelligence and Forward Detachment have Development-only GLES2 decision-board renderers wired to their actual C++ kernels. They appear in both Apps / Projects and Native Games in Development mode and remain denied in Standard mode. Their kernel cycles are operable using native Select to advance and secondary rocker or gamepad-axis edge to cycle choices. These source prototypes are not full 3D gameplay environments and still require Android build/device evidence. This describes source routing, not Galaxy S9 device or signed APK acceptance.

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

Immersive native modules launch under the shared `native-standard` binding profile. The six packaged `classic-*` games use the `native-classics` variant, which preserves all standard native buttons and HID gamepad axes but additionally forwards the centered Daydream touchpad X/Y to native joystick axes. These classic games do not declare shell-locomotion capability: their analog pad is gameplay direction, not world travel. This corrects otherwise nonfunctional Daydream paddle/turn/board-cursor navigation.

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

Touchpad travel is deliberately **not** mapped into joystick movement by the `native-standard` profile. Shell-owned locomotion consumes it separately when a module declares `REVERIE_NATIVE_CAPABILITY_SHELL_LOCOMOTION`, preventing double movement. The classic-specific `native-classics` profile extends those same button mappings with touchpad axes at a 0.28 deadzone. Controller touch release emits neutral X/Y. `BuiltInBindingProfiles.nativeProfileForModule()` owns the selection policy so `VrActivity` does not hard-code separate classic title IDs.

New immersive game modules retain `native-standard`; only the small classic games opt into the analogue pad contract. This is not a change to head movement, controller calibration, HUD, or native ABI.

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

Native modules may request focus/activation/failure and the five fixed gameplay cues (weapon fire, reload start, reload ready, artillery, airstrike) through the optional `ReverieNativeHostV1.request_feedback` tail.

Requests made during a native update are sanitized/coalesced by flag and returned through `NativeModuleRuntime`; `VrShellRenderer` forwards them to the activity for `UiFeedback.focus()`, `activation()`, `failure()`, `weaponFire()`, `reloadStart()`, `reloadReady()`, `artilleryStrike()`, and `airstrike()`. The five gameplay cues use reusable deterministic static AudioTracks, stopped on pause, with no per-eye sound synthesis. This provides no general PCM or MIDI interface.

Test Chamber requests an activation cue when its tint toggle changes. Red Ledger requests activation for completed targeted actions and failure for a targeted action that cannot complete.

The service remains event-level. It does not accept module PCM, sound files, positional audio buffers, arbitrary tones, or mixer controls. It provides only named one-shot cues. Haptics are also not implemented by this path.

### Ordered first-party weapon cues (2026-10-15)

The new optional `request_sound(uint32_t cue_id)` host tail supports
the five named weapon reports only. Breakwater chooses the ID from its
actual `WeaponKind` on a successful shot; old hosts receive the
unchanged generic `WEAPON_FIRE` bit instead. The host stores up to
eight ordered cue bytes without allocating or losing duplicates,
drains at most once per native update through a packed JNI `long`,
and clears on pause/stop. `VrShellRenderer` forwards each cue through
`VrActivity.onNativeModuleSoundCueRequested` to `UiFeedback` and
`NativeCombatSound`, which preload five additional original
22,050-Hz mono PCM16 reports (machine, heavy, flak, missile, cannon).
The existing generic shot, reload/ready, artillery and airstrike
sounds remain as fallbacks; no secondary music playback is changed.

This is not a streaming/spatial mixer and it contains **no externally
sourced recorded SFX**. NPC volleys, hull impacts, aircraft and ambient
loops are future separately typed cues. Complete Android/NDK build,
signed APK and S9 latency/stereo/thermal tests remain unverified.
See `docs/project/BREAKWATER_SOUND_SOURCES.md`.

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

## Classic board games and hosted input (2026-10-11)

Classic Checkers, Go 9x9 and Classic Chess use the existing native ABI
pointer ray and primary/secondary input and optional movement axes.
Shell stereo, controller identity, pause, Quick Menu, Home, and recovery
remain host-owned; no raw Bluetooth/HID or new ABI services are added.
Simulation and local CPU moves run in Update and not in eye rendering.

## Native Music Library (implemented source; Android acceptance pending)

VR Home opens a phone-resident, offline Music Library. The shell's
`MODE_MUSIC_LIBRARY` is a native 3D-UI interaction panel with three
selectable songs per page, previous/next track, pause/resume, page switching
and return to Home. Input remains controller/gaze routed through the shell;
music playback is owned by `ShellMenuMusic` on the Android UI thread.
`NativeMusicCatalog` reads the bounded immutable
`assets/music/catalog.tsv`, which lists only approved local assets.
The first packaged entry is the owner-authorized Suno Starry Cereal MP3;
future `assets/music/tracks/<id>.mp3` or `.wav` entries are not accessible
until explicitly imported and packaged with exact hashes/rights records.
The media player's `MediaPlayer` is released on track changes or lifecycle
shutdown. Audio pauses when the shell yields to video/DOS/native content,
and does not contact or stream Suno. Official paid downloadable Suno files
and the licensing registry are the only admission path; no stream ripping,
hidden PC runtime or startup network dependency. The
`scripts/import-suno-downloads.py` helper is manual and bounded, validates
owner-rights attestations and hashes, and does not download tracks itself.
Galaxy S9 decoder/frame pacing, full-catalog size and on-device UI
acceptance remain outstanding.
