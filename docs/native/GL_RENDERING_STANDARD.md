# ReverieVR Native GL Rendering and State Standard

**Status:** implemented focused shared GL-state guard; reference-device validation remains pending.

**Authoritative implementation:** `app/src/main/jni/native/reverie_native_gl_state.h`

**Synchronization:** governed by `docs/native/CONTRACT_INDEX.md`. Changes to the shared GL-state component require a same-change update here and must pass the native documentation drift guard.

## Host-context rule

A native module renders inside a GL context owned by ReverieVR/Cardboard. It is a guest renderer, not the owner of the complete render pipeline.

A module may change state needed for its draw, but it must restore every host-visible state it changes before returning from `render_eye`.

## Shared guard

`ReverieNativeGlStateV1` plus `ReverieNativeGlStateCaptureV1` and `ReverieNativeGlStateRestoreV1` currently preserve the shared state changed by the first-party native modules:

- current program;
- current array-buffer binding;
- active texture unit;
- texture-2D binding on texture unit 0;
- depth-test enabled state;
- blend enabled state;
- cull-face enabled state.

`ReverieNativeGlAttribStateV1`, `ReverieNativeGlAttribCaptureV1`, and `ReverieNativeGlAttribRestoreV1` preserve per-attribute state that simple enable/disable cleanup does **not** preserve:

- enabled/disabled state;
- component count;
- stride;
- data type;
- normalized flag;
- vertex-buffer binding;
- pointer/offset;
- current generic four-component attribute value.

The current generic value matters when an attribute array is disabled. Red Ledger uses constant color/tile attributes for dynamic geometry, so restoring only the enable flag would still leak module state into the shell.

## Canonical render pattern

For every attribute index the module will modify:

1. capture global shared state;
2. capture each affected vertex attribute;
3. configure module state and draw;
4. restore affected attributes;
5. restore global shared state;
6. inspect/report GL error according to module policy.

Restore attribute state before restoring the overall array-buffer binding, because restoring an attribute pointer temporarily binds that attribute's original buffer.

## Current first-party use

Procedural Test Chamber captures/restores its position and UV attributes.

Red Ledger captures/restores position, UV, color, and tile-origin attributes, including the constant generic values changed by dynamic cube rendering.

Both modules share the global state guard rather than maintaining separate handwritten snapshots.

## Deliberate boundary

The current guard is focused, not magical. It does **not** currently capture state first-party modules do not modify, including:

- framebuffer binding;
- viewport/scissor rectangles;
- scissor enabled state;
- blend function/equation;
- depth function/range/mask;
- cull-face mode/front-face winding;
- element-array buffer;
- additional texture units or texture targets;
- pixel-store state.

If a native module begins modifying one of those, it must restore it itself or extend the shared guard and this standard in the same scoped change.

Do not expand the guard preemptively merely to mirror the entire OpenGL state machine; capture only state ReverieVR modules actually mutate.

## Stereo rule

State capture/restore occurs independently inside each `render_eye` call. It does not authorize per-eye simulation changes. Game state, RNG, animation phase, and input consumption remain update-time concerns.

## Performance rule

State isolation has a cost because `glGet*` queries can synchronize driver state. Correctness comes first, but repeated hardware measurements should determine whether later shell architecture can reduce the number of required queries by establishing a known render-state contract before module entry.

Until such a contract is proven and documented, do not remove state preservation merely for speed.

## Validation

Static/source validation must verify:

- both first-party native modules use the shared guard;
- every attribute they modify is captured/restored;
- old handwritten partial state restoration is removed;
- the helper/document pair remains in the native contract map.

Android/NDK compilation and Galaxy S9 stereo/runtime tests remain required for device acceptance.
