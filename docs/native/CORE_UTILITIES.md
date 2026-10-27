## Bounded GLES shader and program-link failure diagnostics (2026-10-13)

The existing `ReverieNativeCompileShader` and `ReverieNativeBuildProgram` APIs remain source-compatible and retain their resource creation/deletion contract. Optional `ReverieNativeCompileShaderDiagnosticV1` and `ReverieNativeBuildProgramDiagnosticV1` variants accept a caller-owned, bounded output buffer. They capture the GLES driver shader/program info log **only on compile/link failure**; normal successful compilation has no extra driver queries or heap allocations. A missing log yields an explicit stage-specific fallback. The six classic games share the same source module and now report these messages through the existing ABI-v1 host logging callback if their shader cannot be created, compiled, or linked. Other modules continue using the original interfaces unchanged.

# ReverieVR Native Core Utilities

**Status:** implemented provisional shared utility layer.

**Authoritative implementations:**

- `app/src/main/jni/native/reverie_native_math.h`
- `app/src/main/jni/native/reverie_native_gl_utils.h`
- `app/src/main/jni/native/reverie_native_flat_mesh.h`

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

It deliberately does not choose shader language/features or emit module-specific diagnostics.

### `ReverieNativeBuildProgram`

Builds one GLES program from caller-supplied vertex/fragment source plus a compact array of `ReverieNativeGlAttributeBindingV1` entries.

The helper owns only the repeated link lifecycle:

- validates source/binding pointers;
- compiles both shaders through `ReverieNativeCompileShader`;
- creates the program;
- attaches shaders;
- applies caller-declared attribute index/name bindings;
- links;
- deletes both temporary shader objects;
- deletes and returns zero for a failed program link.

Shader source, attribute names/index choices, uniform lookups, program-specific validation, and module-facing diagnostics remain game-owned. Test Chamber currently supplies two attribute bindings; Red Ledger supplies four.

## Flat procedural mesh utility

`reverie_native_flat_mesh.h` provides C++17 `Vertex`, `Triangle`, `Rect`, `Segment` and `Disc` routines in `reverie::flatmesh`. They append deterministic XYZ/RGB float vertices to caller-owned vectors, with no bitmaps, GLES imports, ABI changes or new assets. Rectangles/segments use two triangles and discs use twelve. All six classics and both recon boards consume these recipes. Their gameplay, 3x5 font, shader, draw timing, and GPU ownership remain local. This is not yet a serialized model-recipe language; S9 energy benefit is unmeasured.

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

## Reusable licensed Lato native UI font layer (2026-10-26)

The `reverie_native_lato_text.h` GLES2 helper is the shared genuine-font text utility for game-specific HUDs. It consumes the previously licensed, compressed `breakwater_font_atlas.h` Lato Semibold atlas (OFL-1.1), batches true glyph UV quads and creates one alpha texture/shader/VBO per module GL context. `LatoLayer::Clear/Text/Fit` operate during menu rebuild; `Create` and `Release` belong to context lifetime; `Draw(vp)` issues the game-font draw and restores host-owned GL and bound attribute/blend state. Only the source's texture decompression requires the Android system `-lz` library; no new data download, font binary or ABI changes. Physical S9 legibility and sustained performance require separate acceptance. This is for actual UI lettering; decorative 3D signage can remain game-specific.
