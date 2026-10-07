# ReverieVR Native Module API v1

**Status:** implemented ABI. This document describes the current game-facing C contract and the rules required to use it safely.

**Authoritative declaration:** `app/src/main/jni/native/reverie_native_module.h`

**Synchronization:** this file is governed by `docs/native/CONTRACT_INDEX.md`. Changes to the ABI header, native host services, or `NativeModuleRuntime` require same-change review/update of this reference and should pass both `python3 scripts/verify-native-doc-sync.py` and the host-buildable `python3 scripts/verify-native-abi-contract.py` gate when ABI structure/consumption behavior is affected.

## ABI identity

- ABI version: `REVERIE_NATIVE_MODULE_ABI_VERSION == 1`
- exported module entry symbol: `reverie_native_module_entry_v1`
- module API type: `ReverieNativeModuleApiV1`
- host service type: `ReverieNativeHostV1`
- input type: `ReverieNativeInputV1`
- eye type: `ReverieNativeEyeV1`
- optional capability type: `ReverieNativeModuleCapabilitiesV1`
- maximum single save payload: `65536` bytes

Native modules are trusted libraries packaged inside the APK and admitted through the host's compile-time allowlist. ABI v1 is deliberately *not* a plugin loader for arbitrary code from writable storage.

## Entry point

Every ABI-v1 module exports:

`const ReverieNativeModuleApiV1 *reverie_native_module_entry_v1(void)`

The returned table is owned by the module and must remain valid for the loaded library lifetime.

The host validates the API table, descriptor, ABI version, required callbacks, OpenGL ES requirement, and module id before creating an instance.

## Structure compatibility

Every public ABI structure begins with `struct_size`. ABI v1 extensions are append-only.

A consumer must validate that a structure is large enough for the **fields it actually intends to access**, using the named minimum-prefix constants in the header rather than comparing against the newest total `sizeof(struct)`. Existing fields must not be reordered or reinterpreted.

Current minimum-prefix constants are:

- `REVERIE_NATIVE_HOST_V1_LOG_MIN_SIZE` — host header plus logging service;
- `REVERIE_NATIVE_HOST_V1_SAVE_MIN_SIZE` — host prefix through read/write save services;
- `REVERIE_NATIVE_HOST_V1_FEEDBACK_MIN_SIZE` — host prefix through shell feedback cue requests;
- `REVERIE_NATIVE_DESCRIPTOR_V1_SIZE` — exact frozen ABI-v1 descriptor size;
- `REVERIE_NATIVE_DESCRIPTOR_V1_MIN_SIZE` — compatibility alias to that exact frozen size;
- `REVERIE_NATIVE_INPUT_V1_BASE_MIN_SIZE` — base timing/movement/button input;
- `REVERIE_NATIVE_INPUT_V1_POINTER_MIN_SIZE` — input prefix through world-space pointer data;
- `REVERIE_NATIVE_EYE_V1_MIN_SIZE` — mandatory eye matrices;
- `REVERIE_NATIVE_MODULE_API_V1_MIN_SIZE` — mandatory module callback table through `render_eye`;
- `REVERIE_NATIVE_MODULE_API_V1_CAPABILITIES_MIN_SIZE` — optional API tail through the `capabilities` pointer;
- `REVERIE_NATIVE_CAPABILITIES_V1_LOCOMOTION_MIN_SIZE` — capability block prefix through shell-locomotion bounds.

The native host and both reference modules use these prefixes. This makes the append-only rule operational for structures whose tails can actually grow without moving existing fields.

### Descriptor layout exception

`ReverieNativeModuleDescriptorV1` is **layout-frozen** for ABI v1. It is embedded by value inside `ReverieNativeModuleApiV1` before the callback pointers, so appending a descriptor field would shift every callback offset. The v1 helper therefore requires `descriptor.struct_size == REVERIE_NATIVE_DESCRIPTOR_V1_SIZE`.

Do not place future capability declarations by appending to the v1 descriptor. The implemented `ReverieNativeModuleCapabilitiesV1` pointer demonstrates the compatible pattern: add optional metadata at the **tail of `ReverieNativeModuleApiV1`** and prove its presence from `struct_size`, or introduce ABI v2 when the change cannot remain optional.

Adding optional tail fields can remain ABI v1 only when all existing field offsets stay unchanged and old producers/consumers can safely ignore the added tail. Adding a new mandatory callback, changing existing semantics incompatibly, extending the embedded v1 descriptor, or requiring a different mandatory layout requires a new ABI version.

