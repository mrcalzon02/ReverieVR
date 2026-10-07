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
- coalesced native focus/activation/failure audio cues through shell `UiFeedback`.

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

## Standard native hosted input profile

Native modules launch under one shared `native-standard` binding profile rather than game-id-specific profiles.

The current normalized button mapping is:

- Select → virtual joystick button **0**;
- the reclaimed `VOLUME_UP` input source → virtual joystick button **1**;
- the reclaimed `VOLUME_DOWN` input source → virtual joystick button **2**.

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

Native modules may request the shell's existing focus, activation, and failure cues through the optional `ReverieNativeHostV1.request_feedback` tail.

Requests made during a module update are sanitized and coalesced by flag in the native host. The same JNI update call returns the mask to `NativeModuleRuntime`; `VrShellRenderer` forwards nonzero masks to the activity, which maps them to `UiFeedback.focus()`, `activation()`, and `failure()`.

Test Chamber requests an activation cue when its tint toggle changes. Red Ledger requests activation for completed targeted actions and failure for a targeted action that cannot complete.

The service is deliberately event-level. It does not accept PCM, sound files, spatial audio buffers, arbitrary tones, or mixer controls. Haptics are also not implemented by this path.

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

`PlayerHeadRig` derives the center-head world pose from the adjusted Cardboard head view and keeps left/right eye marker positions at the calibrated IPD. Its small oriented-box collider is currently consumed only for Development logging of controller-proxy overlap transitions; it does not constrain movement or supply a room-scale safety boundary. An allocation-free headset/eye wireframe is available for a future mirror or third-person renderer, but neither that render pass nor a native ABI player-rig service exists yet. The shell does not change the Cardboard eye matrices to produce this geometry. See `docs/records/PLAYER-HEAD-RIG-CONTRACT.md`.
