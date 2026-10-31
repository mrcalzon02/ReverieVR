# ReverieVR Native Creative Suite — Planning Specification

**Status:** Proposed scope; design only, not implemented or accepted.  
**Authority:** Subordinate to `docs/project/INSTRUCTIONS.md`, `docs/project/EXECUTION_CONTRACT.md`, `docs/native/README.md`, and the actual native ABI.  
**Scope:** Offline-first, headset-operable photo, video, 3D-model, and game/module creation applications.

## Vision and boundary

ReverieVR should become a compact **native three-dimensional creative workstation** with a coherent suite of applications living inside the persistent Stage B shell, plus accessible Stage A touchscreen workflows for tasks impractical inside a headset. These are genuine editable workspaces, not simply 2D Android applications projected onto floating panels. Optional flat panels are appropriate for timelines, code, metadata, dialogs, and document previews.

The reference device remains Galaxy S9 + Daydream View (3DoF). All designs must honor 3DoF tracking, thermal limits, frame pacing, low memory, offline use, controller recovery, and shell-owned navigation. A pointing ray, head-look, normalized actions, virtual keyboard, and optional connected gamepad are the baseline. No unverified 6DoF, high-speed GPU compute, or cloud service is assumed.

## Shared platform foundation

Build one set of reusable capabilities instead of four independent editors:

1. **Workspace shell integration:** application launch/exit, pause/resume, recovery, recenter, global battery HUD, controller switching, settings, diagnostics, and safe Stage A fallback remain host-owned.
2. **Spatial workspace UI:** configurable floating/dockable tool panels, object selection ray, transform gizmos, snapping, numeric property editing, hierarchical browser, timeline and inspector widgets. Head-relative panels and world-anchored canvases must remain comfortable to operate while seated.
3. **Input and text:** host-normalized select, drag, confirm, undo/redo, mode switching, zoom/orbit/pan, shortcut bindings, controller and virtual keyboard support. Never hardcode to one controller transport or require incidental touchscreen presses during Stage B.
4. **Local asset library:** Android document-picker/SAF interoperability, import/export, project-specific directories or document URIs, thumbnails, metadata, format/version validation, deterministic project-relative references, bounded cache, missing-asset repair and explicit permission-failure feedback.
5. **Editing kernel:** command-based changes with undo/redo history, transactional save, crash recovery, autosave with configurable limits, explicit dirty state, versioned project format, backup/migration, and safe handling of corrupted or hostile files. Avoid unbounded memory history.
6. **Shared preview/render services:** host-compatible GLES resources, per-eye render separation, context-loss reconstruction, memory/texture budgets, asynchronous CPU tasks with cancellation, and quality fallbacks under thermal pressure. No per-eye simulation or editing mutation.
7. **Tool feedback and accessibility:** focus/hover/pressed/disabled states, audible confirmation and distinct error sound, explanatory errors, legible VR-scale typography, adjustable workspace scale, handedness and comfort settings.
8. **Interoperability:** documented, versioned data boundaries between editor projects, imported media, model assets, and game modules; no proprietary third-party assets bundled without rights.

## Application A — Reverie Photo Studio

**Minimum useful release:** import one photo through SAF; open it on a spatial canvas; zoom/pan; crop, rotate, flip; adjust exposure, brightness, contrast, saturation and white balance; compare original; undo/redo; save editable project; export JPEG or PNG.

**Later expansion:** nondestructive ordered adjustment stack; masks and paint brush with bounded tiling; layers, text/annotations, perspective correction, clone/repair where affordable; stereo photo and equirectangular 360 preview/editing. Clearly distinguish source pixels from VR display transformations.

**Constraints:** CPU/GPU tile processing for large photos, original preservation, EXIF/orientation and color-space integrity, explicit export quality/resolution, memory budget, no silent full-resolution decoding of huge images. Never claim advanced RAW support without a proven decoder.

## Application B — Reverie Video Studio

**Minimum useful release:** import local video/audio; show a frame-accurate-enough preview with explicit seek precision; trim clips; arrange a short multi-clip timeline; reorder/split; apply simple crossfades and audio volume; save an editable project; export using Android-supported codec capabilities. Report actual codecs, container support, output resolution, and errors.

**Later expansion:** multi-track editing, keyframed transforms/opacity, titles/overlays, color adjustments, speed changes, picture-in-picture, audio fades, stereo/180/360 video preview and VR-aware crop/reframing.

**Constraints:** hardware-decoder and encoder capability probes, low-resolution proxy preview, bounded decoded-frame caching, background export with visible progress and cancellation, audio/video sync tests, storage estimates, thermal/battery gating. No promise of desktop-grade multi-stream 4K editing on reference hardware.

## Application C — Reverie Model Workshop

**Minimum useful release:** create/select primitive meshes; vertex/edge/face selection; translate/rotate/scale with world/local axes and snapping; simple extrusion/inset; basic UV/material/color assignment; undo/redo; editable scene hierarchy; import/export a documented interchange subset (prioritize glTF 2.0/GLB after library/license assessment; OBJ is an optional simple mesh path).

**Later expansion:** low-poly retopology helpers, mesh merge/separate, normals/UV tools, texture painting, simple rig/animation preview, collision geometry, LOD and draw-call inspection, procedural mesh recipes, VR scene placement.

**Constraints:** scene graph and CPU mesh representation must remain distinct from GPU buffers; validation for malformed meshes and import bombs; bounded geometry complexity; avoid high-poly sculpting expectations. Prefer native game-ready optimized output with provenance and licensing metadata.

## Application D — Reverie Game Workshop

A user-facing development environment for building **user-authored modules** with a documented API. This is not permission for arbitrary downloaded native binaries to execute inside the shell.

