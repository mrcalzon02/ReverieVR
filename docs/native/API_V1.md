# ReverieVR Native Module API v1

**Status:** implemented ABI. This document describes the current game-facing C contract and the rules required to use it safely.

**Authoritative declaration:** `app/src/main/jni/native/reverie_native_module.h`

**Synchronization:** this file is governed by `docs/native/CONTRACT_INDEX.md`. Changes to the ABI header, native host services, or `NativeModuleRuntime` require same-change review/update of this reference and should pass `python3 scripts/verify-native-doc-sync.py`.

## ABI identity

- ABI version: `REVERIE_NATIVE_MODULE_ABI_VERSION == 1`
- exported module entry symbol: `reverie_native_module_entry_v1`
- module API type: `ReverieNativeModuleApiV1`
- host service type: `ReverieNativeHostV1`
- input type: `ReverieNativeInputV1`
- eye type: `ReverieNativeEyeV1`
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
- `REVERIE_NATIVE_DESCRIPTOR_V1_MIN_SIZE` — mandatory module descriptor fields;
- `REVERIE_NATIVE_INPUT_V1_BASE_MIN_SIZE` — base timing/movement/button input;
- `REVERIE_NATIVE_INPUT_V1_POINTER_MIN_SIZE` — input prefix through world-space pointer data;
- `REVERIE_NATIVE_EYE_V1_MIN_SIZE` — mandatory eye matrices;
- `REVERIE_NATIVE_MODULE_API_V1_MIN_SIZE` — mandatory module callback table.

The native host and both reference modules use these prefixes. This makes the append-only rule operational: a future optional tail extension does not make an older otherwise-valid v1 prefix fail merely because the newest header's `sizeof` increased.

Adding optional tail fields can remain ABI v1 when old producers/consumers can safely ignore them. Adding a new mandatory callback, changing existing semantics incompatibly, or requiring a different mandatory layout requires a new ABI version.

## Module descriptor

`ReverieNativeModuleDescriptorV1` contains:

- structure size;
- ABI version;
- stable module id;
- display name;
- required OpenGL ES major/minor version.

The current host requires the descriptor id to match the allowlisted package entry.

Module ids are treated as storage/security identifiers, not display strings. Current runtime policy accepts simple ids made from letters, digits, `-`, `_`, and `.`, with a maximum length of 80 characters.

## Host services

`ReverieNativeHostV1` currently exposes three services.

### Logging

`log(level, tag, message)`

Use this for bounded module diagnostics. Do not emit high-volume per-frame logs during normal operation.

Supported levels are debug, info, warn, and error.

### Read save

`read_save(slot, buffer, capacity, out_size)`

### Write save

`write_save(slot, data, size)`

Save slots are scoped by the host to the active packaged module. A module does not receive an arbitrary filesystem path.

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

The host bounds frame delta and normalizes movement axes before calling the module.

### Pointer kinds

- `REVERIE_NATIVE_POINTER_NONE`
- `REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER`
- `REVERIE_NATIVE_POINTER_VIRTUAL_CONTROLLER`

The pointer is a host-calibrated world-space ray. Modules should use it directly for interaction and must not reconstruct controller orientation from raw Android or transport-specific data.

The pointer direction is expected to be normalized by the host. Defensive normalization inside sensitive geometry code is still acceptable.

## Eye contract

`ReverieNativeEyeV1` supplies:

- structure size;
- eye index;
- 4x4 view matrix;
- 4x4 projection matrix.

The shell/Cardboard path owns head tracking, optical projection, eye presentation, viewport/scissor isolation, and current comfort calibration.

A module renders the supplied eye. It does not derive its own Daydream/Cardboard headset model.

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

## API evolution policy

The default rule is **append before replace**.

Small optional services should be added as tail extensions when old v1 modules remain safe. Cross-cutting capability that cannot be added compatibly should become ABI v2.

Before adding a host service, prove that at least one real module needs it and that it belongs to the platform rather than one game's private logic.

Likely future candidates include shell-owned audio submission, haptic requests where hardware supports them, comfort/profile queries, richer action state, and shared generated-asset helpers. These are candidates only; they are not part of ABI v1 until implemented in the authoritative header and host.