## Module descriptor

`ReverieNativeModuleDescriptorV1` contains:

- structure size;
- ABI version;
- stable module id;
- display name;
- required OpenGL ES major/minor version.

The current host requires the descriptor id to match the allowlisted package entry.

The native module host currently provides **OpenGL ES 2.0**. It compares both descriptor version components through `ReverieNativeGlesRequirementSupportedV1`: requirements below 2.x are admitted, 2.0 is admitted, and requirements above 2.0 (including 2.1 or 3.0) are rejected before module creation.

Module ids are treated as storage/security identifiers, not display strings. Current runtime policy accepts simple ids made from letters, digits, `-`, `_`, and `.`, with a maximum length of 80 characters.

## Host services

`ReverieNativeHostV1` currently exposes four services.

### Logging

`log(level, tag, message)`

Use this for bounded module diagnostics. Do not emit high-volume per-frame logs during normal operation.

Supported levels are debug, info, warn, and error.

### Read save

`read_save(slot, buffer, capacity, out_size)`

### Write save

`write_save(slot, data, size)`

Save slots are scoped by the host to the active packaged module. A module does not receive an arbitrary filesystem path.

### Shell feedback cues

`request_feedback(flags)`

This append-only host tail accepts a bitmask of implemented shell cue requests:

- `REVERIE_NATIVE_FEEDBACK_FOCUS`;
- `REVERIE_NATIVE_FEEDBACK_ACTIVATION`;
- `REVERIE_NATIVE_FEEDBACK_FAILURE`.

Use `ReverieNativeHostSupportsFeedbackV1` / `ReverieNativeRequestFeedbackV1` so an older v1 host that ends at the save-service prefix remains safe.

The current host coalesces repeated flags during a native update and returns the mask through the existing JNI update path. The Android shell maps those flags to its existing `UiFeedback` audio cues.

This is **not** an arbitrary audio submission/mixing API and does **not** provide haptics. Modules must not describe either capability as implemented through this service.

Slot names are simple filenames. The host owns directory isolation and path validation.

A single write is bounded by `REVERIE_NATIVE_SAVE_MAX_BYTES`.

Save data remains the module's responsibility: use an explicit schema/version, validate length/content before accepting it, and fail safely to a known state when data is missing or corrupt.

## Input contract

`ReverieNativeInputV1` currently supplies:

- `delta_seconds`;
- `move_x`;
- `move_y`;
- `primary_down`;
- `secondary_down`;
- pointer kind;
- pointer origin;
- pointer direction.

The JNI host applies the shared `ReverieNativeSanitizeBaseInputV1` helper before invoking a module: non-finite (NaN/infinite) frame delta and movement axes become neutral **zero**, finite frame delta is clamped to **0–0.1 s**, movement axes to **−1..1**, and button flags to **0/1**. Malformed floating-point input must never become unintended full-strength movement. A game may sanitize a writable input copy again when it needs to defend against another ABI-compatible host. This does not remap controls or grant shell locomotion.

### Pointer kinds

- `REVERIE_NATIVE_POINTER_NONE`
- `REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER`
- `REVERIE_NATIVE_POINTER_VIRTUAL_CONTROLLER`

The pointer is a host-calibrated world-space ray. Modules should use it directly for interaction and must not reconstruct controller orientation from raw Android or transport-specific data.

The JNI host calls the shared `ReverieNativeSanitizePointerV1` helper: finite rays are normalized with an overflow-safe scaled calculation; unknown kinds, non-finite values, near-zero directions, or origins beyond ±1000 m per axis become `REVERIE_NATIVE_POINTER_NONE` with cleared ray data. Native games may reapply this helper to a writable input copy before hit testing. The ABI layout is unchanged.

## Eye contract

`ReverieNativeEyeV1` supplies:

- structure size;
- eye index;
- 4x4 view matrix;
- 4x4 projection matrix.

The shell/Cardboard path owns head tracking, optical projection, eye presentation, viewport/scissor isolation, and current comfort calibration.

A module renders the supplied eye. It does not derive its own Daydream/Cardboard headset model.

Before dispatching `render_eye`, the JNI host uses `ReverieNativeEyeRenderableV1` to require eye index **0 or 1**, the minimum eye prefix, and finite values in all 16 view plus 16 projection floats. Invalid eye data is rejected before guest GLES calls. Both reference games apply the same guard for alternate compatible hosts. This is a safety/compatibility check, **not** an optical-calibration or matrix-invertibility guarantee; the shell does not alter the supplied matrices.