**Minimum useful release:** project creation from a small template; scene/object inspector; placement and transforms; materials/imported model references; actions/events; a small versioned rules/script layer or declarative behavior graph; deterministic test-play in the hosted runtime; save/reload; export/import project bundles; diagnostics and error presentation.

**Language/runtime decision gate:** investigate a safely bounded interpreted scripting layer or restricted declarative bytecode. Compare memory, instruction budget, determinism, tooling, offline viability, and security. Do not pre-commit to Lua, JavaScript, or a custom VM before measured design review. A native source-code editor can follow, but Android on-device NDK compilation must not be assumed available.

**Game authoring API candidates:**
- scene and entity creation, transforms, hierarchy and bounded mesh/material resources;
- simulation tick, clocks, seeded randomness, local save slots;
- collision/picking and simple physics helpers within CPU/performance budgets;
- host-normalized input actions and pointing rays, not raw Bluetooth or Android events;
- spatial audio and UI widgets;
- asset loading from authorized project-relative references;
- event handlers, state transitions, parameter inspectors and prefab/recipe reuse;
- logging, memory/CPU/frame budgets, diagnostics and test fixtures.

**Runtime trust split:** retain the existing packaged/allowlisted **native ABI v1** unchanged for trusted built-in C/C++ games. User-authored bundles run in a separately reviewed restricted content/runtime model, with explicit imports, resource quotas, no direct system/GL/Android/BLE access, no unrestricted filesystem/network, version compatibility checks, and a deterministic failure/recovery route. Never silently treat user packages as loadable `.so` libraries. If future native extension installation is desired, it requires a distinct security architecture and explicit approval.

**Developer delivery:** publish template projects, API reference, schema, format examples, validation CLI or host-side tests, and import/export procedures. Allow desktop-assisted source generation/building as an optional workflow, but keep the baseline project editor useful without it.

## Cross-application workflows

- Photo Studio image/texture -> Model Workshop material -> Game Workshop asset.
- Model Workshop mesh/material/collider -> Game Workshop prefab or level prop.
- Video Studio render -> shell media player or game cutscene asset, subject to decoder support.
- Shared project browser allows editing imported assets without silently breaking other projects; preserve source and create versioned derivatives.

## Phased delivery and dependencies

| Phase | Deliverable | Admission gate |
|---|---|---|
| CS-00 | Capability inventory, UX prototypes, format/codec/security decisions | Existing shell/ABI reuse documented, no speculative API presented as implemented |
| CS-01 | Shared project store, spatial editing widgets, undo/redo, safe import/export | Round-trip and corruption tests, shell recovery intact |
| CS-02 | Photo Studio useful vertical slice | Import -> adjust -> editable save -> export -> reopen verified |
| CS-03 | Model Workshop low-poly vertical slice | Create/edit -> save -> import/export -> native-game preview verified |
| CS-04 | Video Studio short-clip vertical slice | Import -> timeline edit -> export -> synchronized playback verified |
| CS-05 | Game Workshop restricted project/runtime prototype | Template -> edit -> save -> play -> reload under resource/security limits |
| CS-06 | Shared publishing, tutorials, cross-editor round trips | Compatibility/migration, provenance, performance and device gates |

Ordering may change based on dependencies and reference-device measurements; it is not authorization to bypass ongoing shell, controller, crash, or native-game work.

## Engineering and acceptance gates

For each slice, record authoritative source, implementation state, format version, device capabilities, available static/host tests, real handset observations, sustained thermal/frame/memory data, and explicit unsupported behavior. Validate save/load and asset round-trips; malformed inputs; out-of-storage and permission loss; app process death and GL context loss; Stage B exit and Stage A recovery; input selection with Daydream/Steam/other controllers; long-edit undo boundedness; and shell-global status continuity. Do not surface inert controls.

Use the repository's **INTENT -> EXECUTE -> OBSERVE -> VERIFY -> CLAIM** contract. No feature is “implemented” because it appears in this plan. Use authorized GitHub Actions for APK generation when applicable, and verify results; never substitute a container-built APK for required Android device evidence.

## Open architecture decisions

1. Shared scene/document serialization schema and versioning.
2. GL texture/render-target ownership and offscreen editing previews.
3. Photo pixel-processing dependency and image color management.
4. MediaCodec/MediaExtractor/MediaMuxer capabilities and codec fallback policy.
5. glTF/OBJ parser strategy, licenses, mesh limits and import security.
6. Restricted game scripting/behavior format, sandbox/quotas and API evolution.
7. Application workspace persistence across shell module transitions.
8. Project packaging/signing/provenance for local user-shared content.

**Governance:** This is the suite's design/specification document, not a competing execution backlog or acceptance ledger. Implementation work must be decomposed into the authoritative `docs/project/BACKLOG.md` and its incremental development plan before scheduling individual engineering slices.

## CS-01 VR Workshop first runnable slice — 2026-10-30

Project editing is now integrated into the packaged native Test Chamber
as a Home **TOOLS** entry reusing the same trusted module. C++17
`reverie_vr_workshop.h` provides native scene actions and three protected
project save slots; `test_chamber_module.cpp` supplies actual GLES2
scene rendering and the Lato tool menu. The desktop Java editor remains
the more expansive Beckenburg-based companion and does not ship as an
Android Java 17/Swing dependency. Scope, interaction and limitations are
recorded in `docs/native/SCENE_BRIDGE.md`.

Next: validate handset button/pointer behavior, add collision-assisted
transforms, a versioned bundle export/import seam, editor-authored
lighting/audio/animation scene components, and finally a constrained
interpreted project execution format. Neither arbitrary `.so` imports
nor desktop file paths are exposed by this initial headset editor.
