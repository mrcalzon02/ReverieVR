# ReverieVR Native Core Utilities

**Status:** implemented provisional shared utility layer.

**Authoritative implementations:**

- `app/src/main/jni/native/reverie_native_math.h`
- `app/src/main/jni/native/reverie_native_gl_utils.h`

**Synchronization:** governed by `docs/native/CONTRACT_INDEX.md`. These utilities exist because at least two real native modules independently needed the same implementation.

## Purpose

The native SDK should centralize small cross-game correctness mechanisms without becoming a general engine framework.

A utility graduates here only when duplicated real module code demonstrates the need and the behavior is independent of game rules.

## Matrix utility

### `ReverieNativeMat4Multiply`

Multiplies two column-major 4×4 matrices in the convention used by the native GLES2 modules.

Properties:

- output is 16 floats;
- a temporary result makes `out == a` or `out == b` safe;
- null pointers fail without writing;
- it performs no allocation;
- it does not infer row-major/column-major conversion.

Test Chamber and Red Ledger use this for projection/view/model composition instead of maintaining private copies.

## Shader utility

### `ReverieNativeCompileShader`

Compiles a single GLES shader and returns its object id, or zero on failure.

The helper:

- rejects a null source;
- creates the shader;
- submits and compiles source;
- checks `GL_COMPILE_STATUS`;
- deletes a failed shader before returning zero.

It deliberately does **not** link programs, bind attributes, choose shader language/features, or emit module-specific diagnostics. Those policies still differ between the two reference games.

## What does not belong here yet

Do not promote something merely because it looks generally useful.

The following remain game-private or elsewhere until reuse proves otherwise:

- Red Ledger work-target/ray-AABB logic;
- Red Ledger cuboid/world recipes;
- game simulation state machines;
- specific material palettes;
- shader programs;
- gameplay transforms and reach values.

Shared GL state isolation belongs in `reverie_native_gl_state.h`, while ABI capability checks belong in `reverie_native_sdk.h`.

## Growth rule

When another duplicated utility appears:

1. prove both implementations express the same contract;
2. choose the smallest common behavior;
3. move it into a focused shared header/component;
4. migrate the existing modules;
5. add it to the contract index and drift verifier;
6. document limitations as explicitly as capabilities.

This keeps the eventual ReverieVR game SDK composed of proven pieces rather than speculative abstraction.