## Lifecycle

### create

`create(const ReverieNativeHostV1 *host)`

Allocate non-GL module state, validate host ABI/size, keep only services you are allowed to call, and return an opaque instance.

Do not assume a GL context exists here.

### destroy

`destroy(instance)`

Release non-GL state. Destruction must remain safe even after a partial start or failed GL setup.

### on_gl_context_created

Create shaders, textures, buffers, and other GL resources. Procedural materials and immutable generated geometry should normally be built here.

Return zero on failure so the host can fail closed.

### release_gl_context

Delete GL resources while the context is valid. Do not make ordinary object destruction depend on GL calls occurring on an arbitrary teardown thread.

### resume

Resume module-level activity after host lifecycle resume.

### pause

Stop or quiesce module-level activity before lifecycle pause, shell transition, or stop.

### update

Advance simulation from normalized input.

This is the correct place for timers, deterministic animation state, input edge detection, AI, economic simulation, and game-state mutation.

### render_eye

Render one eye using the supplied matrices.

Rendering must not advance persistent simulation. The left and right eye must observe the same logical frame.

## Stereo invariants

The host may call `render_eye` twice per logical frame. Therefore modules must not:

- increment timers during `render_eye`;
- consume one-shot input during `render_eye`;
- advance RNG during `render_eye`;
- mutate AI/economy state during `render_eye`;
- generate different procedural content per eye.

Any shared flicker, animation phase, target state, or material choice should be decided during `update` or from stable state before either eye renders.

## GL-state contract

A module is a guest inside the shell's GL context.

It may bind and configure the resources it needs, but it must restore host-visible state that it changes. At minimum inspect/restore the state actually touched by the module, such as program, buffers, textures, enabled attributes, depth/blend/cull state, and related bindings.

Do not assume that the module owns the viewport, scissor rectangle, framebuffer, or the entire render pass.

## Locomotion contract

ReverieVR now has shell-owned bounded view-relative movement for native 3D scenes. That camera translation is applied consistently to both eyes and the controller/pointer frame.

ABI v1 movement axes remain available for game logic and compatibility, but a module must not apply a second copy of shell locomotion to its own eye matrices.

Game-specific movement mechanics that are not the shared player translation—vehicles, mounted tools, levers, cursors, menu navigation—may still consume appropriate normalized actions.

## Trust and packaging

The host intentionally loads only known packaged native libraries.

A valid module must:

- be part of the APK build;
- have an allowlisted module id/library pairing;
- export the v1 entry symbol;
- pass descriptor/API validation;
- satisfy the host graphics requirement;
- fail closed when required callbacks/resources are unavailable.

Do not design game distribution around downloading executable native modules into app-writable storage under ABI v1.

## Optional module capabilities

ABI v1 now uses its first optional API-tail extension for module-declared platform capabilities.

A module may append a pointer to `ReverieNativeModuleCapabilitiesV1` at the tail of `ReverieNativeModuleApiV1`. An older v1 binary whose `struct_size` ends at `render_eye` remains valid; the host must not read `api.capabilities` until `struct_size >= REVERIE_NATIVE_MODULE_API_V1_CAPABILITIES_MIN_SIZE`.

The current capability flag is:

- `REVERIE_NATIVE_CAPABILITY_SHELL_LOCOMOTION` — requests shell-owned bounded horizontal touchpad locomotion.

When that flag is present, the capability block must include positive finite `locomotion_limit_x` and `locomotion_limit_z` values. The host validates an upper safety bound before exposing the envelope to the Java runtime.

Capabilities describe reusable platform behavior. They are not a route for embedding arbitrary game rules in the ABI.

A module with no capability tail or a null capability pointer continues to run; it simply receives none of the optional shell behaviors represented there.

## API evolution policy

The default rule is **append before replace**.

Small optional services should be added as tail extensions when old v1 modules remain safe. Cross-cutting capability that cannot be added compatibly should become ABI v2.

Before adding a host service, prove that at least one real module needs it and that it belongs to the platform rather than one game's private logic.

Likely future candidates include shell-owned audio submission, haptic requests where hardware supports them, comfort/profile queries, richer action state, and shared generated-asset helpers. These are candidates only; they are not part of ABI v1 until implemented in the authoritative header and host.
